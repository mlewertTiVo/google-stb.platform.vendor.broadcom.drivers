/**
 * @file
 * @brief
 * packet tx complete callback management module source
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


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <bcmwpa.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc.h>
#ifdef PROP_TXSTATUS
#include <wlfc_proto.h>
#include <wl_wlfc.h>
#endif
#include <wlc_pcb.h>

/* packet callback descriptor */
typedef struct pkt_cb {
	pkcb_fn_t fn;		/* function to call when tx frame completes */
	void *arg;		/* void arg for fn */
	uint8 nextidx;		/* index of next call back if threading */
	bool entered;		/* recursion check */
} pkt_cb_t;

/* packet class callback descriptor */
typedef struct {
	wlc_pcb_fn_t *cb;	/* callback function table */
	uint8 size;		/* callback function table size */
	uint8 offset;		/* index byte offset in wlc_pkttag_t */
	uint8 mask;		/* mask in above byte */
	uint8 shift;		/* shift in above byte */
} wlc_pcb_cd_t;

/* module states */
struct wlc_pcb_info {
	wlc_info_t *wlc;
	int maxpktcb;
	pkt_cb_t *pkt_callback;	/* tx completion callback handlers */
	int maxpcbcds;
	wlc_pcb_cd_t *pcb_cd;	/* packet class callback registry */
};

/* packet class callback descriptor template - freed after attach! */
static wlc_pcb_cd_t BCMATTACHDATA(pcb_cd_def)[] = {
	{NULL, MAXCD1PCBS, OFFSETOF(wlc_pkttag_t, flags2), WLF2_PCB1_MASK, WLF2_PCB1_SHIFT},
	{NULL, MAXCD2PCBS, OFFSETOF(wlc_pkttag_t, flags2), WLF2_PCB2_MASK, WLF2_PCB2_SHIFT},
	{NULL, MAXCD3PCBS, OFFSETOF(wlc_pkttag_t, flags2), WLF2_PCB3_MASK, WLF2_PCB3_SHIFT},
};

/* local functions */
/* module entries */

/* Noop packet callback, used to abstract cancellled callbacks. */
static void wlc_pktcb_noop_fn(wlc_info_t *wlc, uint txs, void *arg);

/* callback invocation */
static void wlc_pcb_callback(wlc_pcb_info_t *pcbi, void *pkt, uint txs);
static void wlc_pcb_invoke(wlc_pcb_info_t *pcbi, void *pkt, uint txs);

static void _wlc_pcb_fn_invoke(void *ctx, void *pkt, uint txs);

static void BCMFASTPATH
wlc_pktcb_noop_fn(wlc_info_t *wlc, uint txs, void *arg)
{
	return; /* noop */
}

