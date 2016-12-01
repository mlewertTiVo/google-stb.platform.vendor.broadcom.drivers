/*
 * VCO CAL module implementation.
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
 * $Id: phy_vcocal.c 603319 2015-12-01 21:41:40Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_vcocal.h"
#include <phy_rstr.h>
#include <phy_vcocal.h>

/* forward declaration */
typedef struct phy_vcocal_mem phy_vcocal_mem_t;

/* module private states */
struct phy_vcocal_info {
	phy_info_t 				*pi;	/* PHY info ptr */
	phy_type_vcocal_fns_t	*fns;	/* Function ptr */
	phy_vcocal_mem_t		*mem;	/* Memory layout ptr */
};

/* module private states memory layout */
struct phy_vcocal_mem {
	phy_vcocal_info_t		cmn_info;
	phy_type_vcocal_fns_t	fns;
/* add other variable size variables here at the end */
};

/* local function declaration */

/* attach/detach */
phy_vcocal_info_t *
BCMATTACHFN(phy_vcocal_attach)(phy_info_t *pi)
{
	phy_vcocal_mem_t	*mem;
	phy_vcocal_info_t *cmn_info = NULL;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_vcocal_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	/* Init vcocal params */

	/* Register callbacks */

	return cmn_info;

	/* error */
fail:
	phy_vcocal_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_vcocal_detach)(phy_vcocal_info_t *cmn_info)
{
	phy_vcocal_mem_t	*mem;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	if (!cmn_info)
		return;

	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null vcocal module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_vcocal_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_vcocal_register_impl)(phy_vcocal_info_t *cmn_info, phy_type_vcocal_fns_t *fns)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	*cmn_info->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_vcocal_unregister_impl)(phy_vcocal_info_t *cmn_info)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	cmn_info->fns = NULL;
}

#if defined(BCMINTERNAL) || defined(WLTEST)
void
phy_vcocal_force(phy_info_t *pi)
{
	phy_vcocal_info_t *vi = pi->vcocali;
	phy_type_vcocal_fns_t *fns = vi->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->force != NULL) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		(fns->force)(fns->ctx);
		wlapi_enable_mac(pi->sh->physhim);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */
