/*
 * ACPHY TxPowerCap module interface (to other PHY modules).
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
 * $Id: phy_ac_txpwrcap.h 629423 2016-04-05 11:12:48Z pieterpg $
 */

#ifndef _phy_ac_txpwrcap_h_
#define _phy_ac_txpwrcap_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_txpwrcap.h>

/* forward declaration */
typedef struct phy_ac_txpwrcap_info phy_ac_txpwrcap_info_t;

#ifdef WLC_TXPWRCAP
/* register/unregister ACPHY specific implementations to/from common */
phy_ac_txpwrcap_info_t *phy_ac_txpwrcap_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_txpwrcap_info_t *ti);
void phy_ac_txpwrcap_unregister_impl(phy_ac_txpwrcap_info_t *info);

void wlc_phy_txpwrcap_set_acphy(phy_info_t *pi);
int8 wlc_phy_txpwrcap_tbl_get_max_percore_acphy(phy_info_t *pi, uint8 core);

#endif /* WLC_TXPWRCAP */
#endif /* _phy_ac_txpwrcap_h_ */
