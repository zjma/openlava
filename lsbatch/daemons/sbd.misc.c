/*
 * Copyright (C) 2015-2016 David Bigagli
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#define _GNU_SOURCE
#include <sched.h>

#include "sbd.h"

#define NL_SETN         11

extern short mbdExitVal;
extern int mbdExitCnt;

/* The cpu array
 */
static struct ol_core *cores;
static int num_cores;

#ifdef HAVE_HWLOC_H
static int numa_enabled;
#endif

void
milliSleep( int msec )
{
    struct timeval dtime;

    if (msec < 1)
        return;
    dtime.tv_sec = msec/1000;
    dtime.tv_usec = (msec - dtime.tv_sec * 1000) * 1000;

    select( 0,0,0,0, &dtime );

}



char
window_ok (struct jobCard *jobPtr)
{
    windows_t *wp;
    struct dayhour dayhour;
    char active;
    time_t ckTime;
    time_t now;

    now = time(0);
    active = jobPtr->active;

    if (active && (jobPtr->jobSpecs.options & SUB_WINDOW_SIG))
        ckTime = now + WARN_TIME;
    else
        ckTime = now;

    if (jobPtr->windEdge > ckTime || jobPtr->windEdge == 0)
        return (jobPtr->active);

    getDayHour (&dayhour, ckTime);
    if (jobPtr->week[dayhour.day] == NULL) {
        jobPtr->active = TRUE;
        jobPtr->windEdge = now + (24.0 - dayhour.hour) * 3600.0;
        return (jobPtr->active);
    }

    jobPtr->active = FALSE;
    jobPtr->windEdge = now + (24.0 - dayhour.hour) * 3600.0;
    for (wp = jobPtr->week[dayhour.day]; wp; wp=wp->nextwind)
        checkWindow(&dayhour, &jobPtr->active, &jobPtr->windEdge, wp, now);

    if (active && !jobPtr->active && now - jobPtr->windWarnTime >= WARN_TIME
        && (jobPtr->jobSpecs.options & SUB_WINDOW_SIG)) {

        if (!(jobPtr->jobSpecs.jStatus & JOB_STAT_RUN))
            job_resume(jobPtr);
        jobsig (jobPtr, sig_decode (jobPtr->jobSpecs.sigValue), TRUE);
        jobPtr->windWarnTime = now;
    }

    return (jobPtr->active);

}
void
shout_err (struct jobCard *jobPtr, char *msg)
{
    char buf[MSGSIZE];

    sprintf(buf, \
            "We are unable to run your job %s:<%s>. The error is:\n%s.",
            lsb_jobid2str(jobPtr->jobSpecs.jobId),
            jobPtr->jobSpecs.command, msg);

    if (jobPtr->jobSpecs.options & SUB_MAIL_USER) {
        merr_user(jobPtr->jobSpecs.mailUser, jobPtr->jobSpecs.fromHost,
                  buf, I18N_error);
    } else {
        merr_user(jobPtr->jobSpecs.userName, jobPtr->jobSpecs.fromHost,
                  buf, I18N_error);
    }
}

