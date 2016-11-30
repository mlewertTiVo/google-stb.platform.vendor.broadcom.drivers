/*
 * Common (OS-independent) portion of
 * Broadcom 802.11abgn Networking Device Driver
 *
 * @file
 * @brief
 * Interrupt/dpc handlers of common driver. Shared by BMAC driver, High driver,
 * and Full driver.
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
 * $Id: wlc_intr.c 652660 2016-08-02 22:39:26Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <d11_cfg.h>
#include <siutils.h>
#include <wlioctl.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_channel.h>
#include <wlc_pio.h>
#include <wlc_rm.h>
#include <wlc.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_tx.h>
#include <wlc_mbss.h>
#include <wlc_macdbg.h>
#include <wl_export.h>
#include <wlc_ap.h>
#include <wlc_extlog.h>
#include <wlc_11h.h>
#include <wlc_quiet.h>
#include <wlc_hrt.h>
#ifdef WLWNM_AP
#include <wlc_wnm.h>
#endif /* WLWNM_AP */
#ifdef WLDURATION
#include <wlc_duration.h>
#endif
#ifdef	WLAIBSS
#include <wlc_aibss.h>
#endif /* WLAIBSS */
#ifdef	WLC_TSYNC
#include <wlc_tsync.h>
#endif /* WLC_TSYNC */

#ifdef WL_BCNTRIM
#include <wlc_bcntrim.h>
#endif
#include <wlc_hw.h>
#include <wlc_phy_hal.h>
#include <wlc_perf_utils.h>
#include <wlc_ltecx.h>

#ifdef WLCXO_CTRL
#include <wlc_cxo_ctrl.h>
#ifndef WLCXO_IPC
#include <wlc_cxo_intr.h>
#endif
#endif /* WLCXO_CTRL */
#include <phy_noise_api.h>

#ifndef WLC_PHYTXERR_THRESH
#define WLC_PHYTXERR_THRESH 20
#endif
uint wlc_phytxerr_thresh = WLC_PHYTXERR_THRESH;


static uint8 wlc_process_per_fifo_intr(wlc_info_t *wlc, bool bounded, wlc_dpc_info_t *dpc);
#if !defined(DMA_TX_FREE)
static uint32 wlc_get_fifo_interrupt(wlc_info_t *wlc, uint8 FIFO);
#endif

/* second-level interrupt processing
 *   Return TRUE if another dpc needs to be re-scheduled. FALSE otherwise.
 *   Param 'bounded' indicates if applicable loops should be bounded.
 *   Param 'dpc' returns info such as how many packets have been received/processed.
 */
