/**
 * ACSD include file for ioctl (used only for media)
 *
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
 * $Id: acsd_ioctl.h 584445 2015-09-07 05:46:52Z $
 *
 */


#ifndef __acs_ioctl_h__
#define __acs_ioctl_h__

int wl_ioctl(char *name, int cmd, void *buf, int len);
int wl_iovar_getbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen);
int wl_iovar_setbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen);
int wl_iovar_get(char *ifname, char *iovar, void *bufptr, int buflen);
int wl_iovar_getint(char *ifname, char *iovar, int *val);
int wl_iovar_setint(char *ifname, char *iovar, int val);

#endif //__acs_ioctl_h__
