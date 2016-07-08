/*
 * ACPHY RADIO contorl module implementation
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_radio.h>
#include "phy_type_radio.h"
#include <phy_ac.h>
#include <phy_ac_radio.h>
#include <phy_utils_reg.h>
#include <wlc_phy_ac.h>
#include <wlc_phy_radio.h>
#include <wlc_radioreg_20691.h>
#include <wlc_phytbl_ac.h>
#include <wlc_phytbl_20691.h>
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20696.h>
#include <wlc_phyreg_ac.h>
#include <sbchipc.h>
#include <wlc_phytbl_20693.h>
#include <wlc_phytbl_20696.h>
#include <phy_ac_pllconfig_20696.h>

#ifdef BCM7271
#include <wl_bcm7xxx_dbg.h>
#undef PHY_ERROR
#define PHY_ERROR(args) do {BCM7XXX_TRACE(); Bcm7xxxPrintSpace(NULL); printf args;} while (0)
#endif /* BCM7271 */


/* module private states */
struct phy_ac_radio_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_radio_info_t *ri;
	acphy_pmu_core1_off_radregs_t *pmu_c1_off_info_orig;

/* add other variable size variables here at the end */
};

/* local functions */
static void phy_ac_radio_switch(phy_type_radio_ctx_t *ctx, bool on);
static void phy_ac_radio_on(phy_type_radio_ctx_t *ctx);
#if defined(BCMDBG_PHYDUMP)
static int phy_ac_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_ac_radio_dump NULL
#endif 

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
#define RADIO20693_NUM_RFFE_TUNE_REGS 6
static const uint8 rffe_tune_reg_map_20693[RADIO20693_NUM_RFFE_TUNE_REGS] = {
	RADIO20693_TUNEREG_TYPE_2G,
	RADIO20693_TUNEREG_TYPE_5G,
	RADIO20693_TUNEREG_TYPE_NONE,
	RADIO20693_TUNEREG_TYPE_NONE,
	RADIO20693_TUNEREG_TYPE_NONE,
	RADIO20693_TUNEREG_TYPE_2G
};

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
static void wlc_phy_radio20696_rccal(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy_20691(phy_info_t *pi);
static void wlc_phy_radio2069_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20691_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20696_upd_band_related_reg(phy_info_t *pi);
static void wlc_phy_radio20696_rffe_tune(phy_info_t *pi,
	const chan_info_radio20696_rffe_t *chan_info_rffe);

#if defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD)
	/* Don't download PHY table if ucode is not downloaded */
#else /* !defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD) */
static void WLBANDINITFN(wlc_phy_static_table_download_acphy)(phy_info_t *pi);
#endif /* defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD) */

static void wlc_phy_radio_tiny_rcal(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio_tiny_rcal_wave2(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio_tiny_rccal(phy_info_t *pi);
static void wlc_phy_radio_tiny_rccal_wave2(phy_info_t *pi);
void wlc_phy_logen_reset(phy_info_t *pi, uint8 core);

static void wlc_phy_radio20696_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_radio20696_minipmu_pwron_seq(phy_info_t *pi);
static void wlc_phy_radio20696_r_cal(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio20696_minipmu_cal(phy_info_t *pi);
static void wlc_phy_radio20696_pmu_pll_pwrup(phy_info_t *pi);
static void wlc_phy_radio20696_wars(phy_info_t *pi);
static void wlc_phy_radio20696_refclk_en(phy_info_t *pi);
static void wlc_phy_radio20696_upd_prfd_values(phy_info_t *pi);

/* Register/unregister ACPHY specific implementation to common layer. */
phy_ac_radio_info_t *
BCMATTACHFN(phy_ac_radio_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_radio_info_t *ri)
{
	phy_ac_radio_info_t *info;
	acphy_pmu_core1_off_radregs_t *pmu_c1_off_info_orig = NULL;
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

	info->pi = pi;
	info->aci = aci;
	info->ri = ri;
	info->pmu_c1_off_info_orig = pmu_c1_off_info_orig;

	/* Populate 20696 PLL configuration table */
	if (RADIOID_IS(pi->pubpi.radioid, BCM20696_ID)) {
		if (phy_ac_radio20696_populate_pll_config_tbl(pi) != BCME_OK) {
			PHY_ERROR(("%s: phy_ac_radio20696_populate_pll_config_tbl failed\n",
					__FUNCTION__));
			goto fail;
		}
	}

#ifndef BCM_OL_DEV
	/* make sure the radio is off until we do an "up" */
	phy_ac_radio_switch(info, OFF);
#endif

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctrl = phy_ac_radio_switch;
	fns.on = phy_ac_radio_on;
	fns.dump = phy_ac_radio_dump;
	fns.ctx = info;

	phy_radio_register_impl(ri, &fns);

	return info;

fail:
	if (pmu_c1_off_info_orig != NULL)
		phy_mfree(pi, pmu_c1_off_info_orig, sizeof(acphy_pmu_core1_off_radregs_t));

	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ac_radio_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_radio_unregister_impl)(phy_ac_radio_info_t *info)
{
	phy_info_t *pi;
	phy_radio_info_t *ri;

	ASSERT(info);
	pi = info->pi;
	ri = info->ri;

	phy_radio_unregister_impl(ri);

	phy_mfree(pi, info->pmu_c1_off_info_orig, sizeof(acphy_pmu_core1_off_radregs_t));

	phy_mfree(pi, info, sizeof(phy_ac_radio_info_t));
}

/* switch radio on/off */
static void
phy_ac_radio_switch(phy_type_radio_ctx_t *ctx, bool on)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;

#if defined(BCM7271_EMU)
	BCM7XXX_ERR(("%s(%d)*****HACKHACK***** Force radio to one for emulation only\n",
		__FUNCTION__, __LINE__));
	pi->radio_is_on = on;
	return;
#endif /* defined(BCM7271_EMU) */

	/* sync up soft_radio_state with hard_radio_state */
	pi->radio_is_on = FALSE;
	PHY_TRACE(("wl%d: %s %d\n", pi->sh->unit, __FUNCTION__, on));

	wlc_phy_switch_radio_acphy(pi, on);

}

/* turn radio on */
static void
WLBANDINITFN(phy_ac_radio_on)(phy_type_radio_ctx_t *ctx)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_ac_info_t *aci = info->aci;
	phy_info_t *pi = aci->pi;

	aci->init_done = FALSE; /* To make sure that chanspec_set_acphy */
				/* doesn't get called twice during init */

	/* update phy_corenum and phy_coremask */
	if (ACMAJORREV_4(pi->pubpi.phy_rev))
		phy_ac_update_phycorestate(pi);

	phy_ac_radio_switch(ctx, ON);
}

/* query radio idcode */
uint32
phy_ac_radio_query_idcode(phy_info_t *pi)
{
	uint32 b0, b1;

#ifdef BCM72717
	BCM7XXX_ENTER();
#endif /* BCM72717 */


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

#ifdef BCM7271
	BCM7XXX_DBG(("%s(%d) idcode id=0x%08x rev=0x%08x\n", __FUNCTION__, __LINE__, b0, b1));
#endif/* BCM7271 */

	/* For ACPHY (starting with 4360A0), address 0 has the revid and
	   address 1 has the devid
	*/
#ifdef BCM72717
	BCM7XXX_EXIT();
#endif /* BCM72717 */

	return (b0 << 16) | b1;
}

/* parse radio idcode and write to pi->pubpi */
void
phy_ac_radio_parse_idcode(phy_info_t *pi, uint32 idcode)
{
#ifdef BCM72717
	BCM7XXX_ENTER();
#endif /* BCM72717 */

	PHY_TRACE(("%s: idcode 0x%08x\n", __FUNCTION__, idcode));

	pi->pubpi.radioid = (idcode & IDCODE_ACPHY_ID_MASK) >> IDCODE_ACPHY_ID_SHIFT;
	pi->pubpi.radiorev = (idcode & IDCODE_ACPHY_REV_MASK) >> IDCODE_ACPHY_REV_SHIFT;

	/* ACPHYs do not use radio ver. This param is invalid */
	pi->pubpi.radiover = 0;

#ifdef BCM72717
	BCM7XXX_EXIT();
#endif /* BCM72717 */
}

#if defined(BCMDBG_PHYDUMP)
static int
phy_ac_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_ac_radio_info_t *gi = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = gi->pi;
	const char *name = NULL;
	int i, jtag_core;
	uint16 addr = 0;
	radio_20xx_dumpregs_t *radio20xxdumpregs = NULL;

	if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {
		name = "20693";
		if ((RADIOMAJORREV(pi) == 1) || (RADIOMAJORREV(pi) == 2)) {
			uint16 phymode = phy_get_phymode(pi);
			if (phymode == PHYMODE_RSDB)
				radio20xxdumpregs = dumpregs_20693_rsdb;
			else if (phymode == PHYMODE_MIMO)
				radio20xxdumpregs = dumpregs_20693_mimo;
			else if (phymode == PHYMODE_80P80)
				radio20xxdumpregs = dumpregs_20693_80p80;
			else
				ASSERT(0);
		} else if (RADIOMAJORREV(pi) == 3) {
			radio20xxdumpregs = dumpregs_20693_rev32;
		} else {
			return BCME_NOTFOUND;
		}
	} else if (RADIOID(pi->pubpi.radioid) == BCM2069_ID) {
		name = "2069";
		if (RADIOMAJORREV(pi) == 2) {
			radio20xxdumpregs = dumpregs_2069_rev32;
		} else if ((RADIOREV(pi->pubpi.radiorev) == 16) ||
			(RADIOREV(pi->pubpi.radiorev) == 18)) {
			radio20xxdumpregs = dumpregs_2069_rev16;
		} else if (RADIOREV(pi->pubpi.radiorev) == 17) {
			radio20xxdumpregs = dumpregs_2069_rev17;
		} else if (RADIOREV(pi->pubpi.radiorev) == 25) {
			radio20xxdumpregs = dumpregs_2069_rev25;
		} else {
			radio20xxdumpregs = dumpregs_2069_rev0;
		}
	} else if (RADIOID(pi->pubpi.radioid) == BCM20691_ID) {
		name = "20691";
		switch (RADIOREV(pi->pubpi.radiorev)) {
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
		default:
			;
		}
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
	if (PHYCORENUM(pi->pubpi.phy_corenum) > 1)
		bcm_bprintf(b, "0 Value1 ...");
	bcm_bprintf(b, "\n");

	while ((addr = radio20xxdumpregs[i].address) != 0xffff) {

		if (RADIOID(pi->pubpi.radioid) == BCM2069_ID) {
			jtag_core = (addr & JTAG_2069_MASK);
			addr &= (~JTAG_2069_MASK);
		} else
			jtag_core = 0;

		if ((RADIOID(pi->pubpi.radioid) == BCM2069_ID) &&
			(jtag_core == JTAG_2069_ALL)) {
			switch (PHYCORENUM(pi->pubpi.phy_corenum)) {
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
			if ((RADIOID(pi->pubpi.radioid) == BCM20693_ID) &&
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
#endif 


/* 20693 radio related functions */
int
wlc_phy_chan2freq_20693(phy_info_t *pi, uint8 channel,
	const chan_info_radio20693_pll_t **chan_info_pll,
        const chan_info_radio20693_rffe_t **chan_info_rffe,
	const chan_info_radio20693_pll_wave2_t **chan_info_pll_wave2)
{
	uint  i;
	uint16 freq;
	chan_info_radio20693_pll_t *chan_info_tbl_pll = NULL;
	chan_info_radio20693_rffe_t *chan_info_tbl_rffe = NULL;
	chan_info_radio20693_pll_wave2_t *chan_info_tbl_pll_wave2_part1 = NULL;
	chan_info_radio20693_pll_wave2_t *chan_info_tbl_pll_wave2_part2 = NULL;
	uint32 tbl_len, tbl_len1 = 0;

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi.radiorev)) {
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
		/* A2 tuning tables */
		chan_info_tbl_pll = chan_tuning_20693_rev13_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev13_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev13_pll);
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		/* Update this once the actual tuning settings are available */
		chan_info_tbl_pll = chan_tuning_20693_rev5_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev5_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev5_pll);
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
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi.radiorev)));
		ASSERT(FALSE);
		return -1;
	}

	for (i = 0; i < tbl_len; i++) {
		if (RADIOMAJORREV(pi) < 3) {
			if (chan_info_tbl_pll[i].chan == channel) {
				*chan_info_pll = &chan_info_tbl_pll[i];
				*chan_info_rffe = &chan_info_tbl_rffe[i];
				freq = chan_info_tbl_pll[i].freq;
				break;
			}
		} else {
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
	}

	if (i >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
		           pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
			ASSERT(i < tbl_len);
			//PHY_ERROR(("wl%d: %s: ASSERTION: i < tbl_len\n",
			//			pi->sh->unit, __FUNCTION__));
		}
		return -1;
	}

	return freq;
}

void
wlc_phy_chanspec_radio20693_setup(phy_info_t *pi, uint8 ch,
		uint8 toggle_logen_reset, uint8 pllcore, uint8 mode)
{
	const chan_info_radio20693_pll_t *chan_info_pll;
	const chan_info_radio20693_rffe_t *chan_info_rffe;
	const chan_info_radio20693_pll_wave2_t *chan_info_pll_wave2;
	int fc[NUM_CHANS_IN_CHAN_BONDING];
	uint8 core, start_pllcore = 0, end_pllcore = 0;
	uint8 chans[NUM_CHANS_IN_CHAN_BONDING] = {0, 0};

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	PHY_INFORM(("wl%d: %s pllcore %d, mode %d\n", pi->sh->unit,
			__FUNCTION__, pllcore, mode));

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
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
		if (toggle_logen_reset == 1) {
			wlc_phy_logen_reset(pi, core);
		}

		if (RADIOMAJORREV(pi) < 3) {

			/* pll tuning */
			wlc_phy_radio20693_pll_tune(pi, chan_info_pll, core);

			/* rffe tuning */
			wlc_phy_radio20693_rffe_tune(pi, chan_info_rffe, core);

		} else {
			/* pll tuning */
			wlc_phy_radio20693_pll_tune_wave2(pi, chan_info_pll_wave2, core);
		}
	}

	if (RADIOMAJORREV(pi) >= 3) {
			wlc_phy_radio20693_sel_logen_mode(pi);
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

		if (CHSPEC_IS20(pi->radio_chanspec) &&
				!PHY_AS_80P80(pi, pi->radio_chanspec)) {
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
		} else if (CHSPEC_IS40(pi->radio_chanspec) || CHSPEC_IS80(pi->radio_chanspec)) {
			if (fc[pllcore] >= 5745 && fc[pllcore] <= 5825) {
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


#ifdef BCM7271
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
	}
	else {
		MOD_RADIO_PLLREG_20696(pi, LOGEN_REG0, logen_div3en, 0);
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_div2_pu, 0x0);
			// new 5G optimised settings
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG4, core, logen_tx_rccr_tune, 0);
			MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 127);
			MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_Rfb, 0);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_5g_tx_rccr_iqbias_short, 1);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG2, core, logen_tx_rccr_idac26u, 7);
			// RX settings RCCR related 5G
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG4, core, logen_rx_rccr_tune, 0);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG2, core, logen_rx_rccr_idac26u, 7);
			MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG0, core, logen_5g_rx_rccr_iqbias_short, 1);
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


