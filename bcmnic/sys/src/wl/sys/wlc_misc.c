/*
 * Miscellanous features source code.
 * For features that don't warrant separate wlc modules.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_misc.c 613207 2016-01-18 06:00:27Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <bcmwpa.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <bcmdevs.h>
#include <pcie_core.h>
#include <wlc_hw_priv.h>
#include <wlc_misc.h>

/* IOVar table - please number the enumerators explicity */
enum {
	IOV_MCAST_REGEN_BSS_ENABLE = 1,
	IOV_SR_ENABLE = 2,
	IOV_DUMP_PMU = 3,
	IOV_PMU_KEEP_ON = 4,
	IOV_POWER_ISLAND = 5,
	IOV_LAST
};

static const bcm_iovar_t misc_iovars[] = {
#ifdef MCAST_REGEN
	{"mcast_regen_bss_enable", IOV_MCAST_REGEN_BSS_ENABLE, (0), IOVT_BOOL, 0},
#endif
#ifdef SAVERESTORE
#endif /* SAVERESTORE */
#ifdef SR_DEBUG
	{"sr_dump_pmu", IOV_DUMP_PMU, IOVF_OPEN_ALLOW, IOVT_BUFFER, sizeof(pmu_reg_t)},
	{"sr_pmu_keep_on", IOV_PMU_KEEP_ON, IOVF_SET_UP | IOVF_GET_UP, IOVT_UINT16, sizeof(int)},
	{"sr_power_island", IOV_POWER_ISLAND, IOVF_SET_UP | IOVF_GET_UP, IOVT_UINT16, sizeof(int)},
#endif /* SR_DEBUG */
	{NULL, 0, 0, 0, 0},
};

/* private info */
struct wlc_misc_info {
	wlc_info_t *wlc;
};

/* local functions */
static int
wlc_misc_doiovar(void *ctx, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *p, uint plen, void *a, int alen, int vsize, struct wlc_if *wlcif);

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

