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
#include <wl_bcm7xxx_emu.h>

#ifndef BCM7XXX_DBG_ENABLED
#define BCM7XXX_DBG_ENABLED
#endif /* BCM7XXX_DBG_ENABLED */
#include <wl_bcm7xxx_dbg.h>


#if defined(BCM7271_EMU)
#define BCM7271_REMOTE_MEM_BASE_ADDR	0x12000000 /* @288MB offset */
#define BCM7271_REMOTE_MEM_SLOT_SIZE	0x1000 /* 4K slot size */
#define BCM7271_REMOTE_MEM_MAX_SLOTS	0x4000 /* 16K packets */

#define BCM7271_REMOTE_MEM_SIZE\
		(BCM7271_REMOTE_MEM_SLOT_SIZE * BCM7271_REMOTE_MEM_MAX_SLOTS)
		
typedef struct emu_addr_table_s
{
#ifdef BCM7271_EMU_VELOCE_64BIT
	uint64	local_addr;
	uint64	remote_addr;
	uint64	local_mask;
#else
	uint32	local_addr;
	uint32	remote_addr;
	uint32	local_mask;
#endif /* BCM7271_EMU_VELOCE_64BIT */
	uint32	size;
} emu_addr_table_t;


emu_addr_table_t g_emu_addr_table[BCM7271_REMOTE_MEM_MAX_SLOTS];
emu_addr_table_t g_emu_remote_nfifo_table[BCM7271_EMU_MAX_Q_INDEX];


void emu_free_remote_nfifo(uint32 emu_q_index)
{
	uint32	ii = emu_q_index;

	BCM7XXX_ERR(("%s(%d) emu_q_index=%d\n", __FUNCTION__, __LINE__, ii));

	if (ii < BCM7271_EMU_MAX_Q_INDEX) {
		g_emu_remote_nfifo_table[ ii ].local_addr  = 0;
		g_emu_remote_nfifo_table[ ii ].remote_addr = 0;
		g_emu_remote_nfifo_table[ ii ].local_mask  = ~(BCM7271_EMU_NFIFO_SIZE-1);
		g_emu_remote_nfifo_table[ ii ].size        = BCM7271_EMU_NFIFO_SIZE;
	} else {
		BCM7XXX_ERR(("%s(%d)ERROR!!! emu_q_index=%d is OUT OF RANGE!!!\n",
			__FUNCTION__, __LINE__, ii));
	}
}

void emu_free_all_remote_nfifo(void)
{
	uint32	ii;

	BCM7XXX_ERR(("%s(%d)\n", __FUNCTION__, __LINE__));
	for (ii = 0; ii < BCM7271_EMU_MAX_Q_INDEX; ii++) {
		g_emu_remote_nfifo_table[ ii ].local_addr  = 0;
		g_emu_remote_nfifo_table[ ii ].remote_addr = 0;
		g_emu_remote_nfifo_table[ ii ].local_mask  = ~(BCM7271_EMU_NFIFO_SIZE-1);
		g_emu_remote_nfifo_table[ ii ].size        = BCM7271_EMU_NFIFO_SIZE;
	}
}


void emu_free_all_remote_pkt(void)
{
	uint32	ii;

	BCM7XXX_ERR(("%s(%d)\n", __FUNCTION__, __LINE__));
	for (ii = 0; ii < BCM7271_REMOTE_MEM_MAX_SLOTS; ii++) {
		g_emu_addr_table[ii].local_addr  = 0;
		g_emu_addr_table[ii].remote_addr = 0;
		g_emu_addr_table[ii].local_mask  = 0;
		g_emu_addr_table[ii].size        = 0;
	}
}





#ifdef BCM7271_EMU_VELOCE_64BIT
uint64 emu64_get_remote_nfifo_addr(uint64 local)
{
	uint32	ii;
	uint64	mask;
	uint64	local_addr = local;
	uint64  remote_addr;
	uint32	offset = local_addr & (BCM7271_EMU_NFIFO_SIZE - 1);

	for (ii = 0; ii < BCM7271_EMU_MAX_Q_INDEX; ii++) {
		mask = g_emu_remote_nfifo_table[ ii ].local_mask;
		if (g_emu_remote_nfifo_table[ ii ].local_addr == (local_addr & mask)) {
			remote_addr = g_emu_remote_nfifo_table[ ii ].remote_addr;
			return (remote_addr | offset);
		}
	}

	BCM7XXX_ERR(("%s(%d) ERROR!!! CAN'T FIND LOCAL==0x%08llx\n",
		__FUNCTION__, __LINE__, local_addr));

	return 0;
}

