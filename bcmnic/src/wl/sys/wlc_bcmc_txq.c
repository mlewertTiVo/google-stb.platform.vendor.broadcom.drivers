/*
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
 * $Id: wlc_bcmc_txq.c 664818 2016-10-13 22:06:23Z $
 */

/*
 * @file
 * @brief Broadcast MUX layer functions for Broadcom 802.11 Networking Driver
 */

#include <wlc_cfg.h>

#ifdef TXQ_MUX

#include <wlc.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <wlc_types.h>
#include <wlc_scb.h>
#include <wlc_bsscfg.h>
#include <wlc_scbq.h>
#include <wlc_mux.h>
#include <wlc_tx.h>
#include <wlc_prot.h>
#include <wlc_ampdu.h>
#include <wlc_mux_utils.h>
#include <wlc_wlfc.h>
#include <wlc_txmod.h>

#include <wlc_bcmc_txq.h>

/*
 * @brief Definition of state structure for the BCMC module created by wlc_bcmc_txq_module_attach().
 *
 */

#define WLC_BSS_DATA_FC_ON(cfg) ((cfg)->wlc->block_datafifo != 0)
#define BCMCQ_REQUIRED(cfg)  ((BSSCFG_AP(cfg) || BSSCFG_IBSS(cfg)) == FALSE)

struct wlc_bcmcq_info {
	osl_t *osh;      /**< @brief OSL handle */
	wlc_info_t *wlc; /**< @brief wlc_info_t handle */
	wlc_pub_t *pub;  /**< @brief wlc_pub_t handle */
	uint unit;       /**< @brief wl driver unit number for debug messages */
	int cfgh;        /**< @brief bsscfg cubby handle */
	int scbh;        /**< @brief scb cubby handle */
};

/** TXQ_MUX broadcast/multicast mux source and queues for the bss */
struct bcmc_cubby {
	wlc_bsscfg_t *bsscfg;         /**< bsscfg of mux source */
	mux_srcs_t *msrc;             /**< MUX source pointer */
	struct pktq queue;            /**< BCMC packet queue */
	struct pktq suppress_queue;   /**< BCMC suppression reorder queue */
};

typedef bcmc_cubby_t* bcmc_cubby_p;     /**< Pointer to BCMC cubby */

static void wlc_bcmc_enqueue(void *ctx, struct scb *scb, void *pkt, uint prec);
static uint wlc_bcmcq_txpktcnt(void *ctx);

static txmod_fns_t BCMATTACHDATA(bcmc_txmod_fns) = {
	wlc_bcmc_enqueue,
	wlc_bcmcq_txpktcnt,
	NULL,
	NULL
};

/*
 * @brief This is the output function of the BCMC MUX.
 *
 * This retrieves packets from the specified BCMC mux sources.
 * If the BCMC mux source is activated it dequeues all the data from the 4AC data queues
 * up to the requested time limit.
 *
 * @param ctx             Context pointer to BSS configuration
 * @param ac              Access class to dequeue packets from
 * @param request_time    Requested time duration of the dequeued packets
 * @output_q              Pointer to queued of retireved packets.
 *                        Valid only if the function returns TRUE.
 *
 * @return                supplied_time if function completed successfully,
 *                        and output_q contains a valid chain of packets
 *                        0 if there is an error.
 */
