/*
 * Copyright (C) 2014-2016 David Bigagli
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

#if !defined(_FAIRSHARE_HEADER_)
#define _FAIRSHARE_HEADER_

#include "mbd.h"
#include "../../lsf/intlib/sshare.h"

/* Every HIST_HOUR decay the accumulated numRAN by this
 * factor.
 */
#define DECAY_FACTOR 10

/* Fairshare scheduling plugin
 */
struct fair_sched {
    char *name;
    void *handle;
    struct tree_ *tree;
    int (*fs_init)(struct qData *, struct userConf *);
    int (*fs_own_init)(struct qData *, struct userConf *);
    int (*fs_update_sacct)(struct qData *,
                           struct jData *,
                           int,  /* numJobs */
                           int,  /* numPEND */
                           int,  /* numRUN */
                           int,  /* numUSUSP */
                           int); /* numSSUSP */
    int (*fs_init_sched_session)(struct qData *);
    int (*fs_init_own_sched_session)(struct qData *);
    int (*fs_elect_job)(struct qData *, LIST_T *, struct jRef **);
    int (*fs_fin_sched_session)(struct qData *);
    int (*fs_get_saccts)(struct qData *, int *, struct share_acct ***);
    int (*fs_decay_ran_time)(struct qData *);
};


#endif
