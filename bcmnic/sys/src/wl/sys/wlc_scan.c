/*
 * Common (OS-independent) portion of
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_scan.c 612103 2016-01-13 01:34:26Z $
 */



#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.1d.h>
#include <proto/802.11.h>
#include <proto/802.11e.h>
#include <proto/wpa.h>
#include <proto/vlan.h>
#include <sbconfig.h>
#include <pcicfg.h>
#include <bcmsrom.h>
#include <wlioctl.h>
#include <epivers.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc_channel.h>
#include <wlc_scandb.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_tx.h>
#include <wlc_phy_hal.h>
#include <phy_utils_api.h>
#include <wlc_event.h>
#include <wl_export.h>
#include <wlc_rm.h>
#include <wlc_ap.h>
#include <wlc_assoc.h>
#include <wlc_scan_priv.h>
#include <wlc_scan.h>
#ifdef WLP2P
#include <wlc_p2p.h>
#endif
#include <wlc_11h.h>
#include <wlc_11d.h>
#include <wlc_dfs.h>
#ifdef ANQPO
#include <wl_anqpo.h>
#endif
#include <wlc_hw_priv.h>
#ifdef WL11K
#include <wlc_rrm.h>
#endif /* WL11K */
#include <wlc_obss.h>
#include <wlc_objregistry.h>
#ifdef WLRSDB
#include <wlc_rsdb.h>
#endif /* WLRSDB */
#ifdef WL_EXCESS_PMWAKE
#include <wlc_pm.h>
#endif /* WL_EXCESS_PMWAKE */
#ifdef WLPFN
#include <wl_pfn.h>
#endif
#include <wlc_bmac.h>
#include <wlc_lq.h>
#ifdef WLSCAN_PS
#include <wlc_stf.h>
#endif
#ifdef WLMESH
#include <wlc_mesh.h>
#include <wlc_mesh_peer.h>
#endif
#include <wlc_ulb.h>
#include <wlc_utils.h>
#include <wlc_msch.h>
#include <wlc_event_utils.h>
/* TODO: remove wlc_scan_utils.h dependency */
#include <wlc_scan_utils.h>
#include <wlc_dump.h>

#if defined(WLMSG_INFORM)
#define	WL_INFORM_SCAN(args)									\
	do {										\
		if ((wl_msg_level & WL_INFORM_VAL) || (wl_msg_level2 & WL_SCAN_VAL))	\
		    WL_PRINT(args);					\
	} while (0)
#undef WL_INFORM_ON
#define WL_INFORM_ON()	((wl_msg_level & WL_INFORM_VAL) || (wl_msg_level2 & WL_SCAN_VAL))
#else
#define	WL_INFORM_SCAN(args)
#endif 


/* scan times in milliseconds */
#define WLC_SCAN_MIN_PROBE_TIME	10	/* minimum useful time for an active probe */
#define WLC_SCAN_HOME_TIME	45	/* time for home channel processing */
#define WLC_SCAN_ASSOC_TIME 20 /* time to listen on a channel for probe resp while associated */

#ifdef BCMQT_CPU
#define WLC_SCAN_UNASSOC_TIME 400	/* qt is slow */
#else
#define WLC_SCAN_UNASSOC_TIME 40	/* listen on a channel for prb rsp while unassociated */
#endif
#define WLC_SCAN_NPROBES	2	/* do this many probes on each channel for an active scan */

/* scan_pass state values */

/* Enables the iovars */
#define WLC_SCAN_IOVARS

#if defined(BRINGUP_BUILD)
#undef WLC_SCAN_IOVARS
#endif
#ifdef WLOLPC
#include <wlc_olpc_engine.h>
#endif /* WLOLPC */

static void wlc_scan_watchdog(void *hdl);

static void wlc_scan_channels(scan_info_t *scan_info, chanspec_t *chanspec_list,
	int *pchannel_num, int channel_max, chanspec_t chanspec_start, int channel_type,
	int band);

static void wlc_scantimer(void *arg);

static int scanmac_get(scan_info_t *scan_info, void *params, uint p_len, void *arg, int len);
static int scanmac_set(scan_info_t *scan_info, void *arg, int len);

static
int _wlc_scan(
	wlc_scan_info_t *wlc_scan_info,
	int bss_type,
	const struct ether_addr* bssid,
	int nssid,
	wlc_ssid_t *ssid,
	int scan_type,
	int nprobes,
	int active_time,
	int passive_time,
	int home_time,
	const chanspec_t* chanspec_list, int channel_num, chanspec_t chanspec_start,
	bool save_prb,
	scancb_fn_t fn, void* arg,
	int away_channels_limit,
	bool extdscan,
	bool suppress_ssid,
	bool include_cache,
	uint scan_flags,
	wlc_bsscfg_t *cfg,
	actcb_fn_t act_cb, void *act_cb_arg, int bandinfo, chanspec_t band_chanspec_start,
	struct ether_addr *sa_override);

#ifdef WLRSDB
static void wlc_parallel_scan_cb(void *arg, int status, wlc_bsscfg_t *cfg);
static int wlc_scan_split_channels_per_band(scan_info_t *scan_info,
	const chanspec_t* chanspec_list, chanspec_t chanspec_start, int chan_num,
	chanspec_t** chanspec_list2g, chanspec_t** chanspec_list5g, chanspec_t *chanspec_start_2g,
	chanspec_t *chanspec_start_5g, int *channel_num2g, int *channel_num5g);

#endif /* WLRSDB */

#ifdef WLC_SCAN_IOVARS
static int wlc_scan_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid,
	const char *name, void *params, uint p_len, void *arg,
	int len, int val_size, struct wlc_if *wlcif);
#endif /* WLC_SCAN_IOVARS */

static void wlc_scan_callback(scan_info_t *scan_info, uint status);
static int wlc_scan_apply_scanresults(scan_info_t *scan_info, int status);

static uint wlc_scan_prohibited_channels(scan_info_t *scan_info,
	chanspec_t *chanspec_list, int channel_max);
static void wlc_scan_do_pass(scan_info_t *scan_info, chanspec_t chanspec);
static void wlc_scan_sendprobe(scan_info_t *scan_info);
static void wlc_scan_ssidlist_free(scan_info_t *scan_info);
static int wlc_scan_chnsw_clbk(void* handler_ctxt, wlc_msch_cb_info_t *cb_info);
static int _wlc_scan_schd_channels(wlc_scan_info_t *wlc_scan_info);

#if defined(WLMSG_INFORM)
static void wlc_scan_print_ssids(wlc_ssid_t *ssid, int nssid);
#endif

#ifdef WL11N
static void wlc_ht_obss_scan_update(scan_info_t *scan_info, int status);
#else /* WL11N */
#define wlc_ht_obss_scan_update(a, b) do {} while (0)
#endif /* WL11N */

#ifdef WLSCANCACHE
static void wlc_scan_merge_cache(scan_info_t *scan_info, uint current_timestamp,
                                 const struct ether_addr *BSSID, int nssid, const wlc_ssid_t *SSID,
                                 int BSS_type, const chanspec_t *chanspec_list, uint chanspec_num,
                                 wlc_bss_list_t *bss_list);
static void wlc_scan_fill_cache(scan_info_t *scan_info, uint current_timestamp);
static void wlc_scan_build_cache_list(void *arg1, void *arg2, uint timestamp,
                                      struct ether_addr *BSSID, wlc_ssid_t *SSID,
                                      int BSS_type, chanspec_t chanspec, void *data, uint datalen);
static void wlc_scan_cache_result(scan_info_t *scan_info);
#else
#define wlc_scan_fill_cache(si, ts)	do {} while (0)
#define wlc_scan_merge_cache(si, ts, BSSID, nssid, SSID, BSS_type, c_list, c_num, bss_list)
#define wlc_scan_cache_result(si)
#endif

#ifdef WLSCAN_PS
static int wlc_scan_ps_config_cores(scan_info_t *scan_info, bool flag);
#endif

static void wlc_scan_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
static bool wlc_scan_usage_scan(wlc_scan_info_t *scan);

static void wlc_scan_excursion_end(scan_info_t *scan_info);

#define CHANNEL_PASSIVE_DWELLTIME(s) ((s)->passive_time)
#define CHANNEL_ACTIVE_DWELLTIME(s) ((s)->active_time)
#define SCAN_INVALID_DWELL_TIME		0
#define SCAN_PROBE_TXRATE(scan_info)	0

#ifdef WLC_SCAN_IOVARS
/* IOVar table */

/* Parameter IDs, for use only internally to wlc -- in the wlc_iovars
 * table and by the wlc_doiovar() function.  No ordering is imposed:
 * the table is keyed by name, and the function uses a switch.
 */
enum {
	IOV_PASSIVE = 1,
	IOV_SCAN_ASSOC_TIME,
	IOV_SCAN_UNASSOC_TIME,
	IOV_SCAN_PASSIVE_TIME,
	IOV_SCAN_HOME_TIME,
	IOV_SCAN_NPROBES,
	IOV_SCAN_EXTENDED,
	IOV_SCAN_NOPSACK,
	IOV_SCANCACHE,
	IOV_SCANCACHE_TIMEOUT,
	IOV_SCANCACHE_CLEAR,
	IOV_SCAN_FORCE_ACTIVE,	/* force passive to active conversion in radar/restricted channel */
	IOV_SCAN_ASSOC_TIME_DEFAULT,
	IOV_SCAN_UNASSOC_TIME_DEFAULT,
	IOV_SCAN_PASSIVE_TIME_DEFAULT,
	IOV_SCAN_HOME_TIME_DEFAULT,
	IOV_SCAN_DBG,
	IOV_SCAN_TEST,
	IOV_SCAN_HOME_AWAY_TIME,
	IOV_SCAN_RSDB_PARALLEL_SCAN,
	IOV_SCAN_RX_PWRSAVE,	/* reduce rx chains for pwr optimization */
	IOV_SCAN_TX_PWRSAVE,	/* reduce tx chains for pwr optimization */
	IOV_SCAN_PWRSAVE,	/* turn on/off bith tx and rx for single core scanning  */
	IOV_SCANMAC,
	IOV_LAST 		/* In case of a need to check max ID number */
};

