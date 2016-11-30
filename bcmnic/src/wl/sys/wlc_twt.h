/*
 * 802.11ah/11ax Target Wake Time protocol and d11 h/w manipulation.
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_twt.h 632385 2016-04-19 04:53:03Z $
 */

#ifndef _wlc_twt_h_
#define _wlc_twt_h_

#include <typedefs.h>
#include <wlc_types.h>

#include <proto/802.11ah.h>

/* attach/detach */
wlc_twt_info_t *wlc_twt_attach(wlc_info_t *wlc);
void wlc_twt_detach(wlc_twt_info_t *twti);

/* can the HE/S1G STA be requestor/responder? */
bool wlc_twt_req_cap(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg);
bool wlc_twt_resp_cap(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg);

/* ======== interface between TWT and its users (11ax, 11ah...) ======== */

/* ---- TWT Setup ---- */

/* flow_id */
#define WLC_TWT_SETUP_FLOW_ID_AUTO	0xFF

/* validate flow id or assign flow id if auto and reserve resources */
int wlc_twt_reserve(wlc_twt_info_t *twti, struct scb *scb, wl_twt_sdesc_t *desc);

/* release flow id & resources */
void wlc_twt_release(wlc_twt_info_t *twti, struct scb *scb, uint8 flow_id);

/* build twt IE */
uint wlc_twt_ie_len_calc(wlc_twt_info_t *twti, wl_twt_sdesc_t *desc, uint16 num_desc);
void wlc_twt_ie_build(wlc_twt_info_t *twti, wl_twt_sdesc_t *desc, uint16 num_desc,
	uint8 *body, uint body_len);

/* parse twt IE */
int wlc_twt_ie_proc(wlc_twt_info_t *twti, struct scb *scb,
	twt_ie_top_t *body, uint body_len, wl_twt_sdesc_t *desc, bool *resp);

/* configure broadcast twt and program twt parms to HW */
int wlc_twt_setup_bcast(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg, wl_twt_sdesc_t *desc);

/* program twt parms to HW */
int wlc_twt_setup(wlc_twt_info_t *twti, struct scb *scb, wl_twt_sdesc_t *desc);

/* ---- TWT Teardown ---- */

/* init twt teardown */
uint wlc_twt_teardown_len_calc(wlc_twt_info_t *twti);
void wlc_twt_teardown_init(wlc_twt_info_t *twti, uint8 flow_id,
	uint8 *body, uint body_len);

/* process twt teardown */
int wlc_twt_teardown_proc(wlc_twt_info_t *twti, struct scb *scb,
	uint8 *body, uint body_len);

/* remove broadcast twt flow from HW (and also release other resources) */
int wlc_twt_teardown_bcast(wlc_twt_info_t *twti, wlc_bsscfg_t *cfg, uint8 flow_id);

/* remove twt flow from HW (and also free flow id and release other resources) */
int wlc_twt_teardown(wlc_twt_info_t *twti, struct scb *scb, uint8 flow_id);

/* ---- TWT Information ---- */

/* init twt information */
uint wlc_twt_info_len_calc(wlc_twt_info_t *twti, wl_twt_idesc_t *desc);
void wlc_twt_info_init(wlc_twt_info_t *twti, wl_twt_idesc_t *desc,
	uint8 *body, uint body_len);

/* parse twt information field */
int wlc_twt_info_proc(wlc_twt_info_t *twti, struct scb *scb,
	uint8 *body, uint body_len, wl_twt_idesc_t *desc, bool *resp);

/* ======== other interfaces ======== */

#endif /* _wlc_twt_h_ */
