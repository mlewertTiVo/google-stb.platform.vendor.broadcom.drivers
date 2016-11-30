/*
 * ACPHY One Core Listen (OCL) module implementation
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */
#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <qmath.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_type_ocl.h>
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_reg.h>
#include <phy_ac_ocl.h>

#include <phy_rstr.h>
#include <phy_utils_var.h>
#include <wlioctl.h>

/* module private states */
struct phy_ac_ocl_info {
	phy_info_t *pi;
	phy_ac_info_t *pi_ac;
	phy_ocl_info_t *ocl_info;
};

#ifdef OCL
/* ocl reset2rx */
uint16 const tiny_4349_ocl_reset2rx_cmd[] =
       {0x84, 0x4, 0x3, 0x6, 0x12, 0x11, 0x10, 0x16, 0x2a, 0x2b, 0x24, 0x85,
	0x1f, 0x1f, 0x1f, 0x1f};
uint16 const tiny_4349_ocl_reset2rx_dly[] =
       {0xa, 0xc, 0x2, 0x2, 0x4, 0x4, 0x6, 0x1, 0x4, 0x1, 0x2, 0xa, 0x1, 0x1, 0x1, 0x1};

/* oc shutoff */
uint16 const tiny_4349_ocl_shutoff_cmd[] =
       {0x84, 0x25, 0x12, 0x22, 0x11, 0x10, 0x16, 0x24, 0x2a, 0x2b, 0x85,
	0x1f, 0x1f, 0x1f, 0x1f, 0x1f};
uint16 const tiny_4349_ocl_shutoff_dly[] =
       {0xa, 0x4, 0x4, 0x2, 0x4, 0x6, 0x1, 0x2, 0x4, 0x1, 0xa, 0x1, 0x1, 0x1, 0x1, 0x1};

/* ocl tx2rx */
uint16 const tiny_4349_ocl_tx2rx_cmd[] =
	{0x84, 0x4, 0x3, 0x6, 0x12, 0x11, 0x10, 0x16, 0x2a, 0x24, 0x0, 0x24, 0x2b, 0x0, 0x85, 0x1F};

uint16 const tiny_4349_ocl_tx2rx_dly[] =
	{0x8, 0x8, 0x4, 0x2, 0x2, 0x3, 0x4, 0x6, 0x4, 0x1, 0x2, 0x1, 0x40, 0x1, 0x1, 0x2, 0x1};
/* ocl rx2tx */
uint16 const tiny_4349_ocl_rx2tx_cmd[] =
       {0, 0x1, 0x2, 0x8, 0x5, 0x6, 0x3, 0xf, 0x4, 0x35, 0xf, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f};
uint16 const tiny_4349_ocl_rx2tx_dly[] =
       {0x1, 0x6, 0x6, 0x4, 0x4, 0x10, 0x26, 0x2, 0x5, 0x4, 0xFA, 0xFA, 0x1, 0x1, 0x1, 0x1};
uint16 const tiny_4349_ocl_rx2tx_tssi_sleep_cmd[] =
	{0, 0x1, 0x2, 0x8, 0x5, 0x6, 0x3, 0xf, 0x4, 0x35, 0xf, 0x00, 0x00, 0x84, 0x36, 0x1f};
uint16 const tiny_4349_ocl_rx2tx_tssi_sleep_dly[] =
	{0x1, 0x6, 0x6, 0x4, 0x4, 0x10, 0x26, 0x2, 0x5, 0x4, 0xFA, 0xFA, 0x88, 0xa, 0x1, 0x1};

/* ocl wake on crs */
uint16 const tiny_4349_ocl_wakeoncrs_cmd[] =
	{0x84, 0x12, 0x23, 0x15, 0x16, 0x18, 0x19, 0x24, 0x2a, 0x2b, 0x85, 0x1f,
		0x1f, 0x1f, 0x1f, 0x1f};
uint16 const tiny_4349_ocl_wakeoncrs_dly[] =
	{0x8, 0x4, 0x2, 0x6, 0x12, 0x8, 0x1, 0x2, 0x4, 0x1, 0x1, 0x4, 0x1,  0x1, 0x1, 0x1, 0x1};

