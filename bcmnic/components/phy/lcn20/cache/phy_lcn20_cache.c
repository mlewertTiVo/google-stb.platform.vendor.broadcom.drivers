/*
 * LCN20PHY Cache module implementation
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
 * $Id: phy_lcn20_cache.c 606042 2015-12-14 06:21:23Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_cache.h"
#include <phy_lcn20_cache.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <phy_utils_reg.h>

/* module private states */
struct phy_lcn20_cache_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_cache_info_t *ci;
};

/* local functions */
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_PHYDUMP)
/** dump calibration regs/info */
static void
wlc_phy_cal_dump_lcn20phy(phy_type_cache_ctx_t * cache_ctx, struct bcmstrbuf *b);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */

/* register phy type specific implementation */
phy_lcn20_cache_info_t *
BCMATTACHFN(phy_lcn20_cache_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_cache_info_t *ci)
{
	phy_lcn20_cache_info_t *info;
	phy_type_cache_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_cache_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_cache_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->ci = ci;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = info;
#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_PHYDUMP)
	fns.dump_cal = wlc_phy_cal_dump_lcn20phy;
#endif

	if (phy_cache_register_impl(ci, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_cache_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return info;

	/* error handling */
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_cache_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_cache_unregister_impl)(phy_lcn20_cache_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_cache_info_t *ci = info->ci;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_cache_unregister_impl(ci);

	phy_mfree(pi, info, sizeof(phy_lcn20_cache_info_t));
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP) || defined(BCMDBG_PHYDUMP)
/* dump calibration regs/info */
static void
wlc_phy_cal_dump_lcn20phy(phy_type_cache_ctx_t * cache_ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = (phy_info_t *)cache_ctx;
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	if (!pi->sh->up) {
		return;
	}
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);
#ifdef LCN20_PAPD_ENABLE
	PHY_PAPD(("calling wlc_phy_papd_dump_eps_trace_lcn20\n"));
	wlc_phy_papd_dump_eps_trace_lcn20(pi, b);
	bcm_bprintf(b, "papdcalidx 000\n");
#endif /* LCN20_PAPD_ENABLE */

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}
#endif  
