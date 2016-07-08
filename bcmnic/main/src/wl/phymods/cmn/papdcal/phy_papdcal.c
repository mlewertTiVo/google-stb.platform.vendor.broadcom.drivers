/*
 * PAPD CAL module implementation.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
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
#include "phy_type_papdcal.h"
#include <phy_rstr.h>
#include <phy_papdcal.h>

/* forward declaration */
typedef struct phy_papdcal_mem phy_papdcal_mem_t;

/* module private states */
struct phy_papdcal_info {
	phy_info_t 				*pi;		/* PHY info ptr */
	phy_type_papdcal_fns_t	*fns;		/* Function ptr */
	phy_papdcal_mem_t		*mem;		/* Memory layout ptr */
};

/* module private states memory layout */
struct phy_papdcal_mem {
	phy_papdcal_info_t		cmn_info;
	phy_type_papdcal_fns_t	fns;
/* add other variable size variables here at the end */
};

/* local function declaration */
static int phy_papdcal_init_cb(phy_init_ctx_t *ctx);


/* attach/detach */
phy_papdcal_info_t *
BCMATTACHFN(phy_papdcal_attach)(phy_info_t *pi)
{
	phy_papdcal_mem_t	*mem;
	phy_papdcal_info_t *cmn_info = NULL;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_papdcal_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	/* init the papdcal states */

	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_papdcal_init_cb, cmn_info,
		PHY_INIT_PAPDCAL) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}


	return cmn_info;

	/* error */
fail:
	phy_papdcal_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_papdcal_detach)(phy_papdcal_info_t *cmn_info)
{
	phy_papdcal_mem_t	*mem;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* Malloc has failed. No cleanup is necessary here. */
	if (!cmn_info)
		return;

	/* Freeup memory associated with cmn_info. */
	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null papdcal module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_papdcal_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_papdcal_register_impl)(phy_papdcal_info_t *cmn_info, phy_type_papdcal_fns_t *fns)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	*cmn_info->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_papdcal_unregister_impl)(phy_papdcal_info_t *cmn_info)
{
	PHY_CAL(("%s\n", __FUNCTION__));

	ASSERT(cmn_info);

	cmn_info->fns = NULL;
}

/* papdcal init processing */
static int
WLBANDINITFN(phy_papdcal_init_cb)(phy_init_ctx_t *ctx)
{
	PHY_CAL(("%s\n", __FUNCTION__));

		return BCME_OK;
}
