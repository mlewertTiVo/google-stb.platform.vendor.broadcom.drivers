/*
 * wlc_ltr.c
 *
 * Latency Tolerance Reporting
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_ltr.c 475885 2014-05-07 07:35:16Z $
 *
 */

/**
 * @file
 * @brief
 * Latency Tolerance Reporting (LTR) is a mechanism for devices to report their
 * service latency requirements for Memory Reads and Writes to the PCIe Root Complex
 * such that platform resources can be power managed without impacting device
 * functionality and performance. Platforms want to consume the least amount of power.
 * Devices want to provide the best performance and functionality (which needs power).
 * A device driver can mediate between these conflicting goals by intelligently reporting
 * device latency requirements based on the state of the device.
 */


#include <wlc_cfg.h>
#ifdef WL_LTR
#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmdevs.h>
#include <pcicfg.h>
#include <siutils.h>
#include <bcmendian.h>
#include <nicpci.h>
#include <wlioctl.h>
#include <pcie_core.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wl_export.h>
#include <wl_dbg.h>
#ifdef WLOFFLD
#include <wlc_offloads.h>
#endif
#include <wlc_ltr.h>


enum {
	IOV_LTR = 0,			/* Turn LTR reporting on/off in sw */
	IOV_LTR_ACTIVE_LATENCY, /* Get/Set LTR active latency tolerance */
	IOV_LTR_SLEEP_LATENCY,	/* Get/Set LTR sleep latency tolerance */
	IOV_LTR_HI_WM,			/* Get/Set LTR host fifo hi watermark */
	IOV_LTR_LO_WM
};

static const bcm_iovar_t ltr_iovars[] = {
	{ "ltr", IOV_LTR,
	(0), IOVT_UINT32, 0,
	},
	{ "ltr_active_lat", IOV_LTR_ACTIVE_LATENCY,
	(0), IOVT_UINT32, 0,
	},
	{ "ltr_sleep_lat", IOV_LTR_SLEEP_LATENCY,
	(0), IOVT_UINT32, 0,
	},
	{ "ltr_hi_wm", IOV_LTR_HI_WM,
	(0), IOVT_UINT32, 0,
	},
	{ "ltr_lo_wm", IOV_LTR_LO_WM,
	(0), IOVT_UINT32, 0,
	},
	{ NULL, 0, 0, 0, 0 }
};