int
wlc_phy_chan2freq_20696(phy_info_t *pi, uint8 channel,
	const chan_info_radio20696_rffe_t **chan_info)
{
	uint32 index, tbl_len;
	const chan_info_radio20696_rffe_t *p_chan_info_tbl;

	if (!RADIOID_IS(pi->pubpi.radioid, BCM20696_ID)) {
		PHY_ERROR(("wl%d: %s: Unsupported radio %d\n",
			pi->sh->unit, __FUNCTION__, RADIOID(pi->pubpi.radioid)));
		ASSERT(FALSE);
		return -1;
	}
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


void
wlc_phy_chanspec_radio20696_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset)
{
	const chan_info_radio20696_rffe_t *chan_info;
    
	ASSERT(RADIOID_IS(pi->pubpi.radioid, BCM20696_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20696(pi, ch, &chan_info) < 0)
		return;
        
//	printf("Reset LOGEN\n");
	// logen_reset needs to be toggled whenever bandsel bit if changed 
	// On a bw change, phy_reset is issued which causes currentBand getting reset to 0 
	// So, issue this on both band & bw change  
	MOD_PHYREG(pi, RfctrlCoreTxPus0, logen_reset, 0x1);
	MOD_PHYREG(pi, RfctrlCoreTxPus1, logen_reset, 0x1);
	MOD_PHYREG(pi, RfctrlCoreTxPus2, logen_reset, 0x1);
	MOD_PHYREG(pi, RfctrlCoreTxPus3, logen_reset, 0x1);
	OSL_DELAY(100);
	MOD_PHYREG(pi, RfctrlCoreTxPus0, logen_reset, 0x0);
	MOD_PHYREG(pi, RfctrlCoreTxPus1, logen_reset, 0x0);
	MOD_PHYREG(pi, RfctrlCoreTxPus2, logen_reset, 0x0);
	MOD_PHYREG(pi, RfctrlCoreTxPus3, logen_reset, 0x0);

	WRITE_RADIO_PLLREG_20696(pi, PLL_VCOCAL_OVR1, 0xFF);

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
#endif /* BCM7271 */


void
wlc_phy_radio20693_afeclkpath_setup(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode, uint8 direct_ctrl_en)
{
	uint8 pupd;
	uint16 mac_mode;
	pupd = (direct_ctrl_en == 1) ? 0 : 1;
	mac_mode = phy_get_phymode(pi);

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);
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

void
wlc_phy_radio20693_adc_powerupdown(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 pupdbit, uint8 core)
{
	uint8 en_ovrride;

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);
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
	int row = wlc_phy_radio20693_altclkpln_get_chan_row(pi);
	uint8 afeclkdiv;
	is_per_core_phy_bw80 = (CHSPEC_IS80(pi->radio_chanspec) ||
		CHSPEC_IS8080(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
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
			#ifndef WLTXBF_DISABLED
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_pu, 0);
			#else
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_pu, pupd);
			#endif
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
			#ifndef WLTXBF_DISABLED
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_pu, 0);
			#else
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core,
				afe_clk_div_12g_mimo_pu, pupd);
			#endif
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
		RADIO_PLLREG_20693(pi, WL_XTAL_CFG3,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL18,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL3,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL4,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL7,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL8,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL20,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL1,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL12,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL13,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL10,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL11,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL19,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL6,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL9,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL17,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL15,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL2,         core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL24,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL26,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL25,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL21,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL22,        core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL23,        core),
		RADIO_PLLREG_20693(pi, PLL_VCO7,            core),
		RADIO_PLLREG_20693(pi, PLL_VCO2,            core),
		RADIO_PLLREG_20693(pi, PLL_FRCT2,           core),
		RADIO_PLLREG_20693(pi, PLL_FRCT3,           core),
		RADIO_PLLREG_20693(pi, PLL_VCO6,            core),
		RADIO_PLLREG_20693(pi, PLL_VCO5,            core),
		RADIO_PLLREG_20693(pi, PLL_VCO4,            core),
		RADIO_PLLREG_20693(pi, PLL_LF4,             core),
		RADIO_PLLREG_20693(pi, PLL_LF5,             core),
		RADIO_PLLREG_20693(pi, PLL_LF7,             core),
		RADIO_PLLREG_20693(pi, PLL_LF2,             core),
		RADIO_PLLREG_20693(pi, PLL_LF3,             core),
		RADIO_PLLREG_20693(pi, PLL_CP4,             core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL5,         core),
		RADIO_PLLREG_20693(pi, LO2G_LOGEN0_DRV,     core),
		RADIO_PLLREG_20693(pi, LO2G_VCO_DRV_CFG2,   core),
		RADIO_PLLREG_20693(pi, LO2G_LOGEN1_DRV,     core),
		RADIO_PLLREG_20693(pi, LO5G_CORE0_CFG1,     core),
		RADIO_PLLREG_20693(pi, LO5G_CORE1_CFG1,     core),
		RADIO_PLLREG_20693(pi, LO5G_CORE2_CFG1,     core),
		RADIO_PLLREG_20693(pi, LO5G_CORE3_CFG1,     core),
	};

	uint8 ctbase = ARRAYSIZE(pll_tune_reg_offsets_20693);

	PHY_TRACE(("wl%d: %s\n pll_core: %d Channel: %d\n", pi->sh->unit, __FUNCTION__,
		core, ci20693_pll->chan));

	off_ptr = &pll_tune_reg_offsets_20693[0];
	val_ptr = &ci20693_pll->wl_xtal_cfg3;

	for (ct = 0; ct < ARRAYSIZE(pll_tune_reg_offsets_20693); ct++) {
		phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
	}

	if (ACMAJORREV_33(pi->pubpi.phy_rev) &&
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
		RADIO_REG_20693(pi, SPARE_CFG6, 0),
		RADIO_REG_20693(pi, SPARE_CFG7, 0),
		RADIO_REG_20693(pi, SPARE_CFG4, 0),
		RADIO_REG_20693(pi, PA2G_CFG2,  0)
	};
	uint16 rffe_tune_reg_offsets_20693_cr1[RADIO20693_NUM_RFFE_TUNE_REGS] = {
		RADIO_REG_20693(pi, LNA2G_TUNE, 1),
		RADIO_REG_20693(pi, LNA5G_TUNE, 1),
		RADIO_REG_20693(pi, SPARE_CFG6, 1),
		RADIO_REG_20693(pi, SPARE_CFG7, 1),
		RADIO_REG_20693(pi, SPARE_CFG4, 1),
		RADIO_REG_20693(pi, PA2G_CFG2,  1)
	};

	PHY_TRACE(("wl%d: %s core: %d crisscross_actv: %d\n", pi->sh->unit, __FUNCTION__, core,
		pi_ac->is_crisscross_actv));

	val_ptr = &ci20693_rffe->lna2g_tune;
	if ((pi_ac->is_crisscross_actv == 0) || (phymode == PHYMODE_MIMO)) {
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
	if (ACMAJORREV_4(pi->pubpi.phy_rev) && (!(PHY_IPA(pi))) &&
		(RADIOREV(pi->pubpi.radiorev) == 13) && CHSPEC_IS2G(pi->radio_chanspec)) {
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
		if (core != pi_ac->crisscross_priority_core_80p80) {
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
			if (core != pi_ac->crisscross_priority_core_rsdb) {
				PHY_TRACE(("wl%d: %s IGNORE: priority is not met\n",
					pi->sh->unit, __FUNCTION__));

				PHY_TRACE(("wl%d: %s current_core: %d Priority: %d\n", pi->sh->unit,
					__FUNCTION__, core, pi_ac->crisscross_priority_core_rsdb));

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

static void wlc_phy_radio20693_pmu_override(phy_info_t *pi)
{
	uint8  core;
	uint16 data;

	if (RADIOMAJORREV(pi) < 3) {
		return;
	}

	FOREACH_CORE(pi, core) {
		data = READ_RADIO_REG_20693(pi, PMU_OVR, core);
		data = data | 0xFC;
		phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OVR, core), data);
	}
}

static void
wlc_phy_radio20693_xtal_pwrup(phy_info_t *pi)
{
	// 4365 does not have RSDB support, hence no addtional xtal pwrup
	// of core1 needed & core0 pwrup is taken care of by pmu sequencer
	if (RADIOMAJORREV(pi) == 3) {
		return;
	}

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);
	/* Note: Core0 XTAL will be powerup by PMUcore in chipcommon */
	if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		((phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1))) {
		/* Powerup xtal ldo for MAC_Core = 1 in case of RSDB mode */
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_vref_bias_reset, 1);
		MOD_RADIO_REG_20693(pi, PLL_XTAL_OVR1, 0,
			ovr_ldo_1p2_xtalldo1p2_vref_bias_reset, 1);
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_BG_pu, 1);
		MOD_RADIO_REG_20693(pi, PLL_XTAL_OVR1, 0, ovr_ldo_1p2_xtalldo1p2_BG_pu, 1);
		OSL_DELAY(300);
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_vref_bias_reset, 0);
	} else if ((phy_get_phymode(pi) != PHYMODE_RSDB)) {
		/* Powerup xtal ldo for Core 1 in case of MIMO and 80+80 */
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_vref_bias_reset, 1);
		MOD_RADIO_REG_20693(pi, PLL_XTAL_OVR1, 1,
			ovr_ldo_1p2_xtalldo1p2_vref_bias_reset, 1);
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_BG_pu, 1);
		MOD_RADIO_REG_20693(pi, PLL_XTAL_OVR1, 1, ovr_ldo_1p2_xtalldo1p2_BG_pu, 1);
		OSL_DELAY(300);
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_vref_bias_reset, 0);
	}
}

static void
wlc_phy_radio20693_upd_prfd_values(phy_info_t *pi)
{
	uint i = 0;
	radio_20xx_prefregs_t *prefregs_20693_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi.radiorev)) {
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
		/* Update this once the actual tuning settings are available */
		prefregs_20693_ptr = prefregs_20693_rev13;
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		/* Update this once the actual tuning settings are available */
		prefregs_20693_ptr = prefregs_20693_rev5;
		break;
	case 32:
	case 33:
		prefregs_20693_ptr = prefregs_20693_rev32;
		break;
	case 9:
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi.radiorev)));
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

