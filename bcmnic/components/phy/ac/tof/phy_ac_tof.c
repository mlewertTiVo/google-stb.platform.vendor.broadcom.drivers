/*
 * ACPHY TOF module implementation
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
 * $Id$
 */
#ifdef WL_PROXDETECT
#include <hndpmu.h>
#include <sbchipc.h>
#include <phy_mem.h>
#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_reg.h>
#include <phy_utils_var.h>
#include <phy_rstr.h>
#include "phy_type_tof.h"
#include <phy_ac.h>
#include <phy_ac_tof.h>
#include <phy_tof_api.h>

#define TOF_INITIATOR_K_4345_80M	34434 /* initiator K value for 80M */
#define TOF_TARGET_K_4345_80M		34474 /* target K value for 80M */
#define TOF_INITIATOR_K_4345_40M	35214 /* initiator K value for 40M */
#define TOF_TARGET_K_4345_40M		35214 /* target K value for 40M */
#define TOF_INITIATOR_K_4345_20M	36553 /* initiator K value for 20M */
#define TOF_TARGET_K_4345_20M		36553 /* target K value for 20M */
#define TOF_INITIATOR_K_4345_2G		37169 /* initiator K value for 2G */
#define TOF_TARGET_K_4345_2G		37169 /* target K value for 2G */

static const  uint16 BCMATTACHDATA(proxd_4345_80m_k_values)[] =
{0x0, 0xee12, 0xe201, 0xe4fc, 0xe6f8, 0xe6f7 /* 42, 58, 106, 122, 138, 155 */};

static const  uint16 BCMATTACHDATA(proxd_4345_40m_k_values)[] =
{0x7b7b, 0x757c, 0x7378, 0x7074, 0x9a9a, 0x9898, /* 38, 46, 54, 62, 102,110 */
0x9898, 0x9898, 0x9393, 0x9494, 0x9191, 0x8484 /* 118, 126, 134, 142,151,159 */};

static const  uint16 BCMATTACHDATA(proxd_4345_20m_k_values)[] =
{0x0f0f, 0x0101, 0x0101, 0x1313, 0x0f0f, 0x0101, 0x0f0f, 0x0505, /* 36 -64 */
0xe9e9, 0xe8e8, 0xe6e6, 0xe4e4, 0xcbcb, 0xcbcb, 0xcbcb, 0xcbcb, /* 100 -128 */
0xcbcb, 0xd5d5, 0xdada, 0xcbcb, /* 132 -144 */
0xcbcb, 0xbfbf, 0xd5d5, 0xbfbf, 0xcbcb /* 149 - 165 */
};

static const  uint16 BCMATTACHDATA(proxd_4345_2g_k_values)[] =
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, /* 1 -7 */
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 /* 8 -14 */
};

#define TOF_INITIATOR_K_4350_80M	36546 /* initiator K value for 80M */
#define TOF_TARGET_K_4350_80M		36569 /* target K value for 80M */
#define TOF_INITIATOR_K_4350_40M	35713 /* initiator K value for 40M */
#define TOF_TARGET_K_4350_40M		35713 /* target K value for 40M */
#define TOF_INITIATOR_K_4350_20M	37733 /* initiator K value for 20M */
#define TOF_TARGET_K_4350_20M		37733 /* target K value for 20M */
#define TOF_INITIATOR_K_4350_2G		37733 /* initiator K value for 2G */
#define TOF_TARGET_K_4350_2G		37733 /* target K value for 2G */

static const  uint16 BCMATTACHDATA(proxd_4350_80m_k_values)[] =
{0x0, 0xef02, 0xf404, 0xf704, 0xfc04, 0xf3f9 /* 42, 58, 106, 122, 138, 155 */};

static const  uint16 BCMATTACHDATA(proxd_4350_40m_k_values)[] =
{0x0, 0xfdfd, 0xf6f6, 0x1414, 0xebeb, 0xebeb, /* 38, 46, 54, 62, 102,110 */
0xeeee, 0xeeee, 0xe2e2, 0xe5e5, 0xfdfa, 0xe5e5 /* 118, 126, 134, 142,151,159 */};

static const  uint16 BCMATTACHDATA(proxd_4350_20m_k_values)[] =
{0x0, 0xfdfd, 0xfdfd, 0xf8f8, 0xf8f8, 0xf5f5, 0xf5f5, 0xf5f5, /* 36 -64 */
0xe9e9, 0xe6e6, 0xe3e3, 0xe3e3, 0xe6e6, 0xe6e6, 0xe6e6, 0xe6e6, /* 100 - 128 */
0xe6e6, 0xe6e6, 0xd6d6, 0xe9e9, /* 132 -144 */
0xe9e9, 0x0808, 0xd4d4, 0xd4d4, 0xd4d4 /* 149 - 165 */
};

static const  uint16 BCMATTACHDATA(proxd_4350_2g_k_values)[] =
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, /* 1 -7 */
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 /* 8 -14 */
};

#define TOF_INITIATOR_K_4354_80M	36553 /* initiator K value for 80M */
#define TOF_TARGET_K_4354_80M		36559 /* target K value for 80M */
#define TOF_INITIATOR_K_4354_40M	35699 /* initiator K value for 40M */
#define TOF_TARGET_K_4354_40M		35713 /* target K value for 40M */
#define TOF_INITIATOR_K_4354_20M	37723 /* initiator K value for 20M */
#define TOF_TARGET_K_4354_20M		37728 /* target K value for 20M */
#define TOF_INITIATOR_K_4354_2G		37816 /* initiator K value for 2G */
#define TOF_TARGET_K_4354_2G		37816 /* target K value for 2G */

static const  uint16 BCMATTACHDATA(proxd_4354_80m_k_values)[] =
{0, 0xF0F3, 0xFCFE, 0, 0xFEFE, 0xF0F4 /* 42, 58, 106, 122, 138, 155 */};

static const  uint16 BCMATTACHDATA(proxd_4354_40m_k_values)[] =
{0, 0xFC04, 0xFA00, 0x0d1D, 0xF1FB, 0xF0FB, /* 38, 46, 54, 62, 102 */
0, 0, 0xE9F5, 0xE5F4, 0xFE0B, 0xE0EF /* 110, 118, 126, 134, 142,151,159 */};

static const  uint16 BCMATTACHDATA(proxd_4354_20m_k_values)[] =
{0, 0xFFFE, 0xFAFB, 0xFBFA, 0xF9F8, 0xFAF7, 0xF4F4, 0xFDEC, /* 36 -64 */
0xEEE1, 0xE3E3, 0xE2E0, 0xE2E2, 0, 0, 0, 0, 0xD9DB, 0xDEDD, 0xD7D7, 0xD9D6, /* 100 -144 */
0x0BFD, 0x0209, 0xD6D1, 0xE1C6, 0xE1C6 /* 149 - 165 */
};

static const  uint16 BCMATTACHDATA(proxd_4354_2g_k_values)[] =
{0xbbbb, 0xf1f1, 0xebeb, 0xe5e5, 0xe6e6, 0xe6e6, 0xe3e3, /* 1 -7 */
0xe6e6, 0xe3e3, 0xe3e3, 0xe6e6, 0xb5b5, 0xf0f0, 0x0c0c /* 8 -14 */
};

#define TOF_INITIATOR_K_4349_80M	35984 /* initiator K value for 80M */
#define TOF_TARGET_K_4349_80M		35982 /* target K value for 80M */
#define TOF_INITIATOR_K_4349_40M	35512 /* initiator K value for 40M */
#define TOF_TARGET_K_4349_40M		35513 /* target K value for 40M */
#define TOF_INITIATOR_K_4349_20M	37466 /* initiator K value for 20M */
#define TOF_TARGET_K_4349_20M		37470 /* target K value for 20M */
#define TOF_INITIATOR_K_4349_2G		37550 /* initiator K value for 2G */
#define TOF_TARGET_K_4349_2G		37550 /* initiator K value for 2G */

static const  uint16 BCMATTACHDATA(proxd_4349_80m_k_values)[] =
{0, 0xFFFE, 0xF2F2, 0xF2F2, 0xF0EF, 0xE8E7 /* 42, 58, 106, 122, 138, 155 */};

static const  uint16 BCMATTACHDATA(proxd_4349_40m_k_values)[] =
{0, 0x00FF, 0x00FF, 0xFFFF, 0xFAFB, 0xF2F3, /* 38, 46, 54, 62, 102 110 */
0xF2F3, 0xF2F3, 0xE7E8, 0xE7E8, 0xE5E6, 0xE5E6 /* 118, 126, 134, 142,151,159 */};

static const  uint16 BCMATTACHDATA(proxd_4349_20m_k_values)[] =
{0, 0x0, 0x0300, 0x0300, 0xFCFF, 0xFAFC, 0xF9F9, 0xF9F7, /* 36 -64 */
0xEFEE, 0xE8EC, 0xE9E9, 0xE6E8, 0xCDCE, 0xCDCE, 0xCDCE, 0xCDCE, 0xCCCF, 0xCCCF,  /* 100 -136 */
0xC8CC, 0xC6CA, 0xC6CA, 0xC5C8, 0xC6C7, 0xC5C6, 0xC5C6 /* 140 - 165 */
};

static const  uint16 BCMATTACHDATA(proxd_4349_2g_k_values)[] =
{0x2222, 0x1e1e, 0x1919, 0x1313, 0x0f0f, 0x0e0e, 0x0f0f, /* 1 -7 */
0x1111, 0x1313, 0x1313, 0x1515, 0x1818, 0x1d1d, 0x3939 /* 8 -14 */
};

#define TOF_INITIATOR_K_43602_80M      33836 /* initiator K value for 80M */
#define TOF_TARGET_K_43602_80M         33836 /* target K value for 80M */
#define TOF_INITIATOR_K_43602_40M      35450 /* initiator K value for 40M */
#define TOF_TARGET_K_43602_40M         35475 /* target K value for 40M */
#define TOF_INITIATOR_K_43602_20M      36000 /* initiator K value for 20M */
#define TOF_TARGET_K_43602_20M         35936 /* target K value for 20M */
#define TOF_INITIATOR_K_43602_2G       36390 /* initiator K value for 2G */
#define TOF_TARGET_K_43602_2G          36390 /* target K value for 2G */

static const  uint16 BCMATTACHDATA(proxd_43602_80m_k_values)[] =
{0x0000, 0x0000, 0x0000, 0, 0x0000, 0x0000 /* 42, 58, 106, 122, 138, 155 */};

static const  uint16 BCMATTACHDATA(proxd_43602_40m_k_values)[] =
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, /* 38, 46, 54, 62, 102 110 */
0x0, 0x0, 0x0, 0x0, 0x0, 0x0 /* 118, 126, 134, 142,151,159 */};

static const  uint16 BCMATTACHDATA(proxd_43602_20m_k_values)[] =
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, /* 36 -64 */
0x0, 0x0, 0x0, 0x0, 0, 0, 0, 0, 0x0, 0x0, 0x0, 0x0, /* 100 -144 */
0x0, 0x0, 0x0, 0x0, 0x0 /* 149 - 165 */
};

static const  uint16 BCMATTACHDATA(proxd_43602_2g_k_values)[] =
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, /* 1 -7 */
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 /* 8 -14 */
};

/* ratespec related k offset table for initiator <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int16 BCMATTACHDATA(proxdi_rate_offset_2g)[] = { 0, 0, 0, 0 };
static const  int16 BCMATTACHDATA(proxdi_rate_offset_20m)[] = { 0, 0, 0, 0 };
static const  int16 BCMATTACHDATA(proxdi_rate_offset_40m)[] = { 0, 0, 0, 0 };
static const  int16 BCMATTACHDATA(proxdi_rate_offset_80m)[] = { 0, 0, 0, 0 };

/* ratespec related k offset table for target <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int16 BCMATTACHDATA(proxdt_rate_offset_2g)[] = { 0, 13731, 61, 5743 };
static const  int16 BCMATTACHDATA(proxdt_rate_offset_20m)[] = { 0, 13768, 236, 5776 };
static const  int16 BCMATTACHDATA(proxdt_rate_offset_40m)[] = { 0, 14779, 7, 6333 };
static const int16 BCMATTACHDATA(proxdt_rate_offset_80m)[] = { 0, 13400, -2804, -2804 };

/* legacy ack offset table for initiator  <80M, 40M, 20M, 2g> */
static const int16 BCMATTACHDATA(proxdi_ack_offset)[] = { -2, -3, 2360, 2333};


