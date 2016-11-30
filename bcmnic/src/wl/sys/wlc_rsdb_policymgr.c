/*
 * WLC RSDB POLICY MGR API Implementation
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_rsdb_policymgr.c 635537 2016-05-04 07:13:14Z $
 */

/**
 * @file
 * @brief
 * Real Simultaneous Dual Band operation
 */



#ifdef WLRSDB_POLICY_MGR
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_types.h>
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
#include <wlc_rsdb.h>
#include <wlc_rsdb_policymgr.h>
#include <wlc_assoc.h>
#ifdef WLMCNX
#include <wlc_mcnx.h>
#endif
#include <wlc_ulb.h>
#ifdef WL_NAN
#include <wlc_nan.h>
#endif
#include <bcmdevs.h>
#include <wlc_event.h>
#include <wlc_event_utils.h>

typedef struct rsdb_policymgr_common_info rsdb_policymgr_cmn_info_t;
enum {
	IOV_RSDB_CONFIG = 0,
	IOV_RSDB_POLICY_LAST
};

static void rsdb_policymgr_get_config(wlc_rsdb_policymgr_info_t *policy_info,
	rsdb_config_t *config);
static int rsdb_policymgr_set_config(wlc_rsdb_policymgr_info_t *policy_info, rsdb_config_t *config);
static void rsdb_policymgr_assoc_state_cb(void *ctx, bss_assoc_state_data_t *notif_data);
static void rsdb_policymgr_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data);
static wlc_rsdb_modes_t rsdb_policymgr_find_mode(wlc_rsdb_policymgr_info_t *policy_info,
	rsdb_opmode_t mode);
static int rsdb_policymgr_set_non_infra_mode(wlc_rsdb_policymgr_info_t *policy_info,
	uint  reason);
static wlc_infra_modes_t rsdb_find_infra_mode_from_config(rsdb_config_t *config,
	uint8 band_idx);
static int policymgr_do_mode_switch(wlc_info_t *wlc, rsdb_config_t *rsdbcfg,
	rsdb_modes_t to_policy, uint  modesw_reason);
static int wlc_rsdb_policymgr_mode_change(void *ctx, wlc_bsscfg_t **cfg, wlc_bss_info_t *bi);


struct wlc_rsdb_policymgr_info {
	wlc_info_t	*wlc;
	rsdb_policymgr_cmn_info_t	*cmn;
	bcm_notif_h	policy_state_notif_hdl;
	uint  modesw_reason;
};

struct rsdb_policymgr_common_info {
	rsdb_config_t *config;
	wlc_rsdb_policymgr_state_upd_data_t *post_notify_pending;
};

static const bcm_iovar_t rsdb_policymgr_iovars[] = {
	{"rsdb_config", IOV_RSDB_CONFIG, (0), 0, IOVT_BUFFER, sizeof(rsdb_config_t)},
	{NULL, 0, 0, 0, 0, 0}
};

/* Iovar functionality if any should be added in this function */
static int
wlc_rsdb_policymgr_doiovar(void *handle, uint32 actionid, void *p, uint plen,
	void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	wlc_rsdb_policymgr_info_t *policy_info = (wlc_rsdb_policymgr_info_t *)handle;
	int err = BCME_OK;

	switch (actionid) {

		case IOV_SVAL(IOV_RSDB_CONFIG): {
			rsdb_config_t *config = (rsdb_config_t *)p;
			int i;
			if (plen < (int)sizeof(rsdb_config_t)) {
				err = BCME_BADARG;
				break;
			}
			if (config->ver != WL_RSDB_CONFIG_VER) {
				err = BCME_VERSION;
				break;
			}
			if (config->non_infra_mode < WLC_SDB_MODE_NOSDB_MAIN ||
					config->non_infra_mode > WLC_SDB_MODE_SDB_AUTO) {
				WL_RSDB_PMGR_ERR(("wl%d.%d: wlc_rsdb_policymgr_doiovar: "
					"Invalid Non Infra mode\n",
					WLCWLUNIT(policy_info->wlc),
					WLC_BSSCFG_IDX(policy_info->wlc->cfg)));
				err = BCME_RANGE;
				break;
			}
			for (i = 0; i < MAX_BANDS; i++) {
				if (config->infra_mode[i] < WLC_SDB_MODE_NOSDB_MAIN ||
						config->infra_mode[i] > WLC_SDB_MODE_SDB_AUTO) {
					WL_RSDB_PMGR_ERR(("wl%d.%d: wlc_rsdb_policymgr_doiovar: "
						"Invalid Infra mode\n",
						WLCWLUNIT(policy_info->wlc),
						WLC_BSSCFG_IDX(policy_info->wlc->cfg)));
					err = BCME_RANGE;
					break;
				}
			}
			/* set the rsdb config/policy */
			err = rsdb_policymgr_set_config(policy_info, config);
			break;
		}
		case IOV_GVAL(IOV_RSDB_CONFIG): {
				rsdb_config_t *ptr = (rsdb_config_t *)a;
				memset(ptr, 0, sizeof(*ptr));
				rsdb_policymgr_get_config(policy_info, ptr);
				break;
		}

		default:
			err = BCME_UNSUPPORTED;
			break;
	}
	return err;
}

