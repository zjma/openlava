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

/* Array of testing functions.
 */
int (*funtest[])(int) =
{
    test0, test1, test2, test3, test4, test5, test6,
    test7, test8, test9, test10, test11, test12,
    test13, test14, test15, test16, test17, test18,
    test19, test20, NULL
};

int
main(int argc, char **argv)
{
    int num;
    int num2;
    int n;
    int cc;
    int ok;
    int not_ok;
    struct timeval tv;
    struct timeval tv2;

    gettimeofday(&tv, NULL);
    setbuf(stdout, NULL);

    if (lsb_init(NULL) < 0) {
        lsb_perror("lsb_init()");
        return -1;
    }

    ok = not_ok = 0;
    num = -1;
    if (argv[1]) {
        num = atoi(argv[1]);
    }

    num2 = sizeof(funtest)/sizeof(funtest[0]);
    num2 = num2 - 2; /* account for NULL and 0 */
    if (num > num2) {
        printf("We have only %d tests\n", num2);
        return -1;
    }

    printf("Start pid: %d\n", getpid());

    /* num is set run the specific test only
     */
    if (num > -1) {
        (*funtest[num])(num);
        goto via;
    }

    /* num is not set run all tests
     */
    for (n = 0; funtest[n]; n++) {
        cc = (*funtest[n])(n);
        if (cc == 0)
            ++ok;
        else
            ++not_ok;
    }
    printf("Done. %d tests ok, %d tests not ok\n", ok, not_ok);
via:

    gettimeofday(&tv2, NULL);
    printf("Testing times: %4.2fms\n",
           ((tv2.tv_sec - tv.tv_sec) * 1000.0
            + (tv2.tv_usec - tv.tv_usec) / 1000.0));

    return 0;
}
