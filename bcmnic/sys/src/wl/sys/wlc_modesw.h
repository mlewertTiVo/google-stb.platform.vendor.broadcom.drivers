/*
 * Exposed interfaces of wlc_modesw.c
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
 * $Id: wlc_modesw.h 613250 2016-01-18 09:56:05Z $
 */

#ifndef _wlc_modesw_h_
#define _wlc_modesw_h_

#define BW_SWITCH_TYPE_UPGRADE	1
#define BW_SWITCH_TYPE_DNGRADE	2
#define MSW_MODE_SWITCH_TIMEOUT 3000

/* Notification call back data */
enum notif_events {
	MODESW_DN_STA_COMPLETE = 1,
	MODESW_UP_STA_COMPLETE = 2,
	MODESW_DN_AP_COMPLETE = 3,
	MODESW_UP_AP_COMPLETE = 4,
	MODESW_PHY_UP_COMPLETE = 5,
	MODESW_PHY_DN_COMPLETE = 6,
	MODESW_ACTION_FAILURE = 7,
	MODESW_STA_TIMEOUT = 8,
	MODESW_PHY_UP_START = 9,
	MODESW_LAST = 10
};

/* This enum defines the current mode switch state. At any point of time we can get
 * to know, what state we are currently in. i.e. No switch in progress or RSDB -> VSDB
 * or VSDB -> RSDB.
 */
typedef enum {
	MODESW_NO_SW_IN_PROGRESS = 0, /* No switch */
	MODESW_RSDB_TO_VSDB = 1, /* RSDB to VSDB switch is going on */
	MODESW_VSDB_TO_RSDB = 2, /* VSDB to RSDB switch is going on */
	MODESW_LIST_END
} modesw_type;

typedef struct {
	wlc_bsscfg_t *cfg;
	uint8 opmode;
	uint8 opmode_old;
	int status;
	int signal;
} wlc_modesw_notif_cb_data_t;

#define WLC_MODESW_ANY_SWITCH_IN_PROGRESS(modesw) (wlc_modesw_get_switch_type(modesw) > \
		MODESW_NO_SW_IN_PROGRESS && wlc_modesw_get_switch_type(modesw) < MODESW_LIST_END)
#define MODESW_CTRL_UP_SILENT_UPGRADE			(0x0001)
#define MODESW_CTRL_DN_SILENT_DNGRADE			(0x0002)
#define MODESW_CTRL_UP_ACTION_FRAMES_ONLY		(0x0004)
#define MODESW_CTRL_AP_ACT_FRAMES			(0x0008)
/* Flag to handle all BSSCFGs. for eg: in DYN BWSW
* for PSTA/MBSS cases, we would need a callback from
* modesw for each BSSCFG while normally we would need
* just a single modesw cb. This flag helps to ensure that
* we get a callback for every cfg
*/
#define MODESW_CTRL_HANDLE_ALL_CFGS			(0x0010)
/* Flag to enable disassoc of non-acking STAs/APs
*/
#define MODESW_CTRL_NO_ACK_DISASSOC			(0x0020)
#define MODESW_CTRL_RSDB_MOVE				(0x0040)

#define MODESW_BSS_ACTIVE(wlc, cfg)			\
	(	/* for assoc STA's... */					\
	((BSSCFG_STA(cfg) && cfg->associated) ||	\
	/* ... or UP AP's ... */						\
	(BSSCFG_AP(cfg) && cfg->up)))

#define MODE_SWITCH_IN_PROGRESS(x)	wlc_modesw_in_progress(x)

/* Time Measurement Structure   */
typedef struct modesw_time_calc {
/* Normal Downgrade  */
	uint32 ActDN_SeqCnt;
	uint32 DN_start_time;
	uint32 DN_ActionFrame_time;
	uint32 DN_PHY_BW_UPDTime;
	uint32 DN_CompTime;
/* Only PHY Upgrade */
	uint32 PHY_UP_SeqCnt;
	uint32 PHY_UP_start_time;
	uint32 PHY_UP_PM1_time;
	uint32 PHY_UP_BW_UPDTime;
	uint32 PHY_UP_CompTime;
/* Only PHY Downgrade */
	uint32 PHY_DN_SeqCnt;
	uint32 PHY_DN_start_time;
	uint32 PHY_DN_PM1_time;
	uint32 PHY_DN_BW_UPDTime;
	uint32 PHY_DN_CompTime;
/* Action Frame sending only  */
	uint32 ACTFrame_SeqCnt;
	uint32 ACTFrame_start;
	uint32 ACTFrame_complete;
} modesw_time_calc_t;