static void
rsdb_policymgr_get_config(wlc_rsdb_policymgr_info_t *policy_info, rsdb_config_t *config)
{
	wlc_info_t *wlc_iter = NULL;
	wlc_info_t *wlc_associated = NULL;
	int idx = 0;
	rsdb_config_t *rsdbcfg = policy_info->cmn->config;
	wlc_info_t *wlc =  WLC_RSDB_GET_PRIMARY_WLC(policy_info->wlc);
	wlc_cmn_info_t *wlc_cmn = wlc->cmn;

	memcpy(config, rsdbcfg, sizeof(*rsdbcfg));

	/* Overwrite current_mode based on infra association */
	FOREACH_WLC(wlc_cmn, idx, wlc_iter) {
		if (wlc_iter->cfg->associated) {
			wlc_associated = wlc_iter;
		}
	}
	if (wlc_associated) {
		uint8 bandidx = CHSPEC_WLCBANDUNIT(wlc_associated->cfg->current_bss->chanspec);
		config->current_mode = rsdbcfg->infra_mode[bandidx];
	} else {
		config->current_mode = rsdbcfg->non_infra_mode;
	}
}
static int
rsdb_policymgr_set_config(wlc_rsdb_policymgr_info_t *policy_info, rsdb_config_t *config)
{
	wlc_info_t *wlc_iter = NULL;
	wlc_info_t *wlc_associated = NULL;
	int idx = 0;
	int err = BCME_OK;
	rsdb_config_t *rsdbcfg = policy_info->cmn->config;
	wlc_rsdb_modes_t non_infra_target_mode, non_infra_current_mode;
	wlc_infra_modes_t infra_current_mode, infra_target_mode;
	wlc_info_t *wlc =  WLC_RSDB_GET_PRIMARY_WLC(policy_info->wlc);
	wlc_cmn_info_t *wlc_cmn = wlc->cmn;
	uint8 modesw_done = FALSE;

	/* Check if any scan/roam/assoc in progress */
	if (ANY_SCAN_IN_PROGRESS(wlc->scan) || AS_IN_PROGRESS(wlc) ||
		policy_info->cmn->post_notify_pending) {
		WL_RSDB_PMGR_DEBUG(("wl%d.%d: rsdb_policymgr_set_config: "
			"post notification pending due to scan in progress\n",
			WLCWLUNIT(wlc), WLC_BSSCFG_IDX(wlc->cfg)));
		err = BCME_BUSY;
		goto exit;
	}
	/* Overwrite current_mode based on infra association */
	FOREACH_WLC(wlc_cmn, idx, wlc_iter) {
		if (wlc_iter->cfg->associated) {
			wlc_associated = wlc_iter;
			break;
		}
	}

	/* Check if there is an active infra connection */
	if (wlc_associated) {
		/* Find the band of current association */
		uint8 bandidx = CHSPEC_WLCBANDUNIT(wlc_associated->cfg->current_bss->chanspec);
		infra_current_mode = rsdb_find_infra_mode_from_config(rsdbcfg, bandidx);
		infra_target_mode = rsdb_find_infra_mode_from_config(config, bandidx);
		if (infra_current_mode != infra_target_mode) {
			err = BCME_ASSOCIATED;
			goto exit;
		} else {
			modesw_done = FALSE;
		}
	 } else {
		non_infra_target_mode = rsdb_policymgr_find_mode(policy_info,
			config->non_infra_mode);
		non_infra_current_mode = rsdb_policymgr_find_mode(policy_info,
			rsdbcfg->non_infra_mode);
		if (non_infra_target_mode != non_infra_current_mode ||
			non_infra_target_mode != WLC_RSDB_CURR_MODE(wlc)) {
			modesw_done = TRUE;
			WL_RSDB_PMGR_DEBUG(("wl%d.%d: rsdb_policymgr_set_config: setting to "
				"non_infra_mode = %d, reason = %d\n", WLCWLUNIT(wlc),
				WLC_BSSCFG_IDX(wlc->cfg), config->non_infra_mode,
				WLC_E_REASON_HOST_DIRECT));
			err = policymgr_do_mode_switch(wlc, rsdbcfg,
					config->non_infra_mode, WLC_E_REASON_HOST_DIRECT);
		} else {
			modesw_done  = FALSE;
		}
	}
	if (err != BCME_OK) {
		goto exit;
	}
	if (modesw_done == FALSE) {
		wlc_bss_mac_event_immediate(wlc, wlc->cfg,
			WLC_E_SDB_TRANSITION, NULL,
			WLC_E_STATUS_SDB_COMPLETE,
			WLC_E_REASON_NO_MODE_CHANGE_NEEDED,
			0, NULL, 0);
	}
	memcpy(rsdbcfg, config, sizeof(*config));
	wlc->pub->cmn->_rsdb_policy = TRUE;
exit:
	return err;
}

