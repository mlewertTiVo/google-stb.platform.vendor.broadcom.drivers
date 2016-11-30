/*
 * ACPHY DeepSleepInit module
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
 * $Id: phy_ac_dsi.c 639713 2016-05-24 18:02:57Z vyass $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>

/* PHY common dependencies */
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_utils_var.h>
#include <phy_utils_reg.h>
#include <phy_utils_radio.h>

/* PHY type dependencies */
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#include <wlc_phy_radio.h>
#include <wlc_phytbl_ac.h>

/* DSI module dependencies */
#include <phy_ac_dsi.h>
#include "phy_ac_dsi_data.h"
#include "phy_type_dsi.h"

/* Inter-module dependencies */
#include "phy_ac_radio.h"

#include "fcbs.h"

#ifdef BCMULP
#include "ulp.h"
#endif /* BCMULP */

typedef struct {
	uint8 ds1_napping_enable;
} phy_ac_dsi_params_t;

/* module private states */
struct phy_ac_dsi_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_dsi_info_t *di;
	phy_ac_dsi_params_t *dp;
};

typedef struct {
	int num;
	fcbs_input_data_t *data;
} dsi_fcbs_t;

static const char BCMATTACHDATA(rstr_ds1nap)[] = "ds1_nap";

#define DSI_DBG_PRINTS 0

/* debug prints */
#if defined(DSI_DBG_PRINTS) && DSI_DBG_PRINTS
#define DSI_DBG(args)	printf args; OSL_DELAY(500);
#else
#define DSI_DBG(args)
#endif /* DSI_DBG_PRINTS */

/* accessor functions */
dsi_fcbs_t * BCMRAMFN(dsi_get_ram_seq)(phy_info_t *pi);
fcbs_input_data_t **BCMRAMFN(dsi_get_dyn_ram_seq)(phy_info_t *pi);

#ifndef USE_FCBS_ROM
fcbs_input_data_t *BCMRAMFN(dsi_get_radio_pd_seq)(phy_info_t *pi);
#endif /* USE_FCBS_ROM */

/* top level wrappers */
#ifdef BCMULP
static int  dsi_save(phy_type_dsi_ctx_t *ctx);
static void dsi_restore(phy_type_dsi_ctx_t *ctx);
#endif /* BCMULP */

/* Generic Utils */
static void dsi_save_phyregs(phy_info_t *pi, adp_t *input, uint16 len);
static void dsi_save_radioregs(phy_info_t *pi, adp_t *input, uint16 len);
static void dsi_save_phytbls(phy_info_t *pi, phytbl_t *phytbl, uint16 len);
static void dsi_update_radio_dyn_data(phy_info_t *pi, phy_rad_dyn_adp_t *radioreg, uint16 len);
static void dsi_update_phy_dyn_data(phy_info_t *pi, phy_rad_dyn_adp_t *phyreg, uint16 len);

/* Takes a selective snapshot of current PHY and RADIO register space */
static void dsi_update_snapshot(phy_info_t *pi, fcbs_input_data_t *save_seq);

/* PhySpecific save routine */
#ifdef BCMULP
static void dsi_save_ACMAJORREV_36(phy_info_t *pi);
#endif /* BCMULP */
static void dsi_update_dyn_seq_ACMAJORREV_36(phy_info_t *pi);

/* FCBS Dynamic sequence numbers and pointer for maj36_min0 (43012A0) */
dsi_fcbs_t ram_maj36_min0_seq[] = {
	{CHANSPEC_PHY_RADIO, ram_maj36_min0_chanspec_phy_radio},
	{CALCACHE_PHY_RADIO, ram_maj36_min0_calcache_phy_radio},
	{0xff, NULL}
};

/* Dynamic sequences common for DS0 and DS1 (20695_maj1_min0 - 43012A0) */
fcbs_input_data_t *dyn_ram_maj36_min0_seq[] = {
	dyn_ram_20695_maj1_min0_radio_minipmu_pwr_up,
	dyn_ram_20695_maj1_min0_radio_2g_pll_pwr_up,
	dyn_ram_20695_maj1_min0_radio_5g_pll_pwr_up,
	dyn_ram_20695_maj1_min0_radio_5g_pll_to_2g_pwr_up
};

/* FCBS Dynamic sequence numbers and pointer for maj36_min1 (43012B0) */
dsi_fcbs_t ram_maj36_min1_seq[] = {
	{CHANSPEC_PHY_RADIO, ram_maj36_min1_chanspec_phy_radio},
	{CALCACHE_PHY_RADIO, ram_maj36_min1_calcache_phy_radio},
	{0xff, NULL}
};

