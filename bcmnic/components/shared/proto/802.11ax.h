/*
 * Basic types and constants relating to 802.11ax/HE STA
 * This is a portion of 802.11ax definition. The rest are in 802.11.h.
 *
 * Copyright (C) 2016, Broadcom. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: 802.11ax.h 665611 2016-10-18 11:54:41Z $
 */

#ifndef _802_11ax_h_
#define _802_11ax_h_

#include <typedefs.h>

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

/**
 * HE Capabilites element (sec 9.4.2.218)
 */

/* HE MAC Capabilities Information field (figure 9-589ck) */
#define HE_MAC_CAP_INFO_SIZE	5
typedef uint8 he_mac_cap_t[HE_MAC_CAP_INFO_SIZE];

/* bit position and field width */
#define HE_MAC_HTC_HE_SUPPORT_IDX			0	/* HTC HE Support */
#define HE_MAC_HTC_HE_SUPPORT_FSZ			1
#define HE_MAC_TWT_REQ_SUPPORT_IDX			1	/* TWT Requestor Support */
#define HE_MAC_TWT_REQ_SUPPORT_FSZ			1
#define HE_MAC_TWT_RESP_SUPPORT_IDX			2	/* TWT Responder Support */
#define HE_MAC_TWT_RESP_SUPPORT_FSZ			1
#define HE_MAC_FRAG_SUPPORT_IDX				3	/* Fragmentation Support */
#define HE_MAC_FRAG_SUPPORT_FSZ				2
#define HE_MAC_MAX_MSDU_FRAGS_IDX			5	/* Max. Fragmented MSDUs */
#define HE_MAC_MAX_MSDU_FRAGS_FSZ			3
#define HE_MAC_MIN_FRAG_SIZE_IDX			8	/* Min. Fragment Size */
#define HE_MAC_MIN_FRAG_SIZE_FSZ			2
#define HE_MAC_TRG_PAD_DUR_IDX				10	/* Trigger Frame MAC Pad Dur */
#define HE_MAC_TRG_PAD_DUR_FSZ				2
#define HE_MAC_MULTI_TID_AGG_IDX			12	/* Multi TID Aggregation */
#define HE_MAC_MULTI_TID_AGG_FSZ			3
#define HE_MAC_LINK_ADAPT_IDX				15	/* HE Link Adaptation */
#define HE_MAC_LINK_ADAPT_FSZ				2
#define HE_MAC_ALL_ACK_SUPPORT_IDX			17	/* All Ack Support */
#define HE_MAC_ALL_ACK_SUPPORT_FSZ			1
#define HE_MAC_UL_MU_RESP_SCHED_IDX			18	/* UL MU Response Scheduling */
#define HE_MAC_UL_MU_RESP_SCHED_FSZ			1
#define HE_MAC_A_BSR_IDX				19	/* A-BSR Support */
#define HE_MAC_A_BSR_FSZ				1
#define HE_MAC_BC_TWT_SUPPORT_IDX			20	/* Broadcast TWT Support */
#define HE_MAC_BC_TWT_SUPPORT_FSZ			1
#define HE_MAC_BA_BITMAP_SUPPORT_IDX			21	/* 32-bit BA Bitmap Support */
#define HE_MAC_BA_BITMAP_SUPPORT_FSZ			1
#define HE_MAC_MU_CASCADE_SUPPORT_IDX			22	/* MU Cascade Support */
#define HE_MAC_MU_CASCADE_SUPPORT_FSZ			1
#define HE_MAC_MULTI_TID_AGG_ACK_IDX			23	/* Ack Enabled Multi TID Agg. */
#define HE_MAC_MULTI_TID_AGG_ACK_FSZ			1
#define HE_MAC_GRP_MULTI_STA_MULTI_BACK_IDX		24	/* Group Addr. Multi STA BAck */
#define HE_MAC_GRP_MULTI_STA_MULTI_BACK_FSZ		1
#define HE_MAC_OMI_ACONTROL_SUPPORT_IDX			25	/* OMI A-Control Support */
#define HE_MAC_OMI_ACONTROL_SUPPORT_FSZ			1
#define HE_MAC_OFDMA_RA_SUPPORT_IDX			26	/* OFDMA RA Support */
#define HE_MAC_OFDMA_RA_SUPPORT_FSZ			1
#define HE_MAC_MAX_AMPDU_LEN_EXP_IDX			27	/* Max AMPDU Length Exponent */
#define HE_MAC_MAX_AMPDU_LEN_EXP_FSZ			2
#define HE_MAC_DL_MU_MIMO_PART_BW_RX_IDX		29	/* DL MU MIMO Partial BW Rx */
#define HE_MAC_DL_MU_MIMO_PART_BW_RX_FSZ		1
#define HE_MAC_UL_MU_MIMO_IDX				30	/* UL MU MIMO */
#define HE_MAC_UL_MU_MIMO_FSZ				2

