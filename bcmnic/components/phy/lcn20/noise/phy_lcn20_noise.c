/*
 * lcn20PHY Noise module implementation
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
 * $Id: phy_lcn20_noise.c 630449 2016-04-09 00:27:18Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_wd.h>
#include "phy_type_noise.h"
#include <phy_lcn20.h>
#include <phy_lcn20_noise.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn20.h>
/* TODO: all these are going away... > */
#endif

/* module private states */
struct phy_lcn20_noise_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_noise_info_t *ii;
};

/* local functions */
static void phy_lcn20_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[],
	uint8 extra_gain_1dB, int16 *tot_gain);
static void phy_lcn20_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init);
static bool phy_lcn20_noise_aci_wd(phy_wd_ctx_t *ctx);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int phy_lcn20_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_lcn20_noise_dump NULL
#endif

/* register phy type specific implementation */
phy_lcn20_noise_info_t *
BCMATTACHFN(phy_lcn20_noise_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_noise_info_t *ii)
{
	phy_lcn20_noise_info_t *info;
	phy_type_noise_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_noise_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_noise_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->ii = ii;

	if (CHIPID(pi->sh->chip) == BCM43142_CHIP_ID) {
		pi->sh->interference_mode = pi->sh->interference_mode_2G = WLAN_AUTO_W_NOISE;
	}
	/* Init value for interference mode */
	if ((CHIPID(pi->sh->chip) == BCM43018_CHIP_ID) ||
		(CHIPID(pi->sh->chip) == BCM43430_CHIP_ID)) {
		pi->sh->interference_mode = pi->sh->interference_mode_2G = WLAN_AUTO;
	}

	/* register watchdog fn */
	if (phy_wd_add_fn(pi->wdi, phy_lcn20_noise_aci_wd, info,
	                  PHY_WD_PRD_1TICK, PHY_WD_1TICK_NOISE_ACI,
	                  PHY_WD_FLAG_NONE) != BCME_OK) {
		PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
		goto fail;
	}

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.calc_fine = phy_lcn20_noise_calc_fine_resln;
	fns.mode = phy_lcn20_noise_set_mode;
	fns.dump = phy_lcn20_noise_dump;
	fns.ctx = info;

	phy_noise_register_impl(ii, &fns);

	return info;

	/* error handling */
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_noise_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_noise_unregister_impl)(phy_lcn20_noise_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_noise_info_t *ii = info->ii;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_noise_unregister_impl(ii);

	phy_mfree(pi, info, sizeof(phy_lcn20_noise_info_t));
}

static void
phy_lcn20_noise_calc_fine_resln(phy_type_noise_ctx_t *ctx, int16 cmplx_pwr_dbm[],
	uint8 extra_gain_1dB, int16 *tot_gain)
{
	phy_lcn20_noise_info_t *info = (phy_lcn20_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 i;
	BCM_REFERENCE(pi);

	FOREACH_CORE(pi, i) {
		/* Convert to analog input power at ADC and then
		 * backoff applied gain to get antenna-input referred power.
		 * NOTE: all the calculations below assume 0.25dB format.
		 */
		cmplx_pwr_dbm[i] += LCN20PHY_NOISE_PWR_TO_DBM;
		cmplx_pwr_dbm[i] -= tot_gain[i];
	}
}

/* set mode */
static void
phy_lcn20_noise_set_mode(phy_type_noise_ctx_t *ctx, int wanted_mode, bool init)
{
	phy_lcn20_noise_info_t *info = (phy_lcn20_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("%s: mode %d init %d\n", __FUNCTION__, wanted_mode, init));

	if (init) {
		pi->interference_mode_crs_time = 0;
		pi->crsglitch_prev = 0;
		if (wlc_lcn20phy_acimode_valid(pi, wanted_mode)) {
			wlc_lcn20phy_aci_init(pi);
		}
	}
	if (wlc_lcn20phy_acimode_valid(pi, wanted_mode)) {
		wlc_lcn20phy_aci_modes(pi, wanted_mode);
	}
}

/* watchdog callback */
static bool
phy_lcn20_noise_aci_wd(phy_wd_ctx_t *ctx)
{
	BCM_REFERENCE(ctx);
	return TRUE;
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int
phy_lcn20_noise_dump(phy_type_noise_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_lcn20_noise_info_t *info = (phy_lcn20_noise_info_t *)ctx;
	phy_info_t *pi = info->pi;

	return phy_noise_dump_common(pi, b);
}
#endif
