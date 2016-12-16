/*
 * Tssical module.
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
 * $Id: phy_lcn20_tssical.h 657820 2016-09-02 18:26:33Z $
 */

#ifndef _phy_lcn20_tssical_h_
#define _phy_lcn20_tssical_h_
#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_tssical.h>

/* forward declaration */
typedef struct phy_lcn20_tssical_info phy_lcn20_tssical_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_lcn20_tssical_info_t *phy_lcn20_tssical_register_impl(phy_info_t *pi,
		phy_lcn20_info_t *lcn20i, phy_tssical_info_t *tssicali);
void phy_lcn20_tssical_unregister_impl(phy_lcn20_tssical_info_t *info);

#ifdef WLC_TXCAL
int wlc_phy_txcal_generate_estpwr_lut_lcn20phy(wl_txcal_power_tssi_t
		*txcal_pwr_tssi, uint32 *estpwr, uint8 core);
#endif /* WLC_TXCAL */

#endif /* _phy_lcn20_tssical_h_ */
