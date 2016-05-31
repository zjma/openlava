/*
 * Copyright (C) 2016 David Bigagli
 * Copyright (C) 2007 Platform Computing Inc
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#if !defined(_JGRP_H_)
#define _JGRP_H_

#define DESTROY_REF(x, y) { \
     y(x);\
     x = NULL;\
}

#define    JGRP_VOID         -1
#include "../../lsf/intlib/jidx.h"

typedef void (*FREE_JGARRAY_FUNC_T)(void *);

#define JG_ARRAY_BASE \

struct jgArrayBase {
    int      numRef;
    int      status;
    time_t   changeTime;
    int      oldStatus;
    int      userId;
    char    *userName;
    int      fromPlatform;
    void    (*freeJgArray)(void *);
    int      counts[NUM_JGRP_COUNTERS + 1];
};

struct jarray {
    int      numRef;
    int      status;
    time_t   changeTime;
    int      oldStatus;
    int      userId;
    char    *userName;
    int      fromPlatform;
    void    (*freeJgArray)(void *);
    int      counts[NUM_JGRP_COUNTERS + 1];
    struct jData *jobArray;
    int   maxJLimit;
};

/* job group data
 */
struct jgrpData {
    int numRef;
    int status;
    int userId;
    char *userName;
    time_t submit_time;
    void (*freeJgArray)(void *);
    int counts[NUM_JGRP_COUNTERS + 1];
    int max_jobs;
};

/* Helper macros
 */
#define JOB_DATA(node)   ((struct jData *)node->ndInfo)
#define JGRP_DATA(node)  ((struct jgrpData *)node->ndInfo)
#define ARRAY_DATA(node) ((struct jarray *)node->ndInfo)

/* Generic node structure, each node then has a void
 * pointer to either a job, job array or job group.
 */
struct jgTreeNode {
    struct jgTreeNode *parent;
    struct jgTreeNode *child;
    struct jgTreeNode *left;
    struct jgTreeNode *right;
    int  nodeType; /* jData, jarray or jgrpData */
    char *name;    /* job or group name */
    char *path;    /* full path */
    void *ndInfo;  /* node is void as it points to different objects */
};


typedef enum treeEventType {
    TREE_EVENT_ADD,
    TREE_EVENT_CLIP,
    TREE_EVENT_CTRL,
    TREE_EVENT_NULL
} TREE_EVENT_TYPE_T;


extern char treeFile[];

struct nodeList {
    int isJData;
    void *info;
};

extern struct jgTreeNode *groupRoot;

extern void               treeInit();
extern struct jgTreeNode *treeLexNext(struct jgTreeNode *);
extern struct jgTreeNode *treeLinkSibling(struct jgTreeNode *,
					  struct jgTreeNode *);
extern struct jgTreeNode *treeInsertChild(struct jgTreeNode *,
					  struct jgTreeNode *);
extern void               treeInsertLeft(struct jgTreeNode *,
					 struct jgTreeNode *);
extern void               treeInsertRight(struct jgTreeNode *,
					  struct jgTreeNode *);
extern struct jgTreeNode *treeClip(struct jgTreeNode *);
extern struct jgTreeNode *treeNewNode(int);
extern void               treeFree(struct jgTreeNode *);
extern int                isAncestor(struct jgTreeNode *,
				     struct jgTreeNode *);
extern int                isChild(struct jgTreeNode *, struct jgTreeNode *);
extern char              *parentGroup(char *);
extern char              *parentOfJob(char *);
extern void               initObj(char *, int);
extern char              *parentGroup(char *);
extern struct jgArrayBase *createJgArrayBaseRef (struct jgArrayBase *);
extern void               destroyJgArrayBaseRef(struct jgArrayBase *);
extern int                getIndexOfJStatus(int );
extern void               updJgrpCountByJStatus(struct jData *, int, int);
extern void               putOntoTree(struct jData *, int);
extern void               printTreeStruct(char *);
extern int 		  jgrpPermitOk(struct lsfAuth *,
				       struct jgTreeNode *);
extern int                selectJgrps (struct jobInfoReq *,
                                       void **, int *);
extern void               updJgrpCountByOp(struct jgTreeNode *, int);
extern char              *myName(char * );
extern void               freeJarray(struct jarray *);
extern void               checkJgrpDep(void);
extern int                inIdxList(LS_LONG_INT, struct idxList *);
extern int                matchName(char *, char *);
extern struct jgTreeNode * treeNextSib(struct jgTreeNode *);
extern char * jgrpNodeParentPath(struct jgTreeNode * );
extern char * fullJobName(struct jData *);
extern int    jgrpNodeParentPath_r(struct jgTreeNode *, char *);
extern void   fullJobName_r(struct jData *, char *);
extern int    updLocalJData(struct jData *, struct jData *);
extern int    localizeJobElement(struct jData *);
extern int    localizeJobArray(struct jData *);

#endif
