/*
 * HTPHY TEMPerature sense module implementation
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
 * $Id: phy_ht_temp.c 630449 2016-04-09 00:27:18Z vyass $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_temp.h"
#include "phy_temp_st.h"
#include <phy_ht.h>
#include <phy_ht_temp.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_ht.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_ht_temp_info {
	phy_info_t *pi;
	phy_ht_info_t *hti;
	phy_temp_info_t *ti;
};

/* local functions */
static uint16 phy_ht_temp_throttle(phy_type_temp_ctx_t *ctx);
static int phy_ht_temp_get(phy_type_temp_ctx_t *ctx);
static void phy_ht_temp_upd_gain(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj);

/* Register/unregister HTPHY specific implementation to common layer */
phy_ht_temp_info_t *
BCMATTACHFN(phy_ht_temp_register_impl)(phy_info_t *pi, phy_ht_info_t *hti, phy_temp_info_t *ti)
{
	phy_ht_temp_info_t *info;
	phy_type_temp_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_ht_temp_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_ht_temp_info_t));
	info->pi = pi;
	info->hti = hti;
	info->ti = ti;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.throt = phy_ht_temp_throttle;
	fns.get = phy_ht_temp_get;
	fns.upd_gain = phy_ht_temp_upd_gain;
	fns.ctx = info;

	phy_temp_register_impl(ti, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ht_temp_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ht_temp_unregister_impl)(phy_ht_temp_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_temp_info_t *ti = info->ti;


	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_temp_unregister_impl(ti);

	phy_mfree(pi, info, sizeof(phy_ht_temp_info_t));
}

static uint16
phy_ht_temp_throttle(phy_type_temp_ctx_t *ctx)
{
	phy_ht_temp_info_t *info = (phy_ht_temp_info_t *)ctx;
	phy_temp_info_t *ti = info->ti;
	phy_info_t *pi = info->pi;
	phy_txcore_temp_t *temp;
	uint8 txcore_shutdown_lut[] = {1, 1, 2, 1, 4, 1, 2, 5};
	uint8 phyrxchain = pi->sh->phyrxchain;
	uint8 phytxchain = pi->sh->phytxchain;
	uint8 new_phytxchain;
	int16 currtemp;

	PHY_TRACE(("%s\n", __FUNCTION__));

	ASSERT(phytxchain);

	temp = phy_temp_get_st(ti);
	ASSERT(temp != NULL);

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	currtemp = wlc_phy_tempsense_htphy(pi);
	wlapi_enable_mac(pi->sh->physhim);

#ifdef BCMDBG
	if (pi->tempsense_override)
		currtemp = pi->tempsense_override;
#endif

	if (!temp->heatedup) {
		if (currtemp >= temp->disable_temp) {
			new_phytxchain = txcore_shutdown_lut[phytxchain];
			temp->heatedup = TRUE;
			temp->bitmap = ((phyrxchain << 4) | new_phytxchain);
		}
	} else {
		if (currtemp <= temp->enable_temp) {
			new_phytxchain = pi->sh->hw_phytxchain;
			temp->heatedup = FALSE;
			temp->bitmap = ((phyrxchain << 4) | new_phytxchain);
		}
	}

	return temp->bitmap;
}

/* read the current temperature */
static int
phy_ht_temp_get(phy_type_temp_ctx_t *ctx)
{
	phy_ht_temp_info_t *info = (phy_ht_temp_info_t *)ctx;
	phy_ht_info_t *hti = info->hti;

	return hti->current_temperature;
}

static void
phy_ht_temp_upd_gain(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj)
{
	phy_ht_temp_info_t *info = (phy_ht_temp_info_t *)ctx;
	phy_info_t *pi = info->pi;

	/* read in the temperature */
	int16 temp_diff, curr_temp = 0, gain_temp_slope = 0;
	curr_temp = pi->u.pi_htphy->current_temperature;
	curr_temp = MIN(MAX(curr_temp, PHY_TEMPSENSE_MIN), PHY_TEMPSENSE_MAX);

	/* check that non programmed SROM for cal temp are not changed */
	if (pi->srom_rawtempsense != 255) {
		temp_diff = curr_temp - pi->srom_rawtempsense;
	} else {
		temp_diff = 0;
	}

	/* adjust gain based on the temperature difference now vs. calibration time:
	 * make gain diff rounded to nearest 0.25 dbm, where 1 tick is 0.25 dbm
	 */
	gain_temp_slope = CHSPEC_IS2G(pi->radio_chanspec) ?
			HTPHY_GAIN_VS_TEMP_SLOPE_2G : HTPHY_GAIN_VS_TEMP_SLOPE_5G;


	if (temp_diff >= 0) {
		*gain_err_temp_adj = (temp_diff * gain_temp_slope*2 + 25)/50;
	} else {
		*gain_err_temp_adj = (temp_diff * gain_temp_slope*2 - 25)/50;
	}
}