/* AP IO Vars */
static const bcm_iovar_t wlc_scan_iovars[] = {
	{"passive", IOV_PASSIVE,
	(IOVF_NTRL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
#ifdef STA
	{"scan_assoc_time", IOV_SCAN_ASSOC_TIME,
	(IOVF_NTRL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
	{"scan_unassoc_time", IOV_SCAN_UNASSOC_TIME,
	(IOVF_NTRL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
#endif /* STA */
	{"scan_passive_time", IOV_SCAN_PASSIVE_TIME,
	(IOVF_WHL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
	/* unlike the other scan times, home_time can be zero */
	{"scan_home_time", IOV_SCAN_HOME_TIME,
	(IOVF_WHL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
#ifdef STA
	{"scan_nprobes", IOV_SCAN_NPROBES,
	(IOVF_NTRL|IOVF_OPEN_ALLOW), IOVT_INT8, 0
	},
	{"scan_force_active", IOV_SCAN_FORCE_ACTIVE,
	0, IOVT_BOOL, 0
	},
#ifdef WLSCANCACHE
	{"scancache", IOV_SCANCACHE,
	(IOVF_OPEN_ALLOW), IOVT_BOOL, 0
	},
	{"scancache_timeout", IOV_SCANCACHE_TIMEOUT,
	(IOVF_OPEN_ALLOW), IOVT_INT32, 0
	},
	{"scancache_clear", IOV_SCANCACHE_CLEAR,
	(IOVF_OPEN_ALLOW), IOVT_VOID, 0
	},
#endif /* WLSCANCACHE */
#endif /* STA */
#ifdef STA
	{"scan_assoc_time_default", IOV_SCAN_ASSOC_TIME_DEFAULT,
	(IOVF_NTRL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
	{"scan_unassoc_time_default", IOV_SCAN_UNASSOC_TIME_DEFAULT,
	(IOVF_NTRL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
#endif /* STA */
	{"scan_passive_time_default", IOV_SCAN_PASSIVE_TIME_DEFAULT,
	(IOVF_NTRL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
	/* unlike the other scan times, home_time can be zero */
	{"scan_home_time_default", IOV_SCAN_HOME_TIME_DEFAULT,
	(IOVF_WHL|IOVF_OPEN_ALLOW), IOVT_UINT16, 0
	},
#ifdef STA
	{"scan_home_away_time", IOV_SCAN_HOME_AWAY_TIME, (IOVF_WHL), IOVT_UINT16, 0},
#endif /* STA */
#ifdef WLRSDB
	{"scan_parallel", IOV_SCAN_RSDB_PARALLEL_SCAN, 0, IOVT_BOOL, 0},
#endif
#ifdef WLSCAN_PS
	/* single core scanning to reduce power consumption */
	{"scan_ps", IOV_SCAN_PWRSAVE, (IOVF_WHL), IOVT_UINT8, 0},
#endif /* WLSCAN_PS */
	/* configurable scan MAC */
	{"scanmac", IOV_SCANMAC, 0, IOVT_BUFFER, OFFSETOF(wl_scanmac_t, data)},
	{NULL, 0, 0, 0, 0 }
};
#endif /* WLC_SCAN_IOVARS */

/* debug timer used in scan module */
/* #define DEBUG_SCAN_TIMER */
#ifdef DEBUG_SCAN_TIMER
static void
wlc_scan_add_timer_dbg(scan_info_t *scan, uint to, bool prd, const char *fname, int line)
{
	WL_SCAN(("wl%d: %s(%d): wl_add_timer: timeout %u tsf %u\n",
		scan->unit, fname, line, to, SCAN_GET_TSF_TIMERLOW(scan)));
	wl_add_timer(scan->wlc->wl, scan->timer, to, prd);
}

static bool
wlc_scan_del_timer_dbg(scan_info_t *scan, const char *fname, int line)
{
	WL_SCAN(("wl%d: %s(%d): wl_del_timer: tsf %u\n",
		scan->unit, func, line, SCAN_GET_TSF_TIMERLOW(scan)));
	return wl_del_timer(scan->wlc->wl, scan->timer);
}
#define WLC_SCAN_ADD_TIMER(scan, to, prd) \
	      wlc_scan_add_timer_dbg(scan, to, prd, __FUNCTION__, __LINE__)
#define WLC_SCAN_DEL_TIMER(wlc, scan) \
	      wlc_scan_del_timer_dbg(scan, __FUNCTION__, __LINE__)
#define WLC_SCAN_ADD_TEST_TIMER(scan, to, prd) \
	wl_add_timer((scan)->wlc->wl, (scan)->test_timer, (to), (prd))
#else /* DEBUG_SCAN_TIMER */
#define WLC_SCAN_ADD_TIMER(scan, to, prd) wl_add_timer((scan)->wlc->wl, (scan)->timer, (to), (prd))
#define WLC_SCAN_DEL_TIMER(scan) wl_del_timer((scan)->wlc->wl, (scan)->timer)
#define WLC_SCAN_ADD_TEST_TIMER(scan, to, prd) \
	wl_add_timer((scan)->wlc->wl, (scan)->test_timer, (to), (prd))
#endif /* DEBUG_SCAN_TIMER */
#define WLC_SCAN_FREE_TIMER(scan)	wl_free_timer((scan)->wlc->wl, (scan)->timer)

#define WLC_SCAN_LENGTH_ERR	"%s: Length verification failed len: %d sm->len: %d\n"

#define WL_SCAN_ENT(scan, x)


/* guard time */
#define WLC_SCAN_PSPEND_GUARD_TIME	15
#define WLC_SCAN_WSUSPEND_GUARD_TIME	5

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

wlc_scan_info_t*
BCMATTACHFN(wlc_scan_attach)(wlc_info_t *wlc, void *wl, osl_t *osh, uint unit)
{
	scan_info_t *scan_info;
	iovar_fn_t iovar_fn = NULL;
	const bcm_iovar_t *iovars = NULL;
	int	err = 0;

	uint	scan_info_size = (uint)sizeof(scan_info_t);
	uint	ssid_offs, chan_offs;

	ssid_offs = scan_info_size = ROUNDUP(scan_info_size, sizeof(uint32));
	scan_info_size += sizeof(wlc_ssid_t) * WLC_SCAN_NSSID_PREALLOC;
	chan_offs = scan_info_size = ROUNDUP(scan_info_size, sizeof(uint32));

	scan_info = (scan_info_t *)MALLOCZ(osh, scan_info_size);
	if (scan_info == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__, (int)scan_info_size,
			MALLOCED(osh)));
		return NULL;
	}

	scan_info->scan_pub = (struct wlc_scan_info *)MALLOCZ(osh, sizeof(struct wlc_scan_info));
	if (scan_info->scan_pub == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			(int)sizeof(struct wlc_scan_info), MALLOCED(osh)));
		MFREE(osh, scan_info, scan_info_size);
		return NULL;
	}
	scan_info->scan_pub->scan_priv = (void *)scan_info;

	/* OBJECT REGISTRY: check if shared scan_cmn_info &
	 * wlc_scan_cmn_info  has value already stored
	 */
	scan_info->scan_cmn = (scan_cmn_info_t*)
		obj_registry_get(wlc->objr, OBJR_SCANPRIV_CMN);

	if (scan_info->scan_cmn == NULL) {
		if ((scan_info->scan_cmn =  (scan_cmn_info_t*) MALLOCZ(osh,
			sizeof(scan_cmn_info_t))) == NULL) {
			WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
				(int)sizeof(scan_cmn_info_t), MALLOCED(osh)));
			goto error;
		}
		/* OBJECT REGISTRY: We are the first instance, store value for key */
		obj_registry_set(wlc->objr, OBJR_SCANPRIV_CMN, scan_info->scan_cmn);
	}
	BCM_REFERENCE(obj_registry_ref(wlc->objr, OBJR_SCANPRIV_CMN));

	scan_info->scan_pub->wlc_scan_cmn = (struct wlc_scan_cmn_info*)
		obj_registry_get(wlc->objr, OBJR_SCANPUBLIC_CMN);

	if (scan_info->scan_pub->wlc_scan_cmn == NULL) {
		if ((scan_info->scan_pub->wlc_scan_cmn =  (struct wlc_scan_cmn_info*)
			MALLOC(osh, sizeof(struct wlc_scan_cmn_info))) == NULL) {
			WL_ERROR(("wl%d: %s: wlc_scan_cmn_info alloc falied\n",
				unit, __FUNCTION__));
			goto error;
		}
		bzero((char*)scan_info->scan_pub->wlc_scan_cmn,
			sizeof(struct wlc_scan_cmn_info));
		/* OBJECT REGISTRY: We are the first instance, store value for key */
		obj_registry_set(wlc->objr, OBJR_SCANPUBLIC_CMN,
		scan_info->scan_pub->wlc_scan_cmn);
	}
	BCM_REFERENCE(obj_registry_ref(wlc->objr, OBJR_SCANPUBLIC_CMN));

	scan_info->scan_cmn->memsize = scan_info_size;
	scan_info->wlc = wlc;
	scan_info->osh = osh;
	scan_info->unit = unit;
	scan_info->channel_idx = -1;
	scan_info->scan_pub->in_progress = FALSE;

	scan_info->scan_cmn->defaults.assoc_time = WLC_SCAN_ASSOC_TIME;
	scan_info->scan_cmn->defaults.unassoc_time = WLC_SCAN_UNASSOC_TIME;
	scan_info->scan_cmn->defaults.home_time = WLC_SCAN_HOME_TIME;
	scan_info->scan_cmn->defaults.passive_time = WLC_SCAN_PASSIVE_TIME;
	scan_info->scan_cmn->defaults.nprobes = WLC_SCAN_NPROBES;
	scan_info->scan_cmn->defaults.passive = FALSE;
	scan_info->home_away_time = WLC_SCAN_AWAY_LIMIT;

	scan_info->timer = wl_init_timer((struct wl_info *)wl,
	                                 wlc_scantimer, scan_info, "scantimer");
	if (scan_info->timer == NULL) {
		WL_ERROR(("wl%d: %s: wl_init_timer for scan timer failed\n", unit, __FUNCTION__));
		goto error;
	}

#if defined(WLSCANCACHE) && !defined(WLSCANCACHE_DISABLED)
	scan_info->sdb = wlc_scandb_create(osh, unit);
	if (scan_info->sdb == NULL) {
		WL_ERROR(("wl%d: %s: wlc_create_scandb failed\n", unit, __FUNCTION__));
		goto error;
	}
	wlc->pub->_scancache_support = TRUE;
	scan_info->scan_pub->_scancache = FALSE;	/* disabled by default */
#endif /* WLSCANCACHE || WLSCANCACHE_DISABLED */
	SCAN_SET_WATCHDOG_FN(wlc_scan_watchdog);

#ifdef WLC_SCAN_IOVARS
	iovar_fn = wlc_scan_doiovar;
	iovars = wlc_scan_iovars;
#endif /* WLC_SCAN_IOVARS */

	scan_info->ssid_prealloc = (wlc_ssid_t*)((uintptr)scan_info + ssid_offs);
	scan_info->nssid_prealloc = WLC_SCAN_NSSID_PREALLOC;
	scan_info->ssid_list = scan_info->ssid_prealloc;

	BCM_REFERENCE(chan_offs);

	if (PWRSTATS_ENAB(wlc->pub)) {
		scan_info->scan_stats =
			(wl_pwr_scan_stats_t*)MALLOC(osh, sizeof(wl_pwr_scan_stats_t));
		if (scan_info->scan_stats == NULL) {
			WL_ERROR(("wl%d: %s: failure allocating power stats\n",
				unit, __FUNCTION__));
			goto error;
		}
		bzero((char*)scan_info->scan_stats, sizeof(wl_pwr_scan_stats_t));
	}
	err = wlc_module_register(wlc->pub, iovars, "scan", scan_info, iovar_fn,
		wlc_scan_watchdog, NULL, wlc_scan_down);
	if (err) {
		WL_ERROR(("wl%d: %s: wlc_module_register err=%d\n",
		          unit, __FUNCTION__, err));
		goto error;
	}

	err = wlc_module_add_ioctl_fn(wlc->pub, (void *)scan_info->scan_pub,
	                              (wlc_ioctl_fn_t)wlc_scan_ioctl, 0, NULL);
	if (err) {
		WL_ERROR(("wl%d: %s: wlc_module_add_ioctl_fn err=%d\n",
		          unit, __FUNCTION__, err));
		goto error;
	}

	/* register bsscfg deinit callback through cubby registration */
	if ((err = wlc_bsscfg_cubby_reserve(wlc, 0, NULL, wlc_scan_bss_deinit, NULL,
			scan_info)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve err=%d\n",
		          unit, __FUNCTION__, err));
		goto error;
	}


	scan_info->pspend_guard_time = WLC_SCAN_PSPEND_GUARD_TIME;
	scan_info->wsuspend_guard_time = WLC_SCAN_WSUSPEND_GUARD_TIME;

#ifdef WLSCAN_PS
	scan_info->scan_ps_txchain = 0;
	scan_info->scan_ps_rxchain = 0;
	/* disable scan power optimization by default */
	scan_info->scan_pwrsave_enable = FALSE;
#endif /* WLSCAN_PS */

	return scan_info->scan_pub;

error:
	if (scan_info) {
		if (scan_info->timer != NULL)
			WLC_SCAN_FREE_TIMER(scan_info);
		if (scan_info->sdb)
			wlc_scandb_free(scan_info->sdb);
		if (scan_info->scan_pub)
			MFREE(osh, scan_info->scan_pub, sizeof(struct wlc_scan_info));
		if (PWRSTATS_ENAB(wlc->pub) && scan_info->scan_stats) {
			MFREE(osh, scan_info->scan_stats, sizeof(wl_pwr_scan_stats_t));
		}
		MFREE(osh, scan_info, scan_info_size);
	}

	return NULL;
}

static void
wlc_scan_ssidlist_free(scan_info_t *scan_info)
{
	if (scan_info->ssid_list != scan_info->ssid_prealloc) {
		MFREE(scan_info->osh, scan_info->ssid_list,
		      scan_info->nssid * sizeof(wlc_ssid_t));
		scan_info->ssid_list = scan_info->ssid_prealloc;
		scan_info->nssid = scan_info->nssid_prealloc;
	}
}

int
BCMUNINITFN(wlc_scan_down)(void *hdl)
{
#ifndef BCMNODOWN
	scan_info_t *scan_info = (scan_info_t *)hdl;
	int callbacks = 0;

	if (!WLC_SCAN_DEL_TIMER(scan_info))
		callbacks ++;

	scan_info->state = WLC_SCAN_STATE_START;
	scan_info->channel_idx = -1;
	scan_info->scan_pub->in_progress = FALSE;
	wlc_phy_hold_upd(SCAN_GET_PI_PTR(scan_info), PHY_HOLD_FOR_SCAN, FALSE);

	wlc_scan_ssidlist_free(scan_info);

	return callbacks;
#else
	return 0;
#endif /* BCMNODOWN */
}

void
BCMATTACHFN(wlc_scan_detach)(wlc_scan_info_t *wlc_scan_info)
{
	scan_info_t *scan_info;
	if (!wlc_scan_info)
		return;

	scan_info = (scan_info_t *)wlc_scan_info->scan_priv;

	if (scan_info) {
		int memsize = scan_info->scan_cmn->memsize;
		wlc_info_t * wlc = scan_info->wlc;

		if (scan_info->timer) {
			WLC_SCAN_FREE_TIMER(scan_info);
			scan_info->timer = NULL;
		}
#ifdef WLSCANCACHE
		if (SCANCACHE_SUPPORT(scan_info->wlc->pub))
			wlc_scandb_free(scan_info->sdb);
#endif /* WLSCANCACHE */

		wlc_module_unregister(scan_info->wlc->pub, "scan", scan_info);

		wlc_module_remove_ioctl_fn(scan_info->wlc->pub, (void *)scan_info->scan_pub);

		ASSERT(scan_info->ssid_list == scan_info->ssid_prealloc);
		if (scan_info->ssid_list != scan_info->ssid_prealloc) {
			WL_ERROR(("wl%d: %s: ssid_list not set to prealloc\n",
				scan_info->unit, __FUNCTION__));
		}
		if (obj_registry_unref(wlc->objr, OBJR_SCANPRIV_CMN) == 0) {
			obj_registry_set(wlc->objr, OBJR_SCANPRIV_CMN, NULL);
			MFREE(wlc->osh, scan_info->scan_cmn, sizeof(scan_cmn_info_t));
		}
		if (obj_registry_unref(wlc->objr, OBJR_SCANPUBLIC_CMN) == 0) {
			obj_registry_set(wlc->objr, OBJR_SCANPUBLIC_CMN, NULL);
			MFREE(wlc->osh, wlc_scan_info->wlc_scan_cmn, sizeof(wlc_scan_cmn_t));
		}
		MFREE(scan_info->osh, wlc_scan_info, sizeof(struct wlc_scan_info));
		MFREE(scan_info->osh, scan_info, memsize);
	}
}

#if defined(WLMSG_INFORM)
static void
wlc_scan_print_ssids(wlc_ssid_t *ssid, int nssid)
{
	char ssidbuf[SSID_FMT_BUF_LEN];
	int linelen = 0;
	int len;
	int i;

	for (i = 0; i < nssid; i++) {
		len = wlc_format_ssid(ssidbuf, ssid[i].SSID, ssid[i].SSID_len);
		/* keep the line length under 80 cols */
		if (linelen + (len + 2) > 80) {
			printf("\n");
			linelen = 0;
		}
		printf("\"%s\" ", ssidbuf);
		linelen += len + 3;
	}
	printf("\n");
}
#endif 

bool
wlc_scan_in_scan_chanspec_list(wlc_scan_info_t *wlc_scan_info, chanspec_t chanspec)
{
	scan_info_t *scan_info = (scan_info_t *) wlc_scan_info->scan_priv;
	int i;
	uint8 chan;

	/* scan chanspec list not setup, return no match */
	if (scan_info->channel_idx == -1) {
		WL_INFORM_SCAN(("%s: Scan chanspec list NOT setup, NO match\n", __FUNCTION__));
		return FALSE;
	}

	/* if strict channel match report is not needed, return match */
	if (wlc_scan_info->state & SCAN_STATE_OFFCHAN)
		return TRUE;

	chan = wf_chspec_ctlchan(chanspec);
	for (i = 0; i < scan_info->channel_num; i++) {
		if (wf_chspec_ctlchan(scan_info->chanspec_list[i]) == chan) {
			return TRUE;
		}
	}

	return FALSE;
}

#ifdef WLRSDB
/* return true if any of the scan is in progress. */
int
wlc_scan_anyscan_in_progress(wlc_scan_info_t *scan)
{
	int idx;
	scan_info_t *scan_info = (scan_info_t *)scan->scan_priv;
	wlc_info_t *wlc = SCAN_WLC(scan_info);
	wlc_info_t *wlc_iter;

	FOREACH_WLC(wlc->cmn, idx, wlc_iter) {
		if (wlc_iter->scan && wlc_iter->scan->in_progress)
			return TRUE;
	}
	return FALSE;
}


static
int wlc_scan_split_channels_per_band(scan_info_t *scan_info,
	const chanspec_t* chanspec_list, chanspec_t chanspec_start, int chan_num,
	chanspec_t** chanspec_list2g, chanspec_t** chanspec_list5g, chanspec_t *chanspec_start_2g,
	chanspec_t  *chanspec_start_5g, int *channel_num2g, int *channel_num5g)
{
	int i, j, k;
	scan_cmn_info_t *scan_cmn = scan_info->scan_cmn;
	wlc_info_t *wlc = SCAN_WLC(scan_info);

	scan_cmn->chanspec_list_size = (sizeof(chanspec_t) * chan_num);

	/* Allocate new chanspec list for both 2G and 5G channels. */
	scan_cmn->chanspeclist = MALLOCZ(wlc->osh, scan_cmn->chanspec_list_size);
	if (scan_cmn->chanspeclist == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			(int)scan_cmn->chanspec_list_size, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}
	*chanspec_list5g = *chanspec_list2g = NULL;
	*channel_num2g = *channel_num5g = 0;

	/* Findout number of 2G channels in chanspec_list  */
	for (i = 0; i < chan_num; i++) {
		if (!wf_chspec_malformed(chanspec_list[i]) &&
			wlc_scan_valid_chanspec_db(scan_info, chanspec_list[i])) {
			if (CHSPEC_IS2G(chanspec_list[i]))
				(*channel_num2g)++;
			else
				(*channel_num5g)++;
		}
	}
	ASSERT((*channel_num2g + *channel_num5g) <= chan_num);

	if (*channel_num2g)
		*chanspec_list2g = scan_cmn->chanspeclist;

	if (*channel_num5g)
		*chanspec_list5g = scan_cmn->chanspeclist + *channel_num2g;

	for (i = 0, j = 0, k = 0; i < chan_num; i++) {
		if (!wf_chspec_malformed(chanspec_list[i]) &&
			wlc_scan_valid_chanspec_db(scan_info, chanspec_list[i])) {
			if (CHSPEC_IS2G(chanspec_list[i]))
				(*chanspec_list2g)[j++] = chanspec_list[i];
			else
				(*chanspec_list5g)[k++] = chanspec_list[i];
		}
	}
	if (*channel_num2g) {
		*chanspec_start_2g = (*chanspec_list2g)[0];
	}
	if (*channel_num5g) {
		*chanspec_start_5g = (*chanspec_list5g)[0];
	}

	return BCME_OK;
}

static
void wlc_parallel_scan_cb(void *arg, int status, wlc_bsscfg_t *cfg)
{
	wlc_info_t *scanned_wlc = (wlc_info_t*) arg;
	wlc_info_t *scan_request_wlc = cfg->wlc;
	scan_info_t *scan_info = (scan_info_t *)scanned_wlc->scan->scan_priv;
	int status2 = scan_info->scan_cmn->first_scanresult_status;
	int final_status = WLC_E_STATUS_INVALID;

	WL_SCAN(("wl%d.%d %s Scanned in wlc:%d, requested wlc:%d",
		scanned_wlc->pub->unit, cfg->_idx, __FUNCTION__, scanned_wlc->pub->unit,
		scan_request_wlc->pub->unit));

	if (scan_info->state == WLC_SCAN_STATE_ABORT) {
		if (scan_info->scan_cmn->num_of_cbs > 0) {
			wlc_info_t *otherwlc;
			/* first scan aborted, abort the other and wait for it to finish */
			scan_info->scan_cmn->first_scanresult_status = status;
			otherwlc = wlc_rsdb_get_other_wlc(scanned_wlc);
			wlc_scan_abort(otherwlc->scan, status);
			return;
		} else {
			 /* last scan aborted, free the list and return abort status */
			 wlc_scan_bss_list_free((scan_info_t *)
				 scan_request_wlc->scan->scan_priv);
			(*scan_info->scan_cmn->cb)(scan_info->scan_cmn->cb_arg,
				status, cfg);
			 if (scan_info->scan_cmn->chanspeclist) {
				 MFREE(scan_request_wlc->osh, scan_info->scan_cmn->chanspeclist,
				 scan_info->scan_cmn->chanspec_list_size);
				 scan_info->scan_cmn->chanspeclist = NULL;
				 scan_info->scan_cmn->chanspec_list_size = 0;
			 }
			return;
		}
	}
	if (scan_info->scan_cmn->num_of_cbs > 0) {
		/* Wait for all scan to complete. */
		scan_info->scan_cmn->first_scanresult_status = status;
		return;
	}
	ASSERT(scan_info->scan_cmn->num_of_cbs == 0);
	/* single bands scan will have one scan complete callback. In such cases
	 * first status(status2) will be invalid.
	 */
	if (status2 == WLC_E_STATUS_INVALID || status == status2) {
		final_status = status;
	} else {
		/* We have different scan status from different wlc's...
		 * Send a single scan status based on these status codes.
		Order of increasing priority:
		 WLC_E_STATUS_NOCHANS
		 WLC_E_STATUS_SUCCESS
		 WLC_E_STATUS_PARTIAL
		 WLC_E_STATUS_SUPPRESS
		 Below ABORT status cases handled above:
		 WLC_E_STATUS_NEWASSOC
		 WLC_E_STATUS_CCXFASTRM
		 WLC_E_STATUS_11HQUIET
		 WLC_E_STATUS_CS_ABORT
		 WLC_E_STATUS_ABORT
		*/

		ASSERT(status != WLC_E_STATUS_ABORT);
		ASSERT(status2 != WLC_E_STATUS_ABORT);

		switch (status) {
			case WLC_E_STATUS_NOCHANS:
				final_status = status2;
				break;
			case WLC_E_STATUS_SUPPRESS:
				if (status2 == WLC_E_STATUS_SUCCESS ||
					status2 == WLC_E_STATUS_PARTIAL ||
					status2 == WLC_E_STATUS_NOCHANS)
					final_status = status;
				break;
			case WLC_E_STATUS_SUCCESS:
				if (status2 == WLC_E_STATUS_NOCHANS)
					final_status = status;
				else
					final_status = status2;
				break;
			case WLC_E_STATUS_PARTIAL:
				if (status2 == WLC_E_STATUS_SUPPRESS)
					final_status = status2;
				else
					final_status = status;
				break;
			default:
				/* No Other valid status in scanning apart from abort.
				 * If new status got introduced, handle it in
				 * new switch case.
				 */
				/* Use the last status */
				final_status = status;
				WL_ERROR(("Err. Parallel scan"
				" status's are not matching\n"));
				ASSERT(0);
		}
	}

	(*scan_info->scan_cmn->cb)(scan_info->scan_cmn->cb_arg,
		final_status, cfg);

	if (scan_info->scan_cmn->chanspeclist) {
		MFREE(scan_request_wlc->osh, scan_info->scan_cmn->chanspeclist,
			scan_info->scan_cmn->chanspec_list_size);
		scan_info->scan_cmn->chanspeclist = NULL;
		scan_info->scan_cmn->chanspec_list_size = 0;
	}
}
#endif /* WLRSDB */

/* Caution: when passing a non-primary bsscfg to this function the caller
 * must make sure to abort the scan before freeing the bsscfg!
 */
int
wlc_scan(
	wlc_scan_info_t *wlc_scan_info,
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
	scancb_fn_t fn, void* arg,
	int away_channels_limit,
	bool extdscan,
	bool suppress_ssid,
	bool include_cache,
	uint scan_flags,
	wlc_bsscfg_t *cfg,
	uint8 usage,
	actcb_fn_t act_cb, void *act_cb_arg,
	struct ether_addr *sa_override)
{

	scan_info_t *scan_info = (scan_info_t *) wlc_scan_info->scan_priv;
#ifdef WLRSDB
	wlc_info_t *wlc_2g, *wlc_5g;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )
#endif
	wlc_scan_info->wlc_scan_cmn->usage = (uint8)usage;

#ifdef WLRSDB

	BCM_REFERENCE(scan_info);

	/* RSDB is enabled on this chip... trigger parallel scan
	 * if parallel scan is not disabled.
	 */
	if (RSDB_PARALLEL_SCAN_ON(scan_info)) {
		chanspec_t *chanspec_list5g = NULL;
		chanspec_t *chanspec_list2g = NULL;
		chanspec_t chanspec_start_2g = 0, chanspec_start_5g = 0;
		int channel_num2g = 0, channel_num5g = 0;

		WL_SCAN(("wl%d.%d:%s RSDB PARALLEL SCANNING..\n",
			cfg->wlc->pub->unit, cfg->_idx, __FUNCTION__));

		wlc_rsdb_get_wlcs(cfg->wlc, &wlc_2g, &wlc_5g);
		if (channel_num) {
				/* channel list validation */
			if (channel_num > MAXCHANNEL) {
				WL_ERROR(("wl%d: %s: wlc_scan bad param channel_num %d greater"
				" than max %d\n", scan_info->unit, __FUNCTION__,
				channel_num, MAXCHANNEL));
				channel_num = 0;
				return BCME_EPERM;
			}
			if (channel_num > 0 && chanspec_list == NULL) {
				WL_ERROR(("wl%d: %s: wlc_scan bad param channel_list was NULL"
					" with channel_num = %d\n",
					scan_info->unit, __FUNCTION__, channel_num));
				channel_num = 0;
				return BCME_EPERM;
			}
			wlc_scan_split_channels_per_band(scan_info,
				chanspec_list, chanspec_start, channel_num, &chanspec_list2g,
				&chanspec_list5g, &chanspec_start_2g, &chanspec_start_5g,
				&channel_num2g, &channel_num5g);
		} else {

			WL_SCAN(("2g home channel:%x 5g home channel:%x\n",
			wlc_2g->home_chanspec, wlc_5g->home_chanspec));

			chanspec_start_2g = wlc_2g->home_chanspec;
			chanspec_start_5g = wlc_5g->home_chanspec;

			if (!CHSPEC_IS2G(chanspec_start_2g)) {
				/* Get the first channel of the band we want to scan.
				 */
				chanspec_start_2g = wlc_next_chanspec(wlc_2g->cmi,
				CH20MHZ_CHSPEC(0), CHAN_TYPE_ANY, TRUE);

			}
			if (!CHSPEC_IS5G(chanspec_start_5g)) {
				chanspec_start_5g = wlc_next_chanspec(wlc_5g->cmi,
					CH20MHZ_CHSPEC(15), CHAN_TYPE_ANY, TRUE);
			}
		}
		/* Setup valid scan callbacks. */
		scan_info->scan_cmn->cb = fn;
		scan_info->scan_cmn->cb_arg = arg;
		scan_info->scan_cmn->num_of_cbs = 0;
		scan_info->scan_cmn->first_scanresult_status = WLC_E_STATUS_INVALID;
		/* scan_info->scan_cmn->cfg = SCAN_USER(scan_info, cfg); */

		/* one of the chain for mpc update should be already handled */
		if ((channel_num2g || !channel_num) && (!wlc_2g->mpc_scan)) {
			wlc_2g->mpc_scan = TRUE;
			WL_SCAN(("wl%d.%d:%s PARALLEL SCANNING update mpc\n",
				wlc_2g->pub->unit, cfg->_idx, __FUNCTION__));
			wlc_radio_mpc_upd(wlc_2g);
		}

		if ((channel_num5g || !channel_num) && (!wlc_5g->mpc_scan)) {
			wlc_5g->mpc_scan = TRUE;
			WL_SCAN(("wl%d.%d:%s PARALLEL SCANNING update mpc\n",
				wlc_5g->pub->unit, cfg->_idx, __FUNCTION__));
			wlc_radio_mpc_upd(wlc_5g);
		}

		/* 2G band scan */
		if (channel_num2g || !channel_num) {
			WL_SCAN(("Scanning on wl%d.%d, Start chanspec:%s (2G), total:%d\n",
				wlc_2g->pub->unit, cfg->_idx,
				wf_chspec_ntoa_ex(chanspec_start_2g, chanbuf), channel_num2g));
			scan_info->scan_cmn->num_of_cbs++;
			/* Get the control chanspecs of channel. */
			chanspec_start_2g = wf_chspec_ctlchspec(chanspec_start_2g);

			_wlc_scan(wlc_2g->scan, bss_type, bssid, nssid, ssid,
				scan_type, nprobes, active_time, passive_time, home_time,
				chanspec_list2g, channel_num2g, chanspec_start,
				save_prb, wlc_parallel_scan_cb, wlc_2g, away_channels_limit,
				extdscan, suppress_ssid, include_cache, scan_flags, cfg, act_cb,
				act_cb_arg, WLC_BAND_2G, chanspec_start_2g, sa_override);
		}
		/* 5G band scan */
		if (channel_num5g || !channel_num) {
			WL_SCAN(("Scanning on wl%d.%d Start chanspec:%s (5G) total:%d\n",
				wlc_5g->pub->unit, cfg->_idx,
				wf_chspec_ntoa_ex(chanspec_start_5g, chanbuf), channel_num5g));
			scan_info->scan_cmn->num_of_cbs++;
			/* Get the control chanspecs of channel. */
			chanspec_start_5g = wf_chspec_ctlchspec(chanspec_start_5g);

			_wlc_scan(wlc_5g->scan, bss_type, bssid, nssid, ssid,
				scan_type, nprobes, active_time, passive_time, home_time,
				chanspec_list5g, channel_num5g, chanspec_start,
				save_prb, wlc_parallel_scan_cb, wlc_5g, away_channels_limit,
				extdscan, suppress_ssid, include_cache, scan_flags, cfg, act_cb,
				act_cb_arg, WLC_BAND_5G, chanspec_start_5g, sa_override);
		}
		if (scan_info->scan_cmn->num_of_cbs)
			return BCME_OK;
		else
			return BCME_EPERM;
	} else
#endif /* WLRSDB */

		return _wlc_scan(wlc_scan_info, bss_type, bssid, nssid, ssid,
			scan_type, nprobes, active_time, passive_time, home_time,
			chanspec_list, channel_num, chanspec_start,
			save_prb, fn, arg, away_channels_limit, extdscan, suppress_ssid,
			include_cache, scan_flags, cfg, act_cb, act_cb_arg, WLC_BAND_ALL,
			wf_chspec_ctlchspec(SCAN_HOME_CHANNEL(scan_info)), sa_override);
}

int _wlc_scan(
	wlc_scan_info_t *wlc_scan_info,
	int bss_type,
	const struct ether_addr* bssid,
	int nssid,
	wlc_ssid_t *ssid,
	int scan_type,
	int nprobes,
	int active_time,
	int passive_time,
	int home_time,
	const chanspec_t* chanspec_list, int channel_num, chanspec_t chanspec_start,
	bool save_prb,
	scancb_fn_t fn, void* arg,
	int away_channels_limit,
	bool extdscan,
	bool suppress_ssid,
	bool include_cache,
	uint scan_flags,
	wlc_bsscfg_t *cfg,
	actcb_fn_t act_cb, void *act_cb_arg, int bandinfo, chanspec_t band_chanspec_start,
	struct ether_addr *sa_override)
{
	scan_info_t *scan_info = (scan_info_t *) wlc_scan_info->scan_priv;
	bool scan_in_progress;
	int i, num;
	wlc_info_t *wlc = scan_info->wlc;
#if defined(WLMSG_INFORM)
	char *ssidbuf;
	char eabuf[ETHER_ADDR_STR_LEN];
	char chanbuf[CHANSPEC_STR_LEN];
#endif
#ifdef WLMSG_ROAM
	char SSIDbuf[DOT11_MAX_SSID_LEN+1];
#endif
	int ret = BCME_OK;
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(home_time);
	BCM_REFERENCE(away_channels_limit);
	BCM_REFERENCE(suppress_ssid);
	ASSERT(nssid);
	ASSERT(ssid != NULL);
	ASSERT(bss_type == DOT11_BSSTYPE_INFRASTRUCTURE ||
	       bss_type == DOT11_BSSTYPE_INDEPENDENT ||
	       bss_type == DOT11_BSSTYPE_MESH ||
	       bss_type == DOT11_BSSTYPE_ANY);

	WL_SCAN(("wl%d: %s: scan request at %u\n", scan_info->unit, __FUNCTION__,
	         SCAN_GET_TSF_TIMERLOW(scan_info)));
#if defined(WLMSG_INFORM)
	ssidbuf = (char *) MALLOCZ(scan_info->osh, SSID_FMT_BUF_LEN);
	if (ssidbuf == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, scan_info->unit, __FUNCTION__, (int)SSID_FMT_BUF_LEN,
			MALLOCED(scan_info->osh)));
		return BCME_NOMEM;
	}

	if (nssid == 1) {
		wlc_format_ssid(ssidbuf, ssid->SSID, ssid->SSID_len);
		WL_INFORM_SCAN(("wl%d: %s: scanning for SSID \"%s\"\n", scan_info->unit,
			__FUNCTION__, ssidbuf));
	} else {
		WL_INFORM_SCAN(("wl%d: %s: scanning for SSIDs:\n", scan_info->unit, __FUNCTION__));
		if (WL_INFORM_ON())
			wlc_scan_print_ssids(ssid, nssid);
	}
	WL_INFORM_SCAN(("wl%d: %s: scanning for BSSID \"%s\"\n", scan_info->unit, __FUNCTION__,
	           (bcm_ether_ntoa(bssid, eabuf), eabuf)));
#endif 

	/* enforce valid argument */
	scan_info->ssid_wildcard_enabled = 0;
	for (i = 0; i < nssid; i++) {
		if (ssid[i].SSID_len > DOT11_MAX_SSID_LEN) {
			WL_ERROR(("wl%d: %s: invalid SSID len %d, capping\n",
			          scan_info->unit, __FUNCTION__, ssid[i].SSID_len));
			ssid[i].SSID_len = DOT11_MAX_SSID_LEN;
		}
		if (ssid[i].SSID_len == 0)
			scan_info->ssid_wildcard_enabled = 1;
	}

	scan_in_progress = SCAN_IN_PROGRESS(wlc_scan_info);

	/* clear or set optional params to default */
	/* keep persistent scan suppress flag */
	wlc_scan_info->state &= SCAN_STATE_SUPPRESS;
	scan_info->nprobes = scan_info->scan_cmn->defaults.nprobes;
	if (IS_ASSOCIATED(scan_info)) {
		scan_info->active_time = scan_info->scan_cmn->defaults.assoc_time;
	} else {
		scan_info->active_time = scan_info->scan_cmn->defaults.unassoc_time;
	}
	scan_info->passive_time = scan_info->scan_cmn->defaults.passive_time;
	if (scan_info->scan_cmn->defaults.passive)
		wlc_scan_info->state |= SCAN_STATE_PASSIVE;

	if (scan_type == DOT11_SCANTYPE_ACTIVE) {
		wlc_scan_info->state &= ~SCAN_STATE_PASSIVE;
	} else if (scan_type == DOT11_SCANTYPE_PASSIVE) {
		wlc_scan_info->state |= SCAN_STATE_PASSIVE;
	}
	/* passive scan always has nprobes to 1 */
	if (wlc_scan_info->state & SCAN_STATE_PASSIVE) {
		scan_info->nprobes = 1;
	}
	if (active_time > 0)
		scan_info->active_time = (uint16)active_time;

	if (passive_time >= 0)
		scan_info->passive_time = (uint16)passive_time;

	if (nprobes > 0 && ((wlc_scan_info->state & SCAN_STATE_PASSIVE) == 0))
		scan_info->nprobes = (uint8)nprobes;
	if (save_prb)
		wlc_scan_info->state |= SCAN_STATE_SAVE_PRB;
	if (include_cache && SCANCACHE_ENAB(wlc_scan_info))
		wlc_scan_info->state |= SCAN_STATE_INCLUDE_CACHE;

	if (scan_flags & WL_SCANFLAGS_HOTSPOT) {
#ifdef ANQPO
		if (SCAN_ANQPO_ENAB(scan_info)) {
			wl_scan_anqpo_scan_start(scan_info);
			wlc_scan_info->wlc_scan_cmn->is_hotspot_scan = TRUE;
		}
#endif
	} else {
		wlc_scan_info->wlc_scan_cmn->is_hotspot_scan = FALSE;
	}

#ifdef WL_PROXDETECT
	if (PROXD_ENAB(wlc->pub)) {
		wlc_scan_info->flag &= ~SCAN_FLAG_SWITCH_CHAN;
		if (scan_flags & WL_SCANFLAGS_SWTCHAN) {
			wlc_scan_info->flag |= SCAN_FLAG_SWITCH_CHAN;
		}
	}
#endif

	WL_SCAN(("wl%d: %s: wlc_scan params: nprobes %d dwell active/passive %dms/%dms"
		" flags %d\n",
		scan_info->unit, __FUNCTION__, scan_info->nprobes, scan_info->active_time,
		scan_info->passive_time, wlc_scan_info->state));

	/* In case of ULB Mode with parallel scan disabled, we need scan_info->bss_cfg to
	 * point to correct bsscfg so that min_bw value configured by user can be picked
	 * for scanning
	 */
	scan_info->bsscfg = SCAN_USER(scan_info, cfg);

	if (!wlc_scan_info->iscan_cont) {
		wlc_scan_default_channels(wlc_scan_info, band_chanspec_start, bandinfo,
		scan_info->chanspec_list, &scan_info->channel_num);
	}

	if (scan_flags & WL_SCANFLAGS_PROHIBITED) {
		scan_info->scan_pub->state |= SCAN_STATE_PROHIBIT;
		num = wlc_scan_prohibited_channels(scan_info,
			&scan_info->chanspec_list[scan_info->channel_num],
			(MAXCHANNEL - scan_info->channel_num));
		scan_info->channel_num += num;
	} else
		scan_info->scan_pub->state &= ~SCAN_STATE_PROHIBIT;

	if (scan_flags & WL_SCANFLAGS_OFFCHAN)
		scan_info->scan_pub->state |= SCAN_STATE_OFFCHAN;
	else
		scan_info->scan_pub->state &= ~SCAN_STATE_OFFCHAN;

	if (IS_SIM_ENAB(scan_info)) {
		/* QT hack: abort scan since full scan may take forever */
		scan_info->channel_num = 1;
	}

	/* set required and optional params */
	/* If IBSS Lock Out feature is turned on, set the scan type to BSS only */
	wlc_scan_info->wlc_scan_cmn->bss_type =
		(IS_IBSS_ALLOWED(scan_info) == FALSE)?DOT11_BSSTYPE_INFRASTRUCTURE:bss_type;
	bcopy((const char*)bssid, (char*)&wlc_scan_info->bssid, ETHER_ADDR_LEN);

	/* allocate memory for ssid list, using prealloc if sufficient */
	ASSERT(scan_info->ssid_list == scan_info->ssid_prealloc);
	if (scan_info->ssid_list != scan_info->ssid_prealloc) {
		WL_ERROR(("wl%d: %s: ssid_list not set to prealloc\n",
		          scan_info->unit, __FUNCTION__));
	}
	if (nssid > scan_info->nssid_prealloc) {
		scan_info->ssid_list = MALLOCZ(scan_info->osh,
		                              nssid * sizeof(wlc_ssid_t));
		/* failed, cap at prealloc (best effort) */
		if (scan_info->ssid_list == NULL) {
			WL_ERROR((WLC_MALLOC_ERR, scan_info->unit, __FUNCTION__,
				(int)(nssid * sizeof(wlc_ssid_t)),
				MALLOCED(scan_info->osh)));
			nssid = scan_info->nssid_prealloc;
			scan_info->ssid_list = scan_info->ssid_prealloc;
		}
	}
	/* Now ssid_list is the right size for [current] nssid count */

	bcopy(ssid, scan_info->ssid_list, (sizeof(wlc_ssid_t) * nssid));
	scan_info->nssid = nssid;

#ifdef WLP2P
	if (IS_P2P_ENAB(scan_info)) {
		scan_info->ssid_wildcard_enabled = FALSE;
		for (i = 0; i < nssid; i ++) {
			if (scan_info->ssid_list[i].SSID_len == 0)
				wlc_p2p_fixup_SSID(wlc->p2p, cfg,
					&scan_info->ssid_list[i]);
			if (scan_info->ssid_list[i].SSID_len == 0)
				scan_info->ssid_wildcard_enabled = TRUE;
		}
	}
#endif

	/* channel list validation */
	if (channel_num > MAXCHANNEL) {
		WL_ERROR(("wl%d: %s: wlc_scan bad param channel_num %d greater than max %d\n",
			scan_info->unit, __FUNCTION__, channel_num, MAXCHANNEL));
		channel_num = 0;
	}
	if (channel_num > 0 && chanspec_list == NULL) {
		WL_ERROR(("wl%d: %s: wlc_scan bad param channel_list was NULL with channel_num ="
			" %d\n",
			scan_info->unit, __FUNCTION__, channel_num));
		channel_num = 0;
	}
	for (i = 0; i < channel_num; i++) {
		if ((scan_flags & WL_SCANFLAGS_PROHIBITED) ||
			WLC_CNTRY_DEFAULT_ENAB(wlc)) {
			if (wf_chspec_valid(chanspec_list[i]) == FALSE) {
				channel_num = 0;
			}
		}
		else if (!wlc_scan_valid_chanspec_db(scan_info, chanspec_list[i])) {
			channel_num = 0;
		}
	}

	if (channel_num > 0) {
		for (i = 0; i < channel_num; i++)
			scan_info->chanspec_list[i] = chanspec_list[i];
		scan_info->channel_num = channel_num;
	}
#ifdef WLMSG_ROAM
	bcopy(&ssid->SSID, SSIDbuf, ssid->SSID_len);
	SSIDbuf[ssid->SSID_len] = 0;
	WL_ROAM(("SCAN for '%s' %d SSID(s) %d channels\n", SSIDbuf, nssid, channel_num));
#endif /* WLMSG_ROAM */


	if ((wlc_scan_info->state & SCAN_STATE_SUPPRESS) || (!scan_info->channel_num)) {
		int status;

		WL_INFORM_SCAN(("wl%d: %s: scan->state %d scan->channel_num %d\n",
			scan_info->unit, __FUNCTION__,
			wlc_scan_info->state, scan_info->channel_num));

		if (wlc_scan_info->state & SCAN_STATE_SUPPRESS)
			status = WLC_E_STATUS_SUPPRESS;
		else
			status = WLC_E_STATUS_NOCHANS;

		if (scan_in_progress)
			wlc_scan_abort(wlc_scan_info, status);

		/* no scan now, but free any earlier leftovers */
		wlc_scan_bss_list_free(scan_info);

		if (fn != NULL)
			(fn)(arg, status, SCAN_USER(scan_info, cfg));

		wlc_scan_ssidlist_free(scan_info);

#if defined(WLMSG_INFORM)
		if (ssidbuf != NULL)
			 MFREE(scan_info->osh, (void *)ssidbuf, SSID_FMT_BUF_LEN);
#endif
		return BCME_EPERM;
	}

#ifdef STA
	if (!IS_EXTSTA_ENAB(scan_info))
		if (scan_in_progress && !IS_AS_IN_PROGRESS(scan_info))
			wlc_scan_callback(scan_info, WLC_E_STATUS_ABORT);
#endif /* STA */

	/* use default if sa_override not provided */
	if (!sa_override || ETHER_ISNULLADDR(sa_override)) {
		sa_override = &scan_info->bsscfg->cur_etheraddr;
	}
	memcpy(&scan_info->sa_override, sa_override, sizeof(scan_info->sa_override));

	scan_info->cb = fn;
	scan_info->cb_arg = arg;

	scan_info->act_cb = act_cb;
	scan_info->act_cb_arg = act_cb_arg;

	scan_info->extdscan = extdscan;


	/* extd scan for nssids one ssid per each pass..  */
	scan_info->npasses = (scan_info->extdscan && nssid) ? nssid : scan_info->nprobes;
	scan_info->channel_idx = 0;
	if (chanspec_start != 0) {
		for (i = 0; i < scan_info->channel_num; i++) {
			if (scan_info->chanspec_list[i] == chanspec_start) {
				scan_info->channel_idx = i;
				WL_INFORM_SCAN(("starting new iscan on chanspec %s\n",
				           wf_chspec_ntoa_ex(chanspec_start, chanbuf)));
				break;
			}
		}
	}
	wlc_scan_info->in_progress = TRUE;
#ifdef WLOLPC
	/* if on olpc chan notify - if needed, terminate active cal; go off channel */
	if (OLPC_ENAB(wlc)) {
		wlc_olpc_eng_hdl_chan_update(wlc->olpc_info);
	}
#endif /* WLOLPC */
	wlc_phy_hold_upd(SCAN_GET_PI_PTR(scan_info), PHY_HOLD_FOR_SCAN, TRUE);

#if defined(STA)
	wlc_scan_info->wlc_scan_cmn->scan_start_time = OSL_SYSUPTIME();
#endif /* STA */

	/* ...and free any leftover responses from before */
	wlc_scan_bss_list_free(scan_info);


	/* keep core awake to receive solicited probe responses, SCAN_IN_PROGRESS is TRUE */
	ASSERT(SCAN_STAY_AWAKE(scan_info));
	wlc_scan_set_wake_ctrl(scan_info);

	/* If we are switching away from radar home_chanspec
	 * because STA scans (normal/Join/Roam) with
	 * atleast one local 11H AP in radar channel,
	 * turn of radar_detect.
	 * NOTE: Implied that upstream AP assures this radar
	 * channel is clear.
	 */
	if (WL11H_AP_ENAB(wlc) && WLC_APSTA_ON_RADAR_CHANNEL(wlc) &&
		wlc_radar_chanspec(wlc->cmi, wlc->home_chanspec)) {
		WL_REGULATORY(("wl%d: %s Moving from home channel dfs OFF\n",
			wlc->pub->unit, __FUNCTION__));
		wlc_set_dfs_cacstate(wlc->dfs, OFF);
	}

	if ((ret = _wlc_scan_schd_channels(wlc_scan_info)) != BCME_OK) {
		/* call wlc_scantimer to get the scan state machine going */
		/* DUALBAND - Don't call wlc_scantimer() directly from DPC... */
		WL_ERROR(("wl%d: %s: scan request failed\n", scan_info->unit, __FUNCTION__));
		/* send out AF as soon as possible to aid reliability of GON */
		wlc_scan_abort(wlc_scan_info, WLC_SCAN_STATE_ABORT);
		return ret;
	}

#if defined(WLMSG_INFORM)
	if (ssidbuf != NULL)
		 MFREE(scan_info->osh, (void *)ssidbuf, SSID_FMT_BUF_LEN);
#endif
	/* if a scan is in progress, allow the next callback to restart the scan state machine */
	return ret;
}

static uint32
_wlc_get_next_chan_scan_time(wlc_scan_info_t *wlc_scan_info)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	wlc_info_t *wlc = scan_info->wlc;
	chanspec_t next_chanspec;

	if (scan_info->channel_idx < scan_info->channel_num) {
		next_chanspec = scan_info->chanspec_list[scan_info->channel_idx];
	} else {
		return SCAN_INVALID_DWELL_TIME;
	}

	if (scan_info->scan_pub->state & SCAN_STATE_PASSIVE) {
		return CHANNEL_PASSIVE_DWELLTIME(scan_info);
	} else {
		if (wlc_quiet_chanspec(wlc->cmi, next_chanspec) ||
		    !wlc_valid_chanspec_db(wlc->cmi, next_chanspec)) {
			return CHANNEL_PASSIVE_DWELLTIME(scan_info);
		}

		return CHANNEL_ACTIVE_DWELLTIME(scan_info);
	}
}

static int
_wlc_scan_schd_channels(wlc_scan_info_t *wlc_scan_info)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	uint32 dwell_time = _wlc_get_next_chan_scan_time(wlc_scan_info);
	uint32 chn_cnt = 1;
	int chn_start = scan_info->channel_idx;
	wlc_msch_req_param_t req_param;
	int err;

	memset(&req_param, 0, sizeof(req_param));

	/*
	* We need to account for invalide dwell times of 0, which could be returned if
	* we're doing an active scan and the caller has specified passive dwell time of 0
	* to avoid scanning quiet channels. In that case, we must reject the current channel
	* and see if we can find another channel that can be scanned.
	*/
	if (dwell_time == SCAN_INVALID_DWELL_TIME) {
		while (++scan_info->channel_idx < scan_info->channel_num) {
			dwell_time = _wlc_get_next_chan_scan_time(wlc_scan_info);
			if (dwell_time != SCAN_INVALID_DWELL_TIME) {
				break;
			}
		}

		if (scan_info->channel_idx >= scan_info->channel_num) {
			return BCME_SCANREJECT;
		}
	}

	/*
	* We've got a channel and a target dwell time. So, get the other channels with the
	* same dwell time.
	*/
	while (++scan_info->channel_idx < scan_info->channel_num) {
		if (_wlc_get_next_chan_scan_time(wlc_scan_info) != dwell_time) {
			break;
		}
		chn_cnt++;
	}

	scan_info->cur_scan_chanspec = 0;
	req_param.duration = MS_TO_USEC(dwell_time);
	req_param.req_type = MSCH_RT_START_FLEX;
	req_param.priority = MSCH_RP_CONNECTION;
	if ((err = wlc_msch_timeslot_register(scan_info->wlc->msch_info,
		&scan_info->chanspec_list[chn_start],
		chn_cnt, wlc_scan_chnsw_clbk, wlc_scan_info, &req_param,
		&scan_info->msch_req_hdl)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: MSCH timeslot register failed, err %d\n",
		         scan_info->unit, __FUNCTION__, err));

		return err;
	}

	return BCME_OK;
}

static int
wlc_scan_chnsw_clbk(void* handler_ctxt, wlc_msch_cb_info_t *cb_info)
{
	wlc_scan_info_t *wlc_scan_info = (wlc_scan_info_t *)handler_ctxt;
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	uint32 type = cb_info->type;
	wlc_msch_info_t *msch_info = scan_info->wlc->msch_info;
	uint32  dummy_tsf_h, start_tsf;
	char chanbuf[CHANSPEC_STR_LEN];
	BCM_REFERENCE(chanbuf);
	if (scan_info->msch_req_hdl == NULL) {
		/* The requeset is been cancelled, ignore the Clbk */
		return BCME_OK;
	}

	do {
		if (type & MSCH_CT_SLOT_START) {

			wlc_msch_onchan_info_t *onchan =
				(wlc_msch_onchan_info_t *)cb_info->type_specific;

			scan_info->cur_scan_chanspec = cb_info->chanspec;
#ifdef STA
			/* TODO: remove this backward dependency */
			wlc_scan_utils_set_chanspec(scan_info->wlc, scan_info->cur_scan_chanspec);
#endif
			/* delete any pending timer */
			WLC_SCAN_DEL_TIMER(scan_info);

			/* disable CFP & TSF update */
			wlc_mhf(SCAN_WLC(scan_info), MHF2, MHF2_SKIP_CFP_UPDATE,
				MHF2_SKIP_CFP_UPDATE, WLC_BAND_ALL);
			wlc_scan_skip_adjtsf(scan_info, TRUE, NULL, WLC_SKIP_ADJTSF_SCAN,
				WLC_BAND_ALL);

			if (!wlc_msch_query_ts_shared(msch_info, onchan->timeslot_id) &&
				!scan_info->wlc->excursion_active) {
				wlc_suspend_mac_and_wait(scan_info->wlc);
				/* suspend normal tx queue operation for channel excursion */
				wlc_excursion_start(scan_info->wlc);
				wlc_enable_mac(scan_info->wlc);
			}

			SCAN_BCN_PROMISC(scan_info) =
				wlc_scan_usage_scan(scan_info->scan_pub);
			wlc_scan_mac_bcn_promisc(scan_info);

			SCAN_READ_TSF(scan_info, &start_tsf, &dummy_tsf_h);
			scan_info->start_tsf = start_tsf;

			if (wlc_scan_quiet_chanspec(scan_info, scan_info->cur_scan_chanspec) ||
				!wlc_scan_valid_chanspec_db(scan_info,
				scan_info->cur_scan_chanspec)) {

				/* PASSIVE SCAN */
				scan_info->state = WLC_SCAN_STATE_LISTEN;
				scan_info->timeslot_id = onchan->timeslot_id;

				WL_SCAN(("wl%d: passive dwell time %d ms, chanspec %s,"
					" tsf %u\n", scan_info->unit,
					CHANNEL_PASSIVE_DWELLTIME(scan_info),
					wf_chspec_ntoa_ex(scan_info->cur_scan_chanspec,
					chanbuf), start_tsf));

				return BCME_OK;
			}

			scan_info->state = WLC_SCAN_STATE_START;
			WLC_SCAN_ADD_TIMER(scan_info, 0, 0);
			break;
		}

		if (type & MSCH_CT_SLOT_END) {
			if (scan_info->state != WLC_SCAN_STATE_SEND_PROBE &&
				scan_info->state != WLC_SCAN_STATE_LISTEN) {
				WL_SCAN(("wl%d: %s: wrong scan state %d \n",
					scan_info->unit, __FUNCTION__, scan_info->state));
			}

			scan_info->state = WLC_SCAN_STATE_CHN_SCAN_DONE;
			scan_info->timeslot_id = 0;

			/* scan passes complete for the current channel */
			WL_SCAN(("wl%d: %s: %sscanned channel %d, total responses %d, tsf %u\n",
			         scan_info->unit, __FUNCTION__,
			         ((wlc_scan_quiet_chanspec(scan_info, cb_info->chanspec) &&
			           !(wlc_scan_info->state & SCAN_STATE_RADAR_CLEAR)) ?
			          "passively ":""),
			        CHSPEC_CHANNEL(cb_info->chanspec),
				SCAN_RESULT_MEB(scan_info, count),
			        SCAN_GET_TSF_TIMERLOW(scan_info)));

			/* reset the radar clear flag since we will be leaving the channel */
			wlc_scan_info->state &= ~SCAN_STATE_RADAR_CLEAR;

			/* resume normal tx queue operation */
			wlc_scan_excursion_end(scan_info);
		}

		if (type & MSCH_CT_REQ_END) {
			/* delete any pending timer */
			WLC_SCAN_DEL_TIMER(scan_info);

			/* clear msch request handle */
			scan_info->msch_req_hdl = NULL;

			if (scan_info->channel_idx < scan_info->channel_num) {
				scan_info->state = WLC_SCAN_STATE_PARTIAL;
				WLC_SCAN_ADD_TIMER(scan_info, 0, 0);
			}
			else {
				scan_info->state = WLC_SCAN_STATE_COMPLETE;
				WLC_SCAN_ADD_TIMER(scan_info, 0, 0);
			}
			break;
		}
	} while (0);

	return BCME_OK;
}

void
wlc_scan_timer_update(wlc_scan_info_t *wlc_scan_info, uint32 ms)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;

	WLC_SCAN_DEL_TIMER(scan_info);
	WLC_SCAN_ADD_TIMER(scan_info, ms, 0);
}

/* return number of channels in current scan */
int
wlc_scan_chnum(wlc_scan_info_t *wlc_scan_info)
{
	return ((scan_info_t *)wlc_scan_info->scan_priv)->channel_num;
}

#if defined(STA)
uint32
wlc_curr_roam_scan_time(wlc_info_t *wlc)
{
	uint32 curr_roam_scan_time = 0;
	/* Calculate awake time due to active roam scan */
	if (wlc_scan_inprog(wlc) && (AS_IN_PROGRESS(wlc))) {
		wlc_bsscfg_t *cfg = AS_IN_PROGRESS_CFG(wlc);
		wlc_assoc_t *as = cfg->assoc;
		if (as->type == AS_ROAM)
			curr_roam_scan_time = wlc_get_curr_scan_time(wlc);
	}
	return curr_roam_scan_time;
}

uint32
wlc_get_curr_scan_time(wlc_info_t *wlc)
{
	scan_info_t *scan_info = wlc->scan->scan_priv;
	wlc_scan_info_t *scan_pub = scan_info->scan_pub;
	if (scan_pub->in_progress)
		return (OSL_SYSUPTIME() - scan_pub->wlc_scan_cmn->scan_start_time);
	return 0;
}

static void
wlc_scan_time_upd(wlc_info_t *wlc, scan_info_t *scan_info)
{
	wlc_scan_info_t *scan_pub = scan_info->scan_pub;
	uint32 scan_dur;

	scan_dur = wlc_get_curr_scan_time(wlc);

	/* Record scan end */
	scan_pub->wlc_scan_cmn->scan_stop_time = OSL_SYSUPTIME();

	/* For power stats, accumulate duration in the appropriate bucket */
	if (PWRSTATS_ENAB(wlc->pub)) {
		scan_data_t *scan_data = NULL;

		/* Accumlate this scan in the appropriate pwr_stats bucket */
		if (FALSE);
#ifdef WLPFN
#ifdef WL_EXCESS_PMWAKE
		else if (WLPFN_ENAB(wlc->pub) && wl_pfn_scan_in_progress(wlc->pfn)) {

			wlc_excess_pm_wake_info_t *epmwake = wlc->excess_pmwake;
			scan_data = &scan_info->scan_stats->pno_scans[0];

			epmwake->pfn_scan_ms += scan_dur;

			if (epmwake->pfn_alert_enable && ((epmwake->pfn_scan_ms -
				epmwake->pfn_alert_thresh_ts) >	epmwake->pfn_alert_thresh)) {
				wlc_generate_pm_alert_event(wlc, PFN_ALERT_THRESH_EXCEEDED,
					NULL, 0);
				/* Disable further events */
				epmwake->pfn_alert_enable = FALSE;
			}
		}
#endif /* WL_EXCESS_PMWAKE */
#endif /* WLPFN */
		else if (AS_IN_PROGRESS(wlc)) {
			wlc_bsscfg_t *cfg = AS_IN_PROGRESS_CFG(wlc);
			wlc_assoc_t *as = cfg->assoc;

			if (as->type == AS_ROAM) {
				scan_data = &scan_info->scan_stats->roam_scans;
#ifdef WL_EXCESS_PMWAKE
				wlc->excess_pmwake->roam_ms += scan_dur;
				wlc_check_roam_alert_thresh(wlc);
#endif /* WL_EXCESS_PMWAKE */
			}
			else
				scan_data = &scan_info->scan_stats->assoc_scans;
		}
		else if (scan_pub->wlc_scan_cmn->usage == SCAN_ENGINE_USAGE_NORM ||
			scan_pub->wlc_scan_cmn->usage == SCAN_ENGINE_USAGE_ESCAN)
			scan_data = &scan_info->scan_stats->user_scans;
		else
			scan_data = &scan_info->scan_stats->other_scans;

		if (scan_data) {
			scan_data->count++;
			/* roam scan in ms */
			if (scan_data == &scan_info->scan_stats->roam_scans)
				scan_data->dur += scan_dur;
			else
				scan_data->dur += (scan_dur * 1000); /* Scale to usec */
		}
	}

	return;
}

int
wlc_pwrstats_get_scan(wlc_scan_info_t *scan, uint8 *destptr, int destlen)
{
	scan_info_t *scani = (scan_info_t *)scan->scan_priv;
	wl_pwr_scan_stats_t *scan_stats = scani->scan_stats;
	uint16 taglen = sizeof(wl_pwr_scan_stats_t);

	/* Make sure there's room for this section */
	if (destlen < (int)ROUNDUP(sizeof(wl_pwr_scan_stats_t), sizeof(uint32)))
		return BCME_BUFTOOSHORT;

	/* Update common structure fields and copy into the destination */
	scan_stats->type = WL_PWRSTATS_TYPE_SCAN;
	scan_stats->len = taglen;
	memcpy(destptr, scan_stats, taglen);

	/* Report use of this segment (plus padding) */
	return (ROUNDUP(taglen, sizeof(uint32)));
}

#endif /* STA */

/* abort the current scan, and return to home channel */
void
wlc_scan_abort(wlc_scan_info_t *wlc_scan_info, int status)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
#if defined(WLP2P) || defined(WLRSDB)
	wlc_bsscfg_t *scan_cfg = scan_info->bsscfg;
#endif 

	if (!SCAN_IN_PROGRESS(wlc_scan_info)) {
#ifdef WLRSDB
		/* Check for the other wlc scan in progress. If so, abort the
		 * other wlc scan..
		 */
		 if (RSDB_PARALLEL_SCAN_ON(scan_info)) {
			 wlc_info_t *otherwlc = wlc_rsdb_get_other_wlc(scan_info->wlc);
			 wlc_scan_info = otherwlc->scan;
			 if (SCAN_IN_PROGRESS(wlc_scan_info)) {
				scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
				BCM_REFERENCE(scan_cfg);
			 } else
				return;
		 } else {
			return;
		 }
#else
		return;
#endif /* WLRSDB */
	}

	WL_INFORM_SCAN(("wl%d: %s: aborting scan in progress\n", scan_info->unit, __FUNCTION__));
#ifdef WLRCC
	if ((WLRCC_ENAB(scan_info->wlc->pub)) && (scan_info->bsscfg->roam->n_rcc_channels > 0))
		scan_info->bsscfg->roam->rcc_valid = TRUE;
#endif
	if (SCANCACHE_ENAB(wlc_scan_info) &&
#ifdef WLP2P
	    !BSS_P2P_DISC_ENAB(scan_info->wlc, scan_cfg) &&
#endif
		TRUE) {
		wlc_scan_cache_result(scan_info);
	}

	wlc_scan_bss_list_free(scan_info);
	wlc_scan_terminate(wlc_scan_info, status);

	wlc_ht_obss_scan_update(scan_info, WLC_E_STATUS_ABORT);

}

void
wlc_scan_abort_ex(wlc_scan_info_t *wlc_scan_info, wlc_bsscfg_t *cfg, int status)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	if (scan_info->bsscfg == cfg)
		wlc_scan_abort(wlc_scan_info, status);
}