int8
wlc_phy_tiny_radio_minipmu_cal(phy_info_t *pi)
{
	uint8 calsuccesful = 1, core;
	int8 cal_status = 1;
	int16 waitcounter = 0;
	phy_info_t *pi_core0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
	ASSERT(TINY_RADIO(pi));
	/* Skip this function for QT */
	if (ISSIM_ENAB(pi->sh->sih))
		return 0;
	/* Setup the cal */
	if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_VCOldo_pu, 0);
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_AFEldo_pu, 0);
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_RXldo_pu, 0);
			MOD_RADIO_REG_TINY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 0);
		}
	}

	FOREACH_CORE(pi, core) {
		if (RADIOID(pi->pubpi.radioid) == BCM20691_ID) {
			MOD_RADIO_REG_TINY(pi, PMU_CFG3, core, wlpmu_selavg, 2);
		} else if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {
			MOD_RADIO_REG_TINY(pi, PMU_CFG3, core, wlpmu_selavg, 0);
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
		if ((RADIOID(pi->pubpi.radioid) == BCM20693_ID) &&
			(phy_get_phymode(pi) == PHYMODE_RSDB) &&
			(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1)) {
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
		waitcounter = 0;
		calsuccesful = 1;
		while (READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_done) == 0) {
			OSL_DELAY(100);
			waitcounter ++;
			if (waitcounter > 100) {
				/* cal_done bit is not 1 even after waiting for a while */
				/* Exit gracefully */
				PHY_TRACE(("\nWarning:Mini PMU Cal Failed on Core%d \n", core));
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
	}
	FOREACH_CORE(pi, core) {
		if ((RADIOID(pi->pubpi.radioid) == BCM20693_ID) &&
			(phy_get_phymode(pi) == PHYMODE_RSDB) &&
			(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1)) {
			MOD_RADIO_REG_TINY(pi_core0, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
		} else {
			MOD_RADIO_REG_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
		}
	}

	if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {
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
	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);

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
wlc_phy_radio20693_pmu_pll_config_wave2(phy_info_t *pi, uint8 pll_mode)
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
	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);

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
	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);

	/* minipmu_cal */
	wlc_phy_tiny_radio_minipmu_cal(pi);

	/* r cal */
	if (RADIOID(pi->pubpi.radioid) == BCM20693_ID && RADIOMAJORREV(pi) == 3) {
	  wlc_phy_radio_tiny_rcal_wave2(pi, 2);
	}

	/* power up pmu pll */
	if (RADIOMAJORREV(pi) < 3) {
		wlc_phy_radio20693_pmu_pll_config(pi);
	} else {
		if (phy_get_phymode(pi) == PHYMODE_80P80 ||
				PHY_AS_80P80(pi, pi->radio_chanspec)) {
				/* for 4365 C0 - turn on both PLL */
			wlc_phy_radio20693_pmu_pll_config_wave2(pi, 2);
		} else {
			wlc_phy_radio20693_pmu_pll_config_wave2(pi, 0);
		}
	}

	if (RADIOMAJORREV(pi) < 3) {
	  /* RCAL */
	  wlc_phy_radio_tiny_rcal(pi, 1);
	}
}

static void
wlc_phy_switch_radio_acphy(phy_info_t *pi, bool on)
{
	uint8 core = 0, pll = 0;
	uint16 data = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
#ifdef LOW_TX_CURRENT_SETTINGS_2G
	uint8 is_ipa = 0;
#endif

#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */

	PHY_TRACE(("wl%d: %s %s corenum %d\n", pi->sh->unit, __FUNCTION__, on ? "ON" : "OFF",
		pi->pubpi.phy_corenum));

	if (on) {
		if (!pi->radio_is_on) {
			if (RADIOID(pi->pubpi.radioid) == BCM2069_ID) {
				if (RADIOMAJORREV(pi) == 1) {
					/*  set the low pwoer reg before radio init */
					wlc_phy_set_lowpwr_phy_reg(pi);
					wlc_phy_radio2069_mini_pwron_seq_rev16(pi);
				} else if (RADIOMAJORREV(pi) == 2) {
					/*  set the low pwoer reg before radio init */
					wlc_phy_set_lowpwr_phy_reg_rev3(pi);
					wlc_phy_radio2069_mini_pwron_seq_rev32(pi);
				}

				wlc_phy_radio2069_pwron_seq(pi);
			} else if (RADIOID(pi->pubpi.radioid) == BCM20691_ID) {
				wlc_phy_tiny_radio_pwron_seq(pi);
				wlc_phy_switch_radio_acphy_20691(pi);
			} else if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {

				/* ------------------------------------------------------------- */
				/* In case of 4349A0, there will be 2 XTALs. Core0 XTAL will be  */
				/* Pwrdup by PMU and Core1 XTAL needs to be pwrdup accordingly   */
				/* in all the modes like RSDB, MIMO and 80P80                    */
				/* ------------------------------------------------------------- */
				wlc_phy_radio20693_xtal_pwrup(pi);

				wlc_phy_tiny_radio_pwron_seq(pi);

				// 4365: Enable the PMU overrides because we don't want the powerup
				// sequence to be controlled by the PMU sequencer
				wlc_phy_radio20693_pmu_override(pi);

				/* Power up the radio mini PMU Seq */
				wlc_phy_radio20693_minipmu_pwron_seq(pi);

				/* pll config, minipmu cal, RCAL */
				wlc_phy_switch_radio_acphy_20693(pi);
			} else if (RADIOID(pi->pubpi.radioid) == BCM20696_ID) {
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

			if (!TINY_RADIO(pi) && (RADIOID(pi->pubpi.radioid) != BCM20696_ID)) {

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
					/* For 7271 BFL3_RCAL_OTP_VAL_EN should be set */
					ASSERT(RADIOID(pi->pubpi.radioid) != BCM20696_ID);

					if (BF3_RCAL_WAR(pi_ac) == 1) {
						if (RADIOMAJORREV(pi) == 2) {
							data = ACPHY_RCAL_VAL_2X2;
						} else if (RADIOMAJORREV(pi) == 1) {
							data = ACPHY_RCAL_VAL_1X1;
						}
					} else if (RADIOID(pi->pubpi.radioid) == BCM2069_ID) {
							wlc_phy_radio2069_rcal(pi);
					}
				}

				if (BF3_RCAL_WAR(pi_ac) == 1 ||
					BF3_RCAL_OTP_VAL_EN(pi_ac) == 1) {
					if (RADIOMAJORREV(pi) == 2) {
						FOREACH_CORE(pi, core) {
							MOD_RADIO_REGC(pi, GE32_BG_CFG1, core,
								rcal_trim, data);
							MOD_RADIO_REGC(pi, GE32_OVR2, core,
								ovr_bg_rcal_trim, 1);
							MOD_RADIO_REGC(pi, GE32_OVR2, core,
								ovr_otp_rcal_sel, 0);
						}
					} else if (RADIOMAJORREV(pi) == 1) {
						MOD_RADIO_REG(pi, RFP, GE16_BG_CFG1,
							rcal_trim, data);
						MOD_RADIO_REG(pi, RFP, GE16_OVR2,
							ovr_bg_rcal_trim, 1);
						MOD_RADIO_REG(pi, RFP, GE16_OVR2,
							ovr_otp_rcal_sel, 0);
					}
				}
			}

			if (TINY_RADIO(pi)) {
				if (RADIOID(pi->pubpi.radioid) == BCM20693_ID &&
				    RADIOMAJORREV(pi) == 3) {
					wlc_phy_radio_tiny_rccal_wave2(pi);
				} else {
					wlc_phy_radio_tiny_rccal(pi);
				}
			} else if (RADIOID(pi->pubpi.radioid) == BCM2069_ID) {
				wlc_phy_radio2069_rccal(pi);
			} 
			if (pi_ac->init_done) {
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

		if (!ACMAJORREV_4(pi->pubpi.phy_rev)) {
			/* AFE */
			ACPHYREG_BCAST(pi, RfctrlCoreAfeCfg10, 0);
			ACPHYREG_BCAST(pi, RfctrlCoreAfeCfg20, 0);
			ACPHYREG_BCAST(pi, RfctrlOverrideAfeCfg0, 0x1fff);

			/* Radio RX */
			ACPHYREG_BCAST(pi, RfctrlCoreRxPus0, 0);
			ACPHYREG_BCAST(pi, RfctrlOverrideRxPus0, 0xffff);
		}

		/* Radio TX */
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

		/* {radio, rfpll, pllldo}_pu = 0 */
		MOD_PHYREG(pi, RfctrlCmd, chip_pu, 0);

		/* SW4345-58 */
		if (RADIOID(pi->pubpi.radioid) == BCM20691_ID) {
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
		if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {

			if (RADIOMAJORREV(pi) < 3) {
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

		/* Turn off the mini PMU enable on all cores when going down.
		 * Will be powered up in the UP sequence
		 */
		if (RADIOID(pi->pubpi.radioid) == BCM2069_ID) {
			if (RADIOMAJORREV(pi) == 2) {
				FOREACH_CORE(pi, core)
				        MOD_RADIO_REGC(pi, GE32_PMU_OP, core, wlpmu_en, 0);
			}
		}

		WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
		WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0x1);

		if (CHIPID(pi->sh->chip) == BCM4335_CHIP_ID) {
			/* WAR for XTAL power up issues */
			MOD_RADIO_REG(pi, RFP, GE16_OVR16, ovr_xtal_pu_corebuf_pfd, 1);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_pfddrv, 0);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_corebuf_pfd, 0);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_BT, 0);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 0);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_caldrv, 0);
		}
	}

	if ((CHIPID(pi->sh->chip) == BCM4335_CHIP_ID &&
	     !CST4335_CHIPMODE_USB20D(pi->sh->sih->chipst)) ||
	    (BCM4350_CHIP(pi->sh->chip) &&
	     !CST4350_CHIPMODE_HSIC20D(pi->sh->sih->chipst))) {
		/* Power down HSIC */
		ASSERT(RADIOID(pi->pubpi.radioid) == BCM2069_ID);
		if (RADIOMAJORREV(pi) > 0) {
			MOD_RADIO_REG(pi, RFP,  GE16_PLL_XTAL2, xtal_pu_HSIC, 0x0);
		}
	} else if ((CHIPID(pi->sh->chip) == BCM4345_CHIP_ID &&
	           !CST4345_CHIPMODE_USB20D(pi->sh->sih->chipst))) {
		/* Power down HSIC */
		ASSERT(RADIOID(pi->pubpi.radioid) == BCM20691_ID);
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

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM2069_ID);
	if ((RADIOMAJORREV(pi) == 1) && (is_ipa == 0)) {
		PHY_TRACE(("Modifying PA Bias settings for lower power for 2G!\n"));
		MOD_RADIO_REG(pi, RFX, PA2G_IDAC2, pa2g_biasa_main, 0x36);
		MOD_RADIO_REG(pi, RFX, PA2G_IDAC2, pa2g_biasa_aux, 0x36);
	}

#endif /* LOW_TX_CURRENT_SETTINGS_2G */

#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

static void
wlc_phy_radio2069_mini_pwron_seq_rev16(phy_info_t *pi)
{
	uint8 cntr = 0;

	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, wlpmu_en, 1);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, VCOldo_pu, 1);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, TXldo_pu, 1);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, AFEldo_pu, 1);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, RXldo_pu, 1);

	OSL_DELAY(100);

	/* WAR for XTAL power up issues */
	MOD_RADIO_REG(pi, RFP, GE16_OVR16, ovr_xtal_pu_corebuf_pfd, 0);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_pfddrv, 1);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_BT, 1);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_caldrv, 1);

	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, synth_pwrsw_en, 1);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, wlpmu_ldobg_clk_en, 1);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, ldoref_start_cal, 1);

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

	phy_utils_write_radioreg(pi, RFX_2069_GE32_PMU_OP, 0xbe);

	phy_utils_write_radioreg(pi, RFX_2069_GE32_PMU_OP, 0x20be);
	phy_utils_write_radioreg(pi, RFX_2069_GE32_PMU_OP, 0x60be);

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

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

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
	if (RADIOID(pi->pubpi.radioid) == BCM20693_ID && RADIOMAJORREV(pi) == 3) {
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);
	} else {
		WRITE_PHYREG(pi, RfctrlCoreTxPus0, 0);
		WRITE_PHYREG(pi, RfctrlOverrideTxPus0, 0x3ff);
	}

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl,   0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl,    0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/* Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/* # Reset radio, jtag */
	/* NOTE: por_force doesnt have any effect in 4349A0 and */
	/* radio jtag reset is taken care by the PMU */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	/* # radio_reset to be high for 100us */
	OSL_DELAY(100);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/* Update preferred values (not required for uCode PM) */
	if (RADIOID(pi->pubpi.radioid) == BCM20691_ID) {
		wlc_phy_radio20691_upd_prfd_values(pi);
	} else if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {
		wlc_phy_radio20693_upd_prfd_values(pi);
	}

	/* # {rfpll, pllldo, logen}_{pu, reset} */
	/* # pllldo_{pu, reset} = 1, rfpll_reset = 1 */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	/* # {radio, rfpll}_pwrup = 1 */
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	/* # logen_{pwrup, reset} = 1 */
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(100); /* # resets to be on for 100us */
	/* # {pllldo, rfpll}_reset = 0 */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	/* # logen_reset = 0 */
	if (RADIOID(pi->pubpi.radioid) == BCM20693_ID && RADIOMAJORREV(pi) == 3) {
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
	} else {
		WRITE_PHYREG(pi, RfctrlCoreTxPus0, 0x80);
		/* # leave overrides for logen_{pwrup, reset} */
		WRITE_PHYREG(pi, RfctrlOverrideTxPus0, 0x180);
	}
}

#define MAX_2069_RCAL_WAITLOOPS 100
/* rcal takes ~50us */
static void
wlc_phy_radio2069_rcal(phy_info_t *pi)
{
	uint8 done, rcal_val, core;
	uint16 rcal_itr;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	/* Power-up rcal clock (need both of them for rcal) */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 1);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1);

	/* Rcal can run with 40mhz cls, no diving */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL, 0);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL1, 0);

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM2069_ID);

	/* Make connection with the external 10k resistor */
	/* Turn off all test points in cgpaio block to avoid conflict */
	if (RADIOMAJORREV(pi) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_CGPAIO_CFG1, core, cgpaio_pu, 1);
		}
		phy_utils_write_radioreg(pi, RFX_2069_GE32_CGPAIO_CFG2, 0);
		phy_utils_write_radioreg(pi, RFX_2069_GE32_CGPAIO_CFG3, 0);
		phy_utils_write_radioreg(pi, RFX_2069_GE32_CGPAIO_CFG4, 0);
		phy_utils_write_radioreg(pi, RFX_2069_GE32_CGPAIO_CFG5, 0);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_TOP_SPARE1, 0);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_TOP_SPARE2, 0);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_TOP_SPARE4, 0);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_TOP_SPARE6, 0);
	} else {
		MOD_RADIO_REG(pi, RF2, CGPAIO_CFG1, cgpaio_pu, 1);
		phy_utils_write_radioreg(pi, RF2_2069_CGPAIO_CFG2, 0);
		phy_utils_write_radioreg(pi, RF2_2069_CGPAIO_CFG3, 0);
		phy_utils_write_radioreg(pi, RF2_2069_CGPAIO_CFG4, 0);
		phy_utils_write_radioreg(pi, RF2_2069_CGPAIO_CFG5, 0);
	}
	/* NOTE: xtal_pu, xtal_buf_pu & xtalldo_pu direct control lines should be(& are) ON */

	/* Toggle the rcal pu for calibration engine */
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 0);
	OSL_DELAY(1);
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 1);

	/* Wait for rcal to be done, max = 10us * 100 = 1ms  */
	done = 0;
	for (rcal_itr = 0; rcal_itr < MAX_2069_RCAL_WAITLOOPS; rcal_itr++) {
		OSL_DELAY(10);
		done = READ_RADIO_REGFLD(pi, RF2, RCAL_CFG, i_wrf_jtag_rcal_valid);
		if (done == 1) {
			break;
		}
	}

	ASSERT(done & 0x1);

	/* Status */
	rcal_val = READ_RADIO_REGFLD(pi, RF2, RCAL_CFG, i_wrf_jtag_rcal_value);
	rcal_val = rcal_val >> 1;
	PHY_INFORM(("wl%d: %s rcal=%d\n", pi->sh->unit, __FUNCTION__, rcal_val));

	/* Valid range of values for rcal */
	ASSERT((rcal_val > 0) && (rcal_val < 15));

	/*  Power down blocks not needed anymore */
	if (RADIOMAJORREV(pi) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_CGPAIO_CFG1, core, cgpaio_pu, 0);
		}
	} else {
		MOD_RADIO_REG(pi, RF2, CGPAIO_CFG1, cgpaio_pu, 0);
	}

	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 0);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 0);
}

#define MAX_2069_RCCAL_WAITLOOPS 100
#define NUM_2069_RCCAL_CAPS 3
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

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM2069_ID);

	if (RADIOMAJORREV(pi) > 0) {
		if (pi->xtalfreq == 40000000) {
		  if ((RADIOREV(pi->pubpi.radiorev) == 25) ||
		                      (RADIOREV(pi->pubpi.radiorev) == 26)) {
			gmult_const = 70;
			trc[0] = 0x294;
			trc[1] = 0x202;
			trc[2] = 0x214;
		  } else {
			gmult_const = 160;
		  }
		} else if (pi->xtalfreq == 37400000) {
		  if ((RADIOREV(pi->pubpi.radiorev) == 25) ||
		                      (RADIOREV(pi->pubpi.radiorev) == 26)) {
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
		} else if (pi->xtalfreq == 52000000) {
			if ((RADIOREV(pi->pubpi.radiorev) == 25) ||
				(RADIOREV(pi->pubpi.radiorev) == 26)) {
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
				if (RADIOMAJORREV(pi) > 0) {
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
			pi_ac->rccal_gmult = rccal_val[cal];
			pi_ac->rccal_gmult_rc = pi_ac->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
		} else if (cal == 1) {
			/* adc */
			rccal_val[cal] = READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC5, rccal_raw_adc1p2);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));

			/* don't change this loop to active core loop,
			   gives slightly higher floor, why?
			*/
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_4_0, rccal_val[cal]);
				MOD_RADIO_REGC(pi, TIA_CFG3, core, rccal_hpc, rccal_val[cal]);
			}
		} else {
			/* dacbuf */
			rccal_val[cal] = READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC5, rccal_raw_dacbuf);
			pi_ac->rccal_dacbuf = rccal_val[cal];

			/* take away the override on dacbuf fixed cap */
			FOREACH_CORE(pi, core) {
				if (RADIOMAJORREV(pi) > 0) {
					MOD_RADIO_REGC(pi, GE16_OVR22, core,
					               ovr_afe_DACbuf_fixed_cap, 0);
				} else {
					MOD_RADIO_REGC(pi, OVR21, core,
					               ovr_afe_DACbuf_fixed_cap, 0);
				}
			}
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
		}

		/* Turn off rccal */
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, pu, 0);
	}

	/* Powerdown rccal driver */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
}