/* ocl wake on clip */
uint16 const tiny_4349_ocl_wakeonclip_cmd[] =
	{0x84, 0x12, 0x15, 0x17, 0x00, 0x16, 0x24, 0x2a, 0x2b, 0x85, 0x1f, 0x1f,
		0x1f, 0x1f, 0x1f, 0x1f};
uint16 const tiny_4349_ocl_wakeonclip_dly[] =
	{0x8, 0x4, 0x2, 0x2, 0x2, 0x2, 0x1, 0x2, 0x4, 0x4, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1};

uint16 const ocl_wakeoncrs_cmd_rev40[] = {0x12, 0x15, 0x16, 0x18, 0x00, 0x24, 0x1f};
uint16 const ocl_wakeoncrs_dly_rev40[] = {0x04, 0x16, 0x12, 0x08, 0x10, 0x02, 0x01};

uint16 const ocl_wakeonclip_cmd_rev40[] = {0x12, 0x15, 0x17, 0x00, 0x16, 0x24, 0x1f};
uint16 const ocl_wakeonclip_dly_rev40[] = {0x04, 0x02, 0x02, 0x02, 0x01, 0x02, 0x01};

extern uint16 const tiny_rfseq_rx2tx_tssi_sleep_cmd[];
extern uint16 const tiny_rfseq_rx2tx_tssi_sleep_dly[];

/* local functions */
static void phy_ac_ocl_nvram_attach(phy_info_t *pi);

/* register phy type specific implementation */
phy_ac_ocl_info_t*
BCMATTACHFN(phy_ac_ocl_register_impl)(phy_info_t *pi,
	phy_ac_info_t *pi_ac, phy_ocl_info_t *ocli)
{
	phy_ac_ocl_info_t *ac_ocl_info;
	phy_type_ocl_fns_t fns;
	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_ocl_info = phy_malloc(pi, sizeof(phy_ac_ocl_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	ac_ocl_info->pi = pi;
	ac_ocl_info->pi_ac = pi_ac;
	ac_ocl_info->ocl_info = ocli;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));

	fns.ocl = wlc_phy_ocl_enable_acphy;

	fns.ctx = pi;

	/* Read srom params from nvram */
	phy_ac_ocl_nvram_attach(pi);

	phy_ocl_register_impl(ocli, &fns);

	return ac_ocl_info;
	/* error handling */
fail:
	if (ac_ocl_info != NULL)
		phy_mfree(pi, ac_ocl_info, sizeof(phy_ac_ocl_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_ocl_unregister_impl)(phy_ac_ocl_info_t *ac_ocl_info)
{
	phy_info_t *pi = ac_ocl_info->pi;
	phy_ocl_info_t *ocli = ac_ocl_info->ocl_info;
	PHY_TRACE(("%s\n", __FUNCTION__));
	/* unregister from common */
	phy_ocl_unregister_impl(ocli);
	phy_mfree(pi, ac_ocl_info, sizeof(phy_ac_ocl_info_t));
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

void
wlc_phy_ocl_enable_acphy(phy_info_t *pi, bool enable)
{
	uint16 core;
	bool ocl_en_status = (READ_PHYREGFLD(pi, OCLControl1, ocl_mode_enable) == 1);
	if (enable == ocl_en_status)
		return;
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	MOD_PHYREG(pi, RxFeCtrl1, disable_stalls, 1);
	MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 1);
	wlc_phy_stay_in_carriersearch_acphy(pi, TRUE);
	OSL_DELAY(10);
	if (enable && (pi->sh->phyrxchain == 3)) {
		MOD_PHYREG(pi, RfseqTrigger, en_pkt_proc_dcc_ctrl, 0);
		MOD_PHYREG(pi, fineRxclockgatecontrol, useOclRxfrontEndGating, 1);
		MOD_PHYREG(pi, OCLControl1, ocl_mode_enable, 1);
		if (TINY_RADIO(pi)) {
			if (pi->u.pi_acphy->ocl_coremask == 1) {
				WRITE_PHYREG(pi, PREMPT_per_pkt_en1, 0);
			} else {
				WRITE_PHYREG(pi, PREMPT_per_pkt_en0, 0);
			}
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x00, 16,
				tiny_4349_ocl_rx2tx_tssi_sleep_cmd);
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 112, 16,
				tiny_4349_ocl_rx2tx_tssi_sleep_dly);
			MOD_PHYREG(pi, AntDivConfig2059, bphy_core_div, 0);
		}
		if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, HPFBWovrdigictrl, bphy_core_sel_ovr, 1);
			if (HW_ACMINORREV(pi) == 0) {
				MOD_PHYREG(pi, fineRxclockgatecontrol, EncodeGainClkEn, 0);
				MOD_PHYREG(pi, RxFeCtrl1, forceSdFeClkEn, 3);
			}
		}
	} else {
		MOD_PHYREG(pi, OCLControl1, ocl_mode_enable, 0);
		MOD_PHYREG(pi, RfseqTrigger, en_pkt_proc_dcc_ctrl, 1);
		MOD_PHYREG(pi, fineRxclockgatecontrol, useOclRxfrontEndGating, 0);
		if (TINY_RADIO(pi)) {
			FOREACH_CORE(pi, core) {
				WRITE_PHYREGCE(pi, PREMPT_per_pkt_en, core, 0x3d);
			}
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x00, 16,
				tiny_rfseq_rx2tx_tssi_sleep_cmd);
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 112, 16,
				tiny_rfseq_rx2tx_tssi_sleep_dly);
			MOD_PHYREG(pi, AntDivConfig2059, bphy_core_div, 1);
		}
		if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, HPFBWovrdigictrl, bphy_core_sel_ovr, 0);
			if (HW_ACMINORREV(pi) == 0) {
				MOD_PHYREG(pi, fineRxclockgatecontrol, EncodeGainClkEn, 1);
				MOD_PHYREG(pi, RxFeCtrl1, forceSdFeClkEn, 0);
			}
		}
	}
	wlc_phy_stay_in_carriersearch_acphy(pi, FALSE);
	wlc_phy_resetcca_acphy(pi);
	MOD_PHYREG(pi, RxFeCtrl1, disable_stalls, 0);
	MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 0);
	wlapi_enable_mac(pi->sh->physhim);
}


