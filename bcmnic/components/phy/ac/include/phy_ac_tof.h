/*
 * ACPHY TOF module interface
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
 * $Id: phy_ac_tof.h 635757 2016-05-05 05:18:39Z vyass $
 */
#ifndef _phy_ac_tof_h_
#define _phy_ac_tof_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_tof.h>

/* forward declaration */
typedef struct phy_ac_tof_info phy_ac_tof_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_tof_info_t *phy_ac_tof_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_tof_info_t *ri);
void phy_ac_tof_unregister_impl(phy_ac_tof_info_t *info);

#if defined(WL_PROXDETECT)
void phy_ac_tof_tbl_offset(phy_ac_tof_info_t *tofi, uint32 id, uint32 offset);
#endif /* defined(WL_PROXDETECT) */
#endif /* _phy_ac_tof_h_ */
