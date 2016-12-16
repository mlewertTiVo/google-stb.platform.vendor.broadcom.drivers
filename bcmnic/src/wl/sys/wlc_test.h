/*
 * WLTEST interface.
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
 * $Id: wlc_test.h 657085 2016-08-31 01:19:55Z $
 */

#ifndef _wlc_test_h_
#define _wlc_test_h_

#include <wlc_types.h>

wlc_test_info_t *wlc_test_attach(wlc_info_t *wlc);
void wlc_test_detach(wlc_test_info_t *test);

#if defined(WLPKTENG)
void *wlc_tx_testframe(wlc_info_t *wlc, struct ether_addr *da,
	struct ether_addr *sa, ratespec_t rate_override, int length);
/* Create a test frame and enqueue into tx fifo */
extern void *wlc_mutx_testframe(wlc_info_t *wlc, struct scb *scb, struct ether_addr *sa,
                 ratespec_t rate_override, int fifo, int length, uint16 seq);
#endif

#endif /* _wlc_test_h_ */
