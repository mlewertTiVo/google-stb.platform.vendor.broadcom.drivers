/*
 * Common cubby manipulation library
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_cubby.c 599296 2015-11-13 06:36:13Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_objregistry.h>
#include <wlc_cubby.h>

/* cubby callbacbk functions */
typedef struct cubby_fn {
	cubby_init_fn_t	fn_init;	/**< fn called during object alloc */
	cubby_deinit_fn_t fn_deinit;	/**< fn called during object free */
	cubby_secsz_fn_t fn_secsz;	/**< fn called during object alloc and free,
					 * optional and registered when the secondary cubby
					 * allocation is expected
					 */
	cubby_get_fn_t fn_get;		/**< fn called during object copy */
	cubby_set_fn_t fn_set;
} cubby_fn_t;


typedef struct cubby_dbg cubby_dbg_t;

/* cubby info allocated as a handle */
struct wlc_cubby_info {
	/* info about other parts of the system */
	wlc_info_t	*wlc;
	obj_registry_key_t key;
	/* client info registry */
	uint16		totsize;
	uint16		cp_max_len;
	uint16		ncubby;		/**< index of first available cubby */
	uint16 		ncubbies;	/**< total # of cubbies allocated */
	cubby_fn_t	*cubby_fn;	/**< cubby client callback funcs */
	void		**cubby_ctx;	/**< cubby client callback context */
	cubby_dbg_t	*cubby_dbg;
	/* transient states while doing init/deinit */
	uint16		secsz;		/* secondary allocation total size */
	uint16		secoff;		/* secondary allocation offset (allocated) */
	uint8		*secbase;
};

/* client info allocation size (client callbacks + debug info) */
#define CUBBY_FN_ALLOC_SZ(info, cubbies) \
	(uint)(sizeof(*((info)->cubby_fn)) * (cubbies))

/* cubby info allocation size (cubby info + client contexts) */
#define CUBBY_INFO_ALLOC_SZ(info, cubbies) \
	(uint)(sizeof(*(info)) + \
	       sizeof(*((info)->cubby_ctx)) * (cubbies))

/* debug macro */
#define WL_CUBBY(x)

/* attach/detach */
wlc_cubby_info_t *
BCMATTACHFN(wlc_cubby_attach)(wlc_info_t *wlc, obj_registry_key_t key, uint num)
{
	wlc_cubby_info_t *cubby_info;

	/* cubby info object */
	if ((cubby_info =
	     MALLOCZ(wlc->osh, CUBBY_INFO_ALLOC_SZ(cubby_info, num))) == NULL) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
		          CUBBY_INFO_ALLOC_SZ(cubby_info, num), MALLOCED(wlc->osh)));
		goto fail;
	}
	cubby_info->wlc = wlc;
	cubby_info->key = key;

	/* cubby_ctx was allocated along with cubby_info */
	cubby_info->cubby_ctx = (void **)&cubby_info[1];

	/* OBJECT REGISTRY: check if shared key has value already stored */
	if ((cubby_info->cubby_fn = obj_registry_get(wlc->objr, key)) == NULL) {
		if ((cubby_info->cubby_fn =
		     MALLOCZ(wlc->osh, CUBBY_FN_ALLOC_SZ(cubby_info, num))) == NULL) {
			WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			          CUBBY_FN_ALLOC_SZ(cubby_info, num), MALLOCED(wlc->osh)));
			goto fail;
		}
		/* OBJECT REGISTRY: We are the first instance, store value for key */
		obj_registry_set(wlc->objr, key, cubby_info->cubby_fn);
	}
	(void)obj_registry_ref(wlc->objr, key);


	cubby_info->ncubbies = (uint16)num;

	return cubby_info;

fail:
	wlc_cubby_detach(wlc, cubby_info);
	return NULL;
}