void
child_handler(int sig)
{
    int             pid;
    LS_WAIT_T       status;
    struct rusage   rusage;
    register float  cpuTime;
    struct lsfRusage lsfRusage;
    struct jobCard *jobCard;
    static short lastMbdExitVal = MASTER_NULL;
    static int sbd_finish_sleep = -1;

    cleanRusage (&rusage);
    now = time(0);

    while ((pid = wait3(&status, WNOHANG, &rusage)) > 0) {
        if (pid == mbdPid) {
            int sig = WTERMSIG(status);
            if (mbdExitCnt > 150)
                mbdExitCnt = 150;
            mbdExitVal = WIFSIGNALED(status);
            if (mbdExitVal) {
                ls_syslog(LOG_ERR, "\
%s: mbatchd died with signal <%d> termination", __func__, sig);
                if (WCOREDUMP(status))
                    ls_syslog(LOG_ERR, "%s: mbatchd core dumped", __func__);
                mbdExitVal = sig;
                if (mbdExitVal == lastMbdExitVal)
                    mbdExitCnt++;
                else {
                    mbdExitCnt = 0;
                    lastMbdExitVal = mbdExitVal;
                }
                continue;
            } else {
                mbdExitVal = WEXITSTATUS(status);

                if (mbdExitVal == lastMbdExitVal)
                    mbdExitCnt++;
                else {
                    mbdExitCnt = 0;
                    lastMbdExitVal = mbdExitVal;
                }
                if (mbdExitVal == MASTER_RECONFIG) {
                    ls_syslog(LOG_NOTICE, "\
%s: mbatchd resigned for reconfiguration", __func__);
                    start_master();
                } else
                    ls_syslog(LOG_NOTICE, "\
%s: mbatchd exited with value <%d>", __func__, mbdExitVal);
                continue;
            }
        }

        ls_ruunix2lsf (&rusage, &lsfRusage);
        cpuTime = lsfRusage.ru_utime + lsfRusage.ru_stime;

        for (jobCard = jobQueHead->forw; (jobCard != jobQueHead);
             jobCard = jobCard->forw) {

            if (jobCard->exitPid == pid) {
                jobCard->w_status = LS_STATUS(status);
                jobCard->exitPid = -1;
            }

            if (jobCard->jobSpecs.jobPid == pid) {
                jobCard->collectedChild = TRUE;
                jobCard->cpuTime = cpuTime;
                jobCard->w_status = LS_STATUS(status);
                jobCard->exitPid = -1;
                memcpy ((char *) &jobCard->lsfRusage, (char *) &lsfRusage,
                        sizeof (struct lsfRusage));
                jobCard->notReported++;

                if (sbd_finish_sleep < 0) {
                    if (daemonParams[LSB_SBD_FINISH_SLEEP].paramValue) {
                        errno = 0;
                        sbd_finish_sleep = atoi(daemonParams[LSB_SBD_FINISH_SLEEP].paramValue);
                        if (errno)
                            sbd_finish_sleep = 0;
                    } else {
                        sbd_finish_sleep=0;
                    }
                }
                if (sbd_finish_sleep > 0) {
                    millisleep_(sbd_finish_sleep);
                }

                need_checkfinish = TRUE;
                break;
            }
        }
    }
}

#ifndef BSIZE
#define BSIZE 1024
#endif

int
fcp(char *file1, char *file2, struct hostent *hp)
{
    static char fname[] = "fcp()";
    struct stat sbuf;
    int fd1, fd2;
    char buf[BSIZE];
    int cc;

    fd1 = myopen_(file1, O_RDONLY, 0, hp);
    if (fd1 < 0)
        return -1;

    if (fstat(fd1, &sbuf) < 0) {
        ls_syslog(LOG_ERR, I18N_FUNC_S_FAIL_M, fname, "fstat", file1);
        close(fd1);
        return -1;
    }

    fd2 = myopen_(file2, O_CREAT | O_TRUNC | O_WRONLY, (int) sbuf.st_mode, hp);
    if (fd2 < 0) {
        ls_syslog(LOG_ERR, I18N_FUNC_S_FAIL_M, fname, "creat", file1);
        close(fd1);
        return -1;
    }

    for (;;) {
        cc = read(fd1, buf, BSIZE);
        if (cc == 0)
            break;
        if (cc < 0) {
            close(fd1);
            close(fd2);
            return -1;
        }
        if (write(fd2, buf, cc) != cc) {
            close(fd1);
            close(fd2);
            return -1;
        }
    }

    close(fd1);
    close(fd2);
    return 0;
}

#ifdef __sun__
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

int
rmDir(char *dir)
{
    DIR *dirp;
    struct dirent *dp;
    char path[MAXPATHLEN];

    if ((dirp = opendir(dir)) == NULL)
        return -1;

    readdir(dirp); readdir(dirp);

    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        sprintf (path, "%s/%s", dir, dp->d_name);
        rmdir (path);
        unlink (path);
    }

    closedir (dirp);
    return rmdir(dir);
}


void closeBatchSocket (void)
{
    if (batchSock > 0) {
        chanClose_(batchSock);
        batchSock = -1;
    }
}

