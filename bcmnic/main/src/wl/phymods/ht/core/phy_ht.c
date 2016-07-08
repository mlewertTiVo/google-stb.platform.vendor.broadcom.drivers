/*
 * HTPHY Core module implementation
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_rstr.h>

#include <phy_ht_ana.h>
#include <phy_ht_radio.h>
#include <phy_ht_tbl.h>
#include <phy_ht_tpc.h>
#include <phy_ht_radar.h>
#include <phy_ht_calmgr.h>
#include <phy_ht_noise.h>
#include <phy_ht_temp.h>
#include <phy_ht_rssi.h>
#include <phy_ht_txiqlocal.h>
#include <phy_ht_rxiqcal.h>
#include <phy_ht_papdcal.h>
#include <phy_ht_vcocal.h>

#include "phy_type.h"
#include "phy_type_ht.h"
#include "phy_type_ht_iovt.h"
#include "phy_type_ht_ioct.h"
#include "phy_shared.h"
#include <phy_ht.h>

#include <phy_utils_radio.h>
#include <phy_utils_var.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_ht.h>
#include "wlc_phyreg_ht.h"
/* TODO: all these are going away... > */
#endif

#define PHY_TXPWR_MIN_HTPHY	8	/* for htphy devices */

/* local functions */
static int phy_ht_attach_ext(phy_info_t *pi, int bandtype);
static int phy_ht_register_impl(phy_info_t *pi, phy_type_info_t *ti, int bandtype);
static void phy_ht_unregister_impl(phy_info_t *pi, phy_type_info_t *ti);
static void phy_ht_reset_impl(phy_info_t *pi, phy_type_info_t *ti);
#define	phy_ht_dump_phyregs	NULL

/* attach/detach */
phy_type_info_t *
BCMATTACHFN(phy_ht_attach)(phy_info_t *pi, int bandtype)
{
	phy_ht_info_t *hti;
	phy_type_fns_t fns;
	uint32 idcode;

	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	if (phy_ht_attach_ext(pi, bandtype) != BCME_OK) {
		PHY_ERROR(("%s: phy_ht_attach_ext failed\n", __FUNCTION__));
		return NULL;
	}

	/* read idcode */
	idcode = phy_ht_radio_query_idcode(pi);
	PHY_TRACE(("%s: idcode 0x%08x\n", __FUNCTION__, idcode));
	/* parse idcode */
	phy_utils_parse_idcode(pi, idcode);
	/* validate radio id */
	if (phy_utils_valid_radio(pi) != BCME_OK) {
		PHY_ERROR(("%s: phy_utils_valid_radio failed\n", __FUNCTION__));
		return NULL;
	}

	/* TODO: move the htphy attach code to here... */
	if (wlc_phy_attach_htphy(pi) == FALSE) {
		PHY_ERROR(("%s: wlc_phy_attach_htphy failed\n", __FUNCTION__));
		return NULL;
	}
	hti = pi->u.pi_htphy;
	hti->pi = pi;

	/* register PHY type implementation entry points */
	bzero(&fns, sizeof(fns));
	fns.reg_impl = phy_ht_register_impl;
	fns.unreg_impl = phy_ht_unregister_impl;
	fns.reset_impl = phy_ht_reset_impl;
	fns.reg_iovt = phy_ht_register_iovt;
	fns.reg_ioct = phy_ht_register_ioct;
	fns.dump_phyregs = phy_ht_dump_phyregs;
	fns.ti = (phy_type_info_t *)hti;

	phy_register_impl(pi, &fns);

	return (phy_type_info_t *)hti;
}

void
BCMATTACHFN(phy_ht_detach)(phy_type_info_t *ti)
{
	phy_ht_info_t *hti = (phy_ht_info_t *)ti;
	phy_info_t *pi = hti->pi;

	phy_mfree(pi, hti, sizeof(phy_ht_info_t));
}

/* extension to phy_init */
static int
BCMATTACHFN(phy_ht_attach_ext)(phy_info_t *pi, int bandtype)
{
	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	pi->pubpi.phy_corenum = PHY_CORE_NUM_3;

	pi->aci_exit_check_period = 15;

	/* minimum reliable txpwr target is 8 dBm/mimo, 9dBm/lcn40, 10 dbm/legacy  */
	pi->min_txpower = PHY_TXPWR_MIN_HTPHY;

	pi->phynoise_polling = FALSE;

	return BCME_OK;
}

/* Register/unregister HTPHY specific implementations to their commons.
 * Used to configure features/modules implemented for HTPHY.
 */
