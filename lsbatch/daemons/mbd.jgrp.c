/*
 * Copyright (C) 2011-2015 David Bigagli
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

/* This module deals with job groups. Job groups are organized in nodes
 * and they live on a tree.
 *
 * This is the organization of the tree:
 *
 *   groupRoot /
 *      |
 *       \
 *         --> node -----> node --> node
 *             |            |          |
 *              \            \          \
 *               -> Group      --> Job    -> Project
 *                  |                         |
 *                   \                         \
 *                    ->Job                     -> Job
 *
 * The node itself then point to a specific data type which can
 * be a job, a job array head, a job group or a project.
 * Job groups are very useful especially because they maintain counters
 * of job statuses associated with them it is easy to add a new job
 * organization or grouping, like project for example, then put it on
 * the tree and reuse all this code to add nodes, remove them, keep
 * the counters and display them.
 *
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "mbd.h"

#include <dirent.h>

#define MAX_SPEC_LEN  200
#define SET_STATUS(x, y) ((x) == (y)) ? 0 : ((x) = (y)) == (y)

struct jgTreeNode *groupRoot;

struct jgrpInfo {
    struct nodeList  *jgrpList;
    int    numNodes;
    int    arraySize;
    char   jobName[MAX_CMD_DESC_LEN];
    struct idxList *idxList;
    char   allqueues;
    char   allusers;
    char   allhosts;
    struct gData *uGrp;
};

static void       printCounts(struct jgTreeNode *, FILE *);
static int        printNode(struct jgTreeNode *, FILE *);
static void       freeTreeNode(struct jgTreeNode *);
static int        skipJgrpByReq (int, int);
static int        storeToJgrpList(void *, struct jgrpInfo *, int);
static int        makeTreeNodeList(struct jgTreeNode *,
                                   struct jobInfoReq *,
                                   struct jgrpInfo *);
static void       freeGrpNode(struct jgrpData *);
static int isSelected ( struct jobInfoReq *,
                        struct jData *,
                        struct jgrpInfo *);
static int makeJgrpInfo(struct jgTreeNode *,
                        char *,
                        struct jobInfoReq *,
                        struct jgrpInfo *);
static bool_t isSameUser(struct lsfAuth *, int, char *);
static struct jgTreeNode *tree_new_node(struct jgTreeNode *,
                                        const char *,
                                        const char *,
                                        struct lsfAuth *,
                                        struct job_group *);

char treeFile[48];

extern float version;

#define DEFAULT_LISTSIZE    200
static hTab nodeTab;

/* treeInit()
 */
void
treeInit(void)
{
    hEnt *e;
    int mbdPid;

    mbdPid = getpid();

    groupRoot = treeNewNode(JGRP_NODE_GROUP);
    groupRoot->name = safeSave("/");
    groupRoot->path = strdup("/");
    JGRP_DATA(groupRoot)->userId  = managerId;
    JGRP_DATA(groupRoot)->userName = safeSave(lsbSys);
    JGRP_DATA(groupRoot)->status = JGRP_ACTIVE;
    JGRP_DATA(groupRoot)->numRef = 0;
    JGRP_DATA(groupRoot)->max_jobs = INT32_MAX;
    sprintf(treeFile, "/tmp/jgrpTree.%d", mbdPid);

    /* Initialize the hash table of nodes and insert
     * the root in it.
     */
    h_initTab_(&nodeTab, 111);

    e = h_addEnt_(&nodeTab, groupRoot->name, NULL);
    e->hData = (int *)groupRoot;

}

struct jgTreeNode *
treeLexNext(struct jgTreeNode *node)
{
    struct jgTreeNode *parent;

    if (node == NULL)
        return NULL;
    if (node->child)
        return(node->child) ;
    if (node->right)
        return(node->right);
    parent = node->parent ;
    while (parent) {
        if (parent->right)
            return(parent->right) ;
        parent = parent->parent;
    }
    return NULL ;
}

struct jgTreeNode *
treeNextSib(struct jgTreeNode *node)
{
    if (node == NULL)
        return NULL;
    if (node->right)
        return(node->right);
    while (node->parent) {
        node = node->parent;
        if (node->right)
            return(node->right);
    }
    return NULL;
}

struct jgTreeNode *
treeInsertChild(struct jgTreeNode *parent, struct jgTreeNode *child)
{
    if (!parent)
        return(child);

    if (!child)
        return(parent);

    if (parent->child) {
        child->right = parent->child;
        parent->child->left = child;
        parent->child = child;
    } else {
        parent->child = child;
    }

    child->parent = parent;

    if (child->nodeType == JGRP_NODE_JOB) {
        updJgrpCountByJStatus(JOB_DATA(child),
                              JOB_STAT_NULL,
                              JOB_DATA(child)->jStatus);
    } else {
        updJgrpCountByOp(child, 1);
    }

    return parent;
}

struct jgTreeNode *
treeLinkSibling(struct jgTreeNode *sib1, struct jgTreeNode *sib2)
{
    struct jgTreeNode *nPtr;

    if (!sib1)
        return(sib2);
    if (!sib2)
        return(sib1);

    for(nPtr = sib1; nPtr->right; nPtr = nPtr->right)
        ;
    nPtr->right=sib2;
    sib2->left=nPtr;

    return sib1;
}


void
treeInsertLeft(struct jgTreeNode *node, struct jgTreeNode *subtree)
{
    if (!node || !subtree)
        return;
    subtree->right = node;
    subtree->left = node->left;
    subtree->parent = node->parent;
    if (node->left)
        node->left->right = subtree;
    node->left = subtree;


    if (subtree->nodeType == JGRP_NODE_JOB)
        updJgrpCountByJStatus(JOB_DATA(subtree), JOB_STAT_NULL,
                              JOB_DATA(subtree)->jStatus);
    else
        updJgrpCountByOp(subtree, 1);
}

void
treeInsertRight(struct jgTreeNode *node, struct jgTreeNode *subtree)
{
    if (!node || !subtree)
        return;

    subtree->right = node->right;
    subtree->left  = node;
    subtree->parent = node->parent;
    if (node->right)
        node->right->left = subtree;
    node->right = subtree;


    if (subtree->nodeType == JGRP_NODE_JOB)
        updJgrpCountByJStatus(JOB_DATA(subtree), JOB_STAT_NULL,
                              JOB_DATA(subtree)->jStatus);
    else
        updJgrpCountByOp(subtree, 1);
}

struct jgTreeNode *
treeClip(struct jgTreeNode *node)
{
    if ( !node )
        return NULL;
    if (node->left)
        node->left->right = node->right;
    else if (node->parent)
        node->parent->child = node->right;
    if (node->right)
        node->right->left = node->left;


    if (node->nodeType == JGRP_NODE_JOB)
        updJgrpCountByJStatus(JOB_DATA(node), JOB_DATA(node)->jStatus,
                              JOB_STAT_NULL);
    else
        updJgrpCountByOp(node, -1);

    node->parent = node->left = node->right = NULL;

    return node;
}

