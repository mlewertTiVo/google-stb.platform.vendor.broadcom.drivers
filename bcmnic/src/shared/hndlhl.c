/*
 * Misc utility routines for accessing lhl specific features
 * of the SiliconBackplane-based Broadcom chips.
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
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: hndpmu.c 547757 2015-04-13 10:18:04Z $
 */

#include <hndpmu.h>
#include <hndlhl.h>
#include <sbchipc.h>
#include <hndsoc.h>
#include <bcmdevs.h>
#include <osl.h>
#include <sbgci.h>
#include <siutils.h>
#include <bcmutils.h>
#ifdef BCMULP
#include <ulp.h>
#endif

#define SI_LHL_EXT_WAKE_REQ_MASK_MAGIC		0x7FBBF7FF	/* magic number for LHL EXT */

/* PmuRev1 has a 24-bit PMU RsrcReq timer. However it pushes all other bits
 * upward. To make the code to run for all revs we use a variable to tell how
 * many bits we need to shift.
 */
#define FLAGS_SHIFT	14
#define	LHL_ERROR(args) printf args

void
si_lhl_setup(si_t *sih, osl_t *osh)
{
	if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		/* Enable PMU sleep mode0 */
		LHL_REG(sih, lhl_top_pwrseq_ctl_adr, LHL_PWRSEQ_CTL, PMU_43012_SLEEP_MODE_2);
	}
}

/* To skip this function, specify a invalid "lpo_select" value in nvram */
int
si_lhl_set_lpoclk(si_t *sih, osl_t *osh)
{
	gciregs_t *gciregs;
	uint clk_det_cnt, status;
	int lhl_wlclk_sel;
	uint32 lpo = 0;
	int timeout = 0;
	gciregs = si_setcore(sih, GCI_CORE_ID, 0);

	ASSERT(gciregs != NULL);

	/* Apply nvram override to lpo */
	if ((lpo = (uint32)getintvar(NULL, "lpo_select")) == 0) {
		lpo = LHL_OSC_32k_ENAB;
	}

	/* Power up the desired LPO */
	switch (lpo) {
		case LHL_EXT_LPO_ENAB:
			LHL_REG(sih, lhl_main_ctl_adr, EXTLPO_BUF_PD, 0);
			lhl_wlclk_sel = LHL_EXT_SEL;
			break;

		case LHL_LPO1_ENAB:
			LHL_REG(sih, lhl_main_ctl_adr, LPO1_PD_EN, 0);
			lhl_wlclk_sel = LHL_LPO1_SEL;
			break;

		case LHL_LPO2_ENAB:
			LHL_REG(sih, lhl_main_ctl_adr, LPO2_PD_EN, 0);
			lhl_wlclk_sel = LHL_LPO2_SEL;
			break;

		case LHL_OSC_32k_ENAB:
			LHL_REG(sih, lhl_main_ctl_adr, OSC_32k_PD, 0);
			lhl_wlclk_sel = LHL_32k_SEL;
			break;

		default:
			goto done;
	}

	LHL_REG(sih, lhl_clk_det_ctl_adr,
		LHL_CLK_DET_CTL_AD_CNTR_CLK_SEL, lhl_wlclk_sel);

	/* Detect the desired LPO */

	LHL_REG(sih, lhl_clk_det_ctl_adr, LHL_CLK_DET_CTL_ADR_LHL_CNTR_EN, 0);
	LHL_REG(sih, lhl_clk_det_ctl_adr,
		LHL_CLK_DET_CTL_ADR_LHL_CNTR_CLR, LHL_CLK_DET_CTL_ADR_LHL_CNTR_CLR);
	timeout = 0;
	clk_det_cnt =
		((R_REG(osh, &gciregs->lhl_clk_det_ctl_adr) & LHL_CLK_DET_CNT) >>
		LHL_CLK_DET_CNT_SHIFT);
	while (clk_det_cnt != 0 && timeout <= LPO_SEL_TIMEOUT) {
		OSL_DELAY(10);
		clk_det_cnt =
		((R_REG(osh, &gciregs->lhl_clk_det_ctl_adr) & LHL_CLK_DET_CNT) >>
		LHL_CLK_DET_CNT_SHIFT);
		timeout++;
	}

	if (clk_det_cnt != 0) {
		LHL_ERROR(("Clock not present as clear did not work timeout = %d\n", timeout));
		goto error;
	}
	LHL_REG(sih, lhl_clk_det_ctl_adr, LHL_CLK_DET_CTL_ADR_LHL_CNTR_CLR, 0);
	LHL_REG(sih, lhl_clk_det_ctl_adr, LHL_CLK_DET_CTL_ADR_LHL_CNTR_EN,
		LHL_CLK_DET_CTL_ADR_LHL_CNTR_EN);
	clk_det_cnt =
		((R_REG(osh, &gciregs->lhl_clk_det_ctl_adr) & LHL_CLK_DET_CNT) >>
		LHL_CLK_DET_CNT_SHIFT);
	timeout = 0;

	while (clk_det_cnt <= CLK_DET_CNT_THRESH && timeout <= LPO_SEL_TIMEOUT) {
		OSL_DELAY(10);
		clk_det_cnt =
		((R_REG(osh, &gciregs->lhl_clk_det_ctl_adr) & LHL_CLK_DET_CNT) >>
		LHL_CLK_DET_CNT_SHIFT);
		timeout++;
	}

	if (timeout >= LPO_SEL_TIMEOUT) {
		LHL_ERROR(("LPO is not available timeout = %u\n, timeout", timeout));
		goto error;
	}

	/* Select the desired LPO */

	LHL_REG(sih, lhl_main_ctl_adr,
		LHL_MAIN_CTL_ADR_LHL_WLCLK_SEL, (lhl_wlclk_sel) << LPO_SEL_SHIFT);

	status = ((R_REG(osh, &gciregs->lhl_clk_status_adr) & LHL_MAIN_CTL_ADR_FINAL_CLK_SEL) ==
		(((1 << lhl_wlclk_sel) << LPO_FINAL_SEL_SHIFT))) ? 1 : 0;
	timeout = 0;
	while (!status && timeout <= LPO_SEL_TIMEOUT) {
		OSL_DELAY(10);
		status =
		((R_REG(osh, &gciregs->lhl_clk_status_adr) & LHL_MAIN_CTL_ADR_FINAL_CLK_SEL) ==
		(((1 << lhl_wlclk_sel) << LPO_FINAL_SEL_SHIFT))) ? 1 : 0;
		timeout++;
	}

	if (timeout >= LPO_SEL_TIMEOUT) {
		LHL_ERROR(("LPO is not available timeout = %u\n, timeout", timeout));
		goto error;
	}
	/* Power down the rest of the LPOs */

	if (lpo != LHL_EXT_LPO_ENAB) {
		LHL_REG(sih, lhl_main_ctl_adr, EXTLPO_BUF_PD, EXTLPO_BUF_PD);
	}

	if (lpo != LHL_LPO1_ENAB) {
		LHL_REG(sih, lhl_main_ctl_adr, LPO1_PD_EN, LPO1_PD_EN);
		LHL_REG(sih, lhl_main_ctl_adr, LPO1_PD_SEL, LPO1_PD_SEL_VAL);
	}
	if (lpo != LHL_LPO2_ENAB) {
		LHL_REG(sih, lhl_main_ctl_adr, LPO2_PD_EN, LPO2_PD_EN);
		LHL_REG(sih, lhl_main_ctl_adr, LPO2_PD_SEL, LPO2_PD_SEL_VAL);
	}
	if (lpo != LHL_OSC_32k_ENAB) {
		LHL_REG(sih, lhl_main_ctl_adr, OSC_32k_PD, OSC_32k_PD);
	}
	if (lpo != RADIO_LPO_ENAB) {
		si_gci_chipcontrol(sih, CC_GCI_CHIPCTRL_06, LPO_SEL, 0);
	}
done:
	return BCME_OK;
error:
	ROMMABLE_ASSERT(0);
	return BCME_ERROR;
}

