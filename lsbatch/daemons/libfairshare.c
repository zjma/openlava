/*
 * Copyright (C) 2014-2016 David Bigagli
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

static struct tree_node_ *get_user_node(struct hash_tab *,
                                        struct jData *);

/* fs_init()
 */
int
fs_init(struct qData *qPtr, struct userConf *uConf)
{
    qPtr->fsSched->tree
        = sshare_make_tree(qPtr->fairshare,
                           (uint32_t )uConf->numUgroups,
                           (struct group_acct *)uConf->ugroups);

    if (qPtr->fsSched->tree == NULL) {
        ls_syslog(LOG_ERR, "\
%s: queues %s failed to fairshare configuration, fairshare disabled",
                  __func__, qPtr->queue);
        return -1;
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
                int numRUN)
{
    struct tree_ *t;
    struct tree_node_ *n;
    struct share_acct *sacct;
    int numRAN;

    t = NULL;
    if (qPtr->fsSched)
	t = qPtr->fsSched->tree;
    else if (qPtr->own_sched)
	t = qPtr->own_sched->tree;

    if (t == NULL)
	return -1;

    n = get_user_node(t->node_tab, jPtr);
    if (n == NULL)
        return -1;

    sacct = n->data;
    sacct->uid = jPtr->userId;

    /* The job is starting so decrease the
     * counter of slots this can use.
     */
    if (numPEND < 0
	&& numRUN  > 0) {
	sacct->sent--;
    }

    /* Hoard the running jobs
     */
    numRAN = 0;
    if (numRUN > 0)
        numRAN = numRUN;

    while (n) {
        sacct = n->data;
        sacct->numPEND = sacct->numPEND + numPEND;
        sacct->numRUN = sacct->numRUN + numRUN;
        sacct->numRAN = sacct->numRAN + numRAN;
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

    t = NULL;
    if (qPtr->fsSched)
	t = qPtr->fsSched->tree;
    else if (qPtr->own_sched)
	t = qPtr->own_sched->tree;

    if (t == NULL)
	return -1;

    /* Distribute the tokens all the way
     * down the leafs
     */
    sshare_distribute_slots(t, qPtr->numFairSlots);

    return 0;
}

/* fs_elect_job()
 */
int
fs_elect_job(struct qData *qPtr,
             LIST_T *jRefList,
             struct jRef **jRef)
{
    struct tree_ *t;
    struct tree_node_ *n;
    struct share_acct *s;
    link_t *l;
    struct jRef *jref;
    struct jData *jPtr;
    uint32_t sent;
    struct uData *uPtr;
    struct dlink *dl;
    hEnt *ent;
    int count;
    int found;

    t = NULL;
    if (qPtr->fsSched)
	t = qPtr->fsSched->tree;
    else if (qPtr->own_sched)
	t = qPtr->own_sched->tree;

    if (t == NULL)
	return -1;

    l = t->leafs;
    if (LINK_NUM_ENTRIES(l) == 0) {
        *jRef = NULL;
        return -1;
    }

    /* pop() so if the num sent drops
     * to zero we remove it and never traverse
     * it again. The s->sent is updated in
     * fs_update_sacct() when the job is started.
     */
dalsi:
    sent = 0;
    while ((n = pop_link(l))) {
        s = n->data;
        if (s->sent > 0) {
            ++sent;
            break;
        }
    }
    /* No more jobs to sent
     */
    if (sent == 0) {
        *jRef = NULL;
        return -1;
    }

    if (logclass & LC_FAIR)
	ls_syslog(LOG_INFO, "\
%s: account %s num slots %d queue %s", __func__, s->name,
		  s->sent + 1, qPtr->queue);

    ent = h_getEnt_(&uDataList, s->name);
    uPtr = ent->hData;

    found = false;
    count = 0;
    jref = NULL;
    jPtr = NULL;
    for (dl = uPtr->jobs->back;
	 dl != uPtr->jobs;
	 dl = dl->back) {
	++count;

	jref = dl->e;
	jPtr = jref->job;

	assert(jPtr->userId == s->uid);
        if (jPtr->qPtr == qPtr) {
	    if (logclass & LC_FAIR) {
		ls_syslog(LOG_INFO, "\
%s: jqueue %s %p queue %s %p job %p ref %p %d", __func__,
			  jPtr->qPtr->queue, jPtr->qPtr,
			  qPtr->queue, qPtr,
			  jPtr, jref, count);
	    }
	    dlink_rm_ent(uPtr->jobs, dl);
	    found = true;
	    break;
	}
    }

    if (jPtr == NULL
	|| found == false) {
	/* This happens if MBD_MAX_JOBS_SCHED
	 * is configured or if the user has no
	 * jobs in the current queue.
	 */
	if (logclass & LC_FAIR) {
	    ls_syslog(LOG_INFO, "\
%s: user %s is chosen %d in queue %s but has no jobs count %d numpend %d numj %d",
		      __func__, s->name,
		      s->sent + 1, qPtr->queue,
		      count, uPtr->numPEND, dl->num);
	}
	goto dalsi;
    }

    if (jPtr) {
	if (logclass & LC_FAIR) {
	    ls_syslog(LOG_INFO, "\
%s: user %s job %s queue %s found in %d iterations", __func__, s->name,
		      lsb_jobid2str(jPtr->jobId), qPtr->queue, count);
	}
    }
    /* More to dispatch from this node
     * so back to the leaf link
     */
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

    t = NULL;
    if (qPtr->fsSched)
	t = qPtr->fsSched->tree;
    else if (qPtr->own_sched)
	t = qPtr->own_sched->tree;

    if (t == NULL)
	return -1;

    /* First let's count the number of nodes
     */
    n = t->root;
    nents = 0;
    while ((n = tree_next_node(n)))
        ++nents;

    s = calloc(nents, sizeof(struct share_acct *));
    assert(s);
    /* Now simply address the array elemens
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

/* get_user_node()
 */
static struct tree_node_ *
get_user_node(struct hash_tab *node_tab,
              struct jData *jPtr)
{
    struct tree_node_ *n;
    struct tree_node_ *n2;
    struct share_acct *sacct;
    struct share_acct *sacct2;
    uint32_t sum;
    char key[MAXLSFNAMELEN];;

    if (jPtr->shared->jobBill.userGroup[0] != 0) {
        sprintf(key, "\
%s/%s", jPtr->shared->jobBill.userGroup, jPtr->userName);
    } else {
        sprintf(key, "%s", jPtr->userName);
    }

    /* First direct lookup on the table
     * of nodes.
     */
    n = hash_lookup(node_tab, key);
    if (n)
        return n;

    /* If job specifies parent group lookup
     * all in parent group
     */
    if (jPtr->shared->jobBill.userGroup[0] != 0) {
        sprintf(key, "%s/all", jPtr->shared->jobBill.userGroup);
        n = hash_lookup(node_tab, key);
    } else {
        /* Job specifies no parent group
         * so lookup all in any group
         */
        n = hash_lookup(node_tab, "all");
	if (n == NULL)
	    n = hash_lookup(node_tab, "default");
    }

    /* No user, no all, no hope...
     */
    if (n == NULL)
        return NULL;

    sacct = n->data;
    assert(sacct->options & SACCT_USER);

    n2 = calloc(1, sizeof(struct tree_node_));
    sacct2 = make_sacct(jPtr->userName, sacct->shares);
    sacct2->uid = jPtr->userId;
    n2->data = sacct2;
    n2->name = strdup(jPtr->userName);

    tree_insert_node(n->parent, n2);
    sacct2->options |= SACCT_USER;
    sprintf(key, "%s/%s", n2->parent->name, n2->name);
    hash_install(node_tab, key, n2, NULL);
    sprintf(key, "%s", n2->name);
    hash_install(node_tab, key, n2, NULL);

    sum = 0;
    n2 = n->parent->child;
    while (n2) {

        sacct = n2->data;
        if (sacct->options & SACCT_USER_ALL) {
            n2 = n2->right;
            continue;
        }
        sum = sum + sacct->shares;
        n2 = n2->right;
    }

    n2 = n->parent->child;
    while (n2) {

        sacct = n2->data;
        if (sacct->options & SACCT_USER_ALL) {
            n2 = n2->right;
            continue;
        }
        sacct->dshares = (double)sacct->shares/(double)sum;
        n2 = n2->right;
    }

    return n->parent->child;
}

/* fs_own_init()
 */
int
fs_own_init(struct qData *qPtr, struct userConf *uConf)
{
    qPtr->own_sched->tree
        = sshare_make_tree(qPtr->ownership,
                           (uint32_t )uConf->numUgroups,
                           (struct group_acct *)uConf->ugroups);

    if (qPtr->own_sched->tree == NULL) {
        ls_syslog(LOG_ERR, "\
%s: queues %s failed to ownership configuration, ownership disabled",
                  __func__, qPtr->queue);
        _free_(qPtr->ownership);
	qPtr->qAttrib &= ~Q_ATTRIB_OWNERSHIP;
        return -1;
    }

    return 0;
}

/* fs_init_own_sched_session()
 */
int
fs_init_own_sched_session(struct qData *qPtr)
{
    struct tree_ *t;

    if (qPtr->num_owned_slots == 0)
	return 0;

    t = qPtr->own_sched->tree;

    /* Distribute the tokens all the way
     * down the leafs
     */
    sshare_distribute_own_slots(t, qPtr->num_owned_slots);

    return 0;
}
