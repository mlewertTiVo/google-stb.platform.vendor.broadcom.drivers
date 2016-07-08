/*
 * wlc_modesw.c -- .
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_modesw.c 613250 2016-01-18 09:56:05Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <bcmwifi_channels.h>
#include <wlc_modesw.h>
#include <wlc_pcb.h>
#include <wlc_scb.h>
#include <wl_export.h>
#include <wlc_bmac.h>
#include <bcmendian.h>
#include <wlc_stf.h>
#include <wlc_ap.h>
#include <wlc_scan.h>
#include <wlc_tx.h>
#ifdef WLRSDB
#include <wlc_rsdb.h>
#endif
#include <wlc_assoc.h>
#ifdef WLMCNX
#include <wlc_mcnx.h>
#endif
#ifdef WLMCHAN
#include <wlc_mchan.h>
#endif
#include <wlc_ulb.h>
#ifdef WL_NAN
#include <wlc_nan.h>
#endif
#include <wlc_scb_ratesel.h>
#include <wlc_pm.h>

enum msw_oper_mode_states {
	MSW_NOT_PENDING = 0,
	MSW_UPGRADE_PENDING = 1,
	MSW_DOWNGRADE_PENDING = 2,
	MSW_UP_PM1_WAIT_ACK = 3,
	MSW_UP_PM0_WAIT_ACK = 4,
	MSW_DN_PM1_WAIT_ACK = 5,
	MSW_DN_PM0_WAIT_ACK = 6,
	MSW_UP_AF_WAIT_ACK = 7,
	MSW_DN_AF_WAIT_ACK = 8,
	MSW_DN_DEFERRED = 9,
	MSW_UP_DEFERRED = 10,
	MSW_DN_PERFORM = 11,
	MSW_UP_PERFORM = 12,
	MSW_WAIT_DRAIN = 13,
	MSW_AP_DN_WAIT_DTIM = 14,
	MSW_FAILED = 15
};

/* iovar table */
enum {
	IOV_HT_BWSWITCH = 1,
	IOV_DUMP_DYN_BWSW = 2,
	IOV_MODESW_TIME_CALC = 3,
	IOV_MODESW_LAST
};

static const struct {
	uint state;
	char name[40];
} opmode_st_names[] = {
	{MSW_NOT_PENDING, "MSW_NOT_PENDING"},
	{MSW_UPGRADE_PENDING, "MSW_UPGRADE_PENDING"},
	{MSW_DOWNGRADE_PENDING, "MSW_DOWNGRADE_PENDING"},
	{MSW_UP_PM1_WAIT_ACK, "MSW_UP_PM1_WAIT_ACK"},
	{MSW_UP_PM0_WAIT_ACK, "MSW_UP_PM0_WAIT_ACK"},
	{MSW_DN_PM1_WAIT_ACK, "MSW_DN_PM1_WAIT_ACK"},
	{MSW_DN_PM0_WAIT_ACK, "MSW_DN_PM0_WAIT_ACK"},
	{MSW_UP_AF_WAIT_ACK, "MSW_UP_AF_WAIT_ACK"},
	{MSW_DN_AF_WAIT_ACK, "MSW_DN_AF_WAIT_ACK"},
	{MSW_DN_DEFERRED, "MSW_DN_DEFERRED"},
	{MSW_UP_DEFERRED, "MSW_UP_DEFERRED"},
	{MSW_DN_PERFORM, "MSW_DN_PERFORM"},
	{MSW_UP_PERFORM, "MSW_UP_PERFORM"},
	{MSW_WAIT_DRAIN, "MSW_WAIT_DRAIN"},
	{MSW_AP_DN_WAIT_DTIM, "MSW_AP_DN_WAIT_DTIM"},
	{MSW_FAILED, "MSW_FAILED"}
};

/* Macro Definitions */
#define MODESW_AP_DOWNGRADE_BACKOFF		5

/* Action Frame retry limit init */
#define ACTION_FRAME_RETRY_LIMIT 1

/* IOVAR Specific defines */
#define BWSWITCH_20MHZ	20
#define BWSWITCH_40MHZ	40

#define CTRL_FLAGS_HAS(flags, bit)	((uint32)(flags) & (bit))

/* Extend VHT_ENAB to per-BSS */
#define MODESW_BSS_VHT_ENAB(wlc, cfg) \
	(VHT_ENAB_BAND((wlc)->pub, \
	(wlc)->bandstate[CHSPEC_WLCBANDUNIT((cfg)->current_bss->chanspec)]->bandtype) && \
	!((cfg)->flags & WLC_BSSCFG_VHT_DISABLE))

/* Time Measurement enums */
enum {
	MODESW_TM_START = 0,
	MODESW_TM_PM1 = 1,
	MODESW_TM_PHY_COMPLETE = 2,
	MODESW_TM_PM0 = 3,
	MODESW_TM_ACTIONFRAME_COMPLETE = 4
};

#ifdef WL_MODESW_TIMECAL

#define TIMECAL_LOOP(b, str, val) \
	bcm_bprintf(&b, str); \
	for (val = 0; val < TIME_CALC_LIMT; val++)

/* Time Measurement Macro */
#define TIME_CALC_LIMT 8

/* Time Measurement Global Counters */
uint32 SequenceNum;

/* Increment Counter upto TIME_CALC_LIMT and round around */
#define INCR_CHECK(n, max) \
	if (++n > max) n = 1;

/* Update the Sequeue count in Time Measure Structure and update Sequeue number */
#define SEQ_INCR(n) \
		{n = SequenceNum; SequenceNum++;}
#endif /* WL_MODESW_TIMECAL */

/* Threshold for recovery of mode switch (ms) */
#define MODESW_RECOVERY_THRESHOLD 1500


static int
wlc_modesw_doiovar(void *handle, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint plen, void *arg, int alen, int vsize, struct wlc_if *wlcif);

static const bcm_iovar_t modesw_iovars[] = {
	{"dummy", IOV_MODESW_LAST,
	(0), IOVT_BOOL, 0
	},
	{"ht_bwswitch", IOV_HT_BWSWITCH, 0, IOVT_UINT32, 0},
#ifdef WL_MODESW_TIMECAL
	{"modesw_timecal", IOV_MODESW_TIME_CALC,
	(0), IOVT_BUFFER, 0
	},
#endif
	{NULL, 0, 0, 0, 0}
};

typedef struct wlc_modesw_ctx {
	wlc_modesw_info_t *modesw_info;
	uint16		connID;
	void *pkt;
	wlc_bsscfg_t *cfg;
	int status;
	uint8 oper_mode;
	int signal;
	bool notif;
} wlc_modesw_cb_ctx_t;

/* Private cubby struct for ModeSW */
typedef struct {
	uint8 oper_mode_new;
	uint8 oper_mode_old;
	uint8 state;
	uint16 action_sendout_counter; /* count of sent out action frames */
	uint16 max_opermode_chanspec;  /* Oper mode channel changespec */
	chanspec_t new_chanspec;
	struct wl_timer *oper_mode_timer;
	wlc_modesw_cb_ctx_t *timer_ctx;
	uint32 ctrl_flags;	/* control some overrides, eg: used for pseudo operation */
	uint8 orig_pmstate;
	uint32 start_time;
} modesw_bsscfg_cubby_t;

/* SCB Cubby for modesw */
typedef struct {
	uint16 modesw_retry_counter;
	wlc_modesw_cb_ctx_t *cb_ctx;
} modesw_scb_cubby_t;

/* basic modesw are added in wlc structure having cubby handle */
struct wlc_modesw_info {
	wlc_info_t *wlc;
	int cfgh;   /* bsscfg cubby handle */
	int scbh;		/* scb cubby handle */
	bcm_notif_h modesw_notif_hdl;
#ifdef WL_MODESW_TIMECAL
	bool modesw_timecal_enable;
	uint32 ActDNCnt;
	uint32 PHY_UPCnt;
	uint32 PHY_DNCnt;
	uint32 ActFRM_Cnt;
	modesw_time_calc_t modesw_time[TIME_CALC_LIMT];
#endif /* This should always be at the end of this structure */
	uint8 type_of_switch; /* type of mode switch currently active. R->V or V->R */
};

/* bsscfg specific info access accessor */
#define MODESW_BSSCFG_CUBBY_LOC(modesw, cfg) \
	((modesw_bsscfg_cubby_t **)BSSCFG_CUBBY((cfg), (modesw)->cfgh))
#define MODESW_BSSCFG_CUBBY(modesw, cfg) (*(MODESW_BSSCFG_CUBBY_LOC(modesw, cfg)))

#define MODESW_SCB_CUBBY_LOC(modesw, scb) ((modesw_scb_cubby_t **)SCB_CUBBY((scb), (modesw)->scbh))
#define MODESW_SCB_CUBBY(modesw, scb) (*(MODESW_SCB_CUBBY_LOC(modesw, scb)))

/* Initiate scb cubby ModeSW context */
static int wlc_modesw_scb_init(void *ctx, struct scb *scb);

/* Remove scb cubby ModeSW context */
static void wlc_modesw_scb_deinit(void *ctx, struct scb *scb);

/* Initiate ModeSW context */
static int wlc_modesw_bss_init(void *ctx, wlc_bsscfg_t *cfg);

/* Remove ModeSW context */
static void wlc_modesw_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);

static void wlc_modesw_perform_upgrade_downgrade(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *bsscfg);

static void wlc_modesw_oper_mode_timer(void *arg);


static int wlc_modesw_change_sta_oper_mode(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg,
	uint8 oper_mode, uint8 enabled);

static chanspec_t wlc_modesw_find_downgrade_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	uint8 oper_mode_new, uint8 oper_mode_old);

static chanspec_t wlc_modesw_find_upgrade_chanspec(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *cfg, uint8 oper_mode_new, uint8 oper_mode_old);

static bool wlc_modesw_change_ap_oper_mode(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg,
	uint8 oper_mode, uint8 enabled);

static void wlc_modesw_ap_upd_bcn_act(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *bsscfg, uint8 state);

static int wlc_modesw_send_action_frame_request(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *bsscfg, const struct ether_addr *ea, uint8 oper_mode_new);

static void wlc_modesw_ap_send_action_frames(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg);

static bool wlc_modesw_process_action_frame_status(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct scb *scb, bool *cntxt_clear, uint txstatus);

static void wlc_modesw_update_PMstate(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

static void wlc_modesw_assoc_cxt_cb(void *ctx, bss_assoc_state_data_t *notif_data);

static void wlc_modesw_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data);
static void wlc_modesw_process_recovery(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);
static void wlc_modesw_watchdog(void *context);

static void wlc_modesw_updown_cb(void *ctx, bsscfg_up_down_event_data_t *updown_data);

static void wlc_modesw_dealloc_context(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);

static int wlc_modesw_opmode_pending(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);
static void wlc_modesw_clear_phy_chanctx(void *ctx, wlc_bsscfg_t *cfg);

void wlc_modesw_time_measure(wlc_modesw_info_t *modesw_info, uint32 ctrl_flags, uint32 event);

static int wlc_modesw_oper_mode_complete(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t* cfg, int status, int signal);

static int wlc_modesw_alloc_context(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);
static void wlc_modesw_set_pmstate(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t* bsscfg,
	bool state);

static void wlc_modesw_ctrl_hdl_all_cfgs(wlc_modesw_info_t *modesw_info,
		wlc_bsscfg_t *cfg, enum notif_events signal);

/* ModeSW attach Function to Register Module and reserve cubby Structure */
wlc_modesw_info_t *
BCMATTACHFN(wlc_modesw_attach)(wlc_info_t *wlc)
{
	wlc_modesw_info_t *modesw_info;
	bcm_notif_module_t *notif;

	if (!(modesw_info = (wlc_modesw_info_t *)MALLOCZ(wlc->osh, sizeof(wlc_modesw_info_t)))) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}

	modesw_info->wlc = wlc;

	/* reserve cubby in the bsscfg container for per-bsscfg private data */
	if ((modesw_info->cfgh = wlc_bsscfg_cubby_reserve(wlc, sizeof(modesw_bsscfg_cubby_t *),
	                wlc_modesw_bss_init, wlc_modesw_bss_deinit, NULL,
	                (void *)modesw_info)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto err;
	}

	/* reserve cubby in the scb container for per-scb private data */
	if ((modesw_info->scbh = wlc_scb_cubby_reserve(wlc, sizeof(modesw_scb_cubby_t *),
		wlc_modesw_scb_init, wlc_modesw_scb_deinit, NULL, (void *)modesw_info)) < 0) {
			WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve() failed\n",
				wlc->pub->unit, __FUNCTION__));
			goto err;
	}

	/* register module */
	if (wlc_module_register(wlc->pub, modesw_iovars, "modesw", modesw_info, wlc_modesw_doiovar,
		wlc_modesw_watchdog, NULL, NULL)) {
		WL_ERROR(("wl%d: auth wlc_module_register() failed\n", wlc->pub->unit));
		goto err;
	}

	/* create notification list update. */
	notif = wlc->notif;
	ASSERT(notif != NULL);
	if (bcm_notif_create_list(notif, &modesw_info->modesw_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: modesw_info bcm_notif_create_list() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto err;
	}

	if (wlc_bss_assoc_state_register(wlc, wlc_modesw_assoc_cxt_cb, modesw_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bss_assoc_state_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto err;
	}

	if (wlc_bsscfg_updown_register(wlc, wlc_modesw_updown_cb, modesw_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_updown_register() failed\n",
			wlc->pub->unit, __FUNCTION__));

		goto err;
	}

	/* Add client callback to the scb state notification list */
	if ((wlc_scb_state_upd_register(wlc, wlc_modesw_scb_state_upd_cb,
		modesw_info)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback %p\n",
			wlc->pub->unit, __FUNCTION__, wlc_modesw_scb_state_upd_cb));
		goto err;
	}

	wlc->pub->_modesw = TRUE;

	return modesw_info;
err:
	wlc_modesw_detach(wlc->modesw);
	return NULL;
}

/* Detach modesw Module from system */
void
BCMATTACHFN(wlc_modesw_detach)(wlc_modesw_info_t *modesw_info)
{
	wlc_info_t *wlc = NULL;
	if (!modesw_info)
		return;

	wlc = modesw_info->wlc;
	WL_TRACE(("wl%d: wlc_modesw_detach\n", wlc->pub->unit));
	wlc_module_unregister(wlc->pub, "modesw", modesw_info);

	if (modesw_info->modesw_notif_hdl != NULL) {
		WL_MODE_SWITCH(("REMOVING NOTIFICATION LIST\n"));
		bcm_notif_delete_list(&modesw_info->modesw_notif_hdl);
	}

	if (wlc_bss_assoc_state_unregister(wlc, wlc_modesw_assoc_cxt_cb, wlc->modesw) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bss_assoc_state_unregister() failed\n",
			wlc->pub->unit, __FUNCTION__));
	}

	/* Remove client callback for the scb state notification list */
	wlc_scb_state_upd_unregister(wlc, wlc_modesw_scb_state_upd_cb,
	modesw_info);

	if (wlc_bsscfg_updown_unregister(wlc, wlc_modesw_updown_cb, modesw_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_updown_unregister() failed\n",
			wlc->pub->unit, __FUNCTION__));
	}
	MFREE(wlc->osh, modesw_info, sizeof(wlc_modesw_info_t));
}

/* function return the type of switch currently active */
INLINE
modesw_type wlc_modesw_get_switch_type(wlc_modesw_info_t *modesw)
{
	modesw_type ret = MODESW_NO_SW_IN_PROGRESS;
	if (modesw) {
		ret = modesw->type_of_switch;
	}
	return ret;
}

