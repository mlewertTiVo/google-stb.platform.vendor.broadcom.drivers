/*
 * NPHY Core module implementation
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

#include <phy_n_ana.h>
#include <phy_n_radio.h>
#include <phy_n_tbl.h>
#include <phy_n_tpc.h>
#include <phy_n_radar.h>
#include <phy_n_calmgr.h>
#include <phy_n_noise.h>
#include <phy_n_antdiv.h>
#include <phy_n_temp.h>
#include <phy_n_rssi.h>
#include <phy_n_papdcal.h>
#include <phy_n_vcocal.h>

#include "phy_type.h"
#include "phy_type_n.h"
#include "phy_type_n_iovt.h"
#include "phy_type_n_ioct.h"
#include "phy_shared.h"
#include <phy_n.h>

#include <phy_utils_radio.h>
#include <phy_utils_var.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_n.h>
#include <wlc_phyreg_n.h>
/* TODO: all these are going away... > */
#endif

#define PHY_TXPWR_MIN_NPHY	8	/* for nphy devices */

/* local functions */
static int phy_n_attach_ext(phy_info_t *pi, int bandtype);
static int phy_n_register_impl(phy_info_t *pi, phy_type_info_t *ti, int bandtype);
static void phy_n_unregister_impl(phy_info_t *pi, phy_type_info_t *ti);
static void phy_n_reset_impl(phy_info_t *pi, phy_type_info_t *ti);
static int phy_n_init_impl(phy_info_t *pi, phy_type_info_t *ti);
#define	phy_n_dump_phyregs	NULL

/* attach/detach */
phy_type_info_t *
BCMATTACHFN(phy_n_attach)(phy_info_t *pi, int bandtype)
{
	phy_n_info_t *ni;
	phy_type_fns_t fns;
	uint32 idcode;

	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	/* Extend phy_attach() here to initialize NPHY specific stuff */
	if (phy_n_attach_ext(pi, bandtype) != BCME_OK) {
		PHY_ERROR(("%s: phy_n_attach_ext failed\n", __FUNCTION__));
		return NULL;
	}

	/* read idcode */
	idcode = phy_n_radio_query_idcode(pi);
	PHY_TRACE(("%s: idcode 0x%08x\n", __FUNCTION__, idcode));
	/* parse idcode */
	phy_utils_parse_idcode(pi, idcode);
	/* override radiover */
	if (NREV_IS(pi->pubpi.phy_rev, LCNXN_BASEREV + 4) ||
	    /* 4324B0, 4324B2, 43242/43243 */
	    CHIPID_4324X_MEDIA_FAMILY(pi))
		pi->pubpi.radiover = RADIOVER(pi->pubpi.radiover);
	/* validate radio id */
	if (phy_utils_valid_radio(pi) != BCME_OK) {
		PHY_ERROR(("%s: phy_utils_valid_radio failed\n", __FUNCTION__));
		return NULL;
	}

	/* TODO: move the acphy attach code to here... */
	if (wlc_phy_attach_nphy(pi) == FALSE) {
		PHY_ERROR(("%s: wlc_phy_attach_nphy failed\n", __FUNCTION__));
		return NULL;
	}
	ni = pi->u.pi_nphy;
	ni->pi = pi;

	/* register PHY type implementation entry points */
	bzero(&fns, sizeof(fns));
	fns.reg_impl = phy_n_register_impl;
	fns.unreg_impl = phy_n_unregister_impl;
	fns.reset_impl = phy_n_reset_impl;
	fns.reg_iovt = phy_n_register_iovt;
	fns.reg_ioct = phy_n_register_ioct;
	fns.init_impl = phy_n_init_impl;
	fns.dump_phyregs = phy_n_dump_phyregs;
	fns.ti = (phy_type_info_t *)ni;

	phy_register_impl(pi, &fns);

	return (phy_type_info_t *)ni;
}

void
BCMATTACHFN(phy_n_detach)(phy_type_info_t *ti)
{
	phy_n_info_t *ni = (phy_n_info_t *)ti;
	phy_info_t *pi = ni->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_mfree(pi, ni, sizeof(phy_n_info_t));
}

