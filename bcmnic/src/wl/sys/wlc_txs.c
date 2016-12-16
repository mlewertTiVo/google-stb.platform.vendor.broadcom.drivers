/**
 * @file
 * @brief
 * txstatus processing/debugging
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
 * $Id: wlc_txs.c 664895 2016-10-14 03:04:20Z $
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
#include <wlc_bsscfg.h>
#include <wlc_mbss.h>
#include <wlc.h>
#include <wlc_hw.h>
#include <wlc_apps.h>
#include <wlc_scb.h>
#include <wlc_antsel.h>
#ifdef WLNAR
#include <wlc_nar.h>
#endif
#if defined(SCB_BS_DATA)
#include <wlc_bs_data.h>
#endif /* SCB_BS_DATA */
#include <wlc_ampdu.h>
#ifdef WLMCNX
#include <wlc_mcnx.h>
#endif
#include <wlc_p2p.h>
#include <wlc_scb_ratesel.h>
#ifdef WL_LPC
#include <wlc_scb_powersel.h>
#endif
#include <wlc_assoc.h>
#if defined(PROP_TXSTATUS)
#include <wlc_wlfc.h>
#endif
#include <wlc_txbf.h>
#include <wlc_pcb.h>
#include <wlc_pm.h>
#include <wlc_tx.h>
#include <wlc_bsscfg_psq.h>
#include <wlc_pspretend.h>
#include <wlc_qoscfg.h>
#include <wlc_event_utils.h>
#include <wlc_perf_utils.h>
#include <wlc_txs.h>
#include <wlc_phy_hal.h>
#include <wlc_dbg.h>
#include <wlc_macdbg.h>
#ifdef WLC_SW_DIVERSITY
#include <wlc_swdiv.h>
#endif
#include <wlc_log.h>
#ifdef WL_MU_TX
#include <wlc_mutx.h>
#endif
#include <wlc_vht.h>
#include <wlc_bmac.h>
#ifdef WL_LEAKY_AP_STATS
#include <wlc_leakyapstats.h>
#endif /* WL_LEAKY_AP_STATS */

static void wlc_pkttag_scb_restore_ex(wlc_info_t* wlc, void* p, wlc_txh_info_t* txh_info,
	wlc_bsscfg_t **bsscfg_out, struct scb **scb_out);

#if defined(BCMDBG) || defined(ENABLE_CORECAPTURE)
static void
wlc_print_txs_status_raw(uint16 s)
{
	printf("Raw\n [15:12]  %d  frame attempts\n",
	       (s & TX_STATUS_FRM_RTX_MASK) >> TX_STATUS_FRM_RTX_SHIFT);
	printf(" [11:8]  %d  rts attempts\n",
	       (s & TX_STATUS_RTS_RTX_MASK) >> TX_STATUS_RTS_RTX_SHIFT);
	printf("    [7]  %d  PM\n", ((s & TX_STATUS_PMINDCTD) ? 1 : 0));
	printf("    [6]  %d  intermediate status\n", ((s & TX_STATUS_INTERMEDIATE) ? 1 : 0));
	printf("    [5]  %d  AMPDU\n", (s & TX_STATUS_AMPDU) ? 1 : 0);
	printf("  [4:2]  %d  Suppress Reason\n",
	   ((s & TX_STATUS_SUPR_MASK) >> TX_STATUS_SUPR_SHIFT));
	printf("    [1]  %d  acked\n", ((s & TX_STATUS_ACK_RCV) ? 1 : 0));
}

static void
wlc_print_txs_status_aqm_raw(tx_status_t* txs)
{
	uint16 status_bits = txs->status.raw_bits;

	printf("Raw\n[0]    %d Valid\n", ((status_bits & TX_STATUS_VALID) != 0));
	printf("[2]    %d IM\n", ((status_bits & TX_STATUS40_INTERMEDIATE) != 0));
	printf("[3]    %d PM\n", ((status_bits & TX_STATUS40_PMINDCTD) != 0));
	printf("[7-4]  %d Suppr\n",
		((status_bits & TX_STATUS40_SUPR) >> TX_STATUS40_SUPR_SHIFT));
	printf("[14:8] %d Ncons\n", ((status_bits & TX_STATUS40_NCONS) >> TX_STATUS40_NCONS_SHIFT));
	printf("[15]   %d Acked\n", (status_bits & TX_STATUS40_ACK_RCV) != 0);
}
#endif /* BCMDBG || ENABLE_CORECAPTURE */

void
wlc_print_txstatus(wlc_info_t* wlc, tx_status_t* txs)
{
#if defined(BCMDBG) || defined(BCMDBG_ERR) || defined(WLMSG_PRHDRS) || \
	defined(WLMSG_PRPKT) || defined(ENABLE_CORECAPTURE)
	uint16 s = txs->status.raw_bits;
	static const char *supr_reason[] = {
		"None", "PMQ Entry", "Flush request",
		"Previous frag failure", "Channel mismatch",
		"Lifetime Expiry", "Underflow", "AB NACK or TX SUPR"
	};

	printf("\ntxpkt (MPDU) Complete\n");

	printf("FrameID: 0x%04x   ", txs->frameid);
	printf("Seq: 0x%04x   ", txs->sequence);
	printf("TxStatus: 0x%04x", s);
	printf("\n");

	printf("ACK %d IM %d PM %d Suppr %d (%s)\n",
	       txs->status.was_acked, txs->status.is_intermediate,
	       txs->status.pm_indicated, txs->status.suppr_ind,
	       (txs->status.suppr_ind < ARRAYSIZE(supr_reason) ?
	        supr_reason[txs->status.suppr_ind] : "Unkn supr"));
	printf("CNT(rts_tx)=%d CNT(frag_tx_cnt)=%d CNT(cts_rx_cnt)=%d\n",
		txs->status.rts_tx_cnt, txs->status.frag_tx_cnt,
		txs->status.cts_rx_cnt);

	printf("DequeueTime: 0x%08x ", txs->dequeuetime);
	printf("LastTxTime: 0x%08x ", txs->lasttxtime);
	printf("PHYTxErr:   0x%04x ", txs->phyerr);
	printf("RxAckRSSI: 0x%04x ", (txs->ackphyrxsh & PRXS1_JSSI_MASK) >> PRXS1_JSSI_SHIFT);
	printf("RxAckSQ: 0x%04x", (txs->ackphyrxsh & PRXS1_SQ_MASK) >> PRXS1_SQ_SHIFT);
	printf("\n");

#if defined(BCMDBG) || defined(ENABLE_CORECAPTURE)
	if (txs->phyerr) {
		wlc_dump_phytxerr(wlc, txs->phyerr);
	}

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		wlc_print_txs_status_raw(txs->status.raw_bits);
	} else {
		wlc_print_txs_status_aqm_raw(txs);
		if ((txs->status.s5 & TX_STATUS64_MUTX)) {
			printf("MUTX: mcs %d nss %d sgi %d bw %d txpwr %d\n"
				"gid %d gpos %d gbmp 0x%x mutxcnt %d snr %d\n",
				TX_STATUS64_MU_MCS(txs->status.s4),
				TX_STATUS64_MU_NSS(txs->status.s4),
				TX_STATUS64_MU_SGI(txs->status.s3),
				TX_STATUS64_MU_BW(txs->status.s3),
				TX_STATUS64_MU_TXPWR(txs->status.s3),
				TX_STATUS64_MU_GID(txs->status.s3),
				TX_STATUS64_MU_GPOS(txs->status.s4),
				TX_STATUS64_MU_GBMP(txs->status.s4),
				TX_STATUS64_MU_TXCNT(txs->status.s4),
				TX_STATUS64_MU_SNR(txs->status.s4));
		}
	}
	printf("txpktpend AC_BK %d AC_BE %d AC_VI %d AC_VO %d BCMC %d ATIM %d\n",
		TXPKTPENDGET(wlc, TX_AC_BK_FIFO), TXPKTPENDGET(wlc, TX_AC_BE_FIFO),
		TXPKTPENDGET(wlc, TX_AC_VI_FIFO), TXPKTPENDGET(wlc, TX_AC_VO_FIFO),
		TXPKTPENDGET(wlc, TX_BCMC_FIFO), TXPKTPENDGET(wlc, TX_ATIM_FIFO));