static int
wlc_ltr_doiovar(void *context, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int alen, int vsize, struct wlc_if *wlcif)
{
	int err = BCME_UNSUPPORTED;
	wlc_ltr_info_t *ltr_info = (wlc_ltr_info_t *)context;
	wlc_info_t *wlc = ltr_info->wlc;
	int32 int_val = 0;
	int32 *ret_int_ptr;
	bool bool_val;
	err = BCME_OK;

	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	bool_val = (int_val != 0) ? TRUE : FALSE;

	switch (actionid) {
		case IOV_GVAL(IOV_LTR):
			*ret_int_ptr = LTR_ENAB(wlc->pub);
			break;
		case IOV_SVAL(IOV_LTR):
			if (bool_val == LTR_ENAB(wlc->pub))
				break;
			if (bool_val && !LTR_SUPPORT(wlc->pub))
				break;
			wlc->pub->_ltr = bool_val;
#ifdef WLOFFLD
			if (WLOFFLD_CAP(wlc)) {
				/* New value in ltr_info; send it over to ARM */
				wlc_ol_ltr(wlc->ol, ltr_info);
			}
#endif /* WLOFFLD */
			break;

		case IOV_GVAL(IOV_LTR_ACTIVE_LATENCY):
			*ret_int_ptr = ltr_info->active_lat;
			break;
		case IOV_SVAL(IOV_LTR_ACTIVE_LATENCY):
			if ((int_val <= 0) ||
				(int_val > LTR_MAX_ACTIVE_LATENCY)) {
				err = BCME_RANGE;
				break;
			}
			if (!LTR_ENAB(wlc->pub)) {
				err = BCME_UNSUPPORTED;
				break;
			}
			ltr_info->active_lat = int_val;
			if (!wlc->clk)
				break;
			wlc_ltr_init_reg(wlc->hw->sih, PCIE_LTR0_REG_OFFSET,
				ltr_info->active_lat, LTR_SCALE_US);
#ifdef WLOFFLD
			if (WLOFFLD_CAP(wlc)) {
				/* New value in ltr_info; send it over to ARM */
				wlc_ol_ltr(wlc->ol, ltr_info);
			}
#endif /* WLOFFLD */
			break;

		case IOV_GVAL(IOV_LTR_SLEEP_LATENCY):
			*ret_int_ptr = ltr_info->sleep_lat;
			break;
		case IOV_SVAL(IOV_LTR_SLEEP_LATENCY):
			if ((int_val <= 0) ||
				(int_val > LTR_MAX_SLEEP_LATENCY)) {
				err = BCME_RANGE;
				break;
			}
			if (!LTR_ENAB(wlc->pub)) {
				err = BCME_UNSUPPORTED;
				break;
			}
			ltr_info->sleep_lat = int_val;
			if (!wlc->clk)
				break;
			wlc_ltr_init_reg(wlc->hw->sih, PCIE_LTR2_REG_OFFSET,
				ltr_info->sleep_lat, LTR_SCALE_MS);
#ifdef WLOFFLD
			if (WLOFFLD_CAP(wlc)) {
				/* New value in ltr_info; send it over to ARM */
				wlc_ol_ltr(wlc->ol, ltr_info);
			}
#endif /* WLOFFLD */
			break;

		case IOV_GVAL(IOV_LTR_HI_WM):
			*ret_int_ptr = ltr_info->hi_wm;
			break;
		case IOV_SVAL(IOV_LTR_HI_WM):
			if (!LTR_ENAB(wlc->pub)) {
				err = BCME_UNSUPPORTED;
				break;
			}
			if ((int_val > LTR_MAX_HI_WATERMARK) ||
				((uint32)int_val < ltr_info->lo_wm)) {
				err = BCME_RANGE;
				break;
			}
			if (int_val == ltr_info->hi_wm)
				break;
			ltr_info->hi_wm = int_val;
#ifdef WLOFFLD
			if (WLOFFLD_CAP(wlc)) {
				/* New value in ltr_info; send it over to ARM */
				wlc_ol_ltr(wlc->ol, ltr_info);
			}
#endif /* WLOFFLD */
			break;

		case IOV_GVAL(IOV_LTR_LO_WM):
			*ret_int_ptr = ltr_info->lo_wm;
			break;
		case IOV_SVAL(IOV_LTR_LO_WM):
			if (!LTR_ENAB(wlc->pub)) {
				err = BCME_UNSUPPORTED;
				break;
			}
			if ((int_val < LTR_MIN_LO_WATERMARK) ||
				((uint32)int_val > ltr_info->hi_wm)) {
				err = BCME_RANGE;
				break;
			}
			if (int_val == ltr_info->lo_wm)
				break;
			ltr_info->lo_wm = int_val;
#ifdef WLOFFLD
			if (WLOFFLD_CAP(wlc)) {
				/* New value in ltr_info; send it over to ARM */
				wlc_ol_ltr(wlc->ol, ltr_info);
			}
#endif /* WLOFFLD */
			break;
		default:
			err = BCME_UNSUPPORTED;
			break;
	}
	return err;
}

/* module attach */
wlc_ltr_info_t*
BCMATTACHFN(wlc_ltr_attach)(wlc_info_t *wlc)
{
	wlc_ltr_info_t *ltr_info = NULL;

	if (!wlc) {
		WL_ERROR(("%s - null wlc\n", __FUNCTION__));
		goto fail;
	}
	if ((ltr_info = (wlc_ltr_info_t *)MALLOCZ(wlc->osh, sizeof(wlc_ltr_info_t)))
		== NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	/* Check if LTR is supported by hardware */
	wlc->pub->_ltr_support = si_pcieltrenable(wlc->hw->sih, 0, 0);
	/* Enable LTR if supported by hardware */
	wlc->pub->_ltr = LTR_SUPPORT(wlc->pub);

	ltr_info->wlc = wlc;
	ltr_info->active_idle_lat = LTR_LATENCY_100US;
	ltr_info->active_lat = LTR_LATENCY_60US;
	ltr_info->sleep_lat = LTR_LATENCY_3MS;
	ltr_info->hi_wm = LTR_HI_WATERMARK;
	ltr_info->lo_wm = LTR_LO_WATERMARK;

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, ltr_iovars, "ltr", ltr_info, wlc_ltr_doiovar,
	                        NULL, NULL, NULL)) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	return ltr_info;
