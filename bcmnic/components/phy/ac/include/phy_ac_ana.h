/*
 * ACPHY ANAcore control module interface (to other PHY modules).
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
 * $Id: phy_ac_ana.h 644994 2016-06-22 06:23:44Z $
 */

#ifndef _phy_ac_ana_h_
#define _phy_ac_ana_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_ana.h>

#define FCBGA_43012 0
#define WLBGA_43012 1
/* forward declaration */
typedef struct phy_ac_ana_info phy_ac_ana_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_ana_info_t *phy_ac_ana_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_ana_info_t *ani);
void phy_ac_ana_unregister_impl(phy_ac_ana_info_t *info);

extern uint16 sdadc_cfg20;
extern uint16 sdadc_cfg40;
/* High SNR 40M */
extern uint16 sdadc_cfg40hs;
extern uint16 sdadc_cfg80;

extern void wlc_phy_init_adc_read(phy_info_t *pi, uint16 *save_afePuCtrl, uint16 *save_gpio,
	uint32 *save_chipc, uint16 *fval2g_orig, uint16 *fval5g_orig,
                                  uint16 *fval2g, uint16 *fval5g, uint8 *stall_val,
                                  uint16 *save_gpioHiOutEn);

extern void wlc_phy_restore_after_adc_read(phy_info_t *pi, uint16 *save_afePuCtrl,
	uint16 *save_gpio, uint32 *save_chipc,
	uint16 *fval2g_orig, uint16 *fval5g_orig,
                                           uint16 *fval2g, uint16 *fval5g, uint8 *stall_val,
                                           uint16 *save_gpioHiOutEn);

extern void wlc_phy_pulse_adc_reset_acphy(phy_info_t *pi);
extern void wlc_tiny_dc_static_WAR(phy_info_t *pi);
extern void wlc_phy_write_femctrl_table(phy_ac_ana_info_t *ani);
#ifdef SWCTRL_TO_BT_IN_COEX
extern uint16 wlc_phy_set_mask_for_femctrl10(phy_info_t *pi);
#endif
void wlc_phy_set_regtbl_on_femctrl(phy_info_t *pi);
void wlc_acphy_paldo_change(phy_info_t *pi);
#endif /* _phy_ac_ana_h_ */