#endif /* BCMDBG || ENABLE_CORECAPTURE */
#endif	/* BCMDBG || BCMDBG_ERR || WLMSG_PRHDRS || WLMSG_PRPKT */
}

static void
wlc_txs_clear_supr(wlc_info_t *wlc, tx_status_macinfo_t *status)
{
	ASSERT(status);
	status->suppr_ind = TX_STATUS_SUPR_NONE;
	if (D11REV_LT(wlc->pub->corerev, 40)) {
		status->raw_bits &= ~TX_STATUS_SUPR_MASK;
	} else {
		status->raw_bits &= ~TX_STATUS40_SUPR;
	}
}

uint16
wlc_txs_alias_to_old_fmt(wlc_info_t *wlc, tx_status_macinfo_t* status)
{
	ASSERT(status);

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		return status->raw_bits;
	} else {
		uint16 status_bits = 0;

		if (status->is_intermediate) {
			status_bits |= TX_STATUS_INTERMEDIATE;
		}
		if (status->pm_indicated) {
			status_bits |= TX_STATUS_PMINDCTD;
		}
		status_bits |= (status->suppr_ind << TX_STATUS_SUPR_SHIFT);
		if (status->was_acked) {
			status_bits |= TX_STATUS_ACK_RCV;
		}
		/* frag tx cnt */
		status_bits |= (TX_STATUS_FRM_RTX_MASK &
			(status->frag_tx_cnt << TX_STATUS_FRM_RTX_SHIFT));
		return status_bits;
	}
}

static INLINE bool
wlc_txs_reg_mpdu(wlc_info_t *wlc, tx_status_t *txs)
{
	if (D11REV_LT(wlc->pub->corerev, 40)) {
		return ((txs->status.raw_bits & TX_STATUS_AMPDU) == 0);
	} else {
		/* valid indications are lacking for rev >= 40 */
		return FALSE;
	}
}

static void
wlc_txs_dump_status_info(wlc_info_t *wlc, tx_status_t *txs, uint queue)
{
	if (D11REV_LT(wlc->pub->corerev, 40)) {
		WL_ERROR(("wl%d raw txstatus %04X %04X %04X %04X\n",
			wlc->pub->unit, txs->status.raw_bits,
			txs->frameid, txs->sequence, txs->phyerr));
	} else {
		WL_ERROR(("wl%d raw txstatus %04X %04X | %04X %04X | %08X %08X || "
			"%08X %08X | %08X\n", wlc->pub->unit,
			txs->status.raw_bits, txs->frameid, txs->sequence, txs->phyerr,
			txs->status.s3, txs->status.s4, txs->status.s5,
			txs->status.ack_map1, txs->status.ack_map2));
	}
	WL_ERROR(("pktpend: %d %d %d %d %d \n",
		TXPKTPENDGET(wlc, TX_AC_BK_FIFO),
		TXPKTPENDGET(wlc, TX_AC_BE_FIFO),
		TXPKTPENDGET(wlc, TX_AC_VI_FIFO),
		TXPKTPENDGET(wlc, TX_AC_VO_FIFO),
		TXPKTPENDGET(wlc, TX_BCMC_FIFO)));
	if (MU_TX_ENAB(wlc)) {
		int i = 8;
		for (; i < NFIFO_EXT; i += 3) {
			WL_ERROR(("Fifo %2d: %-4d %-4d %-4d\n", i,
				TXPKTPENDGET(wlc, i),
				TXPKTPENDGET(wlc, i+1),
				TXPKTPENDGET(wlc, i+2)));
		}
	}
	WL_ERROR(("frameid 0x%x queue %d\n", txs->frameid, queue));
}

