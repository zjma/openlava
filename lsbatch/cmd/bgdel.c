static const char *rcsId(const char *id) {return(rcsId(
"@@(#)$Id: bgdel.c,v 2.5 1997/11/14 01:04:40 pangj Exp $"));}
/**************************************************************************
 *
 *                   Load Sharing Facility
 *
 *  Applications: Lsbatch job group deleting routine
 *
 **************************************************************************/

#include "cmd.h"
#include <time.h>

void prtDelErrMsg (struct jgrpReply *);
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
    fprintf(stderr, "Usage: %s [-h] [-V] group_spec \n", cmd);
    exit(-1);
}

/*
 *--------------------------------------------------------------------
 *
 * bgdel [-h] [-V] group_spec
 *
 *--------------------------------------------------------------------
 */
int
main (int argc, char **argv)
{
    extern  int optind;
    extern  char *optarg;
    char    *groupSpec = NULL;
    int     options = 0;
    int     cc;

    struct  jgrpReply *jgrpReply;

    if (lsb_init(argv[0]) < 0) {
        lsb_perror("lsb_init");
        exit(-1);
    }

    while ((cc = getopt(argc, argv, "Vh")) != EOF) {
        switch (cc) {
        case 'V':
            fputs(_LS_VERSION_, stderr);
            exit(0);
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

    if ((groupSpec = argv[optind]) == NULL ||
         strlen(groupSpec) <= 0) {
       	usage(argv[0]);
       	exit(-1);
    }

    if (lsb_deljgrp(groupSpec, options, &jgrpReply)) {
	prtDelErrMsg(jgrpReply);
	return(-1);
    }
    else {
	printf("Job group %s is deleted.\n",
		groupSpec);

    }

    return(0);

} /* main */

/*-------------------------------------------------------------------------
 * prtDelErrMsg -- Print error messages
 *
 *------------------------------------------------------------------------
*/
void
prtDelErrMsg (struct jgrpReply *reply)
{

    switch (lsberrno) {
	case LSBE_JGRP_NULL:
	case LSBE_JGRP_BAD:
	case LSBE_JGRP_HASJOB:
	    lsb_perror(reply->badJgrpName);
	    break;
	default:
	    lsb_perror("bgdel failed");
	    break;
    }
} /* prtErrMsg */
