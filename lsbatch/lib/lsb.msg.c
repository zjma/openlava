/*
 * Copyright (C) 2015 David Bigagli
 * Copyright (C) 2007 Platform Computing Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <pwd.h>
#include "lsb.h"

int
lsb_postjobmsg(LS_LONG_INT jobID, char *msg)
{
    char request_buf[MSGSIZE];
    char *reply_buf;
    XDR xdrs;
    int cc;
    int len;
    struct LSFHeader hdr;
    struct lsfAuth auth;

    if (strlen(msg) > LSB_MAX_MSGSIZE) {
        lsberrno = LSBE_BAD_ARG;
        return -1;
    }

    if (authTicketTokens_(&auth, NULL) == -1)
        return -1;

    xdrmem_create(&xdrs, request_buf, MSGSIZE, XDR_ENCODE);

    initLSFHeader_(&hdr);
    hdr.opCode = BATCH_JOB_MSG;

    XDR_SETPOS(&xdrs, LSF_HEADER_LEN);

    if (!xdr_lsfAuth(&xdrs, &auth, &hdr))
        return FALSE;

    if (! xdr_jobID(&xdrs, &jobID, &hdr)) {
        lsberrno = LSBE_XDR;
        return -1;
    }

    if (! xdr_wrapstring(&xdrs, &msg)) {
        lsberrno = LSBE_XDR;
        return false;
    }

    len = XDR_GETPOS(&xdrs);
    hdr.length = len - LSF_HEADER_LEN;

    XDR_SETPOS(&xdrs, 0);
    if (!xdr_LSFHeader(&xdrs, &hdr))
        return FALSE;

    XDR_SETPOS(&xdrs, len);

    cc = callmbd(NULL,
                 request_buf,
                 XDR_GETPOS(&xdrs),
                 &reply_buf,
		 &hdr,
                 NULL,
                 NULL,
                 NULL);

    if (cc < 0) {
	xdr_destroy(&xdrs);
	return -1;
    }
    xdr_destroy(&xdrs);

    lsberrno = hdr.opCode;
    if (lsberrno == LSBE_NO_ERROR)
        return 0;

    return -1;
}

struct lsbMsg *
lsb_readjobmsg(LS_LONG_INT jobID, int *num)
{
    XDR xdrs;
    struct LSFHeader hdr;
    char request_buf[MSGSIZE/8];
    char *reply_buf;
    struct lsfAuth auth;
    int cc;
    int i;
    struct lsbMsg *msg;

    if (authTicketTokens_(&auth, NULL) == -1) {
        lsberrno = LSBE_BAD_USER;
        return NULL;
    }

    initLSFHeader_(&hdr);
    hdr.opCode = BATCH_JOBMSG_INFO;

    xdrmem_create(&xdrs, request_buf, sizeof(request_buf), XDR_ENCODE);

    if (! xdr_encodeMsg(&xdrs,
                        (char *)&jobID,
                        &hdr,
                        xdr_jobID,
                        0,
                        &auth)) {
        xdr_destroy(&xdrs);
        lsberrno = LSBE_XDR;
        return NULL;
    }

    cc = callmbd(NULL,
                 request_buf,
                 XDR_GETPOS(&xdrs),
                 &reply_buf,
		 &hdr,
                 NULL,
                 NULL,
                 NULL);
    if (cc < 0) {
	xdr_destroy(&xdrs);
        lsberrno = LSBE_PROTOCOL;
	return NULL;
    }
    xdr_destroy(&xdrs);

    xdrmem_create(&xdrs, reply_buf, cc, XDR_DECODE);

    if (! xdr_int(&xdrs, num)) {
        xdr_destroy(&xdrs);
        _free_(reply_buf);
        lsberrno = LSBE_XDR;
        return NULL;
    }

    if (*num == 0) {
        lsberrno = LSBE_NO_ERROR;
        return NULL;
    }

    msg = calloc(*num, sizeof(struct lsbMsg));

    for (i = 0; i < *num; i++) {
        if (! xdr_lsbMsg(&xdrs, &msg[i], &hdr))
            goto via;
    }

    _free_(reply_buf);
    xdr_destroy(&xdrs);

    return msg;

via:
    for (cc = 0; cc < i; cc++)
        _free_(msg[cc].msg);
    _free_(msg);
    lsberrno = LSBE_XDR;
    *num = 0;
    return NULL;
}

/* lsb_jobdep()
 *
 * Get information about job dependencies.
 */
struct job_dep *
lsb_jobdep(LS_LONG_INT jobID, int *num)
{
    XDR xdrs;
    struct LSFHeader hdr;
    char request_buf[MSGSIZE/8];
    char *reply_buf;
    int cc;
    int i;
    struct job_dep *jobdep;

    initLSFHeader_(&hdr);
    hdr.opCode = BATCH_JOBDEP_INFO;

    xdrmem_create(&xdrs, request_buf, sizeof(request_buf), XDR_ENCODE);

    if (! xdr_encodeMsg(&xdrs,
                        (char *)&jobID,
                        &hdr,
                        xdr_jobID,
                        0,
                        NULL)) {
        xdr_destroy(&xdrs);
        lsberrno = LSBE_XDR;
        return NULL;
    }

    cc = callmbd(NULL,
                 request_buf,
                 XDR_GETPOS(&xdrs),
                 &reply_buf,
		 &hdr,
                 NULL,
                 NULL,
                 NULL);
    if (cc < 0) {
	xdr_destroy(&xdrs);
        lsberrno = LSBE_PROTOCOL;
	return NULL;
    }
    xdr_destroy(&xdrs);

    if (hdr.opCode != LSBE_NO_ERROR) {
	_free_(reply_buf);
	lsberrno = hdr.opCode;
	return NULL;
    }

    xdrmem_create(&xdrs, reply_buf, cc, XDR_DECODE);

    if (! xdr_int(&xdrs, num)) {
        xdr_destroy(&xdrs);
        _free_(reply_buf);
        lsberrno = LSBE_XDR;
        return NULL;
    }

    if (*num == 0) {
        lsberrno = LSBE_NO_ERROR;
        return NULL;
    }

    jobdep = calloc(*num, sizeof(struct job_dep));

    for (i = 0; i < *num; i++) {
        if (! xdr_jobdep(&xdrs, &jobdep[i], &hdr))
            goto via;
    }

    _free_(reply_buf);
    xdr_destroy(&xdrs);

    return jobdep;

via:
    free_jobdep(*num, jobdep);
    lsberrno = LSBE_XDR;
    *num = 0;
    return NULL;
}

/* free_jobdep()
 */
void
free_jobdep(int cc, struct job_dep *jobdep)
{
    int i;

    for (i = 0; i < cc; i++) {
	_free_(jobdep[i].dependency);
	_free_(jobdep[i].jobid);
    }

    _free_(jobdep);
}
