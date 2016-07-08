/*
 * 802.11 QoS/EDCF/WME/APSD configuration and utility module
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_qoscfg.c 611978 2016-01-12 18:06:05Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>
#include <wlc_bmac.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>
#include <wlc_ie_helper.h>
#include <wlc_ie_reg.h>
#include <wlc_qoscfg.h>
#include <wlc_pm.h>
#include <wlc_dump.h>
#ifdef PROP_TXSTATUS
#include <wlc_wlfc.h>
#endif
/* iovar table */
enum {
	IOV_WME = 1,
	IOV_WME_BSS_DISABLE = 2,
	IOV_WME_NOACK = 3,
	IOV_WME_APSD = 4,
	IOV_WME_APSD_TRIGGER = 5,	/* APSD Trigger Frame interval in ms */
	IOV_WME_AUTO_TRIGGER = 6,	/* enable/disable APSD AUTO Trigger frame */
	IOV_WME_TRIGGER_AC = 7,		/* APSD trigger frame AC */
	IOV_WME_QOSINFO = 9,
	IOV_WME_DP = 10,
	IOV_WME_COUNTERS = 11,
	IOV_WME_CLEAR_COUNTERS = 12,
	IOV_WME_PREC_QUEUING = 13,
	IOV_WME_TX_PARAMS = 14,
	IOV_WME_MBW_PARAMS = 15,
	IOV_SEND_FRAME = 16,		/* send a frame */
	IOV_WME_AC_STA = 17,
	IOV_WME_AC_AP = 18,
	IOV_EDCF_ACP = 19,		/* ACP used by the h/w */
	IOV_EDCF_ACP_AD = 20,		/* ACP advertised in bcn/prbrsp */
	IOV_LAST
};

static const bcm_iovar_t wme_iovars[] = {
	{"wme", IOV_WME, IOVF_SET_DOWN|IOVF_OPEN_ALLOW|IOVF_RSDB_SET, IOVT_INT32, 0},
	{"wme_bss_disable", IOV_WME_BSS_DISABLE, IOVF_OPEN_ALLOW, IOVT_INT32, 0},
	{"wme_noack", IOV_WME_NOACK, IOVF_OPEN_ALLOW, IOVT_BOOL, 0},
	{"wme_apsd", IOV_WME_APSD, IOVF_SET_DOWN|IOVF_OPEN_ALLOW, IOVT_BOOL, 0},
#ifdef STA
	{"wme_apsd_trigger", IOV_WME_APSD_TRIGGER,
	IOVF_OPEN_ALLOW|IOVF_BSSCFG_STA_ONLY, IOVT_UINT32, 0},
	{"wme_trigger_ac", IOV_WME_TRIGGER_AC, IOVF_OPEN_ALLOW, IOVT_UINT8, 0},
	{"wme_auto_trigger", IOV_WME_AUTO_TRIGGER, IOVF_OPEN_ALLOW, IOVT_BOOL, 0},
	{"wme_qosinfo", IOV_WME_QOSINFO,
	IOVF_BSS_SET_DOWN|IOVF_OPEN_ALLOW|IOVF_BSSCFG_STA_ONLY, IOVT_UINT8, 0},
#endif /* STA */
	{"wme_dp", IOV_WME_DP, IOVF_SET_DOWN|IOVF_OPEN_ALLOW|IOVF_RSDB_SET, IOVT_UINT8, 0},
	{"wme_prec_queuing", IOV_WME_PREC_QUEUING, IOVF_OPEN_ALLOW|IOVF_RSDB_SET, IOVT_BOOL, 0},
#if defined(WLCNT)
	{"wme_counters", IOV_WME_COUNTERS, IOVF_OPEN_ALLOW, IOVT_BUFFER, sizeof(wl_wme_cnt_t)},
	{"wme_clear_counters", IOV_WME_CLEAR_COUNTERS, IOVF_OPEN_ALLOW, IOVT_VOID, 0},
#endif
#ifndef WLP2P_UCODE_MACP
	{"wme_ac_sta", IOV_WME_AC_STA, IOVF_OPEN_ALLOW, IOVT_BUFFER, sizeof(edcf_acparam_t)},
	{"wme_ac_ap", IOV_WME_AC_AP, 0, IOVT_BUFFER, sizeof(edcf_acparam_t)},
#endif
	{"edcf_acp", IOV_EDCF_ACP, IOVF_OPEN_ALLOW, IOVT_BUFFER, sizeof(edcf_acparam_t)},
	{"edcf_acp_ad", IOV_EDCF_ACP_AD, IOVF_OPEN_ALLOW, IOVT_BUFFER, sizeof(edcf_acparam_t)},
#if defined(WME_PER_AC_TUNING) && defined(WME_PER_AC_TX_PARAMS)
	{"wme_tx_params", IOV_WME_TX_PARAMS,
	IOVF_OPEN_ALLOW, IOVT_BUFFER, WL_WME_TX_PARAMS_IO_BYTES},
#endif
#ifdef STA
	{"send_frame", IOV_SEND_FRAME, IOVF_SET_UP, IOVT_BUFFER, 0},
#endif
	{NULL, 0, 0, 0, 0}
};

/* module info */
struct wlc_qos_info {
	wlc_info_t *wlc;
};

/** TX FIFO number to WME/802.1E Access Category */
const uint8 wme_fifo2ac[] = { AC_BK, AC_BE, AC_VI, AC_VO, AC_BE, AC_BE };

/** WME/802.1E Access Category to TX FIFO number */
static const uint8 wme_ac2fifo[] = { 1, 0, 2, 3 };

/* Shared memory location index for various AC params */
#define wme_shmemacindex(ac)	wme_ac2fifo[ac]

const char *const aci_names[] = {"AC_BE", "AC_BK", "AC_VI", "AC_VO"};

static void wlc_edcf_acp_set_sw(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *edcf_acp);
static void wlc_edcf_acp_set_hw(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *edcf_acp);
static void wlc_edcf_acp_ad_get_all(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp);
static int wlc_edcf_acp_ad_set_one(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp);

static const wme_param_ie_t stadef = {
	WME_OUI,
	WME_OUI_TYPE,
	WME_SUBTYPE_PARAM_IE,
	WME_VER,
	0,
	0,
	{
		{ EDCF_AC_BE_ACI_STA, EDCF_AC_BE_ECW_STA, HTOL16(EDCF_AC_BE_TXOP_STA) },
		{ EDCF_AC_BK_ACI_STA, EDCF_AC_BK_ECW_STA, HTOL16(EDCF_AC_BK_TXOP_STA) },
		{ EDCF_AC_VI_ACI_STA, EDCF_AC_VI_ECW_STA, HTOL16(EDCF_AC_VI_TXOP_STA) },
		{ EDCF_AC_VO_ACI_STA, EDCF_AC_VO_ECW_STA, HTOL16(EDCF_AC_VO_TXOP_STA) }
	}
};

