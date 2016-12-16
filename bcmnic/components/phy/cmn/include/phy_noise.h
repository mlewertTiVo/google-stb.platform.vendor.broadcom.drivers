/*
 * NOISEmeasure module internal interface (to other PHY modules).
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
 * $Id: phy_noise.h 661333 2016-09-24 02:42:24Z $
 */

#ifndef _phy_noise_h_
#define _phy_noise_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_noise_info phy_noise_info_t;

/* attach/detach */
phy_noise_info_t *phy_noise_attach(phy_info_t *pi, int bandtype);
void phy_noise_detach(phy_noise_info_t *nxi);

int phy_noise_bss_init(phy_noise_info_t *noisei, int noise);

/* noise calculation */
void wlc_phy_noise_calc(phy_info_t *pi, uint32 *cmplx_pwr, int8 *pwr_ant,
                               uint8 extra_gain_1dB);
void wlc_phy_noise_calc_fine_resln(phy_info_t *pi, uint32 *cmplx_pwr, uint16 *crsmin_pwr,
		int16 *pwr_ant, uint8 extra_gain_1dB, int16 *tot_gain);

/* set mode */
int phy_noise_set_mode(phy_noise_info_t *ii, int mode, bool init);

/* set interference over-ride mode */
int wlc_phy_set_interference_override_mode(phy_info_t *pi, int val);

/* common dump functions for non-ac phy */
int phy_noise_dump_common(phy_info_t *pi, struct bcmstrbuf *b);

#ifdef RADIO_HEALTH_CHECK
phy_crash_reason_t
phy_noise_healthcheck_desense(phy_noise_info_t *noisei);
#endif /* RADIO_HEALTH_CHECK */

void wlc_phy_noise_save(phy_info_t *pi, int8 *noise_dbm_ant, int8 *max_noise_dbm);

void wlc_phy_aci_upd(phy_noise_info_t *ii);

void phy_noise_invoke_callbacks(phy_noise_info_t *noisei, uint8 channel, int8 noise_dbm);

/* Returns noise level (read from srom) for current channel */
int phy_noise_get_srom_level(phy_info_t *pi, int32 *ret_int_ptr);

int8 phy_noise_read_shmem(phy_noise_info_t *noisei, uint8 *lte_on, uint8 *crs_high);

int8 phy_noise_abort_shmem_read(phy_noise_info_t *noisei);
#ifndef WLC_DISABLE_ACI
#endif /* Compiling out ACI code for 4324 */
bool phy_noise_pmstate_get(phy_info_t *pi);
#endif /* _phy_noise_h_ */
