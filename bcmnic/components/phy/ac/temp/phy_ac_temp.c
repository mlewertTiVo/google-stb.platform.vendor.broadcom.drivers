/*
 * ACPHY TEMPerature sense module implementation
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
 * $Id: phy_ac_temp.c 670973 2016-11-18 10:18:04Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_btcx.h>
#include "phy_type_temp.h"
#include "phy_temp_st.h"
#include <phy_ac.h>
#include <phy_ac_temp.h>
#include <phy_ac_calmgr.h>
#include <phy_ac_info.h>
#include <phy_utils_reg.h>
#include <phy_utils_pmu.h>
#include <phy_stf.h>

#include "wlc_phy_radio.h"
#include "wlc_phyreg_ac.h"
#include "wlc_radioreg_20691.h"
#include "wlc_radioreg_20693.h"
#include "wlc_radioreg_20694.h"
#include "wlc_radioreg_20695.h"
#include "wlc_radioreg_20696.h"


#include <phy_rstr.h>
#include <phy_utils_var.h>

uint8 wlc_phy_vbat_monitoring_algorithm_decision
	(phy_info_t * pi);
/* module private states */
struct phy_ac_temp_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_temp_info_t *ti;
	acphy_tempsense_radioregs_t *ac_tempsense_radioregs_orig;
	acphy_tempsense_phyregs_t   *ac_tempsense_phyregs_orig;
	int16 current_temperature;
	uint8 last_regulated;
/* add other variable size variables here at the end */
};

/* local functions */
static uint16 phy_ac_temp_throttle(phy_type_temp_ctx_t *ctx);
static void phy_ac_temp_nvram_attach(phy_ac_temp_info_t *tempi);
static void phy_ac_temp_upd_gain(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj);
static void phy_ac_temp_upd_gain_cal(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj);
static int phy_ac_temp_get_(phy_type_temp_ctx_t *ctx);
static uint8 phy_ac_temp_get_first_actv_core(uint8 coremask);
static int16 phy_ac_temp_do_tempsense(phy_type_temp_ctx_t *ctx);

/* Register/unregister ACPHY specific implementation to common layer. */
phy_ac_temp_info_t *
BCMATTACHFN(phy_ac_temp_register_impl)(phy_info_t *pi, phy_ac_info_t *aci, phy_temp_info_t *ti)
{
	phy_ac_temp_info_t *temp_info;
	phy_type_temp_fns_t fns;
	acphy_tempsense_radioregs_t *ac_tempsense_radioregs_orig = NULL;
	acphy_tempsense_phyregs_t   *ac_tempsense_phyregs_orig = NULL;
	phy_txcore_temp_t *temp = NULL;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((temp_info = phy_malloc(pi, sizeof(phy_ac_temp_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((ac_tempsense_radioregs_orig =
		phy_malloc(pi, sizeof(acphy_tempsense_radioregs_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc ac_tempsense_radioregs_orig failed\n", __FUNCTION__));
		goto fail;
	}

	if ((ac_tempsense_phyregs_orig =
		phy_malloc(pi, sizeof(acphy_tempsense_phyregs_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc ac_tempsense_phyregs_orig failed\n", __FUNCTION__));
		goto fail;
	}

	temp_info->pi = pi;
	temp_info->aci = aci;
	temp_info->ti = ti;
	temp_info->ac_tempsense_radioregs_orig = ac_tempsense_radioregs_orig;
	temp_info->ac_tempsense_phyregs_orig = ac_tempsense_phyregs_orig;

	/* Read srom params from nvram */
	phy_ac_temp_nvram_attach(temp_info);

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.throt = phy_ac_temp_throttle;
	fns.get = phy_ac_temp_get_;
	fns.upd_gain = phy_ac_temp_upd_gain;
	fns.upd_gain_cal = phy_ac_temp_upd_gain_cal;
	fns.do_tempsense = phy_ac_temp_do_tempsense;
	fns.ctx = temp_info;

	/* Initialize any common layer variable */
	temp = phy_temp_get_st(ti);
	ASSERT(temp);
	if ((temp->disable_temp == 0) || (temp->disable_temp == 0xff)) {
		temp->disable_temp = ACPHY_CHAIN_TX_DISABLE_TEMP;
	}
#if defined(BCM94360X51) && defined(BCM94360X52C)
	if ((CHIPID(pi->sh->chip) == BCM4360_CHIP_ID) &&
		((pi->sh->boardtype == BCM94360X51) ||
		(pi->sh->boardtype == BCM94360X51P3) ||
		(pi->sh->boardtype == BCM94360X52C))) {
		temp->disable_temp = ACPHY_CHAIN_TX_DISABLE_TEMP_4360;
	}
#endif /* BCM94360X51 && BCM94360X52C */

	phy_temp_register_impl(ti, &fns);

	return temp_info;
fail:
	if (ac_tempsense_phyregs_orig != NULL)
		phy_mfree(pi, ac_tempsense_phyregs_orig, sizeof(acphy_tempsense_phyregs_t));
	if (ac_tempsense_radioregs_orig != NULL)
		phy_mfree(pi, ac_tempsense_radioregs_orig, sizeof(acphy_tempsense_radioregs_t));

	if (temp_info != NULL)
		phy_mfree(pi, temp_info, sizeof(phy_ac_temp_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_temp_unregister_impl)(phy_ac_temp_info_t *temp_info)
{
	phy_info_t *pi;
	phy_temp_info_t *ti;

	ASSERT(temp_info);
	pi = temp_info->pi;
	ti = temp_info->ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_temp_unregister_impl(ti);

	phy_mfree(pi, temp_info->ac_tempsense_phyregs_orig, sizeof(acphy_tempsense_phyregs_t));

	phy_mfree(pi, temp_info->ac_tempsense_radioregs_orig, sizeof(acphy_tempsense_radioregs_t));

	phy_mfree(pi, temp_info, sizeof(phy_ac_temp_info_t));
}


static uint16
phy_ac_temp_throttle(phy_type_temp_ctx_t *ctx)
{
	phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
	phy_temp_info_t *ti = info->ti;
	phy_info_t *pi = info->pi;
	phy_txcore_temp_t *temp;

	uint8 txcore_shutdown_lut[] = {1, 1, 2, 1, 4, 1, 2, 1,
			8, 8, 2, 2, 4, 4, 4, 13};
	uint8 txcore_shutdown_lut_4366[] = {1, 1, 2, 1, 4, 1, 2, 1,
			8, 8, 2, 9, 4, 9, 4, 13};
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	uint8 new_phytxchain;
	int16 currtemp, delta_temp = 0;
	uint16 duty_cycle_active_chains;

	PHY_TRACE(("%s\n", __FUNCTION__));
	/* No need to do tempsense/Throttle/Cal when not associated
	  * and returning hw_txchain to avoid throttling in upper layer
	  */
	if ((ACMAJORREV_4(pi->pubpi->phy_rev) ||
		(ACMAJORREV_5(pi->pubpi->phy_rev) && ACMINORREV_2(pi))) && PUB_NOT_ASSOC(pi))
		return stf_shdata->hw_phytxchain;

	temp = phy_temp_get_st(ti);

	ASSERT(temp != NULL);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20691_ID))
		return 0;


	if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		PHY_INFORM(("%s: TBD for BCM20694_ID\n", __FUNCTION__));
		return 0;
	}

	/* Throttling is not required */
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		return 0;
	}

	ASSERT(stf_shdata->phytxchain);

	currtemp = wlc_phy_tempsense_acphy(pi);
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		/* Checking temperature for every 10sec and
		     Triggering calibration if there is a temperature
		     change greater than phycal_tempdelta.
		     This is needed to address Tx throughput degradation
		     seen on 4349 MIMO mode.
		  */
		delta_temp = ((currtemp > pi->cal_info->last_cal_temp) ?
			(currtemp - pi->cal_info->last_cal_temp) :
			(pi->cal_info->last_cal_temp - currtemp));
		/* Before triggering the cal make sure no cals are pending */
		if ((temp->phycal_tempdelta) && (delta_temp > temp->phycal_tempdelta) &&
			(!PHY_PERICAL_MPHASE_PENDING(pi))) {
			pi->cal_info->last_cal_temp = currtemp;
			wlc_phy_cals_acphy(pi->u.pi_acphy->calmgri, PHY_PERICAL_UNDEF,
					PHY_CAL_SEARCHMODE_RESTART);
		}
	}

#ifdef BCMDBG
	if (pi->tempsense_override)
		currtemp = pi->tempsense_override;
#endif

	if (currtemp >= temp->disable_temp) {
		if (!ACMAJORREV_33(pi->pubpi->phy_rev))
			temp->heatedup = TRUE;
		if (phy_get_phymode(pi) == PHYMODE_MIMO) {
			temp->duty_cycle = 100;
			temp->duty_cycle_throttle_state = 0;
			temp->duty_cycle_throttle_depth = 10;
			if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
				ACMAJORREV_33(pi->pubpi->phy_rev)) {
				new_phytxchain = txcore_shutdown_lut_4366[stf_shdata->phytxchain];
			} else {
				new_phytxchain = txcore_shutdown_lut[stf_shdata->phytxchain];
			}
			temp->bitmap = ((stf_shdata->phyrxchain << 4) | new_phytxchain);
		} else if(DCT_ENAB(pi)){ /* RSDB */
			if (temp->duty_cycle_throttle_state < temp->duty_cycle_throttle_depth) {
				if (!temp->duty_cycle_throttle_state) {
					/* First time reduce duty cycle by 20per */
					temp->duty_cycle -= 20;
					temp->duty_cycle_throttle_state += 4;
				} else {
					temp->duty_cycle -= 10;
					temp->duty_cycle_throttle_state += 2;
				}
			} else if (temp->duty_cycle_throttle_state ==
				temp->duty_cycle_throttle_depth) {
				/* Temperature is not down after duty cycle throttling */
			}
		}
	} else {
		if (currtemp <= temp->enable_temp) {
			if ((phy_get_phymode(pi) == PHYMODE_RSDB) && DCT_ENAB(pi)) {
				if (temp->duty_cycle_throttle_state) {
					temp->duty_cycle += 5;
					temp->duty_cycle_throttle_state--;
					if (!temp->duty_cycle_throttle_state) {
						new_phytxchain = stf_shdata->hw_phytxchain;
						if (!ACMAJORREV_33(pi->pubpi->phy_rev))
							temp->heatedup = FALSE;
						temp->bitmap = ((stf_shdata->phyrxchain << 4) |
								new_phytxchain);
					}
				}
			} else {
				new_phytxchain = stf_shdata->hw_phytxchain;
				if (!ACMAJORREV_33(pi->pubpi->phy_rev))
					temp->heatedup = FALSE;
				temp->bitmap = ((stf_shdata->phyrxchain << 4) | new_phytxchain);
				temp->duty_cycle = 100;
				temp->duty_cycle_throttle_depth = 10;
				temp->duty_cycle_throttle_state = 0;
			}
		}
	}

	duty_cycle_active_chains = temp->duty_cycle << 8 | temp->bitmap;
	return duty_cycle_active_chains;
}

/* read the current temperature */
int
phy_ac_temp_get(phy_ac_temp_info_t *tempi)
{
	return tempi->current_temperature;
}

static int
phy_ac_temp_get_(phy_type_temp_ctx_t *ctx)
{
	phy_ac_temp_info_t *tempi = (phy_ac_temp_info_t *)ctx;
	return phy_ac_temp_get(tempi);
}

static void
BCMATTACHFN(phy_ac_temp_nvram_attach)(phy_ac_temp_info_t *tempi)
{
	phy_info_t *pi = tempi->pi;
	/* read in temperature at calibration time */
	tempi->current_temperature = pi->srom_rawtempsense;
	pi->sromi->sw_txchain_mask =
		(uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_sw_txchain_mask, 0);
	pi->sromi->sw_rxchain_mask =
		(uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_sw_rxchain_mask, 0);
	pi->u.pi_acphy->sromi->rpcal2g = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal2g, 0);
	pi->u.pi_acphy->sromi->rpcal5gb0 =
	(uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb0, 0);
	pi->u.pi_acphy->sromi->rpcal5gb1 =
	(uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb1, 0);
	pi->u.pi_acphy->sromi->rpcal5gb2 =
	(uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb2, 0);
	pi->u.pi_acphy->sromi->rpcal5gb3 =
	(uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb3, 0);
	pi->sromi->rpcal2gcore3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal2gcore3, 0);
	pi->sromi->rpcal5gb0core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb0core3, 0);
	pi->sromi->rpcal5gb1core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb1core3, 0);
	pi->sromi->rpcal5gb2core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb2core3, 0);
	pi->sromi->rpcal5gb3core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb3core3, 0);
}
/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */
static void wlc_phy_tempsense_radio_setup_acphy(phy_info_t *pi, uint16 Av, uint16 Vmid);
static void wlc_phy_tempsense_radio_cleanup_acphy(phy_info_t *pi);
static void wlc_phy_tempsense_phy_setup_acphy(phy_info_t *pi);
static void wlc_phy_tempsense_phy_cleanup_acphy(phy_info_t *pi);
static int32 wlc_phy_tempsense_paldosense_phy_setup_acphy_tiny(phy_info_t *pi,
	uint8 tempsense_paldosense, uint8 core);