/* Dynamic sequences common for DS0 and DS1 (20695_maj2_min0 - 43012B0) */
fcbs_input_data_t *dyn_ram_maj36_min1_seq[] = {
	dyn_ram_20695_maj2_min0_radio_minipmu_pwr_up,
	dyn_ram_20695_maj2_min0_radio_2g_pll_pwr_up,
	dyn_ram_20695_maj2_min0_radio_5g_pll_pwr_up,
	dyn_ram_20695_maj2_min0_radio_5g_pll_to_2g_pwr_up
};

#define MINI_PMU_PU_OFF		0
#define PLL_2G_OFF		1
#define PLL_5G_OFF		2
#define PLL_5G_TO_2G_OFF	3

dsi_fcbs_t *
BCMRAMFN(dsi_get_ram_seq)(phy_info_t *pi)
{
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		if (ACMINORREV_0(pi)) {
			return ram_maj36_min0_seq;
		} else if (ACMINORREV_1(pi)) {
			return ram_maj36_min1_seq;
		} else {
			PHY_ERROR(("wl%d %s: Invalid ACMINORRREV!\n", PI_INSTANCE(pi),
					__FUNCTION__));
			ASSERT(0);
		}
	} else {
		PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
		ASSERT(0);
	}
	return NULL;
}

fcbs_input_data_t **
BCMRAMFN(dsi_get_dyn_ram_seq)(phy_info_t *pi)
{
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		if (ACMINORREV_0(pi)) {
			return dyn_ram_maj36_min0_seq;
		} else if (ACMINORREV_1(pi)) {
			return dyn_ram_maj36_min1_seq;
		} else {
			PHY_ERROR(("wl%d %s: Invalid ACMINORRREV!\n", PI_INSTANCE(pi),
					__FUNCTION__));
			ASSERT(0);
		}
	} else {
		PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
		ASSERT(0);
	}
	return NULL;
}

#ifndef USE_FCBS_ROM
fcbs_input_data_t *
BCMRAMFN(dsi_get_radio_pd_seq)(phy_info_t *pi)
{
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		if (ACMINORREV_0(pi)) {
			return rom_20695_maj1_min0_radio_pwr_down;
		} else if (ACMINORREV_1(pi)) {
			return rom_20695_maj2_min0_radio_pwr_down;
		} else {
			PHY_ERROR(("wl%d %s: Invalid ACMINORRREV!\n", PI_INSTANCE(pi),
					__FUNCTION__));
			ASSERT(0);
		}
	} else {
		PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
		ASSERT(0);
	}
	return NULL;
}
#endif /* USE_FCBS_ROM */

#ifdef BCMULP
static int
phy_dsi_ulp_enter_cb(void *handle, ulp_ext_info_t *einfo, uint8 *cache_data)
{
	phy_info_t *pi = (phy_info_t *)handle;

	/* Call chip specific save function */
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		/* Save PHY and Radio snapshot */
		dsi_save_ACMAJORREV_36(pi);
	} else {
		PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
		ASSERT(0);
	}

	/* Dynamic sequence update */
	dsi_update_dyn_seq(pi);

	return BCME_OK;
}

/* Call back structure */
static const ulp_p1_module_pubctx_t ulp_phy_dsi_ctx = {
	MODCBFL_CTYPE_STATIC,
	phy_dsi_ulp_enter_cb,
	NULL,
	NULL,
	NULL,
	NULL
};
#endif /* BCMULP */

