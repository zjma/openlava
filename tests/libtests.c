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
        failed(__func__);
        exit(-1);
    }

    waitpid(pid, NULL, 0);
    done(__func__);

    return 0;
}

/* test7()
 *
 * Test lsb_queueinfo()
 *
 */
int
test7(int n)
{
    struct queueInfoEnt *ent;
    int num;
    int i;

    printf("I am %s number %d\n", __func__, n);

    if (lsb_init("test7") < 0) {
        lsb_perror(" lsb_init()");
        failed(__func__);
        return -1;
    }

    ent = lsb_queueinfo(NULL, &num, NULL, NULL, 0);
    if (ent == NULL) {
        lsb_perror(" lsb_queueinfo()");
        failed(__func__);
        return -1;
    }

    for (i = 0; i < num; i++) {
        printf(" queue: %s\n", ent[i].queue);
    }

    done(__func__);
    return 0;
}

int
test8(int n)
{
    struct hostInfoEnt *h;
    int num;
    int i;

    printf("I am %s number %d\n", __func__, n);

    if (lsb_init("test8") < 0) {
        lsb_perror(" lsb_init()");
        failed(__func__);
        return -1;
    }

    /* This must be set to 0 otherwise a random
     * number will be intepreted by the library
     * as the number of we are asking about.
     */
    num = 0;
    h = lsb_hostinfo(NULL, &num);
    if (h == NULL) {
        lsb_perror(" lsb_hostinfo()");
        failed(__func__);
        return -1;
    }

    for (i = 0; i < num; i++) {
        printf(" host: %s\n", h[i].host);
    }

    done(__func__);
    return 0;
}
int
test9(int n)
{
    int i;
    int jobId;
    struct submit req;
    struct submitReply reply;

    printf("I am %s number %d\n", __func__, n);

    if (lsb_init("test9") < 0) {
        lsb_perror(" lsb_init");
        failed(__func__);
        return -1;
    }

    memset(&req, 0, sizeof(struct submit));
    req.options = 0;
    req.maxNumProcessors = 1;
    req.options2 = 0;
    req.resReq = NULL;

    for (i = 0; i < LSF_RLIM_NLIMITS; i++)
        req.rLimits[i] = DEFAULT_RLIMIT;

    req.hostSpec = NULL;
    req.numProcessors = 1;
    req.maxNumProcessors = 1;
    req.beginTime = 0;
    req.termTime  = 0;
    req.command = "sleep 60";
    req.nxf = 0;
    req.delOptions = 0;
    req.outFile = "/dev/null";

    jobId = lsb_submit(&req, &reply);
    if (jobId < 0) {
        switch (lsberrno) {
        case LSBE_QUEUE_USE:
        case LSBE_QUEUE_CLOSED:
            lsb_perror(" lsb_submit");
            failed(__func__);
            return -1;
        default:
            lsb_perror(" lsb_submit");
            failed(__func__);
            return -1;
        }
    }

    done(__func__);
    return 0;
}
int
test10(int n)
{
    struct jobInfoEnt *job;
    int more;

    if (lsb_init("test10") < 0) {
        lsb_perror(" lsb_init");
        failed(__func__);
        return -1;
    }

    if (lsb_openjobinfo(0, NULL, "all", NULL, NULL, ALL_JOB) < 0) {
        lsb_perror(" lsb_openjobinfo");
        failed(__func__);
        return -1;
    }

    printf(" All jobs submitted by all users:\n");
    for (;;) {
        job = lsb_readjobinfo(&more);
        if (job == NULL) {
            lsb_perror(" lsb_readjobinfo");
            failed(__func__);
            return -1;
        }

        /* display the job
         */
        printf(" Job <%s> of user <%s>, submitted from host <%s>\n",
               lsb_jobid2str(job->jobId),
               job->user,
               job->fromHost);

        if (! more)
            break;
    }

    lsb_closejobinfo();
    done(__func__);
    return 0;
}
int
test11(int n)
{
    return 0;
}
int
test12(int n)
{
    return 0;
}
int
test13(int n)
{
    return 0;
}
int
test14(int n)
{
    return 0;
}
int
test15(int n)
{
    return 0;
}
int
test16(int n)
{
    return 0;
}
int
test17(int n)
{
    return 0;
}
int
test18(int n)
{
    return 0;
}
int
test19(int n)
{
    return 0;
}
int
test20(int n)
{
    return 0;
}
