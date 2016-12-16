/*
 * DCCAL module implementation.
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
 * $Id: phy_dccal.c 606042 2015-12-14 06:21:23Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_init.h>
#include "phy_type_dccal.h"
#include <phy_dccal.h>

/* module private states */
struct phy_dccal_info {
	phy_info_t *pi;
	phy_type_dccal_fns_t *fns;
};

/* module private states memory layout */
typedef struct {
	phy_dccal_info_t info;
	phy_type_dccal_fns_t fns;
/* add other variable size variables here at the end */
} phy_dccal_mem_t;

/* local function declaration */

/* attach/detach */
phy_dccal_info_t *
BCMATTACHFN(phy_dccal_attach)(phy_info_t *pi)
{
	phy_dccal_info_t *info;
	phy_type_dccal_fns_t *fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_dccal_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;

	fns = &((phy_dccal_mem_t *)info)->fns;
	info->fns = fns;

	/* Register callbacks */

	return info;

	/* error */
fail:
	phy_dccal_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_dccal_detach)(phy_dccal_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null dccal module\n", __FUNCTION__));
		return;
	}

	pi = info->pi;

	phy_mfree(pi, info, sizeof(phy_dccal_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_dccal_register_impl)(phy_dccal_info_t *dccali, phy_type_dccal_fns_t *fns)
{

	PHY_TRACE(("%s\n", __FUNCTION__));

	*dccali->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_dccal_unregister_impl)(phy_dccal_info_t *dccali)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}
