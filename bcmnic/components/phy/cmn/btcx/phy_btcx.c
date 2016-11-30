/*
 * BlueToothCoExistence module implementation.
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
 * $Id: phy_btcx.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_wd.h>
#include <phy_type_btcx.h>
#include <phy_btcx_api.h>
#include <phy_utils_reg.h>
#include <bcmwifi_channels.h>

/* module private states */
struct phy_btcx_priv_info {
	phy_info_t *pi;
	phy_type_btcx_fns_t	*fns; /* PHY specific function ptrs */
};

/* module private states memory layout */
typedef struct {
	phy_btcx_info_t info;
	phy_type_btcx_fns_t fns;
	phy_btcx_priv_info_t priv;
	phy_btcx_data_t data;
/* add other variable size variables here at the end */
} phy_btcx_mem_t;

/* local function declaration */
static bool phy_btcx_wd(phy_wd_ctx_t *ctx);
static bool wlc_phy_btc_adjust(phy_wd_ctx_t *ctx);
static int phy_btcx_init(phy_init_ctx_t *ctx);

/* attach/detach */
phy_btcx_info_t *
BCMATTACHFN(phy_btcx_attach)(phy_info_t *pi)
{
	phy_btcx_info_t *info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_btcx_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->priv = &((phy_btcx_mem_t *)info)->priv;
	info->priv->pi = pi;
	info->priv->fns = &((phy_btcx_mem_t *)info)->fns;
	info->data = &((phy_btcx_mem_t *)info)->data;
	/* register watchdog fns */
	if (phy_wd_add_fn(pi->wdi, phy_btcx_wd, info,
	                  PHY_WD_PRD_FAST, PHY_WD_FAST_BTCX, PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}

	if (phy_wd_add_fn(pi->wdi, wlc_phy_btc_adjust, info, PHY_WD_PRD_1TICK,
	                  PHY_WD_1TICK_BTCX_ADJUST, PHY_WD_FLAG_DEF_SKIP) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_btcx_init, info, PHY_INIT_BTCX) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

	return info;

	/* error */
fail:
	phy_btcx_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_btcx_detach)(phy_btcx_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null btcx module\n", __FUNCTION__));
		return;
	}

	pi = info->priv->pi;

	phy_mfree(pi, info, sizeof(phy_btcx_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_btcx_register_impl)(phy_btcx_info_t *cmn_info,
	phy_type_btcx_fns_t *fns)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	*cmn_info->priv->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_btcx_unregister_impl)(phy_btcx_info_t *cmn_info)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	cmn_info->priv->fns = NULL;
}

/* watchdog callback */
static bool
phy_btcx_wd(phy_wd_ctx_t *ctx)
{
	phy_btcx_info_t *bi = (phy_btcx_info_t *)ctx;
	phy_info_t *pi = bi->priv->pi;

	phy_utils_phyreg_enter(pi);
	wlapi_update_bt_chanspec(pi->sh->physhim, pi->radio_chanspec,
	                         SCAN_INPROG_PHY(pi), RM_INPROG_PHY(pi));
	phy_utils_phyreg_exit(pi);

	return TRUE;
}

/* Bluetooth Coexistence Initialization */
static int
WLBANDINITFN(phy_btcx_init)(phy_init_ctx_t *ctx)
{
	phy_btcx_info_t *btcxi = (phy_btcx_info_t *)ctx;
	phy_type_btcx_fns_t *fns = btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->init_btcx != NULL) {
		return (fns->init_btcx)(fns->ctx);
	}
	return BCME_OK;
}

bool
phy_btcx_is_btactive(phy_btcx_info_t *cmn_info)
{
	return cmn_info->data->bt_active;
}