/* treeNewNode()
 */
struct jgTreeNode *
treeNewNode(int type)
{
    struct jgTreeNode *node;

    node = my_calloc(1, sizeof(struct jgTreeNode), __func__);

    node->nodeType = type;
    if (type == JGRP_NODE_GROUP) {

        node->ndInfo = my_calloc(1, sizeof (struct jgrpData), __func__);

        JGRP_DATA(node)->freeJgArray = (FREE_JGARRAY_FUNC_T) freeGrpNode;
        JGRP_DATA(node)->status = JGRP_UNDEFINED;

    } else if (type == JGRP_NODE_ARRAY) {

        node->ndInfo = my_calloc(1, sizeof(struct jarray), __func__);
        ARRAY_DATA(node)->freeJgArray = (FREE_JGARRAY_FUNC_T) freeJarray;
    }

    return node;
}

void
treeFree(struct jgTreeNode * node)
{
    struct jgTreeNode *ndPtr, *ndTmp ;

    ndPtr = node;
    while (ndPtr) {
        ndTmp = ndPtr;
        ndPtr = RIGHT_SIBLING(ndPtr);
        if (FIRST_CHILD(ndTmp)) {
            treeFree(FIRST_CHILD(ndTmp));
            freeTreeNode(ndTmp);
        }
        else
            freeTreeNode(ndTmp);
    }
}

/* freeTreeNode()
 */
static void
freeTreeNode(struct jgTreeNode *node)
{
    if (!node)
        return;

    if (node->name)
        FREEUP(node->name);
    if (node->path)
        _free_(node->path);

    if (node->nodeType == JGRP_NODE_JOB) {
        DESTROY_REF(node->ndInfo, destroyjDataRef);
    } else if (node->nodeType == JGRP_NODE_ARRAY) {
        freeJarray(ARRAY_DATA(node));
    } else {
        freeGrpNode(JGRP_DATA(node));
    }

    FREEUP(node);
}

/* freeGrpNode()
 */
static void
freeGrpNode(struct jgrpData *jgrp)
{
    if (!jgrp)
        return;

    if (jgrp->userName)
        FREEUP(jgrp->userName);

    if (jgrp->numRef <= 0) {
        FREEUP(jgrp);
    } else {
        jgrp->status = JGRP_VOID;
    }
}

void
freeJarray(struct jarray *jarray)
{
    if (!jarray)
        return;

    if (jarray->userName)
        FREEUP(jarray->userName);

    DESTROY_REF(jarray->jobArray, destroyjDataRef);

    jarray->jobArray = NULL;

    if (jarray->numRef <= 0) {
        FREEUP(jarray);
    } else {
        jarray->status = JGRP_VOID;
    }
}

int
isAncestor(struct jgTreeNode *x, struct jgTreeNode *y)
{
    struct jgTreeNode *ndPtr;

    if (!x || !y)
        return false;

    for (ndPtr = y->parent; ndPtr; ndPtr = ndPtr->parent)
        if (ndPtr == x)
            return true;

    return false;
}

int
isChild(struct jgTreeNode *x, struct jgTreeNode *y)
{
    struct jgTreeNode *ndPtr;

    if (!x || !y)
        return false;

    for (ndPtr = FIRST_CHILD(x); ndPtr; ndPtr = RIGHT_SIBLING(ndPtr))
        if (ndPtr == y)
            return true;

    return false;
}

struct jgTreeNode *
findTreeNode(struct jgTreeNode *node, char *name)
{
    struct jgTreeNode  *nPtr;

    if (!node)
        return NULL;

    for (nPtr = node; nPtr;   nPtr = nPtr->right) {
        if (strcmp(name, nPtr->name) == 0)
            return(nPtr);
    }
    return NULL;
}

char *
parentGroup(char * group_spec)
{
    static char parentStr[MAXPATHLEN];
    int i;

    parentStr[0] = '\0';
    if (!group_spec){
        lsberrno = LSBE_JGRP_BAD;
        return parentStr;
    }

    if (strlen(group_spec) >= MAXPATHLEN) {
        lsberrno = LSBE_NO_MEM;
        return parentStr;
    }

    strcpy(parentStr, group_spec);

    for (i = strlen(parentStr)-1; (i >= 0) && (parentStr[i] == '/'); i--);

    if (i < 0)  {
        lsberrno = LSBE_JGRP_NULL;
        parentStr[0] = '\0';
        return parentStr;
    }


    for (; (i >= 0) && (parentStr[i] != '/'); i--);

    parentStr[i+1] = '\0';

    return parentStr;
}

char *
parentOfJob(char * group_spec)
{
    static char parentStr[MAXPATHLEN];
    int    i;

    if (!group_spec){
        lsberrno = LSBE_JGRP_NULL;
        return NULL;
    }

    if (strlen(group_spec) >= MAXPATHLEN) {
        lsberrno = LSBE_NO_MEM;
        return NULL;
    }

    strcpy(parentStr, group_spec);


    for (i = strlen(parentStr)-1; (i >= 0) && (parentStr[i] != '/'); i--);

    parentStr[i+1] = '\0';

    return parentStr;
}


char *
myName(char * group_spec)
{
    static char myStr[MAXPATHLEN];
    int    i;

    myStr[0] = '\0';
    if (!group_spec){
        lsberrno = LSBE_JGRP_BAD;
        return(myStr);
    }

    if (strlen(group_spec) >= MAXPATHLEN) {
        lsberrno = LSBE_NO_MEM;
        return(myStr);
    }

    strcpy(myStr, group_spec);


    for (i = strlen(myStr)-1; (i >= 0) && (myStr[i] == '/'); i--);

    myStr[i+1] = '\0';
    if (i < 0)  {
        lsberrno = LSBE_JGRP_BAD;
        return(myStr);
    }


    for (; (i >= 0) && (myStr[i] != '/'); i--);

    return &myStr[i+1];
}


char *
jgrpNodeParentPath(struct jgTreeNode * jgrpNode)
{
    static char fullPath[MAXPATHLEN];
    static char oldPath[MAXPATHLEN];
    struct jgTreeNode * jgrpPtr;
    int first = TRUE;

    if (jgrpNode == NULL) {
        lsberrno = LSBE_JGRP_NULL;
        return NULL;
    }

    jgrpPtr = jgrpNode->parent;

    fullPath[0] = '\0';

    while (jgrpPtr) {
        strcpy(oldPath, fullPath);

        if (!strcmp(jgrpPtr->name,"/"))
            sprintf(fullPath,"/%s", oldPath);
        else {
            if (first == TRUE) {
                sprintf(fullPath,"%s", jgrpPtr->name);
                first = FALSE;
            }
            else
                sprintf(fullPath,"%s/%s", jgrpPtr->name, oldPath);

        }
        jgrpPtr = jgrpPtr->parent;
    }

    return fullPath;
}

