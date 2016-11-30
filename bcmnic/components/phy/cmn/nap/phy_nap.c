/*
 * Napping module implementation.
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
 * $Id$
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_nap.h"
#include <phy_nap.h>
#include <phy_nap_api.h>
#include <phy_utils_var.h>

/* forward declaration */
typedef struct phy_nap_mem phy_nap_mem_t;

/* module private states */
struct phy_nap_info {
	phy_info_t		*pi;	/* PHY info ptr */
	phy_type_nap_fns_t	*fns;	/* PHY specific function ptrs */
	phy_nap_mem_t		*mem;	/* Memory layout ptr */
};

/* module private states memory layout */
struct phy_nap_mem {
	phy_nap_info_t		cmn_info;
	phy_type_nap_fns_t	fns;
	/* add other variable size variables here at the end */
};

static const char BCMATTACHDATA(rstr_napen)[] = "ulpnap";

/* attach/detach */
phy_nap_info_t *
BCMATTACHFN(phy_nap_attach)(phy_info_t *pi)
{
	phy_nap_mem_t	*mem = NULL;
	phy_nap_info_t	*cmn_info = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_nap_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize infra params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	return cmn_info;

fail:
	phy_nap_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_nap_detach)(phy_nap_info_t *cmn_info)
{
	phy_nap_mem_t	*mem;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	/* Malloc has failed. No cleanup is necessary here. */
	if (!cmn_info) {
		return;
	}

	/* Freeup memory associated with cmn_info. */
	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null nap module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_nap_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_nap_register_impl)(phy_nap_info_t *mi, phy_type_nap_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	*mi->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_nap_unregister_impl)(phy_nap_info_t *mi)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

#ifdef WL_NAP
void
wlc_phy_nap_status_get(wlc_phy_t *ppi, uint16 *reqs, bool *nap_en)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_nap_fns_t *fns;
	ASSERT(pi != NULL);
	fns = pi->napi->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->get_status != NULL) {
		(fns->get_status)(fns->ctx, reqs, nap_en);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

void
wlc_phy_nap_disable_req_set(wlc_phy_t *ppi, uint16 req, bool disable, bool agc_reconfig, uint8 req_id)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_nap_fns_t *fns;
	ASSERT(pi != NULL);
	fns = pi->napi->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->set_disable_req != NULL) {
		(fns->set_disable_req)(fns->ctx, req, disable, agc_reconfig, req_id);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}
#endif /* WL_NAP */