static uint
wlc_bcmc_mux_output(void *ctx, uint ac, uint request_time, struct spktq *output_q)
{
	bcmc_cubby_t *cubby = (bcmc_cubby_t *)ctx;
	wlc_bsscfg_t *cfg = cubby->bsscfg;
	wlc_info_t *wlc = cfg->wlc;
	struct pktq *q;
	osl_t *osh;
	ratespec_t rspec;
	wlcband_t *band;
	uint supplied_time = 0;
	DBGONLY(uint supplied_count = 0; )
	timecalc_t timecalc;
	uint current_pktlen = 0;
	uint current_pkttime = 0;
	wlc_pkttag_t *pkttag;
	void *pkt[DOT11_MAXNUMFRAGS] = {0};
	uint16 prec_map = 0;
	int prec;
	int count;
	int i;
#ifdef WLAMPDU_MAC
	bool check_epoch = TRUE;
#endif /* WLAMPDU_MAC */

	BCM_REFERENCE(pkttag);

#ifdef BCMC_MUX_DEBUG
	WL_ERROR(("%s() req_time:%u  ac:%d muxq:0x%p\n",
		__FUNCTION__, request_time, ac, OSL_OBFUSCATE_BUF(cubby->msrc)));
#endif

	ASSERT(ac < AC_COUNT || (ac == MUX_SRC_BCMC));

	if (ac == MUX_SRC_BCMC) {
		/*
		 * Setup prec map to dequeue all bcmc frames into the BCMC fifo
		 * Packets already dequeued into the low txq will be suppressed by ucode
		 */
		for (i = 0; i < AC_COUNT; i++) {
			prec_map |= wlc->fifo2prec_map[i];
		}
	} else {
		prec_map = wlc->fifo2prec_map[ac];
	}

	band = wlc->band;
	rspec = WLC_LOWEST_BAND_RSPEC(band);
	q = &cubby->queue;
	osh = wlc->osh;

	timecalc.rspec = rspec;
	timecalc.fixed_overhead_us = wlc_tx_mpdu_frame_seq_overhead(rspec, cfg, band, ac);
	timecalc.is2g = BAND_2G(band->bandtype);

	/* Determine the preamble type */
	if (RSPEC_ISLEGACY(rspec)) {
		/* For legacy reates calc the short/long preamble.
		 * Only applicable for 2, 5.5, and 11.
		 * Check the bss config and other overrides.
		 */

		uint mac_rate = (rspec & RATE_MASK);

		if ((mac_rate == WLC_RATE_2M ||
		     mac_rate == WLC_RATE_5M5 ||
		     mac_rate == WLC_RATE_11M) &&
		    WLC_PROT_CFG_SHORTPREAMBLE(wlc->prot, cfg) &&
		    (cfg->PLCPHdr_override != WLC_PLCP_LONG)) {
			timecalc.short_preamble = 1;
		} else {
			timecalc.short_preamble = 0;
		}
	} else {
		/* For VHT, always MM, for HT, assume MM and don't bother with Greenfield */
		timecalc.short_preamble = 0;
	}

	while (supplied_time < request_time && (pkt[0] = pktq_mdeq(q, prec_map, &prec))) {
		wlc_txh_info_t txh_info;
		uint pktlen;
		uint pkttime;
		int err;
		uint fifo; /* is this param needed anymore ? */
		struct scb *scb = WLPKTTAGSCBGET(pkt[0]);

		pkttag = WLPKTTAG(pkt[0]);

#ifdef BCMC_MUX_DEBUG
		{
		void * tmp_pkt = pkt[0];

		while (tmp_pkt) {
			prhex(__FUNCTION__, PKTDATA(wlc->osh, tmp_pkt), PKTLEN(wlc->osh, tmp_pkt));
			tmp_pkt = PKTNEXT(wlc->osh, tmp_pkt);
			}
		}
#endif

		/* separate packet preparation and time calculation for
		 * MPDU (802.11 formatted packet with txparams), and
		 * MSDU (Ethernet or 802.3 stack packet)
		 */

		ASSERT((pkttag->flags & WLF_MPDU) == 0);
		/*
		 * MSDU packet prep
		 */

		err = wlc_prep_sdu(wlc, scb, pkt, &count, &fifo);
		if (err) {
			WL_ERROR(("%s(%d) wlc_prep_sdu=%d, fifo=%d count=%d scb:0x%p %s\n",
				__FUNCTION__, __LINE__, err, fifo, count, OSL_OBFUSCATE_BUF(scb),
				SCB_INTERNAL(scb) ? " Internal": ""));
		}
		if (err == BCME_OK) {
			if (count == 1) {
				/* optimization: skip the txtime calculation if the total
				 * pkt len is the same as the last time through the loop
				 */
				pktlen = pkttotlen(osh, pkt[0]);
				if (current_pktlen == pktlen) {
					pkttime = current_pkttime;
				} else {
					wlc_get_txh_info(wlc, pkt[0], &txh_info);

					/* calculate and store the estimated pkt tx time */
					pkttime = wlc_scbq_timecalc(&timecalc,
						txh_info.d11FrameSize);
					current_pktlen = pktlen;
					current_pkttime = pkttime;
				}

				WLPKTTIME(pkt[0]) = (uint16)pkttime;
			} else {
				uint fragtime = 0;
				pkttime = 0;
				for (i = 0; i < count; i++) {
					wlc_get_txh_info(wlc, pkt[i], &txh_info);

					/* calculate and store the estimated pkt tx time */
					fragtime = wlc_scbq_timecalc(&timecalc,
						txh_info.d11FrameSize);
					WLPKTTIME(pkt[i]) = (uint16)fragtime;
					pkttime += fragtime;
				}
			}
		} else {
			if (err == BCME_ERROR) {
				/* BCME_ERROR indicates a tossed packet */

				/* pkt[] should be invalid and count zero */
				ASSERT(count == 0);

				/* let the code finish the loop adding no time
				 * for this dequeued packet, and enqueue nothing to
				 * output_q since count == 0
				 */
				pkttime = 0;
			} else {
				/* should be no other errors */
				ASSERT((err == BCME_OK));
				PKTFREE(osh, pkt[0], TRUE);
				pkttime = 0;
				count = 0;
			}
		}
		supplied_time += pkttime;
		DBGONLY(supplied_count++; )

		for (i = 0; i < count; i++) {
#ifdef WLAMPDU_MAC
			/* For AQM AMPDU Aggregation:
			 * If there is a transition from A-MPDU aggregation frames to a
			 * non-aggregation frame, the epoch needs to change. Otherwise the
			 * non-agg frames may get included in an A-MPDU.
			 */
			if (check_epoch && AMPDU_AQM_ENAB(wlc->pub)) {
				/* Once we check the condition,
				 * we don't need to check again since
				 * we are enqueuing an non_ampdu frame
				 * so wlc_ampdu_was_ampdu() will be false.
				 */
				check_epoch = FALSE;
				/* if the previous frame in the fifo was an ampdu mpdu,
				 * change the epoch
				 */
				if (wlc_ampdu_was_ampdu(wlc->ampdu_tx, ac)) {
					bool epoch;

					wlc_get_txh_info(wlc, pkt[i], &txh_info);
					epoch = wlc_ampdu_chgnsav_epoch(wlc->ampdu_tx,
					    ac,
					    AMU_EPOCH_CHG_MPDU,
					    scb,
					    (uint8)PKTPRIO(pkt[i]),
					    &txh_info);
					wlc_txh_set_epoch(wlc, txh_info.tsoHdrPtr, epoch);
				}
			}
#endif /* WLAMPDU_MAC */

			/* add this pkt to the output queue */
#ifdef BCMC_MUX_DEBUG
			WL_ERROR(("%s() pkttime=%d  pkt[%d]=0x%p\n",
				__FUNCTION__, supplied_time, i, OSL_OBFUSCATE_BUF(pkt[i])));
#endif
			ASSERT(pkt[i]);
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

	/* Return with output packets on 'output_q', and tx time estimate as return value */

	WL_NONE(("%s: exit ac %u supplied %dus, %u pkts\n", __FUNCTION__,
		ac, supplied_time, supplied_count));

	return supplied_time;
}

/*
 * Recreate BCMC mux source.
 *
 * @param wlc       wlc info pointer
 * @param cfg       BSS config pointer
 * @param new_qi    pointer to the new queue
 */
int
wlc_bcmc_recreatesrc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct wlc_txq_info *new_qi)
{
	bcmc_cubby_t *cubby = cfg->bcmc_cubby;
	mux_srcs_t *msrcs;

	ASSERT(cubby);
	msrcs = cubby->msrc;

	if (msrcs) {
		ASSERT(new_qi);
		return (wlc_msrc_recreatesrc(wlc, new_qi->ac_mux,
			msrcs, cubby, wlc_bcmc_mux_output));
	} else {
		/* No BCMC mux configured for the bsscfg */
		return BCME_OK;
	}
}

/*
 * @brief Delete BCMC mux source.
 *
 *
 * @param wlc       wlc info pointer
 * @param cfg       BSS config pointer
 */
void
wlc_bcmc_delsrc(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bcmc_cubby_t *cubby = cfg->bcmc_cubby;

	ASSERT(cubby);

	if (cubby->msrc) {
		wlc_msrc_delsrc(wlc, cubby->msrc);
	}
}

/*
 * @brief Move BCMC mux to a new queue.
 *
 * This deallocates the old source and creates a source on the new queue
 *
 * @param wlc       wlc info pointer
 * @param cfg       BSS config pointer
 * @param new_qi    pointer to the new queue
 */

int
wlc_bcmc_mux_move(wlc_info_t *wlc,
	wlc_bsscfg_t *cfg, struct wlc_txq_info *new_qi)
{
	bcmc_cubby_t *cubby = cfg->bcmc_cubby;

	ASSERT(cubby);

	if (cubby->msrc == NULL) {
		/* No BCMC mux configured for the bsscfg */
		return (BCME_OK);
	}

	wlc_bcmc_delsrc(wlc, cfg);
	return (wlc_bcmc_recreatesrc(wlc, cfg, new_qi));
}


/*
 * @brief Move BCMC and SCB queue muxes to a new queue.
 *
 * This deallocates the old source and creates a source on the new queue
 *
 * @param wlc		 wlc info pointer
 * @param cfg		 BSS config pointer
 * @param new_qi	 pointer to the new queue
 */
int
wlc_bsscfg_mux_move(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct wlc_txq_info *new_qi)
{
	int err = BCME_OK;
	struct scb *scb = NULL;
	struct scb_iter scbiter;

#ifdef AP
	if (BSSCFG_AP(cfg)) {
		/* Move BCMC mux */
		err = wlc_bcmc_mux_move(wlc, cfg, new_qi);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d:%s wlc_bcmc_mux_move() failed err = %d\n",
				wlc->pub->unit, __FUNCTION__, err));
			return (err);
		}
	 }

