/*
 * Broadcom DHCP Server
 * OS specific definitions.
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * $Id: osl.h,v 1.6 2010-05-03 07:15:53 $
 */

#if defined(TARGETENV_android) || defined(TARGETOS_symbian)
#include <unistd.h>
#include <netinet/in.h>
#endif

unsigned long OslGetSeconds();
void OslHandleAssert(char *fileName, int Line);

void *OslCreateLock();
void OslDeleteLock(void *);
void OslLock(void *);
void OslUnlock(void *);
