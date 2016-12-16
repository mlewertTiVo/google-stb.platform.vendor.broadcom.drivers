
/*
 * Health check module.
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
 * $Id: phy_hc.h 659514 2016-09-14 20:19:00Z $
 */
#ifndef _phy_hc_h_
#define _phy_hc_h_

#include <typedefs.h>
#include <phy_api.h>
#include <phy_hc_api.h>

/* forward declaration */
typedef struct phy_hc_info phy_hc_info_t;


#define PHY_HC_TEMP_FAIL_THRESHOLD	12
#define PHY_HC_TEMP_THRESHOLD		120

/* attach/detach */
phy_hc_info_t *phy_hc_attach(phy_info_t *pi);
void phy_hc_detach(phy_hc_info_t *hci);

int phy_hc_healthcheck(phy_hc_info_t *hci, phy_healthcheck_type_t hc);
int phy_hc_iovar_hc_mode(phy_hc_info_t *hci, bool get, int int_val);
uint16 phy_hc_get_rhc_tempthresh(phy_hc_info_t *hci);

#endif /* _phy_hc_h_ */
