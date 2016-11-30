/*
 * Chip/core-specific address space definitions, masks and other macros for d11 core
 * Broadcom 802.11abg Networking Device Driver
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: d11_addr_space.h 618930 2016-02-12 14:55:06Z $
 */

#ifndef	_D11_ADDR_SPACE_H
#define	_D11_ADDR_SPACE_H

#if D11CONF_GE(60)
/* for UCODE_IN_ROM_SUPPORT, 2 axi slave ports are expected */
#define D11_AXI_SP_IDX		(1) /* Index 0 and 1 (First and Second slave port) */

#define UCM_INSTR_WIDTH_BYTES	(8)
#define UCM_RAM_SIZE		(8 * 1024)
#define UCM_END			((UCM_RAM_SIZE * UCM_INSTR_WIDTH_BYTES) - 1)

#define PA_START_OFFSET		(0)
#define PIA_START_OFFSET	(UCM_RAM_SIZE * UCM_INSTR_WIDTH_BYTES)

/* pa address mask in pia[0]. Note that addr width is 13 bits in pia */
#define UCM_PA_ADDR_MASK	(0x1fff)

/* pia or pa minimum sizes */
#define UCM_PATCH_MIN_BYTES	(4)

/* at the end of pa or pia, there is 8 byte delimiter signifying end of patch
 * code. This is 8 byte zeroes. This has to be excluded from the patch
 * download.
 */
#define UCM_PATCH_TERM_SZ	(8)

/* pa will get written 4 bytes at a time as direct AXI register write for non-m2mdma case */
#define UCM_PATCH_WRITE_SZ	(4)

#define APB_UCM_BASE		(0x0)

#define AXI_ADDR_MASK		(0x7FFFFF)
#define APB_ADDR_MASK		(0xFFF)
#define COMMON_ADDR_MASK	(AXI_ADDR_MASK | APB_ADDR_MASK)

/* offsets of different memories based axi slave base, for core d11rev>60 /43012 */
#define AXISL_IHR_BASE		(0x00000000)
#define AXISL_IHR_SIZE		(0x0FFF)
#define AXISL_IHR_END		(AXISL_IHR_BASE + AXISL_IHR_SIZE)
#define AXISL_FCBS_BASE		(0x00002000)
#define AXISL_FCBS_SIZE		(0x001F)
#define AXISL_FCBS_END		(AXISL_FCBS_BASE + AXISL_FCBS_SIZE)
#define AXISL_SHM_BASE		(0x00004000)
#define AXISL_SHM_SIZE		(0x1FFF)
#define AXISL_SHM_END		(AXISL_SHM_BASE + AXISL_SHM_SIZE)
#define AXISL_SCR_BASE		(0x00009000)
#define AXISL_SCR_SIZE		(0x00FF)
#define AXISL_SCR_END		(AXISL_SCR_BASE + AXISL_SCR_SIZE)
#define AXISL_AMT_BASE		(0x0000B000)
#define AXISL_AMT_SIZE		(0x01FF)
#define AXISL_AMT_END		(AXISL_AMT_BASE + AXISL_AMT_SIZE)
#define AXISL_UCM_BASE		(0x00020000)
#define AXISL_UCM_SIZE		(0x07FF)
#define AXISL_UCM_END		(AXISL_UCM_BASE + AXISL_UCM_SIZE)
#define AXISL_BM_BASE		(0x00400000)
#define AXISL_BM_SIZE		(0x17FFF)
#define AXISL_BM_END		(AXISL_BM_BASE + AXISL_BM_SIZE)

#define IS_ADDR_AXISL_SUBSPACE(axi_base, addr, subspace)	\
	((addr) >= ((axi_base) + AXISL_ ## subspace ## _BASE) &&	\
	(addr) <= ((axi_base) + AXISL_ ## subspace ## _END))

#define IS_ADDR_AXISL(axi_base, addr)				\
	(IS_ADDR_AXISL_SUBSPACE((axi_base), (addr), IHR) ||	\
	IS_ADDR_AXISL_SUBSPACE((axi_base), (addr), FCBS) ||	\
	IS_ADDR_AXISL_SUBSPACE((axi_base), (addr), SHM)	||	\
	IS_ADDR_AXISL_SUBSPACE((axi_base), (addr), SCR)	||	\
	IS_ADDR_AXISL_SUBSPACE((axi_base), (addr), AMT)	||	\
	IS_ADDR_AXISL_SUBSPACE((axi_base), (addr), UCM)	||	\
	IS_ADDR_AXISL_SUBSPACE((axi_base), (addr), BM))

#endif /* D11CONF_GT(60) */

#endif	/* _D11_ADDR_SPACE_H */
