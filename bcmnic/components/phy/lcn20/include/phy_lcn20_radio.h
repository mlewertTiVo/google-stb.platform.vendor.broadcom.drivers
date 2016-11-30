/*
 * lcn20PHY RADIO control module interface (to other PHY modules).
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
 * $Id: phy_lcn20_radio.h 616484 2016-02-01 17:32:27Z guangjie $
 */

#ifndef _phy_lcn20_radio_h_
#define _phy_lcn20_radio_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_radio.h>

/* forward declaration */
typedef struct phy_lcn20_radio_info phy_lcn20_radio_info_t;

/* register/unregister lcn20PHY specific implementations to/from common */
phy_lcn20_radio_info_t *phy_lcn20_radio_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_radio_info_t *ri);
void phy_lcn20_radio_unregister_impl(phy_lcn20_radio_info_t *info);

void phy_lcn20_radio_switch(phy_lcn20_radio_info_t *info, bool on);

uint32 phy_lcn20_radio_query_idcode(phy_info_t *pi);
void phy_lcn20_radio_parse_idcode(phy_info_t *pi, uint32 idcode);

void wlc_phy_get_radio_loft_lcn20phy(phy_info_t *pi, uint8 *ei0,
	uint8 *eq0, uint8 *fi0, uint8 *fq0);
#endif /* _phy_lcn20_radio_h_ */
