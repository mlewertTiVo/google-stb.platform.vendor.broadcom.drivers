/*
 * VASIP PHY module public interface (to MAC driver).
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
 * $Id: phy_vasip_api.h 659421 2016-09-14 06:45:22Z $
 */

#ifndef _phy_vasip_api_h_
#define _phy_vasip_api_h_

#include <phy_api.h>

#ifndef VASIP_NOVERSION
#define VASIP_NOVERSION 0xFF
#endif

/*
 * Return vasip version, -1 if not present.
 */
uint8 phy_vasip_get_ver(phy_info_t *pi);
/*
 * reset/activate vasip.
 */
void phy_vasip_reset_proc(phy_info_t *pi, int reset);

void phy_vasip_set_clk(phy_info_t *pi, bool val);
void phy_vasip_write_bin(phy_info_t *pi, const uint32 vasip_code[], const uint nbytes);

#ifdef VASIP_SPECTRUM_ANALYSIS
void phy_vasip_write_spectrum_tbl(phy_info_t *pi,
        const uint32 vasip_spectrum_tbl[], const uint nbytes_tbl);
#endif

void phy_vasip_write_svmp(phy_info_t *pi, uint32 offset, uint16 val);
void phy_vasip_read_svmp(phy_info_t *pi, uint32 offset, uint16 *val);
void phy_vasip_write_svmp_blk(phy_info_t *pi, uint32 offset, uint16 len, uint16 *val);
void phy_vasip_read_svmp_blk(phy_info_t *pi, uint32 offset, uint16 len, uint16 *val);
#endif /* _phy_vasip_api_h_ */
