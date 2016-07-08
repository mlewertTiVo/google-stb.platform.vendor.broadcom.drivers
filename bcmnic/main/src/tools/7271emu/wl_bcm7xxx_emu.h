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

#ifndef WL_BCM7XXX_EMU_H
#define WL_BCM7XXX_EMU_H

#include <typedefs.h>

#ifndef NFIFO_EXT
#define NFIFO_EXT		32
#endif /* NFIFO_EXT */

#ifndef NFIFO
#define NFIFO		6
#endif /* NFIFO */

/* this value is based on MAX_DESC found in wlc_alloc.c */
/* BCM7271_EMU_NFIFO_SIZE = BCM7XXX_MAX_DESC * 16 */
#define BCM7271_EMU_NFIFO_SIZE 0x2000 /* 512 descriptors * 16 bytes, must be power of 2 */
//#define BCM7271_EMU_NFIFO_SIZE 0x2000 /* 8K = 512 descriptors * 16 bytes, must be power of 2 */

#define BCM7271_REMOTE_NFIFO_BASE_ADDR	0x14000000 /* @320MB offset */
#if defined(WL_MU_TX) && !defined(WL_MU_TX_DISABLED)
#define BCM7271_EMU_NFIFO_MAX_ENTRY	NFIFO_EXT
#else
#define BCM7271_EMU_NFIFO_MAX_ENTRY	NFIFO
#endif /* #if defined(WL_MU_TX) && !defined(WL_MU_TX_DISABLED) */
/* 0 to (BCM7271_EMU_NFIFO_MAX_ENTRY-1) is for DMA TX
 * from BCM7271_EMU_NFIFO_MAX_ENTRY onward is DMA RX
 */
#define BCM7271_EMU_RX_Q_INDEX_OFFSET BCM7271_EMU_NFIFO_MAX_ENTRY
#define BCM7271_EMU_MAX_Q_INDEX (BCM7271_EMU_NFIFO_MAX_ENTRY * 2)

void emu_free_remote_nfifo(uint32 emu_q_index);
void emu_free_all_remote_nfifo(void);
void emu_free_all_remote_pkt(void);

#ifdef BCM7271_EMU_VELOCE_64BIT
uint64 emu64_get_remote_nfifo_addr(uint64 local);
uint64 emu64_alloc_remote_nfifo(uint64 local, uint32 size, uint32 emu_q_index);
uint64 emu64_get_remote_pkt_addr(uint64 *data);
uint64 emu64_read_remote_data(uint64* data, uint32 size);
uint64 emu64_write_remote_data(uint64* data, uint32 size);
uint64 emu64_verify_remote_data(uint64* data, uint32 size);
uint64 emu64_alloc_remote_pkt(uint64* data, uint32 size);
void emu64_free_remote_pkt(uint64* data);
extern uint64 emu64_veloce_read_dma_sm(void* handle, uint64* addr);
extern void emu64_veloce_write_dma_sm(void* handle, uint64* addr, uint32 val);
extern void emu64_veloce_bzero(void* handle, uint64* local, size_t len);
extern void emu64_veloce_read_dma_buf(void* handle, uint64* local, uint64* remote, uint32 len);
extern void emu64_veloce_write_dma_buf(void* handle, uint64* local, uint64* remote, uint32 len);
#else
uint32 emu_get_remote_nfifo_addr(uint64 local);
uint32 emu_alloc_remote_nfifo(uint32 local, uint32 size, uint32 emu_q_index);
uint32 emu_get_remote_pkt_addr(uint32 *data);
uint32 emu_read_remote_data(uint32*data, uint32 size);
uint32 emu_write_remote_data(uint32*data, uint32 size);
uint32 emu_verify_remote_data(uint32*data, uint32 size);
uint32 emu_alloc_remote_pkt(uint32 *data, uint32 size);
void emu_free_remote_pkt(uint32 *data);
extern uint32 emu_veloce_read_dma_sm(void* handle, uint32* addr);
extern void emu_veloce_write_dma_sm(void* handle, uint32* addr, uint32 val);
extern void emu_veloce_bzero(void* handle, uint32* local, size_t len);
extern void emu_veloce_read_dma_buf(void* handle, uint32* local, uint32* remote, uint32 len);
extern void emu_veloce_write_dma_buf(void* handle, uint32* local, uint32* remote, uint32 len);
#endif /* BCM7271_EMU_VELOCE_64BIT */

#ifdef BCM7271_EMU_VELOCE_64BIT
#define	BCM7XXX_EMU_FREE_REMOTE_PKT(pkt) \
		emu64_free_remote_pkt((uint64*)(pkt))

#define	BCM7XXX_EMU_READ_REMOTE_DATA(pkt, len) \
		emu64_read_remote_data((uint64*)(pkt), (len))

#define	BCM7XXX_EMU_WRITE_REMOTE_DATA(pkt, len) \
		emu64_write_remote_data((uint64*)(pkt), (len))

#define BCM7XXX_EMU_ALLOC_REMOTE_PKT(pdata, len) \
		emu64_alloc_remote_pkt((uint64*)(pdata), (len));

#define	BCM7XXX_EMU_ALLOC_REMOTE_NFIFO(va, size, index) \
		emu64_alloc_remote_nfifo((uintptr)(va), (size), (index))

#else /* !BCM7271_EMU_VELOCE_64BIT */

#define	BCM7XXX_EMU_FREE_REMOTE_PKT(pkt) \
		emu_free_remote_pkt((uint32*)(pkt))

#define	BCM7XXX_EMU_READ_REMOTE_DATA(pkt, len) \
		emu_read_remote_data((uint32*)(pkt), (len))

#define	BCM7XXX_EMU_WRITE_REMOTE_DATA(pkt, len) \
		emu_write_remote_data((uint32*)(pkt), (len))

#define BCM7XXX_EMU_ALLOC_REMOTE_PKT(pdata, len) \
		emu_alloc_remote_pkt((uint32*)(pdata), (len))

#define	BCM7XXX_EMU_ALLOC_REMOTE_NFIFO(va, size, index) \
		emu_alloc_remote_nfifo((uint32)(va), (size), (index))

#endif /* BCM7271_EMU_VELOCE_64BIT */

#endif /* WL_BCM7XXX_EMU_H */
