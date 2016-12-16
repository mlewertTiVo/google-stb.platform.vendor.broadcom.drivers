/*
 * performance measurement/analysis facilities
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
 * $Id: wlc_perf_utils.h 659395 2016-09-14 03:09:14Z $
 */

#ifndef _wlc_perf_utils_h_
#define _wlc_perf_utils_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <bcmutils.h>
#include <wlioctl.h>

#if defined(BCMPCIEDEV) && defined(BUS_TPUT)
typedef struct bmac_hostbus_tput_info {
	struct wl_timer	*dma_timer; /* timer for bus throughput measurement routine */
	struct wl_timer	*end_timer; /* timer for stopping bus throughput measurement */
	bool		test_running; /* bus throughput measurement is running or not */
	uint32		pktcnt_tot; /* total no of dma completed */
	uint32		pktcnt_cur; /* no of descriptors reclaimed after previous commit */
	uint32		max_dma_descriptors;	/* no of host descriptors programmed by */
						    /* the firmware before a dma commit */
	/* block of host memory for measuring bus throughput */
	dmaaddr_t	host_buf_addr;
	uint16		host_buf_len;
} bmac_hostbus_tput_info_t;

extern void wlc_bmac_tx_fifos_flush(wlc_hw_info_t *wlc_hw, uint fifo_bitmap);
extern void wlc_bmac_tx_fifo_sync(wlc_hw_info_t *wlc_hw, uint fifo_bitmap, uint8 flag);
extern void wlc_bmac_suspend_mac_and_wait(wlc_hw_info_t *wlc_hw);
#endif /* BCMPCIEDEV && BUS_TPUT */

#ifdef WLPKTDLYSTAT
/* Macro used to get the current local time (in us) */
#define WLC_GET_CURR_TIME(wlc)		(R_REG((wlc)->osh, &(wlc)->regs->tsf_timerlow))

extern void wlc_scb_dlystat_dump(scb_t *scb, struct bcmstrbuf *b);
extern void wlc_dlystats_clear(wlc_info_t *wlc);
extern void wlc_delay_stats_upd(scb_delay_stats_t *delay_stats, uint32 delay, uint tr,
	bool ack_recd);
#endif /* WLPKTDLYSTAT */

#if defined(BCMDBG)
/* Mask for individual stats */
#define WLC_PERF_STATS_ISR		0x01
#define WLC_PERF_STATS_DPC		0x02
#define WLC_PERF_STATS_TMR_DPC		0x04
#define WLC_PERF_STATS_PRB_REQ		0x08
#define WLC_PERF_STATS_PRB_RESP		0x10
#define WLC_PERF_STATS_BCN_ISR		0x20
#define WLC_PERF_STATS_BCNS		0x40

/* Performance statistics interfaces */
void wlc_update_perf_stats(wlc_info_t *wlc, uint32 mask);
void wlc_update_isr_stats(wlc_info_t *wlc, uint32 macintstatus);
void wlc_update_p2p_stats(wlc_info_t *wlc, uint32 macintstatus);
#endif /* BCMDBG */

#ifdef PKTQ_LOG
void wlc_pktq_stats_free(wlc_info_t* wlc, struct pktq* q);
pktq_counters_t *wlc_txq_prec_log_enable(wlc_info_t* wlc, struct pktq* q, uint32 i);

#endif /* PKTQ_LOG */

wlc_perf_utils_t *wlc_perf_utils_attach(wlc_info_t *wlc);
void wlc_perf_utils_detach(wlc_perf_utils_t *pui);

#endif /* _wlc_perf_utils_h_ */
