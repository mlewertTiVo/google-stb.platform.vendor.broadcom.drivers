/*
 * AP Channel/Chanspec Selection interface.
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
 * $Id: wlc_apcs.h 555916 2015-05-11 23:07:28Z $
 */


#ifndef _wlc_apcs_h_
#define _wlc_apcs_h_

#ifdef APCS
extern int wlc_cs_scan_start(wlc_bsscfg_t *cfg, wl_uint32_list_t *request, bool bw40,
	bool active, bool periodic, int bandtype, uint8 reason,
	void (*cb)(void *arg, int status), void *arg);

extern cs_info_t* wlc_apcs_attach(wlc_info_t *wlc);
extern void wlc_apcs_detach(cs_info_t *cs);
#endif /* APCS */

#endif	/* _wlc_apcs_h_ */
