/*
 * Copyright (C) 2011 - 2012 David Bigagli
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

#include <sys/types.h>
#include <sys/wait.h>
#include "lim.h"
#include <math.h>
#include <utmp.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/utsname.h>
#include "../lib/mls.h"
#include <unistd.h>
#include "../lib/lproto.h"


#define  EXP3   0.716531311
#define  EXP4   0.77880078
#define  EXP6   0.846481725
#define  EXP12  0.920044415
#define  EXP180 0.994459848

float k_hz;
static FILE *lim_popen(char **, char *);
static int lim_pclose(FILE *);

pid_t elim_pid = -1;
int defaultRunElim = FALSE;

static void getusr(void);
static float overRide[NBUILTINDEX];
static char * getElimRes (void);
static int saveSBValue (char *, char *);
static int callElim(void);
static int startElim(void);
static void termElim(void);
static int isResourceSharedInAllHosts(char *resName);

int ELIMrestarts = -1;
int ELIMdebug = 0;
int ELIMblocktime = -1;

extern int maxnLbHost;
float initLicFactor(void);
static void setUnkwnValues (void);

extern char *getExtResourcesVal(char *);
extern int  blockSigs_(int, sigset_t*, sigset_t*);
static void unblockSigs_(sigset_t* );
void
satIndex(void)
{
    int   i;

    for (i = 0; i < allInfo.numIndx; i++)
        li[i].satvalue = myHostPtr->busyThreshold[i];

}

void
loadIndex(void)
{
    li[R15S].exchthreshold += 0.05*(myHostPtr->statInfo.maxCpus - 1 );
    li[R1M].exchthreshold += 0.04*(myHostPtr->statInfo.maxCpus - 1 );
    li[R15M].exchthreshold += 0.03*(myHostPtr->statInfo.maxCpus - 1 );
}

static void
smooth(float *val, float instant, float factor)
{
    (*val) = ((*val) * factor) + (instant * (1 - factor));
}

/* idletime()
 * Get the machine tty idle time. Traverse on utmp and
 * check if when it is the last time somebody send a character
 * down the tty. The min of this value is the machine tty idle time.
 */
static float
idletime(void)
{
    struct utmp *u;
    struct stat statbuf;
    static char buf[128];
    int idle;
    int l;
    time_t t;

    setutent();
    t = time(NULL);
    idle = 3600*24*30*60;
    while ((u = getutent())) {

        if (u->ut_user[0] == 0
            || u->ut_type != USER_PROCESS)
            continue;

        sprintf(buf, "/dev/%s", u->ut_line);

        if (stat(buf, &statbuf) < 0) {
            ls_syslog(LOG_ERR, "\
%s: stats() failed %s %s %s %m", __func__, u->ut_user, u->ut_line);
            continue;
        }

        ls_syslog(LOG_DEBUG, "\
%s: %s %s %s %d", __func__, u->ut_user, u->ut_line, buf, t - statbuf.st_atime);

        l = t - statbuf.st_atime;
        if (l <= 0) {
            /* Somebody just send a char down the line
             * the machine is certanly no longer idle.
             */
            idle = 0;
            break;
        }
        if (l < idle)
            idle = l;
    } /* while () */

    endutent();

    return idle;
}

/* readLoad()
 */