/* HE PHY Capabilities Information field (figure 9-589cl) */
#define HE_PHY_CAP_INFO_SIZE				9
typedef uint8 he_phy_cap_t[HE_PHY_CAP_INFO_SIZE];

/* bit position and field width */
#define HE_PHY_DBAND_SUPPORT_IDX			0	/* Dual Band Support */
#define HE_PHY_DBAND_SUPPORT_FSZ			1
#define HE_PHY_CHAN_WIDTH_SET_IDX			1	/* Channel Width Set */
#define HE_PHY_CHAN_WIDTH_SET_FSZ			7
#define HE_PHY_PREAMBLE_PUNCT_RX_IDX			8	/* Preamble Puncturing Rx */
#define HE_PHY_PREAMBLE_PUNCT_RX_FSZ			4
#define HE_PHY_DEVICE_CLASS_IDX				12	/* Device Class */
#define HE_PHY_DEVICE_CLASS_FSZ				1
#define HE_PHY_LDPC_PLD_IDX				13	/* LDPC Coding In Payload */
#define HE_PHY_LDPC_PLD_FSZ				1
#define HE_PHY_HE_LTF_GI_PPDU_IDX			14	/* HE-LTF And GI For HE PPDUs */
#define HE_PHY_HE_LTF_GI_PPDU_FSZ			2
#define HE_PHY_HE_LTF_GI_NDP_IDX			16	/* HE-LTF And GI For NDP */
#define HE_PHY_HE_LTF_GI_NDP_FSZ			2
#define HE_PHY_STBC_TX_RX_IDX				18	/* STBC Tx & Rx */
#define HE_PHY_STBC_TX_RX_FSZ				2
#define HE_PHY_DOPPLER_IDX				20	/* Doppler */
#define HE_PHY_DOPPLER_FSZ				2
#define HE_PHY_UL_MU_IDX				22	/* UL MU */
#define HE_PHY_UL_MU_FSZ				2
#define HE_PHY_DCM_ENC_TX_IDX				24	/* DCM Encoding Tx */
#define HE_PHY_DCM_ENC_TX_FSZ				3
#define HE_PHY_DCM_ENC_RX_IDX				27	/* DCM Encoding Rx */
#define HE_PHY_DCM_ENC_RX_FSZ				3
#define HE_PHY_UL_MU_PYLD_RU_IDX			30	/* UL MU Payload Over 106-tone RU */
#define HE_PHY_UL_MU_PYLD_RU_FSZ			1
#define HE_PHY_SU_BEAMFORMER_IDX			31	/* SU Beamformer */
#define HE_PHY_SU_BEAMFORMER_FSZ			1
#define HE_PHY_SU_BEAMFORMEE_IDX			32	/* SU Beamformee */
#define HE_PHY_SU_BEAMFORMEE_FSZ			1
#define HE_PHY_MU_BEAMFORMER_IDX			33	/* MU Beamformer */
#define HE_PHY_MU_BEAMFORMER_FSZ			1
#define HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_IDX		34	/* Beamformee STS For <= 80MHz */
#define HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_FSZ		3
#define HE_PHY_NSTS_TOTAL_BELOW80MHZ_IDX		37	/* NSTS Total for <= 80MHz */
#define HE_PHY_NSTS_TOTAL_BELOW80MHZ_FSZ		3
#define HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_IDX		40	/* Beamformee STS For >80 MHz */
#define HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_FSZ		3
#define HE_PHY_NSTS_TOTAL_ABOVE80MHZ_IDX		43	/*  NSTS Total for > 80MHz */
#define HE_PHY_NSTS_TOTAL_ABOVE80MHZ_FSZ		3
#define HE_PHY_SOUND_DIM_BELOW80MHZ_IDX			46	/* Num. Sounding Dim.<= 80 MHz */
#define HE_PHY_SOUND_DIM_BELOW80MHZ_FSZ			3
#define HE_PHY_SOUND_DIM_ABOVE80MHZ_IDX			49	/* Num. Sounding Dim.> 80 MHz */
#define HE_PHY_SOUND_DIM_ABOVE80MHZ_FSZ			3
#define HE_PHY_SU_FEEDBACK_NG16_SUPPORT_IDX		52	/* Ng=16 For SU Feedback */
#define HE_PHY_SU_FEEDBACK_NG16_SUPPORT_FSZ		1
#define HE_PHY_MU_FEEDBACK_NG16_SUPPORT_IDX		53	/* Ng=16 For MU Feedback */
#define HE_PHY_MU_FEEDBACK_NG16_SUPPORT_FSZ		1
#define HE_PHY_SU_CODEBOOK_SUPPORT_IDX			54	/* Codebook Sz {4, 2} For SU */
#define HE_PHY_SU_CODEBOOK_SUPPORT_FSZ			1
#define HE_PHY_MU_CODEBOOK_SUPPORT_IDX			55	/* Codebook Size {7, 5} For MU */
#define HE_PHY_MU_CODEBOOK_SUPPORT_FSZ			1
#define HE_PHY_TRG_BFM_FEEDBACK_IDX			56	/* Trigger Frame Beamforming FB */
#define HE_PHY_TRG_BFM_FEEDBACK_FSZ			3
#define HE_PHY_EXT_RANGE_SU_PYLD_IDX			59	/* HE ER SU PPDU Payload */
#define HE_PHY_EXT_RANGE_SU_PYLD_FSZ			1
#define HE_PHY_DL_MU_MIMO_PART_BW_IDX			60	/* DL MUMIMO On Partial Bandwidth */
#define HE_PHY_DL_MU_MIMO_PART_BW_FSZ			1
#define HE_PHY_PPE_THRESH_PRESENT_IDX			61	/* PPE Threshold Present */
#define HE_PHY_PPE_THRESH_PRESENT_FSZ			1
#define HE_PHY_SRP_SR_SUPPORT_IDX			62	/* SRPbased SR Support */
#define HE_PHY_SRP_SR_SUPPORT_FSZ			1
#define HE_PHY_PART_BW_SU_FEEDBACK_IDX			63	/* Partial Bandwidth SU Feedback */
#define HE_PHY_PART_BW_SU_FEEDBACK_FSZ			1
#define HE_PHY_PART_BW_MU_FEEDBACK_IDX			64	/* Partial Bandwidth MU Feedback */
#define HE_PHY_PART_BW_MU_FEEDBACK_FSZ			1
#define HE_PHY_CQI_FEEDBACK_IDX				65	/* CQI Feedback Support */
#define HE_PHY_CQI_FEEDBACK_FSZ				1

