/*
 * Packet chaining module source file
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
 * $Id: wlc_pktc.c 660314 2016-09-20 05:47:54Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <proto/ethernet.h>
#include <proto/802.3.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_keymgmt.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_pktc.h>

/* iovars */
enum {
	IOV_PKTC = 1,		/* Packet chaining */
	IOV_PKTCBND = 2,	/* Max packets to chain */
	IOV_LAST
};

static const bcm_iovar_t pktc_iovars[] = {
	{"pktc", IOV_PKTC, 0, 0, IOVT_BOOL, 0},
	{"pktcbnd", IOV_PKTCBND, 0, 0, IOVT_INT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

/* local declarations */
static void wlc_pktc_scb_state_upd(void *ctx, scb_state_upd_data_t *data);

static int wlc_pktc_doiovar(void *hdl, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif);
static void wlc_pktc_watchdog(void *ctx);

#if defined(PKTC) || defined(PKTC_FDAP)
static const char BCMATTACHDATA(rstr_pktc_policy)[] = "pktc_policy";
#endif

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

/* attach/detach */
wlc_pktc_info_t *
BCMATTACHFN(wlc_pktc_attach)(wlc_info_t *wlc)
{
	wlc_pktc_info_t *pktc;

	if ((pktc = MALLOCZ(wlc->osh, sizeof(wlc_pktc_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	pktc->wlc = wlc;

	if (wlc_scb_state_upd_register(wlc, wlc_pktc_scb_state_upd, pktc) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_scb_state_upd_register failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register module -- needs to be last failure prone operation in this function */
	if (wlc_module_register(wlc->pub, pktc_iovars, "pktc", pktc, wlc_pktc_doiovar,
	                        wlc_pktc_watchdog, NULL, NULL)) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


#if defined(PKTC) || defined(PKTC_FDAP)
	/* initialize default pktc policy */
	pktc->policy = (uint8)getintvar(wlc->pub->vars, rstr_pktc_policy);
#endif
#if (defined(PKTC_DONGLE) && !defined(PKTC_FDAP))
	pktc->policy = PKTC_POLICY_SENDUPUC;
#endif

	return pktc;

fail:
	MODULE_DETACH(pktc, wlc_pktc_detach);
	return NULL;
}

void
BCMATTACHFN(wlc_pktc_detach)(wlc_pktc_info_t *pktc)
{
	wlc_info_t *wlc;

	if (pktc == NULL)
		return;

	wlc = pktc->wlc;

	wlc_module_unregister(wlc->pub, "pktc", pktc);

	MFREE(wlc->osh, pktc, sizeof(wlc_pktc_info_t));
}

/* module entries */
static void
wlc_pktc_watchdog(void *ctx)
{
	wlc_pktc_info_t *pktc = ctx;
	wlc_info_t *wlc = pktc->wlc;

	if (PKTC_ENAB(wlc->pub)) {
		struct scb *scb;
		struct scb_iter scbiter;

		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			scb->pktc_pps = 0;
		}
	}
}

static int
wlc_pktc_doiovar(void *hdl, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
	wlc_pktc_info_t *pktc = hdl;
	wlc_info_t *wlc = pktc->wlc;
	int err = BCME_OK;
	int32 int_val = 0;
	int32 *ret_int_ptr;
	bool bool_val;

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	bool_val = (int_val != 0) ? TRUE : FALSE;

	/* Do the actual parameter implementation */
	switch (actionid) {
	case IOV_GVAL(IOV_PKTC):
		*ret_int_ptr = (int32)wlc->pub->_pktc;
		break;

	case IOV_SVAL(IOV_PKTC):
		wlc->pub->_pktc = bool_val;
		break;

	case IOV_GVAL(IOV_PKTCBND):
		*ret_int_ptr = wlc->pub->tunables->pktcbnd;
		break;

	case IOV_SVAL(IOV_PKTCBND):
		wlc->pub->tunables->pktcbnd = MAX(int_val, wlc->pub->tunables->rxbnd);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
} /* wlc_doiovar */

/* enable/disable */
void
wlc_scb_pktc_enable(struct scb *scb, const wlc_key_info_t *key_info)
{
	wlc_bsscfg_t *bsscfg = SCB_BSSCFG(scb);
	wlc_info_t *wlc = bsscfg->wlc;
	wlc_key_info_t tmp_ki;

	if (wlc->wet)
		return;

#ifdef PKTC_DONGLE
	if (BSS_TDLS_BUFFER_STA(bsscfg))
		return;

	if (MONITOR_ENAB(wlc))
		return;
#endif

	if (!SCB_ASSOCIATED(scb) && !SCB_AUTHORIZED(scb))
		return;

	/* No chaining for wds, non qos, non ampdu stas */
	if (SCB_A4_DATA(scb) || !SCB_QOS(scb) || !SCB_WME(scb) || !SCB_AMPDU(scb))
		return;

	if (key_info == NULL) {
		(void)wlc_keymgmt_get_scb_key(wlc->keymgmt, scb, WLC_KEY_ID_PAIRWISE,
			WLC_KEY_FLAG_NONE, &tmp_ki);
		key_info = &tmp_ki;
	}

	if (!WLC_KEY_ALLOWS_PKTC(key_info, bsscfg))
		return;

	SCB_PKTC_ENABLE(scb);
}

static void
wlc_scb_pktc_disable(wlc_pktc_info_t *pktc, struct scb *scb)
{
	int32 cidx;

	if (SCB_ASSOCIATED(scb) || SCB_AUTHORIZED(scb))
		return;

	/* Invalidate rfc entry if scb is in it */
	cidx = BSSCFG_AP(SCB_BSSCFG(scb)) ? 1 : 0;
	if (pktc->rfc[cidx].scb == scb) {
		WL_NONE(("wl%d: %s: Invalidate rfc %d before freeing scb %p\n",
		         pktc->wlc->pub->unit, __FUNCTION__, cidx, OSL_OBFUSCATE_BUF(scb)));
		pktc->rfc[cidx].scb = NULL;
	}

	SCB_PKTC_DISABLE(scb);
}

/* scb state update callback */
static void
wlc_pktc_scb_state_upd(void *ctx, scb_state_upd_data_t *data)
{
	struct scb *scb = data->scb;

	/* When transitioning to ASSOCIATED/AUTHORIZED state try if we can
	 * enable packet chaining for this SCB.
	 */
	if (SCB_ASSOCIATED(scb) || SCB_AUTHORIZED(scb)) {
		wlc_scb_pktc_enable(scb, NULL);
	}
	/* Clear scb pointer in rfc */
	else {
		wlc_scb_pktc_disable(ctx, scb);
	}
}

/* packet manipulations */
static void BCMFASTPATH
wlc_pktc_sdu_prep_copy(wlc_info_t *wlc, struct scb *scb, void *p, void *n, uint32 lifetime)
{
	uint8 *dot3lsh;

	ASSERT(n != NULL);

	WLPKTTAGSCBSET(n, scb);
	WLPKTTAGBSSCFGSET(n, WLPKTTAGBSSCFGGET(p));
#ifdef DMATXRC
	WLPKTTAG(n)->flags = WLPKTTAG(p)->flags & ~WLF_PHDR;
#else
	WLPKTTAG(n)->flags = WLPKTTAG(p)->flags;
#endif
	WLPKTTAG(n)->flags2 = WLPKTTAG(p)->flags2;
	WLPKTTAG(n)->flags3 = WLPKTTAG(p)->flags3;
	PKTSETPRIO(n, PKTPRIO(p));

	if (WLPKTTAG(p)->flags & WLF_NON8023) {

		/* Init llc snap hdr and fixup length */
		/* Original is ethernet hdr (14)
		 * Convert to 802.3 hdr (22 bytes) so only need to
		 * another 8 bytes (DOT11_LLC_SNAP_HDR_LEN)
		 */
		dot3lsh = PKTPUSH(wlc->osh, n, DOT11_LLC_SNAP_HDR_LEN);

		/* Copy only 20 bytes & retain orig ether_type */
		if ((((uintptr)PKTDATA(wlc->osh, p) | (uintptr)dot3lsh) & 3) == 0) {
			typedef struct block_s {
				uint32	data[5];
			} block_t;
			*(block_t*)dot3lsh = *(block_t*)PKTDATA(wlc->osh, p);
		} else {
			bcopy(PKTDATA(wlc->osh, p), dot3lsh,
				OFFSETOF(struct dot3_mac_llc_snap_header, type));
		}
	}

	/* Set packet exptime */
	if (lifetime != 0) {
		WLPKTTAG(n)->u.exptime = WLPKTTAG(p)->u.exptime;
	}
}

void BCMFASTPATH
wlc_pktc_sdu_prep(wlc_info_t *wlc, struct scb *scb, void *p, void *n, uint32 lifetime)
{
	struct dot3_mac_llc_snap_header *dot3lsh;

	wlc_pktc_sdu_prep_copy(wlc, scb, p, n, lifetime);
	dot3lsh = (struct dot3_mac_llc_snap_header *)PKTDATA(wlc->osh, n);
	dot3lsh->length = HTON16(pkttotlen(wlc->osh, n) - ETHER_HDR_LEN);
}

#ifdef DMATXRC
/** Prepend phdr and move p's 802.3 header/flags/attributes to phdr */
void* BCMFASTPATH
wlc_pktc_sdu_prep_phdr(wlc_info_t *wlc, struct scb *scb, void *p, uint32 lifetime)
{
	void *phdr = wlc_phdr_get(wlc);

	if (phdr == NULL)
		return NULL;

	ASSERT(!(WLPKTTAG(p)->flags & WLF_PHDR));

	/*
	 * wlc_pktc_sdu_prep_copy() assumes pkt has ether header
	 * so make room for it.
	 */
	PKTPUSH(wlc->osh, phdr, ETHER_HDR_LEN);

	/* Move 802.3 header/flags/etc. from p to phdr */
	wlc_pktc_sdu_prep_copy(wlc, scb, p, phdr, lifetime);
	PKTPULL(wlc->osh, p, sizeof(struct dot3_mac_llc_snap_header));

	WLPKTTAG(phdr)->flags |= WLF_PHDR;
	if (PROP_TXSTATUS_ENAB(wlc->pub))
		WLFC_PKTAG_INFO_MOVE(p, phdr);

	if (PKTLINK(p)) {
		PKTSETLINK(phdr, PKTLINK(p));
		PKTSETLINK(p, NULL);
		PKTSETCHAINED(wlc->osh, phdr);
		PKTCLRCHAINED(wlc->osh, p);
	}

	PKTSETNEXT(wlc->osh, phdr, p);
	ASSERT(PKTNEXT(wlc->osh, p) == NULL);

	return phdr;
}
#endif /* DMATXRC */
