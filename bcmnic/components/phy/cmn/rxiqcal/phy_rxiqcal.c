/*
 * RXIQ CAL module implementation.
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
 * $Id: phy_rxiqcal.c 639978 2016-05-25 16:03:11Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rxiqcal.h"
#include <phy_rstr.h>
#include <phy_rxiqcal.h>

/* forward declaration */
typedef struct phy_rxiqcal_mem phy_rxiqcal_mem_t;

/* module private states */
struct phy_rxiqcal_priv_info {
	phy_info_t 				*pi;		/* PHY info ptr */
	phy_type_rxiqcal_fns_t	*fns;		/* Function ptr */
};

/* module private states memory layout */
struct phy_rxiqcal_mem {
	phy_rxiqcal_info_t		cmn_info;
	phy_type_rxiqcal_fns_t	fns;
	phy_rxiqcal_priv_info_t priv;
	phy_rxiqcal_data_t data;
/* add other variable size variables here at the end */
};

/* local function declaration */

/* attach/detach */
phy_rxiqcal_info_t *
BCMATTACHFN(phy_rxiqcal_attach)(phy_info_t *pi)
{
	phy_rxiqcal_info_t	*cmn_info = NULL;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((cmn_info = phy_malloc(pi, sizeof(phy_rxiqcal_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize params */
	cmn_info->priv = &((phy_rxiqcal_mem_t *)cmn_info)->priv;
	cmn_info->priv->pi = pi;
	cmn_info->priv->fns = &((phy_rxiqcal_mem_t *)cmn_info)->fns;
	cmn_info->data = &((phy_rxiqcal_mem_t *)cmn_info)->data;

	/* init the rxiqcal states */
	/* pi->phy_rx_diglpf_default_coeffs are not set yet */
	cmn_info->data->phy_rx_diglpf_default_coeffs_valid = FALSE;

	/* Register callbacks */

	return cmn_info;

	/* error */
fail:
	phy_rxiqcal_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_rxiqcal_detach)(phy_rxiqcal_info_t *cmn_info)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	if (cmn_info == NULL) {
		PHY_INFORM(("%s: null rxiqcal module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->priv->pi, cmn_info, sizeof(phy_rxiqcal_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_rxiqcal_register_impl)(phy_rxiqcal_info_t *cmn_info, phy_type_rxiqcal_fns_t *fns)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	*cmn_info->priv->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_rxiqcal_unregister_impl)(phy_rxiqcal_info_t *cmn_info)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	cmn_info->priv->fns = NULL;
}

void phy_rxiqcal_scanroam_cache(phy_info_t *pi, bool set)
{
	phy_type_rxiqcal_fns_t *fns = pi->rxiqcali->priv->fns;

	if (fns->scanroam_cache != NULL)
		(fns->scanroam_cache)(fns->ctx, set);
}