/**
 * Tx Rx MCS NSS Support field - minimum (figure 9-589cm).
 *
 * Note : To be removed after corresponding src changes are complete
 */
typedef uint16 he_mcs_nss_min_t;

/* Tx Rx HE MCS Support field format : IEEE Draft P802.11ax D0.5 Table 9-589cm */
typedef uint16 he_mcs_sup_field_fixed_t;

typedef struct {
	/**
	 * Fixed portion of the TX RX HE MCS SUPPORT FIELD :
	 * - 3 bits for Highest NSS Supported M1
	 * - 3 bits for Highest MCS Supported
	 * - 5 bits for TX BW BITMAP
	 * - 5 bits for RX BW BITMAP
	 */
	he_mcs_sup_field_fixed_t mcs_nss_cap_fixed;

	/**
	 * Variable size TX RX descriptor section :
	 * - TX and RX MCS NSS Desc
	 */
	uint8 tx_rx_desc[];
} he_txrx_mcs_nss_hdr_t;

/* defines for the fixed portion */
#define HE_MCS_NSS_MAX_NSS_M1_MASK	0x0007
#define HE_MCS_NSS_MAX_MCS_MASK		0x0038
#define HE_MCS_NSS_MAX_MCS_SHIFT	3
#define HE_MCS_NSS_TX_BW_BMP_MASK	0x07c0
#define HE_MCS_NSS_TX_BW_BMP_SHIFT	6
#define HE_MCS_NSS_RX_BW_BMP_MASK	0xf800
#define HE_MCS_NSS_RX_BW_BMP_SHIFT	11

