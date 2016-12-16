/*
 * LCN20PHY RADIO contorl module implementation
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
 * $Id: phy_lcn20_radio.c 659938 2016-09-16 16:47:54Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_radio.h>
#include "phy_type_radio.h"
#include <phy_lcn20.h>
#include <phy_lcn20_radio.h>

#include <wlc_phyreg_lcn20.h>
#include <wlc_phy_radio.h>
#include <phy_utils_reg.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_lcn20.h>
/* TODO: all these are going away... > */
#endif

#define IDCODE_LCN20PHY_ID_MASK   0xffff
#define IDCODE_LCN20PHY_ID_SHIFT  0
#define IDCODE_LCN20PHY_REV_MASK  0xffff0000
#define IDCODE_LCN20PHY_REV_SHIFT 16

/* module private states */
struct phy_lcn20_radio_info {
	phy_info_t *pi;
	phy_lcn20_info_t *lcn20i;
	phy_radio_info_t *ri;
};

#ifdef PHY_DUMP_BINARY
/* AUTOGENRATED by the tool : phyreg.py
 * These values cannot be in const memory since
 * the const area might be over-written in case of
 * crash dump
 */
phyradregs_list_t rad20692_majorrev0_registers[] = {
	{0x0,  {0x80, 0x0, 0x1, 0xb2}},
};
#endif /* PHY_DUMP_BINARY */

/* local functions */
static void _phy_lcn20_radio_switch(phy_type_radio_ctx_t *ctx, bool on);
static void phy_lcn20_radio_on(phy_type_radio_ctx_t *ctx);
#ifdef PHY_DUMP_BINARY
static int phy_lcn20_radio_getlistandsize(phy_type_radio_ctx_t *ctx, phyradregs_list_t **radreglist,
	uint16 *radreglist_sz);
#endif
static void phy_lcn20_radio_switch_band(phy_type_radio_ctx_t *ctx);
static uint32 _phy_lcn20_radio_query_idcode(phy_type_radio_ctx_t *ctx);

/* Register/unregister LCN20PHY specific implementation to common layer. */
phy_lcn20_radio_info_t *
BCMATTACHFN(phy_lcn20_radio_register_impl)(phy_info_t *pi, phy_lcn20_info_t *lcn20i,
	phy_radio_info_t *ri)
{
	phy_lcn20_radio_info_t *info;
	phy_type_radio_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_lcn20_radio_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_lcn20_radio_info_t));
	info->pi = pi;
	info->lcn20i = lcn20i;
	info->ri = ri;

	/* make sure the radio is off until we do an "up" */
	_phy_lcn20_radio_switch(info, OFF);

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctrl = _phy_lcn20_radio_switch;
	fns.on = phy_lcn20_radio_on;
#ifdef PHY_DUMP_BINARY
	fns.getlistandsize = phy_lcn20_radio_getlistandsize;
#endif
	fns.bandx = phy_lcn20_radio_switch_band;
	fns.id = _phy_lcn20_radio_query_idcode;
	fns.ctx = info;

	phy_radio_register_impl(ri, &fns);

	return info;

fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_lcn20_radio_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_lcn20_radio_unregister_impl)(phy_lcn20_radio_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_radio_info_t *ri = info->ri;

	phy_radio_unregister_impl(ri);

	phy_mfree(pi, info, sizeof(phy_lcn20_radio_info_t));
}

