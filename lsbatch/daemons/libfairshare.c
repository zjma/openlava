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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 *
 */
#include "fairshare.h"

/* init()
 */
int
init(struct qData *qPtr, struct userConf *uConf)
{
    struct tree_node_ *n;
    hEnt *e;

    qPtr->scheduler->tree
        = sshare_make_tree(qPtr->fairshare,
                           (uint32_t )uConf->numUgroups,
                           (struct group_acct *)uConf->ugroups);

    n = qPtr->scheduler->tree->root;
    while ((n = tree_next_node(n))) {

        e = h_getEnt_(&uDataList, n->path);
        if (e == NULL) {
            ls_syslog(LOG_ERR, "\
%s: skipping uknown user %s", __func__, n->path);
            continue;
        }
    }

    return 0;
}
