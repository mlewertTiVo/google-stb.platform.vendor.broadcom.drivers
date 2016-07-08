/*
 * BSS Configuration routines for
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
 * $Id: wlc_bsscfg.c 612897 2016-01-15 11:22:32Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_mbss.h>
#include <wl_export.h>
#include <wlc_ap.h>
#include <bcm_notif_pub.h>
#ifdef WLRSDB
#include <wlc_rsdb.h>
#endif /* WLRSDB */
#ifdef BCMULP
#include <ulp.h>
#endif /* BCMULP */
#include <wlc_cubby.h>
#include <wlc_event_utils.h>
#include <wlc_dump.h>

/** structure for storing global bsscfg module state */
struct bsscfg_module {
	wlc_info_t	*wlc;		/**< pointer to wlc */
	uint		cfgtotsize;	/**< total bsscfg size including container */
	uint8		ext_cap_max;	/**< extend capability max length */
	wlc_cubby_info_t *cubby_info;	/**< cubby client info */
	bcm_notif_h	state_upd_notif_hdl;	/**< state change notifier handle. */
	bcm_notif_h tplt_upd_notif_hdl; /**< s/w bcn/prbrsp update notifier handle. */
	bcm_notif_h mute_upd_notif_hdl; /**< ibss mute update notifier handle. */
	bcm_notif_h pretbtt_query_hdl;	/**< pretbtt query handle. */
	bcm_notif_h as_st_notif_hdl;	/**< assoc state notifier handle. */
	bcm_notif_h  rxbcn_nh; /* beacon notifier handle. */
	uint16	next_bsscfg_ID;

	/* TO BE REPLACED, LEAVE IT AT THE END */
	bcm_notif_h up_down_notif_hdl;	/**< up/down notifier handle. */
};

/** Flags that should not be cleared on AP bsscfg up */
#define WLC_BSSCFG_PERSIST_FLAGS (0 | \
		WLC_BSSCFG_WME_DISABLE | \
		WLC_BSSCFG_PRESERVE | \
		WLC_BSSCFG_NOBCMC | \
		WLC_BSSCFG_NOIF | \
		WLC_BSSCFG_11N_DISABLE | \
		WLC_BSSCFG_11H_DISABLE | \
		WLC_BSSCFG_NATIVEIF | \
		WLC_BSSCFG_SRADAR_ENAB | \
		WLC_BSSCFG_DYNBCN | \
		WLC_BSSCFG_AP_NORADAR_CHAN | \
		WLC_BSSCFG_BSSLOAD_DISABLE | \
		WLC_BSSCFG_TX_SUPR_ENAB | \
		WLC_BSSCFG_NO_AUTHENTICATOR | \
		WLC_BSSCFG_PSINFO | \
	0)
/* Clear non-persistant flags; by default, HW beaconing and probe resp */
#define WLC_BSSCFG_FLAGS_INIT(cfg) do { \
		(cfg)->flags &= WLC_BSSCFG_PERSIST_FLAGS; \
		(cfg)->flags |= (WLC_BSSCFG_HW_BCN | WLC_BSSCFG_HW_PRB); \
	} while (0)

/* Flags2 that should not be cleared on AP bsscfg up */
#define WLC_BSSCFG_PERSIST_FLAGS2 (0 | \
		WLC_BSSCFG_FL2_MFP_CAPABLE | \
		WLC_BSSCFG_FL2_MFP_REQUIRED | \
		WLC_BSSCFG_FL2_FBT_1X | \
		WLC_BSSCFG_FL2_FBT_PSK | \
	0)
/* Clear non-persistant flags2 */
#define WLC_BSSCFG_FLAGS2_INIT(cfg) do { \
		(cfg)->flags2 &= WLC_BSSCFG_PERSIST_FLAGS2; \
	} while (0)


/* debugging macros */
#define WL_BCB(x) WL_APSTA_UPDN(x)

/* Local Functions */

static int wlc_bsscfg_wlc_up(void *ctx);