int
jgrpNodeParentPath_r(struct jgTreeNode * jgrpNode, char *fullPath)
{
    char oldPath[MAXPATHLEN];
    struct jgTreeNode * jgrpPtr;
    int first = TRUE;

    if (jgrpNode == NULL || fullPath == NULL) {
        lsberrno = LSBE_JGRP_NULL;
        return -1;
    }

    jgrpPtr = jgrpNode->parent;

    fullPath[0] = '\0';

    while (jgrpPtr) {
        strcpy(oldPath, fullPath);

        if (!strcmp(jgrpPtr->name,"/"))
            sprintf(fullPath,"/%s", oldPath);
        else {
            if (first == TRUE) {
                sprintf(fullPath,"%s", jgrpPtr->name);
                first = FALSE;
            }
            else
                sprintf(fullPath,"%s/%s", jgrpPtr->name, oldPath);

        }
        jgrpPtr = jgrpPtr->parent;
    }
    return 0;
}


void
printSubtree(struct jgTreeNode *root, FILE *out_file, int indent)
{

    int    len, i;
    char   space[MAX_SPEC_LEN];
    struct jgTreeNode *gPtr;

    if (!root)
        return;
    len = printNode(root, out_file);
    space[0]='\0' ;
    for (i = 1; i <= (indent+len); i++)
        strcat(space, " ") ;

    for (gPtr = root->child; gPtr; gPtr = gPtr->right) {
        printSubtree(gPtr, out_file, indent+len);
        if (gPtr->right)
            fprintf(out_file, "\n%s", space);
    }
}

void
printTreeStruct(char *fileName)
{
    static char fname[] = "printTreeStruct";
    FILE   *out_file , *fopen() ;

    if ((out_file = fopen(fileName, "w")) == NULL) {
        ls_syslog(LOG_ERR, "%s: can't open file %s: %m", fname, fileName);
        return;
    }
    fprintf(out_file, "***********************************\n");
    fprintf(out_file, "*        Job Group Tree           *\n");
    fprintf(out_file, "***********************************\n");
    printSubtree(groupRoot, out_file, 0);
    fprintf(out_file, "\n");
    printCounts(groupRoot, out_file);
    _fclose_(&out_file);
}

static int
printNode(struct jgTreeNode *root, FILE *out_file)
{
    char ss[MAX_SPEC_LEN];
    if (root->nodeType == JGRP_NODE_GROUP) {
        sprintf(ss, "%s(Root)", root->name);
    }
    else if (root->nodeType == JGRP_NODE_JOB) {
        sprintf(ss, "%s(J)", root->name);
    }
    else if ( root->nodeType == JGRP_NODE_ARRAY) {
        sprintf(ss, "%s(V)", root->name);
    }
    else
        sprintf(ss, "<%lx>UNDEF:", (long)root);
    fprintf(out_file, "%s", ss) ;
    if (root->child!=0){
        fprintf(out_file, "->") ;
        return(strlen(ss)+2);
    }
    else
        return(strlen(ss)) ;
}

static void
printCounts(struct jgTreeNode *root, FILE *out_file)
{
    int i;

    fprintf(out_file, "************** COUNTS ******************\n");
    fprintf(out_file, "\n");
    while (root) {
        if (root->nodeType == JGRP_NODE_GROUP) {
            fprintf(out_file, "GROUP:%s\n", root->name);
            for (i = 0; i < NUM_JGRP_COUNTERS; i++)
                fprintf(out_file, "%d   ", JGRP_DATA(root)->counts[i]);
            fprintf(out_file, "\n");

        }
        else if (root->nodeType == JGRP_NODE_ARRAY) {
            fprintf(out_file, "ARRAY:%s\n", root->name);
            for (i=0; i < NUM_JGRP_COUNTERS; i++)
                fprintf(out_file, "%d   ", ARRAY_DATA(root)->counts[i]);
            fprintf(out_file, "\n");
        }

        root = treeLexNext(root);
    }
}

void
putOntoTree(struct jData *jp, int jobType)
{
    static struct jgTreeNode *newj;
    struct jgTreeNode  *parentNode;
    struct jData *jPtr;
    hEnt *e;

    parentNode = groupRoot;

    newj = treeNewNode(jp->nodeType);

    if (jp->shared->jobBill.options & SUB_JOB_NAME)
        newj->name = safeSave(jp->shared->jobBill.jobName);
    else
        newj->name = safeSave(jp->shared->jobBill.command);

    if (jp->nodeType == JGRP_NODE_JOB)
        newj->ndInfo = (void *)createjDataRef(jp);
    else {
        ARRAY_DATA(newj)->jobArray = createjDataRef(jp);
        ARRAY_DATA(newj)->userId = jp->userId;
        ARRAY_DATA(newj)->userName = safeSave (jp->userName);

        if (jp->shared->jobBill.options2 & SUB2_HOST_NT)
            ARRAY_DATA(newj)->fromPlatform = AUTH_HOST_NT;
        else if (jp->shared->jobBill.options2 & SUB2_HOST_UX)
            ARRAY_DATA(newj)->fromPlatform = AUTH_HOST_UX;
    }

    jp->jgrpNode = newj;

    for (jPtr = jp->nextJob; jPtr; jPtr = jPtr->nextJob) {
        jPtr->jgrpNode = newj;

        updJgrpCountByJStatus(jPtr, JOB_STAT_NULL, jPtr->jStatus);
    }

    /* If the job specifies a group get its node
     */
    if (jp->shared->jobBill.options2 & SUB2_JOB_GROUP) {
        e = h_getEnt_(&nodeTab, jp->shared->jobBill.job_group);
        parentNode = e->hData;
    }

    treeInsertChild(parentNode, newj);

    jp->runCount = 1;

    if (jp->newReason == 0)
        jp->newReason = PEND_JOB_NEW;
    if (jp->shared->dptRoot == NULL) {
        /* If the job depends on some other
         * job we cannot declare it ready here
         * as it has to go thru the checkJgrpDep()
         * function to determine its readiness.
         */
        jp->jFlags |= JFLAG_READY2;
    }

    if (logclass & LC_JGRP)
        printTreeStruct(treeFile);
}


struct jgArrayBase *
createJgArrayBaseRef(struct jgArrayBase *jgArrayBase)
{
    if (jgArrayBase) {
        jgArrayBase->numRef++;
    }
    return jgArrayBase;
}