bool BCMFASTPATH
wlc_dpc(wlc_info_t *wlc, bool bounded, wlc_dpc_info_t *dpc)
{
	uint32 macintstatus;
	wlc_hw_info_t *wlc_hw = wlc->hw;
	d11regs_t *regs = wlc->regs;
	bool fatal = FALSE;

	if (wlc_hw->macintstatus & MI_BUS_ERROR) {
		WL_ERROR(("%s: MI_BUS_ERROR\n",
				__FUNCTION__));

		/* uCode reported bus error. Clear pending errors */
		si_clear_backplane_to(wlc->pub->sih);

		/* Initiate recovery */
		if (wlc_bmac_report_fatal_errors(wlc->hw, WL_REINIT_RC_AXI_BUS_ERROR)) {
			return FALSE;
		}
	}

	if (DEVICEREMOVED(wlc)) {
		WL_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		wl_down(wlc->wl);
		return FALSE;
	}

#ifdef WLCXO_CTRL
	if (WLCXO_ENAB(wlc->pub)) {
		uint32 hwmacintstatus;
		ASSERT(dpc != NULL);
		hwmacintstatus = dpc->cxo_macintstatus;
#if defined(MBSS)
		ASSERT((hwmacintstatus & (MI_TFS | MI_DMAINT | MI_PMQ)) == 0);
		/* BMAC_NOTE: This mechanism to bail out of sending beacons in
		* wlc_ap.c:wlc_ap_sendbeacons() does not seem like a good idea across a bus with
		* non-negligible reg-read time. The time values in the code have
		* wlc_ap_sendbeacons() bail out if the delay between the isr time and it is >
		* 4ms. But the check is done in wlc_ap_sendbeacons() with a reg read that might
		* take on order of 0.3ms.  Should mbss only be supported with beacons in
		* templates instead of beacons from host driver?
		*/
		if (MBSS_ENAB(wlc->pub) && (hwmacintstatus & (MI_DTIM_TBTT | MI_TBTT))) {
			/* Record latest TBTT/DTIM interrupt time for latency calc */
			wlc_mbss_record_time(wlc);
		}
#endif /* MBSS */
		wlc_hw->macintstatus = hwmacintstatus;
	}
#endif /* WLCXO_CTRL */

	/* grab and clear the saved software intstatus bits */
	macintstatus = wlc_hw->macintstatus;
	if (macintstatus & MI_HWACI_NOTIFY) {
		wlc_phy_hwaci_mitigate_intr(wlc_hw->band->pi);
	}
	wlc_hw->macintstatus = 0;

	WLDURATION_ENTER(wlc, DUR_DPC);


#ifdef STA
	if (macintstatus & MI_BT_PRED_REQ)
		wlc_bmac_btc_update_predictor(wlc_hw);
#endif /* STA */

	/* TBTT indication */
	/* ucode only gives either TBTT or DTIM_TBTT, not both */
	if (macintstatus & (MI_TBTT | MI_DTIM_TBTT)) {
#ifdef RADIO_PWRSAVE
		/* Enter power save mode */
		wlc_radio_pwrsave_enter_mode(wlc, ((macintstatus & MI_DTIM_TBTT) != 0));
#endif /* RADIO_PWRSAVE */
#ifdef MBSS
		if (MBSS_ENAB(wlc->pub)) {
			(void)wlc_mbss16_tbtt(wlc, macintstatus);
		}
#endif /* MBSS */


		wlc_tbtt(wlc, regs);
	}

#ifdef WLWNM_AP
	if (macintstatus & MI_TTTT) {
		if (AP_ENAB(wlc->pub) && WLWNM_ENAB(wlc->pub)) {
			/* Process TIMBC sendout frame indication */
			wlc_wnm_tttt(wlc);
		}
	}
#endif /* WLWNM_AP */


#ifdef WL_BCNTRIM
	if (WLC_BCNTRIM_ENAB(wlc->pub)) {
		if (macintstatus & MI_BCNTRIM_RX) {
			wlc_bcntrim_recv_process_partial_beacon(wlc->bcntrim);
			macintstatus &= ~MI_BCNTRIM_RX;
		}
	}
#endif

	/* BCN template is available */
	/* ZZZ: Use AP_ACTIVE ? */
	if (macintstatus & MI_BCNTPL) {
		if (AP_ENAB(wlc->pub) && (!APSTA_ENAB(wlc->pub) || wlc->aps_associated)) {
			WL_APSTA_BCN(("wl%d: wlc_dpc: template -> wlc_update_beacon()\n",
			              wlc->pub->unit));
			wlc_update_beacon(wlc);
		}
	}

	/* PMQ entry addition */
	if (macintstatus & MI_PMQ) {
#ifdef AP
		if (wlc_bmac_processpmq(wlc_hw, bounded))
			wlc_hw->macintstatus |= MI_PMQ;
#endif
	}

	/* tx status */
	if (macintstatus & MI_TFS) {
		WLDURATION_ENTER(wlc, DUR_DPC_TXSTATUS);
		if (wlc_bmac_txstatus(wlc_hw, bounded, &fatal)) {
			wlc_hw->macintstatus |= MI_TFS;
		}
		WLDURATION_EXIT(wlc, DUR_DPC_TXSTATUS);
		if (fatal) {
			WL_ERROR(("wl%d: %s HAMMERING fatal txs err\n",
				wlc_hw->unit, __FUNCTION__));
			if (wlc_hw->need_reinit == WL_REINIT_RC_NONE) {
				wlc_hw->need_reinit = WL_REINIT_RC_INV_TX_STATUS;
			}
			WLC_FATAL_ERROR(wlc);
			goto exit;
		}
	}

	/* ATIM window end */
	if (macintstatus & MI_ATIMWINEND) {
#ifdef	WLAIBSS
		if (AIBSS_ENAB(wlc->pub) && AIBSS_PS_ENAB(wlc->pub)) {
			wlc_aibss_atim_window_end(wlc);
		}
#endif /* WLAIBSS */

		W_REG(wlc->osh, &regs->maccommand, wlc->qvalid);
		wlc->qvalid = 0;
	}

	/* phy tx error */
	if (macintstatus & MI_PHYTXERR) {
		WL_PRINT(("wl%d: PHYTX error\n", wlc_hw->unit));
		WLCNTINCR(wlc->pub->_cnt->txphyerr);
		wlc_hw->phyerr_cnt++;
#if defined(PHYTXERR_DUMP)
		wlc_dump_phytxerr(wlc, 0);
#endif 
	} else
		wlc_hw->phyerr_cnt = 0;

#ifdef DMATXRC
	/* Opportunistically reclaim tx buffers */
	if (macintstatus & MI_DMATX) {
		/* Reclaim more pkts */
		if (DMATXRC_ENAB(wlc->pub))
			wlc_dmatx_reclaim(wlc_hw);
	}
#endif

#ifdef WLRXOV
	if (macintstatus & MI_RXOV) {
		if (WLRXOV_ENAB(wlc->pub))
			wlc_rxov_int(wlc);
	}
#endif

	/* received data or control frame, MI_DMAINT is indication of RX_FIFO interrupt */
	if (macintstatus & MI_DMAINT) {
		if ((BUSTYPE(wlc_hw->sih->bustype) == PCI_BUS) &&
		    wlc_hw->dma_lpbk) {
			wlc_recover_pkts(wlc_hw->wlc, TX_DATA_FIFO);
			wlc_bmac_dma_rxfill(wlc_hw, RX_FIFO);
		} else
		{
			WLDURATION_ENTER(wlc, DUR_DPC_RXFIFO);
			if (BCMSPLITRX_ENAB()) {
				if (wlc_process_per_fifo_intr(wlc, bounded, dpc)) {
					wlc_hw->macintstatus |= MI_DMAINT;
				}
			} else {
				if (wlc_bmac_recv(wlc_hw, RX_FIFO, bounded, dpc)) {
					wlc_hw->macintstatus |= MI_DMAINT;
				}
			}
			WLDURATION_EXIT(wlc, DUR_DPC_RXFIFO);
		}
	}

	/* TX FIFO suspend/flush completion */
	if (macintstatus & MI_TXSTOP) {
		if (wlc_bmac_tx_fifo_suspended(wlc_hw, TX_DATA_FIFO)) {
			wlc_txstop_intr(wlc);
		}
	}

	/* noise sample collected */
	if (macintstatus & MI_BG_NOISE) {
		WL_NONE(("wl%d: got background noise samples\n", wlc_hw->unit));
		wlc_phy_noise_sample_intr(wlc_hw->band->pi);
	}

#if defined(STA) && defined(WLRM)
	if (WLRM_ENAB(wlc->pub) && macintstatus & MI_CCA) { /* CCA measurement complete */
		WL_INFORM(("wl%d: CCA measurement interrupt\n", wlc_hw->unit));
		wlc_bmac_rm_cca_int(wlc_hw);
	}
#endif

	if (macintstatus & MI_GP0) {
		WL_PRINT(("wl%d: PSM microcode watchdog fired at %d (seconds). Resetting.\n",
			wlc->pub->unit, wlc->pub->now));
		wlc_dump_ucode_fatal(wlc, PSM_FATAL_PSMWD);

		/* wlc_iovar_dump(wlc, "ucode_fatal", strlen("ucode_fatal"), NULL, 2000); */

		WLC_EXTLOG(wlc, LOG_MODULE_COMMON, FMTSTR_REG_PRINT_ID, WL_LOG_LEVEL_ERR,
			0, R_REG(wlc->osh, &regs->psmdebug), "psmdebug");
		WLC_EXTLOG(wlc, LOG_MODULE_COMMON, FMTSTR_REG_PRINT_ID, WL_LOG_LEVEL_ERR,
			0, R_REG(wlc->osh, &regs->phydebug), "phydebug");
		WLC_EXTLOG(wlc, LOG_MODULE_COMMON, FMTSTR_REG_PRINT_ID, WL_LOG_LEVEL_ERR,
			0, R_REG(wlc->osh, &regs->psm_brc), "psm_brc");

		ASSERT(!"PSM Watchdog");

		WLCNTINCR(wlc->pub->_cnt->psmwds);

#ifndef BCMNODOWN
		if (g_assert_type == 3) {
			/* bring the driver down without resetting hardware */
			wlc->down_override = TRUE;
			wlc->psm_watchdog_debug = TRUE;
			wlc_out(wlc);

			/* halt the PSM */
			wlc_bmac_mctrl(wlc_hw, MCTL_PSM_RUN, 0);
			wlc->psm_watchdog_debug = FALSE;
			goto out;
		}
		else
#endif /* BCMNODOWN */
		{
			WL_ERROR(("wl%d: %s HAMMERING: MI_GP0 set\n",
				wlc->pub->unit, __FUNCTION__));

			/* big hammer */
			if (wlc_hw->need_reinit == WL_REINIT_RC_NONE) {
				wlc_hw->need_reinit = WL_REINIT_RC_PSM_WD;
			}
			WLC_FATAL_ERROR(wlc);
		}
	}
#ifdef WAR4360_UCODE
	if (wlc_hw->need_reinit) {
		/* big hammer */
		WL_ERROR(("wl%d: %s: need to reinit() %d, Big Hammer\n",
			wlc->pub->unit, __FUNCTION__, wlc_hw->need_reinit));
		WLC_FATAL_ERROR(wlc);
	}
#endif /* WAR4360_UCODE */
	/* gptimer timeout */
	if (macintstatus & MI_TO) {
		wlc_hrt_isr(wlc->hrti);
	}

#ifdef STA
	if (macintstatus & MI_RFDISABLE) {
		wlc_rfd_intr(wlc);
	}
#endif /* STA */

	if (macintstatus & MI_P2P)
		wlc_p2p_bmac_int_proc(wlc_hw);

#if defined(WLC_TXPWRCAP)
	if (macintstatus & MI_BT_PRED_REQ) {
		int cellstatus;
		macintstatus &= ~MI_BT_PRED_REQ;
		cellstatus = (wlc_bmac_read_shm(wlc_hw,
			wlc_hw->btc->bt_shm_addr + M_LTECX_STATE_OFFSET(wlc)) &
			(3 << C_LTECX_ST_LTE_ACTIVE)) >> C_LTECX_ST_LTE_ACTIVE;
		wlc_channel_txcap_cellstatus_cb(wlc_hw->wlc->cmi, cellstatus);
#if defined(BCMLTECOEX) && defined(OCL)
		if (OCL_ENAB(wlc->pub) && BCMLTECOEX_ENAB(wlc->pub) &&
				(wlc->ltecx->ocl_iovar_set != 0))
			wlc_lte_ocl_update(wlc->ltecx, cellstatus);
#endif /* BCMLTECOEX & OCL */
#ifdef BCMLTECOEX
	if (BCMLTECOEX_ENAB(wlc->pub)) {
		wlc_ltecx_ant_update(wlc->ltecx);
	}
#endif /* BCMLTECOEX */
	}
#endif /* WLC_TXPWRCAP */

	/* send any enq'd tx packets. Just makes sure to jump start tx */
	if (WLC_TXQ_OCCUPIED(wlc)) {
		wlc_send_q(wlc, wlc->active_queue);
	}

	ASSERT(wlc_ps_check(wlc));



#if !defined(BCMNODOWN)
out:
	;
#endif /* !BCMNODOWN */

	/* make sure the bound indication and the implementation are in sync */
	ASSERT(bounded == TRUE || wlc_hw->macintstatus == 0);

exit:
	if (wlc_hw->macintstatus != 0) {
		wlc_radio_upd(wlc);
	}
	WLDURATION_EXIT(wlc, DUR_DPC);

#ifdef WLCXO_CTRL
	if (WLCXO_ENAB(wlc->pub))
		dpc->cxo_macintstatus = wlc_hw->macintstatus;
#endif

	/* For dongle, power req off before wfi */
	if ((BUSTYPE(wlc->pub->sih->bustype) == PCI_BUS)) {
		if (SRPWR_ENAB()) {
			if (wlc_hw->macintstatus == 0) {
				wlc_srpwr_request_off(wlc);
			}
		}
	}

	/* it isn't done and needs to be resched if macintstatus is non-zero */
	return (wlc_hw->macintstatus != 0);
}