static int32 wlc_phy_tempsense_paldosense_radio_setup_acphy_tiny(phy_info_t *pi, uint16 Av,
	uint16 Vmid, uint8 tempsense_paldosense, uint8 core);
static int32 wlc_phy_tempsense_poll_adc_war_tiny(phy_info_t *pi,
	bool init_adc_inside, int32 *measured_values, uint8 core);
static int32 wlc_phy_tempsense_poll_samps_tiny(phy_info_t *pi,
	uint16 samples, bool init_adc_inside, uint8 core);
static int32 wlc_phy_tempsense_gpiosel_tiny(phy_info_t *pi,
	uint16 sel, uint8 word_swap);
static int32 wlc_phy_tempsense_radio_swap_tiny(phy_info_t *pi,
	acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static int32 wlc_phy_tempsense_paldosense_phy_cleanup_acphy_tiny(phy_info_t *pi, uint8 core);
static int32 wlc_phy_tempsense_paldosense_radio_cleanup_acphy_tiny(phy_info_t *pi,
	uint8 tempsense_paldosense, uint8 core);
static uint8 wlc_phy_vbat_monitoring_algorithm_acphy(phy_info_t *pi);
static int16 wlc_phy_tempsense_paldosense_acphy_tiny
	(phy_info_t *pi, uint8 tempsense_paldosense);
static int16 wlc_phy_tempsense_paldosense_acphy_28nm
	(phy_ac_temp_info_t *ti, uint8 tempsense_paldosense);
static int32 wlc_phy_tempsense_paldosense_phy_setup_acphy_28nm(phy_ac_temp_info_t *ti,
	uint8 tempsense_paldosense);
static int32 wlc_phy_tempsense_paldosense_radio_setup_acphy_28nm(phy_ac_temp_info_t *ti, uint16 Av,
	uint16 Vmid, uint8 tempsense_paldosense);
static int32 wlc_phy_tempsense_paldosense_phy_cleanup_acphy_28nm(phy_ac_temp_info_t *ti);
static int32 wlc_phy_tempsense_paldosense_radio_cleanup_acphy_28nm(phy_ac_temp_info_t *ti);
static int32 wlc_phy_tempsense_radio_swap_28nm(phy_info_t *pi,
	acphy_tempsense_cfg_opt_t type, uint8 swap);
static int32 wlc_phy_tempsense_poll_adc_war_28nm(phy_info_t *pi,
	bool init_adc_inside, int32 *measured_values);
static int16 wlc_phy_tempsense_vbatsense_acphy_20694
	(phy_info_t *pi, uint8 tempsense_vbatsense);
static int16 wlc_phy_tempsense_vbatsense_acphy_20696
	(phy_info_t *pi, uint8 tempsense_vbatsense);
static int32 wlc_phy_tempsense_poll_adc_war_20694(phy_info_t *pi,
	bool init_adc_inside, int16 *measured_values, uint8 core);
static int32 wlc_phy_tempsense_poll_adc_war_20696(phy_info_t *pi,
	bool init_adc_inside, int16 *measured_values, uint8 core);
static int32 wlc_phy_tempsense_radio_swap_20694(phy_info_t *pi,
	acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static int32 wlc_phy_tempsense_radio_swap_20696(phy_info_t *pi,
	acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static void
wlc_phy_tempsense_radio_setup_acphy_20694(phy_info_t *pi, uint16 Av, uint16 Vmid);
static void
wlc_phy_tempsense_radio_cleanup_acphy_20694(phy_info_t *pi);
static void
wlc_phy_tempsense_radio_setup_acphy_20696(phy_info_t *pi, uint16 Av, uint16 Vmid);
static void
wlc_phy_tempsense_radio_cleanup_acphy_20696(phy_info_t *pi);
static void

wlc_phy_tempsense_radio_setup_acphy_20694(phy_info_t *pi, uint16 Av, uint16 Vmid)
{

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	tempsense_radioregs_20694_t *porig =
		&(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs_20694);

	BCM_REFERENCE(stf_shdata);

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		MOD_RADIO_REG_20694(pi, RF, IQCAL_OVR1, core, ovr_iqcal_PU_loopback_bias, 0x1);
		MOD_RADIO_REG_20694(pi, RF, IQCAL_CFG5, core, loopback_bias_pu, 0x1);

		porig->testbuf_ovr1[core] = READ_RADIO_REG_20694(pi, RF, TESTBUF_OVR1, core);
		porig->testbuf_cfg1[core] = READ_RADIO_REG_20694(pi, RF, TESTBUF_CFG1, core);
		porig->tempsense_ovr1[core] = READ_RADIO_REG_20694(pi, RF, TEMPSENSE_OVR1, core);
		porig->tempsense_cfg[core] = READ_RADIO_REG_20694(pi, RF, TEMPSENSE_CFG, core);
		porig->auxpga_ovr1[core] = READ_RADIO_REG_20694(pi, RF, AUXPGA_OVR1, core);
		porig->auxpga_cfg1[core] = READ_RADIO_REG_20694(pi, RF, AUXPGA_CFG1, core);
		porig->auxpga_vmid[core] = READ_RADIO_REG_20694(pi, RF, AUXPGA_VMID, core);
		porig->tia_cfg1_ovr[core] = READ_RADIO_REG_20694(pi, RF, TIA_CFG1_OVR, core);
		porig->tia_reg7[core] = READ_RADIO_REG_20694(pi, RF, TIA_REG7, core);
		porig->lpf_ovr1[core] = READ_RADIO_REG_20694(pi, RF, LPF_OVR1, core);
		porig->lpf_ovr2[core] = READ_RADIO_REG_20694(pi, RF, LPF_OVR2, core);
		porig->lpf_reg7[core] = READ_RADIO_REG_20694(pi, RF, LPF_REG7, core);
		porig->iqcal_cfg4[core] = READ_RADIO_REG_20694(pi, RF, IQCAL_CFG4, core);
		porig->iqcal_cfg5[core] = READ_RADIO_REG_20694(pi, RF, IQCAL_CFG5, core);
		porig->iqcal_ovr1[core] = READ_RADIO_REG_20694(pi, RF, IQCAL_OVR1, core);

		if (core == 0) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_OVR1, 0, ovr_testbuf_PU, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_CFG1, 0, testbuf_PU, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_CFG1, 0, testbuf_GPIO_EN, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_OVR1, 0, ovr_auxpga_i_pu, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_CFG1, 0, auxpga_i_pu, 0x1)

			// setup Aux path
			MOD_RADIO_REG_20694_ENTRY(pi, RF, IQCAL_CFG4, 0, iqcal2adc, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, IQCAL_CFG4, 0, auxpga2adc, 0x1)

			// setup tempsense
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TEMPSENSE_OVR1, 0, ovr_tempsense_pu, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TEMPSENSE_CFG, 0, tempsense_pu, 0x1)

			// setup sel_test_port
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_OVR1, 0,
				ovr_auxpga_i_sel_input, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_CFG1, 0,
				auxpga_i_sel_input, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_OVR1, 0,
				ovr_testbuf_sel_test_port, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_CFG1, 0,
				testbuf_sel_test_port, 0x1)

			// Mux AUX path to ADC
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_OVR2, 0, ovr_lpf_sw_bq1_adc, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_OVR2, 0, ovr_lpf_sw_aux_adc, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_OVR1, 0, ovr_lpf_sw_bq2_adc, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_REG7, 0, lpf_sw_bq1_adc, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_REG7, 0, lpf_sw_bq2_adc, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_REG7, 0, lpf_sw_aux_adc, 0x1)

			// Turn off TIA
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TIA_CFG1_OVR, 0, ovr_tia_pu, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TIA_REG7, 0, tia_bq_pu, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
		} else {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_OVR1, 1, ovr_testbuf_PU, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_CFG1, 1, testbuf_PU, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_CFG1, 1, testbuf_GPIO_EN, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_OVR1, 1, ovr_auxpga_i_pu, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_CFG1, 1, auxpga_i_pu, 0x1)

			// setup Aux path
			MOD_RADIO_REG_20694_ENTRY(pi, RF, IQCAL_CFG4, 1, iqcal2adc, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, IQCAL_CFG4, 1, auxpga2adc, 0x1)

			// setup tempsense
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TEMPSENSE_OVR1, 1, ovr_tempsense_pu, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TEMPSENSE_CFG, 1, tempsense_pu, 0x1)

			// setup sel_test_port
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_OVR1, 1,
				ovr_auxpga_i_sel_input, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, AUXPGA_CFG1, 1,
				auxpga_i_sel_input, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_OVR1, 1,
				ovr_testbuf_sel_test_port, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TESTBUF_CFG1, 1,
				testbuf_sel_test_port, 0x1)

			// Mux AUX path to ADC
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_OVR2, 1, ovr_lpf_sw_bq1_adc, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_OVR2, 1, ovr_lpf_sw_aux_adc, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_OVR1, 1, ovr_lpf_sw_bq2_adc, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_REG7, 1, lpf_sw_bq1_adc, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_REG7, 1, lpf_sw_bq2_adc, 0x0)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, LPF_REG7, 1, lpf_sw_aux_adc, 0x1)

			// Turn off TIA
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TIA_CFG1_OVR, 1, ovr_tia_pu, 0x1)
			MOD_RADIO_REG_20694_ENTRY(pi, RF, TIA_REG7, 1, tia_bq_pu, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
		}

		MOD_RADIO_REG_20694(pi, RF, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1);
		WRITE_RADIO_REG_20694(pi, RF, AUXPGA_VMID, core, Vmid);
		MOD_RADIO_REG_20694(pi, RF, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1);
		MOD_RADIO_REG_20694(pi, RF, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);
		MOD_RADIO_REG_20694(pi, RF, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0);
	}
	return;
}


