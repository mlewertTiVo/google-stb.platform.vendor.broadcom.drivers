/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Copyright$
 *
 * $Id: $
 */

#ifndef WL_BCM7XXX_DBG_H
#define WL_BCM7XXX_DBG_H
#include <typedefs.h>
#include <linuxver.h>
#include <linux/module.h>

void Bcm7xxxPrintSpace(void *handle);
void Bcm7xxxEnter(void *handle, const char *fname, uint32 line);
void Bcm7xxxExit(void *handle, const char *fname, uint32 line);
void Bcm7xxxExitErr(void *handle, const char *fname, uint32 line);
void Bcm7xxxTrace(void *handle, const char *fname, uint32 line);



#ifdef BCM7XXX_DBG_ENABLED
#define BCM7XXX_TRACE() Bcm7xxxTrace(NULL, __FUNCTION__, __LINE__);
#define BCM7XXX_ENTER() Bcm7xxxEnter(NULL, __FUNCTION__, __LINE__);
#define BCM7XXX_EXIT() Bcm7xxxExit(NULL, __FUNCTION__, __LINE__);
#define BCM7XXX_EXIT_ERR() Bcm7xxxExitErr(NULL, __FUNCTION__, __LINE__);
#define BCM7XXX_ERR(args)   do {Bcm7xxxPrintSpace(NULL); printk args;} while (0)
#define BCM7XXX_DBG(args)   do {Bcm7xxxPrintSpace(NULL); printk args;} while (0)
#else /* !BCM7XXX_DBG_ENABLED */
#define BCM7XXX_TRACE()
#define BCM7XXX_ENTER()
#define BCM7XXX_EXIT()
#define BCM7XXX_EXIT_ERR()
#define BCM7XXX_ERR(args)   do {Bcm7xxxPrintSpace(NULL); printk args;} while (0)
#define BCM7XXX_DBG(args)
#endif /* BCM7XXX_DBG_ENABLED */

#endif /* WL_BCM7XXX_DBG_H */
