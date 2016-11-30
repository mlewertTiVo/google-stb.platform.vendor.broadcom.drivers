/**
 * @file
 * @brief
 * tx header caching module source - It caches the d11 header of
 * a packet and copy it to the next packet if possible.
 * This feature saves a significant amount of processing time to
 * build a packet's d11 header from scratch.
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
 * $Id: wlc_txc.c 643064 2016-06-13 07:04:03Z $
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
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_keymgmt.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_txc.h>
#ifdef WLTOEHW
#include <wlc_tso.h>
#endif
#ifdef WLCXO
#include <wlc_cxo_tsc.h>
#include <wlc_cxo_ctrl.h>
#include <wlc_ampdu.h>
#include <wlc_key.h>
#endif /* WLCXO */
#include <wlc_hw.h>
#include <wlc_tx.h>
#include <wlc_dump.h>

#define txc_iovars NULL

#ifndef WLC_MAX_UCODE_CACHE_ENTRIES
#define WLC_MAX_UCODE_CACHE_ENTRIES 8
#endif

typedef struct scb_txc_info scb_txc_info_t;

/* module private states */
typedef struct {
	wlc_info_t *wlc;
	int scbh;
	/* states */
	bool policy;		/* 0:off 1:auto */
	bool sticky;		/* invalidate txc every second or not */
	uint gen;		/* tx header cache generation number */
	scb_txc_info_t **ucache_entry_inuse;
} wlc_txc_info_priv_t;

/* wlc_txc_info_priv_t offset in module states */
static uint16 wlc_txc_info_priv_offset = sizeof(wlc_txc_info_t);

/* module states layout */
typedef struct {
	wlc_txc_info_t pub;
	wlc_txc_info_priv_t priv;
#if D11CONF_GE(42) && defined(WLC_UCODE_CACHE)
	scb_txc_info_t *ucache_entry_inuse[WLC_MAX_UCODE_CACHE_ENTRIES];
#endif
} wlc_txc_t;
/* module states size */
#define WLC_TXC_INFO_SIZE	(sizeof(wlc_txc_t))
/* moudle states location */
#define WLC_TXC_INFO_PRIV(txc) ((wlc_txc_info_priv_t *) \
	((uintptr)(txc) + wlc_txc_info_priv_offset))

/* scb private states */
struct scb_txc_info {
	uint	txhlen;		/* #bytes txh[] valid, 0=invalid */
	uint	pktlen;		/* tag: original user packet length */
	uint	fifo;           /* fifo for the pkt */
	uint	flags;		/* pkt flags */
	uint8	prio;           /* pkt prio */
	uint8	ps_on;		/* the scb was in power save mode when cache was built */
	uint8	align;		/* alignment of hdrs start in txh array */
	uint16   offset;	/* offset of actual data from txh array start point */
	/* Don't change the order of next two variables, as txh need to start
	 * at a word aligned location
	 */
	uint8 ucache_idx;   /* cache index used by uCode. */
	uint	gen;		/* generation number (compare to priv->gen) */
	uint8	txh[TXOFF];	/* cached tx header */
};

#define SCB_TXC_CUBBY_LOC(txc, scb) ((scb_txc_info_t **) \
				     SCB_CUBBY(scb, WLC_TXC_INFO_PRIV(txc)->scbh))
#define SCB_TXC_INFO(txc, scb) (*SCB_TXC_CUBBY_LOC(txc, scb))

/* d11txh position in the cached entries */
#define SCB_TXC_TXH(sti) ((d11txh_t *)((sti)->txh + (sti)->align + (sti)->offset))
#define SCB_TXC_TXH_NO_OFFST(sti) ((d11txh_t *)((sti)->txh + (sti)->align))


/* handy macros */
#define INCCNTHIT(sti)
#define INCCNTUSED(sti)
#define INCCNTADDED(sti)
#define INCCNTMISSED(sti)

/* WLF_ flags treated as miss when toggling */
#define WLPKTF_TOGGLE_MASK	(WLF_EXEMPT_MASK | WLF_AMPDU_MPDU)
/* WLF_ flags to save in cache */
#define WLPKTF_STORE_MASK	(WLPKTF_TOGGLE_MASK | \
				 WLF_MIMO | WLF_VRATE_PROBE | WLF_RATE_AUTO | \
				 WLF_RIFS | WLF_WME_NOACK)

