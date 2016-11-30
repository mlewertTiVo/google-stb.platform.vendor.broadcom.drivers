/*
 * 802.11 frame fragmentation handling module
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
 * $Id: wlc_frag.c 626490 2016-03-22 00:18:23Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_frmutil.h>
#include <wlc_frag.h>

/* module info */
struct wlc_frag_info {
	wlc_info_t *wlc;
	int scbh;
};

/* scb cubby */
typedef struct scb_frag {
	void	*fragbuf[NUMPRIO];	/**< defragmentation buffer per prio */
	uint16	fragresid[NUMPRIO];	/**< #bytes unused in frag buffer per prio */
} scb_frag_t;

/* handy macros to access the scb cubby and the frag states */
#define SCB_FRAG_LOC(frag, scb) (scb_frag_t **)SCB_CUBBY(scb, (frag)->scbh)
#define SCB_FRAG(frag, scb) *SCB_FRAG_LOC(frag, scb)

/* local decalarations */
static int wlc_frag_scb_init(void *ctx, struct scb *scb);
static void wlc_frag_scb_deinit(void *ctx, struct scb *scb);
static uint wlc_frag_scb_secsz(void *ctx, struct scb *scb);
#define wlc_frag_scb_dump NULL

static void wlc_defrag_prog_cleanup(wlc_frag_info_t *frag, struct scb *scb,
	uint8 prio);

static void wlc_lastfrag(wlc_info_t *wlc, uint8 prio, struct wlc_frminfo *f);
static void wlc_appendfrag(wlc_info_t *wlc, void *fragbuf, uint16 *fragresid,
	uint8 *body, uint body_len);

