/*
 * ABGPHY module header file
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
 * $Id: wlc_phy_n.h 606042 2015-12-14 06:21:23Z jqliu $
 */

#ifndef _wlc_phy_n_h_
#define _wlc_phy_n_h_


#include <typedefs.h>
#include <phy_n_info.h>

extern void
wlc_phy_rfctrl_override_nphy_rev7(phy_info_t *pi, uint16 field, uint16 value, uint8 core_mask,
                                  uint8 off, uint8 override_id);

extern void
wlc_phy_rfctrl_override_nphy(phy_info_t *pi, uint16 field, uint16 value, uint8 core_mask,
                             uint8 off);

/* ********************* REMOVE *********************** */
int16 wlc_phy_rxgaincode_to_dB_nphy(phy_info_t *pi, uint16 gain_code);

#endif /* _wlc_phy_n_h_ */
