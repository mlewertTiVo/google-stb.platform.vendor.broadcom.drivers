/*
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
 * $Id: wlc_mux_utils.h 664252 2016-10-11 21:27:47Z $
 */

/**
 * @file
 * @brief Transmit Path MUX layer helpers/utilities
 */

#ifndef _wlc_mux_utils_h_
#define _wlc_mux_utils_h_

#define MUX_GRP_BITMAP(fifo)	((uint16)(1 << (fifo)))
#define MUX_GRP_AC_BK	(MUX_GRP_BITMAP(TX_AC_BK_FIFO))
#define MUX_GRP_AC_BE	(MUX_GRP_BITMAP(TX_AC_BE_FIFO))
#define MUX_GRP_AC_VI	(MUX_GRP_BITMAP(TX_AC_VI_FIFO))
#define MUX_GRP_AC_VO	(MUX_GRP_BITMAP(TX_AC_VO_FIFO))
#define MUX_GRP_BCMC	(MUX_GRP_BITMAP(TX_BCMC_FIFO))
#define MUX_GRP_ATIM	(MUX_GRP_BITMAP(TX_ATIM_FIFO))
#define MUX_GRP_DATA	\
	((MUX_GRP_AC_BK)|(MUX_GRP_AC_BE)|(MUX_GRP_AC_VI)|(MUX_GRP_AC_VO))

#define MUX_SRC_AC_BK	TX_AC_BK_FIFO
#define MUX_SRC_AC_BE	TX_AC_BE_FIFO
#define MUX_SRC_AC_VI	TX_AC_VI_FIFO
#define MUX_SRC_AC_VO	TX_AC_VO_FIFO
#define MUX_SRC_BCMC	TX_BCMC_FIFO
#define MUX_SRC_ATIM	TX_ATIM_FIFO

/* Convenience accessor macros */
#define MUX_GET(mux, fifo) ((mux)[(fifo)])
#define MUX_GRP_SELECTED(fifo, map) (MUX_GRP_BITMAP(fifo) & (map))
#define MUX_GRP_SET(fifo, map) ((map) |= MUX_GRP_BITMAP(fifo))

typedef struct wlc_mux_srcs mux_srcs_t;

/**
 * @brief get number of mux sources
 */
uint wlc_msrc_get_numsrcs(mux_srcs_t *msrc);

/**
 * @brief get bitmap of enabled mux sources
 */
uint16 wlc_msrc_get_enabsrcs(mux_srcs_t *msrc);

/**
 * @brief Start source attached to a mux queue
 */
void wlc_msrc_start(mux_srcs_t *msrcs, uint src);

/**
 * @brief Wake source attached to a mux queue
 */
void wlc_msrc_wake(mux_srcs_t *msrcs, uint src);

/**
 * @brief Stop source attached to a mux queue
 */
void wlc_msrc_stop(mux_srcs_t *msrcs, uint src);

/**
 * @brief Start a group of sources attached to a mux queue
 */
void wlc_msrc_group_start(mux_srcs_t *msrcs, uint mux_grp_map);

/**
 * @brief Wake a group of sources attached to a mux queue
 */
void wlc_msrc_group_wake(mux_srcs_t *msrcs, uint mux_grp_map);

/**
 * @brief Stop a group of sources attached to a mux queue
 */
void wlc_msrc_group_stop(mux_srcs_t *msrcs, uint mux_grp_map);

/**
 * @brief Generic function to move mux sources.
 */

int wlc_msrc_move(wlc_info_t *wlc, wlc_mux_t **newmux,
	mux_srcs_t *oldmsrc, void *ctx, mux_output_fn_t output_fn);

/**
 * @brief Generic function to delete mux sources.
 */

void wlc_msrc_delsrc(wlc_info_t *wlc, mux_srcs_t *msrc);

/**
 * @brief Generic function to recreate mux sources.
 */

int wlc_msrc_recreatesrc(wlc_info_t *wlc, wlc_mux_t **newmux, mux_srcs_t *msrc,
		void *ctx, mux_output_fn_t output_fn);

/**
 * @brief Allocate and initialize mux sources
 */
mux_srcs_t *wlc_msrc_alloc(wlc_info_t *wlc, wlc_mux_t** mux, uint nfifo,
	void *ctx, mux_output_fn_t output_fn, uint mux_grp_map);

/**
 * @brief Free all resources of the mux sources
 */
void wlc_msrc_free(wlc_info_t *wlc, osl_t *osh, mux_srcs_t *srcs);

#endif /* _wlc_mux_utils_h_ */
