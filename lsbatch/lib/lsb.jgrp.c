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
static struct jobGroupInfo *decode_groups(char *,
                                          struct LSFHeader *,
                                          int *);

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
    return operate_jobgroup(BATCH_JGRP_DEL, jgrp);
}

int
lsb_modjgrp(struct job_group *jgrp)
{
    return operate_jobgroup(BATCH_JGRP_MOD, jgrp);
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

/* lsb_getjgrp()
 */
struct jobGroupInfo *
lsb_getjgrp(int *num)
{
    struct jobGroupInfo *jgrp;
    XDR xdrs;
    struct LSFHeader hdr;
    char *reply;
    int cc;
    char buf[sizeof(struct LSFHeader)];

    initLSFHeader_(&hdr);
    hdr.opCode = BATCH_JGRP_INFO;

    xdrmem_create(&xdrs, buf, sizeof(struct LSFHeader), XDR_ENCODE);

    if (! xdr_LSFHeader(&xdrs, &hdr)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        return NULL;
    }

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
        _free_(reply);
        lsberrno = hdr.opCode;
        return NULL;
    }

    jgrp = decode_groups(reply, &hdr, num);
    _free_(reply);

    return jgrp;
}

/* decode_groups()
 */
static struct jobGroupInfo *
decode_groups(char *reply, struct LSFHeader *hdr, int *num)
{
    int cc;
    int i;
    XDR xdrs;
    struct jobGroupInfo *jgrp;

    xdrmem_create(&xdrs, reply, hdr->length, XDR_DECODE);

    if (! xdr_int(&xdrs, num)) {
        xdr_destroy(&xdrs);
        lsberrno = LSBE_PROTOCOL;
        return NULL;
    }

    /* No groups yet.
     */
    if (*num == 0) {
        lsberrno = LSBE_NO_ERROR;
        return NULL;
    }

    jgrp = calloc(*num, sizeof(struct jobGroupInfo));
    for (cc = 0; cc < *num; cc++) {

        jgrp[cc].path = calloc(MAXLINELEN, sizeof(char));
        jgrp[cc].name = calloc(MAXLINELEN, sizeof(char));

        if (! xdr_wrapstring(&xdrs, &jgrp[cc].path)
            || ! xdr_wrapstring(&xdrs, &jgrp[cc].name)) {
            goto pryc;
        }

        for (i = 0; i < NUM_JGRP_COUNTERS; i++) {
            if (! xdr_int(&xdrs, &jgrp[cc].counts[i])) {
                goto pryc;
            }
        }

        if (! xdr_int(&xdrs, &jgrp[cc].max_jobs))
            goto pryc;
    }

    return jgrp;

pryc:
    free_jobgroupinfo(*num, jgrp);
    lsberrno = LSBE_XDR;

    return NULL;
}

/* free_jobgroupinfo()
 */
void
free_jobgroupinfo(int num, struct jobGroupInfo *jgrp)
{
    int cc;

   for (cc = 0; cc < num; cc++) {
        _free_(jgrp[cc].path);
        _free_(jgrp[cc].name);
    }
    _free_(jgrp);
}