/* wlc_scan_terminate is called when user (from intr/dpc or ioctl) requests to
 * terminate the scan. However it may also be invoked from its own call chain
 * from which tx status processing is executed. Use the flag TERMINATE to prevent
 * it from being re-entered.
 */
/* Driver's primeter lock will prevent wlc_scan_terminate from being invoked from
 * intr/dpc or ioctl when a timer callback is running. However it may be invoked
 * from the wlc_scantimer call chain in which tx status processing is executed.
 * So use the flag IN_TMR_CB to prevent wlc_scan_terminate being re-entered.
 */
void
wlc_scan_terminate(wlc_scan_info_t *wlc_scan_info, int status)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	int err;

	if (!SCAN_IN_PROGRESS(wlc_scan_info))
		return;

	/* ignore if already in terminate/finish process */
	if (wlc_scan_info->state & SCAN_STATE_TERMINATE) {
		WL_SCAN(("wl%d: %s: ignore wlc_scan_terminate request\n",
		         scan_info->unit, __FUNCTION__));
		return;
	}

	/* protect wlc_scan_terminate being recursively called from the
	 * callchain
	 */

	wlc_scan_info->state |= SCAN_STATE_TERMINATE;

	/* defer the termination if called from the timer callback */
	if (wlc_scan_info->state & SCAN_STATE_IN_TMR_CB) {
		WL_SCAN(("wl%d: %s: defer wlc_scan_terminate request\n",
		         scan_info->unit, __FUNCTION__));
		scan_info->status = status;
		return;
	}

	/* abort the current scan, and return to home channel */
	WL_INFORM_SCAN(("wl%d: %s: terminating scan in progress\n", scan_info->unit, __FUNCTION__));