void
destroyJgArrayBaseRef(struct jgArrayBase *jgArrayBase)
{
    if (jgArrayBase) {
        jgArrayBase->numRef--;
        if ((jgArrayBase->status == JGRP_VOID)
            && (jgArrayBase->numRef <= 0)) {

            (* jgArrayBase->freeJgArray)((char *)jgArrayBase);
        }
    }
}

/* CheckJgrpDep()
 */
void
checkJgrpDep(void)
{
    struct jgTreeNode *nPtr;

    nPtr = groupRoot;

    if (logclass & LC_JGRP) {
        ls_syslog(LOG_INFO, "\
%s: Entering checkJgrpDep at node <%s%s>;",
                  __func__, jgrpNodeParentPath(nPtr), nPtr->name);
    }

    while (nPtr) {

        if (logclass & LC_JGRP) {
            ls_syslog(LOG_INFO, "\
%s: node %s %d", __func__, nPtr->name, nPtr->nodeType);
        }

        switch (nPtr->nodeType) {

            case JGRP_NODE_GROUP:
                break;

            case JGRP_NODE_JOB:
            case JGRP_NODE_ARRAY: {
                struct jData * jpbw;

                if (nPtr->nodeType == JGRP_NODE_JOB)
                    jpbw = JOB_DATA(nPtr);
                else
                    jpbw = ARRAY_DATA(nPtr)->jobArray->nextJob;

                for (; jpbw; jpbw = jpbw->nextJob) {
                    int depCond;

                    if (!JOB_PEND(jpbw)) {
                        continue;
                    }

                    jpbw->jFlags &= ~(JFLAG_READY1 | JFLAG_READY2);
                    jpbw->jFlags |= JFLAG_READY1;

                    if (jpbw->jFlags & JFLAG_WAIT_SWITCH) {
                        jpbw->newReason = PEND_JOB_SWITCH;
                        continue;
                    }

                    if (!jpbw->shared->dptRoot) {
                        jpbw->jFlags |= JFLAG_READY2;
                        continue;
                    }

                    depCond = evalDepCond(jpbw->shared->dptRoot,
                                          jpbw,
                                          NULL);
                    if (depCond == DP_FALSE) {
                        jpbw->newReason = PEND_JOB_DEPEND;
                    }
                    else if (depCond == DP_INVALID) {

                        jpbw->newReason = PEND_JOB_DEP_INVALID;
                        jpbw->jFlags |= JFLAG_DEPCOND_INVALID;
                    }
                    else if (depCond == DP_REJECT) {
                        jpbw->newReason = PEND_JOB_DEP_REJECT;
                        jpbw->jFlags |= JFLAG_DEPCOND_REJECT;
                    }
                    if (depCond == DP_TRUE) {
                        jpbw->jFlags |= JFLAG_READY2;
                        continue;
                    }
                } /* for (; jpbw; jpbw->nextJob) */
                break;
            }
        }
        nPtr = treeLexNext(nPtr);

    } /* while (nPtr) */
}

void
updJgrpCountByJStatus(struct jData *job, int oldStatus, int newStatus)
{
    struct jgTreeNode *gPtr = job->jgrpNode;

    while (gPtr) {
        if (gPtr->nodeType == JGRP_NODE_GROUP) {
            if (oldStatus != JOB_STAT_NULL) {
                JGRP_DATA(gPtr)->counts[getIndexOfJStatus(oldStatus)] -= 1;
                JGRP_DATA(gPtr)->counts[JGRP_COUNT_NJOBS] -= 1;
            }
            if (newStatus != JOB_STAT_NULL) {
                JGRP_DATA(gPtr)->counts[getIndexOfJStatus(newStatus)] += 1;
                JGRP_DATA(gPtr)->counts[JGRP_COUNT_NJOBS] += 1;
            }
        }
        else if (gPtr->nodeType == JGRP_NODE_ARRAY) {
            if (oldStatus != JOB_STAT_NULL) {
                ARRAY_DATA(gPtr)->counts[getIndexOfJStatus(oldStatus)] -= 1;
                ARRAY_DATA(gPtr)->counts[JGRP_COUNT_NJOBS] -= 1;
            }
            if (newStatus != JOB_STAT_NULL) {
                ARRAY_DATA(gPtr)->counts[getIndexOfJStatus(newStatus)] += 1;
                ARRAY_DATA(gPtr)->counts[JGRP_COUNT_NJOBS] += 1;
            }
        }
        gPtr = gPtr->parent;
    }
    if (logclass & LC_JGRP)
        printTreeStruct(treeFile);
}


int
getIndexOfJStatus(int status)
{
    switch (MASK_STATUS(status & ~JOB_STAT_UNKWN
                        & ~JOB_STAT_PDONE & ~JOB_STAT_PERR)) {
        case JOB_STAT_PEND:
        case JOB_STAT_RUN|JOB_STAT_WAIT:
            return JGRP_COUNT_PEND;
        case JOB_STAT_PSUSP:
            return JGRP_COUNT_NPSUSP;
        case JOB_STAT_RUN :
            return JGRP_COUNT_NRUN;
        case JOB_STAT_SSUSP:
            return JGRP_COUNT_NSSUSP;
        case JOB_STAT_USUSP:
            return JGRP_COUNT_NUSUSP;
        case JOB_STAT_EXIT:
        case JOB_STAT_EXIT|JOB_STAT_WAIT:
            return JGRP_COUNT_NEXIT;
        case JOB_STAT_DONE:
        case JOB_STAT_DONE|JOB_STAT_WAIT:
            return JGRP_COUNT_NDONE;
        default:
            ls_syslog(LOG_ERR, "\
%s: job status <%d> out of bound", __func__,
                      MASK_STATUS(status));
            return 8;
    }
}

void
updJgrpCountByOp(struct jgTreeNode *jgrp, int factor)
{
    struct jgTreeNode *parent;
    int i;

    for (parent = jgrp->parent; parent; parent = parent->parent) {
        for (i = 0; i < NUM_JGRP_COUNTERS; i++)
            if (jgrp->nodeType == JGRP_NODE_GROUP) {
                JGRP_DATA(parent)->counts[i] += factor*JGRP_DATA(jgrp)->counts[i];
            } else if (jgrp->nodeType == JGRP_NODE_ARRAY) {
                JGRP_DATA(parent)->counts[i] += factor*ARRAY_DATA(jgrp)->counts[i];
            }
    }
}


