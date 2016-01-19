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

/* bresize -a -n 6 -c "/x/y/z" 101
 * bresize -a -H "1:ha" -c "/x/y/z" 101
 * bresize -r -N "6:ha 3:hb" -c "/x/y/z" 101
 * bresize -r -n 3 -c "/x/y/z" 101
 */

#include "cmd.h"

void
usage(void)
{
    fprintf(stderr, "\
usage: bresize [-V] [-h] -a|-r -n num_slots -H \"x:ha y:hb\" \
-c \"/path/to/callback/cmd\", jobID\n");
}

int
main(int argc, char **argv)
{
    int cc;
    int slots;
    int opcode;
    LS_LONG_INT *jobIDs;
    char *hslots;
    char *cmd;

    if (lsb_init(argv[0]) < 0) {
	lsb_perror("lsb_init");
	return -1;
    }

    opcode = -1;
    slots = 0;
    hslots = cmd = NULL;

    while ((cc = getopt(argc, argv, "Vharn:N:c:")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                return -1;
	    case 'a':
		opcode = JOB_RESIZE_ADD;
		break;
	    case 'r':
		opcode = JOB_RESIZE_RELEASE;
		break;
	    case 'H':
		hslots = strdup(optarg);
		break;
	    case 'c':
		cmd = strdup(optarg);
		break;
	    case 'n':
		slots = atoi(optarg);
		break;
            case 'h':
                usage();
                return -1;
            default:
                usage();
                return -1;
        }
    }

    cc = getJobIds(argc,
		   argv,
		   NULL,
		   NULL,
		   NULL,
		   NULL,
		   &jobIDs,
		   0);

    cc = lsb_resizejob(jobIDs[0], slots, hslots, cmd, opcode);
    if (cc < 0) {
        lsb_perror("lsb_resizejob()");
        return -1;
    }

    printf("Job %s is being resized.\n", lsb_jobid2str(jobIDs[0]));

    _free_(hslots);
    _free_(cmd);

    return 0;
}
