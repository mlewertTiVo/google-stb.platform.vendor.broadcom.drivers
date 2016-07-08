/*
 * wlc_ltrol.c
 *
 * Latency Tolerance Reporting (offload device driver)
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_ltrol.c 467328 2014-04-03 01:23:40Z $
 *
 */

/**
 * @file
 * @brief
 * Latency Tolerance Reporting (LTR) is a mechanism for devices to report their
 * service latency requirements for Memory Reads and Writes to the PCIe Root Complex
 * such that platform resources can be power managed without impacting device
 * functionality and performance. Platforms want to consume the least amount of power.
 * Devices want to provide the best performance and functionality (which needs power).
 * A device driver can mediate between these conflicting goals by intelligently reporting
 * device latency requirements based on the state of the device.
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_ltr.h>
#include <bcm_ol_msg.h>
#include <wl_export.h>
#include <wlc_dngl_ol.h>
#include <wlc_ltrol.h>

struct wlc_dngl_ol_ltr_info {
	wlc_dngl_ol_info_t	*wlc_dngl_ol; /* Back pointer */
	uint32 active_lat;		/* LTR active latency */
	uint32 active_idle_lat;	/* LTR active idle latency (not used) */
	uint32 sleep_lat;		/* LTR sleep latency */
	uint32 hi_wm;			/* FIFO watermark for LTR active transition */
	uint32 lo_wm;			/* FIFO watermark for LTR sleep transition */
};

wlc_dngl_ol_ltr_info_t *
wlc_dngl_ol_ltr_attach(wlc_dngl_ol_info_t *wlc_dngl_ol)
{
	wlc_dngl_ol_ltr_info_t *ltr_ol;
	ltr_ol = (wlc_dngl_ol_ltr_info_t *)MALLOC(wlc_dngl_ol->osh, sizeof(wlc_dngl_ol_ltr_info_t));
	if (!ltr_ol) {
		WL_ERROR((" ltr_ol malloc failed: %s\n", __FUNCTION__));
		return NULL;
	}
	bzero(ltr_ol, sizeof(wlc_dngl_ol_ltr_info_t));
	ltr_ol->wlc_dngl_ol = wlc_dngl_ol;
	/*
	 * Set _ltr to false for now,
	 * wlc_dngl_ol_ltr_proc_msg() will set actual value
	 */
	wlc_dngl_ol->_ltr = FALSE;
	ltr_ol->active_idle_lat = LTR_LATENCY_100US;
	ltr_ol->active_lat = LTR_LATENCY_60US;
	ltr_ol->sleep_lat = LTR_LATENCY_3MS;
	ltr_ol->hi_wm = LTR_HI_WATERMARK;
	ltr_ol->lo_wm = LTR_LO_WATERMARK;
	return ltr_ol;
}

void
wlc_dngl_ol_ltr_proc_msg(wlc_dngl_ol_info_t *wlc_dngl_ol, void *buf, int len)
{
	olmsg_ltr *ltr = buf;

	wlc_dngl_ol->_ltr = ltr->_ltr;
	wlc_dngl_ol->ltr_ol->active_idle_lat = ltr->active_idle_lat;
	wlc_dngl_ol->ltr_ol->active_lat = ltr->active_lat;
	wlc_dngl_ol->ltr_ol->sleep_lat = ltr->sleep_lat;
	wlc_dngl_ol->ltr_ol->hi_wm = ltr->hi_wm;
	wlc_dngl_ol->ltr_ol->lo_wm = ltr->lo_wm;
}

void
wlc_dngl_ol_ltr_update(wlc_dngl_ol_info_t *wlc_dngl_ol, osl_t *osh)
{
	int pkts_queued;
	d11regs_t *d11_regs = (d11regs_t *)wlc_dngl_ol->regs;

	/* Change latency if number of packets queued on host FIFO cross LTR watermarks */
	pkts_queued = R_REG(osh, &d11_regs->u_rcv.d11acregs.rcv_frm_cnt_q0);
	WL_TRACE(("%s: %d packet(s) queued on host FIFO\n", __FUNCTION__, pkts_queued));
	if (pkts_queued > wlc_dngl_ol->ltr_ol->hi_wm) {
		WL_TRACE(("%s: Calling wlc_ltr_hwset(LTR_ACTIVE) from ARM\n",
			__FUNCTION__));
		wlc_ltr_hwset(wlc_dngl_ol->wlc_hw, d11_regs, LTR_ACTIVE);
	}
	if (pkts_queued <= wlc_dngl_ol->ltr_ol->lo_wm) {
		uint32 maccontrol = R_REG(osh, &d11_regs->maccontrol);
		uint32 hps_bit = (maccontrol & MCTL_HPS)?1:0;
		uint32 wake_bit = (maccontrol & MCTL_WAKE)?1:0;
		WL_TRACE(("%s: maccontrol: hps %d, wake %d\n", __FUNCTION__,
			hps_bit, wake_bit));
		if (hps_bit & !wake_bit) {
			/* Send LTR sleep message only if host is not active */
			WL_TRACE(("%s: Calling wlc_ltr_hwset(LTR_SLEEP) from ARM\n",
				__FUNCTION__));
			wlc_ltr_hwset(wlc_dngl_ol->wlc_hw, d11_regs, LTR_SLEEP);
		}
	}
}
