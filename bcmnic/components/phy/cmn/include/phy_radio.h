/*
 * RADIO control module internal interface (to other PHY modules).
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
 * $Id: phy_radio.h 659961 2016-09-16 18:46:01Z $
 */

#ifndef _phy_radio_h_
#define _phy_radio_h_

#include <typedefs.h>
#include <phy_api.h>
#include <phy_dbg.h>

/* forward declaration */
typedef struct phy_radio_info phy_radio_info_t;

typedef struct radio_20xx_regs {
	uint16 address;
	uint16  init;
	uint8  do_init;
} radio_20xx_regs_t;

extern radio_20xx_regs_t regs_2057_rev4[], regs_2057_rev5[], regs_2057_rev5v1[];
extern radio_20xx_regs_t regs_2057_rev7[], regs_2057_rev8[], regs_2057_rev9[], regs_2057_rev10[];
extern radio_20xx_regs_t regs_2057_rev12[];
extern radio_20xx_regs_t regs_2059_rev0[];
extern radio_20xx_regs_t regs_2057_rev13[];
extern radio_20xx_regs_t regs_2057_rev14[];
extern radio_20xx_regs_t regs_2057_rev14v1[];
extern radio_20xx_regs_t regs_2069_rev0[];

/* attach/detach */
phy_radio_info_t *phy_radio_attach(phy_info_t *pi);
void phy_radio_detach(phy_radio_info_t *ri);

/* query radio idcode */
uint32 phy_radio_query_idcode(phy_radio_info_t *ri);

#ifdef PHY_DUMP_BINARY
/* get radio register list and size */
int phy_radio_getlistandsize(phy_info_t *pi, phyradregs_list_t **phyreglist, uint16 *phyreglist_sz);
#endif
#ifdef RADIO_HEALTH_CHECK
bool phy_radio_pll_lock(phy_radio_info_t *radioi);
#endif /* RADIO_HEALTH_CHECK */
uint phy_radio_init_radio_regs_allbands(phy_info_t *pi, const radio_20xx_regs_t *radioregs);
#endif /* _phy_radio_h_ */