static void
BCMATTACHFN(phy_ac_ocl_nvram_attach)(phy_info_t *pi)
{
	uint8 ocl, ocl_cm;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	ocl = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_ocl, 0x0);
	ocl_cm = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_ocl_cm, 0x1);
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		if ((ocl_cm != 1) && (ocl_cm != 2))
			ocl_cm = 1;
	}

	pi_ac->ocl_en = ocl;
	pi_ac->ocl_coremask = ocl_cm;
	/* nvram to control init ocl_disable state */
	if (pi_ac->ocl_en)
		pi_ac->ocl_disable_reqs = 0;
	else
		pi_ac->ocl_disable_reqs = OCL_DISABLED_HOST;
}
/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */


void
wlc_phy_ocl_apply_coremask_acphy(phy_info_t *pi)
{
	uint8 noop = 0;

	if (phy_get_phymode(pi) == PHYMODE_MIMO) {
		/* write ocl wakeonclip cmd and dly */
		si_core_cflags(pi->sh->sih, SICF_PHYMODE, 0x20000);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x230, 16,
			tiny_4349_ocl_wakeonclip_cmd);
		si_core_cflags(pi->sh->sih, SICF_PHYMODE, 0x40000);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x230, 16,
			tiny_4349_ocl_wakeonclip_cmd);
		si_core_cflags(pi->sh->sih, SICF_PHYMODE, 0x00000);

		/* Replace 0x84 and 0x85 in ocl wakeoncrs and wakeonclip
		   with noop only on active core
		   on 4349 variants phyreg or tables written with phymode 0x20000
		   gets written only to core0 even if they are common reg or table's
		*/
		if (pi->u.pi_acphy->ocl_coremask == 1) {
			/* switch phymode to 0x20000. writes happen only on core0 */
			si_core_cflags(pi->sh->sih, SICF_PHYMODE, 0x20000);
		} else {
			/* switch phymode to 0x40000. writes happen only on core1 */
			si_core_cflags(pi->sh->sih, SICF_PHYMODE, 0x40000);
		}
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x240, 16, &noop);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x24a, 16, &noop);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x230, 16, &noop);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x239, 16, &noop);
		si_core_cflags(pi->sh->sih, SICF_PHYMODE, 0x00000);

		MOD_PHYREG(pi, OCLControl1, ocl_rx_core_mask, pi->u.pi_acphy->ocl_coremask);
	}
}