static void
wlc_phy_tempsense_radio_setup_acphy_20696(phy_info_t *pi, uint16 Av, uint16 Vmid)
{

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	phy_ac_reg_cache_save(pi_ac, RADIOREGS_TEMPSENSE_VBAT);
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		//should the pu be after or before the save&restore
		//it was first before - now moved it after
		MOD_RADIO_REG_20696(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1);
		MOD_RADIO_REG_20696(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1);
		MOD_RADIO_REG_20696(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x1);
		MOD_RADIO_REG_20696(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1);
		MOD_RADIO_REG_20696(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1);
		MOD_RADIO_REG_20696(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0x0);
		// setup Aux path
		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1);
		WRITE_RADIO_REG_20696(pi, AUXPGA_VMID, core, Vmid);
		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0);
		// setup tempsense
		MOD_RADIO_REG_20696(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0x1);
		MOD_RADIO_REG_20696(pi, TEMPSENSE_CFG, core, tempsense_pu, 0x1);
		// setup sel_test_port
		MOD_RADIO_REG_20696(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1);
		MOD_RADIO_REG_20696(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1);
		//Setup Aux Path
		MOD_RADIO_REG_20696(pi, IQCAL_CFG4, core, iqcal2adc, 0x0);
		MOD_RADIO_REG_20696(pi, IQCAL_CFG4, core, auxpga2adc, 0x1);
		// Mux AUX path to ADC
		MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
		MOD_RADIO_REG_20696(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);
		MOD_RADIO_REG_20696(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
		MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
		MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
		MOD_RADIO_REG_20696(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1);
		// Turn off TIA
		MOD_RADIO_REG_20696(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1);
		MOD_RADIO_REG_20696(pi, TIA_REG7, core, tia_pu, 0x0);
	}
	return;
}


static void
wlc_phy_tempsense_radio_cleanup_acphy_20694(phy_info_t *pi)
{

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	tempsense_radioregs_20694_t *porig =
		&(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs_20694);

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		WRITE_RADIO_REG_20694(pi, RF, TESTBUF_OVR1, core, porig->testbuf_ovr1[core]);
		WRITE_RADIO_REG_20694(pi, RF, TESTBUF_CFG1, core, porig->testbuf_cfg1[core]);
		WRITE_RADIO_REG_20694(pi, RF, TEMPSENSE_OVR1, core, porig->tempsense_ovr1[core]);
		WRITE_RADIO_REG_20694(pi, RF, TEMPSENSE_CFG, core, porig->tempsense_cfg[core]);
		WRITE_RADIO_REG_20694(pi, RF, AUXPGA_OVR1, core, porig->auxpga_ovr1[core]);
		WRITE_RADIO_REG_20694(pi, RF, AUXPGA_CFG1, core, porig->auxpga_cfg1[core]);
		WRITE_RADIO_REG_20694(pi, RF, AUXPGA_VMID, core, porig->auxpga_vmid[core]);
		WRITE_RADIO_REG_20694(pi, RF, TIA_CFG1_OVR, core, porig->tia_cfg1_ovr[core]);
		WRITE_RADIO_REG_20694(pi, RF, TIA_REG7, core, porig->tia_reg7[core]);
		WRITE_RADIO_REG_20694(pi, RF, LPF_OVR1, core, porig->lpf_ovr1[core]);
		WRITE_RADIO_REG_20694(pi, RF, LPF_OVR2, core, porig->lpf_ovr2[core]);
		WRITE_RADIO_REG_20694(pi, RF, LPF_REG7, core, porig->lpf_reg7[core]);
		WRITE_RADIO_REG_20694(pi, RF, IQCAL_CFG4, core, porig->iqcal_cfg4[core]);
		WRITE_RADIO_REG_20694(pi, RF, IQCAL_CFG5, core, porig->iqcal_cfg5[core]);
		WRITE_RADIO_REG_20694(pi, RF, IQCAL_OVR1, core, porig->iqcal_ovr1[core]);
	}
	return;
}

static void
wlc_phy_tempsense_radio_cleanup_acphy_20696(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	/* restore radio config back */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_TEMPSENSE_VBAT);

	return;
}


static void
wlc_phy_tempsense_radio_setup_acphy(phy_info_t *pi, uint16 Av, uint16 Vmid)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	tempsense_radioregs_t *porig =
		&(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs);
	uint8 core;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	BCM_REFERENCE(stf_shdata);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		/* Reg conflict with 2069 rev 16 */
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0)
			porig->OVR18[core]         = READ_RADIO_REGC(pi, RF, OVR18, core);
		else
			porig->OVR19[core]         = READ_RADIO_REGC(pi, RF, GE16_OVR19, core);
		porig->tempsense_cfg[core] = READ_RADIO_REGC(pi, RF, TEMPSENSE_CFG, core);
		porig->OVR5[core]          = READ_RADIO_REGC(pi, RF, OVR5, core);
		porig->testbuf_cfg1[core]  = READ_RADIO_REGC(pi, RF, TESTBUF_CFG1, core);
		porig->OVR3[core]          = READ_RADIO_REGC(pi, RF, OVR3, core);
		porig->auxpga_cfg1[core]   = READ_RADIO_REGC(pi, RF, AUXPGA_CFG1, core);
		porig->auxpga_vmid[core]   = READ_RADIO_REGC(pi, RF, AUXPGA_VMID, core);

		MOD_RADIO_REGC(pi, OVR5, core, ovr_tempsense_pu, 0x1);
		MOD_RADIO_REGC(pi, TEMPSENSE_CFG, core, pu, 0x1);
		MOD_RADIO_REGC(pi, OVR5, core, ovr_testbuf_PU, 0x1);
		MOD_RADIO_REGC(pi, TESTBUF_CFG1, core, PU, 0x1);
		MOD_RADIO_REGC(pi, TESTBUF_CFG1, core, GPIO_EN, 0x0);
		MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_auxpga_i_sel_vmid, 0x1);
		MOD_RADIO_REGC(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
		MOD_RADIO_REGC(pi, OVR3, core, ovr_auxpga_i_sel_gain, 0x1);
		MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);

		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
			MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0x0);
			/* This bit is supposed to be controlled by phy direct control line.
			 * Please check: http://jira.broadcom.com/browse/HW11ACRADIO-45
			 */
			MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1);
		}
	}
}

static void
wlc_phy_tempsense_radio_cleanup_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	tempsense_radioregs_t *porig =
		&(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs);
	uint8 core;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	BCM_REFERENCE(stf_shdata);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		/* Reg conflict with 2069 rev 16 */
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0)
			phy_utils_write_radioreg(pi, RF_2069_OVR18(core), porig->OVR18[core]);
		else
			phy_utils_write_radioreg(pi, RF_2069_GE16_OVR19(core), porig->OVR19[core]);
		phy_utils_write_radioreg(pi, RF_2069_TEMPSENSE_CFG(core),
			porig->tempsense_cfg[core]);
		phy_utils_write_radioreg(pi, RF_2069_OVR5(core), porig->OVR5[core]);
		phy_utils_write_radioreg(pi, RF_2069_TESTBUF_CFG1(core), porig->testbuf_cfg1[core]);
		phy_utils_write_radioreg(pi, RF_2069_OVR3(core), porig->OVR3[core]);
		phy_utils_write_radioreg(pi, RF_2069_AUXPGA_CFG1(core), porig->auxpga_cfg1[core]);
		phy_utils_write_radioreg(pi, RF_2069_AUXPGA_VMID(core), porig->auxpga_vmid[core]);
	}
}

static uint8
phy_ac_temp_get_first_actv_core(uint8 coremask)
{
	/* Get the first active core */
	switch (coremask & (256-coremask)) {
		case 1:
			return 0;
		case 2:
			return 1;
		case 4:
			return 2;
		case 8:
			return 3;
		default:
			return 0;
	}
}

static void
wlc_phy_tempsense_phy_setup_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);
	uint16 sdadc_config, mask;
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	if (CHSPEC_IS80(pi->radio_chanspec) || PHY_AS_80P80(pi, pi->radio_chanspec)) {
		sdadc_config = sdadc_cfg80;
	} else if (CHSPEC_IS160(pi->radio_chanspec)) {
		sdadc_config = sdadc_cfg80;
		ASSERT(0);
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		if (pi->sdadc_config_override)
			sdadc_config = sdadc_cfg40hs;
		else
		sdadc_config = sdadc_cfg40;
	} else {
		sdadc_config = sdadc_cfg20;
	}

	porig->RxFeCtrl1 = READ_PHYREG(pi, RxFeCtrl1);

	ACPHY_REG_LIST_START
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq0, 0)
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq1, 0)
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq2, 0)
	ACPHY_REG_LIST_EXECUTE(pi);

	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		mask = ACPHY_RfctrlIntc0_override_tr_sw_MASK(rev) |
			ACPHY_RfctrlIntc0_tr_sw_tx_pu_MASK(rev);
		porig->RfctrlIntc[core] = phy_utils_read_phyreg(pi,
			ACPHYREGCE(pi, RfctrlIntc, core));
		phy_utils_write_phyreg(pi, ACPHYREGCE(pi,
			RfctrlIntc, core), mask);  /* elna off */

		porig->RfctrlOverrideAuxTssi[core]  = READ_PHYREGCE(pi, RfctrlOverrideAuxTssi,
		                                                    core);
		porig->RfctrlCoreAuxTssi1[core]     = READ_PHYREGCE(pi, RfctrlCoreAuxTssi1, core);
		porig->RfctrlOverrideRxPus[core]    = READ_PHYREGCE(pi, RfctrlOverrideRxPus, core);
		porig->RfctrlCoreRxPus[core]        = READ_PHYREGCE(pi, RfctrlCoreRxPus, core);
		porig->RfctrlOverrideTxPus[core]    = READ_PHYREGCE(pi, RfctrlOverrideTxPus, core);
		porig->RfctrlCoreTxPus[core]        = READ_PHYREGCE(pi, RfctrlCoreTxPus, core);
		porig->RfctrlOverrideLpfSwtch[core] = READ_PHYREGCE(pi, RfctrlOverrideLpfSwtch,
		                                                    core);
		porig->RfctrlCoreLpfSwtch[core]     = READ_PHYREGCE(pi, RfctrlCoreLpfSwtch, core);
		porig->RfctrlOverrideAfeCfg[core]   = READ_PHYREGCE(pi, RfctrlOverrideAfeCfg, core);
		porig->RfctrlCoreAfeCfg1[core]      = READ_PHYREGCE(pi, RfctrlCoreAfeCfg1, core);
		porig->RfctrlCoreAfeCfg2[core]      = READ_PHYREGCE(pi, RfctrlCoreAfeCfg2, core);
		porig->RfctrlOverrideGains[core]    = READ_PHYREGCE(pi, RfctrlOverrideGains, core);
		porig->RfctrlCoreLpfGain[core]      = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);

		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, amux_sel_port, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, amux_sel_port, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, afe_iqadc_aux_en, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, afe_iqadc_aux_en, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideRxPus,    core, lpf_pu_dc, 1);
		MOD_PHYREGCE(pi, RfctrlCoreRxPus,        core, lpf_pu_dc, 0);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus,    core, lpf_bq1_pu, 1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,        core, lpf_bq1_pu, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus,    core, lpf_bq2_pu, 1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,        core, lpf_bq2_pu, 0);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus,    core, lpf_pu, 1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,        core, lpf_pu, 1);

		WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core,         0x154);
		WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core,     0x3ff);
		if (!ACMAJORREV_37(pi->pubpi->phy_rev) && (!ACMAJORREV_40(pi->pubpi->phy_rev))) {
			wlc_phy_txcal_phy_setup_acphy_core_sd_adc(pi, core, sdadc_config);
		}

		MOD_PHYREGCE(pi, RfctrlCoreLpfGain,      core, lpf_bq1_gain, 0);
		MOD_PHYREGCE(pi, RfctrlOverrideGains,    core, lpf_bq1_gain, 1);
	}
	//need to enable dc bypass for 7271
	if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, DcFiltAddress, dcBypass, 1);
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq3, 0);
	}
}

