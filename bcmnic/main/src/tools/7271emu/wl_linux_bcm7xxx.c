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

#include <typedefs.h>
#include <linuxver.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include<linux/irq.h>
#include <osl.h>
#include <wlioctl.h>
#include <wlc_types.h> /* all the forwarding types */
#include <wlc_cfg.h> /* important include, has compiler specific defines */
#include <wl_linux_bcm7xxx.h>
#include <linux/vmalloc.h>
#include <bcmdevs.h> /* contains chip enum and base address */
#include <hndsoc.h> /* contains chip enum and base address */
#include <wlan_emu.h> /* contains emulation API */

//#define USE_CHAR_DRIVER 1
#ifdef USE_CHAR_DRIVER
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/types.h>
#endif /* USE_CHAR_DRIVER */

#ifndef BCM7XXX_DBG_ENABLED
#define BCM7XXX_DBG_ENABLED
#endif /* BCM7XXX_DBG_ENABLED */
#include <wl_bcm7xxx_dbg.h>

struct wl_info;
typedef struct wl_info wl_info_t;


/* Global handle for the bcm7xxx driver */
BCM7XXX_Handle g_bcm7xxx;

#ifdef BCM7271_EMU
int32 gEnableInterrupt = 0;
#endif /* BCM7271_EMU */

#define PROC_ENTRY_SIZE 	256
char g_proc_entry[PROC_ENTRY_SIZE];
uint g_proc_entry_size = PROC_ENTRY_SIZE;

#define MAXSZ_DEVICE_VARS 256
char device_var[MAXSZ_DEVICE_VARS];

extern wl_info_t *wl_attach(uint16 vendor, uint16 device, ulong regs,
	uint bustype, void *btparam, uint irq,
	uchar* bar1_addr, uint32 bar1_size, void *cmndata);


extern void wl_down(wl_info_t *wl);
extern void wl_free(wl_info_t *wl);
extern void wl_lock(wl_info_t *wl);
extern void wl_unlock(wl_info_t *wl);
extern char* wl_interface_get_name(wl_info_t *wl);
extern int wl_interface_get_unit(wl_info_t *wl);

static int wl_bcm7xxx_device_tree_lookup(char *device_str, char **devp);
uint16 wl_bcm7xxx_get_device_id(char *dev);


#if !defined(BCM7271_EMU)
/* HACKHACK - TUAN */
/* HACK FOR NOW TO GET AROUND LINKING ISSUE */
uint32 emu_get_remote_nfifo_addr(uint64 local);
uint32 emu_get_remote_nfifo_addr(uint64 local) {
	return((uint32)(local & 0xFFFFFFFF));
}
#endif /* defined(BCM7271) */



#ifdef BCM7271_EMU
#ifdef BCM7271_EMU_VELOCE_64BIT
void* emu64_veloce_ioremap(uint64 pa, uint32 size);
uint64 emu64_get_remote_nfifo_addr(uint64 local);
#else
void* emu_veloce_ioremap(uint32 pa, uint32 size);
uint32 emu_get_remote_nfifo_addr(uint64 local);
#endif /* BCM7271_EMU_VELOCE_64BIT */
void emu_veloce_iounmap(void *va);
uint8  emu_veloce_read8(void* handle, volatile uint8* reg);
uint16 emu_veloce_read16(void* handle, volatile uint16* reg);
uint32 emu_veloce_read32(void* handle, volatile uint32* reg);
void emu_veloce_write8(void* handle, volatile uint8* reg, uint8 val);
void emu_veloce_write16(void* handle, volatile uint16* reg, uint16 val);
void emu_veloce_write32(void* handle, volatile uint32* reg, uint32 val);

#if defined(BCM7271_EMU)
void emu_wl_isr(int irq);
extern void emu_free_all_remote_nfifo(void);
extern void emu_free_all_remote_pkt(void);

#ifdef BCM7271_EMU_VELOCE_64BIT
void* emu_veloce_ioremap(uint64 pa, uint32 size)
{
//	BCM7XXX_DBG(("%s(%d) 0x%08x 0x%08x\n", __FUNCTION__, __LINE__, pa, size));
	return ((void*)pa);
}
#else
void* emu_veloce_ioremap(uint32 pa, uint32 size)
{
//	BCM7XXX_DBG(("%s(%d) 0x%08x 0x%08x\n", __FUNCTION__, __LINE__, pa, size));
	return ((void*)pa);
}
#endif /* BCM7271_EMU_VELOCE_64BIT */

void emu_veloce_iounmap(void *va)
{
//	BCM7XXX_DBG(("%s(%d) 0x%p\n", __FUNCTION__, __LINE__, va));
}
#endif /* defined(BCM7271_EMU) */


uint8  emu_veloce_read8(void* handle, volatile uint8* reg)
{
	wlan_emu_reg	emu_reg;
	int				err;

	BCM_REFERENCE(handle);

#ifdef REG_DEBUGXXX
	BCM7XXX_DBG(("%s(%d) 0x%p\n", __FUNCTION__, __LINE__, reg));
#endif /* REG_DEBUG */

//	emu_reg.data = readb((volatile uint8*) (reg));
//	emu_reg.data = readw((volatile uint16*)(reg)); 
//	emu_reg.data = readl((volatile uint32*)(reg));

	emu_reg.size = 1; /* 8-bits access */
#ifdef BCM7271_EMU_VELOCE_64BIT
	emu_reg.offset = (uint64)reg;
#else
	emu_reg.offset = (uint32)reg;
#endif

#if defined(BCM7271_EMU)
	err = wlan_emu_reg_read(&emu_reg);
#else /* !defined(BCM7271_EMU) */
	emu_reg.data = *((volatile uint8*) (reg));
	err = 0;
#endif /* defined(BCM7271_EMU) */

#ifdef REG_DEBUG
		BCM7XXX_DBG(("%s(%d) 0x%p 0x%08x\n", __FUNCTION__, __LINE__, reg, (uint32)emu_reg.data));
#endif /* REG_DEBUG */

	return ((uint8)(emu_reg.data&0xFF));
}

