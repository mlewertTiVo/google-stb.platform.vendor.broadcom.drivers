/*
 * CHAN CONTEXT Module Public Interface
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
 * $Id: wlc_chanctxt.h 601651 2015-11-23 20:23:53Z $
 */
#ifndef _WLC_CHANCTXT_H_
#define _WLC_CHANCTXT_H_

/* multi-channel scheduler module interface */
extern wlc_chanctxt_info_t* wlc_chanctxt_attach(wlc_info_t* wlc);
extern void wlc_chanctxt_detach(wlc_chanctxt_info_t *chanctxt_info);

/** bsscfg packets backup callback function. */
typedef int (*wlc_chanctxt_backup_fn_t)(wlc_bsscfg_t *cfg, void *pkt, int prec);
typedef void *(*wlc_chanctxt_restore_fn_t)(wlc_bsscfg_t *cfg, int *prec);

extern void wlc_txqueue_start(wlc_bsscfg_t *cfg, chanspec_t chanspec,
	wlc_chanctxt_restore_fn_t restore_fn);
extern void wlc_txqueue_end(wlc_bsscfg_t *cfg, wlc_chanctxt_backup_fn_t backup_fn);
extern bool wlc_has_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern chanspec_t wlc_get_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern bool wlc_shared_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg1, wlc_bsscfg_t *cfg2);
extern bool _wlc_shared_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	wlc_chanctxt_t *shared_chanctxt);
extern bool wlc_shared_current_chanctxt(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
extern bool _wlc_ovlp_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg, chanspec_t chspec, uint chbw);
extern bool wlc_ovlp_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg1, wlc_bsscfg_t *cfg2, uint chbw);

#endif /* _WLC_CHANCTXT_H_ */
