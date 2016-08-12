#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include "azure.h"

#define BUFLEN 1024
static int status=-1;
static int CoreNum=0;
static char SharedKey[BUFLEN+1];
static char Url[BUFLEN+1];
static char JobId[BUFLEN+1];
static char Account[BUFLEN+1];
static char ScriptPath[BUFLEN+1];


int AZURE_init()
{
    char *lavatop=getenv("OPENLAVA_TOP");
    if (!lavatop) lavatop="";
    char *script_loc = "sbin/agent.py";
    
    int ret=-1;
    
    sprintf(ScriptPath, "%s/%s", lavatop, script_loc);
    syslog(LOG_INFO, ScriptPath);
    
    char *p;

    p=getenv("AZURE_SHARED_KEY");
    if (!p) goto final;
    strncpy(SharedKey, p, BUFLEN);

    p=getenv("AZURE_URL");
    if (!p) goto final;
    strncpy(Url, p, BUFLEN);

    p=getenv("AZURE_JOBID");
    if (!p) goto final;
    strncpy(JobId, p, BUFLEN);

    p=getenv("AZURE_ACCOUNT");
    if (!p) goto final;
    strncpy(Account, p, BUFLEN);

    p=getenv("AZURE_CORE_NUM");
    if (!p) goto final;
    CoreNum=atoi(p);
    
    status = 0;
    
    ret=0;

final:
    return ret;
}


int AZURE_libStatus()
{
    return status;
}


int AZURE_getCoreNum()
{
    return CoreNum;
}


const char *AZURE_getSharedKeyB64()
{
    return SharedKey;
}


const char *AZURE_getAccountName()
{
    return Account;
}


const char *AZURE_getUrl()
{
    return Url;
}


const char *AZURE_getJobId()
{
    return JobId;
}

const char *AZURE_scriptPath()
{
    return ScriptPath;
}