/* function sets the type of switch */
INLINE
void wlc_modesw_set_switch_type(wlc_modesw_info_t *modesw, modesw_type type)
{
	if (modesw && (type < MODESW_LIST_END)) {
		modesw->type_of_switch = type;
	}
}

#ifdef WL_MODESW_TIMECAL
static int
wlc_modesw_time_stats(wlc_modesw_info_t *modesw_info, void *input, int buf_len,
void *output)
{
	char *buf_ptr;
	int val;
	struct bcmstrbuf b;
	modesw_time_calc_t *pmodesw_time = modesw_info->modesw_time;
	buf_ptr = (char *)output;
	bcm_binit(&b, buf_ptr, buf_len);

	if (modesw_info->modesw_timecal_enable == 1)
	{
		bcm_bprintf(&b, "\n\t\t\t\t Statistics (Time in microseconds)\n");

		TIMECAL_LOOP(b, "\n Index \t\t\t", val)
			bcm_bprintf(&b, "\t %d", val);

		/* Actual Downgrade */
		TIMECAL_LOOP(b, "\n DN Seq No :\t\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].ActDN_SeqCnt);

		TIMECAL_LOOP(b, "\n DN Action + ACK Time:\t", val)
			bcm_bprintf(&b, "\t %d",
				pmodesw_time[val].DN_ActionFrame_time -
				pmodesw_time[val].DN_start_time);

		TIMECAL_LOOP(b, "\n DN PHY Down Time\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].DN_PHY_BW_UPDTime -
				pmodesw_time[val].DN_ActionFrame_time);

		TIMECAL_LOOP(b, "\n DN PM Transition Time:\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].DN_CompTime -
				pmodesw_time[val].DN_PHY_BW_UPDTime);

		TIMECAL_LOOP(b, "\n DN Total Time:\t\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].DN_CompTime -
				pmodesw_time[val].DN_start_time);

		/* PHY  Upgrade */
		TIMECAL_LOOP(b, "\n Pseudo UP Seq No:\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_UP_SeqCnt);

		TIMECAL_LOOP(b, "\n Pseudo UP Process Time:", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_UP_PM1_time -
				pmodesw_time[val].PHY_UP_start_time);

		TIMECAL_LOOP(b, "\n Pseudo UP PHY Time:\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_UP_BW_UPDTime
				- pmodesw_time[val].PHY_UP_PM1_time);

		TIMECAL_LOOP(b, "\n Pseudo UP PM TransTime:", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_UP_CompTime
				- pmodesw_time[val].PHY_UP_BW_UPDTime);

		TIMECAL_LOOP(b, "\n Pseudo UP CompleteTime:", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_UP_CompTime
				- pmodesw_time[val].PHY_UP_start_time);

		/* PHY  Downgrade */
		TIMECAL_LOOP(b, "\n Pseudo DN Seq No:\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_DN_SeqCnt);

		TIMECAL_LOOP(b, "\n Pseudo DN Process Time:", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_DN_PM1_time -
				pmodesw_time[val].PHY_DN_start_time);

		TIMECAL_LOOP(b, "\n Pseudo DN PHY Time:\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_DN_BW_UPDTime
				- pmodesw_time[val].PHY_DN_PM1_time);

		TIMECAL_LOOP(b, "\n Pseudo DN PM TransTime:", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_DN_CompTime
				- pmodesw_time[val].PHY_DN_BW_UPDTime);

		TIMECAL_LOOP(b, "\n Pseudo DN CompleteTime:", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].PHY_DN_CompTime
				- pmodesw_time[val].PHY_DN_start_time);
		TIMECAL_LOOP(b, "\n Normal Upgrade Seq No:\t", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].ACTFrame_SeqCnt);

		TIMECAL_LOOP(b, "\n Normal UP CompleteTime:", val)
			bcm_bprintf(&b, "\t %d", pmodesw_time[val].ACTFrame_complete -
				pmodesw_time[val].ACTFrame_start);
		bcm_bprintf(&b, "\n");
	}
	else
	{
		bcm_bprintf(&b, "\n Modesw timecal Disabled \n");
		bcm_bprintf(&b, " No Statistics \n");
	}
	return 0;
}
#endif /* WL_MODESW_TIMECAL */

/* Iovar functionality if any should be added in this function */
static int
wlc_modesw_doiovar(void *handle, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *p, uint plen, void *a, int alen, int vsize, struct wlc_if *wlcif)
{
	int32 *ret_int_ptr;
	wlc_modesw_info_t *modesw = (wlc_modesw_info_t *)handle;
	int32 int_val = 0;
	int err = BCME_OK;
	bool bool_val;
	wlc_info_t *wlc = modesw->wlc;
	wlc_bsscfg_t *bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)a;
	BCM_REFERENCE(ret_int_ptr);
	if (plen >= (int)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	bool_val = (int_val != 0) ? TRUE : FALSE;
	BCM_REFERENCE(bool_val);

	switch (actionid) {
		case IOV_SVAL(IOV_HT_BWSWITCH) :
			if (WLC_MODESW_ENAB(wlc->pub)) {
				uint8 bw;

				if (wlc_modesw_is_req_valid(modesw, bsscfg) != TRUE) {
					err = BCME_BUSY;
					break;
				}
				if (int_val == BWSWITCH_20MHZ)
					bw = DOT11_OPER_MODE_20MHZ;

				else if (int_val == BWSWITCH_40MHZ)
					bw = DOT11_OPER_MODE_40MHZ;

				else
				break;

				err = wlc_modesw_handle_oper_mode_notif_request(modesw,
					bsscfg, bw, TRUE, 0);
			} else
				err = BCME_UNSUPPORTED;
			break;
#ifdef WL_MODESW_TIMECAL
		case IOV_SVAL(IOV_MODESW_TIME_CALC) :
			modesw->modesw_timecal_enable = bool_val;
			break;
		case IOV_GVAL(IOV_MODESW_TIME_CALC) :
		{
			err = wlc_modesw_time_stats(modesw, p, alen, a);
			break;
		}
#endif /* WL_MODESW_TIMECAL */
		default:
			err = BCME_UNSUPPORTED;
	}

	return err;
}

/* Allocate modesw context ,
 * and return the status back
 */
int
wlc_modesw_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_modesw_info_t *modesw_info = (wlc_modesw_info_t *)ctx;
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t **pmodesw_bsscfg = MODESW_BSSCFG_CUBBY_LOC(modesw_info, cfg);
	modesw_bsscfg_cubby_t *modesw = NULL;

	/* allocate memory and point bsscfg cubby to it */
	if ((modesw = MALLOCZ(wlc->osh, sizeof(modesw_bsscfg_cubby_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	*pmodesw_bsscfg = modesw;
	if (wlc_modesw_alloc_context(modesw_info, cfg) != BCME_OK) {
		return BCME_NOMEM;
	}
	return BCME_OK;
}

/* Toss ModeSW context */
static void
wlc_modesw_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_modesw_info_t *modesw_info = (wlc_modesw_info_t *)ctx;
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t **pmodesw_bsscfg = MODESW_BSSCFG_CUBBY_LOC(modesw_info, cfg);
	modesw_bsscfg_cubby_t *modesw, *ipmode_cfg;
	wlc_bsscfg_t* icfg;
	int idx;

	/* free the Cubby reserve allocated memory  */
	modesw = *pmodesw_bsscfg;
	if (modesw) {
		/* timer and Context Free */
		wlc_modesw_dealloc_context(modesw_info, cfg);

		MFREE(wlc->osh, modesw, sizeof(modesw_bsscfg_cubby_t));
		*pmodesw_bsscfg = NULL;
	}

	/* Check if this cfg is the only cfg
	* pending for opmode change ?
	*/
	FOREACH_AS_BSS(wlc, idx, icfg) {
		ipmode_cfg = MODESW_BSSCFG_CUBBY(modesw_info, icfg);
		if (icfg != cfg &&
		    (ipmode_cfg->state != MSW_NOT_PENDING)) {
			WL_MODE_SWITCH(("wl%d: Opmode change pending for cfg = %d..\n",
				WLCWLUNIT(wlc), icfg->_idx));
			return;
		}
	}
	wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);
	return;
}

/* Allocate modesw context , and return the status back
 */
int
wlc_modesw_scb_init(void *ctx, struct scb *scb)
{
	wlc_modesw_info_t *modeswinfo = (wlc_modesw_info_t *)ctx;
	wlc_info_t *wlc = modeswinfo->wlc;
	modesw_scb_cubby_t **pmodesw_scb = MODESW_SCB_CUBBY_LOC(modeswinfo, scb);
	modesw_scb_cubby_t *modesw_scb;

	/* allocate memory and point bsscfg cubby to it */
	if ((modesw_scb = MALLOCZ(wlc->osh, sizeof(modesw_scb_cubby_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}
	*pmodesw_scb = modesw_scb;
	return BCME_OK;
}

static void
wlc_modesw_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_modesw_info_t *modeswinfo = (wlc_modesw_info_t *)ctx;
	wlc_info_t *wlc = modeswinfo->wlc;
	modesw_scb_cubby_t **pmodesw_scb = MODESW_SCB_CUBBY_LOC(modeswinfo, scb);
	modesw_scb_cubby_t *modesw_scb = *pmodesw_scb;

	if (modesw_scb != NULL) {
		if (modesw_scb->cb_ctx != NULL) {
			MFREE(wlc->osh, modesw_scb->cb_ctx,
			sizeof(wlc_modesw_cb_ctx_t));
			modesw_scb->cb_ctx = NULL;
		}
		MFREE(wlc->osh, modesw_scb, sizeof(modesw_scb_cubby_t));
	}
	*pmodesw_scb = NULL;
}

/* These functions register/unregister a callback that wlc_modesw_notif_cb_notif may invoke. */
int
wlc_modesw_notif_cb_register(wlc_modesw_info_t *modeswinfo, wlc_modesw_notif_cb_fn_t cb,
            void *arg)
{
	bcm_notif_h hdl = modeswinfo->modesw_notif_hdl;
	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)cb, arg);
}

int
wlc_modesw_notif_cb_unregister(wlc_modesw_info_t *modeswinfo, wlc_modesw_notif_cb_fn_t cb,
            void *arg)
{
	bcm_notif_h hdl = modeswinfo->modesw_notif_hdl;
	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)cb, arg);
}

static void
wlc_modesw_notif_cb_notif(wlc_modesw_info_t *modeswinfo, wlc_bsscfg_t *cfg, int status,
	uint8 oper_mode, uint8 oper_mode_old, int signal)
{
	wlc_modesw_notif_cb_data_t notif_data;
	bcm_notif_h hdl = modeswinfo->modesw_notif_hdl;

	notif_data.cfg = cfg;
	notif_data.opmode = oper_mode;
	notif_data.opmode_old = oper_mode_old;
	notif_data.status = status;
	notif_data.signal = signal;
	bcm_notif_signal(hdl, &notif_data);
	return;
}

bool
wlc_modesw_is_connection_vht(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	struct scb *scb = NULL;
	if ((BSSCFG_IBSS(bsscfg)) || (BSSCFG_AP(bsscfg))) {
		/* AP or IBSS case */
		return (MODESW_BSS_VHT_ENAB(wlc, bsscfg)) ? TRUE : FALSE;
	} else {
		/* non ibss case */
		scb = wlc_scbfindband(wlc, bsscfg, &bsscfg->BSSID,
			CHSPEC_WLCBANDUNIT(bsscfg->current_bss->chanspec));
		if (scb && SCB_VHT_CAP(scb))
			return TRUE;
		return FALSE;
	}
}

bool
wlc_modesw_is_connection_ht(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	struct scb *scb = NULL;
	if (BSSCFG_IBSS(bsscfg) || (BSSCFG_AP(bsscfg))) {
		/* AP or IBSS case */
		return (BSS_N_ENAB(wlc, bsscfg)) ? TRUE : FALSE;
	} else {
		/* non ibss case */
		scb = wlc_scbfindband(wlc, bsscfg, &bsscfg->BSSID,
			CHSPEC_WLCBANDUNIT(bsscfg->current_bss->chanspec));
		if (scb && SCB_HT_CAP(scb))
			return TRUE;
		return FALSE;
	}
}

/* Interface Function to set Opermode Channel Spec Variable */
void
wlc_modesw_set_max_chanspec(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg,
	chanspec_t chanspec)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);

	pmodesw_bsscfg->max_opermode_chanspec = chanspec;
}

bool
wlc_modesw_is_req_valid(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	bool valid_req = FALSE;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);
	if (pmodesw_bsscfg->state == MSW_NOT_PENDING)
		valid_req = TRUE;
	/* return  the TRUE/ FALSE */
	else
		valid_req = FALSE;

	return valid_req;
}

/* Handles the TBTT interrupt for AP. Required to process
 * the AP downgrade procedure, which is waiting for DTIM
 * beacon to go out before perform actual downgrade of link
 */
void
wlc_modesw_bss_tbtt(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);
	BCM_REFERENCE(wlc);
	if (!BSSCFG_AP(cfg)) {
		return;
	}
	if (cfg->oper_mode_enabled && pmodesw_bsscfg &&
		(pmodesw_bsscfg->state == MSW_AP_DN_WAIT_DTIM) &&
		!CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags, MODESW_CTRL_DN_SILENT_DNGRADE))
	{
		/* sanity checks */
		if (wlc_modesw_alloc_context(modesw_info, cfg) != BCME_OK) {
			goto fail;
		}

		/* AP downgrade is pending */
		/* If it is the TBTT for DTIM then start the non-periodic timer */
#ifdef AP
		if (!wlc_ap_getdtim_count(wlc, cfg)) {
			ASSERT(pmodesw_bsscfg->oper_mode_timer != NULL);
#ifdef WLMCHAN
			if (MCHAN_ACTIVE(wlc->pub)) {
				wlc_modesw_change_state(modesw_info, cfg, MSW_DN_PERFORM);
			} else
#endif /* WLMCHAN */
			{
			WL_MODE_SWITCH(("TBTT Timer set....\n"));
				WL_MODE_SWITCH(("DTIM Tbtt..Start timer for 5 msec \n"));
				if (pmodesw_bsscfg->oper_mode_timer != NULL) {
					wl_add_timer(wlc->wl, pmodesw_bsscfg->oper_mode_timer,
						MODESW_AP_DOWNGRADE_BACKOFF, FALSE);
				}
			}
		}
#endif /* AP */
	}
	return;
fail:
	/* Downgrade can not be completed. Change the mode switch state
	 * so that a next attempt could be tried by upper layer
	 */
	wlc_modesw_change_state(modesw_info, cfg, MSW_NOT_PENDING);
	return;
}

/* This function changes the state of the cfg to new_state */
void
wlc_modesw_change_state(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg, int8 new_state)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);
	if (!pmodesw_bsscfg)
		return;
	if (pmodesw_bsscfg->state != new_state) {
		WL_MODE_SWITCH(("wl%d: cfg = %d MODESW STATE Transition: %s -> %s\n",
			WLCWLUNIT(modesw_info->wlc), cfg->_idx,
			opmode_st_names[pmodesw_bsscfg->state].name,
			opmode_st_names[new_state].name));
		pmodesw_bsscfg->state = new_state;
		/* Take new timestamp for new state transition. */
		pmodesw_bsscfg->start_time = OSL_SYSUPTIME();
	}
	return;
}

/* This function allocates the opmode context which is retained till
 * the opmode is disabled
 */
