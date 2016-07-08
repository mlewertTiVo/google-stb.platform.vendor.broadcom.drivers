/*
 * Common header file between uart driver and wl driver
 * for remote dongle transport support
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
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Broadcom Corporation.
 *
 * $Id: rwl_shared.h 543582 2015-03-24 19:48:01Z $
 */

#ifndef _rwl_shared_h_
#define _rwl_shared_h_

typedef struct rwl_dongle_packet {
	uint packet_len; 		/* len of CDC header + data */
	uint packet_status; /* Dongle UART sets this flag when packet is present */
	uchar *packet_buf;
} rwl_dongle_packet_t;

#endif	/* _rwl_shared_h_ */