/* defines for tx/rx mcs nss desc */
#define HE_MCS_NSS_DESC_MCS_MASK	0x0f
#define HE_MCS_NSS_DESC_NSS_MASK	0x70
#define HE_MCS_NSS_DESC_NSS_SHIFT	4
#define HE_MCS_NSS_DESC_LAST		0x80

/* HE Capabilities element */
BWL_PRE_PACKED_STRUCT struct he_cap_ie {
	uint8 id;
	uint8 len;
	he_mac_cap_t mac_cap;				/* MAC Capabilities Information */
	he_phy_cap_t phy_cap;				/* PHY Capabilities Information */
	he_mcs_sup_field_fixed_t mcs_nss_cap_fixed;	/* Tx Rx MCS NSS Support - Fixed portion */
	/* he_ppe_thresh_t ppe_thresh; */ /* PPE Thresholds (optional) - variable size */
} BWL_POST_PACKED_STRUCT;

typedef struct he_cap_ie he_cap_ie_t;

/* IEEE Draft P802.11ax D0.5 Table 9-262ab, Highest MCS Supported subfield encoding */
#define HE_CAP_MCS_CODE_0_7		0
#define HE_CAP_MCS_CODE_0_8		1
#define HE_CAP_MCS_CODE_0_9		2
#define HE_CAP_MCS_CODE_0_10		3
#define HE_CAP_MCS_CODE_0_11		4
#define HE_CAP_MCS_CODE_SIZE		3	/* num bits for 1-stream */
#define HE_CAP_MCS_CODE_MASK		0x7	/* mask for 1-stream */

/**
 * IEEE Draft P802.11ax D0.5 Figure 9-589cm
 * - Defines for TX & RX BW BITMAP
 *
 * (Size of TX BW bitmap = RX BW bitmap = 5 bits)
 */
#define HE_MCS_NSS_TX_BW_MASK		0x07c0
#define HE_MCS_NSS_TX_BW_SHIFT		6

#define HE_MCS_NSS_RX_BW_MASK		0xf800
#define HE_MCS_NSS_RX_BW_SHIFT		11

/* Fragmentation Support field (table 9-262z) */
#define HE_FRAG_SUPPORT_NO_FRAG		0	/* no frag */
#define HE_FRAG_SUPPORT_VHT_SINGLE_MPDU	1	/* VHT single MPDU only */
#define HE_FRAG_SUPPORT_ONE_PER_MPDU	2	/* 1 frag per MPDU in A-MPDU */
#define HE_FRAG_SUPPORT_MULTI_PER_MPDU	3	/* 2+ frag per MPDU in A-MPDU */

#define HE_CAP_MCS_MAP_NSS_MAX	8
#define HE_MAX_RU_COUNT		4 /* Max number of RU allocation possible */

#define HE_NSSM1_LEN	3 /* length of NSSM1 field in bits */
#define HE_RU_COUNT_LEN 2 /* length of RU count field in bits */

/* For HE SIG A : PLCP0 bit fields [assuming 32bit plcp] */
#define HE_SIGA_FORMAT_HE_SU		0x00000001
#define HE_SIGA_TXOP_PLCP0		0xFC000000
#define HE_SIGA_RESERVED_PLCP0		0x00004000

/* For HE SIG A : PLCP1 bit fields [assuming 32bit plcp] */
#define HE_SIGA_TXOP_PLCP1		0x00000001
#define HE_SIGA_CODING_LDPC		0x00000002
#define HE_SIGA_STBC			0x00000008
#define HE_SIGA_BEAMFORM_ENABLE		0x00000010
#define HE_SIGA_RESERVED_PLCP1		0x00000100

/* For HE SIG A : PLCP0 bit shifts/masks/vals */
#define HE_SIGA_MCS_SHIFT		3
#define HE_SIGA_DCM_SHIFT		7
#define HE_SIGA_NSTS_MASK		0x00000007
#define HE_SIGA_NSTS_SHIFT		23

