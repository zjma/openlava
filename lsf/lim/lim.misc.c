/*
 * Copyright (C) 2011-2015 David Bigagli
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

static struct hostNode *findHNbyAddr(in_addr_t);
static hTab *loadEvents(void);

void
lim_Exit(const char *fname)
{
    ls_syslog(LOG_ERR, "\
%s: Above fatal error(s) found.", fname);
    exit(EXIT_FATAL_ERROR);
}

int
equivHostAddr(struct hostNode *hPtr, u_int from)
{
    int i;

    for (i = 0; i < hPtr->naddr; i++) {

        if (hPtr->addr[i] == from)
            return TRUE;
    }

    return FALSE;
}

struct hostNode *
findHost(char *hostName)
{
    struct hostNode *hPtr;

    if (hostName == NULL)
	return NULL;

    hPtr = findHostbyList(myClusterPtr->hostList, hostName);
    if (hPtr)
        return hPtr;

    hPtr = findHostbyList(myClusterPtr->clientList, hostName);
    if (hPtr)
        return hPtr;

    /* Host unknow let's check is we allow
     * slaves only
     */
    if (limParams[LIM_ACCEPT_FLOAT_CLIENT].paramValue)
       return myHostPtr;

    return NULL;

}

struct hostNode *
findHostbyList(struct hostNode *hList, char *hostName)
{
    struct hostNode *hPtr;

    for (hPtr = hList; hPtr; hPtr = hPtr->nextPtr)
        if (equalHost_(hPtr->hostName, hostName))
            return hPtr;

    return NULL;
}

struct hostNode *
findHostbyNo(struct hostNode *hList, int hostNo)
{
    struct hostNode *hPtr;

    for (hPtr = hList; hPtr; hPtr = hPtr->nextPtr)
        if (hPtr->hostNo == hostNo)
            return hPtr;

    return NULL;
}

struct hostNode *
findHostbyAddr(struct sockaddr_in *from,
               char *fname)
{
    struct hostNode *hPtr;
    struct hostent *hp;
    in_addr_t *tPtr;

    if (from->sin_addr.s_addr == ntohl(LOOP_ADDR))
        return myHostPtr;

    hPtr = findHNbyAddr(from->sin_addr.s_addr);
    if (hPtr)
        return hPtr;

    hp = Gethostbyaddr_(&from->sin_addr.s_addr,
                        sizeof(in_addr_t),
                        AF_INET);
    if (hp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: gethostaddr() failed %s cannot by resolved by %s",
                  __func__, sockAdd2Str_(from),
                  myHostPtr->hostName);
        return NULL;
    }

    hPtr = findHNbyAddr(*(in_addr_t *)hp->h_addr_list[0]);
    if (hPtr == NULL) {
        /* Complain only if the runtime host
         * operations are not allowed. Otherwise
         * this can be a migrant host asking
         * for some TCP operation, so just return
         * NULL and the caller will know what to do.
         */
        if (limParams[LIM_NO_MIGRANT_HOSTS].paramValue)
            ls_syslog(LOG_ERR, "\
%s: Host %s (hp=%s/%s) is unknown by configuration; all hosts used by openlava must have unique official names", fname, sockAdd2Str_(from),
                      hp->h_name,
                      inet_ntoa(*((struct in_addr *)hp->h_addr_list[0])));
        return NULL;
    }

    tPtr = realloc(hPtr->addr, (hPtr->naddr + 1) * sizeof(in_addr_t));
    if (tPtr == NULL)
        return hPtr;

    hPtr->addr = tPtr;
    hPtr->addr[hPtr->naddr] = from->sin_addr.s_addr;
    hPtr->naddr++;

    return hPtr;
}


static struct hostNode *
findHNbyAddr(in_addr_t from)
{
    struct clusterNode *clPtr;
    struct hostNode *hPtr;

    clPtr = myClusterPtr;
    for (hPtr = clPtr->hostList; hPtr; hPtr = hPtr->nextPtr) {
        if (equivHostAddr(hPtr, from))
            return hPtr;
    }

    for (hPtr = clPtr->clientList; hPtr; hPtr = hPtr->nextPtr) {
        if (equivHostAddr(hPtr, from))
            return hPtr;
    }

    return NULL;
}

struct hostNode *
rmHost(struct hostNode *r)
{
    struct hostNode *hPtr0;
    struct hostNode *hPtr;

    hPtr0 = NULL;
    hPtr = myClusterPtr->hostList;
    if (hPtr == r) {
        myClusterPtr->hostList = hPtr->nextPtr;
        return r;
    }

    while (hPtr) {
        if (hPtr == r) {
            hPtr0->nextPtr = hPtr->nextPtr;
            return r;
        }
        hPtr0 = hPtr;
        hPtr = hPtr->nextPtr;
    }

    return NULL;
}

bool_t
findHostInCluster(char *hostname)
{

    if (strcmp(myClusterPtr->clName, hostname) == 0)
        return TRUE;

    if (findHostbyList(myClusterPtr->hostList, hostname) != NULL
        || findHostbyList(myClusterPtr->clientList, hostname) !=NULL)
        return TRUE;

    return FALSE;
}


