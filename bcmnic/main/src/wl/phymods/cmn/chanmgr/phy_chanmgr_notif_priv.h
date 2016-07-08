/*
 * Channel Manager Notification module internal interface.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#ifndef _phy_chanmgr_notif_priv_h_
#define _phy_chanmgr_notif_priv_h_

#include <phy_chanmgr_notif.h>

/* Notify registered clients of event, bail out if client indicates error. */
int phy_chanmgr_notif_signal(phy_chanmgr_notif_info_t *cni, phy_chanmgr_notif_data_t *data,
	bool exit_on_err);

#endif /* _phy_chanmgr_notif_priv_h_ */
