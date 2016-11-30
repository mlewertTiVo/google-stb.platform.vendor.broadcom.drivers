/*
 * Common (OS-independent) portion of
 * Broadcom 802.11bang Networking Device Driver
 *
 * ucode download and initialization routines. [including patch for ucode ROM]
 * used by bmac and wowl.
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
 * $Id: wlc_ucinit.c 644052 2016-06-17 03:04:30Z $
 */

/**
 * @file
 * @brief
 * ucode download and initialization routines. used by bmac and wowl.
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmwifi_channels.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include "d11ucode.h"

#ifdef UCODE_IN_ROM_SUPPORT
#include <d11ucode_upatch.h>
#endif /* UCODE_IN_ROM_SUPPORT */

#include <wlc_ucinit.h>
#include <d11_addr_space.h>
#include <hndd11.h>

#include <wlc_macdbg.h>
#ifdef BCMULP
#include <ulp.h>
#endif
/* macro assumes that sz is 2 or 4 ONLY */
#define WLC_WRITE_UC_VAL(osh, base, addr, addr_mask, val, val_offset, sz)		\
	if ((sz) == 2) {								\
		W_REG(osh, ((uint16*)(((uint8 *)(base)) + ((addr) & (addr_mask)))),	\
			((val) + (val_offset)));	\
	} else {					\
		W_REG(osh, ((uint32*)(((uint8 *)(base)) + ((addr) & (addr_mask)))),	\
			((val) + (val_offset)));	\
	}

typedef struct {
	const uint32	*pa;
	uint32		pa_sz;
	const uint32	*pia;
	uint32		pia_sz;

	const d11axiinit_t *ivl_unord_rom;
	const d11axiinit_t *ivl_unord_new;
	const d11axiinit_t *ivl_unord_patch;

	const d11axiinit_t *ivl_ord_rom;
	const d11axiinit_t *ivl_ord_new;
	const d11axiinit_t *ivl_ord_patch;

	const d11axiinit_t *bsivl_unord_rom;
	const d11axiinit_t *bsivl_unord_new;
	const d11axiinit_t *bsivl_unord_patch;

	const d11axiinit_t *bsivl_ord_rom;
	const d11axiinit_t *bsivl_ord_new;
	const d11axiinit_t *bsivl_ord_patch;
} uc_ulp_rom_patch_info;

/* ulp dbg macro */
#define WLC_UCI_DBG(x)
#define WLC_UCI_ERR(x) WL_ERROR(x)

#ifdef UCODE_IN_ROM_SUPPORT
static int wlc_uci_write_pa_and_pia(wlc_hw_info_t *wlc_hw, uint32 uc_imgsel,
	const uint32 *pia, uint pia_sz,
	const uint32 *pa, uint pa_sz);

static int wlc_uci_write_inits_patches(wlc_hw_info_t *wlc_hw, CONST d11axiinit_t *rom_arr,
	CONST d11axiinit_t *new_arr,
	CONST d11axiinit_t *patch_arr);

static int wlc_uci_write_axi_inits(wlc_hw_info_t *wlc_hw, CONST d11axiinit_t *inits);
static bool wlc_uci_patch_valid(CONST d11axiinit_t *d11vals);
static bool wlc_uci_is_addr_valid(wlc_hw_info_t *wlc_hw, uint32 addr);

#ifdef WOWL
static int wlc_uci_wowl_dnld_ucode_inits_patches(wlc_hw_info_t *wlc_hw,
	uc_ulp_rom_patch_info *rpi);
#endif /* WOWL */

#endif /* UCODE_IN_ROM_SUPPORT */

#ifdef UCODE_IN_ROM_SUPPORT
#if D11CONF_LT(60)
#error "UCODE_IN_ROM_SUPPORT needs D11CONF_GE(60)"
#endif /* D11CONF_LT(60) */