/* attach/detach */
wlc_misc_info_t *
BCMATTACHFN(wlc_misc_attach)(wlc_info_t *wlc)
{
	wlc_misc_info_t *misc;

	if ((misc = MALLOCZ(wlc->osh, sizeof(*misc))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	misc->wlc = wlc;

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, misc_iovars, "misc", misc, wlc_misc_doiovar,
	                        NULL, NULL, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	return misc;

fail:
	wlc_misc_detach(misc);
	return NULL;
}

void
BCMATTACHFN(wlc_misc_detach)(wlc_misc_info_t *misc)
{
	wlc_info_t *wlc;

	if (misc == NULL)
		return;

	wlc = misc->wlc;
	wlc_module_unregister(wlc->pub, "misc", misc);
	MFREE(wlc->osh, misc, sizeof(*misc));
}

/* iovar dispatcher */
static int
wlc_misc_doiovar(void *ctx, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *p, uint plen, void *a, int alen, int vsize, struct wlc_if *wlcif)
{
	wlc_misc_info_t *misc = (wlc_misc_info_t *)ctx;
	wlc_info_t *wlc = misc->wlc;
	int32 *ret_int_ptr;
	int32 int_val = 0;
	bool bool_val;
	int err = BCME_OK;
	wlc_bsscfg_t *bsscfg;
	BCM_REFERENCE(vi);
	BCM_REFERENCE(name);
	BCM_REFERENCE(alen);
	BCM_REFERENCE(vsize);

	if (plen >= (int)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)a;

	bool_val = int_val != 0;

	/* update bsscfg w/provided interface context */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	/* all iovars require mcnx being enabled */
	switch (actionid) {
#ifdef MCAST_REGEN
	case IOV_GVAL(IOV_MCAST_REGEN_BSS_ENABLE):
		*ret_int_ptr = bsscfg->mcast_regen_enable;
		break;
	case IOV_SVAL(IOV_MCAST_REGEN_BSS_ENABLE):
		bsscfg->mcast_regen_enable = bool_val;
		break;
#endif /* MCAST_REGEN */


#ifdef SR_DEBUG
	case IOV_GVAL(IOV_DUMP_PMU):
		si_dump_pmu(wlc->pub->sih, a);
		break;
	case IOV_GVAL(IOV_PMU_KEEP_ON):
		*ret_int_ptr = si_pmu_keep_on_get(wlc->pub->sih);
		break;
	case IOV_SVAL(IOV_PMU_KEEP_ON):
		si_pmu_keep_on(wlc->pub->sih, int_val);
		/* Calculating fast power up delay again as min_res_mask is changed now */
		wlc->hw->fastpwrup_dly = si_clkctl_fast_pwrup_delay(wlc->hw->sih);
		W_REG(wlc->osh, &wlc->regs->u.d11regs.scc_fastpwrup_dly, wlc->hw->fastpwrup_dly);
		break;
	case IOV_GVAL(IOV_POWER_ISLAND):
		*ret_int_ptr = si_power_island_get(wlc->pub->sih);
		break;
	case IOV_SVAL(IOV_POWER_ISLAND):
		*ret_int_ptr = si_power_island_set(wlc->pub->sih, int_val);
		if (*ret_int_ptr == 0) {
			err = BCME_UNSUPPORTED;
		}
		break;
#endif /*  SR_DEBUG */

	default:
		BCM_REFERENCE(ret_int_ptr);
		BCM_REFERENCE(bool_val);
		BCM_REFERENCE(bsscfg);
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

bool
wlc_is_4way_msg(wlc_info_t *wlc, void *pkt, int offset, wpa_msg_t msg)
{
	struct dot11_llc_snap_header * lsh;
	eapol_hdr_t *eapol = NULL;
	eapol_wpa_key_header_t *ek;
	unsigned short us_tmp;
	int pair, ack, mic, kerr, req, sec, install;
	bool is_4way_msg = FALSE;
	osl_t *osh = wlc->osh;
	uint total_len = pkttotlen(osh, pkt);
	uchar *pbody = (uchar*)(PKTDATA(osh, pkt) + offset);
	uint len1;
	void *next = NULL;

	/* This function assumes that total length consists of all 4 components
	 *  Offset, SNAP Header, EAPOL Header, EAPOL WPA Key
	 */
	if (total_len <
	    ((uint)offset +
	     DOT11_LLC_SNAP_HDR_LEN +
	     EAPOL_HDR_LEN +
	     EAPOL_WPA_KEY_LEN))
		return FALSE;

	total_len -= offset;
	len1 = PKTLEN(osh, pkt);

	/* This function only checks for the packets with LLCSNAP header */
	if (len1 >= ((uint)offset + DOT11_LLC_SNAP_HDR_LEN)) {
		lsh = (struct dot11_llc_snap_header *)(pbody);
	} else {
		return FALSE;
	}

	len1 -= (DOT11_LLC_SNAP_HDR_LEN + offset);
	pbody += DOT11_LLC_SNAP_HDR_LEN;

	if (!(lsh->dsap == 0xaa &&
	      lsh->ssap == 0xaa &&
	      lsh->ctl == 0x03 &&
	      lsh->oui[0] == 0 &&
	      lsh->oui[1] == 0 &&
	      lsh->oui[2] == 0 &&
	      ntoh16(lsh->type) == ETHER_TYPE_802_1X)) {
		return FALSE;
	}

	/* Move to the next buffer if we have reached the end of the current buffer */
	if (len1 == 0) {
		next  = PKTNEXT(osh, pkt);
		if (next == NULL)
			return FALSE;
		pbody = PKTDATA(osh, next);
		len1 = PKTLEN(osh, next);
	}

	/* This function only handles an EAPOL Hdr and Key in a contiguous range of memory.
	 * If the EAPOL hdr and Key do not fit in the current buffer, fail.
	 */
	 if (len1 < (EAPOL_HDR_LEN + EAPOL_WPA_KEY_LEN))
		return FALSE;

	eapol = (eapol_hdr_t*)pbody;

	/* make sure it is an EAPOL-Key frame big enough for an 802.11 Key desc */
	if ((eapol->version != WPA_EAPOL_VERSION && eapol->version != WPA2_EAPOL_VERSION) ||
	    (eapol->type != EAPOL_KEY) ||
	    (ntoh16(eapol->length) < EAPOL_WPA_KEY_LEN))
		return FALSE;

	ek = (eapol_wpa_key_header_t*)((int8*)eapol + EAPOL_HDR_LEN);
	/* m1 & m4 signatures on wpa are same on wpa2 */
	if (ek->type == EAPOL_WPA2_KEY || ek->type == EAPOL_WPA_KEY) {
		us_tmp = ntoh16_ua(&ek->key_info);

		pair = 	0 != (us_tmp & WPA_KEY_PAIRWISE);
		ack = 0	 != (us_tmp & WPA_KEY_ACK);
		mic = 0	 != (us_tmp & WPA_KEY_MIC);
		kerr = 	0 != (us_tmp & WPA_KEY_ERROR);
		req = 0	 != (us_tmp & WPA_KEY_REQ);
		sec = 0	 != (us_tmp & WPA_KEY_SECURE);
		install	 = 0 != (us_tmp & WPA_KEY_INSTALL);
		switch (msg) {
		case PMSG4:
		/* check if it matches 4-Way M4 signature */
			if (pair && !req && mic && !ack && !kerr && ek->data_len == 0)
				is_4way_msg = TRUE;
			break;
		case PMSG1:
			/* check if it matches 4-way M1 signature */
			if (!sec && !mic && ack && !install && pair && !kerr && !req)
				is_4way_msg = TRUE;
			break;
		default:
			break;
		}
	}

	return is_4way_msg;
} /* wlc_is_4way_msg */

void
wlc_sr_fix_up(wlc_info_t *wlc)
{
#if !defined(SAVERESTORE) || defined(SAVERESTORE_DISABLED)
	if ((CHIPID(wlc->pub->sih->chip) == BCM4345_CHIP_ID) ||
	    (CHIPID(wlc->pub->sih->chip) == BCM4335_CHIP_ID) ||
	    (CHIPID(wlc->pub->sih->chip) == BCM4349_CHIP_ID)) {

		si_force_islanding(wlc->hw->sih, FALSE);
		si_update_masks(wlc->hw->sih);

		/* Calculating fast power up delay again as min_res_mask is changed now */
		wlc->hw->fastpwrup_dly = si_clkctl_fast_pwrup_delay(wlc->hw->sih);
		W_REG(wlc->osh, &wlc->regs->u.d11regs.scc_fastpwrup_dly, wlc->hw->fastpwrup_dly);

		si_pcie_set_ctrlreg(wlc->pub->sih, PCIE_DISSPROMLD, PCIE_DISSPROMLD);
	}
#endif /* !SAVERESTORE || SAVERESTORE_DISABLED */
}