int
BCMATTACHFN(phy_ht_register_impl)(phy_info_t *pi, phy_type_info_t *ti, int bandtype)
{
	phy_ht_info_t *hti = (phy_ht_info_t *)ti;

	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	/* Register with ANAcore control module */
	if (pi->anai != NULL &&
	    (hti->anai = phy_ht_ana_register_impl(pi, hti, pi->anai)) == NULL) {
		PHY_ERROR(("%s: phy_ht_ana_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with RADIO control module */
	if (pi->radioi != NULL &&
	    (hti->radioi = phy_ht_radio_register_impl(pi, hti, pi->radioi)) == NULL) {
		PHY_ERROR(("%s: phy_ht_radio_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with PHYTableInit module */
	if (pi->tbli != NULL &&
	    (hti->tbli = phy_ht_tbl_register_impl(pi, hti, pi->tbli)) == NULL) {
		PHY_ERROR(("%s: phy_ht_tbl_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with TxPowerCtrl module */
	if (pi->tpci != NULL &&
	    (hti->tpci = phy_ht_tpc_register_impl(pi, hti, pi->tpci)) == NULL) {
		PHY_ERROR(("%s: phy_ht_tpc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

#if defined(AP) && defined(RADAR)
	/* Register with RadarDetect module */
	if (pi->radari != NULL &&
	    (hti->radari = phy_ht_radar_register_impl(pi, hti, pi->radari)) == NULL) {
		PHY_ERROR(("%s: phy_ht_radar_register_impl failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* Register with MPhaseCAL module */
	if (pi->calmgri != NULL &&
	    (hti->calmgri = phy_ht_calmgr_register_impl(pi, hti, pi->calmgri)) == NULL) {
		PHY_ERROR(("%s: phy_ht_calmgr_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

#ifndef WLC_DISABLE_ACI
	/* Register with INTerFerence module */
	if (pi->noisei != NULL &&
	    (hti->noisei = phy_ht_noise_register_impl(pi, hti, pi->noisei)) == NULL) {
		PHY_ERROR(("%s: phy_ht_noise_register_impl failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* Register with TEMPerature sense module */
	if (pi->tempi != NULL &&
	    (hti->tempi = phy_ht_temp_register_impl(pi, hti, pi->tempi)) == NULL) {
		PHY_ERROR(("%s: phy_ht_temp_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with RSSICompute module */
	if (pi->rssii != NULL &&
	    (hti->rssii = phy_ht_rssi_register_impl(pi, hti, pi->rssii)) == NULL) {
		PHY_ERROR(("%s: phy_ht_rssi_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with TxIQLOCal module */
	if (pi->txiqlocali != NULL &&
		(hti->txiqlocali =
		phy_ht_txiqlocal_register_impl(pi, hti, pi->txiqlocali)) == NULL) {
		PHY_ERROR(("%s: phy_ht_txiqlocal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with RxIQCal module */
	if (pi->rxiqcali != NULL &&
		(hti->rxiqcali = phy_ht_rxiqcal_register_impl(pi, hti, pi->rxiqcali)) == NULL) {
		PHY_ERROR(("%s: phy_ht_rxiqcal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with PAPDCal module */
	if (pi->papdcali != NULL &&
		(hti->papdcali = phy_ht_papdcal_register_impl(pi, hti, pi->papdcali)) == NULL) {
		PHY_ERROR(("%s: phy_ht_papdcal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with VCOCal module */
	if (pi->vcocali != NULL &&
		(hti->vcocali = phy_ht_vcocal_register_impl(pi, hti, pi->vcocali)) == NULL) {
		PHY_ERROR(("%s: phy_ht_vcocal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}


	/* ...Add your module registration here... */


	return BCME_OK;

fail:
	return BCME_ERROR;
}

void
BCMATTACHFN(phy_ht_unregister_impl)(phy_info_t *pi, phy_type_info_t *ti)
{
	phy_ht_info_t *hti = (phy_ht_info_t *)ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Unregister from VCO Cal module */
	if (hti->vcocali != NULL)
		phy_ht_vcocal_unregister_impl(hti->vcocali);

	/* Unregister from PAPD Cal module */
	if (hti->papdcali != NULL)
		phy_ht_papdcal_unregister_impl(hti->papdcali);

	/* Unregister from RXIQ Cal module */
	if (hti->rxiqcali != NULL)
		phy_ht_rxiqcal_unregister_impl(hti->rxiqcali);

	/* Unregister from TXIQLO Cal module */
	if (hti->txiqlocali != NULL)
		phy_ht_txiqlocal_unregister_impl(hti->txiqlocali);

	/* Unregister from RSSICompute module */
	if (hti->rssii != NULL)
		phy_ht_rssi_unregister_impl(hti->rssii);

	/* Unregister from TEMPerature sense module */
	if (hti->tempi != NULL)
		phy_ht_temp_unregister_impl(hti->tempi);

#ifndef WLC_DISABLE_ACI
	/* Unregister from INTerFerence module */
	if (hti->noisei != NULL)
		phy_ht_noise_unregister_impl(hti->noisei);
#endif

	/* Unregister from MPhaseCAL module */
	if (hti->calmgri != NULL)
		phy_ht_calmgr_unregister_impl(hti->calmgri);

#if defined(AP) && defined(RADAR)
	/* Unregister from RadarDetect module */
	if (hti->radari != NULL)
		phy_ht_radar_unregister_impl(hti->radari);
#endif

	/* Unregister from TxPowerCtrl module */
	if (hti->tpci != NULL)
		phy_ht_tpc_unregister_impl(hti->tpci);

	/* Unregister from PHYTableInit module */
	if (hti->tbli != NULL)
		phy_ht_tbl_unregister_impl(hti->tbli);

	/* Unregister from RADIO control module */
	if (hti->radioi != NULL)
		phy_ht_radio_unregister_impl(hti->radioi);

	/* Unregister from ANAcore control module */
	if (hti->anai != NULL)
		phy_ht_ana_unregister_impl(hti->anai);

	/* ...Add your module registration here... */
}

/* reset implementation (s/w) */
void
phy_ht_reset_impl(phy_info_t *pi, phy_type_info_t *ti)
{
	phy_ht_info_t *hti = (phy_ht_info_t *)ti;
	uint i;

	PHY_TRACE(("%s\n", __FUNCTION__));

	for (i = 0; i < PHY_CORE_MAX; i++)
		hti->txpwrindex[i] = 40;
}
