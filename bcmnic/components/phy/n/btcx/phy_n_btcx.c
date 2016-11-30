/*
 * NPHY BT Coex module implementation
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
 * $Id: phy_n_btcx.c 642720 2016-06-09 18:56:12Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_btcx.h"
#include <phy_n.h>
#include <phy_n_btcx.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_phyreg_n.h>
#include <wlc_phy_int.h>

#include <phy_utils_reg.h>
#include <phy_n_info.h>


/* module private states */
struct phy_n_btcx_info {
	phy_info_t			*pi;
	phy_n_info_t		*ni;
	phy_btcx_info_t		*cmn_info;

/* add other variable size variables here at the end */
};

/* local functions */
static void wlc_nphy_btc_adjust(phy_type_btcx_ctx_t *ctx, bool btactive);
static void phy_n_btcx_override_enable(phy_type_btcx_ctx_t *ctx);
static void phy_n_btcx_override_disable(phy_type_btcx_ctx_t *ctx);
static int phy_n_btcx_set_restage_rxgain(phy_type_btcx_ctx_t *ctx, int32 set_val);

/* register phy type specific implementation */
phy_n_btcx_info_t *
BCMATTACHFN(phy_n_btcx_register_impl)(phy_info_t *pi, phy_n_info_t *ni,
	phy_btcx_info_t *cmn_info)
{
	phy_n_btcx_info_t *n_info;
	phy_type_btcx_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((n_info = phy_malloc(pi, sizeof(phy_n_btcx_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	n_info->pi = pi;
	n_info->ni = ni;
	n_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.adjust = wlc_nphy_btc_adjust;
	fns.override_enable = phy_n_btcx_override_enable;
	fns.override_disable = phy_n_btcx_override_disable;
	fns.set_restage_rxgain = phy_n_btcx_set_restage_rxgain;
	fns.ctx = n_info;

	if (phy_btcx_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_btcx_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* PHY-Feature specific parameter initialization */


	return n_info;

	/* error handling */
fail:
	if (n_info != NULL)
		phy_mfree(pi, n_info, sizeof(phy_n_btcx_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_btcx_unregister_impl)(phy_n_btcx_info_t *n_info)
{
	phy_info_t *pi;
	phy_btcx_info_t *cmn_info;

	ASSERT(n_info);
	pi = n_info->pi;
	cmn_info = n_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_btcx_unregister_impl(cmn_info);

	phy_mfree(pi, n_info, sizeof(phy_n_btcx_info_t));
}

void
phy_n_btcx_init(phy_n_btcx_info_t *n_info)
{
	/* Need to reset the pi handler for bt_active
	 * to catch the bt_active transition inside
	 * wlc_nphy_btc_adjust() after band switch
	 */
	if ((CHIPID(n_info->pi->sh->chip) == BCM4324_CHIP_ID) &&
		(CHIPREV(n_info->pi->sh->chiprev) == 2)) {
			n_info->cmn_info->data->bt_active = FALSE;
	}
}

static void
wlc_nphy_btc_adjust(phy_type_btcx_ctx_t *ctx, bool btactive)
{
	phy_n_btcx_info_t *info = (phy_n_btcx_info_t *)ctx;
	phy_info_t *pi = info->pi;

	if ((CHIPID(pi->sh->chip) == BCM4324_CHIP_ID) &&
		(CHIPREV(pi->sh->chiprev) == 2)) {

		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			int btc_mode = wlapi_bmac_btc_mode_get(pi->sh->physhim);
			bool suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);

			if (!btc_mode)
				return;

			if (!suspend)
				wlapi_suspend_mac_and_wait(pi->sh->physhim);

			if (btactive && !info->cmn_info->data->bt_active) {
				/* bump LNLDO1 */
				wlc_phy_lnldo1_war_nphy(pi, 1, 3); /* 1.275 V (3) */
				/* trigger cals */
				wlc_phy_trigger_cals_for_btc_adjust(pi);
				/* set flag to indicate lnldo1 is bumped up */
				pi->nphy_btc_lnldo_bump = TRUE;
			} else if (!btactive && info->cmn_info->data->bt_active) {
				/* restore LNLDO1 */
				wlc_phy_lnldo1_war_nphy(pi, 1, 1); /* 1.225 V (1) */
				/* trigger cals */
				wlc_phy_trigger_cals_for_btc_adjust(pi);
				/* reset flag to indicate lnldo1 is restored */
				pi->nphy_btc_lnldo_bump = FALSE;
			}

			if (!suspend)
				wlapi_enable_mac(pi->sh->physhim);
		} else {
			/* clear the bump flag here */
			pi->nphy_btc_lnldo_bump = FALSE;
		}
	}
}

static void
phy_n_btcx_override_enable(phy_type_btcx_ctx_t *ctx)
{
	phy_n_btcx_info_t *info = (phy_n_btcx_info_t *)ctx;
	phy_info_t *pi = info->pi;

	/* This is required only for 2G operation. No BTCX in 5G */
	if ((pi->sh->machwcap & MCAP_BTCX) && CHSPEC_IS2G(pi->radio_chanspec)) {
		/* Ucode better be suspended when we mess with BTCX regs directly */
		ASSERT(!(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC));

		wlapi_coex_flush_a2dp_buffers(pi->sh->physhim);

		/* Enable manual BTCX mode */
		OR_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_CTRL, BTCX_CTRL_EN | BTCX_CTRL_SW);
		/* Force WLAN antenna and priority */
		OR_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_TRANS_CTRL,
			BTCX_TRANS_TXCONF | BTCX_TRANS_ANTSEL);

		/* SWWLAN-30288 For 4324x ucode is not setting clb_rf_sw_ctrl_mask with value
		 * needed for WLAN to  have control whenever BT->WLAN priority switch happens,
		 * so forcing it use correct value
		 */
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3)) {
			pi->btcxi->data->saved_clb_sw_ctrl_mask = phy_utils_read_phyreg(pi,
			NPHY_REV19_clb_rf_sw_ctrl_mask_ctrl);
			phy_utils_write_phyreg(pi, NPHY_REV19_clb_rf_sw_ctrl_mask_ctrl,
				LCNXN_SWCTRL_MASK_DEFAULT);
		}
	}
}

static void
phy_n_btcx_override_disable(phy_type_btcx_ctx_t *ctx)
{
	phy_n_btcx_info_t *info = (phy_n_btcx_info_t *)ctx;
	phy_info_t *pi = info->pi;

	if ((pi->sh->machwcap & MCAP_BTCX) && CHSPEC_IS2G(pi->radio_chanspec)) {
		/* Ucode better be suspended when we mess with BTCX regs directly */
		ASSERT(!(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC));

		/* SWWLAN-30288 For 4324x ucode is not setting clb_rf_sw_ctrl_mask with value
		 * needed for WLAN to have control whenever BT->WLAN priority switch happens
		 * so it was forced to wlan prefered value, so restoring the correct value
		 * for BT to have control
		 */
		if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3)) {
			phy_utils_write_phyreg(pi, NPHY_REV19_clb_rf_sw_ctrl_mask_ctrl,
				pi->btcxi->data->saved_clb_sw_ctrl_mask);
		}

		/* Enable manual BTCX mode */
		OR_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_CTRL, BTCX_CTRL_EN | BTCX_CTRL_SW);
		/* Force BT priority */
		AND_REG(pi->sh->osh, &pi->regs->PHYREF_BTCX_TRANS_CTRL,
			~(BTCX_TRANS_TXCONF | BTCX_TRANS_ANTSEL));
	}
}

