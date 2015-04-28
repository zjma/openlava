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
        done(__func__);
        return 0;
    }

    failed(__func__);
    return -1;

}

/* test4()
 *
 * Test ls_load()
 *
 */
int
test4(int n)
{
    struct hostLoad *hosts;
    int num;

    printf("I am %s number %d\n", __func__, n);

    hosts = ls_load(NULL, &num, 0, NULL);
    if (hosts == NULL) {
        ls_perror(" ls_load");
        failed(__func__);
        return -1;
    }

    printf(" Got load %d hosts\n", num);
    done(__func__);
    return 0;

}

/* test5()
 *
 * Test ls_place()
 */
int
test5(int n)
{
    char **best;
    int num;
    char *res_req;

    printf("I am %s number %d\n", __func__, n);

    best = ls_placereq(NULL, &num, 0, NULL);
    if (best == NULL) {
        ls_perror(" ls_placereq() 1");
        failed(__func__);
        return -1;
    }

    res_req = "order[r1m]";
    best = ls_placereq(res_req, &num, 0, NULL);
    if (best == NULL) {
        ls_perror(" ls_placereq() 2");
        failed(__func__);
        return -1;
    }

    res_req = "select[tmp < 0]";
    best = ls_placereq(res_req, &num, 0, NULL);
    if (best == NULL) {
        done(__func__);
        return 0;
    }

    failed(__func__);
    return -1;
}

/* test6()
 *
 * Test basic remote execution()
 *
 */
int
test6(int n)
{
    char **best;
    char *arg[3];
    int num;
    pid_t pid;

    printf("I am %s number %d\n", __func__, n);

    if (ls_initrex(1, 0) < 0) {
        ls_perror(" ls_initrex");
        failed(__func__);
        return -1;
    }

    best = ls_placereq(NULL, &num, 0, NULL);
    if (best == NULL) {
        ls_perror(" ls_placereq()");
        failed(__func__);
        return -1;
    }

    arg[0] = "/bin/echo";
    arg[1] = " output test 6";
    arg[2] = NULL;

    pid = fork();
    if (pid == 0) {
        printf(" Child pid %d\n", getpid());
        ls_rexecv(best[0], arg, 0);
        /* should never get here
         */
        ls_perror(" ls_rexecv()");
        exit(-1);
    }

    waitpid(pid, NULL, 0);
    done(__func__);

    return 0;
}
