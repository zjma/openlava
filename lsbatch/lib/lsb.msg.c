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
lsb_msgjob(LS_LONG_INT jobID, char *msg)
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