static void
wlc_phy_tempsense_phy_cleanup_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	WRITE_PHYREG(pi, RxFeCtrl1, porig->RxFeCtrl1);

	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		WRITE_PHYREGCE(pi, RfctrlIntc, core, porig->RfctrlIntc[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, porig->RfctrlOverrideAuxTssi[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, porig->RfctrlCoreAuxTssi1[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideRxPus, core, porig->RfctrlOverrideRxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreRxPus, core, porig->RfctrlCoreRxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, porig->RfctrlOverrideTxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, porig->RfctrlCoreTxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core,
		              porig->RfctrlOverrideLpfSwtch[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, porig->RfctrlCoreLpfSwtch[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, porig->RfctrlOverrideAfeCfg[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, porig->RfctrlCoreAfeCfg1[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, porig->RfctrlCoreAfeCfg2[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideGains, core, porig->RfctrlOverrideGains[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core, porig->RfctrlCoreLpfGain[core]);
	}
	//need to disable dc bypass for 7271
	if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, DcFiltAddress, dcBypass, 0);
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq3, 1);
	}
}

static int32
wlc_phy_tempsense_paldosense_phy_setup_acphy_tiny(phy_info_t *pi, uint8 tempsense_paldosense
, uint8 core)
{
	/* # this proc is used to configure the tempsense phy settings. */
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);


	/* Applicable # foreach core */
	porig->RfctrlOverrideAuxTssi[core] = READ_PHYREGCE(pi, RfctrlOverrideAuxTssi, core);
	porig->RfctrlCoreAuxTssi1[core]    = READ_PHYREGCE(pi, RfctrlCoreAuxTssi1, core);
	porig->RfctrlOverrideRxPus[core]   = READ_PHYREGCE(pi, RfctrlOverrideRxPus, core);
	porig->RfctrlCoreRxPus[core]       = READ_PHYREGCE(pi, RfctrlCoreRxPus, core);
	porig->RxFeCtrl1                   = READ_PHYREG(pi, RxFeCtrl1);
	porig->RfctrlOverrideTxPus[core]   = READ_PHYREGCE(pi, RfctrlOverrideTxPus, core);
	porig->RfctrlCoreTxPus[core]       = READ_PHYREGCE(pi, RfctrlCoreTxPus, core);
	porig->RfctrlOverrideGains[core]   = READ_PHYREGCE(pi, RfctrlOverrideGains, core);
	porig->RfctrlCoreLpfGain[core]     = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);
	porig->RxSdFeConfig1               = READ_PHYREG(pi, RxSdFeConfig1);
	porig->RxSdFeConfig6               = READ_PHYREG(pi, RxSdFeConfig6);
	porig->RfctrlOverrideLpfSwtch[core] = READ_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core);
	porig->RfctrlCoreLpfSwtch[core]    = READ_PHYREGCE(pi, RfctrlCoreLpfSwtch, core);
	porig->RfctrlOverrideAfeCfg[core]  = READ_PHYREGCE(pi, RfctrlOverrideAfeCfg, core);
	porig->RfctrlCoreAfeCfg1[core]     = READ_PHYREGCE(pi, RfctrlCoreAfeCfg1, core);
	porig->RfctrlCoreAfeCfg2[core]     = READ_PHYREGCE(pi, RfctrlCoreAfeCfg2, core);

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		// Additional settings for 4365 core3 GPIO WAR
		if (core == 3) {
			MOD_PHYREG(pi, DcFiltAddress,		 dcBypass,	   1);
			MOD_PHYREG(pi, RfctrlCoreRXGAIN13,	 rxgain_dvga,  0);
			MOD_PHYREG(pi, RfctrlCoreLpfGain3,	 lpf_bq2_gain, 0);
			MOD_PHYREG(pi, RfctrlOverrideGains3, rxgain,	   1);
			MOD_PHYREG(pi, RfctrlOverrideGains3, lpf_bq2_gain, 1);
		}

		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, amux_sel_port, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,	 core, amux_sel_port, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, afe_iqadc_aux_en, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,	 core, afe_iqadc_aux_en, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideRxPus,	 core, lpf_pu_dc, 1);
		MOD_PHYREGCE(pi, RfctrlCoreRxPus,		 core, lpf_pu_dc, 0);
		WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core,		 0x154);
		WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core,	 0x3ff);

		/* Power down LNAs - isolation of TIA power down is insufficient */
		/* at high signal power */
		MOD_PHYREGCE(pi, RfctrlOverrideRxPus,	 core, rxrf_lna1_pwrup, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideRxPus,	 core, rxrf_lna1_5G_pwrup, 1);
		MOD_PHYREGCE(pi, RfctrlCoreRxPus,	 core, rxrf_lna1_pwrup, 0);
		MOD_PHYREGCE(pi, RfctrlCoreRxPus,	 core, rxrf_lna1_5G_pwrup, 0);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus,	 core, lpf_bq1_pu, 1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,		 core, lpf_bq1_pu, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus,	 core, lpf_bq2_pu, 1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,		 core, lpf_bq2_pu, 0);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus,	 core, lpf_pu, 1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,		 core, lpf_pu, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideGains,	 core, lpf_bq1_gain, 1);
		MOD_PHYREGCE(pi, RfctrlCoreLpfGain,		 core, lpf_bq1_gain, 0);

		MOD_PHYREG(pi, RxFeCtrl1, swap_iq0, 0);
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq1, 0);
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq2, 0);
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq3, 0);
		MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0);
		MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
		MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0,
		READ_PHYREGFLD(pi, RxSdFeConfig1, farrow_rshift_tx));
	} else {
		ACPHY_REG_LIST_START
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideAuxTssi,  0, afe_iqadc_aux_en, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreAuxTssi1,	 0, afe_iqadc_aux_en, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,	 0, lpf_pu_dc, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,		 0, lpf_pu_dc, 0)
			MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq0, 0)
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_bq1_pu, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_bq1_pu, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_bq2_pu, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_bq2_pu, 0)
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_pu, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_pu, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideGains,    0, lpf_bq1_gain, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreLpfGain,      0, lpf_bq1_gain, 0)

			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideAuxTssi,  0, amux_sel_port, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
			MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,	 0, amux_sel_port,
				tempsense_paldosense ? 3 : 1);
			MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
			MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, tempsense_paldosense ? 2 :
				ACMAJORREV_4(pi->pubpi->phy_rev) ? 1 :
				READ_PHYREGFLD(pi, RxSdFeConfig1, farrow_rshift_tx));
		if (!(tempsense_paldosense)) {
			/* Power down LNAs - isolation of TIA power down is insufficient */
			/* at high signal power */
			ACPHY_REG_LIST_START
				MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,	 0,
					rxrf_lna1_pwrup, 1)
				MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,	 0,
					rxrf_lna1_5G_pwrup, 1)
				MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,	 0, rxrf_lna1_pwrup, 0)
				MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,	 0, rxrf_lna1_5G_pwrup, 0)
			ACPHY_REG_LIST_EXECUTE(pi);
			if (ACMAJORREV_4(pi->pubpi->phy_rev))
				MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 1);
		}
	}
	return BCME_OK;
}

static int32
wlc_phy_tempsense_paldosense_radio_setup_acphy_tiny(phy_info_t *pi, uint16 Av, uint16 Vmid,
uint8 tempsense_paldosense, uint8 coreidx)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	tempsense_radioregs_tiny_t *porig =
		&(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs_tiny);
	uint8 core, cores;
	if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev))
		cores = PHYCORENUM((pi)->pubpi->phy_corenum);
	else
		cores = 1;

	for (core = 0; core < cores; core++) {
		if ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev)) && (core != coreidx))
			continue;

		porig->tempsense_cfg[core]     = READ_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core);
		porig->tempsense_ovr1[core]    = READ_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core);
		porig->testbuf_cfg1[core]      = READ_RADIO_REG_TINY(pi, TESTBUF_CFG1, core);
		porig->testbuf_ovr1[core]      = READ_RADIO_REG_TINY(pi, TESTBUF_OVR1, core);
		porig->auxpga_cfg1[core]       = READ_RADIO_REG_TINY(pi, AUXPGA_CFG1, core);
		porig->auxpga_vmid[core]       = READ_RADIO_REG_TINY(pi, AUXPGA_VMID, core);
		porig->auxpga_ovr1[core]       = READ_RADIO_REG_TINY(pi, AUXPGA_OVR1, core);
		porig->tia_cfg5[core]          = READ_RADIO_REG_TINY(pi, TIA_CFG5, core);
		porig->tia_cfg7[core]          = READ_RADIO_REG_TINY(pi, TIA_CFG7, core);
		porig->tia_cfg9[core]          = READ_RADIO_REG_TINY(pi, TIA_CFG9, core);
		porig->adc_ovr1[core]          = READ_RADIO_REG_TINY(pi, ADC_OVR1, core);
		porig->adc_cfg10[core]         = READ_RADIO_REG_TINY(pi, ADC_CFG10, core);
		porig->rx_bb_2g_ovr_east[core] = READ_RADIO_REG_TINY(pi, RX_BB_2G_OVR_EAST, core);

		if (!(tempsense_paldosense)) {
			/* # Setup Tempsense */
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 1);
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_pu, 1);
		} else {
			/* # Setup paldosense */
			MOD_RADIO_REG_TINY(pi, VBAT_OVR1, core, ovr_vbat_monitor_pu, 1);
			MOD_RADIO_REG_TINY(pi, VBAT_CFG, core, vbat_monitor_pu, 1);
		}

		/* # Setup Testbuf */
		MOD_RADIO_REG_TINY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 1);
		MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_PU, 1);
		MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0);
		MOD_RADIO_REG_TINY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 1);

		MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port,
			tempsense_paldosense ? 3 : 1);

		/* # Setup AuxPGA */
		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 1);
		MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 1);
		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 1);

		MOD_RADIO_REG_TINY(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 1);
		MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);
		MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0);
		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 1);
		MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input,
			tempsense_paldosense ? 3 : 1);
		if (tempsense_paldosense) {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_tx2g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX2G_CFG1, core, tx2g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_pa2g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA2G_CFG1, core, pa2g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_pa2g_bias_cas_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA2G_IDAC2, core,
					pa2g_bias_cas_pu, 1);
			} else {
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_tx5g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX5G_CFG1, core, tx5g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core, pa5g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR2, core,
					ovr_pa5g_bias_cas_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core, pa5g_bias_cas_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core, ovr_pa5g_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA5G_CFG4, core, pa5g_pu, 1);
			}
		}

		/* # Setup Aux Path */
		MOD_RADIO_REG_TINY(pi, TIA_CFG9, core, txbb_dac2adc, 0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_in_test, 1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_in_test, 0xF);
		if (RADIOID_IS((pi)->pubpi->radioid, BCM20691_ID))
		   MOD_RADIO_REG_TINY(pi, TIA_CFG5, core, tia_out_test, 0);
		else
		   MOD_RADIO_REG_TINY(pi, TIA_CFG5, core, tia_out_test, 1);

		/* # Turn off TIA otherwise it dominates the ADC input */
		MOD_RADIO_REG_TINY(pi, RX_BB_2G_OVR_EAST, core, ovr_tia_amp1_pwrup, 1);
		MOD_RADIO_REG_TINY(pi, TIA_CFG5, core, tia_amp1_pwrup, 0);
		MOD_RADIO_REG_TINY(pi, RX_BB_2G_OVR_EAST, core, ovr_tia_pwrup_amp2, 1);
		MOD_RADIO_REG_TINY(pi, TIA_CFG7, core, tia_pwrup_amp2, 0);
	}
	return BCME_OK;
}


