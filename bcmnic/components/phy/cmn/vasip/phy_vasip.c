/*
 * vasip module implementation.
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
 * $Id: phy_vasip.c 659421 2016-09-14 06:45:22Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_vasip.h"
#include <phy_rstr.h>
#include <phy_vasip.h>
#include <phy_vasip_api.h>
#include <phy_utils_var.h>

/* forward declaration */
typedef struct phy_vasip_mem phy_vasip_mem_t;

/* module private states */
struct phy_vasip_info {
	phy_info_t		*pi;	/* PHY info ptr */
	phy_type_vasip_fns_t	*fns;	/* PHY specific function ptrs */
	phy_vasip_mem_t		*mem;	/* Memory layout ptr */
};

/* module private states memory layout */
struct phy_vasip_mem {
	phy_vasip_info_t cmn_info;
	phy_type_vasip_fns_t fns;
/* add other variable size variables here at the end */
};

/* local function declaration */

/* attach/detach */
phy_vasip_info_t *
BCMATTACHFN(phy_vasip_attach)(phy_info_t *pi)
{
	phy_vasip_mem_t	*mem = NULL;
	phy_vasip_info_t *cmn_info = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((mem = phy_malloc(pi, sizeof(phy_vasip_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	/* Initialize infra params */
	cmn_info = &(mem->cmn_info);
	cmn_info->pi = pi;
	cmn_info->fns = &(mem->fns);
	cmn_info->mem = mem;

	/* Register callbacks */

	return cmn_info;

	/* error */
fail:
	phy_vasip_detach(cmn_info);
	return NULL;
}

void
BCMATTACHFN(phy_vasip_detach)(phy_vasip_info_t *cmn_info)
{
	phy_vasip_mem_t	*mem;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* Clean up module related states */

	/* Clean up infra related states */
	/* Malloc has failed. No cleanup is necessary here. */
	if (!cmn_info)
		return;

	/* Freeup memory associated with cmn_info */
	mem = cmn_info->mem;

	if (mem == NULL) {
		PHY_INFORM(("%s: null vasip module\n", __FUNCTION__));
		return;
	}

	phy_mfree(cmn_info->pi, mem, sizeof(phy_vasip_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_vasip_register_impl)(phy_vasip_info_t *vi, phy_type_vasip_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));

	*vi->fns = *fns;

	return BCME_OK;
}

void
BCMATTACHFN(phy_vasip_unregister_impl)(phy_vasip_info_t *vi)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/*
 * Return vasip version, -1 if not present.
 */
uint8
phy_vasip_get_ver(phy_info_t *pi)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->get_ver != NULL)
		return fns->get_ver(fns->ctx);
	else
		return VASIP_NOVERSION;
}

/*
 * reset/activate vasip.
 */
void
phy_vasip_reset_proc(phy_info_t *pi, int reset)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->reset_proc != NULL)
		fns->reset_proc(fns->ctx, reset);
}

void
phy_vasip_set_clk(phy_info_t *pi, bool val)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->set_clk != NULL)
		fns->set_clk(fns->ctx, val);
}

void
phy_vasip_write_bin(phy_info_t *pi, const uint32 vasip_code[], const uint nbytes)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->write_bin != NULL) {
		fns->write_bin(fns->ctx, vasip_code, nbytes);
	}
}

#ifdef VASIP_SPECTRUM_ANALYSIS
void
phy_vasip_write_spectrum_tbl(phy_info_t *pi,
        const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->write_spectrum_tbl != NULL) {
		fns->write_spectrum_tbl(fns->ctx, vasip_spectrum_tbl, nbytes_tbl);
	}
}
#endif

void
phy_vasip_write_svmp(phy_info_t *pi, uint32 offset, uint16 val)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->write_svmp != NULL) {
		fns->write_svmp(fns->ctx, offset, val);
	}
}

void
phy_vasip_read_svmp(phy_info_t *pi, uint32 offset, uint16 *val)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->read_svmp != NULL) {
		fns->read_svmp(fns->ctx, offset, val);
	}
}

void
phy_vasip_write_svmp_blk(phy_info_t *pi, uint32 offset, uint16 len, uint16 *val)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->write_svmp_blk != NULL) {
		fns->write_svmp_blk(fns->ctx, offset, len, val);
	}
}

void
phy_vasip_read_svmp_blk(phy_info_t *pi, uint32 offset, uint16 len, uint16 *val)
{
	phy_type_vasip_fns_t *fns = pi->vasipi->fns;

	if (fns->read_svmp_blk != NULL) {
		fns->read_svmp_blk(fns->ctx, offset, len, val);
	}
}
