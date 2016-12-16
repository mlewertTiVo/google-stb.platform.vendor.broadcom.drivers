/*
 * Radio 20695 channel tuning header file
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
 * $Id: wlc_phytbl_20695.h 650058 2016-07-20 09:50:45Z $
 */

#ifndef _wlc_phytbl_20695_h_
#define _wlc_phytbl_20695_h_
#include "phy_ac_rxgcrs.h"

#define NROWS_CHAN_TUNE_20695 (78)

typedef struct _chan_info_radio20695_rffe {
	uint8 channel;
	uint16 freq;
	uint8 vco_buf; /* This is applicable for 2G only */
	uint8 lo_gen;
	uint8 rx_lna;
	uint8 tx_mix;
	uint8 tx_pad;
} chan_info_radio20695_rffe_t;

#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && defined(DBG_PHY_IOV)) || \
	defined(BCMDBG_PHYDUMP)
extern const radio_20xx_dumpregs_t dumpregs_20695_rev39[];
#endif 

extern radio_20xx_prefregs_t prefregs_20695_rev32[];
extern radio_20xx_prefregs_t prefregs_20695_rev33[];
extern radio_20xx_prefregs_t prefregs_20695_rev37[];
extern radio_20xx_prefregs_t prefregs_20695_rev38[];
extern radio_20xx_prefregs_t prefregs_20695_rev39[];
extern radio_20xx_prefregs_t prefregs_20695_rev40[];
extern radio_20xx_prefregs_t prefregs_20695_rev41[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev32_fcbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev32_wlbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev33)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev37)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev38_fcbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev38_wlbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev39_fcbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev39_wlbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev40_fcbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev40_wlbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev41_fcbga)[];
extern chan_info_radio20695_rffe_t BCMATTACHDATA(chan_tune_20695_rev41_wlbga)[];

extern int8 BCMATTACHDATA(lna12_gain_tbl_2g_maj36)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_5g_maj36)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gainbits_tbl_2g_maj36)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gainbits_tbl_5g_maj36)[2][N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_2g_maj36)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_5g_maj36)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_2g_maj36)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_5g_maj36)[N_LNA12_GAINS];
extern int8 BCMATTACHDATA(gainlimit_tbl_maj36)[RXGAIN_CONF_ELEMENTS][MAX_RX_GAINS_PER_ELEM];
extern int8 BCMATTACHDATA(tia_gain_tbl_maj36)[N_TIA_GAINS];
extern int8 BCMATTACHDATA(tia_gainbits_tbl_maj36)[N_TIA_GAINS];
extern int8 BCMATTACHDATA(biq01_gain_tbl_maj36)[2][N_BIQ01_GAINS];
extern int8 BCMATTACHDATA(biq01_gainbits_tbl_maj36)[2][N_BIQ01_GAINS];
extern uint8 BCMATTACHDATA(lna2_gain_map_2g_maj36)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna2_gain_map_5g_maj36)[N_LNA12_GAINS];
extern int16 BCMATTACHDATA(maxgain_maj36)[ACPHY_MAX_RX_GAIN_STAGES];
extern int8 BCMATTACHDATA(fast_agc_clip_gains_maj36)[4];
extern int8 BCMATTACHDATA(fast_agc_lonf_clip_gains_maj36)[4];

extern int8 BCMATTACHDATA(ssagc_clip_gains_maj36)[SSAGC_CLIP1_GAINS_CNT];
extern bool BCMATTACHDATA(ssagc_lna1byp_maj36)[SSAGC_CLIP1_GAINS_CNT];
extern uint8 BCMATTACHDATA(ssagc_clip2_tbl_maj36)[SSAGC_CLIP2_TBL_IDX_CNT];
extern uint8 BCMATTACHDATA(ssagc_rssi_thresh_2g_maj36)[SSAGC_N_RSSI_THRESH];
extern uint8 BCMATTACHDATA(ssagc_clipcnt_thresh_2g_maj36)[SSAGC_N_CLIPCNT_THRESH];
extern uint8 BCMATTACHDATA(ssagc_rssi_thresh_5g_maj36)[SSAGC_N_RSSI_THRESH];
extern uint8 BCMATTACHDATA(ssagc_clipcnt_thresh_5g_maj36)[SSAGC_N_CLIPCNT_THRESH];

extern uint16 BCMATTACHDATA(rx2nap_seq_maj36)[16];
extern uint16 BCMATTACHDATA(rx2nap_seq_dly_maj36)[16];
extern uint16 BCMATTACHDATA(nap2rx_seq_maj36)[16];
extern uint16 BCMATTACHDATA(nap2rx_seq_dly_maj36)[16];

extern uint16 BCMATTACHDATA(nap_lo_th_adj_maj36)[5];
extern uint16 BCMATTACHDATA(nap_hi_th_adj_maj36)[5];
#endif	/* _WLC_PHYTBL_20695_H_ */
