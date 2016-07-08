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
 * $Id: wlc_scanol.c 506293 2014-10-03 13:28:51Z $
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <d11.h>
#include <proto/802.1d.h>
#include <wlc_types.h>
#include <bcm_ol_msg.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#ifndef BCM_OL_DEV
#include <wlc_bsscfg.h>
#endif
#include <wlc.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_scandb.h>
#include <wlc_scan_priv.h>
#include <wlc_scan.h>
#include <wlc_scanol.h>
#include <wlc_keymgmt.h>
#include <wlc_macol.h>
#include <wlc_dngl_ol.h>
#include <wlc_wowlol.h>
#include <wl_export.h>
#include <wlc_tx.h>
#include <phy_api.h>

static void wlc_scanol_wake_host(scan_info_t *scan, uint32 wake_reason);
static void wlc_scanol_init_default(scan_info_t *scan, wlc_hw_info_t *wlc_hw);
static void wlc_scanol_list_free(scan_info_t *scan, wlc_bss_list_t *bss_list);
static int wlc_scanol_request(wlc_hw_info_t *wlc_hw);
static int wlc_scanol_scanstop(wlc_hw_info_t *wlc_hw);
static int wlc_scanol_config_upd(wlc_hw_info_t *wlc_hw, void *hdl);
static int wlc_scanol_bss_upd(wlc_hw_info_t *wlc_hw, void *hdl);
static int wlc_scanol_bssid_upd(wlc_hw_info_t *wlc_hw, struct ether_addr *addr);
static int wlc_scanol_quietvec_set(wlc_hw_info_t *wlc_hw, chanvec_t *quietvecs);
static int wlc_scanol_chanvec_set(wlc_hw_info_t *wlc_hw, uint bandtype, chanvec_t *chanvec);
static int wlc_scanol_chanspecs_set(wlc_hw_info_t *wlc_hw, wl_uint32_list_t *chanspecs);
static int wlc_scanol_country_upd(wlc_hw_info_t *wlc_hw, void *params, uint len);
static int wlc_scanol_params_set(scan_info_t *scan, void *params, uint len);
static bool wlc_scanol_pref_ssid_match(wlc_scan_info_t *wlc_scan_info, uint8 *ssid);
static int wlc_scanol_pfn_add(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len);
static int wlc_scanol_pfn_del(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len);
static int wlc_scanol_disable(scan_info_t *scan);
static void wlc_scanol_valid_chanspec_upd(scan_info_t *scan, chanvec_t *chanvec_2g,
	chanvec_t *chanvec_5g);
static void wlc_scanol_channel_init(wlc_hw_info_t *wlc_hw, scan_info_t *scan);

void
wlc_scanol_attach(wlc_hw_info_t *wlc_hw, void *wl, int *err)
{
	wlc_info_t *wlc = wlc_hw->wlc;
	scan_info_t *scan;
	osl_t *osh = wlc_hw->osh;
	uint unit = wlc_hw->unit;

	wlc_hw->scan_pub = wlc_scan_attach(wlc, wl, osh, unit);
	if (wlc_hw->scan_pub == NULL)
		*err = 99;

	scan = (scan_info_t *)wlc_hw->scan_pub->scan_priv;
	if ((scan->ol->scanoltimer = wl_init_timer(wlc->wl, wlc_scanol_idle_timeout,
		wlc, "scanol")) == NULL) {
		WL_ERROR(("wl%d: %s: wl_init_timer() failed.\n", unit, __FUNCTION__));
		*err = 99;
	}

	wlc_scanol_init_default(scan, wlc_hw);
}

void
wlc_scanol_detach(wlc_hw_info_t *wlc_hw)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;

	if (scan->ol)
		wlc_scanol_cleanup(scan, scan->osh);

	if (wlc_hw->scan_pub)
		wlc_scan_detach(wlc_scan_info);
}

void
wlc_scanol_idle_timeout(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t*)arg;
	wlc_hw_info_t *wlc_hw = wlc->hw;
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;

	if (!(ol->scan_enab && ol->scanoltimer && ol->scan_active))
		return;

	if (!SCAN_IN_PROGRESS(wlc_scan_info)) {
		wlc_scanol_request(wlc_hw);	/* start scan */
	}
}

void
wlc_scanol_watchdog(wlc_hw_info_t *wlc_hw)
{
	wlc_info_t *wlc = wlc_hw->wlc;
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;
	wlc_dngl_ol_info_t *wlc_dngl_ol = wlc->wlc_dngl_ol;

	if (ol->params.max_scan_cycles == 0 ||
	    (scan->ol->scan_cycle < ol->params.max_scan_cycles)) {
		if (ol->watchdog_fn)
		    (ol->watchdog_fn)((void *)scan);
	}

	if ((++ol->wd_count % 60) == 0)
		WL_INFORM(("%s: watchdog count %d\n", __FUNCTION__, ol->wd_count));

	if (!ol->scan_enab)
		return;

	/* don't start offload scan if associated */
	if (IS_ASSOCIATED(scan) || ol->wowl_pme_asserted == TRUE)
		return;

	if (ol->prefssid_found)
		wlc_scanol_wake_host(wlc_scan_info->scan_priv, WL_WOWL_SCANOL);

	/* start offload scan via zero timer CB */
	if (ol->scan_start_delay != 0) {
		ol->scan_start_delay--;
		return;
	}

	if (!ol->scan_active) {
		if (ol->params.max_scan_cycles)
			WL_ERROR(("%s: Run offload scan for %d cycles\n", __FUNCTION__,
				ol->params.max_scan_cycles));
		else
			WL_ERROR(("%s: Run offload scan for indefinitely\n", __FUNCTION__));
		ol->scan_active = TRUE;
		ol->scan_cycle = 0;
		wl_add_timer(wlc->wl, ol->scanoltimer, 0, FALSE);
		wlc_dngl_ol_event(wlc_dngl_ol, BCM_OL_E_SCAN_BEGIN, NULL);
	}
}

static void
wlc_scanol_event_wowl_start(wlc_dngl_ol_info_t *wlc_dngl_ol, wowl_cfg_t *wowl_cfg)
{
	int state = 0;

	if (wowl_cfg->wowl_enabled != TRUE)
		return;

	state = wowl_cfg->wowl_flags & WL_WOWL_SCANOL;
	if (state == 0)
		return;

	wlc_scanol_enable(wlc_dngl_ol->wlc_hw, state);
}

