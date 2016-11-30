/*
 * Routines related to the uses of Pktfetch in WLC
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
 * $Id: wlc_pktfetch.c 645630 2016-06-24 23:27:55Z $
 */


/**
 * There are use cases which require the complete packet to be consumed in-dongle, and thus
 * neccessitates a mechanism (PktFetch) that can DMA down data blocks from the Host memory to dongle
 * memory, and thus recreate the full ethernet packet. Example use cases include:
 *     EAPOL frames
 *     SW TKIP
 *     WAPI
 *
 * This is a generic module which takes a Host address, block size and a pre-specified local buffer
 * (lbuf) and guarantees that the Host block will be fetched into device memory.
 */

#include <wlc_types.h>
#include <bcmwpa.h>
#include <hnd_cplt.h>
#include <phy_utils_api.h>
#include <wlc.h>
#include <wlc_pub.h>
#include <wlc_pktfetch.h>
#ifdef WLAMPDU
#include <wlc_scb.h>
#endif
#include <wlc_ampdu_rx.h>
#include <wlc_keymgmt.h>
#include <wlc_bsscfg.h>
#include <d11_cfg.h>

#ifdef BCMSPLITRX
#include <rte_pktfetch.h>
#include <wlc_rx.h>

static void wlc_recvdata_pktfetch_cb(void *lbuf, void *orig_lfrag,
	void *ctxt, bool cancelled);
#if defined(PKTC) || defined(PKTC_DONGLE)
static void wlc_sendup_pktfetch_cb(void *lbuf, void *orig_lfrag,
	void *ctxt, bool cancelled);
#endif
static void wlc_recreate_frameinfo(wlc_info_t *wlc, void *lbuf, void *lfrag,
	wlc_frminfo_t *fold, wlc_frminfo_t *fnew);

bool
wlc_pktfetch_required(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct wlc_frminfo *f,	wlc_key_info_t *key_info, bool skip_iv)
{
	struct dot11_llc_snap_header *lsh;
	uint16* ether_type = NULL;
	/* In  MODE-4  need to check for  HdrType of conversion( Type1/Type2) to fetch correct
	 *    EtherType
	 */
	if (SPLITRX_DYN_MODE4(wlc->osh, f->p)) {
		if ((f->wrxh->rxhdr.lt80.HdrConvSt) & ETHER_HDR_CONV_TYPE) {
			ether_type = (unsigned short*)(f->pbody + ETHER_TYPE_2_OFFSET);
		}
		else {
			ether_type = (unsigned short*)(f->pbody + ETHER_TYPE_1_OFFSET);
		}
		if ((ntoh16(*ether_type)) == ETHER_TYPE_802_1X)
		return TRUE;
	}
	/* If IV needs to be accounted for, move past IV len */
	lsh = (struct dot11_llc_snap_header *)(f->pbody + (skip_iv ? key_info->iv_len : 0));

	/* For AMSDU packets packet starts after subframe header */
	if (f->isamsdu)
		lsh = (struct dot11_llc_snap_header*)((char *)lsh + ETHER_HDR_LEN);

	if ((PKTFRAGUSEDLEN(wlc->osh, f->p) > 0) &&
		LLC_SNAP_HEADER_CHECK(lsh) &&
		(EAPOL_PKTFETCH_REQUIRED(lsh) ||
#ifdef WLTDLS
		WLTDLS_PKTFETCH_REQUIRED(wlc, lsh) ||
#endif
#ifdef WLNDOE
		NDOE_PKTFETCH_REQUIRED(wlc, lsh, f->pbody, f->body_len) ||
#endif
#ifdef WOWLPF
		WOWLPF_ACTIVE(wlc->pub) ||
#endif
#ifdef BDO
		BDO_PKTFETCH_REQUIRED(wlc, lsh) ||
#endif
#ifdef WL_TBOW
		WLTBOW_PKTFETCH_REQUIRED(wlc, bsscfg, ntoh16(lsh->type)) ||
#endif
#ifdef TKO
		TKO_PKTFETCH_REQUIRED(wlc, lsh) ||
#endif
		FALSE))
		return TRUE;

	return FALSE;

}

