/*
 * Rx Gain Control and Carrier Sense module interface (to other PHY modules).
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
 * $Id: phy_rxgcrs.h 644994 2016-06-22 06:23:44Z vyass $
 */

#ifndef _phy_rxgcrs_h_
#define _phy_rxgcrs_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_rxgcrs_info phy_rxgcrs_info_t;

/* attach/detach */
phy_rxgcrs_info_t *phy_rxgcrs_attach(phy_info_t *pi);
void phy_rxgcrs_detach(phy_rxgcrs_info_t *cmn_info);

/* down */
int phy_rxgcrs_down(phy_rxgcrs_info_t *cmn_info);

uint8 wlc_phy_get_locale(phy_rxgcrs_info_t *info);
int wlc_phy_adjust_ed_thres(phy_info_t *pi, int32 *assert_thresh_dbm, bool set_threshold);
/* Rx desense Module */
#if defined(RXDESENS_EN)
int phy_rxgcrs_get_rxdesens(phy_info_t *pi, int32 *ret_int_ptr);
int phy_rxgcrs_set_rxdesens(phy_info_t *pi, int32 int_val);
#endif
int wlc_phy_set_rx_gainindex(phy_info_t *pi, int32 gain_idx);
int wlc_phy_get_rx_gainindex(phy_info_t *pi, int32 *gain_idx);
#ifndef ATE_BUILD
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
int wlc_phy_iovar_forcecal_noise(phy_info_t *pi, void *a, bool set);
#endif /* BCMINTERNAL || WLTEST || DBG_PHY_IOV || WFD_PHY_LL_DEBUG */
#endif /* !ATE_BUILD */
#endif /* _phy_rxgcrs_h_ */
