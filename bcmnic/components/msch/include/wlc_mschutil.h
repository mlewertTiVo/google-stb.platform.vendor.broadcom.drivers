/*
 * Interface definitions for multi-channel scheduler
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_mschutil.h 646175 2016-06-28 18:47:15Z jingyao $
 */

#ifndef _wlc_mschutil_h_
#define _wlc_mschutil_h_

typedef enum {
	MSCH_NO_ORDER = 0,
	MSCH_ASCEND = 1,
	MSCH_DESCEND = 2,
	MSCH_INVALID_ORDER
} msch_order_t;

typedef struct msch_list_elem {
	struct msch_list_elem *next;
	struct msch_list_elem *prev;
} msch_list_elem_t;

extern void msch_list_init(msch_list_elem_t *elem);
extern void msch_list_add_at(msch_list_elem_t *at_elem, msch_list_elem_t *new_elem);
extern void msch_list_add_to_tail(msch_list_elem_t* list, msch_list_elem_t* new_elem);
extern void msch_list_sorted_add(msch_list_elem_t* list, msch_list_elem_t* new_elem,
	int comp_offset, int comp_size, msch_order_t order);
extern msch_list_elem_t * msch_list_remove(msch_list_elem_t *to_del_elem);
extern msch_list_elem_t * msch_list_remove_head(msch_list_elem_t *list);
extern msch_list_elem_t * msch_list_iterate(msch_list_elem_t *last_elem);
extern bool msch_list_empty(msch_list_elem_t* list);
extern bool msch_elem_inlist(msch_list_elem_t *elem);
extern uint32 msch_list_length(msch_list_elem_t *elem);

#endif /* _wlc_mschutil _h_ */
