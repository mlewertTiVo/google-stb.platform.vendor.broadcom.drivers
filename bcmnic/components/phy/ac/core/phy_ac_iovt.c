/*
 * ACPHY Core module implementation
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
 * $Id: phy_ac_iovt.c 647115 2016-07-04 01:33:05Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <wlc_iocv_types.h>
#include <phy_ac_chanmgr_iov.h>
#include <phy_ac_misc_iov.h>
#include <phy_ac_radio_iov.h>
#include <phy_ac_rssi_iov.h>
#include <phy_ac_rxgcrs_iov.h>
#include <phy_ac_tbl_iov.h>
#include <phy_ac_tpc_iov.h>
#include "phy_type_ac.h"
#include "phy_type_ac_iovt.h"

/* local functions */

/* register iovar tables/handlers to IOC module */
int
BCMATTACHFN(phy_ac_register_iovt)(phy_info_t *pi, phy_type_info_t *ti, wlc_iocv_info_t *ii)
{
	int err;

	/* Register Channel Manager module ACPHY iovar table/handlers */
	if ((err = phy_ac_chanmgr_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_chanmgr_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register Miscellaneous module ACPHY iovar table/handlers */
	if ((err = phy_ac_misc_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_misc_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register Radio control module ACPHY iovar table/handlers */
	if ((err = phy_ac_radio_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_radio_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register RSSICompute module ACPHY iovar table/handlers */
	if ((err = phy_ac_rssi_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_rssi_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register Rx Gain Control and Carrier Sense module ACPHY iovar table/handlers */
	if ((err = phy_ac_rxgcrs_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_rxgcrs_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register PHYTbl module ACPHY iovar table/handlers */
	if ((err = phy_ac_tbl_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_tbl_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register TxPowerControl module ACPHY iovar table/handlers */
	if ((err = phy_ac_tpc_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_tpc_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	return BCME_OK;

fail:
	return err;
}