/** process an individual tx_status_t */
bool BCMFASTPATH
wlc_dotxstatus(wlc_info_t *wlc, tx_status_t *txs, uint32 frm_tx2)
{
	void *p;
	uint queue;
	wlc_txh_info_t txh_info;
	struct scb *scb = NULL;
	bool update_rate, free_pdu;
	osl_t *osh;
	int tx_rts, tx_frame_count, tx_rts_count;
	uint totlen, supr_status;
	bool lastframe;
	struct dot11_header *h;
	uint16 fc;
	wlc_pkttag_t *pkttag;
	wlc_bsscfg_t *bsscfg = NULL;
	bool pkt_sent = FALSE;
	bool pkt_max_retries = FALSE;
#if defined(PSPRETEND) || defined(WLPKTDLYSTAT)
	bool ack_recd = FALSE;
#endif
#ifdef WLPKTDLYSTAT
	uint32 delay, now;
	scb_delay_stats_t *delay_stats;
#endif
#ifdef PKTQ_LOG
	uint32 prec_pktlen;
	pktq_counters_t *actq_cnt = 0;
#endif
	uint8 ac;
	bool from_host = TRUE;
#ifdef RTS_PER_ITF
	struct wlc_if *wlcif = NULL;
#endif

#ifdef WL_TX_STALL
	wlc_tx_status_t tx_status = WLC_TX_STS_SUCCESS;
#endif
	BCM_REFERENCE(from_host);
	BCM_REFERENCE(frm_tx2);

	if (WL_PRHDRS_ON()) {
		wlc_print_txstatus(wlc, txs);
	}

	/* discard intermediate indications for ucode with legitimate cases:
	 * 1. corerev < 40: e.g. if "useRTS" is set. ucode did a successful rts/cts exchange,
	 * but the subsequent tx of DATA failed. so it will start rts/cts from the beginning
	 * (resetting the rts transmission count)
	 * 2. corerev >= 40:MUTX intermediate txstatus
	 */
	if (txs->status.is_intermediate) {
		if (D11REV_GE(wlc->pub->corerev, 40)) {
			/* Must be MU intermediate txstatus. Return for now */
#if defined(WL_MU_TX)
#ifdef WLCNT
			wlc_ampdu_aqm_mutx_dotxinterm_status(wlc->ampdu_tx, txs);
#endif
#endif /* defined(WL_MU_TX) */
			return FALSE;
		} else if (wlc_txs_reg_mpdu(wlc, txs)) {
			WLCNTADD(wlc->pub->_cnt->txnoack, (txs->status.frag_tx_cnt));
			WL_TRACE(("%s: bail: txs status no ack\n", __FUNCTION__));
			return FALSE;
		}
	}

	osh = wlc->osh;
	queue = WLC_TXFID_GET_QUEUE(txs->frameid);
	ASSERT(queue < WLC_HW_NFIFO_INUSE(wlc));
	if (queue >= WLC_HW_NFIFO_INUSE(wlc) || WLC_HW_DI(wlc, queue) == NULL) {
		p = NULL;
		WL_PRINT(("%s: bail: txs status FID->q %u invalid\n", __FUNCTION__, queue));
		wlc_print_txstatus(wlc, txs);
		goto fatal;
	}

	supr_status = txs->status.suppr_ind;
	if (supr_status) {
		WL_TX_STS_UPDATE(tx_status,
			wlc_tx_status_map_hw_to_sw_supr_code(wlc, supr_status));
	}

	/* Flushed packets have been re-enqueued, just toss the status */
	if (supr_status == TX_STATUS_SUPR_FLUSH) {
		WL_MQ(("MQ: %s: Flush status for txs->frameid 0x%x\n", __FUNCTION__,
		       txs->frameid));
#ifdef WLC_MACDBG_FRAMEID_TRACE
		wlc_macdbg_frameid_trace_txs(wlc->macdbg, NULL, txs);
#endif
		/* we are supposed to get this suppression reason only
		 * when txfifo_detach_pending is set.
		 */
		ASSERT(wlc->txfifo_detach_pending);
		return FALSE;
	}

	p = GETNEXTTXP(wlc, queue);
	WL_TRACE(("%s: pkt fr queue=%d p=%p\n", __FUNCTION__, queue, OSL_OBFUSCATE_BUF(p)));

#if defined(BCMDBG)
	if (p == NULL || wlc->excursion_active || wlc->txfifo_detach_pending) {
		WL_MQ(("MQ: %s: txs->frameid 0x%x\n", __FUNCTION__, txs->frameid));
	}
#endif /* BCMDBG */

	if (p == NULL) {
		void *p1;
		BCM_REFERENCE(p1);
		WL_PRINT(("%s: null ptr2\n", __FUNCTION__));
		wlc_txs_dump_status_info(wlc, txs, queue);
#if defined(ENABLE_CORECAPTURE)
		wlc_print_txstatus(wlc, txs);
		p1 = dma_getnexttxp(WLC_HW_DI(wlc, queue), HNDDMA_RANGE_ALL);
		if (!p1) {
			uint i, fbmp;
			for (i = 0, fbmp = BITMAP_SYNC_ALL_TX_FIFOS; fbmp; i++, fbmp = fbmp >> 1) {
				if ((!PIO_ENAB(wlc->pub) && WLC_HW_DI(wlc, i) == NULL) ||
					(PIO_ENAB(wlc->pub) && WLC_HW_PIO(wlc, i) == NULL)) {
					continue;
				}
				if (TXPKTPENDGET(wlc, i) > 0) {
					uint8* txh;
					void *p2;
					p2 = dma_getnexttxp(WLC_HW_DI(wlc, i), HNDDMA_RANGE_ALL);
					if (p2) {
						txh = PKTDATA(wlc->osh, p2);
						wlc_print_hdrs(wlc, "DMA txpkt hdr", txh,
						     (wlc_txd_t*)txh, NULL,
						     PKTLEN(wlc->osh, p2));
						prhex("DMA txpkt body", txh, PKTLEN(wlc->osh, p2));
					}
				 }
			}
		}
#endif /*  ENABLE_CORECAPTURE  */
		goto fatal;
	}

	ASSERT(p != NULL);

	PKTDBG_TRACE(osh, p, PKTLIST_MI_TFS_RCVD);

	wlc_get_txh_info(wlc, p, &txh_info);

	if ((D11REV_GE(wlc->pub->corerev, 40)) &&
		(ltoh16(txh_info.MacTxControlHigh) & D11AC_TXC_FIX_RATE) &&
		!(txs->status.s5 & TX_STATUS64_MUTX)) {
		/* if using fix rate, retrying 64 mpdus >=4 times can overflow 8-bit cnt.
		 * So ucode treats fix rate specially unless it is MUTX.
		 * Update this before calling AMPDU dotxstatus function.
		 */
		txs->status.frag_tx_cnt = txs->status.s3 & 0xffff;
	}
#ifdef WLC_MACDBG_FRAMEID_TRACE
	wlc_macdbg_frameid_trace_txs(wlc->macdbg, p, txs);
#endif

	if ((supr_status != TX_STATUS_INT_XFER_ERR) &&
		(txs->frameid != htol16(txh_info.TxFrameID))) {
		WL_PRINT(("wl%d: %s: txs->frameid 0x%x txh->TxFrameID 0x%x\n",
		          wlc->pub->unit, __FUNCTION__,
		          txs->frameid, htol16(txh_info.TxFrameID)));
#ifdef WLC_MACDBG_FRAMEID_TRACE
		wlc_txs_dump_status_info(wlc, txs, queue);
		printf("bad pkt p:%p flags:0x%x flags3:0x%x seq:0x%x\n",
		       OSL_OBFUSCATE_BUF(p), WLPKTTAG(p)->flags,
			WLPKTTAG(p)->flags3, WLPKTTAG(p)->seq);
		prpkt("bad pkt", wlc->osh, p);
		wlc_macdbg_frameid_trace_dump(wlc->macdbg);
#endif
		wlc_print_txstatus(wlc, txs);
		ASSERT(txs->frameid == htol16(txh_info.TxFrameID));
		goto fatal;
	}

	pkttag = WLPKTTAG(p);

	ac = WME_PRIO2AC(PKTPRIO(p));

#ifdef PKTQ_LOG
	if ((WLC_GET_TXQ(wlc->active_queue))->pktqlog) {
		actq_cnt =
		(WLC_GET_TXQ(wlc->active_queue))->pktqlog->_prec_cnt[WLC_PRIO_TO_PREC(PKTPRIO(p))];
	}
#endif


	/* The h is actually pointed to the 802.11 header enbedded in TX frames,
	 * but only after huge TX header. It is safe from TX status overwrite
	 * by wlc_process_wlhdr_txstatus() call below.
	 */

	h = (struct dot11_header *)(txh_info.d11HdrPtr);
	fc = ltoh16(h->fc);

#ifdef MBSS
	if (MBSS_ENAB(wlc->pub) && (queue == TX_ATIM_FIFO)) {
		/* Under MBSS, all ATIM packets are handled by mbss_dotxstatus */
		wlc_mbss_dotxstatus(wlc, txs, p, fc, pkttag, supr_status);
		WL_TRACE(("%s: bail after mbss dotxstatus\n", __FUNCTION__));
		return FALSE;
	}
#endif /* MBSS */

	wlc_pkttag_scb_restore_ex(wlc, p, &txh_info, &bsscfg, &scb);

	if (ETHER_ISMULTI(txh_info.TxFrameRA)) {
#ifdef MBSS
		if (bsscfg != NULL) {
			if (MBSS_ENAB(wlc->pub) && (queue == TX_BCMC_FIFO))
				wlc_mbss_dotxstatus_mcmx(wlc, bsscfg, txs);
		}
#endif /* MBSS */
	} else {
#ifdef PKTQ_LOG
		if (D11REV_GE(wlc->pub->corerev, 40)) {
#if defined(SCB_BS_DATA)
			wlc_bs_data_counters_t *bs_data_counters = NULL;
#endif /* SCB_BS_DATA */
			SCB_BS_DATA_CONDFIND(bs_data_counters, wlc, scb);

			WLCNTCONDADD(actq_cnt, actq_cnt->airtime, TX_STATUS40_TX_MEDIUM_DELAY(txs));
			SCB_BS_DATA_CONDADD(bs_data_counters, airtime,
				TX_STATUS40_TX_MEDIUM_DELAY(txs));
		}
#endif /* PKTQ_LOG */
	}

#ifdef RTS_PER_ITF
	if (scb && scb->bsscfg && scb->bsscfg->wlcif) {
		wlcif = scb->bsscfg->wlcif;
	}
#endif

#ifdef WLCNT
	if (N_ENAB(wlc->pub)) {
		if (wlc_txh_get_isSGI(&txh_info))
			WLCNTINCR(wlc->pub->_cnt->txmpdu_sgi);
		if (wlc_txh_get_isSTBC(&txh_info))
			WLCNTINCR(wlc->pub->_cnt->txmpdu_stbc);
	}
#endif

#if defined(WLNAR)
	wlc_nar_dotxstatus(wlc->nar_handle, scb, p, txs);
#endif


#ifdef WL_BEAMFORMING
	if (TXBF_ENAB(wlc->pub) && (txs->status.s5 & TX_STATUS40_IMPBF_MASK)) {
		if (scb) {
			wlc_txbf_imp_txstatus(wlc->txbf, scb, txs);
		}
	}
#endif
#ifdef ENABLE_CORECAPTURE
	if (pkttag->flags & WLF_8021X) {
		wlc_txs_dump_status_info(wlc, txs, queue);
	}
#endif /* ENABLE_CORECAPTURE */

#ifdef WLAMPDU
	/* Reset txnoprog counter if ucode is giving txstatus */
	wlc_ampdu_reset_txnoprog(wlc->ampdu_tx);

	if (WLPKTFLAG_AMPDU(pkttag)) {
		bool regmpdu = (pkttag->flags3 & WLF3_AMPDU_REGMPDU) != 0;
		bool ret_val;

		if (regmpdu) {
			wlc_ampdu_dotxstatus_regmpdu(wlc->ampdu_tx, scb, p, txs, &txh_info);
			ret_val = BCME_OK;
		} else {
#ifdef STA
			if ((scb != NULL) &&
				wlc_bsscfg_find_by_unique_bssid(wlc, &h->a2)) {
					wlc_adv_pspoll_upd(wlc, scb, p, TRUE, queue);
			}
#endif /* STA */
			ret_val = wlc_ampdu_dotxstatus(wlc->ampdu_tx, scb, p, txs, &txh_info);
		}
#ifdef WL_LPC
		if (LPC_ENAB(wlc) && scb) {
			/* Update the LPC database */
			wlc_scb_lpc_update_pwr(wlc->wlpci, scb, ac,
				txh_info.PhyTxControlWord0, txh_info.PhyTxControlWord1);
			/* Reset the LPC vals in ratesel */
			wlc_scb_ratesel_reset_vals(wlc->wrsi, scb, ac);
		}
#endif /* WL_LPC */
		if (regmpdu) {
			/* continue with rest of regular mpdu processing */
		} else {
#ifdef STA
			if (bsscfg != NULL && BSSCFG_STA(bsscfg)) {
				wlc_pm2_ret_upd_last_wake_time(bsscfg, &txs->lasttxtime);
#ifdef WL_LEAKY_AP_STATS
				if (WL_LEAKYAPSTATS_ENAB(wlc->pub)) {
					wlc_leakyapstats_gt_start_time_upd(wlc, bsscfg, txs);
				}
#endif /* WL_LEAKY_AP_STATS */
			}
#endif /* STA */
			if (ret_val) {
				WL_PRINT(("wl%d: %s: %d frameid 0x%x 0x%x\n",
					wlc->pub->unit, __FUNCTION__, __LINE__,
					txs->frameid, htol16(txh_info.TxFrameID)));
				wlc_print_txstatus(wlc, txs);
			}
			return ret_val;
		}
	}
#endif /* WLAMPDU */

#ifdef PROP_TXSTATUS
	from_host = WL_TXSTATUS_GET_FLAGS(pkttag->wl_hdr_information) & WLFC_PKTFLAG_PKTFROMHOST;
	if (PROP_TXSTATUS_ENAB(wlc->pub)) {
		wlc_wlfc_dotxstatus(wlc, bsscfg, scb, p, txs);
	}
#endif /* PROP_TXSTATUS */

	tx_rts = wlc_txh_has_rts(wlc, &txh_info);
	tx_frame_count = txs->status.frag_tx_cnt;
	tx_rts_count = txs->status.rts_tx_cnt;

	/* count all retransmits */
	if (tx_frame_count) {
		WLCNTADD(wlc->pub->_cnt->txretrans, (tx_frame_count - 1));
		WLCNTSCB_COND_ADD(scb && from_host, scb->scb_stats.tx_pkts_retries,
			(tx_frame_count - 1));
		WLCNTSCB_COND_ADD(scb && !from_host, scb->scb_stats.tx_pkts_fw_retries,
			(tx_frame_count - 1));
	}
	if (tx_rts_count)
		WLCNTADD(wlc->pub->_cnt->txretrans, (tx_rts_count - 1));

#ifdef PKTQ_LOG
	WLCNTCONDADD(actq_cnt && tx_rts_count, actq_cnt->rtsfail, tx_rts_count - 1);
#endif

	lastframe = (fc & FC_MOREFRAG) == 0;

	totlen = pkttotlen(osh, p);
	BCM_REFERENCE(totlen);

	/* plan for success */
	update_rate = (pkttag->flags & WLF_RATE_AUTO) ? TRUE : FALSE;
	free_pdu = TRUE;

#ifdef PSPRETEND
	/* Whether we could be doing ps pretend in the case the
	 * tx status is NACK. PS pretend is not operating on multicast scb.
	 * "Conventional" PS mode (SCB_PS_PRETEND_NORMALPS) is excluded to keep the two
	 * mechanisms separated.
	 * This code handles tx status not processed by AMPDU. Normal PS pretend
	 * is (today) only supported on AMPDU traffic, but THRESHOLD PS pretend can be
	 * used for non AMPDU traffic, so this is checked.
	 * Finally, the flag for WLF_PSDONTQ is checked because this is used for probe
	 * packets. We should not operate ps pretend in this case.
	 */
	if (!txs->status.was_acked &&
	    scb != NULL && !SCB_ISMULTI(scb) && SCB_PS_PRETEND_THRESHOLD(scb) && !SCB_PS(scb) &&
	    !(pkttag->flags & WLF_PSDONTQ)) {
		wlc_pspretend_on(wlc->pps_info, scb, 0);
		supr_status |= TX_STATUS_SUPR_PMQ;
	}
#endif /* PSPRETEND */

#ifdef WL_EVENT_LOG_COMPILE
	if (pkttag->flags & WLF_8021X) {
		wl_event_log_tlv_hdr_t tlv_log = {{0, 0}};
		wl_event_log_eapol_tx_t eapol_sts = {{0, 0, 0}};
		uint32 tlv_log_tmp = 0;
		uint16 tx_rate = 0;

		eapol_sts.status = txs->status.was_acked ? 1 : 0;
		eapol_sts.frag_tx_cnt = txs->status.frag_tx_cnt;

		tlv_log.tag = TRACE_TAG_VENDOR_SPECIFIC;
		tlv_log.length = sizeof(wl_event_log_eapol_tx_t);
		tlv_log_tmp = tlv_log.t;

		tlv_log.tag = TRACE_TAG_RATE_MBPS;
		tlv_log.length = sizeof(uint16);

		if (D11REV_GE(wlc->pub->corerev, 64)) {
			tx_rate = txh_info.hdrPtr->txd.u.rev64.RateInfo[0].TxRate;
		} else {
			tx_rate = txh_info.hdrPtr->txd.u.rev40.RateInfo[0].TxRate;
		}
		WL_EVENT_LOG((EVENT_LOG_TAG_TRACE_WL_INFO, TRACE_FW_EAPOL_FRAME_TRANSMIT_STOP,
			tlv_log_tmp, eapol_sts.t,
			tlv_log.t, tx_rate));
	}
#endif /* WL_EVENT_LOG_COMPILE */

	/* process txstatus info */
	if (txs->status.was_acked) {
		/* success... */
#if defined(PSPRETEND) || defined(WLPKTDLYSTAT)
		ack_recd = TRUE;
#endif

		/* update counters */
		WLCNTINCR(wlc->pub->_cnt->txfrag);
		WLCNTADD(wlc->pub->_cnt->txnoack, (tx_frame_count - 1));
		WLCIFCNTINCR(scb, txfrag);
#ifdef PKTQ_LOG
		WLCNTCONDINCR(actq_cnt, actq_cnt->acked);
		WLCNTCONDADD(actq_cnt, actq_cnt->retry, (tx_frame_count - 1));
		prec_pktlen = pkttotlen(wlc->osh, p) - sizeof(d11txh_t) - DOT11_LLC_SNAP_HDR_LEN -
		              DOT11_MAC_HDR_LEN - DOT11_QOS_LEN;
		WLCNTCONDADD(actq_cnt, actq_cnt->throughput, prec_pktlen);
#endif
		if (tx_frame_count > 1) {
			/* counting number of retries for all data frames. */
			wlc->txretried += tx_frame_count - 1;
			/* counting the number of frames where a retry was necessary
			 * per each station.
			 */
			WLCNTSCB_COND_INCR(scb, scb->scb_stats.tx_pkts_retried);
		}
		if (lastframe) {
			WLCNTINCR(wlc->pub->_cnt->txfrmsnt);
			WLCIFCNTINCR(scb, txfrmsnt);
			if (wlc->txretried >= 1) {
				WLCNTINCR(wlc->pub->_cnt->txretry);
				WLCIFCNTINCR(scb, txretry);
			}
			if (wlc->txretried > 1) {
				WLCNTINCR(wlc->pub->_cnt->txretrie);
				WLCIFCNTINCR(scb, txretrie);
			}

			WLCNTSCB_COND_INCR(scb && from_host, scb->scb_stats.tx_pkts_total);
			WLCNTSCB_COND_INCR(scb && !from_host, scb->scb_stats.tx_pkts_fw_total);
		}
		if (scb) {
			/* If P2P/GO send PROBE_RESP frame to existing P2P/GC,
			 * scb->used should not be updated.
			 * Perhaps, it will be already disconnected situation.
			 */
			if (!(FC_TYPE(fc) == FC_TYPE_MNG &&
				(FC_SUBTYPE(fc) == FC_SUBTYPE_PROBE_RESP))) {
				scb->used = wlc->pub->now;
			}
#if defined(STA) && defined(DBG_BCN_LOSS)
			scb->dbg_bcn.last_tx = wlc->pub->now;
#endif
		}
		if (bsscfg && BSSCFG_STA(bsscfg) && bsscfg->BSS) {
			/* clear the time_since_bcn  */
			if (bsscfg->roam->time_since_bcn != 0) {
#ifdef EVENT_LOG_COMPILE
				EVENT_LOG(EVENT_LOG_TAG_BEACON_LOG, "Beacon miss reset by TX");
#endif
				bsscfg->roam->time_since_bcn = 0;
			}
		}

		pkt_sent = TRUE;

	} else if ((!txs->status.was_acked) &&
	           (WLPKTFLAG_AMSDU(pkttag) || WLPKTFLAG_RIFS(pkttag))) {
		update_rate = FALSE;

		/* update counters */
		WLCNTINCR(wlc->pub->_cnt->txfrag);
		WLCIFCNTINCR(scb, txfrag);
		if (lastframe) {
			WLCNTINCR(wlc->pub->_cnt->txfrmsnt);
			WLCIFCNTINCR(scb, txfrmsnt);

			WLCNTSCB_COND_INCR(scb && from_host, scb->scb_stats.tx_pkts_total);
			WLCNTSCB_COND_INCR(scb && !from_host, scb->scb_stats.tx_pkts_fw_total);
		}

		/* should not update scb->used for ap otherwise the ap probe will not work */
		if (scb && bsscfg && BSSCFG_STA(bsscfg)) {
			scb->used = wlc->pub->now;
#if defined(STA) && defined(DBG_BCN_LOSS)
			scb->dbg_bcn.last_tx = wlc->pub->now;
#endif
		}

		pkt_sent = TRUE;

	} else if (supr_status == TX_STATUS_SUPR_FRAG) {
		/* subsequent frag failure... */
		update_rate = FALSE;
	} else if (supr_status == TX_STATUS_SUPR_BADCH) {
		update_rate = FALSE;
		WLCNTINCR(wlc->pub->_cnt->txchanrej);
	} else if (supr_status == TX_STATUS_SUPR_EXPTIME) {
		WLCNTINCR(wlc->pub->_wme_cnt->tx_expired[(WME_ENAB(wlc->pub) ?
			WME_PRIO2AC(PKTPRIO(p)) : AC_BE)].packets);
		WLCNTINCR(wlc->pub->_cnt->txexptime);	/* generic lifetime expiration */
#ifdef RTS_PER_ITF
		if (wlcif) {
			WLCNTINCR(wlcif->_cnt.txexptime);
		}
#endif
		/* Interference detected */
		if (wlc->rfaware_lifetime)
			wlc_exptime_start(wlc);
	} else if (supr_status == TX_STATUS_SUPR_PMQ) {
		update_rate = FALSE;

		if ((bsscfg != NULL && BSSCFG_AP(bsscfg)) || BSS_TDLS_ENAB(wlc, bsscfg)) {
#if defined(BCMDBG) || defined(WLMSG_PS)
			char eabuf[ETHER_ADDR_STR_LEN];
#endif /* BCMDBG_ERR */


			if (scb != NULL && (SCB_ISMULTI(scb) || SCB_ASSOCIATED(scb) ||
				BSS_TDLS_ENAB(wlc, bsscfg)) &&
#ifdef PROP_TXSTATUS
				(!PROP_TXSTATUS_ENAB(wlc->pub)||!from_host) &&
#endif
				TRUE) {
				free_pdu = wlc_apps_suppr_frame_enq(wlc, p, txs, lastframe);
			}

			WL_PS(("wl%d: %s: suppress for PM frame 0x%x for %s %s\n",
			       wlc->pub->unit, __FUNCTION__, txs->frameid,
			       bcm_ether_ntoa((struct ether_addr *)txh_info.TxFrameRA, eabuf),
			       free_pdu?"tossed":"requeued"));
		}

		/* If the packet is not queued for later delivery, it will be freed
		 * and any packet callback needs to consider this as a un-ACKed packet.
		 * Change the txstatus to simple no-ack instead of suppressed.
		 */
		if (free_pdu)
			wlc_txs_clear_supr(wlc, &(txs->status));
	}
#if defined(PSPRETEND)
	 else if (PSPRETEND_ENAB(wlc->pub) && supr_status == TX_STATUS_SUPR_PPS) {
		update_rate = FALSE;
		free_pdu = TRUE;

		if (SCB_PS_PRETEND(scb) && !SCB_ISMULTI(scb) &&
#ifdef PROP_TXSTATUS
			(PROP_TXSTATUS_ENAB(wlc->pub) && !from_host) &&
#endif
			TRUE) {
			free_pdu = wlc_apps_suppr_frame_enq(wlc, p, txs, lastframe);
		}


		WL_PS(("wl%d.%d: %s: ps pretend %s suppress for PM frame 0x%x for "MACF" %s\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
			SCB_PS_PRETEND(scb) ? "ON":"OFF", txs->frameid,
			ETHERP_TO_MACF(txh_info.TxFrameRA), free_pdu?"tossed":"requeued"));

		/* If the packet is not queued for later delivery, it will be freed
		 * and any packet callback needs to consider this as a un-ACKed packet.
		 * Change the txstatus to simple no-ack instead of suppressed.
		 */
		if (free_pdu) {
			wlc_txs_clear_supr(wlc, &(txs->status));
		}
	 }
#endif /* PSPRETEND */
	else if (supr_status == TX_STATUS_SUPR_UF) {
		update_rate = FALSE;
	} else if (supr_status == TX_STATUS_INT_XFER_ERR) {
		update_rate = FALSE;
	} else if (P2P_ABS_SUPR(wlc, supr_status)) {
		update_rate = FALSE;
		if (bsscfg != NULL && BSS_TX_SUPR_ENAB(bsscfg)) {
#ifdef BCMDBG
			if (WL_P2P_ON()) {
#if defined(WLP2P_UCODE)
				int bss = wlc_mcnx_BSS_idx(wlc->mcnx, bsscfg);
				WL_P2P(("wl%d: suppress packet %p for absence, seqnum %d, "
					"TSF 0x%x, state 0x%x\n",
					wlc->pub->unit, OSL_OBFUSCATE_BUF(p),
					ltoh16(h->seq) >> SEQNUM_SHIFT,
					R_REG(wlc->osh, &wlc->regs->tsf_timerlow),
					wlc_mcnx_read_shm(wlc->mcnx, M_P2P_BSS_ST(wlc, bss))));
#endif /* WLP2P_UCODE */
				wlc_print_txdesc(wlc, (wlc_txd_t*)PKTDATA(osh, p));
				wlc_print_txstatus(wlc, txs);
			}
#endif /* BCMDBG */

			/* This is possible if we got a packet suppression
			 * before getting ABS interrupt
			 */
			if (!BSS_TX_SUPR(bsscfg) &&
#ifdef WLP2P
			    (wlc_p2p_noa_valid(wlc->p2p, bsscfg) ||
			     wlc_p2p_ops_valid(wlc->p2p, bsscfg)) &&
#endif
			    TRUE)
				wlc_bsscfg_tx_stop(wlc->psqi, bsscfg);

			/* requeue packet */
			if (scb != NULL) {
				bool requeue  = TRUE;

				if (BSSCFG_AP(bsscfg) &&
				    SCB_ASSOCIATED(scb) && SCB_P2P(scb))
					wlc_apps_scb_tx_block(wlc, scb, 1, TRUE);
#if defined(PROP_TXSTATUS)
				if (PROP_TXSTATUS_ENAB(wlc->pub)) {
					requeue = wlc_should_retry_suppressed_pkt(wlc,
					    p, supr_status);
					requeue |= SCB_ISMULTI(scb);
				}
#endif
				if (requeue) {
					if (BSS_TX_SUPR(bsscfg))
						free_pdu = wlc_pkt_abs_supr_enq(wlc, scb, p);
					else
						free_pdu = TRUE;
				}
			}
		}

		/* If the packet is not queued for later delivery, it will be freed
		 * and any packet callback needs to consider this as a un-ACKed packet.
		 * Change the txstatus to simple no-ack instead of suppressed.
		 */
		if (free_pdu)
			wlc_txs_clear_supr(wlc, &(txs->status));
	} else if (txs->phyerr) {
		wlc_log_unexpected_tx_frame_log_80211hdr(wlc, h);
		update_rate = FALSE;
		WLCNTINCR(wlc->pub->_cnt->txphyerr);
		WL_TX_STS_UPDATE(tx_status, WLC_TX_STS_PHY_ERROR);
		WL_ERROR(("wl%d: %s: tx phy error (0x%x)\n", wlc->pub->unit,
			__FUNCTION__, txs->phyerr));
#ifdef BCMDBG
		if (WL_ERROR_ON()) {
			prpkt("txpkt (MPDU)", osh, p);
			wlc_print_txdesc(wlc, (wlc_txd_t *)PKTDATA(osh, p));
			wlc_print_txstatus(wlc, txs);
		}
#endif /* BCMDBG */
	} else if (ETHER_ISMULTI(txh_info.TxFrameRA)) {
		/* mcast success */
		/* update counters */
		if (tx_frame_count) {
			WLCNTINCR(wlc->pub->_cnt->txfrag);
			WLCIFCNTINCR(scb, txfrag);
		}

		if (lastframe) {
			WLCNTINCR(wlc->pub->_cnt->txfrmsnt);
			WLCIFCNTINCR(scb, txfrmsnt);
			if (tx_frame_count) {
				WLCNTINCR(wlc->pub->_cnt->txmulti);
				WLCIFCNTINCR(scb, txmulti);
			}
		}
		pkt_sent = TRUE;
	}
#ifdef STA
	else if (bsscfg != NULL && BSSCFG_STA(bsscfg) && bsscfg->BSS && ETHER_ISMULTI(&h->a3)) {
		if (tx_frame_count)
			WLCNTINCR(wlc->pub->_cnt->txfrag);

		if (lastframe) {
			WLCNTINCR(wlc->pub->_cnt->txfrmsnt);
			WLCIFCNTINCR(scb, txfrmsnt);
			if (tx_frame_count) {
				WLCNTINCR(wlc->pub->_cnt->txmulti);
				WLCIFCNTINCR(scb, txmulti);
				WLCNTSCB_COND_INCR(scb, scb->scb_stats.tx_mcast_pkts);
			}
		}

		pkt_sent = TRUE;
	}
#endif /* STA */
	else if ((!tx_rts && tx_frame_count >= wlc->SRL) ||
	         (tx_rts && (tx_rts_count >= wlc->SRL || tx_frame_count >= wlc->LRL))) {
		WLCNTADD(wlc->pub->_cnt->txnoack, tx_frame_count);
		WL_TX_STS_UPDATE(tx_status, WLC_TX_STS_RETRY_TIMEOUT);
#ifdef PKTQ_LOG
		WLCNTCONDADD(actq_cnt, actq_cnt->retry, tx_frame_count);
#endif

		/* ucast failure */
		if (lastframe) {
			WLCNTINCR(wlc->pub->_cnt->txfail);
#ifdef WLC_SW_DIVERSITY
			if (WLSWDIV_ENAB(wlc) && WLSWDIV_BSSCFG_SUPPORT(bsscfg)) {
				wlc_swdiv_txfail(wlc->swdiv, scb);
			}
#endif /* WLC_SW_DIVERSITY */
			WLCNTSCB_COND_INCR(scb && from_host,
				scb->scb_stats.tx_pkts_retry_exhausted);
			WLCNTSCB_COND_INCR(scb && !from_host,
				scb->scb_stats.tx_pkts_fw_retry_exhausted);
			WLCNTSCB_COND_INCR(scb, scb->scb_stats.tx_failures);
			WLCIFCNTINCR(scb, txfail);
#ifdef PKTQ_LOG
			WLCNTCONDINCR(actq_cnt, actq_cnt->retry_drop);
#endif
			if (from_host) {
				wlc_log_unexpected_tx_frame_log_80211hdr(wlc, h);
			}
			wlc_bss_mac_event(wlc, bsscfg, WLC_E_TXFAIL,
			                 (struct ether_addr *)txh_info.TxFrameRA,
			                 WLC_E_STATUS_TIMEOUT, 0, 0, 0, 0);
		}
#ifdef BCMDBG
		if ((!tx_rts && tx_frame_count != wlc->SRL) ||
		    (tx_rts && tx_rts_count != wlc->SRL &&
		               tx_frame_count != wlc->LRL)) {
			/* catch ucode retrying too much */
			WL_ERROR(("wl%d: %s: status indicates too many retries before "
			          "failure\n", wlc->pub->unit, __FUNCTION__));
			WL_ERROR(("tx_rts = %d tx_rts_count = %d tx_frame_count = %d ->"
				" srl=%d lrl=%d\n",
				tx_rts, tx_rts_count, tx_frame_count, wlc->SRL, wlc->LRL));
		}
#endif /* BCMDBG */
		pkt_max_retries = TRUE;
	} else {
		/* unexpected tx status */
		wlc_log_unexpected_tx_frame_log_80211hdr(wlc, h);
		update_rate = FALSE;
		WLCNTINCR(wlc->pub->_cnt->txserr);
		WL_TX_STS_UPDATE(tx_status, WLC_TX_STS_TOSS_UNKNOWN);
		WL_TRACE(("wl%d: %s: unexpected tx status returned \n",
			wlc->pub->unit, __FUNCTION__));
		WL_TRACE(("tx_rts = %d tx_rts_count = %d tx_frame_count = %d -> srl=%d lrl=%d\n",
			tx_rts, tx_rts_count, tx_frame_count, wlc->SRL, wlc->LRL));
	}

#ifdef WL_TX_STALL
	/* Update tx_status statistics */
	if (tx_status) {
		wlc_tx_status_update_counters(wlc, p, scb, bsscfg, tx_status, 1);
		if (free_pdu == FALSE) {
			/* Packet is requeued, update the queued count */
			wlc_tx_status_update_counters(wlc, p, scb, bsscfg,
				WLC_TX_STS_QUEUED, 1);
		}
	}
#endif

	/* Check if interference still there and print external log if needed */
	if (wlc->rfaware_lifetime && wlc->exptime_cnt &&
	    supr_status != TX_STATUS_SUPR_EXPTIME)
		wlc_exptime_check_end(wlc);


	/* update rate state and scb used time */
	if (update_rate && scb) {
		uint16 sfbl, lfbl;
		uint8 mcs = MCS_INVALID;
		uint8 antselid = 0;
		bool is_sgi = wlc_txh_get_isSGI(&txh_info);
#ifdef WL11N
		uint8 frametype = ltoh16(txh_info.PhyTxControlWord0) & PHY_TXC_FT_MASK;
		if (frametype == FT_HT || frametype == FT_VHT) {
			mcs = wlc_txh_info_get_mcs(wlc, &txh_info);
		}
		if (WLANTSEL_ENAB(wlc))
			antselid = wlc_antsel_antsel2id(wlc->asi,
				ltoh16(txh_info.w.ABI_MimoAntSel));
#endif /* WL11N */

		ASSERT(!TX_STATUS_UNEXP(txs->status));

		if (queue != TX_BCMC_FIFO) {
			sfbl = WLC_WME_RETRY_SFB_GET(wlc, ac);
			lfbl = WLC_WME_RETRY_LFB_GET(wlc, ac);
		} else {
			sfbl = wlc->SFBL;
			lfbl = wlc->LFBL;
		}

		wlc_scb_ratesel_upd_txstatus_normalack(wlc->wrsi, scb, txs, sfbl, lfbl,
			mcs, is_sgi, antselid,
			ltoh16(txh_info.MacTxControlHigh) & TXC_AMPDU_FBR ? 1 : 0,
			ac);
#ifdef WL_LPC
		if (LPC_ENAB(wlc) && scb) {
			/* Update the LPC database */
			wlc_scb_lpc_update_pwr(wlc->wlpci, scb, ac,
				txh_info.PhyTxControlWord0, txh_info.PhyTxControlWord1);
			/* Reset the LPC vals in ratesel */
			wlc_scb_ratesel_reset_vals(wlc->wrsi, scb, ac);
		}
#endif /* WL_LPC */
#if defined(WLCNT) && defined(WL_MU_TX)
		if (MU_TX_ENAB(wlc) && SCB_MU(scb)) {
			uint8 gid = wlc_vht_get_gid(txh_info.plcpPtr);
			ratespec_t rspec = wlc_vht_get_rspec_from_plcp(txh_info.plcpPtr);

			wlc_mutx_update_txcounters(wlc->mutx, scb, rspec, FALSE,
				(txs->status.s5 & TX_STATUS64_MUTX) ? TRUE : FALSE,
				1, txs->status.was_acked ? 0 : 1, gid, txs);
		}
#endif /* WLCNT && WL_MU_TX */

	}
	WL_TRACE(("%s: calling txfifo_complete\n", __FUNCTION__));

	WLC_TXFIFO_COMPLETE(wlc, queue, 1, WLPKTTIME(p));

#ifdef PSPRETEND
	/* Provide tx status input to the PS Pretend state machine.
	 * The function may cause a tx fifo flush.
	 * As a result, this call needs to be done after
	 * the call to wlc_txfifo_complete() above.
	 */
	if (bsscfg != NULL && (BSSCFG_AP(bsscfg) || BSSCFG_IBSS(bsscfg)) &&
	    PS_PRETEND_ENABLED(bsscfg) &&
	    /* if scb is multicast or in normal PS mode, return as we aren't handling
	     * ps pretend for these
	     */
	    scb != NULL && !SCB_ISMULTI(scb) && !SCB_PS_PRETEND_NORMALPS(scb) &&
	    !(pkttag->flags & WLF_PSDONTQ)) {
		wlc_pspretend_dotxstatus(wlc->pps_info, bsscfg, scb, ack_recd);
	}
#endif /* PSPRETEND */

#ifdef WLPKTDLYSTAT
	/* calculate latency and packet loss statistics */
	if (scb && free_pdu) {
		/* Get the current time */
		now = WLC_GET_CURR_TIME(wlc);
		/* Ignore wrap-around case */
		if (now > WLPKTTAG(p)->shared.enqtime) {
			uint tr = tx_frame_count - 1;
			if (tx_frame_count == 0 || tx_frame_count > RETRY_SHORT_DEF)
				tr = RETRY_SHORT_DEF-1;

			delay_stats = scb->delay_stats + ac;
			delay = now - WLPKTTAG(p)->shared.enqtime;
			wlc_delay_stats_upd(delay_stats, delay, tr, ack_recd);
		}
	}
#endif /* WLPKTDLYSTAT */

#ifdef STA
	/* PM state change */
	/* - in mcast case we always have a valid bsscfg (unless the bsscfg is deleted).
	 * - in ucast case we may not have a valid bsscfg and scb could have been deleted before
	 *   the transmission finishes, in which case lookup the bsscfg from the BSSID
	 * - Infrastructure STAs should always find the bsscfg by RA BSSID match, unless the
	 *   bsscfg has been deleted (in which case we do not care about PM state update for it)
	 * - in case we located neither scb nor bsscfg we assume the PM states have
	 *   been taken care of when deleting the scb or the bsscfg, yes we hope;-)
	 */
	if (bsscfg != NULL) {
#ifdef WL_LEAKY_AP_STATS
		if (WL_LEAKYAPSTATS_ENAB(wlc->pub)) {
			wlc_leakyapstats_gt_start_time_upd(wlc, bsscfg, txs);
		}
#endif /* WL_LEAKY_AP_STATS */
		wlc_update_pmstate(bsscfg, (uint)(wlc_txs_alias_to_old_fmt(wlc, &(txs->status))));

		if ((((fc & FC_KIND_MASK) == FC_DATA) ||
		     ((fc & FC_KIND_MASK) == FC_QOS_DATA)) && lastframe) {
			wlc_pm2_ret_upd_last_wake_time(bsscfg, &txs->lasttxtime);
		}
	}
#endif /* STA */


	/* update the bytes in/out counts */
	if (PIO_ENAB(wlc->pub))
		wlc_pio_cntupd(WLC_HW_PIO(wlc, queue), totlen);

#ifdef STA
	/* Roaming decision based upon too many packets at the min tx rate */
	if (scb != NULL && !SCB_INTERNAL(scb) &&
	    bsscfg != NULL && BSSCFG_STA(bsscfg) &&
	    bsscfg->BSS && bsscfg->associated &&
#ifdef WLP2P
	    !BSS_P2P_ENAB(wlc, bsscfg) &&
#endif
	    !bsscfg->roam->off)
		wlc_txrate_roam(wlc, scb, txs, pkt_sent, pkt_max_retries, ac);

	if (scb != NULL && lastframe) {
		 if (wlc_bsscfg_find_by_unique_bssid(wlc, &h->a2)) {
			wlc_adv_pspoll_upd(wlc, scb, p, TRUE, queue);
		 }
	}
#endif /* STA */
	/* call any matching pkt callbacks */
	if (free_pdu) {
		wlc_pcb_fn_invoke(wlc->pcb, p,
		                  (uint)(wlc_txs_alias_to_old_fmt(wlc, &(txs->status))));
	}

#ifdef WLP2P
	if (WL_P2P_ON()) {
		if (txs->status.was_acked) {
			WL_P2P(("wl%d: sent packet %p successfully, seqnum %d\n",
			        wlc->pub->unit, OSL_OBFUSCATE_BUF(p),
				ltoh16(h->seq) >> SEQNUM_SHIFT));
		}
	}
#endif
#ifdef WLAMPDU
#endif /* WLAMPDU */

	PKTDBG_TRACE(osh, p, PKTLIST_TXDONE);

	/* free the mpdu */
	if (free_pdu) {
#ifdef WLTXMONITOR
#endif /* WLTXMONITOR */
		PKTFREE(osh, p, TRUE);
	}

	if (lastframe)
		wlc->txretried = 0;

	return FALSE;

fatal:
	if (p != NULL) {
		PKTDBG_TRACE(osh, p, PKTLIST_TXFAIL);
		PKTFREE(osh, p, TRUE);
	}

	BCM_REFERENCE(pkt_sent);
	BCM_REFERENCE(pkt_max_retries);
	return TRUE;
} /* wlc_dotxstatus */