void
BCMATTACHFN(wlc_cubby_detach)(wlc_info_t *wlc, wlc_cubby_info_t *cubby_info)
{
	if (cubby_info == NULL) {
		return;
	}

	if (cubby_info->cubby_fn != NULL &&
	    (obj_registry_unref(wlc->objr, cubby_info->key) == 0)) {
		MFREE(wlc->osh, cubby_info->cubby_fn,
		      CUBBY_FN_ALLOC_SZ(cubby_info, cubby_info->ncubbies));
		obj_registry_set(wlc->objr, cubby_info->key, NULL);
	}

	MFREE(wlc->osh, cubby_info,
	      CUBBY_INFO_ALLOC_SZ(cubby_info, cubby_info->ncubbies));
}

/* client registration interface */
int
BCMATTACHFN(wlc_cubby_reserve)(wlc_cubby_info_t *cubby_info, uint size, wlc_cubby_fn_t *fn,
	uint cp_size, wlc_cubby_cp_fn_t *cp_fn, void *ctx)
{
	cubby_fn_t *cubby_fn;
	void **cubby_ctx;
	int offset;

	if (cubby_info->ncubby >= cubby_info->ncubbies) {
		WL_ERROR(("wl%d: %s: max %d cubbies exceeded\n",
		          cubby_info->wlc->pub->unit, __FUNCTION__, cubby_info->ncubbies));
		return BCME_NORESOURCE;
	}

	/* callbacks */
	cubby_fn = &cubby_info->cubby_fn[cubby_info->ncubby];
	cubby_fn->fn_init = fn->fn_init;
	cubby_fn->fn_deinit = fn->fn_deinit;
	cubby_fn->fn_secsz = fn->fn_secsz;
	/* optional callbacks */
	if (cp_fn != NULL) {
		cubby_fn->fn_get = cp_fn->fn_get;
		cubby_fn->fn_set = cp_fn->fn_set;
	}
	/* ctx */
	cubby_ctx = &cubby_info->cubby_ctx[cubby_info->ncubby];
	*cubby_ctx = ctx;

	cubby_info->ncubby++;

	/* actual cubby data is stored at past end of each object */
	offset = cubby_info->totsize;

	/* roundup to pointer boundary */
	cubby_info->totsize = (uint16)ROUNDUP(cubby_info->totsize + size, PTRSZ);

	cubby_info->cp_max_len = MAX(cubby_info->cp_max_len, cp_size);

	WL_CUBBY(("%s: info %p cubby %d total %u offset %u size %u\n",
	          __FUNCTION__, cubby_info, cubby_info->ncubby,
	          cubby_info->totsize, offset, size));

	return offset;
}

/* cubby init/deinit interfaces */
static uint
wlc_cubby_sec_totsize(wlc_cubby_info_t *cubby_info, void *obj)
{
	cubby_fn_t *cubby_fn;
	void *cubby_ctx;
	uint i;
	uint secsz = 0;

	for (i = 0; i < cubby_info->ncubby; i++) {
		cubby_fn = &cubby_info->cubby_fn[i];
		cubby_ctx = cubby_info->cubby_ctx[i];
		if (cubby_fn->fn_secsz) {
			secsz += cubby_fn->fn_secsz(cubby_ctx, obj);
		}
	}

	return secsz;
}

