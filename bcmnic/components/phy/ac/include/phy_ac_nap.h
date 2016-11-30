/*
 * ACPHY Napping module interface (to other PHY modules).
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

#ifndef _phy_ac_nap_h_
#define _phy_ac_nap_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_nap.h>

/* forward declaration */
typedef struct phy_ac_nap_info phy_ac_nap_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_nap_info_t *phy_ac_nap_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_nap_info_t *cmn_info);
void phy_ac_nap_unregister_impl(phy_ac_nap_info_t *ac_info);
bool phy_ac_nap_is_enabled(phy_ac_nap_info_t *nap_info);

#ifdef WL_NAP
void phy_ac_config_napping_28nm_ulp(phy_info_t *pi);
void phy_ac_nap_ed_thresh_cal(phy_info_t *pi, int8 *cmplx_pwr_dBm);
void phy_ac_nap_enable(phy_info_t *pi, bool enable, bool agc_reconfig);
#endif /* WL_NAP */
#endif /* _phy_ac_nap_h_ */