/* local function */
/* module entries */
#define wlc_txc_doiovar NULL
static void wlc_txc_watchdog(void *ctx);

#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
static uint8 wlc_txc_get_free_ucode_cidx(wlc_txc_info_t *txc);
#endif
/* scb cubby */
static int wlc_txc_scb_init(void *ctx, struct scb *scb);
static void wlc_txc_scb_deinit(void *ctx, struct scb *scb);
#define wlc_txc_scb_dump NULL
#ifdef WLCXO_CTRL
static void
wlc_txc_cxo_ctrl_tsc_init(wlc_info_t *wlc, void *p, struct scb *scb, scb_txc_info_t * sti);
static void
wlc_txc_cxo_ctrl_tsc_add(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb, void *pkt,
	scb_txc_info_t * sti);
#endif /* WLCXO_CTRL */

#define GET_TXHDRPTR(txc)	((d11txh_t*)(txc->txh + txc->align + txc->offset))

/* module entries */
wlc_txc_info_t *
BCMATTACHFN(wlc_txc_attach)(wlc_info_t *wlc)
{
	wlc_txc_info_t *txc;
	wlc_txc_info_priv_t *priv;

	/* module states */
	if ((txc = MALLOCZ(wlc->osh, WLC_TXC_INFO_SIZE)) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	txc->_txc = FALSE;

#if D11CONF_GE(42) && defined(WLC_UCODE_CACHE)
	txc->ucache_entry_inuse = &((wlc_txc_t *)txc)->ucache_entry_inuse[0];
#endif

	wlc_txc_info_priv_offset = OFFSETOF(wlc_txc_t, priv);
	priv = WLC_TXC_INFO_PRIV(txc);
	priv->wlc = wlc;
	priv->policy = ON;
	priv->gen = 0;
	/* debug builds should not invalidate the txc per watchdog */
	priv->sticky = FALSE;

	/* reserve a cubby in the scb */
	if ((priv->scbh =
	     wlc_scb_cubby_reserve(wlc, sizeof(scb_txc_info_t *),
	                           wlc_txc_scb_init, wlc_txc_scb_deinit,
	                           wlc_txc_scb_dump, txc)) < 0) {
		WL_ERROR(("wl%d: %s: cubby register for txc failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, txc_iovars, "txc", txc, wlc_txc_doiovar,
	                        wlc_txc_watchdog, NULL, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}


	return txc;

fail:
	MODULE_DETACH(txc, wlc_txc_detach);
	return NULL;
}

void
BCMATTACHFN(wlc_txc_detach)(wlc_txc_info_t *txc)
{
	wlc_txc_info_priv_t *priv;
	wlc_info_t *wlc;

	if (txc == NULL)
		return;

	priv = WLC_TXC_INFO_PRIV(txc);
	wlc = priv->wlc;

	wlc_module_unregister(wlc->pub, "txc", txc);

	MFREE(wlc->osh, txc, WLC_TXC_INFO_SIZE);
}


static void
wlc_txc_watchdog(void *ctx)
{
	wlc_txc_info_t *txc = (wlc_txc_info_t *)ctx;
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);

	/* invalidate tx header cache once per second if not sticky */
	if (!priv->sticky)
		priv->gen++;
}


uint16
wlc_txc_get_txh_offset(wlc_txc_info_t *txc, struct scb *scb)
{
	scb_txc_info_t *sti;
	sti = SCB_TXC_INFO(txc, scb);
	return sti->offset;
}
uint32
wlc_txc_get_d11hdr_len(wlc_txc_info_t *txc, struct scb *scb)
{
	scb_txc_info_t *sti;
	sti = SCB_TXC_INFO(txc, scb);
	return (sti->txhlen - sti->offset);
}

#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
uint8* wlc_txc_get_rate_info_shdr(wlc_txc_info_t *txc, int cache_idx)
{
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);
	scb_txc_info_t *sti = priv->ucache_entry_inuse[cache_idx];
	ASSERT(sti != NULL);
	return (sti->txh + sti->txhlen + sti->align);
}
#endif /* defined(WLC_UCODE_CACHE) && D11CONF_GE(42) */

void wlc_txc_prep_sdu(wlc_txc_info_t *txc, struct scb *scb, wlc_key_t *key,
    const wlc_key_info_t *key_info,  void *p)
{
	wlc_txc_info_priv_t *priv;
	scb_txc_info_t *sti;
	wlc_info_t *wlc;
	wlc_txd_t *txd;
	int d11_off;
	BCM_REFERENCE(key_info);

	ASSERT(txc != NULL && scb != NULL && key != NULL);

	priv = WLC_TXC_INFO_PRIV(txc);
	sti = SCB_TXC_INFO(txc, scb);

	wlc = priv->wlc;

	/* prepare pkt for tx - this updates the txh */
	WLPKTTAGSCBSET(p, scb);

	d11_off = sti->offset + D11_TXH_LEN_EX(wlc);
	txd = (wlc_txd_t *)((uint8 *)PKTDATA(wlc->osh, p) + sti->offset);
	BCM_REFERENCE(txd);
	PKTPULL(wlc->osh, p, d11_off);
	(void)wlc_key_prep_tx_mpdu(key, p, txd);

	PKTPUSH(wlc->osh, p, d11_off);
}

/* retrieve tx header from the cache and copy it to the packet.
 * return d11txh pointer in the packet where the copy started.
 */
d11txh_t * BCMFASTPATH
wlc_txc_cp(wlc_txc_info_t *txc, struct scb *scb, void *pkt, uint *flags)
{
	wlc_txc_info_priv_t *priv;
	wlc_info_t *wlc;
	scb_txc_info_t *sti;
	d11txh_t *txh;
	uint txhlen;
	uint8 *cp;

	ASSERT(txc->_txc == TRUE);

	ASSERT(scb != NULL);

	sti = SCB_TXC_INFO(txc, scb);
	ASSERT(sti != NULL);

	txhlen = sti->txhlen;

	priv = WLC_TXC_INFO_PRIV(txc);
	wlc = priv->wlc;
	(void)wlc;

	/* basic sanity check the tx cache */
	ASSERT(txhlen >= (D11_TXH_SHORT_LEN(wlc) + DOT11_A3_HDR_LEN) && txhlen < TXOFF);

	txh = SCB_TXC_TXH_NO_OFFST(sti);
	cp = PKTPUSH(wlc->osh, pkt, txhlen);
	bcopy((uint8 *)txh, cp, txhlen);

	/* return the saved pkttag flags */
	*flags = sti->flags;
	INCCNTUSED(sti);

	return (d11txh_t *)(cp + sti->offset);
}

/* check if we can find anything in the cache */
bool BCMFASTPATH
wlc_txc_hit(wlc_txc_info_t *txc, struct scb *scb, void *sdu, uint pktlen, uint fifo, uint8 prio)
{
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);
	wlc_info_t *wlc = priv->wlc;
	wlc_bsscfg_t *cfg;
	scb_txc_info_t *sti;
	uint miss;

	ASSERT(txc->_txc == TRUE);

	ASSERT(scb != NULL);

	sti = SCB_TXC_INFO(txc, scb);
	if (sti == NULL)
		return FALSE;

	cfg = SCB_BSSCFG(scb);
	BCM_REFERENCE(cfg);
	ASSERT(cfg != NULL);

	miss = (D11REV_LT(wlc->pub->corerev, 40) && (pktlen != sti->pktlen));
	if (miss)
		return FALSE;
	miss |= (sti->txhlen == 0);
	miss |= (fifo != sti->fifo);
	miss |= (prio != sti->prio);
	miss |= (sti->gen != priv->gen);
	miss |= (!SCB_CXO_ENABLED(scb) &&
		((sti->flags & WLPKTF_TOGGLE_MASK) != (WLPKTTAG(sdu)->flags & WLPKTF_TOGGLE_MASK)));
	miss |= ((pktlen >= wlc->fragthresh[WME_PRIO2AC(prio)]) &&
		((WLPKTTAG(sdu)->flags & WLF_AMSDU) == 0));
	if (BSSCFG_AP(cfg) || BSS_TDLS_ENAB(wlc, cfg)) {
		miss |= (sti->ps_on != SCB_PS(scb));
	}
	if (!PIO_ENAB(wlc->pub)) {
		/* calc the number of descriptors needed to queue this frame */
		uint ndesc = pktsegcnt(wlc->osh, sdu);
		if (BCM4331_CHIP_ID == CHIPID(wlc->pub->sih->chip))
			ndesc *= 2;
		miss |= (TXAVAIL(wlc, fifo) < ndesc);
	} else {
		miss |= !wlc_pio_txavailable(WLC_HW_PIO(wlc, fifo),
			(pktlen + D11_TXH_LEN_EX(wlc) + DOT11_A3_HDR_LEN), 1);
	}
#if defined(WLMSG_PRPKT)
	if (miss && WL_PRPKT_ON()) {
		printf("txc missed: scb %p gen %d txcgen %d hdr_len %d body len %d, pkt_len %d\n",
		        OSL_OBFUSCATE_BUF(scb), sti->gen, priv->gen,
				sti->txhlen, sti->pktlen, pktlen);
	}
#endif

	if (miss == 0) {
		INCCNTHIT(sti);
		return TRUE;
	}

	INCCNTMISSED(sti);
	return FALSE;
}