/* this function expects pre condition as MAC suspend, but it doesn't
 * un-suspends/enables mac. It enables patch
 */
static int
wlc_uci_enable_ucode_patches(wlc_hw_info_t *wlc_hw, uint32 mask, uint32 flags)
{
	d11regs_t *regs = wlc_hw->regs;
	uint32 mc;
	osl_t *osh = wlc_hw->osh;
	int err = BCME_OK;

	WLC_UCI_DBG(("%s enter\n", __FUNCTION__));
	/* pre-condition to this function: MAC HAS TO BE suspended. if not, bail out */
	mc = R_REG(osh, &regs->maccontrol);

	/* if either enabled/runnning. print err and exit */
	if ((mc & MCTL_PSM_RUN) || (mc & MCTL_EN_MAC)) {
		WLC_UCI_ERR(("%s: ucode running/eanbled, not good\n", __FUNCTION__));
		err = BCME_USAGE_ERROR;
		goto done;
	}

	/* retain the mask and flags in wlc_hw except the PSM_PATCHCC_PENG_TRIGGER.
	 * These has to be applied after all d11 resets. But clear
	 * PSM_PATCHCC_PENG_TRIGGER so that, patch engine is not triggered again.
	 * -- Also note that, if the ucode download is coming from wlc_bmac_process_ucode_sr
	 *	wlc_hw will be freed. So, before a d11 reset we need to save these to wlc_hw
	 *	[if not populated already] and restore once reset is done.
	 */
	wlc_hw->pc_flags = flags & ~PSM_PATCHCC_PENG_TRIGGER;
	wlc_hw->pc_mask = mask & ~PSM_PATCHCC_PENG_TRIGGER;

	/* also reset patch_ctrl_rst_mode */
	hndd11_write_reg32(wlc_hw->osh, &regs->psm_patchcopy_ctrl,
		wlc_hw->pc_mask,
		wlc_hw->pc_flags);

	if (flags & PSM_PATCHCC_PENG_TRIGGER) {
		/* also reset patch_ctrl_rst_mode */
		hndd11_write_reg32(wlc_hw->osh, &regs->psm_patchcopy_ctrl,
			wlc_hw->pc_mask | PSM_PATCHCC_PCTRL_RST_MASK,
			wlc_hw->pc_flags | PSM_PATCHCC_PCTRL_RST_HW);
		/* trigger patch engine */
		hndd11_write_reg32(wlc_hw->osh, &regs->psm_patchcopy_ctrl,
			PSM_PATCHCC_PENG_TRIGGER_MASK | PSM_PATCHCC_COPYEN_MASK,
			PSM_PATCHCC_PENG_TRIGGER | PSM_PATCHCC_COPYEN);

		WLC_UCI_DBG(("%s: triggered patch engine! waiting... \n", __FUNCTION__));
		/* wait until patch applied */
		SPINWAIT(((R_REG(osh, &regs->psm_patchcopy_ctrl) &
			PSM_PATCHCC_PENG_TRIGGER) ==
			PSM_PATCHCC_PENG_TRIGGER), PSM_PATCHCOPY_DELAY);
		/* check for completion */
		if ((R_REG(osh, &regs->psm_patchcopy_ctrl) &
			PSM_PATCHCC_PENG_TRIGGER) ==
			PSM_PATCHCC_PENG_TRIGGER) {
			/* means patching did not finish in time */
			err = BCME_BUSY;
			goto done;
		}
	} else {
		WLC_UCI_DBG(("NOT triggring patch engine!"));
	}
done:

	if (err) {
		/* clear the pc mask and flags */
		wlc_hw->pc_flags = 0;
		wlc_hw->pc_mask = 0;
	}
	WLC_UCI_DBG(("%s: exit: status:%d\n", __FUNCTION__, err));
	return err;
}

/* This function expects pa_sz and pia_sz exactly as given by ucode and will make
 * additional checks inside
 */
