/*
 * TXMOD (tx stack) manipulation module.
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
 * $Id: wlc_txmod.c 617211 2016-02-04 13:34:03Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_txmod.h>

/* structure to store registered functions for a txmod */
typedef struct txmod_info {
	txmod_fns_t fns;
	void *ctx;		/* Opaque handle to be passed */
} txmod_info_t;

/* wlc txmod module states */
struct wlc_txmod_info {
	wlc_info_t *wlc;
	int scbh;
	txmod_id_t txmod_last;	/* txmod registry capacity */
	txmod_info_t *txmod;	/* txmod registry (allocated along with this structure) */
};

/* access to scb cubby */
#define SCB_TXPATH_INFO(txmodi, scb) (tx_path_node_t *)SCB_CUBBY(scb, (txmodi)->scbh)

/* local declaration */
static int wlc_txmod_scb_init(void *ctx, struct scb *scb);
/* Dump the active txpath for the current SCB */
#define wlc_txmod_scb_dump NULL

/* sub-alloc sizes, only reference TXMOD_LAST at attach time */
#define TXMOD_REGSZ	TXMOD_LAST * sizeof(txmod_info_t)
#define TXPATH_REGSZ	TXMOD_LAST * sizeof(tx_path_node_t)

/* attach/detach functions */
wlc_txmod_info_t *
BCMATTACHFN(wlc_txmod_attach)(wlc_info_t *wlc)
{
	wlc_txmod_info_t *txmodi;

	/* allocate state info plus the txmod registry */
	if ((txmodi = MALLOCZ(wlc->osh, sizeof(*txmodi) + TXMOD_REGSZ)) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		          wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	txmodi->wlc = wlc;

	/* reserve the scb cubby for per scb txpath */
	if ((txmodi->scbh = wlc_scb_cubby_reserve(wlc, TXPATH_REGSZ,
			wlc_txmod_scb_init, NULL, wlc_txmod_scb_dump,
			txmodi)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* txmod registry */
	txmodi->txmod = (txmod_info_t *)&txmodi[1];
	txmodi->txmod_last = TXMOD_LAST;

	return txmodi;

fail:
	MODULE_DETACH(txmodi, wlc_txmod_detach);
	wlc_txmod_detach(txmodi);
	return NULL;
}

void
BCMATTACHFN(wlc_txmod_detach)(wlc_txmod_info_t *txmodi)
{
	wlc_info_t *wlc;

	if (txmodi == NULL)
		return;

	wlc = txmodi->wlc;
	BCM_REFERENCE(wlc);

	MFREE(wlc->osh, txmodi, sizeof(*txmodi) + TXMOD_REGSZ);
}

/* per scb txpath */
static int
wlc_txmod_scb_init(void *ctx, struct scb *scb)
{
	wlc_txmod_info_t *txmodi = (wlc_txmod_info_t *)ctx;
	tx_path_node_t *txpath = SCB_TXPATH_INFO(txmodi, scb);

	/* for faster txpath access */
	scb->tx_path = txpath;
	return BCME_OK;
}

/**
 * Give then tx_fn, return the feature id from txmod array.
 * If tx_fn is NULL, 0 will be returned
 * If entry is not found, it's an ERROR!
 */
static txmod_id_t
wlc_txmod_fid(wlc_txmod_info_t *txmodi, txmod_tx_fn_t tx_fn)
{
	txmod_id_t txmod;

	for (txmod = TXMOD_START; txmod < txmodi->txmod_last; txmod++) {
		if (tx_fn == txmodi->txmod[txmod].fns.tx_fn)
			return txmod;
	}

	/* Should not reach here */
	ASSERT(txmod < txmodi->txmod_last);
	return txmod;
}

/** return number of packets txmodule has currently; 0 if no pktcnt function */
static uint
wlc_txmod_get_pkts_pending(wlc_txmod_info_t *txmodi, txmod_id_t fid)
{
	if (txmodi->txmod[fid].fns.pktcnt_fn) {
		return txmodi->txmod[fid].fns.pktcnt_fn(txmodi->txmod[fid].ctx);
	}

	return 0;
}

/** Return total transmit packets held by the txmods */
uint
wlc_txmod_txpktcnt(wlc_txmod_info_t *txmodi)
{
	uint pktcnt = 0;
	int i;

	/* Call any pktcnt handlers of registered modules only */
	for (i = TXMOD_START; i < (int)txmodi->txmod_last; i++) {
		pktcnt += wlc_txmod_get_pkts_pending(txmodi, i);
	}

	return pktcnt;
}

/* Dump the active txpath for the current SCB */

/* Helper macro for txpath in scb */
/* A feature in Tx path goes through following states:
 * Unregisterd -> Registered [Global state]
 * Registerd -> Configured -> Active -> Configured [Per-scb state]
 */

/* Set the next feature of given feature */
#define SCB_TXMOD_SET(scb, fid, _next_fid) { \
	scb->tx_path[fid].next_tx_fn = txmodi->txmod[_next_fid].fns.tx_fn; \
	scb->tx_path[fid].next_handle = txmodi->txmod[_next_fid].ctx; \
	scb->tx_path[fid].next_fid = _next_fid; \
}

/* Numeric value designating this feature's position in tx_path */
static const uint8 txmod_pos[TXMOD_LAST] = {
	0, /* TXMOD_START */		/* First */
	8, /* TXMOD_TRANSMIT */		/* Last */
	2, /* TXMOD_TDLS */
	7, /* TXMOD_APPS */
	3, /* TXMOD_TRF_MGMT */
	4, /* TXMOD_NAR */
	5, /* TXMOD_AMSDU */
	6, /* TXMOD_AMPDU */
	1, /* TXMOD_SCBQ */
};

static const uint8 *
wlc_txmod_get_pos(void)
{
	return txmod_pos;
}

/**
 * Add a feature to the path. It should not be already on the path and should be configured
 * Does not take care of evicting anybody
 */
static void
wlc_txmod_activate(wlc_txmod_info_t *txmodi, struct scb *scb, txmod_id_t fid)
{
	const uint8 *txmod_position = wlc_txmod_get_pos();
	uint curr_mod_position;
	txmod_id_t prev, next;
	txmod_info_t curr_mod_info = txmodi->txmod[fid];

	ASSERT(SCB_TXMOD_CONFIGURED(scb, fid));
	ASSERT(!SCB_TXMOD_ACTIVE(scb, fid));

	curr_mod_position = txmod_position[fid];

	prev = TXMOD_START;

	while ((next = wlc_txmod_fid(txmodi, scb->tx_path[prev].next_tx_fn)) != 0 &&
	       txmod_position[next] < curr_mod_position)
		prev = next;

	/* next == 0 indicate this is the first addition to the path
	 * it HAS to be TXMOD_TRANSMIT as it's the one that puts the packet in
	 * txq. If this changes, then assert will need to be removed.
	 */
	ASSERT(next != 0 || fid == TXMOD_TRANSMIT);
	ASSERT(txmod_position[next] != curr_mod_position);

	SCB_TXMOD_SET(scb, prev, fid);
	SCB_TXMOD_SET(scb, fid, next);

	/* invoke any activate notify functions now that it's in the path */
	if (curr_mod_info.fns.activate_notify_fn)
		curr_mod_info.fns.activate_notify_fn(curr_mod_info.ctx, scb);
}

/**
 * Remove a fid from the path. It should be already on the path
 * Does not take care of replacing it with any other feature.
 */
static void
wlc_txmod_deactivate(wlc_txmod_info_t *txmodi, struct scb *scb, txmod_id_t fid)
{
	txmod_id_t prev, next;
	txmod_info_t curr_mod_info = txmodi->txmod[fid];

	/* If not active, do nothing */
	if (!SCB_TXMOD_ACTIVE(scb, fid))
		return;

	/* if deactivate notify function is present, call it */
	if (curr_mod_info.fns.deactivate_notify_fn)
		curr_mod_info.fns.deactivate_notify_fn(curr_mod_info.ctx, scb);

	prev = TXMOD_START;

	while ((next = wlc_txmod_fid(txmodi, scb->tx_path[prev].next_tx_fn)) != fid)
		prev = next;

	SCB_TXMOD_SET(scb, prev, wlc_txmod_fid(txmodi, scb->tx_path[fid].next_tx_fn));
	scb->tx_path[fid].next_tx_fn = NULL;
}

/* Register the function to handle this feature */
void
BCMATTACHFN(wlc_txmod_fn_register)(wlc_txmod_info_t *txmodi, txmod_id_t feature_id,
	void *ctx, txmod_fns_t fns)
{
	ASSERT(feature_id < txmodi->txmod_last);
	/* tx_fn can't be NULL */
	ASSERT(fns.tx_fn != NULL && fns.pktcnt_fn != NULL);

	txmodi->txmod[feature_id].fns = fns;
	txmodi->txmod[feature_id].ctx = ctx;
}

/* Add the fid to handle packets for this SCB, if allowed */
void
wlc_txmod_config(wlc_txmod_info_t *txmodi, struct scb *scb, txmod_id_t fid)
{
	ASSERT(fid < txmodi->txmod_last);

	/* Don't do anything if not yet registered or
	 * already configured
	 */
	if ((txmodi->txmod[fid].fns.tx_fn == NULL) ||
	    (SCB_TXMOD_CONFIGURED(scb, fid)))
		return;

	/* Indicate that the feature is configured */
	scb->tx_path[fid].configured = TRUE;

	ASSERT(!SCB_TXMOD_ACTIVE(scb, fid));

	wlc_txmod_activate(txmodi, scb, fid);
}

/* Remove the feature to handle packets for this SCB.
 * If just configured but not in path, just marked unconfigured
 * If in path, feature is removed and, if applicable, replaced by any other feature
 */
void
wlc_txmod_unconfig(wlc_txmod_info_t *txmodi, struct scb *scb, txmod_id_t fid)
{
	ASSERT(fid < txmodi->txmod_last);
	ASSERT(fid != TXMOD_TRANSMIT);

	if (!SCB_TXMOD_CONFIGURED(scb, fid))
		return;

	scb->tx_path[fid].configured = FALSE;

	/* Nothing to do if not active */
	if (!SCB_TXMOD_ACTIVE(scb, fid))
		return;

	wlc_txmod_deactivate(txmodi, scb, fid);
}
