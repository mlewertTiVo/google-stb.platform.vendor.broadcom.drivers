/*
 * TxPowerCap module internal interface (to other PHY modules).
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
 * $Id: phy_txpwrcap.h 652728 2016-08-03 07:25:22Z $
 */

#ifndef _phy_txpwrcap_h_
#define _phy_txpwrcap_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_txpwrcap_info phy_txpwrcap_info_t;

#ifdef WLC_TXPWRCAP

#define TXPWRCAP_CELLSTATUS_ON 1
#define TXPWRCAP_CELLSTATUS_OFF 0
#define TXPWRCAP_CELLSTATUS_NBIT 0
#define TXPWRCAP_CELLSTATUS_MASK (1<<TXPWRCAP_CELLSTATUS_NBIT)
#define TXPWRCAP_CELLSTATUS_FORCE_MASK 0x2
#define TXPWRCAP_CELLSTATUS_FORCE_UPD_MASK 0x4
#define TXPWRCAP_CELLSTATUS_WCI2_NBIT 4
#define TXPWRCAP_CELLSTATUS_WCI2_MASK (1<<TXPWRCAP_CELLSTATUS_WCI2_NBIT)

#define TXPOWERCAP_MAX_QDB	(127)
#define TXPOWERCAP_MAX_ANT_PER_CORE	(2)


/* attach/detach */
phy_txpwrcap_info_t *phy_txpwrcap_attach(phy_info_t *pi);
void phy_txpwrcap_detach(phy_txpwrcap_info_t *ri);

/* ******** interface for Txpowercap module ******** */
uint32 phy_txpwrcap_get_caps_inuse(phy_info_t *pi);
int8 phy_txpwrcap_tbl_get_max_percore(phy_info_t *pi, uint8 core);

#ifdef WLC_SW_DIVERSITY
void wlc_phy_txpwrcap_to_shm(wlc_phy_t *pih, uint16 tx_ant, uint16 rx_ant);
#endif /* WLC_SW_DIVERSITY */
#endif /* WLC_TXPWRCAP */

#endif /* _phy_txpwrcap_h_ */
