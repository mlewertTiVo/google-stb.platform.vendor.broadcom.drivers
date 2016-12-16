/*
 * ACPHY dccal module interface (to other PHY modules).
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
 * $Id: phy_ac_dccal.h 618979 2016-02-12 23:19:41Z $
 */

#ifndef _phy_ac_dccal_h_
#define _phy_ac_dccal_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_dccal.h>
#include <wlc_radioreg_20695.h>
#include <wlc_radioreg_20694.h>

/* forward declaration */
typedef struct phy_ac_dccal_info phy_ac_dccal_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_dccal_info_t *phy_ac_dccal_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_dccal_info_t *ani);
void phy_ac_dccal_unregister_impl(phy_ac_dccal_info_t *info);

void wlc_dcc_fsm_restart(phy_info_t *pi);
int wlc_phy_tiny_static_dc_offset_cal(phy_info_t *pi);
void wlc_dcc_fsm_reset(phy_info_t *pi);
void phy_ac_dccal(phy_info_t *pi);
void phy_ac_dccal_init(phy_info_t *pi);
void phy_ac_load_gmap_tbl(phy_info_t *pi);

/* Analog DCC sw-war related functions */
void acphy_dcc_idac_set(phy_info_t *pi, int16 dac, int ch, int core);
void acphy_search_idac_iq_est(phy_info_t *pi, uint8 core,
                                     int16 dac_min, int16 dac_max, int8 dac_step);
int acphy_analog_dc_cal_war(phy_info_t *pi);

#endif /* _phy_ac_dccal_h_ */