uint64 emu64_alloc_remote_nfifo(uint64 local, uint32 size, uint32 emu_q_index)
	/* allocate a ring buffer */
{
	uint32	ii = emu_q_index;
	uint64	local_addr = local;
	uint64	remote_addr = BCM7271_REMOTE_NFIFO_BASE_ADDR;


	if (size > BCM7271_EMU_NFIFO_SIZE) {
		BCM7XXX_ERR(("%s(%d) ERRORR!!! emu_q_index=%d is already allocated: size==0x%08x\n",
			__FUNCTION__, __LINE__, ii, size));
		remote_addr = 0;
		return (remote_addr);
	}

	if (ii < BCM7271_EMU_MAX_Q_INDEX) {
		if (g_emu_remote_nfifo_table[ ii ].local_addr == 0) {

			/* found an empty slot, so mark that slot used */
			/* by assigning a valid address */
			g_emu_remote_nfifo_table[ ii ].local_addr = local_addr;

			/* alloc the remote memory, assuming that the size is fixed */
			remote_addr += ii * BCM7271_EMU_NFIFO_SIZE;

			/* store it, so that there is a mapping between local and remote */
			g_emu_remote_nfifo_table[ ii ].remote_addr = remote_addr;
			g_emu_remote_nfifo_table[ ii ].local_mask  = ~(BCM7271_EMU_NFIFO_SIZE-1);
			g_emu_remote_nfifo_table[ ii ].size        = BCM7271_EMU_NFIFO_SIZE;
		} else {
			BCM7XXX_ERR(("%s(%d) ERROR!!! emu_q_index=%d is allocated:local=0x%08llx\n",
				__FUNCTION__, __LINE__, ii, local_addr));
			remote_addr = 0;
		}
	} else {
		BCM7XXX_ERR(("%s(%d) ERROR!!! emu_q_index=%d is OUT OF RANGE!!!\n",
			__FUNCTION__, __LINE__, ii));
		remote_addr = 0;
	}

	BCM7XXX_ERR(("%s(%d) alloc emu_q_index=%d local==0x%08llx, remote==0x%08llx\n",
		__FUNCTION__, __LINE__, ii, local_addr, remote_addr));
	return (remote_addr);
}


uint64 emu64_get_remote_pkt_addr(uint64 *data)
{
	uint32	ii;
	uint64	mask;
	uint64	local_addr = (uint64)data;
	uint64	remote_addr = 0;

	for (ii = 0; ii < BCM7271_REMOTE_MEM_MAX_SLOTS; ii++) {
		mask = g_emu_addr_table[ ii ].local_mask;
		if (g_emu_addr_table[ii].local_addr == local_addr) {
			/* found it */
			remote_addr = g_emu_addr_table[ ii ].remote_addr;
			break;
		}
	}
	if (ii == BCM7271_REMOTE_MEM_MAX_SLOTS) {
		BCM7XXX_ERR(("%s(%d) can't find local==0x%08llx\n",
			__FUNCTION__, __LINE__, local_addr));
	}

	return (remote_addr);
}


uint64 emu64_read_remote_data(uint64*data, uint32 size)
{
	uint64	remote_addr = 0;

	remote_addr = emu64_get_remote_pkt_addr(data);

	/* copy the content from the remote memory */
	if (remote_addr) {
		emu64_veloce_read_dma_buf(NULL, data, (uint64 *)remote_addr, size);
	} else {
		BCM7XXX_ERR(("%s(%d) wrong local==0x%08llx\n",
			__FUNCTION__, __LINE__, (uint64)data));
	}

	return (remote_addr);
}

uint64 emu64_write_remote_data(uint64 *data, uint32 size)
{
	uint64	remote_addr = 0;

	remote_addr = emu64_get_remote_pkt_addr(data);

	/* copy the content to the remote memory */
	if (remote_addr) {
		emu64_veloce_write_dma_buf(NULL, data, (uint64 *)remote_addr, size);
	} else {
		BCM7XXX_ERR(("%s(%d) wrong local==0x%08llx\n",
			__FUNCTION__, __LINE__, (uint64)data));
	}

	return (remote_addr);
}


