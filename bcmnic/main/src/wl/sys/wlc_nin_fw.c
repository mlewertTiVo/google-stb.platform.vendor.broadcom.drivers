
/*   _____________________________________________________________________
 *  |                                                                     |
 *  |  Copyright (C) 2003 Mitsumi electric co,. ltd. All Rights Reserved. |
 *  |  This software contains the valuable trade secrets of Mitsumi.      |
 *  |  The software is protected under copyright laws as an unpublished   |
 *  |  work of Mitsumi. Notice is for informational purposes only and     |
 *  |  does not imply publication. The user of this software may make     |
 *  |  copies of the software for use with parts manufactured by Mitsumi  |
 *  |  or under license from Mitsumi and for no other use.                |
 *  |_____________________________________________________________________|
 */

/*
 * Nintendo utilities always in firmware
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
 * $Id: wlc_nin_fw.c 467328 2014-04-03 01:23:40Z $
 */

#include <wlc_cfg.h>

#include <typedefs.h>

#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <proto/802.11.h>
#include <wlioctl.h>
#include <epivers.h>

#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_rate_sel.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <wlc_frmutil.h>
#include <wl_export.h>
#include <sbpcmcia.h>


#include <wlioctl.h>
#include <wlc_pub.h>
#include <wl_dbg.h>

#include <nintendowm.h>
#include <wlc_nitro.h>
#include <proto/nitro.h>
#include <wlc_nin.h>

/*
 * Send frames up with brcm ether type encapsulation.
 */

void
wlc_nin_sendup(wlc_info_t *wlc, void *pkt, struct wlc_if *wlcif)
{
	struct ether_header *eth;
	int hdrm = PKTHEADROOM(wlc->osh, pkt);

	ASSERT(hdrm >= ETHER_HDR_LEN);
	if (hdrm < ETHER_HDR_LEN) {
		WL_ERROR((
			"wl%d: wlc_nin_sendup: insufficient pkt headroom, discarding\n",
			wlc->pub->unit));
		PKTFREE(wlc->osh, pkt, FALSE);
		return;
	}
	eth = (struct ether_header *)PKTPUSH(wlc->osh, pkt, ETHER_HDR_LEN);
	bcopy(&wlc->pub->cur_etheraddr, (char *)&eth->ether_dhost, ETHER_ADDR_LEN);
	bcopy(&wlc->pub->cur_etheraddr, (char *)&eth->ether_shost, ETHER_ADDR_LEN);
	ETHER_SET_LOCALADDR(&eth->ether_shost);
	eth->ether_type = hton16(ETHER_TYPE_BRCM);

	wl_sendup(wlc->wl, (wlcif ? wlcif->wlif : NULL), pkt, 1);
}
void
wlc_nin_create_iapp_ind(struct wlc_info *wlc, void *p, int len)
{
	char *ppld;
	nwm_ma_iapp_ind_t *pind;
	int hdrm = PKTHEADROOM(wlc->osh, p);
	int hdrm_reqd = sizeof(nwm_ma_iapp_ind_t) + ETHER_HDR_LEN + 2;

	ASSERT(hdrm >= hdrm_reqd);
	if (hdrm < hdrm_reqd) {
		WL_ERROR(("wl%d %s: insufficient pkt headroom: %d, Need %d\n",
			wlc->pub->unit, __FUNCTION__, hdrm, hdrm_reqd));
		PKTFREE(wlc->osh, p, FALSE);
		return;
	}
	/* shift the original packet incl ether hdr up to 4 byte boundary */
	ppld = (char *)PKTPUSH(wlc->osh, p, 2);
	memmove(ppld, ppld + 2, len);

	pind = (nwm_ma_iapp_ind_t *)PKTPUSH(wlc->osh, p,
		NWM_MA_IAPP_FIXED_SIZE);
	bzero(pind, NWM_MA_IAPP_FIXED_SIZE);
	pind->header.indcode   = NWM_CMDCODE_MA_IAPP_IND;
	pind->header.indlength = len/2;
	pind->header.magic     = NWM_NIN_IND_MAGIC;
	PKTSETLEN(wlc->osh, p, NWM_MA_IAPP_FIXED_SIZE + len);
	ASSERT(ISALIGNED(pind, 4));
	wlc_nin_sendup(wlc, p, NULL);
}