static int
wlc_uci_write_pa_and_pia(wlc_hw_info_t *wlc_hw, uint32 uc_imgsel,
	const uint32 *pia, uint pia_sz,
	const uint32 *pa, uint pa_sz)
{
	int err = BCME_OK;
	uint32 ucm_pa_start = 0;
	uint32 ucm_pia_start = 0;
	uint32 ucm_patch_flags = uc_imgsel;
	uint32 ucm_patch_mask = PSM_PATCHCC_UCIMGSEL_MASK;
	uint32 axi_base = si_get_d11_slaveport_addr(wlc_hw->sih,
		D11_AXI_SP_IDX, CORE_BASE_ADDR_0, wlc_hw->unit);
	d11regs_t *regs = wlc_hw->regs;

	WLC_UCI_DBG(("%s: enter\n", __FUNCTION__));
	W_REG(wlc_hw->osh, &regs->psm_patchcopy_ctrl, 0);
	/* modify pa_sz and pia_sz basd on following logic:
	 * "NULL-PATCH" will have contents 4 byte zero's while
	 * "NOT NULL-PATCH" will have > 4 bytes and terminated by 8 byte zero's.
	 * if not null-patch, we should exclude 8 zero bytes from writing
	 */
	pia_sz = (pia_sz <= UCM_PATCH_MIN_BYTES) ? 0 :
		(pia_sz - UCM_PATCH_TERM_SZ);
	pa_sz = (pa_sz <= UCM_PATCH_MIN_BYTES) ? 0 :
		(pa_sz - UCM_PATCH_TERM_SZ);

	/* first 4 bytes in pia will contain where should pa be loaded */
	if (pia_sz > 0) {
		/* program pia:  pia programming is using APB writes only */
		ucm_pia_start = APB_UCM_BASE + PIA_START_OFFSET;

		/* pia programming is using APB writes only */
		wlc_bmac_copyto_objmem(wlc_hw, ucm_pia_start, pia,
			pia_sz, OBJADDR_UCM_SEL);
		/* only instruction offset here... */
		ucm_pa_start = (pia[PA_START_OFFSET] & UCM_PA_ADDR_MASK);
		/* byte offset here.. */
		ucm_pa_start *= UCM_INSTR_WIDTH_BYTES;
		/* pa is valid only when pa_sz is non zero & start-offset-of-pa in
		 * pia is non zero
		 */
		if ((pa_sz > 0) && (ucm_pa_start != 0)) {
			uint32 i = 0;
			/* ...now adding base here */
			ucm_pa_start += axi_base + AXISL_UCM_BASE;

			/* pa pgmming is using axi writes in 4 byte chunks */
			for (i = 0; i < pa_sz; i += UCM_PATCH_WRITE_SZ)
				WLC_WRITE_UC_VAL(wlc_hw->osh, ucm_pa_start, i,
					AXI_ADDR_MASK, pa[i], 0, UCM_PATCH_WRITE_SZ);
		} else {
			/* it is possible that, pia is non empty but pa is empty. nothing here */
		}

		ucm_patch_flags |= PSM_PATCHCC_PMODE_ROM_PATCH | PSM_PATCHCC_PENG_TRIGGER;
		ucm_patch_mask |= PSM_PATCHCC_PMODE_MASK | PSM_PATCHCC_PENG_TRIGGER;
	} else {
		WLC_UCI_DBG(("%s: no pa/pia! ONLY ROM\n", __FUNCTION__));
		/* means no patches and all executed from ROM */
		ucm_patch_flags |= PSM_PATCHCC_PMODE_ROM_RO;
		ucm_patch_mask |= PSM_PATCHCC_PMODE_MASK;
		goto done;
	}
done:
	if ((err = wlc_uci_enable_ucode_patches(wlc_hw, ucm_patch_mask,
		ucm_patch_flags)) != BCME_OK) {
		WLC_UCI_ERR(("%s: error in enabling uc patches!\n", __FUNCTION__));
		ASSERT(0);
		goto finish;
	}

finish:
	WLC_UCI_DBG(("%s: exit: status: %d\n", __FUNCTION__, err));
	return err;
}