/* RSDB policymgr attach Function to Register Module and reserve cubby Structure */
wlc_rsdb_policymgr_info_t *
BCMATTACHFN(wlc_rsdb_policymgr_attach)(wlc_info_t *wlc)
{
	wlc_rsdb_policymgr_info_t *policy_info;
	rsdb_policymgr_cmn_info_t *cmninfo = NULL;
	uint8 index;

	if (!(policy_info = (wlc_rsdb_policymgr_info_t *)MALLOCZ(wlc->osh,
		sizeof(wlc_rsdb_policymgr_info_t)))) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto err;
	}
	policy_info->wlc = wlc;

	cmninfo = (rsdb_policymgr_cmn_info_t *) obj_registry_get(wlc->objr,
		OBJR_RSDB_POLICY_CMN_INFO);
	if (cmninfo == NULL) {
		/* Object not found ! so alloc new object here and set the object */
		if (!(cmninfo = (rsdb_policymgr_cmn_info_t *)MALLOCZ(wlc->osh,
			sizeof(rsdb_policymgr_cmn_info_t)))) {
			WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto err;
		}
		if (!(cmninfo->config = (rsdb_config_t *)MALLOCZ(wlc->osh,
			sizeof(rsdb_config_t)))) {
			WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto err;
		}
		cmninfo->config->ver = WL_RSDB_CONFIG_VER;
		cmninfo->config->len = WL_RSDB_CONFIG_LEN;
		for (index = 0; index < MAX_BANDS; index++) {
			cmninfo->config->flags[index] = FALSE;
		}
		/* override default policy based on CHIP */
		if (!WLC_DUALMAC_RSDB(wlc->cmn)) {
			cmninfo->config->non_infra_mode = WLC_SDB_MODE_SDB_AUTO;
			cmninfo->config->infra_mode[BAND_2G_INDEX] = WLC_SDB_MODE_SDB_AUTO;
			cmninfo->config->infra_mode[BAND_5G_INDEX] =
					WLC_SDB_MODE_NOSDB_MAIN;
		} else {
			cmninfo->config->non_infra_mode = WLC_SDB_MODE_SDB_MAIN;
			for (index = 0; index < MAX_BANDS; index++) {
				cmninfo->config->infra_mode[index] = WLC_SDB_MODE_SDB_MAIN;
			}
		}

		wlc->pub->cmn->_rsdb_policy = TRUE;
		obj_registry_set(wlc->objr, OBJR_RSDB_POLICY_CMN_INFO, cmninfo);
	}
	/* Hit a reference for this object */
	(void) obj_registry_ref(wlc->objr, OBJR_RSDB_POLICY_CMN_INFO);
	policy_info->cmn = cmninfo;

	if (wlc_module_register(wlc->pub, rsdb_policymgr_iovars, "rsdb_policymgr", policy_info,
		wlc_rsdb_policymgr_doiovar, NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: policymgr wlc_module_register() failed\n", wlc->pub->unit));
		goto err;
	}

	if (wlc_bss_assoc_state_register(wlc, rsdb_policymgr_assoc_state_cb, policy_info)) {
		WL_ERROR(("wl%d: ASSOC module callback registration failed\n",
			wlc->pub->unit));
		goto err;
	}

	/* Add client callback to the scb state notification list */
	if ((wlc_scb_state_upd_register(wlc,
		(scb_state_upd_cb_t)rsdb_policymgr_scb_state_upd_cb,
		policy_info)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback %p\n",
			wlc->pub->unit, __FUNCTION__, rsdb_policymgr_scb_state_upd_cb));
		goto err;
	}
	/* create notification list for scb state change. */
	if (bcm_notif_create_list(wlc->notif, &policy_info->policy_state_notif_hdl) !=
		BCME_OK) {
		WL_ERROR(("wl%d: %s: scb bcm_notif_create_list() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto err;
	}
	wlc_rsdb_register_mode_change_cb(wlc->rsdbinfo,
		wlc_rsdb_policymgr_mode_change, policy_info);
	wlc_rsdb_register_get_wlcs_cb(wlc->rsdbinfo,
		rsdb_policymgr_get_non_infra_wlcs, policy_info);
	wlc_eventq_set_ind(wlc->eventq, WLC_E_SDB_TRANSITION, TRUE);
	return policy_info;
err:
	wlc_rsdb_policymgr_detach(policy_info);
	return NULL;
}

/* Detach rsdb policymgr Module from system */
void
BCMATTACHFN(wlc_rsdb_policymgr_detach)(wlc_rsdb_policymgr_info_t *policy_info)
{
	wlc_info_t *wlc;
	if (!policy_info)
		return;
	wlc = policy_info->wlc;
	if (policy_info->policy_state_notif_hdl != NULL)
		bcm_notif_delete_list(&policy_info->policy_state_notif_hdl);

	if (wlc_bss_assoc_state_unregister(wlc, rsdb_policymgr_assoc_state_cb,
		policy_info) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bss_assoc_state_unregister() failed\n",
			wlc->pub->unit, __FUNCTION__));
	}

	/* Remove client callback for the scb state notification list */
	wlc_scb_state_upd_unregister(wlc,
		(scb_state_upd_cb_t)rsdb_policymgr_scb_state_upd_cb,
		policy_info);

	WL_TRACE(("wl%d: wlc_modesw_detach\n", wlc->pub->unit));
	wlc_module_unregister(wlc->pub, "rsdb_policymgr", policy_info);

	if (obj_registry_unref(policy_info->wlc->objr, OBJR_RSDB_POLICY_CMN_INFO) == 0) {
		obj_registry_set(policy_info->wlc->objr, OBJR_RSDB_POLICY_CMN_INFO, NULL);
		if (policy_info->cmn != NULL) {
			if (policy_info->cmn->config != NULL) {
				MFREE(policy_info->wlc->osh, policy_info->cmn->config,
					sizeof(rsdb_config_t));
			}
			MFREE(policy_info->wlc->osh, policy_info->cmn,
				sizeof(rsdb_policymgr_cmn_info_t));
		}
	}
	if (policy_info != NULL)
		MFREE(wlc->osh, policy_info, sizeof(wlc_rsdb_policymgr_info_t));
}

