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
 * $Id: wlc_mux_utils.c 664252 2016-10-11 21:27:47Z $
 */

/**
 * @file
 * @brief Transmit Path MUX layer helpers/utilities
 */

#include <wlc_cfg.h>

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <wlc_types.h>
#include <siutils.h>
#include <wlc_rate.h>
#include <wlc.h>
#include <wlc_tx.h>
#include <wlc_mux.h>
#include <wlc_mux_utils.h>

/**
 * @brief Private state structure for the MUX and associated sources created by wlc_msrc_alloc().
 *
 * This structure ties together the mux, sources and the low txq fifos
 */
struct wlc_mux_srcs {
	/** @brief Back pointer to allocated muxes */
	wlc_mux_t**			mux_object;
	/** @brief Configured sources for this mux queue object */
	mux_source_handle_t	mux_source[NFIFO];
	/** @brief Number of configured fifos up to NFIFO */
	uint				mux_nfifo;
	uint16				mux_srcmap; /* Bit set if mux present */
	uint16				mux_srcstate; /* Bit set if MUX is runnning */
};

/**
 * @brief get number of mux sources
 *
 * @param msrc           pointer to mux sources
 */
uint
wlc_msrc_get_numsrcs(mux_srcs_t *msrc)
{
	ASSERT(msrc);
	return msrc->mux_srcstate;
}

/**
 * @brief get bitmap of enabled mux sources
 *
 * @param msrc           pointer to mux sources
 */
uint16
wlc_msrc_get_enabsrcs(mux_srcs_t *msrc)
{
	ASSERT(msrc);
	return (msrc->mux_srcstate | msrc->mux_srcmap);
}

/**
 * @brief Start source attached to a mux queue
 *
 * @param msrc           pointer to mux sources
 * @param src            source to start
 */
void BCMFASTPATH
wlc_msrc_start(mux_srcs_t *msrc, uint src)
{
	ASSERT(src < msrc->mux_nfifo);
	wlc_mux_source_start(MUX_GET(msrc->mux_object, src), msrc->mux_source[src]);
	msrc->mux_srcstate |= (1 << src);
}

/**
 * @brief Wake source attached to a mux queue
 *
 * @param msrc           pointer to mux sources
 * @param src            source to wake
 */
void BCMFASTPATH
wlc_msrc_wake(mux_srcs_t *msrc, uint src)
{
	ASSERT(src < msrc->mux_nfifo);
	wlc_mux_source_wake(MUX_GET(msrc->mux_object, src), msrc->mux_source[src]);
	msrc->mux_srcstate |= (1 << src);
}

/**
 * @brief Stop source attached to a mux queue
 *
 * @param msrc           pointer to mux sources
 * @param src            source to stop
 */
void
wlc_msrc_stop(mux_srcs_t *msrc, uint src)
{
	ASSERT(src < msrc->mux_nfifo);
	wlc_mux_source_stop(MUX_GET(msrc->mux_object, src), msrc->mux_source[src]);
	msrc->mux_srcstate &= ~(1 << src);
}
/**
 * @brief Start a group of sources attached to a mux queue
 *
 * @param msrc           pointer to mux sources
 * @param mux_grp_map    bitmap of mux srcs, if set, src is started
 */
void
wlc_msrc_group_start(mux_srcs_t *msrc, uint mux_grp_map)
{
	uint src;
	wlc_mux_t* mux_object;

	ASSERT(mux_grp_map < MUX_GRP_BITMAP(msrc->mux_nfifo));

	for (src = 0; src < msrc->mux_nfifo; src++) {
		mux_object = MUX_GET(msrc->mux_object, src);
		if (mux_object && MUX_GRP_SELECTED(src, mux_grp_map)) {
			wlc_msrc_start(msrc, src);
		}
	}
}

/**
 * @brief Wake a group of sources attached to a mux queue
 *
 * @param msrc           pointer to mux sources
 * @param mux_grp_map    bitmap of mux srcs, if set, src is woken
 */
