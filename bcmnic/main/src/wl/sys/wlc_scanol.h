/*
 * SCAN Module Offload Interface
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_scanol.h 467328 2014-04-03 01:23:40Z $
 */


#ifndef _WLC_SCANOL_H_
#define _WLC_SCANOL_H_

#ifdef SCANOL
#define SCANOL_MAX_CHANNELS		50

typedef struct scanol_status {
	uint32	version;
	uint32	len;
	uint32	scan_flags;
	uint32	scan_cycles;
	uint32	scan_result_cnt;
	uint32	scan_cycle_idle_delay;
} scanol_status_t;

#define SCANOL_STATUS_VERSION		0
#define SCANOL_STATUS_LEN		sizeof(struct scanol_status)
#define SCANOL_MAX_CHANNEL_HINTS	4
#define SCANOL_MAX_PFN_PROFILES		16
typedef struct scanol_pfn {
	wlc_ssid_t ssid;
	uint32	cipher_type;
	uint32	auth_type;
	uint8	channels[SCANOL_MAX_CHANNEL_HINTS];
} scanol_pfn_t;

typedef struct scanol_pfnlist {
	uint32	n_profiles;
	scanol_pfn_t	profile[SCANOL_MAX_PFN_PROFILES];
} scanol_pfnlist_t;

struct scanol_info {
	wlc_hw_info_t	*hw;		/* HW module (private h/w states) */
	watchdog_fn_t	watchdog_fn;
	uint		wd_count;
	bool		wowl_pme_asserted;
	wlc_bss_info_t	*bi;
	bool		prefssid_found;
	int		maxbss;		/* max # of bss info elements in scan list */
	uint32		exptime_cnt;	/* number of expired pkts since start_exptime */
	struct wl_timer *scanoltimer;	/* timer for scan cycle delay */
	bool		scan_enab;
	bool		scan_active;	/* scan cycle to performed */
	bool		ulp;		/* ucode low-power mode */
	bool		manual_scan;	/* force a manual scan */
	chanspec_t	home_chanspec;
	bool		_autocountry;
	bool		associated;	/* true:part of [I]BSS, false: not */
	bool		bandlocked;	/* disable auto multi-band switching */
	bool		PMpending;	/* waiting for tx status with PM indicated set */
	bool		bcnmisc_scan;	/* bcns promisc mode override for scan */
	bool		tx_suspended;	/* data fifos need to remain suspended */
	bool		assoc_in_progress;	/* association in progress */
	uint		nvalid_chanspecs;
	chanspec_t	valid_chanspecs[WL_NUMCHANSPECS];
	chanvec_t	quiet_chanvecs;	/* channels on which we cannot transmit */
	chanvec_t	valid_chanvec_2g;
	chanvec_t	valid_chanvec_5g;
	chanvec_t	notx_chanvecs;
	char autocountry_default[WLC_CNTRY_BUF_SZ];	/* initial country for 802.11d */
	ppr_t		*txpwr;		/* cached ppr for general locale & channel */
	uint		nssid;
	wlc_ssid_t	ssidlist[MAX_SSID_CNT];
	uint		npref_ssid;
	wlc_ssid_t	pref_ssidlist[MAX_SSID_CNT];
	uint		nchannels;
	chanspec_t	chanspecs[SCANOL_MAX_CHANNELS];
	scanol_pfnlist_t pfnlist;
	scanol_params_t params;
	uint32		scan_type;
	uint32		scan_cycle;	/* scan cycle to performed */
	uint32		scan_start_delay;
	int8		curpwr[MAXCHANNEL]; /* txpower of ea channel for offload */
	sar_limit_t	sarlimit;
	int16		rssithrsh2g;
	int16		rssithrsh5g;
	int8		min_txpwr_thresh;
};

#define WLC_BAND_T			wlc_hwband_t
#define IS_SCANOL_ENAB(scan)		((scan)->ol->scan_enab)
#define IS_PM_PENDING(scan)		((scan)->ol->PMpending)
#define IS_ASSOCIATED(scan)		((scan)->ol->associated)
#define IS_BSS_ASSOCIATED(cfg)		((cfg)->associated)
#define IS_IBSS_ALLOWED(scan)		(0)
#define IS_AP_ACTIVE(scan)		(0)
#define IS_EXPTIME_CNT_ZERO(scan)	((scan)->ol->exptime_cnt == 0)
#define IS_N_ENAB(scan)			((scan)->ol->hw->_n_enab)
#define IS_11H_ENAB(scan)		((scan)->ol->hw->_11h)
#define IS_11D_ENAB(scan)		((scan)->ol->hw->_11d)
#define IS_SIM_ENAB(scan)		(0)
#define IS_P2P_ENAB(scan)		(0)
#define IS_MCHAN_ENAB(scan)		(0)
#define IS_CCX_ENAB(scan)		(0)
#define IS_AUTOCOUNTRY_ENAB(scan)	((scan)->ol->_autocountry)
#define IS_EXTSTA_ENAB(scan)		(0)
#define IS_AS_IN_PROGRESS(scan)		((scan) && (scan)->ol->assoc_in_progress)
#define IS_COREREV(scan)		((scan)->ol->hw->corerev)
#define IS_SCAN_BLOCK_DATAFIFO(scan, bit)  (0)
#define SCAN_BCN_PROMISC(scan)		((scan)->ol->bcnmisc_scan)
#define SCAN_RESULT_PTR(scan)		((scan)->scan_pub->scan_results)
#define SCAN_RESULT_MEB(scan, m)	((scan)->scan_pub->scan_results->m)
#define SCAN_FOREACH_STA(scan, idx, cfg) \
	for (idx = 0; (int)idx < 1; idx++) \
		if ((cfg = (scan)->bsscfg) && BSSCFG_STA(cfg))
