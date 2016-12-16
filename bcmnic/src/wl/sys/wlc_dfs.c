/*
 * 802.11h DFS module source file
 * Broadcom 802.11abgn Networking Device Driver
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_dfs.c 668637 2016-11-04 08:47:37Z $
 */

/**
 * @file
 * @brief
 * Related to radar avoidance. Implements CAC (Channel Availability Check) state machine.
 */


#include <wlc_cfg.h>

#ifdef WLDFS

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wl_export.h>
#include <wlc_ap.h>
#include <wlc_scan.h>
#include <wlc_phy_hal.h>
#include <phy_radar_api.h>
#include <wlc_quiet.h>
#include <wlc_csa.h>
#include <wlc_11h.h>
#include <wlc_dfs.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_bmac.h>
#include <wlc_dump.h>
#include <wlc_hw.h>
#include <wlc_iocv.h>
#include <wlc_event_utils.h>
#include <wlc_stf.h>
#include <wlc_hw_priv.h>
#include <wl_dbg.h>
#include <phy_misc_api.h>
#include <phy_calmgr_api.h>
#include <wlc_assoc.h>

#ifdef WLRSDB
#include <wlc_rsdb.h>
#endif /* WLRSDB */

#if defined(WLRSDB) || defined(BGDFS)
#include <wlc_modesw.h>
#endif /* WLRSDB || BGDFS */

/* IOVar table */
/* No ordering is imposed */
enum wlc_dfs_iov {
	IOV_DFS_PREISM = 1,		/* preism cac time */
	IOV_DFS_POSTISM = 2,		/* postism cac time */
	IOV_DFS_STATUS = 3,		/* dfs cac status */
	IOV_DFS_ISM_MONITOR = 4,	/* control the behavior of ISM state */
	IOV_DFS_CHANNEL_FORCED = 5,	/* next dfs channel forced */
	IOV_DFS_AP_MOVE = 6,	/* Move the AP to specified RADAR Channel using second core. */
	IOV_DFS_STATUS_ALL = 7,		/* dfs status of multiple cores / parallel radar scans */
	IOV_DFS_MAX_SAFE_TX = 8,       /* Max safe tx traffic on main core during background scan */
	IOV_DFS_TXBLANK_CHECK_MODE = 9,	/* Control considering tx on main core at end of CAC */
	IOV_LAST
};

static const bcm_iovar_t wlc_dfs_iovars[] = {
	{"dfs_preism", IOV_DFS_PREISM, 0, 0, IOVT_UINT32, 0},
	{"dfs_postism", IOV_DFS_POSTISM, 0, 0, IOVT_UINT32, 0},
	{"dfs_status", IOV_DFS_STATUS, (0), 0, IOVT_BUFFER, 0},
	/* dfs_ism_monitor is required for regulatory DVT */
	{"dfs_ism_monitor", IOV_DFS_ISM_MONITOR, (0), 0, IOVT_UINT32, 0},
	{"dfs_channel_forced", IOV_DFS_CHANNEL_FORCED, (0), 0, IOVT_UINT32, 0},
	{"dfs_ap_move", IOV_DFS_AP_MOVE, (0), 0, IOVT_BUFFER, 0},
	{"dfs_status_all", IOV_DFS_STATUS_ALL, (0), 0, IOVT_BUFFER, 0},
	{NULL, 0, 0, 0, 0, 0}
};

#ifdef BGDFS
#define PHYMODE_BGDFS_ACTIVE(phymode) (((phymode) & PHYMODE_BGDFS) != 0)

/* traffic thresholds used to decide if background DFS CAC results must be discarded */
#define BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_ADJACENT		(1)	/* adjacent channel */
#define BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_NONADJACENT_FCC	(17)	/* non-adjacent in FCC */
#define BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_NONADJACENT_ETSI	(30)	/* non-adjacent in ETSI */
#define BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_WEATHER_ETSI	(5)	/* EU weather channel */

/* bandwidth in MHz */
#define CHAN_BW_80MHZ 80
#define CHAN_BW_40MHZ 40
#define CHAN_BW_20MHZ 20

/* In which cases to consider txblanking (traffic) on main core at end of CAC on scan core */
#define BGDFS_TXBLANK_CHECK_MODE_NONE		0x00u		/* never */
#define BGDFS_TXBLANK_CHECK_MODE_ADJ		0x01u		/* if on adjacent channel */
#define BGDFS_TXBLANK_CHECK_MODE_NONADJ		0x02u		/* if on non-adjacent channel */
#define BGDFS_TXBLANK_CHECK_MODE_EU_WEATHER	0x04u		/* if on weather channel in EU */
#define BGDFS_TXBLANK_CHECK_MODE_ALL		0xFFu		/* always */

#endif /* BGDFS */


#if defined(RSDB_DFS_SCAN) || defined(BGDFS)
#define DFS_SCAN_IN_PROGRESS(dfs)  (dfs && dfs->dfs_scan && dfs->dfs_scan->inprogress)
#else /* RSDB_DFS_SCAN || BGDFS */
#define DFS_SCAN_IN_PROGRESS(dfs)  (FALSE)
#endif /* RSDB_DFS_SCAN || BGDFS */

#define RADAR_INFO_SHIFT_TYPE           0
#define RADAR_INFO_SHIFT_MINPW          4
#define RADAR_INFO_SHIFT_SUBBAND        14

#define RADAR_INFO_MASK_TYPE            0xf
#define RADAR_INFO_MASK_MINPW           0x1ff
#define RADAR_INFO_MASK_SUBBAND         0xf

/* Extract ITEM from bit packed radar info in RAD_INF. ITEM must be one of TYPE, MINPW or SUBBAND */
#define RADAR_INFO_EXTRACT(RAD_INF, ITEM) \
		((RAD_INF) >> RADAR_INFO_SHIFT_ ## ITEM & RADAR_INFO_MASK_ ## ITEM)
typedef struct wlc_dfs_cac {
	int	cactime_pre_ism;	/* configured preism cac time in second */
	int	cactime_post_ism;	/* configured postism cac time in second */
	uint32	nop_sec;		/* Non-Operation Period in second */
	int	ism_monitor;		/* 0 for off, non-zero to force ISM to monitor-only mode */
	wl_dfs_status_t	status;		/* data structure for handling dfs_status iovar */
	uint	cactime;      /* holds cac time in WLC_DFS_RADAR_CHECK_INTERVAL for current state */
	/* use of duration
	 * 1. used as a down-counter where timer expiry is needed.
	 * 2. (cactime - duration) is time elapsed at current state
	 */
	uint	duration;
	chanspec_t chanspec_next;	/* next dfs channel */
	bool	timer_running;
	chanspec_list_t *chanspec_forced_list; /* list of chanspecs to use when radar detected */
} wlc_dfs_cac_t;

#define DFS_MODESW_IDLE				0	/* mode switch for dfs scan is in idle. */
#define DFS_MODESW_DOWNGRADE_IN_PROGRESS	1	/* waiting for downgrade. */
#define DFS_MODESW_DOWNGRADE_IN_FINISHED	2	/* Downgrade is done. Scan in progress. */
#define DFS_MODESW_UPGRADE_IN_PROGRESS		3	/* Upgrade requested and pending state. */

#if defined(RSDB_DFS_SCAN) || defined(BGDFS)
struct dfs_scan_cmn {
	uint8	modeswitch_state; /* MIMO to SISO modewitch is pending. */
	uint8	inprogress;	/* DFS scan in progress */
	uint8	_was_ap;	/* Keep to AP state to trigger DFS scan. */
	int8 status;		/* Scan status */
	chanspec_t home_chan;	/* previous home chan */
	wlc_info_t *scan_wlc;	/* wlc where dfs scan is running */
	wl_chan_switch_t csa;	/* Chanswitch information after RADAR clear detection */
};
#endif /* RSDB_DFS_SCAN || BGDFS */

typedef struct dfs_scan_cmn dfs_scan_cmn_t;

/* Country module info */
struct wlc_dfs_info {
	wlc_info_t *wlc;
	uint chan_blocked[MAXCHANNEL];	/* 11h: seconds remaining in channel
					 * out of service period due to radar
					 */
	bool dfs_cac_enabled;		/* set if dfs cac enabled */
	struct wl_timer *dfs_timer;	/* timer for dfs cac handler */
	wlc_dfs_cac_t dfs_cac;		/* channel availability check */
	uint32 radar;			/* radar info: just on or off for now */


	dfs_scan_cmn_t *dfs_scan;	/* dfs_scan structure. Shared via objreg if RSDB */
	uint16 phymode;			/* last known phy mode; eg. PHYMODE_BGDFS */
	chanspec_t sc_chspec;		/* chanspec of scan core */
	bool upgrade_pending;		/* if upgrade to from BGDFS mode is pending */
	bool scan_both;			/* true when ISM on main core is running along with
					 * CAC on scan core
					 */
	bool modesw_cb_regd;		/* is modesw callback registered */
	bool move_stunted;		/* prevent channel change at end of dfs_ap_move */
	bool updown_cb_regd;		/* is updown callback registered */
	uint32 txdur_start;		/* tx duration (usecs) at start of CAC */
	uint32 txdur_delta_percent;	/* tx duration during CAC as percentage of CAC time */
	uint32 max_safe_tx_adj;		/* max safe tx on main core if on adjacent channel */
	uint32 max_safe_tx_nonadj;	/* max safe tx on main core if on non-adjacent chan */
	uint32 max_safe_tx_weather;	/* max safe tx on main core if scan on EU weather chan */
	uint32 txblank_check_mode;	/* Control considering tx on main core at end of CAC */
	bool in_eu;			/* whenever the interface goes up, update whether in EU */
	wlc_bsscfg_t *cfg;		/* AP bsscfg that triggered the DFS MOVE */
	radar_detected_info_t radar_detected;
	uint8 chan_cac_pending[MAXCHANNEL];	/* 11h: CAC pending on channel */
};

/* local functions */
/* module */
static void wlc_dfs_csa_each_up_ap(wlc_info_t *wlc, wl_chan_switch_t *csa, bool docs);
static void wlc_dfs_csa_reset_each_ap(wlc_info_t *wlc);
static void wlc_dfs_quiet_reset_each_ap(wlc_info_t *wlc);
static void wlc_dfs_to_backup_channel(wlc_dfs_info_t *dfs, bool radar_detected);
static int wlc_dfs_doiovar(void *ctx, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif);
static void wlc_dfs_watchdog(void *ctx);
static int wlc_dfs_up(void *ctx);
static int wlc_dfs_down(void *ctx);
static void wlc_dfs_eu_toggle(wlc_dfs_info_t *dfs, bool in_eu);
static void wlc_dfs_updown_cb(void *ctx, bsscfg_up_down_event_data_t *updown_data);
static int wlc_dfs_get_dfs_status_all(wlc_dfs_info_t *dfs, uint8 *arg, int len);
static void wlc_dfs_send_event(wlc_dfs_info_t *dfs, chanspec_t target_chanspec);


#ifdef BGDFS
#define DFS_BG_UPGRADE_DELAY_MS		0 /* delay after which upgrade is announced */

/* #define PHYMODE(w) (((phy_info_t *)WLC_PI(w))->cmni->phymode) */
#define PHYMODE(w) 0
#define phy_wd_override(x, y)  do { (void) x; (void) y; } while (0)
#define phy_ac_chanmgr_set_val_sc_chspec(x, y)  do { (void) x; (void) y; } while (0)
#define phy_ac_chanmgr_set_val_phymode(x, y)  do { (void) x; (void) y; } while (0)

static int wlc_dfs_get_chan_separation(wlc_info_t *wlc, chanspec_t chspec0, chanspec_t chspec1);
static uint32 wlc_dfs_get_max_safe_tx_threshold(wlc_dfs_info_t *dfs, bool adjacent, bool weather);
static bool wlc_dfs_discard_due_to_txblanking(wlc_dfs_info_t *dfs);
static void wlc_dfs_scan_complete_sc(wlc_dfs_info_t *dfs, int reason, int return_home_chan);
static void wlc_dfs_bg_upgrade_phy(wlc_dfs_info_t *dfs);
static void wlc_dfs_bg_downgrade_phy(wlc_dfs_info_t *dfs);
#ifdef WL_MODESW
static int wlc_dfs_bg_upgrade_wlc(wlc_info_t *wlc);
static int wlc_dfs_bg_downgrade_wlc(wlc_info_t *wlc);
#endif /* WL_MODESW */
static int wlc_dfs_bg_scan_prep(wlc_info_t *wlc);
#endif /* BGDFS */


#ifdef RSDB_DFS_SCAN
static bool wlc_dfs_is_phy_blanking_required(chanspec_t scan_chspec, chanspec_t home_chspec);
#endif /* RSDB_DFS_SCAN */

#if defined(BGDFS) && defined(WL_MODESW)
static void wlc_dfs_opmode_change_cb(void *ctx, wlc_modesw_notif_cb_data_t *notif_data);
#endif /* BGDFS && WL_MODESW */


#if defined(RSDB_DFS_SCAN) || defined(BGDFS)
static int wlc_dfs_scan(wlc_dfs_info_t *dfs, wl_chan_switch_t *csa);
static void wlc_dfs_handle_modeswitch(wlc_dfs_info_t *dfs, uint new_state);
static wlc_info_t * wlc_dfs_other_wlc_if_dual(wlc_info_t *wlc);
static int  wlc_dfs_scan_complete(wlc_dfs_info_t *dfs, int reason, int return_home_chan);
static int wlc_dfs_scan_prep(wlc_info_t *wlc);
static void wlc_dfs_scan_start(wlc_dfs_info_t *dfs, chanspec_t scan_chspec);
static void wlc_dfs_scan_cleanup(wlc_dfs_info_t *dfs, bool return_home_chan, bool upgrade);
#endif /* RSDB_DFS_SCAN || BGDFS */


#ifdef BCMDBG
static int wlc_dfs_dump(void *ctx, struct bcmstrbuf *b);
#endif

/* list of bandwidth in decreasing order of magnitude */
static uint16 wlc_dfs_bw_list[] = { WL_CHANSPEC_BW_160, WL_CHANSPEC_BW_8080, WL_CHANSPEC_BW_80,
	WL_CHANSPEC_BW_40, WL_CHANSPEC_BW_20, WL_CHANSPEC_BW_10, WL_CHANSPEC_BW_5,
	WL_CHANSPEC_BW_2P5 };

/* others */

static uint16*	wlc_dfs_get_bw_list(void);
static uint16	wlc_dfs_get_bw_list_len(void);
static void wlc_dfs_cacstate_init(wlc_dfs_info_t *dfs);
static int wlc_dfs_timer_init(wlc_dfs_info_t *dfs);
static void wlc_dfs_timer_add(wlc_dfs_info_t *dfs);
static bool wlc_dfs_timer_delete(wlc_dfs_info_t *dfs);
static void wlc_dfs_chanspec_oos(wlc_dfs_info_t *dfs, chanspec_t chanspec);
static chanspec_t wlc_dfs_chanspec_rand(wlc_dfs_info_t *dfs, uint16 *bw_list, uint16 bw_list_len,
		bool radar_detected);
static chanspec_t wlc_dfs_valid_forced_chanspec(wlc_dfs_info_t *dfs);
static void wlc_dfs_rearrange_channel_list(wlc_dfs_info_t *dfs, chanspec_list_t *ch_list);
static chanspec_t wlc_dfs_chanspec(wlc_dfs_info_t *dfs, bool radar_detected);
static bool wlc_radar_detected(wlc_dfs_info_t *dfs, bool scan_core);
static void wlc_dfs_cacstate_idle_set(wlc_dfs_info_t *dfs);
static void wlc_dfs_cacstate_ism_set(wlc_dfs_info_t *dfs);
static void wlc_dfs_cacstate_ooc_set(wlc_dfs_info_t *dfs, uint target_state);
static void wlc_dfs_cacstate_idle(wlc_dfs_info_t *dfs);
static void wlc_dfs_cacstate_cac(wlc_dfs_info_t *dfs);
static void wlc_dfs_cacstate_ism(wlc_dfs_info_t *dfs);
static void wlc_dfs_cacstate_csa(wlc_dfs_info_t *dfs);
static void wlc_dfs_cacstate_ooc(wlc_dfs_info_t *dfs);
static void wlc_dfs_cacstate_handler(void *arg);
static bool wlc_dfs_validate_forced_param(wlc_info_t *wlc, chanspec_t chanspec);

/* IE mgmt */
#ifdef STA
#ifdef BCMDBG
static int wlc_dfs_bcn_parse_ibss_dfs_ie(void *ctx, wlc_iem_parse_data_t *data);
#endif
#endif /* STA */

/* Local Data Structures */
static void (*wlc_dfs_cacstate_fn_ary[WL_DFS_CACSTATES])(wlc_dfs_info_t *dfs) = {
	wlc_dfs_cacstate_idle,
	wlc_dfs_cacstate_cac, /* preism_cac */
	wlc_dfs_cacstate_ism,
	wlc_dfs_cacstate_csa,
	wlc_dfs_cacstate_cac, /* postism_cac */
	wlc_dfs_cacstate_ooc, /* preism_ooc */
	wlc_dfs_cacstate_ooc /* postism_ooc */
};

#if defined(BCMDBG) || defined(WLMSG_DFS)
const char *dfs_cacstate_str[WL_DFS_CACSTATES] = {
	"IDLE",
	"PRE-ISM Channel Availability Check",
	"In-Service Monitoring(ISM)",
	"Channel Switching Announcement(CSA)",
	"POST-ISM Channel Availability Check",
	"PRE-ISM Out Of Channels(OOC)",
	"POSTISM Out Of Channels(OOC)"
};

static const char *
BCMRAMFN(wlc_dfs_cacstate_str)(uint state)
{
	const char * result;

	if (state >= ARRAYSIZE(dfs_cacstate_str))
		result = "UNKNOWN";
	else {
		result = dfs_cacstate_str[state];
	}

	return result;
}
#endif /* BCMDBG || WLMSG_DFS */

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>


/*
 * State change function for easier tracking of state changes.
 */
static INLINE void
wlc_dfs_cac_state_change(wlc_dfs_info_t *dfs, uint newstate)
{
#if defined(BCMDBG) || defined(WLMSG_DFS)
	WL_DFS(("DFS State %s -> %s\n", wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
		wlc_dfs_cacstate_str(newstate)));
#endif
	dfs->dfs_cac.status.state = newstate;	/* Controlled (logged) state change */
}


/* called when observing toggle between EU to/from non-EU country */
static void
wlc_dfs_eu_toggle(wlc_dfs_info_t *dfs, bool in_eu)
{
	// update in structure
	dfs->in_eu = in_eu;
#ifdef BGDFS
	if (in_eu) {
		dfs->max_safe_tx_nonadj = BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_NONADJACENT_ETSI;
	} else {
		dfs->max_safe_tx_nonadj = BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_NONADJACENT_FCC;
	}
#endif /* BGDFS */
}

/* Callback from wl up/down. This Function helps reset on up/down events */
static void
wlc_dfs_updown_cb(void *ctx, bsscfg_up_down_event_data_t *updown_data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_dfs_info_t *dfs;

	ASSERT(wlc);
	ASSERT(updown_data);
	if (wlc->dfs == NULL) {
		return;
	}
	dfs = wlc->dfs;

#if defined(BCMDBG) || defined(WLMSG_DFS)
	WL_DFS(("%s:got callback from updown. interface %s\n",
		__FUNCTION__, (updown_data->up?"up":"down")));
#endif /* BCMDBG || WLMSG_DFS */

	if (updown_data->up == TRUE) {
		bool in_eu = wlc_is_dfs_eu(wlc);
		if (in_eu != dfs->in_eu) {
			wlc_dfs_eu_toggle(dfs, in_eu);
		}
		return;
	}

	/* when brought down, mark current DFS channel as quiet unless in EU */
	if (WL11H_ENAB(wlc) && dfs->radar &&
			wlc_radar_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC) &&
			!wlc_is_edcrs_eu(wlc)) {
		wlc_set_quiet_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC);
	}

