/*
 * AP Module Public Interface
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
 * $Id: wlc_ap.h 636823 2016-05-10 21:38:16Z $
 */
#ifndef _WLC_AP_H_
#define _WLC_AP_H_

#if defined(RXCHAIN_PWRSAVE) || defined(RADIO_PWRSAVE)
#define WLPWRSAVERXFADD(wlc, v)	do { if ((wlc)->ap != NULL) (wlc)->ap->rxframe += (v); } while (0)
#define WLPWRSAVERXFINCR(wlc)	do { if ((wlc)->ap != NULL) (wlc)->ap->rxframe++; } while (0)
#define WLPWRSAVETXFADD(wlc, v)	do { if ((wlc)->ap != NULL) (wlc)->ap->txframe += (v); } while (0)
#define WLPWRSAVETXFINCR(wlc)	do { if ((wlc)->ap != NULL) (wlc)->ap->txframe++; } while (0)
#define WLPWRSAVERXFVAL(wlc)	(((wlc)->ap != NULL) ? (wlc)->ap->rxframe : 0)
#define WLPWRSAVETXFVAL(wlc)	(((wlc)->ap != NULL) ? (wlc)->ap->txframe : 0)
#endif

struct wlc_ap_info {
	bool		shortslot_restrict;	/**< only allow assoc by shortslot STAs */
	uint8		radio_pwrsave_enable;	/**< radio duty cycle power save enable */
	bool		dcs_enabled;
	uint8		rxchain_pwrsave_enable;	/**< rxchain based power save enable */
	uint32		rxframe;		/**< receive frame counter */
	uint32		txframe;		/**< transmit frame counter */
};

#ifdef BAND5G
#define WL11H_AP_ENAB(wlc)	(AP_ENAB((wlc)->pub) && WL11H_ENAB(wlc))
#else
#define WL11H_AP_ENAB(wlc)	0
#endif /* BAND5G */

#ifdef RXCHAIN_PWRSAVE
#define RXCHAIN_PWRSAVE_ENAB(ap) ((ap)->rxchain_pwrsave_enable)
#else
#define RXCHAIN_PWRSAVE_ENAB(ap) 0
#endif

#ifdef RADIO_PWRSAVE
#define RADIO_PWRSAVE_ENAB(ap) ((ap)->radio_pwrsave_enable)
#else
#define RADIO_PWRSAVE_ENAB(ap) 0
#endif

/* TODO: these SCB_XXXX were AP specific parameters and should not have been used
 * outside of the AP module but...
 * move them back inside .c file once the references from other modules are removed.
 */
#define SCB_TIMEOUT		  60	/**< # seconds: interval to probe idle STAs */
#define SCB_ACTIVITY_TIME	   5	/**< # seconds: skip probe if activity during this time */

/* Idea is to find if STA (connected to upstream AP) and
 * local AP are on overlapping RADAR channels. If overlapping
 * and both on radar channel, then ap_sta_onradar should help
 * local AP to not start CAC, instead proceed to ISM. In ISM
 * local AP ignores radar detection now that STA is conneccted
 * to upstream AP.
 * At the point STA connected to upstream AP dis-connects, this
 * flag will help in not ignore the radar_detect in ISM.
 *
 * NOTE: There is an assumption here that in AP STA mode, always
 * STA comes up first and connectes to upstream AP before even
 * we bringup local AP interface. Otherwise if AP happens to come
 * up first then we get into CAC (phy_muted) which will stop
 * sta from connecting to upstream-AP.
 */
#define WLC_APSTA_ON_RADAR_CHANNEL(wlc)	(wlc_apsta_on_radar_channel(wlc->ap) == TRUE)

#ifdef AP
extern wlc_ap_info_t* wlc_ap_attach(wlc_info_t *wlc);
extern void wlc_ap_detach(wlc_ap_info_t *ap);
extern int wlc_ap_up(wlc_ap_info_t *apinfo, wlc_bsscfg_t *bsscfg);
extern int wlc_ap_down(wlc_ap_info_t *apinfo, wlc_bsscfg_t *bsscfg);
extern void wlc_restart_ap(wlc_ap_info_t *ap);
extern void wlc_ap_authresp(wlc_ap_info_t *ap, wlc_bsscfg_t *bsscfg,
	struct dot11_management_header *hdr, uint8 *body, uint body_len, void *p,
	bool short_preamble, d11rxhdr_t *rxh);
extern void wlc_ap_process_assocreq(wlc_ap_info_t *ap, wlc_bsscfg_t *bsscfg,
	struct dot11_management_header *hdr, uint8 *body, uint body_len, struct scb *scb, bool);
extern bool wlc_roam_check(wlc_ap_info_t *ap, wlc_bsscfg_t *bsscfg,
	struct ether_header *eh, uint len);
extern bool wlc_apsta_on_radar_channel(wlc_ap_info_t *apinfo);

extern ratespec_t wlc_lowest_basicrate_get(wlc_bsscfg_t *cfg);
extern int wlc_ap_get_bcnprb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool bcn,
                             void *buf, uint *buflen);

#if defined(WLRSDB) && defined(WL_MODESW)
extern void wlc_set_ap_up_pending(wlc_info_t* wlc, wlc_bsscfg_t* bsscfg, bool val);
extern bool wlc_get_ap_up_pending(wlc_info_t* wlc, wlc_bsscfg_t* bsscfg);
#endif

extern uint wlc_ap_stas_associated(wlc_ap_info_t *ap);

extern void wlc_ap_bsscfg_scb_cleanup(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);