static int
phy_n_btcx_set_restage_rxgain(phy_type_btcx_ctx_t *ctx, int32 set_val)
{
	phy_n_btcx_info_t *btcxi = (phy_n_btcx_info_t *)ctx;
	phy_info_t *pi = btcxi->pi;

	if (CHIPID_4324X_MEDIA_FAMILY(pi)) {

		const uint8 num_dev_thres = 2;	/* Make configurable later */
		uint16 num_bt_devices;

		/* pi->bt_shm_addr is set by phy_init */
		if (pi->bt_shm_addr == 0) {
			return BCME_NOTUP;
		}

		/* Read number of BT task and desense configuration for SHM */
		num_bt_devices	= wlapi_bmac_read_shm(pi->sh->physhim,
			pi->bt_shm_addr + M_BTCX_NUM_TASKS_OFFSET(pi));

		/* If more than num_dev_thres (two) tasks, no interference override
		 * and not already in manual ACI override mode
		 */
		if ((num_dev_thres > 0) && (num_bt_devices >= num_dev_thres) &&
			(!pi->sh->interference_mode_override) &&
			(pi->sh->interference_mode != WLAN_MANUAL)) {

			wlc_phy_set_interference_override_mode(pi, WLAN_MANUAL);

		} else if ((num_bt_devices < num_dev_thres) &&
			(pi->sh->interference_mode_override)) {

			wlc_phy_set_interference_override_mode(pi, INTERFERE_OVRRIDE_OFF);
		}
		/* Return "not BCME_OK" so that this function will be called every time
		 * wlc_btcx_watchdog is called
		 */
		return BCME_BUSY;
	} else {
		return BCME_UNSUPPORTED;
	}
}
