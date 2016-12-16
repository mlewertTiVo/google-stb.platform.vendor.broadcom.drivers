/*
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
 * $Id: wlc_scbq.h 665067 2016-10-14 20:09:59Z $
 */
/**
 * @file
 * @brief Per-SCB Tx Queuing modulue for Broadcom 802.11 Networking Driver
 */

#ifndef _wlc_scbq_h_
#define _wlc_scbq_h_

#include <typedefs.h>
#include "wlc_types.h"
#include "wlc_mux.h"
#include "wlc_txtime.h"

/**
 * @brief State structure for the SCBQ module created by wlc_scbq_module_attach()
 */
typedef struct wlc_scbq_info wlc_scbq_info_t;

/* Flow Control stop flags for an SCB */
typedef enum scbq_stop_flag {
	SCBQ_FC_BLOCK_DATA     = 0x0001,
	SCBQ_FC_ASSOC          = 0x0002,
	SCBQ_FC_PS_DRAIN       = 0x0004,
	SCBQ_FC_PS             = 0x0008
} scbq_stop_flag_t;

#define TXOUT_APPS 1
void wlc_txoutput_config(wlc_scbq_info_t *scbq_info, struct scb *scb, uint8 module_id);
void wlc_txoutput_unconfig(wlc_scbq_info_t *scbq_info, struct scb *scb, uint8 module_id);


/*
 * Module Attach/Detach
 */

/**
 * @brief Allocate and initialize the SCBQ module.
 */
wlc_scbq_info_t *wlc_scbq_module_attach(wlc_info_t *wlc);

/**
 * @brief Free all resources of the SCBQ module
 */
void wlc_scbq_module_detach(wlc_scbq_info_t *scbq_info);

/**
 * @brief Update the output fn an SCBQ
 */
void wlc_scbq_set_output_fn(wlc_scbq_info_t *scbq_info, struct scb *scb,
                            void *ctx, mux_output_fn_t output_fn);

/**
 * @brief Reset the output fn for an SCBQ to the default output fn
 */
void wlc_scbq_reset_output_fn(wlc_scbq_info_t *scbq_info, struct scb *scb);

/**
 * @breif Call the configured output fn (upstream of an output filter) for the SCBQ
 */
uint wlc_scbq_unfiltered_output(wlc_scbq_info_t *scbq_info, struct scb *scb,
                                uint ac, uint request_time, struct spktq *output_q);

/**
 * @brief Return the given scb's tx queue
 */
struct pktq* wlc_scbq_txq(wlc_scbq_info_t *scbq_info, struct scb *scb);

/**
 * @brief Move muxsources to a new queue
 */
int wlc_scbq_mux_move(wlc_scbq_info_t *scbq_info, struct scb *scb, struct wlc_txq_info *new_qi);

/**
 * @brief Delete mux sources of a given scb
 */
void wlc_scbq_muxdelsrc(wlc_scbq_info_t *scbq_info, struct scb *scb);

/**
 * @brief Recreate mux sources of a given scb
 */
int wlc_scbq_muxrecreatesrc(wlc_scbq_info_t *scbq_info,
		struct scb *scb, struct wlc_txq_info *new_qi);

/**
 * @brief Set a Stop Flag to prevent tx output from all SCBs
 */
void wlc_scbq_global_stop_flag_set(wlc_scbq_info_t *scbq_info, scbq_stop_flag_t flag);

/**
 * @brief Clear a Stop Flag preventing tx output from all SCBs
 */
void wlc_scbq_global_stop_flag_clear(wlc_scbq_info_t *scbq_info, scbq_stop_flag_t flag);

/**
 * @brief Set a Stop Flag to prevent tx output from the given scb
 */
void wlc_scbq_scb_stop_flag_set(wlc_scbq_info_t *scbq_info, struct scb *scb, scbq_stop_flag_t flag);

/**
 * @brief Clear a Stop Flag preventing tx output from the given scb
 */
void wlc_scbq_scb_stop_flag_clear(wlc_scbq_info_t *scbq_info, struct scb *scb,
                                  scbq_stop_flag_t flag);

/**
 * @brief Report on the stalled state for the scbq's MUX Source for the specified AC.
 */
bool wlc_scbq_scb_stalled(wlc_scbq_info_t *scbq_info, struct scb *scb, uint ac);

/**
 * @brief Update the scbq state to indicate that the mux source is stalled.
 */
void wlc_scbq_scb_stall_set(wlc_scbq_info_t *scbq_info, struct scb *scb, uint ac);

/**
 * @brief Clear a mux source stall condition for the given scb
 */
void wlc_scbq_scb_stall_clear(wlc_scbq_info_t *scbq_info, struct scb *scb, uint ac);

/**
 * @brief Calculate a TX airtime estimate using a previously initialized timecalc_t struct
 */
uint wlc_scbq_timecalc(timecalc_t *tc, uint mpdu_len);

/**
 * @brief Packet drop policy handler
 */
typedef void (*scbq_overflow_fn_t)(wlc_scbq_info_t *scbq_info,
	struct scb *scb, struct pktq *q, void *pkt, uint prec);

/**
 *  Set the packet overflow handler for SCBQ
 */
void wlc_scbq_register_overflow_fn(wlc_scbq_info_t *scbq_info,
	struct scb *scb, scbq_overflow_fn_t overflow_fn);

#endif /* _wlc_scbq_h_ */
