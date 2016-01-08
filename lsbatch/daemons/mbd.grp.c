/*
 * Copyright (C) 2014-2015 David Bigagli
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

#include <stdlib.h>
#include "mbd.h"

static void                    catMembers (struct gData*, char*, char);
static struct groupInfoEnt*    getGroupInfoEnt(char**, int*, char);
static struct groupInfoEnt*    getGroupInfoEntFromUdata(struct uData*, char);
static void 	    	       getAllHostGroup(struct gData **, int, int,
		    	    	    	       struct groupInfoEnt *, int *);
static void    	       	       getSpecifiedHostGroup(char **, int,
						     struct gData **, int, int,
						     struct groupInfoEnt *,
						     int *);
static void    	       	       copyHostGroup(struct gData *, int,
					     struct groupInfoEnt *);

LS_BITSET_T                    *uGrpAllSet;

LS_BITSET_T                    *uGrpAllAncestorSet;

int
checkGroups(struct infoReq *groupInfoReq,
            struct groupInfoReply *groupInfoReply)
{
    int recursive;
    struct gData **gplist;
    int ngrp;

    if (groupInfoReq->options & HOST_GRP) {
        gplist = hostgroups;
        ngrp = numofhgroups;
    } else if (groupInfoReq->options & USER_GRP) {
        gplist = usergroups;
        ngrp = numofugroups;
    } else {
        return LSBE_LSBLIB;
    }

    if (groupInfoReq->options & GRP_RECURSIVE)
        recursive = TRUE;
    else
        recursive = FALSE;


    if (groupInfoReq->options & HOST_GRP) {
	if (groupInfoReq->options & GRP_ALL) {
	    getAllHostGroup(gplist, ngrp, recursive, groupInfoReply->groups,
			    &groupInfoReply->numGroups);
	} else {
	    getSpecifiedHostGroup(groupInfoReq->names, groupInfoReq->numNames,
				  gplist, ngrp, recursive,
				  groupInfoReply->groups,
				  &groupInfoReply->numGroups);

	    if (groupInfoReply->numGroups == 0 ||
		groupInfoReply->numGroups != groupInfoReq->numNames) {
		return (LSBE_BAD_GROUP);
	    }
	}
    }


    if (groupInfoReq->options & USER_GRP) {

	if (groupInfoReq->options & GRP_ALL) {
	    int num = 0;

	    groupInfoReply->groups = getGroupInfoEnt(NULL, &num, recursive);
	    groupInfoReply->numGroups = num;

	} else {
	    int num = 0;

	    num = groupInfoReq->numNames;
	    groupInfoReply->groups = getGroupInfoEnt(groupInfoReq->names,
						     &num,
						     recursive);
	    groupInfoReply->numGroups = num;
	    if ( num == 0 || num != groupInfoReq->numNames )

		return (LSBE_BAD_GROUP);
	}
    }

    return (LSBE_NO_ERROR);

}
static struct groupInfoEnt *
getGroupInfoEnt(char **groups, int *num, char recursive)
{
    static char              fname[] = "getGroupInfoEnt()";
    struct groupInfoEnt *    groupInfoEnt;

    if (groups == NULL) {
	struct uData *u;
	int i;

	groupInfoEnt = my_calloc(numofugroups,
				 sizeof(struct groupInfoEnt),
				 fname);

	for (i = 0; i < numofugroups; i++) {
	    struct groupInfoEnt *g;

	    u = getUserData(usergroups[i]->group);

	    g = getGroupInfoEntFromUdata(u, recursive);
	    if (g == NULL) {
		FREEUP(groupInfoEnt);
		lsberrno = LSBE_NO_MEM;
		return NULL;
	    }
	    memcpy(groupInfoEnt + i, g, sizeof(struct groupInfoEnt));
	}

	*num = numofugroups;
    } else {
	struct uData * u;
	int            j = 0;
	int            k;
	int            i;

	groupInfoEnt = (struct groupInfoEnt *)
	    my_calloc(*num, sizeof(struct groupInfoEnt), fname);

	for (i = 0; i < *num; i++) {
	    struct groupInfoEnt *g;
	    bool_t  validUGrp = FALSE;


	    for (k = 0; k < numofugroups; k++) {
		if (strcmp(usergroups[k]->group, groups[i]) == 0) {
		    validUGrp = TRUE;
		    break;
		}
	    }

	    if (validUGrp == FALSE) {
	        struct groupInfoEnt *  tmpgroupInfoEnt;
		lsberrno = LSBE_BAD_USER;
		*num = i;

		if ( i == 0 )  {

		    FREEUP(groupInfoEnt);
		    return NULL;
		}

		tmpgroupInfoEnt = (struct groupInfoEnt *)
			my_calloc(i, sizeof(struct groupInfoEnt), fname);

		for(k=0; k < i; k++)
		    tmpgroupInfoEnt[k] = groupInfoEnt[k];
		FREEUP(groupInfoEnt);
		groupInfoEnt = tmpgroupInfoEnt;

		return (groupInfoEnt);
	    }

	    u = getUserData(groups[i]);
	    if (u == NULL) {
		ls_syslog(LOG_ERR, I18N_FUNC_S_FAIL,
		    fname, "getUserData", groups[i]);
		continue;
	    }

	    g = getGroupInfoEntFromUdata(u, recursive);
	    if (g == NULL){
		lsberrno = LSBE_NO_MEM;
		FREEUP(groupInfoEnt);
		return NULL;
	    }

	    memcpy(groupInfoEnt + j, g, sizeof(struct groupInfoEnt));

	    j++;
	}
	*num = j;
    }

    return (groupInfoEnt);

}
static struct groupInfoEnt *
getGroupInfoEntFromUdata(struct uData *u, char recursive)
{
    static struct groupInfoEnt group;

    memset((struct groupInfoEnt *)&group, 0, sizeof(struct groupInfoEnt));

    group.group = u->user;
    group.memberList = getGroupMembers(u->gData,
				       recursive);
    /* The group_slots is only valid for host groups
     * here we assign static memory which must not
     * be freed in freegroupInfoReply()
     */
    group.group_slots = "";

    return (&group);

}
char *
getGroupMembers (struct gData *gp, char r)
{
    char *members;
    int numMembers;

    numMembers = sumMembers(gp, r, 1);
    if (numMembers == 0) {
	members = safeSave("all");
	return (members);
    }

    members = my_calloc(numMembers, MAX_LSB_NAME_LEN, "getGroupMembers");
    members[0] = '\0';
    catMembers(gp, members, r);
    return members;

}

