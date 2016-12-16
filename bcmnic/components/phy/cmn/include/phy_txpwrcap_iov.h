/*
 * TxPowerCap module internal interface - iovar table/handlers
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
 * $Id: phy_txpwrcap_iov.h 629423 2016-04-05 11:12:48Z $
 */

#ifndef _phy_txpwrcap_iov_h_
#define _phy_txpwrcap_iov_h_

/* register iovar table/handlers */
int phy_txpwrcap_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_txpwrcap_iov_h_ */
