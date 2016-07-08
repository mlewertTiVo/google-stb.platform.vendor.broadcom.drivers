/*
 * Broadcom 802.11abgn Networking Device Driver Configuration file
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wltunable_rte_4319_bmac.h 241182 2011-02-17 21:50:03Z $
 *
 * wl driver tunables for 4319a0 rte dev
 */

#define	D11CONF		0x400000 	/* D11 Core Rev 19 */
#define D11CONF2	0		/* D11 Core Rev > 31 */
#define SSLPNCONF	0x10		/* SSLPN-Phy rev 2 */

#define NTXD		64
#define NRXD		32
#define	NRXBUFPOST	16
#define RXBND		16
#define	MAXSCB		8
#define WLC_DATAHIWAT	10
#define NRPCTXBUFPOST	32
#define NTXD_USB_4319	64
#define NRPCTXBUFPOST_USB_4319 32
#define DNGL_MEM_RESTRICT_RXDMA	(8 * 2048)