static void
wlc_phy_radio20696_rccal(phy_info_t *pi)
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

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint32 dn;

	pi_ac->rccal_dacbuf = 4096;
	pi_ac->rccal_gmult  = 4260;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}

	//Kai: looks like TCL skips powering up RCCAL clock since:
	/*
	# Powerup the RCCal clock
	#set which_mac_core [acphy_which_mac]
	#if {$which_mac_core == 1} {
	#    setcore d11b; #switch to core0 for xtal
	    #set reg(RFP0_xtal5.xtal_pu_RCCAL) 0x1	; # Powerup RCCAL driver  
	#    setcore d11a; #switch back to core1
	#} else {
	    #set reg(RFP0_xtal5.xtal_pu_RCCAL) 0x1	; # Powerup RCCAL driver 
	#}
	*/

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	//MOD_RADIO_PLLREG_20696(pi, XTAL5, xtal_pu_wlandig, 1);
	//MOD_RADIO_PLLREG_20696(pi, XTAL5, xtal_pu_RCCAL, 1);

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

		n0 = READ_RADIO_REGFLD_20696(pi, RF, RCCAL_CFG5, 1, rccal_N0);
		n1 = READ_RADIO_REGFLD_20696(pi, RF, RCCAL_CFG6, 1, rccal_N1);
		dn = n1 - n0; /* set dn [expr {$N1 - $N0}] */
		//Kai: line below in trunk - does not look right - and does not work see labnotebook037
		//dn = 1;

		if (cal == 0) {
			/* lpf  values */
			pi_ac->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
			pi_ac->rccal_gmult_rc = pi_ac->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, pi_ac->rccal_gmult));
			printf("RCCAL - LPF rccal_lpf_gmult %d\n",  pi_ac->rccal_gmult);
		} else if (cal == 1) {
			/* dacbuf  */
			gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
			pi_ac->rccal_dacbuf = (1 << 24) / (gmult_dacbuf);
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi_ac->rccal_dacbuf));
			printf("RCCAL - DACbuf rccal_dacbuf_cmult %d\n",  pi_ac->rccal_dacbuf);
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20696(pi, RCCAL_CFG0, 1, rccal_pu, 0);

	}
	/* Powerdown rccal driver */
	//MOD_RADIO_REG_20696(pi, XTAL5, xtal_pu_RCCAL, 0);

	/* nominal values when rc cal failes  */
	if (done == 0) {
		pi_ac->rccal_dacbuf = 4096;
		pi_ac->rccal_gmult  = 4260;
	}
	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20696(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, pi_ac->rccal_gmult_rc);
		MOD_RADIO_REG_20696(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, pi_ac->rccal_gmult_rc);
		MOD_RADIO_REG_20696(pi, LPF_GMULT_RC, core, lpf_gmult_rc, pi_ac->rccal_gmult_rc);
	}

}


static void
wlc_phy_switch_radio_acphy_20691(phy_info_t *pi)
{

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20691_ID);

	/* 20691_mini_pwron_seq_phyregs_rev12() */
	/* # mini PMU init */
	/* #Step 1 */
	MOD_RADIO_REG_20691(pi, PMU_OP, 0, wlpmu_en, 1);
	MOD_RADIO_REG_20691(pi, PMU_OP, 0, wlpmu_VCOldo_pu, 1);
	MOD_RADIO_REG_20691(pi, PMU_OP, 0, wlpmu_TXldo_pu, 1);
	MOD_RADIO_REG_20691(pi, PMU_OP, 0, wlpmu_AFEldo_pu, 1);
	MOD_RADIO_REG_20691(pi, PMU_OP, 0, wlpmu_RXldo_pu, 1);
	/* #Step 2 */
	OSL_DELAY(200);

	/* # DAC */
	MOD_RADIO_REG_20691(pi, CLK_DIV_CFG1, 0, dac_driver_size, 8);
	MOD_RADIO_REG_20691(pi, CLK_DIV_CFG1, 0, sel_dac_div, 3);
	MOD_RADIO_REG_20691(pi, TX_DAC_CFG5, 0, DAC_invclk, 1);
	MOD_RADIO_REG_20691(pi, TX_DAC_CFG5, 0, DAC_pd_mode, 0);
	/* # improve settling behavior, need for level tracking */
	MOD_RADIO_REG_20691(pi, BG_CFG1, 0, bg_pulse, 1);
	MOD_RADIO_REG_20691(pi, TX_DAC_CFG1, 0, DAC_scram_off, 1);

	/* # ADC */
	MOD_RADIO_REG_20691(pi, PMU_CFG4, 0, wlpmu_ADCldo_pu, 1);

	/* 20691_pmu_pll_pwrup */
	MOD_RADIO_REG_20691(pi, VREG_CFG, 0, vreg25_pu, 1); /* #powerup vreg2p5 */
	MOD_RADIO_REG_20691(pi, BG_CFG1, 0, bg_pu, 1); /* #powerup bandgap */

	/* # powerup pll */
	/* # VCO */
	MOD_RADIO_REG_20691(pi, PLL_CFG1, 0, rfpll_vco_pu, 1);
	/* # Enable override */
	MOD_RADIO_REG_20691(pi, RFPLL_OVR1, 0, ovr_rfpll_vco_pu, 1);
	/* # VCO LDO */
	MOD_RADIO_REG_20691(pi, PLL_HVLDO1, 0, ldo_2p5_pu_ldo_VCO, 1);
	/* # Enable override */
	MOD_RADIO_REG_20691(pi, RFPLL_OVR1, 0, ovr_ldo_2p5_pu_ldo_VCO, 1);
	/* # VCO buf */
	MOD_RADIO_REG_20691(pi, PLL_CFG1, 0, rfpll_vco_buf_pu, 1);
	/* # Enable override */
	MOD_RADIO_REG_20691(pi, RFPLL_OVR1, 0, ovr_rfpll_vco_buf_pu, 1);
	/* # Synth global */
	MOD_RADIO_REG_20691(pi, PLL_CFG1, 0, rfpll_synth_pu, 1);
	/* # Enable override */
	MOD_RADIO_REG_20691(pi, RFPLL_OVR1, 0, ovr_rfpll_synth_pu, 1);
	/* # Charge Pump */
	MOD_RADIO_REG_20691(pi, PLL_CP1, 0, rfpll_cp_pu, 1);
	/* # Enable override */
	MOD_RADIO_REG_20691(pi, RFPLL_OVR1, 0, ovr_rfpll_cp_pu, 1);
	/* # Charge Pump LDO */
	MOD_RADIO_REG_20691(pi, PLL_HVLDO1, 0, ldo_2p5_pu_ldo_CP, 1);
	/* # Enable override */
	MOD_RADIO_REG_20691(pi, RFPLL_OVR1, 0, ovr_ldo_2p5_pu_ldo_CP, 1);
	/* # PLL lock monitor */
	MOD_RADIO_REG_20691(pi, PLL_CFG1, 0, rfpll_monitor_pu, 1);

	/* Nishant's hack to get PLL lock: */
	/* spare 127 (bit 15) is xtal_dcc_clk_sel */
	/* actually controls the inrush current limit on xgtal ldo */
	MOD_RADIO_REG_20691(pi, PLL_XTAL12, 0, xtal_dcc_clk_sel, 1);

	/* minipmu_cal */
	wlc_phy_tiny_radio_minipmu_cal(pi);

	/* r cal */
	if (RADIOID(pi->pubpi.radioid) == BCM20693_ID && RADIOMAJORREV(pi) == 3)
		wlc_phy_radio_tiny_rcal_wave2(pi, 2);
	else
		wlc_phy_radio_tiny_rcal(pi, 1);
}

static void
wlc_phy_radio2069_upd_prfd_values(phy_info_t *pi)
{
	uint8 core;
	radio_20xx_prefregs_t *prefregs_2069_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM2069_ID);

	switch (RADIOREV(pi->pubpi.radiorev)) {
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
		prefregs_2069_ptr = prefregs_2069_rev39;
		break;
	case 36:
		prefregs_2069_ptr = prefregs_2069_rev36;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi.radiorev)));
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
	ASSERT(RADIOID(pi->pubpi.radioid) == BCM2069_ID);
	if (RADIOMAJORREV(pi) == 0) {
		MOD_RADIO_REG(pi, RFP, OVR15, ovr_rfpll_rst_n, 1);
		MOD_RADIO_REG(pi, RFP, OVR15, ovr_rfpll_en_vcocal, 1);
		MOD_RADIO_REG(pi, RFP, OVR16, ovr_rfpll_vcocal_rstn, 1);

		MOD_RADIO_REG(pi, RFP, OVR15, ovr_rfpll_cal_rst_n, 1);
		MOD_RADIO_REG(pi, RFP, OVR15, ovr_rfpll_pll_pu, 1);
		MOD_RADIO_REG(pi, RFP, OVR15, ovr_rfpll_vcocal_cal, 1);
	} else {
		if (RADIOMAJORREV(pi) == 1) {
			MOD_RADIO_REG(pi, RFP, GE16_OVR16, ovr_rfpll_en_vcocal, 1);
			MOD_RADIO_REG(pi, RFP, GE16_OVR17, ovr_rfpll_vcocal_rstn, 1);
			MOD_RADIO_REG(pi, RFP, GE16_OVR16, ovr_rfpll_rst_n, 1);

			MOD_RADIO_REG(pi, RFP, GE16_OVR16, ovr_rfpll_cal_rst_n, 1);
			MOD_RADIO_REG(pi, RFP, GE16_OVR16, ovr_rfpll_pll_pu, 1);
			MOD_RADIO_REG(pi, RFP, GE16_OVR16, ovr_rfpll_vcocal_cal, 1);
		}

		/* Until this is moved to XLS (give bg_filter_en control to radio) */
		if (RADIOMAJORREV(pi) == 2) {
			MOD_RADIO_REG(pi, RFP, GE32_OVR1, ovr_vreg_bg_filter_en, 1);
		}

		/* Ensure that we read the values that are actually applied */
		/* to the radio block and not just the radio register values */

		phy_utils_write_radioreg(pi, RFX_2069_GE16_READOVERRIDES, 1);
		MOD_RADIO_REG(pi, RFP, GE16_READOVERRIDES, read_overrides, 1);

		if (RADIOMAJORREV(pi) == 2) {
			/* For VREG power up in radio jtag as there is a bug in the digital
			 * connection
			 */
			if (RADIOMINORREV(pi) < 5) {
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
	if (RADIOMAJORREV(pi) < 2) {
		MOD_RADIO_REG(pi, RF2, BG_CFG1, bg_pulse, 1);
	}
	if (RADIOMAJORREV(pi) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_BG_CFG1, core, bg_pulse, 1);
			MOD_RADIO_REGC(pi, GE32_OVR2, core, ovr_bg_pulse, 1);
		}
	}

	/* Give control of bg_filter_en to radio */
	if (RADIOMAJORREV(pi) < 2)
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
	if ((RADIOMAJORREV(pi) == 0) &&
		((RADIOREV(pi->pubpi.radiorev) == 7) || (RADIOREV(pi->pubpi.radiorev) == 8))) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, RXRF5G_CFG1, core, pu_pulse, 1);
			MOD_RADIO_REGC(pi, OVR18, core, ovr_rxrf5g_pu_pulse, 1);
		}
	}
}

static void
wlc_phy_radio20691_upd_prfd_values(phy_info_t *pi)
{
	radio_20xx_prefregs_t *prefregs_20691_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20691_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi.radiorev)) {
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
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi.radiorev)));
		ASSERT(FALSE);
		return;
	}

	/* Update preferred values */
	wlc_phy_init_radio_prefregs_allbands(pi, prefregs_20691_ptr);

	MOD_RADIO_REG_20691(pi, LOGEN_OVR1, 0, ovr_logencore_5g_en_lowband, 0x1);
	MOD_RADIO_REG_20691(pi, LOGEN_CFG2, 0, logencore_5g_en_lowband, 0x0);

	if (!PHY_IPA(pi)) {
		/* Saeed's settings for aband epa evm tuning */
		MOD_RADIO_REG_20691(pi, TXMIX5G_CFG1, 0, mx5g_idac_bleed_bias, 0x1f);
		MOD_RADIO_REG_20691(pi, TXMIX5G_CFG5, 0, mx5g_idac_bbdc, 0x0);
		MOD_RADIO_REG_20691(pi, TXMIX5G_CFG5, 0, mx5g_idac_lodc, 0xf);
		MOD_RADIO_REG_20691(pi, TXMIX5G_CFG8, 0, pad5g_idac_gm, 0xa);
		MOD_RADIO_REG_20691(pi, PA5G_IDAC1, 0, pa5g_idac_main, 0x17);
		MOD_RADIO_REG_20691(pi, PA5G_IDAC3, 0, pa5g_idac_tuning_bias, 0xf);

		/* Utku's second setting for gband epa evm tuning */
		MOD_RADIO_REG_20691(pi, PA2G_IDAC2, 0, pad2g_idac_tuning_bias, 6);
		MOD_RADIO_REG_20691(pi, PA2G_INCAP, 0, pa2g_idac_incap_compen_main, 0x2c);
		MOD_RADIO_REG_20691(pi, PA2G_IDAC1, 0, pa2g_idac_main, 0x24);
		MOD_RADIO_REG_20691(pi, PA2G_IDAC1, 0, pa2g_idac_cas, 0x2b);
		MOD_RADIO_REG_20691(pi, TXMIX2G_CFG2, 0, mx2g_idac_cascode, 0xc);
		MOD_RADIO_REG_20691(pi, TX_LOGEN2G_CFG1, 0, logen2g_tx_xover, 4);
		MOD_RADIO_REG_20691(pi, LOGEN_CFG1, 0, logencore_lcbuf_stg1_bias, 0);
		MOD_RADIO_REG_20691(pi, LOGEN_CFG2, 0, logencore_lcbuf_stg2_bias, 7);
		MOD_RADIO_REG_20691(pi, LOGEN_CFG1, 0, logencore_lcbuf_stg1_ftune, 0x18);

		/* A band EVM floor tuning, dfu, cfraser */
		MOD_RADIO_REG_20691(pi, TX_TOP_2G_OVR_EAST, 0, ovr_tx2g_bias_pu, 1);
		MOD_RADIO_REG_20691(pi, TX2G_CFG1, 0, tx2g_bias_pu, 1);
		MOD_RADIO_REG_20691(pi, TX_LPF_CFG3, 0, lpf_sel_2g_5g_cmref_gm, 0);
		MOD_RADIO_REG_20691(pi, TXMIX5G_CFG3, 0, mx5g_pu_bleed, 0);
		MOD_RADIO_REG_20691(pi, TXMIX5G_CFG8, 0, pad5g_idac_gm, 0x8);
		MOD_RADIO_REG_20691(pi, PA5G_INCAP, 0, pad5g_idac_pmos, 0x18);
		MOD_RADIO_REG_20691(pi, PA5G_IDAC1, 0, pa5g_idac_main, 0x1a);
		MOD_RADIO_REG_20691(pi, TXGM5G_CFG1, 0, pad5g_idac_cascode, 0xf);
	}

	if ((RADIOMAJORREV(pi) == 1) && PHY_IPA(pi)) {
		MOD_RADIO_REG_20691(pi, PA2G_INCAP, 0, pa2g_idac_incap_compen_main, 0x0);
		MOD_RADIO_REG_20691(pi, PA2G_IDAC1, 0, pa2g_idac_cas, 0x20);
		MOD_RADIO_REG_20691(pi, PA5G_IDAC1, 0, pa5g_idac_main, 0x18);
	}
}