int
rsdb_policymgr_set_non_infra_mode(wlc_rsdb_policymgr_info_t *policy_info,
	uint  reason)
{
	int err = BCME_OK;
	rsdb_config_t *rsdbcfg = policy_info->cmn->config;
	WL_RSDB_PMGR_DEBUG(("wl%d.%d: rsdb_policymgr_set_non_infra_mode: reason=%d\n",
	          WLCWLUNIT(policy_info->wlc), WLC_BSSCFG_IDX(policy_info->wlc->cfg), reason));
	err = policymgr_do_mode_switch(policy_info->wlc, policy_info->cmn->config,
		rsdbcfg->non_infra_mode, reason);
	return err;
}

static int
policymgr_do_mode_switch(wlc_info_t *wlc, rsdb_config_t *rsdbcfg,
	rsdb_modes_t to_policy, uint  modesw_reason)
{
	wlc_bsscfg_t *cfg = wlc->cfg;
	int err = BCME_OK;
	wlc_rsdb_modes_t to_mode;
	wlc_rsdb_policymgr_state_upd_data_t sdata;
	rsdb_policymgr_cmn_info_t *cmn =  wlc->rsdb_policy_info->cmn;

	ASSERT(!cmn->post_notify_pending);
	to_mode = rsdb_policymgr_find_mode(wlc->rsdb_policy_info,
			to_policy);
	if (to_mode != WLC_RSDB_CURR_MODE(wlc)) {

		WL_RSDB_PMGR_DEBUG(("wl%d.%d: policymgr_do_mode_switch: preforming mode "
			"switch for target_mode = %d, current_mode = %d, reason = %d\n",
			WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg), to_mode,
			WLC_RSDB_CURR_MODE(wlc), modesw_reason));
		/* Mode switch needed */
		/* Sent host mode switch started */
		wlc_bss_mac_event_immediate(wlc, cfg,
			WLC_E_SDB_TRANSITION, NULL,
			WLC_E_STATUS_SDB_START, modesw_reason, 0, NULL, 0);

		/* Notify pre mode-switch state */
		sdata.state = WLC_RSDB_POLICY_PRE_MODE_SWITCH;
		sdata.target_mode = to_mode;
		bcm_notif_signal(wlc->rsdb_policy_info->policy_state_notif_hdl, &sdata);

		wlc->rsdb_policy_info->modesw_reason = modesw_reason;
		WL_RSDB_PMGR_DEBUG(("wl%d.%d:policymgr_do_mode_switch: "
			"mode switch start: TIME = %u \n", WLCWLUNIT(wlc),
			WLC_BSSCFG_IDX(cfg), hnd_time()));
		switch (modesw_reason) {
			case WLC_E_REASON_HOST_DIRECT:
				/* switch mode : host */
				/* Intentioal fallthrough */
			case WLC_E_REASON_INFRA_DISASSOC:
				/* switch mode: non ifra */
				err =  wlc_rsdb_change_mode(wlc, to_mode);
				if (err) {
					return err;
				} else if (modesw_reason == WLC_E_REASON_HOST_DIRECT) {
					rsdbcfg->non_infra_mode = to_policy;
				}
				break;
			case WLC_E_REASON_INFRA_ASSOC:
				/* Intentioal fallthrough */
			case WLC_E_REASON_INFRA_ROAM:
				/* switch mode : pre assoc */
				wlc_assoc_change_state(cfg, AS_MODE_SWITCH_START);
				break;
			default:
				WL_RSDB_PMGR_ERR(("wl%d.%d: policymgr_do_mode_switch:"
					" Undefined modesw reason\n",
					WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg)));
				break;
		}
		WL_RSDB_PMGR_DEBUG(("wl%d.%d:policymgr_do_mode_switch: "
			"mode switch end: TIME = %u \n", WLCWLUNIT(wlc),
			WLC_BSSCFG_IDX(cfg), hnd_time()));
		sdata.state = WLC_RSDB_POLICY_POST_MODE_SWITCH;

		if (!AS_IN_PROGRESS(wlc)) {
			/* Notify post mode-switch state */
			bcm_notif_signal(wlc->rsdb_policy_info->policy_state_notif_hdl, &sdata);
		} else {
			/* Defer notfication for post mode switch if association is already
			  * in progress. This notification is deferred for AWDL module to
			  *serialize core-1 AWDL join if a join/roam is in progress.
			 */
			 WL_RSDB_PMGR_DEBUG(("wl%d.%d: policymgr_do_mode_switch: "
				"assoc in progress. set post notification pending\n",
				WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg)));
			cmn->post_notify_pending =
				MALLOCZ(wlc->pub->osh, sizeof(*(cmn->post_notify_pending)));
			memcpy(cmn->post_notify_pending, &sdata, sizeof(sdata));
		}
		wlc_bss_mac_event_immediate(wlc, cfg, WLC_E_SDB_TRANSITION, NULL,
			WLC_E_STATUS_SDB_COMPLETE, modesw_reason, 0, NULL, 0);
	}
	return err;
}

