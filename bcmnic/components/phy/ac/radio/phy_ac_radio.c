/*
 * ACPHY RADIO contorl module implementation
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
 * $Id: phy_ac_radio.c 664725 2016-10-13 15:23:03Z mvermeid $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_radio.h>
#include "phy_type_radio.h"
#include <phy_ac.h>
#include <phy_ac_radio.h>
#include <phy_ac_misc.h>
#include <phy_ac_tbl.h>
#include <phy_papdcal.h>
#include <phy_tpc.h>
#include <phy_utils_reg.h>
#include <wlc_phy_radio.h>
#include <wlc_radioreg_20691.h>
#include <wlc_phytbl_ac.h>
#include <wlc_phytbl_20691.h>
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20694.h>
#include <wlc_radioreg_20695.h>
#include <wlc_radioreg_20696.h>
#include <wlc_phyreg_ac.h>
#include <sbchipc.h>
#include <hndpmu.h>
#include <wlc_phytbl_20693.h>
#include <wlc_phy_ac_gains.h>
#include "wlc_phytbl_20694.h"
#include "wlc_phytbl_20695.h"
#include "wlc_phytbl_20696.h"
#include "phy_ac_tpc.h"
#include "bcmutils.h"
#include "phy_ac_pllconfig_20695.h"
#include "phy_ac_pllconfig_20694.h"
#include "phy_ac_pllconfig_20696.h"
#include "phy_utils_var.h"
#include "phy_rstr.h"

#ifdef ATE_BUILD
#include <wl_ate.h>
#endif
#include <phy_dbg.h>

#include <phy_rstr.h>
#include <bcmdevs.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_hal.h>
/* TODO: all these are going away... > */
#endif
#include <phy_ac_info.h>
#include <bcmotp.h>

typedef enum {
	TINY_RCAL_MODE0_OTP = 0,
	TINY_RCAL_MODE1_STATIC,
	TINY_RCAL_MODE2_SINGLE_CORE_CAL,
	TINY_RCAL_MODE3_BOTH_CORE_CAL
} acphy_tiny_rcal_modes_t;

#ifdef PHY_DUMP_BINARY
/* AUTOGENRATED by the tool radioreglist.py, see:
 * http://hwnbu-twiki.sj.broadcom.com/bin/view/Mwgroup/AcphyDriver
 * These values cannot be in const memory since
 * the const area might be over-written in case of
 * crash dump
 */
phyradregs_list_t rad20693_majorrev2_registers[] = {
	{0x0,  {0x80, 0x0, 0x0, 0x89}},
	{0x8a,  {0x80, 0x0, 0x0, 0xe7}},
	{0x200,  {0x80, 0x0, 0x0, 0x89}},
	{0x28a,  {0x80, 0x0, 0x0, 0xe7}},
};

phyradregs_list_t rad20696_majorrev0_registers[] = {
        {0x0,  {0x1, 0x80, 0x1f, 0xff}},
        {0x28,  {0x7f, 0xe0, 0x0, 0x7}},
        {0x47,  {0x80, 0x0, 0x0, 0x26}},
        {0x6e,  {0x5, 0xff, 0xfd, 0xff}},
        {0x93,  {0x7d, 0xb7, 0xf0, 0xf3}},
        {0xb2,  {0x3f, 0xff, 0xc0, 0x1}},
        {0xd1,  {0x0, 0x0, 0x0, 0x3f}},
        {0x100,  {0x0, 0x0, 0x0, 0x1}},
        {0x1c1,  {0x7f, 0xff, 0xfd, 0xff}},
        {0x1e0,  {0x3f, 0xfe, 0x7c, 0x1f}},
        {0x1ff,  {0x3, 0x0, 0x3f, 0xff}},
        {0x228,  {0x7f, 0xe0, 0x0, 0x7}},
        {0x247,  {0x80, 0x0, 0x0, 0x26}},
        {0x26e,  {0x7f, 0xff, 0xfd, 0xff}},
        {0x28d,  {0x6d, 0xfc, 0x3d, 0xff}},
        {0x2ad,  {0x7f, 0xf8, 0x0, 0x3f}},
        {0x2cc,  {0x40, 0x0, 0x7, 0xef}},
        {0x2eb,  {0x0, 0x20, 0x2f, 0xff}},
        {0x3c1,  {0x80, 0x0, 0x0, 0x2e}},
        {0x3f1,  {0xf, 0xff, 0xff, 0xff}},
        {0x417,  {0x0, 0xe, 0x0, 0x3}},
        {0x43d,  {0x80, 0x0, 0x0, 0x30}},
        {0x46e,  {0x5, 0xff, 0xfd, 0xff}},
        {0x493,  {0x7d, 0xb7, 0xf0, 0xf3}},
        {0x4b2,  {0x3f, 0xff, 0xc0, 0x1}},
        {0x4d1,  {0x0, 0x0, 0x0, 0x3f}},
        {0x500,  {0x0, 0x0, 0x0, 0x1}},
        {0x5c1,  {0x7f, 0xff, 0xfd, 0xff}},
        {0x5e0,  {0x3f, 0xfe, 0xfc, 0x1f}},
        {0x5ff,  {0x3, 0x0, 0x3f, 0xff}},
        {0x628,  {0x7f, 0xe0, 0x0, 0x7}},
        {0x647,  {0x80, 0x0, 0x0, 0x26}},
        {0x66e,  {0xf, 0xff, 0xfd, 0xff}},
        {0x693,  {0x7d, 0xb7, 0xf0, 0xf3}},
        {0x6b2,  {0x3f, 0xff, 0xc0, 0x1}},
        {0x6d1,  {0x0, 0x0, 0x0, 0x3f}},
        {0x700,  {0x0, 0x0, 0x0, 0x1}},
        {0x7c1,  {0x7f, 0xff, 0xfd, 0xff}},
        {0x7e0,  {0x3f, 0xff, 0xfc, 0x1f}},
        {0x7ff,  {0x7e, 0x0, 0x3f, 0xff}},
        {0x81e,  {0x3f, 0xff, 0x7f, 0xff}},
        {0x83e,  {0x80, 0x0, 0x0, 0x3c}},
        {0x9f5,  {0x0, 0x0, 0x7, 0xff}},
        {0xc00,  {0x1, 0x80, 0x1f, 0xff}},
        {0xc28,  {0x7f, 0xe0, 0x0, 0x7}},
        {0xc47,  {0x80, 0x0, 0x0, 0x26}},
        {0xc6e,  {0x5, 0xff, 0xfd, 0xff}},
        {0xc93,  {0x7d, 0xb7, 0xf0, 0xf3}},
        {0xcb2,  {0x3f, 0xff, 0xc0, 0x1}},
        {0xcd1,  {0x0, 0x0, 0x0, 0x3f}},
        {0xd00,  {0x0, 0x0, 0x0, 0x1}},
        {0xdc1,  {0x7f, 0xff, 0xfd, 0xff}},
        {0xde0,  {0x3f, 0xfe, 0x7c, 0x1f}},
        {0xdff,  {0x0, 0x0, 0x0, 0x1}},
};
#endif

#define MAX_2069_RCCAL_WAITLOOPS 100
#define NUM_2069_RCCAL_CAPS 3

/* module private states */
struct phy_ac_radio_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_radio_info_t *ri;
	acphy_pmu_core1_off_radregs_t *pmu_c1_off_info_orig;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig;
	/* shared data */
	phy_ac_radio_data_t data;
	uint8	rccal_adc_rc;
	uint32	rccal_dacbuf_cmult;
	uint16	rccal_adc_gmult;
	/* std params */
	uint16	rxRfctrlCoreRxPus0;
	uint16	rxRfctrlOverrideRxPus0;
	uint16	afeRfctrlCoreAfeCfg10;
	uint16	afeRfctrlCoreAfeCfg20;
	uint16	afeRfctrlOverrideAfeCfg0;
	uint16	txRfctrlCoreTxPus0;
	uint16	txRfctrlOverrideTxPus0;
	uint16	radioRfctrlCmd;
	uint16	radioRfctrlCoreGlobalPus;
	uint16	radioRfctrlOverrideGlobalPus;
	uint8	crisscross_priority_core_80p80;
	uint8	crisscross_priority_core_rsdb;
	uint8	is_crisscross_actv;
	uint8	prev_subband;
	/* To select the low power mode */
	uint8	acphy_lp_mode;
	uint8	acphy_prev_lp_mode;
	acphy_lp_modes_t	lpmode_2g;
	acphy_lp_modes_t	lpmode_5g;
	/* add other variable size variables here at the end */
};

typedef struct {
	uint16 gi;
	uint16 g21;
	uint16 g32;
	uint16 g43;
	uint16 r12;
	uint16 r34;
	uint16 gff1;
	uint16 gff2;
	uint16 gff3;
	uint16 gff4;
	uint16 g11;
	uint16 ri3;
	uint16 g54;
	uint16 g65;
} tiny_adc_tuning_array_t;

/* local functions */
static void phy_ac_radio_std_params_attach(phy_ac_radio_info_t *info);
static void wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(phy_info_t *pi);
static void phy_ac_radio_switch(phy_type_radio_ctx_t *ctx, bool on);
static void phy_ac_radio_on(phy_type_radio_ctx_t *ctx);
static void phy_ac_radio_init_lpmode(phy_ac_radio_info_t *ri);
#ifdef PHY_DUMP_BINARY
static int phy_ac_radio_getlistandsize(phy_type_radio_ctx_t *ctx, phyradregs_list_t **radreglist,
	uint16 *radreglist_sz);
#endif
#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && (defined(BCMINTERNAL) || \
	defined(DBG_PHY_IOV))) || defined(BCMDBG_PHYDUMP)
static int phy_ac_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_ac_radio_dump NULL
#endif /* BCMDBG, BCMDBG_DUMP, BCMINTERNAL, DBG_PHY_IOV, BCMDBG_PHYDUMP */

/* 20693 tuning Table related */
static void wlc_phy_radio20693_pll_tune(phy_info_t *pi, const void *chan_info_pll,
	uint8 core);
static void wlc_phy_radio20693_rffe_tune(phy_info_t *pi, const void *chan_info_rffe,
	uint8 core);
static void wlc_phy_radio20693_pll_tune_wave2(phy_info_t *pi, const void *chan_info_pll,
	uint8 core);
static void wlc_phy_radio20693_rffe_rsdb_wr(phy_info_t *pi, uint8 core, uint8 reg_type,
	uint16 reg_val, uint16 reg_off);
static void wlc_phy_radio20693_set_logencore_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core);
static void wlc_phy_radio20693_set_rfpll_vco_12g_buf_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core);
static void wlc_phy_radio20693_afeclkdiv_12g_sel(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core);
static void wlc_phy_radio20693_afe_clk_div_mimoblk_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode);
static void wlc_phy_radio20693_set_logen_mimosrcdes_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode, uint16 mac_mode);
static void wlc_phy_radio20693_tx2g_set_freq_tuning_ipa_as_epa(phy_info_t *pi,
	uint8 core, uint8 chan);
static void wlc_phy_set_regtbl_on_pwron_acphy(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm1_lpopt(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm1_lpopt_restore(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm23_lp_opt(phy_info_t *pi, uint8 coremask);
static void wlc_phy_radio20693_mimo_cm23_lp_opt_restore(phy_info_t *pi);
static void wlc_phy_radio20693_afecal(phy_info_t *pi);

/** 20696 ADC cap cal modes */
#define ADC_CAP_CAL_MODE_BYPASS		0
#define ADC_CAP_CAL_MODE_PARALLEL	1
#define ADC_CAP_CAL_MODE_SEQUENTIAL	2

static void
wlc_phy_radio20696_adc_setup(phy_info_t *pi, uint8 core);
static void
wlc_phy_radio20696_adc_cap_cal(phy_info_t *pi, int mode);
static void
wlc_phy_radio20696_adc_cap_cal_setup(phy_info_t *pi, uint8 core);
static void
wlc_phy_radio20696_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout);
static void
wlc_phy_radio20696_adc_cap_cal_sequential(phy_info_t *pi, uint8 core, uint16 *timeout);
static void
wlc_phy_radio20696_apply_adc_cal_result(phy_info_t *pi, uint8 core);

/* Cleanup Chanspec */
static void chanspec_setup_radio_20693(phy_info_t *pi);
static void chanspec_setup_radio_20691(phy_info_t *pi);
static void chanspec_setup_radio_2069(phy_info_t *pi);
static void chanspec_setup_radio_20695(phy_info_t *pi);
static void chanspec_setup_xtal_20695(phy_info_t *pi);
static void chanspec_setup_radio_20694(phy_info_t *pi);
static void chanspec_setup_radio_20696(phy_info_t *pi);
static void chanspec_tune_radio_20693(phy_info_t *pi);
static void chanspec_tune_radio_20691(phy_info_t *pi);
static void chanspec_tune_radio_2069(phy_info_t *pi);
static void chanspec_tune_radio_20695(phy_info_t *pi);
static void chanspec_tune_radio_20694(phy_info_t *pi);

static void wlc_phy_chanspec_radio20691_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset);
static void wlc_phy_chanspec_radio2069_setup(phy_info_t *pi, const void *chan_info,
	uint8 toggle_logen_reset);
static void wlc_phy_chanspec_radio20694_setup(phy_info_t *pi, uint8 ch,
	uint8 toggle_logen_reset, uint8 core);
static void wlc_phy_chanspec_radio20695_setup(phy_info_t *pi, uint8 ch,
	uint8 toggle_logen_reset, uint8 core);
static void wlc_phy_chanspec_radio20696_setup(phy_info_t *pi, uint8 ch,
	uint8 toggle_logen_reset);
static int BCMRAMFN(wlc_phy_ac_get_20695_chan_info_tbl)(phy_info_t *pi,
	const chan_info_radio20695_rffe_t **chan_info, uint32 *p_tbl_len);
static int BCMRAMFN(wlc_phy_ac_get_20694_chan_info_tbl)(phy_info_t *pi,
	const chan_info_radio20694_rffe_t **chan_info, uint32 *p_tbl_len);


static pll_config_20695_tbl_t *
BCMRAMFN(wlc_phy_ac_get_20695_pll_config)(phy_info_t *pi);
static pll_config_20694_tbl_t *
BCMRAMFN(wlc_phy_ac_get_20694_pll_config)(phy_info_t *pi);
static radio_20xx_prefregs_t *
BCMRAMFN(wlc_phy_ac_get_20695_prfd_vals_tbl)(phy_info_t *pi);

static radio_20xx_prefregs_t *
BCMRAMFN(wlc_phy_ac_get_20694_prfd_vals_tbl)(phy_info_t *pi);

#define RADIO20693_TUNE_REG_WR_SHIFT 0
#define RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT 1
/* Define affinities for each core
for 4349A0:
core 0's affinity: "2g"
core 1's affinity: "5g"
*/
#define RADIO20693_CORE0_AFFINITY 2
#define RADIO20693_CORE1_AFFINITY 5

/* Tuning reg type */
#define RADIO20693_TUNEREG_TYPE_2G 2
#define RADIO20693_TUNEREG_TYPE_5G 5
#define RADIO20693_TUNEREG_TYPE_NONE 0


#define RADIO20693_NUM_PLL_TUNE_REGS 32
/* Rev5,7 & 6,8 */
#define RADIO20693_NUM_RFFE_TUNE_REGS 3

#define BCM4349_UMC_FAB_ID 5
#define BCM4349_TSMC_FAB_ID 2
#define UMC_FAB_RCAL_VAL 8
#define TSMC_FAB_RCAL_VAL 11

static const uint8 rffe_tune_reg_map_20693[RADIO20693_NUM_RFFE_TUNE_REGS] = {
	RADIO20693_TUNEREG_TYPE_2G,
	RADIO20693_TUNEREG_TYPE_5G,
	RADIO20693_TUNEREG_TYPE_2G
};

typedef enum {
	PLL2G,
	PLL5G,
	LOGEN5G,
	LOGEN5GTO2G
} pll_logen_block_20695_t;

/* TIA LUT tables to be used in wlc_tiny_tia_config() */
static const uint8 tiaRC_tiny_8b_20[]= { /* LUT 0--51 (20 MHz) */
	0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff,
	0xb7, 0xb5, 0x97, 0x81, 0xe5, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x14, 0x1d, 0x28, 0x34, 0x34, 0x34,
	0x34, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40,
	0x5b, 0x6c, 0x80, 0x80
};

static const uint8 tiaRC_tiny_8b_40[]= { /* LUT 0--51 (40 MHz) */
	0xff, 0xe1, 0xb9, 0x81, 0x5d, 0xff, 0xad, 0x82,
	0x6d, 0x5d, 0x4c, 0x41, 0x73, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x16, 0x16, 0x1b, 0x1d, 0x1d, 0x1f,
	0x20, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x4c,
	0x5b, 0x6c, 0x80, 0x80
};

static const uint8 tiaRC_tiny_8b_80[]= { /* LUT 0--51 (80 MHz) */
	0xc2, 0x86, 0x5d, 0xff, 0xbe, 0x88, 0x5f, 0x43,
	0x37, 0x2e, 0x26, 0x20, 0x39, 0x00, 0x00, 0x00,
	0x0b, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0d,
	0x0d, 0x07, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x4c,
	0x5b, 0x6c, 0x80, 0x80
};

static const uint16 tiaRC_tiny_16b_20[]= { /* LUT 52--82 (20 MHz) */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00b5, 0x0080, 0x005a,
	0x0040, 0x002d, 0x0020, 0x0017, 0x0000, 0x0100, 0x00b5, 0x0080,
	0x005b, 0x0040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0080, 0x001f, 0x1fe0, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_20_rem[]= { /* LUT 79--82 (20 MHz) */
	0x001f, 0x1fe0, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_20_rem_war[]= { /* LUT 79--82 (20 MHz) */
	0x001f, 0x1fe0, 0xc500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_80[]= { /* LUT 52--82 (80 MHz) */
	0x0000, 0x0000, 0x0000, 0x016b, 0x0100, 0x00b6, 0x0080, 0x005a,
	0x0040, 0x002d, 0x0020, 0x0017, 0x0000, 0x0100, 0x00b5, 0x0080,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0080, 0x0007, 0x1ff8, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_80_rem[]= { /* LUT 79--82 (80 MHz) */
	0x0007, 0x1ff8, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_80_rem_war[]= { /* LUT 79--82 (80 MHz) */
	0x0007, 0x1ff8, 0xc500, 0x00ff
};

static const uint16 *tiaRC_tiny_16b_40 = tiaRC_tiny_16b_20;  /* LUT 52--78 (40 MHz) (no LUT 67) */
static const uint16 *tiaRC_tiny_16b_40_rem = tiaRC_tiny_16b_20_rem;  /* LUT 79--82 (40 MHz) */

/* Coefficients generated by 47xxtcl/rgphy/20691/ */
/* lpf_tx_coefficient_generator/filter_tx_tiny_generate_python_and_tcl.py */
static const uint16 lpf_g10[7][15] = {
	{1188, 1527, 1866, 2206, 2545, 2545, 1188, 1188,
	1188, 1188, 1188, 1188, 1188, 1188, 1188},
	{3300, 4242, 5185, 6128, 7071, 7071, 3300, 3300,
	3300, 3300, 3300, 3300, 3300, 3300, 3300},
	{16059, 16059, 16059, 17294, 18529, 18529, 9882,
	1976, 2470, 3088, 3953, 4941, 6176, 7906, 12353},
	{24088, 24088, 25941, 31500, 37059, 37059, 14823,
	2964, 3705, 4632, 5929, 7411, 9264, 11859, 18529},
	{29647, 32118, 34589, 42001, 49412, 49412, 19765,
	3705, 4941, 6176, 7411, 9882, 12353, 14823, 24706},
	{32941, 36236, 39530, 42824, 46118, 49412, 19765,
	4117, 4941, 6588, 8235, 9882, 13176, 16470, 26353},
	{10706, 10706, 10706, 11529, 12353, 12353, 6588,
	1317, 1647, 2058, 2635, 3294, 4117, 5270, 8235}
};
static const uint16 lpf_g12[7][15] = {
	{1882, 1922, 1866, 1752, 1606, 1275, 2984, 14956,
	11880, 9436, 7495, 5954, 4729, 3756, 2370},
	{5230, 5341, 5185, 4868, 4461, 3544, 8289, 41544,
	33000, 26212, 20821, 16539, 13137, 10435, 6584},
	{24872, 19757, 15693, 13424, 11425, 9075, 24258, 24316,
	24144, 23972, 24374, 24201, 24029, 24432, 24086},
	{37309, 29635, 25351, 24452, 22850, 18151, 36388, 36474,
	36216, 35959, 36561, 36302, 36044, 36648, 36130},
	{44360, 38172, 32654, 31496, 29433, 23379, 46870, 44045,
	46648, 46318, 44150, 46759, 46428, 44254, 46538},
	{49288, 43066, 37319, 32113, 27471, 23379, 46870, 48939,
	46648, 49406, 49055, 46759, 49523, 49172, 49640},
	{16581, 13171, 10462, 8949, 7616, 6050, 16172, 16210,
	16096, 15981, 16249, 16134, 16019, 16288, 16057}
};
static const uint16 lpf_g21[7][15] = {
	{1529, 1497, 1542, 1643, 1793, 2257, 965, 192, 242,
	305, 384, 483, 609, 766, 1215},
	{4249, 4160, 4285, 4565, 4981, 6270, 2681, 534, 673,
	847, 1067, 1343, 1691, 2129, 3375},
	{6135, 7723, 9723, 11367, 13356, 16814, 6290, 6275,
	6320, 6365, 6260, 6305, 6350, 6245, 6335},
	{9202, 11585, 13543, 14041, 15025, 18916, 9435, 9413,
	9480, 9548, 9391, 9458, 9525, 9368, 9503},
	{13760, 15990, 18693, 19380, 20738, 26108, 13023, 13858,
	13085, 13178, 13825, 13054, 13147, 13793, 13116},
	{22016, 25197, 29078, 33791, 39502, 46415, 23152, 22173,
	23262, 21964, 22121, 23207, 21912, 22068, 21860},
	{4090, 5149, 6482, 7578, 8904, 11209, 4193, 4183, 4213,
	4243, 4173, 4203, 4233, 4163, 4223}
};
static const uint16 lpf_g11[7] = {994, 2763, 12353, 18529, 17470, 23293, 8235};
static const uint16 g_passive_rc_tx[7] = {62, 172, 772, 1158, 1544, 2058, 514};
static const uint16 biases[7] = {24, 48, 96, 96, 128, 128, 96};
static const int8 g_index1[15] = {0, 1, 2, 3, 4, 5, -2, -9, -8, -7, -6, -5, -4, -3, -1};

static uint8 wlc_phy_radio20693_tuning_get_access_info(phy_info_t *pi, uint8 core, uint8 reg_type);
static uint8 wlc_phy_radio20693_tuning_get_access_type(phy_info_t *pi, uint8 core, uint8 reg_type);

static void wlc_phy_radio20693_minipmu_pwron_seq(phy_info_t *pi);
static void wlc_phy_radio20693_pmu_override(phy_info_t *pi);
static void wlc_phy_radio20693_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20693_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20693_pmu_pll_config(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy_20693(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy(phy_info_t *pi, bool on);
static void wlc_phy_radio2069_mini_pwron_seq_rev16(phy_info_t *pi);
static void wlc_phy_radio2069_mini_pwron_seq_rev32(phy_info_t *pi);
static void wlc_phy_radio2069_pwron_seq(phy_info_t *pi);
static void wlc_phy_tiny_radio_pwron_seq(phy_info_t *pi);
static void wlc_phy_radio2069_rcal(phy_info_t *pi);
static void wlc_phy_radio2069_rccal(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy_20691(phy_info_t *pi);
static void wlc_phy_radio2069_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20691_upd_prfd_values(phy_info_t *pi);
static void WLBANDINITFN(wlc_phy_static_table_download_acphy)(phy_info_t *pi);
static void wlc_phy_radio_tiny_rcal(phy_info_t *pi, acphy_tiny_rcal_modes_t mode);
static void wlc_phy_radio_tiny_rcal_wave2(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio_tiny_rccal(phy_ac_radio_info_t *ri);
static void wlc_phy_radio_tiny_rccal_wave2(phy_ac_radio_info_t *ri);
static void wlc_phy_radio20695_minipmu_pupd_seq(phy_info_t *pi, uint8 pupdbit);
static void wlc_phy_radio20696_minipmu_pwron_seq(phy_info_t *pi);
static void wlc_phy_28nm_radio_pwron_seq_phyregs(phy_info_t *pi, uint8 core);
static void wlc_phy_radio20695_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20694_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy_20695(phy_info_t *pi);
static void wlc_phy_radio20694_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_radio20696_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy_20694(phy_info_t *pi);
int8 wlc_phy_28nm_radio_minipmu_cal(phy_info_t *pi);
int8 wlc_phy_20694_radio_minipmu_cal(phy_info_t *pi);
int8 wlc_phy_20694_radio_minipmu_cal_byaux(phy_info_t *pi);
static void wlc_phy_radio20696_minipmu_cal(phy_info_t *pi);
static void wlc_phy_28nm_radio_rcal(phy_info_t *pi, uint8 mode);
static void wlc_phy_20694_radio_rcal(phy_info_t *pi, uint8 mode);
static void wlc_phy_20694_radio_rcal_byaux(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio20696_r_cal(phy_info_t *pi, uint8 mode);
static void wlc_phy_28nm_radio_rccal(phy_info_t *pi);
static void wlc_phy_radio20696_rccal(phy_info_t *pi);
static void wlc_phy_28nm_radio_pll_logen_pupd_seq(phy_info_t *pi, pll_logen_block_20695_t block,
		uint8 pupdbit, uint8 core);
static void wlc_phy_radio20694_rccal(phy_info_t *pi);
static radio_pll_sel_t
wlc_phy_radio20695_pll_tune(phy_info_t *pi, uint32 chan_freq, uint8 core);
static void
wlc_phy_radio20694_pll_tune(phy_info_t *pi, uint32 chan_freq, uint8 core);
static void wlc_phy_radio20695_rffe_tune(phy_info_t *pi,
		const chan_info_radio20695_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20694_rffe_tune(phy_info_t *pi,
		const chan_info_radio20694_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20696_rffe_tune(phy_info_t *pi,
		const chan_info_radio20696_rffe_t *chan_info_rffe);
static void wlc_phy_radio20694_upd_band_related_reg(phy_info_t *pi);
static void wlc_phy_radio20696_upd_band_related_reg(phy_info_t *pi);
static void phy_ac_radio20695_pll_config_ch_dep_calc(phy_info_t *pi, uint32 lo_freq,
		pll_config_20695_tbl_t *pll);
static void phy_ac_radio20694_pll_config_ch_dep_calc(phy_info_t *pi, uint32 lo_freq,
	uint8 vco_select, uint32 icp_fx, uint32 loop_band,
	uint8 ac_mode, pll_config_20694_tbl_t *pll);
static void phy_ac_radio20695_write_pll_config(phy_info_t *pi, radio_pll_sel_t pll_sel,
		pll_config_20695_tbl_t *pll);
static void phy_ac_radio20694_write_pll_config(phy_info_t *pi, pll_config_20694_tbl_t *pll);
static void wlc_phy_radio20695_pll_logen_pu_config(phy_info_t *pi, uint32 chan_freq, uint8 core);
static void wlc_phy_radio20695_clear_adc_dac_en_ovr(phy_info_t *pi);
static void wlc_phy_radio20696_pmu_pll_pwrup(phy_info_t *pi);
static void wlc_phy_radio20696_wars(phy_info_t *pi);
static void wlc_phy_radio20696_refclk_en(phy_info_t *pi);
static void wlc_phy_radio20696_upd_prfd_values(phy_info_t *pi);


static void BCMATTACHFN(phy_ac_radio20695_pll_config_const_calc)(phy_info_t *pi,
		pll_config_20695_tbl_t *pll);
static int BCMATTACHFN(phy_ac_radio20695_populate_pll_config_tbl)(phy_info_t *pi,
		pll_config_20695_tbl_t *pll);

static void BCMATTACHFN(phy_ac_radio20694_pll_config_const_calc)(phy_info_t *pi,
		pll_config_20694_tbl_t *pll);
static int BCMATTACHFN(phy_ac_radio20694_populate_pll_config_tbl)(phy_info_t *pi,
		pll_config_20694_tbl_t *pll);

static void wlc_phy_radio20694_low_current_mode(phy_info_t *pi, uint8 mode);

/* query radio idcode */
static uint32
_phy_ac_radio_query_idcode(phy_type_radio_ctx_t *ctx)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;

	return phy_ac_radio_query_idcode(pi);
}

/* Structure to hold 20695 PLL config values */
static pll_config_20695_tbl_t pll_conf_20695;
/* Structure to hold 20694 PLL config values */
static pll_config_20694_tbl_t pll_conf_20694;

void print_pll_config_20694(pll_config_20694_tbl_t *pll, uint32 lo_freq,
	uint32 loop_bw, uint32 icp_fx);

static void phy_ac_radio_nvram_attach(phy_info_t *pi, phy_ac_radio_info_t *radioi);

void dump_radioregs_20696(phy_info_t *pi);
void dump_radioregs_20696(phy_info_t *pi) {
	printf("20696: 0x000 0x%04X\n", phy_utils_read_radioreg(pi, 0x000 ));
	// printf("20696: 0x001 0x%04X\n", phy_utils_read_radioreg(pi, 0x001 ));
	printf("20696: 0x001 0x058e\n");
	printf("20696: 0x002 0x%04X\n", phy_utils_read_radioreg(pi, 0x002 ));
	printf("20696: 0x003 0x%04X\n", phy_utils_read_radioreg(pi, 0x003 ));
	printf("20696: 0x004 0x%04X\n", phy_utils_read_radioreg(pi, 0x004 ));
	printf("20696: 0x005 0x%04X\n", phy_utils_read_radioreg(pi, 0x005 ));
	printf("20696: 0x006 0x%04X\n", phy_utils_read_radioreg(pi, 0x006 ));
	printf("20696: 0x007 0x%04X\n", phy_utils_read_radioreg(pi, 0x007 ));
	printf("20696: 0x008 0x%04X\n", phy_utils_read_radioreg(pi, 0x008 ));
	printf("20696: 0x009 0x%04X\n", phy_utils_read_radioreg(pi, 0x009 ));
	printf("20696: 0x00a 0x%04X\n", phy_utils_read_radioreg(pi, 0x00a ));
	printf("20696: 0x00b 0x%04X\n", phy_utils_read_radioreg(pi, 0x00b ));
	printf("20696: 0x00c 0x%04X\n", phy_utils_read_radioreg(pi, 0x00c ));
	printf("20696: 0x017 0x%04X\n", phy_utils_read_radioreg(pi, 0x017 ));
	printf("20696: 0x018 0x%04X\n", phy_utils_read_radioreg(pi, 0x018 ));
	printf("20696: 0x028 0x%04X\n", phy_utils_read_radioreg(pi, 0x028 ));
	printf("20696: 0x029 0x%04X\n", phy_utils_read_radioreg(pi, 0x029 ));
	printf("20696: 0x02a 0x%04X\n", phy_utils_read_radioreg(pi, 0x02a ));
	printf("20696: 0x03d 0x%04X\n", phy_utils_read_radioreg(pi, 0x03d ));
	printf("20696: 0x03e 0x%04X\n", phy_utils_read_radioreg(pi, 0x03e ));
	printf("20696: 0x03f 0x%04X\n", phy_utils_read_radioreg(pi, 0x03f ));
	printf("20696: 0x040 0x%04X\n", phy_utils_read_radioreg(pi, 0x040 ));
	printf("20696: 0x041 0x%04X\n", phy_utils_read_radioreg(pi, 0x041 ));
	printf("20696: 0x042 0x%04X\n", phy_utils_read_radioreg(pi, 0x042 ));
	printf("20696: 0x043 0x%04X\n", phy_utils_read_radioreg(pi, 0x043 ));
	printf("20696: 0x044 0x%04X\n", phy_utils_read_radioreg(pi, 0x044 ));
	printf("20696: 0x045 0x%04X\n", phy_utils_read_radioreg(pi, 0x045 ));
	printf("20696: 0x046 0x%04X\n", phy_utils_read_radioreg(pi, 0x046 ));
	printf("20696: 0x047 0x%04X\n", phy_utils_read_radioreg(pi, 0x047 ));
	printf("20696: 0x048 0x%04X\n", phy_utils_read_radioreg(pi, 0x048 ));
	printf("20696: 0x049 0x%04X\n", phy_utils_read_radioreg(pi, 0x049 ));
	printf("20696: 0x04a 0x%04X\n", phy_utils_read_radioreg(pi, 0x04a ));
	printf("20696: 0x04b 0x%04X\n", phy_utils_read_radioreg(pi, 0x04b ));
	printf("20696: 0x04c 0x%04X\n", phy_utils_read_radioreg(pi, 0x04c ));
	printf("20696: 0x04d 0x%04X\n", phy_utils_read_radioreg(pi, 0x04d ));
	printf("20696: 0x04e 0x%04X\n", phy_utils_read_radioreg(pi, 0x04e ));
	printf("20696: 0x04f 0x%04X\n", phy_utils_read_radioreg(pi, 0x04f ));
	printf("20696: 0x050 0x%04X\n", phy_utils_read_radioreg(pi, 0x050 ));
	printf("20696: 0x051 0x%04X\n", phy_utils_read_radioreg(pi, 0x051 ));
	printf("20696: 0x052 0x%04X\n", phy_utils_read_radioreg(pi, 0x052 ));
	printf("20696: 0x053 0x%04X\n", phy_utils_read_radioreg(pi, 0x053 ));
	printf("20696: 0x054 0x%04X\n", phy_utils_read_radioreg(pi, 0x054 ));
	printf("20696: 0x055 0x%04X\n", phy_utils_read_radioreg(pi, 0x055 ));
	printf("20696: 0x056 0x%04X\n", phy_utils_read_radioreg(pi, 0x056 ));
	printf("20696: 0x057 0x%04X\n", phy_utils_read_radioreg(pi, 0x057 ));
	printf("20696: 0x058 0x%04X\n", phy_utils_read_radioreg(pi, 0x058 ));
	printf("20696: 0x059 0x%04X\n", phy_utils_read_radioreg(pi, 0x059 ));
	printf("20696: 0x05a 0x%04X\n", phy_utils_read_radioreg(pi, 0x05a ));
	printf("20696: 0x05b 0x%04X\n", phy_utils_read_radioreg(pi, 0x05b ));
	printf("20696: 0x05c 0x%04X\n", phy_utils_read_radioreg(pi, 0x05c ));
	printf("20696: 0x05d 0x%04X\n", phy_utils_read_radioreg(pi, 0x05d ));
	printf("20696: 0x05e 0x%04X\n", phy_utils_read_radioreg(pi, 0x05e ));
	printf("20696: 0x05f 0x%04X\n", phy_utils_read_radioreg(pi, 0x05f ));
	printf("20696: 0x060 0x%04X\n", phy_utils_read_radioreg(pi, 0x060 ));
	printf("20696: 0x061 0x%04X\n", phy_utils_read_radioreg(pi, 0x061 ));
	printf("20696: 0x062 0x%04X\n", phy_utils_read_radioreg(pi, 0x062 ));
	printf("20696: 0x063 0x%04X\n", phy_utils_read_radioreg(pi, 0x063 ));
	printf("20696: 0x064 0x%04X\n", phy_utils_read_radioreg(pi, 0x064 ));
	printf("20696: 0x065 0x%04X\n", phy_utils_read_radioreg(pi, 0x065 ));
	printf("20696: 0x066 0x%04X\n", phy_utils_read_radioreg(pi, 0x066 ));
	printf("20696: 0x067 0x%04X\n", phy_utils_read_radioreg(pi, 0x067 ));
	printf("20696: 0x068 0x%04X\n", phy_utils_read_radioreg(pi, 0x068 ));
	printf("20696: 0x069 0x%04X\n", phy_utils_read_radioreg(pi, 0x069 ));
	printf("20696: 0x06a 0x%04X\n", phy_utils_read_radioreg(pi, 0x06a ));
	printf("20696: 0x06b 0x%04X\n", phy_utils_read_radioreg(pi, 0x06b ));
	printf("20696: 0x06c 0x%04X\n", phy_utils_read_radioreg(pi, 0x06c ));
	printf("20696: 0x06e 0x%04X\n", phy_utils_read_radioreg(pi, 0x06e ));
	printf("20696: 0x06f 0x%04X\n", phy_utils_read_radioreg(pi, 0x06f ));
	printf("20696: 0x070 0x%04X\n", phy_utils_read_radioreg(pi, 0x070 ));
	printf("20696: 0x071 0x%04X\n", phy_utils_read_radioreg(pi, 0x071 ));
	printf("20696: 0x072 0x%04X\n", phy_utils_read_radioreg(pi, 0x072 ));
	printf("20696: 0x073 0x%04X\n", phy_utils_read_radioreg(pi, 0x073 ));
	printf("20696: 0x074 0x%04X\n", phy_utils_read_radioreg(pi, 0x074 ));
	printf("20696: 0x075 0x%04X\n", phy_utils_read_radioreg(pi, 0x075 ));
	printf("20696: 0x076 0x%04X\n", phy_utils_read_radioreg(pi, 0x076 ));
	printf("20696: 0x078 0x%04X\n", phy_utils_read_radioreg(pi, 0x078 ));
	printf("20696: 0x079 0x%04X\n", phy_utils_read_radioreg(pi, 0x079 ));
	printf("20696: 0x07a 0x%04X\n", phy_utils_read_radioreg(pi, 0x07a ));
	printf("20696: 0x07b 0x%04X\n", phy_utils_read_radioreg(pi, 0x07b ));
	printf("20696: 0x07c 0x%04X\n", phy_utils_read_radioreg(pi, 0x07c ));
	printf("20696: 0x07d 0x%04X\n", phy_utils_read_radioreg(pi, 0x07d ));
	printf("20696: 0x07e 0x%04X\n", phy_utils_read_radioreg(pi, 0x07e ));
	printf("20696: 0x07f 0x%04X\n", phy_utils_read_radioreg(pi, 0x07f ));
	printf("20696: 0x080 0x%04X\n", phy_utils_read_radioreg(pi, 0x080 ));
	printf("20696: 0x081 0x%04X\n", phy_utils_read_radioreg(pi, 0x081 ));
	printf("20696: 0x082 0x%04X\n", phy_utils_read_radioreg(pi, 0x082 ));
	printf("20696: 0x083 0x%04X\n", phy_utils_read_radioreg(pi, 0x083 ));
	printf("20696: 0x084 0x%04X\n", phy_utils_read_radioreg(pi, 0x084 ));
	printf("20696: 0x085 0x%04X\n", phy_utils_read_radioreg(pi, 0x085 ));
	printf("20696: 0x086 0x%04X\n", phy_utils_read_radioreg(pi, 0x086 ));
	printf("20696: 0x088 0x%04X\n", phy_utils_read_radioreg(pi, 0x088 ));
	printf("20696: 0x093 0x%04X\n", phy_utils_read_radioreg(pi, 0x093 ));
	printf("20696: 0x094 0x%04X\n", phy_utils_read_radioreg(pi, 0x094 ));
	printf("20696: 0x097 0x%04X\n", phy_utils_read_radioreg(pi, 0x097 ));
	printf("20696: 0x098 0x%04X\n", phy_utils_read_radioreg(pi, 0x098 ));
	printf("20696: 0x099 0x%04X\n", phy_utils_read_radioreg(pi, 0x099 ));
	printf("20696: 0x09a 0x%04X\n", phy_utils_read_radioreg(pi, 0x09a ));
	printf("20696: 0x09f 0x%04X\n", phy_utils_read_radioreg(pi, 0x09f ));
	printf("20696: 0x0a0 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a0 ));
	printf("20696: 0x0a1 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a1 ));
	printf("20696: 0x0a2 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a2 ));
	printf("20696: 0x0a3 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a3 ));
	printf("20696: 0x0a4 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a4 ));
	printf("20696: 0x0a5 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a5 ));
	printf("20696: 0x0a7 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a7 ));
	printf("20696: 0x0a8 0x%04X\n", phy_utils_read_radioreg(pi, 0x0a8 ));
	printf("20696: 0x0aa 0x%04X\n", phy_utils_read_radioreg(pi, 0x0aa ));
	printf("20696: 0x0ab 0x%04X\n", phy_utils_read_radioreg(pi, 0x0ab ));
	printf("20696: 0x0ad 0x%04X\n", phy_utils_read_radioreg(pi, 0x0ad ));
	printf("20696: 0x0ae 0x%04X\n", phy_utils_read_radioreg(pi, 0x0ae ));
	printf("20696: 0x0af 0x%04X\n", phy_utils_read_radioreg(pi, 0x0af ));
	printf("20696: 0x0b0 0x%04X\n", phy_utils_read_radioreg(pi, 0x0b0 ));
	printf("20696: 0x0b1 0x%04X\n", phy_utils_read_radioreg(pi, 0x0b1 ));
	printf("20696: 0x0b2 0x%04X\n", phy_utils_read_radioreg(pi, 0x0b2 ));
	printf("20696: 0x0c0 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c0 ));
	printf("20696: 0x0c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c1 ));
	printf("20696: 0x0c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c2 ));
	printf("20696: 0x0c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c3 ));
	printf("20696: 0x0c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c4 ));
	printf("20696: 0x0c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c5 ));
	printf("20696: 0x0c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c6 ));
	printf("20696: 0x0c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c7 ));
	printf("20696: 0x0c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c8 ));
	printf("20696: 0x0c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x0c9 ));
	printf("20696: 0x0ca 0x%04X\n", phy_utils_read_radioreg(pi, 0x0ca ));
	printf("20696: 0x0cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x0cb ));
	printf("20696: 0x0cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x0cc ));
	printf("20696: 0x0cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x0cd ));
	printf("20696: 0x0ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x0ce ));
	printf("20696: 0x0cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x0cf ));
	printf("20696: 0x0d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x0d1 ));
	printf("20696: 0x0d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x0d2 ));
	printf("20696: 0x0d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x0d3 ));
	printf("20696: 0x0d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x0d4 ));
	printf("20696: 0x0d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x0d5 ));
	printf("20696: 0x0d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x0d6 ));
	printf("20696: 0x100 0x%04X\n", phy_utils_read_radioreg(pi, 0x100 ));
	printf("20696: 0x1c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c1 ));
	printf("20696: 0x1c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c2 ));
	printf("20696: 0x1c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c3 ));
	printf("20696: 0x1c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c4 ));
	printf("20696: 0x1c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c5 ));
	printf("20696: 0x1c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c6 ));
	printf("20696: 0x1c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c7 ));
	printf("20696: 0x1c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c8 ));
	printf("20696: 0x1c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x1c9 ));
	printf("20696: 0x1cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x1cb ));
	printf("20696: 0x1cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x1cc ));
	printf("20696: 0x1cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x1cd ));
	printf("20696: 0x1ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x1ce ));
	printf("20696: 0x1cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x1cf ));
	printf("20696: 0x1d0 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d0 ));
	printf("20696: 0x1d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d1 ));
	printf("20696: 0x1d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d2 ));
	printf("20696: 0x1d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d3 ));
	printf("20696: 0x1d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d4 ));
	printf("20696: 0x1d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d5 ));
	printf("20696: 0x1d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d6 ));
	printf("20696: 0x1d7 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d7 ));
	printf("20696: 0x1d8 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d8 ));
	printf("20696: 0x1d9 0x%04X\n", phy_utils_read_radioreg(pi, 0x1d9 ));
	printf("20696: 0x1da 0x%04X\n", phy_utils_read_radioreg(pi, 0x1da ));
	printf("20696: 0x1db 0x%04X\n", phy_utils_read_radioreg(pi, 0x1db ));
	printf("20696: 0x1dc 0x%04X\n", phy_utils_read_radioreg(pi, 0x1dc ));
	printf("20696: 0x1dd 0x%04X\n", phy_utils_read_radioreg(pi, 0x1dd ));
	printf("20696: 0x1de 0x%04X\n", phy_utils_read_radioreg(pi, 0x1de ));
	printf("20696: 0x1df 0x%04X\n", phy_utils_read_radioreg(pi, 0x1df ));
	printf("20696: 0x1e0 0x%04X\n", phy_utils_read_radioreg(pi, 0x1e0 ));
	printf("20696: 0x1e1 0x%04X\n", phy_utils_read_radioreg(pi, 0x1e1 ));
	printf("20696: 0x1e2 0x%04X\n", phy_utils_read_radioreg(pi, 0x1e2 ));
	printf("20696: 0x1e3 0x%04X\n", phy_utils_read_radioreg(pi, 0x1e3 ));
	printf("20696: 0x1e4 0x%04X\n", phy_utils_read_radioreg(pi, 0x1e4 ));
	printf("20696: 0x1ea 0x%04X\n", phy_utils_read_radioreg(pi, 0x1ea ));
	printf("20696: 0x1eb 0x%04X\n", phy_utils_read_radioreg(pi, 0x1eb ));
	printf("20696: 0x1ec 0x%04X\n", phy_utils_read_radioreg(pi, 0x1ec ));
	printf("20696: 0x1ed 0x%04X\n", phy_utils_read_radioreg(pi, 0x1ed ));
	printf("20696: 0x1ee 0x%04X\n", phy_utils_read_radioreg(pi, 0x1ee ));
	printf("20696: 0x1f1 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f1 ));
	printf("20696: 0x1f2 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f2 ));
	printf("20696: 0x1f3 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f3 ));
	printf("20696: 0x1f4 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f4 ));
	printf("20696: 0x1f5 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f5 ));
	printf("20696: 0x1f6 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f6 ));
	printf("20696: 0x1f7 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f7 ));
	printf("20696: 0x1f8 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f8 ));
	printf("20696: 0x1f9 0x%04X\n", phy_utils_read_radioreg(pi, 0x1f9 ));
	printf("20696: 0x1fa 0x%04X\n", phy_utils_read_radioreg(pi, 0x1fa ));
	printf("20696: 0x1fb 0x%04X\n", phy_utils_read_radioreg(pi, 0x1fb ));
	printf("20696: 0x1fc 0x%04X\n", phy_utils_read_radioreg(pi, 0x1fc ));
	printf("20696: 0x1fd 0x%04X\n", phy_utils_read_radioreg(pi, 0x1fd ));
	printf("20696: 0x1ff 0x%04X\n", phy_utils_read_radioreg(pi, 0x1ff ));
	printf("20696: 0x000 0x%04X\n", phy_utils_read_radioreg(pi, 0x200 ));
	// printf("20696: 0x001 0x%04X\n", phy_utils_read_radioreg(pi, 0x201 ));
	printf("20696: 0x001 0x058e\n");
	printf("20696: 0x002 0x%04X\n", phy_utils_read_radioreg(pi, 0x202 ));
	printf("20696: 0x003 0x%04X\n", phy_utils_read_radioreg(pi, 0x203 ));
	printf("20696: 0x004 0x%04X\n", phy_utils_read_radioreg(pi, 0x204 ));
	printf("20696: 0x005 0x%04X\n", phy_utils_read_radioreg(pi, 0x205 ));
	printf("20696: 0x006 0x%04X\n", phy_utils_read_radioreg(pi, 0x206 ));
	printf("20696: 0x007 0x%04X\n", phy_utils_read_radioreg(pi, 0x207 ));
	printf("20696: 0x008 0x%04X\n", phy_utils_read_radioreg(pi, 0x208 ));
	printf("20696: 0x009 0x%04X\n", phy_utils_read_radioreg(pi, 0x209 ));
	printf("20696: 0x00a 0x%04X\n", phy_utils_read_radioreg(pi, 0x20a ));
	printf("20696: 0x00b 0x%04X\n", phy_utils_read_radioreg(pi, 0x20b ));
	printf("20696: 0x00c 0x%04X\n", phy_utils_read_radioreg(pi, 0x20c ));
	printf("20696: 0x017 0x%04X\n", phy_utils_read_radioreg(pi, 0x217 ));
	printf("20696: 0x018 0x%04X\n", phy_utils_read_radioreg(pi, 0x218 ));
	printf("20696: 0x028 0x%04X\n", phy_utils_read_radioreg(pi, 0x228 ));
	printf("20696: 0x029 0x%04X\n", phy_utils_read_radioreg(pi, 0x229 ));
	printf("20696: 0x02a 0x%04X\n", phy_utils_read_radioreg(pi, 0x22a ));
	printf("20696: 0x03d 0x%04X\n", phy_utils_read_radioreg(pi, 0x23d ));
	printf("20696: 0x03e 0x%04X\n", phy_utils_read_radioreg(pi, 0x23e ));
	printf("20696: 0x03f 0x%04X\n", phy_utils_read_radioreg(pi, 0x23f ));
	printf("20696: 0x040 0x%04X\n", phy_utils_read_radioreg(pi, 0x240 ));
	printf("20696: 0x041 0x%04X\n", phy_utils_read_radioreg(pi, 0x241 ));
	printf("20696: 0x042 0x%04X\n", phy_utils_read_radioreg(pi, 0x242 ));
	printf("20696: 0x043 0x%04X\n", phy_utils_read_radioreg(pi, 0x243 ));
	printf("20696: 0x044 0x%04X\n", phy_utils_read_radioreg(pi, 0x244 ));
	printf("20696: 0x045 0x%04X\n", phy_utils_read_radioreg(pi, 0x245 ));
	printf("20696: 0x046 0x%04X\n", phy_utils_read_radioreg(pi, 0x246 ));
	printf("20696: 0x047 0x%04X\n", phy_utils_read_radioreg(pi, 0x247 ));
	printf("20696: 0x048 0x%04X\n", phy_utils_read_radioreg(pi, 0x248 ));
	printf("20696: 0x049 0x%04X\n", phy_utils_read_radioreg(pi, 0x249 ));
	printf("20696: 0x04a 0x%04X\n", phy_utils_read_radioreg(pi, 0x24a ));
	printf("20696: 0x04b 0x%04X\n", phy_utils_read_radioreg(pi, 0x24b ));
	printf("20696: 0x04c 0x%04X\n", phy_utils_read_radioreg(pi, 0x24c ));
	printf("20696: 0x04d 0x%04X\n", phy_utils_read_radioreg(pi, 0x24d ));
	printf("20696: 0x04e 0x%04X\n", phy_utils_read_radioreg(pi, 0x24e ));
	printf("20696: 0x04f 0x%04X\n", phy_utils_read_radioreg(pi, 0x24f ));
	printf("20696: 0x050 0x%04X\n", phy_utils_read_radioreg(pi, 0x250 ));
	printf("20696: 0x051 0x%04X\n", phy_utils_read_radioreg(pi, 0x251 ));
	printf("20696: 0x052 0x%04X\n", phy_utils_read_radioreg(pi, 0x252 ));
	printf("20696: 0x053 0x%04X\n", phy_utils_read_radioreg(pi, 0x253 ));
	printf("20696: 0x054 0x%04X\n", phy_utils_read_radioreg(pi, 0x254 ));
	printf("20696: 0x055 0x%04X\n", phy_utils_read_radioreg(pi, 0x255 ));
	printf("20696: 0x056 0x%04X\n", phy_utils_read_radioreg(pi, 0x256 ));
	printf("20696: 0x057 0x%04X\n", phy_utils_read_radioreg(pi, 0x257 ));
	printf("20696: 0x058 0x%04X\n", phy_utils_read_radioreg(pi, 0x258 ));
	printf("20696: 0x059 0x%04X\n", phy_utils_read_radioreg(pi, 0x259 ));
	printf("20696: 0x05a 0x%04X\n", phy_utils_read_radioreg(pi, 0x25a ));
	printf("20696: 0x05b 0x%04X\n", phy_utils_read_radioreg(pi, 0x25b ));
	printf("20696: 0x05c 0x%04X\n", phy_utils_read_radioreg(pi, 0x25c ));
	printf("20696: 0x05d 0x%04X\n", phy_utils_read_radioreg(pi, 0x25d ));
	printf("20696: 0x05e 0x%04X\n", phy_utils_read_radioreg(pi, 0x25e ));
	printf("20696: 0x05f 0x%04X\n", phy_utils_read_radioreg(pi, 0x25f ));
	printf("20696: 0x060 0x%04X\n", phy_utils_read_radioreg(pi, 0x260 ));
	printf("20696: 0x061 0x%04X\n", phy_utils_read_radioreg(pi, 0x261 ));
	printf("20696: 0x062 0x%04X\n", phy_utils_read_radioreg(pi, 0x262 ));
	printf("20696: 0x063 0x%04X\n", phy_utils_read_radioreg(pi, 0x263 ));
	printf("20696: 0x064 0x%04X\n", phy_utils_read_radioreg(pi, 0x264 ));
	printf("20696: 0x065 0x%04X\n", phy_utils_read_radioreg(pi, 0x265 ));
	printf("20696: 0x066 0x%04X\n", phy_utils_read_radioreg(pi, 0x266 ));
	printf("20696: 0x067 0x%04X\n", phy_utils_read_radioreg(pi, 0x267 ));
	printf("20696: 0x068 0x%04X\n", phy_utils_read_radioreg(pi, 0x268 ));
	printf("20696: 0x069 0x%04X\n", phy_utils_read_radioreg(pi, 0x269 ));
	printf("20696: 0x06a 0x%04X\n", phy_utils_read_radioreg(pi, 0x26a ));
	printf("20696: 0x06b 0x%04X\n", phy_utils_read_radioreg(pi, 0x26b ));
	printf("20696: 0x06c 0x%04X\n", phy_utils_read_radioreg(pi, 0x26c ));
	printf("20696: 0x06e 0x%04X\n", phy_utils_read_radioreg(pi, 0x26e ));
	printf("20696: 0x06f 0x%04X\n", phy_utils_read_radioreg(pi, 0x26f ));
	printf("20696: 0x070 0x%04X\n", phy_utils_read_radioreg(pi, 0x270 ));
	printf("20696: 0x071 0x%04X\n", phy_utils_read_radioreg(pi, 0x271 ));
	printf("20696: 0x072 0x%04X\n", phy_utils_read_radioreg(pi, 0x272 ));
	printf("20696: 0x073 0x%04X\n", phy_utils_read_radioreg(pi, 0x273 ));
	printf("20696: 0x074 0x%04X\n", phy_utils_read_radioreg(pi, 0x274 ));
	printf("20696: 0x075 0x%04X\n", phy_utils_read_radioreg(pi, 0x275 ));
	printf("20696: 0x076 0x%04X\n", phy_utils_read_radioreg(pi, 0x276 ));
	printf("20696: 0x078 0x%04X\n", phy_utils_read_radioreg(pi, 0x278 ));
	printf("20696: 0x079 0x%04X\n", phy_utils_read_radioreg(pi, 0x279 ));
	printf("20696: 0x07a 0x%04X\n", phy_utils_read_radioreg(pi, 0x27a ));
	printf("20696: 0x07b 0x%04X\n", phy_utils_read_radioreg(pi, 0x27b ));
	printf("20696: 0x07c 0x%04X\n", phy_utils_read_radioreg(pi, 0x27c ));
	printf("20696: 0x07d 0x%04X\n", phy_utils_read_radioreg(pi, 0x27d ));
	printf("20696: 0x07e 0x%04X\n", phy_utils_read_radioreg(pi, 0x27e ));
	printf("20696: 0x07f 0x%04X\n", phy_utils_read_radioreg(pi, 0x27f ));
	printf("20696: 0x080 0x%04X\n", phy_utils_read_radioreg(pi, 0x280 ));
	printf("20696: 0x081 0x%04X\n", phy_utils_read_radioreg(pi, 0x281 ));
	printf("20696: 0x082 0x%04X\n", phy_utils_read_radioreg(pi, 0x282 ));
	printf("20696: 0x083 0x%04X\n", phy_utils_read_radioreg(pi, 0x283 ));
	printf("20696: 0x084 0x%04X\n", phy_utils_read_radioreg(pi, 0x284 ));
	printf("20696: 0x085 0x%04X\n", phy_utils_read_radioreg(pi, 0x285 ));
	printf("20696: 0x086 0x%04X\n", phy_utils_read_radioreg(pi, 0x286 ));
	printf("20696: 0x087 0x%04X\n", phy_utils_read_radioreg(pi, 0x287 ));
	printf("20696: 0x088 0x%04X\n", phy_utils_read_radioreg(pi, 0x288 ));
	printf("20696: 0x089 0x%04X\n", phy_utils_read_radioreg(pi, 0x289 ));
	printf("20696: 0x08a 0x%04X\n", phy_utils_read_radioreg(pi, 0x28a ));
	printf("20696: 0x08b 0x%04X\n", phy_utils_read_radioreg(pi, 0x28b ));
	printf("20696: 0x08c 0x%04X\n", phy_utils_read_radioreg(pi, 0x28c ));
	printf("20696: 0x08d 0x%04X\n", phy_utils_read_radioreg(pi, 0x28d ));
	printf("20696: 0x08e 0x%04X\n", phy_utils_read_radioreg(pi, 0x28e ));
	printf("20696: 0x08f 0x%04X\n", phy_utils_read_radioreg(pi, 0x28f ));
	printf("20696: 0x090 0x%04X\n", phy_utils_read_radioreg(pi, 0x290 ));
	printf("20696: 0x091 0x%04X\n", phy_utils_read_radioreg(pi, 0x291 ));
	printf("20696: 0x092 0x%04X\n", phy_utils_read_radioreg(pi, 0x292 ));
	printf("20696: 0x093 0x%04X\n", phy_utils_read_radioreg(pi, 0x293 ));
	printf("20696: 0x094 0x%04X\n", phy_utils_read_radioreg(pi, 0x294 ));
	printf("20696: 0x095 0x%04X\n", phy_utils_read_radioreg(pi, 0x295 ));
	printf("20696: 0x097 0x%04X\n", phy_utils_read_radioreg(pi, 0x297 ));
	printf("20696: 0x098 0x%04X\n", phy_utils_read_radioreg(pi, 0x298 ));
	printf("20696: 0x099 0x%04X\n", phy_utils_read_radioreg(pi, 0x299 ));
	printf("20696: 0x09a 0x%04X\n", phy_utils_read_radioreg(pi, 0x29a ));
	printf("20696: 0x09f 0x%04X\n", phy_utils_read_radioreg(pi, 0x29f ));
	printf("20696: 0x0a0 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a0 ));
	printf("20696: 0x0a1 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a1 ));
	printf("20696: 0x0a2 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a2 ));
	printf("20696: 0x0a3 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a3 ));
	printf("20696: 0x0a4 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a4 ));
	printf("20696: 0x0a5 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a5 ));
	printf("20696: 0x0a7 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a7 ));
	printf("20696: 0x0a8 0x%04X\n", phy_utils_read_radioreg(pi, 0x2a8 ));
	printf("20696: 0x0aa 0x%04X\n", phy_utils_read_radioreg(pi, 0x2aa ));
	printf("20696: 0x0ab 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ab ));
	printf("20696: 0x0ad 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ad ));
	printf("20696: 0x0ae 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ae ));
	printf("20696: 0x0af 0x%04X\n", phy_utils_read_radioreg(pi, 0x2af ));
	printf("20696: 0x0b0 0x%04X\n", phy_utils_read_radioreg(pi, 0x2b0 ));
	printf("20696: 0x0b1 0x%04X\n", phy_utils_read_radioreg(pi, 0x2b1 ));
	printf("20696: 0x0b2 0x%04X\n", phy_utils_read_radioreg(pi, 0x2b2 ));
	printf("20696: 0x0c0 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c0 ));
	printf("20696: 0x0c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c1 ));
	printf("20696: 0x0c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c2 ));
	printf("20696: 0x0c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c3 ));
	printf("20696: 0x0c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c4 ));
	printf("20696: 0x0c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c5 ));
	printf("20696: 0x0c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c6 ));
	printf("20696: 0x0c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c7 ));
	printf("20696: 0x0c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c8 ));
	printf("20696: 0x0c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x2c9 ));
	printf("20696: 0x0ca 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ca ));
	printf("20696: 0x0cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x2cb ));
	printf("20696: 0x0cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x2cc ));
	printf("20696: 0x0cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x2cd ));
	printf("20696: 0x0ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ce ));
	printf("20696: 0x0cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x2cf ));
	printf("20696: 0x0d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x2d1 ));
	printf("20696: 0x0d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x2d2 ));
	printf("20696: 0x0d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x2d3 ));
	printf("20696: 0x0d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x2d4 ));
	printf("20696: 0x0d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x2d5 ));
	printf("20696: 0x0d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x2d6 ));
	printf("20696: 0x0ea 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ea ));
	printf("20696: 0x0eb 0x%04X\n", phy_utils_read_radioreg(pi, 0x2eb ));
	printf("20696: 0x0ec 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ec ));
	printf("20696: 0x0ed 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ed ));
	printf("20696: 0x0ee 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ee ));
	printf("20696: 0x0ef 0x%04X\n", phy_utils_read_radioreg(pi, 0x2ef ));
	printf("20696: 0x0f0 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f0 ));
	printf("20696: 0x0f1 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f1 ));
	printf("20696: 0x0f2 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f2 ));
	printf("20696: 0x0f3 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f3 ));
	printf("20696: 0x0f4 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f4 ));
	printf("20696: 0x0f5 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f5 ));
	printf("20696: 0x0f6 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f6 ));
	printf("20696: 0x0f8 0x%04X\n", phy_utils_read_radioreg(pi, 0x2f8 ));
	printf("20696: 0x100 0x%04X\n", phy_utils_read_radioreg(pi, 0x300 ));
	printf("20696: 0x1c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c1 ));
	printf("20696: 0x1c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c2 ));
	printf("20696: 0x1c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c3 ));
	printf("20696: 0x1c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c4 ));
	printf("20696: 0x1c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c5 ));
	printf("20696: 0x1c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c6 ));
	printf("20696: 0x1c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c7 ));
	printf("20696: 0x1c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c8 ));
	printf("20696: 0x1c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x3c9 ));
	printf("20696: 0x1ca 0x%04X\n", phy_utils_read_radioreg(pi, 0x3ca ));
	printf("20696: 0x1cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x3cb ));
	printf("20696: 0x1cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x3cc ));
	printf("20696: 0x1cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x3cd ));
	printf("20696: 0x1ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x3ce ));
	printf("20696: 0x1cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x3cf ));
	printf("20696: 0x1d0 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d0 ));
	printf("20696: 0x1d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d1 ));
	printf("20696: 0x1d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d2 ));
	printf("20696: 0x1d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d3 ));
	printf("20696: 0x1d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d4 ));
	printf("20696: 0x1d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d5 ));
	printf("20696: 0x1d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d6 ));
	printf("20696: 0x1d7 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d7 ));
	printf("20696: 0x1d8 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d8 ));
	printf("20696: 0x1d9 0x%04X\n", phy_utils_read_radioreg(pi, 0x3d9 ));
	printf("20696: 0x1da 0x%04X\n", phy_utils_read_radioreg(pi, 0x3da ));
	printf("20696: 0x1db 0x%04X\n", phy_utils_read_radioreg(pi, 0x3db ));
	printf("20696: 0x1dc 0x%04X\n", phy_utils_read_radioreg(pi, 0x3dc ));
	printf("20696: 0x1dd 0x%04X\n", phy_utils_read_radioreg(pi, 0x3dd ));
	printf("20696: 0x1de 0x%04X\n", phy_utils_read_radioreg(pi, 0x3de ));
	printf("20696: 0x1df 0x%04X\n", phy_utils_read_radioreg(pi, 0x3df ));
	printf("20696: 0x1e0 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e0 ));
	printf("20696: 0x1e1 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e1 ));
	printf("20696: 0x1e2 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e2 ));
	printf("20696: 0x1e3 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e3 ));
	printf("20696: 0x1e4 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e4 ));
	printf("20696: 0x1e5 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e5 ));
	printf("20696: 0x1e6 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e6 ));
	printf("20696: 0x1e7 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e7 ));
	printf("20696: 0x1e8 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e8 ));
	printf("20696: 0x1e9 0x%04X\n", phy_utils_read_radioreg(pi, 0x3e9 ));
	printf("20696: 0x1ea 0x%04X\n", phy_utils_read_radioreg(pi, 0x3ea ));
	printf("20696: 0x1eb 0x%04X\n", phy_utils_read_radioreg(pi, 0x3eb ));
	printf("20696: 0x1ec 0x%04X\n", phy_utils_read_radioreg(pi, 0x3ec ));
	printf("20696: 0x1ed 0x%04X\n", phy_utils_read_radioreg(pi, 0x3ed ));
	printf("20696: 0x1ee 0x%04X\n", phy_utils_read_radioreg(pi, 0x3ee ));
	printf("20696: 0x1f1 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f1 ));
	printf("20696: 0x1f2 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f2 ));
	printf("20696: 0x1f3 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f3 ));
	printf("20696: 0x1f4 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f4 ));
	printf("20696: 0x1f5 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f5 ));
	printf("20696: 0x1f6 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f6 ));
	printf("20696: 0x1f7 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f7 ));
	printf("20696: 0x1f8 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f8 ));
	printf("20696: 0x1f9 0x%04X\n", phy_utils_read_radioreg(pi, 0x3f9 ));
	printf("20696: 0x1fa 0x%04X\n", phy_utils_read_radioreg(pi, 0x3fa ));
	printf("20696: 0x1fb 0x%04X\n", phy_utils_read_radioreg(pi, 0x3fb ));
	printf("20696: 0x1fc 0x%04X\n", phy_utils_read_radioreg(pi, 0x3fc ));
	printf("20696: 0x1fd 0x%04X\n", phy_utils_read_radioreg(pi, 0x3fd ));
	printf("20696: 0x1fe 0x%04X\n", phy_utils_read_radioreg(pi, 0x3fe ));
	printf("20696: 0x1ff 0x%04X\n", phy_utils_read_radioreg(pi, 0x3ff ));
	printf("20696: 0x000 0x%04X\n", phy_utils_read_radioreg(pi, 0x400 ));
	// printf("20696: 0x001 0x%04X\n", phy_utils_read_radioreg(pi, 0x401 ));
	printf("20696: 0x001 0x058e\n");
	printf("20696: 0x002 0x%04X\n", phy_utils_read_radioreg(pi, 0x402 ));
	printf("20696: 0x003 0x%04X\n", phy_utils_read_radioreg(pi, 0x403 ));
	printf("20696: 0x004 0x%04X\n", phy_utils_read_radioreg(pi, 0x404 ));
	printf("20696: 0x005 0x%04X\n", phy_utils_read_radioreg(pi, 0x405 ));
	printf("20696: 0x006 0x%04X\n", phy_utils_read_radioreg(pi, 0x406 ));
	printf("20696: 0x007 0x%04X\n", phy_utils_read_radioreg(pi, 0x407 ));
	printf("20696: 0x008 0x%04X\n", phy_utils_read_radioreg(pi, 0x408 ));
	printf("20696: 0x009 0x%04X\n", phy_utils_read_radioreg(pi, 0x409 ));
	printf("20696: 0x00a 0x%04X\n", phy_utils_read_radioreg(pi, 0x40a ));
	printf("20696: 0x00b 0x%04X\n", phy_utils_read_radioreg(pi, 0x40b ));
	printf("20696: 0x00c 0x%04X\n", phy_utils_read_radioreg(pi, 0x40c ));
	printf("20696: 0x017 0x%04X\n", phy_utils_read_radioreg(pi, 0x417 ));
	printf("20696: 0x018 0x%04X\n", phy_utils_read_radioreg(pi, 0x418 ));
	printf("20696: 0x028 0x%04X\n", phy_utils_read_radioreg(pi, 0x428 ));
	printf("20696: 0x029 0x%04X\n", phy_utils_read_radioreg(pi, 0x429 ));
	printf("20696: 0x02a 0x%04X\n", phy_utils_read_radioreg(pi, 0x42a ));
	printf("20696: 0x03d 0x%04X\n", phy_utils_read_radioreg(pi, 0x43d ));
	printf("20696: 0x03e 0x%04X\n", phy_utils_read_radioreg(pi, 0x43e ));
	printf("20696: 0x03f 0x%04X\n", phy_utils_read_radioreg(pi, 0x43f ));
	printf("20696: 0x040 0x%04X\n", phy_utils_read_radioreg(pi, 0x440 ));
	printf("20696: 0x041 0x%04X\n", phy_utils_read_radioreg(pi, 0x441 ));
	printf("20696: 0x042 0x%04X\n", phy_utils_read_radioreg(pi, 0x442 ));
	printf("20696: 0x043 0x%04X\n", phy_utils_read_radioreg(pi, 0x443 ));
	printf("20696: 0x044 0x%04X\n", phy_utils_read_radioreg(pi, 0x444 ));
	printf("20696: 0x045 0x%04X\n", phy_utils_read_radioreg(pi, 0x445 ));
	printf("20696: 0x046 0x%04X\n", phy_utils_read_radioreg(pi, 0x446 ));
	printf("20696: 0x047 0x%04X\n", phy_utils_read_radioreg(pi, 0x447 ));
	printf("20696: 0x048 0x%04X\n", phy_utils_read_radioreg(pi, 0x448 ));
	printf("20696: 0x049 0x%04X\n", phy_utils_read_radioreg(pi, 0x449 ));
	printf("20696: 0x04a 0x%04X\n", phy_utils_read_radioreg(pi, 0x44a ));
	printf("20696: 0x04b 0x%04X\n", phy_utils_read_radioreg(pi, 0x44b ));
	printf("20696: 0x04c 0x%04X\n", phy_utils_read_radioreg(pi, 0x44c ));
	printf("20696: 0x04d 0x%04X\n", phy_utils_read_radioreg(pi, 0x44d ));
	printf("20696: 0x04e 0x%04X\n", phy_utils_read_radioreg(pi, 0x44e ));
	printf("20696: 0x04f 0x%04X\n", phy_utils_read_radioreg(pi, 0x44f ));
	printf("20696: 0x050 0x%04X\n", phy_utils_read_radioreg(pi, 0x450 ));
	printf("20696: 0x051 0x%04X\n", phy_utils_read_radioreg(pi, 0x451 ));
	printf("20696: 0x052 0x%04X\n", phy_utils_read_radioreg(pi, 0x452 ));
	printf("20696: 0x053 0x%04X\n", phy_utils_read_radioreg(pi, 0x453 ));
	printf("20696: 0x054 0x%04X\n", phy_utils_read_radioreg(pi, 0x454 ));
	printf("20696: 0x055 0x%04X\n", phy_utils_read_radioreg(pi, 0x455 ));
	printf("20696: 0x056 0x%04X\n", phy_utils_read_radioreg(pi, 0x456 ));
	printf("20696: 0x057 0x%04X\n", phy_utils_read_radioreg(pi, 0x457 ));
	printf("20696: 0x058 0x%04X\n", phy_utils_read_radioreg(pi, 0x458 ));
	printf("20696: 0x059 0x%04X\n", phy_utils_read_radioreg(pi, 0x459 ));
	printf("20696: 0x05a 0x%04X\n", phy_utils_read_radioreg(pi, 0x45a ));
	printf("20696: 0x05b 0x%04X\n", phy_utils_read_radioreg(pi, 0x45b ));
	printf("20696: 0x05c 0x%04X\n", phy_utils_read_radioreg(pi, 0x45c ));
	printf("20696: 0x05d 0x%04X\n", phy_utils_read_radioreg(pi, 0x45d ));
	printf("20696: 0x05e 0x%04X\n", phy_utils_read_radioreg(pi, 0x45e ));
	printf("20696: 0x05f 0x%04X\n", phy_utils_read_radioreg(pi, 0x45f ));
	printf("20696: 0x060 0x%04X\n", phy_utils_read_radioreg(pi, 0x460 ));
	printf("20696: 0x061 0x%04X\n", phy_utils_read_radioreg(pi, 0x461 ));
	printf("20696: 0x062 0x%04X\n", phy_utils_read_radioreg(pi, 0x462 ));
	printf("20696: 0x063 0x%04X\n", phy_utils_read_radioreg(pi, 0x463 ));
	printf("20696: 0x064 0x%04X\n", phy_utils_read_radioreg(pi, 0x464 ));
	printf("20696: 0x065 0x%04X\n", phy_utils_read_radioreg(pi, 0x465 ));
	printf("20696: 0x066 0x%04X\n", phy_utils_read_radioreg(pi, 0x466 ));
	printf("20696: 0x067 0x%04X\n", phy_utils_read_radioreg(pi, 0x467 ));
	printf("20696: 0x068 0x%04X\n", phy_utils_read_radioreg(pi, 0x468 ));
	printf("20696: 0x069 0x%04X\n", phy_utils_read_radioreg(pi, 0x469 ));
	printf("20696: 0x06a 0x%04X\n", phy_utils_read_radioreg(pi, 0x46a ));
	printf("20696: 0x06b 0x%04X\n", phy_utils_read_radioreg(pi, 0x46b ));
	printf("20696: 0x06c 0x%04X\n", phy_utils_read_radioreg(pi, 0x46c ));
	printf("20696: 0x06e 0x%04X\n", phy_utils_read_radioreg(pi, 0x46e ));
	printf("20696: 0x06f 0x%04X\n", phy_utils_read_radioreg(pi, 0x46f ));
	printf("20696: 0x070 0x%04X\n", phy_utils_read_radioreg(pi, 0x470 ));
	printf("20696: 0x071 0x%04X\n", phy_utils_read_radioreg(pi, 0x471 ));
	printf("20696: 0x072 0x%04X\n", phy_utils_read_radioreg(pi, 0x472 ));
	printf("20696: 0x073 0x%04X\n", phy_utils_read_radioreg(pi, 0x473 ));
	printf("20696: 0x074 0x%04X\n", phy_utils_read_radioreg(pi, 0x474 ));
	printf("20696: 0x075 0x%04X\n", phy_utils_read_radioreg(pi, 0x475 ));
	printf("20696: 0x076 0x%04X\n", phy_utils_read_radioreg(pi, 0x476 ));
	printf("20696: 0x078 0x%04X\n", phy_utils_read_radioreg(pi, 0x478 ));
	printf("20696: 0x079 0x%04X\n", phy_utils_read_radioreg(pi, 0x479 ));
	printf("20696: 0x07a 0x%04X\n", phy_utils_read_radioreg(pi, 0x47a ));
	printf("20696: 0x07b 0x%04X\n", phy_utils_read_radioreg(pi, 0x47b ));
	printf("20696: 0x07c 0x%04X\n", phy_utils_read_radioreg(pi, 0x47c ));
	printf("20696: 0x07d 0x%04X\n", phy_utils_read_radioreg(pi, 0x47d ));
	printf("20696: 0x07e 0x%04X\n", phy_utils_read_radioreg(pi, 0x47e ));
	printf("20696: 0x07f 0x%04X\n", phy_utils_read_radioreg(pi, 0x47f ));
	printf("20696: 0x080 0x%04X\n", phy_utils_read_radioreg(pi, 0x480 ));
	printf("20696: 0x081 0x%04X\n", phy_utils_read_radioreg(pi, 0x481 ));
	printf("20696: 0x082 0x%04X\n", phy_utils_read_radioreg(pi, 0x482 ));
	printf("20696: 0x083 0x%04X\n", phy_utils_read_radioreg(pi, 0x483 ));
	printf("20696: 0x084 0x%04X\n", phy_utils_read_radioreg(pi, 0x484 ));
	printf("20696: 0x085 0x%04X\n", phy_utils_read_radioreg(pi, 0x485 ));
	printf("20696: 0x086 0x%04X\n", phy_utils_read_radioreg(pi, 0x486 ));
	printf("20696: 0x088 0x%04X\n", phy_utils_read_radioreg(pi, 0x488 ));
	printf("20696: 0x093 0x%04X\n", phy_utils_read_radioreg(pi, 0x493 ));
	printf("20696: 0x094 0x%04X\n", phy_utils_read_radioreg(pi, 0x494 ));
	printf("20696: 0x097 0x%04X\n", phy_utils_read_radioreg(pi, 0x497 ));
	printf("20696: 0x098 0x%04X\n", phy_utils_read_radioreg(pi, 0x498 ));
	printf("20696: 0x099 0x%04X\n", phy_utils_read_radioreg(pi, 0x499 ));
	printf("20696: 0x09a 0x%04X\n", phy_utils_read_radioreg(pi, 0x49a ));
	printf("20696: 0x09f 0x%04X\n", phy_utils_read_radioreg(pi, 0x49f ));
	printf("20696: 0x0a0 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a0 ));
	printf("20696: 0x0a1 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a1 ));
	printf("20696: 0x0a2 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a2 ));
	printf("20696: 0x0a3 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a3 ));
	printf("20696: 0x0a4 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a4 ));
	printf("20696: 0x0a5 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a5 ));
	printf("20696: 0x0a7 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a7 ));
	printf("20696: 0x0a8 0x%04X\n", phy_utils_read_radioreg(pi, 0x4a8 ));
	printf("20696: 0x0aa 0x%04X\n", phy_utils_read_radioreg(pi, 0x4aa ));
	printf("20696: 0x0ab 0x%04X\n", phy_utils_read_radioreg(pi, 0x4ab ));
	printf("20696: 0x0ad 0x%04X\n", phy_utils_read_radioreg(pi, 0x4ad ));
	printf("20696: 0x0ae 0x%04X\n", phy_utils_read_radioreg(pi, 0x4ae ));
	printf("20696: 0x0af 0x%04X\n", phy_utils_read_radioreg(pi, 0x4af ));
	printf("20696: 0x0b0 0x%04X\n", phy_utils_read_radioreg(pi, 0x4b0 ));
	printf("20696: 0x0b1 0x%04X\n", phy_utils_read_radioreg(pi, 0x4b1 ));
	printf("20696: 0x0b2 0x%04X\n", phy_utils_read_radioreg(pi, 0x4b2 ));
	printf("20696: 0x0c0 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c0 ));
	printf("20696: 0x0c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c1 ));
	printf("20696: 0x0c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c2 ));
	printf("20696: 0x0c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c3 ));
	printf("20696: 0x0c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c4 ));
	printf("20696: 0x0c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c5 ));
	printf("20696: 0x0c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c6 ));
	printf("20696: 0x0c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c7 ));
	printf("20696: 0x0c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c8 ));
	printf("20696: 0x0c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x4c9 ));
	printf("20696: 0x0ca 0x%04X\n", phy_utils_read_radioreg(pi, 0x4ca ));
	printf("20696: 0x0cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x4cb ));
	printf("20696: 0x0cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x4cc ));
	printf("20696: 0x0cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x4cd ));
	printf("20696: 0x0ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x4ce ));
	printf("20696: 0x0cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x4cf ));
	printf("20696: 0x0d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x4d1 ));
	printf("20696: 0x0d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x4d2 ));
	printf("20696: 0x0d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x4d3 ));
	printf("20696: 0x0d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x4d4 ));
	printf("20696: 0x0d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x4d5 ));
	printf("20696: 0x0d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x4d6 ));
	printf("20696: 0x100 0x%04X\n", phy_utils_read_radioreg(pi, 0x500 ));
	printf("20696: 0x1c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c1 ));
	printf("20696: 0x1c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c2 ));
	printf("20696: 0x1c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c3 ));
	printf("20696: 0x1c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c4 ));
	printf("20696: 0x1c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c5 ));
	printf("20696: 0x1c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c6 ));
	printf("20696: 0x1c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c7 ));
	printf("20696: 0x1c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c8 ));
	printf("20696: 0x1c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x5c9 ));
	printf("20696: 0x1cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x5cb ));
	printf("20696: 0x1cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x5cc ));
	printf("20696: 0x1cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x5cd ));
	printf("20696: 0x1ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x5ce ));
	printf("20696: 0x1cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x5cf ));
	printf("20696: 0x1d0 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d0 ));
	printf("20696: 0x1d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d1 ));
	printf("20696: 0x1d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d2 ));
	printf("20696: 0x1d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d3 ));
	printf("20696: 0x1d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d4 ));
	printf("20696: 0x1d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d5 ));
	printf("20696: 0x1d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d6 ));
	printf("20696: 0x1d7 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d7 ));
	printf("20696: 0x1d8 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d8 ));
	printf("20696: 0x1d9 0x%04X\n", phy_utils_read_radioreg(pi, 0x5d9 ));
	printf("20696: 0x1da 0x%04X\n", phy_utils_read_radioreg(pi, 0x5da ));
	printf("20696: 0x1db 0x%04X\n", phy_utils_read_radioreg(pi, 0x5db ));
	printf("20696: 0x1dc 0x%04X\n", phy_utils_read_radioreg(pi, 0x5dc ));
	printf("20696: 0x1dd 0x%04X\n", phy_utils_read_radioreg(pi, 0x5dd ));
	printf("20696: 0x1de 0x%04X\n", phy_utils_read_radioreg(pi, 0x5de ));
	printf("20696: 0x1df 0x%04X\n", phy_utils_read_radioreg(pi, 0x5df ));
	printf("20696: 0x1e0 0x%04X\n", phy_utils_read_radioreg(pi, 0x5e0 ));
	printf("20696: 0x1e1 0x%04X\n", phy_utils_read_radioreg(pi, 0x5e1 ));
	printf("20696: 0x1e2 0x%04X\n", phy_utils_read_radioreg(pi, 0x5e2 ));
	printf("20696: 0x1e3 0x%04X\n", phy_utils_read_radioreg(pi, 0x5e3 ));
	printf("20696: 0x1e4 0x%04X\n", phy_utils_read_radioreg(pi, 0x5e4 ));
	printf("20696: 0x1ea 0x%04X\n", phy_utils_read_radioreg(pi, 0x5ea ));
	printf("20696: 0x1eb 0x%04X\n", phy_utils_read_radioreg(pi, 0x5eb ));
	printf("20696: 0x1ec 0x%04X\n", phy_utils_read_radioreg(pi, 0x5ec ));
	printf("20696: 0x1ed 0x%04X\n", phy_utils_read_radioreg(pi, 0x5ed ));
	printf("20696: 0x1ee 0x%04X\n", phy_utils_read_radioreg(pi, 0x5ee ));
	printf("20696: 0x1ef 0x%04X\n", phy_utils_read_radioreg(pi, 0x5ef ));
	printf("20696: 0x1f1 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f1 ));
	printf("20696: 0x1f2 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f2 ));
	printf("20696: 0x1f3 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f3 ));
	printf("20696: 0x1f4 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f4 ));
	printf("20696: 0x1f5 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f5 ));
	printf("20696: 0x1f6 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f6 ));
	printf("20696: 0x1f7 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f7 ));
	printf("20696: 0x1f8 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f8 ));
	printf("20696: 0x1f9 0x%04X\n", phy_utils_read_radioreg(pi, 0x5f9 ));
	printf("20696: 0x1fa 0x%04X\n", phy_utils_read_radioreg(pi, 0x5fa ));
	printf("20696: 0x1fb 0x%04X\n", phy_utils_read_radioreg(pi, 0x5fb ));
	printf("20696: 0x1fc 0x%04X\n", phy_utils_read_radioreg(pi, 0x5fc ));
	printf("20696: 0x1fd 0x%04X\n", phy_utils_read_radioreg(pi, 0x5fd ));
	printf("20696: 0x1ff 0x%04X\n", phy_utils_read_radioreg(pi, 0x5ff ));
	printf("20696: 0x000 0x%04X\n", phy_utils_read_radioreg(pi, 0x600 ));
	// printf("20696: 0x001 0x%04X\n", phy_utils_read_radioreg(pi, 0x601 ));
	printf("20696: 0x001 0x058e\n");
	printf("20696: 0x002 0x%04X\n", phy_utils_read_radioreg(pi, 0x602 ));
	printf("20696: 0x003 0x%04X\n", phy_utils_read_radioreg(pi, 0x603 ));
	printf("20696: 0x004 0x%04X\n", phy_utils_read_radioreg(pi, 0x604 ));
	printf("20696: 0x005 0x%04X\n", phy_utils_read_radioreg(pi, 0x605 ));
	printf("20696: 0x006 0x%04X\n", phy_utils_read_radioreg(pi, 0x606 ));
	printf("20696: 0x007 0x%04X\n", phy_utils_read_radioreg(pi, 0x607 ));
	printf("20696: 0x008 0x%04X\n", phy_utils_read_radioreg(pi, 0x608 ));
	printf("20696: 0x009 0x%04X\n", phy_utils_read_radioreg(pi, 0x609 ));
	printf("20696: 0x00a 0x%04X\n", phy_utils_read_radioreg(pi, 0x60a ));
	printf("20696: 0x00b 0x%04X\n", phy_utils_read_radioreg(pi, 0x60b ));
	printf("20696: 0x00c 0x%04X\n", phy_utils_read_radioreg(pi, 0x60c ));
	printf("20696: 0x017 0x%04X\n", phy_utils_read_radioreg(pi, 0x617 ));
	printf("20696: 0x018 0x%04X\n", phy_utils_read_radioreg(pi, 0x618 ));
	printf("20696: 0x028 0x%04X\n", phy_utils_read_radioreg(pi, 0x628 ));
	printf("20696: 0x029 0x%04X\n", phy_utils_read_radioreg(pi, 0x629 ));
	printf("20696: 0x02a 0x%04X\n", phy_utils_read_radioreg(pi, 0x62a ));
	printf("20696: 0x03d 0x%04X\n", phy_utils_read_radioreg(pi, 0x63d ));
	printf("20696: 0x03e 0x%04X\n", phy_utils_read_radioreg(pi, 0x63e ));
	printf("20696: 0x03f 0x%04X\n", phy_utils_read_radioreg(pi, 0x63f ));
	printf("20696: 0x040 0x%04X\n", phy_utils_read_radioreg(pi, 0x640 ));
	printf("20696: 0x041 0x%04X\n", phy_utils_read_radioreg(pi, 0x641 ));
	printf("20696: 0x042 0x%04X\n", phy_utils_read_radioreg(pi, 0x642 ));
	printf("20696: 0x043 0x%04X\n", phy_utils_read_radioreg(pi, 0x643 ));
	printf("20696: 0x044 0x%04X\n", phy_utils_read_radioreg(pi, 0x644 ));
	printf("20696: 0x045 0x%04X\n", phy_utils_read_radioreg(pi, 0x645 ));
	printf("20696: 0x046 0x%04X\n", phy_utils_read_radioreg(pi, 0x646 ));
	printf("20696: 0x047 0x%04X\n", phy_utils_read_radioreg(pi, 0x647 ));
	printf("20696: 0x048 0x%04X\n", phy_utils_read_radioreg(pi, 0x648 ));
	printf("20696: 0x049 0x%04X\n", phy_utils_read_radioreg(pi, 0x649 ));
	printf("20696: 0x04a 0x%04X\n", phy_utils_read_radioreg(pi, 0x64a ));
	printf("20696: 0x04b 0x%04X\n", phy_utils_read_radioreg(pi, 0x64b ));
	printf("20696: 0x04c 0x%04X\n", phy_utils_read_radioreg(pi, 0x64c ));
	printf("20696: 0x04d 0x%04X\n", phy_utils_read_radioreg(pi, 0x64d ));
	printf("20696: 0x04e 0x%04X\n", phy_utils_read_radioreg(pi, 0x64e ));
	printf("20696: 0x04f 0x%04X\n", phy_utils_read_radioreg(pi, 0x64f ));
	printf("20696: 0x050 0x%04X\n", phy_utils_read_radioreg(pi, 0x650 ));
	printf("20696: 0x051 0x%04X\n", phy_utils_read_radioreg(pi, 0x651 ));
	printf("20696: 0x052 0x%04X\n", phy_utils_read_radioreg(pi, 0x652 ));
	printf("20696: 0x053 0x%04X\n", phy_utils_read_radioreg(pi, 0x653 ));
	printf("20696: 0x054 0x%04X\n", phy_utils_read_radioreg(pi, 0x654 ));
	printf("20696: 0x055 0x%04X\n", phy_utils_read_radioreg(pi, 0x655 ));
	printf("20696: 0x056 0x%04X\n", phy_utils_read_radioreg(pi, 0x656 ));
	printf("20696: 0x057 0x%04X\n", phy_utils_read_radioreg(pi, 0x657 ));
	printf("20696: 0x058 0x%04X\n", phy_utils_read_radioreg(pi, 0x658 ));
	printf("20696: 0x059 0x%04X\n", phy_utils_read_radioreg(pi, 0x659 ));
	printf("20696: 0x05a 0x%04X\n", phy_utils_read_radioreg(pi, 0x65a ));
	printf("20696: 0x05b 0x%04X\n", phy_utils_read_radioreg(pi, 0x65b ));
	printf("20696: 0x05c 0x%04X\n", phy_utils_read_radioreg(pi, 0x65c ));
	printf("20696: 0x05d 0x%04X\n", phy_utils_read_radioreg(pi, 0x65d ));
	printf("20696: 0x05e 0x%04X\n", phy_utils_read_radioreg(pi, 0x65e ));
	printf("20696: 0x05f 0x%04X\n", phy_utils_read_radioreg(pi, 0x65f ));
	printf("20696: 0x060 0x%04X\n", phy_utils_read_radioreg(pi, 0x660 ));
	printf("20696: 0x061 0x%04X\n", phy_utils_read_radioreg(pi, 0x661 ));
	printf("20696: 0x062 0x%04X\n", phy_utils_read_radioreg(pi, 0x662 ));
	printf("20696: 0x063 0x%04X\n", phy_utils_read_radioreg(pi, 0x663 ));
	printf("20696: 0x064 0x%04X\n", phy_utils_read_radioreg(pi, 0x664 ));
	printf("20696: 0x065 0x%04X\n", phy_utils_read_radioreg(pi, 0x665 ));
	printf("20696: 0x066 0x%04X\n", phy_utils_read_radioreg(pi, 0x666 ));
	printf("20696: 0x067 0x%04X\n", phy_utils_read_radioreg(pi, 0x667 ));
	printf("20696: 0x068 0x%04X\n", phy_utils_read_radioreg(pi, 0x668 ));
	printf("20696: 0x069 0x%04X\n", phy_utils_read_radioreg(pi, 0x669 ));
	printf("20696: 0x06a 0x%04X\n", phy_utils_read_radioreg(pi, 0x66a ));
	printf("20696: 0x06b 0x%04X\n", phy_utils_read_radioreg(pi, 0x66b ));
	printf("20696: 0x06c 0x%04X\n", phy_utils_read_radioreg(pi, 0x66c ));
	printf("20696: 0x06e 0x%04X\n", phy_utils_read_radioreg(pi, 0x66e ));
	printf("20696: 0x06f 0x%04X\n", phy_utils_read_radioreg(pi, 0x66f ));
	printf("20696: 0x070 0x%04X\n", phy_utils_read_radioreg(pi, 0x670 ));
	printf("20696: 0x071 0x%04X\n", phy_utils_read_radioreg(pi, 0x671 ));
	printf("20696: 0x072 0x%04X\n", phy_utils_read_radioreg(pi, 0x672 ));
	printf("20696: 0x073 0x%04X\n", phy_utils_read_radioreg(pi, 0x673 ));
	printf("20696: 0x074 0x%04X\n", phy_utils_read_radioreg(pi, 0x674 ));
	printf("20696: 0x075 0x%04X\n", phy_utils_read_radioreg(pi, 0x675 ));
	printf("20696: 0x076 0x%04X\n", phy_utils_read_radioreg(pi, 0x676 ));
	printf("20696: 0x078 0x%04X\n", phy_utils_read_radioreg(pi, 0x678 ));
	printf("20696: 0x079 0x%04X\n", phy_utils_read_radioreg(pi, 0x679 ));
	printf("20696: 0x07a 0x%04X\n", phy_utils_read_radioreg(pi, 0x67a ));
	printf("20696: 0x07b 0x%04X\n", phy_utils_read_radioreg(pi, 0x67b ));
	printf("20696: 0x07c 0x%04X\n", phy_utils_read_radioreg(pi, 0x67c ));
	printf("20696: 0x07d 0x%04X\n", phy_utils_read_radioreg(pi, 0x67d ));
	printf("20696: 0x07e 0x%04X\n", phy_utils_read_radioreg(pi, 0x67e ));
	printf("20696: 0x07f 0x%04X\n", phy_utils_read_radioreg(pi, 0x67f ));
	printf("20696: 0x080 0x%04X\n", phy_utils_read_radioreg(pi, 0x680 ));
	printf("20696: 0x081 0x%04X\n", phy_utils_read_radioreg(pi, 0x681 ));
	printf("20696: 0x082 0x%04X\n", phy_utils_read_radioreg(pi, 0x682 ));
	printf("20696: 0x083 0x%04X\n", phy_utils_read_radioreg(pi, 0x683 ));
	printf("20696: 0x084 0x%04X\n", phy_utils_read_radioreg(pi, 0x684 ));
	printf("20696: 0x085 0x%04X\n", phy_utils_read_radioreg(pi, 0x685 ));
	printf("20696: 0x086 0x%04X\n", phy_utils_read_radioreg(pi, 0x686 ));
	printf("20696: 0x087 0x%04X\n", phy_utils_read_radioreg(pi, 0x687 ));
	printf("20696: 0x088 0x%04X\n", phy_utils_read_radioreg(pi, 0x688 ));
	printf("20696: 0x089 0x%04X\n", phy_utils_read_radioreg(pi, 0x689 ));
	printf("20696: 0x093 0x%04X\n", phy_utils_read_radioreg(pi, 0x693 ));
	printf("20696: 0x094 0x%04X\n", phy_utils_read_radioreg(pi, 0x694 ));
	printf("20696: 0x097 0x%04X\n", phy_utils_read_radioreg(pi, 0x697 ));
	printf("20696: 0x098 0x%04X\n", phy_utils_read_radioreg(pi, 0x698 ));
	printf("20696: 0x099 0x%04X\n", phy_utils_read_radioreg(pi, 0x699 ));
	printf("20696: 0x09a 0x%04X\n", phy_utils_read_radioreg(pi, 0x69a ));
	printf("20696: 0x09f 0x%04X\n", phy_utils_read_radioreg(pi, 0x69f ));
	printf("20696: 0x0a0 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a0 ));
	printf("20696: 0x0a1 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a1 ));
	printf("20696: 0x0a2 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a2 ));
	printf("20696: 0x0a3 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a3 ));
	printf("20696: 0x0a4 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a4 ));
	printf("20696: 0x0a5 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a5 ));
	printf("20696: 0x0a7 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a7 ));
	printf("20696: 0x0a8 0x%04X\n", phy_utils_read_radioreg(pi, 0x6a8 ));
	printf("20696: 0x0aa 0x%04X\n", phy_utils_read_radioreg(pi, 0x6aa ));
	printf("20696: 0x0ab 0x%04X\n", phy_utils_read_radioreg(pi, 0x6ab ));
	printf("20696: 0x0ad 0x%04X\n", phy_utils_read_radioreg(pi, 0x6ad ));
	printf("20696: 0x0ae 0x%04X\n", phy_utils_read_radioreg(pi, 0x6ae ));
	printf("20696: 0x0af 0x%04X\n", phy_utils_read_radioreg(pi, 0x6af ));
	printf("20696: 0x0b0 0x%04X\n", phy_utils_read_radioreg(pi, 0x6b0 ));
	printf("20696: 0x0b1 0x%04X\n", phy_utils_read_radioreg(pi, 0x6b1 ));
	printf("20696: 0x0b2 0x%04X\n", phy_utils_read_radioreg(pi, 0x6b2 ));
	printf("20696: 0x0c0 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c0 ));
	printf("20696: 0x0c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c1 ));
	printf("20696: 0x0c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c2 ));
	printf("20696: 0x0c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c3 ));
	printf("20696: 0x0c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c4 ));
	printf("20696: 0x0c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c5 ));
	printf("20696: 0x0c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c6 ));
	printf("20696: 0x0c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c7 ));
	printf("20696: 0x0c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c8 ));
	printf("20696: 0x0c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x6c9 ));
	printf("20696: 0x0ca 0x%04X\n", phy_utils_read_radioreg(pi, 0x6ca ));
	printf("20696: 0x0cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x6cb ));
	printf("20696: 0x0cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x6cc ));
	printf("20696: 0x0cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x6cd ));
	printf("20696: 0x0ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x6ce ));
	printf("20696: 0x0cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x6cf ));
	printf("20696: 0x0d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x6d1 ));
	printf("20696: 0x0d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x6d2 ));
	printf("20696: 0x0d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x6d3 ));
	printf("20696: 0x0d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x6d4 ));
	printf("20696: 0x0d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x6d5 ));
	printf("20696: 0x0d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x6d6 ));
	printf("20696: 0x100 0x%04X\n", phy_utils_read_radioreg(pi, 0x700 ));
	printf("20696: 0x1c1 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c1 ));
	printf("20696: 0x1c2 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c2 ));
	printf("20696: 0x1c3 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c3 ));
	printf("20696: 0x1c4 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c4 ));
	printf("20696: 0x1c5 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c5 ));
	printf("20696: 0x1c6 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c6 ));
	printf("20696: 0x1c7 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c7 ));
	printf("20696: 0x1c8 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c8 ));
	printf("20696: 0x1c9 0x%04X\n", phy_utils_read_radioreg(pi, 0x7c9 ));
	printf("20696: 0x1cb 0x%04X\n", phy_utils_read_radioreg(pi, 0x7cb ));
	printf("20696: 0x1cc 0x%04X\n", phy_utils_read_radioreg(pi, 0x7cc ));
	printf("20696: 0x1cd 0x%04X\n", phy_utils_read_radioreg(pi, 0x7cd ));
	printf("20696: 0x1ce 0x%04X\n", phy_utils_read_radioreg(pi, 0x7ce ));
	printf("20696: 0x1cf 0x%04X\n", phy_utils_read_radioreg(pi, 0x7cf ));
	printf("20696: 0x1d0 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d0 ));
	printf("20696: 0x1d1 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d1 ));
	printf("20696: 0x1d2 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d2 ));
	printf("20696: 0x1d3 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d3 ));
	printf("20696: 0x1d4 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d4 ));
	printf("20696: 0x1d5 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d5 ));
	printf("20696: 0x1d6 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d6 ));
	printf("20696: 0x1d7 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d7 ));
	printf("20696: 0x1d8 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d8 ));
	printf("20696: 0x1d9 0x%04X\n", phy_utils_read_radioreg(pi, 0x7d9 ));
	printf("20696: 0x1da 0x%04X\n", phy_utils_read_radioreg(pi, 0x7da ));
	printf("20696: 0x1db 0x%04X\n", phy_utils_read_radioreg(pi, 0x7db ));
	printf("20696: 0x1dc 0x%04X\n", phy_utils_read_radioreg(pi, 0x7dc ));
	printf("20696: 0x1dd 0x%04X\n", phy_utils_read_radioreg(pi, 0x7dd ));
	printf("20696: 0x1de 0x%04X\n", phy_utils_read_radioreg(pi, 0x7de ));
	printf("20696: 0x1df 0x%04X\n", phy_utils_read_radioreg(pi, 0x7df ));
	printf("20696: 0x1e0 0x%04X\n", phy_utils_read_radioreg(pi, 0x7e0 ));
	printf("20696: 0x1e1 0x%04X\n", phy_utils_read_radioreg(pi, 0x7e1 ));
	printf("20696: 0x1e2 0x%04X\n", phy_utils_read_radioreg(pi, 0x7e2 ));
	printf("20696: 0x1e3 0x%04X\n", phy_utils_read_radioreg(pi, 0x7e3 ));
	printf("20696: 0x1e4 0x%04X\n", phy_utils_read_radioreg(pi, 0x7e4 ));
	printf("20696: 0x1ea 0x%04X\n", phy_utils_read_radioreg(pi, 0x7ea ));
	printf("20696: 0x1eb 0x%04X\n", phy_utils_read_radioreg(pi, 0x7eb ));
	printf("20696: 0x1ec 0x%04X\n", phy_utils_read_radioreg(pi, 0x7ec ));
	printf("20696: 0x1ed 0x%04X\n", phy_utils_read_radioreg(pi, 0x7ed ));
	printf("20696: 0x1ee 0x%04X\n", phy_utils_read_radioreg(pi, 0x7ee ));
	printf("20696: 0x1ef 0x%04X\n", phy_utils_read_radioreg(pi, 0x7ef ));
	printf("20696: 0x1f0 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f0 ));
	printf("20696: 0x1f1 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f1 ));
	printf("20696: 0x1f2 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f2 ));
	printf("20696: 0x1f3 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f3 ));
	printf("20696: 0x1f4 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f4 ));
	printf("20696: 0x1f5 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f5 ));
	printf("20696: 0x1f6 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f6 ));
	printf("20696: 0x1f7 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f7 ));
	printf("20696: 0x1f8 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f8 ));
	printf("20696: 0x1f9 0x%04X\n", phy_utils_read_radioreg(pi, 0x7f9 ));
	printf("20696: 0x1fa 0x%04X\n", phy_utils_read_radioreg(pi, 0x7fa ));
	printf("20696: 0x1fb 0x%04X\n", phy_utils_read_radioreg(pi, 0x7fb ));
	printf("20696: 0x1fc 0x%04X\n", phy_utils_read_radioreg(pi, 0x7fc ));
	printf("20696: 0x1fd 0x%04X\n", phy_utils_read_radioreg(pi, 0x7fd ));
	printf("20696: 0x1ff 0x%04X\n", phy_utils_read_radioreg(pi, 0x7ff ));
	printf("20696: 0x000 0x%04X\n", phy_utils_read_radioreg(pi, 0x800 ));
	printf("20696: 0x001 0x%04X\n", phy_utils_read_radioreg(pi, 0x801 ));
	printf("20696: 0x002 0x%04X\n", phy_utils_read_radioreg(pi, 0x802 ));
	printf("20696: 0x003 0x%04X\n", phy_utils_read_radioreg(pi, 0x803 ));
	printf("20696: 0x004 0x%04X\n", phy_utils_read_radioreg(pi, 0x804 ));
	printf("20696: 0x005 0x%04X\n", phy_utils_read_radioreg(pi, 0x805 ));
	printf("20696: 0x006 0x%04X\n", phy_utils_read_radioreg(pi, 0x806 ));
	printf("20696: 0x007 0x%04X\n", phy_utils_read_radioreg(pi, 0x807 ));
	printf("20696: 0x008 0x%04X\n", phy_utils_read_radioreg(pi, 0x808 ));
	printf("20696: 0x009 0x%04X\n", phy_utils_read_radioreg(pi, 0x809 ));
	printf("20696: 0x00a 0x%04X\n", phy_utils_read_radioreg(pi, 0x80a ));
	printf("20696: 0x00b 0x%04X\n", phy_utils_read_radioreg(pi, 0x80b ));
	printf("20696: 0x00c 0x%04X\n", phy_utils_read_radioreg(pi, 0x80c ));
	printf("20696: 0x018 0x%04X\n", phy_utils_read_radioreg(pi, 0x818 ));
	printf("20696: 0x019 0x%04X\n", phy_utils_read_radioreg(pi, 0x819 ));
	printf("20696: 0x01a 0x%04X\n", phy_utils_read_radioreg(pi, 0x81a ));
	printf("20696: 0x01b 0x%04X\n", phy_utils_read_radioreg(pi, 0x81b ));
	printf("20696: 0x01c 0x%04X\n", phy_utils_read_radioreg(pi, 0x81c ));
	printf("20696: 0x01d 0x%04X\n", phy_utils_read_radioreg(pi, 0x81d ));
	printf("20696: 0x01e 0x%04X\n", phy_utils_read_radioreg(pi, 0x81e ));
	printf("20696: 0x01f 0x%04X\n", phy_utils_read_radioreg(pi, 0x81f ));
	printf("20696: 0x020 0x%04X\n", phy_utils_read_radioreg(pi, 0x820 ));
	printf("20696: 0x021 0x%04X\n", phy_utils_read_radioreg(pi, 0x821 ));
	printf("20696: 0x022 0x%04X\n", phy_utils_read_radioreg(pi, 0x822 ));
	printf("20696: 0x023 0x%04X\n", phy_utils_read_radioreg(pi, 0x823 ));
	printf("20696: 0x024 0x%04X\n", phy_utils_read_radioreg(pi, 0x824 ));
	printf("20696: 0x025 0x%04X\n", phy_utils_read_radioreg(pi, 0x825 ));
	printf("20696: 0x026 0x%04X\n", phy_utils_read_radioreg(pi, 0x826 ));
	printf("20696: 0x027 0x%04X\n", phy_utils_read_radioreg(pi, 0x827 ));
	printf("20696: 0x028 0x%04X\n", phy_utils_read_radioreg(pi, 0x828 ));
	printf("20696: 0x029 0x%04X\n", phy_utils_read_radioreg(pi, 0x829 ));
	printf("20696: 0x02a 0x%04X\n", phy_utils_read_radioreg(pi, 0x82a ));
	printf("20696: 0x02b 0x%04X\n", phy_utils_read_radioreg(pi, 0x82b ));
	printf("20696: 0x02c 0x%04X\n", phy_utils_read_radioreg(pi, 0x82c ));
	printf("20696: 0x02e 0x%04X\n", phy_utils_read_radioreg(pi, 0x82e ));
	printf("20696: 0x02f 0x%04X\n", phy_utils_read_radioreg(pi, 0x82f ));
	printf("20696: 0x030 0x%04X\n", phy_utils_read_radioreg(pi, 0x830 ));
	printf("20696: 0x031 0x%04X\n", phy_utils_read_radioreg(pi, 0x831 ));
	printf("20696: 0x032 0x%04X\n", phy_utils_read_radioreg(pi, 0x832 ));
	printf("20696: 0x033 0x%04X\n", phy_utils_read_radioreg(pi, 0x833 ));
	printf("20696: 0x034 0x%04X\n", phy_utils_read_radioreg(pi, 0x834 ));
	printf("20696: 0x035 0x%04X\n", phy_utils_read_radioreg(pi, 0x835 ));
	printf("20696: 0x036 0x%04X\n", phy_utils_read_radioreg(pi, 0x836 ));
	printf("20696: 0x037 0x%04X\n", phy_utils_read_radioreg(pi, 0x837 ));
	printf("20696: 0x038 0x%04X\n", phy_utils_read_radioreg(pi, 0x838 ));
	printf("20696: 0x039 0x%04X\n", phy_utils_read_radioreg(pi, 0x839 ));
	printf("20696: 0x03a 0x%04X\n", phy_utils_read_radioreg(pi, 0x83a ));
	printf("20696: 0x03b 0x%04X\n", phy_utils_read_radioreg(pi, 0x83b ));
	printf("20696: 0x03e 0x%04X\n", phy_utils_read_radioreg(pi, 0x83e ));
	printf("20696: 0x03f 0x%04X\n", phy_utils_read_radioreg(pi, 0x83f ));
	printf("20696: 0x040 0x%04X\n", phy_utils_read_radioreg(pi, 0x840 ));
	printf("20696: 0x041 0x%04X\n", phy_utils_read_radioreg(pi, 0x841 ));
	printf("20696: 0x042 0x%04X\n", phy_utils_read_radioreg(pi, 0x842 ));
	printf("20696: 0x043 0x%04X\n", phy_utils_read_radioreg(pi, 0x843 ));
	printf("20696: 0x044 0x%04X\n", phy_utils_read_radioreg(pi, 0x844 ));
	printf("20696: 0x045 0x%04X\n", phy_utils_read_radioreg(pi, 0x845 ));
	printf("20696: 0x046 0x%04X\n", phy_utils_read_radioreg(pi, 0x846 ));
	printf("20696: 0x047 0x%04X\n", phy_utils_read_radioreg(pi, 0x847 ));
	printf("20696: 0x048 0x%04X\n", phy_utils_read_radioreg(pi, 0x848 ));
	printf("20696: 0x049 0x%04X\n", phy_utils_read_radioreg(pi, 0x849 ));
	printf("20696: 0x04a 0x%04X\n", phy_utils_read_radioreg(pi, 0x84a ));
	printf("20696: 0x04b 0x%04X\n", phy_utils_read_radioreg(pi, 0x84b ));
	printf("20696: 0x04c 0x%04X\n", phy_utils_read_radioreg(pi, 0x84c ));
	printf("20696: 0x04d 0x%04X\n", phy_utils_read_radioreg(pi, 0x84d ));
	printf("20696: 0x04e 0x%04X\n", phy_utils_read_radioreg(pi, 0x84e ));
	printf("20696: 0x04f 0x%04X\n", phy_utils_read_radioreg(pi, 0x84f ));
	printf("20696: 0x050 0x%04X\n", phy_utils_read_radioreg(pi, 0x850 ));
	printf("20696: 0x051 0x%04X\n", phy_utils_read_radioreg(pi, 0x851 ));
	printf("20696: 0x052 0x%04X\n", phy_utils_read_radioreg(pi, 0x852 ));
	printf("20696: 0x053 0x%04X\n", phy_utils_read_radioreg(pi, 0x853 ));
	printf("20696: 0x054 0x%04X\n", phy_utils_read_radioreg(pi, 0x854 ));
	printf("20696: 0x055 0x%04X\n", phy_utils_read_radioreg(pi, 0x855 ));
	printf("20696: 0x056 0x%04X\n", phy_utils_read_radioreg(pi, 0x856 ));
	printf("20696: 0x057 0x%04X\n", phy_utils_read_radioreg(pi, 0x857 ));
	printf("20696: 0x058 0x%04X\n", phy_utils_read_radioreg(pi, 0x858 ));
	printf("20696: 0x059 0x%04X\n", phy_utils_read_radioreg(pi, 0x859 ));
	printf("20696: 0x05a 0x%04X\n", phy_utils_read_radioreg(pi, 0x85a ));
	printf("20696: 0x05b 0x%04X\n", phy_utils_read_radioreg(pi, 0x85b ));
	printf("20696: 0x05c 0x%04X\n", phy_utils_read_radioreg(pi, 0x85c ));
	printf("20696: 0x05d 0x%04X\n", phy_utils_read_radioreg(pi, 0x85d ));
	printf("20696: 0x05e 0x%04X\n", phy_utils_read_radioreg(pi, 0x85e ));
	printf("20696: 0x05f 0x%04X\n", phy_utils_read_radioreg(pi, 0x85f ));
	printf("20696: 0x060 0x%04X\n", phy_utils_read_radioreg(pi, 0x860 ));
	printf("20696: 0x061 0x%04X\n", phy_utils_read_radioreg(pi, 0x861 ));
	printf("20696: 0x062 0x%04X\n", phy_utils_read_radioreg(pi, 0x862 ));
	printf("20696: 0x063 0x%04X\n", phy_utils_read_radioreg(pi, 0x863 ));
	printf("20696: 0x064 0x%04X\n", phy_utils_read_radioreg(pi, 0x864 ));
	printf("20696: 0x065 0x%04X\n", phy_utils_read_radioreg(pi, 0x865 ));
	printf("20696: 0x066 0x%04X\n", phy_utils_read_radioreg(pi, 0x866 ));
	printf("20696: 0x067 0x%04X\n", phy_utils_read_radioreg(pi, 0x867 ));
	printf("20696: 0x068 0x%04X\n", phy_utils_read_radioreg(pi, 0x868 ));
	printf("20696: 0x069 0x%04X\n", phy_utils_read_radioreg(pi, 0x869 ));
	printf("20696: 0x06a 0x%04X\n", phy_utils_read_radioreg(pi, 0x86a ));
	printf("20696: 0x06b 0x%04X\n", phy_utils_read_radioreg(pi, 0x86b ));
	printf("20696: 0x06c 0x%04X\n", phy_utils_read_radioreg(pi, 0x86c ));
	printf("20696: 0x06d 0x%04X\n", phy_utils_read_radioreg(pi, 0x86d ));
	printf("20696: 0x06e 0x%04X\n", phy_utils_read_radioreg(pi, 0x86e ));
	printf("20696: 0x06f 0x%04X\n", phy_utils_read_radioreg(pi, 0x86f ));
	printf("20696: 0x070 0x%04X\n", phy_utils_read_radioreg(pi, 0x870 ));
	printf("20696: 0x071 0x%04X\n", phy_utils_read_radioreg(pi, 0x871 ));
	printf("20696: 0x072 0x%04X\n", phy_utils_read_radioreg(pi, 0x872 ));
	printf("20696: 0x073 0x%04X\n", phy_utils_read_radioreg(pi, 0x873 ));
	printf("20696: 0x074 0x%04X\n", phy_utils_read_radioreg(pi, 0x874 ));
	printf("20696: 0x075 0x%04X\n", phy_utils_read_radioreg(pi, 0x875 ));
	printf("20696: 0x076 0x%04X\n", phy_utils_read_radioreg(pi, 0x876 ));
	printf("20696: 0x077 0x%04X\n", phy_utils_read_radioreg(pi, 0x877 ));
	printf("20696: 0x078 0x%04X\n", phy_utils_read_radioreg(pi, 0x878 ));
	printf("20696: 0x079 0x%04X\n", phy_utils_read_radioreg(pi, 0x879 ));
	printf("20696: 0x1f5 0x%04X\n", phy_utils_read_radioreg(pi, 0x9f5 ));
	printf("20696: 0x1f6 0x%04X\n", phy_utils_read_radioreg(pi, 0x9f6 ));
	printf("20696: 0x1f7 0x%04X\n", phy_utils_read_radioreg(pi, 0x9f7 ));
	printf("20696: 0x1f8 0x%04X\n", phy_utils_read_radioreg(pi, 0x9f8 ));
	printf("20696: 0x1f9 0x%04X\n", phy_utils_read_radioreg(pi, 0x9f9 ));
	printf("20696: 0x1fa 0x%04X\n", phy_utils_read_radioreg(pi, 0x9fa ));
	printf("20696: 0x1fb 0x%04X\n", phy_utils_read_radioreg(pi, 0x9fb ));
	printf("20696: 0x1fc 0x%04X\n", phy_utils_read_radioreg(pi, 0x9fc ));
	printf("20696: 0x1fd 0x%04X\n", phy_utils_read_radioreg(pi, 0x9fd ));
	printf("20696: 0x1fe 0x%04X\n", phy_utils_read_radioreg(pi, 0x9fe ));
	printf("20696: 0x1ff 0x%04X\n", phy_utils_read_radioreg(pi, 0x9ff ));
	printf("20696: 0x000 0x%04X\n", phy_utils_read_radioreg(pi, 0xc00 ));
	// printf("20696: 0x001 0x%04X\n", phy_utils_read_radioreg(pi, 0xc01 ));
	printf("20696: 0x001 0x058e\n");
	printf("20696: 0x002 0x%04X\n", phy_utils_read_radioreg(pi, 0xc02 ));
	printf("20696: 0x003 0x%04X\n", phy_utils_read_radioreg(pi, 0xc03 ));
	printf("20696: 0x004 0x%04X\n", phy_utils_read_radioreg(pi, 0xc04 ));
	printf("20696: 0x005 0x%04X\n", phy_utils_read_radioreg(pi, 0xc05 ));
	printf("20696: 0x006 0x%04X\n", phy_utils_read_radioreg(pi, 0xc06 ));
	printf("20696: 0x007 0x%04X\n", phy_utils_read_radioreg(pi, 0xc07 ));
	printf("20696: 0x008 0x%04X\n", phy_utils_read_radioreg(pi, 0xc08 ));
	printf("20696: 0x009 0x%04X\n", phy_utils_read_radioreg(pi, 0xc09 ));
	printf("20696: 0x00a 0x%04X\n", phy_utils_read_radioreg(pi, 0xc0a ));
	printf("20696: 0x00b 0x%04X\n", phy_utils_read_radioreg(pi, 0xc0b ));
	printf("20696: 0x00c 0x%04X\n", phy_utils_read_radioreg(pi, 0xc0c ));
	printf("20696: 0x017 0x%04X\n", phy_utils_read_radioreg(pi, 0xc17 ));
	printf("20696: 0x018 0x%04X\n", phy_utils_read_radioreg(pi, 0xc18 ));
	printf("20696: 0x028 0x%04X\n", phy_utils_read_radioreg(pi, 0xc28 ));
	printf("20696: 0x029 0x%04X\n", phy_utils_read_radioreg(pi, 0xc29 ));
	printf("20696: 0x02a 0x%04X\n", phy_utils_read_radioreg(pi, 0xc2a ));
	printf("20696: 0x03d 0x%04X\n", phy_utils_read_radioreg(pi, 0xc3d ));
	printf("20696: 0x03e 0x%04X\n", phy_utils_read_radioreg(pi, 0xc3e ));
	printf("20696: 0x03f 0x%04X\n", phy_utils_read_radioreg(pi, 0xc3f ));
	printf("20696: 0x040 0x%04X\n", phy_utils_read_radioreg(pi, 0xc40 ));
	printf("20696: 0x041 0x%04X\n", phy_utils_read_radioreg(pi, 0xc41 ));
	printf("20696: 0x042 0x%04X\n", phy_utils_read_radioreg(pi, 0xc42 ));
	printf("20696: 0x043 0x%04X\n", phy_utils_read_radioreg(pi, 0xc43 ));
	printf("20696: 0x044 0x%04X\n", phy_utils_read_radioreg(pi, 0xc44 ));
	printf("20696: 0x045 0x%04X\n", phy_utils_read_radioreg(pi, 0xc45 ));
	printf("20696: 0x046 0x%04X\n", phy_utils_read_radioreg(pi, 0xc46 ));
	printf("20696: 0x047 0x%04X\n", phy_utils_read_radioreg(pi, 0xc47 ));
	printf("20696: 0x048 0x%04X\n", phy_utils_read_radioreg(pi, 0xc48 ));
	printf("20696: 0x049 0x%04X\n", phy_utils_read_radioreg(pi, 0xc49 ));
	printf("20696: 0x04a 0x%04X\n", phy_utils_read_radioreg(pi, 0xc4a ));
	printf("20696: 0x04b 0x%04X\n", phy_utils_read_radioreg(pi, 0xc4b ));
	printf("20696: 0x04c 0x%04X\n", phy_utils_read_radioreg(pi, 0xc4c ));
	printf("20696: 0x04d 0x%04X\n", phy_utils_read_radioreg(pi, 0xc4d ));
	printf("20696: 0x04e 0x%04X\n", phy_utils_read_radioreg(pi, 0xc4e ));
	printf("20696: 0x04f 0x%04X\n", phy_utils_read_radioreg(pi, 0xc4f ));
	printf("20696: 0x050 0x%04X\n", phy_utils_read_radioreg(pi, 0xc50 ));
	printf("20696: 0x051 0x%04X\n", phy_utils_read_radioreg(pi, 0xc51 ));
	printf("20696: 0x052 0x%04X\n", phy_utils_read_radioreg(pi, 0xc52 ));
	printf("20696: 0x053 0x%04X\n", phy_utils_read_radioreg(pi, 0xc53 ));
	printf("20696: 0x054 0x%04X\n", phy_utils_read_radioreg(pi, 0xc54 ));
	printf("20696: 0x055 0x%04X\n", phy_utils_read_radioreg(pi, 0xc55 ));
	printf("20696: 0x056 0x%04X\n", phy_utils_read_radioreg(pi, 0xc56 ));
	printf("20696: 0x057 0x%04X\n", phy_utils_read_radioreg(pi, 0xc57 ));
	printf("20696: 0x058 0x%04X\n", phy_utils_read_radioreg(pi, 0xc58 ));
	printf("20696: 0x059 0x%04X\n", phy_utils_read_radioreg(pi, 0xc59 ));
	printf("20696: 0x05a 0x%04X\n", phy_utils_read_radioreg(pi, 0xc5a ));
	printf("20696: 0x05b 0x%04X\n", phy_utils_read_radioreg(pi, 0xc5b ));
	printf("20696: 0x05c 0x%04X\n", phy_utils_read_radioreg(pi, 0xc5c ));
	printf("20696: 0x05d 0x%04X\n", phy_utils_read_radioreg(pi, 0xc5d ));
	printf("20696: 0x05e 0x%04X\n", phy_utils_read_radioreg(pi, 0xc5e ));
	printf("20696: 0x05f 0x%04X\n", phy_utils_read_radioreg(pi, 0xc5f ));
	printf("20696: 0x060 0x%04X\n", phy_utils_read_radioreg(pi, 0xc60 ));
	printf("20696: 0x061 0x%04X\n", phy_utils_read_radioreg(pi, 0xc61 ));
	printf("20696: 0x062 0x%04X\n", phy_utils_read_radioreg(pi, 0xc62 ));
	printf("20696: 0x063 0x%04X\n", phy_utils_read_radioreg(pi, 0xc63 ));
	printf("20696: 0x064 0x%04X\n", phy_utils_read_radioreg(pi, 0xc64 ));
	printf("20696: 0x065 0x%04X\n", phy_utils_read_radioreg(pi, 0xc65 ));
	printf("20696: 0x066 0x%04X\n", phy_utils_read_radioreg(pi, 0xc66 ));
	printf("20696: 0x067 0x%04X\n", phy_utils_read_radioreg(pi, 0xc67 ));
	printf("20696: 0x068 0x%04X\n", phy_utils_read_radioreg(pi, 0xc68 ));
	printf("20696: 0x069 0x%04X\n", phy_utils_read_radioreg(pi, 0xc69 ));
	printf("20696: 0x06a 0x%04X\n", phy_utils_read_radioreg(pi, 0xc6a ));
	printf("20696: 0x06b 0x%04X\n", phy_utils_read_radioreg(pi, 0xc6b ));
	printf("20696: 0x06c 0x%04X\n", phy_utils_read_radioreg(pi, 0xc6c ));
	printf("20696: 0x06e 0x%04X\n", phy_utils_read_radioreg(pi, 0xc6e ));
	printf("20696: 0x06f 0x%04X\n", phy_utils_read_radioreg(pi, 0xc6f ));
	printf("20696: 0x070 0x%04X\n", phy_utils_read_radioreg(pi, 0xc70 ));
	printf("20696: 0x071 0x%04X\n", phy_utils_read_radioreg(pi, 0xc71 ));
	printf("20696: 0x072 0x%04X\n", phy_utils_read_radioreg(pi, 0xc72 ));
	printf("20696: 0x073 0x%04X\n", phy_utils_read_radioreg(pi, 0xc73 ));
	printf("20696: 0x074 0x%04X\n", phy_utils_read_radioreg(pi, 0xc74 ));
	printf("20696: 0x075 0x%04X\n", phy_utils_read_radioreg(pi, 0xc75 ));
	printf("20696: 0x076 0x%04X\n", phy_utils_read_radioreg(pi, 0xc76 ));
	printf("20696: 0x078 0x%04X\n", phy_utils_read_radioreg(pi, 0xc78 ));
	printf("20696: 0x079 0x%04X\n", phy_utils_read_radioreg(pi, 0xc79 ));
	printf("20696: 0x07a 0x%04X\n", phy_utils_read_radioreg(pi, 0xc7a ));
	printf("20696: 0x07b 0x%04X\n", phy_utils_read_radioreg(pi, 0xc7b ));
	printf("20696: 0x07c 0x%04X\n", phy_utils_read_radioreg(pi, 0xc7c ));
	printf("20696: 0x07d 0x%04X\n", phy_utils_read_radioreg(pi, 0xc7d ));
	printf("20696: 0x07e 0x%04X\n", phy_utils_read_radioreg(pi, 0xc7e ));
	printf("20696: 0x07f 0x%04X\n", phy_utils_read_radioreg(pi, 0xc7f ));
	printf("20696: 0x080 0x%04X\n", phy_utils_read_radioreg(pi, 0xc80 ));
	printf("20696: 0x081 0x%04X\n", phy_utils_read_radioreg(pi, 0xc81 ));
	printf("20696: 0x082 0x%04X\n", phy_utils_read_radioreg(pi, 0xc82 ));
	printf("20696: 0x083 0x%04X\n", phy_utils_read_radioreg(pi, 0xc83 ));
	printf("20696: 0x084 0x%04X\n", phy_utils_read_radioreg(pi, 0xc84 ));
	printf("20696: 0x085 0x%04X\n", phy_utils_read_radioreg(pi, 0xc85 ));
	printf("20696: 0x086 0x%04X\n", phy_utils_read_radioreg(pi, 0xc86 ));
	printf("20696: 0x088 0x%04X\n", phy_utils_read_radioreg(pi, 0xc88 ));
	printf("20696: 0x093 0x%04X\n", phy_utils_read_radioreg(pi, 0xc93 ));
	printf("20696: 0x094 0x%04X\n", phy_utils_read_radioreg(pi, 0xc94 ));
	printf("20696: 0x097 0x%04X\n", phy_utils_read_radioreg(pi, 0xc97 ));
	printf("20696: 0x098 0x%04X\n", phy_utils_read_radioreg(pi, 0xc98 ));
	printf("20696: 0x099 0x%04X\n", phy_utils_read_radioreg(pi, 0xc99 ));
	printf("20696: 0x09a 0x%04X\n", phy_utils_read_radioreg(pi, 0xc9a ));
	printf("20696: 0x09f 0x%04X\n", phy_utils_read_radioreg(pi, 0xc9f ));
	printf("20696: 0x0a0 0x%04X\n", phy_utils_read_radioreg(pi, 0xca0 ));
	printf("20696: 0x0a1 0x%04X\n", phy_utils_read_radioreg(pi, 0xca1 ));
	printf("20696: 0x0a2 0x%04X\n", phy_utils_read_radioreg(pi, 0xca2 ));
	printf("20696: 0x0a3 0x%04X\n", phy_utils_read_radioreg(pi, 0xca3 ));
	printf("20696: 0x0a4 0x%04X\n", phy_utils_read_radioreg(pi, 0xca4 ));
	printf("20696: 0x0a5 0x%04X\n", phy_utils_read_radioreg(pi, 0xca5 ));
	printf("20696: 0x0a7 0x%04X\n", phy_utils_read_radioreg(pi, 0xca7 ));
	printf("20696: 0x0a8 0x%04X\n", phy_utils_read_radioreg(pi, 0xca8 ));
	printf("20696: 0x0aa 0x%04X\n", phy_utils_read_radioreg(pi, 0xcaa ));
	printf("20696: 0x0ab 0x%04X\n", phy_utils_read_radioreg(pi, 0xcab ));
	printf("20696: 0x0ad 0x%04X\n", phy_utils_read_radioreg(pi, 0xcad ));
	printf("20696: 0x0ae 0x%04X\n", phy_utils_read_radioreg(pi, 0xcae ));
	printf("20696: 0x0af 0x%04X\n", phy_utils_read_radioreg(pi, 0xcaf ));
	printf("20696: 0x0b0 0x%04X\n", phy_utils_read_radioreg(pi, 0xcb0 ));
	printf("20696: 0x0b1 0x%04X\n", phy_utils_read_radioreg(pi, 0xcb1 ));
	printf("20696: 0x0b2 0x%04X\n", phy_utils_read_radioreg(pi, 0xcb2 ));
	printf("20696: 0x0c0 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc0 ));
	printf("20696: 0x0c1 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc1 ));
	printf("20696: 0x0c2 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc2 ));
	printf("20696: 0x0c3 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc3 ));
	printf("20696: 0x0c4 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc4 ));
	printf("20696: 0x0c5 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc5 ));
	printf("20696: 0x0c6 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc6 ));
	printf("20696: 0x0c7 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc7 ));
	printf("20696: 0x0c8 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc8 ));
	printf("20696: 0x0c9 0x%04X\n", phy_utils_read_radioreg(pi, 0xcc9 ));
	printf("20696: 0x0ca 0x%04X\n", phy_utils_read_radioreg(pi, 0xcca ));
	printf("20696: 0x0cb 0x%04X\n", phy_utils_read_radioreg(pi, 0xccb ));
	printf("20696: 0x0cc 0x%04X\n", phy_utils_read_radioreg(pi, 0xccc ));
	printf("20696: 0x0cd 0x%04X\n", phy_utils_read_radioreg(pi, 0xccd ));
	printf("20696: 0x0ce 0x%04X\n", phy_utils_read_radioreg(pi, 0xcce ));
	printf("20696: 0x0cf 0x%04X\n", phy_utils_read_radioreg(pi, 0xccf ));
	printf("20696: 0x0d1 0x%04X\n", phy_utils_read_radioreg(pi, 0xcd1 ));
	printf("20696: 0x0d2 0x%04X\n", phy_utils_read_radioreg(pi, 0xcd2 ));
	printf("20696: 0x0d3 0x%04X\n", phy_utils_read_radioreg(pi, 0xcd3 ));
	printf("20696: 0x0d4 0x%04X\n", phy_utils_read_radioreg(pi, 0xcd4 ));
	printf("20696: 0x0d5 0x%04X\n", phy_utils_read_radioreg(pi, 0xcd5 ));
	printf("20696: 0x0d6 0x%04X\n", phy_utils_read_radioreg(pi, 0xcd6 ));
	printf("20696: 0x100 0x%04X\n", phy_utils_read_radioreg(pi, 0xd00 ));
	printf("20696: 0x1c1 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc1 ));
	printf("20696: 0x1c2 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc2 ));
	printf("20696: 0x1c3 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc3 ));
	printf("20696: 0x1c4 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc4 ));
	printf("20696: 0x1c5 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc5 ));
	printf("20696: 0x1c6 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc6 ));
	printf("20696: 0x1c7 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc7 ));
	printf("20696: 0x1c8 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc8 ));
	printf("20696: 0x1c9 0x%04X\n", phy_utils_read_radioreg(pi, 0xdc9 ));
	printf("20696: 0x1cb 0x%04X\n", phy_utils_read_radioreg(pi, 0xdcb ));
	printf("20696: 0x1cc 0x%04X\n", phy_utils_read_radioreg(pi, 0xdcc ));
	printf("20696: 0x1cd 0x%04X\n", phy_utils_read_radioreg(pi, 0xdcd ));
	printf("20696: 0x1ce 0x%04X\n", phy_utils_read_radioreg(pi, 0xdce ));
	printf("20696: 0x1cf 0x%04X\n", phy_utils_read_radioreg(pi, 0xdcf ));
	printf("20696: 0x1d0 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd0 ));
	printf("20696: 0x1d1 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd1 ));
	printf("20696: 0x1d2 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd2 ));
	printf("20696: 0x1d3 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd3 ));
	printf("20696: 0x1d4 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd4 ));
	printf("20696: 0x1d5 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd5 ));
	printf("20696: 0x1d6 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd6 ));
	printf("20696: 0x1d7 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd7 ));
	printf("20696: 0x1d8 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd8 ));
	printf("20696: 0x1d9 0x%04X\n", phy_utils_read_radioreg(pi, 0xdd9 ));
	printf("20696: 0x1da 0x%04X\n", phy_utils_read_radioreg(pi, 0xdda ));
	printf("20696: 0x1db 0x%04X\n", phy_utils_read_radioreg(pi, 0xddb ));
	printf("20696: 0x1dc 0x%04X\n", phy_utils_read_radioreg(pi, 0xddc ));
	printf("20696: 0x1dd 0x%04X\n", phy_utils_read_radioreg(pi, 0xddd ));
	printf("20696: 0x1de 0x%04X\n", phy_utils_read_radioreg(pi, 0xdde ));
	printf("20696: 0x1df 0x%04X\n", phy_utils_read_radioreg(pi, 0xddf ));
	printf("20696: 0x1e0 0x%04X\n", phy_utils_read_radioreg(pi, 0xde0 ));
	printf("20696: 0x1e1 0x%04X\n", phy_utils_read_radioreg(pi, 0xde1 ));
	printf("20696: 0x1e2 0x%04X\n", phy_utils_read_radioreg(pi, 0xde2 ));
	printf("20696: 0x1e3 0x%04X\n", phy_utils_read_radioreg(pi, 0xde3 ));
	printf("20696: 0x1e4 0x%04X\n", phy_utils_read_radioreg(pi, 0xde4 ));
	printf("20696: 0x1ea 0x%04X\n", phy_utils_read_radioreg(pi, 0xdea ));
	printf("20696: 0x1eb 0x%04X\n", phy_utils_read_radioreg(pi, 0xdeb ));
	printf("20696: 0x1ec 0x%04X\n", phy_utils_read_radioreg(pi, 0xdec ));
	printf("20696: 0x1ed 0x%04X\n", phy_utils_read_radioreg(pi, 0xded ));
	printf("20696: 0x1ee 0x%04X\n", phy_utils_read_radioreg(pi, 0xdee ));
	printf("20696: 0x1f1 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf1 ));
	printf("20696: 0x1f2 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf2 ));
	printf("20696: 0x1f3 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf3 ));
	printf("20696: 0x1f4 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf4 ));
	printf("20696: 0x1f5 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf5 ));
	printf("20696: 0x1f6 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf6 ));
	printf("20696: 0x1f7 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf7 ));
	printf("20696: 0x1f8 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf8 ));
	printf("20696: 0x1f9 0x%04X\n", phy_utils_read_radioreg(pi, 0xdf9 ));
	printf("20696: 0x1fa 0x%04X\n", phy_utils_read_radioreg(pi, 0xdfa ));
	printf("20696: 0x1fb 0x%04X\n", phy_utils_read_radioreg(pi, 0xdfb ));
	printf("20696: 0x1fc 0x%04X\n", phy_utils_read_radioreg(pi, 0xdfc ));
	printf("20696: 0x1fd 0x%04X\n", phy_utils_read_radioreg(pi, 0xdfd ));
	printf("20696: 0x1ff 0x%04X\n", phy_utils_read_radioreg(pi, 0xdff ));
}


void dump_phyregs_7271(phy_info_t *pi);
void dump_phyregs_7271(phy_info_t *pi) {
	printf("acphy: 0x7b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b3));
	printf("acphy: 0x9b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b3));
	printf("acphy: 0xbb3 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb3));
	printf("acphy: 0xdb3 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb3));
	printf("acphy: 0x7b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b4));
	printf("acphy: 0x9b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b4));
	printf("acphy: 0xbb4 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb4));
	printf("acphy: 0xdb4 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb4));
	printf("acphy: 0x7b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b7));
	printf("acphy: 0x9b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b7));
	printf("acphy: 0xbb7 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb7));
	printf("acphy: 0xdb7 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb7));
	printf("acphy: 0x1048 0x%04X\n", phy_utils_read_phyreg(pi, 0x1048));
	printf("acphy: 0x1045 0x%04X\n", phy_utils_read_phyreg(pi, 0x1045));
	printf("acphy: 0x1044 0x%04X\n", phy_utils_read_phyreg(pi, 0x1044));
	printf("acphy: 0x1046 0x%04X\n", phy_utils_read_phyreg(pi, 0x1046));
	printf("acphy: 0x1f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f8));
	printf("acphy: 0x5b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x5b0));
	printf("acphy: 0x5af 0x%04X\n", phy_utils_read_phyreg(pi, 0x5af));
	printf("acphy: 0x5b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x5b1));
	printf("acphy: 0x7aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x7aa));
	printf("acphy: 0x9aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x9aa));
	printf("acphy: 0xbaa 0x%04X\n", phy_utils_read_phyreg(pi, 0xbaa));
	printf("acphy: 0xdaa 0x%04X\n", phy_utils_read_phyreg(pi, 0xdaa));
	printf("acphy: 0x7ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ae));
	printf("acphy: 0x9ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ae));
	printf("acphy: 0xbae 0x%04X\n", phy_utils_read_phyreg(pi, 0xbae));
	printf("acphy: 0xdae 0x%04X\n", phy_utils_read_phyreg(pi, 0xdae));
	printf("acphy: 0x7f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f9));
	printf("acphy: 0x9f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f9));
	printf("acphy: 0xbf9 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf9));
	printf("acphy: 0xdf9 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf9));
	printf("acphy: 0x7a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a9));
	printf("acphy: 0x9a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a9));
	printf("acphy: 0xba9 0x%04X\n", phy_utils_read_phyreg(pi, 0xba9));
	printf("acphy: 0xda9 0x%04X\n", phy_utils_read_phyreg(pi, 0xda9));
	printf("acphy: 0x7a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a8));
	printf("acphy: 0x9a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a8));
	printf("acphy: 0xba8 0x%04X\n", phy_utils_read_phyreg(pi, 0xba8));
	printf("acphy: 0xda8 0x%04X\n", phy_utils_read_phyreg(pi, 0xda8));
	printf("acphy: 0x5a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a1));
	printf("acphy: 0x5a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a0));
	printf("acphy: 0x5c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c1));
	printf("acphy: 0x5a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a6));
	printf("acphy: 0x5c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c8));
	printf("acphy: 0x5a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a7));
	printf("acphy: 0x5c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c9));
	printf("acphy: 0x5a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a4));
	printf("acphy: 0x5c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c6));
	printf("acphy: 0x5a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a5));
	printf("acphy: 0x5c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c7));
	printf("acphy: 0x5aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x5aa));
	printf("acphy: 0x5a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a9));
	printf("acphy: 0x5a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a8));
	printf("acphy: 0x5c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c5));
	printf("acphy: 0x5ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ae));
	printf("acphy: 0x7a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a3));
	printf("acphy: 0x9a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a3));
	printf("acphy: 0xba3 0x%04X\n", phy_utils_read_phyreg(pi, 0xba3));
	printf("acphy: 0xda3 0x%04X\n", phy_utils_read_phyreg(pi, 0xda3));
	printf("acphy: 0x7a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a1));
	printf("acphy: 0x9a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a1));
	printf("acphy: 0xba1 0x%04X\n", phy_utils_read_phyreg(pi, 0xba1));
	printf("acphy: 0xda1 0x%04X\n", phy_utils_read_phyreg(pi, 0xda1));
	printf("acphy: 0x7a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a7));
	printf("acphy: 0x7a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a6));
	printf("acphy: 0x9a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a7));
	printf("acphy: 0x9a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a6));
	printf("acphy: 0xba7 0x%04X\n", phy_utils_read_phyreg(pi, 0xba7));
	printf("acphy: 0xba6 0x%04X\n", phy_utils_read_phyreg(pi, 0xba6));
	printf("acphy: 0xda7 0x%04X\n", phy_utils_read_phyreg(pi, 0xda7));
	printf("acphy: 0xda6 0x%04X\n", phy_utils_read_phyreg(pi, 0xda6));
	printf("acphy: 0x174d 0x%04X\n", phy_utils_read_phyreg(pi, 0x174d));
	printf("acphy: 0x174e 0x%04X\n", phy_utils_read_phyreg(pi, 0x174e));
	printf("acphy: 0x194d 0x%04X\n", phy_utils_read_phyreg(pi, 0x194d));
	printf("acphy: 0x194e 0x%04X\n", phy_utils_read_phyreg(pi, 0x194e));
	printf("acphy: 0x1b4d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b4d));
	printf("acphy: 0x1b4e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b4e));
	printf("acphy: 0x1d4d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d4d));
	printf("acphy: 0x1d4e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d4e));
	printf("acphy: 0x174c 0x%04X\n", phy_utils_read_phyreg(pi, 0x174c));
	printf("acphy: 0x194c 0x%04X\n", phy_utils_read_phyreg(pi, 0x194c));
	printf("acphy: 0x1b4c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b4c));
	printf("acphy: 0x1d4c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d4c));
	printf("acphy: 0x174f 0x%04X\n", phy_utils_read_phyreg(pi, 0x174f));
	printf("acphy: 0x194f 0x%04X\n", phy_utils_read_phyreg(pi, 0x194f));
	printf("acphy: 0x1b4f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b4f));
	printf("acphy: 0x1d4f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d4f));
	printf("acphy: 0x7f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f6));
	printf("acphy: 0x9f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f6));
	printf("acphy: 0xbf6 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf6));
	printf("acphy: 0xdf6 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf6));
	printf("acphy: 0x7f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f5));
	printf("acphy: 0x9f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f5));
	printf("acphy: 0xbf5 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf5));
	printf("acphy: 0xdf5 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf5));
	printf("acphy: 0x7f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f8));
	printf("acphy: 0x7f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f7));
	printf("acphy: 0x9f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f8));
	printf("acphy: 0x9f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f7));
	printf("acphy: 0xbf8 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf8));
	printf("acphy: 0xbf7 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf7));
	printf("acphy: 0xdf8 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf8));
	printf("acphy: 0xdf7 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf7));
	printf("acphy: 0x7ad 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ad));
	printf("acphy: 0x9ad 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ad));
	printf("acphy: 0xbad 0x%04X\n", phy_utils_read_phyreg(pi, 0xbad));
	printf("acphy: 0xdad 0x%04X\n", phy_utils_read_phyreg(pi, 0xdad));
	printf("acphy: 0x7a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a2));
	printf("acphy: 0x9a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a2));
	printf("acphy: 0xba2 0x%04X\n", phy_utils_read_phyreg(pi, 0xba2));
	printf("acphy: 0xda2 0x%04X\n", phy_utils_read_phyreg(pi, 0xda2));
	printf("acphy: 0x7a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a0));
	printf("acphy: 0x9a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a0));
	printf("acphy: 0xba0 0x%04X\n", phy_utils_read_phyreg(pi, 0xba0));
	printf("acphy: 0xda0 0x%04X\n", phy_utils_read_phyreg(pi, 0xda0));
	printf("acphy: 0x7a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a5));
	printf("acphy: 0x7a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x7a4));
	printf("acphy: 0x9a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a5));
	printf("acphy: 0x9a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x9a4));
	printf("acphy: 0xba5 0x%04X\n", phy_utils_read_phyreg(pi, 0xba5));
	printf("acphy: 0xba4 0x%04X\n", phy_utils_read_phyreg(pi, 0xba4));
	printf("acphy: 0xda5 0x%04X\n", phy_utils_read_phyreg(pi, 0xda5));
	printf("acphy: 0xda4 0x%04X\n", phy_utils_read_phyreg(pi, 0xda4));
	printf("acphy: 0x7ac 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ac));
	printf("acphy: 0x9ac 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ac));
	printf("acphy: 0xbac 0x%04X\n", phy_utils_read_phyreg(pi, 0xbac));
	printf("acphy: 0xdac 0x%04X\n", phy_utils_read_phyreg(pi, 0xdac));
	printf("acphy: 0x7af 0x%04X\n", phy_utils_read_phyreg(pi, 0x7af));
	printf("acphy: 0x9af 0x%04X\n", phy_utils_read_phyreg(pi, 0x9af));
	printf("acphy: 0xbaf 0x%04X\n", phy_utils_read_phyreg(pi, 0xbaf));
	printf("acphy: 0xdaf 0x%04X\n", phy_utils_read_phyreg(pi, 0xdaf));
	printf("acphy: 0x7fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x7fa));
	printf("acphy: 0x9fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x9fa));
	printf("acphy: 0xbfa 0x%04X\n", phy_utils_read_phyreg(pi, 0xbfa));
	printf("acphy: 0xdfa 0x%04X\n", phy_utils_read_phyreg(pi, 0xdfa));
	printf("acphy: 0x7ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ab));
	printf("acphy: 0x9ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ab));
	printf("acphy: 0xbab 0x%04X\n", phy_utils_read_phyreg(pi, 0xbab));
	printf("acphy: 0xdab 0x%04X\n", phy_utils_read_phyreg(pi, 0xdab));
	printf("acphy: 0x5a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a2));
	printf("acphy: 0x5a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x5a3));
	printf("acphy: 0x1092 0x%04X\n", phy_utils_read_phyreg(pi, 0x1092));
	printf("acphy: 0x5b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x5b3));
	printf("acphy: 0x1fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x1fa));
	printf("acphy: 0x1fe 0x%04X\n", phy_utils_read_phyreg(pi, 0x1fe));
	printf("acphy: 0x5ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ab));
	printf("acphy: 0x1090 0x%04X\n", phy_utils_read_phyreg(pi, 0x1090));
	printf("acphy: 0x5ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ca));
	printf("acphy: 0x5b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x5b4));
	printf("acphy: 0x1f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f9));
	printf("acphy: 0x1fd 0x%04X\n", phy_utils_read_phyreg(pi, 0x1fd));
	printf("acphy: 0x5c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c2));
	printf("acphy: 0x5c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c3));
	printf("acphy: 0x5c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c4));
	printf("acphy: 0x5cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x5cb));
	printf("acphy: 0x5b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x5b2));
	printf("acphy: 0x5b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x5b5));
	printf("acphy: 0x5ad 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ad));
	printf("acphy: 0x5ac 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ac));
	printf("acphy: 0x165 0x%04X\n", phy_utils_read_phyreg(pi, 0x165));
	printf("acphy: 0x166 0x%04X\n", phy_utils_read_phyreg(pi, 0x166));
	printf("acphy: 0x167 0x%04X\n", phy_utils_read_phyreg(pi, 0x167));
	printf("acphy: 0x52a 0x%04X\n", phy_utils_read_phyreg(pi, 0x52a));
	printf("acphy: 0x467 0x%04X\n", phy_utils_read_phyreg(pi, 0x467));
	printf("acphy: 0x596 0x%04X\n", phy_utils_read_phyreg(pi, 0x596));
	printf("acphy: 0x598 0x%04X\n", phy_utils_read_phyreg(pi, 0x598));
	printf("acphy: 0x595 0x%04X\n", phy_utils_read_phyreg(pi, 0x595));
	printf("acphy: 0x594 0x%04X\n", phy_utils_read_phyreg(pi, 0x594));
	printf("acphy: 0x592 0x%04X\n", phy_utils_read_phyreg(pi, 0x592));
	printf("acphy: 0x593 0x%04X\n", phy_utils_read_phyreg(pi, 0x593));
	printf("acphy: 0x410 0x%04X\n", phy_utils_read_phyreg(pi, 0x410));
	printf("acphy: 0x74e 0x%04X\n", phy_utils_read_phyreg(pi, 0x74e));
	printf("acphy: 0x94e 0x%04X\n", phy_utils_read_phyreg(pi, 0x94e));
	printf("acphy: 0xb4e 0x%04X\n", phy_utils_read_phyreg(pi, 0xb4e));
	printf("acphy: 0xd4e 0x%04X\n", phy_utils_read_phyreg(pi, 0xd4e));
	printf("acphy: 0x749 0x%04X\n", phy_utils_read_phyreg(pi, 0x749));
	printf("acphy: 0x949 0x%04X\n", phy_utils_read_phyreg(pi, 0x949));
	printf("acphy: 0xb49 0x%04X\n", phy_utils_read_phyreg(pi, 0xb49));
	printf("acphy: 0xd49 0x%04X\n", phy_utils_read_phyreg(pi, 0xd49));
	printf("acphy: 0x751 0x%04X\n", phy_utils_read_phyreg(pi, 0x751));
	printf("acphy: 0x951 0x%04X\n", phy_utils_read_phyreg(pi, 0x951));
	printf("acphy: 0xb51 0x%04X\n", phy_utils_read_phyreg(pi, 0xb51));
	printf("acphy: 0xd51 0x%04X\n", phy_utils_read_phyreg(pi, 0xd51));
	printf("acphy: 0x752 0x%04X\n", phy_utils_read_phyreg(pi, 0x752));
	printf("acphy: 0x952 0x%04X\n", phy_utils_read_phyreg(pi, 0x952));
	printf("acphy: 0xb52 0x%04X\n", phy_utils_read_phyreg(pi, 0xb52));
	printf("acphy: 0xd52 0x%04X\n", phy_utils_read_phyreg(pi, 0xd52));
	printf("acphy: 0x753 0x%04X\n", phy_utils_read_phyreg(pi, 0x753));
	printf("acphy: 0x953 0x%04X\n", phy_utils_read_phyreg(pi, 0x953));
	printf("acphy: 0xb53 0x%04X\n", phy_utils_read_phyreg(pi, 0xb53));
	printf("acphy: 0xd53 0x%04X\n", phy_utils_read_phyreg(pi, 0xd53));
	printf("acphy: 0x750 0x%04X\n", phy_utils_read_phyreg(pi, 0x750));
	printf("acphy: 0x950 0x%04X\n", phy_utils_read_phyreg(pi, 0x950));
	printf("acphy: 0xb50 0x%04X\n", phy_utils_read_phyreg(pi, 0xb50));
	printf("acphy: 0xd50 0x%04X\n", phy_utils_read_phyreg(pi, 0xd50));
	printf("acphy: 0x40f 0x%04X\n", phy_utils_read_phyreg(pi, 0x40f));
	printf("acphy: 0x42d 0x%04X\n", phy_utils_read_phyreg(pi, 0x42d));
	printf("acphy: 0x42c 0x%04X\n", phy_utils_read_phyreg(pi, 0x42c));
	printf("acphy: 0x42b 0x%04X\n", phy_utils_read_phyreg(pi, 0x42b));
	printf("acphy: 0x42a 0x%04X\n", phy_utils_read_phyreg(pi, 0x42a));
	printf("acphy: 0x429 0x%04X\n", phy_utils_read_phyreg(pi, 0x429));
	printf("acphy: 0x424 0x%04X\n", phy_utils_read_phyreg(pi, 0x424));
	printf("acphy: 0x422 0x%04X\n", phy_utils_read_phyreg(pi, 0x422));
	printf("acphy: 0x420 0x%04X\n", phy_utils_read_phyreg(pi, 0x420));
	printf("acphy: 0x426 0x%04X\n", phy_utils_read_phyreg(pi, 0x426));
	printf("acphy: 0x425 0x%04X\n", phy_utils_read_phyreg(pi, 0x425));
	printf("acphy: 0x423 0x%04X\n", phy_utils_read_phyreg(pi, 0x423));
	printf("acphy: 0x421 0x%04X\n", phy_utils_read_phyreg(pi, 0x421));
	printf("acphy: 0x3b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b3));
	printf("acphy: 0x3d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d4));
	printf("acphy: 0x407 0x%04X\n", phy_utils_read_phyreg(pi, 0x407));
	printf("acphy: 0x251 0x%04X\n", phy_utils_read_phyreg(pi, 0x251));
	printf("acphy: 0x1161 0x%04X\n", phy_utils_read_phyreg(pi, 0x1161));
	printf("acphy: 0x253 0x%04X\n", phy_utils_read_phyreg(pi, 0x253));
	printf("acphy: 0x1163 0x%04X\n", phy_utils_read_phyreg(pi, 0x1163));
	printf("acphy: 0x252 0x%04X\n", phy_utils_read_phyreg(pi, 0x252));
	printf("acphy: 0x1162 0x%04X\n", phy_utils_read_phyreg(pi, 0x1162));
	printf("acphy: 0x254 0x%04X\n", phy_utils_read_phyreg(pi, 0x254));
	printf("acphy: 0x1164 0x%04X\n", phy_utils_read_phyreg(pi, 0x1164));
	printf("acphy: 0x1060 0x%04X\n", phy_utils_read_phyreg(pi, 0x1060));
	printf("acphy: 0x161 0x%04X\n", phy_utils_read_phyreg(pi, 0x161));
	printf("acphy: 0x1061 0x%04X\n", phy_utils_read_phyreg(pi, 0x1061));
	printf("acphy: 0x406 0x%04X\n", phy_utils_read_phyreg(pi, 0x406));
	printf("acphy: 0x47a 0x%04X\n", phy_utils_read_phyreg(pi, 0x47a));
	printf("acphy: 0x478 0x%04X\n", phy_utils_read_phyreg(pi, 0x478));
	printf("acphy: 0x479 0x%04X\n", phy_utils_read_phyreg(pi, 0x479));
	printf("acphy: 0x47d 0x%04X\n", phy_utils_read_phyreg(pi, 0x47d));
	printf("acphy: 0x47b 0x%04X\n", phy_utils_read_phyreg(pi, 0x47b));
	printf("acphy: 0x47c 0x%04X\n", phy_utils_read_phyreg(pi, 0x47c));
	printf("acphy: 0x691 0x%04X\n", phy_utils_read_phyreg(pi, 0x691));
	printf("acphy: 0x891 0x%04X\n", phy_utils_read_phyreg(pi, 0x891));
	printf("acphy: 0xa91 0x%04X\n", phy_utils_read_phyreg(pi, 0xa91));
	printf("acphy: 0xc91 0x%04X\n", phy_utils_read_phyreg(pi, 0xc91));
	printf("acphy: 0x1200 0x%04X\n", phy_utils_read_phyreg(pi, 0x1200));
	printf("acphy: 0x1201 0x%04X\n", phy_utils_read_phyreg(pi, 0x1201));
	printf("acphy: 0x7b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b8));
	printf("acphy: 0x9b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b8));
	printf("acphy: 0xbb8 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb8));
	printf("acphy: 0xdb8 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb8));
	printf("acphy: 0x7bf 0x%04X\n", phy_utils_read_phyreg(pi, 0x7bf));
	printf("acphy: 0x9bf 0x%04X\n", phy_utils_read_phyreg(pi, 0x9bf));
	printf("acphy: 0xbbf 0x%04X\n", phy_utils_read_phyreg(pi, 0xbbf));
	printf("acphy: 0xdbf 0x%04X\n", phy_utils_read_phyreg(pi, 0xdbf));
	printf("acphy: 0x7df 0x%04X\n", phy_utils_read_phyreg(pi, 0x7df));
	printf("acphy: 0x9df 0x%04X\n", phy_utils_read_phyreg(pi, 0x9df));
	printf("acphy: 0xbdf 0x%04X\n", phy_utils_read_phyreg(pi, 0xbdf));
	printf("acphy: 0xddf 0x%04X\n", phy_utils_read_phyreg(pi, 0xddf));
	printf("acphy: 0x454 0x%04X\n", phy_utils_read_phyreg(pi, 0x454));
	printf("acphy: 0x455 0x%04X\n", phy_utils_read_phyreg(pi, 0x455));
	printf("acphy: 0x456 0x%04X\n", phy_utils_read_phyreg(pi, 0x456));
	printf("acphy: 0x457 0x%04X\n", phy_utils_read_phyreg(pi, 0x457));
	printf("acphy: 0x458 0x%04X\n", phy_utils_read_phyreg(pi, 0x458));
	printf("acphy: 0x459 0x%04X\n", phy_utils_read_phyreg(pi, 0x459));
	printf("acphy: 0x442 0x%04X\n", phy_utils_read_phyreg(pi, 0x442));
	printf("acphy: 0x443 0x%04X\n", phy_utils_read_phyreg(pi, 0x443));
	printf("acphy: 0x44a 0x%04X\n", phy_utils_read_phyreg(pi, 0x44a));
	printf("acphy: 0x44b 0x%04X\n", phy_utils_read_phyreg(pi, 0x44b));
	printf("acphy: 0x44c 0x%04X\n", phy_utils_read_phyreg(pi, 0x44c));
	printf("acphy: 0x44d 0x%04X\n", phy_utils_read_phyreg(pi, 0x44d));
	printf("acphy: 0x44e 0x%04X\n", phy_utils_read_phyreg(pi, 0x44e));
	printf("acphy: 0x44f 0x%04X\n", phy_utils_read_phyreg(pi, 0x44f));
	printf("acphy: 0x450 0x%04X\n", phy_utils_read_phyreg(pi, 0x450));
	printf("acphy: 0x451 0x%04X\n", phy_utils_read_phyreg(pi, 0x451));
	printf("acphy: 0x452 0x%04X\n", phy_utils_read_phyreg(pi, 0x452));
	printf("acphy: 0x453 0x%04X\n", phy_utils_read_phyreg(pi, 0x453));
	printf("acphy: 0x446 0x%04X\n", phy_utils_read_phyreg(pi, 0x446));
	printf("acphy: 0x445 0x%04X\n", phy_utils_read_phyreg(pi, 0x445));
	printf("acphy: 0x444 0x%04X\n", phy_utils_read_phyreg(pi, 0x444));
	printf("acphy: 0x1120 0x%04X\n", phy_utils_read_phyreg(pi, 0x1120));
	printf("acphy: 0x1121 0x%04X\n", phy_utils_read_phyreg(pi, 0x1121));
	printf("acphy: 0x1122 0x%04X\n", phy_utils_read_phyreg(pi, 0x1122));
	printf("acphy: 0x1123 0x%04X\n", phy_utils_read_phyreg(pi, 0x1123));
	printf("acphy: 0x1124 0x%04X\n", phy_utils_read_phyreg(pi, 0x1124));
	printf("acphy: 0x1125 0x%04X\n", phy_utils_read_phyreg(pi, 0x1125));
	printf("acphy: 0x447 0x%04X\n", phy_utils_read_phyreg(pi, 0x447));
	printf("acphy: 0x448 0x%04X\n", phy_utils_read_phyreg(pi, 0x448));
	printf("acphy: 0x449 0x%04X\n", phy_utils_read_phyreg(pi, 0x449));
	printf("acphy: 0x45a 0x%04X\n", phy_utils_read_phyreg(pi, 0x45a));
	printf("acphy: 0x10e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e0));
	printf("acphy: 0x10e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e1));
	printf("acphy: 0x10e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e2));
	printf("acphy: 0x10e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e3));
	printf("acphy: 0x10e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e4));
	printf("acphy: 0x10e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e5));
	printf("acphy: 0x10e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e6));
	printf("acphy: 0x10e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e7));
	printf("acphy: 0x050 0x%04X\n", phy_utils_read_phyreg(pi, 0x050));
	printf("acphy: 0x430 0x%04X\n", phy_utils_read_phyreg(pi, 0x430));
	printf("acphy: 0x431 0x%04X\n", phy_utils_read_phyreg(pi, 0x431));
	printf("acphy: 0x432 0x%04X\n", phy_utils_read_phyreg(pi, 0x432));
	printf("acphy: 0x433 0x%04X\n", phy_utils_read_phyreg(pi, 0x433));
	printf("acphy: 0x023 0x%04X\n", phy_utils_read_phyreg(pi, 0x023));
	printf("acphy: 0x45b 0x%04X\n", phy_utils_read_phyreg(pi, 0x45b));
	printf("acphy: 0x45c 0x%04X\n", phy_utils_read_phyreg(pi, 0x45c));
	printf("acphy: 0x45d 0x%04X\n", phy_utils_read_phyreg(pi, 0x45d));
	printf("acphy: 0x45e 0x%04X\n", phy_utils_read_phyreg(pi, 0x45e));
	printf("acphy: 0x45f 0x%04X\n", phy_utils_read_phyreg(pi, 0x45f));
	printf("acphy: 0x10f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f0));
	printf("acphy: 0x10f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f1));
	printf("acphy: 0x10f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f2));
	printf("acphy: 0x10f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f4));
	printf("acphy: 0x10f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f5));
	printf("acphy: 0x434 0x%04X\n", phy_utils_read_phyreg(pi, 0x434));
	printf("acphy: 0x435 0x%04X\n", phy_utils_read_phyreg(pi, 0x435));
	printf("acphy: 0x436 0x%04X\n", phy_utils_read_phyreg(pi, 0x436));
	printf("acphy: 0x437 0x%04X\n", phy_utils_read_phyreg(pi, 0x437));
	printf("acphy: 0x438 0x%04X\n", phy_utils_read_phyreg(pi, 0x438));
	printf("acphy: 0x10f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f6));
	printf("acphy: 0x10fd 0x%04X\n", phy_utils_read_phyreg(pi, 0x10fd));
	printf("acphy: 0x10fe 0x%04X\n", phy_utils_read_phyreg(pi, 0x10fe));
	printf("acphy: 0x10ff 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ff));
	printf("acphy: 0x3e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e6));
	printf("acphy: 0x364 0x%04X\n", phy_utils_read_phyreg(pi, 0x364));
	printf("acphy: 0x17f 0x%04X\n", phy_utils_read_phyreg(pi, 0x17f));
	printf("acphy: 0x1fb 0x%04X\n", phy_utils_read_phyreg(pi, 0x1fb));
	printf("acphy: 0x15e 0x%04X\n", phy_utils_read_phyreg(pi, 0x15e));
	printf("acphy: 0x351 0x%04X\n", phy_utils_read_phyreg(pi, 0x351));
	printf("acphy: 0x352 0x%04X\n", phy_utils_read_phyreg(pi, 0x352));
	printf("acphy: 0x353 0x%04X\n", phy_utils_read_phyreg(pi, 0x353));
	printf("acphy: 0x34d 0x%04X\n", phy_utils_read_phyreg(pi, 0x34d));
	printf("acphy: 0x34e 0x%04X\n", phy_utils_read_phyreg(pi, 0x34e));
	printf("acphy: 0x34f 0x%04X\n", phy_utils_read_phyreg(pi, 0x34f));
	printf("acphy: 0x350 0x%04X\n", phy_utils_read_phyreg(pi, 0x350));
	printf("acphy: 0x3a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a1));
	printf("acphy: 0x3a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a8));
	printf("acphy: 0x390 0x%04X\n", phy_utils_read_phyreg(pi, 0x390));
	printf("acphy: 0x391 0x%04X\n", phy_utils_read_phyreg(pi, 0x391));
	printf("acphy: 0x397 0x%04X\n", phy_utils_read_phyreg(pi, 0x397));
	printf("acphy: 0x398 0x%04X\n", phy_utils_read_phyreg(pi, 0x398));
	printf("acphy: 0x396 0x%04X\n", phy_utils_read_phyreg(pi, 0x396));
	printf("acphy: 0x399 0x%04X\n", phy_utils_read_phyreg(pi, 0x399));
	printf("acphy: 0x39a 0x%04X\n", phy_utils_read_phyreg(pi, 0x39a));
	printf("acphy: 0x39d 0x%04X\n", phy_utils_read_phyreg(pi, 0x39d));
	printf("acphy: 0x293 0x%04X\n", phy_utils_read_phyreg(pi, 0x293));
	printf("acphy: 0x294 0x%04X\n", phy_utils_read_phyreg(pi, 0x294));
	printf("acphy: 0x295 0x%04X\n", phy_utils_read_phyreg(pi, 0x295));
	printf("acphy: 0x3a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a6));
	printf("acphy: 0x174 0x%04X\n", phy_utils_read_phyreg(pi, 0x174));
	printf("acphy: 0x3f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x3f3));
	printf("acphy: 0x35d 0x%04X\n", phy_utils_read_phyreg(pi, 0x35d));
	printf("acphy: 0x35e 0x%04X\n", phy_utils_read_phyreg(pi, 0x35e));
	printf("acphy: 0x35f 0x%04X\n", phy_utils_read_phyreg(pi, 0x35f));
	printf("acphy: 0x360 0x%04X\n", phy_utils_read_phyreg(pi, 0x360));
	printf("acphy: 0x361 0x%04X\n", phy_utils_read_phyreg(pi, 0x361));
	printf("acphy: 0x362 0x%04X\n", phy_utils_read_phyreg(pi, 0x362));
	printf("acphy: 0x363 0x%04X\n", phy_utils_read_phyreg(pi, 0x363));
	printf("acphy: 0x3a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a2));
	printf("acphy: 0x3a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a3));
	printf("acphy: 0x3a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a4));
	printf("acphy: 0x3a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a5));
	printf("acphy: 0x3a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a9));
	printf("acphy: 0x46a 0x%04X\n", phy_utils_read_phyreg(pi, 0x46a));
	printf("acphy: 0x3a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x3a7));
	printf("acphy: 0x3f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x3f0));
	printf("acphy: 0x29b 0x%04X\n", phy_utils_read_phyreg(pi, 0x29b));
	printf("acphy: 0x418 0x%04X\n", phy_utils_read_phyreg(pi, 0x418));
	printf("acphy: 0x40a 0x%04X\n", phy_utils_read_phyreg(pi, 0x40a));
	printf("acphy: 0x1031 0x%04X\n", phy_utils_read_phyreg(pi, 0x1031));
	printf("acphy: 0x1032 0x%04X\n", phy_utils_read_phyreg(pi, 0x1032));
	printf("acphy: 0x1033 0x%04X\n", phy_utils_read_phyreg(pi, 0x1033));
	printf("acphy: 0x371 0x%04X\n", phy_utils_read_phyreg(pi, 0x371));
	printf("acphy: 0x1750 0x%04X\n", phy_utils_read_phyreg(pi, 0x1750));
	printf("acphy: 0x1950 0x%04X\n", phy_utils_read_phyreg(pi, 0x1950));
	printf("acphy: 0x1b50 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b50));
	printf("acphy: 0x1d50 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d50));
	printf("acphy: 0x372 0x%04X\n", phy_utils_read_phyreg(pi, 0x372));
	printf("acphy: 0x1751 0x%04X\n", phy_utils_read_phyreg(pi, 0x1751));
	printf("acphy: 0x1951 0x%04X\n", phy_utils_read_phyreg(pi, 0x1951));
	printf("acphy: 0x1b51 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b51));
	printf("acphy: 0x1d51 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d51));
	printf("acphy: 0x373 0x%04X\n", phy_utils_read_phyreg(pi, 0x373));
	printf("acphy: 0x1752 0x%04X\n", phy_utils_read_phyreg(pi, 0x1752));
	printf("acphy: 0x1952 0x%04X\n", phy_utils_read_phyreg(pi, 0x1952));
	printf("acphy: 0x1b52 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b52));
	printf("acphy: 0x1d52 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d52));
	printf("acphy: 0x374 0x%04X\n", phy_utils_read_phyreg(pi, 0x374));
	printf("acphy: 0x1753 0x%04X\n", phy_utils_read_phyreg(pi, 0x1753));
	printf("acphy: 0x1953 0x%04X\n", phy_utils_read_phyreg(pi, 0x1953));
	printf("acphy: 0x1b53 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b53));
	printf("acphy: 0x1d53 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d53));
	printf("acphy: 0x375 0x%04X\n", phy_utils_read_phyreg(pi, 0x375));
	printf("acphy: 0x1754 0x%04X\n", phy_utils_read_phyreg(pi, 0x1754));
	printf("acphy: 0x1954 0x%04X\n", phy_utils_read_phyreg(pi, 0x1954));
	printf("acphy: 0x1b54 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b54));
	printf("acphy: 0x1d54 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d54));
	printf("acphy: 0x376 0x%04X\n", phy_utils_read_phyreg(pi, 0x376));
	printf("acphy: 0x1755 0x%04X\n", phy_utils_read_phyreg(pi, 0x1755));
	printf("acphy: 0x1955 0x%04X\n", phy_utils_read_phyreg(pi, 0x1955));
	printf("acphy: 0x1b55 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b55));
	printf("acphy: 0x1d55 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d55));
	printf("acphy: 0x378 0x%04X\n", phy_utils_read_phyreg(pi, 0x378));
	printf("acphy: 0x1756 0x%04X\n", phy_utils_read_phyreg(pi, 0x1756));
	printf("acphy: 0x1956 0x%04X\n", phy_utils_read_phyreg(pi, 0x1956));
	printf("acphy: 0x1b56 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b56));
	printf("acphy: 0x1d56 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d56));
	printf("acphy: 0x379 0x%04X\n", phy_utils_read_phyreg(pi, 0x379));
	printf("acphy: 0x1757 0x%04X\n", phy_utils_read_phyreg(pi, 0x1757));
	printf("acphy: 0x1957 0x%04X\n", phy_utils_read_phyreg(pi, 0x1957));
	printf("acphy: 0x1b57 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b57));
	printf("acphy: 0x1d57 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d57));
	printf("acphy: 0x04f 0x%04X\n", phy_utils_read_phyreg(pi, 0x04f));
	printf("acphy: 0x795 0x%04X\n", phy_utils_read_phyreg(pi, 0x795));
	printf("acphy: 0x995 0x%04X\n", phy_utils_read_phyreg(pi, 0x995));
	printf("acphy: 0xb95 0x%04X\n", phy_utils_read_phyreg(pi, 0xb95));
	printf("acphy: 0xd95 0x%04X\n", phy_utils_read_phyreg(pi, 0xd95));
	printf("acphy: 0x794 0x%04X\n", phy_utils_read_phyreg(pi, 0x794));
	printf("acphy: 0x994 0x%04X\n", phy_utils_read_phyreg(pi, 0x994));
	printf("acphy: 0xb94 0x%04X\n", phy_utils_read_phyreg(pi, 0xb94));
	printf("acphy: 0xd94 0x%04X\n", phy_utils_read_phyreg(pi, 0xd94));
	printf("acphy: 0x1ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ea));
	printf("acphy: 0x10c 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c));
	printf("acphy: 0x3d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d1));
	printf("acphy: 0x3d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d2));
	printf("acphy: 0x3d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d6));
	printf("acphy: 0x170 0x%04X\n", phy_utils_read_phyreg(pi, 0x170));
	printf("acphy: 0x296 0x%04X\n", phy_utils_read_phyreg(pi, 0x296));
	printf("acphy: 0x3d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d8));
	printf("acphy: 0x3d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d9));
	printf("acphy: 0x128b 0x%04X\n", phy_utils_read_phyreg(pi, 0x128b));
	printf("acphy: 0x128c 0x%04X\n", phy_utils_read_phyreg(pi, 0x128c));
	printf("acphy: 0x1289 0x%04X\n", phy_utils_read_phyreg(pi, 0x1289));
	printf("acphy: 0x128a 0x%04X\n", phy_utils_read_phyreg(pi, 0x128a));
	printf("acphy: 0x2d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d0));
	printf("acphy: 0x003 0x%04X\n", phy_utils_read_phyreg(pi, 0x003));
	printf("acphy: 0x004 0x%04X\n", phy_utils_read_phyreg(pi, 0x004));
	printf("acphy: 0x2dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x2dd));
	printf("acphy: 0x2de 0x%04X\n", phy_utils_read_phyreg(pi, 0x2de));
	printf("acphy: 0x2df 0x%04X\n", phy_utils_read_phyreg(pi, 0x2df));
	printf("acphy: 0x2d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d3));
	printf("acphy: 0x2d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d4));
	printf("acphy: 0x2af 0x%04X\n", phy_utils_read_phyreg(pi, 0x2af));
	printf("acphy: 0x1263 0x%04X\n", phy_utils_read_phyreg(pi, 0x1263));
	printf("acphy: 0x2a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a0));
	printf("acphy: 0x2a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a1));
	printf("acphy: 0x2a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a2));
	printf("acphy: 0x2a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a3));
	printf("acphy: 0x2a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a4));
	printf("acphy: 0x2a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a8));
	printf("acphy: 0x2a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a9));
	printf("acphy: 0x2aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x2aa));
	printf("acphy: 0x2ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ab));
	printf("acphy: 0x2ac 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ac));
	printf("acphy: 0x2ad 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ad));
	printf("acphy: 0x2ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ae));
	printf("acphy: 0x140 0x%04X\n", phy_utils_read_phyreg(pi, 0x140));
	printf("acphy: 0x30f 0x%04X\n", phy_utils_read_phyreg(pi, 0x30f));
	printf("acphy: 0x310 0x%04X\n", phy_utils_read_phyreg(pi, 0x310));
	printf("acphy: 0x311 0x%04X\n", phy_utils_read_phyreg(pi, 0x311));
	printf("acphy: 0x312 0x%04X\n", phy_utils_read_phyreg(pi, 0x312));
	printf("acphy: 0x320 0x%04X\n", phy_utils_read_phyreg(pi, 0x320));
	printf("acphy: 0x313 0x%04X\n", phy_utils_read_phyreg(pi, 0x313));
	printf("acphy: 0x1e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e2));
	printf("acphy: 0x1e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e4));
	printf("acphy: 0x1e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e3));
	printf("acphy: 0x1e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e5));
	printf("acphy: 0x29e 0x%04X\n", phy_utils_read_phyreg(pi, 0x29e));
	printf("acphy: 0x1093 0x%04X\n", phy_utils_read_phyreg(pi, 0x1093));
	printf("acphy: 0x2e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e7));
	printf("acphy: 0x2e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e8));
	printf("acphy: 0x103f 0x%04X\n", phy_utils_read_phyreg(pi, 0x103f));
	printf("acphy: 0x6fd 0x%04X\n", phy_utils_read_phyreg(pi, 0x6fd));
	printf("acphy: 0x6fb 0x%04X\n", phy_utils_read_phyreg(pi, 0x6fb));
	printf("acphy: 0x6fc 0x%04X\n", phy_utils_read_phyreg(pi, 0x6fc));
	printf("acphy: 0x6f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f9));
	printf("acphy: 0x6ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ed));
	printf("acphy: 0x1720 0x%04X\n", phy_utils_read_phyreg(pi, 0x1720));
	printf("acphy: 0x6d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d5));
	printf("acphy: 0x6d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d4));
	printf("acphy: 0x6d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d8));
	printf("acphy: 0x6ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ef));
	printf("acphy: 0x1721 0x%04X\n", phy_utils_read_phyreg(pi, 0x1721));
	printf("acphy: 0x6f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f5));
	printf("acphy: 0x6f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f3));
	printf("acphy: 0x6ff 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ff));
	printf("acphy: 0x6d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d7));
	printf("acphy: 0x1733 0x%04X\n", phy_utils_read_phyreg(pi, 0x1733));
	printf("acphy: 0x6ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ea));
	printf("acphy: 0x1734 0x%04X\n", phy_utils_read_phyreg(pi, 0x1734));
	printf("acphy: 0x6d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d3));
	printf("acphy: 0x1732 0x%04X\n", phy_utils_read_phyreg(pi, 0x1732));
	printf("acphy: 0x6d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d0));
	printf("acphy: 0x1730 0x%04X\n", phy_utils_read_phyreg(pi, 0x1730));
	printf("acphy: 0x6d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d1));
	printf("acphy: 0x1731 0x%04X\n", phy_utils_read_phyreg(pi, 0x1731));
	printf("acphy: 0x6f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f6));
	printf("acphy: 0x6eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x6eb));
	printf("acphy: 0x6ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ec));
	printf("acphy: 0x6d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d9));
	printf("acphy: 0x6f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f2));
	printf("acphy: 0x1735 0x%04X\n", phy_utils_read_phyreg(pi, 0x1735));
	printf("acphy: 0x6f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f0));
	printf("acphy: 0x1736 0x%04X\n", phy_utils_read_phyreg(pi, 0x1736));
	printf("acphy: 0x6f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f1));
	printf("acphy: 0x6f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f4));
	printf("acphy: 0x8fd 0x%04X\n", phy_utils_read_phyreg(pi, 0x8fd));
	printf("acphy: 0x8fb 0x%04X\n", phy_utils_read_phyreg(pi, 0x8fb));
	printf("acphy: 0x8fc 0x%04X\n", phy_utils_read_phyreg(pi, 0x8fc));
	printf("acphy: 0x8f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f9));
	printf("acphy: 0x8ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ed));
	printf("acphy: 0x1920 0x%04X\n", phy_utils_read_phyreg(pi, 0x1920));
	printf("acphy: 0x6a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a2));
	printf("acphy: 0x6a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a3));
	printf("acphy: 0x8d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d5));
	printf("acphy: 0x8d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d4));
	printf("acphy: 0x8d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d8));
	printf("acphy: 0x8ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ef));
	printf("acphy: 0x1921 0x%04X\n", phy_utils_read_phyreg(pi, 0x1921));
	printf("acphy: 0x8f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f5));
	printf("acphy: 0x8f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f3));
	printf("acphy: 0x8ff 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ff));
	printf("acphy: 0x8d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d7));
	printf("acphy: 0x1933 0x%04X\n", phy_utils_read_phyreg(pi, 0x1933));
	printf("acphy: 0x8ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ea));
	printf("acphy: 0x1934 0x%04X\n", phy_utils_read_phyreg(pi, 0x1934));
	printf("acphy: 0x8d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d3));
	printf("acphy: 0x1932 0x%04X\n", phy_utils_read_phyreg(pi, 0x1932));
	printf("acphy: 0x8d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d0));
	printf("acphy: 0x1930 0x%04X\n", phy_utils_read_phyreg(pi, 0x1930));
	printf("acphy: 0x8d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d1));
	printf("acphy: 0x1931 0x%04X\n", phy_utils_read_phyreg(pi, 0x1931));
	printf("acphy: 0x8f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f6));
	printf("acphy: 0x8eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x8eb));
	printf("acphy: 0x8ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ec));
	printf("acphy: 0x6a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a0));
	printf("acphy: 0x6a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a1));
	printf("acphy: 0x8d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d9));
	printf("acphy: 0x8f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f2));
	printf("acphy: 0x1935 0x%04X\n", phy_utils_read_phyreg(pi, 0x1935));
	printf("acphy: 0x8f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f0));
	printf("acphy: 0x1936 0x%04X\n", phy_utils_read_phyreg(pi, 0x1936));
	printf("acphy: 0x8f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f1));
	printf("acphy: 0x8f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f4));
	printf("acphy: 0x600 0x%04X\n", phy_utils_read_phyreg(pi, 0x600));
	printf("acphy: 0xafd 0x%04X\n", phy_utils_read_phyreg(pi, 0xafd));
	printf("acphy: 0xafb 0x%04X\n", phy_utils_read_phyreg(pi, 0xafb));
	printf("acphy: 0xafc 0x%04X\n", phy_utils_read_phyreg(pi, 0xafc));
	printf("acphy: 0xaf9 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf9));
	printf("acphy: 0xaed 0x%04X\n", phy_utils_read_phyreg(pi, 0xaed));
	printf("acphy: 0x1b20 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b20));
	printf("acphy: 0x8a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a2));
	printf("acphy: 0x8a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a3));
	printf("acphy: 0xad5 0x%04X\n", phy_utils_read_phyreg(pi, 0xad5));
	printf("acphy: 0xad4 0x%04X\n", phy_utils_read_phyreg(pi, 0xad4));
	printf("acphy: 0xad8 0x%04X\n", phy_utils_read_phyreg(pi, 0xad8));
	printf("acphy: 0xaef 0x%04X\n", phy_utils_read_phyreg(pi, 0xaef));
	printf("acphy: 0x1b21 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b21));
	printf("acphy: 0xaf5 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf5));
	printf("acphy: 0xaf3 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf3));
	printf("acphy: 0xaff 0x%04X\n", phy_utils_read_phyreg(pi, 0xaff));
	printf("acphy: 0xad7 0x%04X\n", phy_utils_read_phyreg(pi, 0xad7));
	printf("acphy: 0x1b33 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b33));
	printf("acphy: 0xaea 0x%04X\n", phy_utils_read_phyreg(pi, 0xaea));
	printf("acphy: 0x1b34 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b34));
	printf("acphy: 0xad3 0x%04X\n", phy_utils_read_phyreg(pi, 0xad3));
	printf("acphy: 0x1b32 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b32));
	printf("acphy: 0xad0 0x%04X\n", phy_utils_read_phyreg(pi, 0xad0));
	printf("acphy: 0x1b30 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b30));
	printf("acphy: 0xad1 0x%04X\n", phy_utils_read_phyreg(pi, 0xad1));
	printf("acphy: 0x1b31 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b31));
	printf("acphy: 0xaf6 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf6));
	printf("acphy: 0xaeb 0x%04X\n", phy_utils_read_phyreg(pi, 0xaeb));
	printf("acphy: 0xaec 0x%04X\n", phy_utils_read_phyreg(pi, 0xaec));
	printf("acphy: 0x8a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a0));
	printf("acphy: 0x8a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a1));
	printf("acphy: 0xad9 0x%04X\n", phy_utils_read_phyreg(pi, 0xad9));
	printf("acphy: 0xaf2 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf2));
	printf("acphy: 0x1b35 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b35));
	printf("acphy: 0xaf0 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf0));
	printf("acphy: 0x1b36 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b36));
	printf("acphy: 0xaf1 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf1));
	printf("acphy: 0xaf4 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf4));
	printf("acphy: 0x800 0x%04X\n", phy_utils_read_phyreg(pi, 0x800));
	printf("acphy: 0xcfd 0x%04X\n", phy_utils_read_phyreg(pi, 0xcfd));
	printf("acphy: 0xcfb 0x%04X\n", phy_utils_read_phyreg(pi, 0xcfb));
	printf("acphy: 0xcfc 0x%04X\n", phy_utils_read_phyreg(pi, 0xcfc));
	printf("acphy: 0xcf9 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf9));
	printf("acphy: 0xced 0x%04X\n", phy_utils_read_phyreg(pi, 0xced));
	printf("acphy: 0x1d20 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d20));
	printf("acphy: 0xaa2 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa2));
	printf("acphy: 0xaa3 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa3));
	printf("acphy: 0xcd5 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd5));
	printf("acphy: 0xcd4 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd4));
	printf("acphy: 0xcd8 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd8));
	printf("acphy: 0xcef 0x%04X\n", phy_utils_read_phyreg(pi, 0xcef));
	printf("acphy: 0x1d21 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d21));
	printf("acphy: 0xcf5 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf5));
	printf("acphy: 0xcf3 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf3));
	printf("acphy: 0xcff 0x%04X\n", phy_utils_read_phyreg(pi, 0xcff));
	printf("acphy: 0xcd7 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd7));
	printf("acphy: 0x1d33 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d33));
	printf("acphy: 0xcea 0x%04X\n", phy_utils_read_phyreg(pi, 0xcea));
	printf("acphy: 0x1d34 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d34));
	printf("acphy: 0xcd3 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd3));
	printf("acphy: 0x1d32 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d32));
	printf("acphy: 0xcd0 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd0));
	printf("acphy: 0x1d30 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d30));
	printf("acphy: 0xcd1 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd1));
	printf("acphy: 0x1d31 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d31));
	printf("acphy: 0xcf6 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf6));
	printf("acphy: 0xceb 0x%04X\n", phy_utils_read_phyreg(pi, 0xceb));
	printf("acphy: 0xcec 0x%04X\n", phy_utils_read_phyreg(pi, 0xcec));
	printf("acphy: 0xaa0 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa0));
	printf("acphy: 0xaa1 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa1));
	printf("acphy: 0xcd9 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd9));
	printf("acphy: 0xcf2 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf2));
	printf("acphy: 0x1d35 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d35));
	printf("acphy: 0xcf0 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf0));
	printf("acphy: 0x1d36 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d36));
	printf("acphy: 0xcf1 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf1));
	printf("acphy: 0xcf4 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf4));
	printf("acphy: 0xa00 0x%04X\n", phy_utils_read_phyreg(pi, 0xa00));
	printf("acphy: 0xca2 0x%04X\n", phy_utils_read_phyreg(pi, 0xca2));
	printf("acphy: 0xca3 0x%04X\n", phy_utils_read_phyreg(pi, 0xca3));
	printf("acphy: 0xca0 0x%04X\n", phy_utils_read_phyreg(pi, 0xca0));
	printf("acphy: 0xca1 0x%04X\n", phy_utils_read_phyreg(pi, 0xca1));
	printf("acphy: 0xc00 0x%04X\n", phy_utils_read_phyreg(pi, 0xc00));
	printf("acphy: 0x160 0x%04X\n", phy_utils_read_phyreg(pi, 0x160));
	printf("acphy: 0x015 0x%04X\n", phy_utils_read_phyreg(pi, 0x015));
	printf("acphy: 0x016 0x%04X\n", phy_utils_read_phyreg(pi, 0x016));
	printf("acphy: 0x017 0x%04X\n", phy_utils_read_phyreg(pi, 0x017));
	printf("acphy: 0x01b 0x%04X\n", phy_utils_read_phyreg(pi, 0x01b));
	printf("acphy: 0x1660 0x%04X\n", phy_utils_read_phyreg(pi, 0x1660));
	printf("acphy: 0x1860 0x%04X\n", phy_utils_read_phyreg(pi, 0x1860));
	printf("acphy: 0x1a60 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a60));
	printf("acphy: 0x1c60 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c60));
	printf("acphy: 0x1662 0x%04X\n", phy_utils_read_phyreg(pi, 0x1662));
	printf("acphy: 0x1862 0x%04X\n", phy_utils_read_phyreg(pi, 0x1862));
	printf("acphy: 0x1a62 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a62));
	printf("acphy: 0x1c62 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c62));
	printf("acphy: 0x1661 0x%04X\n", phy_utils_read_phyreg(pi, 0x1661));
	printf("acphy: 0x1861 0x%04X\n", phy_utils_read_phyreg(pi, 0x1861));
	printf("acphy: 0x1a61 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a61));
	printf("acphy: 0x1c61 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c61));
	printf("acphy: 0x1663 0x%04X\n", phy_utils_read_phyreg(pi, 0x1663));
	printf("acphy: 0x1863 0x%04X\n", phy_utils_read_phyreg(pi, 0x1863));
	printf("acphy: 0x1a63 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a63));
	printf("acphy: 0x1c63 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c63));
	printf("acphy: 0x167c 0x%04X\n", phy_utils_read_phyreg(pi, 0x167c));
	printf("acphy: 0x187c 0x%04X\n", phy_utils_read_phyreg(pi, 0x187c));
	printf("acphy: 0x1a7c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a7c));
	printf("acphy: 0x1c7c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c7c));
	printf("acphy: 0x167d 0x%04X\n", phy_utils_read_phyreg(pi, 0x167d));
	printf("acphy: 0x187d 0x%04X\n", phy_utils_read_phyreg(pi, 0x187d));
	printf("acphy: 0x1a7d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a7d));
	printf("acphy: 0x1c7d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c7d));
	printf("acphy: 0x3cc 0x%04X\n", phy_utils_read_phyreg(pi, 0x3cc));
	printf("acphy: 0x1678 0x%04X\n", phy_utils_read_phyreg(pi, 0x1678));
	printf("acphy: 0x1878 0x%04X\n", phy_utils_read_phyreg(pi, 0x1878));
	printf("acphy: 0x1a78 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a78));
	printf("acphy: 0x1c78 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c78));
	printf("acphy: 0x167a 0x%04X\n", phy_utils_read_phyreg(pi, 0x167a));
	printf("acphy: 0x187a 0x%04X\n", phy_utils_read_phyreg(pi, 0x187a));
	printf("acphy: 0x1a7a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a7a));
	printf("acphy: 0x1c7a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c7a));
	printf("acphy: 0x1679 0x%04X\n", phy_utils_read_phyreg(pi, 0x1679));
	printf("acphy: 0x1879 0x%04X\n", phy_utils_read_phyreg(pi, 0x1879));
	printf("acphy: 0x1a79 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a79));
	printf("acphy: 0x1c79 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c79));
	printf("acphy: 0x167b 0x%04X\n", phy_utils_read_phyreg(pi, 0x167b));
	printf("acphy: 0x187b 0x%04X\n", phy_utils_read_phyreg(pi, 0x187b));
	printf("acphy: 0x1a7b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a7b));
	printf("acphy: 0x1c7b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c7b));
	printf("acphy: 0x166c 0x%04X\n", phy_utils_read_phyreg(pi, 0x166c));
	printf("acphy: 0x186c 0x%04X\n", phy_utils_read_phyreg(pi, 0x186c));
	printf("acphy: 0x1a6c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a6c));
	printf("acphy: 0x1c6c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c6c));
	printf("acphy: 0x166e 0x%04X\n", phy_utils_read_phyreg(pi, 0x166e));
	printf("acphy: 0x186e 0x%04X\n", phy_utils_read_phyreg(pi, 0x186e));
	printf("acphy: 0x1a6e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a6e));
	printf("acphy: 0x1c6e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c6e));
	printf("acphy: 0x166d 0x%04X\n", phy_utils_read_phyreg(pi, 0x166d));
	printf("acphy: 0x186d 0x%04X\n", phy_utils_read_phyreg(pi, 0x186d));
	printf("acphy: 0x1a6d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a6d));
	printf("acphy: 0x1c6d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c6d));
	printf("acphy: 0x166f 0x%04X\n", phy_utils_read_phyreg(pi, 0x166f));
	printf("acphy: 0x186f 0x%04X\n", phy_utils_read_phyreg(pi, 0x186f));
	printf("acphy: 0x1a6f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a6f));
	printf("acphy: 0x1c6f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c6f));
	printf("acphy: 0x355 0x%04X\n", phy_utils_read_phyreg(pi, 0x355));
	printf("acphy: 0x354 0x%04X\n", phy_utils_read_phyreg(pi, 0x354));
	printf("acphy: 0x357 0x%04X\n", phy_utils_read_phyreg(pi, 0x357));
	printf("acphy: 0x356 0x%04X\n", phy_utils_read_phyreg(pi, 0x356));
	printf("acphy: 0x712 0x%04X\n", phy_utils_read_phyreg(pi, 0x712));
	printf("acphy: 0x912 0x%04X\n", phy_utils_read_phyreg(pi, 0x912));
	printf("acphy: 0xb12 0x%04X\n", phy_utils_read_phyreg(pi, 0xb12));
	printf("acphy: 0xd12 0x%04X\n", phy_utils_read_phyreg(pi, 0xd12));
	printf("acphy: 0x330 0x%04X\n", phy_utils_read_phyreg(pi, 0x330));
	printf("acphy: 0x331 0x%04X\n", phy_utils_read_phyreg(pi, 0x331));
	printf("acphy: 0x332 0x%04X\n", phy_utils_read_phyreg(pi, 0x332));
	printf("acphy: 0x710 0x%04X\n", phy_utils_read_phyreg(pi, 0x710));
	printf("acphy: 0x910 0x%04X\n", phy_utils_read_phyreg(pi, 0x910));
	printf("acphy: 0xb10 0x%04X\n", phy_utils_read_phyreg(pi, 0xb10));
	printf("acphy: 0xd10 0x%04X\n", phy_utils_read_phyreg(pi, 0xd10));
	printf("acphy: 0x324 0x%04X\n", phy_utils_read_phyreg(pi, 0x324));
	printf("acphy: 0x325 0x%04X\n", phy_utils_read_phyreg(pi, 0x325));
	printf("acphy: 0x326 0x%04X\n", phy_utils_read_phyreg(pi, 0x326));
	printf("acphy: 0x176 0x%04X\n", phy_utils_read_phyreg(pi, 0x176));
	printf("acphy: 0x57b 0x%04X\n", phy_utils_read_phyreg(pi, 0x57b));
	printf("acphy: 0x57d 0x%04X\n", phy_utils_read_phyreg(pi, 0x57d));
	printf("acphy: 0x57f 0x%04X\n", phy_utils_read_phyreg(pi, 0x57f));
	printf("acphy: 0x578 0x%04X\n", phy_utils_read_phyreg(pi, 0x578));
	printf("acphy: 0x579 0x%04X\n", phy_utils_read_phyreg(pi, 0x579));
	printf("acphy: 0x57c 0x%04X\n", phy_utils_read_phyreg(pi, 0x57c));
	printf("acphy: 0x57e 0x%04X\n", phy_utils_read_phyreg(pi, 0x57e));
	printf("acphy: 0x57a 0x%04X\n", phy_utils_read_phyreg(pi, 0x57a));
	printf("acphy: 0x1668 0x%04X\n", phy_utils_read_phyreg(pi, 0x1668));
	printf("acphy: 0x1868 0x%04X\n", phy_utils_read_phyreg(pi, 0x1868));
	printf("acphy: 0x1a68 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a68));
	printf("acphy: 0x1c68 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c68));
	printf("acphy: 0x166a 0x%04X\n", phy_utils_read_phyreg(pi, 0x166a));
	printf("acphy: 0x186a 0x%04X\n", phy_utils_read_phyreg(pi, 0x186a));
	printf("acphy: 0x1a6a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a6a));
	printf("acphy: 0x1c6a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c6a));
	printf("acphy: 0x1669 0x%04X\n", phy_utils_read_phyreg(pi, 0x1669));
	printf("acphy: 0x1869 0x%04X\n", phy_utils_read_phyreg(pi, 0x1869));
	printf("acphy: 0x1a69 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a69));
	printf("acphy: 0x1c69 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c69));
	printf("acphy: 0x166b 0x%04X\n", phy_utils_read_phyreg(pi, 0x166b));
	printf("acphy: 0x186b 0x%04X\n", phy_utils_read_phyreg(pi, 0x186b));
	printf("acphy: 0x1a6b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a6b));
	printf("acphy: 0x1c6b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c6b));
	printf("acphy: 0x1670 0x%04X\n", phy_utils_read_phyreg(pi, 0x1670));
	printf("acphy: 0x1870 0x%04X\n", phy_utils_read_phyreg(pi, 0x1870));
	printf("acphy: 0x1a70 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a70));
	printf("acphy: 0x1c70 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c70));
	printf("acphy: 0x1672 0x%04X\n", phy_utils_read_phyreg(pi, 0x1672));
	printf("acphy: 0x1872 0x%04X\n", phy_utils_read_phyreg(pi, 0x1872));
	printf("acphy: 0x1a72 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a72));
	printf("acphy: 0x1c72 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c72));
	printf("acphy: 0x1671 0x%04X\n", phy_utils_read_phyreg(pi, 0x1671));
	printf("acphy: 0x1871 0x%04X\n", phy_utils_read_phyreg(pi, 0x1871));
	printf("acphy: 0x1a71 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a71));
	printf("acphy: 0x1c71 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c71));
	printf("acphy: 0x1673 0x%04X\n", phy_utils_read_phyreg(pi, 0x1673));
	printf("acphy: 0x1873 0x%04X\n", phy_utils_read_phyreg(pi, 0x1873));
	printf("acphy: 0x1a73 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a73));
	printf("acphy: 0x1c73 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c73));
	printf("acphy: 0x2c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c3));
	printf("acphy: 0x2c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c4));
	printf("acphy: 0x2c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c5));
	printf("acphy: 0x2c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c1));
	printf("acphy: 0x2c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c2));
	printf("acphy: 0x046 0x%04X\n", phy_utils_read_phyreg(pi, 0x046));
	printf("acphy: 0x047 0x%04X\n", phy_utils_read_phyreg(pi, 0x047));
	printf("acphy: 0x048 0x%04X\n", phy_utils_read_phyreg(pi, 0x048));
	printf("acphy: 0x053 0x%04X\n", phy_utils_read_phyreg(pi, 0x053));
	printf("acphy: 0x052 0x%04X\n", phy_utils_read_phyreg(pi, 0x052));
	printf("acphy: 0x747 0x%04X\n", phy_utils_read_phyreg(pi, 0x747));
	printf("acphy: 0x947 0x%04X\n", phy_utils_read_phyreg(pi, 0x947));
	printf("acphy: 0xb47 0x%04X\n", phy_utils_read_phyreg(pi, 0xb47));
	printf("acphy: 0xd47 0x%04X\n", phy_utils_read_phyreg(pi, 0xd47));
	printf("acphy: 0x1070 0x%04X\n", phy_utils_read_phyreg(pi, 0x1070));
	printf("acphy: 0x1071 0x%04X\n", phy_utils_read_phyreg(pi, 0x1071));
	printf("acphy: 0x1074 0x%04X\n", phy_utils_read_phyreg(pi, 0x1074));
	printf("acphy: 0x1072 0x%04X\n", phy_utils_read_phyreg(pi, 0x1072));
	printf("acphy: 0x173 0x%04X\n", phy_utils_read_phyreg(pi, 0x173));
	printf("acphy: 0x10c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c6));
	printf("acphy: 0x10c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c7));
	printf("acphy: 0x41e 0x%04X\n", phy_utils_read_phyreg(pi, 0x41e));
	printf("acphy: 0x41f 0x%04X\n", phy_utils_read_phyreg(pi, 0x41f));
	printf("acphy: 0x17e 0x%04X\n", phy_utils_read_phyreg(pi, 0x17e));
	printf("acphy: 0x102c 0x%04X\n", phy_utils_read_phyreg(pi, 0x102c));
	printf("acphy: 0x102d 0x%04X\n", phy_utils_read_phyreg(pi, 0x102d));
	printf("acphy: 0x59c 0x%04X\n", phy_utils_read_phyreg(pi, 0x59c));
	printf("acphy: 0x7c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c0));
	printf("acphy: 0x9c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c0));
	printf("acphy: 0xbc0 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc0));
	printf("acphy: 0xdc0 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc0));
	printf("acphy: 0x7ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ca));
	printf("acphy: 0x9ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ca));
	printf("acphy: 0xbca 0x%04X\n", phy_utils_read_phyreg(pi, 0xbca));
	printf("acphy: 0xdca 0x%04X\n", phy_utils_read_phyreg(pi, 0xdca));
	printf("acphy: 0x7cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x7cb));
	printf("acphy: 0x9cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x9cb));
	printf("acphy: 0xbcb 0x%04X\n", phy_utils_read_phyreg(pi, 0xbcb));
	printf("acphy: 0xdcb 0x%04X\n", phy_utils_read_phyreg(pi, 0xdcb));
	printf("acphy: 0x7cc 0x%04X\n", phy_utils_read_phyreg(pi, 0x7cc));
	printf("acphy: 0x9cc 0x%04X\n", phy_utils_read_phyreg(pi, 0x9cc));
	printf("acphy: 0xbcc 0x%04X\n", phy_utils_read_phyreg(pi, 0xbcc));
	printf("acphy: 0xdcc 0x%04X\n", phy_utils_read_phyreg(pi, 0xdcc));
	printf("acphy: 0x7cd 0x%04X\n", phy_utils_read_phyreg(pi, 0x7cd));
	printf("acphy: 0x9cd 0x%04X\n", phy_utils_read_phyreg(pi, 0x9cd));
	printf("acphy: 0xbcd 0x%04X\n", phy_utils_read_phyreg(pi, 0xbcd));
	printf("acphy: 0xdcd 0x%04X\n", phy_utils_read_phyreg(pi, 0xdcd));
	printf("acphy: 0x7ce 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ce));
	printf("acphy: 0x9ce 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ce));
	printf("acphy: 0xbce 0x%04X\n", phy_utils_read_phyreg(pi, 0xbce));
	printf("acphy: 0xdce 0x%04X\n", phy_utils_read_phyreg(pi, 0xdce));
	printf("acphy: 0x7cf 0x%04X\n", phy_utils_read_phyreg(pi, 0x7cf));
	printf("acphy: 0x9cf 0x%04X\n", phy_utils_read_phyreg(pi, 0x9cf));
	printf("acphy: 0xbcf 0x%04X\n", phy_utils_read_phyreg(pi, 0xbcf));
	printf("acphy: 0xdcf 0x%04X\n", phy_utils_read_phyreg(pi, 0xdcf));
	printf("acphy: 0x754 0x%04X\n", phy_utils_read_phyreg(pi, 0x754));
	printf("acphy: 0x954 0x%04X\n", phy_utils_read_phyreg(pi, 0x954));
	printf("acphy: 0xb54 0x%04X\n", phy_utils_read_phyreg(pi, 0xb54));
	printf("acphy: 0xd54 0x%04X\n", phy_utils_read_phyreg(pi, 0xd54));
	printf("acphy: 0x756 0x%04X\n", phy_utils_read_phyreg(pi, 0x756));
	printf("acphy: 0x956 0x%04X\n", phy_utils_read_phyreg(pi, 0x956));
	printf("acphy: 0xb56 0x%04X\n", phy_utils_read_phyreg(pi, 0xb56));
	printf("acphy: 0xd56 0x%04X\n", phy_utils_read_phyreg(pi, 0xd56));
	printf("acphy: 0x757 0x%04X\n", phy_utils_read_phyreg(pi, 0x757));
	printf("acphy: 0x957 0x%04X\n", phy_utils_read_phyreg(pi, 0x957));
	printf("acphy: 0xb57 0x%04X\n", phy_utils_read_phyreg(pi, 0xb57));
	printf("acphy: 0xd57 0x%04X\n", phy_utils_read_phyreg(pi, 0xd57));
	printf("acphy: 0x758 0x%04X\n", phy_utils_read_phyreg(pi, 0x758));
	printf("acphy: 0x958 0x%04X\n", phy_utils_read_phyreg(pi, 0x958));
	printf("acphy: 0xb58 0x%04X\n", phy_utils_read_phyreg(pi, 0xb58));
	printf("acphy: 0xd58 0x%04X\n", phy_utils_read_phyreg(pi, 0xd58));
	printf("acphy: 0x759 0x%04X\n", phy_utils_read_phyreg(pi, 0x759));
	printf("acphy: 0x959 0x%04X\n", phy_utils_read_phyreg(pi, 0x959));
	printf("acphy: 0xb59 0x%04X\n", phy_utils_read_phyreg(pi, 0xb59));
	printf("acphy: 0xd59 0x%04X\n", phy_utils_read_phyreg(pi, 0xd59));
	printf("acphy: 0x75a 0x%04X\n", phy_utils_read_phyreg(pi, 0x75a));
	printf("acphy: 0x95a 0x%04X\n", phy_utils_read_phyreg(pi, 0x95a));
	printf("acphy: 0xb5a 0x%04X\n", phy_utils_read_phyreg(pi, 0xb5a));
	printf("acphy: 0xd5a 0x%04X\n", phy_utils_read_phyreg(pi, 0xd5a));
	printf("acphy: 0x75b 0x%04X\n", phy_utils_read_phyreg(pi, 0x75b));
	printf("acphy: 0x95b 0x%04X\n", phy_utils_read_phyreg(pi, 0x95b));
	printf("acphy: 0xb5b 0x%04X\n", phy_utils_read_phyreg(pi, 0xb5b));
	printf("acphy: 0xd5b 0x%04X\n", phy_utils_read_phyreg(pi, 0xd5b));
	printf("acphy: 0x7fe 0x%04X\n", phy_utils_read_phyreg(pi, 0x7fe));
	printf("acphy: 0x9fe 0x%04X\n", phy_utils_read_phyreg(pi, 0x9fe));
	printf("acphy: 0xbfe 0x%04X\n", phy_utils_read_phyreg(pi, 0xbfe));
	printf("acphy: 0xdfe 0x%04X\n", phy_utils_read_phyreg(pi, 0xdfe));
	printf("acphy: 0x7ff 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ff));
	printf("acphy: 0x9ff 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ff));
	printf("acphy: 0xbff 0x%04X\n", phy_utils_read_phyreg(pi, 0xbff));
	printf("acphy: 0xdff 0x%04X\n", phy_utils_read_phyreg(pi, 0xdff));
	printf("acphy: 0x7fb 0x%04X\n", phy_utils_read_phyreg(pi, 0x7fb));
	printf("acphy: 0x9fb 0x%04X\n", phy_utils_read_phyreg(pi, 0x9fb));
	printf("acphy: 0xbfb 0x%04X\n", phy_utils_read_phyreg(pi, 0xbfb));
	printf("acphy: 0xdfb 0x%04X\n", phy_utils_read_phyreg(pi, 0xdfb));
	printf("acphy: 0x7fc 0x%04X\n", phy_utils_read_phyreg(pi, 0x7fc));
	printf("acphy: 0x9fc 0x%04X\n", phy_utils_read_phyreg(pi, 0x9fc));
	printf("acphy: 0xbfc 0x%04X\n", phy_utils_read_phyreg(pi, 0xbfc));
	printf("acphy: 0xdfc 0x%04X\n", phy_utils_read_phyreg(pi, 0xdfc));
	printf("acphy: 0x7fd 0x%04X\n", phy_utils_read_phyreg(pi, 0x7fd));
	printf("acphy: 0x9fd 0x%04X\n", phy_utils_read_phyreg(pi, 0x9fd));
	printf("acphy: 0xbfd 0x%04X\n", phy_utils_read_phyreg(pi, 0xbfd));
	printf("acphy: 0xdfd 0x%04X\n", phy_utils_read_phyreg(pi, 0xdfd));
	printf("acphy: 0x75c 0x%04X\n", phy_utils_read_phyreg(pi, 0x75c));
	printf("acphy: 0x95c 0x%04X\n", phy_utils_read_phyreg(pi, 0x95c));
	printf("acphy: 0xb5c 0x%04X\n", phy_utils_read_phyreg(pi, 0xb5c));
	printf("acphy: 0xd5c 0x%04X\n", phy_utils_read_phyreg(pi, 0xd5c));
	printf("acphy: 0x75d 0x%04X\n", phy_utils_read_phyreg(pi, 0x75d));
	printf("acphy: 0x95d 0x%04X\n", phy_utils_read_phyreg(pi, 0x95d));
	printf("acphy: 0xb5d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb5d));
	printf("acphy: 0xd5d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd5d));
	printf("acphy: 0x75e 0x%04X\n", phy_utils_read_phyreg(pi, 0x75e));
	printf("acphy: 0x95e 0x%04X\n", phy_utils_read_phyreg(pi, 0x95e));
	printf("acphy: 0xb5e 0x%04X\n", phy_utils_read_phyreg(pi, 0xb5e));
	printf("acphy: 0xd5e 0x%04X\n", phy_utils_read_phyreg(pi, 0xd5e));
	printf("acphy: 0x75f 0x%04X\n", phy_utils_read_phyreg(pi, 0x75f));
	printf("acphy: 0x95f 0x%04X\n", phy_utils_read_phyreg(pi, 0x95f));
	printf("acphy: 0xb5f 0x%04X\n", phy_utils_read_phyreg(pi, 0xb5f));
	printf("acphy: 0xd5f 0x%04X\n", phy_utils_read_phyreg(pi, 0xd5f));
	printf("acphy: 0x170d 0x%04X\n", phy_utils_read_phyreg(pi, 0x170d));
	printf("acphy: 0x190d 0x%04X\n", phy_utils_read_phyreg(pi, 0x190d));
	printf("acphy: 0x1b0d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b0d));
	printf("acphy: 0x1d0d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d0d));
	printf("acphy: 0x170e 0x%04X\n", phy_utils_read_phyreg(pi, 0x170e));
	printf("acphy: 0x190e 0x%04X\n", phy_utils_read_phyreg(pi, 0x190e));
	printf("acphy: 0x1b0e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b0e));
	printf("acphy: 0x1d0e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d0e));
	printf("acphy: 0x7c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c4));
	printf("acphy: 0x9c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c4));
	printf("acphy: 0xbc4 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc4));
	printf("acphy: 0xdc4 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc4));
	printf("acphy: 0x1760 0x%04X\n", phy_utils_read_phyreg(pi, 0x1760));
	printf("acphy: 0x1960 0x%04X\n", phy_utils_read_phyreg(pi, 0x1960));
	printf("acphy: 0x1b60 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b60));
	printf("acphy: 0x1d60 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d60));
	printf("acphy: 0x1761 0x%04X\n", phy_utils_read_phyreg(pi, 0x1761));
	printf("acphy: 0x1961 0x%04X\n", phy_utils_read_phyreg(pi, 0x1961));
	printf("acphy: 0x1b61 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b61));
	printf("acphy: 0x1d61 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d61));
	printf("acphy: 0x1762 0x%04X\n", phy_utils_read_phyreg(pi, 0x1762));
	printf("acphy: 0x1962 0x%04X\n", phy_utils_read_phyreg(pi, 0x1962));
	printf("acphy: 0x1b62 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b62));
	printf("acphy: 0x1d62 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d62));
	printf("acphy: 0x1763 0x%04X\n", phy_utils_read_phyreg(pi, 0x1763));
	printf("acphy: 0x1963 0x%04X\n", phy_utils_read_phyreg(pi, 0x1963));
	printf("acphy: 0x1b63 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b63));
	printf("acphy: 0x1d63 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d63));
	printf("acphy: 0x1764 0x%04X\n", phy_utils_read_phyreg(pi, 0x1764));
	printf("acphy: 0x1964 0x%04X\n", phy_utils_read_phyreg(pi, 0x1964));
	printf("acphy: 0x1b64 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b64));
	printf("acphy: 0x1d64 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d64));
	printf("acphy: 0x1765 0x%04X\n", phy_utils_read_phyreg(pi, 0x1765));
	printf("acphy: 0x1965 0x%04X\n", phy_utils_read_phyreg(pi, 0x1965));
	printf("acphy: 0x1b65 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b65));
	printf("acphy: 0x1d65 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d65));
	printf("acphy: 0x7c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c5));
	printf("acphy: 0x9c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c5));
	printf("acphy: 0xbc5 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc5));
	printf("acphy: 0xdc5 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc5));
	printf("acphy: 0x7c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c6));
	printf("acphy: 0x9c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c6));
	printf("acphy: 0xbc6 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc6));
	printf("acphy: 0xdc6 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc6));
	printf("acphy: 0x7c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c7));
	printf("acphy: 0x9c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c7));
	printf("acphy: 0xbc7 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc7));
	printf("acphy: 0xdc7 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc7));
	printf("acphy: 0x7c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c8));
	printf("acphy: 0x9c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c8));
	printf("acphy: 0xbc8 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc8));
	printf("acphy: 0xdc8 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc8));
	printf("acphy: 0x7c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c9));
	printf("acphy: 0x9c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c9));
	printf("acphy: 0xbc9 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc9));
	printf("acphy: 0xdc9 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc9));
	printf("acphy: 0x59e 0x%04X\n", phy_utils_read_phyreg(pi, 0x59e));
	printf("acphy: 0x599 0x%04X\n", phy_utils_read_phyreg(pi, 0x599));
	printf("acphy: 0x698 0x%04X\n", phy_utils_read_phyreg(pi, 0x698));
	printf("acphy: 0x898 0x%04X\n", phy_utils_read_phyreg(pi, 0x898));
	printf("acphy: 0xa98 0x%04X\n", phy_utils_read_phyreg(pi, 0xa98));
	printf("acphy: 0xc98 0x%04X\n", phy_utils_read_phyreg(pi, 0xc98));
	printf("acphy: 0x699 0x%04X\n", phy_utils_read_phyreg(pi, 0x699));
	printf("acphy: 0x899 0x%04X\n", phy_utils_read_phyreg(pi, 0x899));
	printf("acphy: 0xa99 0x%04X\n", phy_utils_read_phyreg(pi, 0xa99));
	printf("acphy: 0xc99 0x%04X\n", phy_utils_read_phyreg(pi, 0xc99));
	printf("acphy: 0x21a 0x%04X\n", phy_utils_read_phyreg(pi, 0x21a));
	printf("acphy: 0x21b 0x%04X\n", phy_utils_read_phyreg(pi, 0x21b));
	printf("acphy: 0x21c 0x%04X\n", phy_utils_read_phyreg(pi, 0x21c));
	printf("acphy: 0x597 0x%04X\n", phy_utils_read_phyreg(pi, 0x597));
	printf("acphy: 0x590 0x%04X\n", phy_utils_read_phyreg(pi, 0x590));
	printf("acphy: 0x1de 0x%04X\n", phy_utils_read_phyreg(pi, 0x1de));
	printf("acphy: 0x1d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d7));
	printf("acphy: 0x3d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d7));
	printf("acphy: 0x3f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x3f2));
	printf("acphy: 0x299 0x%04X\n", phy_utils_read_phyreg(pi, 0x299));
	printf("acphy: 0x3f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x3f1));
	printf("acphy: 0x1065 0x%04X\n", phy_utils_read_phyreg(pi, 0x1065));
	printf("acphy: 0x1062 0x%04X\n", phy_utils_read_phyreg(pi, 0x1062));
	printf("acphy: 0x1064 0x%04X\n", phy_utils_read_phyreg(pi, 0x1064));
	printf("acphy: 0x1063 0x%04X\n", phy_utils_read_phyreg(pi, 0x1063));
	printf("acphy: 0x1b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b6));
	printf("acphy: 0x1f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f2));
	printf("acphy: 0x358 0x%04X\n", phy_utils_read_phyreg(pi, 0x358));
	printf("acphy: 0x359 0x%04X\n", phy_utils_read_phyreg(pi, 0x359));
	printf("acphy: 0x35a 0x%04X\n", phy_utils_read_phyreg(pi, 0x35a));
	printf("acphy: 0x35b 0x%04X\n", phy_utils_read_phyreg(pi, 0x35b));
	printf("acphy: 0x1fc 0x%04X\n", phy_utils_read_phyreg(pi, 0x1fc));
	printf("acphy: 0x5c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x5c0));
	printf("acphy: 0x7b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b0));
	printf("acphy: 0x9b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b0));
	printf("acphy: 0xbb0 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb0));
	printf("acphy: 0xdb0 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb0));
	printf("acphy: 0x7b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b5));
	printf("acphy: 0x9b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b5));
	printf("acphy: 0xbb5 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb5));
	printf("acphy: 0xdb5 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb5));
	printf("acphy: 0x7b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b6));
	printf("acphy: 0x9b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b6));
	printf("acphy: 0xbb6 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb6));
	printf("acphy: 0xdb6 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb6));
	printf("acphy: 0x16bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x16bc));
	printf("acphy: 0x18bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x18bc));
	printf("acphy: 0x1abc 0x%04X\n", phy_utils_read_phyreg(pi, 0x1abc));
	printf("acphy: 0x1cbc 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cbc));
	printf("acphy: 0x7b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b1));
	printf("acphy: 0x9b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b1));
	printf("acphy: 0xbb1 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb1));
	printf("acphy: 0xdb1 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb1));
	printf("acphy: 0x7b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b2));
	printf("acphy: 0x9b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b2));
	printf("acphy: 0xbb2 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb2));
	printf("acphy: 0xdb2 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb2));
	printf("acphy: 0x16bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x16bb));
	printf("acphy: 0x18bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x18bb));
	printf("acphy: 0x1abb 0x%04X\n", phy_utils_read_phyreg(pi, 0x1abb));
	printf("acphy: 0x1cbb 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cbb));
	printf("acphy: 0x7b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x7b9));
	printf("acphy: 0x9b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x9b9));
	printf("acphy: 0xbb9 0x%04X\n", phy_utils_read_phyreg(pi, 0xbb9));
	printf("acphy: 0xdb9 0x%04X\n", phy_utils_read_phyreg(pi, 0xdb9));
	printf("acphy: 0x7bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x7bc));
	printf("acphy: 0x9bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x9bc));
	printf("acphy: 0xbbc 0x%04X\n", phy_utils_read_phyreg(pi, 0xbbc));
	printf("acphy: 0xdbc 0x%04X\n", phy_utils_read_phyreg(pi, 0xdbc));
	printf("acphy: 0x16b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b8));
	printf("acphy: 0x18b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b8));
	printf("acphy: 0x1ab8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab8));
	printf("acphy: 0x1cb8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb8));
	printf("acphy: 0x7ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x7ba));
	printf("acphy: 0x9ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x9ba));
	printf("acphy: 0xbba 0x%04X\n", phy_utils_read_phyreg(pi, 0xbba));
	printf("acphy: 0xdba 0x%04X\n", phy_utils_read_phyreg(pi, 0xdba));
	printf("acphy: 0x7bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x7bd));
	printf("acphy: 0x9bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x9bd));
	printf("acphy: 0xbbd 0x%04X\n", phy_utils_read_phyreg(pi, 0xbbd));
	printf("acphy: 0xdbd 0x%04X\n", phy_utils_read_phyreg(pi, 0xdbd));
	printf("acphy: 0x16b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b9));
	printf("acphy: 0x18b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b9));
	printf("acphy: 0x1ab9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab9));
	printf("acphy: 0x1cb9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb9));
	printf("acphy: 0x7bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x7bb));
	printf("acphy: 0x9bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x9bb));
	printf("acphy: 0xbbb 0x%04X\n", phy_utils_read_phyreg(pi, 0xbbb));
	printf("acphy: 0xdbb 0x%04X\n", phy_utils_read_phyreg(pi, 0xdbb));
	printf("acphy: 0x7be 0x%04X\n", phy_utils_read_phyreg(pi, 0x7be));
	printf("acphy: 0x9be 0x%04X\n", phy_utils_read_phyreg(pi, 0x9be));
	printf("acphy: 0xbbe 0x%04X\n", phy_utils_read_phyreg(pi, 0xbbe));
	printf("acphy: 0xdbe 0x%04X\n", phy_utils_read_phyreg(pi, 0xdbe));
	printf("acphy: 0x16ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x16ba));
	printf("acphy: 0x18ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x18ba));
	printf("acphy: 0x1aba 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aba));
	printf("acphy: 0x1cba 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cba));
	printf("acphy: 0x1f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f3));
	printf("acphy: 0x3cf 0x%04X\n", phy_utils_read_phyreg(pi, 0x3cf));
	printf("acphy: 0x3d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d0));
	printf("acphy: 0x3c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c6));
	printf("acphy: 0x3c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c4));
	printf("acphy: 0x3c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c5));
	printf("acphy: 0x044 0x%04X\n", phy_utils_read_phyreg(pi, 0x044));
	printf("acphy: 0x4d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x4d6));
	printf("acphy: 0x688 0x%04X\n", phy_utils_read_phyreg(pi, 0x688));
	printf("acphy: 0x888 0x%04X\n", phy_utils_read_phyreg(pi, 0x888));
	printf("acphy: 0xa88 0x%04X\n", phy_utils_read_phyreg(pi, 0xa88));
	printf("acphy: 0xc88 0x%04X\n", phy_utils_read_phyreg(pi, 0xc88));
	printf("acphy: 0x689 0x%04X\n", phy_utils_read_phyreg(pi, 0x689));
	printf("acphy: 0x889 0x%04X\n", phy_utils_read_phyreg(pi, 0x889));
	printf("acphy: 0xa89 0x%04X\n", phy_utils_read_phyreg(pi, 0xa89));
	printf("acphy: 0xc89 0x%04X\n", phy_utils_read_phyreg(pi, 0xc89));
	printf("acphy: 0x68a 0x%04X\n", phy_utils_read_phyreg(pi, 0x68a));
	printf("acphy: 0x88a 0x%04X\n", phy_utils_read_phyreg(pi, 0x88a));
	printf("acphy: 0xa8a 0x%04X\n", phy_utils_read_phyreg(pi, 0xa8a));
	printf("acphy: 0xc8a 0x%04X\n", phy_utils_read_phyreg(pi, 0xc8a));
	printf("acphy: 0x34a 0x%04X\n", phy_utils_read_phyreg(pi, 0x34a));
	printf("acphy: 0x1688 0x%04X\n", phy_utils_read_phyreg(pi, 0x1688));
	printf("acphy: 0x1888 0x%04X\n", phy_utils_read_phyreg(pi, 0x1888));
	printf("acphy: 0x1a88 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a88));
	printf("acphy: 0x1c88 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c88));
	printf("acphy: 0x1689 0x%04X\n", phy_utils_read_phyreg(pi, 0x1689));
	printf("acphy: 0x1889 0x%04X\n", phy_utils_read_phyreg(pi, 0x1889));
	printf("acphy: 0x1a89 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a89));
	printf("acphy: 0x1c89 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c89));
	printf("acphy: 0x1690 0x%04X\n", phy_utils_read_phyreg(pi, 0x1690));
	printf("acphy: 0x1890 0x%04X\n", phy_utils_read_phyreg(pi, 0x1890));
	printf("acphy: 0x1a90 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a90));
	printf("acphy: 0x1c90 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c90));
	printf("acphy: 0x1691 0x%04X\n", phy_utils_read_phyreg(pi, 0x1691));
	printf("acphy: 0x1891 0x%04X\n", phy_utils_read_phyreg(pi, 0x1891));
	printf("acphy: 0x1a91 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a91));
	printf("acphy: 0x1c91 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c91));
	printf("acphy: 0x168c 0x%04X\n", phy_utils_read_phyreg(pi, 0x168c));
	printf("acphy: 0x188c 0x%04X\n", phy_utils_read_phyreg(pi, 0x188c));
	printf("acphy: 0x1a8c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a8c));
	printf("acphy: 0x1c8c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c8c));
	printf("acphy: 0x168d 0x%04X\n", phy_utils_read_phyreg(pi, 0x168d));
	printf("acphy: 0x188d 0x%04X\n", phy_utils_read_phyreg(pi, 0x188d));
	printf("acphy: 0x1a8d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a8d));
	printf("acphy: 0x1c8d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c8d));
	printf("acphy: 0x1694 0x%04X\n", phy_utils_read_phyreg(pi, 0x1694));
	printf("acphy: 0x1894 0x%04X\n", phy_utils_read_phyreg(pi, 0x1894));
	printf("acphy: 0x1a94 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a94));
	printf("acphy: 0x1c94 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c94));
	printf("acphy: 0x1695 0x%04X\n", phy_utils_read_phyreg(pi, 0x1695));
	printf("acphy: 0x1895 0x%04X\n", phy_utils_read_phyreg(pi, 0x1895));
	printf("acphy: 0x1a95 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a95));
	printf("acphy: 0x1c95 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c95));
	printf("acphy: 0x168a 0x%04X\n", phy_utils_read_phyreg(pi, 0x168a));
	printf("acphy: 0x188a 0x%04X\n", phy_utils_read_phyreg(pi, 0x188a));
	printf("acphy: 0x1a8a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a8a));
	printf("acphy: 0x1c8a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c8a));
	printf("acphy: 0x168b 0x%04X\n", phy_utils_read_phyreg(pi, 0x168b));
	printf("acphy: 0x188b 0x%04X\n", phy_utils_read_phyreg(pi, 0x188b));
	printf("acphy: 0x1a8b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a8b));
	printf("acphy: 0x1c8b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c8b));
	printf("acphy: 0x1692 0x%04X\n", phy_utils_read_phyreg(pi, 0x1692));
	printf("acphy: 0x1892 0x%04X\n", phy_utils_read_phyreg(pi, 0x1892));
	printf("acphy: 0x1a92 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a92));
	printf("acphy: 0x1c92 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c92));
	printf("acphy: 0x1693 0x%04X\n", phy_utils_read_phyreg(pi, 0x1693));
	printf("acphy: 0x1893 0x%04X\n", phy_utils_read_phyreg(pi, 0x1893));
	printf("acphy: 0x1a93 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a93));
	printf("acphy: 0x1c93 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c93));
	printf("acphy: 0x168e 0x%04X\n", phy_utils_read_phyreg(pi, 0x168e));
	printf("acphy: 0x188e 0x%04X\n", phy_utils_read_phyreg(pi, 0x188e));
	printf("acphy: 0x1a8e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a8e));
	printf("acphy: 0x1c8e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c8e));
	printf("acphy: 0x168f 0x%04X\n", phy_utils_read_phyreg(pi, 0x168f));
	printf("acphy: 0x188f 0x%04X\n", phy_utils_read_phyreg(pi, 0x188f));
	printf("acphy: 0x1a8f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a8f));
	printf("acphy: 0x1c8f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c8f));
	printf("acphy: 0x1696 0x%04X\n", phy_utils_read_phyreg(pi, 0x1696));
	printf("acphy: 0x1896 0x%04X\n", phy_utils_read_phyreg(pi, 0x1896));
	printf("acphy: 0x1a96 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a96));
	printf("acphy: 0x1c96 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c96));
	printf("acphy: 0x1697 0x%04X\n", phy_utils_read_phyreg(pi, 0x1697));
	printf("acphy: 0x1897 0x%04X\n", phy_utils_read_phyreg(pi, 0x1897));
	printf("acphy: 0x1a97 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a97));
	printf("acphy: 0x1c97 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c97));
	printf("acphy: 0x339 0x%04X\n", phy_utils_read_phyreg(pi, 0x339));
	printf("acphy: 0x07c 0x%04X\n", phy_utils_read_phyreg(pi, 0x07c));
	printf("acphy: 0x1ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ee));
	printf("acphy: 0x1ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ec));
	printf("acphy: 0x163 0x%04X\n", phy_utils_read_phyreg(pi, 0x163));
	printf("acphy: 0x67a 0x%04X\n", phy_utils_read_phyreg(pi, 0x67a));
	printf("acphy: 0x87a 0x%04X\n", phy_utils_read_phyreg(pi, 0x87a));
	printf("acphy: 0xa7a 0x%04X\n", phy_utils_read_phyreg(pi, 0xa7a));
	printf("acphy: 0xc7a 0x%04X\n", phy_utils_read_phyreg(pi, 0xc7a));
	printf("acphy: 0x67b 0x%04X\n", phy_utils_read_phyreg(pi, 0x67b));
	printf("acphy: 0x87b 0x%04X\n", phy_utils_read_phyreg(pi, 0x87b));
	printf("acphy: 0xa7b 0x%04X\n", phy_utils_read_phyreg(pi, 0xa7b));
	printf("acphy: 0xc7b 0x%04X\n", phy_utils_read_phyreg(pi, 0xc7b));
	printf("acphy: 0x679 0x%04X\n", phy_utils_read_phyreg(pi, 0x679));
	printf("acphy: 0x879 0x%04X\n", phy_utils_read_phyreg(pi, 0x879));
	printf("acphy: 0xa79 0x%04X\n", phy_utils_read_phyreg(pi, 0xa79));
	printf("acphy: 0xc79 0x%04X\n", phy_utils_read_phyreg(pi, 0xc79));
	printf("acphy: 0x642 0x%04X\n", phy_utils_read_phyreg(pi, 0x642));
	printf("acphy: 0x842 0x%04X\n", phy_utils_read_phyreg(pi, 0x842));
	printf("acphy: 0xa42 0x%04X\n", phy_utils_read_phyreg(pi, 0xa42));
	printf("acphy: 0xc42 0x%04X\n", phy_utils_read_phyreg(pi, 0xc42));
	printf("acphy: 0x170a 0x%04X\n", phy_utils_read_phyreg(pi, 0x170a));
	printf("acphy: 0x190a 0x%04X\n", phy_utils_read_phyreg(pi, 0x190a));
	printf("acphy: 0x1b0a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b0a));
	printf("acphy: 0x1d0a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d0a));
	printf("acphy: 0x170b 0x%04X\n", phy_utils_read_phyreg(pi, 0x170b));
	printf("acphy: 0x190b 0x%04X\n", phy_utils_read_phyreg(pi, 0x190b));
	printf("acphy: 0x1b0b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b0b));
	printf("acphy: 0x1d0b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d0b));
	printf("acphy: 0x500 0x%04X\n", phy_utils_read_phyreg(pi, 0x500));
	printf("acphy: 0x502 0x%04X\n", phy_utils_read_phyreg(pi, 0x502));
	printf("acphy: 0x510 0x%04X\n", phy_utils_read_phyreg(pi, 0x510));
	printf("acphy: 0x501 0x%04X\n", phy_utils_read_phyreg(pi, 0x501));
	printf("acphy: 0x1af 0x%04X\n", phy_utils_read_phyreg(pi, 0x1af));
	printf("acphy: 0x5e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e0));
	printf("acphy: 0x5ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ef));
	printf("acphy: 0x1980 0x%04X\n", phy_utils_read_phyreg(pi, 0x1980));
	printf("acphy: 0x1981 0x%04X\n", phy_utils_read_phyreg(pi, 0x1981));
	printf("acphy: 0x1982 0x%04X\n", phy_utils_read_phyreg(pi, 0x1982));
	printf("acphy: 0x198b 0x%04X\n", phy_utils_read_phyreg(pi, 0x198b));
	printf("acphy: 0x198c 0x%04X\n", phy_utils_read_phyreg(pi, 0x198c));
	printf("acphy: 0x198d 0x%04X\n", phy_utils_read_phyreg(pi, 0x198d));
	printf("acphy: 0x1983 0x%04X\n", phy_utils_read_phyreg(pi, 0x1983));
	printf("acphy: 0x1984 0x%04X\n", phy_utils_read_phyreg(pi, 0x1984));
	printf("acphy: 0x1985 0x%04X\n", phy_utils_read_phyreg(pi, 0x1985));
	printf("acphy: 0x1986 0x%04X\n", phy_utils_read_phyreg(pi, 0x1986));
	printf("acphy: 0x1987 0x%04X\n", phy_utils_read_phyreg(pi, 0x1987));
	printf("acphy: 0x1988 0x%04X\n", phy_utils_read_phyreg(pi, 0x1988));
	printf("acphy: 0x1989 0x%04X\n", phy_utils_read_phyreg(pi, 0x1989));
	printf("acphy: 0x198a 0x%04X\n", phy_utils_read_phyreg(pi, 0x198a));
	printf("acphy: 0x5e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e8));
	printf("acphy: 0x5e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e9));
	printf("acphy: 0x5ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ea));
	printf("acphy: 0x5eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x5eb));
	printf("acphy: 0x5ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ec));
	printf("acphy: 0x5ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ed));
	printf("acphy: 0x5ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x5ee));
	printf("acphy: 0x5e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e7));
	printf("acphy: 0x5e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e6));
	printf("acphy: 0x5e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e5));
	printf("acphy: 0x5e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e3));
	printf("acphy: 0x5e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e4));
	printf("acphy: 0x240 0x%04X\n", phy_utils_read_phyreg(pi, 0x240));
	printf("acphy: 0x241 0x%04X\n", phy_utils_read_phyreg(pi, 0x241));
	printf("acphy: 0x24a 0x%04X\n", phy_utils_read_phyreg(pi, 0x24a));
	printf("acphy: 0x242 0x%04X\n", phy_utils_read_phyreg(pi, 0x242));
	printf("acphy: 0x243 0x%04X\n", phy_utils_read_phyreg(pi, 0x243));
	printf("acphy: 0x244 0x%04X\n", phy_utils_read_phyreg(pi, 0x244));
	printf("acphy: 0x245 0x%04X\n", phy_utils_read_phyreg(pi, 0x245));
	printf("acphy: 0x246 0x%04X\n", phy_utils_read_phyreg(pi, 0x246));
	printf("acphy: 0x247 0x%04X\n", phy_utils_read_phyreg(pi, 0x247));
	printf("acphy: 0x248 0x%04X\n", phy_utils_read_phyreg(pi, 0x248));
	printf("acphy: 0x249 0x%04X\n", phy_utils_read_phyreg(pi, 0x249));
	printf("acphy: 0x234 0x%04X\n", phy_utils_read_phyreg(pi, 0x234));
	printf("acphy: 0x235 0x%04X\n", phy_utils_read_phyreg(pi, 0x235));
	printf("acphy: 0x236 0x%04X\n", phy_utils_read_phyreg(pi, 0x236));
	printf("acphy: 0x237 0x%04X\n", phy_utils_read_phyreg(pi, 0x237));
	printf("acphy: 0x230 0x%04X\n", phy_utils_read_phyreg(pi, 0x230));
	printf("acphy: 0x231 0x%04X\n", phy_utils_read_phyreg(pi, 0x231));
	printf("acphy: 0x232 0x%04X\n", phy_utils_read_phyreg(pi, 0x232));
	printf("acphy: 0x233 0x%04X\n", phy_utils_read_phyreg(pi, 0x233));
	printf("acphy: 0x212 0x%04X\n", phy_utils_read_phyreg(pi, 0x212));
	printf("acphy: 0x219 0x%04X\n", phy_utils_read_phyreg(pi, 0x219));
	printf("acphy: 0x218 0x%04X\n", phy_utils_read_phyreg(pi, 0x218));
	printf("acphy: 0x238 0x%04X\n", phy_utils_read_phyreg(pi, 0x238));
	printf("acphy: 0x239 0x%04X\n", phy_utils_read_phyreg(pi, 0x239));
	printf("acphy: 0x067 0x%04X\n", phy_utils_read_phyreg(pi, 0x067));
	printf("acphy: 0x066 0x%04X\n", phy_utils_read_phyreg(pi, 0x066));
	printf("acphy: 0x419 0x%04X\n", phy_utils_read_phyreg(pi, 0x419));
	printf("acphy: 0x01e 0x%04X\n", phy_utils_read_phyreg(pi, 0x01e));
	printf("acphy: 0x01d 0x%04X\n", phy_utils_read_phyreg(pi, 0x01d));
	printf("acphy: 0x74b 0x%04X\n", phy_utils_read_phyreg(pi, 0x74b));
	printf("acphy: 0x94b 0x%04X\n", phy_utils_read_phyreg(pi, 0x94b));
	printf("acphy: 0xb4b 0x%04X\n", phy_utils_read_phyreg(pi, 0xb4b));
	printf("acphy: 0xd4b 0x%04X\n", phy_utils_read_phyreg(pi, 0xd4b));
	printf("acphy: 0x128f 0x%04X\n", phy_utils_read_phyreg(pi, 0x128f));
	printf("acphy: 0x177 0x%04X\n", phy_utils_read_phyreg(pi, 0x177));
	printf("acphy: 0x16b 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b));
	printf("acphy: 0x3e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e0));
	printf("acphy: 0x16d 0x%04X\n", phy_utils_read_phyreg(pi, 0x16d));
	printf("acphy: 0x16c 0x%04X\n", phy_utils_read_phyreg(pi, 0x16c));
	printf("acphy: 0x169 0x%04X\n", phy_utils_read_phyreg(pi, 0x169));
	printf("acphy: 0x259 0x%04X\n", phy_utils_read_phyreg(pi, 0x259));
	printf("acphy: 0x1169 0x%04X\n", phy_utils_read_phyreg(pi, 0x1169));
	printf("acphy: 0x690 0x%04X\n", phy_utils_read_phyreg(pi, 0x690));
	printf("acphy: 0x890 0x%04X\n", phy_utils_read_phyreg(pi, 0x890));
	printf("acphy: 0xa90 0x%04X\n", phy_utils_read_phyreg(pi, 0xa90));
	printf("acphy: 0xc90 0x%04X\n", phy_utils_read_phyreg(pi, 0xc90));
	printf("acphy: 0x29f 0x%04X\n", phy_utils_read_phyreg(pi, 0x29f));
	printf("acphy: 0x280 0x%04X\n", phy_utils_read_phyreg(pi, 0x280));
	printf("acphy: 0x281 0x%04X\n", phy_utils_read_phyreg(pi, 0x281));
	printf("acphy: 0x282 0x%04X\n", phy_utils_read_phyreg(pi, 0x282));
	printf("acphy: 0x283 0x%04X\n", phy_utils_read_phyreg(pi, 0x283));
	printf("acphy: 0x284 0x%04X\n", phy_utils_read_phyreg(pi, 0x284));
	printf("acphy: 0x285 0x%04X\n", phy_utils_read_phyreg(pi, 0x285));
	printf("acphy: 0x286 0x%04X\n", phy_utils_read_phyreg(pi, 0x286));
	printf("acphy: 0x287 0x%04X\n", phy_utils_read_phyreg(pi, 0x287));
	printf("acphy: 0x288 0x%04X\n", phy_utils_read_phyreg(pi, 0x288));
	printf("acphy: 0x46b 0x%04X\n", phy_utils_read_phyreg(pi, 0x46b));
	printf("acphy: 0x365 0x%04X\n", phy_utils_read_phyreg(pi, 0x365));
	printf("acphy: 0x2e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e5));
	printf("acphy: 0x2e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e6));
	printf("acphy: 0x2e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e4));
	printf("acphy: 0x2e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e2));
	printf("acphy: 0x2e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e1));
	printf("acphy: 0x69a 0x%04X\n", phy_utils_read_phyreg(pi, 0x69a));
	printf("acphy: 0x89a 0x%04X\n", phy_utils_read_phyreg(pi, 0x89a));
	printf("acphy: 0xa9a 0x%04X\n", phy_utils_read_phyreg(pi, 0xa9a));
	printf("acphy: 0xc9a 0x%04X\n", phy_utils_read_phyreg(pi, 0xc9a));
	printf("acphy: 0x793 0x%04X\n", phy_utils_read_phyreg(pi, 0x793));
	printf("acphy: 0x993 0x%04X\n", phy_utils_read_phyreg(pi, 0x993));
	printf("acphy: 0xb93 0x%04X\n", phy_utils_read_phyreg(pi, 0xb93));
	printf("acphy: 0xd93 0x%04X\n", phy_utils_read_phyreg(pi, 0xd93));
	printf("acphy: 0x792 0x%04X\n", phy_utils_read_phyreg(pi, 0x792));
	printf("acphy: 0x992 0x%04X\n", phy_utils_read_phyreg(pi, 0x992));
	printf("acphy: 0xb92 0x%04X\n", phy_utils_read_phyreg(pi, 0xb92));
	printf("acphy: 0xd92 0x%04X\n", phy_utils_read_phyreg(pi, 0xd92));
	printf("acphy: 0x3e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e3));
	printf("acphy: 0x5d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d2));
	printf("acphy: 0x5d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d3));
	printf("acphy: 0x5d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d7));
	printf("acphy: 0x5d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d8));
	printf("acphy: 0x5d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d9));
	printf("acphy: 0x5d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d0));
	printf("acphy: 0x10d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d0));
	printf("acphy: 0x10d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d3));
	printf("acphy: 0x10d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d4));
	printf("acphy: 0x10d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d5));
	printf("acphy: 0x10d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d6));
	printf("acphy: 0x10d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d7));
	printf("acphy: 0x10d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d8));
	printf("acphy: 0x10d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d9));
	printf("acphy: 0x10da 0x%04X\n", phy_utils_read_phyreg(pi, 0x10da));
	printf("acphy: 0x10db 0x%04X\n", phy_utils_read_phyreg(pi, 0x10db));
	printf("acphy: 0x10dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x10dc));
	printf("acphy: 0x10d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d1));
	printf("acphy: 0x10dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x10dd));
	printf("acphy: 0x10de 0x%04X\n", phy_utils_read_phyreg(pi, 0x10de));
	printf("acphy: 0x10df 0x%04X\n", phy_utils_read_phyreg(pi, 0x10df));
	printf("acphy: 0x10d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x10d2));
	printf("acphy: 0x10c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c4));
	printf("acphy: 0x10c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c5));
	printf("acphy: 0x473 0x%04X\n", phy_utils_read_phyreg(pi, 0x473));
	printf("acphy: 0x472 0x%04X\n", phy_utils_read_phyreg(pi, 0x472));
	printf("acphy: 0x395 0x%04X\n", phy_utils_read_phyreg(pi, 0x395));
	printf("acphy: 0x013 0x%04X\n", phy_utils_read_phyreg(pi, 0x013));
	printf("acphy: 0x393 0x%04X\n", phy_utils_read_phyreg(pi, 0x393));
	printf("acphy: 0x012 0x%04X\n", phy_utils_read_phyreg(pi, 0x012));
	printf("acphy: 0x392 0x%04X\n", phy_utils_read_phyreg(pi, 0x392));
	printf("acphy: 0x394 0x%04X\n", phy_utils_read_phyreg(pi, 0x394));
	printf("acphy: 0x39c 0x%04X\n", phy_utils_read_phyreg(pi, 0x39c));
	printf("acphy: 0x292 0x%04X\n", phy_utils_read_phyreg(pi, 0x292));
	printf("acphy: 0x195 0x%04X\n", phy_utils_read_phyreg(pi, 0x195));
	printf("acphy: 0x196 0x%04X\n", phy_utils_read_phyreg(pi, 0x196));
	printf("acphy: 0x1f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f6));
	printf("acphy: 0x1f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f7));
	printf("acphy: 0x1b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b0));
	printf("acphy: 0x275 0x%04X\n", phy_utils_read_phyreg(pi, 0x275));
	printf("acphy: 0x274 0x%04X\n", phy_utils_read_phyreg(pi, 0x274));
	printf("acphy: 0x273 0x%04X\n", phy_utils_read_phyreg(pi, 0x273));
	printf("acphy: 0x59b 0x%04X\n", phy_utils_read_phyreg(pi, 0x59b));
	printf("acphy: 0x59a 0x%04X\n", phy_utils_read_phyreg(pi, 0x59a));
	printf("acphy: 0x64d 0x%04X\n", phy_utils_read_phyreg(pi, 0x64d));
	printf("acphy: 0x84d 0x%04X\n", phy_utils_read_phyreg(pi, 0x84d));
	printf("acphy: 0xa4d 0x%04X\n", phy_utils_read_phyreg(pi, 0xa4d));
	printf("acphy: 0xc4d 0x%04X\n", phy_utils_read_phyreg(pi, 0xc4d));
	printf("acphy: 0x64e 0x%04X\n", phy_utils_read_phyreg(pi, 0x64e));
	printf("acphy: 0x84e 0x%04X\n", phy_utils_read_phyreg(pi, 0x84e));
	printf("acphy: 0xa4e 0x%04X\n", phy_utils_read_phyreg(pi, 0xa4e));
	printf("acphy: 0xc4e 0x%04X\n", phy_utils_read_phyreg(pi, 0xc4e));
	printf("acphy: 0x66b 0x%04X\n", phy_utils_read_phyreg(pi, 0x66b));
	printf("acphy: 0x86b 0x%04X\n", phy_utils_read_phyreg(pi, 0x86b));
	printf("acphy: 0xa6b 0x%04X\n", phy_utils_read_phyreg(pi, 0xa6b));
	printf("acphy: 0xc6b 0x%04X\n", phy_utils_read_phyreg(pi, 0xc6b));
	printf("acphy: 0x1e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e1));
	printf("acphy: 0x1e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e7));
	printf("acphy: 0x103c 0x%04X\n", phy_utils_read_phyreg(pi, 0x103c));
	printf("acphy: 0x103d 0x%04X\n", phy_utils_read_phyreg(pi, 0x103d));
	printf("acphy: 0x1034 0x%04X\n", phy_utils_read_phyreg(pi, 0x1034));
	printf("acphy: 0x1ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ae));
	printf("acphy: 0x270 0x%04X\n", phy_utils_read_phyreg(pi, 0x270));
	printf("acphy: 0x6c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c3));
	printf("acphy: 0x8c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c3));
	printf("acphy: 0xac3 0x%04X\n", phy_utils_read_phyreg(pi, 0xac3));
	printf("acphy: 0xcc3 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc3));
	printf("acphy: 0x6c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c2));
	printf("acphy: 0x8c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c2));
	printf("acphy: 0xac2 0x%04X\n", phy_utils_read_phyreg(pi, 0xac2));
	printf("acphy: 0xcc2 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc2));
	printf("acphy: 0x6c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c1));
	printf("acphy: 0x8c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c1));
	printf("acphy: 0xac1 0x%04X\n", phy_utils_read_phyreg(pi, 0xac1));
	printf("acphy: 0xcc1 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc1));
	printf("acphy: 0x6c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c0));
	printf("acphy: 0x8c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c0));
	printf("acphy: 0xac0 0x%04X\n", phy_utils_read_phyreg(pi, 0xac0));
	printf("acphy: 0xcc0 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc0));
	printf("acphy: 0x6c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c5));
	printf("acphy: 0x8c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c5));
	printf("acphy: 0xac5 0x%04X\n", phy_utils_read_phyreg(pi, 0xac5));
	printf("acphy: 0xcc5 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc5));
	printf("acphy: 0x6c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c4));
	printf("acphy: 0x8c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c4));
	printf("acphy: 0xac4 0x%04X\n", phy_utils_read_phyreg(pi, 0xac4));
	printf("acphy: 0xcc4 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc4));
	printf("acphy: 0x272 0x%04X\n", phy_utils_read_phyreg(pi, 0x272));
	printf("acphy: 0x271 0x%04X\n", phy_utils_read_phyreg(pi, 0x271));
	printf("acphy: 0x380 0x%04X\n", phy_utils_read_phyreg(pi, 0x380));
	printf("acphy: 0x382 0x%04X\n", phy_utils_read_phyreg(pi, 0x382));
	printf("acphy: 0x381 0x%04X\n", phy_utils_read_phyreg(pi, 0x381));
	printf("acphy: 0x383 0x%04X\n", phy_utils_read_phyreg(pi, 0x383));
	printf("acphy: 0x3b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b0));
	printf("acphy: 0x3af 0x%04X\n", phy_utils_read_phyreg(pi, 0x3af));
	printf("acphy: 0x3b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b1));
	printf("acphy: 0x3b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b2));
	printf("acphy: 0x3b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b5));
	printf("acphy: 0x3e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e9));
	printf("acphy: 0x46c 0x%04X\n", phy_utils_read_phyreg(pi, 0x46c));
	printf("acphy: 0x46d 0x%04X\n", phy_utils_read_phyreg(pi, 0x46d));
	printf("acphy: 0x3b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b9));
	printf("acphy: 0x1a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a0));
	printf("acphy: 0x1a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a2));
	printf("acphy: 0x1a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a1));
	printf("acphy: 0x1a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a3));
	printf("acphy: 0x108e 0x%04X\n", phy_utils_read_phyreg(pi, 0x108e));
	printf("acphy: 0x2b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b3));
	printf("acphy: 0x2b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b1));
	printf("acphy: 0x2b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b0));
	printf("acphy: 0x2bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x2bd));
	printf("acphy: 0x2ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ba));
	printf("acphy: 0x2b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b9));
	printf("acphy: 0x2b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b8));
	printf("acphy: 0x2b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b5));
	printf("acphy: 0x2b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b4));
	printf("acphy: 0x2b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b7));
	printf("acphy: 0x2b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b6));
	printf("acphy: 0x2bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x2bc));
	printf("acphy: 0x2bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x2bb));
	printf("acphy: 0x2be 0x%04X\n", phy_utils_read_phyreg(pi, 0x2be));
	printf("acphy: 0x2b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x2b2));
	printf("acphy: 0x1140 0x%04X\n", phy_utils_read_phyreg(pi, 0x1140));
	printf("acphy: 0x1240 0x%04X\n", phy_utils_read_phyreg(pi, 0x1240));
	printf("acphy: 0x1248 0x%04X\n", phy_utils_read_phyreg(pi, 0x1248));
	printf("acphy: 0x1144 0x%04X\n", phy_utils_read_phyreg(pi, 0x1144));
	printf("acphy: 0x1145 0x%04X\n", phy_utils_read_phyreg(pi, 0x1145));
	printf("acphy: 0x1148 0x%04X\n", phy_utils_read_phyreg(pi, 0x1148));
	printf("acphy: 0x1149 0x%04X\n", phy_utils_read_phyreg(pi, 0x1149));
	printf("acphy: 0x1146 0x%04X\n", phy_utils_read_phyreg(pi, 0x1146));
	printf("acphy: 0x1147 0x%04X\n", phy_utils_read_phyreg(pi, 0x1147));
	printf("acphy: 0x114a 0x%04X\n", phy_utils_read_phyreg(pi, 0x114a));
	printf("acphy: 0x114b 0x%04X\n", phy_utils_read_phyreg(pi, 0x114b));
	printf("acphy: 0x114d 0x%04X\n", phy_utils_read_phyreg(pi, 0x114d));
	printf("acphy: 0x115c 0x%04X\n", phy_utils_read_phyreg(pi, 0x115c));
	printf("acphy: 0x1151 0x%04X\n", phy_utils_read_phyreg(pi, 0x1151));
	printf("acphy: 0x1150 0x%04X\n", phy_utils_read_phyreg(pi, 0x1150));
	printf("acphy: 0x114c 0x%04X\n", phy_utils_read_phyreg(pi, 0x114c));
	printf("acphy: 0x1142 0x%04X\n", phy_utils_read_phyreg(pi, 0x1142));
	printf("acphy: 0x1242 0x%04X\n", phy_utils_read_phyreg(pi, 0x1242));
	printf("acphy: 0x114f 0x%04X\n", phy_utils_read_phyreg(pi, 0x114f));
	printf("acphy: 0x114e 0x%04X\n", phy_utils_read_phyreg(pi, 0x114e));
	printf("acphy: 0x1143 0x%04X\n", phy_utils_read_phyreg(pi, 0x1143));
	printf("acphy: 0x1243 0x%04X\n", phy_utils_read_phyreg(pi, 0x1243));
	printf("acphy: 0x1141 0x%04X\n", phy_utils_read_phyreg(pi, 0x1141));
	printf("acphy: 0x1241 0x%04X\n", phy_utils_read_phyreg(pi, 0x1241));
	printf("acphy: 0x1643 0x%04X\n", phy_utils_read_phyreg(pi, 0x1643));
	printf("acphy: 0x1843 0x%04X\n", phy_utils_read_phyreg(pi, 0x1843));
	printf("acphy: 0x1a43 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a43));
	printf("acphy: 0x1c43 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c43));
	printf("acphy: 0x1644 0x%04X\n", phy_utils_read_phyreg(pi, 0x1644));
	printf("acphy: 0x1844 0x%04X\n", phy_utils_read_phyreg(pi, 0x1844));
	printf("acphy: 0x1a44 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a44));
	printf("acphy: 0x1c44 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c44));
	printf("acphy: 0x1152 0x%04X\n", phy_utils_read_phyreg(pi, 0x1152));
	printf("acphy: 0x1153 0x%04X\n", phy_utils_read_phyreg(pi, 0x1153));
	printf("acphy: 0x1154 0x%04X\n", phy_utils_read_phyreg(pi, 0x1154));
	printf("acphy: 0x1156 0x%04X\n", phy_utils_read_phyreg(pi, 0x1156));
	printf("acphy: 0x1157 0x%04X\n", phy_utils_read_phyreg(pi, 0x1157));
	printf("acphy: 0x1158 0x%04X\n", phy_utils_read_phyreg(pi, 0x1158));
	printf("acphy: 0x1159 0x%04X\n", phy_utils_read_phyreg(pi, 0x1159));
	printf("acphy: 0x1259 0x%04X\n", phy_utils_read_phyreg(pi, 0x1259));
	printf("acphy: 0x125a 0x%04X\n", phy_utils_read_phyreg(pi, 0x125a));
	printf("acphy: 0x125b 0x%04X\n", phy_utils_read_phyreg(pi, 0x125b));
	printf("acphy: 0x115a 0x%04X\n", phy_utils_read_phyreg(pi, 0x115a));
	printf("acphy: 0x115b 0x%04X\n", phy_utils_read_phyreg(pi, 0x115b));
	printf("acphy: 0x1645 0x%04X\n", phy_utils_read_phyreg(pi, 0x1645));
	printf("acphy: 0x1845 0x%04X\n", phy_utils_read_phyreg(pi, 0x1845));
	printf("acphy: 0x1a45 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a45));
	printf("acphy: 0x1c45 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c45));
	printf("acphy: 0x1155 0x%04X\n", phy_utils_read_phyreg(pi, 0x1155));
	printf("acphy: 0x1249 0x%04X\n", phy_utils_read_phyreg(pi, 0x1249));
	printf("acphy: 0x124a 0x%04X\n", phy_utils_read_phyreg(pi, 0x124a));
	printf("acphy: 0x115d 0x%04X\n", phy_utils_read_phyreg(pi, 0x115d));
	printf("acphy: 0x115e 0x%04X\n", phy_utils_read_phyreg(pi, 0x115e));
	printf("acphy: 0x115f 0x%04X\n", phy_utils_read_phyreg(pi, 0x115f));
	printf("acphy: 0x1640 0x%04X\n", phy_utils_read_phyreg(pi, 0x1640));
	printf("acphy: 0x1840 0x%04X\n", phy_utils_read_phyreg(pi, 0x1840));
	printf("acphy: 0x1a40 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a40));
	printf("acphy: 0x1c40 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c40));
	printf("acphy: 0x124b 0x%04X\n", phy_utils_read_phyreg(pi, 0x124b));
	printf("acphy: 0x124c 0x%04X\n", phy_utils_read_phyreg(pi, 0x124c));
	printf("acphy: 0x3e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e2));
	printf("acphy: 0x3ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ea));
	printf("acphy: 0x3eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x3eb));
	printf("acphy: 0x3e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e1));
	printf("acphy: 0x3e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e5));
	printf("acphy: 0x1b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b7));
	printf("acphy: 0x3b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b4));
	printf("acphy: 0x3dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x3dc));
	printf("acphy: 0x3dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x3dd));
	printf("acphy: 0x3e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e7));
	printf("acphy: 0x3e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e8));
	printf("acphy: 0x1280 0x%04X\n", phy_utils_read_phyreg(pi, 0x1280));
	printf("acphy: 0x1283 0x%04X\n", phy_utils_read_phyreg(pi, 0x1283));
	printf("acphy: 0x1284 0x%04X\n", phy_utils_read_phyreg(pi, 0x1284));
	printf("acphy: 0x1285 0x%04X\n", phy_utils_read_phyreg(pi, 0x1285));
	printf("acphy: 0x1286 0x%04X\n", phy_utils_read_phyreg(pi, 0x1286));
	printf("acphy: 0x1281 0x%04X\n", phy_utils_read_phyreg(pi, 0x1281));
	printf("acphy: 0x1282 0x%04X\n", phy_utils_read_phyreg(pi, 0x1282));
	printf("acphy: 0x128d 0x%04X\n", phy_utils_read_phyreg(pi, 0x128d));
	printf("acphy: 0x40c 0x%04X\n", phy_utils_read_phyreg(pi, 0x40c));
	printf("acphy: 0x40d 0x%04X\n", phy_utils_read_phyreg(pi, 0x40d));
	printf("acphy: 0x74f 0x%04X\n", phy_utils_read_phyreg(pi, 0x74f));
	printf("acphy: 0x94f 0x%04X\n", phy_utils_read_phyreg(pi, 0x94f));
	printf("acphy: 0xb4f 0x%04X\n", phy_utils_read_phyreg(pi, 0xb4f));
	printf("acphy: 0xd4f 0x%04X\n", phy_utils_read_phyreg(pi, 0xd4f));
	printf("acphy: 0x3bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x3bb));
	printf("acphy: 0x3ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ba));
	printf("acphy: 0x17c 0x%04X\n", phy_utils_read_phyreg(pi, 0x17c));
	printf("acphy: 0x17d 0x%04X\n", phy_utils_read_phyreg(pi, 0x17d));
	printf("acphy: 0x1642 0x%04X\n", phy_utils_read_phyreg(pi, 0x1642));
	printf("acphy: 0x1842 0x%04X\n", phy_utils_read_phyreg(pi, 0x1842));
	printf("acphy: 0x1a42 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a42));
	printf("acphy: 0x1c42 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c42));
	printf("acphy: 0x1641 0x%04X\n", phy_utils_read_phyreg(pi, 0x1641));
	printf("acphy: 0x1841 0x%04X\n", phy_utils_read_phyreg(pi, 0x1841));
	printf("acphy: 0x1a41 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a41));
	printf("acphy: 0x1c41 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c41));
	printf("acphy: 0x377 0x%04X\n", phy_utils_read_phyreg(pi, 0x377));
	printf("acphy: 0x1632 0x%04X\n", phy_utils_read_phyreg(pi, 0x1632));
	printf("acphy: 0x1832 0x%04X\n", phy_utils_read_phyreg(pi, 0x1832));
	printf("acphy: 0x1a32 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a32));
	printf("acphy: 0x1653 0x%04X\n", phy_utils_read_phyreg(pi, 0x1653));
	printf("acphy: 0x1853 0x%04X\n", phy_utils_read_phyreg(pi, 0x1853));
	printf("acphy: 0x1a53 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a53));
	printf("acphy: 0x1c53 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c53));
	printf("acphy: 0x1c32 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c32));
	printf("acphy: 0x1655 0x%04X\n", phy_utils_read_phyreg(pi, 0x1655));
	printf("acphy: 0x1855 0x%04X\n", phy_utils_read_phyreg(pi, 0x1855));
	printf("acphy: 0x1a55 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a55));
	printf("acphy: 0x1c55 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c55));
	printf("acphy: 0x1656 0x%04X\n", phy_utils_read_phyreg(pi, 0x1656));
	printf("acphy: 0x1856 0x%04X\n", phy_utils_read_phyreg(pi, 0x1856));
	printf("acphy: 0x1a56 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a56));
	printf("acphy: 0x1c56 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c56));
	printf("acphy: 0x1654 0x%04X\n", phy_utils_read_phyreg(pi, 0x1654));
	printf("acphy: 0x1854 0x%04X\n", phy_utils_read_phyreg(pi, 0x1854));
	printf("acphy: 0x1a54 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a54));
	printf("acphy: 0x1c54 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c54));
	printf("acphy: 0x78f 0x%04X\n", phy_utils_read_phyreg(pi, 0x78f));
	printf("acphy: 0x98f 0x%04X\n", phy_utils_read_phyreg(pi, 0x98f));
	printf("acphy: 0xb8f 0x%04X\n", phy_utils_read_phyreg(pi, 0xb8f));
	printf("acphy: 0xd8f 0x%04X\n", phy_utils_read_phyreg(pi, 0xd8f));
	printf("acphy: 0x78e 0x%04X\n", phy_utils_read_phyreg(pi, 0x78e));
	printf("acphy: 0x98e 0x%04X\n", phy_utils_read_phyreg(pi, 0x98e));
	printf("acphy: 0xb8e 0x%04X\n", phy_utils_read_phyreg(pi, 0xb8e));
	printf("acphy: 0xd8e 0x%04X\n", phy_utils_read_phyreg(pi, 0xd8e));
	printf("acphy: 0x28d 0x%04X\n", phy_utils_read_phyreg(pi, 0x28d));
	printf("acphy: 0x28f 0x%04X\n", phy_utils_read_phyreg(pi, 0x28f));
	printf("acphy: 0x291 0x%04X\n", phy_utils_read_phyreg(pi, 0x291));
	printf("acphy: 0x28c 0x%04X\n", phy_utils_read_phyreg(pi, 0x28c));
	printf("acphy: 0x28e 0x%04X\n", phy_utils_read_phyreg(pi, 0x28e));
	printf("acphy: 0x290 0x%04X\n", phy_utils_read_phyreg(pi, 0x290));
	printf("acphy: 0x104e 0x%04X\n", phy_utils_read_phyreg(pi, 0x104e));
	printf("acphy: 0x1050 0x%04X\n", phy_utils_read_phyreg(pi, 0x1050));
	printf("acphy: 0x109c 0x%04X\n", phy_utils_read_phyreg(pi, 0x109c));
	printf("acphy: 0x109d 0x%04X\n", phy_utils_read_phyreg(pi, 0x109d));
	printf("acphy: 0x109e 0x%04X\n", phy_utils_read_phyreg(pi, 0x109e));
	printf("acphy: 0x109f 0x%04X\n", phy_utils_read_phyreg(pi, 0x109f));
	printf("acphy: 0x104f 0x%04X\n", phy_utils_read_phyreg(pi, 0x104f));
	printf("acphy: 0x104c 0x%04X\n", phy_utils_read_phyreg(pi, 0x104c));
	printf("acphy: 0x104d 0x%04X\n", phy_utils_read_phyreg(pi, 0x104d));
	printf("acphy: 0x471 0x%04X\n", phy_utils_read_phyreg(pi, 0x471));
	printf("acphy: 0x1094 0x%04X\n", phy_utils_read_phyreg(pi, 0x1094));
	printf("acphy: 0x1095 0x%04X\n", phy_utils_read_phyreg(pi, 0x1095));
	printf("acphy: 0x1096 0x%04X\n", phy_utils_read_phyreg(pi, 0x1096));
	printf("acphy: 0x1097 0x%04X\n", phy_utils_read_phyreg(pi, 0x1097));
	printf("acphy: 0x1098 0x%04X\n", phy_utils_read_phyreg(pi, 0x1098));
	printf("acphy: 0x1099 0x%04X\n", phy_utils_read_phyreg(pi, 0x1099));
	printf("acphy: 0x109a 0x%04X\n", phy_utils_read_phyreg(pi, 0x109a));
	printf("acphy: 0x109b 0x%04X\n", phy_utils_read_phyreg(pi, 0x109b));
	printf("acphy: 0x49d 0x%04X\n", phy_utils_read_phyreg(pi, 0x49d));
	printf("acphy: 0x4f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x4f5));
	printf("acphy: 0x4f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x4f6));
	printf("acphy: 0x4f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x4f4));
	printf("acphy: 0x002 0x%04X\n", phy_utils_read_phyreg(pi, 0x002));
	printf("acphy: 0x1ce 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ce));
	printf("acphy: 0x1ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ef));
	printf("acphy: 0x1bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x1bb));
	printf("acphy: 0x1ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ba));
	printf("acphy: 0x1b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b9));
	printf("acphy: 0x2d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d1));
	printf("acphy: 0x112b 0x%04X\n", phy_utils_read_phyreg(pi, 0x112b));
	printf("acphy: 0x570 0x%04X\n", phy_utils_read_phyreg(pi, 0x570));
	printf("acphy: 0x01c 0x%04X\n", phy_utils_read_phyreg(pi, 0x01c));
	printf("acphy: 0x1260 0x%04X\n", phy_utils_read_phyreg(pi, 0x1260));
	printf("acphy: 0x1261 0x%04X\n", phy_utils_read_phyreg(pi, 0x1261));
	printf("acphy: 0x1262 0x%04X\n", phy_utils_read_phyreg(pi, 0x1262));
	printf("acphy: 0x1264 0x%04X\n", phy_utils_read_phyreg(pi, 0x1264));
	printf("acphy: 0x550 0x%04X\n", phy_utils_read_phyreg(pi, 0x550));
	printf("acphy: 0x551 0x%04X\n", phy_utils_read_phyreg(pi, 0x551));
	printf("acphy: 0x552 0x%04X\n", phy_utils_read_phyreg(pi, 0x552));
	printf("acphy: 0x553 0x%04X\n", phy_utils_read_phyreg(pi, 0x553));
	printf("acphy: 0x554 0x%04X\n", phy_utils_read_phyreg(pi, 0x554));
	printf("acphy: 0x555 0x%04X\n", phy_utils_read_phyreg(pi, 0x555));
	printf("acphy: 0x556 0x%04X\n", phy_utils_read_phyreg(pi, 0x556));
	printf("acphy: 0x557 0x%04X\n", phy_utils_read_phyreg(pi, 0x557));
	printf("acphy: 0x558 0x%04X\n", phy_utils_read_phyreg(pi, 0x558));
	printf("acphy: 0x559 0x%04X\n", phy_utils_read_phyreg(pi, 0x559));
	printf("acphy: 0x55a 0x%04X\n", phy_utils_read_phyreg(pi, 0x55a));
	printf("acphy: 0x55b 0x%04X\n", phy_utils_read_phyreg(pi, 0x55b));
	printf("acphy: 0x2dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x2dc));
	printf("acphy: 0x609 0x%04X\n", phy_utils_read_phyreg(pi, 0x609));
	printf("acphy: 0x809 0x%04X\n", phy_utils_read_phyreg(pi, 0x809));
	printf("acphy: 0xa09 0x%04X\n", phy_utils_read_phyreg(pi, 0xa09));
	printf("acphy: 0xc09 0x%04X\n", phy_utils_read_phyreg(pi, 0xc09));
	printf("acphy: 0x1039 0x%04X\n", phy_utils_read_phyreg(pi, 0x1039));
	printf("acphy: 0x103a 0x%04X\n", phy_utils_read_phyreg(pi, 0x103a));
	printf("acphy: 0x103b 0x%04X\n", phy_utils_read_phyreg(pi, 0x103b));
	printf("acphy: 0x112a 0x%04X\n", phy_utils_read_phyreg(pi, 0x112a));
	printf("acphy: 0x2d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d9));
	printf("acphy: 0x2da 0x%04X\n", phy_utils_read_phyreg(pi, 0x2da));
	printf("acphy: 0x2d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d8));
	printf("acphy: 0x2db 0x%04X\n", phy_utils_read_phyreg(pi, 0x2db));
	printf("acphy: 0x1128 0x%04X\n", phy_utils_read_phyreg(pi, 0x1128));
	printf("acphy: 0x1129 0x%04X\n", phy_utils_read_phyreg(pi, 0x1129));
	printf("acphy: 0x583 0x%04X\n", phy_utils_read_phyreg(pi, 0x583));
	printf("acphy: 0x2f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x2f1));
	printf("acphy: 0x2f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x2f0));
	printf("acphy: 0x586 0x%04X\n", phy_utils_read_phyreg(pi, 0x586));
	printf("acphy: 0x588 0x%04X\n", phy_utils_read_phyreg(pi, 0x588));
	printf("acphy: 0x58a 0x%04X\n", phy_utils_read_phyreg(pi, 0x58a));
	printf("acphy: 0x58c 0x%04X\n", phy_utils_read_phyreg(pi, 0x58c));
	printf("acphy: 0x58e 0x%04X\n", phy_utils_read_phyreg(pi, 0x58e));
	printf("acphy: 0x585 0x%04X\n", phy_utils_read_phyreg(pi, 0x585));
	printf("acphy: 0x587 0x%04X\n", phy_utils_read_phyreg(pi, 0x587));
	printf("acphy: 0x589 0x%04X\n", phy_utils_read_phyreg(pi, 0x589));
	printf("acphy: 0x58b 0x%04X\n", phy_utils_read_phyreg(pi, 0x58b));
	printf("acphy: 0x58d 0x%04X\n", phy_utils_read_phyreg(pi, 0x58d));
	printf("acphy: 0x580 0x%04X\n", phy_utils_read_phyreg(pi, 0x580));
	printf("acphy: 0x591 0x%04X\n", phy_utils_read_phyreg(pi, 0x591));
	printf("acphy: 0x2ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ef));
	printf("acphy: 0x581 0x%04X\n", phy_utils_read_phyreg(pi, 0x581));
	printf("acphy: 0x584 0x%04X\n", phy_utils_read_phyreg(pi, 0x584));
	printf("acphy: 0x582 0x%04X\n", phy_utils_read_phyreg(pi, 0x582));
	printf("acphy: 0x523 0x%04X\n", phy_utils_read_phyreg(pi, 0x523));
	printf("acphy: 0x522 0x%04X\n", phy_utils_read_phyreg(pi, 0x522));
	printf("acphy: 0x521 0x%04X\n", phy_utils_read_phyreg(pi, 0x521));
	printf("acphy: 0x1df 0x%04X\n", phy_utils_read_phyreg(pi, 0x1df));
	printf("acphy: 0x112d 0x%04X\n", phy_utils_read_phyreg(pi, 0x112d));
	printf("acphy: 0x112e 0x%04X\n", phy_utils_read_phyreg(pi, 0x112e));
	printf("acphy: 0x112f 0x%04X\n", phy_utils_read_phyreg(pi, 0x112f));
	printf("acphy: 0x2cc 0x%04X\n", phy_utils_read_phyreg(pi, 0x2cc));
	printf("acphy: 0x2cd 0x%04X\n", phy_utils_read_phyreg(pi, 0x2cd));
	printf("acphy: 0x2ce 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ce));
	printf("acphy: 0x2cf 0x%04X\n", phy_utils_read_phyreg(pi, 0x2cf));
	printf("acphy: 0x171 0x%04X\n", phy_utils_read_phyreg(pi, 0x171));
	printf("acphy: 0x29d 0x%04X\n", phy_utils_read_phyreg(pi, 0x29d));
	printf("acphy: 0x29a 0x%04X\n", phy_utils_read_phyreg(pi, 0x29a));
	printf("acphy: 0x041 0x%04X\n", phy_utils_read_phyreg(pi, 0x041));
	printf("acphy: 0x042 0x%04X\n", phy_utils_read_phyreg(pi, 0x042));
	printf("acphy: 0x1b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b2));
	printf("acphy: 0x1b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b3));
	printf("acphy: 0x1b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b5));
	printf("acphy: 0x110e 0x%04X\n", phy_utils_read_phyreg(pi, 0x110e));
	printf("acphy: 0x1100 0x%04X\n", phy_utils_read_phyreg(pi, 0x1100));
	printf("acphy: 0x1101 0x%04X\n", phy_utils_read_phyreg(pi, 0x1101));
	printf("acphy: 0x1102 0x%04X\n", phy_utils_read_phyreg(pi, 0x1102));
	printf("acphy: 0x110b 0x%04X\n", phy_utils_read_phyreg(pi, 0x110b));
	printf("acphy: 0x110c 0x%04X\n", phy_utils_read_phyreg(pi, 0x110c));
	printf("acphy: 0x110d 0x%04X\n", phy_utils_read_phyreg(pi, 0x110d));
	printf("acphy: 0x110f 0x%04X\n", phy_utils_read_phyreg(pi, 0x110f));
	printf("acphy: 0x1103 0x%04X\n", phy_utils_read_phyreg(pi, 0x1103));
	printf("acphy: 0x1104 0x%04X\n", phy_utils_read_phyreg(pi, 0x1104));
	printf("acphy: 0x1105 0x%04X\n", phy_utils_read_phyreg(pi, 0x1105));
	printf("acphy: 0x1106 0x%04X\n", phy_utils_read_phyreg(pi, 0x1106));
	printf("acphy: 0x1107 0x%04X\n", phy_utils_read_phyreg(pi, 0x1107));
	printf("acphy: 0x1108 0x%04X\n", phy_utils_read_phyreg(pi, 0x1108));
	printf("acphy: 0x1109 0x%04X\n", phy_utils_read_phyreg(pi, 0x1109));
	printf("acphy: 0x110a 0x%04X\n", phy_utils_read_phyreg(pi, 0x110a));
	printf("acphy: 0x1650 0x%04X\n", phy_utils_read_phyreg(pi, 0x1650));
	printf("acphy: 0x1850 0x%04X\n", phy_utils_read_phyreg(pi, 0x1850));
	printf("acphy: 0x1a50 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a50));
	printf("acphy: 0x1c50 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c50));
	printf("acphy: 0x1651 0x%04X\n", phy_utils_read_phyreg(pi, 0x1651));
	printf("acphy: 0x1851 0x%04X\n", phy_utils_read_phyreg(pi, 0x1851));
	printf("acphy: 0x1a51 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a51));
	printf("acphy: 0x1c51 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c51));
	printf("acphy: 0x130 0x%04X\n", phy_utils_read_phyreg(pi, 0x130));
	printf("acphy: 0x131 0x%04X\n", phy_utils_read_phyreg(pi, 0x131));
	printf("acphy: 0x132 0x%04X\n", phy_utils_read_phyreg(pi, 0x132));
	printf("acphy: 0x133 0x%04X\n", phy_utils_read_phyreg(pi, 0x133));
	printf("acphy: 0x134 0x%04X\n", phy_utils_read_phyreg(pi, 0x134));
	printf("acphy: 0x135 0x%04X\n", phy_utils_read_phyreg(pi, 0x135));
	printf("acphy: 0x136 0x%04X\n", phy_utils_read_phyreg(pi, 0x136));
	printf("acphy: 0x137 0x%04X\n", phy_utils_read_phyreg(pi, 0x137));
	printf("acphy: 0x138 0x%04X\n", phy_utils_read_phyreg(pi, 0x138));
	printf("acphy: 0x139 0x%04X\n", phy_utils_read_phyreg(pi, 0x139));
	printf("acphy: 0x767 0x%04X\n", phy_utils_read_phyreg(pi, 0x767));
	printf("acphy: 0x967 0x%04X\n", phy_utils_read_phyreg(pi, 0x967));
	printf("acphy: 0xb67 0x%04X\n", phy_utils_read_phyreg(pi, 0xb67));
	printf("acphy: 0xd67 0x%04X\n", phy_utils_read_phyreg(pi, 0xd67));
	printf("acphy: 0x769 0x%04X\n", phy_utils_read_phyreg(pi, 0x769));
	printf("acphy: 0x969 0x%04X\n", phy_utils_read_phyreg(pi, 0x969));
	printf("acphy: 0xb69 0x%04X\n", phy_utils_read_phyreg(pi, 0xb69));
	printf("acphy: 0xd69 0x%04X\n", phy_utils_read_phyreg(pi, 0xd69));
	printf("acphy: 0x490 0x%04X\n", phy_utils_read_phyreg(pi, 0x490));
	printf("acphy: 0x4b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x4b8));
	printf("acphy: 0x48e 0x%04X\n", phy_utils_read_phyreg(pi, 0x48e));
	printf("acphy: 0x481 0x%04X\n", phy_utils_read_phyreg(pi, 0x481));
	printf("acphy: 0x483 0x%04X\n", phy_utils_read_phyreg(pi, 0x483));
	printf("acphy: 0x4f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x4f8));
	printf("acphy: 0x10b 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b));
	printf("acphy: 0x16f 0x%04X\n", phy_utils_read_phyreg(pi, 0x16f));
	printf("acphy: 0x3de 0x%04X\n", phy_utils_read_phyreg(pi, 0x3de));
	printf("acphy: 0x3d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d5));
	printf("acphy: 0x297 0x%04X\n", phy_utils_read_phyreg(pi, 0x297));
	printf("acphy: 0x298 0x%04X\n", phy_utils_read_phyreg(pi, 0x298));
	printf("acphy: 0x66a 0x%04X\n", phy_utils_read_phyreg(pi, 0x66a));
	printf("acphy: 0x86a 0x%04X\n", phy_utils_read_phyreg(pi, 0x86a));
	printf("acphy: 0xa6a 0x%04X\n", phy_utils_read_phyreg(pi, 0xa6a));
	printf("acphy: 0xc6a 0x%04X\n", phy_utils_read_phyreg(pi, 0xc6a));
	printf("acphy: 0x120 0x%04X\n", phy_utils_read_phyreg(pi, 0x120));
	printf("acphy: 0x4a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x4a0));
	printf("acphy: 0x129 0x%04X\n", phy_utils_read_phyreg(pi, 0x129));
	printf("acphy: 0x125 0x%04X\n", phy_utils_read_phyreg(pi, 0x125));
	printf("acphy: 0x123 0x%04X\n", phy_utils_read_phyreg(pi, 0x123));
	printf("acphy: 0x670 0x%04X\n", phy_utils_read_phyreg(pi, 0x670));
	printf("acphy: 0x870 0x%04X\n", phy_utils_read_phyreg(pi, 0x870));
	printf("acphy: 0xa70 0x%04X\n", phy_utils_read_phyreg(pi, 0xa70));
	printf("acphy: 0xc70 0x%04X\n", phy_utils_read_phyreg(pi, 0xc70));
	printf("acphy: 0x671 0x%04X\n", phy_utils_read_phyreg(pi, 0x671));
	printf("acphy: 0x871 0x%04X\n", phy_utils_read_phyreg(pi, 0x871));
	printf("acphy: 0xa71 0x%04X\n", phy_utils_read_phyreg(pi, 0xa71));
	printf("acphy: 0xc71 0x%04X\n", phy_utils_read_phyreg(pi, 0xc71));
	printf("acphy: 0x672 0x%04X\n", phy_utils_read_phyreg(pi, 0x672));
	printf("acphy: 0x872 0x%04X\n", phy_utils_read_phyreg(pi, 0x872));
	printf("acphy: 0xa72 0x%04X\n", phy_utils_read_phyreg(pi, 0xa72));
	printf("acphy: 0xc72 0x%04X\n", phy_utils_read_phyreg(pi, 0xc72));
	printf("acphy: 0x673 0x%04X\n", phy_utils_read_phyreg(pi, 0x673));
	printf("acphy: 0x873 0x%04X\n", phy_utils_read_phyreg(pi, 0x873));
	printf("acphy: 0xa73 0x%04X\n", phy_utils_read_phyreg(pi, 0xa73));
	printf("acphy: 0xc73 0x%04X\n", phy_utils_read_phyreg(pi, 0xc73));
	printf("acphy: 0x122 0x%04X\n", phy_utils_read_phyreg(pi, 0x122));
	printf("acphy: 0x67c 0x%04X\n", phy_utils_read_phyreg(pi, 0x67c));
	printf("acphy: 0x87c 0x%04X\n", phy_utils_read_phyreg(pi, 0x87c));
	printf("acphy: 0xa7c 0x%04X\n", phy_utils_read_phyreg(pi, 0xa7c));
	printf("acphy: 0xc7c 0x%04X\n", phy_utils_read_phyreg(pi, 0xc7c));
	printf("acphy: 0x124 0x%04X\n", phy_utils_read_phyreg(pi, 0x124));
	printf("acphy: 0x12a 0x%04X\n", phy_utils_read_phyreg(pi, 0x12a));
	printf("acphy: 0x674 0x%04X\n", phy_utils_read_phyreg(pi, 0x674));
	printf("acphy: 0x874 0x%04X\n", phy_utils_read_phyreg(pi, 0x874));
	printf("acphy: 0xa74 0x%04X\n", phy_utils_read_phyreg(pi, 0xa74));
	printf("acphy: 0xc74 0x%04X\n", phy_utils_read_phyreg(pi, 0xc74));
	printf("acphy: 0x675 0x%04X\n", phy_utils_read_phyreg(pi, 0x675));
	printf("acphy: 0x875 0x%04X\n", phy_utils_read_phyreg(pi, 0x875));
	printf("acphy: 0xa75 0x%04X\n", phy_utils_read_phyreg(pi, 0xa75));
	printf("acphy: 0xc75 0x%04X\n", phy_utils_read_phyreg(pi, 0xc75));
	printf("acphy: 0x121 0x%04X\n", phy_utils_read_phyreg(pi, 0x121));
	printf("acphy: 0x676 0x%04X\n", phy_utils_read_phyreg(pi, 0x676));
	printf("acphy: 0x876 0x%04X\n", phy_utils_read_phyreg(pi, 0x876));
	printf("acphy: 0xa76 0x%04X\n", phy_utils_read_phyreg(pi, 0xa76));
	printf("acphy: 0xc76 0x%04X\n", phy_utils_read_phyreg(pi, 0xc76));
	printf("acphy: 0x677 0x%04X\n", phy_utils_read_phyreg(pi, 0x677));
	printf("acphy: 0x877 0x%04X\n", phy_utils_read_phyreg(pi, 0x877));
	printf("acphy: 0xa77 0x%04X\n", phy_utils_read_phyreg(pi, 0xa77));
	printf("acphy: 0xc77 0x%04X\n", phy_utils_read_phyreg(pi, 0xc77));
	printf("acphy: 0x678 0x%04X\n", phy_utils_read_phyreg(pi, 0x678));
	printf("acphy: 0x878 0x%04X\n", phy_utils_read_phyreg(pi, 0x878));
	printf("acphy: 0xa78 0x%04X\n", phy_utils_read_phyreg(pi, 0xa78));
	printf("acphy: 0xc78 0x%04X\n", phy_utils_read_phyreg(pi, 0xc78));
	printf("acphy: 0x1630 0x%04X\n", phy_utils_read_phyreg(pi, 0x1630));
	printf("acphy: 0x1830 0x%04X\n", phy_utils_read_phyreg(pi, 0x1830));
	printf("acphy: 0x1a30 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a30));
	printf("acphy: 0x1c30 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c30));
	printf("acphy: 0x126 0x%04X\n", phy_utils_read_phyreg(pi, 0x126));
	printf("acphy: 0x127 0x%04X\n", phy_utils_read_phyreg(pi, 0x127));
	printf("acphy: 0x128 0x%04X\n", phy_utils_read_phyreg(pi, 0x128));
	printf("acphy: 0x680 0x%04X\n", phy_utils_read_phyreg(pi, 0x680));
	printf("acphy: 0x880 0x%04X\n", phy_utils_read_phyreg(pi, 0x880));
	printf("acphy: 0xa80 0x%04X\n", phy_utils_read_phyreg(pi, 0xa80));
	printf("acphy: 0xc80 0x%04X\n", phy_utils_read_phyreg(pi, 0xc80));
	printf("acphy: 0x681 0x%04X\n", phy_utils_read_phyreg(pi, 0x681));
	printf("acphy: 0x881 0x%04X\n", phy_utils_read_phyreg(pi, 0x881));
	printf("acphy: 0xa81 0x%04X\n", phy_utils_read_phyreg(pi, 0xa81));
	printf("acphy: 0xc81 0x%04X\n", phy_utils_read_phyreg(pi, 0xc81));
	printf("acphy: 0x682 0x%04X\n", phy_utils_read_phyreg(pi, 0x682));
	printf("acphy: 0x882 0x%04X\n", phy_utils_read_phyreg(pi, 0x882));
	printf("acphy: 0xa82 0x%04X\n", phy_utils_read_phyreg(pi, 0xa82));
	printf("acphy: 0xc82 0x%04X\n", phy_utils_read_phyreg(pi, 0xc82));
	printf("acphy: 0x683 0x%04X\n", phy_utils_read_phyreg(pi, 0x683));
	printf("acphy: 0x883 0x%04X\n", phy_utils_read_phyreg(pi, 0x883));
	printf("acphy: 0xa83 0x%04X\n", phy_utils_read_phyreg(pi, 0xa83));
	printf("acphy: 0xc83 0x%04X\n", phy_utils_read_phyreg(pi, 0xc83));
	printf("acphy: 0x684 0x%04X\n", phy_utils_read_phyreg(pi, 0x684));
	printf("acphy: 0x884 0x%04X\n", phy_utils_read_phyreg(pi, 0x884));
	printf("acphy: 0xa84 0x%04X\n", phy_utils_read_phyreg(pi, 0xa84));
	printf("acphy: 0xc84 0x%04X\n", phy_utils_read_phyreg(pi, 0xc84));
	printf("acphy: 0x67d 0x%04X\n", phy_utils_read_phyreg(pi, 0x67d));
	printf("acphy: 0x87d 0x%04X\n", phy_utils_read_phyreg(pi, 0x87d));
	printf("acphy: 0xa7d 0x%04X\n", phy_utils_read_phyreg(pi, 0xa7d));
	printf("acphy: 0xc7d 0x%04X\n", phy_utils_read_phyreg(pi, 0xc7d));
	printf("acphy: 0x67e 0x%04X\n", phy_utils_read_phyreg(pi, 0x67e));
	printf("acphy: 0x87e 0x%04X\n", phy_utils_read_phyreg(pi, 0x87e));
	printf("acphy: 0xa7e 0x%04X\n", phy_utils_read_phyreg(pi, 0xa7e));
	printf("acphy: 0xc7e 0x%04X\n", phy_utils_read_phyreg(pi, 0xc7e));
	printf("acphy: 0x67f 0x%04X\n", phy_utils_read_phyreg(pi, 0x67f));
	printf("acphy: 0x87f 0x%04X\n", phy_utils_read_phyreg(pi, 0x87f));
	printf("acphy: 0xa7f 0x%04X\n", phy_utils_read_phyreg(pi, 0xa7f));
	printf("acphy: 0xc7f 0x%04X\n", phy_utils_read_phyreg(pi, 0xc7f));
	printf("acphy: 0x080 0x%04X\n", phy_utils_read_phyreg(pi, 0x080));
	printf("acphy: 0x660 0x%04X\n", phy_utils_read_phyreg(pi, 0x660));
	printf("acphy: 0x860 0x%04X\n", phy_utils_read_phyreg(pi, 0x860));
	printf("acphy: 0xa60 0x%04X\n", phy_utils_read_phyreg(pi, 0xa60));
	printf("acphy: 0xc60 0x%04X\n", phy_utils_read_phyreg(pi, 0xc60));
	printf("acphy: 0x662 0x%04X\n", phy_utils_read_phyreg(pi, 0x662));
	printf("acphy: 0x862 0x%04X\n", phy_utils_read_phyreg(pi, 0x862));
	printf("acphy: 0xa62 0x%04X\n", phy_utils_read_phyreg(pi, 0xa62));
	printf("acphy: 0xc62 0x%04X\n", phy_utils_read_phyreg(pi, 0xc62));
	printf("acphy: 0x661 0x%04X\n", phy_utils_read_phyreg(pi, 0x661));
	printf("acphy: 0x861 0x%04X\n", phy_utils_read_phyreg(pi, 0x861));
	printf("acphy: 0xa61 0x%04X\n", phy_utils_read_phyreg(pi, 0xa61));
	printf("acphy: 0xc61 0x%04X\n", phy_utils_read_phyreg(pi, 0xc61));
	printf("acphy: 0x085 0x%04X\n", phy_utils_read_phyreg(pi, 0x085));
	printf("acphy: 0x086 0x%04X\n", phy_utils_read_phyreg(pi, 0x086));
	printf("acphy: 0x08a 0x%04X\n", phy_utils_read_phyreg(pi, 0x08a));
	printf("acphy: 0x08b 0x%04X\n", phy_utils_read_phyreg(pi, 0x08b));
	printf("acphy: 0x082 0x%04X\n", phy_utils_read_phyreg(pi, 0x082));
	printf("acphy: 0x083 0x%04X\n", phy_utils_read_phyreg(pi, 0x083));
	printf("acphy: 0x084 0x%04X\n", phy_utils_read_phyreg(pi, 0x084));
	printf("acphy: 0x087 0x%04X\n", phy_utils_read_phyreg(pi, 0x087));
	printf("acphy: 0x088 0x%04X\n", phy_utils_read_phyreg(pi, 0x088));
	printf("acphy: 0x089 0x%04X\n", phy_utils_read_phyreg(pi, 0x089));
	printf("acphy: 0x081 0x%04X\n", phy_utils_read_phyreg(pi, 0x081));
	printf("acphy: 0x0a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a3));
	printf("acphy: 0x0a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a4));
	printf("acphy: 0x0a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a5));
	printf("acphy: 0x0a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a7));
	printf("acphy: 0x0a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a8));
	printf("acphy: 0x0a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a6));
	printf("acphy: 0x0a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a2));
	printf("acphy: 0x3ac 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ac));
	printf("acphy: 0x3aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x3aa));
	printf("acphy: 0x1b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b1));
	printf("acphy: 0x1cf 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cf));
	printf("acphy: 0x1d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d0));
	printf("acphy: 0x1d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d1));
	printf("acphy: 0x1d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d2));
	printf("acphy: 0x1d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d3));
	printf("acphy: 0x1d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d4));
	printf("acphy: 0x1d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d5));
	printf("acphy: 0x1d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d6));
	printf("acphy: 0x469 0x%04X\n", phy_utils_read_phyreg(pi, 0x469));
	printf("acphy: 0x1ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ed));
	printf("acphy: 0x3e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x3e4));
	printf("acphy: 0x3c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c2));
	printf("acphy: 0x3c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c1));
	printf("acphy: 0x078 0x%04X\n", phy_utils_read_phyreg(pi, 0x078));
	printf("acphy: 0x07a 0x%04X\n", phy_utils_read_phyreg(pi, 0x07a));
	printf("acphy: 0x1139 0x%04X\n", phy_utils_read_phyreg(pi, 0x1139));
	printf("acphy: 0x6b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b7));
	printf("acphy: 0x8b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b7));
	printf("acphy: 0xab7 0x%04X\n", phy_utils_read_phyreg(pi, 0xab7));
	printf("acphy: 0xcb7 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb7));
	printf("acphy: 0x00b 0x%04X\n", phy_utils_read_phyreg(pi, 0x00b));
	printf("acphy: 0x00c 0x%04X\n", phy_utils_read_phyreg(pi, 0x00c));
	printf("acphy: 0x009 0x%04X\n", phy_utils_read_phyreg(pi, 0x009));
	printf("acphy: 0x46e 0x%04X\n", phy_utils_read_phyreg(pi, 0x46e));
	printf("acphy: 0x15d 0x%04X\n", phy_utils_read_phyreg(pi, 0x15d));
	printf("acphy: 0x1c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c7));
	printf("acphy: 0x153 0x%04X\n", phy_utils_read_phyreg(pi, 0x153));
	printf("acphy: 0x1c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c9));
	printf("acphy: 0x155 0x%04X\n", phy_utils_read_phyreg(pi, 0x155));
	printf("acphy: 0x152 0x%04X\n", phy_utils_read_phyreg(pi, 0x152));
	printf("acphy: 0x159 0x%04X\n", phy_utils_read_phyreg(pi, 0x159));
	printf("acphy: 0x692 0x%04X\n", phy_utils_read_phyreg(pi, 0x692));
	printf("acphy: 0x892 0x%04X\n", phy_utils_read_phyreg(pi, 0x892));
	printf("acphy: 0xa92 0x%04X\n", phy_utils_read_phyreg(pi, 0xa92));
	printf("acphy: 0xc92 0x%04X\n", phy_utils_read_phyreg(pi, 0xc92));
	printf("acphy: 0x1c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c4));
	printf("acphy: 0x1c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c5));
	printf("acphy: 0x1c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c6));
	printf("acphy: 0x2cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x2cb));
	printf("acphy: 0x154 0x%04X\n", phy_utils_read_phyreg(pi, 0x154));
	printf("acphy: 0x1c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c8));
	printf("acphy: 0x156 0x%04X\n", phy_utils_read_phyreg(pi, 0x156));
	printf("acphy: 0x405 0x%04X\n", phy_utils_read_phyreg(pi, 0x405));
	printf("acphy: 0x168 0x%04X\n", phy_utils_read_phyreg(pi, 0x168));
	printf("acphy: 0x1043 0x%04X\n", phy_utils_read_phyreg(pi, 0x1043));
	printf("acphy: 0x1040 0x%04X\n", phy_utils_read_phyreg(pi, 0x1040));
	printf("acphy: 0x1042 0x%04X\n", phy_utils_read_phyreg(pi, 0x1042));
	printf("acphy: 0x1041 0x%04X\n", phy_utils_read_phyreg(pi, 0x1041));
	printf("acphy: 0x1e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e6));
	printf("acphy: 0x1066 0x%04X\n", phy_utils_read_phyreg(pi, 0x1066));
	printf("acphy: 0x1067 0x%04X\n", phy_utils_read_phyreg(pi, 0x1067));
	printf("acphy: 0x203 0x%04X\n", phy_utils_read_phyreg(pi, 0x203));
	printf("acphy: 0x204 0x%04X\n", phy_utils_read_phyreg(pi, 0x204));
	printf("acphy: 0x205 0x%04X\n", phy_utils_read_phyreg(pi, 0x205));
	printf("acphy: 0x206 0x%04X\n", phy_utils_read_phyreg(pi, 0x206));
	printf("acphy: 0x207 0x%04X\n", phy_utils_read_phyreg(pi, 0x207));
	printf("acphy: 0x208 0x%04X\n", phy_utils_read_phyreg(pi, 0x208));
	printf("acphy: 0x209 0x%04X\n", phy_utils_read_phyreg(pi, 0x209));
	printf("acphy: 0x20a 0x%04X\n", phy_utils_read_phyreg(pi, 0x20a));
	printf("acphy: 0x20b 0x%04X\n", phy_utils_read_phyreg(pi, 0x20b));
	printf("acphy: 0x20c 0x%04X\n", phy_utils_read_phyreg(pi, 0x20c));
	printf("acphy: 0x1f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f0));
	printf("acphy: 0x1f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f1));
	printf("acphy: 0x366 0x%04X\n", phy_utils_read_phyreg(pi, 0x366));
	printf("acphy: 0x367 0x%04X\n", phy_utils_read_phyreg(pi, 0x367));
	printf("acphy: 0x1e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e0));
	printf("acphy: 0x1068 0x%04X\n", phy_utils_read_phyreg(pi, 0x1068));
	printf("acphy: 0x799 0x%04X\n", phy_utils_read_phyreg(pi, 0x799));
	printf("acphy: 0x999 0x%04X\n", phy_utils_read_phyreg(pi, 0x999));
	printf("acphy: 0xb99 0x%04X\n", phy_utils_read_phyreg(pi, 0xb99));
	printf("acphy: 0xd99 0x%04X\n", phy_utils_read_phyreg(pi, 0xd99));
	printf("acphy: 0x798 0x%04X\n", phy_utils_read_phyreg(pi, 0x798));
	printf("acphy: 0x998 0x%04X\n", phy_utils_read_phyreg(pi, 0x998));
	printf("acphy: 0xb98 0x%04X\n", phy_utils_read_phyreg(pi, 0xb98));
	printf("acphy: 0xd98 0x%04X\n", phy_utils_read_phyreg(pi, 0xd98));
	printf("acphy: 0x3ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ed));
	printf("acphy: 0x415 0x%04X\n", phy_utils_read_phyreg(pi, 0x415));
	printf("acphy: 0x74a 0x%04X\n", phy_utils_read_phyreg(pi, 0x74a));
	printf("acphy: 0x94a 0x%04X\n", phy_utils_read_phyreg(pi, 0x94a));
	printf("acphy: 0xb4a 0x%04X\n", phy_utils_read_phyreg(pi, 0xb4a));
	printf("acphy: 0xd4a 0x%04X\n", phy_utils_read_phyreg(pi, 0xd4a));
	printf("acphy: 0x3ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ee));
	printf("acphy: 0x3da 0x%04X\n", phy_utils_read_phyreg(pi, 0x3da));
	printf("acphy: 0x3db 0x%04X\n", phy_utils_read_phyreg(pi, 0x3db));
	printf("acphy: 0x3ce 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ce));
	printf("acphy: 0x7d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d3));
	printf("acphy: 0x9d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d3));
	printf("acphy: 0xbd3 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd3));
	printf("acphy: 0xdd3 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd3));
	printf("acphy: 0x7d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d7));
	printf("acphy: 0x9d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d7));
	printf("acphy: 0xbd7 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd7));
	printf("acphy: 0xdd7 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd7));
	printf("acphy: 0x7d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d9));
	printf("acphy: 0x9d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d9));
	printf("acphy: 0xbd9 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd9));
	printf("acphy: 0xdd9 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd9));
	printf("acphy: 0x7db 0x%04X\n", phy_utils_read_phyreg(pi, 0x7db));
	printf("acphy: 0x9db 0x%04X\n", phy_utils_read_phyreg(pi, 0x9db));
	printf("acphy: 0xbdb 0x%04X\n", phy_utils_read_phyreg(pi, 0xbdb));
	printf("acphy: 0xddb 0x%04X\n", phy_utils_read_phyreg(pi, 0xddb));
	printf("acphy: 0x7d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d5));
	printf("acphy: 0x9d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d5));
	printf("acphy: 0xbd5 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd5));
	printf("acphy: 0xdd5 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd5));
	printf("acphy: 0x7d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d1));
	printf("acphy: 0x9d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d1));
	printf("acphy: 0xbd1 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd1));
	printf("acphy: 0xdd1 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd1));
	printf("acphy: 0x1739 0x%04X\n", phy_utils_read_phyreg(pi, 0x1739));
	printf("acphy: 0x1939 0x%04X\n", phy_utils_read_phyreg(pi, 0x1939));
	printf("acphy: 0x1b39 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b39));
	printf("acphy: 0x1d39 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d39));
	printf("acphy: 0x7d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d2));
	printf("acphy: 0x9d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d2));
	printf("acphy: 0xbd2 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd2));
	printf("acphy: 0xdd2 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd2));
	printf("acphy: 0x7d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d6));
	printf("acphy: 0x9d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d6));
	printf("acphy: 0xbd6 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd6));
	printf("acphy: 0xdd6 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd6));
	printf("acphy: 0x7d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d8));
	printf("acphy: 0x9d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d8));
	printf("acphy: 0xbd8 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd8));
	printf("acphy: 0xdd8 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd8));
	printf("acphy: 0x7da 0x%04X\n", phy_utils_read_phyreg(pi, 0x7da));
	printf("acphy: 0x9da 0x%04X\n", phy_utils_read_phyreg(pi, 0x9da));
	printf("acphy: 0xbda 0x%04X\n", phy_utils_read_phyreg(pi, 0xbda));
	printf("acphy: 0xdda 0x%04X\n", phy_utils_read_phyreg(pi, 0xdda));
	printf("acphy: 0x7d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d4));
	printf("acphy: 0x9d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d4));
	printf("acphy: 0xbd4 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd4));
	printf("acphy: 0xdd4 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd4));
	printf("acphy: 0x7d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x7d0));
	printf("acphy: 0x9d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x9d0));
	printf("acphy: 0xbd0 0x%04X\n", phy_utils_read_phyreg(pi, 0xbd0));
	printf("acphy: 0xdd0 0x%04X\n", phy_utils_read_phyreg(pi, 0xdd0));
	printf("acphy: 0x1738 0x%04X\n", phy_utils_read_phyreg(pi, 0x1738));
	printf("acphy: 0x1938 0x%04X\n", phy_utils_read_phyreg(pi, 0x1938));
	printf("acphy: 0x1b38 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b38));
	printf("acphy: 0x1d38 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d38));
	printf("acphy: 0x173a 0x%04X\n", phy_utils_read_phyreg(pi, 0x173a));
	printf("acphy: 0x193a 0x%04X\n", phy_utils_read_phyreg(pi, 0x193a));
	printf("acphy: 0x1b3a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b3a));
	printf("acphy: 0x1d3a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d3a));
	printf("acphy: 0x7dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x7dd));
	printf("acphy: 0x9dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x9dd));
	printf("acphy: 0xbdd 0x%04X\n", phy_utils_read_phyreg(pi, 0xbdd));
	printf("acphy: 0xddd 0x%04X\n", phy_utils_read_phyreg(pi, 0xddd));
	printf("acphy: 0x7de 0x%04X\n", phy_utils_read_phyreg(pi, 0x7de));
	printf("acphy: 0x9de 0x%04X\n", phy_utils_read_phyreg(pi, 0x9de));
	printf("acphy: 0xbde 0x%04X\n", phy_utils_read_phyreg(pi, 0xbde));
	printf("acphy: 0xdde 0x%04X\n", phy_utils_read_phyreg(pi, 0xdde));
	printf("acphy: 0x2c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c8));
	printf("acphy: 0x2c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c9));
	printf("acphy: 0x2ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ca));
	printf("acphy: 0x2c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c6));
	printf("acphy: 0x2c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c7));
	printf("acphy: 0x3cd 0x%04X\n", phy_utils_read_phyreg(pi, 0x3cd));
	printf("acphy: 0x78d 0x%04X\n", phy_utils_read_phyreg(pi, 0x78d));
	printf("acphy: 0x98d 0x%04X\n", phy_utils_read_phyreg(pi, 0x98d));
	printf("acphy: 0xb8d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb8d));
	printf("acphy: 0xd8d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd8d));
	printf("acphy: 0x78c 0x%04X\n", phy_utils_read_phyreg(pi, 0x78c));
	printf("acphy: 0x98c 0x%04X\n", phy_utils_read_phyreg(pi, 0x98c));
	printf("acphy: 0xb8c 0x%04X\n", phy_utils_read_phyreg(pi, 0xb8c));
	printf("acphy: 0xd8c 0x%04X\n", phy_utils_read_phyreg(pi, 0xd8c));
	printf("acphy: 0x789 0x%04X\n", phy_utils_read_phyreg(pi, 0x789));
	printf("acphy: 0x989 0x%04X\n", phy_utils_read_phyreg(pi, 0x989));
	printf("acphy: 0xb89 0x%04X\n", phy_utils_read_phyreg(pi, 0xb89));
	printf("acphy: 0xd89 0x%04X\n", phy_utils_read_phyreg(pi, 0xd89));
	printf("acphy: 0x788 0x%04X\n", phy_utils_read_phyreg(pi, 0x788));
	printf("acphy: 0x988 0x%04X\n", phy_utils_read_phyreg(pi, 0x988));
	printf("acphy: 0xb88 0x%04X\n", phy_utils_read_phyreg(pi, 0xb88));
	printf("acphy: 0xd88 0x%04X\n", phy_utils_read_phyreg(pi, 0xd88));
	printf("acphy: 0x78b 0x%04X\n", phy_utils_read_phyreg(pi, 0x78b));
	printf("acphy: 0x98b 0x%04X\n", phy_utils_read_phyreg(pi, 0x98b));
	printf("acphy: 0xb8b 0x%04X\n", phy_utils_read_phyreg(pi, 0xb8b));
	printf("acphy: 0xd8b 0x%04X\n", phy_utils_read_phyreg(pi, 0xd8b));
	printf("acphy: 0x78a 0x%04X\n", phy_utils_read_phyreg(pi, 0x78a));
	printf("acphy: 0x98a 0x%04X\n", phy_utils_read_phyreg(pi, 0x98a));
	printf("acphy: 0xb8a 0x%04X\n", phy_utils_read_phyreg(pi, 0xb8a));
	printf("acphy: 0xd8a 0x%04X\n", phy_utils_read_phyreg(pi, 0xd8a));
	printf("acphy: 0x3df 0x%04X\n", phy_utils_read_phyreg(pi, 0x3df));
	printf("acphy: 0x6b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b0));
	printf("acphy: 0x8b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b0));
	printf("acphy: 0xab0 0x%04X\n", phy_utils_read_phyreg(pi, 0xab0));
	printf("acphy: 0xcb0 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb0));
	printf("acphy: 0x25e 0x%04X\n", phy_utils_read_phyreg(pi, 0x25e));
	printf("acphy: 0x11d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x11d0));
	printf("acphy: 0x116e 0x%04X\n", phy_utils_read_phyreg(pi, 0x116e));
	printf("acphy: 0x25d 0x%04X\n", phy_utils_read_phyreg(pi, 0x25d));
	printf("acphy: 0x116d 0x%04X\n", phy_utils_read_phyreg(pi, 0x116d));
	printf("acphy: 0x250 0x%04X\n", phy_utils_read_phyreg(pi, 0x250));
	printf("acphy: 0x25f 0x%04X\n", phy_utils_read_phyreg(pi, 0x25f));
	printf("acphy: 0x116f 0x%04X\n", phy_utils_read_phyreg(pi, 0x116f));
	printf("acphy: 0x11d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x11d3));
	printf("acphy: 0x11d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x11d4));
	printf("acphy: 0x1160 0x%04X\n", phy_utils_read_phyreg(pi, 0x1160));
	printf("acphy: 0x260 0x%04X\n", phy_utils_read_phyreg(pi, 0x260));
	printf("acphy: 0x1170 0x%04X\n", phy_utils_read_phyreg(pi, 0x1170));
	printf("acphy: 0x264 0x%04X\n", phy_utils_read_phyreg(pi, 0x264));
	printf("acphy: 0x1174 0x%04X\n", phy_utils_read_phyreg(pi, 0x1174));
	printf("acphy: 0x26e 0x%04X\n", phy_utils_read_phyreg(pi, 0x26e));
	printf("acphy: 0x11d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x11d2));
	printf("acphy: 0x117e 0x%04X\n", phy_utils_read_phyreg(pi, 0x117e));
	printf("acphy: 0x265 0x%04X\n", phy_utils_read_phyreg(pi, 0x265));
	printf("acphy: 0x1175 0x%04X\n", phy_utils_read_phyreg(pi, 0x1175));
	printf("acphy: 0x26f 0x%04X\n", phy_utils_read_phyreg(pi, 0x26f));
	printf("acphy: 0x117f 0x%04X\n", phy_utils_read_phyreg(pi, 0x117f));
	printf("acphy: 0x25b 0x%04X\n", phy_utils_read_phyreg(pi, 0x25b));
	printf("acphy: 0x116b 0x%04X\n", phy_utils_read_phyreg(pi, 0x116b));
	printf("acphy: 0x25a 0x%04X\n", phy_utils_read_phyreg(pi, 0x25a));
	printf("acphy: 0x116a 0x%04X\n", phy_utils_read_phyreg(pi, 0x116a));
	printf("acphy: 0x268 0x%04X\n", phy_utils_read_phyreg(pi, 0x268));
	printf("acphy: 0x1178 0x%04X\n", phy_utils_read_phyreg(pi, 0x1178));
	printf("acphy: 0x263 0x%04X\n", phy_utils_read_phyreg(pi, 0x263));
	printf("acphy: 0x1173 0x%04X\n", phy_utils_read_phyreg(pi, 0x1173));
	printf("acphy: 0x25c 0x%04X\n", phy_utils_read_phyreg(pi, 0x25c));
	printf("acphy: 0x116c 0x%04X\n", phy_utils_read_phyreg(pi, 0x116c));
	printf("acphy: 0x11d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x11d5));
	printf("acphy: 0x11d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x11d6));
	printf("acphy: 0x269 0x%04X\n", phy_utils_read_phyreg(pi, 0x269));
	printf("acphy: 0x1179 0x%04X\n", phy_utils_read_phyreg(pi, 0x1179));
	printf("acphy: 0x26a 0x%04X\n", phy_utils_read_phyreg(pi, 0x26a));
	printf("acphy: 0x11d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x11d1));
	printf("acphy: 0x117a 0x%04X\n", phy_utils_read_phyreg(pi, 0x117a));
	printf("acphy: 0x26c 0x%04X\n", phy_utils_read_phyreg(pi, 0x26c));
	printf("acphy: 0x117c 0x%04X\n", phy_utils_read_phyreg(pi, 0x117c));
	printf("acphy: 0x26d 0x%04X\n", phy_utils_read_phyreg(pi, 0x26d));
	printf("acphy: 0x117d 0x%04X\n", phy_utils_read_phyreg(pi, 0x117d));
	printf("acphy: 0x261 0x%04X\n", phy_utils_read_phyreg(pi, 0x261));
	printf("acphy: 0x1171 0x%04X\n", phy_utils_read_phyreg(pi, 0x1171));
	printf("acphy: 0x262 0x%04X\n", phy_utils_read_phyreg(pi, 0x262));
	printf("acphy: 0x1172 0x%04X\n", phy_utils_read_phyreg(pi, 0x1172));
	printf("acphy: 0x255 0x%04X\n", phy_utils_read_phyreg(pi, 0x255));
	printf("acphy: 0x266 0x%04X\n", phy_utils_read_phyreg(pi, 0x266));
	printf("acphy: 0x1165 0x%04X\n", phy_utils_read_phyreg(pi, 0x1165));
	printf("acphy: 0x257 0x%04X\n", phy_utils_read_phyreg(pi, 0x257));
	printf("acphy: 0x1167 0x%04X\n", phy_utils_read_phyreg(pi, 0x1167));
	printf("acphy: 0x256 0x%04X\n", phy_utils_read_phyreg(pi, 0x256));
	printf("acphy: 0x267 0x%04X\n", phy_utils_read_phyreg(pi, 0x267));
	printf("acphy: 0x1166 0x%04X\n", phy_utils_read_phyreg(pi, 0x1166));
	printf("acphy: 0x258 0x%04X\n", phy_utils_read_phyreg(pi, 0x258));
	printf("acphy: 0x1168 0x%04X\n", phy_utils_read_phyreg(pi, 0x1168));
	printf("acphy: 0x019 0x%04X\n", phy_utils_read_phyreg(pi, 0x019));
	printf("acphy: 0x018 0x%04X\n", phy_utils_read_phyreg(pi, 0x018));
	printf("acphy: 0x01a 0x%04X\n", phy_utils_read_phyreg(pi, 0x01a));
	printf("acphy: 0x1020 0x%04X\n", phy_utils_read_phyreg(pi, 0x1020));
	printf("acphy: 0x1000 0x%04X\n", phy_utils_read_phyreg(pi, 0x1000));
	printf("acphy: 0x1001 0x%04X\n", phy_utils_read_phyreg(pi, 0x1001));
	printf("acphy: 0x1002 0x%04X\n", phy_utils_read_phyreg(pi, 0x1002));
	printf("acphy: 0x1003 0x%04X\n", phy_utils_read_phyreg(pi, 0x1003));
	printf("acphy: 0x1004 0x%04X\n", phy_utils_read_phyreg(pi, 0x1004));
	printf("acphy: 0x1028 0x%04X\n", phy_utils_read_phyreg(pi, 0x1028));
	printf("acphy: 0x100f 0x%04X\n", phy_utils_read_phyreg(pi, 0x100f));
	printf("acphy: 0x100c 0x%04X\n", phy_utils_read_phyreg(pi, 0x100c));
	printf("acphy: 0x100d 0x%04X\n", phy_utils_read_phyreg(pi, 0x100d));
	printf("acphy: 0x100e 0x%04X\n", phy_utils_read_phyreg(pi, 0x100e));
	printf("acphy: 0x100a 0x%04X\n", phy_utils_read_phyreg(pi, 0x100a));
	printf("acphy: 0x100b 0x%04X\n", phy_utils_read_phyreg(pi, 0x100b));
	printf("acphy: 0x1005 0x%04X\n", phy_utils_read_phyreg(pi, 0x1005));
	printf("acphy: 0x1006 0x%04X\n", phy_utils_read_phyreg(pi, 0x1006));
	printf("acphy: 0x1007 0x%04X\n", phy_utils_read_phyreg(pi, 0x1007));
	printf("acphy: 0x1008 0x%04X\n", phy_utils_read_phyreg(pi, 0x1008));
	printf("acphy: 0x1009 0x%04X\n", phy_utils_read_phyreg(pi, 0x1009));
	printf("acphy: 0x41b 0x%04X\n", phy_utils_read_phyreg(pi, 0x41b));
	printf("acphy: 0x1027 0x%04X\n", phy_utils_read_phyreg(pi, 0x1027));
	printf("acphy: 0x1025 0x%04X\n", phy_utils_read_phyreg(pi, 0x1025));
	printf("acphy: 0x1026 0x%04X\n", phy_utils_read_phyreg(pi, 0x1026));
	printf("acphy: 0x1024 0x%04X\n", phy_utils_read_phyreg(pi, 0x1024));
	printf("acphy: 0x1021 0x%04X\n", phy_utils_read_phyreg(pi, 0x1021));
	printf("acphy: 0x1022 0x%04X\n", phy_utils_read_phyreg(pi, 0x1022));
	printf("acphy: 0x1023 0x%04X\n", phy_utils_read_phyreg(pi, 0x1023));
	printf("acphy: 0x413 0x%04X\n", phy_utils_read_phyreg(pi, 0x413));
	printf("acphy: 0x40b 0x%04X\n", phy_utils_read_phyreg(pi, 0x40b));
	printf("acphy: 0x409 0x%04X\n", phy_utils_read_phyreg(pi, 0x409));
	printf("acphy: 0x748 0x%04X\n", phy_utils_read_phyreg(pi, 0x748));
	printf("acphy: 0x948 0x%04X\n", phy_utils_read_phyreg(pi, 0x948));
	printf("acphy: 0xb48 0x%04X\n", phy_utils_read_phyreg(pi, 0xb48));
	printf("acphy: 0xd48 0x%04X\n", phy_utils_read_phyreg(pi, 0xd48));
	printf("acphy: 0x408 0x%04X\n", phy_utils_read_phyreg(pi, 0x408));
	printf("acphy: 0x741 0x%04X\n", phy_utils_read_phyreg(pi, 0x741));
	printf("acphy: 0x742 0x%04X\n", phy_utils_read_phyreg(pi, 0x742));
	printf("acphy: 0x743 0x%04X\n", phy_utils_read_phyreg(pi, 0x743));
	printf("acphy: 0x744 0x%04X\n", phy_utils_read_phyreg(pi, 0x744));
	printf("acphy: 0x941 0x%04X\n", phy_utils_read_phyreg(pi, 0x941));
	printf("acphy: 0x942 0x%04X\n", phy_utils_read_phyreg(pi, 0x942));
	printf("acphy: 0x943 0x%04X\n", phy_utils_read_phyreg(pi, 0x943));
	printf("acphy: 0x944 0x%04X\n", phy_utils_read_phyreg(pi, 0x944));
	printf("acphy: 0xb41 0x%04X\n", phy_utils_read_phyreg(pi, 0xb41));
	printf("acphy: 0xb42 0x%04X\n", phy_utils_read_phyreg(pi, 0xb42));
	printf("acphy: 0xb43 0x%04X\n", phy_utils_read_phyreg(pi, 0xb43));
	printf("acphy: 0xb44 0x%04X\n", phy_utils_read_phyreg(pi, 0xb44));
	printf("acphy: 0xd41 0x%04X\n", phy_utils_read_phyreg(pi, 0xd41));
	printf("acphy: 0xd42 0x%04X\n", phy_utils_read_phyreg(pi, 0xd42));
	printf("acphy: 0xd43 0x%04X\n", phy_utils_read_phyreg(pi, 0xd43));
	printf("acphy: 0xd44 0x%04X\n", phy_utils_read_phyreg(pi, 0xd44));
	printf("acphy: 0x739 0x%04X\n", phy_utils_read_phyreg(pi, 0x739));
	printf("acphy: 0x939 0x%04X\n", phy_utils_read_phyreg(pi, 0x939));
	printf("acphy: 0xb39 0x%04X\n", phy_utils_read_phyreg(pi, 0xb39));
	printf("acphy: 0xd39 0x%04X\n", phy_utils_read_phyreg(pi, 0xd39));
	printf("acphy: 0x73a 0x%04X\n", phy_utils_read_phyreg(pi, 0x73a));
	printf("acphy: 0x93a 0x%04X\n", phy_utils_read_phyreg(pi, 0x93a));
	printf("acphy: 0xb3a 0x%04X\n", phy_utils_read_phyreg(pi, 0xb3a));
	printf("acphy: 0xd3a 0x%04X\n", phy_utils_read_phyreg(pi, 0xd3a));
	printf("acphy: 0x73c 0x%04X\n", phy_utils_read_phyreg(pi, 0x73c));
	printf("acphy: 0x93c 0x%04X\n", phy_utils_read_phyreg(pi, 0x93c));
	printf("acphy: 0xb3c 0x%04X\n", phy_utils_read_phyreg(pi, 0xb3c));
	printf("acphy: 0xd3c 0x%04X\n", phy_utils_read_phyreg(pi, 0xd3c));
	printf("acphy: 0x73d 0x%04X\n", phy_utils_read_phyreg(pi, 0x73d));
	printf("acphy: 0x93d 0x%04X\n", phy_utils_read_phyreg(pi, 0x93d));
	printf("acphy: 0xb3d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb3d));
	printf("acphy: 0xd3d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd3d));
	printf("acphy: 0x417 0x%04X\n", phy_utils_read_phyreg(pi, 0x417));
	printf("acphy: 0x745 0x%04X\n", phy_utils_read_phyreg(pi, 0x745));
	printf("acphy: 0x945 0x%04X\n", phy_utils_read_phyreg(pi, 0x945));
	printf("acphy: 0xb45 0x%04X\n", phy_utils_read_phyreg(pi, 0xb45));
	printf("acphy: 0xd45 0x%04X\n", phy_utils_read_phyreg(pi, 0xd45));
	printf("acphy: 0x1703 0x%04X\n", phy_utils_read_phyreg(pi, 0x1703));
	printf("acphy: 0x1903 0x%04X\n", phy_utils_read_phyreg(pi, 0x1903));
	printf("acphy: 0x1b03 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b03));
	printf("acphy: 0x1d03 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d03));
	printf("acphy: 0x73b 0x%04X\n", phy_utils_read_phyreg(pi, 0x73b));
	printf("acphy: 0x93b 0x%04X\n", phy_utils_read_phyreg(pi, 0x93b));
	printf("acphy: 0xb3b 0x%04X\n", phy_utils_read_phyreg(pi, 0xb3b));
	printf("acphy: 0xd3b 0x%04X\n", phy_utils_read_phyreg(pi, 0xd3b));
	printf("acphy: 0x735 0x%04X\n", phy_utils_read_phyreg(pi, 0x735));
	printf("acphy: 0x935 0x%04X\n", phy_utils_read_phyreg(pi, 0x935));
	printf("acphy: 0xb35 0x%04X\n", phy_utils_read_phyreg(pi, 0xb35));
	printf("acphy: 0xd35 0x%04X\n", phy_utils_read_phyreg(pi, 0xd35));
	printf("acphy: 0x734 0x%04X\n", phy_utils_read_phyreg(pi, 0x734));
	printf("acphy: 0x934 0x%04X\n", phy_utils_read_phyreg(pi, 0x934));
	printf("acphy: 0xb34 0x%04X\n", phy_utils_read_phyreg(pi, 0xb34));
	printf("acphy: 0xd34 0x%04X\n", phy_utils_read_phyreg(pi, 0xd34));
	printf("acphy: 0x737 0x%04X\n", phy_utils_read_phyreg(pi, 0x737));
	printf("acphy: 0x937 0x%04X\n", phy_utils_read_phyreg(pi, 0x937));
	printf("acphy: 0xb37 0x%04X\n", phy_utils_read_phyreg(pi, 0xb37));
	printf("acphy: 0xd37 0x%04X\n", phy_utils_read_phyreg(pi, 0xd37));
	printf("acphy: 0x736 0x%04X\n", phy_utils_read_phyreg(pi, 0x736));
	printf("acphy: 0x936 0x%04X\n", phy_utils_read_phyreg(pi, 0x936));
	printf("acphy: 0xb36 0x%04X\n", phy_utils_read_phyreg(pi, 0xb36));
	printf("acphy: 0xd36 0x%04X\n", phy_utils_read_phyreg(pi, 0xd36));
	printf("acphy: 0x1701 0x%04X\n", phy_utils_read_phyreg(pi, 0x1701));
	printf("acphy: 0x1901 0x%04X\n", phy_utils_read_phyreg(pi, 0x1901));
	printf("acphy: 0x1b01 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b01));
	printf("acphy: 0x1d01 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d01));
	printf("acphy: 0x746 0x%04X\n", phy_utils_read_phyreg(pi, 0x746));
	printf("acphy: 0x946 0x%04X\n", phy_utils_read_phyreg(pi, 0x946));
	printf("acphy: 0xb46 0x%04X\n", phy_utils_read_phyreg(pi, 0xb46));
	printf("acphy: 0xd46 0x%04X\n", phy_utils_read_phyreg(pi, 0xd46));
	printf("acphy: 0x738 0x%04X\n", phy_utils_read_phyreg(pi, 0x738));
	printf("acphy: 0x938 0x%04X\n", phy_utils_read_phyreg(pi, 0x938));
	printf("acphy: 0xb38 0x%04X\n", phy_utils_read_phyreg(pi, 0xb38));
	printf("acphy: 0xd38 0x%04X\n", phy_utils_read_phyreg(pi, 0xd38));
	printf("acphy: 0x730 0x%04X\n", phy_utils_read_phyreg(pi, 0x730));
	printf("acphy: 0x930 0x%04X\n", phy_utils_read_phyreg(pi, 0x930));
	printf("acphy: 0xb30 0x%04X\n", phy_utils_read_phyreg(pi, 0xb30));
	printf("acphy: 0xd30 0x%04X\n", phy_utils_read_phyreg(pi, 0xd30));
	printf("acphy: 0x731 0x%04X\n", phy_utils_read_phyreg(pi, 0x731));
	printf("acphy: 0x931 0x%04X\n", phy_utils_read_phyreg(pi, 0x931));
	printf("acphy: 0xb31 0x%04X\n", phy_utils_read_phyreg(pi, 0xb31));
	printf("acphy: 0xd31 0x%04X\n", phy_utils_read_phyreg(pi, 0xd31));
	printf("acphy: 0x729 0x%04X\n", phy_utils_read_phyreg(pi, 0x729));
	printf("acphy: 0x929 0x%04X\n", phy_utils_read_phyreg(pi, 0x929));
	printf("acphy: 0xb29 0x%04X\n", phy_utils_read_phyreg(pi, 0xb29));
	printf("acphy: 0xd29 0x%04X\n", phy_utils_read_phyreg(pi, 0xd29));
	printf("acphy: 0x732 0x%04X\n", phy_utils_read_phyreg(pi, 0x732));
	printf("acphy: 0x932 0x%04X\n", phy_utils_read_phyreg(pi, 0x932));
	printf("acphy: 0xb32 0x%04X\n", phy_utils_read_phyreg(pi, 0xb32));
	printf("acphy: 0xd32 0x%04X\n", phy_utils_read_phyreg(pi, 0xd32));
	printf("acphy: 0x733 0x%04X\n", phy_utils_read_phyreg(pi, 0x733));
	printf("acphy: 0x933 0x%04X\n", phy_utils_read_phyreg(pi, 0x933));
	printf("acphy: 0xb33 0x%04X\n", phy_utils_read_phyreg(pi, 0xb33));
	printf("acphy: 0xd33 0x%04X\n", phy_utils_read_phyreg(pi, 0xd33));
	printf("acphy: 0x170c 0x%04X\n", phy_utils_read_phyreg(pi, 0x170c));
	printf("acphy: 0x190c 0x%04X\n", phy_utils_read_phyreg(pi, 0x190c));
	printf("acphy: 0x1b0c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b0c));
	printf("acphy: 0x1d0c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d0c));
	printf("acphy: 0x728 0x%04X\n", phy_utils_read_phyreg(pi, 0x728));
	printf("acphy: 0x928 0x%04X\n", phy_utils_read_phyreg(pi, 0x928));
	printf("acphy: 0xb28 0x%04X\n", phy_utils_read_phyreg(pi, 0xb28));
	printf("acphy: 0xd28 0x%04X\n", phy_utils_read_phyreg(pi, 0xd28));
	printf("acphy: 0x73e 0x%04X\n", phy_utils_read_phyreg(pi, 0x73e));
	printf("acphy: 0x93e 0x%04X\n", phy_utils_read_phyreg(pi, 0x93e));
	printf("acphy: 0xb3e 0x%04X\n", phy_utils_read_phyreg(pi, 0xb3e));
	printf("acphy: 0xd3e 0x%04X\n", phy_utils_read_phyreg(pi, 0xd3e));
	printf("acphy: 0x725 0x%04X\n", phy_utils_read_phyreg(pi, 0x725));
	printf("acphy: 0x925 0x%04X\n", phy_utils_read_phyreg(pi, 0x925));
	printf("acphy: 0xb25 0x%04X\n", phy_utils_read_phyreg(pi, 0xb25));
	printf("acphy: 0xd25 0x%04X\n", phy_utils_read_phyreg(pi, 0xd25));
	printf("acphy: 0x74c 0x%04X\n", phy_utils_read_phyreg(pi, 0x74c));
	printf("acphy: 0x94c 0x%04X\n", phy_utils_read_phyreg(pi, 0x94c));
	printf("acphy: 0xb4c 0x%04X\n", phy_utils_read_phyreg(pi, 0xb4c));
	printf("acphy: 0xd4c 0x%04X\n", phy_utils_read_phyreg(pi, 0xd4c));
	printf("acphy: 0x727 0x%04X\n", phy_utils_read_phyreg(pi, 0x727));
	printf("acphy: 0x927 0x%04X\n", phy_utils_read_phyreg(pi, 0x927));
	printf("acphy: 0xb27 0x%04X\n", phy_utils_read_phyreg(pi, 0xb27));
	printf("acphy: 0xd27 0x%04X\n", phy_utils_read_phyreg(pi, 0xd27));
	printf("acphy: 0x74d 0x%04X\n", phy_utils_read_phyreg(pi, 0x74d));
	printf("acphy: 0x94d 0x%04X\n", phy_utils_read_phyreg(pi, 0x94d));
	printf("acphy: 0xb4d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb4d));
	printf("acphy: 0xd4d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd4d));
	printf("acphy: 0x1704 0x%04X\n", phy_utils_read_phyreg(pi, 0x1704));
	printf("acphy: 0x1904 0x%04X\n", phy_utils_read_phyreg(pi, 0x1904));
	printf("acphy: 0x1b04 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b04));
	printf("acphy: 0x1d04 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d04));
	printf("acphy: 0x722 0x%04X\n", phy_utils_read_phyreg(pi, 0x722));
	printf("acphy: 0x922 0x%04X\n", phy_utils_read_phyreg(pi, 0x922));
	printf("acphy: 0xb22 0x%04X\n", phy_utils_read_phyreg(pi, 0xb22));
	printf("acphy: 0xd22 0x%04X\n", phy_utils_read_phyreg(pi, 0xd22));
	printf("acphy: 0x416 0x%04X\n", phy_utils_read_phyreg(pi, 0x416));
	printf("acphy: 0x1702 0x%04X\n", phy_utils_read_phyreg(pi, 0x1702));
	printf("acphy: 0x1902 0x%04X\n", phy_utils_read_phyreg(pi, 0x1902));
	printf("acphy: 0x1b02 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b02));
	printf("acphy: 0x1d02 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d02));
	printf("acphy: 0x726 0x%04X\n", phy_utils_read_phyreg(pi, 0x726));
	printf("acphy: 0x926 0x%04X\n", phy_utils_read_phyreg(pi, 0x926));
	printf("acphy: 0xb26 0x%04X\n", phy_utils_read_phyreg(pi, 0xb26));
	printf("acphy: 0xd26 0x%04X\n", phy_utils_read_phyreg(pi, 0xd26));
	printf("acphy: 0x723 0x%04X\n", phy_utils_read_phyreg(pi, 0x723));
	printf("acphy: 0x923 0x%04X\n", phy_utils_read_phyreg(pi, 0x923));
	printf("acphy: 0xb23 0x%04X\n", phy_utils_read_phyreg(pi, 0xb23));
	printf("acphy: 0xd23 0x%04X\n", phy_utils_read_phyreg(pi, 0xd23));
	printf("acphy: 0x724 0x%04X\n", phy_utils_read_phyreg(pi, 0x724));
	printf("acphy: 0x924 0x%04X\n", phy_utils_read_phyreg(pi, 0x924));
	printf("acphy: 0xb24 0x%04X\n", phy_utils_read_phyreg(pi, 0xb24));
	printf("acphy: 0xd24 0x%04X\n", phy_utils_read_phyreg(pi, 0xd24));
	printf("acphy: 0x1700 0x%04X\n", phy_utils_read_phyreg(pi, 0x1700));
	printf("acphy: 0x1900 0x%04X\n", phy_utils_read_phyreg(pi, 0x1900));
	printf("acphy: 0x1b00 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b00));
	printf("acphy: 0x1d00 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d00));
	printf("acphy: 0x721 0x%04X\n", phy_utils_read_phyreg(pi, 0x721));
	printf("acphy: 0x921 0x%04X\n", phy_utils_read_phyreg(pi, 0x921));
	printf("acphy: 0xb21 0x%04X\n", phy_utils_read_phyreg(pi, 0xb21));
	printf("acphy: 0xd21 0x%04X\n", phy_utils_read_phyreg(pi, 0xd21));
	printf("acphy: 0x720 0x%04X\n", phy_utils_read_phyreg(pi, 0x720));
	printf("acphy: 0x920 0x%04X\n", phy_utils_read_phyreg(pi, 0x920));
	printf("acphy: 0xb20 0x%04X\n", phy_utils_read_phyreg(pi, 0xb20));
	printf("acphy: 0xd20 0x%04X\n", phy_utils_read_phyreg(pi, 0xd20));
	printf("acphy: 0x40e 0x%04X\n", phy_utils_read_phyreg(pi, 0x40e));
	printf("acphy: 0x401 0x%04X\n", phy_utils_read_phyreg(pi, 0x401));
	printf("acphy: 0x403 0x%04X\n", phy_utils_read_phyreg(pi, 0x403));
	printf("acphy: 0x404 0x%04X\n", phy_utils_read_phyreg(pi, 0x404));
	printf("acphy: 0x412 0x%04X\n", phy_utils_read_phyreg(pi, 0x412));
	printf("acphy: 0x402 0x%04X\n", phy_utils_read_phyreg(pi, 0x402));
	printf("acphy: 0x411 0x%04X\n", phy_utils_read_phyreg(pi, 0x411));
	printf("acphy: 0x3ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ab));
	printf("acphy: 0x1f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f4));
	printf("acphy: 0x6c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c7));
	printf("acphy: 0x8c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c7));
	printf("acphy: 0xac7 0x%04X\n", phy_utils_read_phyreg(pi, 0xac7));
	printf("acphy: 0xcc7 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc7));
	printf("acphy: 0x6c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c6));
	printf("acphy: 0x8c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c6));
	printf("acphy: 0xac6 0x%04X\n", phy_utils_read_phyreg(pi, 0xac6));
	printf("acphy: 0xcc6 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc6));
	printf("acphy: 0x520 0x%04X\n", phy_utils_read_phyreg(pi, 0x520));
	printf("acphy: 0x3ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ae));
	printf("acphy: 0x113b 0x%04X\n", phy_utils_read_phyreg(pi, 0x113b));
	printf("acphy: 0x1138 0x%04X\n", phy_utils_read_phyreg(pi, 0x1138));
	printf("acphy: 0x1cd 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cd));
	printf("acphy: 0x113a 0x%04X\n", phy_utils_read_phyreg(pi, 0x113a));
	printf("acphy: 0x179 0x%04X\n", phy_utils_read_phyreg(pi, 0x179));
	printf("acphy: 0x3ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ca));
	printf("acphy: 0x3cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x3cb));
	printf("acphy: 0x199 0x%04X\n", phy_utils_read_phyreg(pi, 0x199));
	printf("acphy: 0x19b 0x%04X\n", phy_utils_read_phyreg(pi, 0x19b));
	printf("acphy: 0x19a 0x%04X\n", phy_utils_read_phyreg(pi, 0x19a));
	printf("acphy: 0x19c 0x%04X\n", phy_utils_read_phyreg(pi, 0x19c));
	printf("acphy: 0x6a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a4));
	printf("acphy: 0x6a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a5));
	printf("acphy: 0x6ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ae));
	printf("acphy: 0x6a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a6));
	printf("acphy: 0x6a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a7));
	printf("acphy: 0x6a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a8));
	printf("acphy: 0x6a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x6a9));
	printf("acphy: 0x6aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x6aa));
	printf("acphy: 0x6ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ab));
	printf("acphy: 0x6ac 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ac));
	printf("acphy: 0x6ad 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ad));
	printf("acphy: 0x8a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a4));
	printf("acphy: 0x8a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a5));
	printf("acphy: 0x8ae 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ae));
	printf("acphy: 0x8a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a6));
	printf("acphy: 0x8a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a7));
	printf("acphy: 0x8a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a8));
	printf("acphy: 0x8a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x8a9));
	printf("acphy: 0x8aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x8aa));
	printf("acphy: 0x8ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ab));
	printf("acphy: 0x8ac 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ac));
	printf("acphy: 0x8ad 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ad));
	printf("acphy: 0xaa4 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa4));
	printf("acphy: 0xaa5 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa5));
	printf("acphy: 0xaae 0x%04X\n", phy_utils_read_phyreg(pi, 0xaae));
	printf("acphy: 0xaa6 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa6));
	printf("acphy: 0xaa7 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa7));
	printf("acphy: 0xaa8 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa8));
	printf("acphy: 0xaa9 0x%04X\n", phy_utils_read_phyreg(pi, 0xaa9));
	printf("acphy: 0xaaa 0x%04X\n", phy_utils_read_phyreg(pi, 0xaaa));
	printf("acphy: 0xaab 0x%04X\n", phy_utils_read_phyreg(pi, 0xaab));
	printf("acphy: 0xaac 0x%04X\n", phy_utils_read_phyreg(pi, 0xaac));
	printf("acphy: 0xaad 0x%04X\n", phy_utils_read_phyreg(pi, 0xaad));
	printf("acphy: 0xca4 0x%04X\n", phy_utils_read_phyreg(pi, 0xca4));
	printf("acphy: 0xca5 0x%04X\n", phy_utils_read_phyreg(pi, 0xca5));
	printf("acphy: 0xcae 0x%04X\n", phy_utils_read_phyreg(pi, 0xcae));
	printf("acphy: 0xca6 0x%04X\n", phy_utils_read_phyreg(pi, 0xca6));
	printf("acphy: 0xca7 0x%04X\n", phy_utils_read_phyreg(pi, 0xca7));
	printf("acphy: 0xca8 0x%04X\n", phy_utils_read_phyreg(pi, 0xca8));
	printf("acphy: 0xca9 0x%04X\n", phy_utils_read_phyreg(pi, 0xca9));
	printf("acphy: 0xcaa 0x%04X\n", phy_utils_read_phyreg(pi, 0xcaa));
	printf("acphy: 0xcab 0x%04X\n", phy_utils_read_phyreg(pi, 0xcab));
	printf("acphy: 0xcac 0x%04X\n", phy_utils_read_phyreg(pi, 0xcac));
	printf("acphy: 0xcad 0x%04X\n", phy_utils_read_phyreg(pi, 0xcad));
	printf("acphy: 0x211 0x%04X\n", phy_utils_read_phyreg(pi, 0x211));
	printf("acphy: 0x210 0x%04X\n", phy_utils_read_phyreg(pi, 0x210));
	printf("acphy: 0x695 0x%04X\n", phy_utils_read_phyreg(pi, 0x695));
	printf("acphy: 0x895 0x%04X\n", phy_utils_read_phyreg(pi, 0x895));
	printf("acphy: 0xa95 0x%04X\n", phy_utils_read_phyreg(pi, 0xa95));
	printf("acphy: 0xc95 0x%04X\n", phy_utils_read_phyreg(pi, 0xc95));
	printf("acphy: 0x696 0x%04X\n", phy_utils_read_phyreg(pi, 0x696));
	printf("acphy: 0x896 0x%04X\n", phy_utils_read_phyreg(pi, 0x896));
	printf("acphy: 0xa96 0x%04X\n", phy_utils_read_phyreg(pi, 0xa96));
	printf("acphy: 0xc96 0x%04X\n", phy_utils_read_phyreg(pi, 0xc96));
	printf("acphy: 0x693 0x%04X\n", phy_utils_read_phyreg(pi, 0x693));
	printf("acphy: 0x893 0x%04X\n", phy_utils_read_phyreg(pi, 0x893));
	printf("acphy: 0xa93 0x%04X\n", phy_utils_read_phyreg(pi, 0xa93));
	printf("acphy: 0xc93 0x%04X\n", phy_utils_read_phyreg(pi, 0xc93));
	printf("acphy: 0x694 0x%04X\n", phy_utils_read_phyreg(pi, 0x694));
	printf("acphy: 0x894 0x%04X\n", phy_utils_read_phyreg(pi, 0x894));
	printf("acphy: 0xa94 0x%04X\n", phy_utils_read_phyreg(pi, 0xa94));
	printf("acphy: 0xc94 0x%04X\n", phy_utils_read_phyreg(pi, 0xc94));
	printf("acphy: 0x19e 0x%04X\n", phy_utils_read_phyreg(pi, 0x19e));
	printf("acphy: 0x19d 0x%04X\n", phy_utils_read_phyreg(pi, 0x19d));
	printf("acphy: 0x19f 0x%04X\n", phy_utils_read_phyreg(pi, 0x19f));
	printf("acphy: 0x3ad 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ad));
	printf("acphy: 0x783 0x%04X\n", phy_utils_read_phyreg(pi, 0x783));
	printf("acphy: 0x983 0x%04X\n", phy_utils_read_phyreg(pi, 0x983));
	printf("acphy: 0xb83 0x%04X\n", phy_utils_read_phyreg(pi, 0xb83));
	printf("acphy: 0xd83 0x%04X\n", phy_utils_read_phyreg(pi, 0xd83));
	printf("acphy: 0x782 0x%04X\n", phy_utils_read_phyreg(pi, 0x782));
	printf("acphy: 0x982 0x%04X\n", phy_utils_read_phyreg(pi, 0x982));
	printf("acphy: 0xb82 0x%04X\n", phy_utils_read_phyreg(pi, 0xb82));
	printf("acphy: 0xd82 0x%04X\n", phy_utils_read_phyreg(pi, 0xd82));
	printf("acphy: 0x785 0x%04X\n", phy_utils_read_phyreg(pi, 0x785));
	printf("acphy: 0x985 0x%04X\n", phy_utils_read_phyreg(pi, 0x985));
	printf("acphy: 0xb85 0x%04X\n", phy_utils_read_phyreg(pi, 0xb85));
	printf("acphy: 0xd85 0x%04X\n", phy_utils_read_phyreg(pi, 0xd85));
	printf("acphy: 0x784 0x%04X\n", phy_utils_read_phyreg(pi, 0x784));
	printf("acphy: 0x984 0x%04X\n", phy_utils_read_phyreg(pi, 0x984));
	printf("acphy: 0xb84 0x%04X\n", phy_utils_read_phyreg(pi, 0xb84));
	printf("acphy: 0xd84 0x%04X\n", phy_utils_read_phyreg(pi, 0xd84));
	printf("acphy: 0x787 0x%04X\n", phy_utils_read_phyreg(pi, 0x787));
	printf("acphy: 0x987 0x%04X\n", phy_utils_read_phyreg(pi, 0x987));
	printf("acphy: 0xb87 0x%04X\n", phy_utils_read_phyreg(pi, 0xb87));
	printf("acphy: 0xd87 0x%04X\n", phy_utils_read_phyreg(pi, 0xd87));
	printf("acphy: 0x786 0x%04X\n", phy_utils_read_phyreg(pi, 0x786));
	printf("acphy: 0x986 0x%04X\n", phy_utils_read_phyreg(pi, 0x986));
	printf("acphy: 0xb86 0x%04X\n", phy_utils_read_phyreg(pi, 0xb86));
	printf("acphy: 0xd86 0x%04X\n", phy_utils_read_phyreg(pi, 0xd86));
	printf("acphy: 0x796 0x%04X\n", phy_utils_read_phyreg(pi, 0x796));
	printf("acphy: 0x996 0x%04X\n", phy_utils_read_phyreg(pi, 0x996));
	printf("acphy: 0xb96 0x%04X\n", phy_utils_read_phyreg(pi, 0xb96));
	printf("acphy: 0xd96 0x%04X\n", phy_utils_read_phyreg(pi, 0xd96));
	printf("acphy: 0x797 0x%04X\n", phy_utils_read_phyreg(pi, 0x797));
	printf("acphy: 0x997 0x%04X\n", phy_utils_read_phyreg(pi, 0x997));
	printf("acphy: 0xb97 0x%04X\n", phy_utils_read_phyreg(pi, 0xb97));
	printf("acphy: 0xd97 0x%04X\n", phy_utils_read_phyreg(pi, 0xd97));
	printf("acphy: 0x15b 0x%04X\n", phy_utils_read_phyreg(pi, 0x15b));
	printf("acphy: 0x15a 0x%04X\n", phy_utils_read_phyreg(pi, 0x15a));
	printf("acphy: 0x15c 0x%04X\n", phy_utils_read_phyreg(pi, 0x15c));
	printf("acphy: 0x141 0x%04X\n", phy_utils_read_phyreg(pi, 0x141));
	printf("acphy: 0x143 0x%04X\n", phy_utils_read_phyreg(pi, 0x143));
	printf("acphy: 0x144 0x%04X\n", phy_utils_read_phyreg(pi, 0x144));
	printf("acphy: 0x145 0x%04X\n", phy_utils_read_phyreg(pi, 0x145));
	printf("acphy: 0x146 0x%04X\n", phy_utils_read_phyreg(pi, 0x146));
	printf("acphy: 0x147 0x%04X\n", phy_utils_read_phyreg(pi, 0x147));
	printf("acphy: 0x148 0x%04X\n", phy_utils_read_phyreg(pi, 0x148));
	printf("acphy: 0x142 0x%04X\n", phy_utils_read_phyreg(pi, 0x142));
	printf("acphy: 0x149 0x%04X\n", phy_utils_read_phyreg(pi, 0x149));
	printf("acphy: 0x14a 0x%04X\n", phy_utils_read_phyreg(pi, 0x14a));
	printf("acphy: 0x14b 0x%04X\n", phy_utils_read_phyreg(pi, 0x14b));
	printf("acphy: 0x14c 0x%04X\n", phy_utils_read_phyreg(pi, 0x14c));
	printf("acphy: 0x14d 0x%04X\n", phy_utils_read_phyreg(pi, 0x14d));
	printf("acphy: 0x14e 0x%04X\n", phy_utils_read_phyreg(pi, 0x14e));
	printf("acphy: 0x14f 0x%04X\n", phy_utils_read_phyreg(pi, 0x14f));
	printf("acphy: 0x150 0x%04X\n", phy_utils_read_phyreg(pi, 0x150));
	printf("acphy: 0x151 0x%04X\n", phy_utils_read_phyreg(pi, 0x151));
	printf("acphy: 0x468 0x%04X\n", phy_utils_read_phyreg(pi, 0x468));
	printf("acphy: 0x460 0x%04X\n", phy_utils_read_phyreg(pi, 0x460));
	printf("acphy: 0x463 0x%04X\n", phy_utils_read_phyreg(pi, 0x463));
	printf("acphy: 0x5f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f3));
	printf("acphy: 0x462 0x%04X\n", phy_utils_read_phyreg(pi, 0x462));
	printf("acphy: 0x5f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f2));
	printf("acphy: 0x461 0x%04X\n", phy_utils_read_phyreg(pi, 0x461));
	printf("acphy: 0x5f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f1));
	printf("acphy: 0x5f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f0));
	printf("acphy: 0x465 0x%04X\n", phy_utils_read_phyreg(pi, 0x465));
	printf("acphy: 0x5f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f4));
	printf("acphy: 0x464 0x%04X\n", phy_utils_read_phyreg(pi, 0x464));
	printf("acphy: 0x5f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f8));
	printf("acphy: 0x466 0x%04X\n", phy_utils_read_phyreg(pi, 0x466));
	printf("acphy: 0x5f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f5));
	printf("acphy: 0x040 0x%04X\n", phy_utils_read_phyreg(pi, 0x040));
	printf("acphy: 0x162c 0x%04X\n", phy_utils_read_phyreg(pi, 0x162c));
	printf("acphy: 0x182c 0x%04X\n", phy_utils_read_phyreg(pi, 0x182c));
	printf("acphy: 0x1a2c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a2c));
	printf("acphy: 0x1c2c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c2c));
	printf("acphy: 0x1c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c2));
	printf("acphy: 0x1c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c3));
	printf("acphy: 0x1bf 0x%04X\n", phy_utils_read_phyreg(pi, 0x1bf));
	printf("acphy: 0x1c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c0));
	printf("acphy: 0x1c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c1));
	printf("acphy: 0x1bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x1bc));
	printf("acphy: 0x1bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x1bd));
	printf("acphy: 0x1be 0x%04X\n", phy_utils_read_phyreg(pi, 0x1be));
	printf("acphy: 0x175 0x%04X\n", phy_utils_read_phyreg(pi, 0x175));
	printf("acphy: 0x1a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a4));
	printf("acphy: 0x3c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c9));
	printf("acphy: 0x3c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c7));
	printf("acphy: 0x3c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c8));
	printf("acphy: 0x414 0x%04X\n", phy_utils_read_phyreg(pi, 0x414));
	printf("acphy: 0x2c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x2c0));
	printf("acphy: 0x043 0x%04X\n", phy_utils_read_phyreg(pi, 0x043));
	printf("acphy: 0x030 0x%04X\n", phy_utils_read_phyreg(pi, 0x030));
	printf("acphy: 0x2ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x2ee));
	printf("acphy: 0x1030 0x%04X\n", phy_utils_read_phyreg(pi, 0x1030));
	printf("acphy: 0x1035 0x%04X\n", phy_utils_read_phyreg(pi, 0x1035));
	printf("acphy: 0x1036 0x%04X\n", phy_utils_read_phyreg(pi, 0x1036));
	printf("acphy: 0x1037 0x%04X\n", phy_utils_read_phyreg(pi, 0x1037));
	printf("acphy: 0x1038 0x%04X\n", phy_utils_read_phyreg(pi, 0x1038));
	printf("acphy: 0x4f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x4f9));
	printf("acphy: 0x4fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x4fa));
	printf("acphy: 0x1e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e9));
	printf("acphy: 0x16a 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a));
	printf("acphy: 0x01f 0x%04X\n", phy_utils_read_phyreg(pi, 0x01f));
	printf("acphy: 0x1b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b8));
	printf("acphy: 0x470 0x%04X\n", phy_utils_read_phyreg(pi, 0x470));
	printf("acphy: 0x5f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f7));
	printf("acphy: 0x46f 0x%04X\n", phy_utils_read_phyreg(pi, 0x46f));
	printf("acphy: 0x5f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x5f6));
	printf("acphy: 0x476 0x%04X\n", phy_utils_read_phyreg(pi, 0x476));
	printf("acphy: 0x474 0x%04X\n", phy_utils_read_phyreg(pi, 0x474));
	printf("acphy: 0x477 0x%04X\n", phy_utils_read_phyreg(pi, 0x477));
	printf("acphy: 0x178 0x%04X\n", phy_utils_read_phyreg(pi, 0x178));
	printf("acphy: 0x10cf 0x%04X\n", phy_utils_read_phyreg(pi, 0x10cf));
	printf("acphy: 0x1130 0x%04X\n", phy_utils_read_phyreg(pi, 0x1130));
	printf("acphy: 0x10b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b0));
	printf("acphy: 0x10b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b1));
	printf("acphy: 0x10b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b2));
	printf("acphy: 0x10b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b3));
	printf("acphy: 0x10b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b4));
	printf("acphy: 0x10b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b5));
	printf("acphy: 0x10b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b6));
	printf("acphy: 0x10b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b7));
	printf("acphy: 0x10b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b8));
	printf("acphy: 0x10b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x10b9));
	printf("acphy: 0x10ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ba));
	printf("acphy: 0x10bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x10bb));
	printf("acphy: 0x10bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x10bc));
	printf("acphy: 0x10bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x10bd));
	printf("acphy: 0x10be 0x%04X\n", phy_utils_read_phyreg(pi, 0x10be));
	printf("acphy: 0x10bf 0x%04X\n", phy_utils_read_phyreg(pi, 0x10bf));
	printf("acphy: 0x10a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a0));
	printf("acphy: 0x10c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c8));
	printf("acphy: 0x10c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c9));
	printf("acphy: 0x10ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ca));
	printf("acphy: 0x10cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x10cb));
	printf("acphy: 0x10cc 0x%04X\n", phy_utils_read_phyreg(pi, 0x10cc));
	printf("acphy: 0x10cd 0x%04X\n", phy_utils_read_phyreg(pi, 0x10cd));
	printf("acphy: 0x70d 0x%04X\n", phy_utils_read_phyreg(pi, 0x70d));
	printf("acphy: 0x90d 0x%04X\n", phy_utils_read_phyreg(pi, 0x90d));
	printf("acphy: 0xb0d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb0d));
	printf("acphy: 0xd0d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd0d));
	printf("acphy: 0x1132 0x%04X\n", phy_utils_read_phyreg(pi, 0x1132));
	printf("acphy: 0x10a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a1));
	printf("acphy: 0x10ce 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ce));
	printf("acphy: 0x700 0x%04X\n", phy_utils_read_phyreg(pi, 0x700));
	printf("acphy: 0x703 0x%04X\n", phy_utils_read_phyreg(pi, 0x703));
	printf("acphy: 0x701 0x%04X\n", phy_utils_read_phyreg(pi, 0x701));
	printf("acphy: 0x702 0x%04X\n", phy_utils_read_phyreg(pi, 0x702));
	printf("acphy: 0x777 0x%04X\n", phy_utils_read_phyreg(pi, 0x777));
	printf("acphy: 0x772 0x%04X\n", phy_utils_read_phyreg(pi, 0x772));
	printf("acphy: 0x778 0x%04X\n", phy_utils_read_phyreg(pi, 0x778));
	printf("acphy: 0x77a 0x%04X\n", phy_utils_read_phyreg(pi, 0x77a));
	printf("acphy: 0x77b 0x%04X\n", phy_utils_read_phyreg(pi, 0x77b));
	printf("acphy: 0x77d 0x%04X\n", phy_utils_read_phyreg(pi, 0x77d));
	printf("acphy: 0x77c 0x%04X\n", phy_utils_read_phyreg(pi, 0x77c));
	printf("acphy: 0x770 0x%04X\n", phy_utils_read_phyreg(pi, 0x770));
	printf("acphy: 0x779 0x%04X\n", phy_utils_read_phyreg(pi, 0x779));
	printf("acphy: 0x773 0x%04X\n", phy_utils_read_phyreg(pi, 0x773));
	printf("acphy: 0x776 0x%04X\n", phy_utils_read_phyreg(pi, 0x776));
	printf("acphy: 0x774 0x%04X\n", phy_utils_read_phyreg(pi, 0x774));
	printf("acphy: 0x771 0x%04X\n", phy_utils_read_phyreg(pi, 0x771));
	printf("acphy: 0x775 0x%04X\n", phy_utils_read_phyreg(pi, 0x775));
	printf("acphy: 0x704 0x%04X\n", phy_utils_read_phyreg(pi, 0x704));
	printf("acphy: 0x705 0x%04X\n", phy_utils_read_phyreg(pi, 0x705));
	printf("acphy: 0x706 0x%04X\n", phy_utils_read_phyreg(pi, 0x706));
	printf("acphy: 0x707 0x%04X\n", phy_utils_read_phyreg(pi, 0x707));
	printf("acphy: 0x708 0x%04X\n", phy_utils_read_phyreg(pi, 0x708));
	printf("acphy: 0x709 0x%04X\n", phy_utils_read_phyreg(pi, 0x709));
	printf("acphy: 0x70a 0x%04X\n", phy_utils_read_phyreg(pi, 0x70a));
	printf("acphy: 0x70b 0x%04X\n", phy_utils_read_phyreg(pi, 0x70b));
	printf("acphy: 0x70c 0x%04X\n", phy_utils_read_phyreg(pi, 0x70c));
	printf("acphy: 0x900 0x%04X\n", phy_utils_read_phyreg(pi, 0x900));
	printf("acphy: 0x903 0x%04X\n", phy_utils_read_phyreg(pi, 0x903));
	printf("acphy: 0x901 0x%04X\n", phy_utils_read_phyreg(pi, 0x901));
	printf("acphy: 0x902 0x%04X\n", phy_utils_read_phyreg(pi, 0x902));
	printf("acphy: 0x977 0x%04X\n", phy_utils_read_phyreg(pi, 0x977));
	printf("acphy: 0x972 0x%04X\n", phy_utils_read_phyreg(pi, 0x972));
	printf("acphy: 0x978 0x%04X\n", phy_utils_read_phyreg(pi, 0x978));
	printf("acphy: 0x97a 0x%04X\n", phy_utils_read_phyreg(pi, 0x97a));
	printf("acphy: 0x97b 0x%04X\n", phy_utils_read_phyreg(pi, 0x97b));
	printf("acphy: 0x97d 0x%04X\n", phy_utils_read_phyreg(pi, 0x97d));
	printf("acphy: 0x97c 0x%04X\n", phy_utils_read_phyreg(pi, 0x97c));
	printf("acphy: 0x970 0x%04X\n", phy_utils_read_phyreg(pi, 0x970));
	printf("acphy: 0x979 0x%04X\n", phy_utils_read_phyreg(pi, 0x979));
	printf("acphy: 0x973 0x%04X\n", phy_utils_read_phyreg(pi, 0x973));
	printf("acphy: 0x976 0x%04X\n", phy_utils_read_phyreg(pi, 0x976));
	printf("acphy: 0x974 0x%04X\n", phy_utils_read_phyreg(pi, 0x974));
	printf("acphy: 0x971 0x%04X\n", phy_utils_read_phyreg(pi, 0x971));
	printf("acphy: 0x975 0x%04X\n", phy_utils_read_phyreg(pi, 0x975));
	printf("acphy: 0x904 0x%04X\n", phy_utils_read_phyreg(pi, 0x904));
	printf("acphy: 0x905 0x%04X\n", phy_utils_read_phyreg(pi, 0x905));
	printf("acphy: 0x906 0x%04X\n", phy_utils_read_phyreg(pi, 0x906));
	printf("acphy: 0x907 0x%04X\n", phy_utils_read_phyreg(pi, 0x907));
	printf("acphy: 0x908 0x%04X\n", phy_utils_read_phyreg(pi, 0x908));
	printf("acphy: 0x909 0x%04X\n", phy_utils_read_phyreg(pi, 0x909));
	printf("acphy: 0x90a 0x%04X\n", phy_utils_read_phyreg(pi, 0x90a));
	printf("acphy: 0x90b 0x%04X\n", phy_utils_read_phyreg(pi, 0x90b));
	printf("acphy: 0x90c 0x%04X\n", phy_utils_read_phyreg(pi, 0x90c));
	printf("acphy: 0xb00 0x%04X\n", phy_utils_read_phyreg(pi, 0xb00));
	printf("acphy: 0xb03 0x%04X\n", phy_utils_read_phyreg(pi, 0xb03));
	printf("acphy: 0xb01 0x%04X\n", phy_utils_read_phyreg(pi, 0xb01));
	printf("acphy: 0xb02 0x%04X\n", phy_utils_read_phyreg(pi, 0xb02));
	printf("acphy: 0xb77 0x%04X\n", phy_utils_read_phyreg(pi, 0xb77));
	printf("acphy: 0xb72 0x%04X\n", phy_utils_read_phyreg(pi, 0xb72));
	printf("acphy: 0xb78 0x%04X\n", phy_utils_read_phyreg(pi, 0xb78));
	printf("acphy: 0xb7a 0x%04X\n", phy_utils_read_phyreg(pi, 0xb7a));
	printf("acphy: 0xb7b 0x%04X\n", phy_utils_read_phyreg(pi, 0xb7b));
	printf("acphy: 0xb7d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb7d));
	printf("acphy: 0xb7c 0x%04X\n", phy_utils_read_phyreg(pi, 0xb7c));
	printf("acphy: 0xb70 0x%04X\n", phy_utils_read_phyreg(pi, 0xb70));
	printf("acphy: 0xb79 0x%04X\n", phy_utils_read_phyreg(pi, 0xb79));
	printf("acphy: 0xb73 0x%04X\n", phy_utils_read_phyreg(pi, 0xb73));
	printf("acphy: 0xb76 0x%04X\n", phy_utils_read_phyreg(pi, 0xb76));
	printf("acphy: 0xb74 0x%04X\n", phy_utils_read_phyreg(pi, 0xb74));
	printf("acphy: 0xb71 0x%04X\n", phy_utils_read_phyreg(pi, 0xb71));
	printf("acphy: 0xb75 0x%04X\n", phy_utils_read_phyreg(pi, 0xb75));
	printf("acphy: 0xb04 0x%04X\n", phy_utils_read_phyreg(pi, 0xb04));
	printf("acphy: 0xb05 0x%04X\n", phy_utils_read_phyreg(pi, 0xb05));
	printf("acphy: 0xb06 0x%04X\n", phy_utils_read_phyreg(pi, 0xb06));
	printf("acphy: 0xb07 0x%04X\n", phy_utils_read_phyreg(pi, 0xb07));
	printf("acphy: 0xb08 0x%04X\n", phy_utils_read_phyreg(pi, 0xb08));
	printf("acphy: 0xb09 0x%04X\n", phy_utils_read_phyreg(pi, 0xb09));
	printf("acphy: 0xb0a 0x%04X\n", phy_utils_read_phyreg(pi, 0xb0a));
	printf("acphy: 0xb0b 0x%04X\n", phy_utils_read_phyreg(pi, 0xb0b));
	printf("acphy: 0xb0c 0x%04X\n", phy_utils_read_phyreg(pi, 0xb0c));
	printf("acphy: 0xd00 0x%04X\n", phy_utils_read_phyreg(pi, 0xd00));
	printf("acphy: 0xd03 0x%04X\n", phy_utils_read_phyreg(pi, 0xd03));
	printf("acphy: 0xd01 0x%04X\n", phy_utils_read_phyreg(pi, 0xd01));
	printf("acphy: 0xd02 0x%04X\n", phy_utils_read_phyreg(pi, 0xd02));
	printf("acphy: 0xd77 0x%04X\n", phy_utils_read_phyreg(pi, 0xd77));
	printf("acphy: 0xd72 0x%04X\n", phy_utils_read_phyreg(pi, 0xd72));
	printf("acphy: 0xd78 0x%04X\n", phy_utils_read_phyreg(pi, 0xd78));
	printf("acphy: 0xd7a 0x%04X\n", phy_utils_read_phyreg(pi, 0xd7a));
	printf("acphy: 0xd7b 0x%04X\n", phy_utils_read_phyreg(pi, 0xd7b));
	printf("acphy: 0xd7d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd7d));
	printf("acphy: 0xd7c 0x%04X\n", phy_utils_read_phyreg(pi, 0xd7c));
	printf("acphy: 0xd70 0x%04X\n", phy_utils_read_phyreg(pi, 0xd70));
	printf("acphy: 0xd79 0x%04X\n", phy_utils_read_phyreg(pi, 0xd79));
	printf("acphy: 0xd73 0x%04X\n", phy_utils_read_phyreg(pi, 0xd73));
	printf("acphy: 0xd76 0x%04X\n", phy_utils_read_phyreg(pi, 0xd76));
	printf("acphy: 0xd74 0x%04X\n", phy_utils_read_phyreg(pi, 0xd74));
	printf("acphy: 0xd71 0x%04X\n", phy_utils_read_phyreg(pi, 0xd71));
	printf("acphy: 0xd75 0x%04X\n", phy_utils_read_phyreg(pi, 0xd75));
	printf("acphy: 0xd04 0x%04X\n", phy_utils_read_phyreg(pi, 0xd04));
	printf("acphy: 0xd05 0x%04X\n", phy_utils_read_phyreg(pi, 0xd05));
	printf("acphy: 0xd06 0x%04X\n", phy_utils_read_phyreg(pi, 0xd06));
	printf("acphy: 0xd07 0x%04X\n", phy_utils_read_phyreg(pi, 0xd07));
	printf("acphy: 0xd08 0x%04X\n", phy_utils_read_phyreg(pi, 0xd08));
	printf("acphy: 0xd09 0x%04X\n", phy_utils_read_phyreg(pi, 0xd09));
	printf("acphy: 0xd0a 0x%04X\n", phy_utils_read_phyreg(pi, 0xd0a));
	printf("acphy: 0xd0b 0x%04X\n", phy_utils_read_phyreg(pi, 0xd0b));
	printf("acphy: 0xd0c 0x%04X\n", phy_utils_read_phyreg(pi, 0xd0c));
	printf("acphy: 0x10a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a5));
	printf("acphy: 0x1131 0x%04X\n", phy_utils_read_phyreg(pi, 0x1131));
	printf("acphy: 0x10c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c3));
	printf("acphy: 0x10a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a2));
	printf("acphy: 0x10a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a7));
	printf("acphy: 0x10a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a6));
	printf("acphy: 0x52b 0x%04X\n", phy_utils_read_phyreg(pi, 0x52b));
	printf("acphy: 0x52c 0x%04X\n", phy_utils_read_phyreg(pi, 0x52c));
	printf("acphy: 0x573 0x%04X\n", phy_utils_read_phyreg(pi, 0x573));
	printf("acphy: 0x571 0x%04X\n", phy_utils_read_phyreg(pi, 0x571));
	printf("acphy: 0x572 0x%04X\n", phy_utils_read_phyreg(pi, 0x572));
	printf("acphy: 0x574 0x%04X\n", phy_utils_read_phyreg(pi, 0x574));
	printf("acphy: 0x1684 0x%04X\n", phy_utils_read_phyreg(pi, 0x1684));
	printf("acphy: 0x1884 0x%04X\n", phy_utils_read_phyreg(pi, 0x1884));
	printf("acphy: 0x1a84 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a84));
	printf("acphy: 0x1c84 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c84));
	printf("acphy: 0x1686 0x%04X\n", phy_utils_read_phyreg(pi, 0x1686));
	printf("acphy: 0x1886 0x%04X\n", phy_utils_read_phyreg(pi, 0x1886));
	printf("acphy: 0x1a86 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a86));
	printf("acphy: 0x1c86 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c86));
	printf("acphy: 0x1685 0x%04X\n", phy_utils_read_phyreg(pi, 0x1685));
	printf("acphy: 0x1885 0x%04X\n", phy_utils_read_phyreg(pi, 0x1885));
	printf("acphy: 0x1a85 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a85));
	printf("acphy: 0x1c85 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c85));
	printf("acphy: 0x1687 0x%04X\n", phy_utils_read_phyreg(pi, 0x1687));
	printf("acphy: 0x1887 0x%04X\n", phy_utils_read_phyreg(pi, 0x1887));
	printf("acphy: 0x1a87 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a87));
	printf("acphy: 0x1c87 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c87));
	printf("acphy: 0x1680 0x%04X\n", phy_utils_read_phyreg(pi, 0x1680));
	printf("acphy: 0x1880 0x%04X\n", phy_utils_read_phyreg(pi, 0x1880));
	printf("acphy: 0x1a80 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a80));
	printf("acphy: 0x1c80 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c80));
	printf("acphy: 0x1682 0x%04X\n", phy_utils_read_phyreg(pi, 0x1682));
	printf("acphy: 0x1882 0x%04X\n", phy_utils_read_phyreg(pi, 0x1882));
	printf("acphy: 0x1a82 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a82));
	printf("acphy: 0x1c82 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c82));
	printf("acphy: 0x1681 0x%04X\n", phy_utils_read_phyreg(pi, 0x1681));
	printf("acphy: 0x1881 0x%04X\n", phy_utils_read_phyreg(pi, 0x1881));
	printf("acphy: 0x1a81 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a81));
	printf("acphy: 0x1c81 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c81));
	printf("acphy: 0x1683 0x%04X\n", phy_utils_read_phyreg(pi, 0x1683));
	printf("acphy: 0x1883 0x%04X\n", phy_utils_read_phyreg(pi, 0x1883));
	printf("acphy: 0x1a83 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a83));
	printf("acphy: 0x1c83 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c83));
	printf("acphy: 0x30e 0x%04X\n", phy_utils_read_phyreg(pi, 0x30e));
	printf("acphy: 0x1698 0x%04X\n", phy_utils_read_phyreg(pi, 0x1698));
	printf("acphy: 0x1898 0x%04X\n", phy_utils_read_phyreg(pi, 0x1898));
	printf("acphy: 0x1a98 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a98));
	printf("acphy: 0x1c98 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c98));
	printf("acphy: 0x169a 0x%04X\n", phy_utils_read_phyreg(pi, 0x169a));
	printf("acphy: 0x189a 0x%04X\n", phy_utils_read_phyreg(pi, 0x189a));
	printf("acphy: 0x1a9a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a9a));
	printf("acphy: 0x1c9a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c9a));
	printf("acphy: 0x1699 0x%04X\n", phy_utils_read_phyreg(pi, 0x1699));
	printf("acphy: 0x1899 0x%04X\n", phy_utils_read_phyreg(pi, 0x1899));
	printf("acphy: 0x1a99 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a99));
	printf("acphy: 0x1c99 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c99));
	printf("acphy: 0x169b 0x%04X\n", phy_utils_read_phyreg(pi, 0x169b));
	printf("acphy: 0x189b 0x%04X\n", phy_utils_read_phyreg(pi, 0x189b));
	printf("acphy: 0x1a9b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a9b));
	printf("acphy: 0x1c9b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c9b));
	printf("acphy: 0x1116 0x%04X\n", phy_utils_read_phyreg(pi, 0x1116));
	printf("acphy: 0x1117 0x%04X\n", phy_utils_read_phyreg(pi, 0x1117));
	printf("acphy: 0x1118 0x%04X\n", phy_utils_read_phyreg(pi, 0x1118));
	printf("acphy: 0x1115 0x%04X\n", phy_utils_read_phyreg(pi, 0x1115));
	printf("acphy: 0x1112 0x%04X\n", phy_utils_read_phyreg(pi, 0x1112));
	printf("acphy: 0x1110 0x%04X\n", phy_utils_read_phyreg(pi, 0x1110));
	printf("acphy: 0x1111 0x%04X\n", phy_utils_read_phyreg(pi, 0x1111));
	printf("acphy: 0xe54 0x%04X\n", phy_utils_read_phyreg(pi, 0xe54));
	printf("acphy: 0xe52 0x%04X\n", phy_utils_read_phyreg(pi, 0xe52));
	printf("acphy: 0xe53 0x%04X\n", phy_utils_read_phyreg(pi, 0xe53));
	printf("acphy: 0xe57 0x%04X\n", phy_utils_read_phyreg(pi, 0xe57));
	printf("acphy: 0xe55 0x%04X\n", phy_utils_read_phyreg(pi, 0xe55));
	printf("acphy: 0xe56 0x%04X\n", phy_utils_read_phyreg(pi, 0xe56));
	printf("acphy: 0xe59 0x%04X\n", phy_utils_read_phyreg(pi, 0xe59));
	printf("acphy: 0xe58 0x%04X\n", phy_utils_read_phyreg(pi, 0xe58));
	printf("acphy: 0x1073 0x%04X\n", phy_utils_read_phyreg(pi, 0x1073));
	printf("acphy: 0xe50 0x%04X\n", phy_utils_read_phyreg(pi, 0xe50));
	printf("acphy: 0x3c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c3));
	printf("acphy: 0x3c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x3c0));
	printf("acphy: 0x3bf 0x%04X\n", phy_utils_read_phyreg(pi, 0x3bf));
	printf("acphy: 0x3be 0x%04X\n", phy_utils_read_phyreg(pi, 0x3be));
	printf("acphy: 0x014 0x%04X\n", phy_utils_read_phyreg(pi, 0x014));
	printf("acphy: 0x11e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e0));
	printf("acphy: 0x11e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e1));
	printf("acphy: 0x11e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e2));
	printf("acphy: 0x11e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e3));
	printf("acphy: 0x11e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e4));
	printf("acphy: 0x11f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x11f3));
	printf("acphy: 0x11e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e5));
	printf("acphy: 0x11e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e6));
	printf("acphy: 0x11f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x11f2));
	printf("acphy: 0x11e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e7));
	printf("acphy: 0x11f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x11f4));
	printf("acphy: 0x11e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e8));
	printf("acphy: 0x11e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x11e9));
	printf("acphy: 0x11ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x11ea));
	printf("acphy: 0x11eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x11eb));
	printf("acphy: 0x11ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x11ec));
	printf("acphy: 0x11ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x11ed));
	printf("acphy: 0x11ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x11ee));
	printf("acphy: 0x11ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x11ef));
	printf("acphy: 0x11f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x11f0));
	printf("acphy: 0x11f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x11f1));
	printf("acphy: 0x1772 0x%04X\n", phy_utils_read_phyreg(pi, 0x1772));
	printf("acphy: 0x1972 0x%04X\n", phy_utils_read_phyreg(pi, 0x1972));
	printf("acphy: 0x1b72 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b72));
	printf("acphy: 0x1d72 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d72));
	printf("acphy: 0x1778 0x%04X\n", phy_utils_read_phyreg(pi, 0x1778));
	printf("acphy: 0x1978 0x%04X\n", phy_utils_read_phyreg(pi, 0x1978));
	printf("acphy: 0x1b78 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b78));
	printf("acphy: 0x1d78 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d78));
	printf("acphy: 0x1777 0x%04X\n", phy_utils_read_phyreg(pi, 0x1777));
	printf("acphy: 0x1977 0x%04X\n", phy_utils_read_phyreg(pi, 0x1977));
	printf("acphy: 0x1b77 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b77));
	printf("acphy: 0x1d77 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d77));
	printf("acphy: 0x1775 0x%04X\n", phy_utils_read_phyreg(pi, 0x1775));
	printf("acphy: 0x1975 0x%04X\n", phy_utils_read_phyreg(pi, 0x1975));
	printf("acphy: 0x1b75 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b75));
	printf("acphy: 0x1d75 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d75));
	printf("acphy: 0x1776 0x%04X\n", phy_utils_read_phyreg(pi, 0x1776));
	printf("acphy: 0x1976 0x%04X\n", phy_utils_read_phyreg(pi, 0x1976));
	printf("acphy: 0x1b76 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b76));
	printf("acphy: 0x1d76 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d76));
	printf("acphy: 0x1773 0x%04X\n", phy_utils_read_phyreg(pi, 0x1773));
	printf("acphy: 0x1973 0x%04X\n", phy_utils_read_phyreg(pi, 0x1973));
	printf("acphy: 0x1b73 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b73));
	printf("acphy: 0x1d73 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d73));
	printf("acphy: 0x1774 0x%04X\n", phy_utils_read_phyreg(pi, 0x1774));
	printf("acphy: 0x1974 0x%04X\n", phy_utils_read_phyreg(pi, 0x1974));
	printf("acphy: 0x1b74 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b74));
	printf("acphy: 0x1d74 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d74));
	printf("acphy: 0x1771 0x%04X\n", phy_utils_read_phyreg(pi, 0x1771));
	printf("acphy: 0x1971 0x%04X\n", phy_utils_read_phyreg(pi, 0x1971));
	printf("acphy: 0x1b71 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b71));
	printf("acphy: 0x1d71 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d71));
	printf("acphy: 0x1770 0x%04X\n", phy_utils_read_phyreg(pi, 0x1770));
	printf("acphy: 0x1970 0x%04X\n", phy_utils_read_phyreg(pi, 0x1970));
	printf("acphy: 0x1b70 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b70));
	printf("acphy: 0x1d70 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d70));
	printf("acphy: 0x00d 0x%04X\n", phy_utils_read_phyreg(pi, 0x00d));
	printf("acphy: 0x00e 0x%04X\n", phy_utils_read_phyreg(pi, 0x00e));
	printf("acphy: 0x1287 0x%04X\n", phy_utils_read_phyreg(pi, 0x1287));
	printf("acphy: 0x1288 0x%04X\n", phy_utils_read_phyreg(pi, 0x1288));
	printf("acphy: 0x6bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x6bb));
	printf("acphy: 0x8bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x8bb));
	printf("acphy: 0xabb 0x%04X\n", phy_utils_read_phyreg(pi, 0xabb));
	printf("acphy: 0xcbb 0x%04X\n", phy_utils_read_phyreg(pi, 0xcbb));
	printf("acphy: 0x6bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x6bc));
	printf("acphy: 0x8bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x8bc));
	printf("acphy: 0xabc 0x%04X\n", phy_utils_read_phyreg(pi, 0xabc));
	printf("acphy: 0xcbc 0x%04X\n", phy_utils_read_phyreg(pi, 0xcbc));
	printf("acphy: 0x16e 0x%04X\n", phy_utils_read_phyreg(pi, 0x16e));
	printf("acphy: 0x172 0x%04X\n", phy_utils_read_phyreg(pi, 0x172));
	printf("acphy: 0x1eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x1eb));
	printf("acphy: 0x1740 0x%04X\n", phy_utils_read_phyreg(pi, 0x1740));
	printf("acphy: 0x1749 0x%04X\n", phy_utils_read_phyreg(pi, 0x1749));
	printf("acphy: 0x1949 0x%04X\n", phy_utils_read_phyreg(pi, 0x1949));
	printf("acphy: 0x1b49 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b49));
	printf("acphy: 0x1d49 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d49));
	printf("acphy: 0x1940 0x%04X\n", phy_utils_read_phyreg(pi, 0x1940));
	printf("acphy: 0x174a 0x%04X\n", phy_utils_read_phyreg(pi, 0x174a));
	printf("acphy: 0x194a 0x%04X\n", phy_utils_read_phyreg(pi, 0x194a));
	printf("acphy: 0x1b4a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b4a));
	printf("acphy: 0x1d4a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d4a));
	printf("acphy: 0x1b40 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b40));
	printf("acphy: 0x174b 0x%04X\n", phy_utils_read_phyreg(pi, 0x174b));
	printf("acphy: 0x194b 0x%04X\n", phy_utils_read_phyreg(pi, 0x194b));
	printf("acphy: 0x1b4b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b4b));
	printf("acphy: 0x1d4b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d4b));
	printf("acphy: 0x1d40 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d40));
	printf("acphy: 0x714 0x%04X\n", phy_utils_read_phyreg(pi, 0x714));
	printf("acphy: 0x914 0x%04X\n", phy_utils_read_phyreg(pi, 0x914));
	printf("acphy: 0xb14 0x%04X\n", phy_utils_read_phyreg(pi, 0xb14));
	printf("acphy: 0xd14 0x%04X\n", phy_utils_read_phyreg(pi, 0xd14));
	printf("acphy: 0x715 0x%04X\n", phy_utils_read_phyreg(pi, 0x715));
	printf("acphy: 0x915 0x%04X\n", phy_utils_read_phyreg(pi, 0x915));
	printf("acphy: 0xb15 0x%04X\n", phy_utils_read_phyreg(pi, 0xb15));
	printf("acphy: 0xd15 0x%04X\n", phy_utils_read_phyreg(pi, 0xd15));
	printf("acphy: 0x716 0x%04X\n", phy_utils_read_phyreg(pi, 0x716));
	printf("acphy: 0x916 0x%04X\n", phy_utils_read_phyreg(pi, 0x916));
	printf("acphy: 0xb16 0x%04X\n", phy_utils_read_phyreg(pi, 0xb16));
	printf("acphy: 0xd16 0x%04X\n", phy_utils_read_phyreg(pi, 0xd16));
	printf("acphy: 0x717 0x%04X\n", phy_utils_read_phyreg(pi, 0x717));
	printf("acphy: 0x917 0x%04X\n", phy_utils_read_phyreg(pi, 0x917));
	printf("acphy: 0xb17 0x%04X\n", phy_utils_read_phyreg(pi, 0xb17));
	printf("acphy: 0xd17 0x%04X\n", phy_utils_read_phyreg(pi, 0xd17));
	printf("acphy: 0x718 0x%04X\n", phy_utils_read_phyreg(pi, 0x718));
	printf("acphy: 0x918 0x%04X\n", phy_utils_read_phyreg(pi, 0x918));
	printf("acphy: 0xb18 0x%04X\n", phy_utils_read_phyreg(pi, 0xb18));
	printf("acphy: 0xd18 0x%04X\n", phy_utils_read_phyreg(pi, 0xd18));
	printf("acphy: 0x719 0x%04X\n", phy_utils_read_phyreg(pi, 0x719));
	printf("acphy: 0x919 0x%04X\n", phy_utils_read_phyreg(pi, 0x919));
	printf("acphy: 0xb19 0x%04X\n", phy_utils_read_phyreg(pi, 0xb19));
	printf("acphy: 0xd19 0x%04X\n", phy_utils_read_phyreg(pi, 0xd19));
	printf("acphy: 0x71a 0x%04X\n", phy_utils_read_phyreg(pi, 0x71a));
	printf("acphy: 0x91a 0x%04X\n", phy_utils_read_phyreg(pi, 0x91a));
	printf("acphy: 0xb1a 0x%04X\n", phy_utils_read_phyreg(pi, 0xb1a));
	printf("acphy: 0xd1a 0x%04X\n", phy_utils_read_phyreg(pi, 0xd1a));
	printf("acphy: 0x1741 0x%04X\n", phy_utils_read_phyreg(pi, 0x1741));
	printf("acphy: 0x71b 0x%04X\n", phy_utils_read_phyreg(pi, 0x71b));
	printf("acphy: 0x91b 0x%04X\n", phy_utils_read_phyreg(pi, 0x91b));
	printf("acphy: 0xb1b 0x%04X\n", phy_utils_read_phyreg(pi, 0xb1b));
	printf("acphy: 0xd1b 0x%04X\n", phy_utils_read_phyreg(pi, 0xd1b));
	printf("acphy: 0x1941 0x%04X\n", phy_utils_read_phyreg(pi, 0x1941));
	printf("acphy: 0x71c 0x%04X\n", phy_utils_read_phyreg(pi, 0x71c));
	printf("acphy: 0x91c 0x%04X\n", phy_utils_read_phyreg(pi, 0x91c));
	printf("acphy: 0xb1c 0x%04X\n", phy_utils_read_phyreg(pi, 0xb1c));
	printf("acphy: 0xd1c 0x%04X\n", phy_utils_read_phyreg(pi, 0xd1c));
	printf("acphy: 0x1b41 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b41));
	printf("acphy: 0x71d 0x%04X\n", phy_utils_read_phyreg(pi, 0x71d));
	printf("acphy: 0x91d 0x%04X\n", phy_utils_read_phyreg(pi, 0x91d));
	printf("acphy: 0xb1d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb1d));
	printf("acphy: 0xd1d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd1d));
	printf("acphy: 0x1d41 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d41));
	printf("acphy: 0x71e 0x%04X\n", phy_utils_read_phyreg(pi, 0x71e));
	printf("acphy: 0x91e 0x%04X\n", phy_utils_read_phyreg(pi, 0x91e));
	printf("acphy: 0xb1e 0x%04X\n", phy_utils_read_phyreg(pi, 0xb1e));
	printf("acphy: 0xd1e 0x%04X\n", phy_utils_read_phyreg(pi, 0xd1e));
	printf("acphy: 0x71f 0x%04X\n", phy_utils_read_phyreg(pi, 0x71f));
	printf("acphy: 0x91f 0x%04X\n", phy_utils_read_phyreg(pi, 0x91f));
	printf("acphy: 0xb1f 0x%04X\n", phy_utils_read_phyreg(pi, 0xb1f));
	printf("acphy: 0xd1f 0x%04X\n", phy_utils_read_phyreg(pi, 0xd1f));
	printf("acphy: 0x1742 0x%04X\n", phy_utils_read_phyreg(pi, 0x1742));
	printf("acphy: 0x1942 0x%04X\n", phy_utils_read_phyreg(pi, 0x1942));
	printf("acphy: 0x1b42 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b42));
	printf("acphy: 0x1d42 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d42));
	printf("acphy: 0x1743 0x%04X\n", phy_utils_read_phyreg(pi, 0x1743));
	printf("acphy: 0x1943 0x%04X\n", phy_utils_read_phyreg(pi, 0x1943));
	printf("acphy: 0x1b43 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b43));
	printf("acphy: 0x1d43 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d43));
	printf("acphy: 0x1744 0x%04X\n", phy_utils_read_phyreg(pi, 0x1744));
	printf("acphy: 0x1944 0x%04X\n", phy_utils_read_phyreg(pi, 0x1944));
	printf("acphy: 0x1b44 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b44));
	printf("acphy: 0x1d44 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d44));
	printf("acphy: 0x1745 0x%04X\n", phy_utils_read_phyreg(pi, 0x1745));
	printf("acphy: 0x1945 0x%04X\n", phy_utils_read_phyreg(pi, 0x1945));
	printf("acphy: 0x1b45 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b45));
	printf("acphy: 0x1d45 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d45));
	printf("acphy: 0x1746 0x%04X\n", phy_utils_read_phyreg(pi, 0x1746));
	printf("acphy: 0x1946 0x%04X\n", phy_utils_read_phyreg(pi, 0x1946));
	printf("acphy: 0x1b46 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b46));
	printf("acphy: 0x1d46 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d46));
	printf("acphy: 0x1747 0x%04X\n", phy_utils_read_phyreg(pi, 0x1747));
	printf("acphy: 0x1947 0x%04X\n", phy_utils_read_phyreg(pi, 0x1947));
	printf("acphy: 0x1b47 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b47));
	printf("acphy: 0x1d47 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d47));
	printf("acphy: 0x1748 0x%04X\n", phy_utils_read_phyreg(pi, 0x1748));
	printf("acphy: 0x1948 0x%04X\n", phy_utils_read_phyreg(pi, 0x1948));
	printf("acphy: 0x1b48 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b48));
	printf("acphy: 0x1d48 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d48));
	printf("acphy: 0x47e 0x%04X\n", phy_utils_read_phyreg(pi, 0x47e));
	printf("acphy: 0x47f 0x%04X\n", phy_utils_read_phyreg(pi, 0x47f));
	printf("acphy: 0x41a 0x%04X\n", phy_utils_read_phyreg(pi, 0x41a));
	printf("acphy: 0x3d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x3d3));
	printf("acphy: 0x3b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b8));
	printf("acphy: 0x3b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b7));
	printf("acphy: 0x077 0x%04X\n", phy_utils_read_phyreg(pi, 0x077));
	printf("acphy: 0x079 0x%04X\n", phy_utils_read_phyreg(pi, 0x079));
	printf("acphy: 0x3b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x3b6));
	printf("acphy: 0x076 0x%04X\n", phy_utils_read_phyreg(pi, 0x076));
	printf("acphy: 0x6b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b5));
	printf("acphy: 0x8b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b5));
	printf("acphy: 0xab5 0x%04X\n", phy_utils_read_phyreg(pi, 0xab5));
	printf("acphy: 0xcb5 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb5));
	printf("acphy: 0x072 0x%04X\n", phy_utils_read_phyreg(pi, 0x072));
	printf("acphy: 0x648 0x%04X\n", phy_utils_read_phyreg(pi, 0x648));
	printf("acphy: 0x848 0x%04X\n", phy_utils_read_phyreg(pi, 0x848));
	printf("acphy: 0xa48 0x%04X\n", phy_utils_read_phyreg(pi, 0xa48));
	printf("acphy: 0xc48 0x%04X\n", phy_utils_read_phyreg(pi, 0xc48));
	printf("acphy: 0x11c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x11c0));
	printf("acphy: 0x11c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x11c1));
	printf("acphy: 0x060 0x%04X\n", phy_utils_read_phyreg(pi, 0x060));
	printf("acphy: 0x063 0x%04X\n", phy_utils_read_phyreg(pi, 0x063));
	printf("acphy: 0x064 0x%04X\n", phy_utils_read_phyreg(pi, 0x064));
	printf("acphy: 0x061 0x%04X\n", phy_utils_read_phyreg(pi, 0x061));
	printf("acphy: 0x062 0x%04X\n", phy_utils_read_phyreg(pi, 0x062));
	printf("acphy: 0x6b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b8));
	printf("acphy: 0x8b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b8));
	printf("acphy: 0xab8 0x%04X\n", phy_utils_read_phyreg(pi, 0xab8));
	printf("acphy: 0xcb8 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb8));
	printf("acphy: 0x43c 0x%04X\n", phy_utils_read_phyreg(pi, 0x43c));
	printf("acphy: 0x43d 0x%04X\n", phy_utils_read_phyreg(pi, 0x43d));
	printf("acphy: 0x43e 0x%04X\n", phy_utils_read_phyreg(pi, 0x43e));
	printf("acphy: 0x439 0x%04X\n", phy_utils_read_phyreg(pi, 0x439));
	printf("acphy: 0x43a 0x%04X\n", phy_utils_read_phyreg(pi, 0x43a));
	printf("acphy: 0x43b 0x%04X\n", phy_utils_read_phyreg(pi, 0x43b));
	printf("acphy: 0x3ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ec));
	printf("acphy: 0x1623 0x%04X\n", phy_utils_read_phyreg(pi, 0x1623));
	printf("acphy: 0x1823 0x%04X\n", phy_utils_read_phyreg(pi, 0x1823));
	printf("acphy: 0x1a23 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a23));
	printf("acphy: 0x1c23 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c23));
	printf("acphy: 0x1624 0x%04X\n", phy_utils_read_phyreg(pi, 0x1624));
	printf("acphy: 0x1824 0x%04X\n", phy_utils_read_phyreg(pi, 0x1824));
	printf("acphy: 0x1a24 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a24));
	printf("acphy: 0x1c24 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c24));
	printf("acphy: 0x1621 0x%04X\n", phy_utils_read_phyreg(pi, 0x1621));
	printf("acphy: 0x1821 0x%04X\n", phy_utils_read_phyreg(pi, 0x1821));
	printf("acphy: 0x1a21 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a21));
	printf("acphy: 0x1c21 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c21));
	printf("acphy: 0x1620 0x%04X\n", phy_utils_read_phyreg(pi, 0x1620));
	printf("acphy: 0x1820 0x%04X\n", phy_utils_read_phyreg(pi, 0x1820));
	printf("acphy: 0x1a20 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a20));
	printf("acphy: 0x1c20 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c20));
	printf("acphy: 0x1622 0x%04X\n", phy_utils_read_phyreg(pi, 0x1622));
	printf("acphy: 0x1822 0x%04X\n", phy_utils_read_phyreg(pi, 0x1822));
	printf("acphy: 0x1a22 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a22));
	printf("acphy: 0x1c22 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c22));
	printf("acphy: 0x055 0x%04X\n", phy_utils_read_phyreg(pi, 0x055));
	printf("acphy: 0x056 0x%04X\n", phy_utils_read_phyreg(pi, 0x056));
	printf("acphy: 0x057 0x%04X\n", phy_utils_read_phyreg(pi, 0x057));
	printf("acphy: 0x3bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x3bc));
	printf("acphy: 0x3bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x3bd));
	printf("acphy: 0x02b 0x%04X\n", phy_utils_read_phyreg(pi, 0x02b));
	printf("acphy: 0x1625 0x%04X\n", phy_utils_read_phyreg(pi, 0x1625));
	printf("acphy: 0x1626 0x%04X\n", phy_utils_read_phyreg(pi, 0x1626));
	printf("acphy: 0x1627 0x%04X\n", phy_utils_read_phyreg(pi, 0x1627));
	printf("acphy: 0x1628 0x%04X\n", phy_utils_read_phyreg(pi, 0x1628));
	printf("acphy: 0x1629 0x%04X\n", phy_utils_read_phyreg(pi, 0x1629));
	printf("acphy: 0x162a 0x%04X\n", phy_utils_read_phyreg(pi, 0x162a));
	printf("acphy: 0x162b 0x%04X\n", phy_utils_read_phyreg(pi, 0x162b));
	printf("acphy: 0x1825 0x%04X\n", phy_utils_read_phyreg(pi, 0x1825));
	printf("acphy: 0x1826 0x%04X\n", phy_utils_read_phyreg(pi, 0x1826));
	printf("acphy: 0x1827 0x%04X\n", phy_utils_read_phyreg(pi, 0x1827));
	printf("acphy: 0x1828 0x%04X\n", phy_utils_read_phyreg(pi, 0x1828));
	printf("acphy: 0x1829 0x%04X\n", phy_utils_read_phyreg(pi, 0x1829));
	printf("acphy: 0x182a 0x%04X\n", phy_utils_read_phyreg(pi, 0x182a));
	printf("acphy: 0x182b 0x%04X\n", phy_utils_read_phyreg(pi, 0x182b));
	printf("acphy: 0x1a25 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a25));
	printf("acphy: 0x1a26 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a26));
	printf("acphy: 0x1a27 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a27));
	printf("acphy: 0x1a28 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a28));
	printf("acphy: 0x1a29 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a29));
	printf("acphy: 0x1a2a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a2a));
	printf("acphy: 0x1a2b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a2b));
	printf("acphy: 0x1c25 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c25));
	printf("acphy: 0x1c26 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c26));
	printf("acphy: 0x1c27 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c27));
	printf("acphy: 0x1c28 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c28));
	printf("acphy: 0x1c29 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c29));
	printf("acphy: 0x1c2a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c2a));
	printf("acphy: 0x1c2b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c2b));
	printf("acphy: 0x065 0x%04X\n", phy_utils_read_phyreg(pi, 0x065));
	printf("acphy: 0x7f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f2));
	printf("acphy: 0x9f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f2));
	printf("acphy: 0xbf2 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf2));
	printf("acphy: 0xdf2 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf2));
	printf("acphy: 0x7f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f3));
	printf("acphy: 0x9f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f3));
	printf("acphy: 0xbf3 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf3));
	printf("acphy: 0xdf3 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf3));
	printf("acphy: 0x7f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f1));
	printf("acphy: 0x9f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f1));
	printf("acphy: 0xbf1 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf1));
	printf("acphy: 0xdf1 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf1));
	printf("acphy: 0x7f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f0));
	printf("acphy: 0x9f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f0));
	printf("acphy: 0xbf0 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf0));
	printf("acphy: 0xdf0 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf0));
	printf("acphy: 0x7f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x7f4));
	printf("acphy: 0x9f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x9f4));
	printf("acphy: 0xbf4 0x%04X\n", phy_utils_read_phyreg(pi, 0xbf4));
	printf("acphy: 0xdf4 0x%04X\n", phy_utils_read_phyreg(pi, 0xdf4));
	printf("acphy: 0x605 0x%04X\n", phy_utils_read_phyreg(pi, 0x605));
	printf("acphy: 0x805 0x%04X\n", phy_utils_read_phyreg(pi, 0x805));
	printf("acphy: 0xa05 0x%04X\n", phy_utils_read_phyreg(pi, 0xa05));
	printf("acphy: 0xc05 0x%04X\n", phy_utils_read_phyreg(pi, 0xc05));
	printf("acphy: 0x60b 0x%04X\n", phy_utils_read_phyreg(pi, 0x60b));
	printf("acphy: 0x80b 0x%04X\n", phy_utils_read_phyreg(pi, 0x80b));
	printf("acphy: 0xa0b 0x%04X\n", phy_utils_read_phyreg(pi, 0xa0b));
	printf("acphy: 0xc0b 0x%04X\n", phy_utils_read_phyreg(pi, 0xc0b));
	printf("acphy: 0x60e 0x%04X\n", phy_utils_read_phyreg(pi, 0x60e));
	printf("acphy: 0x80e 0x%04X\n", phy_utils_read_phyreg(pi, 0x80e));
	printf("acphy: 0xa0e 0x%04X\n", phy_utils_read_phyreg(pi, 0xa0e));
	printf("acphy: 0xc0e 0x%04X\n", phy_utils_read_phyreg(pi, 0xc0e));
	printf("acphy: 0x60c 0x%04X\n", phy_utils_read_phyreg(pi, 0x60c));
	printf("acphy: 0x80c 0x%04X\n", phy_utils_read_phyreg(pi, 0x80c));
	printf("acphy: 0xa0c 0x%04X\n", phy_utils_read_phyreg(pi, 0xa0c));
	printf("acphy: 0xc0c 0x%04X\n", phy_utils_read_phyreg(pi, 0xc0c));
	printf("acphy: 0x604 0x%04X\n", phy_utils_read_phyreg(pi, 0x604));
	printf("acphy: 0x804 0x%04X\n", phy_utils_read_phyreg(pi, 0x804));
	printf("acphy: 0xa04 0x%04X\n", phy_utils_read_phyreg(pi, 0xa04));
	printf("acphy: 0xc04 0x%04X\n", phy_utils_read_phyreg(pi, 0xc04));
	printf("acphy: 0x60a 0x%04X\n", phy_utils_read_phyreg(pi, 0x60a));
	printf("acphy: 0x80a 0x%04X\n", phy_utils_read_phyreg(pi, 0x80a));
	printf("acphy: 0xa0a 0x%04X\n", phy_utils_read_phyreg(pi, 0xa0a));
	printf("acphy: 0xc0a 0x%04X\n", phy_utils_read_phyreg(pi, 0xc0a));
	printf("acphy: 0x60d 0x%04X\n", phy_utils_read_phyreg(pi, 0x60d));
	printf("acphy: 0x80d 0x%04X\n", phy_utils_read_phyreg(pi, 0x80d));
	printf("acphy: 0xa0d 0x%04X\n", phy_utils_read_phyreg(pi, 0xa0d));
	printf("acphy: 0xc0d 0x%04X\n", phy_utils_read_phyreg(pi, 0xc0d));
	printf("acphy: 0x60f 0x%04X\n", phy_utils_read_phyreg(pi, 0x60f));
	printf("acphy: 0x80f 0x%04X\n", phy_utils_read_phyreg(pi, 0x80f));
	printf("acphy: 0xa0f 0x%04X\n", phy_utils_read_phyreg(pi, 0xa0f));
	printf("acphy: 0xc0f 0x%04X\n", phy_utils_read_phyreg(pi, 0xc0f));
	printf("acphy: 0x0b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b9));
	printf("acphy: 0x0b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b0));
	printf("acphy: 0x0b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b1));
	printf("acphy: 0x0b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b2));
	printf("acphy: 0x0b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b3));
	printf("acphy: 0x0b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b4));
	printf("acphy: 0x0b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b5));
	printf("acphy: 0x0b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b6));
	printf("acphy: 0x0b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b7));
	printf("acphy: 0x0b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x0b8));
	printf("acphy: 0x10a 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a));
	printf("acphy: 0x0f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f5));
	printf("acphy: 0x0ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ec));
	printf("acphy: 0x0ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ed));
	printf("acphy: 0x0ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ee));
	printf("acphy: 0x0ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ef));
	printf("acphy: 0x0f0 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f0));
	printf("acphy: 0x0f1 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f1));
	printf("acphy: 0x0f2 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f2));
	printf("acphy: 0x0f3 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f3));
	printf("acphy: 0x0f4 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f4));
	printf("acphy: 0x3ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x3ef));
	printf("acphy: 0x028 0x%04X\n", phy_utils_read_phyreg(pi, 0x028));
	printf("acphy: 0x781 0x%04X\n", phy_utils_read_phyreg(pi, 0x781));
	printf("acphy: 0x981 0x%04X\n", phy_utils_read_phyreg(pi, 0x981));
	printf("acphy: 0xb81 0x%04X\n", phy_utils_read_phyreg(pi, 0xb81));
	printf("acphy: 0xd81 0x%04X\n", phy_utils_read_phyreg(pi, 0xd81));
	printf("acphy: 0x780 0x%04X\n", phy_utils_read_phyreg(pi, 0x780));
	printf("acphy: 0x980 0x%04X\n", phy_utils_read_phyreg(pi, 0x980));
	printf("acphy: 0xb80 0x%04X\n", phy_utils_read_phyreg(pi, 0xb80));
	printf("acphy: 0xd80 0x%04X\n", phy_utils_read_phyreg(pi, 0xd80));
	printf("acphy: 0x058 0x%04X\n", phy_utils_read_phyreg(pi, 0x058));
	printf("acphy: 0x059 0x%04X\n", phy_utils_read_phyreg(pi, 0x059));
	printf("acphy: 0x027 0x%04X\n", phy_utils_read_phyreg(pi, 0x027));
	printf("acphy: 0x020 0x%04X\n", phy_utils_read_phyreg(pi, 0x020));
	printf("acphy: 0x02a 0x%04X\n", phy_utils_read_phyreg(pi, 0x02a));
	printf("acphy: 0x02d 0x%04X\n", phy_utils_read_phyreg(pi, 0x02d));
	printf("acphy: 0x02f 0x%04X\n", phy_utils_read_phyreg(pi, 0x02f));
	printf("acphy: 0x005 0x%04X\n", phy_utils_read_phyreg(pi, 0x005));
	printf("acphy: 0x029 0x%04X\n", phy_utils_read_phyreg(pi, 0x029));
	printf("acphy: 0x02c 0x%04X\n", phy_utils_read_phyreg(pi, 0x02c));
	printf("acphy: 0x02e 0x%04X\n", phy_utils_read_phyreg(pi, 0x02e));
	printf("acphy: 0x021 0x%04X\n", phy_utils_read_phyreg(pi, 0x021));
	printf("acphy: 0x6c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c8));
	printf("acphy: 0x8c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c8));
	printf("acphy: 0xac8 0x%04X\n", phy_utils_read_phyreg(pi, 0xac8));
	printf("acphy: 0xcc8 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc8));
	printf("acphy: 0x6c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x6c9));
	printf("acphy: 0x8c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x8c9));
	printf("acphy: 0xac9 0x%04X\n", phy_utils_read_phyreg(pi, 0xac9));
	printf("acphy: 0xcc9 0x%04X\n", phy_utils_read_phyreg(pi, 0xcc9));
	printf("acphy: 0x69b 0x%04X\n", phy_utils_read_phyreg(pi, 0x69b));
	printf("acphy: 0x89b 0x%04X\n", phy_utils_read_phyreg(pi, 0x89b));
	printf("acphy: 0xa9b 0x%04X\n", phy_utils_read_phyreg(pi, 0xa9b));
	printf("acphy: 0xc9b 0x%04X\n", phy_utils_read_phyreg(pi, 0xc9b));
	printf("acphy: 0x69c 0x%04X\n", phy_utils_read_phyreg(pi, 0x69c));
	printf("acphy: 0x89c 0x%04X\n", phy_utils_read_phyreg(pi, 0x89c));
	printf("acphy: 0xa9c 0x%04X\n", phy_utils_read_phyreg(pi, 0xa9c));
	printf("acphy: 0xc9c 0x%04X\n", phy_utils_read_phyreg(pi, 0xc9c));
	printf("acphy: 0x69d 0x%04X\n", phy_utils_read_phyreg(pi, 0x69d));
	printf("acphy: 0x89d 0x%04X\n", phy_utils_read_phyreg(pi, 0x89d));
	printf("acphy: 0xa9d 0x%04X\n", phy_utils_read_phyreg(pi, 0xa9d));
	printf("acphy: 0xc9d 0x%04X\n", phy_utils_read_phyreg(pi, 0xc9d));
	printf("acphy: 0x69e 0x%04X\n", phy_utils_read_phyreg(pi, 0x69e));
	printf("acphy: 0x89e 0x%04X\n", phy_utils_read_phyreg(pi, 0x89e));
	printf("acphy: 0xa9e 0x%04X\n", phy_utils_read_phyreg(pi, 0xa9e));
	printf("acphy: 0xc9e 0x%04X\n", phy_utils_read_phyreg(pi, 0xc9e));
	printf("acphy: 0x63f 0x%04X\n", phy_utils_read_phyreg(pi, 0x63f));
	printf("acphy: 0x83f 0x%04X\n", phy_utils_read_phyreg(pi, 0x83f));
	printf("acphy: 0xa3f 0x%04X\n", phy_utils_read_phyreg(pi, 0xa3f));
	printf("acphy: 0xc3f 0x%04X\n", phy_utils_read_phyreg(pi, 0xc3f));
	printf("acphy: 0x643 0x%04X\n", phy_utils_read_phyreg(pi, 0x643));
	printf("acphy: 0x843 0x%04X\n", phy_utils_read_phyreg(pi, 0x843));
	printf("acphy: 0xa43 0x%04X\n", phy_utils_read_phyreg(pi, 0xa43));
	printf("acphy: 0xc43 0x%04X\n", phy_utils_read_phyreg(pi, 0xc43));
	printf("acphy: 0x6b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b2));
	printf("acphy: 0x8b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b2));
	printf("acphy: 0xab2 0x%04X\n", phy_utils_read_phyreg(pi, 0xab2));
	printf("acphy: 0xcb2 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb2));
	printf("acphy: 0x6b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b4));
	printf("acphy: 0x8b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b4));
	printf("acphy: 0xab4 0x%04X\n", phy_utils_read_phyreg(pi, 0xab4));
	printf("acphy: 0xcb4 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb4));
	printf("acphy: 0x69f 0x%04X\n", phy_utils_read_phyreg(pi, 0x69f));
	printf("acphy: 0x89f 0x%04X\n", phy_utils_read_phyreg(pi, 0x89f));
	printf("acphy: 0xa9f 0x%04X\n", phy_utils_read_phyreg(pi, 0xa9f));
	printf("acphy: 0xc9f 0x%04X\n", phy_utils_read_phyreg(pi, 0xc9f));
	printf("acphy: 0x6ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ca));
	printf("acphy: 0x8ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ca));
	printf("acphy: 0xaca 0x%04X\n", phy_utils_read_phyreg(pi, 0xaca));
	printf("acphy: 0xcca 0x%04X\n", phy_utils_read_phyreg(pi, 0xcca));
	printf("acphy: 0x6cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x6cb));
	printf("acphy: 0x8cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x8cb));
	printf("acphy: 0xacb 0x%04X\n", phy_utils_read_phyreg(pi, 0xacb));
	printf("acphy: 0xccb 0x%04X\n", phy_utils_read_phyreg(pi, 0xccb));
	printf("acphy: 0x6b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b1));
	printf("acphy: 0x8b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b1));
	printf("acphy: 0xab1 0x%04X\n", phy_utils_read_phyreg(pi, 0xab1));
	printf("acphy: 0xcb1 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb1));
	printf("acphy: 0x6b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b3));
	printf("acphy: 0x8b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b3));
	printf("acphy: 0xab3 0x%04X\n", phy_utils_read_phyreg(pi, 0xab3));
	printf("acphy: 0xcb3 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb3));
	printf("acphy: 0x647 0x%04X\n", phy_utils_read_phyreg(pi, 0x647));
	printf("acphy: 0x847 0x%04X\n", phy_utils_read_phyreg(pi, 0x847));
	printf("acphy: 0xa47 0x%04X\n", phy_utils_read_phyreg(pi, 0xa47));
	printf("acphy: 0xc47 0x%04X\n", phy_utils_read_phyreg(pi, 0xc47));
	printf("acphy: 0x073 0x%04X\n", phy_utils_read_phyreg(pi, 0x073));
	printf("acphy: 0x070 0x%04X\n", phy_utils_read_phyreg(pi, 0x070));
	printf("acphy: 0x641 0x%04X\n", phy_utils_read_phyreg(pi, 0x641));
	printf("acphy: 0x841 0x%04X\n", phy_utils_read_phyreg(pi, 0x841));
	printf("acphy: 0xa41 0x%04X\n", phy_utils_read_phyreg(pi, 0xa41));
	printf("acphy: 0xc41 0x%04X\n", phy_utils_read_phyreg(pi, 0xc41));
	printf("acphy: 0x074 0x%04X\n", phy_utils_read_phyreg(pi, 0x074));
	printf("acphy: 0x07d 0x%04X\n", phy_utils_read_phyreg(pi, 0x07d));
	printf("acphy: 0x64a 0x%04X\n", phy_utils_read_phyreg(pi, 0x64a));
	printf("acphy: 0x84a 0x%04X\n", phy_utils_read_phyreg(pi, 0x84a));
	printf("acphy: 0xa4a 0x%04X\n", phy_utils_read_phyreg(pi, 0xa4a));
	printf("acphy: 0xc4a 0x%04X\n", phy_utils_read_phyreg(pi, 0xc4a));
	printf("acphy: 0x6bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x6bd));
	printf("acphy: 0x8bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x8bd));
	printf("acphy: 0xabd 0x%04X\n", phy_utils_read_phyreg(pi, 0xabd));
	printf("acphy: 0xcbd 0x%04X\n", phy_utils_read_phyreg(pi, 0xcbd));
	printf("acphy: 0x645 0x%04X\n", phy_utils_read_phyreg(pi, 0x645));
	printf("acphy: 0x845 0x%04X\n", phy_utils_read_phyreg(pi, 0x845));
	printf("acphy: 0xa45 0x%04X\n", phy_utils_read_phyreg(pi, 0xa45));
	printf("acphy: 0xc45 0x%04X\n", phy_utils_read_phyreg(pi, 0xc45));
	printf("acphy: 0x64c 0x%04X\n", phy_utils_read_phyreg(pi, 0x64c));
	printf("acphy: 0x84c 0x%04X\n", phy_utils_read_phyreg(pi, 0x84c));
	printf("acphy: 0xa4c 0x%04X\n", phy_utils_read_phyreg(pi, 0xa4c));
	printf("acphy: 0xc4c 0x%04X\n", phy_utils_read_phyreg(pi, 0xc4c));
	printf("acphy: 0x6b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x6b6));
	printf("acphy: 0x8b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x8b6));
	printf("acphy: 0xab6 0x%04X\n", phy_utils_read_phyreg(pi, 0xab6));
	printf("acphy: 0xcb6 0x%04X\n", phy_utils_read_phyreg(pi, 0xcb6));
	printf("acphy: 0x644 0x%04X\n", phy_utils_read_phyreg(pi, 0x644));
	printf("acphy: 0x844 0x%04X\n", phy_utils_read_phyreg(pi, 0x844));
	printf("acphy: 0xa44 0x%04X\n", phy_utils_read_phyreg(pi, 0xa44));
	printf("acphy: 0xc44 0x%04X\n", phy_utils_read_phyreg(pi, 0xc44));
	printf("acphy: 0x071 0x%04X\n", phy_utils_read_phyreg(pi, 0x071));
	printf("acphy: 0x64b 0x%04X\n", phy_utils_read_phyreg(pi, 0x64b));
	printf("acphy: 0x84b 0x%04X\n", phy_utils_read_phyreg(pi, 0x84b));
	printf("acphy: 0xa4b 0x%04X\n", phy_utils_read_phyreg(pi, 0xa4b));
	printf("acphy: 0xc4b 0x%04X\n", phy_utils_read_phyreg(pi, 0xc4b));
	printf("acphy: 0x649 0x%04X\n", phy_utils_read_phyreg(pi, 0x649));
	printf("acphy: 0x849 0x%04X\n", phy_utils_read_phyreg(pi, 0x849));
	printf("acphy: 0xa49 0x%04X\n", phy_utils_read_phyreg(pi, 0xa49));
	printf("acphy: 0xc49 0x%04X\n", phy_utils_read_phyreg(pi, 0xc49));
	printf("acphy: 0x075 0x%04X\n", phy_utils_read_phyreg(pi, 0x075));
	printf("acphy: 0x64f 0x%04X\n", phy_utils_read_phyreg(pi, 0x64f));
	printf("acphy: 0x84f 0x%04X\n", phy_utils_read_phyreg(pi, 0x84f));
	printf("acphy: 0xa4f 0x%04X\n", phy_utils_read_phyreg(pi, 0xa4f));
	printf("acphy: 0xc4f 0x%04X\n", phy_utils_read_phyreg(pi, 0xc4f));
	printf("acphy: 0x640 0x%04X\n", phy_utils_read_phyreg(pi, 0x640));
	printf("acphy: 0x840 0x%04X\n", phy_utils_read_phyreg(pi, 0x840));
	printf("acphy: 0xa40 0x%04X\n", phy_utils_read_phyreg(pi, 0xa40));
	printf("acphy: 0xc40 0x%04X\n", phy_utils_read_phyreg(pi, 0xc40));
	printf("acphy: 0x646 0x%04X\n", phy_utils_read_phyreg(pi, 0x646));
	printf("acphy: 0x846 0x%04X\n", phy_utils_read_phyreg(pi, 0x846));
	printf("acphy: 0xa46 0x%04X\n", phy_utils_read_phyreg(pi, 0xa46));
	printf("acphy: 0xc46 0x%04X\n", phy_utils_read_phyreg(pi, 0xc46));
	printf("acphy: 0x07b 0x%04X\n", phy_utils_read_phyreg(pi, 0x07b));
	printf("acphy: 0x026 0x%04X\n", phy_utils_read_phyreg(pi, 0x026));
	printf("acphy: 0x601 0x%04X\n", phy_utils_read_phyreg(pi, 0x601));
	printf("acphy: 0x801 0x%04X\n", phy_utils_read_phyreg(pi, 0x801));
	printf("acphy: 0xa01 0x%04X\n", phy_utils_read_phyreg(pi, 0xa01));
	printf("acphy: 0xc01 0x%04X\n", phy_utils_read_phyreg(pi, 0xc01));
	printf("acphy: 0x603 0x%04X\n", phy_utils_read_phyreg(pi, 0x603));
	printf("acphy: 0x602 0x%04X\n", phy_utils_read_phyreg(pi, 0x602));
	printf("acphy: 0x803 0x%04X\n", phy_utils_read_phyreg(pi, 0x803));
	printf("acphy: 0x802 0x%04X\n", phy_utils_read_phyreg(pi, 0x802));
	printf("acphy: 0xa03 0x%04X\n", phy_utils_read_phyreg(pi, 0xa03));
	printf("acphy: 0xa02 0x%04X\n", phy_utils_read_phyreg(pi, 0xa02));
	printf("acphy: 0xc03 0x%04X\n", phy_utils_read_phyreg(pi, 0xc03));
	printf("acphy: 0xc02 0x%04X\n", phy_utils_read_phyreg(pi, 0xc02));
	printf("acphy: 0x607 0x%04X\n", phy_utils_read_phyreg(pi, 0x607));
	printf("acphy: 0x606 0x%04X\n", phy_utils_read_phyreg(pi, 0x606));
	printf("acphy: 0x807 0x%04X\n", phy_utils_read_phyreg(pi, 0x807));
	printf("acphy: 0x806 0x%04X\n", phy_utils_read_phyreg(pi, 0x806));
	printf("acphy: 0xa07 0x%04X\n", phy_utils_read_phyreg(pi, 0xa07));
	printf("acphy: 0xa06 0x%04X\n", phy_utils_read_phyreg(pi, 0xa06));
	printf("acphy: 0xc07 0x%04X\n", phy_utils_read_phyreg(pi, 0xc07));
	printf("acphy: 0xc06 0x%04X\n", phy_utils_read_phyreg(pi, 0xc06));
	printf("acphy: 0x608 0x%04X\n", phy_utils_read_phyreg(pi, 0x608));
	printf("acphy: 0x808 0x%04X\n", phy_utils_read_phyreg(pi, 0x808));
	printf("acphy: 0xa08 0x%04X\n", phy_utils_read_phyreg(pi, 0xa08));
	printf("acphy: 0xc08 0x%04X\n", phy_utils_read_phyreg(pi, 0xc08));
	printf("acphy: 0x025 0x%04X\n", phy_utils_read_phyreg(pi, 0x025));
	printf("acphy: 0x022 0x%04X\n", phy_utils_read_phyreg(pi, 0x022));
	printf("acphy: 0x049 0x%04X\n", phy_utils_read_phyreg(pi, 0x049));
	printf("acphy: 0x04a 0x%04X\n", phy_utils_read_phyreg(pi, 0x04a));
	printf("acphy: 0x11b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b4));
	printf("acphy: 0x11b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b5));
	printf("acphy: 0x11b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b6));
	printf("acphy: 0x11b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b7));
	printf("acphy: 0x11b8 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b8));
	printf("acphy: 0x11b9 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b9));
	printf("acphy: 0x04b 0x%04X\n", phy_utils_read_phyreg(pi, 0x04b));
	printf("acphy: 0x04c 0x%04X\n", phy_utils_read_phyreg(pi, 0x04c));
	printf("acphy: 0x04d 0x%04X\n", phy_utils_read_phyreg(pi, 0x04d));
	printf("acphy: 0x04e 0x%04X\n", phy_utils_read_phyreg(pi, 0x04e));
	printf("acphy: 0x11b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b0));
	printf("acphy: 0x11b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b1));
	printf("acphy: 0x11b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b2));
	printf("acphy: 0x11b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x11b3));
	printf("acphy: 0x045 0x%04X\n", phy_utils_read_phyreg(pi, 0x045));
	printf("acphy: 0x791 0x%04X\n", phy_utils_read_phyreg(pi, 0x791));
	printf("acphy: 0x991 0x%04X\n", phy_utils_read_phyreg(pi, 0x991));
	printf("acphy: 0xb91 0x%04X\n", phy_utils_read_phyreg(pi, 0xb91));
	printf("acphy: 0xd91 0x%04X\n", phy_utils_read_phyreg(pi, 0xd91));
	printf("acphy: 0x790 0x%04X\n", phy_utils_read_phyreg(pi, 0x790));
	printf("acphy: 0x990 0x%04X\n", phy_utils_read_phyreg(pi, 0x990));
	printf("acphy: 0xb90 0x%04X\n", phy_utils_read_phyreg(pi, 0xb90));
	printf("acphy: 0xd90 0x%04X\n", phy_utils_read_phyreg(pi, 0xd90));
	printf("acphy: 0x11c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x11c2));
	printf("acphy: 0x05a 0x%04X\n", phy_utils_read_phyreg(pi, 0x05a));
	printf("acphy: 0x05b 0x%04X\n", phy_utils_read_phyreg(pi, 0x05b));
	printf("acphy: 0x05c 0x%04X\n", phy_utils_read_phyreg(pi, 0x05c));
	printf("acphy: 0x05d 0x%04X\n", phy_utils_read_phyreg(pi, 0x05d));
	printf("acphy: 0x038 0x%04X\n", phy_utils_read_phyreg(pi, 0x038));
	printf("acphy: 0x03a 0x%04X\n", phy_utils_read_phyreg(pi, 0x03a));
	printf("acphy: 0x03c 0x%04X\n", phy_utils_read_phyreg(pi, 0x03c));
	printf("acphy: 0x03e 0x%04X\n", phy_utils_read_phyreg(pi, 0x03e));
	printf("acphy: 0x039 0x%04X\n", phy_utils_read_phyreg(pi, 0x039));
	printf("acphy: 0x03b 0x%04X\n", phy_utils_read_phyreg(pi, 0x03b));
	printf("acphy: 0x03d 0x%04X\n", phy_utils_read_phyreg(pi, 0x03d));
	printf("acphy: 0x03f 0x%04X\n", phy_utils_read_phyreg(pi, 0x03f));
	printf("acphy: 0x00a 0x%04X\n", phy_utils_read_phyreg(pi, 0x00a));
	printf("acphy: 0x054 0x%04X\n", phy_utils_read_phyreg(pi, 0x054));
	printf("acphy: 0x104a 0x%04X\n", phy_utils_read_phyreg(pi, 0x104a));
	printf("acphy: 0x104b 0x%04X\n", phy_utils_read_phyreg(pi, 0x104b));
	printf("acphy: 0x1051 0x%04X\n", phy_utils_read_phyreg(pi, 0x1051));
	printf("acphy: 0x1052 0x%04X\n", phy_utils_read_phyreg(pi, 0x1052));
	printf("acphy: 0x1049 0x%04X\n", phy_utils_read_phyreg(pi, 0x1049));
	printf("acphy: 0x1113 0x%04X\n", phy_utils_read_phyreg(pi, 0x1113));
	printf("acphy: 0x1114 0x%04X\n", phy_utils_read_phyreg(pi, 0x1114));
	printf("acphy: 0x10e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e9));
	printf("acphy: 0x10e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x10e8));
	printf("acphy: 0x10eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x10eb));
	printf("acphy: 0x10ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ea));
	printf("acphy: 0x10ec 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ec));
	printf("acphy: 0x10ed 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ed));
	printf("acphy: 0x10ef 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ef));
	printf("acphy: 0x10ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x10ee));
	printf("acphy: 0x1119 0x%04X\n", phy_utils_read_phyreg(pi, 0x1119));
	printf("acphy: 0x158 0x%04X\n", phy_utils_read_phyreg(pi, 0x158));
	printf("acphy: 0x157 0x%04X\n", phy_utils_read_phyreg(pi, 0x157));
	printf("acphy: 0x024 0x%04X\n", phy_utils_read_phyreg(pi, 0x024));
	printf("acphy: 0x1cc 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cc));
	printf("acphy: 0x1ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca));
	printf("acphy: 0x1cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb));
	printf("acphy: 0x1d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d8));
	printf("acphy: 0x1d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d9));
	printf("acphy: 0x1da 0x%04X\n", phy_utils_read_phyreg(pi, 0x1da));
	printf("acphy: 0x1db 0x%04X\n", phy_utils_read_phyreg(pi, 0x1db));
	printf("acphy: 0x1dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x1dc));
	printf("acphy: 0x1dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x1dd));
	printf("acphy: 0x529 0x%04X\n", phy_utils_read_phyreg(pi, 0x529));
	printf("acphy: 0x528 0x%04X\n", phy_utils_read_phyreg(pi, 0x528));
	printf("acphy: 0x527 0x%04X\n", phy_utils_read_phyreg(pi, 0x527));
	printf("acphy: 0x526 0x%04X\n", phy_utils_read_phyreg(pi, 0x526));
	printf("acphy: 0x525 0x%04X\n", phy_utils_read_phyreg(pi, 0x525));
	printf("acphy: 0x524 0x%04X\n", phy_utils_read_phyreg(pi, 0x524));
	printf("acphy: 0x58f 0x%04X\n", phy_utils_read_phyreg(pi, 0x58f));
	printf("acphy: 0x103e 0x%04X\n", phy_utils_read_phyreg(pi, 0x103e));
	printf("acphy: 0x1091 0x%04X\n", phy_utils_read_phyreg(pi, 0x1091));
	printf("acphy: 0x2d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d2));
	printf("acphy: 0x2d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d5));
	printf("acphy: 0x112c 0x%04X\n", phy_utils_read_phyreg(pi, 0x112c));
	printf("acphy: 0x10fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x10fa));
	printf("acphy: 0x10fb 0x%04X\n", phy_utils_read_phyreg(pi, 0x10fb));
	printf("acphy: 0x1126 0x%04X\n", phy_utils_read_phyreg(pi, 0x1126));
	printf("acphy: 0x1127 0x%04X\n", phy_utils_read_phyreg(pi, 0x1127));
	printf("acphy: 0x441 0x%04X\n", phy_utils_read_phyreg(pi, 0x441));
	printf("acphy: 0x43f 0x%04X\n", phy_utils_read_phyreg(pi, 0x43f));
	printf("acphy: 0x440 0x%04X\n", phy_utils_read_phyreg(pi, 0x440));
	printf("acphy: 0x10f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f7));
	printf("acphy: 0x10f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f8));
	printf("acphy: 0x10f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x10f9));
	printf("acphy: 0x10fc 0x%04X\n", phy_utils_read_phyreg(pi, 0x10fc));
	printf("acphy: 0x650 0x%04X\n", phy_utils_read_phyreg(pi, 0x650));
	printf("acphy: 0x850 0x%04X\n", phy_utils_read_phyreg(pi, 0x850));
	printf("acphy: 0xa50 0x%04X\n", phy_utils_read_phyreg(pi, 0xa50));
	printf("acphy: 0xc50 0x%04X\n", phy_utils_read_phyreg(pi, 0xc50));
	printf("acphy: 0x200 0x%04X\n", phy_utils_read_phyreg(pi, 0x200));
	printf("acphy: 0x201 0x%04X\n", phy_utils_read_phyreg(pi, 0x201));
	printf("acphy: 0x202 0x%04X\n", phy_utils_read_phyreg(pi, 0x202));
	printf("acphy: 0x16a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a7));
	printf("acphy: 0x18a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a7));
	printf("acphy: 0x1aa7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa7));
	printf("acphy: 0x16a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a8));
	printf("acphy: 0x18a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a8));
	printf("acphy: 0x1aa8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa8));
	printf("acphy: 0x1ca8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca8));
	printf("acphy: 0x1ca7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca7));
	printf("acphy: 0x16a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a9));
	printf("acphy: 0x18a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a9));
	printf("acphy: 0x1aa9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa9));
	printf("acphy: 0x1ca9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca9));
	printf("acphy: 0x16b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b0));
	printf("acphy: 0x18b0 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b0));
	printf("acphy: 0x1ab0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab0));
	printf("acphy: 0x1cb0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb0));
	printf("acphy: 0x16b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b4));
	printf("acphy: 0x18b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b4));
	printf("acphy: 0x1ab4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab4));
	printf("acphy: 0x1cb4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb4));
	printf("acphy: 0x16b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b5));
	printf("acphy: 0x18b5 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b5));
	printf("acphy: 0x1ab5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab5));
	printf("acphy: 0x1cb5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb5));
	printf("acphy: 0x16b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b6));
	printf("acphy: 0x18b6 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b6));
	printf("acphy: 0x1ab6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab6));
	printf("acphy: 0x1cb6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb6));
	printf("acphy: 0x16b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b7));
	printf("acphy: 0x18b7 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b7));
	printf("acphy: 0x1ab7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab7));
	printf("acphy: 0x1cb7 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb7));
	printf("acphy: 0x162 0x%04X\n", phy_utils_read_phyreg(pi, 0x162));
	printf("acphy: 0x001 0x%04X\n", phy_utils_read_phyreg(pi, 0x001));
	printf("acphy: 0x496 0x%04X\n", phy_utils_read_phyreg(pi, 0x496));
	printf("acphy: 0x2a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a5));
	printf("acphy: 0x2a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a6));
	printf("acphy: 0x2a7 0x%04X\n", phy_utils_read_phyreg(pi, 0x2a7));
	printf("acphy: 0x6e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e6));
	printf("acphy: 0x1737 0x%04X\n", phy_utils_read_phyreg(pi, 0x1737));
	printf("acphy: 0x6f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f8));
	printf("acphy: 0x6e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e9));
	printf("acphy: 0x6d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d2));
	printf("acphy: 0x6da 0x%04X\n", phy_utils_read_phyreg(pi, 0x6da));
	printf("acphy: 0x6e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e4));
	printf("acphy: 0x172b 0x%04X\n", phy_utils_read_phyreg(pi, 0x172b));
	printf("acphy: 0x6e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e5));
	printf("acphy: 0x172c 0x%04X\n", phy_utils_read_phyreg(pi, 0x172c));
	printf("acphy: 0x6db 0x%04X\n", phy_utils_read_phyreg(pi, 0x6db));
	printf("acphy: 0x16b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b1));
	printf("acphy: 0x16b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b2));
	printf("acphy: 0x172d 0x%04X\n", phy_utils_read_phyreg(pi, 0x172d));
	printf("acphy: 0x16b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x16b3));
	printf("acphy: 0x6de 0x%04X\n", phy_utils_read_phyreg(pi, 0x6de));
	printf("acphy: 0x1725 0x%04X\n", phy_utils_read_phyreg(pi, 0x1725));
	printf("acphy: 0x6df 0x%04X\n", phy_utils_read_phyreg(pi, 0x6df));
	printf("acphy: 0x1726 0x%04X\n", phy_utils_read_phyreg(pi, 0x1726));
	printf("acphy: 0x6e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e2));
	printf("acphy: 0x1729 0x%04X\n", phy_utils_read_phyreg(pi, 0x1729));
	printf("acphy: 0x6e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e3));
	printf("acphy: 0x172a 0x%04X\n", phy_utils_read_phyreg(pi, 0x172a));
	printf("acphy: 0x6e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e0));
	printf("acphy: 0x1727 0x%04X\n", phy_utils_read_phyreg(pi, 0x1727));
	printf("acphy: 0x6e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e1));
	printf("acphy: 0x1728 0x%04X\n", phy_utils_read_phyreg(pi, 0x1728));
	printf("acphy: 0x6fe 0x%04X\n", phy_utils_read_phyreg(pi, 0x6fe));
	printf("acphy: 0x172e 0x%04X\n", phy_utils_read_phyreg(pi, 0x172e));
	printf("acphy: 0x6e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e8));
	printf("acphy: 0x172f 0x%04X\n", phy_utils_read_phyreg(pi, 0x172f));
	printf("acphy: 0x6dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x6dc));
	printf("acphy: 0x1723 0x%04X\n", phy_utils_read_phyreg(pi, 0x1723));
	printf("acphy: 0x6dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x6dd));
	printf("acphy: 0x1724 0x%04X\n", phy_utils_read_phyreg(pi, 0x1724));
	printf("acphy: 0x6e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x6e7));
	printf("acphy: 0x6ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ee));
	printf("acphy: 0x8e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e6));
	printf("acphy: 0x1937 0x%04X\n", phy_utils_read_phyreg(pi, 0x1937));
	printf("acphy: 0x8f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f8));
	printf("acphy: 0x8e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e9));
	printf("acphy: 0x8d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d2));
	printf("acphy: 0x8da 0x%04X\n", phy_utils_read_phyreg(pi, 0x8da));
	printf("acphy: 0x8e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e4));
	printf("acphy: 0x192b 0x%04X\n", phy_utils_read_phyreg(pi, 0x192b));
	printf("acphy: 0x8e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e5));
	printf("acphy: 0x192c 0x%04X\n", phy_utils_read_phyreg(pi, 0x192c));
	printf("acphy: 0x8db 0x%04X\n", phy_utils_read_phyreg(pi, 0x8db));
	printf("acphy: 0x18b1 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b1));
	printf("acphy: 0x18b2 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b2));
	printf("acphy: 0x192d 0x%04X\n", phy_utils_read_phyreg(pi, 0x192d));
	printf("acphy: 0x18b3 0x%04X\n", phy_utils_read_phyreg(pi, 0x18b3));
	printf("acphy: 0x8de 0x%04X\n", phy_utils_read_phyreg(pi, 0x8de));
	printf("acphy: 0x1925 0x%04X\n", phy_utils_read_phyreg(pi, 0x1925));
	printf("acphy: 0x8df 0x%04X\n", phy_utils_read_phyreg(pi, 0x8df));
	printf("acphy: 0x1926 0x%04X\n", phy_utils_read_phyreg(pi, 0x1926));
	printf("acphy: 0x8e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e2));
	printf("acphy: 0x1929 0x%04X\n", phy_utils_read_phyreg(pi, 0x1929));
	printf("acphy: 0x8e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e3));
	printf("acphy: 0x192a 0x%04X\n", phy_utils_read_phyreg(pi, 0x192a));
	printf("acphy: 0x8e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e0));
	printf("acphy: 0x1927 0x%04X\n", phy_utils_read_phyreg(pi, 0x1927));
	printf("acphy: 0x8e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e1));
	printf("acphy: 0x1928 0x%04X\n", phy_utils_read_phyreg(pi, 0x1928));
	printf("acphy: 0x8fe 0x%04X\n", phy_utils_read_phyreg(pi, 0x8fe));
	printf("acphy: 0x192e 0x%04X\n", phy_utils_read_phyreg(pi, 0x192e));
	printf("acphy: 0x8e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e8));
	printf("acphy: 0x192f 0x%04X\n", phy_utils_read_phyreg(pi, 0x192f));
	printf("acphy: 0x8dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x8dc));
	printf("acphy: 0x1923 0x%04X\n", phy_utils_read_phyreg(pi, 0x1923));
	printf("acphy: 0x8dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x8dd));
	printf("acphy: 0x1924 0x%04X\n", phy_utils_read_phyreg(pi, 0x1924));
	printf("acphy: 0x8e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x8e7));
	printf("acphy: 0x8ee 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ee));
	printf("acphy: 0xae6 0x%04X\n", phy_utils_read_phyreg(pi, 0xae6));
	printf("acphy: 0x1b37 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b37));
	printf("acphy: 0xaf8 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf8));
	printf("acphy: 0xae9 0x%04X\n", phy_utils_read_phyreg(pi, 0xae9));
	printf("acphy: 0xad2 0x%04X\n", phy_utils_read_phyreg(pi, 0xad2));
	printf("acphy: 0xada 0x%04X\n", phy_utils_read_phyreg(pi, 0xada));
	printf("acphy: 0xae4 0x%04X\n", phy_utils_read_phyreg(pi, 0xae4));
	printf("acphy: 0x1b2b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b2b));
	printf("acphy: 0xae5 0x%04X\n", phy_utils_read_phyreg(pi, 0xae5));
	printf("acphy: 0x1b2c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b2c));
	printf("acphy: 0xadb 0x%04X\n", phy_utils_read_phyreg(pi, 0xadb));
	printf("acphy: 0x1ab1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab1));
	printf("acphy: 0x1ab2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab2));
	printf("acphy: 0x1b2d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b2d));
	printf("acphy: 0x1ab3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab3));
	printf("acphy: 0xade 0x%04X\n", phy_utils_read_phyreg(pi, 0xade));
	printf("acphy: 0x1b25 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b25));
	printf("acphy: 0xadf 0x%04X\n", phy_utils_read_phyreg(pi, 0xadf));
	printf("acphy: 0x1b26 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b26));
	printf("acphy: 0xae2 0x%04X\n", phy_utils_read_phyreg(pi, 0xae2));
	printf("acphy: 0x1b29 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b29));
	printf("acphy: 0xae3 0x%04X\n", phy_utils_read_phyreg(pi, 0xae3));
	printf("acphy: 0x1b2a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b2a));
	printf("acphy: 0xae0 0x%04X\n", phy_utils_read_phyreg(pi, 0xae0));
	printf("acphy: 0x1b27 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b27));
	printf("acphy: 0xae1 0x%04X\n", phy_utils_read_phyreg(pi, 0xae1));
	printf("acphy: 0x1b28 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b28));
	printf("acphy: 0xafe 0x%04X\n", phy_utils_read_phyreg(pi, 0xafe));
	printf("acphy: 0x1b2e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b2e));
	printf("acphy: 0xae8 0x%04X\n", phy_utils_read_phyreg(pi, 0xae8));
	printf("acphy: 0x1b2f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b2f));
	printf("acphy: 0xadc 0x%04X\n", phy_utils_read_phyreg(pi, 0xadc));
	printf("acphy: 0x1b23 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b23));
	printf("acphy: 0xadd 0x%04X\n", phy_utils_read_phyreg(pi, 0xadd));
	printf("acphy: 0x1b24 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b24));
	printf("acphy: 0xae7 0x%04X\n", phy_utils_read_phyreg(pi, 0xae7));
	printf("acphy: 0xaee 0x%04X\n", phy_utils_read_phyreg(pi, 0xaee));
	printf("acphy: 0xce6 0x%04X\n", phy_utils_read_phyreg(pi, 0xce6));
	printf("acphy: 0x1d37 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d37));
	printf("acphy: 0xcf8 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf8));
	printf("acphy: 0xce9 0x%04X\n", phy_utils_read_phyreg(pi, 0xce9));
	printf("acphy: 0xcd2 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd2));
	printf("acphy: 0xcda 0x%04X\n", phy_utils_read_phyreg(pi, 0xcda));
	printf("acphy: 0xce4 0x%04X\n", phy_utils_read_phyreg(pi, 0xce4));
	printf("acphy: 0x1d2b 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d2b));
	printf("acphy: 0xce5 0x%04X\n", phy_utils_read_phyreg(pi, 0xce5));
	printf("acphy: 0x1d2c 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d2c));
	printf("acphy: 0xcdb 0x%04X\n", phy_utils_read_phyreg(pi, 0xcdb));
	printf("acphy: 0x1cb1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb1));
	printf("acphy: 0x1cb2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb2));
	printf("acphy: 0x1d2d 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d2d));
	printf("acphy: 0x1cb3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1cb3));
	printf("acphy: 0xcde 0x%04X\n", phy_utils_read_phyreg(pi, 0xcde));
	printf("acphy: 0x1d25 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d25));
	printf("acphy: 0xcdf 0x%04X\n", phy_utils_read_phyreg(pi, 0xcdf));
	printf("acphy: 0x1d26 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d26));
	printf("acphy: 0xce2 0x%04X\n", phy_utils_read_phyreg(pi, 0xce2));
	printf("acphy: 0x1d29 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d29));
	printf("acphy: 0xce3 0x%04X\n", phy_utils_read_phyreg(pi, 0xce3));
	printf("acphy: 0x1d2a 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d2a));
	printf("acphy: 0xce0 0x%04X\n", phy_utils_read_phyreg(pi, 0xce0));
	printf("acphy: 0x1d27 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d27));
	printf("acphy: 0xce1 0x%04X\n", phy_utils_read_phyreg(pi, 0xce1));
	printf("acphy: 0x1d28 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d28));
	printf("acphy: 0xcfe 0x%04X\n", phy_utils_read_phyreg(pi, 0xcfe));
	printf("acphy: 0x1d2e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d2e));
	printf("acphy: 0xce8 0x%04X\n", phy_utils_read_phyreg(pi, 0xce8));
	printf("acphy: 0x1d2f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d2f));
	printf("acphy: 0xcdc 0x%04X\n", phy_utils_read_phyreg(pi, 0xcdc));
	printf("acphy: 0x1d23 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d23));
	printf("acphy: 0xcdd 0x%04X\n", phy_utils_read_phyreg(pi, 0xcdd));
	printf("acphy: 0x1d24 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d24));
	printf("acphy: 0xce7 0x%04X\n", phy_utils_read_phyreg(pi, 0xce7));
	printf("acphy: 0xcee 0x%04X\n", phy_utils_read_phyreg(pi, 0xcee));
	printf("acphy: 0x28a 0x%04X\n", phy_utils_read_phyreg(pi, 0x28a));
	printf("acphy: 0x28b 0x%04X\n", phy_utils_read_phyreg(pi, 0x28b));
	printf("acphy: 0x1664 0x%04X\n", phy_utils_read_phyreg(pi, 0x1664));
	printf("acphy: 0x1864 0x%04X\n", phy_utils_read_phyreg(pi, 0x1864));
	printf("acphy: 0x1a64 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a64));
	printf("acphy: 0x1c64 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c64));
	printf("acphy: 0x1666 0x%04X\n", phy_utils_read_phyreg(pi, 0x1666));
	printf("acphy: 0x1866 0x%04X\n", phy_utils_read_phyreg(pi, 0x1866));
	printf("acphy: 0x1a66 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a66));
	printf("acphy: 0x1c66 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c66));
	printf("acphy: 0x1665 0x%04X\n", phy_utils_read_phyreg(pi, 0x1665));
	printf("acphy: 0x1865 0x%04X\n", phy_utils_read_phyreg(pi, 0x1865));
	printf("acphy: 0x1a65 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a65));
	printf("acphy: 0x1c65 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c65));
	printf("acphy: 0x1667 0x%04X\n", phy_utils_read_phyreg(pi, 0x1667));
	printf("acphy: 0x1867 0x%04X\n", phy_utils_read_phyreg(pi, 0x1867));
	printf("acphy: 0x1a67 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a67));
	printf("acphy: 0x1c67 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c67));
	printf("acphy: 0x1029 0x%04X\n", phy_utils_read_phyreg(pi, 0x1029));
	printf("acphy: 0x102a 0x%04X\n", phy_utils_read_phyreg(pi, 0x102a));
	printf("acphy: 0x102b 0x%04X\n", phy_utils_read_phyreg(pi, 0x102b));
	printf("acphy: 0x7c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c1));
	printf("acphy: 0x9c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c1));
	printf("acphy: 0xbc1 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc1));
	printf("acphy: 0xdc1 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc1));
	printf("acphy: 0x755 0x%04X\n", phy_utils_read_phyreg(pi, 0x755));
	printf("acphy: 0x955 0x%04X\n", phy_utils_read_phyreg(pi, 0x955));
	printf("acphy: 0xb55 0x%04X\n", phy_utils_read_phyreg(pi, 0xb55));
	printf("acphy: 0xd55 0x%04X\n", phy_utils_read_phyreg(pi, 0xd55));
	printf("acphy: 0x7c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c2));
	printf("acphy: 0x9c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c2));
	printf("acphy: 0xbc2 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc2));
	printf("acphy: 0xdc2 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc2));
	printf("acphy: 0x7c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x7c3));
	printf("acphy: 0x9c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x9c3));
	printf("acphy: 0xbc3 0x%04X\n", phy_utils_read_phyreg(pi, 0xbc3));
	printf("acphy: 0xdc3 0x%04X\n", phy_utils_read_phyreg(pi, 0xdc3));
	printf("acphy: 0x1705 0x%04X\n", phy_utils_read_phyreg(pi, 0x1705));
	printf("acphy: 0x1905 0x%04X\n", phy_utils_read_phyreg(pi, 0x1905));
	printf("acphy: 0x1b05 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b05));
	printf("acphy: 0x1d05 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d05));
	printf("acphy: 0x1706 0x%04X\n", phy_utils_read_phyreg(pi, 0x1706));
	printf("acphy: 0x1906 0x%04X\n", phy_utils_read_phyreg(pi, 0x1906));
	printf("acphy: 0x1b06 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b06));
	printf("acphy: 0x1d06 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d06));
	printf("acphy: 0x1707 0x%04X\n", phy_utils_read_phyreg(pi, 0x1707));
	printf("acphy: 0x1907 0x%04X\n", phy_utils_read_phyreg(pi, 0x1907));
	printf("acphy: 0x1b07 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b07));
	printf("acphy: 0x1d07 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d07));
	printf("acphy: 0x1708 0x%04X\n", phy_utils_read_phyreg(pi, 0x1708));
	printf("acphy: 0x1908 0x%04X\n", phy_utils_read_phyreg(pi, 0x1908));
	printf("acphy: 0x1b08 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b08));
	printf("acphy: 0x1d08 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d08));
	printf("acphy: 0x1709 0x%04X\n", phy_utils_read_phyreg(pi, 0x1709));
	printf("acphy: 0x1909 0x%04X\n", phy_utils_read_phyreg(pi, 0x1909));
	printf("acphy: 0x1b09 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b09));
	printf("acphy: 0x1d09 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d09));
	printf("acphy: 0x170f 0x%04X\n", phy_utils_read_phyreg(pi, 0x170f));
	printf("acphy: 0x190f 0x%04X\n", phy_utils_read_phyreg(pi, 0x190f));
	printf("acphy: 0x1b0f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b0f));
	printf("acphy: 0x1d0f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d0f));
	printf("acphy: 0x1710 0x%04X\n", phy_utils_read_phyreg(pi, 0x1710));
	printf("acphy: 0x1910 0x%04X\n", phy_utils_read_phyreg(pi, 0x1910));
	printf("acphy: 0x1b10 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b10));
	printf("acphy: 0x1d10 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d10));
	printf("acphy: 0x1711 0x%04X\n", phy_utils_read_phyreg(pi, 0x1711));
	printf("acphy: 0x1911 0x%04X\n", phy_utils_read_phyreg(pi, 0x1911));
	printf("acphy: 0x1b11 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b11));
	printf("acphy: 0x1d11 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d11));
	printf("acphy: 0x1712 0x%04X\n", phy_utils_read_phyreg(pi, 0x1712));
	printf("acphy: 0x1912 0x%04X\n", phy_utils_read_phyreg(pi, 0x1912));
	printf("acphy: 0x1b12 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b12));
	printf("acphy: 0x1d12 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d12));
	printf("acphy: 0x59d 0x%04X\n", phy_utils_read_phyreg(pi, 0x59d));
	printf("acphy: 0x180 0x%04X\n", phy_utils_read_phyreg(pi, 0x180));
	printf("acphy: 0x198 0x%04X\n", phy_utils_read_phyreg(pi, 0x198));
	printf("acphy: 0x1f5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1f5));
	printf("acphy: 0x1e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1e8));
	printf("acphy: 0x2e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e0));
	printf("acphy: 0x5d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d1));
	printf("acphy: 0x5d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d4));
	printf("acphy: 0x5d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d5));
	printf("acphy: 0x5d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x5d6));
	printf("acphy: 0x1b4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b4));
	printf("acphy: 0x480 0x%04X\n", phy_utils_read_phyreg(pi, 0x480));
	printf("acphy: 0x482 0x%04X\n", phy_utils_read_phyreg(pi, 0x482));
	printf("acphy: 0x370 0x%04X\n", phy_utils_read_phyreg(pi, 0x370));
	printf("acphy: 0x7dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x7dc));
	printf("acphy: 0x9dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x9dc));
	printf("acphy: 0xbdc 0x%04X\n", phy_utils_read_phyreg(pi, 0xbdc));
	printf("acphy: 0xddc 0x%04X\n", phy_utils_read_phyreg(pi, 0xddc));
	printf("acphy: 0x26b 0x%04X\n", phy_utils_read_phyreg(pi, 0x26b));
	printf("acphy: 0x117b 0x%04X\n", phy_utils_read_phyreg(pi, 0x117b));
	printf("acphy: 0x164 0x%04X\n", phy_utils_read_phyreg(pi, 0x164));
	printf("acphy: 0x6f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x6f7));
	printf("acphy: 0x8f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x8f7));
	printf("acphy: 0xaf7 0x%04X\n", phy_utils_read_phyreg(pi, 0xaf7));
	printf("acphy: 0xcf7 0x%04X\n", phy_utils_read_phyreg(pi, 0xcf7));
	printf("acphy: 0x16a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a0));
	printf("acphy: 0x18a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a0));
	printf("acphy: 0x1aa0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa0));
	printf("acphy: 0x1ca0 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca0));
	printf("acphy: 0x16a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a3));
	printf("acphy: 0x18a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a3));
	printf("acphy: 0x1aa3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa3));
	printf("acphy: 0x1ca3 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca3));
	printf("acphy: 0x16a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a4));
	printf("acphy: 0x18a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a4));
	printf("acphy: 0x1aa4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa4));
	printf("acphy: 0x1ca4 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca4));
	printf("acphy: 0x16a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a5));
	printf("acphy: 0x18a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a5));
	printf("acphy: 0x1aa5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa5));
	printf("acphy: 0x1ca5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca5));
	printf("acphy: 0x16a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a6));
	printf("acphy: 0x18a6 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a6));
	printf("acphy: 0x1aa6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa6));
	printf("acphy: 0x1ca6 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca6));
	printf("acphy: 0x16a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a1));
	printf("acphy: 0x18a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a1));
	printf("acphy: 0x1aa1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa1));
	printf("acphy: 0x1ca1 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca1));
	printf("acphy: 0x16a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x16a2));
	printf("acphy: 0x18a2 0x%04X\n", phy_utils_read_phyreg(pi, 0x18a2));
	printf("acphy: 0x1aa2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa2));
	printf("acphy: 0x1ca2 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ca2));
	printf("acphy: 0x10a3 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a3));
	printf("acphy: 0x289 0x%04X\n", phy_utils_read_phyreg(pi, 0x289));
	printf("acphy: 0x35c 0x%04X\n", phy_utils_read_phyreg(pi, 0x35c));
	printf("acphy: 0x6fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x6fa));
	printf("acphy: 0x79d 0x%04X\n", phy_utils_read_phyreg(pi, 0x79d));
	printf("acphy: 0x6d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x6d6));
	printf("acphy: 0x1722 0x%04X\n", phy_utils_read_phyreg(pi, 0x1722));
	printf("acphy: 0x8fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x8fa));
	printf("acphy: 0x99d 0x%04X\n", phy_utils_read_phyreg(pi, 0x99d));
	printf("acphy: 0x8d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x8d6));
	printf("acphy: 0x1922 0x%04X\n", phy_utils_read_phyreg(pi, 0x1922));
	printf("acphy: 0xafa 0x%04X\n", phy_utils_read_phyreg(pi, 0xafa));
	printf("acphy: 0xb9d 0x%04X\n", phy_utils_read_phyreg(pi, 0xb9d));
	printf("acphy: 0xad6 0x%04X\n", phy_utils_read_phyreg(pi, 0xad6));
	printf("acphy: 0x1b22 0x%04X\n", phy_utils_read_phyreg(pi, 0x1b22));
	printf("acphy: 0xcfa 0x%04X\n", phy_utils_read_phyreg(pi, 0xcfa));
	printf("acphy: 0xd9d 0x%04X\n", phy_utils_read_phyreg(pi, 0xd9d));
	printf("acphy: 0xcd6 0x%04X\n", phy_utils_read_phyreg(pi, 0xcd6));
	printf("acphy: 0x1d22 0x%04X\n", phy_utils_read_phyreg(pi, 0x1d22));
	printf("acphy: 0x1674 0x%04X\n", phy_utils_read_phyreg(pi, 0x1674));
	printf("acphy: 0x1874 0x%04X\n", phy_utils_read_phyreg(pi, 0x1874));
	printf("acphy: 0x1a74 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a74));
	printf("acphy: 0x1c74 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c74));
	printf("acphy: 0x1676 0x%04X\n", phy_utils_read_phyreg(pi, 0x1676));
	printf("acphy: 0x1876 0x%04X\n", phy_utils_read_phyreg(pi, 0x1876));
	printf("acphy: 0x1a76 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a76));
	printf("acphy: 0x1c76 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c76));
	printf("acphy: 0x1675 0x%04X\n", phy_utils_read_phyreg(pi, 0x1675));
	printf("acphy: 0x1875 0x%04X\n", phy_utils_read_phyreg(pi, 0x1875));
	printf("acphy: 0x1a75 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a75));
	printf("acphy: 0x1c75 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c75));
	printf("acphy: 0x1677 0x%04X\n", phy_utils_read_phyreg(pi, 0x1677));
	printf("acphy: 0x1877 0x%04X\n", phy_utils_read_phyreg(pi, 0x1877));
	printf("acphy: 0x1a77 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a77));
	printf("acphy: 0x1c77 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c77));
	printf("acphy: 0x197 0x%04X\n", phy_utils_read_phyreg(pi, 0x197));
	printf("acphy: 0x1047 0x%04X\n", phy_utils_read_phyreg(pi, 0x1047));
	printf("acphy: 0x2e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x2e3));
	printf("acphy: 0x2d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d6));
	printf("acphy: 0x2d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x2d7));
	printf("acphy: 0x400 0x%04X\n", phy_utils_read_phyreg(pi, 0x400));
	printf("acphy: 0x108d 0x%04X\n", phy_utils_read_phyreg(pi, 0x108d));
	printf("acphy: 0x6ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x6ba));
	printf("acphy: 0x8ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x8ba));
	printf("acphy: 0xaba 0x%04X\n", phy_utils_read_phyreg(pi, 0xaba));
	printf("acphy: 0xcba 0x%04X\n", phy_utils_read_phyreg(pi, 0xcba));
	printf("acphy: 0x1087 0x%04X\n", phy_utils_read_phyreg(pi, 0x1087));
	printf("acphy: 0x1088 0x%04X\n", phy_utils_read_phyreg(pi, 0x1088));
	printf("acphy: 0x1089 0x%04X\n", phy_utils_read_phyreg(pi, 0x1089));
	printf("acphy: 0x108a 0x%04X\n", phy_utils_read_phyreg(pi, 0x108a));
	printf("acphy: 0x108b 0x%04X\n", phy_utils_read_phyreg(pi, 0x108b));
	printf("acphy: 0x108c 0x%04X\n", phy_utils_read_phyreg(pi, 0x108c));
	printf("acphy: 0x1081 0x%04X\n", phy_utils_read_phyreg(pi, 0x1081));
	printf("acphy: 0x1082 0x%04X\n", phy_utils_read_phyreg(pi, 0x1082));
	printf("acphy: 0x1083 0x%04X\n", phy_utils_read_phyreg(pi, 0x1083));
	printf("acphy: 0x1084 0x%04X\n", phy_utils_read_phyreg(pi, 0x1084));
	printf("acphy: 0x1085 0x%04X\n", phy_utils_read_phyreg(pi, 0x1085));
	printf("acphy: 0x1086 0x%04X\n", phy_utils_read_phyreg(pi, 0x1086));
	printf("acphy: 0x1a5 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a5));
	printf("acphy: 0x1636 0x%04X\n", phy_utils_read_phyreg(pi, 0x1636));
	printf("acphy: 0x1836 0x%04X\n", phy_utils_read_phyreg(pi, 0x1836));
	printf("acphy: 0x1a36 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a36));
	printf("acphy: 0x1c36 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c36));
	printf("acphy: 0x1637 0x%04X\n", phy_utils_read_phyreg(pi, 0x1637));
	printf("acphy: 0x1837 0x%04X\n", phy_utils_read_phyreg(pi, 0x1837));
	printf("acphy: 0x1a37 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a37));
	printf("acphy: 0x1c37 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c37));
	printf("acphy: 0x1a8 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a8));
	printf("acphy: 0x1a9 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a9));
	printf("acphy: 0x1aa 0x%04X\n", phy_utils_read_phyreg(pi, 0x1aa));
	printf("acphy: 0x6af 0x%04X\n", phy_utils_read_phyreg(pi, 0x6af));
	printf("acphy: 0x8af 0x%04X\n", phy_utils_read_phyreg(pi, 0x8af));
	printf("acphy: 0xaaf 0x%04X\n", phy_utils_read_phyreg(pi, 0xaaf));
	printf("acphy: 0xcaf 0x%04X\n", phy_utils_read_phyreg(pi, 0xcaf));
	printf("acphy: 0x1ab 0x%04X\n", phy_utils_read_phyreg(pi, 0x1ab));
	printf("acphy: 0x29c 0x%04X\n", phy_utils_read_phyreg(pi, 0x29c));
	printf("acphy: 0x7e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x7e2));
	printf("acphy: 0x9e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x9e2));
	printf("acphy: 0xbe2 0x%04X\n", phy_utils_read_phyreg(pi, 0xbe2));
	printf("acphy: 0xde2 0x%04X\n", phy_utils_read_phyreg(pi, 0xde2));
	printf("acphy: 0x000 0x%04X\n", phy_utils_read_phyreg(pi, 0x000));
	// printf("acphy: 0x5e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x5e2));
	printf("acphy: 0x5e2 0x1234\n");
	printf("acphy: 0x32d 0x%04X\n", phy_utils_read_phyreg(pi, 0x32d));
	printf("acphy: 0x32e 0x%04X\n", phy_utils_read_phyreg(pi, 0x32e));
	printf("acphy: 0x32f 0x%04X\n", phy_utils_read_phyreg(pi, 0x32f));
	printf("acphy: 0x713 0x%04X\n", phy_utils_read_phyreg(pi, 0x713));
	printf("acphy: 0x913 0x%04X\n", phy_utils_read_phyreg(pi, 0x913));
	printf("acphy: 0xb13 0x%04X\n", phy_utils_read_phyreg(pi, 0xb13));
	printf("acphy: 0xd13 0x%04X\n", phy_utils_read_phyreg(pi, 0xd13));
	printf("acphy: 0x321 0x%04X\n", phy_utils_read_phyreg(pi, 0x321));
	printf("acphy: 0x322 0x%04X\n", phy_utils_read_phyreg(pi, 0x322));
	printf("acphy: 0x323 0x%04X\n", phy_utils_read_phyreg(pi, 0x323));
	printf("acphy: 0x711 0x%04X\n", phy_utils_read_phyreg(pi, 0x711));
	printf("acphy: 0x911 0x%04X\n", phy_utils_read_phyreg(pi, 0x911));
	printf("acphy: 0xb11 0x%04X\n", phy_utils_read_phyreg(pi, 0xb11));
	printf("acphy: 0xd11 0x%04X\n", phy_utils_read_phyreg(pi, 0xd11));
	printf("acphy: 0x1244 0x%04X\n", phy_utils_read_phyreg(pi, 0x1244));
	printf("acphy: 0x1245 0x%04X\n", phy_utils_read_phyreg(pi, 0x1245));
	printf("acphy: 0x1246 0x%04X\n", phy_utils_read_phyreg(pi, 0x1246));
	printf("acphy: 0x1247 0x%04X\n", phy_utils_read_phyreg(pi, 0x1247));
	printf("acphy: 0x1646 0x%04X\n", phy_utils_read_phyreg(pi, 0x1646));
	printf("acphy: 0x1846 0x%04X\n", phy_utils_read_phyreg(pi, 0x1846));
	printf("acphy: 0x1a46 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a46));
	printf("acphy: 0x1c46 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c46));
	printf("acphy: 0x4e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x4e2));
	printf("acphy: 0x4e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x4e6));
	printf("acphy: 0x4dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x4dd));
	printf("acphy: 0x4de 0x%04X\n", phy_utils_read_phyreg(pi, 0x4de));
	printf("acphy: 0x090 0x%04X\n", phy_utils_read_phyreg(pi, 0x090));
	printf("acphy: 0x091 0x%04X\n", phy_utils_read_phyreg(pi, 0x091));
	printf("acphy: 0x095 0x%04X\n", phy_utils_read_phyreg(pi, 0x095));
	printf("acphy: 0x096 0x%04X\n", phy_utils_read_phyreg(pi, 0x096));
	printf("acphy: 0x08d 0x%04X\n", phy_utils_read_phyreg(pi, 0x08d));
	printf("acphy: 0x08e 0x%04X\n", phy_utils_read_phyreg(pi, 0x08e));
	printf("acphy: 0x08f 0x%04X\n", phy_utils_read_phyreg(pi, 0x08f));
	printf("acphy: 0x092 0x%04X\n", phy_utils_read_phyreg(pi, 0x092));
	printf("acphy: 0x093 0x%04X\n", phy_utils_read_phyreg(pi, 0x093));
	printf("acphy: 0x094 0x%04X\n", phy_utils_read_phyreg(pi, 0x094));
	printf("acphy: 0x08c 0x%04X\n", phy_utils_read_phyreg(pi, 0x08c));
	printf("acphy: 0x10c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c1));
	printf("acphy: 0x10c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c2));
	printf("acphy: 0x10c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x10c0));
	printf("acphy: 0x10a4 0x%04X\n", phy_utils_read_phyreg(pi, 0x10a4));
	printf("acphy: 0x0d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d7));
	printf("acphy: 0x0ce 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ce));
	printf("acphy: 0x0cf 0x%04X\n", phy_utils_read_phyreg(pi, 0x0cf));
	printf("acphy: 0x0d0 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d0));
	printf("acphy: 0x0d1 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d1));
	printf("acphy: 0x0d2 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d2));
	printf("acphy: 0x0d3 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d3));
	printf("acphy: 0x0d4 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d4));
	printf("acphy: 0x0d5 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d5));
	printf("acphy: 0x0d6 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d6));
	printf("acphy: 0x0c3 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c3));
	printf("acphy: 0x0ba 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ba));
	printf("acphy: 0x0bb 0x%04X\n", phy_utils_read_phyreg(pi, 0x0bb));
	printf("acphy: 0x0bc 0x%04X\n", phy_utils_read_phyreg(pi, 0x0bc));
	printf("acphy: 0x0bd 0x%04X\n", phy_utils_read_phyreg(pi, 0x0bd));
	printf("acphy: 0x0be 0x%04X\n", phy_utils_read_phyreg(pi, 0x0be));
	printf("acphy: 0x0bf 0x%04X\n", phy_utils_read_phyreg(pi, 0x0bf));
	printf("acphy: 0x0c0 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c0));
	printf("acphy: 0x0c1 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c1));
	printf("acphy: 0x0c2 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c2));
	printf("acphy: 0x0ff 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ff));
	printf("acphy: 0x0f6 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f6));
	printf("acphy: 0x0f7 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f7));
	printf("acphy: 0x0f8 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f8));
	printf("acphy: 0x0f9 0x%04X\n", phy_utils_read_phyreg(pi, 0x0f9));
	printf("acphy: 0x0fa 0x%04X\n", phy_utils_read_phyreg(pi, 0x0fa));
	printf("acphy: 0x0fb 0x%04X\n", phy_utils_read_phyreg(pi, 0x0fb));
	printf("acphy: 0x0fc 0x%04X\n", phy_utils_read_phyreg(pi, 0x0fc));
	printf("acphy: 0x0fd 0x%04X\n", phy_utils_read_phyreg(pi, 0x0fd));
	printf("acphy: 0x0fe 0x%04X\n", phy_utils_read_phyreg(pi, 0x0fe));
	printf("acphy: 0x167e 0x%04X\n", phy_utils_read_phyreg(pi, 0x167e));
	printf("acphy: 0x187e 0x%04X\n", phy_utils_read_phyreg(pi, 0x187e));
	printf("acphy: 0x1a7e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a7e));
	printf("acphy: 0x1c7e 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c7e));
	printf("acphy: 0x167f 0x%04X\n", phy_utils_read_phyreg(pi, 0x167f));
	printf("acphy: 0x187f 0x%04X\n", phy_utils_read_phyreg(pi, 0x187f));
	printf("acphy: 0x1a7f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a7f));
	printf("acphy: 0x1c7f 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c7f));
	printf("acphy: 0x333 0x%04X\n", phy_utils_read_phyreg(pi, 0x333));
	printf("acphy: 0x334 0x%04X\n", phy_utils_read_phyreg(pi, 0x334));
	printf("acphy: 0x335 0x%04X\n", phy_utils_read_phyreg(pi, 0x335));
	printf("acphy: 0x336 0x%04X\n", phy_utils_read_phyreg(pi, 0x336));
	printf("acphy: 0x337 0x%04X\n", phy_utils_read_phyreg(pi, 0x337));
	printf("acphy: 0x338 0x%04X\n", phy_utils_read_phyreg(pi, 0x338));
	printf("acphy: 0x327 0x%04X\n", phy_utils_read_phyreg(pi, 0x327));
	printf("acphy: 0x328 0x%04X\n", phy_utils_read_phyreg(pi, 0x328));
	printf("acphy: 0x329 0x%04X\n", phy_utils_read_phyreg(pi, 0x329));
	printf("acphy: 0x32a 0x%04X\n", phy_utils_read_phyreg(pi, 0x32a));
	printf("acphy: 0x32b 0x%04X\n", phy_utils_read_phyreg(pi, 0x32b));
	printf("acphy: 0x32c 0x%04X\n", phy_utils_read_phyreg(pi, 0x32c));
	printf("acphy: 0x1647 0x%04X\n", phy_utils_read_phyreg(pi, 0x1647));
	printf("acphy: 0x1847 0x%04X\n", phy_utils_read_phyreg(pi, 0x1847));
	printf("acphy: 0x1a47 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a47));
	printf("acphy: 0x1c47 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c47));
	printf("acphy: 0x1648 0x%04X\n", phy_utils_read_phyreg(pi, 0x1648));
	printf("acphy: 0x1848 0x%04X\n", phy_utils_read_phyreg(pi, 0x1848));
	printf("acphy: 0x1a48 0x%04X\n", phy_utils_read_phyreg(pi, 0x1a48));
	printf("acphy: 0x1c48 0x%04X\n", phy_utils_read_phyreg(pi, 0x1c48));
	printf("acphy: 0x4e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x4e0));
	printf("acphy: 0x4df 0x%04X\n", phy_utils_read_phyreg(pi, 0x4df));
	printf("acphy: 0x4e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x4e4));
	printf("acphy: 0x4e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x4e3));
	printf("acphy: 0x4d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x4d9));
	printf("acphy: 0x4d7 0x%04X\n", phy_utils_read_phyreg(pi, 0x4d7));
	printf("acphy: 0x4da 0x%04X\n", phy_utils_read_phyreg(pi, 0x4da));
	printf("acphy: 0x4d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x4d8));
	printf("acphy: 0x09b 0x%04X\n", phy_utils_read_phyreg(pi, 0x09b));
	printf("acphy: 0x09c 0x%04X\n", phy_utils_read_phyreg(pi, 0x09c));
	printf("acphy: 0x0a0 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a0));
	printf("acphy: 0x0a1 0x%04X\n", phy_utils_read_phyreg(pi, 0x0a1));
	printf("acphy: 0x098 0x%04X\n", phy_utils_read_phyreg(pi, 0x098));
	printf("acphy: 0x099 0x%04X\n", phy_utils_read_phyreg(pi, 0x099));
	printf("acphy: 0x09a 0x%04X\n", phy_utils_read_phyreg(pi, 0x09a));
	printf("acphy: 0x09d 0x%04X\n", phy_utils_read_phyreg(pi, 0x09d));
	printf("acphy: 0x09e 0x%04X\n", phy_utils_read_phyreg(pi, 0x09e));
	printf("acphy: 0x09f 0x%04X\n", phy_utils_read_phyreg(pi, 0x09f));
	printf("acphy: 0x097 0x%04X\n", phy_utils_read_phyreg(pi, 0x097));
	printf("acphy: 0x0e1 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e1));
	printf("acphy: 0x0d8 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d8));
	printf("acphy: 0x0d9 0x%04X\n", phy_utils_read_phyreg(pi, 0x0d9));
	printf("acphy: 0x0da 0x%04X\n", phy_utils_read_phyreg(pi, 0x0da));
	printf("acphy: 0x0db 0x%04X\n", phy_utils_read_phyreg(pi, 0x0db));
	printf("acphy: 0x0dc 0x%04X\n", phy_utils_read_phyreg(pi, 0x0dc));
	printf("acphy: 0x0dd 0x%04X\n", phy_utils_read_phyreg(pi, 0x0dd));
	printf("acphy: 0x0de 0x%04X\n", phy_utils_read_phyreg(pi, 0x0de));
	printf("acphy: 0x0df 0x%04X\n", phy_utils_read_phyreg(pi, 0x0df));
	printf("acphy: 0x0e0 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e0));
	printf("acphy: 0x0eb 0x%04X\n", phy_utils_read_phyreg(pi, 0x0eb));
	printf("acphy: 0x0e2 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e2));
	printf("acphy: 0x0e3 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e3));
	printf("acphy: 0x0e4 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e4));
	printf("acphy: 0x0e5 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e5));
	printf("acphy: 0x0e6 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e6));
	printf("acphy: 0x0e7 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e7));
	printf("acphy: 0x0e8 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e8));
	printf("acphy: 0x0e9 0x%04X\n", phy_utils_read_phyreg(pi, 0x0e9));
	printf("acphy: 0x0ea 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ea));
	printf("acphy: 0x0cd 0x%04X\n", phy_utils_read_phyreg(pi, 0x0cd));
	printf("acphy: 0x0c4 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c4));
	printf("acphy: 0x0c5 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c5));
	printf("acphy: 0x0c6 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c6));
	printf("acphy: 0x0c7 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c7));
	printf("acphy: 0x0c8 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c8));
	printf("acphy: 0x0c9 0x%04X\n", phy_utils_read_phyreg(pi, 0x0c9));
	printf("acphy: 0x0ca 0x%04X\n", phy_utils_read_phyreg(pi, 0x0ca));
	printf("acphy: 0x0cb 0x%04X\n", phy_utils_read_phyreg(pi, 0x0cb));
	printf("acphy: 0x0cc 0x%04X\n", phy_utils_read_phyreg(pi, 0x0cc));
	printf("acphy: 0x109 0x%04X\n", phy_utils_read_phyreg(pi, 0x109));
	printf("acphy: 0x100 0x%04X\n", phy_utils_read_phyreg(pi, 0x100));
	printf("acphy: 0x101 0x%04X\n", phy_utils_read_phyreg(pi, 0x101));
	printf("acphy: 0x102 0x%04X\n", phy_utils_read_phyreg(pi, 0x102));
	printf("acphy: 0x103 0x%04X\n", phy_utils_read_phyreg(pi, 0x103));
	printf("acphy: 0x104 0x%04X\n", phy_utils_read_phyreg(pi, 0x104));
	printf("acphy: 0x105 0x%04X\n", phy_utils_read_phyreg(pi, 0x105));
	printf("acphy: 0x106 0x%04X\n", phy_utils_read_phyreg(pi, 0x106));
	printf("acphy: 0x107 0x%04X\n", phy_utils_read_phyreg(pi, 0x107));
	printf("acphy: 0x108 0x%04X\n", phy_utils_read_phyreg(pi, 0x108));
}


/* Register/unregister ACPHY specific implementation to common layer. */
phy_ac_radio_info_t *
BCMATTACHFN(phy_ac_radio_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_radio_info_t *ri)
{
	phy_ac_radio_info_t *info;
	acphy_pmu_core1_off_radregs_t *pmu_c1_off_info_orig = NULL;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = NULL;
	phy_type_radio_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_ac_radio_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pmu_c1_off_info_orig =
		phy_malloc(pi, sizeof(acphy_pmu_core1_off_radregs_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc pmu_c1_off_info_orig failed\n", __FUNCTION__));
		goto fail;
	}
	if ((pmu_lp_opt_orig =
		phy_malloc(pi, sizeof(acphy_pmu_mimo_lp_opt_radregs_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc pmu_lp_opt_orig failed\n", __FUNCTION__));
		goto fail;
	}

	info->pi = pi;
	info->aci = aci;
	info->ri = ri;
	info->pmu_c1_off_info_orig = pmu_c1_off_info_orig;
	info->pmu_lp_opt_orig = pmu_lp_opt_orig;
	pmu_lp_opt_orig->is_orig = FALSE;

	/* By default, use 5g pll for 2g will be disabled */
	info->data.use_5g_pll_for_2g = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_use5gpllfor2g, 0);

	/* Compute 20695 PLL config constants */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		pll_config_20695_tbl_t *pll;

		/* Get pointer to structure */
		pll = wlc_phy_ac_get_20695_pll_config(pi);

		if (phy_ac_radio20695_populate_pll_config_tbl(pi, pll) != BCME_OK) {
			PHY_ERROR(("%s: phy_ac_radio20695_populate_pll_config_tbl failed\n",
					__FUNCTION__));
			goto fail;
		}

		phy_ac_radio20695_pll_config_const_calc(pi, pll);
	}
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		if (pi->fabid == BCM4349_TSMC_FAB_ID)
			pi->rcal_value = TSMC_FAB_RCAL_VAL;
		else
			pi->rcal_value = UMC_FAB_RCAL_VAL;
	} else {
		/* falling back to some value */
		pi->rcal_value = UMC_FAB_RCAL_VAL;
	}

	/* Compute 20694 PLL config constants */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		pll_config_20694_tbl_t *pll;

		/* Get pointer to structure */
		pll = wlc_phy_ac_get_20694_pll_config(pi);

		if (phy_ac_radio20694_populate_pll_config_tbl(pi, pll) != BCME_OK) {
			PHY_ERROR(("%s: phy_ac_radio20694_populate_pll_config_tbl failed\n",
					__FUNCTION__));
			goto fail;
		}

		phy_ac_radio20694_pll_config_const_calc(pi, pll);
	}

	/* Populate 20696 PLL configuration table */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20696_ID)) {
		if (phy_ac_radio20696_populate_pll_config_tbl(pi) != BCME_OK) {
			PHY_ERROR(("%s: phy_ac_radio20696_populate_pll_config_tbl failed\n",
					__FUNCTION__));
			goto fail;
		}
	}

	/* Read srom params from nvram */
	phy_ac_radio_nvram_attach(pi, info);

	/* make sure the radio is off until we do an "up" */
	phy_ac_radio_switch(info, OFF);

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctrl = phy_ac_radio_switch;
	fns.on = phy_ac_radio_on;
#ifdef PHY_DUMP_BINARY
	fns.getlistandsize = phy_ac_radio_getlistandsize;
#endif
	fns.dump = phy_ac_radio_dump;
	fns.id = _phy_ac_radio_query_idcode;
	fns.ctx = info;

	phy_ac_radio_std_params_attach(info);

	phy_ac_radio_init_lpmode(info);

	phy_radio_register_impl(ri, &fns);

	return info;

fail:
	if (pmu_c1_off_info_orig != NULL)
		phy_mfree(pi, pmu_c1_off_info_orig, sizeof(acphy_pmu_core1_off_radregs_t));

	if (pmu_lp_opt_orig != NULL)
		phy_mfree(pi, pmu_c1_off_info_orig, sizeof(acphy_pmu_mimo_lp_opt_radregs_t));

	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ac_radio_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_radio_unregister_impl)(phy_ac_radio_info_t *info)
{
	phy_info_t *pi;
	phy_radio_info_t *ri;

	uint sicoreunit;
	sicoreunit = wlapi_si_coreunit(info->pi->sh->physhim);

	ASSERT(info);
	pi = info->pi;
	ri = info->ri;

	phy_radio_unregister_impl(ri);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20696_ID))
		phy_ac_radio20696_populate_pll_config_mfree(pi);

	phy_mfree(pi, info->pmu_c1_off_info_orig, sizeof(acphy_pmu_core1_off_radregs_t));

	if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		pll_config_20695_tbl_t *pll;

		/* Get pointer to structure */
		pll = wlc_phy_ac_get_20695_pll_config(pi);

		/* Free the allocated memory */
		phy_mfree(pi, pll->reg_addr_2g, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_addr_5g, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_mask_2g,
				(sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_mask_5g,
				(sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_shift_2g,
				(sizeof(uint8) * PLL_CONFIG_20695_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_shift_5g,
				(sizeof(uint8) * PLL_CONFIG_20695_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_val, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		pll_config_20694_tbl_t *pll;

		/* Get pointer to structure */
		pll = wlc_phy_ac_get_20694_pll_config(pi);

		/* Free the allocated memory */
		phy_mfree(pi, pll->reg_addr, (sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_mask,
				(sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_shift,
				(sizeof(uint8) * PLL_CONFIG_20694_ARRAY_SIZE));
		phy_mfree(pi, pll->reg_field_val, (sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE));
		if (sicoreunit == DUALMAC_MAIN) {
			phy_mfree(pi, pll->reg_addr_main,
				(sizeof(uint16) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
			phy_mfree(pi, pll->reg_field_mask_main,
				(sizeof(uint16) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
			phy_mfree(pi, pll->reg_field_shift_main,
				(sizeof(uint8) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
			phy_mfree(pi, pll->reg_field_val_main,
				(sizeof(uint16) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
		}
	}

	phy_mfree(pi, info->pmu_lp_opt_orig, sizeof(acphy_pmu_mimo_lp_opt_radregs_t));

	phy_mfree(pi, info, sizeof(phy_ac_radio_info_t));
}

static void
BCMATTACHFN(phy_ac_radio_std_params_attach)(phy_ac_radio_info_t *radioi)
{
	radioi->prev_subband = 15;
	radioi->aci->acphy_4335_radio_pd_status = 0;
	radioi->data.rccal_gmult = 128;
	radioi->data.rccal_gmult_rc = 128;
	radioi->data.rccal_dacbuf = 12;
	/* AFE */
	radioi->afeRfctrlCoreAfeCfg10 = READ_PHYREG(radioi->pi, RfctrlCoreAfeCfg10);
	radioi->afeRfctrlCoreAfeCfg20 = READ_PHYREG(radioi->pi, RfctrlCoreAfeCfg20);
	radioi->afeRfctrlOverrideAfeCfg0 = READ_PHYREG(radioi->pi, RfctrlOverrideAfeCfg0);
	/* Radio RX */
	radioi->rxRfctrlCoreRxPus0 = READ_PHYREG(radioi->pi, RfctrlCoreRxPus0);
	radioi->rxRfctrlOverrideRxPus0 = READ_PHYREG(radioi->pi, RfctrlOverrideRxPus0);
	/* Radio TX */
	radioi->txRfctrlCoreTxPus0 = READ_PHYREG(radioi->pi, RfctrlCoreTxPus0);
	radioi->txRfctrlOverrideTxPus0 = READ_PHYREG(radioi->pi, RfctrlOverrideTxPus0);

	/* {radio, rfpll, pllldo}_pu = 0 */
	radioi->radioRfctrlCmd = READ_PHYREG(radioi->pi, RfctrlCmd);
	radioi->radioRfctrlCoreGlobalPus = READ_PHYREG(radioi->pi, RfctrlCoreGlobalPus);
	radioi->radioRfctrlOverrideGlobalPus = READ_PHYREG(radioi->pi, RfctrlOverrideGlobalPus);

	/* 4349A0: priority flags for 80p80 and RSDB modes
	   these flags indicate which core's tuning settings takes
	   precedence in conflict scenarios
	 */
	radioi->crisscross_priority_core_80p80 = 0;
	radioi->crisscross_priority_core_rsdb = 0;

	radioi->is_crisscross_actv = 0;
	if (ACMAJORREV_1(radioi->pi->pubpi->phy_rev)) {
		radioi->aci->acphy_force_lpvco_2G = 1; /* Enable 2G LP mode */
	} else {
		radioi->aci->acphy_force_lpvco_2G = 0;
	}
	radioi->acphy_lp_mode = 1;
	radioi->acphy_prev_lp_mode = radioi->acphy_lp_mode;
	radioi->aci->acphy_lp_status = radioi->acphy_lp_mode;

}

/* switch radio on/off */
static void
phy_ac_radio_switch(phy_type_radio_ctx_t *ctx, bool on)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 core;

	/* JIRA: SW4349-1163. 4349 B0: Tx Tput in Core Mask 2 is less compared to Core Mask 1 */
	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
		(RADIOMAJORREV(pi) != 3) && (pi->radio_is_on == TRUE) && (!on)) {
		if (phy_get_phymode(pi) == PHYMODE_MIMO) {
			wlc_phy_radio20693_mimo_lpopt_restore(pi);
		}
	}

	/* Set read override to before mux in 20695 radio */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_28NM(pi, RF, READOVERRIDES, core,
					ReadOverrides_reg, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, READOVERRIDES, core,
					ReadOverrides_reg, 0x0);
		}
	}

	/* ported from tcl confirm ??  bpeiris (janath) */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		FOREACH_CORE(pi, core) {
			if (core == 0) {
				MOD_RADIO_REG_20694(pi, RF, READOVERRIDES, 0,
					ReadOverrides_reg, 0x1);
			} else if (core == 1) {
				MOD_RADIO_REG_20694(pi, RF, READOVERRIDES, 1,
					ReadOverrides_reg, 0x1);
			}

		}
	}

	/* sync up soft_radio_state with hard_radio_state */
	pi->radio_is_on = FALSE;

	PHY_TRACE(("wl%d: %s %d Phymode: %x\n", pi->sh->unit,
		__FUNCTION__, on, phy_get_phymode(pi)));

	wlc_phy_switch_radio_acphy(pi, on);
}

/* turn radio on */
static void
WLBANDINITFN(phy_ac_radio_on)(phy_type_radio_ctx_t *ctx)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_ac_info_t *aci = info->aci;
	phy_info_t *pi = aci->pi;

	/* To make sure that chanspec_set_acphy doesn't get called twice during init */
	phy_ac_chanmgr_get_data(aci->chanmgri)->init_done = FALSE;

	/* update phy_corenum and phy_coremask */
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		phy_ac_update_phycorestate(pi);
	}

	phy_ac_radio_switch(ctx, ON);

	/* Init regs/tables only once that do not get reset on phy_reset */
	wlc_phy_set_regtbl_on_pwron_acphy(pi);
}

/* Internal data api between ac modules */
phy_ac_radio_data_t *
phy_ac_radio_get_data(phy_ac_radio_info_t *radioi)
{
	return &(radioi->data);
}

/* query radio idcode */
uint32
phy_ac_radio_query_idcode(phy_info_t *pi)
{
	uint32 b0, b1;

	W_REG(pi->sh->osh, &pi->regs->radioregaddr, 0);
#ifdef __mips__
	(void)R_REG(pi->sh->osh, &pi->regs->radioregaddr);
#endif
	b0 = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);
	W_REG(pi->sh->osh, &pi->regs->radioregaddr, 1);
#ifdef __mips__
	(void)R_REG(pi->sh->osh, &pi->regs->radioregaddr);
#endif
	b1 = (uint32)R_REG(pi->sh->osh, &pi->regs->radioregdata);

	/* For ACPHY (starting with 4360A0), address 0 has the revid and
	   address 1 has the devid
	*/
	return (b0 << 16) | b1;
}

/* parse radio idcode and write to pi->pubpi */
void
phy_ac_radio_parse_idcode(phy_info_t *pi, uint32 idcode)
{
	PHY_TRACE(("%s: idcode 0x%08x\n", __FUNCTION__, idcode));

	pi->pubpi->radioid = (idcode & IDCODE_ACPHY_ID_MASK) >> IDCODE_ACPHY_ID_SHIFT;
	pi->pubpi->radiorev = (idcode & IDCODE_ACPHY_REV_MASK) >> IDCODE_ACPHY_REV_SHIFT;

	/* ACPHYs do not use radio ver. This param is invalid */
	pi->pubpi->radiover = 0;
}

void
wlc_phy_lp_mode(phy_ac_radio_info_t *ri, int8 lp_mode)
{
	if (ACMAJORREV_1(ri->pi->pubpi->phy_rev) || ACMAJORREV_3(ri->pi->pubpi->phy_rev)) {
		if (lp_mode == 1) {
			/* AC MODE (Full Pwr mode) */
			ri->acphy_lp_mode = 1;
		} else if (lp_mode == 2) {
			/* 11n MODE (VCO in 11n Mode) */
			ri->acphy_lp_mode = 2;
		} else if (lp_mode == 3) {
			/* Low pwr MODE */
			ri->acphy_lp_mode = 3;
		} else {
			return;
		}
	} else {
		return;
	}
}

static void
BCMATTACHFN(phy_ac_radio_init_lpmode)(phy_ac_radio_info_t *ri)
{
	ri->lpmode_2g = ACPHY_LPMODE_NONE;
	ri->lpmode_5g = ACPHY_LPMODE_NONE;

	if ((ACMAJORREV_1(ri->pi->pubpi->phy_rev) && ACMINORREV_2(ri->pi)) ||
	    ACMAJORREV_3(ri->pi->pubpi->phy_rev)) {
		switch (BF3_ACPHY_LPMODE_2G(ri->aci)) {
			case 1:
				ri->lpmode_2g = ACPHY_LPMODE_LOW_PWR_SETTINGS_1;
				break;
			case 2:
				ri->lpmode_2g = ACPHY_LPMODE_LOW_PWR_SETTINGS_2;
				break;
			case 3:
				ri->lpmode_2g = ACPHY_LPMODE_NORMAL_SETTINGS;
				break;
			case 0:
			default:
				ri->lpmode_2g = ACPHY_LPMODE_NONE;
		}

		switch (BF3_ACPHY_LPMODE_5G(ri->aci)) {
			case 1:
				ri->lpmode_5g = ACPHY_LPMODE_LOW_PWR_SETTINGS_1;
				break;
			case 2:
				ri->lpmode_5g = ACPHY_LPMODE_LOW_PWR_SETTINGS_2;
				break;
			case 3:
				ri->lpmode_5g = ACPHY_LPMODE_NORMAL_SETTINGS;
				break;
			case 0:
			default:
				ri->lpmode_5g = ACPHY_LPMODE_NONE;
		}
	} else if (ACMAJORREV_4(ri->pi->pubpi->phy_rev)) {
		switch (BF3_ACPHY_LPMODE_2G(ri->aci)) {
			case 1:
				ri->lpmode_2g = ACPHY_LPMODE_LOW_PWR_SETTINGS_1;
				break;
			case 0:
			default:
				ri->lpmode_2g = ACPHY_LPMODE_NONE;
		}

		switch (BF3_ACPHY_LPMODE_5G(ri->aci)) {
			case 1:
				ri->lpmode_5g = ACPHY_LPMODE_LOW_PWR_SETTINGS_1;
				break;
			case 0:
			default:
				ri->lpmode_5g = ACPHY_LPMODE_NONE;
		}
	}
}

void
acphy_set_lpmode(phy_info_t *pi, acphy_lp_opt_levels_t lp_opt_lvl)
{
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;
	if ((ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) ||
	    ACMAJORREV_3(pi->pubpi->phy_rev)) {
		switch (lp_opt_lvl) {
		case ACPHY_LP_RADIO_LVL_OPT:
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				if (ri->lpmode_2g == ACPHY_LPMODE_LOW_PWR_SETTINGS_1) {
					wlc_phy_radio_vco_opt(pi, ACPHY_VCO_2P5V);
				} else if (ri->lpmode_2g == ACPHY_LPMODE_LOW_PWR_SETTINGS_2) {
					wlc_phy_radio_vco_opt(pi, ACPHY_VCO_1P35V);
				}
			} else {
				/* lpmode reset for 5G */
				MOD_RADIO_REG_20691(pi, PLL_VCO3, 0, rfpll_vco_cvar_extra, 0xf);
				MOD_RADIO_REG_20691(pi, PLL_VCO2, 0, rfpll_vco_cvar, 0x8);
				MOD_RADIO_REG_20691(pi, PLL_VCO6, 0, rfpll_vco_bias_mode, 0x1);
				MOD_RADIO_REG_20691(pi, PLL_VCO6, 0, rfpll_vco_ALC_ref_ctrl, 0xf);
				MOD_RADIO_REG_20691(pi, PLL_CP4, 0, rfpll_cp_kpd_scale, 0x34);
				if (CHIPID(pi->sh->chip) != BCM4364_CHIP_ID) {
					MOD_RADIO_REG_20691(pi, PLL_XTALLDO1, 0,
							ldo_1p2_xtalldo1p2_lowquiescenten, 0x0);
				}
				MOD_RADIO_REG_20691(pi, PLL_HVLDO2, 0,
					ldo_2p5_lowquiescenten_VCO, 0x0);
				MOD_RADIO_REG_20691(pi, PLL_HVLDO2, 0,
					ldo_2p5_lowquiescenten_CP, 0x0);
				MOD_RADIO_REG_20691(pi, PLL_HVLDO4, 0, ldo_2p5_static_load_CP, 0x0);
				MOD_RADIO_REG_20691(pi, PLL_HVLDO4, 0,
					ldo_2p5_static_load_VCO, 0x0);
				MOD_RADIO_REG_20691(pi, PLL_CFG3, 0, rfpll_spare1, 0x0);
				MOD_RADIO_REG_20691(pi, PLL_CFG3, 0, rfpll_spare0, 0x34);
			}
			break;
		case ACPHY_LP_CHIP_LVL_OPT:
		case ACPHY_LP_PHY_LVL_OPT:
		default:
			break;
		}
	} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		/* 4349 related power optimizations */
		bool for_2g = ((ri->lpmode_2g != ACPHY_LPMODE_NONE) &&
			(CHSPEC_IS2G(pi->radio_chanspec)));
		bool for_5g = ((ri->lpmode_5g != ACPHY_LPMODE_NONE) &&
			(CHSPEC_IS5G(pi->radio_chanspec)));

		switch (lp_opt_lvl) {
		case ACPHY_LP_RADIO_LVL_OPT:
		{
			if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
					(phy_get_phymode(pi) == PHYMODE_MIMO) &&
					((for_2g == TRUE) || (for_5g == TRUE))) {
				uint8 new_rxchain = pi->sh->phyrxchain;
				uint8 old_rxchain = phy_ac_chanmgr_get_data
					(ri->aci->chanmgri)->phyrxchain_old;
				uint8 to_cm1_rx = (new_rxchain == 1) &&
					((old_rxchain == 2) || (old_rxchain == 3));
				uint8 rx_cm1_to_cm1 = (new_rxchain == 1) && (old_rxchain == 1);

			PHY_TRACE(("[%s]chains: newrx:%d oldrx: %d newtx:%d both_txrxchain:%d\n",
				__FUNCTION__, pi->sh->phyrxchain, phy_ac_chanmgr_get_data
				(ri->aci->chanmgri)->phyrxchain_old, pi->sh->phytxchain,
				pi->u.pi_acphy->both_txchain_rxchain_eq_1));

				wlc_phy_radio20693_mimo_lpopt_restore(pi);
				if (to_cm1_rx || rx_cm1_to_cm1) {
					wlc_phy_radio20693_mimo_cm1_lpopt(pi);
				} else {
					wlc_phy_radio20693_mimo_cm23_lp_opt(pi, new_rxchain);
				}
			}
		}
		break;
		case ACPHY_LP_CHIP_LVL_OPT:
			break;
		case ACPHY_LP_PHY_LVL_OPT:
			if ((for_5g == TRUE) && (pi->sh->phyrxchain != 2)) {
				MOD_PHYREG(pi, CRSMiscellaneousParam, bphy_pre_det_en, 1);
				MOD_PHYREG(pi, CRSMiscellaneousParam,
					b_over_ag_falsedet_en, 0);
			} else {
				MOD_PHYREG(pi, CRSMiscellaneousParam, bphy_pre_det_en, 0);
				MOD_PHYREG(pi, CRSMiscellaneousParam,
					b_over_ag_falsedet_en, 1);
			}
			/* JIRA: CRDOT11ACPHY-848. Recommendation from RTL team:
			 * Because of RTL issue, some logic related to this table is
			 * in un-initialized state and is consuming current. Just reading
			 * the table (it should be the last operation performed on this
			 *  table) will fix this.
			 */
			/* JIRA: CRDOT11ACPHY-1278, SW4349-1030 */
			if (!((phy_get_phymode(pi) == PHYMODE_RSDB) &&
				(phy_get_current_core(pi) == 1))) {
				uint16 val;
				wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_BFECONFIG2X2TBL,
					1, 0, 16, &val);
				BCM_REFERENCE(val);
			}
			/*
			if {[hwaccess] == $def(hw_jtag)} {
				jtag w 0x18004492 2 0x0
				jtag w 0x18001492 2 0x2
			}
			*/

			WRITE_PHYREG(pi, FFTSoftReset, 0x2);
			break;
		default:
			break;
		}

	}
}

void
wlc_phy_force_lpvco_2G(phy_info_t *pi, int8 force_lpvco_2G)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	if ACMAJORREV_1(pi->pubpi->phy_rev)	{
		pi_ac->acphy_force_lpvco_2G = force_lpvco_2G;
	} else {
		return;
	}
}

/* Proc to power on dac_clocks though override */
bool wlc_phy_poweron_dac_clocks(phy_info_t *pi, uint8 core, uint16 *orig_dac_clk_pu,
	uint16 *orig_ovr_dac_clk_pu)
{
	uint16 dacpwr;

	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		dacpwr = 0;
		/* This condition is put to prevent wrong reg access */
		ASSERT(0);
	} else {
		dacpwr = READ_RADIO_REGFLD_TINY(pi, TX_DAC_CFG1, core, DAC_pwrup);

		if (dacpwr == 0) {
			*orig_dac_clk_pu = READ_RADIO_REGFLD_TINY(pi, CLK_DIV_CFG1, core,
					dac_clk_pu);
			*orig_ovr_dac_clk_pu = READ_RADIO_REGFLD_TINY(pi, CLK_DIV_OVR1, core,
					ovr_dac_clk_pu);
			MOD_RADIO_REG_TINY(pi, CLK_DIV_CFG1, core, dac_clk_pu, 1);
			MOD_RADIO_REG_TINY(pi, CLK_DIV_OVR1, core, ovr_dac_clk_pu, 1);
		}
	}

	return (dacpwr == 0);
}

/* Proc to resotre dac_clock_pu and the corresponding ovrride registers */
void wlc_phy_restore_dac_clocks(phy_info_t *pi, uint8 core, uint16 orig_dac_clk_pu,
	uint16 orig_ovr_dac_clk_pu)
{
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		/* This condition is put to prevent wrong reg access */
		ASSERT(0);
	} else {
		MOD_RADIO_REG_TINY(pi, CLK_DIV_CFG1, core, dac_clk_pu, orig_dac_clk_pu);
		MOD_RADIO_REG_TINY(pi, CLK_DIV_OVR1, core, ovr_dac_clk_pu, orig_ovr_dac_clk_pu);
	}
}

#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && (defined(BCMINTERNAL) || \
	defined(DBG_PHY_IOV))) || defined(BCMDBG_PHYDUMP)
static int
phy_ac_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_ac_radio_info_t *gi = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = gi->pi;
	const char *name = NULL;
	int i, jtag_core;
	uint16 addr = 0;
	const radio_20xx_dumpregs_t *radio20xxdumpregs = NULL;

	uint sicoreunit;
	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		name = "20693";
		if ((RADIO20693_MAJORREV(pi->pubpi->radiorev) == 1) ||
			(RADIO20693_MAJORREV(pi->pubpi->radiorev) == 2)) {
			uint16 phymode = phy_get_phymode(pi);
			if (phymode == PHYMODE_RSDB)
				radio20xxdumpregs = dumpregs_20693_rsdb;
			else if (phymode == PHYMODE_MIMO)
				radio20xxdumpregs = dumpregs_20693_mimo;
			else if (phymode == PHYMODE_80P80)
				radio20xxdumpregs = dumpregs_20693_80p80;
			else
				ASSERT(0);
		} else if (RADIO20693_MAJORREV(pi->pubpi->radiorev) == 3) {
			radio20xxdumpregs = dumpregs_20693_rev32;
		} else {
			return BCME_NOTFOUND;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
		name = "2069";
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			radio20xxdumpregs = dumpregs_2069_rev32;
		} else if ((RADIO2069REV(pi->pubpi->radiorev) == 16) ||
			(RADIO2069REV(pi->pubpi->radiorev) == 18)) {
			radio20xxdumpregs = dumpregs_2069_rev16;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 17) {
			radio20xxdumpregs = dumpregs_2069_rev17;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 25) {
			radio20xxdumpregs = dumpregs_2069_rev25;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 64) {
			radio20xxdumpregs = dumpregs_2069_rev64;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 66) {
			radio20xxdumpregs = dumpregs_2069_rev64;
		} else {
			radio20xxdumpregs = dumpregs_2069_rev0;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
		name = "20691";
		switch (RADIO20691REV(pi->pubpi->radiorev)) {
		case 60:
		case 68:
			radio20xxdumpregs = dumpregs_20691_rev68;
			break;
		case 75:
			radio20xxdumpregs = dumpregs_20691_rev75;
			break;
		case 79:
			radio20xxdumpregs = dumpregs_20691_rev79;
			break;
		case 74:
		case 82:
			radio20xxdumpregs = dumpregs_20691_rev82;
			break;
		case 129:
			radio20xxdumpregs = dumpregs_20691_rev129;
			break;
		default:
			;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		/* TODO: Add 20694 specific code */
		name = "20694";
		switch (RADIOREV(pi->pubpi->radiorev)) {
		case 36:
		  radio20xxdumpregs = dumpregs_20694_rev36;
		  break;
		default:
		if (sicoreunit == DUALMAC_MAIN) {
		  radio20xxdumpregs = dumpregs_20694_rev5;
		} else {
			return 0;
		}
		}
	}
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		/* TODO: Add 20695 specific code */
	         name = "20695";
	}
	else
		return BCME_ERROR;

	i = 0;
	ASSERT(radio20xxdumpregs != NULL);    /* to indicate developer to put in the table */
	if (radio20xxdumpregs == NULL)
		return BCME_OK; /* If the table is not yet implemented, */
						/* just skip the output (for non-assert builds) */

	bcm_bprintf(b, "----- %08s -----\n", name);
	bcm_bprintf(b, "Add Value");
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 1)
		bcm_bprintf(b, "0 Value1 ...");
	bcm_bprintf(b, "\n");

	while ((addr = radio20xxdumpregs[i].address) != 0xffff) {

		if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
			jtag_core = (addr & JTAG_2069_MASK);
			addr &= (~JTAG_2069_MASK);
		} else
			jtag_core = 0;

		if ((RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) &&
			(jtag_core == JTAG_2069_ALL)) {
			switch (PHYCORENUM(pi->pubpi->phy_corenum)) {
			case 1:
				bcm_bprintf(b, "%03x %04x\n", addr,
					phy_utils_read_radioreg(pi, addr | JTAG_2069_CR0));
				break;
			case 2:
				bcm_bprintf(b, "%03x %04x %04x\n", addr,
					phy_utils_read_radioreg(pi, addr | JTAG_2069_CR0),
					phy_utils_read_radioreg(pi, addr | JTAG_2069_CR1));
				break;
			case 3:
				bcm_bprintf(b, "%03x %04x %04x %04x\n", addr,
					phy_utils_read_radioreg(pi, addr | JTAG_2069_CR0),
					phy_utils_read_radioreg(pi, addr | JTAG_2069_CR1),
					phy_utils_read_radioreg(pi, addr | JTAG_2069_CR2));
				break;
			default:
				break;
			}
		} else {
			if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
			    (RADIOMAJORREV(pi) == 3)) {
				if ((addr >> JTAG_20693_SHIFT) ==
				    (JTAG_20693_CR0 >> JTAG_20693_SHIFT)) {
					bcm_bprintf(b, "%03x %04x %04x %04x %04x\n", addr,
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR0),
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR1),
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR2),
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR3));
				} else if ((addr >> JTAG_20693_SHIFT) ==
				           (JTAG_20693_PLL0 >> JTAG_20693_SHIFT)) {
					bcm_bprintf(b, "%03x %04x %04x\n", addr,
					phy_utils_read_radioreg(pi, addr | JTAG_20693_PLL0),
					phy_utils_read_radioreg(pi, addr | JTAG_20693_PLL1));
				}
			} else {
			bcm_bprintf(b, "%03x %04x\n", addr,
			            phy_utils_read_radioreg(pi, addr | jtag_core));
			}
		}
		i++;
	}

	return BCME_OK;
}
#endif /* BCMDBG, BCMDBG_DUMP, BCMINTERNAL, DBG_PHY_IOV, BCMDBG_PHYDUMP */

static void
BCMATTACHFN(phy_ac_radio_nvram_attach)(phy_info_t *pi, phy_ac_radio_info_t *radioi)
{
#ifndef BOARD_FLAGS3
	uint32 bfl3; /* boardflags3 */
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
#endif

	if (BF3_RCAL_OTP_VAL_EN(pi->u.pi_acphy) == 1) {
		if (!otp_read_word(pi->sh->sih, ACPHY_RCAL_OFFSET, &pi->sromi->rcal_otp_val)) {
			pi->sromi->rcal_otp_val &= 0xf;
		} else {
			if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
					pi->sromi->rcal_otp_val = ACPHY_RCAL_VAL_2X2;
				} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
					pi->sromi->rcal_otp_val = ACPHY_RCAL_VAL_1X1;
				}
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
				pi->sromi->rcal_otp_val = ACPHY_RCAL_VAL_1X1;
			}
		}
	}

	if (CHIPID(pi->sh->chip) == BCM4364_CHIP_ID) {
		uint16 rcal_value = 0;
		if (!otp_read_word(pi->sh->sih, ACPHY_RCAL_OFFSET, &pi->sromi->rcal_otp_val)) {
			rcal_value = (pi->sromi->rcal_otp_val & 0x300) >> 8;
			otp_read_word(pi->sh->sih, 0x14, &pi->sromi->rcal_otp_val);
			pi->sromi->rcal_otp_val = ((pi->sromi->rcal_otp_val &
				0x3) << 2) | rcal_value;
		}
	} else if ((CHIPID(pi->sh->chip) == BCM4345_CHIP_ID) &&
		(RADIO20691_MAJORREV(pi->pubpi->radiorev) == 1) &&
		(RADIO20691_MINORREV(pi->pubpi->radiorev) >= 5)) {
		if (!otp_read_word(pi->sh->sih, ACPHY_RCAL_OFFSET, &pi->sromi->rcal_otp_val))
			pi->sromi->rcal_otp_val = (pi->sromi->rcal_otp_val & 0xf00) >> 8;
	}

	pi->dacratemode2g = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_dacratemode2g, 1));
	pi->dacratemode5g = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_dacratemode5g, 1));
	radioi->data.srom_txnospurmod5g = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_txnospurmod5g, 1);
	radioi->data.srom_txnospurmod2g = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_txnospurmod2g, 0);
	radioi->data.vcodivmode = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_vcodivmode, 0));

#ifndef BOARD_FLAGS3
	if ((PHY_GETVAR_SLICE(pi, rstr_boardflags3)) != NULL) {
		bfl3 = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_boardflags3);
		BF3_RCAL_WAR(pi_ac) = ((bfl3 & BFL3_RCAL_WAR) != 0);
		BF3_RCAL_OTP_VAL_EN(pi_ac) = ((bfl3 & BFL3_RCAL_OTP_VAL_EN) != 0);
		BF3_TSSI_DIV_WAR(pi_ac) = (bfl3 & BFL3_TSSI_DIV_WAR) >> BFL3_TSSI_DIV_WAR_SHIFT;
	} else {
		BF3_RCAL_WAR(pi_ac) = 0;
		BF3_RCAL_OTP_VAL_EN(pi_ac) = 0;
		BF3_TSSI_DIV_WAR(pi_ac) = 0;
	}
#endif /* BOARD_FLAGS3 */
}

/* 20695 radio related functions */
void
wlc_phy_radio20695_afe_div_ratio(phy_info_t *pi, uint8 ulp_tx_mode, uint8 ulp_adc_mode)
{
	uint16 fc;
	uint8 *adc_div_lut;
	uint8 *dac_div_lut;
	uint8 *dac_bypN2_lut;
	uint8 *adc_bypN2_lut;
	uint8 adc_bypN2 = 0;
	uint8 adc_div, dac_div, dac_bypN2;

	/* ADC div lut for different channels */
	/* ADC rates for adc_mode 0:3 are 41/50/180/120mhz */
	uint8 adc_div_lut_ge_5745[] = {40, 13, 4, 6};
	uint8 adc_div_lut_ge_5180[] = {40, 13, 4, 6};
	uint8 adc_div_lut_ge_4905[] = {39, 12, 7, 5};
	uint8 adc_div_lut_2g[] = {14, 12, 7, 5};

	uint8 adc_bypN2_lut_ge_5745[] = {0, 0, 0, 0};
	uint8 adc_bypN2_lut_ge_5180[] = {0, 0, 0, 0};
	uint8 adc_bypN2_lut_ge_4905[] = {0, 0, 1, 0};
	uint8 adc_bypN2_lut_2g[] = {0, 0, 1, 0};

	/* DAC div lut for different channels */
	/* DAC rates for mode 0:4 are 360/240/180/120/90mhz */
	uint8 dac_div_lut_ge_5745[] = {4, 6, 8, 12, 40};
	uint8 dac_div_lut_ge_5180[] = {4, 6, 8, 12, 14};
	uint8 dac_div_lut_ge_4905[] = {7, 5, 7, 10, 13};
	uint8 dac_div_lut_2g[] = {7, 5, 7, 10, 13};
	uint8 dac_bypN2_lut_ge_5745[] = {0, 0, 0, 0, 0};
	uint8 dac_bypN2_lut_ge_5180[] = {0, 0, 0, 0, 0};
	uint8 dac_bypN2_lut_ge_4905[] = {1, 0, 0, 0, 0};
	uint8 dac_bypN2_lut_2g[] = {1, 0, 0, 0, 0};

	/* Get center frequency of the channel */
	if (CHSPEC_CHANNEL(pi->radio_chanspec) > 14) {
		fc = CHAN5G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
	} else {
		fc = CHAN2G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
	}

	if (ulp_adc_mode == 2) {
		/* 180MHz mode */
		MOD_RADIO_REG_28NM(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_power_mode, 1);
		MOD_RADIO_REG_28NM(pi, RF, RXADC_CFG0, 0, rxadc_power_mode, 3);
		MOD_RADIO_REG_28NM(pi, RF, RXADC_REG1, 0, rxadc_reg1_15_7, 0);
		MOD_RADIO_REG_28NM(pi, RF, RXADC_REG0, 0, rxadc_reg0_7_0, 0x80);
	} else {
		MOD_RADIO_REG_28NM(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_power_mode, 1);
		MOD_RADIO_REG_28NM(pi, RF, RXADC_CFG0, 0, rxadc_power_mode, 0);
		MOD_RADIO_REG_28NM(pi, RF, RXADC_REG1, 0, rxadc_reg1_15_7, 0x6);
		MOD_RADIO_REG_28NM(pi, RF, RXADC_REG0, 0, rxadc_reg0_7_0, 0x0);
	}

	/* Based on channel frequency, select appropriate LUT */
	if (fc >= 5745) {
		adc_div_lut = adc_div_lut_ge_5745;
		dac_div_lut = dac_div_lut_ge_5745;
		adc_bypN2_lut = adc_bypN2_lut_ge_5745;
		dac_bypN2_lut = dac_bypN2_lut_ge_5745;
	} else if (fc >= 5180) {
		adc_div_lut = adc_div_lut_ge_5180;
		dac_div_lut = dac_div_lut_ge_5180;
		adc_bypN2_lut = adc_bypN2_lut_ge_5180;
		dac_bypN2_lut = dac_bypN2_lut_ge_5180;
	} else if (fc >= 4905) {
		adc_div_lut = adc_div_lut_ge_4905;
		dac_div_lut = dac_div_lut_ge_4905;
		adc_bypN2_lut = adc_bypN2_lut_ge_4905;
		dac_bypN2_lut = dac_bypN2_lut_ge_4905;
	} else {
		adc_div_lut = adc_div_lut_2g;
		dac_div_lut = dac_div_lut_2g;
		adc_bypN2_lut = adc_bypN2_lut_2g;
		dac_bypN2_lut = dac_bypN2_lut_2g;
	}

	adc_div = adc_div_lut[ulp_adc_mode];
	adc_bypN2 = adc_bypN2_lut[ulp_adc_mode];

	/* ulp_tx_mode goes from 1 to 5, subtract 1 */
	dac_div = dac_div_lut[ulp_tx_mode - 1];
	dac_bypN2 = dac_bypN2_lut[ulp_tx_mode - 1];

	if (fc < 3000) {
		/* 2G */
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_pu, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_adc, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_dac, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_adc_en, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_dac_en, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_pu, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_adc_en, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_dac_en, 0x1)

			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_REG4, 0, afediv5g_pu, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_REG5, 0, afediv5g_adc_en, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_REG5, 0, afediv5g_dac_en, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv_rst_en, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG4, 0, afediv2g_pu, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv2g_adc_en, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv2g_dac_en, 0x1)
		ACPHY_REG_LIST_EXECUTE(pi);


		if (RADIOMAJORREV(pi) >= 2) {
			//43012b0
			 //setting ntssi override to zero
			MOD_RADIO_REG_20695(pi, RF, SPARE_CFG1, 0, ovr_afediv2g_adc_Ntssi, 0);
			//setting bypN2 override to one
			MOD_RADIO_REG_20695(pi, RF, SPARE_CFG1, 0, ovr_afediv2g_adc_bypN2, 1);
		}
		//will be controled from direct line in b0

		MOD_RADIO_REG_20695_MULTI_3(pi, RF, AFEDIV2G_REG1, 0,
				afediv2g_adc_bypN2, adc_bypN2,
				afediv2g_dac_bypN2, dac_bypN2,
				afediv2g_adc_Ntssi, 0x0);
		MOD_RADIO_REG_20695(pi, RF, AFEDIV2G_REG3, 0, afediv2g_adc_div, adc_div);
		MOD_RADIO_REG_20695(pi, RF, AFEDIV2G_REG2, 0, afediv2g_dac_div, dac_div);
		MOD_RADIO_REG_20695(pi, RF, TXMIX2G_CFG4, 0, sel_lpf_CM_bias_5g, 0);
	} else {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_pu, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_adc, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_dac, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_adc_en, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0,
					ovr_afediv5g_dac_en, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_pu, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_adc, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_dac, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0,
					ovr_afediv2g_dac_en, 0x1)


			/* RST sequence */
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG4, 0, afediv2g_pu, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv2g_adc_en, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv2g_dac_en, 0x0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv_rst_en, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_REG4, 0, afediv5g_pu, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_REG5, 0, afediv5g_adc_en, 0x1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_REG5, 0, afediv5g_dac_en, 0x1)
		ACPHY_REG_LIST_EXECUTE(pi);


		if (RADIOMAJORREV(pi) >= 2) {
			//43012b0
			 //setting ntssi override to zero
			MOD_RADIO_REG_20695(pi, RF, SPARE_CFG1, 0, ovr_afediv5g_adc_Ntssi, 0);
			//setting bypN2 override to one
			MOD_RADIO_REG_20695(pi, RF, SPARE_CFG1, 0, ovr_afediv5g_adc_bypN2, 1);
		}
		//will be controled from direct line in b0


		MOD_RADIO_REG_20695_MULTI_3(pi, RF, AFEDIV5G_REG1, 0,
				afediv5g_adc_bypN2, adc_bypN2,
				afediv5g_dac_bypN2, dac_bypN2,
				afediv5g_adc_Ntssi, 0x0);
		MOD_RADIO_REG_20695(pi, RF, AFEDIV5G_REG3, 0, afediv5g_adc_div, adc_div);
		MOD_RADIO_REG_20695(pi, RF, AFEDIV5G_REG2, 0, afediv5g_dac_div, dac_div);
		MOD_RADIO_REG_20695_MULTI_2(pi, RF, TX5G_CFG1_OVR, 0, ovr_pad5g_idac_gm, 1,
				ovr_pad5g_idac_pmos, 1);
		MOD_RADIO_REG_20695(pi, RF, TXMIX2G_CFG4, 0, sel_lpf_CM_bias_5g, 1);
	}

	if (PHY_NAP_ENAB(pi->sh->physhim)) {
		/* Clear afediv_adc_en and afediv_dac_en override
		   These will be controlled using direct lines
		*/
		wlc_phy_radio20695_clear_adc_dac_en_ovr(pi);
	}
}

static int
BCMRAMFN(wlc_phy_ac_get_20695_chan_info_tbl)(phy_info_t *pi,
	const chan_info_radio20695_rffe_t **chan_info,
	uint32 *p_tbl_len)
{
	int ret_status = 0;
	uint pkg_id = pi->sh->chippkg;	/* Chip package info */

	switch (RADIOREV(pi->pubpi->radiorev)) {
		case 32:
			if ((pkg_id == BCM943012_WLBGA_PKG_ID)) {
				/* 43012A0 WLBGA */
				*chan_info = chan_tune_20695_rev32_wlbga;
				*p_tbl_len = ARRAYSIZE(chan_tune_20695_rev32_wlbga);
				ret_status = 0;
			} else if ((pkg_id == BCM943012_FCBGA_PKG_ID) ||
				(pkg_id == BCM943012_FCBGAWE_PKG_ID)) {
				/* 43012A0 FCBGA */
				*chan_info = chan_tune_20695_rev32_fcbga;
				*p_tbl_len = ARRAYSIZE(chan_tune_20695_rev32_fcbga);
				ret_status = 0;
			} else if ((pkg_id == BCM943012_WLCSPWE_PKG_ID) ||
				(pkg_id == BCM943012_WLCSPOLY_PKG_ID)) {
				/* 43012A0 WLCSP (same as FCBGA) */
				*chan_info = chan_tune_20695_rev32_fcbga;
				*p_tbl_len = ARRAYSIZE(chan_tune_20695_rev32_fcbga);
				ret_status = 0;
			} else {
				PHY_ERROR(("wl%d: %s: Unsupported package id %d\n",
					pi->sh->unit, __FUNCTION__, pkg_id));
				ROMMABLE_ASSERT(FALSE);
				ret_status = -1;
			}
			break;

		case 33:
			*chan_info = chan_tune_20695_rev33;
			*p_tbl_len = ARRAYSIZE(chan_tune_20695_rev33);
			ret_status = 0;
			break;

		case 38:
			if ((pkg_id == BCM943012_WLBGA_PKG_ID)) {
				/* 43012B0 WLBGA */
				*chan_info = chan_tune_20695_rev38_wlbga;
				*p_tbl_len = ARRAYSIZE(chan_tune_20695_rev38_wlbga);
				ret_status = 0;
			} else if ((pkg_id == BCM943012_FCBGA_PKG_ID) ||
				(pkg_id == BCM943012_FCBGAWE_PKG_ID)) {
				/* 43012B0 FCBGA */
				*chan_info = chan_tune_20695_rev38_fcbga;
				*p_tbl_len = ARRAYSIZE(chan_tune_20695_rev38_fcbga);
				ret_status = 0;
			} else if ((pkg_id == BCM943012_WLCSPWE_PKG_ID) ||
				(pkg_id == BCM943012_WLCSPOLY_PKG_ID)) {
				/* 43012B0 WLCSP (same as FCBGA) */
				*chan_info = chan_tune_20695_rev38_fcbga;
				*p_tbl_len = ARRAYSIZE(chan_tune_20695_rev38_fcbga);
				ret_status = 0;
			} else {
				PHY_ERROR(("wl%d: %s: Unsupported package id %d\n",
					pi->sh->unit, __FUNCTION__, pkg_id));
				ROMMABLE_ASSERT(FALSE);
				ret_status = -1;
			}
			break;

		default:
			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
				pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
			ASSERT(FALSE);
			ret_status = -1;
	}

	return ret_status;
}

static pll_config_20695_tbl_t *
BCMRAMFN(wlc_phy_ac_get_20695_pll_config)(phy_info_t *pi)
{
	return &pll_conf_20695;
}

static pll_config_20694_tbl_t *
BCMRAMFN(wlc_phy_ac_get_20694_pll_config)(phy_info_t *pi)
{
	return &pll_conf_20694;
}

int
wlc_phy_chan2freq_20695(phy_info_t *pi, uint8 channel,
	const chan_info_radio20695_rffe_t **chan_info)
{
	uint32 index, tbl_len;
	const chan_info_radio20695_rffe_t *p_chan_info_tbl;
	int status;

	if (!RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		PHY_ERROR(("wl%d: %s: Unsupported radio %d\n",
			pi->sh->unit, __FUNCTION__, RADIOID(pi->pubpi->radioid)));
		ASSERT(FALSE);
		return -1;
	}

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	status = wlc_phy_ac_get_20695_chan_info_tbl(pi, &p_chan_info_tbl, &tbl_len);

	if (!status) {
		for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
			index++);

		if (index >= tbl_len) {
			PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
				pi->sh->unit, __FUNCTION__, channel));
			if (!ISSIM_ENAB(pi->sh->sih)) {
				/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(index < tbl_len);
			}
			return -1;
		}

		*chan_info = p_chan_info_tbl + index;
		return p_chan_info_tbl[index].freq;
	} else {
		return -1;
	}
}

static void
chanspec_tune_radio_20695(phy_info_t *pi)
{
	uint16 bbmult, bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ? pi->dacratemode2g : pi->dacratemode5g;

	/* Configure ulp tx mode */
	wlc_phy_dac_rate_mode_acphy(pi, pi_ac->dac_mode);

	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		wlc_phy_get_tx_bbmult_acphy(pi, &bbmult, 0);
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, 0);
		wlc_phy_radio_afecal(pi);
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, 0);
	}

	/* Configure DAC buffer BW */
	if (PHY_PAPDEN(pi)) {
		wlc_phy_radio20695_txdac_bw_setup(pi, BUTTERWORTH, DACBW_120);
	} else {
		wlc_phy_radio20695_txdac_bw_setup(pi, BUTTERWORTH, DACBW_30);
	}

	/* forcing reg bits for ADC power up  */
	MOD_RADIO_REG_28NM(pi, RF, RXADC_REG0, 0, rxadc_force_resetb, 1);
	MOD_RADIO_REG_28NM(pi, RF, RXADC_REG1, 0, rxadc_pu_clkgen, 1);
	MOD_RADIO_REG_28NM(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_rx_bq1, 1);
	MOD_RADIO_REG_28NM(pi, RF, TIA_REG18, 0, tia_sw_rx_bq1, 1);
}

static void
wlc_phy_radio20695_clear_adc_dac_en_ovr(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID));

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0, ovr_afediv5g_adc_en, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV5G_CFG1_OVR, 0, ovr_afediv5g_dac_en, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0, ovr_afediv2g_adc_en, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0, ovr_afediv2g_dac_en, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0, ovr_afediv_rst_en, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_CFG1_OVR, 0, ovr_afediv_reset, 0)
	ACPHY_REG_LIST_EXECUTE(pi);
}

static void
wlc_phy_radio20695_minipmu_pupd_seq(phy_info_t *pi, uint8 pupdbit)
{
	uint8 core;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID));

	if (pupdbit == 1) {
		PHY_INFORM(("wl%d: %s powering up 20695 minipmu", pi->sh->unit,
				__FUNCTION__));

		MOD_RADIO_REG_28NM(pi, RFP, BG_REG12, 0, ldo1p8_puok_en, 0x1);
		MOD_RADIO_REG_28NM(pi, RFP, LDO1P8_STAT, 0, ldo1p8_pu, 0x1);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG2, 0, bg_pu_bgcore, 0x1);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG2, 0, bg_pu_V2I, 0x1);

		/* 30 us delay to allow LDO to settle */
		OSL_DELAY(30);

		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, vref_select, 0);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG5, core, wlpmu_en, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG5, core, wlpmu_LDO2P2_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_AFEldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_TXldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_LOGENldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_TXldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, RX2G_REG4, core, rx2g_ldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, RX5G_REG5, core, rx5g_ldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, LDO1P65_STAT, core, ldo1p65_pu, 0x1);
		}
	} else {
		PHY_INFORM(("wl%d: %s powering down 20695 minipmu", pi->sh->unit,
				__FUNCTION__));

		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_28NM(pi, RF, LDO1P65_STAT, core, ldo1p65_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, RX5G_REG5, core, rx5g_ldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, RX2G_REG4, core, rx2g_ldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_TXldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_LOGENldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_TXldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, core, wlpmu_AFEldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG5, core, wlpmu_LDO2P2_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RF, PMU_CFG5, core, wlpmu_en, 0x0);
		}

		MOD_RADIO_REG_28NM(pi, RFP, BG_REG2, 0, bg_pu_V2I, 0x0);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG2, 0, bg_pu_bgcore, 0x0);
		MOD_RADIO_REG_28NM(pi, RFP, LDO1P8_STAT, 0, ldo1p8_pu, 0x0);
	}
}

/* PLL PWR-UP */
static void wlc_phy_radio20696_pmu_pll_pwrup(phy_info_t *pi)
{
	MOD_RADIO_PLLREG_20696(pi, PLL_HVLDO1, ldo_1p8_pu_ldo_CP, 0x1);	/* rfpll cp ldo pu */
	MOD_RADIO_PLLREG_20696(pi, PLL_HVLDO1, ldo_1p8_pu_ldo_VCO, 0x1); /* rfpll vco ldo pu */

	/* Overwrites */
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_ldo_1p8_bias_reset_CP, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_ldo_1p8_bias_reset_VCO, 0x1);

	/* Assert Bias reset */
	MOD_RADIO_PLLREG_20696(pi, PLL_HVLDO1, ldo_1p8_bias_reset_CP, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_HVLDO1, ldo_1p8_bias_reset_VCO, 0x1);
	OSL_DELAY(1000); // Luka: was 89us

	/* Deasssert Bias Reset */
	MOD_RADIO_PLLREG_20696(pi, PLL_HVLDO1, ldo_1p8_bias_reset_CP, 0x0);
	MOD_RADIO_PLLREG_20696(pi, PLL_HVLDO1, ldo_1p8_bias_reset_VCO, 0x0);

	/* Deassert Bias Reset overwrites	 */
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_ldo_1p8_bias_reset_CP, 0x0);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_ldo_1p8_bias_reset_VCO, 0x0);
}

/* Work-arounds */
static void wlc_phy_radio20696_wars(phy_info_t *pi)
{
	uint8 core;

	PHY_INFORM(("%s\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, i_configRX_pd_adc_clk, 0x0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 1);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 1);

		WRITE_RADIO_REG_20696(pi, RXADC_CFG0, core, 0x285c);
		WRITE_RADIO_REG_20696(pi, RXADC_CFG1, core, 0x57ff);
		WRITE_RADIO_REG_20696(pi, RXADC_CFG2, core, 0x007f);
		WRITE_RADIO_REG_20696(pi, RXADC_CFG3, core, 0x33e);
		WRITE_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, 0xcc77);

		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 1);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 1);

		/* 7271: BUGFIX - HWJUSTY-285 TxEVM in 5GHz influenced by ovr_auxpga_i_pu */
		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1);

		/* HWJUSTY-272 part 1 */
		MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 1);
	}
	/* HWJUSTY-272 part 2, improve LOFT comp range */
	MOD_PHYREG(pi, Core1TxControl, loft_comp_shift, 1);
	MOD_PHYREG(pi, Core2TxControl, loft_comp_shift, 1);
	MOD_PHYREG(pi, Core3TxControl, loft_comp_shift, 1);
	MOD_PHYREG(pi, Core4TxControl, loft_comp_shift, 1);

	/* 7271: HWJUSTY-259 missing this reg write causes Tx tone to fail on all cores */
	MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG5, 3, logen_mux_pu, 1);

	/* 7271: frontLDO may be unstabe to power down on 28nm radios */
	MOD_RADIO_PLLREG_20696(pi, FRONTLDO_OVR0, ovr_LDO1P2_BG_en_1p0, 0x1);
	MOD_RADIO_PLLREG_20696(pi, FRONTLDO_OVR0, ovr_LDO1P2_cntl_en_1p0, 0x1);
	MOD_RADIO_PLLREG_20696(pi, FRONTLDO_OVR0, ovr_LDO1P2_pu_1p0, 0x1);
	MOD_RADIO_PLLREG_20696(pi, FRONTLDO_REG0, LDO1P2_BG_en_1p0, 0x1);
	MOD_RADIO_PLLREG_20696(pi, FRONTLDO_REG0, LDO1P2_cntl_en_1p0, 0x1);
	MOD_RADIO_PLLREG_20696(pi, FRONTLDO_REG0, LDO1P2_pu_1p0, 0x1);
}

static void wlc_phy_radio20696_refclk_en(phy_info_t *pi)
{
	MOD_RADIO_PLLREG_20696(pi, PLL_REFDOUBLER1, RefDoubler_pu, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_RefDoubler_pu_pfddrv, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_REFDOUBLER3, RefDoubler_pu_pfddrv, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_REFDOUBLER2, RefDoubler_pu_clkvcocal, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_REFDOUBLER2, RefDoubler_pu_clkrcal, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_REFDOUBLER2, RefDoubler_pu_clkrccal, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR2, ovr_RefDoubler_ldo_1p8_pu_ldo, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_REFDOUBLER1, RefDoubler_ldo_1p8_pu_ldo, 0x1);
}


/* Load additional perfer value after power up */
static void wlc_phy_radio20696_upd_prfd_values(phy_info_t *pi)
{
	uint8 core;
	uint i = 0;
	radio_20xx_prefregs_t *prefregs_20696_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20696_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
	case 0: prefregs_20696_ptr = prefregs_20696_rev0;
		break;

	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return;
	}

	/* Update preferred values */
	while (prefregs_20696_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20696_ptr[i].address,
			(uint16)prefregs_20696_ptr[i].init);
		i++;
	}

	wlc_phy_radio20696_refclk_en(pi);

	/* turn ON Common LOGen pu's */
	MOD_RADIO_PLLREG_20696(pi, LOGEN_REG0, logen_vcobuf_pu, 0x1);
	MOD_RADIO_PLLREG_20696(pi, LOGEN_REG0, logen_pu, 0x1);
	MOD_RADIO_REG_20696(pi, BG_REG10, 1, bg_wlpmu_pu_cbuck_ref, 0x1);
	/* this is not working in veloce when placed here - pmu cal failed? */
	MOD_RADIO_REG_20696(pi, RCCAL_CFG0, 1, rccal_pu, 0x0);

	FOREACH_CORE(pi, core) {
		/* logen */
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_lc_pu, 0x1);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_gm_pu, 0x1);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_bias_pu, 0x1);
		/* clk_div */
		MOD_RADIO_REG_20696(pi, AFEDIV_CFG1_OVR, core, ovr_afediv_inbuf_pu, 0x1);
		MOD_RADIO_REG_20696(pi, AFEDIV_REG0, core, afediv_inbuf_pu, 0x1);

		/* lpf bias pu */
		MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_bias_pu, 0x1);
		MOD_RADIO_REG_20696(pi, LPF_REG6, core, lpf_bias_pu, 0x1);
		MOD_RADIO_REG_20696(pi, LPF_OVR1, 0, ovr_lpf_bq_pu, 0x1);
		MOD_RADIO_REG_20696(pi, LPF_REG6, 0, lpf_bq_pu, 0x1);

		/* new optimised 2G settings */
		/* Tx */
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG0, core, tx2g_mx_gm_offset_en, 1);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG1, core, tx2g_mx_idac_mirror_cas, 4);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG1, core, tx2g_mx_idac_mirror_cas_bb, 4);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG1, core, tx2g_mx_idac_mirror_cas_replica, 4);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG3, core, tx2g_mx_idac_cas_off, 9);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG3, core, tx2g_mx_idac_mirror_lo, 3);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG1, core, tx2g_pad_idac_cas_off, 9);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG3, core, tx2g_pad_idac_cas, 6);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_pa2g_tssi_ctrl_sel, 1);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_pa2g_bias_bw, 1);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_tx2g_mx_bias_reset_cas, 1);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_tx2g_mx_bias_reset_lo, 1);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_tx2g_mx_bias_reset_bbdc, 1);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_pa2g_tssi_ctrl_range, 1);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_tx2g_pad_bias_reset_gm, 1);
		MOD_RADIO_REG_20696(pi, TX2G_CFG1_OVR, core, ovr_tx2g_pad_bias_reset_cas, 1);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG3, core, pad2g_tune, 7);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG4, core, mx2g_tune, 4);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG1, core, tx2g_pad_idac_gm, 4);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG2, core, tx2g_mx_idac_bb, 11);
		/* Rx */
		MOD_RADIO_REG_20696(pi, RX2G_REG5, core, rx2g_wrssi0_low_pu, 1);
		MOD_RADIO_REG_20696(pi, RX2G_REG5, core, rx2g_wrssi0_mid_pu, 1);
		MOD_RADIO_REG_20696(pi, RX2G_REG5, core, rx2g_wrssi0_high_pu, 1);
		MOD_RADIO_REG_20696(pi, RX2G_REG5, core, rx2g_wrssi1_low_pu, 1);
		MOD_RADIO_REG_20696(pi, RX2G_REG5, core, rx2g_wrssi1_mid_pu, 1);
		MOD_RADIO_REG_20696(pi, RX2G_REG5, core, rx2g_wrssi1_high_pu, 1);
		MOD_RADIO_REG_20696(pi, RX2G_REG2, core, rx2g_gm_idac_nmos_ds, 0);
		MOD_RADIO_REG_20696(pi, RX2G_REG2, core, rx2g_gm_idac_pmos_ds, 0);

		/* new optimised 5G settings */
		/* Tx */
		MOD_RADIO_REG_20696(pi, TX5G_MIX_GC_REG, core, tx5g_mx_gc, 1);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG1, core, tx5g_idac_mx5g_tuning, 9);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG1, core, tx5g_idac_mx_bbdc, 26);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG2, core, tx5g_idac_mx_lodc, 15);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG1, core, tx5g_idac_pad_main, 28);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG1, core, tx5g_idac_pad_main_slope, 3);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG1, core, tx5g_idac_pad_cas_bot, 15);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG2, core, tx5g_idac_pad_tuning, 8);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG1, core, tx5g_idac_pa_cas, 35);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG2, core, tx5g_idac_pa_incap_compen, 32);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG1, core, tx5g_idac_pa_main, 52);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG3, core, tx5g_idac_pa_tuning, 11);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG1, core, tx5g_mx_de_q, 0);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG3, core, tx5g_pad_de_q, 0);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG3, core, tx5g_pad_max_cas_bias, 0);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG3, core, tx5g_pad_max_pmos_vbias, 0);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG4, core, tx5g_pa_max_cas_vbias, 0);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG4, core, tx5g_pa_max_pmos_vbias, 0);
		MOD_RADIO_REG_20696(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_range, 1);
		MOD_RADIO_REG_20696(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl_range, 1);
		/* Rx */
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI, core, rx5g_wrssi0_low_pu, 1);
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI, core, rx5g_wrssi0_mid_pu, 1);
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI, core, rx5g_wrssi0_high_pu, 1);
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI, core, rx5g_wrssi1_low_pu, 1);
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI, core, rx5g_wrssi1_mid_pu, 1);
		MOD_RADIO_REG_20696(pi, RX5G_WRSSI, core, rx5g_wrssi1_high_pu, 1);
	}
}

static void
wlc_phy_28nm_radio_pwron_seq_phyregs(phy_info_t *pi, uint8 core)
{
	/* # power down everything */
	/* do not touch bg_pulse */
	WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, 0);
	WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, 0x1ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/* Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/* Update preferred values */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		wlc_phy_radio20695_upd_prfd_values(pi);
	}

	/* Make RfctrlCmd.chip_pu = 1
	   This indicates WLAN RF active
	   This is required for coex to work
	*/
	MOD_PHYREG(pi, RfctrlCmd, chip_pu, 1);
}

static radio_20xx_prefregs_t *
BCMRAMFN(wlc_phy_ac_get_20695_prfd_vals_tbl)(phy_info_t *pi)
{
	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
		case 32:
			return prefregs_20695_rev32;
		case 33:
			return prefregs_20695_rev33;
		case 38:
			return prefregs_20695_rev38;

		default:
			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
				pi->sh->unit, __FUNCTION__,
				RADIOREV(pi->pubpi->radiorev)));
			ASSERT(FALSE);
			return NULL;
	}
}

static void
wlc_phy_radio20695_upd_prfd_values(phy_info_t *pi)
{
	radio_20xx_prefregs_t *prefregs_20695_ptr = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID));

	if ((prefregs_20695_ptr = wlc_phy_ac_get_20695_prfd_vals_tbl(pi)) == NULL) {
		PHY_ERROR(("wl%d: %s: Preferred value table not known for radio_rev %d\n",
				pi->sh->unit, __FUNCTION__,
				RADIOREV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return;
	}

	/* Update preferred values */
	wlc_phy_init_radio_prefregs_allbands(pi, prefregs_20695_ptr);


}

static void
wlc_phy_switch_radio_acphy_20695(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID));

	/* RCAL */
#ifdef ATE_BUILD
	wlc_phy_28nm_radio_rcal(pi, 2);
#else
	wlc_phy_28nm_radio_rcal(pi, 1);
#endif


	/* minipmu_cal */
	wlc_phy_28nm_radio_minipmu_cal(pi);

	/* Xtal settings for better phase noise */
	ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL_OVR1, 0, ovr_xtal_bias_adj, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL_OVR1, 0,
				ovr_xtal_core_strength_nmos, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL_OVR1, 0,
				ovr_xtal_core_strength_pmos, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL_OVR1, 0, ovr_xtal_sel_bias_res, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL_OVR1, 0, ovr_xt_res_bypass, 0x1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL3, 0, xtal_bias_adj, 0x4)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL2, 0, xtal_coresize_nmos, 0x10)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL2, 0, xtal_coresize_pmos, 0x10)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL1, 0, xt_res_bypass, 0x4)
		MOD_RADIO_REG_28NM_ENTRY(pi, RFP, XTAL3, 0, xtal_sel_bias_res, 0x4)
	ACPHY_REG_LIST_EXECUTE(pi);
	/* Modifying PMU registers which impact XTAL settings mentioned above */
	chanspec_setup_xtal_20695(pi);

}

int8
wlc_phy_28nm_radio_minipmu_cal(phy_info_t *pi)
{
	uint8 calsuccesful = 1;
	int8 cal_status = 1;
	int16 waitcounter = 0;
	uint16 temp_code;
	uint32 save_gcichipctrl7;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID));

	/* Enable the PMU cal clock using GCI Control 7 Register (set bit 13 and 14) */
	save_gcichipctrl7 = si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_07, 0, 0);
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_07, (0x3 << 13), (0x3 << 13));

	/* Setup the cal */
	MOD_RADIO_REG_28NM(pi, RFP, BG_OVR1, 0, ovr_pmu_bg_wlpmu_en_i, 0x1);
	MOD_RADIO_REG_28NM(pi, RFP, BG_OVR1, 0, ovr_bgpmucal_pu, 0x1);
	WRITE_RADIO_REG_28NM(pi, RFP, BG_REG10, 0, 0x37);

	/* 25 us delay to allow PMU cal to happen */
	OSL_DELAY(25);

	while (READ_RADIO_REGFLD_28NM(pi, RFP, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		/* 10 us delay wait time */
		OSL_DELAY(10);
		waitcounter++;

		if (waitcounter > 5) {
			PHY_TRACE(("\nWarning:Mini PMU Cal Failed on Core0"));
			calsuccesful = 0;
			ROMMABLE_ASSERT(0);
			break;
		}
	}

	/* Cleanup */
	WRITE_RADIO_REG_28NM(pi, RFP, BG_REG10, 0, 0x6C);
	MOD_RADIO_REG_28NM(pi, RF, PMU_CFG1, 0, vref_select, 0x1);

	if (calsuccesful == 1) {
		PHY_TRACE(("MiniPMU cal done on core0, Cal code = %d",
			READ_RADIO_REGFLD_28NM(pi, RFP, BG_REG11, 0, bg_wlpmu_calcode)));

		temp_code = READ_RADIO_REGFLD_28NM(pi, RFP, BG_REG11, 0, bg_wlpmu_calcode);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG9, 0, bg_wlpmu_cal_mancodes, temp_code);
	} else {
		PHY_TRACE(("Cal Unsuccesful on Core0"));
		cal_status = -1;
	}

	/* Restore gcichipctrl 7 reg */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_07, 0xffffffff, save_gcichipctrl7);

	return cal_status;
}

/*  R Calibration (takes ~50us) */
static void
wlc_phy_28nm_radio_rcal(phy_info_t *pi, uint8 mode)
{
	/* Format: 20695_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* The rcalcode argument works only with mode 1 and is optional. */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_valid, loop_iter, rcal_value;
	uint8 core = 0;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID));

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_REG_28NM(pi, RFP, BG_OVR1, core, ovr_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x36 */
		rcal_value = 0x36;
		MOD_RADIO_REG_28NM(pi, RFP, BG_OVR1, core, ovr_otp_rcal_sel, 0);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG8, core, bg_rcal_trim, rcal_value);
		MOD_RADIO_REG_28NM(pi, RFP, BG_OVR1, core, ovr_bg_rcal_trim, 1);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG1, core, bg_ate_rcal_trim, rcal_value);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG2, core, bg_ate_rcal_trim_en, 1);
	} else if (mode == 2) {
		/* This should run only for core 0 */
		/* Run R Cal and use its output */

		MOD_RADIO_REG_20695_MULTI_2(pi, RFP, XTAL5, core, xtal_pu_wlandig, 1,
			xtal_pu_RCCAL, 1);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG2, core, bg_ate_rcal_trim_en, 0x0);
		/* Make connection with the external 10k resistor */
		/* Also powers up rcal (RF0_rcal_cfg_north.rcal_pu = 1) */

		MOD_RADIO_REG_20695_MULTI_2(pi, RFP, GPAIO_SEL2, core, gpaio_rcal_pu, 0,
			gpaio_pu, 1);
		MOD_RADIO_REG_28NM(pi, RFP, GPAIO_SEL0, core,
				gpaio_sel_0to15_port, 0x0);
		MOD_RADIO_REG_28NM(pi, RFP, GPAIO_SEL1, core,
				gpaio_sel_16to31_port, 0x0);
		MOD_RADIO_REG_28NM(pi, RFP, GPAIO_SEL3, core,
				gpaio_sel_32to47_port, 0x0);
		MOD_RADIO_REG_28NM(pi, RFP, GPAIO_SEL2, core,
				gpaio_rcal_pu, 1);
		MOD_RADIO_REG_28NM(pi, RFP, BG_REG3, core,
				bg_rcal_pu, 1);
		MOD_RADIO_REG_28NM(pi, RF, RCAL_CFG_NORTH, core,
				rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 10)) {
			/* 1 ms delay wait time */
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_REGFLD_28NM(pi, RF, RCAL_CFG_NORTH, core,
					rcal_valid);
			loop_iter++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_REGFLD_28NM(pi, RF, RCAL_CFG_NORTH, core,
					rcal_value);
#ifdef ATE_BUILD
			ate_buffer_regval[0].rcal_value[0] = rcal_value;
#endif
			/* Use the output of the rcal engine */
			/* Don't use OTP value */
			MOD_RADIO_REG_28NM(pi, RFP, BG_OVR1, core, ovr_otp_rcal_sel, 0);
			MOD_RADIO_REG_28NM(pi, RFP, BG_REG8, core, bg_rcal_trim, rcal_value);
			MOD_RADIO_REG_28NM(pi, RFP, BG_OVR1, core, ovr_bg_rcal_trim, 1);


			MOD_RADIO_REG_28NM(pi, RFP, BG_REG2, core, bg_ate_rcal_trim_en, 1);
			MOD_RADIO_REG_28NM(pi, RFP, BG_REG1, core, bg_ate_rcal_trim, rcal_value);

			/* Very coarse sanity check */
			if ((rcal_value < 2) || (12 < rcal_value)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				" 4bit Rcal = %d.\n", rcal_value));
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
				__FUNCTION__, rcal_valid));
		}

		/* Power down blocks not needed anymore */
		MOD_RADIO_REG_28NM(pi, RF, RCAL_CFG_NORTH, core,
				rcal_pu, 0);
	}
}

static void
wlc_phy_28nm_radio_rccal(phy_info_t *pi)
{
	uint8 cal, done;
	uint16 rccal_itr, n0, n1;

	/* lpf, dacbuf */
	uint8 sr[] = {0x1, 0x0};
	uint8 sc[] = {0x0, 0x1};

	/*  Q1 = 0 -> gmult_dacbuf_k = 154142
	 *  Q1 = 1 -> gmult_dacbuf_k = 77071
	 *  Q1 = 2 -> gmult_dacbuf_k = 38535
	*/
	uint32 gmult_dacbuf_k = 77071;

	/*  Q1 = 0 -> gmult_lpf_k = 149600
	 *  Q1 = 1 -> gmult_lpf_k = 74800
	 *  Q1 = 2 -> gmult_lpf_k = 37400
	*/
	uint32 gmult_lpf_k    = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = &(pi->u.pi_acphy->radioi->data);
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;

	if (ISSIM_ENAB(pi->sh->sih)) {
		return;
	}

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_REG_28NM(pi, RFP, XTAL5, 0, xtal_pu_RCCAL, 1);
	MOD_RADIO_REG_28NM(pi, RFP, XTAL5, 0, xtal_pu_wlandig, 1);

	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < 2; cal++) {
		/* Setup */
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG0, 0, rccal_sr, sr[cal]);
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG0, 0, rccal_sc, sc[cal]);
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG1, 0, rccal_Q1, 0x1);


		/* Toggle RCCAL power */
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG0, 0, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG0, 0, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG1, 0, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
			(rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
			rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_28NM(pi, RFP, RCCAL_CFG3, 0, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG1, 0, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		n0 = READ_RADIO_REGFLD_28NM(pi, RFP, RCCAL_CFG4, 0, rccal_N0);
		n1 = READ_RADIO_REGFLD_28NM(pi, RFP, RCCAL_CFG5, 0, rccal_N1);
		dn = n1 - n0; /* set dn [expr {$N1 - $N0}] */

		if (cal == 0) {
			/* lpf  values */
			data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
			data->rccal_gmult_rc = data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {
			/* dacbuf  */
			gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
			pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24) / (gmult_dacbuf);
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_28NM(pi, RFP, RCCAL_CFG0, 0, rccal_pu, 0);

	}
	/* Powerdown rccal driver */
	MOD_RADIO_REG_28NM(pi, RFP, XTAL5, 0, xtal_pu_RCCAL, 0);

	/* nominal values when rc cal failes  */
	if (done == 0) {
		pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
		data->rccal_gmult_rc     = 4260;
	}

	/* Programming lpf gmult and tia gmult */
	MOD_RADIO_REG_28NM(pi, RF, TIA_GMULT_BQ1, 0, tia_gmult_bq1, data->rccal_gmult_rc);
	MOD_RADIO_REG_28NM(pi, RF, TIA_GMULT_BQ2, 0, tia_gmult_bq2, data->rccal_gmult_rc);
#ifdef ATE_BUILD
	ate_buffer_regval[0].gmult_lpf = data->rccal_gmult;
	ate_buffer_regval[0].rccal_dacbuf_cmult = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;
#endif

}

static void
wlc_phy_28nm_radio_pll_logen_pupd_seq(phy_info_t *pi, pll_logen_block_20695_t block, uint8 pupdbit,
		uint8 core)
{
	if (pupdbit == 1) {
		if (block == PLL2G) {
			PHY_INFORM(("wl%d: %s powering up 20695 core%d 2G PLL\n",
					pi->sh->unit, __FUNCTION__, core));

			/* Set rfpll_vcocal_rst_n to 0 using override mode */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_VCOCAL1, core, rfpll_vcocal_rst_n, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_VCOCAL_OVR1, core,
					ovr_rfpll_vcocal_rst_n, 0x1);

			/* Power up pfd LDO 10 us before synth PU */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_PFDLDO1, core, rfpll_pfd_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core,
					ovr_rfpll_pfd_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO2, core,
					rfpll_rfvco_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core,
					ovr_rfpll_vco_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_PFDLDO1, core, rfpll_pfd_ldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_vco_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_vco_pu, 0x1);

			/* Wait for 10 us before powering up Synth */
			OSL_DELAY(10);

			/* reset -> high */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO2, core, rfpll_rfvco_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_vco_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CP1, core, rfpll_cp_bias_reset, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_cp_bias_reset, 0x1);

			/* Power up PLL */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_synth_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_synth_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_vco_buf_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_vco_buf_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_mmd_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_mmd_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO4, core, rfpll_vcobuf2g_sel2g, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_vcobuf2g_sel2g, 0x1);

			/* Wait for 10 us before pulling reset low */
			OSL_DELAY(10);

			/* reset -> low */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_PFDLDO1, core, rfpll_pfd_ldo_bias_rst, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO2, core,
					rfpll_rfvco_ldo_bias_rst, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO2, core, rfpll_rfvco_bias_rst, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CP1, core, rfpll_cp_bias_reset, 0x0);
		} else if (block == PLL5G) {
			PHY_INFORM(("wl%d: %s powering up 20695 core%d 5G PLL\n",
					pi->sh->unit, __FUNCTION__, core));

			/* Set rfpll_vcocal_rst_n to 0 using override mode */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_VCOCAL1, core,
					rfpll_5g_vcocal_rst_n, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_VCOCAL_OVR1, core,
					ovr_rfpll_5g_vcocal_rst_n, 0x1);

			/* Power up pfd LDO 10 us before synth PU */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_PFDLDO1, core,
					rfpll_5g_pfd_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core,
					ovr_rfpll_5g_pfd_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_RFVCO2, core,
					rfpll_5g_rfvco_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core,
					ovr_rfpll_5g_vco_ldo_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_PFDLDO1, core, rfpll_5g_pfd_ldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_PFDLDO1, core, rfpll_pfd_ldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG1, core, rfpll_5g_vco_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core, ovr_rfpll_5g_vco_pu, 0x1);

			/* Wait for 10 us before powering up Synth */
			OSL_DELAY(10);

			/* reset -> high */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_RFVCO2, core,
					rfpll_5g_rfvco_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core,
					ovr_rfpll_5g_vco_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CP1, core, rfpll_5g_cp_bias_rst, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core,
					ovr_rfpll_5g_cp_bias_reset, 0x1);

			/* Power up PLL */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG1, core, rfpll_5g_synth_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core, ovr_rfpll_5g_synth_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG1, core, rfpll_5g_mmd_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core, ovr_rfpll_5g_mmd_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG6, core, rfpll_5g_div_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core, ovr_rfpll_5g_div_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_RFVCO2, core,
					rfpll_5g_rfvco_VthM_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core,
					ovr_rfpll_5g_rfvco_VthM_pu, 0x1);

			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CP1, core, rfpll_5g_cp_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_LF6, core, rfpll_5g_lf_cm_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG1, core, rfpll_5g_pfd_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG6, core, rfpll_5g_frct_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core, ovr_rfpll_5g_pfd_ldo_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_PFDLDO1, core, rfpll_5g_pfd_ldo_pu, 0x1);

			/* Wait for 10 us before pulling reset low */
			OSL_DELAY(10);

			/* reset -> low */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_PFDLDO1, core,
					rfpll_5g_pfd_ldo_bias_rst, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_RFVCO2, core,
					rfpll_5g_rfvco_ldo_bias_rst, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_RFVCO2, core,
					rfpll_5g_rfvco_bias_rst, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CP1, core, rfpll_5g_cp_bias_rst, 0x0);
		} else if (block == LOGEN5G) {
			PHY_INFORM(("wl%d: %s powering down 20695 core%d 5G LOGEN\n",
					pi->sh->unit, __FUNCTION__, core));
			/* Power up LOGEN */
			MOD_RADIO_REG_28NM(pi, RF, LOGEN5G_REG4, core, logen5g_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, LOGEN_OVR1, core, ovr_logen5g_pu, 0x1);
		} else if (block == LOGEN5GTO2G) {
			PHY_INFORM(("wl%d: %s powering down 20695 core%d 5G LOGEN to 2G\n",
					pi->sh->unit, __FUNCTION__, core));
			/* Power up LOGEN */
			MOD_RADIO_REG_28NM(pi, RF, LO5GTO2G_CFG3, core, lo5g_to_2g_pu, 0x1);
			MOD_RADIO_REG_28NM(pi, RF, LOGEN_OVR1, core, ovr_lo5g_to_2g_pu, 0x1);
		}
	} else {
		if (block == PLL2G) {
			PHY_INFORM(("wl%d: %s powering down 20695 core%d 2G PLL\n",
					pi->sh->unit, __FUNCTION__, core));

			/* Power down synth */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO4, core, rfpll_vcobuf2g_sel2g, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_mmd_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_vco_buf_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_synth_pu, 0x0);

			/* Power down LDO */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_PFDLDO1, core, rfpll_pfd_ldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_vco_pu, 0x0);
		} else if (block == PLL5G) {
			PHY_INFORM(("wl%d: %s powering down 20695 core%d 5G PLL\n",
					pi->sh->unit, __FUNCTION__, core));

			/* Power down synth */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_RFVCO2, core,
					rfpll_5g_rfvco_VthM_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG6, core, rfpll_5g_div_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG1, core, rfpll_5g_mmd_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG1, core, rfpll_5g_synth_pu, 0x0);

			/* Power down LDO */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_PFDLDO1, core, rfpll_5g_pfd_ldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_PFDLDO1, core, rfpll_pfd_ldo_pu, 0x0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG1, core, rfpll_5g_vco_pu, 0x0);
		} else if (block == LOGEN5G) {
			PHY_INFORM(("wl%d: %s powering down 20695 core%d 5G LOGEN\n",
					pi->sh->unit, __FUNCTION__, core));
			/* Power down LOGEN */
			MOD_RADIO_REG_28NM(pi, RF, LOGEN5G_REG4, core, logen5g_pu, 0x0);
		} else if (block == LOGEN5GTO2G) {
			PHY_INFORM(("wl%d: %s powering down 20695 core%d 5G LOGEN to 2G\n",
					pi->sh->unit, __FUNCTION__, core));
			/* Power down LOGEN */
			MOD_RADIO_REG_28NM(pi, RF, LO5GTO2G_CFG3, core, lo5g_to_2g_pu, 0x0);
		}
	}
}

static void
wlc_phy_chanspec_radio20695_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset, uint8 core)
{
	const chan_info_radio20695_rffe_t *chan_info;

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID));

	if (wlc_phy_chan2freq_20695(pi, ch, &chan_info) < 0) {
		return;
	}

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, core);
	}

	/* pll tuning */
	pi->u.pi_acphy->pll_sel = wlc_phy_radio20695_pll_tune(pi, chan_info->freq, core);

	/* rffe tuning */
	wlc_phy_radio20695_rffe_tune(pi, chan_info, core);

	/* Do VCO cal based on PLL */
	wlc_phy_28nm_radio_vcocal(pi, VCO_CAL_MODE_20695,
			VCO_CAL_COUPLING_MODE_20695);

	if (chan_info->freq > 4000) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv2g_adc_en, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG5, 0, afediv2g_dac_en, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFEDIV2G_REG4, 0, afediv2g_pu, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX5G_CFG1_OVR, 0,
					ovr_pad5g_idac_gm, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TX5G_CFG1_OVR, 0,
					ovr_pad5g_idac_pmos, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TXMIX2G_CFG4, 0,
					sel_lpf_CM_bias_5g, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, MX5G_CFG1, 0,
					mx5g_gm_gc, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, MX5G_CFG2, 0,
					mx5g_idac_bbdc, 0x1f)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

void
wlc_phy_chanspec_radio20696_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset)
{
	const chan_info_radio20696_rffe_t *chan_info;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20696_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20696(pi, ch, &chan_info) < 0)
		return;

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	/* rffe tuning */
	wlc_phy_radio20696_rffe_tune(pi, chan_info);

	/* band related settings */
	wlc_phy_radio20696_upd_band_related_reg(pi);

	/* pll tuning */
	wlc_phy_radio20696_pll_tune(pi, chan_info->freq);

	/* Do a VCO cal */
	wlc_phy_20696_radio_vcocal(pi, VCO_CAL_MODE_20696, VCO_CAL_COUPLING_MODE_20696);
}


static void
wlc_phy_radio20695_pll_logen_pu_config(phy_info_t *pi, uint32 lo_freq, uint8 core)
{
	/* Based on fc and USE_5G_PLL_FOR_2G, decide the required PLL to be on */
	if (lo_freq > 4000) {
		/* Aband
		Normal mode: 5G PLL is used for 5G. There is no other option
		*/
		MOD_RADIO_REG_28NM(pi, RF, PLL_CRISSCROSS_5GAS2G, core,
				internal_direct_rfpll_5g_as_2g, 0);
		MOD_RADIO_REG_28NM(pi, RFP, PLL_CRISSCROSS_5GAS2G, core,
				internal_direct_rfpll_5g_as_2g, 0);
		wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5GTO2G, 0, 0);
		wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL2G, 0, 0);
		wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL5G, 1, 0);
		wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5G, 1, 0);

		MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG6, core, rfpll_5g_div_2G_buf_pu, 0);
		MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core, ovr_rfpll_5g_div_2G_buf_pu, 0x1);
	} else {
		/* Gband */
		if (pi->u.pi_acphy->radioi->data.use_5g_pll_for_2g == 0) {
			/* Normal mode:  2G PLL is used for 2G
			NO Logen.
			*/
			MOD_RADIO_REG_28NM(pi, RF, PLL_CRISSCROSS_5GAS2G, core,
					internal_direct_rfpll_5g_as_2g, 0);

			MOD_RADIO_REG_28NM(pi, RFP, PLL_CRISSCROSS_5GAS2G, core,
					internal_direct_rfpll_5g_as_2g, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5GTO2G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL5G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL2G, 1, 0);
		} else {
			/* Special mode: 5G PLL is used for 2G
			5G PLL output gets divided by 2 and then goes to 2G LOgen.
			*/
			lo_freq *= 2;
			MOD_RADIO_REG_28NM(pi, RF, PLL_CRISSCROSS_5GAS2G, core,
					internal_direct_rfpll_5g_as_2g, 1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CRISSCROSS_5GAS2G, core,
					internal_direct_rfpll_5g_as_2g, 1);

			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL2G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL5G, 1, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5GTO2G, 1, 0);

			/* Need these additional PUs if using 5G PLL for 2G */
			/* Turn on 2G VCO LDO and VCO buf */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_vco_pu, 1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_vco_pu, 1);

			MOD_RADIO_REG_28NM(pi, RFP, PLL_CFG1, core, rfpll_vco_buf_pu, 1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_vco_buf_pu, 1);

			/* Use 5G PLL output for LO */
			MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO4, core, rfpll_vcobuf2g_sel2g, 0);
			MOD_RADIO_REG_28NM(pi, RFP, PLL_OVR1, core, ovr_rfpll_vcobuf2g_sel2g, 1);

			/* Turn on 5G PLL div2 output for 2G operation */
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_CFG6, core, rfpll_5g_div_2G_buf_pu, 1);
			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_OVR1, core,
					ovr_rfpll_5g_div_2G_buf_pu, 1);

			MOD_RADIO_REG_28NM(pi, RFP, PLL5G_RFVCO5, core,
					rfpll_5g_rfvco_VthM_oa_Rn, 0x6);
		}
	}
}

static radio_pll_sel_t
wlc_phy_radio20695_pll_tune(phy_info_t *pi, uint32 chan_freq, uint8 core)
{
	uint32 lo_freq = chan_freq;
	radio_pll_sel_t pll_sel;
	pll_config_20695_tbl_t *pll;

	/* Based on fc and USE_5G_PLL_FOR_2G, decide the required PLL to be on */
	if (lo_freq > 4000) {
		/* Aband
		Normal mode: 5G PLL is used for 5G. There is no other option
		*/
		pll_sel = PLL_5G;
	} else {
		/* Gband */
		if (pi->u.pi_acphy->radioi->data.use_5g_pll_for_2g == 0) {
			/* Normal mode:  2G PLL is used for 2G
			NO Logen.
			*/
			pll_sel = PLL_2G;
		} else {
			/* Special mode: 5G PLL is used for 2G
			5G PLL output gets divided by 2 and then goes to 2G LOgen.
			*/
			lo_freq *= 2;
			pll_sel = PLL_5G;
		}
	}

	/* Get pointer to structure */
	pll = wlc_phy_ac_get_20695_pll_config(pi);

	if (pll->use_doubler) {
		MOD_RADIO_REG_28NM(pi, RFP, XTAL7, core, xtal_doubler_pu, 1);
	} else {
		MOD_RADIO_REG_28NM(pi, RFP, XTAL7, core, xtal_doubler_pu, 0);
	}

	/* Calculate PLL config values */
	phy_ac_radio20695_pll_config_ch_dep_calc(pi, lo_freq, pll);

	/* Write computed values to PLL registers */
	phy_ac_radio20695_write_pll_config(pi, pll_sel, pll);

	return pll_sel;
}

static void
wlc_phy_radio20694_pll_tune(phy_info_t *pi, uint32 chan_freq, uint8 core)
{
	uint32 lo_freq = chan_freq;
	uint8 vco_select;
	uint32 icp_fx;
	uint32 loop_band;
	uint8 ac_mode;

	pll_config_20694_tbl_t *pll;

	/* Get pointer to structure */
	pll = wlc_phy_ac_get_20694_pll_config(pi);

	if (core == 0) {
		if (pll->use_doubler) {
			MOD_RADIO_REG_20694(pi, RFP, XTAL7, core, xtal_doubler_pu, 1);
		} else {
			MOD_RADIO_REG_20694(pi, RFP, XTAL7, core, xtal_doubler_pu, 0);
		}
	}

	if (core == 0) {
		/* **************** */
		/* Here we can specify band/channel dependent specific pll_config parameters */
		/* Calculate PLL config values */
		/* **************** */
		vco_select = 0;
		/* if 0, then select_VCO=0, enable_coupled_VCO=0;  */
		/* elseif 1, then select_VCO=1, enable_coupled_VCO=0; */
		/* elseif 2, then select_VCO=0, enable_coupled_VCO=1; */
		icp_fx = 131072;	/*  default = 2 mA; 0.16.16 format -> (2.0*pow(2,16)) */
		loop_band = 340;	/* MHz, default */
		ac_mode = 1;
		if (lo_freq < 4000) {
			ac_mode = 0;
		}
		phy_ac_radio20694_pll_config_ch_dep_calc(pi, lo_freq, vco_select, icp_fx,
			loop_band, ac_mode, pll);

		/* Write computed values to PLL registers */
		phy_ac_radio20694_write_pll_config(pi, pll);
	}

}

static void
wlc_phy_radio20695_rffe_tune(phy_info_t *pi, const chan_info_radio20695_rffe_t *chan_info_rffe,
	uint8 core)
{
	if (chan_info_rffe->freq > 4000) {
		/* 5G front end */
		MOD_RADIO_REG_28NM(pi, RF, LOGEN5G_REG5, core, logen5g_tune,
				chan_info_rffe->lo_gen);
		MOD_RADIO_REG_28NM(pi, RF, RX5G_REG1, core, rx5g_lna_tune,
				chan_info_rffe->rx_lna);
		MOD_RADIO_REG_28NM(pi, RF, MX5G_CFG5, core, mx5g_tune,
				chan_info_rffe->tx_mix);
		MOD_RADIO_REG_28NM(pi, RF, PA5G_CFG4, core, pad5g_tune,
				chan_info_rffe->tx_pad);
	} else {
		/* 2G front end */
		MOD_RADIO_REG_28NM(pi, RFP, PLL_RFVCO4, core, rfpll_vcobuf2g_tune,
				chan_info_rffe->vco_buf);
		MOD_RADIO_REG_28NM(pi, RF, LO5GTO2G_CFG3, core, lo5g_to_2g_tune,
				chan_info_rffe->lo_gen);
		MOD_RADIO_REG_28NM(pi, RF, LNA2G_REG1, core, lna2g_lna1_freq_tune,
				chan_info_rffe->rx_lna);
		MOD_RADIO_REG_28NM(pi, RF, TXMIX2G_CFG5, core, mx2g_tune,
				chan_info_rffe->tx_mix);
		MOD_RADIO_REG_28NM(pi, RF, PAD2G_CFG2, core, pad2g_tune,
				chan_info_rffe->tx_pad);
	}
}

static void
phy_ac_radio20695_pll_config_ch_dep_calc(phy_info_t *pi, uint32 lo_freq,
		pll_config_20695_tbl_t *pll)
{
	uint32 pfd_ref_freq_fx;
	uint8 nf;
	uint64 temp_64;
	uint32 temp_32;
	uint32 vco_freq;
	uint32 divide_ratio_fx;
	uint32 ndiv_fx;
	uint32 loop_band;
	uint32 rfpll_vcocal_XtalCount_raw_fx;
	uint8 band_idx;
	uint32 rfpll_vcocal_TargetCountCenter = 0;
	uint32 rfpll_frct_wild_base_dec_fx;
	uint32 wn_fx;
	uint32 loop_band_square_fx;
	uint32 c1_passive_lf_fx;
	uint32 c1_cap_multiplier_mode_fx;
	uint32 i_off_fx;
	uint64 r1_passive_lf_fx;
	uint64 rf_fx;
	uint64 rs_fx;
	uint32 rfpll_lf_lf_r2_ref_fx;
	uint32 rfpll_lf_lf_r2_dec_fx = 0;
	uint64 rfpll_lf_lf_rs_cm_ref_fx;
	uint32 rfpll_lf_lf_rs_cm_dec_fx;
	uint64 rfpll_lf_lf_rf_cm_ref_fx;
	uint32 rfpll_lf_lf_rf_cm_dec_fx;
	uint32 rfpll_lf_lf_c1_ref_fx;
	uint32 rfpll_lf_lf_c1_dec_fx;
	uint32 rfpll_lf_lf_c2_dec_fx;
	uint32 rfpll_lf_lf_c2_ref_fx;
	uint32 rfpll_lf_lf_c3_ref_fx;
	uint32 rfpll_lf_lf_c3_dec_fx;
	uint32 rfpll_cp_ioff_ref_fx;
	uint32 rfpll_cp_ioff_dec_fx;
	int64 temp_sign;
	uint16 rfpll_vcocal_XtalCount_dec;
	uint16 rfpll_vcocal_DeltaPllVal_dec;
	uint16 rfpll_vcocal_ErrorThres = 0;
	uint16 rfpll_vcocal_InitCapA = 0;
	uint16 rfpll_vcocal_InitCapB = 0;
	uint16 rfpll_vcocal_NormCountLeft;
	uint16 rfpll_vcocal_NormCountRight;
	uint16 rfpll_vcocal_TargetCountCenter_dec;
	uint16 rfpll_vcocal_MidCodeSel_dec;
	uint16 rfpll_lf_lf_r2_ideal = 0;
	uint16 rfpll_lf_lf_rs_cm_ideal = 0;
	uint16 rfpll_lf_lf_rf_cm_ideal = 0;
	uint16 rfpll_lf_lf_c1_ideal = 0;
	uint16 rfpll_lf_lf_c2_ideal = 0;
	uint16 rfpll_lf_lf_c3_ideal = 0;

	/* Band dependent constants array */
	const uint32 VCO_cal_cap_bits[] = {VCO_CAL_CAP_BITS_5G,
		VCO_CAL_CAP_BITS_2G};
	const uint32 kvco_code_fx[] = {KVCO_CODE_FX_5G,
		KVCO_CODE_FX_2G};
	const uint32 temp_code_fx[] = {TEMP_CODE_FX_5G,
		TEMP_CODE_FX_2G};
	const uint32 icp_fx[] = {ICP_FX_5G,
		ICP_FX_2G};
	const uint64 c1_passive_lf_const_fx[] = {C1_PASSIVE_LF_CONST_FX_5G,
		C1_PASSIVE_LF_CONST_FX_2G};
	const uint32 r1_passive_lf_const_fx[] = {R1_PASSIVE_LF_CONST_FX_5G,
		R1_PASSIVE_LF_CONST_FX_2G};
	const uint32 rfpll_lf_lf_r2_ref_const_fx[] = {RFPLL_LF_LF_R2_REF_CONST_FX_5G,
		RFPLL_LF_LF_R2_REF_CONST_FX_2G};
	const uint32 rfpll_cp_kpd_scale_dec_fx[] = {RFPLL_CP_KPD_SCALE_DEC_FX_5G,
		RFPLL_CP_KPD_SCALE_DEC_FX_2G};
	const uint32 rfpll_vcocal_XtalCount_fx[] = {1, 0};
	const uint32 ndiv_mult[] = {2, 1};
	const uint32 loop_band_width[] = {LOOP_BW_5G, LOOP_BW_2G};
	uint32 div_ratio_den[2] = {0};

	/* ------------------------------------------------------------------- */
	/* PFD Reference Frequency */
	/* <0.8.24> * <0.32.0> --> <0.8.24> */
	pfd_ref_freq_fx = pll->xtal_fx << pll->use_doubler;
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* VCO Frequency */
	/* UP_CONVERSION_RATIO = 2 */
	/* <0.32.0> * <0.32.0> --> <0.32.0> */
	vco_freq = lo_freq << 1;
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Band dependent constant selection */
	/* <0.8.24> * <0.32.0> --> <0.8.24> */
	div_ratio_den[0] = pfd_ref_freq_fx << 1;
	div_ratio_den[1] = pfd_ref_freq_fx;
	band_idx = (lo_freq > 4000) ? 0 : 1;

	if (pll->loop_bw == 0) {
		/* User did not specify a loop bandwidth, use default */
		loop_band = loop_band_width[band_idx];
	} else {
		loop_band = pll->loop_bw;
	}
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* VCO Frequency dependent calculations */
	/* <0.32.0> / <0.8.24> --> <0.(32 - nf).nf> */
	nf = fp_div_64(vco_freq, div_ratio_den[band_idx], NF0, NF24, &divide_ratio_fx);
	/* floor(<0.(32-nf).nf>, (nf-20)) -> 0.12.20 */
	divide_ratio_fx = fp_floor_32(divide_ratio_fx, (nf - NF20));

	/* <0.12.20> * <0.32.0> --> <0.16.16> */
	ndiv_fx = (uint32)fp_mult_64(divide_ratio_fx, ndiv_mult[band_idx], NF20, NF0, NF16);
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Defines the duration of time that vcocal should wait after it applied a
	   new calibration bit to to vco before it starts to measure vco frequency
	   to calculate the next bit. This time is required by vco to settle.
	*/
	/* <0.8.24> * <0.32.0> --> <0.8.16> */
	rfpll_vcocal_XtalCount_raw_fx = fp_round_32(pll->xtal_fx <<
			rfpll_vcocal_XtalCount_fx[band_idx], (NF24 - NF16));
	/* ceil(<0.8.16>, 16) -> <0.8.0> */
	rfpll_vcocal_XtalCount_dec = (uint16)fp_ceil_64(rfpll_vcocal_XtalCount_raw_fx,
			NF16);
	PLL_CONFIG_20695_VAL_ENTRY(pll, XTALCNT_2G, rfpll_vcocal_XtalCount_dec);
	PLL_CONFIG_20695_VAL_ENTRY(pll, XTALCNT_5G, rfpll_vcocal_XtalCount_dec);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_XTALCOUNT,
			rfpll_vcocal_XtalCount_dec);

	/* <0.32.0> / <0.16.16> --> <0.(32 - nf).nf> */
	nf = fp_div_64(rfpll_vcocal_XtalCount_dec, rfpll_vcocal_XtalCount_raw_fx,
			NF0, NF16, &temp_32);
	/* round(<0.(32-nf).nf>, (nf-16)) -> 0.16.16 */
	temp_32 = fp_round_32(temp_32, (nf - NF16));
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_32	= temp_32 - FX_ONE;
	/* round(<0.16.16> * <0.32.0> ,16) -> <0.32.0> */
	rfpll_vcocal_DeltaPllVal_dec = (uint16)fp_round_64((temp_32 * (1 << 13)), 16);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_DELTAPLLVAL,
			rfpll_vcocal_DeltaPllVal_dec);

	/* Frequency range dependent constants */
	if (lo_freq >= 2000 && lo_freq <= 3000) {
		if (pll->logen_mode == 0) {
			rfpll_vcocal_InitCapA = 1100;
			rfpll_vcocal_InitCapB = 1356;
		} else {
			rfpll_vcocal_InitCapA = 2500;
			rfpll_vcocal_InitCapB = 3012;
		}
		rfpll_vcocal_ErrorThres  = 5;
		rfpll_vcocal_TargetCountCenter  = 2442;
	} else if  (lo_freq < 5250) {
		rfpll_vcocal_InitCapA  = 2048;
		rfpll_vcocal_InitCapB  = 2304;
		rfpll_vcocal_ErrorThres = 8;
		rfpll_vcocal_TargetCountCenter = 5070;
	} else if (lo_freq >= 5250 && lo_freq < 5500) {
		rfpll_vcocal_InitCapA = 1408;
		rfpll_vcocal_InitCapB = 1664;
		rfpll_vcocal_ErrorThres = 9;
		rfpll_vcocal_TargetCountCenter = 5370;
	} else if (lo_freq >= 5500) {
		rfpll_vcocal_InitCapA = 768;
		rfpll_vcocal_InitCapB = 1024;
		rfpll_vcocal_ErrorThres = 14;
		rfpll_vcocal_TargetCountCenter = 5700;
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_ERRORTHRES, rfpll_vcocal_ErrorThres);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_INITCAPA, rfpll_vcocal_InitCapA);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_INITCAPB, rfpll_vcocal_InitCapB);

	if (lo_freq >= 2000 && lo_freq <= 3000) {
		rfpll_vcocal_NormCountLeft = 985;
		rfpll_vcocal_NormCountRight = 39;
	} else {
		rfpll_vcocal_NormCountLeft = 974;
		rfpll_vcocal_NormCountRight = 50;
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_NORMCOUNTLEFT,
			rfpll_vcocal_NormCountLeft);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_NORMCOUNTRIGHT,
			rfpll_vcocal_NormCountRight);

	/* <0.32.0> - <0.32.0> -> <0.32.0> */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_ROUNDLSB, (12 - VCO_cal_cap_bits[band_idx]));

	/* <0.32.0> * <0.32.0> -> <0.32.0> */
	temp_64 = rfpll_vcocal_TargetCountCenter * rfpll_vcocal_DeltaPllVal_dec;
	/* <0.32.0> + round(<0.32.0> * <0.16.16>, 16) -> <0.32.0> */
	rfpll_vcocal_TargetCountCenter_dec = (uint16)(rfpll_vcocal_TargetCountCenter +
			fp_round_64((temp_64 * FIX_2POW_MIN13), NF16));
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_TARGETCOUNTCENTER,
			rfpll_vcocal_TargetCountCenter_dec);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_PLL_VAL, (uint16)lo_freq);
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Other VCOCAL register (not channel/XTAL dependent) */
	if (RFPLL_VCOCAL_ENABLECOUPLING_DEC == 0) {
		if ((VCO_CAL_AUX_CAP_BITS < 6) || (VCO_CAL_AUX_CAP_BITS > 9)) {
			PHY_ERROR(("wl%d: %s: Unsupported value VCO_cal_aux_cap_bits\n",
				pi->sh->unit, __FUNCTION__));
			ASSERT(0);
			return;
		}

		rfpll_vcocal_MidCodeSel_dec = VCO_CAL_AUX_CAP_BITS - 6;
	} else {
		if ((VCO_CAL_AUX_CAP_BITS < 4) || (VCO_CAL_AUX_CAP_BITS > 7)) {
			PHY_ERROR(("wl%d: %s: Unsupported value VCO_cal_aux_cap_bits\n",
				pi->sh->unit, __FUNCTION__));
			ASSERT(0);
			return;
		}

		rfpll_vcocal_MidCodeSel_dec = VCO_CAL_AUX_CAP_BITS - 4;
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_MIDCODESEL, rfpll_vcocal_MidCodeSel_dec);
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* VCO register */
	/* Fract-N (Sigma-Delta) register */
	rfpll_frct_wild_base_dec_fx = divide_ratio_fx;
	temp_32 = rfpll_frct_wild_base_dec_fx >> 16;
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_FRCT_WILD_BASE_HIGH,
			(uint16)(temp_32 & 0xFFFF));
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_FRCT_WILD_BASE_LOW,
			(uint16)(rfpll_frct_wild_base_dec_fx & 0xFFFF));

	/* <0.24.8> * <0.32.0> -> <0.26.6> */
	wn_fx = (uint32)fp_mult_64(WN_CONST_FX, loop_band, NF8, NF0, NF6);
	/* <0.32.0> * <0.32.0> -> <0.32.0> */
	loop_band_square_fx = loop_band * loop_band;
	/* <0.16.16> * <0.32.0> -> <0.26.6> */
	temp_32 = (uint32)fp_mult_64(ndiv_fx, loop_band_square_fx, NF16, NF0, NF6);
	/* <0.16.16> / <0.26.6> -> <0.(32-nf).nf> */
	nf = fp_div_64(c1_passive_lf_const_fx[band_idx], temp_32, NF16, NF6, &c1_passive_lf_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	c1_passive_lf_fx = fp_round_32(c1_passive_lf_fx, (nf - NF16));

	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(c1_passive_lf_fx, CAP_MULTIPLIER_RATIO_PLUS_ONE, NF16, NF0,
			&c1_cap_multiplier_mode_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	c1_cap_multiplier_mode_fx = fp_round_32(c1_cap_multiplier_mode_fx, (nf - NF16));

	/* <0.16.16> * <0.8.24> -> <0.16.16> */
	temp_32 = (uint32)fp_mult_64(T_OFF_FX, pfd_ref_freq_fx, NF16, NF24, NF16);
	/* <0.16.16> * <0.16.16> -> <0.16.16> */
	i_off_fx = (uint32)fp_mult_64(temp_32, icp_fx[band_idx], NF16, NF16, NF16);

	/* <0.6.26> * <0.0.32> -> <0.16.16> */
	temp_64 = fp_mult_64(wn_fx, r1_passive_lf_const_fx[band_idx], NF6, NF32, NF16);
	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(ndiv_fx, 1000, NF16, NF0, &temp_32);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	temp_32 = fp_round_32(temp_32, (nf - NF16));
	/* <0.16.16> * <0.16.16> -> <0.16.16> */
	r1_passive_lf_fx = fp_mult_64(temp_64, temp_32, NF16, NF16, NF16);
	/* <0.16.16> * <0.16.16> -> <0.16.16> */
	rf_fx = fp_mult_64(r1_passive_lf_fx, RF_CONST_FX, NF16, NF16, NF16);
	/* <0.32.0> * <0.16.16> -> <0.16.16> */
	rs_fx = CAP_MULTIPLIER_RATIO * rf_fx;
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Register Definitions (lower) */
	/* rfpll_lf_lf_r2 */
	/* <0.6.26> * <0.16.16> -> <0.24.8> */
	temp_64 = fp_mult_64(wn_fx, ndiv_fx, NF6, NF16, NF8);
	/* <0.24.8> * <0.0.32> -> <0.16.16> */
	temp_64 = fp_mult_64(temp_64, rfpll_lf_lf_r2_ref_const_fx[band_idx], NF8, NF32, NF16);
	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(temp_64, 1000, NF16, NF0, &rfpll_lf_lf_r2_ref_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	rfpll_lf_lf_r2_ref_fx = fp_round_32(rfpll_lf_lf_r2_ref_fx, (nf - NF16));
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_r2_ref_fx - (int64)CONST_800_FX);

	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_400_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_r2_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_r2_ideal = LIMIT((int32)rfpll_lf_lf_r2_dec_fx, 0, 127);
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_R2, rfpll_lf_lf_r2_ideal);

	/* rfpll_lf_lf_r3 */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_R3, rfpll_lf_lf_r2_ideal);

	/* rfpll_lf_lf_rs_cm */
	rfpll_lf_lf_rs_cm_ref_fx = rs_fx;
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_rs_cm_ref_fx - (int64)CONST_3200_FX);

	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_1600_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_rs_cm_dec_fx =  fp_round_32(temp_32, nf);
		rfpll_lf_lf_rs_cm_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_rs_cm_dec_fx,
				0, 255));
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_RS_CM, rfpll_lf_lf_rs_cm_ideal);

	/* rfpll_lf_lf_rf_cm */
	rfpll_lf_lf_rf_cm_ref_fx = rf_fx;
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_rf_cm_ref_fx - (int64)CONST_800_FX);

	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_400_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_rf_cm_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_rf_cm_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_rf_cm_dec_fx,
				0, 127));
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_RF_CM, rfpll_lf_lf_rf_cm_ideal);

	/* rfpll_lf_lf_c1 */
	rfpll_lf_lf_c1_ref_fx  = c1_cap_multiplier_mode_fx;
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_c1_ref_fx - (int64)CONST_5P4_FX);

	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_P9_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_c1_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_c1_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_c1_dec_fx, 0, 31));
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_C1, rfpll_lf_lf_c1_ideal);

	/* rfpll_lf_lf_c2 */
	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(c1_passive_lf_fx, 15, NF16, NF0, &rfpll_lf_lf_c2_ref_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	rfpll_lf_lf_c2_ref_fx = fp_round_32(rfpll_lf_lf_c2_ref_fx, (nf - NF16));
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_c2_ref_fx - (int64)CONST_3P4_FX);

	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_P6_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_c2_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_c2_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_c2_dec_fx, 0, 31));
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_C2, rfpll_lf_lf_c2_ideal);

	/* rfpll_lf_lf_c3 */
	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(c1_passive_lf_fx, C_CONSTANT, NF16, NF0, &rfpll_lf_lf_c3_ref_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	rfpll_lf_lf_c3_ref_fx = fp_round_32(rfpll_lf_lf_c3_ref_fx, (nf - NF16));
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_c3_ref_fx - (int64)CONST_3P4_FX);

	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_P6_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_c3_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_c3_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_c3_dec_fx, 0, 31));
	}

	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_C3, rfpll_lf_lf_c3_ideal);

	/* rfpll_lf_lf_c4 */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_LF_LF_C4, rfpll_lf_lf_c3_ideal);

	/* cp_kpd_scale */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_CP_KPD_SCALE,
			(uint16)(LIMIT((int32)rfpll_cp_kpd_scale_dec_fx[band_idx], 0, 255)));

	/* rfpll_cp_ioff */
	rfpll_cp_ioff_ref_fx = i_off_fx;
	/* round(<0.16.16>, 16) -> <0.32.0> */
	rfpll_cp_ioff_dec_fx = fp_round_32(rfpll_cp_ioff_ref_fx, NF16);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_CP_IOFF,
			(uint16)(LIMIT((int32)rfpll_cp_ioff_dec_fx, 0, 255)));
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Put other register values to structure */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_RFVCO_KVCO_CODE, (uint16)kvco_code_fx[band_idx]);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_RFVCO_TEMP_CODE, (uint16)temp_code_fx[band_idx]);
	/* ------------------------------------------------------------------- */
}

static void
phy_ac_radio20694_pll_config_ch_dep_calc(phy_info_t *pi, uint32 lo_freq, uint8 vco_select,
	uint32 icp_fx, uint32 loop_band, uint8 ac_mode, pll_config_20694_tbl_t *pll)
{
	uint32 pfd_ref_freq_fx;
	uint8 nf;
	uint64 temp_64;
	uint32 temp_32;
	uint32 temp_32_1;
	uint32 vco_freq;
	uint32 divide_ratio_fx;
	uint8 enable_coupled_VCO;
	uint8 select_VCO;
	uint8 rfpll_vcocal_enablecoupling_dec;
	uint32 lo_div_vco_ratio_fx;
	uint32 rfpll_vcocal_XtalCount_raw_fx;
	uint32 rfpll_vcocal_TargetCountCenter = 0;
	uint32 rfpll_frct_wild_base_dec_fx;
	uint32 wn_fx;
	uint32 loop_band_square_fx;
	uint32 c1_passive_lf_fx;
	uint32 c1_cap_multiplier_mode_fx;
	uint32 i_off_fx;
	uint64 r1_passive_lf_fx;
	uint64 rf_fx;
	uint64 rs_fx;
	uint32 rfpll_lf_lf_r2_dec_fx;
	uint64 rfpll_lf_lf_rs_cm_ref_fx;
	uint32 rfpll_lf_lf_rs_cm_dec_fx;
	uint64 rfpll_lf_lf_rf_cm_ref_fx;
	uint32 rfpll_lf_lf_rf_cm_dec_fx;
	uint32 rfpll_lf_lf_c1_ref_fx;
	uint32 rfpll_lf_lf_c1_dec_fx;
	uint32 rfpll_lf_lf_c2_dec_fx;
	uint32 rfpll_lf_lf_c2_ref_fx;
	uint32 rfpll_lf_lf_c3_ref_fx;
	uint32 rfpll_lf_lf_c3_dec_fx;
	uint32 rfpll_lf_lf_c4_ref_fx;
	uint32 rfpll_lf_lf_c4_dec_fx;
	uint32 rfpll_cp_kpd_scale_ref_fx;
	uint32 rfpll_cp_kpd_scale_dec_fx;
	uint32 rfpll_cp_ioff_dec_fx;
	int64 temp_sign;
	uint16 rfpll_vcocal_force_caps2_ovr_dec;
	uint16 rfpll_vcocal_force_caps_ovr_dec;
	uint16 rfpll_vcocal_CouplThres2_dec = 0;
	uint16 rfpll_vco_core1_en_dec = 0;
	uint16 rfpll_vco_core2_en_dec = 0;
	uint16	rfpll_vco_cvar_dec = 0;
	uint16 rfpll_vco_ALC_ref_ctrl_dec = 0;
	uint16 rfpll_vco_bias_mode_dec = 0;
	uint16 rfpll_vco_tempco_dcadj_dec = 0;
	uint16 rfpll_vco_tempco_dec = 0;
	uint16 rfpll_vcocal_XtalCount_dec;
	uint16 rfpll_vcocal_DeltaPllVal_dec;
	uint16 rfpll_vcocal_ErrorThres = 0;
	uint16 rfpll_vcocal_InitCapA = 0;
	uint16 rfpll_vcocal_InitCapB = 0;
	uint16 rfpll_vcocal_NormCountLeft;
	uint16 rfpll_vcocal_NormCountRight;
	uint16 rfpll_vcocal_TargetCountCenter_dec;
	uint16 rfpll_vcocal_MidCodeSel_dec;
	uint16 rfpll_lf_lf_r2_ideal = 0;
	uint16 rfpll_lf_lf_rs_cm_ideal = 0;
	uint16 rfpll_lf_lf_rf_cm_ideal = 0;
	uint16 rfpll_lf_lf_c1_ideal = 0;
	uint16 rfpll_lf_lf_c2_ideal = 0;
	uint16 rfpll_lf_lf_c3_ideal = 0;
	uint16 rfpll_lf_lf_c4_ideal = 0;
	uint16 rfpll_cp_kpd_scale_ideal = 0;
	uint16 rfpll_cp_ioff_ideal = 0;

	/* ------------------------------------------------------------------- */
	/* PFD Reference Frequency */
	/* <0.8.24> * <0.32.0> --> <0.8.24> */
	pfd_ref_freq_fx = pll->xtal_fx << pll->use_doubler;
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* VCO Frequency */
	/* In 2G regular mode. VCO = LO x 3/2, In 2G RSDB mode(some channels) */
	/* VCO = LO x 4/3 to avoid VCO pulling #C14 */
	/* In 5G, VCO  = LO x 2/3  #C15 */
	/* lo_freq #C9,	## logen_mode = C14  */
	if (lo_freq <= 3000 && pll->logen_mode == 0) {
		/* <0.32.0> * <0.32.0> --> <0.32.0> */
		vco_freq = (uint32)fp_mult_64(lo_freq, 3, NF0, NF0, NF0);
		/* <0.32.0> / <0.32.0> --> <0.(32 - nf).nf> */
		nf = fp_div_64(vco_freq, 2, NF0, NF0, &vco_freq);
		/* floor(<0.(32-nf).nf>, (nf-16)) -> 0.16.16 */
		vco_freq = fp_floor_32(vco_freq, (nf - NF16));
	} else if (lo_freq <= 3000 && pll->logen_mode == 1) {
		/* <0.32.0> * <0.32.0> --> <0.32.0> */
		vco_freq = (uint32)fp_mult_64(lo_freq, 4, NF0, NF0, NF0);
		/* <0.32.0> / <0.32.0> --> <0.(32 - nf).nf> */
		nf = fp_div_64(vco_freq, 3, NF0, NF0, &vco_freq);
		/* floor(<0.(32-nf).nf>, (nf-16)) -> 0.16.16 */
		vco_freq = fp_floor_32(vco_freq, (nf - NF16));
	} else {
		/* <0.32.0> / <0.32.0> --> <0.(32 - nf).nf> */
		nf = fp_div_64((lo_freq << 1), 3, NF0, NF0, &vco_freq);
		/* floor(<0.(32-nf).nf>, (nf-16)) -> 0.16.16 */
		vco_freq = fp_floor_32(vco_freq, (nf - NF16));
	}
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	pll->loop_bw = loop_band;
	if (vco_select == 0) {
		enable_coupled_VCO = 0;
		select_VCO = 0;
		rfpll_vcocal_force_caps_ovr_dec = 0;
		rfpll_vcocal_force_caps2_ovr_dec = 1; /* VCO1 */
	} else if (vco_select == 1) {
		enable_coupled_VCO = 0;
		select_VCO = 1;
		rfpll_vcocal_force_caps_ovr_dec = 1;
		rfpll_vcocal_force_caps2_ovr_dec = 0; /* VCO2 */
	} else if (vco_select == 2) {
		enable_coupled_VCO = 1;
		select_VCO = 0;
		rfpll_vcocal_force_caps_ovr_dec = 0;
		rfpll_vcocal_force_caps2_ovr_dec = 0; /* Coupled VCO */
	} else {
		PHY_ERROR(("wl%d: %s: vco_select arg must be 0(VCO1),1(VCO2) or 2(VCO1+VCO2)\n",
			pi->sh->unit, __FUNCTION__));
		ASSERT(0);
		return;
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS_OVR,
		rfpll_vcocal_force_caps_ovr_dec);
	PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS2_OVR,
		rfpll_vcocal_force_caps2_ovr_dec);
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* VCO Frequency dependent calculations */
	/* <0.16.16> / <0.8.24> --> <0.(32 - nf).nf> */
	nf = fp_div_64(vco_freq, pfd_ref_freq_fx, NF16, NF24, &divide_ratio_fx);
	/* floor(<0.(32-nf).nf>, (nf-20)) -> 0.12.20 */
	divide_ratio_fx = fp_floor_32(divide_ratio_fx, (nf - NF20));

	/* <0.16.16> / <0.16.16> --> <0.(32 - nf).nf> */
	nf = fp_div_64(lo_freq, vco_freq, NF0, NF16, &lo_div_vco_ratio_fx);
	/* floor(<0.(32-nf).nf>, (nf-16)) -> 0.16.16 */
	lo_div_vco_ratio_fx = fp_floor_32(lo_div_vco_ratio_fx, (nf - NF16));

	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Defines the duration of time that vcocal should wait after it applied a
	   new calibration bit to to vco before it starts to measure vco frequency
	   to calculate the next bit. This time is required by vco to settle.
	*/
	/* <0.16.16> * <0.8.24> --> <0.8.16> */
	rfpll_vcocal_XtalCount_raw_fx = (uint32)fp_mult_64(lo_div_vco_ratio_fx << 1,
		pll->xtal_fx, NF16, NF24, NF16);
	/* ceil(<0.8.16>, 16) -> <0.8.0> */
	rfpll_vcocal_XtalCount_dec = (uint16)fp_ceil_64(rfpll_vcocal_XtalCount_raw_fx, NF16);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_XTALCOUNT,
		rfpll_vcocal_XtalCount_dec);

	/* <0.32.0> / <0.16.16> --> <0.(32 - nf).nf> */
	nf = fp_div_64(rfpll_vcocal_XtalCount_dec, rfpll_vcocal_XtalCount_raw_fx,
			NF0, NF16, &temp_32);
	/* round(<0.(32-nf).nf>, (nf-16)) -> 0.16.16 */
	temp_32 = fp_round_32(temp_32, (nf - NF16));
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_32	= temp_32 - FX_ONE;
	/* round(<0.16.16> * <0.32.0> ,16) -> <0.32.0> */
	rfpll_vcocal_DeltaPllVal_dec = (uint16)fp_round_64((temp_32 * (1 << 13)), 16);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_DELTAPLLVAL,
		rfpll_vcocal_DeltaPllVal_dec);

	/* Frequency range dependent constants */
	if (lo_freq >= 2000 && lo_freq <= 3000) {
		if (pll->logen_mode == 0) {
			rfpll_vcocal_InitCapA = 1536;
			rfpll_vcocal_InitCapB = 1792;
			rfpll_vcocal_CouplThres2_dec = 25;
		} else {
			rfpll_vcocal_InitCapA = 2944;
			rfpll_vcocal_InitCapB = 3200;
			rfpll_vcocal_CouplThres2_dec = 40;
		}
		rfpll_vcocal_ErrorThres  = 3;
		rfpll_vcocal_TargetCountCenter  = 2442;
	} else if  (lo_freq < 5250) {
		rfpll_vcocal_InitCapA  = 2432;
		rfpll_vcocal_InitCapB  = 2688;
		rfpll_vcocal_ErrorThres = 5;
		rfpll_vcocal_CouplThres2_dec = 40;
		rfpll_vcocal_TargetCountCenter = 5070;
	} else if (lo_freq >= 5250 && lo_freq < 5500) {
		rfpll_vcocal_InitCapA = 1792;
		rfpll_vcocal_InitCapB = 2048;
		rfpll_vcocal_ErrorThres = 7;
		rfpll_vcocal_CouplThres2_dec = 40;
		rfpll_vcocal_TargetCountCenter = 5370;
	} else if (lo_freq >= 5500) {
		rfpll_vcocal_InitCapA = 1088;
		rfpll_vcocal_InitCapB = 1344;
		rfpll_vcocal_ErrorThres = 8;
		rfpll_vcocal_CouplThres2_dec = 40;
		rfpll_vcocal_TargetCountCenter = 5700;
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_ERRORTHRES, rfpll_vcocal_ErrorThres);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_INITCAPA, rfpll_vcocal_InitCapA);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_INITCAPB, rfpll_vcocal_InitCapB);
	PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLTHRES2,
			rfpll_vcocal_CouplThres2_dec);

	if (lo_freq >= 2000 && lo_freq <= 3000) {
		rfpll_vcocal_NormCountLeft = -39;
		rfpll_vcocal_NormCountRight = 39;
	} else {
		rfpll_vcocal_NormCountLeft = -50;
		rfpll_vcocal_NormCountRight = 50;
	}

	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_NORMCOUNTLEFT,
			rfpll_vcocal_NormCountLeft);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_NORMCOUNTRIGHT,
			rfpll_vcocal_NormCountRight);

	/* <0.32.0> - <0.32.0> -> <0.32.0> */
	// pll->rfpll_vcocal_RoundLSB = 12 - VCO_CAL_CAP_BITS;
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_ROUNDLSB, (12 - VCO_CAL_CAP_BITS));

	/* <0.32.0> * <0.32.0> -> <0.32.0> */
	temp_64 = rfpll_vcocal_TargetCountCenter * rfpll_vcocal_DeltaPllVal_dec;
	/* <0.32.0> + round(<0.32.0> * <0.16.16>, 16) -> <0.32.0> */
	rfpll_vcocal_TargetCountCenter_dec = (uint16)(rfpll_vcocal_TargetCountCenter +
			fp_round_64((temp_64 * FIX_2POW_MIN13), NF16));
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_TARGETCOUNTCENTER,
			rfpll_vcocal_TargetCountCenter_dec);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_PLL_VAL, (uint16)lo_freq);

	if (enable_coupled_VCO == 1) {
		rfpll_vcocal_enablecoupling_dec = 1;
	} else {
		rfpll_vcocal_enablecoupling_dec = 0;
	}
	PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_ENABLECOUPLING,
		rfpll_vcocal_enablecoupling_dec);

	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Other VCOCAL register (not channel/XTAL dependent) */
	if (RFPLL_VCOCAL_COUPLINGMODE_DEC == 0) {
		if ((VCO_CAL_AUX_CAP_BITS < 6) || (VCO_CAL_AUX_CAP_BITS > 9)) {
			PHY_ERROR(("wl%d: %s: Unsupported value VCO_cal_aux_cap_bits\n",
				pi->sh->unit, __FUNCTION__));
			ASSERT(0);
			return;
		}

		rfpll_vcocal_MidCodeSel_dec = VCO_CAL_AUX_CAP_BITS - 6;
	} else {
		if ((VCO_CAL_AUX_CAP_BITS < 4) || (VCO_CAL_AUX_CAP_BITS > 7)) {
			PHY_ERROR(("wl%d: %s: Unsupported value VCO_cal_aux_cap_bits\n",
				pi->sh->unit, __FUNCTION__));
			ASSERT(0);
			return;
		}

		rfpll_vcocal_MidCodeSel_dec = VCO_CAL_AUX_CAP_BITS - 4;
	}
	PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_MIDCODESEL, rfpll_vcocal_MidCodeSel_dec);
	/* ------------------------------------------------------------------- */
	/* VCO register */
	/* ------------------------------------------------------------------- */
	if (enable_coupled_VCO == 1) {
		rfpll_vco_core1_en_dec = 1;
		rfpll_vco_core2_en_dec = 1;
	} else if (select_VCO == 0) {
		rfpll_vco_core1_en_dec = 1;
		rfpll_vco_core2_en_dec = 0;
	} else if (select_VCO == 1) {
		rfpll_vco_core1_en_dec = 0;
		rfpll_vco_core2_en_dec = 1;
	}
	PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCO_CORE1_EN, rfpll_vco_core1_en_dec);
	PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCO_CORE2_EN, rfpll_vco_core2_en_dec);

	/* Fract-N (Sigma-Delta) register */
	rfpll_frct_wild_base_dec_fx = divide_ratio_fx;
	temp_32 = rfpll_frct_wild_base_dec_fx >> 16;
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_FRCT_WILD_BASE_HIGH,
			(uint16)(temp_32 & 0xFFFF));
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_FRCT_WILD_BASE_LOW,
			(uint16)(rfpll_frct_wild_base_dec_fx & 0xFFFF));

	/* ------------------------------------------------------------------- */
	/* VCO register (11n vs 11ac mode) */
	/* ------------------------------------------------------------------- */
	/* There is no Class-C VCO anymore. Always CMOS VCO */
	/* 2 VCO mode: 11abgn, 11ac */
	if (ac_mode == 0) {
		rfpll_vco_cvar_dec = 8;
		rfpll_vco_ALC_ref_ctrl_dec = 8;
		rfpll_vco_bias_mode_dec = 0;
		rfpll_vco_tempco_dcadj_dec = 1;
		rfpll_vco_tempco_dec = 3;
	} else if (ac_mode == 1) {
		rfpll_vco_cvar_dec = 8;
		rfpll_vco_ALC_ref_ctrl_dec = 13;
		rfpll_vco_bias_mode_dec = 1;
		rfpll_vco_tempco_dcadj_dec = 3;
		rfpll_vco_tempco_dec = 5;
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCO_CVAR, rfpll_vco_cvar_dec);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCO_ALC_REF_CTRL, rfpll_vco_ALC_ref_ctrl_dec);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCO_BIAS_MODE, rfpll_vco_bias_mode_dec);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCO_TEMPCO_DCADJ, rfpll_vco_tempco_dcadj_dec);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCO_TEMPCO, rfpll_vco_tempco_dec);

	/* ----------------------------- */
	/* CP and Loop Filter Registers  */
	/* ----------------------------- */
	/* <0.24.8> * <0.32.0> -> <0.26.6> */
	wn_fx = (uint32)fp_mult_64(WN_CONST_FX, loop_band, NF8, NF0, NF6);
	/* <0.32.0> * <0.32.0> -> <0.32.0> */
	loop_band_square_fx = loop_band * loop_band;

	/* <0.12.20> * <0.32.0> -> <0.26.6> */
	temp_32 = (uint32)fp_mult_64(divide_ratio_fx, loop_band_square_fx, NF20, NF0, NF6);
	/* <0.32.32> / <0.26.6> -> <0.(32-nf).nf> */
	nf = fp_div_64(C1_PASSIVE_LF_CONST_FX, temp_32, NF16, NF6, &temp_32_1);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	temp_32_1 = fp_round_32(temp_32_1, (nf - NF16));
	c1_passive_lf_fx = (uint32)fp_mult_64(temp_32_1, icp_fx, NF16, NF16, NF16);

	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(c1_passive_lf_fx, CAP_MULTIPLIER_RATIO_PLUS_ONE_20694, NF16, NF0,
			&c1_cap_multiplier_mode_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	c1_cap_multiplier_mode_fx = fp_round_32(c1_cap_multiplier_mode_fx, (nf - NF16));

	/* <0.16.16> * <0.8.24> -> <0.16.16> */
	temp_32 = (uint32)fp_mult_64(T_OFF_FX, pfd_ref_freq_fx, NF16, NF24, NF16);
	/* <0.16.16> * <0.16.16> -> <0.16.16> */
	i_off_fx = (uint32)fp_mult_64(temp_32, icp_fx, NF16, NF16, NF16);

	/* <0.26.6> * <0.0.32> -> <0.16.16> */
	temp_64 = fp_mult_64(wn_fx, R1_PASSIVE_LF_CONST_FX, NF6, NF32, NF16);
	/* <0.12.20> / <0.16.16> -> <0.(32-nf).nf> */
	nf = fp_div_64(divide_ratio_fx, icp_fx, NF20, NF16, &temp_32);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	temp_32 = fp_round_32(temp_32, (nf - NF16));
	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(temp_32, 1000, NF16, NF0, &temp_32_1);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	temp_32_1 = fp_round_32(temp_32_1, (nf - NF16));
	/* <0.16.16> * <0.16.16> -> <0.16.16> */
	r1_passive_lf_fx = fp_mult_64(temp_64, temp_32_1, NF16, NF16, NF16);
	/* <0.16.16> * <0.16.16> -> <0.16.16> */
	rf_fx = fp_mult_64(r1_passive_lf_fx, RF_CONST1_FX, NF16, NF16, NF16);
	/* <0.32.0> * <0.16.16> -> <0.16.16> */
	rs_fx = CAP_MULTIPLIER_RATIO_20694 * rf_fx;
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Register Definitions (lower) */
	/* rfpll_lf_lf_r2 */
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)(r1_passive_lf_fx - (int64)CONST_303_FX);
	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_280_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_r2_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_r2_ideal = LIMIT((int32)rfpll_lf_lf_r2_dec_fx, 0, 255);
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_R2, rfpll_lf_lf_r2_ideal);

	/* rfpll_lf_lf_r3 */
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_R3, rfpll_lf_lf_r2_ideal);

	/* rfpll_lf_lf_rs_cm */
	rfpll_lf_lf_rs_cm_ref_fx = rs_fx;   /* cap_multiplier_PU = 1 */
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_rs_cm_ref_fx - (int64)CONST_303_FX);
	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_280_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_rs_cm_dec_fx =  fp_round_32(temp_32, nf);
		rfpll_lf_lf_rs_cm_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_rs_cm_dec_fx,
			0, 255));
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_RS_CM, rfpll_lf_lf_rs_cm_ideal);

	/* rfpll_lf_lf_rf_cm */
	rfpll_lf_lf_rf_cm_ref_fx = rf_fx;  /* cap_multiplier_PU = 1 */
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_rf_cm_ref_fx - (int64)CONST_303_FX);
	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_280_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_rf_cm_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_rf_cm_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_rf_cm_dec_fx,
				0, 255));
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_RF_CM, rfpll_lf_lf_rf_cm_ideal);

	/* rfpll_lf_lf_c1 */
	rfpll_lf_lf_c1_ref_fx  = c1_cap_multiplier_mode_fx;
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_c1_ref_fx - (int64)CONST_3_FX);
	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_P52_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_c1_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_c1_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_c1_dec_fx, 0, 255));
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_C1, rfpll_lf_lf_c1_ideal);

	/* rfpll_lf_lf_c2 */
	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(c1_passive_lf_fx, 20, NF16, NF0, &rfpll_lf_lf_c2_ref_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	rfpll_lf_lf_c2_ref_fx = fp_round_32(rfpll_lf_lf_c2_ref_fx, (nf - NF16));
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_c2_ref_fx - (int64)CONST_1P32_FX);
	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_P25_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_c2_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_c2_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_c2_dec_fx, 0, 255));
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_C2, rfpll_lf_lf_c2_ideal);

	/* rfpll_lf_lf_c3 */
	/* <0.16.16> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(c1_passive_lf_fx, 40, NF16, NF0, &rfpll_lf_lf_c3_ref_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	rfpll_lf_lf_c3_ref_fx = fp_round_32(rfpll_lf_lf_c3_ref_fx, (nf - NF16));
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_c3_ref_fx - (int64)CONST_P663_FX);
	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_P125_FX, NF16, NF16, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_c3_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_c3_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_c3_dec_fx, 0, 255));
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_C3, rfpll_lf_lf_c3_ideal);

	/* rfpll_lf_lf_c4 */
	rfpll_lf_lf_c4_ref_fx = rfpll_lf_lf_c3_ref_fx >> 1;
	/* <0.16.16> - <0.16.16> -> <0.16.16> */
	temp_sign = (int64)((int64)rfpll_lf_lf_c4_ref_fx - (int64)CONST_P317_FX);

	if (temp_sign > 0) {
		/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
		nf = fp_div_64((uint64)temp_sign, CONST_P061_FX, NF16, NF32, &temp_32);
		/* round(<0.(32-nf).nf>, nf) -> <0.32.0> */
		rfpll_lf_lf_c4_dec_fx = fp_round_32(temp_32, nf);
		rfpll_lf_lf_c4_ideal = (uint16)(LIMIT((int32)rfpll_lf_lf_c4_dec_fx, 0, 255));
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_LF_LF_C4, rfpll_lf_lf_c4_ideal);

	/* cp_kpd_scale */
	/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
	nf = fp_div_64(icp_fx, CONST_P0384_FX, NF16, NF0, &rfpll_cp_kpd_scale_ref_fx);
	/* round(<0.(32-nf).nf>, (nf-16)) -> <0.16.16> */
	rfpll_cp_kpd_scale_dec_fx = fp_round_32(rfpll_cp_kpd_scale_ref_fx, (nf - NF16));
	rfpll_cp_kpd_scale_ideal = (uint16)(LIMIT((int32)rfpll_cp_kpd_scale_dec_fx,
			0, 127));
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_CP_KPD_SCALE, rfpll_cp_kpd_scale_ideal);

	/* rfpll_cp_ioff */
	/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
	nf = fp_div_64(i_off_fx, CONST_P032_FX, NF16, NF16, &temp_32);
	/* round(<0.16.16>, 16) -> <0.16.16> */
	temp_32 = fp_round_32(temp_32, (nf - NF16));
	/* <0.16.16> / <0.16.16> -> <0.(32-nf).nf> */
	nf = fp_div_64(temp_32, rfpll_cp_kpd_scale_dec_fx, NF16, NF16, &temp_32_1);
	/* round(<0.(32-nf).nf>, (nf+16)) -> <0.16.16> ; here (nf+16) since nf < 16 */
	rfpll_cp_ioff_dec_fx = fp_round_32(temp_32_1, (nf+NF16));
	rfpll_cp_ioff_ideal = (uint16)(LIMIT((int32)rfpll_cp_ioff_dec_fx, 0, 255));
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_CP_IOFF, rfpll_cp_ioff_ideal);
	/* ------------------------------------------------------------------- */

	#if defined(PRINT_PLL_CONFIG_FLAG_20694) && PRINT_PLL_CONFIG_FLAG_20694
	/* ------------------------------------------------------------------- */
		print_pll_config_20694(pll, lo_freq, loop_band, icp_fx);
	#endif /* End of PRINT_PLL_CONFIG_FLAG_20694 */
}

void
print_pll_config_20694(pll_config_20694_tbl_t *pll, uint32 lo_freq, uint32 loop_bw, uint32 icp_fx)
{
	printf("------------------------------------------------------------\n");
	printf("\nLO_Freq = %d MHz\n", lo_freq);
	printf("\nLoopFilter_bandwidth = %d MHz\n", loop_bw);
	printf("\nIcp_fx = %u (0.16.16 format)\n", icp_fx);
	printf("\nRegister Definition (upper):\n");
	printf("------------------------------------------------------------\n");

	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_DELAYEND);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_DELAYSTARTCOLD);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_DELAYSTARTWARM);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_XTALCOUNT);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_DELTAPLLVAL);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_ERRORTHRES);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FASTSWITCH);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FIXMSB);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FOURTHMESEN);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_INITCAPA);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_INITCAPB);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_NORMCOUNTLEFT);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_NORMCOUNTRIGHT);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_PAUSECNT);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_ROUNDLSB);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_TARGETCOUNTBASE);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_TARGETCOUNTCENTER);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_OFFSETIN);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_PLL_VAL);

	printf("\nother VCOCAL register (not channel/XTAL dependent):\n");
	printf("------------------------------------------------------------\n");
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_SLOPEIN);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_UPDATESEL);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_CALCAPRBMODE);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_TESTVCOCNT);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FORCE_CAPS_OVR);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FORCE_CAPS_OVRVAL);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FAST_SETTLE_OVR);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FAST_SETTLE_OVRVAL);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FORCE_VCTRL_OVR);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCOCAL_FORCE_VCTRL_OVRVAL);

	printf("\nVCOCAL register (required by coupled VCOs):\n");
	printf("------------------------------------------------------------\n");
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_ENABLECOUPLING);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_COUPLINGMODE);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_SWAPVCO12);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_MIDCODESEL);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_COUPLTHRES);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_COUPLTHRES2);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_SECONDMESEN);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_UPDATESELCOUP);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_COUPLINGIN);

	printf("\nVCOCAL register (optional for coupled VCOs):\n");
	printf("------------------------------------------------------------\n");
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_FORCE_CAPS2_OVR);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_FORCE_CAPS2_OVRVAL);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_FORCE_AUX1_OVR);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_FORCE_AUX1_OVRVAL);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_FORCE_AUX2_OVR);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCOCAL_FORCE_AUX2_OVRVAL);

	printf("\nVCO register:\n");
	printf("------------------------------------------------------------\n");
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCO_CORE1_EN);
	PRINT_PLL_CONFIG_20694_MAIN(pll, RFPLL_VCO_CORE2_EN);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCO_CAP_MODE);

	printf("\nFract-N (Sigma-Delta) register:\n");
	printf("------------------------------------------------------------\n");
	PRINT_PLL_CONFIG_20694(pll, RFPLL_FRCT_WILD_BASE_HIGH);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_FRCT_WILD_BASE_LOW);

	printf("\nVCO Register (11n vs 11ac mode):\n");
	printf("------------------------------------------------------------\n");
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCO_CVAR);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCO_ALC_REF_CTRL);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCO_BIAS_MODE);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCO_TEMPCO_DCADJ);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_VCO_TEMPCO);

	printf("\nCP and Loop Filter register:\n");
	printf("------------------------------------------------------------\n");
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_R2);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_R3);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_RS_CM);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_RF_CM);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_C1);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_C2);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_C3);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_LF_LF_C4);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_CP_IOFF);
	PRINT_PLL_CONFIG_20694(pll, RFPLL_CP_KPD_SCALE);

}

static void
BCMATTACHFN(phy_ac_radio20695_pll_config_const_calc)(phy_info_t *pi, pll_config_20695_tbl_t *pll)
{
	uint32 xtal_freq;
	uint32 xtal_fx;
	uint64 temp_64;
	uint8 nf;

	/* Default parameters to be used in chan dependent computation */
	pll->use_doubler = USE_DOUBLER;
	pll->loop_bw = LOOP_BW;
	pll->logen_mode = LOGEN_MODE;

	/* ------------------------------------------------------------------- */
	/* XTAL frequency in 1.7.24 */
	xtal_freq = pi->xtalfreq;
	/* <0.32.0> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(xtal_freq, 1000000, 0, 0, &xtal_fx);
	/* round(<0.(32-nf).nf>, (nf-24)) -> <0.8.24> */
	pll->xtal_fx = fp_round_32(xtal_fx, (nf - NF24));
	xtal_fx = pll->xtal_fx;
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* XTAL dependent */
	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_DELAYEND_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_DELAYEND, (uint16)fp_round_64(temp_64, NF16));

	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_DELAYSTARTCOLD_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_DELAYSTARTCOLD,
			(uint16)fp_round_64(temp_64, NF16));

	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_DELAYSTARTWARM_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_DELAYSTARTWARM,
			(uint16)fp_round_64(temp_64, NF16));

	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_PAUSECNT_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_PAUSECNT, (uint16)fp_round_64(temp_64, NF16));
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Put other register values to structure */
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_TARGETCOUNTBASE, RFPLL_VCOCAL_TARGETCOUNTBASE);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_OFFSETIN, RFPLL_VCOCAL_OFFSETIN);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_UPDATESEL, RFPLL_VCOCAL_UPDATESEL_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_CALCAPRBMODE, RFPLL_VCOCAL_CALCAPRBMODE_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_TESTVCOCNT, RFPLL_VCOCAL_TESTVCOCNT_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS_OVR,
			RFPLL_VCOCAL_FORCE_CAPS_OVR_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS_OVRVAL,
			RFPLL_VCOCAL_FORCE_CAPS_OVRVAL_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FAST_SETTLE_OVR,
			RFPLL_VCOCAL_FAST_SETTLE_OVR_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FAST_SETTLE_OVRVAL,
			RFPLL_VCOCAL_FAST_SETTLE_OVRVAL_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_VCTRL_OVR,
			RFPLL_VCOCAL_FORCE_VCTRL_OVR_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_VCTRL_OVRVAL,
			RFPLL_VCOCAL_FORCE_VCTRL_OVRVAL_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_ENABLECOUPLING,
			RFPLL_VCOCAL_ENABLECOUPLING_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLINGMODE, RFPLL_VCOCAL_COUPLINGMODE_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_SWAPVCO12, RFPLL_VCOCAL_SWAPVCO12_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLTHRES, RFPLL_VCOCAL_COUPLTHRES_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLTHRES2, RFPLL_VCOCAL_COUPLTHRES2_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_SECONDMESEN, RFPLL_VCOCAL_SECONDMESEN_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_UPDATESELCOUP, RFPLL_VCOCAL_UPDATESELCOUP_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLINGIN, RFPLL_VCOCAL_COUPLINGIN_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS2_OVR,
			RFPLL_VCOCAL_FORCE_CAPS2_OVR_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS2_OVRVAL,
			RFPLL_VCOCAL_FORCE_CAPS2_OVRVAL_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX1_OVR,
			RFPLL_VCOCAL_FORCE_AUX1_OVR_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX1_OVRVAL,
			RFPLL_VCOCAL_FORCE_AUX1_OVRVAL_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX2_OVR,
			RFPLL_VCOCAL_FORCE_AUX2_OVR_DEC);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX2_OVRVAL,
			RFPLL_VCOCAL_FORCE_AUX2_OVRVAL_DEC);

	PLL_CONFIG_20695_VAL_ENTRY(pll, OVR_RFPLL_VCOCAL_OFFSETIN, 0);
	PLL_CONFIG_20695_VAL_ENTRY(pll, OVR_RFPLL_VCOCAL_COUPLINGIN, 1);
	PLL_CONFIG_20695_VAL_ENTRY(pll, OVR_MUX_XT_DEL, 1);
	PLL_CONFIG_20695_VAL_ENTRY(pll, OVR_MUX_SEL_BAND, 1);

	/* Reset SD modulator after setting new channel parameters */
	PLL_CONFIG_20695_VAL_ENTRY(pll, OVR_RFPLL_RST_N, 1);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_RST_N_1, 0);
	PLL_CONFIG_20695_VAL_ENTRY(pll, RFPLL_RST_N_2, 1);
	/* ------------------------------------------------------------------- */
}

static void
BCMATTACHFN(phy_ac_radio20694_pll_config_const_calc)(phy_info_t *pi, pll_config_20694_tbl_t *pll)
{
	uint32 xtal_freq;
	uint32 xtal_fx;
	uint64 temp_64;
	uint8 nf;
	uint sicoreunit;

	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	/* Default parameters to be used in chan dependent computation */
	pll->use_doubler = USE_DOUBLER;
	// pll->loop_bw = LOOP_BW;
	pll->logen_mode = LOGEN_MODE;

	/* ------------------------------------------------------------------- */
	/* XTAL frequency in 1.7.24 */
	xtal_freq = pi->xtalfreq;
	/* <0.32.0> / <0.32.0> -> <0.(32-nf).nf> */
	nf = fp_div_64(xtal_freq, 1000000, 0, 0, &xtal_fx);
	/* round(<0.(32-nf).nf>, (nf-24)) -> <0.8.24> */
	pll->xtal_fx = fp_round_32(xtal_fx, (nf - NF24));
	xtal_fx = pll->xtal_fx;
	/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* XTAL dependent */
	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_DELAYEND_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_DELAYEND, (uint16)fp_round_64(temp_64, NF16));

	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_DELAYSTARTCOLD_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_DELAYSTARTCOLD,
			(uint16)fp_round_64(temp_64, NF16));

	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_DELAYSTARTWARM_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_DELAYSTARTWARM,
			(uint16)fp_round_64(temp_64, NF16));

	/* <0.8.24> * <0.16.16> -> <0.16.16> */
	temp_64 = fp_mult_64(xtal_fx, RFPLL_VCOCAL_PAUSECNT_FX, NF24, NF16, NF16);
	/* round(0.16.16, 16) -> <0.32.0> */
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_PAUSECNT, (uint16)fp_round_64(temp_64, NF16));
/* ------------------------------------------------------------------- */

	/* ------------------------------------------------------------------- */
	/* Put other register values to structure */
	PLL_CONFIG_20694_VAL_ENTRY(pll, OVR_RFPLL_VCOCAL_SLOPEIN, 1);
	PLL_CONFIG_20694_VAL_ENTRY(pll, OVR_RFPLL_VCOCAL_OFFSETIN, 1);
	PLL_CONFIG_20694_VAL_ENTRY(pll, OVR_RFPLL_VCOCAL_RST_N, 1);
	PLL_CONFIG_20694_VAL_ENTRY(pll, OVR_RFPLL_VCOCAL_COUPLINGIN, 1);
	PLL_CONFIG_20694_VAL_ENTRY(pll, SEL_BAND, 3);
	PLL_CONFIG_20694_VAL_ENTRY(pll, OVR_MUX_XT_DEL, 1);
	PLL_CONFIG_20694_VAL_ENTRY(pll, OVR_MUX_SEL_BAND, 1);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_TARGETCOUNTBASE, RFPLL_VCOCAL_TARGETCOUNTBASE);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_OFFSETIN, RFPLL_VCOCAL_OFFSETIN);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_SLOPEIN, RFPLL_VCOCAL_SLOPEIN_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_UPDATESEL, RFPLL_VCOCAL_UPDATESEL_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_CALCAPRBMODE, RFPLL_VCOCAL_CALCAPRBMODE_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FASTSWITCH, RFPLL_VCOCAL_FASTSWITCH_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FIXMSB, RFPLL_VCOCAL_FIXMSB_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FOURTHMESEN, RFPLL_VCOCAL_FOURTHMESEN_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_TESTVCOCNT, RFPLL_VCOCAL_TESTVCOCNT_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS_OVR,
		RFPLL_VCOCAL_FORCE_CAPS_OVR_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS_OVRVAL,
		RFPLL_VCOCAL_FORCE_CAPS_OVRVAL_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FAST_SETTLE_OVR,
		RFPLL_VCOCAL_FAST_SETTLE_OVR_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FAST_SETTLE_OVRVAL,
		RFPLL_VCOCAL_FAST_SETTLE_OVRVAL_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_VCTRL_OVR,
		RFPLL_VCOCAL_FORCE_VCTRL_OVR_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_VCTRL_OVRVAL,
		RFPLL_VCOCAL_FORCE_VCTRL_OVRVAL_DEC);
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCOCAL_TARGETCOUNTBASE,
		RFPLL_VCOCAL_TARGETCOUNTBASE_DEC);
	if (sicoreunit == DUALMAC_MAIN) {
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLINGMODE,
			RFPLL_VCOCAL_COUPLINGMODE_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_SWAPVCO12,
			RFPLL_VCOCAL_SWAPVCO12_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLTHRES,
			RFPLL_VCOCAL_COUPLTHRES_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_SECONDMESEN,
			RFPLL_VCOCAL_SECONDMESEN_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_UPDATESELCOUP,
			RFPLL_VCOCAL_UPDATESELCOUP_DEC_20694);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_COUPLINGIN,
			RFPLL_VCOCAL_COUPLINGIN_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_CAPS2_OVRVAL,
			RFPLL_VCOCAL_FORCE_CAPS2_OVRVAL_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX1_OVR,
			RFPLL_VCOCAL_FORCE_AUX1_OVR_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX1_OVRVAL,
			RFPLL_VCOCAL_FORCE_AUX1_OVRVAL_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX2_OVR,
			RFPLL_VCOCAL_FORCE_AUX2_OVR_DEC);
		PLL_CONFIG_20694_MAIN_VAL_ENTRY(pll, RFPLL_VCOCAL_FORCE_AUX2_OVRVAL,
			RFPLL_VCOCAL_FORCE_AUX2_OVRVAL_DEC);
	}
	PLL_CONFIG_20694_VAL_ENTRY(pll, RFPLL_VCO_CAP_MODE, RFPLL_VCO_CAP_MODE_DEC);
	/* ------------------------------------------------------------------- */
}

static int
BCMATTACHFN(phy_ac_radio20695_populate_pll_config_tbl)(phy_info_t *pi,
		pll_config_20695_tbl_t *pll)
{
	if ((pll->reg_addr_2g =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_addr_2g failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_addr_5g =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_addr_5g failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_mask_2g =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_mask_2g failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_mask_5g =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_mask_5g failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_shift_2g =
		phy_malloc(pi, (sizeof(uint8) * PLL_CONFIG_20695_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_shift_2g failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_shift_5g =
		phy_malloc(pi, (sizeof(uint8) * PLL_CONFIG_20695_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_shift_5g failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_val =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_val failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register addresses */
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, OVR_RFPLL_VCOCAL_OFFSETIN, RFP, PLL_VCOCAL_OVR1,
			PLL5G_VCOCAL_OVR1, ovr_rfpll_vcocal_offsetIn,
			ovr_rfpll_5g_vcocal_offsetIn);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, OVR_RFPLL_VCOCAL_COUPLINGIN, RFP, PLL_VCOCAL_OVR1,
			PLL5G_VCOCAL_OVR1, ovr_rfpll_vcocal_couplingIn,
			ovr_rfpll_5g_vcocal_couplingIn);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, OVR_MUX_XT_DEL, RFP, PLL_MUXSELECT_LINE,
			PLL5G_MUXSELECT_LINE, ovr_mux_xt_del, ovr_pll5g_mux_xt_del);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, OVR_MUX_SEL_BAND, RFP, PLL_MUXSELECT_LINE,
			PLL5G_MUXSELECT_LINE, ovr_mux_sel_band, ovr_pll5g_mux_sel_band);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, XTALCNT_2G, RFP, PLL_XTAL_CNT1, PLL5G_XTAL_CNT1,
			XtalCnt_2g, pll5g_XtalCnt_2g);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, XTALCNT_5G, RFP, PLL_XTAL_CNT2, PLL5G_XTAL_CNT2,
			XtalCnt_5g, pll5g_XtalCnt_5g);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELAYEND, RFP, PLL_VCOCAL18,
			PLL5G_VCOCAL18, rfpll_vcocal_DelayEnd, rfpll_5g_vcocal_DelayEnd);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELAYSTARTCOLD, RFP, PLL_VCOCAL3,
			PLL5G_VCOCAL3, rfpll_vcocal_DelayStartCold,
			rfpll_5g_vcocal_DelayStartCold);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELAYSTARTWARM, RFP, PLL_VCOCAL4,
			PLL5G_VCOCAL4, rfpll_vcocal_DelayStartWarm,
			rfpll_5g_vcocal_DelayStartWarm);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_XTALCOUNT, RFP, PLL_VCOCAL7,
			PLL5G_VCOCAL7, rfpll_vcocal_XtalCount, rfpll_5g_vcocal_XtalCount);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_CALCAPRBMODE, RFP, PLL_VCOCAL7,
			PLL5G_VCOCAL7, rfpll_vcocal_CalCapRBMode, rfpll_5g_vcocal_CalCapRBMode);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELTAPLLVAL, RFP, PLL_VCOCAL8,
			PLL5G_VCOCAL8, rfpll_vcocal_DeltaPllVal, rfpll_5g_vcocal_DeltaPllVal);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_ERRORTHRES, RFP, PLL_VCOCAL20,
			PLL5G_VCOCAL20, rfpll_vcocal_ErrorThres, rfpll_5g_vcocal_ErrorThres);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_INITCAPA, RFP, PLL_VCOCAL12,
			PLL5G_VCOCAL12, rfpll_vcocal_InitCapA, rfpll_5g_vcocal_InitCapA);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_INITCAPB, RFP, PLL_VCOCAL13,
			PLL5G_VCOCAL13, rfpll_vcocal_InitCapB, rfpll_5g_vcocal_InitCapB);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_NORMCOUNTLEFT, RFP, PLL_VCOCAL10,
			PLL5G_VCOCAL10, rfpll_vcocal_NormCountLeft, rfpll_5g_vcocal_NormCountLeft);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_NORMCOUNTRIGHT, RFP, PLL_VCOCAL11,
			PLL5G_VCOCAL11, rfpll_vcocal_NormCountRight,
			rfpll_5g_vcocal_NormCountRight);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_PAUSECNT, RFP, PLL_VCOCAL19,
			PLL5G_VCOCAL19, rfpll_vcocal_PauseCnt, rfpll_5g_vcocal_PauseCnt);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_ROUNDLSB, RFP, PLL_VCOCAL1,
			PLL5G_VCOCAL1, rfpll_vcocal_RoundLSB, rfpll_5g_vcocal_RoundLSB);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_UPDATESEL, RFP, PLL_VCOCAL1,
			PLL5G_VCOCAL1, rfpll_vcocal_UpdateSel, rfpll_5g_vcocal_UpdateSel);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_TESTVCOCNT, RFP, PLL_VCOCAL1,
			PLL5G_VCOCAL1, rfpll_vcocal_testVcoCnt, rfpll_5g_vcocal_testVcoCnt);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FAST_SETTLE_OVR, RFP, PLL_VCOCAL1,
			PLL5G_VCOCAL1, rfpll_vcocal_fast_settle_ovr,
			rfpll_5g_vcocal_fast_settle_ovr);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FAST_SETTLE_OVRVAL, RFP, PLL_VCOCAL1,
			PLL5G_VCOCAL1, rfpll_vcocal_fast_settle_ovrVal,
			rfpll_5g_vcocal_fast_settle_ovrVal);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_VCTRL_OVR, RFP, PLL_VCOCAL1,
			PLL5G_VCOCAL1, rfpll_vcocal_force_vctrl_ovr,
			rfpll_5g_vcocal_force_vctrl_ovr);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_VCTRL_OVRVAL, RFP, PLL_VCOCAL1,
			PLL5G_VCOCAL1, rfpll_vcocal_force_vctrl_ovrVal,
			rfpll_5g_vcocal_force_vctrl_ovrVal);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_TARGETCOUNTBASE, RFP, PLL_VCOCAL6,
			PLL5G_VCOCAL6, rfpll_vcocal_TargetCountBase,
			rfpll_5g_vcocal_TargetCountBase);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_TARGETCOUNTCENTER, RFP, PLL_VCOCAL9,
			PLL5G_VCOCAL9, rfpll_vcocal_TargetCountCenter,
			rfpll_5g_vcocal_TargetCountCenter);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_OFFSETIN, RFP, PLL_VCOCAL17,
			PLL5G_VCOCAL17, rfpll_vcocal_offsetIn, rfpll_5g_vcocal_offsetIn);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_PLL_VAL, RFP, PLL_VCOCAL5,
			PLL5G_VCOCAL5, rfpll_vcocal_pll_val, rfpll_5g_vcocal_pll_val);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS_OVR, RFP, PLL_VCOCAL2,
			PLL5G_VCOCAL2, rfpll_vcocal_force_caps_ovr,
			rfpll_5g_vcocal_force_caps_ovr);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS_OVRVAL, RFP, PLL_VCOCAL2,
			PLL5G_VCOCAL2, rfpll_vcocal_force_caps_ovrVal,
			rfpll_5g_vcocal_force_caps_ovrVal);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_FRCT_WILD_BASE_HIGH, RFP, PLL_FRCT2,
			PLL5G_FRCT2, rfpll_frct_wild_base_high, rfpll_5g_frct_wild_base_high);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_FRCT_WILD_BASE_LOW, RFP, PLL_FRCT3,
			PLL5G_FRCT3, rfpll_frct_wild_base_low, rfpll_5g_frct_wild_base_low);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_ENABLECOUPLING, RFP, PLL_VCOCAL24,
			PLL5G_VCOCAL24, rfpll_vcocal_enableCoupling,
			rfpll_5g_vcocal_enableCoupling);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLINGMODE, RFP, PLL_VCOCAL24,
			PLL5G_VCOCAL24, rfpll_vcocal_couplingMode, rfpll_5g_vcocal_couplingMode);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_SWAPVCO12, RFP, PLL_VCOCAL24,
			PLL5G_VCOCAL24, rfpll_vcocal_swapVco12, rfpll_5g_vcocal_swapVco12);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_MIDCODESEL, RFP, PLL_VCOCAL24,
			PLL5G_VCOCAL24, rfpll_vcocal_MidCodeSel, rfpll_5g_vcocal_MidCodeSel);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_SECONDMESEN, RFP, PLL_VCOCAL24,
			PLL5G_VCOCAL24, rfpll_vcocal_SecondMesEn, rfpll_5g_vcocal_SecondMesEn);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_UPDATESELCOUP, RFP, PLL_VCOCAL24,
			PLL5G_VCOCAL24, rfpll_vcocal_UpdateSelCoup, rfpll_5g_vcocal_UpdateSelCoup);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLTHRES, RFP, PLL_VCOCAL26,
			PLL5G_VCOCAL26, rfpll_vcocal_CouplThres, rfpll_5g_vcocal_CouplThres);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLTHRES2, RFP, PLL_VCOCAL26,
			PLL5G_VCOCAL26, rfpll_vcocal_CouplThres2, rfpll_5g_vcocal_CouplThres2);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLINGIN, RFP, PLL_VCOCAL25,
			PLL5G_VCOCAL25, rfpll_vcocal_couplingIn, rfpll_5g_vcocal_couplingIn);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS2_OVR, RFP, PLL_VCOCAL21,
			PLL5G_VCOCAL21, rfpll_vcocal_force_caps2_ovr,
			rfpll_5g_vcocal_force_caps2_ovr);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS2_OVRVAL, RFP,
			PLL_VCOCAL21, PLL5G_VCOCAL21, rfpll_vcocal_force_caps2_ovrVal,
			rfpll_5g_vcocal_force_caps2_ovrVal);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX1_OVR, RFP, PLL_VCOCAL22,
			PLL5G_VCOCAL22, rfpll_vcocal_force_aux1_ovr,
			rfpll_5g_vcocal_force_aux1_ovr);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX1_OVRVAL, RFP, PLL_VCOCAL22,
			PLL5G_VCOCAL22, rfpll_vcocal_force_aux1_ovrVal,
			rfpll_5g_vcocal_force_aux1_ovrVal);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX2_OVR, RFP, PLL_VCOCAL23,
			PLL5G_VCOCAL23, rfpll_vcocal_force_aux2_ovr,
			rfpll_5g_vcocal_force_aux2_ovr);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX2_OVRVAL, RFP, PLL_VCOCAL23,
			PLL5G_VCOCAL23, rfpll_vcocal_force_aux2_ovrVal,
			rfpll_5g_vcocal_force_aux2_ovrVal);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_R2, RFP, PLL_LF4, PLL5G_LF4,
			rfpll_lf_lf_r2, rfpll_5g_lf_lf_r2);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_R3, RFP, PLL_LF5, PLL5G_LF5,
			rfpll_lf_lf_r3, rfpll_5g_lf_lf_r3);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_RS_CM, RFP, PLL_LF7, PLL5G_LF7,
			rfpll_lf_lf_rs_cm, rfpll_5g_lf_lf_rs_cm);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_RF_CM, RFP, PLL_LF7, PLL5G_LF7,
			rfpll_lf_lf_rf_cm, rfpll_5g_lf_lf_rf_cm);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C1, RFP, PLL_LF2, PLL5G_LF2,
			rfpll_lf_lf_c1, rfpll_5g_lf_lf_c1);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C2, RFP, PLL_LF2, PLL5G_LF2,
			rfpll_lf_lf_c2, rfpll_5g_lf_lf_c2);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C3, RFP, PLL_LF3, PLL5G_LF3,
			rfpll_lf_lf_c3, rfpll_5g_lf_lf_c3);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C4, RFP, PLL_LF3, PLL5G_LF3,
			rfpll_lf_lf_c4, rfpll_5g_lf_lf_c4);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_CP_IOFF, RFP, PLL_CP4, PLL5G_CP4,
			rfpll_cp_ioff, rfpll_5g_cp_ioff);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_CP_KPD_SCALE, RFP, PLL_CP4, PLL5G_CP4,
			rfpll_cp_kpd_scale, rfpll_5g_cp_kpd_scale);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_RFVCO_KVCO_CODE, RFP, PLL_RFVCO1,
			PLL5G_RFVCO1, rfpll_rfvco_KVCO_CODE, rfpll_5g_rfvco_KVCO_CODE);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_RFVCO_TEMP_CODE, RFP, PLL_RFVCO1,
			PLL5G_RFVCO1, rfpll_rfvco_TEMP_CODE, rfpll_5g_rfvco_TEMP_CODE);

	/* Reset SD modulator after setting new channel parameters */
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, OVR_RFPLL_RST_N, RFP, PLL_OVR1, PLL5G_OVR1,
			ovr_rfpll_rst_n, ovr_rfpll_5g_rst_n);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_RST_N_1, RFP, PLL_CFG2, PLL5G_CFG2,
			rfpll_rst_n, rfpll_5g_rst_n);
	PLL_CONFIG_20695_REG_INFO_ENTRY(pi, pll, RFPLL_RST_N_2, RFP, PLL_CFG2, PLL5G_CFG2,
			rfpll_rst_n, rfpll_5g_rst_n);

	return BCME_OK;

fail:
	if (pll->reg_addr_2g != NULL) {
		phy_mfree(pi, pll->reg_addr_2g, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	if (pll->reg_addr_5g != NULL) {
		phy_mfree(pi, pll->reg_addr_5g, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	if (pll->reg_field_mask_2g != NULL) {
		phy_mfree(pi, pll->reg_field_mask_2g,
				(sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	if (pll->reg_field_mask_5g != NULL) {
		phy_mfree(pi, pll->reg_field_mask_5g,
				(sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	if (pll->reg_field_shift_2g != NULL) {
		phy_mfree(pi, pll->reg_field_shift_2g,
				(sizeof(uint8) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	if (pll->reg_field_shift_5g != NULL) {
		phy_mfree(pi, pll->reg_field_shift_5g,
				(sizeof(uint8) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	if (pll->reg_field_val != NULL) {
		phy_mfree(pi, pll->reg_field_val, (sizeof(uint16) * PLL_CONFIG_20695_ARRAY_SIZE));
	}

	return BCME_NOMEM;
}

static int
BCMATTACHFN(phy_ac_radio20694_populate_pll_config_tbl)(phy_info_t *pi,
		pll_config_20694_tbl_t *pll)
{

uint sicoreunit;
sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	if ((pll->reg_addr =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_addr failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_mask =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_mask failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_shift =
		phy_malloc(pi, (sizeof(uint8) * PLL_CONFIG_20694_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_shift failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pll->reg_field_val =
		phy_malloc(pi, (sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE))) == NULL) {
		PHY_ERROR(("%s: phy_malloc reg_field_val failed\n", __FUNCTION__));
		goto fail;
	}

	if (sicoreunit == DUALMAC_MAIN) {
		if ((pll->reg_addr_main =
			phy_malloc(pi, (sizeof(uint16) *
				PLL_CONFIG_20694_MAIN_ARRAY_SIZE))) == NULL) {
			PHY_ERROR(("%s: phy_malloc reg_addr_main failed\n", __FUNCTION__));
			goto fail;
		}

		if ((pll->reg_field_mask_main =
			phy_malloc(pi, (sizeof(uint16) *
				PLL_CONFIG_20694_MAIN_ARRAY_SIZE))) == NULL) {
			PHY_ERROR(("%s: phy_malloc reg_field_mask_main failed\n", __FUNCTION__));
			goto fail;
		}

		if ((pll->reg_field_shift_main =
			phy_malloc(pi, (sizeof(uint8) *
				PLL_CONFIG_20694_MAIN_ARRAY_SIZE))) == NULL) {
			PHY_ERROR(("%s: phy_malloc reg_field_shift_main failed\n", __FUNCTION__));
			goto fail;
		}

		if ((pll->reg_field_val_main =
			phy_malloc(pi, (sizeof(uint16) *
				PLL_CONFIG_20694_MAIN_ARRAY_SIZE))) == NULL) {
			PHY_ERROR(("%s: phy_malloc reg_field_val_main failed\n", __FUNCTION__));
			goto fail;
		}
	}
	/* Register addresses */
	if (sicoreunit == DUALMAC_MAIN) {
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_CORE1_EN, RFP, PLL_CFG5,
			rfpll_vco_core1_en);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_CORE2_EN, RFP, PLL_CFG5,
			rfpll_vco_core2_en);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_ENABLECOUPLING,
			RFP, PLL_VCOCAL24, rfpll_vcocal_enableCoupling);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLINGMODE,
			RFP, PLL_VCOCAL24, rfpll_vcocal_couplingMode);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_SWAPVCO12,
			RFP, PLL_VCOCAL24, rfpll_vcocal_swapVco12);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_MIDCODESEL,
			RFP, PLL_VCOCAL24, rfpll_vcocal_MidCodeSel);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_SECONDMESEN,
			RFP, PLL_VCOCAL24, rfpll_vcocal_SecondMesEn);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_UPDATESELCOUP,
			RFP, PLL_VCOCAL24, rfpll_vcocal_UpdateSelCoup);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLTHRES,
			RFP, PLL_VCOCAL26, rfpll_vcocal_CouplThres);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLTHRES2,
			RFP, PLL_VCOCAL26, rfpll_vcocal_CouplThres2);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_COUPLINGIN,
			RFP, PLL_VCOCAL25, rfpll_vcocal_couplingIn);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS2_OVR,
			RFP, PLL_VCOCAL21, rfpll_vcocal_force_caps2_ovr);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS2_OVRVAL,
			RFP, PLL_VCOCAL21, rfpll_vcocal_force_caps2_ovrVal);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX1_OVR,
			RFP, PLL_VCOCAL22, rfpll_vcocal_force_aux1_ovr);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX1_OVRVAL,
			RFP, PLL_VCOCAL22, rfpll_vcocal_force_aux1_ovrVal);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX2_OVR,
			RFP, PLL_VCOCAL23, rfpll_vcocal_force_aux2_ovr);
		PLL_CONFIG_20694_MAIN_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_AUX2_OVRVAL,
			RFP, PLL_VCOCAL23, rfpll_vcocal_force_aux2_ovrVal);
	}
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, OVR_RFPLL_VCOCAL_SLOPEIN, RFP, PLL_VCOCAL_OVR1,
			ovr_rfpll_vcocal_slopeIn);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, OVR_RFPLL_VCOCAL_OFFSETIN, RFP, PLL_VCOCAL_OVR1,
			ovr_rfpll_vcocal_offsetIn);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, OVR_RFPLL_VCOCAL_RST_N, RFP, PLL_VCOCAL_OVR1,
			ovr_rfpll_vcocal_rst_n);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, OVR_RFPLL_VCOCAL_COUPLINGIN, RFP, PLL_VCOCAL_OVR1,
			ovr_rfpll_vcocal_couplingIn);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, SEL_BAND, RFP, PLL_MUXSELECT_LINE,
			sel_band);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, OVR_MUX_XT_DEL, RFP, PLL_MUXSELECT_LINE,
			ovr_mux_xt_del);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, OVR_MUX_SEL_BAND, RFP, PLL_MUXSELECT_LINE,
			ovr_mux_sel_band);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELAYEND, RFP, PLL_VCOCAL18,
			rfpll_vcocal_DelayEnd);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELAYSTARTCOLD, RFP, PLL_VCOCAL3,
			rfpll_vcocal_DelayStartCold);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELAYSTARTWARM, RFP, PLL_VCOCAL4,
			rfpll_vcocal_DelayStartWarm);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_XTALCOUNT, RFP, PLL_VCOCAL7,
			rfpll_vcocal_XtalCount);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_CALCAPRBMODE, RFP, PLL_VCOCAL7,
			rfpll_vcocal_CalCapRBMode);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_DELTAPLLVAL, RFP, PLL_VCOCAL8,
			rfpll_vcocal_DeltaPllVal);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_ERRORTHRES, RFP, PLL_VCOCAL20,
			rfpll_vcocal_ErrorThres);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_INITCAPA, RFP, PLL_VCOCAL12,
			rfpll_vcocal_InitCapA);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_INITCAPB, RFP, PLL_VCOCAL13,
			rfpll_vcocal_InitCapB);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_NORMCOUNTLEFT, RFP, PLL_VCOCAL10,
			rfpll_vcocal_NormCountLeft);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_NORMCOUNTRIGHT, RFP, PLL_VCOCAL11,
			rfpll_vcocal_NormCountRight);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_PAUSECNT, RFP, PLL_VCOCAL19,
			rfpll_vcocal_PauseCnt);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_ROUNDLSB, RFP, PLL_VCOCAL1,
			rfpll_vcocal_RoundLSB);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_UPDATESEL, RFP, PLL_VCOCAL1,
			rfpll_vcocal_UpdateSel);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_TESTVCOCNT, RFP, PLL_VCOCAL1,
			rfpll_vcocal_testVcoCnt);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FAST_SETTLE_OVR, RFP, PLL_VCOCAL1,
			rfpll_vcocal_fast_settle_ovr);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FAST_SETTLE_OVRVAL, RFP, PLL_VCOCAL1,
			rfpll_vcocal_fast_settle_ovrVal);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_VCTRL_OVR, RFP, PLL_VCOCAL1,
			rfpll_vcocal_force_vctrl_ovr);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_VCTRL_OVRVAL, RFP, PLL_VCOCAL1,
			rfpll_vcocal_force_vctrl_ovrVal);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_TARGETCOUNTBASE, RFP, PLL_VCOCAL6,
			rfpll_vcocal_TargetCountBase);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_TARGETCOUNTCENTER, RFP, PLL_VCOCAL9,
			rfpll_vcocal_TargetCountCenter);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_OFFSETIN, RFP, PLL_VCOCAL17,
			rfpll_vcocal_offsetIn);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_SLOPEIN, RFP, PLL_VCOCAL15,
			rfpll_vcocal_slopeIn);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FASTSWITCH, RFP, PLL_VCOCAL1,
			rfpll_vcocal_FastSwitch);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FIXMSB, RFP, PLL_VCOCAL1,
			rfpll_vcocal_FixMSB);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FOURTHMESEN, RFP, PLL_VCOCAL1,
			rfpll_vcocal_FourthMesEn);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_CAP_MODE, RFP, PLL_VCO2,
			rfpll_vco_cap_mode);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_PLL_VAL, RFP, PLL_VCOCAL5,
			rfpll_vcocal_pll_val);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS_OVR, RFP, PLL_VCOCAL2,
			rfpll_vcocal_force_caps_ovr);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCOCAL_FORCE_CAPS_OVRVAL, RFP, PLL_VCOCAL2,
			rfpll_vcocal_force_caps_ovrVal);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_FRCT_WILD_BASE_HIGH, RFP, PLL_FRCT2,
			rfpll_frct_wild_base_high);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_FRCT_WILD_BASE_LOW, RFP, PLL_FRCT3,
			rfpll_frct_wild_base_low);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_CVAR, RFP, PLL_VCO2,
			rfpll_vco_cvar);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_ALC_REF_CTRL, RFP, PLL_VCO6,
			rfpll_vco_ALC_ref_ctrl);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_BIAS_MODE, RFP, PLL_VCO6,
			rfpll_vco_bias_mode);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_TEMPCO_DCADJ, RFP, PLL_VCO5,
			rfpll_vco_tempco_dcadj);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_VCO_TEMPCO, RFP, PLL_VCO4,
			rfpll_vco_tempco);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_R2, RFP, PLL_LF4, rfpll_lf_lf_r2);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_R3, RFP, PLL_LF5, rfpll_lf_lf_r3);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_RS_CM, RFP,
		PLL_LF7, rfpll_lf_lf_rs_cm);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_RF_CM, RFP,
		PLL_LF7, rfpll_lf_lf_rf_cm);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C1, RFP,
		PLL_LF2, rfpll_lf_lf_c1);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C2, RFP,
		PLL_LF2, rfpll_lf_lf_c2);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C3, RFP,
		PLL_LF3, rfpll_lf_lf_c3);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_LF_LF_C4, RFP,
		PLL_LF3, rfpll_lf_lf_c4);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_CP_IOFF, RFP,
		PLL_CP4, rfpll_cp_ioff);
	PLL_CONFIG_20694_REG_INFO_ENTRY(pi, pll, RFPLL_CP_KPD_SCALE, RFP,
		PLL_CP4, rfpll_cp_kpd_scale);

	return BCME_OK;

fail:
	if (pll->reg_addr != NULL) {
		phy_mfree(pi, pll->reg_addr, (sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE));
	}

	if (pll->reg_field_mask != NULL) {
		phy_mfree(pi, pll->reg_field_mask,
				(sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE));
	}

	if (pll->reg_field_shift != NULL) {
		phy_mfree(pi, pll->reg_field_shift,
				(sizeof(uint8) * PLL_CONFIG_20694_ARRAY_SIZE));
	}

	if (pll->reg_field_val != NULL) {
		phy_mfree(pi, pll->reg_field_val, (sizeof(uint16) * PLL_CONFIG_20694_ARRAY_SIZE));
	}

	if (sicoreunit == DUALMAC_MAIN) {
		if (pll->reg_addr_main != NULL) {
			phy_mfree(pi, pll->reg_addr_main,
				(sizeof(uint16) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
		}

		if (pll->reg_field_mask_main != NULL) {
			phy_mfree(pi, pll->reg_field_mask_main,
				(sizeof(uint16) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
		}

		if (pll->reg_field_shift_main != NULL) {
			phy_mfree(pi, pll->reg_field_shift_main,
				(sizeof(uint8) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
		}

		if (pll->reg_field_val_main != NULL) {
			phy_mfree(pi, pll->reg_field_val_main,
				(sizeof(uint16) * PLL_CONFIG_20694_MAIN_ARRAY_SIZE));
		}
	}
	return BCME_NOMEM;
}

static void
phy_ac_radio20695_write_pll_config(phy_info_t *pi, radio_pll_sel_t pll_sel,
		pll_config_20695_tbl_t *pll)
{
	uint16 *reg_val;
	uint16 *reg_addr;
	uint16 *reg_mask;
	uint8 *reg_shift;
	uint8 i;
	const chan_info_radio20695_rffe_t *chan_info = NULL;

	BCM_REFERENCE(chan_info);

	/* Get channel info */
	wlc_phy_chan2freq_20695(pi, CHSPEC_CHANNEL(pi->radio_chanspec), &chan_info);

	reg_val = pll->reg_field_val;

	if (pll_sel == PLL_2G) {
		reg_addr = pll->reg_addr_2g;
		reg_mask = pll->reg_field_mask_2g;
		reg_shift = pll->reg_field_shift_2g;
	} else {
		reg_addr = pll->reg_addr_5g;
		reg_mask = pll->reg_field_mask_5g;
		reg_shift = pll->reg_field_shift_5g;
	}

	/* Write to register fields */
	for (i = 0; i < PLL_CONFIG_20695_ARRAY_SIZE; i++) {
		phy_utils_mod_radioreg(pi, reg_addr[i], reg_mask[i], (reg_val[i] << reg_shift[i]));
	}

	/* WAR's (overwrite registers) */
	if (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardtype) == BCM943012FCREF_SSID) {
		/* 43012A0 FCBGA package */
		if (chan_info->freq == 2412) {
			PHY_INFORM(("%s : Ch1 600kHz Spur WAR: kvco = 40, lbw = 200\n",
					__FUNCTION__));
			ACPHY_REG_LIST_START
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF2, 0, rfpll_lf_lf_c1,
						RFPLL_LF_LF_C1_CH2412)
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF2, 0, rfpll_lf_lf_c2,
						RFPLL_LF_LF_C2_CH2412)
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF3, 0, rfpll_lf_lf_c3,
						RFPLL_LF_LF_C3_CH2412)
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF3, 0, rfpll_lf_lf_c4,
						RFPLL_LF_LF_C4_CH2412)
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF4, 0, rfpll_lf_lf_r2,
						RFPLL_LF_LF_R2_CH2412)
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF5, 0, rfpll_lf_lf_r3,
						RFPLL_LF_LF_R3_CH2412)
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF7, 0, rfpll_lf_lf_rf_cm,
						RFPLL_LF_LF_RF_CM_CH2412)
				MOD_RADIO_REG_28NM_ENTRY(pi, RFP, PLL_LF7, 0, rfpll_lf_lf_rs_cm,
						RFPLL_LF_LF_RS_CM_CH2412)
			ACPHY_REG_LIST_EXECUTE(pi);
		}
}
}

static void
phy_ac_radio20694_write_pll_config(phy_info_t *pi, pll_config_20694_tbl_t *pll)
{
	uint16 *reg_val;
	uint16 *reg_addr;
	uint16 *reg_mask;
	uint8 *reg_shift;
	uint16 *reg_val_main;
	uint16 *reg_addr_main;
	uint16 *reg_mask_main;
	uint8 *reg_shift_main;
	uint8 i;
	uint sicoreunit;

	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	reg_val = pll->reg_field_val;
	reg_addr = pll->reg_addr;
	reg_mask = pll->reg_field_mask;
	reg_shift = pll->reg_field_shift;
	reg_val_main = pll->reg_field_val_main;
	reg_addr_main = pll->reg_addr_main;
	reg_mask_main = pll->reg_field_mask_main;
	reg_shift_main = pll->reg_field_shift_main;

	/* Write to register fields */
	for (i = 0; i < PLL_CONFIG_20694_ARRAY_SIZE; i++) {
	  phy_utils_mod_radioreg(pi, reg_addr[i], reg_mask[i], (reg_val[i] << reg_shift[i]));
	}
	if (sicoreunit == DUALMAC_MAIN) {
		for (i = 0; i < PLL_CONFIG_20694_MAIN_ARRAY_SIZE; i++) {
			phy_utils_mod_radioreg(pi, reg_addr_main[i], reg_mask_main[i],
			(reg_val_main[i] << reg_shift_main[i]));
		}
	}
}

/* 20694 radio related functions */
int
wlc_phy_chan2freq_20694(phy_info_t *pi, uint8 channel,
	const chan_info_radio20694_rffe_t **chan_info)
{
	uint32 index, tbl_len;
	const chan_info_radio20694_rffe_t *p_chan_info_tbl;
	int status;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID) &&
		(ACMAJORREV_25(pi->pubpi->phy_rev) || ACMAJORREV_40(pi->pubpi->phy_rev)));

	if (!RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		PHY_ERROR(("wl%d: %s: Unsupported radio %d\n",
			pi->sh->unit, __FUNCTION__, RADIOID(pi->pubpi->radioid)));
		ASSERT(FALSE);
		return -1;
	}
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	status = wlc_phy_ac_get_20694_chan_info_tbl(pi, &p_chan_info_tbl, &tbl_len);

	if (!status) {
		for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
		     index++);

		if (index >= tbl_len) {
			PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
				pi->sh->unit, __FUNCTION__, channel));
			if (!ISSIM_ENAB(pi->sh->sih)) {
				/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(index < tbl_len);
			}
			return -1;
		}
		*chan_info = p_chan_info_tbl + index;
		return p_chan_info_tbl[index].freq;
	} else {
		return -1;
	}
}

int
wlc_phy_chan2freq_20696(phy_info_t *pi, uint8 channel,
	const chan_info_radio20696_rffe_t **chan_info)
{
	uint32 index, tbl_len;
	const chan_info_radio20696_rffe_t *p_chan_info_tbl;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20696_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	p_chan_info_tbl = chan_tune_20696_rev0;
	tbl_len = chan_tune_20696_rev0_length;
	for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
	     index++);

	if (index >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
			pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
			ASSERT(index < tbl_len);
		}
		return -1;
	}
	*chan_info = p_chan_info_tbl + index;
	return p_chan_info_tbl[index].freq;
}

static void
wlc_phy_radio20694_upd_prfd_values(phy_info_t *pi)
{
	radio_20xx_prefregs_t *prefregs_20694_ptr = NULL;
	uint sicoreunit;
	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID));
	if ((prefregs_20694_ptr = wlc_phy_ac_get_20694_prfd_vals_tbl(pi)) == NULL) {
		PHY_ERROR(("wl%d: %s: Preferred value table not known for radio_rev %d\n",
				pi->sh->unit, __FUNCTION__,
				RADIOREV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return;
	}
	/* Update preferred values */
	wlc_phy_init_radio_prefregs_allbands(pi, prefregs_20694_ptr);

	if (sicoreunit == DUALMAC_AUX) {
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, 0, iqdac_invclko,
			CHSPEC_IS2G(pi->radio_chanspec));
	}
}

static void
wlc_phy_radio20694_rffe_tune(phy_info_t *pi, const chan_info_radio20694_rffe_t *chan_info_rffe,
	uint8 core)
{

	uint sicoreunit;
	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	if (chan_info_rffe->freq > 4000) {
	/* 5G front end */
	    if (core == 0) {
			MOD_RADIO_REG_20694(pi, RF, LOGEN5G_REG5, core, logen5g_x3_ctune,
				chan_info_rffe->RF0_logen5g_reg5_logen5g_x3_ctune_5G);
			MOD_RADIO_REG_20694(pi, RF, RX5G_REG1, core, rx5g_lna_tune,
				chan_info_rffe->RF0_rx5g_reg1_rx5g_lna_tune_5G);
			MOD_RADIO_REG_20694(pi, RF, TXMIX5G_CFG6, core, mx5g_tune,
				chan_info_rffe->RF0_txmix5g_cfg6_mx5g_tune_5G);
			MOD_RADIO_REG_20694(pi, RF, PA5G_CFG4, core, pad5g_tune,
				chan_info_rffe->RF0_pa5g_cfg4_pad5g_tune_5G);
			MOD_RADIO_REG_20694(pi, RF, PA5G_CFG4, core, pa5g_tune,
				chan_info_rffe->RF0_pa5g_cfg4_pa5g_tune_5G);

	        if (sicoreunit == DUALMAC_AUX) {
				MOD_RADIO_REG_20694(pi, RF, LOGEN5G_REG6, core,
				logen5g_buf_ctune_c0,
				chan_info_rffe->RF0_logen5g_reg6_logen5g_buf_ctune_c0_5G_AUX);
				MOD_RADIO_REG_20694(pi, RF, LOGEN5G_REG6, core,
				logen5g_buf_ctune_c1,
				chan_info_rffe->RF0_logen5g_reg6_logen5g_buf_ctune_c1_5G_AUX);
	        } else {
				MOD_RADIO_REG_20694(pi, RF, LOGEN5G_REG8, core, logen5g_buf_ctune,
				chan_info_rffe->RF0_logen5g_reg8_logen5g_buf_ctune_5G);
				MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG5, core,
				logen5g_mimobuf_ctune,
				chan_info_rffe->RF0_logen2g_reg5_logen5g_mimobuf_ctune_5G);
	        }
	    } else if (core == 1) {
			MOD_RADIO_REG_20694(pi, RF, RX5G_REG1, core, rx5g_lna_tune,
				chan_info_rffe->RF1_rx5g_reg1_rx5g_lna_tune_5G);
			MOD_RADIO_REG_20694(pi, RF, TXMIX5G_CFG6, core, mx5g_tune,
				chan_info_rffe->RF1_txmix5g_cfg6_mx5g_tune_5G);
			MOD_RADIO_REG_20694(pi, RF, PA5G_CFG4, core, pad5g_tune,
				chan_info_rffe->RF1_pa5g_cfg4_pad5g_tune_5G);
			MOD_RADIO_REG_20694(pi, RF, PA5G_CFG4, core, pa5g_tune,
				chan_info_rffe->RF1_pa5g_cfg4_pa5g_tune_5G);
			if (sicoreunit == DUALMAC_MAIN) {
				MOD_RADIO_REG_20694(pi, RF, LOGEN5G_REG8, core, logen5g_buf_ctune,
				chan_info_rffe->RF1_logen5g_reg8_logen5g_buf_ctune_5G);
				MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG5,
					core, logen5g_mimobuf_ctune,
					chan_info_rffe->RF1_logen2g_reg5_logen5g_mimobuf_ctune_5G);
				MOD_RADIO_REG_20694(pi, RF, LOGEN5G_REG5, core, logen5g_x3_ctune,
					chan_info_rffe->RF1_logen5g_reg5_logen5g_x3_ctune_5G);
				MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG5,
					core, logen5g_mimobuf_ctune,
					chan_info_rffe->RF1_logen2g_reg5_logen5g_mimobuf_ctune_5G);
			}
		}
	} else {
	    if (core == 0) {
			MOD_RADIO_REG_20694(pi, RF, LNA2G_REG1, core, lna2g_lna1_freq_tune,
				chan_info_rffe->RF0_lna2g_reg1_lna2g_lna1_freq_tune_2G);
			MOD_RADIO_REG_20694(pi, RF, TXMIX2G_CFG5, core, mx2g_tune,
				chan_info_rffe->RF0_txmix2g_cfg5_mx2g_tune_2G);
			MOD_RADIO_REG_20694(pi, RF, PA2G_CFG2, core, pad2g_tune,
				chan_info_rffe->RF0_pa2g_cfg2_pad2g_tune_2G);
			MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG2, core, logen2g_x2_ctune,
				chan_info_rffe->RF0_logen2g_reg2_logen2g_x2_ctune_2G);
			MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG2, core, logen2g_buf_ctune,
				chan_info_rffe->RF0_logen2g_reg2_logen2g_buf_ctune_2G);

			if (sicoreunit == DUALMAC_AUX) {
			MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG4, core, logen2g_lobuf2g2_ctune,
				chan_info_rffe->RF0_logen2g_reg4_logen2g_lobuf2g2_ctune_2G_AUX);
			}

	    } else if (core == 1) {
			MOD_RADIO_REG_20694(pi, RF, LNA2G_REG1, core, lna2g_lna1_freq_tune,
				chan_info_rffe->RF1_lna2g_reg1_lna2g_lna1_freq_tune_2G);
			MOD_RADIO_REG_20694(pi, RF, TXMIX2G_CFG5, core, mx2g_tune,
				chan_info_rffe->RF1_txmix2g_cfg5_mx2g_tune_2G);
			MOD_RADIO_REG_20694(pi, RF, PA2G_CFG2, core, pad2g_tune,
				chan_info_rffe->RF1_pa2g_cfg2_pad2g_tune_2G);
			if (sicoreunit == DUALMAC_MAIN) {
				MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG2, core, logen2g_buf_ctune,
					chan_info_rffe->RF1_logen2g_reg2_logen2g_buf_ctune_2G);
				MOD_RADIO_REG_20694(pi, RF, LOGEN2G_REG2, core, logen2g_x2_ctune,
					chan_info_rffe->RF1_logen2g_reg2_logen2g_x2_ctune_2G);
			}
		}
	}
}

static void wlc_phy_radio20696_rffe_tune(phy_info_t *pi,
	const chan_info_radio20696_rffe_t *chan_info_rffe)
{
	if (CHSPEC_IS5G(pi->radio_chanspec)) {
		/* 5G front end */
		MOD_RADIO_PLLREG_20696(pi, LOGEN_REG1, logen_mix_ctune,
				chan_info_rffe->u.val_5G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
				chan_info_rffe->u.val_5G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, RXADC_REG2, 0, rxadc_ti_ctune_Q,
				chan_info_rffe->u.val_5G.RF0_rxadc_reg2_rxadc_ti_ctune_Q);
		MOD_RADIO_REG_20696(pi, RXADC_REG2, 0, rxadc_ti_ctune_I,
				chan_info_rffe->u.val_5G.RF0_rxadc_reg2_rxadc_ti_ctune_I);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG2, 0, mx5g_tune,
				chan_info_rffe->u.val_5G.RF0_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG4, 0, tx5g_pa_tune,
				chan_info_rffe->u.val_5G.RF0_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG3, 0, pad5g_tune,
				chan_info_rffe->u.val_5G.RF0_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20696(pi, RX5G_REG1, 0, rx5g_lna_tune,
				chan_info_rffe->u.val_5G.RF0_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
				chan_info_rffe->u.val_5G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG2, 1, mx5g_tune,
				chan_info_rffe->u.val_5G.RF1_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG4, 1, tx5g_pa_tune,
				chan_info_rffe->u.val_5G.RF1_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG3, 1, pad5g_tune,
				chan_info_rffe->u.val_5G.RF1_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20696(pi, RX5G_REG1, 1, rx5g_lna_tune,
				chan_info_rffe->u.val_5G.RF1_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
				chan_info_rffe->u.val_5G.RF2_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG2, 2, mx5g_tune,
				chan_info_rffe->u.val_5G.RF2_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG4, 2, tx5g_pa_tune,
				chan_info_rffe->u.val_5G.RF2_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG3, 2, pad5g_tune,
				chan_info_rffe->u.val_5G.RF2_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20696(pi, RX5G_REG1, 2, rx5g_lna_tune,
				chan_info_rffe->u.val_5G.RF2_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 3, logen_lc_ctune,
				chan_info_rffe->u.val_5G.RF3_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, TX5G_MIX_REG2, 3, mx5g_tune,
				chan_info_rffe->u.val_5G.RF3_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PA_REG4, 3, tx5g_pa_tune,
				chan_info_rffe->u.val_5G.RF3_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20696(pi, TX5G_PAD_REG3, 3, pad5g_tune,
				chan_info_rffe->u.val_5G.RF3_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20696(pi, RX5G_REG1, 3, rx5g_lna_tune,
				chan_info_rffe->u.val_5G.RF3_rx5g_reg1_rx5g_lna_tune);
	} else {
		/* 2G front end */
		MOD_RADIO_PLLREG_20696(pi, LOGEN_REG1, logen_mix_ctune,
				chan_info_rffe->u.val_2G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
				chan_info_rffe->u.val_2G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, RXADC_REG2, 0, rxadc_ti_ctune_Q,
				chan_info_rffe->u.val_2G.RF0_rxadc_reg2_rxadc_ti_ctune_Q);
		MOD_RADIO_REG_20696(pi, RXADC_REG2, 0, rxadc_ti_ctune_I,
				chan_info_rffe->u.val_2G.RF0_rxadc_reg2_rxadc_ti_ctune_I);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG4, 0, mx2g_tune,
				chan_info_rffe->u.val_2G.RF0_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG3, 0, pad2g_tune,
				chan_info_rffe->u.val_2G.RF0_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20696(pi, LNA2G_REG1, 0, lna2g_lna1_freq_tune,
				chan_info_rffe->u.val_2G.RF0_lna2g_reg1_lna2g_lna1_freq_tune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
				chan_info_rffe->u.val_2G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG4, 1, mx2g_tune,
				chan_info_rffe->u.val_2G.RF1_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG3, 1, pad2g_tune,
				chan_info_rffe->u.val_2G.RF1_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20696(pi, LNA2G_REG1, 1, lna2g_lna1_freq_tune,
				chan_info_rffe->u.val_2G.RF1_lna2g_reg1_lna2g_lna1_freq_tune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
				chan_info_rffe->u.val_2G.RF2_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG4, 2, mx2g_tune,
				chan_info_rffe->u.val_2G.RF2_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG3, 2, pad2g_tune,
				chan_info_rffe->u.val_2G.RF2_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20696(pi, LNA2G_REG1, 2, lna2g_lna1_freq_tune,
				chan_info_rffe->u.val_2G.RF2_lna2g_reg1_lna2g_lna1_freq_tune);
		MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG3, 3, logen_lc_ctune,
				chan_info_rffe->u.val_2G.RF3_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20696(pi, TX2G_MIX_REG4, 3, mx2g_tune,
				chan_info_rffe->u.val_2G.RF3_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20696(pi, TX2G_PAD_REG3, 3, pad2g_tune,
				chan_info_rffe->u.val_2G.RF3_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20696(pi, LNA2G_REG1, 3, lna2g_lna1_freq_tune,
				chan_info_rffe->u.val_2G.RF3_lna2g_reg1_lna2g_lna1_freq_tune);
	}
}


static void
wlc_phy_radio20694_upd_band_related_reg(phy_info_t *pi)
{
	uint16 fc;
	uint8 core;
	uint sicoreunit;
	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	/* Get center frequency of the channel */
	if (CHSPEC_CHANNEL(pi->radio_chanspec) > 14) {
	  fc = CHAN5G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
	} else {
	  fc = CHAN2G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
	}
	FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20694(pi, RF, PMU_CFG1, core, wlpmu_LOGENldo_hpm, 1);
	}

	MOD_RADIO_REG_20694(pi, RFP, XTAL_OVR1, 0, ovr_xtal_sel_bias_res, 1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL_OVR1, 0, ovr_xtal_refadj_cbuck, 1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL_OVR1, 0, ovr_xt_res_bypass, 1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL_OVR1, 0, ovr_xtal_core_strength_nmos, 1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL_OVR1, 0, ovr_xtal_core_strength_pmos, 1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL_OVR1, 0, ovr_xtal_bias_adj, 1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL1, 0, vref_adj_cbuck, 0);
	MOD_RADIO_REG_20694(pi, RFP, XTAL2, 0, xtal_coresize_nmos, 0x3f);
	MOD_RADIO_REG_20694(pi, RFP, XTAL2, 0, xtal_coresize_pmos, 0x3f);
	MOD_RADIO_REG_20694(pi, RFP, XTAL1, 0, xtal_core_buf_ds, 0x8);
	MOD_RADIO_REG_20694(pi, RFP, XTAL3, 0, xtal_sel_bias_res, 0);
	MOD_RADIO_REG_20694(pi, RFP, XTAL1, 0, xt_res_bypass, 0x7);
	MOD_RADIO_REG_20694(pi, RFP, XTAL3, 0, xtal_bias_adj, 0x6);

	MOD_RADIO_REG_20694(pi, RFP, PLL_VCO9, 0, rfpll_vco_cap_bias_buf_ib, 0x3);
	MOD_RADIO_REG_20694(pi, RFP, PLL_HVLDO2, 0, ldo_1p8_lowquiescenten_CP, 0x1);
	MOD_RADIO_REG_20694(pi, RFP, PLL_HVLDO2, 0, ldo_1p8_lowquiescenten_VCO, 0x1);
	MOD_RADIO_REG_20694(pi, RFP, PLL_LVLDO1, 0, ldo_1p0_lowquiescenten_PFDMMD, 0x1);
	MOD_RADIO_REG_20694(pi, RFP, PLL_CP5, 0, rfpll_cp_ioff_bias_n, 0x3);
	if (sicoreunit == DUALMAC_MAIN) {
	  if (fc < 3000) {
	    /* 2G front end */
	    FOREACH_CORE(pi, core) {
	      MOD_RADIO_REG_20694(pi, RF, PMU_CFG1, core, wlpmu_LOGENldo_hpm, 0);
	      MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_bbdc, 0xa);
	      MOD_RADIO_REG_20694(pi, RF, TX2G_CFG1_OVR, core, ovr_bst2g_pu, 1);
	      MOD_RADIO_REG_20694(pi, RF, TXMIX2G_CFG5, core, bst2g_pu, 0);
	      MOD_RADIO_REG_20694(pi, RF, TX_CFG2_OVR, core, ovr_bstr_bias_pu, 1);
	      MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG1, core, bstr_bias_pu, 0);
	      MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG6, core, ibias_uncal_20uA_LDO_2P5V_mag, 0xb);
	      MOD_RADIO_REG_20694(pi, RF, PA2G_CFG5, core, pa2g_stby_ldo_adj, 0x3);
	      MOD_RADIO_REG_20694(pi, RF, PA2G_CFG1, core, pa2g_tssi_ctrl_sel, 0x0);

	      //MOD_RADIO_REG_20694(pi, RF, TX_CFG1_OVR, core, ovr_pa_idac_main, 0x1);
	      MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG2, core, pa_idac_main, 35);
	      MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG5, core, idac_pa_casref, 8);
	      MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_cascode, 14);

	    }
	  } else {
	    /* 5G front end */
	    FOREACH_CORE(pi, core) {
	      MOD_RADIO_REG_20694(pi, RF, PMU_CFG1, core, wlpmu_LOGENldo_hpm, 1);
	      MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_bbdc, 0xa);
	      MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG4, core, mx_idac_lobuf, 0x7);
	      MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_cascode, 24);

	      MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1_OVR, core, ovr_pad5g_idac_gm, 1);
	      MOD_RADIO_REG_20694(pi, RF, TXMIX5G_CFG8, core, pad5g_idac_gm, 20);
	      MOD_RADIO_REG_20694(pi, RF, TX5G_CFG3, core, pad5g_idac_cas_bot, 16);
	      MOD_RADIO_REG_20694(pi, RF, TX5G_CFG2, core, pad5g_ptat_slope_gm, 7);

	      MOD_RADIO_REG_20694(pi, RF, TX_CFG1_OVR, core, ovr_pa_idac_main, 1);
	      MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG2, core, pa_idac_main, 55);
	      MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG5, core, idac_pa_casref, 15);
	      MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1_OVR, core, ovr_bst5g_pu, 1);
	      MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1, core, bst5g_pu, 0);
	      MOD_RADIO_REG_20694(pi, RF, PA5G_CFG2, core, pa5g_idac_topc_op1, 0);
	      MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1_OVR, core, ovr_trsw5g_pu, 1);
	      MOD_RADIO_REG_20694(pi, RF, TRSW5G_CFG1, core, trsw5g_pu, 0);

	      MOD_RADIO_REG_20694(pi, RF, LOGEN5G_REG8, core, logen5g_buf_bias, 0x7);
	      if (core == 1) {
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG4, 1, afediv_bias_str, 0x1);
	      }
	    }
	  }
	} else if (sicoreunit == DUALMAC_AUX) {
		if (fc < 3000) {
			if (!(PHY_IPA(pi))) {
				FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_bbdc, 0xa);
				MOD_RADIO_REG_20694(pi, RF, TX2G_CFG1_OVR, core, ovr_bst2g_pu, 1);
				MOD_RADIO_REG_20694(pi, RF, TXMIX2G_CFG5, core, bst2g_pu, 0);
				MOD_RADIO_REG_20694(pi, RF, TX_CFG2_OVR, core, ovr_bstr_bias_pu, 1);
				MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG1, core, bstr_bias_pu, 0);
				MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG6, core,
					ibias_uncal_20uA_LDO_2P5V_mag, 0xb);
				MOD_RADIO_REG_20694(pi, RF, PA2G_CFG5, core,
					pa2g_stby_ldo_adj, 0x3);
				MOD_RADIO_REG_20694(pi, RF, PA2G_CFG1, core,
					pa2g_tssi_ctrl_sel, 0x0);

				//MOD_RADIO_REG_20694(pi, RF, TX_CFG1_OVR, core,
				//	ovr_pa_idac_main, 0x1);
				MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG2, core, pa_idac_main, 35);
				MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG5, core, idac_pa_casref, 8);
				MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_cascode, 14);
			    }
			} else if ((PHY_IPA(pi)) && (RADIOREV(pi->pubpi->radiorev) == 4)) {
			/* placeholder */
			}
		}  else {
		    /* 5G */
		    FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20694(pi, RF, PMU_CFG1, core, wlpmu_LOGENldo_hpm, 1);
			MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_bbdc, 0xa);
			MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG4, core, mx_idac_lobuf, 0x7);
			MOD_RADIO_REG_20694(pi, RF, TX_MX_CFG3, core, mx_idac_cascode, 24);

			MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1_OVR, core, ovr_pad5g_idac_gm, 1);
			MOD_RADIO_REG_20694(pi, RF, TXMIX5G_CFG8, core, pad5g_idac_gm, 20);
			MOD_RADIO_REG_20694(pi, RF, TX5G_CFG3, core, pad5g_idac_cas_bot, 16);
			MOD_RADIO_REG_20694(pi, RF, TX5G_CFG2, core, pad5g_ptat_slope_gm, 7);

			MOD_RADIO_REG_20694(pi, RF, TX_CFG1_OVR, core, ovr_pa_idac_main, 1);
			MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG2, core, pa_idac_main, 55);
			MOD_RADIO_REG_20694(pi, RF, TX_PA_CFG5, core, idac_pa_casref, 15);
			MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1_OVR, core, ovr_bst5g_pu, 1);
			MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1, core, bst5g_pu, 0);
			MOD_RADIO_REG_20694(pi, RF, PA5G_CFG2, core, pa5g_idac_topc_op1, 0);
			MOD_RADIO_REG_20694(pi, RF, TX5G_CFG1_OVR, core, ovr_trsw5g_pu, 1);
			MOD_RADIO_REG_20694(pi, RF, TRSW5G_CFG1, core, trsw5g_pu, 0);
		    }
		}
	}
}

static void
wlc_phy_radio20696_upd_band_related_reg(phy_info_t *pi)
{
	uint8 core;
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		MOD_RADIO_PLLREG_20696(pi, LOGEN_REG0, logen_div3en, 1);
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_div2_pu, 0x1);
			// new 2G optimised settings
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_g_passive_rc, 1);
			MOD_RADIO_REG_20696(pi, LPF_REG5, core, lpf_g_passive_rc, 700);
			MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 127);
		}
	} else {
		MOD_RADIO_PLLREG_20696(pi, LOGEN_REG0, logen_div3en, 0);
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_div2_pu, 0x0);
			// new 5G optimised settings
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG4, core, logen_tx_rccr_tune, 0);
			MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 127);
			MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_Rfb, 0);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core,
			    logen_5g_tx_rccr_iqbias_short, 1);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG2, core, logen_tx_rccr_idac26u, 7);
			// RX settings RCCR related 5G
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG4, core, logen_rx_rccr_tune, 0);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG2, core, logen_rx_rccr_idac26u, 7);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core,
			    logen_5g_rx_rccr_iqbias_short, 1);
		}
	}
}

static void
wlc_phy_switch_radio_acphy_20694(phy_info_t *pi)
{
	uint sicoreunit;
	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID));

	/* RCAL */
#ifdef ATE_BUILD
	wlc_phy_20694_radio_rcal(pi, 1);
	if (sicoreunit == DUALMAC_AUX) {
		wlc_phy_20694_radio_rcal_byaux(pi, 1);
	}

#else
	wlc_phy_20694_radio_rcal(pi, 1);
	if (sicoreunit == DUALMAC_AUX) {
		wlc_phy_20694_radio_rcal_byaux(pi, 1);
	}
#endif

	wlc_phy_20694_radio_minipmu_cal(pi);
	if (sicoreunit == DUALMAC_AUX) {
		wlc_phy_20694_radio_minipmu_cal_byaux(pi);
	}
}


static void
chanspec_tune_radio_20694(phy_info_t *pi)
{
	/* TODO: Add 20694 specific code to be added */
	uint8 core, dacbw;
	uint16 bbmult[2], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ? pi->dacratemode2g : pi->dacratemode5g;

	/* Configure ulp tx mode */
	//ulp dac mode is not supported for 4347 yet.
	//wlc_phy_dac_rate_mode_acphy(pi, pi_ac->dac_mode);

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}
		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}

	/* Configure DAC buffer BW */
	if (PHY_PAPDEN(pi)) {
		wlc_phy_radio20694_txdac_bw_setup(pi, BUTTERWORTH, DACBW_120);
	} else {
		dacbw = CHSPEC_IS20(pi->radio_chanspec) ? DACBW_30:
			CHSPEC_IS40(pi->radio_chanspec) ? DACBW_60:
			CHSPEC_IS80(pi->radio_chanspec) ? DACBW_120: DACBW_150;
		wlc_phy_radio20694_txdac_bw_setup(pi, 0, dacbw);
	}
}

static void
chanspec_tune_radio_20696(phy_info_t *pi)
{
	uint8 core, dacbw;
	uint16 bbmult[2], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ? pi->dacratemode2g : pi->dacratemode5g;

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}
		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}

	/* Configure DAC buffer BW */
	if (PHY_PAPDEN(pi)) {
		wlc_phy_radio20696_txdac_bw_setup(pi, BUTTERWORTH, DACBW_120);
	} else {
		dacbw = CHSPEC_IS20(pi->radio_chanspec) ? DACBW_30:
			CHSPEC_IS40(pi->radio_chanspec) ? DACBW_60:
			CHSPEC_IS80(pi->radio_chanspec) ? DACBW_120: DACBW_150;
		wlc_phy_radio20696_txdac_bw_setup(pi, 0, dacbw);
	}
}

void
wlc_phy_radio20694_afe_div_ratio(phy_info_t *pi, uint8 ulp_tx_mode, uint8 ulp_adc_mode)
{
	uint8 is_5g = CHSPEC_IS5G(pi->radio_chanspec);
	uint16 fc;
	uint8 Index_DA = 0;
	uint8 Index_AD = 0;
	uint16 AFEdiv_AD_ratio_list[9] = {12, 14, 16, 18, 24, 32, 48, 96, 1920};
	uint8  AFEdiv_AD_code[9] = {2, 3, 8, 9, 12, 24, 28, 44, 255};
	uint8  AFEdiv_DA_ratio_list[5] = {4, 6, 8, 12, 24};
	uint8  AFEdiv_DA_code[5] = {0, 1, 2, 3, 5};
	uint16 NAD;
	uint16 NDA;
	uint8 core, i;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	pi_ac->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ? pi->dacratemode2g : pi->dacratemode5g;

	/* Get center frequency of the channel */
	if (CHSPEC_CHANNEL(pi->radio_chanspec) > 14) {
		fc = CHAN5G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
	} else {
		fc = CHAN2G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
	}

	if (CHSPEC_IS160(pi->radio_chanspec)) {
		if (fc == 5815)
			NAD = 18;
		else
			NAD = 16;
		NDA = 4;
	} else if (CHSPEC_IS80(pi->radio_chanspec)) {
		if (fc == 5815)
			NAD = 18;
		else
			NAD = 16;
		NDA = 4;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		NAD = (12 << is_5g);
		NDA = (6 << is_5g);
	} else {
		NAD = (24 << is_5g);
		NDA = (12 << is_5g);
	}

	if ((pi_ac->dac_mode) == 2) {
		NAD = (4 << is_5g);
		NDA = (8 << is_5g);
	}

	for (i = 0; i < 5; i++)
	{
		if (NDA == AFEdiv_DA_ratio_list[i])
		{
			Index_DA = i;
			break;
		}
	}

	for (i = 0; i < 9; i++)
	{
		if (NAD == AFEdiv_AD_ratio_list[i])
		{
			Index_AD = i;
			break;
		}
	}

	FOREACH_CORE(pi, core) {
		if (core == 0) {
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afe_div_adc, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afe_div_dac, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG2, 0,
				afediv_div_dac, AFEdiv_AD_code[Index_AD]);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG3, 0,
				afediv_div_adc, AFEdiv_DA_code[Index_DA]);
		} else if (core == 1) {
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afe_div_adc, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afe_div_dac, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG2, 1,
				afediv_div_dac, AFEdiv_AD_code[Index_AD]);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG3, 1,
				afediv_div_adc, AFEdiv_DA_code[Index_DA]);

		}
	}

	if (fc < 3000) {
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_lo_sel, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_lo_sel, 0);
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_lo_sel, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 1, afediv_lo_sel, 0);
	} else {
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_lo_sel, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_lo_sel, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_lo_sel, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 1, afediv_lo_sel, 0x1);
	}

	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_pu, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_lo_sel, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_adc_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_dac_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_pu, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_rst_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_rsdb_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_arst, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_pu, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_lo_sel, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_adc_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_dac_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_pu, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_rst_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_rsdb_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_arst, 0x1);

	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_progdiv_en, 0x0);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG4, 0, afediv_pu, 0x1);
	MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 0, afediv_rsdb_en_spare, 0x0);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_rst_en, 0x0);
	MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 0, afediv_set_Ten_spare, 0x0);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 0, afediv_dac_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 0, afediv_adc_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 1, afediv_progdiv_en, 0x0);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG4, 1, afediv_pu, 0x1);
	MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 1, afediv_rsdb_en_spare, 0x0);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 1, afediv_rst_en, 0x0);
	MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 1, afediv_set_Ten_spare, 0x0);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 1, afediv_dac_en, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 1, afediv_adc_en, 0x1);

	//Trigger Areset
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_arst, 0x0);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_arst, 0x1);
	MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_arst, 0x0);

	FOREACH_CORE(pi, core) {
		if (core == 0) {
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_pu, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_lo_sel, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_adc_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_dac_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_pu, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_rst_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_rsdb_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 0, ovr_afediv_arst, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_progdiv_en, 0x0);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG4, 0, afediv_pu, 0x1);
			MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 0, afediv_rsdb_en_spare, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 0, afediv_rst_en, 0x0);
			MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 0, afediv_set_Ten_spare, 0x0);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 0, afediv_dac_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 0, afediv_adc_en, 0x1);
		} else if (core == 1) {
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_pu, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_lo_sel, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_adc_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_dac_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_pu, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_rst_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_rsdb_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, 1, ovr_afediv_arst, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 1, afediv_progdiv_en, 0x0);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG4, 1, afediv_pu, 0x1);
			MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 1, afediv_rsdb_en_spare, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG1, 1, afediv_rst_en, 0x0);
			MOD_RADIO_REG_20694(pi, RF, SPARE_CFG0, 1, afediv_set_Ten_spare, 0x0);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 1, afediv_dac_en, 0x1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, 1, afediv_adc_en, 0x1);
		}
	}
}

static void
wlc_phy_radio20694_rccal(phy_info_t *pi)
{

	uint8 cal, done, core;
	uint16 rccal_itr, n0, n1;
	/* lpf, dacbuf */
	uint8 sr[] = {0x1, 0x0};
	uint8 sc[] = {0x0, 0x1};
	uint8 X1[] = {0x1c, 0x40};
	uint16 rccal_trc_set[] = {0x22d, 0x10a};

	uint32 gmult_dacbuf_k = 77705;
	uint32 gmult_lpf_k = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = &(pi->u.pi_acphy->radioi->data);
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_REG_20694(pi, RFP, XTAL5, 0, xtal_pu_wlandig, 1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL5, 0, xtal_pu_RCCAL, 1);

	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < 2; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG1, 0, rccal_Q1, 0x1);
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG0, 0, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG0, 0, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG1, 0, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG2, 0, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG0, 0, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG0, 0, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG1, 0, rccal_START, 0);
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG1, 0, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
		     (rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
		     rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_20694(pi, RFP, RCCAL_CFG3, 0, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG1, 0, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		n0 = READ_RADIO_REGFLD_20694(pi, RFP, RCCAL_CFG4, 0, rccal_N0);
		n1 = READ_RADIO_REGFLD_20694(pi, RFP, RCCAL_CFG5, 0, rccal_N1);
		dn = n1 - n0; /* set dn [expr {$N1 - $N0}] */
		// dn = 1;  // JANATH added to avoid devide by zero error

		if (cal == 0) {
			/* lpf  values */
			data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
			data->rccal_gmult_rc = data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {
			/* dacbuf  */
			gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
			pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24) / (gmult_dacbuf);
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20694(pi, RFP, RCCAL_CFG0, 0, rccal_pu, 0);
	}
	/* Powerdown rccal driver */
	MOD_RADIO_REG_20694(pi, RFP, XTAL5, 0, xtal_pu_RCCAL, 0);

	/* nominal values when rc cal failes  */
	if (done == 0) {
		pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
		data->rccal_gmult_rc     = 4260;
	}

	/* Program the register */
	FOREACH_CORE(pi, core) {
		WRITE_RADIO_REG_20694(pi, RF, TIA_GMULT_BQ1, core, data->rccal_gmult_rc);
		WRITE_RADIO_REG_20694(pi, RF, LPF_GMULT_BQ2, core, data->rccal_gmult_rc);
		WRITE_RADIO_REG_20694(pi, RF, LPF_GMULT_RC, core, data->rccal_gmult_rc);
	}

}

static void
wlc_phy_radio20696_rccal(phy_info_t *pi)
{

	uint8 cal, done, core;
	uint16 rccal_itr;
	int16 n0, n1;
	/* lpf, dacbuf */
	uint8 sr[] = {0x1, 0x0};
	uint8 sc[] = {0x0, 0x1};
	uint8 X1[] = {0x1c, 0x40};
	uint16 rccal_trc_set[] = {0x22d, 0x10a};

	uint32 gmult_dacbuf_k = 77705;
	uint32 gmult_lpf_k = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = &(pi->u.pi_acphy->radioi->data);
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}

	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < 2; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20696(pi, RCCAL_CFG2, 1, rccal_Q1, 0x1);
		MOD_RADIO_REG_20696(pi, RCCAL_CFG0, 1, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20696(pi, RCCAL_CFG0, 1, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20696(pi, RCCAL_CFG2, 1, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20696(pi, RCCAL_CFG3, 1, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20696(pi, RCCAL_CFG0, 1, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20696(pi, RCCAL_CFG0, 1, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_20696(pi, RCCAL_CFG2, 1, rccal_START, 0);
		MOD_RADIO_REG_20696(pi, RCCAL_CFG2, 1, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
		     (rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
		     rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_20696(pi, RF, RCCAL_CFG4, 1, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_20696(pi, RCCAL_CFG2, 1, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		/* n0 and n1 are 12 bit signed */
		n0 = READ_RADIO_REGFLD_20696(pi, RF, RCCAL_CFG5, 1, rccal_N0);
		if (n0 & 0x1000)
			n0 = n0 - 0x2000;

		n1 = READ_RADIO_REGFLD_20696(pi, RF, RCCAL_CFG6, 1, rccal_N1);
		if (n1 & 0x1000)
			n1 = n1 - 0x2000;

		dn = n1 - n0;
		PHY_INFORM(("wl%d: %s n0 = %d\n", pi->sh->unit, __FUNCTION__, n0));
		PHY_INFORM(("wl%d: %s n1 = %d\n", pi->sh->unit, __FUNCTION__, n1));

		if (cal == 0) {
			/* lpf  values */
			data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
			data->rccal_gmult_rc = data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {
			/* dacbuf  */
			gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
			pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24) / (gmult_dacbuf);
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20696(pi, RCCAL_CFG0, 1, rccal_pu, 0);

	}
	/* nominal values when rc cal failes  */
	if (done == 0) {
		pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
		data->rccal_gmult_rc = 4260;
	}
	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20696(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, data->rccal_gmult_rc);
		MOD_RADIO_REG_20696(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, data->rccal_gmult_rc);
		MOD_RADIO_REG_20696(pi, LPF_GMULT_RC, core, lpf_gmult_rc, data->rccal_gmult_rc);
	}

}

static void
wlc_phy_radio20694_pwron_seq_phyregs(phy_info_t *pi)
{
	/* 20694 specific code  added */
	/* # power down everything */
	/* do not touch bg_pulse */

	uint8 core;

	FOREACH_CORE(pi, core) {
		WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, 0x0);
		WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, 0x3ff);
	}

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/* Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/* Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(100);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	// YOU ABSOLUTELY NEED TO UPDATE THE PREFERRED VALUES HERE.
	// RfctrlCmd wipes out a metric ton of RF registers. You gotta re-init.
	// Update preferred values

	wlc_phy_radio20694_upd_prfd_values(pi);

	wlc_phy_radio20694_low_current_mode(pi, 2);

	/* rfpll, pllldo, logen_pu, reset */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	WRITE_PHYREGCE(pi, RfctrlCoreTxPus, 0, 0x180);
	OSL_DELAY(100);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);

	FOREACH_CORE(pi, core) {
		WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, 0x80);
		WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, 0x180);
	}
}

/* Global Radio PU's */
static void wlc_phy_radio20696_pwron_seq_phyregs(phy_info_t *pi)
{
	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything	 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/*  Update preferred values */
	wlc_phy_radio20696_upd_prfd_values(pi);

	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
}


static void
wlc_phy_chanspec_radio20694_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset, uint8 core)
{

	const chan_info_radio20694_rffe_t *chan_info;

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID));

	if (wlc_phy_chan2freq_20694(pi, ch, &chan_info) < 0) {
		return;
	}

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, core);
	}
	PHY_TRACE(("wl%d: BEFORE wlc_phy_radio20694_pll_tune", pi->sh->unit));
	/* pll tuning */
	wlc_phy_radio20694_pll_tune(pi, chan_info->freq, core);
	PHY_TRACE(("wl%d: AFTER wlc_phy_radio20694_pll_tune", pi->sh->unit));
	/* rffe tuning */
	wlc_phy_radio20694_rffe_tune(pi, chan_info, core);
	wlc_phy_20694_radio_vcocal(pi, 0, 1);

	if (core == 0) {
		/* disable BG bias filter */
		MOD_RADIO_REG_20694(pi, RFP, BG_REG2, core, bg_pulse, 1);
		MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, core, ovr_bg_pulse, 1);
	}

	/* Load band related regs */
	wlc_phy_radio20694_upd_band_related_reg(pi);
}


static void
wlc_phy_20694_radio_rcal(phy_info_t *pi, uint8 mode)
{
	/* Format: 20694_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* The rcalcode argument works only with mode 1 and is optional. */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_value; // rcal_valid, loop_iter, rcal_value;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID));

	MOD_RADIO_REG_20694(pi, RFP, XTAL5, 0, xtal_pu_wlandig, 1);

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0, ovr_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		rcal_value = 0x33;
		MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0,
			ovr_otp_rcal_sel, 0);
		MOD_RADIO_REG_20694(pi, RFP, BG_REG8, 0,
			bg_rcal_trim, rcal_value);
		MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0,
			ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		/* add code here */
	}

}

static void
wlc_phy_20694_radio_rcal_byaux(phy_info_t *pi, uint8 mode)
{
	/* Format: 20694_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* The rcalcode argument works only with mode 1 and is optional. */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */

	uint8 rcal_value;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID));

	MOD_RADIO_REG_20694(pi, RFP, XTAL5, 0, xtal_pu_wlandig, 1);

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0, ovr_mainauxshared_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		rcal_value = 0x33;
		MOD_RADIO_REG_20694(pi, RFP, BG_REG1_MAINAUX_SHRD, 0,
			shrd_bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0, ovr_mainauxshared_otp_rcal_sel, 0x0);
		MOD_RADIO_REG_20694(pi, RFP, BG_REG2_MAINAUX_SHRD, 0,
			shrd_bg_rcal_trim, rcal_value);
		MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0, ovr_mainauxshared_bg_rcal_trim, 0x1);
	} else if (mode == 2) {
		/* add code here */

	}
}

/*  R Calibration (takes ~50us) */
/* copied from IGUANA and adabted for 7271 */
/* original code in wlc_phy_28nm_radio_rcal */
/* modified were needed from 20696_procs.tcl */
static void
wlc_phy_radio20696_r_cal(phy_info_t *pi, uint8 mode)
{
	/* Format: 20696_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_valid, loop_iter, rcal_value;
	uint8 core = 1;

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		/* value taken from 20696_procs.tcl */
		rcal_value = 0x32;
		MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
		MOD_RADIO_REG_20696(pi, BG_REG8, core, bg_rcal_trim, rcal_value);
		MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
		MOD_RADIO_REG_20696(pi, BG_REG1, core, bg_rtl_rcal_trim, rcal_value);
	} else if (mode == 2) {
		/* This should run only for core 1 */
		/* Run R Cal and use its output */
		MOD_RADIO_REG_20696(pi, GPAIO_SEL2, core, gpaio_pu, 0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL6, core, gpaio_top_pu, 0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL8, core, gpaio_rcal_pu, 0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL0, core, gpaio_sel_0to15_port, 0x0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL1, core, gpaio_sel_16to31_port, 0x0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL6, core, gpaio_top_pu, 1);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL8, core, gpaio_rcal_pu, 1);
		MOD_RADIO_REG_20696(pi, BG_REG3, core, bg_rcal_pu, 1);
		MOD_RADIO_REG_20696(pi, RCAL_CFG_NORTH, core, rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			/* 1 ms delay wait time */
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_REGFLD_20696(pi, RF, RCAL_CFG_NORTH,
				core, rcal_valid);
			loop_iter++;
		}

		MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
		MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_REGFLD_20696(pi, RF, RCAL_CFG_NORTH, core,
				rcal_value);

			/* Very coarse sanity check */
			if ((rcal_value < 2) || (60 < rcal_value)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				" 6bit Rcal = %d.\n", rcal_value));
				rcal_value = 0x32;
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
				__FUNCTION__, rcal_valid));
			/* take some sane default value */
			rcal_value = 0x32;
		}
		MOD_RADIO_REG_20696(pi, BG_REG1, core, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_REG_20696(pi, BG_REG8, core, bg_rcal_trim, rcal_value);

#ifdef ATE_BUILD
		ate_buffer_regval[0].rcal_value[0] = rcal_value;
#endif

		/* Power down blocks not needed anymore */
		MOD_RADIO_REG_20696(pi, RCAL_CFG_NORTH, core, rcal_pu, 0);
	}
}

void
wlc_phy_radio20694_txdac_bw_setup(phy_info_t *pi, uint8 filter_type, uint8 dacbw)
{
	uint16 code_cfb = 0x50;
	uint32 dacbuf_cmult = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;
	uint8 cfb_default_code[4] = {0x50, 0x3c, 0x30, 0x1c};
	uint8 core;

	ASSERT(dacbw != 0);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG1, core, iqdac_buf_op_cur, 0x0);

		if ((dacbw == DACBW_120) || (dacbw == DACBW_150)) {
			MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_buf_bw, 0x0);
		} else if ((dacbw == DACBW_60) || (dacbw == DACBW_30)) {
			MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_buf_bw, 0x4);
		}

		if (dacbw == DACBW_30) {
			code_cfb = cfb_default_code[0];
		} else if (dacbw == DACBW_60) {
			code_cfb = cfb_default_code[1];
		} else if (dacbw == DACBW_120) {
			code_cfb = cfb_default_code[2];
		} else {
			code_cfb = cfb_default_code[3];
		}

		code_cfb = (dacbuf_cmult * code_cfb) >> 12;
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG1, core, iqdac_buf_Cfb, code_cfb);
	}
}

void
wlc_phy_radio20696_txdac_bw_setup(phy_info_t *pi, uint8 filter_type, uint8 dacbw)
{
	uint16 code_cfb = 0x50;
	uint32 dacbuf_cmult = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;
	uint8 cfb_default_code[4] = {0x50, 0x3c, 0x30, 0x1c};
	uint8 core;

	ASSERT(dacbw != 0);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_op_cur, 0x0);

		if ((dacbw == DACBW_120) || (dacbw == DACBW_150)) {
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_bw, 0x0);
		} else if ((dacbw == DACBW_60) || (dacbw == DACBW_30)) {
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_bw, 0x4);
		}

		if (dacbw == DACBW_30) {
			code_cfb = cfb_default_code[0];
		} else if (dacbw == DACBW_60) {
			code_cfb = cfb_default_code[1];
		} else if (dacbw == DACBW_120) {
			code_cfb = cfb_default_code[2];
		} else {
			code_cfb = cfb_default_code[3];
		}

		code_cfb = (dacbuf_cmult * code_cfb) >> 12;
		MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_Cfb, code_cfb);
	}
}

int8
wlc_phy_20694_radio_minipmu_cal(phy_info_t *pi)
{
	uint8 calsuccesful = 1;
	int8 cal_status = 1;
	int16 waitcounter = 0;
	uint16 temp_code;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID));

	/* Setup the cal */
	MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0, ovr_bg_refadj_cbuck, 0x1);
	MOD_RADIO_REG_20694(pi, RFP, XTAL_OVR1, 0, ovr_xtal_refadj_cbuck, 0x1);

	WRITE_RADIO_REG_20694(pi, RFP, BG_REG10, 0, 0x3f);
	MOD_RADIO_REG_20694(pi, RFP, BG_OVR1, 0, ovr_pmu_bg_wlpmu_en_i, 0x1);

	/* 25 us delay to allow PMU cal to happen */
	OSL_DELAY(25);

	while (READ_RADIO_REGFLD_20694(pi, RFP, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		/* 10 us delay wait time */
	        OSL_DELAY(100);
		waitcounter++;

		if (waitcounter > 100) {
			PHY_TRACE(("\nWarning:Mini PMU Cal Failed on Core0"));
			calsuccesful = 0;
			break;
		}
	}

	/* Cleanup */

	WRITE_RADIO_REG_20694(pi, RFP, BG_REG10, 0, 0x6c);
	MOD_RADIO_REG_20694(pi, RF, PMU_CFG3, 0, vref_select, 0x1);
	MOD_RADIO_REG_20694(pi, RF, PMU_CFG3, 1, vref_select, 0x1);

	if (calsuccesful == 1) {
		PHY_TRACE(("MiniPMU cal done on core0, Cal code = %d",
			READ_RADIO_REGFLD_20694(pi, RFP, BG_REG11, 0, bg_wlpmu_calcode)));
		temp_code = READ_RADIO_REGFLD_20694(pi, RFP, BG_REG11, 0, bg_wlpmu_calcode);
		MOD_RADIO_REG_20694(pi, RFP, BG_REG9, 0, bg_wlpmu_cal_mancodes, temp_code);
		cal_status = 1;
	} else {
		PHY_TRACE(("Cal Unsuccesful on Core1"));
		cal_status = -1;
	}

	return cal_status;
}

int8
wlc_phy_20694_radio_minipmu_cal_byaux(phy_info_t *pi)
{
	uint8 calsuccesful = 1;
	int8 cal_status = 1;
	int16 waitcounter = 0;
	uint16 temp_code;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID));

	ACPHY_REG_LIST_START
		/* Setup the cal */
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_OVR1, 0, ovr_bg_refadj_cbuck, 0x1)

		/* to set correct LDO voltage */
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG9, 0, bg_wlpmu_vrefadj_cbuck, 0x4)
		MOD_RADIO_REG_20694_ENTRY(pi, RF, PMU_REG2_MAINAUX_SHRD,
			0, shrd_wlpmu_LOGENldo_adj, 0x5)
		MOD_RADIO_REG_20694_ENTRY(pi, RF, PMU_REG2_MAINAUX_SHRD,
			0, shrd_wlpmu_TXldo_adj, 0x7)

		//set 1p8V LDO ratio
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, WLLDO1P8_OVR, 0,
			ovr_ldo1p8_refadj_cbuck, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, WLLDO1P8_OVR_SHRD,
			0, ovr_mainauxshared_ldo1p8_pu, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, PMU_REG3_MAINAUX_SHRD,
			0, shrd_ldo1p8_refadj_cbuck, 0x4)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, PMU_REG3_MAINAUX_SHRD,
			0, shrd_i_LDO1p8_0p9refadj_1p1, 0x1)

		MOD_RADIO_REG_20694_ENTRY(pi, RFP, XTAL_OVR1, 0, ovr_xtal_refadj_cbuck, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, XTAL1, 0, vref_adj_cbuck, 0x4)

		//set 1p6V LDO ratio
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_bypcal, 0x0)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_en_i, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_pu_cbuck_ref, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_pu_shrd_bg_ref, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_startcal, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* 25 us delay to allow PMU cal to happen */
	OSL_DELAY(25);

	while (READ_RADIO_REGFLD_20694(pi, RFP, BG_REG4_MAINAUX_SHRD, 0,
		shrd_bg_wlpmu_cal_done) == 0) {

		/* 10 us delay wait time */
		OSL_DELAY(10);
		waitcounter++;

		if (waitcounter > 100) {
			PHY_TRACE(("\nWarning:Mini PMU Cal Failed on Core0"));
			calsuccesful = 0;
			break;
		}
	}

	/* Cleanup */
	ACPHY_REG_LIST_START
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_bypcal, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_en_i, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_pu_cbuck_ref, 0x0)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_pu_shrd_bg_ref, 0x1)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_clk10M_en, 0x0)
		MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG4_MAINAUX_SHRD,
			0, shrd_bg_wlpmu_startcal, 0x0)

		MOD_RADIO_REG_20694_ENTRY(pi, RF, PMU_REG1_MAINAUX_SHRD,
			0, shrd_vref_select, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (calsuccesful == 1) {
		PHY_TRACE(("MiniPMU cal done on core0, Cal code = %d",
			READ_RADIO_REGFLD_20694(pi, RFP, BG_REG4_MAINAUX_SHRD,
				0, shrd_bg_wlpmu_calcode)));
		temp_code = READ_RADIO_REGFLD_20694(pi, RFP, BG_REG4_MAINAUX_SHRD,
				0, shrd_bg_wlpmu_calcode);
		MOD_RADIO_REG_20694(pi, RFP, BG_REG3_MAINAUX_SHRD, 0,
			shrd_bg_wlpmu_cal_mancodes, temp_code);
		cal_status = 1;
	} else {
		PHY_TRACE(("Cal Unsuccesful on Core1"));
		cal_status = -1;
	}

	return cal_status;
}

/* Perform MINI PMU CAL */
static void wlc_phy_radio20696_minipmu_cal(phy_info_t *pi)
{
	uint8 waitcounter;
	uint8 core;
	bool calsuccesful;

	WRITE_RADIO_REG_20696(pi, BG_REG10, 1, 0x37); /* to enable Cbuck (vrefsel) */
	OSL_DELAY(25);

	/* Wait for cal_done	 */
	calsuccesful = TRUE;
	waitcounter = 0;
	while (READ_RADIO_REGFLD_20696(pi, RF, BG_REG11, 1, bg_wlpmu_cal_done) == 0) {
		OSL_DELAY(100);
		waitcounter++;
		if (waitcounter > 100) {
			/* This means the cal_done bit is not 1 even after waiting a while */
			/* Exit gracefully */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n",
				pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
	}
	/* Cleanup : Setting the caldone bit to 0 here */
	WRITE_RADIO_REG_20696(pi, BG_REG10, 1, 0x2C);
	MOD_RADIO_REG_20696(pi, BG_REG10, 1, bg_wlpmu_bypcal, 0x1);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20696(pi, PMU_CFG3, core, vref_select, 0x1);
	}
	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_REGFLD_20696(pi, RF, BG_REG11, 1, bg_wlpmu_calcode);
		PHY_INFORM(("wl%d: %s MiniPMU done. Cal Code = %d\n", pi->sh->unit,
				__FUNCTION__, tempcalcode));
		MOD_RADIO_REG_20696(pi, BG_REG9, 1, bg_wlpmu_cal_mancodes, tempcalcode);
	}
}

/* 20693 radio related functions */
int
wlc_phy_chan2freq_20693(phy_info_t *pi, uint8 channel,
	const chan_info_radio20693_pll_t **chan_info_pll,
	const chan_info_radio20693_rffe_t **chan_info_rffe,
	const chan_info_radio20693_pll_wave2_t **chan_info_pll_wave2)
{
	uint i;
	uint16 freq;
	const chan_info_radio20693_pll_t *chan_info_tbl_pll = NULL;
	const chan_info_radio20693_rffe_t *chan_info_tbl_rffe = NULL;
	const chan_info_radio20693_pll_wave2_t *chan_info_tbl_pll_wave2_part1 = NULL;
	const chan_info_radio20693_pll_wave2_t *chan_info_tbl_pll_wave2_part2 = NULL;
	uint32 tbl_len, tbl_len1 = 0;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Choose the right table to use */
	switch (RADIO20693REV(pi->pubpi->radiorev)) {
	case 3:
	case 4:
	case 5:
		chan_info_tbl_pll = chan_tuning_20693_rev5_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev5_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev5_pll);
		break;
	case 6:
		chan_info_tbl_pll = chan_tuning_20693_rev6_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev6_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev6_pll);
		break;
	case 7:
		chan_info_tbl_pll = chan_tuning_20693_rev5_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev5_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev5_pll);
		break;
	case 8:
		chan_info_tbl_pll = chan_tuning_20693_rev6_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev6_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev6_pll);
		break;
	case 10:
		chan_info_tbl_pll = chan_tuning_20693_rev10_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev10_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev10_pll);
		break;
	case 11:
	case 12:
	case 13:
		chan_info_tbl_pll = chan_tuning_20693_rev13_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev13_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev13_pll);
		break;
	case 14:
	case 19:
		chan_info_tbl_pll = chan_tuning_20693_rev14_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev14_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev14_pll);
		break;
	case 15:
		if (PHY_XTAL_IS37M4(pi)) {
			chan_info_tbl_pll = chan_tuning_20693_rev15_pll_37p4MHz;
			chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_37p4MHz;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_37p4MHz);
		} else {
			chan_info_tbl_pll = chan_tuning_20693_rev15_pll_40MHz;
			chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_40MHz;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_40MHz);
		}
		break;
	case 16:
	case 17:
	case 20:
		chan_info_tbl_pll = chan_tuning_20693_rev5_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev5_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev5_pll);
		break;
	case 18:
	case 21:
		if (PHY_XTAL_IS37M4(pi)) {
			chan_info_tbl_pll = chan_tuning_20693_rev18_pll;
			chan_info_tbl_rffe = chan_tuning_20693_rev18_rffe;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev18_pll);
		} else {
			chan_info_tbl_pll = chan_tuning_20693_rev15_pll_40MHz;
			chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_40MHz;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_40MHz);
		}
		break;
	case 23:
		/* Preferred values for 53573/53574/47189-B0 */
		chan_info_tbl_pll = chan_tuning_20693_rev15_pll_40MHz;
		chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_40MHz;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_40MHz);
		break;
	case 32:
		chan_info_tbl_pll  = NULL;
		chan_info_tbl_rffe = NULL;
		chan_info_tbl_pll_wave2_part1 = chan_tuning_20693_rev32_pll_part1;
		chan_info_tbl_pll_wave2_part2 = chan_tuning_20693_rev32_pll_part2;
		tbl_len1 = ARRAYSIZE(chan_tuning_20693_rev32_pll_part1);
		tbl_len = tbl_len1 + ARRAYSIZE(chan_tuning_20693_rev32_pll_part2);
		break;
	case 33:
		chan_info_tbl_pll  = NULL;
		chan_info_tbl_rffe = NULL;
		chan_info_tbl_pll_wave2_part1 = chan_tuning_20693_rev33_pll_part1;
		chan_info_tbl_pll_wave2_part2 = chan_tuning_20693_rev33_pll_part2;
		tbl_len1 = ARRAYSIZE(chan_tuning_20693_rev32_pll_part1);
		tbl_len = tbl_len1 + ARRAYSIZE(chan_tuning_20693_rev32_pll_part2);
		break;
	case 9:
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO20693REV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return -1;
	}

	if (RADIOMAJORREV(pi) == 3) {
		for (i = 0; i < tbl_len; i++) {
			if (i >= tbl_len1) {
				if (chan_info_tbl_pll_wave2_part2[i-tbl_len1].chan == channel) {
					*chan_info_pll_wave2 =
							&chan_info_tbl_pll_wave2_part2[i-tbl_len1];
					freq = chan_info_tbl_pll_wave2_part2[i-tbl_len1].freq;
					break;
				}
			} else {
				if (chan_info_tbl_pll_wave2_part1[i].chan == channel) {
					*chan_info_pll_wave2 = &chan_info_tbl_pll_wave2_part1[i];
					freq = chan_info_tbl_pll_wave2_part1[i].freq;
					break;
				}
			}
		}

		if (i >= tbl_len) {
			PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
				pi->sh->unit, __FUNCTION__, channel));
			if (!ISSIM_ENAB(pi->sh->sih)) {
				/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(i < tbl_len);
			}
			return -1;
		}
	} else {
		for (i = 0; i < tbl_len && chan_info_tbl_pll[i].chan != channel; i++);

		if (i >= tbl_len) {
			PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
				pi->sh->unit, __FUNCTION__, channel));
			if (!ISSIM_ENAB(pi->sh->sih)) {
				/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(i < tbl_len);
			}
			return -1;
		}

		*chan_info_pll = &chan_info_tbl_pll[i];
		*chan_info_rffe = &chan_info_tbl_rffe[i];
		freq = chan_info_tbl_pll[i].freq;
	}

	return freq;
}

void
phy_ac_radio_20693_chanspec_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset,
	uint8 pllcore, uint8 mode)
{
	const chan_info_radio20693_pll_t *chan_info_pll;
	const chan_info_radio20693_rffe_t *chan_info_rffe;
	const chan_info_radio20693_pll_wave2_t *chan_info_pll_wave2;
	int fc[NUM_CHANS_IN_CHAN_BONDING];
	uint8 core, start_pllcore = 0, end_pllcore = 0;
	uint8 chans[NUM_CHANS_IN_CHAN_BONDING] = {0, 0};

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* mode = 0, setting per pllcore input
	 * mode = 1, setting both pllcores=0/1 at once
	 */
	if (mode == 0) {
		start_pllcore = pllcore;
		end_pllcore = pllcore;
		core = pllcore;
	} else if (mode == 1) {
		start_pllcore = 0;
		end_pllcore = 1;
	} else {
		ASSERT(0);
	}

	if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
		ASSERT(start_pllcore != end_pllcore);
		wf_chspec_get_80p80_channels(pi->radio_chanspec, chans);
	} else {
		chans[pllcore] = ch;
	}

	for (core = start_pllcore; core <= end_pllcore; core++) {
		fc[core] = wlc_phy_chan2freq_20693(pi, chans[core], &chan_info_pll,
				&chan_info_rffe, &chan_info_pll_wave2);

		if (fc[core] < 0)
			return;

		/* logen_reset needs to be toggled whenever bandsel bit if changed */
		/* On a bw change, phy_reset is issued which causes currentBand getting */
		/* reset to 0 */
		/* So, issue this on both band & bw change */
		if (toggle_logen_reset == 1) {
			wlc_phy_logen_reset(pi, core);
		}

		if (RADIOMAJORREV(pi) != 3) {
			/* pll tuning */
			wlc_phy_radio20693_pll_tune(pi, chan_info_pll, core);

			/* rffe tuning */
			wlc_phy_radio20693_rffe_tune(pi, chan_info_rffe, core);
		} else {
			/* pll tuning */
			wlc_phy_radio20693_pll_tune_wave2(pi, chan_info_pll_wave2, core);
			wlc_phy_radio20693_sel_logen_mode(pi);
		}
	}

	if ((RADIOMAJORREV(pi) == 3)) {
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1, ldo_1p2_xtalldo1p2_ctl, 0xe);
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_LVLDO1, ldo_1p2_lowquiescenten_PFDMMD, 1);
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_LVLDO1, ldo_1p2_ldo_PFDMMD_vout_sel, 0xc);
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_LVLDO2, ldo_1p2_ldo_VCOBUF_vout_sel, 0xc);
		MOD_RADIO_ALLREG_20693(pi, TX_DAC_CFG1, DAC_scram_off, 1);

		if (pi->sromi->sr13_1p5v_cbuck) {
			MOD_RADIO_ALLREG_20693(pi, PMU_CFG1, wlpmu_vrefadj_cbuck, 0x5);
			PHY_TRACE(("wl%d: %s: setting wlpmu_vrefadj_cbuck=0x5",
				pi->sh->unit, __FUNCTION__));
		}

		if (CHSPEC_IS20(pi->radio_chanspec) && !PHY_AS_80P80(pi, pi->radio_chanspec)) {
			if (fc[pllcore] <= 2472) {
				if (fc[pllcore] == 2427) {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
					ldo_1p2_xtalldo1p2_ctl, 0xe);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
				} else {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
					ldo_1p2_xtalldo1p2_ctl, 0xa);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
				}
				if ((pi->sromi->sr13_cck_spur_en == 1)) {
					/* 2G cck spur reduction setting for 4366, core0 */
					MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG4, pllcore,
						wl_xtal_wlanrf_ctrl, 0x3);
					PHY_TRACE(("wl%d: %s, fc: %d, 2g cck spur reduction\n",
						pi->sh->unit, __FUNCTION__, fc[pllcore]));
				}
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG4,
					wl_xtal_wlanrf_ctrl, 0x3);
			} else if (fc[pllcore] >= 5180 && fc[pllcore] <= 5320) {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xa);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x8);
			} else if (fc[pllcore] >= 5745 && fc[pllcore] <= 5825) {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xa);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x8);
			} else {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xe);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
			}
		} else {
			MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1, ldo_1p2_xtalldo1p2_ctl, 0xe);
			MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
			MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
		}
	}

	if (RADIOMAJORREV(pi) == 3) {
		uint16 phymode = phy_get_phymode(pi);
		if (phymode == PHYMODE_3x3_1x1) {
			if (pllcore == 1) {
				wlc_phy_radio_tiny_vcocal_wave2(pi, 0, 4, 1, FALSE);
			}
		} else {
			if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
				wlc_phy_radio_tiny_vcocal_wave2(pi, 0, 2, 1, TRUE);
			} else if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0);
			} else {
				wlc_phy_radio_tiny_vcocal_wave2(pi, 0, 1, 1, TRUE);
			}
		}
	} else if (!((phy_get_phymode(pi) == PHYMODE_MIMO) && (pllcore == 1))) {
	/* Do a VCO cal after writing the tuning table regs */
	/* in mimo mode, vco cal needs to be done only for core 0 */
		wlc_phy_radio_tiny_vcocal(pi);
	}
}

static void
wlc_phy_radio20693_afeclkpath_setup(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode, uint8 direct_ctrl_en)
{
	uint8 pupd;
	uint16 mac_mode;
	pupd = (direct_ctrl_en == 1) ? 0 : 1;
	mac_mode = phy_get_phymode(pi);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (direct_ctrl_en == 0) {
		MOD_RADIO_REG_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_12g_buf_pu, pupd);
		MOD_RADIO_REG_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_buf_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_5g_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_5g_mimosrc_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_5g_mimodes_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_2g_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_2g_mimosrc_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_2g_mimodes_pu, pupd);
		MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_mimo_div2_pu, pupd);
		MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_6g_mimo_pu, pupd);

		/* set logen mimo src des pu */
		wlc_phy_radio20693_set_logen_mimosrcdes_pu(pi, core, adc_mode, mac_mode);
	}

	/* power up or down logen core based on band */
	wlc_phy_radio20693_set_logencore_pu(pi, adc_mode, core);
	/* Inside pll vco cell bufs 12G logen outputs at VCO freq */
	wlc_phy_radio20693_set_rfpll_vco_12g_buf_pu(pi, adc_mode, core);
	/* powerup afe clk div 12g sel */
	wlc_phy_radio20693_afeclkdiv_12g_sel(pi, adc_mode, core);
	/* power up afe clkdiv mimo blocks */
	wlc_phy_radio20693_afe_clk_div_mimoblk_pu(pi, core, adc_mode);
}

static void
wlc_phy_radio20693_set_logen_mimosrcdes_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode, uint16 mac_mode)
{
	uint8 pupd;
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	pupd = (mac_mode == PHYMODE_MIMO) ? 1 : 0;

	/* band a */
	if (CHSPEC_IS5G(pi->radio_chanspec)) {
		if (core == 0) {
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimosrc_pu, pupd);
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimodes_pu, 0);
		} else {
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimosrc_pu, 0);
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimodes_pu, pupd);
		}

		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_2g_mimosrc_pu, 0);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_2g_mimodes_pu, 0);
	} else {
		/* band g */
		if (adc_mode == RADIO_20693_SLOW_ADC)  {
			if (core == 0) {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, pupd);
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimodes_pu, 0);
			} else {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, 0);
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimodes_pu, pupd);
			}
		} else {
			if (core == 0) {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, pupd);
			} else {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, 0);
			}

			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
				logencore_2g_mimodes_pu, 0);
		}

		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimosrc_pu, 0);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimodes_pu, 0);
	}
}

static void
wlc_phy_radio20693_adc_powerupdown(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 pupdbit, uint8 core)
{
	uint8 en_ovrride;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (pupdbit == 1) {
	    /* use direct control */
		en_ovrride = 0;
	} else {
	    /* to force powerdown */
		en_ovrride = 1;
	}
	FOREACH_CORE(pi, core) {
		switch (adc_mode) {
		case RADIO_20693_FAST_ADC:
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_clk_fast_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG15, core, adc_clk_fast_pu, pupdbit);
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_fast_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG1, core, adc_fast_pu, pupdbit);
			break;
		case RADIO_20693_SLOW_ADC:
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_clk_slow_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG15, core, adc_clk_slow_pu, pupdbit);
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_slow_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG1, core, adc_slow_pu, pupdbit);
			break;
	    default:
	        PHY_ERROR(("wl%d: %s: Wrong ADC mode \n",
	           pi->sh->unit, __FUNCTION__));
			ASSERT(0);
	        break;
	    }
	    MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_rst_clk, 0x0);
	}
}

static void
wlc_phy_radio20693_set_logencore_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	uint8 pupd = 0;
	bool mimo_core_idx_1;
	mimo_core_idx_1 = ((phy_get_phymode(pi) == PHYMODE_MIMO) && (core == 1)) ? 1 : 0;
	/* Based on the adcmode and band set the logencore_pu for 2G and 5G */
	if ((mimo_core_idx_1 == 1) &&
		(CHSPEC_IS2G(pi->radio_chanspec)) && (adc_mode == RADIO_20693_FAST_ADC)) {
		pupd = 1;
	} else if (mimo_core_idx_1 == 1) {
		pupd = 0;
	} else {
		pupd = 1;
	}

	MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_5g_pu, 1);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_2g_pu, 1);

	if (CHSPEC_IS5G(pi->radio_chanspec)) {
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_5g_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_2g_pu, 0);
	} else {
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_5g_pu, 0);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_2g_pu, pupd);
	}
}

static void
wlc_phy_radio20693_set_rfpll_vco_12g_buf_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	uint8 pupd = 0;
	bool mimo_core_idx_1;
	bool is_per_core_phy_bw80;
	mimo_core_idx_1 = ((phy_get_phymode(pi) == PHYMODE_MIMO) && (core == 1)) ? 1 : 0;
	is_per_core_phy_bw80 = (CHSPEC_IS80(pi->radio_chanspec) ||
		CHSPEC_IS8080(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec));
	/* Based on the adcmode and band set the logencore_pu for 2G and 5G */
	if (mimo_core_idx_1 == 1) {
		pupd = 0;
	} else {
		pupd = 1;
	}
	if ((adc_mode == RADIO_20693_FAST_ADC) || (is_per_core_phy_bw80 == 1) ||
		(CHSPEC_IS5G(pi->radio_chanspec))) {
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_12g_buf_pu, pupd);
	} else {
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_12g_buf_pu, 0);
	}
	/* Feeds MMD and external clocks */
	MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_buf_pu, pupd);
}

static void
wlc_phy_radio20693_afeclkdiv_12g_sel(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	bool is_per_core_phy_bw80;
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int row;
	uint8 afeclkdiv;
	is_per_core_phy_bw80 = (CHSPEC_IS80(pi->radio_chanspec) ||
		CHSPEC_IS8080(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	}
	row = wlc_phy_radio20693_altclkpln_get_chan_row(pi);
	if ((adc_mode == RADIO_20693_FAST_ADC) || (is_per_core_phy_bw80 == 1)) {
		/* 80Mhz  : afe_div = 3 */
		afeclkdiv = 0x4;
	} else {
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			/* 20Mhz  : afe_div = 8 */
			afeclkdiv = 0x3;
		} else {
			/* 40Mhz  : afe_div = 4 */
			afeclkdiv = 0x2;
		}
	}
	if ((row >= 0) && (adc_mode != RADIO_20693_FAST_ADC)) {
		afeclkdiv = altclkpln[row].afeclkdiv;
	}
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, afe_clk_div_12g_sel, afeclkdiv);
}

static void
wlc_phy_radio20693_afe_clk_div_mimoblk_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode) {

	uint8 pupd = 0;
	bool mimo_core_idx_1;
	bool is_per_core_phy_bw80;
	mimo_core_idx_1 = ((phy_get_phymode(pi) == PHYMODE_MIMO) && (core == 1)) ? 1 : 0;
	is_per_core_phy_bw80 = (CHSPEC_IS80(pi->radio_chanspec) ||
		CHSPEC_IS8080(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec));

	if (mimo_core_idx_1 == 1) {
		pupd = 1;
	} else {
		pupd = 0;
	}
	/* Enable the radio Ovrs */
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_6g_mimo_pu, 1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_mimo_div2_pu, 1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_mimo_pu, 1);
	if (CHSPEC_IS2G(pi->radio_chanspec))  {
		if (adc_mode == RADIO_20693_SLOW_ADC) {
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_pu, 0);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_div2_pu, 0);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_6g_mimo_pu, pupd);
		} else {
			if (wlapi_txbf_enab(pi->sh->physhim)) {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
					core, afe_clk_div_12g_mimo_pu, 0);
			} else {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
					core, afe_clk_div_12g_mimo_pu, pupd);
			}
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_div2_pu, pupd);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_6g_mimo_pu, 0);
		}
	} else if (CHSPEC_IS5G(pi->radio_chanspec))  {
		if ((adc_mode == RADIO_20693_SLOW_ADC) &&
			((is_per_core_phy_bw80 == 0))) {
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_pu, 0);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core,
				afe_clk_div_12g_mimo_div2_pu, pupd);
		} else {
			if (wlapi_txbf_enab(pi->sh->physhim)) {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
					core, afe_clk_div_12g_mimo_pu, 0);
			} else {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core,
					afe_clk_div_12g_mimo_pu, pupd);
			}
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core,
				afe_clk_div_12g_mimo_div2_pu, 0);
			MOD_RADIO_REG_20693(pi, SPARE_CFG8, core,
				afe_BF_se_enb0, 1);
			MOD_RADIO_REG_20693(pi, SPARE_CFG8, core,
				afe_BF_se_enb1, 1);
		}
		MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, afe_clk_div_6g_mimo_pu, 0);
	}
}

static void wlc_phy_radio20693_pll_tune(phy_info_t *pi, const void *chan_info_pll,
	uint8 core)
{
	const uint16 *val_ptr;
	uint16 *off_ptr;
	uint8 ct;
	const chan_info_radio20693_pll_t *ci20693_pll = chan_info_pll;

	uint16 pll_tune_reg_offsets_20693[] = {
		RADIO_REG_20693(pi, PLL_VCOCAL1, core),
		RADIO_REG_20693(pi, PLL_VCOCAL11, core),
		RADIO_REG_20693(pi, PLL_VCOCAL12, core),
		RADIO_REG_20693(pi, PLL_FRCT2, core),
		RADIO_REG_20693(pi, PLL_FRCT3, core),
		RADIO_REG_20693(pi, PLL_HVLDO1, core),
		RADIO_REG_20693(pi, PLL_LF4, core),
		RADIO_REG_20693(pi, PLL_LF5, core),
		RADIO_REG_20693(pi, PLL_LF7, core),
		RADIO_REG_20693(pi, PLL_LF2, core),
		RADIO_REG_20693(pi, PLL_LF3, core),
		RADIO_REG_20693(pi, SPARE_CFG1, core),
		RADIO_REG_20693(pi, SPARE_CFG14, core),
		RADIO_REG_20693(pi, SPARE_CFG13, core),
		RADIO_REG_20693(pi, TXMIX2G_CFG5, core),
		RADIO_REG_20693(pi, TXMIX5G_CFG6, core),
		RADIO_REG_20693(pi, PA5G_CFG4, core),
	};


	PHY_TRACE(("wl%d: %s\n core: %d Channel: %d\n", pi->sh->unit, __FUNCTION__,
		core, ci20693_pll->chan));

	off_ptr = &pll_tune_reg_offsets_20693[0];
	val_ptr = &ci20693_pll->pll_vcocal1;

	for (ct = 0; ct < ARRAYSIZE(pll_tune_reg_offsets_20693); ct++)
		phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
}

static void wlc_phy_radio20693_pll_tune_wave2(phy_info_t *pi, const void *chan_info_pll,
	uint8 core)
{
	const uint16 *val_ptr;
	uint16 *off_ptr;
	uint8 ct, rdcore;
	const chan_info_radio20693_pll_wave2_t *ci20693_pll = chan_info_pll;

	uint16 pll_tune_reg_offsets_20693[] = {
		RADIO_PLLREG_20693(pi, WL_XTAL_CFG3, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL18, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL3, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL4, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL7, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL8, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL20, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL1, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL12, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL13, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL10, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL11, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL19, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL6, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL9, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL17, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL15, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL2, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL24, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL26, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL25, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL21, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL22, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL23, core),
		RADIO_PLLREG_20693(pi, PLL_VCO7, core),
		RADIO_PLLREG_20693(pi, PLL_VCO2, core),
		RADIO_PLLREG_20693(pi, PLL_FRCT2, core),
		RADIO_PLLREG_20693(pi, PLL_FRCT3, core),
		RADIO_PLLREG_20693(pi, PLL_VCO6, core),
		RADIO_PLLREG_20693(pi, PLL_VCO5, core),
		RADIO_PLLREG_20693(pi, PLL_VCO4, core),
		RADIO_PLLREG_20693(pi, PLL_LF4, core),
		RADIO_PLLREG_20693(pi, PLL_LF5, core),
		RADIO_PLLREG_20693(pi, PLL_LF7, core),
		RADIO_PLLREG_20693(pi, PLL_LF2, core),
		RADIO_PLLREG_20693(pi, PLL_LF3, core),
		RADIO_PLLREG_20693(pi, PLL_CP4, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL5, core),
		RADIO_PLLREG_20693(pi, LO2G_LOGEN0_DRV, core),
		RADIO_PLLREG_20693(pi, LO2G_VCO_DRV_CFG2, core),
		RADIO_PLLREG_20693(pi, LO2G_LOGEN1_DRV, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE0_CFG1, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE1_CFG1, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE2_CFG1, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE3_CFG1, core),
	};

	uint8 ctbase = ARRAYSIZE(pll_tune_reg_offsets_20693);

	PHY_TRACE(("wl%d: %s\n pll_core: %d Channel: %d\n", pi->sh->unit, __FUNCTION__,
	        core, ci20693_pll->chan));

	off_ptr = &pll_tune_reg_offsets_20693[0];
	val_ptr = &ci20693_pll->wl_xtal_cfg3;

	for (ct = 0; ct < ARRAYSIZE(pll_tune_reg_offsets_20693); ct++) {
		phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
	}

	if (ACMAJORREV_33(pi->pubpi->phy_rev) &&
			PHY_AS_80P80(pi, pi->radio_chanspec) &&
			(core == 1)) {
		FOREACH_CORE(pi, rdcore) {
		   uint16 radio_tune_reg_offsets_20693[] = {
				RADIO_REG_20693(pi, LNA2G_TUNE,			rdcore),
				RADIO_REG_20693(pi, LOGEN2G_RCCR,		rdcore),
				RADIO_REG_20693(pi, TXMIX2G_CFG5,		rdcore),
				RADIO_REG_20693(pi, PA2G_CFG2,			rdcore),
				RADIO_REG_20693(pi, TX_LOGEN2G_CFG1,	rdcore),
				RADIO_REG_20693(pi, LNA5G_TUNE,			rdcore),
				RADIO_REG_20693(pi, LOGEN5G_RCCR,		rdcore),
				RADIO_REG_20693(pi, TX5G_TUNE,			rdcore),
				RADIO_REG_20693(pi, TX_LOGEN5G_CFG1,	rdcore)
			};

			if (rdcore >= 2) {
				for (ct = 0; ct < ARRAYSIZE(radio_tune_reg_offsets_20693); ct++) {
					off_ptr = &radio_tune_reg_offsets_20693[0];
					phy_utils_write_radioreg(pi,
						off_ptr[ct], val_ptr[ctbase + ct]);
					PHY_INFORM(("wl%d: %s, %6x \n", pi->sh->unit, __FUNCTION__,
						phy_utils_read_radioreg(pi, off_ptr[ct])));
				}
			}
		}
	} else {
		uint16 radio_tune_allreg_offsets_20693[] = {
			RADIO_ALLREG_20693(pi, LNA2G_TUNE),
			RADIO_ALLREG_20693(pi, LOGEN2G_RCCR),
			RADIO_ALLREG_20693(pi, TXMIX2G_CFG5),
			RADIO_ALLREG_20693(pi, PA2G_CFG2),
			RADIO_ALLREG_20693(pi, TX_LOGEN2G_CFG1),
			RADIO_ALLREG_20693(pi, LNA5G_TUNE),
			RADIO_ALLREG_20693(pi, LOGEN5G_RCCR),
			RADIO_ALLREG_20693(pi, TX5G_TUNE),
			RADIO_ALLREG_20693(pi, TX_LOGEN5G_CFG1)
		};
		for (ct = 0; ct < ARRAYSIZE(radio_tune_allreg_offsets_20693); ct++) {
			off_ptr = &radio_tune_allreg_offsets_20693[0];
			phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ctbase + ct]);
			PHY_INFORM(("wl%d: %s, %6x \n", pi->sh->unit, __FUNCTION__,
					phy_utils_read_radioreg(pi, off_ptr[ct])));
		}
	}
}

static void wlc_phy_radio20693_rffe_rsdb_wr(phy_info_t *pi, uint8 core, uint8 reg_type,
	uint16 reg_val, uint16 reg_off)
{
	uint8 access_info, write, is_crisscross_wr;
	access_info = wlc_phy_radio20693_tuning_get_access_info(pi, core, reg_type);
	write = (access_info >> RADIO20693_TUNE_REG_WR_SHIFT) & 1;
	is_crisscross_wr = (access_info >> RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT) & 1;

	PHY_TRACE(("wl%d: %s\n"
		"core: %d reg_type: %d reg_val: %x reg_off: %x write %d is_crisscross_wr %d\n",
		pi->sh->unit, __FUNCTION__,
		core, reg_type, reg_val, reg_off, write, is_crisscross_wr));

	if (write != 0) {
		/* switch base address to other core to access regs via criss-cross path */
		if (is_crisscross_wr == 1) {
			si_d11_switch_addrbase(pi->sh->sih, !core);
		}

		phy_utils_write_radioreg(pi, reg_off, reg_val);

		/* restore the based adress */
		if (is_crisscross_wr == 1) {
			si_d11_switch_addrbase(pi->sh->sih, core);
		}
	}
}
static void wlc_phy_radio20693_rffe_tune(phy_info_t *pi, const void *chan_info_rffe,
	uint8 core)
{
	const chan_info_radio20693_rffe_t *ci20693_rffe = chan_info_rffe;
	const uint16 *val_ptr;
	uint16 *off_ptr;
	uint8 ct;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint16 phymode = phy_get_phymode(pi);

	uint16 rffe_tune_reg_offsets_20693_cr0[RADIO20693_NUM_RFFE_TUNE_REGS] = {
		RADIO_REG_20693(pi, LNA2G_TUNE, 0),
		RADIO_REG_20693(pi, LNA5G_TUNE, 0),
		RADIO_REG_20693(pi, PA2G_CFG2,  0)
	};
	uint16 rffe_tune_reg_offsets_20693_cr1[RADIO20693_NUM_RFFE_TUNE_REGS] = {
		RADIO_REG_20693(pi, LNA2G_TUNE, 1),
		RADIO_REG_20693(pi, LNA5G_TUNE, 1),
		RADIO_REG_20693(pi, PA2G_CFG2,  1)
	};

	PHY_TRACE(("wl%d: %s core: %d crisscross_actv: %d\n", pi->sh->unit, __FUNCTION__, core,
		pi_ac->radioi->is_crisscross_actv));

	val_ptr = &ci20693_rffe->lna2g_tune;
	if ((pi_ac->radioi->is_crisscross_actv == 0) || (phymode == PHYMODE_MIMO)) {
		/* crisscross is disabled => its a 2 antenna board.
		in the case of RSDB mode:
			this proc gets called during each core init and hence
			just writing to current core would suffice.
		in the case of 80p80 mode:
			this proc gets called twice since fc is a two
			channel list and hence just writing to current core would suffice.
		in the case of MIMO mode:
			it should be a 2 antenna board.
		so, just writing to $core would take care of all the use case.
		 */
		off_ptr = (core == 0) ? &rffe_tune_reg_offsets_20693_cr0[0] :
			&rffe_tune_reg_offsets_20693_cr1[0];
		for (ct = 0; ct < RADIO20693_NUM_RFFE_TUNE_REGS; ct++)
			phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
	} else {
		/* crisscross is active */
		if (phymode == PHYMODE_RSDB) {
			const uint8 *map_ptr = &rffe_tune_reg_map_20693[0];
			off_ptr = &rffe_tune_reg_offsets_20693_cr0[0];
			for (ct = 0; ct < RADIO20693_NUM_RFFE_TUNE_REGS; ct++) {
				wlc_phy_radio20693_rffe_rsdb_wr(pi, phy_get_current_core(pi),
					map_ptr[ct], val_ptr[ct], off_ptr[ct]);
			}
		} else if (phymode == PHYMODE_80P80) {
			uint8 access_info, write, is_crisscross_wr;

			access_info = wlc_phy_radio20693_tuning_get_access_info(pi, core, 0);
			write = (access_info >> RADIO20693_TUNE_REG_WR_SHIFT) & 1;
			is_crisscross_wr = (access_info >>
				RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT) & 1;

			PHY_TRACE(("wl%d: %s\n write %d is_crisscross_wr %d\n",
				pi->sh->unit, __FUNCTION__,	write, is_crisscross_wr));

			if (write != 0) {
				core = (is_crisscross_wr == 0) ? core : !core;
				off_ptr = (core == 0) ? &rffe_tune_reg_offsets_20693_cr0[0] :
					&rffe_tune_reg_offsets_20693_cr1[0];

				for (ct = 0; ct < RADIO20693_NUM_RFFE_TUNE_REGS; ct++) {
					phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
				}
			} /* Write != 0 */
		} /* 80P80 */
	} /* Criss-Cross Active */
	if (ACMAJORREV_4(pi->pubpi->phy_rev) && (!(PHY_IPA(pi))) &&
		(RADIO20693REV(pi->pubpi->radiorev) == 13) && CHSPEC_IS2G(pi->radio_chanspec)) {
		/* 4349A2 TX2G die-2/rev-13 iPA used as ePA */
		wlc_phy_radio20693_tx2g_set_freq_tuning_ipa_as_epa(pi, core,
			CHSPEC_CHANNEL(pi->radio_chanspec));
	}
}

/* This function return 2bits.
bit 0: 0 means donot write. 1 means write
bit 1: 0 means direct access. 1 means criss-cross access
*/
static uint8 wlc_phy_radio20693_tuning_get_access_info(phy_info_t *pi, uint8 core, uint8 reg_type)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 access_info = 0;

	ASSERT(phymode != PHYMODE_MIMO);

	PHY_TRACE(("wl%d: %s core: %d phymode: %d\n", pi->sh->unit, __FUNCTION__, core, phymode));

	if (phymode == PHYMODE_80P80) {
		if (core != pi_ac->radioi->crisscross_priority_core_80p80) {
			access_info = 0;
		} else {
			access_info = (1 << RADIO20693_TUNE_REG_WR_SHIFT);
			/* figure out which core has 5G affinity */
			if (RADIO20693_CORE1_AFFINITY == 5) {
				access_info |= (core == 1) ? 0 :
					(1 << RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT);
			} else {
				access_info |= (core == 0) ? 0 :
					(1 << RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT);
			}
		}
	} else if (phymode == PHYMODE_RSDB)	{
		uint8 band_curr_cr, band_oth_cr;
		phy_info_t *other_pi = phy_get_other_pi(pi);

		band_curr_cr = CHSPEC_IS2G(pi->radio_chanspec) ? RADIO20693_TUNEREG_TYPE_2G :
			RADIO20693_TUNEREG_TYPE_5G;
		band_oth_cr = CHSPEC_IS2G(other_pi->radio_chanspec) ? RADIO20693_TUNEREG_TYPE_2G :
			RADIO20693_TUNEREG_TYPE_5G;

		PHY_TRACE(("wl%d: %s current_core: %d other_core: %d\n", pi->sh->unit, __FUNCTION__,
			core, !core));
		PHY_TRACE(("wl%d: %s band_curr_cr: %d band_oth_cr: %d\n", pi->sh->unit,
			__FUNCTION__, band_curr_cr, band_oth_cr));

		/* if a tuning reg is neither a 2G reg nor a 5g reg,
		   then assume this register belongs to band of current core
		 */
		if (reg_type == RADIO20693_TUNEREG_TYPE_NONE) {
			reg_type = band_curr_cr;
		}

		/* Do not write anything if reg_type and band_curr_cr are different */
		if (reg_type != band_curr_cr) {
			access_info = 0;
			PHY_TRACE(("w%d: %s. IGNORE: reg_type and band_curr_cr are different\n",
				pi->sh->unit, __FUNCTION__));
			return (access_info);
		}

		/* find out if there is a conflict of bands between two cores.
		 if 	--> no conflict case
		 else 	--> Conflict case
		 */
		if (band_curr_cr != band_oth_cr) {
			PHY_TRACE(("wl%d: %s. No conflict case. bands are different\n",
				pi->sh->unit, __FUNCTION__));

			/* find out if its a direct access or criss-cross_access */
			access_info = wlc_phy_radio20693_tuning_get_access_type(pi, core, reg_type);
		} else {
			PHY_TRACE(("wl%d: %s. Conflict case. bands are same\n",
				pi->sh->unit, __FUNCTION__));
			if (core != pi_ac->radioi->crisscross_priority_core_rsdb) {
				PHY_TRACE(("wl%d: %s IGNORE: priority is not met\n",
					pi->sh->unit, __FUNCTION__));

				PHY_TRACE(("wl%d: %s current_core: %d Priority: %d\n",
					pi->sh->unit, __FUNCTION__, core,
					pi_ac->radioi->crisscross_priority_core_rsdb));

				access_info = 0;
			}

			/* find out if its a direct access or criss-cross_access */
			access_info = wlc_phy_radio20693_tuning_get_access_type(pi, core, reg_type);
		}
	}

	PHY_TRACE(("wl%d: %s. Access Info %x\n",
		pi->sh->unit, __FUNCTION__, access_info));

	return (access_info);
}
static uint8 wlc_phy_radio20693_tuning_get_access_type(phy_info_t *pi, uint8 core, uint8 reg_type)
{
	uint8 is_dir_access, access_info;

	/* find out if its a direct access or criss-cross_access */
	is_dir_access = ((core == 0) && (reg_type == RADIO20693_CORE0_AFFINITY));
	is_dir_access |= ((core == 1) && (reg_type == RADIO20693_CORE1_AFFINITY));

	/* its a direct access */
	access_info = (1 << RADIO20693_TUNE_REG_WR_SHIFT);

	if (is_dir_access == 0) {
		/* Its a criss-cross access */
		access_info |= (1 << RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT);
	}

	return (access_info);
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

static void
wlc_phy_radio20693_minipmu_pwron_seq(phy_info_t *pi)
{
	uint8 core;
	/* Power up the TX LDO */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_en, 0x1);
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_TXldo_pu, 1);
	}
	OSL_DELAY(110);
	/* Power up the radio Band gap and the 2.5V reg */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, VREG_CFG, core, vreg25_pu, 1);
		if (RADIOMAJORREV(pi) == 3) {
			MOD_RADIO_PLLREG_20693(pi, BG_CFG1, core, bg_pu, 1);
		} else {
			MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_pu, 1);
		}
	}
	OSL_DELAY(10);
	/* Power up the VCO, AFE, RX & ADC Ldo's */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_VCOldo_pu, 1);
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_AFEldo_pu, 1);
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_RXldo_pu, 1);
		MOD_RADIO_REG_TINY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 1);
	}
	OSL_DELAY(12);
	/* Enable the Synth and Force the wlpmu_spare to 0 */
	if (RADIOMAJORREV(pi) != 3) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_TINY(pi, PLL_CFG1, core, synth_pwrsw_en, 1);
			MOD_RADIO_REG_TINY(pi, PMU_CFG5, core, wlpmu_spare, 0);
		}
	}
}

/* PU the TX/RX/VCO/..LDO's */
static void wlc_phy_radio20696_minipmu_pwron_seq(phy_info_t *pi)
{
	uint8 core;

	/* Turn on two 1p8LDOs and Bandgap  */
	MOD_RADIO_REG_20696(pi, BG_OVR1, 1, ovr_bg_pu_bgcore, 0x1);
	MOD_RADIO_REG_20696(pi, LDO1P8_STAT, 1, ldo1p8_pu, 0x1);
	MOD_RADIO_REG_20696(pi, LDO1P8_STAT, 3, ldo1p8_pu, 0x1);
	MOD_RADIO_REG_20696(pi, BG_REG2, 1, bg_pu_bgcore, 0x1);
	MOD_RADIO_REG_20696(pi, BG_REG2, 1, bg_pu_V2I, 0x1);

	/* Hold RF PLL in reset */
	MOD_RADIO_PLLREG_20696(pi, PLL_VCOCAL_OVR1, ovr_rfpll_vcocal_rst_n, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_CFG2, rfpll_rst_n, 0x0);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_rfpll_rst_n, 0x1);
	MOD_RADIO_PLLREG_20696(pi, PLL_VCOCAL1, rfpll_vcocal_rst_n, 0x0);

	FOREACH_CORE(pi, core) {
		/* Overwrites */
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_ADCldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_AFEldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_LDO2P1_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_LOGENldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_TXldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_en, 0x1);

		MOD_RADIO_REG_20696(pi, RX2G_REG4, core, rx_ldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, LDO1P65_STAT, core, wlpmu_ldo1p6_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_ADCldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_AFEldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_CFG4, core, wlpmu_LDO2P1_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_LOGENldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_TXldo_pu, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_CFG4, core, wlpmu_en, 0x1);
		MOD_RADIO_REG_20696(pi, PMU_CFG3, core, vref_select, 0x0);
	}
}

static void wlc_phy_radio20693_pmu_override(phy_info_t *pi)
{
	uint8  core;
	uint16 data;

	FOREACH_CORE(pi, core) {
		data = READ_RADIO_REG_20693(pi, PMU_OVR, core);
		data = data | 0xFC;
		phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OVR, core), data);
	}
}

static void
wlc_phy_radio20693_xtal_pwrup(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	/* Note: Core0 XTAL will be powerup by PMUcore in chipcommon */
	if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		((phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1))) {
		/* Powerup xtal ldo for MAC_Core = 1 in case of RSDB mode */
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 0,
				ldo_1p2_xtalldo1p2_vref_bias_reset, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, 0,
				ovr_ldo_1p2_xtalldo1p2_vref_bias_reset, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_BG_pu, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, 0,
				ovr_ldo_1p2_xtalldo1p2_BG_pu, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
		OSL_DELAY(300);
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_vref_bias_reset, 0);
	} else if ((phy_get_phymode(pi) != PHYMODE_RSDB)) {
		/* Powerup xtal ldo for Core 1 in case of MIMO and 80+80 */
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1,
				ldo_1p2_xtalldo1p2_vref_bias_reset, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, 1,
				ovr_ldo_1p2_xtalldo1p2_vref_bias_reset, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_BG_pu, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, 1,
				ovr_ldo_1p2_xtalldo1p2_BG_pu, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
		OSL_DELAY(300);
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_vref_bias_reset, 0);
	}
}

static void
wlc_phy_radio20693_upd_prfd_values(phy_info_t *pi)
{
	uint i = 0;
	const radio_20xx_prefregs_t *prefregs_20693_ptr = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	/* Choose the right table to use */
	switch (RADIO20693REV(pi->pubpi->radiorev)) {
	case 3:
	case 4:
	case 5:
		prefregs_20693_ptr = prefregs_20693_rev5;
		break;
	case 6:
		prefregs_20693_ptr = prefregs_20693_rev6;
		break;
	case 7:
		prefregs_20693_ptr = prefregs_20693_rev5;
		break;
	case 8:
		prefregs_20693_ptr = prefregs_20693_rev6;
		break;
	case 10:
		prefregs_20693_ptr = prefregs_20693_rev10;
		break;
	case 11:
	case 12:
	case 13:
		prefregs_20693_ptr = prefregs_20693_rev13;
		break;
	case 14:
		prefregs_20693_ptr = prefregs_20693_rev14;
		break;
	case 19:
		prefregs_20693_ptr = prefregs_20693_rev19;
		break;
	case 15:
		if (PHY_XTAL_IS37M4(pi)) {
			prefregs_20693_ptr = prefregs_20693_rev15_37p4MHz;
		} else {
			prefregs_20693_ptr = prefregs_20693_rev15_40MHz;
		}
		break;
	case 16:
	case 17:
	case 20:
		prefregs_20693_ptr = prefregs_20693_rev5;
		break;
	case 18:
	case 21:
		if (PHY_XTAL_IS37M4(pi)) {
			prefregs_20693_ptr = prefregs_20693_rev18;
		} else {
			prefregs_20693_ptr = prefregs_20693_rev15_40MHz;
		}
		break;
	case 23:
		/* Preferred values for 53573/53574/47189-B0 */
		prefregs_20693_ptr = prefregs_20693_rev23_40MHz;
		break;
	case 32:
	case 33:
		prefregs_20693_ptr = prefregs_20693_rev32;
		break;
	case 9:
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO20693REV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return;
	}

	/* Update preferred values */
	while (prefregs_20693_ptr[i].address != 0xffff) {
		if (!((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(prefregs_20693_ptr[i].address & JTAG_20693_CR1)))
			phy_utils_write_radioreg(pi, prefregs_20693_ptr[i].address,
				(uint16)prefregs_20693_ptr[i].init);
		i++;
	}
}

void
wlc_phy_radio20693_vco_opt(phy_info_t *pi, bool isVCOTUNE_5G)
{
	uint8 core;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	PHY_TRACE(("%s\n", __FUNCTION__));
	if (pi_ac->radioi->lpmode_2g != ACPHY_LPMODE_NONE) {
		if (CHSPEC_IS2G(pi->radio_chanspec) && (PHY_IPA(pi))) {
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20693(pi, PLL_VCO3, core,
					rfpll_vco_en_alc, 0x1);
				MOD_RADIO_REG_20693(pi, PLL_VCO6, core,
					rfpll_vco_ALC_ref_ctrl, 0x8);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO2, core,
					ldo_2p5_lowquiescenten_CP, 0x1);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO2, core,
					ldo_2p5_lowquiescenten_VCO, 0x1);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO4, core,
					ldo_2p5_static_load_CP, 0x1);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO4, core,
					ldo_2p5_static_load_VCO, 0x1);
			}
		} else {
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20693(pi, PLL_VCO3, core,
					rfpll_vco_en_alc, 0x0);
				MOD_RADIO_REG_20693(pi, PLL_VCO6, core,
					rfpll_vco_ALC_ref_ctrl, 0x8);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO2, core,
					ldo_2p5_lowquiescenten_CP, 0x0);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO2, core,
					ldo_2p5_lowquiescenten_VCO, 0x0);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO4, core,
					ldo_2p5_static_load_CP, 0x0);
				MOD_RADIO_REG_20693(pi, PLL_HVLDO4, core,
					ldo_2p5_static_load_VCO, 0x0);
			}
		}
	}
	/* Loading vcotune settings */
	if (ROUTER_4349(pi)) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20693(pi, PLL_VCO3, core,
				rfpll_vco_en_alc, isVCOTUNE_5G ? 0x1 : 0x0);
			MOD_RADIO_REG_20693(pi, PLL_VCO6, core,
				rfpll_vco_ALC_ref_ctrl, isVCOTUNE_5G ? 0x6 : 0x0);
			MOD_RADIO_REG_20693(pi, PMU_CFG2, core,
				wlpmu_VCOldo_adj, isVCOTUNE_5G ? 0x3 : 0x0);
		}
	}
}

int8
wlc_phy_tiny_radio_minipmu_cal(phy_info_t *pi)
{
	uint8 core, calsuccesful = 1;
	int8 cal_status = 1;
	int16 waitcounter = 0;
	phy_info_t *pi_core0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
	bool is_rsdb_core1_cal = FALSE;

	/* Skipping the minipmu cal for 53573/47189 till we fix the issue JIRA: SWWLAN-71368 */
	if (BCM53573_CHIP(pi->sh->chip) && (CHIPREV((pi)->sh->chiprev) == 0)) {
		return 0;
	}

	ASSERT(TINY_RADIO(pi));
	/* Skip this function for QT */
	if (ISSIM_ENAB(pi->sh->sih))
		return 0;

	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
		(phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1) &&
		(RADIO20693REV(pi->pubpi->radiorev) <= 0x17) &&
		(RADIO20693REV(pi->pubpi->radiorev) != 0x13)) {
		 // this fix is not needed from 4355B1 variants
		is_rsdb_core1_cal = TRUE;
		wlc_phy_cals_mac_susp_en_other_cr(pi, TRUE);
	}

	/* Setup the cal */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_VCOldo_pu, 0);
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_AFEldo_pu, 0);
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_RXldo_pu, 0);
			MOD_RADIO_REG_TINY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 0);
		}
	}
	FOREACH_CORE(pi, core) {
		if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
			MOD_RADIO_REG_TINY(pi, PMU_CFG3, core, wlpmu_selavg, 2);
		} else if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
			if (RADIOMAJORREV(pi) == 2) {
				MOD_RADIO_REG_20693(pi, PMU_CFG3, core, wlpmu_selavg_lo, 0);
			} else {
				MOD_RADIO_REG_20693(pi, PMU_CFG3, core, wlpmu_selavg, 0);
			}
		}
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_vref_select, 1);
		MOD_RADIO_REG_TINY(pi, PMU_CFG2, core, wlpmu_bypcal, 0);
		MOD_RADIO_REG_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 1);
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 0);
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		/* ldobg_cal_clken coming out from radiodig_xxx_core1 is not used */
		/* only radiodig_xx_core0 bit is used.Refer JIRA:SW4349-310 for more details */
		/* JIRA: SWWLAN-70619, SW4349-310 */
		if (is_rsdb_core1_cal) {
			MOD_RADIO_REG_TINY(pi_core0, PMU_STAT, core, wlpmu_ldobg_cal_clken, 1);
		} else {
			MOD_RADIO_REG_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_clken, 1);
		}
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 1);
	}
	FOREACH_CORE(pi, core) {
		/* Wait for cal_done */
		if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev)) {
			waitcounter = 0;
			calsuccesful = 1;
			while (READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core,
				wlpmu_ldobg_cal_done) == 0) {
				OSL_DELAY(100);
				waitcounter ++;
				if (waitcounter > 100) {
					/* cal_done bit is not 1 even after waiting for a while */
					/* Exit gracefully */
					PHY_TRACE(("\nWarning:Mini PMU Cal Failed on Core%d \n",
						core));
					calsuccesful = 0;
					break;
				}
			}
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 0);
			if (calsuccesful == 1) {
				PHY_TRACE(("MiniPMU cal done on core %d, Cal code = %d", core,
					READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core, wlpmu_calcode)));
			} else {
				PHY_TRACE(("Cal Unsuccesful on Core %d", core));
				cal_status = -1;
			}
		} else {
			SPINWAIT(READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core,
				wlpmu_ldobg_cal_done) == 0, ACPHY_SPINWAIT_MINIPMU_CAL_STATUS);
			if (READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_done) == 0) {
				PHY_ERROR(("%s : Cal Unsuccesful on Core %d\n", __FUNCTION__,
					core));
				cal_status = -1;
				PHY_FATAL_ERROR_MESG(
					(" %s: SPINWAIT ERROR : PMU cal failed on Core%d\n",
					__FUNCTION__, core));
				PHY_FATAL_ERROR(pi, PHY_RC_PMUCAL_FAILED);
			}
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 0);
			PHY_TRACE(("MiniPMU cal done on core %d, Cal code = %d\n", core,
				READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core, wlpmu_calcode)));
		}
	}
	FOREACH_CORE(pi, core) {
		/* JIRA: SWWLAN-70619, SW4349-310 */
		if (is_rsdb_core1_cal) {
			MOD_RADIO_REG_TINY(pi_core0, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
		} else {
			MOD_RADIO_REG_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
		}
	}
	if (is_rsdb_core1_cal) {
		wlc_phy_cals_mac_susp_en_other_cr(pi, FALSE);
	}

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_VCOldo_pu, 1);
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_AFEldo_pu, 1);
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_RXldo_pu, 1);
			MOD_RADIO_REG_TINY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 1);
		}
	}

	return cal_status;
}

static void
wlc_phy_radio20693_pmu_pll_config(phy_info_t *pi)
{
	uint8 core;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	/* # powerup pll */
	FOREACH_CORE(pi, core) {
		if ((phy_get_phymode(pi) == PHYMODE_MIMO) &&
			(core != 0))
			break;

		/* # VCO */
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_pu, 1);
		/* # VCO LDO */
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_VCO, 1);
		/* # VCO buf */
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_buf_pu, 1);
		/* # Synth global */
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_synth_pu, 1);
		/* # Charge Pump */
		MOD_RADIO_REG_20693(pi, PLL_CP1, core, rfpll_cp_pu, 1);
		/* # Charge Pump LDO */
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_CP, 1);
		/* # PLL lock monitor */
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_monitor_pu, 1);
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_CP, 1);
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_VCO, 1);
	}
	OSL_DELAY(89);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_CP, 0);
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_VCO, 0);
	}
}

void
phy_ac_radio_20693_pmu_pll_config_wave2(phy_info_t *pi, uint8 pll_mode)
{
	// PLL/VCO operating modes: set by pll_mode
	// Mode 0. RFP0 non-coupled, e.g. 4x4 MIMO non-1024QAM
	// Mode 1. RFP0 coupled, e.g. 4x4 MIMO 1024QAM
	// Mode 2. RFP0 non-coupled + RFP1 non-coupled: 2x2 + 2x2 MIMO non-1024QAM
	// Mode 3. RFP0 non-coupled + RFP1 coupled: 3x3 + 1x1 scanning in 80MHz mode
	// Mode 4. RFP0 coupled + RFP1 non-coupled: 3x3 MIMO 1024QAM + 1x1 scanning in 20MHz mode
	// Mode 5. RFP0 coupled + RFP1 coupled: 3x3 MIMO 1024QAM + 1x1 scanning in 80MHz mode
	// Mode 6. RFP1 non-coupled
	// Mode 7. RFP1 coupled

	uint8 core, start_core, end_core;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_INFORM(("wl%d: %s: pll_mode %d\n",
			pi->sh->unit, __FUNCTION__, pll_mode));

	switch (pll_mode) {
	case 0:
	        // intentional fall through
	case 1:
	        start_core = 0;
		end_core   = 0;
		break;
	case 2:
	        // intentional fall through
	case 3:
	        // intentional fall through
	case 4:
	        // intentional fall through
	case 5:
	        start_core = 0;
		end_core   = 1;
		break;
	case 6:
	        // intentional fall through
	case 7:
	        start_core = 1;
		end_core   = 1;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported PLL/VCO operating mode %d\n",
			pi->sh->unit, __FUNCTION__, pll_mode));
		ASSERT(0);
		return;
	}

	MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x3f);

	for (core = start_core; core <= end_core; core++) {
		uint8  ct;
		// Put all regs/fields write/modification in a array
		uint16 pll_regs_bit_vals1[][3] = {
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_synth_pu, 1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_cp_pu,    1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_buf_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_ldo_2p5_pu_ldo_CP,  1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_ldo_2p5_pu_ldo_VCO, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_synth_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CP1,  core, rfpll_cp_pu,      1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_vco_pu,     1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_vco_buf_pu, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_monitor_pu, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_LF6,  core, rfpll_lf_cm_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_CP,  1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_VCO, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1,   core, rfpll_pfd_en, 0x2),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_rst_n, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_VCOCAL_OVR1, core,
			ovr_rfpll_vcocal_rst_n, 1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core,
			ovr_rfpll_ldo_cp_bias_reset,  1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core,
			ovr_rfpll_ldo_vco_bias_reset, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG2,    core, rfpll_rst_n,            0),
			RADIO_PLLREGC_FLD_20693(pi, PLL_VCOCAL1, core, rfpll_vcocal_rst_n,     0),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_CP,  1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_VCO, 1),
		};
		uint16 pll_regs_bit_vals2[][3] = {
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG2,    core, rfpll_rst_n,            1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_VCOCAL1, core, rfpll_vcocal_rst_n,     1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_CP,  0),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_VCO, 0),
		};

		// now write/modification to radio regs
		for (ct = 0; ct < ARRAYSIZE(pll_regs_bit_vals1); ct++) {
			phy_utils_mod_radioreg(pi, pll_regs_bit_vals1[ct][0],
			                       pll_regs_bit_vals1[ct][1],
			                       pll_regs_bit_vals1[ct][2]);
		}
		OSL_DELAY(10);

		for (ct = 0; ct < ARRAYSIZE(pll_regs_bit_vals2); ct++) {
			phy_utils_mod_radioreg(pi, pll_regs_bit_vals2[ct][0],
			                       pll_regs_bit_vals2[ct][1],
			                       pll_regs_bit_vals2[ct][2]);
		}
	}
}

static void
wlc_phy_switch_radio_acphy_20693(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	/* minipmu_cal */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3)
		wlc_phy_tiny_radio_minipmu_cal(pi);

	/* r cal */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
#ifdef ATE_BUILD
	  /* ATE firmware performs the rcal and the value is put in the OTP. */
	  wlc_phy_radio_tiny_rcal_wave2(pi, 2);
#else
	  wlc_phy_radio_tiny_rcal_wave2(pi, 1);
#endif
	}

	/* power up pmu pll */
	if (RADIOMAJORREV(pi) == 3) {
		if (phy_get_phymode(pi) == PHYMODE_80P80 ||
				PHY_AS_80P80(pi, pi->radio_chanspec)) {
				/* for 4365 C0 - turn on both PLL */
			phy_ac_radio_20693_pmu_pll_config_wave2(pi, 2);
		} else {
			phy_ac_radio_20693_pmu_pll_config_wave2(pi, 0);
		}
	} else {
		wlc_phy_radio20693_pmu_pll_config(pi);
	}

	if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3)) {
		/* minipmu_cal */
		wlc_phy_tiny_radio_minipmu_cal(pi);

	/* RCAL */
#ifdef ATE_BUILD
		/* ATE firmware performs the rcal and the value is put in the OTP. */
		wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE3_BOTH_CORE_CAL);
#else
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			if ((pi->sromi->rcal_otp_val >= 2) &&
				(pi->sromi->rcal_otp_val <= 12)) {
				pi->rcal_value = pi->sromi->rcal_otp_val;
			}
		}
		wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE1_STATIC);
#endif /* ATE_BUILD */
	}
}

static void
wlc_phy_switch_radio_acphy(phy_info_t *pi, bool on)
{
	uint8 core, pll = 0;
	uint16 data = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
#ifdef LOW_TX_CURRENT_SETTINGS_2G
	uint8 is_ipa = 0;
#endif

	PHY_TRACE(("wl%d: %s %s corenum %d\n", pi->sh->unit, __FUNCTION__, on ? "ON" : "OFF",
		pi->pubpi->phy_corenum));

	if (on) {
		if (!pi->radio_is_on) {
			if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
					/*  set the low pwoer reg before radio init */
					wlc_phy_set_lowpwr_phy_reg(pi);
					wlc_phy_radio2069_mini_pwron_seq_rev16(pi);
				} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
					/*  set the low pwoer reg before radio init */
					wlc_phy_set_lowpwr_phy_reg_rev3(pi);
					wlc_phy_radio2069_mini_pwron_seq_rev32(pi);
				}
				if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) &&
						((RADIO2069_MINORREV(pi->pubpi->radiorev) == 16) ||
						(RADIO2069_MINORREV(pi->pubpi->radiorev) == 17))) {
				   /* ToUnblock the QT tests on 4364-3x3 flavor added the */
				   /* reg config. Based on radio team reccomendatation */
				   /* to be reverted after proper settings */
				   MOD_RADIO_REG(pi, RFP, GP_REGISTER, gp_pcie, 3);
				}

				wlc_phy_radio2069_pwron_seq(pi);
				/* 4364 dual phy. when 3x3 is powered down, make sure the 1x1 */
				/* femctrl lines do not toggle ant0/ant1 this is done by giving */
				/* bt the control on ant0/ant1. If bt is not there (bt_reg_on 0) */
				/* this is fine. Ultimately bt firmware needs to write proper */
				/* values in the bt_clb_upi registers bt2clb_swctrl_smask_bt_antX */
				if (IS_4364_1x1(pi) || IS_4364_3x3(pi)) {
					si_gci_set_femctrl_mask_ant01(pi->sh->sih, pi->sh->osh, 1);
				}
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
				wlc_phy_tiny_radio_pwron_seq(pi);
				wlc_phy_switch_radio_acphy_20691(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
				/* JIRA: SW4349-1154 */
				acphy_pmu_core1_off_radregs_t *porig =
					(pi_ac->radioi->pmu_c1_off_info_orig);
				acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig =
					(pi_ac->radioi->pmu_lp_opt_orig);

				porig->is_orig = FALSE;
				pmu_lp_opt_orig->is_orig = FALSE;

				/* ------------------------------------------------------------- */
				/* In case of 4349A0, there will be 2 XTALs. Core0 XTAL will be  */
				/* Pwrdup by PMU and Core1 XTAL needs to be pwrdup accordingly   */
				/* in all the modes like RSDB, MIMO and 80P80                    */
				/* ------------------------------------------------------------- */
				wlc_phy_radio20693_xtal_pwrup(pi);

				wlc_phy_tiny_radio_pwron_seq(pi);

				/* JIRA: HW53573-139. For 4349-B0 and 53573 radio, enabling
				 * pmu_overrides to enable the pmu register programming
				 */
				if (RADIOMAJORREV(pi) == 2) {
					FOREACH_CORE(pi, core) {
						MOD_RADIO_REG_20693(pi, PMU_OVR, core,
							ovr_wlpmu_en, 0x1);
						MOD_RADIO_REG_20693(pi, PMU_OVR, core,
							ovr_wlpmu_AFEldo_pu, 0x1);
						MOD_RADIO_REG_20693(pi, PMU_OVR, core,
							ovr_wlpmu_TXldo_pu, 0x1);
						MOD_RADIO_REG_20693(pi, PMU_OVR, core,
							ovr_wlpmu_VCOldo_pu, 0x1);
						MOD_RADIO_REG_20693(pi, PMU_OVR, core,
							ovr_wlpmu_RXldo_pu, 0x1);
						MOD_RADIO_REG_20693(pi, PMU_OVR, core,
							ovr_wlpmu_ADCldo_pu, 0x1);
						MOD_RADIO_REG_20693(pi, PMU_OVR, core,
							ovr_wlpmu_selavg_lo, 0x1);
					}
				}

				// 4365: Enable the PMU overrides because we don't want the powerup
				// sequence to be controlled by the PMU sequencer
				if (RADIOMAJORREV(pi) == 3)
					wlc_phy_radio20693_pmu_override(pi);

				/* Power up the radio mini PMU Seq */
				wlc_phy_radio20693_minipmu_pwron_seq(pi);

				/* pll config, minipmu cal, RCAL */
				wlc_phy_switch_radio_acphy_20693(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
				/* PHY regs configuration and update preferred values */
				wlc_phy_radio20694_pwron_seq_phyregs(pi);

				/* pll PU, minipmu cal, RCAL, RCCAL */
				PHY_TRACE(("wl%d: BEFORE wlc_phy_switch_radio_acphy_20694\n",
					pi->sh->unit));
				wlc_phy_switch_radio_acphy_20694(pi);
				PHY_TRACE(("wl%d: AFTER wlc_phy_switch_radio_acphy_20694\n",
					pi->sh->unit));
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
				/* PHY regs configuration and update preferred values */
				wlc_phy_28nm_radio_pwron_seq_phyregs(pi, 0);

				/* Power up the radio mini PMU Seq */
				wlc_phy_radio20695_minipmu_pupd_seq(pi, 1);

				/* pll PU, minipmu cal, RCAL, RCCAL */
				wlc_phy_switch_radio_acphy_20695(pi);
			} else if (RADIOID(pi->pubpi->radioid) == BCM20696_ID) {
				/* 20696procs.tcl proc 20696_init */

				/* Global Radio PU's */
				wlc_phy_radio20696_pwron_seq_phyregs(pi);

				/* PU the TX/RX/VCO/..LDO's  */
				wlc_phy_radio20696_minipmu_pwron_seq(pi);

				/* R_CAL will be done only for "Core1" */
				wlc_phy_radio20696_r_cal(pi, 1);

				/* Perform MINI PMU CAL */
				wlc_phy_radio20696_minipmu_cal(pi);

				/* Final R_CAL (on Core1) */
				wlc_phy_radio20696_r_cal(pi, 2);

				/* RC CAL */
				wlc_phy_radio20696_rccal(pi);

				/* PLL PWR-UP */
				wlc_phy_radio20696_pmu_pll_pwrup(pi);

				/* WAR's */
				wlc_phy_radio20696_wars(pi);
			}

			if (!TINY_RADIO(pi) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20696_ID))) {

			/* --------------------------RCAL WAR ---------------------------- */
			/* Currently RCAL resistor is not connected on the board. The pin  */
			/* labelled TSSI_G/GPIO goes into the TSSI pin of the FEM through  */
			/* a 0 Ohm resistor. There is an option to add a shunt 10k to GND  */
			/* on this trace but it is depop. Adding shunt resistance on the   */
			/* TSSI line may affect the voltage from the FEM to our TSSI input */
			/* So, this issue is worked around by forcing below registers      */
			/* THIS IS APPLICABLE FOR BOARDTYPE = $def(boardtype)              */
			/* --------------------------RCAL WAR ---------------------------- */

				if (BF3_RCAL_OTP_VAL_EN(pi_ac) == 1) {
					data = pi->sromi->rcal_otp_val;
				} else {
					if (BF3_RCAL_WAR(pi_ac) == 1) {
						if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
							data = ACPHY_RCAL_VAL_2X2;
						} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev)
						           == 1) {
							data = ACPHY_RCAL_VAL_1X1;
						}
					}
#ifndef ATE_BUILD
					else if ((RADIO2069REV(pi->pubpi->radiorev) == 64)) {
					   /* for 4364 3x3 hard coding for now */
					   MOD_RADIO_REG(pi, RF2, BG_CFG1, rcal_trim, 10);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_bg_rcal_trim, 1);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_otp_rcal_sel, 0);

					} else if ((RADIO2069REV(pi->pubpi->radiorev) == 66)) {
					   MOD_RADIO_REG(pi, RF2, BG_CFG1, rcal_trim, 7);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_bg_rcal_trim, 1);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_otp_rcal_sel, 0);
					}
#endif
					else {
							wlc_phy_radio2069_rcal(pi);
					}
				}

				if (BF3_RCAL_WAR(pi_ac) == 1 ||
					BF3_RCAL_OTP_VAL_EN(pi_ac) == 1) {
					if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
						if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
							FOREACH_CORE(pi, core) {
								MOD_RADIO_REGC(pi, GE32_BG_CFG1,
									core, rcal_trim, data);
								MOD_RADIO_REGC(pi, GE32_OVR2, core,
									ovr_bg_rcal_trim, 1);
								MOD_RADIO_REGC(pi, GE32_OVR2, core,
									ovr_otp_rcal_sel, 0);
							}
						} else if (RADIO2069_MAJORREV
							(pi->pubpi->radiorev) == 1) {
							MOD_RADIO_REG(pi, RFP,  GE16_BG_CFG1,
								rcal_trim, data);
							MOD_RADIO_REG(pi, RFP,  GE16_OVR2,
								ovr_bg_rcal_trim, 1);
							MOD_RADIO_REG(pi, RFP,  GE16_OVR2,
								ovr_otp_rcal_sel, 0);
						}
					}
				}
			}

			if (TINY_RADIO(pi)) {
				if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) &&
				    RADIOMAJORREV(pi) == 3) {
					wlc_phy_radio_tiny_rccal_wave2(pi_ac->radioi);
				} else {
					wlc_phy_radio_tiny_rccal(pi_ac->radioi);
				}
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
				if (!ISSIM_ENAB(pi->sh->sih)) {
					wlc_phy_radio20694_rccal(pi);
				} else {
					PHY_ERROR(("bypass rccal in QT\n"));
				}
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
				wlc_phy_28nm_radio_rccal(pi);
			} else if (!RADIOID_IS(pi->pubpi->radioid, BCM20696_ID)) {
				wlc_phy_radio2069_rccal(pi);
			}

			if (phy_ac_chanmgr_get_data(pi_ac->chanmgri)->init_done) {
				wlc_phy_set_regtbl_on_pwron_acphy(pi);
				wlc_phy_chanspec_set_acphy(pi, pi->radio_chanspec);
			}
			pi->radio_is_on = TRUE;
		}
	} else {
		/* wlc_phy_radio2069_off(); */
		pi->radio_is_on = FALSE;

		/* FEM */
		ACPHYREG_BCAST(pi, RfctrlIntc0, 0);

		if (!ACMAJORREV_4(pi->pubpi->phy_rev)) {
			ACPHY_REG_LIST_START
				/* AFE */
				ACPHYREG_BCAST_ENTRY(pi, RfctrlCoreAfeCfg10, 0)
				ACPHYREG_BCAST_ENTRY(pi, RfctrlCoreAfeCfg20, 0)
				ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x1fff)

				/* Radio RX */
				ACPHYREG_BCAST_ENTRY(pi, RfctrlCoreRxPus0, 0)
				ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideRxPus0, 0xffff)
			ACPHY_REG_LIST_EXECUTE(pi);
		}

		/* Radio TX */
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

		/* {radio, rfpll, pllldo}_pu = 0 */
		MOD_PHYREG(pi, RfctrlCmd, chip_pu, 0);

		/* SW4345-58 */
		if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20691(pi, PMU_OP, core, wlpmu_en, 0);
				MOD_RADIO_REG_20691(pi, PLL_CFG1, core, rfpll_vco_pu, 0);
				MOD_RADIO_REG_20691(pi, PLL_CFG1, core, rfpll_synth_pu, 0);
				MOD_RADIO_REG_20691(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_VCO, 0);
				MOD_RADIO_REG_20691(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_CP, 0);
				MOD_RADIO_REG_20691(pi, LOGEN_CFG2, core, logencore_5g_pu, 0);
				MOD_RADIO_REG_20691(pi, LNA5G_CFG2, core, lna5g_pu_lna2, 0);
			}
		}
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
			if (RADIOMAJORREV(pi) != 3) {
				FOREACH_CORE(pi, core) {
					/* PD the RFPLL */
					data = READ_RADIO_REG_TINY(pi, PLL_CFG1, core);
					data = data & 0xE0FF;
					phy_utils_write_radioreg(pi, RADIO_REG_20693(pi,
						PLL_CFG1, core), data);
					MOD_RADIO_REG_TINY(pi, PLL_HVLDO1, core,
						ldo_2p5_pu_ldo_CP, 0);
					MOD_RADIO_REG_TINY(pi, PLL_HVLDO1, core,
						ldo_2p5_pu_ldo_VCO, 0);
					MOD_RADIO_REG_TINY(pi, PLL_CP1, core, rfpll_cp_pu, 0);
					/* Clear the VCO signal */
					MOD_RADIO_REG_TINY(pi, PLL_CFG2, core, rfpll_rst_n, 0);
					MOD_RADIO_REG_TINY(pi, PLL_VCOCAL13, core,
						rfpll_vcocal_rst_n, 0);
					MOD_RADIO_REG_TINY(pi, PLL_VCOCAL1, core,
						rfpll_vcocal_cal, 0);
					/* Power Down the mini PMU */
					MOD_RADIO_REG_TINY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 0);
					data = READ_RADIO_REG_TINY(pi, PMU_OP, core);
					data = data & 0xFF61;
					phy_utils_write_radioreg(pi, RADIO_REG_20693(pi,
						PMU_OP, core), data);
					MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_pu, 0);
					MOD_RADIO_REG_TINY(pi, VREG_CFG, core, vreg25_pu, 0);
				}
			} else {
				for (pll = 0; pll < 2; pll++) {
					/* PD the RFPLL */
					data = READ_RADIO_PLLREG_20693(pi, PLL_CFG1, pll);
					data = data & 0xE0FF;
					phy_utils_write_radioreg(pi, RADIO_PLLREG_20693(pi,
						PLL_CFG1, pll), data);
					MOD_RADIO_PLLREG_20693(pi, PLL_HVLDO1, pll,
						ldo_2p5_pu_ldo_CP, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_HVLDO1, pll,
						ldo_2p5_pu_ldo_VCO, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_CP1, pll, rfpll_cp_pu, 0);
					/* Clear the VCO signal */
					MOD_RADIO_PLLREG_20693(pi, PLL_CFG2, pll, rfpll_rst_n, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_VCOCAL13, pll,
						rfpll_vcocal_rst_n, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_VCOCAL1, pll,
						rfpll_vcocal_cal, 0);
				}
				FOREACH_CORE(pi, core) {
					/* Power Down the mini PMU */
					MOD_RADIO_REG_20693(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 0);
					data = READ_RADIO_REG_20693(pi, PMU_OP, core);
					data = data & 0xFF61;
					phy_utils_write_radioreg(pi, RADIO_REG_20693(pi,
						PMU_OP, core), data);
					MOD_RADIO_PLLREG_20693(pi, BG_CFG1, core, bg_pu, 0);
					MOD_RADIO_REG_20693(pi, VREG_CFG, core, vreg25_pu, 0);
				}
			}
			if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
				((phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1))) {
				/* Powerdown xtal ldo for MAC_Core = 1 in case of RSDB mode */
				MOD_RADIO_REG_TINY(pi, PLL_XTALLDO1, 0,
					ldo_1p2_xtalldo1p2_BG_pu, 0);
			} else if ((phy_get_phymode(pi) != PHYMODE_RSDB)) {
				/* Powerdown xtal ldo for Core 1 in case of MIMO and 80+80 */
				MOD_RADIO_REG_TINY(pi, PLL_XTALLDO1, 1,
					ldo_1p2_xtalldo1p2_BG_pu, 0);
			}
		}

		/* 4364 dual phy. when 3x3 is powered down, make sure the 1x1 femctrl lines */
		/* do not toggle ant0/ant1 this is done by giving bt the control on ant0/ant1 */
		/* If bt is not there (bt_reg_on 0) this is fine. Ultimately bt firmware needs */
		/* to write proper values in the bt_clb_upi registers bt2clb_swctrl_smask_bt_antX */
		if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) && IS_4364_3x3(pi)) {
			si_gci_set_femctrl_mask_ant01(pi->sh->sih, pi->sh->osh, 0);
		}

		/* Turn off the mini PMU enable on all cores when going down.
		 * Will be powered up in the UP sequence
		 */
		if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
			if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
				FOREACH_CORE(pi, core)
				        MOD_RADIO_REGC(pi, GE32_PMU_OP, core, wlpmu_en, 0);
			}
		}

		WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
		WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0x1);

		if (CHIPID(pi->sh->chip) == BCM4335_CHIP_ID) {
			ACPHY_REG_LIST_START
				/* WAR for XTAL power up issues */
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_xtal_pu_corebuf_pfd, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_pfddrv, 0)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_corebuf_pfd, 0)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_BT, 0)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 0)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_caldrv, 0)
			ACPHY_REG_LIST_EXECUTE(pi);
		}

		/* Ref JIRA64559 for more details. Summary of the issue - In "wl down" path */
		/* AFE_CLK_DIV block was not being turned off, but the */
		/* preceding blocks - RF_LDO and RF_PLL were turned off. So, the output of RF_PLL */
		/* is in undefined state and with that as input, the output of AFE_CLK_DIV block */
		/* would also be in undefined state. The output clock of this block was causing */
		/* BT sensitivity degradation */
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_sel_div_ovr, 1);
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_sel_div, 0x0);
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_en_ovr, 1);
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_en, 0x0);
		}

		if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
			FOREACH_CORE(pi, core) {
				if (core == 0) {
					ACPHY_REG_LIST_START
					MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG10, 0,
						bg_wlpmu_vref_sel, 0x0)
					MOD_RADIO_REG_20694_ENTRY(pi, RF, PMU_CFG4, 0,
						wlpmu_en, 0)
					MOD_RADIO_REG_20694_ENTRY(pi, RFP, LDO1P8_STAT, 0,
						ldo1p8_pu, 0)
					MOD_RADIO_REG_20694_ENTRY(pi, RF, LDO1P65_STAT,
						0, ldo1p6_pu, 0)
					MOD_RADIO_REG_20694_ENTRY(pi, RF, PMU_CFG4,
						0, wlpmu_LDO2P2_pu, 0)
					ACPHY_REG_LIST_EXECUTE(pi);
				} else {
					ACPHY_REG_LIST_START
					MOD_RADIO_REG_20694_ENTRY(pi, RFP, BG_REG10, 0,
						bg_wlpmu_vref_sel, 0x0)
					MOD_RADIO_REG_20694_ENTRY(pi, RF, PMU_CFG4, 1,
						wlpmu_en, 0)
					MOD_RADIO_REG_20694_ENTRY(pi, RFP, LDO1P8_STAT, 0,
						ldo1p8_pu, 0)
					MOD_RADIO_REG_20694_ENTRY(pi, RF, LDO1P65_STAT,
						1, ldo1p6_pu, 0)
					MOD_RADIO_REG_20694_ENTRY(pi, RF, PMU_CFG4,
						1, wlpmu_LDO2P2_pu, 0)
					ACPHY_REG_LIST_EXECUTE(pi);
				}
			}
			WRITE_RADIO_REG_20694(pi, RFP, PLL_HVLDO1, 0, 0);
			MOD_RADIO_REG_20694(pi, RFP, XTAL6, 0, xtal_pu_caldrv, 0);
			MOD_RADIO_REG_20694(pi, RFP, XTAL5, 0, xtal_pu_RCCAL, 0);
		}

		if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
			/* Turn off 2G and 5G PLL */
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL2G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, PLL5G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5G, 0, 0);
			wlc_phy_28nm_radio_pll_logen_pupd_seq(pi, LOGEN5GTO2G, 0, 0);

			/* Turn off minipmu */
			wlc_phy_radio20695_minipmu_pupd_seq(pi, 0);
		}

		/* These register turn on AFE_CLK_DIV block in "wl down" path. Leaving the block
		 * turned on was causing BT BER issue. RB: 46565.
		 */
		if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
			WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0xf);
		} else if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
			WRITE_PHYREG(pi, AfeClkDivOverrideCtrlN0, 0x1);
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) && !ACMINORREV_0(pi)) {
			ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0x1);
			WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x408);
		} else if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
			ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0x1);
			WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x8);
		}
	}

	if ((CHIPID(pi->sh->chip) == BCM4335_CHIP_ID &&
	     !CST4335_CHIPMODE_USB20D(pi->sh->sih->chipst)) ||
	    (BCM4350_CHIP(pi->sh->chip) &&
	     !CST4350_CHIPMODE_HSIC20D(pi->sh->sih->chipst))) {
		/* Power down HSIC */
		ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
			MOD_RADIO_REG(pi, RFP,  GE16_PLL_XTAL2, xtal_pu_HSIC, 0x0);
		}
	} else if ((CHIPID(pi->sh->chip) == BCM4345_CHIP_ID &&
	           !CST4345_CHIPMODE_USB20D(pi->sh->sih->chipst))) {
		/* Power down HSIC */
		ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20691_ID));
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20691(pi, PLL_XTAL2, core, xtal_pu_HSIC, 0x0);
			MOD_RADIO_REG_20691(pi, PLL_XTAL_OVR1, core, ovr_xtal_pu_HSIC, 0x1);
		}
	}

#ifdef LOW_TX_CURRENT_SETTINGS_2G
/* TODO: iPa low power in 2G - check if condition is needed) */
	if ((CHSPEC_IS2G(pi->radio_chanspec) && (pi->sromi->extpagain2g == 2)) ||
		(CHSPEC_IS5G(pi->radio_chanspec) && (pi->sromi->extpagain5g == 2))) {
		is_ipa = 1;
	} else {
		is_ipa = 0;
	}

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) && (is_ipa == 0)) {
		PHY_TRACE(("Modifying PA Bias settings for lower power for 2G!\n"));
		MOD_RADIO_REG(pi, RFX, PA2G_IDAC2, pa2g_biasa_main, 0x36);
		MOD_RADIO_REG(pi, RFX, PA2G_IDAC2, pa2g_biasa_aux, 0x36);
	}

#endif /* LOW_TX_CURRENT_SETTINGS_2G */
}

static void
wlc_phy_radio2069_mini_pwron_seq_rev16(phy_info_t *pi)
{
	uint8 cntr = 0;

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, wlpmu_en, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, VCOldo_pu, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, TXldo_pu, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, AFEldo_pu, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, RXldo_pu, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

	OSL_DELAY(100);

	ACPHY_REG_LIST_START
		/* WAR for XTAL power up issues */
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_xtal_pu_corebuf_pfd, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_pfddrv, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_BT, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_caldrv, 1)

		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, synth_pwrsw_en, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, wlpmu_ldobg_clk_en, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, ldoref_start_cal, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (!ISSIM_ENAB(pi->sh->sih)) {
		while (READ_RADIO_REGFLD(pi, RFP, GE16_PMU_STAT, ldobg_cal_done) == 0) {
			OSL_DELAY(100);
			cntr++;
			if (cntr > 100) {
				PHY_ERROR(("PMU cal Fail \n"));
				break;
			}
		}
	}

	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, ldoref_start_cal, 0);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, wlpmu_ldobg_clk_en, 0);
}

static void
wlc_phy_radio2069_mini_pwron_seq_rev32(phy_info_t *pi)
{

	uint8 cntr = 0;
	phy_utils_write_radioreg(pi, RFX_2069_GE32_PMU_OP, 0x9e);

	OSL_DELAY(100);

	ACPHY_REG_LIST_START
		WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_PMU_OP, 0xbe)

		WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_PMU_OP, 0x20be)
		WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_PMU_OP, 0x60be)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (!ISSIM_ENAB(pi->sh->sih)) {
		while (READ_RADIO_REGFLD(pi, RF0, GE32_PMU_STAT, ldobg_cal_done) == 0 ||
		       READ_RADIO_REGFLD(pi, RF1, GE32_PMU_STAT, ldobg_cal_done) == 0) {

			OSL_DELAY(100);
			cntr++;
			if (cntr > 100) {
				PHY_ERROR(("PMU cal Fail ...222\n"));
				break;
			}
		}
	}
	phy_utils_write_radioreg(pi, RFX_2069_GE32_PMU_OP, 0xbe);
}

static void
wlc_phy_radio2069_pwron_seq(phy_info_t *pi)
{
	/* Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu
	   So, to make rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1
	*/
	uint16 txovrd = READ_PHYREG(pi, RfctrlCoreTxPus0);
	uint16 rfctrlcmd = READ_PHYREG(pi, RfctrlCmd) & 0xfc38;

	ACPHY_REG_LIST_START
		/* Using usleep of 100us below, so don't need these */
		WRITE_PHYREG_ENTRY(pi, Pllldo_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Rfpll_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Logen_AfeDiv_reset, 0x2000)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, txovrd & 0x7e7f);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, READ_PHYREG(pi, RfctrlOverrideTxPus0) | 0x180);

	/* ***  Start Radio rfpll pwron seq  ***
	   Start with chip_pu = 0, por_reset = 0, rfctrl_bundle_en = 0
	*/
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd);

	/* Toggle jtag reset (not required for uCode PM) */
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 1);
	OSL_DELAY(1);
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 0);

	/* Update preferred values (not required for uCode PM) */
	wlc_phy_radio2069_upd_prfd_values(pi);

	/* Toggle radio_reset (while radio_pu = 1) */
	MOD_RADIO_REG(pi, RF2, VREG_CFG, bg_filter_en, 0);   /* radio_reset = 1 */
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 6);   /* radio_pwrup = 1, rfpll_pu = 0 */
	OSL_DELAY(100);                                      /* radio_reset to be high for 100us */
	MOD_RADIO_REG(pi, RF2, VREG_CFG, bg_filter_en, 1);   /* radio_reset = 0 */

	/* {rfpll, pllldo, logen}_{pu, reset} pwron seq */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 2);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, txovrd | 0x180);
	OSL_DELAY(100);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, txovrd & 0xfeff);
}

static void
wlc_phy_tiny_radio_pwron_seq(phy_info_t *pi)
{
	ASSERT(TINY_RADIO(pi));
	/* Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu
	   So, to make rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1
	*/

	/* # power down everything */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);
	} else {
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, 0)
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideTxPus0, 0x3ff)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	ACPHY_REG_LIST_START
		/* Using usleep of 100us below, so don't need these */
		WRITE_PHYREG_ENTRY(pi, Pllldo_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Rfpll_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Logen_AfeDiv_reset, 0x2000)

		/* Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0xd)

		/* # Reset radio, jtag */
		/* NOTE: por_force doesnt have any effect in 4349A0 and */
		/* radio jtag reset is taken care by the PMU */
		WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x7)
	ACPHY_REG_LIST_EXECUTE(pi);
	/* # radio_reset to be high for 100us */
	OSL_DELAY(100);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/* Update preferred values (not required for uCode PM) */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
		wlc_phy_radio20691_upd_prfd_values(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		wlc_phy_radio20693_upd_prfd_values(pi);
	}
	ACPHY_REG_LIST_START
		/* # {rfpll, pllldo, logen}_{pu, reset} */
		/* # pllldo_{pu, reset} = 1, rfpll_reset = 1 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0xd)
		/* # {radio, rfpll}_pwrup = 1 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x2)

		/* # logen_{pwrup, reset} = 1 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, 0x180)
	ACPHY_REG_LIST_EXECUTE(pi);
		OSL_DELAY(100); /* # resets to be on for 100us */
	ACPHY_REG_LIST_START
		/* # {pllldo, rfpll}_reset = 0 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0x4)
	ACPHY_REG_LIST_EXECUTE(pi);
		/* # logen_reset = 0 */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
	} else {
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, 0x80)
			/* # leave overrides for logen_{pwrup, reset} */
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideTxPus0, 0x180)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

#define MAX_2069_RCAL_WAITLOOPS 100
/* rcal takes ~50us */
static void
wlc_phy_radio2069_rcal(phy_info_t *pi)
{
	uint8 done, rcal_val, core;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ACPHY_REG_LIST_START
		/* Power-up rcal clock (need both of them for rcal) */
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1)

		/* Rcal can run with 40mhz cls, no diving */
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL1, 0)
	ACPHY_REG_LIST_EXECUTE(pi);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* Make connection with the external 10k resistor */
	/* Turn off all test points in cgpaio block to avoid conflict */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_CGPAIO_CFG1, core, cgpaio_pu, 1);
		}
		ACPHY_REG_LIST_START
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG2, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG3, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG4, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG5, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE1, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE2, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE4, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE6, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_ENTRY(pi, RF2, CGPAIO_CFG1, cgpaio_pu, 1)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG2, 0)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG3, 0)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG4, 0)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG5, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
	/* NOTE: xtal_pu, xtal_buf_pu & xtalldo_pu direct control lines should be(& are) ON */

	/* Toggle the rcal pu for calibration engine */
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 0);
	OSL_DELAY(1);
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 1);

	/* Wait for rcal to be done, max = 10us * 100 = 1ms  */
	done = 0;


	SPINWAIT(READ_RADIO_REGFLD(pi, RF2, RCAL_CFG,
		i_wrf_jtag_rcal_valid) == 0, ACPHY_SPINWAIT_RCAL_STATUS);

	done = READ_RADIO_REGFLD(pi, RF2, RCAL_CFG, i_wrf_jtag_rcal_valid);

	if (done == 0) {
		PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR : RCAL Failed", __FUNCTION__));
		PHY_FATAL_ERROR(pi, PHY_RC_RCAL_INVALID);
	}

	/* Status */
	rcal_val = READ_RADIO_REGFLD(pi, RF2, RCAL_CFG, i_wrf_jtag_rcal_value);
	rcal_val = rcal_val >> 1;
	PHY_INFORM(("wl%d: %s rcal=%d\n", pi->sh->unit, __FUNCTION__, rcal_val));
#ifdef ATE_BUILD
	/* Same RCAL value for all 3 cores, hence for logging forcing core=0 */
	wl_ate_set_buffer_regval(RCAL_VALUE, rcal_val,
			0, phy_get_current_core(pi), pi->sh->chip);
#endif

	/* Valid range of values for rcal */
	ASSERT((rcal_val > 0) && (rcal_val < 15));

	/*  Power down blocks not needed anymore */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_CGPAIO_CFG1, core, cgpaio_pu, 0);
		}
	} else {
		MOD_RADIO_REG(pi, RF2, CGPAIO_CFG1, cgpaio_pu, 0);
	}

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0)
		MOD_RADIO_REG_ENTRY(pi, RF2, RCAL_CFG, pu, 0)
	ACPHY_REG_LIST_EXECUTE(pi);
}

/* rccal takes ~3ms per, i.e. ~9ms total */
static void
wlc_phy_radio2069_rccal(phy_info_t *pi)
{
	uint8 cal, core, done, rccal_val[NUM_2069_RCCAL_CAPS];
	uint16 rccal_itr, n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x0, 0x0};
	uint8 sc[] = {0x0, 0x2, 0x1};
	uint8 x1[] = {0x1c, 0x70, 0x40};
	uint16 trc[] = {0x14a, 0x101, 0x11a};
	uint16 gmult_const = 193;

	phy_ac_radio_data_t *data = &(pi->u.pi_acphy->radioi->data);

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));


	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
		if PHY_XTAL_IS40M(pi) {
		  if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
		                      (RADIO2069REV(pi->pubpi->radiorev) == 26)) {
			gmult_const = 70;
			trc[0] = 0x294;
			trc[1] = 0x202;
			trc[2] = 0x214;
		  } else {
			gmult_const = 160;
		  }
		} else if (PHY_XTAL_IS37M4(pi)) {
		  if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
		                      (RADIO2069REV(pi->pubpi->radiorev) == 26)) {
			gmult_const = 77;
			trc[0] = 0x45a;
			trc[1] = 0x1e0;
			trc[2] = 0x214;
		  } else {
			gmult_const = 158;
			trc[0] = 0x22d;
			trc[1] = 0xf0;
			trc[2] = 0x10a;
		  }
		} else if (PHY_XTAL_IS52M(pi)) {
			if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
				(RADIO2069REV(pi->pubpi->radiorev) == 26)) {
				gmult_const = 77;
				trc[0] = 0x294;
				trc[1] = 0x202;
				trc[2] = 0x214;
			  } else {
				gmult_const = 160;
				trc[0] = 0x14a;
				trc[1] = 0x101;
				trc[2] = 0x11a;
			  }
		} else {
			gmult_const = 160;
		}

	} else {
		gmult_const = 193;
		if ((RADIO2069REV(pi->pubpi->radiorev) == 64) ||
			(RADIO2069REV(pi->pubpi->radiorev) == 66)) {
			gmult_const = 216;
			trc[0] = 0x22d;
			trc[1] = 0xf0;
			trc[2] = 0x10a;
		}
	}

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL, 2);

	/* Calibrate lpf, adc, dacbuf */
	for (cal = 0; cal < NUM_2069_RCCAL_CAPS; cal++) {
		/* Setup */
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, sr, sr[cal]);
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, sc, sc[cal]);
		MOD_RADIO_REG(pi, RF2, RCCAL_LOGIC1, rccal_X1, x1[cal]);
		phy_utils_write_radioreg(pi, RF2_2069_RCCAL_TRC, trc[cal]);

		/* For dacbuf force fixed dacbuf cap to be 0 while calibration, restore it later */
		if (cal == 2) {
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REGC(pi, DAC_CFG2, core, DACbuf_fixed_cap, 0);
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
					MOD_RADIO_REGC(pi, GE16_OVR22, core,
					               ovr_afe_DACbuf_fixed_cap, 1);
				} else {
					MOD_RADIO_REGC(pi, OVR21, core,
					               ovr_afe_DACbuf_fixed_cap, 1);
				}
			}
		}

		/* Toggle RCCAL power */
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG(pi, RF2, RCCAL_LOGIC1, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
			(rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
			rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC2, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG(pi, RF2, RCCAL_LOGIC1, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		if (cal == 0) {
			/* lpf */
			n0 = READ_RADIO_REG(pi, RF2, RCCAL_LOGIC3);
			n1 = READ_RADIO_REG(pi, RF2, RCCAL_LOGIC4);
			/* gmult = (30/40) * (n1-n0) = (193 * (n1-n0)) >> 8 */
			rccal_val[cal] = (gmult_const * (n1 - n0)) >> 8;
			data->rccal_gmult = rccal_val[cal];
			data->rccal_gmult_rc = data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_LPF, data->rccal_gmult, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif /* ATE_BUILD */
		} else if (cal == 1) {
			/* adc */
			rccal_val[cal] = READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC5, rccal_raw_adc1p2);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_ADC, rccal_val[cal], -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif /* ATE_BUILD */

			/* don't change this loop to active core loop,
			   gives slightly higher floor, why?
			*/
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_4_0, rccal_val[cal]);
				MOD_RADIO_REGC(pi, TIA_CFG3, core, rccal_hpc, rccal_val[cal]);
			}
			/* Store value, might be overriden upon channel change */
			pi->u.pi_acphy->radioi->rccal_adc_rc = rccal_val[cal];
		} else {
			/* dacbuf */
			rccal_val[cal] = READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC5, rccal_raw_dacbuf);
			data->rccal_dacbuf = rccal_val[cal];

			/* take away the override on dacbuf fixed cap */
			FOREACH_CORE(pi, core) {
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
					MOD_RADIO_REGC(pi, GE16_OVR22, core,
					               ovr_afe_DACbuf_fixed_cap, 0);
				} else {
					MOD_RADIO_REGC(pi, OVR21, core,
					               ovr_afe_DACbuf_fixed_cap, 0);
				}
			}
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(RCCAL_DACBUF, data->rccal_dacbuf, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif /* ATE_BUILD */
		}

		/* Turn off rccal */
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, pu, 0);
	}

	/* Powerdown rccal driver */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
}

static void
wlc_phy_switch_radio_acphy_20691(phy_info_t *pi)
{

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20691_ID));

	ACPHY_REG_LIST_START
		/* 20691_mini_pwron_seq_phyregs_rev12() */
		/* # mini PMU init */
		/* #Step 1 */
		MOD_RADIO_REG_20691_ENTRY(pi, PMU_OP, 0, wlpmu_en, 1)
		MOD_RADIO_REG_20691_ENTRY(pi, PMU_OP, 0, wlpmu_VCOldo_pu, 1)
		MOD_RADIO_REG_20691_ENTRY(pi, PMU_OP, 0, wlpmu_TXldo_pu, 1)
		MOD_RADIO_REG_20691_ENTRY(pi, PMU_OP, 0, wlpmu_AFEldo_pu, 1)
		MOD_RADIO_REG_20691_ENTRY(pi, PMU_OP, 0, wlpmu_RXldo_pu, 1)
	ACPHY_REG_LIST_EXECUTE(pi);
	/* #Step 2 */
	OSL_DELAY(200);

	ACPHY_REG_LIST_START
		/* # DAC */
		MOD_RADIO_REG_20691_ENTRY(pi, CLK_DIV_CFG1, 0, dac_driver_size, 8)
		MOD_RADIO_REG_20691_ENTRY(pi, CLK_DIV_CFG1, 0, sel_dac_div, 3)
		MOD_RADIO_REG_20691_ENTRY(pi, TX_DAC_CFG5, 0, DAC_invclk, 1)
		MOD_RADIO_REG_20691_ENTRY(pi, TX_DAC_CFG5, 0, DAC_pd_mode, 0)
		/* # improve settling behavior, need for level tracking */
		MOD_RADIO_REG_20691_ENTRY(pi, BG_CFG1, 0, bg_pulse, 1)
		MOD_RADIO_REG_20691_ENTRY(pi, TX_DAC_CFG1, 0, DAC_scram_off, 1)

		/* # ADC */
		MOD_RADIO_REG_20691_ENTRY(pi, PMU_CFG4, 0, wlpmu_ADCldo_pu, 1)

		/* 20691_pmu_pll_pwrup */
		MOD_RADIO_REG_20691_ENTRY(pi, VREG_CFG, 0, vreg25_pu, 1) /* #powerup vreg2p5 */
		MOD_RADIO_REG_20691_ENTRY(pi, BG_CFG1, 0, bg_pu, 1) /* #powerup bandgap */

		/* # powerup pll */
		/* # VCO */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_CFG1, 0, rfpll_vco_pu, 1)
		/* # Enable override */
		MOD_RADIO_REG_20691_ENTRY(pi, RFPLL_OVR1, 0, ovr_rfpll_vco_pu, 1)
		/* # VCO LDO */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO1, 0, ldo_2p5_pu_ldo_VCO, 1)
		/* # Enable override */
		MOD_RADIO_REG_20691_ENTRY(pi, RFPLL_OVR1, 0, ovr_ldo_2p5_pu_ldo_VCO, 1)
		/* # VCO buf */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_CFG1, 0, rfpll_vco_buf_pu, 1)
		/* # Enable override */
		MOD_RADIO_REG_20691_ENTRY(pi, RFPLL_OVR1, 0, ovr_rfpll_vco_buf_pu, 1)
		/* # Synth global */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_CFG1, 0, rfpll_synth_pu, 1)
		/* # Enable override */
		MOD_RADIO_REG_20691_ENTRY(pi, RFPLL_OVR1, 0, ovr_rfpll_synth_pu, 1)
		/* # Charge Pump */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_CP1, 0, rfpll_cp_pu, 1)
		/* # Enable override */
		MOD_RADIO_REG_20691_ENTRY(pi, RFPLL_OVR1, 0, ovr_rfpll_cp_pu, 1)
		/* # Charge Pump LDO */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO1, 0, ldo_2p5_pu_ldo_CP, 1)
		/* # Enable override */
		MOD_RADIO_REG_20691_ENTRY(pi, RFPLL_OVR1, 0, ovr_ldo_2p5_pu_ldo_CP, 1)
		/* # PLL lock monitor */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_CFG1, 0, rfpll_monitor_pu, 1)

		/* Nishant's hack to get PLL lock: */
		/* spare 127 (bit 15) is xtal_dcc_clk_sel */
		/* actually controls the inrush current limit on xgtal ldo */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL12, 0, xtal_dcc_clk_sel, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* minipmu_cal */
	wlc_phy_tiny_radio_minipmu_cal(pi);

	/* r cal */
#ifdef ATE_BUILD
	/* ATE firmware performs the rcal and the value is put in the OTP. */
	wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE3_BOTH_CORE_CAL);
#else
	if (((RADIO20691_MAJORREV(pi->pubpi->radiorev) == 1) &&
		(RADIO20691_MINORREV(pi->pubpi->radiorev) >= 5) &&
		((RADIO20691_MINORREV(pi->pubpi->radiorev) != 16) &&
		(RADIO20691_MINORREV(pi->pubpi->radiorev) != 17))) ||
		(RADIO20691_MAJORREV(pi->pubpi->radiorev) > 1)) {
		/* 4345c0 and after or 43909 */
		/* use OTP if available */
		wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE0_OTP);
	} else {
		/* hard-coded RCal value */
		wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE1_STATIC);
	}
#endif /* ATE_BUILD */
}

static void
wlc_phy_radio2069_upd_prfd_values(phy_info_t *pi)
{
	uint8 core;
	const radio_20xx_prefregs_t *prefregs_2069_ptr = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	switch (RADIO2069REV(pi->pubpi->radiorev)) {
	case 3:
		prefregs_2069_ptr = prefregs_2069_rev3;
		break;
	case 4:
	case 8:
		prefregs_2069_ptr = prefregs_2069_rev4;
		break;
	case 7:
		prefregs_2069_ptr = prefregs_2069_rev4;
		break;
	case 16:
		prefregs_2069_ptr = prefregs_2069_rev16;
		break;
	case 17:
		prefregs_2069_ptr = prefregs_2069_rev17;
		break;
	case 18:
		prefregs_2069_ptr = prefregs_2069_rev18;
		break;
	case 23:
		prefregs_2069_ptr = prefregs_2069_rev23;
		break;
	case 24:
		prefregs_2069_ptr = prefregs_2069_rev24;
		break;
	case 25:
		prefregs_2069_ptr = prefregs_2069_rev25;
		break;
	case 26:
		prefregs_2069_ptr = prefregs_2069_rev26;
		break;
	case 32:
	case 33:
	case 34:
	case 35:
	case 37:
	case 38:
		prefregs_2069_ptr = prefregs_2069_rev33_37;
		break;
	case 39:
	case 40:
	case 44:
		prefregs_2069_ptr = prefregs_2069_rev39;
		break;
	case 36:
		prefregs_2069_ptr = prefregs_2069_rev36;
		break;
	case 64:
	case 66:
		prefregs_2069_ptr = prefregs_2069_rev64;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
		ASSERT(0);
		return;
	}

	/* Update preferred values */
	wlc_phy_init_radio_prefregs_allbands(pi, prefregs_2069_ptr);

	if (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) &
		BFL_SROM11_WLAN_BT_SH_XTL) {
		MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_BT, 1);
	}

	/* **** NOTE : Move the following to XLS (whenever possible) *** */

	/* Reg conflict with 2069 rev 16 */
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_rst_n, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_en_vcocal, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR16, ovr_rfpll_vcocal_rstn, 1)

			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_cal_rst_n, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_pll_pu, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_vcocal_cal, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else {
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
			ACPHY_REG_LIST_START
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_en_vcocal, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR17, ovr_rfpll_vcocal_rstn, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_rst_n, 1)

				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_cal_rst_n, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_pll_pu, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_vcocal_cal, 1)
			ACPHY_REG_LIST_EXECUTE(pi);
		}

		/* Until this is moved to XLS (give bg_filter_en control to radio) */
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			MOD_RADIO_REG(pi, RFP, GE32_OVR1, ovr_vreg_bg_filter_en, 1);
		}

		/* Ensure that we read the values that are actually applied */
		/* to the radio block and not just the radio register values */

		phy_utils_write_radioreg(pi, RFX_2069_GE16_READOVERRIDES, 1);
		MOD_RADIO_REG(pi, RFP, GE16_READOVERRIDES, read_overrides, 1);

		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			/* For VREG power up in radio jtag as there is a bug in the digital
			 * connection
			 */
			if (RADIO2069_MINORREV(pi->pubpi->radiorev) < 5) {
				MOD_RADIO_REG(pi, RF2, VREG_CFG, pup, 1);
				MOD_RADIO_REG(pi, RFP, GE32_OVR1, ovr_vreg_pup, 1);
			}

			/* This OVR enable is required to change the value of
			 * reg(RFP_pll_xtal4.xtal_outbufstrg) and used the value from the
			 * jtag. Otherwise the direct control from the chip has a fixed
			 * non-programmable value!
			 */
			MOD_RADIO_REG(pi, RFP, GE16_OVR27, ovr_xtal_outbufstrg, 1);
		}
	}

	/* Jira: CRDOT11ACPHY-108. Force bg_pulse to be high. Need to check impact on ADC SNR. */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) < 2) {
		MOD_RADIO_REG(pi, RF2, BG_CFG1, bg_pulse, 1);
	}
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_BG_CFG1, core, bg_pulse, 1);
			MOD_RADIO_REGC(pi, GE32_OVR2, core, ovr_bg_pulse, 1);
		}
	}

	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) &&
			(((RADIO2069_MINORREV(pi->pubpi->radiorev)) == 16) ||
			(RADIO2069_MINORREV(pi->pubpi->radiorev) == 17))) {
			/* 4364 3x3 : to address XTAL spur on core2 */
				MOD_RADIO_REG(pi, RFP, PLL_XTAL8, xtal_repeater3_size, 4);
	}

	/* Give control of bg_filter_en to radio */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) < 2)
		MOD_RADIO_REG(pi, RF2, OVR2, ovr_vreg_bg_filter_en, 1);

	FOREACH_CORE(pi, core) {
		/* Fang's recommended settings */
		MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_9_8, 1);
		MOD_RADIO_REGC(pi, ADC_RC2, core, adc_ctrl_RC_17_16, 2);

		/* These should be 0, as they are controlled via direct control lines
		   If they are 1, then during 5g, they will turn on
		*/
		MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_bias_cas_pu, 0);
		MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_2gtx_pu, 0);
		MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_bias_pu, 0);
		MOD_RADIO_REGC(pi, PAD2G_CFG1, core, pad2g_pu, 0);
	}
	/* SWWLAN-39535  LNA1 clamping issue for 4360b1 and 43602a0 */
	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) &&
		((RADIO2069REV(pi->pubpi->radiorev) == 7) ||
		(RADIO2069REV(pi->pubpi->radiorev) == 8) ||
		(RADIO2069REV(pi->pubpi->radiorev) == 13))) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, RXRF5G_CFG1, core, pu_pulse, 1);
			MOD_RADIO_REGC(pi, OVR18, core, ovr_rxrf5g_pu_pulse, 1);
		}
	}
}

static void
wlc_phy_radio20691_upd_prfd_values(phy_info_t *pi)
{
	const radio_20xx_prefregs_t *prefregs_20691_ptr = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20691_ID));

	/* Choose the right table to use */
	switch (RADIO20691REV(pi->pubpi->radiorev)) {
	case 60:
	case 68:
		prefregs_20691_ptr = prefregs_20691_rev68;
		break;
	case 75:
		prefregs_20691_ptr = prefregs_20691_rev75;
		break;
	case 79:
		prefregs_20691_ptr = prefregs_20691_rev79;
		break;
	case 74:
	case 82:
		prefregs_20691_ptr = prefregs_20691_rev82;
		break;
	case 85:
	case 86:
	case 87:
	case 88:
		prefregs_20691_ptr = prefregs_20691_rev88;
		break;
	case 129:
	case 130:
		prefregs_20691_ptr = prefregs_20691_rev129;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO20691REV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return;
	}

	/* Update preferred values */
	wlc_phy_init_radio_prefregs_allbands(pi, prefregs_20691_ptr);

	MOD_RADIO_REG_20691(pi, LOGEN_OVR1, 0, ovr_logencore_5g_en_lowband, 0x1);
	MOD_RADIO_REG_20691(pi, LOGEN_CFG2, 0, logencore_5g_en_lowband, 0x0);

	if (!PHY_IPA(pi)) {
		ACPHY_REG_LIST_START
			/* Saeed's settings for aband epa evm tuning */
			MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG1, 0, mx5g_idac_bleed_bias, 0x1f)
			MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG5, 0, mx5g_idac_bbdc, 0x0)
			MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG5, 0, mx5g_idac_lodc, 0xf)
			MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG8, 0, pad5g_idac_gm, 0xa)
			MOD_RADIO_REG_20691_ENTRY(pi, PA5G_IDAC1, 0, pa5g_idac_main, 0x17)
			MOD_RADIO_REG_20691_ENTRY(pi, PA5G_IDAC3, 0, pa5g_idac_tuning_bias, 0xf)

			/* Utku's second setting for gband epa evm tuning */
			MOD_RADIO_REG_20691_ENTRY(pi, PA2G_IDAC2, 0, pad2g_idac_tuning_bias, 6)
			MOD_RADIO_REG_20691_ENTRY(pi, PA2G_INCAP, 0, pa2g_idac_incap_compen_main,
				0x2c)
			MOD_RADIO_REG_20691_ENTRY(pi, PA2G_IDAC1, 0, pa2g_idac_main, 0x24)
			MOD_RADIO_REG_20691_ENTRY(pi, PA2G_IDAC1, 0, pa2g_idac_cas, 0x2b)
			MOD_RADIO_REG_20691_ENTRY(pi, TXMIX2G_CFG2, 0, mx2g_idac_cascode, 0xc)
			MOD_RADIO_REG_20691_ENTRY(pi, TX_LOGEN2G_CFG1, 0, logen2g_tx_xover, 4)
			MOD_RADIO_REG_20691_ENTRY(pi, LOGEN_CFG1, 0, logencore_lcbuf_stg1_bias, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, LOGEN_CFG2, 0, logencore_lcbuf_stg2_bias, 7)
			MOD_RADIO_REG_20691_ENTRY(pi, LOGEN_CFG1, 0, logencore_lcbuf_stg1_ftune,
				0x18)

			/* A band EVM floor tuning, dfu, cfraser */
			MOD_RADIO_REG_20691_ENTRY(pi, TX_TOP_2G_OVR_EAST, 0, ovr_tx2g_bias_pu, 1)
			MOD_RADIO_REG_20691_ENTRY(pi, TX2G_CFG1, 0, tx2g_bias_pu, 1)
			MOD_RADIO_REG_20691_ENTRY(pi, TX_LPF_CFG3, 0, lpf_sel_2g_5g_cmref_gm, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG3, 0, mx5g_pu_bleed, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG8, 0, pad5g_idac_gm, 0x8)
			MOD_RADIO_REG_20691_ENTRY(pi, PA5G_INCAP, 0, pad5g_idac_pmos, 0x18)
			MOD_RADIO_REG_20691_ENTRY(pi, PA5G_IDAC1, 0, pa5g_idac_main, 0x1a)
			MOD_RADIO_REG_20691_ENTRY(pi, TXGM5G_CFG1, 0, pad5g_idac_cascode, 0xf)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	if ((RADIO20691_MAJORREV(pi->pubpi->radiorev) == 1) && PHY_IPA(pi)) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, PA2G_INCAP, 0, pa2g_idac_incap_compen_main,
				0x0)
			MOD_RADIO_REG_20691_ENTRY(pi, PA2G_IDAC1, 0, pa2g_idac_cas, 0x20)
			MOD_RADIO_REG_20691_ENTRY(pi, PA5G_IDAC1, 0, pa5g_idac_main, 0x18)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

static void
wlc_phy_radio20691_xtal_tune_prep(phy_info_t *pi)
{
	/* Enable before changing channel */
	MOD_RADIO_REG_20691(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0x1);
	MOD_RADIO_REG_20691(pi, PLL_XTAL2, 0, xtal_pu_RCCAL1, 0x1);
	MOD_RADIO_REG_20691(pi, PLL_XTAL2, 0, xtal_pu_caldrv, 0x1);
}

static void
wlc_phy_radio20691_xtal_tune(phy_info_t *pi)
{
	/* Channel has changed */

	ACPHY_REG_LIST_START
		/* Return the following to zero after channel change */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0x0)
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL1, 0x0)

		/*
		 * pll_xtal2.xtal_pu_caldrv handled differently in driver than in Tcl due to
		 * impact on VCO cal and ucode. SW4345-327
		 */

		/*
		 * These will only work with BT held in reset.
		 * Write the BT CLB register equivalent to 0x0 through backplane to get this to
		 * work in BT+WLAN mode
		 */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL_OVR1, 0, ovr_xtal_pu_corebuf_pfd, 0x1)
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL2, 0, xtal_pu_corebuf_pfd, 0x0)

		/* This will only work if BT is held at reset. CANNOT use it in either BT or
		 * BT+WLAN mode
		 */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL2, 0, xtal_pu_BT, 0x0)

		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL2, 0, xtal_pu_corebuf_bb, 0x1)

		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL4, 0, xtal_outbufBBstrg, 0x0)

		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL_OVR1, 0, ovr_xtal_coresize_pmos, 0x1)
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL_OVR1, 0, ovr_xtal_coresize_nmos, 0x1)

		/* pll_xtal3[11] (xtal_core_change) is unwired after 4345A0, don't touch */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL3, 0, xtal_refsel, 0x4)
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL3, 0, xtal_xtal_swcap_in, 0x4)
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL3, 0, xtal_xtal_swcap_out, 0xa)
	ACPHY_REG_LIST_EXECUTE(pi);
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL1, 0, xtal_coresize_pmos, 0x4)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL1, 0, xtal_coresize_nmos, 0x4)

			/* HSIC=On helps in 2G. Keep HSIC ON while reducing buffer strength to
			 * minimum
			 */
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL2, 0, xtal_pu_HSIC, 0x1)

			/* 0x3 if doubler ON, 0x1 if doubler OFF */
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL4, 0, xtal_xtbufstrg, 0x3)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL4, 0, xtal_outbufstrg, 0x2)

			/* To reduce a few of the output strengths a bit */
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL6, 0, xtal_bufstrg_HSIC, 0x0)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL6, 0, xtal_bufstrg_gci, 0x0)

			/* LDO voltage reduced to 4345B0@~1.06V, pll_xtalldo1 = 0x10e */
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_ctl, 0x8)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else if (CHSPEC_IS5G(pi->radio_chanspec)) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL1, 0, xtal_coresize_pmos, 0x10)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL1, 0, xtal_coresize_nmos, 0x10)

			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL2, 0, xtal_pu_HSIC, 0x0)

			/* doubler no concern in 5G */
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL4, 0, xtal_xtbufstrg, 0x7)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL4, 0, xtal_outbufstrg, 0x4)

			/* To reduce a few of the output strengths a bit */
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL6, 0, xtal_bufstrg_HSIC, 0x4)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTAL6, 0, xtal_bufstrg_gci, 0x4)

			/* LDO voltage reduced to 4345B0@~1.06V, pll_xtalldo1 = 0x10e */
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_ctl, 0x8)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

static void
wlc_2069_rfpll_150khz(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF4, rfpll_lf_lf_r1, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF4, rfpll_lf_lf_r2, 2)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 2)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF7, rfpll_lf_lf_rs_cm, 2)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF7, rfpll_lf_lf_rf_cm, 0xff)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xffff)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xffff)
	ACPHY_REG_LIST_EXECUTE(pi);
}

static void
wlc_phy_2069_4335_set_ovrds(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	ACPHY_REG_LIST_START
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR30, 0x1df3)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR31, 0x1ffc)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR32, 0x0078)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		phy_utils_write_radioreg(pi, RF0_2069_GE16_OVR28, 0x0);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_OVR29, 0x0);
	} else {
		phy_utils_write_radioreg(pi, RF0_2069_GE16_OVR28, 0xffff);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_OVR29, 0xffff);
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1)
			if (PHY_IPA(pi))
			    phy_utils_write_radioreg(pi, RFP_2069_GE16_OVR29, 0x6900);
	}
}

static void
wlc_phy_2069_4350_set_ovrds(phy_ac_radio_info_t *ri)
{
	uint8 core, afediv_size, afeldo1;
	phy_info_t *pi = ri->pi;
	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(pi->radio_chanspec),
		CHSPEC_IS2G(pi->radio_chanspec) ? WF_CHAN_FACTOR_2_4_G
		: WF_CHAN_FACTOR_5_G);
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	ACPHY_REG_LIST_START
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR30, 0x1df3)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR31, 0x1ffc)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR32, 0x0078)

		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_CP, 0x1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_VCO, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);
	if (PHY_IPA(pi)&&(pi->xtalfreq == 37400000)&&CHSPEC_IS2G(pi->radio_chanspec)) {
		FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
			MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_bias_reset, 1);
			MOD_RADIO_REGC(pi, GE16_OVR13, core, ovr_pa2g_bias_reset, 1);
			}
	}
	if ((RADIOREV(pi->pubpi->radiorev) == 36) || (RADIOREV(pi->pubpi->radiorev) >= 39)) {
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, radio_logen2g, idac_qb, 0x2)
			MOD_PHYREG_ENTRY(pi, radio_logen2gN5g, idac_itx, 0x3)
			MOD_PHYREG_ENTRY(pi, radio_logen2gN5g, idac_irx, 0x3)
			MOD_PHYREG_ENTRY(pi, radio_logen2gN5g, idac_qrx, 0x3)
			MOD_PHYREG_ENTRY(pi, radio_logen2g, idac_qtx, 0x3)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CFG4, rfpll_spare2, 0x6)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CFG4, rfpll_spare3, 0x34)

			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_TOP_SPARE7, 0x1)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF1, 0x48)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
	if (!PHY_IPA(pi)) {
		if (pi->xtalfreq == 37400000) {
			MOD_RADIO_REG(pi, RFP, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xb);
		}
		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			if (CHSPEC_IS80(pi->radio_chanspec)) {
				switch (fc) {
				case 5690:
					afediv_size = 0xf;
					afeldo1 = 0x7;
					break;
				case 5775:
					afediv_size = 0xf;
					afeldo1 = 0x0;
					break;
				case 5210:
				case 5290:
					afediv_size = 0x8;
					afeldo1 = 0x0;
					break;
				default:
					afediv_size = 0xB;
					afeldo1 = 0x7;
				}
				MOD_RADIO_REG(pi, RFP, GE16_AFEDIV1,
				    afediv_main_driver_size, afediv_size);
				MOD_RADIO_REG(pi, RF1, GE32_PMU_CFG2, AFEldo_adj, afeldo1);
			} else {
				MOD_RADIO_REG(pi, RFP, GE16_AFEDIV1, afediv_main_driver_size,
				     (fc == 5190) ? 0xa : 0x8);
				MOD_RADIO_REG(pi, RF1, GE32_PMU_CFG2, AFEldo_adj, 0x7);
			}
			/* Override PAD gain for core 0 to be 255 */
			MOD_RADIO_REG(pi, RF0, GE16_OVR14, ovr_pad5g_gc, 0x1);
			MOD_RADIO_REG(pi, RF0, PAD5G_CFG1, gc, 0x7F);
			if (ri->data.srom_txnospurmod5g == 1) {
				/* Override PAD gain for core 1 to be 255 */
				MOD_RADIO_REG(pi, RF1, GE16_OVR14, ovr_pad5g_gc, 0x1);
				MOD_RADIO_REG(pi, RF1, PAD5G_CFG1, gc, 0x7F);
			} else {
				ACPHY_REG_LIST_START
					MOD_RADIO_REG_ENTRY(pi, RF1, PAD5G_IDAC, idac_main, 0x28)
					MOD_RADIO_REG_ENTRY(pi, RF1, PAD5G_TUNE, idac_aux, 0x28)
					MOD_RADIO_REGC_ENTRY(pi, PAD5G_INCAP, 1,
						idac_incap_compen_main, 0x8)
					MOD_RADIO_REGC_ENTRY(pi, PAD5G_INCAP, 1,
						idac_incap_compen_aux, 0x8)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
			if (pi->fabid == 4) {
				/* UMC tuning for 5G EVM */
				FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
					MOD_RADIO_REGC(pi, PAD5G_IDAC, core, idac_main, 0x2);
					MOD_RADIO_REGC(pi, PAD5G_TUNE, core, idac_aux, 0x2a);
					MOD_RADIO_REGC(pi, PAD5G_INCAP, core,
						idac_incap_compen_main, 5);
					MOD_RADIO_REGC(pi, PAD5G_INCAP, core,
						idac_incap_compen_aux, 5);
					MOD_RADIO_REGC(pi, GE16_OVR14, core,
						ovr_pga5g_gainboost, 1);
					MOD_RADIO_REGC(pi, PGA5G_CFG1, core, gainboost, 0xf);
				}
			}
		} else {
			ACPHY_REG_LIST_START
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_AFEDIV1,
					afediv_main_driver_size, 0x8)
				MOD_RADIO_REG_ENTRY(pi, RF1, GE32_PMU_CFG2, AFEldo_adj, 0x0)
			ACPHY_REG_LIST_EXECUTE(pi);
			if ((ri->data.srom_txnospurmod2g == 0) && (pi->xtalfreq == 37400000)) {
				if (fc == 2412) {
				    ACPHY_REG_LIST_START
					/* setting 200khz loopbw */
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xFFD4)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xF3F9)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xA)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xA)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xC65)
				    ACPHY_REG_LIST_EXECUTE(pi);
				}
				if (fc == 2467) {
				    ACPHY_REG_LIST_START
					/* setting 200khz loopbw */
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xFDCF)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xEDF3)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xC68)
				    ACPHY_REG_LIST_EXECUTE(pi);
				}
			}
		}
	} else if ((PHY_IPA(pi)) && (CHSPEC_IS80(pi->radio_chanspec))) {
		MOD_RADIO_REG(pi, RFP, GE16_AFEDIV1,
		    afediv_main_driver_size, 0xb);
	}
	/* set xtal_pu_RCCAL1 to 0 by default to avoid it staying high without rcal */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 0);
}

static void
wlc_phy_chanspec_radio20691_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset)
{
	const chan_info_radio20691_t *ci20691;
	uint8 itr = 0;
	uint8 ovr_rxdiv2g_rs = 0;
	uint8 ovr_rxdiv2g_pu_bias = 0;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20691_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20691(pi, ch, &ci20691) < 0)
		return;

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		/* save the ovr_rxdiv2g radio registers */
		ovr_rxdiv2g_rs = READ_RADIO_REGFLD_20691(pi, RX_TOP_2G_OVR_EAST, 0, ovr_rxdiv2g_rs);
		ovr_rxdiv2g_pu_bias = READ_RADIO_REGFLD_20691(pi, RX_TOP_2G_OVR_EAST, 0,
		                                              ovr_rxdiv2g_pu_bias);

		/* disable trimodal DC WAR */
		MOD_RADIO_REG_20691(pi, RX_TOP_2G_OVR_EAST, 0, ovr_rxdiv2g_rs, 0);
		MOD_RADIO_REG_20691(pi, RX_TOP_2G_OVR_EAST, 0, ovr_rxdiv2g_pu_bias, 0);
	}

	wlc_phy_radio20691_xtal_tune_prep(pi);

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	/* 20691_radio_tune() */
	/* Write chan specific tuning register */
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_VCOCAL1, 0), ci20691->RF_pll_vcocal1);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_VCOCAL11, 0),
	                         ci20691->RF_pll_vcocal11);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_VCOCAL12, 0),
	                         ci20691->RF_pll_vcocal12);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_FRCT2, 0), ci20691->RF_pll_frct2);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_FRCT3, 0), ci20691->RF_pll_frct3);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, LOGEN_CFG2, 0), ci20691->RF_logen_cfg2);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_LF4, 0), ci20691->RF_pll_lf4);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_LF5, 0), ci20691->RF_pll_lf5);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_LF7, 0), ci20691->RF_pll_lf7);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_LF2, 0), ci20691->RF_pll_lf2);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PLL_LF3, 0), ci20691->RF_pll_lf3);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, LOGEN_CFG1, 0), ci20691->RF_logen_cfg1);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, LNA2G_TUNE, 0), ci20691->RF_lna2g_tune);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, TXMIX2G_CFG5, 0),
	                         ci20691->RF_txmix2g_cfg5);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PA2G_CFG2, 0), ci20691->RF_pa2g_cfg2);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, LNA5G_TUNE, 0), ci20691->RF_lna5g_tune);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, TXMIX5G_CFG6, 0),
	                         ci20691->RF_txmix5g_cfg6);
	phy_utils_write_radioreg(pi, RADIO_REG_20691(pi, PA5G_CFG4, 0), ci20691->RF_pa5g_cfg4);

	if (ci20691->chan > CH_MAX_2G_CHANNEL && ci20691->freq < 5250 &&
	    ci20691->freq != 5190 && ci20691->freq != 5230) {
		MOD_RADIO_REG_20691(pi, PLL_VCO2, 0, rfpll_vco_cap_mode, 1);
	} else {
		MOD_RADIO_REG_20691(pi, PLL_VCO2, 0, rfpll_vco_cap_mode, 0);
	}

	/* # The reset/preferred value for ldo_vco and ldo_cp is 0 which is correct
	 * # The above tuning function writes to the whole hvldo register, instead of
	 * read-modify-write (easy for driver guys) and puts back the ldo_vco and ldo_cp
	 * registers to 0 (i.e. to their preferred values)
	 * # The problem is that PHY is not using direct control to turn on ldo_VCO and
	 * ldo_CP and hence it is important to make sure that the radio jtag value for
	 * these is 1 and not 0
	 */

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO1, 0, ldo_2p5_pu_ldo_VCO, 1)
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO1, 0, ldo_2p5_pu_ldo_CP, 1)

		/* 4345a0: Current optimization */
		/* Value for normal power mode, moved out of wlc_phy_radio_tiny_vcocal(). */
		MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO6, 0, rfpll_vco_ALC_ref_ctrl, 0xf)
	ACPHY_REG_LIST_EXECUTE(pi);

	acphy_set_lpmode(pi, ACPHY_LP_RADIO_LVL_OPT);

	/* Do a VCO cal after writing the tuning table regs */
	do {
		wlc_phy_radio_tiny_vcocal(pi);
		itr++;
		if (itr > 2)
			break;
	} while (READ_RADIO_REGFLD_20691(pi, PLL_DSPR27, 0, rfpll_monitor_need_refresh) == 1);

	wlc_phy_radio20691_xtal_tune(pi);

	/* restore the ovr_rxdiv2g radio registers */
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		MOD_RADIO_REG_20691(pi, RX_TOP_2G_OVR_EAST, 0, ovr_rxdiv2g_rs, ovr_rxdiv2g_rs);
		MOD_RADIO_REG_20691(pi, RX_TOP_2G_OVR_EAST, 0, ovr_rxdiv2g_pu_bias,
		                    ovr_rxdiv2g_pu_bias);
	}
}

static void
wlc_phy_chanspec_radio2069_setup(phy_info_t *pi, const void *chan_info, uint8 toggle_logen_reset)
{
	uint8 core;
	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(pi->radio_chanspec),
	        CHSPEC_IS2G(pi->radio_chanspec) ? WF_CHAN_FACTOR_2_4_G
	        : WF_CHAN_FACTOR_5_G);
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	ASSERT(chan_info != NULL);

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		const chan_info_radio2069revGE32_t *ciGE32 = chan_info;

		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5, ciGE32->RFP_pll_vcocal5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6, ciGE32->RFP_pll_vcocal6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2, ciGE32->RFP_pll_vcocal2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1, ciGE32->RFP_pll_vcocal1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11, ciGE32->RFP_pll_vcocal11);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12, ciGE32->RFP_pll_vcocal12);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2, ciGE32->RFP_pll_frct2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3, ciGE32->RFP_pll_frct3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10, ciGE32->RFP_pll_vcocal10);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3, ciGE32->RFP_pll_xtal3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, ciGE32->RFP_pll_vco2);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1, ciGE32->RFP_logen5g_cfg1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8, ciGE32->RFP_pll_vco8);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6, ciGE32->RFP_pll_vco6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3, ciGE32->RFP_pll_vco3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1, ciGE32->RFP_pll_xtalldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1, ciGE32->RFP_pll_hvldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2, ciGE32->RFP_pll_hvldo2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5, ciGE32->RFP_pll_vco5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4, ciGE32->RFP_pll_vco4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE32->RFP_pll_lf4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE32->RFP_pll_lf5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE32->RFP_pll_lf7);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE32->RFP_pll_lf2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE32->RFP_pll_lf3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE32->RFP_pll_cp4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE32->RFP_pll_lf6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL4, ciGE32->RFP_pll_xtal4);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE, ciGE32->RFP_logen2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_LNA2G_TUNE, ciGE32->RFX_lna2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX2G_CFG1, ciGE32->RFX_txmix2g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA2G_CFG2, ciGE32->RFX_pga2g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD2G_TUNE, ciGE32->RFX_pad2g_tune);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1, ciGE32->RFP_logen5g_tune1);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2, ciGE32->RFP_logen5g_tune2);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_IDAC1, ciGE32->RFP_logen5g_idac1);
		phy_utils_write_radioreg(pi, RFX_2069_LNA5G_TUNE, ciGE32->RFX_lna5g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX5G_CFG1, ciGE32->RFX_txmix5g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA5G_CFG2, ciGE32->RFX_pga5g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD5G_TUNE, ciGE32->RFX_pad5g_tune);
		if ((RADIO2069_MINORREV(pi->pubpi->radiorev) == 4) &&
		    pi->sh->chippkg == 2 && PHY_XTAL_IS37M4(pi)) {
			if (fc == 5290) {
				ACPHY_REG_LIST_START
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL5, 0X5)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL6, 0x1C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL2, 0xA09)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL1, 0xF89)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL11, 0xD4)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL12, 0x2A70)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT2, 0x350)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT3, 0xA9C1)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL10, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL3, 0x488)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO2, 0xCE8)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_CFG1, 0x40)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO8, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO6, 0x1D6f)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO3, 0x1F00)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTALLDO1, 0x780)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO1, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO2, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO5, 0x49C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO4, 0x3504)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xD6F)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xECBE)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xDDE2)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF6, 0x1)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL4, 0x36FF)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_TUNE1, 0x80)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (fc == 5180) {
				ACPHY_REG_LIST_START
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL5, 0x5)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL6, 0x1C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL2, 0xA09)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL1, 0xF37)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL11, 0xCF)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL12, 0xC106)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT2, 0x33F)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT3, 0x41B)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL10, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL3, 0x488)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO2, 0xCE8)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_CFG1, 0x40)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO8, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO6, 0x1D6F)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO3, 0x1F00)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTALLDO1, 0x780)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO1, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO2, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO5, 0x49C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO4, 0x3505)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xD6D)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xF1C3)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xE2E7)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF6, 0x1)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL4, 0x36CF)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_TUNE1, 0xA0)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
		}
		/* Move nbclip by 2dBs to the right */
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_ib_Refladder, 7);
			MOD_RADIO_REGC(pi, DAC_CFG1, core, DAC_invclk, 1);
		}

		/* Fix drift/unlock behavior */
		MOD_RADIO_REG(pi, RFP, PLL_CFG3, rfpll_spare1, 0x8);

	} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
		if ((RADIO2069REV(pi->pubpi->radiorev) != 25) &&
			(RADIO2069REV(pi->pubpi->radiorev) != 26)) {
			const chan_info_radio2069revGE16_t *ciGE16 = chan_info;

			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5, ciGE16->RFP_pll_vcocal5);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6, ciGE16->RFP_pll_vcocal6);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2, ciGE16->RFP_pll_vcocal2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1, ciGE16->RFP_pll_vcocal1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11,
			                         ciGE16->RFP_pll_vcocal11);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12,
			                         ciGE16->RFP_pll_vcocal12);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2, ciGE16->RFP_pll_frct2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3, ciGE16->RFP_pll_frct3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10,
			                         ciGE16->RFP_pll_vcocal10);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3, ciGE16->RFP_pll_xtal3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, ciGE16->RFP_pll_vco2);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1,
			                         ciGE16->RFP_logen5g_cfg1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8, ciGE16->RFP_pll_vco8);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6, ciGE16->RFP_pll_vco6);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3, ciGE16->RFP_pll_vco3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1,
			                         ciGE16->RFP_pll_xtalldo1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1, ciGE16->RFP_pll_hvldo1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2, ciGE16->RFP_pll_hvldo2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5, ciGE16->RFP_pll_vco5);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4, ciGE16->RFP_pll_vco4);

			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE16->RFP_pll_lf4);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE16->RFP_pll_lf5);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE16->RFP_pll_lf7);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE16->RFP_pll_lf2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE16->RFP_pll_lf3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE16->RFP_pll_cp4);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE16->RFP_pll_lf6);

			phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE,
			                         ciGE16->RFP_logen2g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_LNA2G_TUNE, ciGE16->RF0_lna2g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_TXMIX2G_CFG1,
			                         ciGE16->RF0_txmix2g_cfg1);
			phy_utils_write_radioreg(pi, RF0_2069_PGA2G_CFG2, ciGE16->RF0_pga2g_cfg2);
			phy_utils_write_radioreg(pi, RF0_2069_PAD2G_TUNE, ciGE16->RF0_pad2g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1,
			                         ciGE16->RFP_logen5g_tune1);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2,
			                         ciGE16->RFP_logen5g_tune2);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_RCCR,
			                         ciGE16->RF0_logen5g_rccr);
			phy_utils_write_radioreg(pi, RF0_2069_LNA5G_TUNE, ciGE16->RF0_lna5g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_TXMIX5G_CFG1,
			                         ciGE16->RF0_txmix5g_cfg1);
			phy_utils_write_radioreg(pi, RF0_2069_PGA5G_CFG2, ciGE16->RF0_pga5g_cfg2);
			phy_utils_write_radioreg(pi, RF0_2069_PAD5G_TUNE, ciGE16->RF0_pad5g_tune);
			/*
			* phy_utils_write_radioreg(pi, RFP_2069_PLL_CP5, ciGE16->RFP_pll_cp5);
			* phy_utils_write_radioreg(pi, RF0_2069_AFEDIV1, ciGE16->RF0_afediv1);
			* phy_utils_write_radioreg(pi, RF0_2069_AFEDIV2, ciGE16->RF0_afediv2);
			* phy_utils_write_radioreg(pi, RF0_2069_ADC_CFG5, ciGE16->RF0_adc_cfg5);
			*/

		} else {
			if (!PHY_XTAL_IS52M(pi)) {
				const chan_info_radio2069revGE25_t *ciGE25 = chan_info;
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5,
				                         ciGE25->RFP_pll_vcocal5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6,
				                         ciGE25->RFP_pll_vcocal6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2,
				                         ciGE25->RFP_pll_vcocal2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1,
				                         ciGE25->RFP_pll_vcocal1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11,
					ciGE25->RFP_pll_vcocal11);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12,
					ciGE25->RFP_pll_vcocal12);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2,
				                         ciGE25->RFP_pll_frct2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3,
				                         ciGE25->RFP_pll_frct3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10,
					ciGE25->RFP_pll_vcocal10);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3,
				                         ciGE25->RFP_pll_xtal3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_CFG3,
				                         ciGE25->RFP_pll_cfg3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2,
				                         ciGE25->RFP_pll_vco2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1,
					ciGE25->RFP_logen5g_cfg1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8,
				                         ciGE25->RFP_pll_vco8);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6,
				                         ciGE25->RFP_pll_vco6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3,
				                         ciGE25->RFP_pll_vco3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1,
					ciGE25->RFP_pll_xtalldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1,
				                         ciGE25->RFP_pll_hvldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2,
				                         ciGE25->RFP_pll_hvldo2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5,
				                         ciGE25->RFP_pll_vco5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4,
				                         ciGE25->RFP_pll_vco4);

				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE25->RFP_pll_lf4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE25->RFP_pll_lf5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE25->RFP_pll_lf7);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE25->RFP_pll_lf2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE25->RFP_pll_lf3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE25->RFP_pll_cp4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE25->RFP_pll_lf6);

				phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE,
					ciGE25->RFP_logen2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LNA2G_TUNE,
				                         ciGE25->RF0_lna2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX2G_CFG1,
					ciGE25->RF0_txmix2g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA2G_CFG2,
				                         ciGE25->RF0_pga2g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD2G_TUNE,
				                         ciGE25->RF0_pad2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1,
					ciGE25->RFP_logen5g_tune1);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2,
					ciGE25->RFP_logen5g_tune2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_RCCR,
					ciGE25->RF0_logen5g_rccr);
				phy_utils_write_radioreg(pi, RF0_2069_LNA5G_TUNE,
				                         ciGE25->RF0_lna5g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX5G_CFG1,
					ciGE25->RF0_txmix5g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA5G_CFG2,
				                         ciGE25->RF0_pga5g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD5G_TUNE,
				                         ciGE25->RF0_pad5g_tune);

				/*
				 * phy_utils_write_radioreg(pi, RFP_2069_PLL_CP5,
				 *                          ciGE25->RFP_pll_cp5);
				 * phy_utils_write_radioreg(pi, RF0_2069_AFEDIV1,
				 *                          ciGE25->RF0_afediv1);
				 * phy_utils_write_radioreg(pi, RF0_2069_AFEDIV2,
				 *                          ciGE25->RF0_afediv2);
				 * phy_utils_write_radioreg(pi, RF0_2069_ADC_CFG5,
				 *                          ciGE25->RF0_adc_cfg5);
				 */

			} else {
				const chan_info_radio2069revGE25_52MHz_t *ciGE25 = chan_info;

				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5,
				                         ciGE25->RFP_pll_vcocal5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6,
				                         ciGE25->RFP_pll_vcocal6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2,
				                         ciGE25->RFP_pll_vcocal2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1,
				                         ciGE25->RFP_pll_vcocal1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11,
					ciGE25->RFP_pll_vcocal11);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12,
					ciGE25->RFP_pll_vcocal12);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2,
				                         ciGE25->RFP_pll_frct2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3,
				                         ciGE25->RFP_pll_frct3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10,
					ciGE25->RFP_pll_vcocal10);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3,
				                         ciGE25->RFP_pll_xtal3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2,
				                         ciGE25->RFP_pll_vco2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1,
					ciGE25->RFP_logen5g_cfg1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8,
				                         ciGE25->RFP_pll_vco8);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6,
				                         ciGE25->RFP_pll_vco6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3,
				                         ciGE25->RFP_pll_vco3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1,
					ciGE25->RFP_pll_xtalldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1,
				                         ciGE25->RFP_pll_hvldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2,
				                         ciGE25->RFP_pll_hvldo2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5,
				                         ciGE25->RFP_pll_vco5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4,
				                         ciGE25->RFP_pll_vco4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE25->RFP_pll_lf4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE25->RFP_pll_lf5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE25->RFP_pll_lf7);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE25->RFP_pll_lf2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE25->RFP_pll_lf3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE25->RFP_pll_cp4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE25->RFP_pll_lf6);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE,
					ciGE25->RFP_logen2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LNA2G_TUNE,
				                         ciGE25->RF0_lna2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX2G_CFG1,
					ciGE25->RF0_txmix2g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA2G_CFG2,
				                         ciGE25->RF0_pga2g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD2G_TUNE,
				                         ciGE25->RF0_pad2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1,
					ciGE25->RFP_logen5g_tune1);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2,
					ciGE25->RFP_logen5g_tune2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_RCCR,
					ciGE25->RF0_logen5g_rccr);
				phy_utils_write_radioreg(pi, RF0_2069_LNA5G_TUNE,
				                         ciGE25->RF0_lna5g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX5G_CFG1,
					ciGE25->RF0_txmix5g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA5G_CFG2,
				                         ciGE25->RF0_pga5g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD5G_TUNE,
				                         ciGE25->RF0_pad5g_tune);
			}

			/* 43162 FCBGA Settings improving Tx EVM */
			/* (1) ch4/ch4m settings to reduce 500k xtal spur */
			/* (2) Rreducing 2440/2480 RX spur */
			if (RADIO2069REV(pi->pubpi->radiorev) == 25 && PHY_XTAL_IS40M(pi)) {
			  ACPHY_REG_LIST_START
			    MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR23, ovr_xtal_coresize_nmos, 0x1)
			    MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL1, xtal_coresize_nmos, 0x8)
			    MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR23, ovr_xtal_coresize_pmos, 0x1)
			    MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL1, xtal_coresize_pmos, 0x8)
			  ACPHY_REG_LIST_EXECUTE(pi);

			  if (CHSPEC_IS2G(pi->radio_chanspec)) {
			    if (CHSPEC_CHANNEL(pi->radio_chanspec) < 12) {
			      if (CHSPEC_CHANNEL(pi->radio_chanspec) == 4)
						phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2,
						                         0xce4);
			      ACPHY_REG_LIST_START
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_bufstrg_BT, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x7)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_outbufstrg, 0x3)
			      ACPHY_REG_LIST_EXECUTE(pi);
			    } else {
			      ACPHY_REG_LIST_START
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_bufstrg_BT, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x0)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_outbufstrg, 0x1)
			      ACPHY_REG_LIST_EXECUTE(pi);
			    }
			  }
			}
		}
	} else {
		const chan_info_radio2069_t *ci = chan_info;

		/* Write chan specific tuning register */
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5, ci->RFP_pll_vcocal5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6, ci->RFP_pll_vcocal6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2, ci->RFP_pll_vcocal2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1, ci->RFP_pll_vcocal1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11, ci->RFP_pll_vcocal11);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12, ci->RFP_pll_vcocal12);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2, ci->RFP_pll_frct2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3, ci->RFP_pll_frct3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10, ci->RFP_pll_vcocal10);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3, ci->RFP_pll_xtal3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, ci->RFP_pll_vco2);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1, ci->RF0_logen5g_cfg1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8, ci->RFP_pll_vco8);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6, ci->RFP_pll_vco6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3, ci->RFP_pll_vco3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1, ci->RFP_pll_xtalldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1, ci->RFP_pll_hvldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2, ci->RFP_pll_hvldo2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5, ci->RFP_pll_vco5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4, ci->RFP_pll_vco4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ci->RFP_pll_lf4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ci->RFP_pll_lf5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ci->RFP_pll_lf7);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ci->RFP_pll_lf2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ci->RFP_pll_lf3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ci->RFP_pll_cp4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP1, ci->RFP_pll_dsp1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP2, ci->RFP_pll_dsp2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP3, ci->RFP_pll_dsp3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP4, ci->RFP_pll_dsp4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP6, ci->RFP_pll_dsp6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP7, ci->RFP_pll_dsp7);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP8, ci->RFP_pll_dsp8);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP9, ci->RFP_pll_dsp9);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE, ci->RF0_logen2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_LNA2G_TUNE, ci->RFX_lna2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX2G_CFG1, ci->RFX_txmix2g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA2G_CFG2, ci->RFX_pga2g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD2G_TUNE, ci->RFX_pad2g_tune);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1, ci->RF0_logen5g_tune1);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2, ci->RF0_logen5g_tune2);
		phy_utils_write_radioreg(pi, RFX_2069_LOGEN5G_RCCR, ci->RFX_logen5g_rccr);
		phy_utils_write_radioreg(pi, RFX_2069_LNA5G_TUNE, ci->RFX_lna5g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX5G_CFG1, ci->RFX_txmix5g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA5G_CFG2, ci->RFX_pga5g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD5G_TUNE, ci->RFX_pad5g_tune);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_CP5, ci->RFP_pll_cp5);
		phy_utils_write_radioreg(pi, RF0_2069_AFEDIV1, ci->RF0_afediv1);
		phy_utils_write_radioreg(pi, RF0_2069_AFEDIV2, ci->RF0_afediv2);
		phy_utils_write_radioreg(pi, RFX_2069_ADC_CFG5, ci->RFX_adc_cfg5);

		/* We need different values for ADC_CFG5 for cores 1 and 2
		 * in order to get the best reduction of spurs from the AFE clk
		 */
		if (RADIO2069REV(pi->pubpi->radiorev) < 4) {
			ACPHY_REG_LIST_START
				WRITE_RADIO_REG_ENTRY(pi, RF1_2069_ADC_CFG5, 0x3e9)
				WRITE_RADIO_REG_ENTRY(pi, RF2_2069_ADC_CFG5, 0x3e9)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CP4, rfpll_cp_ioff, 0xa0)
			ACPHY_REG_LIST_EXECUTE(pi);
		}

		/* Reduce 500 KHz spur at fc=2427 MHz for both 4360 A0 and B0 */
		if (CHSPEC_CHANNEL(pi->radio_chanspec) == 4 &&
			((RADIO2069_MINORREV(pi->pubpi->radiorev) != 16) &&
				(RADIO2069_MINORREV(pi->pubpi->radiorev) != 17))) {
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, 0xce4);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x5);
		}

		/* 43602 XTAL SPUR 2G WAR */
		if (ACMAJORREV_5(pi->pubpi->phy_rev) && CHSPEC_IS2G(pi->radio_chanspec)&&
				!(IS_4364_3x3(pi))) {
			MOD_RADIO_REG(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x3);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL4, xtal_outbufstrg, 0x2);
		}

		/* Move nbclip by 2dBs to the right */
		MOD_RADIO_REG(pi, RFX, NBRSSI_CONFG, nbrssi_ib_Refladder, 7);

		/* 5g only: Changing RFPLL bandwidth to be 150MHz */
		if (CHSPEC_IS5G(pi->radio_chanspec) && !(IS_4364_3x3(pi)))
			wlc_2069_rfpll_150khz(pi);

		if ((RADIO2069_MINORREV(pi->pubpi->radiorev) == 7) &&
		   (BFCTL(pi->u.pi_acphy) == 3)) {
			if ((BF3_FEMCTRL_SUB(pi->u.pi_acphy) == 3 ||
			     BF3_FEMCTRL_SUB(pi->u.pi_acphy) == 5) &&
			    CHSPEC_IS2G(pi->radio_chanspec)) {
			  ACPHY_REG_LIST_START
			    /* 43602 MCH2: offtune to win back linear output power */
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD2G_TUNE, pad2g_tune, 0x1)

			    /* increase gain */
			    MOD_RADIO_REG_ENTRY(pi, RFX, PGA2G_CFG1, pga2g_gainboost, 0x2)
			    WRITE_RADIO_REG_ENTRY(pi, RFX_2069_PAD2G_INCAP, 0x7e7e)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD2G_IDAC, pad2g_idac_main, 0x38)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PGA2G_INCAP, pad2g_idac_aux, 0x38)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD2G_IDAC, pad2g_idac_cascode, 0xe)
			  ACPHY_REG_LIST_EXECUTE(pi);
			} else if (BF3_FEMCTRL_SUB(pi->u.pi_acphy) == 0 &&
			           CHSPEC_IS5G(pi->radio_chanspec)) {
			  ACPHY_REG_LIST_START
			    /* 43602 MCH5: increase gain to win back linear output power */
			    MOD_RADIO_REGC_ENTRY(pi, TXMIX5G_CFG1, 2, gainboost, 0x4)
			    MOD_RADIO_REGC_ENTRY(pi, PGA5G_CFG1, 2, gainboost, 0x4)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD5G_IDAC, idac_main, 0x3d)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD5G_TUNE, idac_aux, 0x3d)
			  ACPHY_REG_LIST_EXECUTE(pi);
			}
		}
	}

	if (RADIO2069REV(pi->pubpi->radiorev) >= 4) {
		/* Make clamping stronger */
		phy_utils_write_radioreg(pi, RFX_2069_ADC_CFG5, 0x83e0);
	}

	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) &&
	    (!(PHY_IPA(pi)))) {
	    MOD_RADIO_REG(pi, RFP, PLL_CP4, rfpll_cp_ioff, 0xe0);
	}

	/* increasing pabias to get good evm with pagain3 */
	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) &&
	    !(ACRADIO_2069_EPA_IS(pi->pubpi->radiorev))) {
		phy_utils_write_radioreg(pi, RF0_2069_PA5G_IDAC2, 0x8484);

		if (PHY_IPA(pi)) {
			ACPHY_REG_LIST_START
				WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_IDAC1, 0x3F37)
				WRITE_RADIO_REG_ENTRY(pi, RF0_2069_PGA5G_IDAC, 0x3838)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR2, ovr_bg_pulse, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_BG_CFG1, bg_pulse, 1)
			ACPHY_REG_LIST_EXECUTE(pi);

			FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
				MOD_RADIO_REGC(pi, PAD5G_IDAC, core, idac_main, 0x20);
				MOD_RADIO_REGC(pi, PAD5G_TUNE, core, idac_aux, 0x20);

				MOD_RADIO_REGC(pi, PA5G_INCAP, core,
					pa5g_idac_incap_compen_main, 0x8);
				MOD_RADIO_REGC(pi, PA5G_INCAP, core,
					pa5g_idac_incap_compen_aux, 0x8);
				MOD_RADIO_REGC(pi, PA2G_INCAP, core,
					pa2g_ptat_slope_incap_compen_main, 0x0);
				MOD_RADIO_REGC(pi, PA2G_INCAP, core,
					pa2g_ptat_slope_incap_compen_aux, 0x0);

				if (pi->sh->chippkg == BCM4335_FCBGA_PKG_ID) {
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_main, 0x1);
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_aux, 0x1);
				} else {
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_main, 0x3);
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_aux, 0x3);
				}

				MOD_RADIO_REGC(pi, PAD5G_INCAP, core, idac_incap_compen_main, 0xc);
				MOD_RADIO_REGC(pi, PAD5G_INCAP, core, idac_incap_compen_aux, 0xc);
				MOD_RADIO_REGC(pi, PGA5G_INCAP, core, idac_incap_compen, 0x8);
				MOD_RADIO_REGC(pi, PA5G_CFG2, core, pa5g_bias_cas, 0x58);
				MOD_RADIO_REGC(pi, PA5G_IDAC2, core, pa5g_biasa_main, 0x84);
				MOD_RADIO_REGC(pi, PA5G_IDAC2, core, pa5g_biasa_aux, 0x84);
				MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_mix5g_gainboost, 0x1);
				MOD_RADIO_REGC(pi, TXMIX5G_CFG1, core, gainboost, 0x0);
				MOD_RADIO_REGC(pi, TXGM_CFG1, core, gc_res, 0x0);
				MOD_RADIO_REGC(pi, PA2G_CFG3, core, pa2g_ptat_slope_main, 0x7);
				MOD_RADIO_REGC(pi, PAD2G_SLOPE, core, pad2g_ptat_slope_main, 0x7);
				MOD_RADIO_REGC(pi, PGA2G_CFG2, core, pga2g_ptat_slope_main, 0x7);
				MOD_RADIO_REGC(pi, PGA2G_IDAC, core, pga2g_idac_main, 0x15);
				MOD_RADIO_REGC(pi, PAD2G_TUNE, core, pad2g_idac_tuning_bias, 0xc);
				MOD_RADIO_REGC(pi, TXMIX2G_CFG1, core, lodc, 0x3);
			}
		}
	}
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
		wlc_phy_2069_4335_set_ovrds(pi);
	} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		wlc_phy_2069_4350_set_ovrds(pi->u.pi_acphy->radioi);
	}

	/* Offset RCCAL by 4 when using divide by 5 for ADC stability */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2 && (!PHY_IPA(pi))) {
		FOREACH_CORE(pi, core) {
			if ((!(fc == 5190) && CHSPEC_IS40(pi->radio_chanspec)) ||
			    (CHSPEC_IS20(pi->radio_chanspec) &&
			     ((((fc == 5180) && (pi->sh->chippkg != 2)) ||
			       ((fc >= 5200) && (fc <= 5320)) ||
			       ((fc >= 5745) && (fc <= 5825)))))) {
				MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_4_0,
				               pi_ac->radioi->rccal_adc_rc + 4);
			} else {
				MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_4_0,
				               pi_ac->radioi->rccal_adc_rc);
			}
		}
	}

	/* 4335C0: Current optimization */
	acphy_set_lpmode(pi, ACPHY_LP_RADIO_LVL_OPT);

	/* Do a VCO cal after writing the tuning table regs */
	wlc_phy_radio2069_vcocal(pi);
}

/**
 * initialize the static tables defined in auto-generated wlc_phytbl_ac.c,
 * see acphyprocs.tcl, proc acphy_init_tbls
 * After called in the attach stage, all the static phy tables are reclaimed.
 */
static void
WLBANDINITFN(wlc_phy_static_table_download_acphy)(phy_info_t *pi)
{
	uint idx;
	uint8 stall_val;
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);
	if (pi->phy_init_por) {
		/* these tables are not affected by phy reset, only power down */
		if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
			if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
				for (idx = 0; idx < acphytbl_info_sz_rev40; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_rev40[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi,
					acphyzerotbl_info_rev40,
					acphyzerotbl_info_cnt_rev40);
			}
			else {
				for (idx = 0; idx < acphytbl_info_sz_maxbw20_rev40; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_maxbw20_rev40[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi,
					acphyzerotbl_info_maxbw20_rev40,
					acphyzerotbl_info_cnt_maxbw20_rev40);
			}
		} else if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev36; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev36[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev36,
				acphyzerotbl_info_cnt_rev36);
		} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		           ACMAJORREV_33(pi->pubpi->phy_rev) ||
		           ACMAJORREV_37(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev32; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev32[idx]);
			}
			if (ACMAJORREV_33(pi->pubpi->phy_rev) ||
			    ACMAJORREV_37(pi->pubpi->phy_rev)) {
				for (idx = 0; idx < acphytbl_info_sz_rev33; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_rev33[idx]);
				}
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev32,
				acphyzerotbl_info_cnt_rev32);
		} else if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev9; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev9[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev9,
				acphyzerotbl_info_cnt_rev9);
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
			if (ACREV_IS(pi->pubpi->phy_rev, 9)) { /* 43602a0 */
				for (idx = 0; idx < acphytbl_info_sz_rev9; idx++) {
					wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev9[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev9,
					acphyzerotbl_info_cnt_rev9);
			} else {
				for (idx = 0; idx < acphytbl_info_sz_rev3; idx++) {
					wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev3[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev3,
				acphyzerotbl_info_cnt_rev3);
			}
		} else if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
			if (ACREV_IS(pi->pubpi->phy_rev, 6))	{
				for (idx = 0; idx < acphytbl_info_sz_rev6; idx++) {
					wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev6[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev6,
					acphyzerotbl_info_cnt_rev6);
			} else {
				for (idx = 0; idx < acphytbl_info_sz_rev2; idx++) {
					wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev2[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev2,
					acphyzerotbl_info_cnt_rev2);
			}
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev0; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev0[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev0,
				acphyzerotbl_info_cnt_rev0);
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			uint16 phymode = phy_get_phymode(pi);
			for (idx = 0; idx < acphytbl_info_sz_rev12; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev12[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev12,
				acphyzerotbl_info_cnt_rev12);

			if ((phymode == PHYMODE_MIMO) || (phymode == PHYMODE_80P80)) {
				wlc_phy_clear_static_table_acphy(pi,
					acphyzerotbl_delta_MIMO_80P80_info_rev12,
					acphyzerotbl_delta_MIMO_80P80_info_cnt_rev12);
			}
		}
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
}

/*  R Calibration (takes ~50us) */
static void
wlc_phy_radio_tiny_rcal(phy_info_t *pi, acphy_tiny_rcal_modes_t calmode)
{
	/* Format: 20691_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* The rcalcode argument works only with mode 1 and is optional. */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */

	uint8 rcal_valid, rcal_value;
	uint8 core = 0;
	bool is_mode3_and_rsdb_core1 = FALSE;
	uint16 macmode = phy_get_phymode(pi);
	uint8 curr_core = phy_get_current_core(pi);
	phy_info_t *other_pi = phy_get_other_pi(pi), *tmp_pi;

	/* Skip this function for QT */
	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));


	if ((calmode == TINY_RCAL_MODE3_BOTH_CORE_CAL) &&
		(macmode == PHYMODE_RSDB) && (curr_core == PHY_RSBD_PI_IDX_CORE1)) {
		is_mode3_and_rsdb_core1 = TRUE;
	}

	if ((calmode == TINY_RCAL_MODE2_SINGLE_CORE_CAL) ||
		(calmode == TINY_RCAL_MODE3_BOTH_CORE_CAL)) {

		FOREACH_CORE(pi, core) {
			if (calmode == TINY_RCAL_MODE2_SINGLE_CORE_CAL) {
				if (((macmode == PHYMODE_RSDB) &&
					(curr_core == PHY_RSBD_PI_IDX_CORE1)) ||
					((macmode != PHYMODE_RSDB) && (core == 1))) {
					break;
				}
			}

			/* This should run only for core 0 */
			/* Run R Cal and use its output */
			/* JIRA: SW4349-1383.
			 * instead of swithc base adress to core 0, use other_pi
			 */
			tmp_pi = is_mode3_and_rsdb_core1 ? other_pi : pi;

			MOD_RADIO_REG_TINY(tmp_pi, PLL_XTAL2, 0, xtal_pu_RCCAL1, 0x1);
			MOD_RADIO_REG_TINY(tmp_pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0x1);

			/* Make connection with the external 10k resistor */
			/* Also powers up rcal (RF_rcal_cfg.rcal_pu = 1) */
			MOD_RADIO_REG_TINY(pi, GPAIO_SEL2, core, gpaio_pu, 1);
			MOD_RADIO_REG_TINY(pi, RCAL_CFG_NORTH, core, rcal_pu, 0);
			MOD_RADIO_REG_TINY(pi, GPAIO_SEL0, core, gpaio_sel_0to15_port, 0);
			MOD_RADIO_REG_TINY(pi, GPAIO_SEL1, core, gpaio_sel_16to31_port, 0);
			MOD_RADIO_REG_TINY(pi, RCAL_CFG_NORTH, core, rcal_pu, 1);

			SPINWAIT(READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
				core, rcal_valid) == 0,
				ACPHY_SPINWAIT_RCAL_STATUS);
			if (READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
					core, rcal_valid) == 0) {
				PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :", __FUNCTION__));
				PHY_FATAL_ERROR_MESG(("RCAL is not valid for Core%d\n", core));
				PHY_FATAL_ERROR(pi, PHY_RC_RCAL_INVALID);
			}
			rcal_valid =  READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
					core, rcal_valid);

			if (rcal_valid == 1) {
				rcal_value = READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
					core, rcal_value) >> 1;

				if (pi->fabid == BCM4349_UMC_FAB_ID) {
					/* In UMC B0 & B1 materials, core0 reference voltage
					 * (Vref) used by RCAL is ~150mV lower from target value
					 * 0.52V It creates 1~2 codes lower than optimal value
					 * 1 RCAL code corresponds to ~150mV change in Vref
					 * So it is recommend to add one code to core0 RCAL
					 * results in ATE
					 */
					if (!((macmode == PHYMODE_RSDB) ? curr_core : core))
						rcal_value += 1;
				}

				PHY_TRACE(("*** rcal_value: %x\n", rcal_value));

#ifdef ATE_BUILD
				wl_ate_set_buffer_regval(RCAL_VALUE, rcal_value,
						core, phy_get_current_core(pi), pi->sh->chip);
#endif

				/* Use the output of the rcal engine */
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
				if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
					/* Copy the rcal value instead of latching */
					MOD_RADIO_REG_TINY(pi, BG_OVR1, core,
						ovr_bg_rcal_trim,  1);
					MOD_RADIO_REG_TINY(pi, BG_CFG1, core,
						bg_rcal_trim,  rcal_value);
				}
				else { /* Use latched output of rcal engine */
					MOD_RADIO_REG_TINY(pi, BG_OVR1, core,
						ovr_bg_rcal_trim, 0);
				}
				/* Very coarse sanity check */
				if ((rcal_value < 2) || (12 < rcal_value)) {
					PHY_ERROR(("*** ERROR: R Cal value out of range."
					" 4bit Rcal = %d.\n", rcal_value));
				}
			} else {
				PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
					__FUNCTION__, rcal_valid));
			}
			/* Power down blocks not needed anymore */
			MOD_RADIO_REG_TINY(pi, GPAIO_SEL2, core, gpaio_pu, 0);
			MOD_RADIO_REG_TINY(pi, PLL_XTAL2, core, xtal_pu_RCCAL1, 0);
			MOD_RADIO_REG_TINY(pi, PLL_XTAL2, core, xtal_pu_RCCAL,  0);
			MOD_RADIO_REG_TINY(pi, RCAL_CFG_NORTH, core, rcal_pu, 0);
		}

		if (calmode == TINY_RCAL_MODE2_SINGLE_CORE_CAL) {
			/* for RSDB/MIMO core 1, copy rcal value from pi of core 0 */
			if (macmode != PHYMODE_RSDB) {
				rcal_value = READ_RADIO_REGFLD_TINY(pi, BG_CFG1,
					0, bg_rcal_trim);
				core = 1;
			} else {
				phy_info_t *pi_core0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
				rcal_value = READ_RADIO_REGFLD_TINY(pi_core0, BG_CFG1,
					0, bg_rcal_trim);
				core = 0;
			}
			MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_rcal_trim,  rcal_value);
			MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_bg_rcal_trim,  1);
		}
	} else {
		FOREACH_CORE(pi, core) {
			if (calmode == TINY_RCAL_MODE0_OTP) {
				if ((pi->sromi->rcal_otp_val >= 2) &&
					(pi->sromi->rcal_otp_val <= 12)) {
					/* Use OTP stored rcal value */
					MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 1);
				} else {
					wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE1_STATIC);
				}
			} else if (calmode == TINY_RCAL_MODE1_STATIC) {

				if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID) &&
					((RADIO20691_MAJORREV(pi->pubpi->radiorev) == 0) ||
					((RADIO20691_MAJORREV(pi->pubpi->radiorev) == 1) &&
					((RADIO20691_MINORREV(pi->pubpi->radiorev) < 5) ||
					((RADIO20691_MINORREV(pi->pubpi->radiorev) == 16)))))) {
					/* before 4345c0  and 4364 1x1 */
					rcal_value = 11;
				} else if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID) &&
					((RADIO20691_MAJORREV(pi->pubpi->radiorev) == 1) &&
					((RADIO20691_MINORREV(pi->pubpi->radiorev) == 17)))) {
					/* 4364B0 1x1 */
					rcal_value = 8;

				} else {
					/* 4345c0 and later */
					if (pi->fabid == BCM4349_TSMC_FAB_ID)
						rcal_value = TSMC_FAB_RCAL_VAL;
					else
						rcal_value = UMC_FAB_RCAL_VAL;
				}
				MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_rcal_trim, rcal_value);
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
			}
		}
	}
}

static void
wlc_phy_radio_tiny_rcal_wave2(phy_info_t *pi, uint8 mode)
{
	/* Format: 20691_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* The rcalcode argument works only with mode 1 and is optional. */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */

	uint8 rcal_valid, loop_iter, rcal_value;
	uint8 core = 0;
	/* Skip this function for QT */
	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));
	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20693(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 8 */
		MOD_RADIO_PLLREG_20693(pi, BG_CFG1, core, bg_rcal_trim, 8);
		MOD_RADIO_PLLREG_20693(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20693(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		/* This should run only for core 0 */
		/* Run R Cal and use its output */
		MOD_RADIO_PLLREG_20693(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);
		MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x3f);

		/* Make connection with the external 10k resistor */
		/* clear all the test-point */
		MOD_RADIO_ALLREG_20693(pi, GPAIO_SEL0, gpaio_sel_0to15_port, 0);
		MOD_RADIO_ALLREG_20693(pi, GPAIO_SEL1, gpaio_sel_16to31_port, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL0, 0,
		                    top_gpaio_sel_0to15_port, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL1, 0,
		                    top_gpaio_sel_16to31_port, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL2, 0, top_gpaio_pu, 1);
		MOD_RADIO_PLLREG_20693(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_PLLREGFLD_20693(pi, RCAL_CFG_NORTH, 0, rcal_valid);
			loop_iter ++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_PLLREGFLD_20693(pi,
				RCAL_CFG_NORTH, 0, rcal_value) >> 1;

			/* Use the output of the rcal engine */
			MOD_RADIO_PLLREG_20693(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
			/* MOD_RADIO_PLLREG_20693(pi, BG_CFG1, 0, bg_rcal_trim, rcal_value); */
			MOD_RADIO_PLLREG_20693(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 0);

			/* Very coarse sanity check */
			if ((rcal_value < 2) || (12 < rcal_value)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				           " 4bit Rcal = %d.\n", rcal_value));
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
			           __FUNCTION__, rcal_valid));
		}

		/* Power down blocks not needed anymore */
		MOD_RADIO_ALLREG_20693(pi, GPAIO_SEL2, gpaio_pu, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL2, 0, top_gpaio_pu, 0);
		MOD_RADIO_PLLREG_20693(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
		MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x3);
	}
}

static void
wlc_phy_radio_tiny_rccal(phy_ac_radio_info_t *ri)
{
	uint8 cal, done;
	uint16 rccal_itr, n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x1, 0x0};
	uint8 sc[] = {0x0, 0x1, 0x2};
	uint8 x1[] = {0x1c, 0x70, 0x40};
	uint16 trc[] = {0x22d, 0xf0, 0x10a};
	phy_info_t *pi = ri->pi;
	uint32 dn;

	if (IS_4364_1x1(pi)) {
	   sr[2] = 0;
	   sc[2] = 0x1;
	   x1[2] = 0x68;
	   trc[2] = 0x120;
	}

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));

	/* For RSDB core 1, copy core 0 values to core 1 for rc cal and return */
	if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(phy_get_current_core(pi) != PHY_RSBD_PI_IDX_CORE0) &&
		(ACMAJORREV_4(pi->pubpi->phy_rev))) {
		phy_info_t *pi_rsdb0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
		ri->data.rccal_gmult = pi_rsdb0->u.pi_acphy->radioi->data.rccal_gmult;
		ri->data.rccal_gmult_rc = pi_rsdb0->u.pi_acphy->radioi->data.rccal_gmult_rc;
		ri->rccal_adc_gmult = pi_rsdb0->u.pi_acphy->radioi->rccal_adc_gmult;
		ri->data.rccal_dacbuf = pi_rsdb0->u.pi_acphy->radioi->data.rccal_dacbuf;
		return;
	}

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 1);

	/* Calibrate lpf, adc, dacbuf */
	for (cal = 0; cal < NUM_2069_RCCAL_CAPS; cal++) {
		/* Setup */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_sr, sr[cal]);
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_sc, sc[cal]);
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG1, 0, rccal_X1, x1[cal]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, RCCAL_CFG2, 0), trc[cal]);

		/* For dacbuf force fixed dacbuf cap to be 0 while calibration, restore it later */
		if (cal == 2) {
			MOD_RADIO_REG_TINY(pi, TX_DAC_CFG5, 0, DACbuf_fixed_cap, 0);
			MOD_RADIO_REG_TINY(pi, TX_BB_OVR1, 0, ovr_DACbuf_fixed_cap, 1);
		}

		/* Toggle RCCAL power */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG1, 0, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
			(rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
			rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG3, 0, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG1, 0, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		n0 = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG4, 0, rccal_N0);
		n1 = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG5, 0, rccal_N1);
		dn = n1 - n0; /* set dn [expr {$N1 - $N0}] */

		if (cal == 0) {
			/* lpf */
			/* set k [expr {$is_adc ? 102051 : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->data.rccal_gmult = (101541 * dn) / (pi->xtalfreq >> 12);
			ri->data.rccal_gmult_rc = ri->data.rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->data.rccal_gmult));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_LPF, ri->data.rccal_gmult, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif
		} else if (cal == 1) {
			/* adc */
			/* set k [expr {$is_adc ? 102051 : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->rccal_adc_gmult = (85000 * dn) / (pi->xtalfreq >> 12);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->rccal_adc_gmult));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_ADC, ri->rccal_adc_gmult, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif
		} else {
			/* dacbuf */
			ri->data.rccal_dacbuf = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG6, 0,
			                                              rccal_raw_dacbuf);
			MOD_RADIO_REG_TINY(pi, TX_BB_OVR1, 0, ovr_DACbuf_fixed_cap, 0);
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, ri->data.rccal_dacbuf));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(RCCAL_DACBUF, ri->data.rccal_dacbuf, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif
		}
		/* Turn off rccal */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_pu, 0);
	}
	/* Powerdown rccal driver */
	MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0);
}

static void
wlc_phy_radio_tiny_rccal_wave2(phy_ac_radio_info_t *ri)
{
	uint8 cal, done;
	uint16 rccal_itr, n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x1, 0x0};
	uint8 sc[] = {0x0, 0x2, 0x1};
	uint8 x1[] = {0x1c, 0x70, 0x68};
	uint16 trc[] = {0x22d, 0xf0, 0x134};
	phy_info_t *pi = ri->pi;
	uint32 dn;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x23);

	/* Calibrate lpf, adc, dacbuf */
	for (cal = 0; cal < NUM_2069_RCCAL_CAPS; cal++) {
		/* Setup */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_sr, sr[cal]);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_sc, sc[cal]);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG1, 0, rccal_X1, x1[cal]);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG2, 0, rccal_Trc, trc[cal]);

		/* For dacbuf force fixed dacbuf cap to be 0 while calibration, restore it later */
		if (cal == 2) {
			MOD_RADIO_ALLREG_20693(pi, TX_DAC_CFG5, DACbuf_fixed_cap, 0);
			MOD_RADIO_ALLREG_20693(pi, TX_BB_OVR1, ovr_DACbuf_fixed_cap, 1);
		}

		/* Toggle RCCAL power */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG1, 0, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
			(rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
			rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG3, 0, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG1, 0, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		n0 = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG4, 0, rccal_N0);
		n1 = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG5, 0, rccal_N1);
		dn = n1 - n0; /* set dn [expr {$N1 - $N0}] */

		if (cal == 0) {
			/* lpf */
			/* set k [expr {$is_adc ? (($def(d11_radio_major_rev) == 3)? */
			/* 85000 : 102051) : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->data.rccal_gmult = (101541 * dn) / (pi->xtalfreq >> 12);
			ri->data.rccal_gmult_rc = ri->data.rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->data.rccal_gmult));
#ifdef ATE_BUILD
			ate_buffer_regval.gmult_lpf = ri->data.rccal_gmult;
#endif
		} else if (cal == 1) {
			/* adc */
			/* set k [expr {$is_adc ? (($def(d11_radio_major_rev) == 3)? */
			/* 85000 : 102051) : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->rccal_adc_gmult = (102051 * dn) / (pi->xtalfreq >> 12);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->rccal_adc_gmult));
#ifdef ATE_BUILD
			ate_buffer_regval.gmult_adc = ri->rccal_adc_gmult;
#endif
		} else {
			/* dacbuf */
			ri->data.rccal_dacbuf = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG6, 0,
			                                              rccal_raw_dacbuf);
			MOD_RADIO_ALLREG_20693(pi, TX_BB_OVR1, ovr_DACbuf_fixed_cap, 0);
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, ri->data.rccal_dacbuf));
#ifdef ATE_BUILD
			ate_buffer_regval.rccal_dacbuf = ri->data.rccal_dacbuf;
#endif
		}
		/* Turn off rccal */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_pu, 0);
	}

	MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x3);
}

/* ***************************** */
/*		External Defintions			*/
/* ***************************** */


/*
Initialize chip regs(RWP) & tables with init vals that do not get reset with phy_reset
*/
static void
wlc_phy_set_regtbl_on_pwron_acphy(phy_info_t *pi)
{
	uint8 core;
	uint16 val;
	uint16 rfseq_bundle_48[3];

	bool flag2rangeon = ((CHSPEC_IS2G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi2g(pi->tpci)) ||
		(CHSPEC_IS5G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi5g(pi->tpci))) && PHY_IPA(pi);

	/* force afediv(core 0, 1, 2) always high */
	if (!(ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)))
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x77);

	if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrlN0, 0x3);
	}
	ACPHY_REG_LIST_START
		/* Remove RFCTRL signal overrides for all cores */
		ACPHYREG_BCAST_ENTRY(pi, RfctrlIntc0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideGains0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfCT0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfSwtch0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLowPwrCfg0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAuxTssi0, 0)
		ACPHYREG_BCAST_ENTRY(pi, AfectrlOverride0, 0)
	ACPHY_REG_LIST_EXECUTE(pi);
	if ((ACMAJORREV_2(pi->pubpi->phy_rev) && !ACMINORREV_0(pi)) ||
			ACMAJORREV_5(pi->pubpi->phy_rev) ||
			ACMAJORREV_36(pi->pubpi->phy_rev)) {
		ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0);
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0);
	}

	/* logen_pwrup = 1, logen_reset = 0 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);

	/* Force LOGENs on both cores in the 5G to be powered up for 435x */
	if (ACMAJORREV_2(pi->pubpi->phy_rev) && !ACMINORREV_0(pi)) {
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, RfctrlOverrideTxPus0, logen5g_lob_pwrup, 1)
			MOD_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, logen5g_lob_pwrup, 1)
			MOD_PHYREG_ENTRY(pi, RfctrlOverrideTxPus1, logen5g_lob_pwrup, 1)
			MOD_PHYREG_ENTRY(pi, RfctrlCoreTxPus1, logen5g_lob_pwrup, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	/* 43012A0 related programming */
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideAfeDivCfg0, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideETCfg0, 0x0)
			WRITE_PHYREG_ENTRY(pi, AfeClkDivOverrideCtrl28nm0, 0x0)
			WRITE_PHYREG_ENTRY(pi, LogenOverrideCtrl28nm0, 0x0)
			WRITE_PHYREG_ENTRY(pi, ETOverrideCtrl28nm0, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfCtrlCorePwrSw0, 0x0)
			WRITE_PHYREG_ENTRY(pi, dyn_radiob0, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideNapPus0, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideLogenBias0, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0)
			WRITE_PHYREG_ENTRY(pi, Extra1AfeClkDivOverrideCtrl28nm0, 0x0)
			WRITE_PHYREG_ENTRY(pi, Extra2AfeClkDivOverrideCtrl28nm0, 0x0)
			MOD_PHYREG_ENTRY(pi, Dac_gain0, afe_iqdac_att, 0x000F)
			MOD_PHYREG_ENTRY(pi, RfctrlCmd, syncResetEn, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		/* 7271 related programming */
		ACPHY_REG_LIST_START
			//more override clear for 4347 28nm radio
			//TCL has the following next register set to 0x0
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeDivCfg0, 0x8f0b);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideETCfg0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrl28nm0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, LogenOverrideCtrl28nm0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfCtrlCorePwrSw0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, dyn_radiob0, 0x0);
			MOD_PHYREG_ENTRY(pi, Dac_gain0, afe_iqdac_att, 0xf);
			MOD_PHYREG_ENTRY(pi, Dac_gain1, afe_iqdac_att, 0xf);
			MOD_PHYREG_ENTRY(pi, Dac_gain2, afe_iqdac_att, 0xf);
			MOD_PHYREG_ENTRY(pi, Dac_gain3, afe_iqdac_att, 0xf);

			// Clear overrides specific to 43012
			// 7271: follow 4347

			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideNapPus0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLogenBias0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, Extra1AfeClkDivOverrideCtrl28nm0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, Extra2AfeClkDivOverrideCtrl28nm0, 0x0);
			MOD_PHYREG_ENTRY(pi, RfctrlCmd, syncResetEn, 0);
		ACPHY_REG_LIST_EXECUTE(pi);

	} else if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		/* Remove RFCTRL signal overrides for all cores */
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x306)
			WRITE_PHYREG_ENTRY(pi, AfeClkDivOverrideCtrl, 0x1)
			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrlN0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlIntc0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideTxPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideRxPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideGains0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfCT0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfSwtch0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeDivCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideETCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLowPwrCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAuxTssi0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLogenBias0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideNapPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, AfectrlOverride0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrl28nm0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, LogenOverrideCtrl28nm0, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
		/* for 4347, aux slice, setting rfpll_clk_buffer_pu using phy override */
		if (wlapi_si_coreunit(pi->sh->physhim) == DUALMAC_AUX) {
			WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xc);
		}
		else {
			WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
		}
		WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);
		MOD_PHYREG(pi, RfctrlCmd, chip_pu, 0x1);

	}

	/* Switch off rssi2 & rssi3 as they are not used in normal operation */
	ACPHYREG_BCAST(pi, RfctrlCoreRxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideRxPus0, 0x5000);

	/* Disable the SD-ADC's overdrive detect feature */
	val = READ_PHYREG(pi, RfctrlCoreAfeCfg20) |
	      ACPHY_RfctrlCoreAfeCfg20_afe_iqadc_reset_ov_det_MASK(pi->pubpi->phy_rev);
	/* val |= ACPHY_RfctrlCoreAfeCfg20_afe_iqadc_clamp_en_MASK; */
	ACPHYREG_BCAST(pi, RfctrlCoreAfeCfg20, val);
	val = READ_PHYREG(pi, RfctrlOverrideAfeCfg0) |
	      ACPHY_RfctrlOverrideAfeCfg0_afe_iqadc_reset_ov_det_MASK(pi->pubpi->phy_rev);
	/* val |= ACPHY_RfctrlOverrideAfeCfg0_afe_iqadc_clamp_en_MASK(pi->pubpi->phy_rev); */
	ACPHYREG_BCAST(pi, RfctrlOverrideAfeCfg0, val);

	/* initialize all the tables defined in auto-generated wlc_phytbl_ac.c,
	 * see acphyprocs.tcl, proc acphy_init_tbls
	 *  skip static one after first up
	 */
	PHY_TRACE(("wl%d: %s, dnld tables = %d\n", pi->sh->unit,
	           __FUNCTION__, pi->phy_init_por));

	/* these tables are not affected by phy reset, only power down */
	if (!RADIOID_IS(pi->pubpi->radioid, BCM20691_ID))
		wlc_phy_static_table_download_acphy(pi);

	if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
		/* wihr 0x15a 0x8000 */

		/* acphy_rfctrl_override bg_pulse 1 */
		/* Override is not required now ..bg pulsing is enabled along with tssi sleep */
		/* val  = READ_PHYREG(pi, RfctrlCoreTxPus0); */
		/* val |= (1 << 14); */
		/* WRITE_PHYREG(pi, ACPHY_RfctrlCoreTxPus0, val); */
		/* val = READ_PHYREG(pi, RfctrlOverrideTxPus0); */
		/* val |= (1 << 9); */
		/* WRITE_PHYREG(pi, ACPHY_RfctrlOverrideTxPus0, val); */

	}

	/* Initialize idle-tssi to be -420 before doing idle-tssi cal */
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_path0, -190);
	} else {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_path0, 0x25C);
	}
	if ((ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_2(pi->pubpi->phy_rev)) &&
	    BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_second_path0, 0x25C);
	}

	if (flag2rangeon) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_second_path0, 0x25C);
	}


	/* Enable the Ucode TSSI_DIV WAR */
	if ((ACMAJORREV_1(pi->pubpi->phy_rev) || ACMAJORREV_2(pi->pubpi->phy_rev)) &&
	    BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
		wlapi_bmac_mhf(pi->sh->physhim, MHF2, MHF2_PPR_HWPWRCTL, MHF2_PPR_HWPWRCTL,
		               WLC_BAND_ALL);
	}

	if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_36(pi->pubpi->phy_rev)) {
		uint8 lutinitval = 0;
		uint16 offset;
		if (BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
			wlapi_bmac_mhf(pi->sh->physhim, MHF2, MHF2_PPR_HWPWRCTL, MHF2_PPR_HWPWRCTL,
			WLC_BAND_2G);
		}
		/* Clean up the estPwrShiftLUT Table */
		/* Table is 26bit witdth, we are writing 32bit at a time */
		FOREACH_ACTV_CORE(pi, pi->sh->hw_phyrxchain, core) {
			for (offset = 0; offset < 40; offset++) {
				wlc_phy_table_write_acphy(pi,
					wlc_phy_get_tbl_id_estpwrshftluts(pi, core),
					1, offset, 32, &lutinitval);
			}
		}
	}

	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		PHY_INFORM(("rfseq_bundle_tbl : {0x01C7034 0x01C7014 0x02C703E 0x02C701C} \n"));

		/* set rfseq_bundle_tbl {0x01C7034 0x01C7014 0x02C703E 0x02C701C} */
		/* acphy_write_table RfseqBundle $rfseq_bundle_tbl 0 */
		rfseq_bundle_48[0] = 0x703C;
		rfseq_bundle_48[1] = 0x1c;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 0, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x7014;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 1, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x700e;
		rfseq_bundle_48[1] = 0x2c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 2, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x702c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 3, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x6020;
		rfseq_bundle_48[1] = 0x20;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 4, 48, rfseq_bundle_48);

		/* For verification */
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 0, 48, &rfseq_bundle_48);
		PHY_INFORM(("rfseq_bundle_tbl : offset %d, 0x%04x%04x%04x\n", 0,
			rfseq_bundle_48[2], rfseq_bundle_48[1], rfseq_bundle_48[0]));
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 1, 48, &rfseq_bundle_48);
		PHY_INFORM(("rfseq_bundle_tbl : offset %d, 0x%04x%04x%04x\n", 1,
			rfseq_bundle_48[2], rfseq_bundle_48[1], rfseq_bundle_48[0]));
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 2, 48, &rfseq_bundle_48);
		PHY_INFORM(("rfseq_bundle_tbl : offset %d, 0x%04x%04x%04x\n", 2,
			rfseq_bundle_48[2], rfseq_bundle_48[1], rfseq_bundle_48[0]));
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 3, 48, &rfseq_bundle_48);
		PHY_INFORM(("rfseq_bundle_tbl : offset %d, 0x%04x%04x%04x\n", 3,
			rfseq_bundle_48[2], rfseq_bundle_48[1], rfseq_bundle_48[0]));

		/* CLB FEM priority static settings */

		/* clb_swctrl_dmask_bt_ant0[9:0] */
		si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_08, 0x3ff<<17, 0x3ff<<17);
		/* clb_swctrl_smask_wlan_ant0[9:0] */
		si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_07, 0x3ff<<20, 0x3ff<<20);
		/* clb_swctrl_dmask_bt_ant1[9:0] */
		si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_10, 0x3ff<<10, 0x3ff<<10);
		/* clb_swctrl_smask_wlan_ant1[9:0] */
		si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_11, 0x3ff<<0, 0x3ff<<0);
		/* 4347_clb_reg_write clb_swctrl_smask_coresel_ant0_en 1 */
		si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_08, 0x1<<29, 0x1<<29);
		/* 4347_clb_reg_write clb_swctrl_smask_coresel_ant1_en 1 */
		si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_08, 0x1<<30, 0x1<<30);

	}

	if (TINY_RADIO(pi)) {
		FOREACH_CORE(pi, core) {
			MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, fast_nap_bias_pu, 0);
			MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, fast_nap_bias_pu, 1);
		}
	}

	/* Temp 4345 register poweron default updates - */
	/* These need to be farmed out to correct locations */
	if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, RxSdFeConfig5, rx_farow_scale_value, 7)
			MOD_PHYREG_ENTRY(pi, FSTRHiPwrTh, finestr_hiPwrSm_th, 77)
			MOD_PHYREG_ENTRY(pi, FSTRHiPwrTh, finestr_hiPwr_th, 63)
			MOD_PHYREG_ENTRY(pi, crsThreshold2l, peakThresh, 77)
			MOD_PHYREG_ENTRY(pi, crsminpoweruSub10, crsminpower0, 54)
			MOD_PHYREG_ENTRY(pi, crsminpoweruSub10, crsminpower0, 54)
			MOD_PHYREG_ENTRY(pi, crsminpoweru0, crsminpower0, 54)
			MOD_PHYREG_ENTRY(pi, bphyaciThresh0, bphyaciThresh0, 0)
			MOD_PHYREG_ENTRY(pi, bphyaciPwrThresh2, bphyaciPwrThresh2, 0)
			MOD_PHYREG_ENTRY(pi, RxSdFeSampCap, capture_both, 0)
			/* MOD_PHYREG_ENTRY(pi, TxMacDelay, macdelay, 680) */
			MOD_PHYREG_ENTRY(pi, AfeseqRx2TxPwrUpDownDly20M, delayPwrUpDownRx2Tx20M, 3)
			MOD_PHYREG_ENTRY(pi, RfctrlOverrideTxPus0, bg_pulse, 1)
			MOD_PHYREG_ENTRY(pi, PapdEnable0, gain_dac_rf_override0, 1)
			MOD_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, bg_pulse, 1)
			MOD_PHYREG_ENTRY(pi, PapdEnable0, gain_dac_rf_reg0, 0)
			MOD_PHYREG_ENTRY(pi, RfseqMode, CoreActv_override, 0)
			MOD_PHYREG_ENTRY(pi, RfctrlOverrideLowPwrCfg0, rxrf_lna2_gm_size, 0)
			MOD_PHYREG_ENTRY(pi, Rfpll_resetCtrl, rfpll_reset_dur, 64)
			MOD_PHYREG_ENTRY(pi, CoreConfig, NumRxAnt, 1)
			MOD_PHYREG_ENTRY(pi, Pllldo_resetCtrl, pllldo_reset_dur, 8000)
			MOD_PHYREG_ENTRY(pi, bphyaciPwrThresh0, bphyaciPwrThresh0, 0)
			MOD_PHYREG_ENTRY(pi, bphyaciThresh1, bphyaciThresh1, 0)
			MOD_PHYREG_ENTRY(pi, HTSigTones, support_gf, 1)
			MOD_PHYREG_ENTRY(pi, ViterbiControl0, CacheHitEn, 1)
			MOD_PHYREG_ENTRY(pi, sampleDepthCount, DepthCount, 19)
			MOD_PHYREG_ENTRY(pi, bphyaciPwrThresh1, bphyaciPwrThresh1, 0)
			MOD_PHYREG_ENTRY(pi, Core0cliploGainCodeB, clip1loTrTxIndex, 0)
			MOD_PHYREG_ENTRY(pi, clip2carrierDetLen, clip2carrierDetLen, 64)
			MOD_PHYREG_ENTRY(pi, Core0clip2GainCodeA, clip2LnaIndex, 1)
			MOD_PHYREG_ENTRY(pi, Core0clipmdGainCodeB, clip1mdBiQ0Index, 0)
			MOD_PHYREG_ENTRY(pi, Core0clip2GainCodeB, clip2BiQ1Index, 2)
			MOD_PHYREG_ENTRY(pi, Core0cliploGainCodeA, clip1loLnaIndex, 1)
			MOD_PHYREG_ENTRY(pi, Core0cliploGainCodeB, clip1loBiQ0Index, 0)
			MOD_PHYREG_ENTRY(pi, Core0clipHiGainCodeB, clip1hiBiQ0Index, 0)
			MOD_PHYREG_ENTRY(pi, Core0clipmdGainCodeA, clip1mdmixergainIndex, 3)
			MOD_PHYREG_ENTRY(pi, Core0cliploGainCodeB, clip1loTrRxIndex, 1)
			WRITE_PHYREG_ENTRY(pi, Core0InitGainCodeB, 0x2204)
			MOD_PHYREG_ENTRY(pi, AfectrlCore10, adc_pd, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
		uint8 zeroval[4] = { 0 };
		uint16 x;
		uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);

		ACPHY_DISABLE_STALL(pi);

		for (x = 0; x < 256; x++) {
			/* Zero out the NV Noise Shaping Table for 4345A0 */
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_NVNOISESHAPINGTBL,
			                          1, x, 32, zeroval);

			/* Zero out the NV Rx EVM Table for 4345A0 */
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_NVRXEVMSHAPINGTBL,
			                          1, x, 8, zeroval);
		}
		ACPHY_ENABLE_STALL(pi, stall_val);

		/* Disable scram dyn en for 4345A0 */
		wlc_acphy_set_scramb_dyn_bw_en((wlc_phy_t *)pi, 0);
	}

	/* 4335C0: This force clk is needed for proper read/write of RfseqBundle table.
	This change is inaccordance with JIRA CRDOT11ACPHY-436
	Dated Mar-08 : Removing force clks for low current
	if (ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
		MOD_PHYREG(pi, fineclockgatecontrol, forceRfSeqgatedClksOn, 1);
	}
	*/

	if ((TINY_RADIO(pi) || (ACMAJORREV_36(pi->pubpi->phy_rev))) &&
		(PHY_IPA(pi) || phy_papdcal_epacal(pi->papdcali))) {
		/* Zero out paplutselect table used in txpwrctrl */
		uint8 initvalue = 0;
		uint16 j;
		for (j = 0; j < 128; j++) {
			wlc_phy_table_write_acphy(pi,
				ACPHY_TBL_ID_PAPDLUTSELECT0, 1, j, 8, &initvalue);
			if (ACMAJORREV_4(pi->pubpi->phy_rev) &&
				(phy_get_phymode(pi) == PHYMODE_MIMO)) {
				wlc_phy_table_write_acphy(pi,
					ACPHY_TBL_ID_PAPDLUTSELECT1, 1, j, 8, &initvalue);
			}
		}
	}

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		// still empty here
	}

	if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		ACPHY_REG_LIST_START

			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0x0);
			WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x306);
			WRITE_PHYREG_ENTRY(pi, AfeClkDivOverrideCtrl, 0x1);

			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrlN0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlIntc0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideTxPus0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideRxPus0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideGains0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfCT0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfSwtch0, 0x0);
			//TCL has the following register write set to 0x0
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeDivCfg0, 0x8f0b);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideETCfg0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLowPwrCfg0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAuxTssi0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLogenBias0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideNapPus0, 0x0);
			ACPHYREG_BCAST_ENTRY(pi, AfectrlOverride0, 0x0);

			WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0x4);
			//using phy override
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0xd);

			MOD_PHYREG_ENTRY(pi, RfctrlCmd, chip_pu, 1);
		ACPHY_REG_LIST_EXECUTE(pi);

		/* set rfseq_bundle_tbl {0x01C7034 0x01C7014 0x02C703E 0x02C701C} */
		/* acphy_write_table RfseqBundle $rfseq_bundle_tbl 0 */
		/* acphy_write_table RfseqBundle */
		/* {0x01C703C 0x01C7014 0x02C700E 0x02C702C 0x0206020 0x400 0x0} 0; */
		rfseq_bundle_48[0] = 0x703c;
		rfseq_bundle_48[1] = 0x1c;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 0, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x7014;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 1, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x700e;
		rfseq_bundle_48[1] = 0x2c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 2, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x702c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 3, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x6020;
		rfseq_bundle_48[1] = 0x20;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 4, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x0400;
		rfseq_bundle_48[1] = 0x00;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 5, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x0000;
		rfseq_bundle_48[1] = 0x00;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 6, 48,
				rfseq_bundle_48);

	}
}

void
wlc_phy_radio2069_pwrdwn_seq(phy_ac_radio_info_t *ri)
{
	phy_info_t *pi = ri->pi;

	/* AFE */
	ri->afeRfctrlCoreAfeCfg10 = READ_PHYREG(pi, RfctrlCoreAfeCfg10);
	ri->afeRfctrlCoreAfeCfg20 = READ_PHYREG(pi, RfctrlCoreAfeCfg20);
	ri->afeRfctrlOverrideAfeCfg0 = READ_PHYREG(pi, RfctrlOverrideAfeCfg0);
	ACPHY_REG_LIST_START
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreAfeCfg10, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreAfeCfg20, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x1fff)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* Radio RX */
	ri->rxRfctrlCoreRxPus0 = READ_PHYREG(pi, RfctrlCoreRxPus0);
	ri->rxRfctrlOverrideRxPus0 = READ_PHYREG(pi, RfctrlOverrideRxPus0);
	WRITE_PHYREG(pi, RfctrlCoreRxPus0, 0x40);
	WRITE_PHYREG(pi, RfctrlOverrideRxPus0, 0xffbf);

	/* Radio TX */
	ri->txRfctrlCoreTxPus0 = READ_PHYREG(pi, RfctrlCoreTxPus0);
	ri->txRfctrlOverrideTxPus0 = READ_PHYREG(pi, RfctrlOverrideTxPus0);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, 0);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* {radio, rfpll, pllldo}_pu = 0 */
	ri->radioRfctrlCmd = READ_PHYREG(pi, RfctrlCmd);
	ri->radioRfctrlCoreGlobalPus = READ_PHYREG(pi, RfctrlCoreGlobalPus);
	ri->radioRfctrlOverrideGlobalPus = READ_PHYREG(pi, RfctrlOverrideGlobalPus);
	ACPHY_REG_LIST_START
		MOD_PHYREG_ENTRY(pi, RfctrlCmd, chip_pu, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);
}

void
wlc_phy_radio2069_pwrup_seq(phy_ac_radio_info_t *ri)
{
	phy_info_t *pi = ri->pi;

	/* AFE */
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg10, ri->afeRfctrlCoreAfeCfg10);
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg20, ri->afeRfctrlCoreAfeCfg20);
	WRITE_PHYREG(pi, RfctrlOverrideAfeCfg0, ri->afeRfctrlOverrideAfeCfg0);

	/* Restore Radio RX */
	WRITE_PHYREG(pi, RfctrlCoreRxPus0, ri->rxRfctrlCoreRxPus0);
	WRITE_PHYREG(pi, RfctrlOverrideRxPus0, ri->rxRfctrlOverrideRxPus0);

	/* Radio TX */
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, ri->txRfctrlCoreTxPus0);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, ri->txRfctrlOverrideTxPus0);

	/* {radio, rfpll, pllldo}_pu = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, ri->radioRfctrlCmd);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, ri->radioRfctrlCoreGlobalPus);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, ri->radioRfctrlOverrideGlobalPus);
}

void
wlc_acphy_get_radio_loft(phy_info_t *pi,
	uint8 *ei0,
	uint8 *eq0,
	uint8 *fi0,
	uint8 *fq0)
{
	/* Not required for 4345 */
	*ei0 = 0;
	*eq0 = 0;
	*fi0 = 0;
	*fq0 = 0;
}

void
wlc_acphy_set_radio_loft(phy_info_t *pi,
	uint8 ei0,
	uint8 eq0,
	uint8 fi0,
	uint8 fq0)
{
	/* Not required for 4345 */
	return;
}

void
wlc_phy_radio20693_config_bf_mode(phy_info_t *pi, uint8 core)
{
	uint8 pupd, band_sel;
	uint8 afeBFpu0 = 0, afeBFpu1 = 0, afeclk_div12g_pu = 1;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlapi_txbf_enab(pi->sh->physhim)) {
		if (phy_get_phymode(pi) == PHYMODE_MIMO) {
			if (core == 1) {
				afeBFpu0 = 0;
				afeBFpu1 = 1;
				afeclk_div12g_pu = 0;
			} else {
				afeBFpu0 = 1;
				afeBFpu1 = 0;
				afeclk_div12g_pu = 1;
			}
		}
		pupd = 1;
	} else {
		pupd = 0;
	}

	MOD_RADIO_REG_20693(pi, SPARE_CFG8, core, afe_BF_pu0, afeBFpu0);
	MOD_RADIO_REG_20693(pi, SPARE_CFG8, core, afe_BF_pu1, afeBFpu1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, afe_clk_div_12g_pu, afeclk_div12g_pu);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_pu, 0x1);

	band_sel = 0;

	if (CHSPEC_IS2G(pi->radio_chanspec))
		band_sel = 1;

	/* Set the 2G divider PUs */

	/* MOD_RADIO_REG_20693(pi, RXMIX2G_CFG1, core, rxmix2g_pu, 1); */
	MOD_RADIO_REG_20693(pi, TX_LOGEN2G_CFG1, core, logen2g_tx_pu_bias, (pupd & band_sel));
	/* Set the corresponding 2G Ovrs */
	/* MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST2, core, ovr_rxmix2g_pu, 1); */
	MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core, ovr_logen2g_tx_pu_bias,
		(pupd & band_sel));

	/* Set the 5G divider PUs */
	/* MOD_RADIO_REG_20693(pi, LNA5G_CFG3, core, mix5g_en, (pupd & (1-band_sel))); */
	MOD_RADIO_REG_20693(pi, TX_LOGEN5G_CFG1, core, logen5g_tx_enable_5g_low_band,
		(pupd & (1-band_sel)));
	/* Set the corresponding 5G Ovrs */
	/* MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_mix5g_en, (pupd & (1-band_sel))); */
	MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR2, core, ovr_logen5g_tx_enable_5g_low_band,
		(pupd & (1-band_sel)));
}

static void
wlc_phy_radio20693_mimo_cm1_lpopt(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi), temp;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);

	/* Turn off core 1 PMU + other lp optimizations */
	ASSERT(!porig->is_orig);
	porig->is_orig = TRUE;
	PHY_TRACE(("%s: Applying core 0 optimizations\n", __FUNCTION__));

	wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(pi);

	ACPHY_REG_LIST_START
		/* Core 0 Settings */
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_lowquiescenten,
			0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_pull_down_sw, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_CP1, 0, rfpll_cp_bias_low_power, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, VREG_CFG, 0, vreg25_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, VREG_OVR1, 0, ovr_vreg25_pu, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* MEM opt. Setting wlpmu_en, wlpmu_VCOldo_pu, wlpmu_TXldo_pu,
	   wlpmu_AFEldo_pu, wlpmu_RXldo_pu
	 */
	temp = READ_RADIO_REG_TINY(pi, PMU_OP, 0);
	phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OP, 0), (temp | 0x009E));

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_20693_ENTRY(pi, PMU_CFG4, 0, wlpmu_ADCldo_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_CFG2, 0, logencore_det_stg1_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR1, 0, ovr_logencore_det_stg1_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_CFG3, 0, logencore_2g_mimodes_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 0, ovr_logencore_2g_mimodes_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_CFG3, 0, logencore_2g_mimosrc_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 0, ovr_logencore_2g_mimosrc_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_CFG1, 0, afe_clk_div_6g_mimo_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 0, ovr_afeclkdiv_6g_mimo_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_CFG1, 0, afe_clk_div_12g_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 0, ovr_afeclkdiv_12g_pu, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);

	MOD_RADIO_REG_20693(pi, SPARE_CFG2, 0, afe_clk_div_se_drive, 0x0);
	/* Power down core0 MIMO LO buffer */
	MOD_RADIO_REG_20693(pi, LOGEN_OVR2, 0, ovr_logencore_5g_mimosrc_pu, 0x1);
	MOD_RADIO_REG_20693(pi, LOGEN_CFG3, 0, logencore_5g_mimosrc_pu, 0x0);

	if (pi->u.pi_acphy->both_txchain_rxchain_eq_1 == TRUE) {
		PHY_TRACE(("%s: Applying core 1 optimizations\n", __FUNCTION__));
		ACPHY_REG_LIST_START
			/* Core 1 Settings */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1,
				ldo_1p2_xtalldo1p2_lowquiescenten, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1,
				ldo_1p2_xtalldo1p2_pull_down_sw, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, AUXPGA_OVR1, 1, ovr_auxpga_i_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, VREG_CFG, 1, vreg25_pu, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, VREG_OVR1, 1, ovr_vreg25_pu, 0x1)
		ACPHY_REG_LIST_EXECUTE(pi);

		/* MEM opt. UN-Setting wlpmu_en, wlpmu_VCOldo_pu, wlpmu_TXldo_pu,
		   wlpmu_AFEldo_pu, wlpmu_RXldo_pu
		 */
		temp = READ_RADIO_REG_TINY(pi, PMU_OP, 1);
		phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OP, 1), (temp & ~0x009E));

		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20693_ENTRY(pi, PMU_CFG4, 1, wlpmu_ADCldo_pu, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR1, 1, ovr_logencore_det_stg1_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 1,
				ovr_logencore_2g_mimodes_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 1,
				ovr_logencore_2g_mimosrc_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 1,
				ovr_afeclkdiv_6g_mimo_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 1, ovr_afeclkdiv_12g_pu, 0x1)
			/* Power down core1 XTAL LDO */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1,
				ldo_1p2_xtalldo1p2_BG_pu, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, 1,
				ovr_ldo_1p2_xtalldo1p2_BG_pu, 0x1)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}
static void wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	uint8 core;
	uint16 *ptr_start = (uint16 *)&porig->pll_xtalldo1[0];

	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
			RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
			RADIO_REG_20693(pi, PLL_CP1, core),
			RADIO_REG_20693(pi, VREG_CFG, core),
			RADIO_REG_20693(pi, VREG_OVR1, core),
			RADIO_REG_20693(pi, PMU_OP, core),
			RADIO_REG_20693(pi, PMU_CFG4, core),
			RADIO_REG_20693(pi, LOGEN_CFG2, core),
			RADIO_REG_20693(pi, LOGEN_OVR1, core),
			RADIO_REG_20693(pi, LOGEN_CFG3, core),
			RADIO_REG_20693(pi, LOGEN_OVR2, core),
			RADIO_REG_20693(pi, CLK_DIV_CFG1, core),
			RADIO_REG_20693(pi, CLK_DIV_OVR1, core),
			RADIO_REG_20693(pi, SPARE_CFG2, core),
			RADIO_REG_20693(pi, AUXPGA_OVR1, core),
			RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core)
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &porig->tx_top_2g_ovr_east[PHY_CORE_MAX-1]);
			*ptr = _READ_RADIO_REG(pi, porig_offsets[ct]);
		}
	}
}
void wlc_phy_radio20693_mimo_lpopt_restore(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);

	ASSERT(!((pmu_lp_opt_orig->is_orig == TRUE) && (porig->is_orig == TRUE)));


	if (porig->is_orig == TRUE) {
		wlc_phy_radio20693_mimo_cm1_lpopt_restore(pi);
	} else if (pmu_lp_opt_orig->is_orig == TRUE) {
		wlc_phy_radio20693_mimo_cm23_lp_opt_restore(pi);
	}
}
static void wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);
	uint16 *ptr_start = (uint16 *)&pmu_lp_opt_orig->pll_xtalldo1[0];

	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
			RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
			RADIO_REG_20693(pi, PMU_OP, core),
			RADIO_REG_20693(pi, PMU_OVR, core),
			RADIO_REG_20693(pi, PMU_CFG4, core),
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &pmu_lp_opt_orig->pmu_cfg4[PHY_CORE_MAX-1]);
			*ptr = _READ_RADIO_REG(pi, porig_offsets[ct]);
		}
	}

}

static void
wlc_phy_radio20693_mimo_cm23_lp_opt(phy_info_t *pi, uint8 coremask)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);

	ASSERT(phy_get_phymode(pi) == PHYMODE_MIMO);
	ASSERT(coremask != 1);
	ASSERT(!pmu_lp_opt_orig->is_orig);
	pmu_lp_opt_orig->is_orig = TRUE;

	PHY_TRACE(("%s: Applying CM2/CM3 radio LP optimizations for coremask : %d\n",
		__FUNCTION__, coremask));

	wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(pi);

	/* Power down core1 XTAL LDO */
	MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_BG_pu, 0x0);
	MOD_RADIO_REG_20693(pi, PLL_XTAL_OVR1, 1, ovr_ldo_1p2_xtalldo1p2_BG_pu, 0x1);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, core, ldo_1p2_xtalldo1p2_pull_down_sw, 0x0);
	}
}

static void
wlc_phy_radio20693_mimo_cm23_lp_opt_restore(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);
	uint16 *ptr_start = (uint16 *)&pmu_lp_opt_orig->pll_xtalldo1[0];


	ASSERT(phy_get_phymode(pi) == PHYMODE_MIMO);
	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	if (pmu_lp_opt_orig->is_orig == TRUE) {
		pmu_lp_opt_orig->is_orig = FALSE;
		PHY_TRACE(("%s: Restoring the radio lp settings\n", __FUNCTION__));

		FOREACH_CORE(pi, core) {
			uint8 ct;
			uint16 *ptr;
			uint16 porig_offsets[] = {
				RADIO_REG_20693(pi, PLL_XTALLDO1, core),
				RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
				RADIO_REG_20693(pi, PMU_OP, core),
				RADIO_REG_20693(pi, PMU_OVR, core),
				RADIO_REG_20693(pi, PMU_CFG4, core),
			};
			/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
			   the data access logic goes beyond this length to access next set of
			   elements, assuming contiguous memory allocation.
			   So, ARRAY OVERRUN is intentional here
			 */
			for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
				ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
				ASSERT(ptr <= &pmu_lp_opt_orig->pmu_cfg4[PHY_CORE_MAX-1]);
				phy_utils_write_radioreg(pi, porig_offsets[ct], *ptr);
			}
		}
		/* JIRA: SW4349-1462:
		 * fix for '[4355] idle tssi for core 0 is coming as zero
		 * when txchain/rxchain are 2'
		 */
		wlc_phy_radio20693_minipmu_pwron_seq(pi);
	}
}

static void
wlc_phy_radio20693_mimo_cm1_lpopt_restore(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	uint16 *ptr_start = (uint16 *)&porig->pll_xtalldo1[0];
	uint8 core;

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);
	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	ASSERT(porig->is_orig);
	porig->is_orig = FALSE;
	PHY_TRACE(("%s: Restoring the core0/1 optimizations to their def values\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
			RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
			RADIO_REG_20693(pi, PLL_CP1, core),
			RADIO_REG_20693(pi, VREG_CFG, core),
			RADIO_REG_20693(pi, VREG_OVR1, core),
			RADIO_REG_20693(pi, PMU_OP, core),
			RADIO_REG_20693(pi, PMU_CFG4, core),
			RADIO_REG_20693(pi, LOGEN_CFG2, core),
			RADIO_REG_20693(pi, LOGEN_OVR1, core),
			RADIO_REG_20693(pi, LOGEN_CFG3, core),
			RADIO_REG_20693(pi, LOGEN_OVR2, core),
			RADIO_REG_20693(pi, CLK_DIV_CFG1, core),
			RADIO_REG_20693(pi, CLK_DIV_OVR1, core),
			RADIO_REG_20693(pi, SPARE_CFG2, core),
			RADIO_REG_20693(pi, AUXPGA_OVR1, core),
			RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core)
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &porig->tx_top_2g_ovr_east[PHY_CORE_MAX-1]);
			phy_utils_write_radioreg(pi, porig_offsets[ct], *ptr);
		}
	}
	wlc_phy_radio20693_minipmu_pwron_seq(pi);
	wlc_phy_tiny_radio_minipmu_cal(pi);
	wlc_phy_radio20693_pmu_pll_config(pi);
	wlc_phy_radio20693_afecal(pi);
}

void wlc_phy_radio20693_mimo_core1_pmu_restore_on_bandhcange(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);
	bool for_2g = ((pi_ac->radioi->lpmode_2g != ACPHY_LPMODE_NONE) &&
		(CHSPEC_IS2G(pi->radio_chanspec)));
	bool for_5g = ((pi_ac->radioi->lpmode_5g != ACPHY_LPMODE_NONE) &&
		(CHSPEC_IS5G(pi->radio_chanspec)));

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);

	if (((for_2g == TRUE) || (for_5g == TRUE))) {
		if (porig->is_orig == TRUE) {
			wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(pi);
		} else if (pmu_lp_opt_orig->is_orig == TRUE) {
			wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(pi);
		}
	}
}

static void wlc_phy_radio20693_tx2g_set_freq_tuning_ipa_as_epa(phy_info_t *pi,
	uint8 core, uint8 chan)
{
	uint8 mix2g_tune_tbl[] = { 0x4, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x2, 0x2, 0x2,
		0x2, 0x2, 0x1};
	uint8 mix2g_tune_val;
	ASSERT((chan >= 1) && (chan <= 14));
	mix2g_tune_val = mix2g_tune_tbl[chan - 1];
	MOD_RADIO_REG_20693(pi, TXMIX2G_CFG5, core,
		mx2g_tune, mix2g_tune_val);
}

void
phy_ac_dsi_radio_fns(phy_info_t *pi)
{
	wlc_phy_radio2069_mini_pwron_seq_rev16(pi);
	wlc_phy_radio2069_pwron_seq(pi);
	wlc_phy_radio2069_vcocal(pi);
	wlc_phy_radio2069x_vcocal_isdone(pi, TRUE, FALSE);
}

static void
chanspec_setup_radio_20693(phy_info_t *pi)
{
	uint8 core = 0;
	const chan_info_radio20693_pll_t *chan_info_pll;
	const chan_info_radio20693_rffe_t *chan_info_rffe;
	const chan_info_radio20693_pll_wave2_t *chan_info_pll_wave2;
	uint8 ch[NUM_CHANS_IN_CHAN_BONDING], n_freq_seg;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	chanspec_get_operating_channels(pi, ch);
	pi_ac->fc = wlc_phy_chan2freq_20693(pi, ch[0], &chan_info_pll, &chan_info_rffe,
		&chan_info_pll_wave2);

	if (pi_ac->fc >= 0) {
		if (RADIOMAJORREV(pi) == 3) {
			if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
				PHY_INFORM(("wl%d: %s: setting chan1=%d, chan2=%d...\n",
						pi->sh->unit, __FUNCTION__, ch[0], ch[1]));
				if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac)) {
					phy_ac_radio_20693_pmu_pll_config_wave2(pi, 7);
				}
				/* setting both pll cores - mode = 1 */
				phy_ac_radio_20693_chanspec_setup(pi, ch[0],
					CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0, 1);
			} else if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0);
			} else {
				phy_ac_radio_20693_chanspec_setup(pi, ch[0],
					CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0, 0);
			}
			if (CCT_INIT(pi_ac) || CCT_BW_CHG_80P80(pi_ac)) {
				if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
					wlc_phy_radio20693_afe_clkdistribtion_mode(pi, 1);
				} else {
					wlc_phy_radio20693_afe_clkdistribtion_mode(pi, 0);
				}
			}
		} else {
			if (phy_get_phymode(pi) == PHYMODE_80P80) {
				for (n_freq_seg = 0; n_freq_seg < NUM_CHANS_IN_CHAN_BONDING;
						n_freq_seg++) {
					phy_ac_radio_20693_chanspec_setup(pi, ch[n_freq_seg],
						CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac),
						n_freq_seg, 0);
				}
			} else {
				FOREACH_CORE(pi, core) {
					phy_ac_radio_20693_chanspec_setup(pi, ch[0],
						CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), core, 0);
				}
			}
		}
	}
}

static void
chanspec_setup_radio_20691(phy_info_t *pi)
{
	const chan_info_radio20691_t *chan_info_20691;
	uint8 ch[NUM_CHANS_IN_CHAN_BONDING];

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	chanspec_get_operating_channels(pi, ch);
	pi_ac->fc = wlc_phy_chan2freq_20691(pi, ch[0], &chan_info_20691);

	if (pi_ac->fc >= 0) {
		wlc_phy_chanspec_radio20691_setup(pi, ch[0],
			CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac));
	}
}

static void
chanspec_setup_radio_2069(phy_info_t *pi)
{
	uint8 ch[NUM_CHANS_IN_CHAN_BONDING];
	const void *chan_info = NULL;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* 4335
	 * low power mode selection based on 2G or 5G
	 */
	if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
		if (CHSPEC_IS2G(pi->radio_chanspec))
			pi_ac->radioi->acphy_lp_mode = 2;
		else
			pi_ac->radioi->acphy_lp_mode = 1;

		pi_ac->radioi->acphy_prev_lp_mode = pi_ac->radioi->acphy_lp_mode;
		pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
	}

	chanspec_get_operating_channels(pi, ch);
	pi_ac->fc = wlc_phy_chan2freq_acphy(pi, ch[0], &chan_info);

	if (pi_ac->fc >= 0) {
		wlc_phy_chanspec_radio2069_setup(pi, chan_info,
			CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac));
	}
}

static void
chanspec_tune_radio_20693(phy_info_t *pi)
{
	uint16 dac_rate;
	uint8 lpf_gain = 1, lpf_bw, biq_bw_ofdm, biq_bw_cck, core;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		pi->u.pi_acphy->dac_mode = 1;
	} else {
		pi_ac->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ? pi->dacratemode2g :
			pi->dacratemode5g;
		wlc_phy_dac_rate_mode_acphy(pi, pi_ac->dac_mode);
	}

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac)) {
				/* bw_change requires afe cal. */
				/* so that all the afe_iqadc signals are correctly set */
				wlc_phy_radio_afecal(pi);
			}
		}
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				lpf_bw = 1;
				lpf_gain = -1;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				lpf_bw = 3;
				lpf_gain = -1;
			} else {
				lpf_bw = 3;
				lpf_gain = -1;
			}
		} else {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				lpf_bw = 3;
				lpf_gain = -1;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				lpf_bw = 3;
				lpf_gain = -1;
			} else {
				lpf_bw = 3;
				lpf_gain = -1;
			}
		}
	} else {
		dac_rate = wlc_phy_get_dac_rate_from_mode(pi, pi_ac->dac_mode);

		/* For 4349 do afecal on chan change to take care of RSDB dac spurs */
		wlc_phy_radio_afecal(pi);

		/* tune lpf setting for 20693 based on dac rate */
		if (!(PHY_IPA(pi))) {
			if (dac_rate == 200) {
				lpf_bw = 1;
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					lpf_bw = 1;
					lpf_gain = 1;
				} else {
					lpf_bw = 2;
					lpf_gain = 0;
				}
			} else if (dac_rate == 400) {
				lpf_bw = 3;
				lpf_gain = 0;
			} else {
				lpf_bw = 5;
				lpf_gain = 0;
			}
		} else {
			if (dac_rate == 200) {
				lpf_bw = 2;
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					lpf_gain = 2;
				} else {
					lpf_gain = 0;
				}
			} else if (dac_rate == 400) {
				lpf_bw = 3;
				lpf_gain = 0;
			} else {
				lpf_bw = 5;
				lpf_gain = 0;
			}
		}
	}

	if (!(PHY_IPA(pi)) && (RADIOREV(pi->pubpi->radiorev) == 13)) {
		lpf_gain = 0;
	} else {
		if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) &&
		RADIOMAJORREV(pi) == 3)) {
			lpf_gain = CHSPEC_IS2G(pi->radio_chanspec) ? 2 : 0;
		}
	}

	biq_bw_ofdm = lpf_bw;
	biq_bw_cck = lpf_bw - 1;

	/* WAR for FDIQI when bq_bw = 9, 25 MHz */
	if (!PHY_PAPDEN(pi) && !PHY_IPA(pi) && CHSPEC_IS2G(pi->radio_chanspec) &&
		CHSPEC_IS20(pi->radio_chanspec) && !(RADIOREV(pi->pubpi->radiorev) == 13)) {
		biq_bw_ofdm = lpf_bw - 1;
	}

	wlc_phy_radio_tiny_lpf_tx_set(pi, lpf_bw, lpf_gain, biq_bw_ofdm, biq_bw_cck);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		wlc_phy_radio20693_force_dacbuf_setting(pi);
	} else {
		/* dac swap */
		FOREACH_CORE(pi, core) {
			if (core == 0)
				MOD_PHYREG(pi, Core1TxControl, iqSwapEnable, 1);
			else if (core == 1)
				MOD_PHYREG(pi, Core2TxControl, iqSwapEnable, 1);
		}

		/* adc swap */
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq0, 1);
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq1, 1);
	}

	OSL_DELAY(20);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
			uint8 bands[NUM_CHANS_IN_CHAN_BONDING];
			phy_ac_chanmgr_get_chan_freq_range_80p80(pi, 0, bands);
			pi_ac->radioi->prev_subband = bands[0];
			PHY_INFORM(("wl%d: %s: FIXME for 80P80\n", pi->sh->unit, __FUNCTION__));
		} else {
			pi_ac->radioi->prev_subband =
				phy_ac_chanmgr_get_chan_freq_range(pi, 0, PRIMARY_FREQ_SEGMENT);
		}
	}
}

static void
chanspec_tune_radio_20691(phy_info_t *pi)
{
	uint8 lpf_gain, lpf_bw, biq_bw_ofdm, biq_bw_cck, core;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* The VCO Calibration clock driver cannot be off until after VCO Cal is done */
	MOD_RADIO_REG_20691(pi, PLL_XTAL2, 0, xtal_pu_caldrv, 0x0);

	pi_ac->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ? pi->dacratemode2g : pi->dacratemode5g;
	wlc_phy_dac_rate_mode_acphy(pi, pi_ac->dac_mode);

	/* bw_change requires afe cal. */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac))
		wlc_phy_radio_afecal(pi);

	/* tune lpf settings for 20691 */
	if (CHSPEC_IS20(pi->radio_chanspec))
		if (IS_4364_1x1(pi)) {
			lpf_bw = 6;
		} else {
			lpf_bw = 2;
		}
	else if (CHSPEC_IS40(pi->radio_chanspec))
		lpf_bw = 3;
	else
		lpf_bw = 5;

	if (!(PHY_IPA(pi)) && (RADIOREV(pi->pubpi->radiorev) == 13))
		lpf_gain = 0;
	else
		lpf_gain = CHSPEC_IS2G(pi->radio_chanspec) ? 2 : 0;

	biq_bw_ofdm = lpf_bw;

	/* WAR for FDIQI when bq_bw = 9, 25 MHz */
	if (!PHY_PAPDEN(pi) && !PHY_IPA(pi) && CHSPEC_IS2G(pi->radio_chanspec) &&
		CHSPEC_IS20(pi->radio_chanspec) && !(RADIOREV(pi->pubpi->radiorev) == 13)) {
			biq_bw_ofdm = lpf_bw - 1;
	}

	if (IS_4364_1x1(pi)) {
		biq_bw_cck = 1;
	} else {
		biq_bw_cck = lpf_bw - 1;
	}

	wlc_phy_radio_tiny_lpf_tx_set(pi, lpf_bw, lpf_gain, biq_bw_ofdm, biq_bw_cck);

	/* dac swap */
	FOREACH_CORE(pi, core) {
		if (core == 0)
			MOD_PHYREG(pi, Core1TxControl, iqSwapEnable, 1);
		else if (core == 1)
			MOD_PHYREG(pi, Core2TxControl, iqSwapEnable, 1);
	}

	/* REV7 fixed adc swap */
	MOD_PHYREG(pi, RxFeCtrl1, swap_iq0, 1);

	OSL_DELAY(20);
}

static void
chanspec_tune_radio_2069(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 tx_gain_tbl_id = wlc_phy_get_tbl_id_gainctrlbbmultluts(pi, 0);

	/* bw_change requires afe cal. */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac))
		wlc_phy_radio_afecal(pi);

	/* Applicable for non-TINY radios */
	if (BF3_VLIN_EN_FROM_NVRAM(pi_ac)) {
		uint16 txidxval, txgaintemp1[3], txgaintemp1a[3];
		uint16 tempmask;
		uint16 vlinmask;
		if (pi_ac->radioi->prev_subband != 15) {
			for (txidxval = phy_ac_chanmgr_get_data(pi_ac->chanmgri)->vlin_txidx;
					txidxval < 128; txidxval++) {
				wlc_phy_table_read_acphy(pi, tx_gain_tbl_id, 1,
						txidxval, 48, &txgaintemp1);
				txgaintemp1a[0] = (txgaintemp1[0] & 0x7FFF) -
					(phy_ac_chanmgr_get_data(pi_ac->chanmgri)->bbmult_comp);
				txgaintemp1a[1] = txgaintemp1[1];
				txgaintemp1a[2] = txgaintemp1[2];
				wlc_phy_tx_gain_table_write_acphy(pi, 1,
						txidxval, 48, txgaintemp1a);
			}
		}
		tempmask = READ_PHYREGFLD(pi, FemCtrl, femCtrlMask);
		if (CHSPEC_IS2G (pi->radio_chanspec)) {
			vlinmask =
			1<<(phy_ac_chanmgr_get_data(pi_ac->chanmgri)->vlinmask2g_from_nvram);
		}
		else {
			vlinmask =
			1<<(phy_ac_chanmgr_get_data(pi_ac->chanmgri)->vlinmask5g_from_nvram);
		}
		MOD_PHYREG(pi, FemCtrl, femCtrlMask, (tempmask|vlinmask));
		wlc_phy_vlin_en_acphy(pi);
	}
	pi_ac->radioi->prev_subband =
		phy_ac_chanmgr_get_chan_freq_range(pi, 0, PRIMARY_FREQ_SEGMENT);
}

static void
chanspec_setup_radio_20695(phy_info_t *pi)
{
	uint8 core = 0;
	const chan_info_radio20695_rffe_t *chan_info;
	uint8 ch_num;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	ch_num =  CHSPEC_CHANNEL(pi->radio_chanspec);

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	pi_ac->fc = wlc_phy_chan2freq_20695(pi, ch_num,
			&chan_info);

	if (pi_ac->fc >= 0) {
		FOREACH_CORE(pi, core) {
			/* If band has changed turn on required PLL and LOGEN */
			if (CCT_BAND_CHG(pi_ac)) {
				wlc_phy_radio20695_pll_logen_pu_config(pi, chan_info->freq, core);
			}

			wlc_phy_chanspec_radio20695_setup(pi, ch_num,
				CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), core);
		}
	}
}


static void
chanspec_setup_xtal_20695(phy_info_t *pi)
{
	si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL9,
			PMUCCTL09_43012_XTAL_CORESIZE_BIAS_ADJ_NORMAL_MASK,
			(0x4 << PMUCCTL09_43012_XTAL_CORESIZE_BIAS_ADJ_NORMAL_SHIFT));
	si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL9,
			PMUCCTL09_43012_XTAL_CORESIZE_RES_BYPASS_NORMAL_MASK,
			(0x4 << PMUCCTL09_43012_XTAL_CORESIZE_RES_BYPASS_NORMAL_SHIFT));
	si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL8,
			PMUCCTL08_43012_XTAL_CORE_SIZE_NMOS_NORMAL_MASK,
			(0x10 << PMUCCTL08_43012_XTAL_CORE_SIZE_NMOS_NORMAL_SHIFT));
	si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL8,
			PMUCCTL08_43012_XTAL_CORE_SIZE_PMOS_NORMAL_MASK,
			(0x10 << PMUCCTL08_43012_XTAL_CORE_SIZE_PMOS_NORMAL_SHIFT));

	if (RADIOREV(pi->pubpi->radiorev) == 33) {
		si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL8,
		PMUCCTL08_43012_XTAL_SEL_BIAS_RES_NORMAL_MASK,
		(0x4 << PMUCCTL08_43012_XTAL_SEL_BIAS_RES_NORMAL_SHIFT));

	} else {
		si_pmu_chipcontrol(pi->sh->sih, PMU_CHIPCTL8,
		PMUCCTL08_43012_XTAL_SEL_BIAS_RES_NORMAL_MASK,
		(0x2 << PMUCCTL08_43012_XTAL_SEL_BIAS_RES_NORMAL_SHIFT));
	}
}


static void
chanspec_setup_radio_20694(phy_info_t *pi)
{
	uint8 core = 0;
	const chan_info_radio20694_rffe_t *chan_info;
	uint8 ch_num;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	ch_num =  CHSPEC_CHANNEL(pi->radio_chanspec);

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	pi_ac->fc = wlc_phy_chan2freq_20694(pi, ch_num,
			&chan_info);

	if (pi_ac->fc >= 0) {
		FOREACH_CORE(pi, core) {
			wlc_phy_chanspec_radio20694_setup(pi, ch_num,
				CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), core);
		}
	}
}

static void
chanspec_setup_radio_20696(phy_info_t *pi)
{
	uint8 ch_num;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	ch_num =  CHSPEC_CHANNEL(pi->radio_chanspec);

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	wlc_phy_chanspec_radio20696_setup(pi, ch_num,
		CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac));
}

void
chanspec_setup_radio(phy_info_t *pi)
{
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID))
		chanspec_setup_radio_20693(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID))
		chanspec_setup_radio_20691(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID))
		chanspec_setup_radio_2069(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID))
		chanspec_setup_radio_20694(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID))
		chanspec_setup_radio_20695(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20696_ID))
		chanspec_setup_radio_20696(pi);
	else {
		PHY_ERROR(("wl%d %s: Invalid RADIOID! %d\n",
			PI_INSTANCE(pi), __FUNCTION__, pi->pubpi->radioid));
		ASSERT(0);
	}
}

void
chanspec_tune_radio(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	if (pi_ac->fc >= 0) {
		if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
			wlc_phy_28nm_radio_vcocal_isdone(pi, FALSE);
		} else if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
			wlc_phy_radio20696_vcocal_isdone(pi, FALSE, TRUE);
		} else if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
			wlc_phy_radio20694_vcocal_isdone(pi, FALSE, TRUE);
		} else if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
			wlc_phy_radio20696_vcocal_isdone(pi, FALSE, TRUE);
		} else {
			wlc_phy_radio2069x_vcocal_isdone(pi, FALSE, FALSE);
		}
	}

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID))
		chanspec_tune_radio_20693(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID))
		chanspec_tune_radio_20691(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID))
		chanspec_tune_radio_2069(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID))
		chanspec_tune_radio_20694(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID))
		chanspec_tune_radio_20695(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20696_ID))
		chanspec_tune_radio_20696(pi);
	else {
		PHY_ERROR(("wl%d %s: Invalid RADIOID! %d\n",
			PI_INSTANCE(pi), __FUNCTION__, pi->pubpi->radioid));
		ASSERT(0);
	}
}

static int
BCMRAMFN(wlc_phy_ac_get_20694_chan_info_tbl)(phy_info_t *pi,
	const chan_info_radio20694_rffe_t **chan_info,
	uint32 *p_tbl_len)
{
	int ret_status = 0;
	uint sicoreunit;

	sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	switch (RADIOREV(pi->pubpi->radiorev)) {
	case 36:

	default:
		if (sicoreunit == DUALMAC_AUX) {
		  *chan_info = chan_tune_20694_rev5_fcbu_e_AUX;
		  *p_tbl_len = ARRAYSIZE(chan_tune_20694_rev5_fcbu_e_AUX);
		} else {
		  *chan_info = chan_tune_20694_rev5_fcbu_e_MAIN;
		  *p_tbl_len = ARRAYSIZE(chan_tune_20694_rev5_fcbu_e_MAIN);
		}
		ret_status = 0;
		break;
		// PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
		//	pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
		// ASSERT(FALSE);
		// ret_status = -1;

	}

	return ret_status;
}

static radio_20xx_prefregs_t *
BCMRAMFN(wlc_phy_ac_get_20694_prfd_vals_tbl)(phy_info_t *pi)
{

uint sicoreunit;
sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
		case 36:
			return prefregs_20694_rev36_wlbga;

		default:
			if (sicoreunit == DUALMAC_MAIN) {
				return prefregs_20694_rev5_main;
			} else {
				return prefregs_20694_rev5_aux;
			}
			// PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			// pi->sh->unit, __FUNCTION__,
			//	RADIOREV(pi->pubpi->radiorev)));
			// ASSERT(FALSE);
			// return NULL;
	}
}

void
wlc_phy_radio20695_txdac_bw_setup(phy_info_t *pi, uint8 filter_type, uint8 dacbw)
{
	uint8  cfb_max  = 127;
	uint8  cin_max  = 255;
	uint8  clpf_max = 127;
	uint16 code_cfb = 17, code_cin = 17, code_clpf = 0;
	uint32 dacbuf_cmult = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	ASSERT(dacbw != 0);

	MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG0, 0, iqdac_buf_bw, 0x4);
	MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG0, 0, iqdac_buf_cc, 0x0);
	MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG1, 0, iqdac_buf_op_cur, 0x0);
	MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG1, 0, iqdac_lowpwr_en,  0x3);

	if (filter_type == 1) { /* butterworth */
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG4, 0, IQDACbuf_biqd_byp, 0x0);
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG4, 0, IQDACbuf_lpfen, 0x0);

		code_cfb = ((dacbuf_cmult * 2132) / dacbw) >> 12;
		code_cin = ((dacbuf_cmult * 2132) / dacbw) >> 12;
		code_cfb = (code_cfb > cfb_max) ? cfb_max : code_cfb;
		code_cin = (code_cin > cin_max) ? cin_max : code_cin;

		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG1, 0, iqdac_buf_Cfb, code_cfb);
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG3, 0, IQDACbuf_Cin,  code_cin);

	} else if (filter_type == 2) { /* "chebyshev"  */
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG4, 0, IQDACbuf_biqd_byp, 0x0);
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG4, 0, IQDACbuf_lpfen, 0x1);

		code_cfb = ((dacbuf_cmult * 1508) / dacbw) >> 12;
		code_cin = ((dacbuf_cmult * 3016) / dacbw) >> 12;
		code_clpf = ((dacbuf_cmult * 1508) / dacbw) >> 12;

		code_cfb  = (code_cfb > cfb_max) ? cfb_max : code_cfb;
		code_cin  = (code_cin > cin_max) ? cin_max : code_cin;
		code_clpf = (code_clpf > clpf_max) ? clpf_max : code_clpf;

		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG1, 0, iqdac_buf_Cfb, code_cfb);
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG3, 0, IQDACbuf_Cin,  code_cin);
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG3, 0, IQDACbuf_Clpf, code_clpf);

	} else if (filter_type == 3) {   /* biquad bypass */
		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG4, 0, IQDACbuf_biqd_byp, 0x1);

		code_cfb  = ((dacbuf_cmult * 3016) / dacbw) >> 12;
		code_cfb  = (code_cfb > cfb_max) ? cfb_max : code_cfb;

		MOD_RADIO_REG_28NM(pi, RF, TXDAC_REG1, 0, iqdac_buf_Cfb, code_cfb);

	}

}

uint16
wlc_phy_get_dac_rate_from_mode(phy_info_t *pi, uint8 dac_rate_mode)
{
	uint16 dac_rate = 200;

	if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
		switch (dac_rate_mode) {
			case 2:
				dac_rate = 600;
				break;
			case 3:
				dac_rate = 400;
				break;
			default: /* dac rate mode 1 */
				dac_rate = 200;
				break;
		}
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		switch (dac_rate_mode) {
			case 2:
				dac_rate = 600;
				break;
			default: /* dac rate mode 1 and 3 */
				dac_rate = 400;
				break;
		}
	} else {
		dac_rate = 600;
	}

	return (dac_rate);
}

void wlc_phy_dac_rate_mode_acphy(phy_info_t *pi, uint8 dac_rate_mode)
{
	uint16 dac_rate;
	uint8 bw_idx = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	bw_idx = (CHSPEC_IS80(pi->radio_chanspec) || PHY_AS_80P80(pi, pi->radio_chanspec))
				? 2 : (CHSPEC_IS160(pi->radio_chanspec)) ? 3
				: (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
	dac_rate = wlc_phy_get_dac_rate_from_mode(pi, dac_rate_mode);

	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		/* For 43012 dac_rate_mode is treated as ulp_tx_mode */
		uint16 ulp_tx_mode;
		ulp_tx_mode = pi_ac->ulp_tx_mode;

		/* In case of 43012. Rely on ulp_tx_mode instead of dac mode to change settings */
		switch (ulp_tx_mode) {
			case 2:
				/* 240 MHz mode */
				si_core_cflags(pi->sh->sih, 0x300, 0x200);
				break;

			case 3:
				/* 180 MHz mode */
				si_core_cflags(pi->sh->sih, 0x300, 0x200);
				break;

			case 4:
				/* 120 MHz mode */
				si_core_cflags(pi->sh->sih, 0x300, 0x000);
				break;

			case 5:
				/* 90 MHz mode */
				si_core_cflags(pi->sh->sih, 0x300, 0x000);
				break;

			case 1:
			default:
				/* Default mode is 360 MHz */
				si_core_cflags(pi->sh->sih, 0x300, 0x300);
				break;
		}
		MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr_value, 0);

	} else {
		if (dac_rate_mode == 2) {
			si_core_cflags(pi->sh->sih, 0x300, 0x200);
		} else if (dac_rate_mode == 3) {
			si_core_cflags(pi->sh->sih, 0x300, 0x300);
		} else {
			si_core_cflags(pi->sh->sih, 0x300, 0x100);
		}

		switch (dac_rate) {
			case 200:
				MOD_RADIO_REG_20691(pi, CLK_DIV_CFG1, 0, sel_dac_div,
					(pi_ac->radioi->data.vcodivmode & 0x1) ? 3 : 4);
				break;
			case 400:
				MOD_RADIO_REG_20691(pi, CLK_DIV_CFG1, 0, sel_dac_div,
					(pi_ac->radioi->data.vcodivmode & 0x2) ? 1 : 2);
				break;
			case 600:
				MOD_RADIO_REG_20691(pi, CLK_DIV_CFG1, 0, sel_dac_div, 0);
				break;
			default:
				PHY_ERROR(("Unsupported dac_rate %d\n", dac_rate));
				ASSERT(0);
				break;
		}

		if ((dac_rate_mode == 1) || (bw_idx == 2)) {
			MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr, 0);
			MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr_value, 0);
		} else {
			MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr, 1);
			if (bw_idx == 1)
				MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr_value, 2);
			else
				MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr_value, 1);
		}
	}

	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		wlc_phy_radio20695_afe_div_ratio(pi, pi_ac->ulp_tx_mode, pi_ac->ulp_adc_mode);
	}

	if (TINY_RADIO(pi)) {
		wlc_phy_farrow_setup_tiny(pi, pi->radio_chanspec);
	} else {
		if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
			wlc_phy_farrow_setup_28nm_ulp(pi, pi_ac->ulp_tx_mode, pi_ac->ulp_adc_mode);
		} else {
			wlc_phy_farrow_setup_acphy(pi, pi->radio_chanspec);
		}
	}
}

#ifdef PHY_DUMP_BINARY
/* The function is forced to RAM since it accesses non-const tables */
static int BCMRAMFN(phy_ac_radio_getlistandsize_20693)(phy_info_t *pi,
                    phyradregs_list_t **radreglist, uint16 *radreglist_sz)
{
	if (RADIO20693_MAJORREV(pi->pubpi->radiorev) == 2) {
		*radreglist = (phyradregs_list_t *) &rad20693_majorrev2_registers[0];
		*radreglist_sz = sizeof(rad20693_majorrev2_registers);
	} else {
		PHY_INFORM(("%s: wl%d: unsupported BCM20693 radio rev %d\n",
			__FUNCTION__,  pi->sh->unit,  pi->pubpi->radiorev));
		return BCME_UNSUPPORTED;
	}

	return BCME_OK;
}

static int BCMRAMFN(phy_ac_radio_getlistandsize_20696)(phy_info_t *pi,
                    phyradregs_list_t **radreglist, uint16 *radreglist_sz)
{
	/* For now, only one radio version */
	*radreglist = (phyradregs_list_t *) &rad20696_majorrev0_registers[0];
	*radreglist_sz = sizeof(rad20696_majorrev0_registers);

	return BCME_OK;
}

static int phy_ac_radio_getlistandsize(phy_type_radio_ctx_t *ctx,
                    phyradregs_list_t **radreglist, uint16 *radreglist_sz)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int ret = BCME_UNSUPPORTED;
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		ret = phy_ac_radio_getlistandsize_20693(pi, radreglist, radreglist_sz);
	} else {
		PHY_INFORM(("%s: wl%d: unsupported radio ID %d\n",
			__FUNCTION__,  pi->sh->unit,  pi->pubpi->radioid));
	}
	return ret;
}
#endif /* PHY_DUMP_BINARY */

static void
wlc_idac_preload_20691(phy_info_t *pi, int16 i, int16 q)
{
	uint16 cnt;
	ASSERT(ACREV_GE(pi->pubpi->phy_rev, 11));

	if (i < 0)
		i += 512;
	if (q < 0)
		q += 512;

	i &= 0x1ff;
	q &= 0x1ff;

	/* WAR for negative values -ensure fsm is running */
	MOD_PHYREG(pi, RfseqTrigger, en_pkt_proc_dcc_ctrl, 0x0);
	cnt = READ_PHYREG(pi, rx_tia_dc_loop_gain_5);
	WRITE_PHYREG(pi, rx_tia_dc_loop_gain_5, 15); /* restart gain is 0, ie NOP */
	MOD_PHYREG(pi, rx_tia_dc_loop_0, en_lock, 0); /* always run */
	OSL_DELAY(1);

	/* overide offset comp PU; phyctrl logic not reliable for preload WAR,
	   dac value fails to update occassionally
	*/
	MOD_RADIO_REG_20691(pi, TIA_CFG15, 0, tia_offset_comp_pwrup, 1);
	MOD_RADIO_REG_20691(pi, RX_BB_2G_OVR_NORTH, 0, ovr_tia_offset_comp_pwrup, 1);

	/* restart to enable preload WAR for negative value (CRDOT11ACPHY-871) */
	wlc_dcc_fsm_restart(pi);

	/* write the values across */
	WRITE_PHYREG(pi, BfmConfig3, 0x0200 | i);
	WRITE_PHYREG(pi, BfmConfig3, 0x0600 | q);
	WRITE_PHYREG(pi, BfmConfig3, 0x0000);

	/* remove comp pwrup force */
	MOD_RADIO_REG_20691(pi, RX_BB_2G_OVR_NORTH, 0, ovr_tia_offset_comp_pwrup, 0);

	/* stop loop running and restore config */
	MOD_PHYREG(pi, rx_tia_dc_loop_0, en_lock, 1);

	OSL_DELAY(1);

	WRITE_PHYREG(pi, rx_tia_dc_loop_gain_5, cnt);

	MOD_PHYREG(pi, RfseqTrigger, en_pkt_proc_dcc_ctrl, 0x1);
}

void
phy_ac_radio_cal_init(phy_info_t *pi)
{
	if (TINY_RADIO(pi)) {
		/* switch back to original rx2tx seq and dly for tiny cal */
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);
		}
		phy_ac_rfseq_mode_set(pi, 1);

		if (pi->u.pi_acphy->dac_mode != 1) {
			pi->u.pi_acphy->dac_mode = 1;
			wlc_phy_dac_rate_mode_acphy(pi, pi->u.pi_acphy->dac_mode);
		}

		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);
		}
	} else {
		if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
			/* switch to cal rx2tx seq and dly */
			phy_ac_rfseq_mode_set(pi, 1);
		}
	}
}

void
phy_ac_radio_cal_reset(phy_info_t *pi, int16 idac_i, int16 idac_q)
{
	if (TINY_RADIO(pi)) {
	/* switch to normal rx2tx seq and dly after tiny cal */
		phy_ac_rfseq_mode_set(pi, 0);
		if (pi->u.pi_acphy->dac_mode != (CHSPEC_IS2G(pi->radio_chanspec)
			? pi->dacratemode2g : pi->dacratemode5g)) {
			pi->u.pi_acphy->dac_mode = CHSPEC_IS2G(pi->radio_chanspec)
				? pi->dacratemode2g : pi->dacratemode5g;
			wlc_phy_dac_rate_mode_acphy(pi, pi->u.pi_acphy->dac_mode);
		}

		if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
			if (PHY_ILNA(pi)) {
				wlc_idac_preload_20691(pi, idac_i, idac_q);
				wlc_phy_enable_lna_dcc_comp_20691(pi, PHY_ILNA(pi));
			} else {
				wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);
				wlc_dcc_fsm_reset(pi);
				wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
			}
		}
	} else {
		if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
			/* switch to normal rx2tx seq and dly after cal */
			phy_ac_rfseq_mode_set(pi, 0);
		}
	}

	if ((ACMAJORREV_32(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) ||
	     (ACMAJORREV_33(pi->pubpi->phy_rev))) {
		//if (pi->sh->phytxchain == 15 || 1)
		if (!PHY_AS_80P80(pi, pi->radio_chanspec)) {
			phy_ac_chanmgr_enable_core2core_sync_setup(pi);
		}
	}
}

static void wlc_phy_radio20694_low_current_mode(phy_info_t *pi, uint8 mode)
{
	uint8 core;
	FOREACH_CORE(pi, core) {
		WRITE_RADIO_REG_20694(pi, RF, READOVERRIDES, core, 0);

		if (BF_ELNA_2G(pi->u.pi_acphy)) {
			if (mode == 0) {
				MOD_RADIO_REG_20694(pi, RF, LNA2G_REG2, core,
						lna2g_lna1_bias_idac, 3);
				MOD_RADIO_REG_20694(pi, RF, RX2G_REG2, core,
						rx2g_gm_idac_main, 3);
			} else if (mode == 1) {
				MOD_RADIO_REG_20694(pi, RF, LNA2G_REG2, core,
						lna2g_lna1_bias_idac, 2);
				MOD_RADIO_REG_20694(pi, RF, RX2G_REG2, core,
						rx2g_gm_idac_main, 3);
			} else {
				MOD_RADIO_REG_20694(pi, RF, LNA2G_REG2, core,
						lna2g_lna1_bias_idac, 2);
				MOD_RADIO_REG_20694(pi, RF, RX2G_REG2, core,
						rx2g_gm_idac_main, 2);
			}
		}

		if (BF_ELNA_5G(pi->u.pi_acphy)) {
			MOD_RADIO_REG_20694(pi, RF, RX5G_REG2, core, rx5g_lna_cc, 5);
			MOD_RADIO_REG_20694(pi, RF, RX5G_REG4, core, rx5g_gm_cc, 7);
		}
		WRITE_RADIO_REG_20694(pi, RF, READOVERRIDES, core, 1);
	}
}

static int
wlc_tiny_sigdel_fast_mult(int raw_p8, int mult_p12, int max_val, int rshift)
{
	int prod;
	/* >> 20 follows from .12 fixed-point for mult, various.8 for raw */
	prod = (mult_p12 * raw_p8) >> rshift;
	return (prod > max_val) ? max_val : prod;
}

static int
wlc_tiny_sigdel_wrap(int prod, int max_val)
{
	/* to make sure you hit the maximum number of bits in word allocated  */
	return (prod > max_val) ? max_val : prod;
}

#define WLC_TINY_GI_MULT_P12		4096U
#define WLC_TINY_GI_MULT_TWEAK_P12	4096U
#define WLC_TINY_GI_MULT		WLC_TINY_GI_MULT_P12
static void
wlc_tiny_sigdel_slow_tune(phy_info_t *pi, int g_mult_raw_p12,
	tiny_adc_tuning_array_t *gvalues, uint8 bw)
{
	int g_mult_p12;
	int ri = 0;
	int r21;
	int r32;
	int r43;
	int rff1;
	int rff2;
	int rff3;
	int rff4;
	int r12v;
	int r34v;
	int r11v;
	int g21;
	int g32;
	int g43;
	int r12;
	int r34;
	int g11;
	int temp;
	int gff3_val = 0;
	uint8 shift_val = 0;
	BCM_REFERENCE(pi);
	/* RC cals the slow ADC IN 40MHz channels or 20MHz bandwidth, based on g_mult */
	/* input signal scaling, changes ADC gain, 4096 <=> 1.0 for g_mult and gi_mult */
	/* Function is 32 bit (signed) integer arithmetic and a/b division rounding  */
	/* is performed in integers from: (a-1)/b+1 */

	/* ERR! 20691_rc_cal "adc" returns the RC value, so correction is 1/rccal! */
	/* so invert it */
	/* This is a nice way of inverting the number... jnh */
	/* inverse of gmult precomputed to minimise division operations for speed */
	/* 4.12 fixed point so scale reciprocal by 2^24 */
	g_mult_p12 = g_mult_raw_p12 > 0 ? g_mult_raw_p12 : 1;
	/* tweak to g_mult */
	g_mult_p12 = (WLC_TINY_GI_MULT_TWEAK_P12 * g_mult_p12) >> 12;

	/* to avoid divide by zeros and negative values */
	if (g_mult_p12 <= 0)
		g_mult_p12 = 1;

	if (bw == 20) {
	    shift_val = 1;
		ri = 10176;
		gff3_val = 32000;
	} else if (bw == 40) {
		shift_val = 0;
		ri = 10670;
		gff3_val = 12000;
	}

	/* RC cal in slow ADC is mostly of the form Runit/(Rval/(g_mult/2**12)-Roff). */
	/* For integer manipulation do Runit/({Rval*2**12}/gmult-Roff).  */
	/* where Rval*2**12 are res design values in matlab script {x kint234 , kr12, kr34} */
	/* but right shifted a number of times */
	/* All but r11 and rff4 resistances are x2 for 20MHz. */

	/* Rvals from matlab already scaled by kint234, kr12 */
	/* or kr34 (due to amplifier finite GBW) */
	/* x2 for half the BW and half the sampling frequency. */
	ri = ri << shift_val;
	r21 = 8323 << shift_val;
	r32 = 6390 << shift_val;
	r43 = 6827 << shift_val;
	rff1 = 19768 << shift_val;
	rff2 = 16916 << shift_val;
	rff3 = 29113  << shift_val;
	/* rff4 does not double with 20MHz channels, 10MHz BW. */
	rff4 = 100000;
	r12v = 83205 << shift_val;
	r34v = 243530 << shift_val;
	/* rff4 does not double with 20MHz channels, 10MHz BW. */
	r11v = 8000;

	/* saturate correctly when you get negative numbers and round divisions */
	/* subject to gmult twice so scale gmult back to 12b so it only divides with 12b+ r21 */
	g21 = (g_mult_p12 * g_mult_p12) >> 12;
	if (g21 <= 0)
		g21 = 1;
	g21 = ((r21 << 12) - 1) / g21 + 1;
	g21 = (512000 - 1) / g21 + 1;
	g21 = wlc_tiny_sigdel_wrap(g21, 127);
	g32 = (256000 - 1) / (((r32 << 12) - 1) / g_mult_p12 + 1) + 1;
	g32 = wlc_tiny_sigdel_wrap(g32, 127);
	g43 = (256000 - 1) / (((r43 << 12) - 1) / g_mult_p12 + 1) + 1;
	g43  = wlc_tiny_sigdel_wrap(g43, 127);

	/* gff1234 subject to gmult and gimult; step operations so range */
	/* is not exceeded and scale is correct */
	/* gff1234 will overflow if $g_mult_p12*$gi_mult < 1023*2, eq. to 0.25, */
	/* assuming rff1234 {<<2} < 131072 */

	/* gi */
	temp = (((ri << 12) - 1) / WLC_TINY_GI_MULT) - 4000 + 1;
	ASSERT(temp > 0);	/* should be a positive value */
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gi = (uint16) temp;

	/* gff1 */
	temp = ((((((rff1 << 12) - 1) / g_mult_p12 + 1) << 12) - 1) / WLC_TINY_GI_MULT) - 8000 + 1;
	if (temp <= 0)
		temp = 1;
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gff1 = (uint16) temp;

	/* gff2 */
	temp = ((((((rff2 << 12) - 1) / g_mult_p12 + 1) << 12) - 1) / WLC_TINY_GI_MULT) - 4000 + 1;
	if (temp <= 0)
		temp = 1;
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gff2 = (uint16) temp;

	/* gff3 */
	temp = ((((((rff3 << 12) - 1) / g_mult_p12 + 1) << 12) - 1) / WLC_TINY_GI_MULT) - gff3_val
	    + 1;

	if (temp <= 0)
		temp = 1;
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gff3 = (uint16) temp;

	/* gff4 */
	temp = (((rff4 << 12) - 1) / WLC_TINY_GI_MULT) - 72000 + 1;	/* subject to gimult only */
	ASSERT(temp > 0);	/* should be a positive value */
	temp = (256000 - 1) / temp + 1;
	temp  = wlc_tiny_sigdel_wrap(temp, 255);
	gvalues->gff4 = (uint16) temp;

	/* stays constant to RC shifts, g21 shifts twice for it. */
	r12 = (r12v - 1) / 4000 + 1;
	r12 = wlc_tiny_sigdel_wrap(r12, 127);

	if (bw == 20)
		r34 = ((((r34v << 12) - 1) / g_mult_p12 +1) - 128000 - 1) / 4000 + 1;
	else
		r34 = ((r34v << 12) - 1) / g_mult_p12 / 4000 + 1;

	if (r34 <= 0)
		r34 = 1;
	r34 = wlc_tiny_sigdel_wrap(r34, 127);
	g11 = (((r11v << 12) - 1) / g_mult_p12 +1) - 2000;
	if (g11 <= 0)
		g11 = 1;
	g11 = (128000 - 1) / g11 + 1;
	g11 = wlc_tiny_sigdel_wrap(g11, 127);

	gvalues->g21 = (uint16) g21;
	gvalues->g32 = (uint16) g32;
	gvalues->g43 = (uint16) g43;
	gvalues->r12 = (uint16) r12;
	gvalues->r34 = (uint16) r34;
	gvalues->g11 = (uint16) g11;
	PHY_TRACE(("gi   = %i\n", gvalues->gi));
	PHY_TRACE(("g21  = %i\n", gvalues->g21));
	PHY_TRACE(("g32  = %i\n", gvalues->g32));
	PHY_TRACE(("g43  = %i\n", gvalues->g43));
	PHY_TRACE(("r12  = %i\n", gvalues->r12));
	PHY_TRACE(("r34  = %i\n", gvalues->r34));
	PHY_TRACE(("gff1 = %i\n", gvalues->gff1));
	PHY_TRACE(("gff2 = %i\n", gvalues->gff2));
	PHY_TRACE(("gff3 = %i\n", gvalues->gff3));
	PHY_TRACE(("gff4 = %i\n", gvalues->gff4));
	PHY_TRACE(("g11  = %i\n", gvalues->g11));
}

static void
wlc_tiny_sigdel_fast_tune(phy_info_t *pi, int g_mult_raw_p12, tiny_adc_tuning_array_t *gvalues)
{
	int g_mult_tweak_p12;
	int g_mult_p12;
	int g_inv_p12;
	int gi_inv_p12;
	int gi_p8;
	int ri3_p8;
	int g21_p8;
	int g32_p8;
	int g43_p8;
	int g54_p8;
	int g65_p8;
	int r12_p8;
	int r34_p8;
	int gi, ri3, g21, g32, g43, g54, g65, r12, r34;

	BCM_REFERENCE(pi);

	/* tweak to g_mult to trade off stability over PVT versus performance */
	g_mult_tweak_p12 = 4096;
	g_mult_p12 = (g_mult_tweak_p12 * g_mult_raw_p12) >> 12;

	/* inverse of gmult precomputed to minimise division operations for speed */
	g_inv_p12 = 16777216 / g_mult_p12;
	gi_inv_p12 = 16777216 / WLC_TINY_GI_MULT;

	/* untuned values in p8 fixed-point format, ie. multiplied by 2^8 */
	gi_p8 = 16384;
	ri3_p8 = 1477;
	g21_p8 = 17997;
	g32_p8 = 18341;
	g43_p8 = 15551;
	g54_p8 = 19915;
	g65_p8 = 12369;
	r12_p8 = 1156;
	r34_p8 = 4331;

	/* RC cal */
	gi = wlc_tiny_sigdel_fast_mult(gi_p8, WLC_TINY_GI_MULT, 127, 20);
	ri3 = wlc_tiny_sigdel_fast_mult(ri3_p8, gi_inv_p12, 63, 20);
	g21 = wlc_tiny_sigdel_fast_mult(g21_p8, g_mult_p12, 127, 20);
	g32 = wlc_tiny_sigdel_fast_mult(g32_p8, g_mult_p12, 127, 20);
	g43 = wlc_tiny_sigdel_fast_mult(g43_p8, g_mult_p12, 127, 20);
	g54 = wlc_tiny_sigdel_fast_mult(g54_p8, g_mult_p12, 127, 20);
	g65 = wlc_tiny_sigdel_fast_mult(g65_p8, g_mult_p12, 63, 20);
	r12 = wlc_tiny_sigdel_fast_mult(r12_p8, g_inv_p12, 63, 20);
	r34 = wlc_tiny_sigdel_fast_mult(r34_p8, g_inv_p12, 63, 20);

	gvalues->gi = (uint16) gi;
	gvalues->ri3 = (uint16) ri3;
	gvalues->g21 = (uint16) g21;
	gvalues->g32 = (uint16) g32;
	gvalues->g43 = (uint16) g43;
	gvalues->g54 = (uint16) g54;
	gvalues->g65 = (uint16) g65;
	gvalues->r12 = (uint16) r12;
	gvalues->r34 = (uint16) r34;
}

static void
wlc_tiny_adc_setup_slow(phy_info_t *pi, tiny_adc_tuning_array_t *gvalues, uint8 bw, uint8 core)
{
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int row;
	uint8 adcclkdiv = 0x1;
	uint8 sipodiv = 0x1;

	ASSERT(TINY_RADIO(pi));

	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	}
	row = wlc_phy_radio20693_altclkpln_get_chan_row(pi);

	if (row >= 0) {
		adcclkdiv = altclkpln[row].adcclkdiv;
		sipodiv = altclkpln[row].sipodiv;
	}
	/* SETS UP THE slow ADC IN 20MHz channels or 10MHz bandwidth */
	/* Function should be 32 bit (signed) arithmetic */
	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) && (bw == 20)) {
		if (RADIOMAJORREV(pi) != 3)
			MOD_RADIO_REG_20693(pi, SPARE_CFG2, core, adc_clk_slow_div, adcclkdiv);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
		/* EXPLICITELY ENABLE/DISABLE ADCs and INTERNAL CLKs? */
		/* Only changes between fast/slow ADC, not 20/40MHz */
		MOD_RADIO_REG_20691(pi, ADC_OVR1, core, ovr_adc_fast_pu, 0x1);
		MOD_RADIO_REG_20691(pi, ADC_CFG1, core, adc_fast_pu, 0x0);
		MOD_RADIO_REG_20691(pi, ADC_OVR1, core, ovr_adc_slow_pu, 0x0);
		MOD_RADIO_REG_20691(pi, ADC_OVR1, core, ovr_adc_clk_fast_pu, 0x1);
		MOD_RADIO_REG_20691(pi, ADC_CFG15, core, adc_clk_fast_pu, 0x0);
		MOD_RADIO_REG_20691(pi, ADC_OVR1, core, ovr_adc_clk_slow_pu, 0x0);
	}
	/* Setup internal dividers and sipo for 1G2Hz mode */
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_drive_strength, 0x4);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, sipodiv);
	/* set adc_clk_slow_div3 to 0x0 in 20MHz mode, 0x1 in 40MHz mode */
	if (bw == 20)
		MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_div3, 0x0);
	else if (bw == 40)
		MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_div3, 0x1);
	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) && (bw == 40) &&
		(RADIOMAJORREV(pi) != 3))
		MOD_RADIO_REG_20693(pi, SPARE_CFG2, core, adc_clk_slow_div, adcclkdiv);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, sipodiv);
	MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_sipo_sel_fast, 0x0);

	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_pu, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_od_pu, 0x1);

	/* Setup biases */
	/* Slow adc halves opamp current from 20MHz to 40MHz channels */
	/* Opamp1 is 26u/0, 4u/2 other 3 are 26u/0, 4u/4,	*/
	/* so opamp1 (40M, 20M) = (0x20, 0x10), opamp234 = (0x10, 0x8) */
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp1, (0x10 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp2, (0x8 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp3, (0x8 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp4, (0x8 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_ff_mult_opamp, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref_control, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref4_control, 0x40);

	/* Setup transconductances. These are tuned with gmult(RC) and/or gimult(input gain) */
	/* rnm */
	MOD_RADIO_REG_TINY(pi, ADC_CFG2, core, adc_gi, gvalues->gi);
	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g21, gvalues->g21);
	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g32, gvalues->g32);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g43, gvalues->g43);
	/* rff */
	MOD_RADIO_REG_TINY(pi, ADC_CFG16, core, adc_gff1, gvalues->gff1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG16, core, adc_gff2, gvalues->gff2);
	MOD_RADIO_REG_TINY(pi, ADC_CFG17, core, adc_gff3, gvalues->gff3);
	MOD_RADIO_REG_TINY(pi, ADC_CFG17, core, adc_gff4, gvalues->gff4);
	/* resonator and r11 */
	MOD_RADIO_REG_TINY(pi, ADC_CFG5, core, adc_r12, gvalues->r12);
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_r34, gvalues->r34);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g54, gvalues->g11);

	/* Setup feedback DAC and tweak delay compensation */
	if (bw == 20) {
		/* In slow 40MHz ADC rt is 0x0, in 20MHz ADC rt is 0x2 */
	    MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_rt, 0x2);
		/* In slow 40MHz ADC slow_dacs is 0x2 in 20MHz ADC rt is 0x1 */
	    MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_slow_dacs, 0x1);
	} else if (bw == 40) {
		/* In slow 40MHz ADC rt is 0x0, in 20MHz ADC rt is 0x2 */
		MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_rt, 0x0);
		/* In slow 40MHz ADC slow_dacs is 0x2 in 20MHz ADC rt is 0x1 */
		MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_slow_dacs, 0x2);
	}
	if (RADIOMAJORREV(pi) != 3) {
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x0);
	}
}

static void
wlc_tiny_adc_setup_fast(phy_info_t *pi, tiny_adc_tuning_array_t *gvalues, uint8 core)
{

	/* Bypass override of slow/fast adc to power up/down for 4365 radio */
	if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && (RADIOMAJORREV(pi) == 3))) {
		/* EXPLICITLY ENABLE/DISABLE ADCs and INTERNAL CLKs?  */
		/* This part is comment out in TCL */
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_fast_pu, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_slow_pu, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_slow_pu, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_clk_fast_pu, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_clk_slow_pu, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_pu, 0x0);
	}
	/* Setup internal dividers and sipo for 3G2Hz mode. */
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_drive_strength, 0x4);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_div3, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_sipo_sel_fast, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_drive_strength, 0x4);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp1, 0x60);
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp2, 0x60);
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp3, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp4, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_ff_mult_opamp, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref_control, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref4_control, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_sipo_sel_fast, 0x1);

	/* Turn on overload detector */
	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_bias_comp, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_threshold, 0x3);
	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_reset_duration, 0x3);
	MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_od_pu, 0x1);

	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_pu, 0x1);
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && (RADIOMAJORREV(pi) == 3))
		MOD_RADIO_REG_TINY(pi, ADC_CFG13, core, adc_od_disable, 0x0);

	MOD_RADIO_REG_TINY(pi, ADC_CFG2, core, adc_gi, gvalues->gi);

	/* typo in spreadsheet for TC only - should be ri3 but got called ri1 */
	MOD_RADIO_REG_TINY(pi, ADC_CFG2, core, adc_ri3, gvalues->ri3);

	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g21, gvalues->g21);
	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g32, gvalues->g32);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g43, gvalues->g43);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g54, gvalues->g54);
	MOD_RADIO_REG_TINY(pi, ADC_CFG5, core, adc_g65, gvalues->g65);
	MOD_RADIO_REG_TINY(pi, ADC_CFG5, core, adc_r12, gvalues->r12);
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_r34, gvalues->r34);
	/* Is it need in 4365? */
	if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && (RADIOMAJORREV(pi) == 3))) {
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x0);
	}
}

static void
wlc_phy_radio20693_adc_setup(phy_ac_radio_info_t *ri, uint8 core,
	radio_20693_adc_modes_t adc_mode)
{
	tiny_adc_tuning_array_t gvalues;
	uint8 bw;
	phy_info_t *pi = ri->pi;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	bw = CHSPEC_IS20(pi->radio_chanspec) ? 20 : CHSPEC_IS40(pi->radio_chanspec) ? 40 : 80;

	if ((adc_mode == RADIO_20693_FAST_ADC) || (CHSPEC_IS80(pi->radio_chanspec)) ||
		(CHSPEC_IS160(pi->radio_chanspec)) ||
		(CHSPEC_IS8080(pi->radio_chanspec))) {
		/* 20 and 40MHz fast mode and 80MHz channel */
		wlc_tiny_sigdel_fast_tune(pi, ri->rccal_adc_gmult, &gvalues);
		wlc_tiny_adc_setup_fast(pi, &gvalues, core);
	} else {
		/* slow mode for 20 and 40MHz channel */
		wlc_tiny_sigdel_slow_tune(pi, ri->rccal_adc_gmult, &gvalues, bw);
		wlc_tiny_adc_setup_slow(pi, &gvalues, bw, core);
	}
}


static void
wlc_phy_radio20696_adc_setup(phy_info_t *pi, uint8 core)
{
	uint8 mode;
	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20696_ID);

	MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 0x1);
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_power_mode_enb, 0x0);

	if (CHSPEC_IS160(pi->radio_chanspec)) {
		mode = 0x0;
	} else if (CHSPEC_IS80(pi->radio_chanspec)) {
		mode = 0x1;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		mode = 0x2;
	} else {
		mode = 0x3;
	}
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_power_mode, mode);
}


/*
 *  The TIA has 13 distinct gain steps.
 *  Each of the tia_* scalers are packed with the
 *  tia settings for each gain step.
 *  Mapping for each gain step is:
 *  pwrup_amp2, amp2_bypass, R1, R2, R3, R4, C1, C2, enable_st1
 */
static void
wlc_tiny_tia_config(phy_info_t *pi, uint8 core)
{
/*
 *  The TIA has 13 distinct gain steps.
 *  Each of the tia_* scalers are packed with the
 *  tia settings for each gain step.
 *  Mapping for each gain step is:
 *  pwrup_amp2, amp2_bypass, R1, R2, R3, R4, C1, C2, enable_st1
 */
	const uint8  *p8;
	const uint16 *p16;
	const uint16 *prem = NULL;
	uint16 lut, lut_51, lut_82;
	int i;

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_80) == 52);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_40) == 52);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_20) == 52);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_80) == 30);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_20) == 30);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_80_rem) == 4);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_20_rem) == 4);
	}

	ASSERT(TINY_RADIO(pi));

	if (CHSPEC_IS80(pi->radio_chanspec)) {
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
			prem = tiaRC_tiny_16b_80_rem;
		else
			STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_80) +
				ARRAYSIZE(tiaRC_tiny_16b_80) == 82);
		p8 = tiaRC_tiny_8b_80;
		p16 = tiaRC_tiny_16b_80;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
			prem = tiaRC_tiny_16b_40_rem;
		else
			STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_40) +
				ARRAYSIZE(tiaRC_tiny_16b_20) == 82);
		p8 = tiaRC_tiny_8b_40;
		p16 = tiaRC_tiny_16b_40;
	} else {
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
			prem = tiaRC_tiny_16b_20_rem;
		else
			STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_20) +
				ARRAYSIZE(tiaRC_tiny_16b_20) == 82);
		p8 = tiaRC_tiny_8b_20;
		p16 = tiaRC_tiny_16b_20;
	}

	lut = RADIO_REG(pi, TIA_LUT_0, core);
	lut_51 = RADIO_REG(pi, TIA_LUT_51, core);
	lut_82 = RADIO_REG(pi, TIA_LUT_82, core);

	/* the assumption is that all the TIA LUT registers are in sequence */
	ASSERT(lut_82 - lut == 81);

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		for (i = 0; i < 52; i++) /* LUT0-LUT51 */
			phy_utils_write_radioreg(pi, lut++, p8[i]);

		for (i = 0; i < 26; i++) /* LUT52-LUT78 (no LUT67) */
			phy_utils_write_radioreg(pi, lut++, p16[i]);

		for (i = 0; i < 4; i++) /* LUT79-LUT82 */
			phy_utils_write_radioreg(pi, lut++, prem[i]);
	} else {
		do {
			phy_utils_write_radioreg(pi, lut++, *p8++);
		} while (lut <= lut_51);

		do {
			phy_utils_write_radioreg(pi, lut++, *p16++);
		} while (lut <= lut_82);
	}
}

void
wlc_phy_set_regtbl_on_band_change_acphy_20691(phy_info_t *pi)
{
	uint8 core, bw;
	tiny_adc_tuning_array_t gvalues;
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20691_ID));
	bw = CHSPEC_BW_LE20(pi->radio_chanspec) ? 20 : CHSPEC_IS40(pi->radio_chanspec) ? 40 : 80;

	/* ### 20691_band_set(pi); */
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		/* # Restore PHY control for Gband blocks which may have been switched
		 * off in Aband
		 */
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, TIA_CFG8, 0, tia_offset_dac_biasadj, 4)
			MOD_RADIO_REG_20691_ENTRY(pi, LOGEN_OVR1, 0, ovr_logencore_2g_pu, 0)
			WRITE_RADIO_REG_ENTRY(pi, RADIO_REG_20691(pi, TX_TOP_2G_OVR_EAST, 0), 0)
			WRITE_RADIO_REG_ENTRY(pi,
				RADIO_REG_20691(pi, TX_TOP_2G_OVR_NORTH, 0), 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_2G_OVR_EAST, 0, ovr_gm2g_auxgm_pwrup,
				1)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_2G_OVR_EAST, 0, ovr_gm2g_pwrup, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_2G_OVR_EAST, 0,
				ovr_lna2g_dig_wrssi1_pu, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_2G_OVR_NORTH, 0, ovr_rxmix2g_pu, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_2G_OVR_NORTH, 0, ovr_lna2g_lna1_pu, 0)

			/* lna5g_pu_lna2 seems to get switched on during 5G band switch */
			MOD_RADIO_REG_20691_ENTRY(pi, LNA5G_CFG2, 0, lna5g_pu_lna2, 0)

			/* # Misc */
			MOD_RADIO_REG_20691_ENTRY(pi, TX_LPF_CFG2, 0, lpf_sel_5g_out_gm, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, TX_LPF_CFG3, 0, lpf_sel_2g_5g_cmref_gm, 0)
		ACPHY_REG_LIST_EXECUTE(pi);

		if (IS_4364_1x1(pi)) {
			/* RSDB 1x1 Tx + 3x3 Rx: Remove 2G TR override when 1x1 is in 2G */
			MOD_RADIO_REG_20691(pi, RX_TOP_2G_OVR_NORTH, 0, ovr_lna2g_tr_rx_en, 0);
			MOD_RADIO_REG_20691(pi, LNA2G_CFG1, 0, lna2g_tr_rx_en, 1);
			/* RSDB 1x1 Tx + 3x3 Rx: Force 5G TR override when 1x1 is in 2G */
			MOD_RADIO_REG_20691(pi, RX_TOP_5G_OVR, 0, ovr_lna5g_tr_rx_en, 1);
			MOD_RADIO_REG_20691(pi, LNA5G_CFG1, 0, lna5g_tr_rx_en, 0);
		}

		if (!PHY_IPA(pi)) {
			MOD_RADIO_REG_20691(pi, TXMIX2G_CFG6, 0, mx2g_idac_bbdc, 0x20);
		}

	} else {
		ACPHY_REG_LIST_START
			/* # clear 5g overrides */
			MOD_RADIO_REG_20691_ENTRY(pi, TIA_CFG8, 0, tia_offset_dac_biasadj, 12)
			MOD_RADIO_REG_20691_ENTRY(pi, LOGEN_OVR1, 0, ovr_logencore_5g_pu, 0)
			WRITE_RADIO_REG_ENTRY(pi, RADIO_REG_20691(pi, TX_TOP_5G_OVR1, 0), 0)
			WRITE_RADIO_REG_ENTRY(pi, RADIO_REG_20691(pi, TX_TOP_5G_OVR2, 0), 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_5G_OVR, 0, ovr_lna5g_lna1_pu, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_5G_OVR, 0, ovr_gm5g_pwrup, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_5G_OVR, 0, ovr_lna5g_dig_wrssi1_pu, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, RX_TOP_5G_OVR, 0, ovr_lna5g_pu_auxlna2, 1)
			MOD_RADIO_REG_20691_ENTRY(pi, LNA5G_CFG2, 0, lna5g_pu_auxlna2, 0)

			/* to power up/down logen appropriately */
			MOD_RADIO_REG_20691_ENTRY(pi, TX_LOGEN5G_CFG1, 0,
				logen5g_tx_enable_5g_low_band, 0)
			MOD_RADIO_REG_20691_ENTRY(pi, TX_TOP_5G_OVR2, 0,
				ovr_logen5g_tx_enable_5g_low_band, 1)

			/* # Restore PHY control for Gband blocks which may have been switched
			 * off in Aband
			 */
			MOD_RADIO_REG_20691_ENTRY(pi, LOGEN_OVR1, 0, ovr_logencore_5g_pu, 0)

			/* # Misc */
			/* # There is no direct control for this */
			MOD_RADIO_REG_20691_ENTRY(pi, TX_LPF_CFG2, 0, lpf_sel_5g_out_gm, 1)
		ACPHY_REG_LIST_EXECUTE(pi);

		/* Fix for LNA clamping issue in 4364-1x1 slice */
		if (IS_4364_1x1(pi)) {
			MOD_RADIO_REG_20691(pi, RX_TOP_5G_OVR, 0, ovr_rxrf5g_pu_pulse, 1);
			MOD_RADIO_REG_20691(pi, RXRF5G_CFG1, 0, rxrf5g_pu_pulse, 1);
			/* RSDB 1x1 Tx + 3x3 Rx: Force 2G TR override when 1x1 is in 5G */
			MOD_RADIO_REG_20691(pi, RX_TOP_2G_OVR_NORTH, 0, ovr_lna2g_tr_rx_en, 1);
			MOD_RADIO_REG_20691(pi, LNA2G_CFG1, 0, lna2g_tr_rx_en, 0);
			/* RSDB 1x1 Tx + 3x3 Rx: Remove 5G TR override when 1x1 is in 5G */
			MOD_RADIO_REG_20691(pi, RX_TOP_5G_OVR, 0, ovr_lna5g_tr_rx_en, 0);
			MOD_RADIO_REG_20691(pi, LNA5G_CFG1, 0, lna5g_tr_rx_en, 1);
		}
		/* # There is no direct control for this */
		MOD_RADIO_REG_20691(pi, TX_LPF_CFG3, 0, lpf_sel_2g_5g_cmref_gm,
				(PHY_IPA(pi)) ? 1 : 0);

		if ((RADIO20691_MAJORREV(pi->pubpi->radiorev) != 0) && !(PHY_IPA(pi))) {
			MOD_RADIO_REG_20691(pi, TXMIX2G_CFG6, 0, mx2g_idac_bbdc, 0xb);
			/* JIRA:SW4345-352 - Improve evm floor and ifs dependent iq imabalance */
			if (IBOARD(pi)) {
				ACPHY_REG_LIST_START
					MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG8, 0,
						pad5g_idac_gm, 0x18)
					MOD_RADIO_REG_20691_ENTRY(pi, PA5G_INCAP, 0,
						pad5g_idac_pmos, 0x34)
					MOD_RADIO_REG_20691_ENTRY(pi, TX5G_CFG1, 0, pad5g_slope_gm,
						0x0)
					MOD_RADIO_REG_20691_ENTRY(pi, TXMIX5G_CFG6, 0,
						mx5g_ptat_slope_lodc, 0x0)
					MOD_RADIO_REG_20691_ENTRY(pi, PA5G_INCAP, 0,
						pa5g_idac_incap_compen_main, 0x2f)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
		}
	}
	if (CHSPEC_IS80(pi->radio_chanspec)) {
		/* set gvalues [20691_sigdel_fast_tune $def(radio_rccal_adc_gmult)] */
		wlc_tiny_sigdel_fast_tune(pi, ri->rccal_adc_gmult, &gvalues);
		/* 20691_adc_setup_fast $gvalues */
		wlc_tiny_adc_setup_fast(pi, &gvalues, 0);
	} else {
		/* set gvalues [20691_sigdel_slow0g6_tune $def(radio_rccal_adc_gmult)] */
		wlc_tiny_sigdel_slow_tune(pi, ri->rccal_adc_gmult, &gvalues, bw);
		/* 20691_adc_setup_slow0g6 $gvalues */
		wlc_tiny_adc_setup_slow(pi, &gvalues, bw, 0);
	}

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
	    wlc_tiny_tia_config(pi, core);
	}
}

/*  lookup radio-chip-specific channel code */
int
wlc_phy_chan2freq_acphy(phy_info_t *pi, uint8 channel, const void **chan_info)
{
	uint i;
	const chan_info_radio2069_t *chan_info_tbl = NULL;
	const chan_info_radio2069revGE16_t *chan_info_tbl_GE16 = NULL;
	const chan_info_radio2069revGE25_t *chan_info_tbl_GE25 = NULL;
	const chan_info_radio2069revGE32_t *chan_info_tbl_GE32 = NULL;
	const chan_info_radio2069revGE25_52MHz_t *chan_info_tbl_GE25_52MHz = NULL;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint32 tbl_len = 0;
	int freq;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	switch (RADIO2069_MAJORREV(pi->pubpi->radiorev)) {
	case 0:
		switch (RADIO2069REV(pi->pubpi->radiorev)) {
		case 3:
			chan_info_tbl = chan_tuning_2069rev3;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev3);
		break;

		case 4:
		case 8:
			chan_info_tbl = chan_tuning_2069rev4;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev4);
			break;
		case 7: /* e.g. 43602a0 */
			chan_info_tbl = chan_tuning_2069rev7;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev7);
			break;
		case 64: /* e.g. 4364 */
			chan_info_tbl = chan_tuning_2069rev64;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev64);
			break;
		case 66: /* e.g. 4364 */
			chan_info_tbl = chan_tuning_2069rev66;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev66);
			break;
		default:

			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			           pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
			ASSERT(0);
		}
		break;

	case 1:
		switch (RADIO2069REV(pi->pubpi->radiorev)) {
			case 16:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
						(pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 = chan_tuning_2069rev_GE16_40_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE16_40_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17_40;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17_40);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else {
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
						(pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 = chan_tuning_2069rev_GE16_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE16_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17);
					}
				}
				pi_ac->radioi->acphy_prev_lp_mode = pi_ac->radioi->acphy_lp_mode;
				break;
			case 17:
			case 23:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
						(pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 =
						       chan_tuning_2069rev_GE16_40_lp;
						tbl_len =
						       ARRAYSIZE(chan_tuning_2069rev_GE16_40_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17_40;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17_40);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else {
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
#ifndef ACPHY_1X1_37P4
					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
						(pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 = chan_tuning_2069rev_GE16_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE16_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17);
					}
#else
					if ((RADIO2069REV(pi->pubpi->radiorev)) == 23) {
						chan_info_tbl_GE16 =
						 chan_tuning_2069rev_23_2Glp_5Gnonlp;
						tbl_len =
						 ARRAYSIZE(chan_tuning_2069rev_23_2Glp_5Gnonlp);

					} else {
						chan_info_tbl_GE16 =
						 chan_tuning_2069rev_GE16_2Glp_5Gnonlp;
						tbl_len =
						 ARRAYSIZE(chan_tuning_2069rev_GE16_2Glp_5Gnonlp);
					}
#endif /* ACPHY_1X1_37P4 */
				}
				pi_ac->radioi->acphy_prev_lp_mode = pi_ac->radioi->acphy_lp_mode;
				break;
			case 18:
			case 24:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
						(pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 =
						    chan_tuning_2069rev_GE16_40_lp;
						tbl_len =
						    ARRAYSIZE(chan_tuning_2069rev_GE16_40_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_18_40;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_18_40);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else {
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
					    (pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure LP mode settings */
						/* For Rev16/17/18 using same LP setting TBD */
						chan_info_tbl_GE16 =
						       chan_tuning_2069rev_GE16_lp;
						tbl_len =
						       ARRAYSIZE(chan_tuning_2069rev_GE16_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_18;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_18);
					}
				}
				pi_ac->radioi->acphy_prev_lp_mode = pi_ac->radioi->acphy_lp_mode;
				break;
			case 25:
			case 26:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;

					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
						(pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						chan_info_tbl_GE25 =
							chan_tuning_2069rev_GE_25_40MHz_lp;
						tbl_len =
						ARRAYSIZE(chan_tuning_2069rev_GE_25_40MHz_lp);
					} else {
						chan_info_tbl_GE25 =
						     chan_tuning_2069rev_GE_25_40MHz;
						tbl_len =
						     ARRAYSIZE(chan_tuning_2069rev_GE_25_40MHz);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else if (PHY_XTAL_IS52M(pi)) {
					chan_info_tbl_GE25_52MHz = pi->u.pi_acphy->chan_tuning;
					tbl_len = pi->u.pi_acphy->chan_tuning_tbl_len;
				} else {
					pi_ac->acphy_lp_status = pi_ac->radioi->acphy_lp_mode;
					if ((pi_ac->radioi->acphy_lp_mode == 2) ||
						(pi_ac->radioi->acphy_lp_mode == 3) ||
						(pi_ac->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						chan_info_tbl_GE25 = chan_tuning_2069rev_GE_25_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE_25_lp);
					} else {
						chan_info_tbl_GE25 = chan_tuning_2069rev_GE_25;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE_25);
					}
				}
				pi_ac->radioi->acphy_prev_lp_mode = pi_ac->radioi->acphy_lp_mode;
				break;
			default:
				PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
				   pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
				ASSERT(0);
		}

		break;

	case 2:
		switch (RADIO2069REV(pi->pubpi->radiorev)) {
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
		case 40:
		case 44:
			/* can have more conditions based on different radio revs */
			/*  RADIOREV(pi->pubpi->radiorev) =32/33/34 */
			/* currently tuning tbls for these are all same */
			chan_info_tbl_GE32 = pi->u.pi_acphy->chan_tuning;
			tbl_len = pi->u.pi_acphy->chan_tuning_tbl_len;
			break;

		default:

			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			           pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
			ASSERT(0);
		}
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio major revision %d\n",
		           pi->sh->unit, __FUNCTION__, RADIO2069_MAJORREV(pi->pubpi->radiorev)));
		ASSERT(0);
	}

	for (i = 0; i < tbl_len; i++) {

		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			if (chan_info_tbl_GE32[i].chan == channel)
				break;
		} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
			if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
			   (RADIO2069REV(pi->pubpi->radiorev) == 26))  {
			    if (!PHY_XTAL_IS52M(pi)) {
					if (chan_info_tbl_GE25[i].chan == channel)
						break;
				} else {
					if (chan_info_tbl_GE25_52MHz[i].chan == channel)
						break;
				}
			}
			else if (chan_info_tbl_GE16[i].chan == channel)
				break;
		} else {
			if (chan_info_tbl[i].chan == channel)
				break;
		}
	}

	if (i >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
		           pi->sh->unit, __FUNCTION__, channel));
		ASSERT(i < tbl_len);

		return -1;
	}

	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		*chan_info = &chan_info_tbl_GE32[i];
		freq = chan_info_tbl_GE32[i].freq;
	} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
		if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
			(RADIO2069REV(pi->pubpi->radiorev) == 26)) {
				if (!PHY_XTAL_IS52M(pi)) {
					*chan_info = &chan_info_tbl_GE25[i];
					freq = chan_info_tbl_GE25[i].freq;
				} else {
					*chan_info = &chan_info_tbl_GE25_52MHz[i];
					freq = chan_info_tbl_GE25_52MHz[i].freq;
				}
		} else {
			*chan_info = &chan_info_tbl_GE16[i];
			freq = chan_info_tbl_GE16[i].freq;
		}
	} else {
		*chan_info = &chan_info_tbl[i];
		freq = chan_info_tbl[i].freq;
	}

	return freq;
}

int
wlc_phy_chan2freq_20691(phy_info_t *pi, uint8 channel, const chan_info_radio20691_t **chan_info)
{
	uint i;
	const chan_info_radio20691_t *chan_info_tbl;
	uint32 tbl_len;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20691_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Choose the right table to use */
	switch (RADIO20691REV(pi->pubpi->radiorev)) {
	case 60:
	case 68:
		chan_info_tbl = chan_tuning_20691_rev68;
		tbl_len = ARRAYSIZE(chan_tuning_20691_rev68);
		break;
	case 75:
		chan_info_tbl = chan_tuning_20691_rev75;
		tbl_len = ARRAYSIZE(chan_tuning_20691_rev75);
		break;
	case 79:
		chan_info_tbl = chan_tuning_20691_rev79;
		tbl_len = ARRAYSIZE(chan_tuning_20691_rev79);
		break;
	case 74:
	case 82:
		chan_info_tbl = chan_tuning_20691_rev82;
		tbl_len = ARRAYSIZE(chan_tuning_20691_rev82);
		break;
	case 85:
	case 86:
	case 87:
	case 88:
		chan_info_tbl = chan_tuning_20691_rev88;
		tbl_len = ARRAYSIZE(chan_tuning_20691_rev88);
		break;
	case 129:
		chan_info_tbl = chan_tuning_20691_rev129;
		tbl_len = ARRAYSIZE(chan_tuning_20691_rev129);
		break;
	case 130:
		chan_info_tbl = chan_tuning_20691_rev130;
		tbl_len = ARRAYSIZE(chan_tuning_20691_rev130);
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO20691REV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return -1;
	}

	for (i = 0; i < tbl_len && chan_info_tbl[i].chan != channel; i++);

	if (i >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
		           pi->sh->unit, __FUNCTION__, channel));
		ASSERT(tbl_len == 0 || i < tbl_len);
		return -1;
	}

	*chan_info = &chan_info_tbl[i];

	return chan_info_tbl[i].freq;
}

/* 20691_lpf_tx_set is the top Tx LPF function and should be the usual */
/* function called from acphyprocs or used from the REPL in the lab */
void
wlc_phy_radio_tiny_lpf_tx_set(phy_info_t *pi, int8 bq_bw, int8 bq_gain,
	int8 rc_bw_ofdm, int8 rc_bw_cck)
{
	uint8 i, core;
	uint16 gmult;
	uint16 gmult_rc;
	uint16 g10_tuned, g11_tuned, g12_tuned, g21_tuned, bias;

	gmult = pi->u.pi_acphy->radioi->data.rccal_gmult;
	gmult_rc = pi->u.pi_acphy->radioi->data.rccal_gmult_rc;

	/* search for given bq_gain */
	for (i = 0; i < ARRAYSIZE(g_index1); i++) {
		if (bq_gain == g_index1[i])
			break;
	}

	if (i < ARRAYSIZE(g_index1)) {
		uint16 g_passive_rc_tx_tuned_ofdm, g_passive_rc_tx_tuned_cck;
		g10_tuned = (lpf_g10[bq_bw][i] * gmult) >> 15;
		g11_tuned = (lpf_g11[bq_bw] * gmult) >> 15;
		g12_tuned = (lpf_g12[bq_bw][i] * gmult) >> 15;
		g21_tuned = (lpf_g21[bq_bw][i] * gmult) >> 15;
		g_passive_rc_tx_tuned_ofdm = (g_passive_rc_tx[rc_bw_ofdm] * gmult_rc) >> 15;
		g_passive_rc_tx_tuned_cck = (g_passive_rc_tx[rc_bw_cck] * gmult_rc) >> 15;
		if (ACMAJORREV_3(pi->pubpi->phy_rev) &&
				RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
		   g10_tuned = (g10_tuned > 8191) ? 8191 : g10_tuned;
		   g11_tuned = (g11_tuned > 8191) ? 8191 : g11_tuned;
		   g12_tuned = (g12_tuned > 8191) ? 8191 : g12_tuned;
		   g21_tuned = (g21_tuned > 8191) ? 8191 : g21_tuned;
		   g_passive_rc_tx_tuned_ofdm = (g_passive_rc_tx_tuned_ofdm > 511)
			   ? 511 : g_passive_rc_tx_tuned_ofdm;
		   g_passive_rc_tx_tuned_cck = (g_passive_rc_tx_tuned_cck > 511)
			   ? 511 : g_passive_rc_tx_tuned_cck;
		}
		bias = biases[bq_bw];
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG3, core, lpf_g10, g10_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG7, core, lpf_g11, g11_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG4, core, lpf_g12, g12_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG5, core, lpf_g21, g21_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG6, core, lpf_g_passive_rc_tx,
				g_passive_rc_tx_tuned_ofdm);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG8, core, lpf_bias_bq, bias);
		}
		if (D11REV_IS(pi->sh->corerev, 47) || D11REV_IS(pi->sh->corerev, 54) ||
			D11REV_IS(pi->sh->corerev, 58)) {
			/* Note down the values of the passive_rc for OFDM and CCK in Shmem */
			wlapi_bmac_write_shm(pi->sh->physhim,
				M_LPF_PASSIVE_RC_OFDM(pi), g_passive_rc_tx_tuned_ofdm);
			wlapi_bmac_write_shm(pi->sh->physhim, M_LPF_PASSIVE_RC_CCK(pi),
				g_passive_rc_tx_tuned_cck);
		}
	} else {
		PHY_ERROR(("wl%d: %s: Invalid bq_gain %d\n", pi->sh->unit, __FUNCTION__, bq_gain));
	}
	/* Change IIR filter shape to solve bandedge problem for 42q(5210q) channel */
	if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
		(CHSPEC_IS80(pi->radio_chanspec))) {
		if (CHSPEC_CHANNEL(pi->radio_chanspec) == 42) {
			WRITE_PHYREG(pi, txfilt80st0a1, 0xf8b);
			WRITE_PHYREG(pi, txfilt80st0a2, 0x343);
			WRITE_PHYREG(pi, txfilt80st1a1, 0xe72);
			WRITE_PHYREG(pi, txfilt80st1a2, 0x11f);
			WRITE_PHYREG(pi, txfilt80st2a1, 0xc57);
			WRITE_PHYREG(pi, txfilt80st2a2, 0x166);
			/* Reducing rc_filter bandwidth for 42q channel */
			/* MOD_RADIO_ALLREG_20693(pi, TX_LPF_CFG6, lpf_g_passive_rc_tx, 25); */
		} else {
			WRITE_PHYREG(pi, txfilt80st0a1, 0xf93);
			WRITE_PHYREG(pi, txfilt80st0a2, 0x36e);
			WRITE_PHYREG(pi, txfilt80st1a1, 0xdf7);
			WRITE_PHYREG(pi, txfilt80st1a2, 0x257);
			WRITE_PHYREG(pi, txfilt80st2a1, 0xbe2);
			WRITE_PHYREG(pi, txfilt80st2a2, 0x16c);
		}
	}
}

static void
wlc_phy_radio20693_set_channel_bw(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int row;
	uint8 dac_rate_mode;
	uint8 dacclkdiv = 0;
	uint16 dac_rate;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	}
	row = wlc_phy_radio20693_altclkpln_get_chan_row(pi);

	dac_rate_mode = pi_ac->dac_mode;
	dac_rate = wlc_phy_get_dac_rate_from_mode(pi, dac_rate_mode);
	ASSERT(dac_rate_mode <= 2);

	if (adc_mode == RADIO_20693_FAST_ADC) {
		if (dac_rate_mode == 1) {
			if (dac_rate == 200)
				dacclkdiv = 3;
			else if (dac_rate == 400)
				dacclkdiv = 1;
			else
				dacclkdiv = 0;
		} else if (dac_rate_mode == 2) {
			dacclkdiv = 0;
		}
	} else {
		if (dac_rate_mode == 1) {
#if !defined(MACOSX)
			if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
				if (CHSPEC_IS5G(pi->radio_chanspec) &&
					(CHSPEC_IS20(pi->radio_chanspec) ||
					CHSPEC_IS40(pi->radio_chanspec)) &&
					!PHY_IPA(pi) && !ROUTER_4349(pi)) {
					dacclkdiv = 1;
				} else {
					dacclkdiv = 0;
				}
			} else {
				dacclkdiv = 0;
			}
#else
			dacclkdiv = 0;
#endif /* MACOSX */
		} else {
			PHY_ERROR(("wl%d: %s: unknown dac rate mode slow\n",
				pi->sh->unit, __FUNCTION__));
		}
	}
	if ((row >= 0) && (adc_mode != RADIO_20693_FAST_ADC)) {
		dacclkdiv = altclkpln[row].dacclkdiv;
	}
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, sel_dac_div, dacclkdiv);
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
#if !defined(MACOSX)
		if (CHSPEC_IS5G(pi->radio_chanspec) &&
			(CHSPEC_IS20(pi->radio_chanspec) ||
			CHSPEC_IS40(pi->radio_chanspec)) &&
			!PHY_IPA(pi) && !ROUTER_4349(pi)) {
			MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_ctl_misc, 0);
		} else {
			MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_ctl_misc, 4);
		}
#endif /* MACOSX */

		/* SWWLAN-79504: Increasing the DAC frequency for Ch-64 in order to avoid DAC spur
		 * in other core in RSDB mode
		 */
		if (row >= 0) {
			MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_ctl_misc,
				altclkpln[row].dacdiv << 2);
		}

	}
}

int
wlc_phy_radio20693_altclkpln_get_chan_row(phy_info_t *pi)
{
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int tbl_len = ARRAYSIZE(altclkpln_radio20693);
	int row;
	int channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	int bw	= CHSPEC_IS20(pi->radio_chanspec) ? 20 : CHSPEC_IS40(pi->radio_chanspec) ? 40 : 80;

	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	        tbl_len = ARRAYSIZE(altclkpln_radio20693_router4349);
	}
#ifdef WL11ULB
	if (PHY_ULB_ENAB(pi->sh->physhim)) {
		if (CHSPEC_IS10(pi->radio_chanspec))
			bw = 10;
		else if (CHSPEC_IS5(pi->radio_chanspec))
			bw = 5;
		else if (CHSPEC_IS2P5(pi->radio_chanspec))
			bw = 2;
	}
#endif	/* WL11ULB */
	for (row = 0; row < tbl_len; row++) {
		if ((altclkpln[row].channel == channel) && (altclkpln[row].bw == bw)) {
			break;
		}
	}

	/* 53574: SWWLAN-79504 & SWWLAN-76882: DAC/ADC-SIPO spur issue is seen in RSDB mode */
	if (ROUTER_4349(pi) && (phy_get_phymode(pi) != PHYMODE_RSDB)) {
		row = tbl_len;
	}
	return ((row < tbl_len) &&
		(ROUTER_4349(pi) ? ALTCLKPLN_ENABLE_ROUTER4349 : ALTCLKPLN_ENABLE)) ? row : -1;
}

static void
wlc_phy_radio20693_adc_config_overrides(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	uint8 is_fast;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	is_fast = (adc_mode == RADIO_20693_FAST_ADC);
	wlc_phy_radio20693_adc_powerupdown(pi, RADIO_20693_SLOW_ADC, !is_fast, core);
	wlc_phy_radio20693_adc_powerupdown(pi, RADIO_20693_FAST_ADC, is_fast, core);
}

static void
wlc_phy_radio20693_adc_dac_setup(phy_info_t *pi, radio_20693_adc_modes_t adc_mode, uint8 core)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (RADIOMAJORREV(pi) != 3) {
		wlc_phy_radio20693_afeclkpath_setup(pi, core, adc_mode, 0);
		wlc_phy_radio20693_adc_config_overrides(pi, adc_mode, core);
	}

	wlc_phy_radio20693_adc_setup(pi->u.pi_acphy->radioi, core, adc_mode);

	if (RADIOMAJORREV(pi) != 3)
		wlc_phy_radio20693_set_channel_bw(pi, adc_mode, core);

	if (RADIOMAJORREV(pi) != 3)
		wlc_phy_radio20693_config_bf_mode(pi, core);
}

static void
wlc_phy_radio20693_setup_crisscorss_ovr(phy_info_t *pi, uint8 core)
{
	uint8 pupd = 1;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR2, core, ovr_tx5g_80p80_cas_pu, pupd);
	MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR2, core, ovr_tx5g_80p80_gm_pu, pupd);
	MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR1_EAST, core, ovr_tx2g_20p20_cas_pu, pupd);
	MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR1_EAST, core, ovr_tx2g_20p20_gm_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_rx5g_80p80_src_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_rx5g_80p80_des_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_rx5g_80p80_gc, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST, core, ovr_rx2g_20p20_src_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST, core, ovr_rx2g_20p20_des_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST, core, ovr_rx2g_20p20_gc, pupd);
}

static void
wlc_phy_radio20695_apply_adccal_result(phy_info_t *pi)
{
	uint8 temp_var[3];
	uint16 temp[3];

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG3, 0,
			rxadc_coeff_out_ctrl_adc_I, 0x3)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG4, 0,
			rxadc_coeff_out_ctrl_adc_Q, 0x3)
	ACPHY_REG_LIST_EXECUTE(pi);
	WRITE_RADIO_REG_28NM(pi, RF, AFE_CFG1_OVR1, 0, 0x0FFF);
	temp[0] = READ_RADIO_REG_20695(pi, RF, RXADC_CAP0_STAT, 0);
	temp[1] = READ_RADIO_REG_20695(pi, RF, RXADC_CAP1_STAT, 0);
	temp[2] = READ_RADIO_REG_20695(pi, RF, RXADC_CAP2_STAT, 0);

	temp_var[0] = (uint8)((temp[0] & 0xFE00) >> 9);
	temp_var[1] = (uint8)((temp[1] & 0xFE00) >> 9);
	temp_var[2] = (uint8)((temp[2] & 0xFE00) >> 9);

	MOD_RADIO_REG_20695_MULTI_3(pi, RF, RXADC_CFG5, 0, rxadc_coeff_cap0_adc_I, temp_var[0],
		rxadc_coeff_cap1_adc_I, temp_var[1], rxadc_coeff_cap2_adc_I, temp_var[2]);

	temp_var[0] = (uint8)(temp[0] & 0x7F);
	temp_var[1] = (uint8)(temp[1] & 0x7F);
	temp_var[2] = (uint8)(temp[2] & 0x7F);

	MOD_RADIO_REG_20695_MULTI_3(pi, RF, RXADC_CFG7, 0, rxadc_coeff_cap0_adc_Q, temp_var[0],
		rxadc_coeff_cap1_adc_Q, temp_var[1], rxadc_coeff_cap2_adc_Q, temp_var[2]);
}

static void
wlc_phy_radio20691_afecal(phy_info_t *pi)
{
	uint8 core, bw;
	tiny_adc_tuning_array_t gvalues;
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;
	bw = CHSPEC_IS20(pi->radio_chanspec) ? 20 : CHSPEC_IS40(pi->radio_chanspec) ? 40 : 80;

	/* enable ?? wlc_phy_radio20691_rccal(pi); */
	if (CHSPEC_IS80(pi->radio_chanspec)) {
		/* set gvalues [20691_sigdel_fast_tune $def(radio_rccal_adc_gmult)] */
		wlc_tiny_sigdel_fast_tune(pi, ri->rccal_adc_gmult, &gvalues);
		/* 20691_adc_setup_fast $gvalues */
		wlc_tiny_adc_setup_fast(pi, &gvalues, 0);
	} else {
		/* set gvalues [20691_sigdel_slow1g2_tune $def(radio_rccal_adc_gmult)] */
		wlc_tiny_sigdel_slow_tune(pi, ri->rccal_adc_gmult, &gvalues, bw);
		/* 20691_adc_setup_slow1g2 $gvalues */
		wlc_tiny_adc_setup_slow(pi, &gvalues, bw, 0);
	}

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		wlc_tiny_tia_config(pi, core);
	}
}

static void
wlc_phy_radio20693_afecal(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 core;
	radio_20693_adc_modes_t adc_mode;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (((phy_ac_chanmgr_get_data(pi_ac->chanmgri))->fast_adc_en == 1) ||
		(CHSPEC_IS80(pi->radio_chanspec)) || (CHSPEC_IS160(pi->radio_chanspec)) ||
		(CHSPEC_IS8080(pi->radio_chanspec))) {
		adc_mode = RADIO_20693_FAST_ADC;
	} else {
		adc_mode = RADIO_20693_SLOW_ADC;
	}

	FOREACH_CORE(pi, core) {
		wlc_phy_radio20693_adc_dac_setup(pi, adc_mode, core);
		wlc_tiny_tia_config(pi, core);
		if (RADIOMAJORREV(pi) != 3) {
			wlc_phy_radio20693_setup_crisscorss_ovr(pi, core);
		}
	}
}

static void
wlc_phy_radio20694_afecal(phy_info_t *pi)
{
	uint8 core;
	uint16 r, invclk_sv;
	uint8 adccapcal_timeout = 20;
	uint sicoreunit = wlapi_si_coreunit(pi->sh->physhim);

	FOREACH_CORE(pi, core) {
		if ((sicoreunit != DUALMAC_AUX) || (core != 1)) {
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, core,
					ovr_afediv_adc_en, 1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, core,
					afediv_adc_en, 1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, core,
					ovr_afediv_dac_en, 1);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_REG5, core,
					afediv_dac_en, 1);
		}
	}
	wlc_phy_resetcca_acphy(pi);

	FOREACH_CORE(pi, core) {
		invclk_sv = READ_RADIO_REGFLD_20694(pi, RF, TXDAC_REG0, core, iqdac_invclko);
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_invclko, 0);

		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdac_pu_diode, 1);
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_pwrup_diode, 1);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdac_pu_I, 1);
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_pwrup_I, 1);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdac_pu_Q, 1);
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_pwrup_Q, 1);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdacbuf_pu, 1);
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_buf_pu, 1);

		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc1_en, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc1_en, 1);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc0_en, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc0_en, 1);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc_pu_I, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_puI, 1);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc_pu_Q, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_puQ, 1);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc_power_mode, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_power_mode_enb, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_power_mode, 3);
		r = READ_RADIO_REG_20694(pi, RF, RXADC_REG6, core);
		r = r | 0x00c0;
		WRITE_RADIO_REG_20694(pi, RF, RXADC_REG6, core, r);

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, afe_en_loopback, 1);
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_buf_cmsel, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG3, core, rxadc_cal_cap, 7);

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG1, core, rxadc_buffer_per, 31);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG1, core, rxadc_wait_per, 31);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG2, core, rxadc_sum_per, 127);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG1, core, rxadc_sum_div, 5);
		//MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0);
		//MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0);
		//MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0);
		//MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0);

		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 1);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 1);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 1);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 1);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 1);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 1);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 1);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_bq2_adc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_bq2_rc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_dac_bq2, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_dac_rc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_bq1_adc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_bq1_bq2, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_REG7, core, lpf_sw_aux_adc, 0);

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG1, core, rxadc_corr_dis, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG1, core, rxadc_coeff_sel, 0);

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc0_I, 3);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG4, core, rxadc_coeff_out_ctrl_adc0_Q, 3);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc1_I, 3);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG4, core, rxadc_coeff_out_ctrl_adc1_Q, 3);

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0);

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc0_I, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc0_Q, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc1_I, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc1_Q, 0);
		OSL_DELAY(100);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc0_I, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc0_Q, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc1_I, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_reset_n_adc1_Q, 1);
		OSL_DELAY(40);

		//trigger CAL
		adccapcal_timeout = 20;
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 1);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 1);

		OSL_DELAY(20);
		r = READ_RADIO_REG_20694(pi, RF, RXADC_CAL_STATUS, core) & 0xf;
		while ((adccapcal_timeout > 10) && (r != 0xf)) {
			OSL_DELAY(10);
			r = READ_RADIO_REG_20694(pi, RF, RXADC_CAL_STATUS, core);
			adccapcal_timeout--;
		}

		if (adccapcal_timeout == 10 && (r != 0xf)) {
			while (adccapcal_timeout > 0 && (r != 0xf)) {
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc0_I, 0);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc0_Q, 0);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc1_I, 0);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc1_Q, 0);

				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc0_I, 0);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc0_Q, 0);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc1_I, 0);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc1_Q, 0);
				OSL_DELAY(100);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc0_I, 1);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc0_Q, 1);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc1_I, 1);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_reset_n_adc1_Q, 1);
				OSL_DELAY(40);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc0_I, 1);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc0_Q, 1);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc1_I, 1);
				MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core,
						rxadc_start_cal_adc1_Q, 1);
				OSL_DELAY(60);
				adccapcal_timeout--;
				r = READ_RADIO_REG_20694(pi, RF, RXADC_CAL_STATUS, core) & 0xf;
			}
		}
		if (r != 0xf) {
			PHY_ERROR(("ADCCAPCAL is not done properly on core %d\n", core));
		} else if (r == 0xf && adccapcal_timeout >= 10) {
			PHY_INFORM(("ADCCAPCAL is good in first try on core %d\n", core));
		} else {
			PHY_INFORM(("ADCCAPCAL is good after %d extra try on core %d\n",
				10-adccapcal_timeout, core));
		}

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0);

		//apply_adc_cal_result
		WRITE_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR1, core, 0x0FFF);

		r = READ_RADIO_REG_20694(pi, RF, RXADC0_CAP0_STAT, core);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG6, core, rxadc_coeff_cap0_adc0_I, (r>>8)&0xFF);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG9, core, rxadc_coeff_cap0_adc0_Q, r&0xFF);

		r = READ_RADIO_REG_20694(pi, RF, RXADC0_CAP1_STAT, core);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG5, core, rxadc_coeff_cap1_adc0_I, (r>>8)&0xFF);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG8, core, rxadc_coeff_cap1_adc0_Q, r&0xFF);

		r = READ_RADIO_REG_20694(pi, RF, RXADC0_CAP2_STAT, core);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG5, core, rxadc_coeff_cap2_adc0_I, (r>>8)&0xFF);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG8, core, rxadc_coeff_cap2_adc0_Q, r&0xFF);

		r = READ_RADIO_REG_20694(pi, RF, RXADC1_CAP0_STAT, core);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG7, core, rxadc_coeff_cap0_adc1_I, (r>>8)&0xFF);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG11, core, rxadc_coeff_cap0_adc1_Q, r&0xFF);

		r = READ_RADIO_REG_20694(pi, RF, RXADC1_CAP1_STAT, core);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG7, core, rxadc_coeff_cap1_adc1_I, (r>>8)&0xFF);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG11, core, rxadc_coeff_cap1_adc1_Q, r&0xFF);

		r = READ_RADIO_REG_20694(pi, RF, RXADC1_CAP2_STAT, core);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG6, core, rxadc_coeff_cap2_adc1_I, (r>>8)&0xFF);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG9, core, rxadc_coeff_cap2_adc1_Q, r&0xFF);

		WRITE_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR1, core, 0x0000);

		//MOD_RADIO_REG_20694(pi, RF, RXADC_CFG1, core, rxadc_corr_dis, 0);
		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG1, core, rxadc_coeff_sel, 1);

		PHY_INFORM(("ADCCAPCAL: after write back, core%d: corr_dis %d coeff_sel %d\n", core,
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG1,
			core, rxadc_corr_dis),
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG1,
			core, rxadc_coeff_sel)));
		PHY_INFORM(("ADCCAPCAL: core%d: adc0: cap0 I: 0x%2x Q: 0x%2x\n", core,
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG6,
			core, rxadc_coeff_cap0_adc0_I),
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG9,
			core, rxadc_coeff_cap0_adc0_Q)));
		PHY_INFORM(("ADCCAPCAL: core%d: adc0: cap1 I: 0x%2x Q: 0x%2x\n", core,
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG5,
			core, rxadc_coeff_cap1_adc0_I),
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG8,
			core, rxadc_coeff_cap1_adc0_Q)));
		PHY_INFORM(("ADCCAPCAL: core%d: adc0: cap2 I: 0x%2x Q: 0x%2x\n", core,
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG5,
			core, rxadc_coeff_cap2_adc0_I),
				READ_RADIO_REGFLD_20694(pi, RF, RXADC_CFG8,
			core, rxadc_coeff_cap2_adc0_Q)));

		//clear out overrides
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0);
		MOD_RADIO_REG_20694(pi, RF, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0);

		MOD_RADIO_REG_20694(pi, RF, RXADC_CFG0, core, afe_en_loopback, 0);
		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_buf_cmsel, 0);

		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc0_en, 0);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc1_en, 0);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc_pu_I, 0);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc_pu_Q, 0);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_adc_power_mode, 0);

		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdac_pu_diode, 0);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdac_pu_I, 0);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdac_pu_Q, 0);
		MOD_RADIO_REG_20694(pi, RF, AFE_CFG1_OVR2, core, ovr_iqdacbuf_pu, 0);

		r = READ_RADIO_REG_20694(pi, RF, RXADC_REG6, core);
		r = r & 0xff3f;
		WRITE_RADIO_REG_20694(pi, RF, RXADC_REG6, core, r);

		MOD_RADIO_REG_20694(pi, RF, TXDAC_REG0, core, iqdac_invclko, invclk_sv);
	}

	FOREACH_CORE(pi, core) {
		if ((sicoreunit != DUALMAC_AUX) || (core != 1)) {
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, core,
					ovr_afediv_adc_en, 0);
			MOD_RADIO_REG_20694(pi, RF, AFEDIV_CFG1_OVR, core,
					ovr_afediv_dac_en, 0);
		}
	}
}

static void
wlc_phy_radio20695_afecal(phy_info_t *pi)
{
	uint16 adccapcal_timeout_I, adccapcal_timeout_Q, adccapcal_done;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	/* Save AFE registers */
	phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		MOD_RADIO_REG_28NM(pi, RF, AFEDIV2G_CFG1_OVR, 0, ovr_afediv2g_dac_en, 1);
		MOD_RADIO_REG_28NM(pi, RF, AFEDIV2G_REG5, 0, afediv2g_dac_en, 1);
	} else {
		MOD_RADIO_REG_28NM(pi, RF, AFEDIV5G_CFG1_OVR, 0, ovr_afediv5g_dac_en, 1);
		MOD_RADIO_REG_28NM(pi, RF, AFEDIV5G_REG5, 0, afediv5g_dac_en, 1);
	}

	if (pi_ac->ulp_adc_mode == 2) {
		MOD_RADIO_REG_28NM(pi, RF, RXADC_CFG0, 0, rxadc_power_mode, 3);
	} else {
		MOD_RADIO_REG_28NM(pi, RF, RXADC_CFG0, 0, rxadc_power_mode, 0);
	}

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TXDAC_REG0, 0, iqdac_buf_cmsel, 1)

		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdacbuf_pu, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdac_pu_diode, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdac_pu_I, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdac_pu_Q, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TXDAC_REG0, 0, iqdac_pwrup_I, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TXDAC_REG0, 0, iqdac_pwrup_Q, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TXDAC_REG0, 0, iqdac_buf_pu, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TXDAC_REG0, 0, iqdac_pwrup_diode, 1)

		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_pu_I, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_pu_Q, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_power_mode, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_puI, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_puQ, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_power_mode_enb, 0)

		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_bq2_adc, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_wbcal_adc, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_aux_adc, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_REG16, 0, tia_sw_bq2_adc, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_REG18, 0, tia_sw_wbcal_adc, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_REG16, 0, tia_sw_aux_adc, 0)

		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG1, 0, rxadc_buffer_per, 0x1f)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG1, 0, rxadc_wait_per, 0x1f)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG2, 0, rxadc_sum_per, 0x3f)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG1, 0, rxadc_sum_div, 4)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG3, 0, rxadc_cal_cap, 7)

		/* Change coeff sel to zero before cal */
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG1, 0, rxadc_coeff_sel, 0)

		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, afe_en_loopback, 0x1)
		/* Reset ADC cal engine	*/
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_reset_n_adc_I, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_reset_n_adc_I, 0)
		/* Start the calibration for I */
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_start_cal_adc_I, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

	adccapcal_timeout_I = 20;
	adccapcal_done = READ_RADIO_REGFLD_28NM(pi, RF, RXADC_CAL_STATUS, 0, rxadc_cal_done_I);
	while ((adccapcal_done != 1) && (adccapcal_timeout_I > 0))
	{
		OSL_DELAY(10);
		adccapcal_done = READ_RADIO_REGFLD_28NM(pi, RF, RXADC_CAL_STATUS, 0,
			rxadc_cal_done_I);
		adccapcal_timeout_I = adccapcal_timeout_I -1;
	}

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_start_cal_adc_I, 0)
		/* Reset ADC cal engine	*/
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_reset_n_adc_Q, 1)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_reset_n_adc_Q, 0)
		/* Start the calibration for Q */
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, RXADC_CFG0, 0, rxadc_start_cal_adc_Q, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

	adccapcal_timeout_Q = 20;
	adccapcal_done = READ_RADIO_REGFLD_28NM(pi, RF, RXADC_CAL_STATUS, 0, rxadc_cal_done_Q);
	while ((adccapcal_done != 1) && (adccapcal_timeout_Q > 0))
	{
		OSL_DELAY(10);
		adccapcal_done = READ_RADIO_REGFLD_28NM(pi, RF, RXADC_CAL_STATUS, 0,
			rxadc_cal_done_Q);
		adccapcal_timeout_Q = adccapcal_timeout_Q -1;
	}
	MOD_RADIO_REG_28NM(pi, RF, RXADC_CFG0, 0, rxadc_start_cal_adc_Q, 0);

	if ((adccapcal_timeout_I > 0) && (adccapcal_timeout_Q > 0))
	{
		wlc_phy_radio20695_apply_adccal_result(pi);
	}

	MOD_RADIO_REG_28NM(pi, RF, RXADC_CFG0, 0, afe_en_loopback, 0);
	ACPHY_REG_LIST_START
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_bq2_adc, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_wbcal_adc, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_aux_adc, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_pu_I, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_pu_Q, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_adc_power_mode, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdacbuf_pu, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdac_pu_diode, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdac_pu_I, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, AFE_CFG1_OVR2, 0, ovr_iqdac_pu_Q, 0)
		MOD_RADIO_REG_28NM_ENTRY(pi, RF, TXDAC_REG0, 0, iqdac_buf_cmsel, 0)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* Restore AFE registers */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

	/* Change coeff sel to one after cal (take bypass coeffs) */
	MOD_RADIO_REG_28NM(pi, RF, RXADC_CFG1, 0, rxadc_coeff_sel, 1);
}

static void
wlc_phy_radio20696_afecal(phy_info_t *pi)
{
	uint8 core;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20696_ID);

	FOREACH_CORE(pi, core) {
		wlc_phy_radio20696_adc_setup(pi, core);
	}
	wlc_phy_radio20696_adc_cap_cal(pi, ADC_CAP_CAL_MODE_PARALLEL);
}

static void
wlc_phy_radio20696_adc_cap_cal(phy_info_t *pi, int mode)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	uint16 adccapcal_Timeout;
	uint16 bbmult = 0;

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* WAR: Force RFSeq is needed to get rid of time-outs and set_tx_bbmult
	 *      to get also the first time after a power cycle valid results
	 */
	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);
	FOREACH_CORE(pi, core) {
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);
	}

	if (mode == ADC_CAP_CAL_MODE_BYPASS) {
		/* Bypass ADC cap calibration and use ideal calibration values */
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
		}
	} else {
		/* Save AFE registers */
		phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);

		FOREACH_CORE(pi, core) {
			wlc_phy_radio20696_adc_cap_cal_setup(pi, core);
			if (mode == ADC_CAP_CAL_MODE_PARALLEL) {
				wlc_phy_radio20696_adc_cap_cal_parallel(pi, core,
					&adccapcal_Timeout);
			} else {
				wlc_phy_radio20696_adc_cap_cal_sequential(pi, core,
					&adccapcal_Timeout);
			}
			if (adccapcal_Timeout == 0) {
				PHY_ERROR(("%s: adc_cap_cal -- core%d - DISABLED correction, "
					"due to cal TIMEOUT\n", __FUNCTION__, core));
				MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
			} else {
				wlc_phy_radio20696_apply_adc_cal_result(pi, core);
				MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
			}
			/* Cleanup */
			MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x1);
			MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, afe_en_loopback, 0x0);
		}
		/* Restore AFE registers */
		phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);
	}
}

static void
wlc_phy_radio20696_adc_cap_cal_setup(phy_info_t *pi, uint8 core)
{
	uint16 val;

	/* Power up DAC and ADC by override */
	/* Set following AFE_CFG1_OVR2 bits:
	 * ovr_iqdac_reset       0x2000
	 * ovr_iqdac_buf_pu      0x0800
	 * ovr_iqdac_pwrup_diode 0x0400
	 * ovr_iqdac_pwrup_I     0x0040
	 * ovr_iqdac_pwrup_Q     0x0020
	 * ovr_rxadc_puI         0x0004
	 * ovr_rxadc_puQ         0x0002
	 * ovr_rxadc_power_mode  0x0001
	 */
	val = READ_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core);
	val |= 0x2c67;
	WRITE_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, val);
	/* Set following TXDAC_REG0 bits:
	 * iqdac_pwrup_diode     0x0001
	 * iqdac_pwrup_I         0x0002
	 * iqdac_pwrup_Q         0x0004
	 * iqdac_buf_pu          0x0008
	 * iqdac_reset           0x0010
	 * iqdac_buf_cmsel       0x0400
	 */
	val = READ_RADIO_REG_20696(pi, TXDAC_REG0, core);
	val |= 0x041f;
	WRITE_RADIO_REG_20696(pi, TXDAC_REG0, core, val);

	/* Set following RXADC_CFG0 bits:
	 * rxadc_puI             0x0004
	 * rxadc_puQ             0x0008
	 * rxadc_power_mode      0x0300
	 * afe_en_loopback       0x8000
	 * Clear following RXADC_CFG0 bits:
	 * rxadc_power_mode_enb  0x0400
	 */
	val = READ_RADIO_REG_20696(pi, RXADC_CFG0, core);
	val |= 0x830c;
	val &= 0xfbff;
	WRITE_RADIO_REG_20696(pi, RXADC_CFG0, core, val);

	/* Disconnect ADC from others */
	/* Set following LPF_OVR1 bits:
	 * ovr_lpf_sw_bq2_adc    0x0400
	 * ovr_lpf_sw_bq2_rc     0x0800
	 * ovr_lpf_sw_dac_bq2    0x1000
	 * ovr_lpf_sw_dac_rc     0x2000
	 */
	val = READ_RADIO_REG_20696(pi, LPF_OVR1, core);
	val |= 0x3c00;
	WRITE_RADIO_REG_20696(pi, LPF_OVR1, core, val);
	/* Set following LPF_OVR2 bits:
	 * ovr_lpf_sw_bq1_adc    0x0001
	 * ovr_lpf_sw_bq1_bq2    0x0002
	 * ovr_lpf_sw_aux_adc    0x0004
	 */
	val = READ_RADIO_REG_20696(pi, LPF_OVR2, core);
	val |= 0x0007;
	WRITE_RADIO_REG_20696(pi, LPF_OVR2, core, val);
	/* Clear following LPF_REG7 bits:
	 * lpf_sw_bq1_bq2        0x0004
	 * lpf_sw_bq2_adc        0x0080
	 * lpf_sw_bq1_adc        0x0008
	 * lpf_sw_dac_bq2        0x0020
	 * lpf_sw_bq2_rc         0x0040
	 * lpf_sw_dac_rc         0x0010
	 * lpf_sw_aux_adc        0x0002
	 */
	val = READ_RADIO_REG_20696(pi, LPF_REG7, core);
	val &= 0xff01;
	WRITE_RADIO_REG_20696(pi, LPF_REG7, core, val);
}

static void
wlc_phy_radio20696_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout)
{
	uint8 normal = 1;
	uint16 adccapcal_Timeout;
	uint16 adccapcal_done;

	if (normal) {
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_start_cal_adc_I, 0x0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_start_cal_adc_Q, 0x0);

		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_reset_n_adc_I, 0x0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_reset_n_adc_Q, 0x0);
		OSL_DELAY(100);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_reset_n_adc_I, 0x1);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_reset_n_adc_Q, 0x1);
		OSL_DELAY(100);
		MOD_RADIO_REG_20696(pi, RXADC_CFG3, core,
			rxadc_coeff_out_ctrl_adc_I, 0x3);
		MOD_RADIO_REG_20696(pi, RXADC_CFG3, core,
			rxadc_coeff_out_ctrl_adc_Q, 0x3);
	} else {
		WRITE_RADIO_REG_20696(pi, RXADC_CFG1, core, 0x53ff);
		/* Reset ADC cal engine */
		WRITE_RADIO_REG_20696(pi, RXADC_CFG0, core, 0x830d);
	}
	/* Trigger CAL */
	if (normal) {
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_start_cal_adc_I, 0x1);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core,
			rxadc_start_cal_adc_Q, 0x1);
	} else {
		/* Reset ADC cal engine */
		WRITE_RADIO_REG_20696(pi, RXADC_CFG0, core, 0xfbfd);
	}

	/* 1000 * 10 = 10ms max time to wait */
	adccapcal_Timeout = 100;

	/* Test both I and Q rail done bit */
	adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_I);
	adccapcal_done &= READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_Q);
	while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
		OSL_DELAY(10);
		adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_I);
		adccapcal_done &= READ_RADIO_REGFLD_20696(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q);
		adccapcal_Timeout--;
	}
	if (adccapcal_Timeout == 0) {
		PHY_ERROR(("%s: ADC cap calibration TIMEOUT\n",
			__FUNCTION__));
	}
	*timeout = adccapcal_Timeout;
}

static void
wlc_phy_radio20696_adc_cap_cal_sequential(phy_info_t *pi, uint8 core, uint16 *timeout)
{
	uint16 adccapcal_Timeout;
	uint16 adccapcal_done;

	/* Sequential, one path at a time.
	 * Reset ADC cal engine.
	 */
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I,
		0x0);
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I,
		0x1);
	/* Start the calibration process for I */
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I,
		0x1);
	/* 100 * 10 = 1ms max time to wait */
	adccapcal_Timeout = 100;
	adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_I);
	while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
		OSL_DELAY(10);
		adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_I);
	        adccapcal_Timeout--;
	}
	if (adccapcal_Timeout == 0) {
		PHY_ERROR(("%s: ADC cap calibration TIMEOUT for Core%d "
			"ADC I\n", __FUNCTION__, core));
	}

	/* Reset ADC cal engine	*/
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q,
		0x0);
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q,
		0x1);
	/* Start the calibration process for Q */
	MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q,
		0x1);
	/* 100 * 10 = 1ms max time to wait */
	adccapcal_Timeout = 100;
	adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_Q);
	while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
		OSL_DELAY(10);
		adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q);
	        adccapcal_Timeout--;
	}
	if (adccapcal_Timeout == 0) {
		PHY_ERROR(("%s: ADC cap calibration TIMEOUT for Core%d ADC"
		" Q\n", __FUNCTION__, core));
	}
	*timeout = adccapcal_Timeout;
}

static void
wlc_phy_radio20696_apply_adc_cal_result(phy_info_t *pi, uint8 core)
{
	uint16 value;

	WRITE_RADIO_REG_20696(pi, AFE_CFG1_OVR1, core, 0x0FFF);

	value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP0_STAT, core, o_coeff_cap0_adc_I);
	MOD_RADIO_REG_20696(pi, RXADC_CFG6, core, rxadc_coeff_cap0_adc_I, value);
	PHY_INFORM(("core%d: cap0 %3d ", core, ((int16)(value << 10)) >> 10));

	value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP0_STAT, core, o_coeff_cap0_adc_Q);
	MOD_RADIO_REG_20696(pi, RXADC_CFG9, core, rxadc_coeff_cap0_adc_Q, value);
	PHY_INFORM(("%3d\n", ((int16)(value << 10)) >> 10));

	value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP1_STAT, core, o_coeff_cap1_adc_I);
	MOD_RADIO_REG_20696(pi, RXADC_CFG5, core, rxadc_coeff_cap1_adc_I, value);
	PHY_INFORM(("core%d: cap1 %3d ", core, ((int16)(value << 10)) >> 10));

	value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP1_STAT, core, o_coeff_cap1_adc_Q);
	MOD_RADIO_REG_20696(pi, RXADC_CFG8, core, rxadc_coeff_cap1_adc_Q, value);
	PHY_INFORM(("%3d\n", ((int16)(value << 10)) >> 10));

	value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP2_STAT, core, o_coeff_cap2_adc_I);
	MOD_RADIO_REG_20696(pi, RXADC_CFG5, core, rxadc_coeff_cap2_adc_I, value);
	PHY_INFORM(("core%d: cap2 %3d ", core, ((int16)(value << 10)) >> 10));

	value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP2_STAT, core, o_coeff_cap2_adc_Q);
	MOD_RADIO_REG_20696(pi, RXADC_CFG8, core, rxadc_coeff_cap2_adc_Q, value);
	PHY_INFORM(("%3d\n", ((int16)(value << 10)) >> 10));
}

void
wlc_phy_radio20696_afe_div_ratio(phy_info_t *pi)
{
	uint16 nad;
	uint16 nda;
	uint8 core;
	uint16 overrides;

	if (CHSPEC_IS160(pi->radio_chanspec)) {
		PHY_ERROR(("%s: 160MHz bandwidth is not supported on this device\n",
			__FUNCTION__));
		nda = 4;
		nad = 8;
	} else if (CHSPEC_IS80(pi->radio_chanspec)) {
		nda = 4;
		nad = 8;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		nda = 6;
		nad = 12;
	} else {
		nda = 12;
		nad = 24;
	}

	FOREACH_CORE(pi, core) {
		/* Set AFEDIV overrides:
			ovr_afediv_adc_div1
			ovr_afediv_adc_div2
			ovr_afediv_dac_div1
			ovr_afediv_dac_div2
		*/
		overrides = READ_RADIO_REG_20696(pi, AFEDIV_CFG1_OVR, core);
		overrides |= 0x0078;
		WRITE_RADIO_REG_20696(pi, AFEDIV_CFG1_OVR, core, overrides);
	}
	/* Set div2 to divide by two and div1 to nda/nad per phybw selected above */
	WRITE_RADIO_ALLREG_20696(pi, AFEDIV_REG1, (nda << 10) | (0x2 << 3) | (0x1));
	WRITE_RADIO_ALLREG_20696(pi, AFEDIV_REG2, (nad << 10) | (0x2 << 3) | (0x1));
}

static void
wlc_phy_radio2069_afecal_invert(phy_info_t *pi)
{
	uint8 core;
	uint16 calcode;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* Switch on the clk */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1);

	/* Output calCode = 1:14, latched = 15:28 */

	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		/* Use calCodes 1:14 instead of 15:28 */
		MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_iqadc_flash_calcode_Ich, 1);
		MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_iqadc_flash_calcode_Qch, 1);

		/* Invert the CalCodes */
		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE28, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE14(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE27, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE13(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE26, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE12(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE25, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE11(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE24, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE10(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE23, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE9(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE22, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE8(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE21, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE7(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE20, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE6(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE19, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE5(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE18, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE4(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE17, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE3(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE16, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE2(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE15, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE1(core), ~calcode & 0xffff);
	}

	/* Turn off the clk */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
}

#define MAX_2069_AFECAL_WAITLOOPS 10
#define SAVE_RESTORE_AFE_CFG_REGS 3
/* 3 registers per AFE core: RfctrlCoreAfeCfg1, RfctrlCoreAfeCfg2, RfctrlOverrideAfeCfg */
static void
wlc_phy_radio2069_afecal(phy_info_t *pi)
{
	uint8 core, done_i, done_q;
	uint16 adc_cfg4, afe_cfg_arr[SAVE_RESTORE_AFE_CFG_REGS*PHY_CORE_MAX] = {0};

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* Used to latch (clk register) rcal, rccal, ADC cal code */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1);

	/* Save config registers and issue reset */
	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		/* Cfg1  in 0 to PHYCORENUM-1 */
		afe_cfg_arr[core] = READ_PHYREGCE(pi, RfctrlCoreAfeCfg1, core);
		/* Cfg2 in PHYCORENUM to 2*PHYCORENUM -1 */
		afe_cfg_arr[core + PHYCORENUM(pi->pubpi->phy_corenum)] =
		  READ_PHYREGCE(pi, RfctrlCoreAfeCfg2, core);
		/* Overrides in 2*PHYCORENUM to 3*PHYCORENUM - 1 */
		afe_cfg_arr[core + 2*PHYCORENUM(pi->pubpi->phy_corenum)] =
		  READ_PHYREGCE(pi, RfctrlOverrideAfeCfg, core);
		MOD_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, afe_iqadc_reset, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_iqadc_reset, 1);
		MOD_RADIO_REGC(pi, ADC_CFG3, core, flash_calrstb, 0); /* reset */
	}

	OSL_DELAY(100);

	/* Bring each AFE core back from reset and perform cal */
	FOREACH_ACTV_CORE(pi, pi->sh->phyrxchain, core) {
		MOD_RADIO_REGC(pi, ADC_CFG3, core, flash_calrstb, 1);
		adc_cfg4 = READ_RADIO_REGC(pi, RF, ADC_CFG4, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CFG4(core), adc_cfg4 | 0xf);


		SPINWAIT((READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core),
			ADC_STATUS, i_wrf_jtag_afe_iqadc_Ich_cal_state) &&
			READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core), ADC_STATUS,
				i_wrf_jtag_afe_iqadc_Qch_cal_state)) == 0,
				ACPHY_SPINWAIT_AFE_CAL_STATUS);
		done_i = READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core), ADC_STATUS,
			i_wrf_jtag_afe_iqadc_Ich_cal_state);
		done_q = READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core), ADC_STATUS,
			i_wrf_jtag_afe_iqadc_Qch_cal_state);
		/* Don't assert for QT */
		if (!ISSIM_ENAB(pi->sh->sih)) {
			if (done_i == 0 || done_q == 0) {
				PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR:", __FUNCTION__));
				PHY_FATAL_ERROR_MESG(("AFECAL FAILED on Core %d\n", core));
				PHY_FATAL_ERROR(pi, PHY_RC_AFE_CAL_FAILED);
			}
		}

		/* calMode = 0 */
		phy_utils_write_radioreg(pi, RF_2069_ADC_CFG4(core), (adc_cfg4 & 0xfff0));
		/* Restore AFE config registers for that core with saved values */
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, afe_cfg_arr[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg2, core,
			afe_cfg_arr[core + PHYCORENUM(pi->pubpi->phy_corenum)]);
		WRITE_PHYREGCE(pi, RfctrlOverrideAfeCfg, core,
			afe_cfg_arr[core + 2 * PHYCORENUM(pi->pubpi->phy_corenum)]);
	}

	/* Turn off clock */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
	if (RADIO2069REV(pi->pubpi->radiorev) < 4) {
	  /* JIRA (CRDOT11ACPHY-153) calCodes are inverted for 4360a0 */
	  wlc_phy_radio2069_afecal_invert(pi);
	}
}

void
wlc_phy_radio_afecal(phy_info_t *pi)
{
	wlc_phy_resetcca_acphy(pi);
	OSL_DELAY(1);
	if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) {
		wlc_phy_radio20691_afecal(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		wlc_phy_radio20693_afecal(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		wlc_phy_radio20694_afecal(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		wlc_phy_radio20695_afecal(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20696_ID)) {
		wlc_phy_radio20696_afecal(pi);
	} else {
		wlc_phy_radio2069_afecal(pi);
	}

}

static void
wlc_phy_radio2069_4335C0_vco_opt(phy_info_t *pi, uint8 vco_mode)
{
	uint16 temp_reg;

	if (vco_mode == ACPHY_VCO_2P5V) {
		ACPHY_REG_LIST_START
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR27, 0xfff8)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO8, rfpll_vco_vctrl_buf_ical, 0x0)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO6, rfpll_vco_bypass_vctrl_buf, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO3, rfpll_vco_cvar_extra, 0xa)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO2, rfpll_vco_cvar, 0xf)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO6, rfpll_vco_bias_mode, 0x0)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO6, rfpll_vco_ALC_ref_ctrl, 0x0)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CP4, rfpll_cp_kpd_scale, 0x21)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_HVLDO2, ldo_2p5_lowquiescenten_VCO, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_HVLDO2, ldo_2p5_lowquiescenten_CP, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_CP, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_VCO, 0x1)
		ACPHY_REG_LIST_EXECUTE(pi);

		temp_reg = phy_utils_read_radioreg(pi, RFP_2069_GE16_PLL_CFG3);
		temp_reg |= (0x3 << 10);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_PLL_CFG3, temp_reg);

		temp_reg = phy_utils_read_radioreg(pi, RFP_2069_GE16_TOP_SPARE3);
		temp_reg |= (0x3 << 11);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_TOP_SPARE3, temp_reg);

	} else if (vco_mode == ACPHY_VCO_1P35V) {
		ACPHY_REG_LIST_START
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR27, 0xfff8)

			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO3, rfpll_vco_cvar_extra, 0xf)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO2, rfpll_vco_cvar, 0xf)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO6, rfpll_vco_bias_mode, 0x0)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_VCO6, rfpll_vco_ALC_ref_ctrl, 0x0)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CP4, rfpll_cp_kpd_scale, 0x21)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_lowquiescenten, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_HVLDO2, ldo_2p5_lowquiescenten_VCO, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_HVLDO2, ldo_2p5_lowquiescenten_CP, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_CP, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_VCO, 0x1)
		ACPHY_REG_LIST_EXECUTE(pi);

		temp_reg = phy_utils_read_radioreg(pi, RFP_2069_GE16_PLL_CFG4);
		temp_reg |= (0x3 << 12);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_PLL_CFG4, temp_reg);

		MOD_RADIO_REG(pi, RFP, PLL_VCO2, rfpll_vco_USE_2p5V, 0x0);

		temp_reg = phy_utils_read_radioreg(pi, RFP_2069_GE16_PLL_CFG3);
		temp_reg |= (0x3 << 10);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_PLL_CFG3, temp_reg);

		temp_reg = phy_utils_read_radioreg(pi, RFP_2069_GE16_TOP_SPARE3);
		temp_reg |= (0x3 << 11);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_TOP_SPARE3, temp_reg);

		ACPHY_REG_LIST_START
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_PLL_LF2, 0x5555)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_PLL_LF3, 0x5555)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_PLL_LF4, 0xe)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_PLL_LF5, 0xe)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_PLL_LF7, 0x1085)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

}

static void
wlc_phy_radio20691_4345_vco_opt(phy_info_t *pi, uint8 vco_mode)
{

	if (vco_mode == ACPHY_VCO_2P5V) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO3, 0, rfpll_vco_cvar_extra, 0xa)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO2, 0, rfpll_vco_cvar, 0xf)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO6, 0, rfpll_vco_bias_mode, 0x0)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO6, 0, rfpll_vco_ALC_ref_ctrl, 0x0)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_CP4, 0, rfpll_cp_kpd_scale, 0x21)
		ACPHY_REG_LIST_EXECUTE(pi);
			/* For 4364 1x1 core, this LP mode setting causes a delay */
			/* in VCO cal convergence when switching from 2G to 5G */
			if (CHIPID(pi->sh->chip) != BCM4364_CHIP_ID) {
				MOD_RADIO_REG_20691(pi, PLL_XTALLDO1, 0,
					ldo_1p2_xtalldo1p2_lowquiescenten, 0x1);
			}
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO2, 0,
				ldo_2p5_lowquiescenten_VCO, 0x1)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO2, 0, ldo_2p5_lowquiescenten_CP, 0x1)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO4, 0, ldo_2p5_static_load_CP, 0x1)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO4, 0, ldo_2p5_static_load_VCO, 0x1)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_CFG3, 0, rfpll_spare1, 0x3)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else if (vco_mode == ACPHY_VCO_1P35V) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO3, 0, rfpll_vco_cvar_extra, 0xf)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO2, 0, rfpll_vco_cvar, 0xf)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO6, 0, rfpll_vco_bias_mode, 0x0)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_VCO6, 0, rfpll_vco_ALC_ref_ctrl, 0x3)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_CP4, 0, rfpll_cp_kpd_scale, 0x21)
		ACPHY_REG_LIST_EXECUTE(pi);
			/* For 4364 1x1 core, this LP mode setting causes a delay */
			/* in VCO cal convergence when switching from 2G to 5G */
			if (CHIPID(pi->sh->chip) != BCM4364_CHIP_ID) {
				MOD_RADIO_REG_20691(pi, PLL_XTALLDO1, 0,
					ldo_1p2_xtalldo1p2_lowquiescenten, 0x1);
			}
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO2, 0,
				ldo_2p5_lowquiescenten_VCO, 0x1)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO2, 0, ldo_2p5_lowquiescenten_CP, 0x1)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO4, 0, ldo_2p5_static_load_CP, 0x1)
			MOD_RADIO_REG_20691_ENTRY(pi, PLL_HVLDO4, 0, ldo_2p5_static_load_VCO, 0x1)

			MOD_RADIO_REG_20691_ENTRY(pi, PLL_CFG3, 0, rfpll_spare0, 0xb4)

			MOD_RADIO_REG_20691_ENTRY(pi, PLL_CFG3, 0, rfpll_spare1, 0x3)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

void
wlc_phy_radio_vco_opt(phy_info_t *pi, uint8 vco_mode)
{
	if (ACMAJORREV_1(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
		wlc_phy_radio2069_4335C0_vco_opt(pi, vco_mode);
	} else if (ACMAJORREV_3(pi->pubpi->phy_rev)) {
		wlc_phy_radio20691_4345_vco_opt(pi, vco_mode);
	} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
	}
}
