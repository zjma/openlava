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

static void print_messages(LS_LONG_INT, struct lsbMsg *, int);
void
usage(void)
{
    fprintf(stderr, "usage: bread [-V] [-h] jobID\n");
}

int
main(int argc, char **argv)
{
    int cc;
    int num;
    LS_LONG_INT *jobIDs;
    struct lsbMsg *msg;

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
            default:
                usage();
                return -1;
        }
    }

    num = getJobIds(argc,
                    argv,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &jobIDs,
                    0,
                    NULL);

    msg = lsb_readjobmsg(jobIDs[0], &num);
    if (msg == NULL) {
        if (lsberrno == LSBE_NO_ERROR) {
            printf("Job %s has no posted messages\n", lsb_jobid2str(jobIDs[0]));
            return 0;
        }
        lsb_perror("lsb_getmsgjob()");
        return -1;
    }

    print_messages(jobIDs[0], msg, num);

    return 0;
}

static void
print_messages(LS_LONG_INT jobID, struct lsbMsg *msg, int num)
{
    int cc;

    /* JOBID     POST_TIME
     * 1234567890123456789
     * - jack bubu mortacci tua
     */
    printf("Messages posted to jobID %s\n", lsb_jobid2str(jobID));
    for (cc = 0; cc < num; cc++) {
        printf("POST_TIME: %s MESSAGE: %s\n",
               ls_time(msg->t), msg[cc].msg);
    }
}