#endif /* AP */
	 /* move SCB muxes */
	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
		err = wlc_scbq_mux_move(wlc->tx_scbq, scb, new_qi);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d:%s wlc_scbq_mux_move() failed err = %d\n",
				wlc->pub->unit, __FUNCTION__, err));
			return (err);
		}
	}
	return (err);
}

/*
 * @brief Recreate BCMC and SCB queue muxes on new queue.
 *
 * This deallocates the old source and creates a source on the new queue
 *
 * @param wlc		 wlc info pointer
 * @param cfg		 BSS config pointer
 * @param new_qi	 pointer to the new queue
 */
int
wlc_bsscfg_recreate_muxsrc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct wlc_txq_info *new_qi)
{
	int err = BCME_OK;
	struct scb *scb = NULL;
	struct scb_iter scbiter;

	ASSERT(new_qi);

#ifdef AP
	if (BSSCFG_AP(cfg)) {
		err = wlc_bcmc_recreatesrc(wlc, cfg, new_qi);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d:%s wlc_bcmc_recreatesrc() failed err = %d\n",
				wlc->pub->unit, __FUNCTION__, err));
			return (err);
		}
	}
#endif /* AP */

	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
		err = wlc_scbq_muxrecreatesrc(wlc->tx_scbq, scb, new_qi);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d:%s wlc_scbq_muxrecreatesrc() failed err = %d\n",
				wlc->pub->unit, __FUNCTION__, err));
			return (err);
		}
	}
	return (err);
}