void
wlc_phy_ocl_config_acphy(phy_info_t *pi)
{
	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		wlc_phy_ocl_config_28nm(pi);

		wlc_phy_set_ocl_crs_peak_thresh_28nm(pi);

		wlc_phy_ocl_apply_coremask_28nm(pi);
	} else {
		if (phy_get_phymode(pi) == PHYMODE_MIMO) {
			wlc_phy_ocl_config_rfseq_phy(pi);

			wlc_phy_ocl_config_phyreg(pi);

			/* copy ocl crs peak thresholds from regular crs peak thresholds */
			wlc_phy_set_ocl_crs_peak_thresh(pi);

			wlc_phy_ocl_apply_coremask_acphy(pi);
		}
	}
}

void
wlc_phy_ocl_config_rfseq_phy(phy_info_t *pi)
{
	uint16 regval = 0;

	if (phy_get_phymode(pi) == PHYMODE_MIMO) {
		/* write ocl shutoff cmd and dly */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x260, 16,
			tiny_4349_ocl_shutoff_cmd);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x3f0, 16,
			tiny_4349_ocl_shutoff_dly);

		/* write ocl tx2rx cmd and dly */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x280, 16,
			tiny_4349_ocl_tx2rx_cmd);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x310, 16,
			tiny_4349_ocl_tx2rx_dly);

		/* write ocl reset2rx cmd and dly */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x290, 16,
			tiny_4349_ocl_reset2rx_cmd);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x320, 16,
			tiny_4349_ocl_reset2rx_dly);

		/* write ocl wakeoncrs cmd and dly */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x240, 16,
			tiny_4349_ocl_wakeoncrs_cmd);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x2d0, 16,
			tiny_4349_ocl_wakeoncrs_dly);

		/* write ocl wakeonclip cmd and dly */
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x230, 16,
			tiny_4349_ocl_wakeonclip_cmd);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x2c0, 16,
			tiny_4349_ocl_wakeonclip_dly);


		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ,
			1, 0x14B, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x210, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x170, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x176, 16, &regval);

		regval = 0xFFFC;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x212, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x177, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x17d, 16, &regval);


		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ,
			1, 0x15B, 16, &regval);

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x340, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x180, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x186, 16, &regval);

		regval = 0xFFFC;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x342, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x187, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x18d, 16, &regval);

		regval = 0xAEEF;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x36F, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x37F, 16, &regval);

		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ,
			1, 0xF6, 16, &regval);

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1D3, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1D0, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1D1, 16, &regval);

		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ,
			1, 0xF9, 16, &regval);

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1D7, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1D4, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1D5, 16, &regval);

		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ,
			1, 0xF7, 16, &regval);

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1E3, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1E0, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1E1, 16, &regval);

		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ,
			1, 0xFA, 16, &regval);

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1E7, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1E4, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1E5, 16, &regval);

		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ,
			1, 0x129, 16, &regval);

		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x3B9, 16, &regval);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x3Bb, 16, &regval);
	}
}