#ifdef WLCXO_CTRL
static void
wlc_txc_cxo_ctrl_tsc_init(wlc_info_t *wlc, void *p, struct scb *scb, scb_txc_info_t * sti)
{
	int32 ret;
	wlc_cx_tsc_t *tsc, *cx_tsc;
	struct dot11_header *h;
	wlc_cx_scb_msdu_t cx_scb_msdu;
	cx_wsec_iv_t txiv;
	wlc_key_t *key;
	wlc_key_info_t key_info;

	UNUSED_PARAMETER(ret);

	tsc = MALLOC(wlc->osh, sizeof(wlc_cx_tsc_t));
	if (tsc == NULL)
		return;

	bzero(tsc, sizeof(wlc_cx_tsc_t));
	tsc->scb = WLC_SCB_C2D(scb);

	/* Allocate a dummy virtual packet and copy the txhdr cache
	 * information.
	 */
	tsc->virt_p = PKTGET(wlc->osh, TXOFF + D11_COMMON_HDR, FALSE);
	if (tsc->virt_p != NULL) {
		wlc_cx_txc_t *cx_txc;
		d11actxh_t *txh;
		wlc_d11txh_cache_info_u d11_cache_info;

		/* Initialize txc of cache */
		cx_txc = &tsc->txc;
		cx_txc->txhlen = sti->txhlen;
		cx_txc->align = sti->align;
		cx_txc->offset = sti->offset;
		cx_txc->fifo = sti->fifo;
		cx_txc->ac = WME_PRIO2AC(PKTPRIO(p));
		cx_txc->flags = sti->flags;
		bcopy(sti->txh, cx_txc->txh, sizeof(cx_txc->txh));

		bcopy(sti->txh + sti->align, PKTDATA(wlc->osh, tsc->virt_p), sti->txhlen);
		txh = (d11actxh_t *)(cx_txc->txh + cx_txc->align + cx_txc->offset);

		d11_cache_info.d11actxh_CacheInfo =
			WLC_TXD_CACHE_INFO_GET(txh, wlc->pub->corerev);

		/* Fill per cache info */
		wlc_ampdu_fill_percache_info(wlc->ampdu_tx, scb, PKTPRIO(p), d11_cache_info);


		wlc_cxo_ctrl_tsc_init(wlc, scb, PKTPRIO(p), scb->bsscfg, tsc);
	} else {
		MFREE(wlc->osh, tsc, sizeof(wlc_cx_tsc_t));
		return;
	}

	/* Unique ID for this snapshot of TSC */
	tsc->tsc_gen = wlc_txc_get_gen_info(wlc->txc);

	tsc->wlcif_idx = scb->bsscfg->wlcif->index;

#ifdef WLTOEHW
	if (!wlc->toe_bypass) {
		tsc->tso_hlen = wlc_tso_hdr_length((d11ac_tso_t *)PKTDATA(wlc->osh, p));
	}
#endif

	h = (struct dot11_header *)(PKTDATA(wlc->osh, p) + sti->offset + D11_TXH_LEN_EX(wlc));

	if (SCB_A4_DATA(scb)) {
		/* WDS */
		eacopy(&h->a3, &tsc->da);
		eacopy(&h->a4, &tsc->sa);
	} else if (BSSCFG_STA(scb->bsscfg)) {
		/* STA to AP */
		eacopy(&h->a3, &tsc->da);
		eacopy(&h->a2, &tsc->sa);
	} else if (BSSCFG_AP(scb->bsscfg)) {
		/* AP to STA */
		eacopy(&h->a1, &tsc->da);
		eacopy(&h->a3, &tsc->sa);
	} else
		ASSERT(0);

	WL_CXO(("%s: TSC size %d scb_msdu %d scb_ampdu %d\n", __FUNCTION__,
	        (int)sizeof(wlc_cx_tsc_t), (int)OFFSETOF(wlc_cx_tsc_t, scb_msdu),
	        (int)OFFSETOF(wlc_cx_tsc_t, scb_ampdu)));

	WL_CXO(("%s: Adding TSC entry, bssidx %d aid %d tid %d\n",
		__FUNCTION__, WLC_BSSCFG_IDX(scb->bsscfg), scb->aid, PKTPRIO(p)));

	/* Add TSC entry */
	tsc->scb_ctrl = scb;

	bzero(&txiv, sizeof(txiv));
	key = wlc_keymgmt_get_scb_key(wlc->keymgmt, scb,
		WLC_KEY_ID_PAIRWISE, WLC_KEY_FLAG_NONE, &key_info);
	if ((key != NULL) && (key_info.algo == CRYPTO_ALGO_AES_CCM)) {
		if ((ret = wlc_key_get_seq(key, (uint8 *)&txiv, sizeof(txiv), 0, TRUE)) < 0) {
			WL_ERROR(("%s: Error getting key seq: %d", __FUNCTION__, ret));
		}
	}

	ret = __wlc_cxo_c2d_tsc_add(wlc->ipc, wlc, tsc, &cx_tsc, &txiv);
	ASSERT(ret == BCME_OK);
	tsc->cx_tsc = cx_tsc;
#ifdef WLCXO_IPC
	wlc_cxo_ctrl_tsc_add(wlc, tsc);
#endif
	if (wlc_cxo_ctrl_tsc_ini_add(wlc, scb, PKTPRIO(p)) != BCME_OK) {
		MFREE(wlc->osh, tsc, sizeof(wlc_cx_tsc_t));
		return;
	}


	/* Add txpq for current priority */
	cx_scb_msdu.release = 4;
	cx_scb_msdu.max_release = 64;
	__wlc_cxo_c2d_tsc_add_txpq(wlc->ipc, wlc, scb->bsscfg->_idx, scb->aid, PKTPRIO(p),
	                       &cx_scb_msdu);

	MFREE(wlc->osh, tsc, sizeof(wlc_cx_tsc_t));
}