int
wlc_recvdata_schedule_pktfetch(wlc_info_t *wlc, struct scb *scb,
	wlc_frminfo_t *f, bool promisc_frame, bool ordered, bool amsdu_msdus)
{
	wlc_eapol_pktfetch_ctx_t *ctx = NULL;
	struct pktfetch_info *pinfo = NULL;
	int ret = BCME_OK;
	uint32 copycount = (wlc->pub->tunables->copycount << 2);

	pinfo = MALLOCZ(wlc->osh, sizeof(struct pktfetch_info));
	if (!pinfo) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			(int)sizeof(struct pktfetch_info), MALLOCED(wlc->osh)));
		ret = BCME_NOMEM;
		goto error;
	}

	ctx = MALLOCZ(wlc->osh, sizeof(wlc_eapol_pktfetch_ctx_t));
	if (!ctx) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			(int)sizeof(wlc_eapol_pktfetch_ctx_t), MALLOCED(wlc->osh)));
		ret = BCME_NOMEM;
		goto error;
	}

	ctx->wlc = wlc;

	bcopy(f, &ctx->f, sizeof(wlc_frminfo_t));
#ifdef WLAMPDU
	if ((f->subtype == FC_SUBTYPE_QOS_DATA) &&
		SCB_AMPDU(scb) && wlc_scb_ampdurx_on_tid(scb, f->prio) &&
		!f->ismulti && !promisc_frame) {
		ctx->ampdu_path = TRUE;
	}
#endif
	ctx->ordered = ordered;
	ctx->promisc = promisc_frame;
	ctx->pinfo = pinfo;
	ctx->scb_assoctime = scb->assoctime;
	memcpy(&ctx->ea, &scb->ea,  sizeof(struct ether_addr));

	/* Headroom does not need to be > PKTRXFRAGSZ */
	pinfo->osh = wlc->osh;
	pinfo->headroom = PKTRXFRAGSZ;

	/* In case of RXMODE 3, host has full pkt including RxStatus (WL_HWRXOFF_AC bytes)
	 * Host buffer addresses in the lfrag points to start of this full pkt
	 * So, pktfetch host_offset needs to be set to (copycount + HW_RXOFF)
	 * so that only necessary (remaining) data if pktfetched from Host.
	*/
	if (SPLITRX_DYN_MODE3(wlc->osh, f->p)) {
		pinfo->host_offset = copycount + wlc->hwrxoff;
		PKTSETFRAGTOTLEN(wlc->osh, f->p,
			PKTFRAGTOTLEN(wlc->osh, f->p) + copycount + wlc->hwrxoff);
	}
	/* In RXMODE4 host has full 802.3 pkt starting from WL_HWRXOFF_AC.
	*  So to recreate orginal pkt dongle has to get the full payload excluding
	* WL_HWRXOFF_AC + Erhernet Header, as that part will be present in lfrag
	*/
	else if (SPLITRX_DYN_MODE4(wlc->osh, f->p)) {
		if ((f->wrxh->rxhdr.lt80.HdrConvSt) & ETHER_HDR_CONV_TYPE) {
			/* This is a type-2 conversion so hostoffset should be
			*  40+ Ethernet Hdr
			*/
			pinfo->host_offset = (wlc->hwrxoff + DOT3HDR_OFFSET);
			PKTSETFRAGTOTLEN(wlc->osh, f->p, PKTFRAGTOTLEN(wlc->osh, f->p) +
					wlc->hwrxoff + DOT3HDR_OFFSET);
		}
		else {
			/* This is Type-1 conversion so hostoffset should be
			* 40+ Ethernet Hdr + LLC hdr
			*/
			pinfo->host_offset = (wlc->hwrxoff + DOT3HDR_OFFSET + LLC_HDR_LEN);
			PKTSETFRAGTOTLEN(wlc->osh, f->p, PKTFRAGTOTLEN(wlc->osh, f->p) +
					wlc->hwrxoff + DOT3HDR_OFFSET + LLC_HDR_LEN);
		}
	}
	else
		pinfo->host_offset = 0;

	pinfo->lfrag = f->p;
	pinfo->ctx = ctx;
	pinfo->cb = wlc_recvdata_pktfetch_cb;
	ctx->flags = 0;
	if (amsdu_msdus)
		ctx->flags = PKTFETCH_FLAG_AMSDU_SUBMSDUS;