#ifdef RXCHAIN_PWRSAVE
extern void wlc_reset_rxchain_pwrsave_mode(wlc_ap_info_t *ap);
extern void wlc_disable_rxchain_pwrsave(wlc_ap_info_t *ap);
#ifdef WL11N
extern uint8 wlc_rxchain_pwrsave_stbc_rx_get(wlc_info_t *wlc);
#endif /* WL11N */
#endif /* RXCHAIN_PWRSAVE */

extern int wlc_ap_get_maxassoc(wlc_ap_info_t *ap);
extern void wlc_ap_set_maxassoc(wlc_ap_info_t *ap, int val);
extern int wlc_ap_get_maxassoc_limit(wlc_ap_info_t *ap);

#ifdef RADIO_PWRSAVE
extern int wlc_radio_pwrsave_in_power_save(wlc_ap_info_t *ap);
extern void wlc_radio_pwrsave_enter_mode(wlc_info_t *wlc, bool dtim);
extern void wlc_radio_pwrsave_exit_mode(wlc_ap_info_t *ap);
extern void wlc_radio_pwrsave_on_time_start(wlc_ap_info_t *ap, bool dtim);
extern bool wlc_radio_pwrsave_bcm_cancelled(const wlc_ap_info_t *ap);
#endif

extern void wlc_bss_up(wlc_ap_info_t *ap, wlc_bsscfg_t *bsscfg);

extern uint8 wlc_ap_count(wlc_ap_info_t *ap, bool include_p2p);

extern int wlc_ap_sendauth(wlc_ap_info_t *ap, wlc_bsscfg_t *bsscfg,
	struct scb *scb, int auth_alg, int auth_seq, int status,
	uint8 *challenge_text, bool short_preamble, bool send_auth);

#if defined(TXQ_MUX)
extern bool wlc_bcmc_enqueue(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, uint prec);
extern void wlc_bcmc_global_start_mux_sources(wlc_info_t *wlc);
extern void wlc_bcmc_global_stop_mux_sources(wlc_info_t *wlc);
extern void wlc_bcmc_set_powersave(wlc_bsscfg_t *cfg, bool ps_enable);

#define WLC_BCMC_PSON(cfg) wlc_bcmc_set_powersave((cfg), TRUE)
#define WLC_BCMC_PSOFF(cfg) wlc_bcmc_set_powersave((cfg), FALSE)
#define WLC_BSS_DATA_FC_ON(cfg) ((cfg)->wlc->block_datafifo != 0)
#endif /* TXQ_MUX */

struct scb *wlc_ap_get_psta_prim(wlc_ap_info_t *ap, struct scb *scb);
extern void wlc_ap_channel_switch(wlc_ap_info_t *ap, wlc_bsscfg_t *cfg);

#if defined(STA)
bool wlc_apup_allowed(wlc_info_t *wlc);
#endif /* STA */
#else /* AP */

/* Stub functions help eliminate using #ifdef AP macros */
#define wlc_ap_attach(a) (wlc_ap_info_t *)(uintptr)0xdeadc0de
#define wlc_ap_detach(a) do {} while (0)
#define wlc_ap_up(a, b) do {} while (0)
#define wlc_ap_down(a, b) 0
#define wlc_restart_ap(a) do {} while (0)
#define wlc_ap_authresp(a, b, c, d, e, f, g, i, j, k) do {} while (0)

#define wlc_ap_process_assocreq(a, b, c, d, e, f, g) do {} while (0)
#define wlc_roam_check(a, b, c, d) FALSE
#define wlc_ap_stas_associated(ap) 0

#define wlc_bss_up(ap, cfg, bcn, len) do {} while (0)

#define wlc_apsta_on_radar_channel(a)     FALSE

#define wlc_ap_sendauth(a, b, c, d, e, f, g, h, i) do {} while (0)

#define wlc_ap_get_psta_prim(a, b) NULL
#define wlc_ap_channel_switch(a, b) do {} while (0)
#define wlc_ap_count(a, b)	FALSE
#endif /* AP */

#if defined(AP) && defined(STA)
extern void wlc_ap_sta_onradar_upd(wlc_bsscfg_t *cfg);
#else
#define wlc_ap_sta_onradar_upd(c)	do {} while (0)
#endif /* AP && STA */

extern ratespec_t wlc_force_bcn_rspec(wlc_info_t *wlc);
#define wlc_wlancoex_on(c) FALSE

void wlc_bsscfg_bcn_disable(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
void wlc_bsscfg_bcn_enable(wlc_info_t *wlc, wlc_bsscfg_t *cfg);

extern int wlc_ap_timeslot_register(wlc_bsscfg_t *cfg);
extern void wlc_ap_timeslot_unregister(wlc_bsscfg_t *cfg);
extern void wlc_ap_timeslot_update(wlc_bsscfg_t *cfg, uint32 start_tsf, uint32 interval);

void wlc_ap_reset_grace_attempt(wlc_ap_info_t *ap, struct scb *scb, uint16 cnt);

uint wlc_ap_get_activity_time(wlc_ap_info_t *ap);

bool wlc_ap_sendnulldata(wlc_ap_info_t *ap, wlc_bsscfg_t *cfg, struct scb *scb);

int wlc_scb_save_wpa_ie(wlc_info_t *wlc, struct scb *scb, bcm_tlv_t *ie);
void wlc_ap_find_wpaie(wlc_ap_info_t *ap, struct scb *scb, uint8 **wpaie, uint *wpaie_len);

uint wlc_ap_get_pre_tbtt(wlc_ap_info_t *ap);

void wlc_ap_set_chanspec(wlc_ap_info_t *ap, chanspec_t chspec);

uint32 wlc_ap_getdtim_count(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
#endif /* _WLC_AP_H_ */
