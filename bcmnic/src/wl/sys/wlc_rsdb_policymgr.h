/*
 * WLC RSDB POLICY MGR API definition
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_rsdb_policymgr.h 617509 2016-02-05 15:01:32Z $
 */


#ifndef _wlc_rsdb_policy_h_
#define _wlc_rsdb_policy_h_
#ifdef WLRSDB_POLICY_MGR

typedef enum {
	WLC_RSDB_POLICY_PRE_MODE_SWITCH,
	WLC_RSDB_POLICY_POST_MODE_SWITCH
} wlc_rsdb_policymgr_states_t;

typedef struct {
	wlc_rsdb_policymgr_states_t	 state;
	wlc_rsdb_modes_t target_mode;
} wlc_rsdb_policymgr_state_upd_data_t;

#ifdef WLMSG_RSDB_PMGR
#if defined(ERR_USE_EVENT_LOG)
#if defined(ERR_USE_EVENT_LOG_RA)
#define   WL_RSDB_PMGR_DEBUG(args)	EVENT_LOG_RA(EVENT_LOG_TAG_RSDB_PMGR_DEBUG, args)
#define   WL_RSDB_PMGR_ERR(args)	EVENT_LOG_RA(EVENT_LOG_TAG_RSDB_PMGR_ERR, args)
#else
#define   WL_RSDB_PMGR_DEBUG(args) \
	EVENT_LOG_FAST_CAST_PAREN_ARGS(EVENT_LOG_TAG_RSDB_PMGR_DEBUG, args)
#define   WL_RSDB_PMGR_ERR(args) EVENT_LOG_FAST_CAST_PAREN_ARGS(EVENT_LOG_TAG_RSDB_PMGR_ERR, args)
#endif /* ERR_USE_EVENT_LOG_RA */
#else
#define   WL_RSDB_PMGR_DEBUG(args)	WL_PRINT(args)
#define   WL_RSDB_PMGR_ERR(args)	WL_PRINT(args)
#endif /* ERR_USE_EVENT_LOG */
#else
#define	WL_RSDB_PMGR_DEBUG(args)
#define   WL_RSDB_PMGR_ERR(args)	WL_ERROR(args)
#endif /* WLMSG_RSDB_PMGR */

wlc_rsdb_policymgr_info_t* wlc_rsdb_policymgr_attach(wlc_info_t* wlc);
void wlc_rsdb_policymgr_detach(wlc_rsdb_policymgr_info_t *policy_info);

bool wlc_rsdb_policymgr_is_sib_set(wlc_rsdb_policymgr_info_t *policy_info, int bandindex);

int wlc_rsdb_policymgr_state_upd_register(wlc_rsdb_policymgr_info_t *policy_info,
	bcm_notif_client_callback fn, void *arg);
int wlc_rsdb_policymgr_state_upd_unregister(wlc_rsdb_policymgr_info_t *policy_info,
	bcm_notif_client_callback fn, void *arg);

wlc_infra_modes_t wlc_rsdb_policymgr_find_infra_mode(wlc_rsdb_policymgr_info_t *policy_info,
	wlc_bss_info_t *bi);
int rsdb_policymgr_get_non_infra_wlcs(void *ctx, wlc_info_t *wlc, wlc_info_t **wlc_2g,
	wlc_info_t **wlc_5g);

#ifdef WLRSDB_MIMO_DEFAULT
int
rsdb_policymgr_set_noninfra_default_mode(wlc_rsdb_policymgr_info_t *policy_info);
#endif
#endif /* WLRSDB_POLICY_MGR */
#endif /* _wlc_rsdb_policy_h_ */
