/*
 * Copyright (C) 2011-2016 David Bigagli
 * Copyright (C) 2007 Platform Computing Inc
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

#include <unistd.h>
#include <pwd.h>
#include "cmd.h"
#include "../lib/lsb.h"
#include <netdb.h>
#include <errno.h>

#define ALL_PROJ_STR "all"

extern int sig_decode(int);
extern char *get_status(struct jobInfoEnt *job);

static void do_options(int, char **, int *, char **, char **,
                       char **, char **, float *, int *, char **, char **);
static int  skip_job(struct jobInfoEnt *);
static void displayJobs(struct jobInfoEnt *, struct jobInfoHead *,
                        int, int);

static LS_LONG_INT *usrJids;
static int *numJobs;

static int numJids;
static int foundJids;

#define MAX_TIMERSTRLEN         20
#define MAX_TIMESTRLEN          20
int uflag = false;
int Wflag = false;
int noheaderflag = false;

#define FORMAT_WL 1
#define FORMAT_WP 2
#define FORMAT_WF 3
int fintimeformat = 0;

static int isLSFAdmin(void);
static char *Timer2String(float timer);
static char *Time2String(int timer);

void
usage (char *cmd)
{

    fprintf(stderr, "\
Usage: %s [-h] [-V] [-w |-l] [-W] [-a] [-d] [-p] [-s] [-r] [-g job_group]\n", cmd);
    fprintf(stderr, "\
             [-A] [-m host_name] [-q queue_name] [-u user_name | -u all]\n");
    fprintf(stderr,"\
             [-P project_name] [-N host_spec] [-J name_spec] [-UF] [-WL|-WP|-WF] [-noheader]\n");
    fprintf(stderr, "\
             [jobId | \"jobId[idxList]\" ...]\n");

    exit(-1);
}

int
main(int argc, char **argv)
{
    char *jobName;
    int  options;
    char *user;
    char *queue;
    char *host;
    char *projectName;
    char *job_group;
    int  format = 0;
    struct jobInfoHead *jInfoH;
    struct jobInfoEnt *job;
    int  i;
    LS_LONG_INT jobId;
    int  jobDisplayed = 0;
    float cpuFactor = -1;
    char prline[MAXLINELEN];
    char defaultJobName[8] = "/";
    static char lsfUserName[MAXLINELEN];
    int cc;

    projectName = NULL;
    job_group = NULL;
    options = 0;
    if (lsb_init(argv[0]) < 0) {
        lsb_perror("lsb_init");
        exit(-1);
    }

    do_options(argc,
               argv,
               &options,
               &user,
               &queue,
               &host,
               &jobName,
               &cpuFactor,
               &format,
               &projectName,
               &job_group);

    if ((format == LONG_FORMAT || format == LSFUF_FORMAT) && (options & PEND_JOB))
        options |= HOST_NAME;

    if ((options & JGRP_ARRAY_INFO) && numJids <= 0 ) {
        if (jobName == NULL)
            jobName = defaultJobName;
    }

    /* Create a hash table to populate the
     * requested jobIDs with it.
     */
    if (numJids > 0) {
        numJobs = calloc(numJids, sizeof(int));
        memset(numJobs, 0, numJids * sizeof(int));
    }

    if (numJids == 1)
        jobId = usrJids[0];
    else
        jobId = 0;

    if (format != LONG_FORMAT && format != LSFUF_FORMAT && !(options & (HOST_NAME | PEND_JOB)))
        options |= NO_PEND_REASONS;

    TIMEIT(0, (cc = getUser(lsfUserName, MAXLINELEN)), "getUser");
    if (cc != 0 ) {
        exit(-1);
    }

    TIMEIT(0, (jInfoH = lsb_openjobinfo_a(jobId,
                                          jobName,
                                          user,
                                          queue,
                                          host,
                                          options)), "lsb_openjobinfo_a");
    if (jInfoH == NULL) {

        if (numJids >= 1) {
            for (i = 0; i < numJids; i++)
                jobInfoErr(usrJids[i], jobName, user, queue, host, options);
        } else {
            jobInfoErr(LSB_ARRAY_JOBID(jobId),
                       jobName,
                       user,
                       queue,
                       host,
                       options);
        }

        exit(-1);
    }

    options &= ~NO_PEND_REASONS;
    jobDisplayed = 0;

    for (i = 0; i < jInfoH->numJobs; i++) {

        TIMEIT(0, (job = lsb_readjobinfo(NULL)), "lsb_readjobinfo");
        if (job == NULL) {
            lsb_perror("lsb_readjobinfo");
            exit(-1);
        }

        if (numJids == 0 && projectName) {
            if (strcmp(job->submit.projectName, projectName) != 0)
                continue;
        }

        if (numJids == 0 && job_group) {
            if (strcmp(job->submit.job_group, job_group) != 0)
                continue;
        }

        if (numJids > 0)
            if (skip_job(job))
                continue;

        if (format ==  LONG_FORMAT || format == LSFUF_FORMAT) {
            if (i > 0) {
                sprintf(prline, "------------------------------------------------------------------------------\n");
                prtLine(prline);
            }
            if (options & PEND_JOB) {
                if (format == LSFUF_FORMAT)
                    displayUF(job, jInfoH, cpuFactor);
                else
                    displayLong(job, jInfoH, cpuFactor);
            }
            else {
                if (format == LSFUF_FORMAT)
                    displayUF(job, NULL, cpuFactor);
                else
                    displayLong(job, NULL, cpuFactor);
            }
        }
        else
            displayJobs(job, jInfoH, options, format);

        jobDisplayed ++;
    }

    if (format == LONG_FORMAT) {
        sprintf(prline, "\n");
        prtLine(prline);
    }

    if (format == LSFUF_FORMAT)
        printf("\n");

    TIMEIT(0, lsb_closejobinfo(), "lsb_closejobinfo");

    if (numJids > 1 ) {
        int errCount = false;
        lsberrno = LSBE_NO_JOB;
        for (i = 0; i < numJids; i++) {
            if (numJobs[i] <= 0) {
                errCount = true;
                jobInfoErr(usrJids[i], jobName, user, queue, host, options);
            }
        }
        if (errCount == true)
            exit(-1);
    } else {

        if (jobDisplayed == 0) {
            if (projectName) {
                fprintf (stderr, "No job found in project %s\n", projectName);
            }
            if (job_group) {
                fprintf (stderr, "No job found in job group %s\n", job_group);
            }
        }
    }

    if (!jobDisplayed) {
        exit(-1);
    }

    return 0;
}

