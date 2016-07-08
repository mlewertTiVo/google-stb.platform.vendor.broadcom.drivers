/*
 * wlc_ltr.h
 *
 * Latency Tolerance Reporting
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_ltr.h 475674 2014-05-06 17:17:59Z $
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


#ifndef _wlc_ltr_h_
#define _wlc_ltr_h_
#ifdef WL_LTR

/*
 * Latency Tolerance Reporting (LTR) states
 * Active has the least tolerant latency requirement
 * Sleep is most tolerant
 */
#define LTR_ACTIVE					2
#define LTR_ACTIVE_IDLE				1
#define LTR_SLEEP					0

/*
 * Latency Tolerance Reporting (LTR) time tolerance
 */
#define LTR_LATENCY_60US		60	/* in us */
#define LTR_LATENCY_100US		100	/* in us */
#define LTR_LATENCY_150US		150	/* in us */
#define LTR_LATENCY_200US		200	/* in us */
#define LTR_LATENCY_300US		300	/* in us */
#define LTR_LATENCY_3MS			3	/* in ms */

#define LTR_MAX_ACTIVE_LATENCY		600	/* in us */
#define LTR_MAX_SLEEP_LATENCY		6	/* in ms */

/*
 * Number of queued packets on host FIFO that trigger
 * an LTR active (hi) or sleep (lo) transition
 */
#define LTR_HI_WATERMARK			50
#define LTR_LO_WATERMARK			20

#define LTR_MAX_HI_WATERMARK		64
#define LTR_MIN_LO_WATERMARK		10

/*
 * Unit of latency stored in LTR registers
 * 2 => latency in microseconds
 * 4 => latency in milliseconds
 */
#define LTR_SCALE_US				2
#define LTR_SCALE_MS				4

struct wlc_ltr_info {
	wlc_info_t *wlc;		/* Back pointer to wlc */
	uint32 active_lat;		/* LTR active latency */
	uint32 active_idle_lat;	/* LTR active idle latency (not used) */
	uint32 sleep_lat;		/* LTR sleep latency */
	uint32 hi_wm;			/* FIFO watermark for LTR active transition */
	uint32 lo_wm;			/* FIFO watermark for LTR sleep transition */
};

/* API: Module-system interfaces */
extern wlc_ltr_info_t* wlc_ltr_attach(wlc_info_t *wlc);
void wlc_ltr_detach(wlc_ltr_info_t *ltr_info);
extern void wlc_ltr_init_reg(si_t *sih, uint32 reg, uint32 latency, uint32 unit);
extern int wlc_ltr_init(wlc_info_t *wlc);
extern int wlc_ltr_up_prep(wlc_info_t *wlc, uint32 ltr_state);
extern int wlc_ltr_hwset(wlc_hw_info_t *wlc_hw, d11regs_t *regs, uint32 ltr_state);

#endif /* WL_LTR */
#endif /* _wlc_ltr_h_ */