static int wlc_bsscfg_init_int(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	wlc_bsscfg_type_t *type, uint flags, struct ether_addr *ea);
static int wlc_bsscfg_init_intf(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

static void wlc_bsscfg_deinit_intf(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

static int wlc_bsscfg_bcmcscballoc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
static void wlc_bsscfg_bcmcscbfree(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

#ifdef AP
static int wlc_bsscfg_ap_init(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
static void wlc_bsscfg_ap_deinit(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
#endif
#ifdef STA
static int wlc_bsscfg_sta_init(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
static void wlc_bsscfg_sta_deinit(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
#endif

static int wlc_bsscfg_alloc_int(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, int idx,
	wlc_bsscfg_type_t *type, uint flags, struct ether_addr *ea);

static bool wlc_bsscfg_preserve(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

static wlc_bsscfg_t *wlc_bsscfg_malloc(wlc_info_t *wlc);
static void wlc_bsscfg_mfree(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
static bool wlc_bsscfg_is_special(wlc_bsscfg_t *cfg);

static void wlc_bsscfg_type_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bsscfg_type_t *type);
static void wlc_bsscfg_info_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bsscfg_info_t *info);

/* module */

#ifdef BCMULP
static uint wlc_bsscfg_ulp_get_retention_size_cb(void *handle, ulp_ext_info_t *einfo);
static int wlc_bsscfg_ulp_enter_cb(void *handle, ulp_ext_info_t *einfo, uint8 *cache_data);

static int wlc_bsscfg_ulp_recreate_cb(void *handle, ulp_ext_info_t *einfo,
	uint8 *cache_data, uint8 *p2_cache_data);

typedef struct wlc_bsscfg_ulp_cache {
	uint32 cfg_cnt;	/* Count of cfgs present */
	/* after the above params, variable sized params
	* (eg: array of wlc_bsscfg_ulp_cache_var_t) will be added here.
	*/
} wlc_bsscfg_ulp_cache_t;

typedef struct wlc_bsscfg_ulp_cache_var {
	struct ether_addr cur_ether;	/* h/w address */
	struct ether_addr BSSID;	/* network BSSID */
	int bsscfg_idx;			/* idx of the cfg */
	int bsscfg_type;		/* bsscfg type */
	int bsscfg_flag;
	int bsscfg_flag2;
	bool _ap;			/* is this configuration an AP */
	uint32 wsec;			/* wireless security bitvec */
	uint32 WPA_auth;		/* WPA: authenticated key management */
	uint16 flags;
	bool	dtim_programmed;
} wlc_bsscfg_ulp_cache_var_t;

static const ulp_p1_module_pubctx_t ulp_bsscfg_ctx = {
	MODCBFL_CTYPE_DYNAMIC,
	wlc_bsscfg_ulp_enter_cb,
	NULL,
	wlc_bsscfg_ulp_get_retention_size_cb,
	wlc_bsscfg_ulp_recreate_cb,
	NULL
};
#endif /* BCMULP */

/**
 * BSS CFG specific IOVARS
 * Creating/Removing an interface is all about creating and removing a
 * BSSCFG. So its better if the interface create/remove IOVARS live here.
 */
enum {
	IOV_INTERFACE_CREATE = 1,	/* Creat an interface i.e bsscfg */
	IOV_INTERFACE_REMOVE = 2,	/* Remove an interface i.e bsscfg */
	IOV_BSSCFG_INFO = 3,		/* bsscfg info */
	IOV_LAST			/* In case of a need to check Max ID number */
};

/* BSSCFG IOVars */
static const bcm_iovar_t wlc_bsscfg_iovars[] = {
	{"interface_create", IOV_INTERFACE_CREATE,
	IOVF_OPEN_ALLOW, IOVT_BUFFER, sizeof(wl_interface_create_t)},
	{"interface_remove", IOV_INTERFACE_REMOVE, IOVF_OPEN_ALLOW, IOVT_VOID, 0},
	{"bsscfg_info", IOV_BSSCFG_INFO, 0, IOVT_BUFFER, sizeof(wlc_bsscfg_info_t)},
	{NULL, 0, 0, 0, 0}
};

static int
wlc_bsscfg_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, struct wlc_if *wlcif)
{
	bsscfg_module_t *bcmh = hdl;
	wlc_info_t *wlc = bcmh->wlc;
	wlc_bsscfg_t *bsscfg;
	int err = 0;
	int32 int_val = 0;
	BCM_REFERENCE(vi);
	BCM_REFERENCE(name);
	BCM_REFERENCE(val_size);

	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* update wlcif pointer */
	if (wlcif == NULL)
		wlcif = bsscfg->wlcif;
	ASSERT(wlcif != NULL);

	/* Do the actual parameter implementation */
	switch (actionid) {

	case IOV_GVAL(IOV_INTERFACE_CREATE): {
		int idx;
		wlc_bsscfg_t *cfg;
		wl_interface_create_t if_buf;
		bool ap;
		wl_interface_info_t *wl_info;
		struct ether_addr *p_ether_addr;
#ifdef WLRSDB
		uint32 wlc_index = 0;
#endif /* WLRSDB */
		wlc_bsscfg_type_t type = {BSSCFG_TYPE_GENERIC, BSSCFG_SUBTYPE_NONE};

		if ((uint)len < sizeof(if_buf)) {
			WL_ERROR(("wl%d: input buffer too short\n", wlc->pub->unit));
			err = BCME_BUFTOOSHORT;
			break;
		}

		bcopy((char *)params, (char*)&if_buf, sizeof(if_buf));

		/* Sanity check on version */
		if (if_buf.ver != WL_INTERFACE_CREATE_VER) {
			err = BCME_VERSION;
			break;
		}

		/*
		 * Note that if -w argument is used we would get the
		 * corresponding wlc, this IOVAR assumes that the correct
		 * wlc is supplied. But if the wlc_index is passed with -c option
		 * then it will override the value with -w option.
		 */
#ifdef WLRSDB
		/* In case if RSDB is enabled, use the wlc index from if_buf */
		if (RSDB_ENAB(wlc->pub) &&
				(if_buf.flags & WL_INTERFACE_WLC_INDEX_USE)) {
			/* Create virtual interface on other WLC */
			wlc_index = if_buf.wlc_index;

			if (wlc_index >= MAX_RSDB_MAC_NUM) {
				WL_ERROR(("wl%d: Invalid WLC Index %d \n", wlc->pub->unit,
					wlc_index));
				err = BCME_BADARG;
				break;
			}

			wlc = wlc->cmn->wlc[wlc_index];
		} /* end of RSDB_ENAB check */
#endif /* WLRSDB */

		/* allocate bsscfg */
		if ((idx = wlc_bsscfg_get_free_idx(wlc)) == BCME_ERROR) {
			WL_ERROR(("wl%d: no free bsscfg\n", wlc->pub->unit));
			return BCME_NORESOURCE;
		}

		if (if_buf.ver < 2) {
			ap = (if_buf.flags & WL_INTERFACE_CREATE_AP) != 0;
		} else {
			ap = (if_buf.iftype == WL_INTERFACE_TYPE_AP) ? TRUE : FALSE;
		}

		if (if_buf.flags & WL_INTERFACE_MAC_USE)
			p_ether_addr = &if_buf.mac_addr;
		else
			p_ether_addr = NULL;

		type.subtype = ap ? BSSCFG_GENERIC_AP : BSSCFG_GENERIC_STA;
		cfg = wlc_bsscfg_alloc(wlc, idx, &type, 0, p_ether_addr);
		if (!cfg) {
			WL_ERROR(("wl%d: can not allocate bsscfg\n", wlc->pub->unit));
			break;
		}

		if (wlc_bsscfg_init(wlc, cfg) != BCME_OK) {
			WL_ERROR(("wl%d: can not init bsscfg\n", wlc->pub->unit));
			wlc_bsscfg_free(wlc, cfg);
			break;
		}

		/*
		 * Note that the wlc_bsscfg_init sends WLC_E_IF_ADD event with
		 * the information about this interface, but all apps are not
		 * capable of event handling, so returning this info in the
		 * IOVAR path too.
		 */
		if (arg) {
			wl_info = (wl_interface_info_t *)arg;
			wl_info->bsscfgidx = (uint8)idx;
			strncpy(wl_info->ifname, wl_ifname(cfg->wlc->wl, cfg->wlcif->wlif),
				sizeof(wl_info->ifname) - 1);
		        wl_info->ifname[sizeof(wl_info->ifname) - 1] = 0;
			bcopy(&cfg->cur_etheraddr.octet, &wl_info->mac_addr.octet, ETHER_ADDR_LEN);
		}
		WL_INFORM(("wl%d: Interface Create success\n", wlc->pub->unit));
		break;
	}

	case IOV_SVAL(IOV_INTERFACE_REMOVE):
		if (WLC_BSSCFG_IDX(bsscfg) != 0) {
			if (bsscfg->enable) {
				wlc_bsscfg_disable(wlc, bsscfg);
			}
			wlc_bsscfg_free(wlc, bsscfg);
		} else {
			WL_ERROR(("wl%d: if_del failed: not delete primary bsscfg\n",
				wlc->pub->unit));
		}
		break;

	case IOV_GVAL(IOV_BSSCFG_INFO):
		wlc_bsscfg_info_get(wlc, bsscfg, (wlc_bsscfg_info_t *)arg);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

bsscfg_module_t *
BCMATTACHFN(wlc_bsscfg_attach)(wlc_info_t *wlc)
{
	bsscfg_module_t *bcmh;

	if ((bcmh = MALLOCZ(wlc->osh, sizeof(bsscfg_module_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	bcmh->wlc = wlc;

	if ((bcmh->cubby_info = wlc_cubby_attach(wlc, OBJR_BSSCFG_CUBBY,
			wlc->pub->tunables->maxbsscfgcubbies)) == NULL) {
		WL_ERROR(("wl%d: %s: wlc_cubby_attach failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* Create notification list for bsscfg state change event. */
	if (bcm_notif_create_list(wlc->notif, &bcmh->state_upd_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list failed (updn)\n",
		         wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* Create notification list for bsscfg up/down events. */
	if (bcm_notif_create_list(wlc->notif, &bcmh->up_down_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list failed (updn)\n",
		         wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* Create notification list for s/w bcn/prbrsp update events. */
	if (bcm_notif_create_list(wlc->notif, &bcmh->tplt_upd_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list failed (swbcn)\n",
		         wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* Create notification list for ibss mute update event. */
	if (bcm_notif_create_list(wlc->notif, &bcmh->mute_upd_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list failed (mute)\n",
		         wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* Create callback list for pretbtt query. */
	if (bcm_notif_create_list(wlc->notif, &bcmh->pretbtt_query_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list failed (pretbtt)\n",
		         wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* Create notification list for assoc state */
	if (bcm_notif_create_list(wlc->notif, &bcmh->as_st_notif_hdl) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list failed (asst)\n",
		         wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* create notification list for beacons. */
	if ((bcm_notif_create_list(wlc->notif, &bcmh->rxbcn_nh)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: bcm_notif_create_list() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	bcmh->cfgtotsize = ROUNDUP(sizeof(wlc_bsscfg_t), PTRSZ);
	bcmh->cfgtotsize += ROUNDUP(DOT11_EXTCAP_LEN_MAX, PTRSZ);
	bcmh->ext_cap_max = DOT11_EXTCAP_LEN_MAX;


	if (wlc_module_register(wlc->pub, wlc_bsscfg_iovars, "bsscfg", bcmh,
		wlc_bsscfg_doiovar, NULL, wlc_bsscfg_wlc_up,
		NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* ULP related registration */
#ifdef BCMULP
	if (ulp_p1_module_register(ULP_MODULE_ID_WLC_BSSCFG,
		&ulp_bsscfg_ctx, (void*)bcmh) != BCME_OK) {
		WL_ERROR(("%s: ulp_p1_module_register failed\n", __FUNCTION__));
		goto fail;
	}
#endif /* BCMULP */
	return bcmh;

fail:
	wlc_bsscfg_detach(bcmh);
	return NULL;
}

void
BCMATTACHFN(wlc_bsscfg_detach)(bsscfg_module_t *bcmh)
{
	wlc_info_t *wlc;

	if (bcmh == NULL)
		return;

	wlc = bcmh->wlc;

	wlc_module_unregister(wlc->pub, "bsscfg", bcmh);

	/* Delete event notification list. */
	if (bcmh->as_st_notif_hdl != NULL)
		bcm_notif_delete_list(&bcmh->as_st_notif_hdl);
	if (bcmh->pretbtt_query_hdl != NULL)
		bcm_notif_delete_list(&bcmh->pretbtt_query_hdl);
	if (bcmh->mute_upd_notif_hdl != NULL)
		bcm_notif_delete_list(&bcmh->mute_upd_notif_hdl);
	if (bcmh->tplt_upd_notif_hdl != NULL)
		bcm_notif_delete_list(&bcmh->tplt_upd_notif_hdl);
	if (bcmh->up_down_notif_hdl != NULL)
		bcm_notif_delete_list(&bcmh->up_down_notif_hdl);
	if (bcmh->state_upd_notif_hdl != NULL)
		bcm_notif_delete_list(&bcmh->state_upd_notif_hdl);
	if (bcmh->rxbcn_nh != NULL)
		bcm_notif_delete_list(&bcmh->rxbcn_nh);

	wlc_cubby_detach(wlc, bcmh->cubby_info);

	MFREE(wlc->osh, bcmh, sizeof(bsscfg_module_t));
}

static int
wlc_bsscfg_wlc_up(void *ctx)
{
#ifdef STA
	bsscfg_module_t *bcmh = (bsscfg_module_t *)ctx;
	wlc_info_t *wlc = bcmh->wlc;
	int idx;
	wlc_bsscfg_t *cfg;

#ifdef WLMCNX
	if (MCNX_ENAB(wlc->pub))
		return BCME_OK;
#endif

	/* Update tsf_cfprep if associated and up */
	FOREACH_AS_STA(wlc, idx, cfg) {
		if (cfg->up) {
			uint32 bi;

			/* get beacon period from bsscfg and convert to uS */
			bi = cfg->current_bss->beacon_period << 10;
			/* update the tsf_cfprep register */
			/* since init path would reset to default value */
			W_REG(wlc->osh, &wlc->regs->tsf_cfprep, (bi << CFPREP_CBI_SHIFT));

			/* Update maccontrol PM related bits */
			wlc_set_ps_ctrl(cfg);

			break;
		}
	}
#endif /* STA */

	return BCME_OK;
}

/**
 * Multiple modules have the need of reserving some private data storage related to a specific BSS
 * configuration. During ATTACH time, this function is called multiple times, typically one time per
 * module that requires this storage. This function does not allocate memory, but calculates values
 * to be used for a future memory allocation instead. The private data is located at the end of a
 * bsscfg.
 *
 * Returns the offset of the private data to the beginning of an allocated bsscfg structure,
 * negative values are errors.
 */
int
BCMATTACHFN(wlc_bsscfg_cubby_reserve_fn)(wlc_info_t *wlc, uint size,
	bsscfg_cubby_init_t fn_init, bsscfg_cubby_deinit_t fn_deinit,
	bsscfg_cubby_dump_t fn_dump, uint config_size,
	bsscfg_cubby_config_get_t fn_get, bsscfg_cubby_config_set_t fn_set,
	void *ctx)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	wlc_cubby_fn_t fn;
	int offset;
	wlc_cubby_cp_fn_t cp_fn;

	ASSERT((bcmh->cfgtotsize % PTRSZ) == 0);

	bzero(&fn, sizeof(fn));
	fn.fn_init = (cubby_init_fn_t)fn_init;
	fn.fn_deinit = (cubby_deinit_fn_t)fn_deinit;
	fn.fn_secsz = (cubby_secsz_fn_t)NULL;
	fn.fn_dump = (cubby_dump_fn_t)fn_dump;

	bzero(&cp_fn, sizeof(cp_fn));
	cp_fn.fn_get = (cubby_get_fn_t)fn_get;
	cp_fn.fn_set = (cubby_set_fn_t)fn_set;

	if ((offset = wlc_cubby_reserve(bcmh->cubby_info, size, &fn,
			config_size, &cp_fn, ctx)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_cubby_reserve failed with err %d\n",
		          wlc->pub->unit, __FUNCTION__, offset));
		return offset;
	}

	return bcmh->cfgtotsize + offset;
}


/**
 * wlc_bsscfg_state_upd_register()
 *
 * This function registers a callback that will be invoked when a bsscfg
 * changes its state (enable/disable/up/down).
 *
 * Parameters
 *    wlc       Common driver context.
 *    fn        Callback function  to invoke on state change event.
 *    arg       Client specified data that will provided as param to the callback.
 * Returns:
 *    BCME_OK on success, else BCME_xxx error code.
 */
int
BCMATTACHFN(wlc_bsscfg_state_upd_register)(wlc_info_t *wlc,
	bsscfg_state_upd_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->state_upd_notif_hdl;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

/**
 * wlc_bsscfg_state_upd_unregister()
 *
 * This function unregisters a bsscfg state change event callback.
 *
 * Parameters
 *    wlc       Common driver context.
 *    fn        Callback function that was previously registered.
 *    arg       Client specified data that was previously registerd.
 * Returns:
 *    BCME_OK on success, else BCME_xxx error code.
 */
int
BCMATTACHFN(wlc_bsscfg_state_upd_unregister)(wlc_info_t *wlc,
	bsscfg_state_upd_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->state_upd_notif_hdl;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

/**
 * wlc_bsscfg_updown_register()
 *
 * This function registers a callback that will be invoked when either a bsscfg
 * up or down event occurs.
 *
 * Parameters
 *    wlc       Common driver context.
 *    callback  Callback function  to invoke on up/down events.
 *    arg       Client specified data that will provided as param to the callback.
 * Returns:
 *    BCME_OK on success, else BCME_xxx error code.
 */
int
BCMATTACHFN(wlc_bsscfg_updown_register)(wlc_info_t *wlc, bsscfg_up_down_fn_t callback, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->up_down_notif_hdl;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)callback, arg);
}

/**
 * wlc_bsscfg_updown_unregister()
 *
 * This function unregisters a bsscfg up/down event callback.
 *
 * Parameters
 *    wlc       Common driver context.
 *    callback  Callback function that was previously registered.
 *    arg       Client specified data that was previously registerd.
 * Returns:
 *    BCME_OK on success, else BCME_xxx error code.
 */
int
BCMATTACHFN(wlc_bsscfg_updown_unregister)(wlc_info_t *wlc, bsscfg_up_down_fn_t callback, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->up_down_notif_hdl;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)callback, arg);
}

/* These functions register/unregister/invoke the callback
 * when bcn or prbresp template needs to be updated for the bsscfg that is of
 * WLC_BSSCFG_SW_BCN or WLC_BSSCFG_SW_PRB.
 */
int
BCMATTACHFN(wlc_bss_tplt_upd_register)(wlc_info_t *wlc, bss_tplt_upd_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->tplt_upd_notif_hdl;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

int
BCMATTACHFN(wlc_bss_tplt_upd_unregister)(wlc_info_t *wlc, bss_tplt_upd_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->tplt_upd_notif_hdl;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

void
wlc_bss_tplt_upd_notif(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int type)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	bss_tplt_upd_data_t notif_data;

	notif_data.cfg = cfg;
	notif_data.type = type;
	bcm_notif_signal(bcmh->tplt_upd_notif_hdl, &notif_data);
}

/**
 * These functions register/unregister/invoke the callback
 * when an IBSS is requested to be muted/unmuted.
 */
int
BCMATTACHFN(wlc_ibss_mute_upd_register)(wlc_info_t *wlc, ibss_mute_upd_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->mute_upd_notif_hdl;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

int
BCMATTACHFN(wlc_ibss_mute_upd_unregister)(wlc_info_t *wlc, ibss_mute_upd_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->mute_upd_notif_hdl;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

void
wlc_ibss_mute_upd_notif(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool mute)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	ibss_mute_upd_data_t notif_data;

	notif_data.cfg = cfg;
	notif_data.mute = mute;
	bcm_notif_signal(bcmh->mute_upd_notif_hdl, &notif_data);
}

/**
 * These functions register/unregister/invoke the callback
 * when a pretbtt query is requested.
 */
int
BCMATTACHFN(wlc_bss_pretbtt_query_register)(wlc_info_t *wlc, bss_pretbtt_query_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->pretbtt_query_hdl;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

int
BCMATTACHFN(wlc_bss_pretbtt_query_unregister)(wlc_info_t *wlc, bss_pretbtt_query_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->pretbtt_query_hdl;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

static void
wlc_bss_pretbtt_max(uint *max_pretbtt, bss_pretbtt_query_data_t *notif_data)
{
	if (notif_data->pretbtt > *max_pretbtt)
		*max_pretbtt = notif_data->pretbtt;
}

uint
wlc_bss_pretbtt_query(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint minval)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	bss_pretbtt_query_data_t notif_data;
	uint max_pretbtt = minval;

	notif_data.cfg = cfg;
	notif_data.pretbtt = minval;
	bcm_notif_signal_ex(bcmh->pretbtt_query_hdl, &notif_data,
	                    (bcm_notif_server_callback)wlc_bss_pretbtt_max,
	                    (bcm_notif_client_data)&max_pretbtt);

	return max_pretbtt;
}

/**
 * These functions register/unregister/invoke the callback
 * when an association state has changed.
 */
int
BCMATTACHFN(wlc_bss_assoc_state_register)(wlc_info_t *wlc, bss_assoc_state_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->as_st_notif_hdl;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

int
BCMATTACHFN(wlc_bss_assoc_state_unregister)(wlc_info_t *wlc, bss_assoc_state_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->as_st_notif_hdl;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

void
wlc_bss_assoc_state_notif(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint type, uint state)
{
	bcm_notif_h hdl = wlc->bcmh->as_st_notif_hdl;
	bss_assoc_state_data_t notif_data;

	notif_data.cfg = cfg;
	notif_data.type = type;
	notif_data.state = state;
	bcm_notif_signal(hdl, &notif_data);
}

/* on-channel beacon reception notification */
int
BCMATTACHFN(wlc_bss_rx_bcn_register)(wlc_info_t *wlc, bss_rx_bcn_notif_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->rxbcn_nh;

	return bcm_notif_add_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

int
BCMATTACHFN(wlc_bss_rx_bcn_unregister)(wlc_info_t *wlc, bss_rx_bcn_notif_fn_t fn, void *arg)
{
	bcm_notif_h hdl = wlc->bcmh->rxbcn_nh;

	return bcm_notif_remove_interest(hdl, (bcm_notif_client_callback)fn, arg);
}

void
wlc_bss_rx_bcn_signal(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
	wlc_d11rxhdr_t *wrxh, uint8 *plcp, struct dot11_management_header *hdr,
	uint8 *body, int bcn_len)
{
	bcm_notif_h hdl = wlc->bcmh->rxbcn_nh;
	bss_rx_bcn_notif_data_t data;

	data.cfg = cfg;
	data.scb = scb;
	data.wrxh = wrxh;
	data.plcp = plcp;
	data.hdr = hdr;
	data.body = body;
	data.bcn_len = bcn_len;
	bcm_notif_signal(hdl, &data);
}


/** Return the number of AP bsscfgs that are UP */
int
wlc_ap_bss_up_count(wlc_info_t *wlc)
{
	uint16 i, apbss_up = 0;
	wlc_bsscfg_t *bsscfg;

	FOREACH_UP_AP(wlc, i, bsscfg) {
		apbss_up++;
	}

	return apbss_up;
}

int
wlc_bsscfg_up(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	int ret = BCME_OK;
	bsscfg_module_t *bcmh = wlc->bcmh;
	bsscfg_state_upd_data_t st_data;
	bool stop = FALSE;

	ASSERT(cfg != NULL);
	ASSERT(cfg->enable);

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_up(%s): stas/aps/associated %d/%d/%d"
	               "flags = 0x%x\n", wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
	               (BSSCFG_AP(cfg) ? "AP" : "STA"),
	               wlc->stas_associated, wlc->aps_associated, wlc->pub->associated,
	               cfg->flags));

	bzero(&st_data, sizeof(st_data));
	st_data.cfg = cfg;
	st_data.old_enable = cfg->enable;
	st_data.old_up = cfg->up;

#ifdef AP
	if (BSSCFG_AP(cfg)) {
#ifdef STA
		bool mpc_out = wlc->mpc_out;
#endif

#ifdef STA
		/* bringup the driver */
		wlc->mpc_out = TRUE;
		wlc_radio_mpc_upd(wlc);
#endif

		/* AP mode operation must have the driver up before bringing
		 * up a configuration
		 */
		if (!wlc->pub->up) {
			ret = BCME_NOTUP;
			goto end;
		}

		/* wlc_ap_up() only deals with getting cfg->target_bss setup correctly.
		 * This should not have any affects that need to be undone even if we
		 * don't end up bring the AP up.
		 */
		ret = wlc_ap_up(wlc->ap, cfg);
		if (ret != BCME_OK)
			goto end;

#ifdef WLRSDB
		if (RSDB_ENAB(wlc->pub)) {
			/* Do not call wlc_rsdb_ap_bringup() from wlc_bsscfg_up() when the
			 * AP cfg is already up.
			 */
			if (!cfg->up) {
				ret = wlc_rsdb_ap_bringup(wlc, &cfg);
				if (ret == BCME_NOTREADY) {
					return ret;
				}
				else if (ret != BCME_OK) {
					goto end;
				}
			}
		}
#endif /* WLRSDB */

		/* No SSID configured yet... */
		if (cfg->SSID_len == 0) {
			cfg->up = FALSE;
			goto end;
		}

#ifdef STA
		/* defer to any STA association in progress */
		if (APSTA_ENAB(wlc->pub) && !wlc_apup_allowed(wlc)) {
			cfg->up = FALSE;
			stop = TRUE;
			goto end;
		}
#endif /* STA */

		/* it's ok to update beacon from onwards */
		/* bsscfg->flags &= ~WLC_BSSCFG_DEFER_BCN; */
		/* will be down next anyway... */

		/* Init (non-persistant) flags */
		WLC_BSSCFG_FLAGS_INIT(cfg);
		/* Init (non-persistant) flags2 */
		WLC_BSSCFG_FLAGS2_INIT(cfg);
		if (cfg->flags & WLC_BSSCFG_DYNBCN)
			cfg->flags &= ~WLC_BSSCFG_HW_BCN;

		WL_APSTA_UPDN(("wl%d: wlc_bsscfg_up(%s): flags = 0x%x\n",
			wlc->pub->unit, (BSSCFG_AP(cfg) ? "AP" : "STA"), cfg->flags));

#ifdef MBSS
		if (wlc_mbss_bsscfg_up(wlc, cfg) != BCME_OK)
			goto end;
#endif /* MBSS */

		cfg->up = TRUE;

		wlc_bss_up(wlc->ap, cfg);

	end:
		if (cfg->up) {
			WL_INFORM(("wl%d: BSS %d is up\n", wlc->pub->unit, cfg->_idx));
#if defined(WLRSDB) && defined(WL_MODESW)
			if (WLC_MODESW_ENAB(wlc->pub)) {
				wlc_set_ap_up_pending(wlc, cfg, FALSE);
			}
#endif
		} else {
			WL_ERROR(("wl%d: %s BSS %d not up\n", WLCWLUNIT(wlc), __FUNCTION__,
				WLC_BSSCFG_IDX(cfg)));
		}
#ifdef STA
		wlc->mpc_out = mpc_out;
		wlc_radio_mpc_upd(wlc);
		wlc_set_wake_ctrl(wlc);
#endif
	}
#endif /* AP */

#ifdef STA
	if (BSSCFG_STA(cfg)) {
		cfg->up = TRUE;
	}
#endif

	if (stop || ret != BCME_OK)
		return ret;

	/* invoke bsscfg state change callbacks */
	WL_BCB(("wl%d.%d: %s: notify clients of bsscfg state change. enable %d>%d up %d>%d.\n",
	        wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
	        st_data.old_enable, cfg->enable, st_data.old_up, cfg->up));
	bcm_notif_signal(bcmh->state_upd_notif_hdl, &st_data);

	/* invoke bsscfg up callbacks */
	/* TO BE REPLACED BY ABOVE STATE CHANGE CALLBACK */
	{
	bsscfg_up_down_event_data_t evt_data;
	memset(&evt_data, 0, sizeof(evt_data));
	evt_data.bsscfg = cfg;
	evt_data.up     = TRUE;
	bcm_notif_signal(bcmh->up_down_notif_hdl, &evt_data);
	}

	return ret;
}

/* Count real IBSS (beaconing by PSM) connections.
 * This could have been done through checking HWBCN flag of the bsscfg
 * without the conditional compilation...
 */
static uint
wlc_ibss_cnt(wlc_info_t *wlc)
{
	uint ibss_bsscfgs = wlc->ibss_bsscfgs;

#if defined(WL_PROXDETECT) || defined(WL_NAN)
	{
	int idx;
	wlc_bsscfg_t *cfg;
	/* infrastructure STA(BSS) can co-exist with AWDL STA(IBSS) and PROXD STA(IBSS) */
	FOREACH_AS_STA(wlc, idx, cfg) {
		if (BSS_PROXD_ENAB(wlc, cfg) || BSSCFG_NAN(cfg) || BSSCFG_AWDL(wlc, cfg))
			ibss_bsscfgs--;
	}
	}
#endif 

	return ibss_bsscfgs;
}

/** Enable: always try to force up */
int
wlc_bsscfg_enable(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	bsscfg_state_upd_data_t st_data;

	ASSERT(bsscfg != NULL);

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_enable: currently %s\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
		(bsscfg->enable ? "ENABLED" : "DISABLED")));

	bzero(&st_data, sizeof(st_data));
	st_data.cfg = bsscfg;
	st_data.old_enable = bsscfg->enable;
	st_data.old_up = bsscfg->up;

	if (!MBSS_ENAB(wlc->pub)) {
		/* block simultaneous multiple same band AP connection */
		if (RSDB_ENAB(wlc->pub)) {
			if (BSSCFG_AP(bsscfg) && AP_ACTIVE(wlc)) {
				int idx;
				wlc_bsscfg_t *cfg;
				FOREACH_BSS(wlc, idx, cfg) {
					if (cfg == bsscfg)
						continue;
					if (BSSCFG_AP_UP(cfg) &&
						CHSPEC_BANDUNIT(cfg->current_bss->chanspec) ==
						CHSPEC_BANDUNIT(wlc->default_bss->chanspec)) {
						WL_ERROR(("wl%d.%d: Cannot enable "
							"multiple AP bsscfg in same band\n",
							wlc->pub->unit,
							bsscfg->_idx));
						return BCME_ERROR;
					}
				}
			}
		}
		/* block simultaneous IBSS and AP connection */
		if (BSSCFG_AP(bsscfg)) {
			uint ibss_bsscfgs = wlc_ibss_cnt(wlc);
			if (ibss_bsscfgs) {
				WL_ERROR(("wl%d: Cannot enable AP bsscfg with a IBSS\n",
					wlc->pub->unit));
				return BCME_ERROR;
			}
		}
	}

	bsscfg->enable = TRUE;

	if (BSSCFG_AP(bsscfg)) {
#ifdef MBSS
		/* make sure we don't exceed max */
		if (MBSS_ENAB(wlc->pub) &&
		    ((uint32)AP_BSS_UP_COUNT(wlc) >= (uint32)WLC_MAX_AP_BSS(wlc->pub->corerev))) {
			WL_ERROR(("wl%d: max %d ap bss allowed\n",
			          wlc->pub->unit, WLC_MAX_AP_BSS(wlc->pub->corerev)));
			bsscfg->enable = FALSE;
			return BCME_ERROR;
		}
#endif /* MBSS */
	}

	/* invoke bsscfg state change callbacks */
	WL_BCB(("wl%d.%d: %s: notify clients of bsscfg state change. enable %d>%d up %d>%d.\n",
	        wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
	        st_data.old_enable, bsscfg->enable, st_data.old_up, bsscfg->up));
	bcm_notif_signal(bcmh->state_upd_notif_hdl, &st_data);

	if (BSSCFG_AP(bsscfg)) {
		return wlc_bsscfg_up(wlc, bsscfg);
	}

	/* wlc_bsscfg_up() will be called for STA assoication code:
	 * - for IBSS, in wlc_join_start_ibss() and in wlc_join_BSS()
	 * - for BSS, in wlc_assoc_complete()
	 */
	/*
	 * if (BSSCFG_STA(bsscfg)) {
	 *	return BCME_OK;
	 * }
	 */

	return BCME_OK;
}

static void
wlc_bsscfg_down_cbpend_sum(uint *cbpend_sum, bsscfg_up_down_event_data_t *notif_data)
{
	*cbpend_sum = *cbpend_sum + notif_data->callbacks_pending;
}

static void
wlc_bsscfg_state_upd_cbpend_sum(uint *cbpend_sum, bsscfg_state_upd_data_t *notif_data)
{
	*cbpend_sum = *cbpend_sum + notif_data->callbacks_pending;
}

int
wlc_bsscfg_down(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	int callbacks = 0;
	bsscfg_module_t *bcmh = wlc->bcmh;
	bsscfg_state_upd_data_t st_data;
	uint cbpend_sum = 0;

	ASSERT(cfg != NULL);

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_down: currently %s %s; stas/aps/associated %d/%d/%d\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
	               (cfg->up ? "UP" : "DOWN"), (BSSCFG_AP(cfg) ? "AP" : "STA"),
	               wlc->stas_associated, wlc->aps_associated, wlc->pub->associated));

	if (!cfg->up) {
#ifdef AP
		if (BSSCFG_AP(cfg) && cfg->associated) {
			/* For AP, cfg->up can be 0 but down never called.
			 * Thus, it's best to check for both !up and !associated
			 * before we decide to skip the down procedures.
			 */
			WL_APSTA_UPDN(("wl%d: AP cfg up = %d but associated, "
			               "continue with down procedure.\n",
			               wlc->pub->unit, cfg->up));
		}
		else
#endif
		return 0;
	}

	bzero(&st_data, sizeof(st_data));
	st_data.cfg = cfg;
	st_data.old_enable = cfg->enable;
	st_data.old_up = cfg->up;

	/* bring down this config */
	cfg->up = FALSE;

	if (wlc_bsscfg_preserve(wlc, cfg)) {
		goto do_down;
	}

	/* invoke bsscfg down callbacks */
	/* TO BE REPLACED BY BELOW STATE CHANGE CALLBACK */
	{
	bsscfg_up_down_event_data_t evt_data;
	memset(&evt_data, 0, sizeof(evt_data));
	evt_data.bsscfg = cfg;
	bcm_notif_signal_ex(bcmh->up_down_notif_hdl, &evt_data,
		(bcm_notif_server_callback)wlc_bsscfg_down_cbpend_sum,
		(bcm_notif_server_context)&cbpend_sum);
	/* Clients update the number of pending asynchronous callbacks in the
	 * driver down path.
	 */
	callbacks += cbpend_sum;
	}

	/* invoke bsscfg state change callbacks */
	WL_BCB(("wl%d.%d: %s: notify clients of bsscfg state change. enable %d>%d up %d>%d.\n",
	        wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
	        st_data.old_enable, cfg->enable, st_data.old_up, cfg->up));
	bcm_notif_signal_ex(bcmh->state_upd_notif_hdl, &st_data,
		(bcm_notif_server_callback)wlc_bsscfg_state_upd_cbpend_sum,
		(bcm_notif_server_context)&cbpend_sum);
	/* Clients update the number of pending asynchronous callbacks in the
	 * driver down path.
	 */
	callbacks += cbpend_sum;

do_down:
#ifdef AP
	if (BSSCFG_AP(cfg)) {

		callbacks += wlc_ap_down(wlc->ap, cfg);

#ifdef MBSS
		wlc_mbss_bsscfg_down(wlc, cfg);
#endif /* MBSS */

#ifdef STA
		wlc_radio_mpc_upd(wlc);
		wlc_set_wake_ctrl(wlc);
#endif
	}
#endif /* AP */


	return callbacks;
}

int
wlc_bsscfg_disable(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	bsscfg_state_upd_data_t st_data;
	int callbacks = 0;
	uint cbpend_sum = 0;

	ASSERT(bsscfg != NULL);

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_disable: currently %s\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
		(bsscfg->enable ? "ENABLED" : "DISABLED")));


	/* If a bss is already disabled, don't do anything */
	if (!bsscfg->enable) {
		ASSERT(!bsscfg->up);
		return 0;
	}

	callbacks += wlc_bsscfg_down(wlc, bsscfg);
	ASSERT(!bsscfg->up);

	bzero(&st_data, sizeof(st_data));
	st_data.cfg = bsscfg;
	st_data.old_enable = bsscfg->enable;
	st_data.old_up = bsscfg->up;

	bsscfg->flags &= ~WLC_BSSCFG_PRESERVE;

	bsscfg->enable = FALSE;

	/* invoke bsscfg state change callbacks */
	WL_BCB(("wl%d.%d: %s: notify clients of bsscfg state change. enable %d>%d up %d>%d.\n",
	        wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
	        st_data.old_enable, bsscfg->enable, st_data.old_up, bsscfg->up));
	bcm_notif_signal_ex(bcmh->state_upd_notif_hdl, &st_data,
		(bcm_notif_server_callback)wlc_bsscfg_state_upd_cbpend_sum,
		(bcm_notif_server_context)&cbpend_sum);
	/* Clients update the number of pending asynchronous callbacks in the
	 * driver down path.
	 */
	callbacks += cbpend_sum;

	return callbacks;
}

static int
wlc_bsscfg_cubby_init(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	uint len;

	/* reset all cubbies */
	len = wlc_cubby_totsize(bcmh->cubby_info);
	memset((uint8 *)cfg + bcmh->cfgtotsize, 0, len);

	return wlc_cubby_init(bcmh->cubby_info, cfg, NULL, NULL);
}

static void
wlc_bsscfg_cubby_deinit(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bsscfg_module_t *bcmh = wlc->bcmh;

	wlc_cubby_deinit(bcmh->cubby_info, cfg, NULL, NULL);
}

#ifdef AP
static int
wlc_bsscfg_ap_init(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	int ret;

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_ap_init:\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));

	/* Init flags: Beacons/probe resp in HW by default */
	bsscfg->flags |= (WLC_BSSCFG_HW_BCN | WLC_BSSCFG_HW_PRB);
	if (bsscfg->flags & WLC_BSSCFG_DYNBCN)
		bsscfg->flags &= ~WLC_BSSCFG_HW_BCN;

#ifndef RADAR
		/* Hard Coding the cfg->flags to not support Radar channel operation */
		bsscfg->flags |= WLC_BSSCFG_AP_NORADAR_CHAN;
#endif

#if defined(MBSS) || defined(WLP2P)
	bsscfg->maxassoc = wlc->pub->tunables->maxscb;
#endif /* MBSS || WLP2P */
#if defined(MBSS)
	bsscfg->bcmc_fid = INVALIDFID;
	bsscfg->bcmc_fid_shm = INVALIDFID;
#endif

	BSSCFG_SET_PSINFO(bsscfg);

#ifdef BCMPCIEDEV_ENABLED
	bsscfg->ap_isolate = AP_ISOLATE_SENDUP_ALL;
#endif

	/* invoke bsscfg cubby init function */
	if ((ret = wlc_bsscfg_cubby_init(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_cubby_init failed with err %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, ret));
		return ret;
	}

	/* Allocate a broadcast SCB for each band */
	if ((ret = wlc_bsscfg_bcmcscballoc(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_bcmcscballoc failed with err %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, ret));
		return ret;
	}

	return BCME_OK;
}

static void
wlc_bsscfg_ap_deinit(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_ap_deinit:\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));

	wlc_bsscfg_bcmcscbfree(wlc, bsscfg);

	/* invoke bsscfg cubby deinit function */
	wlc_bsscfg_cubby_deinit(wlc, bsscfg);

	bsscfg->flags &= ~(WLC_BSSCFG_HW_BCN | WLC_BSSCFG_HW_PRB);
}
#endif /* AP */

#ifdef STA
static int
wlc_bsscfg_sta_init(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	int ret;

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_sta_init:\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));

	/* invoke bsscfg cubby init function */
	if ((ret = wlc_bsscfg_cubby_init(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_cubby_init failed with err %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, ret));
		return ret;
	}

	/* Allocate a broadcast SCB for each band */
	if ((ret = wlc_bsscfg_bcmcscballoc(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_bcmcscballoc failed with err %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, ret));
		return ret;
	}

	return BCME_OK;
}

static void
wlc_bsscfg_sta_deinit(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_sta_deinit:\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));

	wlc_bsscfg_bcmcscbfree(wlc, bsscfg);

	/* invoke bsscfg cubby deinit function */
	wlc_bsscfg_cubby_deinit(wlc, bsscfg);

	if (bsscfg->current_bss != NULL) {
		wlc_bss_info_t *current_bss = bsscfg->current_bss;
		if (current_bss->bcn_prb != NULL) {
			MFREE(wlc->osh, current_bss->bcn_prb, current_bss->bcn_prb_len);
			current_bss->bcn_prb = NULL;
			current_bss->bcn_prb_len = 0;
		}
	}

	wlc_bsscfg_scan_params_reset(wlc, bsscfg);
}
#endif /* STA */

static int
wlc_bsscfg_bss_rsinit(wlc_info_t *wlc, wlc_bss_info_t *bi, uint8 rates, uint8 bw, uint8 mcsallow)
{
	wlc_rateset_t *src = &wlc->band->hw_rateset;
	wlc_rateset_t *dst = &bi->rateset;

	wlc_rateset_filter(src, dst, FALSE, rates, RATE_MASK_FULL, mcsallow);
	if (dst->count == 0)
		return BCME_NORESOURCE;
#ifdef WL11N
	wlc_rateset_bw_mcs_filter(dst, bw);
#endif
	wlc_rate_lookup_init(wlc, dst);

	return BCME_OK;
}

int
wlc_bsscfg_rateset_init(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint8 rates, uint8 bw, uint8 mcsallow)
{
	int err;

	if ((err = wlc_bsscfg_bss_rsinit(wlc, cfg->target_bss, rates, bw, mcsallow)) != BCME_OK)
		return err;
	if ((err = wlc_bsscfg_bss_rsinit(wlc, cfg->current_bss, rates, bw, mcsallow)) != BCME_OK)
		return err;

	return err;
}

#ifdef STA
/* map wlc_bss_info_t::bss_type to bsscfg_type_t */
/* There's not enough information to map bss_type to bsscfg type uniquely
 * so limit it to primary bsscfg and STA only.
 */
typedef struct {
	uint8 type;
	uint8 subtype;
} bsscfg_cfg_type_t;

static const bsscfg_cfg_type_t bsscfg_cfg_types[] = {
	/* DOT11_BSSTYPE_INFRASTRUCTURE */ {BSSCFG_TYPE_GENERIC, BSSCFG_GENERIC_STA},
	/* DOT11_BSSTYPE_INDEPENDENT */ {BSSCFG_TYPE_GENERIC, BSSCFG_GENERIC_IBSS},
	/* DOT11_BSSTYPE_ANY */ {0, 0},	/* place holder */
	/* DOT11_BSSTYPE_MESH */ {BSSCFG_TYPE_MESH, BSSCFG_SUBTYPE_NONE}
};

static const bsscfg_cfg_type_t *
BCMRAMFN(get_bsscfg_cfg_types_tbl)(uint8 *len)
{
	*len = (uint8)ARRAYSIZE(bsscfg_cfg_types);
	return bsscfg_cfg_types;
}

int
wlc_bsscfg_type_bi2cfg(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	wlc_bss_info_t *bi, wlc_bsscfg_type_t *type)
{
	uint8 bsscfg_cfg_types_tbl_len = 0;
	const bsscfg_cfg_type_t *bsscfg_cfg_types_tbl =
	        get_bsscfg_cfg_types_tbl(&bsscfg_cfg_types_tbl_len);
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(cfg);

	if (bi->bss_type < bsscfg_cfg_types_tbl_len &&
	    bi->bss_type != DOT11_BSSTYPE_ANY) {
		type->type = bsscfg_cfg_types_tbl[bi->bss_type].type;
		type->subtype = bsscfg_cfg_types_tbl[bi->bss_type].subtype;
		return BCME_OK;
	}

	return BCME_BADARG;
}
#endif /* STA */

/* is the BSS with type & subtype of AP */
typedef struct {
	uint8 type;
	uint8 subtype;
} bsscfg_ap_type_t;

static const bsscfg_ap_type_t bsscfg_ap_types[] = {
	{BSSCFG_TYPE_GENERIC, BSSCFG_GENERIC_AP},
	{BSSCFG_TYPE_BTA, BSSCFG_BTA_INI},
	{BSSCFG_TYPE_P2P, BSSCFG_P2P_GO}
};

static const bsscfg_ap_type_t *
BCMRAMFN(get_bsscfg_ap_types_tbl)(uint8 *len)
{
	*len = (uint8)ARRAYSIZE(bsscfg_ap_types);
	return bsscfg_ap_types;
}

static bool
wlc_bsscfg_type_isAP(wlc_bsscfg_type_t *type)
{
	uint8 bsscfg_ap_types_tbl_len = 0;
	const bsscfg_ap_type_t *bsscfg_ap_types_tbl =
	        get_bsscfg_ap_types_tbl(&bsscfg_ap_types_tbl_len);
	uint i;

	for (i = 0; i < bsscfg_ap_types_tbl_len; i ++) {
		if (bsscfg_ap_types_tbl[i].type == type->type &&
		    bsscfg_ap_types_tbl[i].subtype == type->subtype) {
			return TRUE;
		}
	}

	return FALSE;
}

/* is the BSS with type & subtype of Infra BSS */
typedef struct {
	uint8 type;
	uint8 subtype;
} bsscfg_bss_type_t;

static const bsscfg_bss_type_t bsscfg_bss_types[] = {
	{BSSCFG_TYPE_GENERIC, BSSCFG_GENERIC_AP},
	{BSSCFG_TYPE_GENERIC, BSSCFG_GENERIC_STA},
	{BSSCFG_TYPE_BTA, BSSCFG_BTA_INI},
	{BSSCFG_TYPE_BTA, BSSCFG_BTA_RESP},
	{BSSCFG_TYPE_P2P, BSSCFG_P2P_GO},
	{BSSCFG_TYPE_P2P, BSSCFG_P2P_GC},
	{BSSCFG_TYPE_PSTA, BSSCFG_SUBTYPE_NONE}
};

static const bsscfg_bss_type_t *
BCMRAMFN(get_bsscfg_bss_types_tbl)(uint8 *len)
{
	*len = (uint8)ARRAYSIZE(bsscfg_bss_types);
	return bsscfg_bss_types;
}

static bool
wlc_bsscfg_type_isBSS(wlc_bsscfg_type_t *type)
{
	uint8 bsscfg_bss_types_tbl_len = 0;
	const bsscfg_bss_type_t *bsscfg_bss_types_tbl =
	        get_bsscfg_bss_types_tbl(&bsscfg_bss_types_tbl_len);
	uint i;

	for (i = 0; i < bsscfg_bss_types_tbl_len; i ++) {
		if (bsscfg_bss_types_tbl[i].type == type->type &&
		    (bsscfg_bss_types_tbl[i].subtype == BSSCFG_SUBTYPE_NONE ||
		     bsscfg_bss_types_tbl[i].subtype == type->subtype)) {
			return TRUE;
		}
	}

	return FALSE;
}

/* map type & subtype variables to _ap & BSS variables */
static void
wlc_bsscfg_type_set(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bsscfg_type_t *type)
{
	BCM_REFERENCE(wlc);
	cfg->type = type->type;
	cfg->_ap = wlc_bsscfg_type_isAP(type);
	cfg->BSS = wlc_bsscfg_type_isBSS(type);
}


/* TODO: fold this minimum reinit into the regular reinit logic to make sure
 * it won't diverge in the future...
 */
static int
wlc_bsscfg_reinit_min(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bsscfg_type_t *type, uint flags)
{
	wlc_bsscfg_type_t old_type;

	if (!BSSCFG_STA(cfg)) {
		WL_ERROR(("wl%d.%d: %s: Bad param as_in_prog for non STA bsscfg\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));
		return BCME_NOTSTA;
	}

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_reinit_min: flags 0x%x type %s subtype %s\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(cfg), flags,
	               wlc_bsscfg_type_name(type), wlc_bsscfg_subtype_name(type)));

	wlc_bsscfg_type_get(wlc, cfg, &old_type);

	if (old_type.type == type->type &&
	    old_type.subtype == type->subtype) {
		return BCME_OK;
	}

	cfg->flags = flags;

	wlc_bsscfg_type_set(wlc, cfg, type);

	wlc_if_event(wlc, WLC_E_IF_CHANGE, cfg->wlcif);

	return BCME_OK;
}

/* Fixup bsscfg type according to the given type and subtype.
 * Only Infrastructure and Independent BSS types can be switched if min_reinit
 * (minimum reinit) is TRUE.
 */
int
wlc_bsscfg_type_fixup(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bsscfg_type_t *type,
	bool min_reinit)
{
	int err;

	ASSERT(cfg != NULL);

	/* Fixup BSS (Infra vs Indep) only when we are in association process */
	if (min_reinit) {
		if ((err = wlc_bsscfg_reinit_min(wlc, cfg, type, cfg->flags)) != BCME_OK) {
			WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_reinit_min failed, err %d\n",
			          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__, err));
			return err;
		}
	}
	/* It's ok to perform a full reinit */
	else {
		if ((err = wlc_bsscfg_reinit(wlc, cfg, type, cfg->flags)) != BCME_OK) {
			WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_reinit failed, err %d\n",
			          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__, err));
			return err;
		}
	}

	/* AP has these flags set in wlc_bsscfg_ap_init() */
	if (BSSCFG_STA(cfg)) {
		/* IBSS deploys PSM bcn/prbrsp */
		if (!cfg->BSS &&
		    !(cfg->flags & (WLC_BSSCFG_SW_BCN | WLC_BSSCFG_SW_PRB)))
			cfg->flags |= (WLC_BSSCFG_HW_BCN | WLC_BSSCFG_HW_PRB);
		/* reset in case of a role change between Infra STA and IBSS STA */
		else
			cfg->flags &= ~(WLC_BSSCFG_HW_BCN | WLC_BSSCFG_HW_PRB);
	}

	return BCME_OK;
}

int
wlc_bsscfg_get_free_idx(wlc_info_t *wlc)
{
	int idx;

	for (idx = 0; idx < WLC_MAXBSSCFG; idx++) {
		if (wlc->bsscfg[idx] == NULL) {
			return idx;
		}
	}

	return BCME_ERROR;
}

void
wlc_bsscfg_ID_assign(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	bsscfg_module_t *bcmh = wlc->bcmh;

	bsscfg->ID = bcmh->next_bsscfg_ID;
	bcmh->next_bsscfg_ID ++;
}

/**
 * After all the modules indicated how much cubby space they need in the bsscfg, the actual
 * wlc_bsscfg_t can be allocated. This happens one time fairly late within the attach phase, but
 * also when e.g. communication with a new remote party is started.
 */
static wlc_bsscfg_t *
wlc_bsscfg_malloc(wlc_info_t *wlc)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	osl_t *osh;
	wlc_bsscfg_t *cfg;
	int malloc_size;
	uint offset;
	uint len;

	osh = wlc->osh;

	len = bcmh->cfgtotsize + wlc_cubby_totsize(bcmh->cubby_info);
	if ((cfg = (wlc_bsscfg_t *)MALLOCZ(osh, len)) == NULL) {
		malloc_size = len;
		goto fail;
	}

	offset = ROUNDUP(sizeof(*cfg), PTRSZ);
	cfg->ext_cap = (uint8 *)cfg + offset;
	/* offset += ROUNDUP(bcmh->ext_cap_max, PTRSZ); */


	if ((cfg->multicast = (struct ether_addr *)
	     MALLOCZ(osh, (sizeof(struct ether_addr)*MAXMULTILIST))) == NULL) {
		malloc_size = sizeof(struct ether_addr) * MAXMULTILIST;
		goto fail;
	}

	if ((cfg->cxn = (wlc_cxn_t *)
		MALLOCZ(osh, sizeof(wlc_cxn_t))) == NULL) {
		malloc_size = sizeof(wlc_cxn_t);
		goto fail;
	}
	malloc_size = sizeof(wlc_bss_info_t);
	if ((cfg->current_bss = (wlc_bss_info_t *) MALLOCZ(osh, malloc_size)) == NULL) {
		goto fail;
	}
	if ((cfg->target_bss = (wlc_bss_info_t *) MALLOCZ(osh, malloc_size)) == NULL) {
		goto fail;
	}

	/* BRCM IE */
	if ((cfg->brcm_ie = (uint8 *)
	     MALLOCZ(osh, WLC_MAX_BRCM_ELT)) == NULL) {
		malloc_size = WLC_MAX_BRCM_ELT;
		goto fail;
	}

	return cfg;

fail:
	WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__, malloc_size, MALLOCED(wlc->osh)));
	wlc_bsscfg_mfree(wlc, cfg);
	return NULL;
}

static void
wlc_bsscfg_mfree(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bsscfg_module_t *bcmh = wlc->bcmh;
	osl_t *osh;
	uint len;

	if (cfg == NULL)
		return;

	osh = wlc->osh;

	if (cfg->multicast) {
		MFREE(osh, cfg->multicast, (sizeof(struct ether_addr) * MAXMULTILIST));
		cfg->multicast = NULL;
	}

	if (cfg->cxn != NULL) {
		MFREE(osh, cfg->cxn, sizeof(wlc_cxn_t));
		cfg->cxn = NULL;
	}
	if (cfg->current_bss != NULL) {
		wlc_bss_info_t *current_bss = cfg->current_bss;
		if (current_bss->bcn_prb != NULL) {
			MFREE(osh, current_bss->bcn_prb, current_bss->bcn_prb_len);
			current_bss->bcn_prb = NULL;
			current_bss->bcn_prb_len = 0;
		}
		MFREE(osh, current_bss, sizeof(wlc_bss_info_t));
		cfg->current_bss = NULL;
	}
	if (cfg->target_bss != NULL) {
		MFREE(osh, cfg->target_bss, sizeof(wlc_bss_info_t));
		cfg->target_bss = NULL;
	}

	/* BRCM IE */
	if (cfg->brcm_ie != NULL) {
		MFREE(osh, cfg->brcm_ie, WLC_MAX_BRCM_ELT);
		cfg->brcm_ie = NULL;
	}


	len = bcmh->cfgtotsize + wlc_cubby_totsize(bcmh->cubby_info);
	MFREE(osh, cfg, len);
}

/**
 * Called when e.g. a wireless interface is added, during the ATTACH phase, or as a result of an
 * IOVAR.
 */
wlc_bsscfg_t *
wlc_bsscfg_alloc(wlc_info_t *wlc, int idx, wlc_bsscfg_type_t *type,
	uint flags, struct ether_addr *ea)
{
	wlc_bsscfg_t *bsscfg;
	int err;

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_alloc: flags 0x%08x type %s subtype %s\n",
	               wlc->pub->unit, idx, flags,
	               wlc_bsscfg_type_name(type), wlc_bsscfg_subtype_name(type)));

	if (idx < 0 || idx >= WLC_MAXBSSCFG) {
		return NULL;
	}

	if ((bsscfg = wlc_bsscfg_malloc(wlc)) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	if ((err = wlc_bsscfg_alloc_int(wlc, bsscfg, idx, type, flags,
			ea != NULL ? ea : &wlc->pub->cur_etheraddr)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_alloc_int failed with %d\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

	return bsscfg;

fail:
	if (bsscfg != NULL)
		wlc_bsscfg_free(wlc, bsscfg);
	return NULL;
}

/* force to reset the existing bsscfg to given type+flags+ea */
int
wlc_bsscfg_reset(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, wlc_bsscfg_type_t *type,
	uint flags, struct ether_addr *ea)
{
	int err;
	wlc_bsscfg_type_t old_type;

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_reset: flags 0x%08x type %s subtype %s\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), flags,
	               wlc_bsscfg_type_name(type), wlc_bsscfg_subtype_name(type)));

	wlc_bsscfg_type_get(wlc, bsscfg, &old_type);

	if (old_type.type == type->type &&
		old_type.subtype == type->subtype &&
		bsscfg->flags == flags) {
		return BCME_OK;
	}

	wlc_bsscfg_deinit(wlc, bsscfg);

	/* clear SSID */
	memset(bsscfg->SSID, 0, DOT11_MAX_SSID_LEN);
	bsscfg->SSID_len = 0;

	/* clear all configurations */
	if ((err = wlc_bsscfg_init_int(wlc, bsscfg, type, flags, ea)) != BCME_OK) {
		WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_init_int failed, err %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, err));
		return err;
	}

	if ((err = wlc_bsscfg_init(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_init failed, err %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, err));
		return err;
	}

	wlc_if_event(wlc, WLC_E_IF_CHANGE, bsscfg->wlcif);

	return err;
}

static void
wlc_bsscfg_bcmcscbfree(struct wlc_info *wlc, wlc_bsscfg_t *bsscfg)
{
	int ii;

	for (ii = 0; ii < MAXBANDS; ii++) {
		if (bsscfg->bcmc_scb[ii]) {
			WL_INFORM(("bcmc_scb: band %d: free internal scb for 0x%p\n",
			           ii, bsscfg->bcmc_scb[ii]));
			wlc_bcmcscb_free(wlc, bsscfg->bcmc_scb[ii]);
			bsscfg->bcmc_scb[ii] = NULL;
		}
	}
}

void
wlc_bsscfg_deinit(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	if (BSSCFG_AP(bsscfg)) {
#ifdef AP
		wlc_bsscfg_ap_deinit(wlc, bsscfg);
#endif
	}
	else {
#ifdef STA
		wlc_bsscfg_sta_deinit(wlc, bsscfg);
#endif
	}
}

/* delete network interface and free wlcif structure */
static void
wlc_bsscfg_deinit_intf(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	if (bsscfg != wlc_bsscfg_primary(wlc) && bsscfg->wlcif != NULL &&
	    /* RSDB: Donot delete (wlif) host interface, incase of RSDB clone
	     * Interface exists, but BSSCFG with WLCIF is moved to different WLC
	     */
	    !BSSCFG_IS_RSDB_CLONE(bsscfg)) {
		wlc_if_event(wlc, WLC_E_IF_DEL, bsscfg->wlcif);
		if (bsscfg->wlcif->wlif != NULL) {
			wl_del_if(wlc->wl, bsscfg->wlcif->wlif);
			bsscfg->wlcif->wlif = NULL;
		}
	}
	wlc_wlcif_free(wlc, wlc->osh, bsscfg->wlcif);
	bsscfg->wlcif = NULL;
}

void
wlc_bsscfg_free(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	int idx;

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_free: flags = 0x%x\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), bsscfg->flags));


	wlc_bsscfg_deinit(wlc, bsscfg);

	/* delete the upper-edge driver interface */
	wlc_bsscfg_deinit_intf(wlc, bsscfg);

	/* free the wlc_bsscfg struct if it was an allocated one */
	idx = bsscfg->_idx;
	wlc_bsscfg_mfree(wlc, bsscfg);
	wlc->bsscfg[idx] = NULL;
	if (bsscfg == wlc_bsscfg_primary(wlc)) {
		wlc->cfg = NULL;
	}
}

int
wlc_bsscfg_init(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	int ret;

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_init:\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));

#if defined(AP) && defined(STA)
	if (BSSCFG_AP(bsscfg)) {
		ret = wlc_bsscfg_ap_init(wlc, bsscfg);
	} else {
		ret = wlc_bsscfg_sta_init(wlc, bsscfg);
	}
#elif defined(AP)
	ret = wlc_bsscfg_ap_init(wlc, bsscfg);
#elif defined(STA)
	ret = wlc_bsscfg_sta_init(wlc, bsscfg);
#endif

	return ret;
}

