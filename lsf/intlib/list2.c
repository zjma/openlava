
/* Simple double linked list.
 * The idea is very simple we have 2 insertion methods
 * enqueue which adds at the end of the queue and push
 * which adds at the front. Then we simply pick the first
 * element in the queue. If you have inserted by enqueue
 * you get a FCFS policy if you pushed you get a stack policy.
 *
 *
 * FCFS
 *
 *   H->1->2->3->4
 *
 * you retrive elements as 1, 2 etc
 *
 * Stack:
 *
 * H->4->3->2->1
 *
 * you retrieve elements as 4,3, etc
 *
 *
 */

#include "list2.h"

struct list_ *
list_make(const char *name)
{
    struct list_ *L;

    L = calloc(1, sizeof(struct list_));
    assert(L);
    L->forw = L->back = L;

    L->name = strdup(name);
    assert(L->name);

    return L;

}

/* list_inisert()
 *
 * Using cartesian coordinates the head h is at
 * zero and elemets are pushed along x axes.
 *
 *       <-- back ---
 *      /             \
 *     h <--> e2 <--> e
 *      \             /
 *        --- forw -->
 *
 * The h points the front, the first element of the list,
 * elements can be pushed in front or enqueued at the back.
 *
 */
int
list_insert(struct list_ *h,
           struct list_ *e,
           struct list_ *e2)
{
    assert(h && e && e2);

    /*  before: h->e
     */

    e->back->forw = e2;
    e2->back = e->back;
    e->back = e2;
    e2->forw = e;

    /* after h->e2->e
     */

    h->num++;

    return h->num;

}

/*
 * list_enqueue()
 *
 * Enqueue a new element at the end
 * of the list.
 *
 * listenque()/listdeque()
 * implements FCFS policy.
 *
 */
int
list_enque(struct list_ *h,
          struct list_ *e2)
{
    assert(h && e2);

    /* before: h->e
     */
    list_insert(h, h, e2);
    /* after: h->e->e2
     */
    return 0;
}

/* list_deque()
 */
struct list_ *
list_deque(struct list_ *h)
{
    struct list_ *e;

    if (h->forw == h) {
        assert(h->back == h);
        return NULL;
    }

    /* before: h->e->e2
     */

    e = list_rm(h, h->forw);

    /* after: h->e2
     */

    return e;
}

/*
 * list_push()
 *
 * Push e at the front of the list
 *
 * H --> e --> e2
 *
 */
int
listpush(struct list_ *h,
         struct list_ *e2)
{
    /* before: h->e
     */
    list_insert(h, h->forw, e2);

    /* after: h->e2->e
     */

    return 0;
}

/* list_pop()
 */
struct list_ *
list_pop(struct list_ *h)
{
    struct list_ *e;

    e = list_deque(h);

    return e;
}

struct list_ *
list_rm(struct list_ *h,
       struct list_ *e)
{
    if (h->num == 0)
        return NULL;

    e->back->forw = e->forw;
    e->forw->back = e->back;
    h->num--;

    return e;

}


/* list_free()
 */
void
list_free(struct list_ *L,
         void (*f)(void *))
{
    struct list_ *l;

    while ((l = list_pop(L))) {
        if (f == NULL)
            free(l);
        else
            (*f)(l);
    }

    free(L->name);
    free(L);
}
