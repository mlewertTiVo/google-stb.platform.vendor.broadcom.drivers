/*
 * RADIO control module implementation - shared by PHY type specific implementations.
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
 * $Id: phy_utils_radio.c 659938 2016-09-16 16:47:54Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <phy_api.h>
#include <phy_utils_radio.h>
#include <phy_radio_api.h>

#include <wlc_phy_hal.h>
#include <wlc_phy_int.h>

void
phy_utils_parse_idcode(phy_info_t *pi, uint32 idcode)
{
	pi->pubpi->radioid = (idcode & IDCODE_ID_MASK) >> IDCODE_ID_SHIFT;
	pi->pubpi->radiorev = (idcode & IDCODE_REV_MASK) >> IDCODE_REV_SHIFT;
	pi->pubpi->radiover = (idcode & IDCODE_VER_MASK) >> IDCODE_VER_SHIFT;

}

int
phy_utils_valid_radio(phy_info_t *pi)
{
	if (VALID_RADIO(pi, RADIOID(pi->pubpi->radioid))) {
		/* ensure that the built image matches the target */
#ifdef BCMRADIOID
		if (pi->pubpi->radioid != BCMRADIOID)
			PHY_ERROR(("%s: Chip's radioid=0x%x, BCMRADIOID=0x%x\n",
			           __FUNCTION__, pi->pubpi->radioid, BCMRADIOID));
		ASSERT(pi->pubpi->radioid == BCMRADIOID);
#endif
#ifdef BCMRADIOREV
#ifdef BCMRADIOREV2
		if (PI_INSTANCE(pi) == 0 && pi->pubpi->radiorev != BCMRADIOREV) {
			PHY_ERROR(("%s: Chip's radiorev=%d, BCMRADIOREV=%d\n",
			           __FUNCTION__, pi->pubpi->radiorev, BCMRADIOREV));
			ASSERT(pi->pubpi->radiorev == BCMRADIOREV);
		} else if (PI_INSTANCE(pi) == 1 && pi->pubpi->radiorev != BCMRADIOREV2) {
			PHY_ERROR(("%s: Chip's radiorev=%d, BCMRADIOREV2=%d\n",
			           __FUNCTION__, pi->pubpi->radiorev, BCMRADIOREV2));
			ASSERT(pi->pubpi->radiorev == BCMRADIOREV2);
		}
#else
		if (pi->pubpi->radiorev != BCMRADIOREV) {
			PHY_ERROR(("%s: Chip's radiorev=%d, BCMRADIOREV=%d\n",
			           __FUNCTION__, pi->pubpi->radiorev, BCMRADIOREV));
			ASSERT(pi->pubpi->radiorev == BCMRADIOREV);
		}
#endif /* BCMRADIOREV2 */
#endif /* BCMRADIOREV */
		return BCME_OK;
	} else {
		PHY_ERROR(("wl%d: %s: Unknown radio ID: 0x%x rev 0x%x phy %d, phyrev %d\n",
		           pi->sh->unit, __FUNCTION__,
		           RADIOID(pi->pubpi->radioid), RADIOREV(pi->pubpi->radiorev),
		           pi->pubpi->phy_type, pi->pubpi->phy_rev));
		return BCME_UNSUPPORTED;
	}
}
