/*
* Multiple channel scheduler
*
* Copyright (C) 2016, Broadcom Corporation
* All Rights Reserved.
* 
* This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
* the contents of this file may not be disclosed to third parties, copied
* or duplicated in any form, in whole or in part, without the prior
* written permission of Broadcom Corporation.
*
* All exported functions are at the end of the file.
*
* The file contains MSCH exported functions (end of the file)
* and scheduling algorithm for different MSCH request types
* defined in wlc_msch.h file.
*
* <<Broadcom-WL-IPTag/Proprietary:>>
*
* $Id: wlc_msch.c 612583 2016-01-14 09:08:40Z $
*/

/**
* @file
* @brief
* Twiki: [MultiChanScheduler]
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
#include <wlc_msch_priv.h>

#ifdef MSCH_PROFILER
#include <wlc_msch_profiler.h>
#define MSCH_MESSAGE(x)	msch_message_printf x
#else /* MSCH_PROFILER */
#define MSCH_MESSAGE(x)
#endif /* MSCH_PROFILER */

#define MSCH_ASSIGN_MAGIC_NUM(msch_info, value)	((msch_info)->magic_num = value)
#define MSCH_CHECK_MAGIC(msch_info)	((msch_info)->magic_num == MSCH_MAGIC_NUMBER)

#define MSCH_TIME_MIN_ERROR	50

#define US_PRE_SEC		1000000
#define NUMBER_PMU_TICKS	1024
#define SHIFT_PMU_TICKS		10

#define	ONE_MS_DELAY	(1000)	/* In u seconds */
#define MAX_BOTH_FLEX_SKIP_COUNT	3

#define MSCH_SKIP_PREPARE (10 * 1000)

static void _msch_arm_chsw_timer(wlc_msch_info_t *msch_info);
static bool wlc_msch_add_timeout(wlc_hrt_to_t *to, int timeout, wlc_hrt_to_cb_fn fun, void *arg);
static bool _msch_check_pending_chan_ctxt(wlc_msch_info_t *msch_info, msch_chan_ctxt_t *ctxt);
static void _msch_chan_ctxt_destroy(wlc_msch_info_t *msch_info, msch_chan_ctxt_t *chan_ctxt,
	bool free2list);
static void _msch_update_chan_ctxt_bw(wlc_msch_info_t *msch_info, msch_chan_ctxt_t *chan_ctxt);
static void _msch_timeslot_destroy(wlc_msch_info_t *msch_info, msch_timeslot_t *ts,
	bool cur_slot);
static void _msch_timeslot_free(wlc_msch_info_t *msch_info,
	msch_timeslot_t *timeslot, bool free2list);
static void _msch_timeslot_chn_clbk(wlc_msch_info_t *msch_info, msch_timeslot_t *ts,
	uint32 clbk_type);
static void _msch_create_pend_slot(wlc_msch_info_t *msch_info, msch_timeslot_t *timeslot);
static void _msch_schd_slot_start_end(wlc_msch_info_t *msch_info, msch_timeslot_t *ts);
static void _msch_update_service_interval(wlc_msch_info_t *msch_info);
static msch_ret_type_t _msch_schd_new_req(wlc_msch_info_t *msch_info,
	wlc_msch_req_handle_t *req_hdl);
static void _msch_schd(wlc_msch_info_t *msch_info);
static void _msch_prio_list_rotate(wlc_msch_info_t *msch_info,
	msch_list_elem_t *list, uint32 offset);
static void _msch_ts_send_skip_notif(wlc_msch_info_t *msch_info,
	msch_timeslot_t *ts, bool timer_ctx);
static msch_ret_type_t _msch_slot_skip(wlc_msch_info_t *msch_info,
	msch_req_entity_t *entity);
static void _msch_sch_fixed(wlc_msch_info_t *msch_info,
	msch_req_entity_t *entity, uint32 prep_dur);
static void _msch_schd_next_chan_ctxt(wlc_msch_info_t *msch_info, msch_timeslot_t *ts,
	uint32 prep_dur, uint64 slot_dur);
static void _msch_next_pend_slot(wlc_msch_info_t *msch_info,
	msch_req_entity_t *req_entity);
static bool _msch_check_both_flex(wlc_msch_info_t *msch_info, uint32 req_dur);
static bool _msch_req_hdl_valid(wlc_msch_info_t *msch_info, wlc_msch_req_handle_t *req_hdl);
static void _msch_update_both_flex_list(wlc_msch_info_t *msch_info, uint64 slot_start,
	uint64 slot_dur);
static void _msch_req_entity_destroy(wlc_msch_info_t *msch_info, msch_req_entity_t *entity);
static uint64 _msch_get_next_slot_fire(wlc_msch_info_t *msch_info);
static void _msch_slotskip_timer(void *arg);

static msch_ret_type_t _msch_check_avail_extend_slot(wlc_msch_info_t *msch_info, uint64 slot_start,
	uint8 req_prio, uint64 req_dur);

static msch_timeslot_t*
_msch_timeslot_alloc(wlc_msch_info_t *msch_info)
{
	msch_timeslot_t *timeslot = NULL;

	timeslot = MALLOCZ(msch_info->wlc->osh, sizeof(msch_timeslot_t));

	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: timeslot ALLOC %p\n", __FUNCTION__, timeslot));

	if (timeslot) {
		timeslot->timeslot_id = msch_info->ts_id++;
#ifdef MSCH_PROFILER
		PROFILE_CNT_INCR(msch_info, msch_timeslot_alloc_cnt);
#endif /* MSCH_PROFILER */
	}

	return timeslot;
}

/* Function to send skip notification if the next slot's both fixed
 * item is going to be skipped. Updates the pend slot time
 * to skip slot time + interval
 */
static msch_ret_type_t
_msch_slot_skip(wlc_msch_info_t *msch_info,  msch_req_entity_t *entity)
{
	wlc_msch_cb_info_t cb_info;
	wlc_msch_skipslot_info_t skipslot;
	wlc_msch_req_handle_t *hdl_before_clbk;
	wlc_msch_req_param_t *req_param;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(entity && entity->req_hdl);

	memset(&cb_info, 0, sizeof(cb_info));
	req_param = entity->req_hdl->req_param;
	ASSERT(MSCH_START_FIXED(req_param->req_type));

	msch_list_remove(&entity->start_fixed_link);

	entity->pend_slot.timeslot = NULL;
	cb_info.type = MSCH_CT_SLOT_SKIP;
	/* notify caller the cancel event */
	cb_info.chanspec = entity->chanspec;
	cb_info.req_hdl = entity->req_hdl;
	skipslot.pre_start_time_h =
		(uint32)(entity->pend_slot.pre_start_time >> 32);
	skipslot.pre_start_time_l = (uint32)entity->pend_slot.pre_start_time;
	skipslot.end_time_h = (uint32)(entity->pend_slot.end_time >> 32);
	skipslot.end_time_l = (uint32)entity->pend_slot.end_time;
	cb_info.type_specific = &skipslot;

	if (req_param->interval) {
		wlc_uint64_add(&req_param->start_time_h,
			&req_param->start_time_l, 0,
			req_param->interval);

		entity->pend_slot.start_time =
			((uint64)req_param->start_time_h << 32) |
			req_param->start_time_l;
		entity->pend_slot.end_time = entity->pend_slot.start_time +
			req_param->duration;

		msch_list_sorted_add(&msch_info->msch_start_fixed_list,
			&entity->start_fixed_link,
			OFFSETOF(msch_req_entity_t, pend_slot) -
			OFFSETOF(msch_req_entity_t, start_fixed_link) +
			OFFSETOF(msch_req_timing_t, start_time),
			sizeof(uint64), MSCH_ASCEND);
	}
	else {
		cb_info.type |= MSCH_CT_REQ_END;
	}

#ifdef MSCH_PROFILER
	_msch_dump_callback(msch_info->profiler, &cb_info);
#endif /* MSCH_PROFILER */
	hdl_before_clbk = entity->req_hdl;
	entity->req_hdl->cb_func(entity->req_hdl->cb_ctxt, &cb_info);

	/* clbk could internally call unregister, which could cause
	 * the elem to go NULL
	 */
	if (!_msch_req_hdl_valid(msch_info, hdl_before_clbk)) {
		return MSCH_TIMESLOT_REMOVED;
	}

	return MSCH_OK;
}

/* Function to send skip notificaion to all the requests
 * that were sheduled in the passed time slot
 */
static void
_msch_ts_send_skip_notif(wlc_msch_info_t *msch_info, msch_timeslot_t *ts,
	bool timer_ctx)
{
	msch_list_elem_t *prev_elem, *elem = NULL;
	msch_req_entity_t *entity = NULL;
	msch_ret_type_t ret;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	if (!ts || !ts->chan_ctxt) {
		ASSERT(FALSE);
		return;
	}

	elem = &ts->chan_ctxt->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
		if (MSCH_START_FIXED(entity->req_hdl->req_param->req_type)) {
			prev_elem = elem->prev;
			ret = _msch_slot_skip(msch_info, entity);
			if (ret == MSCH_TIMESLOT_REMOVED) {
				elem = prev_elem;
				continue;
			}
			if (entity->req_hdl->req_param->interval == 0) {
				wlc_msch_timeslot_unregister(msch_info,
					&entity->req_hdl);
			}
		}
	}
}

/*
 * time slot destroy, checks both channel context and timeslot
 * since they are inter related
 */
static void
_msch_timeslot_destroy(wlc_msch_info_t *msch_info, msch_timeslot_t *ts,
	bool cur_slot)
{
	msch_req_entity_t *entity;
	msch_list_elem_t *elem = NULL;
	bool bf_sch_pending = FALSE;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	if (msch_list_empty(&ts->chan_ctxt->req_entity_list)) {
		_msch_chan_ctxt_destroy(msch_info, ts->chan_ctxt, TRUE);
		ts->chan_ctxt = NULL;
	} else {
		_msch_update_chan_ctxt_bw(msch_info, ts->chan_ctxt);
		elem = &ts->chan_ctxt->req_entity_list;
		while ((elem = msch_list_iterate(elem))) {
			entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
			if (cur_slot && entity->cur_slot.timeslot == ts) {
				entity->cur_slot.timeslot = NULL;
			}
			if (!cur_slot && entity->pend_slot.timeslot == ts) {
				entity->pend_slot.timeslot = NULL;
			}

			if (MSCH_BOTH_FLEX(entity->req_hdl->req_param->req_type) &&
				(entity->pend_slot.timeslot || entity->cur_slot.timeslot)) {
				bf_sch_pending = TRUE;
			}
		}

		if (bf_sch_pending != ts->chan_ctxt->bf_sch_pending) {
			ts->chan_ctxt->bf_sch_pending = bf_sch_pending;
		}
	}

	_msch_timeslot_free(msch_info, ts, TRUE);
	if (ts == msch_info->cur_msch_timeslot) {
		msch_info->cur_msch_timeslot = NULL;
	}
}

static void
_msch_timeslot_free(wlc_msch_info_t *msch_info, msch_timeslot_t *timeslot,
	bool free2list)
{
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: timeslot FREE %p\n", __FUNCTION__, timeslot));
	ASSERT(msch_info && timeslot);

	MFREE(msch_info->wlc->osh, timeslot, sizeof(msch_timeslot_t));

#ifdef MSCH_PROFILER
	PROFILE_CNT_DECR(msch_info, msch_timeslot_alloc_cnt);
#endif /* MSCH_PROFILER */
}

static msch_req_entity_t*
_msch_req_entity_alloc(wlc_msch_info_t *msch_info)
{
	msch_req_entity_t *req_entity = NULL;
	msch_list_elem_t *elem = NULL;

	ASSERT(msch_info);

	elem = msch_list_remove_head(&msch_info->free_req_entity_list);
	if (elem) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, req_hdl_link);
		memset(req_entity, 0, sizeof(msch_req_entity_t));
	} else {
		req_entity = MALLOCZ(msch_info->wlc->osh, sizeof(msch_req_entity_t));
	}
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: req_entity ALLOC %p\n", __FUNCTION__, req_entity));

#ifdef MSCH_PROFILER
	if (req_entity)
		PROFILE_CNT_INCR(msch_info, msch_req_entity_alloc_cnt);
#endif /* MSCH_PROFILER */

	return req_entity;
}

static void
_msch_req_entity_free(wlc_msch_info_t *msch_info, msch_req_entity_t *req_entity,
	bool free2list)
{
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: req_entity FREE %p\n", __FUNCTION__, req_entity));
	ASSERT(msch_info && req_entity);

	if (free2list) {
		/* put back to free list (msch_info->free_req_entity_list) */
		msch_list_add_at(&msch_info->free_req_entity_list, &req_entity->req_hdl_link);
	} else {
		MFREE(msch_info->wlc->osh, req_entity, sizeof(msch_req_entity_t));
	}

#ifdef MSCH_PROFILER
	PROFILE_CNT_DECR(msch_info, msch_req_entity_alloc_cnt);
#endif /* MSCH_PROFILER */
}

static msch_chan_ctxt_t*
_msch_chan_ctxt_alloc(wlc_msch_info_t *msch_info, chanspec_t chanspec)
{
	msch_chan_ctxt_t *chan_ctxt = NULL;
	msch_list_elem_t *elem = NULL;

	ASSERT(msch_info);

	elem = &msch_info->msch_chan_ctxt_list;
	while ((elem = msch_list_iterate(elem))) {
		chan_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, link);

		if (wf_chspec_ctlchan(chanspec) == wf_chspec_ctlchan(chan_ctxt->chanspec)) {
			/* same ctl channel, merge */
			return chan_ctxt;
		}
	}

	elem = msch_list_remove_head(&msch_info->free_chan_ctxt_list);
	if (elem) {
		chan_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, link);
		memset(chan_ctxt, 0, sizeof(msch_chan_ctxt_t));
	} else {
		chan_ctxt = MALLOCZ(msch_info->wlc->osh, sizeof(msch_chan_ctxt_t));
	}
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: chan_ctxt ALLOC %p\n", __FUNCTION__, chan_ctxt));

	if (chan_ctxt) {
		chan_ctxt->chanspec = chanspec;
		msch_list_add_at(&msch_info->msch_chan_ctxt_list, &chan_ctxt->link);
#ifdef MSCH_PROFILER
		PROFILE_CNT_INCR(msch_info, msch_chan_ctxt_alloc_cnt);
#endif /* MSCH_PROFILER */
	}

	return chan_ctxt;
}

/* chan_ctx_destroy takes care of unlinking itself from all the list
 * and free's the context
 */
static void
_msch_chan_ctxt_destroy(wlc_msch_info_t *msch_info, msch_chan_ctxt_t *chan_ctxt,
	bool free2list)
{
	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: chan_ctxt FREE %p\n", __FUNCTION__, chan_ctxt));
	ASSERT(msch_info && chan_ctxt);
	ASSERT(msch_list_empty(&chan_ctxt->req_entity_list));

	msch_list_remove(&chan_ctxt->link);
	msch_list_remove(&chan_ctxt->bf_link);
	msch_list_remove(&chan_ctxt->bf_entity_list);
	if (FALSE) {
		msch_list_add_at(&msch_info->free_chan_ctxt_list, &chan_ctxt->link);
	} else {
		MFREE(msch_info->wlc->osh, chan_ctxt, sizeof(msch_chan_ctxt_t));
	}
#ifdef MSCH_PROFILER
	PROFILE_CNT_DECR(msch_info, msch_chan_ctxt_alloc_cnt);
#endif /* MSCH_PROFILER */
}

