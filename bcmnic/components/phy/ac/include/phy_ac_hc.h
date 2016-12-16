/*
 * Health check module.
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
 * $Id: phy_ac_hc.h 656120 2016-08-25 08:17:57Z $
 */
#ifndef _phy_ac_hc_h_
#define _phy_ac_hc_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_hc.h>

/* forward declaration */
typedef struct phy_ac_hc_info phy_ac_hc_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_hc_info_t *phy_ac_hc_register_impl(phy_info_t *pi,
		phy_ac_info_t *aci, phy_hc_info_t *di);
void phy_ac_hc_unregister_impl(phy_ac_hc_info_t *info);

#endif /* _phy_ac_hc_h_ */