int
definedSharedResource(struct hostNode *host, struct lsInfo *allInfo)
{
    int i, j;
    char *resName;

    for (i = 0; i < host->numInstances; i++) {
        resName = host->instances[i]->resName;
        for (j = 0; j < allInfo->nRes; j++) {
            if (strcmp(resName, allInfo->resTable[j].name) == 0) {
                if (allInfo->resTable[j].flags &  RESF_SHARED)
                    return TRUE;
            }
        }
    }
    return FALSE;
}

struct shortLsInfo *
shortLsInfoDup(struct shortLsInfo *src)
{
    char                   *memp;
    char                   *currp;
    int                    i;
    struct shortLsInfo     *shortLInfo;

    if (src->nRes == 0 && src->nTypes == 0 && src->nModels == 0)
        return NULL;

    shortLInfo = calloc(1, sizeof(struct shortLsInfo));
    if (shortLInfo == NULL)
        return NULL;

    shortLInfo->nRes = src->nRes;
    shortLInfo->nTypes = src->nTypes;
    shortLInfo->nModels = src->nModels;


    memp = malloc((src->nRes + src->nTypes + src->nModels) * MAXLSFNAMELEN +
                  src->nRes * sizeof (char *));

    if (!memp) {
        FREEUP(shortLInfo);
        return NULL;
    }

    currp = memp;
    shortLInfo->resName = (char **)currp;
    currp += shortLInfo->nRes * sizeof (char *);
    for (i = 0; i < src->nRes; i++, currp += MAXLSFNAMELEN)
        shortLInfo->resName[i] = currp;
    for (i = 0; i < src->nTypes; i++, currp += MAXLSFNAMELEN)
        shortLInfo->hostTypes[i] = currp;
    for (i = 0; i < src->nModels; i++, currp += MAXLSFNAMELEN)
        shortLInfo->hostModels[i] = currp;

    for(i = 0; i < src->nRes; i++) {
        strcpy(shortLInfo->resName[i], src->resName[i]);
    }

    for(i = 0; i < src->nTypes; i++) {
        strcpy(shortLInfo->hostTypes[i], src->hostTypes[i]);
    }

    for(i = 0; i < src->nModels; i++) {
        strcpy(shortLInfo->hostModels[i], src->hostModels[i]);
    }

    for(i = 0; i < src->nModels; i++)
        shortLInfo->cpuFactors[i] = src->cpuFactors[i];

    return shortLInfo;
}

void
shortLsInfoDestroy(struct shortLsInfo *shortLInfo)
{
    char *allocBase;

    allocBase = (char *)shortLInfo->resName;

    FREEUP(allocBase);
    FREEUP(shortLInfo);
}

/* LIM events
 * Keep a global FILE pointer to the
 * events file. The events file is always open
 * to speed up the operations.
 */
static FILE *logFp;

int
logInit(void)
{
    static char eFile[PATH_MAX];
    static char eFile2[PATH_MAX];
    struct tm *t;
    struct stat sbuf;
    time_t tp;
    int cc;
    hTab *htab;
    hEnt *ent;
    sTab stab;

    sprintf(eFile, "\
%s/lim.events", limParams[LSB_SHAREDIR].paramValue);

    logFp = fopen(eFile, "a+");
    if (logFp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: failed opening %s: %m", __func__, eFile);
        return -1;
    }

    /* Load the events file and return the
     * table of hosts added but not yet
     * removed from LIM.
     */
    htab = loadEvents();
    if (HTAB_NUM_ELEMENTS(htab) == 0) {
	h_freeRefTab_(htab);
	_free_(htab);
        return 0;
    }

    /* If the size of the file is greater
     * then an arbitrary 1024 * 1024
     * switch it.
     */
    cc = stat(eFile, &sbuf);
    if (cc != 0) {
        ls_syslog(LOG_ERR, "\
%s: Ohmygosh stat() failed on %s.. %m",
                  __func__, eFile);
	h_freeRefTab_(htab);
	_free_(htab);
        return -1;
    }

    logLIMStart();

    if (! (sbuf.st_size > LIM_EVENT_MAXSIZE)) {
	h_freeRefTab_(htab);
	_free_(htab);
        return 0;
    }

    fclose(logFp);

    /* Move the current lim.events to
     * lim.events.day.month.hour.min.sec
     */
    tp = time(NULL);
    t = localtime(&tp);
    sprintf(eFile2, "\
%s.%d.%d.%d.%d.%d", eFile, t->tm_mday, t->tm_mon + 1,
            t->tm_hour, t->tm_min, t->tm_sec);

    ls_syslog(LOG_INFO, "\
%s: switching %s in %s", __func__, eFile, eFile2);

    cc = rename(eFile, eFile2);
    if (cc != 0) {
        ls_syslog(LOG_ERR, "\
%s: failed to rename %s in %s %m, keeping the old file",
                  __func__, eFile, eFile2);
	h_freeRefTab_(htab);
	_free_(htab);
	return -1;
    }

    logFp = fopen(eFile, "a+");
    if (logFp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: failed opening %s: %m", __func__, eFile);
	h_freeRefTab_(htab);
	_free_(htab);
        return -1;
    }

    for (ent = h_firstEnt_(htab, &stab);
         ent != NULL ;
         ent = h_nextEnt_(&stab)) {
	struct hostEntry *hPtr;

	hPtr = ent->hData;
	logAddHost(hPtr);
    }

    logLIMStart();
    h_freeRefTab_(htab);
    _free_(htab);

    return 0;
}

