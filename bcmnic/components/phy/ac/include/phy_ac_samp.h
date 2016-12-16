/*
 * ACPHY Sample Collect module interface (to other PHY modules).
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
 * $Id: phy_ac_samp.h 655756 2016-08-23 12:40:40Z $
 */

#ifndef _phy_ac_samp_h_
#define _phy_ac_samp_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_samp.h>
#include <phy_type_samp.h>

/* Macros for sample play */
#ifdef IQPLAY_DEBUG
#define SAMPLE_COLLECT_PLAY_CTRL_PLAY_MODE_SHIFT	10
#define START_IDX_ADDR	65536
#define AXI_BASE_ADDR	0xE8000000U
#define MAIN_AUX_CORE_ADDR_OFFSET	0x800000U
#define BM_BASE_OFFFSET_ADDR	0x00400000U
#endif /* IQPLAY_DEBUG */

/* forward declaration */
typedef struct phy_ac_samp_info phy_ac_samp_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_samp_info_t *phy_ac_samp_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_samp_info_t *cmn_info);
void phy_ac_samp_unregister_impl(phy_ac_samp_info_t *ac_info);

/* ************************************** */
/* ACPHY sample collect intermodule api's */
/* ************************************** */
#ifdef WL_PROXDETECT
void acphy_set_sc_startptr(phy_info_t *pi, uint32 start_idx);
void acphy_set_sc_stopptr(phy_info_t *pi, uint32 stop_idx);
#endif /* WL_PROXDETECT */
#ifdef SAMPLE_COLLECT
void phy_ac_lpf_hpc_override(phy_ac_info_t *aci, bool setup);
#endif /* SAMPLE_COLLECT */
#endif /* _phy_ac_samp_h_ */
