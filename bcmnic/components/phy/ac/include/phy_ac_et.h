/*
 * ACPHY Envelope Tracking module interface (to other PHY modules).
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
 * $Id$
 */

#ifndef _phy_ac_et_h_
#define _phy_ac_et_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_et.h>

/* forward declaration */
typedef struct phy_ac_et_info phy_ac_et_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_et_info_t *phy_ac_et_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_et_info_t *cmn_info);
void phy_ac_et_unregister_impl(phy_ac_et_info_t *info);

extern int phy_ac_et(phy_info_t *pi);
#endif /* _phy_ac_et_h_ */
