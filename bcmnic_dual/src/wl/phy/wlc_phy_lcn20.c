/*
 * PHY and RADIO specific portion of Broadcom BCM43XX 802.11abgn
 * Networking Device Driver.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_phy_lcn20.c $*
 */


#include <wlc_cfg.h>

#ifdef LCN20CONF
#if LCN20CONF != 0
#include <typedefs.h>
#include <qmath.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <wlc_phy_radio.h>
#include <bitfuncs.h>
#include <bcmdevs.h>
#include <bcmnvram.h>
#include <proto/802.11.h>
#include <hndpmu.h>
#include <bcmsrom_fmt.h>
#include <sbsprom.h>
#include <wlc_phy_hal.h>
#include <wlc_phy_int.h>
#include <wlc_phy_lcn20.h>
#include <sbchipc.h>
/* #include <wlc_phyreg_lcn20.h> */
/* #include <wlc_phytbl_lcn20.h> */
#include <bcmotp.h>
#include <wlc_phy_shim.h>

static void wlc_phy_init_lcn20phy(phy_info_t *pi);
static void wlc_phy_cal_init_lcn20phy(phy_info_t *pi);
static void wlc_phy_detach_lcn20phy(phy_info_t *pi);
static void wlc_phy_chanspec_set_lcn20phy(phy_info_t *pi, chanspec_t chanspec);
static void wlc_phy_txpower_recalc_target_lcn20phy(phy_info_t *pi);

static void wlc_lcn20phy_set_tx_pwr_ctrl(phy_info_t *pi, uint16 mode);
static void wlc_lcn20phy_set_tx_pwr_by_index(phy_info_t *pi, int indx);

static void wlc_phy_set_tx_iqcc_lcn20phy(phy_info_t *pi, uint16 a, uint16 b);
static void wlc_phy_get_tx_iqcc_lcn20phy(phy_info_t *pi, uint16 *a, uint16 *b);
static uint16 wlc_phy_get_tx_locc_lcn20phy(phy_info_t *pi);
static void wlc_phy_set_tx_locc_lcn20phy(phy_info_t *pi, uint16 didq);
static void wlc_phy_get_radio_loft_lcn20phy(phy_info_t *pi, uint8 *ei0,
	uint8 *eq0, uint8 *fi0, uint8 *fq0);


static void wlc_lcn20phy_anacore(phy_info_t *pi, bool on);
static void wlc_lcn20phy_switch_radio(phy_info_t *pi, bool on);
static void wlc_phy_watchdog_lcn20phy(phy_info_t *pi);
static void wlc_lcn20phy_btc_adjust(phy_info_t *pi, bool btactive);
void wlc_lcn20phy_write_table(phy_info_t *pi, const phytbl_info_t *pti);
void wlc_lcn20phy_read_table(phy_info_t *pi, phytbl_info_t *pti);
void wlc_lcn20phy_calib_modes(phy_info_t *pi, uint mode);
static bool wlc_lcn20phy_txpwr_srom_read(phy_info_t *pi);

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/*  function implementation   					*/
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* ATTACH */
bool
BCMATTACHFN(wlc_phy_attach_lcn20phy)(phy_info_t *pi)
{
	/* phy_info_lcn20phy_t *pi_lcn20; */

	pi->u.pi_lcn20phy = (phy_info_lcn20phy_t*)MALLOC(pi->sh->osh, sizeof(phy_info_lcn20phy_t));
	if (pi->u.pi_lcn20phy == NULL) {
	PHY_ERROR(("wl%d: %s: MALLOC failure\n", pi->sh->unit, __FUNCTION__));
		return FALSE;
	}
	bzero((char *)pi->u.pi_lcn20phy, sizeof(phy_info_lcn20phy_t));

	/* pi_lcn20 = pi->u.pi_lcn20phy; */

#if defined(PHYCAL_CACHING)
	/* Reset the var as no cal cache context should exist yet */
	pi->calcache_num = 0;
#endif

	/* Get xtal frequency from PMU */
	pi->xtalfreq = si_alp_clock(pi->sh->sih);
	ASSERT((pi->xtalfreq % 1000) == 0);

	PHY_INFORM(("wl%d: %s: using %d.%d MHz xtalfreq for RF PLL\n",
		pi->sh->unit, __FUNCTION__,
		pi->xtalfreq / 1000000, pi->xtalfreq % 1000000));

	pi->pi_fptr.init = wlc_phy_init_lcn20phy;
	pi->pi_fptr.calinit = wlc_phy_cal_init_lcn20phy;
	pi->pi_fptr.chanset = wlc_phy_chanspec_set_lcn20phy;
	pi->pi_fptr.txpwrrecalc = wlc_phy_txpower_recalc_target_lcn20phy;
	pi->pi_fptr.txiqccget = wlc_phy_get_tx_iqcc_lcn20phy;
	pi->pi_fptr.txiqccset = wlc_phy_set_tx_iqcc_lcn20phy;
	pi->pi_fptr.txloccget = wlc_phy_get_tx_locc_lcn20phy;
	pi->pi_fptr.txloccset = wlc_phy_set_tx_locc_lcn20phy;
	pi->pi_fptr.radioloftget = wlc_phy_get_radio_loft_lcn20phy;
	pi->pi_fptr.phywatchdog = wlc_phy_watchdog_lcn20phy;
	pi->pi_fptr.phybtcadjust = wlc_lcn20phy_btc_adjust;
	pi->pi_fptr.anacore = wlc_lcn20phy_anacore;
	pi->pi_fptr.switchradio = wlc_lcn20phy_switch_radio;
	pi->pi_fptr.phywritetable = wlc_lcn20phy_write_table;
	pi->pi_fptr.phyreadtable = wlc_lcn20phy_read_table;
	pi->pi_fptr.calibmodes = wlc_lcn20phy_calib_modes;
	pi->pi_fptr.detach = wlc_phy_detach_lcn20phy;

	if (!wlc_lcn20phy_txpwr_srom_read(pi))
		return FALSE;

	return TRUE;
}

