/*
 * ACPHY HiRSSI eLNA Bypass module interface (to other PHY modules).
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
 * $Id: phy_ac_hirssi.h 655464 2016-08-19 23:20:17Z $
 */

#ifndef _phy_ac_hirssi_h_
#define _phy_ac_hirssi_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_hirssi.h>

/* forward declaration */
typedef struct phy_ac_hirssi_info phy_ac_hirssi_info_t;

/* register/unregister phy type specific implementation */
phy_ac_hirssi_info_t *phy_ac_hirssi_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_hirssi_info_t *hirssii);
void phy_ac_hirssi_unregister_impl(phy_ac_hirssi_info_t *info);
bool phy_ac_hirssi_shmem_read_clear(phy_info_t *pi);
void phy_ac_hirssi_set_ucode_params(phy_info_t *pi);
void phy_ac_hirssi_set_timer(phy_info_t *pi);
bool phy_ac_hirssi_set(phy_info_t *pi);
bool phy_ac_hirssi_get(phy_info_t *pi);

#endif /* _phy_ac_hirssi_h_ */
