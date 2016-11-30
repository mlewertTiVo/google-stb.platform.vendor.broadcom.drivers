/**
 * This file provides the wrapper for ioctl for acsd media build.
 * Not used in router acsd
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
 * $Id: acsd_ioctl.c 584445 2015-09-07 05:46:52Z $
 *
 */

#include <typedefs.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <wlioctl.h>
#include <bcmutils.h>
#include "bcmendian.h"
#include "acsd.h"

/*
*This function get buffer from from DHD through wl_ioctl
*/
int
wl_ioctl(char *name, int cmd, void *buf, int len)
{
	wl_ioctl_t ioc;
	int ret = 0;
	int s;
	char buffer[100];

	ACSD_INFO("wl_ioctl: %s %d\n", name, cmd);

	/* open socket to kernel */
	if ((s = acsd_open_socket()) < 0) {
		return -1;
	}

	/* do it */
	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;

	/* initializing the remaining fields */
	ioc.set = (cmd == WLC_SET_VAR) ? TRUE: FALSE;
	ioc.used = 0;
	ioc.needed = 0;

	if ((ret = acsd_ioctl(s, name, &ioc)) < 0)
		if (cmd != WLC_GET_MAGIC && cmd != WLC_GET_BSSID) {
			if ((cmd == WLC_GET_VAR) || (cmd == WLC_SET_VAR)) {
				snprintf(buffer, sizeof(buffer), "%s: WLC_%s_VAR(%s)", name,
					cmd == WLC_GET_VAR ? "GET" : "SET", (char *)buf);
			} else {
				snprintf(buffer, sizeof(buffer), "%s: cmd=%d", name, cmd);
			}
			ACSD_ERROR("%s\n", buffer);
		}
	/* cleanup */
	acsd_close_socket(s);
	return ret;
}

int
wl_iovar_getbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen)
{
	int err;
	uint namelen;
	uint iolen;

	ACSD_INFO("%s\n", iovar);

	namelen = strlen(iovar) + 1;	 /* length of iovar name plus null */
	iolen = namelen + paramlen;

	/* check for overflow */
	if (iolen > buflen)
		return (BCME_BUFTOOSHORT);

	memcpy(bufptr, iovar, namelen);	/* copy iovar name including null */
	memcpy((int8*)bufptr + namelen, param, paramlen);

	err = wl_ioctl(ifname, WLC_GET_VAR, bufptr, buflen);

	return (err);
}

/*
*This function get var  from from DHD through wl_ioctl
*/

int
wl_iovar_setbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen)
{
	uint namelen;
	uint iolen;

	ACSD_INFO("%s\n", iovar);

	namelen = strlen(iovar) + 1;	 /* length of iovar name plus null */
	iolen = namelen + paramlen;

	/* check for overflow */
	if (iolen > buflen)
		return (BCME_BUFTOOSHORT);

	memcpy(bufptr, iovar, namelen);	/* copy iovar name including null */
	memcpy((int8*)bufptr + namelen, param, paramlen);

	return wl_ioctl(ifname, WLC_SET_VAR, bufptr, iolen);
}

/*
*This function get buffer from from DHD through wl_iovar_getbuf
*/

int
wl_iovar_get(char *ifname, char *iovar, void *bufptr, int buflen)
{
	char smbuf[WLC_IOCTL_SMLEN];
	int ret;

	/* use the return buffer if it is bigger than what we have on the stack */
	if (buflen > sizeof(smbuf)) {
		ret = wl_iovar_getbuf(ifname, iovar, NULL, 0, bufptr, buflen);
	} else {
		ret = wl_iovar_getbuf(ifname, iovar, NULL, 0, smbuf, sizeof(smbuf));
		if (ret == 0)
			memcpy(bufptr, smbuf, buflen);
	}

	return ret;
}

/*
*This function set  var from from DHD through wl_iovar_setbuf
*/

int
wl_iovar_set(char *ifname, char *iovar, void *param, int paramlen)
{
	char smbuf[WLC_IOCTL_SMLEN];

	return wl_iovar_setbuf(ifname, iovar, param, paramlen, smbuf, sizeof(smbuf));
}

/*
 * get named driver variable to int value and return error indication
 * calling example: wl_iovar_getint(ifname, "arate", &rate)
 */
int
wl_iovar_getint(char *ifname, char *iovar, int *val)
{
	return wl_iovar_get(ifname, iovar, val, sizeof(int));
}

/*
 * set named driver variable to int value
 * calling example: wl_iovar_setint(ifname, "arate", rate)
*/
int
wl_iovar_setint(char *ifname, char *iovar, int val)
{
	return wl_iovar_set(ifname, iovar, &val, sizeof(val));
}
