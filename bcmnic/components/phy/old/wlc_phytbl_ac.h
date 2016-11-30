/*
 * Declarations for Broadcom PHY core tables,
 * Networking Adapter Device Driver.
 *
 * THIS IS A GENERATED FILE - DO NOT EDIT
 * Generated on Tue Aug 16 16:22:54 PDT 2011
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
 * All Rights Reserved.
 *
 * $Id: wlc_phytbl_ac.h 649330 2016-07-15 16:17:13Z mvermeid $
 */
/* FILE-CSTYLED */
#ifndef _wlc_phytbl_ac_h_
#define _wlc_phytbl_ac_h_

#include <phy_ac_info.h>

#define NUM_ROWS_CHAN_TUNING 77
#define NUM_ALTCLKPLN_CHANS 5
#define TXGAIN_TABLES_LEN 384


extern CONST acphytbl_info_t acphytbl_info_rev0[];
extern CONST uint32 acphytbl_info_sz_rev0;
extern CONST acphytbl_info_t acphytbl_info_rev2[];
extern CONST uint32 acphytbl_info_sz_rev2;
extern CONST acphytbl_info_t acphytbl_info_rev6[];
extern CONST uint32 acphytbl_info_sz_rev6;
extern CONST acphytbl_info_t acphytbl_info_rev3[];
extern CONST uint32 acphytbl_info_sz_rev3;
extern CONST acphytbl_info_t acphytbl_info_rev9[];
extern CONST uint32 acphytbl_info_sz_rev9;
extern CONST acphytbl_info_t acphytbl_info_rev12[];
extern CONST uint32 acphytbl_info_sz_rev12;
extern CONST acphytbl_info_t acphytbl_info_rev32[];
extern CONST uint32 acphytbl_info_sz_rev32;
extern CONST acphytbl_info_t acphytbl_info_rev33[];
extern CONST uint32 acphytbl_info_sz_rev33;
extern CONST acphytbl_info_t acphytbl_info_rev36[];
extern CONST uint32 acphytbl_info_sz_rev36;
extern CONST acphytbl_info_t acphytbl_info_rev40[];
extern CONST uint32 acphytbl_info_sz_rev40;
extern CONST acphytbl_info_t acphytbl_info_maxbw20_rev40[];
extern CONST uint32 acphytbl_info_sz_maxbw20_rev40;

extern CONST acphytbl_info_t acphyzerotbl_info_rev0[];
extern CONST uint32 acphyzerotbl_info_cnt_rev0;
extern CONST acphytbl_info_t acphyzerotbl_info_rev2[];
extern CONST uint32 acphyzerotbl_info_cnt_rev2;
extern CONST acphytbl_info_t acphyzerotbl_info_rev6[];
extern CONST uint32 acphyzerotbl_info_cnt_rev6;
extern CONST acphytbl_info_t acphyzerotbl_info_rev3[];
extern CONST uint32 acphyzerotbl_info_cnt_rev3;
extern CONST acphytbl_info_t acphyzerotbl_info_rev9[];
extern CONST uint32 acphyzerotbl_info_cnt_rev9;
extern CONST acphytbl_info_t acphyzerotbl_info_rev12[];
extern CONST uint32 acphyzerotbl_info_cnt_rev12;
extern CONST acphytbl_info_t acphyzerotbl_delta_MIMO_80P80_info_rev12[];
extern CONST uint32 acphyzerotbl_delta_MIMO_80P80_info_cnt_rev12;
extern CONST acphytbl_info_t acphyzerotbl_info_rev32[];
extern CONST uint32 acphyzerotbl_info_cnt_rev32;
extern CONST acphytbl_info_t acphyzerotbl_info_rev36[];
extern CONST uint32 acphyzerotbl_info_cnt_rev36;
extern CONST acphytbl_info_t acphyzerotbl_info_rev40[];
extern CONST uint32 acphyzerotbl_info_cnt_rev40;
extern CONST acphytbl_info_t acphyzerotbl_info_maxbw20_rev40[];
extern CONST uint32 acphyzerotbl_info_cnt_maxbw20_rev40;