#define SCAN_FOREACH_AS_STA(scan, idx, cfg) \
	for (idx = 0; cfg = NULL, (int) idx < 1; idx++) \
		if ((cfg = (scan)->bsscfg) && BSSCFG_STA(cfg) && cfg->associated)
#define SCAN_USER(scan, cfg)		((scan)->bsscfg)
#define SCAN_HOME_CHANNEL(scan)		((scan)->ol->home_chanspec)
#define SCAN_ISUP(scan)			((scan)->ol->hw->up)
#define SCAN_STAY_AWAKE(scan)		wlc_scan_stay_wake(scan)
#define SCAN_NBANDS(scan)		NBANDS_HW((scan)->ol->hw)
#define SCAN_BLOCK_DATAFIFO_SET(scan, bit)	do {} while (0) /* sherman need to revisit */
#define SCAN_BLOCK_DATAFIFO_CLR(scan, bit)	do {} while (0) /* sherman need to revisit */
#define SCAN_READ_TSF(scan, a, b)	wlc_bmac_read_tsf((scan)->ol->hw, a, b)
#define SCAN_IS_MBAND_UNLOCKED(scan)	((NBANDS_HW((scan)->ol->hw) > 1) && \
					!(scan)->ol->bandlocked)
#define SCAN_VALID_CHANNEL20_IN_BAND(scan, bu, val) \
	wlc_scan_valid_channel20_in_band((scan), (bu), (val))
#define SCAN_WL_DOWN(scan)		ASSERT(0)
#define SCAN_GET_PI_PTR(scan)		((scan)->ol->hw->band->pi)
#define SCAN_BAND_PI_RADIO_CHANSPEC(scan)	((scan)->ol->hw->chanspec)
#define SCAN_DEVICEREMOVED(scan)	(0)
#define SCAN_TO_MUTE(scan, b, c)	wlc_bmac_mute((scan)->ol->hw, (b), (c))
#define SCAN_GET_CUR_BAND(scan)		((scan)->ol->hw->band)
#define SCAN_GET_PI_BANDUNIT(scan, bu)	((scan)->ol->hw->bandstate[(bu)]->pi)
#define SCAN_OTHERBANDUNIT(scan)	OTHERBANDUNIT((scan)->ol->hw)
#define SCAN_GET_BANDSTATE(scan, bu)	((scan)->ol->hw->bandstate[(bu)])
#define SCAN_CMIPTR(scan)		((void *)NULL)
#define SCAN_IS_MATCH_SSID(scan, ssid1, ssid2, len1, len2) \
	((len1) == (len2) && !bcmp((ssid1), (ssid2), (len1)))
#define SCAN_GET_TSF_TIMERLOW(scan)	(R_REG((scan)->osh, &(scan)->ol->hw->regs->tsf_timerlow))
#define SCAN_MAXBSS(scan)		((scan)->ol->maxbss)
#define SCAN_ISCAN_CHANSPEC_LAST(scan)	((scan)->scan_pub->iscan_chanspec_last)
#define SCAN_SET_WATCHDOG_FN(fnpt)	(scan_info->ol->watchdog_fn = (fnpt))
#define SCAN_RESTORE_BSSCFG(scan, cfg)	((scan)->bsscfg = cfg)
#define SCAN_ISCAN_IN_PROGRESS(hw)	(0)
#define SCAN_CLK(scan)			(scan)->ol->hw->clk

#define wlc_module_register(a, b, c, d, e, f, g, h)	(0)
#define wlc_module_add_ioctl_fn(a, b, c, d, e)		(0)
#define wlc_dump_register(a, b, c, d)	do { } while (0)
#define wlc_module_unregister(a, b, c)	do { } while (0)
#define wlc_module_remove_ioctl_fn(a, b)	do { } while (0)

