/*
 * Copyright (C) 2011-2012 David Bigagli
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

#include "lim.h"
#include <sys/statvfs.h>
#include <sys/stat.h>

static char cbuf[BUFSIZ];
static int swap;
static int mem;
static int paging;
static int ut;
static int numdisk;

/* numCpus()
 */
int
numCpus(void)
{
    int ncpus;
    FILE *fp;

/* Returns one line information per CPU.
 */
#define NCPUS "/usr/sbin/psrinfo"

    fp = popen(NCPUS, "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: popen() %s failed %m", __func__, NCPUS);
        return 0;
    }

    ncpus = 0;
    while (fgets(cbuf, BUFSIZ, fp)) {
        ++ncpus;
    }
    pclose(fp);

    return ncpus;
}

/* queueLengthEx()
 */
int
queueLengthEx(float *r15s, float *r1m, float *r15m)
{
    FILE *fp;

/* This is how we get the machine load average
 * over the last 1, 5 and 15 minutes
 */
#define LOADAV "/usr/bin/uptime|/usr/bin/awk '{print $(NF-2), $(NF-1), $NF}'|/usr/bin/sed -e's/,//g'"

    fp = popen(LOADAV, "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: popen() %s failed %m", __func__, LOADAV);
        return 1;
    }
    fscanf(fp,"%f%f%f", r15s, r1m, r15m);
    pclose(fp);

    ls_syslog(LOG_DEBUG, "\
%s: Got run queue length r15s %f r1m %f r15m %f",
              __func__, *r15s, *r1m, *r15m);

    return 0;
}
/* Get the dynamic values for swap, memory paging and CPU utilization.
 */

/* cpuTime()
 */
void
cpuTime(float *itime, float *cpuut)
{
    FILE *fp;

    fp = fopen("/var/tmp/lim.runload", "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: fopen() /var/tmp/lim failed %m", __func__);
        return;
    }
    fscanf(fp, "\
%d %d %d %d", &swap, &mem, &paging, &ut);
    fclose(fp);
    /* The caller expects a value < 1.
     */
    *cpuut = (float)ut/100.0;

    ls_syslog(LOG_DEBUG, "\
%s: swap %d mem %d paging %d ut %d", __func__,
              swap, mem, paging, ut);
}

/* realMem()
 * Return free memory.
 */
int
realMem(float extrafactor)
{
    float x;
    /* Free memory is reported in kilobytes so
     * we convert it to MB.
     */
    x = (float)mem/1024.0;
    return x;
}

/* tmpspace()
 */
float
tmpspace(void)
{
    struct statvfs fs;
    double tmps;

    tmps = 0.0;
    if (statvfs("/tmp", &fs) < 0) {
        ls_syslog(LOG_ERR, "%s: statfs() /tmp failed: %m", __FUNCTION__);
        return 0.0;
    }

    tmps = (double)fs.f_bavail / ((double)(1024 * 1024)/fs.f_bsize);

    return tmps;
}

/* getswap()
 */
float
getswap(void)
{
    float x;
    /* Convert to MB
     */
    x = swap/1024.0;
    return x;
}

/* getpaging()
 */
float
getpaging(float etime)
{
    return (float)paging;
}

float
getIoRate(float etime)
{
    FILE *fp;
    float iorate;

    fp = fopen("/var/tmp/lim.iorate", "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: popen() /var/tmp/lim.iorate failed %m", __func__);
        return 0.0;
    }
    fscanf(fp, "%f", &iorate);
    fclose(fp);

    return iorate;
}

/* get the login sessions
 */
#define LS "/usr/bin/w -h|/usr/bin/wc -l"
int
getLogin(void)
{
    FILE *fp;
    int logz;

    fp = popen(LS, "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: popen() %s failed %m", __func__, LS);
        return 0;
    }
    fscanf(fp, "%d", &logz);
    pclose(fp);

    return logz;
}

/* The memory is in megabytes already.
 */
