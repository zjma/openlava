/*
 * Copyright (C) 2015 David Bigsgli
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

#include <unistd.h>
#include <pwd.h>

#include <signal.h>
#include "cmd.h"

#define NL_SETN 8

void
usage(char *cmd)
{
    fprintf(stderr, "usage: %s [-h] [-V] [-f] [-m \"host_name ...\"]\n [-u user_name |-u all] [-J job_name] [jobId | \"jobId[index_list]\" ...]\n", cmd);
}

int
main(int argc, char **argv)
{

    int cc, i, badHostIdx;
    char *user = NULL, *jobName = NULL;
    LS_LONG_INT *jobIds;
    int numJobs = 0;
    struct submig mig;

    if (lsb_init(argv[0]) < 0) {
	lsb_perror("lsb_init");
	exit(-1);
    }

    mig.options = 0;
    mig.askedHosts = NULL;
    mig.numAskedHosts = 0;

    while ((cc = getopt(argc, argv, "Vhfm:u:J:")) != EOF) {
        switch (cc) {
	  case 'm':
              if (mig.numAskedHosts) {
                usage(argv[0]);
                return -1;
              }
            if (getAskedHosts_(optarg, &mig.askedHosts, &mig.numAskedHosts,
                               &i, FALSE) < 0 && lserrno != LSE_BAD_HOST) {
		ls_perror(NULL);
                exit(-1);
            }
	    if (mig.numAskedHosts == 0) {
		usage(argv[0]);
                return -1;
            }
	    break;
        case 'u':
            if (user) {
                usage(argv[0]);
                return -1;
            }
            if (strcmp(optarg, "all") == 0)
                user = ALL_USERS;
            else
                user = optarg;
            break;
        case 'J':
            jobName = optarg;
            break;
	  case 'f':
	    mig.options |= LSB_CHKPNT_FORCE;
	    break;
	  case 'V':
	    fputs(_LS_VERSION_, stderr);
	    exit(0);
	  case 'h':
            usage(argv[0]);
            return -1;
	  default:
            usage(argv[0]);
            return -1;
        }
    }

    numJobs = getJobIds (argc, argv, jobName, user, NULL, NULL, &jobIds, 0, NULL);


    for (i = 0; i < numJobs; i++) {
	mig.jobId = jobIds[i];
	if (lsb_mig(&mig, &badHostIdx) < 0) {
	    fprintf(stderr, "%s <%s>: ", I18N_Job, lsb_jobid2str(jobIds[i]));
	    if (lsberrno == LSBE_QUEUE_HOST || lsberrno == LSBE_BAD_HOST)
		lsb_perror(mig.askedHosts[badHostIdx]);
	    else
		lsb_perror(NULL);
	} else {
	    printf("Job <%s> is being migrated\n", lsb_jobid2str(jobIds[i]));
	}
    }

    return 0;
}
