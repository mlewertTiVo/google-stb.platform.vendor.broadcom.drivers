/*
 * MSDU aggregation related header file
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
 * $Id: wlc_amsdu.h 645630 2016-06-24 23:27:55Z $
*/


#ifndef _wlc_amsdu_h_
#define _wlc_amsdu_h_
/* A-MSDU policy limits */
typedef struct amsdu_txpolicy {
#ifdef TXQ_MUX
	uint	aggbytes_hwm_delta;	/* Length delta for highwatermark */
	bool 	ackRatioEnabled;	/* Set if skipping of TCP ACKS is specified */
	uint32 	passthrough_flags;	/* A-MSDU passthrough condition flags */
	uint16	packet_headroom;	/* Packet headroom at head of A-MSDU */
#else
	uint16	fifo_lowm;		/* low watermark for tx queue precendence */
	uint16	fifo_hiwm;		/* high watermark for tx queue precendence */
#endif /* TXQ_MUX */
	bool	amsdu_agg_enable;	/* TRUE: agg is allowed, FALSE: agg is disallowed */
	uint 	amsdu_max_sframes;	/* Maximum allowed subframes per A-MSDU */
	uint 	amsdu_max_agg_bytes;	/* Maximum allowed payload bytes per A-MSDU */
} amsdu_txpolicy_t;

#ifdef TXQ_MUX
typedef struct amsdu_txsession {
	void			*amsdu_ctx;		/* TX session context */
	wlc_bsscfg_t 	*bsscfg;		/* BSSCFG of the A-MSDU-- optimization */
	ratespec_t		rspec;			/* Rspec for length and frame calc */
	uint			priority;		/* TID/PRIORITY of this A-MSDU */
	uint			aggbytes_hwm;	/* Highwater mark for A-MSDU payload length */
	void 			*ctx;			/* session context pointer */
	bool			ratelimit;		/* SET if rspec limits A-MSDU maxlen */
} amsdu_txsession_t;

typedef struct amsdu_txinfo {
	/* Init by A-MSDU module but can be overiden by caller */
	amsdu_txpolicy_t	txpolicy;

	/* Init by A-MSDU module, exposed to caller for storage allocation purposes */
	amsdu_txsession_t	txsession;
} amsdu_txinfo_t;
#endif /* TXQ_MUX */

extern amsdu_info_t *wlc_amsdu_attach(wlc_info_t *wlc);
extern void wlc_amsdu_detach(amsdu_info_t *ami);
extern bool wlc_amsdutx_cap(amsdu_info_t *ami);
extern bool wlc_amsdurx_cap(amsdu_info_t *ami);
extern uint wlc_amsdu_mtu_get(amsdu_info_t *ami); /* amsdu MTU, depend on rx fifo limit */

extern void wlc_amsdu_flush(amsdu_info_t *ami);
extern void *wlc_recvamsdu(amsdu_info_t *ami, wlc_d11rxhdr_t *wrxh, void *p, uint16 *padp,
		bool chained_sendup);
extern void wlc_amsdu_deagg_hw(amsdu_info_t *ami, struct scb *scb,
	struct wlc_frminfo *f);
#ifdef WLAMSDU_SWDEAGG
extern void wlc_amsdu_deagg_sw(amsdu_info_t *ami, struct scb *scb,
	struct wlc_frminfo *f);
#endif /* SWDEAGG */

#ifdef WLAMSDU_TX
extern int wlc_amsdu_set(amsdu_info_t *ami, bool val);
extern void wlc_amsdu_agglimit_frag_upd(amsdu_info_t *ami);
extern void wlc_amsdu_txop_upd(amsdu_info_t *ami);
extern void wlc_amsdu_scb_agglimit_upd(amsdu_info_t *ami, struct scb *scb);
extern void wlc_amsdu_txpolicy_upd(amsdu_info_t *ami);
#ifdef WL11AC
extern void wlc_amsdu_scb_vht_agglimit_upd(amsdu_info_t *ami, struct scb *scb);
#endif /* WL11AC */
extern void wlc_amsdu_scb_ht_agglimit_upd(amsdu_info_t *ami, struct scb *scb);
#ifdef TXQ_MUX
extern void *wlc_amsdu_pktq_agg(struct pktq *pq, uint pm, amsdu_txinfo_t *txi, void *pkt);
extern bool wlc_amsdu_init_session(wlc_info_t *wlc, struct scb *scb, wlc_bsscfg_t *bsscfg,
	amsdu_txinfo_t *amsdu_txinfo, uint prio, ratespec_t rspec, uint min_agglen, uint min_aggsf);
/* With TXQ_MUX, AMSDU is stateless, out of band packet flush is not supported */
#define wlc_amsdu_agg_flush(a)
#else
extern void wlc_amsdu_agg_flush(amsdu_info_t *ami);
#endif /* TXQ_MUX */
#endif /* WLAMSDU_TX */

#if defined(PKTC) || defined(PKTC_TX_DONGLE)
extern void *wlc_amsdu_pktc_agg(amsdu_info_t *ami, struct scb *scb, void *p,
	void *n, uint8 tid, uint32 lifetime);
#endif
#if defined(PKTC) || defined(PKTC_DONGLE)
extern int32 wlc_amsdu_pktc_deagg_hw(amsdu_info_t *ami, void **pp, wlc_rfc_t *rfc,
        uint16 *index, bool *chained_sendup, struct scb *scb, uint16 sec_offset);
#endif /* defined(PKTC) || defined(PKTC_TX_DONGLE) */

extern bool
wlc_amsdu_is_rxmax_valid(amsdu_info_t *ami);
extern void wlc_amsdu_update_state(wlc_info_t *wlc);

#ifdef WLCXO_CTRL
extern void wlc_cxo_tsc_amsdu_init(amsdu_info_t *ami, struct scb *scb, uint8 tid,
	wlc_cx_scb_amsdu_t *cx_scb_ami);
extern void wlc_cxo_ctrl_amsdu_add(wlc_info_t *wlc, amsdu_info_t *ami);
#endif
#ifdef WL_TXQ_STALL
int wlc_amsdu_tx_health_check(wlc_info_t * wlc);
#endif
int wlc_amsdu_tx_queued_pkts(amsdu_info_t *ami, struct scb * scb, int tid, uint * timestamp);
#endif /* _wlc_amsdu_h_ */