void
wlc_msrc_group_wake(mux_srcs_t *msrc, uint mux_grp_map)
{
	uint src;
	wlc_mux_t* mux_object;

	ASSERT(mux_grp_map < MUX_GRP_BITMAP(msrc->mux_nfifo));

	for (src = 0; src < msrc->mux_nfifo; src++) {
		mux_object = MUX_GET(msrc->mux_object, src);
		if (mux_object && MUX_GRP_SELECTED(src, mux_grp_map)) {
			wlc_msrc_wake(msrc, src);
		}
	}
}

/**
 * @brief Generic function to move mux sources.
 *
 * @param wlc           Pointer to wlc info block
 * @param newmux        Pointer to mux control structure in new queue
 * @param oldmsrc       Pointer origination mux source structure.
 * @param ctx           Context pointer to be provided as a parameter to the output_fn
 * @param output_fn     The mux_output_fn_t the MUX will call to request packets from
 *                      this mux source
 *
 * @return              An error code. BCME_OK on success.
 */
int
wlc_msrc_move(wlc_info_t *wlc, wlc_mux_t **newmux,
		mux_srcs_t *oldmsrc, void *ctx, mux_output_fn_t output_fn)

{
	uint i;
	wlc_mux_t *oldmux_object, *newmux_object;
	int err = BCME_OK;
	uint entries = oldmsrc->mux_nfifo;

	/* Delete old mux sources once copy is successful */
	for (i = 0; (i < entries); i++) {
		oldmux_object = MUX_GET(oldmsrc->mux_object, i);
		if ((oldmux_object != NULL) && (oldmsrc->mux_source[i] != NULL)) {
			wlc_mux_del_source(oldmux_object, oldmsrc->mux_source[i]);

			if (newmux == NULL) {
				continue;
			}

			newmux_object = MUX_GET(newmux, i);
			err = wlc_mux_add_source(newmux_object,
				ctx, output_fn, &oldmsrc->mux_source[i]);
			if (err) {
				WL_ERROR(("wl%d:%s wlc_msrc_move() failed"
					" for FIFO%d err = %d\n",
					wlc->pub->unit, __FUNCTION__, i,	err));
				return (err);
			}
		}
	}

	oldmsrc->mux_object = newmux;

	return (err);
}

/**
 * @brief Stop a group of fifos attached to a mux queue
 *
 * @param msrc           pointer to mux sources
 * @param mux_grp_map    bitmap of mux srcs, if set, src is stopped
 */
void
wlc_msrc_group_stop(mux_srcs_t *msrc, uint mux_grp_map)
{
	uint src;
	wlc_mux_t* mux_object;

	ASSERT(mux_grp_map < MUX_GRP_BITMAP(msrc->mux_nfifo));

	for (src = 0; src < msrc->mux_nfifo; src++) {
		mux_object = MUX_GET(msrc->mux_object, src);
		if (mux_object && MUX_GRP_SELECTED(src, mux_grp_map)) {
			wlc_msrc_stop(msrc, src);
		}
	}
}

/**
 * @brief Generic function to delete mux sources.
 *
 * @param wlc           Pointer to wlc info block
 * @param msrc          Pointer origination mux source structure.
 */
void
wlc_msrc_delsrc(wlc_info_t *wlc, mux_srcs_t *msrc)
{
	uint i;
	uint entries = msrc->mux_nfifo;
	wlc_mux_t *mux_object;

	for (i = 0; (i < entries); i++) {
		mux_object = MUX_GET(msrc->mux_object, i);
		if (MUX_GRP_SELECTED(i, msrc->mux_srcmap) && msrc->mux_source[i])
		{
			ASSERT(mux_object);
			wlc_mux_del_source(mux_object, msrc->mux_source[i]);
			msrc->mux_source[i] = NULL;
		}
	}

}
/**
 * @brief Generic function to recreate mux sources.
 *
 * @param wlc           Pointer to wlc info block
 * @param newmux        Pointer to mux control structure in new queue
 * @param msrc          Pointer origination mux source structure.
 * @param ctx           Context pointer to be provided as a parameter to the output_fn
 * @param output_fn     The mux_output_fn_t the MUX will call to request packets from
 *                      this mux source
 *
 * @return              An error code. BCME_OK on success.
 */
