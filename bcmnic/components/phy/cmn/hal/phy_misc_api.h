/*
 * Miscellaneous PHY module public interface (to MAC driver).
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: phy_misc_api.h 627541 2016-03-25 01:49:14Z hou $
 */

#ifndef _phy_misc_api_h_
#define _phy_misc_api_h_

#include <phy_api.h>


#define VASIP_NOVERSION 0xff

/*
 * Return vasip version, -1 if not present.
 */
uint8 phy_misc_get_vasip_ver(phy_info_t *pi);
/*
 * reset/activate vasip.
 */
void phy_misc_vasip_proc_reset(phy_info_t *pi, int reset);

void phy_misc_vasip_clk_set(phy_info_t *pi, bool val);
void phy_misc_vasip_bin_write(phy_info_t *pi, const uint32 vasip_code[], const uint nbytes);

#ifdef VASIP_SPECTRUM_ANALYSIS
void phy_misc_vasip_spectrum_tbl_write(phy_info_t *pi,
        const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl);
#endif

void phy_misc_vasip_svmp_write(phy_info_t *pi, uint32 offset, uint16 val);
uint16 phy_misc_vasip_svmp_read(phy_info_t *pi, uint32 offset);
void phy_misc_vasip_svmp_write_blk(phy_info_t *pi, uint32 offset, uint16 len, uint16 *val);
void phy_misc_vasip_svmp_read_blk(phy_info_t *pi, uint32 offset, uint16 len, uint16 *val);

void wlc_phy_set_deaf(wlc_phy_t *ppi, bool user_flag);
void wlc_phy_clear_deaf(wlc_phy_t *ppi, bool user_flag);
#ifdef ATE_BUILD
void wlc_phy_gpaio(wlc_phy_t *ppi, wl_gpaio_option_t option, int core);
#endif /* ATE_BUILD */
void wlc_phy_hold_upd(wlc_phy_t *ppi, mbool id, bool val);
void wlc_phy_mute_upd(wlc_phy_t *ppi, bool val, mbool flags);
bool wlc_phy_ismuted(wlc_phy_t *pih);

#endif /* _phy_misc_api_h_ */
