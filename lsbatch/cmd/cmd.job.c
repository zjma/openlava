/*
 * Copyright (C) 2016 Teraproc Inc
 * Copyright (C) 2015 David Bigagli
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
#include "cmd.h"
#include "../lib/lsb.h"
#include "../lsbatch.h"


#include <errno.h>

static char *strXfOptions(int);
void prtJobRusage(struct jobInfoEnt *);
static int uf_format=0;

#define prtLineWUF(prline) if (uf_format) printf(prline); else prtLine(prline)

void
prtHeader(struct jobInfoEnt *job, int prt_q, int tFormat)
{
    char prline[MAXLINELEN];

    if (!tFormat) {
        sprintf(prline, "\nJob%s <%s>,", uf_format?"":" Id", lsb_jobid2str(job->jobId));

        prtLineWUF(prline);
        if (job->submit.options & SUB_JOB_NAME) {
            char *jobName, *pos;
            jobName = job->submit.jobName;
            if ((pos = strchr(jobName, '[')) && LSB_ARRAY_IDX(job->jobId)) {
                *pos = '\0';
                sprintf(jobName, "%s[%d]", jobName, LSB_ARRAY_IDX(job->jobId));
            }
            sprintf(prline, " Job Name <%s>,", jobName);
            prtLineWUF(prline);
        }
    }
    if (tFormat) {
        sprintf(prline, ",");
        prtLine(prline);
    }
    sprintf(prline, " User <%s>,", job->user);
    prtLineWUF(prline);

    if (lsbMode_ & LSB_MODE_BATCH) {
        sprintf(prline, " Project <%s>,", job->submit.projectName);
        prtLineWUF(prline);
    }

    if (job->submit.userGroup && job->submit.userGroup[0] != '\0') {
        sprintf(prline, " User Group <%s>,", job->submit.userGroup);
        prtLineWUF(prline);
    }

    if (job->submit.options & SUB_MAIL_USER) {
        sprintf(prline, " Mail <%s>,", job->submit.mailUser);
        prtLineWUF(prline);
    }

    if (prt_q) {
        sprintf(prline, " Status <%s>, Queue <%s>,",
                get_status(job),
                job->submit.queue);
        prtLineWUF(prline);
    }

    /* Interactive job */
    if (job->submit.options & SUB_INTERACTIVE) {
        sprintf(prline, " Interactive");
        if (job->submit.options & SUB_PTY) {
            strcat(prline, " pseudo-terminal");
            if (job->submit.options & (SUB_PTY_SHELL))
                strcat(prline, " shell");
        }
        strcat(prline, " mode,");
        prtLineWUF(prline);
    }

    if (job->jobPriority > 0) {
        sprintf(prline, " Job Priority <%d>,", job->jobPriority);
        prtLineWUF(prline);
    }

    if (job->submit.options2 & (SUB2_JOB_CMD_SPOOL)) {
        if (tFormat)
            sprintf(prline, " Command(Spooled) <%s>", job->submit.command);
        else
            sprintf(prline, " Command(Spooled) <%s>", job->submit.command);
    } else {
        if (tFormat)
            sprintf(prline, " Command <%s>", job->submit.command);
        else
            sprintf(prline, " Command <%s>", job->submit.command);
    }
    prtLineWUF(prline);

    if ((job->submit.options2 & SUB2_JOB_DESC) &&
        (job->submit.job_description)) {
        sprintf(prline, ", Job Description <%s>", job->submit.job_description);
        prtLineWUF(prline);
    }

    if (job->submit.options2 & SUB2_JOB_GROUP) {
        sprintf(prline, ", Job Group <%s>", job->submit.job_group);
        prtLineWUF(prline);
    }

    sprintf(prline, "\n");
    prtLineWUF(prline);
}


void
prtJobSubmit(struct jobInfoEnt *job, int prt_q, int tFormat)
{
    char prline[MAXLINELEN];
    char timeBuf[128];

    if (tFormat) {
        sprintf(timeBuf, " Job <%s> s", lsb_jobid2str(job->jobId));
    } else {
        sprintf(timeBuf, " S");
    }
    sprintf(prline, "%-12.19s:%submitted from host <%s>",
            ctime(&job->submitTime), timeBuf, job->fromHost);

    prtLineWUF(prline);

    if (job->submit.options2 & SUB2_HOLD) {
        sprintf(prline, " on hold");
        prtLineWUF(prline);
    }

    if (prt_q) {
        sprintf(prline, " to Queue <%s>", job->submit.queue);
        prtLineWUF(prline);
    }

    TIMEIT(2, prtBTTime(job), "prtBTTime");
}