int
logLIMStart(void)
{
    struct lsEventRec ev;

    ev.event = EV_LIM_START;
    ev.etime = time(NULL);
    ev.version = OPENLAVA_XDR_VERSION;
    ev.record = NULL;

    ls_writeeventrec(logFp, &ev);

    return 0;
}

int
logLIMDown(void)
{
    struct lsEventRec ev;

    ev.event = EV_LIM_SHUTDOWN;
    ev.etime = time(NULL);
    ev.version = OPENLAVA_XDR_VERSION;
    ev.record = NULL;

    ls_writeeventrec(logFp, &ev);

    return 0;
}

int
logAddHost(struct hostEntry *hPtr)
{
    struct lsEventRec ev;
    struct hostEntryLog hLog;

    /* copy the pointers as well...
     * just don't free.
     */
    memcpy(&hLog, hPtr, sizeof(struct hostEntry));

    ev.event = EV_ADD_HOST;
    ev.etime = time(NULL);
    ev.version = OPENLAVA_XDR_VERSION;
    ev.record = &hLog;

    ls_writeeventrec(logFp, &ev);

    return 0;
}

int
logRmHost(struct hostEntry *hPtr)
{
    struct lsEventRec ev;
    struct hostEntryLog hLog;

    memset(&hLog, 0, sizeof(struct hostEntryLog));
    /* here we only need the name
     */
    strcpy(hLog.hostName, hPtr->hostName);

    ev.event = EV_REMOVE_HOST;
    ev.etime = time(NULL);
    ev.version = OPENLAVA_XDR_VERSION;
    ev.record = &hLog;

    ls_writeeventrec(logFp, &ev);

    return 0;
}

static hTab *
loadEvents(void)
{
    struct lsEventRec *eRec;
    hTab *tab;
    hEnt *e;
    int new;

    tab = calloc(1, sizeof(hTab));

    h_initTab_(tab, 111);

    /* Load the events and build the
     * hash table of runtime hosts.
     */
znovu:
    while ((eRec = ls_readeventrec(logFp))) {
        struct hostEntryLog *hPtr;
	struct hostEntryLog *hPtr2;

        switch (eRec->event) {
            case EV_ADD_HOST:
                hPtr = eRec->record;
                e = h_addEnt_(tab, hPtr->hostName, &new);
                if (new != TRUE) {
                    /* Somebody has been messing around
                     * with the LIM events?
                     */
                    ls_syslog(LOG_WARNING, "\
%s: host %s already added, ignoring the second instance.",
                              __func__, hPtr->hostName);
                    freeHostEntryLog(&hPtr);
                    break;
                }
                e->hData = hPtr;
                break;
            case EV_REMOVE_HOST:
                hPtr = eRec->record;
                e = h_getEnt_(tab, hPtr->hostName);
                if (e == NULL) {
                    ls_syslog(LOG_WARNING, "\
%s: attempt to remove host %s which is not configured, ignoring it.",
                              __func__, hPtr->hostName);
		    freeHostEntryLog(&hPtr);
		    continue;
                }
		hPtr2 = e->hData;
                h_rmEnt_(tab, e);
                freeHostEntryLog(&hPtr);
		freeHostEntryLog(&hPtr2);
                break;
            case EV_LIM_START:
                break;
            case EV_LIM_SHUTDOWN:
                break;
            case EV_EVENT_LAST:
                break;
        }
    }
    if (lserrno != LSE_EOF) {
        ls_syslog(LOG_ERR, "\
%s: unexpected error while reading lim.events: %M", __func__);
        goto znovu;
    }

    /* heresy... actually quite possible
     * if the migrating host is not being used
     */
    if (HTAB_NUM_ELEMENTS(tab) == 0)
	return tab;

    addHostByTab(tab);

    /* the table now contains hosts
     * added but not yet removed
     * create a new lim.events file
     * purged of the old events.
     */
    return tab;
}

/* lim_system()
 * This is a simplified version of the libc system(),
 * it does not handle SIGCHLD so that the main daemon
 * handler is invoked when the child exits.
 */
int
lim_system(const char *cmd)
{
    pid_t pid;

    if (cmd == NULL)
        return -1;

    pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        return pid;
    /* child goes and executes the command
     */
    execl("/bin/sh", "sh", "-c", cmd, NULL);
    exit(-1);
}
