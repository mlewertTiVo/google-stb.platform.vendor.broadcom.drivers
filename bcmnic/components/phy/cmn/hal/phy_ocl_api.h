/*
 * OCL module public interface (to MAC driver).
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef _phy_ocl_api_h_
#define _phy_ocl_api_h_

#include <typedefs.h>
#include <phy_api.h>

/* OCL disable request ids */
#define WLC_OCL_REQ_HOST		0
#define WLC_OCL_REQ_RSSI		1
#define WLC_OCL_REQ_LTEC		2
#define WLC_OCL_REQ_RXCHAIN		3
#define WLC_OCL_REQ_CAL			4
#define WLC_OCL_REQ_CAL_TXBF		5
#define WLC_OCL_REQ_CM			6
#define WLC_OCL_REQ_CHANSWITCH		7
#define WLC_OCL_REQ_ASPEND		8

int phy_ocl_coremask_change(wlc_phy_t *ppi, uint8 coremask);
uint8 phy_ocl_get_coremask(wlc_phy_t *ppi);
int phy_ocl_status_get(wlc_phy_t *ppi, uint16 *reqs, uint8 *coremask, bool *ocl_en);
int phy_ocl_disable_req_set(wlc_phy_t *ppi, uint16 req, bool disable, uint8 req_id);
#endif /* _phy_ocl_api_h_ */
