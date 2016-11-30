/*
 * PHY Core module implementation
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
 * $Id: phy.c 644198 2016-06-17 10:39:14Z hreddy $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <phy_dbg.h>
#include <phy_rstr.h>
#include <phy_api.h>
#include "phy_iovt.h"
#include "phy_ioct.h"
#include "phy_type.h"
#include "phy_type_disp.h"
#include <phy_cmn.h>
#include <phy_mem.h>
#include <bcmutils.h>
#include <phy_init.h>
#include <phy_wd.h>
#include <phy_ana.h>
#include <phy_ana_api.h>
#include <phy_btcx_api.h>
#include <phy_et.h>
#include <phy_radio.h>
#include <phy_radio_api.h>
#include <phy_tbl.h>
#include <phy_tpc.h>
#ifdef WLC_TXPWRCAP
#include <phy_txpwrcap.h>
#endif
#include <phy_radar.h>
#include <phy_antdiv.h>
#include <phy_noise.h>
#include <phy_temp.h>
#include <phy_rssi.h>
#include <phy_btcx.h>
#include <phy_txiqlocal.h>
#include <phy_rxiqcal.h>
#include <phy_papdcal.h>
#include <phy_vcocal.h>
#include <phy_chanmgr.h>
#include <phy_chanmgr_api.h>
#include <phy_cache.h>
#include <phy_calmgr.h>
#include <phy_chanmgr_notif.h>
#include <phy_fcbs.h>
#include <phy_lpc.h>
#include <phy_dsi.h>
#include <phy_dccal.h>
#include <phy_tof.h>
#include <phy_hirssi.h>
#include <phy.h>
#include <phy_nap.h>
#include <phy_hecap.h>

#include <phy_utils_var.h>

#ifndef ALL_NEW_PHY_MOD
/* TODO: remove these lines... */
#include <wlc_phy_int.h>
#include <wlc_phy_hal.h>
#endif

#define PHY_TXPWR_MIN		9	/* default min tx power */

#define PHY_WREG_LIMIT	24	/* number of consecutive phy register write before a readback */
#define PHY_WREG_LIMIT_VENDOR 1	/* num of consec phy reg write before a readback for vendor */

/* local functions */
static void wlc_phy_srom_attach(phy_info_t *pi, int bandtype);
static void wlc_phy_std_params_attach(phy_info_t *pi);
static int _phy_init(phy_init_ctx_t *ctx);
static void phy_register_dumps(phy_info_t *pi);
static void phy_init_done(phy_info_t *pi);

#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && (defined(BCMINTERNAL) || \
	defined(DBG_PHY_IOV))) || defined(BCMDBG_PHYDUMP)
/* phydump page infra */
static void phyreg_page_parser(phy_info_t *pi, phy_regs_t *reglist, struct bcmstrbuf *b);
#endif /* BCMDBG_PHYDUMP */

#ifdef WLC_TXCAL
static int
BCMATTACHFN(phy_txcal_attach)(phy_info_t *pi)
{
	shared_phy_t *sh = pi->sh;
	osl_t *osh = sh->osh;
	txcal_root_pwr_tssi_t *pi_txcal_tssi_tbl;
	txcal_pwr_tssi_lut_t *pwr_tssi_lut_5G80, *pwr_tssi_lut_5G40, *pwr_tssi_lut_5G20;
	txcal_pwr_tssi_lut_t *pwr_tssi_lut_2G;

	pi->txcali = phy_malloc(pi, sizeof(*pi->txcali));
	if (pi->txcali) {
		if ((pi->txcali->txcal_pwr_tssi = phy_malloc(pi,
				sizeof(*pi->txcali->txcal_pwr_tssi))) == NULL) {
			PHY_ERROR(("wl%d: %s: out of memory, malloced txcal_pwr_tssi %d bytes\n",
					sh->unit, __FUNCTION__, MALLOCED(osh)));
			goto err;
		}

		if ((pi->txcali->txcal_meas = phy_malloc(pi,
				sizeof(*pi->txcali->txcal_meas))) == NULL) {
			PHY_ERROR(("wl%d: %s: out of memory, malloced txcal_meas %d bytes\n",
					sh->unit, __FUNCTION__, MALLOCED(osh)));
			goto err;
		}

		pi->txcali->txcal_root_pwr_tssi_tbl = phy_malloc(pi, sizeof(*pi_txcal_tssi_tbl));
		pi_txcal_tssi_tbl = pi->txcali->txcal_root_pwr_tssi_tbl;
		if (pi_txcal_tssi_tbl) {
			pi_txcal_tssi_tbl->root_pwr_tssi_lut_2G = phy_malloc(pi,
					sizeof(*pwr_tssi_lut_2G));
			pwr_tssi_lut_2G = pi_txcal_tssi_tbl->root_pwr_tssi_lut_2G;
			if (pwr_tssi_lut_2G) {
				if ((pwr_tssi_lut_2G->txcal_pwr_tssi = phy_malloc(pi,
						sizeof(*pwr_tssi_lut_2G->txcal_pwr_tssi))) ==
						NULL) {
					PHY_ERROR(("wl%d: %s: out of memory, malloced",
							sh->unit, __FUNCTION__));
					PHY_ERROR(("lut_2G->txcal_pwr_tssi %d bytes\n",
							MALLOCED(osh)));
					goto err;
				}
			} else {
				PHY_ERROR(("wl%d: %s: out of memory, malloced",
						sh->unit, __FUNCTION__));
				PHY_ERROR(("root_pwr_tssi_lut_2G %d bytes\n", MALLOCED(osh)));
				goto err;
			}
			pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G20 = phy_malloc(pi,
					sizeof(*pwr_tssi_lut_5G20));
			pwr_tssi_lut_5G20 = pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G20;
			if (pwr_tssi_lut_5G20) {
				if ((pwr_tssi_lut_5G20->txcal_pwr_tssi = phy_malloc(pi,
						sizeof(*pwr_tssi_lut_5G20->txcal_pwr_tssi))) ==
						NULL) {
					PHY_ERROR(("wl%d: %s: out of memory, malloced",
							sh->unit, __FUNCTION__));
					PHY_ERROR(("5G20->txcal_pwr_tssi %d bytes\n",
							MALLOCED(osh)));
					goto err;
				}
			} else {
				PHY_ERROR(("wl%d: %s: out of memory, malloced",
					sh->unit, __FUNCTION__));
				PHY_ERROR(("root_pwr_tssi_lut_5G20 %d bytes\n", MALLOCED(osh)));
				goto err;
			}
			pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G40 = phy_malloc(pi,
					sizeof(*pwr_tssi_lut_5G40));
			pwr_tssi_lut_5G40 = pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G40;
			if (pwr_tssi_lut_5G40) {
				if ((pwr_tssi_lut_5G40->txcal_pwr_tssi = phy_malloc(pi,
						sizeof(*pwr_tssi_lut_5G40->txcal_pwr_tssi))) ==
						NULL) {
					PHY_ERROR(("wl%d: %s: out of memory, malloced",
							sh->unit, __FUNCTION__));
					PHY_ERROR(("5G40->txcal_pwr_tssi %d bytes\n",
							MALLOCED(osh)));
					goto err;
				}
			} else {
				PHY_ERROR(("wl%d: %s: out of memory, malloced",
						sh->unit, __FUNCTION__));
				PHY_ERROR(("root_pwr_tssi_lut_5G40 %d bytes\n", MALLOCED(osh)));
				goto err;
			}
			pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G80 = phy_malloc(pi,
					sizeof(*pwr_tssi_lut_5G80));
			pwr_tssi_lut_5G80 = pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G80;
			if (pwr_tssi_lut_5G80) {
				if ((pwr_tssi_lut_5G80->txcal_pwr_tssi = phy_malloc(pi,
						sizeof(*pwr_tssi_lut_5G80->txcal_pwr_tssi))) ==
						NULL) {
					PHY_ERROR(("wl%d: %s: out of memory, malloced",
							sh->unit, __FUNCTION__));
					PHY_ERROR(("5G80->txcal_pwr_tssi %d bytes\n",
							MALLOCED(osh)));
					goto err;
				}
			} else {
				PHY_ERROR(("wl%d: %s: out of memory, malloced",
						sh->unit, __FUNCTION__));
				PHY_ERROR(("root_pwr_tssi_lut_5G80 %d bytes\n", MALLOCED(osh)));
				goto err;
			}
		} else {
			PHY_ERROR(("wl%d: %s: out of memory, malloced", sh->unit, __FUNCTION__));
			PHY_ERROR(("txcal_root_pwr_tssi_tbl %d bytes\n", MALLOCED(osh)));
			goto err;
		}
	} else {
		PHY_ERROR(("wl%d: %s: out of memory, malloced txcali %d bytes\n",
			sh->unit, __FUNCTION__, MALLOCED(osh)));
		goto err;
	}
	return BCME_OK;
err:
	return BCME_NOMEM;
}
#endif /* WLC_TXCAL */