static int
wlc_modesw_alloc_context(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);
	if (pmodesw_bsscfg->timer_ctx == NULL) {
		/* Modesw context is not initialized */
		if (!(pmodesw_bsscfg->timer_ctx =
			(wlc_modesw_cb_ctx_t *)MALLOCZ(wlc->osh,
			sizeof(wlc_modesw_cb_ctx_t)))) {
			WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			return BCME_ERROR;
		}
		pmodesw_bsscfg->timer_ctx->connID = cfg->ID;
		pmodesw_bsscfg->timer_ctx->modesw_info = modesw_info;
		pmodesw_bsscfg->timer_ctx->cfg = cfg;

		WL_MODE_SWITCH(("Initializing timer ...alloc mem\n"));
		/* Initialize the timer */
		pmodesw_bsscfg->oper_mode_timer = wl_init_timer(wlc->wl,
			wlc_modesw_oper_mode_timer, pmodesw_bsscfg->timer_ctx,
			"opermode_dgrade_timer");

		if (pmodesw_bsscfg->oper_mode_timer == NULL)
		{
				WL_ERROR(("Timer alloc failed\n"));
				MFREE(wlc->osh, pmodesw_bsscfg->timer_ctx,
					sizeof(wlc_modesw_cb_ctx_t));
				return BCME_ERROR;
			}
		}
	return BCME_OK;
}

/* This function de-allocates the opmode context when the opmode
 * is disabled
 */
static void
wlc_modesw_dealloc_context(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);
	if (pmodesw_bsscfg != NULL) {
		if (pmodesw_bsscfg->oper_mode_timer != NULL) {
			wl_del_timer(wlc->wl, pmodesw_bsscfg->oper_mode_timer);
			wl_free_timer(wlc->wl, pmodesw_bsscfg->oper_mode_timer);
			pmodesw_bsscfg->oper_mode_timer = NULL;
		}
		if (pmodesw_bsscfg->timer_ctx != NULL) {
			MFREE(wlc->osh, pmodesw_bsscfg->timer_ctx,
				sizeof(wlc_modesw_cb_ctx_t));
			pmodesw_bsscfg->timer_ctx = NULL;
		}
	}
}

void
wlc_modesw_perform_upgrade_downgrade(wlc_modesw_info_t *modesw_info,
wlc_bsscfg_t * bsscfg)
{
	wlc_info_t *wlc = modesw_info->wlc;
	uint8 rxnss, old_rxnss, chains = 0, i;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	bool nss_change;
	ASSERT(modesw_info);
	ASSERT(bsscfg);
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	ASSERT(pmodesw_bsscfg);
	ASSERT((pmodesw_bsscfg->state != MSW_NOT_PENDING));
	rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_new);
	old_rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_old);
	nss_change = (rxnss != old_rxnss) ? TRUE:FALSE;
	WL_MODE_SWITCH(("wl%d: cfg-%d: new_chspec %x old_chspec %x new_rxnss %x old_rxnss %x\n",
		WLCWLUNIT(wlc), bsscfg->_idx, pmodesw_bsscfg->new_chanspec,
		bsscfg->current_bss->chanspec, rxnss, wlc->stf->op_rxstreams));

	/* Invalidate the ctx before cals are triggered */
	wlc_phy_invalidate_chanctx(WLC_PI(wlc), bsscfg->current_bss->chanspec);

	if (pmodesw_bsscfg->state == MSW_UP_PERFORM) {
		wlc_modesw_notif_cb_notif(modesw_info, bsscfg, BCME_OK,
				pmodesw_bsscfg->oper_mode_new, pmodesw_bsscfg->oper_mode_old,
				MODESW_PHY_UP_START);
	}

	/* Perform the NSS change if there is any change required in no of Rx streams */
	/* Perform the NSS change if there is any change required in no of Rx streams */
	if (nss_change) {
		/* Construct the Rx/Tx chains based on Rx streams */
		for (i = 0; i < rxnss; i++) {
			chains |= (1 << i);
		}
		/* Perform the change of Tx chain first, as it waits for
		 * all the Tx packets to flush out. Change of Rx chain
		 * involves sending out the MIMOOPS action frame, which
		 * can cause postponement of the Tx chain changes
		 * due to pending action frame in Tx FIFO.
		 */
		wlc_stf_txchain_set(wlc, chains, TRUE, WLC_TXCHAIN_ID_USR);
		wlc_stf_rxchain_set(wlc, chains, FALSE);
	} else {
		/* Perform the bandwidth switch if the current chanspec
		 * is not matching with requested chanspec in the operating
		 * mode notification
		 */
		bsscfg->flags2 |= WLC_BSSCFG_FL2_MODESW_BWSW;
		wlc_update_bandwidth(wlc, bsscfg, pmodesw_bsscfg->new_chanspec);
		bsscfg->flags2 &= ~WLC_BSSCFG_FL2_MODESW_BWSW;
	}
	wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
		MODESW_TM_PHY_COMPLETE);

	if (pmodesw_bsscfg->state == MSW_DN_PERFORM) {
		wlc_modesw_notif_cb_notif(modesw_info, bsscfg, BCME_OK,
				pmodesw_bsscfg->oper_mode_new, pmodesw_bsscfg->oper_mode_old,
				MODESW_PHY_DN_COMPLETE);
	}

	if (!CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
		MODESW_CTRL_HANDLE_ALL_CFGS)) {
		if (pmodesw_bsscfg->state == MSW_UP_PERFORM) {
			wlc_modesw_notif_cb_notif(modesw_info, bsscfg, BCME_OK,
				pmodesw_bsscfg->oper_mode_new, pmodesw_bsscfg->oper_mode_old,
				MODESW_PHY_UP_COMPLETE);
		}
	}
	WL_MODE_SWITCH(("afterUpdBW:nc:%x,wlccs:%x,bsscs:%x\n",
		pmodesw_bsscfg->new_chanspec, wlc->chanspec,
		bsscfg->current_bss->chanspec));
	wlc_scb_ratesel_init_bss(wlc, bsscfg);
	wlc_phy_cal_perical(WLC_PI(wlc), PHY_PERICAL_PHYMODE_SWITCH);
}

/* Updates beacon and probe response with New bandwidth
* Sents action frames only during normal Upgrade
*/
void
wlc_modesw_ap_upd_bcn_act(wlc_modesw_info_t *modesw_info,
wlc_bsscfg_t *bsscfg, uint8 state)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	ASSERT(BSSCFG_AP(bsscfg) || BSSCFG_IBSS(bsscfg));

	if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags, MODESW_CTRL_AP_ACT_FRAMES)) {
		wlc_modesw_ap_send_action_frames(wlc->modesw, bsscfg);
	} else if (pmodesw_bsscfg->state == MSW_DOWNGRADE_PENDING) {
		wlc_modesw_change_state(modesw_info, bsscfg, MSW_AP_DN_WAIT_DTIM);
	}

	wlc_bss_update_beacon(wlc, bsscfg);
	wlc_bss_update_probe_resp(wlc, bsscfg, TRUE);
}

/* This function resumes the STA/AP downgrade for each bsscfg.
 * This is required if the downgrade was postponed due to pending
 * packets in the Tx FIFO
 */
void
wlc_modesw_bsscfg_complete_downgrade(wlc_modesw_info_t *modesw_info)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg;
	wlc_bsscfg_t *icfg;
	uint8 i = 0;
	FOREACH_AS_BSS(wlc, i, icfg) {
		pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, icfg);
		if (pmodesw_bsscfg == NULL)
			return;
		if (pmodesw_bsscfg->state != MSW_NOT_PENDING) {
			if ((BSSCFG_STA(icfg)) &&
					(pmodesw_bsscfg->state == MSW_WAIT_DRAIN)) {
				wlc_modesw_change_state(modesw_info, icfg, MSW_DN_PERFORM);
				wlc_modesw_resume_opmode_change(modesw_info, icfg);
			}
			else if (BSSCFG_AP(icfg) &&
					(pmodesw_bsscfg->state == MSW_WAIT_DRAIN)) {
				uint8 new_rxnss, old_rxnss;
				new_rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_new);
				old_rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_old);
				if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
					MODESW_CTRL_AP_ACT_FRAMES) && (new_rxnss == old_rxnss)) {
					/* For prot obss cases, we transit from wait_drain
					* to downgrade_pending after drain
					*/
					wlc_modesw_change_state(modesw_info, icfg,
						MSW_DOWNGRADE_PENDING);
					wlc_modesw_change_ap_oper_mode(modesw_info, icfg,
						pmodesw_bsscfg->oper_mode_new, TRUE);
				} else {
					wlc_modesw_change_state(modesw_info, icfg,
						MSW_DN_PERFORM);
					/* Call actual HW change */
					wlc_modesw_perform_upgrade_downgrade(modesw_info, icfg);
					icfg->oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
						icfg->current_bss->chanspec, icfg,
						wlc->stf->rxstreams);
					WLC_BLOCK_DATAFIFO_CLEAR(wlc, DATA_BLOCK_TXCHAIN);
					wlc_modesw_oper_mode_complete(wlc->modesw, icfg,
						BCME_OK, MODESW_DN_AP_COMPLETE);
				}
			}
		}
	}
}

static int
wlc_modesw_oper_mode_complete(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t* cfg, int status,
	int signal)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg, *ipmode_cfg;
	wlc_bsscfg_t* icfg;
	int idx;

	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);

	/* Stop the expiry timer */
	WL_MODE_SWITCH(("wl%d: Called opmode complete for cfg = %d status = %d\n",
		WLCWLUNIT(wlc), cfg->_idx, status));

	if (pmodesw_bsscfg->oper_mode_timer != NULL) {
		wl_del_timer(wlc->wl, pmodesw_bsscfg->oper_mode_timer);
	}

	if (cfg->associated || cfg->up) {
		cfg->oper_mode = wlc_modesw_derive_opermode(modesw_info,
			cfg->current_bss->chanspec, cfg, wlc->stf->rxstreams);
	}

	if (status) {
		WL_MODE_SWITCH(("wl%d:%d Opmode change failed..\n",
			WLCWLUNIT(wlc), cfg->_idx));
	}

	/* Dont bother about other cfg's in MCHAN active mode */
#ifdef WLMCHAN
	if (MCHAN_ACTIVE(wlc->pub)) {
		if (status) {
			wlc_modesw_change_state(modesw_info, cfg, MSW_FAILED);
		}
		else {
			wlc_modesw_change_state(modesw_info, cfg, MSW_NOT_PENDING);
		}
		wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);
		/* Call the registered callbacks */
		wlc_modesw_notif_cb_notif(modesw_info, cfg, status, cfg->oper_mode,
				pmodesw_bsscfg->oper_mode_old, signal);
		return BCME_OK;
	}
#endif /* WLMCHAN */
	WL_MODE_SWITCH(("wl%d: Opmode change completed for cfg %d...Check Others\n",
		WLCWLUNIT(wlc), cfg->_idx));

	FOREACH_AS_BSS(wlc, idx, icfg) {
		ipmode_cfg = MODESW_BSSCFG_CUBBY(modesw_info, icfg);
		if ((ipmode_cfg->state != MSW_NOT_PENDING) &&
			(icfg != cfg)) {
			WL_MODE_SWITCH(("wl%d: Opmode change pending for cfg = %d..\n",
				WLCWLUNIT(wlc), icfg->_idx));
			if (status) {
				wlc_modesw_change_state(modesw_info, cfg, MSW_FAILED);
			}
			else {
				wlc_modesw_change_state(modesw_info, cfg, MSW_NOT_PENDING);
			}
			return BCME_NOTREADY;
		}
	}
	WL_MODE_SWITCH(("wl%d: ALL cfgs are DONE (says cfg %d)\n",
		WLCWLUNIT(wlc), cfg->_idx));

	WL_MODE_SWITCH(("wl%d: Complete opmode change for cfg %d\n",
		WLCWLUNIT(wlc), cfg->_idx));
	wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);
	/* Call the registered callbacks */
	pmodesw_bsscfg->timer_ctx->status = status;
	pmodesw_bsscfg->timer_ctx->oper_mode = cfg->oper_mode;
	pmodesw_bsscfg->timer_ctx->signal = signal;
	pmodesw_bsscfg->timer_ctx->notif = TRUE;
	wl_add_timer(wlc->wl, pmodesw_bsscfg->oper_mode_timer, 0, FALSE);
	return BCME_OK;
}

/* Handle the different states of the operating mode processing. */
void
wlc_modesw_pm_pending_complete(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg, *ipmode_cfg;
	wlc_bsscfg_t *icfg;
	int idx;
	if (!cfg || !modesw_info)
		return;

	wlc =  modesw_info->wlc;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);

	/* Make sure that we have oper mode change pending for this cfg.
	* It may happen that we clear the oper mode context for this cfg
	* due to timeout.
	*/
	if (!pmodesw_bsscfg || (pmodesw_bsscfg->state == MSW_NOT_PENDING) ||
		(pmodesw_bsscfg->state == MSW_FAILED)) {
		return;
	}

	switch (pmodesw_bsscfg->state)
	{
		case MSW_UP_PM1_WAIT_ACK:
			{
				WL_MODE_SWITCH(("wl%d: Got the NULL ack for PM 1 for cfg = %d."
					"Start actualupgrade\n", WLCWLUNIT(wlc), cfg->_idx));
				wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
					MODESW_TM_PM1);
				wlc_modesw_change_state(modesw_info, cfg, MSW_UP_PERFORM);
				wlc_modesw_resume_opmode_change(modesw_info, cfg);
				wlc_modesw_time_measure(modesw_info, pmodesw_bsscfg->ctrl_flags,
					MODESW_TM_PHY_COMPLETE);
			}
			break;
		case MSW_UP_PM0_WAIT_ACK:
			{
				uint8 opermode_bw;
				WL_MODE_SWITCH(("wl%d: Got NULL ack for PM 0 for cfg = %d.."
					"Send action frame\n", WLCWLUNIT(wlc), cfg->_idx));
				opermode_bw = pmodesw_bsscfg->oper_mode_new;

				/* if pseudo state indicates pending, dont send action frame */
				if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
						MODESW_CTRL_UP_SILENT_UPGRADE)) {
					/* donot send action frame., and stop the SM here */
					WL_MODE_SWITCH(("MODESW_CTRL_UP_SILENT_UPGRADE\n"));
					wlc_modesw_change_state(modesw_info, cfg, MSW_NOT_PENDING);
					wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);

					if (wlc_modesw_oper_mode_complete(wlc->modesw, cfg,
							BCME_OK, MODESW_PHY_UP_COMPLETE) ==
							BCME_NOTREADY) {
						WL_MODE_SWITCH(("wl%d: Resume others (cfg %d)\n",
							WLCWLUNIT(wlc), cfg->_idx));
						FOREACH_AS_BSS(wlc, idx, icfg) {
							ipmode_cfg = MODESW_BSSCFG_CUBBY(
								wlc->modesw, icfg);
							if ((icfg != cfg) &&
								(ipmode_cfg->state ==
									MSW_UP_DEFERRED)) {
								ipmode_cfg->state = MSW_UP_PERFORM;
								wlc_modesw_resume_opmode_change(
									wlc->modesw, icfg);
							}
						}
					}
				} else {
					WL_MODE_SWITCH(("UP:send ACTION!\n"));
					wlc_modesw_ctrl_hdl_all_cfgs(modesw_info, cfg,
						MODESW_PHY_UP_COMPLETE);
					wlc_modesw_send_action_frame_request(modesw_info, cfg,
						&cfg->BSSID, opermode_bw);
				}
				wlc_modesw_time_measure(modesw_info, pmodesw_bsscfg->ctrl_flags,
					MODESW_TM_PM0);
			}
			break;
		case MSW_DN_PM1_WAIT_ACK:
			{
				WL_MODE_SWITCH(("wl%d: Got the NULL ack for PM 1 for cfg = %d."
					"Start actual downG\n",
					WLCWLUNIT(wlc), cfg->_idx));
				wlc_modesw_time_measure(modesw_info, pmodesw_bsscfg->ctrl_flags,
					MODESW_TM_PM1);
				/* Dont drain pkts for MCHAN active. TBD for mchan */
				if (TXPKTPENDTOT(wlc) == 0) {
					wlc_modesw_change_state(modesw_info, cfg, MSW_DN_PERFORM);
					wlc_modesw_resume_opmode_change(modesw_info,
						cfg);
				}
				else {
					WLC_BLOCK_DATAFIFO_SET(wlc, DATA_BLOCK_TXCHAIN);
					wlc_modesw_change_state(modesw_info, cfg, MSW_WAIT_DRAIN);
				}
				wlc_modesw_time_measure(modesw_info, pmodesw_bsscfg->ctrl_flags,
					MODESW_TM_PHY_COMPLETE);
			}
			break;
		case MSW_DN_PM0_WAIT_ACK:
			{
				WL_MODE_SWITCH(("wl%d: Got NULL ack for PM 0 for cfg = %d..\n",
					WLCWLUNIT(wlc), cfg->_idx));
				/* Start the Tx queue */
				WLC_BLOCK_DATAFIFO_CLEAR(wlc, DATA_BLOCK_TXCHAIN);
				if (wlc_modesw_oper_mode_complete(modesw_info, cfg,
						BCME_OK, MODESW_DN_STA_COMPLETE) ==
						BCME_NOTREADY) {
					WL_MODE_SWITCH(("wl%d: Resume others (cfg %d)\n",
						WLCWLUNIT(wlc), cfg->_idx));
					FOREACH_AS_BSS(wlc, idx, icfg) {
						ipmode_cfg = MODESW_BSSCFG_CUBBY(modesw_info, icfg);
						if ((icfg != cfg) &&
								(ipmode_cfg->state ==
								MSW_DN_DEFERRED)) {
							ipmode_cfg->state = MSW_DN_PERFORM;
							wlc_modesw_resume_opmode_change(modesw_info,
								icfg);
						}
					}
				}
				wlc_modesw_time_measure(modesw_info, pmodesw_bsscfg->ctrl_flags,
					MODESW_TM_PM0);
			}
			break;
		default:
			break;
	}
}