void
si_lhl_timer_config(si_t *sih, osl_t *osh, int timer_type)
{
	uint origidx;
	pmuregs_t *pmu = NULL;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}

	ASSERT(pmu != NULL);

	switch (timer_type) {
	case LHL_MAC_TIMER:
		/* Enable MAC Timer interrupt */
		LHL_REG(sih, lhl_wl_mactim0_intrp_adr,
				(LHL_WL_MACTIM0_INTRP_EN | LHL_WL_MACTIM0_INTRP_EDGE_TRIGGER),
				(LHL_WL_MACTIM0_INTRP_EN | LHL_WL_MACTIM0_INTRP_EDGE_TRIGGER));

		/* Programs bits for MACPHY_CLK_AVAIL and all its dependent bits in
		 * MacResourceReqMask0.
		 */
		PMU_REG(sih, mac_res_req_mask, ~0, si_pmu_rsrc_macphy_clk_deps(sih, osh));

		/* One time init of mac_res_req_timer to enable interrupt and clock request */
		HND_PMU_SYNC_WR(sih, pmu, pmu, osh,
				PMUREGADDR(sih, pmu, pmu, mac_res_req_timer),
				((PRRT_ALP_REQ | PRRT_HQ_REQ | PRRT_INTEN) << FLAGS_SHIFT));
		break;

	case LHL_ARM_TIMER:
		/* Enable ARM Timer interrupt */
		LHL_REG(sih, lhl_wl_armtim0_intrp_adr,
				(LHL_WL_ARMTIM0_INTRP_EN | LHL_WL_ARMTIM0_INTRP_EDGE_TRIGGER),
				(LHL_WL_ARMTIM0_INTRP_EN | LHL_WL_ARMTIM0_INTRP_EDGE_TRIGGER));

		/* Programs bits for HT_AVAIL and all its dependent bits in ResourceReqMask0 */
		PMU_REG(sih, res_req_mask, ~0, si_pmu_rsrc_ht_avail_clk_deps(sih, osh));

		/* One time init of res_req_timer to enable interrupt and clock request
		 * For low power request only ALP (HT_AVAIL is anyway requested by res_req_mask)
		 */
		HND_PMU_SYNC_WR(sih, pmu, pmu, osh,
				PMUREGADDR(sih, pmu, pmu, res_req_timer),
				((PRRT_ALP_REQ | PRRT_HQ_REQ | PRRT_INTEN) << FLAGS_SHIFT));
		break;
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
}