/* register phy type specific implementation */
phy_ac_dsi_info_t *
BCMATTACHFN(phy_ac_dsi_register_impl)(phy_info_t *pi, phy_ac_info_t *aci, phy_dsi_info_t *di)
{
	phy_ac_dsi_info_t *info;
	phy_type_dsi_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_ac_dsi_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((info->dp = phy_malloc(pi, sizeof(phy_ac_dsi_params_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	info->pi = pi;
	info->aci = aci;
	info->di = di;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));

	fns.ctx = info;
#ifdef BCMULP
	fns.save = dsi_save;
	fns.restore = dsi_restore;
#endif /* BCMULP */

	/* 4339 Prototype Work */
	if (ACMAJORREV_1(pi->pubpi->phy_rev))
		dsi_populate_addr_ACMAJORREV_1(pi);

	phy_dsi_register_impl(di, &fns);

	/* Register DS1 entry call back */
#ifdef BCMULP
	if (BCME_OK != ulp_p1_module_register(ULP_MODULE_ID_PHY_RADIO, &ulp_phy_dsi_ctx,
			(void *)pi)) {
		PHY_ERROR(("wl%d %s: ulp_p1_module_register failed\n", PI_INSTANCE(pi),
			__FUNCTION__));
		goto fail;
	}
#endif /* BCMULP */

	/* By default, DS1 napping will be disabled */
	info->dp->ds1_napping_enable = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_ds1nap, 0);

	return info;

	/* error handling */
fail:
	if (info) {

		if (info->dp)
			phy_mfree(pi, info->dp, sizeof(phy_ac_dsi_params_t));

		phy_mfree(pi, info, sizeof(phy_ac_dsi_info_t));
	}

	return NULL;
}

void
BCMATTACHFN(phy_ac_dsi_unregister_impl)(phy_ac_dsi_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_dsi_info_t *di = info->di;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_dsi_unregister_impl(di);

	if (info->dp)
		phy_mfree(pi, info->dp, sizeof(phy_ac_dsi_params_t));

	phy_mfree(pi, info, sizeof(phy_ac_dsi_info_t));
}

static void
dsi_update_snapshot(phy_info_t *pi, fcbs_input_data_t *save_seq)
{
	while (save_seq->type != FCBS_TYPE_MAX) {
		switch (save_seq->type) {
			case FCBS_PHY_REG:
				if (save_seq->flags & FCBS_PHY_RADIO_DYNAMIC) {
					dsi_update_phy_dyn_data(pi,
							(phy_rad_dyn_adp_t *)save_seq->data,
							save_seq->data_size);
				} else {
					dsi_save_phyregs(pi, (adp_t *)save_seq->data,
							save_seq->data_size);
				}
				break;

			case FCBS_RADIO_REG:
				if (save_seq->flags & FCBS_PHY_RADIO_DYNAMIC) {
					dsi_update_radio_dyn_data(pi,
							(phy_rad_dyn_adp_t *)save_seq->data,
							save_seq->data_size);
				} else {
					dsi_save_radioregs(pi, (adp_t *)save_seq->data,
							save_seq->data_size);
				}
				break;

			case FCBS_PHY_TBL:
				dsi_save_phytbls(pi, (phytbl_t *)save_seq->data,
						save_seq->data_size);
				break;

			case FCBS_DELAY:
				DSI_DBG(("Skipping delay\n"));
				break;

			default:
				DSI_DBG(("Error! Invalid Type!\n"));
				break;
		}
		/* Move to next element */
		save_seq++;
	}
}

static void
dsi_save_phytbls(phy_info_t *pi, phytbl_t *phytbl, uint16 len)
{
	uint16 i;
	uint8 core = 0;

	/* Read and save phy table */
	for (i = 0; i < len; i++) {
		DSI_DBG(("PHY_TABLE : Tbl_id = 0x%x, Len = 0x%x, Offset = 0x%x, Width = %d\n",
				phytbl[i].id, phytbl[i].len, phytbl[i].offset, phytbl[i].width));

		/* Save table */
		if (phytbl[i].id == ACPHY_TBL_ID_EPSILONTABLE0 ||
				phytbl[i].id == ACPHY_TBL_ID_WBCAL_PTBL0 ||
				phytbl[i].id == ACPHY_TBL_ID_WBCAL_TXDELAY_BUF0 ||
				phytbl[i].id == ACPHY_TBL_ID_ETMFTABLE0) {
			wlc_phy_table_read_acphy_dac_war(pi, phytbl[i].id, phytbl[i].len,
					phytbl[i].offset, phytbl[i].width, phytbl[i].data, core);
		} else {
			wlc_phy_table_read_acphy(pi, phytbl[i].id, phytbl[i].len, phytbl[i].offset,
					phytbl[i].width, phytbl[i].data);
		}
	}
}

static void
dsi_save_phyregs(phy_info_t *pi, adp_t *phyreg, uint16 len)
{
	uint16 i;

	/* Read and save phy reg */
	for (i = 0; i < len; i++) {
		phyreg[i].data = phy_utils_read_phyreg(pi, (uint16)phyreg[i].addr);
		DSI_DBG(("PHY_REG : Addr = 0x%x, Data = 0x%x\n",
				phyreg[i].addr, phyreg[i].data));
	}
}

static void
dsi_update_phy_dyn_data(phy_info_t *pi, phy_rad_dyn_adp_t *phyreg, uint16 len)
{
	uint16 i;
	uint16 read_val;

	/* Read and save phy reg  and use static value as well */
	for (i = 0; i < len; i++) {
		read_val = phy_utils_read_phyreg(pi, (uint16)phyreg[i].addr);
		phyreg[i].data = (read_val & ~phyreg[i].mask) | phyreg[i].static_data;
		DSI_DBG(("PHY_REG : Addr = 0x%x, Mask = 0x%x, Read_val=0x%x, Static_data = 0x%x, "
				"Data = 0x%x\n", phyreg[i].addr, phyreg[i].mask, read_val,
				phyreg[i].static_data, phyreg[i].data));
	}
}

static void
dsi_save_radioregs(phy_info_t *pi, adp_t *radioreg, uint16 len)
{
	uint16 i;

	/* Read and save radio reg */
	for (i = 0; i < len; i++) {
		radioreg[i].data = phy_utils_read_radioreg(pi, (uint16)radioreg[i].addr);
		DSI_DBG(("RADIO_REG : Addr = 0x%x, Data = 0x%x\n",
				radioreg[i].addr, radioreg[i].data));
	}
}

static void
dsi_update_radio_dyn_data(phy_info_t *pi, phy_rad_dyn_adp_t *radioreg, uint16 len)
{
	uint16 i;
	uint16 read_val;

	/* Read and save radio reg  and use static value as well */
	for (i = 0; i < len; i++) {
		read_val = phy_utils_read_radioreg(pi, (uint16)radioreg[i].addr);
		radioreg[i].data = (read_val & ~radioreg[i].mask) | radioreg[i].static_data;
		DSI_DBG(("RADIO_REG : Addr = 0x%x, Mask = 0x%x, Read_val=0x%x, Static_data = 0x%x, "
				"Data = 0x%x\n", radioreg[i].addr, radioreg[i].mask, read_val,
				radioreg[i].static_data, radioreg[i].data));
	}
}

#ifdef BCMULP
static void
dsi_save_ACMAJORREV_36(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_dsi_params_t *param = pi_ac->dsii->dp;

	uint16 stall_val;
	dsi_fcbs_t *in;
	bool suspend = FALSE;

	BCM_REFERENCE(pi_ac);
	BCM_REFERENCE(param);

	suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);

	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
	}

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	in = dsi_get_ram_seq(pi);

	wlc_phy_deaf_acphy(pi, TRUE);
	ACPHY_DISABLE_STALL(pi);

	ASSERT(in != NULL);

	DSI_DBG(("**** Updating chanspec and calcache FCBS input ****\n"));