#if defined(BGDFS) || defined(RSDB_DFS_SCAN)
	if (dfs->dfs_scan->status == DFS_SCAN_S_INPROGESS) {
		WL_DFS(("Aborting dfs_ap_move\n"));
		wlc_dfs_scan_abort(wlc_dfs_other_wlc_if_dual(wlc)->dfs);
	}
#endif /* BGDFS || RSDB_DFS_SCAN */

	dfs->scan_both = FALSE;
	dfs->move_stunted = FALSE;
}

/* module */
wlc_dfs_info_t *
BCMATTACHFN(wlc_dfs_attach)(wlc_info_t *wlc)
{
	wlc_dfs_info_t *dfs;

	if ((dfs = MALLOCZ(wlc->osh, sizeof(wlc_dfs_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	dfs->wlc = wlc;
	dfs->updown_cb_regd = FALSE;
	dfs->modesw_cb_regd = FALSE;

	/* register IE mgmt callbacks */
	/* parse */
#ifdef STA
#ifdef BCMDBG
	/* bcn */
	if (wlc_iem_add_parse_fn(wlc->iemi, FC_BEACON, DOT11_MNG_IBSS_DFS_ID,
	                         wlc_dfs_bcn_parse_ibss_dfs_ie, dfs) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_add_parse_fn failed, ibss dfs in bcn\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif
#endif /* STA */

	if (wlc_dfs_timer_init(dfs) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_dfs_timer_init failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#ifdef BCMDBG
	if (wlc_dump_register(wlc->pub, "dfs", wlc_dfs_dump, dfs) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_dumpe_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif

	/* keep the module registration the last other add module unregistratin
	 * in the error handling code below...
	 */
	if (wlc_module_register(wlc->pub, wlc_dfs_iovars, "dfs", dfs, wlc_dfs_doiovar,
	                        wlc_dfs_watchdog, wlc_dfs_up, wlc_dfs_down) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	};
	wlc->pub->cmn->_radar = TRUE;
	wlc->pub->cmn->_wldfs = TRUE;

/* As per current design, if both BGDFS and RSDB_DFS_SCAN are defined, BGDFS gets favored */
#ifdef BGDFS
	dfs->in_eu = FALSE;
	dfs->txblank_check_mode = BGDFS_TXBLANK_CHECK_MODE_ALL;
	dfs->max_safe_tx_adj = BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_ADJACENT;
	dfs->max_safe_tx_nonadj = BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_NONADJACENT_FCC;
	dfs->max_safe_tx_weather = BGDFS_DISCARD_RESULT_TRAFFIC_PERCENT_WEATHER_ETSI;
#ifndef BGDFS_DISABLED
	wlc->pub->_bgdfs = DFS_HAS_BACKGROUND_SCAN_CORE(wlc);
#endif /* BGDFS_DISABLED */
#ifdef WL_MODESW
	if (WLC_MODESW_ENAB(wlc->pub)) {
		ASSERT(wlc->modesw != NULL);
		if (wlc_modesw_notif_cb_register(wlc->modesw,
				wlc_dfs_opmode_change_cb, wlc) == BCME_OK) {
			dfs->modesw_cb_regd = TRUE;
		}
	}
#endif /* WL_MODESW */
	if (dfs->dfs_scan == NULL) {
		if ((dfs->dfs_scan = MALLOCZ(wlc->osh, sizeof(*(dfs->dfs_scan)))) == NULL) {
			WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
					wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->pub->osh)));
			ASSERT(0);
			goto fail;
		}
		dfs->dfs_scan->status = DFS_SCAN_S_IDLE;
	}
#elif defined(RSDB_DFS_SCAN) && !defined(WLRSDB_DFS_SCAN_DISABLED)
	/* get from obj registry; if null allocate and set registry; increment reference counter */
	dfs->dfs_scan = (dfs_scan_cmn_t*)obj_registry_get(wlc->objr, OBJR_DFS_SCAN_INFO);
	if (dfs->dfs_scan == NULL) {
		if ((dfs->dfs_scan = MALLOCZ(wlc->osh, sizeof(dfs_scan_cmn_t))) == NULL) {
			WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->pub->osh)));
			ASSERT(0);
			goto fail;
		}
		obj_registry_set(wlc->objr, OBJR_DFS_SCAN_INFO, dfs->dfs_scan);
		dfs->dfs_scan->status = DFS_SCAN_S_IDLE;
	}
	(void)obj_registry_ref(wlc->objr, OBJR_DFS_SCAN_INFO);
	wlc->pub->cmn->_rsdb_dfs = TRUE;
#endif /* BGDFS,RSDB_DFS_SCAN, ... */

	if (wlc_bsscfg_updown_register(wlc, wlc_dfs_updown_cb, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_updown_register() failed\n",
				wlc->pub->unit, __FUNCTION__));
		ASSERT(0);
		goto fail;
	}
	dfs->updown_cb_regd = TRUE;
	return dfs;

	/* error handling */
fail:
	wlc_dfs_detach(dfs);
	return NULL;
}

void
BCMATTACHFN(wlc_dfs_detach)(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc;

	if (dfs == NULL) {
		return;
	}

	wlc = dfs->wlc;

	if (dfs->updown_cb_regd && wlc_bsscfg_updown_unregister(wlc,
			wlc_dfs_updown_cb, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_updown_unregister() failed\n",
				wlc->pub->unit, __FUNCTION__));
		dfs->updown_cb_regd = FALSE;
	}
#if defined(BGDFS) && defined(WL_MODESW)
	/* modesw is always detached before dfs; but wlc->modesw is not NULL */
	if (BGDFS_ENAB(wlc->pub) && WLC_MODESW_ENAB(wlc->pub) &&
			dfs->modesw_cb_regd && wlc->modesw) {
		wlc_modesw_notif_cb_unregister(wlc->modesw, wlc_dfs_opmode_change_cb, wlc);
		dfs->modesw_cb_regd = FALSE;
	}
#endif /* BGDFS && WL_MODESW */

	wlc_module_unregister(wlc->pub, "dfs", dfs);

	if (dfs->dfs_timer != NULL)
		wl_free_timer(wlc->wl, dfs->dfs_timer);

	if (dfs->dfs_cac.chanspec_forced_list)
		MFREE(wlc->osh, dfs->dfs_cac.chanspec_forced_list,
			WL_CHSPEC_LIST_FIXED_SIZE +
			dfs->dfs_cac.chanspec_forced_list->num * sizeof(chanspec_t));
#ifdef BGDFS
	if (dfs->dfs_scan != NULL) {
		MFREE(wlc->osh, dfs->dfs_scan, sizeof(*(dfs->dfs_scan)));
	}
#elif defined(RSDB_DFS_SCAN)
	if (obj_registry_unref(wlc->objr, OBJR_DFS_SCAN_INFO) == 0) {
		if (dfs->dfs_scan != NULL) {
			MFREE(wlc->osh, dfs->dfs_scan, sizeof(dfs_scan_cmn_t));
		}
		obj_registry_set(wlc->objr, OBJR_DFS_SCAN_INFO, NULL);
	}
#endif /* RSDB_DFS_SCAN */

	MFREE(wlc->osh, dfs, sizeof(wlc_dfs_info_t));
}

/* Validate chanspec forced list configuration. */
static bool wlc_dfs_validate_forced_param(wlc_info_t *wlc, chanspec_t chanspec)
{
	if (!WL11H_ENAB(wlc) || !CHSPEC_IS5G(chanspec) ||
		wf_chspec_malformed(chanspec) ||
		(!wlc_valid_chanspec_db(wlc->cmi, chanspec)) ||
		(!N_ENAB(wlc->pub) && (CHSPEC_IS40(chanspec) ||
		CHSPEC_IS80(chanspec) || CHSPEC_IS8080(chanspec) ||
		CHSPEC_IS160(chanspec)))) {
		return FALSE;
	}

	return TRUE;
}

/*
 * wlc_dfs_csa_each_up_ap - announce CSA on each AP interface that is up
 * wlc - handle to the wl interface
 * csa - parameters for channel switch announcement
 * docs - when true, channel switch will be effected immediately without waiting for
 *        countdown to finish
 */
static void
wlc_dfs_csa_each_up_ap(wlc_info_t *wlc, wl_chan_switch_t *csa, bool docs)
{
	wlc_bsscfg_t *apcfg;
	int idx;
	WL_DFS(("Announcing CSA to chanspec 0x%x on all up APs\n", csa->chspec));
	FOREACH_UP_AP(wlc, idx, apcfg) {
		wlc_csa_do_csa(wlc->csa, apcfg, csa, docs);
	}
}

/*
 * wlc_dfs_csa_reset_each_ap - reset CSA on each AP interface
 * wlc - handle to the wl interface
 */
static void
wlc_dfs_csa_reset_each_ap(wlc_info_t *wlc)
{
	wlc_bsscfg_t *apcfg;
	int idx;
	WL_DFS(("Reset CSA on all APs\n"));
	FOREACH_AP(wlc, idx, apcfg) {
		wlc_csa_reset_all(wlc->csa, apcfg);
	}
}

/*
 * wlc_dfs_quiet_reset_each_ap - reset 'quiet' on each AP interface
 * wlc - handle to the wl interface
 */
static void
wlc_dfs_quiet_reset_each_ap(wlc_info_t *wlc)
{
	wlc_bsscfg_t *apcfg;
	int idx;
	WL_DFS(("Reset 'quiet' on all APs\n"));
	FOREACH_AP(wlc, idx, apcfg) {
		wlc_quiet_reset_all(wlc->quiet, apcfg);
	}
}