static void
_msch_chan_ctxt_free(wlc_msch_info_t *msch_info, msch_chan_ctxt_t *chan_ctxt,
	bool free2list)
{
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: chan_ctxt FREE %p\n", __FUNCTION__, chan_ctxt));
	ASSERT(msch_info && chan_ctxt);

	if (free2list) {
		/* put back to free list (msch_info->free_chan_ctxt_list) */
		msch_list_add_at(&msch_info->free_chan_ctxt_list, &chan_ctxt->link);
	} else {
		MFREE(msch_info->wlc->osh, chan_ctxt, sizeof(msch_chan_ctxt_t));
	}

#ifdef MSCH_PROFILER
	PROFILE_CNT_DECR(msch_info, msch_chan_ctxt_alloc_cnt);
#endif /* MSCH_PROFILER */
}

static wlc_msch_req_handle_t*
_msch_req_handle_alloc(wlc_msch_info_t *msch_info)
{
	wlc_msch_req_handle_t *req_hdl = NULL;
	msch_list_elem_t *elem = NULL;

	ASSERT(msch_info);

	elem = msch_list_remove_head(&msch_info->free_req_hdl_list);
	if (elem) {
		req_hdl = LIST_ENTRY(elem, wlc_msch_req_handle_t, link);
		memset(req_hdl, 0, sizeof(wlc_msch_req_handle_t));
	} else {
		req_hdl = MALLOCZ(msch_info->wlc->osh, sizeof(wlc_msch_req_handle_t));
	}
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: req_hdl ALLOC %p\n", __FUNCTION__, req_hdl));

#ifdef MSCH_PROFILER
	if (req_hdl)
		PROFILE_CNT_INCR(msch_info, msch_req_hdl_alloc_cnt);
#endif /* MSCH_PROFILER */

	return req_hdl;
}

static void
_msch_req_handle_free(wlc_msch_info_t *msch_info, wlc_msch_req_handle_t *req_hdl,
	bool free2list)
{
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: req_hdl FREE %p\n", __FUNCTION__, req_hdl));
	ASSERT(msch_info && req_hdl);

	if (free2list) {
		/* put back to free list (msch_info->free_req_hdl_list) */
		msch_list_add_at(&msch_info->free_req_hdl_list, &req_hdl->link);
	} else {
		MFREE(msch_info->wlc->osh, req_hdl, sizeof(wlc_msch_req_handle_t));
	}

#ifdef MSCH_PROFILER
	PROFILE_CNT_DECR(msch_info, msch_req_hdl_alloc_cnt);
#endif /* MSCH_PROFILER */
}


/* channel change function, majorly invokde wlc_set_chanspec() */
static int
_msch_chan_adopt(wlc_msch_info_t *msch_info, chanspec_t chanspec)
{
	wlc_info_t *wlc = msch_info->wlc;
	MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
		"%s Change Chanspec 0x%x\n", __FUNCTION__, chanspec));
#ifdef MSCH_PROFILER
	_msch_dump_profiler(msch_info->profiler);
#endif /* MSCH_PROFILER */

	wlc_suspend_mac_and_wait(wlc);
	/* No cals during channel switch */
	wlc_phy_hold_upd(WLC_PI(wlc), PHY_HOLD_FOR_VSDB, TRUE);
	wlc_set_chanspec(wlc, chanspec);
	wlc_phy_hold_upd(WLC_PI(wlc), PHY_HOLD_FOR_VSDB, FALSE);
	wlc_enable_mac(wlc);
	ASSERT((WLC_BAND_PI_RADIO_CHANSPEC == chanspec));
	return BCME_OK;
}

static msch_timeslot_t*
_msch_timeslot_request(wlc_msch_info_t *msch_info,
	msch_chan_ctxt_t *chan_ctxt, wlc_msch_req_param_t *req_param, msch_req_timing_t *slot)
{
	msch_timeslot_t *timeslot = NULL;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info->next_timeslot == NULL);

	/* The requested slot's pre_start_time should be greater than current slot's end time */
	if (msch_info->cur_msch_timeslot && msch_info->cur_msch_timeslot->end_time != -1 &&
		(slot->pre_start_time < msch_info->cur_msch_timeslot->end_time)) {
#ifdef MSCH_PROFILER
		_msch_dump_profiler(msch_info->profiler);
#endif /* MSCH_PROFILER */
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"%s: req prestart time %u is less than current slot's end time %u\n",
			__FUNCTION__, (uint32)slot->pre_start_time,
			(uint32)msch_info->cur_msch_timeslot->end_time));
		ASSERT(FALSE);
		return NULL;
	};

	/* create new time slot */
	timeslot = _msch_timeslot_alloc(msch_info);
	if (!timeslot) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"%s: msch_timeslot_alloc failed!!\n", __FUNCTION__));
		ASSERT(FALSE);
		return NULL;
	}

	timeslot->pre_start_time = slot->pre_start_time;
	timeslot->end_time = slot->end_time;
	timeslot->chan_ctxt = chan_ctxt;
	if (timeslot->end_time != -1) {
		timeslot->sch_dur = timeslot->end_time - slot->start_time;
	} else {
		timeslot->sch_dur = -1;
	}

	chan_ctxt->pend_onchan_dur += timeslot->sch_dur;
	return timeslot;
}

/* Called from request unregister or cancel_timeslot
 * This function evaluates if the time slot is still
 * valid based on current pending request entity.
 * Also udpate the new end time if changed due to removed
 * request
 */
static void
_msch_timeslot_check(wlc_msch_info_t *msch_info, msch_timeslot_t *ts)
{
	uint64 new_end_time = 0;
	msch_list_elem_t *elem;
	msch_req_entity_t *entity;

	ASSERT((ts == msch_info->cur_msch_timeslot));
	if (ts == msch_info->cur_msch_timeslot) {
		/* calculate new end time for remaining entities */
		elem = &ts->chan_ctxt->req_entity_list;
		while ((elem = msch_list_iterate(elem))) {
			entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
			if (entity->cur_slot.timeslot == ts &&
				entity->cur_slot.end_time > new_end_time) {
				new_end_time = entity->cur_slot.end_time;
			}
		}

		if (new_end_time) {
			if (new_end_time != ts->end_time) {
				/* no more entity or new end time, schedule next timeslot */
				if (new_end_time) {
					safe_dec(ts->chan_ctxt->pend_onchan_dur,
						ts->end_time - new_end_time);
					ts->sch_dur -= (ts->end_time - new_end_time);
				}
				ts->end_time = new_end_time;

				if (ts->fire_time == -1) {
					ts->fire_time = ts->end_time;
				}
				_msch_arm_chsw_timer(msch_info);
			}
		} else {
			/* No req entity in ts delete it */
			_msch_timeslot_free(msch_info, ts, TRUE);
			msch_info->cur_msch_timeslot = NULL;
		}
	}
}

static int
_msch_req_entity_create(wlc_msch_info_t *msch_info, chanspec_t chanspec,
	wlc_msch_req_param_t *req_param, wlc_msch_req_handle_t *req_hdl)
{
	msch_list_elem_t *elem;
	msch_chan_ctxt_t *chan_ctxt = NULL, *tmp_ctxt;
	msch_req_entity_t *req_entity = NULL;
	int ret = BCME_OK;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	/* find existing chan_ctxt or create new one per chanspec */
	chan_ctxt = _msch_chan_ctxt_alloc(msch_info, chanspec);
	if (!chan_ctxt) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"%s: msch_chan_ctxt_alloc failed!!\n", __FUNCTION__));
		ret = BCME_NOMEM;
		goto fail;
	}

	/* create new req_entity per chanspec */
	req_entity = _msch_req_entity_alloc(msch_info);
	if (!req_entity) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"%s: msch_req_entity_alloc failed!!\n", __FUNCTION__));
		ret = BCME_NOMEM;
		goto fail;
	}

	ASSERT(req_entity->pend_slot.timeslot == NULL);

	req_entity->chanspec = chanspec;
	req_entity->chan_ctxt = chan_ctxt;
	req_entity->req_hdl = req_hdl;
	req_entity->priority = req_hdl->req_param->priority;

	/* Move the start time to future for START_FIXED and BOTH_FLEX */
	if (!MSCH_START_FLEX(req_param->req_type)) {
		req_entity->pend_slot.start_time = ((uint64)req_param->start_time_h << 32) |
			req_param->start_time_l;

		/* Calculate the next slot */
		while (req_entity->req_hdl->req_param->interval &&
			(req_entity->pend_slot.start_time <
			(msch_current_time(msch_info) + MSCH_PREPARE_DUR))) {
			req_entity->pend_slot.start_time +=
				req_entity->req_hdl->req_param->interval;
		}

		req_entity->pend_slot.pre_start_time =
			req_entity->pend_slot.start_time - MSCH_ONCHAN_PREPARE;
		req_entity->pend_slot.end_time = req_entity->pend_slot.start_time +
			req_entity->req_hdl->req_param->duration;
	}

	/* Add the request entity on corresponding list based on its request type */
	if (MSCH_START_FIXED(req_param->req_type)) {
		/* queue in timing */
		msch_list_sorted_add(&msch_info->msch_start_fixed_list,
			&req_entity->start_fixed_link,
			OFFSETOF(msch_req_entity_t, pend_slot) -
			OFFSETOF(msch_req_entity_t, start_fixed_link) +
			OFFSETOF(msch_req_timing_t, start_time),
			sizeof(uint64), MSCH_ASCEND);
	} else if (MSCH_START_FLEX(req_param->req_type)) {
		/* queue in priority */
		msch_list_sorted_add(&msch_info->msch_start_flex_list,
			&req_entity->rt_specific_link,
			OFFSETOF(msch_req_entity_t, priority) -
			OFFSETOF(msch_req_entity_t, rt_specific_link),
			1, MSCH_DESCEND);

	} else if (MSCH_BOTH_FLEX(req_param->req_type)) {
		/* queue in priority in bf_entity_list */
		msch_list_sorted_add(&chan_ctxt->bf_entity_list,
			&req_entity->rt_specific_link,
			OFFSETOF(msch_req_entity_t, priority) -
			OFFSETOF(msch_req_entity_t, chan_ctxt_link),
			1, MSCH_DESCEND);

		/* queue in timing in msch_both_flex_req_entity_list */
		msch_list_sorted_add(&msch_info->msch_both_flex_req_entity_list,
			&req_entity->both_flex_list,
			OFFSETOF(msch_req_entity_t, pend_slot) -
			OFFSETOF(msch_req_entity_t, both_flex_list) +
			OFFSETOF(msch_req_timing_t, start_time),
			1, MSCH_ASCEND);

		if (req_param->interval > msch_info->max_lo_prio_interval) {
			msch_info->max_lo_prio_interval = req_param->interval;
		}

		if (!msch_elem_inlist(&chan_ctxt->bf_link)) {
			msch_list_sorted_add(&msch_info->msch_both_flex_list,
				&chan_ctxt->bf_link, 0, 0, MSCH_NO_ORDER);
			msch_info->flex_list_cnt++;
			if (msch_info->flex_list_cnt > 1) {
				/* only multi bf ctxt need use service interval to schedule */
				msch_info->service_interval = msch_info->max_lo_prio_interval /
					msch_info->flex_list_cnt;
				elem = &msch_info->msch_both_flex_list;
				while ((elem = msch_list_iterate(elem))) {
					tmp_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, bf_link);
					tmp_ctxt->actual_onchan_dur = 0;
					tmp_ctxt->pend_onchan_dur = 0;
				}
			}
		}
	} else {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"%s: No MSCH reqest type found\n", __FUNCTION__));
		ret = BCME_BADARG;
		goto fail;
	}

	/* If there is a change in channel bandwidth, then udpate the context
	 * bandwidth
	 */
	if (chan_ctxt->chanspec) {
		/* pick widest band */
		if (CHSPEC_BW(chanspec) > CHSPEC_BW(chan_ctxt->chanspec)) {
			chan_ctxt->chanspec = chanspec;
		}
	} else {
		chan_ctxt->chanspec = chanspec;
	}

	/* queue req_entity to chan_ctxt->req_entity_list in priority order */
	msch_list_sorted_add(&chan_ctxt->req_entity_list, &req_entity->chan_ctxt_link,
		OFFSETOF(msch_req_entity_t, priority) -
		OFFSETOF(msch_req_entity_t, chan_ctxt_link),
		1, MSCH_DESCEND);

	/* queue req_entity to req_hdl->req_entity_list */
	msch_list_add_to_tail(&req_hdl->req_entity_list, &req_entity->req_hdl_link);
	return ret;

fail:
	if (req_entity) {
		_msch_req_entity_destroy(msch_info, req_entity);
		req_entity = NULL;
	}

	if (chan_ctxt &&
		msch_list_empty(&chan_ctxt->req_entity_list)) {
		if (msch_info->cur_msch_timeslot) {
			ASSERT(chan_ctxt != msch_info->cur_msch_timeslot->chan_ctxt);
		}
		_msch_chan_ctxt_destroy(msch_info, chan_ctxt, TRUE);
		chan_ctxt = NULL;
	}
	return ret;
}

static void
_msch_req_entity_destroy(wlc_msch_info_t *msch_info, msch_req_entity_t *req_entity)
{
	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info && req_entity);

	msch_list_remove(&req_entity->rt_specific_link);
	msch_list_remove(&req_entity->chan_ctxt_link);
	msch_list_remove(&req_entity->req_hdl_link);
	msch_list_remove(&req_entity->start_fixed_link);
	msch_list_remove(&req_entity->both_flex_list);
	msch_list_remove(&req_entity->cur_slot.link);
	msch_list_remove(&req_entity->pend_slot.link);

	_msch_req_entity_free(msch_info, req_entity, TRUE);
}

/* Schedule a timeslot for start flex entity, prepare the
 * start time/end time and request a timeslot
 */
static msch_ret_type_t
_msch_sch_start_flex(wlc_msch_info_t *msch_info, msch_req_entity_t *entity,
	uint64 pre_start_time, uint32 prep_dur)
{
	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info && entity);

	if (entity->pend_slot.timeslot) {
		return MSCH_ALREADY_SCHD;
	}

	entity->pend_slot.pre_start_time = pre_start_time;
	entity->pend_slot.start_time = pre_start_time + prep_dur;
	entity->pend_slot.end_time = entity->pend_slot.start_time +
		entity->req_hdl->req_param->duration;
	entity->pend_slot.timeslot = _msch_timeslot_request(msch_info, entity->chan_ctxt,
		entity->req_hdl->req_param, &entity->pend_slot);
	if (entity->pend_slot.timeslot) {
		/* request succeed */
		return MSCH_OK;
	}

	return MSCH_FAIL;
}

/* Schedule a timeslot for both flex entity, prepare the
 * start time/end time and request a timeslot
 */