static void
BCMATTACHFN(wlc_phy_detach_lcn20phy)(phy_info_t *pi)
{
	MFREE(pi->sh->osh, pi->u.pi_lcn20phy, sizeof(phy_info_lcn20phy_t));
}

static void
WLBANDINITFN(wlc_phy_cal_init_lcn20phy)(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_phy_chanspec_set_lcn20phy(phy_info_t *pi, chanspec_t chanspec)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_phy_txpower_recalc_target_lcn20phy(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}


static void
wlc_phy_get_tx_iqcc_lcn20phy(phy_info_t *pi, uint16 *a, uint16 *b)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}
static void
wlc_phy_set_tx_iqcc_lcn20phy(phy_info_t *pi, uint16 a, uint16 b)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static uint16
wlc_phy_get_tx_locc_lcn20phy(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	return 0;
}

static void
wlc_phy_set_tx_locc_lcn20phy(phy_info_t *pi, uint16 didq)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_phy_get_radio_loft_lcn20phy(phy_info_t *pi,
	uint8 *ei0,
	uint8 *eq0,
	uint8 *fi0,
	uint8 *fq0)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_phy_watchdog_lcn20phy(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_lcn20phy_btc_adjust(phy_info_t *pi, bool btactive)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}


static void
wlc_lcn20phy_anacore(phy_info_t *pi, bool on)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_lcn20phy_switch_radio(phy_info_t *pi, bool on)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

void
wlc_lcn20phy_write_table(phy_info_t *pi, const phytbl_info_t *pti)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

void
wlc_lcn20phy_read_table(phy_info_t *pi, phytbl_info_t *pti)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

void
wlc_lcn20phy_calib_modes(phy_info_t *pi, uint mode)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

/* Read band specific data from the SROM */
static bool
BCMATTACHFN(wlc_lcn20phy_txpwr_srom_read)(phy_info_t *pi)
{
	return TRUE;
}

static void
wlc_lcn20phy_radio_reset(phy_info_t *pi)
{
	if (NORADIO_ENAB(pi->pubpi))
		return;
}

static void
WLBANDINITFN(wlc_lcn20phy_tbl_init)(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
WLBANDINITFN(wlc_lcn20phy_reg_init)(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_lcn20phy_set_tx_pwr_ctrl(phy_info_t *pi, uint16 mode)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_lcn20phy_force_pwr_index(phy_info_t *pi, int indx)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

static void
wlc_lcn20phy_set_tx_pwr_by_index(phy_info_t *pi, int indx)
{
	/* Turn off automatic power control */
	wlc_lcn20phy_set_tx_pwr_ctrl(pi, LCN20PHY_TX_PWR_CTRL_OFF);

	/* Force tx power from the index */
	wlc_lcn20phy_force_pwr_index(pi, indx);
}

static void
wlc_lcn20phy_aci_init(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	PHY_INFORM(("aci init interference_mode %d\n", pi->sh->interference_mode));
}

static void
WLBANDINITFN(wlc_lcn20phy_baseband_init)(phy_info_t *pi)
{
	PHY_TRACE(("%s:\n", __FUNCTION__));
	/* Initialize LCN20PHY tables */
	wlc_lcn20phy_tbl_init(pi);
	wlc_lcn20phy_reg_init(pi);
	wlc_lcn20phy_set_tx_pwr_by_index(pi, 40);
	wlc_lcn20phy_aci_init(pi);
}

static void
WLBANDINITFN(wlc_lcn20phy_radio_init)(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	if (NORADIO_ENAB(pi->pubpi))
		return;
}

static void
WLBANDINITFN(wlc_lcn20phy_tx_pwr_ctrl_init)(phy_info_t *pi)
{
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
}

/* INIT */
static void
WLBANDINITFN(wlc_phy_init_lcn20phy)(phy_info_t *pi)
{
	/* phy_info_lcn20phy_t *pi_lcn20 = pi->u.pi_lcn20phy; */

	PHY_TRACE(("%s:\n", __FUNCTION__));

#ifdef UNRELEASEDCHIP
	if (CHIPID(pi->sh->chip) == BCM43430_CHIP_ID) {
		if (NORADIO_ENAB(pi->pubpi))
			return;
	}
#endif /* UNRELEASEDCHIP */
	/* reset the radio */
	wlc_lcn20phy_radio_reset(pi);

	/* Initialize baseband : bu_tweaks is a placeholder */
	wlc_lcn20phy_baseband_init(pi);

	/* Initialize radio */
	wlc_lcn20phy_radio_init(pi);

	/* Initialize power control */
	wlc_lcn20phy_tx_pwr_ctrl_init(pi);

	/* Tune to the current channel */
	wlc_phy_chanspec_set_lcn20phy(pi, pi->radio_chanspec);
}

#endif /* LCN20CONF != 0 */
#endif /* ifdef LCN20CONF */
