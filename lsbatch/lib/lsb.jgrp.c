/*
 * Copyright (C) 2016 David Bigagli
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

#include "lsb.h"

static int operate_jobgroup(int, struct job_group *);

/* lsb_addjgrp()
 */
int
lsb_addjgrp(struct job_group *jgrp)
{
    return operate_jobgroup(BATCH_JGRP_ADD, jgrp);
}

/* lsb_deljgrp()
 */
int
lsb_deljgrp(struct job_group *jgrp)
{
    return operate_jobgroup(BATCH_JGRP_ADD, jgrp);
}

/* operate_jobgroup()
 */
static int
operate_jobgroup(int opcode, struct job_group *jgrp)
{
    struct LSFHeader hdr;
    XDR xdrs;
    char request_buf[MSGSIZE];
    char *reply_buf;
    int cc;
    struct lsfAuth auth;

    if (authTicketTokens_(&auth, NULL) == -1)
        return -1;

    initLSFHeader_(&hdr);
    hdr.opCode = opcode;

    xdrmem_create(&xdrs, request_buf, sizeof(request_buf), XDR_ENCODE);

    if (!xdr_encodeMsg(&xdrs,
		       (char *)jgrp,
		       &hdr,
		       xdr_jobgroup,
		       0,
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
    	lsberrno = LSBE_LSLIB;
    	return -1;
    }

    xdr_destroy(&xdrs);
    if (hdr.opCode != LSBE_NO_ERROR) {
	_free_(reply_buf);
	lsberrno = hdr.opCode;
	return -1;
    }

    return 0;
}
