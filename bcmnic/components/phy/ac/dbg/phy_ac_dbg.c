/*
 * ACPHY Debug modules implementation
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
 * $Id: phy_ac_dbg.c 620395 2016-02-23 01:15:14Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_dbg.h"
#include <phy_ac.h>
#include <phy_ac_dbg.h>
#include <phy_ac_info.h>
#include <phy_utils_reg.h>
/* *************************** */
/* Modules used by this module */
/* *************************** */
#include <wlc_phyreg_ac.h>

/* module private states */
struct phy_ac_dbg_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_dbg_info_t *info;
};

/* local functions */
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void wlc_acphy_txerr_dump(phy_type_dbg_ctx_t *ctx, uint16 err);
#else
#define wlc_acphy_txerr_dump NULL
#endif
#if defined(DNG_DBGDUMP)
static void wlc_acphy_print_phydbg_regs(phy_type_dbg_ctx_t *ctx);
#else
#define wlc_acphy_print_phydbg_regs NULL
#endif /* DNG_DBGDUMP */
#ifdef WL_MACDBG
static void phy_ac_dbg_gpio_out_enab(phy_type_dbg_ctx_t *ctx, bool enab);
#else
#define phy_ac_dbg_gpio_out_enab NULL
#endif
#ifdef PHY_DUMP_BINARY
static uint16 phy_ac_dbg_get_phyreg_address(phy_type_dbg_ctx_t *ctx, uint16 addr);
#endif
/* register phy type specific implementation */
phy_ac_dbg_info_t *
BCMATTACHFN(phy_ac_dbg_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_dbg_info_t *info)
{
	phy_ac_dbg_info_t *di;
	phy_type_dbg_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((di = phy_malloc(pi, sizeof(*di))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	di->pi = pi;
	di->aci = aci;
	di->info = info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = di;
	fns.txerr_dump = wlc_acphy_txerr_dump;
	fns.print_phydbg_regs = wlc_acphy_print_phydbg_regs;
	fns.gpio_out_enab = phy_ac_dbg_gpio_out_enab;
#ifdef PHY_DUMP_BINARY
	fns.phyregaddr = phy_ac_dbg_get_phyreg_address;
#endif
	if (phy_dbg_register_impl(info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_dbg_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return di;

	/* error handling */
fail:
	if (di != NULL)
		phy_mfree(pi, di, sizeof(*di));
	return NULL;
}

void
BCMATTACHFN(phy_ac_dbg_unregister_impl)(phy_ac_dbg_info_t *di)
{
	phy_info_t *pi;
	phy_dbg_info_t *info;

	ASSERT(di);

	pi = di->pi;
	info = di->info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_dbg_unregister_impl(info);

	phy_mfree(pi, di, sizeof(*di));
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static const bcm_bit_desc_t attr_flags_phy_err[] = {
	{ACPHY_TxError_NDPError_MASK(0), "NDPError"},
	{ACPHY_TxError_RsvdBitError_MASK(0), "RsvdBitError"},
	{ACPHY_TxError_illegal_frame_type_MASK(0), "Illegal frame type"},
	{ACPHY_TxError_COMBUnsupport_MASK(0), "COMBUnsupport"},
	{ACPHY_TxError_BWUnsupport_MASK(0), "BWUnsupport"},
	{ACPHY_TxError_txInCal_MASK(0), "txInCal_MASK"},
	{ACPHY_TxError_send_frame_low_MASK(0), "send_frame_low"},
	{ACPHY_TxError_lengthmismatch_short_MASK(0), "lengthmismatch_short"},
	{ACPHY_TxError_lengthmismatch_long_MASK(0), "lengthmismatch_long"},
	{ACPHY_TxError_invalidRate_MASK(0), "invalidRate_MASK"},
	{ACPHY_TxError_unsupportedmcs_MASK(0), "unsupported mcs"},
	{ACPHY_TxError_send_frame_low_MASK(0), "send_frame_low"},
	{0, NULL}
};

static void
wlc_acphy_txerr_dump(phy_type_dbg_ctx_t *ctx, uint16 err)
{
	if (err != 0) {
		char flagstr[128];
		bcm_format_flags(attr_flags_phy_err, err, flagstr, 128);
		printf("Tx PhyErr 0x%04x (%s)\n", err, flagstr);
	}
}
#endif /* BCMDBG || BCMDBG_DUMP */

#if defined(DNG_DBGDUMP)
static void
wlc_acphy_print_phydbg_regs(phy_type_dbg_ctx_t *ctx)
{
	phy_ac_dbg_info_t *di = (phy_ac_dbg_info_t *)ctx;
	phy_info_t *pi = di->pi;
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		uint16 phymode = phy_get_phymode(pi);
		PHY_ERROR(("*** [PHY_DBG] *** : wl%d:: PHYMODE: 0x%x\n", pi->sh->unit, phymode));

		if ((phymode == PHYMODE_RSDB) &&
			(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE0)) {
			PHY_ERROR(("*** [PHY_DBG] *** : RSDB Cr 0: wl isup: %d\n", pi->sh->up));
		} else if ((phymode == PHYMODE_RSDB) &&
			(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1)) {
			PHY_ERROR(("*** [PHY_DBG] *** : RSDB Cr 1: wl isup: %d\n", pi->sh->up));
		} else {
			PHY_ERROR(("*** [PHY_DBG] *** : MIMO : wl isup: %d\n", pi->sh->up));
		}

		PHY_ERROR(("*** [PHY_DBG] *** : RxFeStatus: 0x%x\n",
			READ_PHYREG(pi, RxFeStatus)));
		PHY_ERROR(("*** [PHY_DBG] *** : TxFIFOStatus0: 0x%x\n",
			READ_PHYREG(pi, TxFIFOStatus0)));
		PHY_ERROR(("*** [PHY_DBG] *** : TxFIFOStatus1: 0x%x\n",
			READ_PHYREG(pi, TxFIFOStatus1)));
		PHY_ERROR(("*** [PHY_DBG] *** : RfseqMode: 0x%x\n",
			READ_PHYREG(pi, RfseqMode)));
		PHY_ERROR(("*** [PHY_DBG] *** : RfseqStatus0: 0x%x\n",
			READ_PHYREG(pi, RfseqStatus0)));
		PHY_ERROR(("*** [PHY_DBG] *** : RfseqStatus1: 0x%x\n",
			READ_PHYREG(pi, RfseqStatus1)));
		PHY_ERROR(("*** [PHY_DBG] *** : bphyTxError: 0x%x\n",
			READ_PHYREG(pi, bphyTxError)));
		PHY_ERROR(("*** [PHY_DBG] *** : TxCCKError: 0x%x\n",
			READ_PHYREG(pi, TxCCKError)));
		PHY_ERROR(("*** [PHY_DBG] *** : TxError: 0x%x\n",
			READ_PHYREG(pi, TxError)));
	}
}
#endif /* DNG_DBGDUMP */

#ifdef WL_MACDBG
static void
phy_ac_dbg_gpio_out_enab(phy_type_dbg_ctx_t *ctx, bool enab)
{
	phy_ac_dbg_info_t *di = (phy_ac_dbg_info_t *)ctx;
	phy_info_t *pi = di->pi;

	if (D11REV_LT(pi->sh->corerev, 50)) {
		W_REG(pi->sh->osh, &pi->regs->phyregaddr, ACPHY_gpioLoOutEn(pi->pubpi->phy_rev));
		W_REG(pi->sh->osh, &pi->regs->phyregdata, 0);
		W_REG(pi->sh->osh, &pi->regs->phyregaddr, ACPHY_gpioHiOutEn(pi->pubpi->phy_rev));
		W_REG(pi->sh->osh, &pi->regs->phyregdata, 0);
	}
}
#endif /* WL_MACDBG */

#ifdef PHY_DUMP_BINARY
static uint16
phy_ac_dbg_get_phyreg_address(phy_type_dbg_ctx_t *ctx, uint16 addr)
{
	phy_ac_dbg_info_t *di = (phy_ac_dbg_info_t *)ctx;
	phy_info_t *pi = di->pi;
	if (addr == ACPHY_TableDataWide(pi->pubpi->phy_rev)) {
		return 0;
	} else {
		return phy_utils_read_phyreg(pi, addr);
	}
}
#endif