uint16 emu_veloce_read16(void* handle, volatile uint16* reg)
{
	wlan_emu_reg	emu_reg;
	int				err;

	BCM_REFERENCE(handle);

#ifdef REG_DEBUGXXX
		BCM7XXX_DBG(("%s(%d) 0x%p\n", __FUNCTION__, __LINE__, reg));
#endif /* REG_DEBUG */

//	emu_reg.data = readb((volatile uint8*) (reg));
//	emu_reg.data = readw((volatile uint16*)(reg)); 
//	emu_reg.data = readl((volatile uint32*)(reg));

	emu_reg.size = 2; /* 16-bits access */
#ifdef BCM7271_EMU_VELOCE_64BIT
	emu_reg.offset = (uint64)reg;
#else
	emu_reg.offset = (uint32)reg;
#endif

#if defined(BCM7271_EMU)
		err = wlan_emu_reg_read(&emu_reg);
#else /* !defined(BCM7271_EMU) */
		emu_reg.data = *((volatile uint16*) (reg));
		err = 0;
#endif /* defined(BCM7271_EMU) */
	
#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s(%d) 0x%p 0x%08x\n", __FUNCTION__, __LINE__, reg, (uint32)emu_reg.data));
#endif /* REG_DEBUG */

	return ((uint16)(emu_reg.data&0xFFFF));
}
uint32 emu_veloce_read32(void* handle, volatile uint32* reg)
{
	uint32			value;
	wlan_emu_reg	emu_reg;
	int				err;
	BCM_REFERENCE(handle);

#ifdef REG_DEBUGXXX
	BCM7XXX_DBG(("%s(%d) 0x%p\n", __FUNCTION__, __LINE__, reg));
#endif /* REG_DEBUG */

//	emu_reg.data = readb((volatile uint8*) (reg));
//	emu_reg.data = readw((volatile uint16*)(reg)); 
//	emu_reg.data = readl((volatile uint32*)(reg));

	emu_reg.size = 4; /* 32-bits access */
#ifdef BCM7271_EMU_VELOCE_64BIT
	emu_reg.offset = (uint64)reg;
#else
	emu_reg.offset = (uint32)reg;
#endif

#if defined(BCM7271_EMU)
		err = wlan_emu_reg_read(&emu_reg);
#else /* !defined(BCM7271_EMU) */
		emu_reg.data = *((volatile uint32*) (reg));
		err = 0;
#endif /* defined(BCM7271_EMU) */

	value = (uint32)(emu_reg.data&0xFFFFFFFF);

/* DEBUG ONLY */
#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s(%d) 0x%p 0x%08x\n", __FUNCTION__, __LINE__, reg, value));
#endif /* REG_DEBUG */

	return (value);
}

void emu_veloce_write8(void* handle, volatile uint8* reg, uint8 val)
{
	wlan_emu_reg	emu_reg;
	int 			err;

	BCM_REFERENCE(handle);

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s(%d) 0x%p 0x%08x\n", __FUNCTION__, __LINE__, reg, val));
#endif /* REG_DEBUG */

	//writeb((uint8)(val),  (volatile uint8*) (reg));
	//writew((uint16)(val), (volatile uint16*)(reg));
	//writel((uint32)(val), (volatile uint32*)(reg));

	emu_reg.size = 1;
#ifdef BCM7271_EMU_VELOCE_64BIT
	emu_reg.offset = (uint64)reg;
#else
	emu_reg.offset = (uint32)reg;
#endif
	emu_reg.data = val;

#if defined(BCM7271_EMU)
	err = wlan_emu_reg_write(&emu_reg);
#else /* !defined(BCM7271_EMU) */
	*((volatile uint8*)  (reg)) = (uint8)(val);
	err = 0;
#endif /* defined(BCM7271_EMU) */

}
void emu_veloce_write16(void* handle, volatile uint16* reg, uint16 val)
{
	wlan_emu_reg	emu_reg;
	int 			err;

	BCM_REFERENCE(handle);

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s(%d) 0x%p 0x%08x\n", __FUNCTION__, __LINE__, reg, val));
#endif /* REG_DEBUG */
	//writeb((uint8)(val),  (volatile uint8*) (reg));
	//writew((uint16)(val), (volatile uint16*)(reg));
	//writel((uint32)(val), (volatile uint32*)(reg));

	emu_reg.size = 2;
#ifdef BCM7271_EMU_VELOCE_64BIT
	emu_reg.offset = (uint64)reg;
#else
	emu_reg.offset = (uint32)reg;
#endif
	emu_reg.data = val;

#if defined(BCM7271_EMU)
		err = wlan_emu_reg_write(&emu_reg);
#else /* !defined(BCM7271_EMU) */
		*((volatile uint16*)  (reg)) = (uint16)(val);
		err = 0;
#endif /* defined(BCM7271_EMU) */

}