/** Initialize a WME Parameter Info Element with default STA parameters from WMM Spec, Table 12 */
static void
wlc_wme_initparams_sta(wlc_info_t *wlc, wme_param_ie_t *pe)
{
	BCM_REFERENCE(wlc);
	STATIC_ASSERT(sizeof(*pe) == WME_PARAM_IE_LEN);
	memcpy(pe, &stadef, sizeof(*pe));
}

static const wme_param_ie_t apdef = {
	WME_OUI,
	WME_OUI_TYPE,
	WME_SUBTYPE_PARAM_IE,
	WME_VER,
	0,
	0,
	{
		{ EDCF_AC_BE_ACI_AP, EDCF_AC_BE_ECW_AP, HTOL16(EDCF_AC_BE_TXOP_AP) },
		{ EDCF_AC_BK_ACI_AP, EDCF_AC_BK_ECW_AP, HTOL16(EDCF_AC_BK_TXOP_AP) },
		{ EDCF_AC_VI_ACI_AP, EDCF_AC_VI_ECW_AP, HTOL16(EDCF_AC_VI_TXOP_AP) },
		{ EDCF_AC_VO_ACI_AP, EDCF_AC_VO_ECW_AP, HTOL16(EDCF_AC_VO_TXOP_AP) }
	}
};

/* Initialize a WME Parameter Info Element with default AP parameters from WMM Spec, Table 14 */
static void
wlc_wme_initparams_ap(wlc_info_t *wlc, wme_param_ie_t *pe)
{
	BCM_REFERENCE(wlc);
	STATIC_ASSERT(sizeof(*pe) == WME_PARAM_IE_LEN);
	memcpy(pe, &apdef, sizeof(*pe));
}

static void
wlc_edcf_setparams(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint aci, bool suspend)
{
	uint i;
	edcf_acparam_t *acp;

	/*
	 * EDCF params are getting updated, so reset the frag thresholds for ACs,
	 * if those were changed to honor the TXOP settings
	 */
	for (i = 0; i < NFIFO; i++)
		wlc_fragthresh_set(wlc, i, wlc->usr_fragthresh);

	ASSERT(aci < AC_COUNT);

	acp = &cfg->wme->wme_param_ie.acparam[aci];

	wlc_edcf_acp_set_sw(wlc, cfg, acp);

	/* Only apply params to h/w if the core is out of reset and has clocks */
	if (!wlc->clk || !cfg->associated)
		return;

	if (suspend)
		wlc_suspend_mac_and_wait(wlc);

	wlc_edcf_acp_set_hw(wlc, cfg, acp);

	if (BSSCFG_AP(cfg)) {
		WL_APSTA_BCN(("wl%d.%d: wlc_edcf_setparams() -> wlc_update_beacon()\n",
		              wlc->pub->unit, WLC_BSSCFG_IDX(cfg)));
		wlc_bss_update_beacon(wlc, cfg);
		wlc_bss_update_probe_resp(wlc, cfg, FALSE);
	}

	if (suspend)
		wlc_enable_mac(wlc);
}

/**
 * Extract one AC param set from the WME IE acparam 'acp' and use them to update WME into in 'cfg'
 */
