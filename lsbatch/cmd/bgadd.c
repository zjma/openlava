static const char *rcsId(const char *id) {return(rcsId(
"@@(#)$Id: bgadd.c,v 2.7 1997/10/23 20:03:00 zfb Exp $"));}
/**************************************************************************
 *
 *                   Load Sharing Facility
 *
 *  Applications: Lsbatch job group adding routine
 *
 **************************************************************************/

#include "cmd.h"
#include <time.h>

void prtAddErrMsg (struct jgrpAdd *, struct jgrpReply *);
/*
 *----------------------------------------------------------------------
 *
 * Usage -- print out command usage.
 *
 *----------------------------------------------------------------------
 */
void
usage (char *cmd)
{
    fprintf(stderr, "Usage: %s [-h] [-V] [-T time_event] [-w desCond] group_spec \n", cmd);
    exit(-1);
}

/*
 *--------------------------------------------------------------------
 *
 * bgadd [-h] [-V] [-T time_event] [-w desCond] group_spec
 *
 *--------------------------------------------------------------------
 */
int
main (int argc, char **argv)
{
    extern  int optind;
    extern  char *optarg;
    int     cc;
    int     timeFlag = FALSE;
    int     condFlag = FALSE;
    static  char *empty = "";
    struct  jgrpAdd jgrpAdd;
    struct  jgrpReply *jgrpReply;

    if (lsb_init(argv[0]) < 0) {
        lsb_perror("lsb_init");
        exit(-1);
    }

    jgrpAdd.timeEvent = empty;
    jgrpAdd.depCond   = empty;
    jgrpAdd.groupSpec = empty;

    while ((cc = getopt(argc, argv, "VhT:w:")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                exit(0);
            case 'T':
	        if (timeFlag == TRUE) {
       	        usage(argv[0]);
       	        exit(-1);
	        }
                timeFlag = TRUE;
	        jgrpAdd.timeEvent = optarg;
                break;
            case 'w':
		if (condFlag == TRUE) {
                    usage(argv[0]);
                    exit(-1);
		}
                condFlag = TRUE;
		jgrpAdd.depCond = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(0);
        }
    }

    if (optind != argc - 1) {
	usage(argv[0]);
	exit(-1);
    }

    if ((jgrpAdd.groupSpec = argv[optind]) == NULL ||
         strlen(jgrpAdd.groupSpec) <= 0) {
	usage(argv[0]);
	exit(-1);
    }

    if (lsb_addjgrp(&jgrpAdd, &jgrpReply)) {
	prtAddErrMsg(&jgrpAdd, jgrpReply);
	return(-1);
    }
    else {
	printf("Job group <%s> is added.\n",
	    jgrpAdd.groupSpec);

    }

    return(0);

} /* main */

/*-------------------------------------------------------------------------
 * prtErrMsg -- Print error messages
 *
 *------------------------------------------------------------------------
*/
void
prtAddErrMsg (struct jgrpAdd *add, struct jgrpReply *reply)
{

    switch (lsberrno) {
	case LSBE_JGRP_EXIST:
	case LSBE_JOB_EXIST:
	case LSBE_JGRP_NULL:
	case LSBE_JGRP_BAD:
	    lsb_perror(reply->badJgrpName);
	    break;
        case LSBE_BAD_TIMEEVENT:
            lsb_perror (add->timeEvent);
            break;
        case LSBE_DEPEND_SYNTAX:
            lsb_perror (add->depCond);
            break;
        case LSBE_NO_JOB:
            lsb_perror (reply->badJgrpName);
            break;
	default:
	    lsb_perror("bgadd failed");
	    break;
    }
} /* prtErrMsg */