void
getManagerId(struct sbdPackage *sbdPackage)
{
    struct passwd *pw;
    int i;

    FREEUP(lsbManager);

    if (sbdPackage->nAdmins <= 0) {
        ls_syslog(LOG_ERR, "\
%s: No LSF administrator defined in sbdPackage from MBD.", __func__);
        die(FATAL_ERR);
    }

    for (i = 0; i < sbdPackage->nAdmins; i++) {
        if ((pw = getpwnam(sbdPackage->admins[i])) != NULL) {
            lsbManager = safeSave(sbdPackage->admins[i]);
            managerId  = pw->pw_uid;
            break;
        }
    }

    if (lsbManager == NULL) {
        ls_syslog(LOG_ERR, "\
%s: getpwnam() failed for LSF administrator defined in sbdPackage.\
 Non uniform userid space?",
                  __func__);
        die(FATAL_ERR);
    }
}

/* init_cores()
 */
void
init_cores(void)
{
    int i;

    if (! daemonParams[SBD_BIND_CPU].paramValue
        && !hostAffinity)
        return;

#ifdef HAVE_HWLOC_H
    /* use numa topology if OS is numa-aware */
    if ((numa_enabled = init_numa_topology()))
        return;
#endif

    num_cores = ls_get_numcpus();
    if (num_cores <= 0) {
        ls_syslog(LOG_ERR, "%s: huh no cores on this box?", __func__);
        return;
    }

    cores = calloc(num_cores, sizeof(struct ol_core));

    for (i = 0; i < num_cores; i++) {
        cores[i].core_num = i;
    }
}

/* find_free_core()
 *
 * bind multiple cores for numa topology
 * only bind 1 core for non-numa
 *
 */
int*
find_free_core(int num)
{
    int i;
    int* selected;

#ifdef HAVE_HWLOC_H
    if (numa_enabled)
        return find_free_numa_core(num);
#endif

    if (! cores)
        return NULL;

    for (i = 0; i < num_cores; i++) {
        if (cores[i].bound == 0) {
            selected = calloc(1, sizeof(int));
            *selected = cores[i].core_num;
            return selected;
        }
    }

    return NULL;
}

/* bind_to_core()
 *
 * bind multiple cores for numa topology
 * only bind 1 core for non-numa
 *
 */
int
bind_to_core(pid_t pid, int num, int* selected_cores)
{
    cpu_set_t  set;
    int cc;

#ifdef HAVE_HWLOC_H
    if (numa_enabled)
        return bind_to_numa_core(pid, num, selected_cores);
#endif

    if (! cores)
        return -1;

    CPU_ZERO(&set);
    CPU_SET(selected_cores[0], &set);

    cc = sched_setaffinity(pid, sizeof(cpu_set_t), &set);
    if (cc < 0) {
        ls_syslog(LOG_ERR, "\
%s: failed binding process %d to cpu %d %m", __func__, pid, selected_cores[0]);
        return -1;
    }
    cores[selected_cores[0]].bound++;

    return 0;
}

/* free_core()
 *
 * bind multiple cores for numa topology
 * only bind 1 core for non-numa
 *
 */
void
free_core(int num, int* core_num)
{
    int i;

#ifdef HAVE_HWLOC_H
    if (numa_enabled)
        return free_numa_core(num, core_num);
#endif

    if (! cores)
        return;

    /* Uot??
     */
    if (core_num < 0)
        return;

    for (i = 0; i < num_cores; i++) {
        if (cores[i].core_num == core_num[0]) {
            cores[i].bound--;
            return;
        }
    }
}

/* find_bound_core()
 *
 * bind multiple cores for numa topology
 * only bind 1 core for non-numa
 *
 */
int*
find_bound_core(pid_t pid)
{
    cpu_set_t set;
    int cc;
    int* selected_cores;

#ifdef HAVE_HWLOC_H
    if (numa_enabled)
        return find_numa_bound_core(pid);
#endif

    if (! cores)
        return NULL;

    CPU_ZERO(&set);

    cc = sched_getaffinity(pid, sizeof(cpu_set_t), &set);
    if (cc < 0) {
        ls_syslog(LOG_ERR, "\
%s: sched_getaffinity() failed for pid %d %m", __func__, pid);
        return NULL;
    }

    for (cc = 0; cc < num_cores; cc++) {
        if (CPU_ISSET(cc, &set)) {
            selected_cores = calloc(1, sizeof(int));
            *selected_cores = cc;
            return selected_cores;
        }
    }

    return NULL;
}
