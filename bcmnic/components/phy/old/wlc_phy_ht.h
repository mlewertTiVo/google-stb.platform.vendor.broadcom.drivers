/*
 * ABGPHY module header file
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
 * $Id: wlc_phy_ht.h 606042 2015-12-14 06:21:23Z jqliu $
 */

#ifndef _wlc_phy_ht_h_
#define _wlc_phy_ht_h_

#include <typedefs.h>

#include <phy_ht_info.h>

/* Make following function public to phy_ht_cache.c file can access it */
void wlc_phy_cal_txiqlo_coeffs_htphy(phy_info_t *pi,
	uint8 rd_wr, uint16 *coeffs, uint8 select, uint8 core);
void wlc_phy_txpwrctrl_set_idle_tssi_htphy(phy_info_t *pi, int8 idle_tssi, uint8 core);

/* **************************** REMOVE ************************** */
void wlc_phy_get_initgain_dB_htphy(phy_info_t *pi, int16 *initgain_dB);

#endif /* _wlc_phy_ht_h_ */
