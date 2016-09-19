#ifndef __AZURE_H__
#define __AZURE_H__


#define AZURE_ST_READY      0
#define AZURE_ST_NEEDINIT   1


/**Initialize Azure Batch library (by reading params from burst.conf).
 */
void AZURE_init();


/**Check Azure Batch library status.
 *
 * \return AZURE_ST_READY if library has been initialized,
 *          or AZURE_ST_NEEDINIT otherwise.
 *
 * \note   You should trust what AZURE_getXXX() returns
 *         only when calling AZURE_init() returns AZURE_ST_READY.
 */
int AZURE_libStatus();


int AZURE_getCoreNum();
const char *AZURE_getBatchAccount();
const char *AZURE_getBatchUrl();
const char *AZURE_getBatchKeyB64();
const char *AZURE_getStorageAccount();
const char *AZURE_getStorageKeyB64();
const char *AZURE_getStorageContainerName();
const char *AZURE_getMpiJobName();

const char *AZURE_scriptPath();

#endif

