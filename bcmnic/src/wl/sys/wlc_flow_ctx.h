/*
 * wlc_flow_ctx.h
 *
 * Common interface for TX/RX Flow Context Table module
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
 * $Id$
 *
 */
#ifndef _wlc_flow_ctx_h
#define _wlc_flow_ctx_h

#include <wlc_types.h>

typedef struct {
	uint16 ref_cnt;
	const scb_t *scb;
	const wlc_bsscfg_t *bsscfg;
} flow_ctx_t;

extern wlc_flow_ctx_info_t * wlc_flow_ctx_attach(wlc_info_t *wlc);
extern void wlc_flow_ctx_detach(wlc_flow_ctx_info_t *flow_tbli);


/*
 * Add a TX/RX context to flow table (returning flowID).
 *
 * Inputs:
 *	flow_tbli: flow context table module handle (i.e. returned from wlc_flow_ctx_attach)
 *	ctx      : user allocated structure to copy to the flow table
 * Output:
 *	flowID or BCME_NORESOURCE if table is full
 */
extern int wlc_flow_ctx_add_context(wlc_flow_ctx_info_t *flow_tbli, const flow_ctx_t *ctx);

/*
 * Delete a TX/RX context from flow table (given flowID).
 * Note: the flow will not be deleted if there is one or more packet reference to it
 *
 * Inputs:
 *	flow_tbli: flow context table module handle (i.e. returned from wlc_flow_ctx_attach)
 *	flowID   : the ID of the flow to delete from the flow table
 * Output:
 *	BCME_OK or flowID BCME_NOTFOUND or BCME_NOTREADY if there is still a reference to it
 */
extern int wlc_flow_ctx_del_context(wlc_flow_ctx_info_t *flow_tbli, uint16 flowID);

/*
 * Add packets to an existing TX/RX flow context
 *
 * Inputs:
 *	flow_tbli  : flow context table module handle (i.e. returned from wlc_flow_ctx_attach)
 *	flowID     : the ID of the flow to add packet references to in the flow context table
 *	num_packets: the number of packets that refer to the given flowID
 * Output:
 *	BCME_OK or flowID BCME_NOTFOUND
 */
extern int wlc_flow_ctx_add_packet_refs(wlc_flow_ctx_info_t *flow_tbli, uint16 flowID,
                                        uint8 num_packets);

/*
 * Remove packets of an existing TX/RX flow context
 *
 * Inputs:
 *	flow_tbli  : flow context table module handle (i.e. returned from wlc_flow_ctx_attach)
 *	flowID     : the ID of the flow to remove packet references from the flow context table
 *	num_packets: the number of packets that refer to the given flowID
 * Output:
 *	BCME_OK or flowID BCME_NOTFOUND
 */
extern int wlc_flow_ctx_remove_packet_refs(wlc_flow_ctx_info_t *flow_tbli, uint16 flowID,
                                           uint8 num_packets);

/*
 * Update a TX/RX context from flow table (given flowID).
 * Note: the user will need to inform the packet classification module about this flow change
 *
 * Inputs:
 *	flow_tbli: flow context table module handle (i.e. returned from wlc_flow_ctx_attach)
 *	flowID   : the ID of the flow to update
 *	ctx      : user allocated structure to copy to the flow table, based on flow->flowID
 * Output:
 *	BCME_OK or flow->flowID BCME_NOTFOUND
 */
extern int wlc_flow_ctx_update(const wlc_flow_ctx_info_t *flow_tbli, uint16 flowID,
                               const flow_ctx_t *ctx);

/*
 * Lookup a TX/RX context to flow table (given flowID).
 *
 * Inputs:
 *	flow_tbli: flow context table module handle (i.e. returned from wlc_flow_ctx_attach)
 *	flowID   : the ID of the flow to find
 * Output:
 *	NULL, or pointer to found entry
 */
extern const flow_ctx_t *wlc_flow_ctx_lookup(const wlc_flow_ctx_info_t *flow_tbli, uint16 flowID);

#endif /* _wlc_flow_ctx_h */
