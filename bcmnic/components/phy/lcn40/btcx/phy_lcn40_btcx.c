/*
 * LCN40PHY BT Coex module implementation
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
 * $Id: phy_lcn40_btcx.c 639978 2016-05-25 16:03:11Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_btcx.h"
#include <phy_lcn40.h>
#include <phy_lcn40_btcx.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_phyreg_lcn40.h>
#include <wlc_phy_int.h>

#include <phy_utils_reg.h>
#include <wlc_phy_lcn40.h>
#include <wlc_phytbl_lcn40.h>
#include <phy_lcn40_tpc.h>

/* module private states */
struct phy_lcn40_btcx_info {
	phy_info_t			*pi;
	phy_lcn40_info_t	*lcn40i;
	phy_btcx_info_t		*cmn_info;

/* add other variable size variables here at the end */
};

/* local functions */
static void wlc_lcn40phy_btc_adjust(phy_type_btcx_ctx_t *ctx, bool btactive);

/* register phy type specific implementation */
phy_lcn40_btcx_info_t *
BCMATTACHFN(phy_lcn40_btcx_register_impl)(phy_info_t *pi, phy_lcn40_info_t *lcn40i,
	phy_btcx_info_t *cmn_info)
{
	phy_lcn40_btcx_info_t *lcn40_info;
	phy_type_btcx_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((lcn40_info = phy_malloc(pi, sizeof(phy_lcn40_btcx_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	lcn40_info->pi = pi;
	lcn40_info->lcn40i = lcn40i;
	lcn40_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.adjust = wlc_lcn40phy_btc_adjust;
	fns.ctx = lcn40_info;

	if (phy_btcx_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_btcx_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* PHY-Feature specific parameter initialization */


	return lcn40_info;

	/* error handling */
fail:
	if (lcn40_info != NULL)
		phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_btcx_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn40_btcx_unregister_impl)(phy_lcn40_btcx_info_t *lcn40_info)
{
	phy_info_t *pi;
	phy_btcx_info_t *cmn_info;

	ASSERT(lcn40_info);
	pi = lcn40_info->pi;
	cmn_info = lcn40_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_btcx_unregister_impl(cmn_info);

	phy_mfree(pi, lcn40_info, sizeof(phy_lcn40_btcx_info_t));
}

static void
wlc_lcn40phy_btc_adjust(phy_type_btcx_ctx_t *ctx, bool btactive)
{
	phy_lcn40_btcx_info_t *info = (phy_lcn40_btcx_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int btc_mode = wlapi_bmac_btc_mode_get(pi->sh->physhim);
	bool suspend = !(R_REG(pi->sh->osh, &((phy_info_t *)pi)->regs->maccontrol) & MCTL_EN_MAC);
	phy_lcn40_info_t *lcn40i = pi->u.pi_lcn40phy;

	if ((CHIPID(pi->sh->chip) == BCM43142_CHIP_ID) && (btc_mode != WL_BTC_DISABLE)) {
		if (!suspend)
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		if (btactive) {
			pi->sh->interference_mode = 0;
			wlc_lcn40phy_aci_init(pi);
		} else {
			pi->sh->interference_mode = pi->sh->interference_mode_2G;
			wlc_lcn40phy_aci_init(pi);
		}
		if (!suspend)
			wlapi_enable_mac(pi->sh->physhim);
		suspend =
			!(R_REG(pi->sh->osh, &((phy_info_t *)pi)->regs->maccontrol) & MCTL_EN_MAC);
	}

	/* for dual antenna design, a gain table switch is to ensure good performance
	 * in simultaneous WLAN RX
	 */
	if (pi->aa2g > 2) {
		if (!suspend)
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		if (LCN40REV_IS(pi->pubpi->phy_rev, 3)) {
			if (btactive && !info->cmn_info->data->bt_active) {
				phy_lcn40_antdiv_set_rx(lcn40i->antdivi, 0);
				PHY_REG_MOD(pi, LCN40PHY, RFOverride0, ant_selp_ovr, 1);
				PHY_REG_MOD(pi, LCN40PHY, RFOverrideVal0, ant_selp_ovr_val, 0);
				wlc_lcn40phy_write_table(pi, dot11lcn40_gain_tbl_2G_info_rev3_BT);
			} else if (!btactive && info->cmn_info->data->bt_active) {
				phy_lcn40_antdiv_set_rx(lcn40i->antdivi, pi->sh->rx_antdiv);
				PHY_REG_MOD(pi, LCN40PHY, RFOverride0, ant_selp_ovr, 0);
				wlc_lcn40phy_write_table(pi, dot11lcn40_gain_tbl_2G_info_rev3);
			}
		}
		if ((btc_mode == WL_BTC_PARALLEL) || (btc_mode == WL_BTC_LITE)) {
			if (btactive) {
				pi->u.pi_lcn40phy->btc_clamp = TRUE;
				wlc_lcn40phy_set_txpwr_clamp(pi);
			} else if (pi->u.pi_lcn40phy->btc_clamp) {
				pi->u.pi_lcn40phy->btc_clamp = FALSE;
				wlc_phy_txpower_recalc_target_lcn40phy(pi);
			}
		}
		if (!suspend)
			wlapi_enable_mac(pi->sh->physhim);
	}

}