bool wlc_should_retry_suppressed_pkt(wlc_info_t *wlc, void *p, uint status)
{
#ifdef DMATXRC
	if (DMATXRC_ENAB(wlc->pub) && (WLPKTTAG(p)->flags & WLF_PHDR) &&
			TXRC_ISRECLAIMED(TXRC_PKTTAIL(wlc->osh, p))) {
		/* Reclaimed packets cannot be requeued. */
		return FALSE;
	}
#endif /* DMATXRC */

	/* TX_STATUS_SUPR_PPS is purposely not tested here */
	if ((status == TX_STATUS_SUPR_PMQ) || (P2P_ABS_SUPR(wlc, status))) {
		/* Retry/Requeue pkt in any of the following cases:
		 * 1. NIC / PropTx Not enabled
		 * 2. Dongle / PropTx Disabled.
		 * 3. Dongle / PropTx Enabled, but Host PropTx inactive
		 * 4. Dongle / PropTx Enabled and Host PropTx active, but dongle(!host) packet
		 */
		if ((!PROP_TXSTATUS_ENAB(wlc->pub)) ||
#ifdef PROP_TXSTATUS
			(!HOST_PROPTXSTATUS_ACTIVATED(wlc)) ||
			(PROP_TXSTATUS_ENAB(wlc->pub) && HOST_PROPTXSTATUS_ACTIVATED(wlc) &&
			(!(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(p)->wl_hdr_information) &
			WLFC_PKTFLAG_PKTFROMHOST))) ||
#endif /* PROP_TXSTATUS */
			FALSE) {
#ifdef PROP_TXSTATUS_DEBUG
			wlc_txh_info_t txh_info;
			struct dot11_header *h;
			uint16 fc;
			wlc_get_txh_info(wlc, p, &txh_info);
			h = (struct dot11_header *)(txh_info.d11HdrPtr);
			fc = ltoh16(h->fc);
			char eabuf[ETHER_ADDR_STR_LEN];
			bcm_ether_ntoa((struct ether_addr *)(txh_info.TxFrameRA), eabuf);
			printf("retry ptk:0x%x FC%x t/st:%x/%x for [%s]\n",
				(WLPKTTAG(p)->wl_hdr_information), fc,
				FC_TYPE(fc), FC_SUBTYPE(fc), eabuf);
#endif /* PROP_TXSTATUS_DEBUG */
			return TRUE;
		}
	}
	return FALSE;
}

