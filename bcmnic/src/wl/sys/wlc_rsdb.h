/*
 * WLC RSDB API definition
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
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_rsdb.h 664685 2016-10-13 11:34:52Z $
 */


#ifndef _wlc_rsdb_h_
#define _wlc_rsdb_h_
#ifdef WLRSDB
typedef enum {
	WLC_RSDB_MODE_AUTO = AUTO,
	WLC_RSDB_MODE_2X2,
	WLC_RSDB_MODE_RSDB,
	WLC_RSDB_MODE_80P80,
	WLC_RSDB_MODE_MAX
} wlc_rsdb_modes_t;

typedef enum {
	WLC_INFRA_MODE_AUTO = AUTO,
	WLC_INFRA_MODE_MAIN,
	WLC_INFRA_MODE_AUX
} wlc_infra_modes_t;

/* Indicates the reason for calling callback of rsdb_clone_timer in mode switch. */
typedef enum {
	WLC_MODESW_LIST_START = 0,
	WLC_MODESW_CLONE_TIMER = 1, /* Timer callback should handle cloning */
	WLC_MODESW_UPGRADE_TIMER = 2, /* Timer callback should handle R->V upgrade */
	WLC_MODESW_LIST_END
} wlc_modesw_cb_type;


#define CFG_CLONE_START		0x01
#define CFG_CLONE_END		0x02

typedef struct rsdb_cfg_clone_upd_data
{
	wlc_bsscfg_t *cfg;
	wlc_info_t *from_wlc;
	wlc_info_t *to_wlc;
	uint	type;
} rsdb_cfg_clone_upd_t;

typedef void (*rsdb_cfg_clone_fn_t)(void *ctx, rsdb_cfg_clone_upd_t *evt_data);
extern int wlc_rsdb_cfg_clone_register(wlc_rsdb_info_t *rsdb,
	rsdb_cfg_clone_fn_t fn, void *arg);
extern int wlc_rsdb_cfg_clone_unregister(wlc_rsdb_info_t *rsdb,
	rsdb_cfg_clone_fn_t fn, void *arg);
extern void wlc_rsdb_suppress_pending_tx_pkts(wlc_bsscfg_t *from_cfg, wlc_bsscfg_t *to_cfg);

#define MODE_RSDB 0
#define MODE_VSDB 1
#define WL_RSDB_VSDB_CLONE_WAIT 2
#define WL_RSDB_ZERO_DELAY 0
#define WL_RSDB_UPGRADE_TIMER_DELAY 2

/* Flags to restrict wlc selection */
#define WL_RSDB_CORE_MATCH_BAND		0x1
#define WL_RSDB_CORE_MATCH_BW		0x2
#define WL_RSDB_CORE_MATCH_CHAINS	0x4

/* RSDB Auto mode override flags */
#define WLC_RSDB_AUTO_OVERRIDE_RATE			0x01
#define WLC_RSDB_AUTO_OVERRIDE_SCAN			0x02
#define WLC_RSDB_AUTO_OVERRIDE_USER			0x04
#define WLC_RSDB_AUTO_OVERRIDE_MFGTEST			0x08
#define WLC_RSDB_AUTO_OVERRIDE_BANDLOCK			0x10
#define WLC_RSDB_AUTO_OVERRIDE_P2PSCAN			0x20
#define WLC_RSDB_AUTO_OVERRIDE_ULB			0x40

/* Default primary cfg index for Core 1 */
#define RSDB_INACTIVE_CFG_IDX	1

#define WLC_RSDB_GET_PRIMARY_WLC(curr_wlc)	((curr_wlc)->cmn->wlc[0])

#ifdef WL_DUALMAC_RSDB
/* In case of asymmetric dual mac chip only RSDB mode is supported */
#define WLC_RSDB_DUAL_MAC_MODE(mode)	(1)
#define WLC_RSDB_SINGLE_MAC_MODE(mode)	(0)
#else
#define WLC_RSDB_DUAL_MAC_MODE(mode)	((mode) == WLC_RSDB_MODE_RSDB)
#define WLC_RSDB_SINGLE_MAC_MODE(mode)	(((mode) == WLC_RSDB_MODE_2X2) ||	\
	((mode) == WLC_RSDB_MODE_80P80))
#endif

#define WLC_RSDB_IS_AUTO_MODE(wlc_any) (wlc_any->cmn->rsdb_mode & WLC_RSDB_MODE_AUTO_MASK)
#define WLC_RSDB_IS_AP_AUTO_MODE(wlc_any) (wlc_any->cmn->ap_rsdb_mode == AUTO)