int
BCMATTACHFN(wlc_uci_download_rom_patches)(wlc_hw_info_t *wlc_hw)
{
	const uint32 *pia = NULL;
	const uint32 *pa = NULL;
	uint pia_sz = 0;
	uint pa_sz = 0;
	int err = BCME_OK;

	// note: ONLY for p2p ucode.
	if (D11REV_IS(wlc_hw->corerev, 60)) {
		pia = d11ucode_p2p60_pia;
		pa = d11ucode_p2p60_pa;
		pia_sz = d11ucode_p2p60_piasz;
		pa_sz = d11ucode_p2p60_pasz;
	} else if (D11REV_IS(wlc_hw->corerev, 62)) {
		pia = d11ucode_p2p62_pia;
		pa = d11ucode_p2p62_pa;
		pia_sz = d11ucode_p2p62_piasz;
		pa_sz = d11ucode_p2p62_pasz;
	} else {
		/* not supported yet */
		WLC_UCI_ERR(("no rom/p2p ucode for rev %d\n", wlc_hw->corerev));
		ASSERT(0);
		err = BCME_UNSUPPORTED;
		goto done;
	}

	if ((err = wlc_uci_write_pa_and_pia(wlc_hw, PSM_PATCHCC_UCIMGSEL_DS0,
		pia, pia_sz, pa, pa_sz)) != BCME_OK) {
		WLC_UCI_ERR(("%s: uc enable error\n", __FUNCTION__));
		goto done;
	}

done:
	return err;
}

/* checks ucode rom and axi capability */
bool
wlc_uci_check_cap_ucode_rom_axislave(wlc_hw_info_t *wlc_hw)
{
	bool ret = FALSE;
	/* check corerev */
	if (D11REV_LT(wlc_hw->corerev, 60)) {
		goto done;
	}

	/* check d11 slave ports */
	if (si_num_slaveports(wlc_hw->sih, D11_CORE_ID) < D11_AXI_SP_ID) {
		goto done;
	}
	/* TBD: add logic here to see how to access MACMemInfo0 (IHR Address 0x368) :
	 * http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/Dot11macRev60
	 * and also add a read of first ucode ROM location to see if it is non zero.
	 */

	ret = TRUE;
done:
	/* TBD put logic here to see the axi slave cap */
	return ret;
}

static bool
wlc_uci_is_addr_valid(wlc_hw_info_t *wlc_hw, uint32 addr)
{
	bool ret = TRUE;
	uint32 axi_base = si_get_d11_slaveport_addr(wlc_hw->sih,
		D11_AXI_SP_IDX, CORE_BASE_ADDR_0, wlc_hw->unit);

	/* currently check only axi space */
	if ((axi_base & addr) != axi_base)
		goto done;

	if (!IS_ADDR_AXISL(axi_base, addr))
		ret = FALSE;
done:
	return ret;
}

static int
wlc_uci_write_axi_inits(wlc_hw_info_t *wlc_hw, CONST d11axiinit_t *inits)
{
	int err = BCME_OK;
	int i;
	osl_t *osh = wlc_hw->osh;

	WLC_UCI_DBG(("wl%d: wlc_write_inits\n", wlc_hw->unit));

	for (i = 0; inits[i].addr != 0xffffffff; i++) {
		uint offset_val = 0;
		ASSERT((inits[i].size == 2) || (inits[i].size == 4));

		if (inits[i].addr == D11CORE_TEMPLATE_REG_OFFSET) {
			/* wlc_hw->templatebase is the template base address for core 1/0
			 * For core-0 it is zero and for core 1 it contains the core-1
			 * template offset.
			 */
			offset_val = wlc_hw->templatebase;
		}
		/* Note that, inits in ucode is in mixed APB/AXI format with absolute address.
		 * But use specific check for AXI space.
		 */
		if (wlc_uci_is_addr_valid(wlc_hw, inits[i].addr)) {
			if ((inits[i].size) == 2) {
				W_REG(osh, ((uint16*)inits[i].addr), inits[i].value + offset_val);
			} else {
				W_REG(osh, ((uint32*)inits[i].addr), inits[i].value + offset_val);
			}
		} else {
			WLC_UCI_ERR(("%s: invalid address!: ix: %d, addr: %x\n", __FUNCTION__, i,
				inits[i].addr));
			err = BCME_BADADDR;
			goto done;
		}
	}
done:
	return err;
}