/*
 * @brief Delete BCMC and SCB queue muxes.
 *
 * This deallocates the old source and creates a source on the new queue
 *
 * @param wlc		 wlc info pointer
 * @param cfg		 BSS config pointer
 * @param new_qi	 pointer to the new queue
 */
void
wlc_bsscfg_delete_muxsrc(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	struct scb *scb = NULL;
	struct scb_iter scbiter;

#ifdef AP
	if (BSSCFG_AP(cfg)) {
		wlc_bcmc_delsrc(wlc, cfg);
	}
#endif /* AP */

	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
		wlc_scbq_muxdelsrc(wlc->tx_scbq, scb);
	}
}

/*
 * @brief Initialize BCMC MUX queue.
 *
 * This initializes the 4AC data mux sources plus the BCMC mux source
 *
 * @param pub    Pointer to wlc_pub_t
 * @param queue  Pointer TXQ to be initialized
 *
 * @return       TRUE if  completed successfully,
 *               FALSE otherwise
 */
static bool
wlc_bcmc_mux_queue_init(wlc_pub_t *pub, struct pktq *queue)
{
	/* Set the overall queue packet limit to the max, just rely on per-prec limits */
	if (pktq_init(queue, WLC_PREC_COUNT, PKTQ_LEN_MAX)) {
		uint i;

		/* Have enough room for control packets along with HI watermark */
		/* Also, add room to txq for total psq packets if all the SCBs leave PS mode */
		/* The watermark for flowcontrol to OS packets will remain the same */
		for (i = 0; i < WLC_PREC_COUNT; i++) {
			pktq_set_max_plen(queue, i, (2 * pub->tunables->datahiwat) +
			PKTQ_LEN_DEFAULT + pub->psq_pkts_total);
		}
		return TRUE;

	} else {
		return FALSE;
	};
}

/*
 * @brief De-initialize BCMC MUX queue.
 *
 * This de-initializes the 4AC data mux sources plus the BCMC mux source
 *
 * @param queue  Pointer TXQ to be de-initialized
 *
 */
static void
wlc_bcmc_mux_queue_deinit(struct pktq *queue)
{
	pktq_deinit(queue);
}

/*
 * @brief Set powersave state of BCMC queues. Exported function.
 *
 * This routine informs the BCMC packet processing logic when any of the nodes
 * in the BSS enter powersave.
 * The 802.11 spec mandates that all BCMC packets to be sent at DTIM.
 * The 4 AC data fifo mux sources are disabled and the BCMC fifo mux is enabled.
 *
 * @param muxq         Pointer to BSS configuration
 * @param ps_enable    TRUE if any of the nodes have entered powersave.
 *
 * @return             No return value
 */
void
wlc_bcmc_set_powersave(wlc_bsscfg_t *cfg, bool ps_enable)
{
	bcmc_cubby_t *cubby = cfg->bcmc_cubby;
	mux_srcs_t *msrcs;

	ASSERT(cubby);
	msrcs = cubby->msrc;
	ASSERT(msrcs);

	if (WLC_BSS_DATA_FC_ON(cfg)) {
		/* Flow control event in progress, exit */
		WL_INFORM(("%s() wl%d: Flowcontrol block_datafifo: 0x%x\n",
			__FUNCTION__, cfg->wlc->pub->unit, cfg->wlc->block_datafifo));
		return;
	}

	if (ps_enable) {
	/*
	 * PS ON
	 * turn ON bcmc mux source that drains into BCM FIFO
	 * turn OFF bcmc mux sources feeding into he data FIFOS
	*/
		/* Stop DATA mux sources */
		wlc_msrc_group_stop(msrcs, MUX_GRP_DATA);
		/* Start BCMC mux source */
		wlc_msrc_start(msrcs, MUX_SRC_BCMC);
	} else {
	/*
	 * PS OFF
	 * turn OFF bcmc mux source that drains into BCM FIFO
	 * turn ON bcmc mux sources feeding into the data FIFOS
	*/
		/* Stop BCMC mux source */
		wlc_msrc_stop(msrcs, MUX_SRC_BCMC);
		/* Start DATA mux sources */
		wlc_msrc_group_start(msrcs, MUX_GRP_DATA);
	}
}

