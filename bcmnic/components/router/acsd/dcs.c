/*
 * dcs.c
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * $Id: dcs.c 597371 2015-11-05 02:05:29Z $
 */
#include <stdio.h>
#include <string.h>

#include <assert.h>
#include <typedefs.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <bcmtimer.h>
#include <bcmendian.h>
#include <shutils.h>
#include <bcmendian.h>
#include <bcmwifi_channels.h>
#include <wlioctl.h>
#include <wlutils.h>

#include "acsd_svr.h"

int
dcs_parse_actframe(dot11_action_wifi_vendor_specific_t *actfrm, wl_bcmdcs_data_t *dcs_data)
{
	uint8 cat;
	uint32 reason;
	chanspec_t chanspec;

	if (!actfrm)
		return ACSD_ERR_NO_FRM;

	cat = actfrm->category;
	ACSD_INFO("recved action frames, category: %d\n", actfrm->category);
	if (cat != DOT11_ACTION_CAT_VS)
		return ACSD_ERR_NOT_ACTVS;

	if ((actfrm->OUI[0] != BCM_ACTION_OUI_BYTE0) ||
		(actfrm->OUI[1] != BCM_ACTION_OUI_BYTE1) ||
		(actfrm->OUI[2] != BCM_ACTION_OUI_BYTE2) ||
		(actfrm->type != BCM_ACTION_RFAWARE)) {
		ACSD_INFO("recved VS action frame, but not DCS request\n");
		return ACSD_ERR_NOT_DCSREQ;
	}

	bcopy(&actfrm->data[0], &reason, sizeof(uint32));
	dcs_data->reason = ltoh32(reason);
	bcopy(&actfrm->data[4], (uint8*)&chanspec, sizeof(chanspec_t));
	dcs_data->chspec = ltoh16(chanspec);

	ACSD_INFO("dcs_data: reason: %d, chanspec: 0x%4x\n", dcs_data->reason,
		(uint16)dcs_data->chspec);

	return ACSD_OK;
}

int
dcs_handle_request(char* ifname, wl_bcmdcs_data_t *dcs_data,
	uint8 mode, uint8 count, uint8 csa_mode)
{
	wl_chan_switch_t csa;
	int err = ACSD_OK;

	ACSD_INFO("ifname: %s, reason: %d, chanspec: 0x%x, csa:%x\n",
		ifname, dcs_data->reason, dcs_data->chspec, csa_mode);

	csa.mode = mode;
	csa.count = count;
	csa.chspec = dcs_data->chspec;
	csa.reg = 0;
	csa.frame_type = csa_mode;

	err = wl_iovar_set(ifname, "csa", &csa, sizeof(wl_chan_switch_t));

	return err;
}

#ifdef WL_MEDIA_ACSD
/*
 * This function will return a random channel number in the range (1,6,11)
 * and will be called only while handling events that would trigger DCS.
 */
void
dcs_select_rand_chspec(acs_chaninfo_t *c_info)
{
	bool rand_value = select_rand_value();

	if (c_info->cur_chspec == 0x1001) {
		c_info->selected_chspec = rand_value ? 0x1006 : 0x100b;
	} else if (c_info->cur_chspec == 0x1006) {
		c_info->selected_chspec = rand_value ? 0x1001 : 0x100b;
	} else {
		c_info->selected_chspec = rand_value ? 0x1001 : 0x1006;
	}

	ACSD_INFO("Selected random channel for DCS: 0x%x\n", c_info->selected_chspec);
	return;
}
#endif /* WL_MEDIA_ACSD */
