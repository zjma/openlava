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
lsb_msgjob(LS_LONG_INT jobId, char *msg)
{
    struct lsbMsg jmsg;
    char *request_buf;
    char *reply_buf;
    XDR xdrs;
    mbdReqType mbdReqtype;
    int cc;
    int len;
    struct LSFHeader hdr;
    struct lsfAuth auth;

    if (authTicketTokens_(&auth, NULL) == -1)
        return -1;

    jmsg.jobId = jobId;
    jmsg.msg = msg;

    len = sizeof(struct lsbMsg) + ALIGNWORD_(strlen(jmsg.msg) + 1);
    len = len * sizeof(int);

    request_buf = calloc(len, sizeof(char));
    if (request_buf == NULL) {
        return -1;

    }

    mbdReqtype = BATCH_JOB_MSG;
    xdrmem_create(&xdrs, request_buf, MSGSIZE, XDR_ENCODE);

    hdr.opCode = mbdReqtype;
    if (!xdr_encodeMsg(&xdrs,
                       (char *)&jmsg,
                       &hdr,
                       xdr_lsbMsg, 0,
		       &auth)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        return -1;
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
	return -1;
    }

    xdr_destroy(&xdrs);
    if (cc)
        free(reply_buf);

    lsberrno = hdr.opCode;
    if (lsberrno == LSBE_NO_ERROR)
        return 0;

    return -1;
}