/* Timer handler for AP downgrade */
void wlc_modesw_oper_mode_timer(void *arg)
{
	wlc_modesw_cb_ctx_t *ctx = (wlc_modesw_cb_ctx_t *)arg;
	wlc_info_t *wlc = (wlc_info_t *)ctx->modesw_info->wlc;
	wlc_bsscfg_t *bsscfg = wlc_bsscfg_find_by_ID(wlc, ctx->connID);
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;

	/* in case bsscfg is freed before this callback is invoked */
	if (bsscfg == NULL) {
		WL_ERROR(("wl%d: %s: unable to find bsscfg by ID %p\n",
		          wlc->pub->unit, __FUNCTION__, arg));
		return;
	}

	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);
	if (ctx->notif) {
		if (ctx->status) {
			wlc_modesw_change_state(wlc->modesw, ctx->cfg, MSW_FAILED);
		}
		else {
			wlc_modesw_change_state(wlc->modesw, ctx->cfg, MSW_NOT_PENDING);
		}
		if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags, MODESW_CTRL_HANDLE_ALL_CFGS)) {
			int idx;
			wlc_bsscfg_t *icfg;
			FOREACH_AS_BSS(wlc, idx, icfg) {
				/* Call the registered callbacks */
				wlc_modesw_ctrl_hdl_all_cfgs(ctx->modesw_info, icfg,
					ctx->signal);
			}
		} else {
			ctx->notif = FALSE;
			wlc_modesw_notif_cb_notif(ctx->modesw_info, ctx->cfg, ctx->status,
				ctx->oper_mode, pmodesw_bsscfg->oper_mode_old, ctx->signal);
		}
		return;
	}
	WL_MODE_SWITCH(("AP downgrade timer expired...Perform the downgrade of AP.\n"));
	WL_MODE_SWITCH(("Yet to be receive Ack count = %d\n",
		pmodesw_bsscfg->action_sendout_counter));
#ifdef WLMCHAN
	if (MCHAN_ACTIVE(wlc->pub)) {
		if (pmodesw_bsscfg->state == MSW_DN_PERFORM) {
			if (pmodesw_bsscfg->action_sendout_counter == 0) {
				WL_MODE_SWITCH(("All acks received.... starting downgrade\n"));
				wlc_modesw_perform_upgrade_downgrade(wlc->modesw, bsscfg);
				wlc_modesw_change_state(wlc->modesw, bsscfg, MSW_NOT_PENDING);
				WL_MODE_SWITCH(("\n Sending signal = MODESW_DN_AP_COMPLETE\n"));
				wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg,
					BCME_OK, MODESW_DN_AP_COMPLETE);
			}
		}
	} else
#endif /* WLMCHAN */
	if (pmodesw_bsscfg->action_sendout_counter == 0) {
		WL_MODE_SWITCH(("All acks received.... starting downgrade\n"));
		wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
			MODESW_TM_PM1);
		/* For prot obss we drain beforehand */
		if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
			MODESW_CTRL_AP_ACT_FRAMES)) {
			wlc_modesw_perform_upgrade_downgrade(wlc->modesw, bsscfg);
		} else {
			/* Non prot obss cases we drain at this point */
			if (TXPKTPENDTOT(wlc) == 0) {
				wlc_modesw_change_state(wlc->modesw, bsscfg,
					MSW_DN_PERFORM);

				wlc_modesw_perform_upgrade_downgrade(wlc->modesw, bsscfg);
				bsscfg->oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
					bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);
				if (wlc->block_datafifo & DATA_BLOCK_TXCHAIN)
					WLC_BLOCK_DATAFIFO_CLEAR(wlc, DATA_BLOCK_TXCHAIN);
			} else {
				WLC_BLOCK_DATAFIFO_SET(wlc, DATA_BLOCK_TXCHAIN);
				wlc_modesw_change_state(wlc->modesw, bsscfg,
					MSW_WAIT_DRAIN);
				return;
			}
		}
		/* Update beacons and probe responses to reflect the update in bandwidth
		*/
		wlc_bss_update_beacon(wlc, bsscfg);
		wlc_bss_update_probe_resp(wlc, bsscfg, TRUE);

		wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg,
			BCME_OK, MODESW_DN_AP_COMPLETE);
		wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
			MODESW_TM_PM0);
	}
	return;
}

/* This function is responsible to understand the bandwidth
 * of the given operating mode and return the corresponding
 * bandwidth using chanpsec values. This is used to compare
 * the bandwidth of the operating mode and the chanspec
 */
uint16 wlc_modesw_get_bw_from_opermode(uint8 oper_mode, vht_op_chan_width_t width)
{
	uint16 opermode_bw_chspec_map[] = {
		WL_CHANSPEC_BW_20,
		WL_CHANSPEC_BW_40,
		WL_CHANSPEC_BW_80,
		/* Op mode value for 80+80 and 160 is same */
		WL_CHANSPEC_BW_8080
	};
	uint16 bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode);
	bw = opermode_bw_chspec_map[bw];
	/* Based on VHT OP IE chan width differentiate between 80+80 and 160 */
	if (bw == WL_CHANSPEC_BW_8080 && width == VHT_OP_CHAN_WIDTH_160) {
		bw = WL_CHANSPEC_BW_160;
	}
	return bw;
}

static int
wlc_modesw_opmode_pending(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	uint8 state;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);
	state = pmodesw_bsscfg->state;

	if ((state != MSW_DN_PERFORM) &&
	    (state !=	MSW_UP_PERFORM) &&
	    (state != MSW_DN_DEFERRED) &&
	    (state != MSW_UP_DEFERRED) &&
	    (state != MSW_UP_PM0_WAIT_ACK) &&
	    (state != MSW_DN_PM0_WAIT_ACK) &&
	    (state != MSW_UP_AF_WAIT_ACK) &&
	    (state !=	MSW_NOT_PENDING)) {
			return BCME_NOTREADY;
		}
	return BCME_OK;
}

/* This function handles the pending downgrade after the Tx packets are sent out */
void wlc_modesw_resume_opmode_change(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	int idx;
	wlc_bsscfg_t* icfg;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);

	if (!pmodesw_bsscfg || (pmodesw_bsscfg->state == MSW_NOT_PENDING)) {
		return;
	}
	WL_MODE_SWITCH(("wl%d: Resuming opmode for cfg = %d with state = %s\n",
		WLCWLUNIT(wlc), bsscfg->_idx, opmode_st_names[pmodesw_bsscfg->state].name));

	/* Check if other cfg's are done before any HW change */
	FOREACH_AS_BSS(wlc, idx, icfg) {
		if ((icfg != bsscfg) && wlc_modesw_opmode_pending(modesw_info, icfg)) {
			if (pmodesw_bsscfg->state == MSW_DN_PERFORM) {
				wlc_modesw_change_state(modesw_info, bsscfg, MSW_DN_DEFERRED);
			}
			else if (pmodesw_bsscfg->state == MSW_UP_PERFORM) {
				wlc_modesw_change_state(modesw_info, bsscfg, MSW_UP_DEFERRED);
			}
			return;
		}
	}

	/* Call actual HW change */
	wlc_modesw_perform_upgrade_downgrade(modesw_info, bsscfg);
	bsscfg->oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
		bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);

#ifdef WL_NAN
	if (BSSCFG_NAN(bsscfg)) {
		int signal = (pmodesw_bsscfg->state == MSW_UP_PERFORM) ?
			MODESW_UP_STA_COMPLETE : MODESW_DN_STA_COMPLETE;

		/* update nan modesw state to UP/DN complete */
		wlc_modesw_oper_mode_complete(modesw_info, bsscfg, BCME_OK, signal);
	}
	else
#endif
	if (pmodesw_bsscfg->state == MSW_UP_PERFORM) {
		wlc_modesw_change_state(modesw_info, bsscfg, MSW_UP_PM0_WAIT_ACK);
	}
	else {
		wlc_modesw_change_state(modesw_info, bsscfg, MSW_DN_PM0_WAIT_ACK);
	}
#ifdef STA
	wlc_modesw_set_pmstate(wlc->modesw, bsscfg, FALSE);
#endif

	return;
}


/* Returns TRUE if the new operating mode settings are lower than
 * the existing operating mode.
 */
bool wlc_modesw_is_downgrade(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	uint8 oper_mode_old, uint8 oper_mode_new)
{
	uint8 old_bw, new_bw, old_nss, new_nss;
	old_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode_old);
	new_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode_new);
	old_nss = DOT11_OPER_MODE_RXNSS(oper_mode_old);
	new_nss = DOT11_OPER_MODE_RXNSS(oper_mode_new);
	if (new_nss < old_nss)
		return TRUE;
	if (new_bw < old_bw)
		return TRUE;
	return FALSE;
}

/* Handles STA Upgrade and Downgrade process
 */
int wlc_modesw_change_sta_oper_mode(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg,
	uint8 oper_mode, uint8 enabled)
{
	wlc_info_t * wlc = modesw_info->wlc;
	struct scb *scb = NULL;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	ASSERT(bsscfg->associated);

	if (oper_mode == bsscfg->oper_mode)
	{
		WL_MODE_SWITCH(("No change in mode required. Returning...\n"));
		return BCME_OK;
	}

	if (wlc_modesw_is_connection_vht(wlc, bsscfg))
	{
		WL_MODE_SWITCH(("VHT case\n"));
		scb = wlc_scbfindband(wlc, bsscfg, &bsscfg->BSSID,
			CHSPEC_WLCBANDUNIT(bsscfg->current_bss->chanspec));

		bsscfg->oper_mode_enabled = enabled;

		if (oper_mode == bsscfg->oper_mode)
		{
			WL_MODE_SWITCH(("No change in mode required. Returning...\n"));
			return BCME_OK;
		}
	}
	pmodesw_bsscfg->oper_mode_new = oper_mode;
	pmodesw_bsscfg->oper_mode_old = bsscfg->oper_mode;
	pmodesw_bsscfg->orig_pmstate = bsscfg->pm->PMenabled;

	if (enabled) {
		wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, TRUE);
		/* Downgrade */
		if (wlc_modesw_is_downgrade(wlc, bsscfg, bsscfg->oper_mode, oper_mode))
		{
			wlc_modesw_change_state(modesw_info, bsscfg, MSW_DOWNGRADE_PENDING);
			pmodesw_bsscfg->new_chanspec =
				wlc_modesw_find_downgrade_chanspec(wlc,
				bsscfg, oper_mode, bsscfg->oper_mode);
			if (MCHAN_ACTIVE(wlc->pub))
				return BCME_OK;
			if (!CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
				MODESW_CTRL_DN_SILENT_DNGRADE)) {
				WL_MODE_SWITCH(("modesw:DN:sendingAction\n"));
				/* send action frame */
				wlc_modesw_send_action_frame_request(wlc->modesw, bsscfg,
					&bsscfg->BSSID, oper_mode);
			} else {
				/* eg: case: obss pseudo upgrade is done for stats collection,
				* but based on that we see that obss is still active. So this
				* path will come on downgrade. So, downgrade without
				* sending action frame.
				*/
				WL_MODE_SWITCH(("modesw:DN:silentDN\n"));

#ifdef WLMCHAN
				if (MCHAN_ACTIVE(wlc->pub)) {
					wlc_modesw_change_state(modesw_info,
						bsscfg, MSW_DN_PERFORM);
					return BCME_OK;
				}
#endif /* WLMCHAN */
				wlc_modesw_change_state(modesw_info, bsscfg, MSW_DN_PM1_WAIT_ACK);
				wlc_modesw_set_pmstate(modesw_info, bsscfg, TRUE);
			}
		}
		/* Upgrade */
		else {
			uint8 new_rxnss, old_rxnss;

			wlc_modesw_change_state(modesw_info, bsscfg, MSW_UPGRADE_PENDING);
			pmodesw_bsscfg->new_chanspec = wlc_modesw_find_upgrade_chanspec(modesw_info,
				bsscfg, oper_mode, bsscfg->oper_mode);
			new_rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_new);
			old_rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_old);
			if (pmodesw_bsscfg->new_chanspec == bsscfg->current_bss->chanspec &&
					(new_rxnss == old_rxnss)) {
				WL_MODE_SWITCH(("===========> No upgrade is possible \n"));
				wlc_modesw_change_state(modesw_info, bsscfg, MSW_NOT_PENDING);
				wlc_modesw_set_pmstate(modesw_info, bsscfg, FALSE);
				wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);
				/* Upgrade failed. inform functions of this failure */
				return BCME_BADOPTION;
			}

#ifdef WLMCHAN
			if (MCHAN_ACTIVE(wlc->pub)) {
				wlc_modesw_change_state(modesw_info, bsscfg, MSW_UP_PERFORM);
				return BCME_OK;
			}
#endif /* WLMCHAN */
			WL_MODE_SWITCH((" Upgrading Started ...Setting PM state to TRUE\n"));
			wlc_modesw_change_state(modesw_info, bsscfg, MSW_UP_PM1_WAIT_ACK);
			wlc_modesw_set_pmstate(modesw_info, bsscfg, TRUE);
		}
	}
	else {
		if (wlc_modesw_is_connection_vht(wlc, bsscfg)) {
			scb->flags3 &= ~(SCB3_OPER_MODE_NOTIF);
			WL_MODE_SWITCH(("Op mode notif disable request enable = %d\n", enabled));
		}
		bsscfg->oper_mode = oper_mode;
	}
	return BCME_OK;
}

/* Returns the new chanpsec value which could be used to downgrade
 * the bsscfg.
 */
