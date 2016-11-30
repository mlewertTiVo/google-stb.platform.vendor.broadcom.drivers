/*
 * Rx Gain Control and Carrier Sense module implementation.
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
 * $Id: phy_rxgcrs.c 644994 2016-06-22 06:23:44Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_rxgcrs_api.h>
#include "phy_type_rxgcrs.h"
#include <phy_rstr.h>
#include <phy_rxgcrs.h>

/* forward declaration */
typedef struct phy_rxgcrs_mem phy_rxgcrs_mem_t;

/* module private states */
struct phy_rxgcrs_info {
	phy_info_t 				*pi;	/* PHY info ptr */
	phy_type_rxgcrs_fns_t	*fns;	/* PHY specific function ptrs */
	phy_rxgcrs_mem_t			*mem;	/* Memory layout ptr */
	uint8 region_group;
};

/* module private states memory layout */
struct phy_rxgcrs_mem {
	phy_rxgcrs_info_t		cmn_info;
	phy_type_rxgcrs_fns_t	fns;
/* add other variable size variables here at the end */
};

/* local function declaration */
static int phy_rxgcrs_init(phy_init_ctx_t *ctx);

/* attach/detach */
phy_rxgcrs_info_t *
BCMATTACHFN(phy_rxgcrs_attach)(phy_info_t *pi)
{
	phy_rxgcrs_mem_t	*mem = NULL;
	phy_rxgcrs_info_t	*cmn_info = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_rxgcrs_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize infra params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	/* Initialize rxgcrs params */
	cmn_info->region_group = REGION_OTHER;

	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_rxgcrs_init, cmn_info, PHY_INIT_RXG) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

	return cmn_info;

	/* error */
fail:
	phy_rxgcrs_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_rxgcrs_detach)(phy_rxgcrs_info_t *cmn_info)
{
	phy_rxgcrs_mem_t	*mem;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	/* Malloc has failed. No cleanup is necessary here. */
	if (!cmn_info)
		return;

	/* Freeup memory associated with cmn_info. */
	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null rxgcrs module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_rxgcrs_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_rxgcrs_register_impl)(phy_rxgcrs_info_t *mi, phy_type_rxgcrs_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	*mi->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_rxgcrs_unregister_impl)(phy_rxgcrs_info_t *mi)
{
	BCM_REFERENCE(mi);
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/* Rx Gain Control Carrier Sense init */
static int
WLBANDINITFN(phy_rxgcrs_init)(phy_init_ctx_t *ctx)
{
	phy_rxgcrs_info_t *rxgcrsi = (phy_rxgcrs_info_t *)ctx;
	phy_type_rxgcrs_fns_t *fns = rxgcrsi->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->init_rxgcrs != NULL) {
		return (fns->init_rxgcrs)(fns->ctx);
	}
	return BCME_OK;
}

/* driver down processing */
int
phy_rxgcrs_down(phy_rxgcrs_info_t *mi)
{
	int callbacks = 0;
	BCM_REFERENCE(mi);
	PHY_TRACE(("%s\n", __FUNCTION__));

	return callbacks;
}


/* When multiple PHY supported, then need to walk thru all instances of pi */
void
wlc_phy_set_locale(phy_info_t *pi, uint8 region_group)
{
	phy_rxgcrs_info_t *info = pi->rxgcrsi;
	phy_type_rxgcrs_fns_t *fns = info->fns;

	info->region_group = region_group;

	if (fns->set_locale != NULL)
		(fns->set_locale)(fns->ctx, region_group);
}

uint8
wlc_phy_get_locale(phy_rxgcrs_info_t *info)
{
	return info->region_group;
}

int
wlc_phy_adjust_ed_thres(phy_info_t *pi, int32 *assert_thresh_dbm, bool set_threshold)
{
	phy_rxgcrs_info_t *info = pi->rxgcrsi;
	phy_type_rxgcrs_fns_t *fns = info->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (NULL != fns->adjust_ed_thres)
	{
		fns->adjust_ed_thres(fns->ctx, assert_thresh_dbm, set_threshold);
		return BCME_OK;
	}

	return BCME_UNSUPPORTED;
}

/* Rx desense Module */
#if defined(RXDESENS_EN)
int
phy_rxgcrs_get_rxdesens(phy_info_t *pi, int32 *ret_int_ptr)
{
	phy_rxgcrs_info_t *info = pi->rxgcrsi;
	phy_type_rxgcrs_fns_t *fns = info->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));
	if (fns->get_rxdesens != NULL) {
		return (fns->get_rxdesens)(fns->ctx, ret_int_ptr);
	} else {
		return BCME_UNSUPPORTED;
	}
}

int
phy_rxgcrs_set_rxdesens(phy_info_t *pi, int32 int_val)
{
	phy_rxgcrs_info_t *info = pi->rxgcrsi;
	phy_type_rxgcrs_fns_t *fns = info->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));
	if (fns->set_rxdesens != NULL) {
		return (fns->set_rxdesens)(fns->ctx, int_val);
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* RXDESENS_EN */

int
wlc_phy_set_rx_gainindex(phy_info_t *pi, int32 gain_idx)
{
	phy_rxgcrs_info_t *info = pi->rxgcrsi;
	phy_type_rxgcrs_fns_t *fns = info->fns;

	if (!pi->sh->up) {
		return BCME_NOTUP;
	}

	PHY_INFORM(("wlc_phy_set_rx_gainindex Called\n"));
	if (fns->set_rxgainindex != NULL) {
		return (fns->set_rxgainindex)(fns->ctx, gain_idx);
	} else {
		return BCME_UNSUPPORTED;
	}
}

int
wlc_phy_get_rx_gainindex(phy_info_t *pi, int32 *gain_idx)
{
	phy_rxgcrs_info_t *info = pi->rxgcrsi;
	phy_type_rxgcrs_fns_t *fns = info->fns;

	if (!pi->sh->up) {
		return BCME_NOTUP;
	}

	PHY_INFORM(("wlc_phy_get_rx_gainindex Called\n"));
	if (fns->get_rxgainindex != NULL) {
		return (fns->get_rxgainindex)(fns->ctx, gain_idx);
	} else {
		return BCME_UNSUPPORTED;
	}
}

#ifndef ATE_BUILD
#if defined(BCMINTERNAL) || defined(WLTEST) || defined(DBG_PHY_IOV) || \
	defined(WFD_PHY_LL_DEBUG)
/* JIRA: SWWLAN-32606, RB: 12975: function to do only Noise cal & read crsmin power of
 * core 0 & core 1
*/
int
wlc_phy_iovar_forcecal_noise(phy_info_t *pi, void *a, bool set)
{
	phy_rxgcrs_info_t *info = pi->rxgcrsi;
	phy_type_rxgcrs_fns_t *fns = info->fns;

	if (fns->forcecal_noise != NULL) {
		return (fns->forcecal_noise)(fns->ctx, a, set);
	} else {
		PHY_INFORM(("%s: No phy specific function\n", __FUNCTION__));
		return BCME_UNSUPPORTED;
	}
}
#endif /* BCMINTERNAL || WLTEST || ATE_BUILD */
#endif /* !ATE_BUILD */