void
readLoad(int kernelPerm)
{
    int i, busyBits = 0;
    float  etime;
    float  itime;
    static  int readCount0 = 10000;
    static  int readCount1 = 5;
    static  int readCount2 = 1500;
    static  float avrun15 = 0.0;
    float   ql;
    static  float avrun1m  = 0.0;
    static  float avrun15m = 0.0;
    static  float   smpages;
    static  float   smkbps;
    float   extrafactor;
    float cpu_usage;
    static  float   swap;
    static  float   instant_ut;
    static  int     loginses;

    TIMEIT(0, getusr(), "getusr()");

    if (kernelPerm < 0)
        goto checkOverRide;

    if (++readCount0 < (5.0/sampleIntvl)) {
        goto checkExchange;
    }

    readCount0 = 0;
    ql = 0.0;
    instant_ut = 0.0;
    if (0)
        smooth(0, 0, 0);

#if defined(__linux__)
    if (queueLengthEx(&avrun15, &avrun1m, &avrun15m) < 0) {
        ql = queueLength();
        smooth(&avrun15, ql, EXP3);
        smooth(&avrun1m, ql, EXP12);
        smooth(&avrun15m, ql, EXP180);
    }
#endif

#if defined(__sun__)
    queueLengthEx(&avrun15, &avrun1m, &avrun15m);
#endif

    if (++readCount1 < 3)
        goto checkExchange;

    readCount1 = 0;
    /* Use preprocessor defined macros.
     */
#if defined(__linux__)
    cpuTime(&itime, &etime);
    instant_ut = 1.0 - itime/etime;
    smooth(&cpu_usage, instant_ut, EXP4);
    etime /= k_hz;
    etime = etime/myHostPtr->statInfo.maxCpus;
#endif

#if defined(__sun__)
    cpuTime(&itime, &cpu_usage);
    loginses = getLogin();
#endif

    TIMEIT(0, myHostPtr->loadIndex[IT] = idletime(), "idletime");
    TIMEIT(0, smpages = getpaging(etime), "getpaging");
    TIMEIT(0, smkbps = getIoRate(etime), "getIoRate");
    TIMEIT(0, swap = getswap(), "getswap");
    TIMEIT(0, myHostPtr->loadIndex[TMP] = tmpspace(), "tmpspace");
    TIMEIT(0, myHostPtr->loadIndex[MEM] = realMem(0.0), "realMem");
    /* Start the next load collector.
     */
    runLoadCollector();
checkOverRide:
    if (overRide[UT] < INFINIT_LOAD)
        cpu_usage = overRide[UT];

    if (overRide[PG] < INFINIT_LOAD)
        smpages = overRide[PG];

    if (overRide[IO] < INFINIT_LOAD)
        smkbps = overRide[IO];

    if (overRide[IT] <  INFINIT_LOAD)
        myHostPtr->loadIndex[IT] = overRide[IT];

    if (overRide[SWP] <  INFINIT_LOAD)
        swap = overRide[SWP];

    if (overRide[TMP] < INFINIT_LOAD)
        myHostPtr->loadIndex[TMP] = overRide[TMP];

    if (overRide[R15S] < INFINIT_LOAD)
        avrun15 = overRide[R15S];

    if (overRide[R15S] < INFINIT_LOAD)
        avrun15 = overRide[R15S];

    if (overRide[R1M] < INFINIT_LOAD)
        avrun1m = overRide[R1M];

    if (overRide[R15M] < INFINIT_LOAD)
        avrun15m = overRide[R15M];

    if (overRide[LS] < INFINIT_LOAD)
        loginses = overRide[LS];

    if (overRide[MEM] < INFINIT_LOAD)
        myHostPtr->loadIndex[MEM] = overRide[MEM];

checkExchange:

    if (++readCount2 < (exchIntvl/sampleIntvl))
        return;
    readCount2 = 0;

    extrafactor = 0;
    if (jobxfer) {
        extrafactor = (float)jobxfer/(float)keepTime;
        jobxfer--;
    }

    myHostPtr->loadIndex[R15S] = avrun15  + extraload[R15S] * extrafactor;
    myHostPtr->loadIndex[R1M]  = avrun1m  + extraload[R1M]  * extrafactor;
    myHostPtr->loadIndex[R15M] = avrun15m + extraload[R15M] * extrafactor;

    myHostPtr->loadIndex[UT] = cpu_usage + extraload[UT] * extrafactor;
    if (myHostPtr->loadIndex[UT] > 1.0)
        myHostPtr->loadIndex[UT] = 1.0;
    myHostPtr->loadIndex[PG]  = smpages + extraload[PG] * extrafactor;
    if (myHostPtr->statInfo.nDisks)
        myHostPtr->loadIndex[IO] = smkbps + extraload[IO] * extrafactor;
    else
        myHostPtr->loadIndex[IO] = smkbps;
    myHostPtr->loadIndex[LS]  = loginses + extraload[LS] * extrafactor;
    myHostPtr->loadIndex[IT] += extraload[IT] * extrafactor;
    if (myHostPtr->loadIndex[IT] < 0)
        myHostPtr->loadIndex[IT] = 0;
    myHostPtr->loadIndex[SWP] = swap + extraload[SWP] *extrafactor;
    if (myHostPtr->loadIndex[SWP] < 0)
        myHostPtr->loadIndex[SWP] = 0;
    myHostPtr->loadIndex[TMP] += extraload[TMP]*extrafactor;
    if (myHostPtr->loadIndex[TMP] < 0)
        myHostPtr->loadIndex[TMP] = 0;

    myHostPtr->loadIndex[MEM] += extraload[MEM]*extrafactor;
    if (myHostPtr->loadIndex[MEM] < 0)
        myHostPtr->loadIndex[MEM] = 0;

    for (i = 0; i < allInfo.numIndx; i++) {

        if (i == R15S || i == R1M || i == R15M) {
            li[i].value = normalizeRq(myHostPtr->loadIndex[i],
                                      1,
                                      myHostPtr->statInfo.maxCpus) - 1;
        } else {
            li[i].value = myHostPtr->loadIndex[i];
        }
    }

    for (i = 0; i < allInfo.numIndx; i++) {

        if ((li[i].increasing && fabs(li[i].value - INFINIT_LOAD) < 1.0)
            || (! li[i].increasing
                && fabs(li[i].value + INFINIT_LOAD) < 1.0)) {
            continue;
        }

        if (!THRLDOK(li[i].increasing, li[i].value, li[i].satvalue))  {
            SET_BIT (i + INTEGER_BITS, myHostPtr->status);
            myHostPtr->status[0] |= LIM_BUSY;
        } else
            CLEAR_BIT(i + INTEGER_BITS, myHostPtr->status);
    }

    for (i = 0; i < GET_INTNUM (allInfo.numIndx); i++)
        busyBits += myHostPtr->status[i+1];
    if (!busyBits)
        myHostPtr->status[0] &= ~LIM_BUSY;

    if (LOCK_BY_USER(limLock.on)) {
        if (time(NULL) > limLock.time ) {
            limLock.on &= ~LIM_LOCK_STAT_USER;
            limLock.time = 0;
            mustSendLoad = TRUE;
            myHostPtr->status[0] = myHostPtr->status[0] & ~LIM_LOCKEDU;
        } else {
            myHostPtr->status[0] = myHostPtr->status[0] | LIM_LOCKEDU;
        }
    }

    myHostPtr->loadMask = 0;

    TIMEIT(0, sendLoad(), "sendLoad()");
    runLoadCollector();

    for(i = 0; i < allInfo.numIndx; i++) {

        if (myHostPtr->loadIndex[i] < MIN_FLOAT16 &&
            i < NBUILTINDEX){
            myHostPtr->loadIndex[i] = 0.0;
        }

        if (i == R15S || i == R1M || i == R15M ) {
            float rawql;

            rawql = myHostPtr->loadIndex[i];
            myHostPtr->loadIndex[i]
                = normalizeRq(rawql,
                              (myHostPtr->hModelNo >= 0) ?
                              shortInfo.cpuFactors[myHostPtr->hModelNo] : 1.0,
                              myHostPtr->statInfo.maxCpus);
            myHostPtr->uloadIndex[i] = rawql;
        } else {
            myHostPtr->uloadIndex[i] = myHostPtr->loadIndex[i];
        }
    }

    return;
}