static bool
wlc_uci_patch_valid(CONST d11axiinit_t *d11vals)
{
	if (d11vals[0].size == 0) {
		/* means invalid and not patched */
		return FALSE;
	}
	return TRUE;
}

/* override and patch */
static int
wlc_uci_write_inits_patches(wlc_hw_info_t *wlc_hw, CONST d11axiinit_t *rom_arr,
	CONST d11axiinit_t *new_arr,
	CONST d11axiinit_t *patch_arr)
{
	int err = BCME_OK;

	if (new_arr && wlc_uci_patch_valid(new_arr)) {
		/* use only new array which is complete override of rom_arr */
		WLC_UCI_DBG(("%s: 1. writing new array\n", __FUNCTION__));
		err = wlc_uci_write_axi_inits(wlc_hw, new_arr);
	} else if (patch_arr && wlc_uci_patch_valid(patch_arr)) {
		/* patch is valid. first write rom array, then patch */
		if (rom_arr) {
			WLC_UCI_DBG(("%s: 2. writing rom array\n", __FUNCTION__));
			err = wlc_uci_write_axi_inits(wlc_hw, rom_arr);
		}
		if (err)
			goto done;
		WLC_UCI_DBG(("%s: 2. writing patch array\n", __FUNCTION__));
		err = wlc_uci_write_axi_inits(wlc_hw, patch_arr);
		if (err)
			goto done;
	} else {
		/* if rom_arr is valid, use it. rom_arr null means it could have programmed by
		 * the help of fcbs.
		 */
		if (rom_arr) {
			WLC_UCI_DBG(("%s: 3. writing rom array\n", __FUNCTION__));
			err = wlc_uci_write_axi_inits(wlc_hw, rom_arr);
			if (err)
				goto done;
		} else {
			WLC_UCI_DBG(("%s: 4. NOT writing anything!\n", __FUNCTION__));
		}
	}
done:
	return err;
}

