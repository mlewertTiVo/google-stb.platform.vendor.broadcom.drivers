/*
 * Noise module public interface (to MAC driver).
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
 * $Id: phy_noise_api.h 663581 2016-10-06 02:16:07Z $
 */

#ifndef _phy_noise_api_h_
#define _phy_noise_api_h_

#include <typedefs.h>
#include <phy_api.h>

/* Reasons for changing the PHY BG NOISE scheduling */
typedef enum  phy_bgnoise_schd_mode {
	PHY_LPAS_MODE = 0,
	PHY_CAL_MODE = 1,
	PHY_FORCEMEASURE_MODE = 2
} phy_bgnoise_schd_mode_t;

/* Fixed Noise for APHY/BPHY/GPHY */
#define PHY_NOISE_FIXED_VAL				(-101)	/* reported fixed noise */
#define PHY_NOISE_FIXED_VAL_NPHY		(-101)	/* reported fixed noise */
#define PHY_NOISE_INVALID				(0)

/* phy_mode bit defs for high level phy mode state information */
#define PHY_MODE_ACI		0x0001	/* set if phy is in ACI mitigation mode */
#define PHY_MODE_CAL		0x0002	/* set if phy is in Calibration mode */
#define PHY_MODE_NOISEM		0x0004	/* set if phy is in Noise Measurement mode */

#ifndef WLC_DISABLE_ACI
void wlc_phy_interf_rssi_update(wlc_phy_t *pi, chanspec_t chanNum, int8 leastRSSI);
#endif
/* HWACI Interrupt Handler */
void wlc_phy_hwaci_mitigate_intr(wlc_phy_t *pih);

/* Interference and Mitigation (ACI, Noise) Module */
int phy_noise_interf_chan_stats_update(wlc_phy_t *pi, chanspec_t chanspec, uint32 crsglitch,
	uint32 bphy_crsglitch, uint32 badplcp, uint32 bphy_badplcp, uint32 mbsstime);
int phy_noise_sched_set(wlc_phy_t *pih, phy_bgnoise_schd_mode_t reason, bool upd);
bool phy_noise_sched_get(wlc_phy_t *pih);
int phy_noise_pmstate_set(wlc_phy_t *pih, bool pmstate);
int8 phy_noise_avg_per_antenna(wlc_phy_t *pih, int coreidx);
int8 phy_noise_avg(wlc_phy_t *wpi);
int8 phy_noise_lte_avg(wlc_phy_t *wpi);
int phy_noise_sample_request_external(wlc_phy_t *ppi);
int phy_noise_sample_request_crsmincal(wlc_phy_t *ppi);
int phy_noise_sample_intr(wlc_phy_t *ppi);
#endif /* _phy_noise_api_h_ */
