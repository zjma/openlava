/*
 * Copyright (C) 2014-2015 David Bigagli
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

static char *get_user_key(struct jData *);

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
    char *key;

    t = qPtr->scheduler->tree;

    key = get_user_key(jPtr);

    n = hash_lookup(t->node_tab, key);
    if (n == NULL)
        return -1;

    /* Add the job to the reference
     * link. The job will be removed
     * during scheduling.
     */
    if (numPEND > 0) {
        sacct = n->data;
        enqueue_link(sacct->jobs, jPtr);
    }

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
    sshare_distribute_slots(t, qPtr->numUsable);

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
    link_t *l;

    l = qPtr->scheduler->tree->leafs;
    if (LINK_NUM_ENTRIES(l) == 0) {
        *jPtr = NULL;
        return -1;
    }

    n = pop_link(l);
    s = n->data;
    assert(s->sent > 0);

    if (s->sent > 0) {

        assert(LINK_NUM_ENTRIES(s->jobs) >= s->sent);
        s->sent--;
        /* Remember to deal with user priorities
         */
        *jPtr = pop_link(s->jobs);
    }

    /* More to dispatch from this node
     * so back to the leaf link
     */
    if (s->sent > 0)
        push_link(l, n);

    return 0;
}

/* fs_fin_sched_session()
 */
int
fs_fin_sched_session(struct qData *qPtr)
{
    return 0;
}

/* get_user_key()
 */
static char *
get_user_key(struct jData *jPtr)
{
    static char buf[MAXLSFNAMELEN];

    if (jPtr->shared->jobBill.userGroup[0] != 0) {
        sprintf(buf, "\
%s/%s", jPtr->shared->jobBill.userGroup, jPtr->userName);
    } else {
        sprintf(buf, "%s", jPtr->userName);
    }

    return buf;
}
