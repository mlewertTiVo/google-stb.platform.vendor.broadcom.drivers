/*
 * PHY utils - nvram access functions.
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id$
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmnvram.h>

#include <phy_utils_var.h>

#define MAX_FABID_CHARS	16

#if defined(LINUX_HYBRID)
char *
phy_utils_getvar(phy_info_t *pi, const char *name)
{

	ASSERT(pi->vars != (char *)&pi->vars);

	NVRAM_RECLAIM_CHECK(name);
	return phy_utils_getvar_internal(pi, name);
}

char *
phy_utils_getvar_internal(phy_info_t *pi, const char *name)
{
	char *vars = pi->vars;
	char *s;
	int len;

	if (!name)
		return NULL;

	len = strlen(name);
	if (len == 0)
		return NULL;

	/* first look in vars[] */
	for (s = vars; s && *s;) {
		if ((bcmp(s, name, len) == 0) && (s[len] == '='))
			return (&s[len+1]);

		while (*s++)
			;
	}

#ifdef LINUX_HYBRID
	/* Don't look elsewhere! */
	return NULL;
#else
	/* Query nvram */
	return (nvram_get(name));

#endif /* LINUX_HYBRID */
}
#else /* LINUX_HYBRID || BCMDBG */
INLINE char *
phy_utils_getvar(phy_info_t *pi, const char *name)
{
	return getvar(pi->vars, name);
}
#endif 

char *
phy_utils_getvar_fabid(phy_info_t *pi, const char *name)
{
	NVRAM_RECLAIM_CHECK(name);
	return phy_utils_getvar_fabid_internal(pi, name);
}

static const char rstr_dotfabdot[] = ".fab.";
static const char rstr_SdotfabdotNd[] = "%s.fab.%d";

char *
phy_utils_getvar_fabid_internal(phy_info_t *pi, const char *name)
{
	char *val = NULL;

	if (pi->fabid) {
		uint16 sz = (strlen(name) + strlen(rstr_dotfabdot) + MAX_FABID_CHARS);
		char *fab_name = (char *) MALLOC(pi->sh->osh, sz);
		/* Prepare fab name */
		if (fab_name == NULL) {
			PHY_ERROR(("wl%d: %s: MALLOC failure\n",
				pi->sh->unit, __FUNCTION__));
			return FALSE;
		}
		snprintf(fab_name, sz, rstr_SdotfabdotNd, name, pi->fabid);

		val = phy_utils_getvar(pi, (const char *)fab_name);
		MFREE(pi->sh->osh, fab_name, sz);
	}

	if (val == NULL) {
		val = phy_utils_getvar(pi, (const char *)name);
	}

	return val;
}


int
phy_utils_getintvar(phy_info_t *pi, const char *name)
{
	return phy_utils_getintvar_default(pi, name, 0);
}

int
phy_utils_getintvar_default(phy_info_t *pi, const char *name, int default_value)
{
	char *val = PHY_GETVAR(pi, name);
	if (val != NULL)
		return (bcm_strtoul(val, NULL, 0));

	return (default_value);
}

int
phy_utils_getintvararray(phy_info_t *pi, const char *name, int idx)
{
	return PHY_GETINTVAR_ARRAY_DEFAULT(pi, name, idx, 0);
}

int
phy_utils_getintvararray_default(phy_info_t *pi, const char *name, int idx, int default_value)
{
	/* Rely on PHY_GETVAR() doing the NVRAM_RECLAIM_CHECK() */
	if (PHY_GETVAR(pi, name)) {
		return phy_utils_getintvararray_default_internal(pi, name, idx, default_value);
	} else return default_value;
}

int
phy_utils_getintvararray_default_internal(phy_info_t *pi, const char *name, int idx,
	int default_value)
{
	int i, val;
	char *vars = pi->vars;
	char *fabspf = NULL;

	val = default_value;
	/* Check if fab specific values are present in the NVRAM
	*  if present, replace the regular value with the fab specific
	*/
	if (pi->fabid) {
		uint16 sz = (strlen(name) + strlen(rstr_dotfabdot) + MAX_FABID_CHARS);
		char *fab_name = (char *) MALLOC(pi->sh->osh, sz);
		/* Prepare fab name */
		if (fab_name == NULL) {
			PHY_ERROR(("wl%d: %s: MALLOC failure\n",
				pi->sh->unit, __FUNCTION__));
			return FALSE;
		}
		snprintf(fab_name, sz, rstr_SdotfabdotNd, name, pi->fabid);
		/* Get the value from fabid specific first if present
		*  Assumption: param.fab.<fabid>.fab.<fabid> never exist
		*/
		fabspf = PHY_GETVAR(pi, (const char *)fab_name);
		if (fabspf != NULL) {
			i = getintvararraysize(vars, fab_name);
			if ((i == 0) || (i <= idx))
				val = default_value;
			else
				val = getintvararray(vars, fab_name, idx);
		}
		MFREE(pi->sh->osh, fab_name, sz);
	}

	if (fabspf == NULL) {
		i = getintvararraysize(vars, name);
		if ((i == 0) || (i <= idx))
			val = default_value;
		else
			val = getintvararray(vars, name, idx);
	}
	return val;
}
