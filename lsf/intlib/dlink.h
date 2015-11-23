
#ifndef _DLINK_T_
#define _DLINK_T_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct dlink {
    struct dlink *forw;
    struct dlink *back;
    int num;
    void *e;
};


#define DLINK_NUM_ENTS(L) ((L)->num)

extern struct dlink *dlink_make(void);
extern int dlink_insert(struct dlink *, void *);
extern int  dlink_enque(struct dlink *, void *);
extern void *dlink_dqueue(struct dlink *);
extern void *dlink_pop(struct dlink *);
extern void *dlink_rm_ent(struct dlink *, struct dlink *);
extern void dlink_rm(struct dlink *);

#endif /* _DLINK_T */