#define SCB_MOVE /* Added flag temporarily to enable scb move during bsscfg clone */
#define WLC_RSDB_CURR_MODE(wlc) WLC_RSDB_EXTRACT_MODE((wlc)->cmn->rsdb_mode)
#define DUALMAC_RSDB_LOAD_THRESH	70
enum rsdb_move_chains {
	RSDB_MOVE_DYN = -1,
	RSDB_MOVE_CHAIN0 = 0,
	RSDB_MOVE_CHAIN1
};
extern int wlc_rsdb_assoc_mode_change(wlc_bsscfg_t **cfg, wlc_bss_info_t *bi);
typedef int (*rsdb_assoc_mode_change_cb_t)(void *ctx, wlc_bsscfg_t **pcfg, wlc_bss_info_t *bi);
typedef int (*rsdb_get_wlcs_cb_t)(void *ctx, wlc_info_t *wlc, wlc_info_t **wlc_2g,
	wlc_info_t **wlc_5g);
extern void wlc_rsdb_register_get_wlcs_cb(wlc_rsdb_info_t *rsdbinfo,
	rsdb_get_wlcs_cb_t cb, void *ctx);
extern void wlc_rsdb_register_mode_change_cb(wlc_rsdb_info_t *rsdbinfo,
        rsdb_assoc_mode_change_cb_t cb, void *ctx);
extern int wlc_rsdb_change_mode(wlc_info_t *wlc, int8 to_mode);
extern uint8 wlc_rsdb_association_count(wlc_info_t* wlc);
#ifdef WL_MODESW
extern uint8 wlc_rsdb_downgrade_wlc(wlc_info_t *wlc);
extern uint8 wlc_rsdb_upgrade_wlc(wlc_info_t *wlc);
extern bool wlc_rsdb_upgrade_allowed(wlc_info_t *wlc);
extern int wlc_rsdb_check_upgrade(wlc_info_t *wlc);
extern int wlc_rsdb_dyn_switch(wlc_info_t *wlc, bool mode);
#endif
extern int wlc_rsdb_handle_sta_csa(wlc_rsdb_info_t* rsdbinfo, wlc_bsscfg_t *bsscfg,
	chanspec_t chanspec);
bool wlc_rsdb_get_mimo_cap(wlc_bss_info_t *bi);
extern int wlc_rsdb_ap_bringup(wlc_rsdb_info_t* rsdbinfo, wlc_bsscfg_t** cfg);
extern int wlc_rsdb_ibss_bringup(wlc_info_t* wlc, wlc_bsscfg_t** cfg);
int wlc_rsdb_get_wlcs(wlc_info_t *wlc, wlc_info_t **wlc_2g, wlc_info_t **wlc_5g);
wlc_info_t * wlc_rsdb_get_other_wlc(wlc_info_t *wlc);
int wlc_rsdb_any_wlc_associated(wlc_info_t *wlc);
wlc_info_t* wlc_rsdb_find_wlc_for_chanspec(wlc_info_t *wlc, wlc_bss_info_t *bi,
	chanspec_t chanspec, wlc_info_t *skip_wlc, uint32 match_mask);
wlc_info_t* wlc_rsdb_find_wlc_for_band(wlc_rsdb_info_t *rsdbinfo, uint bandunit,
	wlc_info_t *skip_wlc);
uint8 wlc_rsdb_num_wlc_for_band(wlc_rsdb_info_t *rsdbinfo, uint bandunit);
wlc_bsscfg_t* wlc_rsdb_cfg_for_chanspec(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_bss_info_t *bi);
int wlc_rsdb_auto_get_wlcs(wlc_info_t *wlc, wlc_info_t **wlc_2g, wlc_info_t **wlc_5g);
void wlc_rsdb_update_wlcif(wlc_info_t *wlc, wlc_bsscfg_t *from, wlc_bsscfg_t *to);
int wlc_rsdb_join_prep_wlc(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 *SSID, int len,
	wl_join_scan_params_t *scan_params,
	wl_join_assoc_params_t *assoc_params, int assoc_params_len);
wlc_bsscfg_t* wlc_rsdb_bsscfg_clone(wlc_info_t *from_wlc, wlc_info_t *to_wlc,
	wlc_bsscfg_t *cfg, int *ret);
