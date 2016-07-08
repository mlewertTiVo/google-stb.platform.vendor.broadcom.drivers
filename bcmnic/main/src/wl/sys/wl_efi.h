/*
 * wl_efi.c exported functions and definitions
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_efi.h 467328 2014-04-03 01:23:40Z $
 */

#ifndef _WL_EFI_H_
#define _WL_EFI_H_

#include "Tiano.h"
#include "EfiDriverLib.h"
#include "Pci22.h"
#include "EfiPrintLib.h"
#include "EfiStdArg.h"
#include "EfiPrintLib.h"
#ifdef APPLE_BUILD
#include EFI_PROTOCOL_DEFINITION(Apple80211)
#include EFI_PROTOCOL_DEFINITION(AppleLinkStateProtocol)
#include EFI_PROTOCOL_DEFINITION(AppleAip)
#else
#include "apple80211.h"
#endif

/* The least significant 6-bytes will be the NIC MAC address */
#define BCMWL_GUID \
	{0x21981f75, 0x0e3f, 0x4a04, 0x83, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

#define BCMWL_DRIVER_SIGNATURE EFI_SIGNATURE_32 ('4', '3', 'x', 'x')

/* This is done to avoid having to organize blocks by #ifdef - #else - #endif */
#if !defined(BCMWLUNDI)
#error "BCMWLUNDI has to be defined"
#endif

#ifdef BCMWLUNDI
#include "EfiPxe.h"

#ifndef PXE_OPFLAGS_STATION_ADDRESS_WRITE
#define  PXE_OPFLAGS_STATION_ADDRESS_WRITE 0x0000
#endif

#include EFI_PROTOCOL_DEFINITION(EfiNetworkInterfaceIdentifier)
#endif /* BCMWLUNDI */

#include EFI_PROTOCOL_DEFINITION(DevicePath)
#include EFI_PROTOCOL_DEFINITION(PciRootBridgeIo)
#include EFI_PROTOCOL_DEFINITION(PciIo)
#include EFI_PROTOCOL_DEFINITION(DriverBinding)
#include EFI_PROTOCOL_DEFINITION(LoadedImage)

/*  bit fields for the command */
#define PCI_COMMAND_MASTER  0x04    /*  bit 2 */
#define PCI_COMMAND	0x04

#endif /* _WL_EFI_H_ */
