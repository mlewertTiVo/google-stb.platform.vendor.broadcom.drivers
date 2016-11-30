/*
 * Miscellaneous modules interface (to other PHY modules).
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
 * $Id: phy_ac_misc.h 644994 2016-06-22 06:23:44Z vyass $
 */

#ifndef _phy_ac_misc_h_
#define _phy_ac_misc_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_misc.h>


/* forward declaration */
typedef struct phy_ac_misc_info phy_ac_misc_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_misc_info_t *phy_ac_misc_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_misc_info_t *cmn_info);
void phy_ac_misc_unregister_impl(phy_ac_misc_info_t *ac_info);

extern void wlc_phy_update_rxldpc_acphy(phy_info_t *pi, bool ldpc);
extern void wlc_phy_force_rfseq_acphy(phy_info_t *pi, uint8 cmd);
extern uint16 wlc_phy_classifier_acphy(phy_info_t *pi, uint16 mask, uint16 val);
extern void wlc_phy_deaf_acphy(phy_info_t *pi, bool mode);
extern bool wlc_phy_get_deaf_acphy(phy_info_t *pi);
extern void wlc_phy_gpiosel_acphy(phy_info_t *pi, uint16 sel, uint8 word_swap);
#if defined(BCMINTERNAL) || defined(WLTEST)
extern void wlc_phy_test_scraminit_acphy(phy_info_t *pi, int8 init);
#endif /* BCMINTERNAL || WLTEST */

void wlc_phy_susp2tx_cts2self(phy_info_t *pi, uint16 duration);
void wlc_phy_cals_mac_susp_en_other_cr(phy_info_t *pi, bool suspend);

/* !!! This has been redeclared in wlc_phy_hal.h. Shoul dbe removed from there. !!! */
extern void wlc_acphy_set_scramb_dyn_bw_en(wlc_phy_t *pi, bool enable);
extern void wlc_phy_force_rfseq_noLoleakage_acphy(phy_info_t *pi);
extern void wlc_phy_force_femreset_acphy(phy_info_t *pi, bool ovr);

void wlc_phy_loadsampletable_acphy(phy_info_t *pi, math_cint32 *tone_buf, const uint16 num_samps,
        const bool alloc, bool conj);
int wlc_phy_tx_tone_acphy(phy_info_t *pi, int32 f_kHz, uint16 max_val, uint8 mode, uint8, bool);
void phy_ac_misc_modify_bbmult(phy_ac_misc_info_t *misci, uint16 max_val, bool modify_bbmult);

extern void wlc_phy_stopplayback_acphy(phy_info_t *pi);
extern void
wlc_phy_runsamples_acphy(phy_info_t *pi, uint16 num_samps, uint16 loops, uint16 wait, uint8 iqmode,
                         uint8 mac_based);

int phy_ac_misc_set_rud_agc_enable(phy_ac_misc_info_t *misci, int32 int_val);
int phy_ac_misc_get_rud_agc_enable(phy_ac_misc_info_t *misci, int32 *ret_int_ptr);
#endif /* _phy_ac_misc_h_ */
