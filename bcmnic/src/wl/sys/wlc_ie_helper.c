/*
 * IE management callback data structure decode utilities
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
 * $Id: wlc_ie_helper.c 665073 2016-10-14 20:33:29Z $
 */

/**
 * @file
 * @brief
 * Hard-coding IEs in management frames by calling out each module's IE build function is less
 * desirable for the following reasons:
 *     - it is costly to patch a ROM'ed management frame generation function when adding new
 *       features that required new IEs
 *     - it is hard to maintain the right order of the IEs presented without knowing all different
 *       features requirements
 *     - it forces other modules to export IE build/parse functions and/or data structures
 *     - it adds IE lookup overhead for each module to find their own IEs
 * The IE management module is designed to address above issues by providing:
 *     - callback registration to decouple management frames' build/parse function and participating
 *       modules and to provide future extension
 *     - IE tags table based on the specification to enforce the IE order in a central place
 *     - IE parser to reduce lookup overhead
 */


#include <wlc_cfg.h>
#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_ie_mgmt_types.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_helper.h>

/* Accessors */

/*
 * 'calc_len' Frame Type specific parameter structure decode accessors.
 */
wlc_bss_info_t *
wlc_iem_calc_get_assocreq_target(wlc_iem_calc_data_t *calc)
{
	switch (calc->ft) {
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ: {
		wlc_iem_cbparm_t *cbparm = calc->cbparm;
		wlc_iem_ft_cbparm_t *ftcbparm;

		ASSERT(cbparm != NULL);
		ASSERT(cbparm->ft != NULL);

		if (cbparm != NULL && (ftcbparm = cbparm->ft) != NULL)
			return ftcbparm->assocreq.target;
		break;
	}
	default:
		ASSERT(0);
		break;
	}

	return NULL;
}

/*
 * 'build' Frame Type specific parameter structure decode accessors.
 */
wlc_bss_info_t *
wlc_iem_build_get_assocreq_target(wlc_iem_build_data_t *build)
{
	switch (build->ft) {
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ: {
		wlc_iem_cbparm_t *cbparm = build->cbparm;
		wlc_iem_ft_cbparm_t *ftcbparm;

		ASSERT(cbparm != NULL);
		ASSERT(cbparm->ft != NULL);

		if (cbparm != NULL && (ftcbparm = cbparm->ft) != NULL)
			return ftcbparm->assocreq.target;
		break;
	}
	default:
		ASSERT(0);
		break;
	}

	return NULL;
}

/*
 * 'parse' Frame Type specific scb structure decode accessors.
 */
struct scb*
wlc_iem_parse_get_assoc_bcn_scb(wlc_iem_parse_data_t *parse)
{
	wlc_iem_ft_pparm_t *ftpparm;
	struct scb *scb;

	switch (parse->ft) {
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ:
		ASSERT(parse->pparm != NULL);
		ftpparm = parse->pparm->ft;
		ASSERT(ftpparm != NULL);
		scb = ftpparm->assocreq.scb;
		break;
	case FC_ASSOC_RESP:
	case FC_REASSOC_RESP:
		ASSERT(parse->pparm != NULL);
		ftpparm = parse->pparm->ft;
		ASSERT(ftpparm != NULL);
		scb = ftpparm->assocresp.scb;
		break;
	case FC_BEACON:
		ASSERT(parse->pparm != NULL);
		ftpparm = parse->pparm->ft;
		ASSERT(ftpparm != NULL);
		scb = ftpparm->bcn.scb;
		break;
	default:
		scb = NULL;
		break;
	}

	return scb;
}

static void
wlc_iem_parse_nhdlr_cb(void *ctx, wlc_iem_nhdlr_data_t *data)
{
	BCM_REFERENCE(ctx);

	if (0) {
		prhex("no parser", data->ie, data->ie_len);
	}
}

static wlc_iem_tag_t
wlc_iem_parse_vsie_cb(void *ctx, wlc_iem_pvsie_data_t *data)
{
	return wlc_iem_vs_get_id(data->ie);
}

/* initialize user provided parse structure.
 * caller can override the entire structure if needed.
 */
void
wlc_iem_parse_upp_init(wlc_iem_info_t *iem, wlc_iem_upp_t *upp)
{
	bzero(upp, sizeof(*upp));

	upp->notif_fn = wlc_iem_parse_nhdlr_cb;
	upp->vsie_fn = wlc_iem_parse_vsie_cb;
	upp->ctx = iem;
}

static wlc_iem_tag_t
wlc_iem_cbvsie_cb(void *ctx, wlc_iem_cbvsie_data_t *data)
{
	return wlc_iem_vs_get_id(data->ie);
}

/* initialize user IE list params - likely common portion only.
 * caller can override the structure if needed.
 */
void
wlc_iem_build_uiel_init(wlc_iem_info_t *iem, wlc_iem_uiel_t *uiel)
{
	bzero(uiel, sizeof(*uiel));

	uiel->vsie_fn = wlc_iem_cbvsie_cb;
	uiel->ctx = iem;
}
