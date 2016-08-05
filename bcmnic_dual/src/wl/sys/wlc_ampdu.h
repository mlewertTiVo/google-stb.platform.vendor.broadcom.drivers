/*
 * A-MPDU Tx (with extended Block Ack) related header file
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_ampdu.h 476261 2014-05-08 09:03:21Z $
*/


#ifndef _wlc_ampdu_tx_h_
#define _wlc_ampdu_tx_h_

#ifdef WLTAF
#include <wlc_taf.h>
#else
typedef void taf_scheduler_public_t;
#endif /* WLTAF */

typedef struct scb_ampdu_tx scb_ampdu_tx_t;
typedef struct scb_ampdu_tid_ini scb_ampdu_tid_ini_t;

extern ampdu_tx_info_t *wlc_ampdu_tx_attach(wlc_info_t *wlc);
extern void wlc_ampdu_tx_detach(ampdu_tx_info_t *ampdu_tx);
extern int wlc_sendampdu(ampdu_tx_info_t *ampdu_tx, wlc_txq_info_t *qi, void **aggp, int prec);
extern bool wlc_ampdu_dotxstatus(ampdu_tx_info_t *ampdu_tx, struct scb *scb, void *p,
	tx_status_t *txs, wlc_txh_info_t *txh_info);
extern void wlc_ampdu_dotxstatus_regmpdu(ampdu_tx_info_t *ampdu_tx, struct scb *scb, void *p,
	tx_status_t *txs);
extern void wlc_ampdu_tx_reset(ampdu_tx_info_t *ampdu_tx);
extern void wlc_ampdu_macaddr_upd(wlc_info_t *wlc);

extern uint8 wlc_ampdu_null_delim_cnt(ampdu_tx_info_t *ampdu_tx, struct scb *scb,
	ratespec_t rspec, int phylen, uint16* minbytes);
extern bool wlc_ampdu_frameburst_override(ampdu_tx_info_t *ampdu_tx);
#ifdef WLC_HIGH_ONLY
extern void wlc_ampdu_txstatus_complete(ampdu_tx_info_t *ampdu_tx, uint32 s1, uint32 s2);
#endif

extern void wlc_ampdu_agg_state_update_tx(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, bool txaggr);
extern void wlc_ampdu_ini_adjust(ampdu_tx_info_t *ampdu_tx, struct scb *scb, void *p);

extern bool wlc_ampdu_tx_cap(ampdu_tx_info_t *ampdu_tx);
extern int wlc_ampdumac_set(ampdu_tx_info_t *ampdu_tx, uint8 on);

#ifdef WLAMPDU_MAC
#define AMU_EPOCH_NO_CHANGE		-1	/* no-epoch change but change params */
#define AMU_EPOCH_CHG_PLCP		0	/* epoch change due to plcp */
#define AMU_EPOCH_CHG_FID		1	/* epoch change due to rate flag in frameid */
#define AMU_EPOCH_CHG_NAGG		2	/* epoch change due to ampdu off */
#define AMU_EPOCH_CHG_MPDU		3	/* epoch change due to mpdu */
#define AMU_EPOCH_CHG_DSTTID		4	/* epoch change due to dst/tid */
#define AMU_EPOCH_CHG_SEQ		5	/* epoch change due to discontinuous seq no */
#define AMU_EPOCH_CHG_TXC_UPD  6    /* Epoch change due to txcache update  */
#define AMU_EPOCH_CHG_TXHDR    7    /* Epoch change due to Long hdr to short hdr transition */

extern void wlc_ampdu_change_epoch(ampdu_tx_info_t *ampdu_tx, int fifo, int reason_code);
extern uint8 wlc_ampdu_chgnsav_epoch(ampdu_tx_info_t *, int fifo,
	int reason_code, struct scb *, uint8 tid, wlc_txh_info_t*);
extern bool wlc_ampdu_was_ampdu(ampdu_tx_info_t *, int fifo);
extern int wlc_dump_aggfifo(wlc_info_t *wlc, struct bcmstrbuf *b);
extern void wlc_ampdu_fill_percache_info(ampdu_tx_info_t *ampdu_tx, struct scb *scb, uint8 tid,
	d11actxh_t *txh);
#endif /* WLAMPDU_MAC */

#define TXFS_WSZ_AC_BE	32
#define TXFS_WSZ_AC_BK	10
#define TXFS_WSZ_AC_VI	4
#define TXFS_WSZ_AC_VO	4

extern void wlc_sidechannel_init(ampdu_tx_info_t *ampdu_tx);


extern void ampdu_cleanup_tid_ini(ampdu_tx_info_t *ampdu_tx, struct scb *scb,
	uint8 tid, bool force);

extern void scb_ampdu_tx_flush(ampdu_tx_info_t *ampdu_tx, struct scb *scb);

extern void BCMATTACHFN(wlc_ampdu_agg_state_txaggr_override)(ampdu_tx_info_t *ampdu_tx,
	int8 txaggr);