static int
wlc_rsdb_policymgr_mode_change(void *ctx, wlc_bsscfg_t **pcfg, wlc_bss_info_t *bi)
{
	wlc_bsscfg_t *cfg = *pcfg;
	wlc_info_t *wlc = cfg->wlc;
	rsdb_config_t *rsdbcfg = wlc->rsdb_policy_info->cmn->config;
	wlc_rsdb_modes_t to_mode;
	uint8 band_idx;
	uint  modesw_reason;
	int err;

	WL_RSDB_PMGR_DEBUG(("wl%d.%d: wlc_rsdb_policymgr_mode_change: "
		"trying mode switch for 2G(%d)/5G(%d)\n", WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg),
		CHSPEC_IS2G(bi->chanspec), CHSPEC_IS5G(bi->chanspec)));

	if (!WLC_DUALMAC_RSDB(wlc->cmn)) {
		/* parse the rsdb_config structure to decide mode */
		band_idx = CHSPEC_WLCBANDUNIT(bi->chanspec);
		to_mode = rsdb_policymgr_find_mode(wlc->rsdb_policy_info,
			rsdbcfg->infra_mode[band_idx]);

		/* Mode Switch */
		modesw_reason = (cfg->assoc->type == AS_ROAM) ?
			(WLC_E_REASON_INFRA_ROAM):
			(WLC_E_REASON_INFRA_ASSOC);

		err = policymgr_do_mode_switch(wlc, rsdbcfg, rsdbcfg->infra_mode[band_idx],
			modesw_reason);
		if (err != BCME_OK) {
			WL_RSDB_PMGR_ERR(("wl%d.%d: wlc_rsdb_policymgr_mode_change: error: "
				"modeswitch failed\n", WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg)));
			return err;
		}

		/* Increment the BI index back to original so
		* that it can be used while re-entering wlc_join
		* attempt function after completing mode switch.
		*/
		/* TODO: check if return from wlc_assoc_change_state
		* guarantees AS_MODE_SWITCH_COMPLETE otherwise
		* wait for mode switch to compelete before changing assoc
		* state to scan.
		*/
		if (to_mode == WLC_RSDB_CURR_MODE(wlc)) {
			WL_RSDB_PMGR_DEBUG(("wl%d.%d: wlc_rsdb_policymgr_mode_change: "
				"modeswitch succesful, Changing state to scan\n", WLCWLUNIT(wlc),
				WLC_BSSCFG_IDX(cfg)));
			wlc_assoc_change_state(cfg, AS_SCAN);
		} else {
			WL_RSDB_PMGR_ERR(("wl%d.%d: wlc_rsdb_policymgr_mode_change: "
				"unexpected! mode didn't switch\n", WLCWLUNIT(wlc),
				WLC_BSSCFG_IDX(cfg)));
			//wlc->as_info->join_targets_last++;
			return BCME_NOTREADY;
		}
	} else {
		/* Dual mac support not present */
	}
	return BCME_OK;
}