void
resetJgrpCount(void)
{
    struct jgTreeNode *jgrp;
    struct jData      *jPtr;
    int    i;

    for (jgrp = groupRoot; jgrp; jgrp = treeLexNext(jgrp)) {
        if (jgrp->nodeType == JGRP_NODE_GROUP) {
            for (i=0; i < NUM_JGRP_COUNTERS; i++)
                JGRP_DATA(jgrp)->counts[i] = 0;
        }
        else if (jgrp->nodeType == JGRP_NODE_ARRAY) {
            for (jPtr = ARRAY_DATA(jgrp)->jobArray->nextJob; jPtr;
                 jPtr = jPtr->nextJob)
                updJgrpCountByJStatus(jPtr, JOB_STAT_NULL, jPtr->jStatus);
        }
        else if (jgrp->nodeType == JGRP_NODE_JOB)
            updJgrpCountByJStatus(JOB_DATA(jgrp), JOB_STAT_NULL,
                                  JOB_DATA(jgrp)->jStatus);
    }
}

static bool_t
isSameUser(struct lsfAuth *auth, int userId, char *userName)
{

    return (strcmp(auth->lsfUserName, userName) == 0);
}

int
jgrpPermitOk(struct lsfAuth *auth, struct jgTreeNode *jgrp)
{
    struct jgTreeNode *nPtr;

    if (!jgrp)
        return false;

    if (mSchedStage == M_STAGE_REPLAY)
        return true;

    if (auth->uid == 0 || isAuthManager(auth)) {
        return TRUE;
    }

    for (nPtr = jgrp; nPtr; nPtr = nPtr->parent) {
        if (nPtr->nodeType == JGRP_NODE_GROUP) {
            if (isSameUser(auth, JGRP_DATA(nPtr)->userId,
                           JGRP_DATA(nPtr)->userName))
                return TRUE;
        }
        if (nPtr->nodeType == JGRP_NODE_ARRAY) {
            if (isSameUser(auth, ARRAY_DATA(nPtr)->userId,
                           ARRAY_DATA(nPtr)->userName))
                return TRUE;
        }
        if (nPtr->nodeType == JGRP_NODE_JOB) {
            if (JOB_DATA(nPtr)->shared->jobBill.options2
                & SUB2_HOST_NT){
                if (isSameUser(auth, JOB_DATA(nPtr)->userId,
                               JOB_DATA(nPtr)->userName))
                    return TRUE;
            } else if (JOB_DATA(nPtr)->shared->jobBill.options2
                       & SUB2_HOST_UX){
                if (isSameUser(auth, JOB_DATA(nPtr)->userId,
                               JOB_DATA(nPtr)->userName))
                    return TRUE;
            } else
                if (isSameUser(auth, JOB_DATA(nPtr)->userId,
                               JOB_DATA(nPtr)->userName))
                    return TRUE;
        }
    }

    return false;
}

bool_t
isJobOwner(struct lsfAuth *auth, struct jData *job)
{

    if (job->shared->jobBill.options2 & SUB2_HOST_UX)
        return (isSameUser(auth, job->userId, job->userName));
    else  if(job->shared->jobBill.options2 & SUB2_HOST_NT)
        return (isSameUser(auth, job->userId, job->userName));
    else
        return (isSameUser(auth, job->userId, job->userName));
}


int
selectJgrps(struct jobInfoReq *jobInfoReq, void **jgList, int *listSize)
{
    struct jgTreeNode  *parent;
    int retError = 0;
    struct jgrpInfo  jgrp;
    char   jobName[MAX_CMD_DESC_LEN];

    jgrp.allqueues = jgrp.allusers =  jgrp.allhosts = FALSE;
    jgrp.uGrp = NULL;
    jgrp.jgrpList = NULL;
    jgrp.numNodes = 0;
    jgrp.arraySize = 0;
    jobName[0] = '\0';

    *jgList = NULL;
    *listSize = 0;

    memset(jobName, 0, MAX_CMD_DESC_LEN);

    if (jobInfoReq->queue[0] == '\0')
        jgrp.allqueues = TRUE;

    if (strcmp(jobInfoReq->userName, ALL_USERS) == 0)
        jgrp.allusers = TRUE;
    else
        jgrp.uGrp = getUGrpData (jobInfoReq->userName);

    if (jobInfoReq->host[0] == '\0')
        jgrp.allhosts = TRUE;

    if (jobInfoReq->jobId != 0 && (jobInfoReq->options & JGRP_ARRAY_INFO)) {
        struct jData *jp;

        if ((jp = getJobData(jobInfoReq->jobId)) == NULL ||
            jp->nodeType != JGRP_NODE_ARRAY)
            goto ret;

        if (!storeToJgrpList((void *)ARRAY_DATA(jp->jgrpNode)->jobArray,
                             &jgrp, JGRP_NODE_JOB))
            return LSBE_NO_MEM;
        goto ret;
    }

    parent = groupRoot;
    if ((strlen(jobInfoReq->jobName) == 1) && (jobInfoReq->jobName[0] == '/'))
        strcpy(jobName, "*");
    else
        ls_strcat(jobName,sizeof(jobName),jobInfoReq->jobName);
    if ((retError = makeJgrpInfo(parent, jobName, jobInfoReq, &jgrp))
        != LSBE_NO_ERROR) {
        return (retError);
    }

    goto ret;
ret:
    if (jgrp.numNodes > 0) {
        *jgList =  (void *)jgrp.jgrpList;
        *listSize = jgrp.numNodes;
        return LSBE_NO_ERROR;
    }

    return LSBE_NO_JOB;
}

static int
makeJgrpInfo(struct jgTreeNode *parent,
             char *jobName,
             struct jobInfoReq *jobInfoReq,
             struct jgrpInfo *jgrp)
{
    int maxJLimit = 0;
    int retError;

    if ((jgrp->idxList =
         parseJobArrayIndex(jobName, &retError, &maxJLimit))
        == NULL) {
        if (retError != LSBE_NO_ERROR)
            return(retError);
    }

    if (!parent)
        return(LSBE_NO_JOB);

    strcpy(jgrp->jobName, jobName);
    if ((retError = makeTreeNodeList(parent, jobInfoReq, jgrp))
        != LSBE_NO_ERROR)
        return(retError);

    return LSBE_NO_ERROR;
}


static int
makeTreeNodeList(struct jgTreeNode *parent,
                 struct jobInfoReq *jobInfoReq,
                 struct jgrpInfo *jgrp)
{
    struct jgTreeNode *nPtr;

    if (!parent)
        return (LSBE_NO_ERROR);


