/*
 * PHY Core internal interface (to PHY type).
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
 * $Id: phy_shared.h 583048 2015-08-31 16:43:34Z $
 */

#ifndef _phy_shared_h_
#define _phy_shared_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_api.h>

typedef struct phy_regs {
	uint16	base;
	uint16	num;
} phy_regs_t;

void phy_dump_phyregs(phy_info_t *pi, const char *str,
	phy_regs_t *rl, uint16 off, struct bcmstrbuf *b);

#endif /* _phy_shared_h_ */