static int
wlc_dfs_doiovar(void *ctx, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
	wlc_dfs_info_t *dfs = (wlc_dfs_info_t *)ctx;
	wlc_info_t *wlc = dfs->wlc;
	wlc_bsscfg_t *bsscfg;
	int err = BCME_OK;
	int32 int_val = 0;
	int32 int_val2 = 0;
	int32 *ret_int_ptr;
	bool bool_val;
	bool bool_val2;
	uint i;
	wl_dfs_forced_t *dfs_forced;
	chanspec_t chanspec = 0;

	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	if (p_len >= (int)sizeof(int_val) * 2)
		bcopy((void*)((uintptr)params + sizeof(int_val)), &int_val2, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	bool_val = (int_val != 0) ? TRUE : FALSE;
	bool_val2 = (int_val2 != 0) ? TRUE : FALSE;
	BCM_REFERENCE(bool_val2);

	/* update wlcif pointer */
	if (wlcif == NULL)
		wlcif = bsscfg->wlcif;
	ASSERT(wlcif != NULL);

	/* Do the actual parameter implementation */
	switch (actionid) {
	case IOV_GVAL(IOV_DFS_PREISM):
		*ret_int_ptr = dfs->dfs_cac.cactime_pre_ism;
		break;

	case IOV_SVAL(IOV_DFS_PREISM):
		if ((int_val < -1) || (int_val >= WLC_DFS_CAC_TIME_SEC_MAX)) {
			err = BCME_RANGE;
			break;
		}
		dfs->dfs_cac.cactime_pre_ism = int_val;
		break;

	case IOV_GVAL(IOV_DFS_POSTISM):
		*ret_int_ptr = dfs->dfs_cac.cactime_post_ism;
		break;

	case IOV_SVAL(IOV_DFS_POSTISM):
		if ((int_val < -1) || (int_val >= WLC_DFS_CAC_TIME_SEC_MAX)) {
			err = BCME_RANGE;
			break;
		}
		dfs->dfs_cac.cactime_post_ism = int_val;
		break;

	case IOV_GVAL(IOV_DFS_STATUS):
		dfs->dfs_cac.status.duration =
		        (dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
		        WLC_DFS_RADAR_CHECK_INTERVAL;
		bcopy((char *)&dfs->dfs_cac.status, (char *)arg, sizeof(wl_dfs_status_t));
		break;

	case IOV_SVAL(IOV_DFS_ISM_MONITOR):
		dfs->dfs_cac.ism_monitor = bool_val;
		break;

	case IOV_GVAL(IOV_DFS_ISM_MONITOR):
		*ret_int_ptr = (int32)dfs->dfs_cac.ism_monitor;
		break;

	/* IOV_DFS_CHANNEL_FORCED is required for regulatory testing */
	case IOV_SVAL(IOV_DFS_CHANNEL_FORCED):
		if (p_len <= (int)sizeof(int_val)) {
			/* Validate Configuration */
			if (!wlc_dfs_validate_forced_param(wlc, (chanspec_t)int_val)) {
				err = BCME_BADCHAN;
				break;
			}
			/* Single configuration is required for regulatory testing */
			if (!dfs->dfs_cac.chanspec_forced_list) {
				dfs->dfs_cac.chanspec_forced_list =
					MALLOCZ(wlc->osh, (WL_CHSPEC_LIST_FIXED_SIZE +
					1 * sizeof(chanspec_t)));

			if (!dfs->dfs_cac.chanspec_forced_list) {
					err = BCME_NOMEM;
					break;
				}
			}
			dfs->dfs_cac.chanspec_forced_list->num = 1;

			if (CHSPEC_CHANNEL((chanspec_t)int_val) == 0) {
				/* This is to handle old command syntax - single chanspec */
				dfs->dfs_cac.chanspec_forced_list->list[0] =
					wlc_create_chspec(wlc, (uint8)int_val);
				break;
			}
			dfs->dfs_cac.chanspec_forced_list->list[0] =
				(chanspec_t)int_val;
		} else {
			dfs_forced = (wl_dfs_forced_t*)params;
			if (dfs_forced->version == DFS_PREFCHANLIST_VER) {
				/* We got list configuration */
				if (dfs_forced->chspec_list.num > WL_NUMCHANNELS) {
					err = BCME_BADCHAN;
					break;
				}
				/* Validate Configuration */
				for (i = 0; i < dfs_forced->chspec_list.num; i++) {
					if (N_ENAB(wlc->pub)) {
						chanspec = dfs_forced->chspec_list.list[i];
					} else {
						chanspec = wlc_create_chspec(wlc,
							(uint8)dfs_forced->chspec_list.list[i]);
					}
					if (!wlc_dfs_validate_forced_param(wlc, chanspec)) {
						err = BCME_BADCHAN;
						return err;
					}
				}
				/* Clears existing list if any */
				if (dfs->dfs_cac.chanspec_forced_list) {
					MFREE(wlc->osh, dfs->dfs_cac.chanspec_forced_list,
					  WL_CHSPEC_LIST_FIXED_SIZE +
					  dfs->dfs_cac.chanspec_forced_list->num *
					  sizeof(chanspec_t));
					dfs->dfs_cac.chanspec_forced_list = NULL;
				}
				if (!dfs_forced->chspec_list.num) {
					/* Clear configuration done */
					break;
				}
				dfs->dfs_cac.chanspec_forced_list =
					MALLOCZ(wlc->osh, (WL_CHSPEC_LIST_FIXED_SIZE +
					dfs_forced->chspec_list.num * sizeof(chanspec_t)));

				if (!dfs->dfs_cac.chanspec_forced_list) {
					err = BCME_NOMEM;
					break;
				}
				dfs->dfs_cac.chanspec_forced_list->num =
					dfs_forced->chspec_list.num;

				for (i = 0; i < dfs_forced->chspec_list.num; i++) {
					if (N_ENAB(wlc->pub)) {
						chanspec = dfs_forced->chspec_list.list[i];
					} else {
						chanspec = wlc_create_chspec(wlc,
							(uint8)dfs_forced->chspec_list.list[i]);
					}
					dfs->dfs_cac.chanspec_forced_list->list[i] = chanspec;
				}
			} else {
				err = BCME_VERSION;
				break;
			}
		}
		break;

	case IOV_GVAL(IOV_DFS_CHANNEL_FORCED):
		if (p_len < sizeof(wl_dfs_forced_t)) {
			if (dfs->dfs_cac.chanspec_forced_list) {
				*ret_int_ptr = dfs->dfs_cac.chanspec_forced_list->list[0];
			} else
				*ret_int_ptr = 0;
		} else {
			wl_dfs_forced_t *inp = (wl_dfs_forced_t*)params;
			dfs_forced = (wl_dfs_forced_t*)arg;
			/* This is issued by new application */
			if (inp->version == DFS_PREFCHANLIST_VER) {
				dfs_forced->version = DFS_PREFCHANLIST_VER;
				if (dfs->dfs_cac.chanspec_forced_list) {
					uint nchan = dfs->dfs_cac.chanspec_forced_list->num;
					uint ioctl_size = WL_DFS_FORCED_PARAMS_FIXED_SIZE +
						dfs->dfs_cac.chanspec_forced_list->num *
						sizeof(chanspec_t);

					if (p_len < ioctl_size) {
						err = BCME_BUFTOOSHORT;
						break;
					}
					for (i = 0; i < nchan; i++) {
						chanspec =
							dfs->dfs_cac.chanspec_forced_list->list[i];
						dfs_forced->chspec_list.list[i] = chanspec;
					}
					dfs_forced->chspec_list.num = nchan;
					/* Rearranges channel list, bringing valid (non current)
					 * channels to head of the list
					 */
					wlc_dfs_rearrange_channel_list(dfs,
							&dfs_forced->chspec_list);
					break;
				} else {
					dfs_forced->chspec_list.num = 0;
					/* wlu reads dfs_forced->chspec when list is not there.
					 * So assigning it with 0 otherwise wlu reports junk.
					 * Driver does not use chspec, it uses chspec_list instead.
					 */
					dfs_forced->chspec = 0;
				}
			} else {
				err = BCME_VERSION;
				break;
			}
		}
		break;
#if defined(BGDFS) || defined(RSDB_DFS_SCAN)
	case IOV_SVAL(IOV_DFS_AP_MOVE):
	{
		wl_chan_switch_t csa;
		chanspec_t chspec = (chanspec_t)int_val;

		if (wlc_11h_get_spect(wlc->m11h) == SPECT_MNGMT_OFF) {
			WL_DFS(("Rejecting dfs_ap_move 11h (spect) spectrum management is OFF\n"));
			err = BCME_NOTENABLED;
			break;
		}

		if (wlc->pub->_ap != 1) {
			WL_DFS(("Rejecting dfs_ap_move wlc->pub->_ap=%d\n", wlc->pub->_ap));
			err = BCME_NOTAP;
			break;
		}

		if ((RSDB_ENAB(wlc->pub) && WLRSDBDFS_ENAB(wlc->pub)) || BGDFS_ENAB(wlc->pub)) {
			err = BCME_OK;
		} else {
			err = BCME_UNSUPPORTED;
			break;
		}

		if (!AP_ACTIVE(wlc) || !BSSCFG_AP(bsscfg)) {
			err = BCME_NOTAP;
			break;
		}
		if ((int16)chspec == -1) {
			WL_DFS(("Abort dfs_ap_move request in state %d\n",
					dfs->dfs_scan->status));
			if (dfs->dfs_scan->status == DFS_SCAN_S_INPROGESS) {
				WL_DFS(("Aborting dfs_ap_move\n"));
				wlc_dfs_scan_abort(wlc_dfs_other_wlc_if_dual(wlc)->dfs);
				break;
			} else if (dfs->dfs_scan->status ==
					DFS_SCAN_S_SCAN_MODESW_INPROGRESS) {
				WL_DFS(("Can not abort dfs_ap_move in middle of modesw; retry\n"));
				err = BCME_BUSY;
				break;
			}
		}
		/* helps test scan core without really effecting a move/channel change;
		 * affects last dfs_ap_move in progress only
		 */
		if ((int16)chspec == -2) {
			WL_DFS(("Stunt recent dfs_ap_move request in state %d\n",
					dfs->dfs_scan->status));
			dfs->move_stunted = TRUE;
			break;
		}

		WL_DFS(("%s scan channel: %04x, current channel %04x\n", __FUNCTION__,
				chspec, WLC_BAND_PI_RADIO_CHANSPEC));

		/* reject if the chanspec is already current radio chanspec */
		if (chspec == WLC_BAND_PI_RADIO_CHANSPEC) {
			err = BCME_BADCHAN;
			break;
		}
		if (wf_chspec_malformed(chspec) || !wlc_radar_chanspec(wlc->cmi, chspec)) {
			err = BCME_BADCHAN;
			break;
		}
		if (CHSPEC_BW(chspec) != CHSPEC_BW(WLC_BAND_PI_RADIO_CHANSPEC)) {
			WL_DFS(("Scan channel bandwidth %d must match current bandwidth %d\n",
					CHSPEC_BW(chspec),
					CHSPEC_BW(WLC_BAND_PI_RADIO_CHANSPEC)));
			err = BCME_BADCHAN;
			break;
		}
		if (!wlc_dfs_valid_ap_chanspec(wlc, chspec)) {
			WL_DFS(("Scan channel 0x%x Not Available\n", chspec));
			err = BCME_BADCHAN;
			break;
		}

		csa.mode = DOT11_CSA_MODE_ADVISORY;
		csa.chspec = chspec;
		csa.count = bsscfg->current_bss->dtim_period;
		csa.reg = wlc_get_regclass(wlc->cmi, csa.chspec);
		csa.frame_type = CSA_BROADCAST_ACTION_FRAME;

		dfs->cfg = bsscfg; /* update DFS bsscfg */

		if (wlc_is_edcrs_eu(wlc) && !wlc_quiet_chanspec(wlc->cmi, chspec)) {
			WL_DFS(("Skip CAC - channel 0x%x is already available (ETSI). To CSA\n",
					chspec));
			wlc_dfs_csa_each_up_ap(wlc, &csa, FALSE);
		} else {
			err = wlc_dfs_scan(dfs, &csa);
		}

		break;
	}
	case IOV_GVAL(IOV_DFS_AP_MOVE):
	{
		wl_dfs_ap_move_status_t * ap_move_status = (wl_dfs_ap_move_status_t *)arg;
		if (len < sizeof(wl_dfs_ap_move_status_t)) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		if ((RSDB_ENAB(wlc->pub) && WLRSDBDFS_ENAB(wlc->pub)) || BGDFS_ENAB(wlc->pub)) {
			err = BCME_OK;
		} else {
			err = BCME_UNSUPPORTED;
			break;
		}
		memset(ap_move_status, 0, sizeof(wl_dfs_ap_move_status_t));

		ap_move_status->version = (int8)WL_DFS_AP_MOVE_VERSION;
		ap_move_status->move_status = (int8)dfs->dfs_scan->status;
		ap_move_status->chanspec = dfs->dfs_scan->csa.chspec;
		err = wlc_dfs_get_dfs_status_all(dfs, (uint8*)(&ap_move_status->scan_status),
				len-sizeof(wl_dfs_ap_move_status_t)+sizeof(wl_dfs_status_all_t));
		WL_DFS(("dfs_scan->status=%d move_status=%d\n", dfs->dfs_scan->status,
				ap_move_status->move_status));

		break;
	}
#endif /* BGDFS || RSDB_DFS_SCAN */

	case IOV_GVAL(IOV_DFS_STATUS_ALL):
		err = wlc_dfs_get_dfs_status_all(dfs, (uint8*)arg, len);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

#if defined(BGDFS) && defined(WL_MODESW)
/* Initiates upgrade of wlc to full MIMO (NSS) using modesw module. */
static int
wlc_dfs_bg_upgrade_wlc(wlc_info_t *wlc)
{
	int idx = 0;
	wlc_bsscfg_t *bsscfg;
	uint8 new_oper_mode = 0, curr_oper_mode, bw = 0, nss;
	bool mode_switch_sched = FALSE;
	int err = BCME_UNSUPPORTED;
	int max_nss = WLC_BITSCNT(wlc->stf->hw_txchain_cap);

	wlc->dfs->phymode = PHYMODE(wlc);

	/* announce upgrade */
	FOREACH_UP_AP(wlc, idx, bsscfg)
	{
		bw = DOT11_OPER_MODE_80MHZ;
		if (WLC_BSS_CONNECTED(bsscfg)) {
			curr_oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
					bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);
			bw = DOT11_OPER_MODE_CHANNEL_WIDTH(curr_oper_mode);
			nss = DOT11_OPER_MODE_RXNSS(curr_oper_mode);
			if (nss == max_nss)
				continue;
		}
		new_oper_mode = DOT11_OPER_MODE(0, max_nss, bw);
		err = wlc_modesw_handle_oper_mode_notif_request(wlc->modesw, bsscfg,
				new_oper_mode, TRUE, MODESW_CTRL_OPMODE_IE_REQD_OVERRIDE);
		if (err != BCME_OK) {
			WL_DFS(("wl%d: failed to request modesw %d\n", wlc->pub->unit, err));
			break;
		}
		mode_switch_sched = TRUE;
	}

	WL_DFS(("wl%d: mode switch up scheduled = %d opmode: 0x%02x, bw: 0x%02x\n",
			wlc->pub->unit, mode_switch_sched, new_oper_mode, bw));

	return mode_switch_sched ? BCME_BUSY : BCME_OK;
}

/* Initiates downgrade of wlc to reduced MIMO plus scan core (3+1) using modesw module. */
static int
wlc_dfs_bg_downgrade_wlc(wlc_info_t *wlc)
{
	int idx = 0;
	wlc_bsscfg_t *bsscfg;
	uint8 new_oper_mode = 0, curr_oper_mode, bw = 0, nss;
	bool mode_switch_sched = FALSE;
	int err = BCME_UNSUPPORTED;
	int max_nss = WLC_BITSCNT(wlc->stf->hw_txchain_cap);

	wlc->dfs->phymode = PHYMODE(wlc);

	FOREACH_UP_AP(wlc, idx, bsscfg)
	{
		bw = DOT11_OPER_MODE_80MHZ;
		if (WLC_BSS_CONNECTED(bsscfg)) {
			curr_oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
					bsscfg->current_bss->chanspec, bsscfg, wlc->stf->rxstreams);
			bw = DOT11_OPER_MODE_CHANNEL_WIDTH(curr_oper_mode);
			nss = DOT11_OPER_MODE_RXNSS(curr_oper_mode);
			if (nss == (max_nss - 1))
				continue;
		}
		new_oper_mode = DOT11_OPER_MODE(0, (max_nss - 1), bw);
		err = wlc_modesw_handle_oper_mode_notif_request(wlc->modesw, bsscfg,
				new_oper_mode, TRUE, MODESW_CTRL_OPMODE_IE_REQD_OVERRIDE);
		if (err != BCME_OK) {
			WL_DFS(("wl%d: failed to request modesw %d\n", wlc->pub->unit, err));
			break;
		}
		mode_switch_sched = TRUE;
	}

	WL_DFS(("wl%d: mode switch down scheduled = %d opmode: 0x%02x, bw: 0x%02x\n",
			wlc->pub->unit, mode_switch_sched, new_oper_mode, bw));

	return mode_switch_sched ? BCME_BUSY : BCME_OK;
}
#endif /* BGDFS && WL_MODESW */


#ifdef BGDFS
/* Chooses the cores on which RADAR scan is required and initiates downgrade as required */
static int
wlc_dfs_bg_scan_prep(wlc_info_t *wlc)
{
	wlc_dfs_info_t *dfs = wlc->dfs;
	dfs->phymode = PHYMODE(wlc);

	/* ISM must continue on main core while scan core goes into CAC */
	dfs->scan_both = wlc_radar_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC);

	if (PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
		return BCME_OK;
	}

#ifdef WL_MODESW
	if (!dfs->modesw_cb_regd) {
		if (wlc->modesw && wlc_modesw_notif_cb_register(wlc->modesw,
				wlc_dfs_opmode_change_cb, wlc) == BCME_OK) {
			dfs->modesw_cb_regd = TRUE;
		} else {
			WL_ERROR(("wl%d: %s: Could not initialize modesw notification callback\n",
					wlc->pub->unit, __FUNCTION__));
		}
	}
	/* downgrade from NxN to (N-1)x(N-1) radar scan core mode after announcing */
	if (dfs->modesw_cb_regd &&
			wlc_dfs_bg_downgrade_wlc(wlc) == BCME_BUSY) {
		/* downgrade phy later */
		return BCME_BUSY;
	}
	else
#endif /* WL_MODESW */
	{
		/* downgrade phy right away */
		wlc_dfs_bg_downgrade_phy(dfs);
		return BCME_OK;
	}
}

/* Downgrades PHY to enable scan core on a specific channel;
 * disables watchdog to avoid periodic calibration
 */
static void
wlc_dfs_bg_downgrade_phy(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;

	ASSERT(dfs->dfs_scan);

	/* disable periodic calibration; so that it doesn't interfere throughout CAC */
	phy_wd_override((phy_info_t *)WLC_PI(wlc), FALSE);

	wlc_mute(wlc, ON, PHY_MUTE_FOR_PREISM);
	phy_ac_chanmgr_set_val_sc_chspec((phy_info_t *)WLC_PI(wlc), dfs->dfs_scan->csa.chspec);
	phy_ac_chanmgr_set_val_phymode((phy_info_t *)WLC_PI(wlc), PHYMODE_BGDFS);
	dfs->phymode = PHYMODE(wlc);
	dfs->upgrade_pending = TRUE;
	wlc_mute(wlc, OFF, PHY_MUTE_FOR_PREISM);

	WL_DFS(("wl%d: downgraded phy to %d+1 mode\n", wlc->pub->unit,
			(WLC_BITSCNT(wlc->stf->hw_txchain_cap) - 1)));
	ASSERT(PHYMODE_BGDFS_ACTIVE(dfs->phymode));
}

/* Upgrade PHY to disable scan core and move to full MIMO (NSS) mode; enables watchdog */
static void
wlc_dfs_bg_upgrade_phy(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;

	ASSERT(dfs->dfs_scan);

	wlc_mute(wlc, ON, PHY_MUTE_FOR_PREISM);
	phy_ac_chanmgr_set_val_phymode((phy_info_t *)WLC_PI(wlc), 0);
	dfs->phymode = PHYMODE(wlc);
	dfs->upgrade_pending = FALSE;
	wlc_mute(wlc, OFF, PHY_MUTE_FOR_PREISM);

	/* enable periodic calibration */
	phy_wd_override((phy_info_t *)WLC_PI(wlc), TRUE);

	WL_DFS(("wl%d: upgraded phy to %dx%d\n", wlc->pub->unit,
			WLC_BITSCNT(wlc->stf->txchain), WLC_BITSCNT(wlc->stf->rxchain)));
	ASSERT(dfs->phymode == 0);
}

#ifdef WL_MODESW
/* callback registered to mode switch module; called on upgrade/downgrade */
static void
wlc_dfs_opmode_change_cb(void *ctx, wlc_modesw_notif_cb_data_t *notif_data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_dfs_info_t *dfs;
#if defined(BCMDBG) || defined(WLMSG_MODESW)
	wlc_bsscfg_t *bsscfg = NULL;
#endif /* BCMDBG || WLMSG_MODESW */
	ASSERT(wlc);
	ASSERT(notif_data);
	if (wlc->dfs == NULL) {
		return;
	}
	dfs = wlc->dfs;

	if (dfs->cfg == NULL) {
		return;
	}

	WL_MODE_SWITCH(("wl%d: MODESW Callback status: %d, opmode: %x, signal: %d, state: %d\n",
			WLCWLUNIT(wlc), notif_data->status, notif_data->opmode, notif_data->signal,
			dfs->dfs_scan->modeswitch_state));
#if defined(BCMDBG) || defined(WLMSG_MODESW)
	bsscfg = notif_data->cfg;
#endif /* BCMDBG || WLMSG_MODESW */
	switch (notif_data->signal) {
		case MODESW_PHY_DN_COMPLETE:
		case MODESW_PHY_UP_COMPLETE:
			{
				int dfs_modesw_state = dfs->dfs_scan->modeswitch_state;
				if (dfs_modesw_state == DFS_MODESW_DOWNGRADE_IN_PROGRESS) {
					wlc_dfs_bg_downgrade_phy(dfs);
				} else if (dfs_modesw_state == DFS_MODESW_UPGRADE_IN_PROGRESS) {
					wlc_dfs_bg_upgrade_phy(dfs);
				}
				WL_MODE_SWITCH(("wl%d: Changed phy mode to (%d) by cfg = %d\n",
						WLCWLUNIT(wlc), dfs->phymode, bsscfg->_idx));
			}
			break;
		case MODESW_PHY_UP_START:
			WL_MODE_SWITCH(("wl%d: Changed chip phy mode to (%d) by cfg = %d\n",
					WLCWLUNIT(wlc), PHYMODE_MIMO, bsscfg->_idx));
			break;
	}

	wlc_dfs_handle_modeswitch(dfs, notif_data->signal);

	return;
}
#endif /* WL_MODESW */

#endif /* BGDFS */


#ifdef RSDB_DFS_SCAN
static bool
wlc_dfs_is_phy_blanking_required(chanspec_t scan_chspec, chanspec_t home_chspec)
{
	int scan_freq;
	int home_freq;
	int separation;

	/* phy separation is required only when both chanspecs are 5G */
	if (CHSPEC_IS2G(home_chspec) || CHSPEC_IS2G(scan_chspec)) {
		return FALSE;
	}

	scan_freq = wf_channel2mhz(CHSPEC_CHANNEL(scan_chspec), CHSPEC_IS5G(scan_chspec)
		? WF_CHAN_FACTOR_5_G : WF_CHAN_FACTOR_2_4_G);
	home_freq = wf_channel2mhz(CHSPEC_CHANNEL(home_chspec), CHSPEC_IS5G
		(home_chspec) ? WF_CHAN_FACTOR_5_G : WF_CHAN_FACTOR_2_4_G);
	separation = ABS(home_freq - scan_freq);

	if ((CHSPEC_IS20(home_chspec) && (separation < 80)) ||
			(CHSPEC_IS40(home_chspec) && (separation < 160)) ||
			(CHSPEC_IS80(home_chspec) && (separation < 240))) {
		return TRUE;
	}

	return FALSE;
}
#endif /* RSDB_DFS_SCAN */

#if defined(RSDB_DFS_SCAN) || defined(BGDFS)
/* apply/initiate any mode changes required before DFS scan */
static int
wlc_dfs_scan_prep(wlc_info_t *wlc)
{
	int ret_val = 0;
#ifdef BGDFS
	if (BGDFS_ENAB(wlc->pub)) {
		ret_val = wlc_dfs_bg_scan_prep(wlc);
	}
#elif defined(RSDB_DFS_SCAN)
	if (RSDB_ENAB(wlc->pub)) {
		ret_val = wlc_rsdb_dfs_scan_prep(wlc);
	}
#endif /* RSDB_DFS_SCAN, BGDFS, ... */
	return ret_val;
}

int
wlc_dfs_scan_in_progress(wlc_dfs_info_t *dfs)
{
	return DFS_SCAN_IN_PROGRESS(dfs);
}

void
wlc_dfs_scan_abort(wlc_dfs_info_t *dfs)
{
#ifdef BGDFS
	if (BGDFS_ENAB(dfs->wlc->pub) && PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
		wlc_dfs_scan_complete_sc(dfs, DFS_SCAN_S_SCAN_ABORTED, TRUE);
		wlc_dfs_bg_upgrade_phy(dfs);
	}
	else
#endif
	{
		wlc_dfs_scan_complete(dfs, DFS_SCAN_S_SCAN_ABORTED, TRUE);
	}
}


static int
wlc_dfs_scan(wlc_dfs_info_t *dfs, wl_chan_switch_t *csa)
{
	wlc_info_t *wlc = dfs->wlc;
	int ret_val = 0;
	ASSERT(wlc->pub->_ap);

	ASSERT(csa != NULL);

	memcpy(&dfs->dfs_scan->csa, csa, sizeof(wl_chan_switch_t));
	WL_DFS(("wl%d %s %d chspec=%x\n", wlc->pub->unit, __FUNCTION__, __LINE__,
		csa->chspec));

	/* request for wlc downgrade if in MIMO mode. */
	ret_val = wlc_dfs_scan_prep(wlc);
	dfs->dfs_scan->status = DFS_SCAN_S_IDLE;
	dfs->move_stunted = FALSE;

	if (ret_val == BCME_OK) {
		/* Start dfs scan (on other_wlc if RSDB) */
		wlc_dfs_scan_start(wlc_dfs_other_wlc_if_dual(wlc)->dfs, csa->chspec);
	} else if (ret_val == BCME_BUSY) {
		/* Wait for protocol & HW modeswitch to complete. */
		dfs->dfs_scan->modeswitch_state = DFS_MODESW_DOWNGRADE_IN_PROGRESS;
		dfs->dfs_scan->status = DFS_SCAN_S_SCAN_MODESW_INPROGRESS;
	} else {
		return ret_val;
	}

	return BCME_OK;
}

static void
wlc_dfs_handle_modeswitch(wlc_dfs_info_t *dfs, uint new_state)
{
	switch (dfs->dfs_scan->modeswitch_state)
	{
		case DFS_MODESW_IDLE:
			WL_DFS(("wl%d %s: new state %d in idle state\n", dfs->wlc->pub->unit,
					__FUNCTION__, new_state));
			break;

		case DFS_MODESW_DOWNGRADE_IN_PROGRESS:
			if (new_state == MODESW_DN_AP_COMPLETE) {
				dfs->dfs_scan->modeswitch_state = DFS_MODESW_DOWNGRADE_IN_FINISHED;
				WL_DFS(("wl%d %s: downgrade completed %d\n", dfs->wlc->pub->unit,
					__FUNCTION__, __LINE__));
				/* Start dfs scan (on other wlc if RSDB) */
				wlc_dfs_scan_start(wlc_dfs_other_wlc_if_dual(dfs->wlc)->dfs,
					dfs->dfs_scan->csa.chspec);
			}
			break;

		case DFS_MODESW_UPGRADE_IN_PROGRESS:
			if (new_state == MODESW_UP_AP_COMPLETE ||
					new_state == MODESW_DN_AP_COMPLETE) {
				dfs->dfs_scan->modeswitch_state = DFS_MODESW_IDLE;
				WL_DFS(("wl%d %s: upgrade completed %d\n", dfs->wlc->pub->unit,
					__FUNCTION__, __LINE__));
			}
			break;

		default:
			break;
	}
}

static void
wlc_dfs_scan_start(wlc_dfs_info_t *dfs, chanspec_t scan_chspec)
{
	wlc_info_t *wlc = dfs->wlc;
#ifndef BGDFS
	wlc_info_t *other_wlc = NULL;
	bool phy_blank = FALSE;
#endif /* ndef BGDFS */


#ifdef BGDFS

	if (BGDFS_ENAB(wlc->pub)) {
		/* when called in BGDFS mode use the scan core for CAC */
		dfs->dfs_scan->inprogress = TRUE;
		dfs->dfs_scan->scan_wlc = wlc;
		dfs->dfs_scan->status = DFS_SCAN_S_INPROGESS;
		dfs->sc_chspec = scan_chspec;
		wlc_set_dfs_cacstate(dfs, ON, dfs->cfg);
		dfs->dfs_scan->_was_ap = wlc->pub->_ap;
		dfs->txdur_start = wlc_bmac_cca_read_counter(wlc->hw, M_CCA_TXDUR_L_OFFSET(wlc->hw),
				M_CCA_TXDUR_H_OFFSET(wlc->hw));
		WL_DFS(("wl%d.. %s txdur_start %uus\n", wlc->pub->unit, __FUNCTION__,
				dfs->txdur_start));
	}

#elif defined(RSDB_DFS_SCAN)

	/* Keep backup of home_chanspec */
	dfs->dfs_scan->home_chan = wlc->home_chanspec;

	other_wlc = wlc_dfs_other_wlc_if_dual(wlc);
	/* If other wlc is associated, we need to apply certain phy parameters for same band
	 * operation.
	 */
	if (other_wlc->pub->associated) {
		ASSERT(!MCHAN_ACTIVE(other_wlc->pub));
		phy_blank = wlc_dfs_is_phy_blanking_required(other_wlc->home_chanspec,
			scan_chspec);
	}
	dfs->dfs_scan->inprogress = FALSE;
	(void) wlc_mpc_off_req_set(wlc, MPC_OFF_REQ_DFS_SCAN_ACTIVE, MPC_OFF_REQ_DFS_SCAN_ACTIVE);

	/* ToDo: wlc_phy_set_rsdb_dfs_scan_in_progress((phy_info_t *)WLC_PI(wlc), ?)
	 *		CHSPEC_IS5G(home_chspec) && CHSPEC_IS5G(scan_chspec));
	 */
	/* If scanning close by channels set ucode radar blanking host flags */
	if (phy_blank) {
		wlc_mhf(wlc, MHF4, MHF4_RSDB_DFS_SCAN, MHF4_RSDB_DFS_SCAN, WLC_BAND_ALL);
		wlc_mhf(other_wlc, MHF4, MHF4_RSDB_DFS_SCAN, MHF4_RSDB_DFS_SCAN, WLC_BAND_ALL);
	}
	WL_DFS(("wl%d %s DFS scan chan=%x(%d)\n", wlc->pub->unit, __FUNCTION__,
		scan_chspec, CHSPEC_CHANNEL(scan_chspec)));

	dfs->dfs_scan->inprogress = TRUE;
	dfs->dfs_scan->scan_wlc = wlc;
	dfs->dfs_scan->status = DFS_SCAN_S_INPROGESS;

	wlc->home_chanspec = scan_chspec;
	/* Change to DFS Scan channel */
	wlc_suspend_mac_and_wait(wlc);
	wlc_set_chanspec(wlc, scan_chspec, CHANSW_APCS);
	wlc_enable_mac(wlc);

	/* Keep a backup of _ap status */
	dfs->dfs_scan->_was_ap = wlc->pub->_ap;
	/* Force AP=1 for dfs_scan. */
	wlc->pub->_ap = 1;
	/* Start CAC state machine */
	wlc_set_dfs_cacstate(dfs, ON, dfs->cfg);
#endif /* BGDFS, RSDB_DFS_SCAN, ... */
}

/* retrieves other wlc if dual band is in use; returns the same (passed) wlc otherwise */
static wlc_info_t *
wlc_dfs_other_wlc_if_dual(wlc_info_t *wlc)
{
#ifdef BGDFS
	if (BGDFS_ENAB(wlc->pub)) {
		return wlc; /* BGDFS uses single instance of WLC */
	}
#elif defined(RSDB_DFS_SCAN)
	if (RSDB_ENAB(wlc->pub)) {
		return wlc_rsdb_get_other_wlc(wlc);
	}
#endif /* BGDFS, RSDB_DFS_SCAN */
	return wlc;
}

#ifdef BGDFS
/* Given two chanspecs, returns channel separation between those in mHz units */
static int
wlc_dfs_get_chan_separation(wlc_info_t *wlc, chanspec_t chspec0, chanspec_t chspec1)
{
	int freq0, freq1, separation;

	BCM_REFERENCE(wlc);

	freq0 = wf_channel2mhz(CHSPEC_CHANNEL(chspec0), CHSPEC_IS5G(chspec0)
		? WF_CHAN_FACTOR_5_G : WF_CHAN_FACTOR_2_4_G);
	freq1 = wf_channel2mhz(CHSPEC_CHANNEL(chspec1), CHSPEC_IS5G(chspec1)
		? WF_CHAN_FACTOR_5_G : WF_CHAN_FACTOR_2_4_G);
	separation = ABS(freq0 - freq1);

	WL_DFS(("wl%d.. %s ch0:0x%x freq0: %dMHz ch1:0x%x freq1: %dMHz sep: %dMHz\n",
			wlc->pub->unit, __FUNCTION__,
			chspec0, freq0, chspec1, freq1, separation));

	return separation;
}

/* returns max safe tx threshold for requested type;
 * weather channel check in ETSI takes highest precedence
 * adjacent channel check comes next
 * when not adjacent, different thresholds will be returned depending on country
 *
 * When multiple conditions satisfy, the most stringent threshold is returned.
 */
static uint32
wlc_dfs_get_max_safe_tx_threshold(wlc_dfs_info_t *dfs, bool adjacent, bool weather)
{
	uint32 threshold = 0;

	threshold = adjacent ? dfs->max_safe_tx_adj :  dfs->max_safe_tx_nonadj;
	if (weather) {
		/* override with weather channel safe tx threshold only if more stringent
		 * than due to other factors computed earlier
		 */
		return MIN(threshold, dfs->max_safe_tx_weather);
	}

	return threshold;
}

/*
 * discard background scan results if traffic on main core exceeds certain thresholds
 *
 * Due to tx blanking scan core results will need to be discarded when main core traffic
 * exceeds certain thresholds.
 *
 * This function is used to discard 'radar free' result at end of scan core CAC if traffic
 * on main core (and hence txblanking) exceeds certain tested thresholds.
 *
 */
static bool
wlc_dfs_discard_due_to_txblanking(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
	uint32 txdur_end, txdur_delta;
	bool adjacent = FALSE, weather = FALSE;
	uint32 threshold = 0;
	int separation;
	bool discard = FALSE;
	uint16 bw_flag = CHSPEC_BW(WLC_BAND_PI_RADIO_CHANSPEC), bw_mhz = CHAN_BW_20MHZ;

	// mark these variables if txblank_check_mode requires the respective checks
	bool check_adj = (dfs->txblank_check_mode & BGDFS_TXBLANK_CHECK_MODE_ADJ) != 0;
	bool check_nonadj = (dfs->txblank_check_mode & BGDFS_TXBLANK_CHECK_MODE_NONADJ) != 0;
	bool check_weather = (dfs->txblank_check_mode & BGDFS_TXBLANK_CHECK_MODE_EU_WEATHER) != 0;

	if (dfs->txblank_check_mode == 0) {
		return FALSE;
	}

	/* get tx duration now (end of CAC) */
	txdur_end = wlc_bmac_cca_read_counter(wlc->hw, M_CCA_TXDUR_L_OFFSET(wlc->hw),
			M_CCA_TXDUR_H_OFFSET(wlc->hw));
	/* get tx duration delta since start of CAC */
	txdur_delta = txdur_end - dfs->txdur_start; /* uint32 will auto-handle wrapping counters */

	/* compute traffic percentage to compare with tested thresholds */
	if (dfs->dfs_cac.cactime > 0 && txdur_delta > 0) {
		/*
		 * txdur_delta (in microsecs) won't be larger than max CAC = 600s = 0x23c34600us
		 * uint32 can fit ~4000s; intermediate and final values are smaller due to division
		 * Readable form:
		 * txdur_delta_percent = 100 * (txdur_delta_in_ms / cactime_in_ms)
		 * where
		 *   txdur_delta_in_ms = txdur_delta / 1000
		 *   cactime_in_ms = cactime * WLC_DFS_RADAR_CHECK_INTERVAL
		 */
		dfs->txdur_delta_percent = (txdur_delta / 10) /
				(dfs->dfs_cac.cactime * WLC_DFS_RADAR_CHECK_INTERVAL);
	} else {
		dfs->txdur_delta_percent = 0;
	}

	/* get channel seperation (in MHz) between main core and scan core channels */
	separation = wlc_dfs_get_chan_separation(wlc, WLC_BAND_PI_RADIO_CHANSPEC, dfs->sc_chspec);

	if (bw_flag == WL_CHANSPEC_BW_80) {
		bw_mhz = CHAN_BW_80MHZ;
	} else if (bw_flag == WL_CHANSPEC_BW_40) {
		bw_mhz = CHAN_BW_40MHZ;
	} else if (bw_flag == WL_CHANSPEC_BW_20) {
		bw_mhz = CHAN_BW_20MHZ;
	}
	/* scan channel is adjacent to main if center frequency seperation is within bandwidth */
	adjacent = separation <= bw_mhz;
	/* note if the scan core is operating on a weather radar channel (when in EU) */
	weather = wlc_is_european_weather_radar_channel(wlc, dfs->sc_chspec);

	/* skip threshold checks depending on txblank_check_mode */
	if (adjacent && !check_adj) {
		// skip only when weather check is also irrelevant
		if (!weather || !check_weather) {
			return FALSE;
		}
	}

	if (!adjacent && !check_nonadj) {
		// skip only when weather check is also irrelevant
		if (!weather || !check_weather) {
			return FALSE;
		}
	}

	threshold = wlc_dfs_get_max_safe_tx_threshold(dfs, adjacent, weather);

	/* compare with >= as integer will inherently round down */
	discard = (dfs->txdur_delta_percent >= threshold);

	WL_DFS(("wl%d.. %s txdur start:%uus, end:%uus, delta: %uus or %d%%, "
			"separation: %uMHz, adjacent: %d, weather: %d, th: %d%%, discard: %d\n",
			wlc->pub->unit, __FUNCTION__,
			dfs->txdur_start, txdur_end, txdur_delta, dfs->txdur_delta_percent,
			separation, adjacent, weather, threshold, discard));

	return discard;
}

/*
 * Called on completion of CAC on scan core.
 * If radar was found, initiates upgrade to full MIMO on current channel itself.
 * If radar wasn't found, initiates channel switch which will be followed by upgrade.
 */
static void
wlc_dfs_scan_complete_sc(wlc_dfs_info_t *dfs, int reason, int return_home_chan)
{
	wlc_info_t *wlc = dfs->wlc;
	int err;
	chanspec_t chspec = dfs->sc_chspec;
	bool discard_due_to_txblanking = FALSE;

	dfs->dfs_scan->inprogress = FALSE;

	dfs->phymode = PHYMODE(wlc);
	if (!PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
		return;
	}

	/* do txblanking related checks only when no radar was found and check-mode is non-zero */
	if (reason == DFS_SCAN_S_RADAR_FREE && dfs->txblank_check_mode != 0) {
		discard_due_to_txblanking = wlc_dfs_discard_due_to_txblanking(dfs);
		if (discard_due_to_txblanking) {
			reason = DFS_SCAN_S_SCAN_ABORTED;
		}
	}

	WL_DFS(("wl%d.. %s chan=%x (%d) reason %d\n", wlc->pub->unit, __FUNCTION__,
			chspec, CHSPEC_CHANNEL(chspec), reason));

	/* cac completed. resume normal bss operation */
	if (dfs->scan_both) {
		/* restore phymode as main core channel changes can't be done in 3+1 */
		phy_ac_chanmgr_set_val_phymode((phy_info_t *)WLC_PI(wlc), 0);

		wlc_dfs_cacstate_ism_set(dfs);
	}

	dfs->dfs_scan->status = reason;

	if (reason == DFS_SCAN_S_RADAR_FREE && dfs->move_stunted &&
			wlc_is_edcrs_eu(wlc)) {
		dfs->dfs_cac.status.chanspec_cleared = dfs->dfs_scan->csa.chspec;
		/* clear the channel */
		wlc_clr_quiet_chanspec(wlc->cmi, dfs->dfs_cac.status.chanspec_cleared);
	}
	/* Channel is radar free. */
	if (reason == DFS_SCAN_S_RADAR_FREE && !dfs->move_stunted) {
		int idx;
		wlc_bsscfg_t *apcfg;
		wl_chan_switch_t *csa = &dfs->dfs_scan->csa;

		/* ensure CSA mode is advisory; since move is not initiated due to radar */
		csa->mode = DOT11_CSA_MODE_ADVISORY;
		csa->chspec = chspec;
		FOREACH_UP_AP(wlc, idx, apcfg) {
			if (BCME_OK != (err = wlc_csa_do_channel_switch(wlc->csa, apcfg,
					csa->chspec, csa->mode, csa->count, csa->reg,
					csa->frame_type))) {
				WL_ERROR(("wl%d %s Err in do chanswitch...%d\n", wlc->pub->unit,
						__FUNCTION__, err));
			}
			WL_DFS(("wl%d Started BGDFS CSA...\n", wlc->pub->unit));
		}
	} else {
		if (!dfs->scan_both) {
			wlc_dfs_scan_cleanup(dfs, TRUE, TRUE);
			dfs->dfs_scan->scan_wlc = NULL;
		}
	}
}
#endif /* BGDFS */

/*
 * Called on completion of CAC on main core.
 */
static int
wlc_dfs_scan_complete(wlc_dfs_info_t *dfs, int reason, int return_home_chan)
{
	wlc_info_t *wlc = dfs->wlc;
	int err;
	chanspec_t chspec = WLC_BAND_PI_RADIO_CHANSPEC;

	WL_DFS(("wl%d.. %s chan=%x (%d) reason %d\n", wlc->pub->unit, __FUNCTION__,
			chspec, CHSPEC_CHANNEL(chspec), reason));

	dfs->dfs_scan->status = reason;

	/* Channel is radar free. */
	if (reason == DFS_SCAN_S_RADAR_FREE) {
		int idx;
		wlc_bsscfg_t *apcfg;
		wl_chan_switch_t *csa = &dfs->dfs_scan->csa;
#ifdef RSDB_DFS_SCAN
		wlc = wlc_dfs_other_wlc_if_dual(wlc);
#endif /* RSDB_DFS_SCAN */
		csa->chspec = chspec;
		FOREACH_UP_AP(wlc, idx, apcfg) {
			if (BCME_OK != (err = wlc_csa_do_channel_switch(wlc->csa, apcfg,
					csa->chspec, csa->mode, csa->count, csa->reg,
					csa->frame_type))) {
				WL_ERROR(("wl%d %s Err in do chanswitch...%d\n", wlc->pub->unit,
						__FUNCTION__, err));
				return BCME_ERROR;
			}
			WL_DFS(("wl%d Started DFS CSA...\n", wlc->pub->unit));
		}
	} else {
		wlc_dfs_scan_cleanup(dfs, FALSE, TRUE);
		dfs->dfs_scan->scan_wlc = NULL;
	}

	return BCME_OK;
}

static void
wlc_dfs_scan_cleanup(wlc_dfs_info_t *dfs, bool return_home_chan, bool upgrade)
{
	wlc_info_t *wlc = dfs->wlc;

#ifdef RSDB_DFS_SCAN
	wlc_info_t *other_wlc = wlc_dfs_other_wlc_if_dual(wlc);
	wlc_phy_set_rsdb_dfs_scan_in_progress((phy_info_t *)WLC_PI(wlc), 0);
	/* unset the ucode radar blanking host flags unconditionally */
	wlc_mhf(wlc, MHF4, MHF4_RSDB_DFS_SCAN, 0, WLC_BAND_ALL);
	wlc_mhf(other_wlc, MHF4, MHF4_RSDB_DFS_SCAN, 0, WLC_BAND_ALL);
#endif /* RSDB_DFS_SCAN */

	wlc_set_dfs_cacstate(dfs, OFF, dfs->cfg);

	wlc->pub->_ap = dfs->dfs_scan->_was_ap;

	dfs->dfs_scan->inprogress = FALSE;
	dfs->scan_both = FALSE;

	if (return_home_chan &&
#ifdef BGDFS
			BGDFS_ENAB(wlc->pub) &&
			!PHYMODE_BGDFS_ACTIVE(dfs->phymode) &&
#endif /* BGDFS */
			TRUE) {
		/* Change the channel back to home channel */
		wlc_suspend_mac_and_wait(wlc);
		wlc_set_chanspec(wlc, dfs->dfs_scan->home_chan, CHANSW_APCS);
		wlc_enable_mac(wlc);
	}
#if !defined(BGDFS) && defined(RSDB_DFS_SCAN)
	(void) wlc_mpc_off_req_set(wlc, MPC_OFF_REQ_DFS_SCAN_ACTIVE, 0);
#endif /* !BGDFS && RSDB_DFS_SCAN */

	if (!upgrade)
		return;

#ifdef BGDFS
	if (BGDFS_ENAB(wlc->pub) && dfs->upgrade_pending &&
			dfs->dfs_scan->modeswitch_state != DFS_MODESW_UPGRADE_IN_PROGRESS) {
		dfs->dfs_scan->modeswitch_state = DFS_MODESW_UPGRADE_IN_PROGRESS;
#ifdef WL_MODESW
		if (WLC_MODESW_ENAB(wlc->pub)) {
			(void) wlc_dfs_bg_upgrade_wlc(wlc);
		}
#else
		wlc_dfs_bg_upgrade_phy(dfs);
#endif /* WL_MODESW */
	}
#elif defined(RSDB_DFS_SCAN)
	if (dfs->dfs_scan->modeswitch_state == DFS_MODESW_DOWNGRADE_IN_FINISHED) {
		dfs->dfs_scan->modeswitch_state = DFS_MODESW_UPGRADE_IN_PROGRESS;
		wlc_rsdb_upgrade_wlc(wlc_dfs_other_wlc_if_dual(wlc));
	}
#endif /* BGDFS, RSDB_DFS_SCAN */

}
#endif /* RSDB_DFS_SCAN || BGDFS */

static void
wlc_dfs_watchdog(void *ctx)
{
	wlc_dfs_info_t *dfs = (wlc_dfs_info_t *)ctx;
	wlc_info_t *wlc = dfs->wlc;

	(void)wlc;

	/* Restore channels 30 minutes after radar detect */
	if (WL11H_ENAB(wlc) && dfs->radar) {
		int chan;

		for (chan = 0; chan < MAXCHANNEL; chan++) {
			if (dfs->chan_blocked[chan] &&
			    dfs->chan_blocked[chan] != WLC_CHANBLOCK_FOREVER) {
				dfs->chan_blocked[chan]--;
				if (!dfs->chan_blocked[chan]) {
					WL_REGULATORY(("\t** DFS *** Channel %d is"
					               " clean after 30 minutes\n", chan));
				}
			}
		}
	}
}

/* Nothing to be done for now, Usually none of the bss are up
 * by now.
 */
static int
wlc_dfs_up(void *ctx)
{
	return BCME_OK;
}

static int
wlc_dfs_down(void *ctx)
{
	wlc_dfs_info_t *dfs = (wlc_dfs_info_t *)ctx;
	int callback = 0;

	/* cancel the radar timer */
	if (dfs->dfs_cac.timer_running == TRUE) {
		if (!wlc_dfs_timer_delete(dfs))
			callback = 1;
		dfs->dfs_cac_enabled = FALSE;
	}

	return callback;
}

static int
wlc_dfs_get_dfs_status_all(wlc_dfs_info_t *dfs, uint8 *arg, int len)
{
	int all_min_sz = sizeof(wl_dfs_status_all_t);
	int sub_sz     = sizeof(wl_dfs_sub_status_t);
	int max_num_sub = len <= 0 ? 0 : (1 + (len - all_min_sz)/sub_sz);
	wl_dfs_status_all_t * all = (wl_dfs_status_all_t*) arg;
	wlc_info_t *wlc = dfs->wlc;
	wl_dfs_sub_status_t *sub0;

	dfs->dfs_cac.status.duration =
		(dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
		WLC_DFS_RADAR_CHECK_INTERVAL;

	WL_DFS(("%s: len=%d, all_min_sz=%d, sub_sz=%d, max_num_sub=%d\n",
			__FUNCTION__, len, all_min_sz, sub_sz, max_num_sub));

	if (len < all_min_sz || max_num_sub < 1) {
		return BCME_BUFTOOSHORT;
	}
	all->version = WL_DFS_STATUS_ALL_VERSION;
	all->num_sub_status = 1;
	sub0 = &(all->dfs_sub_status[0]);
	memset(sub0, 0, sizeof(*sub0));
	sub0->state = dfs->dfs_cac.status.state;
	sub0->duration = dfs->dfs_cac.status.duration;
	sub0->chanspec = wlc->home_chanspec;
	sub0->chanspec_last_cleared = dfs->dfs_cac.status.chanspec_cleared;
	sub0->sub_type = 0;
#ifdef BGDFS
	if (BGDFS_ENAB(wlc->pub) && all->num_sub_status < max_num_sub) {
		wl_dfs_sub_status_t *sub1 = sub0 + 1;
		memset(sub1, 0, sizeof(*sub1));
		all->num_sub_status++;
		if (PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
			sub1->state = dfs->dfs_cac.status.state;
			if (dfs->scan_both) {
				sub0->state = WL_DFS_CACSTATE_ISM;
			} else {
				sub0->state = WL_DFS_CACSTATE_IDLE;
			}
			sub1->duration = dfs->dfs_cac.status.duration;
			sub1->chanspec = dfs->sc_chspec;
			sub1->chanspec_last_cleared = 0;
			sub1->sub_type = 1;
		}
	}
#endif /* BGDFS */

	return BCME_OK;
}

#ifdef BCMDBG
static int
wlc_dfs_dump(void *ctx, struct bcmstrbuf *b)
{
	wlc_dfs_info_t *dfs = (wlc_dfs_info_t *)ctx;
	uint32 i;

	bcm_bprintf(b, "radar %d\n", dfs->radar);
	bcm_bprhex(b, "chan_blocked ", TRUE,
	           (uint8 *)dfs->chan_blocked, sizeof(dfs->chan_blocked));
	bcm_bprintf(b, "cactime_pre_ism %u cactime_post_ism %u nop_sec %u ism_monitor %d\n",
	            dfs->dfs_cac.cactime_pre_ism, dfs->dfs_cac.cactime_post_ism,
	            dfs->dfs_cac.nop_sec, dfs->dfs_cac.ism_monitor);
	if (dfs->dfs_cac.chanspec_forced_list) {
		for (i = 0; i < dfs->dfs_cac.chanspec_forced_list->num; i++)
			bcm_bprintf(b, "chanspec_forced %x status %d cactime %u\n",
			            dfs->dfs_cac.chanspec_forced_list->list[i], dfs->dfs_cac.status,
			            dfs->dfs_cac.cactime, dfs->dfs_cac);
	}
	bcm_bprintf(b, "duration %u chanspec_next %x timer_running %d\n",
	            dfs->dfs_cac.duration, dfs->dfs_cac.chanspec_next,
	            dfs->dfs_cac.timer_running);
	bcm_bprintf(b, "dfs_cac_enabled %d\n", dfs->dfs_cac_enabled);

	return BCME_OK;
}
#endif /* BCMDBG */

uint32
wlc_dfs_get_chan_info(wlc_dfs_info_t *dfs, uint channel)
{
	uint32 result;

	result = 0;
	if (dfs->chan_blocked[channel]) {
		int minutes;

		result |= WL_CHAN_INACTIVE;

		/* Store remaining minutes until channel comes
		 * in-service in high 8 bits.
		 */
		minutes = ROUNDUP(dfs->chan_blocked[channel], 60) / 60;
		result |= ((minutes & 0xff) << 24);
	}

	return (result);
}


/*
 * Helper function to use correct pre- and post-ISM CAC time for european weather radar channels
 * which use a different CAC timer (default is 10 minutes for EU weather radar channels, 1 minute
 * for regular radar CAC).
 *
 * Returns cactime in WLC_DFS_RADAR_CHECK_INTERVAL units.
 */
static uint
wlc_dfs_ism_cactime(wlc_info_t *wlc, int secs_or_default)
{
	chanspec_t chspec = WLC_BAND_PI_RADIO_CHANSPEC;

#ifdef BGDFS
	/* override chanspec with scan core's if scan core is active (eg. in 3x1 mode) */
	if (BGDFS_ENAB(wlc->pub) && PHYMODE_BGDFS_ACTIVE(wlc->dfs->phymode)) {
		chspec = wlc->dfs->sc_chspec;
		WL_DFS(("wl%d: since phymode is using scan core with chanspec %04x\n",
				wlc->pub->unit, chspec));
	}
#endif /* BGDFS */

	if (wlc_is_edcrs_eu(wlc) && !wlc_quiet_chanspec(wlc->cmi, chspec)) {
		WL_DFS(("Skip CAC - channel 0x%x is already available (ETSI). Zero duration.\n",
				chspec));
		return 0; /* zero CAC duration; no need of CAC */
	}

	if (secs_or_default == WLC_DFS_CAC_TIME_USE_DEFAULTS)
	{
		if (wlc_is_european_weather_radar_channel(wlc, chspec)) {

			secs_or_default = WLC_DFS_CAC_TIME_SEC_DEF_EUWR;

			WL_DFS(("wl%d: dfs chanspec %04x is european weather radar\n",
				wlc->pub->unit, chspec));
		}
		else {
			secs_or_default = WLC_DFS_CAC_TIME_SEC_DEFAULT;
		}
	}

	WL_DFS(("wl%d: chanspec %04x, %d second CAC time\n",
		wlc->pub->unit, chspec, secs_or_default));

	return (secs_or_default*1000)/WLC_DFS_RADAR_CHECK_INTERVAL;

}

/*
 * Return CAC duration in ms. ASSOC would want to know it.
 */
uint
wlc_dfs_get_cactime_ms(wlc_dfs_info_t *dfs)
{
	ASSERT(dfs);
	return (dfs->dfs_cac.cactime * WLC_DFS_RADAR_CHECK_INTERVAL);
}

static int
BCMATTACHFN(wlc_dfs_timer_init)(wlc_dfs_info_t *dfs)
{
	wlc_info_t* wlc = dfs->wlc;

	dfs->dfs_cac.ism_monitor = FALSE; /* put it to normal mode */

	dfs->dfs_cac.timer_running = FALSE;

	if (!(dfs->dfs_timer = wl_init_timer(wlc->wl, wlc_dfs_cacstate_handler, dfs, "dfs"))) {
		WL_ERROR(("wl%d: wlc_dfs_timer_init failed\n", wlc->pub->unit));
		return -1;
	}
	dfs->dfs_cac.cactime_pre_ism = dfs->dfs_cac.cactime_post_ism
		= WLC_DFS_CAC_TIME_USE_DEFAULTS;   /* use default values */

	dfs->dfs_cac.nop_sec = WLC_DFS_NOP_SEC_DEFAULT;

	return 0;
}

static void
wlc_dfs_timer_add(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;

	if (dfs->dfs_cac.timer_running == FALSE) {
		dfs->dfs_cac.timer_running = TRUE;
		wl_add_timer(wlc->wl, dfs->dfs_timer, WLC_DFS_RADAR_CHECK_INTERVAL, TRUE);
	}
}

static bool
wlc_dfs_timer_delete(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
	bool canceled = TRUE;

	if (dfs->dfs_cac.timer_running == TRUE) {
		if (dfs->dfs_timer != NULL) {
			canceled = wl_del_timer(wlc->wl, dfs->dfs_timer);
			ASSERT(canceled == TRUE);
		}
		dfs->dfs_cac.timer_running = FALSE;
	}
	return canceled;
}

static void
wlc_dfs_chanspec_oos(wlc_dfs_info_t *dfs, chanspec_t chanspec)
{
	wlc_info_t* wlc = dfs->wlc;
	bool is_block = FALSE;
	uint8 ctrl_ch, ext_ch;

	if (VHT_ENAB(wlc->pub) && CHSPEC_IS80(chanspec)) {
		uint channel;
		int i;
		channel = LOWER_40_SB(CHSPEC_CHANNEL(chanspec));
		channel = LOWER_20_SB(channel);

		/* work through each 20MHz channel in the 80MHz */
		for (i = 0; i < 4; i++, channel += CH_20MHZ_APART) {
			dfs->chan_blocked[channel] = dfs->dfs_cac.nop_sec;
			dfs->chan_cac_pending[channel] = TRUE;
			WL_DFS(("wl%d: dfs : channel %d put out of service\n", wlc->pub->unit,
				channel));
		}
		ctrl_ch = CHSPEC_CHANNEL(chanspec);
		ext_ch = 0;
	} else if (N_ENAB(wlc->pub) && CHSPEC_IS40(chanspec)) {
		ctrl_ch = LOWER_20_SB(CHSPEC_CHANNEL(chanspec));
		ext_ch = UPPER_20_SB(CHSPEC_CHANNEL(chanspec));
		dfs->chan_blocked[ctrl_ch] = dfs->dfs_cac.nop_sec;
		dfs->chan_cac_pending[ctrl_ch] = TRUE;
		dfs->chan_blocked[ext_ch] = dfs->dfs_cac.nop_sec;
		dfs->chan_cac_pending[ext_ch] = TRUE;

		WL_DFS(("wl%d: dfs : channel %d & %d put out of service\n", wlc->pub->unit,
			ctrl_ch, ext_ch));
	} else {
		ctrl_ch = CHSPEC_CHANNEL(chanspec);
		ext_ch = 0;
		dfs->chan_blocked[ctrl_ch] = dfs->dfs_cac.nop_sec;
		dfs->chan_cac_pending[ctrl_ch] = TRUE;

		WL_DFS(("wl%d: dfs : channel %d put out of service\n", wlc->pub->unit, ctrl_ch));
	}

	wlc_set_quiet_chanspec(wlc->cmi, chanspec);

	if (!bcmp("US", wlc_channel_country_abbrev(wlc->cmi), 2) ||
		!bcmp("CA", wlc_channel_country_abbrev(wlc->cmi), 2)) {
		if ((ctrl_ch >= 120 && ctrl_ch <= 128) ||
		   (N_ENAB(wlc->pub) && CHSPEC_IS40(chanspec) && ext_ch >= 120 && ext_ch <= 128))
			is_block = TRUE;
	}

	/* Special US and CA handling, remove set of channels 120, 124, 128 if
	 * any get a radar pulse.  For CA they will be blocked for uptime of the driver.
	 */
	if (is_block) {
		uint32  block_time = !bcmp("CA", wlc_channel_country_abbrev(wlc->cmi), 2) ?
		    WLC_CHANBLOCK_FOREVER : dfs->dfs_cac.nop_sec;

		wlc_set_quiet_chanspec(wlc->cmi, CH20MHZ_CHSPEC(120));
		dfs->chan_blocked[120] = block_time;
		wlc_set_quiet_chanspec(wlc->cmi, CH20MHZ_CHSPEC(124));
		dfs->chan_blocked[124] = block_time;
		wlc_set_quiet_chanspec(wlc->cmi, CH20MHZ_CHSPEC(128));
		dfs->chan_blocked[128] = block_time;
	}
}

/* stating from LSB of each octet in bmp of length len, get the position of the n th valid bit
 *	bmp	- bitmap array of octets
 *	len	- length (number of octets in bmp)
 *	n	- n th valid bit's position is required (n must be between 1 and len * 8)
 * Returns the position of the n th valid bit, stating from LSB of each octet in bmp of length len,
 *	returns -1 if not found or on error
 */
static int32 wlc_dfs_get_nth_valid_bit_position(uint8 *bmp, int32 len, int32 n)
{
	int32 i = 0;

	if (bmp == NULL || n < 1 || n > len * NBBY) {
		return -1;
	}
	while (i < len * NBBY && n > 0) {
		if (isset(bmp, i)) {
			if (--n == 0) return i;
		}
		i++;
	}

	return -1;
}

/* Returns a valid random channel or 0 on error */
static chanspec_t
wlc_dfs_chanspec_rand(wlc_dfs_info_t *dfs, uint16 *bw_list, uint16 bw_list_len, bool radar_detected)
{
	wlc_info_t *wlc = dfs->wlc;
	chanvec_t ch_vec;
	chanspec_t def_chspec = wlc->default_bss->chanspec, chspec, first_valid, rand_chspec;
	uint16 def_sb = CHSPEC_CTL_SB(def_chspec), def_bw = CHSPEC_BW(def_chspec),
	       bw_full, bw_sub, ch1_count, ch1, bw_idx;
	wlc_bsscfg_t *cfg = dfs->cfg;
#if defined(WL11AC_80P80)
	uint16 ch2_count, ch2;
#endif /* WL11AC_80P80 */
	uint32 rand_tsf = wlc->clk ? R_REG(wlc->osh, &wlc->regs->u.d11regs.tsf_random) : 0;
	int16 rand_idx;
#if defined(BCMDBG)
	char chanbuf[CHANSPEC_STR_LEN];
#endif /* BCMDBG */

	first_valid = rand_chspec = 0;
	/* get to the first acceptable bw */
	for (bw_idx = 0; bw_idx < bw_list_len && bw_list[bw_idx] != def_bw; bw_idx++) {
		/* NO OP */;
	}

	if (bw_idx == bw_list_len) {
		WL_DFS(("radar: %d, def_chspec: 0x%04x, bw_list[%d]: 0x%x, def_bw: 0x%x\n",
				radar_detected, def_chspec, bw_idx,
				(bw_idx < bw_list_len ? bw_list[bw_idx] : -1),
				def_bw));
		return 0;
	}

	while (bw_idx < bw_list_len && rand_chspec == 0) {
		bw_full = bw_sub = bw_list[bw_idx++];
		if (!wlc_is_valid_bw(wlc, cfg, BAND_5G_INDEX, bw_full)) {
			continue; /* skip unsupported bandwidth */
		}
#if defined(WL11AC_80P80)
		if (bw_full == WL_CHANSPEC_BW_8080) {
			/* for 80p80, look for two 80MHz channels */
			bw_sub = WL_CHANSPEC_BW_80;
		}
#endif /* WL11AC_80P80 */
		memset(&ch_vec, 0, sizeof(ch_vec));
		ch1_count = 0;
		for (ch1 = CH_MAX_2G_CHANNEL + 1; ch1 < MAXCHANNEL; ch1++) {
			chspec = CHBW_CHSPEC(bw_sub, ch1); // | def_sb;
			if (!wlc_dfs_valid_ap_chanspec(wlc, chspec)) {
				continue; /* skip invalid chspec */
			}
			if (first_valid == 0) {
				first_valid = chspec; /* valid but still CAC might be required */
			}
			/* just detected radar? Avoid DFS ch unless (pre)cleared eg. in EDCRS_EU */
			if (radar_detected && wlc_quiet_chanspec(wlc->cmi, chspec)) {
				continue; /* CAC is required; skip unavailable DFS radar channels */
			}
			setbit(ch_vec.vec, ch1);
			ch1_count++;
		}
		if (ch1_count == 0) {
			continue; /* no valid channels of given bandwidth found */
		}
		rand_idx = rand_tsf % ch1_count;
#if defined(WL11AC_80P80)
		if (bw_full == WL_CHANSPEC_BW_8080) {
			if (ch1_count < 2) {
				continue; /* at least two 80MHz ch are required for 80p80 */
			}
			/* reduce rand_idx to make way for ch2; if count is 2, just use both 80 */
			rand_idx = ch1_count == 2 ? 0 : (rand_idx > 1 ? rand_idx -1 : rand_idx);
		}
		ch2_count = ch1_count - rand_idx -1; /* count of valid channels left for ch2 */
#endif /* WL11AC_80P80 */
		/* choose 'rand_idx'th channel */
		ch1 = (uint8) (WL_CHANSPEC_CHAN_MASK & wlc_dfs_get_nth_valid_bit_position(
					ch_vec.vec, sizeof(ch_vec.vec), rand_idx + 1));
		if (ch1 == 0 || ch1 == MAXCHANNEL) {
			continue; /* failed to find an usable channel */
		}
		rand_chspec = CHBW_CHSPEC(bw_full, ch1);
#if defined(WL11AC_80P80)
		if (bw_full == WL_CHANSPEC_BW_8080) {
			uint8 pri20 = ch1 - CH_20MHZ_APART - CH_10MHZ_APART; /* zeroth side band */
			rand_idx += rand_tsf % ch2_count;
			ch2 = (uint8) (WL_CHANSPEC_CHAN_MASK & wlc_dfs_get_nth_valid_bit_position(
						ch_vec.vec, sizeof(ch_vec.vec), rand_idx + 1));
			if (ch2 == 0 || ch2 == MAXCHANNEL) {
				continue; /* second valid channel of 80MHz bw not found */
			}
			rand_chspec = wf_chspec_get8080_chspec(pri20, ch1, ch2);
		}
#endif /* WL11AC_80P80 */

		if (bw_full == def_bw) { /* if the bandwidth matches default BSS channel's */
			rand_chspec |= def_sb; /* we can use the default BSS sideband */
		}
	}

	if (rand_chspec == 0 && first_valid != 0) {
		rand_chspec = first_valid;
	}

	if (rand_chspec == 0) {
		return 0;
	}

#if defined(BCMDBG)
	WL_DFS(("wl%d: %s: dfs selected random chanspec %s (%04x)\n", wlc->pub->unit, __FUNCTION__,
			wf_chspec_ntoa(rand_chspec, chanbuf), rand_chspec));
#endif

	ASSERT(wlc_valid_chanspec_db(wlc->cmi, rand_chspec));

	return rand_chspec;
}

/*
 * Returns first valid (non current) channel from dfs forced channel list,
 * 0 if none found
 */
static chanspec_t
wlc_dfs_valid_forced_chanspec(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
	chanspec_t chspec, first_valid = 0;
	uint i;
	chanspec_t cur_ch = WLC_BAND_PI_RADIO_CHANSPEC;
	chanspec_list_t *forced = dfs->dfs_cac.chanspec_forced_list;

	if (forced == NULL || forced->num < 1) {
		return 0;
	}

	for (i = 0; i < forced->num && i < WL_NUMCHANNELS; i++) {
		chspec = forced->list[i];
		if (((~WL_CHANSPEC_CTL_SB_MASK) & cur_ch) ==
				((~WL_CHANSPEC_CTL_SB_MASK) & chspec)) {
			continue; /* skip since center same as current channel */
		}
		if (wlc_dfs_valid_ap_chanspec(wlc, chspec)) {
			if (first_valid == 0) {
				first_valid = chspec;
			}
			/* CAC isn't required if not marked quiet. eg. non-DFS / (pre)cleared DFS */
			if (!wlc_quiet_chanspec(wlc->cmi, chspec)) {
				return chspec;
			}
		}
	}

	return first_valid;
}

/* Rearranges channel list, bringing valid (non current) channels to head of the list */
static void
wlc_dfs_rearrange_channel_list(wlc_dfs_info_t *dfs, chanspec_list_t *ch_list)
{
	wlc_info_t *wlc = dfs->wlc;
	chanspec_t chspec;
	uint i, valid = 0;
	chanspec_t cur_ch = WLC_BAND_PI_RADIO_CHANSPEC;

	if (ch_list == NULL || ch_list->num < 2) {
		return;
	}

	for (i = 0; i < ch_list->num; i++) {
		chspec = ch_list->list[i];
		if (((~WL_CHANSPEC_CTL_SB_MASK) & cur_ch) ==
				((~WL_CHANSPEC_CTL_SB_MASK) & chspec)) {
			continue; /* skip since center same as current channel */
		}
		if (wlc_dfs_valid_ap_chanspec(wlc, chspec)) {
			/* valid (non-current) channels are move to head of list */
			ch_list->list[i] = ch_list->list[valid];
			ch_list->list[valid++] = chspec;
		}
	}
}

/* Accessor : dfs_bw_list array */
static uint16*
BCMRAMFN(wlc_dfs_get_bw_list)(void)
{
	return wlc_dfs_bw_list;
}

/* Accessor : length of the list of bandwidth */
static uint16
BCMRAMFN(wlc_dfs_get_bw_list_len)(void)
{
	return (sizeof(wlc_dfs_bw_list) / sizeof(wlc_dfs_bw_list[0]));
}

/*
 * Returns a forced channel if valid or does a random channel selection for DFS
 * Returns a valid chanspec of a valid radar free channel, using the AP configuration
 * to choose 20, 40 or 80 MHz bandwidth and side-band
 * When 'radar_detected' is TRUE, avoid DFS channels unless pre-cleared in ETSI
 * Returns 0 if there are no valid radar free channels available
 */
static chanspec_t
wlc_dfs_chanspec(wlc_dfs_info_t *dfs, bool radar_detected)
{
	chanspec_t chspec;

	chspec = wlc_dfs_valid_forced_chanspec(dfs);

	/* return if suitable channel is in forced list */
	if (chspec != 0) {
		return chspec;
	}

	WL_DFS(("no usable channels found in dfs_channel_forced list; going random now\n"));

	/* walk the channels looking for good channels */
	/* When Radar is detected, look for Non-DFS or cleared (EDCRS_EU) channels */
	return wlc_dfs_chanspec_rand(dfs, wlc_dfs_get_bw_list(),
		wlc_dfs_get_bw_list_len(), radar_detected);
}

/* check for a chanspec on which an AP can set up a BSS
 * Returns TRUE if the chanspec is valid for the local, not restricted, and
 * has not been blocked by a recent radar pulse detection.
 * Otherwise will return FALSE.
 */
bool
wlc_dfs_valid_ap_chanspec(wlc_info_t *wlc, chanspec_t chspec)
{
	uint channel;
	wlc_dfs_info_t *dfs = wlc->dfs;

	if (!wlc_valid_chanspec_db(wlc->cmi, chspec) ||
			wlc_restricted_chanspec(wlc->cmi, chspec)) {
		return FALSE;
	}

	if (dfs == NULL) {
		return TRUE;
	}

	FOREACH_20_SB(chspec, channel) {
		if (dfs->chan_blocked[channel]) {
			return FALSE;
		}
	}

	return TRUE;
}

#ifdef SLAVE_RADAR
/*
 * Since a STA does not decide it's own chanspec, this function
 * will help check whether CAC has been done for a given chanspec
 * after Non Occupancy period and whether this chanspec has been
 * cleared for ISM.
 */
bool
wlc_cac_is_clr_chanspec(wlc_dfs_info_t *dfs, chanspec_t chspec)
{
	uint channel = CHSPEC_CHANNEL(chspec);
	wlc_info_t *wlc = dfs->wlc;

	if (!wlc_valid_chanspec_db(wlc->cmi, chspec) ||
	    wlc_restricted_chanspec(wlc->cmi, chspec))
		return FALSE;

	if (CHSPEC_IS80(chspec)) {
		if (dfs->chan_cac_pending[LL_20_SB(channel)] ||
		    dfs->chan_cac_pending[UU_20_SB(channel)] ||
			dfs->chan_cac_pending[LU_20_SB(channel)] ||
		    dfs->chan_cac_pending[UL_20_SB(channel)])
			return FALSE;
	} else if (CHSPEC_IS40(chspec)) {
		if (dfs->chan_cac_pending[LOWER_20_SB(channel)] ||
		    dfs->chan_cac_pending[UPPER_20_SB(channel)])
			return FALSE;
	} else if (dfs->chan_cac_pending[channel]) {
		return FALSE;
	}

	return TRUE;
}

/*
 * Once a CAC was done on given chanspec and no radar found, clear
 * it for ISM availability.
 */
static void
wlc_cac_do_clr_chanspec(wlc_dfs_info_t *dfs, chanspec_t chspec)
{
	uint channel = CHSPEC_CHANNEL(chspec);
	wlc_info_t *wlc = dfs->wlc;

	if (!wlc_valid_chanspec_db(wlc->cmi, chspec) ||
	    wlc_restricted_chanspec(wlc->cmi, chspec))
		return;

	if (CHSPEC_IS80(chspec)) {
		dfs->chan_cac_pending[LL_20_SB(channel)] = FALSE;
		dfs->chan_cac_pending[UU_20_SB(channel)] = FALSE;
		dfs->chan_cac_pending[LU_20_SB(channel)] = FALSE;
		dfs->chan_cac_pending[UL_20_SB(channel)] = FALSE;
		return;
	} else if (CHSPEC_IS40(chspec)) {
		dfs->chan_cac_pending[LOWER_20_SB(channel)] = FALSE;
		dfs->chan_cac_pending[UPPER_20_SB(channel)] = FALSE;
		return;
	} else {
		dfs->chan_cac_pending[channel] = FALSE;
		return;
	}

	return;
}
#endif /* SLAVE_RADAR */

static bool
wlc_radar_detected(wlc_dfs_info_t *dfs, bool scan_core)
{
	wlc_info_t *wlc = dfs->wlc;
	radar_detected_info_t radar_detected;
	uint8 radar_type;
	uint16 radar_interval;
	uint16 min_pw;
	radar_detected_info_t radar_detected_u80;
	uint8 radar_type_u80;
	uint16 radar_interval_u80;
	uint16 min_pw_u80;
#if defined(BCMDBG) || defined(WLMSG_DFS)
	uint i;
	char radar_type_str[24];
	char radar_type_str_u80[24];
	char chanbuf[CHANSPEC_STR_LEN];
	static const struct {
		int radar_type;
		const char *radar_type_name;
	} radar_names[] = {
		{RADAR_TYPE_NONE, "NONE"},
		{RADAR_TYPE_ETSI_0, "ETSI_0"},
		{RADAR_TYPE_ETSI_1, "ETSI_1"},
		{RADAR_TYPE_ETSI_2, "ETSI_2"},
		{RADAR_TYPE_ETSI_3, "ETSI_3"},
		{RADAR_TYPE_ETSI_4, "ETSI_4"},
		{RADAR_TYPE_ETSI_STG2, "ETSI-S2"},
		{RADAR_TYPE_ETSI_STG3, "ETSI-S3"},
		{RADAR_TYPE_ETSI_5_STG2, "ETSI_5-S2"},
		{RADAR_TYPE_ETSI_5_STG3, "ETSI_5-S3"},
		{RADAR_TYPE_ETSI_6_STG2, "ETSI_6-S2"},
		{RADAR_TYPE_ETSI_6_STG3, "ETSI_6-S3"},
		{RADAR_TYPE_FCC_0, "FCC-0"},
		{RADAR_TYPE_FCC_1, "FCC-1"},
		{RADAR_TYPE_FCC_2, "FCC-2"},
		{RADAR_TYPE_FCC_3, "FCC-3"},
		{RADAR_TYPE_FCC_4, "FCC-4"},
		{RADAR_TYPE_FCC_5, "FCC-5"},
		{RADAR_TYPE_FCC_6, "FCC-6"},
		{RADAR_TYPE_JP1_1, "JP1-1"},
		{RADAR_TYPE_JP1_2, "JP1-2"},
		{RADAR_TYPE_JP2_1_1, "JP2-1-1"},
		{RADAR_TYPE_JP2_1_2, "JP2-1-2"},
		{RADAR_TYPE_JP2_2_1, "JP2-2-1"},
		{RADAR_TYPE_JP2_2_2, "JP2-2-2"},
		{RADAR_TYPE_JP2_2_3, "JP2-2-3"},
		{RADAR_TYPE_JP2_2_4, "JP2-2-4"},
		{RADAR_TYPE_JP3, "JP3"},
		{RADAR_TYPE_JP4, "JP4"},
		{RADAR_TYPE_KN1, "KN1"},
		{RADAR_TYPE_KN2, "KN2"},
		{RADAR_TYPE_KN3, "KN3"},
		{RADAR_TYPE_KN4, "KN4"},
		{RADAR_TYPE_UNCLASSIFIED, "UNCLASSIFIED"}
	};
#endif /* BCMDBG || WLMSG_DFS */
	wlc_bsscfg_t *cfg = dfs->cfg;
	int radar_sim_mask = scan_core ? RADAR_SIM_SC : RADAR_SIM;
	chanspec_t chspec = WLC_BAND_PI_RADIO_CHANSPEC;

	BCM_REFERENCE(chspec);
#ifdef BGDFS
	if (!BGDFS_ENAB(wlc->pub) && scan_core) {
		return FALSE;
	}
	dfs->phymode = PHYMODE(wlc);
	if (scan_core) {
		chspec = dfs->sc_chspec;
		if (!PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
			return FALSE;
		}
	}
#else
	if (scan_core) {
		ASSERT(0);
		return FALSE;
	}
#endif /* BGDFS */
	if ((CHSPEC_IS8080(wlc->chanspec) || CHSPEC_IS160(wlc->chanspec)) && scan_core == FALSE) {
		radar_type = phy_radar_detect((phy_info_t *)WLC_PI(wlc),
			&radar_detected, FALSE, TRUE);
		radar_type_u80 =
			phy_radar_detect((phy_info_t *)WLC_PI(wlc),
			&radar_detected_u80, TRUE, TRUE);
		radar_interval_u80 = radar_detected_u80.min_pri;
		BCM_REFERENCE(radar_interval_u80);
		min_pw_u80 = radar_detected_u80.min_pw;
		BCM_REFERENCE(min_pw_u80);
	} else {
		radar_type =
			phy_radar_detect((phy_info_t *)WLC_PI(wlc),
			&radar_detected, scan_core, FALSE);
		radar_type_u80 = 0;
		radar_interval_u80 = 0;
		min_pw_u80 = 0;
	}
	radar_interval = radar_detected.min_pri;
	BCM_REFERENCE(radar_interval);
	min_pw = radar_detected.min_pw;
	BCM_REFERENCE(min_pw);
	/* Pretend we saw radar - for testing */
	if ((wlc_11h_get_spect_state(wlc->m11h, cfg) & radar_sim_mask) ||
	    radar_type != RADAR_TYPE_NONE || radar_type_u80 != RADAR_TYPE_NONE) {
		if (radar_type != RADAR_TYPE_NONE) {
			dfs->radar_detected = radar_detected;
		} else if (radar_type_u80 != RADAR_TYPE_NONE) {
			dfs->radar_detected = radar_detected_u80;
		}
#if defined(BCMDBG) || defined(WLMSG_DFS)
		snprintf(radar_type_str, sizeof(radar_type_str),
			"%s", "UNKNOWN");
		for (i = 0; i < ARRAYSIZE(radar_names); i++) {
			if (radar_names[i].radar_type == radar_type)
				snprintf(radar_type_str, sizeof(radar_type_str),
					"%s", radar_names[i].radar_type_name);
		}

		WL_DFS(("WL%d: DFS: %s ########## RADAR%s DETECTED ON CHANNEL %s"
			" ########## Intv=%dus, min_pw=%d (1/20us), AT %dMS\n", wlc->pub->unit,
			radar_type_str,
			(scan_core ? "_SC" : ""),
			wf_chspec_ntoa(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf),
			radar_interval, min_pw,
			(dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
			WLC_DFS_RADAR_CHECK_INTERVAL));

		snprintf(radar_type_str_u80, sizeof(radar_type_str_u80),
			"%s", "UNKNOWN");
		for (i = 0; i < ARRAYSIZE(radar_names); i++) {
			if (radar_names[i].radar_type == radar_type_u80)
				snprintf(radar_type_str_u80, sizeof(radar_type_str_u80),
					"%s", radar_names[i].radar_type_name);
		}

		WL_DFS(("WL%d: DFS: %s ########## RADAR%s DETECTED ON Upper80 of CHANNEL %s"
			" ########## Intv=%dus, min_pw=%d (1/20us), AT %dMS\n", wlc->pub->unit,
			radar_type_str_u80,
			(scan_core ? "_SC" : ""),
			wf_chspec_ntoa(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf),
			radar_interval_u80, min_pw_u80,
			(dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
			WLC_DFS_RADAR_CHECK_INTERVAL));
#endif /* BCMDBG || WLMSG_DFS */

		/* clear one-shot radar simulator */
		wlc_11h_set_spect_state(wlc->m11h, cfg, radar_sim_mask, 0);
		return TRUE;
	} else
		return FALSE;
}

/* set cacstate to IDLE and un-mute */
static void
wlc_dfs_cacstate_idle_set(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
#if defined(BCMDBG) || defined(WLMSG_DFS)
	char chanbuf[CHANSPEC_STR_LEN];
#endif

	wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_IDLE);
	wlc_mute(wlc, OFF, PHY_MUTE_FOR_PREISM);

	WL_DFS(("wl%d: dfs : state to %s chanspec %s at %dms\n",
		wlc->pub->unit, wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
		wf_chspec_ntoa_ex(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf),
		(dfs->dfs_cac.cactime -
		dfs->dfs_cac.duration)*WLC_DFS_RADAR_CHECK_INTERVAL));

	dfs->dfs_cac.cactime =  /* unit in WLC_DFS_RADAR_CHECK_INTERVAL */
	dfs->dfs_cac.duration = wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_post_ism);
}

static void
wlc_dfs_send_event(wlc_dfs_info_t *dfs, chanspec_t target_chanspec)
{
	wl_event_radar_detect_data_t radar_data;
	wlc_info_t* wlc = dfs->wlc;

	memset(&radar_data, 0, sizeof(wl_event_radar_detect_data_t));
	radar_data.current_chanspec = WLC_BAND_PI_RADIO_CHANSPEC;
	radar_data.target_chanspec = target_chanspec;
	radar_data.radar_info[0].min_pw = dfs->radar_detected.min_pw;
	radar_data.radar_info[0].subband = dfs->radar_detected.subband;
	radar_data.radar_info[0].radar_type = dfs->radar_detected.radar_type;
	wlc_bss_mac_event(wlc, wlc->cfg, WLC_E_RADAR_DETECTED, NULL, 0,
			0, 0, (void *)&radar_data, sizeof(radar_data));
	WL_DFS(("wl%d: DFS WLC_E_RADAR_DETECTED  on ch 0x%02x going to ch 0x%02x\n",
		WLCWLUNIT(wlc),	radar_data.current_chanspec, radar_data.target_chanspec));
}
/* set cacstate to ISM and un-mute */
static void
wlc_dfs_cacstate_ism_set(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
	int  cal_mode;
#if defined(BCMDBG) || defined(WLMSG_DFS)
	char chanbuf[CHANSPEC_STR_LEN];
#endif

	dfs->dfs_cac.status.chanspec_cleared = WLC_BAND_PI_RADIO_CHANSPEC;
	 /* clear the channel */
	wlc_clr_quiet_chanspec(wlc->cmi, dfs->dfs_cac.status.chanspec_cleared);

	wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_ISM);
	wlc_mute(wlc, OFF, PHY_MUTE_FOR_PREISM);

	wlc_iovar_getint(wlc, "phy_percal", (int *)&cal_mode);
	wlc_iovar_setint(wlc, "phy_percal", PHY_PERICAL_SPHASE);
	wlc_phy_cal_perical(WLC_PI(wlc), PHY_PERICAL_UP_BSS);
	wlc_iovar_setint(wlc, "phy_percal", cal_mode);

	WL_DFS(("wl%d: dfs : state to %s chanspec %s at %dms\n",
		wlc->pub->unit, wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
		wf_chspec_ntoa_ex(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf),
		(dfs->dfs_cac.cactime - dfs->dfs_cac.duration) * WLC_DFS_RADAR_CHECK_INTERVAL));

	dfs->dfs_cac.cactime =  /* unit in WLC_DFS_RADAR_CHECK_INTERVAL */
	dfs->dfs_cac.duration = wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_post_ism);
#ifdef SLAVE_RADAR
	/* ISM started, lets prepare for join */
	wlc_join_bss_prep(wlc->cfg);
#endif /* SLAVE_RADAR */

}

