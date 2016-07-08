/*
 * WLC Object Registry API Implementation
 * Broadcom 802.11abg Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_objregistry.c 475986 2014-05-07 17:29:24Z $
 */

/**
 * @file
 * @brief
 * With the rise of RSDB (Real Simultaneous Dual Band), the need arose to support two 'wlc'
 * structures, with the requirement to share data between the two. To meet that requirement, a
 * simple 'key=value' mechanism is introduced.
 *
 * WLC Object Registry provides mechanisms to share data across WLC instances in RSDB
 * The key-value pairs (enums/void *ptrs) to be stored in the "registry" are decided at design time.
 * Even the Non_RSDB (single instance) goes thru the Registry calls to have a unified interface.
 * But the Non_RSDB functions call have dummy/place-holder implementation managed using MACROS.
 *
 * The registry stores key=value in a simple array which is index-ed by 'key'
 * The registry also maintains a reference counter, which helps the caller in freeing the
 * 'value' associated with a 'key'
 * The registry stores objects as pointers represented by "void *" and hence a NULL value
 * indicates unused key
 *
 */



#ifdef WL_OBJ_REGISTRY

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <wlioctl.h>
#include <wl_dbg.h>
#include <wlc_types.h>
#include <d11.h>
#include <wlc_rate.h>
#include <siutils.h>
#include <wlc_pub.h>
#include <wlc_key.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_objregistry.h>

#define WL_OBJR_DBG(x)

struct obj_registry {
	int count;
	void **value;
	uint8 *ref;
};


/* WLC Object Registry Public APIs */
obj_registry_t* obj_registry_alloc(osl_t *osh, int count)
{
	obj_registry_t *objr = NULL;
	if ((objr = MALLOCZ(osh, sizeof(obj_registry_t))) == NULL) {
			WL_ERROR(("wl %s: out of memory, malloced %d bytes\n",
				__FUNCTION__, MALLOCED(osh)));
	} else {
		objr->count = count;

		if ((objr->value =
			MALLOCZ(osh, sizeof(void*) * count)) == NULL) {
			WL_ERROR(("wl %s: out of memory, malloced %d bytes\n",
				__FUNCTION__, MALLOCED(osh)));
			obj_registry_free(objr, osh);
			return NULL;
		}

		if ((objr->ref =
			MALLOCZ(osh, sizeof(uint8) * count)) == NULL) {
			WL_ERROR(("wl %s: out of memory, malloced %d bytes\n",
				__FUNCTION__, MALLOCED(osh)));
			obj_registry_free(objr, osh);
			return NULL;
		}
	}
	WL_OBJR_DBG(("WLC_OBJR CREATED : %p\n", objr));

	return objr;
}

void obj_registry_free(obj_registry_t *objr, osl_t *osh)
{
	if (objr) {
		if (objr->value)
			MFREE(osh, objr->value, sizeof(void*) * objr->count);
		if (objr->ref)
			MFREE(osh, objr->ref, sizeof(uint8) * objr->count);
		MFREE(osh, objr, sizeof(obj_registry_t));
	}
}

int obj_registry_set(obj_registry_t *objr, obj_registry_key_t key, void *value)
{
	if (objr) {
		WL_OBJR_DBG(("%s:%d:key[%d]=value[%p]\n", __FUNCTION__, __LINE__, (int)key, value));
		if ((key < 0) || (key >= objr->count)) return BCME_RANGE;
		objr->value[(int)key] = value;
		return BCME_OK;
	}
	return BCME_ERROR;
}

void* obj_registry_get(obj_registry_t *objr, obj_registry_key_t key)
{
	void *value = NULL;
	if (objr) {
		if ((key < 0) || (key >= objr->count)) return NULL;
		value = objr->value[(int)key];
		WL_OBJR_DBG(("%s:%d:key[%d]=value[%p]\n", __FUNCTION__, __LINE__, (int)key, value));
	}
	return value;
}

int obj_registry_ref(obj_registry_t *objr, obj_registry_key_t key)
{
	int ref = 0;
	if (objr && (key >= 0) && (key < objr->count)) {
		ref = ++(objr->ref[(int)key]);
		WL_OBJR_DBG(("%s:%d: key[%d]=value[%p]/REF[%d]\n",
			__FUNCTION__, __LINE__, (int)key, value, ref));
	}
	return ref;
}

int obj_registry_unref(obj_registry_t *objr, obj_registry_key_t key)
{
	int ref = 0;
	if (objr && (key >= 0) && (key < objr->count)) {
		ref = (objr->ref[(int)key]);
		if (ref > 0) {
			ref = --(objr->ref[(int)key]);
			WL_OBJR_DBG(("%s:%d: key[%d]=value[%p]/REF[%d]\n",
				__FUNCTION__, __LINE__, (int)key, value, ref));
		}
	}
	return ref;
}


/* A special helper function to identify if we are cleaning up for the finale WLC */
int obj_registry_islast(obj_registry_t *objr)
{
	if (objr && (objr->value[OBJR_SELF])) {
		return ((objr->ref[OBJR_SELF] <= 1) ? 1 : 0);
	}
	return 1;
}


#endif /* WL_OBJ_REGISTRY */