static int32
wlc_phy_tempsense_paldosense_phy_setup_acphy_28nm(phy_ac_temp_info_t *ti,
uint8 tempsense_paldosense)
{
	phy_info_t *pi = ti->pi;

	/* # this proc is used to configure the tempsense phy settings. */
	/* saving the phy registers */
	phy_ac_reg_cache_save(ti->aci, PHYREGS_TEMPSENSE_VBAT);
	ACPHY_REG_LIST_START
		MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideAuxTssi,  0, afe_iqadc_aux_en, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlCoreAuxTssi1,	   0, afe_iqadc_aux_en, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,	   0, lpf_pu_dc, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,		     0, lpf_pu_dc, 0)
		MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_bq1_pu, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_bq1_pu, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_bq2_pu, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_bq2_pu, 0)
		MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_pu, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_pu, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideGains,    0, lpf_bq1_gain, 1)
		MOD_PHYREGCE_ENTRY(pi, RfctrlCoreLpfGain,      0, lpf_bq1_gain, 0)
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq0, 0)
		MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideAuxTssi,  0, amux_sel_port, 1)
		MOD_PHYREG_ENTRY(pi, RxSdFeConfig1, farrow_rshift_force, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,	   0, amux_sel_port,
			tempsense_paldosense ? 3 : 1);
		MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, tempsense_paldosense ? 2 :
		READ_PHYREGFLD(pi, RxSdFeConfig1, farrow_rshift_tx));
	if (!(tempsense_paldosense)) {
		/* Power down LNAs - isolation of TIA power down is insufficient */
		/* at high signal power */
	ACPHY_REG_LIST_START
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,	 0, rxrf_lna1_pwrup, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,	 0, rxrf_lna1_5G_pwrup, 1)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,	 0, rxrf_lna1_pwrup, 0)
			MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,	 0, rxrf_lna1_5G_pwrup, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
	return BCME_OK;
}


static int32
wlc_phy_tempsense_paldosense_radio_setup_acphy_28nm(phy_ac_temp_info_t *ti, uint16 Av, uint16 Vmid,
uint8 tempsense_paldosense)
{
	phy_info_t *pi = ti->pi;
		/* save radio config before changing it */
	phy_ac_reg_cache_save(ti->aci, RADIOREGS_TEMPSENSE_VBAT);

		if (tempsense_paldosense == 0) {
			/* # Setup Tempsense */
			MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_OVR1, 0, ovr_tempsense_pu, 1);
			MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_CFG,  0, tempsense_pu, 1);
		} else {
			/* # Setup paldosense */
			MOD_RADIO_REG_28NM(pi, RFP, VBAT_OVR1, 0, ovr_vbat_monitor_pu, 1);
			MOD_RADIO_REG_28NM(pi, RFP, VBAT_CFG,  0, vbat_monitor_pu, 1);
		}

		MOD_RADIO_REG_28NM(pi, RF, TESTBUF_CFG1, 0, testbuf_sel_test_port,
			tempsense_paldosense ? 3 : 1);
		MOD_RADIO_REG_28NM(pi, RF, AUXPGA_CFG1, 0, auxpga_i_sel_input,
			tempsense_paldosense ? 3 : 1);

		MOD_RADIO_REG_28NM(pi, RF, AUXPGA_VMID, 0, auxpga_i_sel_vmid, Vmid);
		MOD_RADIO_REG_28NM(pi, RF, AUXPGA_CFG1, 0, auxpga_i_sel_gain, Av);

		ACPHY_REG_LIST_START
			/* # Setup Testbuf */
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_OVR1, 0, ovr_testbuf_PU, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_CFG1, 0, testbuf_PU, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_CFG1, 0, testbuf_GPIO_EN, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TESTBUF_OVR1, 0,
					ovr_testbuf_sel_test_port, 1)
			/*	# loopback bis PU  */
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_CFG5, 0, loopback_bias_pu, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, IQCAL_OVR1, 0,
					ovr_iqcal_PU_loopback_bias, 1)
			/* # Setup AuxPGA */
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_OVR1, 0, ovr_auxpga_i_pu, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_CFG1, 0, auxpga_i_pu, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_OVR1, 0, ovr_auxpga_i_sel_vmid, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_OVR1, 0, ovr_auxpga_i_sel_gain, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_CFG1, 0, auxpga_i_vcm_ctrl, 0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, AUXPGA_OVR1, 0, ovr_auxpga_i_sel_input, 1)
		/* # Setup Aux Path */
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_bq2_adc,   1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_wbcal_adc, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG2_OVR, 0, ovr_tia_sw_aux_adc,   1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_REG16, 0, tia_sw_bq2_adc,   0)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_REG16, 0, tia_sw_aux_adc,   1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_REG18, 0, tia_sw_wbcal_adc, 0)
		/* # Turn off TIA otherwise it dominates the ADC input */
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_CFG1_OVR, 0, ovr_tia_bq1_pu, 1)
			MOD_RADIO_REG_28NM_ENTRY(pi, RF, TIA_REG7, 0,         tia_bq1_pu, 0)
		ACPHY_REG_LIST_EXECUTE(pi);

	return BCME_OK;
}


static int32
wlc_phy_tempsense_poll_adc_war_tiny(phy_info_t *pi, bool init_adc_inside, int32 *measured_values
, uint8 core)
{
	uint16 nsamp = 200;
	wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBE, 0, core);
	measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

	wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBG, 0, core);
	measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

	wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBE, 1, core);
	measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

	wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBG, 1, core);
	measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

	return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_adc_war_28nm(phy_info_t *pi, bool init_adc_inside, int32 *measured_values)
{
	wlc_phy_tempsense_radio_swap_28nm(pi, ACPHY_TEMPSENSE_VBE, 0);
	measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, 0);

	wlc_phy_tempsense_radio_swap_28nm(pi, ACPHY_TEMPSENSE_VBG, 0);
	measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, 0);

	wlc_phy_tempsense_radio_swap_28nm(pi, ACPHY_TEMPSENSE_VBE, 1);
	measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, 0);

	wlc_phy_tempsense_radio_swap_28nm(pi, ACPHY_TEMPSENSE_VBG, 1);
	measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, 0);

	return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_adc_war_20694(phy_info_t *pi, bool init_adc_inside, int16 *measured_values,
	uint8 core)
{
	int16 samp[PHY_CORE_MAX];

	wlc_phy_tempsense_radio_swap_20694(pi, ACPHY_TEMPSENSE_VBE, 0, core);
	wlc_phy_poll_samps_acphy(pi, samp, FALSE, 8, TRUE, core);
	measured_values[0] = samp[core];

	wlc_phy_tempsense_radio_swap_20694(pi, ACPHY_TEMPSENSE_VBG, 0, core);
	wlc_phy_poll_samps_acphy(pi, samp, FALSE, 8, TRUE, core);
	measured_values[1] = samp[core];

	wlc_phy_tempsense_radio_swap_20694(pi, ACPHY_TEMPSENSE_VBE, 1, core);
	wlc_phy_poll_samps_acphy(pi, samp, FALSE, 8, TRUE, core);
	measured_values[2] = samp[core];

	wlc_phy_tempsense_radio_swap_20694(pi, ACPHY_TEMPSENSE_VBG, 1, core);
	wlc_phy_poll_samps_acphy(pi, samp, FALSE, 8, TRUE, core);
	measured_values[3] = samp[core];

	return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_adc_war_20696(phy_info_t *pi, bool init_adc_inside, int16 *measured_values,
	uint8 core)
{
	wlc_phy_tempsense_radio_swap_20696(pi, ACPHY_TEMPSENSE_VBE, 0, core);
	measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

	wlc_phy_tempsense_radio_swap_20696(pi, ACPHY_TEMPSENSE_VBG, 0, core);
	measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

	wlc_phy_tempsense_radio_swap_20696(pi, ACPHY_TEMPSENSE_VBE, 1, core);
	measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

	wlc_phy_tempsense_radio_swap_20696(pi, ACPHY_TEMPSENSE_VBG, 1, core);
	measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);
	return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_samps_tiny(phy_info_t *pi, uint16 samples, bool init_adc_inside
, uint8 core)
{
	int32 measured_voltage;
	int32 i_sum = 0;
	int32 i_val = 0;
	int i = 0;
	uint16 mask, signcheck;
	uint32 signextend, discardbits;
	uint16 nsamps = 0;

	/* Need to set the swap bit, otherwise there is a bug */
	if (init_adc_inside) {
		if (!((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
			(core == 3))) {
			wlc_phy_tempsense_gpiosel_tiny(pi, 16, 1);
		}
	}
	if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_36(pi->pubpi->phy_rev) ||
		ACMAJORREV_32(pi->pubpi->phy_rev)|| ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_37(pi->pubpi->phy_rev)) {
		/* 13 bit signed to 16 bit signed conversion. */
		mask = 0x1FFF;
		signcheck = 0x1000;
		signextend = 0xFFFFF000;
		discardbits = 3;
	} else {
		/* 12 bit signed to 16 bit signed conversion. */
		mask = 0x0FFF;
		signcheck = 0x0800;
		signextend = 0xFFFFF800;
		discardbits = 2;
	}

	OSL_DELAY(10);
	if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
		(core == 3)) {
		phy_iq_est_t rx_iq_est[PHY_CORE_MAX];
		MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 1);
		nsamps = 2 * (1 << 2 * discardbits);
		wlc_phy_rx_iq_est_acphy(pi, rx_iq_est,	nsamps, 32, 0, TRUE);
		i_sum = (int32)phy_utils_sqrt_int((uint32)(rx_iq_est[core].i_pwr)) * 4;
		measured_voltage = 0 - i_sum;
	} else if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		phy_iq_est_t rx_iq_est[PHY_CORE_MAX];
		MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 1);
		nsamps = 2 * (1 << 2 * discardbits);
		wlc_phy_rx_iq_est_acphy(pi, rx_iq_est,	128, 32, 0, TRUE);
		i_sum = (int32)(phy_utils_sqrt_int((uint32)(rx_iq_est[core].i_pwr) / 64));
		measured_voltage = ((int32)(1 << 10) - i_sum) * 8;
	} else {
		for (i = 0; i < samples; i++) {
			i_val = READ_PHYREG(pi, gpioHiOut);
			i_val = (i_val & mask);
			if ((i_val & signcheck)) {
				i_val = (i_val | signextend);
			}
			i_sum += i_val;
		}
		measured_voltage = i_sum/samples;
	}
	return (measured_voltage >> discardbits);
}

