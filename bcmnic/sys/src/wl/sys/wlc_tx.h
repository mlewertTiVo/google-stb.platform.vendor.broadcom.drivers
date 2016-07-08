/*
 * wlc_tx.h
 *
 * Common headers for transmit datapath components
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
 * $Id: wlc_tx.h 613017 2016-01-15 23:02:23Z $
 *
 */
#ifndef _wlc_tx_c
#define _wlc_tx_c

/* Place holder for tx datapath functions
 * Refer to RB http://wlan-rb.sj.broadcom.com/r/18439/ and
 * TWIKI http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/WlDriverTxQueuingUpdate2013
 * for details on datapath restructuring work
 * These functions will be moved in stages from wlc.[ch]
 */

#define WLPKTTIME(p) (WLPKTTAG(p)->pktinfo.atf.pkt_time)
#define TX_FIFO_BITMAP(fifo)	(1<<(fifo))		/* TX FIFO number to bit map */

extern chanspec_t wlc_txh_get_chanspec(wlc_info_t* wlc, wlc_txh_info_t* tx_info);

extern uint16 wlc_get_txmaxpkts(wlc_info_t *wlc);
extern void wlc_set_txmaxpkts(wlc_info_t *wlc, uint16 txmaxpkts);
extern void wlc_set_default_txmaxpkts(wlc_info_t *wlc);

extern struct pktq* wlc_low_txq(txq_t *txq);
#define WLC_TXQ_OCCUPIED(w) \
	(!pktq_empty(WLC_GET_TXQ((w)->active_queue)) || \
	!pktq_empty(wlc_low_txq((w)->active_queue->low_txq)))

typedef uint (*txq_supply_fn_t)(void *ctx, uint fifo_idx,
	int requested_time, struct spktq *output_q);

extern uint wlc_pull_q(void *ctx, uint fifo_idx, int requested_time,
	struct spktq *output_q);
void wlc_low_txq_enq(txq_info_t *txqi,
	txq_t *txq, uint fifo_idx, void *pkt, int pkt_time);

/* used in bmac_fifo: acct for pkts that don't go through normal tx path
 * best example is pkteng
*/
extern int wlc_low_txq_buffered_inc(txq_t *txq, uint fifo_idx, int inc_time);

extern int wlc_low_txq_buffered_dec(txq_t *txq, uint fifo_idx, int dec_time);

extern uint wlc_txq_nfifo(txq_t *txq);

#ifdef TXQ_MUX

/* initial Tx MUX service quanta in usec */
#define WLC_TX_MUX_QUANTA 2000

extern ratespec_t wlc_tx_current_ucast_rate(wlc_info_t *wlc,
	struct scb *scb, uint ac);
extern ratespec_t wlc_pdu_txparams_rspec(osl_t *osh, void *p);
extern uint wlc_tx_mpdu_time(wlc_info_t *wlc, struct scb* scb,
	ratespec_t rspec, uint ac, uint mpdu_len);
extern uint wlc_tx_mpdu_frame_seq_overhead(ratespec_t rspec,
	wlc_bsscfg_t *bsscfg, wlcband_t *band, uint ac);
extern int wlc_txq_immediate_enqueue(wlc_info_t *wlc,
	wlc_txq_info_t *qi, void *pkt, uint prec);
extern ratespec_t wlc_lowest_scb_rspec(struct scb *scb);
extern ratespec_t wlc_lowest_band_rspec(wlcband_t *band);

#endif /* TXQ_MUX */

extern txq_info_t * wlc_txq_attach(wlc_info_t *wlc);
extern void wlc_txq_detach(txq_info_t *txq);
extern txq_t* wlc_low_txq_alloc(txq_info_t *txqi,
                                txq_supply_fn_t supply_fn, void *supply_ctx,
                                uint nfifo, int high, int low);

extern void wlc_low_txq_free(txq_info_t *txqi, txq_t* txq);
extern void wlc_low_txq_flush(txq_info_t *txqi, txq_t* txq);

extern void wlc_txq_fill(txq_info_t *txqi, txq_t *txq);
extern void wlc_txq_complete(txq_info_t *txqi, txq_t *txq, uint fifo_idx,
                             int complete_pkts, int complete_time);
extern uint8 txq_stopped_map(txq_t *txq);
extern int wlc_txq_buffered_time(txq_t *txq, uint fifo_idx);
extern void wlc_tx_fifo_attach_complete(wlc_info_t *wlc);
/**
 * @brief Sanity check on the Flow Control state of the TxQ
 */
extern int wlc_txq_fc_verify(txq_info_t *txqi, txq_t *txq);
extern wlc_txq_info_t* wlc_txq_alloc(wlc_info_t *wlc, osl_t *osh);
extern void wlc_txq_free(wlc_info_t *wlc, osl_t *osh, wlc_txq_info_t *qi);
extern void wlc_send_q(wlc_info_t *wlc, wlc_txq_info_t *qi);
extern void wlc_send_active_q(wlc_info_t *wlc);
extern int wlc_prep_pdu(wlc_info_t *wlc, struct scb *scb, void *pdu, uint *fifo);
extern int wlc_prep_sdu(wlc_info_t *wlc, struct scb *scb, void **sdu, int *nsdu, uint *fifo);
extern int wlc_prep_sdu_fast(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct scb *scb,
	void *sdu, uint *fifop);
extern void* wlc_hdr_proc(wlc_info_t *wlc, void *sdu, struct scb *scb);
extern uint16 wlc_d11hdrs(wlc_info_t *wlc, void *p, struct scb *scb, uint txparams_flags,
	uint frag, uint nfrags, uint queue, uint next_frag_len,
	const wlc_key_info_t *key_info, ratespec_t rspec_override, uint16 *txh_off);
