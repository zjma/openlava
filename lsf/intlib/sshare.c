/*
 * Copyright (C) 2014 - 2015 David Bigagli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA
 *
 */

/* sshare = support share
 * Routines and data structure to support hierarchical
 * fairshare in MBD. Implemented in lsf/intlib for eventual
 * reuse somewhere, someday, somehow.
 */

#include "sshare.h"

static link_t *parse_user_shares(const char *);
static link_t *parse_group_member(const char *,
                                  uint32_t,
                                  struct group_acct *);
static struct share_acct *get_sacct(const char *,
                                    const char *);
static uint32_t compute_tokens(struct tree_node_ *, uint32_t);
static void sort_siblings(struct tree_node_ *);
static void tokenize(char *);
static char *get_next_word(char **);
static int node_cmp(const void *, const void *);
static void print_node(struct tree_node_ *);

/* sshare_make_tree()
 */
struct tree_ *sshare_make_tree(const char *user_shares,
                               uint32_t num_grp,
                               struct group_acct *grp)
{
    struct tree_ *t;
    link_t *l;
    link_t *stack;
    linkiter_t iter;
    struct share_acct *sacct;
    struct tree_node_ *n;
    struct tree_node_ *root;

    stack = make_link();
    t = tree_init("");
    /* root summarizes all the tree counters
     */
    t->root->data = make_sacct("/", 1);
    root = NULL;

    l = parse_user_shares(user_shares);

z:
    if (root)
        l = parse_group_member(n->path, num_grp, grp);
    else
        root = t->root;

    traverse_init(l, &iter);
    while ((sacct = traverse_link(&iter))) {

        n = calloc(1, sizeof(struct tree_node_));
        n->path = sacct->name;

        n = tree_insert_node(root, n);
        enqueue_link(stack, n);
        n->data = sacct;
    }

    /* Sort by shares so the tree
     * is always sorted by share
     * priority
     */
    sort_siblings(root);

    fin_link(l);

    n = pop_link(stack);
    if (n) {
        root = n;
        goto z;
    }

    fin_link(stack);

    n = t->root;
    while ((n = tree_next_node(n))) {
        char buf[BUFSIZ];
        /* Create the hash table of nodes and their
         * immediate parent.
         */
        if (n->child == NULL) {
            sprintf(buf, "%s/%s", n->parent->path, n->path);
            hash_install(t->node_tab, buf, n, NULL);
        }
        sprintf(buf, "%s", n->path);
        hash_install(t->node_tab, buf, n, NULL);
        print_node(n);
    }

    traverse_init(t->leafs, &iter);
    while ((n = traverse_link(&iter)))
        print_node(n);

    /* Fairshare tree is built and sorted
     * by decreasing shares, the scheduler
     * algorithm will fill it up with
     * slots from now on.
     */

    return t;
}

/* sshare_distribute_tokens()
 */
int
sshare_distribute_tokens(struct tree_ *t,
                         uint32_t tokens)
{
    struct tree_node_ *n;
    link_t *stack;
    struct share_acct *sacct;

    stack = make_link();
    n = t->root->child;
    /* This must be emptied after every scheduling
     * cycle
     */
    assert(LINK_NUM_ENTRIES(t->leafs) == 0);

znovu:

    /* Iterate at each tree level but
     * don't traverse branches without
     * tokens.
     */
    while (n && tokens) {

        /* enqueue as we want to traverse
         * the tree by priority
         */
        if (n->child)
            enqueue_link(stack, n);

        compute_tokens(n, tokens);

        /* As we traverse in priority order
         * the leafs are also sorted
         */
        sacct = n->data;
        if (n->child == NULL
            && sacct->sent > 0)
            enqueue_link(t->leafs, n);

        n = n->right;
    }

    n = pop_link(stack);
    if (n) {
        /* tokens come from the parent
         */
        sacct = n->data;
        tokens = sacct->sent;
        n = n->child;
        goto znovu;
    }

    fin_link(stack);

    return 0;
}

/* make_saccount()
 */
struct share_acct *
make_sacct(const char *name, uint32_t shares)
{
    struct share_acct *s;

    s = calloc(1, sizeof(struct share_acct));
    s->name = strdup(name);
    s->shares = shares;
    s->jobs = make_link();

    return s;
}

/* free_sacct()
 */
void
free_sacct(struct share_acct *sacct)
{
    _free_(sacct->name);
    fin_link(sacct->jobs);
    _free_(sacct);
}

/* compute_tokens()
 */
static uint32_t
compute_tokens(struct tree_node_ *n, uint32_t tokens)
{
    struct share_acct *s;
    double q;
    double r;
    uint32_t u;

    s = n->data;

    q = s->dshares * (double)tokens;
    r = q - (double)s->numRUN;
    u = lround(r);
    s->sent = MIN(u, s->numPEND);

    return s->sent;
}

/* sort_siblings()
 */
