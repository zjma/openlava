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

static char buf[BUFSIZ];

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

static LS_LONG_INT submit_job(int, char *);
static int wait_for_job(LS_LONG_INT, int);
static int kill_job(LS_LONG_INT, int);
static int get_job_status(LS_LONG_INT);
static struct queueInfoEnt *get_queues(int *);
static void ssleep(int);
static int wait_for_job_run(LS_LONG_INT, int);
static void strip_bra(char *);

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
    printf("My cluster name is %s\n", cluster);

    master = ls_getmastername();
    if (master == NULL) {
        ls_perror("ls_getmastername");
        goto hosed;
    }
    printf("My master name is %s\n", master);

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

    printf("Got %d resources\n", lsInfo->nRes);
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

    printf("Got %d hosts\n", numhosts);
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
        ls_perror("ls_placereq() 1");
        failed(__func__);
        return -1;
    }

    res_req = "order[r1m]";
    best = ls_placereq(res_req, &num, 0, NULL);
    if (best == NULL) {
        ls_perror("ls_placereq() 2");
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
        ls_perror("ls_initrex");
        failed(__func__);
        return -1;
    }

    best = ls_placereq(NULL, &num, 0, NULL);
    if (best == NULL) {
        ls_perror("ls_placereq()");
        failed(__func__);
        return -1;
    }

    arg[0] = "/bin/echo";
    arg[1] = " output test 6";
    arg[2] = NULL;

    pid = fork();
    if (pid == 0) {
        printf("Child pid %d\n", getpid());
        ls_rexecv(best[0], arg, 0);
        /* should never get here
         */
        ls_perror("ls_rexecv()");
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

    ent = lsb_queueinfo(NULL, &num, NULL, NULL, 0);
    if (ent == NULL) {
        lsb_perror("lsb_queueinfo()");
        failed(__func__);
        return -1;
    }

    for (i = 0; i < num; i++) {
        printf("queue: %s\n", ent[i].queue);
    }

    done(__func__);
    return 0;
}

/* test8()
 *
 * Test ls_hostinfo()
 */
int
test8(int n)
{
    struct hostInfoEnt *h;
    int num;
    int i;

    printf("I am %s number %d\n", __func__, n);

    /* This must be set to 0 otherwise a random
     * number will be intepreted by the library
     * as the number of we are asking about.
     */
    num = 0;
    h = lsb_hostinfo(NULL, &num);
    if (h == NULL) {
        lsb_perror("lsb_hostinfo()");
        failed(__func__);
        return -1;
    }

    for (i = 0; i < num; i++) {
        printf("host: %s\n", h[i].host);
    }

    done(__func__);
    return 0;
}

/* test9()
 *
 * Test submit a job and wait for it
 */
int
test9(int n)
{
    LS_LONG_INT jobID;

    printf("I am %s number %d\n", __func__, n);

    jobID = submit_job(60, NULL);
    if (wait_for_job(jobID, 10) < 0) {
        lsb_perror(" wait_for_job()");
        failed(__func__);
        return -1;
    }

    done(__func__);
    return 0;
}

/* test10()
 *
 * Submit a job and kill it
 */
int
test10(int n)
{
    LS_LONG_INT jobID;
    int status;

    printf("I am %s number %d\n", __func__, n);

    jobID = submit_job(3600, NULL);
    kill_job(jobID, SIGKILL);
    ssleep(10);
    status = get_job_status(jobID);
    if (IS_FINISH(status)) {
        printf("Job %s finished\n", lsb_jobid2str(jobID));
    } else {
        failed(__func__);
        return -1;
    }

    done(__func__);

    return 0;
}

/* test11()
 *
 * Submit a job, suspend it and resume it
 *
 */