int
wlc_uci_write_inits_with_rom_support(wlc_hw_info_t *wlc_hw, uint32 flags)
{
	int err = BCME_OK;

	if (flags == UCODE_INITVALS) {
		WLC_UCI_DBG(("%s: UCODE_INITVALS\n", __FUNCTION__));
		if (D11REV_IS(wlc_hw->corerev, 60)) {
			/* override and patch for non ordered/shm arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36initvals60_axislave,
					d11ac36initvals60_axislave_new,
					d11ac36initvals60_axislave_patch);

			/* override and patch for ordered/ihr arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36initvals60_axislave_order,
					d11ac36initvals60_axislave_order_new,
					d11ac36initvals60_axislave_order_patch);
		}
		else if (D11REV_IS(wlc_hw->corerev, 62)) {
			/* override and patch for non ordered/shm arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36initvals62_axislave,
					d11ac36initvals62_axislave_new,
					d11ac36initvals62_axislave_patch);

			/* override and patch for ordered/ihr arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36initvals62_axislave_order,
					d11ac36initvals62_axislave_order_new,
					d11ac36initvals62_axislave_order_patch);
		}
		else {
			WLC_UCI_ERR(("wl%d: %s: corerev %d is invalid", wlc_hw->unit,
				__FUNCTION__, wlc_hw->corerev));
			err = BCME_BADOPTION;
			goto done;
		}
	} else if (flags == UCODE_BSINITVALS) {
		WLC_UCI_DBG(("%s: UCODE_BSINITVALS\n", __FUNCTION__));
		if (D11REV_IS(wlc_hw->corerev, 60)) {
			/* override and patch for non ordered/shm arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36bsinitvals60_axislave,
					d11ac36bsinitvals60_axislave_new,
					d11ac36bsinitvals60_axislave_patch);

			/* override and patch for ordered/ihr arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36bsinitvals60_axislave_order,
					d11ac36bsinitvals60_axislave_order_new,
					d11ac36bsinitvals60_axislave_order_patch);
		} else if (D11REV_IS(wlc_hw->corerev, 62)) {
			/* override and patch for non ordered/shm arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36bsinitvals62_axislave,
					d11ac36bsinitvals62_axislave_new,
					d11ac36bsinitvals62_axislave_patch);

			/* override and patch for ordered/ihr arrays */
			wlc_uci_write_inits_patches(wlc_hw,
					d11ac36bsinitvals62_axislave_order,
					d11ac36bsinitvals62_axislave_order_new,
					d11ac36bsinitvals62_axislave_order_patch);
		} else {
			WLC_UCI_ERR(("wl%d: %s: corerev %d is invalid", wlc_hw->unit,
				__FUNCTION__, wlc_hw->corerev));
			err = BCME_BADOPTION;
			goto done;
		}
	} else {
		WLC_UCI_ERR(("%s: wl%d: unsupported flags for inits %d\n",
			__FUNCTION__, wlc_hw->unit, wlc_hw->corerev));
		ASSERT(0);
		err = BCME_UNSUPPORTED;
		goto done;
	}
done:
	return err;
}

#ifdef WOWL

