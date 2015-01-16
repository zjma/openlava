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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 *
 */
#include "fairshare.h"

/* fs_init()
 */
int
fs_init(struct qData *qPtr, struct userConf *uConf)
{
    struct tree_node_ *n;
    hEnt *e;

    qPtr->scheduler->tree
        = sshare_make_tree(qPtr->fairshare,
                           (uint32_t )uConf->numUgroups,
                           (struct group_acct *)uConf->ugroups);

    n = qPtr->scheduler->tree->root;
    while ((n = tree_next_node(n))) {

        e = h_getEnt_(&uDataList, n->path);
        if (e == NULL) {
            ls_syslog(LOG_ERR, "\
%s: skipping uknown user %s", __func__, n->path);
            continue;
        }
    }

    return 0;
}

/* fs_update_sacct()
 */
int
fs_update_sacct(struct qData *qPtr,
                struct jData *jPtr,
                int numPEND,
                int numRUN)
{
    struct tree_ *t;
    struct tree_node_ *n;
    struct share_acct *sacct;
    char *name;

    t = qPtr->scheduler->tree;

    name = jPtr->userName;

    n = hash_lookup(t->node_tab, name);
    if (n == NULL)
        return -1;

    while (n) {
        sacct = n->data;
        sacct->numPEND = sacct->numPEND + numPEND;
        sacct->numRUN = sacct->numRUN + numRUN;
        n = n->parent;
    }

    return 0;
}

/* fs_init_sched_session()
 */
int
fs_init_sched_session(struct qData *qPtr)
{
    struct tree_ *t;

    t = qPtr->scheduler->tree;

    /* Distribute the tokens all the way
     * down the leafs
     */
    sshare_distribute_tokens(t, qPtr->numUsable);

    return 0;
}

/* fs_get_next_job()
 */
int
fs_elect_job(struct qData *qPtr,
                LIST_T *jRef,
                struct jData **jPtr)
{
    struct tree_node_ *n;
    struct share_acct *s;

    /* Simply pop nodes from the
     * leaf link they are already
     * sorted by priority.
     */
    n = pop_link(qPtr->scheduler->tree->leafs);
    s = n->data;

    return 0;
}

/* fs_fin_sched_session()
 */
int
fs_fin_sched_session(struct qData *qPtr)
{
    return 0;
}
