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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "tests.h"

/* Array of testig functions.
 */
int (*funtest[])(int) =
{
    test0, test1, test2, test3, NULL
};

int
main(int argc, char **argv)
{
    int num;
    int n;
    int cc;
    int ok;
    int not_ok;

    ok = not_ok = 0;
    num = -1;
    if (argv[1]) {
        num = atoi(argv[1]);
    }

    if (num > -1) {
        (*funtest[num])(num);
        goto via;
    }

    for (n = 0; funtest[n]; n++) {
        cc = (*funtest[n])(n);
        if (cc == 0)
            ++ok;
        else
            ++not_ok;
    }
    printf("Done. %d tests ok, %d tests not ok\n", ok, not_ok);
via:
    return 0;
}