static FILE *
lim_popen(char **argv, char *mode)
{
    static char fname[] = "lim_popen()";
    int p[2], pid, i;

    if (mode[0] != 'r')
        return NULL;

    if (pipe(p) < 0)
        return NULL;

    if ((pid = fork()) == 0) {
        char  *resEnv;
        resEnv = getElimRes();
        if (resEnv != NULL) {
            if (logclass & LC_TRACE)
                ls_syslog (LOG_DEBUG, "lim_popen: LS_ELIM_RESOURCES <%s>", resEnv);
            putEnv ("LS_ELIM_RESOURCES", resEnv);
        }
        close(p[0]);
        dup2(p[1], 1);

        alarm(0);

        for(i = 2; i < sysconf(_SC_OPEN_MAX); i++)
            close(i);
        for (i = 1; i < NSIG; i++)
            Signal_(i, SIG_DFL);

        lsfExecvp(argv[0], argv);
        ls_syslog(LOG_ERR, I18N_FUNC_S_FAIL_M, fname, "execvp", argv[0]);
        exit(127);
    }
    if (pid == -1) {
        close(p[0]);
        close(p[1]);
        return NULL;
    }

    elim_pid = pid;
    close(p[1]);

    return fdopen(p[0], mode);
}

static int
lim_pclose(FILE *ptr)
{
    sigset_t   omask;
    sigset_t   newmask;
    pid_t      child;

    child = elim_pid;
    elim_pid = -1;

    if (ptr)
        fclose(ptr);

    if (child == -1)
        return -1;

    kill(child, SIGTERM);

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    sigaddset(&newmask, SIGQUIT);
    sigaddset(&newmask, SIGHUP);
    sigprocmask(SIG_BLOCK, &newmask, &omask);

    sigprocmask(SIG_SETMASK, &omask, NULL);

    return 0;
}

