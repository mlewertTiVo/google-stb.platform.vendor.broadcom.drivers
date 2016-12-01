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
 * $Id: wlc_dump.c 630436 2016-04-08 23:03:14Z $
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
	defined(MCHAN_MINIDUMP) || defined(BCM_DNGDMP) || defined(DNG_DBGDUMP)
#define WLC_DUMP_FULL_SUPPORT
#endif

/* registry compacity */
#ifndef WLC_DUMP_NUM_REGS
#ifdef WLC_DUMP_FULL_SUPPORT
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
	{"dump", IOV_DUMP, IOVF_OPEN_ALLOW, 0, IOVT_BUFFER, 0},
	{"dump_clear", IOV_DUMP_CLR, IOVF_OPEN_ALLOW, 0, IOVT_BUFFER, 0},
	{NULL, 0, 0, 0, 0, 0}
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
	int ret = BCME_NOTFOUND;

	if (dumpi->reg != NULL) {
		ret = wlc_dump_reg_invoke_dump_fn(dumpi->reg, name, b);
	}
	if (ret == BCME_NOTFOUND) {
		ret = wlc_bmac_dump(wlc->hw, name, b);
	}

	return ret;
}

/** Invoke the given named dump clear callback */
static int
wlc_dump_clr_op(wlc_dump_info_t *dumpi, const char *name)
{
	wlc_info_t *wlc = dumpi->wlc;
	int ret = BCME_NOTFOUND;

	if (dumpi->reg != NULL) {
		ret = wlc_dump_reg_invoke_clr_fn(dumpi->reg, name);
	}
	if (ret == BCME_NOTFOUND) {
		ret = wlc_bmac_dump_clr(wlc->hw, name);
	}

	return ret;
}

/* Tokenize the buffer (delimiter is ' ' or '\0') and return the null terminated string
 * in param 'name' if the string fits in 'name'. The string will be truncated in 'name'
 * if it's longer than 'name/name_len', in which case BCME_BADARG is returned in 'err'.
 * Return the name string length in 'buf' as the function return value.
 */
static int
wlc_dump_next_name(char *name, int name_len, char **buf, uint *buf_len, int *err)
{
	char *p = *buf;
	int l = *buf_len;
	int i = 0;

	/* skip leading space in 'buf' */
	while (l > 0 &&
	       bcm_isspace(*p) && *p != '\0') {
		p++;
		l--;
	}

	/* find the end of the name string while copying to 'name'.
	 * name string is terminated by space or null in 'buf'.
	 * the copied string may not be null terminated.
	 */
	while (l > 0 &&
	       !bcm_isspace(*p) && *p != '\0') {
		if (i < name_len) {
			name[i] = *p;
		}
		i++;
		p++;
		l--;
	}

	/* terminate the name if the string plus the null terminator fits 'name',
	 * or return error otherwise.
	 */
	if (i >= name_len) {
		*err = BCME_BUFTOOSHORT;
	}
	else {
		*err = BCME_OK;
		name[i] = 0;
	}

	/* update the buffer pointer and length */
	*buf = p;
	*buf_len = l;

	/* return the name string length in 'buf' */
	return i;
}

/* including null terminator */
#define DUMP_NAME_BUF_LEN 17

/* dump by name (could be a list of names separated by space)
 * in in_buf.
 */
static int
wlc_dump(wlc_dump_info_t *dumpi, char *in_buf, uint in_len, char *out_buf, uint out_len)
{
	int err = BCME_NOTFOUND;
	struct bcmstrbuf b;
	char name[DUMP_NAME_BUF_LEN];
	bool sep = FALSE;
	char *buf = NULL;
	uint buf_len = 0;

	bcm_binit(&b, out_buf, out_len);

	/* do default action if no dump name is present... */
	if (wlc_dump_next_name(name, sizeof(name), &in_buf, &in_len, &err) == 0) {
		WL_PRINT(("doing default dump...\n"));
		goto exit;
	}

	/* dump all names in the name list...unless there's error */
	do {
		char next[DUMP_NAME_BUF_LEN];
		bool more;
		int nerr;

		/* we could ignore the name that is too long instead */
		if (err == BCME_BUFTOOSHORT) {
			name[sizeof(name)- 1] = 0;
			WL_ERROR(("%s: name %s... is too long\n",
			          __FUNCTION__, name));
			err = BCME_BADARG;
			goto exit;
		}

		/* look ahead, do we have more dump ... */
		if ((more =
		     wlc_dump_next_name(next, sizeof(next), &in_buf, &in_len, &nerr) > 0)) {
			/* also allocate a buffer to copy the remaining of names over
			 * as the in_buf and out_buf may be the same and out_buf will
			 * be overwritten next.
			 */
			if (nerr == BCME_OK &&
			    buf == NULL &&
			    (buf_len = (uint)strnlen(in_buf, in_len)) > 0) {
				if ((buf = MALLOC(dumpi->wlc->osh, buf_len)) == NULL) {
					err = BCME_NOMEM;
					goto exit;
				}
				memcpy(buf, in_buf, buf_len);
				in_buf = buf;
				in_len = buf_len;
			}
		}

		/* only print the separator if there are more than one dump */
		sep |= more;
		if (sep) {
			bcm_bprintf(&b, "\n%s:------\n", name);
		}

		if ((err = wlc_dump_op(dumpi, name, &b)) != BCME_OK) {
			break;
		}

		if (!more) {
			break;
		}

		ASSERT(sizeof(name) == sizeof(next));
		memcpy(name, next, sizeof(next));
		err = nerr;

	} while (TRUE);

exit:
	if (buf != NULL) {
		MFREE(dumpi->wlc->osh, buf, buf_len);
	}

	if (err == BCME_NOTFOUND) {
		WL_INFORM(("wl%d: %s: forcing return code to BCME_UNSUPPORTED\n",
		           dumpi->wlc->pub->unit, __FUNCTION__));
		err = BCME_UNSUPPORTED;
	}

	return err;
}