static void
wlc_scanol_ulp_upd(wlc_hw_info_t *wlc_hw)
{
	wlc_scan_info_t *wlc_scan_info;
	scan_info_t *scan;
	uint16 mhf1;

	ASSERT(wlc_hw);
	wlc_scan_info = wlc_hw->scan_pub;
	scan = (scan_info_t *)wlc_scan_info->scan_priv;
	ASSERT(wlc_bmac_mhf_get(wlc_hw, MHF1, WLC_BAND_AUTO) & MHF1_RXFIFO1);

	/* only enable ULP in non-associated state */
	if (scan->ol->associated || !scan->ol->ulp)
		return;

	mhf1 = wlc_bmac_mhf_get(wlc_hw, MHF1, WLC_BAND_AUTO);
	if ((mhf1 & MHF1_ULP) == 0) {
		/* wake ucode and set MHF1_ULP and HPS bit */
		wlc_bmac_set_wake_ctrl(wlc_hw, TRUE);
		wlc_bmac_mhf(wlc_hw, MHF1, MHF1_ULP, MHF1_ULP, WLC_BAND_ALL);
		wlc_bmac_mctrl(wlc_hw, MCTL_HPS, MCTL_HPS);
		/* put the ucode into sleep */
		wlc_bmac_set_wake_ctrl(wlc_hw, FALSE);
	}
}
void
wlc_scanol_event_handler(wlc_dngl_ol_info_t *wlc_dngl_ol, uint32 event, void *event_data)
{
	wlc_info_t *wlc;
	scan_info_t *scan;
	scanol_info_t *ol;
	int32 int_val;

	/* Initialize the locals */
	wlc = wlc_dngl_ol->wlc;
	scan = (scan_info_t *)wlc->hw->scan_pub->scan_priv;
	ol = (scanol_info_t *)scan->ol;

	switch (event) {
		case BCM_OL_E_WOWL_START:
			wlc_scanol_event_wowl_start(wlc_dngl_ol, event_data);
			break;
		case BCM_OL_E_WOWL_COMPLETE:
			if (ol->scan_enab) {
				wlc_scanol_valid_chanspec_upd(scan, &ol->valid_chanvec_2g,
					&ol->valid_chanvec_5g);
				wlc_scanol_ulp_upd(wlc_dngl_ol->wlc_hw);
				WL_ERROR(("%s: WOWL_COMPLETE Event, Delay scan for %dsec\n",
					__FUNCTION__, ol->scan_start_delay));
			}
			break;
		case BCM_OL_E_BCN_LOSS:
		case BCM_OL_E_DEAUTH:
		case BCM_OL_E_DISASSOC:
			/* Beacon Loss or Disass event cause Scan offload to start */
			wlc_phy_hold_upd(SCAN_GET_PI_PTR(scan), PHY_HOLD_FOR_NOT_ASSOC, TRUE);
			if (ol->params.flags & SCANOL_ENABLED)
				ol->scan_enab = TRUE;

			wlc_scanol_ulp_upd(wlc_dngl_ol->wlc_hw);
			if (ol->scan_enab && ol->associated) {
				wowl_host_info_t volatile *wowl_host_info;

				ol->associated = 0;
				wlc_scanol_valid_chanspec_upd(scan, &ol->valid_chanvec_2g,
					&ol->valid_chanvec_5g);
				wlc_scanol_channel_init(wlc_dngl_ol->wlc_hw, scan);
				WL_ERROR(("%s: BCN_LOSS/DISASSOC Event, Delay scan for %dsec\n",
					__FUNCTION__, ol->scan_start_delay));
				/* set LINKDOWN flag in case system is wake by user */
				wowl_host_info = &RXOESHARED(wlc_dngl_ol)->wowl_host_info;
				wowl_host_info->wake_reason |= WL_WOWL_LINKDOWN;
			}
			break;

		case BCM_OL_E_PME_ASSERTED:
			wl_del_timer(wlc->wl, ol->scanoltimer);

			/*
			 * Save state so we don't process additional packets before the
			 * host resumes to D0
			 */
			ol->wowl_pme_asserted = TRUE;
			break;

		case BCM_OL_E_RADIO_HW_DISABLED:

			int_val = *((int *)event_data);

			WL_ERROR(("%s: HW RADIO is tunrded %s\n",
				__FUNCTION__, int_val? "OFF":"ON"));

			if (int_val) {
				/* Handle Radio Disabled Case */

				/* If scan is in initial delay time, set scan_active
				 * so that watchdog does not do anything
				 */
				ol->scan_active = TRUE;

				/* If scan is in progress, stop it */
				wlc_scanol_scanstop(wlc_dngl_ol->wlc_hw);

				/* Kill the scanoltimer to stop scan */
				wl_del_timer(wlc->wl, ol->scanoltimer);

				/* Clear the wake control */
				wlc_bmac_set_wake_ctrl(wlc_dngl_ol->wlc_hw, FALSE);
			} else {
				/* Handle Radio Enabled Case */

				if (ol->scan_start_delay == 0) {
					ol->scan_cycle = 0;
					ol->scan_active = TRUE;
					wl_add_timer(wlc->wl, ol->scanoltimer, 0, FALSE);
				} else {
					/* Let watchdog start the scan when countdown done */
					ol->scan_active = FALSE;
				}
			}
			break;
	}
}

static void
wlc_scanol_wake_host(scan_info_t *scan, uint32 wake_reason)
{
	wlc_info_t *wlc = scan->wlc;
	wlc_dngl_ol_info_t *wlc_dngl_ol = wlc->wlc_dngl_ol;
	scanol_info_t *scanol = (scanol_info_t *)scan->ol;
	wowl_cfg_t *wowl_cfg = &wlc->wlc_dngl_ol->wowl_cfg;
	scan_wake_pkt_info_t scan_wake_pkt;
	wlc_bss_info_t *bi;

	/* Can we wake the host? */
	if ((wowl_cfg->wowl_enabled == FALSE) ||
	    ((wowl_cfg->wowl_flags & wake_reason) == 0) ||
	    (scanol->bi == NULL) ||
	    (scanol->wowl_pme_asserted == TRUE)) {
		WL_ERROR(("%s: Not returning host info\n", __FUNCTION__));

		return;
	}

	WL_ERROR(("%s: assert pme for wake reason %d\n", __FUNCTION__, 16));

	wl_del_timer(wlc->wl, scanol->scanoltimer);

	/*
	 * Update the shared memory block with the scan wake pakt info.
	 */

	/* We caused the PME assetion so return the scan wake packet info */
	WL_TRACE(("%s: Returning beacon info for %s\n",
		__FUNCTION__, scanol->bi->SSID));


	/* Return parts of the bi data that can be used by the host */
	bi = scanol->bi;
	scan_wake_pkt.ssid.SSID_len = bi->SSID_len;
	bcopy(bi->SSID, scan_wake_pkt.ssid.SSID, bi->SSID_len);

	bcopy(&bi->wpa, &scan_wake_pkt.wpa, sizeof(struct rsn_parms));
	bcopy(&bi->wpa2, &scan_wake_pkt.wpa2, sizeof(struct rsn_parms));

	scan_wake_pkt.capability = bi->capability;
	scan_wake_pkt.rssi       = bi->RSSI;
	scan_wake_pkt.chanspec   = bi->chanspec;

	/* Save state so we don't process additional packets before the host resumes to D0 */
	scanol->wowl_pme_asserted = TRUE;

	/* All done...wake up the host */
	wlc_wowl_ol_wake_host(wlc_dngl_ol->wowl_ol,
		&scan_wake_pkt, sizeof(scan_wake_pkt),
		(uchar*)scanol->bi->bcn_prb,
		scanol->bi->bcn_prb_len, wake_reason);
	wlc_scanol_disable(scan);
}

static void
wlc_scanol_valid_chanspec_upd(scan_info_t *scan, chanvec_t *chanvec_2g, chanvec_t *chanvec_5g)
{
	scanol_info_t *ol = scan->ol;
	uint i, nchans;

	bzero((uint8 *)ol->valid_chanspecs, sizeof(ol->valid_chanspecs));
	/* 2g/5g chanspecs (FCC) BW 20MHz */
	for (i = 0, nchans = 0; i < MAXCHANNEL && nchans < WL_NUMCHANSPECS; i++) {
		if (i <= CH_MAX_2G_CHANNEL) {
			if (isset(chanvec_2g, i))
				ol->valid_chanspecs[nchans++] = CH20MHZ_CHSPEC(i);
		} else {
			if (isset(chanvec_5g, i))
				ol->valid_chanspecs[nchans++] = CH20MHZ_CHSPEC(i);
		}
	}
	ASSERT(nchans);
	ol->nvalid_chanspecs = nchans;
}

static void
wlc_scanol_channel_init(wlc_hw_info_t *wlc_hw, scan_info_t *scan)
{
	bool suspend;
	/* check if Mac is suspend */
	suspend = !(R_REG(wlc_hw->osh, &wlc_hw->regs->maccontrol) & MCTL_EN_MAC);
	if (!suspend)
		wlc_scan_suspend_mac_and_wait(scan);
	phy_init((phy_info_t *)wlc_hw->band->pi, scan->ol->home_chanspec);
	if (!suspend)
		wlc_scan_enable_mac(scan);
}


