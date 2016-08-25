#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include "azure.h"

#define BUFLEN 1024
static int status=AZURE_ST_NEEDINIT;
static int CoreNum=1;
static char BatchAccount[BUFLEN+1]={'a', 0};
static char BatchKey[BUFLEN+1]={'k',0};
static char BatchUrl[BUFLEN+1]={'u',0};
static char BatchMpiJobId[BUFLEN+1]={'j',0};
static char StorageAccount[BUFLEN+1]={'a', 0};
static char StorageKey[BUFLEN+1]={'k',0};
static char ScriptPath[BUFLEN+1]={'s', 0};


void AZURE_init()
{
    char *lavatop=getenv("OPENLAVA_TOP");
    if (!lavatop) lavatop="";
    char *script_loc = "sbin/agent.py";
    
    sprintf(ScriptPath, "%s/%s", lavatop, script_loc);
    syslog(LOG_INFO, ScriptPath);
    
    char *p;

    p=getenv("AZURE_BATCH_KEY");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(BatchKey, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure batch key. Use \"%s\" by default.",
                BatchKey);
    }
    
    p=getenv("AZURE_BATCH_URL");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(BatchUrl, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure batch url. Use \"%s\" by default.",
                BatchUrl);
    }

    p=getenv("AZURE_BATCH_MPI_JOB_ID");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(BatchMpiJobId, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure batch mpi job id. Use \"%s\" by default.",
                BatchMpiJobId);
    }

    p=getenv("AZURE_BATCH_ACCOUNT");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(BatchAccount, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure account. Use \"%s\" by default.",
                BatchAccount);
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

    p=getenv("AZURE_STORAGE_ACCOUNT");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(StorageAccount, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure storage account. Use \"%s\" by default.",
                StorageAccount);
    }

    p=getenv("AZURE_STORAGE_KEY");
    if (p && strlen(p)<=BUFLEN)
    {
        strcpy(StorageKey, p);
    }
    else
    {
        syslog(LOG_WARNING,
                "Invalid/missing azure storage key. Use \"%s\" by default.",
                StorageKey);
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

const char *AZURE_scriptPath()
{
    return ScriptPath;
}

const char *AZURE_getBatchAccount()
{
    return BatchAccount;
}

const char *AZURE_getBatchUrl()
{
    return BatchUrl;
}

const char *AZURE_getBatchKeyB64()
{
    return BatchKey;
}

const char *AZURE_getStorageAccount()
{
    return StorageAccount;
}

const char *AZURE_getStorageKeyB64()
{
    return StorageKey;
}

const char *AZURE_getMpiJobName()
{
    return BatchMpiJobId;
}

