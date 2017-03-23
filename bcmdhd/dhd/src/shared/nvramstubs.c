/*
 * Stubs for NVRAM functions for platforms without flash
 *
 * Copyright (C) 1999-2017, Broadcom Corporation
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
 * $Id: nvramstubs.c 523160 2014-12-27 23:04:01Z $
 */

#include <bcm_cfg.h>
#include <typedefs.h>
#include <bcmutils.h>
#undef strcmp
#define strcmp(s1,s2)	0	/* always match */
#include <bcmnvram.h>

int
nvram_init(void *sih)
{
	return 0;
}


int
nvram_append(void *sb, char *vars, uint varsz)
{
	return 0;
}

void
nvram_exit(void *sih)
{
}

char *
nvram_get(const char *name)
{
	return (char *) 0;
}

int
nvram_set(const char *name, const char *value)
{
	return 0;
}

int
nvram_unset(const char *name)
{
	return 0;
}

int
nvram_commit(void)
{
	return 0;
}

int
nvram_getall(char *buf, int count)
{
	/* add null string as terminator */
	if (count < 1)
		return BCME_BUFTOOSHORT;
	*buf = '\0';
	return 0;
}
