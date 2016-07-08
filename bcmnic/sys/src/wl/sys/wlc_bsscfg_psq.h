/*
 * Per-BSS psq interface.
 * Used to save suppressed packets in the BSS.
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
 * $Id: wlc_bsscfg_psq.h 523117 2014-12-26 18:32:49Z $
 */
#ifndef _wlc_bsscfg_psq_h_
#define _wlc_bsscfg_psq_h_

#include <typedefs.h>
#include <wlc_types.h>

wlc_bsscfg_psq_info_t *wlc_bsscfg_psq_attach(wlc_info_t *wlc);
void wlc_bsscfg_psq_detach(wlc_bsscfg_psq_info_t *psqi);

#ifdef WL_BSSCFG_TX_SUPR
void wlc_bsscfg_tx_stop(wlc_bsscfg_psq_info_t *psqi, wlc_bsscfg_t *cfg);
void wlc_bsscfg_tx_start(wlc_bsscfg_psq_info_t *psqi, wlc_bsscfg_t *cfg);
bool wlc_bsscfg_tx_psq_enq(wlc_bsscfg_psq_info_t *psqi, wlc_bsscfg_t *cfg, void *sdu, uint prec);
void wlc_bsscfg_tx_check(wlc_bsscfg_psq_info_t *psqi);
bool wlc_bsscfg_tx_supr_enq(wlc_bsscfg_psq_info_t *psqi, wlc_bsscfg_t *cfg, void *pkt);
uint wlc_bsscfg_tx_pktcnt(wlc_bsscfg_psq_info_t *psqi, wlc_bsscfg_t *cfg);
void wlc_bsscfg_tx_flush(wlc_bsscfg_psq_info_t *psqi, wlc_bsscfg_t *cfg);
#else /* WL_BSSCFG_TX_SUPR */
#define wlc_bsscfg_tx_stop(a, b) do { } while (0)
#define wlc_bsscfg_tx_start(a, b) do { } while (0)
#define wlc_bsscfg_tx_psq_enq(a, b, c, d) FALSE
#define wlc_bsscfg_tx_check(a) do { } while (0)
#define wlc_bsscfg_tx_supr_enq(a, b, c) ((void)(b), FALSE)
#define wlc_bsscfg_tx_pktcnt(a, b) 0
#define wlc_bsscfg_tx_flush(a, b) do { } while (0)
#endif /* WL_BSSCFG_TX_SUPR */

#endif /* _wlc_bsscfg_psq_h_ */
