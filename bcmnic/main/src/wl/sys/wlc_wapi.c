/*
 * WAPI (WLAN Authentication and Privacy Infrastructure) source file
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_wapi.c 487832 2014-06-27 05:13:40Z $
 */

/**
 * @file
 * @brief
 * WAPI is a proposal from the Chinese National Body for a new 802.11 encryption standard.
 */


#error "BCMWAPI_WPI or BCMWAPI_WAI is not defined"

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <siutils.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmwpa.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_keymgmt.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_scb.h>
#include <wlc_ap.h>
#include <wlc_wapi.h>
#include <wlc_wapi_priv.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>

#include <bcmcrypto/sms4.h>


#define wlc_wapi_iovars		NULL
#define wlc_wapi_doiovar	NULL

#define wlc_wapi_dump		NULL


/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>


/* Module attach */
wlc_wapi_info_t *
BCMATTACHFN(wlc_wapi_attach)(wlc_info_t *wlc)
{
	wlc_wapi_info_t *wapi = NULL;
	uint16 arqfstbmp = FT2BMP(FC_ASSOC_REQ) | FT2BMP(FC_REASSOC_REQ);
	uint16 bcnfstbmp = FT2BMP(FC_BEACON) | FT2BMP(FC_PROBE_RESP);
	uint16 scanfstbmp = FT2BMP(WLC_IEM_FC_SCAN_BCN) | FT2BMP(WLC_IEM_FC_SCAN_PRBRSP);

	if ((wapi = MALLOCZ(wlc->osh, sizeof(wlc_wapi_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	wapi->wlc = wlc;
	wapi->pub = wlc->pub;

	/* default enable wapi */
	wapi->pub->_wapi_hw_wpi = WAPI_HW_WPI_CAP(wlc);


	/* keep the module registration the last other add module unregistratin
	 * in the error handling code below...
	 */
	if (wlc_module_register(wlc->pub, wlc_wapi_iovars, "wapi", (void *)wapi,
		wlc_wapi_doiovar, NULL, NULL, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	};


	return wapi;

	/* error handling */
fail:
	if (wapi != NULL)
		MFREE(wlc->osh, wapi, sizeof(wlc_wapi_info_t));

	return NULL;
}

/* Module detach */
void
BCMATTACHFN(wlc_wapi_detach)(wlc_wapi_info_t *wapi)
{
	if (wapi == NULL)
		return;

	wlc_module_unregister(wapi->pub, "wapi", wapi);
	MFREE(wapi->wlc->osh, wapi, sizeof(wlc_wapi_info_t));
}
