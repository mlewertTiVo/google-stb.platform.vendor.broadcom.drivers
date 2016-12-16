/*
 * 802.11ax HE (High Efficiency) STA signaling and d11 h/w manipulation.
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
 * $Id: wlc_he.h 665619 2016-10-18 12:43:21Z $
 */

#ifndef _wlc_he_h_
#define _wlc_he_h_

#include <wlc_types.h>
#include <wlc_twt.h>

#include <proto/802.11ax.h>

/* attach/detach */
wlc_he_info_t *wlc_he_attach(wlc_info_t *wlc);
void wlc_he_detach(wlc_he_info_t *hei);

void wlc_he_init_defaults(wlc_he_info_t *hei);

/* features */
#define WLC_HE_FEATURES_GET(pub, mask) ((pub)->he_features & (mask))
#define WLC_HE_FEATURES_SET(pub, mask) ((pub)->he_features |= (mask))
#define WLC_HE_FEATURES_CLR(pub, mask) ((pub)->he_features &= ~(mask))

/*
 * HE features bitmap.
 * Bit 0:		HE 5G support
 * Bit 1:		HE 2G support
 */
#define WL_HE_FEATURES_5G	0x1
#define WL_HE_FEATURES_2G	0x2

#ifdef WL11AX
#define WLC_HE_FEATURES_5G(pub) (WLC_HE_FEATURES_GET(pub, WL_HE_FEATURES_5G) != 0)
#define WLC_HE_FEATURES_2G(pub) (WLC_HE_FEATURES_GET(pub, WL_HE_FEATURES_2G) != 0)
#else
#define WLC_HE_FEATURES_5G(pub) (FALSE)
#define WLC_HE_FEATURES_2G(pub) (FALSE)
#endif

/* update scb */
void wlc_he_update_scb_state(wlc_he_info_t *hei, int bandtype, struct scb *scb,
	he_cap_ie_t *capie, he_op_ie_t *opie);
void wlc_he_bcn_scb_upd(wlc_he_info_t *hei, int bandtype, struct scb *scb,
	he_cap_ie_t *capie, he_op_ie_t *opie);

/* process action frame */
int wlc_he_actframe_proc(wlc_he_info_t *hei, uint action_id, struct scb *scb,
	uint8 *body, int body_len);

/* twt setup interface */
typedef struct {
	int status;		/* BCME_XXXX. BCME_OK for a successful setup */
	uint16 dialog_token;	/* same as the one specified in or returned from
				 * the call to wlc_he_twt_setup() if AUTO.
				 */
	wl_twt_sdesc_t desc;	/* TWT Setup parameters, valid when status is BCME_OK */
} wlc_he_twt_setup_cplt_data_t;
typedef void (*wlc_he_twt_setup_cplt_fn_t)
	(void *ctx, struct scb *scb, wlc_he_twt_setup_cplt_data_t *rslt);
typedef struct {
	wlc_he_twt_setup_cplt_fn_t cplt_fn;
				/* completion callback fn, will be invoked
				 * when the call to wlc_he_twt_setup() succeeds
				 * and when the setup process is finished.
				 */
	void *cplt_ctx;		/* completion callback context */
	uint16 dialog_token;	/* input: AUTO (0xFFFF) or user specified (< 256);
				 * output: assigned if AUTO or user specified
				 */
	wl_twt_sdesc_t desc;	/* TWT Setup parameters */
} wlc_he_twt_setup_t;
int wlc_he_twt_setup(wlc_he_info_t *hei, struct scb *scb, wlc_he_twt_setup_t *desc);

#define WLC_HE_TWT_SETUP_DIALOG_TOKEN_AUTO 0xFFFF

/* twt teardown interface */
int wlc_he_twt_teardown(wlc_he_info_t *hei, struct scb *scb, uint8 flow_id);

/* twt information interface */
int wlc_he_twt_info(wlc_he_info_t *hei, struct scb *scb, wl_twt_idesc_t *desc);

#define BSSCOLOR_EN  TRUE
#define BSSCOLOR_DIS FALSE

#ifdef WL11AX
void wlc_he_on_switch_bsscfg(wlc_he_info_t *hei, wlc_bsscfg_t *cfg, bool bsscolor_enable);
#else
#define wlc_he_on_switch_bsscfg(hei, cfg, bsscolor_enable)
#endif /* WL11AX */

#endif /* _wlc_he_h_ */