#ifdef WL_NAP
	if (param->ds1_napping_enable) {
		/* Enable napping in DS1 state */
		phy_ac_nap_enable(pi, TRUE, TRUE);
	} else {
		/* Disable napping in DS1 state */
		phy_ac_nap_enable(pi, FALSE, TRUE);
	}
#endif /* WL_NAP */

	while (in->num != 0xff) {
		/* Populate FCBS input by reading from hardware */
		dsi_update_snapshot(pi, in->data);
#ifdef BCMULP
		/* Create FCBS tuples */
		ulp_fcbs_add_dynamic_seq(in->data, FCBS_DS1_PHY_RADIO_BLOCK, in->num);
#endif /* BCMULP */
		in++;
	}

#ifdef BCMULP
	/* Enable or Disable DS1 Napping */
	ulp_fcbs_update_rom_seq(FCBS_DS1_PHY_RADIO_BLOCK,
			EXEC_NAPPING, param->ds1_napping_enable ?
			FUNC_NAPPING : FCBS_ROM_SEQ_DISABLE);
#endif /* BCMULP */

	ACPHY_ENABLE_STALL(pi, stall_val);
	wlc_phy_deaf_acphy(pi, FALSE);
	if (!suspend) {
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
	}
}
#endif /* BCMULP */

void
dsi_update_dyn_seq_ACMAJORREV_36(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_dsi_params_t *param = pi_ac->dsii->dp;

	uint16 stall_val;
	fcbs_input_data_t **dyn_seq;
	bool suspend = FALSE;
	fcbs_input_data_t *radio_pd;
	fcbs_input_data_t *update_seq;

	BCM_REFERENCE(pi_ac);
	BCM_REFERENCE(param);
	BCM_REFERENCE(radio_pd);

	suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);

	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		phy_utils_phyreg_enter(pi);
	}

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	dyn_seq = dsi_get_dyn_ram_seq(pi);

#ifndef USE_FCBS_ROM
	radio_pd = dsi_get_radio_pd_seq(pi);
