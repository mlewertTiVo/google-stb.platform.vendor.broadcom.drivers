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
 * $Id: wlc_ie_helper.h 665073 2016-10-14 20:33:29Z $
 */


#ifndef _wlc_ie_helper_h_
#define _wlc_ie_helper_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_ie_mgmt_types.h>

/*
 * 'calc_len/build' Frame Type specific structure decode accessors.
 */
extern wlc_bss_info_t *wlc_iem_calc_get_assocreq_target(wlc_iem_calc_data_t *calc);
extern wlc_bss_info_t *wlc_iem_build_get_assocreq_target(wlc_iem_build_data_t *build);
extern struct scb* wlc_iem_parse_get_assoc_bcn_scb(wlc_iem_parse_data_t *parse);

/* initialize user provided parse params */
void wlc_iem_parse_upp_init(wlc_iem_info_t *iem, wlc_iem_upp_t *upp);
/* initialize user IE list params */
void wlc_iem_build_uiel_init(wlc_iem_info_t *iem, wlc_iem_uiel_t *upp);

#endif /* _wlc_ie_helper_h_ */