static void
wlc_scanol_init_default(scan_info_t *scan, wlc_hw_info_t *wlc_hw)
{
	scanol_info_t *ol = scan->ol;
	const uint8 def_quietvecs[] = {
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11, 0x01, 0x00,
		0x00, 0x00, 0x10, 0x11, 0x11, 0x00, 0x10, 0x11, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	const uint8 def_chanvecs_2g[] = {
		0xfe, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	const uint8 def_chanvecs_5g[] = {
		0x00, 0x00, 0x00, 0x00, 0x10, 0x11, 0x11, 0x11, 0x01, 0x00,
		0x00, 0x00, 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x21, 0x22,
		0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	wlc_hw->radio_active = ON;

	ol->home_chanspec = 0x1001;

	scan->bsscfg->flags |= WLC_OL_BSSCFG_F_BSS;

	wlc_scanol_bssid_upd(wlc_hw, &wlc_hw->etheraddr);

	bcopy((uint8 *)def_quietvecs, &ol->quiet_chanvecs, sizeof(chanvec_t));
	bcopy((uint8 *)def_chanvecs_2g, &scan->ol->valid_chanvec_2g, sizeof(chanvec_t));
	bcopy((uint8 *)def_chanvecs_5g, &scan->ol->valid_chanvec_5g, sizeof(chanvec_t));
	memset(&ol->notx_chanvecs, 0xff, sizeof(chanvec_t));

	wlc_scanol_valid_chanspec_upd(scan, (chanvec_t *)def_chanvecs_2g,
		(chanvec_t *)def_chanvecs_5g);

	scan->scan_cmn->defaults.assoc_time = SCANOL_ASSOC_TIME;
	scan->scan_cmn->defaults.unassoc_time = SCANOL_UNASSOC_TIME;
	scan->scan_cmn->defaults.home_time = SCANOL_HOME_TIME;
	scan->scan_cmn->defaults.passive_time = SCANOL_PASSIVE_TIME;
	scan->scan_cmn->defaults.nprobes = SCANOL_NPROBES;
	scan->scan_cmn->defaults.passive = TRUE;

	if (scan->scan_cmn->defaults.passive == FALSE)
		ol->params.active_time = SCANOL_UNASSOC_TIME;
	else
		ol->params.active_time = 0;
	ol->params.passive_time = SCANOL_PASSIVE_TIME;
	ol->params.scan_cycle_idle_rest_time = SCANOL_CYCLE_IDLE_REST_TIME;
	ol->params.scan_cycle_idle_rest_multiplier = SCANOL_CYCLE_IDLE_REST_MULTIPLIER;
	ol->params.max_rest_time = SCANOL_MAX_REST_TIME;
	ol->params.max_scan_cycles = SCANOL_CYCLE_DEFAULT;
	ol->params.scan_start_delay = SCANOL_SCAN_START_DLY;
	ol->min_txpwr_thresh = wlc_phy_tssivisible_thresh(wlc_hw->band->pi);
	memset(ol->curpwr, ol->min_txpwr_thresh, sizeof(ol->curpwr));
	memset((uint8 *)&ol->sarlimit, WLC_TXPWR_MAX, sizeof(sar_limit_t));
	ol->rssithrsh2g = RSSI_THRESHOLD_2G_DEF;
	ol->rssithrsh5g = RSSI_THRESHOLD_5G_DEF;

	/* set NULL ssid */
	bzero(ol->ssidlist, sizeof(wlc_ssid_t) * MAX_SSID_CNT);
	ol->nssid = 1;
	ol->ulp = TRUE;
}

int
wlc_scanol_init(scan_info_t *scan, wlc_hw_info_t *hw, osl_t *osh)
{
	wlc_bss_list_t *scan_results = scan->scan_pub->scan_results;
	iscan_ignore_t *iscan_ignore_list = scan->scan_pub->iscan_ignore_list;

	scan->ol = (struct scanol_info *)MALLOC(osh, sizeof(struct scanol_info));
	if (scan->ol == NULL)
		return BCME_NOMEM;
	bzero((uint8 *)scan->ol, sizeof(struct scanol_info));

	scan->ol->hw = hw;
	scan->ol->txpwr = ppr_create(osh, WL_TX_BW_20);
	scan->ol->maxbss = MAXBSS;
	scan->ol->scan_enab = FALSE;
	scan->ol->assoc_in_progress = FALSE;
	scan->ol->home_chanspec = CH20MHZ_CHSPEC(1);

	if (scan->bsscfg == NULL) {
		scan->bsscfg = MALLOC(osh, sizeof(wlc_bsscfg_t));
		if (scan->bsscfg == NULL) {
			WL_ERROR(("%s: No memory for BSSCFG\n", __FUNCTION__));
			goto err;
		}
		bzero((uint8 *)scan->bsscfg, sizeof(wlc_bsscfg_t));
	}

	if (scan->bsscfg->pm == NULL) {
		scan->bsscfg->pm = MALLOC(osh, sizeof(wlc_pm_st_t));
		if (scan->bsscfg->pm == NULL) {
			WL_ERROR(("%s: No memory for BSSCFG->pm\n", __FUNCTION__));
			goto err;
		}
		bzero((uint8 *)scan->bsscfg->pm, sizeof(wlc_pm_st_t));
	}

	if (scan_results == NULL) {
		scan_results = MALLOC(osh, sizeof(wlc_bss_list_t));
		if (scan_results == NULL) {
			WL_ERROR(("%s: No memory for BSS List for scan_results\n", __FUNCTION__));
			goto err;
		}
		bzero((uint8 *)scan_results, sizeof(wlc_bss_list_t));
	}
	scan->scan_pub->scan_results = scan_results;

	scan_results = scan->scan_pub->scanol_results;
	if (scan_results == NULL) {
		scan_results = MALLOC(osh, sizeof(wlc_bss_list_t));
		if (scan_results == NULL) {
			WL_ERROR(("%s: No memory for BSS List for scan_results\n", __FUNCTION__));
			goto err;
		}
		bzero((uint8 *)scan_results, sizeof(wlc_bss_list_t));
	}
	scan->scan_pub->scanol_results = scan_results;

	iscan_ignore_list = scan->scan_pub->iscan_ignore_list;
	if (iscan_ignore_list == NULL) {
		iscan_ignore_list = MALLOC(osh, WLC_ISCAN_IGNORE_LIST_SZ);
		if (iscan_ignore_list == NULL) {
			WL_ERROR(("%s: No memory for BSS List for scan_results\n", __FUNCTION__));
			goto err;
		}
		bzero((uint8 *)iscan_ignore_list, WLC_ISCAN_IGNORE_LIST_SZ);
		scan->scan_pub->iscan_ignore_count = 0;
	}
	scan->scan_pub->iscan_ignore_list = iscan_ignore_list;

	if (wlc_macol_pso_shmem_upd(hw)) {
		WL_ERROR(("%s: issue Scan ABORT due to PSO not set!!!!!!\n", __FUNCTION__));
		ASSERT(0);
		return BCME_ERROR;
	}

	return BCME_OK;
err:
	if (scan->bsscfg->pm)
		MFREE(osh, scan->bsscfg->pm, sizeof(wlc_pm_st_t));

	if (scan->bsscfg)
		MFREE(osh, scan->bsscfg, sizeof(wlc_bsscfg_t));

	if (scan->ol)
		MFREE(osh, scan->ol, sizeof(struct scanol_info));

	if (scan->scan_pub->scan_results)
		MFREE(osh, scan->scan_pub->scan_results, sizeof(wlc_bss_list_t));

	if (scan->scan_pub->scanol_results)
		MFREE(osh, scan->scan_pub->scanol_results, sizeof(wlc_bss_list_t));

	if (scan->scan_pub->iscan_ignore_list)
		MFREE(osh, scan->scan_pub->iscan_ignore_list, WLC_ISCAN_IGNORE_LIST_SZ);

	return BCME_NOMEM;
}

void
wlc_scanol_cleanup(scan_info_t *scan, osl_t *osh)
{
	if (scan->ol->txpwr)
		ppr_delete(osh, scan->ol->txpwr);
	if (scan->bsscfg->pm)
		MFREE(osh, scan->bsscfg->pm, sizeof(wlc_pm_st_t));

	if (scan->bsscfg) {
		MFREE(osh, scan->bsscfg, sizeof(wlc_bsscfg_t));
		scan->bsscfg = 0;
	}

	if (scan->scan_pub->scan_results) {
		wlc_scanol_list_free(scan, scan->scan_pub->scan_results);
		MFREE(osh, scan->scan_pub->scan_results, sizeof(wlc_bss_list_t));
	}

	if (scan->scan_pub->scanol_results) {
		wlc_scanol_list_free(scan, scan->scan_pub->scanol_results);
		MFREE(osh, scan->scan_pub->scanol_results, sizeof(wlc_bss_list_t));
		scan->ol->bi = NULL;
	}

	if (scan->scan_pub->iscan_ignore_list) {
		scan->scan_pub->iscan_ignore_count = 0;
		MFREE(osh, scan->scan_pub->iscan_ignore_list, WLC_ISCAN_IGNORE_LIST_SZ);
	}

	if (scan->ol)
		MFREE(osh, scan->ol, sizeof(struct scanol_info));
	scan->ol = NULL;
}

static void
wlc_scanol_list_free(scan_info_t *scan, wlc_bss_list_t *bss_list)
{
	uint indx;
	wlc_bss_info_t *bi;

	/* inspect all BSS descriptor */
	for (indx = 0; indx < bss_list->count; indx++) {
		bi = bss_list->ptrs[indx];
		if (bi) {
			if (bi->bcn_prb) {
				MFREE(scan->osh, bi->bcn_prb, bi->bcn_prb_len);
			}
			MFREE(scan->osh, bi, sizeof(wlc_bss_info_t));
			bss_list->ptrs[indx] = NULL;
		}
	}
	bss_list->count = 0;
}

static void
wlc_scanol_list_xfer(wlc_bss_list_t *from, wlc_bss_list_t *to)
{
	uint size;

	ASSERT(to->count == 0);
	size = from->count * sizeof(wlc_bss_info_t*);
	bcopy((char*)from->ptrs, (char*)to->ptrs, size);
	bzero((char*)from->ptrs, size);

	to->count = from->count;
	from->count = 0;
	WL_INFORM(("%s: Scan Result Stored %d\n", __FUNCTION__, to->count));
}

static void
wlc_scanol_scanresult(wlc_hw_info_t *wlc_hw, wlc_scan_info_t *wlc_scan_info)
{
	wlc_bss_list_t *bss_list;
	wlc_bss_info_t *bi;
	uint indx;

	bss_list = wlc_scan_info->scanol_results;
	for (indx = 0; indx < bss_list->count; indx++) {
#if defined(BCMDBG_ERR)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif 
		bi = bss_list->ptrs[indx];
		WL_ERROR(("wl%d: BSS %s [%s] on channel %d (0x%x)\n",
			wlc_hw->unit, bcm_ether_ntoa(&bi->BSSID, eabuf),
			bi->SSID, CHSPEC_CHANNEL(bi->chanspec), bi->chanspec));
	}
}


void
wlc_scanol_complete(void *arg, int status, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = (wlc_info_t*)arg;
	wlc_hw_info_t *wlc_hw = wlc->hw;
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;
	wlc_dngl_ol_info_t *wlc_dngl_ol = wlc->wlc_dngl_ol;
	wlc_bss_list_t *bss_list;
	wlc_bss_info_t *bi;
	uint indx, scan_cycles, rest_time, multiplier;

	if (status == WLC_E_STATUS_ERROR)
		return;

	if (wlc_scan_info->scanol_results == NULL) {
		WL_ERROR(("%s: wlc_scan_info->scanol_results == NULL\n", __FUNCTION__));
		ASSERT(0);
		return;
	}
	if (wlc_scan_info->scan_results == NULL) {
		WL_ERROR(("%s: wlc_scan_info->scan_results == NULL\n", __FUNCTION__));
		ASSERT(0);
		return;
	}

	wlc_bmac_set_wake_ctrl(wlc_hw, FALSE);
	/* release old BSS information */
	wlc_scanol_list_free(wlc_scan_info->scan_priv, wlc_scan_info->scanol_results);

	/* copy scan results to custom_scan_results for reporting via ioctl */
	wlc_scanol_list_xfer(wlc_scan_info->scan_results, wlc_scan_info->scanol_results);

	WL_ERROR(("scan_cycles %d  scan_results %d\n",
		ol->scan_cycle, wlc_scan_info->scanol_results->count));

	bss_list = wlc_scan_info->scanol_results;
	for (indx = 0; indx < bss_list->count; indx++) {
		int16 rssithrsh = ol->rssithrsh5g;

		bi = bss_list->ptrs[indx];
		if (bi->chanspec && CHSPEC_IS2G(bi->chanspec))
			rssithrsh = ol->rssithrsh2g;
		WL_ERROR(("Detected SSID %s Chan %d RSSI %d\n",
			bi->SSID, CHSPEC_CHANNEL(bi->chanspec), bi->RSSI));
		if (wlc_scanol_pref_ssid_match(wlc_scan_info, bi->SSID) &&
		    bi->RSSI >= rssithrsh) {
			WL_ERROR(("*****Wake On Perferred SSID %s Chan %d; set wake\n",
				bi->SSID, CHSPEC_CHANNEL(bi->chanspec)));
			WL_ERROR(("%s: Preferred SSID found after %d cycles\n", __FUNCTION__,
				ol->scan_cycle));
			ol->prefssid_found = TRUE;
			ol->bi = bi;
			wlc_dngl_ol_event(wlc_dngl_ol, BCM_OL_E_PREFSSID_FOUND, NULL);
			return;
		}
	}

	/* max_scan_cycles = 0 => scan forever */
	if (ol->params.max_scan_cycles && ol->scan_cycle >= ol->params.max_scan_cycles) {
		/* max # of cycles reached, disabled offload scan */
		WL_ERROR(("%s: Scan completed %d cycles, disable scan offload\n", __FUNCTION__,
			ol->params.max_scan_cycles));
		ol->scan_enab = FALSE;
		wlc_dngl_ol_event(wlc_dngl_ol, BCM_OL_E_SCAN_END, NULL);
		return;
	} else if (ol->manual_scan) {
		ol->manual_scan = FALSE;
		wlc_bmac_mhf(wlc_hw, MHF1, MHF1_RXFIFO1, 0, WLC_BAND_ALL);
		return;
	}

	/* calculate what the idle time to use */
	scan_cycles = 1;
	if (ol->params.max_scan_cycles)
		/* cap the # of SC when multiplier applies */
		scan_cycles = MIN(ol->scan_cycle, SCANOL_MULTIPLIER_MAX);
	/* if the # of SC gets too large, multiplier can overrun the rest_time */
	multiplier = ol->params.scan_cycle_idle_rest_multiplier * scan_cycles;
	rest_time = ol->params.scan_cycle_idle_rest_time;
	if (multiplier) {
		rest_time = ol->params.scan_cycle_idle_rest_time * multiplier;
	}
	rest_time = MIN(rest_time, ol->params.max_rest_time);
	WL_ERROR(("%s: Start Scan cycle idle timer for %dms\n", __FUNCTION__, rest_time));
	wl_add_timer(wlc->wl, ol->scanoltimer, rest_time, FALSE);
}

static int
wlc_scanol_request(wlc_hw_info_t *wlc_hw)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;

	wlc_ssid_t *ssid, nullssid;
	uint nssid;
	const struct ether_addr *bssid = &ether_bcast;
	int scan_type = ol->scan_type;
	int nprobes = -1;
	int active_time = ol->params.active_time;
	int passive_time = ol->params.passive_time;
	int home_time = -1;
	bool scan_suppress_ssid = FALSE;
	chanspec_t chanspec_list[SCANOL_MAX_CHANNELS];
	int i, chanspec_count = MIN(ol->nchannels, SCANOL_MAX_CHANNELS);

	if (IS_ASSOCIATED(scan)) {
		WL_ERROR(("%s: STA Associated, Scan ABORT\n", __FUNCTION__));
		return BCME_ERROR;
	}

	if (scan->ol->manual_scan) {
		passive_time = 40;
		scan_type = DOT11_SCANTYPE_PASSIVE;
	}

	bzero((uint8 *)chanspec_list, sizeof(chanspec_t) * SCANOL_MAX_CHANNELS);
	for (i = 0; i < chanspec_count; i++)
		chanspec_list[i] = ol->chanspecs[i];

	if ((wlc_bmac_mhf_get(wlc_hw, MHF1, WLC_BAND_AUTO) & MHF1_RXFIFO1) == 0)
		wlc_bmac_mhf(wlc_hw, MHF1, MHF1_RXFIFO1, MHF1_RXFIFO1, WLC_BAND_ALL);

	wlc_scan_set_wake_ctrl(scan);

	bzero(&nullssid, sizeof(wlc_ssid_t));
	ssid = &nullssid;
	nssid = 1;
	if (((ol->params.flags & SCANOL_BCAST_SSID) == 0) && ol->nssid > 0) {
		nssid = ol->nssid;
		ssid = ol->ssidlist;
	}

	ol->scan_cycle++;
	return wlc_scan(wlc_scan_info,
		DOT11_BSSTYPE_INFRASTRUCTURE,	/* bsstype */
		bssid,
		nssid, ssid,
		scan_type,
		nprobes,
		active_time,
		passive_time,
		home_time,
		chanspec_list, chanspec_count, 0,
		TRUE,
		wlc_scanol_complete, wlc_hw->wlc,
		0,
		FALSE,
		scan_suppress_ssid,
		FALSE,
		FALSE,
		scan->bsscfg,
		SCAN_ENGINE_USAGE_NORM,
		NULL, NULL);
}

static int
wlc_scanol_scanstop(wlc_hw_info_t *wlc_hw)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;

	if (SCAN_IN_PROGRESS(wlc_scan_info)) {
		wlc_scan_abort(wlc_scan_info, WLC_E_STATUS_ABORT);
	}
	return BCME_OK;
}

static int
wlc_scanol_config_upd(wlc_hw_info_t *wlc_hw, void *hdl)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	wl_uint32_list_t *list = (wl_uint32_list_t *)hdl;
	scanol_info_t *ol = scan->ol;

	if (list->count != MAX_OL_SCAN_CONFIG)
		return BCME_BADARG;

	ol->_autocountry = (bool)list->element[0];
	ol->associated = (bool)list->element[1];
	ol->bandlocked = (bool)list->element[2];
	ol->PMpending = (bool)list->element[3];
	ol->bcnmisc_scan = (bool)list->element[4];
	ol->tx_suspended = (bool)list->element[5];
	ol->exptime_cnt = list->element[6];
	ol->home_chanspec = (chanspec_t)list->element[7];
	ol->rssithrsh2g = (int16)(list->element[8] >> 16);
	ol->rssithrsh5g = (int16)(list->element[8] & 0xffff);

	if (!ol->associated)
		wlc_bmac_sync_macstate(wlc_hw);

	return BCME_OK;
}

