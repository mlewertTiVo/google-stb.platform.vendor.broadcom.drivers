/*
 *  BlueToothCoExistence module public interface (to MAC driver).
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
 * $Id: phy_btcx_api.h 624485 2016-03-11 20:47:51Z vyass $
 */

#ifndef _phy_btcx_api_h_
#define _phy_btcx_api_h_

#include <typedefs.h>
#include <phy_api.h>

void wlc_phy_set_femctrl_bt_wlan_ovrd(wlc_phy_t *pih, int8 state);
int8 wlc_phy_get_femctrl_bt_wlan_ovrd(wlc_phy_t *pih);
#if (!defined(WL_SISOCHIP) && defined(SWCTRL_TO_BT_IN_COEX))
void wlc_phy_femctrl_mask_on_band_change(phy_info_t *pi);
#endif

#endif /* _phy_btcx_api_h_ */
