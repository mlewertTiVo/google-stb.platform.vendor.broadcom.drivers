/*
 * High Efficiency (802.11ax) (HE) phy module implementation
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
#include <phy_type_hecap.h>
#include <phy_hecap.h>

/* module private states */
struct phy_hecap_info {
	phy_info_t *pi;
	phy_type_hecap_fns_t *fns;
};

/* module private states memory layout */
typedef struct {
	phy_hecap_info_t he_info;
	phy_type_hecap_fns_t fns;
/* add other variable size variables here at the end */
} phy_hecap_mem_t;

/* function prototypes */

#ifdef WL11AX
/* attach/detach */
phy_hecap_info_t *
BCMATTACHFN(phy_hecap_attach)(phy_info_t *pi)
{
	phy_hecap_info_t *he_info;
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((he_info = phy_malloc(pi, sizeof(phy_hecap_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	he_info->pi = pi;
	he_info->fns = &((phy_hecap_mem_t *)he_info)->fns;

	return he_info;

	/* error */
fail:
	phy_hecap_detach(he_info);
	return NULL;

}

void

BCMATTACHFN(phy_hecap_detach)(phy_hecap_info_t *he_info)
{
	phy_info_t *pi;
	PHY_TRACE(("%s\n", __FUNCTION__));

	if (he_info == NULL) {
		PHY_INFORM(("%s: null HE module\n", __FUNCTION__));
		return;
	}

	pi = he_info->pi;
	phy_mfree(pi, he_info, sizeof(phy_hecap_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_hecap_register_impl)(phy_hecap_info_t *he_info,
	phy_type_hecap_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
	*he_info->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_hecap_unregister_impl)(phy_hecap_info_t *he_info)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

void wlc_phy_hecap_enable(phy_info_t *pi, bool enable)
{
	phy_hecap_info_t *hecap_info = pi->hecapi;
	phy_type_hecap_fns_t *fns = hecap_info->fns;
	PHY_TRACE(("%s\n", __FUNCTION__));
	ASSERT(fns->hecap != NULL);
	(fns->hecap)(pi, enable);
}
#endif /* WL11AX */
