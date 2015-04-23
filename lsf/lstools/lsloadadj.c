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

#include "../lsf.h"

#define MAXLISTSIZE 256

static void usage(char *);
extern int errno;

int
main(int argc, char **argv)
{
    char *resreq;
    struct placeInfo  placeadvice[MAXLISTSIZE];
    char *p, *hname;
    int cc = 0;
    int	achar;

    if (ls_initdebug(argv[0]) < 0) {
        ls_perror("ls_initdebug");
        return -1;
    }

    opterr = 0;
    while ((achar = getopt(argc, argv, "VhR:")) != EOF) {
	switch (achar) {
	case 'R':
	    resreq = optarg;
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

        if (cc >= MAXLISTSIZE) {
	    fprintf(stderr, "%s: too many hostnames (maximum %d)\n",
		__func__, MAXLISTSIZE);
	    usage(argv[0]);
            return -1;
	}

        p = strchr(argv[optind],':');
        if ( (p != NULL) && (*(p+1) != '\0') )  {
             *p++ = '\0';
             placeadvice[cc].numtask = atoi(p);
             if (errno == ERANGE) {
                 fprintf(stderr, "%s: invalid format for number of components\n",
                         __func__);
                 usage(argv[0]);
                 return -1;
             }
        } else {
             placeadvice[cc].numtask = 1;
        }

        if (!Gethostbyname_(argv[optind])) {
	    fprintf(stderr, "\
%s: invalid hostname %s\n", __func__, argv[optind]);
	    usage(argv[0]);
            return -1;
	}
        strcpy(placeadvice[cc++].hostName, argv[optind]);
    }

    if (cc == 0) {
	if ((hname = ls_getmyhostname()) == NULL) {
	    ls_perror("ls_getmyhostname");
            return -1;
        }
        strcpy(placeadvice[0].hostName, hname);
        placeadvice[0].numtask = 1;
        cc = 1;
    }

    if (ls_loadadj(resreq, placeadvice, cc) < 0) {
        ls_perror("lsloadadj");
        return -1;
    }

    return 0;
}

static void usage(char *cmd)
{
    printf("\
usage: %s [-h] [-V] [-R res_req] [host_name[:num_task] \
 host_name[:num_task] ...]\n", cmd);
}
