/*
 * NPHY RADIO contorl module implementation
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

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_radio.h>
#include "phy_type_radio.h"
#include <phy_n.h>
#include <phy_n_radio.h>
#include "phy_utils_reg.h"

#include <wlc_phy_radio.h>

/* module private states */
struct phy_n_radio_info {
	phy_info_t *pi;
	phy_n_info_t *ni;
	phy_radio_info_t *ri;
};

/* local functions */
static void phy_n_radio_switch(phy_type_radio_ctx_t *ctx, bool on);
static void phy_n_radio_on(phy_type_radio_ctx_t *ctx);
static void phy_n_radio_off_bandx(phy_type_radio_ctx_t *ctx);
static void phy_n_radio_off_init(phy_type_radio_ctx_t *ctx);
static uint32 _phy_n_radio_query_idcode(phy_type_radio_ctx_t *ctx);
#define phy_n_radio_dump NULL

/* Register/unregister NPHY specific implementation to common layer */
phy_n_radio_info_t *
BCMATTACHFN(phy_n_radio_register_impl)(phy_info_t *pi, phy_n_info_t *ni,
	phy_radio_info_t *ri)
{
	phy_n_radio_info_t *info;
	phy_type_radio_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_n_radio_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	bzero(info, sizeof(phy_n_radio_info_t));
	info->pi = pi;
	info->ni = ni;
	info->ri = ri;

	if (NREV_GE(pi->pubpi.phy_rev, 7)) {
		if (NREV_IS(pi->pubpi.phy_rev, 19))
			pi->pubpi.radiooffset = RADIO_20671_READ_OFF;
		else
			pi->pubpi.radiooffset = RADIO_2057_READ_OFF;
	} else
		pi->pubpi.radiooffset = RADIO_2055_READ_OFF;  /* works for 2056 too */

#ifndef BCM_OL_DEV
	/* make sure the radio is off until we do an "up" */
	phy_n_radio_switch(info, OFF);
#endif

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctrl = phy_n_radio_switch;
	fns.on = phy_n_radio_on;
	fns.bandx = phy_n_radio_off_bandx;
	fns.init = phy_n_radio_off_init;
	fns.id = _phy_n_radio_query_idcode;
	fns.dump = phy_n_radio_dump;
	fns.ctx = info;

	phy_radio_register_impl(ri, &fns);

	return info;

fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_n_radio_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_n_radio_unregister_impl)(phy_n_radio_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_radio_info_t *ri = info->ri;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_radio_unregister_impl(ri);

	phy_mfree(pi, info, sizeof(phy_n_radio_info_t));
}

/* switch radio on/off */
static void
phy_n_radio_switch(phy_type_radio_ctx_t *ctx, bool on)
{
	phy_n_radio_info_t *info = (phy_n_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;

	PHY_TRACE(("wl%d: %s %d\n", pi->sh->unit, __FUNCTION__, on));

	wlc_phy_switch_radio_nphy(pi, on);
}

/* turn radio on */
static void
WLBANDINITFN(phy_n_radio_on)(phy_type_radio_ctx_t *ctx)
{
	phy_n_radio_switch(ctx, ON);
}

/* switch radio off when switching band */
static void
phy_n_radio_off_bandx(phy_type_radio_ctx_t *ctx)
{
	/* the radio is left off when switching band if we do it here */
	/* temporarily turn it off until the problem is understood and fixed */
	phy_n_radio_info_t *info = (phy_n_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;

	(void)pi;

	if (NREV_GE(pi->pubpi.phy_rev, 3))
		return;

	phy_n_radio_switch(ctx, OFF);
}

/* switch radio off when initializing */
static void
phy_n_radio_off_init(phy_type_radio_ctx_t *ctx)
{
	phy_n_radio_switch(ctx, OFF);
}

/* query radio idcode */
static uint32
_phy_n_radio_query_idcode(phy_type_radio_ctx_t *ctx)
{
	phy_n_radio_info_t *info = (phy_n_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;

	return phy_n_radio_query_idcode(pi);
}

uint32
phy_n_radio_query_idcode(phy_info_t *pi)
{
	uint32 idcode;

	if (NREV_GE(pi->pubpi.phy_rev, LCNXN_BASEREV + 3)) {
		uint32 rnum;

		W_REG(pi->sh->osh, &pi->regs->radioregaddr, 0);
		rnum = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);
		if (CHIP_4324_B3(pi) || CHIP_4324_B4(pi) || CHIP_4324_B5(pi)) {
			rnum = (rnum >> 4) & 0xF;
		} else {
			rnum = (rnum >> 0) & 0xF;
		}
		if (CHIPID_4324X_MEDIA_A1(pi)) {
			ASSERT(rnum == 1);
			/* patch radiorev to 2 to differentiate from 4324B4 */
			rnum = 2;
		}

		W_REG(pi->sh->osh, &pi->regs->radioregaddr, 1);
		idcode = (R_REG(pi->sh->osh, &pi->regs->radioregdata) <<
			IDCODE_ID_SHIFT) | (rnum << IDCODE_REV_SHIFT);
	}
	else if (D11REV_GE(pi->sh->corerev, 24)) {
		uint32 b0, b1, b2;

		W_REG(pi->sh->osh, &pi->regs->radioregaddr, 0);
#ifdef __mips__
		(void)R_REG(pi->sh->osh, &pi->regs->radioregaddr);
#endif
		b0 = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);
		W_REG(pi->sh->osh, &pi->regs->radioregaddr, 1);
#ifdef __mips__
		(void)R_REG(pi->sh->osh, &pi->regs->radioregaddr);
#endif
		b1 = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);

		W_REG(pi->sh->osh, &pi->regs->radioregaddr, 2);
#ifdef __mips__
		(void)R_REG(pi->sh->osh, &pi->regs->radioregaddr);
#endif
		b2 = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);
		idcode = ((b0  & 0xf) << 28) | (((b2 << 8) | b1) << 12) | ((b0 >> 4) & 0xf);
	}
	else {
		W_REG(pi->sh->osh, &pi->regs->phy4waddr, RADIO_IDCODE);
#ifdef __mips__
		(void)R_REG(pi->sh->osh, &pi->regs->phy4waddr);
#endif
		idcode = (uint32)R_REG(pi->sh->osh, &pi->regs->phy4wdatalo);
		idcode |= (uint32)R_REG(pi->sh->osh, &pi->regs->phy4wdatahi) << 16;
	}

	return idcode;
}
