/*
 * TEMPerature sense module internal interface (to other PHY modules).
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
 * $Id: phy_temp.h 630449 2016-04-09 00:27:18Z vyass $
 */

#ifndef _phy_temp_h_
#define _phy_temp_h_
#define SENSE_TEMP 0
#define SENSE_PALDO 1
#define PHY_TEMPSENSE_MIN 0
#define PHY_TEMPSENSE_MAX 105

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_temp_info phy_temp_info_t;

/* attach/detach */
phy_temp_info_t *phy_temp_attach(phy_info_t *pi);
void phy_temp_detach(phy_temp_info_t *ri);

/* temp. throttle */
uint16 phy_temp_throttle(phy_temp_info_t *ti);

#ifdef	WL_DYNAMIC_TEMPSENSE
int phy_temp_get_cur_temp(phy_temp_info_t *ti);
int phy_temp_get_temp_thresh(phy_temp_info_t *ti);
#if defined(BCMDBG) || defined(WLTEST)
int phy_temp_get_override(phy_temp_info_t *ti);
#endif /* BCMDBG || WLTEST */
#endif /* WL_DYNAMIC_TEMPSENSE */
void wlc_phy_upd_gain_wrt_temp_phy(phy_info_t *pi, int16 *gain_err_temp_adj);
void wlc_phy_upd_gain_wrt_gain_cal_temp_phy(phy_info_t *pi, int16 *gain_err_temp_adj);
#endif /* _phy_temp_h_ */
