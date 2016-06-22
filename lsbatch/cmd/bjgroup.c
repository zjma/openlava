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
#include "cmd.h"

static void
usage(void)
{
    fprintf(stderr, "bjgroups: [ -h ] [ -V ]\n");
}

static void
print_groups(int, struct jobGroupInfo *);

int
main(int argc, char **argv)
{
    int num;
    int cc;
    struct jobGroupInfo *jgrp;

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


    jgrp = lsb_getjgrp(&num);
    if (jgrp == NULL) {
        if (lsberrno == LSBE_NO_ERROR) {
            fprintf(stderr, "\
bjgroups: No job groups yet in the system.\n");
            return -1;
        }
        fprintf(stderr, "bjgroups: %s.\n", lsb_sysmsg());
        return -1;
    }

    print_groups(num, jgrp);

    free_jobgroupinfo(num, jgrp);

    return 0;
}

/* print_groups
 */
static void
print_groups(int num, struct jobGroupInfo *jgrp)
{
    int cc;
    int i;
    int len;
    int maxlen;

    maxlen = -1;

    for (cc = 0; cc < num; cc++) {
        len = strlen(jgrp[cc].path);
        if (len > maxlen)
            maxlen = len;
    }

    printf("GROUP");

    for (cc = 0; cc < maxlen; cc++)
        printf(" ");
    /* NJOBS   PEND   RUN   SUSP   EXITED   DONE JLIMIT
     */
    printf("%-5s   %-5s   %-5s   %-5s   %-5s  %-5s   %-6s\n",
           "NJOBS", "PEND", "RUN", "SUSP", "EXITED", "DONE", "JLIMIT");

    for (cc = 0; cc < num; cc++) {

        len = maxlen - strlen(jgrp[cc].path);
        printf("%s ", jgrp[cc].path);
        /* print the path and strlen(PATH) which
         * is 4
         */
        for (i = 0; i < (len + 4); i++)
            printf(" ");

        printf("%-5d   ", jgrp[cc].counts[0]);
        printf("%-5d   ", jgrp[cc].counts[1] + jgrp[cc].counts[2]);
        printf("%-5d   ", jgrp[cc].counts[3]);
        printf("%-5d   ", jgrp[cc].counts[4] + jgrp[cc].counts[5]);
        printf("%-5d   ", jgrp[cc].counts[6]);
        printf("%-5d   ", jgrp[cc].counts[7]);
        if (jgrp[cc].max_jobs == INT32_MAX)
            printf("%-7s   ", "-");
        else
            printf("%-7d   ", jgrp[cc].max_jobs);

        printf("\n");
    }
}
