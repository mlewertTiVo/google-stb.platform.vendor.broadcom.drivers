/*
 * txstatus related routines
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
 * $Id: wlc_txs.h 596126 2015-10-29 19:53:48Z $
 */

#ifndef __wlc_txstatus_h__
#define __wlc_txstatus_h__

#include <typedefs.h>
#include <wlc_types.h>
#include <d11.h>

void wlc_pkttag_scb_restore(void *ctxt, void* p);

bool wlc_dotxstatus(wlc_info_t *wlc, tx_status_t *txs, uint32 frm_tx2);

uint16 wlc_txs_alias_to_old_fmt(wlc_info_t *wlc, tx_status_macinfo_t* status);
bool wlc_should_retry_suppressed_pkt(wlc_info_t *wlc, void *p, uint status);
void wlc_print_txstatus(wlc_info_t *wlc, tx_status_t* txs);

#endif /* __wlc_txstatus_h__ */
