/*
 * ratespec related routines
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
 * $Id: wlc_rspec.h 644052 2016-06-17 03:04:30Z $
 */

#ifndef __wlc_rspec_h__
#define __wlc_rspec_h__

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_rate.h>

extern int wlc_set_iovar_ratespec_override(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
                                           int band_id, uint32 wl_rspec, bool mcast);
extern ratespec_t wlc_rspec_to_rts_rspec(wlc_bsscfg_t *cfg, ratespec_t rspec, bool use_rspec);
extern ratespec_t wlc_lowest_basic_rspec(wlc_info_t *wlc, wlc_rateset_t *rs);

#if defined(MBSS) || defined(WLTXMONITOR)
extern ratespec_t ofdm_plcp_to_rspec(uint8 rate);
#endif /* MBSS || WLTXMONITOR */

extern uint32 wlc_get_rspec_history(wlc_bsscfg_t *cfg);
extern uint32 wlc_get_current_highest_rate(wlc_bsscfg_t *cfg);
void wlc_bsscfg_reprate_init(wlc_bsscfg_t *bsscfg);
void wlc_rspec_txexp_upd(wlc_info_t *wlc, ratespec_t *rspec);

/* attach/detach */
wlc_rspec_info_t *wlc_rspec_attach(wlc_info_t *wlc);
void wlc_rspec_detach(wlc_rspec_info_t *rsi);

#endif /* __wlc_rspec_h__ */
