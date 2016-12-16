/*
 * MAC debug and print functions
 * Broadcom 802.11bang Networking Device Driver
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
 * $Id: wlc_macdbg.h 659617 2016-09-15 05:45:17Z $
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
#define PSM_FATAL_WAKE		3
#define PSM_FATAL_TXSUFL	4
#define PSM_FATAL_PSMXWD	5
#define PSM_FATAL_TXSTUCK	6
#define PSM_FATAL_LAST		7

#define PSMX_FATAL_ANY		0
#define PSMX_FATAL_PSMWD	1
#define PSMX_FATAL_SUSP		2
#define PSMX_FATAL_TXSTUCK	3
#define PSMX_FATAL_LAST		4

/* attach/detach */
extern wlc_macdbg_info_t *wlc_macdbg_attach(wlc_info_t *wlc);
extern void wlc_macdbg_detach(wlc_macdbg_info_t *macdbg);

#define wlc_macdbg_sendup_d11regs(a) do {} while (0)

extern void wlc_dump_ucode_fatal(wlc_info_t *wlc, uint reason);

/* catch any interrupts from psmx */
#ifdef WL_PSMX
void wlc_bmac_psmx_errors(wlc_info_t *wlc);
void wlc_dump_psmx_fatal(wlc_info_t *wlc, uint reason);
#ifdef WLVASIP
void wlc_dump_vasip_fatal(wlc_info_t *wlc);
#endif	/* WLVASIP */
#else
#define wlc_bmac_psmx_errors(wlc) do {} while (0)
#endif /* WL_PSMX */

extern void wlc_dump_mac_fatal(wlc_info_t *wlc, uint reason);


#if defined(BCMDBG) || defined(BCMDBG_DUMP)|| defined(PHYTXERR_DUMP)
extern void wlc_dump_phytxerr(wlc_info_t *wlc, uint16 PhyErr);
#endif /* BCMDBG || BCMDBG_DUMP  || PHYTXERR_DUMP */

/* Uncomment to enable frameid trace facility: */
/* #define WLC_MACDBG_FRAMEID_TRACE */

#ifdef WLC_MACDBG_FRAMEID_TRACE
void wlc_macdbg_frameid_trace_pkt(wlc_macdbg_info_t *macdbg, void *pkt,
	uint8 fifo, uint16 txFrameID, uint8 epoch);
void wlc_macdbg_frameid_trace_txs(wlc_macdbg_info_t *macdbg, void *pkt, tx_status_t *txs);
void wlc_macdbg_frameid_trace_sync(wlc_macdbg_info_t *macdbg, void *pkt);
void wlc_macdbg_frameid_trace_dump(wlc_macdbg_info_t *macdbg);
#endif

#endif /* WLC_MACDBG_H_ */