void emu_veloce_write32(void* handle, volatile uint32* reg, uint32 val)
{
	wlan_emu_reg	emu_reg;
	int				err;

	BCM_REFERENCE(handle);

#ifdef REG_DEBUG
		BCM7XXX_DBG(("%s(%d) 0x%p 0x%08x\n", __FUNCTION__, __LINE__, reg, val));
#endif /* REG_DEBUG */
	//writeb((uint8)(val),  (volatile uint8*) (reg));
	//writew((uint16)(val), (volatile uint16*)(reg));
	//writel((uint32)(val), (volatile uint32*)(reg));

	emu_reg.size = 4;
#ifdef BCM7271_EMU_VELOCE_64BIT
	emu_reg.offset = (uint64)reg;
#else
	emu_reg.offset = (uint32)reg;
#endif /* BCM7271_EMU_VELOCE_64BIT */
	emu_reg.data = val;
#if defined(BCM7271_EMU)
		err = wlan_emu_reg_write(&emu_reg);
#else /* !defined(BCM7271_EMU) */
		*((volatile uint32*)  (reg)) = (uint32)(val);
		err = 0;
#endif /* defined(BCM7271_EMU) */

}

#if defined(BCM7271_EMU)
dmaaddr_t emu_veloce_malloc(void* handle, size_t len)
{
	wlan_emu_mem mem;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = len;

	err = wlan_emu_remote_malloc(&mem);

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%lx local:0x%lx\n", __FUNCTION__, __LINE__,
		err, (unsigned long)mem.remote_addr, (unsigned long)mem.local_addr));
#endif /* REG_DEBUG */
	return mem.remote_addr;
}

void emu_veloce_free(void* handle, dmaaddr_t pa, size_t len)
{
	wlan_emu_mem mem;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = len;
	mem.remote_addr = pa;

	err = wlan_emu_remote_free(&mem);

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%lx local:0x%lx\n", __FUNCTION__, __LINE__,
		err, (unsigned long)mem.remote_addr, (unsigned long)mem.local_addr));
#endif /* REG_DEBUG */
}

void emu_veloce_memcpy(void* handle, dmaaddr_t local, dmaaddr_t remote, size_t len, int direction)
{
	wlan_emu_mem mem;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = len;
	mem.local_addr = local;
	mem.remote_addr = remote;

	if (direction == DMA_TX)
		err = wlan_emu_mem_write(&mem);
	else if (direction == DMA_RX)
		err = wlan_emu_mem_read(&mem);
	else
		err = -1;

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d direction:%d remote:0x%lx local:0x%lx\n", __FUNCTION__, __LINE__,
		err, direction, (unsigned long)mem.remote_addr, (unsigned long)mem.local_addr));
#endif /* REG_DEBUG */
}
#endif /* defined(BCM7271_EMU) */


#ifdef BCM7271_EMU_VELOCE_64BIT
uint64 emu64_veloce_read_dma_sm(void* handle, uint64* addr)
{
	wlan_emu_mem mem;
	uint64	addr64, *paddr64;
	int err;
	uint64 val;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = 4; /* DMA regs are all 32-bits */

{
	uint64 remote_addr;
	uint64 local_addr;
	local_addr  = (uint64)addr;
	remote_addr = emu64_get_remote_nfifo_addr(local_addr);
	mem.local_addr	= local_addr;
	mem.remote_addr = remote_addr;
}

#if defined(BCM7271_EMU)
	err = wlan_emu_mem_read(&mem);
#else
	err = 0;
#endif /* defined(BCM7271_EMU) */


	/* retrieve the data and return it */
	addr64   = mem.local_addr ;
	paddr64  = (uint64*) addr64;
	val      = *paddr64;

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%llx local:0x%llx val:0x%08llx\n", __FUNCTION__, __LINE__,
		err, (unsigned long)mem.remote_addr, (unsigned long)mem.local_addr, val));
#endif /* REG_DEBUG */

	return val;
}

void emu64_veloce_write_dma_sm(void* handle, uint64* addr, uint32 val)
{
	wlan_emu_mem mem;
	int err;
	uint64	addr64, *paddr64;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));

	mem.len = 4;

{
	uint64 remote_addr;
	uint64 local_addr;

	local_addr  = (uint64)addr;
	remote_addr = emu64_get_remote_nfifo_addr(local_addr);
	mem.local_addr	= local_addr;
	mem.remote_addr = remote_addr;
}

	/* write the value into the local buffer */
	addr64   = mem.local_addr;
	paddr64  = (uint64*) addr64;
	*paddr64 = val;
	
#if defined(BCM7271_EMU)
	err = wlan_emu_mem_write(&mem);
#else
	err = 0;
#endif /* defined(BCM7271_EMU) */

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%llx local:0x%llx val:0x%08llx\n", __FUNCTION__, __LINE__,
		err, mem.remote_addr, mem.local_addr, val));
#endif /* REG_DEBUG */

}

