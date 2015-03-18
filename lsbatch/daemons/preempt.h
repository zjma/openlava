/*
 * Copyright (C) 2015 David Bigagli
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

#if !defined(_PREEMPTION_HEADER_)
#define _PREEMPTION_HEADER_
#include "mbd.h"


/* Preemption scheduling plugin
 */
struct prm_sched {
    char *name;
    void *handle;
    int (*prm_init)(LIST_T *);
    int (*prm_elect_preempt)(struct qData *, link_t *, uint32_t *);
};

#endif
