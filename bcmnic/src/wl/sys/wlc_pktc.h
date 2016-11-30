/*
 * Packet chaining module header file
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
 * $Id: wlc_pktc.h 615360 2016-01-27 09:44:24Z $
 */

#ifndef __wlc_pktc_h__
#define __wlc_pktc_h__

#include <typedefs.h>
#include <wlc_types.h>
#include <proto/ethernet.h>

#define PKTC_POLICY_SENDUPC	0	/* Default, send up chained */
#define PKTC_POLICY_SENDUPUC	1	/* Send up unchained */

/** Receive frames chaining info */
struct wlc_rfc {
	wlc_bsscfg_t		*bsscfg;
	struct scb		*scb;
	void			*wlif_dev;
	struct ether_addr	ds_ea;
	uint8			prio;
	uint8			ac;
};

/** Module info */
/*
 * TODO: make this opaque and create functional APIs.
 */
struct wlc_pktc_info {
	wlc_info_t		*wlc;
	struct	wlc_rfc		rfc[2];		/**< receive chaining info */
	struct ether_addr	*h_da;		/**< da of head frame in chain */
	uint8			policy;		/**< chaining policy */
	struct scb		*h_scb;		/* scb of last processed pkt */
};

/* Access rfc. Will change when modularized */
#define PKTC_RFC(_pktc_, _ap_)		&(_pktc_)->rfc[(_ap_) ? 1 : 0]
/* Access h_da. Will change when modularized */
#define PKTC_HDA(_pktc_)		(_pktc_)->h_da
#define PKTC_HSCB(_pktc_)		(_pktc_)->h_scb
#define PKTC_HDA_SET(_pktc_, _val_)	(_pktc_)->h_da = (_val_)
#define PKTC_HSCB_SET(_pktc_, _val_)	(_pktc_)->h_scb = (_val_)

/* attach/detach */
wlc_pktc_info_t *wlc_pktc_attach(wlc_info_t *wlc);
void wlc_pktc_detach(wlc_pktc_info_t *pktc);

/*
 * TODO: follow the modular interface convention - pass module handle as the first
 * param in func's param list.
 */

void wlc_scb_pktc_enable(struct scb *scb, const wlc_key_info_t *key_info);

void wlc_pktc_sdu_prep(wlc_info_t *wlc, struct scb *scb, void *pkt, void *n, uint32 lifetime);
#ifdef DMATXRC
/** Prepend phdr and move p's 802.3 header/flags/attributes to phdr */
void *wlc_pktc_sdu_prep_phdr(wlc_info_t *wlc, struct scb *scb, void *pkt, uint32 lifetime);
#endif

#endif /* __wlc_pktc_h__ */
