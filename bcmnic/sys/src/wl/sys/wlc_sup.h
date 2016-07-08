/*
 * Exposed interfaces of wlc_sup.c
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
 * $Id: wlc_sup.h 564097 2015-06-16 14:00:38Z $
 */


#ifndef _wlc_sup_h_
#define _wlc_sup_h_


#ifdef BCMSUP_PSK
#include <bcmwpa.h>
#endif
#include <wlc_wpa.h>
#include <wlc_pmkid.h>

extern wlc_sup_info_t * wlc_sup_attach(wlc_info_t *wlc);
extern void wlc_sup_detach(wlc_sup_info_t *sup_info);
#ifdef BCMULP
extern int wlc_sup_preattach(void);
#endif

/* Down the supplicant, return the number of callbacks/timers pending */
extern int wlc_sup_down(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg);

#if defined(BCMSUP_PSK) || defined(BCMAUTH_PSK)
/* Send received EAPOL to supplicant; Return whether packet was used
 * (might still want to send it up to other supplicant)
 */
extern bool wlc_sup_eapol(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg,
	eapol_header_t *eapol_hdr, bool encrypted);
#endif 


/* Values for type parameter of wlc_set_sup() */
#define SUP_UNUSED	0 /**< Supplicant unused */
#if defined(BCMSUP_PSK)
#define SUP_WPAPSK	2 /**< Used for WPA-PSK */
#endif /* BCMSUP_PSK */

#define BSS_SUP_TYPE(idsup, cfg) wlc_get_sup(idsup, cfg)
#define BSS_SUP_ENAB_WPA(idsup, cfg)  wlc_get_wpa_enab(idsup, cfg)

extern bool wlc_set_sup(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg, int type,
	/* parameters used only for PSK follow */
	uint8 *sup_ies, uint sup_ies_len, uint8 *auth_ies, uint auth_ies_len);
int wlc_get_sup(wlc_sup_info_t *idsup, wlc_bsscfg_t *cfg);

extern void wlc_sup_set_ea(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg, struct ether_addr *ea);

/** helper fn to find supplicant and authenticator ies from assocreq and prbresp */
extern void wlc_find_sup_auth_ies(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg,
	uint8 **sup_ies, uint *sup_ies_len, uint8 **auth_ies, uint *auth_ies_len);

extern unsigned char wlc_sup_geteaphdrver(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg);

#if defined(BCMSUP_PSK) || defined(WLFBT)
extern int wlc_set_pmk(wlc_info_t *wlc, wpapsk_info_t *info, wpapsk_t *wpa,
	struct wlc_bsscfg *cfg, wsec_pmk_t *pmk, bool assoc);
#endif /* defined(BCMSUP_PSK) || defined(WLFBT) */

#ifdef	BCMSUP_PSK
/** Install WPA PSK material in supplicant */
extern int wlc_sup_set_pmk(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg,
	wsec_pmk_t *psk, bool assoc);

/** Send SSID to supplicant for PMK computation */
extern int wlc_sup_set_ssid(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg,
	uchar ssid[], int ssid_len);

/** tell supplicant to send a MIC failure report */
extern bool wlc_sup_send_micfailure(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg,
	bool ismulti);
#endif	/* BCMSUP_PSK */

#ifdef BCMSUP_PSK
extern bool wlc_wpa_sup_sendeapol(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg,
	uint16 flags, wpa_msg_t msg);
#endif /* BCMSUP_PSK */


#if defined(BCMSUP_PSK)
/** Send a supplicant status event */
extern void wlc_wpa_send_sup_status(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg,
	uint reason);
#else
#define wlc_wpa_send_sup_status(sup, reason) do {} while (0)
#endif 

#if defined(WOWL) && defined(BCMSUP_PSK)
extern uint16 aes_invsbox[];
extern uint16 aes_xtime9dbe[];
extern void *wlc_sup_hw_wowl_init(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg);
extern void wlc_sup_sw_wowl_update(struct wlc_sup_info *sup_info, struct wlc_bsscfg *cfg);
#else
#define wlc_sup_hw_wowl_init(a, b) NULL
#define wlc_sup_sw_wowl_update(a, b) do { } while (0)
#endif /* defined(WOWL) && defined (BCMSUP_PSK) */

#ifdef WLWNM
extern void wlc_wpa_sup_gtk_update(wlc_sup_info_t *sup, wlc_bsscfg_t *cfg,
	int index, int key_len, uint8 *key, uint8 *rsc);
#endif

extern bool
wlc_sup_find_pmkid(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg,
struct ether_addr *bssid, uint8	*pmkid);

extern void
wlc_sup_clear_pmkid_store(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg);


/** sup init event data. */
typedef struct sup_init_event_data
{
	/* BSSCFG instance data. */
	wlc_bsscfg_t	*bsscfg;
	wpapsk_t *wpa;
	wpapsk_info_t *wpa_info;
	bool	up;
} sup_init_event_data_t;

typedef void (*sup_init_fn_t)(void *ctx, sup_init_event_data_t *evt);
extern int BCMATTACHFN(wlc_sup_up_down_register)(struct wlc_info *wlc, sup_init_fn_t callback,
	void *arg);

extern int BCMATTACHFN(wlc_sup_up_down_unregister)(struct wlc_info *wlc, sup_init_fn_t callback,
	void *arg);

#if defined(BCMSUP_PSK) && defined(WLFBT)
extern void wlc_sup_clear_replay(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg);
#endif

extern int wlc_sup_set_pmkid(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg, uint8 *pmk,
	ushort pmk_len, struct ether_addr *auth_ea, uint8 *pmkid_out);

extern void wlc_sup_add_pmkid(wlc_sup_info_t *sup_info, wlc_bsscfg_t *cfg,
    struct ether_addr *auth_ea, uint8 *pmkid);

bool wlc_get_wpa_enab(wlc_sup_info_t *idsup, wlc_bsscfg_t *cfg);

#endif	/* _wlc_sup_h_ */