/* legacy ack offset table for initiator  <80M, 40M, 20M, 2g> */
static const int16 BCMATTACHDATA(proxdt_ack_offset)[] = { 2728, 2031, 0, 0};

/* different bandwidth  k offset table <VHT legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const int16 BCMATTACHDATA(proxd_subbw_offset) [3][5] = {
	/* 80M-40M */
	{1036, 325, 1366, 2094, 5273},
	/* 80M -20M */
	{1470, -170, 1400, 1412, 5507},
	/* 40M - 20M */
	{200, -714, -216, -800, 20}
};

#ifdef WL_PROXD_SEQ
#define k_tof_filt_1_mask   0x1
#define k_tof_filt_neg_mask 0xc
#define k_tof_filt_non_zero_mask 0x3

#ifdef TOF_SEQ_20MHz_BW_512IFFT

#define k_tof_seq_fft_20MHz 64
#define k_tof_seq_n_20MHz 9
#define k_tof_seq_ifft_20MHz (1 << k_tof_seq_n_20MHz)

#endif

#define k_tof_rfseq_tiny_bundle_base 8
#define k_tof_seq_tiny_rx_fem_gain_offset 0x29
#define k_tof_seq_tiny_tx_fem_gain_offset 0x2b
static const uint16 k_tof_seq_tiny_tbls[] = {
	ACPHY_TBL_ID_RFSEQ,
	0x260,
	15,
	0x42, 0x10, 0x8b, 0x9b, 0xaa, 0xb9, 0x8c, 0x9c, 0x9d,
	0xab, 0x8d, 0x9e, 0x43, 0x42, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x290,
	14,
	0x42, 0x10, 0x88, 0x98, 0xa8, 0xb8, 0x89, 0x99, 0xa9,
	0x8a, 0x9a, 0x43, 0x42, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x2f0,
	15,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04, 0x04,
	0x04, 0x01, 0x04, 0x1e, 0x01, 0x01,
	ACPHY_TBL_ID_RFSEQ,
	0x320,
	14,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04, 0x04,
	0x01, 0x04, 0x1e, 0x01, 0x01,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x008,
	18,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x018,
	21,
	0x0009, 0x8000, 0x0007, 0xf009, 0x8066, 0x0007, 0xf009, 0x8066, 0x0004,
	0x00c9, 0x8060, 0x0007, 0xf7d9, 0x8066, 0x0007, 0xf7f9, 0x8066, 0x0007,
	0xf739, 0x8066, 0x0004,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x028,
	12,
	0x0009, 0x0000, 0x0000, 0x0229, 0x0000, 0x0000, 0x0019, 0x0000, 0x0000,
	0x0099, 0x0000, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x038,
	6,
	0x0fc9, 0x00a6, 0x0000, 0x0fc9, 0x00a6, 0x0000,
};
#define k_tof_seq_rx_gain_tiny ((0 << 13) | (0 << 10) | (5 << 6) | (0 << 3) | (4 << 0))
#define k_tof_seq_rx_loopback_gain_tiny ((0 << 13) | (0 << 10) | (0 << 6) | (0 << 3) | (0 << 0))

#define k_tof_rfseq_bundle_base 8
#define k_tof_seq_rx_fem_gain_offset 0x29
#define k_tof_seq_tx_fem_gain_offset 0x2b
static const uint16 k_tof_seq_tbls[] = {
	ACPHY_TBL_ID_RFSEQ,
	0x260,
	14,
	0x10, 0x8b, 0x9b, 0xaa, 0xb9, 0xc9, 0xd9, 0x8c, 0x9c,
	0x9d, 0xab, 0x8d, 0x9e, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x290,
	13,
	0x10, 0x88, 0x98, 0xa8, 0xb8, 0xc8, 0xd8, 0x89, 0x99,
	0xa9, 0x8a, 0x9a, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x2f0,
	14,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04,
	0x04, 0x04, 0x01, 0x04, 0x01,
	ACPHY_TBL_ID_RFSEQ,
	0x320,
	13,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04,
	0x04, 0x01, 0x04, 0x01,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x008,
	18,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x018,
	21,
	0x0009, 0x8000, 0x000f, 0xff09, 0x8066, 0x000f, 0xff09, 0x8066, 0x000c,
	0x00c9, 0x8060, 0x000f, 0xffd9, 0x8066, 0x000f, 0xfff9, 0x8066, 0x000f,
	0xff39, 0x8066, 0x000c,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x028,
	12,
	0x0009, 0x0000, 0x0000, 0x0229, 0x0000, 0x0000, 0x0019, 0x0000, 0x0000,
	0x0099, 0x0000, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x038,
	6,
	0x0fc9, 0x00a6, 0x0000, 0x0fc9, 0x00a6, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x048,
	6,
	0x2099, 0x586e, 0x0000, 0x1519, 0x586e, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x058,
	6,
	0x2409, 0xc040, 0x000c, 0x6b89, 0xc040, 0x000c,
};
#define k_tof_seq_rx_gain ((0 << 13) | (2 << 10) | (2 << 6) | (6 << 3) | (4 << 0))
#define k_tof_seq_rx_loopback_gain ((2 << 13) | (4 << 10) | (3 << 6) | (3 << 3) | (3 << 0))

#define k_tof_seq_rfseq_gain_base 0x1d0
#define k_tof_seq_rfseq_rx_gain_offset 7
#define k_tof_seq_rfseq_loopback_gain_offset  4

#define k_tof_seq_shm_offset 4
#define k_tof_seq_shm_setup_regs_offset 2*(0 + k_tof_seq_shm_offset)
#define k_tof_seq_shm_setup_regs_len    15
#define k_tof_seq_shm_setup_vals_offset 2*(15 + k_tof_seq_shm_offset)
#define k_tof_seq_shm_setup_vals_len    16
#define k_tof_set_shm_restr_regs_offset 2*(7 + k_tof_seq_shm_offset)
#define k_tof_set_shm_restr_vals_offset 2*(31 + k_tof_seq_shm_offset)
#define k_tof_set_shm_restr_vals_len    8
#define k_tof_seq_shm_fem_radio_hi_gain_offset 2*(18 + k_tof_seq_shm_offset)
#define k_tof_seq_shm_fem_radio_lo_gain_offset 2*(39 + k_tof_seq_shm_offset)
#define k_tof_seq_shm_dly_offset 2*(40 + k_tof_seq_shm_offset)
#define k_tof_seq_shm_dly_len    (3*2)

#define k_tof_seq_sc_start 1024
#define k_tof_seq_sc_stop  (k_tof_seq_sc_start + 2730)


#define k_tof_rfseq_dc_run_event 0x43
#define k_tof_rfseq_epa_event    0x4
#define k_tof_rfseq_end_event    0x1f
#define k_tof_rfseq_tssi_event   0x35
#define k_tof_rfseq_ipa_event    0x3
#define k_tof_rfseq_tx_gain_event 0x6

static const uint16 k_tof_seq_ucode_regs[k_tof_seq_shm_setup_regs_len] = {
	ACPHY_RxControl(0),
	ACPHY_TableID(0),
	ACPHY_TableOffset(0),
	(ACPHY_TableDataWide(0) | (7 << 12)),
	ACPHY_TableID(0),
	ACPHY_TableOffset(0),
	ACPHY_TableDataLo(0),
	ACPHY_TableOffset(0),
	ACPHY_TableDataLo(0),
	ACPHY_RfseqMode(0),
	ACPHY_sampleCmd(0),
	ACPHY_RfseqMode(0),
	ACPHY_SlnaControl(0),
	ACPHY_AdcDataCollect(0),
	ACPHY_RxControl(0),
};

static const uint16 k_tof_seq_fem_gains[] = {
	(8 | (1 << 5) | (1 << 9)), /* fem hi */
	(8 | (1 << 5)), /* fem lo */
};

#define k_tof_seq_in_scale (1<<12)
#define k_tof_seq_out_scale 11
#define k_tof_seq_out_shift 8
#define k_tof_mf_in_shift  0
#define k_tof_mf_out_scale 0
#define k_tof_mf_out_shift 0

#if defined(TOF_DBG)
#define k_tof_dbg_sc_delta 32
#endif /* defined(TOF_DBG) */

#endif /* WL_PROXD_SEQ */

#define tof_w(x, n, log2_n) (x - ((1<<log2_n)/2) + (n*(1<<log2_n)))

#if defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT)

#define k_tof_seq_log2_n_20MHz 7

/* 20MHz case */
static const uint16 k_tof_ucode_dlys_us_20MHz[2][5] = {
	{1, 8, 8, tof_w(200, 2, k_tof_seq_log2_n_20MHz), tof_w(
			200,0, k_tof_seq_log2_n_20MHz)}, /* RX -> TX */

	{2, 6, 8, tof_w(480, -2, k_tof_seq_log2_n_20MHz), tof_w(
			480, 0, k_tof_seq_log2_n_20MHz)}, /* TX -> RX */
};

#endif /* defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT) */

#ifdef TOF_SEQ_40MHz_BW

#define k_tof_seq_log2_n_40MHz 8

/* For 20 in 40MHz */
static const uint16 k_tof_ucode_dlys_us_40MHz[2][5] = {
	{1, 8, 8, tof_w(338, 3, k_tof_seq_log2_n_40MHz), tof_w(
			338, 0, k_tof_seq_log2_n_40MHz)},  /* RX -> TX */
	{2, 6, 8, tof_w(1188, -3, k_tof_seq_log2_n_40MHz), tof_w(
			1188, 0, k_tof_seq_log2_n_40MHz)}, /* TX -> RX */
};

#endif

#ifdef TOF_SEQ_40_IN_40MHz

#define k_tof_seq_log2_n_40MHz 8

/* For 40 in 40MHz */
static const uint16 k_tof_ucode_dlys_us_40MHz_40[2][5] = {
	{1, 8, 8, tof_w(338, 3, k_tof_seq_log2_n_40MHz), tof_w(
			338, 0, k_tof_seq_log2_n_40MHz)},  /* RX -> TX */
	{2, 6, 8, tof_w(1188, -3, k_tof_seq_log2_n_40MHz), tof_w(
			1188, 0, k_tof_seq_log2_n_40MHz)}, /* TX -> RX */
};

#endif

#ifdef TOF_SEQ_20_IN_80MHz

#define k_tof_seq_log2_n_80MHz_20 9

/* For 20 in 80MHz */
static const uint16 k_tof_ucode_dlys_us_80MHz_20[2][5] = {
	{1, 8, 8, tof_w(698, 3, k_tof_seq_log2_n_80MHz_20), tof_w(
			698, 0, k_tof_seq_log2_n_80MHz_20)}, /* RX -> TX */
	{2, 6, 8, tof_w(2368, -3, k_tof_seq_log2_n_80MHz_20), tof_w(
			2368, 0, k_tof_seq_log2_n_80MHz_20)}, /* TX -> RX */
};

#endif

#if !defined(TOF_SEQ_20_IN_80MHz)

#define k_tof_seq_log2_n_80MHz 8

/* Original 80MHz case */
static const uint16 k_tof_ucode_dlys_us_80MHz[2][5] = {
	{1, 6, 6, tof_w(750, 4, k_tof_seq_log2_n_80MHz), tof_w(
			750, 0, k_tof_seq_log2_n_80MHz)}, /* RX -> TX */
	{2, 6, 6, tof_w(1850, -4, k_tof_seq_log2_n_80MHz), tof_w(
			1850, 0, k_tof_seq_log2_n_80MHz)}, /* TX -> RX */
};

#endif


#if (defined(TOF_SEQ_20_IN_80MHz) || defined(TOF_SEQ_20MHz_BW) || \
	defined(TOF_SEQ_40MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT))

#define k_tof_seq_spb_len_20MHz 4
static const uint32 k_tof_seq_spb_20MHz[2*k_tof_seq_spb_len_20MHz] = {
	0x1f11ff10,
	0x1fffff1f,
	0x1f1f1ff1,
	0x000ff111,
	0x11110000,
	0x1f1f11ff,
	0x1ff11111,
	0x1111f1f1,
};

