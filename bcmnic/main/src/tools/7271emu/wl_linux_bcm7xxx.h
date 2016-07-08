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

#ifndef WL_LINUX_BCM7XXX_H
#define WL_LINUX_BCM7XXX_H
#include <typedefs.h>
#include <linuxver.h>
#include <linux/module.h>
#include <linux/netdevice.h>

/* driver includes */
#include <osl.h>
#include <wlioctl.h>
#include <wlc_cfg.h> /* important include, has compiler specific defines */


// *****************************************************************
// * WEBHIF_CPU_INTR1_INTR_W3_STATUS - Interrupt Status Register
// * Offset byte space = 32'h2045460c
// * Physical Address byte space = 32'hf045460c
// * Verilog Macro Address = `WEBHIF_CPU_INTR1_REG_START + `HIF_CPU_INTR1_INTR_W3_STATUS
// * Reset Value = 32'bxxxxxxxx_xxxxxxxx_xxxx0000_00000000
// * Access = RO(32-bit only)
// * NOTE: For each bit: 1 = asserted. 0 = not asserted
// *
// * WEBHIF_CPU_INTR1_INTR_W0_MASK_STATUS - Interrupt Mask Status Register
// * Offset byte space = 32'h20454610
// * Physical Address byte space = 32'hf0454610
// * Verilog Macro Address = `WEBHIF_CPU_INTR1_REG_START + `HIF_CPU_INTR1_INTR_W0_MASK_STATUS
// * Reset Value = 32'hffff_ffff
// * Access = RO(32-bit only)
// *
// * NOTE: For each bit: 1 = masked. 0 = not masked.
// * NOTE: Masked means interrupt is not enabled/used.
// *
// * WEBHIF_CPU_INTR1_INTR_W1_MASK_STATUS - Interrupt Mask Status Register
// * Offset byte space = 32'h20454614
// * Physical Address byte space = 32'hf0454614
// * Verilog Macro Address = `WEBHIF_CPU_INTR1_REG_START + `HIF_CPU_INTR1_INTR_W1_MASK_STATUS
// * Reset Value = 32'hffff_ffff
// * Access = RO(32-bit only)
// * NOTE: write 1 to disable the interrupt; 0 has no effect
// *
// * WEBHIF_CPU_INTR1_INTR_W0_MASK_SET - Interrupt Mask Set Register
// * Offset byte space = 32'h20454620
// * Physical Address byte space = 32'hf0454620
// * Verilog Macro Address = `WEBHIF_CPU_INTR1_REG_START + `HIF_CPU_INTR1_INTR_W0_MASK_SET
// * Reset Value = 32'hffff_ffff
// * Access = WO(32-bit only)
// * NOTE: write 1 to enable the interrupt; 0 has no effect
// *****************************************************************

#define WLAN_D2H_CPU_INTR 133 /* 5 + 3 * 32 = 101 + 32 (ARM GIC pre-processor) = 133 */

#define BCM7271_L1_IRQ_NUM	WLAN_D2H_CPU_INTR

#define WLAN_INTF_START	0xf17e0000 /* offset = 0x217e0000 */ 
#define WLAN_INTF_END		0xf17e0400 /* offset = 0x217e0400 */
#define WLAN_INTF_SIZE		(WLAN_INTF_END - WLAN_INTF_START)

/***************************************************************************
 *HIF_INTR2 - HIF Level 2 Interrupt Controller Registers
 ***************************************************************************/
#define BCHP_HIF_INTR2_CPU_STATUS                0x20201000 /* [RO] CPU interrupt Status Register */
#define BCHP_HIF_INTR2_CPU_SET                   0x20201004 /* [WO] CPU interrupt Set Register */
#define BCHP_HIF_INTR2_CPU_CLEAR                 0x20201008 /* [WO] CPU interrupt Clear Register */
#define BCHP_HIF_INTR2_CPU_MASK_STATUS           0x2020100c /* [RO] CPU interrupt Mask Status Register */
#define BCHP_HIF_INTR2_CPU_MASK_SET              0x20201010 /* [WO] CPU interrupt Mask Set Register */
#define BCHP_HIF_INTR2_CPU_MASK_CLEAR            0x20201014 /* [WO] CPU interrupt Mask Clear Register */
#define BCHP_HIF_INTR2_PCI_STATUS                0x20201018 /* [RO] PCI interrupt Status Register */
#define BCHP_HIF_INTR2_PCI_SET                   0x2020101c /* [WO] PCI interrupt Set Register */
#define BCHP_HIF_INTR2_PCI_CLEAR                 0x20201020 /* [WO] PCI interrupt Clear Register */
#define BCHP_HIF_INTR2_PCI_MASK_STATUS           0x20201024 /* [RO] PCI interrupt Mask Status Register */
#define BCHP_HIF_INTR2_PCI_MASK_SET              0x20201028 /* [WO] PCI interrupt Mask Set Register */
#define BCHP_HIF_INTR2_PCI_MASK_CLEAR            0x2020102c /* [WO] PCI interrupt Mask Clear Register */