#if defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD)
	/* Don't download PHY table if ucode is not downloaded */
#else /* !defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD) */
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
#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */
	ACPHY_DISABLE_STALL(pi);
	if (pi->phy_init_por) {
		/* these tables are not affected by phy reset, only power down */
		if (ACMAJORREV_32(pi->pubpi.phy_rev) || ACMAJORREV_33(pi->pubpi.phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev32; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev32[idx]);
			}
			if (ACMAJORREV_33(pi->pubpi.phy_rev)) {
				for (idx = 0; idx < acphytbl_info_sz_rev33; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_rev33[idx]);
				}
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev32,
				acphyzerotbl_info_cnt_rev32);
		} else if (ACMAJORREV_5(pi->pubpi.phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev9; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev9[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev9,
				acphyzerotbl_info_cnt_rev9);
		} else if (ACMAJORREV_2(pi->pubpi.phy_rev)) {
			if (ACREV_IS(pi->pubpi.phy_rev, 9)) { /* 43602a0 */
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
		} else if (ACMAJORREV_1(pi->pubpi.phy_rev)) {
			if (ACREV_IS(pi->pubpi.phy_rev, 6))	{
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
		} else if (ACMAJORREV_0(pi->pubpi.phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev0; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev0[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev0,
				acphyzerotbl_info_cnt_rev0);
		} else if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
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

#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}
#endif /* defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD) */

/*  R Calibration (takes ~50us) */
static void
wlc_phy_radio_tiny_rcal(phy_info_t *pi, uint8 mode)
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

	if ((mode == 2) && (phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE0)) {
		/* This should run only for core 0 */
		/* Run R Cal and use its output */
		MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL1, 0x1);
		MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0x1);

		/* Make connection with the external 10k resistor */
		/* Also powers up rcal (RF_rcal_cfg.rcal_pu = 1) */
		MOD_RADIO_REG_TINY(pi, GPAIO_SEL2, 0, gpaio_pu, 1);
		MOD_RADIO_REG_TINY(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
		MOD_RADIO_REG_TINY(pi, GPAIO_SEL0, 0, gpaio_sel_0to15_port, 0);
		MOD_RADIO_REG_TINY(pi, GPAIO_SEL1, 0, gpaio_sel_16to31_port, 0);
		MOD_RADIO_REG_TINY(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH, 0, rcal_valid);
			loop_iter ++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH, 0, rcal_value) >> 1;

			/* Use the output of the rcal engine */
			MOD_RADIO_REG_TINY(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
			if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {
				/* Copy the rca. value instead of latching */
				MOD_RADIO_REG_TINY(pi, BG_OVR1, 0,
					ovr_bg_rcal_trim,  1);
				MOD_RADIO_REG_TINY(pi, BG_CFG1, 0,
					bg_rcal_trim,  rcal_value);
			}
			else { /* Use latched output of rcal engine */
				MOD_RADIO_REG_TINY(pi, BG_OVR1, 0,
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
		MOD_RADIO_REG_TINY(pi, GPAIO_SEL2, 0, gpaio_pu, 0);
		MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL1, 0);
		MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL,  0);
		MOD_RADIO_REG_TINY(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
		FOREACH_CORE(pi, core) {
			if (((phy_get_phymode(pi) == PHYMODE_MIMO) ||
				(phy_get_phymode(pi) == PHYMODE_80P80)) &&
				(core != 0)) {
				rcal_value = READ_RADIO_REGFLD_TINY(pi, BG_CFG1,
					0, bg_rcal_trim);
				MOD_RADIO_REG_TINY(pi, BG_CFG1, core,
					bg_rcal_trim,  rcal_value);
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core,
					ovr_bg_rcal_trim,  1);
			}
		}
	} else {
		FOREACH_CORE(pi, core) {
			if (mode == 0) {
				/* Use OTP stored rcal value */
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 1);
			} else if (mode == 1) {
				if (RADIOID(pi->pubpi.radioid) == BCM20693_ID) {
					/* Default RCal code to be used with mode 1 is 8 */
					MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_rcal_trim, 8);
				} else {
					/* Default RCal code to be used with mode 1 is 11 */
					MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_rcal_trim, 11);
				}
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
			} else if ((mode == 2) && (phy_get_phymode(pi) == PHYMODE_RSDB)) {
				/* for RSDB mode copy rcal value from pi of core 0 */
				phy_info_t *pi_core0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
				rcal_value = READ_RADIO_REGFLD_TINY(pi_core0, BG_CFG1,
					0, bg_rcal_trim);
				MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_rcal_trim,  rcal_value);
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_bg_rcal_trim,  1);
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
		MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x33);

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
wlc_phy_radio_tiny_rccal(phy_info_t *pi)
{
	uint8 cal, done;
	uint16 rccal_itr, n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x1, 0x0};
	uint8 sc[] = {0x0, 0x1, 0x2};
	uint8 x1[] = {0x1c, 0x70, 0x40};
	uint16 trc[] = {0x22d, 0xf0, 0x10a};
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint32 dn;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));
	/* For RSDB core 1, copy core 0 values to core 1 for rc cal and return */
	if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(phy_get_current_core(pi) != PHY_RSBD_PI_IDX_CORE0)) {
		phy_info_t *pi_rsdb0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
		pi_ac->rccal_gmult = pi_rsdb0->u.pi_acphy->rccal_gmult;
		pi_ac->rccal_gmult_rc = pi_rsdb0->u.pi_acphy->rccal_gmult_rc;
		pi_ac->rccal_adc_gmult = pi_rsdb0->u.pi_acphy->rccal_adc_gmult;
		pi_ac->rccal_dacbuf = pi_rsdb0->u.pi_acphy->rccal_dacbuf;
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
			pi_ac->rccal_gmult = (101541 * dn) / (pi->xtalfreq >> 12);
			pi_ac->rccal_gmult_rc = pi_ac->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, pi_ac->rccal_gmult));
		} else if (cal == 1) {
			/* adc */
			/* set k [expr {$is_adc ? 102051 : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			pi_ac->rccal_adc_gmult = (102051 * dn) / (pi->xtalfreq >> 12);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
			            __FUNCTION__, pi_ac->rccal_adc_gmult));
		} else {
			/* dacbuf */
			pi_ac->rccal_dacbuf = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG6, 0,
			                                              rccal_raw_dacbuf);
			MOD_RADIO_REG_TINY(pi, TX_BB_OVR1, 0, ovr_DACbuf_fixed_cap, 0);
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, pi_ac->rccal_dacbuf));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_pu, 0);
	}
	/* Powerdown rccal driver */
	MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0);
}


static void
wlc_phy_radio_tiny_rccal_wave2(phy_info_t *pi)
{
	uint8 cal, done;
	uint16 rccal_itr, n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x1, 0x0};
	uint8 sc[] = {0x0, 0x2, 0x1};
	uint8 x1[] = {0x1c, 0x70, 0x68};
	uint16 trc[] = {0x22d, 0xf0, 0x134};
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
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
			pi_ac->rccal_gmult = (101541 * dn) / (pi->xtalfreq >> 12);
			pi_ac->rccal_gmult_rc = pi_ac->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, pi_ac->rccal_gmult));
		} else if (cal == 1) {
			/* adc */
			/* set k [expr {$is_adc ? (($def(d11_radio_major_rev) == 3)? */
			/* 85000 : 102051) : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			pi_ac->rccal_adc_gmult = (85000 * dn) / (pi->xtalfreq >> 12);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
			            __FUNCTION__, pi_ac->rccal_adc_gmult));
		} else {
			/* dacbuf */
			pi_ac->rccal_dacbuf = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG6, 0,
			                                              rccal_raw_dacbuf);
			MOD_RADIO_ALLREG_20693(pi, TX_BB_OVR1, ovr_DACbuf_fixed_cap, 0);
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, pi_ac->rccal_dacbuf));
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
void
wlc_phy_set_regtbl_on_pwron_acphy(phy_info_t *pi)
{
	uint8 core;
	uint16 val;
	bool flag2rangeon =
		((CHSPEC_IS2G(pi->radio_chanspec) && pi->u.pi_acphy->srom_tworangetssi2g) ||
		(CHSPEC_IS5G(pi->radio_chanspec) && pi->u.pi_acphy->srom_tworangetssi5g)) &&
	        PHY_IPA(pi);
	uint16 rfseq_bundle_48[3];


#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */
	/* force afediv(core 0, 1, 2) always high */
	if (!(ACMAJORREV_2(pi->pubpi.phy_rev) || ACMAJORREV_5(pi->pubpi.phy_rev)))
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x77); // Luka: TCL doesn't require this

	if (ACMAJORREV_1(pi->pubpi.phy_rev)) {
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrlN0, 0x3);
	}
	/* Remove RFCTRL signal overrides for all cores */
	ACPHYREG_BCAST(pi, RfctrlIntc0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideAfeCfg0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideGains0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideLpfCT0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideLpfSwtch0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideAfeCfg0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideAuxTssi0, 0);
	ACPHYREG_BCAST(pi, AfectrlOverride0, 0);
	if ((ACMAJORREV_2(pi->pubpi.phy_rev) && !ACMINORREV_0(pi)) ||
	    ACMAJORREV_5(pi->pubpi.phy_rev)) {
		ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0);
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0);
	}

	/* logen_pwrup = 1, logen_reset = 0 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);

	if (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi)) {
		// luka
		//MOD_PHYREG(pi, Core0cliploGainCodeA, clip1loLnaIndex, 1);
		//more override clear for 4347 28nm radio
		//NEDIM_7271: follow 4347
		//KAI: TCL has the following register set to 0x0
		ACPHYREG_BCAST(pi, RfctrlOverrideAfeDivCfg0, 0x8f0b);
		ACPHYREG_BCAST(pi, RfctrlOverrideETCfg0, 0x0);
		ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrl28nm0, 0x0);
		ACPHYREG_BCAST(pi, LogenOverrideCtrl28nm0, 0x0);
		ACPHYREG_BCAST(pi, RfCtrlCorePwrSw0, 0x0);
		ACPHYREG_BCAST(pi, dyn_radiob0, 0x0);
			
		//set temp_reg $phyreg(Dac_gain$core)
		//set phyreg(Dac_gain$core) [hexpr $temp_reg | 0x000F];
		MOD_PHYREG(pi, Dac_gain0, afe_iqdac_att, 0xf);
		MOD_PHYREG(pi, Dac_gain1, afe_iqdac_att, 0xf);
		MOD_PHYREG(pi, Dac_gain2, afe_iqdac_att, 0xf);
		MOD_PHYREG(pi, Dac_gain3, afe_iqdac_att, 0xf);

		// Clear overrides specific to 43012
		// NEDIM_7271: follow 4347

		ACPHYREG_BCAST(pi, RfctrlOverrideNapPus0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideLogenBias0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0);
		ACPHYREG_BCAST(pi, Extra1AfeClkDivOverrideCtrl28nm0, 0x0);
		ACPHYREG_BCAST(pi, Extra2AfeClkDivOverrideCtrl28nm0, 0x0);
		MOD_PHYREG(pi, RfctrlCmd, syncResetEn, 0);
		
	}

	/* Force LOGENs on both cores in the 5G to be powered up for 4350C0 */
	if (ACMAJORREV_2(pi->pubpi.phy_rev) && (ACMINORREV_1(pi) ||
		ACMINORREV_3(pi))) {
		MOD_PHYREG(pi, RfctrlOverrideTxPus0, logen5g_lob_pwrup, 1);
		MOD_PHYREG(pi, RfctrlCoreTxPus0, logen5g_lob_pwrup, 1);
		MOD_PHYREG(pi, RfctrlOverrideTxPus1, logen5g_lob_pwrup, 1);
		MOD_PHYREG(pi, RfctrlCoreTxPus1, logen5g_lob_pwrup, 1);
	}

	/* Switch off rssi2 & rssi3 as they are not used in normal operation */
	ACPHYREG_BCAST(pi, RfctrlCoreRxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideRxPus0, 0x5000);

	/* Disable the SD-ADC's overdrive detect feature */
	val = READ_PHYREG(pi, RfctrlCoreAfeCfg20) |
	      ACPHY_RfctrlCoreAfeCfg20_afe_iqadc_reset_ov_det_MASK(pi->pubpi.phy_rev);
	/* val |= ACPHY_RfctrlCoreAfeCfg20_afe_iqadc_clamp_en_MASK; */
	ACPHYREG_BCAST(pi, RfctrlCoreAfeCfg20, val);
	val = READ_PHYREG(pi, RfctrlOverrideAfeCfg0) |
	      ACPHY_RfctrlOverrideAfeCfg0_afe_iqadc_reset_ov_det_MASK(pi->pubpi.phy_rev);
	/* val |= ACPHY_RfctrlOverrideAfeCfg0_afe_iqadc_clamp_en_MASK(pi->pubpi.phy_rev); */
	ACPHYREG_BCAST(pi, RfctrlOverrideAfeCfg0, val);

	/* initialize all the tables defined in auto-generated wlc_phytbl_ac.c,
	 * see acphyprocs.tcl, proc acphy_init_tbls
	 *  skip static one after first up
	 */
	PHY_TRACE(("wl%d: %s, dnld tables = %d\n", pi->sh->unit,
	           __FUNCTION__, pi->phy_init_por));