#if defined(BCMINTERNAL) || defined(WLTEST)
int
phy_btcx_get_preemptstatus(phy_info_t *pi, int32* ret_ptr)
{
	phy_type_btcx_fns_t *fns = pi->btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->get_preemptstatus != NULL) {
		return (fns->get_preemptstatus)(fns->ctx, ret_ptr);
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

#if (!defined(WLC_DISABLE_ACI) && defined(BCMLTECOEX) && defined(BCMINTERNAL))
int
phy_btcx_desense_ltecx(phy_info_t *pi, int32 mode)
{
	phy_type_btcx_fns_t *fns = pi->btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->desense_ltecx != NULL) {
		return (fns->desense_ltecx)(fns->ctx, mode);
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* !defined (WLC_DISABLE_ACI) && defined (BCMLTECOEX) && defined (BCMINTERNAL) */

#if !defined(WLC_DISABLE_ACI)
int
phy_btcx_desense_btc(phy_info_t *pi, int32 mode)
{
	phy_type_btcx_fns_t *fns = pi->btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->desense_btc != NULL) {
		return (fns->desense_btc)(fns->ctx, mode);
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* !defined(WLC_DISABLE_ACI) */

/* adjust phy setting based on bt states */
static bool
wlc_phy_btc_adjust(phy_wd_ctx_t *ctx)
{
	bool btactive = FALSE;
	uint16 btperiod = 0;
	phy_btcx_info_t *bi = (phy_btcx_info_t *)ctx;
	phy_info_t *pi = bi->priv->pi;
	phy_type_btcx_fns_t *fns = bi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (!(wlapi_bmac_btc_mode_get(pi->sh->physhim)))
		return TRUE;

	wlapi_bmac_btc_period_get(pi->sh->physhim, &btperiod, &btactive);

	if (btactive != bi->data->bt_active) {
		if (fns->adjust != NULL) {
			(fns->adjust)(fns->ctx, btactive);
		} else {
			PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
		}
	}

	bi->data->bt_period = btperiod;
	bi->data->bt_active = btactive;
	return TRUE;
}

static void phy_btcx_override_enable_legacy(phy_info_t *pi)
{
	/* This is required only for 2G operation. No BTCX in 5G */
	if ((pi->sh->machwcap & MCAP_BTCX) && CHSPEC_IS2G(pi->radio_chanspec)) {
		/* Ucode better be suspended when we mess with BTCX regs directly */
		ASSERT(!(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC));

		wlapi_coex_flush_a2dp_buffers(pi->sh->physhim);

		/* Enable manual BTCX mode */
		OR_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_CTRL,
			BTCX_CTRL_EN | BTCX_CTRL_SW);
		/* Force WLAN antenna and priority */
		OR_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_TRANS_CTRL,
			BTCX_TRANS_TXCONF | BTCX_TRANS_ANTSEL);
	}
}

void
wlc_btcx_override_enable(phy_info_t *pi)
{
	phy_btcx_info_t *btcxi = pi->btcxi;
	phy_type_btcx_fns_t *fns = btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->override_enable != NULL) {
		(fns->override_enable)(fns->ctx);
	} else {
		phy_btcx_override_enable_legacy(pi);
	}
}

static void phy_btcx_override_disable_legacy(phy_info_t *pi)
{
	if ((pi->sh->machwcap & MCAP_BTCX) && CHSPEC_IS2G(pi->radio_chanspec)) {
		/* Ucode better be suspended when we mess with BTCX regs directly */
		ASSERT(!(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC));
		/* Enable manual BTCX mode */
		OR_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_CTRL,
			BTCX_CTRL_EN | BTCX_CTRL_SW);
		/* Force BT priority */
		AND_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_TRANS_CTRL,
			~(BTCX_TRANS_TXCONF | BTCX_TRANS_ANTSEL));
	}
}

void
wlc_phy_btcx_override_disable(phy_info_t *pi)
{
	phy_btcx_info_t *btcxi = pi->btcxi;
	phy_type_btcx_fns_t *fns = btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->override_disable != NULL) {
		(fns->override_disable)(fns->ctx);
	} else {
		phy_btcx_override_disable_legacy(pi);
	}
}

void
wlc_phy_set_femctrl_bt_wlan_ovrd(wlc_phy_t *pih, int8 state)
{
	phy_info_t *pi = (phy_info_t *)pih;
	phy_type_btcx_fns_t *fns = pi->btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->set_femctrl != NULL) {
		(fns->set_femctrl)(fns->ctx, state, TRUE);
	}
}

int8
wlc_phy_get_femctrl_bt_wlan_ovrd(wlc_phy_t *pih)
{
	phy_info_t *pi = (phy_info_t *)pih;
	phy_type_btcx_fns_t *fns = pi->btcxi->priv->fns;
	int8 state = AUTO;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->get_femctrl != NULL) {
		state = (fns->get_femctrl)(fns->ctx);
	}
	return state;
}

#if (!defined(WL_SISOCHIP) && defined(SWCTRL_TO_BT_IN_COEX))
void wlc_phy_femctrl_mask_on_band_change(phy_info_t *pi)
{
	phy_type_btcx_fns_t *fns = pi->btcxi->priv->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->femctrl_mask != NULL) {
		(fns->femctrl_mask)(fns->ctx);
	}
}
#endif

void
wlc_phy_btcx_wlan_critical_enter(phy_info_t *pi)
{
	wlapi_bmac_mhf(pi->sh->physhim, MHF1, MHF1_WLAN_CRITICAL, MHF1_WLAN_CRITICAL, WLC_BAND_2G);
}

void
wlc_phy_btcx_wlan_critical_exit(phy_info_t *pi)
{
	wlapi_bmac_mhf(pi->sh->physhim, MHF1, MHF1_WLAN_CRITICAL, 0, WLC_BAND_2G);
}

int
wlc_phy_iovar_set_btc_restage_rxgain(phy_btcx_info_t *btcxi, int32 set_val)
{
	phy_type_btcx_fns_t *fns = btcxi->priv->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->set_restage_rxgain != NULL) {
		return (fns->set_restage_rxgain)(fns->ctx, set_val);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
		return BCME_UNSUPPORTED;
	}
}

int
wlc_phy_iovar_get_btc_restage_rxgain(phy_btcx_info_t *btcxi, int32 *ret_val)
{
	phy_type_btcx_fns_t *fns = btcxi->priv->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->get_restage_rxgain != NULL) {
		return (fns->get_restage_rxgain)(fns->ctx, ret_val);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
		return BCME_UNSUPPORTED;
	}
}