static void
sort_siblings(struct tree_node_ *root)
{
    struct tree_node_ *n;
    int num;
    int i;
    struct tree_node_ **v;

    if (root->child == NULL)
        return;

    if (root->child->right == NULL)
        return;

    num = 0;
    n = root->child;
    while (n) {
        ++num;
        n = n->right;
    }

    v = calloc(num, sizeof(struct tree_node_ *));
    n = root->child;
    num = 0;
    while (n) {
        tree_rm_node(n);
        v[num] = n;
        ++num;
        n = n->right;
    }

    qsort(v, num, sizeof(struct tree_node_ *), node_cmp);

    for (i = 0; i < num; i++) {
        tree_insert_node(root, v[i]);
    }

    free(v);
}

/* parse_user_shares()
 *
 * parse user_shares[[g, 1] [e,1]]
 */
static link_t *
parse_user_shares(const char *user_shares)
{
    link_t *l;
    char *p;
    char *u;
    int cc;
    int n;
    struct share_acct *sacct;
    uint32_t sum_shares;
    linkiter_t iter;

    u = strdup(user_shares);
    assert(u);

    p = strchr(u, '[');
    *p++ = 0;

    tokenize(p);

    l = make_link();
    sum_shares = 0;

    while (1 ) {
        char name[128];
        uint32_t shares;

        cc = sscanf(p, "%s%u%n", name, &shares, &n);
        if (cc == EOF)
            break;
        if (cc != 2)
            goto bail;
        p = p + n;

        sacct = make_sacct(name, shares);
        assert(sacct);

        sum_shares = sum_shares + sacct->shares;

        enqueue_link(l, sacct);
    }

    traverse_init(l, &iter);
    while ((sacct = traverse_link(&iter))) {
        sacct->dshares = (double)sacct->shares/(double)sum_shares;
    }

    _free_(u);
    return l;

bail:

    _free_(u);
    traverse_init(l, &iter);
    while ((sacct = traverse_link(&iter)))
        free_sacct(sacct);
    fin_link(l);

    return NULL;
}

/* parse_group_member()
 */
static link_t *
parse_group_member(const char *gname,
                   uint32_t num,
                   struct group_acct *grps)
{
    link_t *l;
    linkiter_t iter;
    struct group_acct *g;
    int cc;
    char *w;
    char *p;
    uint32_t sum;
    struct share_acct *sacct;

    g = NULL;
    for (cc = 0; cc < num; cc++) {

        if (strcmp(gname, grps[cc].group) == 0) {
            g = calloc(1, sizeof(struct group_acct));
            assert(g);
            g->group = strdup(grps[cc].group);
            g->memberList = strdup(grps[cc].memberList);
            g->user_shares = strdup(grps[cc].user_shares);
            tokenize(g->user_shares);
            break;
        }
    }

    /* gudness...
     */
    if (g == NULL)
        return NULL;

    p = g->memberList;
    l = make_link();
    sum = 0;
    while ((w = get_next_word(&p))) {

        sacct = get_sacct(w, g->user_shares);
        sum = sum + sacct->shares;
        enqueue_link(l, sacct);
    }

    traverse_init(l, &iter);
    while ((sacct = traverse_link(&iter))) {
        sacct->dshares = (double)sacct->shares/(double)sum;
    }

    _free_(g->group);
    _free_(g->memberList);
    _free_(g->user_shares);
    _free_(g);

    return l;
}

static struct share_acct *
get_sacct(const char *acct_name, const char *user_list)
{
    char name[128];
    uint32_t shares;
    int cc;
    int n;
    struct share_acct *sacct;
    char *p;
    char *p0;

    p0 = p = strdup(user_list);

    while (1) {

        cc = sscanf(p, "%s%u%n", name, &shares, &n);
        if (cc == EOF)
            break;
        if (strcmp(name, acct_name) != 0) {
            p = p + n;
            continue;
        }
        sacct = make_sacct(name, shares);
        break;
    }

    _free_(p0);
    return sacct;
}

static void
tokenize(char *p)
{
    int cc;
    int l;

    l = strlen(p);

    for (cc = 0; cc < l; cc++) {
        if (p[cc] == '['
            || p[cc] == ']'
            || p[cc] == ',')
            p[cc] = ' ';
    }
}

static char *
get_next_word(char **line)
{
    static char word[BUFSIZ];
    char *wordp = word;

    while (isspace(**line))
        (*line)++;

    while (**line && !isspace(**line))
        *wordp++ = *(*line)++;

    if (wordp == word)
        return NULL;

    *wordp = '\0';
    return word;
}

/* node_cmp()
 *
 * Function for qsort()
 */
static int
node_cmp(const void *x, const void *y)
{
    struct tree_node_ *n1;
    struct tree_node_ *n2;
    struct share_acct *s1;
    struct share_acct *s2;

    n1 = *(struct tree_node_ **)x;
    n2 = *(struct tree_node_ **)y;

    s1 = n1->data;
    s2 = n2->data;

    if (s1->shares > s2->shares)
        return 1;
    if (s1->shares < s2->shares)
        return -1;

    return 0;
}

/* print_node()
 */
static void
print_node(struct tree_node_ *n)
{
    struct share_acct *s;

    s = n->data;

    printf("%s: node %s shares %d dshares %4.2f\n",
           __func__, n->path, s->shares, s->dshares);
}