static int
wlc_uci_wowl_dnld_ucode_inits_patches(wlc_hw_info_t *wlc_hw,
	uc_ulp_rom_patch_info *rpi)
{
	int ret = BCME_OK;

	WLC_UCI_DBG(("%s: suspend the present ucode\n", __FUNCTION__));
	/* suspend the present ucode */
	if ((ret = wlc_bmac_wowlucode_init(wlc_hw)) != BCME_OK) {
		WLC_UCI_ERR(("wl%d: wlc_wowl_dnld_ucode_inits:  ucode suspend failed\n",
			wlc_hw->wlc->pub->unit));
		goto done;
	}

	wlc_bmac_mctrl(wlc_hw, ~0, ~(MCTL_EN_MAC | MCTL_PSM_RUN));

	/* Set the SHM to indicate to ucode that we are going to wowl state */
	if (D11REV_IS(wlc_hw->corerev, 60) || D11REV_IS(wlc_hw->corerev, 62)) {
		wlc_bmac_write_shm(wlc_hw, M_ULP_STATUS(wlc_hw), 0x0);
	}

	WLC_UCI_DBG(("%s: 2. download the patches\n", __FUNCTION__));
	/* download the patches */
	if ((ret = wlc_uci_write_pa_and_pia(wlc_hw, PSM_PATCHCC_UCIMGSEL_DS1, rpi->pia, rpi->pia_sz,
		rpi->pa, rpi->pa_sz)) != BCME_OK) {
		WLC_UCI_ERR(("wl%d: write patch failed\n",
			wlc_hw->wlc->pub->unit));
		goto done;
	}

	WLC_UCI_DBG(("%s: 3. start/switch the ucode\n", __FUNCTION__));
	/* tbd start/switch the ucode */
	if ((ret = wlc_bmac_wowlucode_start(wlc_hw)) != BCME_OK) {
		WLC_UCI_ERR(("wl%d: wlc_wowl_dnld_ucode_inits:  ucode start failed\n",
			wlc_hw->wlc->pub->unit));
		goto done;
	}
#ifdef DBG_UC
	if (UCDBG_PATCHDUMP_ON()) {
		UCDBG(UCD_INITPATCH_DUMP, ("%s:BEFORE initvals\n", __FUNCTION__));
		wlc_macdbg_ulp_dump(wlc_hw->wlc);
	}
#endif

	WLC_UCI_DBG(("%s: 4. ULP_DS1, inits\n", __FUNCTION__));
	/* download the inits patches */
	wlc_uci_write_inits_patches(wlc_hw, rpi->ivl_unord_rom, rpi->ivl_unord_new,
		rpi->ivl_unord_patch);
	wlc_uci_write_inits_patches(wlc_hw, rpi->ivl_ord_rom, rpi->ivl_ord_new,
		rpi->ivl_ord_patch);

	WLC_UCI_DBG(("%s: 5. ULP_DS1, bsinits\n", __FUNCTION__));
	/* download the binits */
	wlc_uci_write_inits_patches(wlc_hw, rpi->bsivl_unord_rom, rpi->bsivl_unord_new,
		rpi->bsivl_unord_patch);
	wlc_uci_write_inits_patches(wlc_hw, rpi->bsivl_ord_rom, rpi->bsivl_ord_new,
		rpi->bsivl_ord_patch);

#ifdef DBG_UC
	if (UCDBG_PATCHDUMP_ON()) {
		UCDBG(UCD_INITPATCH_DUMP, ("%s:AFTER initvals\n", __FUNCTION__));
		wlc_macdbg_ulp_dump(wlc_hw->wlc);
	}
#endif

	/* finish the post ucode download initialization */
	WLC_UCI_DBG(("%s: 8. post ucode download initialization\n", __FUNCTION__));
	/* tbd: anymore to do in ulp? */
	if (BCME_OK != wlc_bmac_wakeucode_dnlddone(wlc_hw)) {
		WLC_UCI_ERR(("wl%d: wlc_wowl_dnld_ucode_inits:  finish dnld failed\n",
			wlc_hw->wlc->pub->unit));
		ret = BCME_ERROR;
		goto done;
	}

done:
	return ret;
}

