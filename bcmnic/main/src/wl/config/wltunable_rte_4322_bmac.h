/*
 * Broadcom 802.11abg Networking Device Driver Configuration file
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wltunable_rte_4322_bmac.h 404499 2013-05-28 01:06:37Z $
 *
 * wl driver tunables for BOTH HIGH and BMAC driver
 */

#define D11CONF		0x10000		/* D11 Core Rev 16 */
#define D11CONF2	0		/* D11 Core Rev > 31 */
#define NCONF		0x10		/* N-Phy Rev 4 (use 0x18 for Rev 3 & 4) */

#define NTXD		128	/* THIS HAS TO MATCH with HIGH driver tunable, AMPDU/rpc_agg */
#define NRXD		32
#define NRXBUFPOST	16
#define WLC_DATAHIWAT	10
#define RXBND		16
#define NRPCTXBUFPOST	128	/* used in HIGH driver */
#define DNGL_MEM_RESTRICT_RXDMA    (6*2048)
