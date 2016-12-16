/*
 * Health check module.
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
 * $Id: phy_hc_api.h 659514 2016-09-14 20:19:00Z $
 */
#ifndef _phy_hc_api_h_
#define _phy_hc_api_h_

#include <typedefs.h>
#include <phy_api.h>
#include <phy_dbg_api.h>

typedef enum {
	PHY_HC_ALL		= 0,
	PHY_HC_TEMPSENSE	= 1,
	PHY_HC_VCOCAL		= 2,
	PHY_HC_RX		= 3,
	PHY_HC_TX		= 4,
	PHY_HC_LAST		/* This must be the last entry */
} phy_healthcheck_type_t;

int phy_hc_debugcrash_forcefail(wlc_phy_t *ppi, phy_crash_reason_t dctype);
int phy_hc_debugcrash_health_check(wlc_phy_t *ppi, phy_healthcheck_type_t hc);

#endif /* _phy_hc_api_h_ */
