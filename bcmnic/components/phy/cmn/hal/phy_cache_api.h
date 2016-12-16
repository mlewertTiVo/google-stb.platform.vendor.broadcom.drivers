/*
 * Cache module public interface (to MAC driver).
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
 * $Id: phy_cache_api.h 656063 2016-08-25 03:56:25Z $
 */

#ifndef _phy_cache_api_h_
#define _phy_cache_api_h_

#include <typedefs.h>
#include <phy_api.h>

#if defined(PHYCAL_CACHING)
void phy_cache_cal(phy_info_t *pi);
int phy_cache_restore_cal(phy_info_t *pi);
int wlc_phy_reuse_chanctx(wlc_phy_t *ppi, chanspec_t chanspec);
int wlc_phy_invalidate_chanctx(wlc_phy_t *ppi, chanspec_t chanspec);
void wlc_phy_update_chctx_glacial_time(wlc_phy_t *ppi, chanspec_t chanspec);
uint32 wlc_phy_get_current_cachedchans(wlc_phy_t *ppi);
void wlc_phy_get_all_cached_ctx(wlc_phy_t *ppi, chanspec_t *chanlist);
extern int	wlc_phy_cal_cache_init(wlc_phy_t *ppi);
extern void wlc_phy_cal_cache_deinit(wlc_phy_t *ppi);
extern void wlc_phy_cal_cache_set(wlc_phy_t *ppi, bool state);
extern bool wlc_phy_cal_cache_get(wlc_phy_t *ppi);
#endif /* PHYCAL_CACHING */

#endif /* _phy_cache_api_h_ */