wlc_rsdb_info_t* wlc_rsdb_attach(wlc_info_t* wlc);
void wlc_rsdb_detach(wlc_rsdb_info_t* rsdbinfo);
bool wlc_rsdb_update_active(wlc_info_t *wlc, bool *old_state);
extern uint16 wlc_rsdb_mode(void *hdl);
bool wlc_rsdb_chkiovar(wlc_info_t *wlc, const bcm_iovar_t  *vi_ptr, uint32 actid, int32 wlc_indx);
bool wlc_rsdb_is_other_chain_idle(void *hdl);
void wlc_rsdb_force_rsdb_mode(wlc_info_t *wlc);
bool wlc_rsdb_reinit(wlc_info_t *wlc);
void wlc_rsdb_chan_switch_dump(wlc_info_t *wlc, chanspec_t chanspec, uint32 dwell_time);
#if defined(WL_NAN)
int wlc_rsdb_bsscfg_create(wlc_rsdb_info_t *rsdbinfo, uint32 flags,
	wlc_bsscfg_type_t *type, struct ether_addr *mac_addr,
	wlc_bsscfg_t **cfg_arr, int *if_idx);
wlc_bsscfg_t *wlc_rsdb_bsscfg_get_linked_cfg(wlc_rsdb_info_t *rsdbinfo, wlc_bsscfg_t *cfg);
int wlc_rsdb_bsscfg_delete(wlc_rsdb_info_t *rsdbinfo, wlc_bsscfg_t *bsscfg);
int wlc_rsdb_bsscfg_init(wlc_rsdb_info_t *rsdbinfo, wlc_bsscfg_t *bsscfg);
int wlc_rsdb_bsscfg_deinit(wlc_rsdb_info_t *rsdbinfo, wlc_bsscfg_t *bsscfg);
int wlc_rsdb_bsscfg_enable(wlc_rsdb_info_t *rsdbinfo, wlc_bsscfg_t *bsscfg);
int wlc_rsdb_bsscfg_disable(wlc_rsdb_info_t *rsdbinfo, wlc_bsscfg_t *bsscfg);
#endif 
#else /* WLRSDB */
#define wlc_rsdb_mode(hdl)			(PHYMODE_MIMO)
#define wlc_rsdb_any_wlc_associated(wlc)	((wlc)->pub->associated)
#define WLC_RSDB_GET_PRIMARY_WLC(wlc) (wlc)
#endif /* WLRSDB */

#define WLC_DUALMAC_RSDB(cmn)	(cmn->dualmac_rsdb)
#define WLC_DUALMAC_RSDB_WRAP(dm_rsdb) (dm_rsdb)

#if defined(WLRSDB) && !defined(WL_DUALNIC_RSDB)&& !defined(WL_DUALMAC_RSDB)
void wlc_rsdb_bmc_smac_template(void *wlc, int tplbuf, uint32 bufsize);
extern void wlc_rsdb_set_phymode(void *hdl, uint32 phymode);
#else
#define wlc_rsdb_bmc_smac_template(hdl, tplbuf, bufsize)  do {} while (0)
#define wlc_rsdb_set_phymode(a, b) do {} while (0)
#endif /* defined(WLRSDB) && !defined(WL_DUALNIC_RSDB) */

extern bool wlc_rsdb_up_allowed(wlc_info_t *wlc);
extern void wlc_rsdb_init_max_rateset(wlc_info_t *wlc, wlc_bss_info_t *bi);
extern int wlc_rsdb_wlc_cmn_attach(wlc_info_t *wlc);
extern void wlc_rsdb_cmn_detach(wlc_info_t *wlc);
extern void wlc_rsdb_config_auto_mode_switch(wlc_info_t *wlc, uint32 reason, uint32 override);
extern int wlc_rsdb_move_connection(wlc_bsscfg_t *bscfg, int8 chain, bool dynswitch);
extern void wlc_rsdb_pm_move_override(wlc_info_t *wlc, bool ovr);
extern void wlc_rsdb_suppress_pending_tx_pkts(wlc_bsscfg_t *from_cfg, wlc_bsscfg_t *to_cfg);
extern wlc_bsscfg_t * wlc_rsdb_get_infra_cfg(wlc_info_t *wlc);
extern bool wlc_rsdb_change_band_allowed(wlc_info_t *wlc, int bandunit);
extern uint8 wlc_rsdb_get_cmn_bwcap(wlc_info_t *wlc, int bandtype);
extern int wlc_rsdb_scb_clone(wlc_bsscfg_t * from_cfg, wlc_bsscfg_t * to_cfg);
extern uint8 wlc_rsdb_any_aps_active(wlc_info_t* wlc);
extern uint8 wlc_rsdb_any_go_active(wlc_info_t* wlc);
int wlc_rsdb_move_scb(wlc_bsscfg_t * from_cfg, wlc_bsscfg_t * to_cfg,
		struct scb *scb, bool suppress);
int wlc_rsdb_switch_if_to_linked_cfg(wlc_rsdb_info_t *rsdbinfo, wlc_bsscfg_t *to_cfg);
#endif /* _wlc_rsdb_h_ */