void
prtBTTime(struct jobInfoEnt *job)
{
    char prline[MAXLINELEN];

    /* Start time
     */
    if (job->submit.beginTime > 0) {
        sprintf(prline, ", Specified Start Time <%-12.19s>",
                ctime((time_t *)&(job->submit.beginTime)));
        prtLineWUF(prline);
    }

    /* Termination Time
     */
    if (job->submit.termTime > 0) {
        sprintf(prline, ", Specified Termination Time <%-12.19s>",
                ctime((time_t *)&(job->submit.termTime)));
        prtLineWUF(prline);
    }
}

void
prtFileNames(struct jobInfoEnt *job, int prtCwd)
{
    char prline[MAXLINELEN];
    int i;


    if (prtCwd == TRUE) {
        if (job->cwd[0] == '/' || job->cwd[0] == '\\' ||
            (job->cwd[0] != '\0' && job->cwd[1] == ':'))
            sprintf(prline, ", CWD <%s>", job->cwd);
        else if (job->cwd[0] == '\0')
            sprintf(prline, ", CWD <$HOME>");
        else
            sprintf(prline, ", CWD <$HOME/%s>", job->cwd);
        prtLineWUF(prline);
    }

    if (job->submit.options2 & SUB2_IN_FILE_SPOOL)  {
        sprintf(prline, ", Input File(Spooled) <%s>", job->submit.inFile);
        prtLineWUF(prline);
    }

    /* Input file */
    if (job->submit.options & SUB_IN_FILE)  {
        sprintf(prline, ", Input File <%s>", job->submit.inFile);
        prtLineWUF(prline);
    }

    /* Output File */
    if (job->submit.options & SUB_OUT_FILE) {
        sprintf(prline, ", Output File <%s>", job->submit.outFile);
        prtLineWUF(prline);
    }

    /* Error file */
    if (job->submit.options & SUB_ERR_FILE) {
        sprintf(prline, ", Error File <%s>", job->submit.errFile);
        prtLineWUF(prline);
    }

    /* Other files */
    if (job->submit.nxf) {
        sprintf(prline, ", Copy Files");
        prtLineWUF(prline);
    }

    for (i = 0; i < job->submit.nxf; i++) {
        sprintf(prline, " \"%s %s %s\"", job->submit.xf[i].subFn,
                strXfOptions(job->submit.xf[i].options),
                job->submit.xf[i].execFn);
        prtLineWUF(prline);
    }

}

static char *
strXfOptions(int options)
{
    static char s[5];

    s[0] = '\0';
    if (options & XF_OP_SUB2EXEC)
        strcat(s, ">");
    if (options & XF_OP_SUB2EXEC_APPEND)
        strcat(s, ">");
    if (options & XF_OP_EXEC2SUB)
        strcat(s, "<");
    if (options & XF_OP_EXEC2SUB_APPEND)
        strcat(s, "<");

    return (s);
}


