/*
 * 802.11 frame fragmentation module interface.
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
 * $Id: wlc_frag.h 568191 2015-07-01 22:15:26Z $
 */

#ifndef _wlc_frag_h_
#define _wlc_frag_h_

#include <typedefs.h>
#include <wlc_types.h>

/* attach/detach */
wlc_frag_info_t *wlc_frag_attach(wlc_info_t *wlc);
void wlc_frag_detach(wlc_frag_info_t *frag);

/* rx defrag interfacs */
void wlc_defrag_prog_reset(wlc_frag_info_t *frag, struct scb *scb, uint8 prio,
	uint16 prev_seqctl);
bool wlc_defrag_first_frag_proc(wlc_frag_info_t *frag, struct scb *scb, uint8 prio,
	struct wlc_frminfo *f);
bool wlc_defrag_other_frag_proc(wlc_frag_info_t *frag, struct scb *scb, uint8 prio,
	struct wlc_frminfo *f, uint16 prev_seqctl, bool more_frag, bool seq_chk_only);

#endif /* _wlc_frag_h_ */
