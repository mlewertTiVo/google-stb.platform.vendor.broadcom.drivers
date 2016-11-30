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
 * $Id: phy_radio.h 616117 2016-01-29 19:41:50Z vyass $
 */

#ifndef _phy_radio_h_
#define _phy_radio_h_

#include <typedefs.h>
#include <phy_api.h>
#include <phy_dbg.h>

/* forward declaration */
typedef struct phy_radio_info phy_radio_info_t;

/* attach/detach */
phy_radio_info_t *phy_radio_attach(phy_info_t *pi);
void phy_radio_detach(phy_radio_info_t *ri);

/* query radio idcode */
uint32 phy_radio_query_idcode(phy_radio_info_t *ri);

#ifdef PHY_DUMP_BINARY
/* get radio register list and size */
int phy_radio_getlistandsize(phy_info_t *pi, phyradregs_list_t **phyreglist, uint16 *phyreglist_sz);
#endif
#endif /* _phy_radio_h_ */
