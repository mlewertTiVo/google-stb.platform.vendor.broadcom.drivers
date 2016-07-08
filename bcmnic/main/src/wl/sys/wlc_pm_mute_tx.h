/*
 * Support for power-save mode with muted transmit path.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_pm_mute_tx.h 2014-01-31 mgs$
 */

#ifndef _wlc_pm_mute_tx_h_
#define _wlc_pm_mute_tx_h_

typedef enum pm_mute_tx_st {
	DISABLED_ST,	/* pm_mute_tx mode is disabled; */
	PM_ST,		/* executing operations to start power save mode; */
#ifdef PMMT_ENABLE_MUTE
	MUTE_ST,	/* executing operations to suspend tx fifos; */
#endif
	COMPLETE_ST,	/* executing final operations to complete pm_mute_tx req */
	ENABLED_ST,	/* pm_mute_tx mode is enabled; */
	FAIL_ST,	/* last request to enable pm_mode_tx failed (limbo state) */
	MAX_STATES
} pm_mute_tx_st_t;

struct wlc_pm_mute_tx_info {
	wlc_info_t		*wlc;
	wlc_bsscfg_t		*bsscfg;
	uint16			deadline;
	uint8			initial_pm;
	pm_mute_tx_st_t		state;
	uint8			status;
	struct wl_timer		*timer;
};

extern wlc_pm_mute_tx_t *wlc_pm_mute_tx_attach(wlc_info_t *wlc);
extern void wlc_pm_mute_tx_detach(wlc_pm_mute_tx_t *pmmt);
extern void wlc_pm_mute_tx_pm_pending_complete(wlc_pm_mute_tx_t *pmmt);

#endif /* _wlc_pm_mute_tx_h_ */