#ifdef WLSCAN_PS
	/* scan is done. now reset the cores */
	if (WLSCAN_PS_ENAB(scan_info->wlc->pub))
		wlc_scan_ps_config_cores(scan_info, FALSE);
#endif

	/* If we are switching back to radar home_chanspec
	 * because:
	 * 1. STA scans (normal/Join/Roam) aborted with
	 * atleast one local 11H AP in radar channel,
	 * 2. Scan is not join/roam.
	 * turn radar_detect ON.
	 * NOTE: For Join/Roam radar_detect ON is done
	 * at later point in wlc_roam_complete() or
	 * wlc_set_ssid_complete(), when STA succesfully
	 * associates to upstream AP.
	 */
	if (WL11H_AP_ENAB(scan_info->wlc) &&
		WLC_APSTA_ON_RADAR_CHANNEL(scan_info->wlc) &&
#ifdef STA
		!AS_IN_PROGRESS(scan_info->wlc) &&
#endif
		wlc_radar_chanspec(scan_info->wlc->cmi, scan_info->wlc->home_chanspec)) {
		WL_REGULATORY(("wl%d: %s Join/scan aborted back"
			"to home channel dfs ON\n",
			scan_info->wlc->pub->unit, __FUNCTION__));
		wlc_set_dfs_cacstate(scan_info->wlc->dfs, ON);
	}

	/* clear scan ready flag */
	wlc_scan_info->state &= ~SCAN_STATE_READY;

	/* Cancel the MSCH request */
	if (scan_info->msch_req_hdl) {
		if ((err = wlc_msch_timeslot_unregister(scan_info->wlc->msch_info,
			&scan_info->msch_req_hdl)) != BCME_OK) {
			WL_ERROR(("wl%d: %s: MSCH timeslot unregister failed, err %d\n",
			         scan_info->unit, __FUNCTION__, err));
		}

		/* resume normal tx queue operation */
		wlc_scan_excursion_end(scan_info);

		scan_info->msch_req_hdl = NULL;
	}

	scan_info->state = WLC_SCAN_STATE_ABORT;
	scan_info->channel_idx = -1;
