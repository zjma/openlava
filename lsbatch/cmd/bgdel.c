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
    fprintf(stderr, "bgdel: [ -h ] [ -V ] group_name\n");
}

int
main(int argc, char **argv)
{
    int cc;
    struct job_group jg;

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

    jg.group_name = argv[argc - 1];

    cc = lsb_deljgrp(&jg);
    if (cc != LSBE_NO_ERROR) {
        fprintf(stderr, "bgdel: %s.\n", lsb_sysmsg());
        return -1;
    }

    printf("Group %s removed successfully.\n", jg.group_name);

    return 0;
}
