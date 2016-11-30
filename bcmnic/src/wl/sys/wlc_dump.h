/**
 * @file
 * @brief
 * Named dump callback registration for WLC (excluding BMAC)
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
 * $Id: wlc_dump.h 614820 2016-01-23 17:16:17Z $
 */

#ifndef _wlc_dump_h_
#define _wlc_dump_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_dump_reg.h>

/* register dump and/or dump clear callbacks */
typedef wlc_dump_reg_dump_fn_t dump_fn_t;
typedef wlc_dump_reg_clr_fn_t clr_fn_t;
int wlc_dump_add_fns(wlc_pub_t *pub, const char *name,
	dump_fn_t dump_fn, clr_fn_t clr_fn, void *ctx);

/* API to invoke the local dump */
int
wlc_dump_local(wlc_dump_info_t *dumpi,
	char * name, int dump_len);

#define wlc_dump_register(pub, name, fn, ctx) \
	wlc_dump_add_fns(pub, name, fn, NULL, ctx)

/* early attach/late detach for as many others to use the dump module */
wlc_dump_info_t *wlc_dump_pre_attach(wlc_info_t *wlc);
void wlc_dump_post_detach(wlc_dump_info_t *dumpi);

/* attach/detach as a wlc module */
int wlc_dump_attach(wlc_dump_info_t *dumpi);
void wlc_dump_detach(wlc_dump_info_t *dumpi);

#endif /* _wlc_dump_h_ */
