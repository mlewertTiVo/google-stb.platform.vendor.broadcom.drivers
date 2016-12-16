/*
 * Availability support functions
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef _WLU_SUBCOUNTERS_H_
#define _WLU_SUBCOUNTERS_H_
#define MAX_SUBCOUNTER_SUPPORTED	64 /* Max subcounters supported. */

cmd_func_t wl_subcounters;
cmd_func_t wl_counters, wl_if_counters, wl_swdiv_stats, wl_delta_stats;
cmd_func_t wl_clear_counters;

#endif /* _WLU_SUBCOUNTERS_H_ */