#ifdef BCMPCIEDEV
	if (BCMPCIEDEV_ENAB()) {
		ret = hnd_pktfetch(pinfo);
		if (ret != BCME_OK) {
			WL_ERROR(("%s: pktfetch request rejected\n", __FUNCTION__));
			goto error;
		}
	}
#endif /* BCMPCIEDEV */

	return ret;

error:
	if (pinfo)
		MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info));

	if (ctx)
		MFREE(wlc->osh, ctx, sizeof(wlc_eapol_pktfetch_ctx_t));

	if (f->p)
		PKTFREE(wlc->osh, f->p, FALSE);
	return ret;
}

static void
wlc_recvdata_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctxt, bool cancelled)
{
	wlc_eapol_pktfetch_ctx_t *ctx = ctxt;
	wlc_info_t *wlc = ctx->wlc;
	struct scb *scb = NULL;
	osl_t *osh = wlc->osh;
	wlc_frminfo_t f;
	bool ampdu_path = ctx->ampdu_path;
	bool ordered = ctx->ordered;
	bool promisc = ctx->promisc;
	wlc_bsscfg_t *bsscfg = NULL;

	/* Replicate frameinfo buffer */
	bcopy(&ctx->f, &f, sizeof(wlc_frminfo_t));
	if (!(ctx->flags & PKTFETCH_FLAG_AMSDU_SUBMSDUS))
		wlc_recreate_frameinfo(wlc, lbuf, orig_lfrag, &ctx->f, &f);
	else {
		int err = 0;
		bsscfg = wlc_bsscfg_find(wlc, WLPKTTAGBSSCFGGET(orig_lfrag), &err);
		if (bsscfg)
			scb = wlc_scbfind(wlc, bsscfg, &ctx->ea);
		/* Subsequent subframe fetches */
		PKTPUSH(osh, lbuf, PKTLEN(osh, orig_lfrag));
		/* Copy the original lfrag data  */
		memcpy(PKTDATA(osh, lbuf), PKTDATA(osh, orig_lfrag), PKTLEN(osh, orig_lfrag));
		f.p = lbuf;
		/* Set length of the packet */
		PKTSETLEN(wlc->osh, f.p, PKTLEN(osh, orig_lfrag) +
			PKTFRAGTOTLEN(wlc->osh, orig_lfrag));
	}

	/* Copy PKTTAG */
	memcpy(WLPKTTAG(lbuf), WLPKTTAG(orig_lfrag), sizeof(wlc_pkttag_t));
	/* reset pkt next info old one needs to un chain, new one needs to be chained */
	PKTSETNEXT(osh, lbuf, PKTNEXT(osh, orig_lfrag));
	PKTSETNEXT(osh, orig_lfrag, NULL);

	/* Cleanup first before dispatch */
	PKTFREE(osh, orig_lfrag, FALSE);

	/* Extract scb info */
	if (!scb && !(ctx->flags & PKTFETCH_FLAG_AMSDU_SUBMSDUS))
		wlc_pktfetch_get_scb(wlc, &ctx->f, &bsscfg, &scb, promisc, ctx->scb_assoctime);
	 if (scb == NULL) {
		/* Unable to acquire SCB: Clean up */
		WL_INFORM(("%s: Unable to acquire scb!\n", __FUNCTION__));
		PKTFREE(osh, lbuf, FALSE);
	} else {
		if ((ctx->flags & PKTFETCH_FLAG_AMSDU_SUBMSDUS)) {
			/* non first MSDU/suframe and if its 802.1x frame then fetch come here */
			if (wlc_process_eapol_frame(wlc, scb->bsscfg, scb, &f, f.p)) {
				WL_INFORM(("Processed fetched msdu %p\n", f.p));
				/* We have consumed the pkt drop and continue; */
				PKTFREE(osh, f.p, FALSE);
			} else {
				/* Call sendup as frames are in Ethernet format */
				wlc_recvdata_sendup(wlc, scb, FALSE,
					(struct ether_addr *)PKTDATA(wlc->osh, f.p), f.p);
			}
		} else if (ampdu_path) {
			if (ordered)
				wlc_recvdata_ordered(wlc, scb, &f);
			else
				wlc_ampdu_recvdata(wlc->ampdu_rx, scb, &f);
		} else {
			wlc_recvdata_ordered(wlc, scb, &f);
		}
	}
	MFREE(osh, ctx->pinfo, sizeof(struct pktfetch_info));
	MFREE(osh, ctx, sizeof(wlc_eapol_pktfetch_ctx_t));

}

