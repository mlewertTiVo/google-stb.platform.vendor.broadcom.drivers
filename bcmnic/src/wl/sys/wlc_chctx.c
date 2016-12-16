/**
 * Channel Context module WLC module wrapper - A SHARED CHANNEL CONTEXT MANAGEMENT.
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
 * $Id: wlc_chctx.c 658442 2016-09-08 01:17:56Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <wlc_types.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_dump.h>
#include <wlc_chctx_reg.h>
#include <wlc_chctx.h>

/*
 * Operating channel context cache configuration for multi-channel links
 */
/* # operating channels for all links */
#ifndef WLC_CHCTX_MCLNK_CONTEXTS
#define WLC_CHCTX_MCLNK_CONTEXTS	0
#endif

/* # registered clients for all features */
#ifndef WLC_CHCTX_MCLNK_CLIENTS
#define WLC_CHCTX_MCLNK_CLIENTS	0
#endif

/* # registered cubbies for all wlc wide features */
#ifndef WLC_CHCTX_MCLNK_CUBBIES_WLC
#define WLC_CHCTX_MCLNK_CUBBIES_WLC	0
#endif
/* # registered cubbies for all bss wide features */
#ifndef WLC_CHCTX_MCLNK_CUBBIES_CFG
#define WLC_CHCTX_MCLNK_CUBBIES_CFG	0
#endif
/* # registered cubbies for all link wide features */
#ifndef WLC_CHCTX_MCLNK_CUBBIES_SCB
#define WLC_CHCTX_MCLNK_CUBBIES_SCB	0
#endif
/* # registered cubbies for all features */
#ifndef WLC_CHCTX_MCLNK_CUBBIES
#define WLC_CHCTX_MCLNK_CUBBIES \
	(WLC_CHCTX_MCLNK_CUBBIES_WLC + \
	 WLC_CHCTX_MCLNK_CUBBIES_CFG * WLC_MAXBSSCFG + \
	 WLC_CHCTX_MCLNK_CUBBIES_SCB * MAXSCB + \
	 0)
#endif

/* local functions declarations */
static void wlc_chctx_chansw_cb(void *arg, wlc_chansw_notif_data_t *data);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int wlc_chctx_dump(void *ctx, struct bcmstrbuf *b);
#endif

/* private module info */
typedef struct {
	wlc_info_t *wlc;
} wlc_chctx_info_t;

/* pre attach/detach - called early in the wlc attach path without other dependencies */
wlc_chctx_reg_t *
BCMATTACHFN(wlc_chctx_pre_attach)(wlc_info_t *wlc)
{
	wlc_chctx_reg_t *chctx;
	wlc_chctx_info_t *info;

	if ((chctx = wlc_chctx_reg_attach(wlc->osh, sizeof(*info), WLC_CHCTX_REG_H_TYPE,
			WLC_CHCTX_MCLNK_CLIENTS, WLC_CHCTX_MCLNK_CUBBIES,
			WLC_CHCTX_MCLNK_CONTEXTS)) == NULL) {
		WL_ERROR(("wl%d: %s: wlc_chctx_reg_attach failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto exit;
	}

	info = wlc_chctx_reg_user_ctx(chctx);
	info->wlc = wlc;

exit:
	return chctx;
}

void
BCMATTACHFN(wlc_chctx_post_detach)(wlc_chctx_reg_t *chctx)
{
	wlc_chctx_reg_detach(chctx);
}

/* attach/detach - called in normal wlc module attach after wlc_assoc_attach */
int
BCMATTACHFN(wlc_chctx_attach)(wlc_chctx_reg_t *chctx)
{
	wlc_chctx_info_t *info = wlc_chctx_reg_user_ctx(chctx);
	wlc_info_t *wlc = info->wlc;
	int err;

	if ((err = wlc_chansw_notif_register(wlc, wlc_chctx_chansw_cb, info)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_chansw_notif_register failed. err=%d\n",
		          wlc->pub->unit, __FUNCTION__, err));
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	wlc_dump_add_fns(wlc->pub, "chctx", wlc_chctx_dump, NULL, info);
#endif

	return BCME_OK;

fail:
	wlc_chctx_detach(chctx);
	return err;
}

void
BCMATTACHFN(wlc_chctx_detach)(wlc_chctx_reg_t *chctx)
{
	wlc_chctx_info_t *info;
	wlc_info_t *wlc;

	if (chctx == NULL) {
		return;
	}

	info = wlc_chctx_reg_user_ctx(chctx);
	wlc = info->wlc;

	wlc_chansw_notif_unregister(wlc, wlc_chctx_chansw_cb, info);
}

/* hook info channel switch procedure... */
static void
wlc_chctx_chansw_cb(void *ctx, wlc_chansw_notif_data_t *data)
{
	wlc_chctx_info_t *info = ctx;
	wlc_info_t *wlc = info->wlc;
	wlc_chctx_reg_t *chctx = wlc->chctx;
	wlc_chctx_notif_t notif;

	notif.event = WLC_CHCTX_ENTER_CHAN;
	notif.chanspec = data->new_chanspec;

	(void)wlc_chctx_reg_notif(chctx, &notif);
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static int
wlc_chctx_dump(void *ctx, struct bcmstrbuf *b)
{
	wlc_chctx_info_t *info = ctx;
	wlc_info_t *wlc = info->wlc;
	wlc_chctx_reg_t *chctx = wlc->chctx;

	return wlc_chctx_reg_dump(chctx, b);
}
#endif
