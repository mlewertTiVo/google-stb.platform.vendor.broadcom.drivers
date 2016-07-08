/*
 * WAPI (WLAN Authentication and Privacy Infrastructure) private header file
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_wapi_priv.h 467328 2014-04-03 01:23:40Z $
 */
#ifndef _wlc_wapi_priv_h_
#define _wlc_wapi_priv_h_

struct wlc_wapi_info {
	wlc_info_t *wlc;
	wlc_pub_t *pub;
	int cfgh;			/* bsscfg cubby handle to retrieve data from bsscfg */
	uint priv_offset;		/* offset of private bsscfg cubby */
};


#endif /* _wlc_wapi_priv_h_ */