#define chspec_to_rspec(chspec_bw)	\
		(((uint32)(((((uint16)(chspec_bw)) & WL_CHANSPEC_BW_MASK) >> \
			WL_CHANSPEC_BW_SHIFT) - 1)) << RSPEC_BW_SHIFT)

/* Call back registration */
typedef void (*wlc_modesw_notif_cb_fn_t)(void *ctx, wlc_modesw_notif_cb_data_t *notif_data);
extern int wlc_modesw_notif_cb_register(wlc_modesw_info_t *modeswinfo,
	wlc_modesw_notif_cb_fn_t cb, void *arg);
extern int wlc_modesw_notif_cb_unregister(wlc_modesw_info_t *modeswinfo,
	wlc_modesw_notif_cb_fn_t cb, void *arg);
extern wlc_modesw_info_t * wlc_modesw_attach(wlc_info_t *wlc);
extern void wlc_modesw_detach(wlc_modesw_info_t *modesw_info);

extern void wlc_modesw_set_max_chanspec(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg,
	chanspec_t chanspec);

extern bool wlc_modesw_is_req_valid(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);

extern void wlc_modesw_bss_tbtt(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);

extern void wlc_modesw_pm_pending_complete(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);

extern chanspec_t wlc_modesw_ht_chanspec_override(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg,
	chanspec_t beacon_chanspec);

extern uint8 wlc_modesw_derive_opermode(wlc_modesw_info_t *modesw_info,
	chanspec_t chanspec, wlc_bsscfg_t *bsscfg, uint8 rxstreams);

extern bool wlc_modesw_is_downgrade(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 oper_mode_old,
	uint8 oper_mode_new);

extern uint16 wlc_modesw_get_bw_from_opermode(uint8 oper_mode, vht_op_chan_width_t width);

extern int wlc_modesw_handle_oper_mode_notif_request(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *bsscfg, uint8 oper_mode, uint8 enabled, uint32 ctrl_flags);

extern bool wlc_modesw_is_obss_active(wlc_modesw_info_t *modesw_info);

extern void wlc_modesw_clear_context(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);

extern int wlc_modesw_bw_switch(wlc_modesw_info_t *modesw_info,	chanspec_t chanspec,
	uint8 switch_type,	wlc_bsscfg_t *cfg, uint32 ctrl_flags);
extern bool wlc_modesw_is_connection_vht(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
extern bool wlc_modesw_is_connection_ht(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
extern bool wlc_modesw_opmode_ie_reqd(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *bsscfg);
extern void wlc_modesw_bsscfg_complete_downgrade(wlc_modesw_info_t *modesw_info);
extern void wlc_modesw_resume_opmode_change(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg);
extern void wlc_modesw_dynbw_tx_bw_override(wlc_modesw_info_t *modesw_info,
	wlc_bsscfg_t *bsscfg, uint32 *rspec_bw);
extern bool wlc_modesw_pending_check(wlc_modesw_info_t *modesw_info);
extern bool wlc_modesw_in_progress(wlc_modesw_info_t *modesw_info);
extern uint8 wlc_modesw_get_sample_dur(wlc_modesw_info_t* modesw);
extern void wlc_modesw_set_sample_dur(wlc_modesw_info_t* modesw, uint8 setval);
extern bool wlc_modesw_get_last_load_req_if(wlc_modesw_info_t* modesw);
extern void wlc_modesw_set_last_load_req_if(wlc_modesw_info_t* modesw, bool setval);
extern void wlc_modesw_change_state(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t *cfg,
	int8 new_state);
extern modesw_type wlc_modesw_get_switch_type(wlc_modesw_info_t *modesw);
extern void wlc_modesw_set_switch_type(wlc_modesw_info_t *modesw, modesw_type type);
#ifdef WLRSDB
extern void wlc_modesw_move_cfgs_for_mimo(wlc_modesw_info_t *modesw_info);
#endif
#ifdef WLMCHAN
extern bool wlc_modesw_mchan_hw_switch_pending(wlc_info_t * wlc, wlc_bsscfg_t * bsscfg,
	bool hw_only);
extern bool wlc_modesw_mchan_hw_switch_complete(wlc_info_t * wlc, wlc_bsscfg_t * bsscfg);
extern void wlc_modesw_mchan_switch_process(wlc_modesw_info_t *modesw_info, wlc_bsscfg_t * bsscfg);
#endif
#endif  /* _WLC_MODESW_H_ */