static int
saveIndx(char *name, float value)
{
    static char **names;
    int indx;
    int i;

    if (!names) {
        if (!(names = calloc(allInfo.numIndx + 1, sizeof(char *)))) {
            ls_syslog(LOG_ERR, "%s: calloc() failed %m", __func__);
            lim_Exit(__func__);
        }
    }
    indx = getResEntry(name);

    if (indx < 0) {

        for(i = NBUILTINDEX; names[i] && i < allInfo.numIndx; i++) {
            if (strcmp(name, names[i]) == 0)
                return 0;
        }

        ls_syslog(LOG_ERR, "\
%s: Unknown index name %s from ELIM", __func__, name);
        if (names[i]) {
            FREEUP(names[i]);
        }
        names[i] = putstr_(name);
        return 0;
    }

    if (allInfo.resTable[indx].valueType != LS_NUMERIC
        || indx >= allInfo.numIndx) {
        return 0;
    }

    if (indx < NBUILTINDEX) {
        if (!names[indx]) {
            names[indx] = allInfo.resTable[indx].name;
            ls_syslog(LOG_WARNING, "\
%s: ELIM over-riding value of index %s", __func__, name);
        }
        overRide[indx] = value;
    } else
        myHostPtr->loadIndex[indx] = value;

    return 0;
}

static int
getSharedResBitPos(char *resName)
{
    struct sharedResourceInstance *tmpSharedRes;
    int bitPos;

    if (resName == NULL)
        return -1;

    for (tmpSharedRes=sharedResourceHead, bitPos=0;
         tmpSharedRes;
         tmpSharedRes=tmpSharedRes->nextPtr, bitPos++ ){
        if (!strcmp(resName,tmpSharedRes->resName)){
            return bitPos;
        }
    }
    return -1;

}

static void
getExtResourcesLoad(void)
{
    int i, bitPos;
    char *resName, *resValue;
    float fValue;

    for (i=0; i < allInfo.nRes; i++) {
        if (allInfo.resTable[i].flags & RESF_DYNAMIC
            && allInfo.resTable[i].flags & RESF_EXTERNAL) {
            resName = allInfo.resTable[i].name;

            if (!defaultRunElim){
                if ((bitPos=getSharedResBitPos(resName)) == -1)
                    continue;
            }
            if ((resValue=getExtResourcesVal(resName)) == NULL)
                continue;

            if (saveSBValue (resName, resValue) == 0)
                continue;
            fValue = atof(resValue);

            saveIndx(resName, fValue);
        }
    }

}