#if defined(STA)
	wlc_scan_time_upd(scan_info->wlc, scan_info);
#endif /* STA */
	wlc_scan_info->in_progress = FALSE;

	wlc_phy_hold_upd(SCAN_GET_PI_PTR(scan_info), PHY_HOLD_FOR_SCAN, FALSE);
#ifdef WLOLPC
	/* notify scan just terminated - if needed, kick off new cal */
	if (OLPC_ENAB(scan_info->wlc)) {
		wlc_olpc_eng_hdl_chan_update(scan_info->wlc->olpc_info);
	}
#endif /* WLOLPC */

	wlc_scan_ssidlist_free(scan_info);

#ifdef STA
	wlc_scan_set_wake_ctrl(scan_info);
	WL_MPC(("wl%d: %s: SCAN_IN_PROGRESS==FALSE, update mpc\n", scan_info->unit, __FUNCTION__));
	wlc_scan_radio_mpc_upd(scan_info);
#endif /* STA */

	/* abort PM indication process */
	wlc_scan_info->state &= ~SCAN_STATE_PSPEND;
	/* abort tx suspend delay process */
	wlc_scan_info->state &= ~SCAN_STATE_DLY_WSUSPEND;
	/* abort TX FIFO suspend process */
	wlc_scan_info->state &= ~SCAN_STATE_WSUSPEND;
	/* delete a future timer explictly */
	WLC_SCAN_DEL_TIMER(scan_info);
#ifdef STA
	wlc_scan_callback(scan_info, status);
#endif /* STA */

	scan_info->state = WLC_SCAN_STATE_START;

	wlc_scan_info->state &= ~SCAN_STATE_TERMINATE;
}

static void
wlc_scantimer(void *arg)
{
	scan_info_t *scan_info = (scan_info_t *)arg;
	wlc_scan_info_t	*wlc_scan_info = scan_info->scan_pub;
	wlc_info_t *wlc = scan_info->wlc;
	int status = WLC_E_STATUS_SUCCESS;
	/* DBGONLY(char chanbuf[CHANSPEC_STR_LEN];) */

	UNUSED_PARAMETER(wlc);

	WL_SCAN_ENT(scan_info, ("wl%d: %s: enter, state 0x%x tsf %u\n",
	                        scan_info->unit, __FUNCTION__,
	                        wlc_scan_info->state,
	                        SCAN_GET_TSF_TIMERLOW(scan_info)));

	wlc_scan_info->state |= SCAN_STATE_IN_TMR_CB;

	if (SCAN_DEVICEREMOVED(scan_info)) {
		WL_ERROR(("wl%d: %s: dead chip\n", scan_info->unit, __FUNCTION__));
		if (SCAN_IN_PROGRESS(wlc_scan_info)) {
			wlc_scan_bss_list_free(scan_info);
			wlc_scan_callback(scan_info, WLC_E_STATUS_ABORT);

		}
		SCAN_WL_DOWN(scan_info);
		goto exit;
	}

	if (scan_info->state == WLC_SCAN_STATE_PARTIAL) {
		if (_wlc_scan_schd_channels(wlc_scan_info) == BCME_OK) {
			goto exit;
		}

		WL_ERROR(("wl%d: %s: Error, failed to scan %d channels\n",
			scan_info->unit, __FUNCTION__,
			(scan_info->channel_num - scan_info->channel_idx)));

		scan_info->state = WLC_SCAN_STATE_COMPLETE;
	}

	if (scan_info->state == WLC_SCAN_STATE_ABORT) {
		WL_SCAN_ENT(scan_info, ("wl%d: %s: move state to START\n",
		                        scan_info->unit, __FUNCTION__));
		scan_info->state = WLC_SCAN_STATE_START;
		goto exit;
	}

	if (scan_info->state == WLC_SCAN_STATE_START) {
		scan_info->state = WLC_SCAN_STATE_SEND_PROBE;
		scan_info->pass = scan_info->npasses;
	}

	if (scan_info->state == WLC_SCAN_STATE_SEND_PROBE) {
		if (scan_info->pass > 0) {
#ifdef WLSCAN_PS
			/* scan started, switch to one tx/rx core */
			if (WLSCAN_PS_ENAB(scan_info->wlc->pub))
				wlc_scan_ps_config_cores(scan_info, TRUE);
#endif /* WLSCAN_PS */
			wlc_scan_do_pass(scan_info, scan_info->cur_scan_chanspec);
			scan_info->pass--;
			goto exit;
		} else {
			if (scan_info->timeslot_id) {
				if (BCME_OK != wlc_msch_timeslot_cancel(scan_info->wlc->msch_info,
					scan_info->msch_req_hdl, scan_info->timeslot_id))
					goto exit;

				scan_info->timeslot_id = 0;

				/* reset radar clear flag since we are leaving this channel */
				wlc_scan_info->state &= ~SCAN_STATE_RADAR_CLEAR;

				/* resume normal tx queue operation */
				wlc_scan_excursion_end(scan_info);

				if (scan_info->chanspec_list[scan_info->channel_idx - 1] !=
					scan_info->cur_scan_chanspec) {
					/* scan complete for current channel */
					scan_info->state = WLC_SCAN_STATE_CHN_SCAN_DONE;
				} else {
					/* scan complete for current request */
					/* clear msch request handle */
					scan_info->msch_req_hdl = NULL;

					if (scan_info->channel_idx < scan_info->channel_num) {
						scan_info->state = WLC_SCAN_STATE_PARTIAL;
						WLC_SCAN_ADD_TIMER(scan_info, 0, 0);
					}
					else {
						scan_info->state = WLC_SCAN_STATE_COMPLETE;
						WLC_SCAN_ADD_TIMER(scan_info, 0, 0);
					}
					goto exit;
				}
			}
		}
	}

	if (scan_info->state != WLC_SCAN_STATE_COMPLETE) {
		goto exit;
	}

	scan_info->state = WLC_SCAN_STATE_START;
	wlc_scan_info->state |= SCAN_STATE_TERMINATE;

#if defined(WLMSG_INFORM)
	if (scan_info->nssid == 1) {
		char ssidbuf[SSID_FMT_BUF_LEN];
		wlc_ssid_t *ssid = scan_info->ssid_list;

		if (WL_INFORM_ON())
			wlc_format_ssid(ssidbuf, ssid->SSID, ssid->SSID_len);
		WL_INFORM_SCAN(("wl%d: %s: %s scan done, %d total responses for SSID \"%s\"\n",
		           scan_info->unit, __FUNCTION__,
		           (wlc_scan_info->state & SCAN_STATE_PASSIVE) ? "Passive" : "Active",
		           SCAN_RESULT_MEB(scan_info, count), ssidbuf));
	} else {
		WL_INFORM_SCAN(("wl%d: %s: %s scan done, %d total responses for SSIDs:\n",
		           scan_info->unit, __FUNCTION__,
		           (wlc_scan_info->state & SCAN_STATE_PASSIVE) ? "Passive" : "Active",
		           SCAN_RESULT_MEB(scan_info, count)));
		if (WL_INFORM_ON())
			wlc_scan_print_ssids(scan_info->ssid_list, scan_info->nssid);
	}
#endif 

#ifdef WLSCAN_PS
	/* scan is done, revert core mask */
	if (WLSCAN_PS_ENAB(scan_info->wlc->pub))
		wlc_scan_ps_config_cores(scan_info, FALSE);
#endif

	wlc_scan_info->state &= ~SCAN_STATE_READY;
	scan_info->channel_idx = -1;
#if defined(STA)
	wlc_scan_time_upd(wlc, scan_info);
#endif /* STA */
	wlc_scan_info->in_progress = FALSE;
	wlc_phy_hold_upd(SCAN_GET_PI_PTR(scan_info), PHY_HOLD_FOR_SCAN, FALSE);
#ifdef WLOLPC
	/* notify scan just terminated - if needed, kick off new cal */
	if (OLPC_ENAB(scan_info->wlc)) {
		wlc_olpc_eng_hdl_chan_update(scan_info->wlc->olpc_info);
	}
#endif /* WLOLPC */
#ifdef WLMESH
	/* prepare peering list */
	if (WLMESH_ENAB(wlc->pub) && BSSCFG_MESH(scan_info->bsscfg))
		wlc_mpeer_parse_scan_info(wlc->mesh_info, scan_info->bsscfg,
			SCAN_RESULT_PTR(scan_info));
#endif /* WLMESH */

	wlc_scan_ssidlist_free(scan_info);

	/* allow core to sleep again (no more solicited probe responses) */
	wlc_scan_set_wake_ctrl(scan_info);

#ifdef WLCQ
	/* resume any channel quality measurement */
	wlc_lq_channel_qa_sample_req(wlc);
#endif /* WLCQ */

#ifdef STA
	/* disable radio for non-association scan.
	 * Association scan will continue with JOIN process and
	 * end up at MACEVENT: WLC_E_SET_SSID
	 */
	WL_MPC(("wl%d: scan done, SCAN_IN_PROGRESS==FALSE, update mpc\n", scan_info->unit));
	wlc->mpc_scan = FALSE;
	wlc_scan_radio_mpc_upd(scan_info);
	wlc_scan_radio_upd(scan_info);	/* Bring down the radio immediately */
#endif /* STA */
	wlc_scan_callback(scan_info, status);

	wlc_scan_info->state &= ~SCAN_STATE_TERMINATE;

exit:
	wlc_scan_info->state &= ~SCAN_STATE_IN_TMR_CB;

	/* You can't read hardware registers, tsf in this case, if you don't have
	 * clocks enabled. e.g. you are down. The exit path of this function will
	 * result in a down'ed driver if we are just completing a scan and MPC is
	 * enabled and meets the conditions to down the driver (typically no association).
	 * While typically MPC down is deferred for a time delay, the call of the
	 * wlc_scan_mpc_upd() in the scan completion path will force an immediate down
	 * before we get to the exit of this function.  For this reason, we condition
	 * the read of the tsf timer in the debugging output on the presence of
	 * hardware clocks.
	 */
	WL_SCAN_ENT(scan_info, ("wl%d: %s: exit, state 0x%x tsf %u\n",
	                        scan_info->unit, __FUNCTION__,
	                        wlc_scan_info->state,
	                        SCAN_GET_TSF_TIMERLOW(scan_info)));
}

static void
wlc_scan_act(scan_info_t *si, uint dwell)
{
	wlc_info_t *wlc = si->wlc;
	/* real scan request */
	if (si->act_cb == NULL) {
		wlc_scan_sendprobe(si);
		goto set_timer;
	}

	/* other requests using the scan engine */
	(si->act_cb)(wlc, si->act_cb_arg, &dwell);


set_timer:
	WLC_SCAN_ADD_TIMER(si, dwell, 0);
}

void
wlc_scan_radar_clear(wlc_scan_info_t *wlc_scan_info)
{
	scan_info_t	*scan_info = (scan_info_t *) wlc_scan_info->scan_priv;
	uint32		channel_dwelltime;
	uint32 cur_l, cur_h;
	uint32 elapsed_time, remaining_time, active_time;
	DBGONLY(char chanbuf[CHANSPEC_STR_LEN]; )

	/* if we are not on a radar quiet channel,
	 * or a passive scan was requested,
	 * or we already processed the radar clear signal,
	 * or it is not a prohibited channel,
	 * then do nothing
	 */
	if ((wlc_scan_valid_chanspec_db(scan_info, SCAN_BAND_PI_RADIO_CHANSPEC(scan_info)) &&
	     !wlc_scan_quiet_chanspec(scan_info, SCAN_BAND_PI_RADIO_CHANSPEC(scan_info))) ||
	    (wlc_scan_info->state & (SCAN_STATE_PASSIVE | SCAN_STATE_RADAR_CLEAR)))
		return;

	/* if we are not in the channel scan portion of the scan, do nothing */
	if (scan_info->state != WLC_SCAN_STATE_LISTEN)
		return;

	/* if there is not enough time remaining for a probe,
	 * do nothing unless explicitly enabled
	 */
	if (scan_info->force_active == FALSE) {
		SCAN_READ_TSF(scan_info, &cur_l, &cur_h);

		elapsed_time = (cur_l - scan_info->start_tsf) / 1000;

		channel_dwelltime = CHANNEL_PASSIVE_DWELLTIME(scan_info);

		if (elapsed_time > channel_dwelltime)
			remaining_time = 0;
		else
			remaining_time = channel_dwelltime - elapsed_time;

		if (remaining_time < WLC_SCAN_MIN_PROBE_TIME)
			return;

		if (remaining_time >= CHANNEL_ACTIVE_DWELLTIME(scan_info)) {
			active_time = CHANNEL_ACTIVE_DWELLTIME(scan_info)/scan_info->npasses;
			/* if remaining time is MSCH_CANCELSLOT_PREPARE or more than
			 * active dwell time, cancel current time slot to save
			 * after probing for active dwell time. Otherwise do not canel.
			 */
			channel_dwelltime = CHANNEL_ACTIVE_DWELLTIME(scan_info);
			if (remaining_time < channel_dwelltime + MSCH_CANCELSLOT_PREPARE)
				scan_info->timeslot_id = 0;
		} else {
			active_time = remaining_time;
			scan_info->timeslot_id = 0;
		}
	}
	else {
		active_time = CHANNEL_ACTIVE_DWELLTIME(scan_info);
		scan_info->timeslot_id = 0;
	}

	scan_info->pass = scan_info->npasses;
	if (scan_info->pass > 0) {
		/* everything is ok to switch to an active scan */
		wlc_scan_info->state |= SCAN_STATE_RADAR_CLEAR;
		scan_info->state = WLC_SCAN_STATE_SEND_PROBE;

		WLC_SCAN_DEL_TIMER(scan_info);

		SCAN_TO_MUTE(scan_info, OFF, 0);

		wlc_scan_act(scan_info, active_time);
		scan_info->pass--;

		WL_REGULATORY(("wl%d: wlc_scan_radar_clear: rcvd beacon on radar chanspec %s,"
			" converting to active scan, %d ms left\n", scan_info->unit,
			wf_chspec_ntoa_ex(SCAN_BAND_PI_RADIO_CHANSPEC(scan_info), chanbuf),
			active_time));
	}
}


/*
 * Returns default channels for this locale in band 'band'
 */
void
wlc_scan_default_channels(wlc_scan_info_t *wlc_scan_info, chanspec_t chanspec_start,
int band, chanspec_t *chanspec_list, int *channel_count)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	int num;

	ASSERT(wlc_scan_valid_chanspec_db(scan_info, chanspec_start));

	/* enumerate all the active (non-quiet) channels first */
	wlc_scan_channels(scan_info, chanspec_list, &num, MAXCHANNEL,
		chanspec_start, CHAN_TYPE_CHATTY, band);
	*channel_count = num;

	/* if scan_info->passive_time = 0, skip the passive channels */
	if (!scan_info->passive_time)
		return;

	/* enumerate all the passive (quiet) channels second */
	wlc_scan_channels(scan_info, &chanspec_list[num], &num,
		(MAXCHANNEL - *channel_count), chanspec_start, CHAN_TYPE_QUIET, band);
	*channel_count += num;
}

/*
 * Scan channels are always 2.5/5/10/20MHZ, so return the valid set of 2.5/5/10/2020MHZ channels
 * for this locale. This function will return the channels available in the band of argument
 * 'band' band can be WLC_BAND_ALL WLC_BAND_2G or WLC_BAND_5G
 */
