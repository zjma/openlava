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

#include "sshare.h"

static link_t *parse_user_shares(const char *);
static link_t *parse_group_member(const char *,
                                  uint32_t,
                                  struct group_acct *);

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

static link_t *
parse_user_shares(const char *user_shares)
{
    link_t *l = NULL;
    return l;
}

static link_t *
parse_group_member(const char * gname,
                   uint32_t num,
                   struct group_acct *grps)
{
    link_t *l = NULL;
    return l;
}
