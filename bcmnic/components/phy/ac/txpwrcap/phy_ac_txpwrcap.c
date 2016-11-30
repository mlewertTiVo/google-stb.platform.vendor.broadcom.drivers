
/*
 * ACPHY TxPowerCap module implementation
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
 * $Id: phy_ac_txpwrcap.c 644299 2016-06-18 03:37:28Z gpasrija $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_wd.h>
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <phy_type_txpwrcap.h>
#include <phy_txpwrcap_api.h>
#include <phy_ac_txpwrcap.h>
#include <phy_txpwrcap.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_ac.h>
#ifdef WLC_SW_DIVERSITY
#include "phy_ac_antdiv.h"
#endif

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_hal.h>
/* TODO: all these are going away... > */
#endif

#include <bcmdevs.h>

#define ACPHY_TXPOWERCAP_PARAMS	(3)
#define ACPHY_TXPOWERCAP_MAX_QDB	(127)
#define ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE	(2)

/* module private states */
struct phy_ac_txpwrcap_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_txpwrcap_info_t *ti;
	/* std params */
	/* add other variable size variables here at the end */
	/* Tx Power cap vars */
	bool txpwrcap_cellstatus;
	wl_txpwrcap_tbl_t *txpwrcap_tbl;
};

#if defined(WLC_TXPWRCAP)

/* local functions */
static void wlc_phy_txpwrcap_cellstatusupd_acphy(phy_info_t *pi);
static bool wlc_phy_txpwrcap_attach_acphy(phy_ac_txpwrcap_info_t *ti);
static int wlc_phy_txpwrcap_init_acphy(phy_type_txpwrcap_ctx_t *ctx);
static bool wlc_phy_txpwrcap_get_cellstatus_acphy(phy_type_txpwrcap_ctx_t *ctx);
static void wlc_phy_txpwrcap_set_cellstatus_acphy(phy_type_txpwrcap_ctx_t *ctx,
	int mask, int value);
static int wlc_phy_txpwrcap_tbl_get_acphy(phy_type_txpwrcap_ctx_t *ctx,
	wl_txpwrcap_tbl_t *txpwrcap_tbl);
static int wlc_phy_txpwrcap_tbl_set_acphy(phy_type_txpwrcap_ctx_t *ctx,
	wl_txpwrcap_tbl_t *txpwrcap_tbl);
#ifdef WLC_SW_DIVERSITY
static void wlc_phy_txpwrcap_to_shm_acphy(phy_type_txpwrcap_ctx_t *ctx,
	uint16 tx_ant, uint16 rx_ant);
#endif
static void wlc_phy_txpwrcap_set_acphy_sdb(phy_info_t *pi, int8* txcap,
	uint16 tx_ant, uint16 rx_ant);
static uint32 wlc_phy_get_txpwrcap_inuse_acphy(phy_type_txpwrcap_ctx_t *ctx);

/* External functions */


