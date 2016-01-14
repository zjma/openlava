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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "lim.h"

static char buf[BUFSIZ];
static unsigned long long int mem;
static unsigned long long int memfree;
static unsigned long long int memcache;
static unsigned long long int maxmem;
static unsigned long long int swap;
static unsigned long long int maxswap;
static int numcpus;

static int getmeminfo(void);
static char *get_host_model(void);
static int get_core_count(void);

/* numCPUs()
 */
int
numCPUs(void)
{
    FILE *fp;
    int numcores;

    fp = fopen("/proc/cpuinfo","r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: fopen() failed on proc/cpuinfo: %m", __func__);
        ls_syslog(LOG_ERR, "%s: assuming one CPU only", __func__);
        numcpus = 1;
        return numcpus;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        if (strncmp(buf, "processor", sizeof("processor") - 1) == 0) {
            ++numcpus;
        }
    }
    fclose(fp);

    /* Some customers want to use physical cores ignoring
     * hyperthreads.
     */
    if (limParams[LIM_DEFINE_NCPUS].paramValue
	&& strcasecmp(limParams[LIM_DEFINE_NCPUS].paramValue, "cores") == 0) {
	numcores = get_core_count();
	if (numcpus > numcores) {
	    numcpus = numcores;
	}
    }

    return numcpus;
}

/* queuelength()
 */
int
queuelength(float *r15s, float *r1m, float *r15m)
{
    FILE *fp;
    int cc;

    fp = fopen("/proc/loadavg", "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: fopen() /proc/loadavg failed", __func__);
        return -1;
    }

    cc = fscanf(fp, "%f %f %f", r15s, r1m, r15m);
    if (cc != 3) {
        ls_syslog(LOG_ERR, "\
%s: failed fcanf()/proc/loadavg %m", __func__);
        return -1;
    }
    fclose(fp);

    ls_syslog(LOG_DEBUG, "\
%s: Got run queue length r15s %4.2f r1m %4.2f r15m %4.2f",
              __func__, *r15s, *r1m, *r15m);

    return 0;
}

/* cputime()
 */
float
cputime(void)
{
    FILE *fp;
    float ut;
    unsigned long long int user;
    unsigned long long int nice;
    unsigned long long int system;
    time_t t;
    uint32_t dt;
    static float before;
    static time_t tb;
    static int jifs;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: open() /proc/stat failed: %m:", __func__);
        return 0.0;
    }

    fscanf(fp,"%*s %llu %llu %llu", &user, &nice, &system);
    fclose(fp);

    if (before == 0) {
        before = user + nice + system;
        tb = time(NULL);
        jifs = sysconf(_SC_CLK_TCK);
        /* Get runtime memory info since at it.
         */
        getmeminfo();
        return 0.0;
    }

    t = time(NULL);
    ut = (user + nice + system) - before;
    /* The first line of the /proc/stat has the
     * global counters, however it seems we do have to
     * normalize them respect to the total number of
     * cpus.
     */
    dt = (t - tb) * jifs * numcpus;

    tb = t;
    before = user + nice + system;

    ls_syslog(LOG_DEBUG, "\
%s: Got cpu ut %4.2f", __func__, ut/(float)dt);

    /* Get runtime memory info since at it.
     */
    getmeminfo();

    return ut/(float)dt;
}

unsigned long long int
freemem(void)
{
    /* These values are in kilobytes.
     */
    return mem;
}

unsigned long long int
freetmp(void)
{
    struct statvfs fs;
    int tmp;

    if (statvfs("/tmp", &fs) < 0) {
        ls_syslog(LOG_ERR, "%s: statfs() /tmp failed: %m", __func__);
        return 0;
    }

    /* Convert the bytes in MB.
     */
    tmp = (fs.f_bavail * fs.f_bsize)/(1024 * 1024);

    return tmp;
}

unsigned long long int
freeswap(void)
{
    /* These values are in kilobytes.
     */
    return swap;
}