#define REMOTE_BUF_SIZE 2048
char	remote_buf[REMOTE_BUF_SIZE];
uint64 emu64_verify_remote_data(uint64*data, uint32 size)
{
	uint32	ii;
	uint64	remote_addr = 0;
	char	*cdata = (char*)data;

	remote_addr = emu64_get_remote_pkt_addr(data);

	if (size >= REMOTE_BUF_SIZE) {
		size = REMOTE_BUF_SIZE;
	}

	/* copy the content from the remote memory */
	if (remote_addr) {
		emu64_veloce_read_dma_buf(NULL, (uint64 *)remote_buf, (uint64 *)remote_addr, size);
		for (ii = 0; ii < size; ii++) {
			if (remote_buf[ii] != cdata[ii]) {
				BCM7XXX_ERR(("%s(%d)data[%d!=%d]\n",
					__FUNCTION__, __LINE__, remote_buf[ii], cdata[ii]));
			}
		}
	} else {
		BCM7XXX_ERR(("%s(%d) wrong local==0x%08llx\n",
			__FUNCTION__, __LINE__, (uint64)data));
	}

	return (remote_addr);
}

// #define MEM_ALLOC_DEBUG
#ifdef MEM_ALLOC_DEBUG
int memCounter = 0;
#endif /* MEM_ALLOC_DEBUG */

uint64 emu64_alloc_remote_pkt(uint64 *data, uint32 size)
{
	uint32	ii;
	uint64	local_addr = (uint64)data;
	uint64	remote_addr = BCM7271_REMOTE_MEM_BASE_ADDR;


	if (size > BCM7271_REMOTE_MEM_SLOT_SIZE) {
		BCM7XXX_ERR(("%s(%d) data size is TOO BIG=%d\n",
			__FUNCTION__, __LINE__, size));
		return 0;
	}

	/* allocate the remote memory */
	for (ii = 0; ii < BCM7271_REMOTE_MEM_MAX_SLOTS; ii++) {
		/* look for an empty slot */
		if (g_emu_addr_table[ii].local_addr == 0) {

#ifdef MEM_ALLOC_DEBUG
			memCounter++;
#endif /* MEM_ALLOC_DEBUG */

			/* found an empty slot, so mark that slot used */
			/* by assigning a valid address */
			g_emu_addr_table[ ii ].local_addr = local_addr;

			/* alloc the remote memory, assuming that the size is fixed */
			remote_addr += ii * BCM7271_REMOTE_MEM_SLOT_SIZE;

			/* store it, so that there is a mapping between local and remote */
			g_emu_addr_table[ ii ].remote_addr = remote_addr;
			g_emu_addr_table[ ii ].local_mask  = ~(BCM7271_REMOTE_MEM_SLOT_SIZE-1);
			g_emu_addr_table[ ii ].size        = size; /* actual size */

			break;
		}
	}

#ifdef MEM_ALLOC_DEBUG
	BCM7XXX_DBG(("%s(%d) Memory Allocation & Deallocation counter = %d\n",
		__FUNCTION__, __LINE__, memCounter));
#endif /* MEM_ALLOC_DEBUG */

	if (ii == BCM7271_REMOTE_MEM_MAX_SLOTS) {
		BCM7XXX_ERR(("%s(%d) OUT OF MEMORY\n", __FUNCTION__, __LINE__));
		return 0;
	}

	 return (remote_addr);
}


void emu64_free_remote_pkt(uint64 *data)
{
	uint32	ii;
	uint64	mask;
	uint64	local_addr = (uint64)data;

	for (ii = 0; ii < BCM7271_REMOTE_MEM_MAX_SLOTS; ii++) {
		mask = g_emu_addr_table[ ii ].local_mask;
		if (g_emu_addr_table[ii].local_addr == local_addr) {
			/* found it */
			/* clear it (a.k.a free it) */
#ifdef MEM_ALLOC_DEBUG
			memCounter--;
#endif /* MEM_ALLOC_DEBUG */

			g_emu_addr_table[ii].local_addr  = 0;
			g_emu_addr_table[ii].remote_addr = 0;
			break;
		}
	}

#ifdef MEM_ALLOC_DEBUG
	BCM7XXX_DBG(("%s(%d) Memory Allocation & Deallocation counter = %d\n",
		__FUNCTION__, __LINE__, memCounter));
#endif /* MEM_ALLOC_DEBUG */

	if (ii == BCM7271_REMOTE_MEM_MAX_SLOTS) {
		BCM7XXX_ERR(("%s(%d) UNKNOWN address==0x%08llx\n",
			__FUNCTION__, __LINE__, local_addr));
	}
}

#else

uint32 emu_get_remote_nfifo_addr(uint64 local)
{
	uint32	ii;
	uint32	mask;
	uint32	local_addr = (uint32)local;
	uint32  remote_addr;
	uint32	offset = local_addr & (BCM7271_EMU_NFIFO_SIZE - 1);

	for (ii = 0; ii < BCM7271_EMU_MAX_Q_INDEX; ii++) {
		mask = g_emu_remote_nfifo_table[ ii ].local_mask;
		if (g_emu_remote_nfifo_table[ ii ].local_addr == (local_addr & mask)) {
			remote_addr = g_emu_remote_nfifo_table[ ii ].remote_addr;
			return (remote_addr | offset);
		}
	}

	BCM7XXX_ERR(("%s(%d) ERROR!!! CAN'T FIND LOCAL==0x%08x\n",
		__FUNCTION__, __LINE__, local_addr));

	return 0;
}