void
prtSubDetails(struct jobInfoEnt *job, char *hostPtr, float hostFactor)
{
    char prline[MSGSIZE];
    int  k;

    /* notify user */
    if ((job->submit.options & SUB_NOTIFY_END) &&
        (job->submit.options & SUB_NOTIFY_BEGIN)) {
        sprintf(prline, ", Notify when job begins/ends");
        prtLineWUF(prline);
    }
    else if (job->submit.options & SUB_NOTIFY_BEGIN) {
        sprintf(prline, ", Notify when job begins");
        prtLineWUF(prline);
    }
    else if (job->submit.options & SUB_NOTIFY_END) {
        sprintf(prline, ", Notify when job ends");
        prtLineWUF(prline);
    }


    /* Exclusive excution */
    if (job->submit.options & SUB_EXCLUSIVE) {
        sprintf(prline, ", Exclusive Execution");
        prtLineWUF(prline);
    }

    /* Re-runnable */
    if (job->submit.options & SUB_RERUNNABLE) {
        sprintf(prline, ", Re-runnable");
        prtLineWUF(prline);
    }

    /* Restart Job */
    if (job->submit.options & SUB_RESTART) {
        if (job->submit.options & SUB_RESTART_FORCE)
            sprintf(prline, ", Restart force");
        else
            sprintf(prline, ", Restart");
        prtLineWUF(prline);
    }

    /* Chkpnt info */
    if (job->submit.options & SUB_CHKPNTABLE)  {
        if (job->submit.chkpntPeriod) {
            sprintf (prline, ", Checkpoint period %d min.",
                     (int) job->submit.chkpntPeriod/60);
            prtLineWUF (prline);
        }

        sprintf (prline, ", Checkpoint directory <%s>",
                 job->submit.chkpntDir);
        prtLineWUF (prline);
    }

    /* Number of required processors for parallel job */
    if (job->submit.numProcessors > 1
        || job->submit.maxNumProcessors > 1) {
        if (job->submit.numProcessors == job->submit.maxNumProcessors)
            sprintf(prline, ", %d Processors Requested",
                    job->submit.numProcessors);
        else
            sprintf(prline, ", %d-%d Processors Requested",
                    job->submit.numProcessors,
                    job->submit.maxNumProcessors);
        prtLineWUF(prline);
    }

    /* Resource Req. of Job */
    if (strlen(job->submit.resReq)) {
        sprintf(prline, ", Requested Resources <%s>", job->submit.resReq);
        prtLineWUF(prline);
    }

    /* depend_cond string */
    if (strlen(job->submit.dependCond)) {
        char *temp;
        temp = malloc(strlen(job->submit.dependCond) + 48);
        sprintf(temp, ", Dependency Condition <%s>", job->submit.dependCond);
        prtLineWUF(temp);
        free (temp);
    }

    /* login shell */
    if (strlen(job->submit.loginShell)) {
        sprintf(prline, ", Login Shell <%s>", job->submit.loginShell);
        prtLineWUF(prline);
    }

    /* pre_exec_command string */
    if (strlen(job->submit.preExecCmd)) {
        sprintf(prline, ", Pre-execute Command <%s>", job->submit.preExecCmd);
        prtLineWUF(prline);
    }

    /* User Specified Hosts */
    if (job->submit.numAskedHosts) {
        sprintf(prline, ", Specified Hosts <%s>", job->submit.askedHosts[0]);
        prtLineWUF(prline);
        for (k = 1; k < job->submit.numAskedHosts; k++) {
            sprintf(prline, ", <%s>", job->submit.askedHosts[k]);
            prtLineWUF(prline);
        }
    }

    /* Termination Signal */
    if (job->submit.options & SUB_WINDOW_SIG) {
        sprintf(prline, ", Signal Value <%d>",
                sig_decode(job->submit.sigValue));
        prtLineWUF(prline);
    }

    if (job->submit.options2 & SUB2_JOB_PRIORITY
        && job->submit.userPriority > 0) {
        sprintf(prline, ", User Priority <%d>", job->submit.userPriority);
        prtLineWUF(prline);
    }

    sprintf(prline, ";\n");
    prtLineWUF(prline);

    prtResourceLimit (job->submit.rLimits, hostPtr, hostFactor, NULL);
}

void
prtJobStart(struct jobInfoEnt *job, int prtFlag, int jobPid, int tFormat)
{
    char prline[MAXLINELEN], tBuff[20];
    time_t startTime;



    int                 i = 0;
    struct nameList  *hostList = NULL;


    if (lsbParams[LSB_SHORT_HOSTLIST].paramValue && job->numExHosts > 1
        &&  strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0) {
        hostList = lsb_compressStrList(job->exHosts, job->numExHosts);
        if (!hostList) {
            exit(99);
        }
    }


    if (tFormat) {
        sprintf (tBuff, " %s <%s>", I18N_Job, lsb_jobid2str(job->jobId));
    }
    else if (LSB_ARRAY_IDX(job->jobId) > 0 )
        sprintf (tBuff, " [%d]", LSB_ARRAY_IDX(job->jobId));
    else
        tBuff[0] = '\0';

    if (job->startTime && job->numExHosts) {


        if (job->startTime < job->submitTime)
            startTime = job->submitTime;
        else
            startTime = job->startTime;

        if ((job->submit.options & SUB_PRE_EXEC)
            && (prtFlag != BJOBS_PRINT)) {
            if (prtFlag == BHIST_PRINT_PRE_EXEC)
            {
                if (tBuff[0] == '\0')
                    sprintf(prline, "%s: %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T,
                                        &startTime),
                            I18N(604, "The pre-exec command is started on")); /* catgets  604  */
                else
                    sprintf(prline, "%s: %s %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T,
                                        &startTime),
                            tBuff,
                            I18N(605, "the pre-exec command is started on")); /* catgets  605  */
            }
            else
            {
                if (tBuff[0] == '\0')
                    sprintf(prline, "%s: %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &startTime),
                            I18N(606, "The batch job command is started on")); /*catgets 606 */
                else
                    sprintf(prline, "%s: %s %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &startTime),
                            tBuff,
                            I18N(607, "the batch job command is started on")); /*catgets 607 */
            }
        } else {
            if (jobPid > 0)
            {
                if (tBuff[0] == '\0')
                    sprintf(prline, "%s: %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &startTime),
                            I18N(608, "Started on")); /* catgets  608  */
                else
                    sprintf(prline, "%s:%s %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &startTime),
                            tBuff,
                            I18N(609, "started on")); /* catgets  609  */
            }
            else
            {
                if (tBuff[0] == '\0')
                    sprintf(prline, "%s: %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &startTime),
                            I18N(610, "Dispatched to")); /* catgets 610 */
                else
                    sprintf(prline, "%s: %s %s",
                            _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &startTime),
                            tBuff,
                            I18N(611, "dispatched to")); /* catgets  611 */
            }
        }


        prtLineWUF(prline);
        if (job->numExHosts > 1) {
            sprintf(prline, " %d %s",
                    job->numExHosts,
                    I18N(612, "Hosts/Processors")); /* catgets  612  */
            prtLineWUF(prline);
        }

        if (lsbParams[LSB_SHORT_HOSTLIST].paramValue && job->numExHosts > 1
            && strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0) {
            for (i = 0; i < hostList->listSize; i++) {
                sprintf(prline, " <%d*%s>", hostList->counter[i],
                        hostList->names[i]);
                prtLineWUF(prline);
            }
        } else {
            for (i = 0; i < job->numExHosts; i++) {
                sprintf(prline, " <%s>", job->exHosts[i]);
                prtLineWUF(prline);
            }
        }

        if (job->execHome && strcmp (job->execHome, "")) {
            sprintf(prline, ", %s <%s>",
                    I18N(615, "Execution Home"),  /* catgets 615 */
                    job->execHome);
            prtLineWUF(prline);
        }
        if (job->execCwd && strcmp (job->execCwd, "")) {
            sprintf(prline, ", %s <%s>",
                    I18N(616, "Execution CWD"), /* catgets 616 */
                    job->execCwd);
            prtLineWUF(prline);
        }
        if (job->execUsername && strcmp(job->execUsername, "") &&
            strcmp(job->user, job->execUsername)) {
            sprintf(prline, ", %s <%s>",
                    I18N(617, "Execution user name"), /* catgets 617 */
                    job->execUsername);
            prtLineWUF(prline);
        }
        sprintf(prline, ";\n");
        prtLineWUF(prline);
    }
}

