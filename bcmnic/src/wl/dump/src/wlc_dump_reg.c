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
 * $Id: wlc_dump_reg.c 630436 2016-04-08 23:03:14Z $
 */
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <wlc_dump_reg.h>


#ifndef WLC_DUMP_REG_STR_SIZE
#define WLC_DUMP_REG_STR_SIZE	16
#endif

typedef struct {
	char name[WLC_DUMP_REG_STR_SIZE];
	wlc_dump_reg_dump_fn_t dump_fn;
	wlc_dump_reg_clr_fn_t clr_fn;
	void *ctx;
} wlc_dump_reg_ent_t;

struct wlc_dump_reg_info {
	osl_t *osh;
	uint16 cur;
	uint16 max;
	wlc_dump_reg_ent_t *ent;
};

/* debug */
#define WL_DUMP(x)

/* registry alloc size */
#define WLC_DUMP_REG_SZ(cnt) (sizeof(wlc_dump_reg_info_t) + \
			      sizeof(wlc_dump_reg_ent_t) * (cnt))

/* create a registry with 'count' entries */
wlc_dump_reg_info_t *
BCMATTACHFN(wlc_dump_reg_create)(osl_t *osh, uint16 count)
{
	wlc_dump_reg_info_t *reg = NULL;

	if (count > 0) {
		uint sz = (uint)WLC_DUMP_REG_SZ(count);
		reg = MALLOCZ(osh, sz);
		if (reg == NULL) {
			WL_DUMP(("%s: MALLOC(%d) failed; total malloced %d bytes\n",
			          __FUNCTION__, sz, MALLOCED(osh)));
			goto fail;
		}
		reg->cur = 0;
		reg->max = count;
		reg->osh = osh;
		reg->ent = (wlc_dump_reg_ent_t *)(reg + 1);
	}

fail:
	return reg;
}

/* destroy a registry */
void
BCMATTACHFN(wlc_dump_reg_destroy)(wlc_dump_reg_info_t *reg)
{
	ASSERT(reg != NULL);

	MFREE(reg->osh, reg, (uint)WLC_DUMP_REG_SZ(reg->max));
}

/* look up a name in a registry */
static int
wlc_dump_reg_lookup(wlc_dump_reg_info_t *reg, const char *name, uint namelen)
{
	int i;

	/* full string comparison */
	for (i = 0; i < reg->cur; i++) {
		uint cmplen = (uint)strnlen(reg->ent[i].name, WLC_DUMP_REG_STR_SIZE);

		cmplen = MIN(cmplen, WLC_DUMP_REG_STR_SIZE);
		if (cmplen == namelen &&
		    strncmp(name, reg->ent[i].name, cmplen) == 0)
			return i;
	}

	return BCME_NOTFOUND;
}

/* add a name and its callback function to a registry */
int
wlc_dump_reg_add_fns(wlc_dump_reg_info_t *reg, const char *name,
	wlc_dump_reg_dump_fn_t dump_fn, wlc_dump_reg_clr_fn_t clr_fn, void *ctx)
{
	int ret;
	uint namelen;

	ASSERT(reg);
	ASSERT(name != NULL);
	ASSERT(dump_fn != NULL || clr_fn != NULL);

	/* do not add to table if namelen is 0 or longer than WLC_DUMP_REG_STR_SIZE */
	namelen = strlen(name);
	if (namelen == 0 || namelen > WLC_DUMP_REG_STR_SIZE) {
		WL_DUMP(("%s: %s: name length should be between 1 and %d\n",
		          __FUNCTION__, name, WLC_DUMP_REG_STR_SIZE));
		return BCME_BADARG;
	}

	ret = wlc_dump_reg_lookup(reg, name, namelen);
	if (ret == BCME_NOTFOUND) {
		if (reg->cur < reg->max) {
			strncpy(reg->ent[reg->cur].name, name, namelen);
			reg->ent[reg->cur].dump_fn = dump_fn;
			reg->ent[reg->cur].clr_fn = clr_fn;
			reg->ent[reg->cur].ctx = ctx;
			reg->cur++;
			ret = BCME_OK;
		}
		else {
			WL_DUMP(("%s: registry is full\n", __FUNCTION__));
			ret = BCME_NORESOURCE;
		}
	}
	else if (ret >= 0) {
		WL_DUMP(("%s: %s already in registry\n", __FUNCTION__, name));
		ret = BCME_ERROR;
	}

	return ret;
}

/* invoke a dump callback function in a registry by name */
int
wlc_dump_reg_invoke_dump_fn(wlc_dump_reg_info_t *reg, const char *name,
	void *arg)
{
	int ret;
	uint namelen;

	ASSERT(name);

	namelen = strlen(name);
	ret = wlc_dump_reg_lookup(reg, name, namelen);
	if (ret >= 0) {
		if (reg->ent[ret].dump_fn == NULL)
			return BCME_UNSUPPORTED;
		return (reg->ent[ret].dump_fn)(reg->ent[ret].ctx, arg);
	}

	return ret;
}

/* invoke a dump clear callback function in a registry by name */
int
wlc_dump_reg_invoke_clr_fn(wlc_dump_reg_info_t *reg, const char *name)
{
	int ret;
	uint namelen;

	ASSERT(name);

	namelen = strlen(name);
	ret = wlc_dump_reg_lookup(reg, name, namelen);
	if (ret >= 0) {
		if (reg->ent[ret].clr_fn == NULL)
			return BCME_UNSUPPORTED;
		return (reg->ent[ret].clr_fn)(reg->ent[ret].ctx);
	}

	return ret;
}

/* dump the registered names */
int
wlc_dump_reg_list(wlc_dump_reg_info_t *reg, struct bcmstrbuf *b)
{
#ifdef BCMDRIVER
	int i;
	for (i = 0; i < reg->cur; i++) {
		char name[WLC_DUMP_REG_STR_SIZE + 1];
		strncpy(name, reg->ent[i].name, sizeof(name));
		name[WLC_DUMP_REG_STR_SIZE] = '\0';
		bcm_bprintf(b, "%s\n", name);
	}
#endif
	return BCME_OK;
}

/* dump the registry internals */
int
wlc_dump_reg_dump(wlc_dump_reg_info_t *reg, struct bcmstrbuf *b)
{
#ifdef BCMDRIVER
	uint i;
	bcm_bprintf(b, "registry %p: cnt %u max %u\n", reg, reg->cur, reg->max);
	for (i = 0; i < reg->cur; i ++) {
		char name[WLC_DUMP_REG_STR_SIZE + 1];
		strncpy(name, reg->ent[i].name, sizeof(name));
		name[WLC_DUMP_REG_STR_SIZE] = '\0';
		bcm_bprintf(b, "idx %d: name \"%s\" dump_fn %p clr_fn %p ctx %p\n",
		            i, name, reg->ent[i].dump_fn, reg->ent[i].clr_fn,
		            reg->ent[i].ctx);
	}
#endif
	return BCME_OK;
}