/*
 * Read and clear macintmask and macintstatus and intstatus registers.
 * This routine should be called with interrupts off
 * Return:
 *   -1 if DEVICEREMOVED(wlc) evaluates to TRUE;
 *   0 if the interrupt is not for us, or we are in some special cases;
 *   device interrupt status bits otherwise.
 */
static INLINE uint32
wlc_intstatus(wlc_info_t *wlc, bool in_isr)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	d11regs_t *regs = wlc_hw->regs;
	uint32 macintstatus, mask;
	osl_t *osh;

	osh = wlc_hw->osh;

	/* macintstatus includes a DMA interrupt summary bit */
	macintstatus = GET_MACINTSTATUS(osh, regs);

	WL_TRACE(("wl%d: macintstatus: 0x%x\n", wlc_hw->unit, macintstatus));

	/* detect cardbus removed, in power down(suspend) and in reset */
	if (DEVICEREMOVED(wlc))
		return -1;

	/* DEVICEREMOVED succeeds even when the core is still resetting,
	 * handle that case here.
	 */
	if (macintstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		return 0;
	}

	mask = (in_isr ? wlc_hw->macintmask : wlc_hw->defmacintmask);

	mask |= wlc_hw->delayedintmask;

	/* defer unsolicited interrupts */
	macintstatus &= mask;

	/* if not for us */
	if (macintstatus == 0)
		return 0;

