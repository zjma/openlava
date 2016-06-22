/*
 * Copyright (C) 2016 Teraproc
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
    fprintf(stderr, "bresources: [ -h ] [ -V ]\n");
}

static void
print_limits(int, struct resLimit *);

int
main(int argc, char **argv)
{
    int num;
    int cc;
    struct resLimit *limits;

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


    limits = lsb_getlimits(&num);
    if (limits == NULL) {
        if (lsberrno == LSBE_NO_ERROR) {
            fprintf(stderr, "\
bresources: No resource limit yet in the system.\n");
            return -1;
        }
        fprintf(stderr, "bresources: %s.\n", lsb_sysmsg());
        return -1;
    }

    print_limits(num, limits);

    free_resLimits(num, limits);

    return 0;
}

/* print_groups
 */
static void
print_limits(int num, struct resLimit *limits)
{
    int i, j;
    char *name;

    for (i = 0; i < num; i++) {
        printf("%s  %s\n", "LIMIT", limits[i].name);

        for (j = 0; j < limits[i].nConsumer; j++) {
            switch (limits[i].consumers[j].consumer) {
                case LIMIT_CONSUMER_QUEUES:
                    name = "QUEUES";
                    break;
                case LIMIT_CONSUMER_PROJECTS:
                    name = "PROJECTS";
                    break;
                case LIMIT_CONSUMER_HOSTS:
                    name = "HOSTS";
                    break;
                case LIMIT_CONSUMER_USERS:
                    name = "USERS";
                    break;
                default:
                    usage();
                    exit(-1);
            }
            printf("%-10s : %s\n", name, limits[i].consumers[j].def);
            if (limits[i].consumers[j].consumer == LIMIT_CONSUMER_HOSTS
                    || limits[i].consumers[j].consumer == LIMIT_CONSUMER_USERS
                    || limits[i].consumers[j].consumer == LIMIT_CONSUMER_QUEUES)
                printf("    expand : %s\n", limits[i].consumers[j].value);
        }
        for (j = 0; j < limits[i].nRes; j++) {
             switch (limits[i].res[j].res) {
                case LIMIT_RESOURCE_SLOTS:
                    name = "SLOTS";
                    break;
                case LIMIT_RESOURCE_JOBS:
                    name = "JOBS";
                    break;
                default:
                    usage();
                    exit(-1);
            }
            printf("%-10s : %d\n", name, (int)(limits[i].res[j].value));
        }
        printf("\n");
    }
}

