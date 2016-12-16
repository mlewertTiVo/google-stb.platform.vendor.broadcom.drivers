/*
 * Calibration manager module public interface (to MAC driver).
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
 * $Id: phy_calmgr_api.h $
 */

#ifndef _phy_calmgr_api_h_
#define _phy_calmgr_api_h_

#include <typedefs.h>
#include <phy_api.h>

#define PHY_PERICAL_DRIVERUP	1	/* periodic cal for driver up */
#define PHY_PERICAL_WATCHDOG	2	/* periodic cal for watchdog */
#define PHY_PERICAL_PHYINIT	3	/* periodic cal for phy_init */
#define PHY_PERICAL_JOIN_BSS	4	/* periodic cal for join BSS */
#define PHY_PERICAL_START_IBSS	5	/* periodic cal for join IBSS */
#define PHY_PERICAL_UP_BSS	6	/* periodic cal for up BSS */
#define PHY_PERICAL_CHAN	7	/* periodic cal for channel change */
#define PHY_FULLCAL		8	/* full calibration routine */
#define PHY_PAPDCAL		10	/* papd calibration routine */
#define PHY_PERICAL_DCS		11	/* periodic cal for DCS */
#ifdef WLOTA_EN
#define PHY_FULLCAL_SPHASE	PHY_PERICAL_JOIN_BSS /* full cal in single phase */
#endif /* WLOTA_EN */

/* full cal in single phase in the event of phymode switch  */
#define PHY_PERICAL_PHYMODE_SWITCH	12

#define PHY_PERICAL_DISABLE	0	/* periodic cal disabled */
#define PHY_PERICAL_SPHASE	1	/* periodic cal enabled, single phase only */
#define PHY_PERICAL_MPHASE	2	/* periodic cal enabled, can do multiphase */
#define PHY_PERICAL_MANUAL	3	/* disable periodic cal, only run it from iovar */

#define PHY_PERICAL_MAXINTRVL (15*60) /* Maximum time interval in sec between PHY calibrations */

#define PHY_PERICAL_DELAY_DEFAULT	5	/* delay (in ms) between each cal mphase */
#define PHY_PERICAL_DELAY_MIN		1	/* min delay between each cal mphase */
#define PHY_PERICAL_DELAY_MAX		2000	/* max delay between each cal mphase */
#define PHY_MPH_CAL_DELAY		150	/* multi phase phy cal schedule delay */

int phy_calmgr_init(wlc_phy_t *ppi);
int phy_calmgr_enable_initcal(wlc_phy_t *ppi, bool initcal);

/* Get/Set calibration mode: single phase, multiphase... */
uint8 phy_calmgr_get_calmode(phy_info_t *pi);
int phy_calmgr_set_calmode(phy_info_t *pi, uint32 newmode);
int phy_calmgr_set_glacial_timer(wlc_phy_t *ppi, uint period);
uint32 phy_calmgr_get_cal_dur(wlc_phy_t *ppi);

#endif /* _phy_calmgr_api_h_ */