static msch_ret_type_t
_msch_sch_both_flex(wlc_msch_info_t *msch_info, uint64 pre_start_time,
	uint64 free_slot, msch_req_entity_t *entity, uint32 prep_dur)
{
	msch_chan_ctxt_t *chan_ctxt = NULL;
	uint64 sch_dur, time_to_next_bf = 0;
	msch_req_entity_t *next_entity = NULL;
	uint64 next_bf_pre_start = 0;
	msch_list_elem_t *elem;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	chan_ctxt = entity->chan_ctxt;
	if (chan_ctxt->bf_sch_pending) {
		/* already scheduled */
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"%s: both flex already scheduled\n", __FUNCTION__));
		return MSCH_ALREADY_SCHD;
	}

	sch_dur = (free_slot == -1) ? -1 : free_slot;
	entity->pend_slot.pre_start_time = pre_start_time;
	entity->pend_slot.start_time = pre_start_time + prep_dur;

	/* Get next BF element */
	if (entity->both_flex_list.next) {
		next_entity = LIST_ENTRY(entity->both_flex_list.next, msch_req_entity_t,
			both_flex_list);
		next_bf_pre_start = next_entity->pend_slot.start_time -
			MSCH_PREPARE_DUR;
		time_to_next_bf = (next_bf_pre_start - entity->pend_slot.start_time);
		if ((entity->req_hdl->req_param->flex.bf.min_dur > time_to_next_bf) &&
			(sch_dur > time_to_next_bf)) {
			sch_dur = time_to_next_bf;
		}
	} else if ((elem = msch_list_iterate(&msch_info->msch_both_flex_req_entity_list))) {
		next_entity = LIST_ENTRY(elem, msch_req_entity_t, both_flex_list);
		if (next_entity != entity && next_entity->req_hdl->req_param->interval) {
			next_bf_pre_start = next_entity->pend_slot.start_time +
				next_entity->req_hdl->req_param->interval -
				MSCH_PREPARE_DUR;
			while (next_bf_pre_start < entity->pend_slot.start_time) {
				next_bf_pre_start += next_entity->req_hdl->req_param->interval;
			}
			time_to_next_bf = (next_bf_pre_start - entity->pend_slot.start_time);
			if ((entity->req_hdl->req_param->flex.bf.min_dur > time_to_next_bf) &&
				(sch_dur > time_to_next_bf)) {
				sch_dur = time_to_next_bf;
			}
		}
	}

	if (sch_dur == -1) {
		/* scheduler for min duration if there is a pending
		 * chan ctxt
		 */
		if (_msch_check_pending_chan_ctxt(msch_info, chan_ctxt)) {
			/* TODO let the duration pass from higher layer ? */
			/* each req_entity could have different home time so ...? */
			/* VSDB case ? */
			entity->pend_slot.end_time =
				entity->pend_slot.start_time +
				entity->req_hdl->req_param->flex.bf.min_dur;
		} else {
			entity->pend_slot.end_time = -1;
		}
	} else {
		entity->pend_slot.end_time = entity->pend_slot.start_time + sch_dur;
	}

	entity->pend_slot.timeslot = _msch_timeslot_request(msch_info, chan_ctxt,
		entity->req_hdl->req_param, &entity->pend_slot);

	if (entity->pend_slot.timeslot) {
		ASSERT(!msch_info->next_timeslot);
		chan_ctxt->bf_sch_pending = TRUE;
		msch_info->next_timeslot = entity->pend_slot.timeslot;
		return MSCH_OK;
	}
	return MSCH_FAIL;
}

/* Function to notify SLOT_START and SLOT_END callback notification
 * prepares the next notification if required
 * returns the next fire time
 */
static uint64
_msch_slot_start_end_notif(wlc_msch_info_t *msch_info, msch_timeslot_t *ts)
{
	wlc_msch_cb_info_t cb_info;
	msch_req_timing_t *slot;
	msch_req_entity_t *req_entity = NULL;
	msch_list_elem_t *elem = NULL;
	wlc_msch_req_handle_t *hdl_before_clbk;
	uint64 next_fire = -1;
	uint64 cur_time = msch_current_time(msch_info);
	wlc_msch_onchan_info_t onchan;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	/* time sanity check */
	next_fire = _msch_get_next_slot_fire(msch_info);
	if (next_fire > cur_time) {
		MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
			"%s: next fire time is greater than cur_time\n", __FUNCTION__));
		return next_fire;
	}

	while ((elem = msch_list_remove_head(&msch_info->msch_req_timing_list))) {
		wlc_msch_req_param_t *req_param;
		slot = LIST_ENTRY(elem, msch_req_timing_t, link);
		req_entity = LIST_ENTRY(slot, msch_req_entity_t, cur_slot);
		req_param = req_entity->req_hdl->req_param;

		memset(&cb_info, 0, sizeof(cb_info));
		cb_info.chanspec = ts->chan_ctxt->chanspec;
		cb_info.req_hdl = req_entity->req_hdl;

		if (slot->flags & MSCH_RC_FLAGS_START_FIRE_DONE) {
			slot->flags |= MSCH_RC_FLAGS_END_FIRE_DONE;
			if (ts->fire_time != slot->end_time) {
				MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
					"%s fire %s end_time %s cur_time %s\n", __FUNCTION__,
					_msch_display_time(ts->fire_time),
					_msch_display_time(slot->end_time),
					_msch_display_time(cur_time)));
			}

			cb_info.type |= MSCH_CT_SLOT_END;
			if (!req_param->interval) {
				req_entity->req_hdl->chan_cnt--;
				/* Send req end for the non-periodic last channel slot_end */
				if (req_entity->req_hdl->chan_cnt == 0) {
					cb_info.type |= MSCH_CT_REQ_END;
				}
			} else if ((req_param->interval == req_param->duration) &&
					MSCH_BOTH_FIXED(req_param->req_type) &&
					!msch_info->next_timeslot) {
				/* consicutive both_fixed slots
				 * Merge the next slot to cur timeslot if there is no
				 * gap between the current and the pend slot
				 * Gap will be seen if the next pend slot is skipped for
				 * any other request
				 */
				if (req_entity->pend_slot.end_time ==
					(req_entity->cur_slot.end_time + req_param->duration)) {
					/* Update the slot end time to pend slot end time */
					req_entity->cur_slot.end_time =
						req_entity->pend_slot.end_time;

					/* Add the new end time to slot end fire time */
					msch_list_sorted_add(&msch_info->msch_req_timing_list,
						&req_entity->cur_slot.link,
						OFFSETOF(msch_req_timing_t, end_time) -
						OFFSETOF(msch_req_timing_t, link),
						sizeof(uint64), MSCH_ASCEND);

					_msch_next_pend_slot(msch_info, req_entity);

					/* Update the cur timeslot if there is change in
					 * end time
					 */
					if (ts->end_time != -1 &&
						ts->end_time < req_entity->cur_slot.end_time) {
						ts->end_time = req_entity->cur_slot.end_time;
					}

					/* Skip slot end notification since we moved the
					 * slot end to next slot
					 */
					next_fire = _msch_get_next_slot_fire(msch_info);
					if (next_fire == -1 ||
						(ts->fire_time != next_fire &&
						(next_fire > cur_time))) {
						/* Done with slot notification */
						return next_fire;
					}
					/* continue with next slot notification in the timming
					 * list
					 */
					continue;
				}
			}
		} else {
			onchan.timeslot_id = ts->timeslot_id;
			onchan.pre_start_time_h = (uint32)(slot->pre_start_time >> 32);
			onchan.pre_start_time_l = (uint32)slot->pre_start_time;
			onchan.end_time_h = (uint32)(slot->end_time >> 32);
			onchan.end_time_l = (uint32)slot->end_time;
			cb_info.type_specific = &onchan;

			if (ts->fire_time != slot->start_time) {
				MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
					"%s fire %s start_time %s cur_time %s\n", __FUNCTION__,
					_msch_display_time(ts->fire_time),
					_msch_display_time(slot->start_time),
					_msch_display_time(cur_time)));
			}

			if (req_entity->req_hdl->flags & MSCH_REQ_HDL_FLAGS_NEW_REQ) {
				req_entity->req_hdl->flags &= ~MSCH_REQ_HDL_FLAGS_NEW_REQ;
				cb_info.type |= MSCH_CT_REQ_START;
			}

			cb_info.type |= MSCH_CT_SLOT_START;
			slot->flags |= MSCH_RC_FLAGS_START_FIRE_DONE;

			/* For BothFlex it is always OFF_PREP */
			if (!MSCH_BOTH_FLEX(req_entity->req_hdl->req_param->req_type))  {
				msch_list_sorted_add(&msch_info->msch_req_timing_list,
					&req_entity->cur_slot.link,
					OFFSETOF(msch_req_timing_t, end_time) -
					OFFSETOF(msch_req_timing_t, link),
					sizeof(uint64), MSCH_ASCEND);
			}
		}

#ifdef MSCH_PROFILER
		_msch_dump_callback(msch_info->profiler, &cb_info);
#endif /* MSCH_PROFILER */
		hdl_before_clbk = req_entity->req_hdl;
		req_entity->req_hdl->cb_func(req_entity->req_hdl->cb_ctxt,
			&cb_info);

		/* possible timeslot unregister can happen in clbk,
		 * check if req_hdl is valid after clbk
		 */
		if (_msch_req_hdl_valid(msch_info, hdl_before_clbk)) {
			if (cb_info.type & MSCH_CT_SLOT_END) {
				if (cb_info.type & MSCH_CT_REQ_END) {
					wlc_msch_timeslot_unregister(msch_info,
						&req_entity->req_hdl);
				} else if (!req_entity->req_hdl->req_param->interval) {
					_msch_req_entity_destroy(msch_info, req_entity);
				} else {
					/* Req complete, unlink from timeslot */
					req_entity->cur_slot.timeslot = NULL;
				}
			}
		}

		next_fire = _msch_get_next_slot_fire(msch_info);
		if (next_fire == -1 ||
			(ts->fire_time != next_fire && next_fire > cur_time)) {
			/* Done with slot notification */
			return next_fire;
		}
	}
	return -1;
}

static bool
_msch_req_hdl_valid(wlc_msch_info_t *msch_info, wlc_msch_req_handle_t *req_hdl)
{
	msch_list_elem_t *elem;
	wlc_msch_req_handle_t *list_hdl;
	/* Check if the req hdl is valid */
	elem = &msch_info->msch_req_hdl_list;
	while ((elem = msch_list_iterate(elem))) {
		list_hdl = LIST_ENTRY(elem, wlc_msch_req_handle_t, link);
		if (list_hdl == req_hdl) {
			/* found valid handle */
			return TRUE;
		}
	}

	return FALSE;
}

/* MSCH timeslot state machine timer */
static void
_msch_chsw_timer(void *arg)
{
	wlc_msch_info_t *msch_info = (wlc_msch_info_t *)arg;
	msch_chan_ctxt_t *cur_chan_ctxt = NULL;
	msch_list_elem_t *req_elem;
	uint64 cur_time;
	msch_req_timing_t *slot;
	msch_timeslot_t *cur_ts = msch_info->cur_msch_timeslot;
	msch_timeslot_t *next_ts = msch_info->next_timeslot;
	msch_req_entity_t *entity = NULL;
	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info);

	cur_time = msch_current_time(msch_info);
	MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
		"%s: current_time: %s\n", __FUNCTION__,
		_msch_display_time(cur_time)));

	msch_info->flags |=  MSCH_STATE_IN_TIEMR_CTXT;
	msch_info->cur_armed_timeslot = NULL;

	if (cur_ts) {
		cur_chan_ctxt = cur_ts->chan_ctxt;
		/* Handle ONCHAN, SLOT START, SLOT_END notification in
		 * ONCHAN_FIRE state
		 */
		if (cur_ts->state == MSCH_TIMER_STATE_ONCHAN_FIRE) {
			/* Check any new request piggybacked to current timeslot */
			req_elem = &cur_ts->chan_ctxt->req_entity_list;
			while ((req_elem = msch_list_iterate(req_elem))) {
				entity = LIST_ENTRY(req_elem, msch_req_entity_t, chan_ctxt_link);
				slot = &entity->cur_slot;
				/* identify new req based on ONFIRE_DONE flag */
				if (!entity->cur_slot.timeslot ||
					entity->cur_slot.flags & MSCH_RC_FLAGS_ONFIRE_DONE) {
					continue;
				}
				_msch_timeslot_chn_clbk(msch_info, cur_ts, MSCH_CT_ON_CHAN);
				_msch_schd_slot_start_end(msch_info, cur_ts);
			}

			if (!msch_list_empty(&msch_info->msch_req_timing_list)) {
				cur_ts->fire_time = _msch_slot_start_end_notif(msch_info, cur_ts);
				if (cur_ts->fire_time != -1) {
					MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
						"%s: cur_ts->fire_time is not infinite\n",
					 __FUNCTION__));
					goto done;
				}
			}
			cur_ts->fire_time = cur_ts->end_time;
			if (cur_ts->end_time != -1) {
				if (cur_ts->end_time > cur_time) {
					MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
						"%s: end_time is greater than cur_time\n",
					 __FUNCTION__));
					goto done;
				}
				/* slot end_time is past, cleanup */
				if (!msch_list_empty(&cur_chan_ctxt->bf_entity_list)) {
						cur_ts->state = MSCH_TIMER_STATE_OFF_CHN_PREP;
				} else {
					cur_ts->state = MSCH_TIMER_STATE_TS_COMPLETE;
				}
			}

			/* prepare for next fire time */
			if (next_ts && next_ts->fire_time <= msch_current_time(msch_info)) {
				if (!msch_list_empty(&cur_chan_ctxt->bf_entity_list)) {
					cur_ts->state = MSCH_TIMER_STATE_OFF_CHN_PREP;
				} else {
					cur_ts->state = MSCH_TIMER_STATE_TS_COMPLETE;
				}
			}
		}
		/* Prepare for next channel, if Both flex entity exist do
		 * OFF_PREP callback
		 */
		if (cur_ts->state == MSCH_TIMER_STATE_OFF_CHN_PREP) {
			cur_chan_ctxt->actual_onchan_dur += (cur_time - cur_chan_ctxt->onchan_time);
			safe_dec(cur_chan_ctxt->pend_onchan_dur, cur_ts->sch_dur);
			_msch_timeslot_chn_clbk(msch_info, cur_ts, MSCH_CT_OFF_CHAN);

			if (next_ts) {
				next_ts->pre_start_time -= MSCH_MAX_OFFCB_DUR;
			}

			cur_ts->fire_time = msch_current_time(msch_info) + MSCH_MAX_OFFCB_DUR;
			cur_ts->state = MSCH_TIMER_STATE_OFF_CHN_DONE;
			goto done;
		}

		/* OFF_DONE notification to move to next channel */
		if (cur_ts->state == MSCH_TIMER_STATE_OFF_CHN_DONE) {
			_msch_timeslot_chn_clbk(msch_info, cur_ts, MSCH_CT_OFF_CHAN_DONE);
			cur_ts->state = MSCH_TIMER_STATE_TS_COMPLETE;
		}

		/* Cur TS complete, cleanup ts, chan ctxt */
		if (cur_ts->state == MSCH_TIMER_STATE_TS_COMPLETE) {
			if (!next_ts || next_ts->chan_ctxt != cur_chan_ctxt) {
				cur_chan_ctxt->bf_sch_pending = FALSE;
			}

			if (msch_list_empty(&cur_ts->chan_ctxt->req_entity_list)) {
				_msch_chan_ctxt_destroy(msch_info, cur_ts->chan_ctxt, TRUE);
				cur_ts->chan_ctxt = NULL;
				_msch_timeslot_free(msch_info, cur_ts, TRUE);
				cur_ts = msch_info->cur_msch_timeslot = NULL;
			} else {
				/* here we can unlink all req entity of cur ts */
				_msch_timeslot_destroy(msch_info, cur_ts, TRUE);
				cur_ts = NULL;
			}
		}
	}

	if (msch_info->cur_msch_timeslot) {
		goto done;
	}

	/* check next timeslot valid */
	if (!msch_info->next_timeslot) {
		/* Schedule item if one of the list is not empty */
		if (!msch_list_empty(&msch_info->msch_start_fixed_list) ||
			!msch_list_empty(&msch_info->msch_start_flex_list) ||
			!msch_list_empty(&msch_info->msch_both_flex_req_entity_list)) {
			if (!msch_info->slotskip_flag) {
				_msch_schd(msch_info);
			}
		}
		msch_info->flags &=  ~MSCH_STATE_IN_TIEMR_CTXT;
		return;
	}

	if (!next_ts->fire_time || next_ts->fire_time == -1) {
		MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
			"%s: fire_time not set: cur_fire %s\n", __FUNCTION__,
			_msch_display_time(next_ts->fire_time)));
		next_ts->fire_time = next_ts->pre_start_time;
	}

	if (cur_time < next_ts->fire_time) {
		/* next tiemslot is not due, schedule timer for fire time */
		goto done;
	}

	/* Switch to channel */
	_msch_chan_adopt(msch_info, next_ts->chan_ctxt->chanspec);
	next_ts->state = MSCH_TIMER_STATE_ONCHAN_FIRE;
	next_ts->chan_ctxt->onchan_time = msch_current_time(msch_info);
	cur_ts = msch_info->cur_msch_timeslot = next_ts;
	next_ts = msch_info->next_timeslot = NULL;

	/* Create the pending slot for periodic request */
	_msch_create_pend_slot(msch_info, cur_ts);

	/* TODO add debug to capture if the callback has consumed too long
	 * cur/next schedule might need to be re evaluated if
	 * the time consumed is more than the cur_ts end_time
	 */
	_msch_timeslot_chn_clbk(msch_info, cur_ts, MSCH_CT_ON_CHAN);

	/* Create req_timing for SLOT START or END */
	_msch_schd_slot_start_end(msch_info, cur_ts);

	cur_ts->fire_time = cur_ts->end_time;
	if ((req_elem = msch_list_iterate(&msch_info->msch_req_timing_list))) {
		slot = LIST_ENTRY(req_elem, msch_req_timing_t, link);
		if (slot->flags & MSCH_RC_FLAGS_START_FIRE_DONE) {
			cur_ts->fire_time = slot->end_time;
		} else {
			cur_ts->fire_time = slot->start_time;
		}
	}
	/*
	 * Only schedule the entity when skip timer is deleted
	 * to avoid unnecessary slot skips
	 */
	if (!msch_info->slotskip_flag) {
		_msch_schd(msch_info);
	}
