/*
 * STA Module Public Interface
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_sta.h 601651 2015-11-23 20:23:53Z $
 */
#ifndef _WLC_STA_H_
#define _WLC_STA_H_

/* specific bsscfg that is not supported by this sta module handling */
#define BSSCFG_SPECIAL(wlc, cfg) \
	(BSSCFG_NAN(cfg) || BSSCFG_AIBSS(cfg) || BSSCFG_AWDL(wlc, cfg) || \
	BSS_TDLS_ENAB(wlc, cfg) || BSS_PROXD_ENAB(wlc, cfg))

/* multi-channel scheduler module interface */
wlc_sta_info_t* wlc_sta_attach(wlc_info_t* wlc);
void wlc_sta_detach(wlc_sta_info_t *sta_info);

#ifdef STA
extern int wlc_sta_timeslot_register(wlc_bsscfg_t *cfg);
extern void wlc_sta_timeslot_unregister(wlc_bsscfg_t *cfg);
extern bool wlc_sta_timeslot_registed(wlc_bsscfg_t *cfg);
extern void wlc_sta_timeslot_update(wlc_bsscfg_t *cfg, uint32 start_tsf, uint32 interval);

extern void wlc_sta_pm_pending_complete(wlc_info_t *wlc);
extern void wlc_sta_set_pmpending(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool pmpending);

#else

#define wlc_sta_timeslot_register(cfg)	(BCME_ERROR)
#define wlc_sta_timeslot_unregister(cfg)
#define wlc_sta_timeslot_registed(cfg)
#define wlc_sta_timeslot_update(cfg, start_tsf, interval)

#define wlc_sta_pm_pending_complete(wlc)
#define wlc_sta_set_pmpending(wlc, cfg, pmpending)
#define wlc_sta_fifo_suspend_complete(wlc, cfg)

#endif /* STA */

#endif /* _WLC_STA_H_ */
