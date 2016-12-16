/*
 * NPHY RADIO contorl module implementation
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
 * $Id: phy_n_radio.c 662291 2016-09-29 02:58:11Z $
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
#include <phy_radio_api.h>

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
#if (defined(BCMDBG) || defined(BCMDBG_DUMP)) && defined(DBG_PHY_IOV)
static int phy_n_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_n_radio_dump NULL
#endif

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

	if (NREV_IS(pi->pubpi->phy_rev, 19))
		pi->pubpi->radiooffset = RADIO_20671_READ_OFF;
	else
		pi->pubpi->radiooffset = RADIO_2057_READ_OFF;

	/* make sure the radio is off until we do an "up" */
	phy_n_radio_switch(info, OFF);

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
	UNUSED_PARAMETER(ctx);
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
	uint32 idcode;

	idcode = phy_n_radio_query_idcode(pi);
#ifdef BCMRADIOREV
	if (ISSIM_ENAB(pi->sh->sih)) {
		idcode = (idcode & ~IDCODE_REV_MASK) | (BCMRADIOREV << IDCODE_REV_SHIFT);
	}
#endif	/* BCMRADIOREV */

	return idcode;
}

uint32
phy_n_radio_query_idcode(phy_info_t *pi)
{
	uint32 idcode;

	if (NREV_GE(pi->pubpi->phy_rev, LCNXN_BASEREV + 3)) {
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

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
#if defined(DBG_PHY_IOV)
static int
phy_n_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_n_radio_info_t *gi = (phy_n_radio_info_t *)ctx;
	phy_info_t *pi = gi->pi;
	const char *name = NULL;
	int i;
	uint16 addr = 0;
	radio_20xx_regs_t *radio20xxregs = NULL;
	radio_20671_regs_t *radio20671regs = NULL;

	if (RADIOID(pi->pubpi->radioid) == BCM2057_ID) {
		switch (RADIOREV(pi->pubpi->radiorev)) {
		case 3:
			radio20xxregs = regs_2057_rev4;
			break;
		case 4:
			radio20xxregs = regs_2057_rev4;
			break;
		case 5:
			if (RADIOVER(pi->pubpi->radiover) == 0x0) {
				/* A0 */
				radio20xxregs = regs_2057_rev5;
			} else if (RADIOVER(pi->pubpi->radiover) == 0x1) {
				/* B0 */
				radio20xxregs = regs_2057_rev5v1;
			}
			break;
		case 6:
			radio20xxregs = regs_2057_rev4;
			break;
		case 7:
			radio20xxregs = regs_2057_rev7;
			break;
		case 8:
			radio20xxregs = regs_2057_rev8;
			break;
		case 9:
			radio20xxregs = regs_2057_rev9;
			break;
		case 10:
			radio20xxregs = regs_2057_rev10;
			break;
		case 12:
			radio20xxregs = regs_2057_rev12;
			break;
		case 13:
			radio20xxregs = regs_2057_rev13;
			break;
		case 14:
			if (RADIOVER(pi->pubpi->radiover) == 1)
				radio20xxregs = regs_2057_rev14v1;
			else
				radio20xxregs = regs_2057_rev14;
			break;
		default:
			PHY_ERROR(("Unsupported radio rev %d\n", RADIOREV(pi->pubpi->radiorev)));
			ASSERT(0);
			break;
		}
		name = "2057";
	} else if (RADIOID(pi->pubpi->radioid) == BCM20671_ID) {
		switch (RADIOREV(pi->pubpi->radiorev)) {
		case 0:
			radio20671regs = regs_20671_rev0;
			break;
		case 1:
			if (RADIOVER(pi->pubpi->radiover) == 0)
				radio20671regs = regs_20671_rev1;
			else if (RADIOVER(pi->pubpi->radiover) == 1)
				radio20671regs = regs_20671_rev1_ver1;
			break;
		default:
			PHY_ERROR(("Unsupported radio rev %d\n", RADIOREV(pi->pubpi->radiorev)));
			ASSERT(0);
			break;
		}
		name = "20671";
	}

	if (name == NULL)
		return BCME_ERROR;

	bcm_bprintf(b, "----- %08s -----\n", name);
	bcm_bprintf(b, "Add Value\n");

	i = 0;
	while (TRUE) {
		if (radio20671regs) {
			addr = radio20671regs[i].address;
		} else if (radio20xxregs) {
			addr = radio20xxregs[i].address;
		} else {
			addr = 0xffff;
		}

		if (addr == 0xffff)
			break;

		bcm_bprintf(b, "%03x %04x\n", addr, phy_utils_read_radioreg(pi, addr));
		i++;
	}

	return BCME_OK;
}
#endif 
#endif /* BCMDBG || BCMDBG_DUMP */
