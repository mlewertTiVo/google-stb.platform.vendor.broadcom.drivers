/*
 * Named dump callback registry functions
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
 * $Id: wlc_dump_reg.h 613675 2016-01-19 19:24:39Z $
 */

#ifndef _wlc_dump_reg_h_
#define _wlc_dump_reg_h_

#include <typedefs.h>
#include <osl_decl.h>
#include <bcmutils.h>

/* forward declarataion */
typedef struct wlc_dump_reg_info wlc_dump_reg_info_t;

/* create a registry with 'count' entries */
wlc_dump_reg_info_t *wlc_dump_reg_create(osl_t *osh, uint16 count);

/* destroy a registry */
void wlc_dump_reg_destroy(wlc_dump_reg_info_t *reg);

/* dump callback function of registry */
typedef int (*wlc_dump_reg_dump_fn_t)(void *ctx, struct bcmstrbuf *b);
/* dump clear callback function of registry */
typedef int (*wlc_dump_reg_clr_fn_t)(void *ctx);
#define wlc_dump_reg_fn_t wlc_dump_reg_dump_fn_t

/* add a name and its callback functions to a registry */
int wlc_dump_reg_add_fns(wlc_dump_reg_info_t *reg, const char *name,
	wlc_dump_reg_dump_fn_t dump_fn, wlc_dump_reg_clr_fn_t clr_fn, void *ctx);
#define wlc_dump_reg_add_fn(reg, name, fn, ctx) \
	wlc_dump_reg_add_fns(reg, name, fn, NULL, ctx)

/* invoke a dump callback function in a registry by name */
int wlc_dump_reg_invoke_dump_fn(wlc_dump_reg_info_t *reg, const char *name,
	void *arg);
#define wlc_dump_reg_invoke_fn(reg, name, arg) \
	wlc_dump_reg_invoke_dump_fn(reg, name, arg)

/* invoke a dump clear callback function in a registry by name */
int wlc_dump_reg_invoke_clr_fn(wlc_dump_reg_info_t *reg, const char *name);

/* dump the registered names */
int wlc_dump_reg_list(wlc_dump_reg_info_t *reg, struct bcmstrbuf *b);
/* dump the registry internals */
int wlc_dump_reg_dump(wlc_dump_reg_info_t *reg, struct bcmstrbuf *b);

#endif /* _wlc_dump_reg_h_ */