#endif

#ifdef TOF_SEQ_40_IN_40MHz

#define k_tof_seq_spb_len_40MHz 8
static const uint32 k_tof_seq_spb_40MHz[2*k_tof_seq_spb_len_40MHz] = {
	0xf11ff110,
	0x111111f1,
	0x1f1f11ff,
	0x1ff11111,
	0xfff1f1f1,
	0xf1ff11ff,
	0xff1111f1,
	0x0000001f,
	0xf0000000,
	0x1ff11f11,
	0x1111f1f1,
	0x1f11ff11,
	0xf111111f,
	0xf1f1f11f,
	0xff11ffff,
	0x1111f1f1,
};

#endif /* TOF_SEQ_40_IN_40MHz */

#if !defined(TOF_SEQ_20_IN_80MHz)

#define k_tof_seq_spb_len_80MHz 8
static const uint32 k_tof_seq_spb_80MHz[2*k_tof_seq_spb_len_80MHz] = {
	0xee2ee200,
	0xe2e2ee22,
	0xe22eeeee,
	0xeeee2e2e,
	0xe2ee22ee,
	0xe22222e2,
	0xe2e2e22e,
	0x00000eee,
	0x11000000,
	0x1f1f11ff,
	0x1ff11111,
	0x1111f1f1,
	0x1f11ff11,
	0x1fffff1f,
	0x1f1f1ff1,
	0x01fff111,
};

#endif /* !defined(TOF_SEQ_20_IN_80MHz) */

#define k_tof_unpack_sgn_mask (1<<31)
#define k_tof_k_rtt_adj_Q 2

/* module private states */
struct phy_ac_tof_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_tof_info_t *ti;
	uint16 proxd_80m_k_values[6];
	uint16 proxd_40m_k_values[12];
	uint16 proxd_20m_k_values[25];
	uint16 proxd_2g_k_values[14];
	int16 proxdi_rate_offset_80m[4];
	int16 proxdi_rate_offset_40m[4];
	int16 proxdi_rate_offset_20m[4];
	int16 proxdi_rate_offset_2g[4];
	int16 proxdt_rate_offset_80m[4];
	int16 proxdt_rate_offset_40m[4];
	int16 proxdt_rate_offset_20m[4];
	int16 proxdt_rate_offset_2g[4];
	int16 proxdi_ack_offset[4];
	int16 proxdt_ack_offset[4];
	int16 proxd_subbw_offset[3][5];
	uint16 proxd_ki[4];
	uint16 proxd_kt[4];
	uint16 tof_ucode_dlys_us[2][5];
	uint16 tof_rfseq_bundle_offset;
	uint16 tof_shm_ptr;
	uint32 *tof_seq_spb;
	uint8 tof_sc_FS;
	uint8 tof_seq_log2_n;
	uint8 tof_seq_spb_len;
	uint8 tof_rx_fdiqcomp_enable;
	uint8 tof_tx_fdiqcomp_enable;
	uint8 tof_core;
	uint8 tof_smth_enable;
	uint8 tof_smth_dump_mode;
	bool tof_setup_done;
	bool tof_tx;
};


/* Local functions */
static int phy_ac_tof(phy_type_tof_ctx_t *ctx, bool enter, bool tx, bool hw_adj,
	bool seq_en, int core);
static int phy_ac_tof_info(phy_type_tof_ctx_t *ctx, int* p_frame_type, int* p_frame_bw,
	int* p_cfo, int8* p_rssi);
static void phy_ac_tof_cmd(phy_type_tof_ctx_t *ctx, bool seq);
static int phy_ac_tof_kvalue(phy_type_tof_ctx_t *ctx, chanspec_t chanspec, uint32 rspecidx,
	uint32 *kip, uint32 *ktp, uint8 seq_en);
static void phy_ac_tof_seq_upd_dly(phy_type_tof_ctx_t *ctx, bool tx, uint8 core,
	bool mac_suspend);
static int phy_ac_tof_seq_params(phy_type_tof_ctx_t *ctx);
static int phy_ac_chan_freq_response(phy_type_tof_ctx_t *ctx, int len, int nbits,
	cint32* H, uint32* Hraw);
static int phy_ac_chan_mag_sqr_impulse_response(phy_type_tof_ctx_t *ctx, int frame_type,
	int len, int offset, int nbits, int32* h, int* pgd, uint32* hraw, uint16 tof_shm_ptr);
static int phy_ac_seq_ts(phy_type_tof_ctx_t *ctx, int n, cint32* p_buffer, int tx, int cfo,
	int adj, void* pparams, int32* p_ts, int32* p_seq_len, uint32* p_raw);
static void phy_ac_nvram_proxd_read(phy_info_t *pi,  phy_ac_tof_info_t *tofi);
static int wlc_phy_tof_reset_acphy(phy_type_tof_ctx_t *ctx);
/* DEBUG */
#ifdef TOF_DBG
static int phy_ac_tof_dbg(phy_type_tof_ctx_t *ctx, int arg);
#endif

/* register phy type specific implementation */
phy_ac_tof_info_t *
BCMATTACHFN(phy_ac_tof_register_impl)(phy_info_t *pi, phy_ac_info_t *aci, phy_tof_info_t *ti)
{
	phy_ac_tof_info_t *tofi;
	phy_type_tof_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((tofi = phy_malloc(pi, sizeof(phy_ac_tof_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	tofi->pi = pi;
	tofi->aci = aci;
	tofi->ti = ti;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init_tof = wlc_phy_tof_reset_acphy;
	fns.tof = phy_ac_tof;
	fns.cmd = phy_ac_tof_cmd;
	fns.info = phy_ac_tof_info;
	fns.kvalue = phy_ac_tof_kvalue;
	fns.seq_params = phy_ac_tof_seq_params;
	fns.seq_upd_dly = phy_ac_tof_seq_upd_dly;
	fns.chan_freq_response = phy_ac_chan_freq_response;
	fns.chan_mag_sqr_impulse_response = phy_ac_chan_mag_sqr_impulse_response;
	fns.seq_ts = phy_ac_seq_ts;
/* DEBUG */
#ifdef TOF_DBG
	fns.dbg = phy_ac_tof_dbg;
#endif
	fns.ctx = tofi;
	phy_ac_nvram_proxd_read(pi, tofi);
	phy_tof_register_impl(ti, &fns);

	return tofi;

	/* error handling */
fail:
	if (tofi != NULL)
		phy_mfree(pi, tofi, sizeof(phy_ac_tof_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_tof_unregister_impl)(phy_ac_tof_info_t *tofi)
{
	phy_info_t *pi = tofi->pi;
	phy_tof_info_t *ti = tofi->ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_tof_unregister_impl(ti);

	phy_mfree(pi, tofi, sizeof(phy_ac_tof_info_t));
}

static int
WLBANDINITFN(wlc_phy_tof_reset_acphy)(phy_type_tof_ctx_t *ctx)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_acphy_t *pi_ac = tofi->aci;

	tofi->tof_setup_done = FALSE;
	pi_ac->tof_active = FALSE;
	pi_ac->tof_smth_forced = FALSE;
	tofi->tof_rfseq_bundle_offset = 0;
	tofi->tof_shm_ptr =
		(wlapi_bmac_read_shm(tofi->pi->sh->physhim, M_TOF_BLK_PTR(tofi->pi)) * 2);

	return BCME_OK;
}

void
phy_ac_tof_tbl_offset(phy_ac_tof_info_t *tofi, uint32 id, uint32 offset)
{
	/* To keep track of first available offset usable by tof procs */
	if ((id == ACPHY_TBL_ID_RFSEQBUNDLE) && ((offset & 0xf) > tofi->tof_rfseq_bundle_offset))
		tofi->tof_rfseq_bundle_offset = (uint16)(offset & 0xf);
	if ((id == ACPHY_TBL_ID_RFSEQBUNDLE) || (id == ACPHY_TBL_ID_SAMPLEPLAY))
		tofi->tof_setup_done = FALSE;
}

#undef TOF_TEST_TONE

#ifdef WL_PROXD_SEQ
static void phy_ac_tof_sc(phy_info_t *pi, bool setup, int sc_start, int sc_stop, uint16 cfg)
{
	uint16 phy_ctl;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	if (D11REV_GE(pi->sh->corerev, 50)) {
		phy_ctl = R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectPlayCtrl);
		phy_ctl &= ~((1<<0) | (1<<2));
		W_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectPlayCtrl, phy_ctl);
	} else {
		phy_ctl = R_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param) & ~((1<<4) | (1<<5));
		W_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param, phy_ctl);
	}

		MOD_PHYREG(pi, RxFeTesMmuxCtrl, samp_coll_core_sel, pi_ac->tofi->tof_core);
		MOD_PHYREG(pi, RxFeTesMmuxCtrl, rxfe_dbg_mux_sel, 4);

		WRITE_PHYREG(pi, AdcDataCollect, 0);

	/* Disable MAC Clock gating for 43012 */

	if (setup) {
	  acphy_set_sc_startptr(pi, (uint32)sc_start);
	  acphy_set_sc_stopptr(pi, (uint32)sc_stop);
	  if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev)) {
	    uint32 pmu_chipctReg5 = si_pmu_chipcontrol(pi->sh->sih,
	                                               PMU_CHIPCTL5, 0, 0) & 0xcfe0ffff;
	    pmu_chipctReg5 |= (0x1036 << 16);
	    si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL5, 0xFFFFFFFF, pmu_chipctReg5);
	    pmu_chipctReg5 |= (1 << 19);
	    si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL5, 0xFFFFFFFF, pmu_chipctReg5);
	  }
	}
	if (cfg) {
		if (D11REV_GE(pi->sh->corerev, 50))
			W_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectPlayCtrl,
			      phy_ctl | (1<<0) | (1<<2));
		else
			W_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param,
			      phy_ctl | ((1<<4) | (1<<5)));
		WRITE_PHYREG(pi, AdcDataCollect, cfg);
	}
}

static int phy_ac_tof_sc_read(phy_info_t *pi, bool iq, int n, cint32* pIn,
                                     int16 sc_ptr, int16 sc_base_ptr, int16* p_sc_start_ptr)
{
	d11regs_t *regs = pi->regs;
	uint32 dataL = 0, dataH, data;
	cint32* pEnd = pIn + n;
	int nbits = 0, n_out = 0;
	int32* pOut = (int32*)pIn;
	int16 sc_end_ptr;

	if (sc_ptr <= 0) {
	  /* Offset from sc_base_ptr */
	  sc_ptr = (-sc_ptr >> 2);
	  *p_sc_start_ptr = (sc_ptr << 2);
	  if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev))
	    sc_ptr = 3*sc_ptr;
	  else
	    sc_ptr = (sc_ptr << 2);
	  sc_ptr += sc_base_ptr;
	} else {
	  /* Actual address */
	  if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev)) {
	    sc_ptr = (sc_ptr - sc_base_ptr)/3;
	    *p_sc_start_ptr = 4*sc_ptr + sc_base_ptr;
	    sc_ptr = 3*sc_ptr + sc_base_ptr;
	  } else {
	    *p_sc_start_ptr = sc_ptr;
	  }
	}
	sc_end_ptr = R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectCurPtr);
	W_REG(pi->sh->osh, &pi->regs->tplatewrptr, ((uint32)sc_ptr << 2));
	while ((pIn < pEnd) && (sc_ptr < sc_end_ptr)) {
	  if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev)) {
	    dataH = dataL;
	    dataL = (uint32)R_REG(pi->sh->osh, &regs->tplatewrdata);
	    nbits += 32;
	    do {
	      nbits -= 12;
	      data = (dataL >> nbits);
	      if (nbits)
		data |= (dataH << (32 - nbits));
	      if (nbits & 4) {
		pIn->q = (int32)(data) & 0xfff;
	      } else {
		pIn->i = (int32)(data) & 0xfff;
		pIn++;
		n_out++;
	      }
	    } while (nbits >= 12);
	  } else {
	    dataL = (uint32)R_REG(pi->sh->osh, &regs->tplatewrdata);
	    pIn->i = (int32)(dataL & 0xfff);
	    pIn->q = (int32)((dataL >> 16) & 0xfff);
	    pIn++;
	    n_out++;
	  }
	  sc_ptr++;
	}
	if (iq) {
	  int32 datum;

	  n = 2*n_out;
	  while (n-- > 0) {
	    datum = *pOut;
	    if (datum > 2047)
	      datum -= 4096;
	    *pOut++ = datum;
	  }
	}
	return n_out;
}


