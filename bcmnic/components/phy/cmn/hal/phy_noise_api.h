/*
 * Noise module public interface (to MAC driver).
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
 * $Id: phy_noise_api.h 630449 2016-04-09 00:27:18Z vyass $
 */

#ifndef _phy_noise_api_h_
#define _phy_noise_api_h_

#include <typedefs.h>
#include <phy_api.h>

#ifndef WLC_DISABLE_ACI
void wlc_phy_interf_rssi_update(wlc_phy_t *pi, chanspec_t chanNum, int8 leastRSSI);
#endif
/* HWACI Interrupt Handler */
void wlc_phy_hwaci_mitigate_intr(wlc_phy_t *pih);
#endif /* _phy_noise_api_h_ */