/* set cacstate to OOC and mute */
static void
wlc_dfs_cacstate_ooc_set(wlc_dfs_info_t *dfs, uint target_state)
{
	wlc_info_t *wlc = dfs->wlc;

	wlc_mute(wlc, ON, PHY_MUTE_FOR_PREISM);

	wlc_dfs_cac_state_change(dfs, target_state);

	WL_DFS(("wl%d: dfs : state to %s at %dms\n",
		wlc->pub->unit, wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
		(dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
	        WLC_DFS_RADAR_CHECK_INTERVAL));

	dfs->dfs_cac.duration = dfs->dfs_cac.cactime; /* reset it */

}

static void
wlc_dfs_cacstate_idle(wlc_dfs_info_t *dfs)
{
	wlc_dfs_timer_delete(dfs);
	dfs->dfs_cac_enabled = FALSE;
}

#if defined(RSDB_DFS_SCAN) || defined(BGDFS)
/* handles periodic CAC check when DFS scan is active in background */
static void
wlc_dfs_handle_dfs_scan_cacstate(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
#if defined(BCMDBG) || defined(WLMSG_DFS)
	char chanbuf[CHANSPEC_STR_LEN];
#endif

	BCM_REFERENCE(wlc);

	if (!DFS_SCAN_IN_PROGRESS(dfs))
		return;

#ifdef BGDFS
	if (BGDFS_ENAB(wlc->pub) && PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
		if (wlc_radar_detected(dfs, TRUE)) {
			if (dfs->dfs_cac.ism_monitor == TRUE) {
				/* channel switching is disabled */
				WL_DFS(("wl%d: dfs : current chanspec %s is maintained as channel "
						"switching is disabled\n", wlc->pub->unit,
						wf_chspec_ntoa_ex(dfs->sc_chspec, chanbuf)));
			} else {
				wlc_dfs_scan_complete_sc(dfs, DFS_SCAN_S_RADAR_FOUND, TRUE);
				wlc_dfs_chanspec_oos(dfs, dfs->sc_chspec);
			}
		} else if (dfs->scan_both && wlc_radar_detected(dfs, FALSE)) {
			if (dfs->dfs_cac.ism_monitor == TRUE) {
				/* channel switching is disabled */
				WL_DFS(("wl%d: dfs : current chanspec %s is maintained as channel "
						"switching is disabled\n", wlc->pub->unit,
						wf_chspec_ntoa_ex(WLC_BAND_PI_RADIO_CHANSPEC,
								chanbuf)));
			} else { /* Handling Radar detection on 3x3 core */
				wlc_dfs_scan_complete(dfs, DFS_SCAN_S_RADAR_FOUND, TRUE);
				wlc_dfs_to_backup_channel(dfs, TRUE);
				wlc_dfs_cacstate_idle_set(dfs); /* set to IDLE */
				return;
			}
		}
	}
	else
#endif /* BGDFS */
	{
		if (wlc_radar_detected(dfs, FALSE)) {
			wlc_dfs_scan_complete(dfs, DFS_SCAN_S_RADAR_FOUND, TRUE);
			/* TBD If dfs_scan for next channel is required, need to handle that. */
		}
	}

		wlc_dfs_send_event(dfs, dfs->dfs_cac.chanspec_next);

	if (!dfs->dfs_cac.duration) {
#ifdef BGDFS
		if (BGDFS_ENAB(wlc->pub) && PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
			if (dfs->dfs_cac.ism_monitor != TRUE) {
				wlc_dfs_scan_complete_sc(dfs, DFS_SCAN_S_RADAR_FREE, TRUE);
			}
		}
		else
#endif
		{
			wlc_dfs_scan_complete(dfs, DFS_SCAN_S_RADAR_FREE, TRUE);
		}
	}
}
#endif /* RSDB_DFS_SCAN || BGDFS */

static void
wlc_dfs_to_backup_channel(wlc_dfs_info_t *dfs, bool radar_detected)
{
	uint target_state;
	wlc_info_t* wlc = dfs->wlc;
	wlc_bsscfg_t *cfg = dfs->cfg;

	if (radar_detected) {
		wlc_set_quiet_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC);
		wlc_dfs_chanspec_oos(dfs, WLC_BAND_PI_RADIO_CHANSPEC);
	}
	dfs->dfs_cac.chanspec_next = wlc_dfs_chanspec(dfs, radar_detected);

	if (!dfs->dfs_cac.chanspec_next) {
		/* out of channels */
		if (dfs->dfs_cac.status.state == WL_DFS_CACSTATE_PREISM_CAC) {
			target_state = WL_DFS_CACSTATE_PREISM_OOC;
		} else {
			target_state = WL_DFS_CACSTATE_POSTISM_OOC;
		}
		wlc_dfs_cacstate_ooc_set(dfs, target_state);
		return;
	}

	wlc_do_chanswitch(cfg, dfs->dfs_cac.chanspec_next);
}