extern CONST chan_info_radio2069_t chan_tuning_2069rev3[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069_t chan_tuning_2069rev4[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069_t chan_tuning_2069rev7[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069_t chan_tuning_2069rev64[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069_t chan_tuning_2069rev66[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_16_17[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_16_17_40[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_18[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_18_40[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_GE16_lp[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_23_2Glp_5Gnonlp[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_GE16_2Glp_5Gnonlp[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_GE16_40_lp[NUM_ROWS_CHAN_TUNING];
extern CONST BCMATTACHDATA(chan_info_radio2069revGE32_t) chan_tuning_2069_rev33_37[NUM_ROWS_CHAN_TUNING];
extern CONST BCMATTACHDATA(chan_info_radio2069revGE32_t) chan_tuning_2069_rev33_37_40[NUM_ROWS_CHAN_TUNING];
extern CONST BCMATTACHDATA(chan_info_radio2069revGE32_t) chan_tuning_2069_rev36[NUM_ROWS_CHAN_TUNING];
extern CONST BCMATTACHDATA(chan_info_radio2069revGE32_t) chan_tuning_2069_rev36_40[NUM_ROWS_CHAN_TUNING];
extern CONST BCMATTACHDATA(chan_info_radio2069revGE32_t) chan_tuning_2069_rev39[NUM_ROWS_CHAN_TUNING];


extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_GE_18_40MHz_lp[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE16_t chan_tuning_2069rev_GE_18_lp[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE25_t chan_tuning_2069rev_GE_25[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE25_t chan_tuning_2069rev_GE_25_40MHz_lp[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE25_t chan_tuning_2069rev_GE_25_lp[NUM_ROWS_CHAN_TUNING];
extern CONST chan_info_radio2069revGE25_t chan_tuning_2069rev_GE_25_40MHz[NUM_ROWS_CHAN_TUNING];
extern CONST BCMATTACHDATA(chan_info_radio2069revGE25_52MHz_t) chan_tuning_2069rev_GE_25_52MHz[NUM_ROWS_CHAN_TUNING];


#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && (defined(BCMINTERNAL) || \
	defined(DBG_PHY_IOV))) || defined(BCMDBG_PHYDUMP)
extern CONST radio_20xx_dumpregs_t dumpregs_20693_rsdb[];
extern CONST radio_20xx_dumpregs_t dumpregs_20693_mimo[];
extern CONST radio_20xx_dumpregs_t dumpregs_20693_80p80[];
extern CONST radio_20xx_dumpregs_t dumpregs_20693_rev32[];
extern CONST radio_20xx_dumpregs_t dumpregs_2069_rev0[];
extern CONST radio_20xx_dumpregs_t dumpregs_2069_rev16[];
extern CONST radio_20xx_dumpregs_t dumpregs_2069_rev17[];
extern CONST radio_20xx_dumpregs_t dumpregs_2069_rev25[];
extern CONST radio_20xx_dumpregs_t dumpregs_2069_rev32[];
extern CONST radio_20xx_dumpregs_t dumpregs_2069_rev64[];
#endif /* BCMDBG, BCMDBG_DUMP, BCMINTERNAL, DBG_PHY_IOV, BCMDBG_PHYDUMP */
extern CONST radio_20xx_prefregs_t prefregs_2069_rev3[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev4[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev16[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev17[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev18[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev23[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev24[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev25[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev26[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev33_37[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev36[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev39[];
extern CONST radio_20xx_prefregs_t prefregs_2069_rev64[];
extern CONST uint16 ovr_regs_2069_rev2[];
extern CONST uint16 ovr_regs_2069_rev16[];
extern CONST uint16 ovr_regs_2069_rev32[];
#ifndef ACPHY_1X1_ONLY
extern CONST chan_info_rx_farrow BCMATTACHDATA(rx_farrow_tbl)[ACPHY_NUM_BW][ACPHY_NUM_CHANS];
extern CONST chan_info_tx_farrow BCMATTACHDATA(tx_farrow_dac1_tbl)[ACPHY_NUM_BW][ACPHY_NUM_CHANS];
extern CONST chan_info_tx_farrow BCMATTACHDATA(tx_farrow_dac2_tbl)[ACPHY_NUM_BW][ACPHY_NUM_CHANS];
extern CONST chan_info_tx_farrow BCMATTACHDATA(tx_farrow_dac3_tbl)[ACPHY_NUM_BW][ACPHY_NUM_CHANS];
#else /* ACPHY_1X1_ONLY */
extern CONST chan_info_rx_farrow BCMATTACHDATA(rx_farrow_tbl[1][ACPHY_NUM_CHANS]);
extern CONST chan_info_tx_farrow BCMATTACHDATA(tx_farrow_dac1_tbl[1][ACPHY_NUM_CHANS]);
#endif /* ACPHY_1X1_ONLY */
extern CONST uint16 acphy_txgain_epa_2g_2069rev0[];
extern CONST uint16 acphy_txgain_epa_5g_2069rev0[];
extern CONST uint16 acphy_txgain_ipa_2g_2069rev0[];
extern CONST uint16 acphy_txgain_ipa_5g_2069rev0[];

extern CONST uint16 acphy_txgain_ipa_2g_2069rev16[];

extern uint16 acphy_txgain_epa_2g_2069rev17[];
extern uint16 acphy_txgain_epa_5g_2069rev17[];
extern CONST uint16 acphy_txgain_ipa_2g_2069rev17[];
extern CONST uint16 acphy_txgain_ipa_5g_2069rev17[];
extern CONST uint16 acphy_txgain_epa_2g_2069rev18[];
extern CONST uint16 acphy_txgain_epa_5g_2069rev18[];


extern CONST uint16 acphy_txgain_epa_2g_2069rev4[];
extern CONST uint16 acphy_txgain_epa_2g_2069rev4_id1[];
extern CONST uint16 acphy_txgain_epa_5g_2069rev4[];
extern CONST uint16 acphy_txgain_epa_2g_2069rev16[];
extern CONST uint16 acphy_txgain_epa_5g_2069rev16[];
extern CONST uint16 acphy_txgain_ipa_2g_2069rev18[];
extern CONST uint16 acphy_txgain_ipa_5g_2069rev16[];
extern CONST uint16 acphy_txgain_ipa_2g_2069rev25[];
extern CONST uint16 acphy_txgain_ipa_5g_2069rev25[];
extern CONST uint16 acphy_txgain_ipa_5g_2069rev18[];
extern CONST uint16 acphy_txgain_epa_2g_2069rev33_35_36_37[];
extern CONST uint16 acphy_txgain_epa_5g_2069rev33_35_36[];
extern CONST uint16 acphy_txgain_epa_5g_2069rev37_38[];
extern CONST uint16 acphy_txgain_epa_2g_2069rev34[];
extern CONST uint16 acphy_txgain_epa_5g_2069rev34[];
extern CONST uint16 acphy_txgain_ipa_2g_2069rev33_37[];
extern CONST uint16 acphy_txgain_ipa_5g_2069rev33_37[];
extern CONST uint16 acphy_txgain_ipa_2g_2069rev39[];
extern CONST uint16 acphy_txgain_ipa_5g_2069rev39[];
extern CONST uint32 acphy_txv_for_spexp[];
extern uint16 BCMATTACHDATA(acphy_txgain_epa_2g_2069rev64)[TXGAIN_TABLES_LEN];
extern uint16 BCMATTACHDATA(acphy_txgain_epa_5g_2069rev64)[TXGAIN_TABLES_LEN];
extern uint16 BCMATTACHDATA(acphy_txgain_epa_5g_2069rev64_A0_SPUR_WAR)[TXGAIN_TABLES_LEN];

#endif /* _wlc_phytbl_ac_h_ */