/***************************************************************************
 *WLAN_INTF_D2H_INTR2
 ***************************************************************************/
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_STATUS      0x217e0100 /* [RO] CPU interrupt Status Register */
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_SET         0x217e0104 /* [WO] CPU interrupt Set Register */
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_CLEAR       0x217e0108 /* [WO] CPU interrupt Clear Register */
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_MASK_STATUS 0x217e010c /* [RO] CPU interrupt Mask Status Register */
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_MASK_SET    0x217e0110 /* [WO] CPU interrupt Mask Set Register */
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_MASK_CLEAR  0x217e0114 /* [WO] CPU interrupt Mask Clear Register */
#define BCHP_WLAN_INTF_D2H_INTR2_PCI_STATUS      0x217e0118 /* [RO] PCI interrupt Status Register */
#define BCHP_WLAN_INTF_D2H_INTR2_PCI_SET         0x217e011c /* [WO] PCI interrupt Set Register */
#define BCHP_WLAN_INTF_D2H_INTR2_PCI_CLEAR       0x217e0120 /* [WO] PCI interrupt Clear Register */
#define BCHP_WLAN_INTF_D2H_INTR2_PCI_MASK_STATUS 0x217e0124 /* [RO] PCI interrupt Mask Status Register */
#define BCHP_WLAN_INTF_D2H_INTR2_PCI_MASK_SET    0x217e0128 /* [WO] PCI interrupt Mask Set Register */
#define BCHP_WLAN_INTF_D2H_INTR2_PCI_MASK_CLEAR  0x217e012c /* [WO] PCI interrupt Mask Clear Register */

#define BCHP_WLAN_INTF_D2H_INTR2_REGMASK	0xFFFFF000
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_STATUS_OFFSET	0x100
#define BCHP_WLAN_INTF_D2H_INTR2_CPU_CLEAR_OFFSET	0x108


typedef struct BCM7XXX_Data {
	void *hRegister;	/* [Input] Handle to access regiters. */
	void *pwl; 		/* [Input] Wlan global data structure */
	void *hWlanIntf; 	/* [Input] WLAN interface */
} BCM7XXX_Data;

typedef struct BCM7XXX_Data *BCM7XXX_Handle;

/***************************************************************************
 *BOLT device tree files
 ***************************************************************************/
#define BOLT_DEVICE_TREE_PATH_BOARDNAME "/proc/device-tree/bolt/board"

#define BCM9271WLAN_BOARDNAME "BCM97271WLAN"
#define BCM9271IP_BOARDNAME	"BCM97271IP"
#define BCM9271IPC_BOARDNAME "BCM97271IPC"
#define BCM9271C_BOARDNAME "BCM97271C"
#define BCM9271D_BOARDNAME "BCM97271D"
#define BCM9271T_BOARDNAME "BCM97271T"
#define BCM9271DV_BOARDNAME "BCM97271DV"
#define BCM9271IP_V20_BOARDNAME "BCM97271IP_V20"
#define BCM9271IPC_V20_BOARDNAME "BCM97271IPC_V20"

int wl_bcm7xxx_init(void);
void wl_bcm7xxx_deinit(void);


int wl_bcm7xxx_request_irq(void *handle, uint32 irqnum, const char *name);
void wl_bcm7xxx_free_irq(void *handle, uint32 irqnum, void *dev_id);

BCM7XXX_Handle wl_bcm7xxx_attach(void *pwl);
void wl_bcm7xxx_detach(BCM7XXX_Handle bcm7xxx);
int wl_bcm7xxx_open(struct net_device *dev);
int wl_bcm7xxx_close(struct net_device *dev);
irqreturn_t wl_bcm7xxx_isr(int irq, void *handle);
void wl_bcm7xxx_interrupt_enable(void);
void wl_bcm7xxx_interrupt_disable(void);
void wl_bcm7xxx_d2h_int2_isr(void);
void* wl_bcm7xxx_get_justy_addr(void);
#endif /* WL_LINUX_BCM7XXX_H */
