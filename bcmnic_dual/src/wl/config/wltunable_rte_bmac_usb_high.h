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
 * $Id: wltunable_rte_bmac_usb_high.h 432993 2013-10-30 07:41:01Z $
 *
 * wl high driver tunables for bmac(4323, 43236,4319) rte dev
 */

#define NTXD		128	/* THIS HAS TO MATCH with HIGH driver tunable, AMPDU/rpc_agg */
#define NRXD		64
#define NRXBUFPOST	24
#define WLC_DATAHIWAT	20
#define RXBND		24
#define NRPCTXBUFPOST	64	/* used in HIGH driver */

#define MAXTXPKTS_AMPDUAQM 128
#define MAXTXPKTS_AMPDUAQM_DFT 128

#define NTXD_USB_4319	64
#define NRPCTXBUFPOST_USB_4319	64	/* used in HIGH driver */

#define NRPCTXBUFPOST_USB_43556	120	/* used in HIGH driver */
