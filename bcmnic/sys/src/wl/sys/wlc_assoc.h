/*
 * Association/Roam related routines
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
 * $Id: wlc_assoc.h 612151 2016-01-13 04:39:48Z $
 */

#ifndef __wlc_assoc_h__
#define __wlc_assoc_h__

#ifdef STA

extern int wlc_join(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 *SSID, int len,
	wl_join_scan_params_t *scan_params,
	wl_join_assoc_params_t *assoc_params, int assoc_params_len);
extern void wlc_join_recreate(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

extern void wlc_join_complete(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, uint8 *plcp,
	struct dot11_bcn_prb *bcn, int bcn_len);
extern int wlc_join_recreate_complete(wlc_bsscfg_t *cfg, wlc_d11rxhdr_t *wrxh, uint8 *plcp,
	struct dot11_bcn_prb *bcn, int bcn_len);

void wlc_join_pref_reset_cond(wlc_bsscfg_t *cfg);

extern int wlc_reassoc(wlc_bsscfg_t *cfg, wl_reassoc_params_t *reassoc_params);

extern void wlc_roam_complete(wlc_bsscfg_t *cfg, uint status,
                              struct ether_addr *addr, uint bss_type);
extern int wlc_roam_scan(wlc_bsscfg_t *cfg, uint reason, chanspec_t *list, uint32 channum);
extern void wlc_roamscan_start(wlc_bsscfg_t *cfg, uint roam_reason);
extern void wlc_assoc_roam(wlc_bsscfg_t *cfg);
extern void wlc_txrate_roam(wlc_info_t *wlc, struct scb *scb, tx_status_t *txs, bool pkt_sent,
	bool pkt_max_retries);
extern void wlc_build_roam_cache(wlc_bsscfg_t *cfg, wlc_bss_list_t *candidates);
extern void wlc_roam_motion_detect(wlc_bsscfg_t *cfg);
extern void wlc_roam_bcns_lost(wlc_bsscfg_t *cfg);
extern int wlc_roam_trigger_logical_dbm(wlc_info_t *wlc, wlcband_t *band, int val);
extern bool wlc_roam_scan_islazy(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool roam_scan_isactive);
extern bool wlc_lazy_roam_scan_suspend(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern bool wlc_lazy_roam_scan_sync_dtim(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern void wlc_roam_prof_update(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool reset);
extern void wlc_roam_prof_update_default(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern void wlc_roam_handle_join(wlc_bsscfg_t *cfg);
extern void wlc_roam_handle_beacon_loss(wlc_bsscfg_t *cfg);
extern void wlc_roam_handle_missed_beacons(wlc_bsscfg_t *cfg, uint32 missed_beacons);

extern int wlc_disassociate_client(wlc_bsscfg_t *cfg, bool send_disassociate);

extern int wlc_assoc_abort(wlc_bsscfg_t *cfg);
#if defined(WL_ASSOC_MGR)
/*
* Continue an association paused via assoc_mgr module.
* cfg - bsscfg the assoc was paused on
* addr - address of the scb that the assoc paused on
* Returns error if assoc was not paused or unable to restart
*/
extern int wlc_assoc_continue(wlc_bsscfg_t *cfg, struct ether_addr* addr);
#endif /* defined(WL_ASSOC_MGR) */

extern void wlc_assoc_change_state(wlc_bsscfg_t *cfg, uint newstate);
extern void wlc_authresp_client(wlc_bsscfg_t *cfg,
	struct dot11_management_header *hdr, uint8 *body, uint body_len, bool short_preamble);
extern void wlc_assocresp_client(wlc_bsscfg_t *cfg, struct scb *scb,
	struct dot11_management_header *hdr, uint8 *body, uint body_len);
extern void wlc_auth_tx_complete(wlc_info_t *wlc, uint txstatus, void *arg);
extern void wlc_auth_complete(wlc_bsscfg_t *cfg, uint status, struct ether_addr *addr,
	uint auth_status, uint auth_type);
extern void wlc_assocreq_complete(wlc_info_t *wlc, uint txstatus, void *arg);

extern void wlc_sta_assoc_upd(wlc_bsscfg_t *cfg, bool state);

#ifdef ROBUST_DISASSOC_TX
extern void wlc_disassoc_tx(wlc_bsscfg_t *cfg, bool send_disassociate);
#endif /* ROBUST_DISASSOC_TX */

void wlc_handle_ap_lost(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

void wlc_link_monitor_watchdog(wlc_info_t *wlc);

#endif /* STA */

extern int wlc_mac_request_entry(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int req);

extern void wlc_roam_defaults(wlc_info_t *wlc, wlcband_t *band, int *roam_trigger, uint *rm_delta);

extern void wlc_disassoc_complete(wlc_bsscfg_t *cfg, uint status, struct ether_addr *addr,
	uint disassoc_reason, uint bss_type);
extern void wlc_deauth_complete(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint status,
	const struct ether_addr *addr, uint deauth_reason, uint bss_type);

typedef struct wlc_deauth_send_cbargs {
	struct ether_addr	ea;
	int8			_idx;
	void                    *pkt;
} wlc_deauth_send_cbargs_t;

extern void wlc_deauth_sendcomplete(wlc_info_t *wlc, uint txstatus, void *arg);
extern void wlc_disassoc_ind_complete(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint status,
	struct ether_addr *addr, uint disassoc_reason, uint bss_type,
	uint8 *body, int body_len);
extern void wlc_deauth_ind_complete(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint status,
	struct ether_addr *addr, uint deauth_reason, uint bss_type,
	uint8 *body, int body_len);
extern void wlc_assoc_update_dtim_count(wlc_bsscfg_t *cfg, uint16 dtim_count);
#ifdef WLPFN
void wl_notify_pfn(wlc_info_t *wlc);
#endif /* WLPFN */

extern void wlc_assoc_bcn_mon_off(wlc_bsscfg_t *cfg, bool off, uint user);

extern void wlc_join_adopt_ibss_params(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern int wlc_join_start_ibss(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int8 bsstype);
extern int wlc_join_start_prep(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

extern int wlc_assoc_iswpaenab(wlc_bsscfg_t *cfg, bool wpa);

extern void wlc_assoc_init(wlc_bsscfg_t *cfg, uint type);
extern void wlc_adopt_dtim_period(wlc_bsscfg_t *cfg, uint8 dtim_period);

extern uint32 wlc_bss_pref_score(wlc_bsscfg_t *cfg, wlc_bss_info_t *bi, bool band_rssi_boost,
	uint32 *prssi);

extern int wlc_assoc_chanspec_sanitize(wlc_info_t *wlc, chanspec_list_t *list, int len,
	wlc_bsscfg_t *cfg);

#ifdef WLSCANCACHE
bool wlc_assoc_cache_validate_timestamps(wlc_info_t *wlc, wlc_bss_list_t *bss_list);
#endif

/* ******** WORK-IN-PROGRESS ******** */

/* TODO: make these structure opaque */

/* module info */

/* shared wlc module info */
typedef struct wlc_assoc_cmn {
	wlc_bsscfg_t	*assoc_req[WLC_MAXBSSCFG]; /**< join/roam requests */

	/* association */
	uint		sta_retry_time;		/**< time to retry on initial assoc failure */
	wlc_bss_list_t	*join_targets;
	uint		join_targets_last;	/**< index of last target tried (next: --last) */
} wlc_assoc_cmn_t;

/* per wlc module info */
struct wlc_assoc_info {
	wlc_info_t *wlc;
	wlc_assoc_cmn_t *cmn;
	int cfgh;
};

#define AS_IN_PROGRESS_CFG(wlc)	((wlc)->as->cmn->assoc_req[0])
#define AS_IN_PROGRESS(wlc)	(AS_IN_PROGRESS_CFG(wlc) != NULL)
#define AS_IN_PROGRESS_WLC(wlc)	(AS_IN_PROGRESS(wlc) && \
				 BSS_MATCH_WLC(wlc, AS_IN_PROGRESS_CFG(wlc)))

/** per bsscfg association states */
struct wlc_assoc {
	struct wl_timer *timer;		/**< timer for auth/assoc request timeout */
	uint	type;			/**< roam, association, or recreation */
	uint	state;			/**< current state in assoc process */
	uint	flags;			/**< control flags for assoc */

	bool	preserved;		/**< Whether association was preserved */
	uint	recreate_bi_timeout;	/**< bcn intervals to wait to detect our former AP
					 * when performing an assoc_recreate
					 */
	uint	verify_timeout;		/**< ms to wait to allow an AP to respond to class 3
					 * data if it needs to disassoc/deauth
					 */
	uint8	retry_max;		/**< max. retries */
	uint8	ess_retries;		/**< number of ESS retries */
	uint8	bss_retries;		/**< number of BSS retries */

	uint16	capability;		/**< next (re)association request overrides */
	uint16	listen;
	struct ether_addr bssid;
	uint8	*ie;
	uint	ie_len;

	struct dot11_assoc_req *req;	/**< last (re)association request */
	uint	req_len;
	bool	req_is_reassoc;		/**< is a reassoc */
	struct dot11_assoc_resp *resp;	/**< last (re)association response */
	uint	resp_len;

	bool	rt;			/**< true if sta retry timer active */

	/* ROBUST_DISASSOC_TX */
	uint8	disassoc_tx_retry;		/**< disassoc tx retry number */
	bool block_disassoc_tx;

	struct ether_addr last_upd_bssid;	/**< last BSSID updated to the host */
	uint disassoc_txstatus;			/**< dissassoc packet acknowledgement */
	wlc_msch_req_handle_t	*req_msch_hdl;	/* hdl to msch request */
	uint16	dtim_count;			/* DTIM count */
};

/* per bsscfg roam states */
#define ROAM_CACHELIST_SIZE		4
#define MAX_ROAM_CHANNEL		20
#define WLC_SRS_DEF_ROAM_CHAN_LIMIT	6	/* No. of cached channels for Split Roam Scan */

struct wlc_roam {
	uint	bcn_timeout;		/**< seconds w/o bcns until loss of connection */
	bool	assocroam;		/**< roam to preferred assoc band in oid bcast scan */
	bool	off;			/**< disable roaming */
	uint8	time_since_bcn;		/**< second count since our last beacon from AP */
	uint8	minrate_txfail_cnt;	/**< tx fail count at min rate */
	uint8	minrate_txpass_cnt;	/**< number of consecutive frames at the min rate */
	uint	minrate_txfail_thresh;	/**< min rate tx fail threshold for roam */
	uint	minrate_txpass_thresh;	/**< roamscan threshold for being stuck at min rate */
	uint	txpass_cnt;		/**< turn off roaming if we get a better tx rate */
	uint	txpass_thresh;		/**< turn off roaming after x many packets */
	uint32	tsf_h;			/**< TSF high bits (to detect retrograde TSF) */
	uint32	tsf_l;			/**< TSF low bits (to detect retrograde TSF) */
	uint	scan_block;		/**< roam scan frequency mitigator */
	uint	ratebased_roam_block;	/**< limits mintxrate/txfail roaming frequency */
	uint	partialscan_period;	/**< user-specified roam scan period */
	uint	fullscan_period; 	/**< time between full roamscans */
	uint	reason;			/**< cache so we can report w/WLC_E_ROAM event */
	uint	original_reason;	/**< record the reason for precise reporting on link down */
	bool	active;			/**< RSSI based roam in progress */
	bool	cache_valid;		/**< RSSI roam cache is valid */
	uint	time_since_upd;		/**< How long since our update? */
	uint	fullscan_count;		/**< Count of full rssiroams */
	int	prev_rssi;		/**< Prior RSSI, used to invalidate cache */
	uint	cache_numentries;	/**< # of rssiroam APs in cache */
	bool	thrash_block_active;	/**< Some/all of our APs are unavaiable to reassoc */
	bool	motion;			/**< We are currently moving */
	int	RSSIref;		/**< trigger for motion detection */
	uint16	motion_dur;		/**< How long have we been moving? */
	uint16	motion_timeout;		/**< Time left using modifed values */
	uint8	motion_rssi_delta;	/**< Motion detect RSSI delta */
	bool	roam_scan_piggyback;	/**< Use periodic broadcast scans as roam scans */
	bool	piggyback_enab;		/**< Turn on/off roam scan piggybacking */
	struct {			/**< Roam cache info */
		chanspec_t chanspec;
		struct ether_addr BSSID;
		uint16 time_left_to_next_assoc;
	} cached_ap[ROAM_CACHELIST_SIZE];
	uint8	ap_environment;		/**< Auto-detect the AP density of the environment */
	bool	bcns_lost;		/**< indicate if the STA can see the AP */
	uint8	consec_roam_bcns_lost;	/**< counter to keep track of consecutive calls
					 * to wlc_roam_bcns_lost function
					 */
	uint8	roam_on_wowl; /* trigger roam scan (on bcn loss) for prim cfg from wowl watchdog */
	uint8   nfullscans;     /* Number of full scans before switching to partial scans */

	/* WLRCC: Roam Channel Cache */
	bool    rcc_mode;
	bool    rcc_valid;
	bool	roam_scan_started;
	bool	roam_fullscan;
	uint16	n_rcc_channels;
	chanspec_t *rcc_channels;
	uint16  max_roam_time_thresh;   /* maximum roam threshold time in ms */
	/* Roam hot channel cache */
	chanvec_t roam_chn_cache;	/**< split roam scan channel cache */
	bool	roam_chn_cache_locked;	/**< indicates cache was cfg'd manually */
	uint8	roam_chn_cache_limit;	/**< Max chan to use from cache for split roam scan */

	/* Split roam scan */
	int16	roam_trigger_aggr;	/**< aggressive trigger values set by host for roaming */
	uint16	roam_delta_aggr;	/**< aggressive roam mode - roam delta value */
	bool	multi_ap;		/**< TRUE when there are more than one AP for the SSID */
	uint8	split_roam_phase;	/**< tracks the phases of a split roam scan */
	bool	split_roam_scan;	/**< flag for enabling split roam scans */
	uint8	ci_delta;		/**< roam cache invalidate delta */
	uint	roam_type;		/**< Type of roam (standard, partial, split phase) */

	wl_reassoc_params_t *reassoc_param;
	uint8	reassoc_param_len;
	struct wl_timer *timer;		/**< timer to start roam after uatbtt */
	bool timer_active;		/**< Is roam timer active? */
	uint	prev_bcn_timeout;

	/* OPPORTUNISTIC_ROAM */
	bool	oppr_roam_off;		/**< disable opportunistic roam */

	int8	roam_rssi_boost_thresh;
	int8	roam_rssi_boost_delta;	/**< RSSI boost can be negative */
	int16	roam_prof_idx;		/**< current roaming profile (based upon band/RSSI) */


	uint16  prev_scan_home_time;
	uint32  link_monitor_last_update;	/**< last time roam scan counters decreased */

	uint	bcn_thresh;		/**< bcn intervals w/o bcn --> bcn lost event */
	uint	bcn_interval_cnt;	/**< count of bcn intervals since last bcn */
	bool    roam_bcnloss_off;      /* disable roaming for beacon loss */
};

/* attach/detach interface */
wlc_assoc_info_t *wlc_assoc_attach(wlc_info_t *wlc);
void wlc_assoc_detach(wlc_assoc_info_t *asi);

/* ******** WORK-IN-PROGRESS ******** */

#endif /* __wlc_assoc_h__ */
