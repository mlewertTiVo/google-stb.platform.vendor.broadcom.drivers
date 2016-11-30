/*
 * One Core Listen (OCL) phy module implementation
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
 * $Id: $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_ocl.h>
#include <phy_ocl.h>

/* module private states */
struct phy_ocl_info {
	phy_info_t *pi;
	phy_type_ocl_fns_t *fns;
};

/* module private states memory layout */
typedef struct {
	phy_ocl_info_t ocl_info;
	phy_type_ocl_fns_t fns;
/* add other variable size variables here at the end */
} phy_ocl_mem_t;

/* function prototypes */

#ifdef OCL
/* attach/detach */
phy_ocl_info_t *
BCMATTACHFN(phy_ocl_attach)(phy_info_t *pi)
{
	phy_ocl_info_t *ocl_info;
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((ocl_info = phy_malloc(pi, sizeof(phy_ocl_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	ocl_info->pi = pi;
	ocl_info->fns = &((phy_ocl_mem_t *)ocl_info)->fns;

	return ocl_info;

	/* error */
fail:
	phy_ocl_detach(ocl_info);
	return NULL;

}

void

BCMATTACHFN(phy_ocl_detach)(phy_ocl_info_t *ocl_info)
{
	phy_info_t *pi;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (ocl_info == NULL) {
		PHY_INFORM(("%s: null ocl module\n", __FUNCTION__));
		return;
	}

	pi = ocl_info->pi;
	phy_mfree(pi, ocl_info, sizeof(phy_ocl_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_ocl_register_impl)(phy_ocl_info_t *ocl_info,
	phy_type_ocl_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
	*ocl_info->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_ocl_unregister_impl)(phy_ocl_info_t *ocl_info)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

void wlc_phy_ocl_enable(phy_info_t *pi, bool enable)
{
	phy_ocl_info_t *ocl_info = pi->ocli;
	phy_type_ocl_fns_t *fns = ocl_info->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));
	ASSERT(fns->ocl != NULL);
	(fns->ocl)(pi, enable);
}
#endif /* OCL */
