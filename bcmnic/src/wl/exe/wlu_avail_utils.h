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
 * $Id: wlu_avail_utils.h abhik$
 */
#ifndef __WLU_AVAIL_UTILS_H___
#define __WLU_AVAIL_UTILS_H__
int
wl_avail_parse_slot(char *arg_slot, wl_avail_slot_t *out_avail_slot);
int
wl_avail_parse_period(char *arg_slot, wl_time_interval_t *out_intvl);
int
wl_avail_validate_format(char *arg_slot);
#endif /* __WLU_AVAIL_UTILS_H___ */
