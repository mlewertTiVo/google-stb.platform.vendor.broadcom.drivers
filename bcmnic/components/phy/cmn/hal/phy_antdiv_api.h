/*
 * ANTennaDIVersity module public interface (to MAC driver).
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
 * $Id: phy_antdiv_api.h 659179 2016-09-13 03:45:44Z $
 */

#ifndef _phy_antdiv_api_h_
#define _phy_antdiv_api_h_

#include <typedefs.h>
#include <phy_api.h>

/* set/get */
int phy_antdiv_set_rx(phy_info_t *pi, uint8 ant);
void phy_antdiv_get_rx(phy_info_t *pi, uint8 *ant);

#ifdef WLC_SW_DIVERSITY
void phy_antdiv_set_swdiv_ant(wlc_phy_t *ppi, uint8 ant);
uint8 phy_antdiv_get_swdiv_ant(wlc_phy_t *ppi);
#endif

/* FEM, Antenna Selection/Diversity Module */
extern int phy_antdiv_antsel_type_set(wlc_phy_t *ppi, uint8 antsel_type);

typedef enum _wlc_swdiv_swctrl_t {
	SWDIV_SWCTRL_0,   /* Diversity switch controlled via GPIO */
	SWDIV_SWCTRL_1,   /* Diversity switch controlled via LCN40PHY_swctrlOvr_val */
	SWDIV_SWCTRL_2    /* Diversity switch controlled via LCN40PHY_RFOverrideVal0 */
} wlc_swdiv_swctrl_t;

#endif /* _phy_antdiv_api_h_ */
