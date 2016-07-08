/*
 * MU-MIMO receive module for Broadcom 802.11 Networking Adapter Device Drivers
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
 * $Id: wlc_murx.h 539036 2015-03-05 16:34:43Z $
 */

#ifndef _wlc_murx_h_
#define _wlc_murx_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_mumimo.h>

wlc_murx_info_t *(wlc_murx_attach)(wlc_info_t *wlc);
void (wlc_murx_detach)(wlc_murx_info_t *mu_info);
void wlc_murx_filter_bfe_cap(wlc_murx_info_t *mu_info, wlc_bsscfg_t *bsscfg, uint32 *cap);
int wlc_murx_gid_update(wlc_info_t *wlc, struct scb *scb,
                        uint8 *membership_status, uint8 *user_position);

#endif /* _wlc_murx_h_ */
