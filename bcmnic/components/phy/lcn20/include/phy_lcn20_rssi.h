/*
 * lcn20PHY RSSI Compute module interface (to other PHY modules)
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
 * $Id: phy_lcn20_rssi.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_lcn20_rssi_h_
#define _phy_lcn20_rssi_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_rssi.h>

/* forward declaration */
typedef struct phy_lcn20_rssi_info phy_lcn20_rssi_info_t;

/* register/unregister lcn20PHY specific implementations to/from common */
phy_lcn20_rssi_info_t *phy_lcn20_rssi_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_rssi_info_t *ri);
void phy_lcn20_rssi_unregister_impl(phy_lcn20_rssi_info_t *info);

extern int8 *phy_lcn20_pkt_rssi_gain_index_offset_2g;
#define phy_lcn20_get_pkt_rssi_gain_index_offset_2g(idx) \
	phy_lcn20_pkt_rssi_gain_index_offset_2g[idx]
#ifdef BAND5G
extern int8 *phy_lcn20_pkt_rssi_gain_index_offset_5g;
#define phy_lcn20_get_pkt_rssi_gain_index_offset_5g(idx) \
	phy_lcn20_pkt_rssi_gain_index_offset_5g[idx]
#endif

#endif /* _phy_lcn20_rssi_h_ */
