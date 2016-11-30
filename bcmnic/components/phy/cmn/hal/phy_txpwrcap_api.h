/*
 * TxPowerCap module public interface (to MAC driver).
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
 * $Id: phy_txpwrcap_api.h 630712 2016-04-11 19:05:53Z kentryu $
 */

#ifndef _phy_txpwrcap_api_h_
#define _phy_txpwrcap_api_h_

#if defined(WLC_TXPWRCAP)

#include <typedefs.h>
#include <phy_api.h>

int wlc_phy_cellstatus_override_set(phy_info_t *pi, int value);
int wlc_phyhal_txpwrcap_get_cellstatus(wlc_phy_t *pih, int32* cellstatus);
void wlc_phyhal_txpwrcap_set_cellstatus(wlc_phy_t *pih, int32 cellstatus);
int wlc_phy_txpwrcap_tbl_set(wlc_phy_t *pih, wl_txpwrcap_tbl_t *txpwrcap_tbl);
int wlc_phy_txpwrcap_tbl_get(wlc_phy_t *pih, wl_txpwrcap_tbl_t *txpwrcap_tbl);
bool wlc_phy_txpwrcap_get_enabflg(wlc_phy_t *pih);
#ifdef WLC_SW_DIVERSITY
uint8 wlc_phy_get_current_core(wlc_phy_t *pih);
void wlc_phy_txpwrcap_to_shm(wlc_phy_t *pih, uint16 tx_ant, uint16 rx_ant);
#endif /* WLC_SW_DIVERSITY */

#endif /* WLC_TXPWRCAP */
#endif /* _phy_txpwrcap_api_h_ */
