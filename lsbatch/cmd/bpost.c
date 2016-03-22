/*
 * Copyright (C) 2015 David Bigagli
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA
 *
 */

#include "cmd.h"

void
usage(void)
{
    fprintf(stderr, "\
usage: bpost [-V] [-h] -d \"message...\" jobID\n");
}

int
main(int argc, char **argv)
{
    int cc;
    char *msg;
    LS_LONG_INT *jobIDs;

    if (lsb_init(argv[0]) < 0) {
	lsb_perror("lsb_init");
	return -1;
    }

    while ((cc = getopt(argc, argv, "Vhd:")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                return -1;
            case 'h':
                usage();
                return -1;
            case 'd':
                msg = optarg;
                break;
            default:
                usage();
                return -1;
        }
    }

    if (strlen(msg) > LSB_MAX_MSGSIZE) {
        fprintf(stderr, "bpost: message bigger than %d\n", LSB_MAX_MSGSIZE);
        return -1;
    }

    getJobIds(argc,
              argv,
              NULL,
              NULL,
              NULL,
              NULL,
              &jobIDs,
              0,
              NULL);

    cc = lsb_postjobmsg(jobIDs[0], msg);
    if (cc < 0) {
        lsb_perror("lsb_jobmsg()");
        return -1;
    }

    printf("Message to job %s posted all right.\n", lsb_jobid2str(jobIDs[0]));

    return 0;
}