/* Recreates the frameinfo buffer based on offsets of the pulled packet */
static void
wlc_recreate_frameinfo(wlc_info_t *wlc, void *lbuf, void *lfrag,
	wlc_frminfo_t *fold, wlc_frminfo_t *fnew)
{
	osl_t *osh = wlc->osh;
	int16 offset_from_start;
	int16 offset = 0;
	uint8 dot11_offset = 0;
	unsigned char llc_hdr[8] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e};
	uint16* ether_type = NULL;

	/* During recvd frame processing, Pkt Data pointer was offset by some bytes
	 * Need to restore it to start
	 */
	offset_from_start = PKTDATA(osh, lfrag) - (uchar*)fold->wrxh;

	/* Push up data pointer to start of pkt: RxStatus start (wlc_d11rxhdr_t) */
	PKTPUSH(osh, lfrag, offset_from_start);

	/* lfrag data pointer is now at start of pkt.
	 * Push up lbuf data pointer by PKTLEN of lfrag
	 */
	if (SPLITRX_DYN_MODE4(osh, lfrag)) {
		if ((fold->wrxh->rxhdr.lt80.HdrConvSt) & ETHER_HDR_CONV_TYPE) {
			ether_type = (unsigned short*)(fold->pbody + ETHER_TYPE_2_OFFSET);
		} else {
			ether_type = (unsigned short*)(fold->pbody + ETHER_TYPE_1_OFFSET);
		}
		llc_hdr[6] = (uint8)*ether_type;
		llc_hdr[7] = (uint8)((*ether_type) >> 8);
		/* 802.11 hdr offset */
		dot11_offset = ((uchar*)fold->pbody) - (PKTDATA(osh, lfrag)+ offset_from_start);
		PKTPUSH(osh, lbuf, dot11_offset + LLC_HDR_LEN + offset_from_start);
		/* Copy the lfrag data starting from RxStatus (wlc_d11rxhdr_t)
		* till  802.11 hdr
		*/
		bcopy(PKTDATA(osh, lfrag), PKTDATA(osh, lbuf), (dot11_offset + offset_from_start));
		bcopy(llc_hdr, (PKTDATA(osh, lbuf)+ dot11_offset + offset_from_start), LLC_HDR_LEN);
	} else {
		PKTPUSH(osh, lbuf, PKTLEN(osh, lfrag));
		/* Copy the lfrag data starting from RxStatus (wlc_d11rxhdr_t) */
		bcopy(PKTDATA(osh, lfrag), PKTDATA(osh, lbuf), PKTLEN(osh, lfrag));
	}

	/* wrxh and rxh pointers: both have same address */
	fnew->wrxh = (wlc_d11rxhdr_t *)PKTDATA(osh, lbuf);
	fnew->rxh = &fnew->wrxh->rxhdr;

	/* Restore data pointer of lfrag and lbuf */
	PKTPULL(osh, lfrag, offset_from_start);
	PKTPULL(osh, lbuf, offset_from_start);

	/* Calculate offset of dot11_header from original lfrag
	 * and apply the same to lbuf frameinfo
	 */
	offset = ((uchar*)fold->h) - PKTDATA(osh, lfrag);
	fnew->h = (struct dot11_header *)(PKTDATA(osh, lbuf) + offset);

	/* Calculate the offset of Packet body pointer
	 * from original lfrag and apply to lbuf frameinfo
	 */
	offset = ((uchar*)fold->pbody) - PKTDATA(osh, lfrag);
	fnew->pbody = (uchar*)(PKTDATA(osh, lbuf) + offset);

	fnew->p = lbuf;
}

