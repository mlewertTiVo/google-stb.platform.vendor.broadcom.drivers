/*
 * Broadcom DHCP Server
 * DHCP Debug definitions.
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * $Id: dhcpdebug.h,v 1.7 2010-04-29 09:46:57 $
 */

#ifdef __cplusplus
extern "C" {
#endif

#define VALIDATE(p, type)
#define VINIT(p, type)
#define VDEINIT(p, type)
#define DHCPLOG(args) LOGprintf args
extern void LOGprintf(const char *fmt, ...);

#define ASSERT(c)			{ if (!(c)) OslHandleAssert(__FILE__, __LINE__); }

#ifdef __cplusplus
}
#endif