static void
wlc_dfs_cacstate_cac(wlc_dfs_info_t *dfs)
{
	wlc_info_t* wlc = dfs->wlc;
	wlc_bsscfg_t *cfg = dfs->cfg;
#if defined(BCMDBG) || defined(WLMSG_DFS)
	char chanbuf[CHANSPEC_STR_LEN];
#endif

	(void)cfg;
#if defined(RSDB_DFS_SCAN) || defined(BGDFS)
	if ((WLRSDBDFS_ENAB(wlc->pub) || BGDFS_ENAB(wlc->pub)) && DFS_SCAN_IN_PROGRESS(dfs)) {
		wlc_dfs_handle_dfs_scan_cacstate(dfs);
		return;
	}
#endif /* RSDB_DFS_SCAN || BGDFS */
	/*
	 * If CAC was done earlier and chanspec is clear, STA can go to ISM.
	 */
#ifdef SLAVE_RADAR
	if (WL11H_STA_ENAB(wlc) && wlc_radar_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC) &&
		(wlc_cac_is_clr_chanspec(wlc->dfs, WLC_BAND_PI_RADIO_CHANSPEC)))
	{
		dfs->dfs_cac.cactime = 0;
		dfs->dfs_cac.duration = 0;
	}
#endif /* SLAVE_RADAR */

	if (wlc_radar_detected(dfs, FALSE) == TRUE) {
		wlc_dfs_to_backup_channel(dfs, TRUE);

		if (WL11H_AP_ENAB(wlc)) {
		if (wlc_radar_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC) == TRUE) {
			/* do cac with new channel */
			WL_DFS(("wl%d: dfs : state to %s chanspec %s at %dms\n",
				wlc->pub->unit,
				wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
				wf_chspec_ntoa_ex(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf),
				(dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
			        WLC_DFS_RADAR_CHECK_INTERVAL));
			/* Switched to new channel, set up correct pre-ISM timer */
			dfs->dfs_cac.duration =
			dfs->dfs_cac.cactime =
				wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_pre_ism);
			return;
		}
		else {
			wlc_dfs_cacstate_idle_set(dfs); /* set to IDLE */
			return;
		}
		}
