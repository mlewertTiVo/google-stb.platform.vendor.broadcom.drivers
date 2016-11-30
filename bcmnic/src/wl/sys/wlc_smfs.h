/*
 * Selected Management Frame Stats feature interface
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
 * $Id: wlc_smfs.h 599296 2015-11-13 06:36:13Z $
 */

#ifndef _wlc_smfs_h_
#define _wlc_smfs_h_

#include <typedefs.h>
#include <wlc_types.h>

/* Per-BSS FEATURE_ENAB() macro */
#ifdef SMF_STATS
#define BSS_SMFS_ENAB(wlc, cfg) SMFS_ENAB((wlc)->pub) && wlc_smfs_enab((wlc)->smfs, cfg)
#else
#define BSS_SMFS_ENAB(wlc, cfg) FALSE
#endif /* SMF_STATS */

/* Module attach/detach interface */
wlc_smfs_info_t *wlc_smfs_attach(wlc_info_t *wlc);
void wlc_smfs_detach(wlc_smfs_info_t *smfs);

/* Functional interfaces */

#ifdef SMF_STATS

/* Check if the feature is enabled */
bool wlc_smfs_enab(wlc_smfs_info_t *smfs, wlc_bsscfg_t *cfg);

/* Update the stats.
 *
 * Input Params
 * - 'type' is defined in wlioctl.h. See smfs_type enum.
 * - 'code' is defined in wlioctl.h. See SMFS_CODETYPE_XXX.
 *
 * Return BCME_XXXX.
 */
int wlc_smfs_update(wlc_smfs_info_t *smfs, wlc_bsscfg_t *cfg, uint8 type, uint16 code);

#else /* !SMF_STATS */

/* For compilers that don't eliminate unused code */

#define wlc_smfs_enab(smfs, cfg) FALSE

static int wlc_smfs_update(wlc_smfs_info_t *smfs, wlc_bsscfg_t *cfg, uint8 type, uint16 code)
{
	BCM_REFERENCE(smfs);
	BCM_REFERENCE(cfg);
	BCM_REFERENCE(type);
	BCM_REFERENCE(code);
	return BCME_OK;
}

#endif /* !SMF_STATS */

#endif	/* _wlc_smfs_h_ */