#if defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD)
	/* Don't download PHY table if ucode is not downloaded */
	BCM7XXX_ERR(("%s(%d)*****HACKHACK***** DON'T DL PHY TABLE\n",
		__FUNCTION__, __LINE__));
#else /* !defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD) */
	/* these tables are not affected by phy reset, only power down */
	if (RADIOID(pi->pubpi.radioid) != BCM20691_ID)
		wlc_phy_static_table_download_acphy(pi);
#endif /* defined(BCM7271_EMU) && defined(DISABLE_UCODE_DOWNLOAD) */

	if (ACMAJORREV_1(pi->pubpi.phy_rev)) {
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
	if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_path0, -190);
	} else {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_path0, 0x25C);
	}
	if ((ACMAJORREV_1(pi->pubpi.phy_rev) || ACMAJORREV_2(pi->pubpi.phy_rev)) &&
	    BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_second_path0, 0x25C);
	}

	if (flag2rangeon) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_second_path0, 0x25C);
	}


	/* Enable the Ucode TSSI_DIV WAR */
	if ((ACMAJORREV_1(pi->pubpi.phy_rev) || ACMAJORREV_2(pi->pubpi.phy_rev)) &&
	    BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
		wlapi_bmac_mhf(pi->sh->physhim, MHF2, MHF2_PPR_HWPWRCTL, MHF2_PPR_HWPWRCTL,
		               WLC_BAND_ALL);
	}

	if (ACMAJORREV_4(pi->pubpi.phy_rev)) {
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
	if (TINY_RADIO(pi)) {
		FOREACH_CORE(pi, core) {
			MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, fast_nap_bias_pu, 0);
			MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, fast_nap_bias_pu, 1);
		}
	}

	/* Temp 4345 register poweron default updates - */
	/* These need to be farmed out to correct locations */
	if (ACMAJORREV_3(pi->pubpi.phy_rev)) {
		MOD_PHYREG(pi, RxSdFeConfig5, rx_farow_scale_value, 7);
		MOD_PHYREG(pi, FSTRHiPwrTh, finestr_hiPwrSm_th, 77);
		MOD_PHYREG(pi, FSTRHiPwrTh, finestr_hiPwr_th, 63);
		MOD_PHYREG(pi, crsThreshold2l, peakThresh, 77);
		MOD_PHYREG(pi, crsminpoweruSub10, crsminpower0, 54);
		MOD_PHYREG(pi, crsminpoweruSub10, crsminpower0, 54);
		MOD_PHYREG(pi, crsminpoweru0, crsminpower0, 54);
		MOD_PHYREG(pi, bphyaciThresh0, bphyaciThresh0, 0);
		MOD_PHYREG(pi, bphyaciPwrThresh2, bphyaciPwrThresh2, 0);
		MOD_PHYREG(pi, RxSdFeSampCap, capture_both, 0);
		/* MOD_PHYREG(pi, TxMacDelay, macdelay, 680); */
		MOD_PHYREG(pi, AfeseqRx2TxPwrUpDownDly20M, delayPwrUpDownRx2Tx20M, 3);
		MOD_PHYREG(pi, RfctrlOverrideTxPus0, bg_pulse, 1);
		MOD_PHYREG(pi, PapdEnable0, gain_dac_rf_override0, 1);
		MOD_PHYREG(pi, RfctrlCoreTxPus0, bg_pulse, 1);
		MOD_PHYREG(pi, PapdEnable0, gain_dac_rf_reg0, 0);
		MOD_PHYREG(pi, RfseqMode, CoreActv_override, 0);
		MOD_PHYREG(pi, RfctrlOverrideLowPwrCfg0, rxrf_lna2_gm_size, 0);
		MOD_PHYREG(pi, Rfpll_resetCtrl, rfpll_reset_dur, 64);
		MOD_PHYREG(pi, CoreConfig, NumRxAnt, 1);
		MOD_PHYREG(pi, Pllldo_resetCtrl, pllldo_reset_dur, 8000);
		MOD_PHYREG(pi, bphyaciPwrThresh0, bphyaciPwrThresh0, 0);
		MOD_PHYREG(pi, bphyaciThresh1, bphyaciThresh1, 0);
		MOD_PHYREG(pi, HTSigTones, support_gf, 1);
		MOD_PHYREG(pi, ViterbiControl0, CacheHitEn, 1);
		MOD_PHYREG(pi, sampleDepthCount, DepthCount, 19);
		MOD_PHYREG(pi, bphyaciPwrThresh1, bphyaciPwrThresh1, 0);
		MOD_PHYREG(pi, Core0cliploGainCodeB, clip1loTrTxIndex, 0);
		MOD_PHYREG(pi, clip2carrierDetLen, clip2carrierDetLen, 64);
		MOD_PHYREG(pi, Core0clip2GainCodeA, clip2LnaIndex, 1);
		MOD_PHYREG(pi, Core0clipmdGainCodeB, clip1mdBiQ0Index, 0);
		MOD_PHYREG(pi, Core0clip2GainCodeB, clip2BiQ1Index, 2);
		MOD_PHYREG(pi, Core0cliploGainCodeA, clip1loLnaIndex, 1);
		MOD_PHYREG(pi, Core0cliploGainCodeB, clip1loBiQ0Index, 0);
		MOD_PHYREG(pi, Core0clipHiGainCodeB, clip1hiBiQ0Index, 0);
		MOD_PHYREG(pi, Core0clipmdGainCodeA, clip1mdmixergainIndex, 3);
		MOD_PHYREG(pi, Core0cliploGainCodeB, clip1loTrRxIndex, 1);
		WRITE_PHYREG(pi, Core0InitGainCodeB, 0x2204);
		MOD_PHYREG(pi, AfectrlCore10, adc_pd, 1);
	}

	if (ACMAJORREV_3(pi->pubpi.phy_rev)) {
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
	if (ACMAJORREV_1(pi->pubpi.phy_rev) && ACMINORREV_2(pi)) {
		MOD_PHYREG(pi, fineclockgatecontrol, forceRfSeqgatedClksOn, 1);
	}
	*/

	if (TINY_RADIO(pi) && (PHY_IPA(pi) || pi->epacal2g || pi->epacal5g)) {
		/* Zero out paplutselect table used in txpwrctrl */
		uint8 initvalue = 0;
		uint16 j;
		for (j = 0; j < 128; j++) {
			wlc_phy_table_write_acphy(pi,
				ACPHY_TBL_ID_PAPDLUTSELECT0, 1, j, 8, &initvalue);
			if (ACMAJORREV_4(pi->pubpi.phy_rev) &&
				(phy_get_phymode(pi) == PHYMODE_MIMO)) {
				wlc_phy_table_write_acphy(pi,
					ACPHY_TBL_ID_PAPDLUTSELECT1, 1, j, 8, &initvalue);
			}
		}
	}

	if (ACMAJORREV_32(pi->pubpi.phy_rev)) {
		// still empty here
	}
	
	if (ACMAJORREV_33(pi->pubpi.phy_rev) && ACMINORREV_4(pi)) {
		// luka		
		WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0x0);
		WRITE_PHYREG(pi, RfctrlCmd, 0x306);
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x1);
	    
		ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlIntc0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideAfeCfg0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x0 );
		ACPHYREG_BCAST(pi, RfctrlOverrideRxPus0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideGains0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideLpfCT0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideLpfSwtch0, 0x0);
		//KAI: TCL has the following register write set to 0x0
		ACPHYREG_BCAST(pi, RfctrlOverrideAfeDivCfg0, 0x8f0b);
		ACPHYREG_BCAST(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideETCfg0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0x0);
		ACPHYREG_BCAST(pi, RfctrlOverrideAuxTssi0, 0x0);   
		ACPHYREG_BCAST(pi, RfctrlOverrideLogenBias0, 0x0);   
		ACPHYREG_BCAST(pi, RfctrlOverrideNapPus0, 0x0); 
		ACPHYREG_BCAST(pi, AfectrlOverride0, 0x0); 

		//pllldo_pu = 1,  {pllldo, rfpll}_reset = 0
		WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	    
		//using phy override
		WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

		MOD_PHYREG(pi, RfctrlCmd, chip_pu, 1);

		/* set rfseq_bundle_tbl {0x01C7034 0x01C7014 0x02C703E 0x02C701C} */
		/* acphy_write_table RfseqBundle $rfseq_bundle_tbl 0 */
		/* acphy_write_table RfseqBundle {0x01C703C 0x01C7014 0x02C700E 0x02C702C 0x0206020 0x400 0x0} 0; */	
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
#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

void
wlc_phy_radio2069_pwrdwn_seq(phy_info_t *pi)
{

	/* AFE */
	pi->u.pi_acphy->afeRfctrlCoreAfeCfg10 = READ_PHYREG(pi, RfctrlCoreAfeCfg10);
	pi->u.pi_acphy->afeRfctrlCoreAfeCfg20 = READ_PHYREG(pi, RfctrlCoreAfeCfg20);
	pi->u.pi_acphy->afeRfctrlOverrideAfeCfg0 = READ_PHYREG(pi, RfctrlOverrideAfeCfg0);
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg10, 0);
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg20, 0);
	WRITE_PHYREG(pi, RfctrlOverrideAfeCfg0, 0x1fff);

	/* Radio RX */
	pi->u.pi_acphy->rxRfctrlCoreRxPus0 = READ_PHYREG(pi, RfctrlCoreRxPus0);
	pi->u.pi_acphy->rxRfctrlOverrideRxPus0 = READ_PHYREG(pi, RfctrlOverrideRxPus0);
	WRITE_PHYREG(pi, RfctrlCoreRxPus0, 0x40);
	WRITE_PHYREG(pi, RfctrlOverrideRxPus0, 0xffbf);

	/* Radio TX */
	pi->u.pi_acphy->txRfctrlCoreTxPus0 = READ_PHYREG(pi, RfctrlCoreTxPus0);
	pi->u.pi_acphy->txRfctrlOverrideTxPus0 = READ_PHYREG(pi, RfctrlOverrideTxPus0);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, 0);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* {radio, rfpll, pllldo}_pu = 0 */
	pi->u.pi_acphy->radioRfctrlCmd = READ_PHYREG(pi, RfctrlCmd);
	pi->u.pi_acphy->radioRfctrlCoreGlobalPus = READ_PHYREG(pi, RfctrlCoreGlobalPus);
	pi->u.pi_acphy->radioRfctrlOverrideGlobalPus = READ_PHYREG(pi, RfctrlOverrideGlobalPus);
	MOD_PHYREG(pi, RfctrlCmd, chip_pu, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0x1);
}

void
wlc_phy_radio2069_pwrup_seq(phy_info_t *pi)
{

	/* AFE */
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg10, pi->u.pi_acphy->afeRfctrlCoreAfeCfg10);
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg20, pi->u.pi_acphy->afeRfctrlCoreAfeCfg20);
	WRITE_PHYREG(pi, RfctrlOverrideAfeCfg0, pi->u.pi_acphy->afeRfctrlOverrideAfeCfg0);

	/* Restore Radio RX */
	WRITE_PHYREG(pi, RfctrlCoreRxPus0, pi->u.pi_acphy->rxRfctrlCoreRxPus0);
	WRITE_PHYREG(pi, RfctrlOverrideRxPus0, pi->u.pi_acphy->rxRfctrlOverrideRxPus0);

	/* Radio TX */
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, pi->u.pi_acphy->txRfctrlCoreTxPus0);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, pi->u.pi_acphy->txRfctrlOverrideTxPus0);

	/* {radio, rfpll, pllldo}_pu = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, pi->u.pi_acphy->radioRfctrlCmd);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, pi->u.pi_acphy->radioRfctrlCoreGlobalPus);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, pi->u.pi_acphy->radioRfctrlOverrideGlobalPus);
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

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20693_ID);

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

#ifndef WLTXBF_DISABLED
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
#endif
	MOD_RADIO_REG_20693(pi, SPARE_CFG8, core, afe_BF_pu0, afeBFpu0);
	MOD_RADIO_REG_20693(pi, SPARE_CFG8, core, afe_BF_pu1, afeBFpu1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, afe_clk_div_12g_pu, afeclk_div12g_pu);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_pu, 0x1);

#ifdef WLTXBF_DISABLED
	pupd = 0;
#else
	pupd = 1;
#endif /* WLTXBF_DISABLED */

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

