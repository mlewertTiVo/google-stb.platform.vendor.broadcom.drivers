/**
 * AKM (Authentication and Key Management) IE management module interface, 802.1x related.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_akm_ie.h 593626 2015-10-18 23:18:10Z $
 */

#ifndef _wlc_akm_ie_h_
#define _wlc_akm_ie_h_

extern wlc_akm_info_t *wlc_akm_attach(wlc_info_t *wlc);
extern void wlc_akm_detach(wlc_akm_info_t *akm);

extern uint8 wlc_wpa_mcast_cipher(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

#endif /* _wlc_akm_ie_h_ */