/*
 * @brief Function to wake a BCMC mux source once data is queued.
 *
 * If the BSS has nodes in powersave, the BCMC queue is woken up,
 * otherwise the approporate fifo for that AC is woken up
 *
 * @param cfg     BSS config structure
 * @param muxq    Pointer to mux queue associated with that BSS
 * @param ac      FIFO access class
 *
 * @return        No return value
 */
static void
wlc_bcmc_wake_mux_source(wlc_bsscfg_t *cfg, mux_srcs_t *msrcs, int ac)
{
	wlc_msrc_wake(msrcs, WLC_BCMC_PSMODE(cfg->wlc, cfg) ? MUX_SRC_BCMC : ac);
}

/*
 * @brief Function to stop all BCMC mux sources in the given BSS.
 *
 * @param cfg	  BSS config structure
 *
 * @return        No return value
 */
void
wlc_bcmc_stop_mux_sources(wlc_bsscfg_t *cfg)
{
	bcmc_cubby_t *cubby = cfg->bcmc_cubby;

	ASSERT(cubby);
	wlc_msrc_group_stop(cubby->msrc, MUX_GRP_DATA|MUX_GRP_BCMC);
}

/*
 * @brief Function to globally stop all BCMC mux sources on every BSS.
 *
 * @param wlc     wlc structure
 *
 * @return        No return value
 */
void
wlc_bcmc_global_stop_mux_sources(wlc_info_t *wlc)
{
	int idx;
	wlc_bsscfg_t *cfg;

	FOREACH_BSS(wlc, idx, cfg) {
		bcmc_cubby_t *cubby = cfg->bcmc_cubby;
		if (cubby && cubby->msrc) {
			wlc_bcmc_stop_mux_sources(cfg);
		}
	}
}

/*
 * @brief Function to start all BCMC mux sources in the given BSS.
 *
 * If the BSS has nodes in powersave, the BCMC queue is woken up,
 * otherwise the approporate fifo for that AC is woken up
 *
 * @param cfg     BSS config structure
 *
 * @return        No return value
 */
void
wlc_bcmc_start_mux_sources(wlc_bsscfg_t *cfg)
{
	/*
	 * Stop all mux sources and then start appropriate source
	 * depending on PS state.
	 */
	wlc_bcmc_stop_mux_sources(cfg);
	wlc_bcmc_set_powersave(cfg, WLC_BCMC_PSMODE(cfg->wlc, cfg));
}

/*
 * @brief Function to globally start all BCMC mux sources on every BSS.
 *
 * @param wlc     wlc structure
 *
 * @return        No return value
 */
void
wlc_bcmc_global_start_mux_sources(wlc_info_t *wlc)
{
	int idx;
	wlc_bsscfg_t *cfg;

	FOREACH_BSS(wlc, idx, cfg) {
		bcmc_cubby_t *cubby = cfg->bcmc_cubby;
		if (cubby && cubby->msrc) {
			wlc_bcmc_start_mux_sources(cfg);
		}
	}
}

/*
 * @brief Enqueue a Broadcast/Multicast packet. Exported function.
 *
 * Enqueue a Broadcast/Multicast packet for the given bsscfg context.
 *
 * @param ctx     Opaque pointer context to bcmcq_info
 * @param pkt     Packet to be queued
 * @param prec    Precedence of packet to be queued
 *
 * @return        TRUE if successful or FALSE if not
 */