#ifdef WL_RTDC
#undef WL_RTDC
#endif
#define WL_RTDC(a, b, c, d)		do { } while (0)

#define	OL_SCAN(args)			do { printf args; } while (0)

extern int wlc_scanol_init(scan_info_t *scan, wlc_hw_info_t *hw, osl_t *osh);
extern void wlc_scanol_cleanup(scan_info_t *scan, osl_t *osh);
extern bool wlc_scan_valid_chanspec_db(scan_info_t *scan, chanspec_t chanspec);
extern int wlc_scan_stay_wake(scan_info_t *scan);
extern void wlc_scan_assoc_state_upd(wlc_scan_info_t *wlc_scan_info, bool state);
extern bool wlc_scan_get_chanvec(scan_info_t *scan_info, const char* country_abbrev,
	int bandtype, chanvec_t *chanvec);
extern bool wlc_scan_valid_channel20_in_band(scan_info_t *scan, uint bu, uint val);
extern bool wlc_scan_quiet_chanspec(scan_info_t *scan, chanspec_t chanspec);
extern void wlc_scan_bss_list_free(scan_info_t *scan);
extern void wlc_scan_pm2_sleep_ret_timer_start(wlc_bsscfg_t *cfg);
extern bool wlc_scan_tx_suspended(scan_info_t *scan);
extern void _wlc_scan_tx_suspend(scan_info_t *scan);
extern void _wlc_scan_pm_pending_complete(scan_info_t *scan);
extern void wlc_scan_set_pmstate(wlc_bsscfg_t *cfg, bool state);
extern void wlc_scan_skip_adjtsf(scan_info_t *scan, bool skip, wlc_bsscfg_t *cfg,
	uint32 user, int bands);
extern void wlc_scan_suspend_mac_and_wait(scan_info_t *scan);
extern void wlc_scan_enable_mac(scan_info_t *scan);
extern void wlc_scan_mhf(scan_info_t *scan, uint8 idx, uint16 mask, uint16 val, int bands);
extern void _wlc_scan_sendprobe(scan_info_t *scan, wlc_bsscfg_t *cfg,
	const uint8 ssid[], int ssid_len,
	const struct ether_addr *da, const struct ether_addr *bssid,
	ratespec_t ratespec_override, uint8 *extra_ie, int extra_ie_len);
extern void wlc_scan_set_wake_ctrl(scan_info_t *scan);
extern void wlc_scan_ibss_enable(wlc_bsscfg_t *cfg);
extern void wlc_scan_ibss_disable_all(scan_info_t* scan);
extern void wlc_scan_bss_mac_event(scan_info_t* scan, wlc_bsscfg_t *cfg, uint msg,
	const struct ether_addr* addr, uint result, uint status, uint auth_type,
	void *data, int datalen);
extern void wlc_scan_excursion_start(scan_info_t *scan);
extern void wlc_scan_excursion_end(scan_info_t *scan);
extern void wlc_scan_set_chanspec(scan_info_t *scan, chanspec_t chanspec);
extern void wlc_scan_validate_bcn_phytxctl(scan_info_t *scan, wlc_bsscfg_t *cfg);
extern void wlc_scan_mac_bcn_promisc(scan_info_t *scan);
extern void wlc_scan_ap_mute(scan_info_t *scan, bool mute, wlc_bsscfg_t *cfg, uint32 user);
extern void wlc_scan_tx_resume(scan_info_t *scan);
extern void wlc_scan_send_q(scan_info_t *scan);
extern void wlc_scan_11d_scan_complete(scan_info_t *scan, int status);
extern const char *wlc_scan_11d_get_autocountry_default(scan_info_t *scan);
extern void wlc_scan_radio_mpc_upd(scan_info_t *scan);
extern void wlc_scan_radio_upd(scan_info_t *scan);
extern void wlc_build_roam_cache(wlc_bsscfg_t *cfg, wlc_bss_list_t *candidates);

extern void wlc_scanol_complete(void *arg, int status, wlc_bsscfg_t *cfg);
extern int wlc_scanol_params(scan_info_t *scan, void *params, uint len);
extern int wlc_scanol_params_process(scan_info_t *scan, char *arg, uint arg_len,
	scanol_params_t *olscanparams);
extern void wlc_dngl_ol_scan_send_proc(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len);
extern int wlc_scanol_enable(wlc_hw_info_t *wlc_hw, int state);
extern void wlc_scanol_idle_timeout(void *arg);

extern void wlc_scanol_event_handler(
	wlc_dngl_ol_info_t	*wlc_dngl_ol,
	uint32			event,
	void			*event_data);
#else /* SCANOL */
#define wlc_dngl_ol_scan_send_proc(a, b, c)	do { } while (0)
#endif /* SCANOL */
#endif /* _WLC_SCANOL_H_ */