static int
wlc_tof_rfseq_event_offset(phy_info_t *pi, uint16 event, uint16* rfseq_events)
{
	int i;

	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, (uint32)0, 16, (void*)rfseq_events);

	for (i = 0; i < 16; i++) {
	  if (rfseq_events[i] == event) {
	    break;
	  }
	}
	return i;
}

static void phy_ac_tof_cfo(phy_info_t *pi, int cfo, int n, cint32* pIn)
{
	/* CFO Correction Function Placeholder */
}

static void phy_ac_tof_mf(phy_info_t *pi, int n, cint32* pIn, bool seq, int a,
                           int b, int cfo, int s1, int k2, int s2)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;

	int i, k;
	cint32 *pTmp;
	int32 tmp;
	int nF = tofi->tof_seq_spb_len;
	const uint32* pF = tofi->tof_seq_spb;

	pTmp = pIn;
	for (i = 0; i < n; i++) {
	  if (seq) {
	    pTmp->q = 0;
	    pTmp->i = (int32)s1;
	  } else {
	    pTmp->q = (pTmp->q + ((pTmp->i*(int32)a)>>10)) << s1;
	    pTmp->i = (pTmp->i + ((pTmp->i*(int32)b)>>10)) << s1;
	  }
	  pTmp++;
	}

	if (!seq) {
	  if (cfo) {
		  phy_ac_tof_cfo(pi, cfo, n, pIn);
	  }
#ifdef TOF_SEQ_20MHz_BW_512IFFT
	  if (CHSPEC_IS20(pi->radio_chanspec)) {
		  pTmp = pIn;
		  for (i = 0; i < k_tof_seq_fft_20MHz; i++) {
			/* No down sampling for 43012 */
			  pTmp[i] = pIn[i*2];
		  }

		  memset(&pIn[k_tof_seq_fft_20MHz], 0, (k_tof_seq_fft_20MHz * sizeof(cint32)));
		  n = k_tof_seq_fft_20MHz;
	  }
#endif

		/* Undo IQ-SWAP for 43012 */

	  wlapi_fft(pi->sh->physhim, n, (void*)pIn, (void*)pIn, 2);
	}

	pTmp = pIn;
	for (k = 0; k < 2; k++) {
	  for (i = 0; i < nF; i++) {
	    uint32 f;
	    int j;

	    f = *pF++;
	    for (j = 0; j < 32; j += 4) {
	      if (f & k_tof_filt_non_zero_mask) {
		if (!(f & k_tof_filt_1_mask)) {
		  tmp = pTmp->q;
		  pTmp->q = pTmp->i;
		  pTmp->i = -tmp;
		}
		if (f & k_tof_filt_neg_mask) {
		  pTmp->i = -pTmp->i;
		  pTmp->q = -pTmp->q;
		}
	      } else {
		pTmp->i = 0;
		pTmp->q = 0;
	      }
	      pTmp++;
	      f = f >> 4;
	    }
	  }
	  if (!k) {
	    for (i = 0; i < (n - 2*8*nF); i++) {
	      pTmp->i = 0;
	      pTmp->q = 0;
	      pTmp++;
	    }
	  }
	}

#ifdef TOF_SEQ_20MHz_BW_512IFFT
	if (!seq) {
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			pTmp = pIn;
			bcopy(pTmp, pIn, (2*32*sizeof(uint32)));
			bcopy((pTmp+32), (pIn+480), (2*32*sizeof(uint32)));
			memset(&pIn[32], 0, (448 * sizeof(cint32)));
			n = k_tof_seq_ifft_20MHz;
		}
	}
#endif

	wlapi_fft(pi->sh->physhim, n, (void*)pIn, (void*)pIn, 2);

	if (s2) {
	  int32 *pTmpIQ, m2 = (int32)(1 << (s2 - 1));
	  pTmpIQ = (int32*)pIn;
	  for (i = 0; i < 2*n; i++) {
	    tmp = ((k2*(*pTmpIQ) + m2) >> s2);
	    *pTmpIQ++ = tmp;
	  }
	}
}

static void
wlc_tof_seq_write_shm_acphy(phy_info_t *pi, int len, uint16 offset, uint16* p)
{
	uint16 p_shm = pi->u.pi_acphy->tofi->tof_shm_ptr;

	while (len-- > 0) {
	  wlapi_bmac_write_shm(pi->sh->physhim, (p_shm + offset), *p);
	  p++;
	  offset += 2;
	}
}

static void
phy_ac_tof_seq_upd_dly(phy_type_tof_ctx_t *ctx,  bool tx, uint8 core, bool mac_suspend)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	uint8 stall_val;
	int i;
	uint16 *pSrc;
	uint16 shm[18];
	uint16 wrds_per_us, rfseq_trigger;
	bool  suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	if (mac_suspend) {
		if (!suspend)
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);
		stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
		ACPHY_DISABLE_STALL(pi);

	}

	/* Setup delays and triggers */
	if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev))
	  wrds_per_us = ((3*tofi->tof_sc_FS) >> 2);
	else
	  wrds_per_us = tofi->tof_sc_FS;
	pSrc = shm;
	rfseq_trigger = READ_PHYREG(pi, RfseqTrigger) &
	        ACPHY_RfseqTrigger_en_pkt_proc_dcc_ctrl_MASK(0);
	for (i = 0; i < 3; i++) {
	  *pSrc++ = tofi->tof_ucode_dlys_us[(tx ? 1 : 0)][i] * wrds_per_us;
	  if (i ^ tx)
	    *pSrc = rfseq_trigger | ACPHY_RfseqTrigger_ocl_shut_off_MASK(0);
	  else
	    *pSrc = rfseq_trigger | ACPHY_RfseqTrigger_ocl_reset2rx_MASK(0);
	  pSrc++;
	}
	shm[k_tof_seq_shm_dly_len-1] = rfseq_trigger; /* Restore value */
	shm[k_tof_seq_shm_dly_len] = ACPHY_PhyStatsGainInfo0(0) + 0x200*core;

	wlc_tof_seq_write_shm_acphy(pi,
	                            (k_tof_seq_shm_dly_len + 1),
	                            k_tof_seq_shm_dly_offset,
	                            shm);
	if (mac_suspend) {
		ACPHY_ENABLE_STALL(pi, stall_val);
		wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
		if (!suspend)
			wlapi_enable_mac(pi->sh->physhim);
	}
}

static int
phy_ac_tof_seq_setup(phy_info_t *pi, bool enter, bool tx, uint8 core)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	uint8 stall_val;
	uint16 *pSrc, *pEnd;
	uint16 shm[18];
	uint16 tof_rfseq_bundle_offset, rx_ctrl, rfseq_mode, rfseq_offset, mask;
	int i;
	bool  suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	cint32* pSeq;
	uint16 tof_seq_n;
	uint16 tof_rfseq_event;

	if (!enter)
		return BCME_OK;


	if (((tofi->tof_rfseq_bundle_offset >= k_tof_rfseq_bundle_base) && !TINY_RADIO(pi)) ||
	    ((tofi->tof_rfseq_bundle_offset >= k_tof_rfseq_tiny_bundle_base) && TINY_RADIO(pi)) ||
	    (READ_PHYREGFLD(pi, OCLControl1, ocl_mode_enable)))
	  return BCME_ERROR;

	tof_seq_n = (1 << tofi->tof_seq_log2_n);

	WRITE_PHYREG(pi, sampleLoopCount, 0xffff);
	WRITE_PHYREG(pi, sampleDepthCount, (tof_seq_n - 1));
	if (tofi->tof_setup_done)
	  return BCME_OK;

	pSeq = phy_malloc_fatal(pi, tof_seq_n * sizeof(*pSeq));

	tofi->tof_tx = tx;
	tofi->tof_core = core;
	mask = (1 << tofi->tof_core);

	if (!suspend)
	  wlapi_suspend_mac_and_wait(pi->sh->physhim);
	wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	/* Setup rfcontrol sequence for tx_on / tx_off events */
	tof_rfseq_bundle_offset = tofi->tof_rfseq_bundle_offset;
	if (TINY_RADIO(pi)) {
	  pSrc = (uint16*)k_tof_seq_tiny_tbls;
	  pEnd = pSrc + sizeof(k_tof_seq_tiny_tbls)/sizeof(uint16);
	} else {
	  pSrc = (uint16*)k_tof_seq_tbls;
	  pEnd = pSrc + sizeof(k_tof_seq_tbls)/sizeof(uint16);
	}
	while (pSrc != pEnd) {
	  uint32 id, width, len, tbl_len, offset;
	  uint16* pTblData;

	  id = (uint32)*pSrc++;
	  offset = (uint32)*pSrc++;
	  len = (uint32)*pSrc++;
	  if (id == ACPHY_TBL_ID_RFSEQBUNDLE) {
	    width = 48;
	    tbl_len = len / 3;
	    for (i = 0; i < len; i++) {
	      shm[i] = pSrc[i];
	      if ((offset >= 0x10) && ((i % 3) == 0)) {
		shm[i] = (shm[i] & ~7) | mask;
	      }
	    }
	    pTblData = shm;
	  } else {
	    width = 16;
	    tbl_len = len;
	    pTblData = pSrc;
	  }
	  wlc_phy_table_write_acphy(pi, id, tbl_len, offset, width, (void*)pTblData);
	  pSrc += len;
	}
	tofi->tof_rfseq_bundle_offset = tof_rfseq_bundle_offset;
	/* Set rx gain during loopback -- cant use rf bundles due to hw bug in 4350 */
	if (TINY_RADIO(pi))
	  shm[0] = k_tof_seq_rx_loopback_gain_tiny;
	else
	  shm[0] = k_tof_seq_rx_loopback_gain;
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
	                          (k_tof_seq_rfseq_gain_base + core*0x10
	                           + k_tof_seq_rfseq_loopback_gain_offset),
	                           16, (void*)&shm[0]);
	/* Setup shm which tells ucode sequence of phy reg writes */
	/* before/after triggering sequence */
		rx_ctrl = READ_PHYREG(pi, RxControl);
	rfseq_mode = (READ_PHYREG(pi, RfseqMode) &
	              ~(ACPHY_RfseqMode_CoreActv_override_MASK(0) |
	                ACPHY_RfseqMode_Trigger_override_MASK(0)));
	rfseq_offset = wlc_tof_rfseq_event_offset(pi, k_tof_rfseq_tx_gain_event, shm);
	rfseq_offset += 1;
	tof_rfseq_event = shm[rfseq_offset];
		wlc_tof_seq_write_shm_acphy(pi,
			k_tof_seq_shm_setup_regs_len,
			k_tof_seq_shm_setup_regs_offset,
			(uint16*)k_tof_seq_ucode_regs);
	bzero((void*)shm, sizeof(shm));
		shm[0] = rx_ctrl | ACPHY_RxControl_dbgpktprocReset_MASK(0); /* first setup */
	shm[1] = ACPHY_TBL_ID_RFSEQBUNDLE;
	if (TINY_RADIO(pi))
	  shm[2] = k_tof_seq_tiny_rx_fem_gain_offset;
	else
	  shm[2] = k_tof_seq_rx_fem_gain_offset;
	shm[3] = k_tof_seq_fem_gains[0] | mask;
	shm[6] = ACPHY_TBL_ID_RFSEQ; /* first restore */
	shm[7] = k_tof_seq_rfseq_gain_base + core*0x10 + k_tof_seq_rfseq_rx_gain_offset;
#ifdef TOF_DBG
	if (TINY_RADIO(pi))
	  shm[8] = k_tof_seq_rx_gain_tiny;
	else
	  shm[8] = k_tof_seq_rx_gain;
