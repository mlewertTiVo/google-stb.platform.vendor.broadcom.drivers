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

typedef struct phy_ac_ocl_data {
	uint16 ocl_disable_reqs;
	uint8 ocl_coremask;
} phy_ac_ocl_data_t;

#ifdef OCL
/* register/unregister ACPHY specific implementations to/from common */
phy_ac_ocl_info_t *phy_ac_ocl_register_impl(phy_info_t *pi,
	phy_ac_info_t *ac_info, phy_ocl_info_t *ri);
void phy_ac_ocl_unregister_impl(phy_ac_ocl_info_t *info);

/* Functions being used by other AC modules */
void phy_ac_ocl_config(phy_ac_ocl_info_t *ocli);
phy_ac_ocl_data_t *phy_ac_ocl_get_data(phy_ac_ocl_info_t *ocli);
#endif /* OCL */

#endif /* _phy_ac_ocl_h_ */