void emu64_veloce_bzero(void* handle, uint64* local, size_t len)
{
	wlan_emu_mem mem;
	int err;
	uint64 remote_addr;
	uint64 local_addr;
	uint32 ii = 0;

	BCM_REFERENCE(handle);

	memset(local, '\0', len);


	local_addr  = (uint64)local;
	remote_addr = emu64_get_remote_nfifo_addr(local_addr);

	if ((local_addr & 3) || (remote_addr & 3)) {
		BCM7XXX_ERR(("%s(%d) not multiple of 4 bytes local=0x%08llx remote=0x%08llx\n",
			__FUNCTION__, __LINE__, local_addr, remote_addr));
		return;
	}
	if (remote_addr) {
		len = (len + PAGE_SIZE -1) >> PAGE_SHIFT; /* convert byte to 32-bit value */
		for (ii = 0; ii < len; ii++) {
			mem.len = PAGE_SIZE;
			mem.local_addr  = local_addr;
			mem.remote_addr = remote_addr;
	
#if defined(BCM7271_EMU)
		err = wlan_emu_mem_write(&mem);
#else
		err = 0;
#endif /* defined(BCM7271_EMU) */
	
			
			remote_addr += PAGE_SIZE; /* next PAGE_SIZE value */
			local_addr  += PAGE_SIZE; /* next PAGE_SIZE value */
		}
	} else {
		BCM7XXX_ERR(("%s(%d) unknown local=0x%08llx remote=0x%08llx\n",
			__FUNCTION__, __LINE__, local_addr, remote_addr));
	}

}

void emu64_veloce_read_dma_buf(void* handle, uint64* local, uint64* remote, uint32 len)
{
	wlan_emu_mem mem;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = len;
	mem.local_addr = (uint64)local;
	mem.remote_addr = (uint64)remote;

#if defined(BCM7271_EMU)
	err = wlan_emu_mem_read(&mem);
#else
	err = 0;
#endif /* defined(BCM7271_EMU) */

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%llx local:0x%llx\n", __FUNCTION__, __LINE__, err,
		mem.remote_addr, mem.local_addr));
#endif /* REG_DEBUG */

	return;
}


void emu64_veloce_write_dma_buf(void* handle, uint64* local, uint64* remote, uint32 len)
{
	wlan_emu_mem mem;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = len;
	mem.local_addr = (uint64)local;
	mem.remote_addr = (uint64)remote;

#if defined(BCM7271_EMU)
		err = wlan_emu_mem_write(&mem);
#else
		err = 0;
#endif /* defined(BCM7271_EMU) */

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%llx local:0x%llx\n", __FUNCTION__, __LINE__, err,
		mem.remote_addr, mem.local_addr));
#endif /* REG_DEBUG */
}

#else /* !BCM7271_EMU_VELOCE_64BIT */

#ifdef BCM7271_EMU
uint32 emu_veloce_read_dma_sm(void* handle, uint32* addr)
{
	wlan_emu_mem mem;
	uint32	val, addr32, *paddr32;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = 4; /* DMA regs are all 32-bits */

{
	uint32 remote_addr;
	uint32 local_addr;
	local_addr  = (uint32)addr;
	remote_addr = (uint32)emu_get_remote_nfifo_addr(local_addr);
	mem.local_addr	= local_addr;
	mem.remote_addr = remote_addr;
}

	err = wlan_emu_mem_read(&mem);

	/* retrieve the data and return it */
	addr32   = mem.local_addr & 0xFFFFFFFF;
	paddr32  = (uint32*) addr32;
	val      = *paddr32;

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%lx local:0x%lx val:0x%08x\n", __FUNCTION__, __LINE__,
		err, (unsigned long)mem.remote_addr, (unsigned long)mem.local_addr, val));
#endif /* REG_DEBUG */

	return val;
}

void emu_veloce_write_dma_sm(void* handle, uint32* addr, uint32 val)
{
	wlan_emu_mem mem;
	int err;
	uint32	addr32, *paddr32;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));

	mem.len = 4;

{
	uint32 remote_addr;
	uint32 local_addr;

	local_addr  = (uint32)addr;
	remote_addr = (uint32)emu_get_remote_nfifo_addr(local_addr);
	mem.local_addr	= local_addr;
	mem.remote_addr = remote_addr;
}

	/* write the value into the local buffer */
	addr32   = mem.local_addr & 0xFFFFFFFF;
	paddr32  = (uint32*) addr32;
	*paddr32 = val;

	err = wlan_emu_mem_write(&mem);

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%lx local:0x%lx val:0x%08x\n", __FUNCTION__, __LINE__,
		err, (unsigned long)mem.remote_addr, (unsigned long)mem.local_addr, val));
#endif /* REG_DEBUG */

}


void emu_veloce_bzero(void* handle, uint32* local, size_t len)
{
	wlan_emu_mem mem;
	int err;
	uint32 remote_addr;
	uint32 local_addr;
	uint32 ii = 0;

	BCM_REFERENCE(handle);

	memset(local, '\0', len);


	local_addr  = (uint32)local;
	remote_addr = (uint32)emu_get_remote_nfifo_addr(local_addr);

	if ((local_addr & 3) || (remote_addr & 3)) {
		BCM7XXX_ERR(("%s(%d) not multiple of 4 bytes local=0x%08x remote=0x%08x\n",
			__FUNCTION__, __LINE__, local_addr, remote_addr));
		return;
	}
	if (remote_addr) {
		len = (len + PAGE_SIZE -1) >> PAGE_SHIFT; /* convert byte to 32-bit value */
		for (ii = 0; ii < len; ii++) {
			mem.len = PAGE_SIZE;
			mem.local_addr  = local_addr;
			mem.remote_addr = remote_addr;
			err = wlan_emu_mem_write(&mem);
			
			remote_addr += PAGE_SIZE; /* next PAGE_SIZE value */
			local_addr  += PAGE_SIZE; /* next PAGE_SIZE value */
		}
	} else {
		BCM7XXX_ERR(("%s(%d) unknown local=0x%08x remote=0x%08x\n",
			__FUNCTION__, __LINE__, local_addr, remote_addr));
	}

}


