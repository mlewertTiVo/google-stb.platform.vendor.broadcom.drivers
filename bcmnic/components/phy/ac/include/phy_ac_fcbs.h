/*
 * ACPHY FCBS module interface (to other PHY modules).
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
 * $Id: phy_ac_fcbs.h 633216 2016-04-21 20:17:37Z vyass $
 */

#ifndef _phy_ac_fcbs_h_
#define _phy_ac_fcbs_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_fcbs.h>

/* forward declaration */
typedef struct phy_ac_fcbs_info phy_ac_fcbs_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_fcbs_info_t *phy_ac_fcbs_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_fcbs_info_t *cmn_info);
void phy_ac_fcbs_unregister_impl(phy_ac_fcbs_info_t *ac_info);
#ifdef ENABLE_FCBS
bool wlc_phy_prefcbsinit_acphy(phy_info_t *pi, int chanidx);
#endif /* ENABLE_FCBS */
#endif /* _phy_ac_fcbs_h_ */