int
wlc_cubby_init(wlc_cubby_info_t *cubby_info, void *obj,
	cubby_sec_set_fn_t sec_fn, void *sec_ctx)
{
	wlc_info_t *wlc = cubby_info->wlc;
	cubby_fn_t *cubby_fn;
	void *cubby_ctx;
	uint i;
	uint secsz = 0;
	int err;
	uint8 *secbase = NULL;

	BCM_REFERENCE(wlc);

	/* Query each cubby user for secondary cubby size and
	 * allocate all memory at once
	 */
	if (sec_fn != NULL) {
		secsz = wlc_cubby_sec_totsize(cubby_info, obj);
		if (secsz > 0) {
			if ((secbase = MALLOCZ(wlc->osh, secsz)) == NULL) {
				WL_ERROR(("wl%d: %s: malloc failed\n",
				          wlc->pub->unit, __FUNCTION__));
				return BCME_NOMEM;
			}
			sec_fn(sec_ctx, obj, secbase);
		}

		WL_CUBBY(("wl%d: %s: obj %p addr %p size %u\n",
		          wlc->pub->unit, __FUNCTION__, obj, secbase, secsz));

		cubby_info->secsz = (uint16)secsz;
		cubby_info->secoff = 0;
		cubby_info->secbase = secbase;
	}

	/* Ionvoke each cubby's init func.
	 * Each cubby user must call wlc_cubby_sec_alloc to allocate
	 * secondary cubby memory.
	 */
	for (i = 0; i < cubby_info->ncubby; i++) {
		cubby_fn = &cubby_info->cubby_fn[i];
		cubby_ctx = cubby_info->cubby_ctx[i];
		if (cubby_fn->fn_init) {
			err = cubby_fn->fn_init(cubby_ctx, obj);
			if (err) {
				WL_ERROR(("wl%d: %s: Cubby failed\n",
				          wlc->pub->unit, __FUNCTION__));
				return err;
			}
		}
	}

	/* the told size must be equal to the allocated size i.e. the secondary
	 * cubby memory must be all allocated by each cubby user.
	 */
	if (sec_fn != NULL && secsz > 0) {
		ASSERT(cubby_info->secoff == secsz);
	}

	return BCME_OK;
}

void
wlc_cubby_deinit(wlc_cubby_info_t *cubby_info, void *obj,
	cubby_sec_get_fn_t get_fn, void *sec_ctx)
{
	wlc_info_t *wlc = cubby_info->wlc;
	cubby_fn_t *cubby_fn;
	void *cubby_ctx;
	uint i;
	uint secsz = 0;
	uint8 *secbase = NULL;

	BCM_REFERENCE(wlc);

	/* Query each cubby user for secondary cubby size and
	 * allocate all memory at once
	 */
	if (get_fn != NULL) {
		secsz = wlc_cubby_sec_totsize(cubby_info, obj);

		secbase = get_fn(sec_ctx, obj);

		WL_CUBBY(("wl%d: %s: obj %p addr %p size %u\n",
		          wlc->pub->unit, __FUNCTION__, obj, secbase, secsz));

		cubby_info->secsz = (uint16)secsz;
		cubby_info->secoff = (uint16)secsz;
		cubby_info->secbase = secbase;
	}

	/* Ionvoke each cubby's deinit func */
	for (i = 0; i < cubby_info->ncubby; i++) {
		uint j = cubby_info->ncubby - 1 - i;
		cubby_fn = &cubby_info->cubby_fn[j];
		cubby_ctx = cubby_info->cubby_ctx[j];
		if (cubby_fn->fn_deinit) {
			cubby_fn->fn_deinit(cubby_ctx, obj);
		}
	}

	/* Each cubby should call wlc_cubby_sec_free() to free its
	 * secondary cubby allocation.
	 */
	if (get_fn != NULL && secsz > 0) {
		ASSERT(cubby_info->secoff == 0);
		MFREE(wlc->osh, secbase, secsz);
	}
}

/* debug/dump interface */

/* total size of all cubbies */
uint
wlc_cubby_totsize(wlc_cubby_info_t *cubby_info)
{
	return cubby_info->totsize;
}

/* secondary cubby alloc/free */
void *
wlc_cubby_sec_alloc(wlc_cubby_info_t *cubby_info, void *obj, uint secsz)
{
	uint16 secoff = cubby_info->secoff;
	void *secptr = cubby_info->secbase + secoff;
	BCM_REFERENCE(obj);
	ASSERT(cubby_info->secbase != NULL);

	if (secsz == 0) {
		WL_ERROR(("%s: secondary cubby size 0, ignore.\n",
		          __FUNCTION__));
		return NULL;
	}

	ASSERT(cubby_info->secbase != NULL);
	ASSERT(secoff + secsz <= cubby_info->secsz);

	WL_CUBBY(("%s: address %p offset %u size %u\n",
	          __FUNCTION__, secptr, secoff, secsz));

	cubby_info->secoff += (uint16)secsz;
	return secptr;
}