void emu_veloce_read_dma_buf(void* handle, uint32* local, uint32* remote, uint32 len)
{
	wlan_emu_mem mem;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = len;
	mem.local_addr = (uint32)local;
	mem.remote_addr = (uint32)remote;

	err = wlan_emu_mem_read(&mem);

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%lx local:0x%lx\n", __FUNCTION__, __LINE__, err,
		(unsigned long)mem.remote_addr, (unsigned long)mem.local_addr));
#endif /* REG_DEBUG */

	return;
}

void emu_veloce_write_dma_buf(void* handle, uint32* local, uint32* remote, uint32 len)
{
	wlan_emu_mem mem;
	int err;

	BCM_REFERENCE(handle);
	bzero(&mem, sizeof(wlan_emu_mem));
	mem.len = len;
	mem.local_addr = (uint32)local;
	mem.remote_addr = (uint32)remote;

	err = wlan_emu_mem_write(&mem);

#ifdef REG_DEBUG
	BCM7XXX_DBG(("%s:%d err:%d remote:0x%lx local:0x%lx\n", __FUNCTION__, __LINE__, err,
		(unsigned long)mem.remote_addr, (unsigned long)mem.local_addr));
#endif /* REG_DEBUG */
}

#endif /* BCM7271_EMU */

#endif /* BCM7271_EMU_VELOCE_64BIT */
#endif /* BCM7271_EMU */


#ifdef BCM7271_EMU
void emu_wl_isr(int irq)
{
	irqreturn_t	irqret;

	irqret = wl_bcm7xxx_isr(irq, g_bcm7xxx);

	BCM7XXX_TRACE();
	return;
}
EXPORT_SYMBOL(emu_wl_isr);
#endif /* BCM7271_EMU */

/**  file read for platform info **/
static int
wl_bcm7xxx_device_tree_lookup(char *device_str, char **devp)
{
	void	*dev_tree_fp = NULL;
	char 	*dev_buf = *devp;
	int ret = 0, len = 0;
	
	dev_tree_fp = (void*)osl_os_open_image(device_str);
	if (dev_tree_fp != NULL) {
		if (!(len = osl_os_get_image_block(dev_buf, MAXSZ_DEVICE_VARS, dev_tree_fp))) {
			BCM7XXX_ERR(("Could not read %s file\n", device_str));
			ret = -1;
			goto exit;
		}
		BCM7XXX_DBG(("Filepath:%s Len:%d Boardname:%s\n", device_str, len, dev_buf));
	}
	else {
		BCM7XXX_ERR(("Could not open %s\n", device_str));
		ret = -1;
	}

exit:
	if (dev_tree_fp)
		osl_os_close_image(dev_tree_fp);

	return ret;

}

uint16
wl_bcm7xxx_get_device_id(char *dev)
{
	uint16 device_id;
	
	if (!strcmp(dev,BCM9271WLAN_BOARDNAME)){
		device_id = BCM7271_D11AC_ID; /* 0x4410, 7271 802.11ac dualband device */		
	} else if (!strcmp(dev,BCM9271IP_BOARDNAME)){
		device_id = BCM7271_D11AC5G_ID; /* 0x4412, 7271 802.11ac 5G device */
	} else if (!strcmp(dev,BCM9271IPC_BOARDNAME)){
		device_id = BCM7271_D11AC5G_ID;
	} else if (!strcmp(dev,BCM9271C_BOARDNAME)){
		device_id = BCM7271_D11AC5G_ID;
	} else if (!strcmp(dev,BCM9271D_BOARDNAME)){
		device_id = BCM7271_D11AC5G_ID;
	} else if (!strcmp(dev,BCM9271T_BOARDNAME)){
		device_id = BCM7271_D11AC5G_ID;
	} else if (!strcmp(dev,BCM9271DV_BOARDNAME)){
		device_id = BCM7271_D11AC5G_ID;
	} else if (!strcmp(dev,BCM9271IP_V20_BOARDNAME)){
		device_id = BCM7271_D11AC_ID;
	} else if (!strcmp(dev,BCM9271IPC_V20_BOARDNAME)){
		device_id = BCM7271_D11AC_ID;
	} else {		
		device_id = BCM7271_D11AC_ID; /* default to dualband */
	}

	BCM7XXX_DBG(("Boardname:%s DeviceID:0x%x\n", dev, device_id));

	return device_id;
}


void* wl_bcm7xxx_get_justy_addr(void)
{
	return (g_bcm7xxx->hRegister);
}

/*
 * wl_bcm7xxx_attach:
 * extended hook for device attach
 */
