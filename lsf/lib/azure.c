#include <stdlib.h>
#include <stdio.h>
#include "azure.h"


static int status = -1;
static int CoreNum = 0;
static char *SharedKey=NULL;
static char *Url=NULL;
static char *JobId=NULL;
static char *Account=NULL;


int AZURE_init()
{
    /*
    FILE *f=NULL;
    getenv("LSF_ENVDIR");
    f = fopen(path, "r");
    if (!f) return;
    */
    CoreNum=99;
    SharedKey="SCtdsT4NSfjjsDmF6r6RnehhBLevtG3RsS47mp1hVrbw5EVDPNfRPPaLQPJg9ICEsfpr068ihm+Zggftr99p8Q==";
    Url="https://tzhm.australiaeast.batch.azure.com";
    JobId="donotshutdown";
    Account="tzhm";
    status = 0;
    return 0;
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

