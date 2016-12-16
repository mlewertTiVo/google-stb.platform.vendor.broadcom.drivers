/*
 * Channel Manager module public interface (to MAC driver).
 *
 * Controls radio chanspec and manages related operating chanpsec context.
 *
 * Operating chanspec context:
 *
 * A Operating chanspec context is a collection of s/w properties
 * associated with a radio chanspec.
 *
 * Current operating chanspec context:
 *
 * The current operating chanspec context is the operating chanspec context
 * whose associated chanspec is used as the current radio chanspec, and
 * whose s/w properties are applied to the corresponding h/w if any.
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
 * $Id: phy_chanmgr_api.h 659938 2016-09-16 16:47:54Z $
 */

#ifndef _phy_chanmgr_api_h_
#define _phy_chanmgr_api_h_

#include <typedefs.h>
#include <bcmwifi_channels.h>
#include <phy_api.h>

/* add the feature to take out the BW RESET duriny the BW change  0: disable 1: enable */
#define BW_RESET 1

#ifdef PHYCAL_CACHING
/*
 * Create/Destroy an operating chanspec context for radio 'chanspec'.
 */
int phy_chanmgr_create_ctx(phy_info_t *pi, chanspec_t chanspec);
void phy_chanmgr_destroy_ctx(phy_info_t *pi, chanspec_t chanspec);

/*
 * Set the operating chanspec context associated with the 'chanspec'
 * as the current operating chanspec context, and set the 'chanspec'
 * as the current radio chanspec.
 */
int phy_chanmgr_set_oper(phy_info_t *pi, chanspec_t chanspec);

/*
 * Set the radio chanspec to 'chanspec', and unset the current
 * operating chanspec context if any.
 */
int phy_chanmgr_set(phy_info_t *pi, chanspec_t chanspec);
#endif /* PHYCAL_CACHING */

void wlc_phy_chanspec_set(wlc_phy_t *ppi, chanspec_t chanspec);
void wlc_phy_chanspec_radio_set(wlc_phy_t *ppi, chanspec_t newch);
bool wlc_phy_is_txbfcal(wlc_phy_t *ppi);

/* band specific init */
int phy_chanmgr_bsinit(phy_info_t *pi, chanspec_t chanspec, bool forced);
/* band width init */
int phy_chanmgr_bwinit(phy_info_t *pi, chanspec_t chanspec);

/*     VSDB, RVSDB Module related definitions         */
uint8 phy_chanmgr_vsdb_sr_attach_module(wlc_phy_t *ppi, chanspec_t chan0, chanspec_t chan1);
int phy_chanmgr_vsdb_sr_detach_module(wlc_phy_t *pi);
uint8 phy_chanmgr_vsdb_sr_set_chanspec(wlc_phy_t *pi, chanspec_t chanspec, uint8 * last_chan_saved);
int phy_chanmgr_vsdb_force_chans(wlc_phy_t *pi, uint16 * chans, uint8 set);
#endif /* _phy_chanmgr_api_h_ */
