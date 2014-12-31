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
    char *name;
    struct tree_node_ *n;
    struct tree_node_ *N;

    file = argv[1];
    if (file == NULL) {
        printf("Specify tree definition file\n");
        return -1;
    }

    stack = make_link();
    t = tree_init("");
    N = t->root;
    name = "root";

z:
    /* First build the root
     */
    l = find_node(name);
    /* if (l == NULL) leaf found
     */

    traverse_init(l, &iter);
    while ((p = traverse_link(&iter))) {

        n = calloc(1, sizeof(struct tree_node_));
        n->path = strdup(p);

        n = tree_insert_node(N, n);
        enqueue_link(stack, n);
        free(p);
    }
    fin_link(l);

    n = pop_link(stack);
    if (n) {
        /* New root and new group name
         */
        N = n;
        name = n->path;
        goto z;
    }

    fin_link(stack);

    tree_walk(t, tree_print);

    n = t->root;
    printf("%s\n", n->path);
    while ((n = tree_next_node(n))) {
        printf("%s\n", n->path);
    }

    tree_free(t);

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