static int
wlc_scanol_bss_upd(wlc_hw_info_t *wlc_hw, void *hdl)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	wl_uint32_list_t *list = (wl_uint32_list_t *)hdl;
	wlc_bsscfg_t *cfg = scan->bsscfg;

	if (list->count != MAX_OL_SCAN_BSS)
		return BCME_BADARG;

	if (cfg && cfg->pm) {
		cfg->pm->PM = (bool)list->element[0];
		cfg->pm->PMblocked = (mbool)list->element[1];
		cfg->pm->WME_PM_blocked = (bool)list->element[2];
		if ((bool)list->element[3])
			cfg->flags |= WLC_OL_BSSCFG_F_BSS;
		else
			cfg->flags &= ~WLC_OL_BSSCFG_F_BSS;

		if ((bool)list->element[4])
			cfg->flags |= WLC_OL_BSSCFG_F_ASSOCIATED;
		else
			cfg->flags &= ~WLC_OL_BSSCFG_F_ASSOCIATED;
	}
	return BCME_OK;
}

static int
wlc_scanol_bssid_upd(wlc_hw_info_t *wlc_hw, struct ether_addr *addr)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;

	if (!scan->bsscfg)
		return BCME_NORESOURCE;

	bcopy(addr, &scan->bsscfg->BSSID, ETHER_ADDR_LEN);
	return BCME_OK;
}

