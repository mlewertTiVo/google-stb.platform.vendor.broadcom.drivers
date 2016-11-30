/*
 * Common cubby control library interface.
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
 * $Id: wlc_cubby.h 629717 2016-04-06 08:32:16Z $
 */

#ifndef _wlc_cubby_h_
#define _wlc_cubby_h_


/**
 * @file @brief
 * Used by modules such as bsscfg and scb to provide cubby (feature specific
 * private data storage) control, and to provide secondary cubby (cubby is
 * a pointer, it points to another memory block) control and storage.
 *
 * Modules use the following interface to interact with this library:
 * 1. Use wlc_cubby_reserve() interface to register cubby size and
 *    client callbacks.
 * 2. Use wlc_cubby_init()/wlc_cubby_deinit()/wlc_cubby_dump() to invoke
 *    client callbacks when initializing/deinitializing/dumping cubby
 *    contents.
 *
 * This library uses the following client supplied callbacks to communicate
 * information back to clients:
 * 1. Use cubby_sec_set_fn_t callback to inform the client of the allocated
 *    memory for the secondary cubbies. The client must save this information.
 * 2. Use cubby_sec_get_fn_t callback to query the client of the secondary
 *    cubby memory. The secondary cubby memory is allocated once by this
 *    library for all participating clients; it's freed also by this libray.
 */

#include <typedefs.h>
#include <bcmutils.h>
#include <wlc_types.h>
#include <wlc_objregistry.h>

typedef struct wlc_cubby_info wlc_cubby_info_t;

/* module attach/detach interface */
wlc_cubby_info_t *wlc_cubby_attach(osl_t *osh, uint unit, wlc_obj_registry_t *objr,
                                   obj_registry_key_t key, uint num);
void wlc_cubby_detach(wlc_cubby_info_t *cubby_info);

/* cubby callback functions */
typedef int (*cubby_init_fn_t)(void *ctx, void *obj);
typedef void (*cubby_deinit_fn_t)(void *ctx, void *obj);
typedef uint (*cubby_secsz_fn_t)(void *ctx, void *obj);
typedef void (*cubby_dump_fn_t)(void *ctx, void *obj, struct bcmstrbuf *b);
typedef void (*cubby_datapath_log_dump_fn_t)(void *, struct scb *, int);

/** structure for registering per-cubby client info */
/* Note: if cubby init/deinit callbacks are invoked directly in other parts of
 * the code explicitly don't register fn_secsz callback and don't use secondary cubby
 * alloc/free interfaces.
 */
typedef struct wlc_cubby_fn {
	cubby_init_fn_t	fn_init;	/**< fn called during object alloc */
	cubby_deinit_fn_t fn_deinit;	/**< fn called during object free */
	cubby_secsz_fn_t fn_secsz;	/**< fn called during object alloc and free,
					 * optional and registered when the secondary cubby
					 * allocation is expected
					 */
	cubby_dump_fn_t fn_dump;	/**< fn called during object dump - BCMDBG only */
	cubby_datapath_log_dump_fn_t fn_data_log_dump; /**< EVENT_LOG dump */
	const char *name;
} wlc_cubby_fn_t;

/* cubby copy callback functions */
typedef int (*cubby_get_fn_t)(void *ctx, void *obj, uint8 *data, int *len);
typedef int (*cubby_set_fn_t)(void *ctx, void *obj, const uint8 *data, int len);

/* structure for registering per-cubby client info for cubby copy */
typedef struct wlc_cubby_cp_fn {
	cubby_get_fn_t fn_get;		/* fn called to retrieve cubby content */
	cubby_set_fn_t fn_set;		/* fn called to write content to cubby */
} wlc_cubby_cp_fn_t;

/* client registration interface */
int wlc_cubby_reserve(wlc_cubby_info_t *cubby_info, uint size, wlc_cubby_fn_t *fn,
	uint cp_size, wlc_cubby_cp_fn_t *cp_fn, void *ctx);

/* secondary cubby callback functions */
typedef uint (*cubby_sec_sz_fn_t)(void *ctx, void *obj);
typedef void (*cubby_sec_set_fn_t)(void *ctx, void *obj, void *base);
typedef void *(*cubby_sec_get_fn_t)(void *ctx, void *obj);

/* init/deinit all cubbies */
int wlc_cubby_init(wlc_cubby_info_t *cubby_info, void *obj,
	cubby_sec_sz_fn_t secsz_fn, cubby_sec_set_fn_t fn, void *set_ctx);
void wlc_cubby_deinit(wlc_cubby_info_t *cubby_info, void *obj,
	cubby_sec_sz_fn_t secsz_fn, cubby_sec_get_fn_t fn, void *sec_ctx);

/* query total cubby size */
uint wlc_cubby_totsize(wlc_cubby_info_t *cubby_info);
/* query total secondary cubby size */
uint wlc_cubby_sec_totsize(wlc_cubby_info_t *cubby_info, void *obj);

/* copy all cubbies */
int wlc_cubby_copy(wlc_cubby_info_t *from_info, void *from_obj,
	wlc_cubby_info_t *to_info, void *to_obj);

/* secondary cubby alloc/free */
void *wlc_cubby_sec_alloc(wlc_cubby_info_t *cubby_info, void *obj, uint secsz);
void wlc_cubby_sec_free(wlc_cubby_info_t *cubby_info, void *obj, void *secptr);

/* debug/dump interface */

void wlc_cubby_datapath_log_dump(wlc_cubby_info_t *cubby_info, void *obj, int tag);

#endif /* _wlc_cubby_h_ */
