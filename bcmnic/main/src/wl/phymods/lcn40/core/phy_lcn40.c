/*
 * LCN40PHY Core module implementation
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
#include <phy_lcn40_ana.h>
#include <phy_lcn40_radio.h>
#include <phy_lcn40_tbl.h>
#include <phy_lcn40_tpc.h>
#include <phy_lcn40_noise.h>
#include <phy_lcn40_antdiv.h>
#include <phy_lcn40_rssi.h>
#include "phy_type.h"
#include "phy_type_lcn40.h"
#include "phy_type_lcn40_iovt.h"
#include "phy_type_lcn40_ioct.h"
#include "phy_shared.h"
#include <phy_lcn40.h>

#include <phy_utils_radio.h>
#include <phy_utils_var.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn40.h>
/* TODO: all these are going away... > */
#endif

#define PHY_TXPWR_MIN_LCN40PHY	9	/* for lcn40phy devices */

/* local functions */
static int phy_lcn40_attach_ext(phy_info_t *pi, int bandtype);
static int phy_lcn40_register_impl(phy_info_t *pi, phy_type_info_t *ti, int bandtype);
static void phy_lcn40_unregister_impl(phy_info_t *pi, phy_type_info_t *ti);
#define	phy_lcn40_dump_phyregs	NULL

/* attach/detach */
phy_type_info_t *
BCMATTACHFN(phy_lcn40_attach)(phy_info_t *pi, int bandtype)
{
	phy_lcn40_info_t *lcn40i;
	phy_type_fns_t fns;
	uint32 idcode;

	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	/* Extend phy_attach() here to initialize LCN40PHY specific stuff */
	if (phy_lcn40_attach_ext(pi, bandtype) != BCME_OK) {
		PHY_ERROR(("%s: phy_lcn40_attach_ext failed\n", __FUNCTION__));
		return NULL;
	}

	/* read idcode */
	idcode = phy_lcn40_radio_query_idcode(pi);
	PHY_TRACE(("%s: idcode 0x%08x\n", __FUNCTION__, idcode));
	/* parse idcode */
	phy_utils_parse_idcode(pi, idcode);
	/* validate radio id */
	if (phy_utils_valid_radio(pi) != BCME_OK) {
		PHY_ERROR(("%s: phy_utils_valid_radio failed\n", __FUNCTION__));
		return NULL;
	}

	/* TODO: move the acphy attach code to here... */
	if (wlc_phy_attach_lcn40phy(pi) == FALSE) {
		PHY_ERROR(("%s: wlc_phy_attach_lcn40phy failed\n", __FUNCTION__));
		return NULL;
	}
	lcn40i = pi->u.pi_lcn40phy;
	lcn40i->pi = pi;

	/* register PHY type implementation entry points */
	bzero(&fns, sizeof(fns));
	fns.reg_impl = phy_lcn40_register_impl;
	fns.unreg_impl = phy_lcn40_unregister_impl;
	fns.reg_iovt = phy_lcn40_register_iovt;
	fns.reg_ioct = phy_lcn40_register_ioct;
	fns.dump_phyregs = phy_lcn40_dump_phyregs;
	fns.ti = (phy_type_info_t *)lcn40i;

	phy_register_impl(pi, &fns);

	return (phy_type_info_t *)lcn40i;
}

void
BCMATTACHFN(phy_lcn40_detach)(phy_type_info_t *ti)
{
	phy_lcn40_info_t *lcn40i = (phy_lcn40_info_t *)ti;
	phy_info_t *pi = lcn40i->pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_mfree(pi, lcn40i, sizeof(phy_lcn40_info_t));
}

static int
BCMATTACHFN(phy_lcn40_attach_ext)(phy_info_t *pi, int bandtype)
{
	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	pi->min_txpower = PHY_TXPWR_MIN_LCN40PHY;

	if (CHIPID(pi->sh->chip) != BCM43143_CHIP_ID)
		pi->tx_pwr_backoff = (int8)PHY_GETINTVAR_DEFAULT(pi, rstr_txpwrbckof, 4);

	pi->rssi_corr_boardatten =
		(int8)PHY_GETINTVAR_DEFAULT(pi, rstr_rssicorratten, 0);

	pi->phynoise_polling = FALSE;

	return BCME_OK;
}