static int32
wlc_phy_tempsense_gpiosel_tiny(phy_info_t *pi, uint16 sel, uint8 word_swap)
{
	if (!(ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev))) {
		W_REG(pi->sh->osh, &pi->regs->psm_gpio_oe, 0x0);
	}
	WRITE_PHYREG(pi, gpioLoOutEn, 0x0000);
	WRITE_PHYREG(pi, gpioHiOutEn, 0X8000);

	/* set up acphy GPIO sel */
	WRITE_PHYREG(pi, gpioSel, (word_swap<<8) | sel);
	return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_tiny(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap
, uint8 coreidx)
{
	uint8 core, cores;
	if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev))
		cores = PHYCORENUM((pi)->pubpi->phy_corenum);
	else
		cores = 1;

	for (core = 0; core < cores; core++) {
		if ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev)) && core != coreidx)
			continue;
		/* Enable override */
		MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
		MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

		if (swap == 0) {
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
		} else if (swap == 1) {
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
		} else {
			PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
			return BCME_ERROR;
		}
		if (type == ACPHY_TEMPSENSE_VBG) {
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
		} else if (type == ACPHY_TEMPSENSE_VBE) {
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
		} else {
			PHY_ERROR(("Unsupported, supported types are"));
			PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
			return BCME_ERROR;
		}
	}
	return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_28nm(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap)
{

		/* Enable override */
		MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_OVR1, 0, ovr_tempsense_sel_Vbe_Vbg, 1);
		MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_OVR1, 0, ovr_tempsense_swap_amp, 1);

		if (swap == 0) {
			MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_CFG, 0, tempsense_swap_amp, 0);
		} else if (swap == 1) {
			MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_CFG, 0, tempsense_swap_amp, 1);
		} else {
			PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
			return BCME_ERROR;
		}
		if (type == ACPHY_TEMPSENSE_VBG) {
			MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_CFG, 0, tempsense_sel_Vbe_Vbg, 0);
		} else if (type == ACPHY_TEMPSENSE_VBE) {
			MOD_RADIO_REG_28NM(pi, RFP, TEMPSENSE_CFG, 0, tempsense_sel_Vbe_Vbg, 1);
		} else {
			PHY_ERROR(("Unsupported, supported types are"));
			PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
			return BCME_ERROR;
		}

	return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_20694(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
	uint8 core)
{
	/* Enable override */
	MOD_RADIO_REG_20694(pi, RF, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
	MOD_RADIO_REG_20694(pi, RF, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

	if (swap == 0) {
		MOD_RADIO_REG_20694(pi, RF, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
	} else if (swap == 1) {
		MOD_RADIO_REG_20694(pi, RF, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
	} else {
		PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
		return BCME_ERROR;
	}
	if (type == ACPHY_TEMPSENSE_VBG) {
		MOD_RADIO_REG_20694(pi, RF, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
	} else if (type == ACPHY_TEMPSENSE_VBE) {
		MOD_RADIO_REG_20694(pi, RF, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
	} else {
		PHY_ERROR(("Unsupported, supported types are"));
		PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
		return BCME_ERROR;
	}
	return BCME_OK;
}


static int32
wlc_phy_tempsense_radio_swap_20696(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
	uint8 core)
{
	/* Enable override */
	MOD_RADIO_REG_20696(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
	MOD_RADIO_REG_20696(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

	if (swap == 0) {
		MOD_RADIO_REG_20696(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
	} else if (swap == 1) {
		MOD_RADIO_REG_20696(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
	} else {
		PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
		return BCME_ERROR;
	}
	if (type == ACPHY_TEMPSENSE_VBG) {
		MOD_RADIO_REG_20696(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
	} else if (type == ACPHY_TEMPSENSE_VBE) {
		MOD_RADIO_REG_20696(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
	} else {
		PHY_ERROR(("Unsupported, supported types are"));
		PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
		return BCME_ERROR;
	}
	return BCME_OK;
}


static int32
wlc_phy_tempsense_paldosense_phy_cleanup_acphy_tiny(phy_info_t *pi, uint8 core)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);


	// Additional settings for 4365 core3 GPIO WAR
	if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
		(core == 3)) {
		MOD_PHYREG(pi, DcFiltAddress,        dcBypass,     0);
		MOD_PHYREG(pi, RfctrlOverrideGains3, rxgain,       0);
		MOD_PHYREG(pi, RfctrlOverrideGains3, lpf_bq2_gain, 0);
	}

	/* # restore T/R and external PA states */
	WRITE_PHYREG(pi, RxFeCtrl1, porig->RxFeCtrl1);
	WRITE_PHYREG(pi, RxSdFeConfig6, porig->RxSdFeConfig6);
	WRITE_PHYREG(pi, RxSdFeConfig1, porig->RxSdFeConfig1);
	WRITE_PHYREGCE(pi, RfctrlIntc, core, porig->RfctrlIntc[core]);
	WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, porig->RfctrlOverrideAuxTssi[core]);
	WRITE_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, porig->RfctrlCoreAuxTssi1[core]);
	WRITE_PHYREGCE(pi, RfctrlOverrideRxPus, core, porig->RfctrlOverrideRxPus[core]);
	WRITE_PHYREGCE(pi, RfctrlCoreRxPus, core, porig->RfctrlCoreRxPus[core]);
	WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, porig->RfctrlOverrideTxPus[core]);
	WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, porig->RfctrlCoreTxPus[core]);
	WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, porig->RfctrlOverrideLpfSwtch[core]);
	WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, porig->RfctrlCoreLpfSwtch[core]);
	WRITE_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, porig->RfctrlOverrideAfeCfg[core]);
	WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, porig->RfctrlCoreAfeCfg1[core]);
	WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, porig->RfctrlCoreAfeCfg2[core]);
	WRITE_PHYREGCE(pi, RfctrlOverrideGains, core, porig->RfctrlOverrideGains[core]);
	WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core, porig->RfctrlCoreLpfGain[core]);
	return BCME_OK;

}


static int32
wlc_phy_tempsense_paldosense_phy_cleanup_acphy_28nm(phy_ac_temp_info_t *ti)
{

	phy_ac_reg_cache_restore(ti->aci, PHYREGS_TEMPSENSE_VBAT);
	return BCME_OK;
}

static int32
wlc_phy_tempsense_paldosense_radio_cleanup_acphy_28nm(phy_ac_temp_info_t *ti)
{
	/* restore radio config back */
	phy_ac_reg_cache_restore(ti->aci, RADIOREGS_TEMPSENSE_VBAT);
	return BCME_OK;
}

static int32
wlc_phy_tempsense_paldosense_radio_cleanup_acphy_tiny(phy_info_t *pi, uint8 tempsense_paldosense
, uint8 coreidx)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	tempsense_radioregs_tiny_t *porig =
		&(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs_tiny);
	uint8 core, cores;
	if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev))
		cores = PHYCORENUM((pi)->pubpi->phy_corenum);
	else
		cores = 1;

	for (core = 0; core < cores; core++) {
		if ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev)) && (core != coreidx))
			continue;
		/* # Get back old values */
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TEMPSENSE_OVR1, core),
			porig->tempsense_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TEMPSENSE_CFG, core),
			porig->tempsense_cfg[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TESTBUF_OVR1, core),
			porig->testbuf_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TESTBUF_CFG1, core),
			porig->testbuf_cfg1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_OVR1, core),
			porig->auxpga_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_CFG1, core),
			porig->auxpga_cfg1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_VMID, core),
			porig->auxpga_vmid[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG9, core),
			porig->tia_cfg9[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_OVR1, core),
			porig->adc_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_CFG10, core),
			porig->adc_cfg10[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG5, core),
			porig->tia_cfg5[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, RX_BB_2G_OVR_EAST, core),
			porig->rx_bb_2g_ovr_east[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG7, core),
			porig->tia_cfg7[core]);
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 1);
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0);
		} else {
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0);
			MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0);
			MOD_RADIO_REG_TINY(pi, VBAT_CFG, core, vbat_monitor_pu, 0);
			MOD_RADIO_REG_TINY(pi, VBAT_OVR1, core, ovr_vbat_monitor_pu, 0);
			MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0);
			MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0);
		}
		if (tempsense_paldosense) {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_tx2g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX2G_CFG1, core, tx2g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_tx2g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_pa2g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA2G_CFG1, core, pa2g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_pa2g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_pa2g_bias_cas_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA2G_IDAC2, core, pa2g_bias_cas_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
					ovr_pa2g_bias_cas_pu, 0);
			} else {
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_tx5g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, TX5G_CFG1, core, tx5g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_tx5g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_bias_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core, pa5g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_bias_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR2, core,
					ovr_pa5g_bias_cas_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core,
					pa5g_bias_cas_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR2, core,
					ovr_pa5g_bias_cas_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_pu, 1);
				MOD_RADIO_REG_TINY(pi, PA5G_CFG4, core, pa5g_pu, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_pu, 0);
			}
		}


	}
	return BCME_OK;
}
static int16
wlc_phy_tempsense_paldosense_acphy_tiny(phy_info_t *pi, uint8 tempsense_paldosense)
{

	uint16 auxPGA_Av = 0x3, auxPGA_Vmid = 0x91;
	int64 radio_temp = 0;
	int32 t_scale = 16384;
	int32 t_slope = -10246;
	int32 t_offset = 1765868;
	int32 avbq_scale = 800;
	int32 avbq_slope = 1024;
	uint16 save_afePuCtrl = 0, save_gpio = 0, save_gpioHiOutEn = 0;
	uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
	uint32 save_chipc = 0;
	uint8  stall_val = 0;
	int32 measured_voltage[4] = {0};
	uint8 enRx = 0;
	uint8 enTx = 0;
	uint8 core = 0;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		avbq_scale = 1200;
		core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);
		ASSERT(core < PHY_CORE_MAX);
		if (core < 3) {
			auxPGA_Av = 0x5, auxPGA_Vmid = 0x7D;
			t_slope = -3038; t_offset = 1621378;
		} else {
			auxPGA_Av = 0x5, auxPGA_Vmid = 0x9B;
			t_slope = -3795; t_offset = 1606261;
		}
	}


	/*
	 * # If mac is suspended, leave it suspended and don't touch the state of the MAC
	 * # If not, suspend at the beginning of tempsense and resume it at the end.
	 * # (Suspending is required - as we arereading via muxes that are pin-contolled
	 * # during normal RX & TX.)
	 */
	/* Prepare Mac and Phregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if (!ACMAJORREV_32(pi->pubpi->phy_rev) && !ACMAJORREV_33(pi->pubpi->phy_rev)) {
		/* change Av and Vmid for paldosense */
		if ((tempsense_paldosense)) {
			auxPGA_Av = 0x7;
			auxPGA_Vmid = 70;
		}
	}

	if (IS_4364_1x1(pi)) {
		t_slope = -11009;
		t_offset = 1729014;
	}

	/* change slope/offset for 4355/4359 RSDB core 1 */
	if (!(ROUTER_4349(pi)) && (phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1)) {
		t_slope = -11765;
		t_offset = 1671660;
	}
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
		stf_shdata->hw_phyrxchain, stf_shdata->hw_phytxchain);
	wlc_phy_tempsense_paldosense_phy_setup_acphy_tiny(pi, tempsense_paldosense, core);
	wlc_phy_tempsense_paldosense_radio_setup_acphy_tiny(pi, auxPGA_Av, auxPGA_Vmid,
	tempsense_paldosense, core);

	wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
	                          &save_chipc, &fval2g_orig, &fval5g_orig,
	                          &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);
	wlc_phy_tempsense_poll_adc_war_tiny(pi, TRUE, measured_voltage, core);
	if (!(tempsense_paldosense)) {
		radio_temp += (int64)(((measured_voltage[0] + measured_voltage[2]
			- measured_voltage[1] - measured_voltage[3]) / 2)
			* (int64) t_slope * avbq_scale) / avbq_slope;
		radio_temp = (radio_temp + t_offset)/t_scale;
	}

	wlc_phy_tempsense_paldosense_phy_cleanup_acphy_tiny(pi, core);
	wlc_phy_tempsense_paldosense_radio_cleanup_acphy_tiny(pi, tempsense_paldosense, core);
	wlc_phy_restore_after_adc_read(pi,	&save_afePuCtrl, &save_gpio,
		&save_chipc,  &fval2g_orig,  &fval5g_orig,
		&fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);
	phy_utils_phyreg_exit(pi);

	/* # RESUME MAC as soon as we are done reading/writing regs and muxes
	 * # -----------------------------------------------------------------
	 */
	wlapi_enable_mac(pi->sh->physhim);

	if (!(tempsense_paldosense)) {
		/* Store temperature and return value */
		pi->u.pi_acphy->tempi->current_temperature = (int16) radio_temp;

	#ifdef ATE_BUILD
		wl_ate_set_buffer_regval(CURR_RADIO_TEMP, radio_temp, -1, phy_get_current_core(pi),
		pi->sh->chip);
	#endif

		return ((int16)radio_temp);
	} else {
		return ((int16)measured_voltage[3]);
	}
}