void
si_lhl_timer_enable(si_t *sih)
{
	/* Enable clks for pmu int propagation */
	PMU_REG(sih, pmuintctrl0, PMU_INTC_ALP_REQ, PMU_INTC_ALP_REQ);

	PMU_REG(sih, pmuintmask0, RSRC_INTR_MASK_TIMER_INT_0, RSRC_INTR_MASK_TIMER_INT_0);
	LHL_REG(sih, lhl_main_ctl_adr, LHL_FAST_WRITE_EN, LHL_FAST_WRITE_EN);
	PMU_REG(sih, pmucontrol_ext, PCTL_EXT_USE_LHL_TIMER, PCTL_EXT_USE_LHL_TIMER);
}

void
si_lhl_ilp_config(si_t *sih, osl_t *osh, uint32 ilp_period)
{
	 gciregs_t *gciregs;
	 if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		gciregs = si_setcore(sih, GCI_CORE_ID, 0);
		ASSERT(gciregs != NULL);
		W_REG(osh, &gciregs->lhl_wl_ilp_val_adr, ilp_period);
	 }
}

#ifdef BCMULP
void
si_lhl_enable_sdio_wakeup(si_t *sih, osl_t *osh)
{

	gciregs_t *gciregs;
	pmuregs_t *pmu;
	gciregs = si_setcore(sih, GCI_CORE_ID, 0);
	ASSERT(gciregs != NULL);
	if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		/* For SDIO_CMD configure P8 for wake on negedge
		  * LHL  0 -> edge trigger intr mode,
		  * 1 -> neg edge trigger intr mode ,
		  * 6 -> din from wl side enable
		  */
		OR_REG(osh, &gciregs->gpio_ctrl_iocfg_p_adr[ULP_SDIO_CMD_PIN],
			(1 << GCI_GPIO_STS_EDGE_TRIG_BIT |
			1 << GCI_GPIO_STS_NEG_EDGE_TRIG_BIT |
			1 << GCI_GPIO_STS_WL_DIN_SELECT));
		/* Clear any old interrupt status */
		OR_REG(osh, &gciregs->gpio_int_st_port_adr[0], 1 << ULP_SDIO_CMD_PIN);

		/* LHL GPIO[8] intr en , GPIO[8] is mapped to SDIO_CMD */
		/* Enable P8 to generate interrupt */
		OR_REG(osh, &gciregs->gpio_int_en_port_adr[0], 1 << ULP_SDIO_CMD_PIN);

		/* Clear LHL GPIO status to trigger GCI Interrupt */
		OR_REG(osh, &gciregs->gci_intstat, GCI_INTSTATUS_LHLWLWAKE);
		/* Enable LHL GPIO Interrupt to trigger GCI Interrupt */
		OR_REG(osh, &gciregs->gci_intmask, GCI_INTMASK_LHLWLWAKE);
		OR_REG(osh, &gciregs->gci_wakemask, GCI_WAKEMASK_LHLWLWAKE);
		/* Note ->Enable GCI interrupt to trigger Chipcommon interrupt
		 * Set EciGciIntEn in IntMask and will be done from FCBS saved tuple
		 */
		/* Enable LHL to trigger extWake upto HT_AVAIL */
		/* LHL GPIO Interrupt is mapped to extWake[7] */
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
		ASSERT(pmu != NULL);
		/* Set bit 4 and 7 in ExtWakeMask */
		W_REG(osh, &pmu->extwakemask[0], CI_ECI	| CI_WECI);
		/* Program bits for MACPHY_CLK_AVAIL rsrc in ExtWakeReqMaskN */
		W_REG(osh, &pmu->extwakereqmask[0], SI_LHL_EXT_WAKE_REQ_MASK_MAGIC);
		/* Program 0 (no need to request explicitly for any backplane clk) */
		W_REG(osh, &pmu->extwakectrl, 0x0);
		/* Note: Configure MAC/Ucode to receive interrupt
		  * it will be done from saved tuple using FCBS code
		 */
	}
}
#endif /* BCMULP */
