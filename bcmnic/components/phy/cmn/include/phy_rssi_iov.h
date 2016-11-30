/*
 * RSSICompute module internal interface - iovar table registration
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
 * $Id: $
 */

#ifndef _phy_rssi_iov_h_
#define _phy_rssi_iov_h_

#include <phy_api.h>

/* iovar ids */
enum {
	IOV_PHY_RSSI_GAIN_DELTA_2G = 1,
	IOV_PHY_RSSI_GAIN_DELTA_2GH = 2,
	IOV_PHY_RSSI_GAIN_DELTA_2GHH = 3,
	IOV_PHY_RSSI_GAIN_DELTA_5GL = 4,
	IOV_PHY_RSSI_GAIN_DELTA_5GML = 5,
	IOV_PHY_RSSI_GAIN_DELTA_5GMU = 6,
	IOV_PHY_RSSI_GAIN_DELTA_5GH = 7,
	IOV_PKTENG_STATS = 8,
	IOV_PHY_RSSI_CAL_FREQ_GRP_2G = 9,
	IOV_PHY_RSSI_GAIN_DELTA_2GB0 = 10,
	IOV_PHY_RSSI_GAIN_DELTA_2GB1 = 11,
	IOV_PHY_RSSI_GAIN_DELTA_2GB2 = 12,
	IOV_PHY_RSSI_GAIN_DELTA_2GB3 = 13,
	IOV_PHY_RSSI_GAIN_DELTA_2GB4 = 14
};

/* register iovar table/handlers */
int phy_rssi_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_rssi_iov_h_ */