#define HE_SIGA_20MHZ_VAL		0x00000000
#define HE_SIGA_40MHZ_VAL		0x00080000
#define HE_SIGA_80MHZ_VAL		0x00100000
#define HE_SIGA_160MHZ_VAL		0x00180000

#define HE_SIGA_CP_LTF_GI_0_8us_VAL	0x00200000
#define HE_SIGA_CP_LTF_GI_1_6us_VAL	0x00400000
#define HE_SIGA_CP_LTF_GI_3_2us_VAL	0x00600000

/* PPE Threshold field (figure 9-589co) */
#define HE_PPE_THRESH_NSS_M1_MASK	0x0007
#define HE_PPE_THRESH_RU_CNT_MASK	0x0018
#define HE_PPE_THRESH_RU_CNT_SHIFT	3

/* PPE Threshold Info field (figure 9-589cp) */
/* ruc: RU Count; NSSnM1: NSSn - 1; RUmM1: RUm - 1 */
#define HE_PPET16_BIT_OFFSET(ruc, NSSnM1, RUmM1) \
	(5 + ((NSSnM1) * (ruc) + (RUmM1)) * 6)		/* bit offset in PPE Threshold field */
#define HE_PPET16_BYTE_OFFSET(ruc, NSSnM1, RUmM1) \
	(HE_PPET16_BIT_OFFSET(ruc, NSSnM1, RUmM1) >> 3)	/* byte offset in PPE Threshold field,
							 * 3 bits may spin across 2 bytes.
							 */
#define HE_PPET16_MASK(ruc, NSSnM1, RUmM1) \
	(7 << HE_PPET16_SHIFT(ruc, NSSnM1, RUmM1))	/* bitmask in above 2 bytes */
#define HE_PPET16_SHIFT(ruc, NSSnM1, RUmM1) \
	(HE_PPET16_BIT_OFFSET(ruc, NSSnM1, RUmM1) - \
	 (HE_PPET16_BYTE_OFFSET(ruc, NSSnM1, RUmM1) << 3))	/* bit shift in above 2 bytes */
#define HE_PPET8_BIT_OFFSET(ruc, NSSnM1, RUmM1) \
	(5 + ((NSSnM1) * (ruc) + (RUmM1)) * 6 + 3)	/* bit offset in PPE Threshold field */
#define HE_PPET8_BYTE_OFFSET(ruc, NSSnM1, RUmM1) \
	(HE_PPET8_BIT_OFFSET(ruc, NSSnM1, RUmM1) >> 3)	/* byte offset in PPE Threshold field,
							 * 6 bits may spin across 2 bytes.
							 */
#define HE_PPET8_MASK(ruc, NSSnM1, RUmM1) \
	(7 << HE_PPET8_SHIFT(ruc, NSSnM1, RUmM1))	/* bitmask in above 2 bytes */
#define HE_PPET8_SHIFT(ruc, NSSnM1, RUmM1) \
	(HE_PPET8_BIT_OFFSET(ruc, NSSnM1, RUmM1) - \
	 (HE_PPET8_BYTE_OFFSET(ruc, NSSnM1, RUmM1) << 3))	/* bit shift in above 2 bytes */

/* Total PPE Threshold field byte length */
#define HE_PPE_THRESH_LEN(nss, ruc)	(5 + (((nss) * (ruc) * 6) >> 3))

/* RU Allocation Index encoding (table 9-262ae) */
#define HE_RU_ALLOC_IDX_242		0	/* RU alloc: 282 tones */
#define HE_RU_ALLOC_IDX_484		1	/* RU alloc: 484 tones - 40Mhz */
#define HE_RU_ALLOC_IDX_996		2	/* RU alloc: 996 tones - 80Mhz */
#define HE_RU_ALLOC_IDX_2x996		3	/* RU alloc: 2x996 tones - 80p80/160Mhz */

/* Constellation Index encoding (table 9-262ac) */
#define HE_CONST_IDX_BPSK		0
#define HE_CONST_IDX_QPSK		1
#define HE_CONST_IDX_16QAM		2
#define HE_CONST_IDX_64QAM		3
#define HE_CONST_IDX_256QAM		4
#define HE_CONST_IDX_1024QAM		5
#define HE_CONST_IDX_RSVD		6
#define HE_CONST_IDX_NONE		7

