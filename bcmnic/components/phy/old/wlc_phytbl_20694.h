/*
 * Radio 20694 channel tuning header file
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
 * $Id: wlc_phytbl_20694.h 657402 2016-09-01 04:42:53Z $
 */

#ifndef  _wlc_phytbl_20694_h_
#define  _wlc_phytbl_20694_h_
#include "phy_ac_rxgcrs.h"

#define NUM_ROWS_CHAN_TUNING_20694 (101)

typedef struct _chan_info_radio20694 {
	uint8 channel;
	uint8 vco_buf;
	uint8 lo_gen;
	uint8 rx_lna;
	uint8 tx_mix;
	uint8 tx_pad;
	uint8 rsvd1;
	uint8 rsvd2;
	uint8 rsvd3;
	uint8 rsvd4;
	uint8 rsvd5;
	uint8 rsvd6;
	uint8 rsvd7;
	uint8 rsvd8;
	uint16 freq;
} chan_info_radio20694_t;

typedef struct _chan_info_radio20694_rffe {
uint8 channel;
uint16 freq;
uint8 vco_buf;
uint16 RF0_logen2g_reg2_logen2g_x2_ctune_2G;
uint16 RF0_logen2g_reg2_logen2g_buf_ctune_2G;
uint16 RF0_lna2g_reg1_lna2g_lna1_freq_tune_2G;
uint16 RF0_txmix2g_cfg5_mx2g_tune_2G;
uint16 RF0_pa2g_cfg2_pad2g_tune_2G;
uint16 RF1_logen2g_reg2_logen2g_x2_ctune_2G;
uint16 RF1_logen2g_reg2_logen2g_buf_ctune_2G;
uint16 pa5g_cfg4; /* Unused. Keep for ROM compatibility. */
uint16 RF1_lna2g_reg1_lna2g_lna1_freq_tune_2G;
uint16 RF1_txmix2g_cfg5_mx2g_tune_2G;
uint16 RF1_pa2g_cfg2_pad2g_tune_2G;
uint16 RF0_logen2g_reg4_logen2g_lobuf2g2_ctune_2G_AUX;
uint16 RF0_logen5g_reg5_logen5g_x3_ctune_5G;
uint16 RF0_logen5g_reg8_logen5g_buf_ctune_5G;
uint16 RF0_logen2g_reg5_logen5g_mimobuf_ctune_5G;
uint16 RF0_rx5g_reg1_rx5g_lna_tune_5G;
uint16 RF0_txmix5g_cfg6_mx5g_tune_5G;
uint16 RF0_pa5g_cfg4_pad5g_tune_5G;
uint16 RF0_pa5g_cfg4_pa5g_tune_5G;
uint16 RF1_logen5g_reg5_logen5g_x3_ctune_5G;
uint16 RF1_logen5g_reg8_logen5g_buf_ctune_5G;
uint16 RF1_logen2g_reg5_logen5g_mimobuf_ctune_5G;
uint16 RF1_rx5g_reg1_rx5g_lna_tune_5G;
uint16 RF1_txmix5g_cfg6_mx5g_tune_5G;
uint16 RF1_pa5g_cfg4_pad5g_tune_5G;
uint16 RF1_pa5g_cfg4_pa5g_tune_5G;
uint16 RF0_logen5g_reg6_logen5g_buf_ctune_c0_5G_AUX;
uint16 RF0_logen5g_reg6_logen5g_buf_ctune_c1_5G_AUX;
} chan_info_radio20694_rffe_t;

#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && defined(DBG_PHY_IOV)) || \
	defined(BCMDBG_PHYDUMP)
extern const radio_20xx_dumpregs_t dumpregs_20694_rev36[];
extern const radio_20xx_dumpregs_t dumpregs_20694_rev5[];
extern const radio_20xx_dumpregs_t dumpregs_20694_rev8[];
extern const radio_20xx_dumpregs_t dumpregs_20694_rev9[];
#endif 

extern const chan_info_radio20694_rffe_t
    chan_tune_20694_rev5_fcbu_e_MAIN[NUM_ROWS_CHAN_TUNING_20694];
extern const chan_info_radio20694_rffe_t
    chan_tune_20694_rev5_fcbu_e_AUX[NUM_ROWS_CHAN_TUNING_20694];
extern const chan_info_radio20694_rffe_t
    chan_tune_20694_rev8_MAIN[NUM_ROWS_CHAN_TUNING_20694];
extern const chan_info_radio20694_rffe_t
    chan_tune_20694_rev9_AUX[NUM_ROWS_CHAN_TUNING_20694];

extern const chan_info_radio20694_t chan_tune_20694_rev361[NUM_ROWS_CHAN_TUNING_20694];
extern radio_20xx_prefregs_t prefregs_20694_rev36_wlbga[];
extern radio_20xx_prefregs_t prefregs_20694_rev5_main[];
extern radio_20xx_prefregs_t prefregs_20694_rev5_aux[];
extern const chan_info_radio20694_rffe_t chan_tune_20694_rev36_wlbga[NUM_ROWS_CHAN_TUNING_20694];
extern radio_20xx_prefregs_t prefregs_20694_rev8_main[];
extern radio_20xx_prefregs_t prefregs_20694_rev9_aux[];

extern int8 BCMATTACHDATA(lna12_gain_tbl_2g_maj40)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_5g_maj40)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gainbits_tbl_2g_maj40)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gainbits_tbl_5g_maj40)[2][N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_2g_maj40)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_5g_maj40)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_2g_maj40)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_5g_maj40)[N_LNA12_GAINS];
extern int8 BCMATTACHDATA(gainlimit_tbl_maj40)[RXGAIN_CONF_ELEMENTS][MAX_RX_GAINS_PER_ELEM];
extern int8 BCMATTACHDATA(tia_gain_tbl_maj40)[N_TIA_GAINS];
extern int8 BCMATTACHDATA(tia_gainbits_tbl_maj40)[N_TIA_GAINS];
extern int8 BCMATTACHDATA(biq01_gain_tbl_maj40)[2][N_BIQ01_GAINS];
extern int8 BCMATTACHDATA(biq01_gainbits_tbl_maj40)[2][N_BIQ01_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_2g_maj40_min1)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_5g_maj40_min1)[2][N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_2g_maj40_min1)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_5g_maj40_min1)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_2g_maj40_min1)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_5g_maj40_min1)[N_LNA12_GAINS];

extern uint16 BCMATTACHDATA(nap_lo_th_adj_maj40)[5];
extern uint16 BCMATTACHDATA(nap_hi_th_adj_maj40)[5];
#endif /* _WLC_PHYTBL_20694_H_ */
