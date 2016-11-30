/*
 * Per-BSS vendor IE list management interface.
 * Used to manage the user plumbed IEs in the BSS.
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
 * <<Broadcom-WL-IPTag/Proprietary:.*>>
 *
 * $Id: wlc_bsscfg_viel.h 532292 2015-02-05 13:59:45Z $
 */

#ifndef _wlc_bsscfg_viel_h_
#define _wlc_bsscfg_viel_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_vndr_ie_list.h>

/* attach/detach */

wlc_bsscfg_viel_info_t *wlc_bsscfg_viel_attach(wlc_info_t *wlc);
void wlc_bsscfg_viel_detach(wlc_bsscfg_viel_info_t *vieli);

/* APIs */

int wlc_vndr_ie_getlen_ext(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	vndr_ie_list_filter_fn_t filter, uint32 pktflag, int *totie);
#define wlc_vndr_ie_getlen(vieli, cfg, pktflag, totie) \
	wlc_vndr_ie_getlen_ext(vieli, cfg, NULL, pktflag, totie)
uint8 *wlc_vndr_ie_write_ext(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	vndr_ie_list_write_filter_fn_t filter, uint type, uint8 *cp, int buflen,
	uint32 pktflag);
#define wlc_vndr_ie_write(vieli, cfg, cp, buflen, pktflag) \
	wlc_vndr_ie_write_ext(vieli, cfg, NULL, -1, cp, buflen, pktflag)

vndr_ie_listel_t *wlc_vndr_ie_add_elem(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	uint32 pktflag,	vndr_ie_t *vndr_iep);
vndr_ie_listel_t *wlc_vndr_ie_mod_elem(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	vndr_ie_listel_t *old_listel, uint32 pktflag, vndr_ie_t *vndr_iep);

int wlc_vndr_ie_add(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	const vndr_ie_buf_t *ie_buf, int len);
int wlc_vndr_ie_del(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	const vndr_ie_buf_t *ie_buf, int len);
int wlc_vndr_ie_get(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	vndr_ie_buf_t *ie_buf, int len, uint32 pktflag);

int wlc_vndr_ie_mod_elem_by_type(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	uint8 type, uint32 pktflag, vndr_ie_t *vndr_iep);
int wlc_vndr_ie_del_by_type(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	uint8 type);
uint8 *wlc_vndr_ie_find_by_type(wlc_bsscfg_viel_info_t *vieli, wlc_bsscfg_t *cfg,
	uint8 type);

#endif	/* _wlc_bsscfg_viel_h_ */