static int16
wlc_phy_tempsense_paldosense_acphy_28nm(phy_ac_temp_info_t *ti, uint8 tempsense_paldosense)
{
	phy_info_t *pi = ti->pi;

	uint16 auxPGA_Av = 0x3, auxPGA_Vmid = 145;
	int64 radio_temp = 0;
	int32 t_scale = 16384;
	int32 t_slope = -6536;
	int32 t_offset = 1793300;
	int32 measured_voltage[4] = {0};
	int32 psm_gpio_oe;
	/*
	 * # If mac is suspended, leave it suspended and don't touch the state of the MAC
	 * # If not, suspend at the beginning of tempsense and resume it at the end.
	 * # (Suspending is required - as we arereading via muxes that are pin-contolled
	 * # during normal RX & TX.)
	 */
	/* Prepare Mac and Phregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);


	if (ACMAJORREV_36(pi->pubpi->phy_rev) && ACMINORREV_0(pi)) {
		wlc_btcx_override_enable(pi);
	}

	/* change Av and Vmid for paldosense */
	if ((tempsense_paldosense)) {
		auxPGA_Av   = 7;
		auxPGA_Vmid = 70;
	} else {
		auxPGA_Av   = 3;
		auxPGA_Vmid = 145;
	}

	psm_gpio_oe = R_REG(pi->sh->osh, &pi->regs->psm_gpio_oe);
	wlc_phy_tempsense_paldosense_phy_setup_acphy_28nm(ti, tempsense_paldosense);
	wlc_phy_tempsense_paldosense_radio_setup_acphy_28nm(ti, auxPGA_Av, auxPGA_Vmid,
	tempsense_paldosense);

	wlc_phy_tempsense_poll_adc_war_28nm(pi, TRUE, measured_voltage);

	if (!(tempsense_paldosense)) {
		radio_temp += (int64)(((measured_voltage[0] + measured_voltage[2]
			- measured_voltage[1] - measured_voltage[3]) / 2)
			* (int64) t_slope);
		radio_temp = (radio_temp + t_offset)/t_scale;
	}

	wlc_phy_tempsense_paldosense_phy_cleanup_acphy_28nm(ti);
	wlc_phy_tempsense_paldosense_radio_cleanup_acphy_28nm(ti);
	phy_utils_phyreg_exit(pi);

	W_REG(pi->sh->osh, &pi->regs->psm_gpio_oe, psm_gpio_oe);

	psm_gpio_oe = R_REG(pi->sh->osh, &pi->regs->psm_gpio_oe);

	/* # RESUME MAC as soon as we are done reading/writing regs and muxes
	 * # -----------------------------------------------------------------
	 */
	if (ACMAJORREV_36(pi->pubpi->phy_rev) && ACMINORREV_0(pi)) {
		wlc_phy_btcx_override_disable(pi);
	}
	wlapi_enable_mac(pi->sh->physhim);

	if (!(tempsense_paldosense)) {
		/* Store temperature and return value */
		ti->current_temperature = (int16) radio_temp;
	#ifdef ATE_BUILD
		ate_buffer_regval[0].curr_radio_temp = (int16) radio_temp;
	#endif
		return ((int16)radio_temp);
	} else {
		return ((int16)measured_voltage[3]);
	}
}


/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */
int16
wlc_phy_tempsense_vbatsense_acphy_20694(phy_info_t *pi, uint8 tempsense_vbatsense)
{
	int32 t_scale = 16384;
	int32 t_slope = -6900;
	int32 t_offset = 1751040;
	int16 gpio_clk_en;
	int16 Vout[4] = {0, 0, 0, 0};
	int16 measured_voltage[3][4] = {{0}};
	int32 radio_temp = 0;
	uint8 core, core_cnt = 0;
	int16 offset = (int16) pi->phy_tempsense_offset;
	uint16 auxPGA_Av = 0x3, auxPGA_Vmid = 0x91;
	uint16 save_afePuCtrl = 0, save_gpio = 0, save_gpioHiOutEn = 0;
	uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
	uint32 save_chipc = 0;
	uint8  stall_val = 0;
	bool ocl_en, ocl_core_mask_ovr = 0;
	uint8 ocl_core_mask = 1;
	phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	BCM_REFERENCE(stf_shdata);

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	phy_utils_phyreg_enter(pi);

	// If OCL enabled, find out which core is listening
	ocl_en = (READ_PHYREGFLD(pi, OCLControl1, ocl_mode_enable) == 1);
	if (ocl_en) {
		core_cnt = 1;
		ocl_core_mask_ovr = (READ_PHYREGFLD(pi, OCLControl1, ocl_core_mask_ovr) == 1);
		if (ocl_core_mask_ovr)
			ocl_core_mask = READ_PHYREGFLD(pi, OCLControl1, ocl_rx_core_mask);
	} else {
		FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core)
			core_cnt++;
	}

	/* Prepare Mac and Phy regs */
	gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
	MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);

	wlc_phy_tempsense_phy_setup_acphy(pi);
	wlc_phy_tempsense_radio_setup_acphy_20694(pi, auxPGA_Av, auxPGA_Vmid);

	wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
	                          &save_chipc, &fval2g_orig, &fval5g_orig,
	                          &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		if (!ocl_en || (ocl_en && ((ocl_core_mask >> core) & 0x01) == 1)) {
			wlc_phy_tempsense_poll_adc_war_20694(pi, TRUE, measured_voltage[core],
				core);

			radio_temp += (int32)(((measured_voltage[core][0]
					+ measured_voltage[core][2]
					- measured_voltage[core][1]
					- measured_voltage[core][3]) / 2))
					* (t_slope / core_cnt);
			// for PHY_THERMAL message
			Vout[0] += measured_voltage[core][0] / core_cnt;
			Vout[1] += measured_voltage[core][1] / core_cnt;
			Vout[2] += measured_voltage[core][2] / core_cnt;
			Vout[3] += measured_voltage[core][3] / core_cnt;
		}
	}
	radio_temp = (radio_temp + t_offset)/t_scale;

	PHY_THERMAL(("Tempsense\n\tAuxADC0 Av,Vmid = 0x%x,0x%x\n",
	             auxPGA_Av, auxPGA_Vmid));
	PHY_THERMAL(("\tCore0 Vref1,Vref2,Vctat1,Vctat2 = %d,%d,%d,%d\n",
			measured_voltage[0][0], measured_voltage[0][2],
			measured_voltage[0][1], measured_voltage[0][3]));
	PHY_THERMAL(("\tCore1 Vref1,Vref2,Vctat1,Vctat2 = %d,%d,%d,%d\n",
			measured_voltage[1][0], measured_voltage[1][2],
			measured_voltage[1][1], measured_voltage[1][3]));
	PHY_THERMAL(("\t^C Formula: (%d*(Vctat1+Vctat2-Vref1-Vref2)/2*-6900+%d)/%d\n",
	             t_slope, t_offset, t_scale));
	PHY_THERMAL(("\t^C = %d, applied offset = %d\n",
	             radio_temp, offset));

	/* restore registers and resume MAC */
	wlc_phy_restore_after_adc_read(pi,	&save_afePuCtrl, &save_gpio,
		&save_chipc,  &fval2g_orig,  &fval5g_orig,
		&fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

	wlc_phy_tempsense_phy_cleanup_acphy(pi);
	wlc_phy_tempsense_radio_cleanup_acphy_20694(pi);

	MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);

	/* Store temperature and return value */
	ti->current_temperature = (int16) radio_temp + offset;
	//pi->u.pi_acphy->tempi->current_temperature = (int16) radio_temp + offset;


	phy_utils_phyreg_exit(pi);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	wlapi_enable_mac(pi->sh->physhim);

	return ((int16) radio_temp + offset);
}

int16
wlc_phy_tempsense_vbatsense_acphy_20696(phy_info_t *pi, uint8 tempsense_vbatsense)
{
	const int32 t_scale = 16300;
	const int32 t_slope = 2561;
	const int32 t_offset = 1875200;
	int16 gpio_clk_en;
	int16 Vout[4] = {0, 0, 0, 0};
	int16 measured_voltage[PHY_CORE_MAX][4] = {{0}};
	int32 radio_temp = 0;
	uint8 core, core_cnt = 0;
	int16 offset = (int16) pi->phy_tempsense_offset;
	const uint16 auxPGA_Av = 0, auxPGA_Vmid = 0x96;
	uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
	uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
	uint32 save_chipc;
	uint8  stall_val;
	phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	phy_utils_phyreg_enter(pi);


	/* Prepare Mac and Phy regs */
	gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
	MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
	wlc_phy_tempsense_phy_setup_acphy(pi);
	wlc_phy_tempsense_radio_setup_acphy_20696(pi, auxPGA_Av, auxPGA_Vmid);

	wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
	                          &save_chipc, &fval2g_orig, &fval5g_orig,
	                          &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		core_cnt++;
	}

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		wlc_phy_tempsense_poll_adc_war_20696(pi, TRUE, measured_voltage[core], core);
		radio_temp += (int32)(((measured_voltage[core][0]
				+ measured_voltage[core][2]
				- measured_voltage[core][1]
				- measured_voltage[core][3]) / 2))
					* (t_slope / core_cnt);
		// for PHY_THERMAL message
		Vout[0] += measured_voltage[core][0] / core_cnt;
		Vout[1] += measured_voltage[core][1] / core_cnt;
		Vout[2] += measured_voltage[core][2] / core_cnt;
		Vout[3] += measured_voltage[core][3] / core_cnt;
	}

	radio_temp = ((radio_temp + t_offset) / t_scale);

	/* restore registers and resume MAC */
	wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
		&save_chipc,  &fval2g_orig,  &fval5g_orig,
		&fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

	wlc_phy_tempsense_phy_cleanup_acphy(pi);
	wlc_phy_tempsense_radio_cleanup_acphy_20696(pi);

	MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);

	/* Store temperature and return value */
	ti->current_temperature = (int16) radio_temp + offset;


	phy_utils_phyreg_exit(pi);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	wlapi_enable_mac(pi->sh->physhim);
	return ((int16) radio_temp + offset);
}