static void
wlc_scan_channels(scan_info_t *scan_info, chanspec_t *chanspec_list,
	int *pchannel_num, int channel_max, chanspec_t chanspec_start, int channel_type,
	int band)
{
	uint bandunit;
	uint channel;
	chanspec_t chanspec;
	int num = 0;
	uint i;

	/* chanspec start should be for a 2.5/5/10/20MHZ channel */
	ASSERT(CHSPEC_ISLE20(chanspec_start));
	bandunit = CHSPEC_WLCBANDUNIT(chanspec_start);
	for (i = 0; i < SCAN_NBANDS(scan_info); i++) {
		channel = CHSPEC_CHANNEL(chanspec_start);
		chanspec = BSSCFG_MINBW_CHSPEC(scan_info->wlc, scan_info->bsscfg, channel);
		while (num < channel_max) {
			if (WLC_LE20_VALID_SCAN_CHANNEL_IN_BAND(scan_info, bandunit,
				CHSPEC_BW(chanspec), channel) &&
			    ((channel_type == CHAN_TYPE_CHATTY &&
				!wlc_scan_quiet_chanspec(scan_info, chanspec)) ||
			     (channel_type == CHAN_TYPE_QUIET &&
				wlc_scan_quiet_chanspec(scan_info, chanspec))))
					chanspec_list[num++] = chanspec;

			channel = (channel + 1) % MAXCHANNEL;
			chanspec = BSSCFG_MINBW_CHSPEC(scan_info->wlc, scan_info->bsscfg, channel);
			if (wf_chspec_ctlchan(chanspec) == wf_chspec_ctlchan(chanspec_start))
				break;
		}

		/* only find channels for one band */
		if (!SCAN_IS_MBAND_UNLOCKED(scan_info))
			break;
		if (band == WLC_BAND_ALL) {
			/* prepare to find the other band's channels */
			bandunit = ((bandunit == 1) ? 0 : 1);
			chanspec_start = BSSCFG_MINBW_CHSPEC(scan_info->wlc, scan_info->bsscfg, 0);
		} else
			/* We are done with current band. */
			break;
	}

	*pchannel_num = num;
}

static uint
wlc_scan_prohibited_channels(scan_info_t *scan_info, chanspec_t *chanspec_list,
	int channel_max)
{
	WLC_BAND_T *band;
	uint channel, maxchannel, i, j;
	chanvec_t sup_chanvec, chanvec;
	int num = 0;

	if (!IS_AUTOCOUNTRY_ENAB(scan_info))
		return 0;

	band = SCAN_GET_CUR_BAND(scan_info);
	for (i = 0; i < SCAN_NBANDS(scan_info); i++) {
		const char *acdef = wlc_scan_11d_get_autocountry_default(scan_info);

		bzero(&sup_chanvec, sizeof(chanvec_t));
		/* Get the list of all the channels in autocountry_default
		 * and supported by phy
		 */
		phy_utils_chanspec_band_validch(
			(phy_info_t *)SCAN_GET_PI_BANDUNIT(scan_info, band->bandunit),
			band->bandtype, &sup_chanvec);
		if (!wlc_scan_get_chanvec(scan_info, acdef, band->bandtype, &chanvec))
			return 0;

		for (j = 0; j < sizeof(chanvec_t); j++)
			sup_chanvec.vec[j] &= chanvec.vec[j];

		maxchannel = BAND_2G(band->bandtype) ? (CH_MAX_2G_CHANNEL + 1) : MAXCHANNEL;
		ASSERT((int)maxchannel <= channel_max);
		for (channel = 0; channel < maxchannel; channel++) {
			if (isset(sup_chanvec.vec, channel) &&
				!WLC_LE20_VALID_SCAN_CHANNEL_IN_BAND(scan_info, band->bandunit,
					WLC_ULB_GET_BSS_MIN_BW(scan_info->wlc,
					scan_info->bsscfg), channel)) {
				chanspec_list[num++] = BSSCFG_MINBW_CHSPEC(scan_info->wlc,
					scan_info->bsscfg, channel);
				if (num >= channel_max)
					return num;
			}
		}
		band = SCAN_GET_BANDSTATE(scan_info, SCAN_OTHERBANDUNIT(scan_info));
	}

	return num;
}

#ifdef CNTRY_DEFAULT
int
wlc_scan_prohibited_channels_get(wlc_scan_info_t *scan, chanspec_t *chanspec_list,
	int channel_max)
{
	scan_info_t *scan_info = (scan_info_t *)scan->scan_priv;
	return (int)wlc_scan_prohibited_channels(scan_info, chanspec_list, channel_max);
}
#endif /* CNTRY_DEFAULT */

bool
wlc_scan_inprog(wlc_info_t *wlc_info)
{
	return SCAN_IN_PROGRESS(wlc_info->scan);
}

static int
wlc_scan_apply_scanresults(scan_info_t *scan_info, int status)
{
#ifdef STA
	int idx;
	wlc_bsscfg_t *cfg;
#endif /* STA */
	wlc_bsscfg_t *scan_cfg;
	wlc_scan_info_t *wlc_scan_info;
	wlc_info_t *wlc = scan_info->wlc;
	BCM_REFERENCE(status);

	(void)wlc_scan_info;

	/* Store for later use */
	scan_cfg = scan_info->bsscfg;

	UNUSED_PARAMETER(wlc);
	UNUSED_PARAMETER(scan_cfg);

#ifdef WLRSDB
	/* Move scan results to scan request wlc in case of parallel scanning. */
	if (RSDB_PARALLEL_SCAN_ON(scan_info)) {
		scan_info->scan_cmn->num_of_cbs--;
		WL_SCAN(("wl%d.%d:%s Num of CBs pending:%d\n", wlc->pub->unit,
		         scan_cfg->_idx, __FUNCTION__, scan_info->scan_cmn->num_of_cbs));
		/* TODO: remove this backward dependency */
		wlc_scan_utils_scan_complete(wlc, scan_cfg);
		if (scan_info->scan_cmn->num_of_cbs != 0) {
			return BCME_BUSY;
		}
		/* Need to pickup the wlc & wlc_scan_info where the scan request
		 * is given to use scan results below.
		 */
		scan_info = (scan_info_t*)wlc->scan->scan_priv;
	}
#endif /* WLRSDB */

	wlc_scan_info = scan_info->scan_pub;

#ifdef STA
#ifdef WL11D
	/* If we are in 802.11D mode and we are still waiting to find a
	 * valid Country IE, then take this opportunity to parse these
	 * scan results for one.
	 */
	if (IS_AUTOCOUNTRY_ENAB(scan_info))
		wlc_scan_11d_scan_complete(scan_info, WLC_E_STATUS_SUCCESS);
#endif /* WL11D */
#endif /* STA */

	wlc_ht_obss_scan_update(scan_info, WLC_E_STATUS_SUCCESS);

	/* Don't fill the cache with results from a P2P discovery scan since these entries
	 * are short-lived. Also, a P2P association cannot use Scan cache
	 */
	if (SCANCACHE_ENAB(wlc_scan_info) &&
#ifdef WLP2P
	    !BSS_P2P_DISC_ENAB(wlc, scan_cfg) &&
#endif
	    TRUE) {
		wlc_scan_cache_result(scan_info);
	}

#ifdef STA
	/* if this was a broadcast scan across all channels,
	 * update the roam cache, if possible
	 */
	if (ETHER_ISBCAST(&wlc_scan_info->bssid) &&
	    wlc_scan_info->wlc_scan_cmn->bss_type == DOT11_BSSTYPE_ANY) {
		SCAN_FOREACH_AS_STA(scan_info, idx, cfg) {
			wlc_roam_t *roam = cfg->roam;
			if (roam && roam->roam_scan_piggyback &&
			    roam->active && !roam->fullscan_count) {
				WL_ASSOC(("wl%d: %s: Building roam cache with"
				          " scan results from broadcast scan\n",
				          scan_info->unit, __FUNCTION__));
				/* this counts as a full scan */
				roam->fullscan_count = 1;
				/* update the roam cache */
				wlc_build_roam_cache(cfg, SCAN_RESULT_PTR(scan_info));
			}
		}
	}
#endif /* STA */


	return BCME_OK;
}

static void
wlc_scan_callback(scan_info_t *scan_info, uint status)
{
	scancb_fn_t cb = scan_info->cb;
	void *cb_arg = scan_info->cb_arg;
	wlc_bsscfg_t *cfg = scan_info->bsscfg;
#if defined(WLPFN)
	wlc_info_t *wlc = scan_info->wlc;
#endif
	int scan_completed;
#ifdef WLPFN
	bool is_pfn_in_progress;
	/* Check the state before cb is called as the state
	 * changes after cb
	 */
	is_pfn_in_progress = WLPFN_ENAB(wlc->pub) &&
	                     wl_pfn_scan_in_progress(wlc->pfn);
#endif /* WLPFN */

	scan_completed = wlc_scan_apply_scanresults(scan_info, status);

	scan_info->bsscfg = NULL;
	scan_info->cb = NULL;
	scan_info->cb_arg = NULL;

	/* Registered Scan callback function should take care of
	 * sending a BSS event to the interface attached to it.
	 */
	if (cb != NULL)
		(cb)(cb_arg, status, cfg);
	else if (scan_completed == BCME_OK) {
		/* Post a BSS event if an interface is attached to it */
		wlc_scan_bss_mac_event(scan_info, cfg, WLC_E_SCAN_COMPLETE, NULL,
		status, 0, 0, NULL, 0);
	}

	SCAN_RESTORE_BSSCFG(scan_info, cfg);

	/* reset scan engine usage */
	if (scan_completed == BCME_OK) {
#ifdef WLRSDB
		if (RSDB_PARALLEL_SCAN_ON(scan_info))
			scan_info = (scan_info_t *)cfg->wlc->scan->scan_priv;
#endif
		scan_info->scan_pub->wlc_scan_cmn->usage = SCAN_ENGINE_USAGE_NORM;
		/* Free the BSS's in the scan_results. Use the scan info where
		 * scan request is given.
		 */
		wlc_scan_bss_list_free(scan_info);

#ifdef ANQPO
		if (SCAN_ANQPO_ENAB(scan_info) &&
			scan_info->scan_pub->wlc_scan_cmn->is_hotspot_scan) {
			wl_scan_anqpo_scan_stop(scan_info);
		}
#endif /* ANQPO */
	}
#ifdef WLPFN
	if (WLPFN_ENAB(wlc->pub) && !is_pfn_in_progress)
		wl_notify_pfn(wlc);
#endif /* WLPFN */

	/* allow scanmac to be updated */
	wlc_scanmac_update(scan_info->scan_pub);
}

chanspec_t
wlc_scan_get_current_chanspec(wlc_scan_info_t *wlc_scan_info)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;

	if (scan_info->channel_idx >= scan_info->channel_num)
		return scan_info->chanspec_list[scan_info->channel_num - 1];

	return scan_info->chanspec_list[scan_info->channel_idx];
}

int
wlc_scan_ioctl(wlc_scan_info_t *wlc_scan_info,
	int cmd, void *arg, int len, struct wlc_if *wlcif)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	wlc_info_t *wlc = scan_info->wlc;
	int bcmerror = 0;
	int val = 0, *pval;
	bool bool_val;

	/* default argument is generic integer */
	pval = (int *) arg;
	/* This will prevent the misaligned access */
	if (pval && (uint32)len >= sizeof(val))
		bcopy(pval, &val, sizeof(val));

	/* bool conversion to avoid duplication below */
	bool_val = (val != 0);
	BCM_REFERENCE(bool_val);

	switch (cmd) {
#ifdef STA
	case WLC_SET_PASSIVE_SCAN:
		scan_info->scan_cmn->defaults.passive = (bool_val ? 1 : 0);
		break;

	case WLC_GET_PASSIVE_SCAN:
		ASSERT(pval != NULL);
		if (pval != NULL)
			*pval = scan_info->scan_cmn->defaults.passive;
		else
			bcmerror = BCME_BADARG;
		break;

	case WLC_GET_SCANSUPPRESS:
		ASSERT(pval != NULL);
		if (pval != NULL)
			*pval = wlc_scan_info->state & SCAN_STATE_SUPPRESS ? 1 : 0;
		else
			bcmerror = BCME_BADARG;
		break;

	case WLC_SET_SCANSUPPRESS:
		if (val)
			wlc_scan_info->state |= SCAN_STATE_SUPPRESS;
		else
			wlc_scan_info->state &= ~SCAN_STATE_SUPPRESS;
		break;

	case WLC_GET_SCAN_CHANNEL_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_assoc_time", NULL, 0,
			arg, len, IOV_GET, wlcif);
		break;

	case WLC_SET_SCAN_CHANNEL_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_assoc_time", NULL, 0,
			arg, len, IOV_SET, wlcif);
		break;

	case WLC_GET_SCAN_UNASSOC_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_unassoc_time", NULL, 0,
			arg, len, IOV_GET, wlcif);
		break;

	case WLC_SET_SCAN_UNASSOC_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_unassoc_time", NULL, 0,
			arg, len, IOV_SET, wlcif);
		break;
#endif /* STA */

	case WLC_GET_SCAN_PASSIVE_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_passive_time", NULL, 0,
			arg, len, IOV_GET, wlcif);
		break;

	case WLC_SET_SCAN_PASSIVE_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_passive_time", NULL, 0,
			arg, len, IOV_SET, wlcif);
		break;

	case WLC_GET_SCAN_HOME_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_home_time", NULL, 0,
			arg, len, IOV_GET, wlcif);
		break;

	case WLC_SET_SCAN_HOME_TIME:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_home_time", NULL, 0,
			arg, len, IOV_SET, wlcif);
		break;

	case WLC_GET_SCAN_NPROBES:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_nprobes", NULL, 0,
			arg, len, IOV_GET, wlcif);
		break;

	case WLC_SET_SCAN_NPROBES:
		ASSERT(arg != NULL);
		bcmerror = wlc_iovar_op(wlc, "scan_nprobes", NULL, 0,
			arg, len, IOV_SET, wlcif);
		break;

	default:
		bcmerror = BCME_UNSUPPORTED;
		break;
	}
	return bcmerror;
}


#ifdef WLC_SCAN_IOVARS
static int
wlc_scan_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, struct wlc_if *wlcif)
{
	scan_info_t *scan_info = (scan_info_t *)hdl;
	int err = 0;
	int32 int_val = 0;
	bool bool_val = FALSE;
	int32 *ret_int_ptr;
	wlc_info_t *wlc = scan_info->wlc;

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(vi);
	BCM_REFERENCE(name);
	BCM_REFERENCE(val_size);
	BCM_REFERENCE(wlcif);

	/* convenience int and bool vals for first 4 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));
	bool_val = (int_val != 0) ? TRUE : FALSE;
	BCM_REFERENCE(bool_val);

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	/* Do the actual parameter implementation */
	switch (actionid) {
	case IOV_GVAL(IOV_PASSIVE):
		*ret_int_ptr = (int32)scan_info->scan_cmn->defaults.passive;
		break;

	case IOV_SVAL(IOV_PASSIVE):
		scan_info->scan_cmn->defaults.passive = (int8)int_val;
		break;

#ifdef STA
	case IOV_GVAL(IOV_SCAN_ASSOC_TIME):
		*ret_int_ptr = (int32)scan_info->scan_cmn->defaults.assoc_time;
		break;

	case IOV_SVAL(IOV_SCAN_ASSOC_TIME):
		scan_info->scan_cmn->defaults.assoc_time = (uint16)int_val;
		break;

	case IOV_GVAL(IOV_SCAN_UNASSOC_TIME):
		*ret_int_ptr = (int32)scan_info->scan_cmn->defaults.unassoc_time;
		break;

	case IOV_SVAL(IOV_SCAN_UNASSOC_TIME):
		scan_info->scan_cmn->defaults.unassoc_time = (uint16)int_val;
		break;
#endif /* STA */

	case IOV_GVAL(IOV_SCAN_PASSIVE_TIME):
		*ret_int_ptr = (int32)scan_info->scan_cmn->defaults.passive_time;
		break;

	case IOV_SVAL(IOV_SCAN_PASSIVE_TIME):
		scan_info->scan_cmn->defaults.passive_time = (uint16)int_val;
		break;

#ifdef STA
	case IOV_GVAL(IOV_SCAN_HOME_TIME):
		*ret_int_ptr = (int32)scan_info->scan_cmn->defaults.home_time;
		break;

	case IOV_SVAL(IOV_SCAN_HOME_TIME):
		scan_info->scan_cmn->defaults.home_time = (uint16)int_val;
		break;

	case IOV_GVAL(IOV_SCAN_NPROBES):
		*ret_int_ptr = (int32)scan_info->scan_cmn->defaults.nprobes;
		break;

	case IOV_SVAL(IOV_SCAN_NPROBES):
		scan_info->scan_cmn->defaults.nprobes = (int8)int_val;
		break;

	case IOV_GVAL(IOV_SCAN_FORCE_ACTIVE):
		*ret_int_ptr = (int32)scan_info->force_active;
		break;

	case IOV_SVAL(IOV_SCAN_FORCE_ACTIVE):
		scan_info->force_active = (int_val != 0);
		break;

#ifdef WLSCANCACHE
	case IOV_GVAL(IOV_SCANCACHE):
		*ret_int_ptr = scan_info->scan_pub->_scancache;
		break;

	case IOV_SVAL(IOV_SCANCACHE):
		if (SCANCACHE_SUPPORT(wlc->pub)) {
			scan_info->scan_pub->_scancache = bool_val;
#ifdef WL11K
			/* Enable Table mode beacon report in RRM cap if scancache enabled */
			if (WL11K_ENAB(wlc->pub)) {
				wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);

				ASSERT(cfg != NULL);

				wlc_rrm_update_cap(wlc->rrm_info, cfg);
			}
#endif /* WL11K */
		}
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_GVAL(IOV_SCANCACHE_TIMEOUT):
		if (SCANCACHE_ENAB(scan_info->scan_pub))
			*ret_int_ptr = (int32)wlc_scandb_timeout_get(scan_info->sdb);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_SCANCACHE_TIMEOUT):
		if (SCANCACHE_ENAB(scan_info->scan_pub))
			wlc_scandb_timeout_set(scan_info->sdb, (uint)int_val);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_SCANCACHE_CLEAR):
		/* scancache might be disabled while clearing the cache.
		 * So check for scancache_support instead of scancache_enab.
		 */
		if (SCANCACHE_SUPPORT(wlc->pub))
			wlc_scandb_clear(scan_info->sdb);
		else
			err = BCME_UNSUPPORTED;
		break;

#endif /* WLSCANCACHE */

	case IOV_GVAL(IOV_SCAN_ASSOC_TIME_DEFAULT):
		*ret_int_ptr = WLC_SCAN_ASSOC_TIME;
		break;

	case IOV_GVAL(IOV_SCAN_UNASSOC_TIME_DEFAULT):
		*ret_int_ptr = WLC_SCAN_UNASSOC_TIME;
		break;

