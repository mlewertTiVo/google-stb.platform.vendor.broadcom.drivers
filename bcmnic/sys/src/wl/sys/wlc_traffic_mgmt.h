/*
 * Common (OS-independent) portion of
 * Broadcom traffic management support
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
 * $Id: wlc_traffic_mgmt.h 535090 2015-02-17 04:49:01Z $
 */


#ifndef _wlc_traffic_mgmt_h_
#define _wlc_traffic_mgmt_h_

#ifdef TRAFFIC_MGMT

/*
 * Initialize traffic management private context.
 * Returns a pointer to the traffic management private context, NULL on failure.
 */
extern wlc_trf_mgmt_ctxt_t *wlc_trf_mgmt_attach(wlc_info_t *wlc);

/* Cleanup traffic management private context */
extern void wlc_trf_mgmt_detach(wlc_trf_mgmt_ctxt_t *trf_mgmt_ctxt);

/* Handle frames for traffic management */
extern int wlc_trf_mgmt_handle_pkt(
		wlc_trf_mgmt_ctxt_t *trf_mgmt_ctxt,
		wlc_bsscfg_t        *bsscfg,
		struct scb          *scb,
		void                *pkt,
		bool                in_tx_path);

/* Handle a bsscfg allocate event */
extern int wlc_trf_mgmt_bsscfg_allocate(wlc_trf_mgmt_ctxt_t *trf_mgmt_ctxt, wlc_bsscfg_t *bsscfg);

/* Handle a bsscfg free event */
extern void wlc_trf_mgmt_bsscfg_free(wlc_trf_mgmt_ctxt_t *trf_mgmt_ctxt, wlc_bsscfg_t *bsscfg);

/* add trf to scb data path */
extern void wlc_scb_trf_mgmt(wlc_info_t *wlc,  wlc_bsscfg_t *bsscfg, struct scb *scb);

#endif  /* TRAFFIC_MGMT */

#ifdef WLINTFERSTAT
extern void wlc_trf_mgmt_scb_txfail_detect(wlc_trf_mgmt_ctxt_t *trf_mgmt_ctxt, struct scb *scb);
#endif /* WLINTFERSTAT */
#endif  /* _wlc_traffic_mgmt_h_ */
