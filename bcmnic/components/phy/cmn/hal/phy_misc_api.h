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
 * $Id: phy_misc_api.h 659938 2016-09-16 16:47:54Z $
 */

#ifndef _phy_misc_api_h_
#define _phy_misc_api_h_

#include <phy_api.h>

#define PHY_HOLD_FOR_ASSOC	1	/* hold PHY activities(like cal) during association */
#define PHY_HOLD_FOR_SCAN	2	/* hold PHY activities(like cal) during scan */
#define PHY_HOLD_FOR_RM		4	/* hold PHY activities(like cal) during radio measure */
#define PHY_HOLD_FOR_PLT	8	/* hold PHY activities(like cal) during plt */
#define PHY_HOLD_FOR_MUTE		0x10
#define PHY_HOLD_FOR_NOT_ASSOC	0x20
#define PHY_HOLD_FOR_ACI_SCAN	0x40 /* hold PHY activities(like cal) during ACI scan */
#define PHY_HOLD_FOR_PKT_ENG	0x80 /* hold PHY activities(like cal) during PKTENG mode */
#define PHY_HOLD_FOR_DCS	    0x100 /* hold PHY activities(like cal) during DCS */
#define PHY_HOLD_FOR_MPC_SCAN	0x200 /* hold PHY activities(like cal) during scan in mpc 1 mode */
#define PHY_HOLD_FOR_EXCURSION	0x400 /* hold PHY activities while in excursion */
#define PHY_HOLD_FOR_TOF	0x800 /* hold PHY activities while doing wifi ranging */
#define PHY_HOLD_FOR_VSDB	0x1000 /* hold PHY cals during  VSDB */

#define PHY_MUTE_FOR_PREISM	1
#define PHY_MUTE_ALL		0xffffffff

void phy_misc_set_ldpc_override(wlc_phy_t *ppi, bool ldpc);
int phy_misc_set_lo_gain_nbcal(wlc_phy_t *ppi, bool lo_gain);
int phy_misc_set_filt_war(wlc_phy_t *ppi, bool filt_war);
bool phy_misc_get_filt_war(wlc_phy_t *ppi);
int phy_misc_tkip_rifs_war(wlc_phy_t *ppi, uint8 rifs);
/* ******************** */
void wlc_phy_set_deaf(wlc_phy_t *ppi, bool user_flag);
void wlc_phy_clear_deaf(wlc_phy_t *ppi, bool user_flag);
void wlc_phy_hold_upd(wlc_phy_t *ppi, mbool id, bool val);
void wlc_phy_mute_upd(wlc_phy_t *ppi, bool val, mbool flags);
bool wlc_phy_ismuted(wlc_phy_t *ppi);

/* WAR */
void wlc_phy_conditional_suspend(phy_info_t *pi, bool *suspend);
void wlc_phy_conditional_resume(phy_info_t *pi, bool *suspend);
#endif /* _phy_misc_api_h_ */