chanspec_t
wlc_modesw_find_downgrade_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint8 oper_mode_new,
	uint8 oper_mode_old)
{
	chanspec_t ret_chspec = 0, curr_chspec;
	uint8 new_bw, old_bw;
	curr_chspec = cfg->current_bss->chanspec;
	new_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode_new);
	old_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode_old);

	if (old_bw <= new_bw)
		return curr_chspec;

	if (new_bw == DOT11_OPER_MODE_8080MHZ) {
		ASSERT(FALSE);
	}
	else if (new_bw == DOT11_OPER_MODE_80MHZ)
		ret_chspec = wf_chspec_primary80_chspec(curr_chspec);
	else if (new_bw == DOT11_OPER_MODE_40MHZ)
		ret_chspec = wf_chspec_primary40_chspec(curr_chspec);
	else if (new_bw == DOT11_OPER_MODE_20MHZ)
		ret_chspec = wf_chspec_ctlchspec(curr_chspec);
	return ret_chspec;
}

/* Returns the new chanpsec value which could be used to upgrade
 * the bsscfg.
 */
chanspec_t
wlc_modesw_find_upgrade_chanspec(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg,
uint8 oper_mode_new, uint8 oper_mode_old)
{
	chanspec_t curr_chspec, def_chspec;
	uint8 new_bw, old_bw, def_bw, def_opermode;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);

	def_chspec = pmodesw_bsscfg->max_opermode_chanspec;
	def_opermode = wlc_modesw_derive_opermode(modesw_info, def_chspec, cfg,
		modesw_info->wlc->stf->rxstreams);

	new_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode_new);
	old_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode_old);
	def_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(def_opermode);
	curr_chspec = cfg->current_bss->chanspec;

	WL_MODE_SWITCH(("Current bw = %x chspec = %x new_bw = %x orig_bw = %x\n", old_bw,
		curr_chspec, new_bw, def_bw));

	/* Compare if the channel is different in max_opermode_chanspec
	 * which is required to avoid an upgrade to a different channel
	 */
	if ((old_bw >= new_bw) ||
		(old_bw >= def_bw))
			return curr_chspec;

	if (new_bw == DOT11_OPER_MODE_80MHZ) {
		if (CHSPEC_IS8080(def_chspec))
			return wf_chspec_primary80_chspec(def_chspec);
	}
	else if (new_bw == DOT11_OPER_MODE_40MHZ) {
		if (CHSPEC_IS8080(def_chspec))
			return wf_chspec_primary40_chspec(wf_chspec_primary80_chspec(def_chspec));
		else if (CHSPEC_IS80(def_chspec)) {
			return wf_chspec_primary40_chspec(def_chspec);
		}
	}
	else if (new_bw == DOT11_OPER_MODE_20MHZ) {
		ASSERT(FALSE);
	}
	return def_chspec;

}

/* Function to override the current chspec for normal dngrade */
void
wlc_modesw_dynbw_tx_bw_override(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *bsscfg, uint32 *rspec_bw)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	chanspec_t override_chspec;

	ASSERT(bsscfg != NULL);
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	ASSERT(pmodesw_bsscfg != NULL);

	if (pmodesw_bsscfg->state != MSW_DOWNGRADE_PENDING) {
		return;
	}
	override_chspec = wlc_modesw_find_downgrade_chanspec(modesw_info->wlc, bsscfg,
		pmodesw_bsscfg->oper_mode_new, pmodesw_bsscfg->oper_mode_old);
	*rspec_bw = chspec_to_rspec(CHSPEC_BW(override_chspec));
	WL_MODE_SWITCH((" Sending out at overriden rspec bw = %x\n", *rspec_bw));
}


/* Sents Action frame Request to all the STA's Connected to AP
 */
void
wlc_modesw_ap_send_action_frames(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	struct scb_iter scbiter;
	struct scb *scb;

	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	pmodesw_bsscfg->action_sendout_counter = 0;
	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
		/* Sent Action frame only for Associated STA's */
		if (scb->state & ASSOCIATED)
		{
			pmodesw_bsscfg->action_sendout_counter++;
			WL_MODE_SWITCH(("Sending out frame no = %d \n",
				pmodesw_bsscfg->action_sendout_counter));
			wlc_modesw_send_action_frame_request(modesw_info, scb->bsscfg,
				&scb->ea, pmodesw_bsscfg->oper_mode_new);
		}
	}
	if ((pmodesw_bsscfg->state == MSW_DOWNGRADE_PENDING) &&
		(pmodesw_bsscfg->action_sendout_counter == 0)) {
		wlc_modesw_change_state(modesw_info, bsscfg, MSW_AP_DN_WAIT_DTIM);
		}
}

/* Handles AP Upgrade and Downgrade process
 */
bool
wlc_modesw_change_ap_oper_mode(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg,
	uint8 oper_mode, uint8 enabled)
{
	wlc_info_t * wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	uint32 ctrl_flags;

	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);

	if (bsscfg->oper_mode == oper_mode &&
		bsscfg->oper_mode_enabled == enabled)
		return FALSE;

	bsscfg->oper_mode_enabled = enabled;

	if (!enabled)
	{
		return TRUE;
	}
	/* Upgrade */
	if (!wlc_modesw_is_downgrade(wlc, bsscfg, bsscfg->oper_mode, oper_mode)) {
		WL_MODE_SWITCH(("AP upgrade \n"));
		pmodesw_bsscfg->new_chanspec = wlc_modesw_find_upgrade_chanspec(modesw_info,
			bsscfg, oper_mode, bsscfg->oper_mode);

		WL_MODE_SWITCH(("Got the new chanspec as %x bw = %x\n",
			pmodesw_bsscfg->new_chanspec,
			CHSPEC_BW(pmodesw_bsscfg->new_chanspec)));

		wlc_modesw_change_state(modesw_info, bsscfg, MSW_UPGRADE_PENDING);

		pmodesw_bsscfg->oper_mode_old = bsscfg->oper_mode;
		pmodesw_bsscfg->oper_mode_new = oper_mode;
		ctrl_flags = pmodesw_bsscfg->ctrl_flags;
		wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
			MODESW_TM_PM1);
		if (!CTRL_FLAGS_HAS(ctrl_flags, (MODESW_CTRL_UP_SILENT_UPGRADE))) {
			/* Transition to this state for non-obss dyn bw cases */
			wlc_modesw_change_state(modesw_info, bsscfg, MSW_UP_PERFORM);
		}
#ifdef WLMCHAN
		if (MCHAN_ACTIVE(wlc->pub)) {
			return TRUE;
		}
#endif /* WLMCHAN */
		wlc_modesw_perform_upgrade_downgrade(modesw_info, bsscfg);
		/* Update current oper mode to reflect upgraded bw */
		bsscfg->oper_mode = oper_mode;

		wlc_modesw_notif_cb_notif(modesw_info, bsscfg,
			BCME_OK, oper_mode, pmodesw_bsscfg->oper_mode_old, MODESW_PHY_UP_COMPLETE);

		if (!CTRL_FLAGS_HAS(ctrl_flags, (MODESW_CTRL_DN_SILENT_DNGRADE |
			MODESW_CTRL_UP_SILENT_UPGRADE))) {
			wlc_modesw_ap_upd_bcn_act(modesw_info, bsscfg, pmodesw_bsscfg->state);
		}
		wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
			MODESW_TM_PM0);
		wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg, BCME_OK,
			MODESW_UP_AP_COMPLETE);
	}
	/* Downgrade */
	else {
		uint8 new_rxnss, old_rxnss;
		WL_MODE_SWITCH(("AP Downgrade Case \n"));
		pmodesw_bsscfg->new_chanspec = wlc_modesw_find_downgrade_chanspec(wlc, bsscfg,
			oper_mode, bsscfg->oper_mode);
		if (!pmodesw_bsscfg->new_chanspec)
			return FALSE;
		WL_MODE_SWITCH(("Got the new chanspec as %x bw = %x\n",
			pmodesw_bsscfg->new_chanspec,
			CHSPEC_BW(pmodesw_bsscfg->new_chanspec)));
		if (pmodesw_bsscfg->state == MSW_DOWNGRADE_PENDING)
			WL_MODE_SWITCH(("Entering %s again \n", __FUNCTION__));

		wlc_modesw_change_state(modesw_info, bsscfg, MSW_DOWNGRADE_PENDING);
		pmodesw_bsscfg->oper_mode_new = oper_mode;
		pmodesw_bsscfg->oper_mode_old = bsscfg->oper_mode;

		new_rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_new);
		old_rxnss = DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_old);
		if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
			MODESW_CTRL_AP_ACT_FRAMES) &&
			(new_rxnss == old_rxnss)) {
			/* For prot obss we do the drain before sending out action frames */
			if (TXPKTPENDTOT(wlc) != 0) {
				WLC_BLOCK_DATAFIFO_SET(wlc, DATA_BLOCK_TXCHAIN);
				wlc_modesw_change_state(wlc->modesw, bsscfg, MSW_WAIT_DRAIN);
				return TRUE;
			}

			if (wlc->block_datafifo & DATA_BLOCK_TXCHAIN) {
				/* Unblock and measure time at drain end */
				WLC_BLOCK_DATAFIFO_CLEAR(wlc, DATA_BLOCK_TXCHAIN);
			}
		}
		/* Update current oper mode to reflect downgraded bw */
		bsscfg->oper_mode = oper_mode;
		if (!CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
				MODESW_CTRL_DN_SILENT_DNGRADE)) {
			wlc_modesw_ap_upd_bcn_act(modesw_info, bsscfg, pmodesw_bsscfg->state);
		}
		else {
			WL_MODE_SWITCH(("Pseudo downgrade to %x\n",
				pmodesw_bsscfg->new_chanspec));
			wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
				MODESW_TM_PM1);
#ifdef WLMCHAN
			if (MCHAN_ACTIVE(wlc->pub)) {
				wlc_modesw_change_state(modesw_info, bsscfg, MSW_AP_DN_WAIT_DTIM);
				return TRUE;
			}
#endif /* WLMCHAN */
			wlc_modesw_perform_upgrade_downgrade(modesw_info, bsscfg);
			wlc_modesw_notif_cb_notif(modesw_info, bsscfg, BCME_OK,
					bsscfg->oper_mode, pmodesw_bsscfg->oper_mode_old,
					MODESW_DN_AP_COMPLETE);
			wlc_modesw_change_state(modesw_info, bsscfg, MSW_NOT_PENDING);
			wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
				MODESW_TM_PM0);
		}
	}
	if (pmodesw_bsscfg->state == MSW_UPGRADE_PENDING) {
		wlc_modesw_change_state(modesw_info, bsscfg, MSW_NOT_PENDING);
	}

	return TRUE;
}

/* Handles AP and STA case bandwidth Separately
 */
int wlc_modesw_handle_oper_mode_notif_request(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *bsscfg, uint8 oper_mode, uint8 enabled, uint32 ctrl_flags)
{
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg;
	uint8 old_bw, new_bw, old_nss, new_nss;
	int err = BCME_OK;

	if (!bsscfg)
		return BCME_NOMEM;

	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	if ((wlc_modesw_is_connection_vht(wlc, bsscfg)) &&
		(!MODESW_BSS_VHT_ENAB(wlc, bsscfg))) {
		return BCME_UNSUPPORTED;
	}

	if (ANY_SCAN_IN_PROGRESS(wlc->scan)) {
		/* Abort any SCAN in progress */
		wlc_scan_abort(wlc->scan, WLC_E_STATUS_ABORT);
	}

	old_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(bsscfg->oper_mode);
	new_bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode);
	old_nss = DOT11_OPER_MODE_RXNSS(bsscfg->oper_mode);
	new_nss = DOT11_OPER_MODE_RXNSS(oper_mode);

	/* For now we only support either an upgrade of  Nss/BW, or a downgrade
	 * of Nss/BW, not a combination of up/downgrade. Code currently handles
	 * an upgrade by changing the operation mode immediately and waiting for
	 * annoucing the capability, and handles a downgrade by announcing the
	 * lower capability, waiting for peers to act on the notification, then changing
	 * the operating mode. Handling a split up/downgrade would involve more
	 * code to change part of our operational mode immediatly, and part after
	 * a delay. Some hardware may not be able to support the intermmediate
	 * mode. The split up/downgrade is not an important use case, so avoiding
	 * the code complication.
	 */

	/* Skip handling of a combined upgrade/downgrade of Nss and BW */
	if (((new_bw < old_bw) && (new_nss > old_nss)) ||
			((new_bw > old_bw) && (new_nss < old_nss))) {
		return BCME_BADARG;
	}

	if ((pmodesw_bsscfg->state != MSW_NOT_PENDING) &&
		(pmodesw_bsscfg->state != MSW_FAILED))
		return BCME_NOTREADY;

	pmodesw_bsscfg->ctrl_flags = ctrl_flags;

	if (BSSCFG_AP(bsscfg) || (BSSCFG_IBSS(bsscfg) && !BSSCFG_NAN(bsscfg))) {
		/* Initialize the AP operating mode if UP */
		WL_MODE_SWITCH((" AP CASE \n"));
		if (enabled && !bsscfg->oper_mode_enabled && bsscfg->up)
		{
			pmodesw_bsscfg->max_opermode_chanspec =
				bsscfg->current_bss->chanspec;
			bsscfg->oper_mode_enabled = enabled;
			/* Initialize the oper_mode */
			bsscfg->oper_mode =
				wlc_modesw_derive_opermode(modesw_info,
				bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);
		}
		if (bsscfg->up) {
			wlc_modesw_change_ap_oper_mode(modesw_info, bsscfg, oper_mode,
				enabled);
		}
		else {
			bsscfg->oper_mode = oper_mode;
			bsscfg->oper_mode_enabled = enabled;
		}
	}

	if (BSSCFG_INFRA_STA(bsscfg) || BSSCFG_NAN(bsscfg)) {
		if (WLC_BSS_CONNECTED(bsscfg)) {
			struct scb *scb = NULL;

			scb = wlc_scbfindband(wlc, bsscfg, &bsscfg->BSSID,
				CHSPEC_WLCBANDUNIT(bsscfg->current_bss->chanspec));
			BCM_REFERENCE(scb);
			/* for VHT AP, if oper mode is not supported, throw error
			* and continue with other means of mode switch (MIMOPS, CH WIDTH etc)
			*/
			if (wlc_modesw_is_connection_vht(wlc, bsscfg) &&
				!(SCB_OPER_MODE_NOTIF_CAP(scb))) {
				WL_ERROR(("wl%d: No capability for opermode switch in VHT AP"
					"....Unexpected..\n", WLCWLUNIT(wlc)));
			}

			if (enabled && !bsscfg->oper_mode_enabled) {
				bsscfg->oper_mode_enabled = enabled;
				bsscfg->oper_mode =
					wlc_modesw_derive_opermode(wlc->modesw,
					bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);
			}
			pmodesw_bsscfg->start_time = OSL_SYSUPTIME();
			err = wlc_modesw_change_sta_oper_mode(modesw_info, bsscfg,
				oper_mode, enabled);
		}
		else {
			bsscfg->oper_mode = oper_mode;
			bsscfg->oper_mode_enabled = enabled;
		}
	}

	if (!enabled) {
		bsscfg->oper_mode = (uint8)FALSE;
		return TRUE;
	}
	return err;
}

/* Prepares the operating mode field based upon the
 * bandwidth specified in the given chanspec and rx streams
 * configured for the WLC.
 */