void
prtJobReserv(struct jobInfoEnt *job)
{
    char prline[MAXLINELEN];

    int i = 0;
    struct nameList  *hostList = NULL;

    if (lsbParams[LSB_SHORT_HOSTLIST].paramValue && job->numExHosts > 1
        && strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0) {
        hostList = lsb_compressStrList(job->exHosts, job->numExHosts);
        if (!hostList) {
            exit(99);
        }
    }

    if (job->numExHosts > 0 && job->reserveTime > 0) {
        if (job->numExHosts > 1)
            sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,618, "%s: Reserved <%d> job slots on host(s)")), /* catgets  618  */
                    _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->reserveTime),
                    job->numExHosts);
        else
            sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,619, "%s: Reserved <%d> job slot on host")), /* catgets  619  */
                    _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->reserveTime),
                    job->numExHosts);
        prtLineWUF(prline);

        if (lsbParams[LSB_SHORT_HOSTLIST].paramValue && job->numExHosts > 1
            && strcmp(lsbParams[LSB_SHORT_HOSTLIST].paramValue, "1") == 0) {
            for (i = 0; i < hostList->listSize; i++) {
                sprintf(prline, " <%d*%s>", hostList->counter[i],
                        hostList->names[i]);
                prtLineWUF(prline);
            }
        } else {
            for (i = 0; i < job->numExHosts; i++) {
                sprintf(prline, " <%s>", job->exHosts[i]);
                prtLineWUF(prline);
            }
        }

    }

}

void
prtAcctFinish(struct jobInfoEnt *job)
{
    char prline[MAXLINELEN];

    if (job->status == JOB_STAT_DONE)
        sprintf(prline, "%s: %s.\n",
                _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->endTime),
                I18N(620, "Completed <done>")); /* catgets  620  */
    else if (job->status == JOB_STAT_EXIT)
        sprintf(prline, "%s: %s.\n",
                _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->endTime),
                I18N(621, "Completed <exit>")); /* catgets  621  */
    else
        sprintf(prline, "%s: %s.\n",
                _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->endTime),
                I18N(622, "sbatchd unavail <zombi>"));  /* catgets  622  */
    prtLineWUF(prline);
}