static void BCMFASTPATH
wlc_bcmc_enqueue(void *ctx, struct scb *scb, void *pkt, uint prec)
{
	wlc_bcmcq_info_t *bcmcq_info = (wlc_bcmcq_info_t *)ctx;
	wlc_info_t *wlc;
	wlc_bsscfg_t *cfg;
	bcmc_cubby_t *cubby;
	mux_srcs_t *msrcs;

	wlc = bcmcq_info->wlc;
	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg);

	cubby = cfg->bcmc_cubby;

	if (cubby == NULL) {
		ASSERT(SCB_ISMULTI(scb) == 0);
		SCB_TX_NEXT(TXMOD_BCMC, scb, pkt, prec);
		return;
	}

	msrcs = cubby->msrc;
	ASSERT(msrcs);

	if (wlc_prec_enq_head(wlc, &cubby->queue, pkt, prec, FALSE)) {
		/* convert prec to ac fifo number, 4 precs per ac fifo */
		int ac_idx = prec / (WLC_PREC_COUNT / AC_COUNT);

#ifdef BCMC_MUX_DEBUG
		WL_ERROR(("%s() scb:0x%p pkt:0x%p len:%d prec:0x%x\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(WLPKTTAGSCBGET(pkt)),
			OSL_OBFUSCATE_BUF(pkt), PKTLEN(wlc->osh, pkt), prec));
		prhex(__FUNCTION__, PKTDATA(wlc->osh, pkt), PKTLEN(wlc->osh, pkt));
#endif

		wlc_bcmc_wake_mux_source(cfg, msrcs, ac_idx);

		/* kick the low txq */
		wlc_send_q(wlc, wlc->active_queue);
	}

	/* TX success/fail counters updated in wlc_prec_enq_head() */
}

/*
 * @brief Enable BCMC TX Module
 *
 * @param ctx    Opaque cubby context pointer
 * @param scb    scb pointer
 */
static int
wlc_bcmc_scb_init(void *ctx, struct scb *scb)
{
	wlc_bcmcq_info_t *bcmcq_info = (wlc_bcmcq_info_t *)ctx;
	if (SCB_ISMULTI(scb)) {
		WL_ERROR(("%s: ------>TXMOD config BCMC SCB:%p\n",
			__FUNCTION__, OSL_OBFUSCATE_BUF(scb)));
		wlc_txmod_config(bcmcq_info->wlc->txmodi, scb, TXMOD_BCMC);
	}

	return BCME_OK;
}

/*
 * Enqueue a suppressed Broadcast/Multicast packet
 *
 * Enqueue a suppressed Broadcast/Multicast packet on the suppression queue
 * for the given bsscfg context.
 *
 * @param wlc     Pointer to wlc structure
 * @param cfg     Pointer to BSS configuration
 * @param pkt     Packet to be queued
 * @param prec    Precedence of packet to be queued
 *
 * @return        TRUE if successful or FALSE if not
 */
bool
wlc_bcmc_suppress_enqueue(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, uint prec)
{
	bcmc_cubby_t *cubby;
	struct pktq *suppress_queue;

	cubby = cfg->bcmc_cubby;

	ASSERT(cubby);

	suppress_queue = &cubby->suppress_queue;

	/* Enqueue to the BCMC PS reorder queue */
	if (!wlc_prec_enq(wlc, suppress_queue, pkt, prec)) {
		/* This should not happen because the psq should not have a limit */
		ASSERT(0);
		return FALSE;
	}

	return TRUE;
}

/*
 * @brief Allocate BCMC queues and MUX sources.
 *
 * This allocates the 4AC data mux sources plus the BCMC mux source
 *
 * @param cfg    BSS config pointer
 *
 * @return       BCME_OK if function completed successfully,
 *               BCME_NOMEM if allocation fails
 *               BCME_ERROR if queue init fails
 */
