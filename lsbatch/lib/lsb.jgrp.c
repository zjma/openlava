
#include <pwd.h>
#include "lsb.h"

/* lsb_addjgrp()
 */
int
lsb_addjgrp(struct jgrpAdd *jgrpAddPtr, struct jgrpReply **jgrpReplyPtr)
{
    mbdReqType mbdReqtype;
    struct jgrpReq req;
    static struct LSFHeader hdr;
    XDR xdrs, xdrs2;
    char *request_buf;
    char *reply_buf;
    int l;
    int cc;
    struct lsfAuth auth;
    static struct jgrpReply jgrpReply;

    *jgrpReplyPtr = &jgrpReply;

    if (jgrpReply.badJgrpName)
        xdr_lsffree(xdr_jgrpReply, (char *)&jgrpReply, &hdr);

    if (authTicketTokens_(&auth, NULL, NULL) == -1)
        return (-1);

    req.groupSpec  = jgrpAddPtr->groupSpec;
    req.timeEvent  = jgrpAddPtr->timeEvent;
    req.depCond    = jgrpAddPtr->depCond;
    req.destSpec   = "";
    req.submitTime = time(NULL);
    req.delOptions = 0;

    mbdReqtype = BATCH_JGRP_ADD;

    l = sizeof(struct jgrpReq) + strlen(req.groupSpec)
	+ strlen(req.timeEvent) + strlen(req.depCond);
    l = l * sizeof(int);

    if ((request_buf = calloc(l, sizeof(char)) == NULL)) {
        lsberrno = LSBE_NO_MEM;
        return(-1);
    }
    xdrmem_create(&xdrs, request_buf, reqBufSize, XDR_ENCODE);

    hdr.opCode = mbdReqtype;
    if (!xdr_encodeMsg(&xdrs,
		       (char*)&req,
		       &hdr,
		       xdr_jgrpReq,
		       0,
		       &auth)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        free (request_buf);
        return -1;
    }

    if ((cc = callmbd(NULL,
		      request_buf,
		      XDR_GETPOS(&xdrs),
		      &reply_buf,
                      &hdr,
		      NULL,
		      NULL,
		      NULL)) == -1) {
        xdr_destroy(&xdrs);
    	lsberrno = LSBE_LSLIB;
        free (request_buf);
    	return (-1);
    }

    xdr_destroy(&xdrs);
    free (request_buf);

    lsberrno = hdr.opCode;
    if (!cc)
        return(-1);

    if (lsberrno == LSBE_NO_ERROR
	|| lsberrno == LSBE_JGRP_EXIST
	|| lsberrno == LSBE_JOB_EXIST
	|| lsberrno == LSBE_JGRP_NULL
	|| lsberrno == LSBE_JGRP_BAD
	|| lsberrno == LSBE_NO_JOB) {

	xdrmem_create(&xdrs2, reply_buf, XDR_DECODE_SIZE_(cc), XDR_DECODE);

        if (!xdr_jgrpReply(&xdrs2, &jgrpReply, &hdr)) {
	    lsberrno = LSBE_XDR;
            xdr_destroy(&xdrs2);
	    if (cc)
		free(reply_buf);
	    return -1;
        }
        xdr_destroy(&xdrs2);
	if (cc)
	    free(reply_buf);
	if (lsberrno == LSBE_NO_ERROR)
	    return 0;
	return -1;

    }

    if (cc)
        free(reply_buf);

    return -1;


}
/*
 *--------------------------------------------------------------------
 *
 * lsb_modjgrp () --  modify job group definition.
 *
 * Input:
 *     struct jgrpMod *jgrpModPtr  - the pointer to the job group
 *                     modify structure.
 * Output:
 *     struct jgrpReply *jgrpReplyPtr  - the pointer to the job group
 *                     reply structure.
 * Return:
 *     on success, return 0;
 *     otherwise -1, lsberrno and jgrpReplyPtr-> will be set.
 *
 *--------------------------------------------------------------------
 */
