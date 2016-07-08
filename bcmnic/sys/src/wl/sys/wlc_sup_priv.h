/*
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
 * $Id: wlc_sup_priv.h$
 */


#ifndef _wlc_sup_priv_h_
#define _wlc_sup_priv_h_

struct wlc_sup_info {
	/* references to driver `common' things */
	wlc_info_t *wlc;                /**< pointer to main wlc structure */
	wlc_pub_t *pub;                 /**< pointer to wlc public portion */
	void *wl;                       /**< per-port handle */
	osl_t *osh;                     /**< PKT* stuff wants this */
	bool sup_m3sec_ok;              /**< to selectively allow incorrect bit in M3 */
	int sup_wpa2_eapver;            /**< for choosing eapol version in M2 */
	bcm_notif_h                     sup_up_down_notif_hdl; /**< init notifier handle. */
	int cfgh;                       /**< bsscfg cubby handle */
};

/** Supplicant top-level structure hanging off bsscfg */
struct supplicant {
	wlc_info_t *wlc;                /**< pointer to main wlc structure */
	wlc_pub_t *pub;                 /**< pointer to wlc public portion */
	void *wl;                       /**< per-port handle */
	osl_t *osh;                     /**< PKT* stuff wants this */
	wlc_bsscfg_t *cfg;              /**< pointer to sup's bsscfg */
	wlc_sup_info_t *m_handle;       /**< module handle */

	struct ether_addr peer_ea;      /**< peer's ea */

	wpapsk_t *wpa;                  /**< volatile, initialized in set_sup */
	wpapsk_info_t *wpa_info;                /**< persistent wpa related info */
	unsigned char ap_eapver;        /**< eapol version from ap */
#if defined(BCMSUP_PSK)
	uint32 wpa_psk_tmo; /**< 4-way handshake timeout */
	uint32 wpa_psk_timer_active;    /**< 4-way handshake timer active */
	struct wl_timer *wpa_psk_timer; /**< timer for 4-way handshake */
#endif  /* BCMSUP_PSK */

	/* items common to any supplicant */
	int sup_type;                   /**< supplicant discriminator */
	bool sup_enable_wpa;            /**< supplicant WPA on/off */

	uint npmkid_sup;
	sup_pmkid_t *pmkid_sup[SUP_MAXPMKID];
};

typedef struct supplicant supplicant_t;

struct bss_sup_info {
	supplicant_t bss_priv;
};
typedef struct bss_sup_info bss_sup_info_t;


#define SUP_BSSCFG_CUBBY_LOC(sup, cfg) ((supplicant_t **)BSSCFG_CUBBY(cfg, (sup)->cfgh))
#define SUP_BSSCFG_CUBBY(sup, cfg) (*SUP_BSSCFG_CUBBY_LOC(sup, cfg))

/* Simplify maintenance of references to driver `common' items. */
#define UNIT(ptr)       ((ptr)->pub->unit)
#define CUR_EA(ptr)     (((supplicant_t *)ptr)->cfg->cur_etheraddr)
#define PEER_EA(ptr)    (((supplicant_t *)ptr)->peer_ea)
#define BSS_EA(ptr)     (((supplicant_t *)ptr)->cfg->BSSID)
#define BSS_SSID(ptr)   (((supplicant_t *)ptr)->cfg->current_bss->SSID)
#define BSS_SSID_LEN(ptr)       (((supplicant_t *)ptr)->cfg->current_bss->SSID_len)
#define OSH(ptr)        ((ptr)->osh)
#endif /* _wlc_sup_priv_h_ */
