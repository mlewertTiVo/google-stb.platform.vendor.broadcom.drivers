/*
 * wlc_tx.h
 *
 * Common headers for transmit datapath components
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
 * $Id: wlc_tx.h 643064 2016-06-13 07:04:03Z $
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

/**
 * Lookup table for Frametypes [LEGACY (CCK / OFDM) /HT/VHT/HE]
 */
extern const int8 phy_txc_frametype_tbl[];

#define PHY_TXC_FT_LOOKUP(r)		(phy_txc_frametype_tbl[RSPEC_TO_FT_TBL_IDX(r)])

/* Finding FT from rspec */
#define PHY_TXC_FRAMETYPE(r)		((RSPEC_ISLEGACY(r)) ? \
					(RSPEC_ISOFDM(r) ? FT_OFDM : FT_CCK) : \
					(PHY_TXC_FT_LOOKUP(r)))

extern chanspec_t wlc_txh_get_chanspec(wlc_info_t* wlc, wlc_txh_info_t* tx_info);

extern uint16 wlc_get_txmaxpkts(wlc_info_t *wlc);
extern void wlc_set_txmaxpkts(wlc_info_t *wlc, uint16 txmaxpkts);
extern void wlc_set_default_txmaxpkts(wlc_info_t *wlc);

extern bool wlc_low_txq_empty(txq_t *txq);
#define WLC_TXQ_OCCUPIED(w) \
	(!pktq_empty(WLC_GET_TXQ((w)->active_queue)) || \
	!wlc_low_txq_empty((w)->active_queue->low_txq))
#ifdef TXQ_MUX
typedef uint (*txq_supply_fn_t)(void *ctx, uint fifo,
	int requested_time, struct spktq *output_q);
extern uint wlc_pull_q(void *ctx, uint fifo, int requested_time,
	struct spktq *output_q);
#else
typedef uint (*txq_supply_fn_t)(void *ctx, uint ac,
	int requested_time, struct spktq *output_q, uint *fifo_idx);
extern uint wlc_pull_q(void *ctx, uint ac_fifo, int requested_time,
	struct spktq *output_q, uint *fifo_idx);
#endif
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
extern int txq_hw_stopped(txq_t *txq, uint fifo_idx);
extern void txq_hw_fill(txq_info_t *txqi, txq_t *txq, uint fifo_idx);
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
	uint8** ptxd, uint8** ptxh, struct dot11_header** d11_hdr);
