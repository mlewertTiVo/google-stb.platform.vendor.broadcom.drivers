/**
 * AP Powersave state related code
 * This file aims to encapsulating the Power save state of sbc,wlc structure.
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
 * $Id: wlc_apps.h 660987 2016-09-22 19:23:11Z $
*/


#ifndef _wlc_apps_h_
#define _wlc_apps_h_

#include <wlc_frmutil.h>

#ifdef AP

#include <wlc_pspretend.h>

/* these flags are used when exchanging messages
 * about PMQ state between BMAC and HIGH
*/
#define PS_SWITCH_OFF		0
#define PS_SWITCH_PMQ_ENTRY     0x01
#define PS_SWITCH_PMQ_SUPPR_PKT 0x02
#define PS_SWITCH_RESERVED	0x0C
#define PS_SWITCH_PMQ_PSPRETEND	0x10
#define PS_SWITCH_STA_REMOVED	0x20
#define PS_SWITCH_MAC_INVALID	0x40
#define PS_SWITCH_FIFO_FLUSHED	0x80

#define AUXPMQ_INVALID_IDX	0xFFFF

extern void wlc_apps_process_ps_switch(wlc_info_t *wlc, struct ether_addr *ea, int8 ps_on);
extern void wlc_apps_scb_ps_on(wlc_info_t *wlc, struct scb *scb, uint8 flags);
extern void wlc_apps_scb_ps_off(wlc_info_t *wlc, struct scb *scb, bool discard);
extern void wlc_apps_process_pend_ps(wlc_info_t *wlc);
extern void wlc_apps_process_pmqdata(wlc_info_t *wlc, uint32 pmqdata);
extern void wlc_apps_pspoll_resp_prepare(wlc_info_t *wlc, struct scb *scb,
                                         void *pkt, struct dot11_header *h, bool last_frag);
extern void wlc_apps_send_psp_response(wlc_info_t *wlc, struct scb *scb, uint16 fc);
#if defined(TXQ_MUX)
uint BCMFASTPATH wlc_scbq_ps_output(void *ctx, uint ac, uint request_time, struct spktq *output_q);
#endif

extern int wlc_apps_attach(wlc_info_t *wlc);
extern void wlc_apps_detach(wlc_info_t *wlc);

#if !defined(TXQ_MUX)
extern void wlc_apps_psq_ageing(wlc_info_t *wlc);
#else
#define wlc_apps_psq_ageing(wlc) do {} while (0)
#endif /* TXQ_MUX */
extern bool wlc_apps_psq(wlc_info_t *wlc, void *pkt, int prec);
extern void wlc_apps_tbtt_update(wlc_info_t *wlc);
extern bool wlc_apps_suppr_frame_enq(wlc_info_t *wlc, void *pkt, tx_status_t *txs, bool lastframe);
extern void wlc_apps_ps_prep_mpdu(wlc_info_t *wlc, void *pkt, wlc_txh_info_t *txh_info);
extern void wlc_apps_apsd_trigger(wlc_info_t *wlc, struct scb *scb, int ac);
extern void wlc_apps_apsd_prepare(wlc_info_t *wlc, struct scb *scb, void *pkt,
                                  struct dot11_header *h, bool last_frag);

extern uint8 wlc_apps_apsd_ac_available(wlc_info_t *wlc, struct scb *scb);
extern uint8 wlc_apps_apsd_ac_buffer_status(wlc_info_t *wlc, struct scb *scb);

extern void wlc_apps_scb_tx_block(wlc_info_t *wlc, struct scb *scb, uint reason, bool block);
#if !defined(TXQ_MUX)
extern void wlc_apps_scb_psq_norm(wlc_info_t *wlc, struct scb *scb);
#endif /* TXQ_MUX */
extern bool wlc_apps_scb_supr_enq(wlc_info_t *wlc, struct scb *scb, void *pkt);
extern int wlc_apps_scb_apsd_cnt(wlc_info_t *wlc, struct scb *scb);

extern void wlc_apps_process_pspretend_status(wlc_info_t *wlc, struct scb *scb,
                                              bool pps_recvd_ack);
#ifdef PROP_TXSTATUS
bool wlc_apps_pvb_upd_in_transit(wlc_info_t *wlc, struct scb *scb, uint vqdepth, bool op);
#endif

void wlc_apps_pvb_update(wlc_info_t *wlc, struct scb *scb);

extern void wlc_apps_pvb_update_now(wlc_info_t *wlc, struct scb *scb);
extern void wlc_apps_psq_norm(wlc_info_t *wlc, struct scb *scb);

