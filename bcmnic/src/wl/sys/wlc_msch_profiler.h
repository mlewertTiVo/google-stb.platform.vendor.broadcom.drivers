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
 * $Id: wlc_msch_profiler.h 632579 2016-04-19 21:28:59Z $
 */

#ifndef _wlc_msch_profiler_h_
#define _wlc_msch_profiler_h_

#ifdef MSCH_PROFILER
#include <wlc_msch.h>

#define MSCH_ERROR_ON		0x01
#define MSCH_DEBUG_ON		0x02
#define MSCH_INFORM_ON		0x04
#define MSCH_TRACE_ON		0x08

#define MSCH_PROFILE_HEAD_SIZE	4

#define MAX_PROFILE_BUFFER_SIZE		(60 * 1024)
#define MIN_PROFILE_BUFFER_SIZE		(20 * 1024)
#define DEFAULT_PROFILE_BUFFER_SIZE	(30 * 1024)

typedef struct wlc_msch_profiler_info wlc_msch_profiler_info_t;

#ifdef MSCH_TESTING
#define MAX_REGISTER_MSCH	16
typedef struct wlc_msch_test {
	wlc_msch_info_t *p_info;
	wlc_msch_req_handle_t *p_req_hdl;
} wlc_msch_test_t;
#endif

typedef struct wlc_msch_profiler {
	uint32	magic1;
	uint32	magic2;
	int	start_ptr;
	int	write_ptr;
	int	write_size;
	int	read_ptr;
	int	read_size;
	int	total_size;
	uint8  *buffer;
} wlc_msch_profiler_t;

typedef struct wlc_msch_profiler_cmn {
	wlc_msch_profiler_t *profiler;
	uint8               *edata;
	uint8  ver;
	uint8  dump_enable;
	uint8  dump_profiler;
	uint8  dump_callback;
	uint8  dump_register;
	uint8  dump_print_messages;
	uint8  dump_event_messages;
	uint8  dump_status;
#ifdef MSCH_TESTING
	wlc_msch_test_t test[MAX_REGISTER_MSCH];
#endif /* MSCH_TESTING */
	char   mschbufp[WLC_PROFILER_BUFFER_SIZE];
	bool lastMessages;
} wlc_msch_profiler_cmn_t;

struct wlc_msch_profiler_info {
	wlc_msch_profiler_cmn_t *cmn;
	wlc_info_t *wlc;
	wlc_msch_info_t *msch_info;
	uint16 msch_chanspec_alloc_cnt;
	uint16 msch_req_entity_alloc_cnt;
	uint16 msch_req_hdl_alloc_cnt;
	uint16 msch_chan_ctxt_alloc_cnt;
	uint16 msch_timeslot_alloc_cnt;
	uint64 solt_start_time, req_start_time, profiler_start_time;
	uint32 solt_chanspec, req_start;
};

#define MSCH_MAGIC_1		0x4d534348
#define MSCH_MAGIC_2		0x61676963

#define MSCH_EVENT_BIT		0x01
#define MSCH_PROFILE_BIT	0x02

#define PROFILE_CNT_INCR(msch_info, type)	msch_info->profiler->type ++
#define PROFILE_CNT_DECR(msch_info, type)	msch_info->profiler->type --

extern char *_msch_display_time(uint64 cur_time);
extern void msch_message_printf(wlc_msch_info_t *msch_info, int filter,
	const char *fmt, ...);
extern wlc_msch_profiler_info_t *wlc_msch_profiler_attach(wlc_msch_info_t *msch_info);
extern void wlc_msch_profiler_detach(wlc_msch_profiler_info_t *profiler_info);
extern void _msch_dump_profiler(wlc_msch_profiler_info_t *profiler_info);
extern void _msch_dump_register(wlc_msch_profiler_info_t *profiler_info,
	chanspec_t* chanspec_list, int chanspec_cnt, wlc_msch_req_param_t *req_param);
extern void _msch_dump_callback(wlc_msch_profiler_info_t *profiler_info,
	wlc_msch_cb_info_t *cb_info);

#ifdef MSCH_TESTING
extern void _msch_timeslot_unregister(wlc_msch_profiler_info_t *profiler_info,
	wlc_msch_req_handle_t *req_hdl);
#endif /* MSCH_TESTING */
#endif /* MSCH_PROFILER */

#endif /* _wlc_msch_profiler_h_ */
