
#if !defined(_SSHARE_HEADER_)
#define _SSHARE_HEADER_

#include "intlibout.h"

/* The share account structure which is on the
 * share tree representing each user and group.
 */
struct share_acct {
    char *name;
    uint32_t numPEND;
    uint32_t numRUN;
    uint32_t shares;
    double rshares;
    uint32_t sent;
};

/* Support data structure equivalent of groupInfoEnt
 */
struct group_acct {
    char *group;
    char *memberList;
    char *user_shares;
};

extern struct tree_ *sshare_make_tree(const char *,
                                      uint32_t,
                                      struct group_acct *);


#endif /* _SSHARE_HEADER_ */