done:
	cur_time = msch_current_time(msch_info);
	if (cur_ts) {
		if (cur_ts->fire_time != -1) {
			msch_info->cur_armed_timeslot = cur_ts;
			MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
				"%s: fire_time: %s, diff %d\n", __FUNCTION__,
				_msch_display_time(cur_ts->fire_time),
				(int)(cur_ts->fire_time - cur_time)));
			wlc_msch_add_timeout(msch_info->chsw_timer,
				(int)(cur_ts->fire_time - cur_time),
				_msch_chsw_timer, msch_info);
		}
	} else if (next_ts && next_ts->fire_time != -1) {
		msch_info->cur_armed_timeslot = next_ts;
		MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
			"%s: fire_time: %s, diff %d\n", __FUNCTION__,
			_msch_display_time(next_ts->fire_time),
			(int)(next_ts->fire_time - cur_time)));
		wlc_msch_add_timeout(msch_info->chsw_timer,
			(int)(next_ts->fire_time - cur_time),
			_msch_chsw_timer, msch_info);
	}
	msch_info->flags &=  ~MSCH_STATE_IN_TIEMR_CTXT;

}

static void
_msch_next_pend_slot(wlc_msch_info_t *msch_info, msch_req_entity_t *req_entity)
{
	uint64 cur_time = msch_current_time(msch_info);
	wlc_msch_req_param_t *req_param = req_entity->req_hdl->req_param;

	if (MSCH_START_FIXED(req_param->req_type)) {
		msch_list_remove(&req_entity->start_fixed_link);

		/* start time fixed and periodic request */
		if (req_param->interval) {
			wlc_uint64_add(&req_param->start_time_h,
				&req_param->start_time_l, 0, req_param->interval);
			req_entity->pend_slot.start_time =
				((uint64)req_param->start_time_h << 32) |
				req_param->start_time_l;
			req_entity->pend_slot.end_time = req_entity->pend_slot.start_time +
				req_param->duration;

			/* add the req_entity back to the list */
			msch_list_sorted_add(&msch_info->msch_start_fixed_list,
				&req_entity->start_fixed_link,
				OFFSETOF(msch_req_entity_t, pend_slot) -
				OFFSETOF(msch_req_entity_t, start_fixed_link) +
				OFFSETOF(msch_req_timing_t, start_time),
				sizeof(uint64), MSCH_ASCEND);
		}
	} else if (MSCH_START_FLEX(req_param->req_type)) {
		if (req_param->interval) {
			/*
			 * Put the serviced (front) item to the end of list that has
			 * the same priority so that periodic reqeusts can be serviced
			 * in order based on priority
			 */
			_msch_prio_list_rotate(msch_info,
				&req_entity->rt_specific_link,
				OFFSETOF(msch_req_entity_t, rt_specific_link));
		} else {
			msch_list_remove(&req_entity->rt_specific_link);
		}
	} else {
		msch_list_remove(&req_entity->both_flex_list);

		/* both flex, list is not priority based */
		if (req_param->interval) {
			while (req_entity->pend_slot.start_time < cur_time) {
				if (!req_entity->cur_slot.timeslot &&
					!req_entity->chan_ctxt->bf_sch_pending) {
					req_entity->chan_ctxt->bf_skipped_count++;
				}
				wlc_uint64_add(&req_param->start_time_h,
					&req_param->start_time_l, 0, req_param->interval);

				req_entity->pend_slot.start_time =
					((uint64)req_param->start_time_h << 32) |
					req_param->start_time_l;
			}
			req_entity->pend_slot.pre_start_time =
				req_entity->pend_slot.start_time - MSCH_ONCHAN_PREPARE;
			req_entity->pend_slot.end_time = req_entity->pend_slot.start_time +
				req_param->duration;

			msch_list_sorted_add(&msch_info->msch_both_flex_req_entity_list,
				&req_entity->both_flex_list,
				OFFSETOF(msch_req_entity_t, pend_slot) -
				OFFSETOF(msch_req_entity_t, both_flex_list) +
				OFFSETOF(msch_req_timing_t, start_time),
				1, MSCH_ASCEND);
		}
	}
}

static void
_msch_create_pend_slot(wlc_msch_info_t *msch_info, msch_timeslot_t *timeslot)
{
	msch_list_elem_t *elem;
	msch_req_entity_t *req_entity;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(timeslot);

	/* iterate all req_entities for periodic request */
	elem = &timeslot->chan_ctxt->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
		if (req_entity->pend_slot.timeslot != timeslot ||
			req_entity->cur_slot.timeslot != NULL) {
			continue;
		}
		req_entity->cur_slot = req_entity->pend_slot;
		memset(&req_entity->pend_slot, 0, sizeof(msch_req_timing_t));
		_msch_next_pend_slot(msch_info, req_entity);

	}
}

/* function to do ON_CHAN and OFF_CHAN callback for all req in the timeslot */
static void
_msch_timeslot_chn_clbk(wlc_msch_info_t *msch_info, msch_timeslot_t *ts, uint32 clbk_type)
{
	msch_list_elem_t *elem, *prev_elem;
	msch_req_entity_t *req_entity;
	wlc_msch_cb_info_t cb_info;
	wlc_msch_onchan_info_t onchan;
	msch_req_timing_t *slot;
	wlc_msch_req_handle_t *hdl_before_clbk;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(ts);

	memset(&cb_info, 0, sizeof(cb_info));
	cb_info.type = clbk_type;

	elem = &ts->chan_ctxt->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
		slot = &req_entity->cur_slot;
		cb_info.req_hdl = req_entity->req_hdl;
		cb_info.chanspec = req_entity->chanspec;

		if (!req_entity->cur_slot.timeslot) {
			continue;
		}

		if (clbk_type == MSCH_CT_ON_CHAN) {
			if (slot->flags & MSCH_RC_FLAGS_ONFIRE_DONE) {
				continue;
			}
			slot->flags |= MSCH_RC_FLAGS_ONFIRE_DONE;
			cb_info.type = MSCH_CT_ON_CHAN;
			onchan.timeslot_id = ts->timeslot_id;
			onchan.pre_start_time_h = (uint32)(slot->pre_start_time >> 32);
			onchan.pre_start_time_l = (uint32)slot->pre_start_time;
			onchan.end_time_h = (uint32)(slot->end_time >> 32);
			onchan.end_time_l = (uint32)slot->end_time;
			cb_info.type_specific = &onchan;

			if (slot->start_time <= msch_current_time(msch_info)) {
				cb_info.type |= MSCH_CT_SLOT_START;
				slot->flags |= MSCH_RC_FLAGS_START_FIRE_DONE;
			}
			if (req_entity->req_hdl->flags & MSCH_REQ_HDL_FLAGS_NEW_REQ) {
				req_entity->req_hdl->flags &= ~MSCH_REQ_HDL_FLAGS_NEW_REQ;
				cb_info.type |= MSCH_CT_REQ_START;
			}
		}
		if (cb_info.type == MSCH_CT_OFF_CHAN) {
			if (MSCH_BOTH_FLEX(req_entity->req_hdl->req_param->req_type)) {
				req_entity->bf_last_serv_time = msch_current_time(msch_info);
			}
		}
#ifdef MSCH_PROFILER
		_msch_dump_callback(msch_info->profiler, &cb_info);
#endif /* MSCH_PROFILER */
		/* Get the previous elemenent */
		prev_elem = elem->prev;
		hdl_before_clbk = req_entity->req_hdl;
		req_entity->req_hdl->cb_func(req_entity->req_hdl->cb_ctxt,
			&cb_info);

		/* clbk can unregister req, check the req valid */
		if (!_msch_req_hdl_valid(msch_info, hdl_before_clbk)) {
			elem = prev_elem;
		}
	}
}

/* Create a list of all SLOT_STAR/SLOT_END notification for the TS */
static void
_msch_schd_slot_start_end(wlc_msch_info_t *msch_info, msch_timeslot_t *ts)
{
	msch_list_elem_t *elem;
	msch_req_entity_t *req_entity;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(ts);

	elem = &ts->chan_ctxt->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);

		if (!req_entity->cur_slot.timeslot ||
			msch_elem_inlist(&req_entity->cur_slot.link)) {
			continue;
		}

		if (!(req_entity->cur_slot.flags & MSCH_RC_FLAGS_START_FIRE_DONE)) {
			msch_list_sorted_add(&msch_info->msch_req_timing_list,
				&req_entity->cur_slot.link,
				OFFSETOF(msch_req_timing_t, start_time) -
				OFFSETOF(msch_req_timing_t, link),
				sizeof(uint64), MSCH_ASCEND);
		} else if (!(req_entity->cur_slot.flags & MSCH_RC_FLAGS_END_FIRE_DONE) &&
			req_entity->cur_slot.end_time != -1 &&
			!MSCH_BOTH_FLEX(req_entity->req_hdl->req_param->req_type)) {
			msch_list_sorted_add(&msch_info->msch_req_timing_list,
				&req_entity->cur_slot.link,
				OFFSETOF(msch_req_timing_t, end_time) -
				OFFSETOF(msch_req_timing_t, link),
				sizeof(uint64), MSCH_ASCEND);
		}
	}
}

/* Prepare the timer for cur or next timeslot */
static void
_msch_arm_chsw_timer(wlc_msch_info_t *msch_info)
{
	uint64 cur_time = 0, switch_time = 0; /* ms */
	msch_timeslot_t* timeslot = NULL;
	msch_chan_ctxt_t *chan_ctxt = NULL;
	uint64 new_fire_time = 0;
	msch_list_elem_t *elem;
	msch_req_timing_t *slot;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	/* Check if cur_timer need new fire time */
	if (msch_info->cur_msch_timeslot) {
		if ((elem = msch_list_iterate(&msch_info->msch_req_timing_list))) {
			slot = LIST_ENTRY(elem, msch_req_timing_t, link);
			if (slot->flags & MSCH_RC_FLAGS_START_FIRE_DONE) {
				new_fire_time = slot->end_time;
			} else {
				new_fire_time = slot->start_time;
			}
			/*  newly added request to cur_ts might still not be in req_timing list
			 * make sure fire time is earliest
			 */
			if (new_fire_time != -1 &&
				new_fire_time < msch_info->cur_msch_timeslot->fire_time) {
				timeslot = msch_info->cur_msch_timeslot;
				timeslot->fire_time = new_fire_time;
				goto done;
			}
		}
	}

	if (msch_info->cur_msch_timeslot &&
		msch_info->cur_msch_timeslot->fire_time != -1) {
		if (msch_info->next_timeslot && msch_info->cur_msch_timeslot->end_time != -1 &&
			((msch_info->cur_msch_timeslot->end_time >
				msch_info->next_timeslot->pre_start_time))) {
			MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
				"%s:new_ts overlap cur_end %u next_start %u\n",
				__FUNCTION__,
				(uint32)msch_info->cur_msch_timeslot->end_time,
				(uint32)msch_info->next_timeslot->pre_start_time));
			ASSERT(FALSE);
		}
		return;
	}

	if (msch_info->next_timeslot == NULL) {
		MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
			"%s: no next timeslot to schedule\n", __FUNCTION__));
		return;
	}

	msch_info->next_timeslot->fire_time = msch_info->next_timeslot->pre_start_time;
	timeslot = msch_info->next_timeslot;
done:
	cur_time = msch_current_time(msch_info);
	switch_time = (timeslot->fire_time <= cur_time) ? 0 : (timeslot->fire_time - cur_time);
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: current time: %x-%x, switch_time: %x-%x, diff: %u\n",
		__FUNCTION__,
		(uint32)(cur_time >> 32), (uint32)cur_time,
		(uint32)(switch_time >> 32), (uint32)switch_time,
		(uint32)(switch_time - cur_time)));

	/* arm timer */
	ASSERT(msch_info->chsw_timer != NULL);
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: wlc_msch_add_timeout: %d\n", __FUNCTION__,
		(int)(switch_time)));
	/* Done for current timeslot, remove the timer */
	if (msch_info->cur_armed_timeslot) {
		wlc_hrt_del_timeout(msch_info->chsw_timer);
		msch_info->cur_armed_timeslot = NULL;
	}

	msch_info->cur_armed_timeslot = timeslot;
	if (msch_info->cur_msch_timeslot &&
		msch_info->cur_msch_timeslot != timeslot) {
		chan_ctxt = msch_info->cur_msch_timeslot->chan_ctxt;
		if (!msch_list_empty(&chan_ctxt->bf_entity_list)) {
			msch_info->cur_msch_timeslot->state = MSCH_TIMER_STATE_OFF_CHN_PREP;
		} else {
			msch_info->cur_msch_timeslot->state = MSCH_TIMER_STATE_CHN_SW;
		}
		/* Next TS valid, make sure cur_ts has end time.  */
		if (msch_info->cur_msch_timeslot &&
			msch_info->cur_msch_timeslot->end_time == -1) {
			msch_info->cur_msch_timeslot->end_time = timeslot->pre_start_time;
		}
	}
	MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
		"%s : switch_time: %s, diff %d\n", __FUNCTION__,
		_msch_display_time(timeslot->fire_time), (int)(switch_time)));
	/* add the timer for the next schd task */
	wlc_msch_add_timeout(msch_info->chsw_timer, (int)(switch_time),
		_msch_chsw_timer, msch_info);
}

static void
_msch_update_chan_ctxt_bw(wlc_msch_info_t *msch_info, msch_chan_ctxt_t *chan_ctxt)
{
	msch_list_elem_t *elem;
	msch_req_entity_t *entity;
	chanspec_t cur_chanspec = 0;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(chan_ctxt);

	elem = &chan_ctxt->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
		if (!cur_chanspec) {
			cur_chanspec = entity->chanspec;
		}
		if (CHSPEC_BW(entity->chanspec) > CHSPEC_BW(cur_chanspec)) {
			cur_chanspec = entity->chanspec;
		}
	}
	if (cur_chanspec && cur_chanspec != chan_ctxt->chanspec) {
		chan_ctxt->chanspec = cur_chanspec;
	}
}

