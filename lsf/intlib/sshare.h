
#if !defined(_SSHARE_HEADER_)
#define _SSHARE_HEADER_

#include "intlibout.h"

struct share_acct {
    char *path;
    uint32_t numPEND;
    uint32_t numRUN;
    uint32_t shares;
    double rshares;
    uint32_t sent;
};

struct group_acct {
    char *group;
    char *memberList;
    char *user_shares;
};

extern struct tree_ *sshare_get_tree(const char *,
                                     uint32_t,
                                     struct group_acct *);

#endif /* _SSHARE_HEADER_ */