void
prtJobFinish(struct jobInfoEnt *job, struct jobInfoHead *jInfoH)
{
    char prline[MSGSIZE];
    time_t doneTime;
    static struct loadIndexLog *loadIndex = NULL;
    char *pendReasons;

    if (loadIndex == NULL)
        TIMEIT(1, loadIndex = initLoadIndex(), "initLoadIndex");

    doneTime = job->endTime;

    switch (job->status) {
        case JOB_STAT_DONE:
        case (JOB_STAT_DONE | JOB_STAT_PDONE):
        case (JOB_STAT_DONE | JOB_STAT_PERR):

            if ((job->startTime < job->submitTime)
                && (job->endTime < (job->submitTime
                                    + (time_t) MAX(job->cpuTime, MIN_CPU_TIME)))) {
                doneTime = job->submitTime +
                    (time_t) MAX(job->cpuTime, 0.0001);
            } else if (job->startTime >= job->submitTime &&
                       job->endTime < (job->startTime +
                                       (time_t)MAX(job->cpuTime, 0.0001)) &&
                       job->numExHosts == 1) {

                doneTime = job->startTime + (time_t) MAX(job->cpuTime, 0.0001);

                if (job->endTime <= doneTime) {
                    doneTime = job->endTime;
                }


            }

        case (JOB_STAT_EXIT | JOB_STAT_PDONE):
        case (JOB_STAT_EXIT | JOB_STAT_PERR):
        case JOB_STAT_EXIT:
            if (job->reasons & EXIT_ZOMBIE) {
                sprintf(prline, "%s: ",
                        _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->endTime));
                prtLineWUF(prline);
                sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,623, "Termination request issued; the job will be killed once the host is ok;"))); /* catgets  623  */
                prtLineWUF(prline);
                break;
            }
            sprintf(prline, "%s: ",
                    _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &doneTime));
            prtLineWUF(prline);
            if (strcmp(get_status(job), "DONE") == 0)
            {
                sprintf(prline, I18N(624, "Done successfully.")); /* catgets 624 */
            }
            else {
                LS_WAIT_T wStatus;

                LS_STATUS(wStatus) = job->exitStatus;

                if (job->cpuTime >= MIN_CPU_TIME && job->exitStatus) {
                    if (WEXITSTATUS(wStatus))
                        sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,626, "Exited with exit code %d.")), /* catgets  626  */
                                WEXITSTATUS(wStatus));
                    else
                        sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,627, "Exited by signal %d.")), WTERMSIG(wStatus)); /* catgets  627  */
                } else
                    sprintf(prline, I18N_Exited);
            }

            prtLineWUF(prline);

            if (job->numExHosts > 0) {
                if (job->cpuTime < MIN_CPU_TIME)
                    sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,629, " The CPU time used is unknown.\n"))); /* catgets  629  */
                else
                    sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,630, " The CPU time used is %1.1f seconds.\n")),  /* catgets  630  */
                            job->cpuTime);
            } else {
                sprintf(prline, "\n");
            }

            prtLineWUF(prline);
            break;
        case JOB_STAT_PSUSP:
        case JOB_STAT_PEND:
            sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,631, " PENDING REASONS:\n"))); /* catgets  631  */
            prtLineWUF(prline);
            pendReasons = lsb_pendreason(job->numReasons, job->reasonTb,
                                         jInfoH, loadIndex);
            prtLineWUF(pendReasons);
            break;
        case JOB_STAT_SSUSP:
        case JOB_STAT_USUSP:


            TIMEIT(1, prtJobRusage(job), "prtJobRusage");

            sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,632, " SUSPENDING REASONS:\n"))); /* catgets  632  */
            prtLineWUF(prline);

            if (job->reasons) {
                sprintf(prline, "%s", lsb_suspreason(job->reasons,
                                                     job->subreasons,
                                                     loadIndex));
                prtLineWUF(prline);
            }
            break;
        case JOB_STAT_RUN:

            TIMEIT(1, prtJobRusage(job), "prtJobRusage");
            break;
        default:
            break;
    }
}

char *
get_status(struct jobInfoEnt *job)
{
    char *status;

    switch (job->status) {
        case JOB_STAT_NULL:
            status = "NULL";
            break;
        case JOB_STAT_PEND:
            status = "PEND";
            break;
        case JOB_STAT_PSUSP:
            status = "PSUSP";
            break;
        case JOB_STAT_RUN:
            status = "RUN";
            break;
        case JOB_STAT_RUN | JOB_STAT_WAIT:
            status = "WAIT";
            break;
        case JOB_STAT_SSUSP:
            status = "SSUSP";
            break;
        case JOB_STAT_USUSP:
            status = "USUSP";
            break;
        case JOB_STAT_EXIT:
            if (job->reasons & EXIT_ZOMBIE)
                status = "ZOMBI";
            else
                status = "EXIT";
            break;
        case JOB_STAT_DONE:
        case JOB_STAT_DONE|JOB_STAT_PDONE:
        case JOB_STAT_DONE|JOB_STAT_WAIT:
        case JOB_STAT_DONE|JOB_STAT_PERR:
            status = "DONE";
            break;
        case JOB_STAT_UNKWN:
            status = "UNKWN";
            break;
        default:
            status = "ERROR";
    }

    return status;
}