#define MEMMAX "/usr/sbin/prtconf|/usr/bin/awk '{if ($1 == \"Memory\"){print $3; exit}}'"
/* swap -s pritns kilobytes so we convert it to megabytes, awk
 * has dynamic type conversion.
 */
#define SWAPMAX "/usr/sbin/swap -s|/usr/bin/awk '{a = $(NF-1); x = substr(a, 0, length(a) - 1); print x;}"
/* initReadLoad()
 */
void
initReadLoad(int checkMode, int *kernelPerm)
{
    float  maxmem;
    float maxswap;
    struct statvfs fs;
    FILE *fp;

    myHostPtr->loadIndex[R15S] =  0.0;
    myHostPtr->loadIndex[R1M]  =  0.0;
    myHostPtr->loadIndex[R15M] =  0.0;

    if (checkMode)
        return;

    if (statvfs("/tmp", &fs) < 0) {
        ls_syslog(LOG_ERR, "%s: statvfs() failed /tmp: %m", __func__);
        myHostPtr->statInfo.maxTmp = 0;
    } else {
        myHostPtr->statInfo.maxTmp =
            (float)fs.f_blocks/((float)(1024 * 1024)/fs.f_bsize);
    }

    fp = popen(MEMMAX, "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: popen() %s failed %m", __func__, MEMMAX);
        maxmem = 0.0;
    }

    fgets(cbuf, BUFSIZ, fp);
    pclose(fp);
    maxmem = atof(cbuf);
    ls_syslog(LOG_DEBUG, "%s: Got maxmem %4.2f", __func__, maxmem);

    fp = popen(SWAPMAX, "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: popen() %s failed %m", __func__, SWAPMAX);
        maxswap = 0;
    }
    fgets(cbuf, BUFSIZ, fp);
    pclose(fp);
    maxswap = atof(cbuf);
    maxswap = maxswap / 1024;
    ls_syslog(LOG_DEBUG, "%s: Got maxswap %4.2f", __func__, maxswap);

    myHostPtr->statInfo.maxMem = maxmem;
    myHostPtr->statInfo.maxSwap = maxswap;

    /* Get the number of disks from which we are
     * going to collect the Kb/s.
     */
#define NUMDISK "/usr/bin/iostat -dsx|/usr/bin/grep -v device|/usr/bin/wc -l"
    fp = popen(NUMDISK, "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: popen() %s failed %m", __func__, NUMDISK);
        return;
    }
    fscanf(fp, "%d", &numdisk);
    pclose(fp);

    /* Start the collector which ahead of time is
     * going to collect load metrics which we are
     * going to read from /var/tmp
     */
    runLoadCollector();
}

/* getHostModel()
 */
char *
getHostModel(void)
{
    static char model[MAXLSFNAMELEN];
    strcpy(model, "solaris");

    return model;
}

/* runLoadCollector()
 * Get the laod information ahead of the next load reading cycle.
 */
void
runLoadCollector(void)
{
    int cc;
    /* Get the disk IO statistics.
     */
    sprintf(cbuf, "\
/usr/bin/iostat -dsx 1 2|/usr/bin/grep -v device|/usr/bin/tail -%d |/usr/bin/awk '{x=x + $4+$5} END {print x}' 1> /var/tmp/lim.iorate 2> /var/tmp/lim.iorate.err", numdisk);
    cc = lim_system(cbuf);
    if (cc < 0) {
        ls_syslog(LOG_ERR, "\
%s: lim_system() failed for %s %m", __func__, cbuf);
    }

#define RUNLOAD "/usr/bin/vmstat -Sq 1 2|/usr/bin/tail -1|/usr/bin/awk '{print $4, $5, ($8+$9), (100-$NF)}' 1> /var/tmp/lim.runload 2> /var/tmp/lim.runload.err"
    cc = lim_system(RUNLOAD);
    if (cc < 0) {
        ls_syslog(LOG_ERR, "\
%s: lim_system() failed for %s %m", __func__, RUNLOAD);
    }
}