#if defined(WLC_TSYNC)
	if (TSYNC_ENAB(wlc->pub)) {
		if (D11REV_GE(wlc->pub->corerev, 40) && (macintstatus & MI_DMATX)) {
			wlc_tsync_process_ucode(wlc->tsync);
		}
	}
#endif /* WLC_TSYNC */

	/* interrupts are already turned off for CFE build
	 * Caution: For CFE Turning off the interrupts again has some undesired
	 * consequences
	 */
	/* turn off the interrupts */
	SET_MACINTMASK(osh, regs, 0);
	(void)GET_MACINTMASK(osh, regs);	/* sync readback */
	wlc_hw->macintmask = 0;

	/* clear device interrupts */
	SET_MACINTSTATUS(osh, regs, macintstatus);

	/* MI_DMAINT is indication of non-zero intstatus */
	if (macintstatus & MI_DMAINT) {
#if !defined(DMA_TX_FREE)
		/*
			* For corerevs >= 5, only fifo interrupt enabled is I_RI in RX_FIFO.
			* If MI_DMAINT is set, assume it is set and clear the interrupt.
			*/
		if (BCMSPLITRX_ENAB()) {
			if (wlc_get_fifo_interrupt(wlc, RX_FIFO)) {
				/* check for fif0-0 intr */
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_0;
				W_REG(osh, &regs->intctrlregs[RX_FIFO].intstatus,
					DEF_RXINTMASK);
			}
			if (PKT_CLASSIFY_EN(RX_FIFO1) || RXFIFO_SPLIT()) {
				if (wlc_get_fifo_interrupt(wlc, RX_FIFO1)) {
				/* check for fif0-1 intr */
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_1;
					W_REG(osh, &regs->intctrlregs[RX_FIFO1].intstatus,
						DEF_RXINTMASK);
				}
			}
			if (PKT_CLASSIFY_EN(RX_FIFO2)) {
				if (wlc_get_fifo_interrupt(wlc, RX_FIFO2)) {
					/* check for fif0-2 intr */
					wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_2;
					W_REG(osh, &regs->intctrlregs[RX_FIFO2].intstatus,
						DEF_RXINTMASK);
				}
			}
		} else {
			W_REG(osh, &regs->intctrlregs[RX_FIFO].intstatus,
				DEF_RXINTMASK);
		}


#ifdef DMATXRC
		if (D11REV_GE(wlc_hw->corerev, 40) && DMATXRC_ENAB(wlc->pub)) {
			if (R_REG(osh, &regs->intctrlregs[TX_DATA_FIFO].intstatus) & I_XI) {
				/* Clear it */
				W_REG(osh, &regs->intctrlregs[TX_DATA_FIFO].intstatus,
					I_XI);

				/* Trigger processing */
				macintstatus |= MI_DMATX;
			}
		}
#endif

#else /* DMA_TX_FREE */
		int i;

		/* Clear rx and tx interrupts */
		/* When cut through DMA enabled, we use the other indirect indaqm DMA
		 * channel registers instead of legacy intctrlregs on TX direction.
		 * The descriptor in aqm_di contains the corresponding txdata DMA
		 * descriptors information which must be filled in tx.
		 */
		if (BCM_DMA_CT_ENAB(wlc_hw->wlc)) {
			for (i = 0; i < wlc_hw->nfifo_inuse; i++) {
				if (i == RX_FIFO) {
					W_REG(osh, &regs->intctrlregs[i].intstatus,
						DEF_RXINTMASK);
					dma_set_indqsel(wlc_hw->aqm_di[i], FALSE);
					W_REG(osh, &regs->indaqm.indintstatus, I_XI);
				}
				else if (wlc_hw->aqm_di[i] &&
					dma_txactive(wlc_hw->aqm_di[i])) {
					dma_set_indqsel(wlc_hw->aqm_di[idx], FALSE);
					W_REG(osh, &regs->indaqm.indintstatus, I_XI);
				}
			}
		}
		else {
			for (i = 0; i < NFIFO; i++) {
				if (i == RX_FIFO) {
					W_REG(osh, &regs->intctrlregs[i].intstatus,
						DEF_RXINTMASK | I_XI);
				}
				else if (wlc_hw->di[i] && dma_txactive(wlc_hw->di[i])) {
					W_REG(osh, &regs->intctrlregs[i].intstatus, I_XI);
				}
			}
		}
#endif /* DMA_TX_FREE */
	}

