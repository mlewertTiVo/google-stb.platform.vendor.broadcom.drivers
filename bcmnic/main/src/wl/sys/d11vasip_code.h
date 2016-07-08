/*
 * VASIP init declarations for Broadcom 802.11
 * Networking Adapter Device Driver.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: d11vasip_code.h 598621 2015-11-10 10:28:25Z $
 */

/* vasip code and inits */

extern CONST uint32 d11vasipcode_major;
extern CONST uint32 d11vasipcode_minor;

extern CONST uint32 d11vasip0[];
extern CONST uint d11vasip0sz;

#ifdef VASIP_SPECTRUM_ANALYSIS
extern CONST uint32 d11vasip_tbl[];
extern CONST uint d11vasip_tbl_sz;
#endif
