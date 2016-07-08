/*
 * Common (OS-independent) portion of
 * Broadcom 802.11bang Networking Device Driver
 *
 * ucode download and initialization routines. [including patch for ucode ROM]
 * used by bmac and wowl.
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
 * $Id: wlc_ucinit.h 596126 2015-10-29 19:53:48Z $
 */

#ifndef _WLC_UCI_H_
#define _WLC_UCI_H_

#define UCODE_INITVALS		(1)
#define UCODE_BSINITVALS	(2)

extern int wlc_uci_download_rom_patches(wlc_hw_info_t *wlc_hw);
extern bool wlc_uci_check_cap_ucode_rom_axislave(wlc_hw_info_t *wlc_hw);
extern int wlc_uci_write_inits_with_rom_support(wlc_hw_info_t *wlc_hw, uint32 flags);
#ifdef WOWL
extern int wlc_uci_wowl_ucode_init_rom_patches(wlc_info_t *wlc);
#endif /* WOWL */

#endif /* _WLC_UCI_H_ */