    for (nPtr = parent->child; nPtr; nPtr = nPtr->right){
        if (strlen(jgrp->jobName) && !matchName(jgrp->jobName, nPtr->name))
            continue;
        switch (nPtr->nodeType) {
            case JGRP_NODE_ARRAY:{
                struct jData *jpbw;
                if (jobInfoReq->options & JGRP_ARRAY_INFO) {
                    if (isSelected(jobInfoReq, ARRAY_DATA(nPtr)->jobArray,
                                   jgrp)
                        && (!storeToJgrpList(
                                (void *)ARRAY_DATA(nPtr)->jobArray,
                                jgrp, JGRP_NODE_JOB)))
                        return LSBE_NO_MEM;
                }
                else {
                    jpbw = ARRAY_DATA(nPtr)->jobArray->nextJob;
                    for ( ; jpbw; jpbw = jpbw->nextJob) {
                        if (isSelected(jobInfoReq, jpbw, jgrp)) {
                            if (!storeToJgrpList((void *)jpbw, jgrp,
                                                 JGRP_NODE_JOB))
                                return LSBE_NO_MEM;
                        }
                    }
                }
                break;
            }
            case JGRP_NODE_JOB:  {
                struct jData *jpbw;

                if (jobInfoReq->options & JGRP_ARRAY_INFO)
                    continue;

                jpbw = JOB_DATA(nPtr);
                if (isSelected(jobInfoReq, jpbw, jgrp)) {
                    if (!storeToJgrpList((void *)jpbw, jgrp, JGRP_NODE_JOB))
                        return LSBE_NO_MEM;
                }
                break;
            }
        }
    }

    return LSBE_NO_ERROR;
}

static int
skipJgrpByReq (int options, int jStatus)
{
    if (options & (ALL_JOB|JOBID_ONLY_ALL | JOBID_ONLY)) {
        return FALSE;
    }
    else if ((options & (CUR_JOB | LAST_JOB))
             && (IS_START(jStatus) || IS_PEND(jStatus))) {
        return FALSE;
    }
    else if ((options & PEND_JOB) && IS_PEND(jStatus)) {
        return FALSE;
    }
    else if ((options & (SUSP_JOB | RUN_JOB)) && IS_START(jStatus)) {
        return FALSE;
    }
    else if ((options & DONE_JOB) && IS_FINISH(jStatus)) {
        return FALSE;
    }
    return true;
}

static int
isSelected(struct jobInfoReq *jobInfoReq, struct jData *jpbw,
           struct jgrpInfo *jgrp)
{
    static char fname[] = "isSelected()";
    int i;
    char allqueues = jgrp->allqueues;
    char allusers = jgrp->allusers;
    char allhosts = jgrp->allhosts;

    if (skipJgrpByReq (jobInfoReq->options, jpbw->jStatus))
        return false;

    if (!allqueues && strcmp(jpbw->qPtr->queue, jobInfoReq->queue) != 0)
        return false;

    if (!allusers && strcmp(jpbw->userName, jobInfoReq->userName)) {
        if (jgrp->uGrp == NULL)
            return false;
        else if (!gMember(jpbw->userName, jgrp->uGrp))
            return false;
    }

    if (!allhosts) {
        struct gData *gp;
        if (IS_PEND (jpbw->jStatus))
            return false;

        if (jpbw->hPtr == NULL) {
            if (!(jpbw->jStatus & JOB_STAT_EXIT))
                ls_syslog(LOG_ERR, "\
%s: Execution host for job <%s> is null",
                          fname, lsb_jobid2str(jpbw->jobId));
            return false;
        }
        gp = getHGrpData(jobInfoReq->host);
        if (gp != NULL) {
            for (i = 0; i < jpbw->numHostPtr; i++) {
                if (jpbw->hPtr[i] == NULL)
                    return false;
                if (gMember(jpbw->hPtr[i]->host, gp))
                    break;
            }
            if (i >= jpbw->numHostPtr)
                return false;
        }
        else {
            for (i = 0; i < jpbw->numHostPtr; i++) {
                if (jpbw->hPtr[i] == NULL)
                    return false;
                if (equalHost_(jobInfoReq->host, jpbw->hPtr[i]->host))
                    break;
            }
            if (i >= jpbw->numHostPtr)
                return false;
        }
    }

    if (jgrp->idxList) {
        struct idxList *idx;
        for (idx = jgrp->idxList; idx; idx = idx->next) {
            if (LSB_ARRAY_IDX(jpbw->jobId) < idx->start ||
                LSB_ARRAY_IDX(jpbw->jobId) > idx->end)
                continue;
            if (((LSB_ARRAY_IDX(jpbw->jobId)-idx->start) % idx->step) == 0)
                return true;
        }
        return false;
    }
    else
        return true;
}

static int
storeToJgrpList(void *ptr, struct jgrpInfo *jgrp, int type)
{
    if (jgrp->arraySize == 0) {
        jgrp->arraySize = DEFAULT_LISTSIZE;
        jgrp->jgrpList = (struct  nodeList*)
            calloc (jgrp->arraySize, sizeof (struct  nodeList));
        if (jgrp->jgrpList == NULL)
            return FALSE;
    }
    if (jgrp->numNodes >= jgrp->arraySize) {

        struct  nodeList *biglist;
        jgrp->arraySize *= 2;
        biglist = realloc((char *)jgrp->jgrpList,
                          jgrp->arraySize * sizeof (struct  nodeList));
        if (biglist == NULL) {
            FREEUP(jgrp->jgrpList);
            jgrp->numNodes = 0;
            return FALSE;
        }
        jgrp->jgrpList = biglist;
    }
    jgrp->jgrpList[jgrp->numNodes].info = ptr;
    if (type == JGRP_NODE_JOB)
        jgrp->jgrpList[jgrp->numNodes++].isJData = TRUE;
    else
        jgrp->jgrpList[jgrp->numNodes++].isJData = FALSE;
    return true;
}

char *
fullJobName(struct jData *jp)
{
    static char jobName[MAXPATHLEN];

    if (jp->jgrpNode) {
        sprintf(jobName, "%s", jp->jgrpNode->name);
    }
    else
        jobName[0] = '\0';

    return jobName;
}

void
fullJobName_r(struct jData *jp, char *jobName)
{
    if (jobName == NULL || jp == NULL) {
        return;
    }
    if (jp->jgrpNode) {
        sprintf(jobName, "%s", jp->jgrpNode->name);
    }
    else
        jobName[0] = '\0';

    return;
}

/*
 * add_job_group()
 *
 */