#ifdef DMATXRC
	/* Opportunistically reclaim tx buffers */
	if (macintstatus & MI_DMATX) {
		/* Reclaim pkts immediately */
		if (DMATXRC_ENAB(wlc->pub))
			wlc_dmatx_reclaim(wlc_hw);
	}
#endif

	return macintstatus;
}

/* Update wlc_hw->macintstatus and wlc_hw->intstatus[]. */
/* Return TRUE if they are updated successfully. FALSE otherwise */
bool
wlc_intrsupd(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint32 macintstatus;

	ASSERT(wlc_hw->macintstatus != 0);

	if (SRPWR_ENAB()) {
		wlc_srpwr_request_on(wlc);
	}

	/* read and clear macintstatus and intstatus registers */
	macintstatus = wlc_intstatus(wlc, FALSE);

	/* device is removed */
	if (macintstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		return FALSE;
	}

#if defined(MBSS)
	if (MBSS_ENAB(wlc->pub) &&
	    (macintstatus & (MI_DTIM_TBTT | MI_TBTT))) {
		/* Record latest TBTT/DTIM interrupt time for latency calc */
		wlc_mbss_record_time(wlc);
	}
#endif /* MBSS */

	/* update interrupt status in software */
	wlc_hw->macintstatus |= macintstatus;

	return TRUE;
}

