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
    fprintf(stderr, "bgroups: [ -h ] [ -V ]\n");
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
bgroups: No job groups yet in the system.\n");
	    return -1;
	}
	fprintf(stderr, "bgroups: %s.\n", lsb_sysmsg());
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

    for (cc = 0; cc < num; cc++) {
	printf("%s ", jgrp[cc].path);
	for (i = 0; i < NUM_JGRP_COUNTERS; i++)
	    printf("%d ", jgrp[cc].counts[i]);
	printf("\n");
    }
}



