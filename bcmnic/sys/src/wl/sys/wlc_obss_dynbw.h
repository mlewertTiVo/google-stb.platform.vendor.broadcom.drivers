/*
 * OBSS Dynamic bandwidth switch support
 * Broadcom 802.11 Networking Device Driver
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
 * $Id: wlc_obss_dynbw.h 596126 2015-10-29 19:53:48Z $
 */


#ifndef _WLC_OBSS_DYNBW_H_
#define _WLC_OBSS_DYNBW_H_

wlc_obss_dynbw_t * wlc_obss_dynbw_attach(wlc_info_t *wlc);

void wlc_obss_dynbw_detach(wlc_obss_dynbw_t *dynbw_pub);

chanspec_t wlc_obss_dynbw_ht_chanspec_override(wlc_obss_dynbw_t *dynbw_pub,
	wlc_bsscfg_t *bsscfg, chanspec_t beacon_chanspec);
void wlc_obss_dynbw_tx_bw_override(wlc_obss_dynbw_t *dynbw_pub,
	wlc_bsscfg_t *bsscfg, uint32 *rspec_bw);

void wlc_obss_dynbw_beacon_chanspec_override(wlc_obss_dynbw_t *dynbw_pub,
	wlc_bsscfg_t *bsscfg, chanspec_t *chanspec);

#endif /* _WLC_OBSS_DYNBW_H_ */
