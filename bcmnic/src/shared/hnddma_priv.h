/*
 * Generic Broadcom Home Networking Division (HND) DMA module.
 * This supports the following chips: BCM42xx, 44xx, 47xx .
 *
 * Copyright (C) 2016, Broadcom. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: hnddma_priv.h 648804 2016-07-13 19:39:52Z $
 */

#ifndef _HNDDMA_PRIV_H_
#define _HNDDMA_PRIV_H_

#ifdef EVENT_LOG_COMPILE
#include <event_log.h>
#endif

#ifdef WLCXO_DATA
/* Redefine PKTDATA as PKTHOSTDATA as we deal with host memory */
#undef PKTDATA
#define PKTDATA	PKTHOSTDATA
#endif /* WLCXO_DATA */

/* debug/trace */
#define	DMA_ERROR(args)
#define	DMA_TRACE(args)

#define	DMA_NONE(args)

#define d32txregs	dregs.d32_u.txregs_32
#define d32rxregs	dregs.d32_u.rxregs_32
#define txd32		dregs.d32_u.txd_32
#define rxd32		dregs.d32_u.rxd_32

#define d64txregs	dregs.d64_u.txregs_64
#define d64rxregs	dregs.d64_u.rxregs_64
#define txd64		dregs.d64_u.txd_64
#define rxd64		dregs.d64_u.rxd_64

#ifdef BCM_DMAPAD
	#if defined(WL_ENAB_RUNTIME_CHECK)
		#define DMAPADREQUIRED(di)		((di)->dmapad_required)
	#elif defined(BCMSDIODEV_ENABLED)
		#define DMAPADREQUIRED(di)		TRUE
	#else
		#define DMAPADREQUIRED(di)		FALSE
	#endif /* WL_ENAB_RUNTIME_CHECK */
#else
	#define DMAPADREQUIRED(di)		FALSE
#endif /* BCM_DMAPAD */

#ifdef BCM_DMA_TRANSCOHERENT
	#ifdef BCMROMBUILD
		#define DMA_TRANSCOHERENT(di)		((di)->trans_coherent)
	#else
		#define DMA_TRANSCOHERENT(di)		1
	#endif 
#else
	#define DMA_TRANSCOHERENT(di)			0
#endif /* BCM_DMA_TRANSCOHERENT */

/* For SDIO, dma_attach() (and some of the functions that it calls) are only invoked in attach
 * context; therefore, they can be reclaimed as a memory optimization. For USB and PCIE,
 * dma_attach() is called in non-attach context and therefore needs to be a persistent function.
 */
#if !defined(BCM_DMA_CT) && defined(BCMSDIODEV_ENABLED)
	#define HNDDMA_RECLAIM_DMA_ATTACH
#endif

#if defined(HNDDMA_RECLAIM_DMA_ATTACH)
	#define BCMATTACHFN_DMA_ATTACH(_fn)	BCMATTACHFN(_fn)
#else
	#define BCMATTACHFN_DMA_ATTACH(_fn)	(_fn)
#endif

#define	MAXNAMEL	9		/* 9 char names */

#define	DI_INFO(dmah)	((dma_info_t *)(uintptr)dmah)

#define DESCR_DEADBEEF
#if defined(__mips__) || defined(BCM47XX_CA9) || defined(STB)
#undef DESCR_DEADBEEF
#endif

/* SplitRX Feature for D11 <TCM,DDR> split pkt reception in Full Dongle Mode */

#if (!defined(__mips__) && !(defined(BCM47XX_CA9) || defined(STB))) || \
	defined(BCM_SECURE_DMA)
/* Enable/Disable support for Scatter Gather List in RX direction */
#define SGLIST_RX_SUPPORT
#endif

#if (defined(BCM47XX_CA9) || defined(STB))
/* Enable/Disable Bulk Descriptor Flushing optimization */
#define BULK_DESCR_FLUSH
#endif

#if defined(BULK_DESCR_FLUSH)
/* Flush length, given number of descriptors */
#define DMA64_SHIFT (4)  /* multiply by sizeof a dma descriptor  (16 Bytes) */
#define DMA64_FLUSH_LEN(num_desc)   ((num_desc) << DMA64_SHIFT)

#define dma64_rxd64(di, ix) (void *)((uintptr)(&((dma64dd_t *)((di)->rxd64))[ix]))
#define dma64_txd64(di, ix) (void *)((uintptr)(&((dma64dd_t *)((di)->txd64))[ix]))
#endif /* BULK_DESCR_FLUSH */