static void
_msch_update_service_interval(wlc_msch_info_t *msch_info)
{
	msch_list_elem_t *elem, *elem2;
	msch_req_entity_t *entity;
	msch_chan_ctxt_t *tmp_ctxt;
	bool interval_change = FALSE;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	msch_info->max_lo_prio_interval = 0;
	elem = &msch_info->msch_both_flex_list;
	while ((elem = msch_list_iterate(elem))) {
		tmp_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, bf_link);
		elem2 = &tmp_ctxt->bf_entity_list;
		while ((elem2 = msch_list_iterate(elem2))) {
			entity = LIST_ENTRY(elem2, msch_req_entity_t, rt_specific_link);
			if (entity->req_hdl->req_param->interval >
				msch_info->max_lo_prio_interval) {
				msch_info->max_lo_prio_interval =
					entity->req_hdl->req_param->interval;
				interval_change = TRUE;
			}
		}
	}

	if (msch_info->flex_list_cnt > 1) {
		interval_change = TRUE;
		elem = &msch_info->msch_both_flex_list;
		while ((elem = msch_list_iterate(elem))) {
			tmp_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, bf_link);
			tmp_ctxt->actual_onchan_dur = 0;
			tmp_ctxt->pend_onchan_dur = 0;
		}
	}

	if (interval_change) {
		if (msch_info->max_lo_prio_interval == 0 ||
			msch_info->flex_list_cnt == 0) {
				MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
					"%s interval: %d flex_list_cnt %d\n", __FUNCTION__,
					msch_info->max_lo_prio_interval,
					msch_info->flex_list_cnt));
			return;
		}

		msch_info->service_interval = msch_info->max_lo_prio_interval /
			msch_info->flex_list_cnt;
	}
}

static void
_msch_prio_list_rotate(wlc_msch_info_t *msch_info, msch_list_elem_t *list,
		uint32 offset)
{
	msch_req_entity_t *entity, *entity2 = NULL;
	msch_list_elem_t *elem, *prev_elem;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	ASSERT(list);
	ASSERT(msch_elem_inlist(list));

	entity = ((msch_req_entity_t *)((char *)(list) - offset));
	elem = prev_elem = list->next;
	while (elem) {
		entity2 = ((msch_req_entity_t *)((char *)(elem) - offset));
		if (entity->priority == entity2->priority) {
			prev_elem = elem;
			elem = elem->next;
			continue;
		} else {
			break;
		}
	}

	if (entity2) {
		msch_list_remove(list);
		msch_list_add_at(prev_elem, list);
	}
}

/* piggyback a new request to current timeslot */
static void
_msch_curts_schd_new_req(wlc_msch_info_t *msch_info, msch_req_entity_t *entity)
{
	uint64 cur_time = msch_current_time(msch_info);
	if (!msch_info->cur_msch_timeslot) {
		ASSERT(0);
		return;
	}

	entity->pend_slot.timeslot = msch_info->cur_msch_timeslot;
	_msch_create_pend_slot(msch_info, msch_info->cur_msch_timeslot);
	msch_info->cur_msch_timeslot->fire_time = cur_time;
	if (msch_info->cur_armed_timeslot) {
		wlc_hrt_del_timeout(msch_info->chsw_timer);
	}
	msch_info->cur_armed_timeslot = msch_info->cur_msch_timeslot;

	/* callback should not be done on caller context,
	 * Start timer immediately to do callback
	 */
	wlc_msch_add_timeout(msch_info->chsw_timer, (int)(0),
		_msch_chsw_timer, msch_info);
}

/* Algorithm to piggyback a new request on current timeslot chan context */
static int
_msch_schd_new_piggyback_curts(wlc_msch_info_t *msch_info, msch_req_entity_t *new_entity)
{
	uint64 dur_left, curtime = msch_current_time(msch_info);
	wlc_msch_req_param_t *req_param = new_entity->req_hdl->req_param;
	msch_timeslot_t	*cur_ts = msch_info->cur_msch_timeslot;
	msch_timeslot_t *next_ts = msch_info->next_timeslot;
	bool update_curts = TRUE;
	msch_req_entity_t *entity2;
	msch_list_elem_t *elem;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(cur_ts);
	/* if the request time in past don't schedule */
	if (!MSCH_START_FLEX(req_param->req_type) &&
		new_entity->pend_slot.start_time <= curtime) {
		return MSCH_SCHD_NEXT_SLOT;
	}

	if (MSCH_START_FIXED(req_param->req_type)) {
		if (new_entity->pend_slot.start_time > cur_ts->end_time) {
			return MSCH_SCHD_NEXT_SLOT;
		}

		if (new_entity->pend_slot.end_time < cur_ts->end_time) {
			_msch_curts_schd_new_req(msch_info, new_entity);
			return MSCH_OK;
		}
		else if (!next_ts ||
			(next_ts &&
			next_ts->pre_start_time > new_entity->pend_slot.end_time)) {
			cur_ts->end_time = new_entity->pend_slot.end_time;
			_msch_curts_schd_new_req(msch_info, new_entity);
			return MSCH_OK;
		}

		/* check next slot */
		if (new_entity->pend_slot.end_time > cur_ts->end_time &&
			(next_ts != NULL) &&
			(new_entity->pend_slot.end_time > next_ts->pre_start_time)) {

			/* check if we can extend the cur_ts end_time to new_req
			 * end time by cutting next slot
			 */
			elem = &next_ts->chan_ctxt->req_entity_list;
			while ((elem = msch_list_iterate(elem))) {
				entity2 = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
				if (entity2->priority > new_entity->priority &&
					entity2->pend_slot.pre_start_time <
					new_entity->pend_slot.end_time) {

					/* Check if new req duration can be adjusted */
					if (req_param->req_type == MSCH_RT_DUR_FLEX &&
						(entity2->pend_slot.pre_start_time >=
						(new_entity->pend_slot.end_time -
							req_param->flex.dur_flex))) {
						new_entity->pend_slot.end_time =
							entity2->pend_slot.pre_start_time;
						_msch_curts_schd_new_req(msch_info, new_entity);
						return MSCH_OK;
					}
					update_curts = FALSE;
					break;
				}

				if (MSCH_START_FIXED(
					entity2->req_hdl->req_param->req_type)) {
					update_curts = FALSE;
					break;
				}
			}
		}

		if (update_curts) {
			if (next_ts) {
				_msch_ts_send_skip_notif(msch_info, next_ts, FALSE);
				_msch_timeslot_destroy(msch_info, next_ts, FALSE);
				msch_info->next_timeslot = NULL;
			}
			if (cur_ts->end_time < new_entity->pend_slot.end_time) {
				cur_ts->end_time = new_entity->pend_slot.end_time;
			}
			_msch_curts_schd_new_req(msch_info, new_entity);
		}
	}
	else {
		/* !START_FIXED */
		dur_left = (cur_ts->end_time > curtime) ?
			(cur_ts->end_time - curtime) : 0;

		if (dur_left < MSCH_MIN_FREE_SLOT) {
			return MSCH_SCHD_NEXT_SLOT;
		}

		/* since the callback happens in timer, account for the delay */
		dur_left -= MSCH_ONCHAN_PREPARE;
		if (MSCH_START_FLEX(req_param->req_type) &&
			req_param->duration > dur_left) {
			return MSCH_SCHD_NEXT_SLOT;
		}

		new_entity->pend_slot.pre_start_time =
			msch_current_time(msch_info);
		new_entity->pend_slot.start_time =
			new_entity->pend_slot.pre_start_time;
		if (MSCH_BOTH_FLEX(req_param->req_type)) {
			new_entity->pend_slot.end_time = cur_ts->end_time;
		}
		else {
			new_entity->pend_slot.end_time =
				new_entity->pend_slot.start_time + req_param->duration;
		}
		_msch_curts_schd_new_req(msch_info, new_entity);
	}
	return MSCH_OK;
}

/* New request, check if it can be piggyback to cur_ts
 * MSCH_FAIL - memory failure
 * MSCH_OK - no action needed complete the user request
 * MSCH_SCHD_NEXT_SLOT - next timeslot could be invalid so req
 * run the algorithm for next timeslot
 */
static msch_ret_type_t
_msch_schd_new_req(wlc_msch_info_t *msch_info, wlc_msch_req_handle_t *req_hdl)
{
	msch_chan_ctxt_t *chan_ctxt;
	msch_req_entity_t *new_entity, *entity2;
	msch_list_elem_t *elem;
	msch_timeslot_t	*cur_ts = msch_info->cur_msch_timeslot;
	msch_timeslot_t *next_ts = msch_info->next_timeslot;
	wlc_msch_req_param_t *req_param;
	wlc_info_t *wlc = msch_info->wlc;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	elem = msch_list_iterate(&req_hdl->req_entity_list);
	if (!elem) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"wl%d:%s: error entity list empty \n", msch_info->wlc->pub->unit,
			__FUNCTION__));
		return MSCH_FAIL;
	}
	new_entity = LIST_ENTRY(elem, msch_req_entity_t, req_hdl_link);
	req_param = new_entity->req_hdl->req_param;

	if (!msch_info->cur_msch_timeslot) {
		/* No current timeslot so re-run the msch schd algo */
		goto schd_next;
	}

	chan_ctxt = cur_ts->chan_ctxt;
	/* Same channel context, check piggy back */
	if (chan_ctxt == new_entity->chan_ctxt) {
		/* Change in channel bandwidth */
		if (chan_ctxt->chanspec != WLC_BAND_PI_RADIO_CHANSPEC) {
			_msch_chan_adopt(msch_info, chan_ctxt->chanspec);
		}
		if (_msch_schd_new_piggyback_curts(msch_info,
			new_entity) == MSCH_SCHD_NEXT_SLOT) {
			goto schd_next;
		}
		return MSCH_OK;
	} else if (next_ts) {
		if (MSCH_START_FIXED(req_param->req_type) &&
			(new_entity->pend_slot.start_time >
			(next_ts->end_time + MSCH_PREPARE_DUR))) {
			return MSCH_OK;
		}

		/* check priority */
		elem = &next_ts->chan_ctxt->req_entity_list;
		while ((elem = msch_list_iterate(elem))) {
			entity2 = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
			if (entity2->priority > new_entity->priority) {
				return MSCH_OK;
			}
		}
	} else if (MSCH_START_FIXED(req_param->req_type)) {
		/* Make sure the start_time is after cur_ts */
		if (cur_ts->end_time != -1 &&
			cur_ts->end_time >
			(new_entity->pend_slot.start_time - MSCH_PREPARE_DUR)) {
			if (!req_param->interval) {
				return MSCH_FAIL;
			}
			msch_list_remove(&new_entity->start_fixed_link);

			/* Calculate the next slot */
			while (new_entity->pend_slot.start_time <
				(cur_ts->end_time + MSCH_PREPARE_DUR)) {
				new_entity->pend_slot.start_time +=
					req_param->interval;
			}
			new_entity->pend_slot.pre_start_time =
				new_entity->pend_slot.start_time - MSCH_ONCHAN_PREPARE;
			new_entity->pend_slot.end_time =
				new_entity->pend_slot.start_time +
				req_param->duration;

			/* add the req_entity back to the list */
			msch_list_sorted_add(&msch_info->msch_start_fixed_list,
				&new_entity->start_fixed_link,
				OFFSETOF(msch_req_entity_t, pend_slot) -
				OFFSETOF(msch_req_entity_t, start_fixed_link) +
				OFFSETOF(msch_req_timing_t, start_time),
				sizeof(uint64), MSCH_ASCEND);
		}
	}

schd_next:
	if (msch_info->next_timeslot) {
		/* re-schedule next_ts next */
		_msch_timeslot_destroy(msch_info, msch_info->next_timeslot,
			FALSE);
		msch_info->next_timeslot = NULL;
	}

	return MSCH_SCHD_NEXT_SLOT;
}

static void
_msch_sch_fixed(wlc_msch_info_t *msch_info, msch_req_entity_t *entity, uint32 prep_dur)
{
	if (!entity->pend_slot.timeslot) {
		entity->pend_slot.pre_start_time =
			entity->pend_slot.start_time - prep_dur;

		msch_info->next_timeslot = _msch_timeslot_request(msch_info,
			entity->chan_ctxt, entity->req_hdl->req_param,
			&entity->pend_slot);
		entity->pend_slot.timeslot = msch_info->next_timeslot;
	}
}

/* Get the last slot end time for the current timeslot */
static uint64
_msch_curts_slot_endtime(wlc_msch_info_t *msch_info)
{
	uint64 end_time = 0;
	msch_list_elem_t *elem = &msch_info->msch_req_timing_list;
	msch_timeslot_t *cur_ts = msch_info->cur_msch_timeslot;
	msch_req_entity_t *entity;

	ASSERT(cur_ts);

	/* Get valid end time for cur_ts */
	elem = &cur_ts->chan_ctxt->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
		if (entity->cur_slot.timeslot == cur_ts &&
			entity->cur_slot.end_time != -1 &&
			entity->cur_slot.end_time > end_time) {
			end_time = entity->cur_slot.end_time;
		}
	}
	return end_time;
}

/* Get the next slot fire time */
static uint64
_msch_get_next_slot_fire(wlc_msch_info_t *msch_info)
{
	msch_list_elem_t *elem = NULL;
	msch_req_timing_t *slot = NULL;

	if ((elem = msch_list_iterate(&msch_info->msch_req_timing_list))) {
		slot = LIST_ENTRY(elem, msch_req_timing_t, link);

		/* check if the next slot time is due */
		if (slot->flags & MSCH_RC_FLAGS_START_FIRE_DONE) {
			return slot->end_time;
		}
		else {
			return slot->start_time;
		}
	}

	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s, no slot fire pending\n", __FUNCTION__));
	/* If element is not in the list, return max time */
	return -1;
}

static msch_req_entity_t*
_msch_get_next_sfix(wlc_msch_info_t *msch_info, uint64 slot_start, uint64 prep_dur)
{
	msch_req_entity_t *sfix_entity = NULL, *entity = NULL;
	msch_list_elem_t *elem;

	elem = msch_list_iterate(&msch_info->msch_start_fixed_list);
	if (elem) {
		sfix_entity = LIST_ENTRY(elem, msch_req_entity_t, start_fixed_link);

		/* Check Start Fix overlaps */
		while ((elem = msch_list_iterate(elem))) {
			entity = LIST_ENTRY(elem, msch_req_entity_t, start_fixed_link);
			/* If cur sfix is in past move to next one
			 * slot_start_time is pre start time
			 */
			if (sfix_entity->pend_slot.start_time < (slot_start + prep_dur)) {
				sfix_entity = entity;
				continue;
			}

			/* Cur sfix entity should not overlap with
			 * next to be scheduled sfix entity
			 */
			if (sfix_entity->pend_slot.end_time <=
				(entity->pend_slot.start_time - MSCH_PREPARE_DUR)) {
				break;
			}

			if (sfix_entity->priority < entity->priority) {
				sfix_entity = entity;
			}
		}
	}

	return sfix_entity;
}

