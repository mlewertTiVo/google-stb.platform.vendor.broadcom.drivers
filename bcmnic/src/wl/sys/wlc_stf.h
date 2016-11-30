/*
 * Code that controls the antenna/core/chain
 * Broadcom 802.11bang Networking Device Driver
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
 * $Id: wlc_stf.h 638403 2016-05-17 19:01:20Z $
 */

#ifndef _wlc_stf_h_
#define _wlc_stf_h_

#ifdef WL_STF_ARBITRATOR
#include <wlc_stf_arb.h>
#endif
#ifdef WL_MIMOPS_CFG
#include <wlc_stf_mimops.h>
#endif
#include <wlc_types.h>

#define AUTO_SPATIAL_EXPANSION	-1
#define MIN_SPATIAL_EXPANSION	0
#define MAX_SPATIAL_EXPANSION	1

#define PWRTHROTTLE_CHAIN	1
#define PWRTHROTTLE_DUTY_CYCLE	2
#define OCL_RSSI_DELTA				5	/* hysteresis rssi delta */

enum txcore_index {
	CCK_IDX = 0,	/* CCK txcore index */
	OFDM_IDX,
	NSTS1_IDX,
	NSTS2_IDX,
	NSTS3_IDX,
	NSTS4_IDX,
	MAX_CORE_IDX
};

#define ONE_CHAIN_CORE0             0x1
#define ONE_CHAIN_CORE1             0x2

#define WLC_TXCHAIN_ID_USR		0 /* user setting */
#define WLC_TXCHAIN_ID_TEMPSENSE	1 /* chain mask for temperature sensor */
#define WLC_TXCHAIN_ID_PWRTHROTTLE	2 /* chain mask for pwr throttle feature for 43224 board */
#define WLC_TXCHAIN_ID_PWR_LIMIT	3 /* chain mask for tx power offsets */
#define WLC_TXCHAIN_ID_BTCOEX		4 /* chain mask for BT coex */
#define WLC_TXCHAIN_ID_MWS		5 /* chain mask for mws */
#define WLC_TXCHAIN_ID_COUNT		6 /* number of txchain_subval masks */

#define MWS_ANTMAP_DEFAULT	0
#define WL_STF_CONFIG_CHAINS_DISABLED -1
#define WL_STF_CHAINS_NOT_CONFIGURED -1

/* Defines for request entry states; use only defines once */
#define WLC_STF_ARBITRATOR_REQ_STATE_RXTX_INACTIVE  0x00
#define WLC_STF_ARBITRATOR_REQ_STATE_RX_ACTIVE      0x01
#define WLC_STF_ARBITRATOR_REQ_STATE_TX_ACTIVE      0x02
#define WLC_STF_ARBITRATOR_REQ_STATE_RXTX_ACTIVE    0x03

struct wlc_stf_arb_mps_info {
	wlc_info_t  *wlc;
	int cfgh_hwcfg;
	int cfgh_arb;
	int cfgh_mps;
};

/* anything affects the single/dual streams/antenna operation */
struct wlc_stf {
	uint8	hw_txchain;		/**< HW txchain bitmap cfg */
	uint8	txchain;		/**< txchain bitmap being used */
	uint8	txstreams;		/**< number of txchains being used */

	uint8	hw_rxchain;		/**< HW rxchain bitmap cfg */
	uint8	op_rxstreams;		/**< number of operating rxchains being used */
	uint8	rxchain;		/**< rxchain bitmap being used */
	uint8	rxstreams;		/**< number of rxchains being used */

	uint8 	ant_rx_ovr;		/**< rx antenna override */
	int8	txant;			/**< userTx antenna setting */
	uint16	phytxant;		/**< phyTx antenna setting in txheader */

	uint8	ss_opmode;		/**< singlestream Operational mode, 0:siso; 1:cdd */
	bool	ss_algosel_auto;	/**< if TRUE, use wlc->stf->ss_algo_channel; */
					/* else use wlc->band->stf->ss_mode_band; */
	uint16	ss_algo_channel; /**< ss based on per-channel algo: 0: SISO, 1: CDD 2: STBC */
	uint8   no_cddstbc;		/**< stf override, 1: no CDD (or STBC) allowed */

	uint8   rxchain_restore_delay;	/**< delay time to restore default rxchain */

	int8	ldpc;			/**< ON/OFF ldpc RX supported */
	int8	ldpc_tx;		/**< AUTO/ON/OFF ldpc TX supported */
	uint8	txcore_idx;		/**< bitmap of selected core for each Nsts */
	uint8	txcore[MAX_CORE_IDX][2];	/**< bitmap of selected core for each Nsts */
	uint8	txcore_override[MAX_CORE_IDX];	/**< override of txcore for each Nsts */
	int8	spatialpolicy;
	int8	spatialpolicy_pending;
	uint16	shmem_base;
	ppr_t	*txpwr_ctl;

	bool	tempsense_disable;	/**< disable periodic tempsense check */
	uint	tempsense_period;	/**< period to poll tempsense */
	uint 	tempsense_lasttime;
	uint8   siso_tx;		/**< use SISO and limit rates to MSC 0-7 */