/* This API maps the SDB operation mode to target rsdb mode
* from_mode : input mode as defined in enum rsdb_modes_t
* to_mode : output mode i.e RSDB or MIMO
*/
static wlc_rsdb_modes_t
rsdb_policymgr_find_mode(wlc_rsdb_policymgr_info_t *policy_info, rsdb_opmode_t mode)
{
	wlc_rsdb_modes_t hw_mode = 0;
	if (mode == WLC_SDB_MODE_NOSDB_MAIN ||
		mode == WLC_SDB_MODE_NOSDB_AUX) {
		hw_mode = WLC_RSDB_MODE_2X2;
	} else if (mode == WLC_SDB_MODE_SDB_AUTO ||
		mode == WLC_SDB_MODE_SDB_MAIN ||
		mode == WLC_SDB_MODE_SDB_AUX) {
			hw_mode = WLC_RSDB_MODE_RSDB;
	}
	return hw_mode;
}

static wlc_infra_modes_t
rsdb_find_infra_mode_from_config(rsdb_config_t *config, uint8 band_idx)
{
	wlc_infra_modes_t target_mode = 0;
	rsdb_opmode_t infra_mode;
	infra_mode = config->infra_mode[band_idx];
	if ((infra_mode == WLC_SDB_MODE_NOSDB_MAIN) ||
		(infra_mode == WLC_SDB_MODE_SDB_MAIN)) {
			target_mode = WLC_INFRA_MODE_MAIN;
	} else if ((infra_mode == WLC_SDB_MODE_NOSDB_AUX) ||
		(infra_mode == WLC_SDB_MODE_SDB_AUX)) {
			target_mode = WLC_INFRA_MODE_AUX;
	} else if (infra_mode == WLC_SDB_MODE_SDB_AUTO) {
		target_mode = WLC_INFRA_MODE_AUTO;
	}
	return target_mode;
}