#endif
	shm[9] = rfseq_offset;
	shm[10] = k_tof_rfseq_end_event;
	shm[11] = (rfseq_mode | ACPHY_RfseqMode_CoreActv_override_MASK(0));
	shm[12] = ACPHY_sampleCmd_start_MASK(0);
	shm[13] = (rfseq_mode |
	           ACPHY_RfseqMode_Trigger_override_MASK(0) |
	           ACPHY_RfseqMode_CoreActv_override_MASK(0));
	shm[14] = ((~core) & 3) << ACPHY_SlnaControl_SlnaCore_SHIFT(0);
	shm[15] = ACPHY_AdcDataCollect_adcDataCollectEn_MASK(0); /* last setup */
	wlc_tof_seq_write_shm_acphy(pi,
	                            k_tof_seq_shm_setup_vals_len,
	                            k_tof_seq_shm_setup_vals_offset,
	                            shm);
	shm[10] = tof_rfseq_event;
	shm[11] = rfseq_mode;
	shm[12] = ACPHY_sampleCmd_stop_MASK(0);
	shm[13] = rfseq_mode;
	shm[14] = READ_PHYREG(pi, SlnaControl);
	shm[15] = 0;
	shm[16] = rx_ctrl; /* last restore */
	wlc_tof_seq_write_shm_acphy(pi,
	                            k_tof_set_shm_restr_vals_len,
	                            k_tof_set_shm_restr_vals_offset,
	                            &shm[9]);

	/* Setup sample play */
	phy_ac_tof_mf(pi, tof_seq_n, pSeq, TRUE,
	               0, 0, 0,
	               k_tof_seq_in_scale, k_tof_seq_out_scale, k_tof_seq_out_shift);
#ifdef TOF_TEST_TONE
	for (i = 0; i < tof_seq_n; i++) {
	  const int32 v[16] = {96, 89, 68, 37, 0, -37, -68, -89, -96, -89, -68, -37, 0, 37, 68, 89};
	  pSeq[i].i = v[i & 0xf];
	  pSeq[i].q = v[(i-4) & 0xf];
	}
#endif
	wlc_phy_loadsampletable_acphy(pi, pSeq, tof_seq_n, FALSE, TRUE);

#ifdef TOF_DBG
	printf("SETUP CORE %d\n", core);
#endif

	phy_mfree(pi, pSeq, tof_seq_n * sizeof(*pSeq));

	ACPHY_ENABLE_STALL(pi, stall_val);
	wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
	if (!suspend)
	  wlapi_enable_mac(pi->sh->physhim);

	tofi->tof_setup_done = TRUE;
	return BCME_OK;
}

static int phy_ac_seq_ts(phy_type_tof_ctx_t *ctx, int n, cint32* p_buffer, int tx,
                     int cfo, int adj, void* pparams, int32* p_ts, int32* p_seq_len, uint32* p_raw)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	int16 sc_ptr;
	int32 ts[2], dT;
	int i, n_out, a, b;
	uint16 tof_seq_n;
	uint16 tmp_tof_sc_Fs;
	int32 tof_seq_M;

	tof_seq_n = (1 <<  tofi->tof_seq_log2_n);

	if (n < tof_seq_n)
		return BCME_ERROR;

	a = READ_PHYREGCE(pi, Core1RxIQCompA, tofi->tof_core);
	b = READ_PHYREGCE(pi, Core1RxIQCompB, tofi->tof_core);
	if (a > 511)
	  a -= 1024;
	if (b > 511)
	  b -= 1024;

	for (i = 0; i < 2; i++) {
		n_out = phy_ac_tof_sc_read(pi, TRUE, n, p_buffer,
			-(tofi->tof_ucode_dlys_us[tx][3+i]),
			k_tof_seq_sc_start, &sc_ptr);

	  if (n_out != n)
	    return BCME_ERROR;
#ifdef TOF_DBG
	  if (p_raw && (2*(n_out + 1) <= k_tof_collect_Hraw_size)) {
	    int j;
	    for (j = 0; j < n_out; j++) {
	      *p_raw++ = (uint32)(p_buffer[j].i & 0xffff) |
	              ((uint32)(p_buffer[j].q & 0xffff) << 16);
	    }
	    *p_raw++ = (uint32)((int32)a & 0xffff) | (uint32)(((int32)b & 0xffff) << 16);
	  }
#endif
#if defined(TOF_TEST_TONE)
	  ts[i] = 0;
	  tmp_tof_sc_Fs = tofi->tof_sc_FS;
#else
	  uint16 tmp_seq_log2_n;
	  phy_ac_tof_mf(pi, tof_seq_n, p_buffer, FALSE,
	                 a, b, (i)?cfo:0,
	                 k_tof_mf_in_shift,
	                 k_tof_mf_out_scale,
	                 k_tof_mf_out_shift);
#ifdef TOF_SEQ_20MHz_BW_512IFFT
	  if (CHSPEC_IS20(pi->radio_chanspec)) {
		  tmp_seq_log2_n = k_tof_seq_n_20MHz;
		  tmp_tof_sc_Fs = 160;
		  tof_seq_n = (1 << tmp_seq_log2_n);
	  }
	  else
#endif
	  {
		  tmp_seq_log2_n = tofi->tof_seq_log2_n;
		  tmp_tof_sc_Fs = tofi->tof_sc_FS;
	  }

	  if (wlapi_tof_pdp_ts(tmp_seq_log2_n, (void*)p_buffer, tmp_tof_sc_Fs, i, pparams,
	                       &ts[i], NULL) != BCME_OK)
	    return BCME_ERROR;

#endif /* defined(TOF_TEST_TONE) */
	}

	tof_seq_M = ((10000 * tof_seq_n)/ tmp_tof_sc_Fs);

	ts[0] += adj;
	dT = (ts[tx] - ts[tx ^ 1]);
	if (dT < 0)
	  dT += tof_seq_M;
	*p_ts = dT;
	*p_seq_len = tof_seq_M;
	return BCME_OK;
}

#if defined(TOF_DBG)
static int
phy_ac_tof_dbg(phy_type_tof_ctx_t *ctx, int arg)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	cint32 buf[k_tof_dbg_sc_delta];
	int16  p, p_start;
	int i = 0, n = 0;
	bool  suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	uint8 stall_val;

	if (!suspend)
	  wlapi_suspend_mac_and_wait(pi->sh->physhim);
	wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	p = k_tof_seq_sc_start;
	if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev))
	  p += ((k_tof_dbg_sc_delta>>2)*arg*3);
	else
	  p += (k_tof_dbg_sc_delta*arg);
	if (arg >= 255) {
	  int j;
	  uint16 v, offset = 0;
	  uint16 bundle[3*16];
	  const uint16 bundle_addr[] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50};
	  for (i = 0; i < sizeof(bundle_addr)/sizeof(uint16); i++) {
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE,
	                             8, bundle_addr[i]+8, 48, (void*)bundle);
	    for (j = 0; j < 16; j++) {
	      printf("RFBUNDLE 0x%x : 0x%04x%04x%04x\n",
	             (bundle_addr[i]+j),
	             bundle[3*j+2], bundle[3*j+1], bundle[3*j+0]);
	    }
	  }
	  for (i = 0; i < 16; i++) {
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x290 + i), 16, (void*)&bundle[0]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x320 + i), 16, (void*)&bundle[1]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x260 + i), 16, (void*)&bundle[2]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x2f0 + i), 16, (void*)&bundle[3]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x000 + i), 16, (void*)&bundle[4]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x070 + i), 16, (void*)&bundle[5]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x010 + i), 16, (void*)&bundle[6]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x080 + i), 16, (void*)&bundle[7]);
	    printf("RFSEQ RXs 0x%04x 0x%04x TXs 0x%04x 0x%04x TX 0x%04x 0x%04x RX 0x%04x 0x%04x\n",
	           bundle[0], bundle[1], bundle[2], bundle[3], bundle[4], bundle[5], bundle[6],
	           bundle[7]);
	  }
	  for (i = 0; i < 3; i++) {
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
	                             (k_tof_seq_rfseq_gain_base + i*0x10 +
	                              k_tof_seq_rfseq_loopback_gain_offset), 16, (void*)&bundle[0]);
	    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
	                             (k_tof_seq_rfseq_gain_base + i*0x10 +
	                              k_tof_seq_rfseq_rx_gain_offset), 16, (void*)&bundle[1]);
	    printf("GC%d  LBK 0x%04x RX 0x%04x\n", i, bundle[0], bundle[1]);
	  }
	  offset = k_tof_seq_shm_setup_regs_offset;
	  n = 46;
	  while (n-- > 0) {
	    v = wlapi_bmac_read_shm(pi->sh->physhim, (tofi->tof_shm_ptr + offset));
	    printf("SHM %d 0x%04x\n", ((offset - k_tof_seq_shm_setup_regs_offset) >> 1), v);
	    offset += 2;
	  }
	  n = 1;
	} else if (arg == 252) {
	  printf("MAC 0x%x STRT %d STP %d CUR %d\n",
	         R_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param),
	         R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectStartPtr),
	         R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectStopPtr),
	         R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectCurPtr));
	  n = 1;
	} else {
	  if (arg == 0) {
	    printf("MAC 0x%x STRT %d STP %d CUR %d ",
	           R_REG(pi->sh->osh, &pi->regs->psm_phy_hdr_param),
	           R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectStartPtr),
	           R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectStopPtr),
	           R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectCurPtr));
	  }
	  if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev))
	    n = (((int)R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectCurPtr) - p)/3) << 2;
	  else
	    n = ((int)R_REG(pi->sh->osh, &pi->regs->PHYREF_SampleCollectCurPtr) - p);
	  if (n > k_tof_dbg_sc_delta)
	    n = k_tof_dbg_sc_delta;
	  if (n > 0) {
	    arg = phy_ac_tof_sc_read(pi, TRUE, n, buf, p, k_tof_seq_sc_start, &p_start);
	    i = 0;
	    while (i < arg) {
	      if (buf[i].i > 2047)
		buf[i].i -= 4096;
	      if (buf[i].q > 2047)
		buf[i].q -= 4096;
	      printf("SD %4d %d %d\n", p_start, buf[i].i, buf[i].q);
	      i++;
	      p_start++;
	    }
	  } else {
	    printf("\n");
	  }
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
	wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
	if (!suspend)
	  wlapi_enable_mac(pi->sh->physhim);

	return (n > 0) ? 1 : 0;
}
#endif /* defined(TOF_DBG) */

#endif /* WL_PROXD_SEQ */

static int phy_ac_tof(phy_type_tof_ctx_t *ctx, bool enter, bool tx, bool hw_adj,
	bool seq_en, int core)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	phy_info_acphy_t *pi_ac = tofi->aci;
	bool change = pi_ac->tof_active != enter;
	bool  suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);
	int retval = BCME_OK;

	if (change) {
#ifdef WL_PROXD_SEQ
	  if (seq_en) {
	    retval = phy_ac_tof_seq_setup(pi, enter, tx, core);

	  } else
#endif
	  {
		  tofi->tof_setup_done = FALSE;
	    if (!suspend)
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	    if (enter) {
		/* Save state fdiqcomp and disable */
		tofi->tof_rx_fdiqcomp_enable = (uint8)READ_PHYREGFLD(pi, rxfdiqImbCompCtrl,
			rxfdiqImbCompEnable);
		tofi->tof_tx_fdiqcomp_enable = (uint8)READ_PHYREGFLD(pi, fdiqImbCompEnable,
			txfdiqImbCompEnable);
		MOD_PHYREG(pi, rxfdiqImbCompCtrl, rxfdiqImbCompEnable, 0);
		MOD_PHYREG(pi, fdiqImbCompEnable, txfdiqImbCompEnable, 0);
		if (hw_adj) {
			/* Save channel smoothing state and enable special  mode */
			phy_ac_chanmgr_save_smoothing(pi_ac->chanmgri, &tofi->tof_smth_enable,
				&tofi->tof_smth_dump_mode);
			wlc_phy_smth(pi, SMTH_ENABLE, SMTH_TIMEDUMP_AFTER_IFFT);
			pi_ac->tof_smth_forced = TRUE;
		}
	    } else {
		/* Restore fdiqcomp state */
		MOD_PHYREG(pi, rxfdiqImbCompCtrl, rxfdiqImbCompEnable,
			tofi->tof_rx_fdiqcomp_enable);
		MOD_PHYREG(pi, fdiqImbCompEnable, txfdiqImbCompEnable,
			tofi->tof_tx_fdiqcomp_enable);
		if (pi_ac->tof_smth_forced) {
			/* Restore channel smoothing state */
			pi_ac->tof_smth_forced = FALSE;
			wlc_phy_smth(pi, tofi->tof_smth_enable, tofi->tof_smth_dump_mode);
		}
	}

		wlc_phy_resetcca_acphy(pi);
		if (!suspend)
			wlapi_enable_mac(pi->sh->physhim);
	}
}
	pi_ac->tof_active = enter;
	return retval;
}