int
add_job_group(struct job_group *jgPtr, struct lsfAuth *auth)
{
    static char path[PATH_MAX];
    char *node;
    char *name;
    char *p0;
    struct jgTreeNode *parent;
    struct jgTreeNode *jgrp;
    hEnt *e;
    int l;

    if (! jgPtr)
        return LSBE_JGRP_NULL;

    /* strip trailing slash just in case...
     */
    l = strlen(jgPtr->group_name);
    if (jgPtr->group_name[l - 1] == '/')
        jgPtr->group_name[l - 1] = 0;

    e = h_getEnt_(&nodeTab, jgPtr->group_name);
    if (e)
        return LSBE_JGRP_EXIST;

    p0 = name = strdup(jgPtr->group_name);

    /* First node
     */
    node = strtok(name, "/");
    sprintf(path, "/%s", node);
    e = h_getEnt_(&nodeTab, path);
    if (e == NULL) {
        jgrp = tree_new_node(groupRoot, node, path, auth, jgPtr);
    } else {
        jgrp = e->hData;
    }
    parent = jgrp;

    /* check for more nodes like mkdir -p
     */
    while ((node = strtok(NULL, "/"))) {
        sprintf(path + strlen(path), "/%s", node);
        e = h_getEnt_(&nodeTab, path);
        if (e == NULL) {
            jgrp = tree_new_node(parent, node, path, auth, jgPtr);
            if (jgrp == NULL) {
                ls_syslog(LOG_ERR, "\
%s: error while adding node %s path %s to parent %s", __func__,
                          node, path, parent->name);
                    return LSBE_JGRP_LIMIT;
            }
        } else {
            jgrp = e->hData;
        }
        parent = jgrp;
    }
    free(p0);

    return LSBE_NO_ERROR;
}

/* encode_nodes()
 */
int
encode_nodes(XDR *xdrs,
             int *num,
             int type,
             struct LSFHeader *hdr)
{
    struct jgTreeNode *n;
    link_t *stack;
    int cc;

    *num = 0;
    if (groupRoot->child == NULL)
        return 0;

    n = groupRoot->child;
    stack = make_link();

  again:
    while (n) {
        int i;
        struct jgrpData *g;

        if (logclass & LC_JGRP) {
            ls_syslog(LOG_DEBUG, "\
%s: node %s %s %d", __func__, n->name, n->path, type);
        }

        if (n->nodeType != type)
            goto next;

        if (n->child)
            push_link(stack, n->child);

        cc = xdr_wrapstring(xdrs, &n->path);
        if (cc != TRUE)
            goto bail;

        cc = xdr_wrapstring(xdrs, &n->name);
        if (cc != TRUE)
            goto bail;

        g = (struct jgrpData *)n->ndInfo;
        for (i = 0; i < NUM_JGRP_COUNTERS; i++) {
            cc = xdr_int(xdrs, &g->counts[i]);
            if (cc != TRUE)
                goto bail;
        }

        cc = xdr_int(xdrs, &g->max_jobs);
        if (cc != true)
            goto bail;

        ++(*num);

      next:
        n = n->right;
    }

    n = pop_link(stack);
    if (n)
        goto again;

    fin_link(stack);

    return 0;

bail:

    ls_syslog(LOG_ERR, "%s: cannot encode node %s %s %d",
              __func__, n->name, n->path, n->nodeType);
    fin_link(stack);

    return -1;
}

/* tree_size()
 */
int
tree_size(int *num)
{
    struct jgTreeNode *n;
    link_t *stack;
    int cc;

    *num = 0;
    if (groupRoot->child == NULL)
        return sizeof(int);

    n = groupRoot->child;
    stack = make_link();
    cc = 0;

  again:
    while (n) {

        if (logclass & LC_JGRP) {
            ls_syslog(LOG_DEBUG, "\
%s: node %s %s %d", __func__, n->name, n->path, n->nodeType);
        }

        if (n->nodeType != JGRP_NODE_GROUP)
            goto next;

        if (n->child)
            push_link(stack, n->child);

        cc = cc + ALIGNWORD_(strlen(n->path))
            + ALIGNWORD_(strlen(n->name))
            + NUM_JGRP_COUNTERS * sizeof(int) + sizeof(int);

        (*num)++;
      next:
        n = n->right;
    }

    n = pop_link(stack);
    if (n)
        goto again;

    fin_link(stack);

    return cc;

}

/* del_job_group()
 */
int
del_job_group(struct job_group *jgPtr, struct lsfAuth *auth)
{
    struct jgTreeNode *jgrp;
    struct jgrpData *jSpec;
    hEnt *e;
    int i;

    /* Here we expect a full path to the job to be
     * removed. /x/y/z and then z is removed
     */
    e = h_getEnt_(&nodeTab, jgPtr->group_name);
    if (e == NULL) {
        ls_syslog(LOG_ERR, "\
%s: requested group %s does not exist", __func__, jgPtr->group_name);
        return LSBE_JGRP_BAD;
    }

    jgrp = e->hData;
    if (jgrp->child)
        return LSBE_JGRP_NOTEMPTY;

    /* Check the authentication, manager can zap
     * any jobgroup.
     */
    for (i = 0; i < nManagers; i++) {
        if (managerIds[i] == auth->uid)
            goto zapit;
    }

    jSpec = (struct jgrpData *)jgrp->ndInfo;
    if (auth->uid != jSpec->userId) {
        ls_syslog(LOG_ERR, "\
%s: user %s cannot delete group %s of user %s", __func__, auth->lsfUserName,
                  jgPtr->group_name, jSpec->userName);
        return LSBE_PERMISSION;
    }

zapit:
    /* log if not replaying
     */
    if (mSchedStage != M_STAGE_REPLAY) {
        log_deljgrp(jgrp);
    }

    h_rmEnt_(&nodeTab, e);

    /* remove from the tree
     */
    treeClip(jgrp);
    freeTreeNode(jgrp);

    return 0;
}

/* tree_new_node()
 */
static struct jgTreeNode *
tree_new_node(struct jgTreeNode *parent,
              const char *name,
              const char *path,
              struct lsfAuth *auth,
              struct job_group *jg)
{
    struct jgTreeNode *parent2;
    struct jgTreeNode *jgrp;
    struct jgrpData *gData;
    struct jgrpData *gData2;
    hEnt *e;

    parent2 = parent;
    jgrp = treeNewNode(JGRP_NODE_GROUP);

    gData = jgrp->ndInfo;

    /* This is a submission bsub -g
     */
    if (jg->max_jobs == -1) {
        gData2 = parent->ndInfo;
        gData->max_jobs = gData2->max_jobs;
        goto dal;
    }

    if (jg->max_jobs == INT32_MAX) {
        gData->max_jobs = jg->max_jobs;
        goto dal;
    }

    gData->max_jobs = jg->max_jobs;
    while (parent != groupRoot) {
        gData2 = parent->ndInfo;
        if (gData->max_jobs > gData2->max_jobs) {
            ls_syslog(LOG_ERR, "\
%s: max_jobs %d of group %s greater then parent %s max_jobs %d", __func__,
                      gData->max_jobs, name,
                      parent->path, gData2->max_jobs);
            freeTreeNode(jgrp);
            return NULL;
        }
        parent = parent->parent;
    }

dal:
    jgrp->name = strdup(name);
    jgrp->path = strdup(path);
    /* put me under the original parent
     */
    treeInsertChild(parent2, jgrp);
    e = h_addEnt_(&nodeTab, jgrp->path, NULL);
    e->hData = jgrp;