bool
wlc_rsdb_policymgr_is_sib_set(wlc_rsdb_policymgr_info_t *policy_info, int bandindex)
{
	rsdb_config_t *cfg = policy_info->cmn->config;
	return ((cfg->flags[bandindex] & ALLOW_SIB_PARALLEL_SCAN) ? TRUE: FALSE);
}

static void
rsdb_policymgr_assoc_state_cb(void *ctx, bss_assoc_state_data_t *notif_data)
{
	wlc_rsdb_policymgr_info_t *policy_info = (wlc_rsdb_policymgr_info_t *)ctx;
	wlc_bsscfg_t *bsscfg = notif_data->cfg;
	uint as_state = bsscfg->assoc->state;
	rsdb_policymgr_cmn_info_t *cmn =  policy_info->cmn;

	WL_RSDB_PMGR_DEBUG(("wl%d.%d: rsdb_policymgr_assoc_state_cb: notified for assoc_state=%d\n",
	          WLCWLUNIT(policy_info->wlc), WLC_BSSCFG_IDX(bsscfg), as_state));
	if (BSSCFG_INFRA_STA(bsscfg) && (bsscfg == wlc_bsscfg_primary(bsscfg->wlc))) {
		if (!(policy_info->wlc->cmn->rsdb_mode & WLC_RSDB_MODE_AUTO_MASK) &&
			WLRSDB_POLICY_MGR_ENAB(policy_info->wlc->pub)) {
			switch (notif_data->type) {
				case AS_NONE:
					if (as_state == AS_IDLE) {
						if (cmn->post_notify_pending) {
							WL_RSDB_PMGR_DEBUG(("wl%d.%d: "
							"rsdb_policymgr_assoc_state_cb: assoc_"
							"complete, notify and clear post notify"
							" pending\n",
							WLCWLUNIT(policy_info->wlc),
							WLC_BSSCFG_IDX(bsscfg)));
							bcm_notif_signal(
								policy_info->policy_state_notif_hdl,
								cmn->post_notify_pending);
							MFREE(policy_info->wlc->pub->osh,
							cmn->post_notify_pending,
							sizeof(*(cmn->post_notify_pending)));
							cmn->post_notify_pending = NULL;
						}
						if (!bsscfg->associated)
						{
							if (rsdb_policymgr_set_non_infra_mode(
								policy_info,
								WLC_E_REASON_INFRA_DISASSOC) !=
								BCME_OK) {
								WL_RSDB_PMGR_ERR(("wl%d.%d: "
								"rsdb_policymgr_assoc_state_cb:"
								"mode switch to infra failed\n",
								WLCWLUNIT(policy_info->wlc),
								WLC_BSSCFG_IDX(bsscfg)));
							}
						}
					}
					break;
			}
		}
	}
}

