/**
 * PS Pretend feature interface.
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
 * $Id: wlc_pspretend.h 636000 2016-05-06 04:05:27Z $
 */

#ifndef _wlc_pspretend_h_
#define _wlc_pspretend_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>

#ifdef PSPRETEND

/* per bss enable */
#define PS_PRETEND_ENABLED(cfg)            (((cfg)->ps_pretend_retry_limit) > 0)
#define PS_PRETEND_THRESHOLD_ENABLED(cfg)  (((cfg)->ps_pretend_threshold) > 0)

/* per scb enable */
/* bit flags for (uint8) scb.PS_pretend */
#define PS_PRETEND_NOT_ACTIVE    0

/** PS_PRETEND_PROBING states to do probing to the scb */
#define PS_PRETEND_PROBING       (1 << 0)

/** PS_PRETEND_ACTIVE indicates that ps pretend is currently active */
#define	PS_PRETEND_ACTIVE        (1 << 1)

/** PS_PRETEND_ACTIVE_PMQ indicates that we have had a PPS PMQ entry */
#define	PS_PRETEND_ACTIVE_PMQ    (1 << 2)

/***
 * PS_PRETEND_NO_BLOCK states that we should not expect to see a PPS PMQ entry, hence, not to block
 * ourselves waiting to get one.
 */
#define PS_PRETEND_NO_BLOCK      (1 << 3)

/** PS_PRETEND_PREVENT states to not do normal ps pretend for a scb */
#define PS_PRETEND_PREVENT       (1 << 4)

/** PS_PRETEND_RECENT indicates a ps pretend was triggered recently */
#define PS_PRETEND_RECENT        (1 << 5)

/**
 * PS_PRETEND_THRESHOLD indicates that the successive failed TX status count has exceeded the
 * threshold
 */
#define PS_PRETEND_THRESHOLD     (1 << 6)

/***
 * PS_PRETEND_ON is a bit mask of all active states that is used to clear the scb state when ps
 * pretend exits
 */
#define PS_PRETEND_ON	(PS_PRETEND_ACTIVE | PS_PRETEND_PROBING | \
			 PS_PRETEND_ACTIVE_PMQ | PS_PRETEND_THRESHOLD)

#define	SCB_PS_PRETEND(scb)		((scb) && ((scb)->ps_pretend & PS_PRETEND_ACTIVE))
#define SCB_PS_PRETEND_NORMALPS(scb)	(SCB_PS(scb) && !SCB_PS_PRETEND(scb))
#define SCB_PS_PRETEND_THRESHOLD(scb)	((scb) && ((scb)->ps_pretend & PS_PRETEND_THRESHOLD))
/* Threshold mode never expects active PMQ, so do not block waiting for PMQ */
#define	SCB_PS_PRETEND_BLOCKED(scb)	(SCB_PS_PRETEND(scb) && \
					 !SCB_PS_PRETEND_THRESHOLD(scb) && \
					 !(((scb)->ps_pretend & PS_PRETEND_ACTIVE_PMQ) || \
					   ((scb)->ps_pretend & PS_PRETEND_NO_BLOCK)))
#define	SCB_PS_PRETEND_PROBING(scb)	(SCB_PS_PRETEND(scb) && \
					 ((scb)->ps_pretend & PS_PRETEND_PROBING))
#define SCB_PS_PRETEND_ENABLED(cfg, scb) (PS_PRETEND_ENABLED(cfg) && \
					  !((scb)->ps_pretend & PS_PRETEND_PREVENT) && \
					  !SCB_ISMULTI(scb))
#define SCB_PS_PRETEND_THRESHOLD_ENABLED(cfg, scb) (PS_PRETEND_THRESHOLD_ENABLED(cfg) && \
						    !((scb)->ps_pretend & PS_PRETEND_PREVENT) && \
						    !SCB_ISMULTI(scb))
#define SCB_PS_PRETEND_WAS_RECENT(scb)	((scb) && ((scb)->ps_pretend & PS_PRETEND_RECENT))
#define SCB_PS_PRETEND_CSA_PREVENT(scb, bsscfg)    \
					 ((scb) && \
					 ((scb)->ps_pretend & PS_PRETEND_PREVENT) && \
					 (BSSCFG_IS_CSA_IN_PROGRESS(bsscfg)))
#else

/* per bss enable */
#define PS_PRETEND_ENABLED(cfg)			(0)
#define PS_PRETEND_THRESHOLD_ENABLED(cfg)	(0)

/* per scb enable */
#define	SCB_PS_PRETEND(scb)			(0)
#define SCB_PS_PRETEND_NORMALPS(scb)		SCB_PS(scb)
#define	SCB_PS_PRETEND_BLOCKED(scb)		(0)
#define SCB_PS_PRETEND_THRESHOLD(scb)		(0)
#define	SCB_PS_PRETEND_PROBING(scb)		(0)
#define SCB_PS_PRETEND_ENABLED(cfg, scb)	(0)
#define SCB_PS_PRETEND_THRESHOLD_ENABLED(cfg, scb)	(0)
#define SCB_PS_PRETEND_WAS_RECENT(scb)		(0)
#define SCB_PS_PRETEND_CSA_PREVENT(scb, bsscfg)	(0)
#endif /* PSPRETEND */

#ifdef PSPRETEND

bool wlc_pspretend_limit_transit(wlc_pps_info_t *pps, wlc_bsscfg_t *cfg, struct scb *scb,
	int in_transit);
bool wlc_pspretend_pkt_retry(wlc_pps_info_t *pps, wlc_bsscfg_t *cfg, struct scb *scb,
	void *pkt, tx_status_t *txs, d11actxh_t *vhtHdr, uint16 macCtlHigh);
void wlc_pspretend_rate_reset(wlc_pps_info_t *pps, wlc_bsscfg_t *cfg, struct scb *scb,
	uint8 ac);
bool wlc_pspretend_pkt_relist(wlc_pps_info_t *pps, wlc_bsscfg_t *cfg, struct scb *scb,
	int retry_limit, int txretry);

void wlc_pspretend_scb_ps_off(wlc_pps_info_t *pps, struct scb *scb);

void wlc_pspretend_dotxstatus(wlc_pps_info_t *pps, wlc_bsscfg_t *cfg, struct scb *scb,
	bool pps_recvd_ack);
bool wlc_pspretend_on(wlc_pps_info_t *pps, struct scb *scb,
	uint8 flags);

void wlc_pspretend_probe_sched(wlc_pps_info_t *pps, struct scb *scb);

void wlc_pspretend_csa_start(wlc_pps_info_t *pps, wlc_bsscfg_t *cfg);
void wlc_pspretend_csa_upd(wlc_pps_info_t *pps, wlc_bsscfg_t *cfg);

/* attach/detach */
wlc_pps_info_t *wlc_pspretend_attach(wlc_info_t *wlc);
void wlc_pspretend_detach(wlc_pps_info_t *pps_info);
#endif /* PSPRETEND */

#endif /* _wlc_pspretend_h_ */
