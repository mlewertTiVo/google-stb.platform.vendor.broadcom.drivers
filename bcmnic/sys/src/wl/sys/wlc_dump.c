/**
 * @file
 * @brief
 * Named dump callback registration for WLC (excluding BMAC)
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
 * $Id: wlc_dump.c 611702 2016-01-11 23:40:32Z $
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_dump_reg.h>
#include <wlc_dump.h>

#if defined(BCMDBG_PHYDUMP) || defined(TDLS_TESTBED) || defined(BCMDBG_AMPDU) || \
	defined(MCHAN_MINIDUMP) || defined(BCM_DNGDMP)
#define WLC_DUMP_SUPP
#define WLC_DUMP_MULTI
/* #define WLC_DUMP_CLR_MULTI */
#endif

#if defined(WLC_DUMP_MULTI) || defined(WLC_DUMP_CLR_MULTI)
#define NAME_LIST_PARSE
#endif

/* registry compacity */
#ifndef WLC_DUMP_NUM_REGS
#ifdef WLC_DUMP_SUPP
#define WLC_DUMP_NUM_REGS 88
#else
#define WLC_DUMP_NUM_REGS 0
#endif
#endif /* WLC_DUMP_NUM_REGS */

/* iovar table */
enum {
	IOV_DUMP = 1,		/* Dump iovar */
	IOV_DUMP_CLR = 2,
	IOV_LAST
};

static const bcm_iovar_t dump_iovars[] = {
	{"dump", IOV_DUMP, IOVF_OPEN_ALLOW, IOVT_BUFFER, 0},
	{"dump_clear", IOV_DUMP_CLR, IOVF_OPEN_ALLOW, IOVT_BUFFER, 0},
	{NULL, 0, 0, 0, 0}
};

/* module info */
struct wlc_dump_info {
	wlc_info_t *wlc;
	wlc_dump_reg_info_t *reg;
};

/** Invoke the given named dump callback */
static int
wlc_dump_op(wlc_dump_info_t *dumpi, const char *name,
	struct bcmstrbuf *b)
{
	wlc_info_t *wlc = dumpi->wlc;
	int ret;

	ret = wlc_dump_reg_invoke_dump_fn(dumpi->reg, name, b);
	if (ret == BCME_NOTFOUND)
		ret = wlc_bmac_dump(wlc->hw, name, b);

	return ret;
}

/** Invoke the given named dump clear callback */
static int
wlc_dump_clr_op(wlc_dump_info_t *dumpi, const char *name)
{
	wlc_info_t *wlc = dumpi->wlc;
	int ret;

	ret = wlc_dump_reg_invoke_clr_fn(dumpi->reg, name);
	if (ret == BCME_NOTFOUND)
		ret = wlc_bmac_dump_clr(wlc->hw, name);

	return ret;
}

/* dump by name (could be a list of names separated by space) */

#ifdef NAME_LIST_PARSE
static char *
wlc_dump_next_name(char **buf)
{
	char *p;
	char *name;

	p = *buf;

	if (p == NULL)
		return NULL;

	/* skip leading space */
	while (bcm_isspace(*p) && *p != '\0')
		p++;

	/* remember the name start position */
	if (*p != '\0')
		name = p;
	else
		name = NULL;

	/* find the end of the name
	 * name is terminated by space or null
	 */
	while (!bcm_isspace(*p) && *p != '\0')
		p++;

	/* replace the delimiter (or '\0' character) with '\0'
	 * and set the buffer pointer to the character past the delimiter
	 * (or to NULL if the end of the string was reached)
	 */
	if (*p != '\0') {
		*p++ = '\0';
		*buf = p;
	} else {
		*buf = NULL;
	}

	/* return the pointer to the name */
	return name;
}
#endif /* NAME_LIST_PARSE */

#ifdef WLC_DUMP_MULTI
static int
wlc_dump_multi(wlc_dump_info_t *dumpi, char *params, int p_len, char *out_buf, int out_len)
{
	wlc_info_t *wlc = dumpi->wlc;
	struct bcmstrbuf b;
	char *name_list;
	int name_list_len;
	char *name1;
	char *name;
	const char *p;
	const char *endp;
	int err = 0;
	char *name_list_ptr;

	bcm_binit(&b, out_buf, out_len);
	p = params;
	endp = p + p_len;

	/* find the dump name list length to make a copy */
	while (p != endp && *p != '\0')
		p++;

	/* return an err if the name list was not null terminated */
	if (p == endp)
		return BCME_BADARG;

	/* copy the dump name list to a new buffer since the output buffer
	 * may be the same memory as the dump name list
	 */
	name_list_len = (int) ((const uint8*)p - (const uint8*)params + 1);
	name_list = (char*)MALLOC(wlc->osh, name_list_len);
	if (!name_list)
	      return BCME_NOMEM;
	bcopy(params, name_list, name_list_len);

	name_list_ptr = name_list;

	/* get the first two dump names */
	name1 = wlc_dump_next_name(&name_list_ptr);
	name = wlc_dump_next_name(&name_list_ptr);

	/* if the dump list was empty, return the default dump */
	if (name1 == NULL) {
		WL_ERROR(("doing default dump\n"));
		goto exit;
	}

	/* dump the first section
	 * only print the separator if there are more than one dump
	 */
	if (name != NULL)
		bcm_bprintf(&b, "\n%s:------\n", name1);
	err = wlc_dump_op(dumpi, name1, &b);
	if (err)
		goto exit;

	/* dump the rest */
	while (name != NULL) {
		bcm_bprintf(&b, "\n%s:------\n", name);
		err = wlc_dump_op(dumpi, name, &b);
		if (err)
			break;

		name = wlc_dump_next_name(&name_list_ptr);
	}

exit:
	MFREE(wlc->osh, name_list, name_list_len);

	/* make sure the output is at least a null terminated empty string */
	if (b.origbuf == b.buf && b.size > 0)
		b.buf[0] = '\0';

	return err;
}
#else /* !WLC_DUMP_MULTI */
/* Dump the given name */
/* Can't use the string in iovar buffer directly as a null terminated string so
 * use a local buffer to make sure the termination is there.
 */
