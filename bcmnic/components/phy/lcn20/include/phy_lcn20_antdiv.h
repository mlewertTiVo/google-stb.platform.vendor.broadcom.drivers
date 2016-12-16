/*
 * lcn20PHY ANTennaDIVersity module interface (to other PHY modules).
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
 * $Id: phy_lcn20_antdiv.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_lcn20_antdiv_h_
#define _phy_lcn20_antdiv_h_

#include <phy_api.h>
#include <phy_lcn20.h>
#include <phy_antdiv.h>

/* forward declaration */
typedef struct phy_lcn20_antdiv_info phy_lcn20_antdiv_info_t;

/* register/unregister phy type specific implementation */
phy_lcn20_antdiv_info_t *phy_lcn20_antdiv_register_impl(phy_info_t *pi,
	phy_lcn20_info_t *lcn20i, phy_antdiv_info_t *di);
void phy_lcn20_antdiv_unregister_impl(phy_lcn20_antdiv_info_t *di);

/* set ant */
void phy_lcn20_antdiv_set_rx(phy_lcn20_antdiv_info_t *di, uint8 ant);

/* enable/disable? */
void phy_lcn20_swdiv_epa_pd(phy_lcn20_antdiv_info_t *di, bool diable);

#endif /* _phy_lcn20_antdiv_h_ */