int
sumMembers(struct gData *gp, char r, int first)
{
    int i;
    static int num;

    if (first)
	num = 0;

    if (gp->numGroups == 0 && gp->memberTab.numEnts == 0 && r)
        return 0;

    num += gp->memberTab.numEnts;

    if (!r)
	num += gp->numGroups;
    else {
	for (i = 0; i < gp->numGroups; i++)
            if (sumMembers (gp->gPtr[i], r, 0) == 0)
                return 0;
    }

    return num;
}

static void
catMembers(struct gData *gp, char *cbuf, char r)
{
    sTab hashSearchPtr;
    hEnt *hashEntryPtr;
    int i;

    hashEntryPtr = h_firstEnt_(&gp->memberTab, &hashSearchPtr);

    while (hashEntryPtr) {
        strcat(cbuf, hashEntryPtr->keyname);
        strcat(cbuf, " ");
        hashEntryPtr = h_nextEnt_(&hashSearchPtr);
    }

    for (i=0;i<gp->numGroups;i++) {
        if (!r) {
            int lastChar = strlen(gp->gPtr[i]->group)-1;
            strcat(cbuf, gp->gPtr[i]->group);
            if ( gp->gPtr[i]->group[lastChar] != '/' )
                strcat(cbuf, "/ ");
            else
                strcat(cbuf, " ");
        } else {
            catMembers(gp->gPtr[i], cbuf, r);
        }
    }
}

char *
catGnames(struct gData *gp)
{
    int i;
    char *buf;

    if (gp->numGroups <= 0) {
        buf = safeSave(" ");
        return buf;
    }

    buf = my_calloc(gp->numGroups, MAX_LSB_NAME_LEN, "catGnames");
    for (i=0;i<gp->numGroups;i++) {
	strcat (buf, gp->gPtr[i]->group);
	strcat (buf, "/ ");
    }
    return buf;
}

char **
expandGrp(struct gData *gp, char *gName, int *num)
{
    char **memberTab;

    *num = countEntries(gp, TRUE);
    if (! *num) {
        memberTab = (char **) my_calloc(1, sizeof (char *), "expandGrp");
        memberTab[0] = gName;
        *num = 1;
        return memberTab;
    }
    memberTab = (char **) my_calloc(*num, sizeof (char *), "expandGrp");

    fillMembers(gp, memberTab, TRUE);
    return memberTab;

}
void
fillMembers (struct gData *gp, char **memberTab, char first)
{
    sTab hashSearchPtr;
    hEnt *hashEntryPtr;
    static int mcnt;
    int i;

    if (first) {
        first = FALSE;
        mcnt = 0;
    }

    hashEntryPtr = h_firstEnt_(&gp->memberTab, &hashSearchPtr);

    while (hashEntryPtr) {
        memberTab[mcnt] = hashEntryPtr->keyname;
        mcnt++;
        hashEntryPtr = h_nextEnt_(&hashSearchPtr);
    }

    for (i=0; i<gp->numGroups; i++)
        fillMembers(gp->gPtr[i], memberTab, FALSE);

}
struct gData *
getGroup (int groupType, char *member)
{
    struct gData **gplist;
    int i, ngrp;

    if (groupType == HOST_GRP) {
        gplist = hostgroups;
        ngrp = numofhgroups;
    } else if (groupType == USER_GRP) {
        gplist = usergroups;
        ngrp = numofugroups;
    } else
        return NULL;

    for (i = 0; i < ngrp; i++)
        if (gMember(member, gplist[i]))
            return (gplist[i]);

    return NULL;

}