extern void wlc_txprep_pkt_get_hdrs(wlc_info_t* wlc, void* p,
	uint8** txd, d11actxh_t** vhtHdr, struct dot11_header** d11_hdr);
extern bool wlc_txh_get_isSGI(const wlc_txh_info_t* txh_info);
extern bool wlc_txh_get_isSTBC(const wlc_txh_info_t* txh_info);
extern uint8 wlc_txh_info_get_mcs(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern bool wlc_txh_info_is5GHz(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern bool wlc_txh_has_rts(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern bool wlc_txh_get_isAMPDU(wlc_info_t* wlc, const wlc_txh_info_t* txh_info);
extern void wlc_txh_set_epoch(wlc_info_t* wlc, uint8* pdata, uint8 epoch);
extern uint8 wlc_txh_get_epoch(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
#if defined(TXQ_MUX)
#define WLC_TXFIFO_COMPLETE(w, f, p, t) wlc_txfifo_complete((w), (f), (p), (t))
extern void wlc_txfifo_complete(wlc_info_t *wlc, uint fifo, uint16 txpktpend, int txpkttime);
#define WLC_LOWEST_SCB_RSPEC(scb) wlc_lowest_scb_rspec((scb))
#define WLC_LOWEST_BAND_RSPEC(band) wlc_lowest_band_rspec((band))
#else
#define WLC_TXFIFO_COMPLETE(w, f, p, t) wlc_txfifo_complete((w), (f), (p))
extern void wlc_txfifo_complete(wlc_info_t *wlc, uint fifo, uint16 txpktpend);
#define WLC_LOWEST_SCB_RSPEC(scb) 0
#define WLC_LOWEST_BAND_RSPEC(band) 0
#endif /* TXQ_MUX */
extern void wlc_get_txh_info(wlc_info_t* wlc, void* pkt, wlc_txh_info_t* tx_info);
extern void wlc_set_txh_info(wlc_info_t* wlc, void* pkt, wlc_txh_info_t* tx_info);
extern void wlc_block_datafifo(wlc_info_t *wlc, uint32 mask, uint32 val);
extern void wlc_txflowcontrol(wlc_info_t *wlc, wlc_txq_info_t *qi, bool on, int prio);
extern void wlc_txflowcontrol_override(wlc_info_t *wlc, wlc_txq_info_t *qi, bool on, uint override);
extern bool wlc_txflowcontrol_prio_isset(wlc_info_t *wlc, wlc_txq_info_t *qi, int prio);
extern bool wlc_txflowcontrol_override_isset(wlc_info_t *wlc, wlc_txq_info_t *qi, uint override);
extern void wlc_txflowcontrol_reset(wlc_info_t *wlc);
extern void wlc_txflowcontrol_reset_qi(wlc_info_t *wlc, wlc_txq_info_t *qi);
extern uint16 bcmc_fid_generate(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint16 txFrameID);

#define WLC_BLOCK_DATAFIFO_SET(wlc, val) wlc_block_datafifo((wlc), (val), (val))
#define WLC_BLOCK_DATAFIFO_CLEAR(wlc, val) wlc_block_datafifo((wlc), (val), 0)

extern void wlc_tx_fifo_sync_complete(wlc_info_t *wlc, uint fifo_bitmap, uint8 flag);
extern void wlc_excursion_start(wlc_info_t *wlc);
extern void wlc_excursion_end(wlc_info_t *wlc);
extern void wlc_primary_queue_set(wlc_info_t *wlc, wlc_txq_info_t *new_primary);
extern void wlc_active_queue_set(wlc_info_t *wlc, wlc_txq_info_t *new_active_queue);
extern void wlc_sync_txfifo(wlc_info_t *wlc, wlc_txq_info_t *qi, uint fifo_bitmap,
	uint8 flag);

#ifdef STA
extern void *wlc_sdu_to_pdu(wlc_info_t *wlc, void *sdu, struct scb *scb, bool is_8021x);
#endif /* STA */

void wlc_beacon_phytxctl_txant_upd(wlc_info_t *wlc, ratespec_t bcn_rate);
void wlc_beacon_phytxctl(wlc_info_t *wlc, ratespec_t bcn_rspec, chanspec_t chanspec);

uint16 wlc_phytxctl1_calc(wlc_info_t *wlc, ratespec_t rspec, chanspec_t chanspec);

void wlc_txq_scb_free(wlc_info_t *wlc, struct scb *from_scb);

extern void wlc_txh_set_aging(wlc_info_t *wlc, void *hdr, bool enable);

/* generic tx pktq filter */
void wlc_txq_pktq_pfilter(wlc_info_t *wlc, struct pktq *pq, int prec,
	pktq_filter_t fltr, void *ctx);
void wlc_txq_pktq_filter(wlc_info_t *wlc, struct pktq *pq, pktq_filter_t fltr, void *ctx);

/* bsscfg based tx pktq filter */
void wlc_txq_pktq_cfg_filter(wlc_info_t *wlc, struct pktq *pq, wlc_bsscfg_t *cfg);
/* scb based tx pktq filter */
void wlc_txq_pktq_scb_filter(wlc_info_t *wlc, struct pktq *pq, struct scb *scb);
/** process packets associated with the freeing scb */
#define wlc_pktq_scb_free(wlc, pq, scb) \
	wlc_txq_pktq_scb_filter(wlc, pq, scb)

/* flush tx pktq */
void wlc_txq_pktq_pflush(wlc_info_t *wlc, struct pktq *pq, int prec);
void wlc_txq_pktq_flush(wlc_info_t *wlc, struct pktq *pq);

#endif /* _wlc_tx_c */