int16
wlc_phy_tempsense_acphy(phy_info_t *pi)
{
	uint8 core, core_cnt = 0;
	uint8 sel_Vb, swap_amp;
	int8 idx;
	uint16 auxPGA_Av = 0x3, auxPGA_Vmid = 0x91;
	int16 V[4][3] = {{0, 0}}, Vout[4] = {0, 0, 0, 0};
	int16 offset = (int16) pi->phy_tempsense_offset;
	int32 radio_temp = 0;
	int32 t_scale = 16384;
	int32 t_slope = 8766;
	int32 t_offset = 1902100;
	int32 avbq_scale = 256;
	int32 avbq_slope[3] = {527, 521, 522};
	uint16 save_afePuCtrl = 0, save_gpio = 0, save_gpioHiOutEn = 0;
	uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
	uint32 save_chipc = 0;
	uint8  stall_val = 0, log2_nsamps = 3;
	int32 k, tmp_samp, samp_accum;
	phy_ac_info_t *pi_ac = pi->u.pi_acphy;
	phy_ac_temp_info_t *ti = pi_ac->tempi;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	/* return dummy temp */
	if (NORADIO_ENAB(pi->pubpi)) {
		ti->current_temperature = 25;
		return 25;
	}

	if (TINY_RADIO(pi)) {
			return wlc_phy_tempsense_paldosense_acphy_tiny(pi, SENSE_TEMP);
	} else if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		return wlc_phy_tempsense_paldosense_acphy_28nm(ti, SENSE_TEMP);
	} else if (ACMAJORREV_37(pi->pubpi->phy_rev)) {
		return wlc_phy_tempsense_vbatsense_acphy_20696(pi, SENSE_TEMP);
	} else if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		return wlc_phy_tempsense_vbatsense_acphy_20694(pi, SENSE_TEMP);
	}

	/* Prepare Mac and Phregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
		auxPGA_Av = 0x3;
		auxPGA_Vmid = 0x91;
	}

	if (IS_4364_3x3(pi)) {
		t_offset = 1860613;
		avbq_slope[0] = 518;
		avbq_slope[1] = 521;
		avbq_slope[2] = 537;
	}

	wlc_phy_tempsense_phy_setup_acphy(pi);
	wlc_phy_tempsense_radio_setup_acphy(pi, auxPGA_Av, auxPGA_Vmid);
		wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
			&save_chipc, &fval2g_orig, &fval5g_orig,
			&fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);
	wlc_phy_pulse_adc_reset_acphy(pi);

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
	  wlc_phy_gpiosel_acphy(pi, 16+core, 1);
	  core_cnt++;
	  for (idx = 0; idx < 4; idx++) {
	    switch (idx) {
	    case 0:
	      sel_Vb = 1;
	      swap_amp = 0;
		      break;
	    case 1:
	      sel_Vb = 0;
	      swap_amp = 0;
	      break;
	    case 2:
	      sel_Vb = 1;
	      swap_amp = 1;
	      break;
	    case 3:
	      sel_Vb = 0;
	      swap_amp = 1;
	      break;
	    default:
	      sel_Vb = 0;
	      swap_amp = 0;
	      break;
	    }

	    /* Reg conflict with 2069 rev 16 */
	    if (RADIOMAJORREV(pi) == 0) {
		MOD_RADIO_REGC(pi, OVR18, core, ovr_tempsense_sel_Vbe_Vbg, 0x1);
	    } else {
		MOD_RADIO_REGC(pi, GE16_OVR19, core, ovr_tempsense_sel_Vbe_Vbg, 0x1);
	    }
	    MOD_RADIO_REGC(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, sel_Vb);

	    /* Reg conflict with 2069 rev 16 */
	    if (RADIOMAJORREV(pi) == 0) {
		MOD_RADIO_REGC(pi, OVR18, core, ovr_tempsense_swap_amp, 0x1);
	    } else {
		MOD_RADIO_REGC(pi, GE16_OVR19, core, ovr_tempsense_swap_amp, 0x1);
	    }
	    MOD_RADIO_REGC(pi, TEMPSENSE_CFG, core, swap_amp, swap_amp);

	    OSL_DELAY(10);
	    samp_accum = 0;
	    for (k = 0; k < (1 << log2_nsamps); k++) {
	      /* read out the i-value */
	      tmp_samp = READ_PHYREG(pi, gpioHiOut) >> 2;
	      tmp_samp -= (tmp_samp < 512) ? 0 : 1024;
	      samp_accum += tmp_samp;
	    }
	    V[idx][core] = (int16) (samp_accum >> log2_nsamps);
	  }
	}

	if ((RADIOMAJORREV(pi) == 1) ||
	    (RADIOMAJORREV(pi) == 2)) {

		if (RADIOMAJORREV(pi) == 1) {
			t_slope 		= -5453;
			t_offset		= 1748881;
		} else if (RADIOMAJORREV(pi) == 2) {
			t_slope 		= -5549;
			t_offset		= 1697118;
		}

		avbq_scale 		= 800;
		avbq_slope[0]	        = 1024;
		avbq_slope[1] = 1024;
		t_scale			= 16384;

		FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
	        radio_temp += (int32)(((V[0][core] + V[2][core] - V[1][core] - V[3][core]) / 2))
		                      * ((t_slope * avbq_scale) / (avbq_slope[core] * core_cnt));
			Vout[0] += V[0][core] / core_cnt;
			Vout[1] += V[1][core] / core_cnt;
			Vout[2] += V[2][core] / core_cnt;
			Vout[3] += V[3][core] / core_cnt;

		}
		radio_temp = (radio_temp + t_offset)/t_scale;

		/* Forcing offset to zero as this is not characterized yet for 4335 */
		offset = 0;

	} else {
		FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
			radio_temp += (int32)(V[1][core] + V[3][core] - V[0][core] - V[2][core])
					  * t_slope / 2 * avbq_scale / avbq_slope[core] / core_cnt;
			Vout[0] += V[0][core] / core_cnt;
			Vout[1] += V[1][core] / core_cnt;
			Vout[2] += V[2][core] / core_cnt;
			Vout[3] += V[3][core] / core_cnt;
		}
		radio_temp = (radio_temp + t_offset) / t_scale;
	}

	PHY_THERMAL(("Tempsense\n\tAuxADC0 Av,Vmid = 0x%x,0x%x\n",
	             auxPGA_Av, auxPGA_Vmid));
	PHY_THERMAL(("\tVref1,Vref2,Vctat1,Vctat2 =%d,%d,%d,%d\n",
	             Vout[0], Vout[2], Vout[1], Vout[3]));
	PHY_THERMAL(("\t^C Formula: (%d*(Vctat1+Vctat2-Vref1-Vref2)/2*800/1024+%d)/%d\n",
	             t_slope, t_offset, t_scale));
	PHY_THERMAL(("\t^C = %d, applied offset = %d\n",
	             radio_temp, offset));

	wlc_phy_tempsense_phy_cleanup_acphy(pi);
	wlc_phy_tempsense_radio_cleanup_acphy(pi);
		wlc_phy_restore_after_adc_read(pi,  &save_afePuCtrl, &save_gpio,
			&save_chipc,  &fval2g_orig,  &fval5g_orig,
			&fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	/* Store temperature and return value */
	ti->current_temperature = (int16) radio_temp + offset;


	return ((int16) radio_temp + offset);
}


void
phy_ac_update_tempsense_bitmap(phy_info_t *pi)
{
	phy_info_t *other_pi = phy_get_other_pi(pi);
	phy_temp_info_t *tempi = pi->tempi;
	phy_txcore_temp_t *temp_status = phy_temp_get_st(tempi);

	temp_status->bitmap = pi->pubpi->phy_coremask;
	temp_status->bitmap |= (temp_status->bitmap << 4);

	if (phy_get_phymode(pi) == PHYMODE_RSDB) {
		tempi = other_pi->tempi;
		temp_status = phy_temp_get_st(tempi);
		temp_status->bitmap = other_pi->pubpi->phy_coremask;
		temp_status->bitmap |= (temp_status->bitmap << 4);
	}
}
static uint8
wlc_phy_vbat_monitoring_algorithm_acphy(phy_info_t *pi)
{
	uint8 i;
	uint8 do_cals = 1;
	uint32 paldo_data[5] = {0x0, 0x7, 0x6, 0x5, 0x4};
	uint32 temp;
	int16 threshold = 12;
	int16 read_high_voltage;
	int16 read_new_voltage;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_temp_info_t *ti = pi_ac->tempi;

	/* setting the address for LDO and writing the 3p3 voltage */
	phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
	temp = temp & 0xFFFF8FFF;
	temp = temp | (paldo_data[0] << 12);
	phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
	/* setting the address for PALDO and writing the 3p3 voltage */
	phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
	temp = temp & 0xFFFFFFF8;
	temp = temp | paldo_data[0];
	phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
	read_high_voltage = wlc_phy_tempsense_paldosense_acphy_tiny(pi, SENSE_PALDO);
	/* loop over to check till what point regulated conditon is seen */
	for (i = 1; i < 5; i++) {
		/* setting the address for LDO and writing the 3p3 voltage */
		phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
		temp = temp & 0xFFFF8FFF;
		temp = temp | (paldo_data[i] << 12);
		phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
		/* setting the address for PALDO and writing the 3p3 voltage */
		phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
		temp = temp & 0xFFFFFFF8;
		temp = temp | paldo_data[i];
		phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
		read_new_voltage = wlc_phy_tempsense_paldosense_acphy_tiny(pi, SENSE_PALDO);
		if ((read_high_voltage - read_new_voltage) < threshold) {
			read_high_voltage = read_new_voltage;
		} else {
			break;
		}
	}
	if (i != 6) {
		/* setting the address for LDO and writing the 3p3 voltage */
		phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
		temp = temp & 0xFFFF8FFF;
		temp = temp | (paldo_data[i-1] << 12);
		phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
		/* setting the address for PALDO and writing the 3p3 voltage */
		phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
		temp = temp & 0xFFFFFFF8;
		temp = temp | paldo_data[i-1];
		phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
	} else {
		/* setting the address for LDO and writing the 3p3 voltage */
		phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
		temp = temp & 0xFFFF8FFF;
		temp = temp | (paldo_data[i-2] << 12);
		phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
		/* setting the address for PALDO and writing the 3p3 voltage */
		phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
		temp = temp & 0xFFFFFFF8;
		temp = temp | paldo_data[i-2];
		phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
	}
	if (i == ti->last_regulated)
		do_cals = 0;
	ti->last_regulated = i;
	return ((uint8)do_cals);
}
uint8
wlc_phy_vbat_monitoring_algorithm_decision(phy_info_t *pi)
{
	uint8 do_cals = 0;
	do_cals = wlc_phy_vbat_monitoring_algorithm_acphy(pi);
	return ((uint8)do_cals);
}

void
phy_ac_update_dutycycle_throttle_state(phy_info_t *pi)
{
	phy_info_t *other_pi = phy_get_other_pi(pi);
	phy_temp_info_t *tempi = pi->tempi;
	phy_txcore_temp_t *temp_status = phy_temp_get_st(tempi);
	temp_status->duty_cycle = 100;
	temp_status->duty_cycle_throttle_state = 0;
	temp_status->duty_cycle_throttle_depth = 10;
	if (phy_get_phymode(pi) == PHYMODE_RSDB) {
		tempi = other_pi->tempi;
		temp_status = phy_temp_get_st(tempi);
		temp_status->duty_cycle = 100;
		temp_status->duty_cycle_throttle_state = 0;
		temp_status->duty_cycle_throttle_depth = 10;
	}
}

static void
phy_ac_temp_upd_gain(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj)
{
	phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
	phy_info_t *pi = info->pi;

	/* read in the temperature */
	int16 temp_diff, curr_temp = 0, gain_temp_slope = 0;
	curr_temp = info->current_temperature;
	curr_temp = MIN(MAX(curr_temp, PHY_TEMPSENSE_MIN), PHY_TEMPSENSE_MAX);

	/* check that non programmed SROM for cal temp are not changed */
	if (pi->srom_rawtempsense != 255) {
		temp_diff = curr_temp - pi->srom_rawtempsense;
	} else {
		temp_diff = 0;
	}

	/* adjust gain based on the temperature difference now vs. calibration time:
	 * make gain diff rounded to nearest 0.25 dbm, where 1 tick is 0.25 dbm
	 */
	gain_temp_slope = CHSPEC_IS2G(pi->radio_chanspec) ?
			ACPHY_GAIN_VS_TEMP_SLOPE_2G : ACPHY_GAIN_VS_TEMP_SLOPE_5G;

	if (temp_diff >= 0) {
		*gain_err_temp_adj = (temp_diff * gain_temp_slope*2 + 25)/50;
	} else {
		*gain_err_temp_adj = (temp_diff * gain_temp_slope*2 - 25)/50;
	}
}

void
phy_ac_temp_upd_gain_cal(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj)
{
	phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
	phy_info_t *pi = info->pi;

	/* read in the temperature */
	int16 temp_diff, curr_temp = 0, gain_temp_slope = 0;
	curr_temp = info->current_temperature;
	curr_temp = MIN(MAX(curr_temp, PHY_TEMPSENSE_MIN), PHY_TEMPSENSE_MAX);

	/* check that non programmed SROM for cal temp are not changed */
	if (pi->srom_gain_cal_temp != 255) {
		temp_diff = curr_temp - pi->srom_gain_cal_temp;
	} else {
		temp_diff = 0;
	}

	/* adjust gain based on the temperature difference now vs. calibration time:
	* make gain diff rounded to nearest 0.25 dbm, where 1 tick is 0.25 dbm
	*/
	gain_temp_slope = CHSPEC_IS2G(pi->radio_chanspec) ?
	    ACPHY_GAIN_VS_TEMP_SLOPE_2G : ACPHY_GAIN_VS_TEMP_SLOPE_5G;

	if (temp_diff >= 0) {
		*gain_err_temp_adj = (temp_diff * gain_temp_slope*2 + 25)/50;
	} else {
		*gain_err_temp_adj = (temp_diff * gain_temp_slope*2 - 25)/50;
	}
}

static int16
phy_ac_temp_do_tempsense(phy_type_temp_ctx_t *ctx)
{
	phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
	phy_info_t *pi = info->pi;
	return wlc_phy_tempsense_acphy(pi);
}