#define D11RX_WAR_MAX_BUF_SIZE 492 /* Refer JIRA:CRWLDOT11M-1776 */
#define MAXRXBUFSZ 2048

/* Structure for global DMA states and for storing information needed by all
 * DMA channels.
 *
 */
struct dma_common {
	void    *osh;           /* os handle */

	uint8 last_qsel;        /* last queue index that was configured in IndQSel */

	/* INDIRECT DMA channel registers for rev>=64 */
	volatile uint32 *indqsel;
	volatile uint32 *suspreq;
	volatile uint32 *flushreq;
	volatile uint32 *chnflushstatus;
};

/** dma engine software state */
typedef struct dma_info {
	struct hnddma_pub hnddma;	/* exported structure, don't use hnddma_t,
					 * which could be const
					 */
	uint		*msg_level;	/* message level pointer */
	char		name[MAXNAMEL];	/* callers name for diag msgs */

	void		*osh;		/* os handle */
	si_t		*sih;		/* sb handle */

	bool		dma64;		/* this dma engine is operating in 64-bit mode */
	bool		addrext;	/* this dma engine supports DmaExtendedAddrChanges */

	bool		indirect_dma;	/* set if Indirect DMA interface is supported */
	uint8		q_index;		/* index of the queue */
	union {
		struct {
			dma32regs_t	*txregs_32;	/* 32-bit dma tx engine registers */
			dma32regs_t	*rxregs_32;	/* 32-bit dma rx engine registers */
			dma32dd_t	*txd_32;	/* pointer to dma32 tx descriptor ring */
			dma32dd_t	*rxd_32;	/* pointer to dma32 rx descriptor ring */
		} d32_u;
		struct {
			dma64regs_t	*txregs_64;	/* 64-bit dma tx engine registers */
			dma64regs_t	*rxregs_64;	/* 64-bit dma rx engine registers */
			dma64dd_t	*txd_64;	/* pointer to dma64 tx descriptor ring */
			dma64dd_t	*rxd_64;	/* pointer to dma64 rx descriptor ring */
		} d64_u;
	} dregs;

	uint16		dmadesc_align;	/* alignment requirement for dma descriptors */

	uint16		ntxd;		/* # tx descriptors tunable */
	uint16		txin;		/* index of next descriptor to reclaim */
	uint16		txout;		/* index of next descriptor to post */
	void		**txp;		/* pointer to parallel array of pointers to packets */
	osldma_t 	*tx_dmah;	/* DMA TX descriptor ring handle */
	hnddma_seg_map_t	*txp_dmah;	/* DMA MAP meta-data handle */
	dmaaddr_t	txdpa;		/* Aligned physical address of descriptor ring */
	dmaaddr_t	txdpaorig;	/* Original physical address of descriptor ring */
	uint16		txdalign;	/* #bytes added to alloc'd mem to align txd */
	uint32		txdalloc;	/* #bytes allocated for the ring */
	uint32		xmtptrbase;	/* When using unaligned descriptors, the ptr register
					 * is not just an index, it needs all 13 bits to be
					 * an offset from the addr register.
					 */

	uint16		nrxd;		/* # rx descriptors tunable */
	uint16		rxin;		/* index of next descriptor to reclaim */
	uint16		rxout;		/* index of next descriptor to post */
	void		**rxp;		/* pointer to parallel array of pointers to packets */
	osldma_t 	*rx_dmah;	/* DMA RX descriptor ring handle */
	hnddma_seg_map_t	*rxp_dmah;	/* DMA MAP meta-data handle */
	dmaaddr_t	rxdpa;		/* Aligned physical address of descriptor ring */
	dmaaddr_t	rxdpaorig;	/* Original physical address of descriptor ring */
	uint16		rxdalign;	/* #bytes added to alloc'd mem to align rxd */
	uint32		rxdalloc;	/* #bytes allocated for the ring */
	uint32		rcvptrbase;	/* Base for ptr reg when using unaligned descriptors */

	/* tunables */
	uint16		rxbufsize;	/* rx buffer size in bytes,
					 * not including the extra headroom
					 */
	uint		rxextrahdrroom;	/* extra rx headroom, reverseved to assist upper stack
					 *  e.g. some rx pkt buffers will be bridged to tx side
					 *  without byte copying. The extra headroom needs to be
					 *  large enough to fit txheader needs.
					 *  Some dongle driver may not need it.
					 */
	uint		nrxpost;	/* # rx buffers to keep posted */
	uint		rxoffset;	/* rxcontrol offset */
	uint		ddoffsetlow;	/* add to get dma address of descriptor ring, low 32 bits */
	uint		ddoffsethigh;	/*   high 32 bits */
	uint		dataoffsetlow;	/* add to get dma address of data buffer, low 32 bits */
	uint		dataoffsethigh;	/*   high 32 bits */
	bool		aligndesc_4k;	/* descriptor base need to be aligned or not */
	uint8		rxburstlen;	/* burstlen field for rx (for cores supporting burstlen) */
	uint8		txburstlen;	/* burstlen field for tx (for cores supporting burstlen) */
	uint8		txmultioutstdrd; 	/* tx multiple outstanding reads */
	uint8 		txprefetchctl;	/* prefetch control for tx */
	uint8 		txprefetchthresh;	/* prefetch threshold for tx */
	uint8 		rxprefetchctl;	/* prefetch control for rx */
	uint8 		rxprefetchthresh;	/* prefetch threshold for rx */
	uint8		txchanswitch;	/* channel switch for rx */
	pktpool_t	*pktpool;	/* pktpool */
	uint		dma_avoidance_cnt;

	uint32 		d64_xs0_cd_mask; /* tx current descriptor pointer mask */
	uint32 		d64_xs1_ad_mask; /* tx active descriptor mask */
	uint32 		d64_rs0_cd_mask; /* rx current descriptor pointer mask */
	uint16		rs0cd;		/* cached value of rcvstatus0 currdescr */
	uint16		xs0cd;		/* cached value of xmtstatus0 currdescr */
	uint16		xs0cd_snapshot;	/* snapshot of xmtstatus0 currdescr */
	uint8		burstsize_ctrl;
	uint		rxavail;	/* rxavail descriptor */
	uint8		sep_rxhdr;	/* D11_SPLIT_RX_FD: Separate rx header descriptor */
	uint8		split_fifo;	/* to pass down fifo inof to hnddma module */
	struct dma_info	*linked_di;	/* dma link during fifo split  */
	bool		rxfill_suspended;
	/* PLEASE READ THE BELOW before adding new fields
	 * before adding new small fields at the end, please look at the above struct and see
	 * if you could Squeeze the field in the known alignment holes...point is dma_attach is not
	 * an aatachfn, because there are cases where this gets called run time for some compiles
	*/
	bool		dmapad_required;
	bool		d11rx_war;	/* JIRA:CRWLDOT11M-1776; Rx DMA buffer cannot exceed
					 * 492 bytes
					 */
	uint8		rxwaitforcomplt;
	uint32		d64_rs1_ad_mask; /* rx active descriptor pointer mask */
#ifdef BCM_SECURE_DMA
	struct sec_cma_info sec_cma_info_rx;
	struct sec_cma_info sec_cma_info_tx;
#endif /* BCM_SECURE_DMA */

	dma_common_t    *dmacommon; /* dma_common_t handle */
	uint32          trans_coherent; /* coherent per transaction */
} dma_info_t;

