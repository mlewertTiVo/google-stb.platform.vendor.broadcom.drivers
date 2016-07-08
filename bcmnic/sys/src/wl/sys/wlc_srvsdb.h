/*
 * SRVSDB feature interface.
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
 * $Id: wlc_srvsdb.h 612761 2016-01-14 23:06:00Z $
 */

#ifndef _wlc_srvsdb_h_
#define _wlc_srvsdb_h_

#include <typedefs.h>
#include <bcmwifi_channels.h>
#include <wlc_types.h>

#if defined(WL11N)
void wlc_srvsdb_stf_ss_algo_chan_get(wlc_info_t *wlc, chanspec_t chanspec);
#endif

void wlc_srvsdb_reset_engine(wlc_info_t *wlc);
void wlc_srvsdb_switch_ppr(wlc_info_t *wlc, chanspec_t new_chanspec, bool last_chan_saved,
	bool switched);

bool wlc_srvsdb_save_valid(wlc_info_t *wlc, chanspec_t chanspec);
void wlc_srvsdb_set_chanspec(wlc_info_t *wlc, chanspec_t cur, chanspec_t next);

wlc_srvsdb_info_t *wlc_srvsdb_attach(wlc_info_t *wlc);
void wlc_srvsdb_detach(wlc_srvsdb_info_t *srvsdb);

#endif /* _wlc_srvsdb_h_ */