/* Dump clear the given name (could be a list of names separated by space)
 * in in_buf.
 */
static int
wlc_dump_clr(wlc_dump_info_t *dumpi, char *in_buf, uint in_len, char *out_buf, uint out_len)
{
	char name[DUMP_NAME_BUF_LEN];
	int err = BCME_NOTFOUND;

	/* dump clear all names in the name list...unless there's error */
	while (wlc_dump_next_name(name, sizeof(name), &in_buf, &in_len, &err) > 0) {
		if (err == BCME_BUFTOOSHORT) {
			name[sizeof(name)- 1] = 0;
			WL_ERROR(("%s: name %s... is too long\n",
			          __FUNCTION__, name));
			err = BCME_BADARG;
			break;
		}
		if ((err = wlc_dump_clr_op(dumpi, name)) != BCME_OK) {
			break;
		}
	}

	if (err == BCME_NOTFOUND) {
		WL_INFORM(("wl%d: %s: forcing return code to BCME_UNSUPPORTED\n",
		           dumpi->wlc->pub->unit, __FUNCTION__));
		err = BCME_UNSUPPORTED;
	}

	return err;
}

/* 'wl dump [default]' command */


/** Register dump name and handlers. Calling function must keep 'dump function' */
int
wlc_dump_add_fns(wlc_pub_t *pub, const char *name,
	dump_fn_t dump_fn, clr_fn_t clr_fn, void *ctx)
{
	wlc_info_t *wlc = (wlc_info_t *)pub->wlc;

	if (wlc->dumpi->reg != NULL) {
		return wlc_dump_reg_add_fns(wlc->dumpi->reg, name, dump_fn, clr_fn, ctx);
	}

	return BCME_UNSUPPORTED;
}

/* iovar dispatcher */
static int
wlc_dump_doiovar(void *hdl, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
	wlc_dump_info_t *dumpi = hdl;
	int err = BCME_OK;

	BCM_REFERENCE(dumpi);

	switch (actionid) {

	case IOV_GVAL(IOV_DUMP):
		err = wlc_dump(dumpi, params, p_len, arg, len);
		break;

	case IOV_SVAL(IOV_DUMP_CLR):
		err = wlc_dump_clr(dumpi, params, p_len, arg, len);
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

int
wlc_dump_local(wlc_dump_info_t *dumpi, char * name, int dump_len)
{
	char * dump_buf;
	int err = BCME_OK;
	int len;
	char * ptr = NULL;
	int i = 0;
	char tmp = '\0';

	dump_buf = (char *)MALLOC(dumpi->wlc->osh, dump_len);

	if (dump_buf == NULL) {
		err = BCME_NOMEM;
		WL_ERROR(("%s: malloc of %d bytes failed...\b", __FUNCTION__, dump_len));
		goto end;
	}

	/* NULL terminate the string */
	dump_buf[0] = '\0';

	err = wlc_dump(dumpi, name, strlen(name) + 1, dump_buf, dump_len);

	/* Irrespective of return code print the data populated by dump API */
	len = strlen(dump_buf);

	if (len >= dump_len) {
		WL_ERROR(("%s: dump len(%d) greater than input buffer size(%d)\n",
			__FUNCTION__, len, dump_len));
		err = BCME_ERROR;
	}

	/* Print each line in dump buffer */
	while (i < len) {
		/* Mark start of the current print line */
		ptr = &dump_buf[i];

		/* traverse till '\n' or end of buffer */
		while ((i < len) && dump_buf[i] && dump_buf[i] != '\n') {
			i++;
		}

		/* If '\n' add '\0' after this to allow the printing current line */
		if (dump_buf[i]) {
			tmp = dump_buf[i + 1];
			dump_buf[i + 1] = '\0';
		}

#ifdef ENABLE_CORECAPTURE
		WIFICC_LOGDEBUG(("%s", ptr));
#else
		WL_PRINT(("%s", ptr));
#endif /* ENABLE_CORECAPTURE */

		/* Restore the character after '\n' */
		if (tmp) {
			dump_buf[i+1] = tmp;
			tmp = '\0';
		}

		i++;
	}
end:
	if (dump_buf) {
		MFREE(dumpi->wlc->osh, dump_buf, dump_len);
	}

	return err;
}
