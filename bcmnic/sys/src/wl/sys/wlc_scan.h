/*
 * SCAN Module Public Interface
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
 * $Id: wlc_scan.h 609956 2016-01-05 23:20:04Z $
 */


#ifndef _WLC_SCAN_H_
#define _WLC_SCAN_H_

/* scan state bits */
#define SCAN_STATE_SUPPRESS	(1 << 0)
#define SCAN_STATE_SAVE_PRB	(1 << 1)
#define SCAN_STATE_PASSIVE	(1 << 2)
#define SCAN_STATE_WSUSPEND	(1 << 3)
#define SCAN_STATE_RADAR_CLEAR	(1 << 4)
#define SCAN_STATE_PSPEND	(1 << 5)
#define SCAN_STATE_READY	(1 << 7)
#define SCAN_STATE_INCLUDE_CACHE	(1 << 8)
#define SCAN_STATE_PROHIBIT	(1 << 9)
#define SCAN_STATE_DLY_WSUSPEND	(1 << 10)
#define SCAN_STATE_IN_TMR_CB	(1 << 11)
#define SCAN_STATE_OFFCHAN	(1 << 12)
#define SCAN_STATE_TERMINATE	(1 << 13)
#define SCAN_STATE_AWDL_AW	(1 << 14)
#define SCAN_STATE_HOME_TIME_SPENT	(1 << 15)

#define SCAN_FLAG_SWITCH_CHAN	(1 << 0)

#define WLC_SCAN_PASSIVE_TIME	110	/* ms to listen on a channel for beacons for passivescan */
#define WLC_SCAN_AWAY_LIMIT	100	/* max time to be away from home channel before return */
#define WLC_SCAN_PS_PREP_TIME	10	/* average time to announce PS mode */
#define WLC_SCAN_PS_OVERHEAD	50	/* overhead time to allow for one long PS announce delay */

#define WLC_SCAN_NSSID_PREALLOC	5	/* Number of preallocated SSID slots */

struct wlc_scan_info {
	void		*scan_priv;		/* pointer to scan private struct */
	struct wlc_scan_cmn_info *wlc_scan_cmn;
	uint16		state;			/* scan state bits */
	bool		in_progress;		/* scan in progress */
	struct ether_addr bssid;
	/* WLSCANCACHE */
	bool		_scancache;		/* scan cache enable */
	uint32		flag;			/* scan flag supplement of state bits */
	bool		iscan_cont;		/* true if iscan continuing pass */
};

struct wlc_scan_cmn_info {
	/* Add Scan Function Call Interface */
	uint8 		usage;			/* scan engine usage */
	int		bss_type;		/* Scan for Infra, IBSS, or Any */
	bool		is_hotspot_scan;	/* hotspot scan */

	/* STA */
	uint32		scan_start_time;	/* for scan time accumulation... */
	uint32		scan_stop_time;		/* ...here so callbacks can see it */
};
#define SCAN_IN_PROGRESS(scan_info)	((scan_info) && (scan_info)->in_progress)

#ifdef WLRSDB
#define ANY_SCAN_IN_PROGRESS(scaninfo) wlc_scan_anyscan_in_progress(scaninfo)
extern int wlc_scan_anyscan_in_progress(wlc_scan_info_t *scan);
#else
#define ANY_SCAN_IN_PROGRESS(scaninfo) SCAN_IN_PROGRESS(scaninfo)
#endif

/* scan engine usage */
#define SCAN_ENGINE_USAGE_NORM	0	/* scan, assoc, roam, etc. */
#define SCAN_ENGINE_USAGE_ESCAN	1	/* escan */
#define SCAN_ENGINE_USAGE_RM	3	/* RM */
#define SCAN_ENGINE_USAGE_EXCRX	4	/* all other non-scan excursions */

#define NORM_IN_PROGRESS(scan)		((scan)->wlc_scan_cmn->usage == SCAN_ENGINE_USAGE_NORM)
#define ESCAN_IN_PROGRESS(scan)		((scan)->wlc_scan_cmn->usage == SCAN_ENGINE_USAGE_ESCAN)
#define LPRIO_EXCRX_IN_PROGRESS(scan)	((scan)->wlc_scan_cmn->usage == SCAN_ENGINE_USAGE_EXCRX)

#ifdef WLSCANCACHE
	#if defined(WL_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define SCANCACHE_ENAB(scan_info)	((scan_info)->_scancache)
	#elif defined(WLSCANCACHE_DISABLED)
		#define SCANCACHE_ENAB(pub)	(0)
	#else
		#define SCANCACHE_ENAB(scan)	((scan)->_scancache)
	#endif
#else
	#define SCANCACHE_ENAB(pub)	(0)
#endif /* WLSCANCACHE */