static int
BCMATTACHFN(phy_n_attach_ext)(phy_info_t *pi, int bandtype)
{
	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	pi->pubpi.phy_corenum = PHY_CORE_NUM_2;

	pi->aci_exit_check_period = 15;

#ifdef N2WOWL
	/* Reduce phyrxchain to 1 to save power in WOWL mode */
	if (CHIPID(pi->sh->chip) == BCM43237_CHIP_ID) {
		pi->sh->phyrxchain = 0x1;
	}
#endif /* N2WOWL */

	/* minimum reliable txpwr target is 8 dBm/mimo, 9dBm/lcn40, 10 dbm/legacy  */
	pi->min_txpower =
	        (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_mintxpower, PHY_TXPWR_MIN_NPHY);

	pi->phynoise_polling = FALSE;

	return BCME_OK;
}

/* initialize s/w and/or h/w shared by different modules */
static int
WLBANDINITFN(phy_n_init_impl)(phy_info_t *pi, phy_type_info_t *ti)
{
	/* Reset gain_boost on band-change */
	pi->nphy_gain_boost = TRUE;

	return BCME_OK;
}

/* Register/unregister NPHY specific implementations to their commons.
 * Used to configure features/modules implemented for NPHY.
 */
static int
BCMATTACHFN(phy_n_register_impl)(phy_info_t *pi, phy_type_info_t *ti, int bandtype)
{
	phy_n_info_t *ni = (phy_n_info_t *)ti;

	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	/* Register with ANAcore control module */
	if (pi->anai != NULL &&
	    (ni->anai = phy_n_ana_register_impl(pi, ni, pi->anai)) == NULL) {
		PHY_ERROR(("%s: phy_n_ana_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with RADIO control module */
	if (pi->radioi != NULL &&
	    (ni->radioi = phy_n_radio_register_impl(pi, ni, pi->radioi)) == NULL) {
		PHY_ERROR(("%s: phy_n_radio_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with PHYTableInit module */
	if (pi->tbli != NULL &&
	    (ni->tbli = phy_n_tbl_register_impl(pi, ni, pi->tbli)) == NULL) {
		PHY_ERROR(("%s: phy_n_tbl_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with TxPowerCtrl module */
	if (pi->tpci != NULL &&
	    (ni->tpci = phy_n_tpc_register_impl(pi, ni, pi->tpci)) == NULL) {
		PHY_ERROR(("%s: phy_n_tpc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

#if defined(AP) && defined(RADAR)
	/* Register with RadarDetect module */
	if (pi->radari != NULL &&
	    (ni->radari = phy_n_radar_register_impl(pi, ni, pi->radari)) == NULL) {
		PHY_ERROR(("%s: phy_n_radar_register_impl failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* Register with MPhaseCAL module */
	if (pi->calmgri != NULL &&
	    (ni->calmgri = phy_n_calmgr_register_impl(pi, ni, pi->calmgri)) == NULL) {
		PHY_ERROR(("%s: phy_n_calmgr_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with ANTennaDIVersity module */
	if (pi->antdivi != NULL &&
	    (ni->antdivi = phy_n_antdiv_register_impl(pi, ni, pi->antdivi)) == NULL) {
		PHY_ERROR(("%s: phy_n_antdiv_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

#ifndef WLC_DISABLE_ACI
	/* Register with INTerFerence module */
	if (pi->noisei != NULL &&
	    (ni->noisei = phy_n_noise_register_impl(pi, ni, pi->noisei)) == NULL) {
		PHY_ERROR(("%s: phy_n_noise_register_impl failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* Register with TEMPerature sense module */
	if (pi->tempi != NULL &&
	    (ni->tempi = phy_n_temp_register_impl(pi, ni, pi->tempi)) == NULL) {
		PHY_ERROR(("%s: phy_n_temp_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with RSSICompute module */
	if (pi->rssii != NULL &&
	    (ni->rssii = phy_n_rssi_register_impl(pi, ni, pi->rssii)) == NULL) {
		PHY_ERROR(("%s: phy_n_rssi_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with TxIQLOCal module */
	if (pi->txiqlocali != NULL &&
		(ni->txiqlocali =
		phy_n_txiqlocal_register_impl(pi, ni, pi->txiqlocali)) == NULL) {
		PHY_ERROR(("%s: phy_n_txiqlocal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with RxIQCal module */
	if (pi->rxiqcali != NULL &&
		(ni->rxiqcali = phy_n_rxiqcal_register_impl(pi, ni, pi->rxiqcali)) == NULL) {
		PHY_ERROR(("%s: phy_n_rxiqcal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with PAPDCal module */
	if (pi->papdcali != NULL &&
		(ni->papdcali = phy_n_papdcal_register_impl(pi, ni, pi->papdcali)) == NULL) {
		PHY_ERROR(("%s: phy_n_papdcal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with VCOCal module */
	if (pi->vcocali != NULL &&
		(ni->vcocali = phy_n_vcocal_register_impl(pi, ni, pi->vcocali)) == NULL) {
		PHY_ERROR(("%s: phy_n_vcocal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}


	/* ...Add your module registration here... */

	return BCME_OK;
fail:
	return BCME_ERROR;
}

static void
BCMATTACHFN(phy_n_unregister_impl)(phy_info_t *pi, phy_type_info_t *ti)
{
	phy_n_info_t *ni = (phy_n_info_t *)ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Unregister from VCO Cal module */
	if (ni->vcocali != NULL)
		phy_n_vcocal_unregister_impl(ni->vcocali);

	/* Unregister from PAPD Cal module */
	if (ni->papdcali != NULL)
		phy_n_papdcal_unregister_impl(ni->papdcali);

	/* Unregister from RXIQ Cal module */
	if (ni->rxiqcali != NULL)
		phy_n_rxiqcal_unregister_impl(ni->rxiqcali);

	/* Unregister from TXIQLO Cal module */
	if (ni->txiqlocali != NULL)
		phy_n_txiqlocal_unregister_impl(ni->txiqlocali);

	/* Unregister from RSSICompute module */
	if (ni->rssii != NULL)
		phy_n_rssi_unregister_impl(ni->rssii);

	/* Unregister from TEMPerature sense module */
	if (ni->tempi != NULL)
		phy_n_temp_unregister_impl(ni->tempi);

#ifndef WLC_DISABLE_ACI
	/* Unregister from INTerFerence module */
	if (ni->noisei != NULL)
		phy_n_noise_unregister_impl(ni->noisei);
#endif

	/* Unregister from ANTennaDIVersity module */
	if (ni->antdivi != NULL)
		phy_n_antdiv_unregister_impl(ni->antdivi);

	/* Unregister from MPhaseCAL module */
	if (ni->calmgri != NULL)
		phy_n_calmgr_unregister_impl(ni->calmgri);

#if defined(AP) && defined(RADAR)
	/* Unregister from RadarDetect module */
	if (ni->radari != NULL)
		phy_n_radar_unregister_impl(ni->radari);
#endif

	/* Unregister from TxPowerCtrl module */
	if (ni->tpci != NULL)
		phy_n_tpc_unregister_impl(ni->tpci);

	/* Unregister from PHYTableInit module */
	if (ni->tbli != NULL)
		phy_n_tbl_unregister_impl(ni->tbli);

	/* Unregister from RADIO control module */
	if (ni->radioi != NULL)
		phy_n_radio_unregister_impl(ni->radioi);

	/* Unregister from ANAcore control module */
	if (ni->anai != NULL)
		phy_n_ana_unregister_impl(ni->anai);

	/* ...Add your module registration here... */
}

/* reset implementation (s/w) */
static void
phy_n_reset_impl(phy_info_t *pi, phy_type_info_t *ti)
{
	phy_n_info_t *ni = (phy_n_info_t *)ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (!(NREV_GE(pi->pubpi.phy_rev, 3)))
		pi->phy_spuravoid = SPURAVOID_DISABLE;

	ni->nphy_papd_skip = 0;
	ni->nphy_papd_epsilon_offset[0] = 0xf588;
	ni->nphy_papd_epsilon_offset[1] = 0xf588;
	ni->nphy_txpwr_idx_2G[0] = 128;
	ni->nphy_txpwr_idx_2G[1] = 128;
	ni->nphy_txpwr_idx_5G[0] = 128;
	ni->nphy_txpwr_idx_5G[1] = 128;
	ni->nphy_txpwrindex[0].index_internal = 40;
	ni->nphy_txpwrindex[1].index_internal = 40;
	ni->nphy_pabias = 0; /* default means no override */
}
