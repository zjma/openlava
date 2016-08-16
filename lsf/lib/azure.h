#ifndef __AZURE_H__
#define __AZURE_H__


#define AZURE_ST_READY      0
#define AZURE_ST_NEEDINIT   1


/**Initialize Azure Batch library (by reading params from azure.conf).
 */
void AZURE_init();


/**Check Azure Batch library status.
 *
 * \return AZURE_ST_READY if library has been initialized,
 *          or AZURE_ST_NEEDINIT otherwise.
 */
int AZURE_libStatus();


int AZURE_getCoreNum();
const char *AZURE_getSharedKeyB64();
const char *AZURE_getAccountName();
const char *AZURE_getUrl();
const char *AZURE_getJobId();

const char *AZURE_scriptPath();

#endif