int
test11(int n)
{
    LS_LONG_INT jobID;
    int status;

    printf("I am %s number %d\n", __func__, n);

    jobID = submit_job(3600, NULL);
    kill_job(jobID, SIGSTOP);
    ssleep(10);
    status = get_job_status(jobID);
    if (IS_SUSP(status)) {
        printf("Job %s correctly suspended\n", lsb_jobid2str(jobID));
    } else {
        failed(__func__);
        return -1;
    }

    kill_job(jobID, SIGCONT);
    ssleep(10);
    status = get_job_status(jobID);
    if (IS_START(status)) {
        printf("Job %s correctly resumed\n", lsb_jobid2str(jobID));
    } else {
        failed(__func__);
        return -1;
    }

    kill_job(jobID, SIGKILL);
    ssleep(10);
    status = get_job_status(jobID);
    if (IS_FINISH(status)) {
        printf("Job %s correctly finished\n", lsb_jobid2str(jobID));
    } else {
        failed(__func__);
        return -1;
    }

    done(__func__);
    return 0;
}

/* test12()
 *
 * Switch a job between queues
 *
 */
int
test12(int n)
{
    struct queueInfoEnt *ent;
    int num;
    int cc;
    LS_LONG_INT jobID;

    printf("I am %s number %d\n", __func__, n);
    /* need at least 2 queues
     */
    ent = get_queues(&num);
    if (ent == NULL) {
        failed(__func__);
        return -1;
    }

    if (num < 2) {
        printf("Need at least 2 queues\n");
        done(__func__);
        return 0;
    }

    /* submit a job to one queue
     */
    jobID = submit_job(300, ent[0].queue);
    if (jobID < 0) {
        failed(__func__);
        return -1;
    }

    /* wait for the job to start
     */
    cc = wait_for_job_run(jobID, 60);
    if (cc < 0) {
        failed(__func__);
        return -1;
    }

    /* switch it
     */
    if (lsb_switchjob(jobID, ent[1].queue) < 0) {
        lsb_perror("lsb_switchjob");
        failed(__func__);
        return -1;
    }

    done(__func__);
    return 0;
}

/* Test13
 *
 * Submit a job in hold modify it and release it
 *
 */
int
test13(int n)
{
    FILE *fp;
    char jobid[24];
    static char lbuf[128];

    printf("I am %s number %d\n", __func__, n);

    sprintf(buf, "bsub -H -o /dev/null -R \"order[mem:r1m]\" sleep 120");

    fp = popen(buf, "r");
    if (fp == NULL) {
        lsb_perror("bsub");
        failed(__func__);
        return -1;
    }

    if (fscanf(fp, "%*s%s", jobid) != 1) {
        perror("fscanf()");
        failed(__func__);
        return -1;
    }

    strip_bra(jobid);
    pclose(fp);

    sprintf(buf, "bmod -R \"order[mem]\" %s", jobid);

    fp = popen(buf, "r");
    if (fp == NULL) {
        lsb_perror("bsub");
        failed(__func__);
        return -1;
    }

    fgets(lbuf, sizeof(lbuf), fp);
    if (! strstr(lbuf, "Parameters of job")) {
        pclose(fp);
        failed(__func__);
        return -1;
    }
    pclose(fp);

    sprintf(buf, "bkill %s", jobid);
    system(buf);

    done(__func__);
    return 0;
}

/* test14
 *
 * Test mbd up after reconfig
 *
 */
