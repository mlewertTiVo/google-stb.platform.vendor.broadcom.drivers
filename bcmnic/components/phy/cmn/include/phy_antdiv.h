/*
 * ANTennaDIVersity module interface (to other PHY modules).
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
 * $Id: phy_antdiv.h 629393 2016-04-05 06:55:25Z gpasrija $
 */

#ifndef _phy_antdiv_h_
#define _phy_antdiv_h_

#include <phy_api.h>

/* forward declaration */
typedef struct phy_antdiv_info phy_antdiv_info_t;

/* attach/detach */
phy_antdiv_info_t *phy_antdiv_attach(phy_info_t *pi);
void phy_antdiv_detach(phy_antdiv_info_t *di);
void phy_antdiv_set_sw_control(phy_info_t *pi, int8 divOvrride, int core);
void phy_antdiv_get_sw_control(phy_info_t *pi, int32 *ret_int_ptr, int core);

#ifdef WLC_SW_DIVERSITY
uint16 phy_antdiv_get_swctrl_mask(phy_antdiv_info_t *di);
#endif

#endif /* _phy_antdiv_h_ */