/* module entries */
wlc_pcb_info_t *
BCMATTACHFN(wlc_pcb_attach)(wlc_info_t *wlc)
{
	wlc_pcb_info_t *pcbi;
	int i;

	/* module states */
	if ((pcbi = MALLOCZ(wlc->osh, sizeof(wlc_pcb_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	pcbi->wlc = wlc;

	/* packet callbacks array */
	pcbi->pkt_callback = (pkt_cb_t *)
	        MALLOCZ(wlc->osh, (sizeof(pkt_cb_t) * (wlc->pub->tunables->maxpktcb + 1)));

	if (pcbi->pkt_callback == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	pcbi->maxpktcb = wlc->pub->tunables->maxpktcb;

	/* packet class callback class descriptors array */
	ASSERT(wlc->pub->tunables->maxpcbcds == ARRAYSIZE(pcb_cd_def));
	pcbi->maxpcbcds = wlc->pub->tunables->maxpcbcds;
	if ((pcbi->pcb_cd = MALLOC(wlc->osh, sizeof(pcb_cd_def))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	bcopy(pcb_cd_def, pcbi->pcb_cd, sizeof(pcb_cd_def));
	/* packet class callback descriptors */
	for (i = 0; i < pcbi->maxpcbcds; i++) {
		pcbi->pcb_cd[i].cb = (wlc_pcb_fn_t *)
		        MALLOCZ(wlc->osh, sizeof(wlc_pcb_fn_t) * pcb_cd_def[i].size);

		if (pcbi->pcb_cd[i].cb == NULL) {
			WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto fail;
		}
	}


	/* Register with OSL the callback to be called when PKTFREE is called (w/o tx attempt) */
	PKTFREESETCB(wlc->osh, _wlc_pcb_fn_invoke, pcbi);

	return pcbi;

fail:
	wlc_pcb_detach(pcbi);
	return NULL;
}

void
BCMATTACHFN(wlc_pcb_detach)(wlc_pcb_info_t *pcbi)
{
	wlc_info_t *wlc;
	int i;

	if (pcbi == NULL)
		return;

	wlc = pcbi->wlc;


	if (pcbi->pkt_callback != NULL)
		MFREE(wlc->osh, pcbi->pkt_callback, sizeof(pkt_cb_t) * (pcbi->maxpktcb + 1));

	if (pcbi->pcb_cd != NULL) {
		for (i = 0; i < pcbi->maxpcbcds; i ++) {
			if (pcbi->pcb_cd[i].cb == NULL)
				continue;
			MFREE(wlc->osh, pcbi->pcb_cd[i].cb,
			      sizeof(wlc_pcb_fn_t) * pcbi->pcb_cd[i].size);
		}
		MFREE(wlc->osh, pcbi->pcb_cd, sizeof(pcb_cd_def));
	}

	MFREE(wlc->osh, pcbi, sizeof(wlc_pcb_info_t));
}


/* Prepend pkt's callback list to new_pkt. This is useful mainly for A-MSDU */
void
wlc_pcb_fn_move(wlc_pcb_info_t *pcbi, void *new_pkt, void *pkt)
{
	wlc_pkttag_t *pt = WLPKTTAG(pkt);
	uint8 idx = pt->callbackidx;
	wlc_pkttag_t *npt;

	if (idx == 0)
		return;

	/* Go to the end of the chain */
	while (pcbi->pkt_callback[idx].nextidx)
		idx = pcbi->pkt_callback[idx].nextidx;

	npt = WLPKTTAG(new_pkt);
	pcbi->pkt_callback[idx].nextidx = npt->callbackidx;
	npt->callbackidx = pt->callbackidx;
	pt->callbackidx = 0;
}

/* register packet callback */
int
wlc_pcb_fn_register(wlc_pcb_info_t *pcbi, pkcb_fn_t fn, void *arg, void *pkt)
{
	pkt_cb_t *pcb;
	uint8 i;
	wlc_pkttag_t *pt;

	/* find a free entry */
	for (i = 1; i <= pcbi->maxpktcb; i++) {
		if (pcbi->pkt_callback[i].fn == NULL)
			break;
	}

	if (i > pcbi->maxpktcb) {
		WLCNTINCR(pcbi->wlc->pub->_cnt->pkt_callback_reg_fail);
		WL_ERROR(("wl%d: failed to register callback %p\n",
		          pcbi->wlc->pub->unit, fn));
		ASSERT(0);
		return (-1);
	}

	/* alloc and init next free callback struct */
	pcb = &pcbi->pkt_callback[i];
	pcb->fn = fn;
	pcb->arg = arg;
	pcb->entered = FALSE;
	/* Chain this callback */
	pt = WLPKTTAG(pkt);
	pcb->nextidx = pt->callbackidx;
	pt->callbackidx = i;

	return (0);
}

/* Search the callback table for a matching <fn,arg> callback, optionally
 * cancelling all matching callback(s). Return number of matching callbacks.
 */
int BCMFASTPATH
wlc_pcb_fn_find(wlc_pcb_info_t *pcbi, pkcb_fn_t fn, void *arg, bool cancel_all)
{
	int i;
	int matches = 0; /* count of matching callbacks */

	/* search the pktcb array for a matching <fn,arg> entry */
	for (i = 1; i <= pcbi->maxpktcb; i++) {

		pkt_cb_t *pcb = &pcbi->pkt_callback[i];

		if ((pcb->fn == fn) && (pcb->arg == arg)) { /* matching callback */
			matches++;

			if (cancel_all) {
				pcb->fn = wlc_pktcb_noop_fn; /* callback: noop */
				pcb->arg = (void *)pcb; /* self: dummy non zero arg */
			}
		}
	}

	return matches;
}

/* set packet class callback */
int
BCMATTACHFN(wlc_pcb_fn_set)(wlc_pcb_info_t *pcbi, int tbl, int cls, wlc_pcb_fn_t pcb)
{
	if (tbl >= pcbi->maxpcbcds)
		return BCME_NORESOURCE;

	if (cls >= pcbi->pcb_cd[tbl].size)
		return BCME_NORESOURCE;

	pcbi->pcb_cd[tbl].cb[cls] = pcb;
	return BCME_OK;
}

/* Invokes the callback chain attached to 'pkt' */
static void BCMFASTPATH
wlc_pcb_callback(wlc_pcb_info_t *pcbi, void *pkt, uint txs)
{
	wlc_pkttag_t *pt = WLPKTTAG(pkt);
	uint8 idx = pt->callbackidx;
	pkt_cb_t *pcb;

	/* No callbacks to this packet. Do nothing */
	if (idx == 0)
		return;

	ASSERT(idx <= pcbi->maxpktcb && pcbi->pkt_callback[idx].fn != NULL);
	while (idx != 0 && idx <= pcbi->maxpktcb) {
		pcb = &pcbi->pkt_callback[idx];

		/* ensure the callback is not called recursively */
		ASSERT(pcb->entered == FALSE);
		ASSERT(pcb->fn != NULL);
		pcb->entered = TRUE;
		/* call the function */
		(pcb->fn)(pcbi->wlc, txs, pcb->arg);
		pcb->fn = NULL;
		pcb->entered = FALSE;
		idx = pcb->nextidx;
		pcb->nextidx = 0;
	}
	pt->callbackidx = 0;
}

/* invokes the packet class callback chain attached to 'pkt' */
static void BCMFASTPATH
wlc_pcb_invoke(wlc_pcb_info_t *pcbi, void *pkt, uint txs)
{
	int tbl;

	/* invoke all registered callbacks in the order in defined by wlc_pcb_tbl */
	for (tbl = 0; tbl < pcbi->maxpcbcds; tbl ++) {
		wlc_pcb_cd_t *pcb_cd = &pcbi->pcb_cd[tbl];
		wlc_pkttag_t *pt = WLPKTTAG(pkt);
		uint8 *loc = (uint8 *)pt + pcb_cd->offset;
		uint idx = (*loc & pcb_cd->mask) >> pcb_cd->shift;
		wlc_pcb_fn_t cb;

		/* No callback for this packet. Do nothing */
		if (idx == 0)
			continue;

		/* call the function */
		ASSERT(idx < pcb_cd->size);
		if ((cb = pcb_cd->cb[idx]) != NULL)
			(cb)(pcbi->wlc, pkt, txs);
		/* clear after the invocation in case someone may check pcb value */
		*loc = (*loc & ~pcb_cd->mask);
	}
}

static bool BCMFASTPATH
wlc_pkt_cb_ignore(wlc_info_t *wlc, uint txs)
{
	uint supr_indication = (txs & TX_STATUS_SUPR_MASK) >> TX_STATUS_SUPR_SHIFT;

	/* If the frame has been suppressed for deferred delivery, skip the callback
	 * now as it will be attempted later.
	 */

	/* skip power save (PMQ) */
	if (supr_indication == TX_STATUS_SUPR_PMQ) {
		return 1;
	}

#ifdef WLP2P
	/* skip P2P (NACK_ABS) */
	if (P2P_ENAB(wlc->pub) &&
	    supr_indication == TX_STATUS_SUPR_NACK_ABS) {
		return 1;
	}
#endif

	return 0;
}

/* called from either at the end of transmit attempt (success or fail [dotxstatus()]),
 * or when it's freed by OSL [PKTFREE()].
 */
static void BCMFASTPATH
_wlc_pcb_fn_invoke(void *ctx, void *pkt, uint txs)
{
	wlc_pcb_info_t *pcbi = (wlc_pcb_info_t *)ctx;

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(pcbi->wlc->pub))
		wlc_process_wlhdr_txstatus(pcbi->wlc, WLFC_CTL_PKTFLAG_DISCARD, pkt, FALSE);
#endif

	wlc_pcb_callback(pcbi, pkt, txs);
	wlc_pcb_invoke(pcbi, pkt, txs);
}

void BCMFASTPATH
wlc_pcb_fn_invoke(wlc_pcb_info_t *pcbi, void *pkt, uint txs)
{
	wlc_info_t *wlc = pcbi->wlc;

	/* skip the callback if the frame txs indicates one of the ignorance conditions */
	if (wlc_pkt_cb_ignore(wlc, txs))
		return;

	_wlc_pcb_fn_invoke(pcbi, pkt, txs);
}