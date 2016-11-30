/*
 * 802.11h DFS module header file
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
 * $Id: wlc_dfs.h 633968 2016-04-26 09:01:47Z $
 */


#ifndef _wlc_dfs_h_
#define _wlc_dfs_h_

/* check if core revision supports background DFS scan core */
#define DFS_HAS_BACKGROUND_SCAN_CORE(wlc) D11REV_GE((wlc)->pub->corerev, 0x40)


/* radar subband information is available only for 4365/4366 corerev 0x40 (<=b1) and 0x41 (c0) */
#define DFS_HAS_SUBBAND_INFO(wlc) (D11REV_IS(wlc->pub->corerev, 0x40) || \
		D11REV_IS(wlc->pub->corerev, 0x41))

#ifdef WLDFS

/* module */
extern wlc_dfs_info_t *wlc_dfs_attach(wlc_info_t *wlc);
extern void wlc_dfs_detach(wlc_dfs_info_t *dfs);

/* others */
extern void wlc_set_dfs_cacstate(wlc_dfs_info_t *dfs, int state, wlc_bsscfg_t *cfg);
extern chanspec_t wlc_dfs_sel_chspec(wlc_dfs_info_t *dfs, bool force, wlc_bsscfg_t *cfg);
extern void wlc_dfs_reset_all(wlc_dfs_info_t *dfs);
extern int wlc_dfs_set_radar(wlc_dfs_info_t *dfs, int radar);

extern bool wlc_dfs_valid_ap_chanspec(wlc_info_t *wlc, chanspec_t chspec);
/* accessors */
extern uint32 wlc_dfs_get_radar(wlc_dfs_info_t *dfs);

extern uint32 wlc_dfs_get_chan_info(wlc_dfs_info_t *dfs, uint channel);

#else /* !WLDFS */

#define wlc_dfs_attach(wlc) NULL
#define wlc_dfs_detach(dfs) do {} while (0)

#define wlc_set_dfs_cacstate(dfs, state, cfg) do {} while (0)
#define wlc_dfs_sel_chspec(dfs, force, cfg) 0
#define wlc_dfs_reset_all(dfs) do {} while (0)
#define wlc_dfs_set_radar(dfs, radar)  BCME_UNSUPPORTED
#define wlc_dfs_valid_ap_chanspec(wlc, chspec) (TRUE)

#define wlc_dfs_get_radar(dfs) 0
#define wlc_dfs_get_chan_info(dfs, channel) 0

#endif /* !WLDFS */

#if defined(WLDFS) && (defined(RSDB_DFS_SCAN) || defined(BGDFS))
extern int wlc_dfs_scan_in_progress(wlc_dfs_info_t *dfs);
extern void wlc_dfs_scan_abort(wlc_dfs_info_t *dfs);
#else
#define wlc_dfs_scan_in_progress(dfs) 0
#define wlc_dfs_scan_abort(dfs) do {} while (0)
#endif /* WLDFS && (RSDB_DFS_SCAN || BGDFS) */

#endif /* _wlc_dfs_h_ */