/*
 * First-level interrupt processing.
 * Return TRUE if this was our interrupt, FALSE otherwise.
 * *wantdpc will be set to TRUE if further wlc_dpc() processing is required,
 * FALSE otherwise.
 */
bool BCMFASTPATH
wlc_isr(wlc_info_t *wlc, bool *wantdpc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint32 macintstatus;

	*wantdpc = FALSE;

	if (wl_powercycle_inprogress(wlc->wl)) {
		return FALSE;
	}

	if (!wlc_hw->up || !wlc_hw->macintmask)
		return (FALSE);

	if (SRPWR_ENAB()) {
		wlc_srpwr_request_on(wlc);
	}

	/* read and clear macintstatus and intstatus registers */
	macintstatus = wlc_intstatus(wlc, TRUE);
	if (macintstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		WL_ERROR(("DEVICEREMOVED detected in the ISR code path.\n"));
		/* in rare cases, we may reach this condition as a race condition may occur */
		/* between disabling interrupts and clearing the SW macintmask */
		/* clear mac int status as there is no valid interrupt for us */
		wlc_hw->macintstatus = 0;
		/* assume this is our interrupt as before; note: want_dpc is FALSE */
		return (TRUE);
	}

	/* it is not for us */
	if (macintstatus == 0)
		return (FALSE);

	*wantdpc = TRUE;

#if defined(MBSS)
	/* BMAC_NOTE: This mechanism to bail out of sending beacons in
	 * wlc_ap.c:wlc_ap_sendbeacons() does not seem like a good idea across a bus with
	 * non-negligible reg-read time. The time values in the code have
	 * wlc_ap_sendbeacons() bail out if the delay between the isr time and it is >
	 * 4ms. But the check is done in wlc_ap_sendbeacons() with a reg read that might
	 * take on order of 0.3ms.  Should mbss only be supported with beacons in
	 * templates instead of beacons from host driver?
	 */
	if (MBSS_ENAB(wlc->pub) &&
	    (macintstatus & (MI_DTIM_TBTT | MI_TBTT))) {
		/* Record latest TBTT/DTIM interrupt time for latency calc */
		wlc_mbss_record_time(wlc);
	}
#endif /* MBSS */

	/* save interrupt status bits */
	ASSERT(wlc_hw->macintstatus == 0);
	wlc_hw->macintstatus = macintstatus;

	return (TRUE);

}

