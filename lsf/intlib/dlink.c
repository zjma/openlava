/*
 * Copyright (C) 2016 David Bigagli
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

#include "dlink.h"

static inline int dlink_empty(struct dlink *);

/* dlink_make()
 */
struct dlink *
dlink_make(void)
{
    struct dlink *L;

    L = calloc(1, sizeof(struct dlink));
    L->forw = L->back = L;

    return L;
}

/* dlink_insert()
 *
 * Using cartesian coordinates the head h is at
 * zero and elemets are pushed along x axes.
 *
 *       <-- forw ---
 *      /             \
 *     h <--> e2 <--> e
 *      \             /
 *        --- back -->
 *
 * The h points the front, the first element of the list,
 * elements can be pushed in front or enqueued at the back.
 *
 */
int
dlink_insert(struct dlink *l,
             void *el)
{
    struct dlink *dl;

    /*  before: h->el
     */
    dl = calloc(1, sizeof(struct dlink));
    dl->e = el;

    l->forw->back = dl;
    dl->forw = l->forw;
    l->forw = dl;
    dl->back = l;

    /* after h->el2->el
     */
    l->num++;

    return l->num;

}

/* dlink_rm_ent()
 */
void *
dlink_rm_ent(struct dlink *l, struct dlink *dl)
{
    void *v;

    if (dlink_empty(l))
        return NULL;

    dl->back->forw = dl->forw;
    dl->forw->back = dl->back;
    l->num--;

    v = dl->e;
    free(dl);

    return v;
}

/* dlink_dequeue()
 */
void *
dlink_dequeue(struct dlink *l)
{
    void *v;

    v = dlink_rm_ent(l, l->back);

    return v;
}

/* dlink_pop()
 */
void *
dlink_pop(struct dlink *l)
{
    void *v;

    v = dlink_rm_ent(l, l->forw);

    return v;
}

void
dlink_rm(struct dlink *l)
{
    struct dlink *dl;

    if (dlink_empty(l)) {
        free(l);
        return;
    }

    while ((dl = dlink_pop(l)))
        free(dl);

    free(l);
}

static inline int
dlink_empty(struct dlink *l)
{
    if (l->forw == l->back
        && l->forw == l
        && l->back == l) {
        assert(l->num == 0);
        return 1;
    }

    return 0;
}

#if 0
int
main(int argc, char **argv)
{
    struct dlink *l;
    struct dlink *dl;
    struct dlink *dl2;
    int *n;
    int i;
    int iter;

    iter = atoi(argv[1]);

    l = dlink_make();

    for (i = 0; i < iter; i++) {
        n = calloc(1, sizeof(int));
        *n = i;
        dlink_insert(l, n);
    }

    printf("dequeue\n");
    while ((n = dlink_dequeue(l))) {
        printf("%d \n", *n);
        free(n);
    }

    for (i = 0; i < iter; i++) {
        n = calloc(1, sizeof(int));
        *n = i;
        dlink_insert(l, n);
    }

    printf("pop\n");
    while ((n = dlink_pop(l))) {
        printf("%d \n", *n);
        free(n);
    }

    for (i = 0; i < iter; i++) {
        n = calloc(1, sizeof(int));
        *n = i;
        dlink_insert(l, n);
    }

    printf("iterate\n");
    for (dl = l->back;
         dl != l;
         dl = dl2) {

        dl2 = dl->back;
        n = dl->e;

        printf("%d\n", *n);

    }

    printf("remove\n");
    for (dl = l->back;
         dl != l;
         dl = dl2) {

        dl2 = dl->back;
        n = dl->e;
        if (*n == 2
            || *n == 8) {
            n = dlink_rm_ent(l, dl);
            free(n);
            continue;
        }

        printf("%d\n", *n);

    }


    dlink_rm(l);

    return 0;
}
#endif