uint8
wlc_modesw_derive_opermode(wlc_modesw_info_t *modesw_info, chanspec_t chanspec,
	wlc_bsscfg_t *bsscfg, uint8 rxstreams)
{
	wlc_info_t *wlc = modesw_info->wlc;
	uint8 bw = DOT11_OPER_MODE_20MHZ, rxnss, rxnss_type;
	/* Initialize the oper_mode */
	if (CHSPEC_IS8080(chanspec))
		bw = DOT11_OPER_MODE_8080MHZ;
	else if (CHSPEC_IS80(chanspec))
		bw = DOT11_OPER_MODE_80MHZ;
	else if (CHSPEC_IS40(chanspec))
		bw = DOT11_OPER_MODE_40MHZ;
	else if (WLC_ULB_CHSPEC_ISLE20(wlc, chanspec))
		bw = DOT11_OPER_MODE_20MHZ;
	else {
		ASSERT(FALSE);
	}
	if (wlc_modesw_is_connection_vht(wlc, bsscfg)) {
		rxnss =  MIN(rxstreams, VHT_CAP_MCS_MAP_NSS_MAX);
		rxnss_type = FALSE; /* Currently only type 0 is supported */
		return DOT11_OPER_MODE(rxnss_type, rxnss, bw);
	} else {
		rxnss =  MIN(rxstreams, WLC_BITSCNT(wlc->stf->hw_rxchain));
		rxnss_type = FALSE; /* Currently only type 0 is supported */
		return DOT11_OPER_MODE(rxnss_type, rxnss, bw);
	}
}
/* This function will check the txstatus value for the Action frame request.
* If ACK not Received,Call Action Frame Request again and Return FALSE -> no cb_ctx will happen
* If retry limit  exceeded,Send disassoc Reuest and return TRUE so that we can clear the scb cb_ctx
* If ACK Received properly,return TRUE so that we can clear the scb cb_ctx
*/
static bool
wlc_modesw_process_action_frame_status(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct scb *scb,
bool *cntxt_clear, uint txstatus)
{
	modesw_scb_cubby_t *pmodesw_scb = MODESW_SCB_CUBBY(wlc->modesw, scb);
	wlc_modesw_cb_ctx_t *ctx = pmodesw_scb->cb_ctx;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg, *ipmode_cfg;
	wlc_bsscfg_t *icfg;
	int idx;

	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);


	ASSERT(pmodesw_scb != NULL);
	ASSERT(ctx != NULL);

	if (BSSCFG_AP(bsscfg)) {
		WL_MODE_SWITCH(("Value of sendout counter = %d\n",
			pmodesw_bsscfg->action_sendout_counter));
		pmodesw_bsscfg->action_sendout_counter--;
	}

	if (txstatus & TX_STATUS_ACK_RCV) {
		WL_ERROR(("wl%d: cfg = %d Action Frame Ack received \n",
			WLCWLUNIT(wlc), bsscfg->_idx));
		 /* Reset the Retry counter if ACK is received properly */
		 pmodesw_scb->modesw_retry_counter = 0;
	}
	else if ((txstatus & TX_STATUS_MASK) == TX_STATUS_NO_ACK)
	{
		WL_ERROR(("ACK NOT RECEIVED... retrying....%d\n",
			pmodesw_scb->modesw_retry_counter+1));
		/* Clearing  the cb_cxt */
		MFREE(wlc->osh, ctx, sizeof(wlc_modesw_cb_ctx_t));
		pmodesw_scb->cb_ctx = NULL;
		*cntxt_clear = FALSE;
		if (pmodesw_scb->modesw_retry_counter <=
			ACTION_FRAME_RETRY_LIMIT) {
			pmodesw_scb->modesw_retry_counter++;
			/* Increment the sendout counter only for AP case */
			if (BSSCFG_AP(bsscfg))
			pmodesw_bsscfg->action_sendout_counter++;
			/* Send Action frame again */
			wlc_modesw_send_action_frame_request(wlc->modesw,
			scb->bsscfg, &scb->ea,
			pmodesw_bsscfg->oper_mode_new);
			return FALSE;
		}
		else {
			/* Retry limit Exceeded */
			pmodesw_scb->modesw_retry_counter = 0;
			/* Disassoc the non-acking recipient
			* For STA and PSTAs, they'll roam and reassoc back
			*/
			if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
					MODESW_CTRL_NO_ACK_DISASSOC)) {
				wlc_senddisassoc(wlc, bsscfg, scb, &scb->ea,
					&bsscfg->BSSID, &bsscfg->cur_etheraddr,
					DOT11_RC_INACTIVITY);
			}
			if ((BSSCFG_STA(bsscfg)) &&
				(wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg, BCME_ERROR,
				MODESW_ACTION_FAILURE) == BCME_NOTREADY)) {
				WL_MODE_SWITCH(("wl%d: Resume others (cfg %d)\n",
					WLCWLUNIT(wlc), bsscfg->_idx));
				FOREACH_AS_BSS(wlc, idx, icfg) {
					ipmode_cfg = MODESW_BSSCFG_CUBBY(wlc->modesw, icfg);
					if ((icfg != bsscfg) &&
						(ipmode_cfg->state ==
						MSW_DN_DEFERRED)) {
						ipmode_cfg->state = MSW_DN_PERFORM;
						wlc_modesw_resume_opmode_change(wlc->modesw, icfg);
					}
				}
				wlc_modesw_ctrl_hdl_all_cfgs(wlc->modesw, bsscfg,
					MODESW_ACTION_FAILURE);
			}
		}
	}
	return TRUE;
}

/* This function is used when we are in partial state
* a) action frame sent for UP/DN grade and we received callback, but scb becomes null
* b) based on action frame complete status. That is "ACK is successfully received or
* Retry Limit exceeded"
*/
static void
wlc_modesw_update_PMstate(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg, *ipmode_cfg;
	wlc_bsscfg_t *icfg;
	int idx;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);
	if (BSSCFG_STA(bsscfg)) {
	/* Perform the downgrade and report success */
		if (pmodesw_bsscfg->state == MSW_DN_AF_WAIT_ACK) {
#ifdef WLMCHAN
			if (MCHAN_ACTIVE(wlc->pub)) {
				wlc_modesw_change_state(wlc->modesw, bsscfg, MSW_DN_PERFORM);
				return;
			}
#endif /* WLMCHAN */
			wlc_modesw_change_state(wlc->modesw, bsscfg, MSW_DN_PM1_WAIT_ACK);
			WL_MODE_SWITCH(("Setting PM to TRUE for downgrade \n"));
			wlc_modesw_set_pmstate(wlc->modesw, bsscfg, TRUE);
		}
		else if (pmodesw_bsscfg->state == MSW_UP_AF_WAIT_ACK)
		{
			WL_MODE_SWITCH(("modesw:Upgraded: opermode: %x, chanspec: %x\n",
				bsscfg->oper_mode,
				bsscfg->current_bss->chanspec));
			wlc_modesw_change_state(wlc->modesw, bsscfg, MSW_NOT_PENDING);
			if (wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg,
				BCME_OK, MODESW_UP_STA_COMPLETE) == BCME_NOTREADY) {
				WL_MODE_SWITCH(("wl%d: Resume others (cfg %d)\n",
					WLCWLUNIT(wlc), bsscfg->_idx));
				FOREACH_AS_BSS(wlc, idx, icfg) {
					ipmode_cfg = MODESW_BSSCFG_CUBBY(wlc->modesw, icfg);
					if ((icfg != bsscfg) &&
						(ipmode_cfg->state ==
						MSW_UP_DEFERRED)) {
						ipmode_cfg->state = MSW_UP_PERFORM;
						wlc_modesw_resume_opmode_change(wlc->modesw, icfg);
					}
				}
			}
			wlc_modesw_time_measure(wlc->modesw, pmodesw_bsscfg->ctrl_flags,
				MODESW_TM_ACTIONFRAME_COMPLETE);
		}
	}
	else if (BSSCFG_AP(bsscfg)) {
		if (pmodesw_bsscfg->state == MSW_DN_AF_WAIT_ACK &&
			pmodesw_bsscfg->action_sendout_counter == 0)
		{
			/* Set state as DEFERRED for downgrade case and
			* process in tbtt using this state
			*/
			wlc_modesw_change_state(wlc->modesw, bsscfg, MSW_AP_DN_WAIT_DTIM);
		}
	}
	return;
}

/* Receives the Frame Acknowledgement and Retries till ACK received Properly
 * and Updates PM States Properly
 */

static void
wlc_modesw_send_action_frame_complete(wlc_info_t *wlc, uint txstatus, void *arg)
{
	wlc_modesw_cb_ctx_t *ctx;
	wlc_bsscfg_t *bsscfg;
	struct scb *scb = NULL;
	bool cntxt_clear = TRUE;
	modesw_scb_cubby_t *pmodesw_scb = NULL;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;

	pmodesw_scb = (modesw_scb_cubby_t*)arg;
	ctx = pmodesw_scb->cb_ctx;

	if (ctx == NULL)
			return;
	WL_MODE_SWITCH(("wlc_modesw_send_action_frame_complete called \n"));
	bsscfg = wlc_bsscfg_find_by_ID(wlc, ctx->connID);

	/* in case bsscfg is freed before this callback is invoked */
	if (bsscfg == NULL) {
		WL_ERROR(("wl%d: %s: unable to find bsscfg by ID %p\n",
		          wlc->pub->unit, __FUNCTION__, arg));
		wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);
		/* Sync up the SW and MAC wake states */
		return;
	}

	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);
	scb = WLPKTTAGSCBGET(ctx->pkt);

	/* Make sure that we have oper mode change pending for this cfg.
	* It may happen that we clear the oper mode context for this cfg
	* due to timeout.
	*/
	if (!pmodesw_bsscfg || ((pmodesw_bsscfg->state != MSW_DN_AF_WAIT_ACK) &&
		(pmodesw_bsscfg->state != MSW_UP_AF_WAIT_ACK))) {
		return;
	}


	/* make sure the scb still exists */
	if (scb == NULL) {
		WL_ERROR(("wl%d: %s: unable to find scb from the pkt %p\n",
		          wlc->pub->unit, __FUNCTION__, ctx->pkt));
		wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);
		wlc_modesw_update_PMstate(wlc, bsscfg);
		return;
	}
	/* Retry Action frame sending upto RETRY LIMIT if ACK not received for STA and AP case */
	if (wlc_modesw_process_action_frame_status(wlc, bsscfg, scb, &cntxt_clear,
		txstatus) == TRUE) {
		/* Upgrade or Downgrade the Bandwidth only for STA case -
		* This will be called only if ACK is successfully received or Retry Limit exceeded
		*/
		wlc_modesw_update_PMstate(wlc, bsscfg);
	}
	/* For Upgrade Case : After all ACKs are received - set state as  MODESW_UP_AP_COMPLETE */
	if (pmodesw_bsscfg->action_sendout_counter == 0 && (BSSCFG_AP(bsscfg)) &&
		pmodesw_bsscfg->state == MSW_UP_AF_WAIT_ACK) {
		wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg, BCME_OK, MODESW_UP_AP_COMPLETE);
	}

	if (cntxt_clear) {
		MFREE(wlc->osh, ctx, sizeof(wlc_modesw_cb_ctx_t));
		pmodesw_scb->cb_ctx = NULL;
	}
}

/* Return the current state of the modesw module for given bsscfg */
bool
wlc_modesw_opmode_ie_reqd(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	wlc_info_t *wlc = modesw_info->wlc;
	uint8 ret = FALSE, narrow_nss = FALSE;

	ASSERT(bsscfg != NULL);

	narrow_nss = (((uint)(DOT11_OPER_MODE_RXNSS(bsscfg->oper_mode)))
		< WLC_BITSCNT(wlc->stf->hw_rxchain_cap));

	if ((bsscfg->oper_mode_enabled) &&
		(narrow_nss || (pmodesw_bsscfg->state == MSW_UP_PERFORM))) {
		ret = TRUE;
	}
	return ret;
}

/* Prepare and send the operating mode notification action frame
 * Registers a callback to handle the acknowledgement
 */
static int
wlc_modesw_send_action_frame_request(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg,
	const struct ether_addr *ea, uint8 oper_mode_new) {
	struct scb *scb = NULL;
	void *p;
	uint8 *pbody;
	uint body_len;
	struct dot11_action_vht_oper_mode *ahdr;
	struct dot11_action_ht_ch_width *ht_hdr;
	struct dot11_action_ht_mimops *ht_mimops_hdr;
	wlc_info_t *wlc = modesw_info->wlc;
	modesw_scb_cubby_t *pmodesw_scb = NULL;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	wlc_pkttag_t* pkttag;

	ASSERT(bsscfg != NULL);

	scb = wlc_scbfindband(wlc, bsscfg, ea,
		CHSPEC_WLCBANDUNIT(bsscfg->current_bss->chanspec));

	/* If STA disassoc during this time, update PM state */
	if (scb == NULL) {
		WL_ERROR(("Inside %s scb not found \n", __FUNCTION__));
		wlc_modesw_update_PMstate(wlc, bsscfg);
		return BCME_OK;
	}

	/* Legacy Peer */
	if (!wlc_modesw_is_connection_vht(wlc, bsscfg) &&
	    !wlc_modesw_is_connection_ht(wlc, bsscfg)) {
		/* Legacy Peer. Skip action frame stage */
		pmodesw_bsscfg->oper_mode_new = oper_mode_new;
		if (wlc_modesw_is_downgrade(wlc, bsscfg,
		    pmodesw_bsscfg->oper_mode_old, oper_mode_new)) {
			wlc_modesw_change_state(modesw_info, bsscfg, MSW_DN_AF_WAIT_ACK);
		} else {
			wlc_modesw_change_state(modesw_info, bsscfg, MSW_UP_AF_WAIT_ACK);
		}
		wlc_modesw_update_PMstate(wlc, bsscfg);
		return BCME_OK;
	}

	pmodesw_scb = MODESW_SCB_CUBBY(wlc->modesw, scb);
	if (wlc_modesw_is_connection_vht(wlc, bsscfg))
		body_len = sizeof(struct dot11_action_vht_oper_mode);
	else
		body_len = sizeof(struct dot11_action_ht_ch_width);

	p = wlc_frame_get_mgmt(wlc, FC_ACTION, ea, &bsscfg->cur_etheraddr,
		&bsscfg->BSSID, body_len, &pbody);
	if (p == NULL)
	{
		WL_ERROR(("Unable to allocate the mgmt frame \n"));
		return BCME_NOMEM;
	}

	/* Avoid NULL frame for this pkt in PM2 */
	pkttag = WLPKTTAG(p);
	pkttag->flags3 |= WLF3_NO_PMCHANGE;

	if ((pmodesw_scb->cb_ctx = ((wlc_modesw_cb_ctx_t *)MALLOCZ(modesw_info->wlc->osh,
		sizeof(wlc_modesw_cb_ctx_t)))) == NULL) {
		WL_ERROR(("Inside %s....cant allocate context \n", __FUNCTION__));
		PKTFREE(wlc->osh, p, TRUE);
		return BCME_NOMEM;
	}

	if (SCB_OPER_MODE_NOTIF_CAP(scb) &&
		wlc_modesw_is_connection_vht(wlc, bsscfg)) {
		WL_MODE_SWITCH(("wl%d: cfg %d Send Oper Mode Action Frame..\n", WLCWLUNIT(wlc),
			bsscfg->_idx));
		ahdr = (struct dot11_action_vht_oper_mode *)pbody;
		ahdr->category = DOT11_ACTION_CAT_VHT;
		ahdr->action = DOT11_VHT_ACTION_OPER_MODE_NOTIF;
		ahdr->mode = oper_mode_new;
	} else if (wlc_modesw_is_connection_vht(wlc, bsscfg) ||
	wlc_modesw_is_connection_ht(wlc, bsscfg)) {
		if (DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_old)
			!= DOT11_OPER_MODE_RXNSS(pmodesw_bsscfg->oper_mode_new)) {
			WL_MODE_SWITCH(("wl%d: cfg %d Send MIMO PS Action Frame..\n",
				WLCWLUNIT(wlc), bsscfg->_idx));
			/* NSS change needs MIMOPS action frame */
			ht_mimops_hdr = (struct dot11_action_ht_mimops *)pbody;
			ht_mimops_hdr->category = DOT11_ACTION_CAT_HT;
			ht_mimops_hdr->action = DOT11_ACTION_ID_HT_MIMO_PS;
			if (wlc_modesw_is_downgrade(wlc, bsscfg, bsscfg->oper_mode,
				oper_mode_new)) {
				ht_mimops_hdr->control = SM_PWRSAVE_ENABLE;
			} else {
				ht_mimops_hdr->control = 0;
			}
		} else {
			ht_hdr = (struct dot11_action_ht_ch_width *) pbody;
			ht_hdr->category = DOT11_ACTION_CAT_HT;
			ht_hdr->action = DOT11_ACTION_ID_HT_CH_WIDTH;
			ht_hdr->ch_width = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode_new);
		}
	}
	pmodesw_bsscfg->oper_mode_new = oper_mode_new;
	pmodesw_scb->cb_ctx->pkt = p;
	pmodesw_scb->cb_ctx->connID = bsscfg->ID;

	wlc_sendmgmt(wlc, p, bsscfg->wlcif->qi, scb);
	WL_MODE_SWITCH(("Registering the action frame callback in state %s for cfg %d\n",
		opmode_st_names[pmodesw_bsscfg->state].name, bsscfg->_idx));
	if (wlc_modesw_is_downgrade(wlc, bsscfg, pmodesw_bsscfg->oper_mode_old,
		oper_mode_new)) {
		wlc_modesw_change_state(modesw_info, bsscfg, MSW_DN_AF_WAIT_ACK);
	} else {
		wlc_modesw_change_state(modesw_info, bsscfg, MSW_UP_AF_WAIT_ACK);
	}
	wlc_pcb_fn_register(wlc->pcb,
		wlc_modesw_send_action_frame_complete, (void*)pmodesw_scb, p);
	return BCME_OK;
}