int
test14(int n)
{
    FILE *fp;
    char lbuf[256];

    printf("I am %s number %d\n", __func__, n);

    sprintf(buf, "badmin reconfig");
    system(buf);

    ssleep(15);

    sprintf(buf, "bparams");
    fp = popen(buf, "r");
    if (fp == NULL) {
        perror("popen()");
        pclose(fp);
        failed(__func__);
        return -1;
    }

    while (fgets(lbuf, sizeof(lbuf), fp)) {
        if (strstr(lbuf, "Job Dispatch Interval:")) {
            pclose(fp);
            done(__func__);
            return 0;
        }
    }

    pclose(fp);
    failed(__func__);
    return -1;
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

static LS_LONG_INT
submit_job(int runTime, char *queue)
{
    int i;
    int jobId;
    struct submit req;
    struct submitReply reply;
    char cmd[128];

    memset(&req, 0, sizeof(struct submit));

    if (queue)
        req.queue = queue;
    req.options = 0;
    req.maxNumProcessors = 1;
    req.options2 = 0;
    req.resReq = NULL;

    for (i = 0; i < LSF_RLIM_NLIMITS; i++)
        req.rLimits[i] = DEFAULT_RLIMIT;

    if (runTime <= 0)
        runTime = rand()%60;

    req.hostSpec = NULL;
    req.numProcessors = 1;
    req.maxNumProcessors = 1;
    req.beginTime = 0;
    req.termTime  = 0;
    sprintf(cmd, "sleep %d", runTime);
    req.command = cmd;
    req.nxf = 0;
    req.delOptions = 0;
    req.options |= SUB_OUT_FILE;
    req.outFile = "/dev/null";

    jobId = lsb_submit(&req, &reply);
    if (jobId < 0) {
        switch (lsberrno) {
        case LSBE_QUEUE_USE:
        case LSBE_QUEUE_CLOSED:
            lsb_perror("lsb_submit");
            failed(__func__);
            return -1;
        default:
            lsb_perror("lsb_submit");
            failed(__func__);
            return -1;
        }
    }

    return jobId;
}

static int
wait_for_job(LS_LONG_INT jobID, int tm)
{
    struct jobInfoEnt  *job;
    int more;
    int num;
    int cc;

    num = 0;
    while (1) {

        cc = lsb_openjobinfo(jobID, NULL, "all", NULL, NULL, CUR_JOB);
        if (cc < 0 && num > 0)
            break;
        if (cc < 0 && num == 0) {
            lsb_perror("lsb_openjobinfo");
            failed(__func__);
            return -1;
        }

        job = lsb_readjobinfo(&more);
        if (job == NULL) {
            lsb_perror("lsb_readjobinfo");
            failed(__func__);
            return -1;
        }

        /* display the job
         */
        printf("\
Job <%s> of user <%s>, submitted from host <%s> still running...\n",
               lsb_jobid2str(job->jobId),
               job->user,
               job->fromHost);

        sleep(tm);
        ++num;
        lsb_closejobinfo();
    }

    printf("Job <%s> done...\n", lsb_jobid2str(job->jobId));

    return 0;
}

static int
kill_job(LS_LONG_INT jobID, int s)
{

    if (lsb_signaljob(jobID, s) <0) {
        lsb_perror(" lsb_signaljob");
        failed(__func__);
        return -1;
    }
    printf("%s: signal %d delivered all right\n", __func__, s);

    return 0;
}

static int
get_job_status(LS_LONG_INT jobID)
{
    struct jobInfoEnt *job;
    int more;
    int cc;

    cc = lsb_openjobinfo(jobID, NULL, "all", NULL, NULL, ALL_JOB);
    if (cc < 0) {
        lsb_perror(" lsb_openjobinfo");
        failed(__func__);
        return -1;
    }

    job = lsb_readjobinfo(&more);
    if (job == NULL) {
        lsb_perror(" lsb_readjobinfo");
        failed(__func__);
        return -1;
    }

    lsb_closejobinfo();

    return job->status;
}

static void
ssleep(int s)
{
    time_t start;

    start = time(NULL);
    printf("wait");
    while (1) {
        sleep(1);
        printf(".");
        if (time(NULL) - start >= s)
            break;
    }

    printf("\n");
}

static struct queueInfoEnt *
get_queues(int *num)
{
    struct queueInfoEnt *ent;

    ent = lsb_queueinfo(NULL, num, NULL, NULL, 0);
    if (ent == NULL) {
        lsb_perror("lsb_queueinfo()");
        failed(__func__);
        return NULL;
    }

    return ent;
}

static int
wait_for_job_run(LS_LONG_INT jobID, int tm)
{
    int status;
    time_t start;

    start = time(NULL);
    printf("wait");
znovu:
    status = get_job_status(jobID);
    if (IS_START(status))
        return 0;
    if (time(NULL) - start > tm)
        return -1;
    printf(".");
    sleep(2);
    goto znovu;
}

static void
strip_bra(char *p)
{
    int i;

    for (i = 0; p[i]; i++) {
        if (p[i] == '>'
            || p[i] == '<')
            p[i] = ' ';
    }
}