void wlc_phy_radio20693_mimo_core1_pmu_off(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi), temp;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	uint16 *ptr_start = (uint16 *)&porig->pll_xtalldo1[0];
	uint8 core;

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);
	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	/* Turn off core 1 PMU + other lp optimizations */
	ASSERT(!porig->is_orig);
	porig->is_orig = TRUE;
	PHY_TRACE(("%s: Turning off core 1\n", __FUNCTION__));
	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
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
			RADIO_REG_20693(pi, SPARE_CFG2, core)
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &porig->spare_cfg2[PHY_CORE_MAX-1]);
			*ptr = _READ_RADIO_REG(pi, porig_offsets[ct]);
		}
	}
	/* Core 0 Settings */
	MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_lowquiescenten, 0x1);
	MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_pull_down_sw, 0x0);
	MOD_RADIO_REG_20693(pi, PLL_CP1, 0, rfpll_cp_bias_low_power, 0x1);
	MOD_RADIO_REG_20693(pi, VREG_CFG, 0, vreg25_pu, 0x1);
	MOD_RADIO_REG_20693(pi, VREG_OVR1, 0, ovr_vreg25_pu, 0x1);

	/* MEM opt. Setting wlpmu_en, wlpmu_VCOldo_pu, wlpmu_TXldo_pu,
	   wlpmu_AFEldo_pu, wlpmu_RXldo_pu
	 */
	temp = READ_RADIO_REG_TINY(pi, PMU_OP, 0);
	phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OP, 0), (temp | 0x009E));

	MOD_RADIO_REG_20693(pi, PMU_CFG4, 0, wlpmu_ADCldo_pu, 0x1);
	MOD_RADIO_REG_20693(pi, LOGEN_CFG2, 0, logencore_det_stg1_pu, 0x0);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR1, 0, ovr_logencore_det_stg1_pu, 0x1);
	MOD_RADIO_REG_20693(pi, LOGEN_CFG3, 0, logencore_2g_mimodes_pu, 0x0);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR2, 0, ovr_logencore_2g_mimodes_pu, 0x1);
	MOD_RADIO_REG_20693(pi, LOGEN_CFG3, 0, logencore_2g_mimosrc_pu, 0x0);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR2, 0, ovr_logencore_2g_mimosrc_pu, 0x1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, 0, afe_clk_div_6g_mimo_pu, 0x0);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, 0, ovr_afeclkdiv_6g_mimo_pu, 0x1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, 0, afe_clk_div_12g_pu, 0x1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, 0, ovr_afeclkdiv_12g_pu, 0x1);
	MOD_RADIO_REG_20693(pi, SPARE_CFG2, 0, afe_clk_div_se_drive, 0x1);

	/* Core 1 Settings */
	MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_lowquiescenten, 0x1);
	MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_pull_down_sw, 0x0);
	MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, 1, ovr_pa2g_tssi_ctrl_pu, 0x1);
	MOD_RADIO_REG_20693(pi, AUXPGA_OVR1, 1, ovr_auxpga_i_pu, 0x1);
	MOD_RADIO_REG_20693(pi, VREG_CFG, 1, vreg25_pu, 0x0);
	MOD_RADIO_REG_20693(pi, VREG_OVR1, 1, ovr_vreg25_pu, 0x1);

	/* MEM opt. UN-Setting wlpmu_en, wlpmu_VCOldo_pu, wlpmu_TXldo_pu,
	   wlpmu_AFEldo_pu, wlpmu_RXldo_pu
	 */
	temp = READ_RADIO_REG_TINY(pi, PMU_OP, 1);
	phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OP, 1), (temp & ~0x009E));

	MOD_RADIO_REG_20693(pi, PMU_CFG4, 1, wlpmu_ADCldo_pu, 0x0);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR1, 1, ovr_logencore_det_stg1_pu, 0x1);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR2, 1, ovr_logencore_2g_mimodes_pu, 0x1);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR2, 1, ovr_logencore_2g_mimosrc_pu, 0x1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, 1, ovr_afeclkdiv_6g_mimo_pu, 0x1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, 1, ovr_afeclkdiv_12g_pu, 0x1);
}

void wlc_phy_radio20693_mimo_core1_pmu_on(phy_info_t *pi)
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
	PHY_TRACE(("%s: Turning on core 1\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
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
			RADIO_REG_20693(pi, SPARE_CFG2, core)
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &porig->spare_cfg2[PHY_CORE_MAX-1]);
			phy_utils_write_radioreg(pi, porig_offsets[ct], *ptr);
		}
	}
	wlc_phy_radio20693_minipmu_pwron_seq(pi);
	wlc_phy_tiny_radio_minipmu_cal(pi);
	wlc_phy_radio20693_pmu_pll_config(pi);
	wlc_phy_radio20693_afecal(pi);
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

void wlc_phy_logen_reset(phy_info_t *pi, uint8 core)
{
	// before 4365, we use direct controls to reset logen
	if (!(RADIOID(pi->pubpi.radioid) == BCM20693_ID && RADIOMAJORREV(pi) == 3)) {
		MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, logen_reset, 1);
		OSL_DELAY(1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, logen_reset, 0);
	} else {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			MOD_RADIO_PLLREG_20693(pi, LO2G_LOGEN0_CFG1, 0, logen0_reset, 1);
			MOD_RADIO_PLLREG_20693(pi, LO2G_LOGEN1_CFG1, 1, logen1_reset, 1);
			OSL_DELAY(1);
			MOD_RADIO_PLLREG_20693(pi, LO2G_LOGEN0_CFG1, 0, logen0_reset, 0);
			MOD_RADIO_PLLREG_20693(pi, LO2G_LOGEN1_CFG1, 1, logen1_reset, 0);
		} else {
			MOD_RADIO_PLLREG_20693(pi, LO5G_MISC1, 0, lo5g_reset, 1);
			OSL_DELAY(1);
			MOD_RADIO_PLLREG_20693(pi, LO5G_MISC1, 0, lo5g_reset, 0);
		}
	}
}

/* Global Radio PU's */
static void wlc_phy_radio20696_pwron_seq_phyregs(phy_info_t *pi)
{
#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */
	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  So, to make rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything	 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);                  /* {radio, rfpll}_pu = 0 */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);        /* pllldo_{pu, reset} = 0, rfpll_reset = 0 */
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);  /* using phy override   */

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);                /* radio_{pwrup, reset} = 1, rfpll_pu = 0 */
	OSL_DELAY(1000);                                 /* radio_reset to be high for 100us // change to 1ms by Luka*/
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);                /* radio_pwrup = 1, radio_ reset = 0, rfpll_pu = 0 */

	/* YOU ABSOLUTELY NEED TO UPDATE THE PREFERRED VALUES HERE. */
	/* phyreg(RfctrlCmd) wipes out a metric ton of RF registers. You gotta re-init. */
	/*  Update preferred values */
	wlc_phy_radio20696_upd_prfd_values(pi);
	
	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);      /* pllldo_{pu, reset} = 1, rfpll_reset = 1 */
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);                /* {radio, rfpll}_pwrup = 1 */

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180)      /* logen_{pwrup, reset} = 1 */;
	OSL_DELAY(1000);                                 /* resets to be on for 100us // change to 1ms by Luka*/
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);      /* {pllldo, rfpll}_reset = 0 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);      /* logen_reset = 0 */
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180); /* leave overrides for logen_{pwrup, reset} */
#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

/* PU the TX/RX/VCO/..LDO's */
static void wlc_phy_radio20696_minipmu_pwron_seq(phy_info_t *pi)
{
	uint8 core;
#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */

	/* Turn on two 1p8LDOs and Bandgap  */
	MOD_RADIO_REG_20696(pi, LDO1P8_STAT, 1, ldo1p8_pu, 0x1);
	MOD_RADIO_REG_20696(pi, LDO1P8_STAT, 3, ldo1p8_pu, 0x1);
	MOD_RADIO_REG_20696(pi, BG_REG2, 1, bg_pu_bgcore, 0x1);
	MOD_RADIO_REG_20696(pi, BG_REG2, 1, bg_pu_V2I, 0x1);
	MOD_RADIO_REG_20696(pi, BG_OVR1, 1, ovr_bg_pu_bgcore, 0x1);

	/* Hold RF PLL in reset */
	MOD_RADIO_PLLREG_20696(pi, PLL_CFG2, rfpll_rst_n, 0x0);
	MOD_RADIO_PLLREG_20696(pi, PLL_OVR1, ovr_rfpll_rst_n, 0x1	);
	MOD_RADIO_PLLREG_20696(pi, PLL_VCOCAL1, rfpll_vcocal_rst_n, 0x0	);
	MOD_RADIO_PLLREG_20696(pi, PLL_VCOCAL_OVR1, ovr_rfpll_vcocal_rst_n, 0x1);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20696(pi, RX2G_REG4, core, rx_ldo_pu, 0x1);             /* RxLDO pu */
		MOD_RADIO_REG_20696(pi, LDO1P65_STAT, core, wlpmu_ldo1p6_pu, 0x1);    /* 1p6 LDO pu */
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_ADCldo_pu, 0x1);        /* ADC LDO pu */
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_AFEldo_pu, 0x1);        /* AFE LDO pu */
		MOD_RADIO_REG_20696(pi, PMU_CFG4, core, wlpmu_LDO2P1_pu, 0x1);        /* 2p2 LDO pu */
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_LOGENldo_pu, 0x1);      /* LOGEN LDO pu */
		MOD_RADIO_REG_20696(pi, PMU_CFG1, core, wlpmu_TXldo_pu, 0x1);         /* TxLDO pu */
		MOD_RADIO_REG_20696(pi, PMU_CFG4, core, wlpmu_en, 0x1);               /* Main miniPMU pu */
		MOD_RADIO_REG_20696(pi, PMU_CFG3, core, vref_select, 0x0);            /* Use miniPMU reference for power up */

            /* Overwrites */
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_ADCldo_pu, 0x1);    /* ADC LDO overwrite */
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_AFEldo_pu, 0x1);    /* AFE LDO overwrite */
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_LDO2P1_pu, 0x1);    /* 2p2 LDO overwrite */
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_LOGENldo_pu, 0x1);  /* LOGEN LDO overwrite */
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_TXldo_pu, 0x1);     /* Tx LDO overwrite */
		MOD_RADIO_REG_20696(pi, PMU_OVR1, core, ovr_wlpmu_en, 0x1);           /* Main miniPMU overwrite */
	}
#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

/*  R Calibration (takes ~50us) */
/* copied from IGUANA and adabted for 7271 */
/* original code in wlc_phy_28nm_radio_rcal */
/* modified were needed from 20696_procs.tcl */ 
static void 
wlc_phy_radio20696_r_cal(phy_info_t *pi , uint8 mode)
{
	/* Format: 20696_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
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
		/* Default RCal code to be used with mode 1 is 0x32 (value taken from20696_procs.tcl */
		rcal_value = 0x32;
		MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
		MOD_RADIO_REG_20696(pi, BG_REG8, core, bg_rcal_trim, rcal_value);
		MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
		MOD_RADIO_REG_20696(pi, BG_REG1, core, bg_rtl_rcal_trim, rcal_value);
		//MOD_RADIO_REG_20696(pi, BG_REG2, core, bg_ate_rcal_trim_en, 1);
	} else if (mode == 2) {
		/* This should run only for core 1 */
		/* Run R Cal and use its output */

		//MOD_RADIO_REG_20696_MULTI_2(pi, XTAL5, core, xtal_pu_wlandig, 1,
		//	xtal_pu_RCCAL, 1);
		//MOD_RADIO_REG_20696(pi, BG_REG2, core, bg_ate_rcal_trim_en, 0x0);
		/* Make connection with the external 10k resistor */
		/* Also powers up rcal (RF0_rcal_cfg_north.rcal_pu = 1) */

		//MOD_RADIO_REG_20696_MULTI_2(pi, GPAIO_SEL2, core, gpaio_rcal_pu, 0,
		//	gpaio_pu, 1);
		/*  This is what Tcl does to get GPIOs set
		Y	set reg(RF${core}_gpaio_sel2.gpaio_pu) 0
		Y	set reg(RF1_gpaio_sel6.gpaio_top_pu) 0
		Y	set reg(RF1_gpaio_sel8.gpaio_rcal_pu) 0
		Y	set reg(RF${core}_gpaio_sel0.gpaio_sel_0to15_port) 0x0
		Y	set reg(RF${core}_gpaio_sel1.gpaio_sel_16to31_port) 0x0		 
		Y	set reg(RF1_gpaio_sel6.gpaio_top_pu) 1
		Y	set reg(RF1_gpaio_sel8.gpaio_rcal_pu) 1
		Y	set reg(RF1_bg_reg3.bg_rcal_pu) 1
		Y	set reg(RF1_rcal_cfg_north.rcal_pu) 1		
		*/
		MOD_RADIO_REG_20696(pi, GPAIO_SEL2, core,
				gpaio_pu, 0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL6, core,
				gpaio_top_pu, 0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL8, core,
				gpaio_rcal_pu, 0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL0, core,
				gpaio_sel_0to15_port, 0x0);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL1, core,
				gpaio_sel_16to31_port, 0x0);
		//MOD_RADIO_REG_20696(pi, GPAIO_SEL3, core,
		//		gpaio_sel_32to47_port, 0x0);

		MOD_RADIO_REG_20696(pi, GPAIO_SEL6, core,
				gpaio_top_pu, 1);
		MOD_RADIO_REG_20696(pi, GPAIO_SEL8, core,
				gpaio_rcal_pu, 1);
		MOD_RADIO_REG_20696(pi, BG_REG3, core,
				bg_rcal_pu, 1);
		MOD_RADIO_REG_20696(pi, RCAL_CFG_NORTH, core,
				rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			/* 1 ms delay wait time */
			OSL_DELAY(1000);
			//KAIHACK watch out READ macro only takes 3 arguments
			//rcal_valid = READ_RADIO_REG_20696(pi, RCAL_CFG_NORTH, core);
			rcal_valid = READ_RADIO_REGFLD_20696(pi, RF, RCAL_CFG_NORTH, core,rcal_valid);
			loop_iter++;
		}

		if (rcal_valid == 1) {
			//rcal_value = READ_RADIO_REG_20696(pi, RCAL_CFG_NORTH, core);
			rcal_value = READ_RADIO_REGFLD_20696(pi, RF, RCAL_CFG_NORTH, core,rcal_value);
			/* Use the output of the rcal engine */
			/* Don't use OTP value */
			MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
			MOD_RADIO_REG_20696(pi, BG_REG8, core, bg_rcal_trim, rcal_value);
			MOD_RADIO_REG_20696(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
			printf("7271 -- R CAL -- rcal_value = 0x%x\n",rcal_value);
			//KAIHACk comment out following 2 - not used in TCL either
			//MOD_RADIO_REG_20696(pi, BG_REG2, core, bg_ate_rcal_trim_en, 1);
			//MOD_RADIO_REG_20696(pi, BG_REG1, core, bg_ate_rcal_trim, rcal_value);

			/* Very coarse sanity check */
			if ((rcal_value < 2) || (60 < rcal_value)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				" 6bit Rcal = %d.\n", rcal_value));
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
				__FUNCTION__, rcal_valid));
		}

		/* Power down blocks not needed anymore */
		MOD_RADIO_REG_20696(pi, RCAL_CFG_NORTH, core,
				rcal_pu, 0);
	}
}

/* Perform MINI PMU CAL */
static void wlc_phy_radio20696_minipmu_cal(phy_info_t *pi)
{
	uint8 waitcounter;
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
			/* This means the cal_done bit is not 1 even after waiting a while. Exit gracefully. */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n", pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
	} 
	/* Cleanup : Setting the caldone bit to 0 here */
	WRITE_RADIO_REG_20696(pi, BG_REG10, 1, 0x2C);	
	MOD_RADIO_REG_20696(pi, BG_REG10, 1, bg_wlpmu_bypcal, 0x1 );
	MOD_RADIO_REG_20696(pi, PMU_CFG3, 0, vref_select, 0x1);
	MOD_RADIO_REG_20696(pi, PMU_CFG3, 1, vref_select, 0x1);
	MOD_RADIO_REG_20696(pi, PMU_CFG3, 2, vref_select, 0x1);
	MOD_RADIO_REG_20696(pi, PMU_CFG3, 3, vref_select, 0x1);
	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_REGFLD_20696(pi, RF, BG_REG11, 1, bg_wlpmu_calcode);
		printf("%s: MiniPMU cal done. Cal code = %d\n", __FUNCTION__, tempcalcode);
		MOD_RADIO_REG_20696(pi, BG_REG9, 1, bg_wlpmu_cal_mancodes, tempcalcode);
	}
}

