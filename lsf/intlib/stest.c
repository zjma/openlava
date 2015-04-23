
/* Simulate what MBD does. From a given file build the queue
 * FAIRSHARE string and based on what configured in lsb.users
 * build the struct group_acct. The invoke the share support
 * parser and build the tree that will be used by the fairshare
 * plugin to determine how many slots will be allocated to
 * users in the fairshare queue.
 *
 * The simulation file has the following format:
 *
 * USER_SHARES [[group_a, 3] [group_b , 1]]
 * GROUP_MEMBER (group_a) (david john zebra) ([david, 1] [john,1] [zebra, 1])
 * GROUP_MEMBER (group_b) (crock wlu kluk) ([crock, 2] [wlu,1] [kluk,1])
 *
 * From USER_SHARES extract the membership string and from
 * GROUP_MEMBERS build the group_acct structures.
 *
 * These are the two parameters for sshare_get_tree() API.
 *
 */

#include "sshare.h"

/* print_node()
 */
static int
print_node(struct tree_node_ *n, struct tree_ *t)
{
    struct share_acct *s;

    if (t == NULL
        || n == NULL)
        return -1;

    s = n->data;

    printf("%s: node %s shares %d dshares %4.2f\n",
           __func__, n->name, s->shares, s->dshares);

    return -1;
}

int
main(int argc, char **argv)
{
    struct tree_ *t;
    uint32_t num;
    struct group_acct *grps;
    char *user_shares;

    num = 0;
    grps = NULL;
    user_shares = NULL;

    t = sshare_make_tree(user_shares,
                         num,
                         grps);
    tree_walk2(t, print_node);

    return 0;
}
