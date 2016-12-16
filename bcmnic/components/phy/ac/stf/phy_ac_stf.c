/*
 * ACPHY STF module implementation
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
 * $Id: phy_ac_stf.c 672458 2016-11-28 17:25:10Z $
 */
#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <qmath.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_stf.h>
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_reg.h>
#include <phy_ac_chanmgr.h>
#include <phy_ac_stf.h>
#include <wlc_radioreg_20693.h>
#include <phy_rstr.h>
#include <phy_utils_var.h>
#include <wlioctl.h>
#include <phy_stf.h>

/* module private states */
struct phy_ac_stf_info {
	phy_info_t *pi;
	phy_ac_info_t *pi_ac;
	phy_stf_info_t *stf_info;
};

/* locally used functions */

/* Functions used by common layer as callbacks */
static int phy_ac_stf_set_stf_chain(phy_type_stf_ctx_t *ctx,
		uint8 txchain, uint8 rxchain);
static int phy_ac_stf_chain_init(phy_type_stf_ctx_t *ctx, bool txrxchain_mask,
		uint8 txchain, uint8 rxchain);

/* register phy type specific implementation */
phy_ac_stf_info_t*
BCMATTACHFN(phy_ac_stf_register_impl)(phy_info_t *pi,
	phy_ac_info_t *pi_ac, phy_stf_info_t *stf_info)
{
	phy_ac_stf_info_t *ac_stf_info;
	phy_type_stf_fns_t fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_stf_info = phy_malloc(pi, sizeof(phy_ac_stf_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	ac_stf_info->pi = pi;
	ac_stf_info->pi_ac = pi_ac;
	ac_stf_info->stf_info = stf_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.set_stf_chain = phy_ac_stf_set_stf_chain;
	fns.chain_init = phy_ac_stf_chain_init;
	fns.ctx = ac_stf_info;

	if (phy_stf_register_impl(stf_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_stf_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ac_stf_info;
	/* error handling */
fail:
	if (ac_stf_info != NULL)
		phy_mfree(pi, ac_stf_info, sizeof(phy_ac_stf_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_stf_unregister_impl)(phy_ac_stf_info_t *ac_stf_info)
{
	phy_info_t *pi = ac_stf_info->pi;
	phy_stf_info_t *stf_info = ac_stf_info->stf_info;
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_stf_unregister_impl(stf_info);
	phy_mfree(pi, ac_stf_info, sizeof(phy_ac_stf_info_t));
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
/* ****       Functions used by other AC modules     **** */

/* ********************************************* */
/*	Callback Functions                                                   */
/* ********************************************* */
static int
phy_ac_stf_set_stf_chain(phy_type_stf_ctx_t *ctx, uint8 txchain, uint8 rxchain)
{
	phy_ac_stf_info_t *ac_stf_info = NULL;
	phy_stf_info_t *cmn_stf_info = NULL;
	phy_info_t *pi = NULL;

	ASSERT(ctx);
	ac_stf_info = (phy_ac_stf_info_t *) ctx;

	ASSERT(ac_stf_info->stf_info);
	cmn_stf_info = (phy_stf_info_t *) ac_stf_info->stf_info;
	pi = ac_stf_info->pi;

	PHY_TRACE(("phy_ac_stf_set_stf_chain, new phy chain tx %d, rx %d", txchain, rxchain));

	phy_stf_set_phytxchain(cmn_stf_info, txchain);
	if (!(ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_37(pi->pubpi->phy_rev))) {
		phy_ac_chanmgr_set_both_txchain_rxchain(pi->u.pi_acphy->chanmgri, rxchain, txchain);
	} else {
#ifdef STA
		if (pi->sh->up) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		}
#endif
		wlc_phy_rxcore_setstate_acphy((wlc_phy_t *)pi, rxchain,
				phy_stf_get_data(cmn_stf_info)->phytxchain);

#ifdef STA
		if (pi->sh->up) {
			wlapi_mimops_pmbcnrx(pi->sh->physhim);
			wlapi_enable_mac(pi->sh->physhim);
		}
#endif
	}

	return BCME_OK;
}

static int
phy_ac_stf_chain_init(phy_type_stf_ctx_t *ctx, bool txrxchain_mask,
		uint8 txchain, uint8 rxchain)
{
	phy_ac_stf_info_t *ac_stf_info = NULL;
	phy_stf_info_t *cmn_stf_info = NULL;
	phy_info_t *pi = NULL;

	ASSERT(ctx);
	ac_stf_info = (phy_ac_stf_info_t *) ctx;

	ASSERT(ac_stf_info->stf_info);
	cmn_stf_info = (phy_stf_info_t *) ac_stf_info->stf_info;
	pi = ac_stf_info->pi;

	if ((txrxchain_mask) &&
		(ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev))) {
		phy_stf_set_phytxchain(cmn_stf_info, txchain & pi->sromi->sw_txchain_mask);
		phy_stf_set_phyrxchain(cmn_stf_info, rxchain & pi->sromi->sw_rxchain_mask);
	} else {
		phy_stf_set_phytxchain(cmn_stf_info, txchain);
		phy_stf_set_phyrxchain(cmn_stf_info, rxchain);
	}

	return BCME_OK;
}
