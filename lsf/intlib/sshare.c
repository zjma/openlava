
#include "sshare.h"

static link_t *parse_root(const char *);
static link_t *parse_users(uint32_t, struct group_acct *);

struct tree_ *sshare_get_tree(const char *group_root,
                              uint32_t num_users,
                              struct group_acct *users)
{
    struct tree_ *t;
    link_t *l;
    link_t *stack;
    linkiter_t iter;
    char *p;
    struct tree_node_ *n;
    struct tree_node_ *N;

    stack = make_link();
    t = tree_init("");
    N = t->root;
z:
    if (N->child == 0)
        l = parse_root(group_root);
    else
        l = parse_users(num_users, users);

    traverse_init(l, &iter);
    while ((p = traverse_link(&iter))) {

        n = calloc(1, sizeof(struct tree_node_));
        assert(n);
        n->path = strdup(p);

        n = tree_insert_node(N, n);
        enqueue_link(stack, n);

        n->data = calloc(1, sizeof(struct share_acct));
        assert(n->data);

        free(p);
    }
    fin_link(l);

    n = pop_link(stack);
    if (n) {
        N = n;
        goto z;
    }

    fin_link(stack);

    n = t->root;
    printf("%s\n", n->path);
    while ((n = tree_next_node(n))) {
        printf("%s\n", n->path);
    }

    return t;

}

/* parse_root()
 *
 * Parse: [[group_a, 3] [group_b , 1]]
 *
 * and return a link of share accounts.
 *
 */
static link_t *
parse_root(const char *group)
{
    link_t *l;
    l = NULL;
    return l;
}

/* parse_users()
 *
 * Parse:
 * (group_a) (david john zebra) ([david, 1] [john,1] [zebra, 1])
 * given in the form of group_acct and return a list of
 * share account.
 *
 */
static link_t *
parse_users(uint32_t num, struct group_acct *acct)
{
    link_t *l;
    l = NULL;
    return l;
}
