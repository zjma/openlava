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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */
#include "fairshare.h"

static link_t *get_sacct_link(char *);

/* init()
 */
int
init(struct qData *qPtr, struct userConf *uConf)
{
     struct tree_ *root;
    link_t *link;
    struct share_acct *sAcct;
    linkiter_t iter;

    qPtr->scheduler = calloc(1, sizeof(struct fair_sched));
    assert(qPtr->scheduler);

    root = calloc(1, sizeof(struct tree_));
    assert(root);

    qPtr->scheduler->root = root;

    link = get_sacct_link(qPtr->schedulerType);

    traverse_init(link, &iter);
    while ((sAcct = traverse_link(&iter))) {
    }

    fin_link(link);

    return 0;
}
static link_t *
get_sacct_link(char *s)
{
    link_t *l;

    l = NULL;

    return l;
}
