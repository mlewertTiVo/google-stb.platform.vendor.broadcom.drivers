/*
 * ACPHY STF module interface
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
 * $Id: phy_ac_stf.h 659995 2016-09-16 22:23:48Z $
 */

#ifndef _phy_ac_stf_h_
#define _phy_ac_stf_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_stf.h>

/* forward declaration */
typedef struct phy_ac_stf_info phy_ac_stf_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_stf_info_t *phy_ac_stf_register_impl(phy_info_t *pi,
	phy_ac_info_t *ac_info, phy_stf_info_t *ri);
void phy_ac_stf_unregister_impl(phy_ac_stf_info_t *info);

/* Functions being used by other AC modules */

#endif /* _phy_ac_stf_h_ */
