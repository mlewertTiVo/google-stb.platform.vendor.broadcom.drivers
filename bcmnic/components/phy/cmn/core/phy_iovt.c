/*
 * PHY Core module implementation - IOVarTable registration
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
 * $Id: phy_iovt.c 657044 2016-08-30 21:37:55Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_api.h>
#include "phy_iovt.h"
#include <phy_btcx_iov.h>
#include <phy_chanmgr_iov.h>
#include <phy_calmgr_iov.h>
#include <phy_hirssi_iov.h>
#include <phy_radar_iov.h>
#include <phy_temp_iov.h>
#include <phy_dsi_iov.h>
#include <phy_misc_iov.h>
#include <phy_noise_iov.h>
#include <phy_tpc_iov.h>
#include <phy_rxgcrs_iov.h>
#include <phy_antdiv_iov.h>
#include <phy_papdcal_iov.h>
#include <phy_rssi_iov.h>
#include <phy_rxspur_iov.h>
#include <phy_vcocal_iov.h>
#include <phy_tssical_iov.h>
#include <phy_txiqlocal_iov.h>
#include <wlc_iocv_types.h>
#include <phy_fcbs_iov.h>
#ifdef WLC_TXPWRCAP
#include <phy_txpwrcap_iov.h>
#endif
#ifdef IQPLAY_DEBUG
#include <phy_samp_iov.h>
#endif /* IQPLAY_DEBUG */
#ifdef RADIO_HEALTH_CHECK
#include <phy_hc_iov.h>
#endif /* RADIO_HEALTH_CHECK */

/* local functions */

#ifndef ALL_NEW_PHY_MOD
int phy_legacy_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);
#endif

/* Register all modules' iovar tables/handlers */
int
BCMATTACHFN(phy_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	ASSERT(ii != NULL);

#ifndef ALL_NEW_PHY_MOD
	/* Register legacy iovar table/handlers */
	if (phy_legacy_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_legacy_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}
#endif

#if defined(AP) && defined(RADAR)
	/* Register radar common iovar table/handlers */
	if (phy_radar_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_radar_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* Register TEMPerature sense common iovar table/handlers */
	if (phy_temp_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_temp_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

#ifdef WL_DSI
	/* Register DeepSleepInit modules common iovar table/handlers */
	if (phy_dsi_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_dsi_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}
#endif /* WL_DSI */

	/* Register  Miscellaneous module common iovar table/handlers */
	if (phy_misc_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_misc_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register TxPowerControl common iovar table/handlers */
	if (phy_tpc_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_tpc_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

#ifdef WLC_TXPWRCAP
	/* Register TxPowerCap common iovar table/handlers */
	if (phy_txpwrcap_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_txpwrcap_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* Register Rx Gain Control and Carrier Sense common iovar table/handlers */
	if (phy_rxgcrs_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxgcrs_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register TSSI CAL common iovar table/handlers */
	if (phy_tssical_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_tssical_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register ANTennaDIVersity common iovar tables/handlers */
	if (phy_antdiv_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_antdiv_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register PAPD Calibration common iovar tables/handlers */
	if (phy_papdcal_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_papdcal_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register VCO Calibration common iovar tables/handlers */
	if (phy_vcocal_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_vcocal_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register Channel Manager common iovar tables/handlers */
	if (phy_chanmgr_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_chanmgr_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register Cal Manager common iovar tables/handlers */
	if (phy_calmgr_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_calmgr_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register FCBS module common iovar table/handlers */
	if (phy_fcbs_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_fcbs_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register tx iqlo calibration module common iovar table/handlers */
	if (phy_txiqlocal_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_txiqlocal_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register hi rssi elna bypass module common iovar table/handlers */
	if (phy_hirssi_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_hirssi_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register bluetooth coexistence module common iovar table/handlers */
	if (phy_btcx_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_btcx_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register RSSICompute module common iovar table/handlers */
	if (phy_rssi_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_rssi_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register NOISEmeasure module common iovar table/handlers */
	if (phy_noise_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxspur_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register Rx Spur canceller module common iovar table/handlers */
	if (phy_rxspur_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxspur_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

#ifdef IQPLAY_DEBUG
	/* Register sample play  module common iovar table/handlers */
	if (phy_samp_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_samp_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}
#endif /* IQPLAY_DEBUG */

#ifdef RADIO_HEALTH_CHECK
	/* Register health check common iovar table/handlers */
	if (phy_hc_register_iovt(pi, ii) != BCME_OK) {
		PHY_ERROR(("%s: phy_hc_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}
#endif /* RADIO_HEALTH_CHECK */

	/* Register other modules' common iovar tables/dispatchers here ... */

	return BCME_OK;

fail:
	return BCME_ERROR;
}