int
isResourceSharedByHost(struct hostNode *host, char * resName)
{
    int i;
    for (i = 0; i < host->numInstances; i++) {
        if (strcmp(host->instances[i]->resName, resName) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

#define timersub(a,b,result)                                    \
    do {                                                        \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;           \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;        \
        if ((result)->tv_usec < 0) {                            \
            --(result)->tv_sec;                                 \
            (result)->tv_usec += 1000000;                       \
        }                                                       \
    } while (0)

#define ELIMNAME "elim"
#define MAXEXTRESLEN 4096
static void
getusr(void)
{
    static char fname[]="getusr";
    static FILE *fp;
    static time_t lastStart;
    static char first = TRUE;
    int i, nfds;
    struct timeval timeout;
    static char *masterResStr = "Y";
    static char *resStr = NULL;
    int size;
    struct sharedResourceInstance *tmpSharedRes;
    char * hostNamePtr;
    struct timeval t, expire;
    struct timeval time0 = {0,0};
    int bw, scc;

    if (first) {
        for(i = 0; i < NBUILTINDEX; i++)
            overRide[i] = INFINIT_LOAD;
        first = FALSE;
    }

    if (!callElim()) {
        return;
    }

    getExtResourcesLoad();

    if (!startElim()) {
        return;
    }

    if ((elim_pid < 0) && (time(0) - lastStart > 90)) {

        if (ELIMrestarts < 0 || ELIMrestarts > 0) {

            if (ELIMrestarts > 0) {
                ELIMrestarts--;
            }

            if (!myClusterPtr->eLimArgv) {
                char *path = malloc(strlen(limParams[LSF_SERVERDIR].paramValue) +
                                    strlen(ELIMNAME) + 8);
                if (!path) {
                    ls_syslog(LOG_ERR, I18N_FUNC_FAIL, fname, "malloc");
                    setUnkwnValues();
                    return;
                }
                strcpy(path, limParams[LSF_SERVERDIR].paramValue);
                strcat(path, "/");
                strcat(path, ELIMNAME);

                if (logclass & LC_EXEC) {
                    ls_syslog(LOG_DEBUG, "%s : the elim's name is <%s>\n",
                              fname, path);
                }


                myClusterPtr->eLimArgv = parseCommandArgs(
                    path, myClusterPtr->eLimArgs);
                if (!myClusterPtr->eLimArgv) {
                    ls_syslog(LOG_ERR, I18N_FUNC_FAIL, fname, "malloc");
                    setUnkwnValues();
                    return;
                }
            }

            if (fp) {
                fclose(fp);
                fp = NULL;
            }

            lastStart = time(0);

            if (masterMe)
                putEnv("LSF_MASTER", masterResStr);
            else
                putEnv("LSF_MASTER", "N");

            if (resStr != NULL) free(resStr);

            hostNamePtr = ls_getmyhostname();

            size = 0;
            for (tmpSharedRes=sharedResourceHead;
                 tmpSharedRes ;
                 tmpSharedRes=tmpSharedRes->nextPtr ){
                size += strlen(tmpSharedRes->resName) + sizeof(char) ;
            }
            for (i = NBUILTINDEX; i < allInfo.nRes; i++) {
                if (allInfo.resTable[i].flags & RESF_EXTERNAL)
                    continue;
                if ((allInfo.resTable[i].flags & RESF_DYNAMIC)
                    && !(allInfo.resTable[i].flags & RESF_BUILTIN)){

                    size += strlen(allInfo.resTable[i].name) + sizeof(char);
                }
            }
            resStr = calloc(size + 1, sizeof(char)) ;
            if (!resStr) {
                ls_syslog(LOG_ERR, "%s: calloc() failed %m", __func__);
                setUnkwnValues();
                return;
            }
            resStr[0] = '\0' ;

            for (i = NBUILTINDEX; i < allInfo.nRes; i++) {
                if (allInfo.resTable[i].flags & RESF_EXTERNAL)
                    continue;
                if ((allInfo.resTable[i].flags & RESF_DYNAMIC)
                    && !(allInfo.resTable[i].flags & RESF_BUILTIN)){

                    if ((allInfo.resTable[i].flags & RESF_SHARED)
                        && (!masterMe)
                        && (isResourceSharedInAllHosts(allInfo.resTable[i].name))){
                        continue;
                    }

                    if( (allInfo.resTable[i].flags & RESF_SHARED)
                        && (!isResourceSharedByHost(myHostPtr, allInfo.resTable[i].name)) )
                        continue;

                    if (resStr[0] == '\0')
                        sprintf(resStr, "%s", allInfo.resTable[i].name);
                    else {
                        sprintf(resStr,"%s %s", resStr, allInfo.resTable[i].name);
                    }
                }
            }
            putEnv ("LSF_RESOURCES",resStr);

            if ((fp = lim_popen(myClusterPtr->eLimArgv, "r")) == NULL) {
                ls_syslog(LOG_ERR, "\
%s: lim_popen() failed %s", __func__, myClusterPtr->eLimArgv[0]);
                setUnkwnValues();

                return;
            }
            ls_syslog(LOG_INFO, (_i18n_msg_get(ls_catd , NL_SETN, 5930,
                                               "%s: Started ELIM %s pid %d")), fname, /* catgets 5930 */
                      myClusterPtr->eLimArgv[0], (int)elim_pid);
            mustSendLoad = TRUE ;

        }

    }

    if (elim_pid < 0) {
        setUnkwnValues();
        if (fp) {
            fclose(fp);
            fp = NULL;
        }
        return;
    }

    timeout.tv_sec  = 0;
    timeout.tv_usec = 5;

    if ((nfds = rd_select_(fileno(fp), &timeout)) < 0) {
        ls_syslog(LOG_ERR, "%s: rd_select() failed %m", __func__);
        lim_pclose(fp);
        fp = NULL;

        return;
    }

    if (nfds == 1) {
        int numIndx, cc;
        char name[MAXLSFNAMELEN], valueString[MAXEXTRESLEN];
        float value;
        sigset_t  oldMask;
        sigset_t  newMask;

        static char *fromELIM = NULL;
        static int sizeOfFromELIM = MAXLINELEN;
        char *elimPos;
        int spaceLeft, spaceRequired;

        if( !fromELIM ) {
            fromELIM = malloc(sizeOfFromELIM);
            if (!fromELIM) {
                ls_syslog(LOG_ERR, I18N_FUNC_FAIL, fname, "malloc");

                ls_syslog(LOG_ERR, "\
%s: Received from ELIM: <out of memory to record contents>", __func__);

                setUnkwnValues();
                lim_pclose(fp);
                fp = NULL;
                return;
            }
        }

        elimPos = fromELIM;
        *elimPos = '\0';

        blockSigs_(0, &newMask, &oldMask);

        if (logclass & LC_TRACE) {
            ls_syslog(LOG_DEBUG,"\
%s: Signal mask has been changed, all are signals blocked now", fname);
        }

        if (ELIMblocktime >= 0) {
            io_nonblock_(fileno(fp));
        }

        cc = fscanf(fp,"%d",&numIndx);
        if ( cc != 1) {
            ls_syslog(LOG_ERR, "\
%s: Protocol error numIndx not read (cc=%d): %m", __func__, cc);
            lim_pclose(fp);
            fp = NULL;
            unblockSigs_(&oldMask);

            return;
        }


        bw = sprintf (elimPos, "%d ", numIndx);
        elimPos += bw;

        if ( numIndx < 0) {
            ls_syslog(LOG_ERR, "%\
s: Protocol error numIndx %d", __func__, numIndx);
            setUnkwnValues();
            lim_pclose(fp);
            fp = NULL;
            unblockSigs_(&oldMask);
            return;
        }

        if (ELIMblocktime >= 0) {
            gettimeofday(&t, NULL);
            expire.tv_sec = t.tv_sec + ELIMblocktime;
            expire.tv_usec = t.tv_usec;
        }

        i = numIndx * 2;
        while(i) {

            if( i%2 ) {
                cc = fscanf (fp, "%4096s", valueString);
                valueString[MAXEXTRESLEN-1] = '\0';
            } else {
                cc = fscanf (fp, "%40s", name);
                name[MAXLSFNAMELEN-1] = '\0';
            }

            if (cc == -1) {
                int scanerrno = errno;
                if (scanerrno == EAGAIN) {


                    gettimeofday(&t, NULL);
                    timersub(&expire, &t, &timeout);
                    if (timercmp(&timeout, &time0, <)) {
                        timerclear(&timeout);
                    }
                    scc = rd_select_(fileno(fp), &timeout);
                }

                if (scanerrno != EAGAIN || scc <= 0) {

                    ls_syslog(LOG_ERR, "\
%s: Protocol error, expected %d more tokens (cc=%d): %m",
                              __func__, i, cc);

                    ls_syslog(LOG_ERR, "\
%s: Received from ELIM: %s.", __func__, fromELIM);

                    setUnkwnValues();
                    lim_pclose(fp);
                    fp = NULL;
                    unblockSigs_(&oldMask);
                    return;
                }
                continue;
            }

            spaceLeft = sizeOfFromELIM - (elimPos - fromELIM)-1;
            spaceRequired = strlen((i%2)? valueString: name)+1;

            if( spaceLeft < spaceRequired ) {

                char *oldFromElim = fromELIM;
                int oldSizeOfFromELIM = sizeOfFromELIM;


                sizeOfFromELIM += (spaceRequired - spaceLeft);

                fromELIM = realloc(fromELIM, sizeOfFromELIM);
                if (!fromELIM) {
                    ls_syslog(LOG_ERR, I18N_FUNC_FAIL, fname, "malloc");

                    ls_syslog(LOG_ERR, "\
%s: Received from ELIM: <out of memory to record contents>", fname);

                    sizeOfFromELIM = oldSizeOfFromELIM;
                    fromELIM = oldFromElim;

                    setUnkwnValues();
                    lim_pclose(fp);
                    fp = NULL;
                    unblockSigs_(&oldMask);
                    return;
                }
                elimPos = fromELIM + strlen(fromELIM);
            }

            bw = sprintf (elimPos, "%s ", (i%2)? valueString: name);
            elimPos += bw;

            if (i%2) {

                if (saveSBValue (name, valueString) == 0) {
                    i--;
                    continue;
                }


                value = atof (valueString);
                saveIndx(name, value);
            }
            i--;
        }

        unblockSigs_(&oldMask);

        if (ELIMdebug) {
            ls_syslog(LOG_WARNING, "%s: ELIM: %s.", __func__, fromELIM);
        }
    }
}

void
unblockSigs_(sigset_t*  mask)
{
    static char fname[] = "unblockSigs_()";

    sigprocmask(SIG_SETMASK, mask, NULL);

    if (logclass & LC_TRACE) {
        ls_syslog(LOG_DEBUG,"\
%s: The original signal mask has been restored", fname);
    }

}

static void
setUnkwnValues (void)
{
    int i;

    for(i=0; i < allInfo.numUsrIndx; i++)
        myHostPtr->loadIndex[NBUILTINDEX + i] = INFINIT_LOAD;
    for(i=0; i < NBUILTINDEX; i++)
        overRide[i] = INFINIT_LOAD;

    for (i = 0; i < myHostPtr->numInstances; i++) {
        if (myHostPtr->instances[i]->updateTime == 0
            || myHostPtr->instances[i]->updHost == NULL)

            continue;
        if (myHostPtr->instances[i]->updHost == myHostPtr) {
            strcpy (myHostPtr->instances[i]->value, "-");
            myHostPtr->instances[i]->updHost = NULL;
            myHostPtr->instances[i]->updateTime = 0;
        }
    }
}

static int
saveSBValue (char *name, char *value)
{
    static char fname[] = "saveSBValue()";
    int i, indx, j, myHostNo = -1, updHostNo = -1;
    char *temp = NULL;
    time_t  currentTime = 0;

    if ((indx = getResEntry(name)) < 0)
        return (-1);

    if (!(allInfo.resTable[indx].flags & RESF_DYNAMIC))
        return -1 ;

    if (allInfo.resTable[indx].valueType == LS_NUMERIC){
        if (!isanumber_(value)){
            return -1;
        }
    }

    if (myHostPtr->numInstances <= 0)
        return (-1);

    for (i = 0; i < myHostPtr->numInstances; i++) {
        if (strcmp (myHostPtr->instances[i]->resName, name))
            continue;
        if (currentTime == 0)
            currentTime = time(0);
        if (masterMe) {

            for (j = 0; j < myHostPtr->instances[i]->nHosts; j++) {
                if (myHostPtr->instances[i]->updHost
                    && (myHostPtr->instances[i]->updHost
                        == myHostPtr->instances[i]->hosts[j]))
                    updHostNo = j;
                if (myHostPtr->instances[i]->hosts[j] == myHostPtr)
                    myHostNo = j;
                if (myHostNo >= 0
                    && (updHostNo >= 0
                        || myHostPtr->instances[i]->updHost == NULL))
                    break;
            }
            if (updHostNo >= 0
                && (myHostNo < 0
                    || ((updHostNo < myHostNo)
                        && strcmp (myHostPtr->instances[i]->value, "-"))))
                return(0);
        }

        if ((temp = (char *)malloc (strlen(value) + 1)) == NULL) {
            ls_syslog(LOG_ERR, I18N_FUNC_FAIL_M, fname, "malloc");
            FREEUP (temp);
            return (0);
        }
        strcpy (temp, value);
        FREEUP (myHostPtr->instances[i]->value);
        myHostPtr->instances[i]->value = temp;
        myHostPtr->instances[i]->updateTime = currentTime;
        myHostPtr->instances[i]->updHost = myHostPtr;
        if (logclass & LC_LOADINDX)
            ls_syslog(LOG_DEBUG3, "saveSBValue: i = %d, resName=%s, value=%s, newValue=%s, updHost=%s", i, myHostPtr->instances[i]->resName, myHostPtr->instances[i]->value, temp, myHostPtr->instances[i]->updHost->hostName);
        return (0);
    }
    return (-1);

}

void
initConfInfo(void)
{
    char *sp;

    if((sp = getenv("LSF_NCPUS")) != NULL)
        myHostPtr->statInfo.maxCpus = atoi(sp);
    else
        myHostPtr->statInfo.maxCpus = numCpus();

    if (myHostPtr->statInfo.maxCpus <= 0) {
        ls_syslog(LOG_ERR, "\
%s: Invalid num of CPUs %d. Default to 1", __func__, myHostPtr->statInfo.maxCpus);
        myHostPtr->statInfo.maxCpus = 1;
    }

    myHostPtr->statInfo.portno = lim_tcp_port;
    myHostPtr->statInfo.hostNo = myHostPtr->hostNo;
    myHostPtr->infoValid = TRUE;

}

static char *
getElimRes(void)
{

    int i;
    int numEnv;
    int resNo;
    char *resNameString;

    resNameString = malloc((allInfo.nRes) * MAXLSFNAMELEN);
    if (resNameString == NULL) {
        ls_syslog(LOG_ERR, "\
%s: failed allocate %d bytes %m", __func__, allInfo.nRes * MAXLSFNAMELEN);
        lim_Exit("getElimRes");
    }

    numEnv = 0;
    resNameString[0] = '\0';
    for (i = 0; i < allInfo.numIndx; i++) {
        if (allInfo.resTable[i].flags & RESF_EXTERNAL)
            continue;
        if (numEnv != 0)
            strcat (resNameString, " ");
        strcat(resNameString, allInfo.resTable[i].name);
        numEnv++;
    }

    for (i = 0; i < myHostPtr->numInstances; i++) {
        resNo = resNameDefined (myHostPtr->instances[i]->resName);
        if (allInfo.resTable[resNo].flags & RESF_EXTERNAL)
            continue;
        if (allInfo.resTable[resNo].interval > 0) {
            if (numEnv != 0)
                strcat(resNameString, " ");
            strcat (resNameString, myHostPtr->instances[i]->resName);
            numEnv++;
        }
    }

    if (numEnv == 0)
        return NULL;

    return resNameString;
}

static int
callElim(void)
{
    static int runit = FALSE;
    static int lastTimeMasterMe = FALSE;

    if (masterMe && !lastTimeMasterMe) {
        lastTimeMasterMe = TRUE ;
        if (runit){
            termElim() ;
            if (myHostPtr->callElim || defaultRunElim)
                return TRUE ;

            runit = FALSE ;
            return FALSE ;
        }
    }

    if (!masterMe && lastTimeMasterMe){
        lastTimeMasterMe = FALSE ;

        if (runit){
            termElim() ;

            if (myHostPtr->callElim || defaultRunElim)
                return TRUE ;

            runit = FALSE ;
            return FALSE ;
        }
    }

    if (masterMe)
        lastTimeMasterMe = TRUE ;
    else
        lastTimeMasterMe = FALSE ;

    if (runit) {
        if (!myHostPtr->callElim && !defaultRunElim){
            termElim();
            runit = FALSE ;
            return FALSE ;
        }
    }

    if (defaultRunElim) {
        runit = TRUE;
        return TRUE;
    }

    if (myHostPtr->callElim) {
        runit = TRUE ;
        return TRUE ;
    }
    runit = FALSE ;
    return FALSE ;
}

static int
startElim(void)
{
    static int notFirst=FALSE, startElim=FALSE;
    int i;

    if (!notFirst){

        for (i = 0; i < allInfo.nRes; i++) {
            if (allInfo.resTable[i].flags & RESF_EXTERNAL)
                continue;
            if ((allInfo.resTable[i].flags & RESF_DYNAMIC)
                && !(allInfo.resTable[i].flags & RESF_BUILTIN)){
                startElim = TRUE;
                break;
            }
        }
        notFirst = TRUE;
    }

    return startElim;
}


static void
termElim(void)
{
    if (elim_pid == -1)
        return;

    kill(elim_pid, SIGTERM);
    elim_pid = -1;

}

static int
isResourceSharedInAllHosts(char *resName)
{
    struct sharedResourceInstance *tmpSharedRes;

    for (tmpSharedRes=sharedResourceHead;
         tmpSharedRes ;
         tmpSharedRes=tmpSharedRes->nextPtr ){

        if (strcmp(tmpSharedRes->resName, resName)) {
            continue;
        }
        if (tmpSharedRes->nHosts == myClusterPtr->numHosts) {
            return(1);
        }
    }

    return(0);

}