static int
wlc_scanol_quietvec_set(wlc_hw_info_t *wlc_hw, chanvec_t *quietvecs)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;

	if (!quietvecs)
		return BCME_ERROR;

	bcopy(quietvecs, &scan->ol->quiet_chanvecs, sizeof(chanvec_t));
	scan->ol->quiet_chanvecs.vec[0] |= 1;	/* channel 0 is not valid */

	return BCME_OK;
}

static int
wlc_scanol_chanvec_set(wlc_hw_info_t *wlc_hw, uint bandtype, chanvec_t *chanvec)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;

	if (bandtype != WLC_BAND_2G && bandtype != WLC_BAND_5G) {
		WL_ERROR(("%s: Bad Band\n", __FUNCTION__));
		return BCME_BADBAND;
	}

	if (bandtype == WLC_BAND_2G)
		bcopy(chanvec, &scan->ol->valid_chanvec_2g, sizeof(chanvec_t));
	else
		bcopy(chanvec, &scan->ol->valid_chanvec_5g, sizeof(chanvec_t));

	return BCME_OK;
}

static int
wlc_scanol_chanspecs_set(wlc_hw_info_t *wlc_hw, wl_uint32_list_t *list)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	uint i;

	if (list) {
		bzero((uint8 *)scan->ol->valid_chanspecs, sizeof(scan->ol->valid_chanspecs));

		for (i = 0; i < list->count; i++)
			scan->ol->valid_chanspecs[i] = (chanspec_t)(list->element[i]);
	}

	return BCME_OK;
}

static int
wlc_scanol_country_upd(wlc_hw_info_t *wlc_hw, void *params, uint len)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;

	if (len > WLC_CNTRY_BUF_SZ)
		return BCME_BADARG;

	strncpy(scan->ol->autocountry_default, (char *)params, WLC_CNTRY_BUF_SZ - 1);
	return BCME_OK;
}

static int
wlc_scanol_params_set(scan_info_t *scan, void *params, uint len)
{
	scanol_info_t *ol = scan->ol;
	scanol_params_t *scan_params = (scanol_params_t *)params;
	uint nchan, nssid;
	uint8 *cptr;

	if (len < sizeof(scanol_params_t)) {
		WL_ERROR(("%s: len too short [%d < %d]\n", __FUNCTION__,
			len, sizeof(scanol_params_t)));
		return BCME_BADARG;
	}
	bzero(&ol->params, sizeof(scanol_params_t));
	ol->params.flags = scan_params->flags;

	/* first phase of scan offload only support Unassociated state */
	ol->params.active_time = scan_params->active_time;
	ol->params.passive_time = scan_params->passive_time;
	ol->params.idle_rest_time = scan_params->idle_rest_time;
	ol->params.idle_rest_time_multiplier = scan_params->idle_rest_time_multiplier;
	ol->params.active_rest_time = scan_params->active_rest_time;
	ol->params.active_rest_time_multiplier = scan_params->active_rest_time_multiplier;
	ol->params.scan_cycle_idle_rest_time = scan_params->scan_cycle_idle_rest_time;
	ol->params.scan_cycle_idle_rest_multiplier =
		scan_params->scan_cycle_idle_rest_multiplier;
	ol->params.scan_cycle_active_rest_time = scan_params->scan_cycle_active_rest_time;
	ol->params.scan_cycle_active_rest_multiplier =
		scan_params->scan_cycle_active_rest_multiplier;
	ol->params.max_rest_time = scan_params->max_rest_time;
	ol->params.max_scan_cycles = scan_params->max_scan_cycles;
	ol->params.nprobes = scan_params->nprobes;
	ol->params.scan_start_delay = scan_params->scan_start_delay;

	scan->scan_cmn->defaults.unassoc_time = ol->params.active_time;
	scan->scan_cmn->defaults.passive_time = ol->params.passive_time;
	scan->scan_cmn->defaults.nprobes = ol->params.nprobes;

	/* Do passive scan if active_time = 0 */
	if (ol->params.active_time == 0) {
		ol->scan_type = DOT11_SCANTYPE_PASSIVE;
		scan->scan_cmn->defaults.passive = TRUE;
	} else {
		ol->scan_type = DOT11_SCANTYPE_ACTIVE;
		scan->scan_cmn->defaults.passive = FALSE;
	}

	nchan = MIN(scan_params->nchannels, MAXCHANNEL);
	nssid = MIN(scan_params->ssid_count, MAX_SSID_CNT);

	if (nssid > 0) {
		bcopy(scan_params->ssidlist, ol->ssidlist, (sizeof(wlc_ssid_t) * nssid));
		bcopy(scan_params->ssidlist, ol->pref_ssidlist, (sizeof(wlc_ssid_t) * nssid));
		ol->nssid = nssid;
		ol->npref_ssid = nssid;
	} else {
		bzero(ol->ssidlist, sizeof(wlc_ssid_t) * MAX_SSID_CNT);
		bzero(ol->pref_ssidlist, sizeof(wlc_ssid_t) * MAX_SSID_CNT);
		ol->nssid = 1;
		ol->npref_ssid = 0;
	}

	cptr = (uint8 *)&scan_params->ssidlist[ol->nssid];
	if (nchan > 0) {
		bcopy(cptr, ol->chanspecs, (sizeof(chanspec_t) * nchan));
		ol->nchannels = nchan;
	}
	for (nchan = 0; nchan < ol->nchannels; nchan++)
		ol->chanspecs[nchan] = CH20MHZ_CHSPEC(CHSPEC_CHANNEL(ol->chanspecs[nchan]));
	return BCME_OK;
}

static int
wlc_scanol_disable(scan_info_t *scan)
{
	scanol_info_t *ol = scan->ol;
	wlc_hw_info_t *wlc_hw = ol->hw;

	ol->scan_enab = FALSE;
	wlc_bmac_set_wake_ctrl(wlc_hw, TRUE);
	wlc_bmac_mctrl(wlc_hw, MCTL_HPS, 0);
	wlc_bmac_mhf(wlc_hw, MHF1, MHF1_ULP, 0, WLC_BAND_ALL);
	wlc_bmac_set_wake_ctrl(wlc_hw, FALSE);
	return BCME_OK;
}

int
wlc_scanol_enable(wlc_hw_info_t *wlc_hw, int state)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;

	ol->scan_enab = state ? TRUE : FALSE;
	ol->prefssid_found = FALSE;
	ol->wowl_pme_asserted = FALSE;
	ol->scan_cycle = 0;
	ol->bi = NULL;
	ol->scan_active = FALSE;
	if (ol->params.scan_start_delay == -1)
		ol->scan_start_delay = SCANOL_SCAN_START_DLY;
	else
		ol->scan_start_delay = ol->params.scan_start_delay;
	ol->wd_count = 0;
	return BCME_OK;
}

static void
rxoe_scanol_config_upd(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	wl_uint32_list_t *list = (wl_uint32_list_t *)buf;
	uint count = list->count;
	uint size = sizeof(uint32) * (count + 1);

	WL_ERROR(("%s: Set offload parameters len %d size %d\n", __FUNCTION__, len, size));
	if (size <= len && list->count == MAX_OL_SCAN_CONFIG)
		wlc_scanol_config_upd(wlc_hw, list);
}

static void
rxoe_scanol_bss_upd(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	wl_uint32_list_t *list = (wl_uint32_list_t *)buf;
	uint count = list->count;
	uint size = sizeof(uint32) * (count + 1);

	WL_ERROR(("%s: Set BSS parameters len %d size %d\n", __FUNCTION__, len, size));
	if (size <= len && list->count == MAX_OL_SCAN_BSS)
		wlc_scanol_bss_upd(wlc_hw, list);
}

static void
rxoe_scanol_chanspecs_set(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	wl_uint32_list_t *list = (wl_uint32_list_t *)buf;
	uint count = list->count;
	uint size = sizeof(uint32) * (count + 1);

	WL_ERROR(("%s: Set Valid chanspecs len %d size %d\n", __FUNCTION__, len, size));
	if (count < WL_NUMCHANSPECS && size <= len)
		wlc_scanol_chanspecs_set(wlc_hw, list);
	else
		WL_ERROR(("%s: args too long count %d len %d\n", __FUNCTION__, count, len));
}

static void
rxoe_scanol_ssidlist_set(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len,
	wlc_ssid_t *ssidlist, uint *nssid)
{
	olmsg_ssids *msg = (olmsg_ssids *)buf;

	if (msg->ssid[0].SSID[0] == 0) {
		bzero(ssidlist, sizeof(wlc_ssid_t) * MAX_SSID_CNT);
		*nssid = 0;
	} else if (*nssid < MAX_SSID_CNT) {
		bcopy((uint8 *)&msg->ssid, (uint8 *)&ssidlist[*nssid], sizeof(wlc_ssid_t));
		*nssid += 1;
	}
}