void
wlc_intrson(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;

#ifdef WLCXO
	if (WLCXO_ENAB(wlc->pub)) {
		__wlc_cxo_c2d_intrson(wlc->ipc, wlc);
		return;
	}
#endif

	if (SRPWR_ENAB()) {
		wlc_srpwr_request_on(wlc);
	}

	ASSERT(wlc_hw->defmacintmask);
	wlc_hw->macintmask = wlc_hw->defmacintmask;
	SET_MACINTMASK(wlc_hw->osh, wlc_hw->regs, wlc_hw->macintmask);
}

/* mask off interrupts */
#define SET_MACINTMASK_OFF(osh, regs) { \
	SET_MACINTMASK(osh, regs, 0); \
	(void)GET_MACINTMASK(osh, regs);	/* sync readback */ \
	OSL_DELAY(1); /* ensure int line is no longer driven */ \
}

uint32
wlc_intrsoff(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint32 macintmask;

	if (!wlc_hw->clk)
		return 0;

#ifdef WLCXO
	if (WLCXO_ENAB(wlc->pub)) {
		return __wlc_cxo_c2d_intrsoff(wlc->ipc, wlc);
	}
#endif

	if (SRPWR_ENAB()) {
		wlc_srpwr_request_on(wlc);
	}

	macintmask = wlc_hw->macintmask;	/* isr can still happen */
	SET_MACINTMASK_OFF(wlc_hw->osh, wlc_hw->regs);
	wlc_hw->macintmask = 0;

	/* return previous macintmask; resolve race between us and our isr */
	return (wlc_hw->macintstatus ? 0 : macintmask);
}

