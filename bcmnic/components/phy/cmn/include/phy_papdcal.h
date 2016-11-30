/*
 * PAPD CAL module interface (to other PHY modules).
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
 * $Id: phy_papdcal.h 639713 2016-05-24 18:02:57Z vyass $
 */

#ifndef _phy_papdcal_h_
#define _phy_papdcal_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_papdcal_info phy_papdcal_info_t;

/* attach/detach */
phy_papdcal_info_t *phy_papdcal_attach(phy_info_t *pi);
void phy_papdcal_detach(phy_papdcal_info_t *cmn_info);

bool phy_papdcal_epacal(phy_papdcal_info_t *papdcali);

/* Changed from IS_WFD_PHY_LL_ENABLE */
bool phy_papdcal_is_wfd_phy_ll_enable(phy_papdcal_info_t *papdcali);

/* EPAPD */
bool phy_papdcal_epapd(phy_papdcal_info_t *papdcali);

#ifndef WLC_DISABLE_PAPD_SUPPORT
#define PHY_PAPDEN(pi)	(PHY_IPA(pi) || phy_papdcal_epapd(pi->papdcali))
#else
#define PHY_PAPDEN(pi)	0
#endif

#if defined(WFD_PHY_LL)
int phy_papdcal_set_wfd_ll_enable(phy_papdcal_info_t *papdcali, uint8 int_val);
int phy_papdcal_get_wfd_ll_enable(phy_papdcal_info_t *papdcali, int32 *ret_int_ptr);
#endif /* WFD_PHY_LL */
#if defined(WLTEST) || defined(BCMDBG)
void phy_papdcal_epa_dpd_set(phy_info_t *pi, uint8 enab_epa_dpd, bool in_2g_band);
#endif /* defined(WLTEST) || defined(BCMDBG) */
#if defined(BCMINTERNAL) || defined(WLTEST)
int phy_papdcal_get_lut_idx0(phy_info_t *pi, int32* idx);
int phy_papdcal_get_lut_idx1(phy_info_t *pi, int32* idx);
int phy_papdcal_set_idx(phy_info_t *pi, int8 idx);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
#ifndef ATE_BUILD
int phy_papdcal_set_skip(phy_info_t *pi, uint8 skip);
int phy_papdcal_get_skip(phy_info_t *pi, int32* skip);
#endif /* !ATE_BUILD */
#endif /* BCMINTERNAL || WLTEST || DBG_PHY_IOV || WFD_PHY_LL_DEBUG */
#endif /* _phy_papdcal_h_ */
