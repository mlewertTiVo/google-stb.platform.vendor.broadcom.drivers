/*
 * NPHY RADIO control module interface (to other PHY modules).
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
 * $Id: phy_n_radio.h 616484 2016-02-01 17:32:27Z $
 */

#ifndef _phy_n_radio_h_
#define _phy_n_radio_h_

#include <phy_api.h>
#include <phy_n.h>
#include <phy_radio.h>

/* forward declaration */
typedef struct phy_n_radio_info phy_n_radio_info_t;

/* register/unregister NPHY specific implementations to/from common */
phy_n_radio_info_t *phy_n_radio_register_impl(phy_info_t *pi,
	phy_n_info_t *ni, phy_radio_info_t *ri);
void phy_n_radio_unregister_impl(phy_n_radio_info_t *info);

uint32 phy_n_radio_query_idcode(phy_info_t *pi);
void wlc_nphy_get_radio_loft(phy_info_t *pi, uint8 *ei0, uint8 *eq0, uint8 *fi0, uint8 *fq0,
	uint8 *ei1, uint8 *eq1, uint8 *fi1, uint8 *fq1);
void wlc_nphy_set_radio_loft(phy_info_t *pi, uint8 ei0, uint8 eq0, uint8 fi0, uint8 fq0,
	uint8 ei1, uint8 eq1, uint8 fi1, uint8 fq1);
#endif /* _phy_n_radio_h_ */