/* Unpacks floating point to fixed point for further processing */
/* Fixed point format:

	A.fmt = TRUE
			sign      real         sign       image             exp
			|-|--------------------||-|--------------------||----------|
	size:            nman                   nman              nexp

	B.fmt = FALSE
	         exp     sign		 image                  real
			|----------||-|-||-------------------||--------------------|
	size:		 nexp	 1 1          nman					nman

	When Hi is NULL, we return "real * real + image * image" in Hr array, otherwise
	real and image is save in Hr and Hi.

	When autoscale is TRUE, calculate the max shift to save fixed point value into uint32
*/


/*
	H holds upacked 32 bit data when the function is called.
	H and Hout could partially overlap.
	H and h can not overlap
*/

static int
wlc_unpack_float_acphy(int nbits, int autoscale, int shft,
	int fmt, int nman, int nexp, int nfft, uint32* H, cint32* Hout, int32* h)
{
	int e_p, maxbit, e, i, pwr_shft = 0, e_zero, sgn;
	int n_out, e_shift;
	int8 He[256];
	int32 vi, vq, *pOut;
	uint32 x, iq_mask, e_mask, sgnr_mask, sgni_mask;

	/* when fmt is TRUE, the size nman include its sign bit */
	/* so need to minus one to get value mask */
	if (fmt)
		iq_mask = (1<<(nman-1))- 1;
	else
		iq_mask = (1<<nman)- 1;

	e_mask = (1<<nexp)-1;	/* exp part mask */
	e_p = (1<<(nexp-1));	/* max abs value of exp */

	if (h) {
		/* Set pwr_shft to make sure that square sum can be hold by uint32 */
		pwr_shft = (2*nman + 1 - 31);
		if (pwr_shft < 0)
			pwr_shft = 0;
		pwr_shft = (pwr_shft + 1)>>1;
		sgnr_mask = 0;	/* don't care sign for square sum */
		sgni_mask = 0;
		e_zero = -(2*(nman - pwr_shft) + 1);
		pOut = (int32*)h;
		n_out = nfft;
		e_shift = 0;
	} else {
		/* Set the location of sign bit */
		if (fmt) {
			sgnr_mask = (1 << (nexp + 2*nman - 1));
			sgni_mask = (sgnr_mask >> nman);
		} else {
			sgnr_mask = (1 << 2*nman);
			sgni_mask = (sgnr_mask << 1);
		}
		e_zero = -nman;
		pOut = (int32*)Hout;
		n_out = (nfft << 1);
		e_shift = 1;
	}

	maxbit = -e_p;
	for (i = 0; i < nfft; i++) {
		/* get the real, image and exponent value */
		if (fmt) {
			vi = (int32)((H[i] >> (nexp + nman)) & iq_mask);
			vq = (int32)((H[i] >> nexp) & iq_mask);
			e =   (int)(H[i] & e_mask);
		} else {
			vi = (int32)(H[i] & iq_mask);
			vq = (int32)((H[i] >> nman) & iq_mask);
			e = (int32)((H[i] >> (2*nman + 2)) & e_mask);
		}

		/* adjust exponent */
		if (e >= e_p)
			e -= (e_p << 1);

		if (h) {
			/* calculate square sum of real and image data */
			vi = (vi >> pwr_shft);
			vq = (vq >> pwr_shft);
			h[i] = vi*vi + vq*vq;
			vq = 0;
			e = 2*(e + pwr_shft);
		}

		He[i] = (int8)e;

		/* auto scale need to find the maximus exp bits */
		x = (uint32)vi | (uint32)vq;
		if (autoscale && x) {
			uint32 m = 0xffff0000, b = 0xffff;
			int s = 16;

			while (s > 0) {
				if (x & m) {
					e += s;
					x >>= s;
				}
				s >>= 1;
				m = (m >> s) & b;
				b >>= s;
			}
			if (e > maxbit)
				maxbit = e;
		}

		if (!h) {
			if (H[i] & sgnr_mask)
				vi |= k_tof_unpack_sgn_mask;
			if (H[i] & sgni_mask)
				vq |= k_tof_unpack_sgn_mask;
			Hout[i].i = vi;
			Hout[i].q = vq;
		}
	}

	/* shift bits */
	if (autoscale)
		shft = nbits - maxbit;

	/* scal and sign */
	for (i = 0; i < n_out; i++) {
		e = He[(i >> e_shift)] + shft;
		vi = *pOut;
		sgn = 1;
		if (!h && (vi & k_tof_unpack_sgn_mask)) {
			sgn = -1;
			vi &= ~k_tof_unpack_sgn_mask;
		}
		/* trap the zero case */
		if (e < e_zero) {
			vi = 0;
		} else if (e < 0) {
			e = -e;
			vi = (vi >> e);
		} else {
			vi = (vi << e);
		}
		*pOut++ = (int32)sgn*vi;
	}

	return shft;
}

/* Get channel frequency response for deriving 11v rx timestamp */
static int
phy_ac_chan_freq_response(phy_type_tof_ctx_t *ctx,
	int len, int nbits, cint32* H, uint32* Hraw)
{
	uint32 *pTmp, *pIn;
	int i, i_l, i_r, n1, n2, n3, nfft, nfft_over_2;
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;


	if ((len != TOF_NFFT_20MHZ) && (len != TOF_NFFT_40MHZ) && (len != TOF_NFFT_80MHZ))
		return BCME_ERROR;

	ASSERT(sizeof(cint32) == 2*sizeof(int32));

	pTmp = (uint32*)H;
	pIn = (uint32*)H + len;
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_CORE0CHANESTTBL, len, 0,
			CORE0CHANESTTBL_TABLE_WIDTH, pTmp);
#ifdef TOF_DBG
#if defined(k_tof_collect_Hraw_size)
	if (Hraw && (len <= k_tof_collect_Hraw_size))
	  bcopy((void*)pTmp, (void*)Hraw, len*sizeof(uint32));
#else
	if (Hraw) {
		bcopy((void*)pTmp, (void*)Hraw, len*sizeof(uint32));
	}

#endif
/* store raw data for log collection */
#endif /* TOF_DBG */
	/* reorder tones */
		nfft = len;
		nfft_over_2 = (len >> 1);
		if (CHSPEC_IS80(pi->radio_chanspec) ||
			PHY_AS_80P80(pi, pi->radio_chanspec)) {
			i_l = 122;
			i_r = 2;
		} else if (CHSPEC_IS160(pi->radio_chanspec)) {
			i_l = 122;
			i_r = 2;
			ASSERT(0);
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			i_l = 58;
			i_r = 2;
		} else {
			/* for legacy this is 26, for vht-20 this is 28 */
			i_l = 28;
			i_r = 1;
		}
		memset((void *)pIn, 0, len * sizeof(int32));
		for (i = i_l; i >= i_r; i--) {
			n1 = nfft_over_2 - i;
			n2 = nfft_over_2 + i;
			n3 = nfft - i;
			pIn[n1] = pTmp[n3];
			pIn[n2] = pTmp[i];
	}

	if (ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_3(pi->pubpi->phy_rev)) {
		int32 chi, chq;

		for (i = 0; i < len; i++) {
			chi = ((int32)pIn[i] >> CORE0CHANESTTBL_INTEGER_DATA_SIZE) &
				CORE0CHANESTTBL_INTEGER_DATA_MASK;
			chq = (int32)pIn[i] & CORE0CHANESTTBL_INTEGER_DATA_MASK;
			if (chi >= CORE0CHANESTTBL_INTEGER_MAXVALUE)
				chi = chi - (CORE0CHANESTTBL_INTEGER_MAXVALUE << 1);
			if (chq >= CORE0CHANESTTBL_INTEGER_MAXVALUE)
				chq = chq - (CORE0CHANESTTBL_INTEGER_MAXVALUE << 1);
			H[i].i = chi;
			H[i].q = chq;
		}
	} else if (ACMAJORREV_2(pi->pubpi->phy_rev) ||
	           ACMAJORREV_4(pi->pubpi->phy_rev)) {
		wlc_unpack_float_acphy(nbits, UNPACK_FLOAT_AUTO_SCALE, 0,
			CORE0CHANESTTBL_FLOAT_FORMAT, CORE0CHANESTTBL_REV2_DATA_SIZE,
			CORE0CHANESTTBL_REV2_EXP_SIZE, len, pIn, H, NULL);
	} else if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
		wlc_unpack_float_acphy(nbits, UNPACK_FLOAT_AUTO_SCALE, 0,
			CORE0CHANESTTBL_FLOAT_FORMAT, CORE0CHANESTTBL_REV0_DATA_SIZE,
			CORE0CHANESTTBL_REV0_EXP_SIZE, len, pIn, H, NULL);
	} else {
		return BCME_UNSUPPORTED;
	}

	return BCME_OK;
}

/* Get mag sqrd channel impulse response(from channel smoothing hw) to derive 11v rx timestamp */
static int
phy_ac_chan_mag_sqr_impulse_response(phy_type_tof_ctx_t *ctx, int frame_type,
	int len, int offset, int nbits, int32* h, int* pgd, uint32* hraw, uint16 tof_shm_ptr)
{
	uint8 stall_val;
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	phy_info_acphy_t *pi_ac = tofi->aci;

	int N, s0, s1, m_mask, idx, i, j, l, m, i32x4, i4x2, tbl_offset, gd, n;
	uint32 *pdata, *ptmp, data_l, data_h;
	uint16 chnsm_status0;
	uint16 channel_smoothing = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_37(pi->pubpi->phy_rev)) ?
		AC2PHY_TBL_ID_CHANNELSMOOTHING_1x1 : ACPHY_TBL_ID_CHANNELSMOOTHING_1x1;

	idx = -offset;

	if (ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
		/* Only 11n/acphy frames */
		if (frame_type < 2)
			return BCME_UNSUPPORTED;
	} else if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
		/* 11n/acphy frames + 20MHz legacy frames */
		if ((frame_type < 2) && !((frame_type == 1) && (pi_ac->curr_bw == 0)))
			return BCME_UNSUPPORTED;
	} else {
		return BCME_UNSUPPORTED;
	}

	if (CHSPEC_IS80(pi->radio_chanspec)) {
		N = TOF_NFFT_80MHZ;
		s0 = 2 + TOF_BW_80MHZ_INDEX;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		N = TOF_NFFT_40MHZ;
		s0 = 2 + TOF_BW_40MHZ_INDEX;
	} else if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
		N = TOF_NFFT_20MHZ;
		s0 = 2 + TOF_BW_20MHZ_INDEX;
	} else
		return BCME_ERROR;

	chnsm_status0 = wlapi_bmac_read_shm(pi->sh->physhim, tof_shm_ptr +
		M_TOF_CHNSM_0_OFFSET(pi));
	gd = (int)((chnsm_status0 >>
		ACPHY_chnsmStatus0_group_delay_SHIFT(pi->pubpi->phy_rev)) & 0xff);
	if (gd > 127)
		gd -= 256;

	if (ACMAJORREV_3(pi->pubpi->phy_rev))
		idx += gd; /* Impulse response is not shited, 0 is @ gd */

	*pgd = Q1_NS * gd; /* gd in Q1 ns */

	if (len > N)
		len = N;

	s1 = s0 + 2;
	m_mask = (1 << s1) - 1;

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	if (stall_val == 0)
		ACPHY_DISABLE_STALL(pi);

	phy_utils_write_phyreg(pi, ACPHY_TableID(pi->pubpi->phy_rev), channel_smoothing);

	n = len;
	ptmp = (uint32*)h + len;
	pdata = ptmp;
	while (n > 0) {
		idx = idx & (N - 1);
		i = (idx >> 2);
		j = (idx & 3);
		l = i + j;
		i32x4 = (i & 0xc);
		if (N == TOF_NFFT_80MHZ) {
			m  = (i32x4 + (i << 2) + (i32x4 >> 2)) & 0xf;
			m += (i ^ ((i << 1) & 0x20)) & 0x30;
			l += (i >> 4) + (i >> 2);
		} else if (N == TOF_NFFT_40MHZ) {
			i4x2 = (i >> 3) & 2;
			m  = ((0x1320 >> i32x4) & 0xf) << 3;
			m += ((((i >> 2) & 1) + i + i4x2) & 3) << 1;
			m += (i4x2 >> 1);
			l += (0x130 >> i32x4) + i4x2;
		} else {
			m  = ((i & 3) + (i32x4 ^ (i32x4 << 1))) & 0xf;
			l += (i >> 2);
		}
		l = (l & 3);
		tbl_offset = ((m + (l << s0)) & m_mask) + (l << s1) + CHANNELSMOOTHING_DATA_OFFSET;
		phy_utils_write_phyreg(pi, ACPHY_TableOffset(pi->pubpi->phy_rev),
			(uint16) tbl_offset);
		data_l = (uint32)phy_utils_read_phyreg(pi, ACPHY_TableDataWide(pi->pubpi->phy_rev));
		data_h = (uint32)phy_utils_read_phyreg_wide(pi);
		*pdata++ = (data_h << 16) | (data_l & 0xffff);
		idx++;
		n--;
	};

	if (stall_val == 0)
		ACPHY_ENABLE_STALL(pi, stall_val);