/* MSCH slot skip timer */
static void
_msch_slotskip_timer(void *arg)
{
	wlc_msch_info_t *msch_info = (wlc_msch_info_t *)arg;
	msch_list_elem_t *req_elem, *prev_elem;
	uint64 cur_time, slot_due, slot_end_time;
	int switch_dur;
	msch_timeslot_t *cur_ts;
	msch_timeslot_t *next_ts;
	msch_req_entity_t *entity = NULL;
	msch_ret_type_t ret;
	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	if (!msch_info) {
		WL_ERROR(("%s: msch_info is NULL\n", __FUNCTION__));
		ASSERT(FALSE);
		return;
	}

	msch_info->slotskip_flag = FALSE;

	cur_time = msch_current_time(msch_info);
	MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
		"%s: current_time: %s\n", __FUNCTION__,
		_msch_display_time(cur_time)));

	req_elem = &msch_info->msch_start_fixed_list;
	while ((req_elem = msch_list_iterate(req_elem))) {
		entity = LIST_ENTRY(req_elem, msch_req_entity_t, start_fixed_link);
		prev_elem = req_elem->prev;

		cur_ts = msch_info->cur_msch_timeslot;
		next_ts = msch_info->next_timeslot;

		if (!next_ts && !cur_ts) {
			MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
				"Current and next time slots are invalid, return\n"));
			_msch_schd(msch_info);
			return;
		}
		if (!next_ts) {
			slot_end_time = cur_ts->end_time;
		} else {
			slot_end_time = next_ts->end_time;
		}

		if (entity->pend_slot.timeslot) {
			continue;
		}

		slot_due = entity->pend_slot.start_time - MSCH_PREPARE_DUR;
		switch_dur = (int)(slot_due - MSCH_SKIP_PREPARE - cur_time);

		if ((cur_ts && cur_ts->end_time == -1) ||
			(next_ts && next_ts->end_time == -1)) {
			WL_ERROR(("Current or next time slot end time is -1!"));
			return;
		}

		if (slot_due >= slot_end_time) {
			break;
		}

		/* skip the slot if
		 * 1. entity's prestart time is less than current time
		 * 2. entity's start time is less than current/next slot end time
		 * and there is not much switch duration left
		 */
		if ((cur_time >= slot_due) || (switch_dur < 0)) {
			ret = _msch_slot_skip(msch_info, entity);
			if (ret != MSCH_TIMESLOT_REMOVED &&
				(entity->req_hdl->req_param->interval == 0)) {
				wlc_msch_timeslot_unregister(msch_info, &entity->req_hdl);
			} else if (ret == MSCH_TIMESLOT_REMOVED) {
				req_elem = prev_elem;
				continue;
			}
			/* restart from head of list */
			req_elem = &msch_info->msch_start_fixed_list;
		} else {
			/* Add time out for the current entity if no skip
			 * is needed at current stage
			 */
			msch_info->slotskip_flag = TRUE;
			wlc_msch_add_timeout(msch_info->slotskip_timer,
				switch_dur, _msch_slotskip_timer, msch_info);
			break;
		}
	}
	if (!msch_info->slotskip_flag && !msch_info->next_timeslot) {
		_msch_schd(msch_info);
	}
}

/*
 * Schedule request for next timeslot
 */
static void
_msch_schd(wlc_msch_info_t *msch_info)
{
	msch_list_elem_t *elem;
	uint64 cur_ts_endtime = 0, dfix_end_time = 0;
	/* Initialize slot_dur to maximum size slot */
	uint64 slot_dur = -1;
	uint32 prep_dur = MSCH_ONCHAN_PREPARE;
	msch_req_entity_t *entity = NULL;
	msch_req_entity_t *sfix_entity = NULL;
	msch_req_entity_t *dfix_entity = NULL;
	msch_req_entity_t *bflex_entity = NULL;
	msch_req_timing_t *sfix_slot = NULL;
	uint8 sfix_prio, dfix_prio, bflex_prio;
	msch_timeslot_t *cur_ts = msch_info->cur_msch_timeslot;
	uint64 slot_start_time = msch_current_time(msch_info);
	bool cur_ts_is_bf = FALSE;
	uint64 bf_slot_dur = -1;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	sfix_prio = dfix_prio = bflex_prio = MSCH_RP_DISC_BCN_FRAME;

	if (msch_info->next_timeslot) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"wl%d: %s: redundant call\n", msch_info->wlc->pub->unit,
			__FUNCTION__));
		return;
	}

	/* Get the next slot start time */
	if (cur_ts) {
		if (cur_ts->end_time != -1) {
			slot_start_time = msch_info->cur_msch_timeslot->end_time;
		} else {
			/* Get cur_ts slot end time */
			cur_ts_endtime = _msch_curts_slot_endtime(msch_info);

			if (cur_ts_endtime) {
				slot_start_time = cur_ts_endtime;
			}
		}
		if (!msch_list_empty(&cur_ts->chan_ctxt->bf_entity_list)) {
			prep_dur += MSCH_MAX_OFFCB_DUR;
			cur_ts_is_bf = TRUE;
		}
	}

	sfix_entity = _msch_get_next_sfix(msch_info, slot_start_time, prep_dur);

	if (sfix_entity) {
		sfix_slot = &sfix_entity->pend_slot;
		sfix_prio = sfix_entity->priority;
		if (sfix_slot->start_time > (prep_dur + slot_start_time + MSCH_PREPARE_DUR)) {
			slot_dur = sfix_slot->start_time -
			MSCH_PREPARE_DUR - prep_dur - slot_start_time;
		} else {
			MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
				"%s: slot duration is small(less than prep dur) keep it 0\n",
				__FUNCTION__));
			slot_dur = 0;
		}
	}

	elem = msch_list_iterate(&msch_info->msch_start_flex_list);
	if (elem) {
		dfix_entity = LIST_ENTRY(elem, msch_req_entity_t, rt_specific_link);
		dfix_prio = dfix_entity->priority;
	}

	/* make sure all entity in BF list are in future time */
	_msch_update_both_flex_list(msch_info, slot_start_time, slot_dur);

	/* Get the next unscheduled bf entity */
	elem = &msch_info->msch_both_flex_req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		entity = LIST_ENTRY(elem, msch_req_entity_t, both_flex_list);
		if (entity->chan_ctxt->bf_sch_pending) {
			continue;
		}
		bflex_entity = entity;
		bflex_prio = bflex_entity->priority;
	}

	/* SCHD START Fix as next slot
	 *  - start time due
	 *  - or cur_time + dur_fixed dur exceeds pending start_fix start time
	 */
	if (dfix_entity) {
		dfix_end_time = slot_start_time + prep_dur +
			dfix_entity->req_hdl->req_param->duration;
	}

	if (sfix_entity && ((sfix_slot->start_time <= (slot_start_time + prep_dur)) ||
		(dfix_entity && ((sfix_slot->start_time - prep_dur) <= dfix_end_time)))) {
		if (sfix_prio >= dfix_prio && sfix_prio >= bflex_prio) {

			/* If the sfix slot is in current timeslot and the priority is same
			 * as dfix/bflex schedule dfix/bflex timeslot
			 */
			if (!sfix_entity->cur_slot.timeslot ||
				(sfix_prio > dfix_prio && sfix_prio > bflex_prio)) {
				/* sfix is due, schedule it as next timeslot */
				_msch_sch_fixed(msch_info, sfix_entity, prep_dur);
				goto done;
			}
		}
	}

	/* SF done, move on to dfix or both flex */
	if (dfix_entity && dfix_prio >= bflex_prio) {
		if (dfix_entity->req_hdl->req_param->duration <= slot_dur ||
			(_msch_check_avail_extend_slot(msch_info, slot_start_time, dfix_prio,
			prep_dur + dfix_entity->req_hdl->req_param->duration + MSCH_PREPARE_DUR) ==
			MSCH_REQ_DUR_AVAIL)) {
			if (!bflex_entity || !_msch_check_both_flex(msch_info,
				dfix_entity->req_hdl->req_param->duration) ||
				dfix_prio > bflex_prio) {
				/* schedule start flex */
				if (_msch_sch_start_flex(msch_info, dfix_entity, slot_start_time,
					prep_dur) == MSCH_OK) {
					msch_info->next_timeslot = dfix_entity->pend_slot.timeslot;
					goto done;
				}
			}
		}
	}

	if (bflex_entity && (slot_dur >= MSCH_MIN_FREE_SLOT)) {
		/* VSDB is in use */
		if ((msch_info->flex_list_cnt > 1) && cur_ts_is_bf &&
			(elem = msch_list_iterate(&cur_ts->chan_ctxt->bf_entity_list))) {
			bf_slot_dur = slot_dur + prep_dur + MSCH_PREPARE_DUR +
				(bflex_entity->pend_slot.start_time - slot_start_time);
		} else {
			bf_slot_dur = slot_dur + prep_dur + MSCH_PREPARE_DUR;
		}
		if (_msch_check_avail_extend_slot(msch_info, slot_start_time,
			bflex_entity->priority, bf_slot_dur) == MSCH_REQ_DUR_AVAIL) {

			if ((msch_info->flex_list_cnt > 1) && cur_ts_is_bf && elem) {
				slot_start_time = bflex_entity->pend_slot.start_time - prep_dur;
				entity = LIST_ENTRY(elem, msch_req_entity_t, rt_specific_link);
				entity->pend_slot.end_time = slot_start_time;
				if (entity->cur_slot.timeslot == cur_ts) {
					entity->cur_slot.end_time = slot_start_time;
				}
				cur_ts->end_time = slot_start_time;
			}
			if (_msch_sch_both_flex(msch_info, slot_start_time, slot_dur,
				bflex_entity, prep_dur) == MSCH_OK) {
				goto done;
			}
		}
	}

	/* Nothing is due so go back to start fix */
	if (sfix_entity) {
		_msch_sch_fixed(msch_info, sfix_entity, prep_dur);
		goto done;
	}

	return;

done:
	if (msch_info->next_timeslot) {
		uint64 next_end_time = msch_info->next_timeslot->end_time;
		slot_dur = next_end_time - msch_info->next_timeslot->pre_start_time - prep_dur;
		_msch_schd_next_chan_ctxt(msch_info, msch_info->next_timeslot, prep_dur, slot_dur);
		if (msch_info->cur_msch_timeslot &&
			msch_info->cur_msch_timeslot->end_time == -1) {
			msch_info->cur_msch_timeslot->end_time =
				msch_info->next_timeslot->pre_start_time;
		}

		/* Use slot skip timer to schedule slot skip */

		/* Get the next Start Fixed from list */
		elem = &msch_info->msch_start_fixed_list;
		while ((elem = msch_list_iterate(elem))) {
			entity = LIST_ENTRY(elem, msch_req_entity_t, start_fixed_link);
			/* the entity is already scheduled */
			if (entity->pend_slot.timeslot) {
				continue;
			}

			if (entity->pend_slot.start_time > (next_end_time + MSCH_PREPARE_DUR)) {
				break;
			}

			if (msch_info->slotskip_flag) {
				/* delete the timer */
				wlc_hrt_del_timeout(msch_info->slotskip_timer);
			}
			msch_info->slotskip_flag = TRUE;
			wlc_msch_add_timeout(msch_info->slotskip_timer,
				0, _msch_slotskip_timer, msch_info);
			break;
		}
	}
	_msch_arm_chsw_timer(msch_info);
}

static void
_msch_schd_next_chan_ctxt(wlc_msch_info_t *msch_info, msch_timeslot_t *ts,
	uint32 prep_dur, uint64 slot_dur)
{
	msch_list_elem_t *elem;
	msch_req_entity_t *entity;
	uint64 new_end_time = 0;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	/* TODO : piggyback is allowed only for both flexible
	 * revisit after the TXQ changes (TXQ per SCB)
	 */
	if (!ts->chan_ctxt->bf_sch_pending) {
		/* Don't piggyback non bf slot */
		return;
	}

	/* iterate all req_entities for periodic request */
	elem = &ts->chan_ctxt->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);

		/* More than 1 channel conext for single request,
		 * sharring ts might change the order of the request
		 */
		if (entity->req_hdl->chan_cnt > 1) {
			continue;
		}

		/* START_FIXED - don't share timeslot
		 * START_FLEX - ts can be shared but if not schd
		 * BOTH_FLEX - ts shared, can exist in cur and next timeslot
		 */
		if (MSCH_START_FIXED(entity->req_hdl->req_param->req_type) ||
			(MSCH_START_FLEX(entity->req_hdl->req_param->req_type) &&
			entity->cur_slot.timeslot)) {
			continue;
		}

		if (entity->pend_slot.timeslot != NULL) {
			continue;
		}

		if (MSCH_BOTH_FLEX(entity->req_hdl->req_param->req_type) ||
			entity->req_hdl->req_param->duration <= slot_dur) {
			if (entity->chan_ctxt->bf_sch_pending &&
				entity->pend_slot.timeslot) {
				continue;
			}

			entity->pend_slot.timeslot = ts;
			entity->pend_slot.pre_start_time = ts->pre_start_time;
			entity->pend_slot.start_time = ts->pre_start_time + prep_dur;

			if (MSCH_START_FLEX(entity->req_hdl->req_param->req_type)) {
				entity->pend_slot.end_time = (entity->pend_slot.start_time +
				entity->req_hdl->req_param->duration);
			}
			else {
				entity->chan_ctxt->bf_sch_pending = TRUE;
				if (slot_dur != -1) {
					entity->pend_slot.end_time =
						entity->pend_slot.start_time + slot_dur;
				}
				else {
					entity->pend_slot.end_time = -1;
				}
			}
		}
		new_end_time = entity->pend_slot.end_time;
	}

	if (ts->end_time < new_end_time) {
		ts->end_time = new_end_time;
	}
}

/* check if the duration can be extended by skipping some
 * more start fixed slots based on requested priority
 */
static msch_ret_type_t
_msch_check_avail_extend_slot(wlc_msch_info_t *msch_info, uint64 slot_start,
	uint8 req_prio, uint64 req_dur)
{
	msch_req_entity_t *sfix_entity;
	msch_list_elem_t *elem = &msch_info->msch_start_fixed_list;

	while ((elem = msch_list_iterate(elem))) {
		sfix_entity = LIST_ENTRY(elem, msch_req_entity_t, start_fixed_link);

		/* ignore slots before the slot start */
		if (slot_start > sfix_entity->pend_slot.start_time) {
			continue;
		}
		/* Got requested duration, return success */
		if (req_dur <= (sfix_entity->pend_slot.start_time - slot_start)) {
			return MSCH_REQ_DUR_AVAIL;
		}
		if (sfix_entity->priority > req_prio) {
			return MSCH_NO_SLOT;
		}
	}
	return MSCH_REQ_DUR_AVAIL;
}

static bool
wlc_msch_add_timeout(wlc_hrt_to_t *to, int timeout, wlc_hrt_to_cb_fn fun, void *arg)
{
	if (timeout <= 0) {
		timeout = 1;
	}
	/* hrt_add_timeout timer does not support 0 */
	return wlc_hrt_add_timeout(to, timeout, fun, arg);
}

/* start_flex list valid or vsdb (more than 1 both flex chan_ctxt)
 * return chan_ctxt pending as TRUE
 */
static bool
_msch_check_pending_chan_ctxt(wlc_msch_info_t *msch_info, msch_chan_ctxt_t *cur_ctxt)
{
	msch_list_elem_t *elem;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	if (!msch_list_empty(&msch_info->msch_start_flex_list)) {
		/* Pending Start flex req */
		return TRUE;
	}

	elem = &msch_info->msch_both_flex_list;
	while ((elem = msch_list_iterate(elem))) {
		msch_chan_ctxt_t *chan_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, bf_link);
		if (chan_ctxt != cur_ctxt) {
			/* VSDB (2 both flex chan ctxt exist) */
			/* TODO  just check the bf count instead */
			return TRUE;
		}
	}
	return FALSE;
}

