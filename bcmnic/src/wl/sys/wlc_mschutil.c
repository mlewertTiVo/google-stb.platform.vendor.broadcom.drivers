/*
* Multiple channel scheduler
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
* $Id: wlc_mschutil.c 617664 2016-02-06 13:09:19Z $
*
*/

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmwpa.h>
#include <bcmendian.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wl_export.h>
#include <wlc_hrt.h>
#include <hndpmu.h>

#include <wlc_msch.h>
#include <wlc_mschutil.h>

void
msch_list_init(msch_list_elem_t *elem)
{
	ASSERT(elem);
	elem->prev = elem->next = NULL;
}

void
msch_list_add_at(msch_list_elem_t *at_elem, msch_list_elem_t *new_elem)
{
	ASSERT(at_elem && new_elem);

	new_elem->prev = at_elem;
	new_elem->next = at_elem->next;
	if (at_elem->next) {
		at_elem->next->prev = new_elem;
	}
	at_elem->next = new_elem;
}

void
msch_list_add_to_tail(msch_list_elem_t* list, msch_list_elem_t* new_elem)
{
	msch_list_elem_t *elem, *prev_elem;

	ASSERT(list && new_elem);

	prev_elem = list;

	/* find proper location */
	elem = prev_elem->next;
	while (elem) {
		prev_elem = elem;
		elem = elem->next;
	}

	msch_list_add_at(prev_elem, new_elem);
	return;
}

void
msch_list_sorted_add(msch_list_elem_t* list, msch_list_elem_t* new_elem,
	int comp_offset, int comp_size, msch_order_t order)
{
	msch_list_elem_t *elem, *prev_elem;

	ASSERT(list && new_elem && (order < MSCH_INVALID_ORDER));

	prev_elem = list;
	if (order == MSCH_NO_ORDER) {
		goto exit;
	}

	/* find proper location */
	elem = prev_elem->next;
	while (elem) {
		if ((bcm_cmp_bytes((const uchar *)new_elem + comp_offset,
			(const uchar *)elem + comp_offset, (uint8)comp_size) > 0) !=
			(int)(order == MSCH_ASCEND)) {
			break;
		}
		prev_elem = elem;
		elem = elem->next;
	}

exit:
	msch_list_add_at(prev_elem, new_elem);
	return;
}

msch_list_elem_t*
msch_list_remove(msch_list_elem_t *to_del_elem)
{
	ASSERT(to_del_elem);

	if (!to_del_elem->prev && to_del_elem->next) {
		WL_ERROR(("%s : %p => unhandled link prev %p next %p\n", __FUNCTION__,
			OSL_OBFUSCATE_BUF(to_del_elem), OSL_OBFUSCATE_BUF(to_del_elem->prev),
			OSL_OBFUSCATE_BUF(to_del_elem->next)));
	}

	if (to_del_elem->prev == NULL) {
		return NULL;
	}

	to_del_elem->prev->next = to_del_elem->next;
	if (to_del_elem->next) {
		to_del_elem->next->prev = to_del_elem->prev;
	}
	msch_list_init(to_del_elem);

	return to_del_elem;
}

msch_list_elem_t*
msch_list_remove_head(msch_list_elem_t *list)
{
	ASSERT(list);

	if (!list->next) {
		return NULL;
	}

	return msch_list_remove(list->next);
}

msch_list_elem_t*
msch_list_iterate(msch_list_elem_t *last_elem)
{
	if (!last_elem) {
		return NULL;
	}

	return last_elem->next;
}

bool
msch_list_empty(msch_list_elem_t* list)
{
	if (!list) {
		return (list == NULL);
	}

	return (list->next == NULL);
}

bool
msch_elem_inlist(msch_list_elem_t *elem)
{
	ASSERT(elem);

	return (elem->prev != NULL);
}

uint32
msch_list_length(msch_list_elem_t *elem)
{
	uint32 len = 0;

	while ((elem = msch_list_iterate(elem))) {
		len++;
	}

	return len;
}