float
paging(void)
{
    FILE *fp;
    char name[64];
    unsigned long long int pagein;
    unsigned long long int pageout;
    static unsigned long long int ppagein;
    static unsigned long long int ppageout;
    unsigned long long int v;
    float curr;
    static time_t pt;
    time_t t;

    pagein = pageout = 0;
    if ((fp = fopen("/proc/vmstat", "r")) == NULL) {
        ls_syslog(LOG_ERR, "\
%s: fopen() failed /proc/vmstat: %m", __func__);
        return -1;
    }

    t = time(NULL);
    while (fgets(buf, sizeof(buf), fp)) {

        if (sscanf(buf, "%s %llu", name, &v) != 2)
            continue;
        /* Paging is a natural system activity
         * so we measure the load in terms
         * of swapping.
         * Instead of using pgpin and pgpout
         * we consider pswpin pswpout instead.
         */
        if (strcmp(name, "pswpin") == 0)
            pagein = v;
        if (strcmp(name, "pswpout") == 0)
            pageout = v;

        /* optimize...
         */
        if (pagein != 0
            && pageout != 0)
            break;
    }
    fclose(fp);

    if (ppagein == 0) {
        ppagein = pagein;
        ppageout = pageout;
        pt = t;
        return 0;
    }
    /* Normalize the paging to the time interval and
     * the number of cpus.
     */
    curr = (pagein - ppagein) + (pageout - ppageout);
    curr = curr/((t - pt) * numcpus);
    /* Save state...
     */
    pt = t;
    ppagein = pagein;
    ppageout = pageout;

    ls_syslog(LOG_DEBUG, "\
%s: Got paging rate %4.2f in %llu out %llu", __func__,
              curr, (pagein - ppagein), (pageout - ppageout));

    return curr;
}

float
iorate(void)
{
    return 0.0;
}

/* getmeminfo()
 *
 *   http://git.kernel.org/cgit/linux/kernel/git/torvalds/
 *    linux.git/tree/Documentation/filesystems/proc.txt?id=HEAD#l451
 */
static int
getmeminfo(void)
{
    FILE *fp;
    unsigned long long int v;
    char name[64];

    if ((fp = fopen("/proc/meminfo", "r")) == NULL) {
        ls_syslog(LOG_ERR, "\
%s: open() failed /proc/meminfo: %m", __func__);
        mem = swap = 0.0;
        return -1;
    }

    while (fgets(buf, sizeof(buf), fp)) {

        if (sscanf(buf, "%s %llu", name, &v) != 2)
            continue;
        /* These values are in kilobytes.
         */
        if (strcmp(name, "MemTotal:") == 0)
            maxmem = v;
        if (strcmp(name, "MemFree:") == 0)
            memfree = v;
        if (strcmp(name, "SwapTotal:") == 0)
            maxswap = v;
        if (strcmp(name, "SwapFree:") == 0)
            swap = v;
        if (strcmp(name, "Cached:") == 0)
            memcache = v;
    }
    mem = memfree + memcache;
    fclose(fp);

    ls_syslog(LOG_DEBUG, "\
%s: Got maxmem %llu freemem %llu maxswap %llu freeswap %llu.",
              __func__, maxmem, mem, maxswap, swap);
    return 0;
}

void
initReadLoad(int checkMode)
{
    struct statvfs fs;

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

    if (getmeminfo() == -1)
        return;

    myHostPtr->statInfo.maxMem = maxmem / 1024;
    myHostPtr->statInfo.maxSwap = maxswap / 1024;
}

char *
getHostModel(void)
{
    static char model[MAXLSFNAMELEN];
    char buf[128], b1[128], b2[128];
    int pos = 0;
    int bmips = 0;
    FILE* fp;

    model[pos] = '\0';
    b1[0] = '\0';
    b2[0] = '\0';

    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
        return model;

    while (fgets(buf, sizeof(buf) - 1, fp)) {

        if (strncasecmp(buf, "cpu\t", 4) == 0
            || strncasecmp(buf, "cpu family", 10) == 0) {
            char *p = strchr(buf, ':');
            if (p)
                strcpy(b1, stripIllegalChars(p + 2));
        }
        if (strstr(buf, "model") != 0) {
            char *p = strchr(buf, ':');
            if (p)
                strcpy(b2, stripIllegalChars(p + 2));
        }
        if (strncasecmp(buf, "bogomips", 8) == 0) {
            char *p = strchr(buf, ':');
            if (p)
                bmips = atoi(p + 2);
        }
    }

    fclose(fp);

    if (!b1[0])
        return model;

    if (isdigit(b1[0]))
        model[pos++] = 'x';

    strncpy(&model[pos], b1, MAXLSFNAMELEN - 15);
    model[MAXLSFNAMELEN - 15] = '\0';
    pos = strlen(model);
    if (bmips) {
        pos += sprintf(&model[pos], "_%d", bmips);
        if (b2[0]) {
            model[pos++] = '_';
            strncpy(&model[pos], b2, MAXLSFNAMELEN - pos - 1);
        }
        model[MAXLSFNAMELEN - 1] = '\0';
    }

    return model;
}

