
/*
 * OBSS and bandwidth switch utilities
 * Broadcom 802.11 Networking Device Driver
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
 * $Id: wlc_obss_util.c 637713 2016-05-13 22:12:35Z $
 */

/**
 * @file
 * @brief
 * Out of Band BSS
 * Banwidth switch utilities
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_scan.h>
#include <wlc_bmac.h>
#include <wlc_obss_util.h>

void
wlc_obss_util_update(wlc_info_t *wlc, wlc_bmac_obss_counts_t *curr,
		wlc_bmac_obss_counts_t *prev, wlc_bmac_obss_counts_t *o_total, chanspec_t chanspec)
{
	uint16 delta;
	if (SCAN_IN_PROGRESS(wlc->scan)) {
		return;
	}

	BCM_REFERENCE(delta);

	/* Save a copy of previous counters */
	memcpy(prev, curr, sizeof(*prev));

	/* Read current ucode counters */
	wlc_bmac_obss_stats_read(wlc->hw, curr);

	/*
	 * Calculate the total counts.
	 */

	/* CCA duration stats */
	o_total->usecs += curr->usecs - prev->usecs;
	o_total->txdur += curr->txdur - prev->txdur;
	o_total->ibss += curr->ibss - prev->ibss;
	o_total->obss += curr->obss - prev->obss;
	o_total->noctg += curr->noctg - prev->noctg;
	o_total->nopkt += curr->nopkt - prev->nopkt;
	o_total->PM += curr->PM - prev->PM;
	o_total->txopp += curr->txopp - prev->txopp;
	o_total->gdtxdur += curr->gdtxdur - prev->gdtxdur;
	o_total->bdtxdur += curr->bdtxdur - prev->bdtxdur;
	o_total->slot_time_txop = curr->slot_time_txop;

	/* OBSS stats */
	delta = curr->rxdrop20s - prev->rxdrop20s;
	o_total->rxdrop20s += delta;
	delta = curr->rx20s - prev->rx20s;
	o_total->rx20s += delta;

	o_total->rxcrs_pri += curr->rxcrs_pri - prev->rxcrs_pri;
	o_total->rxcrs_sec20 += curr->rxcrs_sec20 - prev->rxcrs_sec20;
	o_total->rxcrs_sec40 += curr->rxcrs_sec40 - prev->rxcrs_sec40;

	delta = curr->sec_rssi_hist_hi - prev->sec_rssi_hist_hi;
	o_total->sec_rssi_hist_hi += delta;
	delta = curr->sec_rssi_hist_med - prev->sec_rssi_hist_med;
	o_total->sec_rssi_hist_med += delta;
	delta = curr->sec_rssi_hist_low - prev->sec_rssi_hist_low;
	o_total->sec_rssi_hist_low += delta;

	/* counters */
	o_total->crsglitch += curr->crsglitch - prev->crsglitch;
	o_total->bphy_crsglitch += curr->bphy_crsglitch - prev->bphy_crsglitch;
	o_total->badplcp += curr->badplcp - prev->badplcp;
	o_total->bphy_badplcp += curr->bphy_badplcp - prev->bphy_badplcp;
	o_total->badfcs += curr->badfcs - prev->badfcs;
	o_total->suspend += curr->suspend - prev->suspend;
	o_total->suspend_cnt += curr->suspend_cnt - prev->suspend_cnt;
	o_total->txfrm += curr->txfrm - prev->txfrm;
	o_total->rxstrt += curr->rxstrt - prev->rxstrt;
}