int
lsb_modjgrp (struct jgrpMod *jgrpModPtr, struct jgrpReply **jgrpReplyPtr)
{
    mbdReqType mbdReqtype;
    struct jgrpReq req;
    static struct LSFHeader hdr;
    XDR xdrs, xdrs2;
    char *request_buf;
    char *reply_buf;
    int reqBufSize = 0, cc;
    struct lsfAuth auth;
    static struct jgrpReply jgrpReply;

    *jgrpReplyPtr = &jgrpReply;
    if (jgrpReply.badJgrpName)
        xdr_lsffree(xdr_jgrpReply, (char *)&jgrpReply, &hdr);

    if (authTicketTokens_(&auth, NULL, NULL) == -1)
        return (-1);

    req.groupSpec  = jgrpModPtr->jgrp.groupSpec;
    req.destSpec   = jgrpModPtr->destSpec;
    req.delOptions = 0;
    if (!strcmp(jgrpModPtr->jgrp.timeEvent, "n")) {
    	req.timeEvent  = "";
    	req.delOptions |= SUB_TIME_EVENT;
    }
    else {
    	req.timeEvent  = jgrpModPtr->jgrp.timeEvent;
    }
    if (!strcmp(jgrpModPtr->jgrp.depCond, "n")) {
    	req.depCond  = "";
    	req.delOptions |= SUB_DEPEND_COND;
    }
    else {
    	req.depCond  = jgrpModPtr->jgrp.depCond;
    }
    req.submitTime = time(NULL);

    /*
     * marshall request code using xdr
     */

    mbdReqtype = BATCH_JGRP_MOD;
    /* BOB check out the magic number 500 later */
    reqBufSize += sizeof(struct jgrpReq) + strlen (req.groupSpec)
		+ strlen (req.destSpec) + strlen (req.timeEvent)
		+ strlen (req.depCond) + 500;
    if ((request_buf = malloc (reqBufSize)) == NULL) {
        lsberrno = LSBE_NO_MEM;
        return(-1);
    }
    xdrmem_create(&xdrs, request_buf, reqBufSize, XDR_ENCODE);

    hdr.opCode = mbdReqtype;
    if (!xdr_encodeMsg(&xdrs, (char*) &req, &hdr, xdr_jgrpReq, 0, &auth)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        free (request_buf);
        return(-1);
    }

    /* callmbd() to send request and receive reply; callmbd() sets lsberrno */
    if ((cc = callmbd(NULL,request_buf, XDR_GETPOS(&xdrs), &reply_buf,
              &hdr, NULL, NULL, NULL)) == -1)
    {
        xdr_destroy(&xdrs);
	lsberrno = LSBE_LSLIB;
        free (request_buf);
	return (-1);
    }

    xdr_destroy(&xdrs);
    free (request_buf);

    /*
     * de-marshall reply msg
     */
    lsberrno = hdr.opCode;
    if (!cc)
        return(-1);

    /* check to see if it need to decode the reply struct */
    if (lsberrno == LSBE_NO_ERROR ||
        lsberrno == LSBE_JGRP_EXIST ||
        lsberrno == LSBE_JOB_EXIST ||
	lsberrno == LSBE_JGRP_NULL ||
	lsberrno == LSBE_JGRP_BAD ||
        lsberrno == LSBE_NO_JOB) {
	xdrmem_create(&xdrs2, reply_buf, XDR_DECODE_SIZE_(cc), XDR_DECODE);

        if (!xdr_jgrpReply(&xdrs2, &jgrpReply, &hdr)) {
 	    lsberrno = LSBE_XDR;
            xdr_destroy(&xdrs2);
	    if (cc)
	    free(reply_buf);
	    return(-1);
        }
        xdr_destroy(&xdrs2);
	if (cc)
	    free(reply_buf);
	if (lsberrno == LSBE_NO_ERROR)
	    return(0);
	return(-1);

    }
    if (cc)
        free(reply_buf);
    return(-1);


} /* lsb_modjgrp */

/*
 *--------------------------------------------------------------------
 *
 * lsb_cntjgrp () --  control job group status.
 *
 * Input:
 *     struct jgrpCtrl *jgrpCtrlPtr  - the pointer to the job group
 *                     control structure.
 * Output:
 *     struct jgrpReply *jgrpReplyPtr  - the pointer to the job group
 *                     reply structure.
 * Return:
 *     on success, return 0;
 *     otherwise -1, lsberrno and jgrpReplyPtr-> will be set.
 *
 *--------------------------------------------------------------------
 */