#ifdef SLAVE_RADAR
		else {
			/* radar detected. mark the channel back to QUIET channel */
			wlc_set_quiet_chanspec(wlc->cmi, dfs->dfs_cac.status.chanspec_cleared);
			dfs->dfs_cac.status.chanspec_cleared = 0; /* clear it */
			wlc_dfs_chanspec_oos(dfs, WLC_BAND_PI_RADIO_CHANSPEC);
			/* Radar detected during CAC */
			wlc_assoc_change_state(wlc->cfg, AS_DFS_CAC_FAIL);
			/* Non Occupancy Period begins. */
			wlc_dfs_cacstate_idle_set(dfs); /* set to IDLE */
			return;
		}
#endif /* SLAVE_RADAR */
	}

	if (!dfs->dfs_cac.duration) {
		WL_DFS((" CAC duration 0\n"));
#ifdef SLAVE_RADAR
		if (WL11H_STA_ENAB(wlc) &&
			!wlc_cac_is_clr_chanspec(dfs, WLC_BAND_PI_RADIO_CHANSPEC)) {
			wlc_cac_do_clr_chanspec(dfs, WLC_BAND_PI_RADIO_CHANSPEC);
			/* No radar during CAC */
			wlc_assoc_change_state(wlc->cfg, AS_DFS_ISM_INIT);
			return;
		}
#endif /* SLAVE_RADAR */
		/* cac completed. un-mute all. resume normal bss operation */
		wlc_dfs_cacstate_ism_set(dfs);
	}
}