#endif /* STA */

	case IOV_GVAL(IOV_SCAN_HOME_TIME_DEFAULT):
		*ret_int_ptr = WLC_SCAN_HOME_TIME;
		break;

	case IOV_GVAL(IOV_SCAN_PASSIVE_TIME_DEFAULT):
		*ret_int_ptr = WLC_SCAN_PASSIVE_TIME;
		break;
#ifdef STA
	case IOV_GVAL(IOV_SCAN_HOME_AWAY_TIME):
		*ret_int_ptr = (int32)scan_info->home_away_time;
		break;
	case IOV_SVAL(IOV_SCAN_HOME_AWAY_TIME):
		if (int_val <= 0)
			err = BCME_BADARG;
		else
			scan_info->home_away_time = (uint16)int_val;
		break;
#endif /* STA */

#ifdef WLRSDB
	case IOV_GVAL(IOV_SCAN_RSDB_PARALLEL_SCAN):
		if (RSDB_ENAB(wlc->pub))
			*ret_int_ptr  = scan_info->scan_cmn->rsdb_parallel_scan;
		else
			err = BCME_UNSUPPORTED;
		break;
	case IOV_SVAL(IOV_SCAN_RSDB_PARALLEL_SCAN):

#if defined(WLSCANCACHE)
		if (IS_EXTSTA_ENAB(scan_info) || SCANCACHE_ENAB(scan_info->scan_pub)) {
			WL_ERROR(("Parallel Scan is not supported with EXT sta or Scan Cache\n"));
			err = BCME_UNSUPPORTED;
		} else
#endif 
		{
		if (RSDB_ENAB(wlc->pub)) {
			if (ANY_SCAN_IN_PROGRESS(scan_info->scan_pub)) {
				err = BCME_BUSY;
			} else {
				scan_info->scan_cmn->rsdb_parallel_scan = bool_val;
			}
		} else
			err = BCME_UNSUPPORTED;
		}
		break;
#endif /* WLRSDB */

#ifdef WLSCAN_PS
	case IOV_GVAL(IOV_SCAN_PWRSAVE):
		*ret_int_ptr = (uint8)scan_info->scan_pwrsave_enable;
		break;
	case IOV_SVAL(IOV_SCAN_PWRSAVE):
		if (int_val < 0 || int_val > 1)
			err = BCME_BADARG;
		else
			scan_info->scan_pwrsave_enable = (uint8)int_val;
		break;
#endif /* WLSCAN_PS */
	case IOV_GVAL(IOV_SCANMAC):
		err = scanmac_get(scan_info, params, p_len, arg, len);
		break;
	case IOV_SVAL(IOV_SCANMAC):
		err = scanmac_set(scan_info, arg, len);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}
#endif /* WLC_SCAN_IOVARS */

static void
wlc_scan_sendprobe(scan_info_t *scan_info)
{
	int i;
	wlc_ssid_t *ssid;
	int n;
	wlc_bsscfg_t *cfg = scan_info->bsscfg;
	const struct ether_addr *da = &ether_bcast;
	const struct ether_addr *bssid = &ether_bcast;

	ASSERT(scan_info->pass >= 1);

	if (scan_info->extdscan) {
		ssid = &scan_info->ssid_list[scan_info->pass - 1];
		n = scan_info->nprobes;
	}
	else {
		ssid = scan_info->ssid_list;
		n = scan_info->nssid;
	}


	for (i = 0; i < n; i++) {
#ifdef WLFMC
		wlc_scan_info_t *wlc_scan_info = scan_info->scan_pub;
		/* in case of roaming reassoc, use unicast scans */
		if (WLFMC_ENAB(scan_info->wlc->pub) &&
			!ETHER_ISMULTI(&wlc_scan_info->bssid) &&
			(cfg->assoc->type == AS_ROAM) &&
			(cfg->roam->reason == WLC_E_REASON_INITIAL_ASSOC)) {
			da = &wlc_scan_info->bssid;
			bssid = &wlc_scan_info->bssid;
		}
#endif /* WLFMC */
		_wlc_scan_sendprobe(scan_info, cfg, ssid->SSID, ssid->SSID_len,
			&scan_info->sa_override, da, bssid, SCAN_PROBE_TXRATE(scan_info), NULL, 0);
		if (!scan_info->extdscan)
			ssid++;
	}
}

static bool
wlc_scan_usage_scan(wlc_scan_info_t *scan)
{
	return (NORM_IN_PROGRESS(scan) || ESCAN_IN_PROGRESS(scan));
}

static void
wlc_scan_do_pass(scan_info_t *scan_info, chanspec_t chanspec)
{
	uint32	channel_dwelltime = 0;
	uint32  dummy_tsf_h, start_tsf;
	char chanbuf[CHANSPEC_STR_LEN];
	BCM_REFERENCE(chanbuf);

	SCAN_READ_TSF(scan_info, &start_tsf, &dummy_tsf_h);
	scan_info->start_tsf = start_tsf;

	channel_dwelltime = CHANNEL_ACTIVE_DWELLTIME(scan_info)/scan_info->npasses;

	WL_SCAN(("wl%d: active dwell time %d ms, chanspec %s, tsf %u\n",
		scan_info->unit, channel_dwelltime,
		wf_chspec_ntoa_ex(scan_info->cur_scan_chanspec, chanbuf),
		start_tsf));

	wlc_scan_act(scan_info, channel_dwelltime);

	/* record phy noise for the scan channel */
	if (D11REV_LT(IS_COREREV(scan_info), 40))
		wlc_lq_noise_sample_request(scan_info->wlc, WLC_NOISE_REQUEST_SCAN,
			CHSPEC_CHANNEL(chanspec));
}

bool
wlc_scan_ssid_match(wlc_scan_info_t *scan_pub, bcm_tlv_t *ssid_ie, bool filter)
{
	scan_info_t 	*scan_info = (scan_info_t *)scan_pub->scan_priv;
	wlc_ssid_t 	*ssid;
	int 		i;
	char		*c;

	if ((scan_pub->wlc_scan_cmn->usage == SCAN_ENGINE_USAGE_RM) ||
	    ((scan_info->nssid == 1) && ((scan_info->ssid_list[0]).SSID_len == 0))) {
		return TRUE;
	}

	if (scan_info->ssid_wildcard_enabled)
		return TRUE;

	if (!ssid_ie || ssid_ie->len > DOT11_MAX_SSID_LEN) {
		return FALSE;
	}

	/* filter out beacons which have all spaces or nulls as ssid */
	if (filter) {
		if (ssid_ie->len == 0)
			return FALSE;
		c = (char *)&ssid_ie->data[0];
		for (i = 0; i < ssid_ie->len; i++) {
			if ((*c != 0) && (*c != ' '))
				break;
			c++;
		}
		if (i == ssid_ie->len)
			return FALSE;
	}

	/* do not do ssid matching if we are sending out bcast SSIDs
	 * do the filtering before the scan_complete callback
	 */

	ssid = scan_info->ssid_list;
	for (i = 0; i < scan_info->nssid; i++) {
		if (SCAN_IS_MATCH_SSID(scan_info, ssid->SSID, ssid_ie->data,
		                      ssid->SSID_len, ssid_ie->len))
			return TRUE;
#ifdef WLP2P
		if (IS_P2P_ENAB(scan_info) &&
		    wlc_p2p_ssid_match(scan_info->wlc->p2p, scan_info->bsscfg,
		                       ssid->SSID, ssid->SSID_len,
		                       ssid_ie->data, ssid_ie->len))
			return TRUE;
#endif
		ssid++;
	}
	return FALSE;
}

#ifdef WL11N
static void
wlc_ht_obss_scan_update(scan_info_t *scan_info, int status)
{
	wlc_info_t *wlc = scan_info->wlc;
	wlc_bsscfg_t *cfg;
	uint8 chanvec[OBSS_CHANVEC_SIZE]; /* bitvec of channels in 2G */
	uint8 chan, i;
	uint8 num_chan = 0;

	(void)wlc;

	WL_TRACE(("wl%d: wlc_ht_obss_scan_update\n", scan_info->unit));

	cfg = scan_info->bsscfg;

	/* checking  for valid fields */
	if (!wlc_obss_scan_fields_valid(wlc->obss, cfg)) {
		return;
	}

	if (!wlc_obss_is_scan_complete(wlc->obss, cfg, (status == WLC_E_STATUS_SUCCESS),
		scan_info->active_time, scan_info->passive_time)) {
		return;
	}

	bzero(chanvec, OBSS_CHANVEC_SIZE);
	for (i = 0; i < scan_info->channel_num; i++) {
		chan = CHSPEC_CHANNEL(scan_info->chanspec_list[i]);
		if (chan <= CH_MAX_2G_CHANNEL) {
			setbit(chanvec, chan);
			num_chan++;
		}
	}

	wlc_obss_scan_update_countdown(wlc->obss, cfg, chanvec, num_chan);

}
#endif /* WL11N */

#ifdef WLSCANCACHE

/* Add the current wlc_scan_info->scan_results to the scancache */
static void
wlc_scan_fill_cache(scan_info_t *scan_info, uint current_timestamp)
{
	uint index;
	wlc_bss_list_t *scan_results;
	wlc_bss_info_t *bi;
	wlc_ssid_t SSID;
	size_t datalen = 0;
	uint8* data = NULL;
	size_t bi_len;
	wlc_bss_info_t *new_bi;

	scan_results = SCAN_RESULT_PTR(scan_info);
	ASSERT(scan_results);

	wlc_scandb_ageout(scan_info->sdb, current_timestamp);

	/* walk the list of scan resutls, adding each to the cache */
	for (index = 0; index < scan_results->count; index++) {
		bi = scan_results->ptrs[index];
		if (bi == NULL) continue;

		bi_len = sizeof(wlc_bss_info_t);
		if (bi->bcn_prb)
			bi_len += bi->bcn_prb_len;

		/* allocate a new buffer if the current one is not big enough */
		if (data == NULL || bi_len > datalen) {
			if (data != NULL)
				MFREE(scan_info->osh, data, datalen);

			datalen = ROUNDUP(bi_len, 64);

			data = MALLOCZ(scan_info->osh, datalen);
			if (data == NULL) {
				WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(scan_info->wlc), __FUNCTION__,
					(int)datalen, MALLOCED(scan_info->osh)));
				continue;
			}
		}

		new_bi = (wlc_bss_info_t*)data;

		memcpy(new_bi, bi, sizeof(wlc_bss_info_t));
		if (bi->bcn_prb) {
			new_bi->bcn_prb = (struct dot11_bcn_prb*)(data + sizeof(wlc_bss_info_t));
			memcpy(new_bi->bcn_prb, bi->bcn_prb, bi->bcn_prb_len);
		}
		new_bi->flags |= WLC_BSS_CACHE;

		SSID.SSID_len = bi->SSID_len;
		memcpy(SSID.SSID, bi->SSID, DOT11_MAX_SSID_LEN);

		wlc_scandb_add(scan_info->sdb,
		               &bi->BSSID, &SSID, bi->bss_type, bi->chanspec, current_timestamp,
		               new_bi, bi_len);
	}

	if (data != NULL)
		MFREE(scan_info->osh, data, datalen);
}

/* Return the contents of the scancache in the 'bss_list' param.
 *
 * Return only those scan results that match the criteria specified by the other params:
 *
 * BSSID:	match the provided BSSID exactly unless BSSID is a NULL pointer or FF:FF:FF:FF:FF:FF
 * nssid:	nssid number of ssids in the array pointed to by SSID
 * SSID:	match [one of] the provided SSIDs exactly unless SSID is a NULL pointer,
 *		SSID[0].SSID_len == 0 (broadcast SSID), or nssid = 0 (no SSIDs to match)
 * BSS_type:	match the 802.11 infrastructure type. Should be one of the values:
 *		{DOT11_BSSTYPE_INFRASTRUCTURE, DOT11_BSSTYPE_INDEPENDENT, DOT11_BSSTYPE_ANY}
 * chanspec_list, chanspec_num: if chanspec_num == 0, no channel filtering is done. Otherwise
 *		the chanspec list should contain 20MHz chanspecs. Only BSSs with a matching channel,
 *		or for a 40MHz BSS, with a matching control channel, will be returned.
 */
void
wlc_scan_get_cache(wlc_scan_info_t *scan_pub,
                   const struct ether_addr *BSSID, int nssid, const wlc_ssid_t *SSID,
                   int BSS_type, const chanspec_t *chanspec_list, uint chanspec_num,
                   wlc_bss_list_t *bss_list)
{
	scan_iter_params_t params;
	scan_info_t *scan_info = (scan_info_t *)scan_pub->scan_priv;

	params.merge = FALSE;
	params.bss_list = bss_list;
	params.current_ts = 0;

	memset(bss_list, 0, sizeof(wlc_bss_list_t));

	/* ageout any old entries */
	wlc_scandb_ageout(scan_info->sdb, OSL_SYSUPTIME());

	wlc_scandb_iterate(scan_info->sdb,
	                   BSSID, nssid, SSID, BSS_type, chanspec_list, chanspec_num,
	                   wlc_scan_build_cache_list, scan_info, &params);
}

/* Merge the contents of the scancache with entries already in the 'bss_list' param.
 *
 * Return only those scan results that match the criteria specified by the other params:
 *
 * current_timestamp: timestamp matching the most recent additions to the cache. Entries with
 *		this timestamp will not be added to bss_list.
 * BSSID:	match the provided BSSID exactly unless BSSID is a NULL pointer or FF:FF:FF:FF:FF:FF
 * nssid:	nssid number of ssids in the array pointed to by SSID
 * SSID:	match [one of] the provided SSIDs exactly unless SSID is a NULL pointer,
 *		SSID[0].SSID_len == 0 (broadcast SSID), or nssid = 0 (no SSIDs to match)
 * BSS_type:	match the 802.11 infrastructure type. Should be one of the values:
 *		{DOT11_BSSTYPE_INFRASTRUCTURE, DOT11_BSSTYPE_INDEPENDENT, DOT11_BSSTYPE_ANY}
 * chanspec_list, chanspec_num: if chanspec_num == 0, no channel filtering is done. Otherwise
 *		the chanspec list should contain 20MHz chanspecs. Only BSSs with a matching channel,
 *		or for a 40MHz BSS, with a matching control channel, will be returned.
 */
static void
wlc_scan_merge_cache(scan_info_t *scan_info, uint current_timestamp,
                   const struct ether_addr *BSSID, int nssid, const wlc_ssid_t *SSID,
                   int BSS_type, const chanspec_t *chanspec_list, uint chanspec_num,
                   wlc_bss_list_t *bss_list)
{
	scan_iter_params_t params;

	params.merge = TRUE;
	params.bss_list = bss_list;
	params.current_ts = current_timestamp;

	wlc_scandb_iterate(scan_info->sdb,
	                   BSSID, nssid, SSID, BSS_type, chanspec_list, chanspec_num,
	                   wlc_scan_build_cache_list, scan_info, &params);
}

static void
wlc_scan_build_cache_list(void *arg1, void *arg2, uint timestamp,
                          struct ether_addr *BSSID, wlc_ssid_t *SSID,
                          int BSS_type, chanspec_t chanspec, void *data, uint datalen)
{
	scan_info_t *scan_info = (scan_info_t*)arg1;
	scan_iter_params_t *params = (scan_iter_params_t*)arg2;
	wlc_bss_list_t *bss_list = params->bss_list;
	wlc_bss_info_t *bi;
	wlc_bss_info_t *cache_bi;

	BCM_REFERENCE(chanspec);
	BCM_REFERENCE(BSSID);
	BCM_REFERENCE(SSID);
	BCM_REFERENCE(BSS_type);

	/* skip the most recent batch of results when merging the cache to a bss_list */
	if (params->merge &&
	    params->current_ts == timestamp)
		return;

	if (bss_list->count >= (uint)SCAN_MAXBSS(scan_info))
		return;

	bi = MALLOC(scan_info->osh, sizeof(wlc_bss_info_t));
	if (!bi) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		          scan_info->unit, __FUNCTION__, MALLOCED(scan_info->osh)));
		return;
	}

	ASSERT(data != NULL);
	ASSERT(datalen >= sizeof(wlc_bss_info_t));

	cache_bi = (wlc_bss_info_t*)data;

	memcpy(bi, cache_bi, sizeof(wlc_bss_info_t));
	if (cache_bi->bcn_prb_len) {
		ASSERT(datalen >= sizeof(wlc_bss_info_t) + bi->bcn_prb_len);
		if (!(bi->bcn_prb = MALLOC(scan_info->osh, bi->bcn_prb_len))) {
			WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
				scan_info->unit, __FUNCTION__, MALLOCED(scan_info->osh)));
			MFREE(scan_info->osh, bi, sizeof(wlc_bss_info_t));
			return;
		}
		/* Source is a flattened out structure but its bcn_prb pointer is not fixed
		 * when the entry was added to scancache db. So find out the new location.
		 */
		cache_bi->bcn_prb = (struct dot11_bcn_prb*)((uchar *) data +
		                                            sizeof(wlc_bss_info_t));

		memcpy(bi->bcn_prb, cache_bi->bcn_prb, bi->bcn_prb_len);
	}

	bss_list->ptrs[bss_list->count++] = bi;

}

static void
wlc_scan_cache_result(scan_info_t *scan_info)
{
	wlc_scan_info_t	*wlc_scan_info = scan_info->scan_pub;
	uint timestamp = OSL_SYSUPTIME();

	/* if we have scan caching enabled, enter these results in the cache */
	wlc_scan_fill_cache(scan_info, timestamp);

	/* filter to just the desired SSID if we did a bcast scan for suppress */

	/* Provide the latest results plus cached results if they were requested. */
	if (wlc_scan_info->state & SCAN_STATE_INCLUDE_CACHE) {
		/* Merge cached results with current results */
		wlc_scan_merge_cache(scan_info, timestamp,
		                     &wlc_scan_info->bssid,
		                     scan_info->nssid, &scan_info->ssid_list[0],
		                     wlc_scan_info->wlc_scan_cmn->bss_type,
		                     scan_info->chanspec_list, scan_info->channel_num,
		                     SCAN_RESULT_PTR(scan_info));

		WL_SCAN(("wl%d: %s: Merged scan results with cache, new total %d\n",
		         scan_info->unit, __FUNCTION__, SCAN_RESULT_MEB(scan_info, count)));
	}
}
#endif /* WLSCANCACHE */

static void
wlc_scan_watchdog(void *hdl)
{
#ifdef WLSCANCACHE
	scan_info_t *scan = (scan_info_t *)hdl;

	/* ageout any old entries to free up memory */
	if (SCANCACHE_ENAB(scan->scan_pub))
		wlc_scandb_ageout(scan->sdb, OSL_SYSUPTIME());
#endif
}