BCM7XXX_Handle wl_bcm7xxx_attach(void *pwl)
{
	BCM7XXX_Handle 	bcm7xxx = NULL;

	bcm7xxx = (BCM7XXX_Handle)kmalloc(sizeof(BCM7XXX_Data), GFP_ATOMIC);

	if (bcm7xxx)
	{
		BCM7XXX_TRACE();
		bcm7xxx->hRegister = REG_MAP(SI_ENUM_BASE, SI_REG_BASE_SIZE);;
		bcm7xxx->pwl = pwl;
		bcm7xxx->hWlanIntf = REG_MAP(WLAN_INTF_START, WLAN_INTF_SIZE);
		
		if (bcm7xxx->hWlanIntf== NULL) {
			BCM7XXX_ERR(("%s(%d): failed to may WLAN INTF\n",
				__FUNCTION__, __LINE__));
			goto ERROR;
		}

		g_bcm7xxx = bcm7xxx;
	}

	BCM7XXX_DBG(("%s(%d) WLAN_INTF_JUSTY PHY=0x%08x VIR=0x%p SIZE=0x%08x\n",
		__FUNCTION__, __LINE__, SI_ENUM_BASE, bcm7xxx->hRegister, SI_REG_BASE_SIZE));

	BCM7XXX_DBG(("%s(%d) WLAN_INTF PHY=0x%08x VIR=0x%p SIZE=0x%08x\n",
		__FUNCTION__, __LINE__, WLAN_INTF_START, bcm7xxx->hWlanIntf, WLAN_INTF_SIZE));
	
ERROR:
	return bcm7xxx;
}

/*
 * wl_bcm7xxx_detach:
 * extended hook for device attach
 */
void wl_bcm7xxx_detach(BCM7XXX_Handle bcm7xxx)
{
	if (bcm7xxx)
	{
		BCM7XXX_TRACE();

		if (bcm7xxx->hRegister)
		{
			REG_UNMAP(bcm7xxx->hRegister);
			bcm7xxx->hRegister = NULL;
		}

		if (bcm7xxx->hWlanIntf)
		{
			REG_UNMAP(bcm7xxx->hWlanIntf);
			bcm7xxx->hWlanIntf = NULL;
		}

		/* Clear this handle but dont free it.  It wl layer will free it */
		bcm7xxx->pwl = NULL;
		g_bcm7xxx = NULL;
	}

	return;
}


/*
 * wl_bcm7xxx_open:
 * extended hook for device open.
 */
int wl_bcm7xxx_open(struct net_device *dev)
{
	BCM_REFERENCE(dev);
	BCM7XXX_DBG(("%s(%d)\n", __FUNCTION__, __LINE__));
	return 0;
}

/*
 * wl_bcm7xxx_close:
 * extended hook for device close.
 */
int wl_bcm7xxx_close(struct net_device *dev)
{
	BCM7XXX_TRACE();
	BCM7XXX_DBG(("%s(%d)\n", __FUNCTION__, __LINE__));
	return 0;
}


void wl_bcm7xxx_interrupt_enable(void) 
{
	volatile uint32 *pReg = g_bcm7xxx->hWlanIntf;
	uint32	offset;

	/* clear wlan interface mask to enable interrupt */
	offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_MASK_CLEAR & 0x1ff )>>2);
	pReg = pReg + offset ;

	
	//BCM7XXX_DBG(("%s(%d) intf=0x%p offset=0x%08x, pReg=0x%p\n",
	//	__FUNCTION__, __LINE__, g_bcm7xxx->hWlanIntf, offset, pReg));

	*pReg = 0xFFFFF;

	
#ifdef BCM7271_EMU
	gEnableInterrupt = 1;
#endif /* BCM7271_EMU */
}

void wl_bcm7xxx_interrupt_disable(void) 
{
	volatile uint32 *pReg = g_bcm7xxx->hWlanIntf;
	uint32	offset;

	/* clear wlan interface mask to disable interrupt */
	offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_MASK_SET & 0x1ff)>>2);
	pReg = pReg + offset;
	
	//BCM7XXX_DBG(("%s(%d) intf=0x%p offset=0x%08x, pReg=0x%p\n",
	//	__FUNCTION__, __LINE__, g_bcm7xxx->hWlanIntf, offset, pReg));

	*pReg = 0xFFFFF;


#ifdef BCM7271_EMU
	gEnableInterrupt = 0;
#endif /* BCM7271_EMU */
}

void wl_bcm7xxx_d2h_int2_isr(void)
{
	volatile uint32 *pReg = g_bcm7xxx->hWlanIntf;
	volatile uint32 *reg;
	uint32 offset;
	uint32 d2hintstatus;

	offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_STATUS & 0x1ff)>>2);
	reg = pReg + offset;
//	BCM7XXX_ERR(("ENTER intf=0x%p \n", reg));
	
	d2hintstatus = *((volatile uint32*)(reg));
//	BCM7XXX_ERR(("L2 CPU status 0x%08x=0x%08x\n",reg, d2hintstatus));

	/* Check if it's any WLAN D2H INTR2 interrupts */
	if (d2hintstatus & 0xFFFFF) {
		/* Clear WLAN D2H int2 */
		offset = ((BCHP_WLAN_INTF_D2H_INTR2_CPU_CLEAR & 0x1ff)>>2);;
		reg = pReg + offset;
		*((volatile uint32*)(reg)) = d2hintstatus;
	} else {
		BCM7XXX_ERR(("%s:%d ERROR wl_isr triggered but not WLAN_INTF_D2H_INTR2 interrupt\n", __FUNCTION__, __LINE__));
	}

}
extern irqreturn_t wl_isr(int irq, void *dev_id);
/*
 * wl_bcm7xxx_isr - called by the 7xxx L2 ISR:
 */