static void BCMFASTPATH
wlc_pkttag_scb_restore_ex(wlc_info_t* wlc, void* p, wlc_txh_info_t* txh_info,
	wlc_bsscfg_t **bsscfg_out, struct scb **scb_out)
{
	wlc_bsscfg_t *bsscfg = NULL;
	struct scb *scb = NULL;
	int err;

	BCM_REFERENCE(err);

	/*	use the bsscfg index in the packet tag to find out which
	 *	bsscfg this packet belongs to
	 */
	bsscfg = wlc_bsscfg_find(wlc, WLPKTTAGBSSCFGGET(p), &err);

	if (ETHER_ISMULTI(txh_info->TxFrameRA)) {
		if (bsscfg != NULL) {
			/* With pkt suppression, need to assign SCB for any frame */
			scb = WLC_BCMCSCB_GET(wlc, bsscfg);

			/* BCMC scb is NULL in case of P2P discovery.
			 * For P2P discovery make use of hw rateset scb.
			 */
			if (scb == NULL)
				scb = wlc->band->hwrs_scb;

			ASSERT(scb != NULL);
		}
	} else {
		/* For ucast frames, lookup the scb directly by the RA.
		 * The scb may not be found if we sent a frame with no scb, or if the
		 * scb was deleted while the frame was queued.
		 */
		if (bsscfg != NULL) {
			scb = wlc_scbfindband(wlc, bsscfg,
				(struct ether_addr*)txh_info->TxFrameRA,
				(wlc_txh_info_is5GHz(wlc, txh_info)
				?
				BAND_5G_INDEX : BAND_2G_INDEX));
		}

		if (scb != NULL) {
			bsscfg = SCB_BSSCFG(scb);
			ASSERT(bsscfg != NULL);
		}
#ifdef STA
		else {
			/* no scb, but we may be able to look up a bsscfg by the BSSID if this
			 * was a frame sent by an infra STA to its AP
			 */
			bsscfg = wlc_bsscfg_find_by_bssid(wlc,
				(struct ether_addr *)txh_info->TxFrameRA);
		}
#endif /* STA */
	}

	if (bsscfg != NULL) {
		WLPKTTAGBSSCFGSET(p, WLC_BSSCFG_IDX(bsscfg));
	}
	/* else when NULL, then no-op for bsscfg */

	/* update to usable value (NULL or valid scb ptr) */
	WLPKTTAGSCBSET(p, scb);

	if (bsscfg_out) {
		*bsscfg_out = bsscfg;
	}
	if (scb_out) {
		*scb_out = scb;
	}
} /* wlc_pkttag_scb_restore_ex */

void
wlc_pkttag_scb_restore(void *ctxt, void* p)
{
	wlc_txh_info_t txh_info;
	wlc_info_t* wlc = (wlc_info_t*)ctxt;

	wlc_get_txh_info(wlc, p, &txh_info);

	wlc_pkttag_scb_restore_ex(wlc, p, &txh_info, NULL, NULL);
}
