
#if !defined(_SSHARE_HEADER_)
#define _SSHARE_HEADER_

#include "tree.h"
#include "link.h"

/* The share account structure which is on the
 * share tree representing each user and group.
 */
struct share_acct {
    char *name;
    uint32_t shares;
    double dshares;
    uint32_t sent;
    int numPEND;
    int numRUN;
    uint32_t totalRUN;
    link_t *jobs;
};

/* Support data structure equivalent of groupInfoEnt
 */
struct group_acct {
    char *group;
    char *memberList;
    char *user_shares;
};

/* sshare_make_tree()
 *
 * Make the fairshare tree based on the MBD configuration
 * from lsb.queues and lsb.users
 *
 * The root of the tree is specified in lsb.queues:
 *
 *  USER_SHARES[[G1,1] [G2,1]]
 *
 * the users are specified in lsb.users and represented
 * as an array of group_acct (groupInfoEnt) data structures.
 *
 */
extern struct tree_ *sshare_make_tree(const char *,
                                      uint32_t,
                                      struct group_acct *);
extern struct share_acct *make_sacct(const char *, uint32_t);
extern void free_sacct(struct share_acct *);
extern int sshare_distribute_slots(struct tree_ *,
                                   uint32_t);

#endif /* _SSHARE_HEADER_ */
