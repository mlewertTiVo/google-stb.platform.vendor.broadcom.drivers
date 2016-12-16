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