/* Register/unregister LCN40PHY specific implementations to their commons.
 * Used to configure features/modules implemented for LCN40PHY.
 */
static int
BCMATTACHFN(phy_lcn40_register_impl)(phy_info_t *pi, phy_type_info_t *ti, int bandtype)
{
	phy_lcn40_info_t *lcn40i = (phy_lcn40_info_t *)ti;

	PHY_TRACE(("%s: band %d\n", __FUNCTION__, bandtype));

	/* Register with ANAcore control module */
	if (pi->anai != NULL &&
	    (lcn40i->anai = phy_lcn40_ana_register_impl(pi, lcn40i, pi->anai)) == NULL) {
		PHY_ERROR(("%s: phy_lcn40_ana_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with RADIO control module */
	if (pi->radioi != NULL &&
	    (lcn40i->radioi = phy_lcn40_radio_register_impl(pi, lcn40i, pi->radioi)) == NULL) {
		PHY_ERROR(("%s: phy_lcn40_radio_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with PHYTableInit module */
	if (pi->tbli != NULL &&
	    (lcn40i->tbli = phy_lcn40_tbl_register_impl(pi, lcn40i, pi->tbli)) == NULL) {
		PHY_ERROR(("%s: phy_lcn40_tbl_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with TxPowerCtrl module */
	if (pi->tpci != NULL &&
	    (lcn40i->tpci = phy_lcn40_tpc_register_impl(pi, lcn40i, pi->tpci)) == NULL) {
		PHY_ERROR(("%s: phy_lcn40_tpc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register with ANTennaDIVersity module */
	if (pi->antdivi != NULL &&
	    (lcn40i->antdivi = phy_lcn40_antdiv_register_impl(pi, lcn40i, pi->antdivi)) == NULL) {
		PHY_ERROR(("%s: phy_lcn40_antdiv_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

#ifndef WLC_DISABLE_ACI
	/* Register with INTerFerence module */
	if (pi->noisei != NULL &&
	    (lcn40i->noisei = phy_lcn40_noise_register_impl(pi, lcn40i, pi->noisei)) == NULL) {
		PHY_ERROR(("%s: phy_lcn40_noise_register_impl failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* Register with RSSICompute module */
	if (pi->rssii != NULL &&
	    (lcn40i->rssii = phy_lcn40_rssi_register_impl(pi, lcn40i, pi->rssii)) == NULL) {
		PHY_ERROR(("%s: phy_lcn40_rssi_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	/* ...Add your module registration here... */

	return BCME_OK;

fail:
	return BCME_ERROR;
}

static void
BCMATTACHFN(phy_lcn40_unregister_impl)(phy_info_t *pi, phy_type_info_t *ti)
{
	phy_lcn40_info_t *lcn40i = (phy_lcn40_info_t *)ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Unregister from ANAcore control module */
	if (lcn40i->anai != NULL)
		phy_lcn40_ana_unregister_impl(lcn40i->anai);

	/* Unregister from RADIO control module */
	if (lcn40i->radioi != NULL)
		phy_lcn40_radio_unregister_impl(lcn40i->radioi);

	/* Unregister from PHYTableInit module */
	if (lcn40i->tbli != NULL)
		phy_lcn40_tbl_unregister_impl(lcn40i->tbli);

	/* Unregister from TxPowerCtrl module */
	if (lcn40i->tpci != NULL)
		phy_lcn40_tpc_unregister_impl(lcn40i->tpci);

	/* Unregister from ANTennaDIVersity module */
	if (lcn40i->antdivi != NULL)
		phy_lcn40_antdiv_unregister_impl(lcn40i->antdivi);

#ifndef WLC_DISABLE_ACI
	/* Unregister from INTerFerence module */
	if (lcn40i->noisei != NULL)
		phy_lcn40_noise_unregister_impl(lcn40i->noisei);
#endif

	/* Unregister from RSSICompute module */
	if (lcn40i->rssii != NULL)
		phy_lcn40_rssi_unregister_impl(lcn40i->rssii);

	/* ...Add your module registration here... */
}
