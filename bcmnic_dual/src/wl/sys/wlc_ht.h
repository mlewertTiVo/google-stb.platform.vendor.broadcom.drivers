/*
 * Common (OS-independent) portion of
 * Broadcom 802.11 Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

/** 802.11n (High Throughput) */

#ifndef _wlc_ht_h_
#define _wlc_ht_h_

/* module entries */
extern wlc_ht_info_t *wlc_ht_attach(wlc_info_t *wlc);
extern void wlc_ht_detach(wlc_ht_info_t *hti);

#endif /* _wlc_ht_h_ */
