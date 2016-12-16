/*
 * ACPHY Prephy modules implementation
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
 * $Id: phy_ac_prephy.c 658443 2016-09-08 01:19:02Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_prephy.h>
#include <phy_prephy_api.h>
#include "phy_type_prephy.h"
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <phy_ac_prephy.h>
#include <wlc_phy_int.h>
/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_phyreg_ac.h>

#include <phy_utils_reg.h>
#include <phy_utils_var.h>
#include <phy_rstr.h>
#include <bcmdevs.h>

/* preattachphy with nopi phy reg accessor macros */
#define _PHY_REG_READ_PREPHY(pi, regs, reg)	phy_utils_read_phyreg_nopi(pi, regs, reg)

#define READ_PHYREG_PREPHY(pi, regs, reg) \
	_PHY_REG_READ_PREPHY(pi, regs, ACPHY_##reg(pi->pubpi->phy_rev))

#define READ_PHYREGFLD_PREPHY(pi, regs, reg, field)				\
	((READ_PHYREG_PREPHY(pi, regs, reg)					\
	 & ACPHY_##reg##_##field##_##MASK(pi->pubpi->phy_rev)) >>	\
	 ACPHY_##reg##_##field##_##SHIFT(pi->pubpi->phy_rev))

#define WRITE_PHYREG_PREPHY(pi, regs, addr, val) \
	phy_utils_write_phyreg_nopi(pi, regs, ACPHY_##addr(pi->pubpi->phy_rev), val)

#define PHYREGFLD_SHIFT_PREPHY(pi, reg, field) \
	ACPHY_##reg##_##field##_##SHIFT(pi->pubpi->phy_rev)

/* vasip version read without pi */
void
phy_ac_prephy_vasip_ver_get(prephy_info_t *pi, d11regs_t *regs, uint32 *vasipver)
{
	bool vasippresent;

	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		vasippresent = TRUE;
	} else {
		vasippresent = READ_PHYREGFLD_PREPHY(pi, regs, PhyCapability2, vasipPresent);
	}

	if (vasippresent) {
		*vasipver = READ_PHYREGFLD_PREPHY(pi, regs, MinorVersion, vasipversion);
	} else {
		*vasipver = VASIP_NOVERSION;
	}
}

/* vasip reset without pi */
void
phy_ac_prephy_vasip_proc_reset(prephy_info_t *pi, d11regs_t *regs, bool reset)
{
	uint16 reset_offset = reset ? VASIPREGISTERS_RESET : VASIPREGISTERS_SET;
	uint16 reset_val = 1;

	WRITE_PHYREG_PREPHY(pi, regs, TableID, ACPHY_TBL_ID_VASIPREGISTERS);
	WRITE_PHYREG_PREPHY(pi, regs, TableOffset, reset_offset);
	WRITE_PHYREG_PREPHY(pi, regs, TableDataHi, 0);
	WRITE_PHYREG_PREPHY(pi, regs, TableDataLo, reset_val);
}

/* vasip reset without pi */
void
phy_ac_prephy_vasip_clk_set(prephy_info_t *pi, d11regs_t *regs, bool set)
{
	if (ACMAJORREV_40(pi->pubpi->phy_rev)) {
		phy_ac_vasip_clk_enable(pi->sh->sih, set);
	} else {
		uint16 regval;

		regval = READ_PHYREG_PREPHY(pi, regs, dacClkCtrl);
		if (set) {
			regval = regval |
				(1 << PHYREGFLD_SHIFT_PREPHY(pi, dacClkCtrl, vasipClkEn));
		} else {
			regval = regval &
				~(1 << PHYREGFLD_SHIFT_PREPHY(pi, dacClkCtrl, vasipClkEn));
		}
		WRITE_PHYREG_PREPHY(pi, regs, dacClkCtrl, regval);
	}
}

uint32
phy_ac_prephy_caps(prephy_info_t *pi, uint32 *pacaps)
{
	int ret = BCME_OK;

	ASSERT(pacaps);
	if (ACREV_LT(pi->pubpi->phy_rev, HECAP_FIRST_ACREV)) {
		ret = BCME_UNSUPPORTED;
		goto done;
	}

	*pacaps = READ_PHYREGFLD_PREPHY(pi, pi->regs, PhyCapability0, Support5GHz) ?
		PHY_PREATTACH_CAP_SUP_5G : 0;

	*pacaps |= READ_PHYREGFLD_PREPHY(pi, pi->regs, PhyInternalCapability1, Support2GHz) ?
		PHY_PREATTACH_CAP_SUP_2G : 0;

done:
	return ret;
}