void
wlc_phy_ocl_config_phyreg(phy_info_t *pi)
{
	/* Make gain backoff on sleeping core 0 */
	WRITE_PHYREG(pi, OCL_WakeOnDetect_Backoff0, 0);
	WRITE_PHYREG(pi, OCL_WakeOnDetect_Backoff1, 0);

	MOD_PHYREG(pi, OCL_tx2rx_Ctrl, use_tx2rx_seq_in_ocl, 0);
	MOD_PHYREG(pi, OCLControl5, use_tx2rx_seq_in_ocl, 0);

	MOD_PHYREG(pi, OCLControl1, ocl_bphy_dontwakeup_core, 1);
	MOD_PHYREG(pi, OCLControl1, ocl_core_mask_ovr, 1);
	MOD_PHYREG(pi, OCLControl1, dsss_cck_scd_shutOff_enable, 0);

	/* force c-str on active core only when wake on crs happens */
	MOD_PHYREG(pi, OCLControl2, wo_det_hipwrant_sel, 1);
	/* force f-str, cfo, ffo to active core only when wake on crs happens */
	MOD_PHYREG(pi, OCLControl2, scale_ant_weights_ocl_wo_det, 0);
	MOD_PHYREG(pi, OCLControl2, use_only_active_core_cfo_ffo, 1);
	MOD_PHYREG(pi, OCLControl2, OCLcckdigigainEnCntValue,
		READ_PHYREGFLD(pi, overideDigiGain1, cckdigigainEnCntValue));
}

