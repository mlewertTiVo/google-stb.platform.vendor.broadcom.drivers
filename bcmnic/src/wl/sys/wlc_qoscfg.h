/*
 * 802.11 QoS/EDCF/WME/APSD configuration and utility module interface
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_qoscfg.h 650062 2016-07-20 10:02:05Z $
 */

#ifndef _wlc_qoscfg_h_
#define _wlc_qoscfg_h_

#include <typedefs.h>
#include <bcmdefs.h>
#include <wlc_types.h>

/* an arbitary number for unbonded USP */
#define WLC_APSD_USP_UNB 0xfff

/* Per-AC retry limit register definitions; uses bcmdefs.h bitfield macros */
#define EDCF_SHORT_S            0
#define EDCF_SFB_S              4
#define EDCF_LONG_S             8
#define EDCF_LFB_S              12
#define EDCF_SHORT_M            BITFIELD_MASK(4)
#define EDCF_SFB_M              BITFIELD_MASK(4)
#define EDCF_LONG_M             BITFIELD_MASK(4)
#define EDCF_LFB_M              BITFIELD_MASK(4)

#define WME_RETRY_SHORT_GET(val, ac)        GFIELD(val, EDCF_SHORT)
#define WME_RETRY_SFB_GET(val, ac)          GFIELD(val, EDCF_SFB)
#define WME_RETRY_LONG_GET(val, ac)         GFIELD(val, EDCF_LONG)
#define WME_RETRY_LFB_GET(val, ac)          GFIELD(val, EDCF_LFB)

#define WLC_WME_RETRY_SHORT_GET(wlc, ac)    GFIELD(wlc->wme_retries[ac], EDCF_SHORT)
#define WLC_WME_RETRY_SFB_GET(wlc, ac)      GFIELD(wlc->wme_retries[ac], EDCF_SFB)
#define WLC_WME_RETRY_LONG_GET(wlc, ac)     GFIELD(wlc->wme_retries[ac], EDCF_LONG)
#define WLC_WME_RETRY_LFB_GET(wlc, ac)      GFIELD(wlc->wme_retries[ac], EDCF_LFB)

#define WLC_WME_RETRY_SHORT_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_SHORT, val)
#define WLC_WME_RETRY_SFB_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_SFB, val)
#define WLC_WME_RETRY_LONG_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_LONG, val)
#define WLC_WME_RETRY_LFB_SET(wlc, ac, val) \
	wlc->wme_retries[ac] = SFIELD(wlc->wme_retries[ac], EDCF_LFB, val)

uint16 wlc_wme_get_frame_medium_time(wlc_info_t *wlc, uint8 bandunit,
	ratespec_t ratespec, uint8 preamble_type, uint mac_len);

void wlc_wme_retries_write(wlc_info_t *wlc);
void wlc_wme_shm_read(wlc_info_t *wlc);

int wlc_edcf_acp_apply(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend);
void wlc_edcf_acp_get_all(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp);
int wlc_edcf_acp_set_one(wlc_info_t *wlc, wlc_bsscfg_t *cfg, edcf_acparam_t *acp,
	bool suspend);

extern void wlc_qosinfo_update(struct scb *scb, uint8 qosinfo, bool ac_upd);

/* attach/detach interface */
wlc_qos_info_t *wlc_qos_attach(wlc_info_t *wlc);
void wlc_qos_detach(wlc_qos_info_t *qosi);

/* ******** WORK-IN-PROGRESS ******** */

/* TODO: make data structures opaque */

/* per bsscfg wme info */
struct wlc_wme {
	/* IE */
	wme_param_ie_t	wme_param_ie;		/**< WME parameter info element,
						 * contains parameters in use locally.
						 */
	wme_param_ie_t	*wme_param_ie_ad;	/**< WME parameter info element,
						 * contains parameters advertised to STA
						 * in beacons and assoc responses.
						 */
	/* EDCF */
	uint16		edcf_txop[AC_COUNT];	/**< current txop for each ac */
	/* APSD */
	ac_bitmap_t	apsd_trigger_ac;	/**< Permissible Acess Category in which APSD Null
						 * Trigger frames can be send
						 */
	bool		apsd_auto_trigger;	/**< Enable/Disable APSD frame after indication in
						 * Beacon's PVB
						 */
	uint8		apsd_sta_qosinfo;	/**< Application-requested APSD configuration */
	/* WME */
	bool		wme_apsd;		/**< enable Advanced Power Save Delivery */
	bool		wme_noack;		/**< enable WME no-acknowledge mode */
	ac_bitmap_t	wme_admctl;		/**< bit i set if AC i under admission control */
};

extern const char *const aci_names[];
extern const uint8 wme_ac2fifo[];

/* ******** WORK-IN-PROGRESS ******** */

#endif /* _wlc_qoscfg_h_ */