static void
rsdb_policymgr_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data)
{
	wlc_rsdb_policymgr_info_t *policy_info = (wlc_rsdb_policymgr_info_t *)ctx;

	struct scb *scb = notif_data->scb;
	wlc_bsscfg_t *bsscfg = scb->bsscfg;
	uint8 oldstate = notif_data->oldstate;
	rsdb_policymgr_cmn_info_t *cmn =  policy_info->cmn;

	/* Check if the state transited SCB is internal.
		 * In that case we have to do nothing, just return back
		 */
	if (SCB_INTERNAL(scb))
			return;
	/* Assoc in progress is handled from assoc state callback */
	if (AS_IN_PROGRESS(policy_info->wlc))
		return;

	/* SCB state callback takes care of deauth scenario */
	if (BSSCFG_STA(bsscfg)) {
		if (policy_info->cmn->post_notify_pending) {
			WL_RSDB_PMGR_DEBUG(("wl%d.%d: "
				"rsdb_policymgr_scb_state_upd_cb: "
				"assoc complete, notify and clear post notify pending\n",
				WLCWLUNIT(policy_info->wlc), WLC_BSSCFG_IDX(bsscfg)));
			bcm_notif_signal(policy_info->policy_state_notif_hdl,
				cmn->post_notify_pending);
			MFREE(policy_info->wlc->pub->osh, cmn->post_notify_pending,
				sizeof(*(cmn->post_notify_pending)));
			cmn->post_notify_pending = NULL;
		}
		if (!(policy_info->wlc->cmn->rsdb_mode & WLC_RSDB_MODE_AUTO_MASK) &&
		WLRSDB_POLICY_MGR_ENAB(policy_info->wlc->pub)) {
			if ((oldstate & ASSOCIATED) && !SCB_ASSOCIATED(scb)) {
				if (rsdb_policymgr_set_non_infra_mode(policy_info,
					WLC_E_REASON_INFRA_DISASSOC) != BCME_OK) {
					WL_RSDB_PMGR_ERR(("wl%d.%d: "
						"rsdb_policymgr_scb_state_upd_cb: "
						"mode switch to infra failed\n",
						WLCWLUNIT(policy_info->wlc),
						WLC_BSSCFG_IDX(bsscfg)));
				}
			}
		}
	}
}

int
BCMATTACHFN(wlc_rsdb_policymgr_state_upd_register)
	(wlc_rsdb_policymgr_info_t *policy_info, bcm_notif_client_callback fn, void *arg)
{
	bcm_notif_h hdl = policy_info->policy_state_notif_hdl;

	return bcm_notif_add_interest(hdl, fn, arg);
}
/* This API returns the wlc_5g and wlc_2g given one wlc, based on SDB policy */
int
rsdb_policymgr_get_non_infra_wlcs(void *ctx, wlc_info_t *wlc, wlc_info_t **wlc_2g,
	wlc_info_t **wlc_5g)
{
	wlc_cmn_info_t *wlc_cmn;
	int err = BCME_OK;
	wlc_cmn = wlc->cmn;
	rsdb_config_t *rsdbcfg = wlc->rsdb_policy_info->cmn->config;

	WL_INFORM(("wl%d:%s:%d\n", wlc->pub->unit, __FUNCTION__, __LINE__));
	wlc_cmn = wlc->cmn;

	switch (rsdbcfg->non_infra_mode) {
		case WLC_SDB_MODE_SDB_AUX:
			*wlc_2g = wlc_cmn->wlc[0];
			*wlc_5g = wlc_cmn->wlc[1];
			break;
		default:
			*wlc_5g = wlc_cmn->wlc[0];
			*wlc_2g = wlc_cmn->wlc[1];
			break;
	}
	return err;
}

int
BCMATTACHFN(wlc_rsdb_policymgr_state_upd_unregister)
	(wlc_rsdb_policymgr_info_t *policy_info, bcm_notif_client_callback fn, void *arg)
{
	bcm_notif_h hdl = policy_info->policy_state_notif_hdl;

	return bcm_notif_remove_interest(hdl, fn, arg);
}

wlc_infra_modes_t
wlc_rsdb_policymgr_find_infra_mode(wlc_rsdb_policymgr_info_t *policy_info,
	wlc_bss_info_t *bi) {
	uint8  band_idx;
	band_idx = CHSPEC_WLCBANDUNIT(bi->chanspec);
	rsdb_config_t *config = policy_info->cmn->config;
	return rsdb_find_infra_mode_from_config(config, band_idx);
}
#ifdef WLRSDB_MIMO_DEFAULT
int
rsdb_policymgr_set_noninfra_default_mode(wlc_rsdb_policymgr_info_t *policy_info)
{
	int err = BCME_OK;
	wlc_info_t *wlc = policy_info->wlc;
	rsdb_config_t *rsdbcfg = policy_info->cmn->config;

	wlc_eventq_set_ind(wlc->eventq, WLC_E_SDB_TRANSITION, FALSE);
	err = policymgr_do_mode_switch(policy_info->wlc, rsdbcfg,
		WLC_SDB_MODE_NOSDB_MAIN, WLC_E_REASON_HOST_DIRECT);
	wlc_eventq_set_ind(wlc->eventq, WLC_E_SDB_TRANSITION, TRUE);
	return err;
}
#endif /* WLRSDB_MIMO_DEFAULT */
#endif /* WLRSDB_POLICY_MGR */