static void
wlc_dfs_cacstate_ism(wlc_dfs_info_t *dfs)
{
	wlc_info_t* wlc = dfs->wlc;
	wlc_bsscfg_t *cfg = dfs->cfg;
	wl_chan_switch_t csa;
#if defined(BCMDBG) || defined(WLMSG_DFS)
	char chanbuf1[CHANSPEC_STR_LEN];
	char chanbuf2[CHANSPEC_STR_LEN];
#endif


#ifdef BGDFS
	/* after moving to the new channel, modeswitch announcement is initiated */
	if (BGDFS_ENAB(wlc->pub) && dfs->upgrade_pending &&
			dfs->dfs_scan->modeswitch_state != DFS_MODESW_UPGRADE_IN_PROGRESS) {
		int delay = ((dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
				WLC_DFS_RADAR_CHECK_INTERVAL);
		if (delay > DFS_BG_UPGRADE_DELAY_MS) {
			dfs->dfs_scan->modeswitch_state = DFS_MODESW_UPGRADE_IN_PROGRESS;
			// delayed upgrade to full NSS
#ifdef WL_MODESW
			if (WLC_MODESW_ENAB(wlc->pub)) {
				(void) wlc_dfs_bg_upgrade_wlc(wlc);
			}
#else
			wlc_dfs_bg_upgrade_phy(dfs);
#endif /* WL_MODESW */
		}
	}
#endif /* BGDFS */

	if (wlc_radar_detected(dfs, FALSE) == FALSE)
		return;

	/* Ignore radar_detect, if STA conencted to upstream AP on radar channel
	 * and local AP is on same radar channel.
	 */
	if (WLC_APSTA_ON_RADAR_CHANNEL(wlc)) {
		WL_DFS(("wl%d: dfs : radar detected but ignoring,"
			"dfs slave present\n", wlc->pub->unit));
		return;
	}

	/* radar has been detected */

	if (dfs->dfs_cac.ism_monitor == TRUE) {
		/* channel switching is disabled */
		WL_DFS(("wl%d: dfs : current chanspec %s is maintained as channel switching is"
		        " disabled.\n",
		        wlc->pub->unit, wf_chspec_ntoa_ex(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf1)));
		return;
	}

	/* continue with CSA */
	wlc_dfs_chanspec_oos(dfs, WLC_BAND_PI_RADIO_CHANSPEC);
	if (AP_ENAB(wlc->pub)) {
		/* it will be included in csa */
		dfs->dfs_cac.chanspec_next = wlc_dfs_chanspec(dfs, TRUE);

		wlc_dfs_send_event(dfs, dfs->dfs_cac.chanspec_next);
		/* send csa */
		if (!dfs->dfs_cac.chanspec_next) {
			/* out of channels */
			/* just use the current channel for csa */
			csa.chspec = WLC_BAND_PI_RADIO_CHANSPEC;
		} else {
			csa.chspec = dfs->dfs_cac.chanspec_next;
		}
		csa.mode = DOT11_CSA_MODE_NO_TX;
		csa.count = MAX((WLC_DFS_CSA_MSEC/cfg->current_bss->beacon_period),
			WLC_DFS_CSA_BEACONS);
		/* ensure count is at least DTIM of this AP */
		csa.count = MAX(csa.count, cfg->current_bss->dtim_period);
		csa.reg = wlc_get_regclass(wlc->cmi, csa.chspec);
		csa.frame_type = CSA_BROADCAST_ACTION_FRAME;
		wlc_dfs_csa_each_up_ap(wlc, &csa, FALSE);
	} /* AP_ENAB() */
#ifdef SLAVE_RADAR
	else {
		/* Slave detected radar */
		wlc_disassociate_client(wlc->cfg, FALSE);
		wlc_roamscan_start(wlc->cfg, WLC_E_REASON_RADAR_DETECTED);
	}
#endif /* SLAVE_RADAR */
	if (WL11H_AP_ENAB(wlc))
		wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_CSA);        /* next state */
#ifdef SLAVE_RADAR
	else
	/*
	 * Thats what we want to do for a Slave, as we don't decide our
	 * own chanspec
	 */
		wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_IDLE);        /* next state */