static int
wlc_scanol_pfn_add(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;
	olmsg_pfn *msg = (olmsg_pfn *)buf;
	scanol_pfn_t *profile;
	uint i;

	if (ol->pfnlist.n_profiles >= SCANOL_MAX_PFN_PROFILES)
		return BCME_ERROR;
	for (i = 0; i < SCANOL_MAX_PFN_PROFILES; i++) {
		profile = &ol->pfnlist.profile[i];
		if (profile->ssid.SSID[0] != 0)
			continue;
		if (profile->cipher_type != 0)
			continue;
		if (profile->auth_type != 0)
			continue;
		break;
	}
	bcopy(msg, profile, sizeof(scanol_pfn_t));
	ol->pfnlist.n_profiles++;
	return BCME_OK;
}

static int
wlc_scanol_pfn_del(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;
	olmsg_pfn *msg = (olmsg_pfn *)buf;
	scanol_pfn_t *profile;
	uint i;

	for (i = 0; i < SCANOL_MAX_PFN_PROFILES; i++) {
		profile = &ol->pfnlist.profile[i];
		if (strcmp((char *)profile->ssid.SSID, (char *)msg->params.ssid.SSID) != 0)
			continue;
		if (profile->cipher_type != msg->params.cipher_type)
			continue;
		if (profile->auth_type != msg->params.auth_type)
			continue;
		break;
	}
	bzero(profile, sizeof(scanol_pfn_t));
	ol->pfnlist.n_profiles--;
	return BCME_OK;
}

static int
wlc_scanol_curpwr_set(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;
	uint i;
	bool passive = TRUE;

	memset(ol->curpwr, ol->min_txpwr_thresh, sizeof(ol->curpwr));
	memset(&ol->notx_chanvecs, 0xff, sizeof(chanvec_t));
	for (i = 0; i < len; i++) {
		ol->curpwr[i] = MAX(buf[i], ol->curpwr[i]);
		if (ol->curpwr[i] > ol->min_txpwr_thresh) {
			  clrbit(ol->notx_chanvecs.vec, i);
			  passive = FALSE;
		}
	}
	if (passive) {
		ol->scan_type = DOT11_SCANTYPE_PASSIVE;
		scan->scan_cmn->defaults.passive = TRUE;
	}
	return BCME_OK;
}

static int
wlc_scanol_sarlimit_set(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;

	bcopy(buf, (uint8 *)&ol->sarlimit, sizeof(sar_limit_t));
	return BCME_OK;
}