/* Register/unregister ACPHY specific implementation to common layer. */
phy_ac_txpwrcap_info_t *
BCMATTACHFN(phy_ac_txpwrcap_register_impl)(phy_info_t *pi,
	phy_ac_info_t *aci, phy_txpwrcap_info_t *ti)
{
	phy_ac_txpwrcap_info_t *info;
	phy_type_txpwrcap_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_ac_txpwrcap_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	info->pi = pi;
	info->aci = aci;
	info->ti = ti;

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = wlc_phy_txpwrcap_init_acphy;
	fns.txpwrcap_tbl_set = wlc_phy_txpwrcap_tbl_set_acphy;
	fns.txpwrcap_tbl_get = wlc_phy_txpwrcap_tbl_get_acphy;
	fns.txpwrcap_get_cellstatus = wlc_phy_txpwrcap_get_cellstatus_acphy;
	fns.txpwrcap_set_cellstatus = wlc_phy_txpwrcap_set_cellstatus_acphy;
#ifdef WLC_SW_DIVERSITY
	fns.txpwrcap_to_shm = wlc_phy_txpwrcap_to_shm_acphy;
#endif
	fns.txpwrcap_in_use = wlc_phy_get_txpwrcap_inuse_acphy;
	fns.ctx = info;

	if (!wlc_phy_txpwrcap_attach_acphy(info))
		goto fail;

	phy_txpwrcap_register_impl(ti, &fns);

	return info;
fail:
	if (info != NULL)
		phy_mfree(pi, info, sizeof(phy_ac_txpwrcap_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_txpwrcap_unregister_impl)(phy_ac_txpwrcap_info_t *info)
{
	phy_info_t *pi;
	phy_txpwrcap_info_t *ti;

	ASSERT(info);
	pi = info->pi;
	ti = info->ti;

	PHY_TRACE(("%s\n", __FUNCTION__));

	phy_txpwrcap_unregister_impl(ti);

	if (info->txpwrcap_tbl != NULL) {
		phy_mfree(pi, info->txpwrcap_tbl, sizeof(wl_txpwrcap_tbl_t));
	}

	phy_mfree(pi, info, sizeof(phy_ac_txpwrcap_info_t));
}


int8
wlc_phy_txpwrcap_tbl_get_max_percore_acphy(phy_info_t *pi, uint8 core)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	phy_ac_txpwrcap_info_t *ti = pi_ac->txpwrcapi;

	uint8 k;
	int8 maxpwr = 127;
	for (k = 0; k < ti->txpwrcap_tbl->num_antennas_per_core[core]; k++) {
		/* Get the min of txpwrcap for all cores/cell on/off */
		if (ti->txpwrcap_tbl->pwrcap_cell_off[2*core+k] != -128) {
			maxpwr = MIN(maxpwr, ti->txpwrcap_tbl->pwrcap_cell_off[2*core+k]);
		}
		if (ti->txpwrcap_tbl->pwrcap_cell_on[2*core+k] != -128) {
			maxpwr = MIN(maxpwr, ti->txpwrcap_tbl->pwrcap_cell_on[2*core+k]);
		}
	}
	return maxpwr;
}

static bool
wlc_phy_txpwrcap_get_cellstatus_acphy(phy_type_txpwrcap_ctx_t *ctx)
{
	phy_ac_txpwrcap_info_t *info = (phy_ac_txpwrcap_info_t *)ctx;
	return (info->txpwrcap_cellstatus & TXPWRCAP_CELLSTATUS_MASK);
}

static void
wlc_phy_txpwrcap_set_cellstatus_acphy(phy_type_txpwrcap_ctx_t *ctx, int mask, int value)
{
	phy_ac_txpwrcap_info_t *info = (phy_ac_txpwrcap_info_t *)ctx;
	phy_info_t *pi = info->pi;
	info->txpwrcap_cellstatus &= ~(mask);
	value &= mask;
	info->txpwrcap_cellstatus |= value;

	wlc_phy_txpwrcap_cellstatusupd_acphy(pi);
}

static void
wlc_phy_txpwrcap_cellstatusupd_acphy(phy_info_t *pi)
{
	int txpwrcap_upreqd = FALSE;
	int cellstatus_new, cellstatus_cur;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_ac_txpwrcap_info_t *ti = pi_ac->txpwrcapi;

	/* CELLSTATUS_FORCE_UPD => Update forced, either by iovar or at init time
	 * CELLSTATUS_FORCE => value forced by iovar, ignore value from WCI2
	 * else compare value from WCI2 (cellstatus_new) and current value (cellstatus_cur)
	 * to determine if update needed
	 */
	if (ti->txpwrcap_cellstatus & TXPWRCAP_CELLSTATUS_FORCE_UPD_MASK) {
		ti->txpwrcap_cellstatus &= ~(TXPWRCAP_CELLSTATUS_FORCE_UPD_MASK);
		txpwrcap_upreqd = TRUE;
	}
	else if (!(ti->txpwrcap_cellstatus & TXPWRCAP_CELLSTATUS_FORCE_MASK)) {
		cellstatus_cur = (ti->txpwrcap_cellstatus & TXPWRCAP_CELLSTATUS_MASK)
			>> TXPWRCAP_CELLSTATUS_NBIT;
		cellstatus_new = (ti->txpwrcap_cellstatus & TXPWRCAP_CELLSTATUS_WCI2_MASK)
			>> TXPWRCAP_CELLSTATUS_WCI2_NBIT;
		/* Note if status change, as need to update PHY */
		if (cellstatus_cur != cellstatus_new) {
			ti->txpwrcap_cellstatus &= ~(1 << TXPWRCAP_CELLSTATUS_NBIT);
			ti->txpwrcap_cellstatus |= (cellstatus_new << TXPWRCAP_CELLSTATUS_NBIT);
			txpwrcap_upreqd = TRUE;
		}
	}

	if (txpwrcap_upreqd && pi->sh->clk) {
		/* PHY update required */
#ifdef WLC_SW_DIVERSITY
		if (PHYSWDIV_ENAB(pi)) {
			wlapi_swdiv_cellstatus_notif(pi->sh->physhim,
				(ti->txpwrcap_cellstatus & TXPWRCAP_CELLSTATUS_MASK));
		}
#endif
		wlc_phy_txpwrcap_set_acphy(pi);
	}
}

static bool
BCMATTACHFN(wlc_phy_txpwrcap_attach_acphy)(phy_ac_txpwrcap_info_t *ti)
{
	phy_info_t *pi = ti->pi;
	uint8 i;

	pi->_txpwrcap = TRUE;

	if ((ti->txpwrcap_tbl =
		phy_malloc(pi, sizeof(wl_txpwrcap_tbl_t))) == NULL) {
		PHY_ERROR(("%s: txpwrcap_tbl malloc failed\n", __FUNCTION__));
		return FALSE;
	}

	/* By default TxPwerCap state has CellOn state */
	ti->txpwrcap_cellstatus = TXPWRCAP_CELLSTATUS_ON;
	for (i = 0; i < TXPWRCAP_MAX_NUM_CORES; i++) {
		ti->txpwrcap_tbl->num_antennas_per_core[i] = 2;
	}
	for (i = 0; i < TXPWRCAP_MAX_NUM_ANTENNAS; i++) {
		ti->txpwrcap_tbl->pwrcap_cell_off[i] = ACPHY_TXPOWERCAP_MAX_QDB;
		ti->txpwrcap_tbl->pwrcap_cell_on[i] = ACPHY_TXPOWERCAP_MAX_QDB;
	}

	return TRUE;
}

static int
wlc_phy_txpwrcap_init_acphy(phy_type_txpwrcap_ctx_t *ctx)
{
	phy_ac_txpwrcap_info_t *ti = (phy_ac_txpwrcap_info_t *)ctx;
	phy_info_t *pi = ti->pi;
	uint16 bt_shm_addr;
	/* Write failsafe caps in shms */
	bt_shm_addr = 2 * wlapi_bmac_read_shm(pi->sh->physhim, M_BTCX_BLK_PTR(pi));
	/* Choose pwrcap value for Ant 0 on core 0 for failsafe */
	wlapi_bmac_write_shm(pi->sh->physhim,
		bt_shm_addr + M_LTECX_PWRCP_C0_FS_OFFSET(pi),
		ti->txpwrcap_tbl->pwrcap_cell_on[0] - pi->tx_pwr_backoff);
	if (PHYCORENUM(pi->pubpi->phy_corenum) > 1)
		wlapi_bmac_write_shm(pi->sh->physhim,
			bt_shm_addr + M_LTECX_PWRCP_C1_FS_OFFSET(pi),
			ti->txpwrcap_tbl->pwrcap_cell_on[2] - pi->tx_pwr_backoff);
	return BCME_OK;
}

static int
wlc_phy_txpwrcap_check_in_limits_acphy(phy_info_t *pi,
	int8 txpwrcaptbl[WLC_TXCORE_MAX], uint8 core)
{
	int8 txpwrcap[WLC_TXCORE_MAX] = { ACPHY_TXPOWERCAP_MAX_QDB, ACPHY_TXPOWERCAP_MAX_QDB,
			ACPHY_TXPOWERCAP_MAX_QDB, ACPHY_TXPOWERCAP_MAX_QDB};
	uint8 loop_cnt = 0, i = 0;
	bool swdiv_supported = FALSE;
	BCM_REFERENCE(txpwrcap);
	BCM_REFERENCE(swdiv_supported);
	BCM_REFERENCE(loop_cnt);
	BCM_REFERENCE(i);
#ifdef WLC_SW_DIVERSITY
	if (PHYSWDIV_ENAB(pi)) {
		swdiv_supported = phy_ac_swdiv_is_supported(pi->u.pi_acphy->antdivi, core, TRUE);
	}
#endif /* WLC_SW_DIVERSITY */
	txpwrcap[loop_cnt++] = txpwrcaptbl[(core * ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE)];
	if (swdiv_supported) {
		txpwrcap[loop_cnt++] = txpwrcaptbl[(core * ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE) + 1];
	}
#ifdef WLC_TXCAL
	/* Check if tx powercap is lower than the limit with OLPC on */
	if (!pi->olpci->disable_olpc) {
		/* TXCAL Data based OLPC */
		if (pi->olpci->olpc_idx_valid && pi->olpci->olpc_idx_in_use) {
			for (i = 0; i < loop_cnt; ++i) {
				if (txpwrcap[i] < ACPHY_MIN_POWER_SUPPORTED_WITH_OLPC_QDBM) {
					PHY_FATAL_ERROR_MESG(("core%d, txcap = %d\n",
						core, txpwrcap[i]));
					PHY_FATAL_ERROR(pi, PHY_RC_TXPOWER_LIMITS);
					return BCME_ERROR;
				}
			}
		}
	}
#endif /* WLC_TXCAL */
	return BCME_OK;
}

/* This function is called in SDB mode to program TxCap values independently for each SDB core */
static void
wlc_phy_txpwrcap_set_acphy_sdb(phy_info_t *pi, int8* txcap,
	uint16 tx_ant, uint16 rx_ant)
{
	uint8 core; uint8 chain;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	phy_ac_txpwrcap_info_t *ti = pi_ac->txpwrcapi;
	uint16 cap0, cap1;
	int8 txpwrcap[WLC_TXCORE_MAX] = {ACPHY_TXPOWERCAP_MAX_QDB,
		ACPHY_TXPOWERCAP_MAX_QDB,
		ACPHY_TXPOWERCAP_MAX_QDB,
		ACPHY_TXPOWERCAP_MAX_QDB};
	uint16 bt_shm_addr;
#ifdef WLC_SW_DIVERSITY
	bool swdiv_supported = FALSE;
	uint8 ant = 0;
#endif /* WLC_SW_DIVERSITY */

	core = phy_get_current_core(pi);

	FOREACH_ACTV_CORE(pi, pi->sh->phytxchain, chain) {
#ifdef WLC_SW_DIVERSITY
	swdiv_supported = phy_ac_swdiv_is_supported(pi->u.pi_acphy->antdivi,
		ACPHY_PHYCORE_ANY, TRUE);
#endif /* WLC_SW_DIVERSITY */
	if (!ti->txpwrcap_tbl->num_antennas_per_core[core * 2 + chain]) {
		PHY_ERROR(("%s: Tx Power Caps not provided for core %d chain %d antennas\n",
			__FUNCTION__, core, chain));
	} else if (ti->txpwrcap_tbl->num_antennas_per_core[core * 2 + chain] == 1)
		txpwrcap[chain] = txcap[(core * 2 + chain) * ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE];
#ifdef WLC_SW_DIVERSITY
	else {
		if (swdiv_supported) {
			/* Based on swOvr, choose either 0 or 1 antennae for core */
			ant = phy_ac_swdiv_get_rxant_bycoreidx(pi->u.pi_acphy->antdivi, core);
			txpwrcap[core] = txcap[(core * ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE) +
					(ant ? 1 : 0)];
		} else {
			/* Currently support just one antenna for all cores other than 0 */
			txpwrcap[core] = txcap[core * ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE];
		}
	}
#endif /* WLC_SW_DIVERSITY */
	}

	cap0 = (uint16)(txpwrcap[0] - pi->tx_pwr_backoff);
	cap1 = (uint16)(txpwrcap[1] - pi->tx_pwr_backoff);
	bt_shm_addr = 2 * wlapi_bmac_read_shm(pi->sh->physhim, M_BTCX_BLK_PTR(pi));
	wlapi_bmac_write_shm(pi->sh->physhim, bt_shm_addr + M_LTECX_PWRCP_C0_OFFSET(pi), cap0);
	wlapi_bmac_write_shm(pi->sh->physhim, bt_shm_addr + M_LTECX_PWRCP_C1_OFFSET(pi), cap1);

#ifdef WLC_SW_DIVERSITY
	if (swdiv_supported) {
		phy_ac_swdiv_txpwrcap_shmem_set(pi->u.pi_acphy->antdivi, core, cap0, cap0,
			tx_ant, rx_ant);
	}
#endif /* WLC_SW_DIVERSITY */
}

#ifdef WLC_SW_DIVERSITY
static void
wlc_phy_txpwrcap_to_shm_acphy(phy_type_txpwrcap_ctx_t *ctx, uint16 tx_ant, uint16 rx_ant)
{
	phy_ac_txpwrcap_info_t *ti = (phy_ac_txpwrcap_info_t *)ctx;
	phy_info_t *pi = ti->pi;
	int8 *txcap = NULL;
	uint8 core;
	uint8 cap_tx, cap_rx;
	uint16 cap_core0, cap_core1;
	uint16 bt_shm_addr, phy_mode;

	if (wlc_phy_txpwrcap_get_cellstatus_acphy(ti))
		txcap = ti->txpwrcap_tbl->pwrcap_cell_on;
	else
		txcap = ti->txpwrcap_tbl->pwrcap_cell_off;

	phy_mode = phy_get_phymode(pi);
	if (phy_mode == PHYMODE_MIMO) {
		/* need to extend when uCode support multi-core */
		core = 0;
		/* Use cap corresponding to RX Ant for M_LTECX_PWRCP_C0_OFFSET */
		cap_core0 = (uint16)(txcap[rx_ant & (1 << core) ? 1 : 0]
			- pi->tx_pwr_backoff);
		cap_core1 = (uint16)(txcap[2] - pi->tx_pwr_backoff);

		bt_shm_addr = 2 * wlapi_bmac_read_shm(pi->sh->physhim, M_BTCX_BLK_PTR(pi));
		wlapi_bmac_write_shm(pi->sh->physhim,
			bt_shm_addr + M_LTECX_PWRCP_C0_OFFSET(pi), cap_core0);
		if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
			wlapi_bmac_write_shm(pi->sh->physhim, bt_shm_addr +
					M_LTECX_PWRCP_C1_OFFSET(pi), cap_core1);
		}
		cap_tx = (uint8) (txcap[(tx_ant & (1 << core)) ? 1 : 0]
			- pi->tx_pwr_backoff);
		cap_rx = (uint8) (txcap[(rx_ant & (1 << core)) ? 1 : 0]
			- pi->tx_pwr_backoff);
		/* multi core support will be limited by ucode impl.
		 * currently core0 only
		 */
		phy_ac_swdiv_txpwrcap_shmem_set(pi->u.pi_acphy->antdivi,
			core, cap_tx, cap_rx, tx_ant, rx_ant);
	} else if (phy_mode == PHYMODE_RSDB) {
		wlc_phy_txpwrcap_set_acphy_sdb(pi, txcap, tx_ant, rx_ant);
	}
}
#endif /* WLC_SW_DIVERSITY */