static int
wlc_bcmc_mux_init(void *ctx, wlc_bsscfg_t *cfg)
{
	int cfgh;
	wlc_bcmcq_info_t *bcmcq_info;
	mux_srcs_t *bcmc_msrc;
	wlc_info_t *wlc = cfg->wlc;
	wlc_pub_t *pub = wlc->pub;
	bcmc_cubby_t *bcmc_cubby;
	bcmc_cubby_p *cubby_ptr;
	struct pktq *bcmc_queue;
	struct pktq *suppress_queue;
	int rc = BCME_OK;

	ASSERT(ctx);

	bcmcq_info = (wlc_bcmcq_info_t *)ctx;
	cfgh = bcmcq_info->cfgh;
	cubby_ptr = (bcmc_cubby_p *)BSSCFG_CUBBY(cfg, cfgh);

#ifdef BCMDBG
	{
		char *type = NULL;
		if (BSSCFG_STA(cfg)) {
			type = "STA";
		} else if (BSSCFG_AP(cfg)) {
			type = "AP";
		} else if (BSSCFG_IBSS(cfg)) {
			type = "IBSS";
		}
		WL_ERROR(("---->%s cfg:%p type=%s\n", __FUNCTION__, cfg, type));
	}
#endif

	/* Cubby pointers to NULL, this is the majority case in the STA */
	*cubby_ptr = NULL;
	cfg->bcmc_cubby = NULL;

	/* Skip cubby creation if bsscfg type does not require a BCMC cubby */
	if (!BCMCQ_REQUIRED(cfg)) {
		return rc;
	}

	bcmc_cubby = MALLOCZ(wlc->osh, sizeof(*bcmc_cubby));
	if (bcmc_cubby == NULL) {
		return BCME_NOMEM;
	};

	bcmc_cubby->bsscfg = cfg;
	bcmc_queue = &bcmc_cubby->queue;
	suppress_queue = &bcmc_cubby->suppress_queue;

	/* queue for BCMC traffic for this bsscfg */
	if (wlc_bcmc_mux_queue_init(pub, bcmc_queue) == FALSE) {
		rc = BCME_ERROR;
		goto wlc_bcmc_mux_init_fail;
	}

	/* queue for suppression reorder of BCMC traffic */
	if (pktq_init(suppress_queue, WLC_PREC_COUNT, PKTQ_LEN_MAX) == FALSE) {
		wlc_bcmc_mux_queue_deinit(bcmc_queue);
		rc = BCME_ERROR;
		goto wlc_bcmc_mux_init_fail;

	}

	bcmc_msrc = wlc_msrc_alloc(wlc, cfg->wlcif->qi->ac_mux,
	             wlc_txq_nfifo(cfg->wlcif->qi->low_txq),
	             bcmc_cubby, wlc_bcmc_mux_output,
	             MUX_GRP_DATA|MUX_GRP_BCMC);

	if (bcmc_msrc == NULL) {
		wlc_bcmc_mux_queue_deinit(bcmc_queue);
		pktq_deinit(suppress_queue);
		rc = BCME_NOMEM;
		goto wlc_bcmc_mux_init_fail;
	}

	bcmc_cubby->msrc = bcmc_msrc;

	cfg->bcmc_cubby = bcmc_cubby;
	*cubby_ptr = bcmc_cubby;

	/*
	 * Set BCMC PS MUX state to OFF,
	 * this  turns ON the BCMC muxes connected to the data fifos and
	 * turns OFF the mux connected to the BCMC FIFO.
	 */
	WLC_BCMC_PSOFF(cfg);

	return rc;

wlc_bcmc_mux_init_fail:
	MFREE(wlc->osh, bcmc_cubby, sizeof(*bcmc_cubby));
	return rc;
}

/**
 * @brief Deallocate BCMC MUX queues and sources.
 *
 * This deallocates the 4AC data mux sources and the BCMC mux source
 *
 * @param cfg    BSS config pointer
 */
static void
wlc_bcmc_mux_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = cfg->wlc;
	osl_t *osh = wlc->osh;
	bcmc_cubby_t *cubby = cfg->bcmc_cubby;
	wlc_bcmcq_info_t *bcmcq_info;

#ifdef BCMDBG
	{
		char *type = NULL;
		if (BSSCFG_STA(cfg)) {
			type = "STA";
		} else if (BSSCFG_AP(cfg)) {
			type = "AP";
		} else if (BSSCFG_IBSS(cfg)) {
			type = "IBSS";
		}
		WL_ERROR(("----->%s cfg:%p type=%s\n", __FUNCTION__, cfg, type));
	}
#endif

	/* If cubby is null return if not a BSS or IBSS */
	if (cubby == NULL) {
		if (BCMCQ_REQUIRED(cfg)) {
			ASSERT(0);
		}
		return;
	}

	if (cubby->msrc) {
		struct pktq *q = &cubby->queue;
		void *pkt;

		ASSERT(q);

		/* Flush BCMC queue */
		while ((pkt = pktq_deq(q, NULL))) {
#ifdef PROP_TXSTATUS
			if (PROP_TXSTATUS_ENAB(wlc->pub)) {
				wlc_process_wlhdr_txstatus(wlc,
					WLFC_CTL_PKTFLAG_DISCARD, pkt, FALSE);
			}
#endif /* PROP_TXSTATUS */
			PKTFREE(osh, pkt, TRUE);
		}

		wlc_msrc_free(wlc, osh, cubby->msrc);
		wlc_bcmc_mux_queue_deinit(q);
		cubby->msrc = NULL;
	}

	/* Free BCMC cubby and NULL out cubby pointers */
	MFREE(osh, cubby, sizeof(*cubby));
	cfg->bcmc_cubby = NULL;
	bcmcq_info = (wlc_bcmcq_info_t *)ctx;
	*((bcmc_cubby_p *)BSSCFG_CUBBY(cfg, bcmcq_info->cfgh)) = NULL;
}

/* BCMC TXMOD functions */
/**
 * @brief BCMC's Tx Module packet count function
 *
 * BCMC's Tx Module packet count function. This funciton is registerd through the
 * @ref scbq_txmod_fns table via a call to wlc_txmod_fn_register() by
 * wlc_scbq_module_attach(). This function is responsible for returning the count
 * of all tx packets held by the BCMC's Tx Module.
 */
