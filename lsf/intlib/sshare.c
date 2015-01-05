/*
 * Copyright (C) 2014 David Bigagli
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
static void tokenize(char *);
static char *get_next_word(char **);

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
    fin_link(l);

    n = pop_link(stack);
    if (n) {
        root = n;
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

/* free_sacct()
 */
void
free_sacct(struct share_acct *sacct)
{
    _free_(sacct->name);
    _free_(sacct);
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

        sacct = calloc(1, sizeof(struct share_acct));
        assert(sacct);

        sacct->name = strdup(name);
        sacct->shares = shares;
        sum_shares = sum_shares + sacct->shares;

        enqueue_link(l, sacct);
    }

    traverse_init(l, &iter);
    while ((sacct = traverse_link(&iter))) {
        sacct->dshares = (double)sacct->shares/(double)sum_shares;
    }

    return l;

bail:

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
    struct group_acct *g;
    int cc;
    char *w;

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

    l = make_link();
    while ((w = get_next_word(&g->memberList))) {
        struct share_acct *sacct;

        sacct = get_sacct(w, g->user_shares);
        enqueue_link(l, sacct);
    }

    _free_(g->group);
    _free_(g->memberList);
    _free_(g->user_shares);

    return l;
}

static struct share_acct *
get_sacct(const char *acct_name, const char *user_list)
{
    char name[128];
    uint32_t shares;
    int cc;
    struct share_acct *sacct;

    while (1) {

        cc = sscanf(user_list, "%s%u", name, &shares);
        if (cc == EOF)
            break;
        if (strcmp(name, acct_name) != 0)
            continue;

        sacct = calloc(1, sizeof(struct share_acct));
        sacct->name = strdup(name);
        sacct->shares = shares;
        break;
    }

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
