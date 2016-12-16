/*
 * Rx Gain Control and Carrier Sense module internal interface - iovar table/handlers
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
 * $Id: phy_rxgcrs_iov.h 644994 2016-06-22 06:23:44Z $
 */

#ifndef _phy_rxgcrs_iov_t_
#define _phy_rxgcrs_iov_t_

/* register iovar table/handlers */
int phy_rxgcrs_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_rxgcrs_iov_t_ */
