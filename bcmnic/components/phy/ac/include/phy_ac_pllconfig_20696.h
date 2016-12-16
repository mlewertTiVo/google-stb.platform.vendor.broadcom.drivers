/*
 * ACPHY 20696 Radio PLL configuration
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: phy_ac_pllconfig_20696.h 625800 2016-03-17 20:12:06Z $
 */

#ifndef _PHY_AC_20696_PLLCONFIG_H
#define _PHY_AC_20696_PLLCONFIG_H

extern void
wlc_phy_radio20696_pll_tune(phy_info_t *pi, uint32 chan_freq);
extern int
BCMATTACHFN(phy_ac_radio20696_populate_pll_config_tbl)(phy_info_t *pi);
extern void
BCMATTACHFN(phy_ac_radio20696_populate_pll_config_mfree)(phy_info_t *pi);

#endif /* _PHY_AC_20696_PLLCONFIG_H */
