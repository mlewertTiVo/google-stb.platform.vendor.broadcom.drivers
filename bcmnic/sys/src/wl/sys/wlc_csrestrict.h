/*
 * Regulated MPDU Release after Channel Switch interface.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_csrestrict.h 599296 2015-11-13 06:36:13Z $
 */

#ifndef _wlc_cs_restrict_h_
#define _wlc_cs_restrict_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_scb.h>
#include <wlc.h>

#ifdef WL_CS_RESTRICT_RELEASE
/**
 * Limit number of packets in transit, starting from minimal number.
 * Each time packet sent successfully using primary rate limit is
 * exponentially grow till some number, then unlimited.
 * In case of failure while limit is growing, it fall back to
 * original minimal number.
 */

/* attach/detach */
int wlc_restrict_attach(wlc_info_t *wlc);
void wlc_restrict_detach(wlc_info_t *wlc);

/* start cs restrict release for CSA */
void wlc_restrict_csa_start(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool start);

#define SCB_RESTRICT_WD_TIMEOUT		1
#define SCB_RESTRICT_MIN_TXWIN_SHIFT	1
#define SCB_RESTRICT_MAX_TXWIN_SHIFT	6

#define SCB_RESTRICT_MIN_TXWIN		(1 << (SCB_RESTRICT_MIN_TXWIN_SHIFT))
#define SCB_RESTRICT_MAX_TXWIN		(1 << (SCB_RESTRICT_MAX_TXWIN_SHIFT))

static INLINE void
wlc_scb_restrict_txstatus(struct scb *scb, bool success)
{
	if (!scb->restrict_txwin) {
		/* Disabled. Most likely case. */
	} else if (!success) {
		scb->restrict_txwin = SCB_RESTRICT_MIN_TXWIN;
	} else {
		scb->restrict_txwin = scb->restrict_txwin << 1;
		if (scb->restrict_txwin >= SCB_RESTRICT_MAX_TXWIN) {
			scb->restrict_txwin = 0;
		}
	}
}

static INLINE bool
wlc_scb_restrict_can_txq(wlc_info_t *wlc, struct scb *scb)
{
	if (!scb->restrict_txwin) {
		/* Disabled. Most likely case. Can release single packet. */
		return TRUE;
	} else {
		/*
		 * Return TRUE if number of packets in transit is less then restriction window.
		 * If TRUE caller can release single packet.
		 */
		return (TXPKTPENDTOT(wlc) < scb->restrict_txwin);
	}
}

static INLINE uint16
wlc_scb_restrict_can_ampduq(wlc_info_t *wlc, struct scb *scb, uint16 in_transit, uint16 release)
{
	if (!scb->restrict_txwin) {
		/* Disabled. Most likely case. Can release same number of packets as queried. */
		return release;
	} else if (in_transit >= scb->restrict_txwin) {
		/* Already too many packets in transit. Release denied. */
		return 0;
	} else {
		/* Return how many packets can be released. */
		return MIN(release, scb->restrict_txwin - in_transit);
	}
}

static INLINE bool
wlc_scb_restrict_can_txeval(wlc_info_t *wlc)
{
	/*
	 * Whether AMPDU txeval function can proceed or not.
	 * Prevents to release packets into txq if DATA_BLOCK_QUIET set
	 * (preparations to channel switch are in progress).
	 * Idea is in case of point-to-multipoint traffic
	 * better to have restrictions on boundary between AMPDU queue
	 * and txq, so single bad link would not affect much other good links.
	 * And to have this boundary efficient we need nothing at txq
	 * after channel switch, so control between AMPDU queue and txq
	 * would work.
	 */
	return ((wlc->block_datafifo & DATA_BLOCK_QUIET) == 0);
}

static INLINE bool
wlc_scb_restrict_do_probe(struct scb *scb)
{
	/* If restriction is not disabled yet, then frequent probing should be used. */
	return (scb->restrict_txwin != 0);
}
#else
static INLINE void wlc_scb_restrict_start(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(bsscfg);
}
static INLINE void wlc_scb_restrict_txstatus(struct scb *scb, bool success)
{
	BCM_REFERENCE(scb);
	BCM_REFERENCE(success);
}
static INLINE bool wlc_scb_restrict_can_txq(wlc_info_t *wlc, struct scb *scb)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(scb);
	return TRUE;
}
static INLINE uint16 wlc_scb_restrict_can_ampduq(wlc_info_t *wlc,
	struct scb *scb, uint16 in_transit, uint16 release)
{
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(scb);
	BCM_REFERENCE(in_transit);
	return release;
}
static INLINE bool wlc_scb_restrict_can_txeval(wlc_info_t *wlc)
{
	BCM_REFERENCE(wlc);
	return TRUE;
}
static INLINE bool wlc_scb_restrict_do_probe(struct scb *scb)
{
	BCM_REFERENCE(scb);
	return FALSE;
}
#endif /* WL_CS_RESTRICT_RELEASE */

#endif /* _wlc_cs_restrict_h_ */