#ifdef BCM_DMA_INDIRECT
#define DMA_INDIRECT(di) ((di)->indirect_dma)
#else
#define DMA_INDIRECT(di) FALSE
#endif


/*
 * If BCMDMA32 is defined, hnddma will support both 32-bit and 64-bit DMA engines.
 * Otherwise it will support only 64-bit.
 *
 * DMA32_ENAB indicates whether hnddma is compiled with support for 32-bit DMA engines.
 * DMA64_ENAB indicates whether hnddma is compiled with support for 64-bit DMA engines.
 *
 * DMA64_MODE indicates whether the current DMA engine is running as 64-bit.
 */
#ifdef BCMDMA32
#define	DMA32_ENAB(di)		1
#define	DMA64_ENAB(di)		1
#define	DMA64_MODE(di)		((di)->dma64)
#else /* !BCMDMA32 */
#define	DMA32_ENAB(di)		0
#define	DMA64_ENAB(di)		1
#define	DMA64_MODE(di)		1
#endif /* !BCMDMA32 */

/* DMA Scatter-gather list is supported. Note this is limited to TX direction only */
#ifdef BCMDMASGLISTOSL
#define DMASGLIST_ENAB TRUE
#else
#define DMASGLIST_ENAB FALSE
#endif /* BCMDMASGLISTOSL */

/* descriptor bumping macros */
#define	XXD(x, n)	((x) & ((n) - 1))	/* faster than %, but n must be power of 2 */
#define	TXD(x)		XXD((x), di->ntxd)
#define	RXD(x)		XXD((x), di->nrxd)
#define	RXDPERHANDLE(x, handler)		(XXD((x), handler->nrxd))

