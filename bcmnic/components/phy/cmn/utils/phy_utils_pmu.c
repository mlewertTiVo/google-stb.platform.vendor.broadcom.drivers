/*
 * PMU control module implementation - shared by PHY type specific implementations.
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
 * $Id: phy_utils_pmu.c 659421 2016-09-14 06:45:22Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <phy_api.h>
#include <phy_utils_pmu.h>
#include <siutils.h>
#include <sbchipc.h>


#include <wlc_phy_hal.h>
#include <wlc_phy_int.h>

void
phy_utils_pmu_regcontrol_access(phy_info_t *pi, uint8 addr, uint32* val, bool write)
{
	si_t *sih;
	chipcregs_t *cc;
	uint origidx, intr_val;

	/* shared pi handler */
	sih = (si_t*)pi->sh->sih;

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);

	if (write) {
		W_REG(si_osh(sih), &cc->regcontrol_addr, addr);
		W_REG(si_osh(sih), &cc->regcontrol_data, *val);
		/* read back to confirm */
		*val = R_REG(si_osh(sih), &cc->regcontrol_data);
	} else {
		W_REG(si_osh(sih), &cc->regcontrol_addr, addr);
		*val = R_REG(si_osh(sih), &cc->regcontrol_data);
	}

	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);
}

void
phy_utils_pmu_chipcontrol_access(phy_info_t *pi, uint8 addr, uint32* val, bool write)
{
	si_t *sih;
	chipcregs_t *cc;
	uint origidx, intr_val;

	/* shared pi handler */
	sih = (si_t*)pi->sh->sih;

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);

	if (write) {
		W_REG(si_osh(sih), &cc->chipcontrol_addr, addr);
		W_REG(si_osh(sih), &cc->chipcontrol_data, *val);
		/* read back to confirm */
		*val = R_REG(si_osh(sih), &cc->chipcontrol_data);
	} else {
		W_REG(si_osh(sih), &cc->chipcontrol_addr, addr);
		*val = R_REG(si_osh(sih), &cc->chipcontrol_data);
	}

	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);
}
