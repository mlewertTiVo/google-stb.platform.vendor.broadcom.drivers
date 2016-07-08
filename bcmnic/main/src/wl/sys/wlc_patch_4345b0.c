/**
 * @file
 * @brief
 * Chip specific software patches for ROM functions. These are used as a memory optimization for
 * invalidated ROM functions. The patch functions execute a relatively small amount of code in
 * order to correct the behavior of ROM functions and then call the original ROM function to
 * perform the majority of the work. For example, a ROM function could be corrected by running a
 * small amount of new code before the call to the ROM function, after the call to the ROM function,
 * or both before and after.
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

#include <wlc_patch.h>


#if defined(EXAMPLE_PATCH)

#include <bcm_mpool_pub.h>
int CALLROM_ENTRY(bcm_mp_stats)(bcm_mp_pool_h pool, bcm_mp_stats_t *stats);
int bcm_mp_stats(bcm_mp_pool_h pool, bcm_mp_stats_t *stats)
{
	printf("PATCH: %s\n", __FUNCTION__);
	return CALLROM_ENTRY(bcm_mp_stats)(pool, stats);
}

int CALLROM_ENTRY(bcm_mpm_stats)(bcm_mpm_mgr_h mgr, bcm_mp_stats_t *stats, int *nentries);
int bcm_mpm_stats(bcm_mpm_mgr_h mgr, bcm_mp_stats_t *stats, int *nentries)
{
	printf("PATCH: %s\n", __FUNCTION__);
	return CALLROM_ENTRY(bcm_mpm_stats)(mgr, stats, nentries);
}

#endif /* EXAMPLE_PATCH */