fail:
	wlc_ltr_detach(ltr_info);
	return NULL;
}

/* module detach */
void
BCMATTACHFN(wlc_ltr_detach)(wlc_ltr_info_t *ltr_info)
{
	wlc_info_t *wlc;

	if (ltr_info == NULL) {
		return;
	}
	wlc = ltr_info->wlc;
	wlc_module_unregister(wlc->pub, "ltr", ltr_info);
	MFREE(wlc->osh, ltr_info, sizeof(wlc_ltr_info_t));
}

void
wlc_ltr_init_reg(si_t *sih, uint32 offset, uint32 latency, uint32 unit)
{
	uint32 val;

	val = latency & 0x3FF;	/* enforce 10 bit latency value */
	val |= (unit << 10);	/* unit=2 sets latency in units of 1us, unit=4 sets unit of 1ms */
	val |= (1 << 15);		/* enable latency requirement */
	val |= val << 16;		/* repeat for no snoop */
	WL_PS(("%s: rd[0x%x]=0x%x wr[0x%x]=0x%x\n", __FUNCTION__,
	       offset, si_pcieltr_reg(sih, offset, 0, 0), offset, val));
	si_pcieltr_reg(sih, offset, ~0, val);
}

int
wlc_ltr_init(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw;
	si_t *sih;
	uint32 val;

	wlc_hw = wlc->hw;
	sih = wlc_hw->sih;

	/* 4360 Hysteresis Values where XTAL frequency is 40Mhz */
	if ((CHIPID(wlc_hw->sih->chip) != BCM4352_CHIP_ID) &&
	    (CHIPID(wlc_hw->sih->chip) != BCM4360_CHIP_ID))
		return BCME_OK;

	/* Short Spacing delay should be 0 to allow
	 * low latency LTR to be send immediately
	 */
	val = 0x1F4;
	si_pcieltrspacing_reg(sih, 1, val);
	WL_PS(("%s: ltrspacing wr=0x%x rd=0x%x\n", __FUNCTION__,
	       val, si_pcieltrspacing_reg(sih, 0, 0)));

	/* Spacing count for exit out of Active state- 12us
	 * Spacing out for exit out of Active.Idle - 1us
	 */
	val = 0xC0001;
	si_pcieltrhysteresiscnt_reg(sih, 1, val);
	WL_PS(("%s: ltrhysteresiscnt wr=0x%x rd=0x%x\n", __FUNCTION__,
	       val, si_pcieltrhysteresiscnt_reg(sih, 0, 0)));

	/*
	 * Set LTR active, active_idle and sleep tolerances
	 */
	wlc_ltr_init_reg(sih, PCIE_LTR0_REG_OFFSET, wlc->ltr_info->active_lat, LTR_SCALE_US);
	wlc_ltr_init_reg(sih, PCIE_LTR1_REG_OFFSET, wlc->ltr_info->active_idle_lat, LTR_SCALE_US);
	wlc_ltr_init_reg(sih, PCIE_LTR2_REG_OFFSET, wlc->ltr_info->sleep_lat, LTR_SCALE_MS);

	if ((CHIPID(wlc_hw->sih->chip) == BCM4352_CHIP_ID) &&
	    (CHIPID(wlc_hw->sih->chip) == BCM4360_CHIP_ID)) {
		W_REG(wlc->osh, (uint32 *)((uint8 *)(wlc->regs) + 0x1160), 0x2838280);
		W_REG(wlc->osh, (uint32 *)((uint8 *)(wlc->regs) + 0x1164), 0x3);
		WL_PS(("%s: D11 reg offset 0x1160 [0x%p]=0x%x\n", __FUNCTION__,
		       ((uint8 *)(wlc->regs) + 0x1160),
		       R_REG(wlc->osh, (uint32 *)((uint8 *)(wlc->regs) + 0x1160))));
		WL_PS(("%s: D11 reg offset 0x1164 [0x%p]=0x%x\n", __FUNCTION__,
		       ((uint8 *)(wlc->regs) + 0x1164),
		       R_REG(wlc->osh, (uint32 *)((uint8 *)(wlc->regs) + 0x1164))));
	}


	return BCME_OK;
}

/**
 * Init LTR registers. That needs pci link up, xtal on and d11 core
 * out of reset. Do this temporarily to set LTR registers and
 * then turn everything off.
 */
int
wlc_ltr_up_prep(wlc_info_t *wlc, uint32 ltr_state)
{
	wlc_hw_info_t *wlc_hw;
	si_t *sih;
	bool clk, xtal;
	uint32 resetbits = 0, flags = 0;

	WL_TRACE(("%s\n", __FUNCTION__));

	if (wlc->hw == NULL) {
		WL_ERROR(("%s: wlc->hw is NULL\n", __FUNCTION__));
		return BCME_ERROR;
	}

	wlc_hw = wlc->hw;
	sih = wlc_hw->sih;

	/*
	 * Bring up pcie, turn on xtal and bring core out of reset
	 */

	si_pci_up(sih);

	xtal = wlc_hw->sbclk;
	if (!xtal)
		wlc_bmac_xtal(wlc_hw, ON);

	/* may need to take core out of reset first */
	clk = wlc_hw->clk;
	if (!clk) {
		/*
		 * corerev >= 18, mac no longer enables phyclk automatically when driver accesses
		 * phyreg throughput mac. This can be skipped since only mac reg is accessed below
		 */
		if (D11REV_GE(wlc_hw->corerev, 18))
			flags |= SICF_PCLKE;

		/* AI chip doesn't restore bar0win2 on hibernation/resume, need sw fixup */
		if ((CHIPID(sih->chip) == BCM43224_CHIP_ID) ||
		    (CHIPID(sih->chip) == BCM43225_CHIP_ID) ||
		    (CHIPID(sih->chip) == BCM43421_CHIP_ID)) {
			wlc_hw->regs = (d11regs_t *)si_setcore(
					wlc_hw->sih, D11_CORE_ID, wlc_hw->macunit);
			ASSERT(wlc_hw->regs != NULL);
		}
		si_core_reset(wlc_hw->sih, flags, resetbits);
		wlc_mctrl_reset(wlc_hw);
	}

	/*
	 * Init LTR and set inital state
	 */
	wlc_ltr_init(wlc);
	wlc_ltr_hwset(wlc->hw, wlc->regs, ltr_state);

	/*
	 * Bring down pcie, turn off xtal and put core back into reset
	 */
	if (!clk)
		si_core_disable(sih, 0);

	if (!xtal)
		wlc_bmac_xtal(wlc_hw, OFF);

	si_pci_down(sih);

	return BCME_OK;
}

int
wlc_ltr_hwset(wlc_hw_info_t *wlc_hw, d11regs_t *regs, uint32 ltr_state)
{
	osl_t *osh = wlc_hw->osh;
	si_t *sih = wlc_hw->sih;
	uint32 val, orig_state;

	if ((CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM4352_CHIP_ID)) {
		val = R_REG(osh, &regs->psm_corectlsts);
		orig_state = (val >> PSM_CORE_CTL_LTR_BIT) & PSM_CORE_CTL_LTR_MASK;
	} else {
		val = si_corereg(sih, sih->buscoreidx,
			OFFSETOF(sbpcieregs_t, u.pcie2.ltr_state), 0, 0);
		orig_state = val & PSM_CORE_CTL_LTR_MASK;
	}
	WL_PS(("wl:%s: orig ltr=%d new ltr=%d [0=>sleep, 2=>active]\n",
		__FUNCTION__, orig_state, ltr_state));

	if (orig_state != ltr_state) {
		if ((CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
		    (CHIPID(sih->chip) == BCM4352_CHIP_ID)) {
			/* Write new LTR state to PSMCoreControlStatus[10:9] */
			val &= ~(PSM_CORE_CTL_LTR_MASK << PSM_CORE_CTL_LTR_BIT);
			val |= ((ltr_state & PSM_CORE_CTL_LTR_MASK) << PSM_CORE_CTL_LTR_BIT);
			W_REG(osh, &regs->psm_corectlsts, val);
		} else {
			si_corereg(sih, sih->buscoreidx,
				OFFSETOF(sbpcieregs_t, u.pcie2.ltr_state),
				PSM_CORE_CTL_LTR_MASK, ltr_state);
		}
		WL_PS(("wl:%s: Sending LTR message ltr=%d [0=>sleep, 2=>active]\n",
		       __FUNCTION__, ltr_state));
	}

	return BCME_OK;
}

#endif /* WL_LTR */
