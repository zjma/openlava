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
    fprintf(stderr, "btokens: [-m master] [-p port] [ -h ] [ -V ]\n");
}

static void
print_tokens(int, struct glb_token *, const char *master, const char *port);

int
main(int argc, char **argv)
{
    int num;
    int cc;
    char *port;
    char *master;
    struct glb_token *t;

    if (lsb_init(argv[0]) < 0) {
        lsb_perror("lsb_init");
        return -1;
    }

    master = port = NULL;
    while ((cc = getopt(argc, argv, "hVm:p:")) != EOF) {
        switch (cc) {
            case 'm':
                master = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'V':
                fputs(_LS_VERSION_, stderr);
                return 0;
            case 'h':
                usage();
                exit(-1);
        }
    }


    t = lsb_gettokens(master, port, &num);
    if (t == NULL) {
        if (lsberrno == LSBE_NO_ERROR) {
            fprintf(stderr, "\
btokens: No tokens in the system.\n");
            return -1;
        }
        fprintf(stderr, "btokens: %s.\n", lsb_sysmsg());
        return -1;
    }

    print_tokens(num, t, master, port);

    free_tokens(num, t);

    return 0;
}

static void
print_tokens(int num,
             struct glb_token *t,
             const char *master,
             const char *port)
{
    int cc;

    if (master && port) {
        printf("master: %-10s port: %s\n", master, port);
    }

    for (cc = 0; cc < num; cc++) {
        printf("\
  name: %-10s ideal: %d allocated: %d recalled: %d\n", t[cc].name, t[cc].ideal,
               t[cc].allocated, t[cc].recalled);
    }
}
