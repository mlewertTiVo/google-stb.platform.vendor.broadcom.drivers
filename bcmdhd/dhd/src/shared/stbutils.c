/*
 * Read-only support for stb external params
 *
 * Copyright (C) 1999-2018, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id:stbutils.c $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <bcmdevs.h>
#include <stbutils.h>

#define STBUTIL_MSG(x)

#define NUM_PSIZES 16
typedef struct _params {
	struct _params *next;
	int bufsz;		/* allocated size */
	int size;		/* actual params size */
	char *params;
	uint16 psizes[NUM_PSIZES];
} params_t;

#define	PARAMS_ARRAY_MAXSIZE	128		/* For now */
#define	PARAMS_T_OH	sizeof(params_t)

static params_t *params = NULL;

static int stbpriv_params_init(osl_t *osh);
static int initparams_file(osl_t *osh, char **params_p, int *params_len);

static char *findparam(char *params_arg, char *lim, const char *name);

static char *param_get_internal(const char *name);
static int params_getall_internal(char *buf, int count);

static void
sortparams(osl_t *osh, params_t *new)
{
	char *s = new->params;
	int i;
	char *temp;
	char *c, *cend;
	uint8 *lp;

	/*
	 * Sort the variables by length.  Everything len NUM_PSIZES or
	 * greater is dumped into the end of the area
	 * in no particular order.  The search algorithm can then
	 * restrict the search to just those variables that are the
	 * proper length.
	 */

	/* get a temp array to hold the sorted vars */
	temp = MALLOC(osh, PARAMS_ARRAY_MAXSIZE + 1);
	if (!temp) {
		STBUTIL_MSG(("stbutils.c: Out of memory for malloc for STB params sort"));
		return;
	}

	c = temp;
	lp = (uint8 *) c++;

	/* Mark the var len and exp len as we move to the temp array */
	while ((s < (new->params + new->size)) && *s) {
		uint8 len = 0;
		char *start = c;
		while ((*s) && (*s != '=')) {  /* Scan the variable */
			*c++ = *s++;
			len++;
		}

		/* skip if param not set to anything */
		if (*s != '=') {
			c = start;

		} else {
			*lp = len; 		/* Set the len of this var */
			lp = (uint8 *) c++;
			s++;
			len = 0;
			while (*s) { 		/* Scan the expr */
				*c++ = *s++;
				len++;
			}
			*lp = len; 		/* Set the len of the expr */
			lp = (uint8 *) c++;
			s++;
		}
	}

	cend = (char *) lp;

	s = new->params;
	for (i = 1; i <= NUM_PSIZES; i++) {
		new->psizes[i - 1] = (uint16) (s - new->params);
		/* Scan for all variables of size i */
		for (c = temp; c < cend;) {
			int len = *c++;
			if ((len == i) || ((i == NUM_PSIZES) && (len >= NUM_PSIZES))) {
				/* Move the variable back */
				while (len--) {
					*s++ = *c++;
				}

				/* Get the length of the expression */
				len = *c++;
				*s++ = '=';

				/* Move the expression back */
				while (len--) {
					*s++ = *c++;
				}
				*s++ = 0;	/* Reinstate string terminator */

			} else {
				/* Wrong size - skip to next in temp copy */
				c += len;
				c += *c + 1;
			}
		}
	}

	MFREE(osh, temp, PARAMS_ARRAY_MAXSIZE + 1);
}

int
BCMATTACHFN(stbpriv_init)(osl_t *osh)
{
	if (stbpriv_params_init(osh) != 0)
		return BCME_ERROR;

	return 0;
}

static int
stbpriv_params_init(osl_t *osh)
{
	char *base = NULL, *nvp = NULL, *flvars = NULL;
	int err = 0, nvlen = 0;

	base = nvp = MALLOC(osh, PARAMS_ARRAY_MAXSIZE);
	if (base == NULL)
		return BCME_NOMEM;

	/* Init params from stbpriv file if they exist */
	err = initparams_file(osh, &nvp, (int*)&nvlen);
	if (err != 0) {
		STBUTIL_MSG(("stbutils.c: stbpriv.txt file failure\n"));
		err = -1;
		goto exit;
	}
	if (nvlen) {
		flvars = MALLOC(osh, nvlen);
		if (flvars == NULL)
			return BCME_NOMEM;
	}
	else {
		err = -1;
		goto exit;
	}

	bcopy(base, flvars, nvlen);
	err = stbparam_append(osh, flvars, nvlen);

exit:
	MFREE(osh, base, PARAMS_ARRAY_MAXSIZE);

	return err;
}

