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
 * $Id: phy_ac_hc.c 656120 2016-08-25 08:17:57Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_hc.h"
#include <phy_ac.h>
#include <phy_ac_hc.h>
#include <phy_ac_info.h>

/* module private states */
struct phy_ac_hc_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_hc_info_t *info;
};

#ifdef RADIO_HEALTH_CHECK

/* local functions */
static int phy_ac_hc_debugcrash_forcefail(phy_type_hc_ctx_t *ctx, phy_crash_reason_t dctype);

/* register phy type specific implementation */
phy_ac_hc_info_t *
BCMATTACHFN(phy_ac_hc_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_hc_info_t *info)
{
	phy_ac_hc_info_t *hci;
	phy_type_hc_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((hci = phy_malloc(pi, sizeof(*hci))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	hci->pi = pi;
	hci->aci = aci;
	hci->info = info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = hci;
	fns.force_fail = phy_ac_hc_debugcrash_forcefail;
	if (phy_hc_register_impl(info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_hc_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return hci;

	/* error handling */
fail:
	if (hci != NULL)
		phy_mfree(pi, hci, sizeof(*hci));
	return NULL;
}

void
BCMATTACHFN(phy_ac_hc_unregister_impl)(phy_ac_hc_info_t *hci)
{
	phy_info_t *pi;
	phy_hc_info_t *info;

	ASSERT(hci);

	pi = hci->pi;
	info = hci->info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_hc_unregister_impl(info);

	phy_mfree(pi, hci, sizeof(*hci));
}

static int
phy_ac_hc_debugcrash_forcefail(phy_type_hc_ctx_t *ctx, phy_crash_reason_t dctype)
{
	phy_ac_hc_info_t *hci = (phy_ac_hc_info_t *)ctx;
	phy_info_t *pi = hci->pi;

	PHY_TRACE(("\n %s : dctype : %d \n", __FUNCTION__, dctype));
	switch (dctype) {
		case PHY_RC_DESENSE_LIMITS:
			if (phy_ac_noise_force_fail_desense(pi->u.pi_acphy->noisei) !=
					BCME_OK)
				return BCME_UNSUPPORTED;
			break;
		case PHY_RC_BASEINDEX_LIMITS:
			if (phy_ac_tpc_force_fail_baseindex(pi->u.pi_acphy->tpci) !=
					BCME_OK)
				return BCME_UNSUPPORTED;
			break;
		default:
			ASSERT(0);
			return BCME_UNSUPPORTED;
	}
	return BCME_OK;
}
#endif /* RADIO_HEALTH_CHECK */
