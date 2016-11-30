/*
 * ACPHY Gain Table loading specific portion of Broadcom BCM43XX 802.11abgn
 * Networking Device Driver.
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
 * $Id: wlc_phy_ac_gains.c 647804 2016-07-07 15:46:23Z ernst $
 */

#include <wlc_cfg.h>

#if (ACCONF != 0) || (ACCONF2 != 0)
#include <typedefs.h>
#include <osl.h>
#include <bcmwifi_channels.h>

#include "wlc_phy_types.h"
#include "wlc_phy_int.h"
#include "wlc_phyreg_ac.h"
#include "wlc_phytbl_ac.h"
#include "wlc_phytbl_20691.h"
#include "wlc_phytbl_20693.h"
#include <wlc_radioreg_20694.h>
#include <wlc_radioreg_20695.h>
#include "wlc_phytbl_ac_gains.h"
#include "wlc_phy_ac_gains.h"
#include <phy_ac_papdcal.h>
#include <phy_ac_tbl.h>
#include <phy_utils_reg.h>
#include <phy_mem.h>
static const uint16 *wlc_phy_get_tiny_txgain_tbl(phy_info_t *pi);

const uint16 *
wlc_phy_get_txgain_tbl_20695(phy_info_t *pi)
{
	const uint16 *tx_pwrctrl_tbl = NULL;

	ASSERT((RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)));

	tx_pwrctrl_tbl = (PHY_IPA(pi)) ? txgain_20695_phyrev36_2g_ipa
			: ((CHSPEC_IS2G(pi->radio_chanspec)) ? txgain_20695_phyrev36_2g_epa
			: (ACMINORREV_0(pi) ? txgain_20695_phyrev36_5g_epa
			: txgain_20695_phyrev36_5g_epa_B0));

	return tx_pwrctrl_tbl;
}

const uint16 *
wlc_phy_get_txgain_tbl_20696(phy_info_t *pi)
{
	const uint16 *tx_pwrctrl_tbl = NULL;

	ASSERT((RADIOID_IS(pi->pubpi->radioid, BCM20696_ID)));

	tx_pwrctrl_tbl = CHSPEC_IS2G(pi->radio_chanspec) ? acphy28nm_txgain_epa_2g_p5_20696a0_rev0
						: acphy28nm_txgain_epa_5g_p5_20696a0_rev0;

	return tx_pwrctrl_tbl;
}

const uint16 *
wlc_phy_get_txgain_tbl_20694(phy_info_t *pi)
{
	const uint16 *tx_pwrctrl_tbl = NULL;

	ASSERT((RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)));

	tx_pwrctrl_tbl = (CHSPEC_IS2G(pi->radio_chanspec)) ? acphy28nm_txgain_epa_2g_p5_20694a0_rev5
						: acphy28nm_txgain_epa_5g_p5_20694a0_rev5;

	return tx_pwrctrl_tbl;
}

static const uint16 *
wlc_phy_get_tiny_txgain_tbl(phy_info_t *pi)
{
	phy_info_acphy_t *pi_acphy = pi->u.pi_acphy;
	const uint16 *tx_pwrctrl_tbl = NULL;

	ASSERT((RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) ||
		(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)));

	tx_pwrctrl_tbl = (CHSPEC_IS2G(pi->radio_chanspec)) ? pi_acphy->gaintbl_2g
						: pi_acphy->gaintbl_5g;

	return tx_pwrctrl_tbl;
}