	uint	ipaon;			/**< IPA on/off, assume it's the same for 2g and 5g */
	int	pwr_throttle;		/**< enable the feature */
	uint8	throttle_state;		/**< hwpwr/temp throttle state */
	uint8   tx_duty_cycle_pwr;	/**< maximum allowed duty cycle under pwr throttle */
	uint16	tx_duty_cycle_ofdm;	/**< maximum allowed duty cycle for OFDM */
	uint16	tx_duty_cycle_cck;	/**< maximum allowed duty cycle for CCK */
	uint8	pwr_throttle_mask;	/**< core to enable/disable with thermal throttle kick in */
	uint8	pwr_throttle_test;	/**< for testing */
	uint8	txchain_pending;	/**< pending value of txchain */
	bool	rssi_pwrdn_disable;	/**< disable rssi power down feature */
	uint8	txchain_subval[WLC_TXCHAIN_ID_COUNT];	/**< set of txchain enable masks */
	int8	spatial_mode_config[SPATIAL_MODE_MAX_IDX]; /**< setting for each band or sub-band */
	wl_txchain_pwr_offsets_t txchain_pwr_offsets;
	int8 	onechain;		/**< disable 1 TX/RS */
	uint8	pwrthrottle_config;
	uint32	pwrthrottle_pin;	/**< contain the bit map of pin use for pwr throttle */
	uint16	shm_rt_txpwroff_pos;

	uint16	tx_duty_cycle_ofdm_40_5g;	/**< max allowed duty cycle for 5g 40Mhz OFDM */
	uint16	tx_duty_cycle_thresh_40_5g;	/* max rate in 500K to apply 5g 40Mhz duty cycle */
	uint16	tx_duty_cycle_ofdm_80_5g;	/**< max allowed duty cycle for 5g 40Mhz OFDM */
	uint16	tx_duty_cycle_thresh_80_5g;	/* max rate in 500K to apply 5g 40Mhz duty cycle */
	ppr_t	*txpwr_ctl_qdbm;

	/* used by OLPC to register for callback if stored stf state */
	/* changes during phy calibration */
	wlc_stf_txchain_evt_notify txchain_perrate_state_modify;
	uint8	max_offset;
	/* Country code allows txbf */
	uint8	allow_txbf;
	uint8	tx_duty_cycle_thermal;	/**< maximum allowed duty cycle under thermal throttle */
	uint8   txstream_value;
	uint8	hw_txchain_cap;	/**< copy of the nvram for reuse while upgrade/downgrade */
	uint8	hw_rxchain_cap;	/**< copy of the nvram for reuse while upgrade/downgrade */
	uint8	coremask_override;
	uint8	channel_bonding_cores; /**< 80p80/160MHz runs using 2 PHY cores, not tx/rx-chain */

	/* previous override of txcore for each Nsts */
	uint8   prev_txcore_override[MAX_CORE_IDX];

	int8    nap_rssi_delta;
	int8    nap_rssi_threshold;
	uint8	new_rxchain;
	uint8	mimops_send_cnt;
	uint8	mimops_acked_cnt;
	int32	scb_handle;
	bool    ocl_en;
	int8    ocl_rssi_delta;
	int8    ocl_rssi_threshold;
	wlc_stf_arb_t    *arb;
	wlc_stf_nss_request_q_t    *arb_req_q;
	uint8   bw_update_in_progress;
	chanspec_t    pending_bw;
	wlc_stf_arb_mps_info_t    *arb_mps_info_hndl;
};

extern int wlc_stf_attach(wlc_info_t* wlc);
extern void wlc_stf_detach(wlc_info_t* wlc);
extern void wlc_stf_chain_init(wlc_info_t *wlc);

#ifdef WL11N
extern void wlc_stf_txchain_set_complete(wlc_info_t *wlc);
#ifdef WL11AC
extern void wlc_stf_chanspec_upd(wlc_info_t *wlc);
#endif /* WL11AC */
extern void wlc_stf_tempsense_upd(wlc_info_t *wlc);
extern void wlc_stf_ss_algo_channel_get(wlc_info_t *wlc, uint16 *ss_algo_channel,
	chanspec_t chanspec);
extern void wlc_stf_ss_update(wlc_info_t *wlc, wlcband_t *band);
extern void wlc_stf_phy_txant_upd(wlc_info_t *wlc);
extern int wlc_stf_txchain_set(wlc_info_t* wlc, int32 int_val, bool force, uint16 id);
extern int wlc_stf_txchain_subval_get(wlc_info_t* wlc, uint id, uint *txchain);
extern int wlc_stf_rxchain_set(wlc_info_t* wlc, int32 int_val, bool update);
extern bool wlc_stf_rxchain_ishwdef(wlc_info_t* wlc);
extern bool wlc_stf_txchain_ishwdef(wlc_info_t* wlc);
extern bool wlc_stf_stbc_rx_set(wlc_info_t* wlc, int32 int_val);
extern uint8 wlc_stf_txchain_get(wlc_info_t *wlc, ratespec_t rspec);
extern uint8 wlc_stf_txcore_get(wlc_info_t *wlc, ratespec_t rspec);
extern void wlc_stf_spatialpolicy_set_complete(wlc_info_t *wlc);
extern void wlc_stf_txcore_shmem_write(wlc_info_t *wlc, bool forcewr);
extern uint16 wlc_stf_spatial_expansion_get(wlc_info_t *wlc, ratespec_t rspec);
extern uint8 wlc_stf_get_pwrperrate(wlc_info_t *wlc, ratespec_t rspec,
	uint16 spatial_map);