static void
_msch_update_both_flex_list(wlc_msch_info_t *msch_info, uint64 slot_start, uint64 slot_dur)
{
	msch_list_elem_t *elem;
	msch_req_entity_t *req_entity = NULL, *next_entity = NULL;
	uint64 cur_time = msch_current_time(msch_info);
	uint64 next_bf_pre_start = slot_start + slot_dur;

	elem = &msch_info->msch_both_flex_req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, both_flex_list);
		/* check if the request is past */
		if (req_entity->pend_slot.start_time < cur_time) {
			/* Get next BF element */
			if (elem->next) {
				next_entity = LIST_ENTRY(elem->next, msch_req_entity_t,
					both_flex_list);
				next_bf_pre_start = next_entity->pend_slot.start_time -
					MSCH_PREPARE_DUR;
			}

			/* check if we can still give some time for the current
			 * both flex without exceeding the next both flex
			 * start
			 */
			if ((req_entity->pend_slot.end_time < slot_start) &&
				(slot_start > next_bf_pre_start) &&
				((slot_start - next_bf_pre_start) > MSCH_MIN_FREE_SLOT)) {
				break;
			}

			/* Make sure the request is in the future. */
			_msch_next_pend_slot(msch_info, req_entity);
			elem = &msch_info->msch_both_flex_req_entity_list;
			continue;
		}
	}
}

static bool
_msch_check_both_flex(wlc_msch_info_t *msch_info, uint32 req_dur)
{
	msch_list_elem_t *elem, *elem2;
	msch_req_entity_t *req_entity = NULL;
	msch_req_entity_t *next_entity = NULL;
	uint64 cur_time = msch_current_time(msch_info);
	uint64 min_dur = -1;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	elem = &msch_info->msch_both_flex_req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, both_flex_list);

		/* Already scheduled as cur or next timeslot */
		if (req_entity->chan_ctxt->bf_sch_pending) {
			/* Check the next entry */
			continue;
		}

		/* Condition to schedule a Both flex entity
		 *  - First time schedule or connection in VSDB mode skipped too long.
		 *  or a both flex start time due
		 */
		if (!req_entity->bf_last_serv_time ||
			(msch_info->flex_list_cnt > 1 &&
			req_entity->chan_ctxt->bf_skipped_count > MAX_BOTH_FLEX_SKIP_COUNT)) {
			req_entity->chan_ctxt->bf_skipped_count = 0;
			return TRUE;
		}

		/* Check if other connection is due */
		if (msch_info->flex_list_cnt > 1) {
			/* Get time to next BF start */
			elem2 = elem;
			while ((elem2 = msch_list_iterate(elem2))) {
				next_entity = LIST_ENTRY(elem2, msch_req_entity_t, both_flex_list);
				if (next_entity->chan_ctxt == req_entity->chan_ctxt) {
					continue;
				}

				if (next_entity->pend_slot.start_time > cur_time) {
					min_dur = cur_time - next_entity->pend_slot.start_time;
					min_dur -= MSCH_PREPARE_DUR;
				}
				min_dur = MIN(min_dur,
					req_entity->req_hdl->req_param->flex.bf.min_dur);
			}

			/* cur BF slot need to be skipped */
			if (cur_time >= (req_entity->pend_slot.start_time + min_dur)) {
				memset(&req_entity->pend_slot, 0, sizeof(msch_req_timing_t));
				_msch_next_pend_slot(msch_info, req_entity);

				/* list is updated so check from the begining */
				elem = &msch_info->msch_both_flex_req_entity_list;
				continue;
			}

			if (req_entity->pend_slot.start_time > (cur_time + req_dur)) {
				/* BF not due */
				return FALSE;
			}
		}

		/* reached max_away limit, schd_bf */
		if (req_entity->req_hdl->req_param->flex.bf.max_away_dur <
			((cur_time - req_entity->bf_last_serv_time)
				+ req_dur)) {
				return TRUE;
		}
		break;
	}
	return FALSE;
}

/* MSCH Exported Functions starts here */

wlc_msch_info_t*
BCMATTACHFN(wlc_msch_attach)(wlc_info_t *wlc)
{
	wlc_msch_info_t *msch_info = NULL;

	msch_info = MALLOCZ(wlc->osh, sizeof(wlc_msch_info_t));
	if (!msch_info) {
		WL_ERROR(("wl%d: %s: msch_info mallocz failed\n",
			wlc->pub->unit, __FUNCTION__));
		return NULL;
	}
	MSCH_ASSIGN_MAGIC_NUM(msch_info, MSCH_MAGIC_NUMBER);
	msch_info->wlc = wlc;


	msch_info->chsw_timer = wlc_hrt_alloc_timeout(wlc->hrti);
	if (!msch_info->chsw_timer) {
		WL_ERROR(("wl%d: %s: wlc_hrt_alloc_timeout failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	msch_info->slotskip_timer = wlc_hrt_alloc_timeout(wlc->hrti);
	if (!msch_info->slotskip_timer) {
		WL_ERROR(("wl%d: %s: wlc_hrt_alloc_timeout for slot skip timer failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#ifdef MSCH_PROFILER
	msch_info->profiler = wlc_msch_profiler_attach(msch_info);
	if (msch_info->profiler == NULL) {
		WL_ERROR(("wl%d: %s: wlc_msch_profiler_attach() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif /* MSCH_PROFILER */

	return msch_info;

fail:

	wlc_msch_detach(msch_info);

	return NULL;
}

void
BCMATTACHFN(wlc_msch_detach)(wlc_msch_info_t *msch_info)
{
	msch_list_elem_t *elem;

	if (!msch_info) {
		WL_ERROR(("%s: msch_info is NULL\n", __FUNCTION__));
		return;
	}
	ASSERT(MSCH_CHECK_MAGIC(msch_info));

#ifdef MSCH_PROFILER
	wlc_msch_profiler_detach(msch_info->profiler);
#endif /* MSCH_PROFILER */

	if (msch_info->chsw_timer) {
		wlc_hrt_free_timeout(msch_info->chsw_timer);
		msch_info->chsw_timer = NULL;
	}

	if (msch_info->slotskip_timer) {
		wlc_hrt_free_timeout(msch_info->slotskip_timer);
		msch_info->slotskip_timer = NULL;
	}

	/* free global list */
	while ((elem = msch_list_remove_head(&msch_info->msch_req_hdl_list))) {
		wlc_msch_req_handle_t *req_hdl = LIST_ENTRY(elem, wlc_msch_req_handle_t, link);
		wlc_msch_timeslot_unregister(msch_info, &req_hdl);
	}

	while ((elem = msch_list_remove_head(&msch_info->msch_chan_ctxt_list))) {
		msch_chan_ctxt_t *chan_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, link);
		_msch_chan_ctxt_free(msch_info, chan_ctxt, FALSE);
	}

	/* free current and next time slot */
	if (msch_info->cur_msch_timeslot) {
		_msch_timeslot_free(msch_info, msch_info->cur_msch_timeslot, FALSE);
	}

	if (msch_info->next_timeslot) {
		_msch_timeslot_free(msch_info, msch_info->next_timeslot, FALSE);
	}

	/* free memory pool */
	while ((elem = msch_list_remove_head(&msch_info->free_req_hdl_list))) {
		wlc_msch_req_handle_t *req_hdl = LIST_ENTRY(elem, wlc_msch_req_handle_t, link);
		_msch_req_handle_free(msch_info, req_hdl, FALSE);
	}

	while ((elem = msch_list_remove_head(&msch_info->free_req_entity_list))) {
		msch_req_entity_t *req_entity = LIST_ENTRY(elem, msch_req_entity_t, req_hdl_link);
		_msch_req_entity_free(msch_info, req_entity, FALSE);
	}

	while ((elem = msch_list_remove_head(&msch_info->free_chan_ctxt_list))) {
		msch_chan_ctxt_t *chan_ctxt = LIST_ENTRY(elem, msch_chan_ctxt_t, link);
		_msch_chan_ctxt_free(msch_info, chan_ctxt, FALSE);
	}

	MFREE(msch_info->wlc->osh, msch_info, sizeof(wlc_msch_info_t));
}


/* MSCH runs on PMU timer which starts when the device is powered on.
 * All the time is represented in uint64 so it is almost take 1/2
 * millon years to wrap around
 */
int
wlc_msch_timeslot_register(wlc_msch_info_t *msch_info, chanspec_t* chanspec_list,
	int chanspec_cnt, wlc_msch_callback cb_func, void *cb_ctxt, wlc_msch_req_param_t *req_param,
	wlc_msch_req_handle_t **p_req_hdl)
{
	wlc_msch_req_handle_t *req_hdl = NULL;
	int chanspec_num = 0, ret = BCME_OK;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info && chanspec_list && req_param && p_req_hdl);
	ASSERT(MSCH_CHECK_MAGIC(msch_info));

	req_hdl = _msch_req_handle_alloc(msch_info);
	if (!req_hdl) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
		"%s: msch_req_handle_alloc failed!!\n", __FUNCTION__));
		return BCME_NOMEM;
	}
	req_hdl->cb_func = cb_func;
	req_hdl->cb_ctxt = cb_ctxt;
	req_hdl->req_param = MALLOCZ(msch_info->wlc->osh, sizeof(wlc_msch_req_param_t));
	req_hdl->chan_cnt = chanspec_cnt;
	req_hdl->flags |= MSCH_REQ_HDL_FLAGS_NEW_REQ;

	if (req_hdl->req_param == NULL) {
		_msch_req_handle_free(msch_info, req_hdl, TRUE);
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"%s: req_param alloc failed!!\n", __FUNCTION__));
		return BCME_NOMEM;
	}
	memcpy(req_hdl->req_param, req_param, sizeof(*req_hdl->req_param));

	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"chanspec_list:\n"));
	for (chanspec_num = 0; chanspec_num < chanspec_cnt; chanspec_num++) {
		MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
			"[%02d] 0x%04x(%d)\n", chanspec_num, chanspec_list[chanspec_num],
			(chanspec_list[chanspec_num] & 0xFF)));
	}

	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"flags = 0x%04x\treq_type = %d\tpriority = %d\n",
		req_hdl->req_param->flags, req_hdl->req_param->req_type,
		req_hdl->req_param->priority));
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"start_time = %x-%x\tduration = %d\tinterval = %d\n",
		req_hdl->req_param->start_time_h, req_hdl->req_param->start_time_l,
		req_hdl->req_param->duration, req_hdl->req_param->interval));

	if (req_hdl->req_param->req_type == MSCH_RT_DUR_FLEX) {
		MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
			"dur_flex = %d\n", req_hdl->req_param->flex.dur_flex));
	} else if (MSCH_BOTH_FLEX(req_hdl->req_param->req_type)) {
		MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
			"min_dur = %d\tmax_away_dur = %d\n",
			req_hdl->req_param->flex.bf.min_dur,
			req_hdl->req_param->flex.bf.max_away_dur));
		MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
			"hi_prio_time = %x-%x\thi_prio_interval = %d\n",
			req_hdl->req_param->flex.bf.hi_prio_time_h,
			req_hdl->req_param->flex.bf.hi_prio_time_l,
			req_hdl->req_param->flex.bf.hi_prio_interval));
	}

#ifdef MSCH_PROFILER
	_msch_dump_register(msch_info->profiler, chanspec_list, chanspec_cnt,
		req_hdl->req_param);
#endif /* MSCH_PROFILER */

	if (req_param->flags & MSCH_REQ_FLAGS_CHAN_CONTIGUOUS) {
		ASSERT(MSCH_START_FIXED(req_param->req_type));
		ASSERT(req_param->duration > MSCH_ONCHAN_PREPARE);
		req_param->duration -= MSCH_ONCHAN_PREPARE;
	}

	/* if still have multiple chanspecs here, it must bf CHAN_CONTIGUOUS */
	MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON,
		"%s: req_entity_create, chanspec_cnt %d\n", __FUNCTION__,
		chanspec_cnt));
	for (chanspec_num = 0; chanspec_num < chanspec_cnt; chanspec_num++) {
		ret = _msch_req_entity_create(msch_info, chanspec_list[chanspec_num],
			req_param, req_hdl);
		if (ret != BCME_OK) {
			MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
				"%s: _msch_req_entity_create failed\n", __FUNCTION__));
			goto fail;
		}

		/* calculate new start time for next chanspec */
		wlc_uint64_add(&req_param->start_time_h, &req_param->start_time_l, 0,
			req_param->duration + MSCH_ONCHAN_PREPARE);
	}

	/* queue req_hdl to msch_info->msch_reqhandle_list */
	msch_list_sorted_add(&msch_info->msch_req_hdl_list, &req_hdl->link, 0, 0, MSCH_NO_ORDER);

#ifdef MSCH_PROFILER
	_msch_dump_profiler(msch_info->profiler);
#endif /* MSCH_PROFILER */

	/* schedule timer now */
	MSCH_MESSAGE((msch_info, MSCH_INFORM_ON,
		"%s: schedule timer now\n", __FUNCTION__));
	/* run next timeslot scheduling if MSCH_STATE not in Timer_CTXT */
	if (!(msch_info->flags & MSCH_STATE_IN_TIEMR_CTXT)) {
		if ((ret = _msch_schd_new_req(msch_info, req_hdl)) == MSCH_SCHD_NEXT_SLOT ||
			!msch_info->next_timeslot) {
			_msch_schd(msch_info);
		}
	}

	if (ret != MSCH_FAIL) {
		*p_req_hdl = req_hdl;
		return BCME_OK;
	}
fail:
	wlc_msch_timeslot_unregister(msch_info, &req_hdl);

	return ret;
}

int
wlc_msch_timeslot_unregister(wlc_msch_info_t *msch_info, wlc_msch_req_handle_t **p_req_hdl)
{
	msch_list_elem_t *elem;
	msch_req_entity_t *req_entity;
	wlc_msch_req_handle_t *req_hdl;
	msch_timeslot_t *ts = NULL;
	msch_chan_ctxt_t *chan_ctxt;
	bool timer_ctxt = TRUE;
	msch_req_timing_t *slot;
	bool unlink_bf_chan_ctxt = TRUE;
	uint64 cur_end_time = 0;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info && p_req_hdl);
	ASSERT(MSCH_CHECK_MAGIC(msch_info));

	req_hdl = *p_req_hdl;
	if (!req_hdl->req_param) {
		WL_ERROR(("wl%d: %s: invalid req param\n",
			msch_info->wlc->pub->unit, __FUNCTION__));
		return BCME_ERROR;
	}

	if (!_msch_req_hdl_valid(msch_info, req_hdl)) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"wl%d: %s: Invalid req_hdl\n", msch_info->wlc->pub->unit,
			__FUNCTION__));
		*p_req_hdl = NULL;
		return BCME_OK;
	}
	timer_ctxt = (msch_info->flags & MSCH_STATE_IN_TIEMR_CTXT);

#ifdef MSCH_TESTING
	_msch_timeslot_unregister(msch_info->profiler, req_hdl);
