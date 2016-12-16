/*
 * ANTennaDIVersity module config.
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
 * $Id: phy_antdiv_cfg.h 659179 2016-09-13 03:45:44Z $
 */

#ifndef _phy_antdiv_cfg_h_
#define _phy_antdiv_cfg_h_

#include <phy_antdiv_api.h>

struct phy_swdiv {
	bool swdiv_enable;
	uint8 swdiv_gpio_num;
	uint8 swdiv_gpio_ctrl;
	wlc_swdiv_swctrl_t swdiv_swctrl_en;
	uint16 swdiv_swctrl_mask;
	uint16 swdiv_swctrl_ant0;
	uint16 swdiv_swctrl_ant1;
	uint16 swdiv_coreband_map;	/* backward compatibility */
	uint8 swdiv_antmap2g_main;
	uint8 swdiv_antmap5g_main;
	uint8 swdiv_antmap2g_aux;
	uint8 swdiv_antmap5g_aux;
};
typedef struct phy_swdiv phy_swdiv_t;

#ifdef WLC_SW_DIVERSITY
void phy_swdiv_read_srom(phy_info_t *pi, phy_swdiv_t *swdiv);
#endif

#endif /* _phy_antdiv_cfg_h_ */