/**
 * HE Operation IE (sec 9.4.2.219)
 */
/* HE Operation Parameters field (figure 9-589cr) */
#define HE_BASIC_MCS_NSS_SIZE	3
typedef uint8 he_basic_mcs_nss_set_t[HE_BASIC_MCS_NSS_SIZE];

typedef uint16 he_op_parms_t;

/* bit position and field width */
#define HE_OP_BSS_COLOR_IDX			0	/* BSS Color */
#define HE_OP_BSS_COLOR_FSZ			6
#define HE_OP_DEF_PE_DUR_IDX			6	/* Default PE Duration */
#define HE_OP_DEF_PE_DUR_FSZ			3
#define HE_OP_TWT_REQD_IDX			9	/* TWT Required */
#define HE_OP_TWT_REQD_FSZ			1
#define HE_OP_HE_DUR_RTS_THRESH_IDX		10	/* HE Duration Based RTS Threshold */
#define HE_OP_HE_DUR_RTS_THRESH_FSZ		10
#define HE_OP_PART_BSS_COLOR_IDX		20	/* Partial BSS Color */
#define HE_OP_PART_BSS_COLOR_FSZ		1

/* HE Operation element */
BWL_PRE_PACKED_STRUCT struct he_op_ie {
	uint8 id;
	uint8 len;
	he_op_parms_t parms;
	he_basic_mcs_nss_set_t mcs_nss_op;	/* Basic HE MCS & NSS Set */
} BWL_POST_PACKED_STRUCT;

typedef struct he_op_ie he_op_ie_t;

/* Defines for Basic HE mcs-nss set */
#define HE_OP_MCS_NSS_SET_MASK		0x00ffffff /* Field is to be 24 bits long */
#define HE_OP_MCS_NSS_GET_SS_IDX(nss) (((nss)-1) * HE_CAP_MCS_CODE_SIZE)
#define HE_OP_MCS_NSS_GET_MCS(nss, mcs_nss_map) \
	(((mcs_nss_map) >> HE_OP_MCS_NSS_GET_SS_IDX(nss)) & HE_CAP_MCS_CODE_MASK)
#define HE_OP_MCS_NSS_SET_MCS(nss, numMcs, mcs_nss_map) \
	do { \
	 (mcs_nss_map) &= (~(HE_CAP_MCS_CODE_MASK << HE_OP_MCS_NSS_GET_SS_IDX(nss))); \
	 (mcs_nss_map) |= (((numMcs) & HE_CAP_MCS_CODE_MASK) << HE_OP_MCS_NSS_GET_SS_IDX(nss)); \
	 (mcs_nss_map) &= (HE_OP_MCS_NSS_SET_MASK); \
	} while (0)

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

/* HE Action Frame */
#define HE_AF_CAT_OFF	0
#define HE_AF_ACT_OFF	1

/* TWT Setup */
#define HE_AF_TWT_SETUP_TOKEN_OFF	2
#define HE_AF_TWT_SETUP_TWT_IE_OFF	3

/* TWT Teardown */
#define HE_AF_TWT_TEARDOWN_FLOW_OFF	2

/* TWT Information */
#define HE_AF_TWT_INFO_OFF	2

/* HE Action ID */
#define HE_ACTION_TWT_SETUP	1
#define HE_ACTION_TWT_TEARDOWN	2
#define HE_ACTION_TWT_INFO	3

#define HE_MAC_PPE_THRESH_PRESENT	0	/**< PPE Thresholds Present */
#define HE_MAC_TWT_REQ_SUPPORT		1	/**< TWT Requestor Support */
#define HE_MAC_TWT_RESP_SUPPORT		2	/**< TWT Responder Support */
#define HE_MAC_FRAG_SUPPORT		3	/**< Fragmentation Support */
#define HE_MAC_MAX_NSS_DCM		17	/**< Maximum Nss With DCM */
#define HE_MAC_UL_MU_RSP_SCHED		18	/**< UL MU Rsp Schedule */
#define HE_MAC_A_BSR			19	/**< A-BSR Support */

#define HE_OP_BSS_COLOR_MASK		0x003f	/**< BSS Color */

#endif /* _802_11ax_h_ */