/* module attach/detach interfaces */
wlc_frag_info_t *
BCMATTACHFN(wlc_frag_attach)(wlc_info_t *wlc)
{
	scb_cubby_params_t cubby_params;
	wlc_frag_info_t *frag;

	if ((frag = MALLOCZ(wlc->osh, sizeof(*frag))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	frag->wlc = wlc;

	/* reserve some space in scb container for frag states */
	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = frag;
	cubby_params.fn_init = wlc_frag_scb_init;
	cubby_params.fn_deinit = wlc_frag_scb_deinit;
	cubby_params.fn_dump = wlc_frag_scb_dump;
	cubby_params.fn_secsz = wlc_frag_scb_secsz;

	frag->scbh = wlc_scb_cubby_reserve_ext(wlc, sizeof(scb_frag_t *), &cubby_params);

	if (frag->scbh < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve_ext failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	return frag;

fail:
	MODULE_DETACH(frag, wlc_frag_detach);
	return NULL;
}

void
BCMATTACHFN(wlc_frag_detach)(wlc_frag_info_t *frag)
{
	wlc_info_t *wlc;

	if (frag == NULL)
		return;

	wlc = frag->wlc;
	BCM_REFERENCE(wlc);
	MFREE(wlc->osh, frag, sizeof(*frag));
}

/* scb cubby interfaces */
static int
wlc_frag_scb_init(void *ctx, struct scb *scb)
{
	wlc_frag_info_t *frag = (wlc_frag_info_t *)ctx;
	wlc_info_t *wlc = frag->wlc;
	scb_frag_t **pscb_frag = SCB_FRAG_LOC(frag, scb);
	scb_frag_t *scb_frag = SCB_FRAG(frag, scb);

	BCM_REFERENCE(wlc);

	ASSERT(scb_frag == NULL);

	if (SCB_INTERNAL(scb))
		return BCME_OK;

	if ((scb_frag = wlc_scb_sec_cubby_alloc(wlc, scb, sizeof(*scb_frag))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}
	*pscb_frag = scb_frag;

	return BCME_OK;
}

static void
wlc_frag_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_frag_info_t *frag = (wlc_frag_info_t *)ctx;
	wlc_info_t *wlc = frag->wlc;
	scb_frag_t **pscb_frag = SCB_FRAG_LOC(frag, scb);
	scb_frag_t *scb_frag = SCB_FRAG(frag, scb);
	uint8 prio;

	BCM_REFERENCE(wlc);

	/* free any frame reassembly buffer */
	for (prio = 0; prio < NUMPRIO; prio++) {
		wlc_defrag_prog_reset(frag, scb, prio, 0);
	}

	if (scb_frag != NULL) {
		wlc_scb_sec_cubby_free(wlc, scb, scb_frag);
	}
	*pscb_frag = NULL;
}

static uint
wlc_frag_scb_secsz(void *ctx, struct scb *scb)
{
	if (SCB_INTERNAL(scb))
		return 0;
	return sizeof(scb_frag_t);
}


/* rx defragmentation interfaces */

/* reset the state machine */
void
wlc_defrag_prog_reset(wlc_frag_info_t *frag, struct scb *scb, uint8 prio,
	uint16 prev_seqctl)
{
	wlc_info_t *wlc = frag->wlc;
	scb_frag_t *scb_frag = SCB_FRAG(frag, scb);

	BCM_REFERENCE(wlc);

	if (scb_frag != NULL &&
	    scb_frag->fragbuf[prio] != NULL) {
		WL_ERROR(("wl%d: %s: discarding partial "
		          "MSDU %03x with prio %d received from %s\n",
		          wlc->pub->unit, __FUNCTION__,
		          prev_seqctl >> SEQNUM_SHIFT, prio,
		          bcm_ether_ntoa(&scb->ea, eabuf)));
		PKTFREE(wlc->osh, scb_frag->fragbuf[prio], FALSE);
		wlc_defrag_prog_cleanup(frag, scb, prio);
	}
}

/* process the first frag.
 * return TRUE when success FALSE otherwise
 */
bool
wlc_defrag_first_frag_proc(wlc_frag_info_t *frag, struct scb *scb, uint8 prio,
	struct wlc_frminfo *f)
{
	wlc_info_t *wlc = frag->wlc;
	osl_t *osh = wlc->osh;
	uint rxbufsz = wlc->pub->tunables->rxbufsz;
	scb_frag_t *scb_frag = SCB_FRAG(frag, scb);
	void *p = f->p;

	/* map the contents of 1st frag */
	PKTCTFMAP(osh, p);

	/* save packet for reassembly */
	scb_frag->fragbuf[prio] = p;

	if (((PKTTAILROOM(osh, p) + PKTHEADROOM(osh, p) + PKTLEN(osh, p))) < rxbufsz) {
		scb_frag->fragbuf[prio] = PKTGET(osh, rxbufsz, FALSE);
		if (scb_frag->fragbuf[prio] == NULL) {
			WL_ERROR(("wl%d: %s(): Allocate %d rxbuf for"
			          " frag pkt failed!\n",
			          wlc->pub->unit, __FUNCTION__,
			          rxbufsz));
			goto toss;
		}
		memcpy(PKTDATA(osh, scb_frag->fragbuf[prio]),
		       (PKTDATA(osh, p) - PKTHEADROOM(osh, p)),
		       (PKTHEADROOM(osh, p) + PKTLEN(osh, p)));
		PKTPULL(osh, scb_frag->fragbuf[prio],
		        (int)PKTHEADROOM(osh, p));
		PKTSETLEN(osh, scb_frag->fragbuf[prio], PKTLEN(osh, p));

		PKTFREE(osh, p, FALSE);
	}

	scb_frag->fragresid[prio] =
	        (uint16)(rxbufsz - wlc->hwrxoff - D11_PHY_HDR_LEN - f->len);

	scb->flags |= SCB_DEFRAG_INPROG;

	return TRUE;

toss:
	return FALSE;
}

/* process the remaining frags
 * return TRUE when success FALSE otherwise
 */
bool
wlc_defrag_other_frag_proc(wlc_frag_info_t *frag, struct scb *scb, uint8 prio,
	struct wlc_frminfo *f, uint16 prev_seqctl, bool more_frag, bool seq_chk_only)
{
	wlc_info_t *wlc = frag->wlc;
	osl_t *osh = wlc->osh;
	scb_frag_t *scb_frag = SCB_FRAG(frag, scb);
	void *p = f->p;

	BCM_REFERENCE(wlc);

	/* Make sure this MPDU:
	 * - matches the partially-received MSDU
	 * - is the one we expect (next in sequence)
	 */
	if (((f->seq & ~FRAGNUM_MASK) != (prev_seqctl & ~FRAGNUM_MASK)) ||
	    ((f->seq & FRAGNUM_MASK) != ((prev_seqctl & FRAGNUM_MASK) + 1))) {
		/* discard the partially-received MSDU */
		wlc_defrag_prog_reset(frag, scb, prio, prev_seqctl);

		/* discard the MPDU */
		WL_INFORM(("wl%d: %s: discarding MPDU %04x with "
		           "prio %d; previous MPDUs missed\n",
		           wlc->pub->unit, __FUNCTION__,
		           f->seq, prio));
		goto toss;
	}

	if (!seq_chk_only) {

		/*
		 * This isn't the first frag, but we don't have a partially-
		 * received MSDU.  We must have somehow missed the previous
		 * frags or timed-out the partially-received MSDU (not implemented yet).
		 */

		if (scb_frag->fragbuf[prio] == NULL) {
			WL_ERROR(("wl%d: %s: discarding MPDU %04x with "
			          "prio %d; previous MPDUs missed or partially-received "
			          "MSDU timed-out\n", wlc->pub->unit, __FUNCTION__,
			          f->seq, prio));
			goto toss;
		}

		/* detect fragbuf overflow */
		if (f->body_len > scb_frag->fragresid[prio]) {
			/* discard the partially-received MSDU */
			wlc_defrag_prog_reset(frag, scb, prio, prev_seqctl);

			/* discard the MPDU */
			WL_ERROR(("wl%d: %s: discarding MPDU %04x "
			          "with prio %d; resulting MSDU too big\n",
			          wlc->pub->unit, __FUNCTION__,
			          f->seq, prio));
			goto toss;
		}

		/* map the contents of each subsequent frag before copying */
		PKTCTFMAP(osh, p);

		/* copy frame into fragbuf */
		wlc_appendfrag(wlc, scb_frag->fragbuf[prio], &scb_frag->fragresid[prio],
		               f->pbody, f->body_len);

		PKTFREE(osh, p, FALSE);

		/* last frag. */
		if (!more_frag) {
			f->p = scb_frag->fragbuf[prio];

			wlc_defrag_prog_cleanup(frag, scb, prio);

			wlc_lastfrag(wlc, prio, f);
		}
	}

	return TRUE;

toss:
	WLCNTINCR(wlc->pub->_cnt->rxfragerr);
	WLCIFCNTINCR(scb, rxfragerr);
	return FALSE;
}

static void
wlc_defrag_prog_cleanup(wlc_frag_info_t *frag, struct scb *scb, uint8 prio)
{
	scb_frag_t *scb_frag = SCB_FRAG(frag, scb);

	/* clear the packet pointer */
	scb_frag->fragbuf[prio] = NULL;
	scb_frag->fragresid[prio] = 0;

	/* clear the scb flag - note it's a summary flag */
	for (prio = 0; prio < NUMPRIO; prio++) {
		if (scb_frag->fragbuf[prio] != NULL)
			return;
	}
	scb->flags &= ~SCB_DEFRAG_INPROG;
}

static void
wlc_lastfrag(wlc_info_t *wlc, uint8 prio, struct wlc_frminfo *f)
{
	osl_t *osh = wlc->osh;


	/* reset packet pointers to beginning */
	f->h = (struct dot11_header *)PKTDATA(osh, f->p);
	f->len = PKTLEN(osh, f->p);
	f->pbody = (uchar*)(f->h) + DOT11_A3_HDR_LEN;
	f->body_len = f->len - DOT11_A3_HDR_LEN;
	if (f->wds) {
		f->pbody += ETHER_ADDR_LEN;
		f->body_len -= ETHER_ADDR_LEN;
	}
	/* WME: account for QoS Control Field */
	if (f->qos) {
		f->prio = prio;
		f->pbody += DOT11_QOS_LEN;
		f->body_len -= DOT11_QOS_LEN;
	}
	f->fc = ltoh16(f->h->fc);
	if (f->rx_wep && f->key) {
		f->pbody += f->key_info.iv_len;
		f->body_len -= f->key_info.iv_len;
	}
}

static void
wlc_appendfrag(wlc_info_t *wlc, void *fragbuf, uint16 *fragresid,
               uint8 *body, uint body_len)
{
	osl_t *osh = wlc->osh;
	uint8 *dst;
	uint fraglen;

	/* append frag payload to end of partial packet */
	fraglen = PKTLEN(osh, fragbuf);
	dst = PKTDATA(osh, fragbuf) + fraglen;
	bcopy(body, dst, body_len);
	PKTSETLEN(osh, fragbuf, (fraglen + body_len));
	*fragresid -= (uint16)body_len;
}
