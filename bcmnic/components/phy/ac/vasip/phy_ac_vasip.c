/*
 * ACPHY VASIP modules implementation
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
 * $Id: phy_ac_vasip.c 659421 2016-09-14 06:45:22Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_mem.h>
#include <phy_vasip.h>
#include <phy_vasip_api.h>
#include "phy_type_vasip.h"
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <phy_ac_vasip.h>
#include <wlc_phy_int.h>
/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_phyreg_ac.h>

#include <phy_utils_reg.h>
#include <phy_utils_var.h>

/* module private states */
struct phy_ac_vasip_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_vasip_info_t *cmn_info;
	uint8	vasipver;
};

/*
 * Return vasip version, -1 if not present.
 */
static uint8 phy_ac_vasip_get_ver(phy_type_vasip_ctx_t *ctx);
/*
 * reset/activate vasip.
 */
static void phy_ac_vasip_reset_proc(phy_type_vasip_ctx_t *ctx, int reset);
static void phy_ac_vasip_set_clk(phy_type_vasip_ctx_t *ctx, bool val);
static void phy_ac_vasip_write_bin(phy_type_vasip_ctx_t *ctx, const uint32 vasip_code[],
	const uint nbytes);
#ifdef VASIP_SPECTRUM_ANALYSIS
static void phy_ac_vasip_write_spectrum_tbl(phy_type_vasip_ctx_t *ctx,
	const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl);
#endif
static void phy_ac_vasip_write_svmp(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 val);
static void phy_ac_vasip_read_svmp(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 *val);
static void phy_ac_vasip_write_svmp_blk(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 len,
	uint16 *val);
static void phy_ac_vasip_read_svmp_blk(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 len,
	uint16 *val);

/* register phy type specific implementation */
phy_ac_vasip_info_t *
BCMATTACHFN(phy_ac_vasip_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_vasip_info_t *cmn_info)
{
	phy_ac_vasip_info_t *vasip_info;
	phy_type_vasip_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((vasip_info = phy_malloc(pi, sizeof(phy_ac_vasip_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	vasip_info->pi = pi;
	vasip_info->aci = aci;
	vasip_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = vasip_info;
	fns.get_ver = phy_ac_vasip_get_ver;
	fns.reset_proc = phy_ac_vasip_reset_proc;
	fns.set_clk = phy_ac_vasip_set_clk;
	fns.write_bin = phy_ac_vasip_write_bin;
#ifdef VASIP_SPECTRUM_ANALYSIS
	fns.write_spectrum_tbl = phy_ac_vasip_write_spectrum_tbl;
#endif
	fns.write_svmp = phy_ac_vasip_write_svmp;
	fns.read_svmp = phy_ac_vasip_read_svmp;
	fns.write_svmp_blk = phy_ac_vasip_write_svmp_blk;
	fns.read_svmp_blk = phy_ac_vasip_read_svmp_blk;

	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		vasip_info->vasipver =
			READ_PHYREGFLD(pi, MinorVersion, vasipversion);
	} else if (ACMAJORREV_GE32(pi->pubpi->phy_rev)) {
		vasip_info->vasipver = READ_PHYREGFLD(pi, PhyCapability2, vasipPresent) ?
			READ_PHYREGFLD(pi, MinorVersion, vasipversion) : VASIP_NOVERSION;
	} else {
		vasip_info->vasipver = VASIP_NOVERSION;
	}

	phy_vasip_register_impl(cmn_info, &fns);

	return vasip_info;

	/* error handling */
fail:
	if (vasip_info != NULL)
		phy_mfree(pi, vasip_info, sizeof(phy_ac_vasip_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_vasip_unregister_impl)(phy_ac_vasip_info_t *vasip_info)
{
	phy_info_t *pi;
	phy_vasip_info_t *cmn_info;

	ASSERT(vasip_info);
	pi = vasip_info->pi;
	cmn_info = vasip_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_vasip_unregister_impl(cmn_info);

	phy_mfree(pi, vasip_info, sizeof(phy_ac_vasip_info_t));
}

/*
 * Return vasip version, -1 if not present.
 */
static uint8
phy_ac_vasip_get_ver(phy_type_vasip_ctx_t *ctx)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;

	return info->vasipver;
}

/*
 * reset/activate vasip.
 */
static void
phy_ac_vasip_reset_proc(phy_type_vasip_ctx_t *ctx, int reset)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 reset_val = 1;

	if (reset) {
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_VASIPREGISTERS,
			1, VASIPREGISTERS_RESET, 32, &reset_val);
	} else {
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_VASIPREGISTERS,
			1, VASIPREGISTERS_SET, 32, &reset_val);
	}
}

void
phy_ac_vasip_clk_enable(si_t *sih, bool val)
{
	uint32 acphyidx = si_findcoreidx(sih, ACPHY_CORE_ID, 0);

	if (val) {
		/* enter reset */
		si_core_wrapperreg(sih, acphyidx,
			OFFSETOF(aidmp_t, resetctrl), ~0, 0x1);
		/* enable vasip clock */
		si_core_wrapperreg(sih, acphyidx,
			OFFSETOF(aidmp_t, ioctrl), ~0, 0x1);
		/* exit reset */
		si_core_wrapperreg(sih, acphyidx,
			OFFSETOF(aidmp_t, resetctrl), ~0, 0x0);
	} else {
		/* disable vasip clock */
		si_core_wrapperreg(sih, acphyidx,
			OFFSETOF(aidmp_t, ioctrl), ~0, 0x0);
	}
}

static void
phy_ac_vasip_set_clk(phy_type_vasip_ctx_t *ctx, bool val)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;

	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		phy_ac_vasip_clk_enable(pi->sh->sih, val);
	} else {
		MOD_PHYREG(pi, dacClkCtrl, vasipClkEn, val);
	}
}

