/*
 * TxPowerCtrl module public interface (to MAC driver).
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
 * $Id: phy_tpc_api.h 651513 2016-07-27 08:59:58Z mvermeid $
 */

#ifndef _phy_tpc_api_h_
#define _phy_tpc_api_h_

#include <typedefs.h>
#include <phy_api.h>

/*
 * Compute power control cap.
 */
#ifdef WLOLPC
int8 wlc_phy_calc_ppr_pwr_cap(wlc_phy_t *ppi, uint8 core);
void wlc_phy_update_olpc_cal(wlc_phy_t *ppi, bool set, bool dbg);
#endif
void wlc_phy_txpower_limit_set(wlc_phy_t *ppi, ppr_t* txpwr, chanspec_t chanspec);
int wlc_phy_txpower_set(wlc_phy_t *ppi, int8 qdbm, bool override, ppr_t *reg_pwr);
void wlc_phy_txpower_recalc_target(phy_info_t *pi, ppr_t *txpwr_reg, ppr_t *txpwr_targets);
int wlc_phy_txpower_get_current(wlc_phy_t *ppi, ppr_t *reg_pwr, phy_tx_power_t *power);
void wlc_phy_txpower_sromlimit(wlc_phy_t *ppi, chanspec_t chanspec, uint8 *min_pwr,
	ppr_t *max_pwr, uint8 core);
#ifdef WL_SAR_SIMPLE_CONTROL
void wlc_phy_dynamic_sarctrl_set(wlc_phy_t *ppi, bool isctrlon);
#endif /* WL_SAR_SIMPLE_CONTROL */
#ifdef WL_SARLIMIT
void wlc_phy_sar_limit_set(wlc_phy_t *ppi, uint32 int_val);
#endif /* WL_SARLIMIT */
bool wlc_phy_txpower_hw_ctrl_get(wlc_phy_t *ppi);

int wlc_phy_txpower_core_offset_set(wlc_phy_t *ppi,
	struct phy_txcore_pwr_offsets *offsets);
int wlc_phy_txpower_core_offset_get(wlc_phy_t *ppi,
	struct phy_txcore_pwr_offsets *offsets);
void wlc_phy_txpwr_percent_set(wlc_phy_t *ppi, uint8 txpwr_percent);

int wlc_phy_txpower_get(wlc_phy_t *ppi, int8 *qdbm, bool *override);
int	wlc_phy_neg_txpower_set(wlc_phy_t *ppi, uint qdbm);
extern void wlc_phy_set_country(wlc_phy_t *ppi, const char* ccode_ptr);
extern int8 wlc_phy_maxtxpwr_lowlimit(wlc_phy_t *ppi);
#endif /* _phy_tpc_api_h_ */
