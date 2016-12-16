/*
 * ACPHY PAPD CAL module interface (to other PHY modules).
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
 * $Id: phy_ac_papdcal.h 656647 2016-08-29 12:15:57Z $
 */

#ifndef _phy_ac_papdcal_h_
#define _phy_ac_papdcal_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_papdcal.h>

/* papd params */
#define PAPD_TX_ATTN_2G 0xFF
#define PAPD_TX_ATTN_5G 0xFF00
#define PAPD_TX_ATTN_5G_SHIFT 8
#define PAPD_RX_ATTN_2G 0xFF
#define PAPD_RX_ATTN_5G 0xFF00
#define PAPD_RX_ATTN_5G_SHIFT 8
#define PAPD_CAL_IDX_2G 0xFF
#define PAPD_CAL_IDX_5G 0xFF00
#define PAPD_CAL_IDX_5G_SHIFT 8
#define PAPD_BBMULT_2G 0xFF
#define PAPD_BBMULT_5G 0xFF00
#define PAPD_BBMULT_5G_SHIFT 8
#define TIA_GAIN_MODE_2G 0xFF
#define TIA_GAIN_MODE_5G 0xFF00
#define TIA_GAIN_MODE_5G_SHIFT 8
#define PAPD_EPS_OFFSET_2G 0xFFFF
#define PAPD_EPS_OFFSET_5G 0xFFFF0000
#define PAPD_EPS_OFFSET_5G_SHIFT 16
#define PAPD_CALREF_DB_2G 0xFF
#define PAPD_CALREF_DB_5G 0xFF00
#define PAPD_CALREF_DB_5G_SHIFT 8
#define PAPD_BUF_OFFSET_2G 0xFF
#define PAPD_BUF_OFFSET_5G 0xFF00
#define PAPD_BUF_OFFSET_5G_SHIFT 8
#define PAPD_FRACDELAY_OFFSET_2G 0xFF
#define PAPD_FRACDELAY_OFFSET_5G 0xFF00
#define PAPD_FRACDELAY_OFFSET_5G_SHIFT 8
#define WPAPD_MAC_PLAY_ROM_ADDR_43012B0 0x192f0


/* forward declaration */
typedef struct phy_ac_papdcal_info phy_ac_papdcal_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_papdcal_info_t *phy_ac_papdcal_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_papdcal_info_t *mi);
void phy_ac_papdcal_unregister_impl(phy_ac_papdcal_info_t *info);


/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
extern uint16 papd_gainctrl_pga[];

/* used for PAPD cal */
typedef struct _acphy_txgains {
	uint16 txlpf;
	uint16 txgm;
	uint16 pga;
	uint16 pad;
	uint16 ipa;
} acphy_txgains_t;

typedef struct _acphy_papdCalParams {
	uint8 core;
	uint16 startindex;
	uint16 stopindex;
	uint16 yrefindex;
	uint8 epsilon_table_id;
	uint16 num_iter;
} acphy_papdCalParams_t;

#define ACPHY_PAPD_EPS_TBL_SIZE		64
#define WBPAPD_REGLIST_SIZE 11
/* PAPD MODE CONFIGURATION */
#define PAPD_LMS 0
#define PAPD_ANALYTIC 1
#define PAPD_ANALYTIC_WO_YREF 2
#define PAPD_WB 3

extern int phy_ac_papd_cal_run(phy_info_t *pi);
extern void wlc_phy_papd_set_rfpwrlut(phy_info_t *pi);
extern void wlc_phy_papd_set_rfpwrlut_tiny(phy_info_t *pi);
extern void wlc_phy_papd_set_rfpwrlut_phymaj_rev36(phy_info_t *pi);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
extern void wlc_phy_papd_dump_eps_trace_acphy(phy_info_t *pi, struct bcmstrbuf *b);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */
extern void wlc_phy_do_papd_cal_acphy(phy_info_t *pi);
extern void wlc_phy_get_papd_cal_pwr_acphy(phy_info_t *pi, int8 *targetpwr, int8 *tx_idx,
                                           uint8 core);

/* APAPD_ARRAYSIZE = corr_end - startindex + 1 */
#define APAPD_ARRAYSIZE 76

void phy_ac_papdcal_cal_init(phy_info_t *pi);
void phy_ac_papdcal(phy_info_t *pi);
#ifdef PHYCAL_CACHING
void phy_ac_papdcal_save_cache(phy_ac_papdcal_info_t *papdcali, ch_calcache_t *ctx);
#endif /* PHYCAL_CACHING */
void phy_ac_papdcal_get_gainparams(phy_ac_papdcal_info_t *i, uint8 *Gainoverwrite, uint8 *PAgain);
#endif /* _phy_ac_papdcal_h_ */