void
wlc_phy_set_shmdefs(wlc_phy_t *ppi, const shmdefs_t *shmdefs)
{
	phy_info_t *pi = (phy_info_t *)ppi;
	pi->shmdefs = shmdefs;
}

/* attach/detach the PHY Core module to the system. */
phy_info_t *
BCMATTACHFN(phy_module_attach)(shared_phy_t *sh, void *regs, int bandtype, char *vars)
{
	phy_info_t *pi;
	uint32 sflags = 0;
	uint phyversion;
	osl_t *osh = sh->osh;

	PHY_TRACE(("wl: %s(%p, %d, %p)\n", __FUNCTION__, regs, bandtype, sh));

	sflags = si_core_sflags(sh->sih, 0, 0);

	if (BAND_5G(bandtype)) {
		if ((sflags & (SISF_5G_PHY | SISF_DB_PHY)) == 0) {
			PHY_ERROR(("wl%d: %s: No phy available for 5G\n",
			          sh->unit, __FUNCTION__));
			return NULL;
		}
	}

	/* Figure out if we have a phy for the requested band and attach to it */
	if ((sflags & SISF_DB_PHY) && (pi = sh->phy_head)) {
		pi->vars = vars;
		/* For the second band in dualband phys, load the band specific
		 * NVRAM parameters
		  * The second condition excludes UNO3 inorder to
		  * keep the device id as 0x4360 (dual band).
		  * Purely to be backward compatible to previous UNO3 NVRAM file.
		  *
		 */
		/* For the second band in dualband phys, just bring the core back out of reset */
		wlapi_bmac_corereset(pi->sh->physhim, pi->pubpi->coreflags);

		pi->refcnt++;
		goto exit;
	}

	/* ONLY common PI is allocated. pi->u.pi_xphy is not available yet */
	if ((pi = (phy_info_t *)MALLOCZ(osh, sizeof(phy_info_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
		return NULL;
	}
	pi->sh = sh; /* Assign sh so that phy_malloc can be used from here on */

	if ((pi->pubpi = phy_malloc(pi, sizeof(wlc_phy_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced pubpi %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->pubpi_ro = phy_malloc(pi, sizeof(wlc_phy_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced pubpi_ro %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->interf = phy_malloc(pi, sizeof(interference_info_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced interf %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->pwrdet = phy_malloc(pi, sizeof(srom_pwrdet_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced pwrdet %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->txcore_temp = phy_malloc(pi, sizeof(phy_txcore_temp_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced txcore_temp %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->def_cal_info = phy_malloc(pi, sizeof(phy_cal_info_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced def_cal_info %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

#ifdef ENABLE_FCBS
	if ((pi->phy_fcbs = phy_malloc(pi, sizeof(fcbs_info))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced phy_fcbs %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}
#endif /* ENABLE_FCBS */

	if ((pi->fem2g = phy_malloc(pi, sizeof(srom_fem_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced fem2g %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->fem5g = phy_malloc(pi, sizeof(srom_fem_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced fem5g %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

#ifdef WLSRVSDB
	if ((pi->srvsdb_state = phy_malloc(pi, sizeof(srvsdb_info_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced srvsdb_state %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}
#endif /* WLSRVSDB */

	if ((pi->pi_fptr = phy_malloc(pi, sizeof(phy_func_ptr_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced pi_fptr %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->ppr = phy_malloc(pi, sizeof(ppr_info_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced ppr %d bytes\n", sh->unit,
		          __FUNCTION__, MALLOCED(osh)));
	    goto err;
	}

	if ((pi->pwrdet_ac = phy_malloc(pi, (SROMREV(sh->sromrev) >= 12 ? sizeof(srom12_pwrdet_t)
					       : sizeof(srom11_pwrdet_t)))) == NULL) {
	    PHY_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", sh->unit,
	       __FUNCTION__, MALLOCED(sh->osh)));
	    goto err;
	}

#ifdef WLC_TXCAL
	/* txcal attach */
	if (phy_txcal_attach(pi) != BCME_OK) {
		PHY_ERROR(("%s: phy_txcal_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif /* WLC_TXCAL */
#if defined(WLC_TXCAL) || (defined(WLOLPC) && !defined(WLOLPC_DISABLED))
	if ((pi->olpci = phy_malloc(pi, sizeof(*pi->olpci))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced olpci %d bytes\n",
			sh->unit, __FUNCTION__, MALLOCED(osh)));
		goto err;
	}
#endif	/* WLC_TXCAL || (WLOLPC && !WLOLPC_DISABLED) */
	if ((pi->pdpi = phy_malloc(pi, sizeof(*pi->pdpi))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced pdpi %d bytes\n",
			sh->unit, __FUNCTION__, MALLOCED(osh)));
		goto err;
	}

	if ((pi->ff = phy_malloc(pi, sizeof(phy_feature_flags_t))) == NULL) {
		PHY_ERROR(("wl%d: %s: out of memory, malloced phy_features_enab %d bytes\n",
			sh->unit, __FUNCTION__, MALLOCED(osh)));
		goto err;
	}

	pi->regs = (d11regs_t *)regs;
	pi->vars = vars;

	/* point pi->sromi to phy_sh->sromi */
	pi->sromi = pi->sh->sromi;

	/* Good phy, increase refcnt and put it in list */
	pi->refcnt++;
	pi->next = pi->sh->phy_head;
	sh->phy_head = pi;

	/* set init power on reset to TRUE */
	pi->phy_init_por = TRUE;

	if ((pi->sh->boardvendor == VENDOR_APPLE) &&
	    (pi->sh->boardtype == 0x0093)) {
		pi->phy_wreg_limit = PHY_WREG_LIMIT_VENDOR;
	}
	else {
		pi->phy_wreg_limit = PHY_WREG_LIMIT;
	}
	if (BAND_2G(bandtype) && (sflags & SISF_2G_PHY)) {
		/* Set the sflags gmode indicator */
		pi->pubpi->coreflags = SICF_GMODE;
	}

	/* get the phy type & revision */
	/* Note: corereset seems to be required to get the phyversion read correctly */
	wlapi_bmac_corereset(pi->sh->physhim, pi->pubpi->coreflags);
	phyversion = R_REG(osh, &pi->regs->phyversion);
	pi->pubpi->phy_type = PHY_TYPE(phyversion);
	pi->pubpi->phy_rev = phyversion & PV_PV_MASK;

	/* Read the fabid */
	pi->fabid = si_fabid(GENERIC_PHY_INFO(pi)->sih);

	if (((pi->sh->chip == BCM43235_CHIP_ID) ||
	     (pi->sh->chip == BCM43236_CHIP_ID) ||
	     (pi->sh->chip == BCM43238_CHIP_ID) ||
	     (pi->sh->chip == BCM43234_CHIP_ID)) &&
	    ((pi->sh->chiprev == 2) || (pi->sh->chiprev == 3))) {
		pi->pubpi->phy_rev = 9;
	}

	/* LCNXN */
	if (pi->pubpi->phy_type == PHY_TYPE_LCNXN) {
		pi->pubpi->phy_type = PHY_TYPE_N;
		pi->pubpi->phy_rev += LCNXN_BASEREV;
	}

	/* Default to 1 core. Each PHY specific attach should initialize it
	 * to PHY/chip specific.
	 */
	pi->pubpi->phy_corenum = PHY_CORE_NUM_1;
	pi->pubpi->ana_rev = (phyversion & PV_AV_MASK) >> PV_AV_SHIFT;

	if (!VALID_PHYTYPE(pi)) {
		PHY_ERROR(("wl%d: %s: invalid phy_type %d\n",
		          sh->unit, __FUNCTION__, pi->pubpi->phy_type));
		goto err;
	}

	/* default channel and channel bandwidth is 20 MHZ */
	pi->bw = WL_CHANSPEC_BW_20;
	pi->radio_chanspec = BAND_2G(bandtype) ? CH20MHZ_CHSPEC(1) : CH20MHZ_CHSPEC(36);
	pi->radio_chanspec_sc = BAND_2G(bandtype) ? CH20MHZ_CHSPEC(1) : CH20MHZ_CHSPEC(36);

	/* attach nvram driven variables */
	wlc_phy_srom_attach(pi, bandtype);

	/* update standard configuration params to defaults */
	wlc_phy_std_params_attach(pi);


	/* ######## Attach process start ######## */


	/* ======== Attach infrastructure services ======== */

	/* Attach debug/dump registry module - MUST BE THE FIRST! */
	if ((pi->dbgi = phy_dbg_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_dbg_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach PHY Common info */
	if ((pi->cmni = phy_cmn_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_cmn_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach PHY type specific implementation dispatch info */
	if ((pi->typei = phy_type_disp_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_type_disp_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach INIT control module */
	if ((pi->initi = phy_init_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_init_attach failed\n", __FUNCTION__));
		goto err;
	}
#ifdef NEW_PHY_CAL_ARCH
	/* Attach Channel Manager Nofitication module */
	if ((pi->chanmgr_notifi = phy_chanmgr_notif_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_chanmgr_notif_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif
	/* Attach CACHE module */
	if ((pi->cachei = phy_cache_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_cache_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach WATCHDOG module */
	if ((pi->wdi = phy_wd_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_wd_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach CALibrationManaGeR module */
	if ((pi->calmgri = phy_calmgr_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_calmgr_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* ======== Attach PHY specific layer ======== */

	/* Attach PHY Core type specific implementation */
	if (pi->typei != NULL &&
	    (*(phy_type_info_t **)(uintptr)&pi->u =
	     phy_type_attach(pi->typei, bandtype)) == NULL) {
		PHY_ERROR(("%s: phy_type_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* ======== Attach modules' common layer ======== */

	/* Attach ANAcore control module */
	if ((pi->anai = phy_ana_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_ana_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach RADIO control module */
	if ((pi->radioi = phy_radio_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_radio_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach PHYTableInit module */
	if ((pi->tbli = phy_tbl_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_tbl_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach TxPowerCtrl module */
	if ((pi->tpci = phy_tpc_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_tpc_attach failed\n", __FUNCTION__));
		goto err;
	}

#if defined(WLC_TXPWRCAP) && !defined(WLC_TXPWRCAP_DISABLED)
	/* Attach TxPowerCap module */
	if ((pi->txpwrcapi = phy_txpwrcap_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_txpwrcap_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif

#if defined(AP) && defined(RADAR)
	/* Attach RadarDetect module */
	if ((pi->radari = phy_radar_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_radar_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif

	/* Attach ANTennaDIVersity module */
	if ((pi->antdivi = phy_antdiv_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_antdiv_attach failed\n", __FUNCTION__));
		goto err;
	}

#ifndef WLC_DISABLE_ACI
	/* Attach NOISE module */
	if ((pi->noisei = phy_noise_attach(pi, bandtype)) == NULL) {
		PHY_ERROR(("%s: phy_noise_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif

	/* Attach TEMPerature sense module */
	if ((pi->tempi = phy_temp_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_temp_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach RSSICompute module */
	if ((pi->rssii = phy_rssi_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_rssi_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach BlueToothCoExistence module */
	if ((pi->btcxi = phy_btcx_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_btcx_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach TxIQLOCal module */
	if ((pi->txiqlocali = phy_txiqlocal_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_txiqlocal_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach RxIQCal module */
	if ((pi->rxiqcali = phy_rxiqcal_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_rxiqcal_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach PAPDCal module */
	if ((pi->papdcali = phy_papdcal_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_papdcal_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach VCOCal module */
	if ((pi->vcocali = phy_vcocal_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_vcocal_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach CHannelManaGeR module */
	if ((pi->chanmgri = phy_chanmgr_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_chanmgr_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach FCBS module */
	if ((pi->fcbsi = phy_fcbs_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_fcbs_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach LPC module */
	if ((pi->lpci = phy_lpc_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_lpc_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach MISC module */
	if ((pi->misci = phy_misc_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_misc_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach TSSI Cal module */
	if ((pi->tssicali = phy_tssical_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_tssi_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach RXGCRS module */
	if ((pi->rxgcrsi = phy_rxgcrs_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_rxgcrs_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach RXSPUR module */
	if ((pi->rxspuri = phy_rxspur_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_rxspur_attach failed\n", __FUNCTION__));
		goto err;
	}

#ifdef SAMPLE_COLLECT
	/* Attach sample collect module */
	if ((pi->sampi = phy_samp_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_samp_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif /* SAMPLE_COLLECT */

#ifdef WL_DSI
	/* Attach DeepSleepInit module */
	if ((pi->dsii = phy_dsi_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_dsi_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif /* WL_DSI */

#ifdef WL_MU_RX
	/* Attach MU-MIMO module */
	if ((pi->mui = phy_mu_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_mu_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif  /* WL_MU_RX */

#ifdef OCL
	/* Attach OCL module */
	if ((pi->ocli = phy_ocl_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_ocl_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif  /* OCL */

#ifdef WL11AX
	/* Attach HECAP module if PHY can support it */
	if (ISHECAPPHY(pi)) {
		if ((pi->hecapi = phy_hecap_attach(pi)) == NULL) {
			PHY_ERROR(("%s: phy_hecap_attach failed\n", __FUNCTION__));
			goto err;
		}
	}
#endif /* WL11AX */

	/* Attach MU-MIMO module */
	if ((pi->dccali = phy_dccal_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_dccal_attach failed\n", __FUNCTION__));
		goto err;
	}

#ifdef WL_PROXDETECT
		/* Attach TOF module */
		if ((pi->tofi = phy_tof_attach(pi)) == NULL) {
			PHY_ERROR(("%s: phy_tof_attach failed\n", __FUNCTION__));
			goto err;
		}
#endif  /* WL_PROXDETECT */

#if defined(WL_NAP) && !defined(WL_NAP_DISABLED)
	/* Attach nap module */
	if ((pi->napi = phy_nap_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_nap_attach failed\n", __FUNCTION__));
		goto err;
	}
#endif /* WL_NAP */

	/* Attach HiRSSIeLNABypass module */
	if ((pi->hirssii = phy_hirssi_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_hirssi_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* Attach envelope tracking module */
	if ((pi->eti = phy_et_attach(pi)) == NULL) {
		PHY_ERROR(("%s: phy_et_attach failed\n", __FUNCTION__));
		goto err;
	}

	/* ...Attach other modules... */

	/* ======== Attach modules' PHY specific layer ======== */

	/* Register PHY type implementation layer to common layer */
	if (pi->typei != NULL &&
	    phy_type_register_impl(pi->typei, bandtype) != BCME_OK) {
		PHY_ERROR(("%s: phy_type_register_impl failed\n", __FUNCTION__));
		goto err;
	}

#ifdef WLTXPWR_CACHE
	pi->txpwr_cache = wlc_phy_txpwr_cache_create(pi->sh->osh);
#endif

	/* ######## Attach process end ######## */

	/* register reset fn */
	if (phy_init_add_init_fn(pi->initi, _phy_init, pi, PHY_INIT_PHYIMPL) != BCME_OK) {
		PHY_ERROR(("%s: phy_init_add_init_fn failed\n", __FUNCTION__));
		goto err;
	}

	/* Make a public copy of the attach time constant phy attributes */
	bcopy(pi->pubpi, pi->pubpi_ro, sizeof(wlc_phy_t));

	/* register dump functions */
	phy_register_dumps(pi);

exit:
	/* Mark that they are not longer available so we can error/assert.  Use a pointer
	 * to self as a flag.
	 */
	pi->vars = (char *)&pi->vars;
	return pi;

err:
	phy_module_detach(pi);
	return NULL;
}

static void
BCMATTACHFN(wlc_phy_srom_attach)(phy_info_t *pi, int bandtype)
{
	uint8 i = 0;
	uint8 j = 0;
	uint8 k = 0;

	pi->rssi_corr_normal = (int8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rssicorrnorm, 0);
	pi->rssi_corr_boardatten = (int8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rssicorratten, 7);

	/* Re-init the interference value based on the nvram variables */
	if (PHY_GETVAR_SLICE(pi, rstr_interference) != NULL) {
		pi->sh->interference_mode_2G = (int)PHY_GETINTVAR_SLICE(pi, rstr_interference);
		pi->sh->interference_mode_5G = (int)PHY_GETINTVAR_SLICE(pi, rstr_interference);

		if (BAND_2G(bandtype))
			pi->sh->interference_mode = pi->sh->interference_mode_2G;
		else
			pi->sh->interference_mode = pi->sh->interference_mode_5G;
	}

#if defined(RXDESENS_EN)
	/* phyrxdesens in db for SS SPC production */
	pi->sh->phyrxdesens = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_phyrxdesens, 0);
#endif

#ifdef BAND5G
	for (i = 0; i < 3; i++) {
		pi->rssi_corr_normal_5g[i] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_rssicorrnorm5g, i, 0);
		pi->rssi_corr_boardatten_5g[i] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_rssicorratten5g, i, 0);
	}
#endif /* BAND5G */
	/* The below parameters are used to adjust JSSI based range */
	pi->rssi_corr_perrg_2g[0] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
		rstr_rssicorrperrg2g, 0, -150);
	pi->rssi_corr_perrg_2g[1] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
		rstr_rssicorrperrg2g, 1, -150);
#ifdef BAND5G
	pi->rssi_corr_perrg_5g[0] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
		rstr_rssicorrperrg5g, 0, -150);
	pi->rssi_corr_perrg_5g[1] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
		rstr_rssicorrperrg5g, 1, -150);
#endif /* BAND5G */
	for (i = 2; i < 5; i++) {
		pi->rssi_corr_perrg_2g[i] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_rssicorrperrg2g, i, 0);
#ifdef BAND5G
		pi->rssi_corr_perrg_5g[i] = (int8)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_rssicorrperrg5g, i, 0);
#endif /* BAND5G */
	}

	/* CCK Power index correction read from nvram */
	pi->sromi->cckPwrIdxCorr = (int16) PHY_GETINTVAR_DEFAULT(pi, rstr_cckPwrIdxCorr, 0);

	pi->min_txpower = PHY_TXPWR_MIN;
	pi->tx_pwr_backoff = (int8)PHY_GETINTVAR_DEFAULT(pi, rstr_txpwrbckof, PHY_TXPWRBCKOF_DEF);
	/* Some old boards doesn't have valid value programmed, use default */
	if (pi->tx_pwr_backoff == -1)
		pi->tx_pwr_backoff = PHY_TXPWRBCKOF_DEF;

	pi->phy_tempsense_offset = 0;
	/* Read default temp delta setting. */
	wlc_phy_read_tempdelta_settings(pi, NPHY_CAL_MAXTEMPDELTA);
	pi->core2slicemap = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_core2slicemap, 0);
	if (pi->core2slicemap != ((1 << DUAL_PHY_NUM_CORE_MAX) -1)) {
	  for (i = 0; i < DUAL_PHY_NUM_CORE_MAX; i++) {
	    if ((pi->core2slicemap >> i) & 1) {
	      pi->aux_slice_ant[k] = i;
	      k++;
	    } else {
	      pi->main_slice_ant[j] = i;
	      j++;
	    }
	  }
	}
}

static void
BCMATTACHFN(wlc_phy_std_params_attach)(phy_info_t *pi)
{
	/* set default rx iq est antenna/samples */
	pi->phy_rxiq_samps = PHY_NOISE_SAMPLE_LOG_NUM_NPHY;
	pi->phy_rxiq_antsel = ANT_RX_DIV_DEF;

	/* initialize SROM "isempty" flags for rxgainerror */
	pi->rxgainerr2g_isempty = FALSE;
	pi->rxgainerr5gl_isempty = FALSE;
	pi->rxgainerr5gm_isempty = FALSE;
	pi->rxgainerr5gh_isempty = FALSE;
	pi->rxgainerr5gu_isempty = FALSE;

	/* Do not enable the PHY watchdog for ATE */
#ifndef ATE_BUILD
	pi->phywatchdog_override = TRUE;
#endif

	/* Enable both cores by default */
	pi->sh->phyrxchain = 0x3;

#ifdef N2WOWL
	/* Reduce phyrxchain to 1 to save power in WOWL mode */
	if (CHIPID(pi->sh->chip) == BCM43237_CHIP_ID) {
		pi->sh->phyrxchain = 0x1;
	}
#endif /* N2WOWL */

#if defined(BCMINTERNAL) || defined(WLTEST)
	/* Initialize to invalid index values */
	pi->nphy_tbldump_minidx = -1;
	pi->nphy_tbldump_maxidx = -1;
	pi->nphy_phyreg_skipcnt = 0;
#endif

	/* This is the temperature at which the last PHYCAL was done.
	 * Initialize to a very low value.
	 */
	pi->def_cal_info->last_cal_temp = -50;
	pi->def_cal_info->cal_suppress_count = 0;

	/* default, PHY type overrides if interrupt based noise measurement isn't supported */
	pi->phynoise_polling = TRUE;

	/* still need to have this information hanging around, even for OPT version */
	pi->tx_power_offset = NULL;

	/* Assign the default cal info */
	pi->cal_info = pi->def_cal_info;
	pi->cal_info->cal_suppress_count = 0;
}
#ifdef WLC_TXCAL
static void
BCMATTACHFN(phy_txcal_detach)(phy_info_t *pi)
{
	txcal_root_pwr_tssi_t *pi_txcal_tssi_tbl;
	txcal_pwr_tssi_lut_t *pwr_tssi_lut_5G80, *pwr_tssi_lut_5G40, *pwr_tssi_lut_5G20;
	txcal_pwr_tssi_lut_t *pwr_tssi_lut_2G;

	pi_txcal_tssi_tbl = pi->txcali->txcal_root_pwr_tssi_tbl;
	if (pi_txcal_tssi_tbl != NULL) {
		pwr_tssi_lut_5G80 = pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G80;
		pwr_tssi_lut_5G40 = pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G40;
		pwr_tssi_lut_5G20 = pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G20;
		pwr_tssi_lut_2G = pi_txcal_tssi_tbl->root_pwr_tssi_lut_2G;
		if (pwr_tssi_lut_5G80 != NULL) {
			if (pwr_tssi_lut_5G80->txcal_pwr_tssi != NULL) {
				phy_mfree(pi, pwr_tssi_lut_5G80->txcal_pwr_tssi,
						sizeof(*pwr_tssi_lut_5G80->txcal_pwr_tssi));
				pwr_tssi_lut_5G80->txcal_pwr_tssi = NULL;
			}
			phy_mfree(pi, pwr_tssi_lut_5G80,
					sizeof(*pwr_tssi_lut_5G80));
			 pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G80 = NULL;
		}
		if (pwr_tssi_lut_5G40 != NULL) {
			if (pwr_tssi_lut_5G40->txcal_pwr_tssi != NULL) {
				phy_mfree(pi, pwr_tssi_lut_5G40->txcal_pwr_tssi,
						sizeof(*pwr_tssi_lut_5G40->txcal_pwr_tssi));
				pwr_tssi_lut_5G40->txcal_pwr_tssi = NULL;
			}
			phy_mfree(pi, pwr_tssi_lut_5G40,
					sizeof(*pwr_tssi_lut_5G40));
			 pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G40 = NULL;
		}
		if (pwr_tssi_lut_5G20 != NULL) {
			if (pwr_tssi_lut_5G20->txcal_pwr_tssi != NULL) {
				phy_mfree(pi, pwr_tssi_lut_5G20->txcal_pwr_tssi,
						sizeof(*pwr_tssi_lut_5G20->txcal_pwr_tssi));
				pwr_tssi_lut_5G20->txcal_pwr_tssi = NULL;
			}
			phy_mfree(pi, pwr_tssi_lut_5G20,
					sizeof(*pwr_tssi_lut_5G20));
			 pi_txcal_tssi_tbl->root_pwr_tssi_lut_5G20 = NULL;
		}
		if (pwr_tssi_lut_2G != NULL) {
			if (pwr_tssi_lut_2G->txcal_pwr_tssi != NULL) {
				phy_mfree(pi, pwr_tssi_lut_2G->txcal_pwr_tssi,
						sizeof(*pwr_tssi_lut_2G->txcal_pwr_tssi));
				pwr_tssi_lut_2G->txcal_pwr_tssi = NULL;
			}
			phy_mfree(pi, pwr_tssi_lut_2G, sizeof(*pwr_tssi_lut_2G));
			pi_txcal_tssi_tbl->root_pwr_tssi_lut_2G = NULL;
		}
		phy_mfree(pi, pi_txcal_tssi_tbl, sizeof(*pi_txcal_tssi_tbl));
		pi->txcali->txcal_root_pwr_tssi_tbl = NULL;
	}
	if (pi->txcali->txcal_meas != NULL) {
		phy_mfree(pi, pi->txcali->txcal_meas, sizeof(*pi->txcali->txcal_meas));
		pi->txcali->txcal_meas = NULL;
	}
	if (pi->txcali->txcal_pwr_tssi != NULL) {
		phy_mfree(pi, pi->txcali->txcal_pwr_tssi,
				sizeof(*pi->txcali->txcal_pwr_tssi));
		pi->txcali->txcal_pwr_tssi = NULL;
	}
	phy_mfree(pi, pi->txcali, sizeof(*pi->txcali));
	pi->txcali = NULL;
}
#endif /* WLC_TXCAL */

void
BCMATTACHFN(phy_module_detach)(phy_info_t *pi)
{

	PHY_TRACE(("wl: %s: pi = %p\n", __FUNCTION__, pi));

	if (pi == NULL)
		return;

	ASSERT(pi->refcnt > 0);

	if (--pi->refcnt)
		return;

	/* ======== Detach modules' PHY specific layer ======== */

	/* Unregister PHY type implementations from common - MUST BE THE FIRST! */
	if (pi->typei != NULL)
		phy_type_unregister_impl(pi->typei);

	/* ======== Detach modules' common layer ======== */

	/* ...Detach other modules... */

	/* Detach envelope tracking module */
	if (pi->eti != NULL)
		phy_et_detach(pi->eti);

	/* Detach HiRSSIeLNABypass module */
	if (pi->hirssii != NULL)
		phy_hirssi_detach(pi->hirssii);

#if defined(WL_NAP) && !defined(WL_NAP_DISABLED)
	/* Detach Nap module */
	if (pi->napi != NULL) {
		phy_nap_detach(pi->napi);
	}
#endif /* WL_NAP */

#ifdef WL_PROXDETECT
	/* Detach TOF module */
	if (pi->tofi != NULL)
		phy_tof_detach(pi->tofi);
#endif  /* WL_PROXDETECT */

	/* Detach dccal module */
	if (pi->dccali != NULL)
		phy_dccal_detach(pi->dccali);

#ifdef WL_MU_RX
	/* Detach MU-MIMO module */
	if (pi->mui != NULL)
		phy_mu_detach(pi->mui);
#endif /* WL_MU_RX */

#ifdef WL_DSI
	/* Detach DeepSleepInit module */
	if (pi->dsii != NULL)
		phy_dsi_detach(pi->dsii);
#endif /* WL_DSI */

#ifdef SAMPLE_COLLECT
	/* Detach SAMPle collect module */
	if (pi->sampi != NULL)
		phy_samp_detach(pi->sampi);
#endif /* SAMPLE_COLLECT */

#ifdef OCL
	/* Detach OCL module */
	if (pi->ocli != NULL)
		phy_ocl_detach(pi->ocli);
#endif /* OCL */

#ifdef WL11AX
	/* Detach HE module */
	if (ISHECAPPHY(pi)) {
		if ((pi->hecapi != NULL))
			phy_hecap_detach(pi->hecapi);
	}
#endif /* WL11AX */

	/* Detach RXSPUR module */
	if (pi->rxspuri != NULL)
		phy_rxspur_detach(pi->rxspuri);

	/* Detach RXGCRS module */
	if (pi->rxgcrsi != NULL)
		phy_rxgcrs_detach(pi->rxgcrsi);

	/* Detach TSSI Cal module */
	if (pi->tssicali != NULL)
		phy_tssical_detach(pi->tssicali);

	/* Detach misc module */
	if (pi->misci != NULL)
		phy_misc_detach(pi->misci);

	/* Detach LPC module */
	if (pi->lpci != NULL)
		phy_lpc_detach(pi->lpci);

	/* Detach FCBS module */
	if (pi->fcbsi != NULL)
		phy_fcbs_detach(pi->fcbsi);

	/* Detach CHannelManaGeR module */
	if (pi->chanmgri != NULL)
		phy_chanmgr_detach(pi->chanmgri);

	/* Detach VCO Cal module */
	if (pi->vcocali != NULL)
		phy_vcocal_detach(pi->vcocali);

	/* Detach PAPD Cal module */
	if (pi->papdcali != NULL)
		phy_papdcal_detach(pi->papdcali);

	/* Detach RXIQ Cal module */
	if (pi->rxiqcali != NULL)
		phy_rxiqcal_detach(pi->rxiqcali);

	/* Detach TXIQLO Cal module */
	if (pi->txiqlocali != NULL)
		phy_txiqlocal_detach(pi->txiqlocali);

	/* Detach BlueToothCoExistence module */
	if (pi->btcxi != NULL)
		phy_btcx_detach(pi->btcxi);

	/* Detach RSSICompute module */
	if (pi->rssii != NULL)
		phy_rssi_detach(pi->rssii);

	/* Detach TEMPerature sense module */
	if (pi->tempi != NULL)
		phy_temp_detach(pi->tempi);

#ifndef WLC_DISABLE_ACI
	/* Detach INTerFerence module */
	if (pi->noisei != NULL)
		phy_noise_detach(pi->noisei);
#endif

	/* Detach ANTennaDIVersity module */
	if (pi->antdivi != NULL)
		phy_antdiv_detach(pi->antdivi);

#if defined(AP) && defined(RADAR)
	/* Detach RadarDetect module */
	if (pi->radari != NULL)
		phy_radar_detach(pi->radari);
#endif

#if defined(WLC_TXPWRCAP) && !defined(WLC_TXPWRCAP_DISABLED)
	/* Detach TxPowerCtrl module */
	if (pi->txpwrcapi != NULL)
		phy_txpwrcap_detach(pi->txpwrcapi);
#endif

	/* Detach TxPowerCtrl module */
	if (pi->tpci != NULL)
		phy_tpc_detach(pi->tpci);

	/* Detach PHYTableInit module */
	if (pi->tbli != NULL)
		phy_tbl_detach(pi->tbli);

	/* Detach RADIO control module */
	if (pi->radioi != NULL)
		phy_radio_detach(pi->radioi);

	/* Detach ANAcore control module */
	if (pi->anai != NULL)
		phy_ana_detach(pi->anai);

	/* ======== Detach PHY specific layer ======== */

	/* Detach PHY type implementation layer from common layer */
	if (pi->typei != NULL &&
	    *(phy_type_info_t **)(uintptr)&pi->u != NULL)
		phy_type_detach(pi->typei, *(phy_type_info_t **)(uintptr)&pi->u);

	/* ======== Detach infrastructure services ======== */

	/* Detach CALibrationManaGeR module */
	if (pi->calmgri != NULL)
		phy_calmgr_detach(pi->calmgri);

	/* Detach watchdog module */
	if (pi->wdi != NULL)
		phy_wd_detach(pi->wdi);

	/* Detach CACHE module */
	if (pi->cachei != NULL)
		phy_cache_detach(pi->cachei);
#ifdef NEW_PHY_CAL_ARCH
	/* Detach CHannelManaGeR Notification module */
	if (pi->chanmgr_notifi != NULL)
		phy_chanmgr_notif_detach(pi->chanmgr_notifi);
#endif
	/* Detach INIT control module */
	if (pi->initi != NULL)
		phy_init_detach(pi->initi);

	/* Detach PHY type implementation dispatch info */
	if (pi->typei != NULL)
		phy_type_disp_detach(pi->typei);

	/* Detach PHY Common info */
	if (pi->cmni != NULL)
		phy_cmn_detach(pi->cmni);

	/* Detach dump registry - MUST BE THE LAST */
	if (pi->dbgi != NULL)
		phy_dbg_detach(pi->dbgi);


/* *********************************************** */

#if defined(PHYCAL_CACHING)
	pi->phy_calcache_on = FALSE;
	wlc_phy_cal_cache_deinit((wlc_phy_t *)pi);
#endif

	/* Quick-n-dirty remove from list */
	if (pi->sh->phy_head == pi)
		pi->sh->phy_head = pi->next;
	else if (pi->sh->phy_head->next == pi)
		pi->sh->phy_head->next = NULL;
	else
		ASSERT(0);

#ifdef WLTXPWR_CACHE
	if (pi->tx_power_offset != NULL)
		wlc_phy_clear_tx_power_offset((wlc_phy_t *)pi);
#if defined(WLTXPWR_CACHE_PHY_ONLY)
	if (pi->txpwr_cache != NULL)
		wlc_phy_txpwr_cache_close(pi->sh->osh, pi->txpwr_cache);
#endif
#else
	if (pi->tx_power_offset != NULL)
		ppr_delete(pi->sh->osh, pi->tx_power_offset);
#endif	/* WLTXPWR_CACHE */

	if (pi->ff != NULL) {
		phy_mfree(pi, pi->ff, sizeof(phy_feature_flags_t));
		pi->ff = NULL;
	}

	if (pi->pdpi != NULL) {
		phy_mfree(pi, pi->pdpi, sizeof(*pi->pdpi));
		pi->pdpi = NULL;
	}

	if (pi->olpci != NULL) {
		phy_mfree(pi, pi->olpci, sizeof(*pi->olpci));
		pi->olpci = NULL;
	}
#ifdef WLC_TXCAL
	if (pi->txcali != NULL)
		phy_txcal_detach(pi);
#endif /* WLC_TXCAL */

	if (pi->pwrdet_ac != NULL) {
	    phy_mfree(pi, pi->pwrdet_ac, (SROMREV(pi->sh->sromrev) >= 12 ?
	       sizeof(srom12_pwrdet_t) : sizeof(srom11_pwrdet_t)));
	    pi->pwrdet_ac = NULL;
	}

	if (pi->ppr != NULL) {
		phy_mfree(pi, pi->ppr, sizeof(ppr_info_t));
		pi->ppr = NULL;
	}

	if (pi->pi_fptr != NULL) {
		phy_mfree(pi, pi->pi_fptr, sizeof(phy_func_ptr_t));
		pi->pi_fptr = NULL;
	}
#ifdef WLSRVSDB
	if (pi->srvsdb_state != NULL) {
		phy_mfree(pi, pi->srvsdb_state, sizeof(srvsdb_info_t));
		pi->srvsdb_state = NULL;
	}
#endif /* WLSRVSDB */
	if (pi->fem5g != NULL) {
		phy_mfree(pi, pi->fem5g, sizeof(srom_fem_t));
		pi->fem5g = NULL;
	}
	if (pi->fem2g != NULL) {
		phy_mfree(pi, pi->fem2g, sizeof(srom_fem_t));
		pi->fem2g = NULL;
	}
#ifdef ENABLE_FCBS
	if (pi->phy_fcbs != NULL) {
	    phy_mfree(pi, pi->phy_fcbs, sizeof(fcbs_info));
	    pi->phy_fcbs = NULL;
	}
#endif /* ENABLE_FCBS */
	if (pi->def_cal_info != NULL) {
	    phy_mfree(pi, pi->def_cal_info, sizeof(phy_cal_info_t));
	    pi->def_cal_info = NULL;
	}
	if (pi->txcore_temp != NULL) {
	    phy_mfree(pi, pi->txcore_temp, sizeof(phy_txcore_temp_t));
	    pi->txcore_temp = NULL;
	}
	if (pi->pwrdet != NULL) {
	    phy_mfree(pi, pi->pwrdet, sizeof(srom_pwrdet_t));
	    pi->pwrdet = NULL;
	}
	if (pi->interf != NULL) {
	    phy_mfree(pi, pi->interf, sizeof(interference_info_t));
	    pi->interf = NULL;
	}

	if (pi->pubpi_ro != NULL) {
	    phy_mfree(pi, pi->pubpi_ro, sizeof(wlc_phy_t));
	    pi->pubpi_ro = NULL;
	}

	if (pi->pubpi != NULL) {
	    phy_mfree(pi, pi->pubpi, sizeof(wlc_phy_t));
	    pi->pubpi = NULL;
	}

	MFREE(pi->sh->osh, pi, sizeof(phy_info_t));
}

/* Register all iovar tables to/from system */
int
BCMATTACHFN(phy_register_iovt_all)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	int err;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Register all common layer's iovar tables/handlers */
	if ((err = phy_register_iovt(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register PHY type implementation layer's iovar tables/handlers */
	if ((err = phy_type_register_iovt(pi->typei, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_type_register_iovt failed\n", __FUNCTION__));
		goto fail;
	}

	return BCME_OK;

fail:
	return err;
}

/* Register all ioctl tables to/from system */
int
BCMATTACHFN(phy_register_ioct_all)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	int err;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* Register all common layer's ioctl tables/handlers */
	if ((err = phy_register_ioct(pi, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_register_ioct failed\n", __FUNCTION__));
		goto fail;
	}

	/* Register PHY type implementation layer's ioctl tables/handlers */
	if ((err = phy_type_register_ioct(pi->typei, ii)) != BCME_OK) {
		PHY_ERROR(("%s: phy_type_register_ioct failed\n", __FUNCTION__));
		goto fail;
	}

	return BCME_OK;

fail:
	return err;
}

/* band specific init */
void
WLBANDINITFN(phy_bsinit)(phy_info_t *pi, chanspec_t chanspec, bool forced)
{
	/* if chanswitch path, skip phy_init for D11REV > 40 */
	if (!ISACPHY(pi) || forced)
		phy_init(pi, chanspec);
}

/* band width init */
void
WLBANDINITFN(phy_bwinit)(phy_info_t *pi, chanspec_t chanspec)
{
	if (!ISACPHY(pi))
		phy_init(pi, chanspec);
}

/* Init/deinit the PHY h/w. */
void
WLBANDINITFN(phy_init)(phy_info_t *pi, chanspec_t chanspec)
{
	uint32	mc;
#if defined(BCMDBG) || defined(PHYDBG)
	char chbuf[CHANSPEC_STR_LEN];
#endif

	ASSERT(pi != NULL);

	pi->phy_crash_rc = PHY_RC_NONE;

#if defined(BCMDBG) || defined(PHYDBG)
	PHY_TRACE(("wl%d: %s chanspec %s\n", pi->sh->unit, __FUNCTION__,
		wf_chspec_ntoa(chanspec, chbuf)));
#endif

	/* skip if this function is called recursively(e.g. when bw is changed) */
	if (pi->init_in_progress)
		return;

	pi->last_radio_chanspec = pi->radio_chanspec;

	pi->init_in_progress = TRUE;
	wlc_phy_chanspec_radio_set((wlc_phy_t *)pi, chanspec);
	pi->phynoise_state = 0;

	/* Update ucode channel value */
	wlc_phy_chanspec_shm_set(pi, chanspec);

	mc = R_REG(pi->sh->osh, &pi->regs->maccontrol);
	if ((mc & MCTL_EN_MAC) != 0) {
		if (mc == 0xffffffff)
			PHY_ERROR(("wl%d: %s: chip is dead !!!\n", pi->sh->unit, __FUNCTION__));
		else
			PHY_ERROR(("wl%d: %s: MAC running! mc=0x%x\n",
			          pi->sh->unit, __FUNCTION__, mc));
		ASSERT((const char*)"wlc_phy_init: Called with the MAC running!" == NULL);
	}

	/* clear during init. To be set by higher level wlc code */
	pi->cur_interference_mode = INTERFERE_NONE;

	/* init PUB_NOT_ASSOC */
	if (!(pi->measure_hold & PHY_HOLD_FOR_SCAN) && pi->interf->aci.nphy != NULL &&
	    !(pi->interf->aci.nphy->detection_in_progress)) {
#ifdef WLSRVSDB
		if (!pi->srvsdb_state->srvsdb_active)
			pi->measure_hold |= PHY_HOLD_FOR_NOT_ASSOC;
#else
		pi->measure_hold |= PHY_HOLD_FOR_NOT_ASSOC;
#endif
	}

	/* check D11 is running on Fast Clock */
	ASSERT(si_core_sflags(pi->sh->sih, 0, 0) & SISF_FCLKA);

#ifndef WLC_DISABLE_ACI
	/* update interference mode before INIT process start */
	if (((ISNPHY(pi) && NREV_GE(pi->pubpi->phy_rev, 3)) || ISHTPHY(pi) ||
		ISACPHY(pi)) && !(SCAN_INPROG_PHY(pi)))
		pi->sh->interference_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
			pi->sh->interference_mode_2G : pi->sh->interference_mode_5G;
#endif

#ifdef WL_DSI
	if (phy_get_dsi_trigger_st(pi)) {
		/* DeepSleepInit */
		phy_dsi_restore(pi);
	} else
#endif /* WL_DSI */
	{
		/* ######## Init process start ######## */

		/* ======== Common inits ======== */

		/* Init each feature/module including s/w and h/w */
		if (phy_init_invoke_init_fns(pi->initi) != BCME_OK) {
			PHY_ERROR(("wl%d: %s: phy_init_invoke_init_fns failed."
				"phy_type %d, rev %d\n", pi->sh->unit, __FUNCTION__,
				pi->pubpi->phy_type, pi->pubpi->phy_rev));
			return;
		}

		/* ======== Special inits ======== */

		/* ^^^Add other special init calls here^^^ */


		/* ######## Init process end ######## */
	}

	/* Indicate a power on reset isn't needed for future phy init's */
	pi->phy_init_por = FALSE;

	pi->init_in_progress = FALSE;

	/* clear flag */
	phy_init_done(pi);

	pi->bt_shm_addr = 2 * wlapi_bmac_read_shm(pi->sh->physhim, M_BTCX_BLK_PTR(pi));

#if (!defined(WL_SISOCHIP) && defined(SWCTRL_TO_BT_IN_COEX))
	/* Ensure that WLAN/BT coex FEMctrl shmems are populated upon
	 * the first WLAN init
	 */
	wlc_phy_femctrl_mask_on_band_change(pi);
#endif
	if (CHIPID(pi->sh->chip) == BCM43012_CHIP_ID) {
		if (ISACPHY(pi)) {
			uint16 curtemp = 0;
			curtemp = wlc_phy_tempsense_acphy(pi);
			wlapi_openloop_cal(pi->sh->physhim, curtemp);
		}
	}
}

/* driver up/init processing */
static int
WLBANDINITFN(_phy_init)(phy_init_ctx_t *ctx)
{
	phy_info_t *pi = (phy_info_t *)ctx;

	return phy_type_init_impl(pi->typei);
}

int
BCMUNINITFN(phy_down)(phy_info_t *pi)
{
	int callbacks = 0;

	PHY_TRACE(("%s\n", __FUNCTION__));

#ifndef BCMNODOWN
	/* all activate phytest should have been stopped */
	ASSERT(pi->phytest_on == FALSE);

	/* ^^^Add other special down calls here^^^ */


	/* ======== Common down ======== */

	phy_init_invoke_down_fns(pi->initi);
#endif /* !BCMNODOWN */

	return callbacks;
}

#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && (defined(BCMINTERNAL) || \
	defined(DBG_PHY_IOV))) || defined(BCMDBG_PHYDUMP)
static int
_phy_dump_phyregs(void *ctx, struct bcmstrbuf *b)
{
	phy_info_t *pi = ctx;

	if (!pi->sh->clk)
		return BCME_NOCLK;

	return phy_type_dump_phyregs(pi->typei, b);

}

#define PAGE_SZ 512
#define NUM_PAGES 10

typedef struct {
	phy_regs_t *rl_start;
	phy_regs_t *rl_end;
} dp_info_t;

dp_info_t dpi[NUM_PAGES];

static void
phyreg_page_parser(phy_info_t *pi, phy_regs_t *reglist, struct bcmstrbuf *b)
{
	int i = 0, nr = 0;
	bool rl_end_set = FALSE, dbg_prints = FALSE;

	phy_regs_t *rl_end = NULL, *rl_last = NULL;

	/* initialize base */
	dpi[0].rl_start = reglist;

	for (i = 0; i < ARRAYSIZE(dpi); i++) {
		nr = 0; rl_end_set = FALSE;
		reglist = dpi[i].rl_start;

		while (reglist->num > 0) {
			if (nr > PAGE_SZ) {
				rl_end = reglist;
				rl_end_set = TRUE;
				break;
			}
			nr += reglist->num;
			rl_last = ++reglist;
		}

		if (!rl_end_set)
			rl_end = rl_last;

		dpi[i].rl_end = rl_end;

		if (i < (ARRAYSIZE(dpi) - 1))
			dpi[i + 1].rl_start = dpi[i].rl_end;

		if (dbg_prints) {
			bcm_bprintf(b, "%d: sb %04x eb %04x\n", i,
					dpi[i].rl_start->base,
					dpi[i].rl_end->base);
		}
	}

	for (i = 0; i < ARRAYSIZE(dpi); i++) {
		if (dpi[i].rl_start == dpi[i].rl_end) {
			pi->pdpi->phyregs_pmax = --i;
			break;
		}
	}

	if (dbg_prints)
		bcm_bprintf(b, "Num of Pages = %d\n", pi->pdpi->phyregs_pmax);
}

/* dump phyregs listed in 'reglist' */
void
phy_dump_phyregs(phy_info_t *pi, const char *str,
	phy_regs_t *rl, uint16 off, struct bcmstrbuf *b)
{
	uint16 addr, val = 0, num;
	phy_regs_t *reglist = rl, *rl_end = NULL;

#if defined(BCMINTERNAL) || defined(WLTEST)
	uint16 i = 0;
	bool skip;
#endif
	if (reglist == NULL)
		return;

	/* setup dump pages */
	phyreg_page_parser(pi, reglist, b);

	if (pi->pdpi->page > pi->pdpi->phyregs_pmax) {
		bcm_bprintf(b, "\n(%d > %d) | Exceeding # of available pages \n\n",
				pi->pdpi->page, pi->pdpi->phyregs_pmax);
		return;
	}

	reglist = dpi[pi->pdpi->page].rl_start;
	rl_end = dpi[pi->pdpi->page].rl_end;

	if (reglist == rl_end) {
		bcm_bprintf(b, "\nMax # of pages exceeded\n\n", str);
		return;
	}

	bcm_bprintf(b, "----- %06s -----\n", str);
	bcm_bprintf(b, "Add Value\n");

	while (((num = reglist->num) > 0) && (reglist != rl_end)) {
#if defined(BCMINTERNAL) || defined(WLTEST)
		skip = FALSE;

		for (i = 0; i < pi->nphy_phyreg_skipcnt; i++) {
			if (pi->nphy_phyreg_skipaddr[i] == reglist->base) {
				skip = TRUE;
				break;
			}
		}

		if (skip) {
			reglist++;
			continue;
		}
#endif /* defined(BCMINTERNAL) || defined(WLTEST) */

		for (addr = reglist->base + off; num && b->size > 0; addr++, num--) {
			val = phy_type_read_phyreg(pi->typei, addr);

			if (PHY_INFORM_ON() && si_taclear(pi->sh->sih, FALSE)) {
				PHY_INFORM(("%s: TA reading phy reg %s:0x%x\n",
				           __FUNCTION__, str, addr));
				bcm_bprintf(b, "%03x tabort\n", addr);
			} else
				bcm_bprintf(b, "%03x %04x\n", addr, val);
		}
		reglist++;
	}
}
#endif /* ((BCMDBG || BCMDBG_DUMP) && (BCMINTERNAL || DBG_PHY_IOV)) || BCMDBG_PHYDUMP */

static void
BCMATTACHFN(phy_register_dumps)(phy_info_t *pi)
{
#if ((defined(BCMDBG) || defined(BCMDBG_DUMP)) && defined(BCMINTERNAL)) || \
	defined(BCMDBG_PHYDUMP)
	phy_dbg_add_dump_fn(pi, "phyreg", _phy_dump_phyregs, pi);
#endif /* ((BCMDBG || BCMDBG_DUMP) && BCMINTERNAL) || BCMDBG_PHYDUMP */
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	phy_dbg_add_dump_fn(pi, "phypapd", wlc_phydump_papd, pi);
	phy_dbg_add_dump_fn(pi, "phystate", wlc_phydump_state, pi);
	phy_dbg_add_dump_fn(pi, "phylnagain", wlc_phydump_lnagain, pi);
	phy_dbg_add_dump_fn(pi, "phyinitgain", wlc_phydump_initgain, pi);
	phy_dbg_add_dump_fn(pi, "phyhpf1tbl", wlc_phydump_hpf1tbl, pi);
	phy_dbg_add_dump_fn(pi, "phychanest", wlc_phydump_chanest, pi);
	phy_dbg_add_dump_fn(pi, "phytxv0", wlc_phydump_txv0, pi);
#if defined(DBG_BCN_LOSS)
	phy_dbg_add_dump_fn(pi, "phycalrxmin", wlc_phydump_phycal_rx_min, pi);
#endif
#endif /* BCMDBG || BCMDBG_DUMP */

#if defined(WLTEST)
	phy_dbg_add_dump_fn(pi, "phych4rpcal", wlc_phydump_ch4rpcal, pi);
#endif /* WLTEST */

	/*  default page */
	pi->pdpi->page = 0;
}

void *
phy_malloc_fatal(phy_info_t *pi, uint sz)
{
	void* ptr;
	if ((ptr = phy_malloc(pi, sz)) == NULL) {
		PHY_FATAL_ERROR_MESG(("Memory allocation failed in function %p,"
			"malloced %d bytes\n", CALL_SITE, MALLOCED(pi->sh->osh)));
		PHY_FATAL_ERROR(pi, PHY_RC_NOMEM);
	}
	return ptr;
}

/* PHYMODE switch requires a clean phy initialization,
 * set a flag to indicate phyinit is pending
 */
bool
phy_init_pending(phy_info_t *pi)
{
	ASSERT(pi != NULL);
	return pi->phyinit_pending;
}

/* clear flag upon phyinit */
static void
phy_init_done(phy_info_t *pi)
{
	ASSERT(pi != NULL);
	pi->phyinit_pending = FALSE;
}

mbool
phy_get_measure_hold_status(phy_info_t *pi)
{
	return pi->measure_hold;
}

void
phy_set_measure_hold_status(phy_info_t *pi, mbool set)
{
	pi->measure_hold = set;
}