extern bool wlc_txh_get_isSGI(const wlc_txh_info_t* txh_info);
extern bool wlc_txh_get_isSTBC(const wlc_txh_info_t* txh_info);
extern uint8 wlc_txh_info_get_mcs(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern bool wlc_txh_info_is5GHz(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern bool wlc_txh_has_rts(wlc_info_t* wlc, wlc_txh_info_t* txh_info);
extern bool wlc_txh_get_isAMPDU(wlc_info_t* wlc, const wlc_txh_info_t* txh_info);
#ifdef WL11AX
extern void wlc_txh_set_ft(wlc_info_t* wlc, uint8* pdata, uint16 ft);
extern void wlc_txh_set_frag_allow(wlc_info_t* wlc, uint8* pdata, bool frag_allow);
#endif /* WL11AX */
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
extern uint16 wlc_get_txh_frameid(wlc_info_t* wlc, void* pkt);
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

extern int32 wlc_dma_tx(wlc_info_t *wlc, uint fifo, void *p, bool commit);
#ifdef WLCXO_CTRL
extern struct scb* wlc_cxo_ctrl_recover_pkt_scb(wlc_info_t *wlc, void *pkt);
extern void wlc_cxo_ctrl_prepare_test_data_frame(wlc_info_t *wlc, struct scb *scb, uint8 prio);
#endif

void wlc_beacon_phytxctl_txant_upd(wlc_info_t *wlc, ratespec_t bcn_rate);
void wlc_beacon_phytxctl(wlc_info_t *wlc, ratespec_t bcn_rspec, chanspec_t chanspec);

uint16 wlc_phytxctl1_calc(wlc_info_t *wlc, ratespec_t rspec, chanspec_t chanspec);

void wlc_txq_scb_free(wlc_info_t *wlc, struct scb *from_scb);

extern void wlc_txh_set_aging(wlc_info_t *wlc, void *hdr, bool enable);
extern bool wlc_suppress_flush_pkts(wlc_info_t *wlc, void *p,  uint16 *time_adj);

/* generic tx pktq filter */
void wlc_txq_pktq_pfilter(wlc_info_t *wlc, struct pktq *pq, int prec,
	pktq_filter_t fltr, void *ctx);
void wlc_txq_pktq_filter(wlc_info_t *wlc, struct pktq *pq, pktq_filter_t fltr, void *ctx);
void wlc_low_txq_pktq_filter(wlc_info_t *wlc, struct spktq *spq, pktq_filter_t fltr, void *ctx);

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
#if defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS)
extern void wlc_epoch_upd(wlc_info_t *wlc, void *pkt, uint8 *flipEpoch, uint8 *lastEpoch);
#endif /* defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS) */
void wlc_tx_datapath_log_dump(wlc_info_t *wlc, int tag);

#ifdef WL_FRAGDUR
extern uint wlc_calc_frame_len_ex(wlc_info_t *wlc, ratespec_t ratespec,
	uint8 preamble_type, uint dur);
#endif /* WL_FRAGDUR */

/* Below are the different reasons for packet toss in both
 * the Rx and Tx path.
 * WARNING !!! Whenever a toss reason is added to this enum,
 * make sure to add the corresponding toss reason string in
 * TX_STS_REASON_STRINGS and RX_STS_REASON_STRINGS
 */
typedef enum {
	WLC_TX_STS_QUEUED                   = 0,
	WLC_TX_STS_SUCCESS                  = 0,  /* 0 also indicates TX status success */
	WLC_TX_STS_FAILURE                  = 1,  /* 1 also indicatesgenerix TX failure */
	WLC_TX_STS_SUPPRESS                 = 1,
	WLC_TX_STS_SUPR_PMQ                 = 2,  /* PMQ entry */
	WLC_TX_STS_SUPR_FLUSH               = 3,  /* flush request */
	WLC_TX_STS_SUPR_FRAG_TBTT           = 4,  /* previous frag failure */
	WLC_TX_STS_SUPR_BADCH               = 5,  /* channel mismatch */
	WLC_TX_STS_SUPR_EXPTIME             = 6,  /* lifetime expiry */
	WLC_TX_STS_SUPR_UF                  = 7,  /* underflow */
	WLC_TX_STS_SUPR_NACK_ABS            = 8,  /* BSS entered ABSENCE period */
	WLC_TX_STS_SUPR_PPS                 = 9,  /* Pretend PS */
	WLC_TX_STS_SUPR_PHASE1_KEY          = 10, /* Req new TKIP phase-1 key */
	WLC_TX_STS_SUPR_UNUSED              = 11, /* Unused */
	WLC_TX_STS_SUPR_INT_ERR             = 12, /* Internal DMA xfer error */
	WLC_TX_STS_SUPR_RES1                = 13, /* Reserved */
	WLC_TX_STS_SUPR_RES2                = 14, /* Reserved */
	WLC_TX_STS_TX_Q_FULL                = 15,
	WLC_TX_STS_RETRY_TIMEOUT            = 16,
	WLC_TX_STS_BAD_CHAN                 = 17,
	WLC_TX_STS_PHY_ERROR                = 18,
	WLC_TX_STS_TOSS_INV_SEQ             = 19,
	WLC_TX_STS_TOSS_BSSCFG_DOWN         = 20,
	WLC_TX_STS_TOSS_CSA_BLOCK           = 21,
	WLC_TX_STS_TOSS_MFP_CCX             = 22,
	WLC_TX_STS_TOSS_NOT_PSQ1            = 23,
	WLC_TX_STS_TOSS_NOT_PSQ2            = 24,
	WLC_TX_STS_TOSS_L2_FILTER           = 25,
	WLC_TX_STS_TOSS_WMF_DROP            = 26,
	WLC_TX_STS_TOSS_WNM_DROP3           = 27,
	WLC_TX_STS_TOSS_PSTA_DROP           = 28,
	WLC_TX_STS_TOSS_CLASS3_BSS          = 29,
	WLC_TX_STS_TOSS_AWDL_DISABLED       = 30,
	WLC_TX_STS_TOSS_NULL_SCB1           = 31,
	WLC_TX_STS_TOSS_NULL_SCB2           = 32,
	WLC_TX_STS_TOSS_AWDL_NO_ASSOC       = 33,
	WLC_TX_STS_TOSS_INV_MCAST_FRAME     = 34,
	WLC_TX_STS_TOSS_INV_CLASS4_BTAMP    = 35,
	WLC_TX_STS_TOSS_HDR_CONV_FAILED     = 36,
	WLC_TX_STS_TOSS_CRYPTO_ALGO_OFF     = 37,
	WLC_TX_STS_TOSS_DROP_CAC_PKT        = 38,
	WLC_TX_STS_TOSS_WL_DOWN             = 39,
	WLC_TX_STS_TOSS_INV_CLASS3_NO_ASSOC = 40,
	WLC_TX_STS_TOSS_INV_CLASS4_BT_AMP   = 41,
	WLC_TX_STS_TOSS_INV_BAND            = 42,
	WLC_TX_STS_TOSS_FIFO_LOOKUP_FAIL    = 43,
	WLC_TX_STS_TOSS_RUNT_FRAME          = 44,
	WLC_TX_STS_TOSS_MCAST_PKT_ROAM      = 45,
	WLC_TX_STS_TOSS_STA_NOTAUTH         = 46,
	WLC_TX_STS_TOSS_NWAI_FRAME_NOTAUTH  = 47,
	WLC_TX_STS_TOSS_UNENCRYPT_FRAME1    = 48,
	WLC_TX_STS_TOSS_UNENCRYPT_FRAME2    = 49,
	WLC_TX_STS_TOSS_KEY_PREP_FAIL       = 50,
	WLC_TX_STS_TOSS_SUPR_PKT            = 51,
	WLC_TX_STS_TOSS_DOWNGRADE_FAIL      = 52,
	WLC_TX_STS_TOSS_UNKNOWN             = 53,
	WLC_TX_STS_TOSS_NON_AMPDU_SCB       = 54,
	WLC_TX_STS_TOSS_BAD_INI             = 55,
	WLC_TX_STS_TOSS_BAD_ADDR            = 56,
	WLC_TX_STS_TOSS_WET                 = 57,
	WLC_TX_STS_UNKNOWN                  = 58,
	WLC_TX_STS_NO_BUF                   = 59,
	WLC_TX_STS_COMP_NO_SCB              = 60,
	WLC_TX_STS_BAD_NCONS                = 61,
	WLC_TX_STS_LAST                     = 62
} wlc_tx_status_t;

#define TX_STS_REASON_STRINGS \
	"tx",                       /* WLC_TX_STS_QUEUED                   = 0,  */ \
	"tx_fail",                  /* WLC_TX_STS_FAILURE                  = 1,  */ \
	"supr_pmq",                 /* WLC_TX_STS_SUPR_PMQ                 = 2,  */ \
	"supr_flush",               /* WLC_TX_STS_SUPR_FLUSH               = 3,  */ \
	"supr_frag_tbtt",           /* WLC_TX_STS_SUPR_FRAG_TBTT           = 4,  */ \
	"supr_badch",               /* WLC_TX_STS_SUPR_BADCH               = 5,  */ \
	"supr_exp",                 /* WLC_TX_STS_SUPR_EXPTIME             = 6,  */ \
	"supr_uf",                  /* WLC_TX_STS_SUPR_UF                  = 7,  */ \
	"supr_nack_abs",            /* WLC_TX_STS_SUPR_NACK_ABS            = 8,  */ \
	"supr_pps",                 /* WLC_TX_STS_SUPR_PPS                 = 9,  */ \
	"supr_phase1_key",          /* WLC_TX_STS_SUPR_PHASE1_KEY          = 10, */ \
	"supr_unused",              /* WLC_TX_STS_SUPR_UNUSED              = 11, */ \
	"supr_int_err",             /* WLC_TX_STS_SUPR_INT_ERR             = 12, */ \
	"supr_res1",                /* WLC_TX_STS_SUPR_RES1                = 13, */ \
	"supr_res2",                /* WLC_TX_STS_SUPR_RES2                = 14, */ \
	"tx_q_full",                /* WLC_TX_STS_TX_Q_FULL                = 15, */ \
	"retry_tmo",                /* WLC_TX_STS_RETRY_TIMEOUT            = 16, */ \
	"bad_ch",                   /* WLC_TX_STS_BAD_CHAN                 = 17, */ \
	"phy_err",                  /* WLC_TX_STS_PHY_ERROR                = 18, */ \
	"toss_inv_seq",             /* WLC_TX_STS_TOSS_INV_SEQ             = 19, */ \
	"toss_bss_down",            /* WLC_TX_STS_TOSS_BSSCFG_DOWN         = 20, */ \
	"toss_csa_blk",             /* WLC_TX_STS_TOSS_CSA_BLOCK           = 21, */ \
	"toss_mfp_ccx",             /* WLC_TX_STS_TOSS_MFP_CCX             = 22, */ \
	"toss_not_psq1",            /* WLC_TX_STS_TOSS_NOT_PSQ1            = 23, */ \
	"toss_not_psq2",            /* WLC_TX_STS_TOSS_NOT_PSQ2            = 24, */ \
	"toss_l2_filter",           /* WLC_TX_STS_TOSS_L2_FILTER           = 25, */ \
	"toss_wmf_drop",            /* WLC_TX_STS_TOSS_WMF_DROP            = 26, */ \
	"toss_wnm_drop",            /* WLC_TX_STS_TOSS_WNM_DROP3           = 27, */ \
	"toss_psta",                /* WLC_TX_STS_TOSS_PSTA_DROP           = 28, */ \
	"toss_class3",              /* WLC_TX_STS_TOSS_CLASS3_BSS          = 29, */ \
	"toss_awdl_dis",            /* WLC_TX_STS_TOSS_AWDL_DISABLED       = 30, */ \
	"toss_null_scb1",           /* WLC_TX_STS_TOSS_NULL_SCB1           = 31, */ \
	"toss_null_scb2",           /* WLC_TX_STS_TOSS_NULL_SCB2           = 32, */ \
	"toss_awdl_no_assoc",       /* WLC_TX_STS_TOSS_AWDL_NO_ASSOC       = 33, */ \
	"toss_inv_mcast",           /* WLC_TX_STS_TOSS_INV_MCAST_FRAME     = 34, */ \
	"toss_inv_class4_btamp",    /* WLC_TX_STS_TOSS_INV_CLASS4_BTAMP    = 35, */ \
	"toss_hdr_conv_fail",       /* WLC_TX_STS_TOSS_HDR_CONV_FAILED     = 36, */ \
	"toss_crypto_off",          /* WLC_TX_STS_TOSS_CRYPTO_ALGO_OFF     = 37, */ \
	"toss_cac_pkt",             /* WLC_TX_STS_TOSS_DROP_CAC_PKT        = 38, */ \
	"toss_wl_down",             /* WLC_TX_STS_TOSS_WL_DOWN             = 39, */ \
	"toss_inv_class3",          /* WLC_TX_STS_TOSS_INV_CLASS3_NO_ASSOC = 40, */ \
	"toss_inv_class4",          /* WLC_TX_STS_TOSS_INV_CLASS4_BT_AMP   = 41, */ \
	"toss_inv_band",            /* WLC_TX_STS_TOSS_INV_BAND            = 42, */ \
	"toss_fifo_lookup",         /* WLC_TX_STS_TOSS_FIFO_LOOKUP_FAIL    = 43, */ \
	"toss_runt",                /* WLC_TX_STS_TOSS_RUNT_FRAME          = 44, */ \
	"toss_mcast_roam",          /* WLC_TX_STS_TOSS_MCAST_PKT_ROAM      = 45, */ \
	"toss_not_auth",            /* WLC_TX_STS_TOSS_STA_NOTAUTH         = 46, */ \
	"toss_nwai_no_auth",        /* WLC_TX_STS_TOSS_NWAI_FRAME_NOTAUTH  = 47, */ \
	"toss_unencr1",             /* WLC_TX_STS_TOSS_UNENCRYPT_FRAME1    = 48, */ \
	"toss_unencr2",             /* WLC_TX_STS_TOSS_UNENCRYPT_FRAME2    = 49, */ \
	"toss_key_fail",            /* WLC_TX_STS_TOSS_KEY_PREP_FAIL       = 50, */ \
	"toss_supr",                /* WLC_TX_STS_TOSS_SUPR_PKT            = 51, */ \
	"toss_downgrade_fail",      /* WLC_TX_STS_TOSS_DOWNGRADE_FAIL      = 52, */ \
	"toss_unknown",             /* WLC_TX_STS_TOSS_UNKNOWN             = 53, */ \
	"toss_non_ampdu_scb",       /* WLC_TX_STS_TOSS_NON_AMPDU_SCB       = 54, */ \
	"toss_bad_ini",             /* WLC_TX_STS_TOSS_BAD_INI             = 55, */ \
	"toss_bad_addr",            /* WLC_TX_STS_TOSS_BAD_ADDR            = 56, */ \
	"toss_wet",                 /* WLC_TX_STS_TOSS_WET                 = 57, */ \
	"txs_unknown",              /* WLC_TX_STS_UNKNOWN                  = 58, */ \
	"tx_nobuf",                 /* WLC_TX_STS_NO_BUF                   = 59, */ \
	"tx_comp_no_scb",           /* WLC_TX_STS_COMP_NO_SCB              = 60, */ \
	"tx_bad_ncons"              /* WLC_TX_STS_BAD_NCONS                = 61, */

/** Remap HW assigned suppress code to software suppress code */
int wlc_tx_status_map_hw_to_sw_supr_code(wlc_info_t * wlc, int supr_status);

/** In case of non full dump, only total tx and total failures are counted.
 * For any failure only the failure correspondign to index 1 will
 * be incremented.
 * WLC_TX_STS_LAST can be updated to required amount of granularity.
 * In order to achieve this entries in wlc_tx_status_t need to be
 * rearranged to have required field within the required MIN STATS range
 */
#ifdef WLC_TXSTALL_FULL_STATS
#define WLC_TX_STS_MAX WLC_TX_STS_LAST
#else
#define WLC_TX_STS_MAX	2
#endif /* WLC_TXSTALL_FULL_STATS */

#define TX_STS_RSN_MAX	(WLC_TX_STS_LAST)

#ifdef WL_TX_STALL
typedef struct wlc_tx_stall_error_info {
	uint32 stall_reason;
	uint32 stall_bitmap;
	uint32 stall_bitmap1;
	uint32 failure_ac;
	uint32 timeout;
	uint32 sample_len;
	uint32 threshold;
	uint32 tx_all;
	uint32 tx_failure_all;
#define WLC_TX_STALL_REASON_STRING_LEN 128
	char reason[WLC_TX_STALL_REASON_STRING_LEN];
} wlc_tx_stall_error_info_t;

/** Update the tx statistics for global counters, per bsscfg, per scb and per AC */
int wlc_tx_status_update_counters(wlc_info_t * wlc,
	void * pkt, scb_t * scb, wlc_bsscfg_t * bsscfg, int tx_status, int count);

/**
 * Input:
 *  - counters structure
 *  - reset_counters if sample len is more than threshold
 *  - Prefix to be used for logging and failure reason
 * Return
 *  - max failure rate
 *  - Failure info corresponding to max failure AC
 * This API will clear the counters per AC if sample count more than sample_count
 */
uint32
wlc_tx_status_calculate_failure_rate(wlc_info_t * wlc,
	wlc_tx_stall_counters_t * counters,
	bool reset_counters, const char * prefix, wlc_tx_stall_error_info_t * error);

/** TX Stall health check.
 * Verifies TX stall conditions and triggers fatal error on detecting the stall
 */
int wlc_tx_status_health_check(wlc_info_t * wlc);

/** Get the last report error */
wlc_tx_stall_error_info_t *
wlc_tx_status_get_error_info(wlc_info_t * wlc);

/** Reset TX stall statemachine */
int wlc_tx_status_reset(wlc_info_t * wlc, bool clear_all, wlc_bsscfg_t * cfg, scb_t * scb);

/** trigger a forced rx stall */
int
wlc_tx_status_force_stall(wlc_info_t * wlc, uint32 stall_type);

wlc_tx_stall_info_t *
BCMATTACHFN(wlc_tx_stall_attach)(wlc_info_t * wlc);
void
BCMATTACHFN(wlc_tx_stall_detach)(wlc_tx_stall_info_t * tx_stall);

int32
wlc_tx_status_params(wlc_tx_stall_info_t * tx_stall, bool set, int param, int value);
#define WLC_TX_STALL_IOV_THRESHOLD   (1)
#define WLC_TX_STALL_IOV_SAMPLE_LEN  (2)
#define WLC_TX_STALL_IOV_TIME        (3)
#define WLC_TX_STALL_IOV_FORCE       (4)
#define WLC_TX_STALL_IOV_EXCLUDE     (5)
#define WLC_TX_STALL_IOV_EXCLUDE1    (6)

#define WL_TX_STS_UPDATE(a, b) (a) = (b)

#else /* WL_TX_STALL */

#define wlc_tx_status_update_counters(a, b, c, d, e, f)
#define wlc_tx_status_health_check(a)

#define WL_TX_STS_UPDATE(a, b)

#endif /* WL_TX_STALL */

#ifdef WL_TX_STALL
/** Holds total TX and failures.
 * index 0 - Total TX
 * Others - failure count of each type
 */
struct wlc_tx_stall_counters
{
	uint32 sysup_time[AC_COUNT];

	/* Total consecutive stalls detected */
	uint32 stall_count[AC_COUNT];

	uint16 tx_stats[AC_COUNT][WLC_TX_STS_MAX];
};

/* Stall detected for 9 seconds ( > beacon loss) will trigger fatal error */
#ifndef WLC_TX_STALL_PEDIOD
#define WLC_TX_STALL_PEDIOD         (12)
#endif

/* Minimum sample len to be collected to validate stall  */
#ifndef WLC_TX_STALL_SAMPLE_LEN
#define WLC_TX_STALL_SAMPLE_LEN     (50)
#endif

/* >75% total vs failed TX will treated as stall         */
#ifndef WLC_TX_STALL_THRESHOLD
#define WLC_TX_STALL_THRESHOLD      (90)
#endif

#ifndef WLC_TX_STALL_EXCLUDE
#define WLC_TX_STALL_EXCLUDE         ((1 << WLC_TX_STS_TOSS_BSSCFG_DOWN) | \
					(1 << WLC_TX_STS_SUPR_EXPTIME)     | \
					(1 << WLC_TX_STS_RETRY_TIMEOUT))
#endif

#ifndef WLC_TX_STALL_EXCLUDE1
#define WLC_TX_STALL_EXCLUDE1        (0)
#endif

#ifndef WLC_TX_STALL_ASSERT_ON_ERROR
#ifdef WL_DATAPATH_HC_NOASSERT
#define WLC_TX_STALL_ASSERT_ON_ERROR 0
#else
#define WLC_TX_STALL_ASSERT_ON_ERROR 1
#endif
#endif /* WLC_TX_STALL_ASSERT_ON_ERROR */

#define WLC_TX_STALL_REASON_TX_ERROR            (1 << 0)
#define WLC_TX_STALL_REASON_TXQ_NO_PROGRESS     (1 << 1)
#define WLC_TX_STALL_REASON_RXQ_NO_PROGRESS     (1 << 2)

/* History of last 60 seconds */
#ifndef WLC_TX_STALL_COUNTERS_HISTORY_LEN
#define WLC_TX_STALL_COUNTERS_HISTORY_LEN   (64)
#endif

struct wlc_tx_stall_info {
	wlc_info_t * wlc;
	int scb_handle;
	int cfg_handle;

	/* TX counters given validation period  */
	wlc_tx_stall_counters_t counters;

	/* TX DROP/FAILURE reasons to be excluded */
	uint32 exclude_bitmap;
	uint32 exclude_bitmap1;

	/* History of N periods   */
	wlc_tx_stall_counters_t * history;
	uint32 history_idx;

	/* timeout value. 0 - Disable validation */
	uint32 timeout;

	/* Minimum number of packets to be accumulated before health check */
	uint32 sample_len;

	/* Success vs failure percentage to trigger failure   */
	uint32 stall_threshold;

	/* Test code to pretend tx_stall */
	bool force_tx_stall;

	/* Trigger fatal error on stall detection */
	bool assert_on_error;

	/* Stall info */
	wlc_tx_stall_error_info_t error;
};

#endif /* WL_TX_STALL */

#if defined(HC_TX_HANDLER)
int wlc_hc_tx_set(void *ctx, const uint8 *buf, uint16 type, uint16 len);
int wlc_hc_tx_get(wlc_info_t *wlc, wlc_if_t *wlcif,
	bcm_xtlv_t *params, void *out, uint o_len);
#endif /* HC_TX_HANDLER */



#ifdef WL_TXQ_STALL
int wlc_txq_health_check(wlc_info_t *wlc);
int wlc_txq_hc_global_txq(wlc_info_t *wlc);

int wlc_txq_hc_ampdu_txq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb);
int wlc_txq_hc_ampdu_rxq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb);
int wlc_txq_hc_awdl_rcq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb);
int wlc_txq_hc_ap_psq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb);
int wlc_txq_hc_supr_txq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb);
#endif /* WL_TXQ_STALL */
#ifdef WLATF_DONGLE
extern ratespec_t wlc_ravg_get_scb_cur_rspec(wlc_info_t *wlc, struct scb *scb);
extern uint32 wlc_scb_dot11hdrsize(struct scb *scb);
#endif /* WLATF_DONGLE */

