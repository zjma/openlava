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
    fprintf(stderr, "btokens: [ -h ] [ -V ]\n");
}

static void
print_tokens(int, struct glb_token *);

int
main(int argc, char **argv)
{
    int num;
    int cc;
    struct glb_token *t;

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


    t = lsb_gettokens(&num);
    if (t == NULL) {
        if (lsberrno == LSBE_NO_ERROR) {
            fprintf(stderr, "\
btokens: No tokens in the system.\n");
            return -1;
        }
        fprintf(stderr, "btokens: %s.\n", lsb_sysmsg());
        return -1;
    }

    print_tokens(num, t);

    free_tokens(num, t);

    return 0;
}

static void
print_tokens(int num, struct glb_token *t)
{
    int cc;

    for (cc = 0; cc < num; cc++) {
        printf("\
name: %-10s ideal: %d allocated: %d recalled: %d\n", t[cc].name, t[cc].ideal,
               t[cc].allocated, t[cc].recalled);
    }
}
