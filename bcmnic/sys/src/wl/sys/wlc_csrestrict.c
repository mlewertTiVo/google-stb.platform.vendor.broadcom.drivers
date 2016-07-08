/*
 * Regulated MPDU Release after Channel Switch feature.
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
 * $Id: wlc_csrestrict.c 556701 2015-05-14 17:22:38Z $
 */

/* Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_channel.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_ampdu.h>
#include <wlc_csrestrict.h>

#ifdef WL_CS_RESTRICT_RELEASE
static void
wlc_scb_restrict_wd(void *ctx)
{
	wlc_info_t *wlc = (wlc_info_t *)ctx;
	struct scb *scb;
	struct scb_iter scbiter;

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		if (scb->restrict_deadline) {
			scb->restrict_deadline--;
		}
		if (!scb->restrict_deadline) {
			scb->restrict_txwin = 0;
		}
	}
}

static void
wlc_scb_restrict_start(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	struct scb *scb;
	struct scb_iter scbiter;

	FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
		if (!SCB_ISMULTI(scb) && SCB_AMPDU(scb)) {
			scb->restrict_txwin = SCB_RESTRICT_MIN_TXWIN;
			scb->restrict_deadline = SCB_RESTRICT_WD_TIMEOUT + 1;
		}
	}
}

void
wlc_restrict_csa_start(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool start)
{
	if (start) {
		wlc_scb_restrict_start(wlc, cfg);
	}
	wlc_ampdu_txeval_all(wlc);
}

int
BCMATTACHFN(wlc_restrict_attach)(wlc_info_t *wlc)
{
	return wlc_module_register(wlc->pub, NULL, "restrict", wlc, NULL,
	                           wlc_scb_restrict_wd, NULL, NULL);
}

void
BCMATTACHFN(wlc_restrict_detach)(wlc_info_t *wlc)
{
	wlc_module_unregister(wlc->pub, "restrict", wlc);
}
#endif /* WL_CS_RESTRICT_RELEASE */