void
wlc_phy_set_ocl_crs_peak_thresh(phy_info_t *pi)
{
	MOD_PHYREG(pi, OCL_crsThreshold2uSub1, OCL_peakDiffThresh,
		READ_PHYREGFLD(pi, crsThreshold2uSub10, peakDiffThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2uSub1, OCL_peakThresh,
		READ_PHYREGFLD(pi, crsThreshold2uSub10, peakThresh));
	MOD_PHYREG(pi, OCL_crsThreshold3uSub1, OCL_peakValThresh,
		READ_PHYREGFLD(pi, crsThreshold3uSub10, peakValThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2lSub1, OCL_peakDiffThresh,
		READ_PHYREGFLD(pi, crsThreshold2lSub10, peakDiffThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2lSub1, OCL_peakThresh,
		READ_PHYREGFLD(pi, crsThreshold2lSub10, peakThresh));
	MOD_PHYREG(pi, OCL_crsThreshold3lSub1, OCL_peakValThresh,
		READ_PHYREGFLD(pi, crsThreshold3lSub10, peakValThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2u, OCL_peakDiffThresh,
		READ_PHYREGFLD(pi, crsThreshold2u0, peakDiffThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2u, OCL_peakThresh,
		READ_PHYREGFLD(pi, crsThreshold2u0, peakThresh));
	MOD_PHYREG(pi, OCL_crsThreshold3u, OCL_peakValThresh,
		READ_PHYREGFLD(pi, crsThreshold3u0, peakValThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2l, OCL_peakDiffThresh,
		READ_PHYREGFLD(pi, crsThreshold2l0, peakDiffThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2l, OCL_peakThresh,
		READ_PHYREGFLD(pi, crsThreshold2l0, peakThresh));
	MOD_PHYREG(pi, OCL_crsThreshold3l, OCL_peakValThresh,
		READ_PHYREGFLD(pi, crsThreshold3l0, peakValThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1uSub1, OCL_highpowautoThresh2,
		READ_PHYREGFLD(pi, crshighpowThreshold1uSub10, highpowautoThresh2));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1uSub1, OCL_highpowpeakValThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold1uSub10, highpowpeakValThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1lSub1, OCL_highpowautoThresh2,
		READ_PHYREGFLD(pi, crshighpowThreshold1lSub10, highpowautoThresh2));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1lSub1, OCL_highpowpeakValThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold1lSub10, highpowpeakValThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1u, OCL_highpowautoThresh2,
		READ_PHYREGFLD(pi, crshighpowThreshold1u0, highpowautoThresh2));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1u, OCL_highpowpeakValThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold1u0, highpowpeakValThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1l, OCL_highpowautoThresh2,
		READ_PHYREGFLD(pi, crshighpowThreshold1l0, highpowautoThresh2));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1l, OCL_highpowpeakValThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold1l0, highpowpeakValThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2uSub1, OCL_highpowpeakDiffThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2uSub10, highpowpeakDiffThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2uSub1, OCL_highpowpeakThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2uSub10, highpowpeakThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2lSub1, OCL_highpowpeakDiffThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2lSub10, highpowpeakDiffThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2lSub1, OCL_highpowpeakThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2lSub10, highpowpeakThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2u, OCL_highpowpeakDiffThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2u0, highpowpeakDiffThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2u, OCL_highpowpeakThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2u0, highpowpeakThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2l, OCL_highpowpeakDiffThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2l0, highpowpeakDiffThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2l, OCL_highpowpeakThresh,
		READ_PHYREGFLD(pi, crshighpowThreshold2l0, highpowpeakThresh));
}

void
wlc_phy_set_ocl_crs_peak_thresh_28nm(phy_info_t *pi)
{
	uint8 core;
	if (pi->u.pi_acphy->ocl_coremask == 1) {
		core = 0;
	} else {
		core = 1;
	}
	MOD_PHYREG(pi, OCL_crsThreshold2u, OCL_peakDiffThresh,
		READ_PHYREGFLDCE(pi, crsThreshold2u, core, peakDiffThresh));
	MOD_PHYREG(pi, OCL_crsThreshold2u, OCL_peakThresh,
		READ_PHYREGFLDCE(pi, crsThreshold2u, core, peakThresh));
	MOD_PHYREG(pi, OCL_crsThreshold3u, OCL_peakValThresh,
		READ_PHYREGFLDCE(pi, crsThreshold3u, core, peakValThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1u, OCL_highpowautoThresh2,
		READ_PHYREGFLDCE(pi, crshighpowThreshold1u, core, highpowautoThresh2));
	MOD_PHYREG(pi, OCL_crshighpowThreshold1u, OCL_highpowpeakValThresh,
		READ_PHYREGFLDCE(pi, crshighpowThreshold1u, core, highpowpeakValThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2u, OCL_highpowpeakDiffThresh,
		READ_PHYREGFLDCE(pi, crshighpowThreshold2u, core, highpowpeakDiffThresh));
	MOD_PHYREG(pi, OCL_crshighpowThreshold2u, OCL_highpowpeakThresh,
		READ_PHYREGFLDCE(pi, crshighpowThreshold2u, core, highpowpeakThresh));

	if (CHSPEC_IS40(pi->radio_chanspec)) {
		MOD_PHYREG(pi, OCL_crsThreshold2l, OCL_peakDiffThresh,
			READ_PHYREGFLDCE(pi, crsThreshold2l, core, peakDiffThresh));
		MOD_PHYREG(pi, OCL_crsThreshold2l, OCL_peakThresh,
			READ_PHYREGFLDCE(pi, crsThreshold2l, core, peakThresh));
		MOD_PHYREG(pi, OCL_crsThreshold3l, OCL_peakValThresh,
			READ_PHYREGFLDCE(pi, crsThreshold3l, core, peakValThresh));
		MOD_PHYREG(pi, OCL_crshighpowThreshold1l, OCL_highpowautoThresh2,
			READ_PHYREGFLDCE(pi, crshighpowThreshold1l, core, highpowautoThresh2));
		MOD_PHYREG(pi, OCL_crshighpowThreshold1l, OCL_highpowpeakValThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold1l, core, highpowpeakValThresh));
		MOD_PHYREG(pi, OCL_crshighpowThreshold2l, OCL_highpowpeakDiffThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold2l, core, highpowpeakDiffThresh));
		MOD_PHYREG(pi, OCL_crshighpowThreshold2l, OCL_highpowpeakThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold2l, core, highpowpeakThresh));
	}

	if (CHSPEC_IS80(pi->radio_chanspec)) {
		MOD_PHYREG(pi, OCL_crsThreshold2uSub1, OCL_peakDiffThresh,
			READ_PHYREGFLDCE(pi, crsThreshold2uSub1, core, peakDiffThresh));
		MOD_PHYREG(pi, OCL_crsThreshold2uSub1, OCL_peakThresh,
			READ_PHYREGFLDCE(pi, crsThreshold2uSub1, core, peakThresh));
		MOD_PHYREG(pi, OCL_crsThreshold3uSub1, OCL_peakValThresh,
			READ_PHYREGFLDCE(pi, crsThreshold3uSub1, core, peakValThresh));
		MOD_PHYREG(pi, OCL_crsThreshold2lSub1, OCL_peakDiffThresh,
			READ_PHYREGFLDCE(pi, crsThreshold2lSub1, core, peakDiffThresh));
		MOD_PHYREG(pi, OCL_crsThreshold2lSub1, OCL_peakThresh,
			READ_PHYREGFLDCE(pi, crsThreshold2lSub1, core, peakThresh));
		MOD_PHYREG(pi, OCL_crsThreshold3lSub1, OCL_peakValThresh,
			READ_PHYREGFLDCE(pi, crsThreshold3lSub1, core, peakValThresh));

		MOD_PHYREG(pi, OCL_crshighpowThreshold1uSub1, OCL_highpowautoThresh2,
			READ_PHYREGFLDCE(pi, crshighpowThreshold1uSub1, core, highpowautoThresh2));
		MOD_PHYREG(pi, OCL_crshighpowThreshold1uSub1, OCL_highpowpeakValThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold1uSub1, core,
				highpowpeakValThresh));
		MOD_PHYREG(pi, OCL_crshighpowThreshold1lSub1, OCL_highpowautoThresh2,
			READ_PHYREGFLDCE(pi, crshighpowThreshold1lSub1, core, highpowautoThresh2));
		MOD_PHYREG(pi, OCL_crshighpowThreshold1lSub1, OCL_highpowpeakValThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold1lSub1, core,
				highpowpeakValThresh));

		MOD_PHYREG(pi, OCL_crshighpowThreshold2uSub1, OCL_highpowpeakDiffThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold2uSub1, core,
				highpowpeakDiffThresh));
		MOD_PHYREG(pi, OCL_crshighpowThreshold2uSub1, OCL_highpowpeakThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold2uSub1, core, highpowpeakThresh));
		MOD_PHYREG(pi, OCL_crshighpowThreshold2lSub1, OCL_highpowpeakDiffThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold2lSub1, core,
				highpowpeakDiffThresh));
		MOD_PHYREG(pi, OCL_crshighpowThreshold2lSub1, OCL_highpowpeakThresh,
			READ_PHYREGFLDCE(pi, crshighpowThreshold2lSub1, core, highpowpeakThresh));
	}
}