/* deassert interrupt */
void
wlc_intrs_deassert(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;

	if (!wlc_hw->clk)
		return;

	if (SRPWR_ENAB()) {
		wlc_srpwr_request_on(wlc);
	}

	SET_MACINTMASK_OFF(wlc_hw->osh, wlc_hw->regs);
}

void
wlc_intrsrestore(wlc_info_t *wlc, uint32 macintmask)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;

	if (!wlc_hw->clk)
		return;

#ifdef WLCXO
	if (WLCXO_ENAB(wlc->pub)) {
		__wlc_cxo_c2d_intrsrestore(wlc->ipc, wlc, macintmask);
		return;
	}
#endif

	if (SRPWR_ENAB()) {
		wlc_srpwr_request_on(wlc);
	}

	wlc_hw->macintmask = macintmask;
	SET_MACINTMASK(wlc_hw->osh, wlc_hw->regs, wlc_hw->macintmask);
}

/* Read per fifo interrupt and process the frame if any interrupt is present */
static uint8
wlc_process_per_fifo_intr(wlc_info_t *wlc, bool bounded, wlc_dpc_info_t *dpc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint8 fifo_status = wlc_hw->dma_rx_intstatus;
	/* reset sw interrup bit */
	wlc_hw->dma_rx_intstatus = 0;
	/* process fifo- 0 */
	if (fifo_status & RX_INTR_FIFO_0) {
		if (wlc_bmac_recv(wlc_hw, RX_FIFO, bounded, dpc)) {
			/* More frames to be processed */
			wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_0;
		}
	}
	/* process fifo- 1 */
	if (PKT_CLASSIFY_EN(RX_FIFO1) || RXFIFO_SPLIT()) {
		if (fifo_status & RX_INTR_FIFO_1) {
			if (wlc_bmac_recv(wlc_hw, RX_FIFO1, bounded, dpc)) {
				/* More frames to be processed */
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_1;
			}
		}
	}
	/* Process fifo-2 */
	if (PKT_CLASSIFY_EN(RX_FIFO2)) {
		if (fifo_status & RX_INTR_FIFO_2) {
			if (wlc_bmac_recv(wlc_hw, RX_FIFO2, bounded, dpc)) {
				/* More frames to be processed */
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_2;
			}
		}
	}
	return wlc_hw->dma_rx_intstatus;
}

#if !defined(DMA_TX_FREE)
/* Return intstaus fof fifo-x */
static uint32
wlc_get_fifo_interrupt(wlc_info_t *wlc, uint8 FIFO)
{
	d11regs_t *regs = wlc->regs;
	return (R_REG(wlc->osh, &regs->intctrlregs[FIFO].intstatus) &
		DEF_RXINTMASK);
}
#endif /* !DMA_TX_FREE */