/* Add TSC entry for STA. Only cacheable STAs are considered for
 * addition. TSC entry is populated with AMPDU and AMSDU information
 * if it is capable.
 */
static void BCMFASTPATH
wlc_txc_cxo_ctrl_tsc_add(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb, void *pkt,
	scb_txc_info_t * sti)
{
	wlc_cx_tsc_t *tsc;
	wlc_cx_txc_t *cx_txc;
	wlc_cx_scb_msdu_t cx_scb_msdu;
	d11actxh_t *txh;
	wlc_d11txh_cache_info_u d11_cache_info;

	ASSERT(WLCXO_ENAB(wlc->pub) && SCB_CXO_ENABLED(scb));

#ifdef WLCXO_IPC
	tsc = wlc_cxo_ctrl_tsc_find_by_aid(wlc, cfg->_idx, scb->aid);
#else
	tsc = wlc_cxo_tsc_find_by_aid(wlc->wlc_cx, cfg->_idx, scb->aid);
#endif
	if (tsc == NULL) {
		WL_CXO(("%s: Adding TSC, pkt prio:%d\n", __FUNCTION__, PKTPRIO(pkt)));
		wlc_txc_cxo_ctrl_tsc_init(wlc, pkt, scb, sti);
		return;
	}

	/* Update cached header in tsc */
	cx_txc = &tsc->txc;

	cx_txc->flags = sti->flags;
	bcopy(sti->txh + sti->align, cx_txc->txh + cx_txc->align,
	      cx_txc->txhlen);

	/* Add ini at ofld if not already */
	if (wlc_cxo_ctrl_tsc_ini_add(wlc, scb, PKTPRIO(pkt)) != BCME_OK)
		return;
	txh = (d11actxh_t *)(cx_txc->txh + cx_txc->align + cx_txc->offset);

	d11_cache_info.d11actxh_CacheInfo =
		WLC_TXD_CACHE_INFO_GET(txh, wlc->pub->corerev);

	/* Fill per cache info */
	wlc_ampdu_fill_percache_info(wlc->ampdu_tx, scb, PKTPRIO(pkt), d11_cache_info);


	WL_CXO(("%s: Updating txc contents of TSC, pkt prio:%d\n",
		__FUNCTION__, PKTPRIO(pkt)));
	__wlc_cxo_c2d_tsc_update_txc(wlc->ipc, wlc, cfg->_idx,
			scb->aid, WLCNTSCBVAL(scb->scb_stats.tx_rate), cx_txc, TRUE);

	/* Add txpq for current priority */
	cx_scb_msdu.release = 4;
	cx_scb_msdu.max_release = 64;
	__wlc_cxo_c2d_tsc_add_txpq(wlc->ipc, wlc, cfg->_idx,
	                       scb->aid, PKTPRIO(pkt), &cx_scb_msdu);
}
#endif /* WLCXO_CTRL */