void
wlc_phy_ocl_config_28nm(phy_info_t *pi)
{
	uint16 regval = 0;
	regval = 0x82df;
	//trsw_rx_pwrup(fast_nap_bias_pu) set to 1
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x170, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x180, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x176, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x186, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x210, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x340, 16, &regval);

	regval = 0xfff8;
	//sw_tia_bq1 is connected to rx2g_gm_bypass in 4347A0, set to 0 to avoid gm bypass
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x177, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x187, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x179, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x189, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x17b, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x18b, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x17d, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x18d, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x212, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x342, 16, &regval);

	//copy regular init gain for ocl
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xf9, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d4, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d5, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d6, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d7, 16, &regval);

	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xf6, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d0, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d1, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d2, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1d3, 16, &regval);

	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xfa, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e4, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e5, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e6, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e7, 16, &regval);

	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xf7, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e0, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e1, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e2, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1e3, 16, &regval);

	//update wake_on_clip
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 7, 0x230, 16, ocl_wakeonclip_cmd_rev40);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 7, 0x2c0, 16, ocl_wakeonclip_dly_rev40);

	//update wake_on_crs
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 7, 0x240, 16, ocl_wakeoncrs_cmd_rev40);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 7, 0x2d0, 16, ocl_wakeoncrs_dly_rev40);

	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		if (HW_ACMINORREV(pi) == 0) {
			MOD_PHYREG(pi, OCLControl1, dsss_cck_scd_shutOff_enable, 0);
			MOD_PHYREG(pi, ocl_inactivecore_gain_ctrl,
					ocl_inactivecore_wakeon_crs_defer_pktgain, 0);
			//move to the enable function
			//MOD_PHYREG(pi, fineRxclockgatecontrol, EncodeGainClkEn, 0);
			//MOD_PHYREG(pi, RxFeCtrl1, forceSdFeClkEn, 3);
		} else {
			//MOD_PHYREG(pi, fineRxclockgatecontrol, EncodeGainClkEn, 1);
			MOD_PHYREG(pi, OCLControl1, dsss_cck_scd_shutOff_enable, 1);
			//MOD_PHYREG(pi, RxFeCtrl1, forceSdFeClkEn, 0);
			MOD_PHYREG(pi, ocl_inactivecore_gain_ctrl,
					ocl_inactivecore_wakeon_crs_defer_pktgain, 1);
			MOD_PHYREG(pi, ocl_inactivecore_gain_ctrl,
					ocl_inactivecore_wakeon_crs_defer_pktgain_initindx, 1);
			WRITE_PHYREG(pi, ocl_ant_decision_est_holdoff_ctr_ofdm_det, 64);
			WRITE_PHYREG(pi, ocl_ant_decision_est_holdoff_ctr_cck_det, 64);
		}

	}

	//MOD_PHYREG(pi, fineRxclockgatecontrol, useOclRxfrontEndGating, 1);
	MOD_PHYREG(pi, OCL_WakeOnDetect_Backoff0, ocl_cck_gain_backoff_db, 0);
	MOD_PHYREG(pi, OCL_WakeOnDetect_Backoff1, ocl_cck_gain_backoff_db, 0);
	MOD_PHYREG(pi, OCLControl2, OCLcckdigigainEnCntValue, 0xff);
	MOD_PHYREG(pi, OCLControl1, ocl_wake_on_energy_en, 0);
	MOD_PHYREG(pi, OCLControl1, dis_ocl_ed_if_clip, 1);
	MOD_PHYREG(pi, OCLControl2, use_gud_pkt_active_core, 1);

	//LOGEN related
	regval = 0x9000;
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xe2, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0xe4, 16, &regval);

	regval = 0xfee0;
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1a0, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1b0, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1a7, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1b7, 16, &regval);

	regval = 0xfeff;
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1a2, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1b2, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1a6, 16, &regval);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x1b6, 16, &regval);


}