/* reset the existing bsscfg to the given type, generate host event */
int
wlc_bsscfg_reinit(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, wlc_bsscfg_type_t *type,
	uint flags)
{
	int ret;
	wlc_bsscfg_type_t old_type;

	WL_APSTA_UPDN(("wl%d.%d: wlc_bsscfg_reinit: flags 0x%x type %s subtype %s\n",
	               wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), flags,
	               wlc_bsscfg_type_name(type), wlc_bsscfg_subtype_name(type)));

	wlc_bsscfg_type_get(wlc, bsscfg, &old_type);

	if (old_type.type == type->type &&
		old_type.subtype == type->subtype &&
		bsscfg->flags == flags) {
		return BCME_OK;
	}

	wlc_bsscfg_deinit(wlc, bsscfg);

	bsscfg->flags = flags;

	wlc_bsscfg_type_set(wlc, bsscfg, type);

	if ((ret = wlc_bsscfg_init(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d.%d: %s: wlc_bsscfg_init failed, err %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, ret));
		return ret;
	}

	wlc_if_event(wlc, WLC_E_IF_CHANGE, bsscfg->wlcif);

	return ret;
}

/* query type & subtype */
static void
wlc_bsscfg_type_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bsscfg_type_t *type)
{
	type->type = cfg->type;
	type->subtype = 0;

	if (type->type == BSSCFG_TYPE_GENERIC) {
		if (BSSCFG_AP(cfg)) {
			type->subtype = BSSCFG_GENERIC_AP;
		} else if (cfg->BSS) {
			type->subtype = BSSCFG_GENERIC_STA;
		} else {
			type->subtype = BSSCFG_GENERIC_IBSS;
		}
	}
	else if (type->type == BSSCFG_TYPE_P2P) {
		if (P2P_GO(wlc, cfg)) {
			type->subtype = BSSCFG_P2P_GO;
		}
		else if (P2P_CLIENT(wlc, cfg)) {
			type->subtype = BSSCFG_P2P_GC;
		}
		else if (BSS_P2P_DISC_ENAB(wlc, cfg)) {
			type->subtype = BSSCFG_P2P_DISC;
		}
	}
}