#ifdef TOF_DBG
#if defined(k_tof_collect_Hraw_size)
	if (hraw && ((len + 1) <= k_tof_collect_Hraw_size)) {
		bcopy((void*)ptmp, (void*)hraw, len*sizeof(uint32));
		/* store raw data for log collection */
		hraw[len] = (uint32)chnsm_status0;
	}
#else
	if (hraw) {
		bcopy((void*)ptmp, (void*)hraw, len*sizeof(uint32));
		hraw[len] = (uint32)chnsm_status0;
	}
#endif
#endif /* TOF_DBG */
	wlc_unpack_float_acphy(nbits, UNPACK_FLOAT_AUTO_SCALE, 0, CHANNELSMOOTHING_FLOAT_FORMAT,
		CHANNELSMOOTHING_FLOAT_DATA_SIZE, CHANNELSMOOTHING_FLOAT_EXP_SIZE,
		len, ptmp, NULL, h);

	return BCME_OK;
}

/* get rxed frame type, bandwidth and rssi value */
static int phy_ac_tof_info(phy_type_tof_ctx_t *ctx,  int* p_frame_type, int* p_frame_bw,
	int* p_cfo, int8* p_rssi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	uint16 status0, status2, status5;
	int frame_type, frame_bw, rssi, cfo;

	status0 = (int)READ_PHYREG(pi, RxStatus0) & 0xffff;
	status2 = (int)READ_PHYREG(pi, RxStatus2);
	status5 = (int)READ_PHYREG(pi, RxStatus5);

	frame_type = status0 & PRXS0_FT_MASK;
	frame_bw = status0 & PRXS0_ACPHY_SUBBAND_MASK;
	if (frame_bw == (PRXS_SUBBAND_80 << PRXS0_ACPHY_SUBBAND_SHIFT))
	  frame_bw = TOF_BW_80MHZ_INDEX | (frame_bw << 16);
	else if ((frame_bw == (PRXS_SUBBAND_40L << PRXS0_ACPHY_SUBBAND_SHIFT)) ||
		(frame_bw == (PRXS_SUBBAND_40U << PRXS0_ACPHY_SUBBAND_SHIFT)))
	  frame_bw = TOF_BW_40MHZ_INDEX | (frame_bw << 16);
	else
	  frame_bw = TOF_BW_20MHZ_INDEX | (frame_bw << 16);
	rssi = ((int)status2 >> 8) & 0xff;
	if (rssi > 127)
		rssi -= 256;

	cfo = ((int)status5 & 0xff);
	if (cfo > 127)
		cfo -= 256;
	cfo = cfo * 2298;

	*p_frame_type = frame_type;
	*p_frame_bw = frame_bw;
	*p_rssi = (int8)rssi;
	*p_cfo = cfo;

	return BCME_OK;
}

/* turn on classification to receive frames */
static void phy_ac_tof_cmd(phy_type_tof_ctx_t *ctx,  bool seq)
{
	uint16 class_mask;
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
#ifdef WL_PROXD_SEQ

	if (seq) {
		uint16 tof_seq_fem_gains[2],  mask = (1 << tofi->tof_core);

		MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, mask);
		tof_seq_fem_gains[0] = k_tof_seq_fem_gains[0] | mask;
		tof_seq_fem_gains[1] = k_tof_seq_fem_gains[1] | mask;
		wlc_tof_seq_write_shm_acphy(pi, 1, k_tof_seq_shm_fem_radio_hi_gain_offset,
			(uint16*)tof_seq_fem_gains);
		wlc_tof_seq_write_shm_acphy(pi, 1, k_tof_seq_shm_fem_radio_lo_gain_offset,
			(uint16*)tof_seq_fem_gains+1);
			phy_ac_tof_sc(pi, TRUE, k_tof_seq_sc_start, k_tof_seq_sc_stop, 0);
	}
#endif /* WL_PROXD_SEQ */

	class_mask = CHSPEC_IS2G(pi->radio_chanspec) ? 7 : 6; /* No bphy in 5g */
	wlc_phy_classifier_acphy(pi, ACPHY_ClassifierCtrl_classifierSel_MASK, class_mask);
}

/* get K value for initiator or target  */
static
uint16 *phy_ac_tof_kvalue_tables(phy_info_t *pi, phy_ac_tof_info_t *tofi,
	chanspec_t chanspec, int32* ki, int32* kt, int32* kseq)
{
	uint16 const *kvalueptr = NULL;

	if (CHSPEC_IS80(chanspec)) {
		*ki = tofi->proxd_ki[0];
		*kt = tofi->proxd_kt[0];
	}
	else if (CHSPEC_IS40(chanspec)) {
		*ki = tofi->proxd_ki[1];
		*kt = tofi->proxd_kt[1];
	}
	else if (CHSPEC_IS20_5G(chanspec)) {
		*ki = tofi->proxd_ki[2];
		*kt = tofi->proxd_kt[2];
	}
	else {
		*ki = tofi->proxd_ki[3];
		*kt = tofi->proxd_kt[3];
	}

	if (ACMAJORREV_3(pi->pubpi->phy_rev) ||
		(ACMAJORREV_2(pi->pubpi->phy_rev) &&
		(ACMINORREV_1(pi) || ACMINORREV_4(pi) ||
		ACMINORREV_3(pi) || ACMINORREV_5(pi))) ||
		ACMAJORREV_4(pi->pubpi->phy_rev) ||
		ACMAJORREV_0(pi->pubpi->phy_rev) ||
		ACMAJORREV_5(pi->pubpi->phy_rev))
	{
		/* For 4345 B0/B1 */
		/* For 4350 C0/C1/C2 */
		/* For 4354A1/4345A2(4356) */
		/* For 4349/4355/4359 */
		/* For 43602 */
		if (CHSPEC_IS80(chanspec)) {
			kvalueptr = tofi->proxd_80m_k_values;
			/* For 4350 C0/C1/C2 */
			/* For 4354A1/4345A2(4356) */
			if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
				(ACMINORREV_1(pi) || ACMINORREV_4(pi) ||
				ACMINORREV_3(pi) || ACMINORREV_5(pi)))
			{
				*kseq = 90;
			}
		}
		else if (CHSPEC_IS40(chanspec)) {
			kvalueptr = tofi->proxd_40m_k_values;
		}
		else if (CHSPEC_IS20_5G(chanspec)) {
			kvalueptr = tofi->proxd_20m_k_values;
		}
		else {
			kvalueptr = tofi->proxd_2g_k_values;
		}
	}
	return (uint16 *)kvalueptr;
}

static int phy_ac_tof_kvalue(phy_type_tof_ctx_t *ctx, chanspec_t chanspec, uint32 rspecidx,
	uint32 *kip, uint32 *ktp, uint8 flag)
{
	uint16 const *kvaluep = NULL;
	int idx = 0, channel = CHSPEC_CHANNEL(chanspec);
	int32 ki = 0, kt = 0, kseq = 0;
	int rtt_adj = 0, rtt_adj_ts, irate_adj = 0, iack_adj = 0, trate_adj = 0, tack_adj = 0;
	int rtt_adj_papd = 0, papd_en = 0;
	uint8 bwidx;

	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	kvaluep = phy_ac_tof_kvalue_tables(pi, tofi, chanspec, &ki, &kt, &kseq);
	if (flag & WL_PROXD_SEQEN) {
		if (kip)
			*kip = kseq;
		if (ktp)
			*ktp = kseq;
		return BCME_OK;
	}
	bwidx = flag & WL_PROXD_BW_MASK;

	if (kvaluep) {
		int8 rateidx, ackidx; /* VHT = -1, legacy6M = 0, legacy = 1, mcs0 = 2, mcs = 3 */
		rateidx = rspecidx & 0xff;
		rateidx--;
		ackidx = (rspecidx >> 8) & 0xff;
		ackidx --;
		rtt_adj = (4 - READ_PHYREGFLD(pi, RxFeCtrl1, rxfe_bilge_cnt));
		rtt_adj_ts = 80;
		if (READ_PHYREGFLD(pi, PapdEnable0, papd_compEnb0))
			papd_en = 1;
		if (CHSPEC_IS80(chanspec)) {
			if (channel <= 58)
				idx = (channel - 42) >> 4;
			else if (channel <= 138)
				idx = ((channel - 106) >> 4) + 2;
			else
				idx = 5;
			rtt_adj_ts = 25;
			if (papd_en)
				rtt_adj_papd = 25;
			if (rateidx != -1) {
				irate_adj = tofi->proxdi_rate_offset_80m[rateidx];
				trate_adj = tofi->proxdt_rate_offset_80m[rateidx];
			}
			if (ackidx != -1) {
				iack_adj = tofi->proxdi_ack_offset[0];
				tack_adj = tofi->proxdt_ack_offset[0];
			}
		} else if (CHSPEC_IS40(chanspec)) {
			if (channel <= 62)
				idx = (channel - 38) >> 3;
			else if (channel <= 142)
				idx = ((channel - 102) >> 3) + 4;
			else
				idx = ((channel - 151) >> 3) + 10;
			rtt_adj_ts = 40;
			if (papd_en)
				rtt_adj_papd = 30;
			if (rateidx != -1) {
				irate_adj = tofi->proxdi_rate_offset_40m[rateidx];
				trate_adj = tofi->proxdt_rate_offset_40m[rateidx];
			}
			if (ackidx != -1) {
				iack_adj = tofi->proxdi_ack_offset[1];
				tack_adj = tofi->proxdt_ack_offset[1];
			}
		} else if (CHSPEC_IS20_5G(chanspec)) {
			/* 5G 20M Hz channels */
			if (channel <= 64)
				idx = (channel - 36) >> 2;
			else if (channel <= 144)
				idx = ((channel - 100) >> 2) + 8;
			else
				idx = ((channel - 149) >> 2) + 20;
			if (papd_en)
				rtt_adj_papd = 66;
			if (rateidx != -1) {
				irate_adj = tofi->proxdi_rate_offset_20m[rateidx];
				trate_adj = tofi->proxdt_rate_offset_20m[rateidx];
			}
			if (ackidx != -1) {
				iack_adj = tofi->proxdi_ack_offset[2];
				tack_adj = tofi->proxdt_ack_offset[2];
			}
		} else if (channel >= 1 && channel <= 14) {
			/* 2G channels */
			idx = channel - 1;
			if (papd_en)
				rtt_adj_papd = 70;
			if (rateidx != -1) {
				irate_adj = tofi->proxdi_rate_offset_2g[rateidx];
				trate_adj = tofi->proxdt_rate_offset_2g[rateidx];
			}
			if (ackidx != -1) {
				iack_adj = tofi->proxdi_ack_offset[3];
				tack_adj = tofi->proxdt_ack_offset[3];
			}
		}
		rtt_adj = (rtt_adj_ts * rtt_adj) >> k_tof_k_rtt_adj_Q;
		ki += ((int32)rtt_adj + (int32)rtt_adj_papd - irate_adj - iack_adj);
		kt += ((int32)rtt_adj + (int32)rtt_adj_papd - trate_adj - tack_adj);
		if (bwidx) {
			kt -= tofi->proxd_subbw_offset[bwidx-1][rateidx+1];
		}
		if (kip)
		{
			*kip = (uint32)(ki + (int8)(kvaluep[idx] & 0xff));
		}
		if (ktp)
		{
			*ktp = (uint32)(kt + (int8)(kvaluep[idx] >> 8));
		}
		return BCME_OK;
	}

	return BCME_ERROR;
}