extern void wlc_ampdu_clear_tx_dump(ampdu_tx_info_t *ampdu_tx);

extern void wlc_ampdu_recv_ba(ampdu_tx_info_t *ampdu_tx, struct scb *scb, uint8 *body,
	int body_len);
extern void wlc_ampdu_recv_addba_req_ini(ampdu_tx_info_t *ampdu_tx, struct scb *scb,
	dot11_addba_req_t *addba_req, int body_len);
extern void wlc_ampdu_recv_addba_resp(ampdu_tx_info_t *ampdu_tx, struct scb *scb,
	uint8 *body, int body_len);

#if defined(BCMDBG_AMPDU)
extern int wlc_ampdu_tx_dump(ampdu_tx_info_t *ampdu_tx, struct bcmstrbuf *b);
#endif 

extern void wlc_ampdu_tx_set_mpdu_density(ampdu_tx_info_t *ampdu_tx, uint8 mpdu_density);
extern void wlc_ampdu_tx_set_ba_tx_wsize(ampdu_tx_info_t *ampdu_tx, uint8 wsize);
extern uint8 wlc_ampdu_tx_get_ba_tx_wsize(ampdu_tx_info_t *ampdu_tx);
extern uint8 wlc_ampdu_tx_get_ba_max_tx_wsize(ampdu_tx_info_t *ampdu_tx);
extern void wlc_ampdu_tx_recv_delba(ampdu_tx_info_t *ampdu_tx, struct scb *scb, uint8 tid,
	uint8 category, uint16 initiator, uint16 reason);
extern void wlc_ampdu_tx_send_delba(ampdu_tx_info_t *ampdu_tx, struct scb *scb, uint8 tid,
	uint16 initiator, uint16 reason);

extern int wlc_ampdu_tx_set(ampdu_tx_info_t *ampdu_tx, bool on);

extern uint wlc_ampdu_tx_get_tcp_ack_ratio(ampdu_tx_info_t *ampdu_tx);


#if defined(WLNAR)
extern uint8 BCMFASTPATH wlc_ampdu_ba_on_tidmask(const struct scb *scb);
#endif


#ifdef WLTAF
extern void * BCMFASTPATH
wlc_ampdu_get_taf_scb_info(ampdu_tx_info_t *ampdu_tx, struct scb* scb);
extern void * BCMFASTPATH
wlc_ampdu_get_taf_scb_tid_info(scb_ampdu_tx_t *scb_ampdu, int tid);
extern uint16 BCMFASTPATH
wlc_ampdu_get_taf_scb_tid_pktlen(scb_ampdu_tx_t *scb_ampdu, scb_ampdu_tid_ini_t *ini);
extern uint8 BCMFASTPATH
wlc_ampdu_get_taf_scb_tid_rel(scb_ampdu_tx_t *scb_ampdu);
extern uint8 BCMFASTPATH
wlc_ampdu_get_taf_txq_fullness_pct(scb_ampdu_tx_t *scb_ampdu, scb_ampdu_tid_ini_t *ini);
#endif /* WLTAF */

extern bool BCMFASTPATH
wlc_ampdu_txeval_action(ampdu_tx_info_t *ampdu_tx, scb_ampdu_tx_t *scb_ampdu,
                        scb_ampdu_tid_ini_t* ini, bool force, taf_scheduler_public_t* taf);

extern void wlc_wlfc_flush_queue(wlc_info_t *wlc, struct pktq *q);

extern struct pktq* wlc_ampdu_txq(ampdu_tx_info_t *ampdu, struct scb *scb);
#ifdef PROP_TXSTATUS
extern void wlc_ampdu_send_bar_cfg(ampdu_tx_info_t * ampdu, struct scb *scb);
extern void wlc_ampdu_flush_ampdu_q(ampdu_tx_info_t *ampdu, wlc_bsscfg_t *cfg);
extern void wlc_ampdu_flush_pkts(wlc_info_t *wlc, struct scb *scb, uint8 tid);
#endif /* PROP_TXSTATUS */
#ifdef WLAWDL
extern void wlc_awdl_ampdu_txeval(void *hdl, bool awdlorbss);
#endif

#ifdef WLATF
extern void wlc_ampdu_atf_set_default_mode(ampdu_tx_info_t *ampdu_tx,
	scb_module_t *scbstate, uint32 mode);
extern void wlc_ampdu_atf_rate_override(wlc_info_t *, ratespec_t, wlcband_t *);
extern void wlc_ampdu_atf_scb_rate_override(ampdu_tx_info_t *, struct scb *, ratespec_t);
#endif /* WLATF */

extern void wlc_check_ampdu_fc(ampdu_tx_info_t *ampdu, struct scb *scb);

extern void wlc_ampdu_txeval_all(wlc_info_t *wlc);

#endif /* _wlc_ampdu_tx_h_ */
