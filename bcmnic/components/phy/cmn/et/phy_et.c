/*
 * Envelope Tracking module implementation.
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
 * $Id: phy_et.c 623369 2016-03-07 20:07:03Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_et.h"

/* module private states */
struct phy_et_info {
	phy_info_t *pi;
	phy_type_et_fns_t *fns; /* PHY specific function ptrs */
/* add other variable size variables here at the end */
};

/* module private states memory layout */
typedef struct {
	phy_et_info_t info;
	phy_type_et_fns_t fns;
/* add other variable size variables here at the end */
} phy_et_mem_t;

/* local function declarations */

/* attach/detach */
phy_et_info_t *
BCMATTACHFN(phy_et_attach)(phy_info_t *pi)
{
	phy_et_info_t *info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_et_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;
	info->fns = &((phy_et_mem_t *)info)->fns;

	/* register watchdog fns */

	return info;

	/* error */
fail:
	phy_et_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_et_detach)(phy_et_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null et module\n", __FUNCTION__));
		return;
	}

	pi = info->pi;
	phy_mfree(pi, info, sizeof(phy_et_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_et_register_impl)(phy_et_info_t *cmn_info, phy_type_et_fns_t *fns)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	*cmn_info->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_et_unregister_impl)(phy_et_info_t *cmn_info)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	cmn_info->fns = NULL;
}
