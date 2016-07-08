/*
 * MAC debug and print functions
 * Broadcom 802.11bang Networking Device Driver
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
 * $Id: wlc_macdbg.h 599153 2015-11-12 22:27:26Z $
 */
#ifndef WLC_MACDBG_H_
#define WLC_MACDBG_H_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlioctl.h>

/* fatal reason code */
#define PSM_FATAL_ANY		0
#define PSM_FATAL_PSMWD		1
#define PSM_FATAL_SUSP		2
#define PSM_FATAL_LAST		3

/* attach/detach */
extern wlc_macdbg_info_t *wlc_macdbg_attach(wlc_info_t *wlc);
extern void wlc_macdbg_detach(wlc_macdbg_info_t *macdbg);

#define wlc_macdbg_sendup_d11regs(a) do {} while (0)


extern void wlc_dump_ucode_fatal(wlc_info_t *wlc, uint reason);

/* Uncomment to enable frameid trace facility: */
/* #define WLC_MACDBG_FRAMEID_TRACE */

#ifdef WLC_MACDBG_FRAMEID_TRACE
void wlc_macdbg_frameid_trace_pkt(wlc_macdbg_info_t *macdbg, void *pkt, wlc_txh_info_t *txh,
	uint8 fifo);
void wlc_macdbg_frameid_trace_txs(wlc_macdbg_info_t *macdbg, void *pkt, tx_status_t *txs);
void wlc_macdbg_frameid_trace_sync(wlc_macdbg_info_t *macdbg, void *pkt);
void wlc_macdbg_frameid_trace_dump(wlc_macdbg_info_t *macdbg);
#endif

#endif /* WLC_MACDBG_H_ */