uint32 emu_alloc_remote_nfifo(uint32 local, uint32 size, uint32 emu_q_index)
	/* allocate a ring buffer */
{
	uint32	ii = emu_q_index;
	uint32	local_addr = local;
	uint32	remote_addr = BCM7271_REMOTE_NFIFO_BASE_ADDR;


	if (size > BCM7271_EMU_NFIFO_SIZE) {
		BCM7XXX_ERR(("%s(%d) ERRORR!!! emu_q_index=%d is already allocated: size==0x%08x\n",
			__FUNCTION__, __LINE__, ii, size));
		remote_addr = 0;
		return (remote_addr);
	}

	if (ii < BCM7271_EMU_MAX_Q_INDEX) {
		if (g_emu_remote_nfifo_table[ ii ].local_addr == 0) {

			/* found an empty slot, so mark that slot used */
			/* by assigning a valid address */
			g_emu_remote_nfifo_table[ ii ].local_addr = local_addr;

			/* alloc the remote memory, assuming that the size is fixed */
			remote_addr += ii * BCM7271_EMU_NFIFO_SIZE;

			/* store it, so that there is a mapping between local and remote */
			g_emu_remote_nfifo_table[ ii ].remote_addr = remote_addr;
			g_emu_remote_nfifo_table[ ii ].local_mask  = ~(BCM7271_EMU_NFIFO_SIZE-1);
			g_emu_remote_nfifo_table[ ii ].size        = BCM7271_EMU_NFIFO_SIZE;
		} else {
			BCM7XXX_ERR(("%s(%d) ERROR!!! emu_q_index=%d is allocated: local=0x%08x\n",
				__FUNCTION__, __LINE__, ii, local_addr));
			remote_addr = 0;
		}
	} else {
		BCM7XXX_ERR(("%s(%d) ERROR!!! emu_q_index=%d is OUT OF RANGE!!!\n",
			__FUNCTION__, __LINE__, ii));
		remote_addr = 0;
	}

	BCM7XXX_ERR(("%s(%d) alloc emu_q_index=%d local==0x%08x, remote==0x%08x\n",
		__FUNCTION__, __LINE__, ii, local_addr, remote_addr));
	return (remote_addr);
}


uint32 emu_get_remote_pkt_addr(uint32 *data)
{
	uint32	ii;
	int32	mask;
	uint32	local_addr = (uint32) data;
	uint32	remote_addr = 0;

	for (ii = 0; ii < BCM7271_REMOTE_MEM_MAX_SLOTS; ii++) {
		mask = g_emu_addr_table[ ii ].local_mask;
		if (g_emu_addr_table[ii].local_addr == local_addr) {
			/* found it */
			remote_addr = g_emu_addr_table[ ii ].remote_addr;
			break;
		}
	}
	if (ii == BCM7271_REMOTE_MEM_MAX_SLOTS) {
		BCM7XXX_ERR(("%s(%d) can't find local==0x%08x\n",
			__FUNCTION__, __LINE__, local_addr));
	}

	return (remote_addr);
}


uint32 emu_read_remote_data(uint32*data, uint32 size)
{
	uint32	remote_addr = 0;

	remote_addr = emu_get_remote_pkt_addr(data);

	/* copy the content from the remote memory */
	if (remote_addr) {
		emu_veloce_read_dma_buf(NULL, data, (uint32*)remote_addr, size);
	} else {
		BCM7XXX_ERR(("%s(%d) wrong local==0x%08x\n",
			__FUNCTION__, __LINE__, (uint32)data));
	}

	return (remote_addr);
}

uint32 emu_write_remote_data(uint32*data, uint32 size)
{
	uint32	remote_addr = 0;

	remote_addr = emu_get_remote_pkt_addr(data);

	/* copy the content to the remote memory */
	if (remote_addr) {
		emu_veloce_write_dma_buf(NULL, data, (uint32*)remote_addr, size);
	} else {
		BCM7XXX_ERR(("%s(%d) wrong local==0x%08x\n",
			__FUNCTION__, __LINE__, (uint32)data));
	}

	return (remote_addr);
}


