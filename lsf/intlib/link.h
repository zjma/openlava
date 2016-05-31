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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA
 *
 */

/*
 * Elementary single linked list in C.
 * D. Knuth Art of Computer Programming Volume 1. 2.2
 *
 */
#ifndef __LINK__
#define __LINK__

/* Each linked list is made of a head whose ptr is always
 * NULL, a list of following links starting from next and
 * a number num indicating how many elements are in the
 * list.
 */
typedef struct link {
    int           num;
    void          *ptr;
    struct link   *next;
} link_t;

#define LINK_NUM_ENTRIES(L) ((L)->num)

typedef struct linkiter {
    link_t   *pos;
} linkiter_t;

link_t   *make_link(void);
void     fin_link(link_t *);
int      in_link(link_t *,void *);
void     *rm_link(link_t *, void *);
void     *peek_link(link_t *, void *val);
int      push_link(link_t *, void *);
int      enqueue_link(link_t *, void *);
void     *dequeue_link(link_t *);
int      enqueue_sort_link(link_t *,
                           void *,
                           void *,
                           int (*cmp)(const void *,
                                      const void *));
void     *pop_link(link_t *);
void     *visit_link(link_t *);
void     traverse_init(const link_t *,
                      linkiter_t *);
void     *traverse_link(linkiter_t *);

#endif /* __LINK__ */