irqreturn_t wl_bcm7xxx_isr(int irq, void *handle)
{
	irqreturn_t irqret = 0;
	BCM7XXX_Handle bcm7xxx = (BCM7XXX_Handle) handle;

	BCM_REFERENCE(bcm7xxx);



#ifdef BCM7271_EMU
	/* If interrupt is able then call the actual ISR, otherwise ignore the EMU callback */
	if (gEnableInterrupt)
		irqret = wl_isr(irq, bcm7xxx->pwl);
#endif /* BCM7271_EMU */


	return irqret;
}


int wl_bcm7xxx_request_irq(void *handle, uint32 irqnum, const char *name)
{
	int		error = 0;
	BCM7XXX_Handle	bcm7xxx = (BCM7XXX_Handle) handle;

	BCM_REFERENCE(bcm7xxx);
	BCM_REFERENCE(irqnum);
	BCM_REFERENCE(name);

#if defined(BCM7271_EMU)
	irqnum = 0x128 | (SI_ENUM_BASE+0x1000); /* macintstatus=0x128, macintmask=0x12c */
	irqnum = SI_ENUM_BASE; 

	BCM7XXX_DBG(("%s(%d) irqnum==0x%08x\n", __FUNCTION__, __LINE__, irqnum));

	error = wlan_emu_request_irq(irqnum, wl_bcm7xxx_isr, IRQF_SHARED, name, bcm7xxx);

	if (error) {
		BCM7XXX_ERR(("%s(%d) wlan_emu_request_irq() failed\n", __FUNCTION__, __LINE__));
		return (-ENODEV);
	}

#else /* !(defined(BCM7271_EMU)) */
	if (request_irq(irqnum, wl_isr, IRQF_SHARED, name, bcm7xxx->pwl)) {
		BCM7XXX_ERR(("%s(%d) request_irq() failed\n", __FUNCTION__, __LINE__));
		return (-ENODEV);
	}
	BCM7XXX_DBG(("%s(%d) request_irq with irqnum=%d for device %s\n", __FUNCTION__, __LINE__, irqnum, name));
	
#endif /* defined(BCM7271_EMU) */

	return (error);
}


void wl_bcm7xxx_free_irq(void *handle, uint32 irqnum, void *dev_id)
{
	BCM7XXX_Handle	bcm7xxx = (BCM7XXX_Handle) handle;

	BCM_REFERENCE(bcm7xxx);
	BCM_REFERENCE(irqnum);
	BCM_REFERENCE(dev_id);

#if defined(BCM7271_EMU)
	wlan_emu_free_irq(irqnum, dev_id);
#else /* !(defined(BCM7271_EMU)) */
		free_irq(irqnum, dev_id);
		BCM7XXX_DBG(("%s(%d) free_irq with irqnum=%d\n", __FUNCTION__, __LINE__, irqnum));
#endif /* defined(BCM7271_EMU) */
}


#ifdef USE_CHAR_DRIVER
/* client ioctls */
#define WIFI_CHAR_DEV_DRV_DEV_NAME  "wifi_char_dev_drv"
#define WIFI_CHAR_DEV_DRV_DEV_MAGIC 'l'
#define WIFI_CHAR_DEV_DRV_DEV_IOC       _IOWR( WIFI_CHAR_DEV_DRV_DEV_MAGIC, 1, int )  /* ioclt to test server */

typedef struct wifi_char_dev_drv_dev
{
    dev_t			devno;               /* alloted device number */
    struct class	*dev_class;           /* device model */
    struct device	*dev_device;          /* /dev */
    struct cdev		*cdev;                /* char device structure */
} wifi_char_dev_drv_dev_t;

static wifi_char_dev_drv_dev_t *gpDev = NULL;


int wifi_char_dev_drv_init(void);
void wifi_char_dev_drv_deinit(void);
static int wifi_char_dev_drv_open(struct inode *inode, struct file *file);
static int wifi_char_dev_drv_release(struct inode *inode, struct file  *file);
static long wifi_char_dev_drv_ioctl(struct file  *filp, unsigned int  cmd, unsigned long uMsg);

static int wifi_char_dev_drv_open(struct inode *inode, struct file *file)
{
	BCM7XXX_TRACE();
	return( 0 );
}

static int wifi_char_dev_drv_release(struct inode *inode, struct file  *file)
{
	BCM7XXX_TRACE();
	return( 0 );
}

static long wifi_char_dev_drv_ioctl(struct file  *filp, unsigned int  cmd, unsigned long uMsg)
{
	BCM7XXX_TRACE();
	return( 0 );
}

struct file_operations wifi_char_dev_drv_ops = {
    .owner          = THIS_MODULE,
    .open           = wifi_char_dev_drv_open,
    .release        = wifi_char_dev_drv_release,
    .unlocked_ioctl = wifi_char_dev_drv_ioctl,
};


