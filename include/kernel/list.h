/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * Mach Operating System
 * Copyright (c) 1993-1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon rights
 * to redistribute these changes.
 */
/*
 *      File:   queue.h
 *      Author: Avadis Tevanian, Jr.
 *      Date:   1985
 *
 *      Type definitions for generic queues.
 *
 */

#ifndef	_KERNEL_LIST_H_
#define	_KERNEL_LIST_H_

#include <kernel/common.h>
#include <kernel/macro_help.h>

/*
 *      List of abstract objects.  List is maintained
 *      within that object.
 *
 *      Supports fast removal from within the list.
 *
 *      How to declare a list of elements of type "foo_t":
 *              In the "*foo_t" type, you must have a field of
 *              type "list_chain_t" to hold together this queue.
 *              There may be more than one chain through a
 *              "foo_t", for use by different lists.
 *
 *              Declare the list as a "list_t" type.
 *
 *              Elements of the list (of type "foo_t", that is)
 *              are referred to by reference, and cast to type
 *              "list_entry_t" within this module.
 */

/*
 * A generic doubly-linked list.
 */
struct list_entry {
    struct list_entry *next;
    struct list_entry *prev;
};

typedef struct list_entry      *list_t;
typedef	struct list_entry      list_head_t;
typedef	struct list_entry      list_chain_t;
typedef	struct list_entry      *list_entry_t;

/*
 * Queue interface
 */
#define enqueue(queue,elt)      list_insert_tail(queue, elt)
#define dequeue(queue)          list_remove_head(queue)
#define queue_peek(queue)       list_first(queue)

/*
 * Stack interface
 */
#define stack_push(stack,elt)   list_insert_head(stack, elt)
#define stack_pop(stack)        list_remove_head(stack, elt)
#define stack_peek(stack)       list_first(stack)

/* Initialize the given list */
static inline void list_init (list_t q)
{
    q->next = q->prev = q;
}

/* Tests whether a list is empty */
static inline int list_empty (list_t q)
{
    return q == q->next;
}

/* Returns the first entry in the list */
static inline list_entry_t list_first (list_t q)
{
    return q->next;
}

/* Returns the first entry in the list, or NULL if the list is empty. */
static inline list_entry_t list_first_safe (list_t q)
{
    return list_empty (q) ? (list_entry_t) 0 : q->next;
}

/* Returns the entry after an item in the list */
static inline list_entry_t list_next (list_entry_t qe)
{
    return qe->next;
}

/* Returns the last entry in the list */
static inline list_entry_t list_last (list_t q)
{
    return q->prev;
}

/* Returns the entry before an item in the list */
static inline list_entry_t list_prev (list_entry_t qe)
{
    return qe->prev;
}

/* Tests whether a new entry is the end of the list */
static inline int list_end(list_t q, list_entry_t qe)
{
    return q == qe;
}

/* Adds the element at the tail of the list */
static inline void list_insert_tail (list_t q, list_entry_t elt)
{
    elt->next = q;
    elt->prev = q->prev;
    q->prev->next = elt;
    q->prev = elt;
}

/* Adds the element at the head of the list */
static inline void list_insert_head (list_t q, list_entry_t elt)
{
    elt->next = q->next;
    elt->prev = q;
    elt->next->prev = elt;
    q->next = elt;
}

/* Removes and returns the element at the head of the list */
static inline list_entry_t list_remove_head (list_t q)
{
    list_entry_t elt;

    if (list_empty (q))
        return (list_entry_t) 0;
    
    elt = q->next;
    elt->next->prev = q;
    q->next = elt->next;
    return elt;
}

/* Removes and returns the element at the tail of the list */
static inline list_entry_t list_remove_tail (list_t q)
{
    list_entry_t elt;

    if (list_empty (q))
        return (list_entry_t) 0;

    elt = q->prev;
    elt->prev->next = q;
    q->prev = elt->prev;
    return elt;
}

static inline void list_replace_entry (list_entry_t from, list_entry_t to)
{
    from->prev->next = to;
    from->next->prev = to;

    to->next = from->next;
    to->prev = from->prev;
}

/* Removes an arbitrary element from the list.
 * Assumes that the element is on the list.    */
static inline void list_remove (list_t q, list_entry_t elt)
{
    elt->next->prev = elt->prev;
    elt->prev->next = elt->next;
}

/* Insert 'elt' after 'pred' in the list. */
static inline void insqueue (list_entry_t elt, list_entry_t pred)
{
    elt->next = pred->next;
    elt->prev = pred;
    (pred->next)->prev = elt;
    pred->next = elt;
}

/*---------------------------------------------------------------------------*/
/*
 * Macros that operate on generic structures.  The list chain may be at any
 * location within the structure, and there may be more than one chain.
 */

