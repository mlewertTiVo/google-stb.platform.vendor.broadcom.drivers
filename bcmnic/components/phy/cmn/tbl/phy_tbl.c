/*
 * PHYTableInit module implementation
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
 * $Id: phy_tbl.c 657351 2016-08-31 23:00:22Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_tbl.h"
#include <phy_init.h>
#include <phy_tbl.h>
#include <phy_utils_reg.h>

/* module private states */
struct phy_tbl_info {
	phy_info_t *pi;
	phy_type_tbl_fns_t *fns;
};

/* module private states memory layout */
typedef struct {
	phy_tbl_info_t info;
	phy_type_tbl_fns_t fns;
} phy_tbl_mem_t;

/* local function declaration */
static int phy_tbl_init(phy_init_ctx_t *ctx);
#ifndef BCMNODOWN
static int phy_tbl_down(phy_init_ctx_t *ctx);
#endif
#if defined(BCMDBG_PHYDUMP)
static int phy_tbl_dumptbl(void *ctx, struct bcmstrbuf *b);
static uint16 phytbl_page_parser(phy_table_info_t *ti);
static void phy_tbl_process_dumptbl(phy_tbl_info_t *info,
		phy_table_info_t *ti, struct bcmstrbuf *b);
#endif 

/* attach/detach */
phy_tbl_info_t *
BCMATTACHFN(phy_tbl_attach)(phy_info_t *pi)
{
	phy_tbl_info_t *info;
	phy_type_tbl_fns_t *fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate attach info storage */
	if ((info = phy_malloc(pi, sizeof(phy_tbl_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;

	/* init the type specific fns */
	fns = &((phy_tbl_mem_t *)info)->fns;
	info->fns = fns;

	/* register init fn */
	if (phy_init_add_init_fn(pi->initi, phy_tbl_init, info, PHY_INIT_PHYTBL) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto fail;
	}

#ifndef BCMNODOWN
	/* register down fn */
	if (phy_init_add_down_fn(pi->initi, phy_tbl_down, info, PHY_DOWN_PHYTBL) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_down_fn failed\n", __FUNCTION__));
		goto fail;
	}
#endif

	/* register dump callback */
#if defined(BCMDBG_PHYDUMP)
	phy_dbg_add_dump_fn(pi, "phytbl", phy_tbl_dumptbl, info);
#endif 
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	phy_dbg_add_dump_fn(pi, "phytxv0", phy_tbl_dump_txv0, info);
#endif /* BCMDBG || BCMDBG_DUMP */

	return info;

	/* error */
fail:
	phy_tbl_detach(info);
	return NULL;
}

void
BCMATTACHFN(phy_tbl_detach)(phy_tbl_info_t *info)
{
	phy_info_t *pi;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (info == NULL) {
		PHY_INFORM(("%s: null init module\n", __FUNCTION__));
		return;
	}

	pi = info->pi;

	phy_mfree(pi, info, sizeof(phy_tbl_mem_t));
}

/* register phy type specific implementations */
int
BCMATTACHFN(phy_tbl_register_impl)(phy_tbl_info_t *ii, phy_type_tbl_fns_t *fns)
{
	PHY_TRACE(("%s\n", __FUNCTION__));


	*ii->fns = *fns;
	return BCME_OK;
}

void
BCMATTACHFN(phy_tbl_unregister_impl)(phy_tbl_info_t *ii)
{
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/* init PHY tables */
static int
WLBANDINITFN(phy_tbl_init)(phy_init_ctx_t *ctx)
{
	phy_tbl_info_t *ii = (phy_tbl_info_t *)ctx;
	phy_type_tbl_fns_t *fns = ii->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	ASSERT(fns->init != NULL);
	return (fns->init)(fns->ctx);
}

#ifndef BCMNODOWN
/* down the h/w */
static int
BCMUNINITFN(phy_tbl_down)(phy_init_ctx_t *ctx)
{
	phy_tbl_info_t *ii = (phy_tbl_info_t *)ctx;
	phy_type_tbl_fns_t *fns = ii->fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	if (fns->down == NULL)
		return 0;

	return (fns->down)(fns->ctx);
}
#endif

#if defined(BCMDBG_PHYDUMP)
static int
phy_tbl_dumptbl(void *ctx, struct bcmstrbuf *b)
{
	phy_type_tbl_fns_t *fns = ((phy_tbl_info_t *)ctx)->fns;
	phy_info_t *pi = ((phy_tbl_info_t *)ctx)->pi;
	int ret = BCME_UNSUPPORTED;

	if (!pi->sh->clk)
		return BCME_NOCLK;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if (fns->dump)
		ret = (fns->dump)(fns->ctx, b);

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	return ret;
}

static uint16
phytbl_page_parser(phy_table_info_t *ti)
{
	uint16 np = 0;

	while (ti->table != 0xff) {
		np++; ti++;
	}

	return np;
}

static void
phy_tbl_process_dumptbl(phy_tbl_info_t *info, phy_table_info_t *ti, struct bcmstrbuf *b)
{
	phy_type_tbl_fns_t *fns = info->fns;
	phy_info_t *pi = info->pi;

	uint16 val = 0;
	uint16 qval[3] = {0, 0, 0};

	if (pi->pdpi->entry >= ti->max) {
		bcm_bprintf(b, "\n\nExceeding # of available entries (%d)\n\n", ti->max - 1);
		return;
	}

	for (; pi->pdpi->entry < ti->max; pi->pdpi->entry++) {
		if (fns->addrfltr != NULL &&
				!(fns->addrfltr)(pi, ti, pi->pdpi->entry)) {
			continue;
		}

		/* Check whether we have enough buffer left for print */
		if (b->size < 44)
			return;

		ASSERT(fns->readtbl != NULL);
		ASSERT(fns->ctx != NULL);

		(fns->readtbl)(fns->ctx, ti, pi->pdpi->entry, &val, &qval[0]);

		if (PHY_INFORM_ON() && si_taclear(pi->sh->sih, FALSE)) {
			PHY_INFORM(("%s: TA reading aphy table 0x%02X:0x%04X\n", __FUNCTION__,
				ti->table, pi->pdpi->entry));
			bcm_bprintf(b, "0x%02X:0x%04X tabort\n", ti->table, pi->pdpi->entry);
		} else if (!si_taclear(pi->sh->sih, FALSE)) {
			if (ti->q == 1) {
				bcm_bprintf(b, "0x%02X:0x%04X = 0x%04X 0x%04X\n", ti->table,
					pi->pdpi->entry, val, qval[0]);
			} else if (ti->q == 2) {
				bcm_bprintf(b, "0x%02X:0x%04X = 0x%04X 0x%04X 0x%04X\n",
					ti->table, pi->pdpi->entry, val, qval[1], qval[0]);
			} else if (ti->q == 3) {
				bcm_bprintf(b, "0x%02X:0x%04X = 0x%04X 0x%04X 0x%04X 0x%04X\n",
					ti->table, pi->pdpi->entry, val, qval[2], qval[1], qval[0]);
			} else {
				bcm_bprintf(b, "0x%02X:0x%04X = 0x%04X\n",
					ti->table, pi->pdpi->entry, val);
			}
		}
	}
	/* Wrap around entry when reaching end */
	if (pi->pdpi->entry >= ti->max)
		pi->pdpi->entry = 0;
}

void
phy_tbl_do_dumptbl(phy_tbl_info_t *info, phy_table_info_t *ti, struct bcmstrbuf *b)
{
	phy_type_tbl_fns_t *fns = info->fns;
	phy_info_t *pi = info->pi;
	uint16 np = 0;

	ASSERT(ti != NULL);
	np = phytbl_page_parser(ti);
	/* point to the right page */
	if (pi->pdpi->page >= np) {
		bcm_bprintf(b, "\n\nExceeding # of available pages (%d)\n\n", np - 1);
		return;
	}
	else {
		ti = &ti[pi->pdpi->page];
	}

	if (ISACPHY(pi)) {
		phy_tbl_process_dumptbl(info, ti, b);

	} else if (TRUE &&
			!(fns->tblfltr && !(fns->tblfltr)(pi, ti))) {
		phy_tbl_process_dumptbl(info, ti, b);
	}

	/* Increment page and wrap around */
	if ((pi->pdpi->entry == 0) && (++pi->pdpi->page >= np))
		pi->pdpi->page = 0;
}
#endif 

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
int
phy_tbl_dump_txv0(void *ctx, struct bcmstrbuf *b)
{
	phy_tbl_info_t *tbli = (phy_tbl_info_t *)ctx;
	phy_type_tbl_fns_t *fns = tbli->fns;

	if (fns->dump_txv0 != NULL) {
		return (fns->dump_txv0)(fns->ctx, b);
	} else {
		return BCME_UNSUPPORTED;
	}
}
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */
