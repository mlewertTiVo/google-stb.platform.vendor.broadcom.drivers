/**
 * iocv/wlc module wrapper source file
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_iocv.c 661109 2016-09-23 07:54:52Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <wlc_types.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bsscfg.h>
#include <wlioctl.h>
#include <wl_dbg.h>
#include <wl_export.h>
#include <wlc_hw.h>
#include <wlc_seq_cmds.h>
#include <wlc_iocv_high.h>
#include <wlc_bmac_iocv.h>
#include <phy_high_api.h>
#include <wlc_dump.h>
#include <wlc_txc.h>
#ifdef WLRSDB
#include <wlc_rsdb.h>
#endif
#include <wlc_iocv.h>

/* high iovar table & dispatcher registry capacity */
#ifndef WLC_IOVT_HIGH_REG_SZ
#define WLC_IOVT_HIGH_REG_SZ 112
#endif

/* high ioctl table & dispatcher registry capacity */
#ifndef WLC_IOCT_HIGH_REG_SZ
#define WLC_IOCT_HIGH_REG_SZ 28
#endif

/* local function declarations */
static int wlc_iocv_doioctl(void *ctx, uint32 cmd, void *arg, uint len, struct wlc_if *wlcif);
static int wlc_iovar_ext(wlc_info_t *wlc, const char *name,
	void *p, int plen, void *a, int alen, uint flags, bool set, struct wlc_if *wlcif);

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

/* ==== attach/detach ==== */

/* top level function to hook all iovars tables/dispatchers (from BMAC/PHY and WLC) together */
static int
BCMATTACHFN(wlc_register_iovt_all)(wlc_info_t *wlc, wlc_iocv_info_t *ii)
{
	phy_info_t *pi, *prev_pi = NULL;
	int err;
	uint i;

	/* N.B.: Register wlc_bmac module's iovar table.
	 * The dispatcher will be registered in wlc_bmac_register_iovt_all().
	 */
	if ((err = wlc_bmac_register_iovt(wlc->hw, ii)) != BCME_OK) {
		WL_ERROR(("%s: wlc_bmac_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register all PHY modules' iovar tables */
	for (i = 0; i < MAXBANDS; i++) {
		wlcband_t *band = wlc->bandstate[i];

		if (band == NULL)
			continue;

		pi = (phy_info_t *)WLC_PI_BANDUNIT(wlc, i);
		if (pi == NULL)
			continue;

		if (pi == prev_pi)
			continue;

		if ((err = phy_high_register_iovt_all(band->phytype, ii)) != BCME_OK) {
			WL_ERROR(("%s: phy_high_register_iovt_all failed\n", __FUNCTION__));
			goto fail;
		}

		prev_pi = pi;
	}

	return BCME_OK;

fail:
	return err;
}

/* top level function to hook all ioctls' tables/dispatchers (from BMAC/PHY and WLC) together */
static int
BCMATTACHFN(wlc_register_ioct_all)(wlc_info_t *wlc, wlc_iocv_info_t *ii)
{
	phy_info_t *pi, *prev_pi = NULL;
	int err;
	uint i;

	/* Register all PHY modules' ioctl tables */
	for (i = 0; i < MAXBANDS; i++) {
		wlcband_t *band = wlc->bandstate[i];

		if (band == NULL)
			continue;

		pi = (phy_info_t *)WLC_PI_BANDUNIT(wlc, i);
		if (pi == NULL)
			continue;

		if (pi == prev_pi)
			continue;

		if ((err = phy_high_register_ioct_all(band->phytype, ii)) != BCME_OK) {
			WL_ERROR(("%s: phy_high_register_ioct_all failed\n", __FUNCTION__));
			goto fail;
		}

		prev_pi = pi;
	}

	return BCME_OK;

fail:
	return err;
}

/* attach/detach */
wlc_iocv_info_t *
BCMATTACHFN(wlc_iocv_attach)(wlc_info_t *wlc)
{
	wlc_iocv_info_t *iocvi;

	/* Allocate iovar/ioctl table (pointer) registry for WLC */
	if ((iocvi = wlc_iocv_high_attach(wlc->osh,
			WLC_IOVT_HIGH_REG_SZ, WLC_IOCT_HIGH_REG_SZ, wlc, wlc->hw)) == NULL) {
		WL_ERROR(("wl%d: %s: wlc_iocv_high_attach failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* Register iovar tables for all modules */
	if (wlc_register_iovt_all(wlc, iocvi) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_register_iovt_all failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* Register ioctl tables for all modules */
	if (wlc_register_ioct_all(wlc, iocvi) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_register_ioct_all failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* add top level ioctl dispacher */
	if (wlc_iocv_add_ioc_fn(iocvi, NULL, 0, wlc_iocv_doioctl, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iocv_add_ioc_fn failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	wlc_dump_register(wlc->pub, "iocv", (dump_fn_t)wlc_iocv_high_dump, iocvi);
#endif

	return iocvi;

fail:
	wlc_iocv_detach(iocvi);
	return NULL;
}

void
BCMATTACHFN(wlc_iocv_detach)(wlc_iocv_info_t *iocvi)
{
	if (iocvi == NULL)
		return;

	wlc_iocv_high_detach(iocvi);
}

/* ==== ioctl dispatching ==== */

/** This function is always forced to RAM to support WLTEST_DISABLED */
static int
wlc_ioctl_filter(wlc_info_t *wlc, int cmd, void *arg, int len)
{
	BCM_REFERENCE(len);
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(arg);
	BCM_REFERENCE(cmd);

#if defined(OPENSRC_IOV_IOCTL) && !defined(BCMDBG)
	/* List of commands supported by linux sta hybrid module */
	switch (cmd) {
	case WLC_SET_DTIMPRD:
	case WLC_SET_REGULATORY:
	case WLC_SET_CLOSED:
	case WLC_SET_COUNTRY:
	case WLC_FREQ_ACCURACY:
	case WLC_CARRIER_SUPPRESS:
	case WLC_GET_PHYREG:
	case WLC_SET_PHYREG:
	case WLC_GET_RADIOREG:
	case WLC_SET_RADIOREG:
	case WLC_SET_MACMODE:
	case WLC_SET_WDSLIST:
	case WLC_SET_CLK:
	case WLC_SET_SUP_RATESET_OVERRIDE:
	case WLC_SET_ASSOC_PREFER:
	case WLC_SET_BAD_FRAME_PREEMPT:
	case WLC_SET_SPECT_MANAGMENT:
		return BCME_UNSUPPORTED;
	}
#endif /* OPENSRC_IOV_IOCTL && !BCMDBG */

#ifdef WLTEST_DISABLED
	/* ioctls encapsulated by WLTEST */
	switch (cmd) {
	case WLC_GET_PHYREG:
	case WLC_SET_PHYREG:
	case WLC_GET_TSSI:
	case WLC_GET_ATTEN:
	case WLC_SET_ATTEN:
	case WLC_LONGTRAIN:
	case WLC_EVM:
	case WLC_FREQ_ACCURACY:
	case WLC_CARRIER_SUPPRESS:
	case WLC_GET_PWRIDX:
	case WLC_SET_PWRIDX:
	case WLC_GET_MSGLEVEL:
	case WLC_SET_MSGLEVEL:
	case WLC_GET_UCANTDIV:
	case WLC_SET_UCANTDIV:
	case WLC_SET_SROM:
	case WLC_NVRAM_GET:
#if !defined(WL_EXPORT_CURPOWER)
	case WLC_CURRENT_PWR:
#endif /* !WL_EXPORT_CURPOWER */
	case WLC_OTPW:
	case WLC_NVOTPW:
		return BCME_UNSUPPORTED;
	}
#endif /* WLTEST_DISABLED */

#if defined(BCMDBG)
	switch (cmd) {
	case WLC_EVM: {
		/* EVM is only defined for CCK rates */
		ratespec_t *rspec = (((uint *)arg) + 1);

		if (!rspec || !RSPEC_ISCCK(*rspec)) {
			return BCME_BADARG;
		}

		break;
	}
	}
#endif 

	return BCME_OK;
} /* wlc_ioctl_filter */

/* A few commands don't need any arguments; all the others do. */
static int
wlc_ioctl_check_len(wlc_info_t *wlc, int cmd, void *arg, int len, struct wlc_if *wlcif)
{
	int bcmerror = BCME_OK;
	BCM_REFERENCE(wlcif);

	switch (cmd) {
	case WLC_UP:
	case WLC_OUT:
	case WLC_DOWN:
	case WLC_DISASSOC:
	case WLC_TERMINATED:
	case WLC_REBOOT:
	case WLC_START_CHANNEL_QA:
	case WLC_INIT:
	case WLC_SOFT_RESET:
	case WLC_DISASSOC_MYAP:
		break;

	default:
		if ((arg == NULL) || (len <= 0)) {
			WL_ERROR(("wl%d: %s: Command %d needs arguments\n",
			          wlc->pub->unit, __FUNCTION__, cmd));
			bcmerror = BCME_BADARG;
			break;
		}
	}

	return bcmerror;
}

/** Check module IOCTL parameters. */
static int
wlc_module_ioctl_check(wlc_info_t *wlc, const wlc_ioctl_cmd_t *ci,
	void *arg, int len, wlc_if_t *wlcif)
{
	int err = BCME_OK;
	uint16 flags = ci->flags;
	uint band;

	BCM_REFERENCE(arg);

	/* Found module, check argument length */
	if (len < ci->min_len)
		err = BCME_BADARG;
	/* Check flags */
	else if ((flags & WLC_IOCF_DRIVER_DOWN) && wlc->pub->up)
		err = BCME_NOTDOWN;
	else if ((flags & WLC_IOCF_DRIVER_UP) && !wlc->pub->up)
		err = BCME_NOTUP;
	else if ((flags & WLC_IOCF_FIXED_BAND) && IS_MBAND_UNLOCKED(wlc))
		err = BCME_NOTBANDLOCKED;
	else if ((flags & WLC_IOCF_CORE_CLK) && !wlc->clk)
		err = BCME_NOCLK;

#if defined(OPENSRC_IOV_IOCTL) && !defined(BCMDBG)
	else if ((flags & IOVF_OPEN_ALLOW) == 0)
		err = BCME_UNSUPPORTED;
#endif

#ifdef WLTEST_DISABLED
	else if (flags & WLC_IOCF_MFG)
		err = BCME_UNSUPPORTED;
#endif

	else if ((flags & (WLC_IOCF_BSSCFG_STA_ONLY | WLC_IOCF_BSSCFG_AP_ONLY)) != 0) {
		wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
		BCM_REFERENCE(cfg);
		ASSERT(cfg != NULL);

		if ((flags & WLC_IOCF_BSSCFG_STA_ONLY) && !BSSCFG_STA(cfg))
			err = BCME_NOTSTA;
		else if ((flags & WLC_IOCF_BSSCFG_AP_ONLY) && !BSSCFG_AP(cfg))
			err = BCME_NOTAP;
	}
	else if ((flags & WLC_IOCF_REG_CHECK) &&
	         /* optional band is stored in the second integer of incoming buffer */
	         (band = (len < (int)(2 * sizeof(int))) ? WLC_BAND_AUTO : ((int *)arg)[1],
	          err = wlc_iocregchk(wlc, band)) != BCME_OK)
		;
	else if ((flags & WLC_IOCF_REG_CHECK_AUTO) &&
	         (err = wlc_iocregchk(wlc, WLC_BAND_AUTO)) != BCME_OK)
		;
	else if ((flags & WLC_IOCF_BAND_CHECK_AUTO) &&
	         (err = wlc_iocbandchk(wlc, NULL, 0, NULL, FALSE)) != BCME_OK)
		;

	return err;
}

/** Perform module IOCTL handling. */
static int
wlc_module_do_ioctl(wlc_info_t *wlc, int cmd, void *arg, int len, wlc_if_t *wlcif)
{
	const wlc_ioctl_cmd_t *ci = NULL;
	uint16 tid;
	int err;

	/* find the command and the table */
	if ((ci = wlc_iocv_high_find_ioc(wlc->iocvi, (uint)cmd, &tid)) == NULL) {
		WL_NONE(("%s: wlc_iocv_high_find_ioc failed\n", __FUNCTION__));
		/* cmd isn't found in any of registered ioctl tables hence
		 * let's try every dispatcher that doesn't have corresponding
		 * ioctl table...
		 */
		return wlc_iocv_high_proc_ioc(wlc->iocvi, cmd, arg, len, wlcif);
	}

	/* found the cmd in one of the tables so let's check arg and flags */
	if ((err = wlc_module_ioctl_check(wlc, ci, arg, len, wlcif)) != BCME_OK) {
		WL_NONE(("%s: wlc_module_ioctl_check failed\n", __FUNCTION__));
		return err;
	}

	/* forward cmd to dispatcher */
	return wlc_iocv_high_fwd_ioc(wlc->iocvi, tid, ci, arg, len, wlcif);
} /* wlc_module_do_ioctl */

/* top level ioctl/iovarl dispatcher.
 * returns:
 * 0: success
 * < 0: failure
 * > 0: see wlc_seq_cmds_process
 */
int
wlc_ioctl(wlc_info_t *wlc, int cmd, void *arg, int len, struct wlc_if *wlcif)
{
	int seq_cmd_rc;
	int bcmerror;
	wl_if_t *wlif = NULL;

	BCM_REFERENCE(wlif);

	if (wlcif != NULL) {
		wlif = wlcif->wlif;
	}

	WL_OID(("wl%d: wlc_ioctl: interface %s cmd %d arg %p len %d\n",
	        wlc->pub->unit, wl_ifname(wlc->wl, wlif), cmd, OSL_OBFUSCATE_BUF(arg), len));

	/* If the device is turned off, then it's not "removed" */
	if (!wlc->pub->hw_off && DEVICEREMOVED(wlc)) {
		WL_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		bcmerror = BCME_ERROR;
		wl_down(wlc->wl);
		goto done;
	}

	ASSERT(!(wlc->pub->hw_off && wlc->pub->up));

	/* If the IOCTL is part of a "batched" sequence command, simply queue it
	 * for future execution, and return a dummy success response. If not,
	 * process it immediately.
	 */
	seq_cmd_rc = wlc_seq_cmds_process(wlc->seq_cmds_info, cmd, arg, len, wlcif);
	if (SEQ_CMDS_BUFFERED == seq_cmd_rc) {
		return (BCME_OK);
	}
	if (SEQ_CMDS_NOT_BUFFERED != seq_cmd_rc) {
		bcmerror = seq_cmd_rc;
		goto done;
	}

	/* run through filters and checks... */
	bcmerror = wlc_ioctl_filter(wlc, cmd, arg, len);
	if (bcmerror != BCME_OK) {
		goto done;
	}

	bcmerror = wlc_ioctl_check_len(wlc, cmd, arg, len, wlcif);
	if (bcmerror != BCME_OK) {
		goto done;
	}

	/* process the ioctl now that we are good so far... */
	bcmerror = wlc_module_do_ioctl(wlc, cmd, arg, len, wlcif);

done:
	if (bcmerror != BCME_OK) {
		if (VALID_BCMERROR(bcmerror)) {
			wlc->pub->bcmerror = bcmerror;
		} else {
			bcmerror = BCME_OK;
		}
	}

	return bcmerror;
}

/** simplified integer set interface for common ioctl handler */
int
wlc_set(wlc_info_t *wlc, int cmd, int arg)
{
	return wlc_ioctl(wlc, cmd, (void *)&arg, sizeof(arg), NULL);
}

/** simplified integer get interface for common ioctl handler */
int
wlc_get(wlc_info_t *wlc, int cmd, int *arg)
{
	return wlc_ioctl(wlc, cmd, arg, sizeof(int), NULL);
}

/** consolidated register access ioctl error checking */
int
wlc_iocregchk(wlc_info_t *wlc, uint band)
{
	/* if band is specified, it must be the current band */
	if ((band != WLC_BAND_AUTO) && (band != (uint)wlc->band->bandtype))
		return (BCME_BADBAND);

	/* if multiband and band is not specified, band must be locked */
	if ((band == WLC_BAND_AUTO) && IS_MBAND_UNLOCKED(wlc))
		return (BCME_NOTBANDLOCKED);

	/* must have core clocks */
	if (!wlc->clk)
		return (BCME_NOCLK);

	return (0);
}

/** For some ioctls, make sure that the pi pointer matches the current phy */
int
wlc_iocpichk(wlc_info_t *wlc, uint phytype)
{
	if (wlc->band->phytype != phytype)
		return BCME_BADBAND;
	return 0;
}

/**
 * Checking band specifications for ioctls.  See if there's a supplied band
 * arg and return it (ALL, band A, or band B) if it's acceptable (valid).
 * If no band is supplied, or if it is explicitly AUTO, return the "current
 * band" if that's definitive (single-band or band locked).  [AUTO on a
 * dual-band product without bandlocking is ambiguous, since the "current"
 * band may be temporary, e.g. during a roam scan.]
 */
int
wlc_iocbandchk(wlc_info_t *wlc, int *arg, int len, uint *bands, bool clkchk)
{
	uint band;

	/* get optional band */
	band = (len < (int)(2 * sizeof(int))) ? WLC_BAND_AUTO : arg[1];

	/* If band is not specified (AUTO), determine band to use.
	 * Only valid if single-band or bandlocked (else error).
	 */
	if (band == WLC_BAND_AUTO) {
		if (IS_MBAND_UNLOCKED(wlc))
			return (BCME_NOTBANDLOCKED);
		band = (uint) wlc->band->bandtype;
	}

	/* If band is specified, it must be either a valid band or "all" */
	if (band != WLC_BAND_ALL) {
		if ((band != WLC_BAND_2G) && (band != WLC_BAND_5G))
			return (BCME_BADBAND);

		if ((NBANDS(wlc) == 1) && (band != (uint) wlc->band->bandtype))
			return (BCME_BADBAND);
	}

	/* no need clocks */
	if (clkchk && !wlc->clk)
		return (BCME_NOCLK);

	if (bands)
		*bands = band;

	return BCME_OK;
}

/* ==== iovar dispatching ==== */

static int
wlc_iovar_rangecheck(wlc_info_t *wlc, uint32 val, const bcm_iovar_t *vi)
{
	int err = 0;
	uint32 min_val = 0;
	uint32 max_val = 0;

	BCM_REFERENCE(wlc);

	/* Only ranged integers are checked */
	switch (vi->type) {
	case IOVT_INT32:
		max_val |= 0x7fffffff;
		/* fall through */
	case IOVT_INT16:
		max_val |= 0x00007fff;
		/* fall through */
	case IOVT_INT8:
		max_val |= 0x0000007f;
		min_val = ~max_val;
		if (vi->flags & IOVF_NTRL)
			min_val = 1;
		else if (vi->flags & IOVF_WHL)
			min_val = 0;
		/* Signed values are checked against max_val and min_val */
		if ((int32)val < (int32)min_val || (int32)val > (int32)max_val)
			err = BCME_RANGE;
		break;

	case IOVT_UINT32:
		max_val |= 0xffffffff;
		/* fall through */
	case IOVT_UINT16:
		max_val |= 0x0000ffff;
		/* fall through */
	case IOVT_UINT8:
		max_val |= 0x000000ff;
		if (vi->flags & IOVF_NTRL)
			min_val = 1;
		if ((val < min_val) || (val > max_val))
			err = BCME_RANGE;
		break;
	}

	return err;
}

/** This function is always forced to RAM to support WLTEST_DISABLED */
static int
wlc_iovar_filter(wlc_info_t *wlc, const bcm_iovar_t *vi)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(vi);

#ifdef WLTEST_DISABLED
	if (vi->flags & IOVF_MFG)
		return BCME_UNSUPPORTED;
#endif

	return 0;
}

static int
wlc_iovar_check(wlc_info_t *wlc, const bcm_iovar_t *vi, void *arg, int len, bool set,
	wlc_if_t *wlcif)
{
	int err = BCME_OK;
	int32 int_val = 0;
	uint16 flags;

	/* check generic condition flags */
	if (set) {
		if (((vi->flags & IOVF_SET_DOWN) && wlc->pub->up) ||
		    ((vi->flags & IOVF_SET_UP) && !wlc->pub->up)) {
			err = (wlc->pub->up ? BCME_NOTDOWN : BCME_NOTUP);
		} else if ((vi->flags & IOVF_SET_BAND) && IS_MBAND_UNLOCKED(wlc)) {
			err = BCME_NOTBANDLOCKED;
		} else if ((vi->flags & IOVF_SET_CLK) && !wlc->clk) {
			err = BCME_NOCLK;
		}
	} else {
		if (((vi->flags & IOVF_GET_DOWN) && wlc->pub->up) ||
		    ((vi->flags & IOVF_GET_UP) && !wlc->pub->up)) {
			err = (wlc->pub->up ? BCME_NOTDOWN : BCME_NOTUP);
		} else if ((vi->flags & IOVF_GET_BAND) && IS_MBAND_UNLOCKED(wlc)) {
			err = BCME_NOTBANDLOCKED;
		} else if ((vi->flags & IOVF_GET_CLK) && !wlc->clk) {
			err = BCME_NOCLK;
		}
	}

#if defined(OPENSRC_IOV_IOCTL) && !defined(BCMDBG)
	if ((vi->flags & IOVF_OPEN_ALLOW) == 0)
		err = BCME_UNSUPPORTED;
#endif

	if (err)
		goto exit;

	if ((err = wlc_iovar_filter(wlc, vi)) < 0)
		goto exit;

	if ((flags = (vi->flags & (IOVF_BSSCFG_STA_ONLY | IOVF_BSSCFG_AP_ONLY |
		IOVF_BSS_SET_DOWN))) != 0) {
		wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
		ASSERT(cfg != NULL);

		if (set && ((flags & IOVF_BSS_SET_DOWN) != 0) && cfg->up) {
			err = BCME_NOTDOWN;
			goto exit;
		}

		flags &= ~IOVF_BSS_SET_DOWN;

		switch (flags) {
		case IOVF_BSSCFG_STA_ONLY:
			if (!BSSCFG_STA(cfg)) {
				err = BCME_NOTSTA;
				goto exit;
			}
			break;
		case IOVF_BSSCFG_AP_ONLY:
			if (!BSSCFG_AP(cfg)) {
				err = BCME_NOTAP;
				goto exit;
			}
			break;
		case IOVF_BSSCFG_STA_ONLY|IOVF_BSSCFG_AP_ONLY:
			err = BCME_UNSUPPORTED;
			goto exit;
		default:
			break; /* okay */
		}
	}

	/* length check on io buf */
	if ((err = bcm_iovar_lencheck(vi, arg, len, set)) < 0)
		goto exit;

	/* On set, check value ranges for integer types */
	if (set) {
		switch (vi->type) {
		case IOVT_BOOL:
		case IOVT_INT8:
		case IOVT_INT16:
		case IOVT_INT32:
		case IOVT_UINT8:
		case IOVT_UINT16:
		case IOVT_UINT32:
			bcopy(arg, &int_val, sizeof(int));
			err = wlc_iovar_rangecheck(wlc, int_val, vi);
			break;
		}
	}
exit:
	return err;
} /* wlc_iovar_check */

/* process the iovar described by tid, actionid, and 'vi' */
static int
wlc_iocv_proc_iov(wlc_info_t *wlc, uint16 tid, uint32 actionid, const bcm_iovar_t *vi,
	void *params, int p_len, void *arg, int len, int val_size,
	bool set, struct wlc_if *wlcif)
{
	int err = wlc_iocv_high_fwd_iov(wlc->iocvi, tid, actionid, vi,
			params, p_len, arg, len, val_size, wlcif);
	if (err != BCME_OK) {
		return err;
	}

	/* if cache is enabled, invalidate - ensure use NOW of iovar setting */
	if (set && WLC_TXC_ENAB(wlc)) {
		wlc_txc_info_t *txc = wlc->txc;

		if (TXC_CACHE_ENAB(txc)) {
			wlc_txc_inv_all(txc);
		}
	}

	return err;
}

/* wlc_iovar_ext flags */
#define IOVOP_FLAG_CONT_BUF	(1 << 0)
#define IOVOP_FLAG_ALLOW_ALIGN	(1 << 1)

#define IOVOP_FLAG_ALIGN_PARM	(IOVOP_FLAG_CONT_BUF | IOVOP_FLAG_ALLOW_ALIGN)
#define IOVOP_ALIGN_PARM(flags)	(((flags) & IOVOP_FLAG_ALIGN_PARM) == IOVOP_FLAG_ALIGN_PARM)

/* wlc_iovar_ext literals */
#define WLC_IOVBUF_WLCIDX_PREFIX	"wlc:"
#define WLC_IOVBUF_WLCIDX_PREFIX_SIZE	(sizeof(WLC_IOVBUF_WLCIDX_PREFIX) - 1)
#define WLC_IOVBUF_BSSCFGIDX_PREFIX	"bsscfg:"
#define WLC_IOVBUF_BSSCFGIDX_PREFIX_SIZE (sizeof(WLC_IOVBUF_BSSCFGIDX_PREFIX) - 1)

/* 'cont' indicates if name/params/arg are in the contiguous buffer,
 * 'align' allows params/arg alignment to be performed.
 *
 * Contiguous iovar buffer format in WLC_GET_VAR/WLC_SET_VAR:
 *
 *                [wlc:][bsscfg:]<name>\0[<wlcidx>][<bsscfgidx>]<param>
 *                ^                      ^
 * upon enter: 'name'                    ^
 *                ^                      ^
 *        get:  'arg'                 'params'
 *                                       ^
 *        set: ('params' = NULL)       'arg'
 *
 * <wlcidx> and <bsscfgidx> are all 32 bit wide if present.
 */
/* TODO:
 * Contiguous iovar buffer format in upcoming WLC_GET_VAR_ALIGN/WLC_SET_VAR_ALIGN:
 *
 *                [wlc:][bsscfg:]<name>\0[<pads>][<wlcidx>][<bsscfgidx>]<param>
 *                ^                              ^
 * upon enter: 'name'                            ^
 *                ^                              ^
 *        get:  'arg'                         'params'
 *                                               ^
 *        set: ('params' = NULL)               'arg'
 *
 * Always assume the iovar buffer starting address is 32 bit word aligned!
 * The iovar buffer builder adds necessary <pads> after the <name> null terminatoer
 * to make the [wlc:][bsscfg:]<name> string plus null terminator plus <pads>
 * multiple of 32 bit words.
 *
 * The func. caller adjusts 'params' and 'arg' to align at the 32 bit word boundary!
 */
static int
wlc_iovar_ext(wlc_info_t *wlc, const char *name,
	void *params, int p_len, void *arg, int len, uint flags,
	bool set, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	int val_size;
	const bcm_iovar_t *vi = NULL;
	uint32 actionid;
	uint16 tid;
#ifdef WLRSDB
	int32 wlc_idx = -1;
	int idx;
	wlc_info_t *wlc_update;
#endif
	const uint8 *iovbuf = NULL;
	uint move = 0;
	bool moved = FALSE;
	uint8 save[4];

	ASSERT(name != NULL);
	ASSERT(len >= 0);

	/* Get MUST have return space */
	ASSERT(set || (arg && len));

	ASSERT(!(wlc->pub->hw_off && wlc->pub->up));

	/* Set does NOT take qualifiers */
	ASSERT(!set || (params == NULL && p_len == 0));

	WL_NONE(("wl%d: IOVAR %s\n", wlc->pub->unit, name));


	/* find the given iovar name */
	if ((vi = wlc_iocv_high_find_iov(wlc->iocvi, name, &tid)) == NULL) {
		err = BCME_UNSUPPORTED;
		goto exit;
	}

	WL_OID(("wl%d: tid %u iovar \"%s\" %s len %d p_len %d\n",
	        wlc->pub->unit, tid, name, set ? "set" : "get",
	        len, p_len));

	/* check for alignment only for integer IOVAR types */
	if (!set && BCM_IOVT_IS_INT(vi->type) && (len == sizeof(int)) &&
	    !ISALIGNED(arg, sizeof(int))) {
		WL_ERROR(("wl%d: %s unaligned get ptr for %s\n",
			wlc->pub->unit, __FUNCTION__, name));
		ASSERT(0);
	}

	/* set up 'params' pointer in case this is a set command so that
	 * the convenience int and bool code can be common to set and get
	 */
	if (params == NULL) {
		params = arg;
		p_len = len;
	}

	/* iovar buffer starts with name string */
	if (IOVOP_ALIGN_PARM(flags)) {
		iovbuf = (const uint8 *)name;
	}

#ifdef WLRSDB
	/* handle the "wlc:" prefix and assign the correct wlc */
	if (RSDB_ENAB(wlc->pub) &&
	    strncmp(WLC_IOVBUF_WLCIDX_PREFIX, name, WLC_IOVBUF_WLCIDX_PREFIX_SIZE) == 0) {
		if (!(vi->flags2 & IOVF2_RSDB_CORE_OVERRIDE)) {
			WL_ERROR(("iovar: %s does not support per core option\n", vi->name));
			err = BCME_BADOPTION;
			goto exit;
		}
		if (p_len < (int)sizeof(wlc_idx)) {
			err = BCME_BUFTOOSHORT;
			goto exit;
		}
		bcopy(params, &wlc_idx, sizeof(wlc_idx));
		if (wlc_idx >= MAX_RSDB_MAC_NUM) {
			err = BCME_BADARG;
			goto exit;
		}
		if (WLC_RSDB_SINGLE_MAC_MODE(WLC_RSDB_CURR_MODE(wlc)) && wlc_idx != 0) {
			err = BCME_NOTUP;
			goto exit;
		}
		wlc = wlc->cmn->wlc[wlc_idx];
		ASSERT(wlc != NULL);
		if (wlc == NULL) {
			err = BCME_BADARG;
			goto exit;
		}
		/* adjust the name and i/o pointers to skip over the wlc index
		 * parameters
		 */
		name += WLC_IOVBUF_WLCIDX_PREFIX_SIZE;
		params = (int8*)params + sizeof(wlc_idx);
		p_len -= sizeof(wlc_idx);
		if (set) {
			arg = (int8*)arg + sizeof(wlc_idx);
			len -= sizeof(wlc_idx);
		}
	}
#endif /* WLRSDB */

	/* handle the "bsscfg:" prefix and index arg for multi-bsscfg cases */
	if (strncmp(WLC_IOVBUF_BSSCFGIDX_PREFIX, name, WLC_IOVBUF_BSSCFGIDX_PREFIX_SIZE) == 0) {
		int32 bsscfg_idx;
		wlc_bsscfg_t *bsscfg;

		if (p_len < (int)sizeof(bsscfg_idx)) {
			err = BCME_BUFTOOSHORT;
			goto exit;
		}

		bcopy(params, &bsscfg_idx, sizeof(bsscfg_idx));

		bsscfg = wlc_bsscfg_find(wlc, bsscfg_idx, &err);
#ifdef AP
		if (err == BCME_NOTFOUND && set) {
			wlc_bsscfg_type_t type = {BSSCFG_TYPE_GENERIC, BSSCFG_GENERIC_AP};
			bsscfg = wlc_bsscfg_alloc(wlc, bsscfg_idx, &type, 0, NULL);
			if (bsscfg == NULL) {
				err = BCME_NOMEM;
			} else if ((err = wlc_bsscfg_init(wlc, bsscfg))) {
				WL_ERROR(("wl%d: wlc_bsscfg_init failed (%d)\n",
				          wlc->pub->unit, err));
				wlc_bsscfg_free(wlc, bsscfg);
			}
		}
#endif /* AP */
		if (err) {
			goto exit;
		}

		/* adjust the name and i/o pointers to skip over the bsscfg index
		 * parameters
		 */
		name += WLC_IOVBUF_BSSCFGIDX_PREFIX_SIZE;
		params = (int8*)params + sizeof(bsscfg_idx);
		p_len -= sizeof(bsscfg_idx);
		if (set) {
			arg = (int8*)arg + sizeof(bsscfg_idx);
			len -= sizeof(bsscfg_idx);
		}

		/* use the wlcif from possibly a different bsscfg! */
		ASSERT(bsscfg != NULL);
		wlcif = bsscfg->wlcif;
		ASSERT(wlcif != NULL);
#ifdef WLRSDB
		if (RSDB_ENAB(wlc->pub)) {
			/* RSDB: Fixup wlc from bsscfg */
			/* This _MUST_ override the -W parameter as well! */
			wlc = bsscfg->wlc;
		}
#endif /* WLRSDB */
	}

	/* make sure the parameter (past the name) is aligned at 4 byte
	 * boundary; move it upwards if necessary and possible...;
	 * move it back if set operation...
	 */
	if (IOVOP_ALIGN_PARM(flags)) {

		/* see comment above the function for what params and arg are... */

		if (set) {
			/* caller params:
			 * wlc_iovar_ext(wlc, arg, NULL, 0, (int8*)arg + i, len - i,
			 *               IOVOP_FLAG_ALIGN_PARM, IOV_SET, wlcif);
			 */
			if ((move = (uint)((uintptr)arg % sizeof(uint32))) > 0 &&
			    (uint8 *)arg - move >= iovbuf) {
				memcpy(save, (uint8 *)arg - move, move);
				memmove((uint8 *)arg - move, arg, len);
				params = (uint8 *)params - move;
				arg = (uint8 *)arg - move;
				moved = TRUE;
			}
		} else {
			/* caller params:
			 * wlc_iovar_ext(wlc, arg, (int8*)arg + i, len - i, arg, len,
			 *               IOVOP_FLAG_ALIGN_PARM, IOV_GET, wlcif);
			 */
			if ((move = (uint)((uintptr)params % sizeof(uint32))) > 0 &&
			    (uint8 *)params - move >= iovbuf) {
				memmove((uint8 *)params - move, params, p_len);
				params = (uint8 *)params - move;
			}
		}

		/* the name string from this point on may not be useful at all
		 * since we are done with lookup but just in case it's needed
		 * point the name variable to a valid name string...
		 */
		name = vi->name;
	}

	if ((err = wlc_iovar_check(wlc, vi, arg, len, set, wlcif)) != 0) {
		goto exit;
	}

	actionid = set ? IOV_SVAL(vi->varid) : IOV_GVAL(vi->varid);

	if (vi->type == IOVT_VOID) {
		val_size = 0;
	} else if (vi->type == IOVT_BUFFER) {
		val_size = len;
	} else {
		/* all other types are integer sized */
		val_size = sizeof(int);
	}

#ifdef WLRSDB
	if (RSDB_ENAB(wlc->pub) &&
	    wlc_rsdb_chkiovar(wlc, vi, actionid, wlc_idx)) {
		FOREACH_WLC(wlc->cmn, idx, wlc_update) {
			if ((err = wlc_iocv_proc_iov(wlc_update, tid, actionid, vi, params, p_len,
					arg, len, val_size, set, wlcif)) != BCME_OK) {
				goto exit;
			}
		}
	} else
#endif /* WLRSDB */
	if ((err = wlc_iocv_proc_iov(wlc, tid, actionid, vi, params, p_len,
			arg, len, val_size, set, wlcif)) != BCME_OK) {
		goto exit;
	}

	/* ==== ADD more processing code here when needed ==== */

exit:
	/* restore the buffer for set operation */
	if (IOVOP_ALIGN_PARM(flags)) {
		if (set) {
			if (moved > 0) {
				ASSERT(move > 0);
				memmove((uint8 *)arg + move, arg, len);
				memcpy((uint8 *)arg, save, move);
			}
		}
	}

	if (err && VALID_BCMERROR(err)) {
		WL_ERROR(("wl%d: %s: %s: BCME %d\n", WLCWLUNIT(wlc), __FUNCTION__, name, err));
		wlc->pub->bcmerror = err;
	}
	WL_NONE(("wl%d: %s: BCME %d (%s) %s\n", wlc->pub->unit, __FUNCTION__, err,
		bcmerrorstr(err), name));
	return err;
} /* wlc_iovar_ext */

/**
 * Get or set an iovar. The params/p_len pair specifies any additional qualifying parameters (e.g.
 * an "element index") for a get, while the arg/len pair is the buffer for the value to be set or
 * retrieved. Operation (get/set) is specified by the 'set' argument, interface context provided by
 * wlcif. All pointers may point into the same buffer.
 * NOTE:
 * It's the callers' responsibility to align 'params' and/or 'arg' to a 32 bit word boundary when
 * this function is called from with wlc (except WLC_GET_VAR/WLC_SET_VAR handler) as a direct
 * function call.
 */
int
wlc_iovar_op(wlc_info_t *wlc, const char *name,
	void *params, int p_len, void *arg, int len,
	bool set, struct wlc_if *wlcif)
{
	return wlc_iovar_ext(wlc, name, params, p_len, arg, len, 0, set, wlcif);
}

/** simplified integer get interface for common WLC_GET_VAR ioctl handler */
int
wlc_iovar_getint(wlc_info_t *wlc, const char *name, int *arg)
{
	return wlc_iovar_op(wlc, name, NULL, 0, arg, sizeof(int32), IOV_GET, NULL);
}

/** simplified integer set interface for common WLC_SET_VAR ioctl handler */
int
wlc_iovar_setint(wlc_info_t *wlc, const char *name, int arg)
{
	return wlc_iovar_op(wlc, name, NULL, 0, (void *)&arg, sizeof(arg), IOV_SET, NULL);
}

/** simplified int8 get interface for common WLC_GET_VAR ioctl handler */
int
wlc_iovar_getint8(wlc_info_t *wlc, const char *name, int8 *arg)
{
	int iovar_int;
	int err;

	err = wlc_iovar_op(wlc, name, NULL, 0, &iovar_int, sizeof(iovar_int), IOV_GET, NULL);
	if (!err)
		*arg = (int8)iovar_int;

	return err;
}

/** simplified bool get interface for common WLC_GET_VAR ioctl handler */
int
wlc_iovar_getbool(wlc_info_t *wlc, const char *name, bool *arg)
{
	int iovar_int;
	int err;

	err = wlc_iovar_op(wlc, name, NULL, 0, &iovar_int, sizeof(iovar_int), IOV_GET, NULL);
	if (!err)
		*arg = (bool)iovar_int;

	return err;
}

/* ==== ioctl dispatcher ==== */

/* IOCTL => IOVAR */
static int
wlc_iocv_doioctl(void *ctx, uint32 cmd, void *arg, uint len, struct wlc_if *wlcif)
{
	wlc_info_t *wlc = ctx;
	int bcmerror = BCME_OK;

	switch (cmd) {

	case WLC_SET_VAR:
	case WLC_GET_VAR: {
		/* validate the name value */
		char *name = (char*)arg;
		uint i;

		for (i = 0; i < (uint)len && *name != '\0'; i++, name++)
			;

		if (i == (uint)len) {
			bcmerror = BCME_BUFTOOSHORT;
			break;
		}
		i++; /* include the null in the string length */

		if (cmd == WLC_GET_VAR) {
			bcmerror = wlc_iovar_ext(wlc, arg, (int8*)arg + i, len - i, arg, len,
			                         IOVOP_FLAG_ALIGN_PARM, IOV_GET, wlcif);
		} else {
			bcmerror = wlc_iovar_ext(wlc, arg, NULL, 0, (int8*)arg + i, len - i,
			                         IOVOP_FLAG_ALIGN_PARM, IOV_SET, wlcif);
		}

		break;
	}

	case WLC_IOV_BLOCK_GET:
		/*
		 * The IO buffer is really IO.  On entry, contains three integers:
		 * max length of a name, first variable to transfer (in driver's "standard
		 * enumeration" and how many variables to transfer (max).
		 */
		bcmerror = wlc_iocv_high_iov_names(wlc->iocvi,
				((uint *)arg)[0], /* max name len */
				((uint *)arg)[1], /* variable start index */
				((uint *)arg)[2],
				arg, len);
		break;

	default:
		bcmerror = BCME_UNSUPPORTED;
		break;

	}

	return bcmerror;
}
