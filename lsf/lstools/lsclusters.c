/* 
 * Copyright (C) 2015 Teraproc Inc.
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
#include "../lib/lib.table.h"
#include "../lib/lproto.h"
#include "../intlib/intlibout.h"

#define MAXLISTSIZE 256

extern int makeFields(struct hostLoad *, char *loadval[], char **);
extern int makewideFields(struct hostLoad *, char *loadval[], char **);
extern char *formatHeader(char **, char);
extern char *wideformatHeader(char **, char);
extern char **filterToNames(char *);
extern int num_loadindex;

void
usage(char *cmd)
{
    fprintf(stderr,"Usage:\n%s [-h] [-V] [-l | -w] [cluster_name ...]\n", cmd);
}

int
main(int argc, char **argv)
{
    int i, j, achar, numclusters, listsize;
    struct clusterInfo *clusters;
    char **clustername;
    char longFormat = FALSE;
    char wideFormat = FALSE;
    char status[10];
    char format_title[]="%-14.14s %-8.8s %-15.15s %15.15s %8.8s %8.8s\n";
    char format_content[]="%-14.14s %-8.8s %-15.15s %15.15s %8d %8d\n";

    if (ls_initdebug(argv[0]) < 0) {
        ls_perror("ls_initdebug");
        exit(-1);
    }

    while ((achar = getopt(argc, argv, "hlVw")) != EOF) {
	switch (achar) {
        case 'l':
	    longFormat = TRUE;
	    if (wideFormat == TRUE)
		usage(argv[0]);
	    break;

	case 'w':
	    wideFormat = TRUE;
	    if (longFormat == TRUE)
		usage(argv[0]);
	    break;

	case 'V':
	    fputs(_LS_VERSION_, stderr);
	    exit(0);

	case 'h':
	default:
            usage(argv[0]);
	    exit(0);
        }
    }

    if (optind==argc) {
	clustername=NULL;
	listsize=0;
    }
    else {
	clustername=argv+optind;
	listsize=argc-optind;
    }


    if (listsize >= MAXLISTSIZE) {
        fprintf(stderr, "too many clusters specified (maximum %d)\n",
                MAXLISTSIZE);
        exit(-1);
    }
    TIMEIT(0, (clusters = ls_clusterinfo(NULL, 
		&numclusters, clustername, listsize, 0)), "ls_clusterinfo");


    if (clusters  == NULL ) {
	ls_perror("lsclusters");
        exit(-10);
    } 
    
    printf (format_title, "CLUSTER_NAME","STATUS","MASTER_HOST","ADMIN","HOSTS","SERVERS");
    for (i=0; i<numclusters; i++) {
        if (clusters[i].status==1) 
            strcpy (status, "ok");
        else
            strcpy (status, "unknown");
	printf (format_content, clusters[i].clusterName,
		status,
		clusters[i].masterName,	clusters[i].admins[0],
		clusters[i].numServers+clusters[i].numClients,
		clusters[i].numServers);
    
        if (longFormat) {
            printf ("OpenLava administrators:");
	    for (j=0; j<clusters[i].nAdmins; j++)
                printf (" %s",clusters[i].admins[j]);
            printf ("\nAvailable resources:");
            for (j=0; j<clusters[i].nRes; j++)
                printf (" %s",clusters[i].resources[j]);
            printf ("\nAvailable host types:");
            for (j=0; j<clusters[i].nTypes;j++)
                printf (" %s",clusters[i].hostTypes[j]);
            printf ("\nAvailable host models:");
            for (j=0; j<clusters[i].nModels;j++)
                printf (" %s",clusters[i].hostModels[j]);
            printf ("\n");
        }
    }
    return 0;
}