extern int wlc_stf_get_204080_pwrs(wlc_info_t *wlc, ratespec_t rspec, txpwr204080_t* pwrs,
        wl_tx_chains_t txbf_chains);
extern void wlc_set_pwrthrottle_config(wlc_info_t *wlc);
extern int wlc_stf_duty_cycle_set(wlc_info_t *wlc, int duty_cycle, bool isOFDM, bool writeToShm);
extern void wlc_stf_chain_active_set(wlc_info_t *wlc, uint8 active_chains);
#ifdef RXCHAIN_PWRSAVE
extern uint8 wlc_stf_enter_rxchain_pwrsave(wlc_info_t *wlc);
extern void wlc_stf_exit_rxchain_pwrsave(wlc_info_t *wlc, uint8 ht_cap_rx_stbc);
#endif
#else
#define wlc_stf_spatial_expansion_get(a, b) 0
#define wlc_stf_get_pwrperrate(a, b, c) 0
#define wlc_stf_get_204080_pwrs(a, b, c, d) 0
#define wlc_stf_txchain_set(a, b, c, d) (void)BCME_OK
#define wlc_stf_rxchain_set(a, b, c) (void)BCME_OK
#endif /* WL11N */

extern int wlc_stf_ant_txant_validate(wlc_info_t *wlc, int8 val);
extern void wlc_stf_phy_txant_upd(wlc_info_t *wlc);
extern void wlc_stf_phy_chain_calc(wlc_info_t *wlc);
extern uint16 wlc_stf_phytxchain_sel(wlc_info_t *wlc, ratespec_t rspec);
extern uint16 wlc_stf_d11hdrs_phyctl_txant(wlc_info_t *wlc, ratespec_t rspec);
extern void wlc_stf_wowl_upd(wlc_info_t *wlc);
extern void wlc_stf_shmem_base_upd(wlc_info_t *wlc, uint16 base);
extern void wlc_stf_wowl_spatial_policy_set(wlc_info_t *wlc, int policy);
extern void wlc_update_txppr_offset(wlc_info_t *wlc, ppr_t *txpwr);
extern int wlc_stf_spatial_policy_set(wlc_info_t *wlc, int val);
extern int8 wlc_stf_stbc_tx_get(wlc_info_t* wlc);

typedef uint16 wlc_stf_txchain_st;
extern void wlc_stf_txchain_get_perrate_state(wlc_info_t *wlc, wlc_stf_txchain_st *state,
	wlc_stf_txchain_evt_notify func);
extern void
wlc_stf_txchain_restore_perrate_state(wlc_info_t *wlc, wlc_stf_txchain_st *state);
extern bool wlc_stf_saved_state_is_consistent(wlc_info_t *wlc, wlc_stf_txchain_st *state);
#ifdef WL_BEAMFORMING
extern void wlc_stf_set_txbf(wlc_info_t *wlc, bool enable);
#endif
#if defined(WLTXPWR_CACHE)
extern int wlc_stf_txchain_pwr_offset_set(wlc_info_t *wlc, wl_txchain_pwr_offsets_t *offsets);
#endif
#if defined(WL_EXPORT_CURPOWER)
uint8 get_pwr_from_targets(wlc_info_t *wlc, ratespec_t rspec, ppr_t *txpwr);
#endif
extern int wlc_stf_mws_set(wlc_info_t *wlc, uint32 txantmap, uint32 rxantmap);
#ifdef WLRSDB
extern void wlc_stf_phy_chain_calc_set(wlc_info_t *wlc);
#else
extern void BCMATTACHFN(wlc_stf_phy_chain_calc_set)(wlc_info_t *wlc);
#endif
extern void wlc_stf_reinit_chains(wlc_info_t* wlc);
#ifdef WL_STF_ARBITRATOR
extern void wlc_stf_set_arbitrated_chains_complete(wlc_info_t *wlc);
extern void wlc_stf_arb_req_update(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint8 state);
#else /* stub */
#define wlc_stf_arb_req_update(a, b, c) do {} while (0)
#endif /* WL_STF_ARBITRATOR */

extern int wlc_stf_siso_bcmc_rx(wlc_info_t *wlc,  bool enable);
typedef struct stf_scb_info {
	uint8	mimops_acked;
	uint8	mimops_retry_required;
	uint8	mimops_retry;
} stf_scb_info_t;
#define STF_CUBBY(wlc, scb) SCB_CUBBY((scb), (wlc)->stf->scb_handle)

#define NAP_RSSI_DELTA          3       /* hysteresis rssi delta */


#endif /* _wlc_stf_h_ */
