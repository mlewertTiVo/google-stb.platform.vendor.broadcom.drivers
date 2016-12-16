/*
 * Link Power Control module implementation.
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
 * $Id: phy_lpc.c 618416 2016-02-11 01:05:38Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_lpc.h"
#include <phy_rstr.h>
#include <phy_lpc.h>
#include <phy_lpc_api.h>

/* forward declaration */
typedef struct phy_lpc_mem phy_lpc_mem_t;

/* module private states */
struct phy_lpc_info {
	phy_info_t			*pi;	/* PHY info ptr */
	phy_type_lpc_fns_t	*fns;	/* PHY specific function ptrs */
	phy_lpc_mem_t		*mem;	/* Memory layout ptr */
};

/* module private states memory layout */
struct phy_lpc_mem {
	phy_lpc_info_t		cmn_info;
	phy_type_lpc_fns_t	fns;
/* add other variable size variables here at the end */
};

/* attach/detach */
phy_lpc_info_t *
BCMATTACHFN(phy_lpc_attach)(phy_info_t *pi)
{
	phy_lpc_mem_t	*mem = NULL;
	phy_lpc_info_t	*cmn_info = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_lpc_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize infra params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	/* Initialize lpc params */

	/* Register callbacks */

	return cmn_info;

	/* error */
fail:
	phy_lpc_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_lpc_detach)(phy_lpc_info_t *cmn_info)
{
	phy_lpc_mem_t	*mem;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	/* Malloc has failed. No cleanup is necessary here. */
	if (!cmn_info)
		return;

	/* Freeup memory associated with cmn_info */
	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null lpc module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_lpc_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_lpc_register_impl)(phy_lpc_info_t *cmn_info, phy_type_lpc_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	*cmn_info->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_lpc_unregister_impl)(phy_lpc_info_t *cmn_info)
{
	BCM_REFERENCE(cmn_info);
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/* driver down processing */
int
phy_lpc_down(phy_lpc_info_t *cmn_info)
{
	int callbacks = 0;
	BCM_REFERENCE(cmn_info);
	PHY_TRACE(("%s\n", __FUNCTION__));

	return callbacks;
}

/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
uint8
wlc_phy_lpc_getminidx(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_lpc_fns_t *fns = pi->lpci->fns;
	if (fns->getminidx)
		return (fns->getminidx)();
	return 0;
}

uint8
wlc_phy_lpc_getoffset(wlc_phy_t *ppi, uint8 index)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_lpc_fns_t *fns = pi->lpci->fns;
	if (fns->getpwros)
		return (fns->getpwros)(index);
	return 0;
}

uint8
wlc_phy_lpc_get_txcpwrval(wlc_phy_t *ppi, uint16 phytxctrlword)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_lpc_fns_t *fns = pi->lpci->fns;
	if (fns->gettxcpwrval)
		return (fns->gettxcpwrval)(phytxctrlword);
	return 0;
}

void
wlc_phy_lpc_set_txcpwrval(wlc_phy_t *ppi, uint16 *phytxctrlword, uint8 txcpwrval)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_lpc_fns_t *fns = pi->lpci->fns;
	if (fns->settxcpwrval)
		(fns->settxcpwrval)(phytxctrlword, txcpwrval);
	return;
}

void
wlc_phy_lpc_algo_set(wlc_phy_t *ppi, bool enable)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_lpc_fns_t *fns = pi->lpci->fns;
	pi->lpc_algo = enable;
	if (fns->setmode)
		(fns->setmode)(fns->ctx, enable);
	return;
}

#ifdef WL_LPC_DEBUG
uint8 *
wlc_phy_lpc_get_pwrlevelptr(wlc_phy_t *ppi)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	phy_type_lpc_fns_t *fns = pi->lpci->fns;
	if (fns->getpwrlevelptr)
		return (fns->getpwrlevelptr)();
	return 0;
}
#endif