#ifdef WLTDLS
extern void wlc_apps_apsd_tdls_send(wlc_info_t *wlc, struct scb *scb);
#endif

extern int  wlc_apps_psq_len(wlc_info_t *wlc, struct scb *scb);

void wlc_apps_ps_trans_upd(wlc_info_t *wlc, struct scb *scb);

void wlc_apps_set_listen_prd(wlc_info_t *wlc, struct scb *scb, uint16 listen);
uint16 wlc_apps_get_listen_prd(wlc_info_t *wlc, struct scb *scb);

#ifdef WL_AUXPMQ
extern void wlc_apps_clear_auxpmq(wlc_info_t *wlc);
#endif

#else /* AP */

#ifdef PROP_TXSTATUS
#define wlc_apps_pvb_upd_in_transit(a, b, c, d) FALSE
#endif

#define wlc_apps_pvb_update(a, b) do {} while (0)
#define wlc_apps_pvb_update_now(a, b) do {} while (0)

#define wlc_apps_attach(a) FALSE
#define wlc_apps_psq(a, b, c) FALSE
#define wlc_apps_suppr_frame_enq(a, b, c, d) FALSE

#define wlc_apps_scb_ps_on(a, b, c) do {} while (0)
#define wlc_apps_scb_ps_off(a, b, c) do {} while (0)
#define wlc_apps_process_pend_ps(a) do {} while (0)

#define wlc_apps_process_pmqdata(a, b) do {} while (0)
#define wlc_apps_pspoll_resp_prepare(a, b, c, d, e) do {} while (0)
#define wlc_apps_send_psp_response(a, b, c) do {} while (0)

#define wlc_apps_detach(a) do {} while (0)
#define wlc_apps_process_ps_switch(a, b, c) do {} while (0)
#define wlc_apps_psq_ageing(a) do {} while (0)
#define wlc_apps_tbtt_update(a) do {} while (0)
#define wlc_apps_ps_prep_mpdu(a, b, c) do {} while (0)
#define wlc_apps_apsd_trigger(a, b, c) do {} while (0)
#define wlc_apps_apsd_prepare(a, b, c, d, e) do {} while (0)
#define wlc_apps_apsd_ac_available(a, b) 0
#define wlc_apps_apsd_ac_buffer_status(a, b) 0

#define wlc_apps_scb_tx_block(a, b, c, d) do {} while (0)
#if !defined(TXQ_MUX)
#define wlc_apps_scb_psq_norm(a, b) do {} while (0)
#endif /* TXQ_MUX */
#define wlc_apps_scb_supr_enq(a, b, c) FALSE

#define wlc_apps_set_listen_prd(a, b, c) do {} while (0)
#define wlc_apps_get_listen_prd(a, b) 0
#ifdef WLTDLS
#define wlc_apps_apsd_tdls_send(a, b) do {} while (0)
#endif
#endif /* AP */

#if defined(MBSS)
extern void wlc_apps_bss_ps_off_done(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg);
extern void wlc_apps_bss_ps_on_done(wlc_info_t *wlc);
extern int wlc_apps_bcmc_ps_enqueue(wlc_info_t *wlc, struct scb *bcmc_scb, void *pkt);
#else
#define wlc_apps_bss_ps_off_done(wlc, bsscfg)
#define wlc_apps_bss_ps_on_done(wlc)
#endif /* MBSS */

#ifdef PROP_TXSTATUS
extern void wlc_apps_ps_flush_mchan(wlc_info_t *wlc, struct scb *scb);
#endif /* PROP_TXSTATUS && WLMCHAN */

#ifdef BCMDBG_TXSTUCK
extern void wlc_apps_print_scb_psinfo_txstuck(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif /* BCMDBG_TXSTUCK */

void wlc_apps_dbg_dump(wlc_info_t *wlc, int hi, int lo);

extern uint wlc_apps_scb_txpktcnt(wlc_info_t *wlc, struct scb *scb);
#if !defined(TXQ_MUX)
extern void wlc_apps_ps_flush_by_prio(wlc_info_t *wlc, struct scb *scb, int prec);
#endif /* TXQ_MUX */
#ifdef PKTQ_LOG
struct pktq * wlc_apps_prec_pktq(wlc_info_t* wlc, struct scb* scb);
#endif

struct pktq * wlc_apps_get_psq(wlc_info_t * wlc, struct scb * scb);
#endif /* _wlc_apps_h_ */
