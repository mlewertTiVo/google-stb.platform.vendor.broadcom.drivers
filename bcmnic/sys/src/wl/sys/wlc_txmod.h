/*
 * TXMOD (tx stack) manipulation module interface.
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
 * $Id: wlc_txmod.h 550528 2015-04-21 00:12:58Z $
 */

#ifndef _wlc_txmod_h_
#define _wlc_txmod_h_

#include <typedefs.h>
#include <wlc_types.h>

/* txmod attach/detach */

wlc_txmod_info_t *wlc_txmod_attach(wlc_info_t *wlc);
void wlc_txmod_detach(wlc_txmod_info_t *txmodi);

/* txmod registration */

/* Allowed txpath features */
typedef enum txmod_id {
	TXMOD_START = 0,	/* Node to designate start of the tx path */
	TXMOD_TRANSMIT = 1,	/* Node to designate enqueue to common tx queue */
	TXMOD_TDLS = 2,
	TXMOD_APPS = 3,
	TXMOD_TRF_MGMT = 4,
	TXMOD_NAR = 5,
	TXMOD_AMSDU = 6,
	TXMOD_AMPDU = 7,
	TXMOD_SCBQ = 8,
	TXMOD_AIBSS = 9,
/* !!!Add new txmod ID above and update array 'txmod_pos' with the position of the txmod!!! */
	TXMOD_LAST
} txmod_id_t;

/* Function that does transmit packet feature processing. prec is precedence of the packet */
typedef void (*txmod_tx_fn_t)(void *ctx, struct scb *scb, void *pkt, uint prec);

/* Callback for txmod when it gets deactivated by other txmod */
typedef void (*txmod_deactivate_fn_t)(void *ctx, struct scb *scb);

/* Callback for txmod when it gets activated */
typedef void (*txmod_activate_fn_t)(void *ctx, struct scb *scb);

/* Callback for txmod to return packets held by this txmod */
typedef uint (*txmod_pktcnt_fn_t)(void *ctx);

/* Function vector to make it easy to initialize txmod
 * Note: txmod_info itself is not modified to avoid adding one more level of indirection
 * during transmit of a packet
 */
typedef struct txmod_fns {
	txmod_tx_fn_t		tx_fn;			/* Process the packet */
	txmod_pktcnt_fn_t	pktcnt_fn;		/* Return the packet count */
	txmod_deactivate_fn_t	deactivate_notify_fn;	/* Handle the deactivation of the feature */
	txmod_activate_fn_t	activate_notify_fn;	/* Handle the activation of the feature */
} txmod_fns_t;

/* Register a txmod handlers with txmod module */
void wlc_txmod_fn_register(wlc_txmod_info_t *txmodi, txmod_id_t fid, void *ctx, txmod_fns_t fns);

/* Insert/remove a txmod at/from the fixed loction in the scb txpath */
void wlc_txmod_config(wlc_txmod_info_t *txmodi, struct scb *scb, txmod_id_t fid);
void wlc_txmod_unconfig(wlc_txmod_info_t *txmodi, struct scb *scb, txmod_id_t fid);

/* fast txpath access in scb */

/* per scb txpath node */
struct tx_path_node {
	txmod_tx_fn_t next_tx_fn;	/* Next function to be executed */
	void *next_handle;
	uint8 next_fid;			/* Next fid in transmit path */
	bool configured;		/* Whether this feature is configured */
};

/* Given the 'feature', invoke the next stage of transmission in tx path */
#define SCB_TX_NEXT(fid, scb, pkt, prec) \
	scb->tx_path[fid].next_tx_fn(scb->tx_path[fid].next_handle, scb, pkt, prec)


/* utilities */
uint wlc_txmod_txpktcnt(wlc_txmod_info_t *txmodi);

/* KEEP THE FOLLOWING FOR NOW. THE REFERENCE TO THESE SHOULD BE ELIMINATED FIRST
 * AND THEN THESE SHOULD BE MOVED BACK TO THE .c FILE SINCE NO ONE SHOULD PEEK INTO
 * THE TXMOD ORDER I.E. ALL A TXMOD SHOULD IS TO PASS THE PACKET TO THE NEXT TXMOD.
 */
/* Is the feature currently in the path to handle transmit. ACTIVE implies CONFIGURED */
#define SCB_TXMOD_ACTIVE(scb, fid) (scb->tx_path[fid].next_tx_fn != NULL)
/* Next feature configured */
#define SCB_TXMOD_NEXT_FID(scb, fid) (scb->tx_path[fid].next_fid)
/* Is the feature configured? */
#define SCB_TXMOD_CONFIGURED(scb, fid) (scb->tx_path[fid].configured)

#endif /* _wlc_txmod_h_ */
