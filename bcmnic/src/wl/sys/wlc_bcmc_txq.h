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
 * $Id: wlc_bcmc_txq.h 664818 2016-10-13 22:06:23Z $
 */

/**
 * @file
 * @brief Broadcast MUX layer functions for Broadcom 802.11 Networking Driver
 */

#ifndef WLC_BCMC_TXQ_H
#define WLC_BCMC_TXQ_H

#define WLC_BCMC_PSON(cfg) wlc_bcmc_set_powersave((cfg), TRUE)
#define WLC_BCMC_PSOFF(cfg) wlc_bcmc_set_powersave((cfg), FALSE)

extern int wlc_bsscfg_mux_move(wlc_info_t *wlc,
	wlc_bsscfg_t *cfg, struct wlc_txq_info *new_qi);
extern int wlc_bsscfg_recreate_muxsrc(wlc_info_t *wlc,
wlc_bsscfg_t *cfg, struct wlc_txq_info *new_qi);
extern void wlc_bsscfg_delete_muxsrc(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
void wlc_bcmc_set_powersave(wlc_bsscfg_t *cfg, bool ps_enable);
extern int wlc_bcmc_mux_move(wlc_info_t *wlc,
	wlc_bsscfg_t *cfg, struct wlc_txq_info *new_qi);
extern int wlc_bcmc_recreatesrc(wlc_info_t *wlc,
	wlc_bsscfg_t *cfg,  struct wlc_txq_info *new_qi);
extern void wlc_bcmc_delsrc(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
bool wlc_bcmc_suppress_enqueue(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, uint prec);
void wlc_bcmc_stop_mux_sources(wlc_bsscfg_t *cfg);
void wlc_bcmc_start_mux_sources(wlc_bsscfg_t *cfg);
void wlc_bcmc_global_start_mux_sources(wlc_info_t *wlc);
void wlc_bcmc_global_stop_mux_sources(wlc_info_t *wlc);

/**
 * @brief Allocate and initialize the SCBQ module.
 */
wlc_bcmcq_info_t *wlc_bcmcq_module_attach(wlc_info_t *wlc);

/**
 * @brief Free all resources of the SCBQ module
 */
void wlc_bcmcq_module_detach(wlc_bcmcq_info_t *bcmcq_info);


#endif /* WLC_BCMC_TXQ_H */