static void
phy_ac_vasip_write_bin(phy_type_vasip_ctx_t *ctx, const uint32 vasip_code[], const uint nbytes)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8	stall_val, mem_id;
	uint32	count;
	uint32 svmp_addr = 0x0;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	mem_id = 0;
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, 0x8000, 32, &mem_id);

	count = (nbytes/sizeof(uint32));
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, count, svmp_addr, 32, &vasip_code[0]);

	/* restore stall value */
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

#ifdef VASIP_SPECTRUM_ANALYSIS
static void
phy_ac_vasip_write_spectrum_tbl(phy_type_vasip_ctx_t *ctx,
        const uint32 vasip_tbl_code[], const uint nbytes_tbl)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8  stall_val, mem_id_tbl;
	uint32 count_tbl;
	uint32 svmp_tbl_addr = 0x3400; // (0x26800-0x8000*4)>>1

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	mem_id_tbl = 4;
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, 0x8000, 32, &mem_id_tbl);

	count_tbl = (nbytes_tbl/sizeof(uint32));
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, count_tbl, svmp_tbl_addr, 32,
		&vasip_tbl_code[0]);

	/* restore stall value */
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}
#endif /* VASIP_SPECTRUM_ANALYSIS */

static void
phy_ac_vasip_write_svmp(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 val)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 tbl_val;
	uint8  stall_val, mem_id, odd_even;

	mem_id = offset/0x8000;
	offset = offset%0x8000;

	odd_even = offset%2;
	offset = offset >> 1;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, 0x8000, 32, &mem_id);
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
	if (odd_even) {
		tbl_val = tbl_val & 0xffff;
		tbl_val = tbl_val | (uint32) (val << NBITS(uint16));
	} else {
		tbl_val = tbl_val & (0xffff << NBITS(uint16));
		tbl_val = tbl_val | (uint32) (val);
	}
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);

	/* restore stall value */
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

static void
phy_ac_vasip_read_svmp(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 *val)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 tbl_val;
	uint8 stall_val, mem_id, odd_even;

	mem_id = offset/0x8000;
	offset = offset%0x8000;

	odd_even = offset%2;
	offset = offset >> 1;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, 0x8000, 32, &mem_id);
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);

	/* restore stall value */
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	*val = odd_even ? ((tbl_val>> NBITS(uint16)) & 0xffff): (tbl_val & 0xffff);
}

static void
phy_ac_vasip_write_svmp_blk(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 len, uint16 *val)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 tbl_val;
	uint8  stall_val, mem_id;
	uint16 n, odd_start, odd_end;

	mem_id = offset / 0x8000;
	offset = offset % 0x8000;

	odd_start = offset % 2;
	odd_end = (offset + len) % 2;

	offset = offset >> 1;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, 0x8000, 32, &mem_id);

	if (odd_start == 1) {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
		tbl_val &= 0xffff;
		tbl_val |= ((uint32)val[0] << NBITS(uint16));
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
		offset += 1;
	}
	for (n = odd_start; n < (len-odd_start-odd_end); n += 2) {
		tbl_val  = ((uint32)val[n+1] << NBITS(uint16));
		tbl_val |= ((uint32)val[n]);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
		offset += 1;
	}
	if (odd_end == 1) {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
		tbl_val &= (0xffff << NBITS(uint16));
		tbl_val |= ((uint32)val[len-1]);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
	}

	/* restore stall value */
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}

static void
phy_ac_vasip_read_svmp_blk(phy_type_vasip_ctx_t *ctx, uint32 offset, uint16 len, uint16 *val)
{
	phy_ac_vasip_info_t *info = (phy_ac_vasip_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 tbl_val;
	uint8 stall_val, mem_id;
	uint16 n, odd_start, odd_end;

	mem_id = offset / 0x8000;
	offset = offset % 0x8000;

	odd_start = offset % 2;
	odd_end = (offset + len) % 2;

	offset = offset >> 1;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, 0x8000, 32, &mem_id);

	if (odd_start == 1) {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
		val[0] = ((tbl_val >> NBITS(uint16)) & 0xffff);
		offset += 1;
	}
	for (n = odd_start; n < (len-odd_start-odd_end); n += 2) {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
		val[n]   = (tbl_val & 0xffff);
		val[n+1] = ((tbl_val >> NBITS(uint16)) & 0xffff);
		offset += 1;
	}
	if (odd_end == 1) {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_SVMPMEMS, 1, offset, 32, &tbl_val);
		val[len-1] = (tbl_val & 0xffff);
	}

	/* restore stall value */
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}