/* get_cpu_info()
 */
int
get_cpu_info(struct cpu_info *cpu)
{
    char *cmd = "/usr/bin/lscpu";
    FILE *fp;
    struct timeval t;
    char tmp[256];

    t.tv_sec = 5;
    t.tv_usec = 0;

    fp = ls_popen(cmd, "r", &t);
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: Ohmygosh ls_peopen() failed, no cpu info: %M", __func__);
        return -1;
    }

    while ((fgets(buf, sizeof(buf), fp))) {
        /* Determine architecture
         */
        if (strstr(buf, "Architecture:")) {
            sscanf(buf, "%*s%s", tmp);
            cpu->arch = strdup(tmp);
            continue;
        }
        /* Find sockets
         */
        if (strstr(buf, "Socket(s):")) {
            sscanf(buf, "%*s%d", &cpu->sockets);
            continue;
        }
        /* Find cores per socket
         */
        if (strstr(buf, "Core(s) per socket:")) {
            sscanf(buf, "%*s%*s%*s%d", &cpu->cores);
            continue;
        }
        /* Find thread per core
         */
        if (strstr(buf, "Thread(s) per core:")) {
            sscanf(buf, "%*s%*s%*s%d", &cpu->threads);
            continue;
        }
        /* Find cpus logical and physical
         */
        if (strstr(buf, "CPU(s):")) {
            sscanf(buf, "%*s%d", &cpu->cpus);
            continue;
        }
        /* Find bogomips
         */
        if (strstr(buf, "BogoMIPS:")) {
            sscanf(buf, "%*s%f", &cpu->bogo_mips);
            continue;
        }
        /* Find NUMA nodes
         */
        if (strstr(buf, "NUMA node(s):")) {
            sscanf(buf, "%*s%*s%d", &cpu->numa_nodes);
            continue;
        }
    }

    ls_pclose(fp);

    cpu->model = get_host_model();
    if (cpu->model == NULL) {
        ls_syslog(LOG_ERR, "\
%s: failed to read cpu model", __func__);
        return -1;
    }

    return 0;
}

/* get_host_model()
 */
static char *
get_host_model(void)
{
    FILE *fp;
    char *p;
    char *p2;

    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: fopen() failed on proc/cpuinfo: %m cannot determine host model",
                  __func__);
        return NULL;
    }

    while ((fgets(buf, sizeof(buf), fp))) {
        if ((p = strstr(buf, "model name"))) {
            p = strchr(buf, ':');
            /* skip : and leading space
             */
            p = p + 2;
            p2 = strdup(p);
            p = strchr(p2, '\n');
            *p = 0;
            fclose(fp);
            return p2;
        }
    }

    fclose(fp);
    return NULL;
}

/* get_core_count()
 */
static int
get_core_count(void)
{
    char *cmd;
    FILE *fp;
    struct stat sbuf;
    int numcores;

    cmd = "\
/usr/bin/lscpu --parse=core |/bin/grep -v '#'|/bin/sort -n |/usr/bin/uniq|/usr/bin/wc -l";

    if (stat("/usr/bin/lscpu", &sbuf) < 0) {
	ls_syslog(LOG_ERR, "\
%s: stat(/usr/bin/lscpus) failed: %m use cpus from /proc/cpuinfo", __func__);
	return 0;
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: Ohmygosh ls_peopen() failed, no cpu info: %M", __func__);
        return 0;
    }

    fscanf(fp, "%d", &numcores);
    pclose(fp);

    return numcores;
}
