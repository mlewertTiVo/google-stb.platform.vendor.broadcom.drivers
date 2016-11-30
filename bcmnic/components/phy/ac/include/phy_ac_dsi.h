/*
 * ACPHY DeepSleepInit module interface
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
 * $Id: phy_ac_dsi.h 599661 2015-11-16 10:58:55Z nahegde $
 */

#ifndef _phy_ac_dsi_h_
#define _phy_ac_dsi_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_dsi.h>

/* forward declaration */
typedef struct phy_ac_dsi_info phy_ac_dsi_info_t;
phy_ac_dsi_info_t *phy_ac_dsi_register_impl(phy_info_t *pi, phy_ac_info_t *aci, phy_dsi_info_t *ri);
void phy_ac_dsi_unregister_impl(phy_ac_dsi_info_t *info);

void dsi_update_dyn_seq(phy_info_t *pi);

/* 4339 ProtoWork */
extern void dsi_save_ACMAJORREV_1(phy_info_t *pi);
extern void dsi_restore_ACMAJORREV_1(phy_info_t *pi);
extern void dsi_populate_addr_ACMAJORREV_1(phy_info_t *pi);
#endif /* _phy_ac_dsi_h_ */