int wifi_char_dev_drv_init(void)
{
	int ret   = 0;
	int minor = 0;

	BCM7XXX_TRACE();

	gpDev = kmalloc( sizeof( wifi_char_dev_drv_dev_t ), GFP_KERNEL );
	memset( gpDev, 0, sizeof( wifi_char_dev_drv_dev_t ));

	if (gpDev == NULL)
	{
	    BCM7XXX_ERR(( "couldn't allocatd wlan_emu_dev\n" ));
	    return( -1 );
	}

	/* request dynamic allocation device major number */
	if (MAJOR( gpDev->devno ) == 0)
	{
	    ret = alloc_chrdev_region( &gpDev->devno, 0, 1, WIFI_CHAR_DEV_DRV_DEV_NAME );
	}
	if (ret)
	{
	    BCM7XXX_ERR(( "couldn't alloc major devno\n" ));
	    kfree( gpDev );
	    return( ret );
	}

	/* populate sysfs */
	gpDev->dev_class = class_create( THIS_MODULE, WIFI_CHAR_DEV_DRV_DEV_NAME );
	if (gpDev->dev_class == NULL)
	{
	    BCM7XXX_ERR(( "failed to create class\n" ));
	    unregister_chrdev_region( gpDev->devno, 1 );
	    kfree( gpDev );
	    return( -1 );
	}

	gpDev->cdev = cdev_alloc();
	if (gpDev->cdev == NULL)
	{
	    BCM7XXX_ERR(( "failed to allocated cdev" ));
	    class_destroy( gpDev->dev_class );
	    unregister_chrdev_region( gpDev->devno, 1 );
	    kfree( gpDev );
	    return( -1 );
	}

	/* connect file operations to cdev */
	cdev_init( gpDev->cdev, &wifi_char_dev_drv_ops );
	gpDev->cdev->owner = THIS_MODULE;
	ret               = cdev_add( gpDev->cdev, gpDev->devno+minor, 1 );
	if (ret)
	{
	    BCM7XXX_ERR(( "Failed add cdev\n" ));
	    cdev_del( gpDev->cdev );
	    class_destroy( gpDev->dev_class );
	    unregister_chrdev_region( gpDev->devno, 1 );
	    kfree( gpDev );
	    return( ret );
	}
	
	/* send event to udev , to create /dev node */
	gpDev->dev_device = device_create( gpDev->dev_class, NULL, gpDev->devno+minor, 
							NULL, WIFI_CHAR_DEV_DRV_DEV_NAME "%d", minor );
	if (gpDev->dev_device==NULL)
	{
	    cdev_del( gpDev->cdev );
	    class_destroy( gpDev->dev_class );
	    unregister_chrdev_region( gpDev->devno, 1 );
	    kfree( gpDev );
	    return( -1 );
	}

	return( ret );
}

void wifi_char_dev_drv_deinit(void)
{

	BCM7XXX_TRACE();

	if (gpDev->cdev)
	{
	    cdev_del( gpDev->cdev );
	    gpDev->cdev = NULL;
	}
	if (gpDev->dev_class)
	{
	    device_destroy( gpDev->dev_class, gpDev->devno );
	    class_destroy( gpDev->dev_class );
	    gpDev->dev_class = NULL;
	}
	unregister_chrdev_region( gpDev->devno, 1 );
	kfree( gpDev );
}
#endif /* USE_CHAR_DRIVER */

/*
 * wl_bcm7xxx_init:
 * initialize the device.  called from module init
 */
int wl_bcm7xxx_init(void)
{
	int 		error = 0;
	void		*dev_device = NULL;
	wl_info_t	*pwl;
	uint16 	device;
	char 	*device_buf = device_var;


#if defined(BCM7271_EMU)
	emu_free_all_remote_nfifo();
	emu_free_all_remote_pkt();
#endif /* defined(BCM7271_EMU) */

#ifdef USE_CHAR_DRIVER
	error = wifi_char_dev_drv_init();
	if (error != 0) {
		BCM7XXX_ERR(("%s:%d: failed to init the driver.\n", __FUNCTION__, __LINE__));
	}
	dev_device = gpDev->dev_device;
#endif /* USE_CHAR_DRIVER */

	error = wl_bcm7xxx_device_tree_lookup(BOLT_DEVICE_TREE_PATH_BOARDNAME, &device_buf);
	if (error != 0) {
		BCM7XXX_ERR(("%s:%d: Device tree read failed.\n", __FUNCTION__, __LINE__));
	}

	device = wl_bcm7xxx_get_device_id(device_buf);

	/*	devid = BCM7271_D11AC_ID, BCM7271_D11AC2G_ID, BCM7271_D11AC5G_ID, */
	pwl = wl_attach(VENDOR_BROADCOM, device, SI_ENUM_BASE /* regs */,
		SI_BUS /* bus type */, dev_device /* btparam or dev */, BCM7271_L1_IRQ_NUM,
		NULL /* BAR1_ADDR */, 0 /* BAR1_SIZE */, NULL /* private data */);

	if (pwl == NULL)
		error = -ENODEV;


	return (error);
}

/*
 * wl_bcm7xxx_deinit:
 * shutdown the device.  called from module exit
 */
void wl_bcm7xxx_deinit(void)
{
#ifdef USE_CHAR_DRIVER
	wifi_char_dev_drv_deinit();
#endif /* USE_CHAR_DRIVER */

	if (g_bcm7xxx->pwl) {
		wl_free(g_bcm7xxx->pwl);
	}
}


static int __init
wl_module_init(void)
{
	int error = -ENODEV;

	error = wl_bcm7xxx_init();

	return (error);
}

/**
 * This function unloads the WL driver from the system.
 *
 * This function unconditionally unloads the WL driver module from the
 * system.
 *
 */
static void __exit
wl_module_exit(void)
{
	wl_bcm7xxx_deinit();
}


module_init(wl_module_init);
module_exit(wl_module_exit);
/* Needs to be GPL to build build 4.2 linaro kernel for Dragon Board */
MODULE_LICENSE("GPL"); 
