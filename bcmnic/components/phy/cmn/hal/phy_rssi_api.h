/*
 * RSSICompute module public interface (to MAC driver).
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
 * $Id: phy_rssi_api.h 642720 2016-06-09 18:56:12Z vyass $
 */

#ifndef _phy_rssi_api_h_
#define _phy_rssi_api_h_

#include <typedefs.h>
#include <d11.h>
#include <phy_api.h>

/*
 * Compute rssi based on rxh info and save the result in wrxh.
 * Return the computed rssi value as well.
 */
int8 phy_rssi_compute_rssi(phy_info_t *pi, wlc_d11rxhdr_t *wrxh);

#if (defined(WLTEST) || defined(BCMINTERNAL))
void wlc_phy_pkteng_rxstats_update(wlc_phy_t *ppi, uint8 statidx);
#endif  /* (defined(WLTEST) || defined (BCMINTERNAL)) */

#endif /* _phy_rssi_api_h_ */
