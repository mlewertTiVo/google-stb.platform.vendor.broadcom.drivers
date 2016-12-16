/*
 * wlc_tx.c
 *
 * Common transmit datapath components
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
 * $Id: wlc_tx.c 664252 2016-10-11 21:27:47Z $
 *
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlc_types.h>
#include <siutils.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <bcmwifi_channels.h>
#include <proto/802.1d.h>
#include <proto/802.11.h>
#include <proto/vlan.h>
#include <d11.h>
#include <wlioctl.h>
#include <wlc_rate.h>
#include <wlc.h>
#include <wlc_pub.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>
#include <wlc_ampdu.h>
#include <wlc_scb_ratesel.h>
#include <wlc_key.h>
#include <wlc_antsel.h>
#include <wlc_stf.h>
#include <wlc_ht.h>
#include <wlc_prot.h>
#include <wlc_prot_g.h>
#define _inc_wlc_prot_n_preamble_	/* include static INLINE uint8 wlc_prot_n_preamble() */
#include <wlc_prot_n.h>
#include <wlc_apps.h>
#include <wlc_bmac.h>
#if defined(WL_PROT_OBSS) && !defined(WL_PROT_OBSS_DISABLED)
#include <wlc_prot_obss.h>
#endif
#include <wlc_cac.h>
#include <wlc_vht.h>
#include <wlc_txbf.h>
#include <wlc_txc.h>
#include <wlc_keymgmt.h>
#include <wlc_led.h>
#include <wlc_ap.h>
#include <bcmwpa.h>
#include <wlc_btcx.h>
#ifdef BCMLTECOEX
#include <wlc_ltecx.h>
#endif
#include <wlc_p2p.h>
#include <wlc_bsscfg.h>
#include <wl_export.h>
#ifdef WLTOEHW
#include <wlc_tso.h>
#endif /* WLTOEHW */
#ifdef WL_LPC
#include <wlc_scb_powersel.h>
#endif /* WL_LPC */
#ifdef WLTDLS
#include <wlc_tdls.h>
#endif /* WLTDLS */
#ifdef WL_RELMCAST
#include "wlc_relmcast.h"
#endif /* WL_RELMCAST */
#if defined(STA)
#include <proto/eapol.h>
#endif 
#ifdef WLMCNX
#include <wlc_mcnx.h>
#endif /* WLMCNX */
#ifdef WLMCHAN
#include <wlc_mchan.h>
#endif /* WLMCHAN */
#ifdef WLAMSDU_TX
#include <wlc_amsdu.h>
#endif /* WLAMSDU_TX */
#ifdef L2_FILTER
#include <wlc_l2_filter.h>
#endif /* L2_FILTER */
#ifdef WET
#include <wlc_wet.h>
#endif /* WET */
#ifdef WET_TUNNEL
#include <wlc_wet_tunnel.h>
#endif /* WET_TUNNEL */
#ifdef WMF
#include <wlc_wmf.h>
#endif /* WMF */
#ifdef PSTA
#include <wlc_psta.h>
#endif /* PSTA */
#include <proto/ethernet.h>
#include <proto/802.3.h>
#ifdef STA
#include <wlc_wpa.h>
#include <wlc_pm.h>
#endif /* STA */
#ifdef PROP_TXSTATUS
#include <wlc_wlfc.h>
#endif /* PROP_TXSTATUS */
#ifdef WLWNM
#include <wlc_wnm.h>
#endif
#ifdef WL_PROXDETECT
#include <wlc_pdsvc.h>
#endif
#include <wlc_txtime.h>
#include <wlc_airtime.h>
#include <wlc_11u.h>
#include <wlc_tx.h>
#include <wlc_mbss.h>
#ifdef WL11K
#include <wlc_rrm.h>
#endif
#ifdef WL_MODESW
#include <wlc_modesw.h>
#endif
#ifdef WL_NAN
#include <wlc_nan.h>
#endif
#ifdef WL_PWRSTATS
#include <wlc_pwrstats.h>
#endif
#if defined(WL_LINKSTAT)
#include <wlc_linkstats.h>
#endif
#include <wlc_bsscfg_psq.h>
#include <wlc_txmod.h>
#ifdef WLMESH
#include <wlc_mesh.h>
#include <wlc_mesh_route.h>
#endif
#ifdef BCM_SFD
#include <wlc_sfd.h>
#endif
#include <wlc_pktc.h>
#include <wlc_misc.h>
#include <event_trace.h>
#include <wlc_pspretend.h>
#include <wlc_assoc.h>
#include <wlc_rspec.h>
#include <wlc_event_utils.h>
#include <wlc_txs.h>
#if defined(WL_DATAPATH_LOG_DUMP)
#include <event_log.h>
#endif /* WL_DATAPATH_LOG_DUMP */

#if defined(WL_OBSS_DYNBW) && !defined(WL_OBSS_DYNBW_DISABLED)
#include <wlc_obss_dynbw.h>
#endif /* defined(WL_OBSS_DYNBW) && !defined(WL_OBSS_DYNBW_DISABLED) */
#include <wlc_qoscfg.h>
#include <wlc_pm.h>
#include <wlc_scan_utils.h>
#include <wlc_perf_utils.h>
#include <wlc_dbg.h>
#include <wlc_macdbg.h>
#include <wlc_dump.h>
#ifdef BCMULP
#include <wlc_ulp.h>
#endif
#ifdef	WLAIBSS
#include <wlc_aibss.h>
#endif /* WLAIBSS */
#if defined(SAVERESTORE)
#include <saverestore.h>
#endif /* SAVERESTORE */
#include <phy_api.h>
#include <phy_lpc_api.h>
#include <wlc_log.h>

#ifdef WL_FRAGDUR
#include <wlc_fragdur.h>
#include <wlc_ampdu.h>
#include <wlc_qoscfg.h>
#endif

#if defined(WLATF_DONGLE)
#include <wlc_wlfc.h>
#endif /* WLATF_DONGLE */

#include <wlc_ampdu_cmn.h>

#include <wlc_ampdu_rx.h>

#ifdef WL_MU_TX
#include <wlc_mutx.h>
#endif

#ifdef HEALTH_CHECK
#include <hnd_hchk.h>
#endif

#ifdef TXQ_MUX
#include <wlc_bcmc_txq.h>
#endif

struct txq_fifo_state {
	int flags;	/* queue state */
	int highwater;	/* target fill capacity, in usec */
	int lowwater;	/* low-water mark to re-enable flow, in usec */
	int buffered;	/* current buffered estimate, in usec */
};

struct txq_fifo_cnt {
	uint32 pkts;
	uint32 time;
	uint32 q_stops;
	uint32 q_service;
	uint32 hw_stops;
	uint32 hw_service;
};

/* Define TXQ_LOG for extra logging info with a 'dump' function "txqlog" */
#ifdef TXQ_LOG

#ifndef TXQ_LOG_LEN
#define TXQ_LOG_LEN 1024
#endif

/* txq_log_entry.type values */
enum {
	TXQ_LOG_NULL = 0,
	TXQ_LOG_REQ = 1,
	TXQ_LOG_PKT,
	TXQ_LOG_DMA
};

/* sample format for a wlc_txq_fill() request */
typedef struct fill_req {
	uint32 time;    /* usec time of request */
	uint32 reqtime; /* usecs requested for fill */
	uint8 fifo;     /* AC fifo for the req */
} fill_req_t;

/* sample format for a pkt supplied to low TxQ for a fill req */
typedef struct pkt_sample {
	uint8 prio;     /* packet precidence */
	uint16 bytes;   /* MPDU bytes */
	uint32 time;    /* estimated txtime of pkt */
} pkt_sample_t;

/* sample format for a low TxQ fill of DMA ring */
typedef struct dma_fill {
	uint32 time;        /* usec time of request */
	uint16 pktq_before; /* pktq length before fill */
	uint16 pktq_after;  /* pktq length after fill */
	uint16 desc_before; /* DMA desc in use before fill */
	uint16 desc_after;  /* DMA desc in use after fill */
} dma_fill_t;

/* general sample format for txq_log */
typedef struct txq_log_entry {
	uint8 type;     /* the type of the sample union */
	union {
		fill_req_t req;
		pkt_sample_t pkt;
		dma_fill_t dma_fill;
	} u;
} txq_log_entry_t;

static void wlc_txq_log_req(txq_info_t *txqi, uint fifo, uint reqtime);
static void wlc_txq_log_pkts(txq_info_t *txqi, struct spktq* queue);

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int wlc_txq_log_dump(void *ctx, struct bcmstrbuf *b);
#endif

#define TXQ_LOG_REQ(txqi, fifo, reqtime)	wlc_txq_log_req(txqi, fifo, reqtime)
#define TXQ_LOG_PKTS(txqi, queue)		wlc_txq_log_pkts(txqi, queue)

#else /* !TXQ_LOG */

#define TXQ_LOG_REQ(txqi, fifo, reqtime)	do {} while (0)
#define TXQ_LOG_PKTS(txqi, queue)		do {} while (0)

#endif /* TXQ_LOG */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int wlc_datapath_dump(wlc_info_t *wlc, struct bcmstrbuf *b);
static int wlc_pktq_dump(wlc_info_t *wlc, struct pktq *q,
	wlc_bsscfg_t *bsscfg, struct scb * scb, const char * prefix, struct bcmstrbuf *b);
#endif

#define TXQ_MAXFIFO 4

/* status during txfifo sync */
#define TXQ_PKT_DEL	0x01
#define HEAD_PKT_FLUSHED 0xFF

struct txq {
	txq_t *next;
	txq_supply_fn_t supply;
	void *supply_ctx;
	uint nfifo;
	struct spktq *swq;
	struct txq_fifo_state fifo_state[NFIFO_EXT];
#ifdef WLCNT
	struct txq_fifo_cnt fifo_cnt[NFIFO_EXT];
#endif
};

struct txq_info {
	wlc_info_t *wlc;	/* Handle to wlc cntext */
	wlc_pub_t *pub;		/* Handle to public context */
	osl_t *osh;		/* OSL handle */
	txq_t *txq_list;	/* List of tx queues */
	struct spktq *delq;	/* delete queue holding pkts-to-delete temporarily */
#ifdef TXQ_MUX
/* TXQ_MUX is not a feature but the new datapath, when turned on
 * this obsoletes the current datapath, changeover from current datapath to TXQ_MUX
 * on already ROMed dongles is not supported at this time
 */
	bool rerun;		/* If set txqueue processing is in re-entrant code */
	uint rerun_count;	/* Number of times the re-entrant code is invoked */
#endif /* TXQ_MUX */
#ifdef TXQ_LOG
	uint16 log_len;		/* Length of log */
	uint16 log_idx;		/* Log index */
	txq_log_entry_t *log;	/* Handle to log entry */
#endif /* TXQ_LOG */
};

/** txq_fifo_state.flags: Flow control at top of low TxQ for queue buffered time */
#define TXQ_STOPPED     0x01

/** txq_fifo_state.flags: Mask of any HW (DMA/rpctx) flow control flags */
#define TXQ_HW_MASK     0x06

/** txq_fifo_state.flags: HW (DMA/rpctx) flow control for packet count limit */
#define TXQ_HW_STOPPED  0x02

/** txq_fifo_state.flags: HW (DMA/rpctx) flow control for outside reason */
#define TXQ_HW_HOLD     0x04

#define IS_EXCURSION_QUEUE(q, txqi) ((txq_t *)(q) == (txqi)->wlc->excursion_queue->low_txq)

/**
 * Lookup table for Frametypes [LEGACY (CCK / OFDM) /HT/VHT/HE]
 */
const int8 phy_txc_frametype_tbl[] = {FT_LEGACY, FT_HT, FT_VHT, FT_HE};

#if defined(BCMDBG)
static const uint32 txbw2rspecbw[] = {
	RSPEC_BW_20MHZ, /* WL_TXBW20L   */
	RSPEC_BW_20MHZ, /* WL_TXBW20U   */
	RSPEC_BW_40MHZ, /* WL_TXBW40    */
	RSPEC_BW_40MHZ, /* WL_TXBW40DUP */
	RSPEC_BW_20MHZ, /* WL_TXBW20LL */
	RSPEC_BW_20MHZ, /* WL_TXBW20LU */
	RSPEC_BW_20MHZ, /* WL_TXBW20UL */
	RSPEC_BW_20MHZ, /* WL_TXBW20UU */
	RSPEC_BW_40MHZ, /* WL_TXBW40L */
	RSPEC_BW_40MHZ, /* WL_TXBW40U */
	RSPEC_BW_80MHZ /* WL_TXBW80 */
};

static const uint16 txbw2phyctl0bw[] = {
	PHY_TXC1_BW_20MHZ,
	PHY_TXC1_BW_20MHZ_UP,
	PHY_TXC1_BW_40MHZ,
	PHY_TXC1_BW_40MHZ_DUP,
};

static const uint16 txbw2acphyctl0bw[] = {
	D11AC_PHY_TXC_BW_20MHZ,
	D11AC_PHY_TXC_BW_20MHZ,
	D11AC_PHY_TXC_BW_40MHZ,
	D11AC_PHY_TXC_BW_40MHZ,
	D11AC_PHY_TXC_BW_20MHZ,
	D11AC_PHY_TXC_BW_20MHZ,
	D11AC_PHY_TXC_BW_20MHZ,
	D11AC_PHY_TXC_BW_20MHZ,
	D11AC_PHY_TXC_BW_40MHZ,
	D11AC_PHY_TXC_BW_40MHZ,
	D11AC_PHY_TXC_BW_80MHZ
};

static const uint16 txbw2acphyctl1bw[] = {
	(WL_CHANSPEC_CTL_SB_LOWER >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_UPPER >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_LOWER >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_LOWER >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_LL >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_LU >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_UL >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_UU >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_LOWER >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_UPPER >> WL_CHANSPEC_CTL_SB_SHIFT),
	(WL_CHANSPEC_CTL_SB_LOWER >> WL_CHANSPEC_CTL_SB_SHIFT)
};

/* Override BW bit in phyctl */
#define WLC_PHYCTLBW_OVERRIDE(pctl, m, pctlo) \
		if (~pctlo & 0xffff)  \
			pctl = (pctl & ~(m)) | (pctlo & (m))

#else /*  defined(BCMDBG) || (defined(WLTEST) && !defined(WLTEST_DISABLED)) */

#define WLC_PHYCTLBW_OVERRIDE(pctl, m, pctlo)

static void txq_init(txq_t *txq, uint nfifo,
	txq_supply_fn_t supply_fn, void *supply_ctx, int high, int low);

static int txq_space(txq_t *txq, uint fifo_idx);
static int txq_inc(txq_t *txq, uint fifo_idx, int inc_time);
static int txq_dec(txq_t *txq, uint fifo_idx, int dec_time);
static void txq_stop_set(txq_t *txq, uint fifo_idx);
static void txq_stop_clr(txq_t *txq, uint fifo_idx);
static int txq_stopped(txq_t *txq, uint fifo_idx);
static void txq_hw_stop_set(txq_t *txq, uint fifo_idx);
static void txq_hw_stop_clr(txq_t *txq, uint fifo_idx);
static void txq_hw_hold_set(txq_t *txq, uint fifo_idx);
static void txq_hw_hold_clr(txq_t *txq, uint fifo_idx);
static int wlc_txq_down(void *ctx);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int wlc_txq_module_dump(void *ctx, struct bcmstrbuf *b);
static int wlc_txq_dump(txq_info_t *txqi, txq_t *txq, struct bcmstrbuf *b);
#endif

static int wlc_scb_txfifo(wlc_info_t *wlc, struct scb *scb, void *sdu, uint *fifo);
#ifdef TXQ_MUX
static uint wlc_txq_immediate_output(void *ctx, uint ac, uint request_time, struct spktq *output_q);
#else
static int wlc_scb_peek_txfifo(wlc_info_t *wlc, struct scb *scb, void *sdu, uint *fifo);
#endif /* TXQ_MUX */

#endif 

#ifdef WL_TX_STALL
static void wlc_tx_status_report_error(wlc_info_t * wlc);

#ifdef HEALTH_CHECK
static int wlc_hchk_tx_stall_check(uint8 *buffer, uint16 length, void *context,
	int16 *bytes_written);
#endif

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int wlc_tx_status_tx_activity_dump(wlc_info_t *wlc, struct bcmstrbuf *b);
static void wlc_tx_status_dump_counters(wlc_tx_stall_counters_t * counters,
	struct bcmstrbuf *b, const char * prefix);

static const char * tx_sts_string[TX_STS_RSN_MAX] =  {
	TX_STS_REASON_STRINGS
};
#else
#define wlc_tx_status_dump_counters(a, b, c)
#endif

#endif /* WL_TX_STALL */

#ifdef BCMDBG_ERR
static const char *fifo_names[] = { "AC_BK", "AC_BE", "AC_VI", "AC_VO", "BCMC", "ATIM" };
#endif

static void wlc_pdu_txhdr(wlc_info_t *wlc, void *p, struct scb *scb, wlc_txh_info_t *txh_info);
static void wlc_txfast(wlc_info_t *wlc, struct scb *scb, void *sdu, uint pktlen,
	wlc_key_t *key, const wlc_key_info_t *key_info);
static void wlc_update_txpktfail_stats(wlc_info_t *wlc, uint pkt_len, uint8 prio);
static void wlc_dofrag(wlc_info_t *wlc, void *p, uint frag, uint nfrags,
	uint next_payload_len, struct scb *scb, bool is8021x,
	uint fifo, wlc_key_t *key, const wlc_key_info_t *key_info,
	uint8 prio, uint frag_length);
static struct dot11_header *wlc_80211hdr(wlc_info_t *wlc, void *p,
	struct scb *scb, bool MoreFrag, const wlc_key_info_t *key_info,
	uint8 prio, uint16 *pushlen, uint fifo);
static void* wlc_allocfrag(osl_t *osh, void *sdu, uint offset, uint headroom, uint frag_length,
	uint tailroom);
#if defined(BCMPCIEDEV) && defined(BCMFRAGPOOL)
static void * wlc_allocfrag_txfrag(osl_t *osh, void *sdu, uint offset,
	uint frag_length, bool lastfrag);
#endif
static INLINE uint wlc_frag(wlc_info_t *wlc, struct scb *scb, uint8 ac, uint plen, uint *flen);
static INLINE void wlc_d11ac_hdrs_rts_cts(struct scb *scb /* [in] */,
	wlc_bsscfg_t *bsscfg /* [in] */, ratespec_t rspec, bool use_rts, bool use_cts,
	ratespec_t *rts_rspec /* [in/out] */, d11actxh_rate_t *rate_hdr /* [in/out] */,
	uint8 *rts_preamble_type /* [out] */, uint16 *mcl /* [in/out] */);
static uint wlc_calc_frame_len(wlc_info_t *wlc, ratespec_t rate, uint8 preamble_type, uint dur);
static void wlc_low_txq_scb_flush(wlc_info_t *wlc, wlc_txq_info_t *qi, struct scb *remove);
static struct scb* wlc_recover_pkt_scb(wlc_info_t *wlc, void *pkt);

static void wlc_detach_queue(wlc_info_t *wlc, wlc_txq_info_t *qi);
static void wlc_attach_queue(wlc_info_t *wlc, wlc_txq_info_t *qi);
static void wlc_txq_free_pkt(wlc_info_t *wlc, void *pkt, uint16 *time_adj);
static void wlc_txq_freed_pkt_time(wlc_info_t *wlc, void *pkt, uint16 *time_adj);
static void wlc_low_txq_account(wlc_info_t *wlc, txq_t *low_txq, uint prec,
	uint8 flag, uint16 *time_adj);
static void wlc_low_txq_sync(wlc_info_t *wlc, txq_t* low_txq, uint fifo_bitmap,
	uint8 flag);
static void wlc_tx_pktpend_check(wlc_info_t *wlc, uint *fifo_bitmap);

#ifdef STA
static void wlc_pm_tx_upd(wlc_info_t *wlc, struct scb *scb, void *pkt, bool ps0ok, uint fifo);
#endif /* STA */

#ifdef WLAMPDU
static uint16 wlc_compute_ampdu_mpdu_dur(wlc_info_t *wlc, uint8 bandunit,
	ratespec_t rate);
#endif /* WLAMPDU */


static int wlc_txq_scb_init(void *ctx, struct scb *scb);
static void wlc_txq_scb_deinit(void *context, struct scb *scb);

static void wlc_txq_enq(void *ctx, struct scb *scb, void *sdu, uint prec);
static uint wlc_txq_txpktcnt(void *ctx);

static uint16 wlc_acphy_txctl0_calc(wlc_info_t *wlc, ratespec_t rspec, uint8 preamble);
static uint16 wlc_acphy_txctl1_calc(wlc_info_t *wlc, ratespec_t rspec,
	int8 lpc_offset, uint8 txpwr, bool is_mu);
static uint16 wlc_acphy_txctl2_calc(wlc_info_t *wlc, ratespec_t rspec, uint8 txbf_uidx);
#ifdef WL11AX
static INLINE uint16 wlc_apply_he_tx_policy(wlc_info_t *wlc, struct scb *scb, uint16 type);
#endif /* WL11AX */
#ifdef WL_FRAGDUR
static bool BCMFASTPATH wlc_do_fragdur(wlc_info_t *wlc, struct scb *scb, void *pkt);
static bool BCMFASTPATH wlc_txagg_state_changed(wlc_info_t *wlc, struct scb *scb, void *pkt);
#endif /* WL_FRAGDUR */

/* txmod */
static const txmod_fns_t BCMATTACHDATA(txq_txmod_fns) = {
	wlc_txq_enq,
	wlc_txq_txpktcnt,
	NULL,
	NULL
};

/* enqueue the packet to delete queue */
static void
wlc_txq_delq_enq(void *ctx, void *pkt)
{
	txq_info_t *txqi = ctx;

	spktenq(txqi->delq, pkt);
}

/* flush the delete queue/free all the packet */
static void
wlc_txq_delq_flush(void *ctx)
{
	txq_info_t *txqi = ctx;

	spktqflush(txqi->osh, txqi->delq, TRUE);
}

/**
 * pktq filter function to set pkttag::scb pointer to NULL when
 * the SCB the packet is associated with is being deleted i.e. pkttag::scb == pkts.
 */
static pktq_filter_result_t
wlc_delq_scb_free_filter(void *ctx, void *pkt)
{
	struct scb *scb = ctx;

	if (WLPKTTAGSCBGET(pkt) == scb) {
		WLPKTTAGSCBSET(pkt, NULL);
	}

	return PKT_FILTER_NOACTION;
}

/** scb free callback - invoked when scn is deleted and nullify packets' scb pointer
 * in the pkttag for all packets in the delete queue that belong to the scb.
 */
static void
wlc_delq_scb_free(wlc_info_t *wlc, struct scb *scb)
{
	txq_info_t *txqi = wlc->txqi;

	spktqfilter(txqi->delq, wlc_delq_scb_free_filter, scb,
	           NULL, NULL, wlc_txq_delq_flush, txqi);
}

/** pktq filter returns PKT_FILTER_DELETE if pkt idx matches with bsscfg idx value */
static pktq_filter_result_t
wlc_ifpkt_chk_cb(void *ctx, void *pkt)
{
	int idx = (int)(uintptr)ctx;

	return (WLPKTTAGBSSCFGGET(pkt) == idx) ? PKT_FILTER_DELETE : PKT_FILTER_NOACTION;
}

/**
 * delete packets that belong to a particular bsscfg in a packet queue.
 */
void
wlc_txq_pktq_cfg_filter(wlc_info_t *wlc, struct pktq *pq, wlc_bsscfg_t *cfg)
{
	txq_info_t *txqi = wlc->txqi;

	pktq_filter(pq, wlc_ifpkt_chk_cb, (void *)(uintptr)WLC_BSSCFG_IDX(cfg),
	            wlc_txq_delq_enq, txqi, wlc_txq_delq_flush, txqi);
}

/**
 * pktq filter function to delete pkts associated with an SCB
 */
static pktq_filter_result_t
wlc_pktq_scb_free_filter(void* ctx, void* pkt)
{
	struct scb *scb = (struct scb *)ctx;
	return (WLPKTTAGSCBGET(pkt) == scb) ? PKT_FILTER_DELETE: PKT_FILTER_NOACTION;
}

/** free all pkts asscoated with the given scb on a pktq */
void
wlc_txq_pktq_scb_filter(wlc_info_t *wlc, struct pktq *pq, struct scb *scb)
{
	txq_info_t *txqi = wlc->txqi;

	pktq_filter(pq, wlc_pktq_scb_free_filter, scb,
	            wlc_txq_delq_enq, txqi, wlc_txq_delq_flush, txqi);
}

/** generic pktq filter - packets passing the pktq filter will be placed
 * in the delete queue first and freed later once all packets are processed.
 */
void
wlc_txq_pktq_filter(wlc_info_t *wlc, struct pktq *pq, pktq_filter_t fltr, void *ctx)
{
	txq_info_t *txqi = wlc->txqi;

	pktq_filter(pq, fltr, ctx,
	            wlc_txq_delq_enq, txqi, wlc_txq_delq_flush, txqi);
}

void
wlc_low_txq_pktq_filter(wlc_info_t *wlc, struct spktq *spq, pktq_filter_t fltr, void *ctx)
{
	txq_info_t *txqi = wlc->txqi;

	spktq_filter(spq, fltr, ctx,
		wlc_txq_delq_enq, txqi, wlc_txq_delq_flush, txqi);
}

void
wlc_txq_pktq_pfilter(wlc_info_t *wlc, struct pktq *pq, int prec, pktq_filter_t fltr, void *ctx)
{
	txq_info_t *txqi = wlc->txqi;

	pktq_pfilter(pq, prec, fltr, ctx,
	             wlc_txq_delq_enq, txqi, wlc_txq_delq_flush, txqi);
}

/* safe pktq flush */
void
wlc_txq_pktq_flush(wlc_info_t *wlc, struct pktq *pq)
{
	pktq_flush(wlc->osh, pq, TRUE);
}

void
wlc_txq_pktq_pflush(wlc_info_t *wlc, struct pktq *pq, int prec)
{
	pktq_pflush(wlc->osh, pq, prec, TRUE);
}

/**
 * Clean up and fixups, called from from wlc_txfifo()
 * This routine is called for each tx dequeue,
 * the #ifdefs make sure we have the shortest possble code path
 */
static void BCMFASTPATH
txq_hw_hdr_fixup(wlc_info_t *wlc, void *p, dot11_header_t *d11h, uint fifo)
{
	wlc_pkttag_t *pkttag = WLPKTTAG(p);

#ifdef STA
	uint16 fc;
#endif /* STA */

#if defined(STA)
	struct scb *scb = WLPKTTAGSCBGET(p);
	wlc_bsscfg_t *cfg;
#endif

	ASSERT(pkttag);
	BCM_REFERENCE(pkttag);

#if defined(STA)
	ASSERT(scb);
	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);
	BCM_REFERENCE(cfg);
#endif

#ifdef STA
	/* If the assert below fires this packet is missing the d11 header */
	ASSERT(d11h);
	fc = ltoh16(d11h->fc);
	if (BSSCFG_STA(cfg) && !(pkttag->flags3 & WLF3_NO_PMCHANGE) &&
		!(FC_TYPE(fc) == FC_TYPE_CTL && FC_SUBTYPE_ANY_PSPOLL(FC_SUBTYPE(fc))) &&
		!(FC_TYPE(fc) == FC_TYPE_DATA && FC_SUBTYPE_ANY_NULL(FC_SUBTYPE(fc)) &&
		(cfg->pm->PM == PM_MAX ? !FC_SUBTYPE_ANY_QOS(fc) : TRUE))) {
		if (cfg->pm->PMenabled == TRUE) {
			wlc_pm_tx_upd(wlc, scb, p, TRUE, fifo);
		}
		if (cfg->pm->adv_ps_poll == TRUE) {
			wlc_adv_pspoll_upd(wlc, scb, p, FALSE, fifo);
		}
	}
#endif /* STA */

	PKTDBG_TRACE(wlc->osh, p, PKTLIST_TXFIFO);

#ifdef WLPKTDLYSTAT
	/* Save the packet enqueue time (for latency calculations) */
	if (pkttag->shared.enqtime == 0) {
		pkttag->shared.enqtime = WLC_GET_CURR_TIME(wlc);
	}
#endif


	/* Packet has been accepted for transmission, so clear scb pointer */
} /* txq_hw_hdr_fixup */

/**
 * Free SCB specific pkts from txq and low_txq. While
 * freeing pkts low_txq special handling is required to
 * reduce the buffered time for low_txq accordingly.
 * Clearing the txq pkts is done using wlc_pktq_scb_free
 * instead.
 */
void
wlc_txq_scb_free(wlc_info_t *wlc, struct scb *from_scb)
{
	wlc_txq_info_t *qi = NULL;

	for (qi = wlc->tx_queues; qi != NULL; qi = qi->next) {
		/* Free the packets for this scb in low txq */
		wlc_low_txq_scb_flush(wlc, qi, from_scb);
		wlc_pktq_scb_free(wlc, &qi->txq, from_scb);
	}
}

/* Place holder for tx datapath functions
 * Refer to RB http://wlan-rb.sj.broadcom.com/r/18439/ and
 * TWIKI http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/WlDriverTxQueuingUpdate2013
 * for details on datapath restructuring work
 * These functions will be moved in stages from wlc.[ch]
 */

static void BCMFASTPATH
txq_stop_clr(txq_t *txq, uint fifo_idx)
{
	txq->fifo_state[fifo_idx].flags &= ~TXQ_STOPPED;
}

static int BCMFASTPATH
txq_stopped(txq_t *txq, uint fifo_idx)
{
	return (txq->fifo_state[fifo_idx].flags & TXQ_STOPPED) != 0;
}

uint8
txq_stopped_map(txq_t *txq)
{
	uint fifo_idx;
	uint8 stop_map = 0;

	for (fifo_idx = 0; fifo_idx < txq->nfifo; fifo_idx++) {
		if (txq_stopped(txq, fifo_idx)) {
			stop_map |= 1 << fifo_idx;
		}
	}

	return stop_map;
}

bool BCMFASTPATH
wlc_low_txq_empty(txq_t *txq)
{
	uint i;

	ASSERT(txq);

	for (i = 0; i < txq->nfifo; i++) {
		if (spktq_n_pkts(&txq->swq[i]) > 0) {
			return FALSE;
		}
	}

	return TRUE;
}

int
wlc_txq_buffered_time(txq_t *txq, uint fifo_idx)
{
	return txq->fifo_state[fifo_idx].buffered;
}

static int BCMFASTPATH
txq_space(txq_t *txq, uint fifo_idx)
{
	return txq->fifo_state[fifo_idx].highwater - txq->fifo_state[fifo_idx].buffered;
}

static int BCMFASTPATH
txq_inc(txq_t *txq, uint fifo_idx, int inc_time)
{
	int rem;

	rem = (txq->fifo_state[fifo_idx].buffered += inc_time);

	if (rem < 0) {
		WL_ERROR(("%s rem: %d fifo:%d\n", __FUNCTION__, rem, fifo_idx));
	}

	ASSERT(rem >= 0);

	return rem;
}

static int BCMFASTPATH
txq_dec(txq_t *txq, uint fifo_idx, int dec_time)
{
	int rem;

	rem = (txq->fifo_state[fifo_idx].buffered -= dec_time);

	if (rem < 0) {
		WL_ERROR(("%s rem: %d fifo:%d dec_time:%d \n",
			__FUNCTION__, rem, fifo_idx, dec_time));
	}
	ASSERT(rem >= 0);

	return rem;
}

extern int
wlc_low_txq_buffered_inc(txq_t *txq, uint fifo_idx, int inc_time)
{
	return txq_inc(txq, fifo_idx, inc_time);
}

extern int
wlc_low_txq_buffered_dec(txq_t *txq, uint fifo_idx, int dec_time)
{
	return txq_dec(txq, fifo_idx, dec_time);
}

static void BCMFASTPATH
txq_stop_set(txq_t *txq, uint fifo_idx)
{
	txq->fifo_state[fifo_idx].flags |= TXQ_STOPPED;
}

static void BCMFASTPATH
txq_hw_stop_set(txq_t *txq, uint fifo_idx)
{
	txq->fifo_state[fifo_idx].flags |= TXQ_HW_STOPPED;
}

static void BCMFASTPATH
txq_hw_stop_clr(txq_t *txq, uint fifo_idx)
{
	txq->fifo_state[fifo_idx].flags &= ~TXQ_HW_STOPPED;
}

static void BCMFASTPATH
txq_hw_hold_set(txq_t *txq, uint fifo_idx)
{
	txq->fifo_state[fifo_idx].flags |= TXQ_HW_HOLD;
}

static void BCMFASTPATH
txq_hw_hold_clr(txq_t *txq, uint fifo_idx)
{
	txq->fifo_state[fifo_idx].flags &= ~TXQ_HW_HOLD;
}

int BCMFASTPATH
txq_hw_stopped(txq_t *txq, uint fifo_idx)
{
	return (txq->fifo_state[fifo_idx].flags & TXQ_HW_MASK) != 0;
}

#define TXQ_DMA_NPKT       8 /* Initial burst of packets to the DMA, each one committed */
#define TXQ_DMA_NPKT_SKP   8 /* Commit every (TXQ_DMA_NPKT_SKP +1)afterwards */
/**
 * Drains MPDUs from the caller supplied queue and feeds them to the d11 core
 * using DMA.
 */
void BCMFASTPATH
txq_hw_fill(txq_info_t *txqi, txq_t *txq, uint fifo_idx)
{
	void *p;
	struct spktq *swq;
	osl_t *osh;
	wlc_info_t *wlc = txqi->wlc;
	wlc_pub_t *pub = txqi->pub;
	bool commit = FALSE;
	bool delayed_commit = TRUE;
	uint npkt = 0;
	uint iter = 0;
#ifdef BCMLTECOEX
	uint16 fc;
#endif
#ifdef BCM_DMA_CT
	bool firstentry = TRUE;
#endif /* BCM_DMA_CT */
#ifdef WLAMPDU
	bool sw_ampdu;
#endif /* WLAMPDU */
	wlc_bsscfg_t *bsscfg;
#ifdef WLCNT
	struct txq_fifo_cnt *cnt;
#endif

#ifdef WLAMPDU
	sw_ampdu = (AMPDU_ENAB(pub) && AMPDU_HOST_ENAB(pub));
	if (sw_ampdu) {
		/* sw_ampdu has its own method of determining when to commit */
		delayed_commit = FALSE;
	}
#endif /* WLAMPDU */
	swq = &txq->swq[fifo_idx];
	osh = txqi->osh;

	BCM_REFERENCE(osh);
	BCM_REFERENCE(pub);
	BCM_REFERENCE(bsscfg);

	if (!PIO_ENAB(pub)) {
		hnddma_t *di = WLC_HW_DI(wlc, fifo_idx);
		uint queue;
#ifdef BCM_DMA_CT
		uint prev_queue;
		bool bulk_commit = FALSE;

		queue = prev_queue = WLC_HW_NFIFO_INUSE(wlc);
		if (BCM_DMA_CT_ENAB(wlc)) {
			/* in CutThru dma do not update dma for every pkt. */
			commit = FALSE;
		}
#endif /* BCM_DMA_CT */
		while ((p = spktq_deq(swq))) {
			uint ndesc;
			struct scb *scb = WLPKTTAGSCBGET(p);
			void *txh;
			dot11_header_t *d11h = NULL;
			uint hdrSize;
			uint16 TxFrameID;
			uint16 MacTxControlLow;
			uint8 *pkt_data = PKTDATA(wlc->osh, p);
			uint tsoHdrSize = 0;
#ifdef WLC_MACDBG_FRAMEID_TRACE
			uint8 *tsoHdrPtr;
			uint8 epoch = 0;
#endif
			wlc_pkttag_t *pkttag;

			/* calc the number of descriptors needed to queue this frame */

			/* for unfragmented packets, count the number of packet buffer
			 * segments, each being a contiguous virtual address range.
			 */

			bsscfg = wlc->bsscfg[WLPKTTAGBSSCFGGET(p)];
			ASSERT(bsscfg != NULL);

			ndesc = pktsegcnt(osh, p);

			if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(osh, p))
				ndesc += 2;

			if (wlc->dma_avoidance_war)
				ndesc *= 2;

			if (D11REV_LT(wlc->pub->corerev, 40)) {
				hdrSize = sizeof(d11txh_t);
				txh = (void*)pkt_data;
				d11h = (dot11_header_t*)((uint8*)txh + hdrSize + D11_PHY_HDR_LEN);
				TxFrameID = ltoh16(((d11txh_t*)txh)->TxFrameID);
				MacTxControlLow = ((d11txh_t*)txh)->MacTxControlLow;
			} else {
#ifdef WLTOEHW
				tsoHdrSize = WLC_TSO_HDR_LEN(wlc, (d11ac_tso_t*)pkt_data);
#ifdef WLC_MACDBG_FRAMEID_TRACE
				tsoHdrPtr = (void*)((tsoHdrSize != 0) ? pkt_data : NULL);
				epoch = (tsoHdrPtr[2] & TOE_F2_EPOCH) ? 1 : 0;
#endif /* WLC_MACDBG_FRAMEID_TRACE */
#endif /* WLTOEHW */
				hdrSize = sizeof(d11actxh_t);
				txh = (void*)(pkt_data + tsoHdrSize);
				d11h = (dot11_header_t*)((uint8*)txh + hdrSize);
				TxFrameID = ltoh16(((d11actxh_t*)txh)->PktInfo.TxFrameID);
				MacTxControlLow = ((d11actxh_t*)txh)->PktInfo.MacTxControlLow;
			}

			queue = WLC_TXFID_GET_QUEUE(TxFrameID);
			ASSERT(queue < NFIFO_EXT);
			di = WLC_HW_DI(wlc, queue);

#ifdef BCM_DMA_CT
			if (BCM_DMA_CT_ENAB(wlc)) {
				/* take care of queue change */
				if ((queue != prev_queue) &&
					(prev_queue != WLC_HW_NFIFO_INUSE(wlc))) {
						dma_txcommit(WLC_HW_DI(wlc, prev_queue));
						dma_txcommit(WLC_HW_AQM_DI(wlc, prev_queue));
						bulk_commit = FALSE;
				}
			}
#endif /* BCM_DMA_CT */
			if (txq_hw_stopped(txq, queue)) {
				/* return the packet to the queue */
				spktq_enq_head(swq, p);
				break;
			}
#ifdef WLCNT
			cnt = &txq->fifo_cnt[queue];
			WLCNTINCR(cnt->hw_service);
#endif

			/* check for sufficient dma resources */
			if ((uint)TXAVAIL(wlc, queue) <= ndesc) {
				/* this pkt will not fit on the dma ring */

				/* mark this hw fifo as stopped */
				txq_hw_stop_set(txq, queue);
				WLCNTINCR(cnt->hw_stops);

				/* return the packet to the queue */
				spktq_enq_head(swq, p);
				break;
			}

			pkttag = WLPKTTAG(p);

			/* We are done with WLF_FIFOPKT regardless */
			pkttag->flags &= ~WLF_FIFOPKT;

			/* We are done with WLF_TXCMISS regardless */
			pkttag->flags &= ~WLF_TXCMISS;

			/* Apply misc fixups for state and powersave */
			txq_hw_hdr_fixup(wlc, p, d11h, queue);
			if ((pkttag->flags & WLF_EXPTIME)) {
				uint32 Tstamp = pkttag->u.exptime;
				if (D11REV_LT(wlc->pub->corerev, 40)) {
					MacTxControlLow |= htol16(TXC_LIFETIME);
					((d11txh_t*)txh)->MacTxControlLow = MacTxControlLow;
					((d11txh_t*)txh)->TstampLow |= htol16((uint16)Tstamp);
					((d11txh_t*)txh)->TstampHigh =
						htol16((uint16)(Tstamp >> 16));
				} else {
					MacTxControlLow |= htol16(D11AC_TXC_AGING);
					((d11actxh_t*)txh)->PktInfo.MacTxControlLow =
						MacTxControlLow;
					((d11actxh_t*)txh)->PktInfo.Tstamp =
						htol16((uint16)(Tstamp >> D11AC_TSTAMP_SHIFT));
				}
			}

#ifdef BCMLTECOEX
			/* activate coex signal if 2G tx-active co-ex
			 * enabled and we're about to send packets
			 */
			if BCMLTECOEX_ENAB(wlc->pub) {
				fc = ltoh16(d11h->fc);
				if (wlc->hw->btc->bt_shm_addr && BAND_2G(wlc->band->bandtype) &&
						FC_TYPE(fc) == FC_TYPE_DATA &&
						!FC_SUBTYPE_ANY_NULL(FC_SUBTYPE(fc))) {
					uint16 ltecx_flags;
					ltecx_flags = wlc_bmac_read_shm
						(wlc->hw, M_LTECX_FLAGS(wlc->hw));
					if ((ltecx_flags & (1 << C_LTECX_FLAGS_TXIND)) == 0) {
						ltecx_flags |= (1 << C_LTECX_FLAGS_TXIND);
						wlc_bmac_write_shm
							(wlc->hw, M_LTECX_FLAGS(wlc->hw),
							 ltecx_flags);
					}
				}
			}
#endif /* BCMLTECOEX */

			/* When a Broadcast/Multicast frame is being committed to the BCMC
			 * fifo via DMA (NOT PIO), update ucode or BSS info as appropriate.
			 */
			if (queue == TX_BCMC_FIFO) {
#if defined(MBSS)
				/* For MBSS mode, keep track of the
				 * last bcmc FID in the bsscfg info.
				 * A snapshot of the FIDs for each BSS
				 * will be committed to shared memory at DTIM.
				 */
				if (MBSS_ENAB(pub)) {
					bsscfg->bcmc_fid = TxFrameID;
					wlc_mbss_txq_update_bcmc_counters(wlc, bsscfg);
				} else
#endif /* MBSS */
				{
					/*
					 * Commit BCMC sequence number in
					 * the SHM frame ID location
					*/
					wlc_bmac_write_shm(wlc->hw, M_BCMC_FID(wlc), TxFrameID);
				}
			}

#ifdef BCM_DMA_CT
			if (BCM_DMA_CT_ENAB(wlc) && firstentry &&
				wlc->cfg->pm->PM != PM_OFF && (D11REV_IS(wlc->pub->corerev, 65))) {
				if (!(wlc->hw->maccontrol & MCTL_WAKE) &&
				    !(wlc->hw->wake_override & WLC_WAKE_OVERRIDE_CLKCTL)) {
				  wlc_ucode_wake_override_set(wlc->hw, WLC_WAKE_OVERRIDE_CLKCTL);
				} else {
					OSL_DELAY(10);
				}
				firstentry = FALSE;
			}
#endif /* BCM_DMA_CT */
			if (((D11REV_IS(wlc->pub->corerev, 41)) ||
				(D11REV_IS(wlc->pub->corerev, 44)) ||
				(D11REV_IS(wlc->pub->corerev, 59))) &&
				!(wlc->user_wake_req & WLC_USER_WAKE_REQ_TX)) {
				mboolset(wlc->user_wake_req, WLC_USER_WAKE_REQ_TX);
				wlc_set_wake_ctrl(wlc);
			}
#ifndef WL_MU_TX
			if (fifo_idx != WLC_TXFID_GET_QUEUE(TxFrameID)) {
				WL_ERROR(("%s: FIFO (%d) and FID (%d) mismatch\n",
					__FUNCTION__, fifo_idx,
					WLC_TXFID_GET_QUEUE(TxFrameID)));
			}
#endif
#ifdef WLAMPDU
			/* Toggle commit bit on last AMPDU when using software/host aggregation */
			if (sw_ampdu) {
				uint16 mpdu_type =
				        (((ltoh16(MacTxControlLow)) &
				          TXC_AMPDU_MASK) >> TXC_AMPDU_SHIFT);
				commit = ((mpdu_type == TXC_AMPDU_LAST) ||
				          (mpdu_type == TXC_AMPDU_NONE));
			}
#endif /* WLAMPDU */

#if defined(BCMDBG) || defined(BCMDBG_ASSERT)
			if (D11REV_GE(wlc->pub->corerev, 40)) {
				/* DON'T PUT ANYTHING THERE */
				/* ucode is going to report the value
				 * plus modification back to the host
				 * in txstatus.
				 */
				ASSERT(((d11actxh_t*)txh)->PktInfo.TxStatus == 0);
			}
#endif
#ifdef WLC_MACDBG_FRAMEID_TRACE
			wlc_macdbg_frameid_trace_pkt(wlc->macdbg, p,
				(uint8)fifo_idx, htol16(TxFrameID), epoch);
#endif

			if (wlc_bmac_dma_txfast(wlc, queue, p, commit) < 0) {
				/* the dma did not have enough room to take the pkt */

				/* mark this hw fifo as stopped */
				txq_hw_stop_set(txq, queue);
				WLCNTINCR(cnt->hw_stops);

				/* return the packet to the queue */
				WLPKTTAGSCBSET(p, scb);
				spktq_enq_head(swq, p);
				break;
			}
#ifdef BCM_DMA_CT
			if (BCM_DMA_CT_ENAB(wlc)) {
				prev_queue = queue;
				bulk_commit = TRUE;
			}
#endif /* BCM_DMA_CT */

			if (delayed_commit)
			{
				npkt++;
				iter++;
				/*
				 * Commit the first TXQ_DMA_NPKT packets
				 * and then every (TXQ_DMA_NPKT_SKP + 1)-th packet
				 */
				if ((npkt > TXQ_DMA_NPKT) && (iter > TXQ_DMA_NPKT_SKP))
				{
					iter = 0;
					dma_txcommit(di);
				}
			}

			PKTDBG_TRACE(osh, p, PKTLIST_DMAQ);

			/* WES: Check if this is still needed. HW folks should be in on this. */
			if ((BCM4331_CHIP_ID == CHIPID(wlc->pub->sih->chip) ||
				BCM4350_CHIP(wlc->pub->sih->chip) ||
				BCM4352_CHIP_ID == CHIPID(wlc->pub->sih->chip) ||
				BCM43602_CHIP(wlc->pub->sih->chip) ||
				BCM4360_CHIP_ID == CHIPID(wlc->pub->sih->chip)) &&
				(bsscfg->pm->PM))
			{
				(void)R_REG(wlc->osh, &wlc->regs->maccontrol);
			}
		}

		/*
		 * Do the commit work for packets posted to the DMA.
		 * This call updates the DMA registers for the additional descriptors
		 * posted to the ring.
		 */
		if (iter > 0) {
			ASSERT(delayed_commit);
			/*
			 * Software AMPDU takes care of the commit in the code above,
			 * delay commit for everything else.
			 */
			dma_txcommit(di);
		}
#if defined(BCM_DMA_CT)
		if (BCM_DMA_CT_ENAB(wlc) && bulk_commit) {
			ASSERT(queue < WLC_HW_NFIFO_INUSE(wlc));
			dma_txcommit(WLC_HW_DI(wlc, queue));
			dma_txcommit(WLC_HW_AQM_DI(wlc, queue));
		}
#endif /* BCM_DMA_CT */

	} else {
#ifdef WLCNT
		cnt = &txq->fifo_cnt[fifo_idx];
#endif
		while ((p = spktq_deq(swq))) {
			uint nbytes = pkttotlen(osh, p);
			BCM_REFERENCE(nbytes);

			/* return if insufficient pio resources */
			if (!wlc_pio_txavailable(WLC_HW_PIO(wlc, fifo_idx), nbytes, 1)) {
				/* mark this hw fifo as stopped */
				txq_hw_stop_set(txq, fifo_idx);
#ifdef WLCNT
				WLCNTINCR(cnt->hw_stops);
#endif
				break;
			}

			/* Following code based on original wlc_txfifo() */
			wlc_pio_tx(WLC_HW_PIO(wlc, fifo_idx), p);
		}
	}
}

static int
wlc_txq_down(void *ctx)
{
	txq_info_t *txqi = (txq_info_t*)ctx;
	txq_t *txq;

	for (txq = txqi->txq_list; txq != NULL; txq = txq->next) {
		wlc_low_txq_flush(txqi, txq);
	}

	wlc_txq_delq_flush(txqi);

	return 0;
}

static void
wlc_low_txq_set_watermark(txq_t *txq, int highwater, int lowwater)
{
	uint i;

	for (i = 0; i < txq->nfifo; i++) {
		if (highwater >= 0) {
			txq->fifo_state[i].highwater = highwater;
		}
		if (lowwater >= 0) {
			txq->fifo_state[i].lowwater = lowwater;
		}
	}
}

static void
txq_init(txq_t *txq, uint nfifo, txq_supply_fn_t supply_fn, void *supply_ctx, int high,
	int low)
{
	uint i;

	txq->next = NULL;
	txq->supply = supply_fn;
	txq->supply_ctx = supply_ctx;
	txq->nfifo = nfifo;

	for (i = 0; i < nfifo; i++) {
		spktq_init(&txq->swq[i], PKTQ_LEN_MAX);
	}

	/*
	 * If TXQ_MUX is not defined the watermarks will be reset to
	 * to packet values based on txmaxpkts when
	 * the AMPDU module finishes initialization
	 */
	wlc_low_txq_set_watermark(txq, high, low);
}

txq_t*
wlc_low_txq_alloc(txq_info_t *txqi, txq_supply_fn_t supply_fn, void *supply_ctx,
                  uint nfifo, int high, int low)
{
	txq_t *txq;

	/* The fifos managed by the low txq can never exceed the count of hw fifos */
	ASSERT(nfifo <= NFIFO_EXT);

	/* Allocate private state struct */
	if ((txq = MALLOCZ(txqi->osh, sizeof(txq_t))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
		          txqi->pub->unit, __FUNCTION__, MALLOCED(txqi->osh)));
		return NULL;
	}

	/* Allocate software queue */
	if ((txq->swq = MALLOCZ(txqi->osh, (sizeof(struct spktq) * nfifo))) == NULL) {
		MFREE(txqi->osh, txq, sizeof(txq_t));
		WL_ERROR(("wl%d: %s: MALLOC software queue failed, malloced %d bytes\n",
		          txqi->pub->unit, __FUNCTION__, MALLOCED(txqi->osh)));
		return NULL;
	}

	txq_init(txq, nfifo, supply_fn, supply_ctx, high, low);

	/* add new queue to the list */
	txq->next = txqi->txq_list;
	txqi->txq_list = txq;

	return txq;
}

void
wlc_low_txq_free(txq_info_t *txqi, txq_t* txq)
{
	txq_t *p;

	if (txq == NULL) {
		return;
	}

	wlc_low_txq_flush(txqi, txq);

	/* remove the queue from the linked list */
	p = txqi->txq_list;
	if (p == txq)
		txqi->txq_list = txq->next;
	else {
		while (p != NULL && p->next != txq)
			p = p->next;

		if (p != NULL) {
			p->next = txq->next;
		} else {
			/* assert that we found txq before getting to the end of the list */
			WL_ERROR(("%s: did not find txq %p\n",
				__FUNCTION__, OSL_OBFUSCATE_BUF(txq)));
			ASSERT(p != NULL);
		}
	}
	txq->next = NULL;

	MFREE(txqi->osh, txq->swq, (sizeof(*txq->swq) * txq->nfifo));
	MFREE(txqi->osh, txq, sizeof(*txq));
}

/**
 * Context structure used by wlc_low_txq_filter() while filtering a pktq
 */
struct wlc_low_txq_filter_info {
	struct scb* scb;  /**< scb who's packets are being deleted */
	uint count;       /**< total num packets deleted */
	uint time;        /**< total tx time exstimate of packets deleted */
};

/**
 * pktq filter function to delete pkts associated with an SCB,
 * keeping track of pkt count and time.
 * Used by wlc_low_txq_scb_flush().
 */
static pktq_filter_result_t
wlc_low_txq_filter(void* ctx, void* pkt)
{
	struct wlc_low_txq_filter_info *info = (struct wlc_low_txq_filter_info *)ctx;
	pktq_filter_result_t ret;

	if (WLPKTTAGSCBGET(pkt) == info->scb) {
		info->count++;
		info->time += WLPKTTIME(pkt);
		ret = PKT_FILTER_DELETE;
	} else {
		ret = PKT_FILTER_NOACTION;
	}

	return ret;
}

/**
 * Clean up function for the low_txq. Frees all packets belonging to the given
 * scb 'remove' in the low_txq.
 */
static void
wlc_low_txq_scb_flush(wlc_info_t *wlc, wlc_txq_info_t *qi, struct scb *remove)
{
	uint fifo_idx;
	struct wlc_low_txq_filter_info info;
	struct spktq *swq;

	info.scb = remove;

	/* filter the queue by fifo_idx so that per-fifo bookkeeping can be updated */
	for (fifo_idx = 0; fifo_idx < qi->low_txq->nfifo; fifo_idx++) {
		info.count = 0;
		info.time = 0;

		swq = &qi->low_txq->swq[fifo_idx];

		/* filter this particular fifo_idx of pkts belonging to scb 'remove' */
		wlc_low_txq_pktq_filter(wlc, swq, wlc_low_txq_filter, &info);

		/* Update the bookkeeping of low_txq buffered time for deleted packets */
#ifdef TXQ_MUX
		wlc_low_txq_buffered_dec(qi->low_txq, fifo_idx, info.time);
#else
		/* for non-MUX, time is just 1 for each packet, use 'count' instead of 'time' */
		wlc_low_txq_buffered_dec(qi->low_txq, fifo_idx, info.count);
#endif
		/* Update the bookkeeping of TXPKTPEND if qi is active Q */
		if (wlc->active_queue == qi) {
			TXPKTPENDDEC(wlc, fifo_idx, (int16)info.count);
		}
	}
}

void
wlc_low_txq_flush(txq_info_t *txqi, txq_t* txq)
{
	uint fifo_idx;

	for (fifo_idx = 0; fifo_idx < txq->nfifo; fifo_idx++) {
		/* flush any pkts in the software queues */
		spktq_flush(txqi->osh, &txq->swq[fifo_idx], TRUE);
	}
	for (fifo_idx = 0; fifo_idx < txq->nfifo; fifo_idx++) {

		/* Flushing the low txq assumes there will be no more completes from the
		 * lower level DMA/PIO/RPC fifos.
		 * Clean up the buffered pkt time with the flush.
		 */
		txq->fifo_state[fifo_idx].buffered = 0;

		/* Clear any 'stopped' flags since there are no pkts */
		txq->fifo_state[fifo_idx].flags &= ~(TXQ_STOPPED | TXQ_HW_MASK);
	}
	ASSERT(wlc_low_txq_empty(txq));
}

#ifdef TXQ_MUX
/**
 * @brief Tx path MUX output function
 *
 * Tx path MUX output function. This function is registerd via a call to wlc_mux_add_member() by
 * wlc_txq_alloc(). This function will supply the packets to the caller from packets held
 * in the TxQ's immediate packet path.
 */
static uint BCMFASTPATH
wlc_txq_immediate_output(void *ctx, uint ac, uint request_time, struct spktq *output_q)
{
	wlc_txq_info_t *qi = (wlc_txq_info_t *)ctx;
	struct pktq *q = WLC_GET_TXQ(qi);
	wlc_info_t *wlc = qi->wlc;
	struct scb *scb;
	ratespec_t rspec;
	wlc_txh_info_t txh_info;
	wlc_pkttag_t *pkttag;
	uint16 prec_map;
	int prec;
	void *pkt[DOT11_MAXNUMFRAGS];
	int i, count;
	uint pkttime;
	uint supplied_time = 0;
#ifdef WLAMPDU_MAC
	bool check_epoch = TRUE;
#endif /* WLAMPDU_MAC */

	ASSERT(ac < AC_COUNT);

	/* use a prec_map that matches the AC fifo parameter */
	prec_map = wlc->fifo2prec_map[ac];

	while (supplied_time < request_time && (pkt[0] = pktq_mdeq(q, prec_map, &prec))) {
		pkttag = WLPKTTAG(pkt[0]);
		scb = WLPKTTAGSCBGET(pkt[0]);
		ASSERT(scb != NULL);

		/* Drop the frame if it was targeted to an scb not on the current band */
		if (scb->bandunit != wlc->band->bandunit) {
			PKTFREE(wlc->osh, pkt[0], TRUE);
			WLCNTINCR(wlc->pub->_cnt->txchanrej);
			continue;
		}

		if (pkttag->flags & WLF_MPDU) {
			uint fifo; /* is this param needed anymore ? */

			/*
			 * Drop packets that have been inserted here due to mulitqueue reclaim
			 * Need multiqueue support to push pkts to low_txq instead of qi->q
			 */
			if (pkttag->flags & WLF_TXHDR) {
				ASSERT((pkttag->flags & WLF_TXHDR) == 0);

				PKTFREE(wlc->osh, pkt[0], TRUE);
				WLCNTINCR(wlc->pub->_cnt->txchanrej);
				continue;
			}

			/* fetch the rspec saved in tx_prams at the head of the pkt
			 * before tx_params are removed by wlc_prep_pdu()
			 */
			rspec = wlc_pdu_txparams_rspec(wlc->osh, pkt[0]);

			count = 1;
			/* not looking at error return */
			(void)wlc_prep_pdu(wlc, scb, pkt[0], &fifo);

			wlc_get_txh_info(wlc, pkt[0], &txh_info);

			/* calculate and store the estimated pkt tx time */
			pkttime = wlc_tx_mpdu_time(wlc, scb, rspec, ac, txh_info.d11FrameSize);
			WLPKTTAG(pkt[0])->pktinfo.atf.pkt_time = (uint16)pkttime;
		} else {
			uint fifo; /* is this param needed anymore ? */
			int rc;
			rc = wlc_prep_sdu(wlc, scb, pkt, &count, &fifo);
			if (rc) {
				WL_ERROR(("%s(): SCB%s:0x%p q:0x%p "
				"FIFO:%u AC:%u rc:%d prec_map:0x%x len:%u\n",
				__FUNCTION__, SCB_INTERNAL(scb) ?  "(Internal)" : "",
				OSL_OBFUSCATE_BUF(scb), OSL_OBFUSCATE_BUF(q),
				fifo, ac, rc, prec_map, pktq_mlen(q, prec_map)));
			}
			/* the immediate queue should not have SDUs, right? */
			ASSERT(0);

			rspec = wlc_tx_current_ucast_rate(wlc, scb, ac);
			wlc_get_txh_info(wlc, pkt[0], &txh_info);

			/* calculate and store the estimated pkt tx time */
			pkttime = wlc_tx_mpdu_time(wlc, scb, rspec, ac, txh_info.d11FrameSize);
			WLPKTTAG(pkt[0])->pktinfo.atf.pkt_time = (uint16)pkttime;
		}

		supplied_time += pkttime;

		for (i = 0; i < count; i++) {
#ifdef WL11N
			uint16 frameid;

			wlc_get_txh_info(wlc, pkt[i], &txh_info);

#ifdef WLAMPDU_MAC
			/* For AQM AMPDU Aggregation:
			 * If there is a transition from A-MPDU aggregation frames to a
			 * non-aggregation frame, the epoch needs to change. Otherwise the
			 * non-agg frames may get included in an A-MPDU.
			 */
			if (check_epoch && AMPDU_AQM_ENAB(wlc->pub)) {
				/* Once we check the condition, we don't need to check again since
				 * we are enqueuing an non_ampdu frame so wlc_ampdu_was_ampdu() will
				 * be false.
				 */
				check_epoch = FALSE;
				/* if the previous frame in the fifo was an ampdu mpdu,
				 * change the epoch
				 */
				if (wlc_ampdu_was_ampdu(wlc->ampdu_tx, ac)) {
					bool epoch =
					        wlc_ampdu_chgnsav_epoch(wlc->ampdu_tx,
					                                ac,
					                                AMU_EPOCH_CHG_MPDU,
					                                scb,
					                                (uint8)PKTPRIO(pkt[i]),
					                                &txh_info);
					wlc_txh_set_epoch(wlc, txh_info.tsoHdrPtr, epoch);
				}
			}
#endif /* WLAMPDU_MAC */

			frameid = ltoh16(txh_info.TxFrameID);
			if (frameid & TXFID_RATE_PROBE_MASK) {
				wlc_scb_ratesel_probe_ready(wlc->wrsi, scb, frameid,
				                            FALSE, 0, (uint8)ac);
			}
#endif /* WL11N */
			/* add this pkt to the output queue */
			spktenq(output_q, pkt[i]);
		}
	}

#ifdef WLAMPDU_MAC
	/* For Ucode/HW AMPDU Aggregation:
	 * If there are non-aggreation packets added to a fifo, make sure the epoch will
	 * change the next time entries are made to the aggregation info side-channel.
	 * Otherwise, the agg logic may include the non-aggreation packets into an A-AMPDU.
	 */
	if (spktq_n_pkts(output_q) > 0 &&
		AMPDU_MAC_ENAB(wlc->pub) && !AMPDU_AQM_ENAB(wlc->pub)) {
		wlc_ampdu_change_epoch(wlc->ampdu_tx, ac, AMU_EPOCH_CHG_MPDU);
	}
#endif /* WLAMPDU_MAC */

	return supplied_time;
}

/** TXQ_MUX specific */
static void wlc_txq_packet_overflow_handler(wlc_info_t *wlc,
	struct pktq *q, void *pkt, uint prec)
{
	/* Tail drop packets */
	PKTFREE(wlc->osh, pkt, TRUE);
	WLCNTINCR(wlc->pub->_cnt->txnobuf);

#ifdef PKTQ_LOG
	/* Update pktq_log stats */
	if (q->pktqlog) {
		pktq_counters_t* prec_cnt = q->pktqlog->_prec_cnt[prec];
		WLCNTCONDINCR(prec_cnt, prec_cnt->dropped);
	}
#else
	BCM_REFERENCE(prec);
#endif
	WL_NONE(("wl%d: %s:  prec:%d. Tail dropping.\n",
		wlc->pub->unit, __FUNCTION__, prec));
}

/**
 * @brief Tx path MUX enqueue function (TXQ_MUX specific)
 *
 * Tx path MUX enqueue function. This funciton is used to send a packet immediately in the
 * context of a TxQ, but not in any SCB flow.
 */
int BCMFASTPATH
wlc_txq_immediate_enqueue(wlc_info_t *wlc, wlc_txq_info_t *qi, void *pkt, uint prec)
{
	struct pktq *q = WLC_GET_TXQ(qi);

	ASSERT(WLPKTTAG(pkt)->flags & WLF_MPDU);

	if (pktqprec_full(q, prec)) {
		wlc_txq_packet_overflow_handler(wlc, q, pkt, prec);
	} else if (wlc_prec_enq(wlc, q, pkt, prec)) {
		/* convert prec to ac fifo number, 4 precs per ac fifo */
		int ac_idx = prec / (WLC_PREC_COUNT / AC_COUNT);
		wlc_mux_source_wake(qi->ac_mux[ac_idx], qi->mux_hdl[ac_idx]);

		/* kick the low txq */
		wlc_send_q(wlc, qi);

		return TRUE;
	}

	return FALSE;
}

/*
 * Rate and time utils
 */

/** TXQ_MUX specific */
ratespec_t
wlc_tx_current_ucast_rate(wlc_info_t *wlc, struct scb *scb, uint ac)
{

	return wlc_scb_ratesel_get_primary(wlc, scb, NULL);
}

/**
 * Retrieve the rspec from the tx_params structure at the head of an WLF_MPDU
 * packet. TXQ_MUX specific.
 */
ratespec_t
wlc_pdu_txparams_rspec(osl_t *osh, void *p)
{
	ratespec_t rspec;
	int8 *rspec_ptr;

	/* pkt should be an MPDU, but not TXHDR, a packet with ucode txhdr
	 * The pkts with WLF_MPDU start with the wlc_pdu_tx_params_t, but pkts
	 * with WLF_TXHDR format flag indicate that PKTDATA starts with a ucode
	 * txhdr.
	 */
	ASSERT((WLPKTTAG(p)->flags & (WLF_MPDU | WLF_TXHDR)) == WLF_MPDU);

	/* copy the rspec from the tx_params
	 * Note that the tx_params address may not be 32bit aligned, so
	 * doing a copy instead of casting the PKTDATA pointer to a tx_params
	 * and doing a direct dereferenc for the rspec
	 */
	rspec_ptr = (int8*)PKTDATA(osh, p) +
		OFFSETOF(wlc_pdu_tx_params_t, rspec_override);

	memcpy(&rspec, rspec_ptr, sizeof(rspec));

	return rspec;
}

/** TXQ_MUX specific */
uint
wlc_tx_mpdu_frame_seq_overhead(ratespec_t rspec,
	wlc_bsscfg_t *bsscfg, wlcband_t *band, uint ac)
{
	ratespec_t ack_rspec;
	ratespec_t ctl_rspec;
	uint flags = 0;

	ack_rspec = band->basic_rate[RSPEC_REFERENCE_RATE(rspec)];

	ctl_rspec = ack_rspec;

	/*
	 * Short slot for ERP STAs
	 * The AP bsscfg will keep track of all sta's shortslot/longslot cap,
	 * and keep current_bss->capability up to date.
	 */
	if (BAND_5G(band->bandtype) ||
		bsscfg->current_bss->capability & DOT11_CAP_SHORTSLOT)
			flags |= (WLC_AIRTIME_SHORTSLOT);

	return wlc_airtime_pkt_overhead_us(flags, ctl_rspec, ack_rspec,
	                                   bsscfg, wme_fifo2ac[ac]);
}

/**
 * returns [us] units for a caller supplied ratespec/ac/mpdu length combination.
 * TXQ_MUX specific.
 */
uint
wlc_tx_mpdu_time(wlc_info_t *wlc, struct scb* scb,
	ratespec_t rspec, uint ac, uint mpdu_len)
{
	wlcband_t *band = wlc_scbband(wlc, scb);
	int bandtype = band->bandtype;
	wlc_bsscfg_t *bsscfg;
	uint fixed_overhead_us;
	int short_preamble;
	bool is2g;

	ASSERT(scb->bsscfg);
	bsscfg = scb->bsscfg;

	fixed_overhead_us = wlc_tx_mpdu_frame_seq_overhead(rspec,
		bsscfg, band, ac);

	is2g = BAND_2G(bandtype);

	/* calculate the preample type */
	if (RSPEC_ISLEGACY(rspec)) {
		/* For legacy reates calc the short/long preamble.
		 * Only applicable for 2, 5.5, and 11.
		 * Check the bss config and other overrides.
		 */

		uint mac_rate = (rspec & RATE_MASK);

		if ((mac_rate == WLC_RATE_2M ||
		     mac_rate == WLC_RATE_5M5 ||
		     mac_rate == WLC_RATE_11M) &&
		    WLC_PROT_CFG_SHORTPREAMBLE(wlc->prot, bsscfg) &&
		    (scb->flags & SCB_SHORTPREAMBLE) &&
		    (bsscfg->PLCPHdr_override != WLC_PLCP_LONG)) {
			short_preamble = 1;
		} else {
			short_preamble = 0;
		}
	} else {
		/* For VHT, always MM, for HT, assume MM and
		 * don't bother with Greenfield
		 */
		short_preamble = 0;
	}


	return (fixed_overhead_us +
	        wlc_txtime(rspec, is2g, short_preamble, mpdu_len));
}

ratespec_t
wlc_lowest_scb_rspec(struct scb *scb)
{
	ASSERT(scb != NULL);

	return LEGACY_RSPEC(scb->rateset.rates[0]);
}

ratespec_t
wlc_lowest_band_rspec(wlcband_t *band)
{
	ASSERT(band != NULL);

	return LEGACY_RSPEC(band->hw_rateset.rates[0]);
}

#if defined(TXQ_LOG)
static void
wlc_txq_log_req(txq_info_t *txqi, uint fifo, uint reqtime)
{
	txq_log_entry_t *e;
	uint16 idx = txqi->log_idx;
	uint32 time = 0;

	if (txqi->log_len == 0) {
		return;
	}

	{
		time = (1000000 / HZ) * jiffies;
	}

	/* get the current entry */
	e = &txqi->log[idx];

	/* save the info */
	e->type = TXQ_LOG_REQ;
	e->u.req.time = time;
	e->u.req.reqtime = (uint32)reqtime;
	e->u.req.fifo = (uint8)fifo;

	txqi->log_idx = (idx + 1) % txqi->log_len;
}

static void
wlc_txq_log_pkts(txq_info_t *txqi, struct spktq* queue)
{
	txq_log_entry_t *e;
	uint16 idx;
	uint16 log_len = txqi->log_len;
	void* p;

	if (log_len == 0) {
		return;
	}

	/* get the current entry */
	idx = txqi->log_idx;
	e = &txqi->log[idx];

	p = queue->q[0].head;
	while (p) {
		/* save the info */
		e->type = TXQ_LOG_PKT;
		e->u.pkt.prio = (uint8)PKTPRIO(p);
		e->u.pkt.bytes = (uint16)pkttotlen(txqi->osh, p);
		e->u.pkt.time = (uint32)WLPKTTIME(p);

		p = PKTLINK(p);
		idx++;
		if (idx == log_len) {
			idx = 0;
			e = txqi->log;
		} else {
			e++;
		}
	}

	txqi->log_idx = idx;
}

static int
wlc_txq_log_dump(void *ctx, struct bcmstrbuf *b)
{
	txq_info_t *txqi = (txq_info_t *)ctx;
	int idx;
	uint16 log_len = txqi->log_len;
	uint16 log_idx = txqi->log_idx;
	txq_log_entry_t *e;
	uint32 base_time = 0;
	uint32 last_time = 0;
	uint pkt_count = 0;
	uint pkt_bytes_tot = 0;
	uint pkt_time_tot = 0;

	idx = log_idx;
	e = &txqi->log[idx];

	do {
		/* summary after a block of pkt samples */
		if (e->type != TXQ_LOG_PKT &&
		    pkt_count > 0) {
			/* only print a summary line if there is more than 1 pkt */
			if (pkt_count > 1) {
				bcm_bprintf(b, "PktTot %u B%u %u us\n",
				            pkt_count, pkt_bytes_tot, pkt_time_tot);
			}

			pkt_count = 0;
			pkt_bytes_tot = 0;
			pkt_time_tot = 0;
		}

		/* individual samples */
		if (e->type == TXQ_LOG_REQ) {
			if (base_time == 0) {
				base_time = e->u.req.time;
				last_time = base_time;
			}

			bcm_bprintf(b, "FillReq %u(+%u) F%u %u us\n",
			            e->u.req.time - base_time,
			            e->u.req.time - last_time,
			            e->u.req.fifo,
			            e->u.req.reqtime);

			last_time = e->u.req.time;
		} else if (e->type == TXQ_LOG_PKT) {
			uint bytes = e->u.pkt.bytes;
			uint time = e->u.pkt.time;

			bcm_bprintf(b, "Pkt P%d B%u %u us\n",
			            wlc_prio2prec_map[e->u.pkt.prio], bytes, time);

			pkt_count++;
			pkt_bytes_tot += bytes;
			pkt_time_tot += time;
		}

		/* advance to the next entry */
		idx++;
		if (idx == log_len) {
			idx = 0;
			e = txqi->log;
		} else {
			e++;
		}

	} while (idx != log_idx);

	return BCME_OK;
}

#endif /* TXQ_LOG */

/** TXQ_MUX specific */
void BCMFASTPATH
wlc_txq_fill(txq_info_t *txqi, txq_t *txq)
{
	uint fifo_idx;
	int fill_space;
	int added_time;
	struct spktq fill_q;
	struct spktq *swq;
	wlc_info_t *wlc = txqi->wlc;
#ifdef WLCNT
	struct txq_fifo_cnt *cnt;
#endif

	if (wlc->in_send_q) {
		txqi->rerun = TRUE;
		WL_INFORM(("wl%d: in_send_q, txq=%p\n", wlc->pub->unit, OSL_OBFUSCATE_BUF(txq)));
		return;
	}

	wlc->in_send_q = TRUE;

	/* init the round robin fill_q */
	spktqinit(&fill_q, PKTQ_LEN_MAX);

	/* Loop thru all the fifos and dequeue the packets
	 * BCMC packets will be added to the BCMC fifo if any of the nodes enter PS on the AP.
	 */
	for (fifo_idx = 0; fifo_idx < txq->nfifo; fifo_idx++) {
		swq = &txq->swq[fifo_idx];

		/* skip fill from above if this fifo is stopped */
		if (!txq_stopped(txq, fifo_idx)) {
#ifdef WLCNT
			cnt = &txq->fifo_cnt[fifo_idx];
#endif
			WLCNTINCR(cnt->q_service);

			/* find out how much room (measured in time) in this fifo of txq */
			fill_space = txq_space(txq, fifo_idx);

			/* Ask feed for pkts to fill available time */
			TXQ_LOG_REQ(txqi, fifo_idx, fill_space);

			/* CPU INTENSIVE */
			added_time = txq->supply(txq->supply_ctx, fifo_idx, fill_space, &fill_q);

			/* skip work if no new data */
			if (added_time > 0) {

				TXQ_LOG_PKTS(txqi, &fill_q);

				/* total accumulation counters update */
				WLCNTADD(cnt->pkts, spktq_n_pkts(&fill_q));
				WLCNTADD(cnt->time, added_time);

				/* track pending packet count per fifo */
				TXPKTPENDINC(wlc, fifo_idx, spktq_n_pkts(&fill_q));
				WL_TRACE(("wl:%s: pktpend inc %d to %d\n", __FUNCTION__,
				          spktq_n_pkts(&fill_q), TXPKTPENDGET(wlc, fifo_idx)));

				/* track added time per fifo */
				txq_inc(txq, fifo_idx, added_time);

				/* append the provided packets to the txq */
				spktq_append(swq, &fill_q);

				/* check for flow control from new added time */
				if (added_time >= fill_space) {
					txq_stop_set(txq, fifo_idx);
					WLCNTINCR(cnt->q_stops);
				}
			}
		}

		/* post to HW fifo if HW fifo is not stopped and there is data to send */
		if (!txq_hw_stopped(txq, fifo_idx) && spktq_n_pkts(swq) > 0) {
			txq_hw_fill(txqi, txq, fifo_idx);
		}

		/* if there was a reenterant call to this fn, rerun the loop */
		if (txqi->rerun && (fifo_idx == txq->nfifo - 1)) {
			txqi->rerun = FALSE;
			fifo_idx = (uint)-1; /* set index back so loop will re-run */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
			txqi->rerun_count++;
			WL_ERROR(("wl:%s: rerun %d triggered\n", __FUNCTION__, txqi->rerun_count));
#endif
		}
	}

	spktqdeinit(&fill_q);

	wlc->in_send_q = FALSE;
}

#else /* TXQ_MUX */

/** Non-TXQ_MUX specific
 * Attempts to fill lower transmit queue with packets from wlc/common queue.
 * @param txqi E.g. the wlc/common transmit queue
 * @param dest_txq  E.g. the lower transmit queue
 */
void BCMFASTPATH
wlc_txq_fill(txq_info_t *txqi, txq_t *dest_txq)
{
	uint fifo_idx, ac;
	int fill_space;
	int added_time;
	wlc_info_t *wlc = txqi->wlc;
	struct spktq temp_q; /* simple, non-priority pkt queue */
#ifdef WLCNT
	struct txq_fifo_cnt *cnt;
#endif

	if (wlc->in_send_q) {
		return;
	}
	wlc->in_send_q = TRUE;

	/* init the round robin temp_q */
	spktqinit(&temp_q, PKTQ_LEN_MAX);

	for (ac = 0; ac < AC_COUNT; ac++) {
		added_time = dest_txq->supply(dest_txq->supply_ctx, ac, -1, &temp_q, &fifo_idx);

		if (added_time == 0) {
			continue;
		}
		ASSERT(fifo_idx < WLC_HW_NFIFO_INUSE(wlc));

#ifdef WLCNT
		cnt = &dest_txq->fifo_cnt[fifo_idx];
#endif
		WLCNTINCR(cnt->q_service);

		/* find out how much room (measured in time) in this fifo of dest_txq */
		fill_space = txq_space(dest_txq, fifo_idx);


		/* total accumulation counters update */
		WLCNTADD(cnt->pkts, spktq_n_pkts(&temp_q));
		WLCNTADD(cnt->time, added_time);

		/* track weighted pending packet count per fifo */
		TXPKTPENDINC(wlc, fifo_idx, (int16)added_time);
		WL_TRACE(("wl:%s: fifo:%d pktpend inc %d to %d\n", __FUNCTION__,
		         fifo_idx, added_time, TXPKTPENDGET(wlc, fifo_idx)));

		/* track added time per fifo */
		txq_inc(dest_txq, fifo_idx, added_time);

		/* append the provided packets to the (lower) txq */
		spktq_append(&dest_txq->swq[fifo_idx], &temp_q);

		/* check for flow control from new added time */
		if (added_time >= fill_space) {
			txq_stop_set(dest_txq, fifo_idx);
			WLCNTINCR(cnt->q_stops);
		}
	} /* for */

	for (fifo_idx = 0; fifo_idx < dest_txq->nfifo; fifo_idx ++) {
		txq_hw_fill(txqi, dest_txq, fifo_idx);
	}

	spktqdeinit(&temp_q);

	wlc->in_send_q = FALSE;
}
#endif /* TXQ_MUX */

void BCMFASTPATH
wlc_txq_complete(txq_info_t *txqi, txq_t *txq, uint fifo_idx, int complete_pkts, int complete_time)
{
	int remaining_time;
	BCM_REFERENCE(txqi);
	BCM_REFERENCE(complete_pkts);

	remaining_time = txq_dec(txq, fifo_idx, complete_time);

	/* open up the hw fifo as soon as any pkts free */
	txq_hw_stop_clr(txq, fifo_idx);

	/* check for flow control release */
	if (remaining_time <= txq->fifo_state[fifo_idx].lowwater) {
		txq_stop_clr(txq, fifo_idx);
	}
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int
wlc_txq_dump(txq_info_t *txqi, txq_t *txq, struct bcmstrbuf *b)
{
	uint i;
	wlc_info_t *wlc = txqi->wlc;
	const char *name;

	if (wlc->active_queue->low_txq == txq) {
		name = "Active Q";
	} else if (wlc->primary_queue->low_txq == txq) {
		name = "Primary Q";
	} else if (wlc->excursion_queue->low_txq == txq) {
		name = "Excursion Q";
	} else {
		name = "";
	}

	bcm_bprintf(b, "txq %p: %s\n", OSL_OBFUSCATE_BUF(txq), name);

	for (i = 0; i < txq->nfifo; i++) {
		struct txq_fifo_state *f = &txq->fifo_state[i];
		struct pktq_prec *q = &txq->swq[i].q;
#ifdef WLCNT
		struct txq_fifo_cnt *cnt = &txq->fifo_cnt[i];
#endif

		bcm_bprintf(b, "fifo %u:\n", i);

		bcm_bprintf(b, "qpkts %u flags 0x%x hw %d lw %d buffered %d\n",
		            (uint)q->n_pkts, f->flags, f->highwater, f->lowwater, f->buffered);

#ifdef WLCNT
		bcm_bprintf(b,
			"Total: pkts %u time %u Q stop %u service %u HW stop %u service %u\n",
			cnt->pkts, cnt->time, cnt->q_stops,
			cnt->q_service, cnt->hw_stops, cnt->hw_service);
#endif
	}

	return BCME_OK;
}

static int
wlc_txq_module_dump(void *ctx, struct bcmstrbuf *b)
{
	txq_info_t *txqi = (txq_info_t *)ctx;
	txq_t *txq;

	txq = txqi->txq_list;

	while (txq != NULL) {
		wlc_txq_dump(txqi, txq, b);
		txq = txq->next;
		if (txq != NULL) {
			bcm_bprintf(b, "\n");
		}
	}
#ifdef TXQ_MUX
	bcm_bprintf(b, "Re-run count: %d\n", txqi->rerun_count);
#endif
	return BCME_OK;
}

#endif /* BCMDBG || BCMDBG_DUMP */

#if defined(WL_DATAPATH_LOG_DUMP)
/*
 * Helper fn to fill out the 'plen' array of txq_summary_v2_t struct given a pktq
 */
static void
wlc_txq_prec_summary(txq_summary_v2_t *txq_sum, struct pktq *q, uint num_prec)
{
	uint prec;

	txq_sum->prec_count = (uint8)num_prec;

	for (prec = 0; prec < num_prec; prec++) {
		txq_sum->plen[prec] = pktqprec_n_pkts(q, prec);
	}
}

/**
 * Helper fn for wlc_txq_datapath_summary(). Summarize the low_txq portion of a TxQ context.
 * @param txq       pointer to low txq state structure
 * @param txq_sum   pointer to txq_summary_v2_t EVENT_LOG reporting structure
 * @param tag       EVENT_LOG tag for output
 */
static void
wlc_low_txq_datapath_summary(txq_t *txq, txq_summary_v2_t *txq_sum, int tag)
{
	uint fifo_idx;
	uint num_fifo;

	num_fifo = txq->nfifo;

	txq_sum->id = EVENT_LOG_XTLV_ID_TXQ_SUM_V2;
	txq_sum->len = TXQ_SUMMARY_V2_FULL_LEN(num_fifo) - BCM_XTLV_HDR_SIZE;
	txq_sum->bsscfg_map = 0;

	txq_sum->prec_count = (uint8)num_fifo;
	for (fifo_idx = 0; fifo_idx < num_fifo; fifo_idx++) {
		txq_sum->plen[fifo_idx] = spktq_n_pkts(&txq->swq[fifo_idx]);
	}

	for (fifo_idx = 0; fifo_idx < num_fifo; fifo_idx++) {
		if (txq_stopped(txq, fifo_idx)) {
			txq_sum->stopped |= 1 << fifo_idx;
		}
		if (txq_hw_stopped(txq, fifo_idx)) {
			txq_sum->hw_stopped |= 1 << fifo_idx;
		}
	}

	EVENT_LOG_BUFFER(tag, (uint8*)txq_sum, txq_sum->len + BCM_XTLV_HDR_SIZE);
}

/**
 * Use EVENT_LOG to dump a summary of a TxQ context (wlc_txq_info_t)
 * @param wlc       pointer to wlc_info state structure
 * @param qi        pointer to wlc_txq_info_t state structure
 * @param txq_sum   pointer to txq_summary_v2_t EVENT_LOG reporting structure
 * @param tag       EVENT_LOG tag for output
 */
static void
wlc_txq_datapath_summary(wlc_info_t *wlc, wlc_txq_info_t *qi, txq_summary_v2_t *txq_sum, int tag)
{
	wlc_bsscfg_t *cfg;
	struct pktq *q;
	int idx;
	uint num_prec;
	uint32 bsscfg_map = 0;

	q = WLC_GET_TXQ(qi);

	num_prec = MIN(q->num_prec, PKTQ_MAX_PREC);

	FOREACH_BSS(wlc, idx, cfg) {
		if (cfg->wlcif->qi == qi) {
			bsscfg_map |= 1 << idx;
		}
	}

	txq_sum->id = EVENT_LOG_XTLV_ID_TXQ_SUM_V2;
	txq_sum->len = TXQ_SUMMARY_V2_FULL_LEN(num_prec) - BCM_XTLV_HDR_SIZE;
	txq_sum->bsscfg_map = bsscfg_map;
	txq_sum->stopped = qi->stopped;

	wlc_txq_prec_summary(txq_sum, q, num_prec);

	EVENT_LOG_BUFFER(tag, (uint8*)txq_sum, txq_sum->len + BCM_XTLV_HDR_SIZE);

	wlc_low_txq_datapath_summary(qi->low_txq, txq_sum, tag);
}

/**
 * Use EVENT_LOG to dump a summary of all TxQ contexts
 * @param wlc       pointer to wlc_info state structure
 * @param tag       EVENT_LOG tag for output
 */
void
wlc_tx_datapath_log_dump(wlc_info_t *wlc, int tag)
{
	wlc_txq_info_t *qi;
	int buf_size;
	txq_summary_v2_t *txq_sum;
	osl_t *osh = wlc->osh;

	/* allocate a size large enough for max precidences */
	buf_size = TXQ_SUMMARY_V2_FULL_LEN(PKTQ_MAX_PREC * 2);

	txq_sum = MALLOCZ(osh, buf_size);
	if (txq_sum == NULL) {
		EVENT_LOG(tag,
		          "wlc_tx_datapath_log_dump(): MALLOC %d failed, malloced %d bytes\n",
		          buf_size, MALLOCED(osh));
	} else {
		wlc_txq_datapath_summary(wlc, wlc->excursion_queue, txq_sum, tag);

		for (qi = wlc->tx_queues; qi != NULL; qi = qi->next) {
			if (qi != wlc->excursion_queue) {
				wlc_txq_datapath_summary(wlc, qi, txq_sum, tag);
			}
		}

		MFREE(osh, txq_sum, buf_size);
	}
}
#endif /* WL_DATAPATH_LOG_DUMP */

static int BCMFASTPATH
wlc_scb_txfifo(wlc_info_t *wlc, struct scb *scb, void *pkt, uint *pfifo)
{
	uint8 prio;
	wlc_bsscfg_t *cfg;
	int bcmerror = BCME_OK;

	ASSERT(scb != NULL);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);
	BCM_REFERENCE(cfg);

	/*
	 * On the AP, bcast/mcast always goes through the bcmc fifo if bcmc_scb is in PS Mode.
	 * bcmc_scb is in PS Mode if at least one ucast STA is in PS.  SDUs are data
	 * packets and always go in BE_FIFO unless WME is enabled.
	 *
	 * Default prio has to be 0 for TKIP MIC calculation when sending packet on a non-WME
	 * link, as the receiving end uses prio of 0 for MIC calculation when QoS header
	 * is absent from the packet.
	 */
	prio = 0;
	if (SCB_QOS(scb)) {
		prio = (uint8)PKTPRIO(pkt);
		ASSERT(prio <= MAXPRIO);
	}

	*pfifo = TX_AC_BE_FIFO;

	if ((BSSCFG_AP(cfg) || BSSCFG_IBSS(cfg)) && SCB_ISMULTI(scb) &&
		WLC_BCMC_PSMODE(wlc, cfg)) {
		*pfifo = TX_BCMC_FIFO; /* broadcast/multicast */
	} else if (SCB_WME(scb)) {
		*pfifo = prio2fifo[prio];

		if (!(WLPKTTAG(pkt)->flags & WLF_AMPDU_MPDU)) {
			/* this can change pkt prio */
			*pfifo = wlc_wme_wmmac_check_fixup(wlc, scb, pkt, prio, &bcmerror);
		}
#ifdef WL_MU_TX
		if ((bcmerror == BCME_OK) && (BSSCFG_AP(cfg) && SCB_MU(scb) && MU_TX_ENAB(wlc))) {
			wlc_mutx_sta_txfifo(wlc->mutx, scb, pfifo);
		} else
#endif
		{
			/* if error, wlc_prep_sdu() will drop this pkt */
		}
	}
	return bcmerror;
}

#if defined(TXQ_MUX)
uint BCMFASTPATH
wlc_pull_q(void *ctx, uint fifo_idx, int requested_time, struct spktq *output_q)
{
	wlc_txq_info_t *txq = (wlc_txq_info_t*)ctx;
	if (txq->wlc->block_datafifo & DATA_BLOCK_TXCHAIN) {
		WL_INFORM(("wl%d: pull_q(mux): wlc->block_datafifo & DATA_BLOCK_TXCHAIN\n",
		            txq->wlc->pub->unit));
		return 0;
	}

	return wlc_mux_output(txq->ac_mux[fifo_idx], requested_time, output_q);
}
#else /* TXQ_MUX */
/** !TXQ_MUX specific */
void BCMFASTPATH
wlc_low_txq_enq(txq_info_t *txqi, txq_t *txq, uint fifo_idx, void *pkt, int pkt_time)
{
	wlc_info_t *wlc = txqi->wlc;
#ifdef WLCNT
	struct txq_fifo_cnt *cnt = &txq->fifo_cnt[fifo_idx];
#endif

	/* enqueue the packet to the specified fifo queue */
	spktq_enq(&txq->swq[fifo_idx], pkt);

	/* total accumulation counters update */
	WLCNTADD(cnt->pkts, 1);
	WLCNTADD(cnt->time, pkt_time);

#ifdef TXQ_MUX
	WLPKTTAG(p)->pktinfo.atf.pkt_time = (uint16)pkt_time;
#endif

	/* track pending packet count per fifo */
	TXPKTPENDINC(wlc, fifo_idx, 1);
	WL_TRACE(("wl:%s: pktpend inc 1 to %d\n", __FUNCTION__,
	          TXPKTPENDGET(wlc, fifo_idx)));

	/* track added time per fifo */
	txq_inc(txq, fifo_idx, pkt_time);

	/* check for flow control from new added time */
	if (!txq_stopped(txq, fifo_idx) && txq_space(txq, fifo_idx) <= 0) {
		txq_stop_set(txq, fifo_idx);
		WLCNTINCR(cnt->q_stops);
	}
}

static int BCMFASTPATH
wlc_scb_peek_txfifo(wlc_info_t *wlc, struct scb *scb, void *pkt, uint *pfifo)
{
	int bcmerror = BCME_OK;
	uint32 flags;
	uint16 txframeid;
	wlc_bsscfg_t *cfg;
	wlc_pdu_tx_params_t *tx_params;

	ASSERT(pkt);

	/* check if dest fifo is already decided */
	flags = WLPKTTAG(pkt)->flags;
	if (flags & WLF_MPDU) {
		if (flags & WLF_TXHDR) {
			cfg = SCB_BSSCFG(scb);

			ASSERT(cfg != NULL);
			if ((BSSCFG_AP(cfg) || BSSCFG_IBSS(cfg)) &&
				SCB_ISMULTI(scb) && WLC_BCMC_PSMODE(wlc, cfg)) {
				/* The wlc_apps_ps_prep_mpdu will change the txfifo when
				 * scb ISMULTI and PSMODE is true, so get tx fifo from
				 * wlc_scb_txfifo.
				 */
				bcmerror = wlc_scb_txfifo(wlc, scb, pkt, pfifo);
			} else {
				txframeid = wlc_get_txh_frameid(wlc, pkt);
				*pfifo = WLC_TXFID_GET_QUEUE(txframeid);
			}
		} else {
			tx_params = (wlc_pdu_tx_params_t *)PKTDATA(wlc->osh, pkt);
			*pfifo = load32_ua(&tx_params->fifo);
		}

		if (!MU_TX_ENAB(wlc) && (*pfifo >= NFIFO)) {
			WL_ERROR(("%s: fifo %u >= NFIFO %u when MUTX is disabled\n", __FUNCTION__,
				*pfifo, NFIFO));
			bcmerror = wlc_scb_txfifo(wlc, scb, pkt, pfifo);
			WL_ERROR(("%s: changed to fifo %u txhdr %s\n", __FUNCTION__, *pfifo,
				(flags & WLF_TXHDR) ? "true":"false"));
			if (flags & WLF_TXHDR) {
				wlc_txh_info_t txh_info;

				wlc_get_txh_info(wlc, pkt, &txh_info);
				/* setup frameid, also possibly change seq */
				if (*pfifo == TX_BCMC_FIFO) {
					cfg = SCB_BSSCFG(scb);
					txframeid = bcmc_fid_generate(wlc, cfg,
						txh_info.TxFrameID);
				} else {
					txframeid = (((wlc->counter++) << TXFID_SEQ_SHIFT) &
						TXFID_SEQ_MASK) | WLC_TXFID_SET_QUEUE(*pfifo);
				}
				txh_info.TxFrameID = htol16(txframeid);
				wlc_set_txh_info(wlc, pkt, &txh_info);
			} else {
				int8 *fifo_ptr = (int8*)PKTDATA(wlc->osh, pkt) +
					OFFSETOF(wlc_pdu_tx_params_t, fifo);
				memcpy(fifo_ptr, pfifo, sizeof(*pfifo));
			}
		}
	} else {
		bcmerror = wlc_scb_txfifo(wlc, scb, pkt, pfifo);
	}
	return bcmerror;
}

/**
 * 'Supply/feed function' of the lower transmit queue. Pulls one or more packets
 * from the common/wlc transmit queue and puts them on the caller-provided
 * output_q. After this function returns, 'output_q' is used by the caller to
 * forward the packets to the low transmit queue. !TXQ_MUX specific.
 * TODO: calls wlc_low_txq_enq(), which seems wrong.
 */
uint BCMFASTPATH
wlc_pull_q(void *ctx, uint ac_fifo, int requested_time, struct spktq *output_q, uint *fifo_idx)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	void *pkt[DOT11_MAXNUMFRAGS];
	wlc_txq_info_t *qi;
	int prec;
	uint16 prec_map;
	int err, i, count;
	int supplied_time = 0;
	wlc_pkttag_t *pkttag;
	struct scb *scb;
	struct pktq *q;
	wlc_bsscfg_t *cfg;
	osl_t *osh;
	uint fifo = NFIFO_EXT, prev_fifo = NFIFO_EXT;
#ifdef WL_BSSCFG_TX_SUPR
#ifdef PROP_TXSTATUS
	int suppress_to_host = 0;
#endif /* PROP_TXSTATUS */
#endif /* WL_BSSCFG_TX_SUPR */
#ifdef WL_FRAGDUR
	int txagg_state_changed = 0;
#endif /* WL_FRAGDUR */

#ifdef WL_BSSCFG_TX_SUPR
	/* Waiting to drain the FIFO so don't pull any packet */
	if (wlc->block_datafifo & DATA_BLOCK_TX_SUPR) {
		WL_INFORM(("wl%d: wlc->block_datafifo & DATA_BLOCK_TX_SUPR\n", wlc->pub->unit));
		return supplied_time;
	}
#endif /* WL_BSSCFG_TX_SUPR */

	if (wlc->block_datafifo & DATA_BLOCK_TXCHAIN) {
		WL_INFORM(("wl%d: pull_q: wlc->block_datafifo & DATA_BLOCK_TXCHAIN\n",
		            wlc->pub->unit));
		return supplied_time;
	}


	/* WES: Make sure wlc_send_q is not called when detach pending */
	/* Detaching queue is still pending, don't queue packets to FIFO */
	if (wlc->txfifo_detach_pending) {
		WL_INFORM(("wl%d: wlc->txfifo_detach_pending %d\n",
		           wlc->pub->unit, wlc->txfifo_detach_pending));
		return supplied_time;
	}

	/* only do work for the active queue */
	qi = wlc->active_queue;

	osh = wlc->osh;
	BCM_REFERENCE(osh);

	ASSERT(qi);
	q = WLC_GET_TXQ(qi);

	/* Use a prec_map that matches the AC fifo parameter */
	prec_map = wlc->fifo2prec_map[ac_fifo];

	pkt[0] = pktq_mpeek(q, prec_map, &prec);

	if (pkt[0] == NULL) {
		return supplied_time;
	}

	scb = WLPKTTAGSCBGET(pkt[0]);
	ASSERT(scb != NULL);

	wlc_scb_peek_txfifo(wlc, scb, pkt[0], &fifo);
	ASSERT(fifo < WLC_HW_NFIFO_INUSE(wlc));

	if (txq_stopped(qi->low_txq, fifo)) {
		return supplied_time;
	}

	/* find out how much room (measured in time) in this fifo of txq */
	requested_time = txq_space(qi->low_txq, fifo);

	/* Send all the enq'd pkts that we can.
	 * Dequeue packets with precedence with empty HW fifo only
	 */
	while (supplied_time < requested_time && prec_map &&
		(pkt[0] = pktq_mdeq(q, prec_map, &prec))) {
#ifdef BCMPCIEDEV
		if (BCMPCIEDEV_ENAB() && BCMLFRAG_ENAB() &&
			!wlc->cmn->hostmem_access_enabled && PKTISTXFRAG(osh, pkt[0])) {
			/* Drop the Host originated LFRAG tx since no host
			* memory access is allowed in PCIE D3 suspend
			* mode. Need to put one more check to be sure
			* that this packet is originated from host.
			*/
			PKTFREE(osh, pkt[0], TRUE);
			continue;
		}
#endif /* BCMPCIEDEV */
		/* Send AMPDU using wlc_sendampdu (calls wlc_txfifo() also),
		 * SDU using wlc_prep_sdu and PDU using wlc_prep_pdu followed by
		 * wlc_txfifo() for each of the fragments
		 */
		pkttag = WLPKTTAG(pkt[0]);

		scb = WLPKTTAGSCBGET(pkt[0]);
		ASSERT(scb != NULL);

		cfg = SCB_BSSCFG(scb);
		ASSERT(cfg != NULL);
		BCM_REFERENCE(cfg);

#ifdef WL_FRAGDUR
		/* if tx aggregation state has changed, break while loop */
		if ((pkttag->flags & WLF_DATA) &&
			!SCB_ISMULTI(scb) &&
			wlc_txagg_state_changed(wlc, scb, pkt[0])) {
			txagg_state_changed = 1;
			break;
		}
#endif /* WL_FRAGDUR */

		wlc_scb_peek_txfifo(wlc, scb, pkt[0], &fifo);

		if ((prev_fifo != NFIFO_EXT) && (prev_fifo != fifo)) {
			/* if dest fifo changed in the loop then exit with
			 * prev_fifo; caller of wlc_pull_q cannot handle fifo
			 * changes.
			 */
			pktq_penq_head(q, prec, pkt[0]);

			break;
		}
		ASSERT(fifo < WLC_HW_NFIFO_INUSE(wlc));

		*fifo_idx = fifo;
		prev_fifo = fifo;

#ifdef WL_BSSCFG_TX_SUPR
		if (BSS_TX_SUPR(cfg) &&
			!(BSSCFG_IS_AIBSS_PS_ENAB(cfg) && (pkttag->flags & WLF_PSDONTQ))) {

			ASSERT(!(wlc->block_datafifo & DATA_BLOCK_TX_SUPR));
#ifdef PROP_TXSTATUS
			if (PROP_TXSTATUS_ENAB(wlc->pub)) {
				if (suppress_to_host) {
					if (wlc_suppress_sync_fsm(wlc, scb, pkt[0], FALSE)) {
						wlc_process_wlhdr_txstatus(wlc,
						        WLFC_CTL_PKTFLAG_WLSUPPRESS,
						        pkt[0], FALSE);
						PKTFREE(osh, pkt[0], TRUE);
					} else {
						suppress_to_host = 0;
					}
				}
				if (!suppress_to_host) {
					if (wlc_bsscfg_tx_psq_enq(wlc->psqi, cfg, pkt[0], prec)) {
						suppress_to_host = 1;
						wlc_suppress_sync_fsm(wlc, scb, pkt[0], TRUE);
						/*
						release this packet and move on to the next
						element in q,
						host will resend this later.
						*/
						wlc_process_wlhdr_txstatus(wlc,
						        WLFC_CTL_PKTFLAG_WLSUPPRESS,
						        pkt[0], FALSE);
						PKTFREE(osh, pkt[0], TRUE);
					}
				}
			} else
#endif /* PROP_TXSTATUS */
			if (wlc_bsscfg_tx_psq_enq(wlc->psqi, cfg, pkt[0], prec)) {
				WL_ERROR(("%s: FAILED TO ENQUEUE PKT\n", __FUNCTION__));
				PKTFREE(osh, pkt[0], TRUE);
			}
			continue;
		}
#ifdef BCMDBG
		{
			uint cnt = wlc_bsscfg_tx_pktcnt(wlc->psqi, cfg);
			if (cnt > 0) {
				WL_ERROR(("%s: %u SUPR PKTS LEAKED IN PSQ\n",
				          __FUNCTION__, cnt));
			}
		}
#endif
#endif /* WL_BSSCFG_TX_SUPR */

#ifdef WLAMPDU
		if (WLPKTFLAG_AMPDU(pkttag)) {
			err = wlc_sendampdu(wlc->ampdu_tx,
				qi, pkt, prec, output_q, &supplied_time);
		} else
#endif /* WLAMPDU */
		{
			if (pkttag->flags & WLF_MPDU) {
				count = 1;
				err = wlc_prep_pdu(wlc, scb,  pkt[0], &fifo);
			} else {
				err = wlc_prep_sdu(wlc, scb, pkt, &count, &fifo);
			}

			/* transmit if no error */
			if (!err) {

#ifdef WLAMPDU_MAC
				if (fifo < WLC_HW_NFIFO_INUSE(wlc) &&
				    AMPDU_MAC_ENAB(wlc->pub) && !AMPDU_AQM_ENAB(wlc->pub)) {
					wlc_ampdu_change_epoch(wlc->ampdu_tx, fifo,
						AMU_EPOCH_CHG_MPDU);
				}
#endif /* WLAMPDU_MAC */

				for (i = 0; i < count; i++) {
					wlc_txh_info_t txh_info;

					uint16 frameid;

					wlc_get_txh_info(wlc, pkt[i], &txh_info);
					frameid = ltoh16(txh_info.TxFrameID);
#ifdef WLAMPDU_MAC
					if (fifo < WLC_HW_NFIFO_INUSE(wlc) &&
					    AMPDU_AQM_ENAB(wlc->pub)) {
						if ((i == 0) &&
							wlc_ampdu_was_ampdu(wlc->ampdu_tx, fifo)) {
							bool epoch =
							wlc_ampdu_chgnsav_epoch(wlc->ampdu_tx,
							        fifo,
								AMU_EPOCH_CHG_MPDU,
								scb,
								(uint8)PKTPRIO(pkt[i]),
								&txh_info);
							wlc_txh_set_epoch(wlc, txh_info.tsoHdrPtr,
							                  epoch);
						}
#ifdef WL11AX
						if (D11REV_GE(wlc->pub->corerev, 80)) {
							struct dot11_header *h;
							uint16 fc_type;
							h = (struct dot11_header *)
								(txh_info.d11HdrPtr);
							fc_type = FC_TYPE(ltoh16(h)->fc);
							wlc_txh_set_ft(wlc, txh_info.tsoHdrPtr,
								fc_type);
							wlc_txh_set_frag_allow(wlc,
								txh_info.tsoHdrPtr,
								(fc_type == FC_TYPE_DATA));
						}
#endif /* WL11AX */
					}
#endif /* WLAMPDU_MAC */

#ifdef WL11N
					if (frameid & TXFID_RATE_PROBE_MASK) {
						wlc_scb_ratesel_probe_ready(wlc->wrsi, scb, frameid,
							FALSE, 0, WME_PRIO2AC(PKTPRIO(pkt[i])));
					}
#endif /* WL11N */
					/* add this pkt to the output queue */
					if ((WLC_TXFID_GET_QUEUE(frameid)) == fifo) {
						spktenq(output_q, pkt[i]);
						 /* fake a time of 1 for each pkt */
						supplied_time += 1;
					} else {
						if ((WLC_TXFID_GET_QUEUE(frameid)) != fifo) {
							WL_ERROR((
							"%s %s IDX(%d) FIFO(%d) FID(%d)mismatch\n",
							__FUNCTION__,
							(pkttag->flags & WLF_MPDU) ? "PDU" :"SDU",
							*fifo_idx, fifo,
							WLC_TXFID_GET_QUEUE(frameid)));
						}
						/* fake a time of 1us for now */
						wlc_low_txq_enq(wlc->txqi,
							qi->low_txq, fifo, pkt[i], 1);
					}
#if defined(SAVERESTORE) && defined(WLAIBSS)
					/* To keep uCode awake when SR and IBSS PS enabled */
					if (AIBSS_ENAB(wlc->pub) && AIBSS_PS_ENAB(wlc->pub) &&
						SR_ENAB() && (fifo == TX_BCMC_FIFO)) {
						wlc_aibss_stay_awake(wlc->aibss_info);
					}
#endif /* SAVERESTORE && WLAIBSS */
				}
			}
		}

		if (err == BCME_BUSY) {
			PKTDBG_TRACE(osh, pkt[0], PKTLIST_PRECREQ);
			pktq_penq_head(q, prec, pkt[0]);

			/* Remove this prec from the prec map when the pkt at the head
			 * causes a BCME_BUSY. The next lower prec may have pkts that
			 * are not blocked.
			 * When prec_map is zero, the top of the loop will bail out
			 * because it will not dequeue any pkts.
			 */
			prec_map &= ~(1 << prec);
		}
	}

	/* Check if flow control needs to be turned off after sending the packet */
	if (!EDCF_ENAB(wlc->pub) || (wlc->pub->wlfeatureflag & WL_SWFL_FLOWCONTROL)) {
		if (wlc_txflowcontrol_prio_isset(wlc, qi, ALLPRIO) &&
		    (pktq_n_pkts_tot(q) < wlc->pub->tunables->datahiwat / 2)) {
			wlc_txflowcontrol(wlc, qi, OFF, ALLPRIO);
		}
	} else if (wlc->pub->_priofc) {
		int prio;

		for (prio = MAXPRIO; prio >= 0; prio--) {
			if (wlc_txflowcontrol_prio_isset(wlc, qi, prio) &&
			    (pktqprec_n_pkts(q, wlc_prio2prec_map[prio]) <
			     wlc->pub->tunables->datahiwat/2)) {
				wlc_txflowcontrol(wlc, qi, OFF, prio);
			}
		}
	}

#ifdef WL_FRAGDUR
	/* if txagg state changed, requeue packet */
	if (txagg_state_changed) {
		txagg_state_changed = 0;
		pktq_penq_head(q, prec, pkt[0]);
	}
#endif /* WL_FRAGDUR */

	return supplied_time;
} /* wlc_pull_q */
#endif /* TXQ_MUX */

/* module entries */
txq_info_t *
BCMATTACHFN(wlc_txq_attach)(wlc_info_t *wlc)
{
	txq_info_t *txqi;
	int err;

	/* Allocate private states struct. */
	if ((txqi = MALLOCZ(wlc->osh, sizeof(txq_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->pub->osh)));
		goto fail;
	}

	txqi->wlc = wlc;
	txqi->pub = wlc->pub;
	txqi->osh = wlc->osh;

	if ((txqi->delq = MALLOC(wlc->osh, sizeof(*(txqi->delq)))) == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->pub->osh)));
		goto fail;
	}
	spktqinit(txqi->delq, PKTQ_LEN_MAX);

	/* Register module entries. */
	err = wlc_module_register(wlc->pub,
	                          NULL /* txq_iovars */,
	                          "txq", txqi,
	                          NULL /* wlc_txq_doiovar */,
	                          NULL /* wlc_txq_watchdog */,
	                          NULL,
	                          wlc_txq_down);

	if (err != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          txqi->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	wlc_dump_register(txqi->pub, "txq", wlc_txq_module_dump, (void *)txqi);
	wlc_dump_register(wlc->pub, "datapath", (dump_fn_t)wlc_datapath_dump, (void *)wlc);
#endif /* BCMDBG || BCMDBG_DUMP */


#ifdef TXQ_LOG
	/* allocate the debugging txq log */
	txqi->log_len = TXQ_LOG_LEN;
	txqi->log = MALLOCZ(txqi->osh, sizeof(*txqi->log) * TXQ_LOG_LEN);
	if (txqi->log == NULL) {
		WL_ERROR(("wl%d: %s: MALLOC failed for TXQ log, malloced %d bytes\n",
		          txqi->pub->unit, __FUNCTION__, MALLOCED(txqi->osh)));
		txqi->log_len = 0;
	}
	/* register the dump fn for the txq log */
	wlc_dump_register(txqi->pub, "txqlog", wlc_txq_log_dump, (void *)txqi);
#endif /* TXQ_LOG */

	/* Get notified when an SCB is freed */
	wlc_scb_cubby_reserve(wlc, 0, wlc_txq_scb_init, wlc_txq_scb_deinit, NULL, wlc);
	/* Register the 1st txmod */
	wlc_txmod_fn_register(wlc->txmodi, TXMOD_TRANSMIT, wlc, txq_txmod_fns);

	/* Setup wlc_d11hdrs routine */
	if (FALSE) {
	}
#ifdef WL11AX
	else if (D11REV_GE(wlc->pub->corerev, 80)) {
		wlc->wlc_d11hdrs_fn = wlc_d11hdrs_rev80;
	}
#endif
	else if (D11REV_GE(wlc->pub->corerev, 40)) {
		wlc->wlc_d11hdrs_fn = wlc_d11hdrs_rev40;
	}
	else {
		wlc->wlc_d11hdrs_fn = wlc_d11hdrs_pre40;
	}

	return txqi;

fail:
	MODULE_DETACH(txqi, wlc_txq_detach);
	return NULL;
}

void
BCMATTACHFN(wlc_txq_detach)(txq_info_t *txqi)
{
	osl_t *osh;
	txq_t *txq, *next;

	if (txqi == NULL)
		return;

	osh = txqi->osh;

	for (txq = txqi->txq_list; txq != NULL; txq = next) {
		next = txq->next;
		wlc_low_txq_free(txqi, txq);
	}

	wlc_module_unregister(txqi->pub, "txq", txqi);

#ifdef TXQ_LOG
	/* free the debugging txq log */
	if (txqi->log != NULL) {
		MFREE(osh, txqi->log, sizeof(*txqi->log) * txqi->log_len);
		txqi->log = NULL;
	}
#endif /* TXQ_LOG */

	if (txqi->delq != NULL) {
		spktqdeinit(txqi->delq);
		MFREE(osh, txqi->delq, sizeof(*(txqi->delq)));
	}

	MFREE(osh, txqi, sizeof(txq_info_t));
}

static int
wlc_txq_scb_init(void *ctx, struct scb *scb)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;

	/* Init the basic feature tx path to regular tx function */
	wlc_txmod_config(wlc->txmodi, scb, TXMOD_TRANSMIT);
	return BCME_OK;
}

static void
wlc_txq_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_txq_info_t *qi;

	for (qi = wlc->tx_queues; qi != NULL; qi = qi->next) {
		/* Free the packets for this scb in low txq */
		wlc_low_txq_scb_flush(wlc, qi, scb);
		wlc_pktq_scb_free(wlc, WLC_GET_TXQ(qi), scb);
	}

	/* nullify scb pointer in the pkttag for pkts of this scb */
	wlc_delq_scb_free(wlc, scb);
}

int
wlc_txq_fc_verify(txq_info_t *txqi, txq_t *txq)
{
	uint fifo_idx;

	for (fifo_idx = 0; fifo_idx < txq->nfifo; fifo_idx++) {
		if (txq_stopped(txq, fifo_idx) && TXPKTPENDGET(txqi->wlc, fifo_idx) == 0) {
			WL_ERROR(("%s FIFO %d NOT stopped\n", __FUNCTION__,  fifo_idx));
			return FALSE;
		}
	}

	return TRUE;
}

void
wlc_block_datafifo(wlc_info_t *wlc, uint32 mask, uint32 val)
{
	uint8 block = wlc->block_datafifo;
	uint8 new_block;
#if defined(TXQ_MUX)
	uint8 scb_block_mask;
#endif  /* TXQ_MUX */

	/* the mask should specify some flag bits */
	ASSERT((mask & 0xFF) != 0);
	/* the value should specify only valid flag bits */
	ASSERT((val & 0xFFFFFF00) == 0);
	/* value should only have bits in the mask */
	ASSERT((val & ~mask) == 0);

	new_block = (block & ~mask) | val;

	/* just return if no change */
	if (block == new_block) {
		return;
	}
	wlc->block_datafifo = new_block;

#if defined(TXQ_MUX)
	/* mask for all block_datafifo reasons to stop scb tx */
	scb_block_mask = (DATA_BLOCK_CHANSW | DATA_BLOCK_QUIET | DATA_BLOCK_JOIN |
	                  DATA_BLOCK_TXCHAIN | DATA_BLOCK_SPATIAL);

	/* if scbs were blocked, but not now, start scb tx */
	if ((block & scb_block_mask) != 0 &&
	    (new_block & scb_block_mask) == 0) {
		wlc_scbq_global_stop_flag_clear(wlc->tx_scbq, SCBQ_FC_BLOCK_DATA);
#ifdef AP
		wlc_bcmc_global_start_mux_sources(wlc);
#endif
	}
	/* if scbs were not blocked, but are now, stop scb tx */
	if ((block & scb_block_mask) == 0 &&
	    (new_block & scb_block_mask) != 0) {
		wlc_scbq_global_stop_flag_set(wlc->tx_scbq, SCBQ_FC_BLOCK_DATA);
#ifdef AP
		wlc_bcmc_global_stop_mux_sources(wlc);
#endif
	}
#endif  /* TXQ_MUX */
} /* wlc_block_datafifo */

/** reduces tx packet buffering in the driver and balance the airtime access among flows */
void BCMFASTPATH
wlc_send_q(wlc_info_t *wlc, wlc_txq_info_t *qi)
{
	if (!wlc->pub->up) {
		return;
	}

	if ((qi == wlc->active_queue) &&
		!wlc->txfifo_detach_pending) {
		wlc_txq_fill(wlc->txqi, qi->low_txq);
	}
}

#if defined(WLATF_DONGLE)
/*
The 2 functions below is a wrapper + a local static function.
Wrapper is for a local version of the function to
make it globally visible
This places the function closest to the most frequently used call site
and is static in that context.

This is an optimization of the most frequently used call path,
at the expense of the less frequently used ones.

The scope is declared as static and the wrapper allows for it to be visible,
this gives the compiler some elbow room for optimization
when trading off space for performance.

This particular function has been cycle counted to yield a small difference
of 1-2% on the 4357 and this method should only be applied
when there is some supporting data to warrant its use.

The 4357 on 1024QAM is CPU cycle limited.
*/

static uint32 BCMFASTPATH
wlc_scb_dot11hdrsize_lcl(struct scb *scb)
{
	wlc_bsscfg_t *bsscfg = SCB_BSSCFG(scb);
	wlc_info_t *wlc = bsscfg->wlc;
	wlc_key_info_t key_info;
	uint32 len;

	len = DOT11_MAC_HDR_LEN + DOT11_FCS_LEN;

	if (SCB_QOS(scb))
		len += DOT11_QOS_LEN;

	if (SCB_A4_DATA(scb))
		len += ETHER_ADDR_LEN;

	wlc_keymgmt_get_scb_key(wlc->keymgmt, scb, WLC_KEY_ID_PAIRWISE,
			WLC_KEY_FLAG_NONE, &key_info);

	if (key_info.algo != CRYPTO_ALGO_OFF) {
		len += key_info.iv_len;
		len += key_info.icv_len;
		if (key_info.algo == CRYPTO_ALGO_TKIP)
			len += TKIP_MIC_SIZE;
	}

	return len;
}

uint32 BCMFASTPATH
wlc_scb_dot11hdrsize(struct scb *scb)
{
	return wlc_scb_dot11hdrsize_lcl(scb);
}

static ratespec_t BCMFASTPATH
wlc_ravg_get_scb_cur_rspec_lcl(wlc_info_t *wlc, struct scb *scb)
{
	ratespec_t cur_rspec = 0;

	if (!BCMPCIEDEV_ENAB()) {
		ASSERT(cur_rspec);
		return cur_rspec;
	}

	if (SCB_ISMULTI(scb) || SCB_INTERNAL(scb)) {
		if (RSPEC_ACTIVE(wlc->band->mrspec_override)) {
			cur_rspec = wlc->band->mrspec_override;
		} else {
			cur_rspec = scb->rateset.rates[0];
		}
	} else {
		cur_rspec = wlc_scb_ratesel_get_primary(wlc, scb, NULL);
	}

	ASSERT(cur_rspec);
	return cur_rspec;
}

ratespec_t BCMFASTPATH
wlc_ravg_get_scb_cur_rspec(wlc_info_t *wlc, struct scb *scb)
{
	return wlc_ravg_get_scb_cur_rspec_lcl(wlc, scb);
}

static void BCMFASTPATH
wlc_upd_flr_weight(wlc_atfd_t *atfd, wlc_info_t *wlc, struct scb* scb, void *p)
{
	uint8 fl;
	ratespec_t cur_rspec = 0;
	uint16 frmbytes = 0;

	if (BCMPCIEDEV_ENAB()) {
		frmbytes = pkttotlen(wlc->osh, p) +
			wlc_scb_dot11hdrsize_lcl(scb);
		fl = RAVG_PRIO2FLR(PRIOMAP(wlc), PKTPRIO(p));
		cur_rspec = wlc_ravg_get_scb_cur_rspec_lcl(wlc, scb);
		/* adding pktlen into the corresponding moving average buffer */
		RAVG_ADD(TXPKTLEN_RAVG(atfd, fl), TXPKTLEN_RBUF(atfd, fl),
			frmbytes, RAVG_EXP_PKT);
		/* adding weight into the corresponding moving average buffer */
		wlc_ravg_add_weight(atfd, fl, cur_rspec);
	}
}
#endif /* WLATF_DONGLE */

/*
 * Enqueues a packet on txq, then sends as many packets as possible.
 * Packets are enqueued to a maximum depth of WLC_DATAHIWAT*2.
 * tx flow control is turned on at WLC_DATAHIWAT, and off at WLC_DATAHIWAT/2.
 *
 * NOTE: it's important that the txq be able to accept at least
 * one packet after tx flow control is turned on, as this ensures
 * NDIS does not drop a packet.
 *
 * Returns TRUE if packet discarded and freed because txq was full, FALSE otherwise.
 */
bool BCMFASTPATH
wlc_sendpkt(wlc_info_t *wlc, void *sdu, struct wlc_if *wlcif)
{
	struct scb *scb = NULL;
	osl_t *osh;
	struct ether_header *eh;
	struct ether_addr *dst;
	wlc_bsscfg_t *bsscfg;
	struct ether_addr *wds = NULL;
	bool discarded = FALSE;
#ifdef WLTDLS
	struct scb *tdls_scb = NULL;
#endif
	void *pkt, *n;
	int8 bsscfgidx = -1;
	uint32 lifetime = 0;
#if defined(BCMDBG) || defined(WLMSG_INFORM)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif /* BCMDBG || WLMSG_INFORM */
#ifdef WL_RELMCAST
	uint8 flag = FALSE;
#endif
	wlc_bss_info_t *current_bss;
#if defined(PKTC) || defined(PKTC_TX_DONGLE)
	uint prec, next_fid;
#endif
	uint bandunit;
	wlc_key_info_t scb_key_info;
	wlc_key_info_t bss_key_info;
#ifdef WLATF_DONGLE
	ratespec_t cur_rspec = 0;
	uint8 fl = 0;
	wlc_atfd_t *atfd = NULL;
#endif

#ifdef WL_TX_STALL
	wlc_tx_status_t toss_reason = WLC_TX_STS_TOSS_UNKNOWN;
#endif
	WL_TRACE(("wlc%d: wlc_sendpkt\n", wlc->pub->unit));

	osh = wlc->osh;

	/* sanity */
	ASSERT(sdu != NULL);
	ASSERT(PKTLEN(osh, sdu) >= ETHER_HDR_LEN);

	if (PKTLEN(osh, sdu) < ETHER_HDR_LEN)
	{
		PKTCFREE(osh, sdu, TRUE);
		return TRUE;
	}

	/* figure out the bsscfg for this packet */
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);

	ASSERT(WLPKTTAG(sdu) != NULL);

	{

		eh = (struct ether_header*) PKTDATA(osh, sdu);

#ifdef ENABLE_CORECAPTURE
		if (ntoh16(eh->ether_type) ==  ETHER_TYPE_802_1X) {
			char dst[ETHER_ADDR_STR_LEN] = {0};
			char src[ETHER_ADDR_STR_LEN] = {0};
			WIFICC_LOGDEBUG(("wl%d: %s: 802.1x dst %s src %s\n",
				wlc->pub->unit, __FUNCTION__,
				bcm_ether_ntoa((struct ether_addr*)eh->ether_dhost, dst),
				bcm_ether_ntoa((struct ether_addr*)eh->ether_shost, src)));
		}
#endif /* ENABLE_CORECAPTURE */

		/* figure out the bsscfg for this packet */
		bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
		ASSERT(bsscfg != NULL);
	}

	/* fixup wlcif in case it is NULL */
	if (wlcif == NULL)
		wlcif = bsscfg->wlcif;
	ASSERT(wlcif != NULL);

	WL_APSTA_TX(("wl%d: wlc_sendpkt() pkt %p, len %u, wlcif %p type %d\n",
	             wlc->pub->unit, OSL_OBFUSCATE_BUF(sdu), pkttotlen(osh, sdu),
			OSL_OBFUSCATE_BUF(wlcif), wlcif->type));

	/* QoS map set */
	if (WL11U_ENAB(wlc)) {
		wlc_11u_set_pkt_prio(wlc->m11u, bsscfg, sdu);
	}

#ifdef WET
	/* Apply WET only on the STA interface */
	if (wlc->wet && BSSCFG_STA(bsscfg) &&
	    wlc_wet_send_proc(wlc->weth, sdu, &sdu) < 0) {
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_WET);
		goto toss;
	}
#endif /* WET */

	if (wlcif->type == WLC_IFTYPE_WDS) {
		scb = wlcif->u.scb;
		wds = &scb->ea;
	}

	/* discard if we're not up or not yet part of a BSS */
	if (!wlc->pub->up ||
	    (!wds &&
	     (!bsscfg->up ||
	      (BSSCFG_STA(bsscfg) && ETHER_ISNULLADDR(&bsscfg->BSSID))))) {
		WL_INFORM(("wl%d: %s: bsscfg %d is not permitted to transmit. "
		           "wlc up:%d bsscfg up:%d BSSID %s\n", wlc->pub->unit, __FUNCTION__,
		           WLC_BSSCFG_IDX(bsscfg), wlc->pub->up, bsscfg->up,
		           bcm_ether_ntoa(&bsscfg->BSSID, eabuf)));
		WLCNTINCR(wlc->pub->_cnt->txnoassoc);
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_BSSCFG_DOWN);
		goto toss_silently;
	}

#ifdef L2_FILTER
	if (L2_FILTER_ENAB(wlc->pub)) {
		if (wlc_l2_filter_send_data_frame(wlc, bsscfg, sdu) == 0) {
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_L2_FILTER);
			goto toss;
		}
	}
#endif /* L2_FILTER */

	if (!BSSCFG_SAFEMODE(bsscfg)) {
#ifdef WMF
		/* Do the WMF processing for multicast packets */
		if (!wds && WMF_ENAB(bsscfg) &&
		    (ETHER_ISMULTI(eh->ether_dhost) ||
		     (bsscfg->wmf_ucast_igmp && is_igmp(eh)))) {
			/* We only need to process packets coming from the stack/ds */
			switch (wlc_wmf_packets_handle(wlc->wmfi, bsscfg, NULL, sdu, 0)) {
			case WMF_TAKEN:
				/* The packet is taken by WMF return */
				return (FALSE);
			case WMF_DROP:
				/* Packet DROP decision by WMF. Toss it */
				WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_WMF_DROP);
				goto toss;
			default:
				/* Continue the transmit path */
				break;
			}
		}
#endif /* WMF */

#ifdef MAC_SPOOF
		/* in MAC Clone/Spoof mode, change our MAC address to be that of the original
		 * sender of the packet.  This will allow full layer2 bridging for that client.
		 * Note:  This is to be used in STA mode, and is not to be used with WET.
		 */
		if (wlc->mac_spoof &&
		    bcmp(&eh->ether_shost, &wlc->pub->cur_etheraddr, ETHER_ADDR_LEN)) {
			if (wlc->wet)
				WL_ERROR(("Configuration Error,"
					"MAC spoofing not supported in WET mode"));
			else {
				bcopy(&eh->ether_shost, &wlc->pub->cur_etheraddr, ETHER_ADDR_LEN);
				bcopy(&eh->ether_shost, &bsscfg->cur_etheraddr, ETHER_ADDR_LEN);
				WL_INFORM(("%s:  Setting WL device MAC address to %s",
					__FUNCTION__,
					bcm_ether_ntoa(&bsscfg->cur_etheraddr, eabuf)));
				wlc_set_mac(bsscfg);
			}
		}
#endif /* MAC_SPOOF */
	}

#ifdef WLWNM_AP
	/* Do the WNM processing */
	/* if packet was handled by DMS/FMS already, bypass it */
	if (BSSCFG_AP(bsscfg) && WLWNM_ENAB(wlc->pub) && WLPKTTAGSCBGET(sdu) == NULL) {
		/* try to process wnm related functions before sending packet */
		int ret = wlc_wnm_packets_handle(bsscfg, sdu, 1);
		switch (ret) {
		case WNM_TAKEN:
			/* The packet is taken by WNM return */
			return FALSE;
		case WNM_DROP:
			/* The packet drop decision by WNM free and return */
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_WNM_DROP);
			goto toss;
		default:
			/* Continue the forwarding path */
			break;
		}
	}
#endif /* WLWNM_AP */

#ifdef WET_TUNNEL
	if (BSSCFG_AP(bsscfg) && WET_TUNNEL_ENAB(wlc->pub)) {
		wlc_wet_tunnel_send_proc(wlc->wetth, sdu);
	}
#endif /* WET_TUNNEL */

#ifdef PSTA
	if (PSTA_ENAB(wlc->pub) && BSSCFG_STA(bsscfg)) {
		/* If a connection for the STAs or the wired clients
		 * doesn't exist create it. If connection already
		 * exists send the frames on it.
		 */
		if (wlc_psta_send_proc(wlc->psta, &sdu, &bsscfg) != BCME_OK) {
			WL_INFORM(("wl%d: tossing frame\n", wlc->pub->unit));
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_PSTA_DROP);
			goto toss;
		}
	}
#endif /* PSTA */

	if (wds)
		dst = wds;
	else if (BSSCFG_AP(bsscfg)) {
#ifdef WLWNM_AP
		/* Do the WNM processing */
		if (WLWNM_ENAB(wlc->pub) &&
		    wlc_wnm_dms_amsdu_on(wlc, bsscfg) &&
		    WLPKTTAGSCBGET(sdu) != NULL) {
			dst = &(WLPKTTAGSCBGET(sdu)->ea);
		} else
#endif /* WLWNM_AP */
		dst = (struct ether_addr*)eh->ether_dhost;
	} else {
		dst = bsscfg->BSS ? &bsscfg->BSSID : (struct ether_addr*)eh->ether_dhost;
#ifdef WL_RELMCAST
		/* increment rmc tx frame counter only when RMC is enabled */
		if (RMC_ENAB(wlc->pub) && ETHER_ISMULTI(dst))
			flag = TRUE;
#endif
#ifdef WLTDLS
		if (TDLS_ENAB(wlc->pub) && wlc_tdls_isup(wlc->tdls)) {
			if (memcmp(&bsscfg->BSSID, (void *)eh->ether_dhost, ETHER_ADDR_LEN)) {
				tdls_scb = wlc_tdls_query(wlc->tdls, bsscfg,
					sdu, (struct ether_addr*)eh->ether_dhost);
				if (tdls_scb) {
					dst = (struct ether_addr*)eh->ether_dhost;
					bsscfg = tdls_scb->bsscfg;
					wlcif = bsscfg->wlcif;
					WL_TMP(("wl%d:%s(): to dst %s, use TDLS, bsscfg=0x%p\n",
						wlc->pub->unit, __FUNCTION__,
						bcm_ether_ntoa(dst, eabuf),
						OSL_OBFUSCATE_BUF(bsscfg)));
					ASSERT(bsscfg != NULL);
				}
			}
		}
#endif /* WLTDLS */

	}

	current_bss = bsscfg->current_bss;

	bandunit = CHSPEC_WLCBANDUNIT(current_bss->chanspec);

	/* check IAPP L2 update frame */
	if (!wds && BSSCFG_AP(bsscfg) && ETHER_ISMULTI(dst)) {

		if ((ntoh16(eh->ether_type) == ETHER_TYPE_IAPP_L2_UPDATE) &&
			(WLPKTTAG(sdu)->flags & WLF_HOST_PKT)) {
			struct ether_addr *src;

			src = (struct ether_addr*)eh->ether_shost;

			/* cleanup the scb */
			if ((scb = wlc_scbfindband(wlc, bsscfg, src, bandunit)) != NULL) {
				WL_INFORM(("wl%d: %s: non-associated station %s\n", wlc->pub->unit,
					__FUNCTION__, bcm_ether_ntoa(src, eabuf)));
				wlc_bss_mac_event(wlc, bsscfg, WLC_E_DISASSOC_IND, &scb->ea,
					WLC_E_STATUS_SUCCESS, DOT11_RC_DISASSOC_LEAVING, 0, 0, 0);
				wlc_scbfree(wlc, scb);
			}
		}
	}
	/* Done with WLF_HOST_PKT flag; clear it now. This flag should not be
	 * used beyond this point as it is overloaded on WLF_FIFOPKT. It is
	 * set when pkt leaves the per port layer indicating it is coming from
	 * host or bridge.
	 */
	WLPKTTAG(sdu)->flags &= ~WLF_HOST_PKT;

	/* toss if station is not associated to the correct bsscfg. Make sure to use
	 * the band while looking up as we could be scanning on different band
	 */
	/* Class 3 (BSS) frame */
	if (!wds && bsscfg->BSS && !ETHER_ISMULTI(dst)) {
		if ((scb = wlc_scbfindband(wlc, bsscfg, dst, bandunit)) == NULL) {
			WL_INFORM(("wl%d: %s: invalid class 3 frame to "
				"non-associated station %s\n", wlc->pub->unit, __FUNCTION__,
				bcm_ether_ntoa(dst, eabuf)));
			WLCNTINCR(wlc->pub->_cnt->txnoassoc);
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_CLASS3_BSS);
			goto toss;
		}
	}
	/* Class 1 (IBSS/TDLS) or 4 (WDS) frame */
	else {
#if defined(WLTDLS)
		if ((TDLS_ENAB(wlc->pub)) && (tdls_scb != NULL))
			scb = tdls_scb;
		else
#endif /* defined(WLTDLS) */
#ifdef WL_NAN
		if (bsscfg && BSSCFG_NAN_DATA(bsscfg)) {
			if (!NAN_ENAB(wlc->pub)) {
				WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_BSSCFG_DOWN);
				goto toss;
			}
			if (ETHER_ISMULTI(dst))
				scb = WLC_BCMCSCB_GET(wlc, bsscfg);
			else
				scb = wlc_scbfind_dualband(wlc, bsscfg, dst);
			if (!scb) {
				WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_NULL_SCB1);
				goto toss;
			}
		}
#endif /* WL_NAN */

		if (ETHER_ISMULTI(dst)) {
			scb = WLC_BCMCSCB_GET(wlc, bsscfg);
			if (scb == NULL) {
				WL_ERROR(("wl%d: %s: invalid multicast frame\n",
				          wlc->pub->unit, __FUNCTION__));
				WLCNTINCR(wlc->pub->_cnt->txnoassoc);
				WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_INV_MCAST_FRAME);
				goto toss;
			}
		}
#ifdef WLMESH
		else if (WLMESH_ENAB(wlc->pub) && BSSCFG_MESH(bsscfg)) {
			int status;
			status = wlc_mroute_sendpkt(wlc->mesh_info, bsscfg, dst, &scb, bandunit,
					sdu);
			if (status == BCME_NORESOURCE) {
				/* scb is not found and not able to allocate new scb,
				 * hence going to toss.
				 */
				WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_NULL_SCB2);
				goto toss;
			} else if (status == BCME_NOTREADY) {
				/* Route not found, hence initiated create route.
				 * This packet is queued into mesh queues.
				 */
				return FALSE;
			}
		}
#endif /* WLMESH */
		else if (scb == NULL &&
		         (scb = wlc_scblookupband(wlc, bsscfg, dst, bandunit)) == NULL) {
			WL_ERROR(("wl%d: %s: out of scbs\n", wlc->pub->unit, __FUNCTION__));
			WLCNTINCR(wlc->pub->_cnt->txnobuf);
			/* Increment interface stats */
			if (wlcif) {
				WLCNTINCR(wlcif->_cnt->txnobuf);
			}
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_NO_BUF);
			goto toss;
		}
	}

	/* per-port code must keep track of WDS cookies */
	ASSERT(!wds || SCB_WDS(scb));

#if defined(WLTDLS)
	if (TDLS_ENAB(wlc->pub))
		ASSERT(!tdls_scb || BSSCFG_IS_TDLS(bsscfg));
#endif /* defined(WLTDLS) */

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
#ifdef WLTDLS
		bool tdls_action = FALSE;
		if (eh->ether_type == hton16(ETHER_TYPE_89_0D))
			tdls_action = TRUE;
#endif
#ifdef WLMCHAN
		if (MCHAN_ACTIVE(wlc->pub)) {
			/* Free up packets to non active queue */
			if (wlc->primary_queue) {
				wlc_bsscfg_t *other_cfg = wlc_mchan_get_other_cfg_frm_q(wlc,
					wlc->primary_queue);
				if ((other_cfg == scb->bsscfg) && !SCB_ISMULTI(scb)) {
					FOREACH_CHAINED_PKT(sdu, n) {
						/* Update tx stall counters */
						wlc_tx_status_update_counters(wlc, sdu,
							scb, scb->bsscfg,
							WLC_TX_STS_TOSS_UNKNOWN, 1);

						PKTCLRCHAINED(osh, sdu);
						if (WLFC_CONTROL_SIGNALS_TO_HOST_ENAB(wlc->pub))
							wlc_suppress_sync_fsm(wlc, scb, sdu, TRUE);
						wlc_process_wlhdr_txstatus(wlc,
							WLFC_CTL_PKTFLAG_WLSUPPRESS, sdu, FALSE);
						PKTFREE(wlc->pub->osh, sdu, TRUE);
					}
					return FALSE;
				}
			}
		}
#endif /* WLMCHAN */
		if (!SCB_ISMULTI(scb) && WLFC_CONTROL_SIGNALS_TO_HOST_ENAB(wlc->pub) &&
#ifdef WLTDLS
			!tdls_action &&
#endif
		1) {
			void *head = NULL, *tail = NULL;
			void *pktc_head = sdu;
			uint32 pktc_head_flags = WLPKTTAG(sdu)->flags;

			FOREACH_CHAINED_PKT(sdu, n) {
				if (wlc_suppress_sync_fsm(wlc, scb, sdu, FALSE)) {
						/* Update tx stall counters */
						wlc_tx_status_update_counters(wlc, sdu,
							scb, scb->bsscfg,
							WLC_TX_STS_SUPPRESS, 1);

					PKTCLRCHAINED(osh, sdu);
					/* wlc_suppress? */
					wlc_process_wlhdr_txstatus(wlc, WLFC_CTL_PKTFLAG_WLSUPPRESS,
						sdu, FALSE);
					PKTFREE(wlc->pub->osh, sdu, TRUE);
				} else {
					PKTCENQTAIL(head, tail, sdu);
				}
			}

			/* return if all packets are suppressed */
			if (head == NULL)
				return FALSE;
			sdu = head;

			if (pktc_head != sdu)
				WLPKTTAG(sdu)->flags = pktc_head_flags;
		}
	}
#endif /* PROP_TXSTATUS */

	{
		/* allocs headroom, converts to 802.3 frame */
		pkt = wlc_hdr_proc(wlc, sdu, scb);
	}

	/* Header conversion failed */
	if (pkt == NULL) {
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_HDR_CONV_FAILED);
		goto toss;
	}

	/*
	 * Header conversion is done and packet is in 802.3 format
	 * 802.3 format:
	 * -------------------------------------------
	 * |                   |   DA   |   SA   | L |
	 * -------------------------------------------
	 *                          6        6     2
	 */

#ifdef WLMESH
	if (WLMESH_ENAB(wlc->pub) && BSSCFG_MESH(bsscfg)) {
		wlc_mroute_add_meshctrlhdr(wlc->mesh_info, pkt, bsscfg);
	}
#endif /* WLMESH */

	{
		eh = (struct ether_header *)PKTDATA(osh, pkt);
		if (ntoh16(eh->ether_type) > DOT11_MAX_MPDU_BODY_LEN) {
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_FAILURE);
			PKTCFREE(wlc->osh, pkt, TRUE);
			goto toss;
		}
	}

	sdu = pkt;

	if (WLPKTTAG(pkt)->flags & WLF_8021X)
		WL_EVENT_LOG((EVENT_LOG_TAG_TRACE_WL_INFO, TRACE_FW_EAPOL_FRAME_TRANSMIT_START));

	/* early discard of non-8021x frames if keys are not plumbed */
	(void)wlc_keymgmt_get_tx_key(wlc->keymgmt, scb, bsscfg, &scb_key_info);
	(void)wlc_keymgmt_get_bss_tx_key(wlc->keymgmt, bsscfg, FALSE, &bss_key_info);
	if (!BSSCFG_SAFEMODE(bsscfg) &&
	    (WSEC_ENABLED(bsscfg->wsec) && !WSEC_SES_OW_ENABLED(bsscfg->wsec)) &&
	    !(WLPKTTAG(pkt)->flags & WLF_8021X) &&
		(scb_key_info.algo == CRYPTO_ALGO_OFF) && !(BYPASS_ENC_DATA(bsscfg)) &&
		(bss_key_info.algo == CRYPTO_ALGO_OFF)) {
		WL_ERROR(("wl%d: %s: tossing unencryptable frame, flags=0x%x\n",
			wlc->pub->unit, __FUNCTION__, WLPKTTAG(pkt)->flags));
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_CRYPTO_ALGO_OFF);
		goto toss;
	}

	bsscfgidx = WLC_BSSCFG_IDX(bsscfg);
	ASSERT(BSSCFGIDX_ISVALID(bsscfgidx));
	WLPKTTAGBSSCFGSET(sdu, bsscfgidx);
	WLPKTTAGSCBSET(sdu, scb);

#ifdef	WLCAC
	if (CAC_ENAB(wlc->pub)) {

		if (!wlc_cac_is_traffic_admitted(wlc->cac, WME_PRIO2AC(PKTPRIO(sdu)), scb)) {
			WL_CAC(("%s: Pkt dropped. Admission not granted for ac %d pktprio %d\n",
				__FUNCTION__, WME_PRIO2AC(PKTPRIO(sdu)), PKTPRIO(sdu)));
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_DROP_CAC_PKT);
			goto toss;
		}

		if (BSSCFG_AP(bsscfg) && !SCB_ISMULTI(scb)) {
			wlc_cac_reset_inactivity_interval(wlc->cac, WME_PRIO2AC(PKTPRIO(sdu)), scb);
		}
	}
#endif /* WLCAC */

	/* Set packet lifetime if configured */
	if (!(WLPKTTAG(sdu)->flags & WLF_EXPTIME) &&
	    (lifetime = wlc->lifetime[(SCB_WME(scb) ? WME_PRIO2AC(PKTPRIO(sdu)) : AC_BE)]))
		wlc_lifetime_set(wlc, sdu, lifetime);

	WLPKTTAG(sdu)->flags |= WLF_DATA;
#ifdef STA
	/* Restart the Dynamic Fast Return To Sleep sleep return timer if needed */
	wlc_update_sleep_ret(bsscfg, FALSE, TRUE, 0, pkttotlen(wlc->osh, sdu));
#ifdef WL11K
	if (WL11K_ENAB(wlc->pub)) {
		wlc_rrm_upd_data_activity_ts(wlc->rrm_info);
	}
#endif /* WL11K */
#endif /* STA */

#ifdef WL_TX_STALL
	{
		int pkt_count = 0;
		n = sdu;

		while (n != NULL) {
			n = PKTISCHAINED(n) ? PKTCLINK(n) : NULL;
			pkt_count++;
		}

		wlc_tx_status_update_counters(wlc, sdu, scb,
			bsscfg, WLC_TX_STS_QUEUED, pkt_count);
	}
#endif /* WL_TX_STALL */

#ifdef WLATF_DONGLE
	if (ATFD_ENAB(wlc)) {
		atfd = wlfc_get_atfd(wlc, scb);
	}
#endif


#if defined(PKTC) || defined(PKTC_TX_DONGLE)
	prec = WLC_PRIO_TO_PREC(PKTPRIO(sdu));
	next_fid = SCB_TXMOD_NEXT_FID(scb, TXMOD_START);

#ifdef WLAMSDU_TX
	if (AMSDU_TX_ENAB(wlc->pub) && PKTC_ENAB(wlc->pub) &&
	    (next_fid == TXMOD_AMSDU) &&
	    (SCB_TXMOD_NEXT_FID(scb, TXMOD_AMSDU) == TXMOD_AMPDU)) {
		/* When chaining and amsdu tx are enabled try doing AMSDU agg
		 * while queing the frames to per scb queues.
		 */
		WL_TRACE(("%s: skipping amsdu mod for %p chained %d\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(sdu), PKTISCHAINED(sdu)));
		SCB_TX_NEXT(TXMOD_AMSDU, scb, sdu, prec);
	} else
#endif /* WLAMSDU_TX */
		if ((next_fid == TXMOD_AMPDU) || (next_fid == TXMOD_NAR)) {
			WL_TRACE(("%s: ampdu mod for sdu %p chained %d\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(sdu), PKTISCHAINED(sdu)));
#if defined(WLATF_DONGLE)
			if (ATFD_ENAB(wlc)) {
				wlc_upd_flr_weight(atfd, wlc, scb, sdu);
			}
#endif /* WLATF_DONGLE */
			SCB_TX_NEXT(TXMOD_START, scb, sdu, prec);
		} else {
#if defined(WLATF_DONGLE)
			if (ATFD_ENAB(wlc)) {
				fl = RAVG_PRIO2FLR(PRIOMAP(wlc), PKTPRIO(sdu));
				cur_rspec = wlc_ravg_get_scb_cur_rspec_lcl(wlc, scb);
			}
#endif /* WLATF_DONGLE */
			/* Modules other than ampdu and NAR are not aware of chaining */
			FOREACH_CHAINED_PKT(sdu, n) {
				PKTCLRCHAINED(osh, sdu);
				if (n != NULL) {
					wlc_pktc_sdu_prep(wlc, scb, sdu, n, lifetime);
				}
#if defined(WLATF_DONGLE)
				if (ATFD_ENAB(wlc)) {
					uint16 frmbytes = pkttotlen(wlc->osh, sdu) +
						wlc_scb_dot11hdrsize_lcl(scb);
					/* add pktlen to the moving avg buffer */
					RAVG_ADD(TXPKTLEN_RAVG(atfd, fl),
						TXPKTLEN_RBUF(atfd, fl),
						frmbytes, RAVG_EXP_PKT);
				}
#endif /* WLATF_DONGLE */
				SCB_TX_NEXT(TXMOD_START, scb, sdu, prec);
			} /* FOREACH_CHAINED_PKT(sdu, n) */
#if defined(WLATF_DONGLE)
			if (ATFD_ENAB(wlc)) {
			/* adding weight into the moving average buffer */
					ASSERT(cur_rspec);
					wlc_ravg_add_weight(atfd, fl, cur_rspec);
			}
#endif /* WLATF_DONGLE */
		} /* (next_fid == TXMOD_AMPDU) */
#else /* PKTC */

#ifdef WLATF_DONGLE
	if (ATFD_ENAB(wlc)) {
		wlc_upd_flr_weight(atfd, wlc, scb, sdu);
	}
#endif /* WLATF_DONGLE */
	SCB_TX_NEXT(TXMOD_START, scb, sdu, WLC_PRIO_TO_PREC(PKTPRIO(sdu)));
#endif /* PKTC */

#ifdef WL_RELMCAST
	if (RMC_ENAB(wlc->pub) && flag) {
		wlc_rmc_tx_frame_inc(wlc);
	}
#endif
	if (WLC_TXQ_OCCUPIED(wlc)) {
		wlc_send_q(wlc, wlcif->qi);
	}

#ifdef WL_EXCESS_PMWAKE
	wlc->excess_pmwake->ca_txrxpkts++;
#endif /* WL_EXCESS_PMWAKE */

	return (FALSE);

toss:
	wlc_log_unexpected_tx_frame_log_8023hdr(wlc, eh);

toss_silently:
	FOREACH_CHAINED_PKT(sdu, n) {
		PKTCLRCHAINED(osh, sdu);

		/* Increment wme stats */
		if (WME_ENAB(wlc->pub)) {
			WLCNTINCR(wlc->pub->_wme_cnt->tx_failed[WME_PRIO2AC(PKTPRIO(sdu))].packets);
			WLCNTADD(wlc->pub->_wme_cnt->tx_failed[WME_PRIO2AC(PKTPRIO(sdu))].bytes,
			         pkttotlen(osh, sdu));
		}

#ifdef PROP_TXSTATUS
		if (PROP_TXSTATUS_ENAB(wlc->pub)) {
			wlc_process_wlhdr_txstatus(wlc, WLFC_CTL_PKTFLAG_TOSSED_BYWLC, sdu, FALSE);
		}
#endif /* PROP_TXSTATUS */


		WL_APSTA_TX(("wl%d: %s: tossing pkt %p\n",
			wlc->pub->unit, __FUNCTION__, OSL_OBFUSCATE_BUF(sdu)));

		discarded = TRUE;

#ifdef WL_TX_STALL
		/* Queued packet count is not incremented yet.
		 * Update both queued packet count and failure count
		 */
		if (toss_reason) {
			wlc_tx_status_update_counters(wlc, sdu,
				scb, bsscfg, WLC_TX_STS_QUEUED, 1);
			ASSERT(toss_reason != WLC_TX_STS_TOSS_UNKNOWN);
			wlc_tx_status_update_counters(wlc, sdu,
				scb, bsscfg, toss_reason, 1);
		}
#endif /* WL_TX_STALL */

		PKTFREE(osh, sdu, TRUE);
	}
	return (discarded);
} /* wlc_sendpkt */

#if defined(MBSS) && !defined(TXQ_MUX)

/**
 * Return true if packet got enqueued in BCMC PS packet queue.
 * This happens when the BSS is in transition from ON to OFF.
 * Called in prep_pdu and prep_sdu.
 */
static bool
bcmc_pkt_q_check(wlc_info_t *wlc, struct scb *bcmc_scb, wlc_pkt_t pkt)
{
	if (!MBSS_ENAB(wlc->pub) || !SCB_PS(bcmc_scb) ||
		!(bcmc_scb->bsscfg->flags & WLC_BSSCFG_PS_OFF_TRANS)) {
		/* No need to enqueue pkt to PS queue */
		return FALSE;
	}

	/* BSS is in PS transition from ON to OFF; Enqueue frame on SCB's PSQ */
	if (wlc_apps_bcmc_ps_enqueue(wlc, bcmc_scb, pkt) < 0) {
		WL_PS(("wl%d: Failed to enqueue BC/MC pkt for BSS %d\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bcmc_scb->bsscfg)));
		PKTFREE(wlc->osh, pkt, TRUE);
	}
	/* Force caller to give up packet and not tx */
	return TRUE;
}

#else
#define bcmc_pkt_q_check(wlc, bcmc_scb, pkt) FALSE
#endif /* MBSS */

/**
 * Common transmit packet routine, called when one or more SDUs in a queue are to be forwarded to
 * the d11 hardware. Called by e.g. wlc_send_q() and _wlc_sendampdu_aqm().
 *
 * Parameters:
 *     scb   : remote party to send the packets to
 *     *pkts : an array of SDUs (so no MPDUs)
 *
 * Return 0 when a packet is accepted and should not be referenced again by the caller -- wlc will
 * free it.
 * Return an error code when we're flow-controlled and the caller should try again later..
 *
 * - determines encryption key to use
 */
int BCMFASTPATH
wlc_prep_sdu(wlc_info_t *wlc, struct scb *scb, void **pkts, int *npkts, uint *fifop)
{
	uint i, j, nfrags, frag_length = 0, next_payload_len, fifo;
	void *sdu;
	uint offset;
	osl_t *osh;
	uint headroom, want_tailroom;
	uint pkt_length;
	uint8 prio = 0;
	struct ether_header *eh;
	bool is_8021x;
	bool fast_path;
	wlc_bsscfg_t *bsscfg;
	wlc_pkttag_t *pkttag;
#if defined(BCMDBG_ERR) || defined(WLMSG_INFORM)
	char eabuf[ETHER_ADDR_STR_LEN];
#endif
	bool is_tkip = FALSE;
	bool key_prep_sdu = FALSE;

#ifdef STA
	bool is_4way_m4 = FALSE;
#endif /* STA */
	wlc_key_t *key = NULL;
	wlc_key_info_t key_info;
#ifdef WL_TX_STALL
	wlc_tx_status_t toss_reason = WLC_TX_STS_TOSS_UNKNOWN;
#endif
	/* Make sure that we are passed in an SDU */
	sdu = pkts[0];
	ASSERT(sdu != NULL);
	pkttag = WLPKTTAG(sdu);
	ASSERT(pkttag != NULL);

	ASSERT((pkttag->flags & WLF_MPDU) == 0);

	osh = wlc->osh;

	ASSERT(scb != NULL);

	/* Something is blocking data packets */
	if (wlc->block_datafifo) {
		WL_INFORM(("wl%d: %s: block_datafifo 0x%x\n",
		           wlc->pub->unit, __FUNCTION__, wlc->block_datafifo));
		return BCME_BUSY;
	}

	is_8021x = (WLPKTTAG(sdu)->flags & WLF_8021X);

#ifndef TXQ_MUX
	/* The MBSS BCMC PS work is checks to see if
	 * bcmc pkts need to go on a bcmc BSSCFG PS queue,
	 * what is being added here *is* the bcmc bsscfg queue with TXQ_MUX.
	 */
	if (SCB_ISMULTI(scb) && bcmc_pkt_q_check(wlc, scb, sdu)) {
		/* Does BCMC pkt need to go to BSS's PS queue? */
		/* Force caller to give up packet and not tx */
		return BCME_NOTREADY;
	}
#endif

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	WL_APSTA_TX(("wl%d.%d: wlc_prep_sdu: pkt %p dst %s\n", wlc->pub->unit,
	             WLC_BSSCFG_IDX(bsscfg), OSL_OBFUSCATE_BUF(sdu),
	             bcm_ether_ntoa(&scb->ea, eabuf)));

	/* check enough headroom has been reserved */
	/* uncomment after fixing vx port
	 * ASSERT((uint)PKTHEADROOM(osh, sdu) < D11_TXH_LEN_EX(wlc));
	 */
	eh = (struct ether_header*) PKTDATA(osh, sdu);

	if (!wlc->pub->up) {
		WL_INFORM(("wl%d: wlc_prep_sdu: wl is not up\n", wlc->pub->unit));
		WLCNTINCR(wlc->pub->_cnt->txnoassoc);
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_WL_DOWN);
		goto toss;
	}

	/* toss if station is not associated to the correct bsscfg */
	if (bsscfg->BSS && !SCB_LEGACY_WDS(scb) && !SCB_ISMULTI(scb)) {
		/* Class 3 (BSS) frame */
		if (!SCB_ASSOCIATED(scb)) {
			WL_INFORM(("wl%d.%d: wlc_prep_sdu: invalid class 3 frame to "
				   "non-associated station %s\n", wlc->pub->unit,
				    WLC_BSSCFG_IDX(bsscfg), bcm_ether_ntoa(&scb->ea, eabuf)));
			WLCNTINCR(wlc->pub->_cnt->txnoassoc);
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_INV_CLASS3_NO_ASSOC);
			goto toss;
		}
	}

#if defined(TXQ_MUX)
	ASSERT(scb->bandunit == wlc->band->bandunit);
#else
	/* Toss the frame if scb's band does not match our current band
	 * this is possible if the STA has roamed while the packet was on txq
	 */
	if ((scb->bandunit != wlc->band->bandunit) &&
		!BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) &&
		!BSSCFG_NAN(SCB_BSSCFG(scb))) {
		/* different band */
		WL_INFORM(("wl%d.%d: frame destined to %s sent on incorrect band %d\n",
		           wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), bcm_ether_ntoa(&scb->ea, eabuf),
		           scb->bandunit));
		WLCNTINCR(wlc->pub->_cnt->txnoassoc);
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_INV_BAND);
		goto toss;
	}
#endif /* TXQ_MUX */

	if (wlc_scb_txfifo(wlc, scb, sdu, &fifo) != BCME_OK) {
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_DOWNGRADE_FAIL);
		goto toss;
	}
	if (SCB_QOS(scb) || SCB_WME(scb)) {
		/* prio could be different now */
		prio = (uint8)PKTPRIO(sdu);
	}
	*fifop = fifo;


	/* toss runt frames */
	pkt_length = pkttotlen(osh, sdu);
	if (pkt_length < ETHER_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN) {
		WLCNTINCR(wlc->pub->_cnt->txrunt);
		WLCIFCNTINCR(scb, txrunt);
		WLCNTSCBINCR(scb->scb_stats.tx_failures);
		WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_RUNT_FRAME);
		goto toss;
	}

	if (BSSCFG_AP(bsscfg) && !SCB_LEGACY_WDS(scb)) {
		/* Check if this is a packet indicating a STA roam and remove assoc state if so.
		 * Only check if we are getting the send on a non-wds link so that we do not
		 * process our own roam/assoc-indication packets
		 */
		if (ETHER_ISMULTI(eh->ether_dhost))
			if (wlc_roam_check(wlc->ap, bsscfg, eh, pkt_length)) {
				WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_MCAST_PKT_ROAM);
				goto toss;
			}
	}

	/* toss if station not yet authorized to receive non-802.1X frames */
	if (bsscfg->eap_restrict && !SCB_ISMULTI(scb) && !SCB_AUTHORIZED(scb)) {
		if (!is_8021x) {
			WL_ERROR(("wl%d.%d: non-802.1X frame to unauthorized station %s\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
			          bcm_ether_ntoa(&scb->ea, eabuf)));
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_STA_NOTAUTH);
			goto toss;
		}
	}


	WL_PRUSR("tx", (uchar*)eh, ETHER_HDR_LEN);

	if (WSEC_ENABLED(bsscfg->wsec) && !BSSCFG_SAFEMODE(bsscfg)) {
	    {
		if (is_8021x) {
			/*
			* use per scb WPA_auth to handle 802.1x frames differently
			* in WPA/802.1x mixed mode when having a pairwise key:
			*  - 802.1x frames are unencrypted in plain 802.1x mode
			*  - 802.1x frames are encrypted in WPA mode
			*/
			uint32 WPA_auth = SCB_LEGACY_WDS(scb) ? bsscfg->WPA_auth : scb->WPA_auth;

			key = wlc_keymgmt_get_tx_key(wlc->keymgmt, scb, bsscfg, &key_info);
			if ((WPA_auth != WPA_AUTH_DISABLED) &&
			    (key_info.algo != CRYPTO_ALGO_OFF)) {
				WL_WSEC(("wl%d.%d: wlc_prep_sdu: encrypting 802.1X frame using "
					"per-path key\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
			} else {
				/* Do not encrypt 802.1X packets with group keys */
				WL_WSEC(("wl%d.%d: wlc_prep_sdu: not encrypting 802.1x frame\n",
					wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
			}
		}
		else {
			/* Use a paired key or primary group key if present, toss otherwise */
			key = wlc_keymgmt_get_tx_key(wlc->keymgmt, scb, bsscfg, &key_info);
			if (key_info.algo != CRYPTO_ALGO_OFF) {
				WL_WSEC(("wl%d.%d: wlc_prep_sdu: using pairwise key\n",
					wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
			} else if ((key = wlc_keymgmt_get_bss_tx_key(wlc->keymgmt,
				bsscfg, FALSE, &key_info)) != NULL &&
				key_info.algo != CRYPTO_ALGO_OFF) {
				WL_WSEC(("wl%d.%d: wlc_prep_sdu: using default tx key\n",
				         wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
			} else {
				WL_WSEC(("wl%d.%d: wlc_prep_sdu: tossing unencryptable frame\n",
					wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
				WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_UNENCRYPT_FRAME2);
				goto toss;
			}
		}
	    }
	} else {	/* Do not encrypt packet */
		WL_WSEC(("wl%d.%d: wlc_prep_sdu: not encrypting frame, encryption disabled\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
		WL_WSEC(("wl%d.%d: wlc_prep_sdu: wsec 0x%x \n", wlc->pub->unit,
			WLC_BSSCFG_IDX(bsscfg), bsscfg->wsec));
	}

	/* ensure we have a valid (potentially null, with ALGO_OFF) key and key_info */
	if (key == NULL)
		key = wlc_keymgmt_get_bss_key(wlc->keymgmt, bsscfg, WLC_KEY_ID_INVALID, &key_info);

#if !defined(WLWSEC) || defined(WLWSEC_DISABLED)
	if (key == NULL) {
		memset(&key_info, 0, sizeof(wlc_key_info_t));
	}
#else
	ASSERT(key != NULL);
#endif
#ifdef STA
	if (is_8021x && !BSSCFG_SAFEMODE(bsscfg) &&
		wlc_keymgmt_b4m4_enabled(wlc->keymgmt, bsscfg)) {
		if (wlc_is_4way_msg(wlc, sdu, ETHER_HDR_LEN, PMSG4)) {
			WL_WSEC(("wl%d:%s(): Tx 4-way M4 pkt...\n", wlc->pub->unit, __FUNCTION__));
			is_4way_m4 = TRUE;
		}
	}
#endif /* STA */

	if (key_info.algo != CRYPTO_ALGO_OFF) {

		/* TKIP MIC space reservation */
		if (key_info.algo == CRYPTO_ALGO_TKIP) {
			is_tkip = TRUE;
			pkt_length += TKIP_MIC_SIZE;
		}
	}

	/* calculate how many frags in device memory needed
	 *  ETHER_HDR - (CKIP_LLC_MIC) - LLC/SNAP-ETHER_TYPE - payload - TKIP_MIC
	 */
	if (!WLPKTFLAG_AMSDU(pkttag) && !WLPKTFLAG_AMPDU(pkttag) && !BSSCFG_SAFEMODE(bsscfg))
		nfrags = wlc_frag(wlc, scb, WME_PRIO2AC(prio), pkt_length, &frag_length);
	else {
		frag_length = pkt_length - ETHER_HDR_LEN;
		nfrags = 1;
	}

#ifdef BCM_SFD
	/* Try later if SFD entry not available */
	if (SFD_ENAB(wlc->pub) && PKTISTXFRAG(osh, sdu)) {
		if (nfrags > wlc_sfd_entry_avail(wlc->sfd)) {
			return BCME_BUSY;
		}
	}
#endif
	fast_path = (nfrags == 1);

	/* prepare sdu (non ckip, which already added the header mic) */
	if (key_prep_sdu != TRUE) {
		(void)wlc_key_prep_tx_msdu(key, sdu, ((nfrags > 1) ? frag_length : 0), prio);
	}

	/*
	 * Prealloc all fragment buffers for this frame.
	 * If any of the allocs fail, free them all and bail.
	 * p->data points to ETHER_HEADER, original ether_type is passed separately
	 */
	offset = ETHER_HDR_LEN;
	for (i = 0; i < nfrags; i++) {
		headroom = TXOFF;
		want_tailroom = 0;

		if ((key_info.algo != CRYPTO_ALGO_OFF) && (!BYPASS_ENC_DATA(scb->bsscfg))) {
			if (WLC_KEY_SW_ONLY(&key_info) || WLC_KEY_IS_LINUX_CRYPTO(&key_info)) {
				headroom += key_info.iv_len;
				want_tailroom += key_info.icv_len;
				fast_path = FALSE;		/* no fast path for SW encryption */
			}

			if (is_tkip) {
				void *p;
				p = PKTNEXT(osh, sdu);
				if ((i + 2) >= nfrags) {
					WL_WSEC(("wl%d.%d: wlc_prep_sdu: checking "
						"space for TKIP tailroom\n",
						wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
					want_tailroom += TKIP_MIC_SIZE;
					if (p != NULL) {
						if (PKTNEXT(osh, p))
							fast_path = FALSE;
						else if ((uint)PKTTAILROOM(osh, p) < want_tailroom)
							fast_path = FALSE;
					} else if ((uint)PKTTAILROOM(osh, sdu) < want_tailroom)
						fast_path = FALSE;
				}

				/* Cloned TKIP packets to go via the slow path so that
				 * each interface gets a private copy of data to add MIC
				 */
				if (PKTSHARED(sdu) || (p && PKTSHARED(p)))
					fast_path = FALSE;
			}
		}

		if (fast_path) {
			/* fast path for non fragmentation/non copy of pkts
			 * header buffer has been allocated in wlc_sendpkt()
			 * don't change ether_header at the front
			 */
			/* tx header cache hit */
			if (WLC_TXC_ENAB(wlc) &&
			    !BSSCFG_SAFEMODE(bsscfg)) {
				wlc_txc_info_t *txc = wlc->txc;

				if (TXC_CACHE_ENAB(txc) &&
				    /* wlc_prep_sdu_fast may set WLF_TXCMISS... */
				    (pkttag->flags & WLF_TXCMISS) == 0 &&
				    (pkttag->flags & WLF_BYPASS_TXC) == 0) {
					if (wlc_txc_hit(txc, scb, sdu,
						((is_tkip && WLC_KEY_MIC_IN_HW(&key_info)) ?
						(pkt_length - TKIP_MIC_SIZE) : pkt_length),
						fifo, prio)) {
						wlc_txfast(wlc, scb, sdu, pkt_length,
							key, &key_info);
						WLCNTINCR(wlc->pub->_cnt->txchit);
						*npkts = 1;
						goto done;
					}
					pkttag->flags |= WLF_TXCMISS;
					WLCNTINCR(wlc->pub->_cnt->txcmiss);
				}
			}
			ASSERT(i == 0);
			pkts[0] = sdu;
		} else {
			/* before fragmentation make sure the frame contents are valid */
			PKTCTFMAP(osh, sdu);

			/* fragmentation:
			 * realloc new buffer for each frag, copy over data,
			 * append original ether_header at the front
			 */
#if defined(BCMPCIEDEV) && defined(BCMFRAGPOOL)
			if (BCMPCIEDEV_ENAB() && BCMLFRAG_ENAB() && PKTISTXFRAG(osh, sdu) &&
			    (PKTFRAGTOTLEN(osh, sdu) > 0)) {
				pkts[i] = wlc_allocfrag_txfrag(osh, sdu, offset, frag_length,
					(i == (nfrags-1)));
				wl_inform_additional_buffers(wlc->wl, nfrags);
			}
			else
#endif
				pkts[i] = wlc_allocfrag(osh, sdu, offset, headroom, frag_length,
					want_tailroom); /* alloc and copy */

			if (pkts[i] == NULL) {
				for (j = 0; j < i; j++) {
					PKTFREE(osh, pkts[j], TRUE);
				}
				pkts[0] = sdu;    /* restore pointer to sdu */
				WL_ERROR(("wl%d.%d: %s: allocfrag failed\n",
					wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
					__FUNCTION__));
				WLCNTINCR(wlc->pub->_cnt->txnobuf);
				WLCIFCNTINCR(scb, txnobuf);
				WLCNTSCBINCR(scb->scb_stats.tx_failures);
#if defined(BCMPCIEDEV) && defined(BCMLFRAG) && defined(PROP_TXSTATUS)
				if (BCMPCIEDEV_ENAB() && BCMLFRAG_ENAB() &&
					(PROP_TXSTATUS_ENAB(wlc->pub)))
					if (!wlc_suppress_recent_for_fragmentation(wlc,
						sdu, nfrags)) {
						/* If we could not find anything else to suppress,
						 * this must be only one lets suppress this instead
						 * of dropping it.
						 */
						wlc_process_wlhdr_txstatus(wlc,
							WLFC_CTL_PKTFLAG_WLSUPPRESS, sdu, FALSE);
						/* free up pkt */
						WL_TX_STS_UPDATE(toss_reason,
							WLC_TX_STS_TOSS_SUPR_PKT);
						goto toss;
					}
#endif /* BCMPCIEDEV && BCMLFRAG && PROP_TXSTATUS */

				return BCME_BUSY;
			} else {
				PKTPUSH(osh, pkts[i], ETHER_HDR_LEN);
				bcopy((char*)eh, (char*)PKTDATA(osh, pkts[i]), ETHER_HDR_LEN);

				/* Transfer SDU's pkttag info to the last fragment */
				if (i == (nfrags - 1)) {
					wlc_pkttag_info_move(wlc, sdu, pkts[i]);

					/* Reset original sdu's metadata, so that PKTFREE of
					 * original sdu does not send tx status prematurely
					 */
#ifdef BCMPCIEDEV
					if (BCMPCIEDEV_ENAB() && BCMLFRAG_ENAB() &&
					    PKTISTXFRAG(osh, sdu) && (PKTFRAGTOTLEN(osh, sdu) > 0))
						PKTRESETHASMETADATA(osh, sdu);
#endif
				}
			}

			/* set fragment priority */
			PKTSETPRIO(pkts[i], PKTPRIO(sdu));

			/* copy pkt lifetime */
			if ((WLPKTTAG(sdu)->flags & WLF_EXPTIME)) {
				WLPKTTAG(pkts[i])->flags |= WLF_EXPTIME;
				WLPKTTAG(pkts[i])->u.exptime = WLPKTTAG(sdu)->u.exptime;
			}

			/* leading frag buf must be aligned 0-mod-2 */
			ASSERT(ISALIGNED(PKTDATA(osh, pkts[i]), 2));
			offset += frag_length;
		}
	}

	/* build and transmit each fragment */
	for (i = 0; i < nfrags; i++) {
		if (i < nfrags - 1) {
			next_payload_len = pkttotlen(osh, pkts[i + 1]);
			ASSERT(next_payload_len >= ETHER_HDR_LEN);
			next_payload_len -= ETHER_HDR_LEN;
		} else {
			next_payload_len = 0;
		}

		WLPKTTAGSCBSET(pkts[i], scb);

		/* ether header is on front of each frag to provide the dhost and shost
		 *  ether_type may not be original, should not be used
		 */
		wlc_dofrag(wlc, pkts[i], i, nfrags, next_payload_len, scb,
			is_8021x, fifo, key, &key_info, prio, frag_length);

	}

	/* Free the original SDU if not shared */
	if (pkts[nfrags - 1] != sdu) {
		PKTFREE(osh, sdu, TRUE);
		sdu = pkts[nfrags - 1];
		BCM_REFERENCE(sdu);
	}

	*npkts = nfrags;

done:
#ifdef STA
	if (is_4way_m4)
		wlc_keymgmt_notify(wlc->keymgmt, WLC_KEYMGMT_NOTIF_M4_TX, bsscfg, scb, key, sdu);
#endif /* STA */

	WLCNTSCBINCR(scb->scb_stats.tx_pkts);
	WLCNTSCBADD(scb->scb_stats.tx_ucast_bytes, pkt_length);
	wlc_update_txpktsuccess_stats(wlc, scb, pkt_length, prio, 1);
	return 0;
toss:
	wlc_log_unexpected_tx_frame_log_8023hdr(wlc, eh);

	*npkts = 0;
	wlc_update_txpktfail_stats(wlc, pkttotlen(osh, sdu), prio);
#ifdef WL_TX_STALL
	ASSERT(toss_reason != WLC_TX_STS_TOSS_UNKNOWN);
#endif
	wlc_tx_status_update_counters(wlc, sdu, NULL, NULL, toss_reason, 1);

	PKTFREE(osh, sdu, TRUE);
	return BCME_ERROR;
} /* wlc_prep_sdu */

/**
 * Despite its name, wlc_txfast() does not transmit anything.
 * Driver fast path to send sdu using cached tx d11-phy-mac header (before
 * llc/snap).
 * Doesn't support wsec and eap_restrict for now.
 */
static void BCMFASTPATH
wlc_txfast(wlc_info_t *wlc, struct scb *scb, void *sdu, uint pktlen,
	wlc_key_t *key, const wlc_key_info_t *key_info)
{
	osl_t *osh = wlc->osh;
	struct ether_header *eh;
	uint8 *txh;
	uint16 seq = 0, frameid;
	struct dot11_header *h;
	struct ether_addr da, sa;
	uint fifo = 0;
	wlc_pkttag_t *pkttag;
	wlc_bsscfg_t *bsscfg;
	uint flags = 0;
	uint16 txc_hwseq, TxFrameID_off;
	uint d11TxdLen;

	ASSERT(scb != NULL);

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	ASSERT(ISALIGNED(PKTDATA(osh, sdu), 2));

	pkttag = WLPKTTAG(sdu);

	/* headroom has been allocated, may have llc/snap header already */
	/* uncomment after fixing osl_vx port
	 * ASSERT(PKTHEADROOM(osh, sdu) >= (TXOFF - DOT11_LLC_SNAP_HDR_LEN - ETHER_HDR_LEN));
	 */

	/* eh is going to be overwritten, save DA and SA for later fixup */
	eh = (struct ether_header*) PKTDATA(osh, sdu);
	eacopy((char*)eh->ether_dhost, &da);
	eacopy((char*)eh->ether_shost, &sa);

	/* strip off ether header, copy cached tx header onto the front of the frame */
	PKTPULL(osh, sdu, ETHER_HDR_LEN);
	ASSERT(WLC_TXC_ENAB(wlc));

	txh = (uint8*)wlc_txc_cp(wlc->txc, scb, sdu, &flags);

#if defined(WLC_UCODE_CACHE)
	if (D11REV_GE(wlc->pub->corerev, 42) && D11REV_LT(wlc->pub->corerev, 80))
		d11TxdLen = D11_TXH_SHORT_LEN(wlc);
	else
#endif /* WLC_UCODE_CACHE */
	if (SFD_ENAB(wlc->pub) && D11REV_LT(wlc->pub->corerev, 80)) {
		d11TxdLen = D11_TXH_LEN_SFD(wlc);
	} else {
		d11TxdLen = D11_TXH_LEN_EX(wlc);
	}

	/* txc_hit doesn't check DA,SA, fixup is needed for different configs */
	h = (struct dot11_header*) (((char*)txh) + d11TxdLen);

#ifdef WL11AX
	if (D11REV_GE(wlc->pub->corerev, 80)) {
		TxFrameID_off = OFFSETOF(d11txh_rev80_t, PktInfo.TxFrameID);
		txc_hwseq = (ltoh16(((d11txh_rev80_t *)txh)->PktInfo.MacTxControlLow)) &
			D11AC_TXC_ASEQ;
	} else
#endif /* WL11AX */
	if (D11REV_GE(wlc->pub->corerev, 40)) {
		TxFrameID_off = OFFSETOF(d11actxh_t, PktInfo.TxFrameID);
		txc_hwseq = (ltoh16(((d11actxh_t *)txh)->PktInfo.MacTxControlLow)) &
			D11AC_TXC_ASEQ;
	} else {
		TxFrameID_off = OFFSETOF(d11txh_t, TxFrameID);
		txc_hwseq = (ltoh16(((d11txh_t *)txh)->MacTxControlLow)) & TXC_HWSEQ;
	}

	if (!bsscfg->BSS) {
		/* IBSS, no need to fixup BSSID */
#ifdef WLMESH
		if (WLMESH_ENAB(wlc->pub) && BSSCFG_MESH(bsscfg)) {
			ASSERT((ltoh16(h->fc) & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS));
			eacopy((char*)&da, (char*)&h->a3);
			eacopy((char*)&sa, (char*)&h->a4);
		}
#endif
	} else if (SCB_A4_DATA(scb)) {
		/* wds: fixup a3 with DA, a4 with SA */
		ASSERT((ltoh16(h->fc) & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS));
		eacopy((char*)&da, (char*)&h->a3);
		eacopy((char*)&sa, (char*)&h->a4);
	} else if (BSSCFG_STA(bsscfg)) {
		/* normal STA to AP, fixup a3 with DA */
		eacopy((char*)&da, (char*)&h->a3);
	} else {
		/* normal AP to normal STA, fixup a3 with SA */
		eacopy((char*)&sa, (char*)&h->a3);
	}

#ifdef WLAMSDU_TX
	/* fixup qos control field to indicate it is an AMSDU frame */
	if (AMSDU_TX_ENAB(wlc->pub) && SCB_QOS(scb)) {
		uint16 *qos;
		qos = (uint16 *)((uint8 *)h + ((SCB_WDS(scb) || SCB_DWDS(scb)) ? DOT11_A4_HDR_LEN :
		                                              DOT11_A3_HDR_LEN));
		/* set or clear the A-MSDU bit */
		if (WLPKTFLAG_AMSDU(pkttag))
			*qos |= htol16(QOS_AMSDU_MASK);
		else
			*qos &= ~htol16(QOS_AMSDU_MASK);
	}
#endif

	/* fixup counter in TxFrameID */
	fifo = WLC_TXFID_GET_QUEUE(ltoh16(*(uint16 *)(txh + TxFrameID_off)));
	if (fifo == TX_BCMC_FIFO) { /* broadcast/multicast */
		frameid = bcmc_fid_generate(wlc, bsscfg, *(uint16 *)(txh + TxFrameID_off));
	} else {
		frameid = ltoh16(*(uint16 *)(txh + TxFrameID_off)) & ~TXFID_SEQ_MASK;
		seq = (wlc->counter++) << TXFID_SEQ_SHIFT;
		frameid |= (seq & TXFID_SEQ_MASK);
	}
	*(uint16 *)(txh + TxFrameID_off) = htol16(frameid);

	/* fix up the seqnum in the hdr */
#ifdef PROP_TXSTATUS
	if (IS_WL_TO_REUSE_SEQ(pkttag->seq)) {
		seq = WL_SEQ_GET_NUM(pkttag->seq) << SEQNUM_SHIFT;
		h->seq = htol16(seq);
	} else if (WLPKTFLAG_AMPDU(pkttag)) {
		seq = WL_SEQ_GET_NUM(pkttag->seq) << SEQNUM_SHIFT;
		h->seq = htol16(seq);
	}
#else
	if (WLPKTFLAG_AMPDU(pkttag)) {
		seq = pkttag->seq << SEQNUM_SHIFT;
		h->seq = htol16(seq);
	}
#endif /* PROP_TXSTATUS */

	else if (!txc_hwseq) {
		seq = SCB_SEQNUM(scb, PKTPRIO(sdu)) << SEQNUM_SHIFT;
		SCB_SEQNUM(scb, PKTPRIO(sdu))++;
		h->seq = htol16(seq);
	}

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub) && WLFC_GET_REUSESEQ(wlfc_query_mode(wlc->wlfc))) {
		WL_SEQ_SET_NUM(pkttag->seq, ltoh16(h->seq) >> SEQNUM_SHIFT);
		SET_WL_HAS_ASSIGNED_SEQ(pkttag->seq);
	}
#endif /* PROP_TXSTATUS */

	/* TDLS U-APSD Buffer STA: save Seq and TID for PIT */
#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) && wlc_tdls_buffer_sta_enable(wlc->tdls)) {
		uint16 fc;
		uint16 type;
		uint8 tid = 0;
		bool a4;

		fc = ltoh16(h->fc);
		type = FC_TYPE(fc);
		a4 = ((fc & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS));
		if ((type == FC_TYPE_DATA) && FC_SUBTYPE_ANY_QOS(FC_SUBTYPE(fc))) {
			uint16 qc;
			int offset = a4 ? DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN;

			qc = (*(uint16 *)((uchar *)h + offset));
			tid = (QOS_TID(qc) & 0x0f);
		}
		wlc_tdls_update_tid_seq(wlc->tdls, scb, tid, seq);
	}
#endif /* WLTDLS */

	if (D11REV_GE(wlc->pub->corerev, 40)) {
		uint tsoHdrSize = 0;

		/* Update timestamp */
		if ((pkttag->flags & WLF_EXPTIME)) {
			((d11actxh_t *)txh)->PktInfo.Tstamp =
			                htol16((pkttag->u.exptime >> D11AC_TSTAMP_SHIFT) & 0xffff);
			((d11actxh_t *)txh)->PktInfo.MacTxControlLow |= htol16(D11AC_TXC_AGING);
		}
		((d11actxh_t *)txh)->PktInfo.Seq = h->seq;

		/* Get the packet length after adding d11 header. */
		pktlen = pkttotlen(osh, sdu);
		/* Update frame len (fc to fcs) */
#ifdef WLTOEHW
		tsoHdrSize = (wlc->toe_bypass ?
			0 : wlc_tso_hdr_length((d11ac_tso_t*)PKTDATA(wlc->osh, sdu)));
#endif
		if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(wlc->osh, sdu)) {
			((d11actxh_t *)txh)->PktInfo.FrameLen =
				htol16((uint16)(pktlen +  DOT11_FCS_LEN - d11TxdLen - tsoHdrSize +
			                        (sizeof(d11actxh_rate_t) * D11AC_TXH_NUM_RATES) +
			                        sizeof(d11actxh_cache_t)));
		} else {
			((d11actxh_t *)txh)->PktInfo.FrameLen =
				htol16((uint16)(pktlen +  DOT11_FCS_LEN - d11TxdLen - tsoHdrSize));
		}
	} else {
		/* Update timestamp */
		if ((pkttag->flags & WLF_EXPTIME)) {
			((d11txh_t *)txh)->TstampLow = htol16(pkttag->u.exptime & 0xffff);
			((d11txh_t *)txh)->TstampHigh = htol16((pkttag->u.exptime >> 16) & 0xffff);
			((d11txh_t *)txh)->MacTxControlLow |= htol16(TXC_LIFETIME);
		}
	}

	if (CAC_ENAB(wlc->pub) && fifo <= TX_AC_VO_FIFO) {
		/* update cac used time with cached value */
		if (wlc_cac_update_used_time(wlc->cac, wme_fifo2ac[fifo], -1, scb))
			WL_ERROR(("wl%d: ac %d: txop exceeded allocated TS time\n",
			          wlc->pub->unit, wme_fifo2ac[fifo]));
	}

	/* update packet tag with the saved flags */
	pkttag->flags |= flags;

	wlc_txc_prep_sdu(wlc->txc, scb, key, key_info, sdu);

	if ((BSSCFG_AP(bsscfg) || BSS_TDLS_BUFFER_STA(bsscfg)) &&
		SCB_PS(scb) && (fifo != TX_BCMC_FIFO)) {
		if (AC_BITMAP_TST(scb->apsd.ac_delv, wme_fifo2ac[fifo]))
			wlc_apps_apsd_prepare(wlc, scb, sdu, h, TRUE);
		else
			wlc_apps_pspoll_resp_prepare(wlc, scb, sdu, h, TRUE);
	}

	/* (bsscfg specific): Add one more entry of the current rate to keep an accurate histogram.
	 * If the rate changed, we wouldn't be in the fastpath
	*/
	if (!bsscfg->txrspecidx) {
		bsscfg->txrspec[bsscfg->txrspecidx][0] = bsscfg->txrspec[NTXRATE-1][0]; /* rspec */
		bsscfg->txrspec[bsscfg->txrspecidx][1] = bsscfg->txrspec[NTXRATE-1][1]; /* nfrags */
	} else {
		bsscfg->txrspec[bsscfg->txrspecidx][0] = bsscfg->txrspec[bsscfg->txrspecidx - 1][0];
		bsscfg->txrspec[bsscfg->txrspecidx][1] = bsscfg->txrspec[bsscfg->txrspecidx - 1][1];
	}

	bsscfg->txrspecidx = (bsscfg->txrspecidx+1) % NTXRATE;

#if defined(PROP_TXSTATUS) && defined(WLFCTS)
#ifdef WLCNTSCB
	if (WLFCTS_ENAB(wlc->pub))
		pkttag->rspec = scb->scb_stats.tx_rate;
#endif
#endif /* PROP_TXSTATUS & WLFCTS */
} /* wlc_txfast */

/**
 * Common transmit packet routine, called when an sdu in a queue is to be
 * forwarded to he d11 hardware.
 * Called by e.g. wlc_send_q(),_wlc_sendampdu_aqm() and wlc_ampdu_output_aqm()
 *
 * Parameters:
 *     scb   : remote party to send the packets to
 *     **key : when key is not NULL, we already obtained the key & keyinfo
 *             we can bypass calling the expensive routines: wlc_txc_hit(),
 *             wlc_keymgmt_get_tx_key(), and wlc_keymgmt_get_bss_tx_key().
 *             The key shall be cleared in wlc_ampdu_output_aqm() when TID is
 *             changed. So this routine can reevaluate the wlc_tx_hit() call.
 *
 * Return 0 when a packet is accepted and should not be referenced again
 * by the caller -- wlc will free it.
 * When an error code is returned, wlc_prep_sdu ahall be called.
 */
int BCMFASTPATH
wlc_prep_sdu_fast(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct scb *scb,
	void *sdu, uint *fifop, wlc_key_t **key, wlc_key_info_t *key_info)
{
	uint pktlen;
	uint32 fifo;
	uint8 prio = 0;
	BCM_REFERENCE(bsscfg);

#ifdef WL_TX_STALL
	wlc_tx_status_t toss_reason = WLC_TX_STS_TOSS_UNKNOWN;
#endif

	ASSERT(SCB_AMPDU(scb));
	ASSERT(!SCB_ISMULTI(scb));

	/* On the AP, bcast/mcast always goes through the bcmc fifo if bcmc_scb
	 * is in PS Mode. bcmc_scb is in PS Mode if at least one ucast STA is in PS.
	 * SDUs are data packets and always go in BE_FIFO unless WME is enabled.
	 */
	prio = SCB_QOS(scb) ? (uint8)PKTPRIO(sdu) : 0;
	ASSERT(prio <= MAXPRIO);

	if (SCB_WME(scb)) {
		*fifop = fifo = prio2fifo[prio];
#ifdef WME
		if (wlc_wme_downgrade_fifo(wlc, &fifo, scb) == BCME_ERROR) {
			WL_TX_STS_UPDATE(toss_reason, WLC_TX_STS_TOSS_DOWNGRADE_FAIL);
			goto toss;
		}

		if (*fifop != fifo) {
			prio = fifo2prio[fifo];
			PKTSETPRIO(sdu, prio);
		}
#endif /* WME */
	} else
		fifo = TX_AC_BE_FIFO;

	*fifop = fifo;

	/* 1x or fragmented frames will force slow prep */
	pktlen = pkttotlen(wlc->osh, sdu);
	if ((WLPKTTAG(sdu)->flags &
	    (WLF_8021X
	     | 0)) || (((WLPKTTAG(sdu)->flags & WLF_AMSDU) == 0) &&
	      ((uint16)pktlen >= (wlc->fragthresh[WME_PRIO2AC(prio)] -
	      (uint16)(DOT11_A4_HDR_LEN + DOT11_QOS_LEN + DOT11_FCS_LEN + ETHER_ADDR_LEN))))) {
		return BCME_UNSUPPORTED;
	}

	/* TX header cache hit */
	if (WLC_TXC_ENAB(wlc)) {
		wlc_txc_info_t *txc = wlc->txc;

		if (TXC_CACHE_ENAB(txc) &&
		    (WLPKTTAG(sdu)->flags & WLF_BYPASS_TXC) == 0) {
			ASSERT((WLPKTTAG(sdu)->flags & WLF_TXCMISS) == 0);
			if (*key != NULL) {
				goto gotkey;
			}
			if (wlc_txc_hit(txc, scb, sdu, pktlen, fifo, prio)) {
				*key = wlc_keymgmt_get_tx_key(wlc->keymgmt, scb, bsscfg, key_info);
				if (key_info->algo == CRYPTO_ALGO_OFF) {
					*key = wlc_keymgmt_get_bss_tx_key(wlc->keymgmt,
						bsscfg, FALSE, key_info);
				}

				if (key_info->algo != CRYPTO_ALGO_OFF &&
					WLC_KEY_SW_ONLY(key_info)) {
					return BCME_UNSUPPORTED;
				}

gotkey:
				WLCNTINCR(wlc->pub->_cnt->txchit);
				wlc_txfast(wlc, scb, sdu, pktlen, *key, key_info);
				goto done;
			}
			WLPKTTAG(sdu)->flags |= WLF_TXCMISS;
			WLCNTINCR(wlc->pub->_cnt->txcmiss);
		}
	}

	return BCME_UNSUPPORTED;

done:
	WLCNTSCBINCR(scb->scb_stats.tx_pkts);
	WLCNTSCBADD(scb->scb_stats.tx_ucast_bytes, pktlen);
	wlc_update_txpktsuccess_stats(wlc, scb, pktlen, prio, 1);
	return BCME_OK;

#ifdef WME
toss:
	wlc_update_txpktfail_stats(wlc, pkttotlen(wlc->osh, sdu), prio);

#ifdef WL_TX_STALL
	ASSERT(toss_reason != WLC_TX_STS_TOSS_UNKNOWN);
#endif
	wlc_tx_status_update_counters(wlc, sdu, NULL, NULL, toss_reason, 1);

	PKTFREE(wlc->osh, sdu, TRUE);
	return BCME_ERROR;
#endif /* WME */
} /* wlc_prep_sdu_fast */

#if defined(TXQ_MUX)
void BCMFASTPATH
wlc_txfifo_complete(wlc_info_t *wlc, uint fifo, uint16 txpktpend, int txpkttime)
#else
void BCMFASTPATH
wlc_txfifo_complete(wlc_info_t *wlc, uint fifo, uint16 txpktpend)
#endif /* TXQ_MUX */
{
	txq_t *txq;
#ifdef STA
	wlc_bsscfg_t *cfg;
#endif /* STA */

	TXPKTPENDDEC(wlc, fifo, txpktpend);

	WL_TRACE(("wlc_txfifo_complete, pktpend dec %d to %d\n", txpktpend,
		TXPKTPENDGET(wlc, fifo)));

	/* track time per fifo */
	txq = (wlc->txfifo_detach_transition_queue != NULL) ?
		wlc->txfifo_detach_transition_queue->low_txq : wlc->active_queue->low_txq;

#if defined(TXQ_MUX)
	wlc_txq_complete(wlc->txqi, txq, fifo, txpktpend, txpkttime);
#else
	wlc_txq_complete(wlc->txqi, txq, fifo, txpktpend, txpktpend);
#endif

	ASSERT(TXPKTPENDGET(wlc, fifo) >= 0);

	if (wlc->block_datafifo && TXPKTPENDTOT(wlc) == 0) {
		if (wlc->block_datafifo & DATA_BLOCK_TX_SUPR)
			wlc_bsscfg_tx_check(wlc->psqi);

		if (wlc->block_datafifo & DATA_BLOCK_PS)
			wlc_apps_process_pend_ps(wlc);

#ifdef WL11N
		if ((wlc->block_datafifo & DATA_BLOCK_TXCHAIN)) {
#ifdef WL_STF_ARBITRATOR
			if (WLC_STF_ARB_ENAB(wlc->pub)) {
				wlc_stf_arb_handle_txfifo_complete(wlc);
			} else
#endif /* WL_STF_ARBITRATOR */
			if (wlc->stf->txchain_pending) {
				wlc_stf_txchain_set_complete(wlc);
			}
			/* remove the fifo block now */
			wlc_block_datafifo(wlc, DATA_BLOCK_TXCHAIN, 0);

#ifdef WL_MODESW
			if (WLC_MODESW_ENAB(wlc->pub))
				wlc_modesw_bsscfg_complete_downgrade(wlc->modesw);
#endif /* WL_MODESW */
		}
		if ((wlc->block_datafifo & DATA_BLOCK_SPATIAL)) {
			wlc_stf_spatialpolicy_set_complete(wlc);
			wlc_block_datafifo(wlc, DATA_BLOCK_SPATIAL, 0);
		}
#endif /* WL11N */
	}

#ifdef BCM_DMA_CT
	if (BCM_DMA_CT_ENAB(wlc) && wlc->cfg->pm->PM != PM_OFF) {
		if ((D11REV_IS(wlc->pub->corerev, 65)) &&
			(pktq_empty(WLC_GET_TXQ(wlc->cfg->wlcif->qi)) &&
			(wlc->hw->wake_override & WLC_WAKE_OVERRIDE_CLKCTL))) {
			wlc_ucode_wake_override_clear(wlc->hw, WLC_WAKE_OVERRIDE_CLKCTL);
		}
	}
#endif /* BCM_DMA_CT */

	if (((D11REV_IS(wlc->pub->corerev, 41)) ||
		(D11REV_IS(wlc->pub->corerev, 44)) ||
		(D11REV_IS(wlc->pub->corerev, 59))) &&
		(pktq_empty(&wlc->cfg->wlcif->qi->txq) &&
		(TXPKTPENDTOT(wlc) == 0) &&
		(wlc->user_wake_req & WLC_USER_WAKE_REQ_TX))) {
		mboolclr(wlc->user_wake_req, WLC_USER_WAKE_REQ_TX);
		wlc_set_wake_ctrl(wlc);
	}

	/* Clear MHF2_TXBCMC_NOW flag if BCMC fifo has drained */
	if (AP_ENAB(wlc->pub) &&
	    wlc->bcmcfifo_drain && TXPKTPENDGET(wlc, TX_BCMC_FIFO) == 0) {
		wlc->bcmcfifo_drain = FALSE;
		wlc_mhf(wlc, MHF2, MHF2_TXBCMC_NOW, 0, WLC_BAND_AUTO);
	}

#ifdef STA
	/* figure out which bsscfg is being worked on... */

	cfg = AS_IN_PROGRESS_CFG(wlc);
	if (cfg == NULL)
		return;

	if (cfg->assoc->state == AS_WAIT_TX_DRAIN &&
	    pktq_empty(WLC_GET_TXQ(cfg->wlcif->qi))&&
	    TXPKTPENDTOT(wlc) == 0) {
		wl_del_timer(wlc->wl, cfg->assoc->timer);
		wl_add_timer(wlc->wl, cfg->assoc->timer, 0, 0);
	}
#endif	/* STA */
} /* wlc_txfifo_complete */

/**
 * Called by wlc_prep_pdu(). Create the d11 hardware txhdr for an MPDU packet that does not (yet)
 * have a txhdr. The packet begins with a tx_params structure used to supply some parameters to
 * wlc_d11hdrs().
 */
static void
wlc_pdu_txhdr(wlc_info_t *wlc, void *p, struct scb *scb, wlc_txh_info_t *txh_info)
{
	wlc_pdu_tx_params_t tx_params;
	wlc_key_info_t key_info;
	memset(&key_info, 0, sizeof(wlc_key_info_t));

	/* pull off the saved tx params */
	memcpy(&tx_params, PKTDATA(wlc->osh, p), sizeof(wlc_pdu_tx_params_t));
	PKTPULL(wlc->osh, p, sizeof(wlc_pdu_tx_params_t));

	/* add headers */
	wlc_key_get_info(tx_params.key, &key_info);
	wlc_d11hdrs(wlc, p, scb, tx_params.flags, 0, 1, tx_params.fifo, 0,
	            &key_info, tx_params.rspec_override, NULL);

	wlc_get_txh_info(wlc, p, txh_info);
#ifdef WLWSEC
	/* management frame protection */
	if (key_info.algo != CRYPTO_ALGO_OFF) {
		uint d11h_off;
		int err;

		d11h_off = (uint)((uint8 *)txh_info->d11HdrPtr - (uint8 *)PKTDATA(wlc->osh, p));

		PKTPULL(wlc->osh, p, d11h_off);
		err  = wlc_key_prep_tx_mpdu(tx_params.key, p, txh_info->hdrPtr);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: %s: wlc_key_prep_tx_mpdu failed with status %d\n",
				wlc->pub->unit, __FUNCTION__, err));
		}
		PKTPUSH(wlc->osh, p, d11h_off);
	}
#endif /* WLWSEC */
}

/**
 * Prepares PDU for transmission. Called by e.g. wlc_send_q() and _wlc_sendampdu_aqm().
 * Returns a (BCM) error code when e.g. something is blocking the transmission of the PDU.
 * Adds a D11 header to the PDU if not yet present.
 */
int
wlc_prep_pdu(wlc_info_t *wlc, struct scb *scb, void *pdu, uint *fifop)
{
	uint fifo;
	wlc_txh_info_t txh_info;
	wlc_bsscfg_t *cfg;
	wlc_pkttag_t *pkttag;

	WL_TRACE(("wl%d: %s()\n", wlc->pub->unit, __FUNCTION__));

	pkttag = WLPKTTAG(pdu);
	ASSERT(pkttag != NULL);

	/* Make sure that it's a PDU */
	ASSERT(pkttag->flags & WLF_MPDU);

	ASSERT(scb != NULL);

	cfg = SCB_BSSCFG(scb);
	BCM_REFERENCE(cfg);
	ASSERT(cfg != NULL);

#ifndef TXQ_MUX
	/* The MBSS BCMC PS work checks to see if
	 * bcmc pkts need to go on a bcmc BSSCFG PS queue,
	 * what is being added here *is* the bcmc bsscfg queue with TXQ_MUX.
	 */
	if (SCB_ISMULTI(scb) && bcmc_pkt_q_check(wlc, scb, pdu)) {
		/* Does BCMC pkt need to go to BSS's PS queue? */
		/* Force caller to give up packet and not tx */
		return BCME_NOTREADY;
	}
#endif

	/* Drop the frame if it's not on the current band */
	/* Note: We drop it instead of returning as this can result in head-of-line
	 * blocking for Probe request frames during the scan
	 */
	if (scb->bandunit != CHSPEC_WLCBANDUNIT(WLC_BAND_PI_RADIO_CHANSPEC)) {
		PKTFREE(wlc->osh, pdu, TRUE);
		WLCNTINCR(wlc->pub->_cnt->txchanrej);
		return BCME_BADBAND;
	}

	/* Something is blocking data packets */
	if (wlc->block_datafifo & DATA_BLOCK_PS)
		return BCME_BUSY;

	/* add the txhdr if not present */
	if ((pkttag->flags & WLF_TXHDR) == 0)
		wlc_pdu_txhdr(wlc, pdu, scb, &txh_info);
	else
		wlc_get_txh_info(wlc, pdu, &txh_info);

	/*
	 * If the STA is in PS mode, this must be a PS Poll response or APSD delivery frame;
	 * fix the MPDU accordingly.
	 */
	if ((BSSCFG_AP(cfg) || BSS_TDLS_ENAB(wlc, cfg)) && SCB_PS(scb)) {
		/* TxFrameID can be updated for multi-cast packets */
		wlc_apps_ps_prep_mpdu(wlc, pdu, &txh_info);
	}


	/* get the pkt queue info. This was put at wlc_sendctl or wlc_send for PDU */
	fifo = WLC_TXFID_GET_QUEUE(ltoh16(txh_info.TxFrameID));
	*fifop = fifo;

	if (ltoh16(((struct dot11_header *)(txh_info.d11HdrPtr))->fc) != FC_TYPE_DATA)
		WLCNTINCR(wlc->pub->_cnt->txctl);

	return 0;
} /* wlc_prep_pdu */

static void
wlc_update_txpktfail_stats(wlc_info_t *wlc, uint32 pkt_len, uint8 prio)
{
	if (WME_ENAB(wlc->pub)) {
		WLCNTINCR(wlc->pub->_wme_cnt->tx_failed[WME_PRIO2AC(prio)].packets);
		WLCNTADD(wlc->pub->_wme_cnt->tx_failed[WME_PRIO2AC(prio)].bytes, pkt_len);
	}
}

void BCMFASTPATH
wlc_update_txpktsuccess_stats(wlc_info_t *wlc, struct scb *scb, uint32 pkt_len, uint8 prio,
                              uint16 npkts)
{
	/* update stat counters */
	WLCNTADD(wlc->pub->_cnt->txframe, npkts);
	WLCNTADD(wlc->pub->_cnt->txbyte, pkt_len);
	WLPWRSAVETXFADD(wlc, npkts);
#ifdef WLLED
	wlc_led_start_activity_timer(wlc->ledh);
#endif

	/* update interface stat counters */
	WLCNTADD(SCB_WLCIFP(scb)->_cnt->txframe, npkts);
	WLCNTADD(SCB_WLCIFP(scb)->_cnt->txbyte, pkt_len);

	if (WME_ENAB(wlc->pub)) {
		WLCNTADD(wlc->pub->_wme_cnt->tx[WME_PRIO2AC(prio)].packets, npkts);
		WLCNTADD(wlc->pub->_wme_cnt->tx[WME_PRIO2AC(prio)].bytes, pkt_len);
	}
}

/**
 * Convert 802.3 MAC header to 802.11 MAC header (data only)
 * and add WEP IV information if enabled.
 */
static struct dot11_header * BCMFASTPATH
wlc_80211hdr(wlc_info_t *wlc, void *p, struct scb *scb,
	bool MoreFrag, const wlc_key_info_t *key_info, uint8 prio, uint16 *pushlen, uint fifo)
{
	struct ether_header *eh;
	struct dot11_header *h;
	struct ether_addr tmpaddr;
	uint16 offset;
	struct ether_addr *ra;
	wlc_pkttag_t *pkttag;
	bool a4;
	osl_t *osh = wlc->osh;
	wlc_bsscfg_t *bsscfg;
	uint16 fc = 0;
	BCM_REFERENCE(fifo);

	ASSERT(scb != NULL);

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	osh = wlc->osh;
	BCM_REFERENCE(osh);
	pkttag = WLPKTTAG(p);
	eh = (struct ether_header *) PKTDATA(osh, p);
	/* Only doing A3 frame if 802.3 frame's destination address
	 * matches this SCB's address.
	 */
	if (pkttag->flags & WLF_8021X)
		a4 = SCB_DWDS(scb) ?
		(bcmp(&eh->ether_dhost, &scb->ea, ETHER_ADDR_LEN) != 0) : (SCB_A4_8021X(scb));
	else
		a4 = (SCB_A4_DATA(scb) != 0);
	ra = (a4 ? &scb->ea : NULL);

	/* convert 802.3 header to 802.11 header */
	/* Make room for 802.11 header, add additional bytes if WEP enabled for IV */
	offset = DOT11_A3_HDR_LEN - ETHER_HDR_LEN;
	if (a4)
		offset += ETHER_ADDR_LEN;
	if (SCB_QOS(scb))
		offset += DOT11_QOS_LEN;

	ASSERT(key_info != NULL);
	if (!WLC_KEY_IS_LINUX_CRYPTO(key_info) && (!BYPASS_ENC_DATA(bsscfg)))
		offset += key_info->iv_len;

	h = (struct dot11_header *) PKTPUSH(osh, p, offset);
	bzero((char*)h, offset);

	*pushlen = offset + ETHER_HDR_LEN;

	if (a4) {
		ASSERT(ra != NULL);
		/* WDS: a1 = RA, a2 = TA, a3 = DA, a4 = SA, ToDS = 0, FromDS = 1 */
		bcopy((char*)ra, (char*)&h->a1, ETHER_ADDR_LEN);
		bcopy((char*)&bsscfg->cur_etheraddr, (char*)&h->a2, ETHER_ADDR_LEN);
		/* eh->ether_dhost and h->a3 may overlap */
#ifdef WLWSEC
		if (key_info->algo != CRYPTO_ALGO_OFF || SCB_QOS(scb)) {
			/* In WEP case, &h->a3 + 4 = &eh->ether_dhost due to IV offset, thus
			 * need to bcopy
			 */
			bcopy((char*)&eh->ether_dhost, (char*)&tmpaddr, ETHER_ADDR_LEN);
			bcopy((char*)&tmpaddr, (char*)&h->a3, ETHER_ADDR_LEN);
		}
#endif
		/* eh->ether_shost and h->a4 may overlap */
		bcopy((char*)&eh->ether_shost, (char*)&tmpaddr, ETHER_ADDR_LEN);
		bcopy((char*)&tmpaddr, (char*)&h->a4, ETHER_ADDR_LEN);
		fc |= FC_TODS | FC_FROMDS;

		/* A-MSDU: use BSSID for A3 and A4, only need to fix up A3 */
		if (WLPKTFLAG_AMSDU(pkttag))
			bcopy((char*)&bsscfg->BSSID, (char*)&h->a3, ETHER_ADDR_LEN);
	} else if (BSSCFG_AP(bsscfg)) {
		ASSERT(ra == NULL);
		/* AP: a1 = DA, a2 = BSSID, a3 = SA, ToDS = 0, FromDS = 1 */
		bcopy(eh->ether_dhost, h->a1.octet, ETHER_ADDR_LEN);
		bcopy(bsscfg->BSSID.octet, h->a2.octet, ETHER_ADDR_LEN);
		/* eh->ether_shost and h->a3 may overlap */
#ifdef WLWSEC
		if (key_info->algo != CRYPTO_ALGO_OFF || SCB_QOS(scb)) {
			/* In WEP case, &h->a3 + 4 = &eh->ether_shost due to IV offset,
			 * thus need to bcopy
			 */
			bcopy((char*)&eh->ether_shost, (char*)&tmpaddr, ETHER_ADDR_LEN);
			bcopy((char*)&tmpaddr, (char*)&h->a3, ETHER_ADDR_LEN);
		}
#endif
		fc |= FC_FROMDS;
	}
#ifdef WLMESH
	else if (WLMESH_ENAB(wlc->pub) && BSSCFG_MESH(bsscfg)) {
		if (ETHER_ISBCAST(eh->ether_dhost)) {
			bcopy((char*)&eh->ether_dhost, (char*)&h->a1, ETHER_ADDR_LEN);
			bcopy((char*)&bsscfg->BSSID, (char*)&h->a2, ETHER_ADDR_LEN);
			bcopy((char*)&eh->ether_shost, (char*)&h->a3, ETHER_ADDR_LEN);
			fc |= FC_FROMDS;
		} else {
			h = (struct dot11_header *) PKTPUSH(osh, p, ETHER_ADDR_LEN);
			bzero((char*)h, ETHER_ADDR_LEN);
			*pushlen = *pushlen + ETHER_ADDR_LEN;
			ra = &scb->ea;
			a4 = TRUE;
			/* a1=RA, a2=TA, a3=Mesh DA, a4= Mesh SA, ToDS=1, FromDS=1 */
			bcopy((char*)ra, (char*)&h->a1, ETHER_ADDR_LEN);
			bcopy((char*)&bsscfg->cur_etheraddr, (char*)&h->a2, ETHER_ADDR_LEN);
			bcopy((char*)&eh->ether_dhost, (char*)&tmpaddr, ETHER_ADDR_LEN);
			bcopy((char*)&tmpaddr, (char*)&h->a3, ETHER_ADDR_LEN);
			bcopy((char*)&eh->ether_shost, (char*)&tmpaddr, ETHER_ADDR_LEN);
			bcopy((char*)&tmpaddr, (char*)&h->a4, ETHER_ADDR_LEN);
			fc |= FC_TODS | FC_FROMDS;
		}
	}
#endif /* WLMESH */
	else {
		ASSERT(ra == NULL);
		if (bsscfg->BSS) {
			/* BSS STA: a1 = BSSID, a2 = SA, a3 = DA, ToDS = 1, FromDS = 0 */
			bcopy((char*)&bsscfg->BSSID, (char*)&h->a1, ETHER_ADDR_LEN);
			bcopy((char*)&eh->ether_dhost, (char*)&tmpaddr, ETHER_ADDR_LEN);
			bcopy((char*)&eh->ether_shost, (char*)&h->a2, ETHER_ADDR_LEN);
			bcopy((char*)&tmpaddr, (char*)&h->a3, ETHER_ADDR_LEN);
			fc |= FC_TODS;
		} else {
			/* IBSS/TDLS STA: a1 = DA, a2 = SA, a3 = BSSID, ToDS = 0, FromDS = 0 */
			bcopy((char*)&eh->ether_dhost, (char*)&h->a1, ETHER_ADDR_LEN);
			bcopy((char*)&eh->ether_shost, (char*)&h->a2, ETHER_ADDR_LEN);
			bcopy((char*)&bsscfg->BSSID, (char*)&h->a3, ETHER_ADDR_LEN);
		}
	}

	/* SCB_QOS: Fill QoS Control Field */
	if (SCB_QOS(scb)) {
		uint16 qos, *pqos;
		wlc_wme_t *wme = bsscfg->wme;

		/* Set fragment priority */
		qos = (prio << QOS_PRIO_SHIFT) & QOS_PRIO_MASK;

		/* Set the ack policy; AMPDU overrides wme_noack */
		if (WLPKTFLAG_AMPDU(pkttag))
			qos |= (QOS_ACK_NORMAL_ACK << QOS_ACK_SHIFT) & QOS_ACK_MASK;
		else if (wme->wme_noack == QOS_ACK_NO_ACK) {
			WLPKTTAG(p)->flags |= WLF_WME_NOACK;
			qos |= (QOS_ACK_NO_ACK << QOS_ACK_SHIFT) & QOS_ACK_MASK;
		} else {
			qos |= (wme->wme_noack << QOS_ACK_SHIFT) & QOS_ACK_MASK;
		}

		/* Set the A-MSDU bit for AMSDU packet */
		if (WLPKTFLAG_AMSDU(pkttag))
			qos |= (1 << QOS_AMSDU_SHIFT) & QOS_AMSDU_MASK;

		pqos = (uint16 *)((uchar *)h + (a4 ? DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
		ASSERT(ISALIGNED(pqos, sizeof(*pqos)));

		*pqos = htol16(qos);

		/* Set subtype to QoS Data */
		fc |= (FC_SUBTYPE_QOS_DATA << FC_SUBTYPE_SHIFT);
	}

	fc |= (FC_TYPE_DATA << FC_TYPE_SHIFT);

	/* Set MoreFrag, WEP, and Order fc fields */
	if (MoreFrag)
		fc |= FC_MOREFRAG;
#ifdef WLWSEC
	if (key_info->algo != CRYPTO_ALGO_OFF && (!BYPASS_ENC_DATA(bsscfg)))
		fc |= FC_WEP;
#endif
	h->fc = htol16(fc);

	return h;
} /* wlc_80211hdr */

/** alloc fragment buf and fill with data from msdu input packet */
static void *
wlc_allocfrag(osl_t *osh, void *sdu, uint offset, uint headroom, uint frag_length, uint tailroom)
{
	void *p1;
	uint plen;
	uint totlen = pkttotlen(osh, sdu);

	/* In TKIP, (offset >= pkttotlen(sdu)) is possible due to MIC. */
	if (offset >= totlen) {
		plen = 0; /* all sdu has been consumed */
	} else {
		plen = MIN(frag_length, totlen - offset);
	}
	/* one-copy: alloc fragment buffers and fill with data from msdu input pkt */


	/* alloc a new pktbuf */
	if ((p1 = PKTGET(osh, headroom + tailroom + plen, TRUE)) == NULL)
		return (NULL);
	PKTPULL(osh, p1, headroom);

	/* copy frags worth of data at offset into pktbuf */
	pktcopy(osh, sdu, offset, plen, (uchar*)PKTDATA(osh, p1));
	PKTSETLEN(osh, p1, plen);

	/* Currently our PKTXXX macros only handle contiguous pkts. */
	ASSERT(!PKTNEXT(osh, p1));

	return (p1);
}

#if defined(BCMPCIEDEV) && defined(BCMFRAGPOOL)
/** called in the context of wlc_prep_sdu() */
static void *
wlc_allocfrag_txfrag(osl_t *osh, void *sdu, uint offset, uint frag_length,
	bool lastfrag)
{
	void *p1;
	uint plen;

	/* Len remaining to fill in the fragments */
	uint lenToFill = pkttotlen(osh, sdu) - offset;

	/* Currently TKIP is not handled */
	plen = MIN(frag_length, lenToFill);

	/* Cut out ETHER_HDR_LEN as frag_data does not account for that */
	offset -= ETHER_HDR_LEN;

	/* Need 202 bytes of headroom for TXOFF, 22 bytes for amsdu path */
	/* TXOFF + amsdu headroom */

	if ((p1 = pktpool_get(SHARED_FRAG_POOL)) == NULL)
		return (NULL);

	PKTPULL(osh, p1, FRAG_HEADROOM);
	PKTSETLEN(osh, p1, 0);

	/* If first fragment, copy LLC/SNAP header
	 * Host fragment length in this case, becomes plen - DOT11_LLC_SNAP hdr len
	 */
	if (WLPKTTAG(sdu)->flags & WLF_NON8023) {
		/* If first fragment, copy LLC/SNAP header
		* Host fragment length in this case, becomes plen - DOT11_LLC_SNAP hdr len
		*/
		if (offset == 0) {
			PKTPUSH(osh, p1, DOT11_LLC_SNAP_HDR_LEN);
			bcopy((uint8*) (PKTDATA(osh, sdu) + ETHER_HDR_LEN),
				PKTDATA(osh, p1), DOT11_LLC_SNAP_HDR_LEN);
			plen -= DOT11_LLC_SNAP_HDR_LEN;
		} else {
			/* For fragments other than the first fragment, deduct DOT11_LLC_SNAP len,
			* as that len should not be offset from Host data low address
			*/
			offset -= DOT11_LLC_SNAP_HDR_LEN;
		}
	}
	/* Set calculated address offsets for Host data */
	PKTSETFRAGDATA_HI(osh, p1, 1, PKTFRAGDATA_HI(osh, sdu, 1));
	PKTSETFRAGDATA_LO(osh, p1, 1, PKTFRAGDATA_LO(osh, sdu, 1) + offset);
	PKTSETFRAGTOTNUM(osh, p1, 1);
	PKTSETFRAGLEN(osh, p1, 1, plen);
	PKTSETFRAGTOTLEN(osh, p1, plen);
	PKTSETIFINDEX(osh, p1, PKTIFINDEX(osh, sdu));

	/* Only last fragment should have metadata
	 * and valid PKTID. Reset metadata and set invalid PKTID
	 * for other fragments
	 */
	if (lastfrag) {
		PKTSETFRAGPKTID(osh, p1, PKTFRAGPKTID(osh, sdu));
		PKTSETFRAGFLOWRINGID(osh, p1, PKTFRAGFLOWRINGID(osh, sdu));
		PKTFRAGSETRINGINDEX(osh, p1, PKTFRAGRINGINDEX(osh, sdu));
		PKTSETFRAGMETADATA_HI(osh, p1, PKTFRAGMETADATA_HI(osh, sdu));
		PKTSETFRAGMETADATA_LO(osh, p1, PKTFRAGMETADATA_LO(osh, sdu));
		PKTSETFRAGMETADATALEN(osh, p1,  PKTFRAGMETADATALEN(osh, sdu));
		PKTSETHASMETADATA(osh, p1);
	} else {
		PKTRESETHASMETADATA(osh, p1);
		PKTSETFRAGPKTID(osh, p1, 0xdeadbeaf);
	}

	return (p1);
} /* wlc_allocfrag_txfrag */
#endif	/* BCMPCIEDEV */

/** converts a single caller provided fragment (in 'p') to 802.11 format */
static void BCMFASTPATH
wlc_dofrag(wlc_info_t *wlc, void *p, uint frag, uint nfrags,
	uint next_payload_len, struct scb *scb, bool is8021x, uint fifo,
	wlc_key_t *key, const wlc_key_info_t *key_info,
	uint8 prio, uint frag_length)
{
	struct dot11_header *h = NULL;
	uint next_frag_len;
	uint16 frameid, txc_hdr_len = 0;
	uint16 txh_off = 0;
	wlc_bsscfg_t *cfg;
	uint16 d11hdr_len = 0;
	ASSERT(scb != NULL);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);
	BCM_REFERENCE(frag_length);

#ifdef BCMDBG
	{
	char eabuf[ETHER_ADDR_STR_LEN];
	WL_APSTA_TX(("wl%d.%d: wlc_dofrag: send %p to %s, cfg %p, fifo %d, frag %d, nfrags %d\n",
	             wlc->pub->unit, WLC_BSSCFG_IDX(cfg), OSL_OBFUSCATE_BUF(p),
	             bcm_ether_ntoa(&scb->ea, eabuf), OSL_OBFUSCATE_BUF(cfg), fifo, frag, nfrags));
	}
#endif /* BCMDBG */

	/*
	 * 802.3 (header length = 22):
	 *                     (LLC includes ether_type in last 2 bytes):
	 * ----------------------------------------------------------------------------------------
	 * |                                      |   DA   |   SA   | L | LLC/SNAP | T |  Data... |
	 * ----------------------------------------------------------------------------------------
	 *                                             6        6     2       6      2
	 *
	 * NON-WDS
	 *
	 * Conversion to 802.11 (header length = 32):
	 * ----------------------------------------------------------------------------------------
	 * |              | FC | D |   A1   |   A2   |   A3   | S | QoS | LLC/SNAP | T |  Data... |
	 * ----------------------------------------------------------------------------------------
	 *                   2   2      6        6        6     2    2        6      2
	 *
	 * Conversion to 802.11 (WEP, QoS):
	 * ----------------------------------------------------------------------------------------
	 * |         | FC | D |   A1   |   A2   |   A3   | S | QoS | IV | LLC/SNAP | T |  Data... |
	 * ----------------------------------------------------------------------------------------
	 *             2   2      6        6        6     2    2     4        6      2
	 *
	 * WDS
	 *
	 * Conversion to 802.11 (header length = 38):
	 * ----------------------------------------------------------------------------------------
	 * |     | FC | D |   A1   |   A2   |   A3   | S |   A4   | QoS | LLC/SNAP | T |  Data... |
	 * ----------------------------------------------------------------------------------------
	 *         2   2      6        6        6     2      6      2         6      2
	 *
	 * Conversion to 802.11 (WEP, QoS):
	 * ----------------------------------------------------------------------------------------
	 * | FC | D |   A1   |   A2   |   A3   | S |   A4   |  QoS | IV | LLC/SNAP | T |  Data... |
	 * ----------------------------------------------------------------------------------------
	 *    2   2      6        6        6     2      6       2    4        6      2
	 *
	 */

	WL_WSEC(("wl%d.%d: %s: tx sdu, len %d(with 802.3 hdr)\n",
	         wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__, pkttotlen(wlc->osh, p)));

	ASSERT(key != NULL && key_info != NULL);

	/*
	 * Convert 802.3 MAC header to 802.11 MAC header (data only)
	 * and add WEP IV information if enabled.
	 *  requires valid DA and SA, does not care ether_type
	 */
	if (!BSSCFG_SAFEMODE(cfg))
		h = wlc_80211hdr(wlc, p, scb, (bool)(frag != (nfrags - 1)),
			key_info, prio, &d11hdr_len, fifo);

	WL_WSEC(("wl%d.%d: %s: tx sdu, len %d(w 80211hdr and iv, no d11hdr, icv and fcs)\n",
	         wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__, pkttotlen(wlc->osh, p)));

	/* determine total MPDU length of next frag */
	next_frag_len = next_payload_len;
	if (next_frag_len > 0) {	/* there is a following frag */
		next_frag_len += DOT11_A3_HDR_LEN + DOT11_FCS_LEN;
		/* A4 header */
		if (SCB_A4_DATA(scb))
			next_frag_len += ETHER_ADDR_LEN;
		/* SCB_QOS: account for QoS Control Field */
		if (SCB_QOS(scb))
			next_frag_len += DOT11_QOS_LEN;
		if (!WLC_KEY_IS_LINUX_CRYPTO(key_info))
			next_frag_len += key_info->iv_len + key_info->icv_len;
	}

	/* add d11 headers */
	frameid = wlc_d11hdrs(wlc, p, scb,
		(WLC_PROT_CFG_SHORTPREAMBLE(wlc->prot, cfg) &&
	         (scb->flags & SCB_SHORTPREAMBLE) != 0),
		frag, nfrags, fifo, next_frag_len, key_info, 0, &txh_off);
	BCM_REFERENCE(frameid);

	if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(wlc->osh, p)) {
		txc_hdr_len = d11hdr_len + (txh_off + D11_TXH_LEN_SFD(wlc));
	} else {
		txc_hdr_len = d11hdr_len + (txh_off + D11_TXH_LEN_EX(wlc));
	}

#ifdef STA
	if (BSSCFG_STA(cfg) && next_frag_len == 0 && is8021x) {
		bool tkip_cm;
		tkip_cm = wlc_keymgmt_tkip_cm_enabled(wlc->keymgmt, cfg);
		if (tkip_cm) {
			WL_WSEC(("wl%d.%d: %s: TKIP countermeasures: sending MIC failure"
				" report...\n", WLCWLUNIT(wlc),
				WLC_BSSCFG_IDX(cfg), __FUNCTION__));
			if (frameid == 0xffff) {
				WL_ERROR(("wl%d.%d: %s: could not register MIC failure packet"
					" callback\n", WLCWLUNIT(wlc), WLC_BSSCFG_IDX(cfg),
					__FUNCTION__));
			} else {
				/* register packet callback */
				WLF2_PCB1_REG(p, WLF2_PCB1_TKIP_CM);
			}
		}
	}
#endif /* STA */

	/* 802.3 header needs to be included in WEP-encrypted portion of frame, and
	 * we need the IV info in the frame, so we're forced to do WEP here
	 */
	if (!BYPASS_ENC_DATA(scb->bsscfg)) {
	uint d11h_off;
	wlc_txh_info_t txh_info;

	if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(wlc->osh, p)) {
		d11h_off = txh_off + D11_TXH_LEN_SFD(wlc);
	} else {
		d11h_off = txh_off + D11_TXH_LEN_EX(wlc);
	}

	wlc_get_txh_info(wlc, p, &txh_info);
	PKTPULL(wlc->osh, p, d11h_off);
	if (wlc_key_prep_tx_mpdu(key, p, (wlc_txd_t *)txh_info.hdrPtr) != BCME_OK)
		WL_WSEC(("wl%d: %s: wlc_key_prep_tx_mpdu error\n",
		         wlc->pub->unit, __FUNCTION__));
	PKTPUSH(wlc->osh, p, d11h_off);
	}

	if (BSSCFG_AP(cfg) ||
	    BSS_TDLS_BUFFER_STA(cfg)) {
		/* Processing to set MoreData when needed */
		if (fifo != TX_BCMC_FIFO) {
			/* Check if packet is being sent to STA in PS mode */
			if (SCB_PS(scb)) {
				bool last_frag;

				last_frag = (frag == nfrags - 1);
				/* Make preparations for APSD delivery frame or PS-Poll response */
				if (AC_BITMAP_TST(scb->apsd.ac_delv, wme_fifo2ac[fifo]))
					wlc_apps_apsd_prepare(wlc, scb, p, h, last_frag);
				else
					wlc_apps_pspoll_resp_prepare(wlc, scb, p, h, last_frag);
				BCM_REFERENCE(last_frag);
			}
		} else {
			/* Broadcast/multicast. The uCode clears the MoreData field of the last bcmc
			 * pkt per dtim period. Suppress setting MoreData if we have to support
			 * legacy AES. This should probably also be conditional on at least one
			 * legacy STA associated.
			 */
			/*
			* Note: We currently use global WPA_auth for WDS and per scb WPA_auth
			* for others, but fortunately there is no bcmc frames over WDS link
			* therefore we don't need to worry about WDS and it is safe to use per
			* scb WPA_auth only here!
			*/
			/* Also look at wlc_apps_ps_prep_mpdu if following condition ever changes */
			wlc_key_info_t bss_key_info;
			(void)wlc_keymgmt_get_bss_tx_key(wlc->keymgmt, cfg, FALSE, &bss_key_info);
			if (!bcmwpa_is_wpa_auth(cfg->WPA_auth) ||
				(bss_key_info.algo != CRYPTO_ALGO_AES_CCM)) {
				h->fc |= htol16(FC_MOREDATA);
			}
		}
	}

	/* dont cache 8021x frames in case of DWDS */
	if (is8021x && SCB_A4_DATA(scb) && !SCB_A4_8021X(scb))
		return;

	/* install new header into tx header cache: starting from the bytes after DA-SA-L  */
	if (WLC_TXC_ENAB(wlc)) {
		wlc_txc_info_t *txc = wlc->txc;

		if (TXC_CACHE_ENAB(txc) && (nfrags == 1) &&
		    !(WLPKTTAG(p)->flags & WLF_BYPASS_TXC) &&
		    !BSSCFG_SAFEMODE(cfg)) {
			wlc_txc_add(txc, scb, p, txc_hdr_len, fifo, prio, txh_off, d11hdr_len);
		}
	}
} /* wlc_dofrag */


/**
 * allocate headroom buffer if necessary and convert ether frame to 8023 frame
 * !! WARNING: HND WL driver only supports data frame of type ethernet, 802.1x or 802.3
 */
void * BCMFASTPATH
wlc_hdr_proc(wlc_info_t *wlc, void *sdu, struct scb *scb)
{
	osl_t *osh;
	struct ether_header *eh;
	void *pkt, *phdr;
	int prio, use_phdr;
	uint16 ether_type;

	osh = wlc->osh;

	/* allocate enough room once for all cases */
	prio = PKTPRIO(sdu);
	phdr = NULL;
	use_phdr = FALSE;
#ifdef DMATXRC
	use_phdr = DMATXRC_ENAB(wlc->pub) ? TRUE : FALSE;
	if (use_phdr)
		phdr = wlc_phdr_get(wlc);
#endif /* DMATXRC */

	/* SFD based lfrag has assured headroom */
	if (SFD_ENAB(wlc->pub) && PKTISTXFRAG(wlc->osh, sdu))
		goto skip_realloc;

	if ((uint)PKTHEADROOM(osh, sdu) < TXOFF || PKTSHARED(sdu) || (use_phdr && phdr)) {
		if (use_phdr && phdr)
			pkt = phdr;
		else
			pkt = PKTGET(osh, TXOFF, TRUE);

		if (pkt == NULL) {
			WL_ERROR(("wl%d: %s, PKTGET headroom %d failed\n",
				wlc->pub->unit, __FUNCTION__, (int)TXOFF));
			WLCNTINCR(wlc->pub->_cnt->txnobuf);
			/* increment interface stats */
			WLCIFCNTINCR(scb, txnobuf);
			WLCNTSCBINCR(scb->scb_stats.tx_failures);
			return NULL;
		}

		if (phdr == NULL)
			PKTPULL(osh, pkt, TXOFF);

		wlc_pkttag_info_move(wlc, sdu, pkt);

#ifdef DMATXRC
		if (DMATXRC_ENAB(wlc->pub) && use_phdr && phdr) {
			/* Over-written by wlc_pkttag_info_move */
			WLPKTTAG(pkt)->flags |= WLF_PHDR;
		}
#endif /* DMATXRC */
		/* Transfer priority */
		PKTSETPRIO(pkt, prio);

		/* move ether_hdr from data buffer to header buffer */
		eh = (struct ether_header*) PKTDATA(osh, sdu);
		PKTPULL(osh, sdu, ETHER_HDR_LEN);
		PKTPUSH(osh, pkt, ETHER_HDR_LEN);
		bcopy((char*)eh, (char*)PKTDATA(osh, pkt), ETHER_HDR_LEN);

		/* chain original sdu onto newly allocated header */
		PKTSETNEXT(osh, pkt, sdu);
#if defined(DMATXRC) && defined(PKTC_TX_DONGLE)
		if (DMATXRC_ENAB(wlc->pub) && PKTC_ENAB(wlc->pub) && PKTLINK(sdu)) {
			PKTSETLINK(pkt, PKTLINK(sdu));
			PKTSETLINK(sdu, NULL);
			PKTSETCHAINED(wlc->osh, pkt);
		}
#endif
		sdu = pkt;
	}

skip_realloc:

#ifdef WLMESH
	if (WLMESH_ENAB(wlc->pub) && (WLPKTTAG(sdu)->flags & WLF_MESH_RETX)) {
		return sdu;
	}
#endif

	{
	/*
	 * Optionally add an 802.1Q VLAN tag to convey non-zero priority for
	 * non-WMM associations.
	 */
	eh = (struct ether_header *)PKTDATA(osh, sdu);

	if (prio && !SCB_QOS(scb)) {
		if ((wlc->vlan_mode != OFF) && (ntoh16(eh->ether_type) != ETHER_TYPE_8021Q)) {
			struct ethervlan_header *vh;
			struct ether_header da_sa;

			bcopy(eh, &da_sa, VLAN_TAG_OFFSET);
			vh = (struct ethervlan_header *)PKTPUSH(osh, sdu, VLAN_TAG_LEN);
			bcopy(&da_sa, vh, VLAN_TAG_OFFSET);

			vh->vlan_type = hton16(ETHER_TYPE_8021Q);
			vh->vlan_tag = hton16(prio << VLAN_PRI_SHIFT);	/* Priority-only Tag */
		}

		if (WME_ENAB(wlc->pub) && !wlc->wme_prec_queuing) {
			prio = 0;
			PKTSETPRIO(sdu, prio);
		}
	}
	}

	/*
	 * Original Ethernet (header length = 14):
	 * ----------------------------------------------------------------------------------------
	 * |                                                     |   DA   |   SA   | T |  Data... |
	 * ----------------------------------------------------------------------------------------
	 *                                                            6        6     2
	 *
	 * Conversion to 802.3 (header length = 22):
	 *                     (LLC includes ether_type in last 2 bytes):
	 * ----------------------------------------------------------------------------------------
	 * |                                      |   DA   |   SA   | L | LLC/SNAP | T |  Data... |
	 * ----------------------------------------------------------------------------------------
	 *                                             6        6     2       6      2
	 */

	eh = (struct ether_header *)PKTDATA(osh, sdu);
	ether_type = ntoh16(eh->ether_type);
	if (ether_type > ETHER_MAX_DATA) {
		if (ether_type == ETHER_TYPE_802_1X) {
			WLPKTTAG(sdu)->flags |= WLF_8021X;
			WLPKTTAG(sdu)->flags |= WLF_BYPASS_TXC;
			WLPKTTAG(sdu)->flags3 |= WLF3_BYPASS_AMPDU;
		}

		/* save original type in pkt tag */
		WLPKTTAG(sdu)->flags |= WLF_NON8023;
		wlc_ether_8023hdr(wlc, osh, eh, sdu);
	}

	return sdu;
} /* wlc_hdr_proc */

static INLINE uint
wlc_frag(wlc_info_t *wlc, struct scb *scb, uint8 ac, uint plen, uint *flen)
{
	uint payload, thresh, nfrags, bt_thresh = 0;
	int btc_mode;
#ifdef WL_FRAGDUR
	uint frag_thresh;
#endif

	plen -= ETHER_HDR_LEN;

	ASSERT(ac < AC_COUNT);

	thresh = wlc->fragthresh[ac];

	btc_mode = wlc_btc_mode_get(wlc);

	if (IS_BTCX_FULLTDM(btc_mode))
		bt_thresh = wlc_btc_frag_threshold(wlc, scb);

	if (bt_thresh)
		thresh = thresh > bt_thresh ? bt_thresh : thresh;

#ifdef WL_FRAGDUR
	if (wlc_fragdur_length(wlc->fragdur)) {
		frag_thresh = wlc_fragdur_threshold(wlc->fragdur, scb, wme_fifo2ac[fifo]);
		thresh = thresh > frag_thresh ? frag_thresh : thresh;
	}
#endif

	/* optimize for non-fragmented case */
	if (plen < (thresh - (DOT11_A4_HDR_LEN + DOT11_QOS_LEN +
		DOT11_FCS_LEN + ETHER_ADDR_LEN))) {
		*flen = plen;
		return (1);
	}

	/* account for 802.11 MAC header */
	thresh -= DOT11_A3_HDR_LEN + DOT11_FCS_LEN;

	/* account for A4 */
	if (SCB_A4_DATA(scb))
		thresh -= ETHER_ADDR_LEN;

	/* SCB_QOS: account for QoS Control Field */
	if (SCB_QOS(scb))
		thresh -= DOT11_QOS_LEN;

	/*
	 * Spec says broadcast and multicast frames are not fragmented.
	 * LLC/SNAP considered part of packet payload.
	 * Fragment length must be even per 9.4 .
	 */
	if ((plen > thresh) && !SCB_ISMULTI(scb)) {
		*flen = payload = thresh & ~1;
		nfrags = (plen + payload - 1) / payload;
	} else {
		*flen = plen;
		nfrags = 1;
	}

	ASSERT(nfrags <= DOT11_MAXNUMFRAGS);

	return (nfrags);
} /* wlc_frag */

/**
 * Add d11txh_t, cck_phy_hdr_t for pre-11ac mac & phy.
 *
 * 'p' data must start with 802.11 MAC header
 * 'p' must allow enough bytes of local headers to be "pushed" onto the packet
 *
 * headroom == D11_PHY_HDR_LEN + D11_TXH_LEN (D11_TXH_LEN is now 104 bytes)
 *
 */
uint16
wlc_d11hdrs_pre40(wlc_info_t *wlc, void *p, struct scb *scb, uint txparams_flags, uint frag,
	uint nfrags, uint queue, uint next_frag_len, const wlc_key_info_t *key_info,
	ratespec_t rspec_override, uint16 *txh_off)
{
	struct dot11_header *h;
	d11txh_t *txh;
	uint8 *plcp, plcp_fallback[D11_PHY_HDR_LEN];
	osl_t *osh;
	int len, phylen, rts_phylen;
	uint16 fc, type, frameid, mch, phyctl, xfts, mainrates, rate_flag;
	uint16 seq = 0, mcl = 0, status = 0;
	bool use_rts = FALSE;
	bool use_cts = FALSE, rspec_history = FALSE;
	bool use_rifs = FALSE;
	bool short_preamble;
	uint8 preamble_type = WLC_LONG_PREAMBLE, fbr_preamble_type = WLC_LONG_PREAMBLE;
	uint8 rts_preamble_type = WLC_LONG_PREAMBLE, rts_fbr_preamble_type = WLC_LONG_PREAMBLE;
	uint8 *rts_plcp, rts_plcp_fallback[D11_PHY_HDR_LEN];
	struct dot11_rts_frame *rts = NULL;
	ratespec_t rts_rspec = 0, rts_rspec_fallback = 0;
	bool qos, a4;
	uint8 ac;
	uint rate;
	wlc_pkttag_t *pkttag;
	ratespec_t rspec, rspec_fallback;
	ratesel_txparams_t cur_rate;
#if defined(BCMDBG)
	uint16 phyctl_bwo = -1;
#endif 
#if WL_HT_TXBW_OVERRIDE_ENAB
	int8 txbw_override_idx;
#endif /* WL_HT_TXBW_OVERRIDE_ENAB */
#ifdef WL11N
#define ANTCFG_NONE 0xFF
	uint8 antcfg = ANTCFG_NONE;
	uint8 fbantcfg = ANTCFG_NONE;
	uint16 mimoantsel;
	bool use_mimops_rts = FALSE;
	uint phyctl1_stf = 0;
#endif /* WL11N */
	uint16 durid = 0;
	wlc_bsscfg_t *bsscfg;
	bool g_prot;
	int8 n_prot;
	wlc_wme_t *wme;
#ifdef WL_LPC
	uint8 lpc_offset = 0;
#endif
#ifdef WL11N
	uint8 sgi_tx;
	wlc_ht_info_t *hti = wlc->hti;
#endif /* WL11N */
	ASSERT(scb != NULL);
	ASSERT(queue < NFIFO);

	if (BYPASS_ENC_DATA(scb->bsscfg)) {
		key_info = NULL;
	}

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	short_preamble = (txparams_flags & WLC_TX_PARAMS_SHORTPRE) != 0;

	g_prot = WLC_PROT_G_CFG_G(wlc->prot_g, bsscfg);
	n_prot = WLC_PROT_N_CFG_N(wlc->prot_n, bsscfg);

	wme = bsscfg->wme;

	osh = wlc->osh;

	/* locate 802.11 MAC header */
	h = (struct dot11_header*) PKTDATA(osh, p);
	pkttag = WLPKTTAG(p);
	fc = ltoh16(h->fc);
	type = FC_TYPE(fc);
	qos = (type == FC_TYPE_DATA && FC_SUBTYPE_ANY_QOS(FC_SUBTYPE(fc)));
	a4 = (fc & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS);

	/* compute length of frame in bytes for use in PLCP computations
	 * phylen =  packet length + ICV_LEN + FCS_LEN
	 */
	len = pkttotlen(osh, p);
	phylen = len + DOT11_FCS_LEN;

	/* Add room in phylen for the additional bytes for icv, if required.
	 * this is true for both h/w and s/w encryption. The latter only
	 * modifies the pkt length
	 */
	if (key_info != NULL) {
		if (WLC_KEY_IS_MGMT_GROUP(key_info))
			phylen += WLC_KEY_MMIC_IE_LEN(key_info);
		else
			phylen += key_info->icv_len;

		/* external crypto adds iv to the pkt, include it in phylen */
		if (WLC_KEY_IS_LINUX_CRYPTO(key_info))
			phylen += key_info->iv_len;

		if (WLC_KEY_FRAG_HAS_TKIP_MIC(p, key_info, frag, nfrags))
			phylen += TKIP_MIC_SIZE;
	}

	WL_NONE(("wl%d: %s: len %d, phylen %d\n", WLCWLUNIT(wlc), __FUNCTION__, len, phylen));

	/* add PLCP */
	plcp = PKTPUSH(osh, p, D11_PHY_HDR_LEN);

	/* add Broadcom tx descriptor header */
	txh = (d11txh_t*)PKTPUSH(osh, p, D11_TXH_LEN);
	bzero((char*)txh, D11_TXH_LEN);

	/* use preassigned or software seqnum */
#ifdef PROP_TXSTATUS
	if (IS_WL_TO_REUSE_SEQ(pkttag->seq)) {
		seq = WL_SEQ_GET_NUM(pkttag->seq);
	} else if (WLPKTFLAG_AMPDU(pkttag)) {
		seq = WL_SEQ_GET_NUM(pkttag->seq);
	}
#else
	if (WLPKTFLAG_AMPDU(pkttag))
		seq = pkttag->seq;
#endif /* PROP_TXSTATUS */
	else if (SCB_QOS(scb) && ((fc & FC_KIND_MASK) == FC_QOS_DATA) && !ETHER_ISMULTI(&h->a1)) {
		seq = SCB_SEQNUM(scb, PKTPRIO(p));
		/* Increment the sequence number only after the last fragment */
		if (frag == (nfrags - 1))
			SCB_SEQNUM(scb, PKTPRIO(p))++;
	}
	/* use h/w seqnum */
	else if (type != FC_TYPE_CTL)
		mcl |= TXC_HWSEQ;

	if (type != FC_TYPE_CTL) {
		seq = (seq << SEQNUM_SHIFT) | (frag & FRAGNUM_MASK);
		h->seq = htol16(seq);
	}

#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub) &&
		WLFC_GET_REUSESEQ(wlfc_query_mode(wlc->wlfc))) {
		WL_SEQ_SET_NUM(pkttag->seq, ltoh16(h->seq) >> SEQNUM_SHIFT);
		SET_WL_HAS_ASSIGNED_SEQ(pkttag->seq);
	}
#endif /* PROP_TXSTATUS */
	/* setup frameid */
	if (queue == TX_BCMC_FIFO) {
		frameid = bcmc_fid_generate(wlc, bsscfg, txh->TxFrameID); /* broadcast/multicast */
	} else {
		frameid = (((wlc->counter++) << TXFID_SEQ_SHIFT) & TXFID_SEQ_MASK) |
			WLC_TXFID_SET_QUEUE(queue);
	}

	/* set the ignpmq bit for all pkts tx'd in PS mode and for beacons and for anything
	 * going out from a STA interface.
	 */
	if (SCB_PS(scb) || ((fc & FC_KIND_MASK) == FC_BEACON) || BSSCFG_STA(bsscfg))
		mcl |= TXC_IGNOREPMQ;

	ac = WME_PRIO2AC(PKTPRIO(p));
	/* (1) RATE: determine and validate primary rate and fallback rates */

	if (RSPEC_ACTIVE(rspec_override)) {
		rspec = rspec_fallback = rspec_override;
		rspec_history = TRUE;
#ifdef WL11N
		if (WLANTSEL_ENAB(wlc) && !ETHER_ISMULTI(&h->a1)) {
			/* set tx antenna config */
			wlc_antsel_antcfg_get(wlc->asi, FALSE, FALSE, 0, 0,
				&antcfg, &fbantcfg);
		}
#endif /* WL11N */
	}
	else if ((type == FC_TYPE_MNG) ||
		(type == FC_TYPE_CTL) ||
		(pkttag->flags & WLF_8021X) ||
		(pkttag->flags3 & WLF3_DATA_WOWL_PKT))
		rspec = rspec_fallback = scb->rateset.rates[0] & RATE_MASK;
	else if (RSPEC_ACTIVE(wlc->band->mrspec_override) && ETHER_ISMULTI(&h->a1))
		rspec = rspec_fallback = wlc->band->mrspec_override;
	else if (RSPEC_ACTIVE(wlc->band->rspec_override) && !ETHER_ISMULTI(&h->a1)) {
		rspec = rspec_fallback = wlc->band->rspec_override;
		rspec_history = TRUE;
#ifdef WL11N
		if (WLANTSEL_ENAB(wlc)) {
			/* set tx antenna config */
			wlc_antsel_antcfg_get(wlc->asi, FALSE, FALSE, 0, 0,
			&antcfg, &fbantcfg);
		}
#endif /* WL11N */
	} else if (ETHER_ISMULTI(&h->a1) || SCB_INTERNAL(scb))
		rspec = rspec_fallback = scb->rateset.rates[0] & RATE_MASK;
	else {
		/* run rate algorithm for data frame only, a cookie will be deposited in frameid */
		cur_rate.num = 2; /* only need up to 2 for non-11ac */
		cur_rate.ac = ac;
		wlc_scb_ratesel_gettxrate(wlc->wrsi, scb, &frameid, &cur_rate, &rate_flag);
		rspec = cur_rate.rspec[0];
		rspec_fallback = cur_rate.rspec[1];

		if (((scb->flags & SCB_BRCM) == 0) &&
		    (((fc & FC_KIND_MASK) == FC_NULL_DATA) ||
		     ((fc & FC_KIND_MASK) == FC_QOS_NULL))) {
			/* Use RTS/CTS rate for NULL frame */
			rspec = wlc_rspec_to_rts_rspec(bsscfg, rspec, FALSE);
			rspec_fallback = scb->rateset.rates[0] & RATE_MASK;
		} else {
			pkttag->flags |= WLF_RATE_AUTO;

			/* The rate histo is updated only for packets on auto rate. */
			/* perform rate history after txbw has been selected */
			if (frag == 0)
				rspec_history = TRUE;
		}
#ifdef WL11N
		if (rate_flag & RATESEL_VRATE_PROBE)
			WLPKTTAG(p)->flags |= WLF_VRATE_PROBE;

		if (WLANTSEL_ENAB(wlc)) {
			wlc_antsel_antcfg_get(wlc->asi, FALSE, TRUE, cur_rate.antselid[0],
				cur_rate.antselid[1], &antcfg, &fbantcfg);
		}
#endif /* WL11N */

#ifdef WL_LPC
		if (LPC_ENAB(wlc)) {
			/* Query the power offset to be used from LPC */
			lpc_offset = wlc_scb_lpc_getcurrpwr(wlc->wlpci, scb, ac);
		} else {
			/* No Link Power Control. Transmit at nominal power. */
		}
#endif
	}

#ifdef WL11N
	phyctl1_stf = wlc->stf->ss_opmode;

	if (N_ENAB(wlc->pub)) {
		uint32 mimo_txbw;
		uint8 mimo_preamble_type;

		bool stbc_tx_forced = WLC_IS_STBC_TX_FORCED(wlc);
		bool stbc_ht_scb_auto = WLC_STF_SS_STBC_HT_AUTO(wlc, scb);

		/* apply siso/cdd to single stream mcs's or ofdm if rspec is auto selected */
		if (((IS_MCS(rspec) && IS_SINGLE_STREAM(rspec & RSPEC_RATE_MASK)) ||
		     RSPEC_ISOFDM(rspec)) &&
		    !(rspec & RSPEC_OVERRIDE_MODE)) {
			rspec &= ~(RSPEC_TXEXP_MASK | RSPEC_STBC);

			/* For SISO MCS use STBC if possible */
			if (IS_MCS(rspec) && (stbc_tx_forced ||
				(RSPEC_ISHT(rspec) && stbc_ht_scb_auto))) {
				ASSERT(WLC_STBC_CAP_PHY(wlc));
				rspec |= RSPEC_STBC;
			} else if (phyctl1_stf == PHY_TXC1_MODE_CDD) {
				rspec |= (1 << RSPEC_TXEXP_SHIFT);
			}
		}

		if (((IS_MCS(rspec_fallback) &&
		      IS_SINGLE_STREAM(rspec_fallback & RSPEC_RATE_MASK)) ||
		     RSPEC_ISOFDM(rspec_fallback)) &&
		    !(rspec_fallback & RSPEC_OVERRIDE_MODE)) {
			rspec_fallback &= ~(RSPEC_TXEXP_MASK | RSPEC_STBC);

			/* For SISO MCS use STBC if possible */
			if (IS_MCS(rspec_fallback) && (stbc_tx_forced ||
				(RSPEC_ISHT(rspec_fallback) && stbc_ht_scb_auto))) {
				ASSERT(WLC_STBC_CAP_PHY(wlc));
				rspec_fallback |= RSPEC_STBC;
			} else if (phyctl1_stf == PHY_TXC1_MODE_CDD) {
				rspec_fallback |= (1 << RSPEC_TXEXP_SHIFT);
			}
		}

		/* Is the phy configured to use 40MHZ frames? If so then pick the desired txbw */
		if (CHSPEC_IS40(wlc->chanspec)) {
			/* default txbw is 20in40 SB */
			mimo_txbw = RSPEC_BW_20MHZ;

			if (RSPEC_BW(rspec) != RSPEC_BW_UNSPECIFIED) {
				/* If the ratespec override has a bw greater than
				 * 20MHz, specify th txbw value here.
				 * Otherwise, the default setup above is already 20MHz.
				 */
				if (!RSPEC_IS20MHZ(rspec)) {
					/* rspec bw > 20MHz is not allowed for CCK/DSSS
					 * so we can only have OFDM or HT here
					 * mcs 32 and legacy OFDM must be 40b/w DUP, other HT
					 * is just plain 40MHz
					 */
					mimo_txbw = RSPEC_BW(rspec);
				}
			} else if (IS_MCS(rspec)) {
				if ((scb->flags & SCB_IS40) &&
#ifdef WLMCHAN
					 /* if mchan enabled and bsscfg is AP, then must
					  * check the bsscfg chanspec to make sure our AP
					  * is operating on 40MHz channel.
					  */
					 (!MCHAN_ENAB(wlc->pub) || !BSSCFG_AP(bsscfg) ||
					  CHSPEC_IS40(bsscfg->current_bss->chanspec)) &&
#endif /* WLMCHAN */
					 TRUE) {
					mimo_txbw = RSPEC_BW_40MHZ;
				}
			}
#if WL_HT_TXBW_OVERRIDE_ENAB
			WL_HT_TXBW_OVERRIDE_IDX(hti, rspec, txbw_override_idx);

			if (txbw_override_idx >= 0) {
				mimo_txbw = txbw2rspecbw[txbw_override_idx];
				phyctl_bwo = txbw2phyctl0bw[txbw_override_idx];
			}
#endif /* WL_HT_TXBW_OVERRIDE_ENAB */
			/* mcs 32 must be 40b/w DUP */
			if (IS_MCS(rspec) && ((rspec & RSPEC_RATE_MASK) == 32))
				mimo_txbw = RSPEC_BW_40MHZ;
		} else  {
			/* mcs32 is 40 b/w only.
			 * This is possible for probe packets on a STA during SCAN
			 */
			if ((rspec & RSPEC_RATE_MASK) == 32) {
				WL_INFORM(("wl%d: wlc_d11hdrs mcs 32 invalid in 20MHz mode, using"
					"mcs 0 instead\n", wlc->pub->unit));

				rspec = HT_RSPEC(0);	/* mcs 0 */
			}

			/* Fix fallback as well */
			if ((rspec_fallback & RSPEC_RATE_MASK) == 32)
				rspec_fallback = HT_RSPEC(0);	/* mcs 0 */

			mimo_txbw = RSPEC_BW_20MHZ;
		}

		rspec &= ~RSPEC_BW_MASK;
		rspec |= mimo_txbw;

		rspec_fallback &= ~RSPEC_BW_MASK;
		if (IS_MCS(rspec_fallback))
			rspec_fallback |= mimo_txbw;
		else
			rspec_fallback |= RSPEC_BW_20MHZ;
		sgi_tx = WLC_HT_GET_SGI_TX(hti);
		if (!RSPEC_ACTIVE(wlc->band->rspec_override)) {
			if (IS_MCS(rspec) && (sgi_tx == ON))
				rspec |= RSPEC_SHORT_GI;
			else if (sgi_tx == OFF)
				rspec &= ~RSPEC_SHORT_GI;

			if (IS_MCS(rspec_fallback) && (sgi_tx == ON))
				rspec_fallback |= RSPEC_SHORT_GI;
			else if (sgi_tx == OFF)
				rspec_fallback &= ~RSPEC_SHORT_GI;
		}

		if (!RSPEC_ACTIVE(wlc->band->rspec_override)) {
			ASSERT(!(rspec & RSPEC_LDPC_CODING));
			ASSERT(!(rspec_fallback & RSPEC_LDPC_CODING));
			rspec &= ~RSPEC_LDPC_CODING;
			rspec_fallback &= ~RSPEC_LDPC_CODING;
			if (wlc->stf->ldpc_tx == ON ||
			    (SCB_LDPC_CAP(scb) && wlc->stf->ldpc_tx == AUTO)) {
				if (IS_MCS(rspec) &&
				    (CHIPID(wlc->pub->sih->chip) != BCM43430_CHIP_ID ||
				    !IS_PROPRIETARY_11N_MCS(rspec & RSPEC_RATE_MASK)))
					rspec |= RSPEC_LDPC_CODING;
				if (IS_MCS(rspec_fallback) &&
				    (CHIPID(wlc->pub->sih->chip) != BCM43430_CHIP_ID ||
				    !IS_PROPRIETARY_11N_MCS(rspec_fallback & RSPEC_RATE_MASK)))
					rspec_fallback |= RSPEC_LDPC_CODING;
			}
		}

		mimo_preamble_type = wlc_prot_n_preamble(wlc, scb);
		if (IS_MCS(rspec)) {
			preamble_type = mimo_preamble_type;
			if (n_prot == WLC_N_PROTECTION_20IN40 &&
			    RSPEC_IS40MHZ(rspec))
				use_cts = TRUE;

			/* if the mcs is multi stream check if it needs an rts */
			if (!IS_SINGLE_STREAM(rspec & RSPEC_RATE_MASK)) {
				if (WLC_HT_SCB_RTS_ENAB(hti, scb)) {
					use_rts = use_mimops_rts = TRUE;
				}
			}

			/* if SGI is selected, then forced mm for single stream */
			if ((rspec & RSPEC_SHORT_GI) && IS_SINGLE_STREAM(rspec & RSPEC_RATE_MASK)) {
				ASSERT(IS_MCS(rspec));
				preamble_type = WLC_MM_PREAMBLE;
			}
		}

		if (IS_MCS(rspec_fallback)) {
			fbr_preamble_type = mimo_preamble_type;

			/* if SGI is selected, then forced mm for single stream */
			if ((rspec_fallback & RSPEC_SHORT_GI) &&
			    IS_SINGLE_STREAM(rspec_fallback & RSPEC_RATE_MASK))
				fbr_preamble_type = WLC_MM_PREAMBLE;
		}
	} else
#endif /* WL11N */
	{
		/* Set ctrlchbw as 20Mhz */
		ASSERT(!IS_MCS(rspec));
		ASSERT(!IS_MCS(rspec_fallback));
		rspec &= ~RSPEC_BW_MASK;
		rspec_fallback &= ~RSPEC_BW_MASK;
		rspec |= RSPEC_BW_20MHZ;
		rspec_fallback |= RSPEC_BW_20MHZ;

#ifdef WL11N
#if NCONF
		/* for nphy, stf of ofdm frames must follow policies */
		if ((WLCISNPHY(wlc->band) || WLCISHTPHY(wlc->band)) &&
		    RSPEC_ISOFDM(rspec)) {
			rspec &= ~RSPEC_TXEXP_MASK;
			if (phyctl1_stf == PHY_TXC1_MODE_CDD) {
				rspec |= (1 << RSPEC_TXEXP_SHIFT);
			}
		}
		if ((WLCISNPHY(wlc->band) || WLCISHTPHY(wlc->band)) &&
		    RSPEC_ISOFDM(rspec_fallback)) {
			rspec_fallback &= ~RSPEC_TXEXP_MASK;
			if (phyctl1_stf == PHY_TXC1_MODE_CDD) {
				rspec_fallback |= (1 << RSPEC_TXEXP_SHIFT);
			}
		}
#endif /* NCONF */
#endif /* WL11N */
	}

	/* record rate history after the txbw is valid */
	if (rspec_history) {
#ifdef WL11N
		/* store current tx ant config ratesel (ignore probes) */
		if (WLANTSEL_ENAB(wlc) && N_ENAB(wlc->pub) &&
			((frameid & TXFID_RATE_PROBE_MASK) == 0)) {
			wlc_antsel_set_unicast(wlc->asi, antcfg);
		}
#endif

		/* update per bsscfg tx rate */
		bsscfg->txrspec[bsscfg->txrspecidx][0] = rspec;
		bsscfg->txrspec[bsscfg->txrspecidx][1] = (uint8) nfrags;
		bsscfg->txrspecidx = (bsscfg->txrspecidx+1) % NTXRATE;

		WLCNTSCBSET(scb->scb_stats.tx_rate_fallback, rspec_fallback);
	}

	/* mimo bw field MUST now be valid in the rspec (it affects duration calculations) */
	rate = RSPEC2RATE(rspec);

	/* (2) PROTECTION, may change rspec */
	if (((type == FC_TYPE_DATA) || (type == FC_TYPE_MNG)) &&
	    ((phylen > wlc->RTSThresh) || (pkttag->flags & WLF_USERTS)) &&
	    !ETHER_ISMULTI(&h->a1))
		use_rts = TRUE;

	if ((wlc->band->gmode && g_prot && RSPEC_ISOFDM(rspec)) ||
	    (N_ENAB(wlc->pub) && IS_MCS(rspec) && n_prot)) {
		if (nfrags > 1) {
			/* For a frag burst use the lower modulation rates for the entire frag burst
			 * instead of protection mechanisms.
			 * As per spec, if protection mechanism is being used, a fragment sequence
			 * may only employ ERP-OFDM modulation for the final fragment and control
			 * response. (802.11g Sec 9.10) For ease we send the *whole* sequence at the
			 * lower modulation instead of using a higher modulation for the last frag.
			 */

			/* downgrade the rate to CCK or OFDM */
			if (g_prot) {
				/* Use 11 Mbps as the rate and fallback. We should make sure that if
				 * we are downgrading an OFDM rate to CCK, we should pick a more
				 * robust rate.  6 and 9 Mbps are not usually selected by rate
				 * selection, but even if the OFDM rate we are downgrading is 6 or 9
				 * Mbps, 11 Mbps is more robust.
				 */
				rspec = rspec_fallback = CCK_RSPEC(WLC_RATE_FRAG_G_PROTECT);
			} else {
				/* Use 24 Mbps as the rate and fallback for what would have been
				 * a MIMO rate. 24 Mbps is the highest phy mandatory rate for OFDM.
				 */
				rspec = rspec_fallback = OFDM_RSPEC(WLC_RATE_FRAG_N_PROTECT);
			}
			pkttag->flags &= ~WLF_RATE_AUTO;
		} else {
			/* Use protection mechanisms on unfragmented frames */
			/* If this is a 11g protection, then use CTS-to-self */
			if (wlc->band->gmode && g_prot && !RSPEC_ISCCK(rspec))
				use_cts = TRUE;
		}
	}

	/* calculate minimum rate */
	ASSERT(RSPEC2KBPS(rspec_fallback) <= RSPEC2KBPS(rspec));
	ASSERT(VALID_RATE_DBG(wlc, rspec));
	ASSERT(VALID_RATE_DBG(wlc, rspec_fallback));

	if (RSPEC_ISCCK(rspec)) {
		if (short_preamble &&
		    !((RSPEC2RATE(rspec) == WLC_RATE_1M) ||
		      (scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG)))
			preamble_type = WLC_SHORT_PREAMBLE;
		else
			preamble_type = WLC_LONG_PREAMBLE;
	}

	if (RSPEC_ISCCK(rspec_fallback)) {
		if (short_preamble &&
		    !((RSPEC2RATE(rspec_fallback) == WLC_RATE_1M) ||
		      (scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG)))
			fbr_preamble_type = WLC_SHORT_PREAMBLE;
		else
			fbr_preamble_type = WLC_LONG_PREAMBLE;
	}

	ASSERT(!IS_MCS(rspec) || WLC_IS_MIMO_PREAMBLE(preamble_type));
	ASSERT(!IS_MCS(rspec_fallback) || WLC_IS_MIMO_PREAMBLE(fbr_preamble_type));

	WLCNTSCB_COND_SET(((type == FC_TYPE_DATA) &&
		((FC_SUBTYPE(fc) != FC_SUBTYPE_NULL) &&
			(FC_SUBTYPE(fc) != FC_SUBTYPE_QOS_NULL))),
			scb->scb_stats.tx_rate, rspec);
#ifdef WLFCTS
	if (WLFCTS_ENAB(wlc->pub))
		WLPKTTAG(p)->rspec = rspec;
#endif

	/* RIFS(testing only): based on frameburst, non-CCK frames only */
	if (SCB_HT_CAP(scb) &&
	    WLC_HT_GET_FRAMEBURST(hti) &&
	    WLC_HT_GET_RIFS(hti) &&
	    n_prot != WLC_N_PROTECTION_MIXEDMODE &&
	    !RSPEC_ISCCK(rspec) &&
	    !ETHER_ISMULTI(&h->a1) &&
	    ((fc & FC_KIND_MASK) == FC_QOS_DATA) &&
	    (queue < TX_BCMC_FIFO)) {
		uint16 qos_field, *pqos;

		WLPKTTAG(p)->flags |= WLF_RIFS;
		mcl |= (TXC_FRAMEBURST | TXC_USERIFS);
		use_rifs = TRUE;

		/* RIFS implies QoS frame with no-ack policy, hack the QoS field */
		pqos = (uint16 *)((uchar *)h + (a4 ?
			DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
		qos_field = ltoh16(*pqos) & ~QOS_ACK_MASK;
		qos_field |= (QOS_ACK_NO_ACK << QOS_ACK_SHIFT) & QOS_ACK_MASK;
		*pqos = htol16(qos_field);
	}

	/* (3) PLCP: determine PLCP header and MAC duration, fill d11txh_t */
	wlc_compute_plcp(wlc, bsscfg, rspec, phylen, fc, plcp);
	wlc_compute_plcp(wlc, bsscfg, rspec_fallback, phylen, fc, plcp_fallback);
	bcopy(plcp_fallback, (char*)&txh->FragPLCPFallback, sizeof(txh->FragPLCPFallback));

	/* Length field now put in CCK FBR CRC field for AES */
	if (RSPEC_ISCCK(rspec_fallback)) {
		txh->FragPLCPFallback[4] = phylen & 0xff;
		txh->FragPLCPFallback[5] = (phylen & 0xff00) >> 8;
	}

#ifdef WLAMPDU
	/* mark pkt as aggregable if it is */
	if (WLPKTFLAG_AMPDU(pkttag) && IS_MCS(rspec)) {
		if (WLC_KEY_ALLOWS_AMPDU(key_info)) {
			WLPKTTAG(p)->flags |= WLF_MIMO;
			if (WLC_HT_GET_AMPDU_RTS(wlc->hti))
				use_rts = TRUE;
		}
	}
#endif /* WLAMPDU */

	/* MIMO-RATE: need validation ?? */
	mainrates = RSPEC_ISOFDM(rspec) ? D11A_PHY_HDR_GRATE((ofdm_phy_hdr_t *)plcp) : plcp[0];

	/* DUR field for main rate */
	if (((fc & FC_KIND_MASK) != FC_PS_POLL) && !ETHER_ISMULTI(&h->a1) && !use_rifs) {
#ifdef WLAMPDU
		if (WLPKTFLAG_AMPDU(pkttag))
			durid = wlc_compute_ampdu_mpdu_dur(wlc,
				CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec), rspec);
		else
#endif /* WLAMPDU */
		durid = wlc_compute_frame_dur(wlc,
				CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
				rspec, preamble_type, next_frag_len);
		h->durid = htol16(durid);
	} else if (use_rifs) {
		/* NAV protect to end of next max packet size */
		durid = (uint16)wlc_calc_frame_time(wlc, rspec, preamble_type, DOT11_MAX_FRAG_LEN);
		durid += RIFS_11N_TIME;
		h->durid = htol16(durid);
	}

	/* DUR field for fallback rate */
	if (fc == FC_PS_POLL)
		txh->FragDurFallback = h->durid;
	else if (ETHER_ISMULTI(&h->a1) || use_rifs)
		txh->FragDurFallback = 0;
	else {
#ifdef WLAMPDU
		if (WLPKTFLAG_AMPDU(pkttag))
			durid = wlc_compute_ampdu_mpdu_dur(wlc,
				CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec), rspec_fallback);
		else
#endif /* WLAMPDU */
		durid = wlc_compute_frame_dur(wlc, CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
				rspec_fallback, fbr_preamble_type, next_frag_len);
		txh->FragDurFallback = htol16(durid);
	}

	/* Timestamp */
	if ((pkttag->flags & WLF_EXPTIME)) {
		txh->TstampLow = htol16(pkttag->u.exptime & 0xffff);
		txh->TstampHigh = htol16((pkttag->u.exptime >> 16) & 0xffff);

		mcl |= TXC_LIFETIME;	/* Enable timestamp for the packet */
	}

	/* (4) MAC-HDR: MacTxControlLow */
	if (frag == 0)
		mcl |= TXC_STARTMSDU;

	if (!ETHER_ISMULTI(&h->a1) && !WLPKTFLAG_RIFS(pkttag) &&
	    !(wlc->_amsdu_noack && WLPKTFLAG_AMSDU(pkttag)) &&
	    !(!WLPKTFLAG_AMPDU(pkttag) && qos && wme->wme_noack))
		mcl |= TXC_IMMEDACK;

	if (type == FC_TYPE_DATA && (queue < TX_BCMC_FIFO)) {
		if ((WLC_HT_GET_FRAMEBURST(hti) && (rate > WLC_FRAMEBURST_MIN_RATE) &&
#ifdef WLAMPDU
		     (wlc_ampdu_frameburst_override(wlc->ampdu_tx, scb) == FALSE) &&
#endif
		     (!wme->edcf_txop[ac] || WLPKTFLAG_AMPDU(pkttag))) ||
		     FALSE)
#ifdef WL11N
			/* don't allow bursting if rts is required around each mimo frame */
			if (use_mimops_rts == FALSE)
#endif
				mcl |= TXC_FRAMEBURST;
	}

	if (BAND_5G(wlc->band->bandtype))
		mcl |= TXC_FREQBAND_5G;

	if (CHSPEC_IS40(WLC_BAND_PI_RADIO_CHANSPEC))
		mcl |= TXC_BW_40;

	txh->MacTxControlLow = htol16(mcl);

	/* MacTxControlHigh */
	mch = 0;

	/* Set fallback rate preamble type */
	if ((fbr_preamble_type == WLC_SHORT_PREAMBLE) ||
	    (fbr_preamble_type == WLC_GF_PREAMBLE)) {
		ASSERT((fbr_preamble_type == WLC_GF_PREAMBLE) ||
		       (!IS_MCS(rspec_fallback)));
		mch |= TXC_PREAMBLE_DATA_FB_SHORT;
	}

	/* MacFrameControl */
	bcopy((char*)&h->fc, (char*)&txh->MacFrameControl, sizeof(uint16));

	txh->TxFesTimeNormal = htol16(0);

	txh->TxFesTimeFallback = htol16(0);

	/* TxFrameRA */
	bcopy((char*)&h->a1, (char*)&txh->TxFrameRA, ETHER_ADDR_LEN);

	/* TxFrameID */
	txh->TxFrameID = htol16(frameid);

#ifdef WL11N
	/* Set tx antenna configuration for all transmissions */
	if (WLANTSEL_ENAB(wlc)) {
		if (antcfg == ANTCFG_NONE) {
			/* use tx antcfg default */
			wlc_antsel_antcfg_get(wlc->asi, TRUE, FALSE, 0, 0, &antcfg, &fbantcfg);
		}
		mimoantsel = wlc_antsel_buildtxh(wlc->asi, antcfg, fbantcfg);
		txh->ABI_MimoAntSel = htol16(mimoantsel);
	}
#endif /* WL11N */

#ifdef WLMCNX
	if (MCNX_ENAB(wlc->pub)) {
		uint16 bss = (uint16)wlc_mcnx_BSS_idx(wlc->mcnx, bsscfg);
		ASSERT(bss < M_P2P_BSS_MAX);
		txh->ABI_MimoAntSel |= htol16(bss << ABI_MAS_ADDR_BMP_IDX_SHIFT);
	}
#endif

	/* TxStatus, Note the case of recreating the first frag of a suppressed frame
	 * then we may need to reset the retry cnt's via the status reg
	 */
	txh->TxStatus = htol16(status);

	if (D11REV_GE(wlc->pub->corerev, 16)) {
		/* extra fields for ucode AMPDU aggregation, the new fields are added to
		 * the END of previous structure so that it's compatible in driver.
		 * In old rev ucode, these fields should be ignored
		 */
		txh->MaxNMpdus = htol16(0);
		txh->u1.MaxAggDur = htol16(0);
		txh->u2.MaxAggLen_FBR = htol16(0);
		txh->MinMBytes = htol16(0);
	}

	/* (5) RTS/CTS: determine RTS/CTS PLCP header and MAC duration, furnish d11txh_t */

	/* RTS PLCP header and RTS frame */
	if (use_rts || use_cts) {
		if (use_rts && use_cts)
			use_cts = FALSE;

#ifdef NOT_YET
		if (SCB_ISGF_CAP(scb)) {
			hdr_mcs_mixedmode = FALSE;
			hdr_cckofdm_shortpreamble = TRUE;
		}
#endif /* NOT_YET */

		rts_rspec = wlc_rspec_to_rts_rspec(bsscfg, rspec, FALSE);
		rts_rspec_fallback = wlc_rspec_to_rts_rspec(bsscfg, rspec_fallback, FALSE);

		if (!RSPEC_ISOFDM(rts_rspec) &&
		    !((RSPEC2RATE(rts_rspec) == WLC_RATE_1M) ||
		      (scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG))) {
			rts_preamble_type = WLC_SHORT_PREAMBLE;
			mch |= TXC_PREAMBLE_RTS_MAIN_SHORT;
		}

		if (!RSPEC_ISOFDM(rts_rspec_fallback) &&
		    !((RSPEC2RATE(rts_rspec_fallback) == WLC_RATE_1M) ||
		      (scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG))) {
			rts_fbr_preamble_type = WLC_SHORT_PREAMBLE;
			mch |= TXC_PREAMBLE_RTS_FB_SHORT;
		}

		/* RTS/CTS additions to MacTxControlLow */
		if (use_cts) {
			txh->MacTxControlLow |= htol16(TXC_SENDCTS);
		} else {
			txh->MacTxControlLow |= htol16(TXC_SENDRTS);
			txh->MacTxControlLow |= htol16(TXC_LONGFRAME);
		}

		/* RTS PLCP header */
		ASSERT(ISALIGNED(txh->RTSPhyHeader, sizeof(uint16)));
		rts_plcp = txh->RTSPhyHeader;
		if (use_cts)
			rts_phylen = DOT11_CTS_LEN + DOT11_FCS_LEN;
		else
			rts_phylen = DOT11_RTS_LEN + DOT11_FCS_LEN;

		/* dot11n headers */
		wlc_compute_plcp(wlc, bsscfg, rts_rspec, rts_phylen, fc, rts_plcp);

		/* fallback rate version of RTS PLCP header */
		wlc_compute_plcp(wlc, bsscfg, rts_rspec_fallback, rts_phylen,
			fc, rts_plcp_fallback);

		bcopy(rts_plcp_fallback, (char*)&txh->RTSPLCPFallback,
			sizeof(txh->RTSPLCPFallback));

		/* RTS frame fields... */
		rts = (struct dot11_rts_frame*)&txh->rts_frame;

		durid = wlc_compute_rtscts_dur(wlc, CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
			use_cts, rts_rspec, rspec, rts_preamble_type, preamble_type, phylen, FALSE);
		rts->durid = htol16(durid);

		/* fallback rate version of RTS DUR field */
		durid = wlc_compute_rtscts_dur(wlc, CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
			use_cts, rts_rspec_fallback, rspec_fallback, rts_fbr_preamble_type,
		        fbr_preamble_type, phylen, FALSE);
		txh->RTSDurFallback = htol16(durid);

		if (use_cts) {
			rts->fc = htol16(FC_CTS);
			bcopy((char*)&h->a2, (char*)&rts->ra, ETHER_ADDR_LEN);
		} else {
			rts->fc = htol16((uint16)FC_RTS);
			bcopy((char*)&h->a1, (char*)&rts->ra, ETHER_ADDR_LEN);
			bcopy((char*)&h->a2, (char*)&rts->ta, ETHER_ADDR_LEN);
		}

		/* mainrate
		 *    low 8 bits: main frag rate/mcs,
		 *    high 8 bits: rts/cts rate/mcs
		 */
		mainrates |= (RSPEC_ISOFDM(rts_rspec) ?
			D11A_PHY_HDR_GRATE((ofdm_phy_hdr_t *)rts_plcp) : rts_plcp[0]) << 8;
	} else {
		bzero((char*)txh->RTSPhyHeader, D11_PHY_HDR_LEN);
		bzero((char*)&txh->rts_frame, sizeof(struct dot11_rts_frame));
		bzero((char*)txh->RTSPLCPFallback, sizeof(txh->RTSPLCPFallback));
		txh->RTSDurFallback = 0;
	}

#ifdef WLAMPDU
	/* add null delimiter count */
	if (WLPKTFLAG_AMPDU(pkttag) && IS_MCS(rspec)) {
		uint16 minbytes = 0;
		txh->RTSPLCPFallback[AMPDU_FBR_NULL_DELIM] =
			wlc_ampdu_null_delim_cnt(wlc->ampdu_tx, scb, rspec, phylen, &minbytes);
#ifdef WLAMPDU_HW
		if (AMPDU_HW_ENAB(wlc->pub))
			txh->MinMBytes = htol16(minbytes);
#endif
	}
#endif	/* WLAMPDU */

	/* Now that RTS/RTS FB preamble types are updated, write the final value */
	txh->MacTxControlHigh = htol16(mch);

	/* MainRates (both the rts and frag plcp rates have been calculated now) */
	txh->MainRates = htol16(mainrates);

	/* XtraFrameTypes */
	xfts = PHY_TXC_FRAMETYPE(rspec_fallback);
	xfts |= (PHY_TXC_FRAMETYPE(rts_rspec) << XFTS_RTS_FT_SHIFT);
	xfts |= (PHY_TXC_FRAMETYPE(rts_rspec_fallback) << XFTS_FBRRTS_FT_SHIFT);
	xfts |= CHSPEC_CHANNEL(WLC_BAND_PI_RADIO_CHANSPEC) << XFTS_CHANNEL_SHIFT;
	txh->XtraFrameTypes = htol16(xfts);

	/* PhyTxControlWord */
	phyctl = PHY_TXC_FRAMETYPE(rspec);
	if ((preamble_type == WLC_SHORT_PREAMBLE) ||
	    (preamble_type == WLC_GF_PREAMBLE)) {
		ASSERT((preamble_type == WLC_GF_PREAMBLE) || !IS_MCS(rspec));
		phyctl |= PHY_TXC_SHORT_HDR;
		WLCNTINCR(wlc->pub->_cnt->txprshort);
	}

	/* phytxant is properly bit shifted */
	phyctl |= wlc_stf_d11hdrs_phyctl_txant(wlc, rspec);
	if (WLCISNPHY(wlc->band) && (wlc->pub->sromrev >= 9)) {
		uint8 rate_offset;
		uint16 minbytes;
		rate_offset = wlc_stf_get_pwrperrate(wlc, rspec, 0);
		phyctl |= (rate_offset <<  PHY_TXC_PWR_SHIFT);
		rate_offset = wlc_stf_get_pwrperrate(wlc, rspec_fallback, 0);
		minbytes = ltoh16(txh->MinMBytes);
		minbytes |= (rate_offset << MINMBYTES_FBRATE_PWROFFSET_SHIFT);
		txh->MinMBytes = htol16(minbytes);
	}

#ifdef WL_LPC
	if ((pkttag->flags & WLF_RATE_AUTO) && LPC_ENAB(wlc)) {
			uint16 ratepwr_offset = 0;

			/* Note the per rate power offset for this rate */
			ratepwr_offset = wlc_phy_lpc_get_txcpwrval(WLC_PI(wlc), phyctl);

			/* Update the Power offset bits */
			if (ratepwr_offset) {
				/* for the LPC enabled 11n chips ratepwr_offset should be 0 */
				ASSERT(FALSE);
			}
			wlc_phy_lpc_set_txcpwrval(WLC_PI(wlc), &phyctl, lpc_offset);
	}
#endif /* WL_LPC */
	txh->PhyTxControlWord = htol16(phyctl);

	/* PhyTxControlWord_1 */
	if (WLC_PHY_11N_CAP(wlc->band)) {
		uint16 phyctl1 = 0;

		phyctl1 = wlc_phytxctl1_calc(wlc, rspec, wlc->chanspec);
		WLC_PHYCTLBW_OVERRIDE(phyctl1, PHY_TXC1_BW_MASK, phyctl_bwo);
		txh->PhyTxControlWord_1 = htol16(phyctl1);

		phyctl1 = wlc_phytxctl1_calc(wlc, rspec_fallback, wlc->chanspec);
		if (IS_MCS(rspec_fallback)) {
			WLC_PHYCTLBW_OVERRIDE(phyctl1, PHY_TXC1_BW_MASK, phyctl_bwo);
		}
		txh->PhyTxControlWord_1_Fbr = htol16(phyctl1);

		if (use_rts || use_cts) {
			phyctl1 = wlc_phytxctl1_calc(wlc, rts_rspec, wlc->chanspec);
			txh->PhyTxControlWord_1_Rts = htol16(phyctl1);
			phyctl1 = wlc_phytxctl1_calc(wlc, rts_rspec_fallback, wlc->chanspec);
			txh->PhyTxControlWord_1_FbrRts = htol16(phyctl1);
		}

		/*
		 * For mcs frames, if mixedmode(overloaded with long preamble) is going to be set,
		 * fill in non-zero MModeLen and/or MModeFbrLen
		 *  it will be unnecessary if they are separated
		 */
		if (IS_MCS(rspec) && (preamble_type == WLC_MM_PREAMBLE)) {
			uint16 mmodelen = wlc_calc_lsig_len(wlc, rspec, phylen);
			txh->MModeLen = htol16(mmodelen);
		}

		if (IS_MCS(rspec_fallback) && (fbr_preamble_type == WLC_MM_PREAMBLE)) {
			uint16 mmodefbrlen = wlc_calc_lsig_len(wlc, rspec_fallback, phylen);
			txh->MModeFbrLen = htol16(mmodefbrlen);
		}
	}

	ASSERT(!IS_MCS(rspec) ||
	       ((preamble_type == WLC_MM_PREAMBLE) == (txh->MModeLen != 0)));
	ASSERT(!IS_MCS(rspec_fallback) ||
	       ((fbr_preamble_type == WLC_MM_PREAMBLE) == (txh->MModeFbrLen != 0)));

	if (SCB_WME(scb) && qos && wme->edcf_txop[ac]) {
		uint frag_dur, dur, dur_fallback;

		ASSERT(!ETHER_ISMULTI(&h->a1));

		/* WME: Update TXOP threshold */
		if ((!WLPKTFLAG_AMPDU(pkttag)) && (frag == 0)) {
			int16 delta;

			frag_dur = wlc_calc_frame_time(wlc, rspec, preamble_type, phylen);

			if (rts) {
				/* 1 RTS or CTS-to-self frame */
				dur = wlc_calc_cts_time(wlc, rts_rspec, rts_preamble_type);
				dur_fallback = wlc_calc_cts_time(wlc, rts_rspec_fallback,
				                                 rts_fbr_preamble_type);
				/* (SIFS + CTS) + SIFS + frame + SIFS + ACK */
				dur += ltoh16(rts->durid);
				dur_fallback += ltoh16(txh->RTSDurFallback);
			} else if (use_rifs) {
				dur = frag_dur;
				dur_fallback = 0;
			} else {
				/* frame + SIFS + ACK */
				dur = frag_dur;
				dur += wlc_compute_frame_dur(wlc,
					CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
					rspec, preamble_type, 0);

				dur_fallback = wlc_calc_frame_time(wlc, rspec_fallback,
				               fbr_preamble_type, phylen);
				dur_fallback += wlc_compute_frame_dur(wlc,
						CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
						rspec_fallback, fbr_preamble_type, 0);
			}
			/* NEED to set TxFesTimeNormal (hard) */
			txh->TxFesTimeNormal = htol16((uint16)dur);
			/* NEED to set fallback rate version of TxFesTimeNormal (hard) */
			txh->TxFesTimeFallback = htol16((uint16)dur_fallback);

			/* update txop byte threshold (txop minus intraframe overhead) */
			delta = (int16)(wme->edcf_txop[ac] - (dur - frag_dur));
			if (delta >= 0) {
#ifdef WLAMSDU_TX
				if (AMSDU_TX_ENAB(wlc->pub) &&
				    WLPKTFLAG_AMSDU(pkttag) && (queue == TX_AC_BE_FIFO)) {
					WL_ERROR(("edcf_txop changed, update AMSDU\n"));
					wlc_amsdu_txop_upd(wlc->ami);
				} else
#endif
				{

					if (!(pkttag->flags & WLF_8021X)) {
						uint newfragthresh =
							wlc_calc_frame_len(wlc, rspec,
								preamble_type, (uint16)delta);
						/* range bound the fragthreshold */
						newfragthresh = MAX(newfragthresh,
							DOT11_MIN_FRAG_LEN);
						newfragthresh = MIN(newfragthresh,
							wlc->usr_fragthresh);
						/* update the fragthresh and do txc update */
						if (wlc->fragthresh[ac] != (uint16)newfragthresh)
						{
							wlc_fragthresh_set(wlc,
								ac, (uint16)newfragthresh);
						}
					}
				}
			} else
				WL_ERROR(("wl%d: %s txop invalid for rate %d\n",
					wlc->pub->unit, fifo_names[queue], RSPEC2RATE(rspec)));

			if (CAC_ENAB(wlc->pub) &&
				queue <= TX_AC_VO_FIFO) {
				/* update cac used time */
				if (wlc_cac_update_used_time(wlc->cac, ac, dur, scb))
					WL_ERROR(("wl%d: ac %d: txop exceeded allocated TS time\n",
						wlc->pub->unit, ac));
			}

			if (dur > wme->edcf_txop[ac])
				WL_ERROR(("wl%d: %s txop exceeded phylen %d/%d dur %d/%d\n",
					wlc->pub->unit, fifo_names[queue], phylen,
					wlc->fragthresh[ac], dur, wme->edcf_txop[ac]));
		}
	} else if (SCB_WME(scb) && qos && CAC_ENAB(wlc->pub) && queue <= TX_AC_VO_FIFO) {
		uint dur;
		if (rts) {
			/* 1 RTS or CTS-to-self frame */
			dur = wlc_calc_cts_time(wlc, rts_rspec, rts_preamble_type);
			/* (SIFS + CTS) + SIFS + frame + SIFS + ACK */
			dur += ltoh16(rts->durid);
		} else {
			/* frame + SIFS + ACK */
			dur = wlc_calc_frame_time(wlc, rspec, preamble_type, phylen);
			dur += wlc_compute_frame_dur(wlc,
				CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
				rspec, preamble_type, 0);
		}

		/* update cac used time */
		if (wlc_cac_update_used_time(wlc->cac, ac, dur, scb))
			WL_ERROR(("wl%d: ac %d: txop exceeded allocated TS time\n",
				wlc->pub->unit, ac));
	}


	/* With d11 hdrs on, mark the packet as MPDU with txhdr */
	WLPKTTAG(p)->flags |= (WLF_MPDU | WLF_TXHDR);

	/* TDLS U-APSD buffer STA: if peer is in PS, save the last MPDU's seq and tid for PTI */
#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) &&
		SCB_PS(scb) &&
		wlc_tdls_buffer_sta_enable(wlc->tdls)) {
		uint8 tid = 0;
		if (qos) {
			uint16 qc;
			qc = (*(uint16 *)((uchar *)h + (a4 ?
				DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN)));
			tid = (QOS_TID(qc) & 0x0f);
		}
		wlc_tdls_update_tid_seq(wlc->tdls, scb, tid, seq);
	}
#endif

	return (frameid);
} /* wlc_d11hdrs_pre40 */

#ifdef WL11N
/** At this point the rspec may not include a valid txbw. Pick a transmit bandwidth. */
static INLINE ratespec_t
wlc_d11ac_hdrs_determine_mimo_txbw(wlc_info_t *wlc, struct scb *scb, wlc_bsscfg_t *bsscfg,
                                   ratespec_t rspec)
{
	uint32 mimo_txbw;

	if (RSPEC_BW(rspec) != RSPEC_BW_UNSPECIFIED) {
		mimo_txbw = RSPEC_BW(rspec);

		/* If the ratespec override has a bw greater than
		 * the channel bandwidth, limit here.
		 */
		if (CHSPEC_IS20(wlc->chanspec)) {
			mimo_txbw = RSPEC_BW_20MHZ;
		} else if (CHSPEC_IS40(wlc->chanspec)) {
			mimo_txbw = MIN(mimo_txbw, RSPEC_BW_40MHZ);
		} else if (CHSPEC_IS80(wlc->chanspec)) {
			mimo_txbw = MIN(mimo_txbw, RSPEC_BW_80MHZ);
		} else if (CHSPEC_IS160(wlc->chanspec) || CHSPEC_IS8080(wlc->chanspec)) {
			mimo_txbw = MIN(mimo_txbw, RSPEC_BW_160MHZ);
		}
	}
	/* Is the phy configured to use > 20MHZ frames? If so then pick the
	 * desired txbw
	 */
	else if (CHSPEC_IS8080(wlc->chanspec) &&
		RSPEC_ISVHT(rspec) && (scb->flags3 & SCB3_IS_80_80)) {
		mimo_txbw = RSPEC_BW_160MHZ;
	} else if (CHSPEC_IS160(wlc->chanspec) &&
		RSPEC_ISVHT(rspec) && (scb->flags3 & SCB3_IS_160)) {
		mimo_txbw = RSPEC_BW_160MHZ;
	} else if (CHSPEC_IS80(wlc->chanspec) && RSPEC_ISVHT(rspec)) {
		mimo_txbw = RSPEC_BW_80MHZ;
	} else if (CHSPEC_BW_GE(wlc->chanspec, WL_CHANSPEC_BW_40)) {
		/* default txbw is 20in40 */
		mimo_txbw = RSPEC_BW_20MHZ;

		if (RSPEC_ISHT(rspec) || RSPEC_ISVHT(rspec)) {
			if (scb->flags & SCB_IS40) {
				mimo_txbw = RSPEC_BW_40MHZ;
#ifdef WLMCHAN
				if (MCHAN_ENAB(wlc->pub) && BSSCFG_AP(bsscfg) &&
					CHSPEC_IS20(
					bsscfg->current_bss->chanspec)) {
					mimo_txbw = RSPEC_BW_20MHZ;
				}
#endif /* WLMCHAN */
			}
		}

		if (RSPEC_ISHT(rspec) && ((rspec & RSPEC_RATE_MASK) == 32))
			mimo_txbw = RSPEC_BW_40MHZ;
	} else	{
		mimo_txbw = RSPEC_BW_20MHZ;
	}

	return mimo_txbw;
} /* wlc_d11ac_hdrs_determine_mimo_txbw */
#endif /* WL11N */

/**
 * If the caller decided that an rts or cts frame needs to be transmitted, the transmit properties
 * of the rts/cts have to be determined and communicated to the transmit hardware. RTS/CTS rates are
 * always legacy rates (HR/DSSS, ERP, or OFDM).
 */
static INLINE void
wlc_d11ac_hdrs_rts_cts(struct scb *scb /* [in] */, wlc_bsscfg_t *bsscfg /* [in] */,
	ratespec_t rspec, bool use_rts, bool use_cts,
	ratespec_t *rts_rspec /* [out] */, d11actxh_rate_t *rate_hdr /* [in/out] */,
	uint8 *rts_preamble_type /* [out] */, uint16 *mcl /* [in/out] MacControlLow */)
{
	*rts_preamble_type = WLC_LONG_PREAMBLE;
	/* RTS PLCP header and RTS frame */
	if (use_rts || use_cts) {
		uint16 phy_rate;
		uint8 rts_rate;

		if (use_rts && use_cts)
			use_cts = FALSE;

#ifdef NOT_YET
		if (SCB_ISGF_CAP(scb)) {
			hdr_mcs_mixedmode = FALSE;
			hdr_cckofdm_shortpreamble = TRUE;
		}
#endif /* NOT_YET */

		*rts_rspec = wlc_rspec_to_rts_rspec(bsscfg, rspec, FALSE);
		ASSERT(RSPEC_ISLEGACY(*rts_rspec));

		/* extract the MAC rate in 0.5Mbps units */
		rts_rate = (*rts_rspec & RSPEC_RATE_MASK);

		rate_hdr->RtsCtsControl = RSPEC_ISCCK(*rts_rspec) ?
			htol16(D11AC_RTSCTS_FRM_TYPE_11B) :
			htol16(D11AC_RTSCTS_FRM_TYPE_11AG);

		if (!RSPEC_ISOFDM(*rts_rspec) &&
			!((rts_rate == WLC_RATE_1M) ||
			(scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG))) {
			*rts_preamble_type = WLC_SHORT_PREAMBLE;
			rate_hdr->RtsCtsControl |= htol16(D11AC_RTSCTS_SHORT_PREAMBLE);
		}

		/* set RTS/CTS flag */
		if (use_cts) {
			rate_hdr->RtsCtsControl |= htol16(D11AC_RTSCTS_USE_CTS);
		} else {
			rate_hdr->RtsCtsControl |= htol16(D11AC_RTSCTS_USE_RTS);
			*mcl |= D11AC_TXC_LFRM;
		}

		/* RTS/CTS Rate index - Bits 3-0 of plcp byte0	*/
		phy_rate = rate_info[rts_rate] & 0xf;
		rate_hdr->RtsCtsControl |= htol16((phy_rate << 8));
	} else {
		rate_hdr->RtsCtsControl = 0;
	}
} /* wlc_d11ac_hdrs_rts_cts */

#define FBW_BW_20MHZ		4
#define FBW_BW_40MHZ		5
#define FBW_BW_80MHZ		6
#define FBW_BW_INVALID		(FBW_BW_20MHZ + 3)

#ifdef WL11N
static INLINE uint8 BCMFASTPATH
wlc_tx_dynbw_fbw(wlc_info_t *wlc, struct scb *scb, ratespec_t rspec)
{
	uint8 fbw = FBW_BW_INVALID;

	if (RSPEC_BW(rspec) == RSPEC_BW_80MHZ)
		fbw = FBW_BW_40MHZ;
	else if (RSPEC_BW(rspec) == RSPEC_BW_40MHZ) {
#ifdef WL11AC
		if (RSPEC_ISVHT(rspec)) {
			uint8 mcs, nss, prop_mcs = VHT_PROP_MCS_MAP_NONE;
			bool ldpc;
			uint16 mcsmap = 0;
			uint8 vht_ratemask = wlc_vht_get_scb_ratemask(wlc->vhti, scb);
			mcs = rspec & RSPEC_VHT_MCS_MASK;
			nss = (rspec & RSPEC_VHT_NSS_MASK) >> RSPEC_VHT_NSS_SHIFT;
			ldpc = (rspec & RSPEC_LDPC_CODING) ? TRUE : FALSE;
			mcs = VHT_MCS_MAP_GET_MCS_PER_SS(nss, vht_ratemask);
			if (vht_ratemask & WL_VHT_FEATURES_1024QAM)
				prop_mcs = VHT_PROP_MCS_MAP_10_11;
			mcsmap = wlc_get_valid_vht_mcsmap(mcs, prop_mcs, BW_20MHZ,
			                                  ldpc, nss, vht_ratemask);
			if (mcsmap & (1 << mcs))
				fbw = FBW_BW_20MHZ;
		} else
#endif /* WL11AC */
			fbw = FBW_BW_20MHZ;
	}

	return fbw;
}
#endif /* WL11N */

typedef struct wlc_d11mac_params wlc_d11mac_parms_t;
struct wlc_d11mac_params {
	struct dot11_header *h;
	wlc_pkttag_t *pkttag;
	uint16 fc;
	uint16 type;
	bool qos;
	bool a4;
	int len;
	int phylen;
	uint8 IV_offset;
	uint frag;
	uint nfrags;
};

/**
 * Function to extract 80211 mac hdr information from the pkt.
 */
static void
wlc_extract_80211mac_hdr(wlc_info_t *wlc, void *p, wlc_d11mac_parms_t *parms,
	const wlc_key_info_t *key_info, uint frag, uint nfrags)
{
	uint8 *IV_offset = &(parms->IV_offset);
	uint16 *type = &(parms->type);
	int *phylen = &(parms->phylen);

	parms->frag = frag;
	parms->nfrags = nfrags;

	/* locate 802.11 MAC header */
	parms->h = (struct dot11_header*) PKTDATA(wlc->osh, p);
	parms->pkttag = WLPKTTAG(p);
	parms->fc = ltoh16((parms->h)->fc);
	*type = FC_TYPE(parms->fc);
	parms->qos = (*type == FC_TYPE_DATA &&
		FC_SUBTYPE_ANY_QOS(FC_SUBTYPE(parms->fc)));
	parms->a4 = (parms->fc & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS);

	/* compute length of frame in bytes for use in PLCP computations */
	parms->len = pkttotlen(wlc->osh, p);
	*phylen = parms->len + DOT11_FCS_LEN;

	/* Add room in phylen for the additional bytes for icv, if required.
	 * this is true for both h/w and s/w encryption. The latter only
	 * modifies the pkt length
	 */

	if (key_info != NULL) {

		if (WLC_KEY_IS_MGMT_GROUP(key_info))
			*phylen += WLC_KEY_MMIC_IE_LEN(key_info);
		else
			*phylen += key_info->icv_len;

		/* external crypto adds iv to the pkt, include it in phylen */
		if (WLC_KEY_IS_LINUX_CRYPTO(key_info))
			*phylen += key_info->iv_len;

		if (WLC_KEY_FRAG_HAS_TKIP_MIC(p, key_info,
			frag, nfrags))
			*phylen += TKIP_MIC_SIZE;
	}

	/* IV Offset, i.e. start of the 802.11 header */

	*IV_offset = DOT11_A3_HDR_LEN;

	if (*type == FC_TYPE_DATA) {
		if (parms->a4)
			*IV_offset += ETHER_ADDR_LEN;

		if (parms->qos)
			*IV_offset += DOT11_QOS_LEN;

	 } else if (*type == FC_TYPE_CTL) {
		/* Subtract one address and SeqNum */
		*IV_offset -= ETHER_ADDR_LEN + 2;
	 }

	WL_NONE(("wl%d: %s: len %d, phylen %d\n", WLCWLUNIT(wlc), __FUNCTION__,
		parms->len, *phylen));
}

/**
 * Function to compute frameid.
 */
static INLINE void
wlc_compute_frameid(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint16 txFrameID,
	uint queue, uint16 *frameid)
{
	if (queue == TX_BCMC_FIFO) { /* broadcast/multicast */
		*frameid = bcmc_fid_generate(wlc, bsscfg, txFrameID);
	} else {
		*frameid = (((wlc->counter++) << TXFID_SEQ_SHIFT) & TXFID_SEQ_MASK) |
			WLC_TXFID_SET_QUEUE(queue);
	}
}

static INLINE void
wlc_compute_initial_MacTxCtrl(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct scb *scb, uint16 *mcl, uint16 *mch, wlc_d11mac_parms_t *parms,
	bool mutx_pkteng_on)
{
	wlc_pkttag_t *pkttag = parms->pkttag;

	/* MAC-HDR: MacTxControlLow */
	/* set the ignpmq bit for all pkts tx'd in PS mode and for beacons and for anything
	 * going out from a STA interface.
	 */
	if (SCB_PS(scb) || ((parms->fc & FC_KIND_MASK) == FC_BEACON) || BSSCFG_STA(bsscfg))
		*mcl |= D11AC_TXC_IPMQ;

#ifndef WL11AX
#ifdef WL_MUPKTENG
	if (mutx_pkteng_on)
		*mcl |= D11AC_TXC_IPMQ;
#endif /* WL_MUPKTENG */
#endif /* WL11AX */

	if (parms->frag == 0)
		*mcl |= D11AC_TXC_STMSDU;

	/* MacTxControlHigh */
	/* start from fix rate. clear it if auto */
	*mch = D11AC_TXC_FIX_RATE;

#ifdef PSPRETEND
	/* If D11AC_TXC_PPS is set, ucode is monitoring for failed TX status. In that case,
	 * it will add new PMQ entry so start the process of draining the TX fifo.
	 * Normal PS pretend is currently only supporting AMPDU traffic. Of course, PS pretend
	 * should be in enabled state and we guard against the "ignore PMQ" because this
	 * is used to force a packet to air.
	 */

	/* Do not allow IBSS bsscfg to use normal pspretend yet,
	 * as PMQ process is not supported for them now.
	 */
	if (WLPKTFLAG_AMPDU(pkttag) && SCB_PS_PRETEND_ENABLED(bsscfg, scb) &&
	    !BSSCFG_IBSS(bsscfg) && !(*mcl & D11AC_TXC_IPMQ)) {
		*mch |= D11AC_TXC_PPS;
	}
#endif /* PSPRETEND */

	/* Setting Packet Expiry */
	if (pkttag->flags & WLF_EXPTIME) {
		*mcl |= D11AC_TXC_AGING; /* Enable timestamp for the packet */
	}

	if (!ETHER_ISMULTI(&parms->h->a1) && !WLPKTFLAG_RIFS(pkttag) &&
		!(wlc->_amsdu_noack && WLPKTFLAG_AMSDU(pkttag)) &&
		!(!WLPKTFLAG_AMPDU(pkttag) && parms->qos && bsscfg->wme->wme_noack))
		*mcl |= D11AC_TXC_IACK;
}

/**
 * Function to fech / compute and assign seq numbers to MAC hdr.
 * 1. Try to use preassigned / software seq numbers
 * 2. If SW generated seq is not available :
 *     - set MacTxControlLow to indicate usage of ucode generated seq
 */
static INLINE void
wlc_fill_80211mac_hdr_seqnum(wlc_info_t *wlc, void *p, struct scb *scb,
	uint16 *mcl, uint16 *seq, wlc_d11mac_parms_t *parms)
{
	wlc_pkttag_t *pkttag = parms->pkttag;
	/* use preassigned or software seqnum */
#ifdef PROP_TXSTATUS
	if (IS_WL_TO_REUSE_SEQ(pkttag->seq)) {
		*seq = WL_SEQ_GET_NUM(pkttag->seq);
	} else if (WLPKTFLAG_AMPDU(pkttag)) {
		*seq = WL_SEQ_GET_NUM(pkttag->seq);
	}
#else
	if (WLPKTFLAG_AMPDU(pkttag)) {
		*seq = pkttag->seq;
	}
#endif /* PROP_TXSTATUS */
	else if (SCB_QOS(scb) && ((parms->fc & FC_KIND_MASK) == FC_QOS_DATA) &&
		!ETHER_ISMULTI(&parms->h->a1)) {
		*seq = SCB_SEQNUM(scb, PKTPRIO(p));
		/* Increment the sequence number only after the last fragment */
		if (parms->frag == (parms->nfrags - 1))
			SCB_SEQNUM(scb, PKTPRIO(p))++;
	} else if (parms->type != FC_TYPE_CTL) {
		*mcl |= D11AC_TXC_ASEQ;
	}

	if (parms->type != FC_TYPE_CTL) {
		*seq = (*seq << SEQNUM_SHIFT) | (parms->frag & FRAGNUM_MASK);
		parms->h->seq = htol16(*seq);
	}

#ifdef PROP_TXSTATUS
	if (WLFC_GET_REUSESEQ(wlfc_query_mode(wlc->wlfc))) {
		WL_SEQ_SET_NUM(pkttag->seq, ltoh16(parms->h->seq) >> SEQNUM_SHIFT);
		SET_WL_HAS_ASSIGNED_SEQ(pkttag->seq);
	}
#endif /* PROP_TXSTATUS */

	WL_NONE(("wl%d: %s: seq %d, Add ucode generated SEQ %d\n",
		WLCWLUNIT(wlc), __FUNCTION__, *seq, (*mcl & D11AC_TXC_ASEQ)));
}

/**
 * Compute and fill "per packet info" (d11txh_rev80_pkt_t) field of the TX desc.
 * This field of size 22 bytes is also referred as the short TX desc (not cached).
 */
static INLINE void
wlc_fill_txd_per_pkt_info(wlc_info_t *wlc, void *p, struct scb *scb,
	wlc_d11pktinfo_t *D11PktInfo, uint16 mcl, uint16 mch, wlc_d11mac_parms_t *parms,
	uint16 frameid, uint16 d11_txh_len, uint16 HEModeControl)
{
	d11pktinfo_common_t *PktInfo = D11PktInfo->PktInfo;

	BCM_REFERENCE(d11_txh_len);
	BCM_REFERENCE(HEModeControl);

	/* Chanspec - channel info for packet suppression */
	PktInfo->Chanspec = htol16(wlc->chanspec);

	/* IV Offset, i.e. start of the 802.11 header */
	PktInfo->IVOffset = parms->IV_offset;

	/* FrameLen */
	PktInfo->FrameLen = htol16((uint16)parms->phylen);

	/* Sequence number final write */
	PktInfo->Seq = parms->h->seq;

#ifdef WL11AX
	if (D11REV_GE(wlc->pub->corerev, 80)) {
		d11pktinfo_rev80_t *PktInfoExt = D11PktInfo->PktInfoExt;

		/* Total length of TXD in bytes */
		PktInfoExt->length = htol16(d11_txh_len);

		/* For HE associations, apply tx policy */
		if (HE_ENAB(wlc->pub) && SCB_HE_CAP(scb)) {
			HEModeControl |=
				wlc_apply_he_tx_policy(wlc, scb, parms->type);
		}

		/* Todo : Fill HE Control */
		PktInfoExt->HEModeControl = htol16(HEModeControl);
	}
#endif /* WL11AX */

	/* Timestamp */
	if (parms->pkttag->flags & WLF_EXPTIME) {
		PktInfo->Tstamp = htol16((parms->pkttag->u.exptime >>
			D11AC_TSTAMP_SHIFT) & 0xffff);
	}

	/* TxFrameID (gettxrate may have updated it) */
	PktInfo->TxFrameID = htol16(frameid);

	/* MacTxControl High and Low */
	PktInfo->MacTxControlLow = htol16(mcl);
	PktInfo->MacTxControlHigh = htol16(mch);
}

/**
 * Function to fill the per Cache Info field of the txd
 */
static void
wlc_fill_txd_per_cache_info(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct scb *scb, void *p, wlc_pkttag_t *pkttag, wlc_d11txh_cache_info_u d11_cache_info,
	bool mutx_pkteng_on)
{
#ifdef WLMCNX
	if (MCNX_ENAB(wlc->pub)) {
		d11txh_cache_common_t *cache_info;
		uint16 bss = (uint16)wlc_mcnx_BSS_idx(wlc->mcnx, bsscfg);
		ASSERT(bss < M_P2P_BSS_MAX);
#ifdef WL11AX
		if (D11REV_GE(wlc->pub->corerev, 80)) {
			cache_info = &d11_cache_info.rev80_CacheInfo->common;
		} else
#endif
		{
			cache_info = &d11_cache_info.d11actxh_CacheInfo->common;
		}
		cache_info->BssIdEncAlg |= (bss << D11AC_BSSID_SHIFT);
	}
#endif /* WLMCNX */

#ifdef WLAMPDU_MAC
	if (WLPKTFLAG_AMPDU(pkttag) && D11REV_GE(wlc->pub->corerev, 40)) {
#ifdef WL_MUPKTENG
		if (mutx_pkteng_on)
			wlc_ampdu_mupkteng_fill_percache_info(wlc->ampdu_tx, scb,
				(uint8)PKTPRIO(p), d11_cache_info);
		else
#endif /* WL_MUPKTENG */
		{
			BCM_REFERENCE(mutx_pkteng_on);
			/* Fill per cache info */
			wlc_ampdu_fill_percache_info(wlc->ampdu_tx, scb,
				(uint8)PKTPRIO(p), d11_cache_info);
		}
	}
#endif /* WLAMPDU_MAC */
}

/**
 * Add d11actxh_t for 11ac mac & phy
 *
 * 'p' data must start with 802.11 MAC header
 * 'p' must allow enough bytes of local headers to be "pushed" onto the packet
 *
 * headroom == D11AC_TXH_LEN (D11AC_TXH_LEN is now 124 bytes which include PHY PLCP header)
 */
uint16
wlc_d11hdrs_rev40(wlc_info_t *wlc, void *p, struct scb *scb, uint txparams_flags, uint frag,
	uint nfrags, uint queue, uint next_frag_len, const wlc_key_info_t *key_info,
	ratespec_t rspec_override, uint16 *txh_off)
{
	d11actxh_t *txh;
	osl_t *osh;
	wlc_d11mac_parms_t parms;
	uint16 frameid, mch, phyctl, rate_flag;
	uint16 seq = 0, mcl = 0;
	bool use_rts = FALSE;
	bool use_cts = FALSE, rspec_history = FALSE;
	bool use_rifs = FALSE;
	bool short_preamble;
	uint8 preamble_type = WLC_LONG_PREAMBLE;
	uint8 rts_preamble_type;
	struct dot11_rts_frame *rts = NULL;
	ratespec_t rts_rspec = 0;
	uint8 ac;
	uint txrate;
	ratespec_t rspec;
	ratesel_txparams_t cur_rate;
#if defined(BCMDBG)
	uint16 phyctl_bwo = -1;
	uint16 phyctl_sbwo = -1;
#endif 
#if WL_HT_TXBW_OVERRIDE_ENAB
	int8 txbw_override_idx;
#endif
#ifdef WL11N
	wlc_ht_info_t *hti = wlc->hti;
	uint8 sgi_tx;
	bool use_mimops_rts = FALSE;
	int rspec_legacy = -1; /* is initialized to prevent compiler warning */
#endif /* WL11N */
	wlc_bsscfg_t *bsscfg;
	bool g_prot;
	int8 n_prot;
	wlc_wme_t *wme;
	uint corerev = wlc->pub->corerev;
	d11actxh_rate_t *rate_blk;
	d11actxh_rate_t *rate_hdr;
#ifdef BCM_SFD
	int sfd_id = 0;
	d11actxh_rate_t *sfd_rate_info;
	d11actxh_cache_t *sfd_cache_info;
#endif
	uint8 *plcp;
#ifdef WL_LPC
	uint8 lpc_offset = 0;
#endif
#if defined(WL_BEAMFORMING)
	uint8 bf_shm_index = BF_SHM_IDX_INV, bf_shmx_idx = BF_SHM_IDX_INV;
	bool bfen = FALSE, fbw_bfen = FALSE;
	bool mutx_en = FALSE, mutx_on = FALSE;
#endif /* WL_BEAMFORMING */
	uint8 fbw = FBW_BW_INVALID; /* fallback bw */
	txpwr204080_t txpwrs;
	int txpwr_bfsel;
	uint8 txpwr;
	int k;
	wlc_d11txh_cache_info_u d11_cache_info;
	wlc_d11pktinfo_t D11PktInfo;
	bool mutx_pkteng_on = FALSE;
	wl_tx_chains_t txbf_chains = 0;
	uint8	bfe_sts_cap = 0, txbf_uidx = 0;
#if defined(WL_PROT_OBSS) && !defined(WL_PROT_OBSS_DISABLED)
	ratespec_t phybw = (CHSPEC_IS8080(wlc->chanspec) || CHSPEC_IS160(wlc->chanspec)) ?
		RSPEC_BW_160MHZ :
		(CHSPEC_IS80(wlc->chanspec) ? RSPEC_BW_80MHZ :
		(CHSPEC_IS40(wlc->chanspec) ? RSPEC_BW_40MHZ : RSPEC_BW_20MHZ));
#endif

	BCM_REFERENCE(bfe_sts_cap);
	BCM_REFERENCE(txbf_chains);
	BCM_REFERENCE(mutx_pkteng_on);
	ASSERT(scb != NULL);
	ASSERT(queue < WLC_HW_NFIFO_INUSE(wlc));

	if (BYPASS_ENC_DATA(scb->bsscfg)) {
		key_info = NULL;
	}

#ifdef WL_MUPKTENG
	mutx_pkteng_on = wlc_mutx_pkteng_on(wlc->mutx);
#endif
	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	short_preamble = (txparams_flags & WLC_TX_PARAMS_SHORTPRE) != 0;

	g_prot = WLC_PROT_G_CFG_G(wlc->prot_g, bsscfg);
	n_prot = WLC_PROT_N_CFG_N(wlc->prot_n, bsscfg);

	wme = bsscfg->wme;

	osh = wlc->osh;

	/* Extract 802.11 mac hdr and compute frame length in bytes (for PLCP) */
	wlc_extract_80211mac_hdr(wlc, p, &parms, key_info, frag, nfrags);

	/*
	 * SFD frames have the following format
	 * (a) Lfrag contains PktInfo + 802.11Hdr + Frag Information
	 * (b) SFD cached header contains RateInfo + CacheInfo
	 *
	 * (a) and (b) are stiched together in D11Tx DMA routine
	 */
	if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(osh, p)) {
		ASSERT(0);
	}

#ifdef BCM_SFD
	/* Add Broadcom tx descriptor header */
	if (SFD_ENAB(wlc->pub) && PKTISTXFRAG(osh, p)) {
		txh = (d11actxh_t*)PKTPUSH(osh, p, D11AC_TXH_SFD_LEN);
		bzero((char*)txh, D11AC_TXH_SFD_LEN);

		sfd_id = wlc_sfd_entry_alloc(wlc->sfd, &sfd_rate_info, &sfd_cache_info);
		PKTSETSFDFRAME(osh, p);
	} else
#endif
	{
		txh = (d11actxh_t*)PKTPUSH(osh, p, D11AC_TXH_LEN);
		bzero((char*)txh, D11AC_TXH_LEN);
	}

	/*
	 * Some fields of the MacTxCtrl are tightly coupled,
	 * and dependant on various other params, which are
	 * yet to be computed. Due to complex dependancies
	 * with a number of params, filling of MacTxCtrl cannot
	 * be completely decoupled and made independant.
	 *
	 * Compute and fill independant fields of the the MacTxCtrl here.
	 */
	wlc_compute_initial_MacTxCtrl(wlc, bsscfg, scb, &mcl, &mch,
		&parms, mutx_pkteng_on);

	/* Compute and assign sequence number to MAC HDR */
	wlc_fill_80211mac_hdr_seqnum(wlc, p, scb, &mcl, &seq, &parms);

	/* TDLS U-APSD buffer STA: if peer is in PS, save the last MPDU's seq and tid for PTI */
#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) &&
		SCB_PS(scb) &&
		wlc_tdls_buffer_sta_enable(wlc->tdls)) {
		uint8 tid = 0;
		if (parms.qos) {
			uint16 qc;
			qc = (*(uint16 *)((uchar *)parms.h + (parms.a4 ?
				DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN)));
			tid = (QOS_TID(qc) & 0x0f);
		}
		wlc_tdls_update_tid_seq(wlc->tdls, scb, tid, seq);
	}
#endif

	/* Compute frameid, also possibly change seq */
	wlc_compute_frameid(wlc, bsscfg, txh->PktInfo.TxFrameID, queue, &frameid);

	/* TxStatus, Note the case of recreating the first frag of a suppressed frame
	 * then we may need to reset the retry cnt's via the status reg
	 */
	txh->PktInfo.TxStatus = 0;

	ac = WME_PRIO2AC(PKTPRIO(p));

	/* (1) RATE: determine and validate primary rate and fallback rates */

	cur_rate.num = 1; /* default to 1 */

	if (RSPEC_ACTIVE(rspec_override)) {
		cur_rate.rspec[0] = rspec_override;
		rspec_history = TRUE;
	} else if ((parms.type == FC_TYPE_MNG) ||
		(parms.type == FC_TYPE_CTL) ||
		((parms.pkttag)->flags & WLF_8021X) ||
		FALSE) {
		cur_rate.rspec[0] = scb->rateset.rates[0] & RATE_MASK;
	} else if (RSPEC_ACTIVE(wlc->band->mrspec_override) && ETHER_ISMULTI(&parms.h->a1)) {
		cur_rate.rspec[0] = wlc->band->mrspec_override;
	} else if (RSPEC_ACTIVE(wlc->band->rspec_override) && !ETHER_ISMULTI(&parms.h->a1)) {
		cur_rate.rspec[0] = wlc->band->rspec_override;
		rspec_history = TRUE;
	} else if (ETHER_ISMULTI(&parms.h->a1) || SCB_INTERNAL(scb)) {
		cur_rate.rspec[0] = scb->rateset.rates[0] & RATE_MASK;
	} else {
		/* run rate algorithm for data frame only, a cookie will be deposited in frameid */
		cur_rate.num = 4; /* enable multi fallback rate */
		cur_rate.ac = ac;
		wlc_scb_ratesel_gettxrate(wlc->wrsi, scb, &frameid, &cur_rate, &rate_flag);

#if defined(WL_LINKSTAT)
		if (LINKSTAT_ENAB(wlc->pub))
			wlc_cckofdm_tx_inc(wlc, parms.type,
				wlc_rate_rspec2rate(cur_rate.rspec[0])/500);
#endif /* WL_LINKSTAT */

		if (((scb->flags & SCB_BRCM) == 0) &&
		    (((parms.fc & FC_KIND_MASK) == FC_NULL_DATA) ||
		     ((parms.fc & FC_KIND_MASK) == FC_QOS_NULL))) {
			/* Use RTS/CTS rate for NULL frame */
			cur_rate.num = 2;
			cur_rate.rspec[0] =
				wlc_rspec_to_rts_rspec(bsscfg, cur_rate.rspec[0], FALSE);
			cur_rate.rspec[1] = scb->rateset.rates[0] & RATE_MASK;
		} else {
			(parms.pkttag)->flags |= WLF_RATE_AUTO;

			/* The rate histo is updated only for packets on auto rate. */
			/* perform rate history after txbw has been selected */
			if (frag == 0)
				rspec_history = TRUE;
		}

		mch &= ~D11AC_TXC_FIX_RATE;
#ifdef WL11N
		if (rate_flag & RATESEL_VRATE_PROBE)
			WLPKTTAG(p)->flags |= WLF_VRATE_PROBE;
#endif /* WL11N */

#ifdef WL_LPC
		if (LPC_ENAB(wlc)) {
			/* Query the power offset to be used from LPC */
			lpc_offset = wlc_scb_lpc_getcurrpwr(wlc->wlpci, scb, ac);
		} else {
			/* No Link Power Control. Transmit at nominal power. */
		}
#endif
	}

#ifdef WL_RELMCAST
	if (RMC_ENAB(wlc->pub))
		wlc_rmc_process(wlc->rmc, parms.type, p, parms.h, &mcl, &cur_rate.rspec[0]);
#endif

#ifdef WL_NAN
	if (NAN_ENAB(wlc->pub) && BSSCFG_NAN_MGMT(bsscfg)) {
		wlc_nan_set_d11actxh(wlc, bsscfg, p, parms.h, parms.fc, &mcl, &mch, txh);
	}
#endif
#ifdef WL_MU_TX
	if (SCB_MU(scb)) {
		bf_shmx_idx = wlc_txbf_get_mubfi_idx(wlc->txbf, scb);
	}
	if (bf_shmx_idx != BF_SHM_IDX_INV) {
		/* link is capable of mutx and has been enabled to do mutx */
		mutx_en = TRUE;
		if (WLPKTFLAG_AMPDU(parms.pkttag) &&
#if defined(BCMDBG) || defined(BCMDBG_MU)
		wlc_mutx_on(wlc->mutx) &&
#endif
		TRUE) {
			mutx_on = TRUE;
		}
	}
#endif /* WL_MU_TX */

#ifdef BCM_SFD
	if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(osh, p)) {
		rate_blk = sfd_rate_info;
	} else
#endif
	{
		rate_blk = WLC_TXD_RATE_INFO_GET(txh, corerev);
	}

	for (k = 0; k < cur_rate.num; k++) {
		/* init primary and fallback rate pointers */
		rspec = cur_rate.rspec[k];
		rate_hdr = &rate_blk[k];

		plcp = rate_hdr->plcp;
		rate_hdr->RtsCtsControl = 0;

#ifdef WL11N
		if (N_ENAB(wlc->pub)) {
			uint32 mimo_txbw;

			rspec_legacy = RSPEC_ISLEGACY(rspec);

			mimo_txbw = wlc_d11ac_hdrs_determine_mimo_txbw(wlc, scb, bsscfg, rspec);
#if WL_HT_TXBW_OVERRIDE_ENAB
			if (CHSPEC_IS40(wlc->chanspec) || CHSPEC_IS80(wlc->chanspec)) {
				WL_HT_TXBW_OVERRIDE_IDX(hti, rspec, txbw_override_idx);

				if (txbw_override_idx >= 0) {
					mimo_txbw = txbw2rspecbw[txbw_override_idx];
					phyctl_bwo = txbw2acphyctl0bw[txbw_override_idx];
					phyctl_sbwo = txbw2acphyctl1bw[txbw_override_idx];
				}
			}
#endif /* WL_HT_TXBW_OVERRIDE_ENAB */

#ifdef WL_OBSS_DYNBW
			if (WLC_OBSS_DYNBW_ENAB(wlc->pub)) {
				wlc_obss_dynbw_tx_bw_override(wlc->obss_dynbw, bsscfg,
					&mimo_txbw);
			}
#endif /* WL_OBSS_DYNBW */

			rspec &= ~RSPEC_BW_MASK;
			rspec |= mimo_txbw;
		} else /* N_ENAB */
#endif /* WL11N */
		{
			/* Set ctrlchbw as 20Mhz */
			ASSERT(RSPEC_ISLEGACY(rspec));
			rspec &= ~RSPEC_BW_MASK;
			rspec |= RSPEC_BW_20MHZ;
		}


#ifdef WL11N
		if (N_ENAB(wlc->pub)) {
			uint8 mimo_preamble_type;

			sgi_tx = WLC_HT_GET_SGI_TX(hti);

			if (!RSPEC_ACTIVE(wlc->band->rspec_override)) {
				bool _scb_stbc_on = FALSE;
				bool stbc_tx_forced = WLC_IS_STBC_TX_FORCED(wlc);
				bool stbc_ht_scb_auto = WLC_STF_SS_STBC_HT_AUTO(wlc, scb);
				bool stbc_vht_scb_auto = WLC_STF_SS_STBC_VHT_AUTO(wlc, scb);

				if (sgi_tx == OFF) {
					rspec &= ~RSPEC_SHORT_GI;
				} else if (sgi_tx == ON) {
					if (RSPEC_ISHT(rspec) || RSPEC_ISVHT(rspec)) {
						rspec |= RSPEC_SHORT_GI;
					}
				}


				ASSERT(!(rspec & RSPEC_LDPC_CODING));
				rspec &= ~RSPEC_LDPC_CODING;
				rspec &= ~RSPEC_STBC;

				/* LDPC */
				if (wlc->stf->ldpc_tx == ON ||
				    (((RSPEC_ISVHT(rspec) && SCB_VHT_LDPC_CAP(wlc->vhti, scb)) ||
				      (RSPEC_ISHT(rspec) && SCB_LDPC_CAP(scb))) &&
				     wlc->stf->ldpc_tx == AUTO)) {
					if (!rspec_legacy)
#ifdef WL_PROXDETECT
						if (!(PROXD_ENAB(wlc->pub) &&
							wlc_proxd_frame(wlc, parms.pkttag)))
#endif
							rspec |= RSPEC_LDPC_CODING;
				}

				/* STBC */
				if ((wlc_ratespec_nsts(rspec) == 1)) {
					if (stbc_tx_forced ||
					  ((RSPEC_ISHT(rspec) && stbc_ht_scb_auto) ||
					   (RSPEC_ISVHT(rspec) && stbc_vht_scb_auto))) {
						_scb_stbc_on = TRUE;
					}

					if (_scb_stbc_on && !rspec_legacy)
#ifdef WL_PROXDETECT
						if (PROXD_ENAB(wlc->pub) &&
							!wlc_proxd_frame(wlc, parms.pkttag))
#endif
							rspec |= RSPEC_STBC;
				}
			}

			/* Determine the HT frame format, HT_MM or HT_GF */
			if (RSPEC_ISHT(rspec)) {
				int nsts;

				mimo_preamble_type = wlc_prot_n_preamble(wlc, scb);
				if (RSPEC_ISHT(rspec)) {
					nsts = wlc_ratespec_nsts(rspec);
					preamble_type = mimo_preamble_type;
					if (n_prot == WLC_N_PROTECTION_20IN40 &&
						RSPEC_IS40MHZ(rspec))
						use_cts = TRUE;

					/* if the mcs is multi stream check if it needs
					 * an rts
					 */
					if (nsts > 1) {
						if (WLC_HT_GET_SCB_MIMOPS_ENAB(hti, scb) &&
							WLC_HT_GET_SCB_MIMOPS_RTS_ENAB(hti, scb))
							use_rts = use_mimops_rts = TRUE;
					}

					/* if SGI is selected, then forced mm for single
					 * stream
					 * (spec section 9.16, Short GI operation)
					 */
					if (RSPEC_ISSGI(rspec) && nsts == 1) {
						preamble_type = WLC_MM_PREAMBLE;
					}
				}
			} else {
				/* VHT always uses MM preamble */
				if (RSPEC_ISVHT(rspec)) {
					preamble_type = WLC_MM_PREAMBLE;
				}
			}
		}

		if (wlc->pub->_dynbw == TRUE) {
			fbw = wlc_tx_dynbw_fbw(wlc, scb, rspec);
		}
#endif /* WL11N */

		/* - FBW_BW_20MHZ to make it 0-index based */
		rate_hdr->FbwInfo = (((fbw - FBW_BW_20MHZ) & FBW_BW_MASK) << FBW_BW_SHIFT);


		/* (2) PROTECTION, may change rspec */
		if (((parms.type == FC_TYPE_DATA) || (parms.type == FC_TYPE_MNG)) &&
			((parms.phylen > wlc->RTSThresh) || ((parms.pkttag)->flags & WLF_USERTS)) &&
			!ETHER_ISMULTI(&parms.h->a1))
			use_rts = TRUE;

		if ((wlc->band->gmode && g_prot && RSPEC_ISOFDM(rspec)) ||
			((RSPEC_ISHT(rspec) || RSPEC_ISVHT(rspec)) && n_prot)) {
			if (nfrags > 1) {
				/* For a frag burst use the lower modulation rates for the
				 * entire frag burst instead of protection mechanisms.
				 * As per spec, if protection mechanism is being used, a
				 * fragment sequence may only employ ERP-OFDM modulation for
				 * the final fragment and control response. (802.11g Sec 9.10)
				 * For ease we send the *whole* sequence at the
				 * lower modulation instead of using a higher modulation for the
				 * last frag.
				 */

				/* downgrade the rate to CCK or OFDM */
				if (g_prot) {
					/* Use 11 Mbps as the rate and fallback. We should make
					 * sure that if we are downgrading an OFDM rate to CCK,
					 * we should pick a more robust rate.  6 and 9 Mbps are not
					 * usually selected by rate selection, but even if the OFDM
					 * rate we are downgrading is 6 or 9 Mbps, 11 Mbps is more
					 * robust.
					 */
					rspec = CCK_RSPEC(WLC_RATE_FRAG_G_PROTECT);
				} else {
					/* Use 24 Mbps as the rate and fallback for what would have
					 * been a MIMO rate. 24 Mbps is the highest phy mandatory
					 * rate for OFDM.
					 */
					rspec = OFDM_RSPEC(WLC_RATE_FRAG_N_PROTECT);
				}
				parms.pkttag->flags &= ~WLF_RATE_AUTO;
			} else {
				/* Use protection mechanisms on unfragmented frames */
				/* If this is a 11g protection, then use CTS-to-self */
				if (wlc->band->gmode && g_prot && !RSPEC_ISCCK(rspec))
					use_cts = TRUE;
			}
		}

		/* calculate minimum rate */
		ASSERT(VALID_RATE_DBG(wlc, rspec));

		if (RSPEC_ISCCK(rspec)) {
			if (short_preamble &&
				!((RSPEC2RATE(rspec) == WLC_RATE_1M) ||
				(scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG)))
				preamble_type = WLC_SHORT_PREAMBLE;
			else
				preamble_type = WLC_LONG_PREAMBLE;
		}

		ASSERT(RSPEC_ISLEGACY(rspec) || WLC_IS_MIMO_PREAMBLE(preamble_type));

#if defined(WL_BEAMFORMING)
		if (TXBF_ENAB(wlc->pub)) {
			/* bfe_sts_cap = 3: foursteams, 2: three streams, 1: two streams */
			bfe_sts_cap = wlc_txbf_get_bfe_sts_cap(wlc->txbf, scb);
			if (bfe_sts_cap && (D11REV_LE(wlc->pub->corerev, 64) ||
				!wlc_txbf_bfrspexp_enable(wlc->txbf))) {
				/* Explicit TxBF: Number of txbf chains
				 * is min(#active txchains, #bfe sts + 1)
				 */
				txbf_chains = MIN((uint8)WLC_BITSCNT(wlc->stf->txchain),
						(bfe_sts_cap + 1));
			} else {
				/* bfe_sts_cap=0 indicates there is no Explicit TxBf link to this
				 * peer, and driver will probably use Implicit TxBF. Ignore the
				 * spatial_expension policy, and always use all currently enabled
				 * txcores
				 */
				txbf_chains = (uint8)WLC_BITSCNT(wlc->stf->txchain);
			}
		}
#endif /* WL_BEAMFORMING */
		/* get txpwr for bw204080 and txbf on/off */
		if (wlc_stf_get_204080_pwrs(wlc, rspec, &txpwrs, txbf_chains) != BCME_OK) {
			ASSERT(!"phyctl1 ppr returns error!");
		}

		txpwr_bfsel = 0;
#if defined(WL_BEAMFORMING)
		if (TXBF_ENAB(wlc->pub) &&
			(wlc->allow_txbf) &&
			(preamble_type != WLC_GF_PREAMBLE) &&
			!SCB_ISMULTI(scb) &&
			(parms.type == FC_TYPE_DATA) && !mutx_en) {
			ratespec_t fbw_rspec;
			uint16 txpwr_mask, stbc_val;
			fbw_rspec = rspec;
			bfen = wlc_txbf_sel(wlc->txbf, rspec, scb, &bf_shm_index, &txpwrs);

			if (bfen) {
				/* BFM0: Provide alternative tx info if phyctl has bit 3
				 * (BFM) bit
				 * set but ucode has to tx with BFM cleared.
				 */
				if (D11AC2_PHY_SUPPORT(wlc)) {
					/* acphy2 mac/phy interface */
					txpwr_mask = D11AC2_BFM0_TXPWR_MASK;
					stbc_val = D11AC2_BFM0_STBC;
				} else {
					txpwr_mask = BFM0_TXPWR_MASK;
					stbc_val = BFM0_STBC;
				}
				rate_hdr->Bfm0 =
					((uint16)(txpwrs.pbw[((rspec & RSPEC_BW_MASK) >>
					RSPEC_BW_SHIFT) - BW_20MHZ][TXBF_OFF_IDX])) & txpwr_mask;
				rate_hdr->Bfm0 |=
					(uint16)(RSPEC_ISSTBC(rspec) ? stbc_val : 0);
			}

			txpwr_bfsel = bfen ? 1 : 0;

			if (fbw != FBW_BW_INVALID) {
				fbw_rspec = (fbw_rspec & ~RSPEC_BW_MASK) |
				((fbw == FBW_BW_40MHZ) ?  RSPEC_BW_40MHZ : RSPEC_BW_20MHZ);
				if (!RSPEC_ISSTBC(fbw_rspec))
					fbw_bfen = wlc_txbf_sel(wlc->txbf, fbw_rspec, scb,
						&bf_shm_index, &txpwrs);
				else
					fbw_bfen = bfen;

				rate_hdr->FbwInfo |= (uint16)(fbw_bfen ? FBW_TXBF : 0);
			}

			if (RSPEC_ACTIVE(wlc->band->rspec_override)) {
				wlc_txbf_applied2ovr_upd(wlc->txbf, bfen);
			}
		}
#endif /* WL_BEAMFORMING */

		/* (3) PLCP: determine PLCP header and MAC duration, fill d11txh_t */
		wlc_compute_plcp(wlc, bsscfg, rspec, parms.phylen, parms.fc, plcp);

		/* RateInfo.TxRate */
		txrate = wlc_rate_rspec2rate(rspec);
		rate_hdr->TxRate = htol16(txrate/500);


		/* RIFS(testing only): based on frameburst, non-CCK frames only */
		if (SCB_HT_CAP(scb) && WLC_HT_GET_FRAMEBURST(hti) &&
			WLC_HT_GET_RIFS(hti) &&
			n_prot != WLC_N_PROTECTION_MIXEDMODE && !RSPEC_ISCCK(rspec) &&
			!ETHER_ISMULTI(&parms.h->a1) && ((parms.fc & FC_KIND_MASK) ==
			FC_QOS_DATA) && (queue < TX_BCMC_FIFO)) {
			uint16 qos_field, *pqos;

			WLPKTTAG(p)->flags |= WLF_RIFS;
			mcl |= (D11AC_TXC_MBURST | D11AC_TXC_URIFS);
			use_rifs = TRUE;

			/* RIFS implies QoS frame with no-ack policy, hack the QoS field */
			pqos = (uint16 *)((uchar *)parms.h + (parms.a4 ?
				DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
			qos_field = ltoh16(*pqos) & ~QOS_ACK_MASK;
			qos_field |= (QOS_ACK_NO_ACK << QOS_ACK_SHIFT) & QOS_ACK_MASK;
			*pqos = htol16(qos_field);
		}

#ifdef WLAMPDU
		/* mark pkt as aggregable if it is */
		if ((k == 0) && WLPKTFLAG_AMPDU(parms.pkttag) && !RSPEC_ISLEGACY(rspec)) {
			if (WLC_KEY_ALLOWS_AMPDU(key_info)) {
				WLPKTTAG(p)->flags |= WLF_MIMO;
				if (WLC_HT_GET_AMPDU_RTS(wlc->hti))
					use_rts = TRUE;
			}
		}
#endif /* WLAMPDU */

		if (parms.type == FC_TYPE_DATA && (queue < TX_BCMC_FIFO)) {
			if ((WLC_HT_GET_FRAMEBURST(hti) &&
				(txrate > WLC_FRAMEBURST_MIN_RATE) &&
#ifdef WLAMPDU
				(wlc_ampdu_frameburst_override(wlc->ampdu_tx, scb) == FALSE) &&
#endif
				(!wme->edcf_txop[ac] || WLPKTFLAG_AMPDU(parms.pkttag))) ||
				FALSE)
#ifdef WL11N
				/* dont allow bursting if rts is required around each mimo frame */
				if (use_mimops_rts == FALSE)
#endif
					mcl |= D11AC_TXC_MBURST;
		}

		if (RSPEC_ISVHT(rspec)) {
			mch |= D11AC_TXC_SVHT;
		}

#if defined(WL_PROT_OBSS) && !defined(WL_PROT_OBSS_DISABLED)
		/* If OBSS protection is enabled, set CTS/RTS accordingly. */
		if (WLC_PROT_OBSS_ENAB(wlc->pub)) {
			if (WLC_PROT_OBSS_PROTECTION(wlc->prot_obss) && !use_rts && !use_cts) {
				if (ETHER_ISMULTI(&parms.h->a1)) {
					/* Multicast and broadcast pkts need CTS protection */
					use_cts = TRUE;
				} else {
					/* Unicast pkts < 80 bw need RTS protection */
					if (RSPEC_BW(rspec) < phybw) {
						use_rts = TRUE;
					}
				}
			}
		}
#endif /* WL_PROT_OBSS && !WL_PROT_OBSS_DISABLED */

		/* (5) RTS/CTS: determine RTS/CTS PLCP header and MAC duration, furnish d11txh_t */
		wlc_d11ac_hdrs_rts_cts(scb, bsscfg, rspec, use_rts, use_cts, &rts_rspec, rate_hdr,
		                       &rts_preamble_type, &mcl);

#ifdef WL_BEAMFORMING
		if (TXBF_ENAB(wlc->pub)) {
			if (bfen) {
				if (bf_shm_index != BF_SHM_IDX_INV) {
					rate_hdr->RtsCtsControl |= htol16((bf_shm_index <<
						D11AC_RTSCTS_BF_IDX_SHIFT));
				} else {
					rate_hdr->RtsCtsControl |= htol16(D11AC_RTSCTS_IMBF);
				}
			} else if (mutx_on && (k == 0)) {
				rate_hdr->RtsCtsControl |= htol16((bf_shmx_idx <<
						D11AC_RTSCTS_BF_IDX_SHIFT));
			}
			/* Move wlc_txbf_fix_rspec_plcp() here to remove RSPEC_STBC
			 * from rspec before wlc_acphy_txctl0_calc_ex().
			 * rate_hdr->PhyTxControlWord_0 will not set D11AC2_PHY_TXC_STBC
			 * if this rate enable txbf.
			 */
			if (bfen || mutx_on)
				wlc_txbf_fix_rspec_plcp(wlc->txbf, &rspec, plcp, txbf_chains);
		}
#endif /* WL_BEAMFORMING */

		/* PhyTxControlWord_0 */
		phyctl = wlc_acphy_txctl0_calc(wlc, rspec, preamble_type);
		WLC_PHYCTLBW_OVERRIDE(phyctl, D11AC_PHY_TXC_BW_MASK, phyctl_bwo);
#ifdef WL_PROXDETECT
		if (PROXD_ENAB(wlc->pub) && wlc_proxd_frame(wlc, parms.pkttag)) {
			/* TOF measurement pkts use only primary antenna to tx */
			wlc_proxd_tx_conf(wlc, &phyctl, &mch, parms.pkttag);
		}
#endif
#ifdef WL_BEAMFORMING
		if (TXBF_ENAB(wlc->pub) && (bfen) && !mutx_en) {
			phyctl |= (D11AC_PHY_TXC_BFM);
#if !defined(WLTXBF_DISABLED)
		} else {
			if (wlc_txbf_bfmspexp_enable(wlc->txbf) &&
			(preamble_type != WLC_GF_PREAMBLE) &&
			(!RSPEC_ISSTBC(rspec)) && IS_MCS(rspec)) {

				uint8 nss = (uint8) wlc_ratespec_nss(rspec);
				uint8 ntx = (uint8) wlc_stf_txchain_get(wlc, rspec);

				if (nss == 3 && ntx == 4) {
					txbf_uidx = 96;
				} else if (nss == 2 && ntx == 4) {
					txbf_uidx = 97;
				} else if (nss == 2 && ntx == 3) {
					txbf_uidx = 98;
				}
				/* no need to set BFM (phytxctl.B3) as ucode will do it */
			}
#endif /* !defined(WLTXBF_DISABLED) */
		}
#endif /* WL_BEAMFORMING */
		rate_hdr->PhyTxControlWord_0 = htol16(phyctl);

		/* PhyTxControlWord_1 */
		txpwr = txpwrs.pbw[((rspec & RSPEC_BW_MASK) >> RSPEC_BW_SHIFT) -
			BW_20MHZ][txpwr_bfsel];
#ifdef WL_LPC
		/* Apply the power index */
		if ((parms.pkttag->flags & WLF_RATE_AUTO) && LPC_ENAB(wlc) && k == 0) {
			phyctl = wlc_acphy_txctl1_calc(wlc, rspec, lpc_offset, txpwr, FALSE);
			/* Preserve the PhyCtl for later (dotxstatus) */
			wlc_scb_lpc_store_pcw(wlc->wlpci, scb, ac, phyctl);
		} else
#endif
		{
			phyctl = wlc_acphy_txctl1_calc(wlc, rspec, 0, txpwr, FALSE);
		}

		/* for fallback bw */
		if (fbw != FBW_BW_INVALID) {
			txpwr = txpwrs.pbw[fbw - FBW_BW_20MHZ][TXBF_OFF_IDX];
			rate_hdr->FbwInfo |= ((uint16)txpwr << FBW_BFM0_TXPWR_SHIFT);
#ifdef WL_BEAMFORMING
			if (fbw_bfen) {
				txpwr = txpwrs.pbw[fbw - FBW_BW_20MHZ][TXBF_ON_IDX];
				rate_hdr->FbwInfo |= ((uint16)txpwr << FBW_BFM_TXPWR_SHIFT);
			}
#endif
			rate_hdr->FbwInfo = htol16(rate_hdr->FbwInfo);
		}

		WLC_PHYCTLBW_OVERRIDE(phyctl, D11AC_PHY_TXC_PRIM_SUBBAND_MASK, phyctl_sbwo);
#ifdef WL_PROXDETECT
		if (PROXD_ENAB(wlc->pub) && wlc_proxd_frame(wlc, parms.pkttag)) {
			/* TOF measurement pkts use initiator's subchannel to tx */
			wlc_proxd_tx_conf_subband(wlc, &phyctl, parms.pkttag);
		}
#endif
		rate_hdr->PhyTxControlWord_1 = htol16(phyctl);

		/* PhyTxControlWord_2 */
		phyctl = wlc_acphy_txctl2_calc(wlc, rspec, txbf_uidx);
		rate_hdr->PhyTxControlWord_2 = htol16(phyctl);

		/* Avoid changing TXOP threshold based on multicast packets */
		if ((k == 0) && SCB_WME(scb) && parms.qos && wme->edcf_txop[ac] &&
			!ETHER_ISMULTI(&parms.h->a1) && !(parms.pkttag->flags & WLF_8021X)) {
			uint frag_dur, dur;

			/* WME: Update TXOP threshold */
			if ((!WLPKTFLAG_AMPDU(parms.pkttag)) && (frag == 0)) {
				int16 delta;

				frag_dur = wlc_calc_frame_time(wlc, rspec, preamble_type,
					parms.phylen);

				if (rts) {
					/* 1 RTS or CTS-to-self frame */
					dur = wlc_calc_cts_time(wlc, rts_rspec, rts_preamble_type);
					/* (SIFS + CTS) + SIFS + frame + SIFS + ACK */
					dur += ltoh16(rts->durid);
				} else if (use_rifs) {
					dur = frag_dur;
				} else {
					/* frame + SIFS + ACK */
					dur = frag_dur;
					dur += wlc_compute_frame_dur(wlc,
						CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
						rspec, preamble_type, 0);
				}

				/* update txop byte threshold (txop minus intraframe overhead) */
				delta = (int16)(wme->edcf_txop[ac] - (dur - frag_dur));
				if (delta >= 0) {
#ifdef WLAMSDU_TX
					if (WLPKTFLAG_AMSDU(parms.pkttag) &&
						(queue == TX_AC_BE_FIFO)) {
						WL_ERROR(("edcf_txop changed, update AMSDU\n"));
						wlc_amsdu_txop_upd(wlc->ami);
					} else
#endif
					{
						if (!((parms.pkttag)->flags & WLF_8021X)) {
							uint newfragthresh =
								wlc_calc_frame_len(wlc, rspec,
								preamble_type, (uint16)delta);
							/* range bound the fragthreshold */
							newfragthresh = MAX(newfragthresh,
								DOT11_MIN_FRAG_LEN);
							newfragthresh = MIN(newfragthresh,
								wlc->usr_fragthresh);
							/* update the fragthr and do txc update */
							if (wlc->fragthresh[ac] !=
								(uint16)newfragthresh) {
									wlc_fragthresh_set(wlc,
									ac,
									(uint16)newfragthresh);
							}

						}
					}
				} else {
					WL_ERROR(("wl%d: %s txop invalid for rate %d\n",
						wlc->pub->unit, fifo_names[queue],
						RSPEC2RATE(rspec)));
				}

				if (CAC_ENAB(wlc->pub) &&
					queue <= TX_AC_VO_FIFO) {
					/* update cac used time */
					if (wlc_cac_update_used_time(wlc->cac, ac, dur, scb))
						WL_ERROR(("wl%d: ac %d: txop exceeded allocated TS"
							"time\n", wlc->pub->unit, ac));
				}

				if (dur > wme->edcf_txop[ac])
					WL_ERROR(("wl%d: %s txop exceeded phylen %d/%d dur %d/%d\n",
						wlc->pub->unit, fifo_names[queue], parms.phylen,
						wlc->fragthresh[ac], dur, wme->edcf_txop[ac]));
			}
		} else if ((k == 0) && SCB_WME(scb) &&
			parms.qos && CAC_ENAB(wlc->pub) &&
			queue <= TX_AC_VO_FIFO) {
			uint dur;
			if (rts) {
				/* 1 RTS or CTS-to-self frame */
				dur = wlc_calc_cts_time(wlc, rts_rspec, rts_preamble_type);
				/* (SIFS + CTS) + SIFS + frame + SIFS + ACK */
				dur += ltoh16(rts->durid);
			} else {
				/* frame + SIFS + ACK */
				dur = wlc_calc_frame_time(wlc, rspec, preamble_type,
					parms.phylen);
				dur += wlc_compute_frame_dur(wlc,
					CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
					rspec, preamble_type, 0);
			}
			/* update cac used time */
			if (wlc_cac_update_used_time(wlc->cac, ac, dur, scb))
				WL_ERROR(("wl%d: ac %d: txop exceeded allocated TS time\n",
					wlc->pub->unit, ac));
		}
		/* Store the final rspec back into the cur_rate */
		cur_rate.rspec[k] = rspec;
	} /* rate loop ends */

	/* Mark last rate */
	rate_blk[cur_rate.num-1].RtsCtsControl |= htol16(D11AC_RTSCTS_LAST_RATE);
#ifdef WL_BEAMFORMING
	if (mutx_on) {
		mch |= D11AC_TXC_MU;
#ifdef WL_MUPKTENG
		if (mutx_pkteng_on) {
			mcl |=  D11AC_TXC_AMPDU;
			mch &= ~D11AC_TXC_SVHT;
		}
#endif
	}
#endif /* WL_BEAMFORMING */

	/* record rate history here (pick final primary rspec) */
	rspec = cur_rate.rspec[0];
	WLCNTSCB_COND_SET(((parms.type == FC_TYPE_DATA) &&
		((FC_SUBTYPE(parms.fc) != FC_SUBTYPE_NULL) &&
			(FC_SUBTYPE(parms.fc) != FC_SUBTYPE_QOS_NULL))),
			scb->scb_stats.tx_rate, rspec);

	/* record rate history after the txbw is valid */
	if (rspec_history) {
		/* update per bsscfg tx rate */
		bsscfg->txrspec[bsscfg->txrspecidx][0] = rspec;
		bsscfg->txrspec[bsscfg->txrspecidx][1] = (uint8) nfrags;
		bsscfg->txrspecidx = (bsscfg->txrspecidx+1) % NTXRATE;

		WLCNTSCBSET(scb->scb_stats.tx_rate_fallback, cur_rate.rspec[cur_rate.num - 1]);
	}

#ifdef WLFCTS
	if (WLFCTS_ENAB(wlc->pub))
		WLPKTTAG(p)->rspec = rspec;
#endif

	D11PktInfo.PktInfo = &txh->PktInfo;

	/* Compute and fill "per packet info" / short TX Desc (24 bytes) */
	wlc_fill_txd_per_pkt_info(wlc, p, scb, &D11PktInfo,
		mcl, mch, &parms, frameid, D11AC_TXH_LEN, 0);

#ifdef BCM_SFD
	if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(osh, p)) {
		WLC_SFD_SET_SFDID(txh->PktInfo.MacTxControlLow, sfd_id);

		d11_cache_info.d11actxh_CacheInfo =
			wlc_sfd_get_cache_info(wlc->sfd, txh);
	} else
#endif
	{
		d11_cache_info.d11actxh_CacheInfo =
			WLC_TXD_CACHE_INFO_GET(txh, wlc->pub->corerev);
	}

	/* Compute and fill "per cache info" for TX Desc (24 bytes) */
	wlc_fill_txd_per_cache_info(wlc, bsscfg, scb, p, parms.pkttag,
		d11_cache_info, mutx_pkteng_on);

	/* With d11 hdrs on, mark the packet as MPDU with txhdr */
	WLPKTTAG(p)->flags |= (WLF_MPDU | WLF_TXHDR);

#ifdef WLTOEHW
	/* add tso-oe header, if capable and toe module is not bypassed */
	if (wlc->toe_capable && !wlc->toe_bypass)
		wlc_toe_add_hdr(wlc, p, scb, key_info, nfrags, txh_off);
#endif

	return (frameid);
} /* wlc_d11hdrs_rev40 */


#ifdef WL11AX
/**
 * If the caller decided that an rts or cts frame needs to be transmitted, the transmit properties
 * of the rts/cts have to be determined and communicated to the transmit hardware. RTS/CTS rates are
 * always legacy rates (HR/DSSS, ERP, or OFDM).
 */
static INLINE void
wlc_d11hdrs_rev80_rts_cts(struct scb *scb /* [in] */, wlc_bsscfg_t *bsscfg /* [in] */,
	ratespec_t rspec, bool use_rts, bool use_cts,
	ratespec_t *rts_rspec /* [out] */, d11txh_rev80_rate_fixed_t *rate_fixed /* [in/out] */,
	uint8 *rts_preamble_type /* [out] */, uint16 *mcl /* [in/out] MacControlLow */)
{
	*rts_preamble_type = WLC_LONG_PREAMBLE;
	/* RTS PLCP header and RTS frame */
	if (use_rts || use_cts) {
		uint16 phy_rate;
		uint8 rts_rate;

		if (use_rts && use_cts)
			use_cts = FALSE;

		*rts_rspec = wlc_rspec_to_rts_rspec(bsscfg, rspec, FALSE);
		ASSERT(RSPEC_ISLEGACY(*rts_rspec));

		/* extract the MAC rate in 0.5Mbps units */
		rts_rate = (*rts_rspec & RSPEC_RATE_MASK);

		rate_fixed->RtsCtsControl = RSPEC_ISCCK(*rts_rspec) ?
			htol16(D11AC_RTSCTS_FRM_TYPE_11B) :
			htol16(D11AC_RTSCTS_FRM_TYPE_11AG);

		if (!RSPEC_ISOFDM(*rts_rspec) &&
			!((rts_rate == WLC_RATE_1M) ||
			(scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG))) {
			*rts_preamble_type = WLC_SHORT_PREAMBLE;
			rate_fixed->RtsCtsControl |= htol16(D11AC_RTSCTS_SHORT_PREAMBLE);
		}

		/* set RTS/CTS flag */
		if (use_cts) {
			rate_fixed->RtsCtsControl |= htol16(D11AC_RTSCTS_USE_CTS);
		} else {
			rate_fixed->RtsCtsControl |= htol16(D11AC_RTSCTS_USE_RTS);
			*mcl |= D11AC_TXC_LFRM;
		}

		/* RTS/CTS Rate index - Bits 3-0 of plcp byte0	*/
		phy_rate = rate_info[rts_rate] & 0xf;
		rate_fixed->RtsCtsControl |= htol16((phy_rate << 8));
	} else {
		rate_fixed->RtsCtsControl = 0;
	}
} /* wlc_d11hdrs_rev80_rts_cts */

static INLINE uint8
wlc_axphy_calc_per_core_power_offs(wlc_info_t *wlc, ratespec_t rspec,
	uint8 core)
{
	/* Todo : Need to compute individual power offsets (typically per-PHY based) */
	return 0;
}

/**
 * Function to fill byte 11,12 of TX Ctl Word :
 * - bits[0-4] Number of power offsets
 */
static INLINE void
wlc_axphy_txctl_calc_pwr_offs(wlc_info_t *wlc, ratespec_t rspec,
	wlc_d11txh_rev80_rate_t *rate_hdr, uint8 n_pwr_offsets)
{
	uint8 i;
	for (i = 0; i < n_pwr_offsets; i++) {
		rate_hdr->PhyTxControlByte[i + D11_REV80_PHY_TXC_PWROFS0_BYTE_POS] =
			wlc_axphy_calc_per_core_power_offs(wlc, rspec, i);
	}
}

/**
 * Function to fill word 2 of TX Ctl Word :
 * - bits[0-8] Core Mask (8 bits for upto 8 core design)
 * - bits[0-8] Antennae Configuration
 */
static INLINE uint16
wlc_axphy_fixed_txctl_calc_word_2(wlc_info_t *wlc, ratespec_t rspec)
{
	uint16 phytxant = 0;

	phytxant = wlc_stf_d11hdrs_phyctl_txant(wlc, rspec);

	phytxant >>= PHY_TXC_ANT_SHIFT;
	phytxant &= D11_REV80_PHY_TXC_ANT_CORE_MASK;

	return phytxant;
}

/**
 * Function to fill word 1 (bytes 2-3) of TX Ctl Word :
 * 1. Bits [0-1] Packet BW
 * 2. Bits [2-4] Sub band location
 * 3. Bits [5-12] Partial Ofdma Sub band (not to be filled by SW)
 * 4. Bit [13] DynBW present
 * 5. Bit [14] DynBw Mode
 * 6. Bit [15] MU
 */
static INLINE uint16
wlc_axphy_fixed_txctl_calc_word_1(wlc_info_t *wlc, ratespec_t rspec, bool is_mu)
{
	uint16 bw, sb;
	uint16 phyctl = 0;

	/* bits [0:1] - 00/01/10/11 => 20/40/80/160 */
	switch (RSPEC_BW(rspec)) {
		case RSPEC_BW_20MHZ:
			bw = D11_REV80_PHY_TXC_BW_20MHZ;
		break;
		case RSPEC_BW_40MHZ:
			bw = D11_REV80_PHY_TXC_BW_40MHZ;
		break;
		case RSPEC_BW_80MHZ:
			bw = D11_REV80_PHY_TXC_BW_80MHZ;
		break;
		case RSPEC_BW_160MHZ:
			bw = D11_REV80_PHY_TXC_BW_160MHZ;
		break;
		default:
			ASSERT(0);
			bw = D11_REV80_PHY_TXC_BW_20MHZ;
		break;
	}
	phyctl |= bw;

	/**
	 * bits [2:4] Sub band location
	 * Primary Subband Location: b 2-4
	 * LLL ==> 000
	 * LLU ==> 001
	 * ...
	 * UUU ==> 111
	 */
	sb = ((wlc->chanspec & WL_CHANSPEC_CTL_SB_MASK) >> WL_CHANSPEC_CTL_SB_SHIFT);
	phyctl |= ((sb >> ((RSPEC_BW(rspec) >> RSPEC_BW_SHIFT) - 1)) << D11_REV80_PHY_TXC_SB_SHIFT);

	/* Note : Leave DynBw present and DynBw mode fields as 0 */
	if (is_mu) {
		phyctl |= D11_REV80_PHY_TXC_MU;
	}

	return phyctl;
}

/**
 * Function to fill Byte 1 of TX Ctl Word :
 * 1. Bits [0-7] MCS +NSS
 * 2. Bit [8] STBC
 */
static INLINE uint8
wlc_axphy_fixed_txctl_calc_byte_1(wlc_info_t *wlc, ratespec_t rspec)
{
	uint nss = wlc_ratespec_nss(rspec);
	uint mcs;
	uint8 rate, phyctl_byte;
	const wlc_rateset_t* cur_rates = NULL;
	uint8 rindex;

	/* mcs+nss: bits [0-7] of PhyCtlWord Byte 3  */
	if (RSPEC_ISHT(rspec)) {
		/* for 11n: B[0:5] for mcs[0:32] */
		if (WLPROPRIETARY_11N_RATES_ENAB(wlc->pub)) {
			mcs = rspec & D11_REV80_PHY_TXC_11N_MCS_MASK;
		} else {
			mcs = rspec & RSPEC_RATE_MASK;
		}
		phyctl_byte = (uint8)mcs;
	} else if (RSPEC_ISVHT(rspec)) {
		/* for 11ac: B[0:3] for mcs[0-9], B[4:5] for n_ss[1-4]
			(0 = 1ss, 1 = 2ss, 2 = 3ss, 3 = 4ss)
		*/
		mcs = rspec & RSPEC_VHT_MCS_MASK;
		ASSERT(mcs <= WLC_MAX_VHT_MCS);
		phyctl_byte = (uint8)mcs;
		ASSERT(nss <= 8);
		phyctl_byte |= ((nss-1) << D11_REV80_PHY_TXC_11AC_NSS_SHIFT);
	} else {
		rate = RSPEC2RATE(rspec);

		if (RSPEC_ISOFDM(rspec)) {
			cur_rates = &ofdm_rates;
		} else {
			/* for 11b: B[0:1] represents phyrate
				(0 = 1mbps, 1 = 2mbps, 2 = 5.5mbps, 3 = 11mbps)
			*/
			cur_rates = &cck_rates;
		}
		for (rindex = 0; rindex < cur_rates->count; rindex++) {
			if ((cur_rates->rates[rindex] & RATE_MASK) == rate) {
				break;
			}
		}
		ASSERT(rindex < cur_rates->count);
		phyctl_byte = rindex;
	}
	if (D11AC2_PHY_SUPPORT(wlc)) {
		if (WLPROPRIETARY_11N_RATES_ENAB(wlc->pub)) {
			if (RSPEC_ISHT(rspec) && (rspec & RSPEC_RATE_MASK)
				>= WLC_11N_FIRST_PROP_MCS) {
				phyctl_byte |= D11_REV80_PHY_TXC_11N_PROP_MCS;
			}
		}
	}

	/* STBC : Bit [8] of PhyCtlWord Byte 3 */
	if (D11AC2_PHY_SUPPORT(wlc)) {
		/* STBC */
		if (RSPEC_ISSTBC(rspec)) {
			/* set 8th bit */
			phyctl_byte |= D11_REV80_PHY_TXC_STBC;
		}
	}

	return phyctl_byte;
}
/**
 * Function to compute and fill the
 * variable size portion of txCtrlWord
 * (depends on n_rus and #power offsets)
 */
static void
wlc_axphy_var_txctl_calc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	wlc_d11txh_rev80_rate_t *rate_hdr, ratespec_t rspec, uint8 n_ru,
	uint8 power_offset, bool bfen)
{
	BCM_REFERENCE(n_ru);
	BCM_REFERENCE(bsscfg);
	BCM_REFERENCE(bfen);

	/* Todo : Compute and fill phytxctl based upon #power offsets and n_rus */

	/* ---- Compute and fill power offset fields (based on power_offset) ---- */
	wlc_axphy_txctl_calc_pwr_offs(wlc, rspec, rate_hdr, power_offset);

	/* [5+n_pwr+1] Compute and fill BFM field (min n_ru is 1 for STA) */
#ifdef WL_BEAMFORMING
	if (bfen) {
		/* Note : Leave Txbf field as 0 for now */
		rate_hdr->PhyTxControlByte[D11_REV80_PHY_TXC_PWROFS0_BYTE_POS + power_offset] |=
			(D11_REV80_PHY_TXC_BFM);
	}
#endif /* WL_BEAMFORMING */

}

/**
 * Function to compute and fill TXCTL_EXT (depends on n_users)
 */
static void
wlc_axphy_txctl_ext_calc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	wlc_d11txh_rev80_rate_t *rate_hdr, ratespec_t rspec, uint8 n_user)
{
	/* Todo : Compute and fill phytxctl based upon n_users */
}

/**
 * Function to compute and fill common / fixed portion
 * of PhyTxControl (9 Phytxcontrol bytes).
 */
static void
wlc_axphy_fixed_txctl_calc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	wlc_d11txh_rev80_rate_t *rate_hdr, ratespec_t rspec, uint8 preamble_type)
{
	uint16 phyctl_word;

	/* ---- [0] Compute and fill PhyCtlWord Byte Pos (offset from length field) 0  ---- */
	/* Compute Frame Format */
	rate_hdr->PhyTxControlByte[0] = PHY_TXC_FRAMETYPE(rspec);

	/* HE Format field (bits 3,4) not to be populated by SW.
	 *
	 * HEControl field in the short TXD shall be used by the ucode,
	 * for determining the method of TX for the given packet.
	 */

	/* TODO: following fields need to be computed and filled:
	 * D11_REV80_PHY_TXC_NON_SOUNDING - bit 2 - is this sounding frame or not?
	 * hard-code to NOT
	 */
	rate_hdr->PhyTxControlByte[0] |= (D11_REV80_PHY_TXC_NON_SOUNDING);

	if ((preamble_type == WLC_SHORT_PREAMBLE) ||
	    (preamble_type == WLC_GF_PREAMBLE)) {
		ASSERT((preamble_type == WLC_GF_PREAMBLE) || !RSPEC_ISHT(rspec));
		rate_hdr->PhyTxControlByte[0] |= D11_REV80_PHY_TXC_SHORT_PREAMBLE;
		WLCNTINCR(wlc->pub->_cnt->txprshort);
	}

	/* ---- [1] Compute and fill PhyCtlWord Byte Pos (offset from length field) 1  ---- */
	rate_hdr->PhyTxControlByte[1] = wlc_axphy_fixed_txctl_calc_byte_1(wlc, rspec);

	/* ---- [2, 3] Compute and fill PhyCtlWord Byte Pos (offset from length field) 2,3 ---- */
	phyctl_word = wlc_axphy_fixed_txctl_calc_word_1(wlc, rspec, FALSE);
	phyctl_word = htol16(phyctl_word);
	rate_hdr->PhyTxControlByte[2] = ((phyctl_word & 0xff00) >> 8);
	rate_hdr->PhyTxControlByte[3] = (phyctl_word & 0x00ff);

	/* ---- [4,5] Compute and fill PhyCtlWord Byte Pos (offset from length field) 4,5 ---- */
	phyctl_word = wlc_axphy_fixed_txctl_calc_word_2(wlc, rspec);

	rate_hdr->PhyTxControlByte[4] = (phyctl_word & D11_REV80_PHY_TXC_CORE_MASK);
	rate_hdr->PhyTxControlByte[5] = (phyctl_word & D11_REV80_PHY_TXC_ANT_MASK) >>
		D11_REV80_PHY_TXC_ANT_SHIFT;
}

/**
 * Calculate required size of Phytxctrl based upon N-Users, N-RUs and #Power_offset
 * (Return size in bytes).
 */
static INLINE uint16
wlc_compute_phytxctl_size(uint8 n_users, uint8 n_rus, uint8 n_pwr_offsets)
{
	return ((D11_REV80_TXH_TXC_RU_FIELD_SIZE(n_rus)) +
		(D11_REV80_TXH_TXC_N_USERs_FIELD_SIZE(n_users)) +
		(n_pwr_offsets * D11_REV80_TXH_TXC_PWR_OFFSET_SIZE) +
		D11_REV80_TXH_TXC_CONST_FIELDS_SIZE);
}

#ifdef WL11AX
static INLINE uint16
wlc_apply_he_tx_policy(wlc_info_t *wlc, struct scb *scb, uint16 type)
{
/*
 * Data - For HE, send all data packets via triggered queues by default
 * MGMT - Pre-assoc mgmt is sent as EDCA
 */
	if (SCB_HE_TRIG_TX(scb)) {
		/* All HE data and post assoc mgmt frames fall here */
#ifdef  WL11AX_MGMT_EDCAONLY
		if (type == FC_TYPE_MNG) {
			/* If EDCA is enforced for mgmt compile time */
			return D11_REV80_PHY_TXC_EDCA;
		}
#endif
		return D11_REV80_PHY_TXC_OFDMA_ET;
	} else {
		/* All pre-assoc and non-HE frames, send via EDCA */
		return D11_REV80_PHY_TXC_EDCA;
	}
}
#endif /* WL11AX */

/**
 * Calculate required size of variable portion in RateInfo Blk based upon N-Users, N-RUs
 * and #Power_offset
 * (Return size in bytes).
 */
static INLINE uint16
wlc_txh_var_RateInfo_len(uint8 n_users, uint8 n_rus, uint8 n_pwr_offsets)
{
	return	(wlc_compute_phytxctl_size(n_users, n_rus, n_pwr_offsets) +
		D11_REV80_TXH_BFM0_FIXED_LEN(n_pwr_offsets) +
		D11_REV80_TXH_FBWINFO_FIXED_LEN(n_pwr_offsets));
}

/**
 * Calculate required size of RateInfo Blk based upon N-Users, N-RUs and #Power_offset
 * (Return size in bytes).
 */
INLINE uint16
wlc_compute_RateInfoElem_size(uint8 n_users, uint8 n_rus, uint8 n_pwr_offsets)
{
	return	(D11_REV80_TXH_FIXED_RATEINFO_LEN +
		wlc_txh_var_RateInfo_len(n_users, n_rus, n_pwr_offsets));
}

/**
 * Fill target locations / addresses of each RateInfoBlock and it's fields into the provided
 * (wlc_d11txh_rev80_rate_t *)
 */
static void
wlc_fill_RateInfoBlk_elems(d11txh_rev80_t *txh, wlc_d11txh_rev80_rate_t *rate_blk,
	uint8 n_user, uint8 n_ru, uint8 power_offset)
{
	uint16 RateInfo_elem_size, k;

	RateInfo_elem_size = wlc_compute_RateInfoElem_size(n_user, n_ru, power_offset);

	/* Identify the target locations / addresses of each RateInfoBlock and it's fields */
	for (k = 0; k < D11_REV80_TXH_NUM_RATES; k++) {
		uint8 *r_txh;

		r_txh = (uint8 *)&txh->RateInfoBlock[(k * RateInfo_elem_size)];
		rate_blk[k].txh_rate_fixed = (d11txh_rev80_rate_fixed_t *)r_txh;

		r_txh += D11_REV80_TXH_FIXED_RATEINFO_LEN;
		rate_blk[k].FbwInfo = (uint16 *)r_txh;

		r_txh += D11_REV80_TXH_FBWINFO_FIXED_LEN(power_offset);
		rate_blk[k].Bfm0 = r_txh;

		r_txh += D11_REV80_TXH_BFM0_FIXED_LEN(power_offset);
		rate_blk[k].PhyTxControlByte = r_txh;
	}
}

#if defined(BCMDBG) || defined(WLMSG_PRHDRS) || defined(BCM_11AXHDR_DBG)
/* Print rev80 Tx Desc */
static void
wlc_print_txhdr_rev80(wlc_info_t *wlc, const char *prefix, d11txh_rev80_t *txh,
	uint16 d11_rev80_txh_size, uint8 n_users, uint8 n_rus, uint8 n_pwr_offsets)
{
	uint16 RateInfo_elem_size, k;

	ASSERT(txh && d11_rev80_txh_size);
	ASSERT(D11REV_GE(wlc->pub->corerev, 80));
	ASSERT(txh->PktInfoExt.length == d11_rev80_txh_size);

	/* Print corerev >= 80 TX Desc format */
	printf("\n*** wl%d: %s: TX Desc *** \n", wlc->pub->unit, prefix);

	/* Printing TXD in raw hex format */
	prhex("Raw rev80 TxDesc ", (uchar *)txh, d11_rev80_txh_size);

	/* Print (d11pktinfo_common_t) PktInfo field */
	printf("--- PktInfo (d11pktinfo_common_t) --- \n");
	printf("TSOInfo: 0x%x\n", txh->PktInfo.TSOInfo);
	printf("MacTxControlLow: 0x%x\n", txh->PktInfo.MacTxControlLow);
	printf("MacTxControlHigh: 0x%x\n", txh->PktInfo.MacTxControlHigh);
	printf("Chanspec: 0x%x\n", txh->PktInfo.Chanspec);
	printf("IVOffset: 0x%x\n", txh->PktInfo.IVOffset);
	printf("PktCacheLen: 0x%x\n", txh->PktInfo.PktCacheLen);
	printf("FrameLen: 0x%x\n", txh->PktInfo.FrameLen);
	printf("TxFrameID: 0x%x\n", txh->PktInfo.TxFrameID);
	printf("Seq: 0x%x\n", txh->PktInfo.Seq);
	printf("Tstamp: 0x%x\n", txh->PktInfo.Tstamp);
	printf("TxStatus: 0x%x\n", txh->PktInfo.TxStatus);

	/* Print (d11pktinfo_rev80_t) PktInfoExt field */
	printf("--- PktInfoExt (d11pktinfo_rev80_t) --- \n");
	printf("HEModeControl: 0x%x\n", txh->PktInfoExt.HEModeControl);
	printf("length: 0x%x\n", txh->PktInfoExt.length);

	/* Print (d11txh_rev80_cache_t) CacheInfo field */
	printf("--- CacheInfo (d11txh_rev80_cache_t) --- \n");
	printf("BssIdEncAlg: 0x%x\n", txh->CacheInfo.common.BssIdEncAlg);
	printf("KeyIdx: 0x%x\n", txh->CacheInfo.common.KeyIdx);
	printf("PrimeMpduMax: 0x%x\n", txh->CacheInfo.common.PrimeMpduMax);
	printf("FallbackMpduMax: 0x%x\n", txh->CacheInfo.common.FallbackMpduMax);
	printf("AmpduDur: 0x%x\n", txh->CacheInfo.common.AmpduDur);
	printf("BAWin: 0x%x\n", txh->CacheInfo.common.BAWin);
	printf("MaxAggLen: 0x%x\n", txh->CacheInfo.common.MaxAggLen);
	printf("ampdu_mpdu_all: 0x%x\n", txh->CacheInfo.ampdu_mpdu_all);
	printf("aggid: 0x%x\n", txh->CacheInfo.aggid);
	printf("tkipph1_index: 0x%x\n", txh->CacheInfo.tkipph1_index);
	printf("pktext: 0x%x\n", txh->CacheInfo.pktext);
	printf("reserved: 0x%x\n", txh->CacheInfo.reserved);

	/* Print (*RateInfoBlock) Rate Info Block () */
	RateInfo_elem_size = wlc_compute_RateInfoElem_size(n_users, n_rus, n_pwr_offsets);

	printf("--- RateInfo [Each rate block size = %d, Max Rates = %d] --- \n",
		RateInfo_elem_size, D11_REV80_TXH_NUM_RATES);

	/* Print per Rate Info Blocks in raw hex format */
	for (k = 0; k < D11_REV80_TXH_NUM_RATES; k++) {
		printf("RateCtlBLK [%d] ", k);
		prhex("Raw data :", (uchar *)&txh->RateInfoBlock[(k * RateInfo_elem_size)],
			RateInfo_elem_size);
	}
}
#endif /* BCMDBG || WLMSG_PRHDRS || BCM_11AXHDR_DBG */

/**
 * Add d11txh_rev80_t for rev80 mac & phy
 *
 * 'p' data must start with 802.11 MAC header
 * 'p' must allow enough bytes of local headers to be "pushed" onto the packet
 *
 * headroom == D11_REV80_TXH_LEN (D11_REV80_TXH_LEN is now variable size, includes PHY PLCP header)
 */
uint16
wlc_d11hdrs_rev80(wlc_info_t *wlc, void *p, struct scb *scb, uint txparams_flags, uint frag,
	uint nfrags, uint queue, uint next_frag_len, const wlc_key_info_t *key_info,
	ratespec_t rspec_override, uint16 *txh_off)
{
	d11txh_rev80_t *txh;
	wlc_d11mac_parms_t parms;
	osl_t *osh;
	uint16 frameid, mch, rate_flag;
	uint16 seq = 0, mcl = 0;
	bool use_rts = FALSE;
	bool use_cts = FALSE, rspec_history = FALSE;
	bool use_rifs = FALSE;
	bool short_preamble;
	uint8 preamble_type = WLC_LONG_PREAMBLE;
	uint8 rts_preamble_type;
	struct dot11_rts_frame *rts = NULL;
	ratespec_t rts_rspec = 0;
	uint8 ac;
	uint txrate;
	ratespec_t rspec;
	ratesel_txparams_t cur_rate;
#if defined(BCMDBG)
	uint16 phyctl_bwo = -1;
	uint16 phyctl_sbwo = -1;
#endif 
#if WL_HT_TXBW_OVERRIDE_ENAB
	int8 txbw_override_idx;
#endif
#ifdef WL11N
	wlc_ht_info_t *hti = wlc->hti;
	uint8 sgi_tx;
	bool use_mimops_rts = FALSE;
	int rspec_legacy = -1; /* is initialized to prevent compiler warning */
#endif /* WL11N */
	wlc_bsscfg_t *bsscfg;
	bool g_prot;
	int8 n_prot;
	wlc_wme_t *wme;
	wlc_d11txh_rev80_rate_t rate_blk[D11_REV80_TXH_NUM_RATES];
	uint8 *plcp;
	uint8 n_user, n_ru, power_offset;	/* used for computing phytxctlsize */
	uint16 d11_rev80_txh_size;
#ifdef WL_LPC
	uint8 lpc_offset = 0;
#endif
#if defined(WL_BEAMFORMING)
	uint8 bf_shm_index = BF_SHM_IDX_INV;
	bool bfen = FALSE, fbw_bfen = FALSE;
#endif /* WL_BEAMFORMING */
	uint8 fbw = FBW_BW_INVALID; /* fallback bw */
	txpwr204080_t txpwrs;
	int txpwr_bfsel;
	uint8 txpwr;
	int k;
	wlc_d11pktinfo_t D11PktInfo;
	wlc_d11txh_cache_info_u d11_cache_info;
	bool mutx_pkteng_on = FALSE;
	uint16 HEModeControl = 0;
	wl_tx_chains_t txbf_chains = 0;
	uint8	bfe_sts_cap = 0;
#if defined(WL_PROT_OBSS) && !defined(WL_PROT_OBSS_DISABLED)
	ratespec_t phybw = (CHSPEC_IS8080(wlc->chanspec) || CHSPEC_IS160(wlc->chanspec)) ?
		RSPEC_BW_160MHZ :
		(CHSPEC_IS80(wlc->chanspec) ? RSPEC_BW_80MHZ :
		(CHSPEC_IS40(wlc->chanspec) ? RSPEC_BW_40MHZ : RSPEC_BW_20MHZ));
#endif

	BCM_REFERENCE(bfe_sts_cap);
	BCM_REFERENCE(txbf_chains);
	BCM_REFERENCE(txpwr_bfsel);
	BCM_REFERENCE(mutx_pkteng_on);
#ifdef WL_LPC
	BCM_REFERENCE(lpc_offset);
#endif
	ASSERT(scb != NULL);
	ASSERT(queue < NFIFO_EXT);

	if (BYPASS_ENC_DATA(scb->bsscfg)) {
		key_info = NULL;
	}

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	short_preamble = (txparams_flags & WLC_TX_PARAMS_SHORTPRE) != 0;

	g_prot = WLC_PROT_G_CFG_G(wlc->prot_g, bsscfg);
	n_prot = WLC_PROT_N_CFG_N(wlc->prot_n, bsscfg);

	wme = bsscfg->wme;

	osh = wlc->osh;

	/* Extract 802.11 mac hdr and compute frame length in bytes (for PLCP) */
	wlc_extract_80211mac_hdr(wlc, p, &parms, key_info, frag, nfrags);

	/* N-RUs, N-Users and Power offset values
	 * need to available (by some TBD method) here.
	 *
	 * For now use power offset value of 2, N_RUs value
	 * of 1 and N_Users value of 0 (for supporting STA operation).
	 */

	n_user = 0;
	n_ru = 1;
	power_offset = 2;

	d11_rev80_txh_size = D11_REV80_TXH_LEN(n_user, n_ru, power_offset);

	/* add Broadcom tx descriptor header */
	txh = (d11txh_rev80_t *)PKTPUSH(osh, p, d11_rev80_txh_size);
	bzero((char *)txh, d11_rev80_txh_size);

	/* Locate and fill target addresses of Rate Info block members */
	wlc_fill_RateInfoBlk_elems(txh, rate_blk, n_user, n_ru, power_offset);

	/*
	 * Some fields of the MacTxCtrl are tightly coupled,
	 * and dependant on various other params, which are
	 * yet to be computed. Due to complex dependancies
	 * with a number of params, filling of MacTxCtrl cannot
	 * be completely decoupled and made independant.
	 *
	 * Compute and fill independant fields of the the MacTxCtrl here.
	 */
	wlc_compute_initial_MacTxCtrl(wlc, bsscfg, scb, &mcl, &mch,
		&parms, mutx_pkteng_on);

	/* Compute and assign sequence number to MAC HDR */
	wlc_fill_80211mac_hdr_seqnum(wlc, p, scb, &mcl, &seq, &parms);

	/* TDLS U-APSD buffer STA: if peer is in PS, save the last MPDU's seq and tid for PTI */
#ifdef WLTDLS
	if (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) &&
		SCB_PS(scb) &&
		wlc_tdls_buffer_sta_enable(wlc->tdls)) {
		uint8 tid = 0;
		if (parms.qos) {
			uint16 qc;
			qc = (*(uint16 *)((uchar *)parms.h + (parms.a4 ?
				DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN)));
			tid = (QOS_TID(qc) & 0x0f);
		}
		wlc_tdls_update_tid_seq(wlc->tdls, scb, tid, seq);
	}
#endif

	/* Compute frameid, also possibly change seq */
	wlc_compute_frameid(wlc, bsscfg, txh->PktInfo.TxFrameID, queue, &frameid);

	/* TxStatus, Note the case of recreating the first frag of a suppressed frame
	 * then we may need to reset the retry cnt's via the status reg
	 */
	txh->PktInfo.TxStatus = 0;

	/* (1) RATE: determine and validate primary rate and fallback rates */
	ac = WME_PRIO2AC(PKTPRIO(p));

	/* Todo : Accommodate mgmt/ctl frames sent at non-802.11 basic rates (e.g. Nokia) */
	cur_rate.num = 1; /* default to 1 */

	if (RSPEC_ACTIVE(rspec_override)) {
		cur_rate.rspec[0] = rspec_override;
		rspec_history = TRUE;
	} else if ((parms.type == FC_TYPE_MNG) ||
		(parms.type == FC_TYPE_CTL) ||
		((parms.pkttag)->flags & WLF_8021X) ||
		FALSE) {
		cur_rate.rspec[0] = scb->rateset.rates[0] & RATE_MASK;
	} else if (RSPEC_ACTIVE(wlc->band->mrspec_override) && ETHER_ISMULTI(&parms.h->a1)) {
		cur_rate.rspec[0] = wlc->band->mrspec_override;
	} else if (RSPEC_ACTIVE(wlc->band->rspec_override) && !ETHER_ISMULTI(&parms.h->a1)) {
		cur_rate.rspec[0] = wlc->band->rspec_override;
		rspec_history = TRUE;
	} else if (ETHER_ISMULTI(&parms.h->a1) || SCB_INTERNAL(scb)) {
		cur_rate.rspec[0] = scb->rateset.rates[0] & RATE_MASK;
	} else {
		/* run rate algorithm for data frame only, a cookie will be deposited in frameid */
		cur_rate.num = 4; /* enable multi fallback rate */
		wlc_scb_ratesel_gettxrate(wlc->wrsi, scb, &frameid, &cur_rate, &rate_flag);

#if defined(WL_LINKSTAT)
		if (LINKSTAT_ENAB(wlc->pub))
			wlc_cckofdm_tx_inc(wlc, parms.type,
				wlc_rate_rspec2rate(cur_rate.rspec[0])/500);
#endif /* WL_LINKSTAT */

		if (((scb->flags & SCB_BRCM) == 0) &&
		    (((parms.fc & FC_KIND_MASK) == FC_NULL_DATA) ||
		     ((parms.fc & FC_KIND_MASK) == FC_QOS_NULL))) {
			/* Use RTS/CTS rate for NULL frame */
			cur_rate.num = 2;
			cur_rate.ac = ac;
			cur_rate.rspec[0] =
				wlc_rspec_to_rts_rspec(bsscfg, cur_rate.rspec[0], FALSE);
			cur_rate.rspec[1] = scb->rateset.rates[0] & RATE_MASK;
		} else {
			(parms.pkttag)->flags |= WLF_RATE_AUTO;

			/* The rate histo is updated only for packets on auto rate. */
			/* perform rate history after txbw has been selected */
			if (frag == 0)
				rspec_history = TRUE;
		}

		mch &= ~D11AC_TXC_FIX_RATE;
#ifdef WL11N
		if (rate_flag & RATESEL_VRATE_PROBE)
			WLPKTTAG(p)->flags |= WLF_VRATE_PROBE;
#endif /* WL11N */

#ifdef WL_LPC
		if (LPC_ENAB(wlc)) {
			/* Query the power offset to be used from LPC */
			lpc_offset = wlc_scb_lpc_getcurrpwr(wlc->wlpci, scb, ac);
		} else {
			/* No Link Power Control. Transmit at nominal power. */
		}
#endif
	}

#ifdef WL_RELMCAST
	if (RMC_ENAB(wlc->pub))
		wlc_rmc_process(wlc->rmc, parms.type, p, parms.h, &mcl, &cur_rate.rspec[0]);
#endif

#ifdef WL_NAN
	if (NAN_ENAB(wlc->pub) && BSSCFG_NAN_MGMT(bsscfg)) {
		/* Todo : handle nan with rev80 TXD */
	}
#endif

	for (k = 0; k < cur_rate.num; k++) {
		wlc_d11txh_rev80_rate_t rate_hdr;

		/* init primary and fallback rate pointers */
		rspec = cur_rate.rspec[k];
		rate_hdr = rate_blk[k];

		plcp = rate_hdr.txh_rate_fixed->plcp;
		rate_hdr.txh_rate_fixed->RtsCtsControl = 0;

#ifdef WL11N
		if (N_ENAB(wlc->pub)) {
			uint32 mimo_txbw;

			rspec_legacy = RSPEC_ISLEGACY(rspec);

			mimo_txbw = wlc_d11ac_hdrs_determine_mimo_txbw(wlc, scb, bsscfg, rspec);
#if WL_HT_TXBW_OVERRIDE_ENAB
			if (CHSPEC_IS40(wlc->chanspec) || CHSPEC_IS80(wlc->chanspec)) {
				WL_HT_TXBW_OVERRIDE_IDX(hti, rspec, txbw_override_idx);

				if (txbw_override_idx >= 0) {
					mimo_txbw = txbw2rspecbw[txbw_override_idx];
					phyctl_bwo = txbw2acphyctl0bw[txbw_override_idx];
					phyctl_sbwo = txbw2acphyctl1bw[txbw_override_idx];
				}
				BCM_REFERENCE(mimo_txbw);
				BCM_REFERENCE(phyctl_bwo);
				BCM_REFERENCE(phyctl_sbwo);
			}
#endif /* WL_HT_TXBW_OVERRIDE_ENAB */

#ifdef WL_OBSS_DYNBW
			if (WLC_OBSS_DYNBW_ENAB(wlc->pub)) {
				wlc_obss_dynbw_tx_bw_override(wlc->obss_dynbw, bsscfg,
					&mimo_txbw);
			}
#endif /* WL_OBSS_DYNBW */

			rspec &= ~RSPEC_BW_MASK;
			rspec |= mimo_txbw;
		} else /* N_ENAB */
#endif /* WL11N */
		{
			/* Set ctrlchbw as 20Mhz */
			ASSERT(RSPEC_ISLEGACY(rspec));
			rspec &= ~RSPEC_BW_MASK;
			rspec |= RSPEC_BW_20MHZ;
		}


#ifdef WL11N
		if (N_ENAB(wlc->pub)) {
			uint8 mimo_preamble_type;

			sgi_tx = WLC_HT_GET_SGI_TX(hti);

			if (!RSPEC_ACTIVE(wlc->band->rspec_override)) {
				bool _scb_stbc_on = FALSE;
				bool stbc_tx_forced = WLC_IS_STBC_TX_FORCED(wlc);
				bool stbc_ht_scb_auto = WLC_STF_SS_STBC_HT_AUTO(wlc, scb);
				bool stbc_vht_scb_auto = WLC_STF_SS_STBC_VHT_AUTO(wlc, scb);

				if (sgi_tx == OFF) {
					rspec &= ~RSPEC_SHORT_GI;
				} else if (sgi_tx == ON) {
					if (RSPEC_ISHT(rspec) || RSPEC_ISVHT(rspec)) {
						rspec |= RSPEC_SHORT_GI;
					}
				}


				ASSERT(!(rspec & RSPEC_LDPC_CODING));
				rspec &= ~RSPEC_LDPC_CODING;
				rspec &= ~RSPEC_STBC;

				/* LDPC */
				if (wlc->stf->ldpc_tx == ON ||
				    (((RSPEC_ISVHT(rspec) && SCB_VHT_LDPC_CAP(wlc->vhti, scb)) ||
				      (RSPEC_ISHT(rspec) && SCB_LDPC_CAP(scb))) &&
				     wlc->stf->ldpc_tx == AUTO)) {
					if (!rspec_legacy)
#ifdef WL_PROXDETECT
						if (PROXD_ENAB(wlc->pub) &&
							!wlc_proxd_frame(wlc, parms.pkttag))
#endif
							rspec |= RSPEC_LDPC_CODING;
				}

				/* STBC */
				if ((wlc_ratespec_nsts(rspec) == 1)) {
					if (stbc_tx_forced ||
					  ((RSPEC_ISHT(rspec) && stbc_ht_scb_auto) ||
					   (RSPEC_ISVHT(rspec) && stbc_vht_scb_auto))) {
						_scb_stbc_on = TRUE;
					}

					if (_scb_stbc_on && !rspec_legacy)
#ifdef WL_PROXDETECT
						if (PROXD_ENAB(wlc->pub) &&
							!wlc_proxd_frame(wlc, parms.pkttag))
#endif
							rspec |= RSPEC_STBC;
				}
			}

			/* Determine the HT frame format, HT_MM or HT_GF */
			if (RSPEC_ISHT(rspec)) {
				int nsts;

				mimo_preamble_type = wlc_prot_n_preamble(wlc, scb);
				if (RSPEC_ISHT(rspec)) {
					nsts = wlc_ratespec_nsts(rspec);
					preamble_type = mimo_preamble_type;
					if (n_prot == WLC_N_PROTECTION_20IN40 &&
						RSPEC_IS40MHZ(rspec))
						use_cts = TRUE;

					/* if the mcs is multi stream check if it needs
					 * an rts
					 */
					if (nsts > 1) {
						if (WLC_HT_GET_SCB_MIMOPS_ENAB(hti, scb) &&
							WLC_HT_GET_SCB_MIMOPS_RTS_ENAB(hti, scb))
							use_rts = use_mimops_rts = TRUE;
					}

					/* if SGI is selected, then forced mm for single
					 * stream
					 * (spec section 9.16, Short GI operation)
					 */
					if (RSPEC_ISSGI(rspec) && nsts == 1) {
						preamble_type = WLC_MM_PREAMBLE;
					}
				}
			} else {
				/* VHT always uses MM preamble */
				if (RSPEC_ISVHT(rspec)) {
					preamble_type = WLC_MM_PREAMBLE;
				}
			}
		}

		if (wlc->pub->_dynbw == TRUE) {
			fbw = wlc_tx_dynbw_fbw(wlc, scb, rspec);
		}
#endif /* WL11N */

		/* - FBW_BW_20MHZ to make it 0-index based */
		*rate_hdr.FbwInfo =
			(((fbw - FBW_BW_20MHZ) & FBW_BW_MASK) << FBW_BW_SHIFT);


		/* (2) PROTECTION, may change rspec */
		if (((parms.type == FC_TYPE_DATA) || (parms.type == FC_TYPE_MNG)) &&
			((parms.phylen > wlc->RTSThresh) || ((parms.pkttag)->flags & WLF_USERTS)) &&
			!ETHER_ISMULTI(&parms.h->a1))
			use_rts = TRUE;

		if ((wlc->band->gmode && g_prot && RSPEC_ISOFDM(rspec)) ||
			((RSPEC_ISHT(rspec) || RSPEC_ISVHT(rspec)) && n_prot)) {
			if (nfrags > 1) {
				/* For a frag burst use the lower modulation rates for the
				 * entire frag burst instead of protection mechanisms.
				 * As per spec, if protection mechanism is being used, a
				 * fragment sequence may only employ ERP-OFDM modulation for
				 * the final fragment and control response. (802.11g Sec 9.10)
				 * For ease we send the *whole* sequence at the
				 * lower modulation instead of using a higher modulation for the
				 * last frag.
				 */

				/* downgrade the rate to CCK or OFDM */
				if (g_prot) {
					/* Use 11 Mbps as the rate and fallback. We should make
					 * sure that if we are downgrading an OFDM rate to CCK,
					 * we should pick a more robust rate.  6 and 9 Mbps are not
					 * usually selected by rate selection, but even if the OFDM
					 * rate we are downgrading is 6 or 9 Mbps, 11 Mbps is more
					 * robust.
					 */
					rspec = CCK_RSPEC(WLC_RATE_11M);
				} else {
					/* Use 24 Mbps as the rate and fallback for what would have
					 * been a MIMO rate. 24 Mbps is the highest phy mandatory
					 * rate for OFDM.
					 */
					rspec = OFDM_RSPEC(WLC_RATE_24M);
				}
				(parms.pkttag)->flags &= ~WLF_RATE_AUTO;
			} else {
				/* Use protection mechanisms on unfragmented frames */
				/* If this is a 11g protection, then use CTS-to-self */
				if (wlc->band->gmode && g_prot && !RSPEC_ISCCK(rspec))
					use_cts = TRUE;
			}
		}

		/* calculate minimum rate */
		ASSERT(VALID_RATE_DBG(wlc, rspec));

		if (RSPEC_ISCCK(rspec)) {
			if (short_preamble &&
				!((RSPEC2RATE(rspec) == WLC_RATE_1M) ||
				(scb->bsscfg->PLCPHdr_override == WLC_PLCP_LONG)))
				preamble_type = WLC_SHORT_PREAMBLE;
			else
				preamble_type = WLC_LONG_PREAMBLE;
		}

		ASSERT(RSPEC_ISLEGACY(rspec) || WLC_IS_MIMO_PREAMBLE(preamble_type));

#if defined(WL_BEAMFORMING)
		/* bfe_sts_cap = 3: foursteams, 2: three streams, 1: two streams */
		bfe_sts_cap  = wlc_txbf_get_bfe_sts_cap(wlc->txbf, scb);
		if (bfe_sts_cap) {
			/* Explicit TxBF: Number of txbf chains
			 * is min(#active txchains, #bfe sts + 1)
			 */
			txbf_chains = MIN((uint8)WLC_BITSCNT(wlc->stf->txchain), (bfe_sts_cap + 1));
		} else {
			/* bfe_sts_cap=0 indicates there is no Explicit TxBf link to this peer,
			 * and driver will probably use Implicit TxBF. Ignore the spatial_expension
			 * policy, and always use all currently enabled txcores
			 */
			txbf_chains = (uint8)WLC_BITSCNT(wlc->stf->txchain);
		}
#endif /* WL_BEAMFORMING */
		/* get txpwr for bw204080 and txbf on/off */
		if (wlc_stf_get_204080_pwrs(wlc, rspec, &txpwrs, txbf_chains) != BCME_OK) {
			ASSERT(!"phyctl1 ppr returns error!");
		}

		txpwr_bfsel = 0;
#if defined(WL_BEAMFORMING)
		if (TXBF_ENAB(wlc->pub) &&
			(wlc->allow_txbf) &&
			(preamble_type != WLC_GF_PREAMBLE) &&
			!SCB_ISMULTI(scb) &&
			(parms.type == FC_TYPE_DATA)) {
			ratespec_t fbw_rspec;
			uint16 txpwr_mask, stbc_val;
			fbw_rspec = rspec;
			bfen = wlc_txbf_sel(wlc->txbf, rspec, scb, &bf_shm_index, &txpwrs);

			if (bfen) {
				/* BFM0: Provide alternative tx info if phyctl has bit 3
				 * (BFM) bit
				 * set but ucode has to tx with BFM cleared.
				 */
				if (D11AC2_PHY_SUPPORT(wlc)) {
					/* acphy2 mac/phy interface */
					txpwr_mask = D11AC2_BFM0_TXPWR_MASK;
					stbc_val = D11AC2_BFM0_STBC;
				} else {
					txpwr_mask = BFM0_TXPWR_MASK;
					stbc_val = BFM0_STBC;
				}
				*rate_hdr.Bfm0 =
					((uint16)(txpwrs.pbw[((rspec & RSPEC_BW_MASK) >>
					RSPEC_BW_SHIFT) - BW_20MHZ][TXBF_OFF_IDX])) & txpwr_mask;
				*rate_hdr.Bfm0 |=
					(uint16)(RSPEC_ISSTBC(rspec) ? stbc_val : 0);
			}

			txpwr_bfsel = bfen ? 1 : 0;

			if (fbw != FBW_BW_INVALID) {
				fbw_rspec = (fbw_rspec & ~RSPEC_BW_MASK) |
				((fbw == FBW_BW_40MHZ) ?  RSPEC_BW_40MHZ : RSPEC_BW_20MHZ);
				if (!RSPEC_ISSTBC(fbw_rspec))
					fbw_bfen = wlc_txbf_sel(wlc->txbf, fbw_rspec, scb,
						&bf_shm_index, &txpwrs);
				else
					fbw_bfen = bfen;

				*rate_hdr.FbwInfo |=
					(uint16)(fbw_bfen ? FBW_TXBF : 0);
			}

			if (RSPEC_ACTIVE(wlc->band->rspec_override)) {
				wlc_txbf_applied2ovr_upd(wlc->txbf, bfen);
			}
		}
#endif /* WL_BEAMFORMING */

		/* (3) PLCP: determine PLCP header and MAC duration, fill d11txh_t */
		wlc_compute_plcp(wlc, bsscfg, rspec, parms.phylen, parms.fc, plcp);

		/* RateInfo.TxRate */
		txrate = wlc_rate_rspec2rate(rspec);
		rate_hdr.txh_rate_fixed->TxRate = htol16(txrate/500);


		/* RIFS(testing only): based on frameburst, non-CCK frames only */
		if (SCB_HT_CAP(scb) && WLC_HT_GET_FRAMEBURST(hti) &&
			WLC_HT_GET_RIFS(hti) &&
			n_prot != WLC_N_PROTECTION_MIXEDMODE && !RSPEC_ISCCK(rspec) &&
			!ETHER_ISMULTI(&parms.h->a1) && ((parms.fc & FC_KIND_MASK) ==
			FC_QOS_DATA) &&	(queue < TX_BCMC_FIFO)) {
			uint16 qos_field, *pqos;

			WLPKTTAG(p)->flags |= WLF_RIFS;
			mcl |= (D11AC_TXC_MBURST | D11AC_TXC_URIFS);
			use_rifs = TRUE;

			/* RIFS implies QoS frame with no-ack policy, hack the QoS field */
			pqos = (uint16 *)((uchar *)parms.h + (parms.a4 ?
				DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
			qos_field = ltoh16(*pqos) & ~QOS_ACK_MASK;
			qos_field |= (QOS_ACK_NO_ACK << QOS_ACK_SHIFT) & QOS_ACK_MASK;
			*pqos = htol16(qos_field);
		}

#ifdef WLAMPDU
		/* mark pkt as aggregable if it is */
		if ((k == 0) && WLPKTFLAG_AMPDU(parms.pkttag) && !RSPEC_ISLEGACY(rspec)) {
			if (WLC_KEY_ALLOWS_AMPDU(key_info)) {
				WLPKTTAG(p)->flags |= WLF_MIMO;
				if (WLC_HT_GET_AMPDU_RTS(wlc->hti))
					use_rts = TRUE;
			}
		}
#endif /* WLAMPDU */

		if (parms.type == FC_TYPE_DATA && (queue < TX_BCMC_FIFO)) {
			if ((WLC_HT_GET_FRAMEBURST(hti) &&
				(txrate > WLC_FRAMEBURST_MIN_RATE) &&
#ifdef WLAMPDU
				(wlc_ampdu_frameburst_override(wlc->ampdu_tx, scb) == FALSE) &&
#endif
				(!wme->edcf_txop[ac] || WLPKTFLAG_AMPDU(parms.pkttag))) ||
				FALSE)
#ifdef WL11N
				/* dont allow bursting if rts is required around each mimo frame */
				if (use_mimops_rts == FALSE)
#endif
					mcl |= D11AC_TXC_MBURST;
		}

		if (RSPEC_ISVHT(rspec)) {
			mch |= D11AC_TXC_SVHT;
		}

#if defined(WL_PROT_OBSS) && !defined(WL_PROT_OBSS_DISABLED)
		/* If OBSS protection is enabled, set CTS/RTS accordingly. */
		if (WLC_PROT_OBSS_ENAB(wlc->pub)) {
			if (WLC_PROT_OBSS_PROTECTION(wlc->prot_obss) && !use_rts && !use_cts) {
				if (ETHER_ISMULTI(&parms.h->a1)) {
					/* Multicast and broadcast pkts need CTS protection */
					use_cts = TRUE;
				} else {
					/* Unicast pkts < 80 bw need RTS protection */
					if (RSPEC_BW(rspec) < phybw) {
						use_rts = TRUE;
					}
				}
			}
		}
#endif /* WL_PROT_OBSS && !WL_PROT_OBSS_DISABLED */

		/* (5) RTS/CTS: determine RTS/CTS PLCP header and MAC duration, furnish d11txh_t */
		wlc_d11hdrs_rev80_rts_cts(scb, bsscfg, rspec, use_rts, use_cts, &rts_rspec,
			rate_hdr.txh_rate_fixed, &rts_preamble_type, &mcl);

#ifdef WL_BEAMFORMING
		if (TXBF_ENAB(wlc->pub) && bfen) {
			if (bf_shm_index != BF_SHM_IDX_INV)
				rate_hdr.txh_rate_fixed->RtsCtsControl |=
					htol16((bf_shm_index <<	D11AC_RTSCTS_BF_IDX_SHIFT));
			else
				rate_hdr.txh_rate_fixed->RtsCtsControl |=
					htol16(D11AC_RTSCTS_IMBF);
		}
#endif

		/* (6) Compute and fill in the PhyTxControlBytes */

		/* (6.1) : Fill the fixed size portion of txCtrlWord */
		wlc_axphy_fixed_txctl_calc(wlc, bsscfg, &rate_hdr, rspec, preamble_type);

		/* (6.2) : Fill the variable size portion of txCtrlWord
		 * (depends on n_rus and #power offs)
		 */
#ifdef WL_BEAMFORMING
		if (bfen)
			wlc_axphy_var_txctl_calc(wlc, bsscfg, &rate_hdr, rspec, n_ru,
				power_offset, bfen);
#else
		wlc_axphy_var_txctl_calc(wlc, bsscfg, &rate_hdr, rspec, n_ru,
				power_offset, FALSE);
#endif /* WL_BEAMFORMING */

		/* (6.3) : Compute and fill TXCTL_EXT (depends on n_users) */
		wlc_axphy_txctl_ext_calc(wlc, bsscfg, &rate_hdr, rspec, n_user);

#ifdef WL_PROXDETECT
		if (PROXD_ENAB(wlc->pub) && wlc_proxd_frame(wlc, parms.pkttag)) {
			/* Todo : Handle Proxd antennae masking */
		}
#endif

		/* for fallback bw */
		if (fbw != FBW_BW_INVALID) {
			txpwr = txpwrs.pbw[fbw - FBW_BW_20MHZ][TXBF_OFF_IDX];
			*rate_hdr.FbwInfo |=
				((uint16)txpwr << FBW_BFM0_TXPWR_SHIFT);
#ifdef WL_BEAMFORMING
			if (fbw_bfen) {
				txpwr = txpwrs.pbw[fbw - FBW_BW_20MHZ][TXBF_ON_IDX];
				*rate_hdr.FbwInfo |=
					((uint16)txpwr << FBW_BFM_TXPWR_SHIFT);
			}
#endif
			*rate_hdr.FbwInfo =
				htol16(*rate_hdr.FbwInfo);
		}

#ifdef WL_PROXDETECT
		if (PROXD_ENAB(wlc->pub) && wlc_proxd_frame(wlc, parms.pkttag)) {
			/* Todo : Handle Proxd Sub band location */
		}
#endif

		/* Avoid changing TXOP threshold based on multicast packets */
		if ((k == 0) && SCB_WME(scb) && parms.qos && wme->edcf_txop[ac] &&
			!ETHER_ISMULTI(&parms.h->a1)) {
			uint frag_dur, dur;

			/* WME: Update TXOP threshold */
			if ((!WLPKTFLAG_AMPDU(parms.pkttag)) && (frag == 0)) {
				int16 delta;

				frag_dur = wlc_calc_frame_time(wlc, rspec, preamble_type,
					parms.phylen);

				if (rts) {
					/* 1 RTS or CTS-to-self frame */
					dur = wlc_calc_cts_time(wlc, rts_rspec, rts_preamble_type);
					/* (SIFS + CTS) + SIFS + frame + SIFS + ACK */
					dur += ltoh16(rts->durid);
				} else if (use_rifs) {
					dur = frag_dur;
				} else {
					/* frame + SIFS + ACK */
					dur = frag_dur;
					dur += wlc_compute_frame_dur(wlc,
						CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
						rspec, preamble_type, 0);
				}

				/* update txop byte threshold (txop minus intraframe overhead) */
				delta = (int16)(wme->edcf_txop[ac] - (dur - frag_dur));
				if (delta >= 0) {
#ifdef WLAMSDU_TX
					if (WLPKTFLAG_AMSDU(parms.pkttag) &&
						(queue == TX_AC_BE_FIFO)) {
						WL_ERROR(("edcf_txop changed, update AMSDU\n"));
						wlc_amsdu_txop_upd(wlc->ami);
					} else
#endif
					{
						if (!((parms.pkttag)->flags & WLF_8021X)) {
							uint newfragthresh =
								wlc_calc_frame_len(wlc, rspec,
								preamble_type, (uint16)delta);
							/* range bound the fragthreshold */
							newfragthresh = MAX(newfragthresh,
								DOT11_MIN_FRAG_LEN);
							newfragthresh = MIN(newfragthresh,
								wlc->usr_fragthresh);
							/* update the fragthr and do txc update */
							if (wlc->fragthresh[ac] !=
								(uint16)newfragthresh) {
									wlc_fragthresh_set(wlc,
									ac,
									(uint16)newfragthresh);
							}

						}
					}
				} else {
					WL_ERROR(("wl%d: %s txop invalid for rate %d\n",
						wlc->pub->unit, fifo_names[queue],
						RSPEC2RATE(rspec)));
				}

				if (CAC_ENAB(wlc->pub) &&
					queue <= TX_AC_VO_FIFO) {
					/* update cac used time */
					if (wlc_cac_update_used_time(wlc->cac, ac, dur, scb))
						WL_ERROR(("wl%d: ac %d: txop exceeded allocated TS"
							"time\n", wlc->pub->unit, ac));
				}

				if (dur > wme->edcf_txop[ac])
					WL_ERROR(("wl%d: %s txop exceeded phylen %d/%d dur %d/%d\n",
						wlc->pub->unit, fifo_names[queue], parms.phylen,
						wlc->fragthresh[ac], dur, wme->edcf_txop[ac]));
			}
		} else if ((k == 0) && SCB_WME(scb) && parms.qos && CAC_ENAB(wlc->pub) &&
			queue <= TX_AC_VO_FIFO) {
			uint dur;
			if (rts) {
				/* 1 RTS or CTS-to-self frame */
				dur = wlc_calc_cts_time(wlc, rts_rspec, rts_preamble_type);
				/* (SIFS + CTS) + SIFS + frame + SIFS + ACK */
				dur += ltoh16(rts->durid);
			} else {
				/* frame + SIFS + ACK */
				dur = wlc_calc_frame_time(wlc, rspec, preamble_type, parms.phylen);
				dur += wlc_compute_frame_dur(wlc,
					CHSPEC2WLC_BAND(bsscfg->current_bss->chanspec),
					rspec, preamble_type, 0);
			}
			/* update cac used time */
			if (wlc_cac_update_used_time(wlc->cac, ac, dur, scb))
				WL_ERROR(("wl%d: ac %d: txop exceeded allocated TS time\n",
					wlc->pub->unit, ac));
		}
		/* Store the final rspec back into the cur_rate */
		cur_rate.rspec[k] = rspec;
	} /* rate loop ends */

	/* Mark last rate */
	rate_blk[cur_rate.num-1].txh_rate_fixed->RtsCtsControl |=
		htol16(D11AC_RTSCTS_LAST_RATE);

	D11PktInfo.PktInfo = &txh->PktInfo;
	D11PktInfo.PktInfoExt = &txh->PktInfoExt;

	/* Compute and fill "per packet info" / short TX Desc (24 bytes) */
	wlc_fill_txd_per_pkt_info(wlc, p, scb, &D11PktInfo,
		mcl, mch, &parms, frameid, d11_rev80_txh_size, HEModeControl);

	/* record rate history here (pick final primary rspec) */
	rspec = cur_rate.rspec[0];
	WLCNTSCB_COND_SET(((parms.type == FC_TYPE_DATA) &&
		((FC_SUBTYPE(parms.fc) != FC_SUBTYPE_NULL) &&
			(FC_SUBTYPE(parms.fc) != FC_SUBTYPE_QOS_NULL))),
			scb->scb_stats.tx_rate, rspec);

	/* record rate history after the txbw is valid */
	if (rspec_history) {
		/* update per bsscfg tx rate */
		bsscfg->txrspec[bsscfg->txrspecidx][0] = rspec;
		bsscfg->txrspec[bsscfg->txrspecidx][1] = (uint8) nfrags;
		bsscfg->txrspecidx = (bsscfg->txrspecidx+1) % NTXRATE;

		WLCNTSCBSET(scb->scb_stats.tx_rate_fallback, cur_rate.rspec[cur_rate.num - 1]);
	}

#ifdef WLFCTS
	if (WLFCTS_ENAB(wlc->pub))
		WLPKTTAG(p)->rspec = rspec;
#endif
	d11_cache_info.rev80_CacheInfo = &txh->CacheInfo;

	/* Compute and fill "per cache info" for TX Desc (24 bytes) */
	wlc_fill_txd_per_cache_info(wlc, bsscfg, scb, p, parms.pkttag,
		d11_cache_info, FALSE);

#ifdef WLTOEHW
	/* add tso-oe header, if capable and toe module is not bypassed */
	if (wlc->toe_capable && !wlc->toe_bypass)
		wlc_toe_add_hdr(wlc, p, scb, key_info, nfrags, txh_off);
#endif

	/* With d11 hdrs on, mark the packet as MPDU with txhdr */
	WLPKTTAG(p)->flags |= (WLF_MPDU | WLF_TXHDR);

#if defined(BCMDBG) || defined(WLMSG_PRHDRS) || defined(BCM_11AXHDR_DBG)
	wlc_print_txhdr_rev80(wlc, "11AX (corerev >= 80)", txh,
		d11_rev80_txh_size, n_user, n_ru, power_offset);
#endif /* BCMDBG || WLMSG_PRHDRS || BCM_11AXHDR_DBG */

	return (frameid);
} /* wlc_d11hdrs_rev80 */
#endif /* WL11AX */

/**
 * Add d11 tx headers
 *
 * 'pkt' data must start with 802.11 MAC header
 * 'pkt' must allow enough bytes of headers (headroom) to be "pushed" onto the packet
 *
 * Each core revid may require different headroom. See each wlc_d11hdrs_xxx() for details.
 */
#ifdef WLC_D11HDRS_DBG
uint16
wlc_d11hdrs(wlc_info_t *wlc, void *pkt, struct scb *scb, uint txparams_flags,
	uint frag, uint nfrags, uint queue, uint next_frag_len, const wlc_key_info_t *key_info,
	ratespec_t rspec_override, uint16 *txh_off)
{
	uint16 retval;
	uint8 *txh;
#if defined(BCMDBG) || defined(WLMSG_PRHDRS)
	uint8 *h = PKTDATA(wlc->osh, pkt);
#endif

	retval = WLC_D11HDRS_FN(wlc, pkt, scb, txparams_flags,
		frag, nfrags, queue, next_frag_len, key_info,
		rspec_override, txh_off);

	txh = PKTDATA(wlc->osh, pkt);

	WL_PRHDRS(wlc, "txpkt hdr (MPDU)", h, (wlc_txd_t*)txh, NULL, PKTLEN(wlc->osh, pkt));
	WL_PRPKT("txpkt body (MPDU)", txh, PKTLEN(wlc->osh, pkt));

	return retval;

} /* wlc_d11hdrs */
#endif /* WLC_D11HDRS_DBG */

/** The opposite of wlc_calc_frame_time */
static uint
wlc_calc_frame_len(wlc_info_t *wlc, ratespec_t ratespec, uint8 preamble_type, uint dur)
{
	uint nsyms, mac_len, Ndps;
	uint rate = RSPEC2RATE(ratespec);

	WL_TRACE(("wl%d: %s: rspec 0x%x, preamble_type %d, dur %d\n",
	          wlc->pub->unit, __FUNCTION__, ratespec, preamble_type, dur));

	if (RSPEC_ISVHT(ratespec)) {
		mac_len = (dur * rate) / 8000;
	} else if (RSPEC_ISHT(ratespec)) {
		mac_len = wlc_ht_calc_frame_len(wlc->hti, ratespec, preamble_type, dur);
	} else if (RSPEC_ISOFDM(ratespec)) {
		dur -= APHY_PREAMBLE_TIME;
		dur -= APHY_SIGNAL_TIME;
		/* Ndbps = Mbps * 4 = rate(500Kbps) * 2 */
		Ndps = rate*2;
		nsyms = dur / APHY_SYMBOL_TIME;
		mac_len = ((nsyms * Ndps) - (APHY_SERVICE_NBITS + APHY_TAIL_NBITS)) / 8;
	} else {
		if (preamble_type & WLC_SHORT_PREAMBLE)
			dur -= BPHY_PLCP_SHORT_TIME;
		else
			dur -= BPHY_PLCP_TIME;
		mac_len = dur * rate;
		/* divide out factor of 2 in rate (1/2 mbps) */
		mac_len = mac_len / 8 / 2;
	}
	return mac_len;
}

#ifdef WL_FRAGDUR
uint
wlc_calc_frame_len_ex(wlc_info_t *wlc, ratespec_t ratespec, uint8 preamble_type, uint dur)
{
	int  nsyms, mac_len, rem_dur;
	uint Ndps, kNdps;
	uint rate = RSPEC2RATE(ratespec);

	WL_TRACE(("wl%d: wlc_calc_frame_len_ex: rspec 0x%x, preamble_type %d, dur %d\n",
		wlc->pub->unit, ratespec, preamble_type, dur));
/*
 *	rem_dur is the remaining duration for transmitting a frame of duration <dur>.
 *	it is used to calculate the payload length after overhead due to preamble and header
 *	transmission time is taken into account. <rem_dur> and the resulting frame length <mac_len>
 *	cannot be negative; if otherwise, it shall be handled as an error condition with
 *	this function's return value set to 0.
 */
	rem_dur = (int) dur;
	if (RSPEC_ISVHT(ratespec)) {
		mac_len = (dur * rate) / 8000;
	} else if (RSPEC_ISHT(ratespec)) {
		uint mcs = ratespec & RSPEC_RATE_MASK;
		int tot_streams = wlc_ratespec_nsts(ratespec);
		ASSERT(WLC_PHY_11N_CAP(wlc->band));
		rem_dur -= PREN_PREAMBLE + ((tot_streams - 1)*PREN_PREAMBLE_EXT);
		/* payload calculation matches that of regular ofdm */
		if (BAND_2G(wlc->band->bandtype))
			rem_dur -= DOT11_OFDM_SIGNAL_EXTENSION;
		if (rem_dur <= 0)
			return 0;
		/* kNdbps = kbps * 4 */
		kNdps = MCS_RATE(mcs, RSPEC_IS40MHZ(ratespec), RSPEC_ISSGI(ratespec)) * 4;
		nsyms = rem_dur / APHY_SYMBOL_TIME;
		mac_len = ((nsyms * kNdps) - ((APHY_SERVICE_NBITS + APHY_TAIL_NBITS)*1000)) / 8000;
		if (mac_len <= 0)
			return 0;
	} else if (RSPEC_ISOFDM(ratespec)) {
		rem_dur -= APHY_PREAMBLE_TIME;
		rem_dur -= APHY_SIGNAL_TIME;
		if (BAND_2G(wlc->band->bandtype))
			rem_dur -= DOT11_OFDM_SIGNAL_EXTENSION;
		if (rem_dur <= 0)
			return 0;
		/* Ndbps = Mbps * 4 = rate(500Kbps) * 2 */
		Ndps = rate*2;
		nsyms = rem_dur / APHY_SYMBOL_TIME;
		mac_len = ((nsyms * Ndps) - (APHY_SERVICE_NBITS + APHY_TAIL_NBITS)) / 8;
		if (mac_len <= 0)
			return 0;
	} else {
		if (preamble_type & WLC_SHORT_PREAMBLE)
			rem_dur -= BPHY_PLCP_SHORT_TIME;
		else
			rem_dur -= BPHY_PLCP_TIME;
		if (rem_dur <= 0)
			return 0;
		mac_len = rem_dur * rate;
		/* divide out factor of 2 in rate (1/2 mbps) */
		mac_len = mac_len / 8 / 2;
	}
	return mac_len;
}
#endif /* WL_FRAGDUR */

/**
 * Fetch the TxD (DMA hardware hdr), vhtHdr (rev 40+ ucode hdr),
 * and 'd11_hdr' (802.11 frame header), from the given Tx prepared pkt.
 * This function only works for d11 core rev >= 40 since it is extracting
 * rev 40+ specific header information.
 *
 * A Tx Prepared packet is one that has been prepared for the hw fifo and starts with the
 * hw/ucode header.
 *
 * @param wlc           wlc_info_t pointer
 * @param p             pointer to pkt from wich to extract interesting header pointers
 * @param ptxd          on return will be set to a pointer to the TxD hardware header,
 *                      also know as @ref d11ac_tso_t
 * @param ptxh          on return will be set to a pointer to the ucode TxH header @ref d11actxh_t
 * @param pd11hdr       on return will be set to a pointer to the 802.11 header bytes,
 *                      starting with Frame Control
 */
void BCMFASTPATH
wlc_txprep_pkt_get_hdrs(wlc_info_t* wlc, void* p,
	uint8** ptxd, uint8** ptxh, struct dot11_header** pd11_hdr)
{
	uint8* txd;
	uint8* txh;
	uint txd_len;
	uint txh_len;
	uint16 mcl;

	/* this fn is only for new VHT (11ac) capable ucode headers */
	ASSERT(D11REV_GE(wlc->pub->corerev, 40));

	BCM_REFERENCE(mcl);

	txd = PKTDATA(wlc->osh, p);
	txd_len = (txd[0] & TOE_F0_HDRSIZ_NORMAL) ?
	        TSO_HEADER_LENGTH : TSO_HEADER_PASSTHROUGH_LENGTH;

	txh = (uint8 *)(txd + txd_len);

	if (D11REV_GE(wlc->pub->corerev, 80)) {
		txh_len = ((d11txh_rev80_t *)txh)->PktInfoExt.length;
	}
	else {
#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
		mcl = ((d11pktinfo_common_t *)txh)->PktInfo.MacTxControlLow;

		if (mcl & htol16(D11AC_TXC_HDR_FMT_SHORT)) {
			txh_len = D11AC_TXH_SHORT_LEN;
		} else
#endif /* defined(WLC_UCODE_CACHE) && D11CONF_GE(42) */
		if (SFD_ENAB(wlc->pub) && PKTISTXFRAG(wlc->osh, p)) {
			txh_len = D11AC_TXH_SFD_LEN;
		} else {
			txh_len = D11AC_TXH_LEN;
		}
	}

	*ptxd = txd;
	*ptxh = txh;
	*pd11_hdr = (struct dot11_header*)((uint8*)txh + txh_len);
} /* wlc_txprep_pkt_get_hdrs */

/** have to extract SGI/STBC differently pending on frame type being 11n or 11vht */
bool BCMFASTPATH
wlc_txh_get_isSGI(const wlc_txh_info_t* txh_info)
{
	uint16 frametype;

	ASSERT(txh_info);

	frametype = ltoh16(txh_info->PhyTxControlWord0) & PHY_TXC_FT_MASK;
	if (frametype == FT_HT)
		return (PLCP3_ISSGI(txh_info->plcpPtr[3]));
	else if (frametype == FT_VHT)
		return (VHT_PLCP3_ISSGI(txh_info->plcpPtr[3]));

	return FALSE;
}

bool BCMFASTPATH
wlc_txh_get_isSTBC(const wlc_txh_info_t* txh_info)
{
	uint8 frametype;

	ASSERT(txh_info);
	ASSERT(txh_info->plcpPtr);

	frametype = ltoh16(txh_info->PhyTxControlWord0) & PHY_TXC_FT_MASK;
	if (frametype == FT_HT)
		return (PLCP3_ISSTBC(txh_info->plcpPtr[3]));
	else if (frametype == FT_VHT)
		return ((txh_info->plcpPtr[0] & VHT_SIGA1_STBC) != 0);

	return FALSE;
}

chanspec_t
wlc_txh_get_chanspec(wlc_info_t* wlc, wlc_txh_info_t* tx_info)
{
	if (D11REV_LT(wlc->pub->corerev, 40)) {
		d11txh_t* nonVHTHdr = (d11txh_t*)(tx_info->hdrPtr);
		return (ltoh16(nonVHTHdr->XtraFrameTypes) >> XFTS_CHANNEL_SHIFT);
	} else {
		d11actxh_t* vhtHdr = (d11actxh_t*)(tx_info->hdrPtr);
		return ltoh16(vhtHdr->PktInfo.Chanspec);
	}
}

uint16 BCMFASTPATH
wlc_get_txh_frameid(wlc_info_t	*wlc, void *p)
{
	uint8 *pkt_data = NULL;

	ASSERT(p);
	pkt_data = PKTDATA(wlc->osh, p);
	ASSERT(pkt_data != NULL);

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		d11txh_t* nonVHTHdr = (d11txh_t *)pkt_data;

		return ltoh16(nonVHTHdr->TxFrameID);
	} else {
		uint tsoHdrSize = 0;
		d11actxh_t *vhtHdr;
		d11actxh_pkt_t *ppkt_info;

#ifdef WLTOEHW
		tsoHdrSize = (wlc->toe_bypass ?
			0 : wlc_tso_hdr_length((d11ac_tso_t*)pkt_data));
#endif /* WLTOEHW */

		ASSERT(PKTLEN(wlc->osh, p) >= D11AC_TXH_SHORT_LEN + tsoHdrSize);

		vhtHdr = (d11actxh_t*)(pkt_data + tsoHdrSize);
		ppkt_info = &(vhtHdr->PktInfo);

		return ltoh16(ppkt_info->TxFrameID);
	}
}

/**
 * parse info from a tx header and return in a common format
 * -- needed due to differences between ac, ax and pre-ac tx hdrs
 */
void BCMFASTPATH
wlc_get_txh_info(wlc_info_t* wlc, void* p, wlc_txh_info_t* tx_info)
{
	uint8 *pkt_data = NULL;
	if (p == NULL || tx_info == NULL ||
		((pkt_data = PKTDATA(wlc->osh, p)) == NULL)) {
		WL_ERROR(("%s: null P or tx_info or pktdata.\n", __FUNCTION__));
		ASSERT(p != NULL);
		ASSERT(tx_info != NULL);
		ASSERT(pkt_data != NULL);
		return;
	}

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		d11txh_t* nonVHTHdr = (d11txh_t *)pkt_data;

		tx_info->tsoHdrPtr = NULL;
		tx_info->tsoHdrSize = 0;

		tx_info->TxFrameID = nonVHTHdr->TxFrameID;
		tx_info->Tstamp = (((uint32)ltoh16(nonVHTHdr->TstampHigh) << 16) |
		                   ltoh16(nonVHTHdr->TstampLow));
		tx_info->MacTxControlLow = nonVHTHdr->MacTxControlLow;
		tx_info->MacTxControlHigh = nonVHTHdr->MacTxControlHigh;
		tx_info->hdrSize = sizeof(d11txh_t);
		tx_info->hdrPtr = (wlc_txd_t*)nonVHTHdr;
		tx_info->d11HdrPtr = (void*)((uint8*)(nonVHTHdr + 1) + D11_PHY_HDR_LEN);
		tx_info->TxFrameRA = nonVHTHdr->TxFrameRA;
		tx_info->plcpPtr = (uint8 *)(nonVHTHdr + 1);
		tx_info->PhyTxControlWord0 = nonVHTHdr->PhyTxControlWord;
		tx_info->PhyTxControlWord1 = nonVHTHdr->PhyTxControlWord_1;
		tx_info->seq = ltoh16(((struct dot11_header*)tx_info->d11HdrPtr)->seq) >>
			SEQNUM_SHIFT;

		tx_info->d11FrameSize = (uint16)(pkttotlen(wlc->osh, p) -
			(sizeof(d11txh_t) + D11_PHY_HDR_LEN));

		/* unused */
		tx_info->PhyTxControlWord2 = 0;
		tx_info->w.ABI_MimoAntSel = (uint16)(nonVHTHdr->ABI_MimoAntSel);
	} else if (D11REV_LT(wlc->pub->corerev, 80)) {
		uint8* pktHdr;
		uint tsoHdrSize = 0;
		d11actxh_t* vhtHdr;
		d11actxh_rate_t *rateInfo;
		d11actxh_pkt_t *ppkt_info;

		pktHdr = (uint8*)pkt_data;

#ifdef WLTOEHW
		tsoHdrSize = WLC_TSO_HDR_LEN(wlc, (d11ac_tso_t*)pktHdr);
		tx_info->tsoHdrPtr = (void*)((tsoHdrSize != 0) ? pktHdr : NULL);
		tx_info->tsoHdrSize = tsoHdrSize;
#else
		tx_info->tsoHdrPtr = NULL;
		tx_info->tsoHdrSize = 0;
#endif /* WLTOEHW */

		ASSERT((uint)PKTLEN(wlc->osh, p) >= D11AC_TXH_SHORT_LEN + tsoHdrSize);

		vhtHdr = (d11actxh_t*)(pktHdr + tsoHdrSize);

		ppkt_info = &(vhtHdr->PktInfo);

		tx_info->Tstamp = ((uint32)ltoh16(ppkt_info->Tstamp)) << D11AC_TSTAMP_SHIFT;
		tx_info->TxFrameID = ppkt_info->TxFrameID;
		tx_info->MacTxControlLow = ppkt_info->MacTxControlLow;
		tx_info->MacTxControlHigh = ppkt_info->MacTxControlHigh;

#if defined(WLC_UCODE_CACHE) && D11CONF_GE(42)
		if (tx_info->MacTxControlLow & htol16(D11AC_TXC_HDR_FMT_SHORT)) {
			int cache_idx = (ltoh16(tx_info->MacTxControlLow) &
				D11AC_TXC_CACHE_IDX_MASK) >> D11AC_TXC_CACHE_IDX_SHIFT;
			rateInfo = (d11actxh_rate_t *)wlc_txc_get_rate_info_shdr(wlc->txc,
				cache_idx);
			tx_info->hdrSize = D11AC_TXH_SHORT_LEN;
		} else
#endif /* defined(WLC_UCODE_CACHE) && D11CONF_GE(42) */
#ifdef BCM_SFD
		if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(wlc->osh, p)) {
			rateInfo = wlc_sfd_get_rate_info(wlc->sfd, vhtHdr);
			tx_info->hdrSize = D11AC_TXH_SFD_LEN;
		} else
#endif
		{
			rateInfo = WLC_TXD_RATE_INFO_GET(vhtHdr, wlc->pub->corerev);
			tx_info->hdrSize = D11AC_TXH_LEN;
		}

		tx_info->hdrPtr = (wlc_txd_t*)vhtHdr;
		tx_info->d11HdrPtr = (void*)((uint8*)vhtHdr + tx_info->hdrSize);
		tx_info->d11FrameSize =
		        pkttotlen(wlc->osh, p) - (tsoHdrSize + tx_info->hdrSize);

		/* a1 holds RA when RA is used; otherwise this field can/should be ignored */
		tx_info->TxFrameRA = (uint8*)(((struct dot11_header *)
								(tx_info->d11HdrPtr))->a1.octet);

		tx_info->plcpPtr = (uint8*)(rateInfo[0].plcp);
		/* usu this will work, as long as primary rate is the only one concerned with */
		tx_info->PhyTxControlWord0 = rateInfo[0].PhyTxControlWord_0;
		tx_info->PhyTxControlWord1 = rateInfo[0].PhyTxControlWord_1;
		tx_info->PhyTxControlWord2 = rateInfo[0].PhyTxControlWord_2;
		tx_info->w.FbwInfo = ltoh16(rateInfo[0].FbwInfo);

		tx_info->seq = ltoh16(ppkt_info->Seq) >> SEQNUM_SHIFT;
	}
#ifdef WL11AX
	else {
		uint8* pktHdr;
		uint tsoHdrSize = 0;
		d11txh_rev80_t* heHdr;
		uint8 *rateInfo;
		d11pktinfo_rev80_t *PktInfoExt;
		d11pktinfo_common_t *PktInfo;
		uint16 *FbwInfo;
		uint16 *PhyTxControlBlk;
		uint8 power_offs = 2;

		pktHdr = (uint8*)pkt_data;

#ifdef WLTOEHW
		tsoHdrSize = WLC_TSO_HDR_LEN(wlc, (d11ac_tso_t*)pktHdr);
		tx_info->tsoHdrPtr = (void*)((tsoHdrSize != 0) ? pktHdr : NULL);
		tx_info->tsoHdrSize = tsoHdrSize;
#else
		tx_info->tsoHdrPtr = NULL;
		tx_info->tsoHdrSize = 0;
#endif /* WLTOEHW */

		ASSERT((uint)PKTLEN(wlc->osh, p) >= D11_REV80_TXH_SHORT_LEN + tsoHdrSize);

		heHdr = (d11txh_rev80_t*)(pktHdr + tsoHdrSize);

		PktInfo = &(heHdr->PktInfo);
		PktInfoExt = &(heHdr->PktInfoExt);

		tx_info->Tstamp = ((uint32)ltoh16(PktInfo->Tstamp)) << D11AC_TSTAMP_SHIFT;
		tx_info->TxFrameID = PktInfo->TxFrameID;
		tx_info->MacTxControlLow = PktInfo->MacTxControlLow;
		tx_info->MacTxControlHigh = PktInfo->MacTxControlHigh;

#if defined(WLC_UCODE_CACHE)
		if (tx_info->MacTxControlLow & htol16(D11AC_TXC_HDR_FMT_SHORT)) {
			/* Todo : Handle Ucode caching for 11ax */
		} else
#endif /* defined(WLC_UCODE_CACHE) */
		{
			rateInfo = heHdr->RateInfoBlock;
			tx_info->hdrSize = PktInfoExt->length;
		}

		tx_info->hdrPtr = (wlc_txd_t *)heHdr;
		tx_info->d11HdrPtr = (void *)((uint8*)heHdr + tx_info->hdrSize);
		tx_info->d11FrameSize =
		        pkttotlen(wlc->osh, p) - (tsoHdrSize + tx_info->hdrSize);

		/* a1 holds RA when RA is used; otherwise this field can/should be ignored */
		tx_info->TxFrameRA = (uint8 *)(((struct dot11_header *)
			(tx_info->d11HdrPtr))->a1.octet);

		tx_info->seq = ltoh16(PktInfo->Seq) >> SEQNUM_SHIFT;

		tx_info->plcpPtr = (uint8 *)(((d11txh_rev80_rate_fixed_t *) rateInfo)->plcp);
		FbwInfo = (uint16 *)(rateInfo + D11_REV80_TXH_FIXED_RATEINFO_LEN);
		tx_info->w.FbwInfo = ltoh16(*FbwInfo);

		/* TODO : power_offs to be calculated during run time */
		PhyTxControlBlk = (uint16 *)((uint8 *)FbwInfo +
			D11_REV80_TXH_BFM0_FIXED_LEN(power_offs));

		/* this will work, as long as primary rate is the only one concerned with */
		tx_info->PhyTxControlWord0 = PhyTxControlBlk[0];
		tx_info->PhyTxControlWord1 = PhyTxControlBlk[1];
		tx_info->PhyTxControlWord2 = PhyTxControlBlk[2];
		tx_info->PhyTxControlWord3 = PhyTxControlBlk[3];
		tx_info->PhyTxControlWord4 = PhyTxControlBlk[4];
	}
#endif /* WL11AX */
} /* wlc_get_txh_info */

/**
 * set info in a tx header from a common format
 * -- needed due to differences between ac, ax and pre-ac tx hdrs
 * -- ptr fields are NOT settable and this function will ASSERT if such op is tried
 */
void
wlc_set_txh_info(wlc_info_t* wlc, void* p, wlc_txh_info_t* tx_info)
{
	ASSERT(p);
	ASSERT(tx_info);

	if (p == NULL) return;

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		d11txh_t* nonVHTHdr = &tx_info->hdrPtr->d11txh;

		nonVHTHdr->TxFrameID = tx_info->TxFrameID;
		nonVHTHdr->TstampLow = htol16((uint16)tx_info->Tstamp);
		nonVHTHdr->TstampHigh = htol16((uint16)(tx_info->Tstamp >> 16));
		nonVHTHdr->PhyTxControlWord = tx_info->PhyTxControlWord0;
		nonVHTHdr->MacTxControlLow = tx_info->MacTxControlLow;
		nonVHTHdr->MacTxControlHigh = tx_info->MacTxControlHigh;

		nonVHTHdr->ABI_MimoAntSel = tx_info->w.ABI_MimoAntSel;

		/* these values should not be modified */
		ASSERT(tx_info->hdrSize == sizeof(d11txh_t));
		ASSERT(tx_info->d11HdrPtr == (void*)((uint8*)(nonVHTHdr + 1) + D11_PHY_HDR_LEN));
		ASSERT(tx_info->TxFrameRA == nonVHTHdr->TxFrameRA);
	} else if (D11REV_LT(wlc->pub->corerev, 80)) {
		d11actxh_t* vhtHdr = &tx_info->hdrPtr->txd;
		d11actxh_rate_t* local_rate_info;

		vhtHdr->PktInfo.TxFrameID = tx_info->TxFrameID;
		/* vht tx hdr has only one time stamp field */
		vhtHdr->PktInfo.Tstamp = htol16((uint16)(tx_info->Tstamp >> D11AC_TSTAMP_SHIFT));
		vhtHdr->PktInfo.MacTxControlLow = tx_info->MacTxControlLow;
		vhtHdr->PktInfo.MacTxControlHigh = tx_info->MacTxControlHigh;

#ifdef BCM_SFD
		if (SFD_ENAB(wlc->pub) && PKTISSFDFRAME(wlc->osh, p)) {
			local_rate_info = wlc_sfd_get_rate_info(wlc->sfd, vhtHdr);
		} else
#endif
		{
			local_rate_info = WLC_TXD_RATE_INFO_GET(vhtHdr, wlc->pub->corerev);
		}


		local_rate_info[0].PhyTxControlWord_0 = tx_info->PhyTxControlWord0;
		local_rate_info[0].PhyTxControlWord_1 = tx_info->PhyTxControlWord1;
		local_rate_info[0].PhyTxControlWord_2 = tx_info->PhyTxControlWord2;

		/* a1 holds RA when RA is used; otherwise this field should be ignored */
		ASSERT(tx_info->TxFrameRA == (uint8*)
				((struct dot11_header *)tx_info->d11HdrPtr)->a1.octet);
		ASSERT(tx_info->plcpPtr == (uint8*)(local_rate_info[0].plcp));
	}
#ifdef WL11AX
	else {
		d11txh_rev80_t* heHdr = &tx_info->hdrPtr->rev80_txd;
		uint16 *FbwInfo;
		uint16 *PhyTxControlBlk;
		uint8 power_offs = 2;
		uint8 *rateInfo;

		heHdr->PktInfo.TxFrameID = tx_info->TxFrameID;
		/* vht tx hdr has only one time stamp field */
		heHdr->PktInfo.Tstamp = htol16((uint16)(tx_info->Tstamp >> D11AC_TSTAMP_SHIFT));
		heHdr->PktInfo.MacTxControlLow = tx_info->MacTxControlLow;
		heHdr->PktInfo.MacTxControlHigh = tx_info->MacTxControlHigh;

		rateInfo = heHdr->RateInfoBlock;
		FbwInfo = (uint16 *)(rateInfo + D11_REV80_TXH_FIXED_RATEINFO_LEN);

		/* TODO : power_offs to be calculated during run time */
		PhyTxControlBlk = (uint16 *)((uint8 *)FbwInfo +
			D11_REV80_TXH_BFM0_FIXED_LEN(power_offs));

		PhyTxControlBlk[0] = tx_info->PhyTxControlWord0;
		PhyTxControlBlk[1] = tx_info->PhyTxControlWord1;
		PhyTxControlBlk[2] = tx_info->PhyTxControlWord2;
		PhyTxControlBlk[3] = tx_info->PhyTxControlWord3;
		PhyTxControlBlk[4] = tx_info->PhyTxControlWord4;

		/* a1 holds RA when RA is used; otherwise this field should be ignored */
		ASSERT(tx_info->TxFrameRA == (uint8*)
				((struct dot11_header *)tx_info->d11HdrPtr)->a1.octet);
		ASSERT(tx_info->plcpPtr ==
		       (uint8*)(((d11txh_rev80_rate_fixed_t *)(rateInfo))->plcp));
	}
#endif /* WL11AX */
} /* wlc_set_txh_info */

#ifdef WL11N
/**
 *	For HT framem, return mcs index
 *	For VHT frame:
 *		bit 0-3 mcs index
 *		bit 6-4 nsts for VHT
 * 		bit 7:	 1 for VHT
 */
uint8
wlc_txh_info_get_mcs(wlc_info_t* wlc, wlc_txh_info_t* txh_info)
{
	uint8 mcs, frametype;
	uint8 *plcp;
	BCM_REFERENCE(wlc);
	ASSERT(txh_info);

	frametype = ltoh16(txh_info->PhyTxControlWord0) & PHY_TXC_FT_MASK;
	ASSERT((frametype == FT_HT) || (frametype == FT_VHT));

	plcp = (uint8 *)(txh_info->plcpPtr);
	if (frametype == FT_HT) {
		mcs = (plcp[0] & MIMO_PLCP_MCS_MASK);
	} else {
		mcs = wlc_vht_get_rate_from_plcp(plcp);
	}
	return mcs;
}
#endif /* WL11N */

bool
wlc_txh_info_is5GHz(wlc_info_t* wlc, wlc_txh_info_t* txh_info)
{
	uint16 mcl;
	bool is5GHz = FALSE;

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		ASSERT(txh_info);
		mcl = ltoh16(txh_info->MacTxControlLow);
		is5GHz = ((mcl & TXC_FREQBAND_5G) == TXC_FREQBAND_5G);
	} else {
		d11actxh_t* vhtHdr = &(txh_info->hdrPtr->txd);
		uint16 cs = (uint16)ltoh16(vhtHdr->PktInfo.Chanspec);
		is5GHz = CHSPEC_IS5G(cs);
	}
	return is5GHz;
}

bool
wlc_txh_has_rts(wlc_info_t* wlc, wlc_txh_info_t* txh_info)
{
	uint16 flag;

	ASSERT(txh_info);
	if (D11REV_LT(wlc->pub->corerev, 40))
		flag = htol16(txh_info->MacTxControlLow) & TXC_SENDRTS;
	else if (D11REV_LT(wlc->pub->corerev, 64))
		flag = htol16(txh_info->hdrPtr->txd.u.rev40.RateInfo[0].RtsCtsControl)
		        & D11AC_RTSCTS_USE_RTS;
	else
		flag = htol16(txh_info->hdrPtr->txd.u.rev64.RateInfo[0].RtsCtsControl)
		        & D11AC_RTSCTS_USE_RTS;

	return (flag ? TRUE : FALSE);
}

void BCMFASTPATH
wlc_txh_set_aging(wlc_info_t *wlc, void *hdr, bool enable)
{
	if (D11REV_LT(wlc->pub->corerev, 40)) {
		d11txh_t* txh = (d11txh_t*)hdr;
		if (!enable)
			txh->MacTxControlLow &= htol16(~TXC_LIFETIME);
		else
			txh->MacTxControlLow |= htol16(TXC_LIFETIME);
	} else {
		d11actxh_t* txh_ac = (d11actxh_t*)hdr;
		if (!enable)
			txh_ac->PktInfo.MacTxControlLow &= htol16(~D11AC_TXC_AGING);
		else
			txh_ac->PktInfo.MacTxControlLow |= htol16(D11AC_TXC_AGING);
	}
}

/**
 * p2p normal/excursion queue mechanism related. Attaches a software queue to all hardware d11 FIFOs
 */
static void
wlc_attach_queue(wlc_info_t *wlc, wlc_txq_info_t *qi)
{
	wlc_txq_info_t *cur_active_queue = wlc->active_queue;

	if (wlc->active_queue == qi)
		return;

	WL_MQ(("MQ: %s: qi %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(qi)));

	wlc->active_queue = qi;

	/* No need to process further (TX FIFO sync, etc.,) if WLC is down */
	if (!wlc->clk)
		return;

	/* set a flag indicating that wlc_tx_fifo_attach_complete() needs to be called */
	wlc->txfifo_attach_pending = TRUE;

	if (cur_active_queue != NULL) {
		wlc_detach_queue(wlc, cur_active_queue);
	}

	/* If the detach is is pending at this point, wlc_tx_fifo_attach_complete() will
	 * be called when the detach completes in wlc_tx_fifo_sync_complete().
	 */

	/* If the detach is not pending (done or call was skipped just above), then see if
	 * wlc_tx_fifo_attach_complete() was called. If wlc_tx_fifo_attach_complete() was not
	 * called (wlc->txfifo_attach_pending), then call it now.
	 */
	if (!wlc->txfifo_detach_pending &&
	    wlc->txfifo_attach_pending) {
		wlc_tx_fifo_attach_complete(wlc);
	}
} /* wlc_attach_queue */

/**
 * p2p/normal/excursion queue mechanism related. Detaches the caller supplied software queue 'qi'
 * from all hardware d11 FIFOs.
 */
static void
wlc_detach_queue(wlc_info_t *wlc, wlc_txq_info_t *qi)
{
	uint fifo_bitmap = BITMAP_SYNC_ALL_TX_FIFOS;
	uint fbmp;
	uint txpktpendtot = 0; /* packets that driver/firmware handed over to d11 DMA */
	uint i;

	ASSERT(wlc->active_queue != NULL);

	WL_MQ(("MQ: %s: qi %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(qi)));

	/* no hardware to sync if we are down */
	if (!wlc->pub->up) {
		WL_MQ(("MQ: %s: !up, so bailing early. TXPEND = %d\n",
		__FUNCTION__, TXPKTPENDTOT(wlc)));
		ASSERT(TXPKTPENDTOT(wlc) == 0);
		return;
	}

	/* If there are pending packets on the fifo, then stop the fifo
	 * processing and re-enqueue packets
	 */
	for (i = 0, fbmp = fifo_bitmap; fbmp; i++, fbmp = fbmp >> 1) {
		if ((fbmp & 0x01) == 0)
			continue;
		txpktpendtot += TXPKTPENDGET(wlc, i);
	}

	if ((txpktpendtot > 0) && (!wlc->txfifo_detach_pending)) {
		/* Need this for split driver */
		wlc->txfifo_detach_pending = TRUE;

		/* save the fifo that is being detached as the destination to re-enqueue
		 * packets from the txfifos if no previous detach pending
		 */
		wlc->txfifo_detach_transition_queue = qi;

		/* Do not allow any new packets to flow to the fifo from the new active_queue
		 * while we are synchronizing the fifo for the detached queue.
		 * The wlc_bmac_tx_fifo_sync() process started below will trigger tx status
		 * processing that could trigger new pkt enqueues to the fifo.
		 * The 'hold' call will flow control the fifo.
		 */
		for (i = 0; i < NFIFO; i++) {
			txq_hw_hold_set(wlc->active_queue->low_txq, i);
		}

		/* flush the fifos and process txstatus from packets that were sent before the flush
		 * wlc_tx_fifo_sync_complete() will be called when all transmitted
		 * packets' txstatus have been processed. The call may be done
		 * before wlc_bmac_tx_fifo_sync() returns, or after in a split driver.
		 */
		wlc_bmac_tx_fifo_sync(wlc->hw, fifo_bitmap, SYNCFIFO);
	} else {
		if (TXPKTPENDTOT(wlc) == 0) {
			WL_MQ(("MQ: %s: skipping FIFO SYNC since TXPEND = 0\n", __FUNCTION__));
		} else {
			WL_MQ(("MQ: %s: skipping FIFO SYNC since detach already pending\n",
			__FUNCTION__));
		}

		WL_MQ(("MQ: %s: DMA pend 0:%d 1:%d 2:%d 3:%d 4:%d 5:%d \n", __FUNCTION__,
		       WLC_HW_DI(wlc, 0) ? dma_txpending(WLC_HW_DI(wlc, 0)) : 0,
		       WLC_HW_DI(wlc, 1) ? dma_txpending(WLC_HW_DI(wlc, 1)) : 0,
		       WLC_HW_DI(wlc, 2) ? dma_txpending(WLC_HW_DI(wlc, 2)) : 0,
		       WLC_HW_DI(wlc, 3) ? dma_txpending(WLC_HW_DI(wlc, 3)) : 0,
		       WLC_HW_DI(wlc, 4) ? dma_txpending(WLC_HW_DI(wlc, 4)) : 0,
		       WLC_HW_DI(wlc, 5) ? dma_txpending(WLC_HW_DI(wlc, 5)) : 0));
	}
} /* wlc_detach_queue */

/**
 * Start or continue an excursion from the currently active tx queue context. Switch to the
 * dedicated excursion queue. During excursions from the primary channel, transfers from the txq to
 * the DMA ring are suspended, and the FIFO processing in the d11 core by ucode is halted for all
 * but the VI AC FIFO (Voice and 802.11 control/management). During the excursion the VI FIFO may be
 * used for outgoing frames (e.g. probe requests). When the driver returns to the primary channel,
 * the FIFOs are re-enabled and the txq processing is resumed.
 */
void
wlc_excursion_start(wlc_info_t *wlc)
{
	if (wlc->excursion_active) {
		WL_MQ(("MQ: %s: already active, exiting\n", __FUNCTION__));
		return;
	}

	wlc->excursion_active = TRUE;

	if (wlc->excursion_queue == wlc->active_queue) {
		WL_MQ(("MQ: %s: same queue %p\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(wlc->excursion_queue)));
		return;
	}

	WL_MQ(("MQ: %s: %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(wlc->excursion_queue)));

	/* if we are not in an excursion, the active_queue should be the primary_queue */
	ASSERT(wlc->primary_queue == wlc->active_queue || wlc->primary_queue == NULL);

	wlc_attach_queue(wlc, wlc->excursion_queue);
}

/**
 * Terminate the excursion from the active tx queue context. Switch from the excursion queue back to
 * the current primary_queue.
 */
void
wlc_excursion_end(wlc_info_t *wlc)
{
	if (!wlc->excursion_active) {
		WL_MQ(("MQ: %s: not in active excursion, exiting\n", __FUNCTION__));
		return;
	}

	WL_MQ(("MQ: %s: %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(wlc->primary_queue)));

	wlc->excursion_active = FALSE;

	if (wlc->primary_queue) {
		wlc_attach_queue(wlc, wlc->primary_queue);
	}
}

/* Switch from the excursion queue to the given queue during excursion. */
void
wlc_active_queue_set(wlc_info_t *wlc, wlc_txq_info_t *new_active_queue)
{
	wlc_txq_info_t *old_active_queue = wlc->active_queue;

	WL_MQ(("MQ: %s: qi %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(new_active_queue)));

	ASSERT(new_active_queue != NULL);
	if (new_active_queue == old_active_queue)
		return;

	wlc_attach_queue(wlc, new_active_queue);
	if (old_active_queue != wlc->excursion_queue)
		return;

	/* Packets being sent during an excursion should only be valid
	 * for the duration of that excursion.  If any packets are still
	 * in the queue at the end of excursion should be flushed.
	 */
	wlc_txq_pktq_flush(wlc, &old_active_queue->txq);
}

/**
 * Use the given queue as the new primary queue wlc->primary_queue is updated, detaching the former
 * primary_queue from the fifos if necessary
 */
void
wlc_primary_queue_set(wlc_info_t *wlc, wlc_txq_info_t *new_primary)
{
	WL_MQ(("MQ: %s: qi %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(new_primary)));

#ifdef RTS_PER_ITF
	/* Update the RTS/CTS information of the per-interface stats
	 * of the current primary queue.
	 */
	wlc_statsupd(wlc);
#endif /* RTS_PER_ITF */

	wlc->primary_queue = new_primary;

	/* if an excursion is active the active_queue should remain on the
	 * excursion_queue. At the end of the excursion, the new primary_queue
	 * will be made active.
	 */
	if (wlc->excursion_active)
		return;

	wlc_attach_queue(wlc, new_primary? new_primary : wlc->excursion_queue);
}

#ifdef WL_MU_TX
int
wlc_tx_fifo_hold_set(wlc_info_t *wlc, uint fifo_bitmap)
{
	uint i, fbmp;
	uint txpktpendtot = 0;

	if ((wlc->txfifo_detach_pending) || (wlc->excursion_active)) {
		return BCME_NOTREADY;
	}

	for (i = 0, fbmp = fifo_bitmap; fbmp; i++, fbmp = fbmp >> 1) {
		if ((fbmp & 0x01) == 0)
			continue;
		txpktpendtot += TXPKTPENDGET(wlc, i);

		/* Do not allow any new packets to flow to the fifo from the active_queue
		 * while we are synchronizing the fifo for the active_queue.
		 * The wlc_bmac_tx_fifo_sync() process started below will trigger tx status
		 * processing that could trigger new pkt enqueues to the fifo.
		 * The 'hold' call will flow control the fifo.
		 */
		txq_hw_hold_set(wlc->active_queue->low_txq, i);
	}

	/* flush the fifos and process txstatus from packets that
	 * were sent before the flush
	 * wlc_tx_fifo_sync_complete() will be called when all transmitted
	 * packets' txstatus have been processed. The call may be done
	 * before wlc_bmac_tx_fifo_sync() returns, or after in a split driver.
	 */
	if (txpktpendtot) {
		wlc->txfifo_detach_pending = TRUE;
		wlc->txfifo_detach_transition_queue = wlc->active_queue;
		wlc_bmac_tx_fifo_sync(wlc->hw, fifo_bitmap, FLUSHFIFO);
	}

	return BCME_OK;
}

void
wlc_tx_fifo_hold_clr(wlc_info_t *wlc, uint fifo_bitmap)
{
	uint i, fbmp;

	for (i = 0, fbmp = fifo_bitmap; fbmp; i++, fbmp = fbmp >> 1) {
		if ((fbmp & 0x01) == 0)
			continue;
		/* Clear the hw fifo hold since we are done with the MU Client scheduler. */
		txq_hw_hold_clr(wlc->active_queue->low_txq, i);
	}
}
#endif /* WL_MU_TX */

static void
wlc_txq_freed_pkt_time(wlc_info_t *wlc, void *pkt, uint16 *time_adj)
{
	ASSERT(time_adj != NULL);
#if defined(TXQ_MUX)
	/* Account for the buffered time we are eliminating */
	*time_adj += WLPKTTIME(pkt);
#else
#ifdef WLAMPDU
	/* For SW AMPDU, only the last MPDU in an AMPDU counts
	 * as txpkt_weight.
	 * Otherwise, 1 for each packet.
	 *
	 * SW AMPDU is only relavant for d11 rev < 40, non-AQM.
	 */
	if (WLPKTFLAG_AMPDU(WLPKTTAG(pkt)) &&
		AMPDU_HOST_ENAB(wlc->pub)) {
		uint16 count;
		wlc_txh_info_t txh_info;
		uint16 mpdu_type;

		wlc_get_txh_info(wlc, pkt, &txh_info);
		mpdu_type = (((ltoh16(txh_info.MacTxControlLow)) &
			TXC_AMPDU_MASK) >> TXC_AMPDU_SHIFT);

		if (mpdu_type == TXC_AMPDU_NONE) {
			/* regular MPDUs just count as 1 */
			count = 1;
		} else if (mpdu_type != TXC_AMPDU_LAST) {
			/* non-last MPDUs are counted as zero */
			count = 0;
		} else {
			/* the last MPDU in an AMPDU has a count that
			 * equals the txpkt_weight
			 */
			count = wlc_ampdu_get_txpkt_weight(wlc->ampdu_tx);
		}
		*time_adj += count;
	}
	else
#endif /* WLAMPDU */
		*time_adj += 1;
#endif /* TXQ_MUX */
}

static void
wlc_txq_free_pkt(wlc_info_t *wlc, void *pkt, uint16 *time_adj)
{
	wlc_txq_freed_pkt_time(wlc, pkt, time_adj);
	PKTFREE(wlc->osh, pkt, TRUE);
}

static struct scb*
wlc_recover_pkt_scb(wlc_info_t *wlc, void *pkt)
{
	struct scb *scb;
	wlc_bsscfg_t *bsscfg;
	wlc_txh_info_t txh_info;

	wlc_get_txh_info(wlc, pkt, &txh_info);


	scb = WLPKTTAG(pkt)->_scb;

	/* check to see if the SCB is just one of the permanent hwrs_scbs */
	if (scb == wlc->band->hwrs_scb ||
	    (NBANDS(wlc) > 1 && scb == wlc->bandstate[OTHERBANDUNIT(wlc)]->hwrs_scb)) {
		WL_MQ(("MQ: %s: recovering %p hwrs_scb\n",
		       __FUNCTION__, OSL_OBFUSCATE_BUF(pkt)));
		return scb;
	}

	/*  use the bsscfg index in the packet tag to find out which
	 *  bsscfg this packet belongs to
	 */
	bsscfg = wlc->bsscfg[WLPKTTAGBSSCFGGET(pkt)];

	/* unicast pkts have been seen with null bsscfg here */
	ASSERT(bsscfg || !ETHER_ISMULTI(txh_info.TxFrameRA));
	if (bsscfg == NULL) {
		WL_MQ(("MQ: %s: pkt cfg idx %d no longer exists\n",
			__FUNCTION__, WLPKTTAGBSSCFGGET(pkt)));
		scb = NULL;
	} else if (ETHER_ISMULTI(txh_info.TxFrameRA)) {
		scb = WLC_BCMCSCB_GET(wlc, bsscfg);
		WL_MQ(("MQ: %s: recovering %p bcmc scb\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(pkt)));
	} else {
		uint bandindex = (wlc_txh_info_is5GHz(wlc, &txh_info))?BAND_5G_INDEX:BAND_2G_INDEX;
		scb = wlc_scbfindband(wlc, bsscfg, (struct ether_addr*)txh_info.TxFrameRA,
		                      bandindex);

#if defined(BCMDBG)
		if (scb != NULL) {
			char eabuf[ETHER_ADDR_STR_LEN];
			bcm_ether_ntoa(&scb->ea, eabuf);
			WL_MQ(("MQ: %s: recovering %p scb %p:%s\n", __FUNCTION__,
			       OSL_OBFUSCATE_BUF(pkt), OSL_OBFUSCATE_BUF(scb), eabuf));
		} else {
			WL_MQ(("MQ: %s: failed recovery scb for pkt %p\n",
				__FUNCTION__, OSL_OBFUSCATE_BUF(pkt)));
		}
#endif
	}

	WLPKTTAGSCBSET(pkt, scb);

	return scb;
}

/* Suppress host generated packet and flush internal once,
 * Returns TRUE if its host pkt else FALSE,
 * Also note packet will be freed in this function.
 */
bool
wlc_suppress_flush_pkts(wlc_info_t *wlc, void *p,  uint16 *time_adj)
{
	struct scb *scb;
	bool ret = FALSE;
	scb = WLPKTTAGSCBGET(p);
	BCM_REFERENCE(scb);
#ifdef PROP_TXSTATUS
	if (PROP_TXSTATUS_ENAB(wlc->pub) &&
		((WL_TXSTATUS_GET_FLAGS(WLPKTTAG(p)->wl_hdr_information) &
		WLFC_PKTFLAG_PKTFROMHOST))) {
		wlc_suppress_sync_fsm(wlc, scb, p, TRUE);
		wlc_process_wlhdr_txstatus(wlc, WLFC_CTL_PKTFLAG_WLSUPPRESS, p, FALSE);
		ret = TRUE;
	}
#endif /* PROP_TXSTATUS */
	wlc_txq_free_pkt(wlc, p, time_adj);
	return ret;
}

#ifdef PROP_TXSTATUS
/**
 * Additional processing for full dongle proptxstatus. Packets are freed unless
 * they are retried. NEW_TXQ specific function.
 */
static void *
wlc_proptxstatus_process_pkt(wlc_info_t *wlc, void *pkt, uint16 *buffered)
{
	if (PROP_TXSTATUS_ENAB(wlc->pub) && ((wlc->excursion_active == FALSE) ||
		MCHAN_ACTIVE(wlc->pub))) {
		struct scb *scb_pkt = WLPKTTAGSCBGET(pkt);

		if (!(scb_pkt && SCB_ISMULTI(scb_pkt))) {
			wlc_suppress_sync_fsm(wlc, scb_pkt, pkt, TRUE);
			wlc_process_wlhdr_txstatus(wlc,
				WLFC_CTL_PKTFLAG_WLSUPPRESS, pkt, FALSE);
		}
		if (wlc_should_retry_suppressed_pkt(wlc, pkt, TX_STATUS_SUPR_PMQ) ||
			(scb_pkt && SCB_ISMULTI(scb_pkt))) {
			/* These are dongle generated pkts
			 * don't free and attempt requeue to interface queue
			 */
			return pkt;
		} else {
			wlc_txq_free_pkt(wlc, pkt, buffered);
			return NULL;
		}
	} else {
		return pkt;
	}
} /* wlc_proptxstatus_process_pkt */
#endif /* PROP_TXSTATUS */

#ifdef BCMDBG
/** WL_MQ() debugging output helper function */
static void
wlc_txpend_counts(wlc_info_t *wlc, const char* fn)
{
	int i;
	struct {
		uint txp;
		uint pend;
	} counts[NFIFO] = {{0, 0}};

	if (!WL_MQ_ON()) {
		return;
	}

	for  (i = 0; i < NFIFO; i++) {
		hnddma_t *di = WLC_HW_DI(wlc, i);

		if (di != NULL) {
			counts[i].txp = dma_txp(di);
			counts[i].pend = TXPKTPENDGET(wlc, i);
		}
	}

	WL_MQ(("MQ: %s: TXPKTPEND/DMA "
	       "[0]:(%d,%d) [1]:(%d,%d) [2]:(%d,%d) [3]:(%d,%d) [4]:(%d,%d) [5]:(%d,%d)\n",
	       fn,
	       counts[0].pend, counts[0].txp,
	       counts[1].pend, counts[1].txp,
	       counts[2].pend, counts[2].txp,
	       counts[3].pend, counts[3].txp,
	       counts[4].pend, counts[4].txp,
	       counts[5].pend, counts[5].txp));
}
#else /* BCMDBG */
#define wlc_txpend_counts(w, fn)
#endif /* BCMDBG */

static void
wlc_low_txq_account(wlc_info_t *wlc, txq_t *low_txq, uint prec,
	uint8 flag, uint16* time_adj)
{
	void *pkt;
	struct spktq *swq;
	struct spktq pkt_list;
#if defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS)
	uint8 flipEpoch = 0;
	uint8 lastEpoch = HEAD_PKT_FLUSHED;
#endif /* defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS) */

	spktqinit(&pkt_list, PKTQ_LEN_MAX);

	swq = &low_txq->swq[prec];
	while ((pkt = spktq_deq(swq))) {
		if (!wlc_recover_pkt_scb(wlc, pkt)) {
#if defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS)
			flipEpoch |= TXQ_PKT_DEL;
#endif /* defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS) */
			wlc_txq_free_pkt(wlc, pkt, time_adj);
			continue;
		}

		/* SWWLAN-39516: Removing WLF3_TXQ_SHORT_LIFETIME optimization.
		 * Could be addressed by pkt lifetime?
		 */
		if (WLPKTTAG(pkt)->flags3 & WLF3_TXQ_SHORT_LIFETIME) {
			WL_INFORM(("MQ: %s: cancel TxQ short-lived pkt %p"
				" during chsw...\n",
				__FUNCTION__, OSL_OBFUSCATE_BUF(pkt)));
#if defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS)
			flipEpoch |= TXQ_PKT_DEL;
#endif /* defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS) */

			wlc_txq_free_pkt(wlc, pkt, time_adj);
			continue;
		}

		if (flag == SYNCFIFO) {
#ifdef PROP_TXSTATUS
			if (wlc_proptxstatus_process_pkt(wlc, pkt, time_adj) == NULL) {
#ifdef WLAMPDU_MAC
				flipEpoch |= TXQ_PKT_DEL;
				continue;
#endif /* WLAMPDU_MAC */

			} else {
				/* not a host packet or pktfree not allowed
				* enqueue it back
				*/
#ifdef WLAMPDU_MAC
				wlc_epoch_upd(wlc, pkt, &flipEpoch, &lastEpoch);
				/* clear pkt delete condition */
				flipEpoch &= ~TXQ_PKT_DEL;
#endif /* WLAMPDU_MAC */

				spktenq(&pkt_list, pkt);
			}
#else
			spktenq(&pkt_list, pkt);
#endif /* PROP_TXSTATUS */
		}
#ifdef PROP_TXSTATUS
		else if (flag == FLUSHFIFO_FLUSHID) {
			if (wlc->fifoflush_id == PKTFRAGFLOWRINGID(wlc->osh, pkt)) {
#if defined(WLAMPDU_MAC)
				flipEpoch |= TXQ_PKT_DEL;
#endif /* defined(WLAMPDU_MAC) */
				wlc_txq_free_pkt(wlc, pkt, time_adj);
				continue;
			}
			else {
				/* sync the remaining packets */
				if (wlc_proptxstatus_process_pkt(wlc, pkt, time_adj) == NULL) {
					/* epcho flip ? */
#if defined(WLAMPDU_MAC)
					flipEpoch |= TXQ_PKT_DEL;
#endif /* defined(WLAMPDU_MAC) */
				} else {
					/* not a host packet or pktfree not allowed
					* enqueue it back
					*/
#ifdef WLAMPDU_MAC
					wlc_epoch_upd(wlc, pkt, &flipEpoch, &lastEpoch);
					/* clear pkt delete condition */
					flipEpoch &= ~TXQ_PKT_DEL;
#endif /* WLAMPDU_MAC */
					spktenq(&pkt_list, pkt);
				}
			}
		}
#endif /* PROP_TXSTATUS */
#ifdef PROP_TXSTATUS
		else if (flag == SUPPRESS_FLUSH_FIFO) {
			if (wlc_suppress_flush_pkts(wlc, pkt, time_adj)) {
#ifdef WLAMPDU_MAC
				flipEpoch |= TXQ_PKT_DEL;
#endif
			}
			continue;
		}
#endif /* PROP_TXSTATUS */
		/* Add new flag like FLUSHFIFO_FLUSHID
		 * above this else block in an else-if {}
		 */
		else {
			/* FLUSHFIFO should be fallback mode of operation
			* FOR ASSERT builds, FW must TRAP if incorrect flag
			* for sync/flush is passed. For non-ASSERT builds,
			* it's perhaps best to just FLUSH the packets.
			*/
			ASSERT(flag == FLUSHFIFO);
			wlc_txq_free_pkt(wlc, pkt, time_adj);
		}
	}
	spktq_prepend(swq, &pkt_list);
	spktqdeinit(&pkt_list);
}

#ifdef WLAMPDU_MAC
/* Update and save epoch value of the last packet in the queue.
 * Epoch value shall be saved for the queue that is getting dettached,
 * Same shall be restored upon "re-attach" of the given queue in wlc_tx_fifo_attach_complete.
 */
static void
wlc_tx_fifo_epoch_save(wlc_info_t *wlc, struct spktq *swq, int prec)
{
	ASSERT(wlc->txfifo_detach_transition_queue != NULL);
	ASSERT(swq != NULL);

	if (wlc->txfifo_detach_transition_queue && swq && prec < AC_COUNT) {
		void *pkt;

		if ((pkt = swq->q.tail)) {
			wlc_txh_info_t txh_info;
			wlc_get_txh_info(wlc, pkt, &txh_info);
			wlc->txfifo_detach_transition_queue->epoch[prec] =
				wlc_txh_get_epoch(wlc, &txh_info);
		}
	}
}
#endif /* WLAMPDU_MAC */

#if defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS)
/* After deletion of packet to flip epoch bit of next enque packets in low_txq if last save epoch
 * value equal to next enque packet epoch value otherwise don't flip epoch value of enqueue
 * packets.
 * flipEpoch 0th bit is used to keep track of last packet state i.e deleted or enq.
 * flipEpoch 1st bit is uesed to keep track of flip epoch state.
*/

void
wlc_epoch_upd(wlc_info_t *wlc, void *pkt, uint8 *flipEpoch, uint8 *lastEpoch)
{
	wlc_txh_info_t txh_info;
	wlc_get_txh_info(wlc, pkt, &txh_info);
	uint8* tsoHdrPtr = txh_info.tsoHdrPtr;

	/* if last packet was deleted then storing the epoch flip state in 1st bit of flipEpoch */
	if ((*flipEpoch & TXQ_PKT_DEL) && (*lastEpoch != HEAD_PKT_FLUSHED)) {
		if ((*lastEpoch & TOE_F2_EPOCH) == (tsoHdrPtr[2] & TOE_F2_EPOCH))
			*flipEpoch |= TOE_F2_EPOCH;
		else
			*flipEpoch &= ~TOE_F2_EPOCH;
	}

	/* flipping epoch bit of packet if flip epoch state bit (1st bit) is set */
	if (*flipEpoch & TOE_F2_EPOCH)
		tsoHdrPtr[2] ^= TOE_F2_EPOCH;
	/* storing current epoch bit */
	*lastEpoch = (tsoHdrPtr[2] & TOE_F2_EPOCH);
}
#endif /* defined(WLAMPDU_MAC) && defined(PROP_TXSTATUS) */


/**
 * called by BMAC, related to new ucode method of suspend & flushing d11 core tx
 * fifos.
 */
void
wlc_tx_fifo_sync_complete(wlc_info_t *wlc, uint fifo_bitmap, uint8 flag)
{
	struct spktq pkt_list;
	void *pkt;
	txq_t *low_txq;
	uint i;
	uint fbmp;
	uint16 time_adj;

	/* check for up first? If we down the driver, maybe we will
	 * get and ignore this call since we flush all txfifo pkts,
	 * and wlc->txfifo_detach_pending would be false.
	 */
	ASSERT(wlc->txfifo_detach_pending);
	ASSERT(wlc->txfifo_detach_transition_queue != NULL);

	WL_MQ(("MQ: %s: TXPENDTOT = %d\n", __FUNCTION__, TXPKTPENDTOT(wlc)));
	wlc_txpend_counts(wlc, __FUNCTION__);

	/* init a local pkt queue to shuffle pkts in this fn */
	spktqinit(&pkt_list, PKTQ_LEN_MAX);

	/* get the destination queue */
	low_txq = wlc->txfifo_detach_transition_queue->low_txq;

	/* pull all the packets that were queued to HW or RPC layer,
	 * and push them on the low software TxQ
	 */
	for (i = 0, fbmp = fifo_bitmap; fbmp; i++, fbmp = fbmp >> 1) {
		if ((fbmp & 0x01) == 0) /* not the right fifo to process */
			continue;
		time_adj = 0;
		if (!PIO_ENAB((wlc)->pub)) {
			hnddma_t *di = WLC_HW_DI(wlc, i);

			if (di == NULL)
				continue;

			while (NULL != (pkt = wlc_bmac_dma_getnexttxp(wlc, i,
					HNDDMA_RANGE_ALL))) {
#ifdef WLC_MACDBG_FRAMEID_TRACE
				wlc_macdbg_frameid_trace_sync(wlc->macdbg, pkt);
#endif
				spktenq(&pkt_list, pkt);
			}
			/* The DMA should have been cleared of all packets by the
			 * wlc_bmac_dma_getnexttxp() loop above.
			 */
			ASSERT(dma_txactive(di) == 0);
		} else { /* if !PIO */
			pio_t *pio = WLC_HW_PIO(wlc, i);

			if (pio == NULL)
				continue;

			while (NULL != (pkt = wlc_pio_getnexttxp(pio))) {
				spktenq(&pkt_list, pkt);
			}
		} /* if !PIO */

		WL_MQ(("MQ: %s: fifo %d: collected %d pkts\n", __FUNCTION__,
		       i, spktq_n_pkts(&pkt_list)));

		/* enqueue the collected pkts from DMA ring
		* at the head of the low software TxQ
		*/
		if (spktq_n_pkts(&pkt_list) > 0) {
			spktq_prepend(&low_txq->swq[i], &pkt_list);
		}

		/*
		* Account for the collective packets in low_txq at this stage.
		* Need to suppress or flush all packets currently in low_txq
		* depending on the argument 'flag'
		*/
		wlc_low_txq_account(wlc, low_txq, i, flag, &time_adj);

		if ((TXPKTPENDGET(wlc, i) > 0) && (time_adj > 0)) {
#if defined(TXQ_MUX)
			int pkt_adj;

			/* Calculate how many pkts were removed in wlc_low_txq_account().
			 * time_adj already has the time contribution of the revoved frames.
			 * The count is just the difference of previous TXPKTPEND count and
			 * what is now in the low_txq. All pkts from the DMA have been pulled
			 * back to the low_txq.
			 */
			pkt_adj = TXPKTPENDGET(wlc, i) - spktq_n_pkts(&low_txq->swq[i]);

			ASSERT(pkt_adj >= 0);

			WLC_TXFIFO_COMPLETE(wlc, i, (uint16)pkt_adj, time_adj);
#else
			/* The time_adj is the weighted pkt count for TXPKTPEND. Normally this
			 * is just a count of pkts, but for SW AMPDU, an entire AMPDU of pkts
			 * is a weighted value, txpkt_weight, typically 2.
			 * The time value parameter for WLC_TXFIFO_COMPLETE() is ignored for
			 * non-TXQ_MUX.
			 */
			WLC_TXFIFO_COMPLETE(wlc, i, time_adj, 0);
#endif /* TXQ_MUX */
		}

		spktq_deinit(&pkt_list);

#ifdef WLAMPDU_MAC
		/*
		 * Make a backup of epoch bit settings before completing queue switch.
		 * Packets on the dma ring have already been accounted for at this point and
		 * low_q packet state is finalized. Now find and save appropriate epoch bit
		 * setting which shall be restored when switching back to this queue.
		 *
		 * For multi-destination traffic case, each destination traffic packet stream
		 * would be differentiated from each other by an epoch flip which would have
		 * already been well handled at this point. We are only concerned about the
		 * final packet in order to determine the appropriate epoch state of the FIFO,
		 * for use from the next pkt on wards.
		 *
		 * Note : It would not be handling the selective TID
		 * flush cases (FLUSHFIFO_FLUSHID)
		 * TBD :
		 * 1. Epoch save and restore for NON -AQM cases (SWAMPDU and HWAMPDU)
		 * 2. Handle selective TID flushes (FLUSHFIFO_FLUSHID)
		*/
		if (AMPDU_AQM_ENAB(wlc->pub)) {
			wlc_tx_fifo_epoch_save(wlc, &low_txq->swq[i], i);
		}
#endif /* WLAMPDU_MAC */

		/* clear any HW 'stopped' flags since the hw fifo is now empty */
		txq_hw_stop_clr(low_txq, i);
	} /* end for */

	if (IS_EXCURSION_QUEUE(low_txq, wlc->txqi)) {
		/* Packets being sent during an excursion should only be valid
		 * for the duration of that excursion.  If any packets are still
		 * in the queue at the end of excursion should be flushed.
		 * Flush both the high and low txq.
		 */
		wlc_txq_pktq_flush(wlc, &wlc->excursion_queue->txq);
		wlc_low_txq_flush(wlc->txqi, low_txq);
	}

	wlc->txfifo_detach_transition_queue = NULL;
	wlc->txfifo_detach_pending = FALSE;

} /* wlc_tx_fifo_sync_complete */

void
wlc_tx_fifo_attach_complete(wlc_info_t *wlc)
{
	int i;
	int new_count;
	txq_t *low_q;
#ifdef BCMDBG
	struct {
		uint pend;
		uint pkt;
		uint txp;
		uint buffered;
	} counts [NFIFO] = {{0, 0, 0, 0}};
	hnddma_t *di;
#endif /* BCMDBG */

	wlc->txfifo_attach_pending = FALSE;

	/* bail out if no new active_queue */
	if (wlc->active_queue == NULL) {
		return;
	}

	low_q = wlc->active_queue->low_txq;

	/* for each HW fifo, set the new pending count when we are attaching a new queue */
	for  (i = 0; i < NFIFO; i++) {
		TXPKTPENDCLR(wlc, i);

#if defined(TXQ_MUX)
		/* The TXPKTPEND count will be the number of pkts in the low queue
		 * we are attaching
		 */
		new_count = spktq_n_pkts(&low_q->swq[i]);
#else
		/* The TXPKTPEND count is the number of pkts in the low queue, except for
		 * SW AMPDU. For SW AMPDUs, the TXPKTPEND count is a weighted value, txpkt_weight,
		 * typically 2, for an entire SW AMPDU. The wlc_txq_buffered_time() holds the
		 * weighted TXPKTPEND count we want to use.
		 */
		new_count = wlc_txq_buffered_time(low_q, i);
#endif /* TXQ_MUX */

		/* Initialize per FIFO TXPKTPEND count */
		TXPKTPENDINC(wlc, i, (uint16)new_count);

		/* Clear the hw fifo hold since we are done with the fifo_detach */
		txq_hw_hold_clr(low_q, i);

		/* Process completions from all fifos.
		 * TXFIFO_COMPLETE is called with pktpend count as
		 * zero in order to ensure that FIFO blocks are reset properly
		 * as attach is now complete.
		 */
		WLC_TXFIFO_COMPLETE(wlc, i, 0, 0);

#ifdef BCMDBG
		counts[i].pend = new_count;
		counts[i].pkt = spktq_n_pkts(&low_q->swq[i]);
		counts[i].buffered = wlc_txq_buffered_time(low_q, i);
		/* At the point we are attaching a new queue, the DMA should have been
		 * cleared of all packets
		 */
		if ((di = WLC_HW_DI(wlc, i)) != NULL) {
			ASSERT(dma_txactive(di) == 0);
			counts[i].txp = dma_txp(di);
		}
#endif /* BCMDBG */
	}

#ifdef WLAMPDU_MAC
	if (AMPDU_AQM_ENAB(wlc->pub)) {
		for  (i = 0; i < AC_COUNT; i++) {
			/* restore the epoch bit setting for this queue fifo */
			wlc_ampdu_set_epoch(wlc->ampdu_tx, i, wlc->active_queue->epoch[i]);
		}
	}
#endif /* WLAMPDU_MAC */

	WL_MQ(("MQ: %s: New TXPKTPEND/pkt/time/DMA "
	       "[0]:(%d,%d,%d,%d) [1]:(%d,%d,%d,%d) [2]:(%d,%d,%d,%d) "
	       "[3]:(%d,%d,%d,%d) [4]:(%d,%d,%d,%d) [5]:(%d,%d,%d,%d)\n",
	       __FUNCTION__,
	       counts[0].pend, counts[0].pkt, counts[0].buffered, counts[0].txp,
	       counts[1].pend, counts[1].pkt, counts[0].buffered, counts[1].txp,
	       counts[2].pend, counts[2].pkt, counts[0].buffered, counts[2].txp,
	       counts[3].pend, counts[3].pkt, counts[0].buffered, counts[3].txp,
	       counts[4].pend, counts[4].pkt, counts[0].buffered, counts[4].txp,
	       counts[5].pend, counts[5].pkt, counts[0].buffered, counts[5].txp));

} /* wlc_tx_fifo_attach_complete */

static void
wlc_low_txq_sync(wlc_info_t *wlc, txq_t* low_txq, uint fifo_bitmap,
	uint8 flag)
{
	uint16 time_adj;
	uint i;
	uint fbmp;

	ASSERT(low_txq != NULL);

	for (i = 0, fbmp = fifo_bitmap; fbmp; i++, fbmp = fbmp >> 1) {
		if ((fbmp & 0x01) == 0) /* not the right fifo to process */
			continue;
		time_adj = 0;
		wlc_low_txq_account(wlc, low_txq, i, flag, &time_adj);
		txq_dec(low_txq, i, time_adj);
	}
}

static void
wlc_tx_pktpend_check(wlc_info_t *wlc, uint *fifo_bitmap)
{
	uint fifo;

	for (fifo = 0; fifo < NFIFO; fifo++) {
		if (TXPKTPENDGET(wlc, fifo) == 0) {
			/* Clear bitmap for FIFO which has no pkt pending */
			*fifo_bitmap &= ~(TX_FIFO_BITMAP(fifo));
		}
	}
}

/* standalone fifo/low_txq sync routine */
void
wlc_sync_txfifo(wlc_info_t *wlc, wlc_txq_info_t *qi, uint fifo_bitmap, uint8 flag)
{
	/* protective call to avoid any recursion because of fifo sync */
	if (wlc->txfifo_detach_pending)
		return;

	ASSERT(qi != NULL);

	if (qi != wlc->active_queue) {
		/* sync low txq only, cannot touch the FIFO */
		wlc_low_txq_sync(wlc, qi->low_txq, fifo_bitmap, flag);
	}
	else {
		wlc_tx_pktpend_check(wlc, &fifo_bitmap);
		if (fifo_bitmap) {
			wlc->txfifo_detach_transition_queue = qi;
			wlc->txfifo_detach_pending = TRUE;
			wlc_bmac_tx_fifo_sync(wlc->hw, fifo_bitmap, flag);
		}
	}
}

#ifdef WLAMPDU
bool BCMFASTPATH
wlc_txh_get_isAMPDU(wlc_info_t* wlc, const wlc_txh_info_t* txh_info)
{
	bool isAMPDU = FALSE;
	uint16 mcl;
	ASSERT(wlc);
	ASSERT(txh_info);

	mcl = ltoh16(txh_info->MacTxControlLow);
	if (D11REV_GE(wlc->pub->corerev, 40)) {
		isAMPDU = ((mcl & D11AC_TXC_AMPDU) != 0);
	} else {
		isAMPDU = ((mcl & TXC_AMPDU_MASK) != TXC_AMPDU_NONE);
	}

	return isAMPDU;
}

#ifdef WL11AX
/**
 * Set the frame type value in the given packet's TSO header section.
 * This function is only for AQM hardware (d11 core rev >= 80) headers.
 */
void BCMFASTPATH
wlc_txh_set_ft(wlc_info_t* wlc, uint8* pdata, uint16 ft)
{
	uint8* tsoHdrPtr = pdata;

	ASSERT(D11REV_GE(wlc->pub->corerev, 80));
	ASSERT(pdata != NULL);

	tsoHdrPtr[1] |= ((ft << TOE_F1_FT_SHIFT) & TOE_F1_FT_MASK);
}

/**
 * Set the frame type value in the given packet's TSO header section.
 * This function is only for AQM hardware (d11 core rev >= 80) headers.
 */
void BCMFASTPATH
wlc_txh_set_frag_allow(wlc_info_t* wlc, uint8* pdata, bool frag_allow)
{
	uint8* tsoHdrPtr = pdata;

	ASSERT(D11REV_GE(wlc->pub->corerev, 80));
	ASSERT(pdata != NULL);

	if (frag_allow)
		tsoHdrPtr[1] |= TOE_F1_FRAG_ALLOW;
}
#endif /* WL11AX */

/**
 * Set the epoch value in the given packet data.
 * This function is only for AQM hardware (d11 core rev >= 40) headers.
 *
 * @param wlc           wlc_info_t pointer
 * @param pdata	        pointer to the beginning of the packet data which
 *                      should be the TxD hardware header.
 * @param epoch         the epoch value (0/1) to set
 */
void BCMFASTPATH
wlc_txh_set_epoch(wlc_info_t* wlc, uint8* pdata, uint8 epoch)
{
	uint8* tsoHdrPtr = pdata;

	ASSERT(D11REV_GE(wlc->pub->corerev, 40));
	ASSERT(pdata != NULL);

	if (epoch) {
		tsoHdrPtr[2] |= TOE_F2_EPOCH;
	} else {
		tsoHdrPtr[2] &= ~TOE_F2_EPOCH;
	}

	if (D11REV_LT(wlc->pub->corerev, 42)) {
		uint tsoHdrSize;
		d11actxh_t* vhtHdr;

		tsoHdrSize = ((tsoHdrPtr[0] & TOE_F0_HDRSIZ_NORMAL) ?
		              TSO_HEADER_LENGTH : TSO_HEADER_PASSTHROUGH_LENGTH);

		vhtHdr = (d11actxh_t*)(tsoHdrPtr + tsoHdrSize);

		vhtHdr->PktInfo.TSOInfo = htol16(tsoHdrPtr[2] << 8);
	}
}

/**
 * Get the epoch value from the given packet data.
 * This function is only for AQM hardware (d11 core rev >= 40) headers.
 *
 * @param wlc           wlc_info_t pointer
 * @param pdata	        pointer to the beginning of the packet data which
 *                      should be the TxD hardware header.
 * @return              The fucntion returns the epoch value (0/1) from the frame
 */
uint8
wlc_txh_get_epoch(wlc_info_t* wlc, wlc_txh_info_t* txh_info)
{
	uint8* tsoHdrPtr = txh_info->tsoHdrPtr;

	ASSERT(D11REV_GE(wlc->pub->corerev, 40));
	ASSERT(txh_info->tsoHdrPtr != NULL);

	return ((tsoHdrPtr[2] & TOE_F2_EPOCH)? 1 : 0);
}

/**
* wlc_compute_ampdu_mpdu_dur()
 *
 * Calculate the 802.11 MAC header DUR field for MPDU in an A-AMPDU DUR for MPDU = 1 SIFS + 1 BA
 *
 * rate			MPDU rate in unit of 500kbps
 */
static uint16
wlc_compute_ampdu_mpdu_dur(wlc_info_t *wlc, uint8 bandunit, ratespec_t rate)
{
	uint16 dur = SIFS(bandunit);

	dur += (uint16)wlc_calc_ba_time(wlc->hti, rate, WLC_SHORT_PREAMBLE);

	return (dur);
}
#endif /* WLAMPDU */

#ifdef STA
static void
wlc_pm_tx_upd(wlc_info_t *wlc, struct scb *scb, void *pkt, bool ps0ok, uint fifo)
{
	wlc_bsscfg_t *cfg;
	wlc_pm_st_t *pm;

	WL_RTDC(wlc, "wlc_pm_tx_upd", 0, 0);

	ASSERT(scb != NULL);

	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);

	pm = cfg->pm;

	if (!pm) {
		return;
	}

	WL_NONE(("%s: wme=%d, qos=%d, pm_ena=%d, pm_pend=%d\n", __FUNCTION__,
		WME_ENAB(wlc->pub), SCB_QOS(scb), pm->PMenabled, pm->PMpending));

	if (!pm->PMenabled ||
#ifdef WLTDLS
	    (BSS_TDLS_ENAB(wlc, cfg) && !wlc_tdls_pm_enabled(wlc->tdls, cfg)) ||
#endif
	    (!cfg->BSS && !BSS_TDLS_ENAB(wlc, cfg) && !BSSCFG_IS_AIBSS_PS_ENAB(cfg)))
		return;

	if (BSSCFG_AP(cfg)) {
		WL_PS(("%s: PMEnabled on AP cfg %d\n", __FUNCTION__, WLC_BSSCFG_IDX(cfg)));
		ASSERT(0);
		return;
	}


	/* Turn off PM mode */
	/* Do not leave PS mode on a tx if the Receive Throttle
	 * feature is  enabled. Leaving PS mode during the OFF
	 * part of the receive throttle duty cycle
	 * for any reason will defeat the whole purpose
	 * of the OFF part of the duty cycle - heat reduction.
	 */
	if (!PM2_RCV_DUR_ENAB(cfg)) {
		/* Leave PS mode if in fast PM mode */
		if (pm->PM == PM_FAST &&
		    ps0ok &&
		    !pm->PMblocked) {
			if (!pm->PMpending || pm->pm2_ps0_allowed) {
				WL_RTDC(wlc, "wlc_pm_tx_upd: exit PS", 0, 0);
				wlc_set_pmstate(cfg, FALSE);
#ifdef WL_PWRSTATS
				if (PWRSTATS_ENAB(wlc->pub)) {
					wlc_pwrstats_set_frts_data(wlc->pwrstats, TRUE);
				}
#endif /* WL_PWRSTATS */
				wlc_pm2_sleep_ret_timer_start(cfg, 0);
			} else if (pm->PMpending) {
				pm->pm2_ps0_allowed = TRUE;
			}
		}
	}
#ifdef BCMULP
	/* Handle the ULP Timer Restart for TX packet case - PM1 case  */
	if (BCMULP_ENAB()) {
		if (pm->PM == PM_MAX) {
			wlc_ulp_perform_sleep_ctrl_action(wlc->ulp, ULP_TIMER_START);
		}
	}
#endif /* BCMULP */

#ifdef WME
	/* Start an APSD USP */
	ASSERT(WLPKTTAG(pkt)->flags & WLF_TXHDR);

	/* If sending APSD trigger frame, stay awake until EOSP */
	if (WME_ENAB(wlc->pub) &&
	    SCB_QOS(scb) &&
	    /* APSD trigger is only meaningful in PS mode */
	    pm->PMenabled &&
	    /* APSD trigger must not be any PM transitional frame */
	    !pm->PMpending) {
		struct dot11_header *h;
		wlc_txh_info_t txh_info;
		uint16 kind;
		bool qos;
#ifdef WLTDLS
		uint16 qosctrl;
#endif

		wlc_get_txh_info(wlc, pkt, &txh_info);
		h = txh_info.d11HdrPtr;
		kind = (ltoh16(h->fc) & FC_KIND_MASK);
		qos = (kind  == FC_QOS_DATA || kind == FC_QOS_NULL);

#ifdef WLTDLS
		qosctrl = qos ? ltoh16(*(uint16 *)((uint8 *)h + DOT11_A3_HDR_LEN)) : 0;
#endif
		WL_NONE(("%s: qos=%d, fifo=%d, trig=%d, sta_usp=%d\n", __FUNCTION__,
			qos, fifo, AC_BITMAP_TST(scb->apsd.ac_trig,
			WME_PRIO2AC(PKTPRIO(pkt))), pm->apsd_sta_usp));

		if (qos &&
		    fifo != TX_BCMC_FIFO &&
		    AC_BITMAP_TST(scb->apsd.ac_trig, WME_PRIO2AC(PKTPRIO(pkt))) &&
#ifdef WLTDLS
			(!BSS_TDLS_ENAB(wlc, cfg) || kind != FC_QOS_NULL ||
			!QOS_EOSP(qosctrl)) &&
#endif
			TRUE) {
#ifdef WLTDLS
			if (BSS_TDLS_ENAB(wlc, cfg))
				wlc_tdls_apsd_upd(wlc->tdls, cfg);
#endif
			if (!pm->apsd_sta_usp) {
				WL_PS(("wl%d.%d: APSD wake\n",
				       wlc->pub->unit, WLC_BSSCFG_IDX(cfg)));
				wlc_set_apsd_stausp(cfg, TRUE);
#ifdef WLP2P
				/* APSD trigger frame is being sent, and re-trigger frame
				 * is expected when the next presence period starts unless
				 * U-APSD EOSP is received between now and then.
				 */
				if (BSS_P2P_ENAB(wlc, cfg))
					wlc_p2p_apsd_retrigger_upd(wlc->p2p, cfg, TRUE);
#endif
			}
			scb->flags |= SCB_SENT_APSD_TRIG;
		}
	}
#endif /* WME */
} /* wlc_pm_tx_upd */

/**
 * Given an ethernet packet, prepare the packet as if for transmission. The driver needs to program
 * the frame in the template and also provide d11txh equivalent information to it.
 */
void *
wlc_sdu_to_pdu(wlc_info_t *wlc, void *sdu, struct scb *scb, bool is_8021x)
{
	void *pkt;
	wlc_key_t *key = NULL;
	wlc_key_info_t key_info;
	uint8 prio = 0;
	uint frag_length = 0;
	uint pkt_length, nfrags;
	wlc_bsscfg_t *bsscfg;

	ASSERT(scb != NULL);

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);
	memset(&key_info, 0, sizeof(wlc_key_info_t));

	/* wlc_hdr_proc --> Ether to 802.3 */
	pkt = wlc_hdr_proc(wlc, sdu, scb);

	if (pkt == NULL) {
		PKTFREE(wlc->osh, sdu, TRUE);
		return NULL;
	}

	ASSERT(sdu == pkt);

	prio = 0;
	if (SCB_QOS(scb)) {
		prio = (uint8)PKTPRIO(sdu);
		ASSERT(prio <= MAXPRIO);
	}

	if (WSEC_ENABLED(bsscfg->wsec) && !BSSCFG_SAFEMODE(bsscfg)) {
		/* Use a paired key or primary group key if present */
		key = wlc_keymgmt_get_tx_key(wlc->keymgmt, scb, bsscfg, &key_info);
		if (key_info.algo != CRYPTO_ALGO_OFF) {
			WL_WSEC(("wl%d.%d: %s: using pairwise key\n",
				WLCWLUNIT(wlc), WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
		} else if (is_8021x && WSEC_SES_OW_ENABLED(bsscfg->wsec)) {
			WL_WSEC(("wl%d.%d: wlc_sdu_to_pdu: not encrypting 802.1x frame "
					"during OW\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
		} else {
			key = wlc_keymgmt_get_bss_tx_key(wlc->keymgmt, bsscfg, FALSE, &key_info);
			if (key_info.algo != CRYPTO_ALGO_OFF) {
				WL_WSEC(("wl%d.%d: %s: using group key",
					WLCWLUNIT(wlc), WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
			} else {
				WL_ERROR(("wl%d.%d: %s: no key for encryption\n",
					WLCWLUNIT(wlc), WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
			}
		}
	} else {
		key = wlc_keymgmt_get_bss_key(wlc->keymgmt, bsscfg, WLC_KEY_ID_INVALID, &key_info);
		ASSERT(key != NULL);
	}

	pkt_length = pkttotlen(wlc->osh, pkt);

	/* TKIP MIC space reservation */
	if (key_info.algo == CRYPTO_ALGO_TKIP) {
		pkt_length += TKIP_MIC_SIZE;
		PKTSETLEN(wlc->osh, pkt, pkt_length);
	}

	nfrags = wlc_frag(wlc, scb, wme_fifo2ac[TX_AC_BE_FIFO], pkt_length, &frag_length);
	BCM_REFERENCE(nfrags);
	ASSERT(nfrags == 1);

	/* This header should not be installed to or taken from Header Cache */
	WLPKTTAG(pkt)->flags |= WLF_BYPASS_TXC;

	/* wlc_dofrag --> Get the d11hdr put on it with TKIP MIC at the tail for TKIP */
	wlc_dofrag(wlc, pkt, 0, 1, 0, scb, is_8021x, TX_AC_BE_FIFO, key,
		&key_info, prio, frag_length);

	return pkt;
} /* wlc_sdu_to_pdu */
#endif /* STA */

/** allocates e.g. initial and excursion transmit queues */
wlc_txq_info_t*
wlc_txq_alloc(wlc_info_t *wlc, osl_t *osh)
{
	wlc_txq_info_t *qi, *p;
	int i;
#if defined(TXQ_MUX)
	uint ac;
#else
	uint no_hw_fifos = NFIFO;
#endif /* TXQ_MUX */

	qi = (wlc_txq_info_t*)MALLOCZ(osh, sizeof(wlc_txq_info_t));
	if (qi == NULL) {
		return NULL;
	}

	/* Set the overall queue packet limit to the max, just rely on per-prec limits */
	pktq_init(WLC_GET_TXQ(qi), WLC_PREC_COUNT, PKTQ_LEN_MAX);

	/* Have enough room for control packets along with HI watermark */
	/* Also, add room to txq for total psq packets if all the SCBs leave PS mode */
	/* The watermark for flowcontrol to OS packets will remain the same */
	for (i = 0; i < WLC_PREC_COUNT; i++) {
		pktq_set_max_plen(WLC_GET_TXQ(qi), i,
		                  (2 * wlc->pub->tunables->datahiwat) + PKTQ_LEN_DEFAULT +
		                  wlc->pub->psq_pkts_total);
	}

	/* add this queue to the the global list */
	p = wlc->tx_queues;
	if (p == NULL) {
		wlc->tx_queues = qi;
	} else {
		while (p->next != NULL)
			p = p->next;
		p->next = qi;
	}

	WL_MQ(("MQ: %s: qi %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(qi)));

	/* Note call to wlc_low_txq_alloc() uses txq as supply context if mux is enabled,
	 * wlc otherwise
	 */

#ifdef TXQ_MUX
	/*
	 * Allocate the low txq to accompany this queue context
	 * Allocated for all physical queues in the device
	 *
	 */
	qi->wlc = wlc;

	/* Allocate queues for the 4 data, bcmc and atim FIFOs */
	qi->low_txq = wlc_low_txq_alloc(wlc->txqi, wlc_pull_q, qi, NFIFO,
	                                wlc->pub->tunables->txq_highwater,
	                                wlc->pub->tunables->txq_lowwater);

	if (qi->low_txq == NULL) {
		WL_ERROR(("wl%d: %s: wlc_low_txq_alloc failed\n", wlc->pub->unit, __FUNCTION__));
		wlc_txq_free(wlc, osh, qi);
		return NULL;
	}

	/* Allocate a mux for each FIFO and create our immediate path MUX input
	 * Allocate an immediate queue handler only for the 4 data FIFOs
	 */
	for (ac = 0; ac < NFIFO; ac++) {
		wlc_mux_t *mux = wlc_mux_alloc(wlc->tx_mux, ac, WLC_TX_MUX_QUANTA);
		int err = 0;

		qi->ac_mux[ac] = mux;

		if ((mux != NULL) && (ac < AC_COUNT)) {
			err = wlc_mux_add_source(mux,
			                         qi,
			                         wlc_txq_immediate_output,
			                         &qi->mux_hdl[ac]);

			if (err) {
				WL_ERROR(("wl%d:%s wlc_mux_add_member() failed err = %d\n",
				          wlc->pub->unit, __FUNCTION__, err));
			}
		}

		if (mux == NULL || err) {
			wlc_txq_free(wlc, osh, qi);
			qi = NULL;
			break;
		}
	}
#else
#if defined(BCM_DMA_CT) && !defined(BCM_DMA_CT_DISABLED)
	no_hw_fifos = NFIFO_EXT;
#endif
	/*
	 * Allocate the low txq to accompany this queue context
	 * Allocated for all physical queues in the device
	 */
	qi->low_txq = wlc_low_txq_alloc(wlc->txqi, wlc_pull_q, wlc, no_hw_fifos,
		wlc_get_txmaxpkts(wlc), wlc_get_txmaxpkts(wlc)/2);

	if (qi->low_txq == NULL) {
		WL_ERROR(("wl%d: %s: wlc_low_txq_alloc failed\n", wlc->pub->unit, __FUNCTION__));
		wlc_txq_free(wlc, osh, qi);
		return NULL;
	}
#endif /* TXQ_MUX */

	WL_MQ(("MQ: %s: qi %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(qi)));

	return qi;
} /* wlc_txq_alloc */

void
wlc_txq_free(wlc_info_t *wlc, osl_t *osh, wlc_txq_info_t *qi)
{
	wlc_txq_info_t *p;
#if defined(TXQ_MUX)
	uint ac;
#endif /* TXQ_MUX */

	if (qi == NULL)
		return;

	WL_MQ(("MQ: %s: qi %p\n", __FUNCTION__, OSL_OBFUSCATE_BUF(qi)));

	/* remove the queue from the linked list */
	p = wlc->tx_queues;
	if (p == qi)
		wlc->tx_queues = p->next;
	else {
		while (p != NULL && p->next != qi)
			p = p->next;
		if (p == NULL)
			WL_ERROR(("%s: null ptr2", __FUNCTION__));

		/* assert that we found qi before getting to the end of the list */
		ASSERT(p != NULL);

		if (p != NULL) {
			p->next = p->next->next;
		}
	}

#ifdef TXQ_MUX

	/* free the immediate queue per-AC MUX sources that connect to the MUXs */
	for (ac = 0; ac < ARRAYSIZE(qi->mux_hdl); ac++) {
		mux_source_handle_t mux_src = qi->mux_hdl[ac];

		/* the mux source pointer may be NULL if there was a
		 * malloc failure during wlc_txq_alloc()
		 */
		if (mux_src != NULL) {
			wlc_mux_del_source(qi->ac_mux[ac], mux_src);

			/* clear the pointer to tidy up */
			qi->mux_hdl[ac] = NULL;
		}
	}

	/* free the per-FIFO MUXs */
	for (ac = 0; ac < ARRAYSIZE(qi->ac_mux); ac++) {
		wlc_mux_t *mux = qi->ac_mux[ac];

		/* the mux pointer may be NULL if there was a
		 * malloc failure during wlc_txq_alloc()
		 */
		if (mux != NULL) {
			wlc_mux_free(mux);

			/* clear the pointer to tidy up */
			qi->ac_mux[ac] = NULL;
		}
	}

#endif /* TXQ_MUX */

	/* Free the low_txq accompanying this txq context */
	wlc_low_txq_free(wlc->txqi, qi->low_txq);

#ifdef PKTQ_LOG
	wlc_pktq_stats_free(wlc, WLC_GET_TXQ(qi));
#endif

	ASSERT(pktq_empty(WLC_GET_TXQ(qi)));
	MFREE(osh, qi, sizeof(wlc_txq_info_t));
} /* wlc_txq_free */

static void
wlc_txflowcontrol_signal(wlc_info_t *wlc, wlc_txq_info_t *qi, bool on, int prio)
{
	wlc_if_t *wlcif;
	uint curr_qi_stopped = qi->stopped;

	for (wlcif = wlc->wlcif_list; wlcif != NULL; wlcif = wlcif->next) {
		if (curr_qi_stopped != qi->stopped) {
			/* This tells us that while performing wl_txflowcontrol(),
			 * the qi->stopped state changed.
			 * This can happen when turning wl flowcontrol OFF since in
			 * the process of draining packets from wl layer, flow control
			 * can get turned back on.
			 */
			WL_ERROR(("wl%d: qi(%p) stopped changed from 0x%x to 0x%x, exit %s\n",
			          wlc->pub->unit, OSL_OBFUSCATE_BUF(qi), curr_qi_stopped,
				qi->stopped, __FUNCTION__));
			break;
		}
		if (wlcif->qi == qi &&
		    wlcif->if_flags & WLC_IF_LINKED)
			wl_txflowcontrol(wlc->wl, wlcif->wlif, on, prio);
	}
}

void
wlc_txflowcontrol_reset(wlc_info_t *wlc)
{
	wlc_txq_info_t *qi;

	for (qi = wlc->tx_queues; qi != NULL; qi = qi->next) {
		wlc_txflowcontrol_reset_qi(wlc, qi);
	}
}

/** check for the particular priority flow control bit being set */
bool
wlc_txflowcontrol_prio_isset(wlc_info_t *wlc, wlc_txq_info_t *q, int prio)
{
	uint prio_mask;
	BCM_REFERENCE(wlc);

	if (prio == ALLPRIO) {
		prio_mask = TXQ_STOP_FOR_PRIOFC_MASK;
	} else {
		ASSERT(prio >= 0 && prio <= MAXPRIO);
		prio_mask = NBITVAL(prio);
	}

	return (q->stopped & prio_mask) == prio_mask;
}

/** check if a particular override is set for a queue */
bool
wlc_txflowcontrol_override_isset(wlc_info_t *wlc, wlc_txq_info_t *qi, uint override)
{
	BCM_REFERENCE(wlc);
	/* override should have some bit on */
	ASSERT(override != 0);
	/* override should not have a prio bit on */
	ASSERT((override & TXQ_STOP_FOR_PRIOFC_MASK) == 0);

	return ((qi->stopped & override) != 0);
}

/** propagate the flow control to all interfaces using the given tx queue */
void
wlc_txflowcontrol(wlc_info_t *wlc, wlc_txq_info_t *qi, bool on, int prio)
{
	uint prio_bits;
	uint cur_bits;

	if (prio == ALLPRIO) {
		prio_bits = TXQ_STOP_FOR_PRIOFC_MASK;
	} else {
		ASSERT(prio >= 0 && prio <= MAXPRIO);
		prio_bits = NBITVAL(prio);
	}

	cur_bits = qi->stopped & prio_bits;

	/* Check for the case of no change and return early
	 * Otherwise update the bit and continue
	 */
	if (on) {
		if (cur_bits == prio_bits) {
			return;
		}
		mboolset(qi->stopped, prio_bits);
	} else {
		if (cur_bits == 0) {
			return;
		}
		mboolclr(qi->stopped, prio_bits);
	}

	/* If there is a flow control override we will not change the external
	 * flow control state.
	 */
	if (qi->stopped & ~TXQ_STOP_FOR_PRIOFC_MASK) {
		return;
	}

	wlc_txflowcontrol_signal(wlc, qi, on, prio);
} /* wlc_txflowcontrol */

/** called in e.g. association and AMPDU connection setup scenario's */
void
wlc_txflowcontrol_override(wlc_info_t *wlc, wlc_txq_info_t *qi, bool on, uint override)
{
	uint prev_override;

	ASSERT(override != 0);
	ASSERT((override & TXQ_STOP_FOR_PRIOFC_MASK) == 0);
	ASSERT(qi != NULL);

	prev_override = (qi->stopped & ~TXQ_STOP_FOR_PRIOFC_MASK);

	/* Update the flow control bits and do an early return if there is
	 * no change in the external flow control state.
	 */
	if (on) {
		mboolset(qi->stopped, override);
		/* if there was a previous override bit on, then setting this
		 * makes no difference.
		 */
		if (prev_override) {
			return;
		}

		wlc_txflowcontrol_signal(wlc, qi, ON, ALLPRIO);
	} else {
		mboolclr(qi->stopped, override);
		/* clearing an override bit will only make a difference for
		 * flow control if it was the only bit set. For any other
		 * override setting, just return
		 */
		if (prev_override != override) {
			return;
		}

		if (qi->stopped == 0) {
			wlc_txflowcontrol_signal(wlc, qi, OFF, ALLPRIO);
		} else {
			int prio;

			for (prio = MAXPRIO; prio >= 0; prio--) {
				if (!mboolisset(qi->stopped, NBITVAL(prio)))
					wlc_txflowcontrol_signal(wlc, qi, OFF, prio);
			}
		}
	}
} /* wlc_txflowcontrol_override */

void
wlc_txflowcontrol_reset_qi(wlc_info_t *wlc, wlc_txq_info_t *qi)
{
	ASSERT(qi != NULL);

	if (qi->stopped) {
		wlc_txflowcontrol_signal(wlc, qi, OFF, ALLPRIO);
		qi->stopped = 0;
	}
}

/** enqueue SDU on a remote party ('scb') specific queue */
static void BCMFASTPATH
wlc_txq_enq(void *ctx, struct scb *scb, void *sdu, uint prec)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_txq_info_t *qi = SCB_WLCIFP(scb)->qi;
	struct pktq *q = WLC_GET_TXQ(qi);
	int prio;
	int datahiwat;

#ifdef TXQ_MUX
	WL_ERROR(("%s() scb:0x%p sdu:0x%p prec:0x%x\n",
		__FUNCTION__, OSL_OBFUSCATE_BUF(scb),
		OSL_OBFUSCATE_BUF(sdu), prec));
#endif
	ASSERT(!PKTISCHAINED(sdu));

	prio = PKTPRIO(sdu);
	datahiwat = (WLPKTTAG(sdu)->flags & WLF_AMPDU_MPDU)
		  ? wlc->pub->tunables->ampdudatahiwat
		  : wlc->pub->tunables->datahiwat;

	ASSERT(pktq_max(q) >= datahiwat);
#ifdef BCMDBG_POOL
	if (WLPKTTAG(sdu)->flags & WLF_PHDR) {
		void *pdata;

		pdata = PKTNEXT(wlc->pub.osh, sdu);
		ASSERT(pdata);
		ASSERT(WLPKTTAG(pdata)->flags & WLF_DATA);
		PKTPOOLSETSTATE(pdata, POOL_TXENQ);
	} else {
		PKTPOOLSETSTATE(sdu, POOL_TXENQ);
	}
#endif

#ifdef TXQ_MUX
{
	void * tmp_pkt = sdu;

	while (tmp_pkt) {
		prhex(__FUNCTION__, PKTDATA(wlc->osh, tmp_pkt), PKTLEN(wlc->osh, tmp_pkt));
		tmp_pkt = PKTNEXT(wlc->osh, tmp_pkt);
	}
}
#endif

	if (!wlc_prec_enq(wlc, q, sdu, prec)) {
		if (!EDCF_ENAB(wlc->pub) || (wlc->pub->wlfeatureflag & WL_SWFL_FLOWCONTROL))
			WL_ERROR(("wl%d: %s: txq overflow\n", wlc->pub->unit, __FUNCTION__));
		else
			WL_INFORM(("wl%d: %s: txq overflow\n", wlc->pub->unit, __FUNCTION__));

		PKTDBG_TRACE(wlc->osh, sdu, PKTLIST_FAIL_PRECQ);
		PKTFREE(wlc->osh, sdu, TRUE);
		WLCNTINCR(wlc->pub->_cnt->txnobuf);
		WLCIFCNTINCR(scb, txnobuf);
		WLCNTSCB_COND_INCR(scb, scb->scb_stats.tx_failures);
	}

	/* Check if flow control needs to be turned on after enqueuing the packet
	 *   Don't turn on flow control if EDCF is enabled. Driver would make the decision on what
	 *   to drop instead of relying on stack to make the right decision
	 */
	if (!EDCF_ENAB(wlc->pub) || (wlc->pub->wlfeatureflag & WL_SWFL_FLOWCONTROL)) {
		if (pktq_n_pkts_tot(q) >= datahiwat) {
			wlc_txflowcontrol(wlc, qi, ON, ALLPRIO);
		}
	} else if (wlc->pub->_priofc) {
		if (pktqprec_n_pkts(q, wlc_prio2prec_map[prio]) >= datahiwat) {
			wlc_txflowcontrol(wlc, qi, ON, prio);
		}
	}
} /* wlc_txq_enq */

void BCMFASTPATH
wlc_txq_enq_spq(wlc_info_t *wlc, struct scb *scb, struct spktq *spq, uint prec)
{
	wlc_txq_info_t *qi = SCB_WLCIFP(scb)->qi;
	struct pktq *q = WLC_GET_TXQ(qi);
	int prio;
	int datahiwat;
	void *sdu = spktq_peek(spq);

#ifdef TXQ_MUX
	WL_ERROR(("%s() scb:0x%p sdu:0x%p prec:0x%x\n",
		__FUNCTION__, OSL_OBFUSCATE_BUF(scb),
		OSL_OBFUSCATE_BUF(sdu), prec));
	{
		void *next;
		void *p = sdu;
		while (p) {
			void *tmp_pkt = p;
			next = PKTLINK(p);
			while (tmp_pkt) {
				prhex(__FUNCTION__, PKTDATA(wlc->osh, tmp_pkt),
					PKTLEN(wlc->osh, tmp_pkt));
				tmp_pkt = PKTNEXT(wlc->osh, tmp_pkt);
			}
			p = next;
		}
	}
#endif /* TXQ_MUX */
	ASSERT(!PKTISCHAINED(sdu));

	prio = PKTPRIO(sdu);
	datahiwat = (WLPKTTAG(sdu)->flags & WLF_AMPDU_MPDU)
		  ? wlc->pub->tunables->ampdudatahiwat
		  : wlc->pub->tunables->datahiwat;

	ASSERT(pktq_max(q) >= datahiwat);
#ifdef BCMDBG_POOL
	{
		void *next;
		void *p = sdu;
		while (p) {
			next = PKTLINK(p);
			if (WLPKTTAG(p)->flags & WLF_PHDR) {
				void *pdata = PKTNEXT(wlc->pub.osh, p);
				ASSERT(pdata);
				ASSERT(WLPKTTAG(pdata)->flags & WLF_DATA);
				PKTPOOLSETSTATE(pdata, POOL_TXENQ);
			} else {
				PKTPOOLSETSTATE(p, POOL_TXENQ);
			}
			p = next;
		}
	}
#endif /* BCMDBG_POOL */

	wlc_prec_enq_spq(wlc, scb, q, spq, prec);

	/* Check if flow control needs to be turned on after enqueuing the packet
	 *   Don't turn on flow control if EDCF is enabled. Driver would make the decision on what
	 *   to drop instead of relying on stack to make the right decision
	 */
	if (!EDCF_ENAB(wlc->pub) || (wlc->pub->wlfeatureflag & WL_SWFL_FLOWCONTROL)) {
		if (pktq_n_pkts_tot(q) >= datahiwat) {
			wlc_txflowcontrol(wlc, qi, ON, ALLPRIO);
		}
	} else if (wlc->pub->_priofc) {
		if (pktqprec_n_pkts(q, wlc_prio2prec_map[prio]) >= datahiwat) {
			wlc_txflowcontrol(wlc, qi, ON, prio);
		}
	}
} /* wlc_txq_enq_spq */

/** returns number of packets on the *active* transmit queue */
static uint BCMFASTPATH
wlc_txq_txpktcnt(void *ctx)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	wlc_txq_info_t *qi = wlc->active_queue;

	return (uint)pktq_n_pkts_tot(WLC_GET_TXQ(qi));
}

/**
 * bcmc_fid_generate: generate frame ID for a Broadcast/Multicast packet. The frag field is not used
 * for MC frames so is used as part of the sequence number.
 */
uint16
bcmc_fid_generate(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint16 txFrameID)
{
	uint16 frameid;
	BCM_REFERENCE(bsscfg);

	frameid = ltoh16(txFrameID) & ~(TXFID_SEQ_MASK | TXFID_QUEUE_MASK);
	frameid |= (((wlc->mc_fid_counter++) << TXFID_SEQ_SHIFT) & TXFID_SEQ_MASK) |
		TX_BCMC_FIFO;

	return frameid;
}

/** Get the number of fifos configured for the specified txq */
uint
wlc_txq_nfifo(txq_t *txq)
{
	return (txq->nfifo);
}

/** Flow control related. Get the value of the txmaxpkts */
uint16
wlc_get_txmaxpkts(wlc_info_t *wlc)
{
	return wlc->pub->txmaxpkts;
}

/** Flow control related. Set the value of the txmaxpkts */
void
wlc_set_txmaxpkts(wlc_info_t *wlc, uint16 txmaxpkts)
{
#if !defined(TXQ_MUX)
	wlc_txq_info_t *qi;
	for (qi = wlc->tx_queues; qi != NULL; qi = qi->next) {
		wlc_low_txq_set_watermark(qi->low_txq, txmaxpkts, txmaxpkts - 1);
	}

#endif /* !TXQ_MUX */
	wlc->pub->txmaxpkts = txmaxpkts;
}

/** Flow control related. Reset txmaxpkts to their defaults */
void
wlc_set_default_txmaxpkts(wlc_info_t *wlc)
{
	uint16 txmaxpkts = MAXTXPKTS;

#ifdef WLAMPDU_MAC
	if (AMPDU_AQM_ENAB(wlc->pub)) {
		if (D11REV_GE(wlc->pub->corerev, 65))
			txmaxpkts = MAXTXPKTS_AMPDUAQM_DFT << 1;
		else
			txmaxpkts = MAXTXPKTS_AMPDUAQM_DFT;
	}
	else if (AMPDU_MAC_ENAB(wlc->pub))
		txmaxpkts = MAXTXPKTS_AMPDUMAC;
#endif
	wlc_set_txmaxpkts(wlc, txmaxpkts);
}

/**
 * This function changes the phytxctl for beacon based on current beacon ratespec AND txant
 * setting as per this table:
 *  ratespec     CCK		ant = wlc->stf->txant
 *  		OFDM		ant = 3
 */
void
wlc_beacon_phytxctl_txant_upd(wlc_info_t *wlc, ratespec_t bcn_rspec)
{
	uint16 phytxant = wlc->stf->phytxant;
	uint16 mask = PHY_TXC_ANT_MASK;
	uint shm_bcn_phy_ctlwd0;


	if (D11REV_GE(wlc->pub->corerev, 40))
		shm_bcn_phy_ctlwd0 = M_BCN_TXPCTL0(wlc);
	else
		shm_bcn_phy_ctlwd0 = M_BCN_PCTLWD(wlc);

	/* for non-siso rates or default setting, use the available chains */
	if (WLC_PHY_11N_CAP(wlc->band) || WLC_PHY_VHT_CAP(wlc->band)) {
		if (WLCISHTPHY(wlc->band) || WLCISACPHY(wlc->band)) {
			mask = PHY_TXC_HTANT_MASK;
			if (bcn_rspec == 0)
				bcn_rspec = wlc->bcn_rspec;
		}
		phytxant = wlc_stf_phytxchain_sel(wlc, bcn_rspec);
	}
	WL_NONE(("wl%d: wlc_beacon_phytxctl_txant_upd: beacon txant 0x%04x mask 0x%04x\n",
		wlc->pub->unit, phytxant, mask));
	wlc_update_shm(wlc, shm_bcn_phy_ctlwd0, phytxant, mask);
}

/**
 * This function doesn't change beacon body(plcp, mac hdr). It only updates the
 *   phyctl0 and phyctl1 with the exception of tx antenna,
 *   which is handled in wlc_stf_phy_txant_upd() and wlc_beacon_phytxctl_txant_upd()
 */
void
wlc_beacon_phytxctl(wlc_info_t *wlc, ratespec_t bcn_rspec, chanspec_t chanspec)
{
	uint16 phyctl;
	int rate;
	wlcband_t *band;

	if (D11REV_GE(wlc->pub->corerev, 40)) {
		txpwr204080_t txpwrs;
		bcn_rspec &= ~RSPEC_BW_MASK;
		bcn_rspec |= RSPEC_BW_20MHZ;

		phyctl = wlc_acphy_txctl0_calc(wlc, bcn_rspec, WLC_LONG_PREAMBLE);
		wlc_write_shm(wlc, M_BCN_TXPCTL0(wlc), phyctl);
		if (wlc_stf_get_204080_pwrs(wlc, bcn_rspec, &txpwrs, 0) != BCME_OK) {
			ASSERT(!"beacon phyctl1 ppr returns error!");
		}
		phyctl = wlc_acphy_txctl1_calc(wlc, bcn_rspec, 0,
			txpwrs.pbw[BW20_IDX][TXBF_OFF_IDX], FALSE);
		wlc_write_shm(wlc, M_BCN_TXPCTL1(wlc), phyctl);
		phyctl = wlc_acphy_txctl2_calc(wlc, bcn_rspec, 0);
		wlc_write_shm(wlc, M_BCN_TXPCTL2(wlc), phyctl);
		return;
	}

	phyctl = wlc_read_shm(wlc, M_BCN_PCTLWD(wlc));

	phyctl &= ~PHY_TXC_FT_MASK;
	phyctl |= PHY_TXC_FRAMETYPE(bcn_rspec);
	/* ??? If ever beacons use short headers or phy pwr override, add the proper bits here. */

	rate = RSPEC2RATE(bcn_rspec);
	band = wlc->bandstate[CHSPEC_BANDUNIT(chanspec)];
	if ((WLCISNPHY(band) || WLCISHTPHY(band)) && RSPEC_ISCCK(bcn_rspec))
		phyctl |= ((rate * 5) << 10);

	wlc_write_shm(wlc, M_BCN_PCTLWD(wlc), phyctl);

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		uint16 phyctl1;

		bcn_rspec &= ~RSPEC_BW_MASK;
		bcn_rspec |= RSPEC_BW_20MHZ;

		phyctl1 = wlc_phytxctl1_calc(wlc, bcn_rspec, chanspec);
		wlc_write_shm(wlc, M_BCN_PCTL1WD(wlc), phyctl1);
	}
} /* wlc_beacon_phytxctl */

#ifdef BCMASSERT_SUPPORT
/** Validate the beacon phytxctl given current band */
static bool
wlc_valid_beacon_phytxctl(wlc_info_t *wlc)
{
	uint16 phyctl;
	uint shm_bcn_phy_ctlwd0;

	if (D11REV_GE(wlc->pub->corerev, 40))
		shm_bcn_phy_ctlwd0 = M_BCN_TXPCTL0(wlc);
	else
		shm_bcn_phy_ctlwd0 = M_BCN_PCTLWD(wlc);

	phyctl = wlc_read_shm(wlc, shm_bcn_phy_ctlwd0);

	return ((phyctl & PHY_TXC_FT_MASK) == PHY_TXC_FRAMETYPE(wlc->bcn_rspec));
}

/**
 * pass cfg pointer for future "per BSS bcn" validation - it is NULL
 * when the function is called from STA association context.
 */
void
wlc_validate_bcn_phytxctl(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	int idx;
	wlc_bsscfg_t *bc;

	FOREACH_BSS(wlc, idx, bc) {
		if (HWBCN_ENAB(bc)) {
			ASSERT(wlc_valid_beacon_phytxctl(wlc));
			break;
		}
	}
}
#endif /* BCMASSERT_SUPPORT */

uint16
wlc_acphy_txctl0_calc(wlc_info_t *wlc, ratespec_t rspec, uint8 preamble_type)
{
	uint16 phyctl;
	uint16 bw;

	/* PhyTxControlWord_0 */
	phyctl = PHY_TXC_FRAMETYPE(rspec);
	if ((preamble_type == WLC_SHORT_PREAMBLE) ||
	    (preamble_type == WLC_GF_PREAMBLE)) {
		ASSERT((preamble_type == WLC_GF_PREAMBLE) || !RSPEC_ISHT(rspec));
		phyctl |= D11AC_PHY_TXC_SHORT_PREAMBLE;
		WLCNTINCR(wlc->pub->_cnt->txprshort);
	}

	if (D11AC2_PHY_SUPPORT(wlc)) {
		/* STBC */
		if (RSPEC_ISSTBC(rspec)) {
			/* set b5 */
			phyctl |= D11AC2_PHY_TXC_STBC;
		}
	}

	phyctl |= wlc_stf_d11hdrs_phyctl_txant(wlc, rspec);

	/* bit 14/15 - 00/01/10/11 => 20/40/80/160 */
	switch (RSPEC_BW(rspec)) {
		case RSPEC_BW_20MHZ:
			bw = D11AC_PHY_TXC_BW_20MHZ;
		break;
		case RSPEC_BW_40MHZ:
			bw = D11AC_PHY_TXC_BW_40MHZ;
		break;
		case RSPEC_BW_80MHZ:
			bw = D11AC_PHY_TXC_BW_80MHZ;
		break;
		case RSPEC_BW_160MHZ:
			bw = D11AC_PHY_TXC_BW_160MHZ;
		break;
		default:
			ASSERT(0);
			bw = D11AC_PHY_TXC_BW_20MHZ;
		break;
	}
	phyctl |= bw;

	phyctl |= (D11AC_PHY_TXC_NON_SOUNDING);


	return phyctl;
} /* wlc_rspec_to_rts_rspec_ex */

#ifdef WL_LPC
#define PWR_SATURATE_POS 0x1F
#define PWR_SATURATE_NEG 0xE0
#endif

uint16
wlc_acphy_txctl1_calc(wlc_info_t *wlc, ratespec_t rspec, int8 lpc_offset,
	uint8 txpwr, bool is_mu)
{
	uint16 phyctl = 0;
	chanspec_t cspec = wlc->chanspec;
	uint16 sb = ((cspec & WL_CHANSPEC_CTL_SB_MASK) >> WL_CHANSPEC_CTL_SB_SHIFT);

	/* Primary Subband Location: b 16-18
		LLL ==> 000
		LLU ==> 001
		...
		UUU ==> 111
	*/
	phyctl = sb >> ((RSPEC_BW(rspec) >> RSPEC_BW_SHIFT) - 1);

#ifdef WL_LPC
	/* Include the LPC offset also in the power offset */
	if (LPC_ENAB(wlc)) {
		int8 rate_offset = txpwr, tot_offset;

		/* Addition of two S4.1 words */
		rate_offset <<= 2;
		rate_offset >>= 2; /* Sign Extend */

		lpc_offset <<= 2;
		lpc_offset >>= 2; /* Sign Extend */

		tot_offset = rate_offset + lpc_offset;

		if (tot_offset < (int8)PWR_SATURATE_NEG)
			tot_offset = PWR_SATURATE_NEG;
		else if (tot_offset > PWR_SATURATE_POS)
			tot_offset = PWR_SATURATE_POS;

		txpwr = tot_offset;
	}
#endif /* WL_LPC */

	phyctl |= (txpwr << PHY_TXC1_HTTXPWR_OFFSET_SHIFT);

	if (D11REV_LT(wlc->pub->corerev, 64)) {
		if (WLPROPRIETARY_11N_RATES_ENAB(wlc->pub)) {
			if (RSPEC_ISHT(rspec) && (rspec & RSPEC_RATE_MASK)
				>= WLC_11N_FIRST_PROP_MCS) {
				phyctl |= D11AC_PHY_TXC_11N_PROP_MCS;
			}
		}
	} else if (is_mu) {
		/* MU frame for corerev >= 64 */
		phyctl |= D11AC2_PHY_TXC_MU;
	}

	return phyctl;
} /* wlc_acphy_txctl1_calc */

uint16
wlc_acphy_txctl2_calc(wlc_info_t *wlc, ratespec_t rspec, uint8 txbf_uidx)
{
	uint16 phyctl = 0;
	uint nss = wlc_ratespec_nss(rspec);
	uint mcs;
	uint8 rate;
	const wlc_rateset_t* cur_rates = NULL;
	uint16 rindex;

	/* mcs+nss: bit 32 through 37 */
	if (RSPEC_ISHT(rspec)) {
		/* for 11n: B[32:37] for mcs[0:32] */
		if (WLPROPRIETARY_11N_RATES_ENAB(wlc->pub)) {
			mcs = rspec & D11AC_PHY_TXC_11N_MCS_MASK;
		} else {
			mcs = rspec & RSPEC_RATE_MASK;
		}
		phyctl = (uint16)mcs;
	} else if (RSPEC_ISVHT(rspec)) {
		/* for 11ac: B[32:35] for mcs[0-9], B[36:37] for n_ss[1-4]
			(0 = 1ss, 1 = 2ss, 2 = 3ss, 3 = 4ss)
		*/
		mcs = rspec & RSPEC_VHT_MCS_MASK;
		ASSERT(mcs <= WLC_MAX_VHT_MCS);
		phyctl = (uint16)mcs;
		ASSERT(nss <= 8);
		phyctl |= ((nss-1) << D11AC_PHY_TXC_11AC_NSS_SHIFT);
	} else {
		rate = RSPEC2RATE(rspec);

		if (RSPEC_ISOFDM(rspec)) {
			cur_rates = &ofdm_rates;
		} else {
			/* for 11b: B[32:33] represents phyrate
				(0 = 1mbps, 1 = 2mbps, 2 = 5.5mbps, 3 = 11mbps)
			*/
			cur_rates = &cck_rates;
		}
		for (rindex = 0; rindex < cur_rates->count; rindex++) {
			if ((cur_rates->rates[rindex] & RATE_MASK) == rate) {
				break;
			}
		}
		ASSERT(rindex < cur_rates->count);
		phyctl = rindex;
	}

	if (D11AC2_PHY_SUPPORT(wlc)) {
		if (WLPROPRIETARY_11N_RATES_ENAB(wlc->pub)) {
			if (RSPEC_ISHT(rspec) && (rspec & RSPEC_RATE_MASK)
				>= WLC_11N_FIRST_PROP_MCS) {
				phyctl |= D11AC2_PHY_TXC_11N_PROP_MCS;
			}
		}

		/* b41 - b47: TXBF user index */
		phyctl |= (txbf_uidx << D11AC2_PHY_TXC_TXBF_USER_IDX_SHIFT);
	} else {
		/* corerev < 64 */
		/* STBC */
		if (RSPEC_ISSTBC(rspec)) {
			/* set b38 */
			phyctl |= D11AC_PHY_TXC_STBC;
		}
		/* b39 - b 47 all 0 */
	}

	return phyctl;
} /* wlc_acphy_txctl2_calc */

static bool
wlc_phy_rspec_check(wlc_info_t *wlc, uint16 bw, ratespec_t rspec)
{
	if (RSPEC_ISVHT(rspec)) {
		uint8 mcs;
		uint Nss;
		uint Nsts;

		mcs = rspec & WL_RSPEC_VHT_MCS_MASK;
		Nss = (rspec & WL_RSPEC_VHT_NSS_MASK) >> WL_RSPEC_VHT_NSS_SHIFT;

		BCM_REFERENCE(mcs);
		ASSERT(mcs <= WLC_MAX_VHT_MCS);

		/* VHT STBC expansion always is Nsts = Nss*2, so STBC expansion == Nss */
		if (RSPEC_ISSTBC(rspec)) {
			Nsts = 2*Nss;
		} else {
			Nsts = Nss;
		}

		/* we only support up to 3x3 */
		BCM_REFERENCE(Nsts);
		ASSERT(Nsts + RSPEC_TXEXP(rspec) <= 3);
	} else if (RSPEC_ISHT(rspec)) {
		uint mcs = rspec & RSPEC_RATE_MASK;

		ASSERT(mcs <= 32);

		if (mcs == 32) {
			ASSERT(RSPEC_ISSTBC(rspec) == FALSE);
			ASSERT(RSPEC_TXEXP(rspec) == 0);
			ASSERT(bw == PHY_TXC1_BW_40MHZ_DUP);
		} else {
			uint Nss = 1 + (mcs / 8);
			uint Nsts;

			/* BRCM HT chips only support STBC expansion by 2*Nss */
			if (RSPEC_ISSTBC(rspec)) {
				Nsts = 2*Nss;
			} else {
				Nsts = Nss;
			}

			/* we only support up to 3x3 */
			BCM_REFERENCE(Nsts);
			ASSERT(Nsts + RSPEC_TXEXP(rspec) <= 3);
		}
	} else if (RSPEC_ISOFDM(rspec)) {
		ASSERT(RSPEC_ISSTBC(rspec) == FALSE);
	} else {
		ASSERT(RSPEC_ISCCK(rspec));

		ASSERT((bw == PHY_TXC1_BW_20MHZ) || (bw == PHY_TXC1_BW_20MHZ_UP));
		ASSERT(RSPEC_ISSTBC(rspec) == FALSE);
#if defined(WL_BEAMFORMING)
		if (TXBF_ENAB(wlc->pub) && PHYCORENUM(wlc->stf->txstreams) > 1) {
			ASSERT(RSPEC_ISTXBF(rspec) == FALSE);
		}
#endif
	}

	return TRUE;
} /* wlc_phy_rspec_check */

uint16
wlc_phytxctl1_calc(wlc_info_t *wlc, ratespec_t rspec, chanspec_t chanspec)
{
	uint16 phyctl1 = 0;
	uint16 bw;
	wlcband_t *band = wlc->bandstate[CHSPEC_BANDUNIT(chanspec)];

	if (RSPEC_IS20MHZ(rspec)) {
		bw = PHY_TXC1_BW_20MHZ;
		if (CHSPEC_IS40(chanspec) && CHSPEC_SB_UPPER(WLC_BAND_PI_RADIO_CHANSPEC)) {
			bw = PHY_TXC1_BW_20MHZ_UP;
		}
	} else  {
		ASSERT(RSPEC_IS40MHZ(rspec));

		bw = PHY_TXC1_BW_40MHZ;
		if (((rspec & RSPEC_RATE_MASK) == 32) || RSPEC_ISOFDM(rspec)) {
			bw = PHY_TXC1_BW_40MHZ_DUP;
		}

	}
	wlc_phy_rspec_check(wlc, bw, rspec);

	/* Handle HTPHY */
	if (WLCISHTPHY(band)) {
		uint16 spatial_map = wlc_stf_spatial_expansion_get(wlc, rspec);
		/* retrieve power offset on per packet basis */
		uint8 pwr = wlc_stf_get_pwrperrate(wlc, rspec, spatial_map);
		phyctl1 = (bw | (pwr << PHY_TXC1_HTTXPWR_OFFSET_SHIFT));
		/* spatial mapper table lookup on per packet basis */
		phyctl1 |= (spatial_map << PHY_TXC1_HTSPARTIAL_MAP_SHIFT);
		goto exit;
	}

	/* Handle the other mcs capable phys (NPHY, LCN at this point) */
	if (RSPEC_ISHT(rspec)) {
		uint mcs = rspec & RSPEC_RATE_MASK;
		uint16 stf;

		/* Determine the PHY_TXC1_MODE value based on MCS and STBC or TX expansion.
		 * PHY_TXC1_MODE values cover 1 and 2 chains only and are:
		 * (Nss = 2, NTx = 2) 		=> SDM (Spatial Division Multiplexing)
		 * (Nss = 1, NTx = 2) 		=> CDD (Cyclic Delay Diversity)
		 * (Nss = 1, +1 STBC, NTx = 2) 	=> STBC (Space Time Block Coding)
		 * (Nss = 1, NTx = 1) 		=> SISO (Single In Single Out)
		 *
		 * MCS 0-7 || MCS == 32 	=> Nss = 1 (Number of spatial streams)
		 * MCS 8-15 			=> Nss = 2
		 */
		if ((!WLPROPRIETARY_11N_RATES_ENAB(wlc->pub) && mcs > 7 && mcs <= 15) ||
		     (WLPROPRIETARY_11N_RATES_ENAB(wlc->pub) && GET_11N_MCS_NSS(mcs) == 2)) {
			/* must be SDM, only 2x2 devices */
			stf = PHY_TXC1_MODE_SDM;
		} else if (RSPEC_TXEXP(rspec)) {
			/*
			 * Tx Expansion: number of tx chains (NTx) beyond the minimum required for
			 * the space-time-streams, to increase robustness. Expansion is non-STBC.
			 */
			stf = PHY_TXC1_MODE_CDD;
		} else if (RSPEC_ISSTBC(rspec)) {
			/* expansion is STBC */
			stf = PHY_TXC1_MODE_STBC;
		} else {
			/* no expansion */
			stf = PHY_TXC1_MODE_SISO;
		}

		phyctl1 = bw | (stf << PHY_TXC1_MODE_SHIFT);

		/* set the upper byte of phyctl1 */
		phyctl1 |= (mcs_table[mcs].tx_phy_ctl3 << 8);
	} else if (RSPEC_ISCCK(rspec)) {
		phyctl1 = (bw | PHY_TXC1_MODE_SISO);
	} else {	/* legacy OFDM/CCK */
		int16 phycfg;
		uint16 stf;
		uint rate = (rspec & RSPEC_RATE_MASK);

		/* get the phyctl byte from rate phycfg table */
		if ((phycfg = wlc_rate_legacy_phyctl(rate)) == -1) {
			WL_ERROR(("%s: wrong legacy OFDM/CCK rate\n", __FUNCTION__));
			ASSERT(0);
			phycfg = 0;
		}

		if (RSPEC_TXEXP(rspec)) {
			/* CDD expansion for OFDM */
			stf = PHY_TXC1_MODE_CDD;
		} else {
			stf = PHY_TXC1_MODE_SISO;
		}

		/* set the upper byte of phyctl1 */
		phyctl1 = (bw | (phycfg << 8) | (stf << PHY_TXC1_MODE_SHIFT));
	}
exit:
#ifdef BCMDBG
	/* phy clock must support 40Mhz if tx descriptor uses it */
	if ((phyctl1 & PHY_TXC1_BW_MASK) >= PHY_TXC1_BW_40MHZ) {
		ASSERT(CHSPEC_IS40(wlc->chanspec));
		ASSERT(wlc->chanspec == phy_utils_get_chanspec((phy_info_t *)WLC_PI(wlc)));
	}
#endif /* BCMDBG */
	return phyctl1;
} /* wlc_phytxctl1_calc */
#ifdef WL_FRAGDUR
/* check if a frame is good for fragmentation */
static bool BCMFASTPATH
wlc_do_fragdur(wlc_info_t *wlc, struct scb *scb, void *pkt)
{
	wlc_key_t *key = NULL;
	wlc_key_info_t key_info;
	wlc_bsscfg_t *bsscfg;
	uint frag_length = 0;
	uint pkt_length, nfrags;
	uint8 prio, ac;

	prio = (uint8)PKTPRIO(pkt);
	ac = WME_PRIO2AC(prio);

	pkt_length = pkttotlen(wlc->osh, pkt);

	bsscfg = SCB_BSSCFG(scb);
	ASSERT(bsscfg != NULL);

	key = wlc_keymgmt_get_tx_key(wlc->keymgmt, scb, bsscfg, &key_info);
	/* TKIP MIC space reservation */
	if (key) {
		if (key_info.algo == CRYPTO_ALGO_TKIP) {
			pkt_length += TKIP_MIC_SIZE;
		}
	}

	nfrags = wlc_frag(wlc, scb, ac, pkt_length, &frag_length);

	return (nfrags > 1);
}

/* check if tx aggregation state needs to be changed */
static bool BCMFASTPATH
wlc_txagg_state_changed(wlc_info_t *wlc, struct scb *scb, void *pkt)
{
	wlc_pkttag_t *pkttag = WLPKTTAG(pkt);

	/* Only process pkts without txhdr, pkts with txhdr are requeued and have been
	 * previously processed. The txhdr contains AMPDU bits which may depend on
	 * previous/subsequent pkts (TXC_AMPDU_FIRST/MIDDLE/LAST) so WLF_AMPDU_MPDU
	 * for these pkts should not be changed. Also, wlc_frag_ok() does not account
	 * for the length of the txhdr if present.
	 */

	/* turn off tx aggregation if fragmented frame, else turn it on  */
	if ((wlc_fragdur_length(wlc->fragdur) ||
		wlc->usr_fragthresh < DOT11_MAX_FRAG_LEN) &&
		!(pkttag->flags & WLF_TXHDR)) {
		wlc_bsscfg_t *bsscfg;
		bsscfg = SCB_BSSCFG(scb);
		ASSERT(bsscfg != NULL);

		if (wlc_do_fragdur(wlc, scb, pkt)) {
			/* turn off tx aggregation if it is on */
			if (wlc_ampdu_tx_get_bsscfg_aggr(wlc->ampdu_tx, bsscfg)) {

				wlc_ampdu_tx_set_bsscfg_aggr(wlc->ampdu_tx, bsscfg,
					OFF, AMPDU_ALL_TID_BITMAP);

				/* remove ampdu flag to do fragmentation in wlc_prep_sdu */
				if (WLPKTFLAG_AMPDU(pkttag))
					pkttag->flags &= ~WLF_AMPDU_MPDU;

				return TRUE;
			}
		}
		else {
			/* turn on tx aggregation if it has been turned off */
			if (!wlc_ampdu_tx_get_bsscfg_aggr(wlc->ampdu_tx, bsscfg) &&
				wlc_scb_ratesel_hit_txaggthreshold(wlc->wrsi, scb)) {
				wlc_ampdu_tx_set_bsscfg_aggr(wlc->ampdu_tx, bsscfg,
					ON, AMPDU_ALL_TID_BITMAP);
				return TRUE;
			}
		}
	}

	return FALSE;
}
#endif /* WL_FRAGDUR */

/** Remap HW issigned suppress code to software packet suppress tx_status */
int
wlc_tx_status_map_hw_to_sw_supr_code(wlc_info_t * wlc, int supr_status)
{
	BCM_REFERENCE(wlc);

	if ((supr_status > TX_STATUS_SUPR_NONE) &&
		(supr_status < NUM_TX_STATUS_SUPR)) {
			return WLC_TX_STS_SUPPRESS + supr_status;
	}

	return WLC_TX_STS_SUPPRESS;
}

#ifdef WL_TX_STALL
/** Utility macro to get tx_stall counters BSSCFG cubby */
#define ST_STALL_BSSCFG_CUBBY(tx_stall, cfg) \
	((wlc_tx_stall_counters_t *)BSSCFG_CUBBY(cfg, (tx_stall)->cfg_handle))

/** Utility macro to get tx_stall counters SCB cubby */
#define ST_STALL_SCB_CUBBY(tx_stall, scb) \
	((wlc_tx_stall_counters_t *)SCB_CUBBY(scb, (tx_stall)->scb_handle))


/** scb cubby init function */
static int
wlc_tx_stall_scb_init(void *context, struct scb * scb)
{
	wlc_tx_stall_info_t * tx_stall;
	wlc_tx_stall_counters_t * counters;
	char * ifname = "";
	wlc_bsscfg_t * cfg;

	BCM_REFERENCE(ifname);
	BCM_REFERENCE(counters);

	if ((context == NULL) || (scb == NULL)) {
		return BCME_ERROR;
	}

	tx_stall = (wlc_tx_stall_info_t *)context;
	counters = ST_STALL_SCB_CUBBY(tx_stall, scb);

	counters->sysup_time[AC_BE] = tx_stall->wlc->pub->now;
	counters->sysup_time[AC_BK] = tx_stall->wlc->pub->now;
	counters->sysup_time[AC_VI] = tx_stall->wlc->pub->now;
	counters->sysup_time[AC_VO] = tx_stall->wlc->pub->now;

	cfg = SCB_BSSCFG(scb);
	if (cfg && cfg->wlc && cfg->wlcif) {
		ifname = wl_ifname(cfg->wlc->wl, cfg->wlcif->wlif);
	}
	WL_ASSOC(("%s: if:%s, "MACF"\n", __FUNCTION__, ifname, ETHERP_TO_MACF(&scb->ea)));

	return BCME_OK;
}

/** scb cubby deinit function */
static void
wlc_tx_stall_scb_deinit(void *context, struct scb * scb)
{
	wlc_tx_stall_info_t * tx_stall;
	wlc_tx_stall_counters_t * counters;
	char * ifname = "";
	wlc_bsscfg_t * cfg;

	BCM_REFERENCE(ifname);
	BCM_REFERENCE(counters);

	if ((context == NULL) || (scb == NULL)) {
		return;
	}

	tx_stall = (wlc_tx_stall_info_t *)context;
	counters = ST_STALL_SCB_CUBBY(tx_stall, scb);

	cfg = SCB_BSSCFG(scb);
	if (cfg && cfg->wlc && cfg->wlcif) {
		ifname = wl_ifname(cfg->wlc->wl, cfg->wlcif->wlif);
	}
	WL_ASSOC(("%s: if:%s, "MACF"\n", __FUNCTION__, ifname, ETHERP_TO_MACF(&scb->ea)));

	wlc_tx_status_dump_counters(counters, NULL, __FUNCTION__);

	/* Clear the stall count at global and bsscfg on scb deletion */
	wlc_tx_status_reset(tx_stall->wlc, FALSE, NULL, scb);
	return;
}

/** bsscfg cubby init function */
static int
wlc_tx_stall_cfg_init(void *context, wlc_bsscfg_t * cfg)
{
	wlc_tx_stall_info_t * tx_stall;
	wlc_tx_stall_counters_t * counters;
	char * ifname = "";

	BCM_REFERENCE(ifname);
	BCM_REFERENCE(counters);

	if ((context == NULL) || (cfg == NULL)) {
		return BCME_ERROR;
	}

	tx_stall = (wlc_tx_stall_info_t *)context;
	counters = ST_STALL_BSSCFG_CUBBY(tx_stall, cfg);
	counters->sysup_time[AC_BE] = tx_stall->wlc->pub->now;
	counters->sysup_time[AC_BK] = tx_stall->wlc->pub->now;
	counters->sysup_time[AC_VI] = tx_stall->wlc->pub->now;
	counters->sysup_time[AC_VO] = tx_stall->wlc->pub->now;

	if (cfg->wlc && cfg->wlcif) {
		ifname = wl_ifname(cfg->wlc->wl, cfg->wlcif->wlif);
	}

	WL_ASSOC(("%s: %s\n", __FUNCTION__, ifname));

	return BCME_OK;
}

/** bsscfg cubby deinit function */
static void
wlc_tx_stall_cfg_deinit(void *context, wlc_bsscfg_t * cfg)
{
	wlc_tx_stall_info_t * tx_stall;
	wlc_tx_stall_counters_t * counters;
	char * ifname = "";

	BCM_REFERENCE(ifname);
	BCM_REFERENCE(counters);

	if ((context == NULL) || (cfg == NULL)) {
		return;
	}

	tx_stall = (wlc_tx_stall_info_t *)context;
	counters = ST_STALL_BSSCFG_CUBBY(tx_stall, cfg);
	if (cfg->wlc && cfg->wlcif) {
		ifname = wl_ifname(cfg->wlc->wl, cfg->wlcif->wlif);
	}

	WL_ASSOC(("%s: %s\n", __FUNCTION__, ifname));

	wlc_tx_status_dump_counters(counters, NULL, __FUNCTION__);

	/* Clear the stall count at global and each scb on bsscfg deletion */
	wlc_tx_status_reset(tx_stall->wlc, FALSE, cfg, NULL);
	return;
}

/** Allocate, init and return the wlc_tx_stall_info */
wlc_tx_stall_info_t *
BCMATTACHFN(wlc_tx_stall_attach)(wlc_info_t * wlc)
{
	osl_t * osh = wlc->osh;
	wlc_tx_stall_info_t * tx_stall = NULL;
	wlc_tx_stall_counters_t * counters = NULL;
	wlc_tx_stall_error_info_t * error = NULL;

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	wlc_tx_stall_counters_t * history = NULL;
#endif

	/* alloc parent tx_stall structure */
	tx_stall = (wlc_tx_stall_info_t*)MALLOCZ(osh, sizeof(*tx_stall));

	if (tx_stall == NULL) {
		WL_ERROR(("wlc_tx_stall_attach: "
			"no mem for %zd bytes tx_stall info,"
			"malloced %d bytes\n",
			sizeof(*tx_stall), MALLOCED(osh)));
		return NULL;
	}

	counters = (wlc_tx_stall_counters_t *)MALLOCZ(osh, sizeof(*counters));
	if (counters == NULL) {
		WL_ERROR(("wlc_tx_stall_attach: "
			"no mem for %zd bytes tx_stall_counters,"
			"malloced %d bytes\n", sizeof(*counters), MALLOCED(osh)));
		goto fail;
	}
	tx_stall->counters = counters;

	error = (wlc_tx_stall_error_info_t *)MALLOCZ(osh, sizeof(*error));
	if (error == NULL) {
		WL_ERROR(("wlc_tx_stall_attach: "
			"no mem for %zd bytes tx_stall_error,"
			"malloced %d bytes\n", sizeof(*error), MALLOCED(osh)));
		goto fail;
	}
	tx_stall->error = error;

	tx_stall->wlc = wlc;

	/* Initialize default parameters */
	tx_stall->timeout = WLC_TX_STALL_PEDIOD;
	tx_stall->sample_len = WLC_TX_STALL_SAMPLE_LEN;
	tx_stall->stall_threshold = WLC_TX_STALL_THRESHOLD;
	tx_stall->exclude_bitmap = WLC_TX_STALL_EXCLUDE;
	tx_stall->exclude_bitmap1 = WLC_TX_STALL_EXCLUDE1;
	tx_stall->assert_on_error = WLC_TX_STALL_ASSERT_ON_ERROR;

	counters->sysup_time[AC_BE] = wlc->pub->now;
	counters->sysup_time[AC_BK] = wlc->pub->now;
	counters->sysup_time[AC_VI] = wlc->pub->now;
	counters->sysup_time[AC_VO] = wlc->pub->now;

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	/* History of N periods   */
	history = (wlc_tx_stall_counters_t *)MALLOCZ(osh,
		(sizeof(*tx_stall->history) * WLC_TX_STALL_COUNTERS_HISTORY_LEN));

	tx_stall->history = history;
	tx_stall->history_idx = 0;
#endif /* BCMDBG || BCMDBG_DUMP */

	tx_stall->scb_handle = wlc_scb_cubby_reserve(wlc,
		sizeof(wlc_tx_stall_counters_t),
		wlc_tx_stall_scb_init, wlc_tx_stall_scb_deinit,
		NULL, (void *)tx_stall);

	if (tx_stall->scb_handle == -1) {
		goto fail;
	}

	tx_stall->cfg_handle = wlc_bsscfg_cubby_reserve(wlc,
		sizeof(wlc_tx_stall_counters_t),
		wlc_tx_stall_cfg_init, wlc_tx_stall_cfg_deinit,
		NULL, tx_stall);

	if (tx_stall->cfg_handle == -1) {
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	wlc_dump_register(wlc->pub, "tx_activity",
		(dump_fn_t)wlc_tx_status_tx_activity_dump, (void *)wlc);
#endif /* BCMDBG || BCMDBG_DUMP */

#ifdef HEALTH_CHECK
	if (!wl_health_check_module_register(wlc->wl, "wl_tx_stall_check",
		wlc_hchk_tx_stall_check, wlc, WL_HC_DD_TX_STALL)) {
			goto fail;
	}
#endif

	return tx_stall;

fail:

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	if (history) {
		MFREE(osh, history, sizeof(*history));
	}
#endif

	if (error) {
		MFREE(osh, error, sizeof(*error));
	}

	if (counters) {
		MFREE(osh, counters, sizeof(*counters));
	}

	if (tx_stall) {
		MFREE(osh, tx_stall, sizeof(*tx_stall));
	}

	return NULL;
}

void
BCMATTACHFN(wlc_tx_stall_detach)(wlc_tx_stall_info_t * tx_stall)
{
	wlc_info_t * wlc;
	osl_t *osh;

	if (tx_stall == NULL) {
		return;
	}

	wlc = tx_stall->wlc;
	osh = wlc->osh;
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	if (tx_stall->history != NULL) {
		MFREE(osh, tx_stall->history,
			(sizeof(*tx_stall->history) * WLC_TX_STALL_COUNTERS_HISTORY_LEN));
		tx_stall->history = NULL;
	}
#endif /* BCMDBG || BCMDBG_DUMP */
	if (tx_stall->counters) {
		MFREE(osh, tx_stall->counters, sizeof(*tx_stall->counters));
	}

	if (tx_stall->error) {
		MFREE(osh, tx_stall->error, sizeof(*tx_stall->error));
	}

	MFREE(osh, tx_stall, sizeof(*tx_stall));
	wlc->tx_stall = NULL;
}

/** Reset TX stall statemachine */
int
wlc_tx_status_reset(wlc_info_t * wlc, bool clear_all, wlc_bsscfg_t * cfg, scb_t * scb)
{
	wlc_tx_stall_info_t * tx_stall;
	wlc_tx_stall_counters_t * bsscfg_counters = NULL;
	wlc_tx_stall_counters_t * scb_counters = NULL;
	wlc_tx_stall_counters_t * counters = NULL;
	struct scb_iter scbiter;
	int idx;

	ASSERT(wlc);

	tx_stall = wlc->tx_stall;

	if (tx_stall == NULL) {
		return BCME_NOTREADY;
	}

	counters = tx_stall->counters;

	/* Clear all counters and stall count */
	if (clear_all) {
		memset(counters, 0x00, sizeof(*counters));

		/* Reset each BSSCFG counters */
		FOREACH_BSS(wlc, idx, cfg) {
			/* Verify total failure rate of this BSSCFG */
			/* tx stall counters cubby */
			bsscfg_counters = ST_STALL_BSSCFG_CUBBY(tx_stall, cfg);
			memset(bsscfg_counters, 0x00, sizeof(*bsscfg_counters));

			/* Clear counters of each SCB */
			FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
				scb_counters = ST_STALL_SCB_CUBBY(tx_stall, scb);
				memset(scb_counters, 0x00, sizeof(*scb_counters));
			}
		}
	} else {
		/* Just clear the stall count only for global and given bsscfg or scbs */
		memset(&counters->stall_count, 0x00, sizeof(counters->stall_count));

		if (cfg) {
			bsscfg_counters = ST_STALL_BSSCFG_CUBBY(tx_stall, cfg);
			memset(&bsscfg_counters->stall_count, 0x00,
				sizeof(bsscfg_counters->stall_count));

			FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
				scb_counters = ST_STALL_SCB_CUBBY(tx_stall, scb);
				memset(&scb_counters->stall_count, 0x00,
					sizeof(scb_counters->stall_count));
			}
		}

		if (scb) {
			cfg = SCB_BSSCFG(scb);
			scb_counters = ST_STALL_SCB_CUBBY(tx_stall, scb);
			memset(&scb_counters->stall_count, 0x00, sizeof(scb_counters->stall_count));

			if (cfg) {
				bsscfg_counters = ST_STALL_BSSCFG_CUBBY(tx_stall, cfg);
				memset(&bsscfg_counters->stall_count, 0x00,
					sizeof(bsscfg_counters->stall_count));
			}
		}
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	if (clear_all && tx_stall->history) {
		/* History of N periods   */
		memset(tx_stall->history, 0x00,
			sizeof(*tx_stall->history) * WLC_TX_STALL_COUNTERS_HISTORY_LEN);
		tx_stall->history_idx = 0;
	}
#endif /* BCMDBG || BCMDBG_DUMP */

	return BCME_OK;
}

/** trigger a forced rx stall */
int
wlc_tx_status_force_stall(wlc_info_t * wlc, uint32 stall_type)
{
	wlc_tx_stall_info_t * tx_stall;
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(stall_type);

	tx_stall = wlc->tx_stall;

	if (tx_stall) {
		tx_stall->force_tx_stall = TRUE;
	}

	return BCME_NOTREADY;
}

/** Update TX counters */
int
wlc_tx_status_update_counters(wlc_info_t * wlc, void * pkt,
	scb_t * scb, wlc_bsscfg_t * bsscfg, int tx_status,
	int count)
{
	int ac = AC_BE;
	wlc_tx_stall_info_t * tx_stall;
	wlc_tx_stall_counters_t * bsscfg_counters = NULL;
	wlc_tx_stall_counters_t * scb_counters = NULL;
	bool update_counters = FALSE;

	ASSERT(wlc);


	tx_stall = wlc->tx_stall;

	ASSERT(tx_stall);

	/* Timeout == 0  indicates this feature is disabled */
	if (tx_stall->timeout == 0) {
		return BCME_OK;
	}

#ifndef WLC_TXSTALL_FULL_STATS
	/* Min stats maintains only total tx and total failures.
	 * Filter out exceptions for updating stats
	 */
	if (tx_status < 32) {
		/* Count only the failures not in the excluded list */
		if (!isset(&tx_stall->exclude_bitmap, tx_status)) {
			update_counters = TRUE;
		}
	} else if (tx_status < 64) {
		if (!isset(&tx_stall->exclude_bitmap1, (tx_status - 32))) {
			update_counters = TRUE;
		}
	}

	/* Remap any toss reason to index 1 only which holds all failures */
	if (tx_status != WLC_TX_STS_QUEUED) {
		tx_status = WLC_TX_STS_FAILURE;
	}
#else
	/* Update counters always. Excluded counters will be
	 * ignored during health check
	 */
	update_counters = TRUE;
#endif /* WLC_TXSTALL_FULL_STATS */

	/* bail out of nothing to do */
	if (update_counters == FALSE) {
		return BCME_OK;
	}

	/* Get packet priority */
	if (pkt) {
		ac = WME_PRIO2AC(PKTPRIO(pkt));
	}

	if ((ac < AC_BE) || (ac > AC_VO)) {
		ac = AC_BE;
	}

	/* If input packet pointer is valid then get the scb from packet */
	if ((scb == NULL) && pkt) {
			scb = WLPKTTAGSCBGET(pkt);
	}

	if (scb) {
		/* Do not update SCB's marked for deletion */
		if (SCB_MARKED_DEL(scb)) {
			scb = NULL;
		} else {
			/* tx stall counters cubby */
			scb_counters = ST_STALL_SCB_CUBBY(tx_stall, scb);
		}
	}

	/* If bsscfg is not provided, then get if from either scb or from pkt */
	if (bsscfg == NULL) {
		/* If valid SCB, then get the bsscfg from scb */
		if (scb) {
			bsscfg = SCB_BSSCFG(scb);
		} else if (pkt) {
			/* else get the bsscfg from packet tag */
			bsscfg = WLC_BSSCFG(wlc, WLPKTTAGBSSCFGGET(pkt));
		}
	}

	if (bsscfg) {
		/* tx stall counters cubby */
		bsscfg_counters = ST_STALL_BSSCFG_CUBBY(tx_stall, bsscfg);
	}

	if (tx_status >= WLC_TX_STS_MAX) {
		WL_ERROR(("%s: ERROR: Unknown tx fail reason: %d\n", __FUNCTION__, tx_status));
		ASSERT(!"tx_status < WLC_TX_STS_MAX");
		tx_status = WLC_TX_STS_FAILURE;
	}

	WLCNTADD(tx_stall->counters->tx_stats[ac][tx_status], count);
	WLCNTCONDADD(scb_counters, scb_counters->tx_stats[ac][tx_status], count);
	WLCNTCONDADD(bsscfg_counters,
		bsscfg_counters->tx_stats[ac][tx_status], count);

	/* Test mode to trigger failure with no buffer reason code */
	if (tx_stall->force_tx_stall) {
		WLCNTADD(tx_stall->counters->tx_stats[ac][WLC_TX_STS_SUPPRESS],
			count);
		WLCNTCONDADD(scb_counters,
			scb_counters->tx_stats[ac][WLC_TX_STS_SUPPRESS], count);
		WLCNTCONDADD(bsscfg_counters,
			bsscfg_counters->tx_stats[ac][WLC_TX_STS_SUPPRESS], count);
	}

	return BCME_OK;
}

static void
wlc_tx_status_report_error(wlc_info_t * wlc)
{
	wlc_tx_stall_info_t * tx_stall = wlc->tx_stall;

	/* Disable test mode after failure is triggered */
	if (tx_stall->force_tx_stall) {
		tx_stall->force_tx_stall = FALSE;
	}

	if (tx_stall->assert_on_error) {
		wlc->hw->need_reinit = WL_REINIT_TX_STALL;
		tx_stall->error->stall_reason = WLC_TX_STALL_REASON_TX_ERROR;
		WLC_FATAL_ERROR(wlc);
	} else {
		WL_ERROR(("WL unit: %d %s: Possible TX stall...\n",
			wlc->pub->unit, __FUNCTION__));
		wl_log_system_state(wlc->wl, "TX_STALL", TRUE);
	}

	return;
}

/** TX Stall health check.
 * Verifies TX stall conditions and triggers fatal error on
 * detecting the stall
 */
int
wlc_tx_status_health_check(wlc_info_t * wlc)
{
	scb_t * scb;
	wlc_bsscfg_t * cfg;
	wlc_tx_stall_info_t * tx_stall = NULL;
	wlc_tx_stall_counters_t * bsscfg_counters = NULL;
	wlc_tx_stall_counters_t * scb_counters = NULL;
	struct scb_iter scbiter;

	int ret = BCME_OK;
	int idx;
	uint32 stall_detected = 0;

	tx_stall = wlc->tx_stall;

	ASSERT(tx_stall);

	/* tx_stall health check disabled */
	if (tx_stall->timeout == 0) {
		return ret;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	if (tx_stall->history) {
		/* Cache global counters history before clearing them */
		tx_stall->history[tx_stall->history_idx] = *tx_stall->counters;
	}

	/* Update index */
	tx_stall->history_idx++;
	if (tx_stall->history_idx >= WLC_TX_STALL_COUNTERS_HISTORY_LEN) {
		tx_stall->history_idx = 0;
	}
#endif /* BCMDBG || BCMDBG_DUMP */

	/* Check global failure threshold */
	stall_detected += wlc_tx_status_calculate_failure_rate(wlc,
		tx_stall->counters, TRUE, "Global", tx_stall->error);

	/* Iterate all active BSS */
	FOREACH_BSS(wlc, idx, cfg) {
		char ifname[32] = "UNKNOWN BSS";

		if (!cfg->up) {
			continue;
		}

		if ((cfg->wlcif != NULL)) {
			strncpy(ifname, wl_ifname(wlc->wl, cfg->wlcif->wlif),
					sizeof(ifname));
			ifname[sizeof(ifname) - 1] = '\0';
		}

		/* Verify total failure rate of this BSSCFG */
		/* tx stall counters cubby */
		bsscfg_counters = ST_STALL_BSSCFG_CUBBY(tx_stall, cfg);

		stall_detected += wlc_tx_status_calculate_failure_rate(wlc, bsscfg_counters,
			TRUE, ifname, tx_stall->error);

		/* Verify TX Failure rate for each SCB */
		FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
			char eabuf[ETHER_ADDR_STR_LEN];

			scb_counters = ST_STALL_SCB_CUBBY(tx_stall, scb);
			bcm_ether_ntoa(&scb->ea, eabuf);

			stall_detected += wlc_tx_status_calculate_failure_rate(wlc,
				scb_counters, TRUE, eabuf, tx_stall->error);
		}
	}

	if (stall_detected) {
		wlc_tx_status_report_error(wlc);
	}

	return (stall_detected) ? BCME_ERROR : BCME_OK;
}

/** Input: counters structure.
 * Return :
 *   Max failure rate.
 *   tx_total - Total queued packets across all AC
 *   tx_fail  - Total failures across all AC
 *   failure_ac - AC corresponding to max failure
 * This API will clear the counters per AC if sample count is moredata
 * than sample_len
 */
uint32
wlc_tx_status_calculate_failure_rate(wlc_info_t * wlc,
	wlc_tx_stall_counters_t * counters,
	bool reset_counters, const char * prefix, wlc_tx_stall_error_info_t * error)
{
	wlc_tx_stall_info_t * tx_stall;
	uint32 tx_queued_count, tx_queued_all_ac = 0;
	uint32 tx_failure_count, tx_failure_all_ac = 0;
	uint32 max_failure_ac = ID32_INVALID, failure_sample_len = ID32_INVALID;
	uint32 idx, ac;
	uint32 failure_rate, max_failure_rate = 0;
	uint32 bitmap, bitmap1;
	uint32 stall_detected = 0;
	uint32 max_failure_bitmap = 0, max_failure_bitmap1 = 0;

	tx_stall = wlc->tx_stall;

	if (tx_stall == NULL) {
		return 0;
	}

	/* Verify stall on each access category */
	for (ac = AC_BE; ac < AC_COUNT; ac++) {
		tx_queued_count = counters->tx_stats[ac][0];
		tx_failure_count = 0;
		bitmap = bitmap1 = 0;

		/* Count the total failures for all required reasons */
		for (idx = 1; idx < WLC_TX_STS_MAX; idx++) {

			if (idx < 32) {
				/* Count only the failures not in the excluded list */
				if (!isset(&tx_stall->exclude_bitmap, idx)) {
					tx_failure_count += counters->tx_stats[ac][idx];
					if (counters->tx_stats[ac][idx]) {
						setbit(&bitmap, idx);
					}
				}
			} else if (idx < 64) {
				if (!isset(&tx_stall->exclude_bitmap1, (idx - 32))) {
					tx_failure_count += counters->tx_stats[ac][idx];
					if (counters->tx_stats[ac][idx]) {
						setbit(&bitmap1, (idx - 32));
					}
				}
			}

			if (counters->tx_stats[ac][idx]) {
				WL_TRACE(("\t%s: TX Failure on ac:%d, Failure reason:%d,"
					"total_tx:%d, failed:%d\n",
						prefix ? prefix : "", ac, idx,
						tx_queued_count, counters->tx_stats[ac][idx]));
			}
		}

		if (tx_queued_count > tx_stall->sample_len) {
			failure_rate = ((tx_failure_count * 100) / tx_queued_count);

			WL_TRACE(("%s:ac:%d, Failure threshold: %d, count:%d, timeout:%d\n",
				prefix ? prefix : "??",
				ac, failure_rate, counters->stall_count[ac], tx_stall->timeout));

			if (failure_rate > tx_stall->stall_threshold) {
				counters->stall_count[ac]++;

				if (counters->stall_count[ac] >= tx_stall->timeout) {
					stall_detected++;
					counters->stall_count[ac] = 0;
				}
			} else {
				counters->stall_count[ac] = 0;
			}

			if (reset_counters) {
				/* Clear counters */
				memset(&counters->tx_stats[ac],
					0x00, sizeof(counters->tx_stats[ac]));

				/* Holds the time since counters started */
				counters->sysup_time[ac] = wlc->pub->now;
			}
		} else {
			failure_rate = 0;
		}

		/* Cache the max failure rate and corresponding AC */
		if (failure_rate > max_failure_rate) {
			max_failure_rate = failure_rate;
			max_failure_ac = ac;
			failure_sample_len = tx_queued_count;
			max_failure_bitmap = bitmap;
			max_failure_bitmap1 = bitmap1;
		}

		tx_queued_all_ac += tx_queued_count;
		tx_failure_all_ac += tx_failure_count;

		if (failure_rate > tx_stall->stall_threshold) {
			WL_ERROR(("%s: AC:%d Total queued:%d, total failures:%d, threshold:%d\n",
				prefix ? prefix : "", ac, tx_queued_count,
				tx_failure_count, failure_rate));
		} else if (tx_queued_count || tx_failure_count) {
			WL_TRACE(("%s: AC:%d Total queued:%d, total failures:%d, threshold:%d\n",
				prefix ? prefix : "", ac, tx_queued_count,
				tx_failure_count, failure_rate));
		}
	}

	/* Update error info if failure rate exceeds the threshold */
	if (error && (max_failure_rate > tx_stall->stall_threshold)) {
		error->stall_bitmap = max_failure_bitmap;
		error->stall_bitmap1 = max_failure_bitmap1;
		error->failure_ac = max_failure_ac;
		error->timeout = tx_stall->timeout;
		error->sample_len = failure_sample_len;
		error->threshold = max_failure_rate;
		error->tx_all = tx_queued_all_ac;
		error->tx_failure_all = tx_failure_all_ac;

		if (prefix) {
			strncpy(error->reason, prefix, sizeof(error->reason));
			error->reason[sizeof(error->reason) - 1] = '\0';
		}

	}

	return stall_detected;
}

wlc_tx_stall_error_info_t *
wlc_tx_status_get_error_info(wlc_info_t * wlc)
{
	if (wlc && wlc->tx_stall) {
		return wlc->tx_stall->error;
	}

	return NULL;
}

int32
wlc_tx_status_params(wlc_tx_stall_info_t * tx_stall, bool set, int param, int value)
{
	int32 ret = BCME_OK;

	if (set) {
		switch (param) {
			case WLC_TX_STALL_IOV_THRESHOLD:
				tx_stall->stall_threshold = (uint32)value;
				break;

			case WLC_TX_STALL_IOV_SAMPLE_LEN:
				tx_stall->sample_len = (uint32)value;
				break;

			case WLC_TX_STALL_IOV_TIME:
				tx_stall->timeout = (uint32)value;
				break;

			case WLC_TX_STALL_IOV_FORCE:
				tx_stall->force_tx_stall = (uint32)value;
				break;

			case WLC_TX_STALL_IOV_EXCLUDE:
				tx_stall->exclude_bitmap = (uint32)value;
				break;

			case WLC_TX_STALL_IOV_EXCLUDE1:
				tx_stall->exclude_bitmap1 = (uint32)value;
				break;
		}

	} else {
		switch (param) {
			case WLC_TX_STALL_IOV_THRESHOLD:
				ret = (int32)tx_stall->stall_threshold;
				break;

			case WLC_TX_STALL_IOV_SAMPLE_LEN:
				ret = (int32)tx_stall->sample_len;
				break;

			case WLC_TX_STALL_IOV_TIME:
				ret = (int32)tx_stall->timeout;
				break;

			case WLC_TX_STALL_IOV_FORCE:
				ret = (int32)tx_stall->force_tx_stall;
				break;

			case WLC_TX_STALL_IOV_EXCLUDE:
				ret = (int32)tx_stall->exclude_bitmap;
				break;

			case WLC_TX_STALL_IOV_EXCLUDE1:
				ret = (int32)tx_stall->exclude_bitmap1;
				break;
		}
	}

	return ret;
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void
wlc_tx_status_dump_counters(wlc_tx_stall_counters_t * counters,
	struct bcmstrbuf *b, const char * prefix)
{
	int i, ac;
	bool print_timestamp = TRUE;

	if (counters == NULL) {
		return;
	}
	if (b) {
		bcm_bprintf(b, "%s", prefix ? prefix : "");
	} else {
		WL_ASSOC(("%s", prefix ? prefix : ""));
	}
	for (ac = 0; ac < AC_COUNT; ac++) {
		/* Print time stamp of each AC */
		print_timestamp = TRUE;
		for (i = 0; i < WLC_TX_STS_MAX; i++) {
			if (counters->tx_stats[ac][i]) {
				if (print_timestamp) {
					if (b) {
						bcm_bprintf(b, " \tAC:%d,ts:%u,",
							ac, counters->sysup_time[ac]);
					} else {
						WL_ASSOC((" AC:%d,ts:%u,",
							ac, counters->sysup_time[ac]));
					}
					/* Print timestamp only once per AC */
					print_timestamp = FALSE;
				}

				if (b) {
					bcm_bprintf(b, "%s:%d,",
						tx_sts_string[i], counters->tx_stats[ac][i]);
				} else {
					WL_ASSOC(("  %s:%d",
						tx_sts_string[i], counters->tx_stats[ac][i]));
				}
			}
		}
	}
	if (b) {
		bcm_bprintf(b, "\n");
	} else {
		WL_ASSOC(("\n"));
	}

	return;
}

static int wlc_tx_status_tx_activity_dump(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int i;
	int hist_idx;

	wlc_tx_stall_info_t * tx_stall;
	if ((wlc == NULL) || (wlc->tx_stall == NULL) || (wlc->tx_stall->history == NULL)) {
		return BCME_ERROR;
	}
	tx_stall = wlc->tx_stall;
	hist_idx = tx_stall->history_idx;
	bcm_bprintf(b, "Current timestamp:%u\n",
		wlc->pub->now);

	for (i = 0; i < WLC_TX_STALL_COUNTERS_HISTORY_LEN; i++) {
		wlc_tx_status_dump_counters(&tx_stall->history[hist_idx], b, "");
		hist_idx++;
		if (hist_idx == WLC_TX_STALL_COUNTERS_HISTORY_LEN) {
			hist_idx = 0;
		}
	}
	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP */

#ifdef HEALTH_CHECK
static int wlc_hchk_tx_stall_check(uint8 *buffer, uint16 length, void *context,
	int16 *bytes_written)
{
	int rc = HEALTH_CHECK_STATUS_OK;
	wlc_info_t *wlc = (wlc_info_t*)context;
	wl_tx_hc_info_t hc_info;

	/* Run TX stall health check */
	if (wlc_tx_status_health_check(wlc) == BCME_OK) {
		*bytes_written = 0;
		return HEALTH_CHECK_STATUS_OK;
	}

	/* If an error is detected and space is available, client must write
	 * a XTLV record to indicate what happened.
	 * The buffer provided is word aligned.
	 */
	if (length >= sizeof(hc_info)) {
		hc_info.type = WL_HC_DD_TX_STALL;
		hc_info.length = sizeof(hc_info) - BCM_XTLV_HDR_SIZE;
		hc_info.stall_bitmap = wlc->tx_stall->error->stall_bitmap;
		hc_info.stall_bitmap1 = wlc->tx_stall->error->stall_bitmap1;
		hc_info.failure_ac = wlc->tx_stall->error->failure_ac;
		hc_info.threshold = wlc->tx_stall->error->threshold;
		hc_info.tx_all = wlc->tx_stall->error->tx_all;
		hc_info.tx_failure_all = wlc->tx_stall->error->tx_failure_all;

		bcopy(&hc_info, buffer, sizeof(hc_info));
		*bytes_written = sizeof(hc_info);
	} else {
		/* hc buffer too short */
		*bytes_written = 0;
	}

	/* overwrite the rc to return a proper status back to framework */
	rc = HEALTH_CHECK_STATUS_INFO_LOG_BUF;
#if defined(WL_DATAPATH_HC_NOASSERT)
	rc |= HEALTH_CHECK_STATUS_ERROR;
#else
	wlc->hw->need_reinit = WL_REINIT_TX_STALL;
	rc |= HEALTH_CHECK_STATUS_TRAP;
#endif

	return rc;
}
#endif /* HEALTH_CHECK */

#endif /* WL_TX_STALL */

#ifdef WL_TXQ_STALL
static int
wlc_txq_stall_check(wlc_info_t * wlc,
	struct pktq * q,
	char * info)
{
	uint prec;
	wlc_tx_stall_info_t * tx_stall = wlc->tx_stall;
	uint high_prec;
	bool report_fatal = TRUE;

	for (prec = 0; prec < q->num_prec; prec++) {

		/* Skip empty queues */
		if (pktqprec_n_pkts(q, prec) == 0) {
			/* Reset stall count to clear previous state */
			q->q[prec].stall_count = 0;
			q->q[prec].dequeue_count = 0;
			continue;
		}

		/* Check for no dequeue in last second */
		if (q->q[prec].dequeue_count == 0) {
			/* Increment stall count */
			q->q[prec].stall_count++;

			WL_TRACE(("%s,prec:%d,len:%d,tlen:%d,stall_count:%d\n",
					info,
					prec, pktqprec_n_pkts(q, prec),
					pktq_n_pkts_tot(q),
					q->q[prec].stall_count));

			if (q->q[prec].stall_count >=
				tx_stall->timeout) {
				q->q[prec].stall_count = 0;

				snprintf(tx_stall->error->reason,
					sizeof(tx_stall->error->reason) - 1,
					"%s,prec:%d,len:%d,tlen:%d",
					info,
					prec, pktqprec_n_pkts(q, prec),
					pktq_n_pkts_tot(q));

				tx_stall->error->reason[sizeof(tx_stall->error->reason) - 1] = '\0';
				WL_ERROR(("TXQ stall detected: %s\n",
					tx_stall->error->reason));

				/* Check if higher priority packets are dequeued?
				 * If yes it may not be a stall
				 */
				for (high_prec = prec + 1; high_prec < q->num_prec; high_prec++) {
					if (q->q[high_prec].dequeue_count) {
						report_fatal = FALSE;
						break;
					}
				}

				if (tx_stall->assert_on_error && report_fatal) {
					if (report_fatal) {
						tx_stall->error->stall_reason =
							WLC_TX_STALL_REASON_TXQ_NO_PROGRESS;

						wlc_bmac_report_fatal_errors(wlc->hw,
							WL_REINIT_TX_STALL);
					}
				} else {
					char log[WLC_TX_STALL_REASON_STRING_LEN];
					snprintf(log, sizeof(log) - 1, "TX_STALL{%s}",
						tx_stall->error->reason);
					log[sizeof(log) - 1] = '\0';

					wl_log_system_state(wlc->wl, log, TRUE);
				}

				return BCME_ERROR;
			}
		} else {
			q->q[prec].stall_count = 0;
			q->q[prec].dequeue_count = 0;
		}
	}

	return BCME_OK;
}

int
wlc_txq_hc_global_txq(wlc_info_t *wlc)
{
	int ret = BCME_OK;
	wlc_txq_info_t *qi;
	uint cnt = 0;
	char info[WLC_TX_STALL_REASON_STRING_LEN];

	/* Check for all queues for possible stall */
	for (qi = wlc->tx_queues; qi; qi = qi->next, cnt++) {
		snprintf(info, sizeof(info), "txq:%d%s%s",
			cnt,
			(wlc->active_queue == qi ? "(Active Q)" : ""),
#ifdef WL_MULTIQUEUE
			(wlc->primary_queue == qi ? "(Primry Q)" :
			wlc->excursion_queue == qi ? "(Excursion Q)" : ""));
#else
			"");
#endif
		info[sizeof(info) - 1] = '\0';
		ret = wlc_txq_stall_check(wlc, &qi->txq, info);
	}

	return ret;
}

#ifdef WLAMPDU
int
wlc_txq_hc_ampdu_txq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb)
{
	int ret = BCME_OK;
	char info[WLC_TX_STALL_REASON_STRING_LEN];
	struct pktq * pktq;
	char eabuf[ETHER_ADDR_STR_LEN];

	/* Check for stalled tx ampdu queue */
	if (wlc->ampdu_tx &&
		SCB_AMPDU(scb) &&
		(pktq = scb_ampdu_prec_pktq(wlc, scb)) &&
		(pktq_n_pkts_tot(pktq))) {
			snprintf(info, sizeof(info), "txq:ampdu,cfg:%s,scb:%s",
				wl_ifname(wlc->wl, cfg->wlcif->wlif),
				bcm_ether_ntoa(&scb->ea, eabuf));

			info[sizeof(info) - 1] = '\0';
			ret = wlc_txq_stall_check(wlc, pktq, info);
	}

	return ret;
}

int
wlc_txq_hc_ampdu_rxq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb)
{
	int ret = BCME_OK;
	char eabuf[ETHER_ADDR_STR_LEN];
	int idx;
	wlc_tx_stall_info_t * tx_stall = wlc->tx_stall;

	/* Check for stalled RX reorder queue */
	if (wlc->ampdu_rx &&
		SCB_AMPDU(scb)) {
		for (idx = 0; idx < AMPDU_MAX_SCB_TID; idx++) {
			uint ts;
			if (wlc_ampdu_rx_queued_pkts(
				wlc->ampdu_rx, scb, idx, &ts)) {
				if ((wlc->pub->now - ts) >
					tx_stall->timeout) {
					char * tmp_err = tx_stall->error->reason;
					int tmp_len =
						sizeof(tx_stall->error->reason);
					snprintf(tmp_err, tmp_len,
						"rxq:ampdu_rx,cfg:%s,scb:%s",
						wl_ifname(wlc->wl,
							cfg->wlcif->wlif),
						bcm_ether_ntoa(&scb->ea, eabuf));
					tmp_err[tmp_len - 1] = '\0';
					if (tx_stall->assert_on_error) {
						tx_stall->error->stall_reason =
							WLC_TX_STALL_REASON_RXQ_NO_PROGRESS;

						wlc_bmac_report_fatal_errors(wlc->hw,
							WL_REINIT_TX_STALL);
					} else {
						wl_log_system_state(wlc->wl, tmp_err, TRUE);
					}
					ret = BCME_ERROR;
				}
			}
		}
	}
	return ret;
}
#endif /* WLAMPDU */


#ifdef AP
int
wlc_txq_hc_ap_psq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb)
{
	int ret = BCME_OK;
	char eabuf[ETHER_ADDR_STR_LEN];
	char info[WLC_TX_STALL_REASON_STRING_LEN];
	struct pktq * pktq;

	/* Check for stalled packets in AP PSQ */
	if (wlc->psinfo &&
		BSSCFG_AP(cfg) &&
		SCB_PS(scb) &&
		(pktq = wlc_apps_get_psq(wlc, scb)) &&
		pktq_n_pkts_tot(pktq)) {
		snprintf(info, sizeof(info), "txq:appsq,cfg:%s,scb:%s",
			wl_ifname(wlc->wl, cfg->wlcif->wlif),
			bcm_ether_ntoa(&scb->ea, eabuf));
		info[sizeof(info) - 1] = '\0';
		ret = wlc_txq_stall_check(wlc, pktq, info);
	}
	return ret;
}
#endif /* AP */

#ifdef WL_BSSCFG_TX_SUPR
int
wlc_txq_hc_supr_txq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb)
{
	int ret = BCME_OK;
	struct pktq * pktq;
	char info[WLC_TX_STALL_REASON_STRING_LEN];
	char eabuf[ETHER_ADDR_STR_LEN];

	pktq = wlc_bsscfg_get_psq(wlc->psqi, cfg);
	/* Check for stalled packets in TX suppression Q */
	if (pktq &&
		pktq_n_pkts_tot(pktq)) {
		snprintf(info, sizeof(info), "txq:suppq,cfg:%s,scb:%s",
			wl_ifname(wlc->wl, cfg->wlcif->wlif),
			bcm_ether_ntoa(&scb->ea, eabuf));
		info[sizeof(info) - 1] = '\0';
		ret = wlc_txq_stall_check(wlc, pktq, info);
	}
	return ret;
}
#endif /* WL_BSSCFG_TX_SUPR */
/**
 * Routine to iterate all TXQs in the system and validate for stalls
 */
int
wlc_txq_health_check(wlc_info_t *wlc)
{
	uint idx = 0;
	wlc_bsscfg_t * cfg;
	struct scb *scb;
	struct scb_iter scbiter;
	wlc_tx_stall_info_t * tx_stall;

	if (wlc == NULL) {
		return BCME_BADARG;
	}

	tx_stall = wlc->tx_stall;

	if (tx_stall->timeout == 0) {
		return BCME_NOTREADY;
	}

	wlc_txq_hc_global_txq(wlc);

	FOREACH_BSS(wlc, idx, cfg) {
		/* Iterate each SCB and verify AMPDU and AWDL RCQ */
		FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
#ifdef AMPDU
			wlc_txq_hc_ampdu_txq(wlc, cfg, scb);

			wlc_txq_hc_ampdu_rxq(wlc, cfg, scb);
#endif


#ifdef AP
			wlc_txq_hc_ap_psq(wlc, cfg, scb);
#endif

#ifdef WL_BSSCFG_TX_SUPR
			wlc_txq_hc_supr_txq(wlc, cfg, scb);
#endif
		}

	}

#if !defined(TXQ_MUX)
	/* Note: TXQ_MUX does not queue pkts for AMSDU, so there is only a tx healthcheck
	 * on AMSDU when !TXQ_MUX.
	 */
	if (AMSDU_TX_ENAB(wlc->pub)) {
		return wlc_amsdu_tx_health_check(wlc);
	}
#endif /* TXQ_MUX */

	return BCME_OK;
}
#endif /* WL_TXQ_STALL */

#if defined(BCMDBG) || defined(BCMDBG_DUMP)

#ifdef WLAMPDU
void
wlc_txq_dump_ampdu_txq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb, struct bcmstrbuf *b)
{
	struct pktq * pktq;
	/* Print # of packets queued in tx ampdu queue */
	if (wlc->ampdu_tx && SCB_AMPDU(scb) &&
		(pktq = scb_ampdu_prec_pktq(wlc, scb)) &&
		(pktq_n_pkts_tot(pktq))) {
		bcm_bprintf(b, "\t\t\t\tSCB TX AMPDU Q len %d\n", pktq_n_pkts_tot(pktq));
		wlc_pktq_dump(wlc, pktq, NULL, scb, "\t\t\t\t\t", b);
	}
}

void
wlc_txq_dump_ampdu_rxq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb, struct bcmstrbuf *b)
{
	int idx, queued;

	/* Print # of packets queued in RX reorder queue */
	if (wlc->ampdu_rx && SCB_AMPDU(scb)) {
		for (idx = 0; idx < AMPDU_MAX_SCB_TID; idx++) {
			uint ts;
			if ((queued = wlc_ampdu_rx_queued_pkts(wlc->ampdu_rx, scb, idx, &ts))) {
				wlc_ampdu_rx_dump_queued_pkts(wlc->ampdu_rx, scb, idx, b);
			}
		}
	}
}
#endif /* WLAMPDU */

#if defined(WLAMSDU_TX) && !defined(TXQ_MUX)
void
wlc_txq_dump_amsdu_txq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb, struct bcmstrbuf *b)
{
	int idx, queued;

	if (wlc->ami &&
		SCB_AMSDU(scb)) {
		for (idx = 0; idx < NUMPRIO; idx++) {
			uint ts;
			if ((queued = wlc_amsdu_tx_queued_pkts(wlc->ami, scb, idx, &ts))) {
				bcm_bprintf(b, "\t\t\t\tSCB TX AMSDU Q: TID %d, "
					"queued %d at %u\n", idx, queued, ts);
			}
		}
	}
}
#endif /* WLAMSDU_TX && !TXQ_MUX */

#ifdef AP
void
wlc_txq_dump_ap_psq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb, struct bcmstrbuf *b)
{
	struct pktq * pktq;
	int queued;

	/* Dump APPS PSQ */
	if (wlc->psinfo && BSSCFG_AP(cfg) && SCB_PS(scb) &&
		(pktq = wlc_apps_get_psq(wlc, scb)) && (queued = pktq_n_pkts_tot(pktq))) {
		bcm_bprintf(b, "\t\t\t\tSCB APPS Q len %d\n", queued);
		wlc_pktq_dump(wlc, pktq, NULL, scb, "\t\t\t\t\t", b);
	}
}
#endif /* AP */

#ifdef WL_BSSCFG_TX_SUPR
void
wlc_txq_dump_supr_txq(wlc_info_t *wlc, wlc_bsscfg_t * cfg, struct scb *scb, struct bcmstrbuf *b)
{
	struct pktq * pktq;
	int queued;

	/* Dump suppression Q */
	pktq = wlc_bsscfg_get_psq(wlc->psqi, cfg);
	if (pktq && (queued = pktq_n_pkts_tot(pktq))) {
		bcm_bprintf(b, "\t\t\t\t SUPPR Q len %d\n", queued);
		wlc_pktq_dump(wlc, pktq, NULL, scb, "\t\t\t\t\t", b);
	}
}
#endif /* WL_BSSCFG_TX_SUPR */

static int
wlc_datapath_dump(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	wlc_txq_info_t *qi;
	uint idx = 0, cnt = 0;
	struct pktq * pktq;
	wlc_bsscfg_t * cfg;
	struct scb *scb;
	struct scb_iter scbiter;
	int scb_cnt;

	if ((wlc == NULL) || (b == NULL))  {
		return BCME_BADARG;
	}


	/* Dump FIFO queued states */
	bcm_bprintf(b, "FIFO_PKTS: BK:%d, BE:%d, VI:%d, VO:%d, BCMC:%d, ATIM:%d\n",
		TXPKTPENDGET(wlc, TX_AC_BK_FIFO), TXPKTPENDGET(wlc, TX_AC_BE_FIFO),
		TXPKTPENDGET(wlc, TX_AC_VI_FIFO), TXPKTPENDGET(wlc, TX_AC_VO_FIFO),
		TXPKTPENDGET(wlc, TX_BCMC_FIFO), TXPKTPENDGET(wlc, TX_ATIM_FIFO));

	/* Dump all txq states */
	for (qi = wlc->tx_queues; qi; qi = qi->next, cnt++) {

		pktq = &qi->txq;

		bcm_bprintf(b, "tx_queue(%d) = %p, qlen:%u, max:%u, stopped = 0x%x %s%s\n",
			cnt, OSL_OBFUSCATE_BUF(qi), pktq_n_pkts_tot(pktq),
			pktq_max(pktq), qi->stopped,
			(wlc->active_queue == qi ? "(Active Q)" : ""),
#ifdef WL_MULTIQUEUE
			(wlc->primary_queue == qi ? "(Primry Q)" :
			wlc->excursion_queue == qi ? "(Excursion Q)" : ""));
#else
			"");
#endif

		/* Dump interfaces associated with this queue and pending packets per bsscfg */
		FOREACH_BSS(wlc, idx, cfg) {
			if ((cfg->wlcif != NULL) && (cfg->wlcif->qi == qi)) {
				char ifname[32];

				strncpy(ifname, wl_ifname(wlc->wl, cfg->wlcif->wlif),
					sizeof(ifname));
				ifname[sizeof(ifname) - 1] = '\0';
				bcm_bprintf(b, "\tbsscfg %d (%s)\n", cfg->_idx, ifname);

				/* Dump packets queued for this bsscfg */
				wlc_pktq_dump(wlc, pktq, cfg, NULL, "\t\t", b);

				scb_cnt = 0;
				/* Dump packets queued for each scb */
				FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
					char eabuf[ETHER_ADDR_STR_LEN];
					int queued;
					struct pktq * tmp_pktq;
					BCM_REFERENCE(queued);
					BCM_REFERENCE(tmp_pktq);

					bcm_bprintf(b, "\t\t\tSCB:%3d%s %s\n",
						scb_cnt, (scb->permanent? "*" : " "),
						bcm_ether_ntoa(&scb->ea, eabuf));
					wlc_pktq_dump(wlc, pktq, NULL, scb, "\t\t\t\t", b);

#ifdef WLAMPDU
					wlc_txq_dump_ampdu_txq(wlc, cfg, scb, b);

					wlc_txq_dump_ampdu_rxq(wlc, cfg, scb, b);
#endif /* WLAMPDU */

#if defined(WLAMSDU_TX) && !defined(TXQ_MUX)
					wlc_txq_dump_amsdu_txq(wlc, cfg, scb, b);
#endif


#ifdef AP
					wlc_txq_dump_ap_psq(wlc, cfg, scb, b);
#endif

#ifdef WL_BSSCFG_TX_SUPR
					wlc_txq_dump_supr_txq(wlc, cfg, scb, b);
#endif
					scb_cnt++;
				}

			}
		}
	}
	return BCME_OK;
}

static int
wlc_pktq_dump(wlc_info_t *wlc, struct pktq *q, wlc_bsscfg_t *bsscfg, struct scb * scb,
	const char * prefix, struct bcmstrbuf *b)
{
	uint prec;
	void * p;
	uint bsscfg_pkt_count, scb_pkt_count;

	/* Print the details of packets held in the queue */
	for (prec = 0; prec < q->num_prec; prec++) {

		/* Do not print empty queues */
		if (pktqprec_n_pkts(q, prec) == 0) {
			continue;
		}

		bsscfg_pkt_count = 0;
		scb_pkt_count = 0;

		for (p = pktqprec_peek(q, prec); (p != NULL); p = PKTLINK(p)) {
			if (bsscfg && (wlc->bsscfg[WLPKTTAGBSSCFGGET(p)] == bsscfg)) {
				bsscfg_pkt_count++;
			}

			if (scb && (WLPKTTAGSCBGET(p) == scb)) {
				scb_pkt_count++;
			}
		}

		/* print packets queued for all bsscfg and all SCB */
		if ((bsscfg == NULL) && (scb == NULL)) {
			bcm_bprintf(b, "%sprec %u qpkts %u, max_pkts:%u\n", prefix ? prefix : "",
				prec, pktqprec_n_pkts(q, prec), pktqprec_max_pkts(q, prec));
		} else if (bsscfg == NULL) {
			/* print packets queued for input scb */
			if (scb_pkt_count) {
				bcm_bprintf(b, "%sprec %u scb_pkts %u, \n", prefix ? prefix : "",
					prec, scb_pkt_count);
			}
		} else if (scb == NULL) {
			/* print packets queued for input bsscfg */
			if (bsscfg_pkt_count) {
				bcm_bprintf(b, "%sprec %u bss_pkts %u\n", prefix ? prefix : "",
					prec, bsscfg_pkt_count);
			}
		} else {
			/* print packets queued for both bsscfg and scb */
			bcm_bprintf(b, "%sprec %u qpkts %u, max_pkts:%u, bss_pkts %u,"
				" scb_pkts %u\n", prefix ? prefix : "",
				prec, pktqprec_n_pkts(q, prec), pktqprec_max_pkts(q, prec),
				bsscfg_pkt_count, scb_pkt_count);
		}
	}

	return BCME_OK;
}
#endif /* BCMDBG || BCMDBG_DUMP */

#if defined(HC_TX_HANDLER)
int
wlc_hc_tx_set(void *ctx, const uint8 *buf, uint16 type, uint16 len)
{
	struct wlc_hc_ctx *hc_ctx = ctx;
	wlc_info_t *wlc = hc_ctx->wlc;
	int32 val;
	uint16 expect_len;
	int err = BCME_OK;

	/* most values are int32, with the exclude list being the exception */
	if (type == WL_HC_TX_XTLV_ID_VAL_STALL_EXCLUDE) {
		expect_len = (2 * sizeof(int32));
	} else {
		expect_len = sizeof(int32);
	}

	if (len < expect_len) {
		return BCME_BUFTOOSHORT;
	} else if (len > expect_len) {
		return BCME_BUFTOOLONG;
	}

	val = ((int32*)buf)[0];
	val = ltoh32(val);

	switch (type) {
	case WL_HC_TX_XTLV_ID_VAL_STALL_THRESHOLD:
		wlc_tx_status_params(wlc->tx_stall, TRUE, WLC_TX_STALL_IOV_THRESHOLD, val);
		break;
	case WL_HC_TX_XTLV_ID_VAL_STALL_SAMPLE_SIZE:
		wlc_tx_status_params(wlc->tx_stall, TRUE, WLC_TX_STALL_IOV_SAMPLE_LEN, val);
		break;
	case WL_HC_TX_XTLV_ID_VAL_STALL_TIMEOUT:
		wlc_tx_status_params(wlc->tx_stall, TRUE, WLC_TX_STALL_IOV_TIME, val);
		break;
	case WL_HC_TX_XTLV_ID_VAL_STALL_FORCE:
		wlc_tx_status_params(wlc->tx_stall, TRUE, WLC_TX_STALL_IOV_FORCE, val);
		break;
	case WL_HC_TX_XTLV_ID_VAL_STALL_EXCLUDE: {
		/* exclude bitmap is 2 words, size of buf checked above */
		const int32 *bitmap = (const int32*)buf;

		val = ltoh32(bitmap[0]);
		wlc_tx_status_params(wlc->tx_stall, TRUE, WLC_TX_STALL_IOV_EXCLUDE, val);
		val = ltoh32(bitmap[1]);
		wlc_tx_status_params(wlc->tx_stall, TRUE, WLC_TX_STALL_IOV_EXCLUDE1, val);
		break;
	}
	default:
		err = BCME_BADOPTION;
		break;
	}
	return err;
}

int
wlc_hc_tx_get(wlc_info_t *wlc, wlc_if_t *wlcif, bcm_xtlv_t *params, void *out, uint o_len)
{
	bcm_xtlv_t *hc_tx;
	bcm_xtlvbuf_t tbuf;
	uint32 val;
	int err = BCME_OK;
	/* local list for params copy, or for request of all attributes */
	uint16 req_id_list[] = {
		WL_HC_TX_XTLV_ID_VAL_STALL_THRESHOLD,
		WL_HC_TX_XTLV_ID_VAL_STALL_SAMPLE_SIZE,
		WL_HC_TX_XTLV_ID_VAL_STALL_TIMEOUT,
		WL_HC_TX_XTLV_ID_VAL_STALL_EXCLUDE,
	};
	uint i, req_id_count;
	uint16 val_id;

	/* The input params are expected to be in the same memory as the
	 * output buffer, so save the parameter list.
	 */

	/* size (in elements) of the req buffer */
	req_id_count = ARRAYSIZE(req_id_list);

	err = wlc_hc_unpack_idlist(params, req_id_list, &req_id_count);
	if (err) {
		return err;
	}

	/* start formatting the output buffer */

	/* HC container XTLV comes first */
	if (o_len < BCM_XTLV_HDR_SIZE) {
		return BCME_BUFTOOSHORT;
	}

	hc_tx = out;
	hc_tx->id = htol16(WL_HC_XTLV_ID_CAT_DATAPATH_TX);
	/* fill out hc_tx->len when the sub-tlvs are formatted */

	/* adjust len for the hc_tx header */
	o_len -= BCM_XTLV_HDR_SIZE;

	/* bcm_xtlv_buf_init() takes length up to uint16 */
	o_len = MIN(o_len, 0xFFFF);

	err = bcm_xtlv_buf_init(&tbuf, hc_tx->data, (uint16)o_len, BCM_XTLV_OPTION_ALIGN32);
	if (err) {
		return err;
	}

	/* walk the requests and write the value to the 'out' buffer */
	for (i = 0; !err && i < req_id_count; i++) {
		val_id = req_id_list[i];

		if (val_id == WL_HC_TX_XTLV_ID_VAL_STALL_EXCLUDE) {
			/* exclude bitmap is 2 words */
			uint32 bitmap[2];

			/* XTLV packs all as LE */
			bitmap[0] = (uint32)wlc_tx_status_params(wlc->tx_stall, FALSE,
			                                         WLC_TX_STALL_IOV_EXCLUDE, 0);
			bitmap[1] = (uint32)wlc_tx_status_params(wlc->tx_stall, FALSE,
			                                         WLC_TX_STALL_IOV_EXCLUDE1, 0);

			bitmap[0] = htol32(bitmap[0]);
			bitmap[1] = htol32(bitmap[1]);

			/* pack an XTLV with the bitmap */
			err = bcm_xtlv_put_data(&tbuf, val_id,
				(const uint8 *)&bitmap, sizeof(bitmap));
		} else {
			int id;

			switch (val_id) {
			case WL_HC_TX_XTLV_ID_VAL_STALL_THRESHOLD:
				id = WLC_TX_STALL_IOV_THRESHOLD;
				break;
			case WL_HC_TX_XTLV_ID_VAL_STALL_SAMPLE_SIZE:
				id = WLC_TX_STALL_IOV_SAMPLE_LEN;
				break;
			case WL_HC_TX_XTLV_ID_VAL_STALL_TIMEOUT:
				id = WLC_TX_STALL_IOV_TIME;
				break;
			default:
				return BCME_BADOPTION; /* unknown attribute ID */
			}

			val = wlc_tx_status_params(wlc->tx_stall, FALSE, id, 0);

			/* pack an XTLV with the single value */
			err = bcm_xtlv_put32(&tbuf, val_id, &val, 1);
		}
	}

	if (!err) {
		/* now we can fill out the container XTLV length */
		hc_tx->len = htol16(bcm_xtlv_buf_len(&tbuf));
	}

	return err;
}
#endif /* HC_TX_HANDLER */
