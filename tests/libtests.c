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

#include "../lsf/lsf.h"

static inline void
failed(const char *test)
{
    printf("test %s FAILED\n", test);
}

static inline void
done(const char *test)
{
    printf("test %s DONE\n", test);
}

/*
 * test0()
 *
 * Test:
 *    char *ls_getclustername(void)
 *    char *ls_getmastername(void)
 *
 */
int
test0(int n)
{
    char *cluster;
    char *master;

    printf("I am %s number %d\n", __func__, n);

    cluster = ls_getclustername();
    if (cluster == NULL) {
        ls_perror("ls_getclustername");
        goto hosed;
    }
    printf(" My cluster name is %s\n", cluster);

    master = ls_getmastername();
    if (master == NULL) {
        ls_perror("ls_getmastername");
        goto hosed;
    }
    printf(" My master name is %s\n", cluster);

    done(__func__);
    return 0;

hosed:
    failed(__func__);
    return -1;
}

/*
 * test1()
 *
 * Test ls_info()
 *
 */
int
test1(int n)
{
    struct lsInfo *lsInfo;

    printf("I am %s number %d\n", __func__, n);

    lsInfo = ls_info();
    if (lsInfo == NULL) {
        ls_perror("ls_info");
        failed(__func__);
        return -1;
    }

    printf(" Got %d resources\n", lsInfo->nRes);
    done(__func__);
    return 0;
}

/* test2()
 *
 * Test ls_gethostinfo()
 *
 */
int
test2(int n)
{
    struct hostInfo *hostinfo;
    int numhosts;

    printf("I am %s number %d\n", __func__, n);

    hostinfo = ls_gethostinfo(NULL, &numhosts, NULL, 0, 0);
    if (hostinfo == NULL) {
        ls_perror("ls_info");
        failed(__func__);
        return -1;
    }

    printf(" Got %d hosts\n", numhosts);
    done(__func__);
    return 0;
}

/* test3()
 *
 * Test ls_gethostinfo() with impossible resreq
 * the call should fail so the test should be DONE
 *
 */
int
test3(int n)
{
    struct hostInfo *hostinfo;
    int numhosts;
    char *resreq;

    printf("I am %s number %d\n", __func__, n);

    resreq = "tmp < 0";
    hostinfo = ls_gethostinfo(resreq, &numhosts, NULL, 0, 0);
    if (hostinfo == NULL) {
        ls_perror(" ls_info");
        done(__func__);
        return 0;
    }

    failed(__func__);
    return -1;

}