#endif /* USE_FCBS_ROM */

	wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);
	ACPHY_DISABLE_STALL(pi);

	/* Update Radio minipmu PU sequence */
	DSI_DBG(("**** Updating Radio minipmu dynamic FCBS input ****\n"));
	dsi_update_snapshot(pi, dyn_seq[MINI_PMU_PU_OFF]);

#ifdef WL_DSI
	/* Radio minpmu PU sequence */
#ifdef BCMULP
	ulp_fcbs_add_dynamic_seq(dyn_seq[MINI_PMU_PU_OFF],
			FCBS_DS1_PHY_RADIO_BLOCK, DS1_EXEC_MINIPMU_PU);
#endif /* BCMULP */
	ulp_fcbs_add_dynamic_seq(dyn_seq[MINI_PMU_PU_OFF],
			FCBS_DS0_RADIO_PU_BLOCK, DS0_EXEC_MINIPMU_PU);
#ifndef USE_FCBS_ROM
	ulp_fcbs_add_dynamic_seq(radio_pd,
			FCBS_DS0_RADIO_PD_BLOCK, DS0_EXEC_RADIO_PD);
#endif /* USE_FCBS_ROM */
#endif /* WL_DSI */

	/* Update the PLL, LOGEN PU and VCO Cal sequence */
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		if (phy_ac_radio_get_data(pi_ac->radioi)->use_5g_pll_for_2g) {
			DSI_DBG(("**** Updating Radio 5G PLL to 2G PU dynamic FCBS input ****\n"));
			dsi_update_snapshot(pi, dyn_seq[PLL_5G_TO_2G_OFF]);
			update_seq = dyn_seq[PLL_5G_TO_2G_OFF];
		} else {
			DSI_DBG(("**** Updating Radio 2G PLL PU dynamic FCBS input ****\n"));
			dsi_update_snapshot(pi, dyn_seq[PLL_2G_OFF]);
			update_seq = dyn_seq[PLL_2G_OFF];
		}
	} else {
		DSI_DBG(("**** Updating Radio 5G PLL PU dynamic FCBS input ****\n"));
		dsi_update_snapshot(pi, dyn_seq[PLL_5G_OFF]);
		update_seq = dyn_seq[PLL_5G_OFF];
	}

#ifdef WL_DSI
#ifdef BCMULP
	ulp_fcbs_add_dynamic_seq(update_seq,
			FCBS_DS1_PHY_RADIO_BLOCK, DS1_EXEC_PLL_PU);
#endif /* BCMULP */
	ulp_fcbs_add_dynamic_seq(update_seq,
			FCBS_DS0_RADIO_PU_BLOCK, DS0_EXEC_PLL_PU);
#endif /* WL_DSI */

	ACPHY_ENABLE_STALL(pi, stall_val);
	wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
	if (!suspend) {
		phy_utils_phyreg_exit(pi);
		wlapi_enable_mac(pi->sh->physhim);
	}
}

#ifdef BCMULP
static void
dsi_restore(phy_type_dsi_ctx_t *ctx)
{
	phy_ac_dsi_info_t *di = (phy_ac_dsi_info_t *)ctx;
	phy_info_t *pi = di->pi;

	if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
		dsi_restore_ACMAJORREV_1(pi);
	} else {
		PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
		ASSERT(0);
	}
}

static int
dsi_save(phy_type_dsi_ctx_t *ctx)
{
	phy_ac_dsi_info_t *di = (phy_ac_dsi_info_t *)ctx;
	phy_info_t *pi = di->pi;

	/* Call phy specific save routines  */
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		/* Save PHY and Radio snapshot */
		dsi_save_ACMAJORREV_36(pi);
	} else if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
		dsi_save_ACMAJORREV_1(pi);
	} else {
		PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
		ASSERT(0);
	}

	/* Update dynamic sequence */
	dsi_update_dyn_seq(pi);

	return BCME_OK;
}
#endif /* BCMULP */

void
dsi_update_dyn_seq(phy_info_t *pi)
{
	/* Call phy specific update routines  */
	if (ACMAJORREV_36(pi->pubpi->phy_rev)) {
		/* Update Radio sequence */
		dsi_update_dyn_seq_ACMAJORREV_36(pi);
	} else if (ACMAJORREV_1(pi->pubpi->phy_rev)) {
		PHY_INFORM(("wl%d %s : There is no dynamic seq in 4335c0\n",
				PI_INSTANCE(pi), __FUNCTION__));
	} else {
		PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
		ASSERT(0);
	}
}