/* install tx header from the packet into the cache */
void BCMFASTPATH
wlc_txc_add(wlc_txc_info_t *txc, struct scb *scb, void *pkt, uint txhlen, uint fifo, uint8 prio,
	uint16 txh_off, uint d11hdr_len)
{
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);
	wlc_info_t *wlc = priv->wlc;
	wlc_bsscfg_t *cfg;
	scb_txc_info_t *sti;
	d11txh_t *txh;
	osl_t *osh;
#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
	uint8 ucidx;
	d11actxh_t* vhtHdr;
#endif

	BCM_REFERENCE(d11hdr_len);

	ASSERT(txc->_txc == TRUE);
	ASSERT(scb != NULL);

	sti = SCB_TXC_INFO(txc, scb);
	if (sti == NULL)
		return;

	cfg = SCB_BSSCFG(scb);
	BCM_REFERENCE(cfg);
	ASSERT(cfg != NULL);

	osh = wlc->osh;

	/* cache any pkt with same DA-SA-L(before LLC/SNAP), hdr must be in one buffer */

	sti->align = (uint8)((uintptr)PKTDATA(osh, pkt) & 3);
	ASSERT((sti->align == 0) || (sti->align == 2));
	sti->offset = txh_off;

#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
	if (D11REV_GE(wlc->pub->corerev, 42)) {
		if (priv->ucache_entry_inuse[sti->ucache_idx] != sti) {
			ucidx = wlc_txc_get_free_ucode_cidx(txc);
			if (ucidx >= WLC_MAX_UCODE_CACHE_ENTRIES)
				return;
			sti->ucache_idx = ucidx;
			priv->ucache_entry_inuse[ucidx] = sti;
		} else {
			ASSERT(sti->ucache_idx < WLC_MAX_UCODE_CACHE_ENTRIES);
		}
		{
		d11actxh_pkt_t *short_hdr;
		d11ac_tso_t *tsoHdr;
		/*
		* For uCode based TXD cache, txc->txh contain cached header data
		* in the following format: This is to avoid second copy for mac header
		* when there is  cache hit.
		* | d11actxh_pkt_t | MAC Header of length 'd11hdr_len' |
		*  d11actxh_rate_t | d11actxh_cache_t|
		*/
		/* Copy tshoHeader and shot TXD */
		bcopy(PKTDATA(osh, pkt), sti->txh + sti->align,  sti->offset + D11AC_TXH_SHORT_LEN);
		/* Copy mac header parameters. */
		bcopy(PKTDATA(osh, pkt) + D11AC_TXH_LEN + sti->offset,
			sti->txh + sti->align + sti->offset + D11AC_TXH_SHORT_LEN,
			d11hdr_len);
		/* At the end copy the remaining long TXD */
		 bcopy(PKTDATA(osh, pkt) + D11AC_TXH_SHORT_LEN + sti->offset,
		 sti->txh + sti->align + D11AC_TXH_SHORT_LEN + d11hdr_len +
		 sti->offset, D11AC_TXH_LEN - D11AC_TXH_SHORT_LEN);

		tsoHdr = (d11ac_tso_t*)(sti->txh + sti->align);
		/* Update tsoHdr short descriptor */
		short_hdr = (d11actxh_pkt_t *) (sti->txh + sti->align + sti->offset);
		tsoHdr->flag[2] |= TOE_F2_TXD_HEAD_SHORT;
		short_hdr->MacTxControlLow |= htol16(D11AC_TXC_HDR_FMT_SHORT |((sti->ucache_idx <<
			D11AC_TXC_CACHE_IDX_SHIFT) & D11AC_TXC_CACHE_IDX_MASK));
		short_hdr->MacTxControlLow &= htol16(~D11AC_TXC_UPD_CACHE);
		sti->txhlen = D11AC_TXH_SHORT_LEN + sti->offset + d11hdr_len;
		short_hdr->PktCacheLen = 0;
		short_hdr->TxStatus = 0;
		short_hdr->Tstamp = 0;


		wlc_pkt_get_vht_hdr(wlc, pkt, &vhtHdr);
		vhtHdr->PktInfo.MacTxControlLow |=
			htol16(D11AC_TXC_UPD_CACHE |((sti->ucache_idx <<
			D11AC_TXC_CACHE_IDX_SHIFT) & D11AC_TXC_CACHE_IDX_MASK));

		}
	} else {
		sti->txhlen = txhlen;
		bcopy(PKTDATA(osh, pkt), sti->txh + sti->align, txhlen);
	}