static int
phy_ac_tof_seq_params(phy_type_tof_ctx_t *ctx)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	if (CHSPEC_IS2G(pi->radio_chanspec))
		return BCME_ERROR;

	if (CHSPEC_IS80(pi->radio_chanspec)) {

		tofi->tof_sc_FS = 160;
#ifdef TOF_SEQ_20_IN_80MHz
		tofi->tof_seq_log2_n = k_tof_seq_log2_n_80MHz_20;
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_80MHz_20, sizeof(uint16)*10);
		tofi->tof_seq_spb_len = k_tof_seq_spb_len_20MHz;
		tofi->tof_seq_spb = (uint32 *) k_tof_seq_spb_20MHz;
#else
		tofi->tof_seq_log2_n = k_tof_seq_log2_n_80MHz;
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_80MHz, sizeof(uint16)*10);
		tofi->tof_seq_spb_len = k_tof_seq_spb_len_80MHz;
		tofi->tof_seq_spb = (uint32 *) k_tof_seq_spb_80MHz;
#endif

		return BCME_OK;

	}

#if defined(TOF_SEQ_40MHz_BW) || defined(TOF_SEQ_40_IN_40MHz)
	else if (CHSPEC_IS40(pi->radio_chanspec)) {
		tofi->tof_sc_FS = 80;
		tofi->tof_seq_log2_n = k_tof_seq_log2_n_40MHz;
#ifdef TOF_SEQ_40_IN_40MHz
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_40MHz_40, sizeof(uint16)*10);
		tofi->tof_seq_spb_len = k_tof_seq_spb_len_40MHz;
		tofi->tof_seq_spb = (uint32 *) k_tof_seq_spb_40MHz;
#else
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_40MHz, sizeof(uint16)*10);
		tofi->tof_seq_spb_len = k_tof_seq_spb_len_20MHz;
		tofi->tof_seq_spb = (uint32 *) k_tof_seq_spb_20MHz;
#endif
		return BCME_OK;

	}
#endif /* defined(TOF_SEQ_40MHz_BW) || defined (TOF_SEQ_40_IN_40MHz) */

#if defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT)
else {
		tofi->tof_sc_FS = 40;
		tofi->tof_seq_log2_n = k_tof_seq_log2_n_20MHz;
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_20MHz, sizeof(uint16)*10);
		tofi->tof_seq_spb_len = k_tof_seq_spb_len_20MHz;
		tofi->tof_seq_spb = (uint32 *) k_tof_seq_spb_20MHz;

		return BCME_OK;

	}
#endif /* defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT) */
	return BCME_ERROR;
}

static void
BCMATTACHFN(phy_ac_nvram_proxd_read)(phy_info_t *pi, phy_ac_tof_info_t *tofi)
{
	uint8 i;

	if (PHY_GETVAR(pi, rstr_proxd_basekival)) {
		for (i = 0; i < 4; i++) {
			tofi->proxd_ki[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_basekival, i);
		}
	} else {
		if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
			/* For 4345 B0/B1 */
			tofi->proxd_ki[0] = TOF_INITIATOR_K_4345_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_4345_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_4345_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_4345_2G;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) && (ACMINORREV_1(pi) ||
			ACMINORREV_4(pi))) {
			/* For 4350 C0/C1/C2 */
			tofi->proxd_ki[0] = TOF_INITIATOR_K_4350_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_4350_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_4350_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_4350_2G;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) && (ACMINORREV_3(pi) ||
			ACMINORREV_5(pi))) {
			/* For 4354A1/4345A2(4356) */
			tofi->proxd_ki[0] = TOF_INITIATOR_K_4354_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_4354_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_4354_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_4354_2G;
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			/* For 4349/4355/4359 */
			tofi->proxd_ki[0] = TOF_INITIATOR_K_4349_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_4349_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_4349_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_4349_2G;
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
			/* For 43602 */
			tofi->proxd_ki[0] = TOF_INITIATOR_K_43602_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_43602_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_43602_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_43602_2G;
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_basektval)) {
		for (i = 0; i < 4; i++) {
			tofi->proxd_kt[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_basektval, i);
		}
	} else {
		if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
			/* For 4345 B0/B1 */
			tofi->proxd_kt[0] = TOF_TARGET_K_4345_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_4345_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_4345_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_4345_2G;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) && (ACMINORREV_1(pi) ||
			ACMINORREV_4(pi))) {
			/* For 4350 C0/C1/C2 */
			tofi->proxd_kt[0] = TOF_TARGET_K_4350_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_4350_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_4350_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_4350_2G;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) && (ACMINORREV_3(pi) ||
			ACMINORREV_5(pi))) {
			/* For 4354A1/4345A2(4356) */
			tofi->proxd_kt[0] = TOF_TARGET_K_4354_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_4354_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_4354_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_4354_2G;
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			/* For 4349/4355/4359 */
			tofi->proxd_kt[0] = TOF_TARGET_K_4349_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_4349_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_4349_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_4349_2G;
		} else if (ACMAJORREV_5(pi->pubpi->phy_rev) || ACMAJORREV_0(pi->pubpi->phy_rev)) {
			/* For 43602 */
			tofi->proxd_kt[0] = TOF_TARGET_K_43602_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_43602_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_43602_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_43602_2G;
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_80mkval)) {
		for (i = 0; i < 6; i++) {
			tofi->proxd_80m_k_values[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_80mkval, i);
		}
	} else {
		uint16 const *kvalueptr;

		if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
			/* For 4345 B0/B1 */
			kvalueptr = proxd_4345_80m_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_1(pi) || ACMINORREV_4(pi))) {
			/* For 4350 C0/C1/C2 */
			kvalueptr = proxd_4350_80m_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_3(pi) || ACMINORREV_5(pi))) {
			/* For 4354A1/4345A2(4356) */
			kvalueptr = proxd_4354_80m_k_values;
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			/* For 4349/4355/4359 */
			kvalueptr = proxd_4349_80m_k_values;
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
			/* For 43602 */
			kvalueptr = proxd_43602_80m_k_values;
		} else {
			return;
		}
		for (i = 0; i < 6; i++) {
			tofi->proxd_80m_k_values[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_40mkval)) {
		for (i = 0; i < 12; i++) {
			tofi->proxd_40m_k_values[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_40mkval, i);
		}
	} else {
		uint16 const *kvalueptr;

		if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
			/* For 4345 B0/B1 */
			kvalueptr = proxd_4345_40m_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_1(pi) || ACMINORREV_4(pi))) {
			/* For 4350 C0/C1/C2 */
			kvalueptr = proxd_4350_40m_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_3(pi) || ACMINORREV_5(pi))) {
			/* For 4354A1/4345A2(4356) */
			kvalueptr = proxd_4354_40m_k_values;
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			/* For 4349/4355/4359 */
			kvalueptr = proxd_4349_40m_k_values;
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
			/* For 43602 */
			kvalueptr = proxd_43602_40m_k_values;
		} else {
			return;
		}
		for (i = 0; i < 12; i++) {
			tofi->proxd_40m_k_values[i] = kvalueptr[i];
		}
	}


	if (PHY_GETVAR(pi, rstr_proxd_20mkval)) {
		for (i = 0; i < 25; i++) {
			tofi->proxd_20m_k_values[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_20mkval, i);
		}
	} else {
		uint16 const *kvalueptr;

		if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
			/* For 4345 B0/B1 */
			kvalueptr = proxd_4345_20m_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_1(pi) || ACMINORREV_4(pi))) {
			/* For 4350 C0/C1/C2 */
			kvalueptr = proxd_4350_20m_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_3(pi) || ACMINORREV_5(pi))) {
			/* For 4354A1/4345A2(4356) */
			kvalueptr = proxd_4354_20m_k_values;
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			/* For 4349/4355/4359 */
			kvalueptr = proxd_4349_20m_k_values;
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
			/* For 43602 */
			kvalueptr = proxd_43602_20m_k_values;
			/* For 43012 */
		} else {
			return;
		}
		for (i = 0; i < 25; i++) {
			tofi->proxd_20m_k_values[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_2gkval)) {
		for (i = 0; i < 14; i++) {
			tofi->proxd_2g_k_values[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_2gkval, i);
		}
	} else {
		uint16 const *kvalueptr;
		if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
			/* For 4345 B0/B1 */
			kvalueptr = proxd_4345_2g_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_1(pi) || ACMINORREV_4(pi))) {
			/* For 4350 C0/C1/C2 */
			kvalueptr = proxd_4350_2g_k_values;
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_3(pi) || ACMINORREV_5(pi))) {
			/* For 4354A1/4345A2(4356) */
			kvalueptr = proxd_4354_2g_k_values;
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			/* For 4349/4355/4359 */
			kvalueptr = proxd_4349_2g_k_values;
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
			/* For 43602 */
			kvalueptr = proxd_43602_2g_k_values;
			/* For 43012 */
		} else {
			return;
		}
		for (i = 0; i < 14; i++) {
			tofi->proxd_2g_k_values[i] = kvalueptr[i];
		}
	}

	for (i = 0; i < sizeof(proxdi_rate_offset_80m)/sizeof(int16); i++) {
		tofi->proxdi_rate_offset_80m[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdi_rate80m, i, proxdi_rate_offset_80m[i]);
	}

	for (i = 0; i < sizeof(proxdi_rate_offset_40m)/sizeof(int16); i++) {
		tofi->proxdi_rate_offset_40m[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdi_rate40m, i, proxdi_rate_offset_40m[i]);
	}

	for (i = 0; i < sizeof(proxdi_rate_offset_20m)/sizeof(int16); i++) {
		tofi->proxdi_rate_offset_20m[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdi_rate20m, i, proxdi_rate_offset_20m[i]);
	}

	for (i = 0; i < sizeof(proxdi_rate_offset_2g)/sizeof(int16); i++) {
		tofi->proxdi_rate_offset_2g[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdi_rate2g, i, proxdi_rate_offset_2g[i]);
	}

	for (i = 0; i < sizeof(proxdt_rate_offset_80m)/sizeof(int16); i++) {
		tofi->proxdt_rate_offset_80m[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdt_rate80m, i, proxdt_rate_offset_80m[i]);
	}

	for (i = 0; i < sizeof(proxdt_rate_offset_40m)/sizeof(int16); i++) {
		tofi->proxdt_rate_offset_40m[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdt_rate40m, i, proxdt_rate_offset_40m[i]);
	}

	for (i = 0; i < sizeof(proxdt_rate_offset_20m)/sizeof(int16); i++) {
		tofi->proxdt_rate_offset_20m[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdt_rate20m, i, proxdt_rate_offset_20m[i]);
	}

	for (i = 0; i < sizeof(proxdt_rate_offset_2g)/sizeof(int16); i++) {
		tofi->proxdt_rate_offset_2g[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdt_rate2g, i, proxdt_rate_offset_2g[i]);
	}

	for (i = 0; i < sizeof(proxdi_ack_offset)/sizeof(int16); i++) {
		tofi->proxdi_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdi_ack, i, proxdi_ack_offset[i]);
	}

	for (i = 0; i < sizeof(proxdt_ack_offset)/sizeof(int16); i++) {
		tofi->proxdt_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdt_ack, i, proxdt_ack_offset[i]);
	}

	for (i = 0; i < 5; i++) {
		tofi->proxd_subbw_offset[0][i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxd_sub80m40m, i, proxd_subbw_offset[0][i]);
	}

	for (i = 0; i < 5; i++) {
		tofi->proxd_subbw_offset[1][i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxd_sub80m20m, i, proxd_subbw_offset[1][i]);
	}

	for (i = 0; i < 5; i++) {
		tofi->proxd_subbw_offset[2][i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxd_sub40m20m, i, proxd_subbw_offset[2][i]);
	}
}

#endif /* WL_PROXDETECT */
