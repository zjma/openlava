/*
 * Copyright (C) 2016 David Bigagli
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

static void
usage(void)
{
    fprintf(stderr, "\
bgmod: [ -h ] [ -V ] [-L job limit] [-Ln] group_name\n");
}

/* in lsb.misc.c
 */
extern char isint_(char *);

int
main(int argc, char **argv)
{
    int cc;
    struct job_group jg;
    char *max_jobs;

    if (lsb_init(argv[0]) < 0) {
        lsb_perror("lsb_init");
        return -1;
    }

    max_jobs = NULL;
    while ((cc = getopt(argc, argv, "hVL:")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                return 0;
            case 'L':
                max_jobs = optarg;
                break;
            case 'h':
                usage();
                exit(-1);
        }
    }

    if (argc <= optind) {
        usage();
        return -1;
    }

    if (max_jobs
        && max_jobs[0] != 'n'
        && (! isint_(max_jobs)
            || atoi(max_jobs)) < 0) {
        fprintf(stderr, "\
%s is not a valid integer in the interval [0-%d]\n", max_jobs, INT32_MAX);
        return -1;
    }

    jg.group_name = argv[argc - 1];
    jg.max_jobs = INT32_MAX;
    if (max_jobs
        && max_jobs[0] != 'n')
        jg.max_jobs = atoi(max_jobs);

    cc = lsb_modjgrp(&jg);
    if (cc != LSBE_NO_ERROR) {
        fprintf(stderr, "bgmod: %s.\n", lsb_sysmsg());
        return -1;
    }

    printf("Group %s modified successfully.\n", jg.group_name);

    return 0;
}
