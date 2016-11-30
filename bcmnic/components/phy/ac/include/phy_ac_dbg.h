/*
 * ACPHY Debug module interface
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
 * $Id: phy_ac_dbg.h 589186 2015-09-28 14:24:26Z jqliu $
 */

#ifndef _phy_ac_dbg_h_
#define _phy_ac_dbg_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_dbg.h>

/* forward declaration */
typedef struct phy_ac_dbg_info phy_ac_dbg_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_dbg_info_t *phy_ac_dbg_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_dbg_info_t *di);
void phy_ac_dbg_unregister_impl(phy_ac_dbg_info_t *info);

#endif /* _phy_ac_dbg_h_ */