extern wlc_scan_info_t *wlc_scan_attach(wlc_info_t *wlc, void *wl, osl_t *osh, uint);
extern void wlc_scan_detach(wlc_scan_info_t *scan_ptr);
extern int wlc_scan_down(void *hdl);

extern bool wlc_scan_in_scan_chanspec_list(wlc_scan_info_t *wlc_scan_info, chanspec_t chanspec);

/* scan completion callback */
typedef void (*scancb_fn_t)(void *wlc, int status, wlc_bsscfg_t *cfg);
/* scan action callback */
typedef void (*actcb_fn_t)(wlc_info_t *wlc, void *arg, uint *dwell);

extern int wlc_scan(
	wlc_scan_info_t *scan_ptr,
	int bss_type,
	const struct ether_addr* bssid,
	int nssid,
	wlc_ssid_t *ssid,
	int scan_type,
	int nprobes,
	int active_time,
	int passive_time,
	int home_time,
	const chanspec_t* chanspec_list,
	int channel_num,
	chanspec_t chanspec_start,
	bool save_prb,
	scancb_fn_t fn,
	void* arg,
	int away_limit_override,
	bool extd_scan,
	bool suppress_ssid,
	bool include_cache,
	uint scan_flags,
	wlc_bsscfg_t *cfg,
	uint8 usage,
	actcb_fn_t act_cb,
	void *act_arg,
	struct ether_addr *sa_override
);

extern void wlc_scan_abort(wlc_scan_info_t *wlc_scan_info, int status);
extern void wlc_scan_abort_ex(wlc_scan_info_t *wlc_scan_info, wlc_bsscfg_t *cfg, int status);
extern void wlc_scan_terminate(wlc_scan_info_t *wlc_scan_info, int status);
extern void wlc_scan_radar_clear(wlc_scan_info_t *wlc_scan_info);
extern void wlc_scan_default_channels(wlc_scan_info_t *wlc_scan_info, chanspec_t chanspec_start,
	int band, chanspec_t *chanspec_list, int *channel_count);

extern bool wlc_scan_inprog(wlc_info_t *wlc_info);
extern void wlc_scan_fifo_suspend_complete(wlc_scan_info_t *wlc_scan_info);
extern void wlc_scan_pm_pending_complete(wlc_scan_info_t *wlc_scan_info);
extern chanspec_t wlc_scan_get_current_chanspec(wlc_scan_info_t *wlc_scan_info);
extern int wlc_scan_ioctl(wlc_scan_info_t *wlc_scan_info, int cmd, void *arg, int len,
	struct wlc_if *wlcif);
extern bool wlc_scan_ssid_match(wlc_scan_info_t *wlc_scan_info, bcm_tlv_t *ssid_ie, bool filter);
extern int wlc_scan_chnum(wlc_scan_info_t *wlc_scan_info);

#ifdef WLSCANCACHE
extern void wlc_scan_get_cache(wlc_scan_info_t *scan_info,
                   const struct ether_addr *BSSID, int nssid, const wlc_ssid_t *SSID,
                   int BSS_type, const chanspec_t *chanspec_list, uint chanspec_num,
                   wlc_bss_list_t *bss_list);
#else
#define wlc_scan_get_cache(si, BSSID, nssid, SSID, BSS_type, c_list, c_num, bss_list)	\
	(void)((bss_list)->count = 0)
#endif

#ifdef CNTRY_DEFAULT
extern int wlc_scan_prohibited_channels_get(wlc_scan_info_t *scan, chanspec_t *chanspec_list,
	int channel_max);
#else
#define wlc_scan_prohibited_channels_get(scan_info, chanspec_list, channel_max) (0)
#endif /* CNTRY_DEFAULT */

extern wlc_bsscfg_t *wlc_scan_bsscfg(wlc_scan_info_t *scan_info);
extern void wlc_scan_timer_update(wlc_scan_info_t *wlc_scan_info, uint32 ms);

#if defined(STA)
/* pwrstats retrieval function */
extern int wlc_pwrstats_get_scan(wlc_scan_info_t *scan, uint8 *destptr, int destlen);
extern uint32 wlc_get_curr_scan_time(wlc_info_t *wlc);
extern uint32 wlc_curr_roam_scan_time(wlc_info_t *wlc);
#endif /* STA */

extern wlc_bsscfg_t *wlc_scanmac_get_bsscfg(wlc_scan_info_t *scan, int macreq, wlc_bsscfg_t *cfg);
struct ether_addr *wlc_scanmac_get_mac(wlc_scan_info_t *scan, int macreq, wlc_bsscfg_t *bsscfg);
extern int wlc_scanmac_update(wlc_scan_info_t *scan);
#endif /* _WLC_SCAN_H_ */