/** STB params (stbpriv.txt) file read for pcie NIC's */
static int
initparams_file(osl_t *osh, char **params_p, int *params_len)
{
#if defined(BCMDRIVER)
	/* Init stb params from stbpriv file if it exists */
	char *params_buf = *params_p;
	void	*params_fp = NULL;
	int nv_len = 0, ret = 0, i = 0, len = 0;

#if (defined(OEM_ANDROID) && defined(STBLINUX))
	params_fp = (void*)osl_os_open_image("/data/vendor/nexus/secdma/stbpriv.txt");
#else
	params_fp = (void*)osl_os_open_image("stbpriv.txt");
#endif
	if (params_fp != NULL) {
		while ((nv_len = osl_os_get_image_block(params_buf,
			PARAMS_ARRAY_MAXSIZE, params_fp)))
			len = nv_len;
	}
	else {
		STBUTIL_MSG(("stbutils.c: Could not open stbpriv.txt file\n"));
		ret = -1;
		goto exit;
	}

	/* Make array of strings */
	for (i = 0; i < len; i++) {
		if ((*params_buf == ' ') || (*params_buf == '\t') || (*params_buf == '\n') ||
			(*params_buf == '\0')) {
			*params_buf = '\0';
			params_buf++;
		}
		else
			params_buf++;
	}
	*params_buf++ = '\0';
	*params_p = params_buf;
	*params_len = len+1; /* add one for the null character */

exit:
	if (params_fp)
		osl_os_close_image(params_fp);

	return ret;
#else /* BCMDRIVER */
	return BCME_ERROR
#endif /* BCMDRIVER */
}

int
BCMATTACHFN(stbparam_append)(osl_t *osh, char *paramlst, uint paramsz)
{
	uint bufsz = PARAMS_T_OH;
	params_t *new;

	if ((new = MALLOC(osh, bufsz)) == NULL)
		return BCME_NOMEM;

	new->params = paramlst;
	new->bufsz = bufsz;
	new->size = paramsz;
	new->next = params;
	sortparams(osh, new);

	params = new;

	return BCME_OK;
}

void
BCMATTACHFN(stbpriv_exit)(osl_t *osh)
{
	params_t *this, *next;

	this = params;

	if (this)
		MFREE(osh, this->params, this->bufsz);

	while (this) {
		next = this->next;
		MFREE(osh, this, this->bufsz);
		this = next;
	}
	params = NULL;
}

static char *
findparam(char *vars_arg, char *lim, const char *name)
{
	char *s;
	int len;

	len = strlen(name);

	for (s = vars_arg; (s < lim) && *s;) {
		if ((bcmp(s, name, len) == 0) && (s[len] == '=')) {
			return (&s[len+1]);
		}

		while (*s++)
			;
	}

	return NULL;
}

static char *
param_get_internal(const char * name)
{
	params_t *cur;
	char *v = NULL;
	const int len = strlen(name);

	for (cur = params; cur; cur = cur->next) {
		if (len >= NUM_PSIZES) {
			/* Scan all strings with len > NUM_PSIZES */
			v = findparam(cur->params + cur->psizes[NUM_PSIZES - 1],
			            cur->params + cur->size, name);
		} else {
			/* Scan just the strings that match the len */
			v = findparam(cur->params + cur->psizes[len - 1],
			            cur->params + cur->psizes[len], name);
		}

		if (v) {
			return v;
		}
	}

	return v;
}

char *
stbparam_get(const char *name)
{
	return param_get_internal(name);
}

static int
params_getall_internal(char *buf, int count)
{
	int len, resid = count;
	params_t *this;

	this = params;
	while (this) {
		char *from, *lim, *to;
		int acc;

		from = this->params;
		lim = (char *)((uintptr)this->params + this->size);
		to = buf;
		acc = 0;
		while ((from < lim) && (*from)) {
			len = strlen(from) + 1;
			if (resid < (acc + len))
				return BCME_BUFTOOSHORT;
			bcopy(from, to, len);
			acc += len;
			from += len;
			to += len;
		}

		resid -= acc;
		buf += acc;
		this = this->next;
	}
	if (resid < 1)
		return BCME_BUFTOOSHORT;
	*buf = '\0';
	return 0;
}

int
stbparams_getall(char *buf, int count)
{
	return params_getall_internal(buf, count);
}