#if defined(PKTC) || defined(PKTC_DONGLE)
bool
wlc_sendup_chain_pktfetch_required(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, void *p,
		uint16 body_offset)
{
	uint body_len;
	struct dot11_llc_snap_header *lsh;

	/* For AMSDU pkts, packet starts after subframe header */
	if (WLPKTTAG(p)->flags & WLF_HWAMSDU)
		body_offset += ETHER_HDR_LEN;

	lsh = (struct dot11_llc_snap_header *) (PKTDATA(wlc->osh, p) + body_offset);
	body_len = PKTLEN(wlc->osh, p) - body_offset;
	BCM_REFERENCE(body_len);

	if ((PKTFRAGUSEDLEN(wlc->osh, p) > 0) &&
		LLC_SNAP_HEADER_CHECK(lsh) &&
		(EAPOL_PKTFETCH_REQUIRED(lsh) ||
#ifdef WLTDLS
		WLTDLS_PKTFETCH_REQUIRED(wlc, lsh) ||
#endif
#ifdef WLNDOE
		NDOE_PKTFETCH_REQUIRED(wlc, lsh, lsh, body_len) ||
#endif
#ifdef WOWLPF
		WOWLPF_ACTIVE(wlc->pub) ||
#endif
#ifdef BDO
		BDO_PKTFETCH_REQUIRED(wlc, lsh) ||
#endif
#ifdef WL_TBOW
		WLTBOW_PKTFETCH_REQUIRED(wlc, bsscfg, ntoh16(lsh->type)) ||
#endif
#ifdef TKO
		TKO_PKTFETCH_REQUIRED(wlc, lsh) ||
#endif
		FALSE))
		return TRUE;

	return FALSE;
}