static void
do_options(int argc,
           char **argv,
           int *options,
           char **user,
           char **queue,
           char **host,
           char **jobName,
           float *cpuFactor,
           int *format,
           char **projectName,
           char **job_group)
{
    int cc, Nflag = 0;
    char *norOp = NULL;

    *options = 0;
    *user = NULL;
    *queue = NULL;
    *host  = NULL;
    *jobName = NULL;
    *format = 0;

    while ((cc = getopt(argc, argv, "VladpsrwW::RAhJ:q:u:m:n:N:P:SU:g:")) != EOF) {
        switch (cc) {
            case 'w':
                if (*format == LONG_FORMAT || *format == LSFUF_FORMAT ||
                    fintimeformat != 0)
                    usage(argv[0]);
                *format = WIDE_FORMAT;
                break;
            case 'l':
                if (*format == WIDE_FORMAT || fintimeformat != 0)
                    usage(argv[0]);
		if (*format != LSFUF_FORMAT)
                    *format = LONG_FORMAT;
                break;
            case 'a':
                *options |= ALL_JOB;
                break;
            case 'd':
                *options |= DONE_JOB;
                break;
            case 'p':
                *options |= PEND_JOB;
                break;
            case 's':
                *options |= SUSP_JOB;
                break;
            case 'r':
                *options |= RUN_JOB;
                break;
            case 'A':
                *options |= JGRP_ARRAY_INFO;
                break;
            case 'J':
                if ((*jobName) || (*optarg == '\0'))
                    usage(argv[0]);
                *jobName = optarg;
                break;
            case 'q':
                if ((*queue) || (*optarg == '\0'))
                    usage(argv[0]);
                *queue = optarg;
                break;
            case 'u':
                if ((*user) || (*optarg == '\0'))
                    usage(argv[0]);
                *user = optarg;
                uflag = true;
                break;
            case 'm':
                if ((*host) || (*optarg == '\0'))
                    usage(argv[0]);
                *host = optarg;
                break;
            case 'N':
                Nflag = true;
                norOp = optarg;
                break;
            case 'P':
                if ((*projectName) || (*optarg == '\0'))
                    usage(argv[0]);
                *projectName = optarg;
                break;
            case 'W':
                if (optarg == NULL) {
                    Wflag = true;
                    *format = WIDE_FORMAT;
                }
                else {
                    if (*format != 0)
                        usage(argv[0]);
                    switch (optarg[0]) {
                    case 'L':
                        fintimeformat = FORMAT_WL;
                        break;
                    case 'P':
                        fintimeformat = FORMAT_WP;
                        break;
                    case 'F':
                        fintimeformat = FORMAT_WF;
                        break;
                    default:
                        usage(argv[0]);
                        break;
                    }
                }
                break;
            case 'V':
                fputs(_LS_VERSION_, stderr);
                exit(0);
            case 'U':
                if (optarg[0]=='F') {
                    *format = LSFUF_FORMAT;
                    break;
                }
		usage(argv[0]);
            case 'n':
                if (strcmp(optarg,"oheader")==0) {
		    if ((*format==0) || (*format==WIDE_FORMAT)) {
			/* short format or wide format */
                        noheaderflag=true;
		    }
		    break;
                }
                usage(argv[0]);
            case 'g':
                if (*job_group)
                    usage(argv[0]);
                *job_group = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    TIMEIT(1, (numJids = getSpecJobIds (argc, argv, &usrJids, NULL)), "getSpecJobIds");

    if (numJids > 0) {
        *user = "all";
        *options |= ALL_JOB;
    }
    else {
        if (uflag != true && Wflag == true) {
            if ((getuid() == 0) || isLSFAdmin()) {
                *user = "all";
            }
        }
    }

    if ((*options
         & (~JGRP_ARRAY_INFO)) == 0) {
        *options |= CUR_JOB;
    }

    if (Nflag) {
        float *tempPtr;

        *options |= DONE_JOB;
        *format = LONG_FORMAT;
        TIMEIT(0, (tempPtr = getCpuFactor (norOp, false)), "getCpuFactor");
        if (tempPtr == NULL)
            if ((tempPtr = getCpuFactor (norOp, true)) == NULL)
                if (!isanumber_(norOp)
                    || (*cpuFactor = atof(norOp)) <= 0) {
                    fprintf(stderr, (_i18n_msg_get(ls_catd,NL_SETN,1458, "<%s> is neither a host model, nor a host name, nor a CPU factor\n")), norOp); /* catgets  1458  */
                    exit(-1);
                }
        if (tempPtr)
            *cpuFactor = *tempPtr;
    }

}


static void
displayJobs(struct jobInfoEnt *job, struct jobInfoHead *jInfoH,
            int options, int format)
{
    struct submit *submitInfo;
    static char first = true;
    char *status;
    char subtime[64], donetime[64], esttime[64];
    static char  *exechostfmt;
    static struct loadIndexLog *loadIndex = NULL;
    char *exec_host = "";
    char *jobName, *pos;
    struct nameList  *hostList = NULL;
    char tmpBuf[MAXLINELEN];
    int i = 0;
    time_t now;
    int remain;
    float completed;

    if (lsbParams[LSB_SHORT_HOSTLIST].paramValue && job->numExHosts > 1
        && strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0 ) {
        hostList = lsb_compressStrList(job->exHosts, job->numExHosts);

        if (!hostList)
            exit(99);
    }

    if (loadIndex == NULL)
        loadIndex = initLoadIndex();

    submitInfo = &job->submit;
    status = get_status(job);

    strcpy(subtime, _i18n_ctime( ls_catd, CTIME_FORMAT_b_d_H_M, &job->submitTime));
    if (IS_FINISH (job->status))
        strcpy(donetime, _i18n_ctime( ls_catd, CTIME_FORMAT_b_d_H_M, &(job->endTime)));
    else
        strcpy(donetime, "      ");

    if (IS_PEND(job->status))
        exec_host = "";
    else if ( job->numExHosts == 0)
        exec_host = "   -   ";
    else
    {

        if (lsbParams[LSB_SHORT_HOSTLIST].paramValue && job->numExHosts > 1
            && strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0 ) {
            sprintf(tmpBuf, "%d*%s", hostList->counter[0], hostList->names[0]);
            exec_host = tmpBuf;
        }
        else
            exec_host = job->exHosts[0];
    }

    if (first && noheaderflag==false) {
        first = false;
        if (job->jType == JGRP_NODE_ARRAY)
            printf("JOBID    ARRAY_SPEC  OWNER   NJOBS PEND DONE  RUN EXIT SSUSP USUSP PSUSP\n");
        else {
            printf("JOBID   USER    STAT  QUEUE      FROM_HOST   EXEC_HOST   JOB_NAME   SUBMIT_TIME");

            if (Wflag == true) {
                printf("  PROJ_NAME CPU_USED MEM SWAP PIDS START_TIME FINISH_TIME");
            }
            switch (fintimeformat) {
            case FORMAT_WL:
                printf("   TIME_LEFT");
                break;
            case FORMAT_WP:
                printf("   %%COMPLETE");
                break;
            case FORMAT_WF:
                printf("   FINISH_TIME");
                break;
            default:
                break;
            }
            printf("\n");

            exechostfmt = "%45s%-s\n";
        }
    }

    if (job->jType == JGRP_NODE_ARRAY) {
        if (format != WIDE_FORMAT) {
            printf("%-7d  %-8.8s ", LSB_ARRAY_JOBID(job->jobId), job->submit.jobName);
            printf("%8.8s ", job->user);
        }
        else {
            printf("%-7d  %s ", LSB_ARRAY_JOBID(job->jobId), job->submit.jobName);
            printf("%s ", job->user);
        }
        printf("  %5d %4d %4d %4d %4d %5d %5d %5d\n",
               job->counter[JGRP_COUNT_NJOBS],
               job->counter[JGRP_COUNT_PEND],
               job->counter[JGRP_COUNT_NDONE],
               job->counter[JGRP_COUNT_NRUN],
               job->counter[JGRP_COUNT_NEXIT],
               job->counter[JGRP_COUNT_NSSUSP],
               job->counter[JGRP_COUNT_NUSUSP],
               job->counter[JGRP_COUNT_NPSUSP]);
        return;
    }

    jobName = submitInfo->jobName;
    if (LSB_ARRAY_IDX(job->jobId) && (pos = strchr(jobName, '['))) {
        *pos = '\0';
        sprintf(jobName, "%s[%d]", jobName, LSB_ARRAY_IDX(job->jobId));
    }
    if (format != WIDE_FORMAT) {
        TRUNC_STR(jobName, 10);
        printf("%-7d %-7.7s %-5.5s %-10.10s %-11.11s %-11.11s %-10.10s %-14.14s",
               LSB_ARRAY_JOBID(job->jobId), job->user, status,
               submitInfo->queue, job->fromHost,
               exec_host,
               jobName, subtime);
        switch(fintimeformat) {
        case FORMAT_WL:
            if (submitInfo->rLimits[LSF_RLIMIT_RUN] >=0 && job->startTime > 0 &&
                job->endTime <= 0) {
                now = time(NULL);
                remain = (int)(job->startTime + submitInfo->rLimits[LSF_RLIMIT_RUN]
                    - now) / 60;
                printf ("%d:%d L", remain/60, remain%60);
            }
            else
                printf ("   -");
            break;
        case FORMAT_WP:
            if (submitInfo->rLimits[LSF_RLIMIT_RUN] >=0 && job->startTime > 0 &&
                job->endTime <= 0) {
                now = time(NULL);
                completed = (float)(now - job->startTime) /
                    (float)submitInfo->rLimits[LSF_RLIMIT_RUN];
                printf ("%.2f%% L",completed*100.f);
            }
            else
                printf ("   -");
            break;
        case FORMAT_WF:
            if (job->endTime > 0)
                printf("%s",donetime);
            else {
                if (submitInfo->rLimits[LSF_RLIMIT_RUN] >=0 && job->startTime > 0) {
                    now = job->startTime + submitInfo->rLimits[LSF_RLIMIT_RUN];
                    strcpy(esttime, _i18n_ctime( ls_catd, CTIME_FORMAT_b_d_H_M, &(now)));
                    printf("%s", esttime);
                }
                else
                    printf ("   -");
            }
            break;
        default:
            break;
        }
        printf("\n");
    } else {
        if (IS_PEND(job->status)) {
            exec_host = "   -    ";
        } else {
            static char *execHostList;
            static int execHostListSize;
            int execHostListUsed;
            int L;

            /* Assume one page is 4K.
             */
            L = 8 * MAXLINELEN;
            if (execHostList == NULL) {
                if ((execHostList = calloc(1, L)) == NULL) {
                    perror("calloc");
                    exit(-1);
                }
                execHostListSize = L;
            }

            strcpy(execHostList, exec_host);
            execHostListUsed = strlen(exec_host);

            if (lsbParams[LSB_SHORT_HOSTLIST].paramValue
                && job->numExHosts > 1
                &&
                strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0) {
                for (i = 1; i < hostList->listSize; i++) {

                    /* The +4 is to allow for the extra characters
                     * that are added to the node name which include
                     * the number of jobs on a node (e.g., 2* or 4*)
                     * and the delimiter (:). We use +4 instead of
                     * +3 to account for the possibility of running
                     * more than 9 jobs on a node.
                     */
                    execHostListUsed += (strlen(job->exHosts[i]) + 4 + 1);
                    if (execHostListUsed >= execHostListSize) {
                        execHostListSize += L;
                        if ((execHostList = realloc(execHostList,
                                                    execHostListSize)) == NULL) {
                            perror("realloc");
                            exit(-1);
                        }
                    }
                    strcat(execHostList,":");
                    sprintf(tmpBuf, "%d*%s", hostList->counter[i],
                            hostList->names[i]);
                    strcat(execHostList, tmpBuf);
                }
            } else {
                for (i = 1; i < job->numExHosts; i++) {
                    execHostListUsed += (strlen(job->exHosts[i]) + 1);
                    if (execHostListUsed >= execHostListSize) {
                        execHostListSize += L;
                        if ((execHostList = realloc(execHostList,
                                                    execHostListSize)) == NULL) {
                            perror("realloc");
                            exit(-1);
                        }
                    }
                    strcat(execHostList,":");
                    strcat(execHostList, job->exHosts[i]);
                }
            }

            if (execHostList[0] == '\0')
                exec_host = "   -   ";
            else
                exec_host = execHostList;

        }

        if (Wflag == true) {
            printf("%-7d %-7s %-5.5s %-10s %-11s %-11s %-10s %-14.14s",
                   LSB_ARRAY_JOBID(job->jobId),
                   job->user,
                   status,
                   submitInfo->queue,
                   job->fromHost,
                   exec_host,
                   jobName,
                   Time2String(job->submitTime));
        } else {
            printf("%-7d %-7s %-5.5s %-10s %-11s %-11s %-10s %s",
                   LSB_ARRAY_JOBID(job->jobId),
                   job->user,
                   status,
                   submitInfo->queue,
                   job->fromHost,
                   exec_host,
                   jobName,

                   subtime);
        }

        if (Wflag == true) {
            int         i;
            float cpuTime;

            if (job->cpuTime > 0) {
                cpuTime = job->cpuTime;
            }
            else {
                cpuTime = job->runRusage.utime + job->runRusage.stime;
            }
            printf(" %-10s %-10s %-6d %-6d ",
                   job->submit.projectName,
                   Timer2String(cpuTime),
                   ((job->runRusage.mem >0)?job->runRusage.mem :0),
                   ((job->runRusage.swap>0)?job->runRusage.swap:0));
            if (job->runRusage.npids) {
                for (i = 0; i < job->runRusage.npids; i++) {
                    if (i == 0) {
                        printf("%d",job->runRusage.pidInfo[i].pid);
                    } else {
                        printf(",%d",job->runRusage.pidInfo[i].pid);
                    }
                }
            } else {
                printf(" - ");
            }


            if (job->startTime == 0)
                printf(" - ");
            else
                printf(" %s",Time2String(job->startTime));

            if (job->endTime == 0) {
                printf(" - ");
            } else {
                printf(" %s",Time2String(job->endTime));
            }
        }

        printf("\n");
    }


    if (lsbParams[LSB_SHORT_HOSTLIST].paramValue && job->numExHosts > 1
        && strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0 ) {
        if (!IS_PEND(job->status) && format != WIDE_FORMAT) {
            for (i = 1 ; i < hostList->listSize; i++) {
                sprintf(tmpBuf, "%d*%s", hostList->counter[i],
                        hostList->names[i]);
                printf(exechostfmt, "", tmpBuf);
            }
        }
    }
    else {
        if (!IS_PEND(job->status) && format != WIDE_FORMAT) {
            for (i = 1; i < job->numExHosts; i++) {
                printf(exechostfmt, "", job->exHosts[i]);
            }
        }
    }

    if ((options & PEND_JOB) &&  IS_PEND(job->status)) {
        printf("%s", lsb_pendreason(job->numReasons, job->reasonTb, NULL,
                                    loadIndex));
    }


    if ((options & SUSP_JOB) &&  IS_SUSP(job->status)) {
        if (job->status & JOB_STAT_PSUSP && !(options & PEND_JOB))
            printf("%s", lsb_pendreason(job->numReasons, job->reasonTb, NULL,
                                        loadIndex));
        else if (!(job->status & JOB_STAT_PSUSP))
            printf("%s", lsb_suspreason(job->reasons, job->subreasons, loadIndex));
    }

}

static int
skip_job(struct jobInfoEnt *job)
{
    int i;

    for (i = 0; i < numJids; i++) {
        if (job->jobId == usrJids[i] ||
            LSB_ARRAY_JOBID(job->jobId) == usrJids[i]) {
            numJobs[i]++;
            foundJids++;
            return false;
        }
    }

    return true;
}

static int
isLSFAdmin(void)
{
    static char fname[] = "isLSFAdmin";
    struct clusterInfo *clusterInfo;
    char  *mycluster;
    char   lsfUserName[MAXLINELEN];
    int i, j, num;

    if ((mycluster = ls_getclustername()) == NULL) {
        if (logclass & (LC_TRACE))
            ls_syslog(LOG_ERR, "%s: ls_getclustername(): %M", fname);
        return false;
    }

    num = 0;
    if ((clusterInfo = ls_clusterinfo(NULL, &num, NULL, 0, 0)) == NULL) {
        if (logclass & (LC_TRACE))
            ls_syslog(LOG_ERR, "%s: ls_clusterinfo(): %M", fname);
        return false;
    }


    if (getUser(lsfUserName, MAXLINELEN) != 0) {
        ls_syslog(LOG_ERR, I18N_FUNC_FAIL_MM, fname, "getUser");
        return false;
    }

    for (i = 0; i < num; i++) {
        if (!strcmp(mycluster, clusterInfo[i].clusterName)) {
            for (j = 0; j < clusterInfo->nAdmins; j++) {
                if (strcmp(lsfUserName, clusterInfo->admins[j]) == 0)
                    return true;
            }
            return false;
        }
    }

    return false;
}

static char *
Timer2String(float timer)
{
    static char TimerStr[MAX_TIMERSTRLEN];
    int         Hour, Minute, Second, Point, Time;

    Point   = timer*100.0;
    Point   = Point%100;
    Time    = timer;
    Hour    = Time/3600;
    Minute  = (Time%3600)/60;
    Second  = (Time%3600)%60;
    sprintf(TimerStr,"%03d:%02d:%02d.%02d",
            Hour,
            Minute,
            Second,
            Point);
    return TimerStr;
}

static char *
Time2String(int timer)
{
    static char TimeStr[MAX_TIMESTRLEN];
    struct tm *Time;
    time_t tempTime;

    memset(TimeStr, '\0', sizeof(TimeStr));
    tempTime = (time_t) timer;
    Time = (struct tm *) localtime(&tempTime);
    sprintf(TimeStr, "%02d/%02d-%02d:%02d:%02d",
            Time->tm_mon+1,
            Time->tm_mday,
            Time->tm_hour,
            Time->tm_min,
            Time->tm_sec);

    return TimeStr;
}