/* Function finds the New Bandwidth depending on the Switch Type
*	bw is an IN/OUT parameter
*/
static int
wlc_modesw_get_next_bw(uint8 *bw, uint8 switch_type, uint8 max_bw)
{
	uint8 new_bw = 0;

	if (switch_type == BW_SWITCH_TYPE_DNGRADE) {
		if (*bw == DOT11_OPER_MODE_20MHZ)
			return BCME_ERROR;

		new_bw = *bw - 1;
	} else if (switch_type == BW_SWITCH_TYPE_UPGRADE) {
		new_bw = *bw + 1;
	}
	WL_MODE_SWITCH(("%s:cur:%d,max:%d,new:%d\n", __FUNCTION__, *bw, max_bw, new_bw));
	if (new_bw > max_bw)
		return BCME_ERROR;

	*bw = new_bw;

	return BCME_OK;
}

/* This function mainly identifies whether the connection is VHT or HT
*   and calls the Notify Request function to switch the Bandwidth
*/

int
wlc_modesw_bw_switch(wlc_modesw_info_t *modesw_info,
	chanspec_t chanspec, uint8 switch_type, wlc_bsscfg_t *cfg, uint32 ctrl_flags)
{
	int iov_oper_mode = 0;
	uint8 oper_mode = 0;
	uint8 max_bw = 0, bw = 0;
	int err = BCME_OK;
	bool enabled = TRUE;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg =	MODESW_BSSCFG_CUBBY(modesw_info, cfg);
	if (wlc_modesw_is_req_valid(modesw_info, cfg) != TRUE) {
		return BCME_BUSY;
	}
	pmodesw_bsscfg->ctrl_flags = ctrl_flags;

	wlc_modesw_time_measure(modesw_info, ctrl_flags, MODESW_TM_START);
	/* send ONLY action frame now */
	if (CTRL_FLAGS_HAS(ctrl_flags, MODESW_CTRL_UP_ACTION_FRAMES_ONLY)) {
		wlc_modesw_change_state(modesw_info, cfg, MSW_UPGRADE_PENDING);
		if (BSSCFG_STA(cfg)) {
			return wlc_modesw_send_action_frame_request(modesw_info, cfg,
				&cfg->BSSID, pmodesw_bsscfg->oper_mode_new);
		}
		else {
			wlc_modesw_ap_upd_bcn_act(modesw_info, cfg,
				pmodesw_bsscfg->state);
			if (pmodesw_bsscfg->action_sendout_counter == 0) {
				wlc_modesw_oper_mode_complete(modesw_info, cfg, BCME_OK,
					MODESW_UP_AP_COMPLETE);
			}
			wlc_modesw_time_measure(modesw_info, pmodesw_bsscfg->ctrl_flags,
				MODESW_TM_ACTIONFRAME_COMPLETE);
			return BCME_OK;
		}
	}

	WL_MODE_SWITCH(("%s:cs:%x\n", __FUNCTION__, chanspec));

	iov_oper_mode = cfg->oper_mode & 0xff;
	if (wlc_modesw_is_connection_vht(modesw_info->wlc, cfg))
		max_bw = DOT11_OPER_MODE_160MHZ;
	else
		max_bw = DOT11_OPER_MODE_40MHZ;
	if (!cfg->oper_mode_enabled) {
		/* if not enabled, based on current bw, set values. TBD:
		* nss not taken care
		*/
		oper_mode = wlc_modesw_derive_opermode(modesw_info,
			chanspec, cfg, modesw_info->wlc->stf->rxstreams);
	} else {
		oper_mode = cfg->oper_mode;
	}
	bw = DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode);
	if ((err = wlc_modesw_get_next_bw(&bw, switch_type, max_bw)) ==
		BCME_OK) {
		if (wlc_modesw_is_connection_vht(modesw_info->wlc, cfg)) {
			iov_oper_mode = ((DOT11_OPER_MODE_RXNSS(oper_mode)-1) << 4)
				| bw;
			WL_MODE_SWITCH(("%s:conVHT:%x\n", __FUNCTION__, iov_oper_mode));
		}
		else {
				iov_oper_mode = bw;
				WL_MODE_SWITCH(("%s:conHT: %x\n", __FUNCTION__, iov_oper_mode));
		}
		if (wlc_modesw_is_req_valid(modesw_info, cfg) != TRUE)
			return BCME_BUSY;
		err = wlc_modesw_handle_oper_mode_notif_request(modesw_info, cfg,
			(uint8)iov_oper_mode, enabled, ctrl_flags);
	} else {
	WL_MODE_SWITCH(("%s: error\n", __FUNCTION__));
	}
	return err;
}

/* Function restores all modesw variables and states to
* default values
*/
static void
wlc_modesw_restore_defaults(void *ctx, wlc_bsscfg_t *bsscfg)
{
	wlc_modesw_info_t *modesw_info = (wlc_modesw_info_t *)ctx;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, bsscfg);
	if (pmodesw_bsscfg != NULL) {
		pmodesw_bsscfg->state = MSW_NOT_PENDING;
		pmodesw_bsscfg->oper_mode_new = 0;
		pmodesw_bsscfg->oper_mode_old = 0;
	}
}

/* Callback from assoc. This Function will free the timer context and reset
* the bsscfg variables when STA association state gets updated.
*/
static void
wlc_modesw_assoc_cxt_cb(void *ctx, bss_assoc_state_data_t *notif_data)
{
	ASSERT(notif_data->cfg != NULL);
	ASSERT(ctx != NULL);

	if (notif_data->state != AS_IDLE)
		return;

	WL_MODE_SWITCH(("%s:Got Callback from Assoc. Clearing Context\n",
		__FUNCTION__));

	if (!notif_data->cfg)
		return;

	if (notif_data->state == AS_IDLE) {
		wlc_modesw_clear_phy_chanctx(ctx, notif_data->cfg);
	}

}

/* Callback from wldown. This Function will reset
* the bsscfg variables on wlup event.
*/
static void
wlc_modesw_updown_cb(void *ctx, bsscfg_up_down_event_data_t *updown_data)
{
	ASSERT(updown_data->bsscfg != NULL);
	ASSERT(ctx != NULL);

	if (updown_data->up == FALSE)
	{
		/* Clear PHY context during wl down */
		wlc_modesw_clear_phy_chanctx(ctx, updown_data->bsscfg);
		return;
	}
	WL_MODE_SWITCH(("wl%d: %s: got callback from updown. intf up resetting modesw\n",
		WLCWLUNIT(updown_data->bsscfg->wlc), __FUNCTION__));

	wlc_modesw_restore_defaults(ctx, updown_data->bsscfg);
}

/* Function to Clear PHY context created during bandwidth switching */
static void
wlc_modesw_clear_phy_chanctx(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_modesw_info_t *modesw_info = (wlc_modesw_info_t *)ctx;
	wlc_info_t *wlc = modesw_info->wlc;
	int idx;
	wlc_bsscfg_t *bsscfg;
	chanspec_t chanspec_80, chanspec_40, chanspec_20;
	chanspec_t ctl_ch;
	bool cspec80 = FALSE, cspec40 = FALSE, cspec20 = FALSE;

	WL_MODE_SWITCH(("%s: Clearing PHY Contexts chanspec[%x] cfg[%p]\n",
		__FUNCTION__, cfg->current_bss->chanspec, cfg));

	/* Get Control channel using the current bsscfg chanspec */
	ctl_ch = wf_chspec_ctlchan(cfg->current_bss->chanspec);

	/* Get 80 Mhz Chanspec  using the Control channel */
	chanspec_80 = wf_channel2chspec(ctl_ch, WL_CHANSPEC_BW_80);

	/* Get 40 Mhz Chanspec  using the Control channel */
	chanspec_40 = wf_channel2chspec(ctl_ch, WL_CHANSPEC_BW_40);

	/* Get 20 Mhz Chanspec  using the Control channel */
	{
		uint16 min_bw = WL_CHANSPEC_BW_20;
#ifdef WL11ULB
		min_bw = WLC_ULB_GET_BSS_MIN_BW(wlc, cfg);
#endif /* WL11ULB */
		chanspec_20 = wf_channel2chspec(ctl_ch, min_bw);
	}
	WL_MODE_SWITCH(("%s: chanspec_80 [%x]chanspec_40[%x] chanspec_20 [%x]\n",
		__FUNCTION__, chanspec_80, chanspec_40, chanspec_20));

	/* Check whether same chanspec is used by any other interface or not.
	* If so,Dont clear that chanspec phy context as it can be used by other interface
	*/
	FOREACH_BSS(wlc, idx, bsscfg) {

		WL_MODE_SWITCH(("%s: idx [%d] bsscfg [%p] <===> cfg[%p]\n",
			__FUNCTION__, idx, bsscfg, cfg));

		/* Clear chanspec for not Associated STA and AP down cases as well */
		if (!MODESW_BSS_ACTIVE(wlc, bsscfg))
			continue;

		/* Skip the bsscfg that is being sent */
		if (bsscfg == cfg)
			continue;

		if (bsscfg->current_bss->chanspec == chanspec_80)
			cspec80 = TRUE;

		if (bsscfg->current_bss->chanspec == chanspec_40)
			cspec40 = TRUE;

		if (bsscfg->current_bss->chanspec == chanspec_20)
			cspec20 = TRUE;
	}

	WL_MODE_SWITCH(("%s: cspec80 [%d] cspec40[%d] cspec20[%d]\n",
		__FUNCTION__, cspec80, cspec40, cspec20));

	/* Clear all the PHY contexts  */
	if (!cspec80)
		wlc_phy_destroy_chanctx(wlc->pi, chanspec_80);

	if (!cspec40)
		wlc_phy_destroy_chanctx(wlc->pi, chanspec_40);

	if (!cspec20)
		wlc_phy_destroy_chanctx(wlc->pi, chanspec_20);
}

/* Function to Measure Time taken for each step for Actual Downgrade,
*  Silent Upgrade , Silent downgrade
*/
void
wlc_modesw_time_measure(wlc_modesw_info_t *modesw_info, uint32 ctrl_flags, uint32 event)
{
#ifdef WL_MODESW_TIMECAL
	uint32 value;
	modesw_time_calc_t *pmodtime = modesw_info->modesw_time;

	/* Get the current Time value */
	wlc_read_tsf(modesw_info->wlc, &value, NULL);

	WL_MODE_SWITCH((" wlc_modesw_time_measure ctrl_flags [0x%x] event[%d] ",
		ctrl_flags, event));

	if (!ctrl_flags) /* Normal Downgrade case */
	{
		switch (event)
		{
			case MODESW_TM_START:
				INCR_CHECK(modesw_info->ActDNCnt, TIME_CALC_LIMT);
				SEQ_INCR(pmodtime[modesw_info->ActDNCnt - 1].ActDN_SeqCnt);
				pmodtime[modesw_info->ActDNCnt - 1].DN_start_time = value;
				break;
			case MODESW_TM_PM1:
				pmodtime[modesw_info->ActDNCnt - 1].DN_ActionFrame_time
					= value;
				break;
			case MODESW_TM_PHY_COMPLETE:
				pmodtime[modesw_info->ActDNCnt - 1].DN_PHY_BW_UPDTime
					= value;
				break;
			case MODESW_TM_PM0:
				pmodtime[modesw_info->ActDNCnt - 1].DN_CompTime
					= value;
				break;
			default:
				break;
		}
	}
	/* Pseudo upgrade case */
	else if (CTRL_FLAGS_HAS(ctrl_flags, MODESW_CTRL_UP_SILENT_UPGRADE))
	{
		switch (event)
		{
			case MODESW_TM_START:
				INCR_CHECK(modesw_info->PHY_UPCnt, TIME_CALC_LIMT);
				SEQ_INCR(pmodtime[modesw_info->PHY_UPCnt-1].PHY_UP_SeqCnt);
				pmodtime[modesw_info->PHY_UPCnt - 1].PHY_UP_start_time
					= value;
				break;
			case MODESW_TM_PM1:
				pmodtime[modesw_info->PHY_UPCnt - 1].PHY_UP_PM1_time
					= value;
				break;
			case MODESW_TM_PHY_COMPLETE:
				pmodtime[modesw_info->PHY_UPCnt - 1].PHY_UP_BW_UPDTime
					= value;
				break;
			case MODESW_TM_PM0:
				pmodtime[modesw_info->PHY_UPCnt - 1].PHY_UP_CompTime
					= value;
				break;
			default:
				break;
		}
	}
	/* Silent Downgrade case */
	else if (CTRL_FLAGS_HAS(ctrl_flags, MODESW_CTRL_DN_SILENT_DNGRADE))
	{
		switch (event)
		{
			case MODESW_TM_START:
				INCR_CHECK(modesw_info->PHY_DNCnt, TIME_CALC_LIMT);
				SEQ_INCR(pmodtime[modesw_info->PHY_DNCnt-1].PHY_DN_SeqCnt);
				pmodtime[modesw_info->PHY_DNCnt - 1].PHY_DN_start_time
					= value;
				break;
			case MODESW_TM_PM1:
				pmodtime[modesw_info->PHY_DNCnt - 1].PHY_DN_PM1_time
					= value;
				break;
			case MODESW_TM_PHY_COMPLETE:
				pmodtime[modesw_info->PHY_DNCnt - 1].PHY_DN_BW_UPDTime
					= value;
				break;
			case MODESW_TM_PM0:
				pmodtime[modesw_info->PHY_DNCnt - 1].PHY_DN_CompTime
					= value;
				break;
			default:
				break;
		}
	}
	/* Normal upgrade case */
	else if (CTRL_FLAGS_HAS(ctrl_flags, MODESW_CTRL_UP_ACTION_FRAMES_ONLY))
	{
		switch (event)
		{
			case MODESW_TM_START:
				INCR_CHECK(modesw_info->ActFRM_Cnt, TIME_CALC_LIMT);
				SEQ_INCR(pmodtime[modesw_info->ActFRM_Cnt-1].ACTFrame_SeqCnt);
				pmodtime[modesw_info->ActFRM_Cnt - 1].ACTFrame_start
					= value;
				break;
			case MODESW_TM_ACTIONFRAME_COMPLETE:
				pmodtime[modesw_info->ActFRM_Cnt - 1].ACTFrame_complete
					= value;
				break;
			default:
				break;
		}
	}
#endif /* WL_MODESW_TIMECAL */
}

