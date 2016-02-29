/*
 * Copyright (C) 2016 David Bigagli
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

#include "lsbatch.h"
#include "cmd.h"

static void
usage(void)
{
    fprintf(stderr, "bjdep: [ -h ] [ -V ] jobId\n");
}

static void
print_jobdep(int, struct job_dep *, LS_LONG_INT);

int
main(int argc, char **argv)
{
    int cc;
    LS_LONG_INT jobid;
    struct job_dep *jobdep;

    if (lsb_init(argv[0]) < 0) {
	lsb_perror("lsb_init");
	return -1;
    }

    while ((cc = getopt(argc, argv, "hV")) != EOF) {
	switch (cc) {
	    case 'V':
		fputs(_LS_VERSION_, stderr);
		return 0;
	    case 'h':
		usage();
		exit(-1);
	}
    }

    if (argc <= optind) {
	usage();
	return -1;
    }

    if (getOneJobId(argv[argc - 1], &jobid, 0)) {
	usage();
	return -1;
    }

    jobdep = lsb_jobdep(jobid, &cc);
    if (jobdep == NULL) {
	if (lsberrno == LSBE_NO_ERROR) {
	    fprintf(stderr, "\
bjdep: No job dependencies for job %s.\n", lsb_jobid2str(jobid));
	    return -1;
	}
	fprintf(stderr, "bjdep: %s.\n", lsb_sysmsg());
	return -1;
    }
    print_jobdep(cc, jobdep, jobid);

    free_jobdep(cc, jobdep);

    return 0;
}

/* print_jobdep()
 */
static void
print_jobdep(int cc, struct job_dep *jobdep, LS_LONG_INT jobid)
{
    int i;

    printf("Dependencies of job %s\n", lsb_jobid2str(jobid));

    printf("%-10s  %-5s  %-10s   %-17s\n",
	   "DEPENDENCY", "JOBID", "JOB_STATUS", "DEPENDENCY_STATUS");

    for (i = 0; i < cc; i++) {
	printf("\
%-10s  %-5s   ", jobdep[i].dependency, jobdep[i].jobid);
	if (IS_PEND(jobdep[i].jstatus))
	    printf("%-10s   ", "PEND");
	else if (IS_START(jobdep[i].jstatus))
	    printf("%-10s   ", "RUN");
	else if (jobdep[i].jstatus & JOB_STAT_DONE)
	    printf("%-10s   ", "DONE");
	else if (jobdep[i].jstatus & JOB_STAT_EXIT)
	    printf("%-10s   ", "EXIT");
	else
	    printf("%-10s   ", "BOH?");
	if (jobdep[i].depstatus == true)
	    printf("%-s", "true");
	else
	    printf("%-s", "false");
	printf("\n");
    }
}



