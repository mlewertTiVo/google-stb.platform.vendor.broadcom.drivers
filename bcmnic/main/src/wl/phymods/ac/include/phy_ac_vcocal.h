/*
 * ACPHY VCO CAL module interface (to other PHY modules).
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#ifndef _phy_ac_vcocal_h_
#define _phy_ac_vcocal_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_vcocal.h>

/* vco calcode cache address for ucode pm mode */
#define VCOCAL_MAINCAP_OFFSET_20693   14
#define VCOCAL_SECONDCAP_OFFSET_20693 18
#define VCOCAL_AUXCAP0_OFFSET_20693   22
#define VCOCAL_AUXCAP1_OFFSET_20693   26

#ifdef BCM7271
#include <wl_bcm7xxx_dbg.h>
#undef PHY_ERROR
#define PHY_ERROR(args) do {BCM7XXX_TRACE(); Bcm7xxxPrintSpace(NULL); printf args;} while (0)
#endif /* BCM7271 */

/* forward declaration */
typedef struct phy_ac_vcocal_info phy_ac_vcocal_info_t;
typedef struct _acphy_vcocal_radregs_t {
	uint16 clk_div_ovr1;
	uint16 clk_div_cfg1;
	bool is_orig;
} acphy_vcocal_radregs_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_vcocal_info_t *phy_ac_vcocal_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_vcocal_info_t *mi);
void phy_ac_vcocal_unregister_impl(phy_ac_vcocal_info_t *info);


/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
extern void wlc_phy_radio_tiny_vcocal(phy_info_t *pi);
extern void wlc_phy_radio_tiny_vcocal_wave2(phy_info_t *pi,
	uint8 vcocal_mode, uint8 pll_mode, uint8 coupling_mode, bool cache_calcode);
extern void wlc_phy_radio_tiny_afe_resynch(phy_info_t *pi, uint8 mode);
extern void wlc_phy_radio2069_vcocal(phy_info_t *pi);
extern void wlc_phy_radio2069x_vcocal_isdone(phy_info_t *pi, bool set_delay, bool cache_calcode);
extern void wlc_phy_20696_radio_vcocal(phy_info_t *pi, uint8 cal_mode, uint8 coupling_mode);
extern void wlc_phy_radio20696_vcocal_isdone(phy_info_t *pi, bool set_delay, bool cache_calcode);

/* 20696 vco cal */
#define VCO_CAL_MODE_20696		     0
#define VCO_CAL_COUPLING_MODE_20696	 0

#endif /* _phy_ac_vcocal_h_ */