#endif /* MSCH_TESTING */

	if (msch_info->cur_msch_timeslot) {
		cur_end_time = msch_info->cur_msch_timeslot->end_time;
	}

	/* Non contionous req_entity are not part of req_hdl list, check it
	 * from current timeslot list
	 */
	if (!req_hdl->req_param->interval && msch_info->cur_msch_timeslot) {
		elem = &msch_info->msch_req_timing_list;
		while ((elem = msch_list_iterate(elem))) {
			slot = LIST_ENTRY(elem, msch_req_timing_t, link);
			req_entity = LIST_ENTRY(slot, msch_req_entity_t, cur_slot);
			if (req_entity->req_hdl == req_hdl) {
				elem = elem->prev;
				ts = msch_info->cur_msch_timeslot;
				chan_ctxt = req_entity->chan_ctxt;
				ASSERT(msch_info->cur_msch_timeslot ==
					req_entity->cur_slot.timeslot);
				_msch_req_entity_destroy(msch_info, req_entity);
				if (!timer_ctxt) {
					if (ts) {
						_msch_timeslot_check(msch_info, ts);
					}
					if (chan_ctxt &&
						msch_list_empty(&chan_ctxt->req_entity_list)) {
						_msch_chan_ctxt_destroy(msch_info, chan_ctxt, TRUE);
					}
					ts = NULL;
				}
			}
		}
	}

	/* revert all requested timeslot, delist from chan_ctxt, req_hdl list */
	while ((elem = msch_list_remove_head(&req_hdl->req_entity_list))) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, req_hdl_link);
		ts = NULL;
		chan_ctxt = req_entity->chan_ctxt;
		if (req_entity->cur_slot.timeslot) {
			ASSERT(ts == NULL);
			ts = req_entity->cur_slot.timeslot;
			req_entity->cur_slot.timeslot = NULL;
		}

		if (req_entity->pend_slot.timeslot) {
			ASSERT((msch_info->next_timeslot == req_entity->pend_slot.timeslot));
			if (msch_info->next_timeslot) {
				_msch_timeslot_destroy(msch_info, msch_info->next_timeslot, FALSE);
				msch_info->next_timeslot = NULL;
			}
		}

		_msch_req_entity_destroy(msch_info, req_entity);

		/* Check the channel context entity list, if no more bf entity
		 * unlink channel context from list
		 */
		if (MSCH_BOTH_FLEX(req_hdl->req_param->req_type)) {
			elem = &chan_ctxt->bf_entity_list;
			while ((elem = msch_list_iterate(elem))) {
				req_entity = LIST_ENTRY(elem, msch_req_entity_t, rt_specific_link);
				if (MSCH_BOTH_FLEX(req_entity->req_hdl->req_param->req_type)) {
					unlink_bf_chan_ctxt = FALSE;
				}
			}

			if (unlink_bf_chan_ctxt) {
				 msch_list_remove(&chan_ctxt->bf_link);
			 }
		}

		if (!timer_ctxt) {
			if (ts) {
				_msch_timeslot_check(msch_info, ts);
			}
			if (chan_ctxt &&
				msch_list_empty(&chan_ctxt->req_entity_list)) {
				/* Free the cur_ts before destroying channel context */
				if (msch_info->cur_msch_timeslot &&
					msch_info->cur_msch_timeslot->chan_ctxt == chan_ctxt) {
					_msch_timeslot_free(msch_info,
						msch_info->cur_msch_timeslot, TRUE);
					msch_info->cur_msch_timeslot = NULL;
				}
				_msch_chan_ctxt_destroy(msch_info, chan_ctxt, TRUE);
			}
			ts = NULL;
		}
	}

	if (MSCH_BOTH_FLEX(req_hdl->req_param->req_type)) {
		msch_info->flex_list_cnt = msch_list_length(&msch_info->msch_both_flex_list);
		_msch_update_service_interval(msch_info);
	}

	msch_list_remove(&req_hdl->link);

	MFREE(msch_info->wlc->osh, req_hdl->req_param, sizeof(*(req_hdl->req_param)));
	req_hdl->req_param = NULL;

	/* free req_hdl */
	_msch_req_handle_free(msch_info, req_hdl, TRUE);
	*p_req_hdl = NULL;

	/* Current TS valid before unregister */
	if (cur_end_time && !msch_info->slotskip_flag && !timer_ctxt) {
		if (!msch_info->cur_msch_timeslot ||
			(msch_info->cur_msch_timeslot->end_time != cur_end_time)) {
			if (msch_info->next_timeslot) {
				_msch_timeslot_destroy(msch_info, msch_info->next_timeslot,
					FALSE);
				msch_info->next_timeslot = NULL;
			}
		}
	}

	/* Change in pending request (user request unregisterd)
	 * re-schedule next ts unless skip or timeslot timer active
	 * mark cur_ts end to identify change in time slot after unregister
	 */
	if (!msch_info->slotskip_flag && !timer_ctxt &&
		!msch_info->next_timeslot) {
		_msch_schd(msch_info);
	} else if (msch_info->slotskip_flag) {
		wlc_hrt_del_timeout(msch_info->slotskip_timer);
		wlc_msch_add_timeout(msch_info->slotskip_timer,
			0, _msch_slotskip_timer, msch_info);
	}
	return BCME_OK;
}

int
wlc_msch_timeslot_cancel(wlc_msch_info_t *msch_info, wlc_msch_req_handle_t *req_hdl,
	uint32 timeslot_id)
{
	msch_req_entity_t *req_entity = NULL;
	msch_list_elem_t *elem = NULL;
	msch_timeslot_t *timeslot = NULL;
	uint64 old_end_time = 0;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info && req_hdl);
	ASSERT(MSCH_CHECK_MAGIC(msch_info));

	elem = &req_hdl->req_entity_list;
	while ((elem = msch_list_iterate(elem))) {
		req_entity = LIST_ENTRY(elem, msch_req_entity_t, req_hdl_link);

		timeslot = req_entity->cur_slot.timeslot;
		if (timeslot && (timeslot->timeslot_id == timeslot_id)) {
			ASSERT((timeslot == msch_info->cur_msch_timeslot));
			old_end_time = msch_info->cur_msch_timeslot->end_time;

			msch_list_remove(&req_entity->cur_slot.link);
			memset(&req_entity->cur_slot, 0, sizeof(req_entity->cur_slot));

			if (!req_entity->req_hdl->req_param->interval) {
				req_entity->req_hdl->chan_cnt--;
				if (!req_entity->req_hdl->chan_cnt) {
					wlc_msch_timeslot_unregister(msch_info, &req_hdl);
					req_hdl = NULL;
				} else {
					_msch_req_entity_destroy(msch_info, req_entity);
					req_entity = NULL;
					_msch_timeslot_check(msch_info, timeslot);
				}
			}
			break;
		}
	}

	if (msch_info->slotskip_flag) {
		wlc_hrt_del_timeout(msch_info->slotskip_timer);
		wlc_msch_add_timeout(msch_info->slotskip_timer,
			0, _msch_slotskip_timer, msch_info);
	}

	/* change in current timeslot - destroy the next timeslot
	 * and schedule a new next timeslot
	 */
	if (old_end_time && (!msch_info->cur_msch_timeslot ||
		(old_end_time != msch_info->cur_msch_timeslot->end_time))) {
		if (msch_info->next_timeslot) {
			_msch_timeslot_destroy(msch_info, msch_info->next_timeslot,
				FALSE);
			msch_info->next_timeslot = NULL;
			_msch_schd(msch_info);
		}
	}

	return BCME_OK;
}

int
wlc_msch_timeslot_update(wlc_msch_info_t *msch_info, wlc_msch_req_handle_t *p_req_hdl,
	wlc_msch_req_param_t *param, uint32 update_mask)
{
	msch_req_entity_t *entity;
	msch_list_elem_t *elem;
	uint64 start_time;
	uint64 cur_time;
	wlc_msch_req_type_t param_req_type;

	ASSERT(msch_info);
	ASSERT(MSCH_CHECK_MAGIC(msch_info));

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));

	ASSERT((p_req_hdl && param));

	if (!p_req_hdl || !param) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"wl%d: %s: req_hdl NULL\n", msch_info->wlc->pub->unit,
			__FUNCTION__));
		return BCME_ERROR;
	}

	if (!_msch_req_hdl_valid(msch_info, p_req_hdl)) {
		MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
			"wl%d: %s: Invalid req_hdl\n", msch_info->wlc->pub->unit,
			__FUNCTION__));
		return BCME_ERROR;
	}

	MSCH_MESSAGE((msch_info, MSCH_DEBUG_ON, "%s: start time %s\n", __FUNCTION__,
		_msch_display_time(((uint64)param->start_time_h << 32) + param->start_time_l)));

	param_req_type = p_req_hdl->req_param->req_type;
	elem = msch_list_iterate(&p_req_hdl->req_entity_list);
	ASSERT(elem);

	entity = LIST_ENTRY(elem, msch_req_entity_t, req_hdl_link);

	if (update_mask & MSCH_UPDATE_INTERVAL) {
		entity->req_hdl->req_param->interval = param->interval;
	}

	do {
		if (update_mask & MSCH_UPDATE_START_TIME) {
			start_time = (((uint64)param->start_time_h << 32) | param->start_time_l);
			cur_time = msch_current_time(msch_info);
			if (start_time <= cur_time) {
				MSCH_MESSAGE((msch_info, MSCH_ERROR_ON,
					"%s: start time %u is less than current time %u\n",
					__FUNCTION__, (uint32)start_time, (uint32)cur_time));
				ASSERT(FALSE);
			}

			/* Support Start time update for START fixed and Both flex
			 * (pretbtt drift/VSDB mode)
			 */
			if (param_req_type != MSCH_RT_DUR_FLEX) {
				/* If the request item's start time is same as
				 * the next pending entity's start time, do not update the slot
				 */
				if (entity->pend_slot.start_time == start_time) {
					break;
				}
				/* Remove the entity from corresponding list */
				if (MSCH_START_FIXED(param_req_type)) {
					msch_list_remove(&entity->start_fixed_link);
				} else {
					/* BF */
					msch_list_remove(&entity->both_flex_list);
				}

				entity->req_hdl->req_param->start_time_l = param->start_time_l;
				entity->req_hdl->req_param->start_time_h = param->start_time_h;
				entity->pend_slot.start_time = start_time;
				entity->pend_slot.end_time = entity->pend_slot.start_time +
				entity->req_hdl->req_param->duration;

				entity->pend_slot.pre_start_time =
					entity->pend_slot.start_time - MSCH_ONCHAN_PREPARE;

				if (MSCH_START_FIXED(param_req_type)) {
					/* add the req_entity back to start fixed list */
					msch_list_sorted_add(&msch_info->msch_start_fixed_list,
						&entity->start_fixed_link,
						OFFSETOF(msch_req_entity_t, pend_slot) -
						OFFSETOF(msch_req_entity_t, start_fixed_link) +
						OFFSETOF(msch_req_timing_t, start_time),
						sizeof(uint64), MSCH_ASCEND);
				} else {
					/* add the req_entity back to both flex list */
					msch_list_sorted_add(
						&msch_info->msch_both_flex_req_entity_list,
						&entity->both_flex_list,
						OFFSETOF(msch_req_entity_t, pend_slot) -
						OFFSETOF(msch_req_entity_t, both_flex_list) +
						OFFSETOF(msch_req_timing_t, start_time),
						sizeof(uint64), MSCH_ASCEND);
				}

				/* Destroy and re-schedule the next time slot either if the
				 * next_timeslot req is updated or the updated request end
				 * time is before the next timeslot start time. In other words,
				 * if the updated time is after next time slot, no need to change
				 * anything
				 */

				if (msch_info->next_timeslot && ((entity->pend_slot.timeslot ==
					msch_info->next_timeslot) || (entity->pend_slot.end_time <
					msch_info->next_timeslot->pre_start_time))) {
					_msch_timeslot_destroy(msch_info, msch_info->next_timeslot,
						FALSE);
					msch_info->next_timeslot = NULL;

					if (!(msch_info->flags & MSCH_STATE_IN_TIEMR_CTXT)) {
						_msch_schd(msch_info);
					}
				}
			}
		}
	} while (0);

	return BCME_OK;
}

bool
wlc_msch_query_ts_shared(wlc_msch_info_t *msch_info, uint32 timeslot_id)
{
	msch_req_entity_t *entity = NULL;
	msch_list_elem_t *elem = NULL;
	msch_timeslot_t *timeslot = msch_info->cur_msch_timeslot;
	int req_count = 0;

	MSCH_MESSAGE((msch_info, MSCH_TRACE_ON, "%s: Entry\n", __FUNCTION__));
	ASSERT(msch_info);
	ASSERT(MSCH_CHECK_MAGIC(msch_info));

	if (timeslot && timeslot->timeslot_id == timeslot_id) {
		elem = &msch_info->cur_msch_timeslot->chan_ctxt->req_entity_list;
		while ((elem = msch_list_iterate(elem))) {
			entity = LIST_ENTRY(elem, msch_req_entity_t, chan_ctxt_link);
			if (entity->cur_slot.timeslot == timeslot) {
				req_count++;
				if (req_count > 1) {
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

uint64
msch_current_time(wlc_msch_info_t *msch_info)
{
	uint32 usec_time = wlc_read_usec_timer(msch_info->wlc);
	if (usec_time < msch_info->time_l) {
		msch_info->time_h++;
	}
	msch_info->time_l = usec_time;
	return (((uint64)msch_info->time_h << 32) | msch_info->time_l);
}

uint64
msch_future_time(wlc_msch_info_t *msch_info, uint32 delta_in_usec)
{
	return (msch_current_time(msch_info) + delta_in_usec);
}

/*
 * Queries the slot availability from start_time for duration
 * start_time 0 - returns the end time of current timeslot
 *
 * TBD : Only start time 0 is implemented
 *		 time based query not implemented
 */
uint64
wlc_msch_query_timeslot(wlc_msch_info_t *msch_info, uint64 start_time, uint32 duration)
{
	msch_timeslot_t *cur_ts = msch_info->cur_msch_timeslot;
	uint64 next_start_time = 0;
	if (!start_time && cur_ts) {
		if (cur_ts->end_time != -1) {
			next_start_time = msch_info->cur_msch_timeslot->end_time;
		} else {
			/* Get cur_ts slot end time */
			next_start_time = _msch_curts_slot_endtime(msch_info);
		}

		if (next_start_time < msch_current_time(msch_info)) {
			next_start_time = msch_current_time(msch_info);
		}

		if (!msch_list_empty(&cur_ts->chan_ctxt->bf_entity_list)) {
			next_start_time += MSCH_MAX_OFFCB_DUR;
		}

		/* Add extra 5ms delay for clbk processing */
		next_start_time += (MSCH_ONCHAN_PREPARE + MSCH_EXTRA_DELAY_FOR_CLBK);
		return next_start_time;
	}

	/* TODO not yet implemented */
	return (msch_current_time(msch_info) + MSCH_ONCHAN_PREPARE +
		MSCH_EXTRA_DELAY_FOR_CLBK);
}

static uint64
_msch_tsf2pmu_diff(wlc_msch_info_t *msch_info)
{
	wlc_info_t *wlc = msch_info->wlc;
	uint32 tsf_l, tsf_h;
	uint64 cur_time, tsfdiff;
	bool prev_awake;

	wlc_force_ht(wlc, TRUE, &prev_awake);
	wlc_read_tsf(wlc, &tsf_l, &tsf_h);
	wlc_force_ht(wlc, prev_awake, NULL);
	cur_time = msch_current_time(msch_info);
	tsfdiff = cur_time - (((uint64)tsf_h << 32) | tsf_l);

	return tsfdiff;
}

uint64
msch_tsf2pmu(wlc_msch_info_t *msch_info, uint32 tsf_lo, uint32 tsf_hi,
	uint32 *pmu_lo, uint32 *pmu_hi)
{
	uint64 pmu_time = _msch_tsf2pmu_diff(msch_info) + (((uint64)tsf_hi << 32) | tsf_lo);

	if (pmu_hi)
		*pmu_hi = (uint32)(pmu_time >> 32);
	if (pmu_lo)
		*pmu_lo = (uint32)(pmu_time);

	return pmu_time;
}

void
msch_pmu2tsf(wlc_msch_info_t *msch_info, uint64 pmu_time, uint32 *tsf_lo, uint32 *tsf_hi)
{
	pmu_time -= _msch_tsf2pmu_diff(msch_info);

	if (tsf_hi)
		*tsf_hi = (uint32)(pmu_time >> 32);
	if (tsf_lo)
		*tsf_lo = (uint32)pmu_time;
}