void
wlc_phy_txpwrcap_set_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	phy_ac_txpwrcap_info_t *ac_ti = pi_ac->txpwrcapi;

	uint core;
	int8 txpwrcap[WLC_TXCORE_MAX] = {ACPHY_TXPOWERCAP_MAX_QDB,
		ACPHY_TXPOWERCAP_MAX_QDB,
		ACPHY_TXPOWERCAP_MAX_QDB,
		ACPHY_TXPOWERCAP_MAX_QDB};
	int8 *txcap = NULL;
	uint16 cap_core0, cap_core1;
	uint16 bt_shm_addr, phy_mode;
	uint16 tx_ant = 0, rx_ant = 0;
#ifdef WLC_SW_DIVERSITY
	uint8 ant = 0;
	tx_ant = ANTDIV_ANTMAP_NOCHANGE;
	rx_ant = ANTDIV_ANTMAP_NOCHANGE;
#endif

	if (!PHYTXPWRCAP_ENAB(pi))
		return;

	ASSERT(pi->sh->clk);
	phy_mode = phy_get_phymode(pi);

	if (wlc_phy_txpwrcap_get_cellstatus_acphy(ac_ti))
		txcap = ac_ti->txpwrcap_tbl->pwrcap_cell_on;
	else
		txcap = ac_ti->txpwrcap_tbl->pwrcap_cell_off;

	if (phy_mode == PHYMODE_MIMO) {
		FOREACH_ACTV_CORE(pi, pi->sh->phytxchain, core) {
			if (!ac_ti->txpwrcap_tbl->num_antennas_per_core[core]) {
				PHY_ERROR(("%s: Tx Power Caps not provided for core %d antennas\n",
					__FUNCTION__, core));
			  break;
			}
			else if	(ac_ti->txpwrcap_tbl->num_antennas_per_core[core] == 1)
				txpwrcap[core] = txcap[core*2];
#ifdef WLC_SW_DIVERSITY
			else {
				if (PHYSWDIV_ENAB(pi)) {
					if (phy_ac_swdiv_is_supported(pi->u.pi_acphy->antdivi,
							core, TRUE)) {
						/* Based on swOvr, choose either 0 or 1 antennae
						 * for core 0
						 */
						ant = phy_ac_swdiv_get_rxant_bycoreidx(
							pi->u.pi_acphy->antdivi, core);
						txpwrcap[core] = txcap[(core *
							ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE) +
							ant ? 1 : 0];
					} else {
						/* Currently support just one antenna for all
						 * cores other than 0
						 */
						txpwrcap[core] = txcap[core *
							ACPHY_TXPOWERCAP_MAX_ANT_PER_CORE];
					}
				}
			}
#endif /* WLC_SW_DIVERSITY */
		}
		cap_core0 = (uint16)(txpwrcap[0] - pi->tx_pwr_backoff);
		cap_core1 = (uint16)(txpwrcap[1] - pi->tx_pwr_backoff);
		bt_shm_addr = 2 * wlapi_bmac_read_shm(pi->sh->physhim, M_BTCX_BLK_PTR(pi));
		wlapi_bmac_write_shm(pi->sh->physhim, bt_shm_addr + M_LTECX_PWRCP_C0_OFFSET(pi),
			cap_core0);
		if (PHYCORENUM(pi->pubpi->phy_corenum) > 1)
			wlapi_bmac_write_shm(pi->sh->physhim, bt_shm_addr +
					M_LTECX_PWRCP_C1_OFFSET(pi), cap_core1);

#ifdef WLC_SW_DIVERSITY
		/* Add another shared memory here if sw div is available on other cores */
		if (PHYSWDIV_ENAB(pi)) {
			phy_ac_swdiv_txpwrcap_shmem_set(pi->u.pi_acphy->antdivi,
				0, cap_core0, cap_core0, tx_ant, rx_ant);
		}
#endif
	} else if (phy_mode == PHYMODE_RSDB) {
		wlc_phy_txpwrcap_set_acphy_sdb(pi, txcap, tx_ant, rx_ant);
	}
}