char
gDirectMember(char *word, struct gData *gp)
{

    if (word == NULL || gp == NULL)
        return FALSE;

    if (gp->numGroups == 0 && gp->memberTab.numEnts == 0)
        return TRUE;

    if (h_getEnt_(&gp->memberTab, word))
        return TRUE;
    return FALSE;

}


char
gMember(char *word, struct gData *gp)
{
    int i;

    INC_CNT(PROF_CNT_gMember);

    if (word == NULL || gp == NULL)
        return FALSE;

    if (gDirectMember(word, gp))
        return TRUE;

    for (i = 0; i < gp->numGroups; i++) {
        if (gMember(word, gp->gPtr[i]))
            return TRUE;
    }
    return FALSE;

}


int
countEntries(struct gData *gp, char first)
{
    static int num;
    int i;

    if (first)
        num = 0;

    num += gp->memberTab.numEnts;

    for (i=0;i<gp->numGroups;i++)
        countEntries(gp->gPtr[i], FALSE);

    return num;

}

struct gData *
getUGrpData(char *gname)
{
    return (getGrpData (usergroups, gname, numofugroups));
}

struct gData *
getHGrpData(char *gname)
{
    return (getGrpData (hostgroups, gname, numofhgroups));
}

struct gData *
getGrpData(struct gData *groups[], char *name, int num)
{
    int i;

    if (name == NULL)
        return NULL;

    for (i = 0; i < num; i++) {
        if (strcmp (name, groups[i]->group) == 0)
            return (groups[i]);
    }

    return NULL;

}


#define SKIP_HOST_GROUP(group) (strstr(group, "others") != NULL)

static void
getAllHostGroup(struct gData **gplist, int ngrp, int recursive,
		struct groupInfoEnt *groupInfoEnt, int *numHostGroup)
{
    int i, count;

    for (i = 0, count = 0; i < ngrp; i++) {
	if (SKIP_HOST_GROUP(gplist[i]->group)) {
	    continue;
	}
	copyHostGroup(gplist[i],
                      recursive,
		      &(groupInfoEnt[count]));
	count++;
    }
    *numHostGroup = count;
}

static void
getSpecifiedHostGroup(char **grpName, int numGrpName, struct gData **gplist,
		      int ngrp, int recursive,
		      struct groupInfoEnt *groupInfoEnt, int *numHostGroup)
{
    int i, count;

    for (count = 0; count < numGrpName; count++) {
	int groupFound = FALSE;

	for (i = 0; i < ngrp; i++) {
	    if (SKIP_HOST_GROUP(gplist[i]->group)) {
		continue;
	    }
	    if (strcmp(gplist[i]->group, grpName[count]) == 0) {
		groupFound = TRUE;
		break;
	    }
	}
	if (groupFound) {
	    copyHostGroup(gplist[i], recursive,
			  &(groupInfoEnt[count]));
	} else {

	    break;
	}
    }
    *numHostGroup = count;
}

static void
copyHostGroup(struct gData *grp,
              int recursive,
	      struct groupInfoEnt *groupInfoEnt)
{
    groupInfoEnt->group = grp->group;
    groupInfoEnt->memberList = getGroupMembers(grp, recursive);
    groupInfoEnt->group_slots = grp->group_slots;
    if (groupInfoEnt->group_slots == NULL)
	groupInfoEnt->group_slots = "";
    groupInfoEnt->max_slots = grp->max_slots;
}

int
sizeofGroupInfoReply(struct groupInfoReply *ugroups)
{
    int len;
    int i;

    len = ALIGNWORD_(sizeof(struct groupInfoReply)
                     + ugroups->numGroups * sizeof(struct groupInfoEnt)
                     + ugroups->numGroups * NET_INTSIZE_);

    for (i = 0; i < ugroups->numGroups; i++) {
	struct groupInfoEnt *ent;

	ent = &(ugroups->groups[i]);
	len += ALIGNWORD_(strlen(ent->group) + 1);
	len += ALIGNWORD_(strlen(ent->memberList) + 1);
        if (ent->group_slots)
            len += ALIGNWORD_(strlen(ent->group_slots) + 1);
    }

    return(len);

}

void
uDataGroupCreate(void)
{
    int i;

    for (i = 0; i < numofugroups; i++) {
	struct uData *u;

	if ((u = getUserData(usergroups[i]->group)) == NULL) {
	    ls_syslog(LOG_ERR, "\
%s getUserData() failed for group %s", __func__, usergroups[i]->group);
	    mbdDie(MASTER_FATAL);
	}
	u->flags |= USER_GROUP;

	if (USER_GROUP_IS_ALL_USERS(usergroups[i]) == TRUE)
	    u->flags |= USER_ALL;

	u->gData = usergroups[i];
    }
}
