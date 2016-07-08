/*
 * performance measurement/analysis facilities
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
 * $Id: wlc_perf_utils.h 612761 2016-01-14 23:06:00Z $
 */

#ifndef _wlc_perf_utils_h_
#define _wlc_perf_utils_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <bcmutils.h>
#include <wlioctl.h>

#ifdef WLPKTDLYSTAT
/* Macro used to get the current local time (in us) */
#define WLC_GET_CURR_TIME(wlc)		(R_REG((wlc)->osh, &(wlc)->regs->tsf_timerlow))

extern void wlc_scb_dlystat_dump(scb_t *scb, struct bcmstrbuf *b);
extern void wlc_dlystats_clear(wlc_info_t *wlc);
extern void wlc_delay_stats_upd(scb_delay_stats_t *delay_stats, uint32 delay, uint tr,
	bool ack_recd);
#endif /* WLPKTDLYSTAT */


#ifdef PKTQ_LOG
void wlc_pktq_stats_free(wlc_info_t* wlc, struct pktq* q);
pktq_counters_t *wlc_txq_prec_log_enable(wlc_info_t* wlc, struct pktq* q, uint32 i);

#endif /* PKTQ_LOG */

wlc_perf_utils_t *wlc_perf_utils_attach(wlc_info_t *wlc);
void wlc_perf_utils_detach(wlc_perf_utils_t *pui);

#endif /* _wlc_perf_utils_h_ */