int
wlc_msrc_recreatesrc(wlc_info_t *wlc, wlc_mux_t **newmux, mux_srcs_t *msrc,
		void *ctx, mux_output_fn_t output_fn)
{
	uint i;
	uint entries = msrc->mux_nfifo;
	wlc_mux_t *mux_object;
	int err = BCME_OK;

	for (i = 0; (i < entries); i++) {
		mux_object = MUX_GET(newmux, i);
		if (MUX_GRP_SELECTED(i, msrc->mux_srcmap))
		{
			ASSERT(mux_object);
			err = wlc_mux_add_source(mux_object,
				ctx, output_fn, &msrc->mux_source[i]);
			if (err) {
				return (err);
			}
		}
	}
	msrc->mux_object = newmux;
	return (err);
}

/**
 * @brief Generic function to deallocates mux sources.
 *
 * @param wlc     Pointer to wlc block
 * @param osl     Pointer to port layer osl routines.
 * @param srcs    Pointer mux source structure. Freed by this function.
 */
void
wlc_msrc_free(wlc_info_t *wlc, osl_t *osh, mux_srcs_t *msrc)
{
	uint i;
	uint entries;
	BCM_REFERENCE(wlc);

	entries = msrc->mux_nfifo;

	/* Free  sources and MUXs */
	for (i = 0; i < entries; i++) {
		if (msrc->mux_source[i] != NULL) {
			if ((MUX_GET(msrc->mux_object, i)) != NULL) {
				wlc_mux_del_source(MUX_GET(msrc->mux_object, i),
					msrc->mux_source[i]);
			} else {
				/* Something is wrong if the source is present for a null mux */
				ASSERT(0);
			}
		}
	}

	MFREE(osh, msrc, sizeof(mux_srcs_t));
}

/**
 * @brief Allocate and initialize mux sources
 *
 * @param wlc            pointer to wlc_info_t
 * @param mux            pointer to list of muxes to attach sources
 * @param nfifo          number of fifos in low txq
 * @param ctx            pointer to context for mux source output function
 * @param output_fn      mux output function
 * @param mux_grp_map    bitmap of mux srcs, if set, src is alloc/init
 *
 * @return               mux_srcs_t containing mux sources
 */
mux_srcs_t *
wlc_msrc_alloc(wlc_info_t *wlc, wlc_mux_t** mux, uint nfifo, void *ctx,
	mux_output_fn_t output_fn, uint mux_grp_map)
{
	mux_srcs_t *msrc;
	osl_t *osh = wlc->osh;
	uint i;
	int err = 0;
	wlc_mux_t* mux_object;

	/* ASSERT if sources bit map specifies fifo above configured nfifo */
	ASSERT(mux_grp_map  < MUX_GRP_BITMAP(nfifo));

	msrc = (mux_srcs_t*)MALLOCZ(osh, sizeof(mux_srcs_t));
	if (msrc == NULL) {
		return NULL;
	}

	msrc->mux_object = mux;
	msrc->mux_nfifo = nfifo;

	/* Allocate mux sources and attach the handler */
	for (i = 0; i < nfifo; i++) {
		mux_object = MUX_GET(mux, i);
		if (mux_object && MUX_GRP_SELECTED(i, mux_grp_map)) {
			err = wlc_mux_add_source(mux_object, ctx, output_fn, &msrc->mux_source[i]);
			if (err) {
				WL_ERROR(("wl%d:%s wlc_mux_add_member() failed"
				" for FIFO%d err = %d\n", wlc->pub->unit, __FUNCTION__, i,  err));
				wlc_msrc_free(wlc, osh, msrc);
				msrc = NULL;
				break;
			} else {
				MUX_GRP_SET(i, msrc->mux_srcmap);
			}
		}
	}

	ASSERT(msrc);
	return (msrc);
}