int
wlc_uci_wowl_ucode_init_rom_patches(wlc_info_t *wlc)
{
	int ret = BCME_OK;
	uc_ulp_rom_patch_info *rpi = NULL;

	WLC_UCI_DBG(("%s: enter\n", __FUNCTION__));

	rpi = MALLOCZ(wlc->pub->osh, sizeof(*rpi));
	if (rpi == NULL) {
		ret = BCME_NOMEM;
		goto done;
	}

	if (D11REV_IS(wlc->pub->corerev, 60)) {
		if (BCMULP_ENAB()) {
			rpi->pa = d11ucode_ulp60_pa;
			rpi->pa_sz = d11ucode_ulp60_pasz;
			rpi->pia = d11ucode_ulp60_pia;
			rpi->pia_sz = d11ucode_ulp60_piasz;

			rpi->ivl_unord_rom = d11ulpac36initvals60_axislave;
			rpi->ivl_unord_new = d11ulpac36initvals60_axislave_new;
			rpi->ivl_unord_patch = d11ulpac36initvals60_axislave_patch;

			rpi->ivl_ord_rom = d11ulpac36initvals60_axislave_order;
			rpi->ivl_ord_new = d11ulpac36initvals60_axislave_order_new;
			rpi->ivl_ord_patch = d11ulpac36initvals60_axislave_order_patch;

			rpi->bsivl_unord_rom = d11ulpac36bsinitvals60_axislave;
			rpi->bsivl_unord_new = d11ulpac36bsinitvals60_axislave_new;
			rpi->bsivl_unord_patch = d11ulpac36bsinitvals60_axislave_patch;

			rpi->bsivl_ord_rom = d11ulpac36bsinitvals60_axislave_order;
			rpi->bsivl_ord_new = d11ulpac36bsinitvals60_axislave_order_new;
			rpi->bsivl_ord_patch = d11ulpac36bsinitvals60_axislave_order_patch;
		} else {
			WLC_UCI_ERR(("wl: %s:Invalid rev/phy for which"
				"ulp rom patch unavailable\n",	__FUNCTION__));
			ret = BCME_ERROR;
		}
	} else if (D11REV_IS(wlc->pub->corerev, 62)) {
		if (BCMULP_ENAB()) {
			rpi->pa = d11ucode_ulp62_pa;
			rpi->pa_sz = d11ucode_ulp62_pasz;
			rpi->pia = d11ucode_ulp62_pia;
			rpi->pia_sz = d11ucode_ulp62_piasz;

			rpi->ivl_unord_rom = d11ulpac36initvals62_axislave;
			rpi->ivl_unord_new = d11ulpac36initvals62_axislave_new;
			rpi->ivl_unord_patch = d11ulpac36initvals62_axislave_patch;

			rpi->ivl_ord_rom = d11ulpac36initvals62_axislave_order;
			rpi->ivl_ord_new = d11ulpac36initvals62_axislave_order_new;
			rpi->ivl_ord_patch = d11ulpac36initvals62_axislave_order_patch;

			rpi->bsivl_unord_rom = d11ulpac36bsinitvals62_axislave;
			rpi->bsivl_unord_new = d11ulpac36bsinitvals62_axislave_new;
			rpi->bsivl_unord_patch = d11ulpac36bsinitvals62_axislave_patch;

			rpi->bsivl_ord_rom = d11ulpac36bsinitvals62_axislave_order;
			rpi->bsivl_ord_new = d11ulpac36bsinitvals62_axislave_order_new;
			rpi->bsivl_ord_patch = d11ulpac36bsinitvals62_axislave_order_patch;
		} else {
			WLC_UCI_ERR(("wl: %s:Invalid rev/phy for which"
				"ulp rom patch unavailable\n",	__FUNCTION__));
			ret = BCME_ERROR;
		}
	} else {
		WLC_UCI_ERR(("wl: %s:Invalid rev/phy for which"
			"ulp rom patch unavailable\n",  __FUNCTION__));
		ret = BCME_ERROR;
		goto done;
	}

	if (rpi) {
		ret = wlc_uci_wowl_dnld_ucode_inits_patches(wlc->hw, rpi);
		MFREE(wlc->pub->osh, rpi, sizeof(*rpi));
	}
done:
	return ret;
}
#endif /* WOWL */

#endif /* UCODE_IN_ROM_SUPPORT */

static int
wlc_uci_get_axi_ucm_base(wlc_hw_info_t *wlc_hw, uint32 *ucbase)
{
	int ret = BCME_OK;

#if D11CONF_IS(60) || D11CONF_IS(62)
		*ucbase = (si_get_d11_slaveport_addr(wlc_hw->sih,
				D11_AXI_SP_IDX, CORE_BASE_ADDR_0, wlc_hw->unit) + AXISL_UCM_BASE);
#else
		WL_TRACE(("%s: rev not supported for axi access\n", __FUNCTION__));
		ret = BCME_UNSUPPORTED;
#endif

	return ret;
}

int
wlc_uci_write_axi_slave(wlc_hw_info_t *wlc_hw, const uint32 ucode[], const uint nbytes)
{
	uint32 ucbase = 0;
	int ret = BCME_OK;

	ret = wlc_uci_get_axi_ucm_base(wlc_hw, &ucbase);

	WLC_UCI_DBG(("%s:using axislave (base: 0x%08x)\n", __FUNCTION__, (uint32)ucbase));

	if (ret || !ucbase)
		goto done;

	memcpy((void *)((uintptr)ucbase), ucode, nbytes);

done:
	return ret;
}