void
wlc_phy_ac_gains_load(phy_info_t *pi)
{
	uint idx;
	const uint16 *tx_pwrctrl_tbl = NULL;
	uint16 GainWord_Tbl[3];
	uint8 Gainoverwrite = 0;
	uint8 PAgain = 0xff;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	bool suspend = !(R_REG(pi->sh->osh, &pi->regs->maccontrol) & MCTL_EN_MAC);

	if (NORADIO_ENAB(pi->pubpi))
		return;

	/* Setting tx gain table resolution */
	if (ROUTER_4349(pi)) {
		pi->u.pi_acphy->is_p25TxGainTbl = TRUE;
	}

	/* Load tx gain table */
	if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
		tx_pwrctrl_tbl = wlc_phy_get_tx_pwrctrl_tbl_2069(pi);
	} else if ((RADIOID_IS(pi->pubpi->radioid, BCM20691_ID)) ||
		(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID))) {
		tx_pwrctrl_tbl = wlc_phy_get_tiny_txgain_tbl(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20694_ID)) {
		tx_pwrctrl_tbl = wlc_phy_get_txgain_tbl_20694(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20695_ID)) {
		tx_pwrctrl_tbl = wlc_phy_get_txgain_tbl_20695(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20696_ID)) {
		tx_pwrctrl_tbl = wlc_phy_get_txgain_tbl_20696(pi);
	} else {
		PHY_ERROR(("%s: Unsupported Radio!: 0x%8x\n", __FUNCTION__,
			RADIOID(pi->pubpi->radioid)));
		ASSERT(0);
		return;
	}

	ASSERT(tx_pwrctrl_tbl != NULL);

	/* suspend mac */
	if (!suspend)
		wlapi_suspend_mac_and_wait(pi->sh->physhim);

	if (PHY_PAPDEN(pi)) {
		phy_ac_papdcal_get_gainparams(pi_ac->papdcali, &Gainoverwrite, &PAgain);
	}

	if (Gainoverwrite > 0) {
		for (idx = 0; idx < 128; idx++) {
			GainWord_Tbl[0] = tx_pwrctrl_tbl[3*idx];
			GainWord_Tbl[1] = (tx_pwrctrl_tbl[3*idx+1] & 0xff00) + (0xff & PAgain);

			GainWord_Tbl[2] = tx_pwrctrl_tbl[3*idx+2];
			wlc_phy_tx_gain_table_write_acphy(pi,
				1, idx, 48, GainWord_Tbl);
		}
	} else {
		wlc_phy_tx_gain_table_write_acphy(pi,
			128, 0, 48, tx_pwrctrl_tbl);
	}

	if (TINY_RADIO(pi) && (pi_ac->enIndxCap)) {
		uint16 k, txgaintemp[3];
		uint16 txidxcap = 0;
		BCM_REFERENCE(txidxcap);
#if defined(WLC_TXCAL)
		if (!pi->olpci->olpc_idx_valid) {
#endif /* WLC_TXCAL */
			if (CHSPEC_IS2G(pi->radio_chanspec) &&
				BF3_2GTXGAINTBL_BLANK(pi->u.pi_acphy)) {
					txidxcap = pi->sromi->txidxcap2g & 0xFF;
			}
			if (CHSPEC_IS5G(pi->radio_chanspec) &&
				BF3_5GTXGAINTBL_BLANK(pi->u.pi_acphy)) {
					txidxcap = pi->sromi->txidxcap5g & 0xFF;
			}
			if (txidxcap != 0) {
				txgaintemp[0] = tx_pwrctrl_tbl[3*txidxcap];
				txgaintemp[1] = tx_pwrctrl_tbl[3*txidxcap+1];
				txgaintemp[2] = tx_pwrctrl_tbl[3*txidxcap+2];

				for (k = 0; k < txidxcap; k++) {
					wlc_phy_tx_gain_table_write_acphy(pi,
					1, k, 48, txgaintemp);
				}
			}
#if defined(WLC_TXCAL)
	}
#endif /* WLC_TXCAL */


#if defined(WLC_TXCAL)
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			if (pi->olpci->olpc_idx_in_use && pi->olpci->olpc_idx_valid) {
				uint8 core = 0, iidx;
				int8 max_fem_out;
				uint8 band = (CHSPEC_IS2G(pi->radio_chanspec) ? 0 : 1);
				/* save the original tx_power_max_per_core */
				int8 tx_power_max_per_core_orig = pi->tx_power_max_per_core[core];

				/* Gain Table Capping on the lower side */
				pi->tx_power_max_per_core[core] = pi->sromi->txidxcaplow[band];
				wlc_phy_olpc_idx_tempsense_comp_acphy(pi, &iidx, core);
				txgaintemp[0] = tx_pwrctrl_tbl[3*iidx];
				txgaintemp[1] = tx_pwrctrl_tbl[3*iidx+1];
				txgaintemp[2] = tx_pwrctrl_tbl[3*iidx+2];
				for (k = iidx + 1; k < 128; k++) {
					wlc_phy_tx_gain_table_write_acphy(pi,
					1, k, 48, txgaintemp);
				}

				/* Gain Table Capping on the higher side */
				if (pi->sromi->maxepagain[band] != 0) {
					max_fem_out = pi->sromi->maxchipoutpower[band]
						+ pi->sromi->maxepagain[band];
					pi->tx_power_max_per_core[core] = max_fem_out;
					wlc_phy_olpc_idx_tempsense_comp_acphy(pi, &iidx, core);
					txgaintemp[0] = tx_pwrctrl_tbl[3*iidx];
					txgaintemp[1] = tx_pwrctrl_tbl[3*iidx+1];
					txgaintemp[2] = tx_pwrctrl_tbl[3*iidx+2];
					for (k = 0; k < iidx; k++) {
						wlc_phy_tx_gain_table_write_acphy(pi,
						1, k, 48, txgaintemp);
					}
				}
				/* restore the original tx_power_max_per_core */
				pi->tx_power_max_per_core[core] = tx_power_max_per_core_orig;
			}
		}
#endif /* WLC_TXCAL */

	}

	/* resume mac */
	if (!suspend)
		wlapi_enable_mac(pi->sh->physhim);
}

void
wlc_phy_tx_gain_table_write_acphy(phy_info_t *pi, uint32 l, uint32 o, uint32 w, const void *d)
{
	uint8 core, tx_gain_tbl_id;
	FOREACH_CORE(pi, core) {
		tx_gain_tbl_id = wlc_phy_get_tbl_id_gainctrlbbmultluts(pi, core);
		if ((!(ACMAJORREV_4(pi->pubpi->phy_rev) ||
			ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev) ||
			ACMAJORREV_37(pi->pubpi->phy_rev))) && (core != 0) &&
			!(ACMAJORREV_5(pi->pubpi->phy_rev) && ACMINORREV_2(pi))) {
			continue;
		}
		wlc_phy_table_write_acphy(pi, tx_gain_tbl_id, l, o, w, d);
	}

}

void
BCMATTACHFN(wlc_phy_ac_delete_gain_tbl)(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	if (pi_ac->gaintbl_2g) {
		phy_mfree(pi, (void *)pi_ac->gaintbl_2g,
			sizeof(uint16)*TXGAIN_TABLES_LEN);
		pi_ac->gaintbl_2g = NULL;
	}

#ifdef BAND5G
	if (pi_ac->gaintbl_5g) {
		phy_mfree(pi, (void *)pi_ac->gaintbl_5g,
			sizeof(uint16)*TXGAIN_TABLES_LEN);
		pi_ac->gaintbl_5g = NULL;
	}
#endif
}

#endif /* (ACCONF != 0) || (ACCONF2 != 0) */