int
lsb_cntjgrp (struct jgrpCtrl *jgrpCtrlPtr, struct jgrpReply **jgrpReplyPtr)
{
    mbdReqType mbdReqtype;
    static struct LSFHeader hdr;
    XDR xdrs, xdrs2;
    char *request_buf;
    char *reply_buf;
    int reqBufSize = 0, cc;
    struct lsfAuth auth;

    static struct jgrpReply jgrpReply;

    *jgrpReplyPtr = &jgrpReply;
    if (jgrpReply.badJgrpName)
        xdr_lsffree(xdr_jgrpReply, (char *)&jgrpReply, &hdr);

    if (authTicketTokens_(&auth, NULL, NULL) == -1)
        return (-1);

    /*
     * marshall request code using xdr
     */

    mbdReqtype = BATCH_JGRP_SIG;
	/* BOB check out the magic number 500 later */
    reqBufSize += sizeof(struct jgrpCtrl) + strlen (jgrpCtrlPtr->groupSpec)
		+ 500;
    if ((request_buf = malloc (reqBufSize)) == NULL) {
        lsberrno = LSBE_NO_MEM;
        return(-1);
    }
    xdrmem_create(&xdrs, request_buf, reqBufSize, XDR_ENCODE);

    hdr.opCode = mbdReqtype;
    if (!xdr_encodeMsg(&xdrs, (char*) jgrpCtrlPtr, &hdr, xdr_jgrpCtrl, 0, &auth)) {
        lsberrno = LSBE_XDR;
        xdr_destroy(&xdrs);
        free (request_buf);
        return(-1);
    }

    /* callmbd() to send request and receive reply; callmbd() sets lsberrno */
    if ((cc = callmbd(NULL,request_buf, XDR_GETPOS(&xdrs), &reply_buf,
                      &hdr, NULL, NULL, NULL)) == -1)
    {
        xdr_destroy(&xdrs);
		lsberrno = LSBE_LSLIB;
        free (request_buf);
		return (-1);
    }

    xdr_destroy(&xdrs);
    free (request_buf);

    /*
     * de-marshall reply msg
     */
    lsberrno = hdr.opCode;
    if (!cc)
        return(-1);

    /* check to see if it need to decode the reply struct */
    if (lsberrno == LSBE_NO_ERROR ||
        lsberrno == LSBE_JGRP_EXIST ||
        lsberrno == LSBE_JOB_EXIST ||
        lsberrno == LSBE_JGRP_HASJOB ||
        lsberrno == LSBE_JGRP_CTRL_UNKWN ||
        lsberrno == LSBE_NO_JOB ||
        lsberrno == LSBE_JGRP_BAD ||
	lsberrno == LSBE_JGRP_NULL) {
        xdrmem_create(&xdrs2, reply_buf, XDR_DECODE_SIZE_(cc), XDR_DECODE);

        if (!xdr_jgrpReply(&xdrs2, &jgrpReply, &hdr)) {
	    lsberrno = LSBE_XDR;
            xdr_destroy(&xdrs2);
    	    if (cc)
	    free(reply_buf);
    	    return(-1);
        }
        xdr_destroy(&xdrs2);
        if (cc)
        free(reply_buf);
        if (lsberrno == LSBE_NO_ERROR)
    	    return(0);
        return(-1);

    }
    if (cc)
        free(reply_buf);
    return(-1);


} /* lsb_cntjgrp */

/*
 *--------------------------------------------------------------------
 *
 * lsb_holdjgrp () --  set the job group to hold status
 *
 * Input:
 *     char     *jgrpSpec;   -the name of job group
 *     int      options;     -the option of this API (for future use)
 * Output:
 *     struct   jgrpReply *jgrpReplyPtr  - the pointer to the job group
 *                     reply structure.
 * Return:
 *     on success, return 0;
 *     otherwise -1, lsberrno and jgrpReplyPtr-> will be set.
 *
 *--------------------------------------------------------------------
 */
int
lsb_holdjgrp(char *jgrpSpec, int options, struct jgrpReply **jgrpReplyPtr)
{
    struct jgrpCtrl cnt;

    cnt.groupSpec  = jgrpSpec;
    cnt.ctrlOp     = JGRP_HOLD;
    cnt.options    = options;

    /*
     * call lsb_cntjgrp();
    */

    return(lsb_cntjgrp((struct jgrpCtrl *)&cnt, jgrpReplyPtr));

} /* lsb_holdjgrp */

/*
 *--------------------------------------------------------------------
 *
 * lsb_reljgrp () --  set the job group to release status
 *
 * Input:
 *     char     *jgrpSpec;   -the name of job group
 *     int      options;     -the option of this API "-d" is supported now.
 * Output:
 *     struct jgrpReply *jgrpReplyPtr  - the pointer to the job group
 *                     reply structure.
 * Return:
 *     on success, return 0;
 *     otherwise -1, lsberrno and jgrpReplyPtr-> will be set.
 *
 *--------------------------------------------------------------------
 */
int
lsb_reljgrp(char *jgrpSpec, int options, struct jgrpReply **jgrpReplyPtr)
{
    struct jgrpCtrl cnt;

    cnt.groupSpec  = jgrpSpec;
    cnt.ctrlOp     = JGRP_RELEASE;
    cnt.options    = options;

    /*
     * call lsb_cntjgrp();
    */

    return(lsb_cntjgrp((struct jgrpCtrl *)&cnt, jgrpReplyPtr));

} /* lsb_reljgrp */

/*
 *--------------------------------------------------------------------
 *
 * lsb_deljgrp () --  delete the job group
 *
 * Input:
 *     char     *jgrpSpec;   -the name of job group
 *     int      options;     -the option of this API (for future use)
 * Output:
 *     struct jgrpReply *jgrpReplyPtr  - the pointer to the job group
 *                     reply structure.
 * Return:
 *     on success, return 0;
 *     otherwise -1, lsberrno and jgrpReplyPtr-> will be set.
 *
 *--------------------------------------------------------------------
 */
int
lsb_deljgrp(char *jgrpSpec, int options, struct jgrpReply **jgrpReplyPtr)
{
    struct jgrpCtrl cnt;

    cnt.groupSpec  = jgrpSpec;
    cnt.ctrlOp     = JGRP_DEL;
    cnt.options    = options;

    /*
     * call lsb_cntjgrp();
    */

    return(lsb_cntjgrp((struct jgrpCtrl *)&cnt, jgrpReplyPtr));

} /* lsb_deljgrp */
