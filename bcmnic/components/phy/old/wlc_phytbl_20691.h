/*
 * Declarations for Broadcom PHY core tables,
 * Networking Adapter Device Driver.
 *
 * THE CONTENTS OF THIS FILE IS TEMPORARY.
 * Eventually it'll be auto-generated.
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
 * $Id: wlc_phytbl_20691.h 614873 2016-01-25 06:02:55Z $
 */

#ifndef _WLC_PHYTBL_20691_H_
#define _WLC_PHYTBL_20691_H_

#include <wlc_cfg.h>
#include <typedefs.h>

#include "wlc_phy_int.h"

/*
 * Channel Info table for the 20691 (4345).
 */

typedef struct _chan_info_radio20691 {
	uint16 chan;            /* channel number */
	uint16 freq;            /* in Mhz */

	/* other stuff */
	uint16 RF_pll_vcocal1;
	uint16 RF_pll_vcocal11;
	uint16 RF_pll_vcocal12;
	uint16 RF_pll_frct2;
	uint16 RF_pll_frct3;
	uint16 RF_pll_lf4;
	uint16 RF_pll_lf5;
	uint16 RF_pll_lf7;
	uint16 RF_pll_lf2;
	uint16 RF_pll_lf3;
	uint16 RF_logen_cfg1;
	uint16 RF_logen_cfg2;
	uint16 RF_lna2g_tune;
	uint16 RF_txmix2g_cfg5;
	uint16 RF_pa2g_cfg2;
	uint16 RF_lna5g_tune;
	uint16 RF_txmix5g_cfg6;
	uint16 RF_pa5g_cfg4;
} chan_info_radio20691_t;

#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && defined(DBG_PHY_IOV)) || \
	defined(BCMDBG_PHYDUMP)
extern const radio_20xx_dumpregs_t dumpregs_20691_rev68[];
extern const radio_20xx_dumpregs_t dumpregs_20691_rev75[];
extern const radio_20xx_dumpregs_t dumpregs_20691_rev79[];
extern const radio_20xx_dumpregs_t dumpregs_20691_rev82[];
extern const radio_20xx_dumpregs_t dumpregs_20691_rev88[];
extern const radio_20xx_dumpregs_t dumpregs_20691_rev129[];
#endif 

/* Radio referred values tables */
extern const radio_20xx_prefregs_t prefregs_20691_rev68[];
extern const radio_20xx_prefregs_t prefregs_20691_rev75[];
extern const radio_20xx_prefregs_t prefregs_20691_rev79[];
extern const radio_20xx_prefregs_t prefregs_20691_rev82[];
extern const radio_20xx_prefregs_t prefregs_20691_rev88[];
extern const radio_20xx_prefregs_t prefregs_20691_rev129[];

/* Radio tuning values tables */
extern const chan_info_radio20691_t chan_tuning_20691_rev68[59];
extern const chan_info_radio20691_t chan_tuning_20691_rev75[59];
extern const chan_info_radio20691_t chan_tuning_20691_rev79[59];
extern const chan_info_radio20691_t chan_tuning_20691_rev82[59];
extern const chan_info_radio20691_t chan_tuning_20691_rev88[59];
extern const chan_info_radio20691_t chan_tuning_20691_rev129[59];
extern const chan_info_radio20691_t chan_tuning_20691_rev130[59];

#endif	/* _WLC_PHYTBL_20691_H_ */
