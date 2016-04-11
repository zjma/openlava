
#ifndef TOOLS_LIST_
#define TOOLS_LIST_

#include "intlibout.h"

struct list_ {
    struct list_   *forw;
    struct list_   *back;
    int            num;
    char           *name;
};

/* Just to save typing...
 */
typedef struct list_ list_t;

#define LIST_NUM_ENTS(L) ((L)->num)

extern struct list_ *list_make(const char *);
extern int  list_insert(struct list_ *,
                       struct list_ *,
                       struct list_ *);
extern int list_push(struct list_ *,
                    struct list_ *);
extern int list_enque(struct list_ *,
                     struct list_ *);
extern struct list_ * list_rm(struct list_ *,
                             struct list_ *);
extern struct list_ *list_pop(struct list_ *);
extern struct list_ *list_deque(struct list_ *);
extern void list_free(struct list_ *, void (*f)(void *));

#endif /* TOOLS_LIST_ */