wlc_bsscfg_t *
wlc_scan_bsscfg(wlc_scan_info_t *wlc_scan_info)
{
	scan_info_t *scan_info = (scan_info_t *)wlc_scan_info->scan_priv;
	return scan_info->bsscfg;
}

#ifdef WLSCAN_PS
/* This function configures tx & rxcores to save power.
 *  flag: TRUE to set & FALSE to revert config
 */
static int
wlc_scan_ps_config_cores(scan_info_t *scan_info, bool flag)
{

	int idx;
	wlc_bsscfg_t *cfg;

	/* bail out if both scan tx & rx pwr opt. are disabled */
	if (!scan_info->scan_tx_pwrsave &&
	    !scan_info->scan_rx_pwrsave) {
		WL_SCAN(("wl%d: %s(%d): tx_ps %d rx_ps\n",
		                scan_info->unit, func, line,
		                scan_info->scan_tx_pwrsave,
		                scan_info->scan_rx_pwrsave));
		return BCME_OK;
	}

	/* enable cores only when device is in PM = 1 or 2 mode */
	SCAN_FOREACH_AS_STA(scan_info, idx, cfg) {
		if (cfg->BSS && cfg->pm->PM == PM_OFF) {
			WL_SCAN(("wl%d: %s(%d): pm %d\n",
				scan_info->unit, func, line, cfg->pm->PM));

			/* If PM becomes 0 after scan initiated,
			  * we need to reset the cores
			  */
			if (!scan_info->scan_ps_rxchain &&
			    !scan_info->scan_ps_txchain) {
				WL_SCAN(("wl%d: %s(%d): txchain %d rxchain %d\n",
				    scan_info->unit, func, line, scan_info->scan_ps_txchain,
				    scan_info->scan_ps_rxchain));
				return BCME_ERROR;
			}
		}
	}

	wlc_suspend_mac_and_wait(scan_info->wlc);

	if (flag) {
		/* Scanning is started */
		if ((
			scan_info->scan_pwrsave_enable) && !scan_info->scan_ps_txchain) {
		/* if txchains doesn't match with hw defaults, don't modify chain mask
		  * and also ignore for 1x1. scan pwrsave iovar should be enabled otherwise ignore.
		  */
			if (wlc_stf_txchain_ishwdef(scan_info->wlc) &&
				scan_info->wlc->stf->hw_txchain >= 0x03) {
				wlc_suspend_mac_and_wait(scan_info->wlc);
				/* back up chain configuration */
				scan_info->scan_ps_txchain = scan_info->wlc->stf->txchain;
				wlc_stf_txchain_set(scan_info->wlc, 0x1, FALSE, WLC_TXCHAIN_ID_USR);
				wlc_enable_mac(scan_info->wlc);
			}
		}
		if ((
			scan_info->scan_pwrsave_enable) && !scan_info->scan_ps_rxchain) {
		/* if rxchain doesn't match with hw defaults, don't modify chain mask
		  * and also ignore for 1x1. scan pwrsave iovar should be enabled otherwise ignore.
		  */
			if (wlc_stf_rxchain_ishwdef(scan_info->wlc) &&
				scan_info->wlc->stf->hw_rxchain >= 0x03) {
				wlc_suspend_mac_and_wait(scan_info->wlc);
				/* back up chain configuration */
				scan_info->scan_ps_rxchain = scan_info->wlc->stf->rxchain;
				wlc_stf_rxchain_set(scan_info->wlc, 0x1, TRUE);
				wlc_enable_mac(scan_info->wlc);
			}
		}
	} else {
		/* Scanning is ended */
		if (!scan_info->scan_ps_txchain && !scan_info->scan_ps_rxchain) {
			return BCME_OK;
		} else {
			/* when scan_ps_txchain is 0, it mean scan module has not modified chains */
			if (scan_info->scan_ps_txchain) {
				wlc_suspend_mac_and_wait(scan_info->wlc);
				wlc_stf_txchain_set(scan_info->wlc, scan_info->scan_ps_txchain,
					FALSE, WLC_TXCHAIN_ID_USR);
				scan_info->scan_ps_txchain = 0;
				wlc_enable_mac(scan_info->wlc);
			}
			if (scan_info->scan_ps_rxchain) {
				wlc_suspend_mac_and_wait(scan_info->wlc);
				wlc_stf_rxchain_set(scan_info->wlc,
				                    scan_info->scan_ps_rxchain, TRUE);
				/* make the chain value to 0 */
				scan_info->scan_ps_rxchain = 0;
				wlc_enable_mac(scan_info->wlc);
			}
		}
	}

	return BCME_OK;
}
#endif /* WLSCAN_PS */


/* scanmac enable */
static int
scanmac_enable(scan_info_t *scan_info, int enable)
{
	int err = BCME_OK;
	wlc_info_t *wlc = scan_info->wlc;
	wlc_bsscfg_t *bsscfg = NULL;
	bool state = scan_info->scanmac_bsscfg != NULL;

	if (enable) {
		int idx;
		uint32 flags = WLC_BSSCFG_NOIF | WLC_BSSCFG_NOBCMC;
		wlc_bsscfg_type_t type = {BSSCFG_TYPE_GENERIC, BSSCFG_GENERIC_STA};

		/* scanmac already enabled */
		if (state) {
			WL_SCAN(("wl%d: scanmac already enabled\n", wlc->pub->unit));
			return BCME_OK;
		}

		/* bsscfg with the MAC address exists */
		if (wlc_bsscfg_find_by_hwaddr(wlc, &scan_info->scanmac_config.mac) != NULL) {
			WL_ERROR(("wl%d: MAC address is in use\n", wlc->pub->unit));
			return BCME_BUSY;
		}

		/* allocate bsscfg */
		if ((idx = wlc_bsscfg_get_free_idx(wlc)) == -1) {
			WL_ERROR(("wl%d: no free bsscfg\n", wlc->pub->unit));
			return BCME_NORESOURCE;
		}
		else if ((bsscfg = wlc_bsscfg_alloc(wlc, idx, &type, flags,
		                &scan_info->scanmac_config.mac)) == NULL) {
			WL_ERROR(("wl%d: cannot create bsscfg\n", wlc->pub->unit));
			return BCME_NOMEM;
		}
		else if (wlc_bsscfg_init(wlc, bsscfg) != BCME_OK) {
			WL_ERROR(("wl%d: cannot init bsscfg\n", wlc->pub->unit));
			err = BCME_ERROR;
			goto free;
		}
		memcpy(&bsscfg->BSSID, &bsscfg->cur_etheraddr, ETHER_ADDR_LEN);
		bsscfg->current_bss->bss_type = DOT11_BSSTYPE_INDEPENDENT;
		scan_info->scanmac_bsscfg = bsscfg;
	}
	else {
		wlc_scan_info_t *wlc_scan_info = wlc->scan;

		/* scanmac not enabled */
		if (!state) {
			WL_SCAN(("wl%d: scanmac is already disabled\n", wlc->pub->unit));
			return BCME_OK;
		}

		bsscfg = scan_info->scanmac_bsscfg;
		ASSERT(bsscfg != NULL);

		wlc_scan_abort(wlc_scan_info, WLC_E_STATUS_ABORT);

	free:
		/* free bsscfg + error handling */
		if (bsscfg != NULL)
			wlc_bsscfg_free(wlc, bsscfg);
		scan_info->scanmac_bsscfg = NULL;
		memset(&scan_info->scanmac_config, 0, sizeof(scan_info->scanmac_config));
	}

	return err;
}

/* get random mac */
static void
scanmac_random_mac(scan_info_t *scan_info, struct ether_addr *mac)
{
	wlc_info_t *wlc = scan_info->wlc;

	/* HW random generator only available if core is up */
	if (wlc_getrand(wlc, (uint8 *)mac, ETHER_ADDR_LEN) != BCME_OK) {
		/* random MAC based on OSL_RAND() */
		uint32 rand1 = OSL_RAND();
		uint32 rand2 = OSL_RAND();
		memcpy(&mac->octet[0], &rand1, 2);
		memcpy(&mac->octet[2], &rand2, sizeof(rand2));
	}
}

/* update configured or randomized MAC */
static void
scanmac_update_mac(scan_info_t *scan_info)
{
	wlc_info_t *wlc = scan_info->wlc;
	wlc_bsscfg_t *bsscfg = scan_info->scanmac_bsscfg;
	wl_scanmac_config_t *scanmac_config = &scan_info->scanmac_config;
	struct ether_addr *mac = &scanmac_config->mac;
	struct ether_addr *mask = &scanmac_config->random_mask;
	struct ether_addr random_mac;

	/* generate random MAC if random mask is non-zero */
	if (!ETHER_ISNULLADDR(mask)) {
		struct ether_addr *prefix = &scanmac_config->mac;
		int i;
start_random_mac:
		scanmac_random_mac(scan_info, &random_mac);
		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			/* AND the random mask bits */
			random_mac.octet[i] &= mask->octet[i];
			/* OR the MAC prefix */
			random_mac.octet[i] |= prefix->octet[i] & ~mask->octet[i];
		}

		/* randomize again if MAC is not unique accoss bsscfgs */
		if (wlc_bsscfg_find_by_hwaddr(wlc, &random_mac) != NULL) {
			WL_INFORM(("wl%d: scanmac_update_mac: regenerate random MAC\n",
				scan_info->unit));
			goto start_random_mac;
		}

		mac = &random_mac;
		scan_info->is_scanmac_config_updated = TRUE;
	}

	if (scan_info->is_scanmac_config_updated) {
		/* activate new MAC */
		memcpy(&bsscfg->BSSID, mac, ETHER_ADDR_LEN);
		wlc_validate_mac(wlc, bsscfg, mac);
		scan_info->is_scanmac_config_updated = FALSE;
	}
}

/* scanmac config */
static int
scanmac_config(scan_info_t *scan_info, wl_scanmac_config_t *config)
{
	wlc_info_t *wlc = scan_info->wlc;
	wlc_bsscfg_t *bsscfg = scan_info->scanmac_bsscfg;

	if (scan_info->scanmac_bsscfg == NULL) {
		/* scanmac not enabled */
		return BCME_NOTREADY;
	}

	/* don't allow multicast MAC or random mask */
	if (ETHER_ISMULTI(&config->mac) || ETHER_ISMULTI(&config->random_mask)) {
		return BCME_BADADDR;
	}

	/* check if MAC exists */
	if (ETHER_ISNULLADDR(&config->random_mask)) {
		wlc_bsscfg_t *match = wlc_bsscfg_find_by_hwaddr(wlc, &config->mac);
		if (match != NULL && match != bsscfg) {
			WL_ERROR(("wl%d: MAC address is in use\n", wlc->pub->unit));
			return BCME_BUSY;
		}
	}

	/* save config */
	memcpy(&scan_info->scanmac_config, config, sizeof(*config));
	scan_info->is_scanmac_config_updated = TRUE;

	/* update MAC if scan not in progress */
	if (!SCAN_IN_PROGRESS(scan_info->scan_pub)) {
		scanmac_update_mac(scan_info);
	}

	return BCME_OK;
}

/* scanmac GET iovar */
static int
scanmac_get(scan_info_t *scan_info, void *params, uint p_len, void *arg, int len)
{
	int err = BCME_OK;
	wl_scanmac_t *sm = params;
	wl_scanmac_t *sm_out = arg;

	BCM_REFERENCE(len);

	/* verify length */
	if (p_len < OFFSETOF(wl_scanmac_t, data) ||
		sm->len > p_len - OFFSETOF(wl_scanmac_t, data)) {
		WL_ERROR((WLC_SCAN_LENGTH_ERR, __FUNCTION__, p_len, sm->len));
		return BCME_BUFTOOSHORT;
	}

	/* copy subcommand to output */
	sm_out->subcmd_id = sm->subcmd_id;

	/* process subcommand */
	switch (sm->subcmd_id) {
	case WL_SCANMAC_SUBCMD_ENABLE:
	{
		wl_scanmac_enable_t *sm_enable = (wl_scanmac_enable_t *)sm_out->data;
		sm_out->len = sizeof(*sm_enable);
		sm_enable->enable = scan_info->scanmac_bsscfg ? 1 : 0;
		break;
	}
	case WL_SCANMAC_SUBCMD_BSSCFG:
	{
		wl_scanmac_bsscfg_t *sm_bsscfg = (wl_scanmac_bsscfg_t *)sm_out->data;
		if (scan_info->scanmac_bsscfg == NULL) {
			return BCME_ERROR;
		} else {
			sm_out->len = sizeof(*sm_bsscfg);
			sm_bsscfg->bsscfg = WLC_BSSCFG_IDX(scan_info->scanmac_bsscfg);
		}
		break;
	}
	case WL_SCANMAC_SUBCMD_CONFIG:
	{
		wl_scanmac_config_t *sm_config = (wl_scanmac_config_t *)sm_out->data;
		sm_out->len = sizeof(*sm_config);
		memcpy(sm_config, &scan_info->scanmac_config, sizeof(*sm_config));
		break;
	}
	default:
		ASSERT(0);
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
}

/* scanmac SET iovar */
static int
scanmac_set(scan_info_t *scan_info, void *arg, int len)
{
	int err = BCME_OK;
	wl_scanmac_t *sm = arg;

	/* verify length */
	if (len < (int)OFFSETOF(wl_scanmac_t, data) ||
		sm->len > len - OFFSETOF(wl_scanmac_t, data)) {
		WL_ERROR((WLC_SCAN_LENGTH_ERR, __FUNCTION__, len, sm->len));
		return BCME_BUFTOOSHORT;
	}

	/* process subcommand */
	switch (sm->subcmd_id) {
	case WL_SCANMAC_SUBCMD_ENABLE:
	{
		wl_scanmac_enable_t *sm_enable = (wl_scanmac_enable_t *)sm->data;
		if (sm->len >= sizeof(*sm_enable)) {
			err = scanmac_enable(scan_info, sm_enable->enable);
		} else  {
			err = BCME_BADLEN;
		}
		break;
	}
	case WL_SCANMAC_SUBCMD_BSSCFG:
		err = BCME_BADARG;
		break;
	case WL_SCANMAC_SUBCMD_CONFIG:
	{
		wl_scanmac_config_t *sm_config = (wl_scanmac_config_t *)sm->data;
		if (sm->len >= sizeof(*sm_config)) {
			err = scanmac_config(scan_info, sm_config);
		} else  {
			err = BCME_BADLEN;
		}
		break;
	}
	default:
		ASSERT(0);
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
}

/* return scanmac bsscfg if macreq enabled, else return NULL */
wlc_bsscfg_t *
wlc_scanmac_get_bsscfg(wlc_scan_info_t *scan, int macreq, wlc_bsscfg_t *cfg)
{
	scan_info_t *scan_info = scan->scan_priv;
	wlc_bsscfg_t *bsscfg = SCAN_USER(scan_info, cfg);

	/* check if scanmac enabled */
	if (scan_info->scanmac_bsscfg != NULL) {
		uint16 sbmap = scan_info->scanmac_config.scan_bitmap;
		bool is_associated = IS_BSS_ASSOCIATED(bsscfg);
		bool is_host_scan = (macreq == WLC_ACTION_SCAN || macreq == WLC_ACTION_ISCAN ||
			macreq == WLC_ACTION_ESCAN);

		/* return scanmac bsscfg if bitmap for macreq is enabled */
		if ((!is_associated && (is_host_scan || macreq == WLC_ACTION_PNOSCAN) &&
			(sbmap & WL_SCANMAC_SCAN_UNASSOC)) ||
			(is_associated &&
			(((macreq == WLC_ACTION_ROAM) && (sbmap & WL_SCANMAC_SCAN_ASSOC_ROAM)) ||
			((macreq == WLC_ACTION_PNOSCAN) && (sbmap & WL_SCANMAC_SCAN_ASSOC_PNO)) ||
			(is_host_scan && (sbmap & WL_SCANMAC_SCAN_ASSOC_HOST))))) {
			return scan_info->scanmac_bsscfg;
		}
	}
	return NULL;
}

/* return scanmac mac if macreq enabled, else return NULL */
struct ether_addr *
wlc_scanmac_get_mac(wlc_scan_info_t *scan, int macreq, wlc_bsscfg_t *bsscfg)
{
	wlc_bsscfg_t *scanmac_bsscfg = wlc_scanmac_get_bsscfg(scan, macreq, bsscfg);

	if (scanmac_bsscfg != NULL) {
		return &scanmac_bsscfg->cur_etheraddr;
	}
	return NULL;
}

/* invoked at scan or GAS complete to allow MAC to be updated */
int
wlc_scanmac_update(wlc_scan_info_t *scan)
{
	scan_info_t *scan_info = scan->scan_priv;

	if (scan_info->scanmac_bsscfg == NULL) {
		/* scanmac not enabled */
		return BCME_NOTREADY;
	}

	scanmac_update_mac(scan_info);

	return BCME_OK;
}

static void
wlc_scan_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	scan_info_t *scan_info = (scan_info_t *)ctx;
	wlc_info_t *wlc = scan_info->wlc;
	int idx;
	wlc_info_t *my_wlc;
	int scan_cfg = FALSE;

	FOREACH_WLC(wlc->cmn, idx, my_wlc) {
		if (SCAN_IN_PROGRESS(my_wlc->scan) && cfg == wlc_scan_bsscfg(my_wlc->scan)) {
			scan_cfg = TRUE;
			break;
		}
	}
	/* Make sure that any active scan is not associated to this cfg */
	if (scan_cfg) {
		/* ASSERT(bsscfg != wlc_scan_bsscfg(wlc->scan)); */
		WL_ERROR(("wl%d.%d: %s: scan still active using cfg %p\n", WLCWLUNIT(wlc),
		          WLC_BSSCFG_IDX(cfg), __FUNCTION__, cfg));
		wlc_scan_abort(wlc->scan, WLC_E_STATUS_ABORT);
	}
}


/* a wrapper to check & end scan excursion & resume primary txq operation */
static void
wlc_scan_excursion_end(scan_info_t *scan_info)
{
	/* enable CFP and TSF update */
	wlc_mhf(SCAN_WLC(scan_info), MHF2, MHF2_SKIP_CFP_UPDATE, 0, WLC_BAND_ALL);
	wlc_scan_skip_adjtsf(scan_info, FALSE, NULL, WLC_SKIP_ADJTSF_SCAN,
		WLC_BAND_ALL);

	/* Restore promisc behavior for beacons and probes */
	SCAN_BCN_PROMISC(scan_info) = FALSE;
	wlc_scan_mac_bcn_promisc(scan_info);

	/* check & resume normal tx queue operation */
	if (scan_info->wlc->excursion_active) {
		wlc_suspend_mac_and_wait(scan_info->wlc);
		wlc_excursion_end(scan_info->wlc);
		wlc_enable_mac(scan_info->wlc);
	}
}