void
wlc_dngl_ol_scan_send_proc(wlc_hw_info_t *wlc_hw, uint8 *buf, uint len)
{
	olmsg_header *msg_hdr = (olmsg_header *)buf;
	wlc_scan_info_t *wlc_scan_info = wlc_hw->scan_pub;
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;
	olmsg_test *msg = (olmsg_test *)buf;
	int32 int_val = msg->data;

	switch (msg_hdr->type) {
		case BCM_OL_SCAN_ENAB:
			wlc_scanol_enable(wlc_hw, int_val);
			break;
		case BCM_OL_SCAN:
			if (ol->associated || SCAN_IN_PROGRESS(wlc_scan_info) || ol->manual_scan) {
				WL_ERROR(("Not Ready, Cancel Scan Request\n"));
				break;
			}
			ol->manual_scan = TRUE;
			if ((wlc_bmac_mhf_get(wlc_hw, MHF1, WLC_BAND_AUTO) & MHF1_RXFIFO1) == 0)
				wlc_bmac_mhf(wlc_hw, MHF1, MHF1_RXFIFO1, MHF1_RXFIFO1,
					WLC_BAND_ALL);
			wlc_scanol_request(wlc_hw);
			break;
		case BCM_OL_SCAN_RESULTS:
			wlc_scanol_scanresult(wlc_hw, wlc_scan_info);
			break;
		case BCM_OL_SCAN_CONFIG:
			rxoe_scanol_config_upd(wlc_hw, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_SCAN_BSS:
			rxoe_scanol_bss_upd(wlc_hw, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_SCAN_QUIET:
		{
			chanvec_t *quietvecs = (chanvec_t *)&msg_hdr[1];
			wlc_scanol_quietvec_set(wlc_hw, quietvecs);
			break;
		}
		case BCM_OL_SCAN_VALID2G:
		{
			chanvec_t *chanvec = (chanvec_t *)&msg_hdr[1];
			wlc_scanol_chanvec_set(wlc_hw, WLC_BAND_2G, chanvec);
			break;
		}
		case BCM_OL_SCAN_VALID5G:
		{
			chanvec_t *chanvec = (chanvec_t *)&msg_hdr[1];
			wlc_scanol_chanvec_set(wlc_hw, WLC_BAND_5G, chanvec);
			break;
		}
		case BCM_OL_SCAN_CHANSPECS:
			rxoe_scanol_chanspecs_set(wlc_hw,  (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_SCAN_BSSID:
		{
			olmsg_addr *msg = (olmsg_addr *)buf;

			if (scan->bsscfg)
				bcopy(&msg->addr, &scan->bsscfg->BSSID, ETHER_ADDR_LEN);
			break;
		}
		case BCM_OL_MACADDR:
		{
			olmsg_addr *msg = (olmsg_addr *)buf;

			bcopy(&msg->addr, &wlc_hw->etheraddr, ETHER_ADDR_LEN);
			bcopy(&msg->addr, &wlc_hw->orig_etheraddr, ETHER_ADDR_LEN);
			if (scan->bsscfg)
				bcopy(&msg->addr, &scan->bsscfg->cur_etheraddr, ETHER_ADDR_LEN);
			break;
		}
		case BCM_OL_SCAN_TXRXCHAIN:
		{
			uint8 txchain, rxchain;
			uint32 *data = (uint32 *)&msg_hdr[1];

			if (*data != 1)
				break;
			data++;
			txchain = (*data >> 8) & 0xff;
			rxchain = (*data) & 0xff;
			wlc_macol_chain_set(wlc_hw->ol, txchain, rxchain);
			break;
		}
		case BCM_OL_SCAN_COUNTRY:
			wlc_scanol_country_upd(wlc_hw, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_SCAN_PARAMS:
			wlc_scanol_params_set(scan, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_SSIDS:
			rxoe_scanol_ssidlist_set(wlc_hw, buf, len, ol->ssidlist, &ol->nssid);
			break;
		case BCM_OL_PREFSSIDS:
			rxoe_scanol_ssidlist_set(wlc_hw, buf, len, ol->pref_ssidlist,
				&ol->npref_ssid);
			break;
		case BCM_OL_PFN_LIST:
		{
			uint i;
			for (i = 0; i < SCANOL_MAX_PFN_PROFILES; i++) {
				if (ol->pfnlist.profile[i].ssid.SSID[0] == 0)
					continue;
				printf("ssid: %s cipher_type: %d auth_type: %d"
				       " channel: %d %d %d %d\n",
				       ol->pfnlist.profile[i].ssid.SSID,
				       ol->pfnlist.profile[i].cipher_type,
				       ol->pfnlist.profile[i].auth_type,
				       ol->pfnlist.profile[i].channels[0],
				       ol->pfnlist.profile[i].channels[1],
				       ol->pfnlist.profile[i].channels[2],
				       ol->pfnlist.profile[i].channels[3]);
			}
			break;
		}
		case BCM_OL_PFN_ADD:
			wlc_scanol_pfn_add(wlc_hw, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_PFN_DEL:
			wlc_scanol_pfn_del(wlc_hw, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_ULP:
			ol->ulp = (int_val != 0);
			break;
		case BCM_OL_CURPWR:
			wlc_scanol_curpwr_set(wlc_hw, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_SARLIMIT:
			wlc_scanol_sarlimit_set(wlc_hw, (uint8 *)&msg_hdr[1], msg_hdr->len);
			break;
		case BCM_OL_TXCORE:
		{
			uint32 *data = (uint32 *)&msg_hdr[1];

			if (*data++ != 6)
				break;
			wlc_hw->txcore[0][0] = (*data >> 8) & 0xff;
			wlc_hw->txcore[0][1] = *data++ & 0xff;
			wlc_hw->txcore[1][0] = (*data >> 8) & 0xff;
			wlc_hw->txcore[1][1] = *data++ & 0xff;
			wlc_hw->txcore[2][0] = (*data >> 8) & 0xff;
			wlc_hw->txcore[2][1] = *data++ & 0xff;
			wlc_hw->txcore[3][0] = (*data >> 8) & 0xff;
			wlc_hw->txcore[3][1] = *data++ & 0xff;
			wlc_hw->txcore[4][0] = (*data >> 8) & 0xff;
			wlc_hw->txcore[4][1] = *data++ & 0xff;
			wlc_hw->txcore[5][0] = (*data >> 8) & 0xff;
			wlc_hw->txcore[5][1] = *data & 0xff;
			wlc_macol_set_shmem_coremask(wlc_hw);
			break;
		}
		default:
			WL_ERROR(("INVALID message type:%d\n", msg_hdr->type));
			break;
	}
}

/* END of IOVAR functions and START of Support functions */
bool
wlc_scan_valid_chanspec_db(scan_info_t *scan, chanspec_t chanspec)
{
	uint i;
	for (i = 0; i < scan->ol->nvalid_chanspecs; i++) {
		if (scan->ol->valid_chanspecs[i] == chanspec)
			return TRUE;
	}
	return FALSE;
}

int
wlc_scan_stay_wake(scan_info_t *scan)
{
	return 1;
}

void
wlc_scan_assoc_state_upd(wlc_scan_info_t *wlc_scan_info, bool state)
{
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scan->ol->assoc_in_progress = state;
}

bool
wlc_scan_get_chanvec(scan_info_t *scan, const char* country_abbrev,
	int bandtype, chanvec_t *chanvec)
{
	if (bandtype != WLC_BAND_2G && bandtype != WLC_BAND_5G)
		return FALSE;
	if (bandtype == WLC_BAND_2G)
		bcopy(&scan->ol->valid_chanvec_2g, chanvec, sizeof(chanvec_t));
	else
		bcopy(&scan->ol->valid_chanvec_5g, chanvec, sizeof(chanvec_t));
	return TRUE;
}

bool
wlc_scan_valid_channel20_in_band(scan_info_t *scan, uint bu, uint val)
{
	chanvec_t *valid_chanvec;
	valid_chanvec = (bu == BAND_2G_INDEX) ?
		&scan->ol->valid_chanvec_2g : &scan->ol->valid_chanvec_5g;
	return ((val < MAXCHANNEL) && isset(valid_chanvec->vec, val));
}

bool
wlc_scan_quiet_chanspec(scan_info_t *scan, chanspec_t chanspec)
{
	uint ctrlch;

	if (scan->ol->scan_type == DOT11_SCANTYPE_PASSIVE)
		return FALSE;

	/* scan engine only deal with 20MHz channel, OK using CHSPEC_CHANNEL */
	ctrlch = CHSPEC_CHANNEL(chanspec);
	if (CHSPEC_IS40(chanspec)) {
		return (isset(scan->ol->quiet_chanvecs.vec, LOWER_20_SB(ctrlch)) ||
			isset(scan->ol->quiet_chanvecs.vec, UPPER_20_SB(ctrlch)));
	} else {
		return (isset(scan->ol->quiet_chanvecs.vec, ctrlch));
	}
	return FALSE;
}

void
wlc_scan_bss_list_free(scan_info_t *scan)
{
	wlc_scanol_list_free(scan, scan->scan_pub->scan_results);
}

void
wlc_scan_pm2_sleep_ret_timer_start(wlc_bsscfg_t *cfg)
{
	do {} while (0);
}

bool
wlc_scan_tx_suspended(scan_info_t *scan)
{
	uint fifo = TX_DATA_FIFO;
#ifdef BCM_OL_DEV
	fifo = TX_ATIM_FIFO;	/* for low mac, only fifo 5 allowed */
#endif
	return wlc_bmac_tx_fifo_suspended(scan->ol->hw, fifo);
}

void
_wlc_scan_tx_suspend(scan_info_t *scan)
{
	uint fifo = TX_DATA_FIFO;
#ifdef BCM_OL_DEV
	fifo = TX_ATIM_FIFO;	/* for low mac, only fifo 5 allowed */
#endif
	scan->ol->tx_suspended = TRUE;
	wlc_bmac_tx_fifo_suspend(scan->ol->hw, fifo);
}

void
_wlc_scan_pm_pending_complete(scan_info_t *scan)
{
	do {} while (0);
}

void
wlc_scan_set_pmstate(wlc_bsscfg_t *cfg, bool state)
{
	do {} while (0);
}

void
wlc_scan_skip_adjtsf(scan_info_t *scan, bool skip, wlc_bsscfg_t *cfg, uint32 user, int bands)
{
	do {} while (0);
}

void
wlc_scan_suspend_mac_and_wait(scan_info_t *scan)
{
	wlc_bmac_suspend_mac_and_wait(scan->ol->hw);
}

void
wlc_scan_enable_mac(scan_info_t *scan)
{
	wlc_bmac_enable_mac(scan->ol->hw);
}

void
wlc_scan_mhf(scan_info_t *scan, uint8 idx, uint16 mask, uint16 val, int bands)
{
	wlc_bmac_mhf(scan->ol->hw, idx, mask, val, bands);
}

void
wlc_scan_set_wake_ctrl(scan_info_t *scan)
{
	volatile uint32 mc;
	uint32 new_mc;
	bool wake;
	bool awake_before;

	if (!scan->ol->hw->up)
		return;

	mc = R_REG(scan->wlc->osh, &scan->ol->hw->regs->maccontrol);

	wake = SCAN_STAY_AWAKE(scan);

	new_mc = wake ? MCTL_WAKE : 0;

#if defined(WLMSG_PS)
	if ((mc & MCTL_WAKE) && !wake)
		WL_INFORM(("PS mode: clear WAKE (sleep if permitted)\n"));
	if (!(mc & MCTL_WAKE) && wake)
		WL_INFORM(("PS mode: set WAKE (stay awake)\n"));
#endif	

	wlc_bmac_mctrl(scan->ol->hw, MCTL_WAKE, new_mc);

	awake_before = !!(mc & MCTL_WAKE);

	if (wake && !awake_before)
		wlc_bmac_wait_for_wake(scan->ol->hw);
}

void
wlc_scan_ibss_enable(wlc_bsscfg_t *cfg)
{
	do {} while (0);
}

void
wlc_scan_ibss_disable_all(scan_info_t* scan)
{
	do {} while (0);
}

void
wlc_scan_bss_mac_event(scan_info_t* scan, wlc_bsscfg_t *cfg, uint msg,
	const struct ether_addr* addr, uint result, uint status, uint auth_type,
	void *data, int datalen)
{
	do {} while (0);
}

void
wlc_scan_excursion_start(scan_info_t *scan)
{
	do {} while (0);
}

void
wlc_scan_excursion_end(scan_info_t *scan)
{
	do {} while (0);
}

void
wlc_scan_set_chanspec(scan_info_t *scan, chanspec_t chanspec)
{
	scanol_info_t *ol = scan->ol;
	wlc_hw_info_t *wlc_hw = ol->hw;
	uint32 sar = 0;
	uint band, chan;

	if (chanspec == wlc_hw->chanspec)
		return;

	wlc_bmac_set_chanspec(wlc_hw, chanspec,
		(wlc_scan_quiet_chanspec(scan, chanspec) != 0),
		scan->ol->txpwr);

	chan = CHSPEC_CHANNEL(chanspec);
	if (chan < CHANNEL_5G_MID_START)
		band = 0;
	else if (chan >= CHANNEL_5G_MID_START && chan < CHANNEL_5G_HIGH_START)
		band = 1;
	else if (chan >= CHANNEL_5G_HIGH_START && chan < CHANNEL_5G_UPPER_START)
		band = 2;
	else
		band = 3;

	if (BAND_5G(wlc_hw->band->bandtype))
		memcpy((uint8 *)sar, (uint8 *)ol->sarlimit.band5g[band], WLC_TXCORE_MAX);
	else
		memcpy((uint8 *)sar, (uint8 *)ol->sarlimit.band2g, WLC_TXCORE_MAX);

	/* for now just write sar and curpwr to phyreg directly */
	wlc_phy_sarlimit_write(wlc_hw->band->pi, sar);
	wlc_phy_curpwr_write(wlc_hw->band->pi, ol->curpwr[chan]);
}

void
wlc_scan_validate_bcn_phytxctl(scan_info_t *scan, wlc_bsscfg_t *cfg)
{
	do {} while (0);
}

void
wlc_scan_mac_bcn_promisc(scan_info_t *scan)
{
	if (scan->ol->bcnmisc_scan)
		wlc_bmac_mctrl(scan->ol->hw, MCTL_BCNS_PROMISC, MCTL_BCNS_PROMISC);
	else
		wlc_bmac_mctrl(scan->ol->hw, MCTL_BCNS_PROMISC, 0);
}

void
wlc_scan_ap_mute(scan_info_t *scan, bool mute, wlc_bsscfg_t *cfg, uint32 user)
{
	do {} while (0);
}

void
wlc_scan_tx_resume(scan_info_t *scan)
{
	wlc_hw_info_t *wlc_hw = scan->ol->hw;
	scan->ol->tx_suspended = FALSE;
	uint fifo = TX_DATA_FIFO;
#ifdef BCM_OL_DEV
	fifo = TX_ATIM_FIFO;	/* for low mac, only fifo 5 allowed */
#endif

	if (!wlc_scan_quiet_chanspec(scan, SCAN_BAND_PI_RADIO_CHANSPEC(scan))) {
		wlc_bmac_tx_fifo_resume(wlc_hw, fifo);
	}
}

void
wlc_scan_send_q(scan_info_t *scan)
{
/*	wlc_send_active_q(scan->wlc); */
}

static void *
wlc_scan_frame_get_mgmt(scan_info_t *scan, uint16 fc, const struct ether_addr *da,
	const struct ether_addr *sa, const struct ether_addr *bssid, uint body_len,
	uint8 **pbody)
{
	uint len;
	void *p = NULL;
	osl_t *osh;
	struct dot11_management_header *hdr;

	osh = scan->osh;

	len = DOT11_MGMT_HDR_LEN + body_len;

	if ((p = PKTGET(osh, (TXOFF + len), TRUE)) == NULL) {
		WL_ERROR(("wl%d: wlc_frame_get_mgmt: pktget error for len %d fc %x\n",
		           scan->unit, ((int)TXOFF + len), fc));
		return NULL;
	}

	ASSERT(ISALIGNED((uintptr)PKTDATA(osh, p), sizeof(uint32)));

	/* reserve TXOFF bytes of headroom */
	PKTPULL(osh, p, TXOFF);
	PKTSETLEN(osh, p, len);

	/* construct a management frame */
	hdr = (struct dot11_management_header *)PKTDATA(osh, p);
	hdr->fc = htol16(fc);
	hdr->durid = 0;
	bcopy((const char*)da, (char*)&hdr->da, ETHER_ADDR_LEN);
	bcopy((const char*)sa, (char*)&hdr->sa, ETHER_ADDR_LEN);
	bcopy((const char*)bssid, (char*)&hdr->bssid, ETHER_ADDR_LEN);
	hdr->seq = 0;

	*pbody = (uint8*)&hdr[1];

	/* Set Prio for MGMT packets */
	PKTSETPRIO(p, MAXPRIO);

	return (p);
}

static void
wlc_scan_get_sup_ext_rates(scan_info_t *scan, wlc_rateset_t *sup_rates,
	wlc_rateset_t *ext_rates)
{
	const uint8 ofdm_rates[WL_NUM_RATES_OFDM] =
		{0x8c, 0x12, 0x98, 0x24, 0xB0, 0x48, 0x60, 0x6c};
	const uint8 cck_rates[WL_NUM_RATES_CCK] =
		{0x82, 0x84, 0x8b, 0x96};

	bzero((uint8 *)sup_rates, sizeof(wlc_rateset_t));
	bzero((uint8 *)ext_rates, sizeof(wlc_rateset_t));
	if (BAND_2G(scan->ol->hw->band->bandtype)) {
		bcopy(cck_rates, sup_rates->rates, WL_NUM_RATES_CCK);
		sup_rates->count = WL_NUM_RATES_CCK;
		bcopy(ofdm_rates, ext_rates->rates, WL_NUM_RATES_OFDM);
		ext_rates->count = WL_NUM_RATES_OFDM;
	} else {
		bcopy(ofdm_rates, sup_rates->rates, WL_NUM_RATES_OFDM);
		sup_rates->count = WL_NUM_RATES_OFDM;
	}
}

static uint8 *
wlc_scan_write_info_elt(uint8 *cp, int id, int len, const void* data)
{
	ASSERT(len < 256);
	ASSERT(len >= 0);

	cp[0] = (uint8)id;
	cp[1] = (uint8)len;
	if (len > 0)
		bcopy(data, (char*)cp+2, len);

	return (cp + 2 + len);
}

void
_wlc_scan_sendprobe(scan_info_t *scan, wlc_bsscfg_t *cfg,
	const uint8 ssid[], int ssid_len,
	const struct ether_addr *da, const struct ether_addr *bssid,
	ratespec_t rspec_override, uint8 *extra_ie, int extra_ie_len)
{
	void *p;
	uint8 *pbody, *bufend, *tmp_pbody;
	wlc_rateset_t sup_rates, ext_rates;
	int body_len;
	wlc_hw_info_t *wlc_hw = scan->ol->hw;
	uint frameid;
	uint fifo = TX_AC_BE_FIFO;
#ifdef BCM_OL_DEV
	fifo = TX_ATIM_FIFO;	/* for low mac, only fifo 5 allowed */
#endif
	if (rspec_override == 0) {
		rspec_override = (RSPEC_BW_20MHZ | RSPEC_OVERRIDE_RATE);
		if (CHSPEC_IS2G(wlc_hw->chanspec))
			rspec_override |= WLC_RATE_1M;
		else
			rspec_override |= WLC_RATE_6M;
	}

	wlc_scan_get_sup_ext_rates(scan, &sup_rates, &ext_rates);

	/*
	 * get a packet - probe req has the following contents:
	 * InfoElt   SSID
	 * InfoElt   Supported rates
	 * InfoElt   Extended Supported rates (11g)
	 * InfoElt   DS Params
	 */

	body_len = TLV_HDR_LEN + ssid_len;
	body_len += TLV_HDR_LEN + sup_rates.count;

	if (ext_rates.count > 0)
		body_len += TLV_HDR_LEN + ext_rates.count;

	if (CHSPEC_IS2G(wlc_hw->chanspec))
		body_len += TLV_HDR_LEN + 1; /* 1 byte to specify chanspec */

	if ((p = wlc_scan_frame_get_mgmt(scan, FC_PROBE_REQ, da,
	           &cfg->cur_etheraddr, bssid, body_len, &pbody)) == NULL) {
		return;
	}

	/* save end of buffer location */
	bufend = pbody + body_len;

	tmp_pbody = pbody - sizeof(struct dot11_header) + ETHER_ADDR_LEN;
	body_len += sizeof(struct dot11_header) - ETHER_ADDR_LEN;
	/* fill out the probe request body */

	/* SSID */
	pbody = wlc_scan_write_info_elt(pbody, DOT11_MNG_SSID_ID, ssid_len, ssid);

	/* Supported Rates */
	pbody = wlc_scan_write_info_elt(pbody, DOT11_MNG_RATES_ID,
	                                sup_rates.count, sup_rates.rates);

	/* Extended Supported Rates */
	if (ext_rates.count > 0)
		pbody = wlc_scan_write_info_elt(pbody, DOT11_MNG_EXT_RATES_ID, ext_rates.count,
		                                ext_rates.rates);

	/* If 2.4 Ghz, add DS Parameters. New in 802.11k, see 7.2.3.8 */
	if (CHSPEC_IS2G(wlc_hw->chanspec)) {
		pbody[0] = DOT11_MNG_DS_PARMS_ID;
		pbody[1] = 1;
		pbody[2] = wf_chspec_ctlchan(wlc_hw->chanspec);
		pbody += 3;
	}

	frameid = wlc_macol_d11hdrs(wlc_hw->ol, p, rspec_override, fifo);
	wlc_bmac_txfifo(wlc_hw, fifo, p, TRUE, frameid, 1);
	wlc_dngl_cntinc(wlc_hw->wlc->wlc_dngl_ol, TXPROBEREQ);
}

void
wlc_scan_11d_scan_complete(scan_info_t *scan, int status)
{
/*	wlc_11d_scan_complete(scan->wlc->m11d, WLC_E_STATUS_SUCCESS); */
}

const char *
wlc_scan_11d_get_autocountry_default(scan_info_t *scan)
{
	return scan->ol->autocountry_default;
}

void
wlc_scan_radio_mpc_upd(scan_info_t *scan)
{
	do {} while (0);
}

void
wlc_scan_radio_upd(scan_info_t *scan)
{
	do {} while (0);
}

uint
wlc_rateset_get_legacy_rateidx(ratespec_t rspec)
{
	uint8 rate = rspec & RATE_MASK;	/* mask out basic rate flag */
	uint rindex;
	const wlc_rateset_t* cur_rates = NULL;

	if (IS_OFDM(rspec)) {
		cur_rates = &ofdm_rates;
	} else {
		/* for 11b: B[32:33] represents phyrate
		 * (0 = 1mbps, 1 = 2mbps, 2 = 5.5mbps, 3 = 11mbps)
		 */
		cur_rates = &cck_rates;
	}
	for (rindex = 0; rindex < cur_rates->count; rindex++) {
		if ((cur_rates->rates[rindex] & RATE_MASK) == rate) {
			break;
		}
	}
	ASSERT(rindex < cur_rates->count);
	return rindex;
}

static bool
wlc_scanol_pref_ssid_match(wlc_scan_info_t *wlc_scan_info, uint8 *ssid)
{
	scan_info_t *scan = (scan_info_t *)wlc_scan_info->scan_priv;
	scanol_info_t *ol = scan->ol;
	uint i;

	for (i = 0; i < ol->npref_ssid; i++) {
		WL_INFORM(("match ssid %s, ol->prefssid %s\n", ssid, ol->pref_ssidlist[i].SSID));
		if (strcmp((char *)ol->pref_ssidlist[i].SSID, (char *)ssid) == 0)
			return TRUE;
	}
	return FALSE;
}

void
wlc_build_roam_cache(wlc_bsscfg_t *cfg, wlc_bss_list_t *candidates)
{
}
/* ======== End of Scan Offload support functions ======== */
