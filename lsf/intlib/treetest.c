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

#include <stdio.h>
#include "tree.h"
#include "link.h"

static link_t *find_node(const char *);
static char *file;

int
main(int argc, char **argv)
{
    struct tree_ *t;
    link_t *l;
    link_t *stack;
    linkiter_t iter;
    char *p;
    struct tree_node_ *n;
    struct tree_node_ *child;

    file = argv[1];
    if (file == NULL) {
        printf("Specify tree definition file\n");
        return -1;
    }

    t = tree_init("");

    /* First build the root
     */
    l = find_node("root");
    if (l == NULL) {
        printf("Ohmygosh root not found!\n");
        return -1;
    }

    traverse_init(l, &iter);
    while ((p = traverse_link(&iter))) {

        n = calloc(1, sizeof(struct tree_node_));
        n->path = strdup(p);

        n = tree_insert_node(t->root, n);
        free(p);
    }
    fin_link(l);

    /* The traverse the children and build their
     * subtrees
     */
    n = t->root;
    stack = make_link();
z:
    while (n) {

        l = find_node(n->path);
        if (l) {

            traverse_init(l, &iter);
            while ((p = traverse_link(&iter))) {
                child = calloc(1, sizeof(struct tree_node_));
                child->path = strdup(p);
                child = tree_insert_node(n->child, child);
                /* push each one on da stack
                 */
                push_link(stack, child);
                free(p);
            }
        }
        /* nexte
         */
        n = n->right;
    }

    n = pop_link(stack);
    if (n)
        goto z;

    fin_link(stack);

    return 0;
}

/* find_node()
 *
 * Find the ascii node name describing the tree element.
 */
static link_t *
find_node(const char *name)
{
    FILE *fp;
    link_t *link;
    int n;
    int nitems;
    int i;
    char item[128];
    char *buf;
    char *buf0;

    fp = fopen(file, "r");
    if (fp == NULL)
        return NULL;

    link = NULL;
    buf0 = buf = calloc(BUFSIZ, sizeof(char));

    while (fgets(buf, BUFSIZ, fp)) {

        sscanf(buf, "%s%n", item, &n);
        if (strcmp(item, name) != 0)
            continue;

        buf = buf + n;
        /* ok item found
         */
        sscanf(buf, "%d%n", &nitems, &n);
        buf = buf + n;

        link = make_link();
        for (i = 0; i < nitems; i++) {
            char *p;
            char child[64];

            sscanf(buf, "%s%n", child, &n);
            p = strdup(child);
            enqueue_link(link, p);
            buf = buf + n;
        }
    }

    fclose(fp);
    free(buf0);

    return link;
}
