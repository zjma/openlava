/*
 * Copyright (C) 2015 - 2016 David Bigagli
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

#include "lsb.h"

static struct glb_token *
decode_tokens(char *reply, struct LSFHeader *, int *);


struct parameterInfo *
lsb_parameterinfo(char **names, int *numUsers, int options)
{
    mbdReqType mbdReqtype;
    XDR xdrs;
    struct LSFHeader hdr;
    char *request_buf;
    char *reply_buf;
    static struct parameterInfo paramInfo;
    struct parameterInfo *reply;
    static struct infoReq infoReq;
    static int alloc = FALSE;
    int cc = 0;


    infoReq.options = options;
    if (alloc == TRUE) {
        alloc = FALSE;
        FREEUP(infoReq.names);
    }

    if (numUsers)
        infoReq.numNames = *numUsers;
    else
        infoReq.numNames = 0;
    if (names)
        infoReq.names = names;
    else {
        if ((infoReq.names = malloc(sizeof(char *))) == NULL) {
            lsberrno = LSBE_NO_MEM;
            return NULL;
        }
        alloc = TRUE;
        infoReq.names[0] = "";
        cc = 1;
    }
    infoReq.resReq = "";


    mbdReqtype = BATCH_PARAM_INFO;
    cc = sizeof(struct infoReq) + cc * MAXHOSTNAMELEN + cc + 100;
    if ((request_buf = malloc (cc)) == NULL) {
        lsberrno = LSBE_NO_MEM;
        return NULL;
    }
    xdrmem_create(&xdrs, request_buf, cc, XDR_ENCODE);

    hdr.opCode = mbdReqtype;
    if (!xdr_encodeMsg(&xdrs, (char *)&infoReq, &hdr, xdr_infoReq, 0, NULL)) {
        xdr_destroy(&xdrs);
        free (request_buf);
        lsberrno = LSBE_XDR;
        return NULL;
    }


    if ((cc = callmbd (NULL,request_buf, XDR_GETPOS(&xdrs), &reply_buf, &hdr,
                       NULL, NULL, NULL)) == -1) {
        xdr_destroy(&xdrs);
        free (request_buf);
        return NULL;
    }
    xdr_destroy(&xdrs);
    free (request_buf);

    lsberrno = hdr.opCode;
    if (lsberrno == LSBE_NO_ERROR || lsberrno == LSBE_BAD_USER) {
        xdrmem_create(&xdrs, reply_buf, XDR_DECODE_SIZE_(cc), XDR_DECODE);
        reply = &paramInfo;
        if(!xdr_parameterInfo (&xdrs, reply, &hdr)) {
            lsberrno = LSBE_XDR;
            xdr_destroy(&xdrs);
            if (cc)
                free(reply_buf);
            return NULL;
        }
        xdr_destroy(&xdrs);
        if (cc)
            free(reply_buf);
        return(reply);
    }

    if (cc)
        free(reply_buf);
    return NULL;

}

/* lsb_gettokens()
 */
struct glb_token *
lsb_gettokens(const char *master, const char *port, int *num)
{
    struct LSFHeader hdr;
    XDR xdrs;
    char request_buf[sizeof(struct LSFHeader)];
    char *reply_buf;
    int cc;
    struct glb_token *tokens;

    initLSFHeader_(&hdr);
    hdr.opCode = BATCH_TOKEN_INFO;

    xdrmem_create(&xdrs, request_buf, sizeof(request_buf), XDR_ENCODE);

    if (! xdr_LSFHeader(&xdrs, &hdr)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        return NULL;
    }

    if (master != NULL
        && port != NULL) {
        /* fool callmbd into calling a non local MBD
         */
        setenv("MBD_HOST", master, 1);
        setenv("MBD_PORT", port, 1);
    }

    reply_buf = NULL;
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
        return NULL;
    }

    xdr_destroy(&xdrs);
    if (hdr.opCode != LSBE_NO_ERROR) {
        _free_(reply_buf);
        lsberrno = hdr.opCode;
        return NULL;
    }

    tokens = decode_tokens(reply_buf,
                           &hdr,
                           num);
    if (tokens == NULL) {
        _free_(reply_buf);
        return NULL;
    }

    _free_(reply_buf);

    return tokens;
}

static struct glb_token *
decode_tokens(char *reply, struct LSFHeader *hdr, int *num)
{
    int cc;
    XDR xdrs;
    struct glb_token *tokens;

    xdrmem_create(&xdrs, reply, hdr->length, XDR_DECODE);

    if (! xdr_int(&xdrs, num)) {
        xdr_destroy(&xdrs);
        lsberrno = LSBE_PROTOCOL;
        return NULL;
    }

    /* No tokens
     */
    if (*num == 0) {
        lsberrno = LSBE_NO_ERROR;
        return NULL;
    }

    tokens = calloc(*num, sizeof(struct glb_token));

    for (cc = 0; cc < *num; cc++) {

        tokens[cc].name = calloc(MAXLINELEN, sizeof(char));

        if (! xdr_glb_token(&xdrs, &tokens[cc], hdr)) {
            lsberrno = LSBE_XDR;
            free_tokens(*num, tokens);
            *num = 0;
            return NULL;
        }
    }

    return tokens;
}

void
free_tokens(int num, struct glb_token *t)
{
    int cc;

    for (cc = 0; cc < num; cc++) {
        _free_(t[cc].name);
    }

    _free_(t);
}