#define REMOTE_BUF_SIZE 2048
char	remote_buf[REMOTE_BUF_SIZE];
uint32 emu_verify_remote_data(uint32*data, uint32 size)
{
	uint32	ii;
	uint32	remote_addr = 0;
	char	*cdata = (char*)data;

	remote_addr = emu_get_remote_pkt_addr(data);

	if (size >= REMOTE_BUF_SIZE) {
		size = REMOTE_BUF_SIZE;
	}

	/* copy the content from the remote memory */
	if (remote_addr) {
		emu_veloce_read_dma_buf(NULL, (uint32*)remote_buf, (uint32*)remote_addr, size);
		for (ii = 0; ii < size; ii++) {
			if (remote_buf[ii] != cdata[ii]) {
				BCM7XXX_ERR(("%s(%d)data[%d!=%d]\n",
					__FUNCTION__, __LINE__, remote_buf[ii], cdata[ii]));
			}
		}
	} else {
		BCM7XXX_ERR(("%s(%d) wrong local==0x%08x\n",
			__FUNCTION__, __LINE__, (uint32)data));
	}

	return (remote_addr);
}

// #define MEM_ALLOC_DEBUG
#ifdef MEM_ALLOC_DEBUG
int memCounter = 0;
#endif /* MEM_ALLOC_DEBUG */

uint32 emu_alloc_remote_pkt(uint32 *data, uint32 size)
{
	uint32	ii;
	uint32	local_addr = (uint32) data;
	uint32	remote_addr = BCM7271_REMOTE_MEM_BASE_ADDR;


	if (size > BCM7271_REMOTE_MEM_SLOT_SIZE) {
		BCM7XXX_ERR(("%s(%d) data size is TOO BIG=%d\n",
			__FUNCTION__, __LINE__, size));
		return 0;
	}

	/* allocate the remote memory */
	for (ii = 0; ii < BCM7271_REMOTE_MEM_MAX_SLOTS; ii++) {
		/* look for an empty slot */
		if (g_emu_addr_table[ii].local_addr == 0) {

#ifdef MEM_ALLOC_DEBUG
			memCounter++;
#endif /* MEM_ALLOC_DEBUG */

			/* found an empty slot, so mark that slot used */
			/* by assigning a valid address */
			g_emu_addr_table[ ii ].local_addr = local_addr;

			/* alloc the remote memory, assuming that the size is fixed */
			remote_addr += ii * BCM7271_REMOTE_MEM_SLOT_SIZE;

			/* store it, so that there is a mapping between local and remote */
			g_emu_addr_table[ ii ].remote_addr = remote_addr;
			g_emu_addr_table[ ii ].local_mask  = ~(BCM7271_REMOTE_MEM_SLOT_SIZE-1);
			g_emu_addr_table[ ii ].size        = size; /* actual size */

			break;
		}
	}

#ifdef MEM_ALLOC_DEBUG
	BCM7XXX_DBG(("%s(%d) Memory Allocation & Deallocation counter = %d\n",
		__FUNCTION__, __LINE__, memCounter));
#endif /* MEM_ALLOC_DEBUG */

	if (ii == BCM7271_REMOTE_MEM_MAX_SLOTS) {
		BCM7XXX_ERR(("%s(%d) OUT OF MEMORY\n", __FUNCTION__, __LINE__));
		return 0;
	}

	 return (remote_addr);
}


void emu_free_remote_pkt(uint32 *data)
{
	uint32	ii;
	uint32	mask;
	uint32	local_addr = (uint32)data;

	for (ii = 0; ii < BCM7271_REMOTE_MEM_MAX_SLOTS; ii++) {
		mask = g_emu_addr_table[ ii ].local_mask;
		if (g_emu_addr_table[ii].local_addr == local_addr) {
			/* found it */
			/* clear it (a.k.a free it) */
#ifdef MEM_ALLOC_DEBUG
			memCounter--;
#endif /* MEM_ALLOC_DEBUG */

			g_emu_addr_table[ii].local_addr  = 0;
			g_emu_addr_table[ii].remote_addr = 0;
			break;
		}
	}

#ifdef MEM_ALLOC_DEBUG
	BCM7XXX_DBG(("%s(%d) Memory Allocation & Deallocation counter = %d\n",
		__FUNCTION__, __LINE__, memCounter));
#endif /* MEM_ALLOC_DEBUG */

	if (ii == BCM7271_REMOTE_MEM_MAX_SLOTS) {
		BCM7XXX_ERR(("%s(%d) UNKNOWN address==0x%08x\n",
			__FUNCTION__, __LINE__, local_addr));
	}
}

#endif /* BCM7271_EMU_VELOCE_64BIT */
#endif /* defined(BCM7271_EMU) */
