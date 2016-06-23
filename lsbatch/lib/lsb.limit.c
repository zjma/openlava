/*
 * Copyright (C) 2016 Teraproc
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

/* lsb_getlimits()
 */
struct resLimit *
lsb_getlimits(int *num)
{
    XDR xdrs;
    struct LSFHeader hdr;
    char *reply;
    int cc;
    char buf[sizeof(struct LSFHeader)];
    struct resLimitReply limitReply;

    initLSFHeader_(&hdr);
    hdr.opCode = BATCH_RESLIMIT_INFO;

    xdrmem_create(&xdrs, buf, sizeof(struct LSFHeader), XDR_ENCODE);

    if (! xdr_LSFHeader(&xdrs, &hdr)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        return NULL;
    }

    reply = NULL;
    cc = callmbd(NULL,
                 buf,
                 XDR_GETPOS(&xdrs),
                 &reply,
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
        FREEUP(reply);
        lsberrno = hdr.opCode;
        return NULL;
    }

    xdrmem_create(&xdrs, reply, XDR_DECODE_SIZE_(cc), XDR_DECODE);

    if(!xdr_resLimitReply(&xdrs, &limitReply, &hdr)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        if (cc)
            FREEUP(reply);
        *num = 0;
        return NULL;
    }

    xdr_destroy(&xdrs);
    if (cc)
        FREEUP(reply);
    *num = limitReply.numLimits;
    return limitReply.limits;
}

/* free_resLimits()
 */
void
free_resLimits(int num, struct resLimit *limits)
{
    int i, j;

    for (i = 0; i < num; i++) {
        for (j = 0; j < limits[i].nConsumer; j++) {
            FREEUP(limits[i].consumers[j].def);
            FREEUP(limits[i].consumers[j].value);
        }

        FREEUP(limits[i].name);
        FREEUP(limits[i].consumers);
        FREEUP(limits[i].res);
    }
    FREEUP(limits);
}