typedef struct wlc_d11axtxh_rate wlc_d11axtxh_rate_t;
struct wlc_d11axtxh_rate {
	d11axtxh_rate_fixed_t *txh_rate_fixed;	/* per rate info - fixed portion */
	uint16	*FbwInfo;	/* Considering that FBwInfo is of variable length */
	uint8	*Bfm0;		/* Each power offset is 1 byte long */

	/* Ensure that PhyTxControlByte is the last member of the struct */
	uint8 *PhyTxControlByte;
};

extern uint16 wlc_compute_RateInfoElem_size(uint8 n_users, uint8 n_rus,
	uint8 n_pwr_offsets);

/**
* Size of entire variable length RateInfo block
*/
#define D11AX_TXH_RATE_BLKSIZE(nu, nru, pwr_offs)	((D11AC_TXH_NUM_RATES) * \
						(wlc_compute_RateInfoElem_size(nu, nru, pwr_offs)))

/*
* Size of the TXH is variable. Size shall vary depending upon
* no. of phytxctrl words to be used and filled in each RateInfo block.
*/
#define D11AX_TXH_LEN(nu, nru, pwr_offs)	((D11AX_TXH_FIXED_LEN) + \
						(D11AX_TXH_RATE_BLKSIZE(nu, nru, pwr_offs)))

/** Abstraction for 11ax and 11ac per cache info pointers */
typedef union wlc_d11txh_cache_info wlc_d11txh_cache_info_u;
union wlc_d11txh_cache_info {
	d11actxh_cache_t *d11actxh_CacheInfo;	/* 11ac txh per cache info pointer */
	d11axtxh_cache_t *d11axtxh_CacheInfo;	/* 11ax txh per cache info pointer */
};

/** Abstraction for 11ax and 11ac txds */
typedef union wlc_d11txh_info wlc_d11txh_u;
union wlc_d11txh_info {
	d11actxh_t *d11actxh;	/* 11ac txh per cache info pointer */
	d11axtxh_t *d11axtxh;	/* 11ax txh per cache info pointer */
};

/* pointers to TXD per packet info fields */
typedef struct wlc_d11pktinfo wlc_d11pktinfo_t;
struct wlc_d11pktinfo {
	d11pktinfo_common_t *PktInfo;	/* 0 - 20 */
	d11pktinfo_ax_t *PktInfoExt;	/* 21-23 */
};
#endif /* _wlc_tx_c */
