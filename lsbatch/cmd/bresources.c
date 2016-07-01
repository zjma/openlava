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
#include "lsb.h"

static void
usage(void)
{
    fprintf(stderr, "bresources: [ -h ] [ -V ]\n");
}

static void
print_limits(struct resLimitReply *);

int
main(int argc, char **argv)
{
    int cc;
    struct resLimitReply *limits;

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


    limits = lsb_getlimits();
    if (limits == NULL) {
        if (lsberrno == LSBE_NO_ERROR) {
            fprintf(stderr, "\
bresources: No resource limit yet in the system.\n");
            return -1;
        }
        fprintf(stderr, "bresources: %s.\n", lsb_sysmsg());
        return -1;
    }

    print_limits(limits);

    free_resLimits(limits);

    return 0;
}

/* print_groups
 */
static void
print_limits(struct resLimitReply *reply)
{
    int i, j, k;
    char *name;
    int  inUse;

    for (i = 0; i < reply->numLimits; i++) {
        printf("%s  %s\n", "LIMIT", reply->limits[i].name);

        for (j = 0; j < reply->limits[i].nConsumer; j++) {
            switch (reply->limits[i].consumers[j].consumer) {
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
            printf("%-10s : %s\n", name, reply->limits[i].consumers[j].def);
            if (reply->limits[i].consumers[j].consumer == LIMIT_CONSUMER_HOSTS
                    || reply->limits[i].consumers[j].consumer == LIMIT_CONSUMER_USERS
                    || reply->limits[i].consumers[j].consumer == LIMIT_CONSUMER_QUEUES)
                printf("    expand : %s\n", reply->limits[i].consumers[j].value);
        }

        for (j = 0; j < reply->limits[i].nRes; j++) {
             switch (reply->limits[i].res[j].res) {
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
            printf("%-10s : %d\n", name, (int)(reply->limits[i].res[j].value));

            if (reply->numUsage > 0
                    && strcmp(name, "JOBS") == 0) {
                printf("\n%s\n", "CURRENT LIMIT RESOURCE USAGE:");
                printf("    %-10s  %-10s  %-10s\n", "PROJECT", "QUEUE", "JOBS");

                inUse = FALSE;
                for (k = 0; k < reply->numUsage; k++) {
                    if (strcmp(reply->limits[i].name, reply->usage[k].limitName)
                            || reply->usage[k].used <= 0)
                        continue;
                    printf("    %-10s  %-10s  %d/%d\n",
                                reply->usage[k].project,
                                reply->usage[k].queue,
                                (int)reply->usage[k].used,
                                (int)(reply->limits[i].res[j].value));
                    inUse = TRUE;
                }
                if (!inUse)
                    printf("    %-10s  %-10s  %-10s\n", "-", "-", "-");
            }
        }
        printf("\n\n");
    }
}
