/*
 * MCHAN related header file
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
 * $Id: wlc_mchan.h 601651 2015-11-23 20:23:53Z $
 */


#ifndef _wlc_mchan_h_
#define _wlc_mchan_h_

#ifdef WLMCHAN
#include <wlc_msch.h>
#include <wlc_types.h>

extern bool wlc_mchan_stago_is_disabled(mchan_info_t *mchan);
extern bool wlc_mchan_bypass_pm(mchan_info_t *mchan);
extern bool wlc_mchan_in_abs(mchan_info_t *mchan, wlc_bsscfg_t *bsscfg);

extern mchan_info_t *wlc_mchan_attach(wlc_info_t *wlc);
extern void wlc_mchan_detach(mchan_info_t *mchan);
extern uint16 wlc_mchan_get_pretbtt_time(mchan_info_t *mchan);
extern void wlc_mchan_recv_process_beacon(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, struct scb *scb,
	wlc_d11rxhdr_t *wrxh, uint8 *plcp, uint8 *body, int bcn_len);

extern 	wlc_bsscfg_t *wlc_mchan_get_cfg_frm_q(wlc_info_t *wlc, wlc_txq_info_t *qi);
extern 	wlc_bsscfg_t *wlc_mchan_get_other_cfg_frm_q(wlc_info_t *wlc, wlc_txq_info_t *qi);

extern int wlc_mchan_msch_clbk(void* handler_ctxt, wlc_msch_cb_info_t *cb_info);

extern void wlc_mchan_abs_proc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint32 tsf_l);
extern void wlc_mchan_psc_proc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint32 tsf_l);
extern void wlc_mchan_client_noa_clear(mchan_info_t *mchan, wlc_bsscfg_t *cfg);
extern bool wlc_mchan_check_tbtt_valid(mchan_info_t *mchan);
extern void wlc_mchan_reset_params(mchan_info_t * mchan);

void wlc_mchan_config_go_chanspec(mchan_info_t *mchan, wlc_bsscfg_t *cfg, chanspec_t chspec);
chanspec_t wlc_mchan_configd_go_chanspec(mchan_info_t *mchan, wlc_bsscfg_t *cfg);
bool wlc_mchan_ap_tbtt_setup(wlc_info_t *wlc, wlc_bsscfg_t *ap_cfg);
#if defined(WLRSDB) && defined(WL_MODESW)
extern void wlc_mchan_set_clone_pending(mchan_info_t* mchan, bool value);
extern bool wlc_mchan_get_clone_pending(mchan_info_t* mchan);
#endif
#ifdef WL_MODESW
extern bool wlc_mchan_get_modesw_pending(mchan_info_t *mchan);
#endif
#ifdef WLRSDB
extern void wlc_mchan_clone_context_all(wlc_info_t *from_wlc, wlc_info_t *to_wlc,
	wlc_bsscfg_t *cfg);
extern int wlc_mchan_modesw_set_cbctx(wlc_info_t *wlc, uint8 type);
#endif
#else	/* stubs */
#define wlc_mchan_attach(a) (mchan_info_t *)0x0dadbeef
#define	wlc_mchan_detach(a) do {} while (0)
#endif /* WLMCHAN */

#endif /* _wlc_mchan_h_ */
