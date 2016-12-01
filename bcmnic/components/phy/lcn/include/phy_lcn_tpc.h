/*
 * LCNPHY TxPowerCtrl module interface (to other PHY modules).
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
 * $Id: phy_lcn_tpc.h 583048 2015-08-31 16:43:34Z jqliu $
 */

#ifndef _phy_lcn_tpc_h_
#define _phy_lcn_tpc_h_

#include <phy_api.h>
#include <phy_lcn.h>
#include <phy_tpc.h>

/* forward declaration */
typedef struct phy_lcn_tpc_info phy_lcn_tpc_info_t;

/* register/unregister LCNPHY specific implementations to/from common */
phy_lcn_tpc_info_t *phy_lcn_tpc_register_impl(phy_info_t *pi,
	phy_lcn_info_t *lcni, phy_tpc_info_t *ri);
void phy_lcn_tpc_unregister_impl(phy_lcn_tpc_info_t *info);

#endif /* _phy_lcn_tpc_h_ */