struct loadIndexLog *
initLoadIndex(void)
{
    int i;
    struct lsInfo *lsInfo;
    static struct loadIndexLog loadIndex;
    static char *defNames[] = {"r15s", "r1m", "r15m", "ut", "pg", "io", "ls",
                               "it", "swp", "mem", "tmp"};
    static char **names;

    TIMEIT(1, (lsInfo = ls_info()), "ls_info");
    if (lsInfo == NULL) {
        loadIndex.nIdx = 11;
        loadIndex.name = defNames;
    } else {
        loadIndex.nIdx = lsInfo->nRes;
        if (!names)
            if(!(names = malloc(lsInfo->nRes * sizeof(char *)))) {
                lserrno = LSE_MALLOC;
                ls_perror("initLoadIndex");
                return NULL;
            }
        for (i = 0; i < loadIndex.nIdx; i++)
            names[i] = lsInfo->resTable[i].name;
        loadIndex.name = names;
    }
    return (&loadIndex);
}

void
prtJobRusage(struct jobInfoEnt *job)
{
    char prline[MAXLINELEN];

    int i, j;
    int linepos;

    if (IS_FINISH(job->status))
        return;

    if (IS_PEND(job->status)) {
        if (job->runRusage.utime || job->runRusage.stime) {
            if (uf_format)
                printf ("%s: Resource usage collected. The CPU time used is %d seconds.",
                        _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->jRusageUpdateTime),
                        job->runRusage.utime + job->runRusage.stime);
            else {
                sprintf(prline, "%s: %s.\n",
                        _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T,
                                    &job->jRusageUpdateTime),
                        I18N(644, "Resource usage collected")); /* catgets 644  */
                prtLine(prline);
                sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,645, "                     The CPU time used is %d seconds.\n")),  /* catgets  645  */
                        job->runRusage.utime + job->runRusage.stime);
                prtLine(prline);
            }
        }
        return;
    };




    if (job->runRusage.utime > 0 || job->runRusage.stime > 0
        || job->runRusage.mem > 0 || job->runRusage.swap > 0
        || job->runRusage.npgids > 0 || job->runRusage.npids > 0) {
        if (uf_format)
            printf ("%s: Resource usage collected.",
                    _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->jRusageUpdateTime));
        else {
            sprintf(prline, "%s: %s.\n",
                    _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T,
                                &job->jRusageUpdateTime),
                    I18N(646, "Resource usage collected")); /* catgets  646  */
            prtLine(prline);
        }
    } else
        return;

    if (job->runRusage.utime > 0 || job->runRusage.stime > 0) {
        if (uf_format)
            printf (" The CPU time used is %d seconds.",
                    job->runRusage.utime + job->runRusage.stime);
        else {
            sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,647, "                     The CPU time used is %d seconds.\n")), /* catgets  647  */
                    job->runRusage.utime + job->runRusage.stime);
            prtLine(prline);
        }
    }


    if (job->runRusage.mem > 0) {
        if (uf_format) {
            if (job->runRusage.mem > 1024)
                printf(" MEM: %d Mbytes;", job->runRusage.mem/1024);
            else
                printf(" MEM: %d Kbytes;", job->runRusage.mem);
        }
        else {
            if (job->runRusage.mem > 1024)
                sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,648, "                     MEM: %d Mbytes")), job->runRusage.mem/1024); /* catgets  648  */
            else
                sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,649, "                     MEM: %d Kbytes")), job->runRusage.mem); /* catgets  649  */
            prtLine(prline);
        }
    }

    if (job->runRusage.swap > 0) {
        char *space;

        if (job->runRusage.mem > 0)
            space = ";  ";
        else
            space = "                     ";

        if (uf_format) {
            if (job->runRusage.swap > 1024)
                printf(" SWAP: %d Mbytes;", job->runRusage.swap/1024);
            else
                printf(" SWAP: %d Kbytes;", job->runRusage.swap);
        }
        else {
            if (job->runRusage.swap > 1024)
                sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,650, "%sSWAP: %d Mbytes\n")), space, /* catgets  650  */
                        job->runRusage.swap/1024);
            else
                sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,651, "%sSWAP: %d Kbytes\n")), space, job->runRusage.swap); /* catgets  651  */
            prtLine(prline);
        }
    } else {
        if (job->runRusage.mem > 0 && !uf_format) {
            sprintf(prline, "\n");
            prtLine(prline);
        }
    }

    if (job->runRusage.npgids <= 0)
        return;


    for (i=0; i < job->runRusage.npgids; i++) {
        if (uf_format)
            printf (" PGID: %d; ", job->runRusage.pgid[i]);
        else {
            sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,652, "                     PGID: %d;  ")), job->runRusage.pgid[i]); /* catgets  652  */
            linepos = strlen(prline);
            prtLine(prline);
        }
        sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,653, "PIDs: "))); /* catgets  653  */
        linepos += 6;
        prtLineWUF(prline);
        for (j=0; j < job->runRusage.npids; j++) {
            if (job->runRusage.pgid[i] == job->runRusage.pidInfo[j].pgid) {
                sprintf(prline, "%d ", job->runRusage.pidInfo[j].pid);
                if (uf_format)
                    printf ("%d%s", job->runRusage.pidInfo[j].pid, j==job->runRusage.npids-1?"":" ");
                else {
                    linepos += strlen(prline);

                    if (linepos >= 80) {
                        char *newline ="\n                     ";
                        prtLine(newline);
                        prtLine(prline);
                        linepos = strlen(prline) + 21;
                    }
                    else
                        prtLine(prline);
                }
            }
        }
        if (uf_format)
            printf(";");
        else {
            sprintf(prline, "\n");
            prtLine(prline);
        }
    }
    sprintf(prline, "\n");
    prtLineWUF(prline);

    if (uf_format && job->runRusage.mem > 0) {
        printf ("\n MEMORY USAGE:\n");
        printf (" MAX MEM: N/A MBytes;  AVG MEM: N/A MBytes\n");
    }
}

