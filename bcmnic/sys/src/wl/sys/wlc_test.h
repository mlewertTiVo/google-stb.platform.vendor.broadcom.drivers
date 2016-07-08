/*
 * WLTEST interface.
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
 * $Id: wlc_test.h 589774 2015-09-30 17:27:17Z $
 */

#ifndef _wlc_test_h_
#define _wlc_test_h_

#include <wlc_types.h>

wlc_test_info_t *wlc_test_attach(wlc_info_t *wlc);
void wlc_test_detach(wlc_test_info_t *test);

#if defined(WLPKTENG)
void *wlc_tx_testframe(wlc_info_t *wlc, struct ether_addr *da,
	struct ether_addr *sa, ratespec_t rate_override, int length);
#endif

#endif /* _wlc_test_h_ */
