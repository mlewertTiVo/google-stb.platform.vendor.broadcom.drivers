/**
 * @file
 * @brief
 * ACTION FRAME Module Public Interface
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
 * $Id: wlc_act_frame.h 658636 2016-09-08 19:27:17Z $
 */

#ifndef _WLC_ACT_FRAME_H_
#define _WLC_ACT_FRAME_H_

wlc_act_frame_info_t* wlc_act_frame_attach(wlc_info_t* wlc);
void wlc_act_frame_detach(wlc_act_frame_info_t *mctxt);

extern int wlc_send_action_frame(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
	const struct ether_addr *bssid, void *action_frame);
extern int wlc_tx_action_frame_now(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, struct scb *scb);
extern int wlc_is_publicaction(wlc_info_t * wlc, struct dot11_header *hdr,
                               int len, struct scb *scb, wlc_bsscfg_t *bsscfg);

typedef int (*wlc_act_frame_tx_cb_fn_t)(wlc_info_t *wlc, void *arg, void *pkt);

extern int wlc_send_action_frame_off_channel(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	chanspec_t chanspec, int32 dwell_time, struct ether_addr *bssid,
	wl_action_frame_t *action_frame, wlc_act_frame_tx_cb_fn_t cb, void *arg);

/*
 * Action frame call back interface declaration
 */
typedef void (*wlc_actframe_callback)(wlc_info_t *wlc, void* handler_ctxt, uint *arg);

extern int wlc_msch_actionframe(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	uint32 channel, uint32 dwell_time, struct ether_addr *bssid,
	wlc_actframe_callback cb_func, void *arg);

#define ACT_FRAME_IN_PROGRESS(wlc, cfg) wlc_act_frame_tx_inprog(wlc, cfg)
bool wlc_act_frame_tx_inprog(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

extern void wlc_set_protected_dual_publicaction(uint8 *action_frame,
	uint8 mfp, wlc_bsscfg_t *bsscfg);

bool wlc_is_protected_dual_publicaction(uint8 act, wlc_bsscfg_t *bsscfg);
#endif /* _WLC_ACT_FRAME_H_ */