#else
	sti->txhlen = txhlen;
	bcopy(PKTDATA(osh, pkt), sti->txh + sti->align, txhlen);
#endif /* defined(WLC_UCODE_CACHE) && D11CONF_GE(42) */


	txh = SCB_TXC_TXH(sti);
	sti->pktlen = pkttotlen(osh, pkt) - txhlen + ETHER_HDR_LEN;
	sti->fifo = fifo;
	sti->prio = prio;
	sti->gen = priv->gen;

	if (BSSCFG_AP(cfg) || BSS_TDLS_ENAB(wlc, cfg)) {
		sti->ps_on = SCB_PS(scb);
	}
	/* need MPDU flag to avoid going back to prep_sdu */
	sti->flags = (WLF_MPDU | WLF_TXHDR);
	sti->flags |= (WLPKTTAG(pkt)->flags & WLPKTF_STORE_MASK);

	/* Clear pkt exptime from the txheader cache */
	wlc_txh_set_aging(wlc, txh, FALSE);

#if defined(WLMSG_PRPKT)
	if (WL_PRPKT_ON()) {
		printf("install txc: scb %p gen %d hdr_len %d, body len %d\n",
		       OSL_OBFUSCATE_BUF(scb), sti->gen, sti->txhlen, sti->pktlen);
	}
