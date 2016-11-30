/*
 * Exposed interfaces of wlc_auth.c
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
 * $Id: wlc_auth.h 636200 2016-05-06 19:51:20Z $
 */

#ifndef _wlc_auth_h_
#define _wlc_auth_h_

#include <proto/ethernet.h>
#include <proto/eapol.h>

/* Values for type parameter of wlc_set_auth() */
#define AUTH_UNUSED	0	/* Authenticator unused */
#define AUTH_WPAPSK	1	/* Used for WPA-PSK */

#ifdef BCMAUTH_PSK
#define BSS_AUTH_TYPE(authi, cfg)	wlc_get_auth(authi, cfg)
#else
#define BSS_AUTH_TYPE(authi, cfg)	AUTH_UNUSED
#endif /* BCMAUTH_PSK */

extern wlc_auth_info_t* wlc_auth_attach(wlc_info_t *wlc);
extern void wlc_auth_detach(wlc_auth_info_t *auth_info);

/* Install WPA PSK material in authenticator */
extern int wlc_auth_set_pmk(wlc_auth_info_t *authi, wlc_bsscfg_t *cfg, wsec_pmk_t *psk);

extern bool wlc_set_auth(wlc_auth_info_t *authi, wlc_bsscfg_t *cfg, int type,
	uint8 *sup_ies, uint sup_ies_len, uint8 *auth_ies, uint auth_ies_len, struct scb *scb);
int wlc_get_auth(wlc_auth_info_t *authi, wlc_bsscfg_t *cfg);

extern bool wlc_auth_eapol(wlc_auth_info_t *authi, wlc_bsscfg_t *cfg,
	eapol_header_t *eapol_hdr, bool encrypted, struct scb *scb);

extern void wlc_auth_tkip_micerr_handle(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

#endif	/* _wlc_auth_h_ */
