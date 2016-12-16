/*
 * Radio 20696 channel tuning header file
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
 * $Id: wlc_phytbl_20694.h 618585 2016-02-11 18:13:22Z $
 */

#ifndef _WLC_PHYTBL_20696_H_
#define _WLC_PHYTBL_20696_H_

#include <wlc_cfg.h>
#include <typedefs.h>

#include "wlc_phy_int.h"

typedef struct _chan_info_radio20696_rffe_2G {
	/* 2G tuning data */
	uint8 RFP0_logen_reg1_logen_mix_ctune;
	uint8 RF0_logen_core_reg3_logen_lc_ctune;
	uint8 RF0_rxadc_reg2_rxadc_ti_ctune_Q;
	uint8 RF0_rxadc_reg2_rxadc_ti_ctune_I;
	uint8 RF0_tx2g_mix_reg4_mx2g_tune;
	uint8 RF0_tx2g_pad_reg3_pad2g_tune;
	uint8 RF0_lna2g_reg1_lna2g_lna1_freq_tune;
	uint8 RF1_logen_core_reg3_logen_lc_ctune;
	uint8 RF1_tx2g_mix_reg4_mx2g_tune;
	uint8 RF1_tx2g_pad_reg3_pad2g_tune;
	uint8 RF1_lna2g_reg1_lna2g_lna1_freq_tune;
	uint8 RF2_logen_core_reg3_logen_lc_ctune;
	uint8 RF2_tx2g_mix_reg4_mx2g_tune;
	uint8 RF2_tx2g_pad_reg3_pad2g_tune;
	uint8 RF2_lna2g_reg1_lna2g_lna1_freq_tune;
	uint8 RF3_logen_core_reg3_logen_lc_ctune;
	uint8 RF3_tx2g_mix_reg4_mx2g_tune;
	uint8 RF3_tx2g_pad_reg3_pad2g_tune;
	uint8 RF3_lna2g_reg1_lna2g_lna1_freq_tune;
} chan_info_radio20696_rffe_2G_t;

typedef struct _chan_info_radio20696_rffe_5G {
	/* 5G tuning data */
	uint8 RFP0_logen_reg1_logen_mix_ctune;
	uint8 RF0_logen_core_reg3_logen_lc_ctune;
	uint8 RF0_rxadc_reg2_rxadc_ti_ctune_Q;
	uint8 RF0_rxadc_reg2_rxadc_ti_ctune_I;
	uint8 RF0_tx5g_mix_reg2_mx5g_tune;
	uint8 RF0_tx5g_pa_reg4_tx5g_pa_tune;
	uint8 RF0_tx5g_pad_reg3_pad5g_tune;
	uint8 RF0_rx5g_reg1_rx5g_lna_tune;
	uint8 RF1_logen_core_reg3_logen_lc_ctune;
	uint8 RF1_tx5g_mix_reg2_mx5g_tune;
	uint8 RF1_tx5g_pa_reg4_tx5g_pa_tune;
	uint8 RF1_tx5g_pad_reg3_pad5g_tune;
	uint8 RF1_rx5g_reg1_rx5g_lna_tune;
	uint8 RF2_logen_core_reg3_logen_lc_ctune;
	uint8 RF2_tx5g_mix_reg2_mx5g_tune;
	uint8 RF2_tx5g_pa_reg4_tx5g_pa_tune;
	uint8 RF2_tx5g_pad_reg3_pad5g_tune;
	uint8 RF2_rx5g_reg1_rx5g_lna_tune;
	uint8 RF3_logen_core_reg3_logen_lc_ctune;
	uint8 RF3_tx5g_mix_reg2_mx5g_tune;
	uint8 RF3_tx5g_pa_reg4_tx5g_pa_tune;
	uint8 RF3_tx5g_pad_reg3_pad5g_tune;
	uint8 RF3_rx5g_reg1_rx5g_lna_tune;
} chan_info_radio20696_rffe_5G_t;

typedef struct _chan_info_radio20696_rffe {
	uint16 channel;
	uint16 freq;
	union {
		/* In this union, make sure the largest struct is at the top. */
		chan_info_radio20696_rffe_5G_t val_5G;
		chan_info_radio20696_rffe_2G_t val_2G;
	} u;
} chan_info_radio20696_rffe_t;

extern const chan_info_radio20696_rffe_t
	chan_tune_20696_rev0[];

extern const uint16 chan_tune_20696_rev0_length;

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
#if defined(DBG_PHY_IOV)
extern radio_20xx_dumpregs_t dumpregs_20696_rev0[];

#endif	
#endif	/* BCMDBG || BCMDBG_DUMP */

/* Radio referred values tables */
extern radio_20xx_prefregs_t prefregs_20696_rev0[];

#endif	/* _WLC_PHYTBL_20696_H_ */