#endif

	INCCNTADDED(sti);

#ifdef WLCXO_CTRL
	if (SCB_CXO_ENABLED(scb)) {
		wlc_txc_cxo_ctrl_tsc_add(wlc, cfg, scb, pkt, sti);
	}
#endif /* WLCXO_CTRL */
}

#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
static uint8
wlc_txc_get_free_ucode_cidx(wlc_txc_info_t *txc)
{
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);
	uint8 i;

	for (i = 0; i < WLC_MAX_UCODE_CACHE_ENTRIES; i++) {
		if (!priv->ucache_entry_inuse[i]) {
			return i;
		}
	}
	wlc_txc_inv_all(txc);
	/* Reset all entries as they use new entry.. */
	for (i = 0; i < WLC_MAX_UCODE_CACHE_ENTRIES; i++) {
		priv->ucache_entry_inuse[i] = NULL;
	}
	/* use ZEROth entry for this txc */
	return 0;
}
#endif /* defined(WLC_UCODE_CACHE)&& D11CONF_GE(42) */

/* update tx header cache enable flag (txc->_txc) */
void
wlc_txc_upd(wlc_txc_info_t *txc)
{
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);
	wlc_info_t *wlc = priv->wlc;

	BCM_REFERENCE(wlc);

	txc->_txc = (priv->policy == ON) ? TRUE : FALSE;

