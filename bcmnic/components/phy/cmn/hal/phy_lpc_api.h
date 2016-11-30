/*
 * LPC module public interface (to MAC driver).
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
 * $Id: phy_lpc_api.h$
 */

#ifndef _phy_lpc_api_h_
#define _phy_lpc_api_h_

/* Link Power Control Module */
/* Remove the following #defs once branches have been updated */
#define wlc_phy_pwrsel_getminidx        wlc_phy_lpc_getminidx
#define wlc_phy_pwrsel_getoffset        wlc_phy_lpc_getoffset
#define wlc_phy_pwrsel_get_txcpwrval    wlc_phy_lpc_get_txcpwrval
#define wlc_phy_pwrsel_set_txcpwrval    wlc_phy_lpc_set_txcpwrval
#define wlc_phy_powersel_algo_set       wlc_phy_lpc_algo_set

uint8 wlc_phy_lpc_getminidx(wlc_phy_t *ppi);
uint8 wlc_phy_lpc_getoffset(wlc_phy_t *ppi, uint8 index);
uint8 wlc_phy_lpc_get_txcpwrval(wlc_phy_t *ppi, uint16 phytxctrlword);
void wlc_phy_lpc_set_txcpwrval(wlc_phy_t *ppi, uint16 *phytxctrlword, uint8 txcpwrval);
void wlc_phy_lpc_algo_set(wlc_phy_t *ppi, bool enable);
#ifdef WL_LPC_DEBUG
uint8 * wlc_phy_lpc_get_pwrlevelptr(wlc_phy_t *ppi);
#endif

#endif /* _phy_lpc_api_h_ */