static uint
wlc_bcmcq_txpktcnt(void *ctx)
{
	wlc_bcmcq_info_t *bcmcq_info = (wlc_bcmcq_info_t *)ctx;
	int pktcnt = 0;
	int idx;
	wlc_bsscfg_t *cfg;

	ASSERT(bcmcq_info);

	FOREACH_BSS(bcmcq_info->wlc, idx, cfg) {
		bcmc_cubby_t *cubby = cfg->bcmc_cubby;
		if (cubby && cubby->msrc) {
			pktcnt += pktq_n_pkts_tot(&cubby->queue);
		}
	}
	return pktcnt;
}

/* Module Attach/Detach */

#if defined(BCMDBG) || defined(WLDUMP)
/**
 * Dump current state and statistics of the SCBQ as a whole
 */
static void
wlc_bcmcq_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b)
{
	wlc_bcmcq_info_t *bcmcq_info = (wlc_bcmcq_info_t *)ctx;
	bcmc_cubby_t *cubby;

	if (bcmcq_info == NULL) {
			return;
	}

	cubby = cfg->bcmc_cubby;
	if (cubby && cubby->msrc) {
		bcm_bprintf(b, "BCMC:Enabled npkts:%d\n",
			pktq_n_pkts_tot(&cubby->queue));
		bcm_bprintf(b, "nsrcs:%d enab:0x%X\n",
			wlc_msrc_get_numsrcs(cubby->msrc),
			wlc_msrc_get_enabsrcs(cubby->msrc));
	} else {
		bcm_bprintf(b, "BCMC:Disabled\n");
	}

}
#else
#define wlc_bcmcq_dump NULL
#endif /* BCMDBG || WLDUMP */
/*
 * Create the BCMC txq module infrastructure for the wl driver. wlc_module_register() is called
 * to register the module's handlers.
 * The module registers a cubby and handlers, a Tx Module, and a dump function
 * for BCMC state is also registered.
 */
wlc_bcmcq_info_t*
BCMATTACHFN(wlc_bcmcq_module_attach)(wlc_info_t *wlc)
{
	wlc_bcmcq_info_t *bcmcq_info;
	osl_t *osh = wlc->osh;
	uint unit = wlc->pub->unit;

	/* allocate the main state structure */
	bcmcq_info = MALLOCZ(osh, sizeof(wlc_bcmcq_info_t));
	if (bcmcq_info == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			unit, __FUNCTION__, MALLOCED(osh)));
		goto fail;
	}

	/* Initialize handle/info caches */
	bcmcq_info->wlc = wlc;
	bcmcq_info->pub = wlc->pub;
	bcmcq_info->osh = osh;
	bcmcq_info->unit = unit;

	if (wlc_module_register(wlc->pub,
	                NULL,
	                "bcmcq", /* name */
	                bcmcq_info, /* handle */
	                NULL, /* wlc_iov_disp_fn */
	                NULL, /* watchdog_fn */
	                NULL, /* up_fn */
	                NULL /* down_fn */) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* Reserve bsscfg cubby */
	bcmcq_info->cfgh = wlc_bsscfg_cubby_reserve(
			wlc, /* wlc */
			sizeof(bcmc_cubby_p), /* size */
			wlc_bcmc_mux_init, /* init_fn */
			wlc_bcmc_mux_deinit, /* deinit_fn */
			wlc_bcmcq_dump, /* dump_fn */
			bcmcq_info); /* context */

	if (bcmcq_info->cfgh < 0) {
		WL_ERROR(("wl%d:%s wlc_bsscfg_cubby_reserve() failed\n",
			unit, __FUNCTION__));
		goto fail;
	}

	/* Reserve scb cubby */
	bcmcq_info->scbh = wlc_scb_cubby_reserve(
			wlc, /* wlc */
			0, /* Zero size, just want init callback */
			wlc_bcmc_scb_init, /* init_fn */
			NULL, /* deinit_fn */
			NULL, /* dump_fn */
			bcmcq_info); /* context */

	if (bcmcq_info->scbh < 0) {
		WL_ERROR(("wl%d:%s wlc_scb_cubby_reserve() failed\n",
			unit, __FUNCTION__));
		goto fail;
	}

	/* Register txmod function */
	wlc_txmod_fn_register(wlc->txmodi, TXMOD_BCMC, bcmcq_info, bcmc_txmod_fns);

	return bcmcq_info;

fail:
	/* use detach as a common failure deallocation */
	MODULE_DETACH(bcmcq_info, wlc_bcmcq_module_detach);

	return NULL;
}

/**
 * Free all resources assoicated with the BCMCQ module infrastructure.
 */
void
BCMATTACHFN(wlc_bcmcq_module_detach)(wlc_bcmcq_info_t *bcmcq_info)
{
	if (bcmcq_info == NULL) {
		return;
	}

	wlc_module_unregister(bcmcq_info->pub, "bcmcq", bcmcq_info);

	MFREE(bcmcq_info->osh, bcmcq_info, sizeof(wlc_bcmcq_info_t));
}


#endif /* TXQ_MUX */