static void
wlc_edcf_acp_set_sw(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp)
{
	uint aci;
	wlc_wme_t *wme = cfg->wme;

	/* find out which ac this set of params applies to */
	aci = (acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT;
	ASSERT(aci < AC_COUNT);
	/* set the admission control policy for this AC */
	wme->wme_admctl &= ~(1 << aci);
	if (acp->ACI & EDCF_ACM_MASK) {
		wme->wme_admctl |= (1 << aci);
	}
	/* convert from units of 32us to us for ucode */
	wme->edcf_txop[aci] = EDCF_TXOP2USEC(ltoh16(acp->TXOP));

	WL_INFORM(("wl%d.%d: Setting %s: admctl 0x%x txop 0x%x\n",
	           wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
	           aci_names[aci], wme->wme_admctl, wme->edcf_txop[aci]));
}

/** Extract one AC param set from the WME IE acparam 'acp' and plumb them into h/w */
static void
wlc_edcf_acp_set_hw(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp)
{
	uint aci, j;
	shm_acparams_t acp_shm;
	uint16 *shm_entry;
	uint offset;

	bzero((char*)&acp_shm, sizeof(shm_acparams_t));
	/* find out which ac this set of params applies to */
	aci = (acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT;
	ASSERT(aci < AC_COUNT);

	/* fill in shm ac params struct */
	acp_shm.txop = ltoh16(acp->TXOP);
	/* convert from units of 32us to us for ucode */

	acp_shm.txop = EDCF_TXOP2USEC(acp_shm.txop);

	acp_shm.aifs = (acp->ACI & EDCF_AIFSN_MASK);

	if (acp_shm.aifs < EDCF_AIFSN_MIN || acp_shm.aifs > EDCF_AIFSN_MAX) {
		WL_ERROR(("wl%d.%d: %s: Setting %s: bad aifs %d\n",
		          wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__,
		          aci_names[aci], acp_shm.aifs));
		return;
	}


	/* CWmin = 2^(ECWmin) - 1 */
	acp_shm.cwmin = EDCF_ECW2CW(acp->ECW & EDCF_ECWMIN_MASK);
	/* CWmax = 2^(ECWmax) - 1 */
	acp_shm.cwmax =
	        EDCF_ECW2CW((acp->ECW & EDCF_ECWMAX_MASK) >> EDCF_ECWMAX_SHIFT);
	acp_shm.cwcur = acp_shm.cwmin;
	acp_shm.bslots = R_REG(wlc->osh, &wlc->regs->u.d11regs.tsf_random) & acp_shm.cwcur;
	acp_shm.reggap = acp_shm.bslots + acp_shm.aifs;
	/* Indicate the new params to the ucode */
	offset = M_EDCF_BLKS(wlc) + wme_shmemacindex(aci) * M_EDCF_QLEN(wlc) +
		M_EDCF_STATUS_OFF(wlc);
	acp_shm.status = wlc_read_shm(wlc, offset);
	acp_shm.status |= WME_STATUS_NEWAC;

	/* Fill in shm acparam table */
	shm_entry = (uint16*)&acp_shm;
	for (j = 0; j < (int)sizeof(shm_acparams_t); j += 2) {
		offset = M_EDCF_BLKS(wlc) + wme_shmemacindex(aci) * M_EDCF_QLEN(wlc) + j;
		wlc_write_shm(wlc, offset, *shm_entry++);
	}

	WL_INFORM(("wl%d.%d: Setting %s: txop 0x%x cwmin 0x%x cwmax 0x%x "
	           "cwcur 0x%x aifs 0x%x bslots 0x%x reggap 0x%x status 0x%x\n",
	           wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
	           aci_names[aci], acp_shm.txop, acp_shm.cwmin,
	           acp_shm.cwmax, acp_shm.cwcur, acp_shm.aifs,
	           acp_shm.bslots, acp_shm.reggap, acp_shm.status));
} /* wlc_edcf_acp_set_hw */

/** Extract all AC param sets from WME IE 'ie' and write them into host struct 'acp' */
static void
_wlc_edcf_acp_get_all(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wme_param_ie_t *ie, edcf_acparam_t *acp)
{
	uint i;
	edcf_acparam_t *acp_ie = ie->acparam;
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(cfg);
	for (i = 0; i < AC_COUNT; i++, acp_ie++, acp ++) {
		acp->ACI = acp_ie->ACI;
		acp->ECW = acp_ie->ECW;
		/* convert to host order */
		acp->TXOP = ltoh16(acp_ie->TXOP);
	}
}

/** Extract all AC param sets used by the BSS and write them into host struct 'acp' */
void
wlc_edcf_acp_get_all(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp)
{
	_wlc_edcf_acp_get_all(wlc, cfg, &cfg->wme->wme_param_ie, acp);
}

/** Extract all AC param sets used in BCN/PRBRSP and write them into host struct 'acp' */
static void
wlc_edcf_acp_ad_get_all(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp)
{
	ASSERT(BSSCFG_AP(cfg));
	_wlc_edcf_acp_get_all(wlc, cfg, cfg->wme->wme_param_ie_ad, acp);
}

/** Extract one set of AC params from host struct 'acp' and write them into WME IE 'ie' */
static int
_wlc_edcf_acp_set_ie(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp, wme_param_ie_t *ie)
{
	uint aci_in;
	edcf_acparam_t *acp_ie;

	/* Determine requested entry */
	aci_in = (acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT;
	if (aci_in >= AC_COUNT) {
		WL_ERROR(("wl%d.%d: Set of AC Params with bad ACI %d\n", WLCWLUNIT(wlc),
			WLC_BSSCFG_IDX(cfg), aci_in));
		return BCME_RANGE;
	}

	/* Set the contents as specified */
	acp_ie = &ie->acparam[aci_in];
	acp_ie->ACI = acp->ACI;
	acp_ie->ECW = acp->ECW;
	/* convert to network order */
	acp_ie->TXOP = htol16(acp->TXOP);

	WL_INFORM(("wl%d.%d: setting %s RAW AC Params ACI 0x%x ECW 0x%x TXOP 0x%x\n",
	           wlc->pub->unit, WLC_BSSCFG_IDX(cfg), aci_names[aci_in],
	           acp_ie->ACI, acp_ie->ECW, acp_ie->TXOP));

	/* APs need to notify any clients */
	if (BSSCFG_AP(cfg)) {
		/* Increment count field to notify associated
		 * STAs of parameter change
		 */
		int count = ie->qosinfo & WME_QI_AP_COUNT_MASK;
		ie->qosinfo &= ~WME_QI_AP_COUNT_MASK;
		ie->qosinfo |= (count + 1) & WME_QI_AP_COUNT_MASK;
	}

	return BCME_OK;
} /* _wlc_edcf_acp_set_ie */

/**
 * Extract one set of AC params from host struct 'acp' and put them into WME IE whose AC params are
 * used by h/w.
 */
static int
wlc_edcf_acp_set_ie(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp)
{
	return _wlc_edcf_acp_set_ie(wlc, cfg, acp, &cfg->wme->wme_param_ie);
}

/**
 * Extract one set of AC params from host struct 'acp' and write them into WME IE used in
 * BCN/PRBRSP.
 */
static int
wlc_edcf_acp_ad_set_ie(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp)
{
	ASSERT(BSSCFG_AP(cfg));
	return _wlc_edcf_acp_set_ie(wlc, cfg, acp, cfg->wme->wme_param_ie_ad);
}

/**
 * Extract one set of AC params from host struct 'acp', write them into the WME IE whose AC param
 * are used by h/w, and them to the h/w'. It also updates other WME info in 'cfg' based on 'acp'.
 */
static int
wlc_edcf_acp_set(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp, bool suspend)
{
	int err;
	uint aci;

	err = wlc_edcf_acp_set_ie(wlc, cfg, acp);
	if (err != BCME_OK)
		return err;

	aci = (acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT;
	ASSERT(aci < AC_COUNT);

	wlc_edcf_setparams(wlc, cfg, aci, suspend);
	return BCME_OK;
}

/* APIs to external users including ioctl/iovar handlers... */

/**
 * Function used by ioctl/iovar code. Extract one set of AC params from host struct 'acp'
 * and write them into the WME IE whose AC params are used by h/w and update the h/w.
 */
int
wlc_edcf_acp_set_one(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp, bool suspend)
{
	uint aci = (acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT;
#ifndef WLP2P_UCODE_MACP
	int idx;
	wlc_bsscfg_t *bc;
	int err;
#endif

	if (aci >= AC_COUNT)
		return BCME_RANGE;

#ifndef WLP2P_UCODE_MACP
	/* duplicate to all BSSs if the h/w supports only a single set of WME params... */
	FOREACH_BSS(wlc, idx, bc) {
		if (bc == cfg)
			continue;
		WL_INFORM(("wl%d: Setting %s in cfg %d...\n",
		           wlc->pub->unit, aci_names[aci], WLC_BSSCFG_IDX(bc)));
		err = wlc_edcf_acp_set_ie(wlc, bc, acp);
		if (err != BCME_OK)
			return err;
	}
#endif /* !WLP2P_UCODE_MACP */

	return wlc_edcf_acp_set(wlc, cfg, acp, suspend);
}

/**
 * Function used by ioctl/iovar code. Extract one set of AC params from host struct 'acp'
 * and write them into the WME IE used in BCN/PRBRSP and update the h/w.
 */
static int
wlc_edcf_acp_ad_set_one(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp)
{
	uint aci = (acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT;
#ifndef WLP2P_UCODE_MACP
	int idx;
	wlc_bsscfg_t *bc;
#endif

	if (aci >= AC_COUNT)
		return BCME_RANGE;

#ifndef WLP2P_UCODE_MACP
	/* duplicate to all BSSs if the h/w supports only a single set of WME params... */
	FOREACH_AP(wlc, idx, bc) {
		if (bc == cfg)
			continue;
		WL_INFORM(("wl%d: Setting advertised %s in cfg %d...\n",
		           wlc->pub->unit, aci_names[aci], WLC_BSSCFG_IDX(bc)));
		wlc_edcf_acp_ad_set_ie(wlc, bc, acp);
	}
#endif /* !WLP2P_UCODE_MACP */

	return wlc_edcf_acp_ad_set_ie(wlc, cfg, acp);
}

/** Apply all AC param sets stored in WME IE in 'cfg' to the h/w */
int
wlc_edcf_acp_apply(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend)
{
	edcf_acparam_t acp[AC_COUNT];
	uint i;
	int err = BCME_OK;
#ifndef WLP2P_UCODE_MACP
	int idx;
	wlc_bsscfg_t *tplt;
#endif
#ifdef PROP_TXSTATUS
	bool bCancelCreditBorrow = FALSE;
#endif
#ifndef WLP2P_UCODE_MACP
	/* find the first STA (primary STA has priority) and use its AC params */
	FOREACH_AS_STA(wlc, idx, tplt) {
		if (BSS_WME_AS(wlc, tplt))
			break;
	}
	if (tplt != NULL) {
		WL_INFORM(("wl%d: Using cfg %d's AC params...\n",
		           wlc->pub->unit, WLC_BSSCFG_IDX(tplt)));
		cfg = tplt;
	}
#endif

	wlc_edcf_acp_get_all(wlc, cfg, acp);
#ifdef PROP_TXSTATUS
	for (i = AC_VI; i < AC_COUNT; ++i) {
		if ((acp[i].ACI & EDCF_AIFSN_MASK) > (acp[i-1].ACI & EDCF_AIFSN_MASK))
			bCancelCreditBorrow = TRUE;
	}
	wlfc_enable_cred_borrow(wlc, !bCancelCreditBorrow);
#endif /* PROP_TXSTATUS */
	for (i = 0; i < AC_COUNT; i++) {
		err = wlc_edcf_acp_set_one(wlc, cfg, &acp[i], suspend);
		if (err != BCME_OK)
			return err;
	}
	return BCME_OK;
}

/* module interfaces */
static int
wlc_qos_wlc_up(void *ctx)
{
	wlc_qos_info_t *qosi = (wlc_qos_info_t *)ctx;
	wlc_info_t *wlc = qosi->wlc;

	/* Set EDCF hostflags */
	if (EDCF_ENAB(wlc->pub)) {
		wlc_mhf(wlc, MHF1, MHF1_EDCF, MHF1_EDCF, WLC_BAND_ALL);
#if defined(WME_PER_AC_TX_PARAMS)
		{
		int ac;
		bool enab = FALSE;

		for (ac = 0; ac < AC_COUNT; ac++) {
			if (wlc->wme_max_rate[ac] != 0) {
				enab = TRUE;
				break;
			}
		}
		WL_RATE(("Setting per ac maxrate to %d \n", enab));
		wlc->pub->_per_ac_maxrate = enab;
		}
#endif /* WME_PER_AC_TX_PARAMS */
	} else {
		wlc_mhf(wlc, MHF1, MHF1_EDCF, 0, WLC_BAND_ALL);
	}

	return BCME_OK;
}

/** Write WME tunable parameters for retransmit/max rate from wlc struct to ucode */
void
wlc_wme_retries_write(wlc_info_t *wlc)
{
	int ac;

	/* Need clock to do this */
	if (!wlc->clk)
		return;

	for (ac = 0; ac < AC_COUNT; ac++) {
		wlc_write_shm(wlc, M_AC_TXLMT_ADDR(wlc, ac), wlc->wme_retries[ac]);
	}
}

/** Convert discard policy AC bitmap to Precedence bitmap */
static uint16
wlc_convert_acbitmap_to_precbitmap(ac_bitmap_t acbitmap)
{
	uint16 prec_bitmap = 0;

	if (AC_BITMAP_TST(acbitmap, AC_BE))
		prec_bitmap |= WLC_PREC_BMP_AC_BE;

	if (AC_BITMAP_TST(acbitmap, AC_BK))
		prec_bitmap |= WLC_PREC_BMP_AC_BK;

	if (AC_BITMAP_TST(acbitmap, AC_VI))
		prec_bitmap |= WLC_PREC_BMP_AC_VI;

	if (AC_BITMAP_TST(acbitmap, AC_VO))
		prec_bitmap |= WLC_PREC_BMP_AC_VO;

	return prec_bitmap;
}

void
wlc_wme_shm_read(wlc_info_t *wlc)
{
	uint ac;

	for (ac = 0; ac < AC_COUNT; ac++) {
		uint16 temp = wlc_read_shm(wlc, M_AC_TXLMT_ADDR(wlc, ac));

		if (WLC_WME_RETRY_SHORT_GET(wlc, ac) == 0)
			WLC_WME_RETRY_SHORT_SET(wlc, ac, WME_RETRY_SHORT_GET(temp, ac));

		if (WLC_WME_RETRY_SFB_GET(wlc, ac) == 0)
			WLC_WME_RETRY_SFB_SET(wlc, ac, WME_RETRY_SFB_GET(temp, ac));

		if (WLC_WME_RETRY_LONG_GET(wlc, ac) == 0)
			WLC_WME_RETRY_LONG_SET(wlc, ac, WME_RETRY_LONG_GET(temp, ac));

		if (WLC_WME_RETRY_LFB_GET(wlc, ac) == 0)
			WLC_WME_RETRY_LFB_SET(wlc, ac, WME_RETRY_LFB_GET(temp, ac));
	}
}

/* Nybble swap; QoS Info bits are in backward order from AC bitmap */
static const ac_bitmap_t qi_bitmap_to_ac_bitmap[16] = {
	0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15
};
/* Max USP Length field decoding (WMM 2.2.1) */
static const uint16 maxsp[4] = { WLC_APSD_USP_UNB, 2, 4, 6 };

/** Decode the WMM 2.1.1 QoS Info field, as sent by WMM STA, and save in scb */
void
	wlc_qosinfo_update(struct scb *scb, uint8 qosinfo, bool ac_upd)
{
	scb->apsd.maxsplen =
	        maxsp[(qosinfo & WME_QI_STA_MAXSPLEN_MASK) >> WME_QI_STA_MAXSPLEN_SHIFT];

	/* Update ac_defl during assoc and reassoc (static power save settings). */
	scb->apsd.ac_defl = qi_bitmap_to_ac_bitmap[qosinfo & WME_QI_STA_APSD_ALL_MASK];

	/* Initial config; can be updated by TSPECs, and revert back to ac_defl on DELTS. */
	if (ac_upd) {
		/* Do not update trig and delv for reassoc resp with same AP */
		scb->apsd.ac_trig = scb->apsd.ac_defl;
		scb->apsd.ac_delv = scb->apsd.ac_defl;
	}
}

static int
wlc_qos_doiovar(void *ctx, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint plen, void *arg, int alen, int val_size, struct wlc_if *wlcif)
{
	wlc_qos_info_t *qosi = (wlc_qos_info_t *)ctx;
	wlc_info_t *wlc = qosi->wlc;
	wlc_bsscfg_t *bsscfg;
	int err = BCME_OK;
	int32 int_val = 0;
	int32 *ret_int_ptr;
	bool bool_val;
	wlc_wme_t *wme;

	BCM_REFERENCE(vi);
	BCM_REFERENCE(name);
	BCM_REFERENCE(val_size);
	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	wme = bsscfg->wme;

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (plen >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	bool_val = (int_val != 0) ? TRUE : FALSE;

	/* Do the actual parameter implementation */
	switch (actionid) {

#ifndef WLP2P_UCODE_MACP
	case IOV_GVAL(IOV_WME_AC_STA): {
		edcf_acparam_t acp_all[AC_COUNT];

		if (!WME_ENAB(wlc->pub)) {
			err = BCME_UNSUPPORTED;
			break;
		}
		if (alen < (int)sizeof(acp_all)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		if (BSSCFG_STA(bsscfg))
			wlc_edcf_acp_get_all(wlc, bsscfg, acp_all);
		if (BSSCFG_AP(bsscfg))
			wlc_edcf_acp_ad_get_all(wlc, bsscfg, acp_all);
		memcpy(arg, acp_all, sizeof(acp_all));    /* Copy to handle unaligned */
		break;
	}
	case IOV_GVAL(IOV_WME_AC_AP):
#endif /* WLP2P_UCODE_MACP */
	case IOV_GVAL(IOV_EDCF_ACP): {
		edcf_acparam_t acp_all[AC_COUNT];

		if (!WME_ENAB(wlc->pub)) {
			err = BCME_UNSUPPORTED;
			break;
		}
		if (alen < (int)sizeof(acp_all)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		wlc_edcf_acp_get_all(wlc, bsscfg, acp_all);
		memcpy(arg, acp_all, sizeof(acp_all));    /* Copy to handle unaligned */
		break;
	}
#ifdef AP
	case IOV_GVAL(IOV_EDCF_ACP_AD): {
		edcf_acparam_t acp_all[AC_COUNT];

		if (!WME_ENAB(wlc->pub)) {
			err = BCME_UNSUPPORTED;
			break;
		}
		if (alen < (int)sizeof(acp_all)) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		if (!BSSCFG_AP(bsscfg)) {
			err = BCME_ERROR;
			break;
		}

		wlc_edcf_acp_ad_get_all(wlc, bsscfg, acp_all);
		memcpy(arg, acp_all, sizeof(acp_all));    /* Copy to handle unaligned */
		break;
	}
#endif /* AP */

#ifndef WLP2P_UCODE_MACP
	case IOV_SVAL(IOV_WME_AC_STA): {
		edcf_acparam_t acp;

		if (!WME_ENAB(wlc->pub)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		memcpy(&acp, arg, sizeof(acp));
		if (BSSCFG_STA(bsscfg))
			err = wlc_edcf_acp_set_one(wlc, bsscfg, &acp, TRUE);
		if (BSSCFG_AP(bsscfg))
			err = wlc_edcf_acp_ad_set_one(wlc, bsscfg, &acp);
		break;
	}
	case IOV_SVAL(IOV_WME_AC_AP):
#endif /* WLP2P_UCODE_MACP */
	case IOV_SVAL(IOV_EDCF_ACP): {
		edcf_acparam_t acp;

		if (!WME_ENAB(wlc->pub)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		memcpy(&acp, arg, sizeof(acp));
		err = wlc_edcf_acp_set_one(wlc, bsscfg, &acp, TRUE);
		break;
	}
#ifdef AP
	case IOV_SVAL(IOV_EDCF_ACP_AD): {
		edcf_acparam_t acp;

		if (!WME_ENAB(wlc->pub)) {
			err = BCME_UNSUPPORTED;
			break;
		}
		if (!BSSCFG_AP(bsscfg)) {
			err = BCME_ERROR;
			break;
		}

		memcpy(&acp, arg, sizeof(acp));
		err = wlc_edcf_acp_ad_set_one(wlc, bsscfg, &acp);
		break;
	}
#endif /* AP */

	case IOV_GVAL(IOV_WME):
		*ret_int_ptr = (int32)wlc->pub->_wme;
		break;

	case IOV_SVAL(IOV_WME):
		if (int_val < AUTO || int_val > ON) {
			err = BCME_RANGE;
			break;
		}

		/* For AP, AUTO mode is same as ON */
		if (AP_ENAB(wlc->pub) && int_val == AUTO)
			wlc->pub->_wme = ON;
		else
			wlc->pub->_wme = int_val;

#ifdef STA
		/* If not in AUTO mode, PM is always allowed
		 * In AUTO mode, PM is allowed on for UAPSD enabled AP
		 */
		if (int_val != AUTO) {
			int idx;
			wlc_bsscfg_t *bc;

			FOREACH_BSS(wlc, idx, bc) {
				if (!BSSCFG_STA(bc))
					continue;
				bc->pm->WME_PM_blocked = FALSE;
			}
		}
#endif

		break;

	case IOV_GVAL(IOV_WME_BSS_DISABLE):
		*ret_int_ptr = ((bsscfg->flags & WLC_BSSCFG_WME_DISABLE) != 0);
		break;

	case IOV_SVAL(IOV_WME_BSS_DISABLE):
		WL_INFORM(("%s(): set IOV_WME_BSS_DISABLE to %s\n", __FUNCTION__,
			bool_val ? "TRUE" : "FALSE"));
		if (bool_val) {
			bsscfg->flags |= WLC_BSSCFG_WME_DISABLE;
		} else {
			bsscfg->flags &= ~WLC_BSSCFG_WME_DISABLE;
		}
		break;

	case IOV_GVAL(IOV_WME_NOACK):
		*ret_int_ptr = (int32)wme->wme_noack;
		break;

	case IOV_SVAL(IOV_WME_NOACK):
		wme->wme_noack = bool_val;
		break;

	case IOV_GVAL(IOV_WME_APSD):
		*ret_int_ptr = (int32)wme->wme_apsd;
		break;

	case IOV_SVAL(IOV_WME_APSD):
		wme->wme_apsd = bool_val;
		if (BSSCFG_AP(bsscfg) && wlc->clk) {
			wlc_bss_update_beacon(wlc, bsscfg);
			wlc_bss_update_probe_resp(wlc, bsscfg, TRUE);
		}
		break;

	case IOV_SVAL(IOV_SEND_FRAME): {
		bool status;
		osl_t *osh = wlc->osh;
		void *pkt;

		/* Reject runts and jumbos */
		if (plen < ETHER_MIN_LEN || plen > ETHER_MAX_LEN || params == NULL) {
			err = BCME_BADARG;
			break;
		}
		pkt = PKTGET(osh, plen + TXOFF, TRUE);
		if (pkt == NULL) {
			err = BCME_NOMEM;
			break;
		}

		PKTPULL(osh, pkt, TXOFF);
		bcopy(params, PKTDATA(osh, pkt), plen);
		PKTSETLEN(osh, pkt, plen);
		status = wlc_sendpkt(wlc, pkt, wlcif);
		if (status)
			err = BCME_NORESOURCE;
		break;
	}

#ifdef STA
	case IOV_GVAL(IOV_WME_APSD_TRIGGER):
	        *ret_int_ptr = (int32)bsscfg->pm->apsd_trigger_timeout;
		break;

	case IOV_SVAL(IOV_WME_APSD_TRIGGER):
	        /*
		 * Round timeout up to an even number because we set
		 * the timer at 1/2 the timeout period.
		 */
		bsscfg->pm->apsd_trigger_timeout = ROUNDUP((uint32)int_val, 2);
		wlc_apsd_trigger_upd(bsscfg, TRUE);
		break;

	case IOV_GVAL(IOV_WME_TRIGGER_AC):
		*ret_int_ptr = (int32)wme->apsd_trigger_ac;
		break;

	case IOV_SVAL(IOV_WME_TRIGGER_AC):
		if (int_val <= AC_BITMAP_ALL)
			wme->apsd_trigger_ac = (ac_bitmap_t)int_val;
		else
			err = BCME_BADARG;
		break;

	case IOV_GVAL(IOV_WME_AUTO_TRIGGER):
		*ret_int_ptr = (int32)wme->apsd_auto_trigger;
		break;

	case IOV_SVAL(IOV_WME_AUTO_TRIGGER):
		wme->apsd_auto_trigger = bool_val;
		break;
#endif /* STA */

	case IOV_GVAL(IOV_WME_DP):
		*ret_int_ptr = (int32)wlc->wme_dp;
		break;

	case IOV_SVAL(IOV_WME_DP):
		wlc->wme_dp = (uint8)int_val;
		wlc->wme_dp_precmap = wlc_convert_acbitmap_to_precbitmap(wlc->wme_dp);
		break;

#if defined(WLCNT)
	case IOV_GVAL(IOV_WME_COUNTERS):
		bcopy(wlc->pub->_wme_cnt, arg, sizeof(wl_wme_cnt_t));
		break;
	case IOV_SVAL(IOV_WME_CLEAR_COUNTERS):
		/* Zero the counters but reinit the version and length */
		bzero(wlc->pub->_wme_cnt,  sizeof(wl_wme_cnt_t));
		WLCNTSET(wlc->pub->_wme_cnt->version, WL_WME_CNT_VERSION);
		WLCNTSET(wlc->pub->_wme_cnt->length, sizeof(wl_wme_cnt_t));
		break;
#endif
	case IOV_GVAL(IOV_WME_PREC_QUEUING):
		*ret_int_ptr = wlc->wme_prec_queuing ? 1 : 0;
		break;
	case IOV_SVAL(IOV_WME_PREC_QUEUING):
		wlc->wme_prec_queuing = bool_val;
		break;

#ifdef STA
	case IOV_GVAL(IOV_WME_QOSINFO):
		*ret_int_ptr = wme->apsd_sta_qosinfo;
		break;

	case IOV_SVAL(IOV_WME_QOSINFO):
		wme->apsd_sta_qosinfo = (uint8)int_val;
		break;
#endif /* STA */

#if defined(WME_PER_AC_TUNING) && defined(WME_PER_AC_TX_PARAMS)
	case IOV_GVAL(IOV_WME_TX_PARAMS):
		if (WME_PER_AC_TX_PARAMS_ENAB(wlc->pub)) {
			int ac;
			wme_tx_params_t *prms;

			prms = (wme_tx_params_t *)arg;

			for (ac = 0; ac < AC_COUNT; ac++) {
				prms[ac].max_rate = wlc->wme_max_rate[ac];
				prms[ac].short_retry = WLC_WME_RETRY_SHORT_GET(wlc, ac);
				prms[ac].short_fallback = WLC_WME_RETRY_SFB_GET(wlc, ac);
				prms[ac].long_retry = WLC_WME_RETRY_LONG_GET(wlc, ac);
				prms[ac].long_fallback = WLC_WME_RETRY_LFB_GET(wlc, ac);
			}
		}
		break;

	case IOV_SVAL(IOV_WME_TX_PARAMS):
		if (WME_PER_AC_TX_PARAMS_ENAB(wlc->pub)) {
			int ac;
			wme_tx_params_t *prms;

			prms = (wme_tx_params_t *)arg;

			wlc->LRL = prms[AC_BE].long_retry;
			wlc->SRL = prms[AC_BE].short_retry;
			wlc_bmac_retrylimit_upd(wlc->hw, wlc->SRL, wlc->LRL);

			for (ac = 0; ac < AC_COUNT; ac++) {
				wlc->wme_max_rate[ac] = prms[ac].max_rate;
				WLC_WME_RETRY_SHORT_SET(wlc, ac, prms[ac].short_retry);
				WLC_WME_RETRY_SFB_SET(wlc, ac, prms[ac].short_fallback);
				WLC_WME_RETRY_LONG_SET(wlc, ac, prms[ac].long_retry);
				WLC_WME_RETRY_LFB_SET(wlc, ac, prms[ac].long_fallback);
			}

			wlc_wme_retries_write(wlc);
		}
		break;

#endif /* WME_PER_AC_TUNING && PER_AC_TX_PARAMS */


	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
} /* wlc_qos_doiovar */

#define wlc_qos_bss_dump NULL

static int
wlc_qos_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_qos_info_t *qosi = (wlc_qos_info_t *)ctx;
	wlc_info_t *wlc = qosi->wlc;
	int err = BCME_OK;

	/* EDCF/APSD/WME */
	if ((cfg->wme = (wlc_wme_t *)
	     MALLOCZ(wlc->osh, sizeof(wlc_wme_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		err = BCME_NOMEM;
		goto fail;
	}

	/* APSD defaults */
	cfg->wme->wme_apsd = TRUE;

	if (BSSCFG_STA(cfg)) {
		cfg->wme->apsd_trigger_ac = AC_BITMAP_ALL;
		wlc_wme_initparams_sta(wlc, &cfg->wme->wme_param_ie);
	}
	/* WME ACP advertised in bcn/prbrsp */
	else if (BSSCFG_AP(cfg)) {
		if ((cfg->wme->wme_param_ie_ad =
		     MALLOCZ(wlc->osh, sizeof(wme_param_ie_t))) == NULL) {
			WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		    err = BCME_NOMEM;
		    goto fail;
		}
		wlc_wme_initparams_ap(wlc, &cfg->wme->wme_param_ie);
		wlc_wme_initparams_sta(wlc, cfg->wme->wme_param_ie_ad);
	}

	return BCME_OK;
fail:
	return err;
}

static void
wlc_qos_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_qos_info_t *qosi = (wlc_qos_info_t *)ctx;
	wlc_info_t *wlc = qosi->wlc;

	/* EDCF/APSD/WME */
	if (cfg->wme != NULL) {
		/* WME AC parms */
		if (cfg->wme->wme_param_ie_ad != NULL) {
			MFREE(wlc->osh, cfg->wme->wme_param_ie_ad, sizeof(wme_param_ie_t));
		}
		MFREE(wlc->osh, cfg->wme, sizeof(wlc_wme_t));
	}
}

/* Common: WME parameters IE */
static uint
wlc_bss_calc_wme_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;
	bool iswme = FALSE;

	if (!BSS_WME_ENAB(wlc, cfg))
		return 0;

	switch (data->ft) {
	case FC_BEACON:
	case FC_PROBE_RESP:
		/* WME IE for beacon responses in IBSS when 11n enabled */
		if (BSSCFG_AP(cfg) || (!cfg->BSS && data->cbparm->ht))
			iswme = TRUE;
		break;
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ:
		/* Include a WME info element if the AP supports WME */
		if (ftcbparm->assocreq.target->flags & WLC_BSS_WME)
			iswme = TRUE;
		break;
	case FC_ASSOC_RESP:
	case FC_REASSOC_RESP:
		if (SCB_WME(ftcbparm->assocresp.scb))
			iswme = TRUE;
		break;
	}
	if (!iswme)
		return 0;

	if (BSSCFG_AP(cfg))
		return TLV_HDR_LEN + sizeof(wme_param_ie_t);
	else
		return TLV_HDR_LEN + sizeof(wme_ie_t);

	return 0;
}

static int
wlc_bss_write_wme_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	wlc_wme_t *wme = cfg->wme;
	wlc_iem_ft_cbparm_t *ftcbparm = data->cbparm->ft;
	bool iswme = FALSE;
	bool apapsd = FALSE;

	if (!BSS_WME_ENAB(wlc, cfg))
		return BCME_OK;

	switch (data->ft) {
	case FC_BEACON:
	case FC_PROBE_RESP:
		if (BSSCFG_AP(cfg))
			iswme = TRUE;
		/* WME IE for beacon responses in IBSS when 11n enabled */
		else if (!cfg->BSS && data->cbparm->ht) {
			apapsd = (cfg->current_bss->wme_qosinfo & WME_QI_AP_APSD_MASK) ?
			        TRUE : FALSE;
			iswme = TRUE;
		}
		break;
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ:
		/* Include a WME info element if the AP supports WME */
		if (ftcbparm->assocreq.target->flags & WLC_BSS_WME) {
			apapsd = (ftcbparm->assocreq.target->wme_qosinfo & WME_QI_AP_APSD_MASK) ?
			        TRUE : FALSE;
			iswme = TRUE;
		}
		break;
	case FC_ASSOC_RESP:
	case FC_REASSOC_RESP:
		if (SCB_WME(ftcbparm->assocresp.scb))
			iswme = TRUE;
		break;
	}
	if (!iswme)
		return BCME_OK;

	/* WME parameter info element in infrastructure beacons responses only */
	if (BSSCFG_AP(cfg)) {
		edcf_acparam_t *acp_ie = wme->wme_param_ie.acparam;
		wme_param_ie_t *ad = wme->wme_param_ie_ad;
		uint8 i = 0;

		if (wme->wme_apsd)
			ad->qosinfo |= WME_QI_AP_APSD_MASK;
		else
			ad->qosinfo &= ~WME_QI_AP_APSD_MASK;

		/* update the ACM value in WME IE */
		for (i = 0; i < AC_COUNT; i++, acp_ie++) {
			if (acp_ie->ACI & EDCF_ACM_MASK)
				ad->acparam[i].ACI |= EDCF_ACM_MASK;
			else
				ad->acparam[i].ACI &= ~EDCF_ACM_MASK;
		}

		bcm_write_tlv(DOT11_MNG_VS_ID, (uint8 *)ad, sizeof(wme_param_ie_t), data->buf);
	}
	else {
		wme_ie_t wme_ie;

		ASSERT(sizeof(wme_ie) == WME_IE_LEN);
		bcopy(WME_OUI, wme_ie.oui, WME_OUI_LEN);
		wme_ie.type = WME_OUI_TYPE;
		wme_ie.subtype = WME_SUBTYPE_IE;
		wme_ie.version = WME_VER;
		if (!wme->wme_apsd || !apapsd) {
			wme_ie.qosinfo = 0;
		} else {
			wme_ie.qosinfo = wme->apsd_sta_qosinfo;
		}

		bcm_write_tlv(DOT11_MNG_VS_ID, (uint8 *)&wme_ie, sizeof(wme_ie), data->buf);
	}

	return BCME_OK;
}

static int
wlc_bss_parse_wme_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	struct scb *scb;
	bcm_tlv_t *wme_ie = (bcm_tlv_t *)data->ie;
	wlc_wme_t *wme = cfg->wme;

	scb = wlc_iem_parse_get_assoc_bcn_scb(data);
	switch (data->ft) {
#ifdef AP
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ:
		ASSERT(scb != NULL);

		/* Handle WME association */
		scb->flags &= ~(SCB_WMECAP | SCB_APSDCAP);

		if (!BSS_WME_ENAB(wlc, cfg))
			break;

		wlc_qosinfo_update(scb, 0, TRUE);     /* Clear Qos Info by default */

		if (wme_ie == NULL)
			break;

		scb->flags |= SCB_WMECAP;

		/* Note requested APSD parameters if AP supporting APSD */
		if (!wme->wme_apsd)
			break;

		wlc_qosinfo_update(scb, ((wme_ie_t *)wme_ie->data)->qosinfo, TRUE);
		if (scb->apsd.ac_trig & AC_BITMAP_ALL)
			scb->flags |= SCB_APSDCAP;
		break;
#endif /* AP */
#ifdef STA
	case FC_ASSOC_RESP:
	case FC_REASSOC_RESP: {
		wlc_pm_st_t *pm = cfg->pm;
		bool upd_trig_delv;

		ASSERT(scb != NULL);

		/* If WME is enabled, check if response indicates WME association */
		scb->flags &= ~SCB_WMECAP;
		cfg->flags &= ~WLC_BSSCFG_WME_ASSOC;

		if (!BSS_WME_ENAB(wlc, cfg))
			break;

		/* Do not update ac_delv and ac for ReassocResp with same AP */
		/* upd_trig_delv is FALSE for ReassocResp with same AP, TRUE otherwise */
		upd_trig_delv = !((data->ft == FC_REASSOC_RESP) &&
			(!bcmp((char *)&cfg->prev_BSSID,
			(char *)&cfg->target_bss->BSSID, ETHER_ADDR_LEN)));
		wlc_qosinfo_update(scb, 0, upd_trig_delv);

		if (wme_ie == NULL)
			break;

		scb->flags |= SCB_WMECAP;
		cfg->flags |= WLC_BSSCFG_WME_ASSOC;

		/* save the new IE, or params IE which is superset of IE */
		bcopy(wme_ie->data, &wme->wme_param_ie, wme_ie->len);
		/* Apply the STA AC params sent by AP,
		 * will be done in wlc_join_adopt_bss()
		 */
		/* wlc_edcf_acp_apply(wlc, cfg, TRUE); */
		/* Use locally-requested APSD config if AP advertised APSD */
		/* STA is in AUTO WME mode,
		 *     AP has UAPSD enabled, then allow STA to use wlc->PM
		 *            else, don't allow STA to sleep based on wlc->PM only
		 *                  if it's BRCM AP not capable of handling
		 *                                  WME STAs in PS,
		 *                  and leave PM mode if already set
		 */
		if (wme->wme_param_ie.qosinfo & WME_QI_AP_APSD_MASK) {
			wlc_qosinfo_update(scb, wme->apsd_sta_qosinfo, upd_trig_delv);
			pm->WME_PM_blocked = FALSE;
			if (pm->PM == PM_MAX)
				wlc_set_pmstate(cfg, TRUE);
		}
		else if (WME_AUTO(wlc) &&
		         (scb->flags & SCB_BRCM)) {
			if (!(scb->flags & SCB_WMEPS)) {
				pm->WME_PM_blocked = TRUE;
				WL_RTDC(wlc, "wlc_recvctl: exit PS", 0, 0);
				wlc_set_pmstate(cfg, FALSE);
			}
			else {
				pm->WME_PM_blocked = FALSE;
				if (pm->PM == PM_MAX)
					wlc_set_pmstate(cfg, TRUE);
			}
		}
		break;
	}
	case FC_BEACON:
		/* WME: check if the AP has supplied new acparams */
		/* WME: check if IBSS WME_IE is present */
		if (!BSS_WME_AS(wlc, cfg))
			break;

		if (scb && BSSCFG_IBSS(cfg)) {
			if (wme_ie != NULL) {
				scb->flags |= SCB_WMECAP;
			}
			break;
		}

		if (wme_ie == NULL) {
			WL_ERROR(("wl%d: %s: wme params ie missing\n",
			          wlc->pub->unit, __FUNCTION__));
			/* for non-wme association, BE ACI is 2 */
			wme->wme_param_ie.acparam[0].ACI = NON_EDCF_AC_BE_ACI_STA;
			wlc_edcf_acp_apply(wlc, cfg, TRUE);
			break;
		}

		if ((((wme_ie_t *)wme_ie->data)->qosinfo & WME_QI_AP_COUNT_MASK) !=
		    (wme->wme_param_ie.qosinfo & WME_QI_AP_COUNT_MASK)) {
			/* save and apply new params ie */
			bcopy(wme_ie->data, &wme->wme_param_ie,	sizeof(wme_param_ie_t));
			/* Apply the STA AC params sent by AP */
			wlc_edcf_acp_apply(wlc, cfg, TRUE);
		}
		break;
#endif /* STA */
	default:
		(void)wlc;
		(void)scb;
		(void)wme_ie;
		(void)wme;
		break;
	}

	return BCME_OK;
}

wlc_qos_info_t *
BCMATTACHFN(wlc_qos_attach)(wlc_info_t *wlc)
{
	/* WME Vendor Specific IE */
	uint16 wme_build_fstbmp =
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_REQ) |
	        FT2BMP(FC_REASSOC_RESP) |
	        FT2BMP(FC_PROBE_RESP) |
	        FT2BMP(FC_BEACON) |
	        0;
	uint16 wme_parse_fstbmp =
	        FT2BMP(FC_ASSOC_REQ) |
	        FT2BMP(FC_ASSOC_RESP) |
	        FT2BMP(FC_REASSOC_REQ) |
	        FT2BMP(FC_REASSOC_RESP) |
	        FT2BMP(FC_BEACON) |
	        0;
	wlc_qos_info_t *qosi;

	ASSERT(M_EDCF_QLEN(wlc) == sizeof(shm_acparams_t));

	/* allocate module info */
	if ((qosi = MALLOCZ(wlc->osh, sizeof(*qosi))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	qosi->wlc = wlc;

#ifdef WLCNT
	if ((wlc->pub->_wme_cnt = MALLOCZ(wlc->osh, sizeof(wl_wme_cnt_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	WLCNTSET(wlc->pub->_wme_cnt->version, WL_WME_CNT_VERSION);
	WLCNTSET(wlc->pub->_wme_cnt->length, sizeof(wl_wme_cnt_t));
#endif /* WLCNT */

	/* reserve bss init/deinit */
	if (wlc_bsscfg_cubby_reserve(wlc, 0,
			wlc_qos_bss_init, wlc_qos_bss_deinit, wlc_qos_bss_dump,
			qosi) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* build/calc IE */
	if (wlc_iem_vs_add_build_fn_mft(wlc->iemi, wme_build_fstbmp, WLC_IEM_VS_IE_PRIO_WME,
			wlc_bss_calc_wme_ie_len, wlc_bss_write_wme_ie, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_vs_add_build_fn failed, wme ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* parse IE */
	if (wlc_iem_vs_add_parse_fn_mft(wlc->iemi, wme_parse_fstbmp, WLC_IEM_VS_IE_PRIO_WME,
			wlc_bss_parse_wme_ie, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_iem_vs_add_parse_fn failed, wme ie\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register module entries */
	if (wlc_module_register(wlc->pub, wme_iovars, "wme", qosi, wlc_qos_doiovar,
			NULL, wlc_qos_wlc_up, NULL)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


	return qosi;

fail:
	wlc_qos_detach(qosi);
	return NULL;
}

void
BCMATTACHFN(wlc_qos_detach)(wlc_qos_info_t *qosi)
{
	wlc_info_t *wlc;

	if (qosi == NULL)
		return;

	wlc = qosi->wlc;

	(void)wlc_module_unregister(wlc->pub, "wme", qosi);

#ifdef WLCNT
	if (wlc->pub->_wme_cnt) {
		MFREE(wlc->osh, wlc->pub->_wme_cnt, sizeof(wl_wme_cnt_t));
		wlc->pub->_wme_cnt = NULL;
	}
#endif

	MFREE(wlc->osh, qosi, sizeof(*qosi));
}