#define DUMP_MAX_NAME_LEN	32

static int
wlc_dump(wlc_dump_info_t *dumpi, char *params, int p_len, char *out_buf, int out_len)
{
	char name[DUMP_MAX_NAME_LEN];
	struct bcmstrbuf b;

	if (p_len >= DUMP_MAX_NAME_LEN)
		return BCME_BADARG;

	memcpy(name, params, p_len);
	name[p_len] = '\0';

	bcm_binit(&b, out_buf, out_len);

	return wlc_dump_op(dumpi, name, &b);
}
#endif /* !WLC_DUMP_MULTI */

#ifdef WLC_DUMP_CLR_MULTI
#else /* !WLC_DUMP_CLR_MULTI */
/* Dump clear the given name. */
/* Can't use the string in iovar buffer directly as a null terminated string so
 * use a local buffer to make sure the termination is there.
 */
#define DUMP_CLR_MAX_NAME_LEN	32

static int
wlc_dump_clr(wlc_dump_info_t *dumpi, char *in_buf, uint in_len, char *out_buf, uint out_len)
{
	char name[DUMP_CLR_MAX_NAME_LEN];

	if (in_len >= DUMP_CLR_MAX_NAME_LEN)
		return BCME_BADARG;

	memcpy(name, in_buf, in_len);
	name[in_len] = '\0';

	return wlc_dump_clr_op(dumpi, name);
}
#endif /* !WLC_DUMP_CLR_MULTI */

/* 'wl dump [default]' command */


/** Register dump name and handlers. Calling function must keep 'dump function' */
int
BCMINITFN(wlc_dump_add_fns)(wlc_pub_t *pub, const char *name,
	dump_fn_t dump_fn, clr_fn_t clr_fn, void *ctx)
{
	wlc_info_t *wlc = (wlc_info_t *)pub->wlc;

	return wlc_dump_reg_add_fns(wlc->dumpi->reg, name, dump_fn, clr_fn, ctx);
}

/* iovar dispatcher */
static int
wlc_dump_doiovar(void *hdl, const bcm_iovar_t *vi, uint32 actionid, const char *name,
	void *params, uint p_len, void *arg, int len, int val_size, struct wlc_if *wlcif)
{
	wlc_dump_info_t *dumpi = hdl;
	int err = BCME_OK;

	BCM_REFERENCE(dumpi);

	/* Do the actual parameter implementation */
	switch (actionid) {

	case IOV_GVAL(IOV_DUMP):
#ifdef WLC_DUMP_MULTI
		err = wlc_dump_multi(dumpi, params, p_len, arg, len);
#else
		err = wlc_dump(dumpi, params, p_len, arg, len);
#endif
		break;

	case IOV_SVAL(IOV_DUMP_CLR):
#ifdef WLC_DUMP_CLR_MULTI
#else /* !WLC_DUMP_CLR_MULTI */
		err = wlc_dump_clr(dumpi, params, p_len, arg, len);
#endif
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* attach/detach to/from the system */
wlc_dump_info_t *
BCMATTACHFN(wlc_dump_pre_attach)(wlc_info_t *wlc)
{
	wlc_dump_info_t *dumpi;

	if ((dumpi = MALLOCZ(wlc->osh, sizeof(*dumpi))) == NULL) {
		goto fail;
	}
	dumpi->wlc = wlc;
#if WLC_DUMP_NUM_REGS > 0
	if ((dumpi->reg = wlc_dump_reg_create(wlc->osh, WLC_DUMP_NUM_REGS)) == NULL) {
		goto fail;
	}
#endif
	return dumpi;

fail:
	wlc_dump_post_detach(dumpi);
	return NULL;
}

void
BCMATTACHFN(wlc_dump_post_detach)(wlc_dump_info_t *dumpi)
{
	wlc_info_t *wlc;

	if (dumpi == NULL)
		return;

	wlc = dumpi->wlc;
#if WLC_DUMP_NUM_REGS > 0
	if (dumpi->reg != NULL) {
		wlc_dump_reg_destroy(dumpi->reg);
	}
#endif
	MFREE(wlc->osh, dumpi, sizeof(*dumpi));
}

/* attach/detach to/from wlc module registry */
int
BCMATTACHFN(wlc_dump_attach)(wlc_dump_info_t *dumpi)
{
	wlc_info_t *wlc = dumpi->wlc;

	if (wlc_module_register(wlc->pub, dump_iovars, "dump",
			dumpi, wlc_dump_doiovar,
			NULL, NULL, NULL) != BCME_OK) {
		goto fail;
	}


	return BCME_OK;

fail:
	wlc_dump_detach(dumpi);
	return BCME_ERROR;
}

void
BCMATTACHFN(wlc_dump_detach)(wlc_dump_info_t *dumpi)
{
	wlc_info_t *wlc;

	if (dumpi == NULL)
		return;

	wlc = dumpi->wlc;
	(void)wlc_module_unregister(wlc->pub, "dump", dumpi);
}