/*
 * Insert a new element at the tail of the queue.
 *
 *      void list_enter(list_t q, <type> elt, <type>, <chain field>)
 */
#define list_enter(head, elt, type, field)\
    MACRO_BEGIN                                             \
        list_entry_t prev;                                  \
                                                            \
        prev = (head)->prev;                                \
        if ((head) == prev) {                               \
            (head)->next = (list_entry_t) (elt);            \
        }                                                   \
        else {                                              \
            ((type)prev)->field.next = (list_entry_t)(elt); \
	}                                                   \
        (elt)->field.prev = prev;                           \
        (elt)->field.next = head;                           \
        (head)->prev = (list_entry_t) elt;                  \
    MACRO_END

/*
 * Insert a new element at the head of the list.
 *
 *      void list_enter_first(list_t q, <type> elt, <type>, <chain field>)
 */
#define list_enter_first(head, elt, type, field)            \
    MACRO_BEGIN                                             \
        list_entry_t next;                                  \
                                                            \
        next = (head)->next;                                \
        if ((head) == next) {                               \
            (head)->prev = (list_entry_t) (elt);            \
        }                                                   \
        else {                                              \
            ((type)next)->field.prev = (list_entry_t)(elt); \
        }                                                   \
        (elt)->field.next = next;                           \
        (elt)->field.prev = head;                           \
        (head)->next = (list_entry_t) elt;                  \
    MACRO_END

/*
 * [internal use only]
 *
 * Find the list_chain_t (or list_t) for the given element (thing) in the
 * given list (head).
 */
#define	list_field(head, thing, type, field) \
        (((head) == (thing)) ? (head) : &((type)(thing))->field)

/*
 * Remove an arbitrary item from the list.
 *
 *      void list_remove(list_t q, <type> elt, <type>, <chain field>)
 */
#define	list_remove_macro(head, elt, type, field)           \
    MACRO_BEGIN                                             \
        list_entry_t next, prev;                            \
                                                            \
        next = (elt)->field.next;                           \
        prev = (elt)->field.prev;                           \
                                                            \
        if ((head) == next)                                 \
            (head)->prev = prev;                            \
        else                                                \
            ((type)next)->field.prev = prev;                \
                                                            \
        if ((head) == prev)                                 \
            (head)->next = next;                            \
        else                                                \
            ((type)prev)->field.next = next;                \
    MACRO_END

/*
 * Remove and return the entry at the head of the list.
 *
 *      void list_remove_first(list_t head, <type> entry, <type>, <chain>)
 */
#define	list_remove_first(head, entry, type, field)         \
    MACRO_BEGIN                                             \
        list_entry_t next;                                  \
                                                            \
        (entry) = (type) ((head)->next);                    \
        next = (entry)->field.next;                         \
                                                            \
        if ((head) == next)                                 \
            (head)->prev = (head);                          \
        else                                                \
            ((type)(next))->field.prev = (head);            \
        (head)->next = next;                                \
    MACRO_END

/* 
 * Remove and return the entry at the tail of the list.
 *
 *      void list_remove_last(list_t head, <type> entry, <type>, <chain>)
 */
#define	list_remove_last(head, entry, type, field)          \
    MACRO_BEGIN                                             \
        list_entry_t prev;                                  \
                                                            \
        (entry) = (type) ((head)->prev);                    \
        prev = (entry)->field.prev;                         \
                                                            \
        if ((head) == prev)                                 \
            (head)->next = (head);                          \
        else                                                \
            ((type)(prev))->field.next = (head);            \
        (head)->prev = prev;                                \
    MACRO_END

/* 
 * Assign to a link in the list.
 *
 *      void list_assign(list_entry_t to, list_entry_t from,
 *                       <type>, <chain field>)
 */
#define	list_assign(to, from, type, field)                  \
    MACRO_BEGIN                                             \
        ((type)((from)->prev))->field.next = (to);          \
        ((type)((from)->next))->field.prev = (to);          \
        *to = *from;                                        \
    MACRO_END

/* Iterate over each item in the list.  Generates a 'for' loop, setting
 * elt to each item in turn (by reference).
 *
 *      list_iterate(list_t head, <type> elt, <type>, <chain field>)
 */
#define list_iterate(head, elt, type, field)                \
        for ((elt) = (type) list_first(head);               \
             !list_end((head), (list_entry_t)(elt));        \
             (elt) = (type) list_next(&(elt)->field))

/* Iterate over each item in the queue, deqeuing after each iteration.
 * Generates a 'for' loop, setting elt to each item in turn (by reference).
 *
 *      dequeue_iterate(list_t head, <type> elt, <type>)
 */
#define dequeue_iterate(head, elt, type)                    \
        for ((elt) = (type) dequeue(head);                  \
            (elt);                                          \
            (elt) = (type) dequeue(head))

#endif /* _KERNEL_LIST_H_ */
