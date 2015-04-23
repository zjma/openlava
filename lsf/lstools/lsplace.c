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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>

#include "../lsf.h"

#include "../lib/lproto.h"
#define MAXLISTSIZE 256

void
usage(char *cmd)
{
    fprintf(stderr, "usage: %s [-h] [-V] [-L] [-R res_req] [-n needed] \
 [-w wanted] [host_name ... ]\n", cmd);
}


int
main(int argc, char **argv)
{
    char *resreq = NULL;
    char *hostnames[MAXLISTSIZE];
    char **desthosts;
    int cc = 0;
    int needed = 1;
    int wanted = 1;
    int i;
    char locality=FALSE;
    int	achar;
    char badHost = FALSE;

    if (ls_initdebug(argv[0]) < 0) {
        ls_perror("ls_initdebug");
        return -1;
    }
    if (logclass & (LC_TRACE))
        ls_syslog(LOG_DEBUG, "%s: Entering this routine...", __func__);

    opterr = 0;
    while ((achar = getopt(argc, argv, "VR:Lhn:w:")) != EOF) {
	switch (achar) {
	case 'L':
            locality=TRUE;
            break;
	case 'R':
            resreq = optarg;
            break;
        case 'n':
            for (i = 0 ; optarg[i] ; i++)
                if (! isdigit(optarg[i])) {
                    usage(argv[0]);
                    return -1;
                }
            needed = atoi(optarg);
            break;
        case 'w':
            for (i = 0 ; optarg[i] ; i++)
                if (! isdigit(optarg[i])) {
                    usage(argv[0]);
                    return -1;
                }
            wanted = atoi(optarg);
            break;
	case 'V':
	    fputs(_LS_VERSION_, stderr);
	    return 0;
	case 'h':
            usage(argv[0]);
            return -1;
	default:
            usage(argv[0]);
            return -1;
        }
    }

    for ( ; optind < argc ; optind++) {
        if (cc>=MAXLISTSIZE) {
            fprintf(stderr, "%s: too many hosts specified (max %d)\n",
		argv[0], MAXLISTSIZE);
            return -1;
        }

        if (ls_isclustername(argv[optind]) <= 0
            && !Gethostbyname_(argv[optind])) {
            fprintf(stderr, "\
%s: invalid hostname %s\n", argv[0], argv[optind]);
            badHost = TRUE;
            continue;
        }
        hostnames[cc] = argv[optind];
        cc++;
    }
    if (cc == 0 && badHost)
        return -1;

    if (needed == 0 || wanted == 0)
        wanted = 0;
    else if (needed > wanted)
	wanted = needed;

    if (wanted == needed)
	i = EXACT;
    else
	i = 0;

    i = i | DFT_FROMTYPE;

    if (locality)
        i = i | LOCALITY;

    if (cc == 0)
        desthosts = ls_placereq(resreq, &wanted, i, NULL);
    else
        desthosts = ls_placeofhosts(resreq, &wanted, i, 0, hostnames, cc);

    if (!desthosts) {
        ls_perror("lsplace");
        return -1;
    }

    if (wanted < needed)  {
	fprintf(stderr, "lsplace: %s\n", ls_errmsg[LSE_NO_HOST]);
        return -1;
    }

    for (cc = 0; cc < wanted; cc++)
        printf("%s ", desthosts[cc]);

    printf("\n");

    return 0;
}
