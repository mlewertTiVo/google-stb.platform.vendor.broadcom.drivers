/*
 * ACPHY OCL module interface
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
 * $Id: $
 */

#ifndef _phy_ac_ocl_h_
#define _phy_ac_ocl_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_ocl.h>

/* forward declaration */

typedef struct phy_ac_ocl_info phy_ac_ocl_info_t;

#ifdef OCL
/* register/unregister ACPHY specific implementations to/from common */
phy_ac_ocl_info_t *phy_ac_ocl_register_impl(phy_info_t *pi,
	phy_ac_info_t *ac_info, phy_ocl_info_t *ri);
void phy_ac_ocl_unregister_impl(phy_ac_ocl_info_t *info);

void wlc_phy_ocl_enable_acphy(phy_info_t *pi, bool enable);
void wlc_phy_ocl_apply_coremask_acphy(phy_info_t *pi);
void wlc_phy_ocl_config_acphy(phy_info_t *pi);
void wlc_phy_ocl_config_rfseq_phy(phy_info_t *pi);
void wlc_phy_ocl_config_phyreg(phy_info_t *pi);
void wlc_phy_set_ocl_crs_peak_thresh(phy_info_t *pi);

void wlc_phy_ocl_apply_coremask_28nm(phy_info_t *pi);
void wlc_phy_ocl_config_28nm(phy_info_t *pi);
void wlc_phy_set_ocl_crs_peak_thresh_28nm(phy_info_t *pi);

#endif /* OCL */

#endif /* _phy_ac_ocl_h_ */