/* query info */
static void
wlc_bsscfg_info_get(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bsscfg_info_t *info)
{
	wlc_bsscfg_type_t type;
	wlc_bsscfg_type_get(wlc, cfg, &type);
	info->type = type.type;
	info->subtype = type.subtype;
}

/**
 * Get a bsscfg pointer, failing if the bsscfg does not alreay exist.
 * Sets the bsscfg pointer in any event.
 * Returns BCME_RANGE if the index is out of range or BCME_NOTFOUND
 * if the wlc->bsscfg[i] pointer is null
 */
wlc_bsscfg_t *
wlc_bsscfg_find(wlc_info_t *wlc, int idx, int *perr)
{
	wlc_bsscfg_t *bsscfg;

	if ((idx < 0) || (idx >= WLC_MAXBSSCFG)) {
		if (perr)
			*perr = BCME_RANGE;
		return NULL;
	}

	bsscfg = wlc->bsscfg[idx];
	if (perr)
		*perr = bsscfg ? 0 : BCME_NOTFOUND;

	return bsscfg;
}

/**
 * allocs/inits that can't go in wlc_bsscfg_malloc() (which is
 * called when wlc->wl for example may be invalid...)
 * come in here.
 */
static int
wlc_bsscfg_alloc_int(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, int idx,
	wlc_bsscfg_type_t *type, uint flags, struct ether_addr *ea)
{
	int err;

	wlc->bsscfg[idx] = bsscfg;
	bsscfg->_idx = (int8)idx;

	wlc_bsscfg_ID_assign(wlc, bsscfg);

	if ((err = wlc_bsscfg_init_int(wlc, bsscfg, type, flags, ea)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_init_int() failed with %d\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

	/* create a new upper-edge driver interface */
	if ((err = wlc_bsscfg_init_intf(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_init_intf() failed with %d\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

	return BCME_OK;

fail:
	return err;
}

wlc_bsscfg_t *
wlc_bsscfg_primary(wlc_info_t *wlc)
{
	return wlc->cfg;
}

/**
 * Called fairly late during wlc_attach(), when most modules already completed attach.
 */
int
BCMATTACHFN(wlc_bsscfg_primary_init)(wlc_info_t *wlc)
{
	wlc_bsscfg_t *bsscfg = NULL;
	int err;
	int idx;
	wlc_bsscfg_type_t type = {BSSCFG_TYPE_GENERIC, BSSCFG_SUBTYPE_NONE};

	idx = wlc_bsscfg_get_free_idx(wlc);
	if (idx == BCME_ERROR) {
		WL_ERROR(("wl%d: no free bsscfg\n", wlc->pub->unit));
		err = BCME_NORESOURCE;
		goto fail;
	}

	if ((bsscfg = wlc_bsscfg_malloc(wlc)) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		err = BCME_NOMEM;
		goto fail;
	}
	wlc->cfg = bsscfg;

	type.subtype = wlc->pub->_ap ? BSSCFG_GENERIC_AP : BSSCFG_GENERIC_STA;
	if ((err = wlc_bsscfg_alloc_int(wlc, bsscfg, idx, &type, 0,
			&wlc->pub->cur_etheraddr)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_alloc_int() failed with %d\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

	if ((err = wlc_bsscfg_init(wlc, bsscfg)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_init() failed with %d\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

	return BCME_OK;

fail:
	if (bsscfg != NULL)
		wlc_bsscfg_free(wlc, bsscfg);
	return err;
}

/*
 * Find a bsscfg from matching cur_etheraddr, BSSID, SSID, or something unique.
 */

/** match wlcif */
wlc_bsscfg_t *
wlc_bsscfg_find_by_wlcif(wlc_info_t *wlc, wlc_if_t *wlcif)
{
	/* wlcif being NULL implies primary interface hence primary bsscfg */
	if (wlcif == NULL)
		return wlc_bsscfg_primary(wlc);

	switch (wlcif->type) {
	case WLC_IFTYPE_BSS:
		return wlcif->u.bsscfg;
#ifdef AP
	case WLC_IFTYPE_WDS:
		return SCB_BSSCFG(wlcif->u.scb);
#endif
	}

	WL_ERROR(("wl%d: Unknown wlcif %p type %d\n", wlc->pub->unit, wlcif, wlcif->type));
	return NULL;
}

/* special bsscfg types */
static bool
wlc_bsscfg_is_special(wlc_bsscfg_t *cfg)
{
	BCM_REFERENCE(cfg);
	return (0);
}

/* match cur_etheraddr */
wlc_bsscfg_t * BCMFASTPATH
wlc_bsscfg_find_by_hwaddr(wlc_info_t *wlc, struct ether_addr *hwaddr)
{
	int i;
	wlc_bsscfg_t *bsscfg;

	if (ETHER_ISNULLADDR(hwaddr) || ETHER_ISMULTI(hwaddr))
		return NULL;

	FOREACH_BSS(wlc, i, bsscfg) {
		if (eacmp(hwaddr->octet, bsscfg->cur_etheraddr.octet) == 0 &&
		    (wlc_bsscfg_is_special(bsscfg) == 0)) {
			return bsscfg;
		}
	}

	return NULL;
}

/** match BSSID */
wlc_bsscfg_t *
wlc_bsscfg_find_by_bssid(wlc_info_t *wlc, const struct ether_addr *bssid)
{
	int i;
	wlc_bsscfg_t *bsscfg;

	if (ETHER_ISNULLADDR(bssid) || ETHER_ISMULTI(bssid))
		return NULL;

	FOREACH_BSS(wlc, i, bsscfg) {
		if (eacmp(bssid->octet, bsscfg->BSSID.octet) == 0)
			return bsscfg;
	}

	return NULL;
}

/** match cur_etheraddr and BSSID */
wlc_bsscfg_t * BCMFASTPATH
wlc_bsscfg_find_by_hwaddr_bssid(wlc_info_t *wlc,
                                const struct ether_addr *hwaddr,
                                const struct ether_addr *bssid)
{
	int i;
	wlc_bsscfg_t *bsscfg;

	if (ETHER_ISMULTI(hwaddr) || ETHER_ISMULTI(bssid))
		return NULL;

	FOREACH_BSS(wlc, i, bsscfg) {
		if ((eacmp(hwaddr->octet, bsscfg->cur_etheraddr.octet) == 0) &&
		    (eacmp(bssid->octet, bsscfg->BSSID.octet) == 0))
			return bsscfg;
	}

	return NULL;
}

/** match target_BSSID */
wlc_bsscfg_t *
wlc_bsscfg_find_by_target_bssid(wlc_info_t *wlc, const struct ether_addr *bssid)
{
	int i;
	wlc_bsscfg_t *bsscfg;

	if (ETHER_ISNULLADDR(bssid) || ETHER_ISMULTI(bssid))
		return NULL;

	FOREACH_BSS(wlc, i, bsscfg) {
		if (!BSSCFG_STA(bsscfg))
			continue;
		if (eacmp(bssid->octet, bsscfg->target_bss->BSSID.octet) == 0)
			return bsscfg;
	}

	return NULL;
}

/** match SSID */
wlc_bsscfg_t *
wlc_bsscfg_find_by_ssid(wlc_info_t *wlc, uint8 *ssid, int ssid_len)
{
	int i;
	wlc_bsscfg_t *bsscfg;

	FOREACH_BSS(wlc, i, bsscfg) {
		if (ssid_len > 0 &&
		    ssid_len == bsscfg->SSID_len && bcmp(ssid, bsscfg->SSID, ssid_len) == 0)
			return bsscfg;
	}

	return NULL;
}

/** match ID */
wlc_bsscfg_t *
wlc_bsscfg_find_by_ID(wlc_info_t *wlc, uint16 id)
{
	int i;
	wlc_bsscfg_t *bsscfg;

	FOREACH_BSS(wlc, i, bsscfg) {
		if (bsscfg->ID == id)
			return bsscfg;
	}

	return NULL;
}

static void
wlc_bsscfg_bss_init(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	wlc_bss_info_t * bi = wlc->default_bss;

	bcopy((char*)bi, (char*)bsscfg->target_bss, sizeof(wlc_bss_info_t));
	bcopy((char*)bi, (char*)bsscfg->current_bss, sizeof(wlc_bss_info_t));
}

/* reset all configurations */
static int
wlc_bsscfg_init_int(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	wlc_bsscfg_type_t *type, uint flags, struct ether_addr *ea)
{
	brcm_ie_t *brcm_ie;

	ASSERT(bsscfg != NULL);
	ASSERT(ea != NULL);

	bsscfg->wlc = wlc;

	bsscfg->flags = flags;

	wlc_bsscfg_type_set(wlc, bsscfg, type);

	bcopy(ea, &bsscfg->cur_etheraddr, ETHER_ADDR_LEN);

	/* Match Wi-Fi default of true for aExcludeUnencrypted,
	 * instead of 802.11 default of false.
	 */
	bsscfg->wsec_restrict = TRUE;

	/* disable 802.1X authentication by default */
	bsscfg->eap_restrict = FALSE;

	/* disable WPA by default */
	bsscfg->WPA_auth = WPA_AUTH_DISABLED;
#ifdef ACKSUPR_MAC_FILTER
	/* acksupr initialization */
	bsscfg->acksupr_mac_filter = FALSE;
#endif /* ACKSUPR_MAC_FILTER */
	wlc_bsscfg_bss_init(wlc, bsscfg);

	/* initialize our proprietary elt */
	brcm_ie = (brcm_ie_t *)&bsscfg->brcm_ie[0];
	bzero((char*)brcm_ie, sizeof(brcm_ie_t));
	brcm_ie->id = DOT11_MNG_PROPR_ID;
	brcm_ie->len = BRCM_IE_LEN - TLV_HDR_LEN;
	bcopy(BRCM_OUI, &brcm_ie->oui[0], DOT11_OUI_LEN);
	brcm_ie->ver = BRCM_IE_VER;

	wlc_bss_update_brcm_ie(wlc, bsscfg);

	return BCME_OK;
}

/* allocate wlcif and create network interface if necessary */
static int
wlc_bsscfg_init_intf(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	wlc_txq_info_t *primary_queue = wlc->primary_queue;

	bsscfg->wlcif = wlc_wlcif_alloc(wlc, wlc->osh, WLC_IFTYPE_BSS, primary_queue);
	if (bsscfg->wlcif == NULL) {
		WL_ERROR(("wl%d: %s: failed to alloc wlcif\n",
		          wlc->pub->unit, __FUNCTION__));
		return BCME_NOMEM;
	}
	bsscfg->wlcif->u.bsscfg = bsscfg;

	/* create an OS interface for non-primary bsscfg */
	if (bsscfg == wlc_bsscfg_primary(wlc)) {
		/* primary interface has an implicit wlif which is assumed when
		 * the wlif pointer is NULL.
		 */
		bsscfg->wlcif->if_flags |= WLC_IF_LINKED;
	}
	else if (!BSSCFG_HAS_NOIF(bsscfg) && !BSSCFG_IS_RSDB_CLONE(bsscfg)) {
		uint idx = WLC_BSSCFG_IDX(bsscfg);
		bsscfg->wlcif->wlif = wl_add_if(wlc->wl, bsscfg->wlcif, idx, NULL);
		if (bsscfg->wlcif->wlif == NULL) {
			WL_ERROR(("wl%d: %s: wl_add_if failed for"
			          " index %d\n", wlc->pub->unit, __FUNCTION__, idx));
			return BCME_ERROR;
		}
		bsscfg->wlcif->if_flags |= WLC_IF_VIRTUAL;
		bsscfg->wlcif->if_flags |= WLC_IF_LINKED;
		wlc_if_event(wlc, WLC_E_IF_ADD, bsscfg->wlcif);
	}

	return BCME_OK;
}

static int
_wlc_bsscfg_bcmcscballoc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint band)
{
	ASSERT(bsscfg->bcmc_scb[band] == NULL);

	if ((bsscfg->bcmc_scb[band] =
	     wlc_bcmcscb_alloc(wlc, bsscfg, wlc->bandstate[band])) == NULL) {
		WL_ERROR(("wl%d: %s: fail to alloc scb for bsscfg %d\n",
		          wlc->pub->unit, __FUNCTION__, WLC_BSSCFG_IDX(bsscfg)));
		return BCME_NOMEM;
	}

	return  BCME_OK;
}

static int
wlc_bsscfg_bcmcscballoc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	int ret;

	if (!(bsscfg->flags & WLC_BSSCFG_NOBCMC)) {
		if (!IS_SINGLEBAND_5G(wlc->deviceid)) {
			if (_wlc_bsscfg_bcmcscballoc(wlc, bsscfg, BAND_2G_INDEX)) {
				WL_ERROR(("wl%d: %s: _wlc_bsscfg_bcmcscbinit failed for 2G\n",
					wlc->pub->unit, __FUNCTION__));
				ret = BCME_NOMEM;
				goto err;
			}
		}
		if (NBANDS(wlc) > 1 || IS_SINGLEBAND_5G(wlc->deviceid)) {
			if (_wlc_bsscfg_bcmcscballoc(wlc, bsscfg, BAND_5G_INDEX)) {
				WL_ERROR(("wl%d: %s: _wlc_bsscfg_bcmcscbinit failed for 5G\n",
					wlc->pub->unit, __FUNCTION__));
				ret = BCME_NOMEM;
				goto err;
			}
		}
	}

	return BCME_OK;

err:
	return ret;
}

#ifdef STA
/** Set/reset association parameters */
int
wlc_bsscfg_assoc_params_set(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	wl_join_assoc_params_t *assoc_params, int assoc_params_len)
{
	ASSERT(wlc != NULL);
	ASSERT(bsscfg != NULL);

	if (bsscfg->assoc_params != NULL) {
		MFREE(wlc->osh, bsscfg->assoc_params, bsscfg->assoc_params_len);
		bsscfg->assoc_params = NULL;
		bsscfg->assoc_params_len = 0;
	}
	if (assoc_params == NULL || assoc_params_len == 0)
		return BCME_OK;
	if ((bsscfg->assoc_params = MALLOCZ(wlc->osh, assoc_params_len)) == NULL) {
		WL_ERROR((WLC_BSS_MALLOC_ERR, WLCWLUNIT(wlc), WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
			assoc_params_len, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}
	bcopy(assoc_params, bsscfg->assoc_params, assoc_params_len);
	bsscfg->assoc_params_len = (uint16)assoc_params_len;

	return BCME_OK;
}

int
wlc_bsscfg_assoc_params_reset(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	if (bsscfg != NULL) {
		return (wlc_bsscfg_assoc_params_set(wlc, bsscfg, NULL, 0));
	}

	return BCME_ERROR;
}

/** Set/reset scan parameters */
int
wlc_bsscfg_scan_params_set(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	wl_join_scan_params_t *scan_params)
{
	ASSERT(bsscfg != NULL);

	if (scan_params == NULL) {
		if (bsscfg->scan_params != NULL) {
			MFREE(wlc->osh, bsscfg->scan_params, sizeof(wl_join_scan_params_t));
			bsscfg->scan_params = NULL;
		}
		return BCME_OK;
	}
	else if (bsscfg->scan_params != NULL ||
	         (bsscfg->scan_params = MALLOC(wlc->osh, sizeof(wl_join_scan_params_t))) != NULL) {
		bcopy(scan_params, bsscfg->scan_params, sizeof(wl_join_scan_params_t));
		return BCME_OK;
	}

	WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
		wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
	return BCME_NOMEM;
}

void
wlc_bsscfg_scan_params_reset(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	if (bsscfg != NULL)
		wlc_bsscfg_scan_params_set(wlc, bsscfg, NULL);
}
#endif /* STA */

void
wlc_bsscfg_SSID_set(wlc_bsscfg_t *bsscfg, uint8 *SSID, int len)
{
	ASSERT(bsscfg != NULL);
	ASSERT(len <= DOT11_MAX_SSID_LEN);

	if ((bsscfg->SSID_len = (uint8)len) > 0) {
		ASSERT(SSID != NULL);
		/* need to use memove here to handle overlapping copy */
		memmove(bsscfg->SSID, SSID, len);

		if (len < DOT11_MAX_SSID_LEN)
			bzero(&bsscfg->SSID[len], DOT11_MAX_SSID_LEN - len);
		return;
	}

	bzero(bsscfg->SSID, DOT11_MAX_SSID_LEN);
}

static void
wlc_bsscfg_update_ext_cap_len(wlc_bsscfg_t *bsscfg)
{
	int i;

	for (i = DOT11_EXTCAP_LEN_MAX - 1; i >= 0; i--) {
		if (bsscfg->ext_cap[i] != 0)
			break;
	}

	bsscfg->ext_cap_len = i + 1;
}

int
wlc_bsscfg_set_ext_cap(wlc_bsscfg_t *bsscfg, uint32 bit, bool val)
{
	bsscfg_module_t *bcmh = bsscfg->wlc->bcmh;

	if (((bit + 8) >> 3) > bcmh->ext_cap_max)
		return BCME_RANGE;

	if (val)
		setbit(bsscfg->ext_cap, bit);
	else
		clrbit(bsscfg->ext_cap, bit);

	wlc_bsscfg_update_ext_cap_len(bsscfg);

	return BCME_OK;
}

bool
wlc_bsscfg_test_ext_cap(wlc_bsscfg_t *bsscfg, uint32 bit)
{
	bsscfg_module_t *bcmh = bsscfg->wlc->bcmh;

	if (((bit + 8) >> 3) > bcmh->ext_cap_max)
		return FALSE;

	return isset(bsscfg->ext_cap, bit);
}

/** helper function for per-port code to call to query the "current" chanspec of a BSS */
chanspec_t
wlc_get_home_chanspec(wlc_bsscfg_t *cfg)
{
	ASSERT(cfg != NULL);

	if (cfg->associated)
		return cfg->current_bss->chanspec;

	return cfg->wlc->home_chanspec;
}

/** helper function for per-port code to call to query the "current" bss info of a BSS */
wlc_bss_info_t *
wlc_get_current_bss(wlc_bsscfg_t *cfg)
{
	ASSERT(cfg != NULL);

	return cfg->current_bss;
}

/**
 * Do multicast filtering
 * returns TRUE on success (reject/filter frame).
 */
bool
wlc_bsscfg_mcastfilter(wlc_bsscfg_t *cfg, struct ether_addr *da)
{
	unsigned int i;

	for (i = 0; i < cfg->nmulticast; i++) {
		if (bcmp((void*)da, (void*)&cfg->multicast[i],
			ETHER_ADDR_LEN) == 0)
			return FALSE;
	}

	return TRUE;
}

/* API usage :
* To ensure that only one instance of bsscfg exists for a given MACADDR
* To get existing bsscfg instance for the given unique MACADDR
* To be used typically in iface create/delete/query path
*/
wlc_bsscfg_t * BCMFASTPATH
wlc_bsscfg_find_by_unique_hwaddr(wlc_info_t *wlc, struct ether_addr *hwaddr)
{
	int idx;
	wlc_bsscfg_t *bsscfg;

	if (ETHER_ISNULLADDR(hwaddr) || ETHER_ISMULTI(hwaddr))
		return NULL;
	/* Don't use FOREACH_BSS (restritcted per WLC)
	* For RSDB, we _must_ walk across WLCs and ensure that the given
	* hwaddr(MACADDR) is not in use currently
	* So, we walk the (shared) bsscfg pointer table.
	*/
	for (idx = 0; idx < WLC_MAXBSSCFG; idx++) {
		bsscfg = wlc->bsscfg[idx];
		if (bsscfg) {
			if (eacmp(hwaddr->octet, bsscfg->cur_etheraddr.octet) == 0 &&
				(wlc_bsscfg_is_special(bsscfg) == 0)) {
				return bsscfg;
			}
		}
	}
	return NULL;
}

/* API usage :
* To ensure that only one instance of bsscfg exists for a given BSSID
* To get existing bsscfg instance for the given unique BSSID
*/

wlc_bsscfg_t * BCMFASTPATH
wlc_bsscfg_find_by_unique_bssid(wlc_info_t *wlc, struct ether_addr *bssid)
{
	int idx;
	wlc_bsscfg_t *bsscfg;

	if (ETHER_ISNULLADDR(bssid) || ETHER_ISMULTI(bssid))
		return NULL;
	/* Don't use FOREACH_BSS (restritcted per WLC)
	* For RSDB, we _must_ walk across WLCs and ensure that the given
	* bssid(MACADDR) is not in use currently
	* So, we walk the (shared) bsscfg pointer table.
	*/
	for (idx = 0; idx < WLC_MAXBSSCFG; idx++) {
		bsscfg = wlc->bsscfg[idx];
		if (bsscfg) {
			if (eacmp(bssid->octet, bsscfg->BSSID.octet) == 0 &&
				(wlc_bsscfg_is_special(bsscfg) == 0)) {
				return bsscfg;
			}
		}
	}
	return NULL;
}

#ifdef WLRSDB
/* Copies cubby CONFIG info from one cfg to another */
static int
wlc_bsscfg_cubby_copy(wlc_bsscfg_t *from, wlc_bsscfg_t *to)
{
	bsscfg_module_t *from_bcmh = from->wlc->bcmh;
	bsscfg_module_t *to_bcmh = to->wlc->bcmh;

	return wlc_cubby_copy(from_bcmh->cubby_info, from, to_bcmh->cubby_info, to);
}

int
wlc_bsscfg_configure_from_bsscfg(wlc_bsscfg_t *from_cfg, wlc_bsscfg_t *to_cfg)
{
	int err = BCME_OK;
	wlc_info_t *to_wlc = to_cfg->wlc;
	wlc_info_t *from_wlc = from_cfg->wlc;

	BCM_REFERENCE(from_wlc);

	/* Copy flags */
	to_cfg->flags = from_cfg->flags;
	to_cfg->_ap = from_cfg->_ap;
	to_cfg->BSS = from_cfg->BSS;
	/* copy type */
	to_cfg->type = from_cfg->type;

	to_cfg->enable = from_cfg->enable;
	/* copy SSID */
	wlc_bsscfg_SSID_set(to_cfg, from_cfg->SSID, from_cfg->SSID_len);

	/* copy/clone current bss info. */
	bcopy(from_cfg->current_bss->SSID, to_cfg->current_bss->SSID,
		from_cfg->current_bss->SSID_len);
	to_cfg->current_bss->SSID_len = from_cfg->current_bss->SSID_len;
	to_cfg->current_bss->flags = from_cfg->current_bss->flags;
	to_cfg->current_bss->bss_type = from_cfg->current_bss->bss_type;

	if (BSSCFG_AP(from_cfg)) {
		to_cfg->current_bss->chanspec = from_cfg->current_bss->chanspec;
		to_cfg->target_bss->chanspec = from_cfg->target_bss->chanspec;
	}

#ifdef STA
	wlc_bsscfg_assoc_params_set(to_wlc, to_cfg, from_cfg->assoc_params,
		from_cfg->assoc_params_len);

	wlc_bsscfg_scan_params_set(to_wlc, to_cfg, from_cfg->scan_params);
#endif /* STA */

	/* security */
	err = wlc_wsec(to_wlc, to_cfg, from_cfg->wsec);
	to_cfg->auth = from_cfg->auth;
	to_cfg->openshared = from_cfg->openshared;
	to_cfg->wsec_restrict = from_cfg->wsec_restrict;
	to_cfg->eap_restrict = from_cfg->eap_restrict;
	to_cfg->WPA_auth = from_cfg->WPA_auth;
	to_cfg->is_WPS_enrollee = from_cfg->is_WPS_enrollee;
	to_cfg->oper_mode = from_cfg->oper_mode;
	to_cfg->oper_mode_enabled = from_cfg->oper_mode_enabled;

#if defined(BCMSUP_PSK) && defined(BCMINTSUP)
	if (SUP_ENAB(from_wlc->pub)) {
		/* TODO: should the init be handled by cubby config get/set itself? */
		int sup_wpa = FALSE;
		err = wlc_iovar_op(from_wlc, "sup_wpa", NULL, 0, &sup_wpa,
			sizeof(sup_wpa), IOV_GET, from_cfg->wlcif);
		WL_WSEC(("sup_wpa(%d) from (%p > %p) \n", sup_wpa, from_cfg, to_cfg));
		err = wlc_iovar_op(to_wlc, "sup_wpa", NULL, 0, &sup_wpa,
			sizeof(sup_wpa), IOV_SET, to_cfg->wlcif);
	}
#endif 

	/* Now, copy/clone cubby_data */
	err = wlc_bsscfg_cubby_copy(from_cfg, to_cfg);

	return err;
}
#endif /* WLRSDB */


static bool
wlc_bsscfg_preserve(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bool preserve = FALSE;

	ASSERT(cfg);
#ifdef WOWL
	/* return false if WoWL is not enable or active */
	if (!WOWL_ENAB(wlc->pub) || !WOWL_ACTIVE(wlc->pub))
		preserve = FALSE;
#endif /* WOWL */
	return preserve;
}

/** Return the number of bsscfgs present now */
int
wlc_bss_count(wlc_info_t *wlc)
{
	uint16 i, ccount = 0;
	wlc_bsscfg_t *bsscfg;

	FOREACH_BSS(wlc, i, bsscfg) {
		ccount++;
	}

	return ccount;
}

#ifdef BCMULP
static uint
wlc_bsscfg_ulp_get_retention_size_cb(void *handle, ulp_ext_info_t *einfo)
{
	bsscfg_module_t *bcmh = (bsscfg_module_t *)handle;
	int cfg_cnt = wlc_bss_count(bcmh->wlc);

	ULP_DBG(("%s: cnt: %d, sz: %d\n", __FUNCTION__, cfg_cnt,
		sizeof(wlc_bsscfg_ulp_cache_t) +
		sizeof(wlc_bsscfg_ulp_cache_var_t) * cfg_cnt));
	return (sizeof(wlc_bsscfg_ulp_cache_t) +
		sizeof(wlc_bsscfg_ulp_cache_var_t) * cfg_cnt);
}

static int
wlc_bsscfg_ulp_enter_cb(void *handle, ulp_ext_info_t *einfo, uint8 *cache_data)
{
	bsscfg_module_t *bcmh = (bsscfg_module_t *)handle;
	int cfg_cnt = wlc_bss_count(bcmh->wlc);
	wlc_bsscfg_ulp_cache_t *cfg_ulp_cache =
		(wlc_bsscfg_ulp_cache_t *)cache_data;
	wlc_bsscfg_ulp_cache_var_t *cfg_ulp_cache_var =
		(wlc_bsscfg_ulp_cache_var_t *)(cfg_ulp_cache + 1);
	wlc_bsscfg_t *icfg;
	int i = 0;

	ULP_DBG(("%s: cd: %p\n", __FUNCTION__,
		cache_data));
	cfg_ulp_cache->cfg_cnt = cfg_cnt;

	ULP_DBG(("REC: cfg's:%d\n", cfg_cnt));

	/* write variable size fields */
	for (i = 0; i < cfg_cnt; i++) {
		icfg = wlc_bsscfg_find(bcmh->wlc, i, NULL);
		if (!icfg)
			continue;
		cfg_ulp_cache_var->bsscfg_idx = WLC_BSSCFG_IDX(icfg);
		cfg_ulp_cache_var->bsscfg_type = icfg->type;
		cfg_ulp_cache_var->bsscfg_flag = icfg->flags;
		cfg_ulp_cache_var->bsscfg_flag2 = icfg->flags2;
		cfg_ulp_cache_var->_ap = icfg->_ap;
		cfg_ulp_cache_var->wsec = icfg->wsec;
		cfg_ulp_cache_var->WPA_auth = icfg->WPA_auth;
		cfg_ulp_cache_var->flags = icfg->current_bss->flags;
		cfg_ulp_cache_var->dtim_programmed = icfg->dtim_programmed;
		memcpy(&cfg_ulp_cache_var->cur_ether, &icfg->cur_etheraddr,
			sizeof(struct ether_addr));
		memcpy(&cfg_ulp_cache_var->BSSID, &icfg->current_bss->BSSID,
			sizeof(struct ether_addr));
		ULP_DBG(("CAC:i:%d,cfg:%p,0x%p,ty:%x,fl:%x,fl2:%x\n", i, icfg,
			cfg_ulp_cache_var, cfg_ulp_cache_var->bsscfg_type,
			cfg_ulp_cache_var->bsscfg_flag,
			cfg_ulp_cache_var->bsscfg_flag2));
		cfg_ulp_cache_var++;
	}
#ifdef ULP_DBG_ON
	prhex("enter: cfgInfo", (uchar *)cfg_ulp_cache,
		wlc_bsscfg_ulp_get_retention_size_cb(bcmh, einfo));
#endif /* ULP_DBG_ON */

	return BCME_OK;
}

/*
 * bsscfg's allocated by wlc_attach(eg:primary) and
 * wlc_bsscfg_ulp_exit_cb will be initialized/restored here
 */
static int
wlc_bsscfg_ulp_recreate_cb(void *handle, ulp_ext_info_t *einfo,
	uint8 *cache_data, uint8 *p2_cache_data)
{
	bsscfg_module_t *bcmh = (bsscfg_module_t *)handle;
	wlc_info_t *wlc = bcmh->wlc;
	wlc_bsscfg_ulp_cache_t *cfg_ulp_cache =
		(wlc_bsscfg_ulp_cache_t *)cache_data;
	wlc_bsscfg_ulp_cache_var_t *cfg_ulp_cache_var =
		(wlc_bsscfg_ulp_cache_var_t *)(cfg_ulp_cache + 1);
	wlc_bsscfg_t *bsscfg = NULL;
	wlc_bsscfg_type_t cfg_type;
	int i = 0;
	ULP_DBG(("%s: REC:ptr:%p,cfg's:%d\n", __FUNCTION__, cache_data,
		cfg_ulp_cache->cfg_cnt));

	if (cfg_ulp_cache->cfg_cnt > 0) {
		cfg_ulp_cache_var = (wlc_bsscfg_ulp_cache_var_t *)(cfg_ulp_cache + 1);
		for (i = 0; i < cfg_ulp_cache->cfg_cnt; i++) {
			ULP_DBG(("REC:i:%d,0x%p,ty:%x,fl:%x,fl2:%x\n", i,
				cfg_ulp_cache_var, cfg_ulp_cache_var->bsscfg_type,
				cfg_ulp_cache_var->bsscfg_flag,
				cfg_ulp_cache_var->bsscfg_flag2));
			/* primary bsscfg idx */
			if (IS_PRIMARY_BSSCFG_IDX(i)) {
				bsscfg = wlc_bsscfg_primary(wlc);
				bsscfg->flags = cfg_ulp_cache_var->bsscfg_flag;
				bsscfg->type = cfg_ulp_cache_var->bsscfg_type;
			} else {
				cfg_type.type = cfg_ulp_cache_var->bsscfg_type;
				cfg_type.subtype =
					((cfg_ulp_cache_var->bsscfg_type == BSSCFG_TYPE_P2P) &&
						(cfg_ulp_cache_var->_ap)) ?
						BSSCFG_P2P_GO : BSSCFG_P2P_GC;
				bsscfg = wlc_bsscfg_alloc(wlc, i, &cfg_type,
					cfg_ulp_cache_var->bsscfg_flag | WLC_BSSCFG_ASSOC_RECR,
					&cfg_ulp_cache_var->cur_ether);
				if (!bsscfg) {
					WL_ERROR(("wl%d: cannot create bsscfg idx: %d\n",
						wlc->pub->unit, i));
					continue;
				}
				if (wlc_bsscfg_init(wlc, bsscfg) != BCME_OK) {
					WL_ERROR(("wl%d: cannot init bsscfg\n", wlc->pub->unit));
					break;
				}
			}
			bsscfg->flags2 = cfg_ulp_cache_var->bsscfg_flag2;
			bsscfg->wsec = cfg_ulp_cache_var->wsec;
			bsscfg->WPA_auth = cfg_ulp_cache_var->WPA_auth;
			bsscfg->current_bss->flags = cfg_ulp_cache_var->flags;
			bsscfg->dtim_programmed = cfg_ulp_cache_var->dtim_programmed;
			memcpy(&bsscfg->current_bss->BSSID, &cfg_ulp_cache_var->BSSID,
				sizeof(struct ether_addr));
			memcpy(&bsscfg->BSSID, &bsscfg->current_bss->BSSID,
				sizeof(struct ether_addr));
			memcpy(&bsscfg->cur_etheraddr, &cfg_ulp_cache_var->cur_ether,
				sizeof(struct ether_addr));

			cfg_ulp_cache_var++;
		}
	}

	return BCME_OK;
}
#endif /* BCMULP */

#ifdef BCMDBG_TXSTUCK
void
wlc_bsscfg_print_txstuck(wlc_info_t *wlc, struct bcmstrbuf *b)
{
#ifdef WL_BSSCFG_TX_SUPR
	int i;
	wlc_bsscfg_t *cfg;
	char eabuf[ETHER_ADDR_STR_LEN];
	bsscfg_module_t *bcmh = wlc->bcmh;

	FOREACH_BSS(wlc, i, cfg) {
		bcm_bprintf(b, "%s:\n", bcm_ether_ntoa(&cfg->BSSID, eabuf));

		/* invoke bsscfg cubby dump function */
		bcm_bprintf(b, "-------- bsscfg cubbies (%d) --------\n", WLC_BSSCFG_IDX(cfg));
		wlc_cubby_dump(bcmh->cubby_info, cfg, b);
	}
#endif /* WL_BSSCFG_TX_SUPR */
}
#endif /* BCMDBG_TXSTUCK */

/* This function checks if the BSS Info is capable
 * of MIMO or not. This is useful to identify if it is
 * required to JOIN in MIMO or RSDB mode.
 */
bool
wlc_bss_get_mimo_cap(wlc_bss_info_t *bi)
{
	bool mimo_cap = FALSE;
	if (bi->flags2 & WLC_BSS_VHT) {
		/* VHT capable peer */
		if ((VHT_MCS_SS_SUPPORTED(2, bi->vht_rxmcsmap))&&
			(VHT_MCS_SS_SUPPORTED(2, bi->vht_txmcsmap))) {
			mimo_cap = TRUE;
		}
	} else {
		/* HT Peer */
		/* mcs[1] tells the support for 2 spatial streams for Rx
		 * mcs[12] tells the support for 2 spatial streams for Tx
		 * which is populated if different from Rx MCS set.
		 * Refer Spec Section 8.4.2.58.4
		 */
		if (bi->rateset.mcs[1]) {
			/* Is Rx and Tx MCS set not equal ? */
			if (bi->rateset.mcs[12] & 0x2) {
				/* Check the Tx stream support */
				if (bi->rateset.mcs[12] & 0xC) {
					mimo_cap = TRUE;
				}
			}
			/* Tx MCS and Rx MCS
			 * are equel
			 */
			else {
				mimo_cap = TRUE;
			}
		}
	}
	return mimo_cap;
}

/* This function checks if the BSS Info is capable
 * of 80p80 or not. This is useful to identify if it is
 * required to JOIN in 80p80 or RSDB mode.
 */
bool
wlc_bss_get_80p80_cap(wlc_bss_info_t *bi)
{
	bool capa_80p80 = FALSE;
	if (CHSPEC_IS8080(bi->chanspec)) {
		capa_80p80 = TRUE;
		WL_ERROR(("%s, ssid:%s is 80p80 capable chanspec:%x\n",
			__FUNCTION__, bi->SSID, bi->chanspec));
	}
	return capa_80p80;
}
