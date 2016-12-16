/*
 * wl leakyapstats command module
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
 * $Id:$
 */

#include <wlioctl.h>

#include <bcmutils.h>
#include "wlu_common.h"
#include "wlu.h"

static cmd_t wl_leakyapstats_cmds[] = {
	{ "leaky_ap_stats", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"Enable/disable leaky ap stats collection and reporting\n"
	"\t0 - disable\n"
	"\t1 - enable" },
	{ "leaky_ap_sep", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"Enable/disable leaky ap per packet report suppresion\n"
	"\t0 - disable suppression\n"
	"\t1 - enable suppression" },
	{ NULL, NULL, 0, 0, NULL }
};

/* module initialization */
void
wluc_leakyapstats_module_init(void)
{
	/* register leaky_ap_stats commands */
	wl_module_cmds_register(wl_leakyapstats_cmds);
}
