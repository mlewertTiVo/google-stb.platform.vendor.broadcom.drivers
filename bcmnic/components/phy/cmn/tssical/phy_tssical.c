/*
 * TSSI Cal module implementation.
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
 * $Id: phy_tssical.c 610412 2016-01-06 23:43:14Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_utils_reg.h>
#include "phy_type_tssical.h"
#include <phy_rstr.h>
#include <phy_tssical_api.h>

/* forward declaration */
typedef struct phy_tssical_mem phy_tssical_mem_t;

/* module private states */
struct phy_tssical_info {
	phy_info_t				*pi;	/* PHY info ptr */
	phy_type_tssical_fns_t *fns;	/* PHY specific function ptrs */
	phy_tssical_mem_t		*mem;	/* Memory layout ptr */
};

/* module private states memory layout */
struct phy_tssical_mem {
	phy_tssical_info_t		cmn_info;
	phy_type_tssical_fns_t	fns;
/* add other variable size variables here at the end */
};

/* local function declaration */

/* attach/detach */
phy_tssical_info_t *
BCMATTACHFN(phy_tssical_attach)(phy_info_t *pi)
{
	phy_tssical_mem_t		*mem = NULL;
	phy_tssical_info_t		*cmn_info = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_tssical_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	return cmn_info;

	/* error */
fail:
	phy_tssical_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_tssical_detach)(phy_tssical_info_t *cmn_info)
{
	phy_tssical_mem_t *mem;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	if (!cmn_info)
		return;

	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null tssical module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_tssical_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_tssical_register_impl)(phy_tssical_info_t *cmn_info, phy_type_tssical_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	*cmn_info->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_tssical_unregister_impl)(phy_tssical_info_t *cmn_info)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

int wlc_phy_tssivisible_thresh(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_tssical_info_t *ti = pi->tssicali;
	phy_type_tssical_fns_t *fns = ti->fns;
	int visi_thresh_qdbm = WL_RATE_DISABLED;

#if defined(WLOLPC)
	if (fns->visible_thresh != NULL) {
		visi_thresh_qdbm = (fns->visible_thresh)(fns->ctx);
	}
#endif /* WLOLPC */

	if (fns->visible_thresh == NULL) {
		visi_thresh_qdbm = (PHY_TXPWR_MIN_LEGACYPHY * WLC_TXPWR_DB_FACTOR);
	}

	return visi_thresh_qdbm;
}

void wlc_phy_get_tssi_sens_min(wlc_phy_t *ppi, int8 *tssiSensMinPwr)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_tssical_info_t *ti = pi->tssicali;
	phy_type_tssical_fns_t *fns = ti->fns;
	if (fns->sens_min != NULL) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
		(fns->sens_min)(fns->ctx, tssiSensMinPwr);
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
	}
	else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
	}
}

/* driver down processing */
int
phy_tssical_down(phy_tssical_info_t *cmn_info)
{
	int callbacks = 0;

	PHY_TRACE(("%s\n", __FUNCTION__));

	return callbacks;
}
