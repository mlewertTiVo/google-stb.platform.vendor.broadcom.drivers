/*
 * VASIP related declarations
 * Broadcom 802.11abg Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_vasip.h 525449 2015-01-09 18:13:18Z $
 */

#ifndef _WLC_VASIP_H_
#define _WLC_VASIP_H_

#ifdef VASIP_HW_SUPPORT
/* initialize vasip */
void wlc_vasip_init(wlc_hw_info_t *wlc_hw);

/* attach/detach */
extern int wlc_vasip_attach(wlc_info_t *wlc);
extern void wlc_vasip_detach(wlc_info_t *wlc);
#else
#define wlc_vasip_init(wlc_hw) do {} while (0)
#endif /* VASIP_HW_SUPPORT */

#endif /* _WLC_VASIP_H_ */
