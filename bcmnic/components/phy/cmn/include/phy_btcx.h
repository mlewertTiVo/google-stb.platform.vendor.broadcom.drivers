/*
 * BlueToothCoExistence module internal interface (to other PHY modules).
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
 * $Id: phy_btcx.h 642720 2016-06-09 18:56:12Z vyass $
 */

#ifndef _phy_btcx_h_
#define _phy_btcx_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_btcx_info phy_btcx_info_t;

/* attach/detach */
phy_btcx_info_t *phy_btcx_attach(phy_info_t *pi);
void phy_btcx_detach(phy_btcx_info_t *ri);
void wlc_btcx_override_enable(phy_info_t *pi);
void wlc_phy_btcx_override_disable(phy_info_t *pi);
void wlc_phy_btcx_wlan_critical_enter(phy_info_t *pi);
void wlc_phy_btcx_wlan_critical_exit(phy_info_t *pi);
bool phy_btcx_is_btactive(phy_btcx_info_t *cmn_info);
int wlc_phy_iovar_set_btc_restage_rxgain(phy_btcx_info_t *btcxi, int32 set_val);
int wlc_phy_iovar_get_btc_restage_rxgain(phy_btcx_info_t *btcxi, int32 *ret_val);
#if defined(BCMINTERNAL) || defined(WLTEST)
int phy_btcx_get_preemptstatus(phy_info_t *pi, int32* ret_ptr);
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
#if (!defined(WLC_DISABLE_ACI) && defined(BCMLTECOEX) && defined(BCMINTERNAL))
int phy_btcx_desense_ltecx(phy_info_t *pi, int32 mode);
#endif /* !defined (WLC_DISABLE_ACI) && defined (BCMLTECOEX) && defined (BCMINTERNAL) */
#if !defined(WLC_DISABLE_ACI)
int phy_btcx_desense_btc(phy_info_t *pi, int32 mode);
#endif /* !defined(WLC_DISABLE_ACI) */
#endif /* _phy_btcx_h_ */