void
wlc_sendup_schedule_pktfetch(wlc_info_t *wlc, void *pkt)
{
	struct pktfetch_info *pinfo = NULL;
	struct pktfetch_generic_ctx *pctx = NULL;
	int ctx_count = 2;	/* No. of ctx variables needed to be saved */
	uint32 copycount = (wlc->pub->tunables->copycount << 2);

	pinfo = MALLOCZ(wlc->osh, sizeof(struct pktfetch_info));
	if (!pinfo) {
		WL_ERROR(("%s: Out of mem: Unable to alloc pktfetch ctx!\n", __FUNCTION__));
		goto error;
	}

	pctx = MALLOCZ(wlc->osh, sizeof(struct pktfetch_generic_ctx) +
		ctx_count*sizeof(void*));
	if (!pctx) {
		WL_ERROR(("%s: Out of mem: Unable to alloc pktfetch ctx!\n", __FUNCTION__));
		goto error;
	}

	/* Fill up context */
	pctx->ctx_count = ctx_count;
	pctx->ctx[0] = (void *)wlc;
	pctx->ctx[1] = (void *)pinfo;

	/* Fill up pktfetch info */
	/* In case of RXMODE 3, host has full pkt including RxStatus (WL_HWRXOFF_AC bytes)
	 * Host buffer addresses in the lfrag points to start of this full pkt
	 * So, pktfetch host_offset needs to be set to (copycount + HW_RXOFF)
	 * so that only necessary (remaining) data if pktfetched from Host.
	*/
	if (SPLITRX_DYN_MODE3(wlc->osh, pkt)) {
		pinfo->host_offset = copycount + wlc->hwrxoff;
		PKTSETFRAGTOTLEN(wlc->osh, pkt,
			PKTFRAGTOTLEN(wlc->osh, pkt) + copycount + wlc->hwrxoff);
	}
	else
		pinfo->host_offset = 0;

	pinfo->osh = wlc->osh;
	pinfo->headroom = PKTRXFRAGSZ;

	/* if key processing done, make headroom to save body_offset */
	if (WLPKTTAG(pkt)->flags & WLF_RX_KM)
		pinfo->headroom += PKTBODYOFFSZ;
	pinfo->lfrag = (void*)pkt;
	pinfo->cb = wlc_sendup_pktfetch_cb;
	pinfo->ctx = (void *)pctx;
	pinfo->next = NULL;
	if (hnd_pktfetch(pinfo) != BCME_OK) {
		WL_ERROR(("%s: pktfetch request rejected\n", __FUNCTION__));
		goto error;
	}

	return;

error:

	if (pinfo)
		MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info));

	if (pctx)
		MFREE(wlc->osh, pctx, sizeof(struct pktfetch_generic_ctx));

	if (pkt)
		PKTFREE(wlc->osh, pkt, FALSE);
}

static void
wlc_sendup_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctxt, bool cancelled)
{
	wlc_info_t *wlc;
	uint lcl_len;
	struct pktfetch_info *pinfo;
	struct pktfetch_generic_ctx *pctx = (struct pktfetch_generic_ctx *)ctxt;
	int ctx_count = pctx->ctx_count;
	uint32 body_offset;

	/* Retrieve contexts */
	wlc = (wlc_info_t *)pctx->ctx[0];
	pinfo = (struct pktfetch_info *)pctx->ctx[1];
	body_offset = (uint32)pctx->ctx[2];

	lcl_len = PKTLEN(wlc->osh, orig_lfrag);
	PKTPUSH(wlc->osh, lbuf, lcl_len);
	bcopy(PKTDATA(wlc->osh, orig_lfrag), PKTDATA(wlc->osh, lbuf), lcl_len);

	/* append body_offset if key management has been processed */
	if (WLPKTTAG(orig_lfrag)->flags & WLF_RX_KM) {
		PKTPUSH(wlc->osh, lbuf, PKTBODYOFFSZ);
		*((uint32 *)PKTDATA(wlc->osh, lbuf)) = body_offset;
	}
	/* Copy wl pkttag area */
	wlc_pkttag_info_move(wlc, orig_lfrag, lbuf);
	PKTSETIFINDEX(wlc->osh, lbuf, PKTIFINDEX(wlc->osh, orig_lfrag));
	PKTSETPRIO(lbuf, PKTPRIO(orig_lfrag));

	/* Free the original pktfetch_info and generic ctx  */
	MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info));
	MFREE(wlc->osh, pctx, sizeof(struct pktfetch_generic_ctx)
		+ ctx_count*sizeof(void *));

	PKTFREE(wlc->osh, orig_lfrag, TRUE);

	/* Mark this lbuf has pktfetched lbuf pkt */
	PKTSETPKTFETCHED(wlc->osh, lbuf);
	wlc_sendup_chain(wlc, lbuf);
}
#endif /* PKTC || PKTC_DONGLE */

#endif /* BCMSPLITRX */