#define	NEXTTXD(i)	TXD((i) + 1)
#define	PREVTXD(i)	TXD((i) - 1)
#define	NEXTRXD(i)	RXD((i) + 1)
#define	PREVRXD(i)	RXD((i) - 1)
#define	PREVNRXD(i, n)	((i < n) ? (di->nrxd - (n - i)) : (i - n))

#define	NTXDACTIVE(h, t)	TXD((t) - (h))
#define	NRXDACTIVE(h, t)	RXD((t) - (h))
#define	NRXDACTIVEPERHANDLE(h, t, handler)		(RXDPERHANDLE(((t) - (h)), handler))

/* macros to convert between byte offsets and indexes */
#define	B2I(bytes, type)	((uint16)((bytes) / sizeof(type)))
#define	I2B(index, type)	((index) * sizeof(type))

#define	PCI32ADDR_HIGH		0xc0000000	/* address[31:30] */
#define	PCI32ADDR_HIGH_SHIFT	30		/* address[31:30] */

#define	PCI64ADDR_HIGH		0x80000000	/* address[63] */
#define	PCI64ADDR_HIGH_SHIFT	31		/* address[63] */

void dma32_dd_upd(dma_info_t *di, dma32dd_t *ddring, dmaaddr_t pa, uint outidx, uint32 *flags,
	uint32 bufcount);
void dma64_dd_upd(dma_info_t *di, dma64dd_t *ddring, dmaaddr_t pa, uint outidx, uint32 *flags,
	uint32 bufcount);
void dma64_dd_upd_64_from_params(dma_info_t *di, dma64dd_t *ddring, dma64addr_t pa,
	uint outidx, uint32 *flags, uint32 bufcount);

uintptr _dma_getvar(dma_info_t *di, const char *name);
#ifdef WLCXO_DATA
bool cxo_data_dma_rxfill(dma_info_t *di);
void *cxo_data_dma_rx(dma_info_t *di);
void cxo_data_dma_rxreclaim(dma_info_t *di);
#else /* WLCXO_DATA */
bool _dma_rxfill(dma_info_t *di);
void *_dma_rx(dma_info_t *di);
void _dma_rxreclaim(dma_info_t *di);
bool dma32_rxenabled(dma_info_t *di);
#endif /* WLCXO_DATA */
void *dma32_getnextrxp(dma_info_t *di, bool forceall);
bool dma32_rxidle(dma_info_t *di);
void *_dma_getnextrxp(dma_info_t *di, bool forceall);
void *dma64_getnextrxp(dma_info_t *di, bool forceall);
bool dma64_rxenabled(dma_info_t *di);
bool dma64_rxidle(dma_info_t *di);

#ifdef WLCXO_DATA
int  cxo_data_dma64_txfast(dma_info_t *di, void *p0, bool commit);
void cxo_data_dma64_txreclaim(dma_info_t *di, txd_range_t range);
#else /* WLCXO_DATA */
void *dma32_getnexttxp(dma_info_t *di, txd_range_t range);
int  dma32_txfast(dma_info_t *di, void *p0, bool commit);
void dma32_txreclaim(dma_info_t *di, txd_range_t range);
void dma32_txcommit(dma_info_t *di);
#endif /* WLCXO_DATA */
int  dma64_txfast(dma_info_t *di, void *p0, bool commit);
void dma64_txreclaim(dma_info_t *di, txd_range_t range);
void *dma64_getnexttxp(dma_info_t *di, txd_range_t range);
void dma64_txcommit(dma_info_t *di);

/** Check for odd number of 1's */
STATIC INLINE uint32 parity32(uint32 data)
{
	data ^= data >> 16;
	data ^= data >> 8;
	data ^= data >> 4;
	data ^= data >> 2;
	data ^= data >> 1;

	return (data & 1);
}

#define DMA64_DD_PARITY(dd)  parity32((dd)->addrlow ^ (dd)->addrhigh ^ (dd)->ctrl1 ^ (dd)->ctrl2)


/**
 * Helper routine to do commit only operation on descriptors
 */

/* Local inline copy, the macro causes inlining to happen in the dongle case */
#define dma32_txcommit_local(di) \
	W_REG((di)->osh, &(di)->d32txregs->ptr, I2B((di)->txout, dma32dd_t))

#define dma64_txcommit_local(di) \
	W_REG((di)->osh, &(di)->d64txregs->ptr, (di)->xmtptrbase + I2B((di)->txout, dma64dd_t))

#endif /* _HNDDMA_PRIV_H_ */
