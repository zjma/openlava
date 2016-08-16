#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include "azure.h"

#define BUFLEN 1024
static int status=AZURE_ST_NEEDINIT;
static int CoreNum=1;
static char SharedKey[BUFLEN+1]={'k',0};
static char Url[BUFLEN+1]={'u',0};
static char JobId[BUFLEN+1]={'j',0};
static char Account[BUFLEN+1]={'a', 0};
static char ScriptPath[BUFLEN+1]={'s', 0};


void AZURE_init()
{
    char *lavatop=getenv("OPENLAVA_TOP");
    if (!lavatop) lavatop="";
    char *script_loc = "sbin/agent.py";
    
    sprintf(ScriptPath, "%s/%s", lavatop, script_loc);
    syslog(LOG_INFO, ScriptPath);
    
    char *p;

    p=getenv("AZURE_SHARED_KEY");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(SharedKey, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure shared key. Use \"%s\" by default.",
                SharedKey);
    }
    
    p=getenv("AZURE_URL");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(Url, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure url. Use \"%s\" by default.",
                Url);
    }

    p=getenv("AZURE_JOBID");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(JobId, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure job id. Use \"%s\" by default.",
                JobId);
    }

    p=getenv("AZURE_ACCOUNT");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(Account, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure account. Use \"%s\" by default.",
                Account);
    }

    p=getenv("AZURE_CORE_NUM");
    if (p)
    {
        CoreNum=atoi(p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure core num. Use %d by default.",
                CoreNum);
    }

    status = AZURE_ST_READY;
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

