/*
 * TSSI CAL module internal interface - iovar table/handlers
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
 * $Id: phy_tssical_iov.h 657820 2016-09-02 18:26:33Z $
 */

#ifndef _phy_tssical_iov_t_
#define _phy_tssical_iov_t_

#include <phy_api.h>

/* iovar ids */
enum {
	IOV_TSSIVISI_THRESH = 1,
	IOV_OLPC_THRESH = 2,
	IOV_OLPC_ANCHOR_2G = 3,
	IOV_OLPC_ANCHOR_5G = 4,
	IOV_DISABLE_OLPC = 5,
	IOV_OLPC_ANCHOR_IDX = 6,
	IOV_OLPC_IDX_VALID = 7,
	IOV_PHY_TXCAL_STATUS = 8,
	IOV_PHY_ADJUSTED_TSSI = 9,
	IOV_OLPC_OFFSET = 10
};

/* register iovar table/handlers */
int phy_tssical_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_tssical_iov_t_ */
