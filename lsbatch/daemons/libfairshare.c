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
                int numJobs,
                int numPEND,
                int numRUN,
                int numDONE)
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

    sacct = n->data;
    sacct->uid = jPtr->userId;

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
             LIST_T *jRefList,
             struct jRef **jRef)
{
    struct tree_node_ *n;
    struct share_acct *s;
    link_t *l;
    struct jRef *jref;
    struct jData *jPtr;

    l = qPtr->scheduler->tree->leafs;
    if (LINK_NUM_ENTRIES(l) == 0) {
        *jRef = NULL;
        return -1;
    }

    n = pop_link(l);
    s = n->data;
    assert(s->sent > 0);

    for (jref = (struct jRef *)jRefList->back;
         jref != (void *)jRefList;
         jref = jref->back) {

        jPtr = jref->job;

        if (jref->back == (void *)jRefList
            || jPtr->qPtr->queueId != jPtr->back->qPtr->queueId) {
            *jRef = NULL;
        }

        if (jPtr->userId == s->uid)
            break;
    }

    /* More to dispatch from this node
     * so back to the leaf link
     */
    if (s->sent > 0)
        push_link(l, n);

    *jRef = jref;

    return 0;
}

/* fs_fin_sched_session()
 */
int
fs_fin_sched_session(struct qData *qPtr)
{
    return 0;
}

/* fs_get_saccts()
 *
 * Return an array of share accounts for a queue tree.
 * In other words flatten the tree.
 */
int
fs_get_saccts(struct qData *qPtr, int *num, struct share_acct ***as)
{
    struct tree_ *t;
    struct tree_node_ *n;
    struct share_acct **s;
    int nents;
    int i;

    t = qPtr->scheduler->tree;

    /* First let's count the number of nodes
     */
    n = t->root;
    nents = 0;
    while ((n = tree_next_node(n)))
        ++nents;

    s = calloc(nents, sizeof(struct share_acct *));
    assert(s);
    /* Nwo simply address the array elemens
     * to the nodes. Don't dup memory
     */

    i = 0;
    n = t->root;
    while ((n = tree_next_node(n))) {
        s[i] = n->data;
        ++i;
    }

    *as = s;
    *num = nents;

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
