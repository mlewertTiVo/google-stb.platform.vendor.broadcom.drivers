/*
 * Copyright 2016, Broadcom Corporation
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

#include <wl_bcm7xxx_dbg.h>
#include <osl.h>

#define BCM7XXX_OUT_CHAR 3
static int32 gSpace = 0;

void Bcm7xxxPrintSpace(void *handle)
{
	int32 ii;

	for (ii = 0; ii < gSpace; ii++)
		printk(" ");
}

void Bcm7xxxEnter(void *handle, const char *fname, uint32 line)
{
	int32 ii;

	for (ii = 0; ii < gSpace; ii++)
		printk(" ");
	printk("-->%s(%d)\n", fname, line);
	gSpace += BCM7XXX_OUT_CHAR;
	if (gSpace > 30)
		gSpace = 30;
}

void Bcm7xxxExit(void *handle, const char *fname, uint32 line)
{
	int32 ii;

	gSpace -= BCM7XXX_OUT_CHAR;
	if (gSpace < 0)
		gSpace = 0;
	for (ii = 0; ii < gSpace; ii++)
		printk(" ");
	printk("<--%s(%d)\n", fname, line);
}

void Bcm7xxxExitErr(void *handle, const char *fname, uint32 line)
{
	int32 ii;

	gSpace -= BCM7XXX_OUT_CHAR;
	if (gSpace < 0)
		gSpace = 0;
	for (ii = 0; ii < gSpace; ii++)
		printk(" ");
	printk("<--***ERROR*** %s(%d)\n", fname, line);
}

void Bcm7xxxTrace(void *handle, const char *fname, uint32 line)
{
	int32 ii;

	for (ii = 0; ii < gSpace; ii++)
		printk(" ");
	printk("%s(%d)\n", fname, line);
}