void
wlc_phy_ocl_apply_coremask_28nm(phy_info_t *pi)
{
	uint8 ocl_listen_core;
	if (pi->u.pi_acphy->ocl_coremask == 1) {
		ocl_listen_core = 0;
		MOD_PHYREGCE(pi, pllLogenMaskCtrl, 0, logen_reset_mask, 0);
		MOD_PHYREGCE(pi, pllLogenMaskCtrl, 1, logen_reset_mask, 1);
	} else {
		ocl_listen_core = 1;
		MOD_PHYREGCE(pi, pllLogenMaskCtrl, 0, logen_reset_mask, 1);
		MOD_PHYREGCE(pi, pllLogenMaskCtrl, 1, logen_reset_mask, 0);
	}
	MOD_PHYREG(pi, OCLControl1, ocl_core_mask_ovr, 1);
	MOD_PHYREG(pi, OCLControl1, ocl_rx_core_mask, pi->u.pi_acphy->ocl_coremask);
	//move to the enable function
	//MOD_PHYREG(pi, HPFBWovrdigictrl, bphy_core_sel_ovr, 1);
	MOD_PHYREG(pi, HPFBWovrdigictrl, bphy_core_sel_val, ocl_listen_core);
	//if (ACMAJORREV_40(pi->pubpi->phy_rev) && HW_ACMINORREV(pi) == 1) {
	//	MOD_PHYREG(pi, ocl_sgi_eventDelta_AdjCount, scd_core_ovr_en, 1);
	//	MOD_PHYREG(pi, ocl_sgi_eventDelta_AdjCount, scd_core_ovr_val, ocl_listen_core);
	//}
}
#endif /* OCL */
