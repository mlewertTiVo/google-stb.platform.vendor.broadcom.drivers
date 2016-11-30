/*
 * lcn20PHY Rx Spur canceller module implementation
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

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_mem.h>
#include <phy_rxspur.h>
#include "phy_type_rxspur.h"
#include <phy_lcn20.h>
#include <phy_lcn20_rxspur.h>
#include <hndpmu.h>
#include <wlc_phy_shim.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn20.h>
/* TODO: all these are going away... > */
#endif

/* The default fvco MUST match the setting in pmu-pll-init */
#define LCN20PHY_SPURMODE_FVCO_DEFAULT  LCN20PHY_SPURMODE_FVCO_972

/* module private states */
struct phy_lcn20_rxspur_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_rxspur_info_t *rxspuri;
};

/* local functions */
#if defined(WLTEST)
static int phy_lcn20_rxspur_set_force_spurmode(phy_type_rxspur_ctx_t *ctx, int16 int_val);
static int phy_lcn20_rxspur_get_force_spurmode(phy_type_rxspur_ctx_t *ctx, int32 *ret_int_ptr);
#endif /* WLTEST */

/* Register/unregister lcn20PHY specific implementation to common layer */
phy_lcn20_rxspur_info_t *
BCMATTACHFN(phy_lcn20_rxspur_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_rxspur_info_t *rxspuri)
{
	phy_lcn20_rxspur_info_t *info;
	phy_type_rxspur_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_rxspur_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_rxspur_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->rxspuri = rxspuri;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
#if defined(WLTEST)
	fns.set_force_spurmode = phy_lcn20_rxspur_set_force_spurmode;
	fns.get_force_spurmode  = phy_lcn20_rxspur_get_force_spurmode;
#endif /* WLTEST */
	fns.ctx = info;

	phy_rxspur_register_impl(rxspuri, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_rxspur_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_rxspur_unregister_impl)(phy_lcn20_rxspur_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_rxspur_info_t *rxspuri = info->rxspuri;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_rxspur_unregister_impl(rxspuri);

	phy_mfree(pi, info, sizeof(phy_lcn20_rxspur_info_t));
}

#if defined(WLTEST)
static void
wlc_lcn20phy_set_spurmode(phy_info_t *pi, uint8 channel)
{
	phy_info_lcn20phy_t *pi_lcn20 = pi->u.pi_lcn20phy;
	uint8 spurmode = LCN20PHY_SPURMODE_FVCO_DEFAULT;

	PHY_TRACE(("%s:\n", __FUNCTION__));

	if (pi_lcn20->spurmode_override) {
		/*
		  spurmode decided by iovar
		*/
		spurmode = pi_lcn20->forced_spurmode;
	} else {
		/*
		  spurmode decided by nvram config
		*/
		spurmode = pi_lcn20->spurmode;
	}

	si_pmu_spuravoid(pi->sh->sih, pi->sh->osh, spurmode);
	wlapi_switch_macfreq(pi->sh->physhim, WL_SPURAVOID_ON1);
}

static int
phy_lcn20_rxspur_set_force_spurmode(phy_type_rxspur_ctx_t *ctx, int16 int_val)
{
	phy_lcn20_rxspur_info_t *rxspuri = (phy_lcn20_rxspur_info_t *) ctx;
	phy_info_t *pi = rxspuri->pi;
	phy_info_lcn20phy_t *pi_lcn20 = rxspuri->lcn20i;
	uint8 channel = CHSPEC_CHANNEL(pi->radio_chanspec); /* see wlioctl.h */

	if (int_val == -1) {
		pi_lcn20->spurmode_override = FALSE;
	} else {
		pi_lcn20->spurmode_override = TRUE;
	}

	switch (int_val) {
	case -1:
		PHY_TRACE(("Spurmode override is off; default spurmode restored; %s \n",
		__FUNCTION__));
		break;
	case LCN20PHY_SPURMODE_FVCO_972:
		PHY_TRACE(("Force spurmode to Fvco 972 MHz; %s \n",
		__FUNCTION__));
		pi_lcn20->forced_spurmode = LCN20PHY_SPURMODE_FVCO_972;
		break;
	case LCN20PHY_SPURMODE_FVCO_980:
		PHY_TRACE(("Force spurmode to Fvco 980 MHz; %s \n",
		__FUNCTION__));
		pi_lcn20->forced_spurmode = LCN20PHY_SPURMODE_FVCO_980;
		break;
	case LCN20PHY_SPURMODE_FVCO_984:
		PHY_TRACE(("Force spurmode to Fvco 984 MHz; %s \n",
		__FUNCTION__));
		pi_lcn20->forced_spurmode = LCN20PHY_SPURMODE_FVCO_984;
		break;
	case LCN20PHY_SPURMODE_FVCO_326P4:
		PHY_TRACE(("Force spurmode to Fvco 326.4 MHz; %s \n",
		__FUNCTION__));
		pi_lcn20->forced_spurmode = LCN20PHY_SPURMODE_FVCO_326P4;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported spurmode %d.\n",
			pi->sh->unit, __FUNCTION__, int_val));
		ASSERT(FALSE);
		pi_lcn20->forced_spurmode = LCN20PHY_SPURMODE_FVCO_DEFAULT;
		return BCME_ERROR;
	}

	wlc_lcn20phy_set_spurmode(pi, channel);
	return BCME_OK;
}

static int
phy_lcn20_rxspur_get_force_spurmode(phy_type_rxspur_ctx_t *ctx, int32 *ret_int_ptr)
{
	phy_lcn20_rxspur_info_t *rxspuri = (phy_lcn20_rxspur_info_t *) ctx;

	if (rxspuri->lcn20i->spurmode_override) {
		*ret_int_ptr = rxspuri->lcn20i->forced_spurmode;
	} else {
		*ret_int_ptr = -1;
	}
	return BCME_OK;
}
#endif /* defined(WLTEST) */