void
wlc_cubby_sec_free(wlc_cubby_info_t *cubby_info, void *obj, void *secptr)
{
	uint secoff = (uint)((uint8 *)secptr - cubby_info->secbase);
	uint secsz = cubby_info->secoff - secoff;
	BCM_REFERENCE(obj);
	ASSERT(cubby_info->secbase != NULL);

	if (secptr == NULL) {
		WL_ERROR(("%s: secondary cubby pointer NULL, ignore.\n",
		          __FUNCTION__));
		return;
	}

	ASSERT(cubby_info->secbase != NULL);
	ASSERT(secoff <= cubby_info->secoff);

	WL_CUBBY(("%s: address %p offset %u size %u\n",
	          __FUNCTION__, secptr, secoff, secsz));

	cubby_info->secoff -= (uint16)secsz;
}

/* copy cubbies */
int
wlc_cubby_copy(wlc_cubby_info_t *from_info, void *from_obj,
	wlc_cubby_info_t *to_info, void *to_obj)
{
	uint i;
	void *data = NULL;
	int err = BCME_OK;
	wlc_info_t *wlc = from_info->wlc;
	/* MALLOC and FREE once for MAX cubby size, to minimize fragmentation */
	int max_len = from_info->cp_max_len;

	/* We allocate only if atleast one cubby had provided max_len.
	   Else, invoke fn_get() with data=NULL, which is supposed to update len and max_len.
	*/
	if (max_len)
		data = MALLOC(wlc->osh, max_len);
	if (max_len && data == NULL) {
		WL_ERROR(("%s: out of memory, malloced %d bytes\n",
		          __FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	WL_INFORM(("%s:do cubby get/set cp_max_len=%d\n", __FUNCTION__, max_len));

	/* Move the info over */
	for (i = 0; i < from_info->ncubby; i++) {
		cubby_fn_t *from_fn = &from_info->cubby_fn[i];
		void *from_ctx = from_info->cubby_ctx[i];
		cubby_fn_t *to_fn = &to_info->cubby_fn[i];
		void *to_ctx = to_info->cubby_ctx[i];

		if (from_fn->fn_get != NULL && to_fn->fn_set != NULL) {
			int len = max_len;

			err = (from_fn->fn_get)(from_ctx, from_obj, data, &len);
			/* Incase the cubby has more data than its initial max_len
			 * We re-alloc and update our max_len
			 * Note: max_len is max of all cubby
			 */
			if (err == BCME_BUFTOOSHORT && (len > max_len)) {
				MFREE(wlc->osh, data, max_len);
				data = MALLOC(wlc->osh, len);
				if (data == NULL) {
					WL_ERROR(("%s: out of memory, malloced %d bytes\n",
					          __FUNCTION__, MALLOCED(wlc->osh)));
					err = BCME_NOMEM;
					break;
				}
				max_len = len; /* update max_len for mfree */
				err = (from_fn->fn_get)(from_ctx, from_obj, data, &len);
			}
			if (err == BCME_OK && data && len) {
				err = (to_fn->fn_set)(to_ctx, to_obj, data, len);
			}
			if (err != BCME_OK) {
				WL_ERROR(("%s: failed for cubby[%d] err=%d\n",
					__FUNCTION__, i, err));
				break;
			}
		}
	}

	WL_INFORM(("%s:done cubby get/set cp_max_len=%d\n", __FUNCTION__, max_len));

	if (data)
		MFREE(wlc->osh, data, max_len);

	return err;
}
