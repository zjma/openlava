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

#include "tree.h"

/* tree_init()
 */
struct tree_ *
tree_init(void)
{
    struct tree_ *t;

    t = calloc(1, sizeof(struct tree_));
    assert(t);

    t->root = calloc(1, sizeof(struct tree_node_));
    assert(t->root);
    t->root->path = strdup("/");

    t->node_tab = hash_make(11);

    return t;
}

/* tree_insert_node()
 *
 * Insert child as leftmost respect to the parent
 *
 * Before:
 * r->e
 *
 * After
 *
 * r->child->e
 */
struct tree_node_ *
tree_insert_node(struct tree_node_ *parent,
                 struct tree_node_ *child)
{
    if (parent == NULL)
        return NULL;

    /* No children yet..
     */
    if (parent->child == NULL) {

        parent->child = child;
        child->parent = parent;
        child->child = child->right = child->left = NULL;

        return child;
    }

    /* Insert as left most
     */
    child->parent = parent;
    child->right = parent->child;
    child->left = parent->child->left;
    parent->child->left = child;
    child->child = NULL;
    parent->child = child;

    return child;
}

/* tree_rm_node()
 */
struct tree_node_ *
tree_rm_node(struct tree_node_ *n)
{
    if (n->right)
        n->right->left = n->left;
    if (n->left)
        n->left->right = n->right;
    if (n->parent->child == n)
        n->parent->child = n->right;

    return n;
}

/* tree_print()
 *
 * Print using stack traversal.
 *
 */
void
tree_print(struct tree_node_ *t)
{
}

/* tree_walk()
 *
 * Walk on the tree from the root down
 * level by level and for each node run
 * the function f(). Tree operator...
 * In other words, traverse the link of
 * siblings then dive down to the next
 * level.
 */
int
tree_walk(struct tree_ *r,
	  int (*f)(struct tree_node_ *, struct tree_ *))
{
    link_t  *link;
    struct tree_node_ *n;
    int  cc;

    /* funny tree, no nodes...?
     */
    if (!r->root)
	return -1;

    /* All right only one node...
     */
    n = r->root;
    if (! n->child) {
	cc = (*f)(n, r);
	if (cc < 0)
	    return cc;
	/* protocol, (*f)() may want
	 * to know when we are done.
	 */
	(*f)(NULL, NULL);
	return 0;
    }

    /* Init my stack...
     */
    link = initLink();

  Loop:
    while (n) {

	/* traverse all siblings and enqueue (not push)
	 * on the stack subroots...
	 */
	if (n->child)
	    enqueueLink(link, n);

	    cc = (*f)(n, r);
	    /* protocol, the caller is asking
	     * me to stop the traversal, sure...
	     */
	    if (cc < 0) {
		finLink(link);
		return cc;
	    }
            n = n->right;
    }

    /* pop each subroot and traverse
     * again...
     */
    n = popLink(link);
    if (n) {
	n = n->child;
	goto Loop; /* don't you love this */
    }

    finLink(link);
    /* a little protocol... tell
     * him we are done with the
     * traversal.
     */
    (*f)(NULL, NULL);

    return 0;

} /* tree_walk() */