uint32
wlc_phy_get_txpwrcap_inuse_acphy(phy_type_txpwrcap_ctx_t *ctx)
{
	phy_ac_txpwrcap_info_t *info = (phy_ac_txpwrcap_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 cap_in_use = 0;

	ASSERT(pi->sh->clk);

	IF_ACTV_CORE(pi, pi->sh->phytxchain, 0)
		cap_in_use = READ_PHYREGFLD(pi, TxPwrCapping_path0, maxTxPwrCap_path0);

	IF_ACTV_CORE(pi, pi->sh->phytxchain, 1)
		cap_in_use = cap_in_use | (READ_PHYREGFLD(pi, TxPwrCapping_path1,
			maxTxPwrCap_path1) << 8);

	IF_ACTV_CORE(pi, pi->sh->phytxchain, 2)
		cap_in_use = cap_in_use | (READ_PHYREGFLD(pi, TxPwrCapping_path2,
			maxTxPwrCap_path2) << 16);

	return cap_in_use;
}

static int
wlc_phy_txpwrcap_tbl_get_acphy(phy_type_txpwrcap_ctx_t *ctx, wl_txpwrcap_tbl_t *txpwrcap_tbl)
{
	phy_ac_txpwrcap_info_t *ti = (phy_ac_txpwrcap_info_t *)ctx;
	uint8 i, j = 0, k;

	for (i = 0; i < TXPWRCAP_MAX_NUM_CORES; i++) {
		txpwrcap_tbl->num_antennas_per_core[i] =
			ti->txpwrcap_tbl->num_antennas_per_core[i];
		for (k = 0; k < ti->txpwrcap_tbl->num_antennas_per_core[i]; k++) {
			txpwrcap_tbl->pwrcap_cell_off[j] =
				ti->txpwrcap_tbl->pwrcap_cell_off[2*i+k];
			txpwrcap_tbl->pwrcap_cell_on[j] =
				ti->txpwrcap_tbl->pwrcap_cell_on[2*i+k];
			j++;
		}
	}

	return BCME_OK;
}

static int
wlc_phy_txpwrcap_tbl_set_acphy(phy_type_txpwrcap_ctx_t *ctx, wl_txpwrcap_tbl_t *txpwrcap_tbl)
{
	phy_ac_txpwrcap_info_t *ti = (phy_ac_txpwrcap_info_t *)ctx;
	phy_info_t *pi = ti->pi;
	uint8 i, j = 0, k;
	int err = 0;
	uint8 num_antennas = 0, max_antennas = 0, min_antennas = 0;
	uint core;
	uint16 bt_shm_addr, phy_mode;
	uint8 default_ant_core0 = 0;	/* no swdiv default */
	uint16 failsafe_pwrcap_ant;

	/* Some Error Checking */
	phy_mode = phy_get_phymode(pi);
	if (phy_mode == PHYMODE_MIMO) {
		for (i = 0; i < TXPWRCAP_MAX_NUM_CORES; i++) {
			num_antennas += txpwrcap_tbl->num_antennas_per_core[i];
		}
		FOREACH_CORE(pi, core) {
			max_antennas += 2;
			min_antennas++;
		}
	} else if (phy_mode == PHYMODE_RSDB) { /* SDB mode */
		num_antennas += txpwrcap_tbl->num_antennas_per_core[phy_get_current_core(pi)];
		max_antennas += 2;
		min_antennas++;
	} else
		return err;

	if ((num_antennas > max_antennas) || (num_antennas < min_antennas))
		return BCME_ERROR;

	for (i = 0; i < TXPWRCAP_MAX_NUM_CORES; i++) {
		ti->txpwrcap_tbl->num_antennas_per_core[i] =
			txpwrcap_tbl->num_antennas_per_core[i];
		for (k = 0; k < ti->txpwrcap_tbl->num_antennas_per_core[i]; k++) {
			ti->txpwrcap_tbl->pwrcap_cell_off[2*i+k] =
				txpwrcap_tbl->pwrcap_cell_off[j];
			ti->txpwrcap_tbl->pwrcap_cell_on[2*i+k] =
				txpwrcap_tbl->pwrcap_cell_on[j];
			j++;
		}
	}

	if (phy_mode == PHYMODE_MIMO) {
		FOREACH_CORE(pi, core) {
			err = wlc_phy_txpwrcap_check_in_limits_acphy(pi,
					ti->txpwrcap_tbl->pwrcap_cell_on, core);
			if (err)
				break;
			err = wlc_phy_txpwrcap_check_in_limits_acphy(pi,
					ti->txpwrcap_tbl->pwrcap_cell_off, core);
			if (err)
				break;
		}
	} else if (phy_mode == PHYMODE_RSDB) {
		core = phy_get_current_core(pi);
		err = wlc_phy_txpwrcap_check_in_limits_acphy(pi,
				ti->txpwrcap_tbl->pwrcap_cell_on, core);
		if (err == BCME_OK) {
			err = wlc_phy_txpwrcap_check_in_limits_acphy(pi,
					ti->txpwrcap_tbl->pwrcap_cell_off, core);
		}
	}

	/* Apply the new caps */
	if (pi->sh->clk && (err == BCME_OK)) {
		wlc_phy_txpwrcap_set_acphy(pi);

		/* Write failsafe caps in shms */
		bt_shm_addr = 2 * wlapi_bmac_read_shm(pi->sh->physhim, M_BTCX_BLK_PTR(pi));
#ifdef WLC_SW_DIVERSITY
		if (PHYSWDIV_ENAB(pi)) {
			default_ant_core0 = wlapi_swdiv_get_default_ant(pi->sh->physhim, 0);
		}
#endif
		failsafe_pwrcap_ant =
			((ti->txpwrcap_tbl->pwrcap_cell_on[default_ant_core0]
			- pi->tx_pwr_backoff) << 8) | default_ant_core0;
		wlapi_bmac_write_shm(pi->sh->physhim,
				bt_shm_addr + M_LTECX_PWRCP_C0_FSANT_OFFSET(pi),
				failsafe_pwrcap_ant);
		wlapi_bmac_write_shm(pi->sh->physhim,
				bt_shm_addr + M_LTECX_PWRCP_C0_FS_OFFSET(pi),
				ti->txpwrcap_tbl->pwrcap_cell_on[default_ant_core0] -
				pi->tx_pwr_backoff);
		if (PHYCORENUM(pi->pubpi->phy_corenum) > 1)
			wlapi_bmac_write_shm(pi->sh->physhim,
				bt_shm_addr + M_LTECX_PWRCP_C1_FS_OFFSET(pi),
				ti->txpwrcap_tbl->pwrcap_cell_on[2] - pi->tx_pwr_backoff);
	}
	return err;
}
#endif /* WLC_TXPWRCAP */