/* PLL PWR-UP */
static void wlc_phy_radio20696_pmu_pll_pwrup(phy_info_t *pi)
{
#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */
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
#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

/* Work-arounds */
static void wlc_phy_radio20696_wars(phy_info_t *pi)
{
	uint8 core;

#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */

	/* TODO: Check if this can be done with RFX just like in TCL */
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

		//Kai: Following TCL we will remove lines below - Luka enabled AFE cal elsewhere in this file
		// HACKHACK: following reg write necessary for Tx until proc 20696_afe_cal is ported
		//WRITE_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, 0xc000);

		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 1);
		MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 1);

		/* 7271: BUGFIX - HWJUSTY-285 TxEVM in 5GHz influenced by ovr_auxpga_i_pu */
		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1);
	}

	/* if { $def(d11_radio_minor_rev) == 0 } { */
	FOREACH_CORE(pi, core) {
		/* 20696_tia_lpf_set_by_override bq1_bw 0 bq1_gain 6 bq2_bw 0 bq2_q 1 bq2_gain 1 rc_bw 4 gmult 4096 core $core */
		/* puts "\n ###############  override ALPF ###############  \n" */
		/* These 2 are subset of (and can taken care of) via  lpf_set_by_override above */
		/* temporary fix for 7271 - move this to RFseq and condition on BW */
		
		// MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_g_passive_rc, 0x1);
		// MOD_RADIO_REG_20696(pi, LPF_REG5, core, lpf_g_passive_rc, 0x1820);
	}
	/* 7271: BUGFIX - HWJUSTY-259 missing this reg write causes Tx tone to fail on all 4 cores */
	MOD_RADIO_REG_20696(pi, LOGEN_CORE_REG5, 3, logen_mux_pu, 1);

#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */

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

	ASSERT(RADIOID(pi->pubpi.radioid) == BCM20696_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi.radiorev)) {
	case 0: prefregs_20696_ptr = prefregs_20696_rev0;
		break;

	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi.radiorev)));
		ASSERT(FALSE);
		return;
	}

	/* Update preferred values */
	while (prefregs_20696_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20696_ptr[i].address,
			(uint16)prefregs_20696_ptr[i].init);
		i++;
	}


	printf("..... calling 20696_refclk_en from proc 20696_upd_prfd_values (in 20696_proc.tcl) .....\n");
	wlc_phy_radio20696_refclk_en(pi);
	
	/* printf("..... ovr and turn on pu's relevant to PLL (in 20696_proc.tcl) .....\n"); */
	
	/* turn ON Common LOGen pu's */
	MOD_RADIO_PLLREG_20696(pi, LOGEN_REG0, logen_vcobuf_pu, 0x1);
	MOD_RADIO_PLLREG_20696(pi, LOGEN_REG0, logen_pu, 0x1);
	MOD_RADIO_REG_20696(pi, BG_REG10, 1, bg_wlpmu_pu_cbuck_ref, 0x1); /* this is not working in veloce when placed here - pmu cal failed? */
	printf("forcing bg_wlpmu_pu_cbuck_ref $reg(RF1_bg_reg10.bg_wlpmu_pu_cbuck_ref)\n");
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

void
wlc_phy_radio20696_adc_cap_cal(phy_info_t *pi, int mode) {
	uint8 core;
	uint8 normal = 1;
	uint16 adccapcal_Timeout;
	uint16 adccapcal_done;

#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */

	if ( mode == ADC_CAL_CAL_MODE_BYPASS ) {
		printf("ADC cap calibration -- bypass\n");

		//bypass ADC cap calibration, and use ideal calibration values
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
		}
	} else {
		FOREACH_CORE(pi, core) {
			//power up dac by override
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_buf_pu, 1);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_diode, 1);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_I, 1);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_Q, 1);
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_pwrup_diode, 1);
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_pwrup_I, 1);
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_pwrup_Q, 1);
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_pu, 1);

			//power up adc by override
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puI, 1);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puQ, 1);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 1);
			MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_puI, 1);
			MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_puQ, 1);
			MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_power_mode_enb, 0x0);
			MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_power_mode, 0x3);
				
			//just enable afe loopback to drive ADC 
			MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, afe_en_loopback, 1);
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 1);
			
			//disconnect ADC from others
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,        1);
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,         1);
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,        1);
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,         1);
			MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,        1);
			MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,        1);
			MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,        1);
			MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq1_bq2,           0x0);
			MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq2_adc,           0x0);
			MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq1_adc,           0x0);
			MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_dac_bq2,           0x0);
			MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq2_rc,            0x0);
			MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_dac_rc,            0x0);
			MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_aux_adc,           0x0);
			//acphy_tx_tone off
			//wlc_phy_stopplayback_acphy(pi); /* TODO: is this needed? Causes an assert */
			//wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
			//wlapi_enable_mac(pi->sh->physhim);

			if ( mode == ADC_CAL_CAL_MODE_PARALLEL) {
//				printf("ADC cap calibration -- parallel\n");
				if (normal) {
					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x0);
					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x0);

					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x0);
					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x0);
					OSL_DELAY(100);
					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x1);
					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x1);
					OSL_DELAY(100);
					MOD_RADIO_REG_20696(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_I, 0x3);
					MOD_RADIO_REG_20696(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_Q, 0x3);
				} else {
					WRITE_RADIO_REG_20696(pi, RXADC_CFG1, core, 0x53ff);
					WRITE_RADIO_REG_20696(pi, RXADC_CFG0, core, 0x830d); // reset ADC cal engine					
				}
				//Trigger CAL
				if (normal) {
					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x1);
					MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x1);
				} else {
					WRITE_RADIO_REG_20696(pi, RXADC_CFG0, core, 0xfbfd); // reset ADC cal engine		
				}
			
				adccapcal_Timeout = 100; // 1000 * 10 = 10ms max time to wait
				/* Test both I and Q rail done bit */
				adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_I);
				adccapcal_done &= READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q);			
				while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
					OSL_DELAY(10);
					adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_I);
					adccapcal_done &= READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q);
					adccapcal_Timeout--;
				}
				if (adccapcal_Timeout == 0) {
//					printf("ADC cap calibration TIMEOUT\n");
				}
//				printf("adc_cap_cal -- core%d adc status (IQ):\n", core);
//				printf("RF%d_RXADC_CAL_STATUS.o_rxadc_cal_done_I = 0x%X\n", core, (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_I));
//				printf("RF%d_RXADC_CAL_STATUS.o_rxadc_cal_done_Q = 0x%X\n", core, (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q));
		
				if (0) {
					OSL_DELAY(10);
					printf("after adc cap cal the output register hold value\n");
					printf("core%d adc0: cap0 0x%X 0x%X\n", core, (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP0_STAT, core, o_coeff_cap0_adc_I), (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP0_STAT, core, o_coeff_cap0_adc_Q));
					printf("core%d adc0: cap1 0x%X 0x%X\n", core, (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP1_STAT, core, o_coeff_cap1_adc_I), (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP1_STAT, core, o_coeff_cap1_adc_Q));
					printf("core%d adc0: cap2 0x%X 0x%X\n", core, (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP2_STAT, core, o_coeff_cap2_adc_I), (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP2_STAT, core, o_coeff_cap2_adc_Q));
				}
			} else { 
				printf("ADC cap calibration -- sequential\n");
				//sequential, one path at a time
				//reset ADC cal engine			
				MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x0);
				MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x1);				
				//start the calibration process for I	
				MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x1);
				adccapcal_Timeout = 100; // 100 * 10 = 1ms max time to wait
				adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_I);
				while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
					OSL_DELAY(10);
					adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_I);
				        adccapcal_Timeout--;
				}
				if (adccapcal_Timeout == 0) {
//					printf("ADC cap calibration TIMEOUT for Core%d ADC I\n", core);
				}
//				printf("adc_cap_cal -- core%d adc status (I):\n", core);
//				printf("RF%d_RXADC_CAL_STATUS.o_rxadc_cal_done_I = 0x%X\n", core, (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_I));

				
				//reset ADC cal engine					
				MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x0);
				MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x1);				
				//start the calibration process for Q	
				MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x1);
				adccapcal_Timeout = 100; // 100 * 10 = 1ms max time to wait
				adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q);
				while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
					OSL_DELAY(10);
					adccapcal_done = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q);
				        adccapcal_Timeout--;
				}
				if (adccapcal_Timeout == 0) {
//					printf("ADC cap calibration TIMEOUT for Core%d ADC Q\n", core);
				}						
//				printf("adc_cap_cal -- core%d adc status (Q):\n", core);
//				printf("RF%d_RXADC_CAL_STATUS.o_rxadc_cal_done_Q = 0x%X\n", core, (unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAL_STATUS, core, o_rxadc_cal_done_Q));
			}
			wlc_phy_radio20696_apply_adc_cal_result(pi);
			if (adccapcal_Timeout == 0) {
				MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
			} else {
				MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
			}
			MOD_RADIO_REG_20696(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x1);
			if (0) {
				printf("after adc cal code write back, core%d: corr_dis 0x%X coeff_sel 0x%X \n", core, 
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG1, core, rxadc_corr_dis),
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG1, core, rxadc_coeff_sel));
				printf("core%d adc: cap0 0x%X 0x%X\n", core,
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG6, core, rxadc_coeff_cap0_adc_I),
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG9, core, rxadc_coeff_cap0_adc_Q));
				printf("core%d adc: cap1 0x%X 0x%X\n", core,
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG5, core, rxadc_coeff_cap1_adc_I),
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG8, core, rxadc_coeff_cap1_adc_Q));
				printf("core%d adc: cap2 0x%X 0x%X\n", core,
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG5, core, rxadc_coeff_cap2_adc_I),
					(unsigned int)READ_RADIO_REGFLD_20696(pi, RF, RXADC_CFG8, core, rxadc_coeff_cap2_adc_Q));
			}
			//remove lpf sw override
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,        0);
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,         0);
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,        0);
			MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,         0);
			MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,        0);
			MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,        0);
			MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,        0);
			MOD_RADIO_REG_20696(pi, RXADC_CFG0, core, afe_en_loopback,         0);
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 	1); // 0->1 HWJUSTY-272 
			//remove adc override 	
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puI, 0);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puQ, 0);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 0);
			//remove dac override
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_buf_pu, 0);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_diode, 0);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_I, 0);
			MOD_RADIO_REG_20696(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_Q, 0);

		}
	}
	wlc_phy_radio20696_txdac_bw_setup(pi, TXDAC_BW_SETUP_DACLOAD_LPF);

#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

void
wlc_phy_radio20696_apply_adc_cal_result(phy_info_t *pi) {
	uint8 core;
	uint16 value;

#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */

//	printf("Applying ADC cal result\n");
	FOREACH_CORE(pi, core) {	
		WRITE_RADIO_REG_20696(pi, AFE_CFG1_OVR1, core, 0x0FFF);	

		value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP0_STAT, core, o_coeff_cap0_adc_I);
		MOD_RADIO_REG_20696(pi, RXADC_CFG6, core, rxadc_coeff_cap0_adc_I, value);

		value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP1_STAT, core, o_coeff_cap1_adc_I);
		MOD_RADIO_REG_20696(pi, RXADC_CFG5, core, rxadc_coeff_cap1_adc_I, value);

		value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP2_STAT, core, o_coeff_cap2_adc_I);
		MOD_RADIO_REG_20696(pi, RXADC_CFG5, core, rxadc_coeff_cap2_adc_I, value);

		value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP0_STAT, core, o_coeff_cap0_adc_Q);
		MOD_RADIO_REG_20696(pi, RXADC_CFG9, core, rxadc_coeff_cap0_adc_Q, value);

		value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP1_STAT, core, o_coeff_cap1_adc_Q);
		MOD_RADIO_REG_20696(pi, RXADC_CFG8, core, rxadc_coeff_cap1_adc_Q, value);

		value = READ_RADIO_REGFLD_20696(pi, RF, RXADC_CAP2_STAT, core, o_coeff_cap2_adc_Q);
		MOD_RADIO_REG_20696(pi, RXADC_CFG8, core, rxadc_coeff_cap2_adc_Q, value);

		WRITE_RADIO_REG_20696(pi, AFE_CFG1_OVR1, core, 0x0);	
	}
#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

void
wlc_phy_radio20696_adc_offset_gain_cal(phy_info_t *pi) {
#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */

//	printf("adc_offset_gain_cal -- Bypass. It needs to be enabled if adc is in TI mode\n");
	
#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}

void
wlc_phy_radio20696_txdac_bw_setup(phy_info_t *pi, int dacload) {
	uint8 core;
	/*
	uint8 bwidx = 0;
	uint8 cfb_default_code[] = {0x50, 0x3c, 0x30, 0x1c};
	uint32 cfb_raw;
	uint32 radio_rccal_dacbuf_cmult = 4096;
	*/

#ifdef BCM7271
	BCM7XXX_ENTER();
#endif /* BCM7271 */

	FOREACH_CORE(pi, core) {
		// J -- this should have been set properly already
		// set reg(RF${core}_txdac_reg0.iqdac_buf_cmsel) 1 ; # Abhishek

		// J -- default is 0 but set to 1 during adccal
		MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_op_cur, 0x0);

		// dacload can be LPF or GM. Set to 1 with LPF, otherwise to 0 (TCL comment wrong?: isn't it inverted?)
		MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_cc, dacload);

		if (CHSPEC_IS160(pi->radio_chanspec) || CHSPEC_IS80(pi->radio_chanspec)) {
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_bw, 0x0);
		} else if (CHSPEC_IS40(pi->radio_chanspec) || CHSPEC_IS20(pi->radio_chanspec)) {
			MOD_RADIO_REG_20696(pi, TXDAC_REG0, core, iqdac_buf_bw, 0x4);
		}
		/*
		if (CHSPEC_IS160(pi->radio_chanspec)) {
			bwidx = 3;
		} else if (CHSPEC_IS80(pi->radio_chanspec)) {
			bwidx = 2;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			bwidx = 1;
		} else if (CHSPEC_IS20(pi->radio_chanspec)) {
			bwidx = 0;
		}

		cfb_raw = radio_rccal_dacbuf_cmult/ 4096 * cfb_default_code[bwidx];		
		MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_Cfb, (int)cfb_raw);
		*/
		MOD_RADIO_REG_20696(pi, TXDAC_REG1, core, iqdac_buf_Cfb, (int)127);
	}

#ifdef BCM7271
	BCM7XXX_EXIT();
#endif /* BCM7271 */
}