void wlc_modesw_clear_context(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
#ifdef STA
		bool PM = cfg->pm->PM;
#endif
	wlc = modesw_info->wlc;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);

	if (!pmodesw_bsscfg)
		return;

	if (pmodesw_bsscfg->state == MSW_NOT_PENDING) {
		return;
	}
	if (BSSCFG_STA(cfg)) {
#ifdef STA
		if (!BSSCFG_AP(cfg)) {
			cfg->pm->PM = 0;
			wlc_set_pm_mode(wlc, PM, cfg);
		}
#endif
		wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_VHT, FALSE);
	}

	if (wlc->block_datafifo & DATA_BLOCK_TXCHAIN) {
		WLC_BLOCK_DATAFIFO_CLEAR(wlc, DATA_BLOCK_TXCHAIN);
	}
	pmodesw_bsscfg->start_time = 0;
	pmodesw_bsscfg->state = MSW_NOT_PENDING;
	return;
}

static void wlc_modesw_ctrl_hdl_all_cfgs(wlc_modesw_info_t *modesw_info,
		wlc_bsscfg_t *cfg, enum notif_events signal)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);

	if (pmodesw_bsscfg == NULL)
		return;

	if (CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
			MODESW_CTRL_HANDLE_ALL_CFGS)) {
		/* This flag signifies that a per bsscfg
		* callback is needed. eg:for Dynamic
		* BWSW we need a callback for every
		* bsscfg to indicate that PHY UP is
		* done.
		*/
		wlc_modesw_notif_cb_notif(modesw_info, cfg, BCME_OK,
			cfg->oper_mode, pmodesw_bsscfg->oper_mode_old, signal);
	}
}
bool
wlc_modesw_pending_check(wlc_modesw_info_t *modesw_info)
{
	wlc_info_t *wlc = modesw_info->wlc;
	uint8 idx;
	modesw_bsscfg_cubby_t *ipmode_cfg;
	wlc_bsscfg_t* icfg;

#if defined(WLMCHAN) && defined(WLRSDB)
	if (MCHAN_ENAB(wlc->pub) && RSDB_ENAB(wlc->pub)) {
		if (wlc_mchan_get_clone_pending(wlc->mchan))
			return FALSE;
	}
#endif

	FOREACH_AS_BSS(wlc, idx, icfg) {
		ipmode_cfg = MODESW_BSSCFG_CUBBY(modesw_info, icfg);
		/* FAILED for someone else ?? */
		if (ipmode_cfg->state == MSW_FAILED) {
			WL_MODE_SWITCH(("wl%d: Opmode change failed for cfg = %d..\n",
				WLCWLUNIT(wlc), icfg->_idx));
		}
		/* Pending for someone else ?? */
		else if ((ipmode_cfg->state != MSW_NOT_PENDING)) {
			WL_MODE_SWITCH(("wl%d: Opmode change pending for cfg = %d..\n",
				WLCWLUNIT(wlc), icfg->_idx));
			return FALSE;
		}
	}
	return TRUE;
}

bool
wlc_modesw_in_progress(wlc_modesw_info_t *modesw_info)
{
	wlc_info_t *wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	wlc_bsscfg_t *icfg;
	int idx;
	uint8 msw_pend = FALSE;
	if (!modesw_info)
		return msw_pend;
	wlc = modesw_info->wlc;
	for (idx = 0; idx < WLC_MAXBSSCFG; idx++) {
		icfg = wlc->bsscfg[idx];
		if (!icfg) {
			continue;
		}
		pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, icfg);
		if (pmodesw_bsscfg && pmodesw_bsscfg->state != MSW_NOT_PENDING &&
			pmodesw_bsscfg->state != MSW_FAILED)
			msw_pend = TRUE;
	}
	return msw_pend;
}

#ifdef WLMCHAN
bool
wlc_modesw_mchan_hw_switch_complete(wlc_info_t * wlc, wlc_bsscfg_t * bsscfg)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);
	if (pmodesw_bsscfg && ((pmodesw_bsscfg->state == MSW_NOT_PENDING) ||
		(pmodesw_bsscfg->state == MSW_FAILED)))
		return TRUE;
	else
		return FALSE;
}

bool
wlc_modesw_mchan_hw_switch_pending(wlc_info_t * wlc, wlc_bsscfg_t * bsscfg,
	bool hw_only)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	bool ret = FALSE;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);
	if (pmodesw_bsscfg)
	{
		if (!hw_only) {
			if ((pmodesw_bsscfg->state == MSW_DN_PERFORM) ||
				(pmodesw_bsscfg->state == MSW_UP_PERFORM) ||
				(pmodesw_bsscfg->state == MSW_DOWNGRADE_PENDING))
				ret = TRUE;
		} else {
			if ((pmodesw_bsscfg->state == MSW_DN_PERFORM) ||
				(pmodesw_bsscfg->state == MSW_UP_PERFORM))
				ret = TRUE;
		}
	}
	return ret;
}

void
wlc_modesw_mchan_switch_process(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t * bsscfg)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	wlc_info_t * wlc;
	wlc = modesw_info->wlc;
	if (!bsscfg) {
		return;
	}
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);
	if (!pmodesw_bsscfg) {
		return;
	}

	if (BSSCFG_STA(bsscfg)) {
		switch (pmodesw_bsscfg->state)
		{
			case MSW_DOWNGRADE_PENDING:
				WL_MODE_SWITCH(("modesw:DN:sendingAction cfg-%d\n", bsscfg->_idx));
				/* send action frame */
				wlc_modesw_send_action_frame_request(wlc->modesw, bsscfg,
					&bsscfg->BSSID, pmodesw_bsscfg->oper_mode_new);
			break;
			case MSW_DN_PERFORM:
				wlc_modesw_perform_upgrade_downgrade(modesw_info, bsscfg);
				bsscfg->oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
					bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);
				wlc_modesw_oper_mode_complete(modesw_info, bsscfg,
				BCME_OK, MODESW_DN_STA_COMPLETE);
			break;
			case MSW_UP_PERFORM:
				wlc_modesw_perform_upgrade_downgrade(modesw_info, bsscfg);
				bsscfg->oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
					bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);
				wlc_modesw_send_action_frame_request(modesw_info, bsscfg,
					&bsscfg->BSSID, pmodesw_bsscfg->oper_mode_new);
			break;
		}
	}
	else {
		switch (pmodesw_bsscfg->state)
		{
			case MSW_DN_PERFORM:
				if (pmodesw_bsscfg->action_sendout_counter == 0) {
					WL_MODE_SWITCH(("All acks received. starting downgrade\n"));
					wlc_modesw_perform_upgrade_downgrade(wlc->modesw, bsscfg);
					wlc_modesw_change_state(wlc->modesw, bsscfg,
						MSW_NOT_PENDING);
					WL_MODE_SWITCH(("\n Send Sig = MODESW_DN_AP_COMPLETE\n"));
					wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg,
						BCME_OK, MODESW_DN_AP_COMPLETE);
				}
			break;
			case MSW_UP_PERFORM:
				wlc_modesw_perform_upgrade_downgrade(wlc->modesw, bsscfg);
				/* Update the oper_mode of the cfg */
				bsscfg->oper_mode = wlc_modesw_derive_opermode(modesw_info,
					bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);

				if (!CTRL_FLAGS_HAS(pmodesw_bsscfg->ctrl_flags,
					(MODESW_CTRL_DN_SILENT_DNGRADE |
					MODESW_CTRL_UP_SILENT_UPGRADE))) {
					wlc_modesw_ap_upd_bcn_act(wlc->modesw, bsscfg,
						pmodesw_bsscfg->state);
				}
				wlc_modesw_oper_mode_complete(wlc->modesw, bsscfg, BCME_OK,
					MODESW_UP_AP_COMPLETE);
			break;
		}
	}
}
#endif /* WLMCHAN */

static void
wlc_modesw_set_pmstate(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t* bsscfg, bool state)
{
	modesw_bsscfg_cubby_t *pmodesw_bsscfg = NULL;
	wlc_info_t * wlc;
	wlc = modesw_info->wlc;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(wlc->modesw, bsscfg);

	/* Update PM mode accordingly to reflect new PM state */
	if (state) {
		mboolset(bsscfg->pm->PMblocked, WLC_PM_BLOCK_MODESW);
		if (bsscfg->pm->PMenabled != state) {
			/* Set the PM state if we are not already
			* in the same state.
			*/
			wlc_set_pmstate(bsscfg, state);
		} else {
			/* If we are already in PM1 mode then both the above calls
			* are skipped. So fake the PM ack reception for moving the
			* state machine forward.
			*/
			wlc_modesw_pm_pending_complete(modesw_info, bsscfg);
		}
	}
	else {
		mboolclr(bsscfg->pm->PMblocked, WLC_PM_BLOCK_MODESW);
		/* Restore PM State only if we changed it during mode switch */
		if (pmodesw_bsscfg->orig_pmstate != bsscfg->pm->PMenabled) {
			wlc_set_pmstate(bsscfg, pmodesw_bsscfg->orig_pmstate);
		} else {
		/* If we did not change PM mode and PM state then both the above calls
		* are skipped. So fake the PM ack reception for moving the
		* state machine forward.
		*/
			wlc_modesw_pm_pending_complete(modesw_info, bsscfg);
		}
	}
	return;
}

/* To keep track of STAs which get associated or disassociated */
static void
wlc_modesw_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data)
{
	wlc_modesw_info_t *modesw_info = (wlc_modesw_info_t*)ctx;
	wlc_bsscfg_t *bsscfg;
	struct scb *scb;
	uint8 oldstate = notif_data->oldstate;

	scb = notif_data->scb;
	bsscfg = scb->bsscfg;

	/* Check if the state transited SCB is internal.
	 * In that case we have to do nothing, just return back
	 */
	if (SCB_INTERNAL(scb))
		return;

	/* Check to see if we are a Soft-AP/GO and the state transited SCB
	 * is a non BRCM as well as a legacy STA
	 */
	if (BSSCFG_STA(bsscfg)) {
		/* Decrement the count if state transition is from
		 * associated -> unassociated
		 */
		if (!BSSCFG_IS_RSDB_CLONE(bsscfg) &&
			(((oldstate & ASSOCIATED) && !SCB_ASSOCIATED(scb)) ||
			((oldstate & AUTHENTICATED) && !SCB_AUTHENTICATED(scb)))) {
			WL_MODE_SWITCH(("wl%d: cfg %d Deauth Done. Clear MODESW Context"
				"Clone = %d\n", WLCWLUNIT(bsscfg->wlc), bsscfg->_idx,
				BSSCFG_IS_RSDB_CLONE(bsscfg)));
			wlc_modesw_restore_defaults(modesw_info, bsscfg);
		}
	}
}

#ifdef WLRSDB
/* This function tries to move all the associated cfg's
* from Core1 to Core 0 in case WLC0 is not associated
* and WLC1 is associated in RSDB AUTO operation.
*/
void
wlc_modesw_move_cfgs_for_mimo(wlc_modesw_info_t *modesw_info)
{
	wlc_info_t *wlc, *wlc0, *wlc1;
	wlc_bsscfg_t *icfg;
	uint8 idx;
	int ret;
	wlc = modesw_info->wlc;
	wlc0 =  wlc->cmn->wlc[0];
	wlc1 =  wlc->cmn->wlc[1];

	if (!WLC_RSDB_IS_AUTO_MODE(wlc) ||
		WLC_RSDB_SINGLE_MAC_MODE(WLC_RSDB_CURR_MODE(wlc)))
		return;

	if (!wlc0->pub->associated &&
		wlc1->pub->associated) {
		WL_MODE_SWITCH(("wl%d: Try to move any associated cfg from Core1 "
			"to Core 0.\n", WLCWLUNIT(wlc)));
		/* find the associated cfg */
		FOREACH_AS_BSS(wlc1, idx, icfg) {
			if (WLC_BSS_CONNECTED(icfg)) {
				/* move to wlc 0 */
#ifdef WL_NAN
				if (NAN_ENAB(wlc1->pub) && BSSCFG_NAN(icfg)) {
					wlc_nan_bsscfg_move(wlc1, wlc0, icfg, &ret);
				} else
#endif
				{
					wlc_rsdb_bsscfg_clone(wlc1, wlc0, icfg, &ret);
				}
			}
		}
	}
}
#endif /* WLRSDB */

/* Recover the mode switch module whenever pkt response
* is delayed.
*/
static void
wlc_modesw_process_recovery(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc;
	modesw_bsscfg_cubby_t *pmodesw_bsscfg;
	uint32 current_time = 0;

	if (!cfg || !modesw_info)
		return;

	wlc =  modesw_info->wlc;
	pmodesw_bsscfg = MODESW_BSSCFG_CUBBY(modesw_info, cfg);

	/* Make sure that we have oper mode change pending for this cfg.
	* It may happen that we clear the oper mode context for this cfg.
	*/
	if (!pmodesw_bsscfg || (pmodesw_bsscfg->state == MSW_NOT_PENDING) ||
		(pmodesw_bsscfg->state == MSW_FAILED) || (!pmodesw_bsscfg->start_time)) {
		return;
	}

	current_time = OSL_SYSUPTIME();

	if ((current_time - pmodesw_bsscfg->start_time) > MODESW_RECOVERY_THRESHOLD) {
		WL_ERROR(("wl%d: cfg %d Needs Recovery state = %d time = %d msec\n", WLCWLUNIT(wlc),
			cfg->_idx, pmodesw_bsscfg->state,
			(current_time - pmodesw_bsscfg->start_time)));

		switch (pmodesw_bsscfg->state)
		{
			case MSW_DN_AF_WAIT_ACK:
			case MSW_UP_AF_WAIT_ACK:
				wlc_modesw_update_PMstate(wlc, cfg);
			break;
			case MSW_DN_PM0_WAIT_ACK:
			case MSW_DN_PM1_WAIT_ACK:
			case MSW_UP_PM0_WAIT_ACK:
			case MSW_UP_PM1_WAIT_ACK:
				wlc_modesw_pm_pending_complete(modesw_info, cfg);
			break;
			case MSW_WAIT_DRAIN:
				wlc_modesw_bsscfg_complete_downgrade(wlc->modesw);
			break;
		}
		/* Take new timestamp for recovery. */
		pmodesw_bsscfg->start_time = OSL_SYSUPTIME();
	}
	return;
}

static void
wlc_modesw_watchdog(void *context)
{
	wlc_info_t *wlc;
	uint8 idx;
	wlc_bsscfg_t *icfg;
	wlc_modesw_info_t *modesw_info = (wlc_modesw_info_t *)context;
	wlc = modesw_info->wlc;

	FOREACH_AS_BSS(wlc, idx, icfg) {
		wlc_modesw_process_recovery(modesw_info, icfg);
	}
	return;
}