void
displayLong(struct jobInfoEnt *job,
            struct jobInfoHead *jInfoH,
            float cpuFactor)
{
    char *hostPtr, *sp;
    char hostName[MAXHOSTNAMELEN];
    float hostFactor, *getFactor;
    static int first = TRUE;
    static struct lsInfo *lsInfo;
    char prline[MAXLINELEN];

    if (first) {
        first = FALSE;
        TIMEIT(0, (lsInfo = ls_info()), "ls_info");
        if (lsInfo == NULL) {
            ls_perror("ls_info");
            exit(-1);
        }
    }

    prtHeader(job, TRUE, FALSE);
    prtJobSubmit(job, FALSE, FALSE);
    TIMEIT(1, prtFileNames(job, TRUE), "prtFileNames");
    hostPtr = job->submit.hostSpec;
    hostFactor = 1.0;

    if (job->numExHosts > 0
        && (strcmp (job->exHosts[0], LOST_AND_FOUND) != 0)
        && !IS_PEND (job->status)) {
        strcpy (hostName, job->exHosts[0]);

        if ((sp = strstr (hostName, "@")) != NULL) {
            *sp = '\0';

            if (strcmp(hostName, hostPtr) == 0) {
                if ((getFactor=getCpuFactor(hostName, TRUE)) == NULL) {
                    prtLine("\n");
                    fprintf(stderr,
                            (_i18n_msg_get(ls_catd,NL_SETN,1451, "Cannot obtain execution host information: %s\n")), ls_errmsg[lserrno]);
                    exit(-1);
                } else {
                    hostFactor = *getFactor;
                    hostPtr = job->exHosts[0];
                }
            }
        }
    }

    TIMEIT(1, prtSubDetails(job, hostPtr, hostFactor), "prtSubDetails");
    if (job->numExHosts > 0 && job->reserveTime > 0) {
        TIMEIT(1, prtJobReserv(job), "prtJobReserv");
        sprintf(prline, ";\n");
        prtLine(prline);
    }


    if (job->predictedStartTime && IS_PEND(job->status)) {
        char localTimeStr[60];
        strcpy ( localTimeStr, _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->predictedStartTime));
        sprintf(prline, (_i18n_msg_get(ls_catd,NL_SETN,1466, "%s: Will be started;\n")), /* catgets  1466  */
                localTimeStr );
        prtLine(prline);
    }

    if (job->startTime && !IS_PEND(job->status)) {
        TIMEIT(1, prtJobStart(job, BJOBS_PRINT, job->jobPid, FALSE), "prtJobStart");
    }

    if ((cpuFactor > 0.0) && (job->cpuTime > 0))
        job->cpuTime = job->cpuTime * hostFactor / cpuFactor;

    if (job->jType == JGRP_NODE_ARRAY) {

        printf("\n %s:\n",
               _i18n_msg_get(ls_catd,NL_SETN,1467, "COUNTERS")); /* catgets  1467  */
        printf( (_i18n_msg_get(ls_catd,NL_SETN,1468, " NJOBS PEND DONE RUN EXIT SSUSP USUSP PSUSP\n"))); /* catgets  1468  */
        printf(" %5d %4d %3d %4d %4d %5d %5d %5d\n",
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
    TIMEIT(1, prtJobFinish(job, jInfoH), "prtJobFinish");

    if (lsbMode_ & LSB_MODE_BATCH) {
        sprintf(prline, "\n %s:\n",
                _i18n_msg_get(ls_catd,NL_SETN,1469, "SCHEDULING PARAMETERS")); /* catgets  1469  */
        prtLine(prline);
        if (printThresholds (job->loadSched, job->loadStop, NULL, NULL,
                             job->nIdx, lsInfo) < 0)
            exit (-1);
    }

    return;
}

void
displayUF (struct jobInfoEnt *job, struct jobInfoHead *jInfoH, float cpuFactor)
{
    char *hostPtr, *sp;
    char hostName[MAXHOSTNAMELEN];
    float hostFactor, *getFactor;
    static int first = TRUE;
    static struct lsInfo *lsInfo;

    if (first) {
        first = FALSE;
        TIMEIT(0, (lsInfo = ls_info()), "ls_info");
        if (lsInfo == NULL) {
            ls_perror("ls_info");
            exit(-1);
        }
    }

    uf_format=1;

    prtHeader(job, TRUE, FALSE);
    prtJobSubmit(job, FALSE, FALSE);
    TIMEIT(1, prtFileNames(job, TRUE), "prtFileNames");
    hostPtr = job->submit.hostSpec;
    hostFactor = 1.0;

    if (job->numExHosts > 0
        && (strcmp (job->exHosts[0], LOST_AND_FOUND) != 0)
        && !IS_PEND (job->status)) {
        strcpy (hostName, job->exHosts[0]);

        if ((sp = strstr (hostName, "@")) != NULL) {
            *sp = '\0';

            if (strcmp(hostName, hostPtr) == 0) {
                if ((getFactor=getCpuFactor(hostName, TRUE)) == NULL) {
                    printf("\n");
                    fprintf(stderr,
                            (_i18n_msg_get(ls_catd,NL_SETN,1451, "Cannot obtain execution host information: %s\n")), ls_errmsg[lserrno]);
                    exit(-1);
                } else {
                    hostFactor = *getFactor;
                    hostPtr = job->exHosts[0];
                }
            }
        }
    }

    TIMEIT(1, prtSubDetails(job, hostPtr, hostFactor), "prtSubDetails");
    if (job->numExHosts > 0 && job->reserveTime > 0) {
        TIMEIT(1, prtJobReserv(job), "prtJobReserv");
        printf(";\n");
    }


    if (job->predictedStartTime && IS_PEND(job->status)) {
        char localTimeStr[60];
        strcpy ( localTimeStr, _i18n_ctime(ls_catd, CTIME_FORMAT_a_b_d_T, &job->predictedStartTime));
        printf((_i18n_msg_get(ls_catd,NL_SETN,1466, "%s: Will be started;\n")), /* catgets  1466  */
               localTimeStr );
    }

    if (job->startTime && !IS_PEND(job->status)) {
        TIMEIT(1, prtJobStart(job, BJOBS_PRINT, job->jobPid, FALSE), "prtJobStart");
    }

    if ((cpuFactor > 0.0) && (job->cpuTime > 0))
        job->cpuTime = job->cpuTime * hostFactor / cpuFactor;

    if (job->jType == JGRP_NODE_ARRAY) {

        printf("\n %s:\n",
               _i18n_msg_get(ls_catd,NL_SETN,1467, "COUNTERS")); /* catgets  1467  */
        printf( (_i18n_msg_get(ls_catd,NL_SETN,1468, " NJOBS PEND DONE RUN EXIT SSUSP USUSP PSUSP\n"))); /* catgets  1468  */
        printf(" %5d %4d %3d %4d %4d %5d %5d %5d\n",
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
    TIMEIT(1, prtJobFinish(job, jInfoH), "prtJobFinish");

    if (lsbMode_ & LSB_MODE_BATCH) {
        printf("\n %s:\n",
               _i18n_msg_get(ls_catd,NL_SETN,1469, "SCHEDULING PARAMETERS")); /* catgets  1469  */
        if (printThresholds (job->loadSched, job->loadStop, NULL, NULL,
                             job->nIdx, lsInfo) < 0)
            exit (-1);
    }

    printf ("\n RESOURCE REQUIREMENT DETAILS:\n");
    printf (" Combined: %s\n Effective: %s\n", job->submit.resReq, job->submit.resReq);
    return;
}
