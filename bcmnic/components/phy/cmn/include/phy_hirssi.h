/*
 * HiRSSI eLNA Bypass module internal interface (to other PHY modules).
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
 * $Id: phy_hirssi.h 617885 2016-02-09 00:18:54Z jkoster $
 */

#ifndef _phy_hirssi_h_
#define _phy_hirssi_h_

#include <typedefs.h>
#include <phy_api.h>

/* HiRSSI options */
typedef enum phy_hirssi {
	PHY_HIRSSI_BYP,
	PHY_HIRSSI_RES
} phy_hirssi_t;

/* forward declaration */
typedef struct phy_hirssi_info phy_hirssi_info_t;

/* attach/detach */
phy_hirssi_info_t *phy_hirssi_attach(phy_info_t *pi);
void phy_hirssi_detach(phy_hirssi_info_t *info);

/* functions */
int phy_hirssi_get_period(phy_info_t *pi, int32 *period);
int phy_hirssi_set_period(phy_info_t *pi, int32 period);
int phy_hirssi_get_en(phy_info_t *pi, int32 *enable);
int phy_hirssi_set_en(phy_info_t *pi, int32 enable);
int phy_hirssi_get_rssi(phy_info_t *pi, int32 *rssi, phy_hirssi_t opt);
int phy_hirssi_set_rssi(phy_info_t *pi, int32 rssi, phy_hirssi_t opt);
int phy_hirssi_get_cnt(phy_info_t *pi, int32 *cnt, phy_hirssi_t opt);
int phy_hirssi_set_cnt(phy_info_t *pi, int32 cnt, phy_hirssi_t opt);
int phy_hirssi_get_status(phy_info_t *pi, int32 *status);

#endif /* _phy_hirssi_h_ */