/* switch radio on/off */
static void
_phy_lcn20_radio_switch(phy_type_radio_ctx_t *ctx, bool on)
{
	phy_lcn20_radio_info_t *info = (phy_lcn20_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;

	wlc_lcn20phy_switch_radio(pi, on);
}

void
phy_lcn20_radio_switch(phy_lcn20_radio_info_t *info, bool on)
{
	_phy_lcn20_radio_switch(info, on);
}

/* turn radio on */
static void
WLBANDINITFN(phy_lcn20_radio_on)(phy_type_radio_ctx_t *ctx)
{
	_phy_lcn20_radio_switch(ctx, ON);
}

/* switch radio off when changing band */
static void
phy_lcn20_radio_switch_band(phy_type_radio_ctx_t *ctx)
{
	_phy_lcn20_radio_switch(ctx, OFF);
}

/* query radio idcode */
static uint32
_phy_lcn20_radio_query_idcode(phy_type_radio_ctx_t *ctx)
{
	phy_lcn20_radio_info_t *info = (phy_lcn20_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 idcode;

	idcode = phy_lcn20_radio_query_idcode(pi);
#ifdef BCMRADIOREV
	if (ISSIM_ENAB(pi->sh->sih)) {
		idcode = (idcode & ~IDCODE_REV_MASK) | (BCMRADIOREV << IDCODE_REV_SHIFT);
	}
#endif	/* BCMRADIOREV */

	return idcode;
}

uint32
phy_lcn20_radio_query_idcode(phy_info_t *pi)
{
	uint32 b0, b1;

	W_REG(pi->sh->osh, &pi->regs->radioregaddr, 0);
	b0 = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);
	W_REG(pi->sh->osh, &pi->regs->radioregaddr, 1);
	b1 = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);

	return ((b0 << IDCODE_LCN20PHY_REV_SHIFT) | (b1 & IDCODE_LCN20PHY_ID_MASK));
}

/* parse radio idcode and write to pi->pubpi */
void
phy_lcn20_radio_parse_idcode(phy_info_t *pi, uint32 idcode)
{
	PHY_TRACE(("%s: idcode 0x%08x\n", __FUNCTION__, idcode));

	pi->pubpi->radioid = (idcode & IDCODE_LCN20PHY_ID_MASK) >> IDCODE_LCN20PHY_ID_SHIFT;
	pi->pubpi->radiorev = (idcode & IDCODE_LCN20PHY_REV_MASK) >> IDCODE_LCN20PHY_REV_SHIFT;

	/* LCN20 does not use radio ver. This param is invalid */
	pi->pubpi->radiover = 0;
}

#ifdef PHY_DUMP_BINARY
/* The function is forced to RAM since it accesses non-const tables */
static int BCMRAMFN(phy_lcn20_radio_getlistandsize_20692)(phy_info_t *pi,
                    phyradregs_list_t **radreglist, uint16 *radreglist_sz)
{
	if (RADIOREV(pi->pubpi->radiorev) <= 5) {
		*radreglist = (phyradregs_list_t *) &rad20692_majorrev0_registers[0];
		*radreglist_sz = sizeof(rad20692_majorrev0_registers);
	} else {
		PHY_INFORM(("%s: wl%d: unsupported BCM20692 radio rev %d\n",
			__FUNCTION__,  pi->sh->unit,  pi->pubpi->radiorev));
		return BCME_UNSUPPORTED;
	}

	return BCME_OK;
}

static int phy_lcn20_radio_getlistandsize(phy_type_radio_ctx_t *ctx,
                    phyradregs_list_t **radreglist, uint16 *radreglist_sz)
{
	phy_lcn20_radio_info_t *info = (phy_lcn20_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int ret = BCME_UNSUPPORTED;
	if (RADIOID_IS(pi->pubpi->radioid, BCM20692_ID)) {
		ret = phy_lcn20_radio_getlistandsize_20692(pi, radreglist, radreglist_sz);
	} else {
		PHY_INFORM(("%s: wl%d: unsupported radio id %d\n",
			__FUNCTION__,  pi->sh->unit,  pi->pubpi->radioid));
	}
	return ret;
}
#endif /* PHY_DUMP_BINARY */

void
wlc_phy_get_radio_loft_lcn20phy(phy_info_t *pi,
	uint8 *ei0,
	uint8 *eq0,
	uint8 *fi0,
	uint8 *fq0)
{
	BCM_REFERENCE(ei0);
	BCM_REFERENCE(eq0);
	BCM_REFERENCE(fi0);
	BCM_REFERENCE(fq0);
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}
