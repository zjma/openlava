#ifndef __AZURE_H__
#define __AZURE_H__


/**Initialize Azure Batch library (by reading params from azure.conf).
 *
 * \return  0 if succ, or -1 if operation failed or azure.conf content invalid.
 */
int AZURE_init();


/**Check Azure Batch library status.
 *
 * \return 0 if library has been initialized,
 *          or -1 otherwise
 *          (i.e., calling methods below returns undefined values).
 */
int AZURE_libStatus();


int AZURE_getCoreNum();
const char *AZURE_getSharedKeyB64();
const char *AZURE_getAccountName();
const char *AZURE_getUrl();
const char *AZURE_getJobId();

const char *AZURE_scriptPath();

#endif