#endif /* SLAVE_RADAR */

	WL_DFS(("wl%d: dfs : state to %s chanspec current %s next %s at %dms, starting CSA"
		" process\n",
		wlc->pub->unit, wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
		wf_chspec_ntoa_ex(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf1),
		wf_chspec_ntoa_ex(csa.chspec, chanbuf2),
		(dfs->dfs_cac.cactime -
			dfs->dfs_cac.duration)*WLC_DFS_RADAR_CHECK_INTERVAL));

	dfs->dfs_cac.duration = dfs->dfs_cac.cactime =
		wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_post_ism);
}

/* csa transmission */
static void
wlc_dfs_cacstate_csa(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
	wlc_bsscfg_t *cfg = dfs->cfg;

	if ((wlc_11h_get_spect_state(wlc->m11h, cfg) &
	     (NEED_TO_SWITCH_CHANNEL | NEED_TO_UPDATE_BCN)) ||
	    (wlc->block_datafifo & DATA_BLOCK_QUIET))
	        return;

	/* csa completed - TBTT dpc switched channel */

	if (!(dfs->dfs_cac.chanspec_next)) {
	        /* ran out of channels, goto OOC */
	        wlc_dfs_cacstate_ooc_set(dfs, WL_DFS_CACSTATE_POSTISM_OOC);
		return;
	}

	if (wlc_radar_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC) == TRUE) {
		if (dfs->dfs_cac.cactime_post_ism) {
			wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_POSTISM_CAC);
			WL_DFS(("wl%d: dfs : state to %s at %dms\n",
				wlc->pub->unit,
				wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
			        (dfs->dfs_cac.cactime - dfs->dfs_cac.duration) *
			        WLC_DFS_RADAR_CHECK_INTERVAL));

			dfs->dfs_cac.duration =
			dfs->dfs_cac.cactime =
				wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_post_ism);
				wlc_mute(wlc, ON, PHY_MUTE_FOR_PREISM);
		}
		else {
			wlc_dfs_cacstate_ism_set(dfs);
		}
	}
	else {
		wlc_dfs_cacstate_idle_set(dfs);
	}
	if (AP_ENAB(wlc->pub)) {
		wlc_update_beacon(wlc);
		wlc_update_probe_resp(wlc, TRUE);
	}
}


/*
 * dfs has run Out Of Channel.
 * wait for a channel to come out of Non-Occupancy Period.
 */
static void
wlc_dfs_cacstate_ooc(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
	uint    current_time;
	wlc_bsscfg_t *cfg = dfs->cfg;
#if defined(BCMDBG) || defined(WLMSG_DFS)
	char chanbuf[CHANSPEC_STR_LEN];
#endif

	if (!(dfs->dfs_cac.chanspec_next = wlc_dfs_chanspec(dfs, FALSE))) {
		/* still no channel out of channels. Nothing to do */
		return;
	}

	wlc_dfs_send_event(dfs, dfs->dfs_cac.chanspec_next);
	wlc_do_chanswitch(cfg, dfs->dfs_cac.chanspec_next);

	if (wlc_radar_chanspec(wlc->cmi, WLC_BAND_PI_RADIO_CHANSPEC) == TRUE) {
		current_time = (dfs->dfs_cac.cactime -
			dfs->dfs_cac.duration)*WLC_DFS_RADAR_CHECK_INTERVAL;
		BCM_REFERENCE(current_time);

		/* unit of cactime is WLC_DFS_RADAR_CHECK_INTERVAL */
		if (dfs->dfs_cac.status.state == WL_DFS_CACSTATE_PREISM_OOC) {
			dfs->dfs_cac.cactime = dfs->dfs_cac.duration =
				wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_pre_ism);
			wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_PREISM_CAC);
		} else {
			dfs->dfs_cac.cactime = dfs->dfs_cac.duration =
				wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_post_ism);
			wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_POSTISM_CAC);
		}

		if (dfs->dfs_cac.cactime) {
			wlc_mute(wlc, ON, PHY_MUTE_FOR_PREISM);

			WL_DFS(("wl%d: dfs : state to %s chanspec %s at %dms\n",
				wlc->pub->unit,
				wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
				wf_chspec_ntoa_ex(WLC_BAND_PI_RADIO_CHANSPEC, chanbuf),
				current_time));
		} else {
			/* corresponding cac is disabled */
			wlc_dfs_cacstate_ism_set(dfs);
		}
	} else {
		wlc_dfs_cacstate_idle_set(dfs); /* set to idle */
	}
}

static void
wlc_dfs_cacstate_handler(void *arg)
{
	wlc_dfs_info_t *dfs = (wlc_dfs_info_t *)arg;
	wlc_info_t *wlc = dfs->wlc;

	if (!wlc->pub->up || !dfs->dfs_cac_enabled)
		return;

	if (DEVICEREMOVED(wlc)) {
		WL_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		wl_down(wlc->wl);
		return;
	}


	ASSERT(dfs->dfs_cac.status.state < WL_DFS_CACSTATES);

	wlc_dfs_cacstate_fn_ary[dfs->dfs_cac.status.state](dfs);

	dfs->dfs_cac.duration--;
}

static void
wlc_dfs_cacstate_init(wlc_dfs_info_t *dfs)
{
	wlc_info_t *wlc = dfs->wlc;
	int skip_pre_ism = FALSE;
	chanspec_t chspec = WLC_BAND_PI_RADIO_CHANSPEC;
	char chanbuf[CHANSPEC_STR_LEN];

#ifdef SLAVE_RADAR
	ASSERT(WL11H_AP_ENAB(wlc) || WL11H_STA_ENAB(wlc));
#else
	ASSERT(WL11H_AP_ENAB(wlc));
#endif /* SLAVE_RADAR */

	BCM_REFERENCE(chanbuf);

	if (!wlc->pub->up)
		return;

#ifdef BGDFS
	/* override chanspec with scan core's if scan core is active (eg. in 3+1 mode) */
	if (BGDFS_ENAB(wlc->pub) && dfs->dfs_scan->status == DFS_SCAN_S_INPROGESS &&
			((dfs->phymode = PHYMODE(wlc)) == PHYMODE_BGDFS ||
			PHYMODE_BGDFS_ACTIVE(dfs->phymode))) {
		if (wlc_radar_chanspec(wlc->cmi, dfs->dfs_cac.status.chanspec_cleared) == TRUE) {
			/* restore QUIET setting unless in EU */
			if (!wlc_is_edcrs_eu(wlc)) {
				wlc_set_quiet_chanspec(wlc->cmi,
						dfs->dfs_cac.status.chanspec_cleared);
			}
		}
		chspec = dfs->sc_chspec;
		wlc_dfs_csa_reset_each_ap(wlc);
		wlc_dfs_quiet_reset_each_ap(wlc);
		if (wlc_radar_chanspec(wlc->cmi, chspec) != TRUE) {
			wlc_dfs_cacstate_idle_set(dfs); /* set to idle */
			return;
		}
		dfs->dfs_cac_enabled = TRUE;
		wlc_dfs_timer_add(dfs);
		/* unit of cactime is WLC_DFS_RADAR_CHECK_INTERVAL */
		dfs->dfs_cac.cactime = wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_pre_ism);
		dfs->dfs_scan->status = DFS_SCAN_S_INPROGESS;
		wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_PREISM_CAC);
		dfs->dfs_cac.duration = dfs->dfs_cac.cactime;
		// wlc_dfs_cacstate_ism_set(dfs);
		wlc_radar_detected(dfs, TRUE); /* refresh detector */
		WL_REGULATORY(("wl%d: %s: state to %s chanspec %s BGDFS\n",
				wlc->pub->unit, __FUNCTION__,
				wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
				wf_chspec_ntoa_ex(chspec, chanbuf)));
		return;
	}
#endif /* BGDFS */

	if (wlc_radar_chanspec(wlc->cmi, dfs->dfs_cac.status.chanspec_cleared) == TRUE) {
		/* restore QUIET setting unless in EU */
		if (!wlc_is_edcrs_eu(wlc)) {
			wlc_set_quiet_chanspec(wlc->cmi, dfs->dfs_cac.status.chanspec_cleared);
		}
	}
	dfs->dfs_cac.status.chanspec_cleared = 0; /* clear it */

	wlc_dfs_csa_reset_each_ap(wlc);
	wlc_dfs_quiet_reset_each_ap(wlc);

	if (wlc_radar_chanspec(wlc->cmi, chspec) == TRUE) {
		dfs->dfs_cac_enabled = TRUE;
		wlc_dfs_timer_add(dfs);
#if defined(RSDB_DFS_SCAN) || defined(BGDFS)
		if (((RSDB_ENAB(wlc->pub) && WLRSDBDFS_ENAB(wlc->pub)) || BGDFS_ENAB(wlc->pub)) &&
			dfs->dfs_scan->status == DFS_SCAN_S_RADAR_FREE &&
			dfs->dfs_scan->csa.chspec == chspec)
		{
			skip_pre_ism = TRUE;
			dfs->dfs_scan->status = DFS_SCAN_S_IDLE;
		}
#endif /* RSDB_DFS_SCAN || BGDFS */
		/* unit of cactime is WLC_DFS_RADAR_CHECK_INTERVAL */
		dfs->dfs_cac.cactime = wlc_dfs_ism_cactime(wlc, dfs->dfs_cac.cactime_pre_ism);
		if (!WLC_APSTA_ON_RADAR_CHANNEL(wlc) && dfs->dfs_cac.cactime && !skip_pre_ism) {
			/* preism cac is enabled */
			wlc_dfs_cac_state_change(dfs, WL_DFS_CACSTATE_PREISM_CAC);
			dfs->dfs_cac.duration = dfs->dfs_cac.cactime;
			wlc_mute(wlc, ON, PHY_MUTE_FOR_PREISM);
		} else {
			/* preism cac is disabled */
			wlc_dfs_cacstate_ism_set(dfs);
		}

		(void) wlc_radar_detected(dfs, FALSE); /* refresh detector */

	} else {
		wlc_dfs_cacstate_idle_set(dfs); /* set to idle */
	}

	WL_REGULATORY(("wl%d: %s: state to %s chanspec %s NOT BGDFS\n",
		wlc->pub->unit, __FUNCTION__,
		wlc_dfs_cacstate_str(dfs->dfs_cac.status.state),
		wf_chspec_ntoa_ex(chspec, chanbuf)));
}

void
wlc_set_dfs_cacstate(wlc_dfs_info_t *dfs, int state, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = dfs->wlc;

	(void)wlc;

	if (cfg != NULL) {
		dfs->cfg = cfg; /* update DFS bsscfg */
	}

	ASSERT(dfs->cfg);

	WL_REGULATORY(("wl%d: %s dfs from %s to %s on channel 0x%x\n",
		wlc->pub->unit, __FUNCTION__, dfs->dfs_cac_enabled ? "ON":"OFF",
		state ? "ON":"OFF", WLC_BAND_PI_RADIO_CHANSPEC));

	if (state == OFF) {
		if (dfs->dfs_cac_enabled) {
			wlc_dfs_cacstate_idle_set(dfs);
			wlc_dfs_cacstate_idle(dfs);
		}
	} else {
		/* start CAC unless the chanspec isn't valid now (eg. when marked inactive) */
		if (!wlc_dfs_valid_ap_chanspec(wlc, WLC_BAND_PI_RADIO_CHANSPEC) &&
				!DFS_SCAN_IN_PROGRESS(dfs)) {
			WL_REGULATORY(("wl%d: %s dfs avoiding channel 0x%x; finding backup\n",
					wlc->pub->unit, __FUNCTION__,
					WLC_BAND_PI_RADIO_CHANSPEC));
			wlc_dfs_to_backup_channel(dfs, FALSE);
		} else {
			wlc_dfs_cacstate_init(dfs);
		}
	}
}

chanspec_t
wlc_dfs_sel_chspec(wlc_dfs_info_t *dfs, bool force, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = dfs->wlc;
	char chanbuf[CHANSPEC_STR_LEN];

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(chanbuf);

	if (!force && dfs->dfs_cac.chanspec_next != 0)
		return dfs->dfs_cac.chanspec_next;

	if (cfg != NULL) {
		dfs->cfg = cfg; /* update DFS bsscfg */
	}

	ASSERT(dfs->cfg);

	dfs->dfs_cac.chanspec_next = wlc_dfs_chanspec(dfs, FALSE);

	WL_REGULATORY(("wl%d: %s: dfs selected chanspec %s\n",
	               wlc->pub->unit, __FUNCTION__,
	               wf_chspec_ntoa_ex(dfs->dfs_cac.chanspec_next, chanbuf)));

	return dfs->dfs_cac.chanspec_next;
}

void
wlc_dfs_reset_all(wlc_dfs_info_t *dfs)
{
	bzero(dfs->chan_blocked, sizeof(dfs->chan_blocked));
	bzero(dfs->chan_cac_pending, sizeof(dfs->chan_cac_pending));
}

int
wlc_dfs_set_radar(wlc_dfs_info_t *dfs, int radar)
{
	wlc_info_t *wlc = dfs->wlc;
	wlcband_t *band5G = NULL;
	int radar_max = (int)WL_RADAR_SIMULATED;
	chanspec_t chanspec = wlc->home_chanspec;
	int spec_state = RADAR_SIM;
	wlc_bsscfg_t *cfg = dfs->cfg;

#ifdef BGDFS
	if (BGDFS_ENAB(wlc->pub)) {
		/* override range max, chanspec and spec state */
		radar_max = (int)WL_RADAR_SIMULATED_SC;
		if (radar == (int)WL_RADAR_SIMULATED_SC) {
			WL_DFS(("wl%d: %s simulation requested on scan core\n",
					wlc->pub->unit, __FUNCTION__));
			if (!PHYMODE_BGDFS_ACTIVE(dfs->phymode)) {
				WL_DFS(("wl%d: %s rejecting one shot radar simulation "
						"as scan core is not active\n",
						wlc->pub->unit, __FUNCTION__));
				return BCME_NOTREADY;
			}
			chanspec = dfs->sc_chspec;
			spec_state = RADAR_SIM_SC;
		}
	}
#endif /* BGDFS */

	if (radar < 0 || radar > radar_max) {
		return BCME_RANGE;
	}

	/*
	 * WL_RADAR_SIMULATED / WL_RADAR_SIMULATED_SC are used for Wi-Fi testing.
	 */
	if (radar == (int)WL_RADAR_SIMULATED || radar == (int)WL_RADAR_SIMULATED_SC) {
		/* Radar must be enabled to pull test trigger */
		if (dfs->radar != 1) {
			WL_DFS(("wl%d: %s rejecting one shot radar simulation "
					"as radar detection is not enabled\n",
					wlc->pub->unit, __FUNCTION__));
			return BCME_BADARG;
		}
		/* Can't do radar detect on non-radar channel */
		if (wlc_radar_chanspec(wlc->cmi, chanspec) != TRUE) {
			WL_DFS(("wl%d: %s rejecting one shot radar simulation "
					"as chanspec (0x%x) is not a DFS channel\n",
					wlc->pub->unit, __FUNCTION__, chanspec));
			return BCME_BADCHAN;
		}

		wlc_11h_set_spect_state(wlc->m11h, cfg, spec_state, spec_state);
		WL_DFS(("wl%d: %s enabled one shot radar simulation on %s core\n",
				wlc->pub->unit, __FUNCTION__,
				(radar == (int)WL_RADAR_SIMULATED_SC)?"scan":"main"));
		return BCME_OK;
	}

	if ((int)dfs->radar == radar) {
		return BCME_OK;
	}

	/* Check there is a 5G band available */
	if (BAND_5G(wlc->band->bandtype)) {
		band5G = wlc->band;
	} else if (NBANDS(wlc) > 1 &&
			BAND_5G(wlc->bandstate[OTHERBANDUNIT(wlc)]->bandtype)) {
		band5G = wlc->bandstate[OTHERBANDUNIT(wlc)];
	}

	if (band5G == NULL) {
		return BCME_BADBAND;
	}

	dfs->radar = (uint32)radar;

	phy_radar_detect_enable((phy_info_t *)WLC_PI(wlc), radar != 0);

	/* if we are not currently on the APhy, then radar detect
	 * will be initialized in the phy init
	 */

	WL_DFS(("wl%d: %s DFS radar=%d\n",
			wlc->pub->unit, __FUNCTION__, dfs->radar));
	return BCME_OK;
}

uint32
wlc_dfs_get_radar(wlc_dfs_info_t *dfs)
{
	WL_DFS(("wl%d: %s DFS radar=%d\n",
			dfs->wlc->pub->unit, __FUNCTION__, dfs->radar));
	return dfs->radar;
}

#ifdef STA
#ifdef BCMDBG
static int
wlc_dfs_bcn_parse_ibss_dfs_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	return BCME_OK;
}
#endif /* BCMDBG */
#endif /* STA */
#endif /* WLDFS */