#ifdef WLCXO_CTRL
	/* Update TSC for each associated STAs. When called from IOVAR
	 * dispatcher/handler function, this might be redundant for
	 * most IOVARs but chose to update the cache with out checking
	 * for specific IOVAR or BSS or SCB to keep the implementation
	 * simple. Since this is not expected to be a frequent case there
	 * shouldn't be any thru'put impact.
	 */
	if (WLCXO_ENAB(wlc->pub) && txc->_txc && (wlc->scbstate != NULL)) {
		struct scb *scb;
		struct scb_iter scbiter;
		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			wlc_cxo_ctrl_tsc_update_txc(scb, FALSE);
		}
	}
#endif
}

/* scb cubby */
static int
wlc_txc_scb_init(void *ctx, struct scb *scb)
{
	wlc_txc_info_t *txc = (wlc_txc_info_t *)ctx;
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);
	wlc_info_t *wlc = priv->wlc;
	scb_txc_info_t **psti = SCB_TXC_CUBBY_LOC(txc, scb);
	scb_txc_info_t *sti;

	if (SCB_INTERNAL(scb) || !WLC_TXC_ENAB(wlc)) {
		WL_INFORM(("%s: Not allocating the cubby, txc enab %d\n",
		           __FUNCTION__, WLC_TXC_ENAB(wlc)));
		return BCME_OK;
	}

	if ((sti = MALLOCZ(wlc->osh, sizeof(scb_txc_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: failed to allocate cubby space for txheader cache\n",
		          wlc->pub->unit, __FUNCTION__));
		return BCME_NOMEM;
	}
	*psti = sti;

	return BCME_OK;
}

static void
wlc_txc_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_txc_info_t *txc = (wlc_txc_info_t *)ctx;
	wlc_txc_info_priv_t *priv;
	wlc_info_t *wlc;
	scb_txc_info_t **psti = SCB_TXC_CUBBY_LOC(txc, scb);
	scb_txc_info_t *sti = *psti;

	if (sti == NULL)
		return;

	priv = WLC_TXC_INFO_PRIV(txc);
	wlc = priv->wlc;
#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
	if (D11REV_GE(wlc->pub->corerev, 42)) {
		priv->ucache_entry_inuse[sti->ucache_idx] = NULL;
	}
#endif /* defined(WLC_UCODE_CACHE) && D11CONF_GE(42) */
	MFREE(wlc->osh, sti, sizeof(scb_txc_info_t));
	*psti = NULL;
}


/* invalidate the cache entry */
void
wlc_txc_inv(wlc_txc_info_t *txc, struct scb *scb)
{
	scb_txc_info_t *sti;

	ASSERT(scb != NULL);

	sti = SCB_TXC_INFO(txc, scb);
	if (sti == NULL)
		return;

#ifdef WLCXO_CTRL
	if (WLCXO_ENAB(SCB_WLC(scb)->pub)) {
		wlc_cxo_ctrl_tsc_update_txc(scb, TRUE);
	}
#endif

	sti->txhlen = 0;
}

/* invalidate all cache entries */
void
wlc_txc_inv_all(wlc_txc_info_t *txc)
{
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);

	priv->gen++;
}

uint
wlc_txc_get_gen_info(wlc_txc_info_t *txc)
{
	wlc_txc_info_priv_t *priv = WLC_TXC_INFO_PRIV(txc);
	return priv->gen;
}