    gData->userId = auth->uid;
    gData->userName = safeSave(auth->lsfUserName);
    gData->status = JGRP_ACTIVE;
    gData->submit_time = time(NULL);

    /* log only if not replaying
     */
    if (mSchedStage != M_STAGE_REPLAY) {
        log_newjgrp(jgrp);
    }

    return jgrp;
}

/* can_switch_jgrp()
*/
int
can_switch_jgrp(struct jgrpLog *jgrp)
{
    if (!jgrp)
        return true;

    /* If we still have the group this means
     * we cannot switch the record out and
     * there is no remove record for this
     * group in the current lsb.events
     * file either.
     */
    if (h_getEnt_(&nodeTab, jgrp->path))
        return false;

    return true;
}

/* check_job_group()
 *
 * Make a new job group at job submission if does
 * not exist already.
 */
int
check_job_group(struct jData *jPtr, struct lsfAuth *auth)
{
    hEnt *e;
    struct job_group jgrp;

    if (! (jPtr->shared->jobBill.options2 & SUB2_JOB_GROUP))
        return LSBE_NO_ERROR;

    e = h_getEnt_(&nodeTab, jPtr->shared->jobBill.job_group);
    if (e)
        return LSBE_NO_ERROR;

    /* ah... / is missing as the first char
     */
    if (jPtr->shared->jobBill.job_group[0] != '/') {
        int l;
        char *p;

        l = strlen(jPtr->shared->jobBill.job_group);
        l = l + 2; /* / + 0 */
        p = calloc(l, sizeof(char));
        sprintf(p, "/%s", jPtr->shared->jobBill.job_group);
        _free_(jPtr->shared->jobBill.job_group);
        jPtr->shared->jobBill.job_group = p;
    }

    jgrp.group_name = jPtr->shared->jobBill.job_group;
    jgrp.max_jobs = -1;

    return add_job_group(&jgrp, auth);
}

/* jobgroup_limit_ok()
 */
bool_t
jobgroup_limit_ok(struct jData *jPtr)
{
    hEnt *ent;
    struct jgrpData *gSpec;
    struct jgTreeNode *jgptr;
    struct jgTreeNode *parent;

    if (! (jPtr->shared->jobBill.options2 & SUB2_JOB_GROUP))
        return true;

    ent = h_getEnt_(&nodeTab, jPtr->shared->jobBill.job_group);
    if (ent == NULL) {
        ls_syslog(LOG_WARNING, "\
%s: job %s has  SUB2_JOB_GROUP flag on but no entry in groupTree?",
                  __func__, lsb_jobid2str(jPtr->jobId));
        return true;
    }

    /* At each level of the tree the limit must not be reached
     */
    parent = jgptr = ent->hData;
    gSpec = (struct jgrpData *)jgptr->ndInfo;

    do {
        if ((gSpec->counts[JGRP_COUNT_NRUN]
            + gSpec->counts[JGRP_COUNT_NSSUSP]
            + gSpec->counts[JGRP_COUNT_NUSUSP]) >= gSpec->max_jobs ) {
            if (logclass & LC_JGRP) {
                ls_syslog(LOG_INFO, "\
%s: job %s reached group limit %d of parent %s", __func__,
                          lsb_jobid2str(jPtr->jobId),
                          (gSpec->counts[JGRP_COUNT_NRUN]
                           + gSpec->counts[JGRP_COUNT_NSSUSP]
                           + gSpec->counts[JGRP_COUNT_NUSUSP]),
                          jgptr->parent->name);
            }
            return false;
        }

        parent = parent->parent;
        gSpec = (struct jgrpData *)parent->ndInfo;

    } while (parent != groupRoot);

    return true;
}

/* modify_job_group()
 */
int
modify_job_group(struct job_group *jgPtr, struct lsfAuth *auth)
{
    struct jgTreeNode *jgrp;
    struct jgTreeNode *parent;
    struct jgTreeNode *child;
    struct jgrpData *jSpec;
    struct jgrpData *jSpec2;
    hEnt *e;
    int max_jobs;

    /* We always expect a full path
     */
    e = h_getEnt_(&nodeTab, jgPtr->group_name);
    if (e == NULL) {
        ls_syslog(LOG_ERR, "\
%s: requested group %s does not exist", __func__, jgPtr->group_name);
        return LSBE_JGRP_BAD;
    }

    jgrp = e->hData;
    jSpec = (struct jgrpData *)jgrp->ndInfo;

    if (auth) {
        if (auth->uid != 0
            && !isAuthManager(auth)) {
            ls_syslog(LOG_ERR, "\
%s: user %s id %d cannot modify job group owned by user %s id %d", __func__,
                      auth->lsfUserName, auth->uid,
                      jSpec->userName, jSpec->userId);
            return LSBE_PERMISSION;
        }
    }

    max_jobs = jgPtr->max_jobs;

    /* Undo the limit.
     */
    if (max_jobs == INT32_MAX) {
        if (jgrp->parent != groupRoot) {
            parent = jgrp->parent;
            jSpec2 = (struct jgrpData *)parent->ndInfo;
            max_jobs = jSpec2->max_jobs;
        }
        goto bez;
    }

    /* I cannot be bigger than any of my parents
     */
    parent = jgrp->parent;
    while (parent != groupRoot) {

        jSpec2 = (struct jgrpData *)parent->ndInfo;
        if (max_jobs > jSpec2->max_jobs) {
            ls_syslog(LOG_ERR, "\
%s: parent %s job_limit %d smaller than mine %s %d", __func__,
                      parent->name, jSpec2->max_jobs,
                      jgrp->name, max_jobs);
            return LSBE_JGRP_LIMIT;
        }
        parent = parent->parent;
    }

    /* Any children cannot be bigger than me
     */
    child = jgrp->child;
    while (child) {
        jSpec2 = (struct jgrpData *)child->ndInfo;
        if (max_jobs < jSpec2->max_jobs) {
            ls_syslog(LOG_ERR, "\
%s: child %s job_limit %d bigger than mine %s %d", __func__,
                      child->name, jSpec2->max_jobs,
                      jgrp->name, max_jobs);
            return LSBE_JGRP_LIMIT;
        }
        child = child->child;
    }

bez:

    if (logclass & LC_JGRP) {
        ls_syslog(LOG_INFO, "\
%s: changing job group %s max_job limit from %d to %d", __func__,
                  jgrp->name, jSpec->max_jobs, max_jobs);
    }

    jSpec->max_jobs = max_jobs;

    if (mSchedStage != M_STAGE_REPLAY) {
        log_modjgrp(jgrp);
    }

    return LSBE_NO_ERROR;
}
