/*
 * Generic Broadcom Home Networking Division (HND) DMA receive routines.
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
 * $Id: hnddma_rx.c 650258 2016-07-20 22:39:09Z $
 */

/**
 * @file
 * @brief
 * Source file for HNDDMA module. This file contains the functionality for the RX data path.
 */

#include <bcm_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <osl.h>
#include <bcmendian.h>
#include <hndsoc.h>
#include <bcmutils.h>
#include <siutils.h>

#ifdef HNDCTF
#include <ctf/hndctf.h>
#endif

#include <sbhnddma.h>
#include <hnddma.h>
#include "hnddma_priv.h"

#if defined(D11_SPLIT_RX_FD)
static bool dma_splitrxfill(dma_info_t *di);
#endif

#ifdef WLCXO_DATA
#include <d11_cfg.h>
/* default dma message level (if input msg_level pointer is null in dma_attach()) */
static uint dma_msg_level =
	0;

const di_fcn_t dma64proc_cxo = {
	.txreclaim	= (di_txreclaim_t)cxo_data_dma64_txreclaim,
	.rx		= (di_rx_t)cxo_data_dma_rx,
	.rxfill		= (di_rxfill_t)cxo_data_dma_rxfill,
	.rxreclaim	= (di_rxreclaim_t)cxo_data_dma_rxreclaim,
	.getnextrxp	= (di_getnextrxp_t)_dma_getnextrxp,
	.rxenabled	= (di_rxenabled_t)dma64_rxenabled,
	.rxidle		= (di_rxidle_t)dma64_rxidle,
	.txfast		= (di_txfast_t)cxo_data_dma64_txfast,
	.getnexttxp	= (di_getnexttxp_t)dma64_getnexttxp,
	.d_getvar	= (di_getvar_t)_dma_getvar
};
#endif /* WLCXO_DATA */

/**
 * !! rx entry routine
 * returns a pointer to the next frame received, or NULL if there are no more
 *   if DMA_CTRL_RXMULTI is defined, DMA scattering(multiple buffers) is supported
 *      with pkts chain
 *   otherwise, it's treated as giant pkt and will be tossed.
 *   The DMA scattering starts with normal DMA header, followed by first buffer data.
 *   After it reaches the max size of buffer, the data continues in next DMA descriptor
 *   buffer WITHOUT DMA header
 */
void * BCMFASTPATH
#ifdef WLCXO_DATA
cxo_data_dma_rx(dma_info_t *di)
#else
_dma_rx(dma_info_t *di)
#endif /* WLCXO_DATA */
{
	void *p, *head, *tail;
	uint len;
	uint pkt_len;
	int resid = 0;
#if defined(D11_SPLIT_RX_FD)
	uint tcm_maxsize = 0;		/* max size of tcm descriptor */
#endif
	void *data;

next_frame:
	head = _dma_getnextrxp(di, FALSE);
	if (head == NULL)
		return (NULL);

	if (di->split_fifo == SPLIT_FIFO_0) {
		/* Fifo-0 handles all host address */
		/* Below PKT ops are not valid for host pkts */
		goto ret;
	}

	data = PKTDATA(di->osh, head);

#if (!defined(__mips__) && !(defined(BCM47XX_CA9) || defined(STB)))
	if (di->hnddma.dmactrlflags & DMA_CTRL_SDIO_RXGLOM) {
		/* In case of glommed pkt get length from hwheader */
		len = ltoh16(*((uint16 *)(data) + di->rxoffset/2 + 2)) + 4;

		*(uint16 *)(data) = (uint16)len;
	} else {
		len = ltoh16(*(uint16 *)(data));
	}
#else  /* !__mips__ && !BCM47XX_CA9 && !__NetBSD__ */
	{
	int read_count = 0;
	for (read_count = 200; read_count; read_count--) {
		len = ltoh16(*(uint16 *)(data));
		if (len != 0)
			break;
			DMA_MAP(di->osh, data, sizeof(uint16), DMA_RX, NULL, NULL);
		OSL_DELAY(1);
	}

	if (!len) {
		DMA_ERROR(("%s: dma_rx: frame length (%d)\n", di->name, len));
		PKTFREE(di->osh, head, FALSE);
		goto next_frame;
	}

	}
#endif 
	DMA_TRACE(("%s: dma_rx len %d\n", di->name, len));

	/* set actual length */
	pkt_len = MIN((di->rxoffset + len), di->rxbufsize);

#if defined(D11_SPLIT_RX_FD)
	if (di->sep_rxhdr) {
		/* If separate desc feature is enabled, set length for lcl & host pkt */
		tcm_maxsize = PKTLEN(di->osh, head);
		/* Dont support multi buffer pkt for now */
		if (pkt_len <= tcm_maxsize) {
			/* Full pkt sitting in TCM */
			PKTSETLEN(di->osh, head, pkt_len);	/* LCL pkt length */
			PKTSETFRAGUSEDLEN(di->osh, head, 0);	/* Host segment length */
		} else {
			/* Pkt got split between TCM and host */
			PKTSETLEN(di->osh, head, tcm_maxsize);	/* LCL pkt length */
			/* use PKTFRAGUSEDLEN to indicate actual length dma ed  by d11 */
			/* Cant use PKTFRAGLEN since if need to reclaim this */
			/* we need fraglen intact */
			PKTSETFRAGUSEDLEN(di->osh, head,
				(pkt_len - tcm_maxsize)); /* Host segment length */
		}
	} else
#endif /* D11_SPLIT_RX_FD */
	{
		PKTSETLEN(di->osh, head, pkt_len);
	}

	resid = len - (di->rxbufsize - di->rxoffset);

	if (resid <= 0) {
		/* Single frame, all good */
	} else if (di->hnddma.dmactrlflags & DMA_CTRL_RXSINGLE) {
		DMA_TRACE(("%s: dma_rx: corrupted length (%d)\n", di->name, len));
		PKTFREE(di->osh, head, FALSE);
		di->hnddma.rxgiants++;
		goto next_frame;
	} else {
		/* multi-buffer rx */
		tail = head;
		while ((resid > 0) && (p = _dma_getnextrxp(di, FALSE))) {
			PKTSETNEXT(di->osh, tail, p);
			pkt_len = MIN(resid, (int)di->rxbufsize);
#if defined(D11_SPLIT_RX_FD)
			if (di->sep_rxhdr)
				PKTSETLEN(di->osh, p, MIN(pkt_len, tcm_maxsize));
			else
#endif /* D11_SPLIT_RX_FD */
			PKTSETLEN(di->osh, p, pkt_len);

			tail = p;
			resid -= di->rxbufsize;
		}


		if ((di->hnddma.dmactrlflags & DMA_CTRL_RXMULTI) == 0) {
			DMA_ERROR(("%s: dma_rx: bad frame length (%d)\n", di->name, len));
			PKTFREE(di->osh, head, FALSE);
			di->hnddma.rxgiants++;
			goto next_frame;
		}
	}

ret:
#if defined(BCM47XX_CA9) || defined(STB) || defined(BCM7271)
	/* JIRA: SWWLAN-23796 */
	DMA_MAP(di->osh, PKTDATA(di->osh, head), PKTLEN(di->osh, head), DMA_RX, head, NULL);
#endif /* defined(BCM47XX_CA9) */

	return (head);
}


/**
 * A 'receive' DMA engine must be fed with buffers to write received data into. This function
 * 'posts' receive buffers. If the 'packet pool' feature is enabled, the buffers are drawn from the
 * packet pool. Otherwise, the buffers are retrieved using the OSL 'PKTGET' macro.
 *
 *  return FALSE is refill failed completely and ring is empty
 *  this will stall the rx dma and user might want to call rxfill again asap
 *  This unlikely happens on memory-rich NIC, but often on memory-constrained dongle
 */
bool BCMFASTPATH
#ifdef WLCXO_DATA
cxo_data_dma_rxfill(dma_info_t *di)
#else
_dma_rxfill(dma_info_t *di)
#endif /* WLCXO_DATA */
{
	void *p;
	uint16 rxin, rxout;
	uint16 rxactive;
	uint32 flags = 0;
	uint n;
	uint i;
	dmaaddr_t pa;
	uint extra_offset = 0, extra_pad = 0;
	bool ring_empty;
	uint alignment_req = (di->hnddma.dmactrlflags & DMA_CTRL_USB_BOUNDRY4KB_WAR) ?
				16 : 1;	/* MUST BE POWER of 2 */

	/* if sep_rxhdr is enabled, for every pkt, two descriptors are programmed */
	/* NRXDACTIVE(rxin, rxout) would show 2 times no of actual full pkts */
	uint dpp = (di->sep_rxhdr) ? 2 : 1; /* dpp - descriptors per packet */
#if defined(D11_SPLIT_RX_FD)
	uint pktlen;
	dma64addr_t pa64 = {0, 0};

	if (di->rxfill_suspended)
		return FALSE;

	if (di->split_fifo) {
		/* SPLIFIFO rxfill is handled separately */
		return dma_splitrxfill(di);
	}
#endif /* D11_SPLIT_RX_FD */

	if (di->rxfill_suspended)
		return FALSE;

	if (di->d11rx_war) {
		int plen = 0;
		dpp = 0; /* Recompute descriptors per pkt, for D11 RX WAR */
#ifdef BCMPKTPOOL
		if (POOL_ENAB(di->pktpool)) {
			plen = pktpool_max_pkt_bytes(di->pktpool);
		}
		else
#endif /* BCMPKTPOOL */
		{
			plen = di->rxbufsize;
		}

		if (di->sep_rxhdr) {
			dpp = 1; /* TCM + (HOST breakup) */
			plen = MAXRXBUFSZ - plen;
		}
		dpp += CEIL(plen, D11RX_WAR_MAX_BUF_SIZE);
	}

	ring_empty = FALSE;

	/*
	 * Determine how many receive buffers we're lacking
	 * from the full complement, allocate, initialize,
	 * and post them, then update the chip rx lastdscr.
	 */

	rxin = di->rxin;
	rxout = di->rxout;
	rxactive = NRXDACTIVE(rxin, rxout);

	/* if currently posted is more than requested, no need to do anything */
	if (MIN((di->nrxd/dpp), di->nrxpost) < CEIL(rxactive, dpp))
		return TRUE;

	/* No. of packets to post is : n = MIN(nrxd, nrxpost) - NRXDACTIVE;
	 * But, if (dpp/descriptors per packet > 1), we need this adjustment.
	 */
	n = MIN((di->nrxd/dpp), di->nrxpost) - CEIL(rxactive, dpp);

	if (di->rxbufsize > BCMEXTRAHDROOM)
		extra_offset = di->rxextrahdrroom;

	DMA_TRACE(("%s: dma_rxfill: post %d\n", di->name, n));

	for (i = 0; i < n; i++) {
		int dlen = 0;
		int ds_len = 0;
		/* the di->rxbufsize doesn't include the extra headroom, we need to add it to the
		   size to be allocated
		*/
#ifdef BCMPKTPOOL
		if (POOL_ENAB(di->pktpool)) {
			ASSERT(di->pktpool);
			p = pktpool_get(di->pktpool);
#ifdef BCMDBG_POOL
			if (p)
				PKTPOOLSETSTATE(p, POOL_RXFILL);
#endif /* BCMDBG_POOL */
		} else
#endif /* BCMPKTPOOL */
		{
#ifdef WLCXO_DATA
			if (BCMSPLITRX_ENAB() && !di->sep_rxhdr) {
				p = PKTGET(di->osh, (di->rxbufsize + extra_offset +
					alignment_req - 1), CXO_PKTPOOL_RX1);
			} else {
				p = PKTGET(di->osh, (di->rxbufsize + extra_offset +
					alignment_req - 1), CXO_PKTPOOL_RX);
			}
#else /* WLCXO_DATA */
			p = PKTGET(di->osh, (di->rxbufsize + extra_offset +
				alignment_req - 1), FALSE);
#endif /* WLCXO_DATA */
		}
		if (p == NULL) {
			DMA_TRACE(("%s: dma_rxfill: out of rxbufs\n", di->name));
			if (i == 0) {
				if (DMA64_ENAB(di) && DMA64_MODE(di)) {
					if (dma64_rxidle(di)) {
						DMA_TRACE(("%s: rxfill64: ring is empty !\n",
							di->name));
						ring_empty = TRUE;
					}
				} else if (DMA32_ENAB(di)) {
					if (dma32_rxidle(di)) {
						DMA_TRACE(("%s: rxfill32: ring is empty !\n",
							di->name));
						ring_empty = TRUE;
					}
				} else
					ASSERT(0);
			}
			di->hnddma.rxnobuf += (n - i);
			break;
		}
		/* reserve an extra headroom, if applicable */
		if (di->hnddma.dmactrlflags & DMA_CTRL_USB_BOUNDRY4KB_WAR) {
			extra_pad = ((alignment_req - (((uintptr)PKTDATA(di->osh, p) -
				(uintptr)(uchar *)0))) & (alignment_req - 1));
		} else
			extra_pad = 0;

		if (extra_offset + extra_pad)
			PKTPULL(di->osh, p, extra_offset + extra_pad);


		/* Do a cached write instead of uncached write since DMA_MAP
		 * will flush the cache.
		*/
		/* Keep it 32 bit-wide to prevent asserts: RB:17293 JIRA:SWWLAN-36682 */
		*(uint32 *)(PKTDATA(di->osh, p)) = 0;


#if (defined(BCM47XX_CA9) || defined(STB) || defined(__mips__))
		DMA_MAP(di->osh, PKTDATA(di->osh, p), sizeof(uint32), DMA_TX, NULL, NULL);
#endif
#if defined(SGLIST_RX_SUPPORT)
		if (DMASGLIST_ENAB)
			bzero(&di->rxp_dmah[rxout], sizeof(hnddma_seg_map_t));

#ifdef BCM_SECURE_DMA
		pa = SECURE_DMA_MAP(di->osh, PKTDATA(di->osh, p), di->rxbufsize, DMA_RX,
			NULL, NULL, &di->sec_cma_info_rx, 0);
#else
		pa = DMA_MAP(di->osh, PKTDATA(di->osh, p), di->rxbufsize, DMA_RX, p,
			&di->rxp_dmah[rxout]);
#endif /* BCM_SECURE_DMA */
#else  /* !SGLIST_RX_SUPPORT */
#ifdef BCM_SECURE_DMA
		pa = SECURE_DMA_MAP(di->osh, PKTDATA(di->osh, p), di->rxbufsize, DMA_RX,
			NULL, NULL, &di->sec_cma_info_rx, 0);
#else
		pa = DMA_MAP(di->osh, PKTDATA(di->osh, p),
		             di->rxbufsize, DMA_RX, p, NULL);
#endif /* BCM_SECURE_DMA */
#endif /* !SGLIST_RX_SUPPORT */

		ASSERT(ISALIGNED(PHYSADDRLO(pa), 4));

		/* save the free packet pointer */
		ASSERT(di->rxp[rxout] == NULL);
		di->rxp[rxout] = p;


#if defined(D11_SPLIT_RX_FD)
		if (!di->sep_rxhdr)
#endif
		{
			/* reset flags for each descriptor */
			flags = D64_CTRL1_SOF;
			dlen = di->rxbufsize;
			ds_len = (di->d11rx_war) ? D11RX_WAR_MAX_BUF_SIZE : dlen;
			do {
				uint64 addr;
				if (DMA64_ENAB(di) && DMA64_MODE(di)) {
					if (rxout == (di->nrxd - 1))
						flags |= D64_CTRL1_EOT;
					dma64_dd_upd(di, di->rxd64, pa, rxout, &flags,
						MIN(dlen, ds_len));
				} else if (DMA32_ENAB(di)) {
					if (rxout == (di->nrxd - 1))
						flags |= CTRL_EOT;
					ASSERT(PHYSADDRHI(pa) == 0);
					dma32_dd_upd(di, di->rxd32, pa, rxout, &flags,
						MIN(dlen, ds_len));
				} else {
					ASSERT(0);
				}

				rxout = NEXTRXD(rxout);
				dlen -= ds_len;
				if (dlen <= 0) {
					/* bail out if we are done */
					break;
				}
				di->rxp[rxout] = (void*)PCI64ADDR_HIGH;

				/* prep for next descriptor */
				addr = (((uint64)PHYSADDRHI(pa) << 32) | PHYSADDRLO(pa)) + ds_len;
				PHYSADDRHISET(pa, (uint32)(addr >> 32));
				PHYSADDRLOSET(pa, (uint32)(addr & 0xFFFFFFFF));
				flags = 0;
			} while (dlen > 0);
		}
#if defined(D11_SPLIT_RX_FD)
		else {

			/* TCM Descriptor */

			flags = 0;
			pktlen = PKTLEN(di->osh, p);
			if (rxout == (di->nrxd - 1))
				flags = D64_CTRL1_EOT;

			/* MAR SOF, so start frame will go to this descriptor */
			flags |= D64_CTRL1_SOF;
			dma64_dd_upd(di, di->rxd64, pa, rxout, &flags, pktlen);
			rxout = NEXTRXD(rxout);		/* update rxout */

			/* Now program host descriptor(s) */

			/* Extract out host addresses */
			pa64.loaddr = PKTFRAGDATA_LO(di->osh, p, 1);
			pa64.hiaddr = PKTFRAGDATA_HI(di->osh, p, 1);

			/* Extract out host length */
			dlen = pktlen = PKTFRAGLEN(di->osh, p, 1);
			ds_len = (di->d11rx_war) ? D11RX_WAR_MAX_BUF_SIZE : pktlen;

			do {
				uint64 addr  = 0;
				/* Mark up this descriptor that its a host descriptor
				* store 0x80000000 so that dma_rx need'nt
				* process this descriptor
				*/
				di->rxp[rxout] = (void*)PCI64ADDR_HIGH;
				flags = 0;	/* Reset the flags */

				if (rxout == (di->nrxd - 1))
					flags = D64_CTRL1_EOT;

				/* load the descriptor */
				dma64_dd_upd_64_from_params(di, di->rxd64, pa64, rxout,
					&flags,	MIN(ds_len, dlen));
				rxout = NEXTRXD(rxout);	/* update rxout */

				/* prep for next descriptor */
				dlen -= ds_len;
				if (dlen <= 0) break; /* bail out if we are done */
				addr =  (((uint64)pa64.hiaddr << 32) | pa64.loaddr) + ds_len;
				pa64.hiaddr = (uint32)(addr >> 32);
				pa64.loaddr = (uint32)(addr & 0xFFFFFFFF);
			} while (dlen > 0);
		}
#endif /* D11_SPLIT_RX_FD */
	}

#if !defined(BULK_DESCR_FLUSH)
	di->rxout = rxout;
#endif

	/* update the chip lastdscr pointer */
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
#if defined(BULK_DESCR_FLUSH)
		uint32 flush_cnt = NRXDACTIVE(di->rxout, rxout);
		if (rxout < di->rxout) {
			DMA_MAP(di->osh, dma64_rxd64(di, 0), DMA64_FLUSH_LEN(rxout),
			        DMA_TX, NULL, NULL);
			flush_cnt -= rxout;
		}
		DMA_MAP(di->osh, dma64_rxd64(di, di->rxout), DMA64_FLUSH_LEN(flush_cnt),
		        DMA_TX, NULL, NULL);
#endif /* BULK_DESCR_FLUSH */
		W_REG(di->osh, &di->d64rxregs->ptr, di->rcvptrbase + I2B(rxout, dma64dd_t));
	} else if (DMA32_ENAB(di)) {
		W_REG(di->osh, &di->d32rxregs->ptr, I2B(rxout, dma32dd_t));
	} else
		ASSERT(0);

#if defined(BULK_DESCR_FLUSH)
	di->rxout = rxout;
#endif

	return !ring_empty;
} /* _dma_rxfill */

void  BCMFASTPATH
#ifdef WLCXO_DATA
cxo_data_dma_rxreclaim(dma_info_t *di)
#else
_dma_rxreclaim(dma_info_t *di)
#endif
{
	void *p;
#ifdef BCMPKTPOOL
	bool origcb = TRUE;
#endif /* BCMPKTPOOL */

	DMA_TRACE(("%s: dma_rxreclaim\n", di->name));

#ifdef BCMPKTPOOL
	if (POOL_ENAB(di->pktpool) &&
	    ((origcb = pktpool_emptycb_disabled(di->pktpool)) == FALSE))
		pktpool_emptycb_disable(di->pktpool, TRUE);
#endif /* BCMPKTPOOL */

	/* For split fifo, this function expects fifo-1 di to proceed further. */
	/* fifo-0 requests can be discarded since fifo-1 will reclaim both fifos */
	if (di->split_fifo == SPLIT_FIFO_0) {
		return;
	}

	while ((p = _dma_getnextrxp(di, TRUE))) {
		/* For unframed data, we don't have any packets to free */
#if (defined(__mips__) || defined(BCM47XX_CA9) || defined(STB))
		if (!(di->hnddma.dmactrlflags & DMA_CTRL_UNFRAMED))
#endif /* (__mips__ || BCM47XX_CA9) && linux */
		{
#if defined(BCMSPLITRX) && defined(D11_SPLIT_RX_FD)
			if (PKTISRXFRAG(di->osh, p)) {
				PKTRESETFIFO0INT(di->osh, p);
				PKTRESETFIFO1INT(di->osh, p);
			}
#endif /* BCMSPLITRX && D11_SPLIT_RX_FD */
			PKTFREE(di->osh, p, FALSE);
		}
	}

#ifdef BCMPKTPOOL
	if (POOL_ENAB(di->pktpool) && origcb == FALSE)
		pktpool_emptycb_disable(di->pktpool, FALSE);
#endif /* BCMPKTPOOL */
}

#ifndef WLCXO_DATA

/**
 * Initializes one tx or rx descriptor with the caller provided arguments, notably a buffer.
 *    @param[in] di       Handle
 *    @param[in] ddring   Pointer to properties of the descriptor ring
 *    @param[in] pa       Physical address of rx or tx buffer that the descriptor should point at
 *    @param[in] outidx   Index of the targetted DMA descriptor within the descriptor ring
 *    @param[in] flags    Value that is written into the descriptors 'ctrl1' field
 *    @param[in] bufcount Buffer count value, written into the descriptors 'ctrl2' field
 */
void
dma32_dd_upd(dma_info_t *di, dma32dd_t *ddring, dmaaddr_t pa, uint outidx, uint32 *flags,
	uint32 bufcount)
{
	/* dma32 uses 32-bit control to fit both flags and bufcounter */
	*flags = *flags | (bufcount & CTRL_BC_MASK);

	if ((di->dataoffsetlow == 0) || !(PHYSADDRLO(pa) & PCI32ADDR_HIGH)) {
		W_SM(&ddring[outidx].addr, BUS_SWAP32(PHYSADDRLO(pa) + di->dataoffsetlow));
		W_SM(&ddring[outidx].ctrl, BUS_SWAP32(*flags));
	} else {
		/* address extension */
		uint32 ae;
		ASSERT(di->addrext);
		ae = (PHYSADDRLO(pa) & PCI32ADDR_HIGH) >> PCI32ADDR_HIGH_SHIFT;
		PHYSADDRLO(pa) &= ~PCI32ADDR_HIGH;

		*flags |= (ae << CTRL_AE_SHIFT);
		W_SM(&ddring[outidx].addr, BUS_SWAP32(PHYSADDRLO(pa) + di->dataoffsetlow));
		W_SM(&ddring[outidx].ctrl, BUS_SWAP32(*flags));
	}
}

bool
dma32_rxidle(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_rxidle\n", di->name));

	if (di->nrxd == 0)
		return TRUE;

	return ((R_REG(di->osh, &di->d32rxregs->status) & RS_CD_MASK) ==
	        R_REG(di->osh, &di->d32rxregs->ptr));
}

bool
dma32_rxenabled(dma_info_t *di)
{
	uint32 rc;

	rc = R_REG(di->osh, &di->d32rxregs->control);
	return ((rc != 0xffffffff) && (rc & RC_RE));
}


void *
dma32_getnextrxp(dma_info_t *di, bool forceall)
{
	uint16 i, curr;
	void *rxp;
	dmaaddr_t pa;
	/* if forcing, dma engine must be disabled */
	ASSERT(!forceall || !dma32_rxenabled(di));

	i = di->rxin;

	/* return if no packets posted */
	if (i == di->rxout)
		return (NULL);

	if (di->rxin == di->rs0cd) {
		curr = (uint16)B2I(R_REG(di->osh, &di->d32rxregs->status) & RS_CD_MASK, dma32dd_t);
		di->rs0cd = curr;
	} else
		curr = di->rs0cd;

	/* ignore curr if forceall */
	if (!forceall && (i == curr))
		return (NULL);

	/* get the packet pointer that corresponds to the rx descriptor */
	rxp = di->rxp[i];
	ASSERT(rxp);
	di->rxp[i] = NULL;


	PHYSADDRLOSET(pa, (BUS_SWAP32(R_SM(&di->rxd32[i].addr)) - di->dataoffsetlow));
	PHYSADDRHISET(pa, 0);

	/* clear this packet from the descriptor ring */
#ifdef BCM_SECURE_DMA
	SECURE_DMA_UNMAP(di->osh, pa, di->rxbufsize, DMA_RX, NULL, NULL,
		&di->sec_cma_info_rx, 0);
#else
	DMA_UNMAP(di->osh, pa, di->rxbufsize, DMA_RX, rxp, &di->rxp_dmah[i]);
#endif /* BCM_SECURE_DMA */
#if defined(DESCR_DEADBEEF)
	W_SM(&di->rxd32[i].addr, 0xdeadbeef);
#endif

	di->rxin = NEXTRXD(i);

	return (rxp);
}
#endif /* WLCXO_DATA */

#if !defined(WLCXO_DATA) || !defined(WLCXO_FULL)

/**
 * Initializes one tx or rx descriptor with the caller provided arguments, notably a buffer.
 * Knows how to handle native 64-bit addressing AND bit64 extension.
 *    @param[in] di       Handle
 *    @param[in] ddring   Pointer to properties of the descriptor ring
 *    @param[in] pa       Physical address of rx or tx buffer that the descriptor should point at
 *    @param[in] outidx   Index of the targetted DMA descriptor within the descriptor ring
 *    @param[in] flags    Value that is written into the descriptors 'ctrl1' field
 *    @param[in] bufcount Buffer count value, written into the descriptors 'ctrl2' field
 */
void BCMFASTPATH_CXO
dma64_dd_upd(dma_info_t *di, dma64dd_t *ddring, dmaaddr_t pa, uint outidx, uint32 *flags,
	uint32 bufcount)
{
	uint32 ctrl2 = bufcount & D64_CTRL2_BC_MASK;

	/* PCI bus with big(>1G) physical address, use address extension */
#if defined(__mips__) && defined(IL_BIGENDIAN)
	if ((di->dataoffsetlow == SI_SDRAM_SWAPPED) || !(PHYSADDRLO(pa) & PCI32ADDR_HIGH)) {
#else
	if ((di->dataoffsetlow == 0) || !(PHYSADDRLO(pa) & PCI32ADDR_HIGH)) {
#endif /* defined(__mips__) && defined(IL_BIGENDIAN) */
		/* This is where 64-bit addr ext will come into picture but most likely
		 * nobody will be around by the time we have full 64-bit memory addressing
		 * requirement
		 */
		ASSERT((PHYSADDRHI(pa) & PCI64ADDR_HIGH) == 0);

		W_SM(&ddring[outidx].addrlow, BUS_SWAP32(PHYSADDRLO(pa) + di->dataoffsetlow));
		W_SM(&ddring[outidx].addrhigh, BUS_SWAP32(PHYSADDRHI(pa) + di->dataoffsethigh));
		W_SM(&ddring[outidx].ctrl1, BUS_SWAP32(*flags));
		W_SM(&ddring[outidx].ctrl2, BUS_SWAP32(ctrl2));
	} else {
		/* address extension for 32-bit PCI */
		uint32 ae;
		ASSERT(di->addrext);

		ae = (PHYSADDRLO(pa) & PCI32ADDR_HIGH) >> PCI32ADDR_HIGH_SHIFT;
		PHYSADDRLO(pa) &= ~PCI32ADDR_HIGH;
		ASSERT(PHYSADDRHI(pa) == 0);

		ctrl2 |= (ae << D64_CTRL2_AE_SHIFT) & D64_CTRL2_AE;
		W_SM(&ddring[outidx].addrlow, BUS_SWAP32(PHYSADDRLO(pa) + di->dataoffsetlow));
		W_SM(&ddring[outidx].addrhigh, BUS_SWAP32(0 + di->dataoffsethigh));
		W_SM(&ddring[outidx].ctrl1, BUS_SWAP32(*flags));
		W_SM(&ddring[outidx].ctrl2, BUS_SWAP32(ctrl2));
	}

	if (di->hnddma.dmactrlflags & DMA_CTRL_PEN) {
		if (DMA64_DD_PARITY(&ddring[outidx])) {
			W_SM(&ddring[outidx].ctrl2, BUS_SWAP32(ctrl2 | D64_CTRL2_PARITY));
		}
	}

#if (defined(BCM47XX_CA9) || defined(STB)) && !defined(BULK_DESCR_FLUSH)
#ifdef BCM_SECURE_DMA
	SECURE_DMA_DD_MAP(di->osh, (void *)(((uint)(&ddring[outidx])) & ~0x1f),
		32, DMA_TX, NULL, NULL);
#else
#if (__LINUX_ARM_ARCH__ == 8)
	DMA_MAP(di->osh, (void *)(((uint64)(&ddring[outidx])) & ~0x1f), 32, DMA_TX, NULL, NULL);
#else
	DMA_MAP(di->osh, (void *)(((uint)(&ddring[outidx])) & ~0x1f), 32, DMA_TX, NULL, NULL);
#endif /* (__LINUX_ARM_ARCH__ == 8) */
#endif /* BCM_SECURE_DMA */
#endif 

	/* memory barrier before posting the descriptor */
	DMB();

} /* dma64_dd_upd */

/** returns entries on the ring, in the order in which they were placed on the ring */
void * BCMFASTPATH
_dma_getnextrxp(dma_info_t *di, bool forceall)
{
	if (di->nrxd == 0)
		return (NULL);

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		return dma64_getnextrxp(di, forceall);
	} else if (DMA32_ENAB(di)) {
		return dma32_getnextrxp(di, forceall);
	} else
		ASSERT(0);
}

bool
dma64_rxidle(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_rxidle\n", di->name));

	if (di->nrxd == 0)
		return TRUE;

	return ((R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_CD_MASK) ==
		(R_REG(di->osh, &di->d64rxregs->ptr) & D64_RS0_CD_MASK));
}

bool
dma64_rxenabled(dma_info_t *di)
{
	uint32 rc;

	rc = R_REG(di->osh, &di->d64rxregs->control);
	return ((rc != 0xffffffff) && (rc & D64_RC_RE));
}

/** returns entries on the ring, in the order in which they were placed on the ring */
void * BCMFASTPATH
dma64_getnextrxp(dma_info_t *di, bool forceall)
{
	uint16 i, curr;
	void *rxp;
#if (!defined(__mips__) && !(defined(BCM47XX_CA9) || defined(STB))) || \
	defined(BCM_SECURE_DMA)
	dmaaddr_t pa;
#endif

#ifdef BCM_BACKPLANE_TIMEOUT
	if (!si_deviceremoved(di->sih))
	{
		/* if forcing, dma engine must be disabled */
		ASSERT(!forceall || !dma64_rxenabled(di));
	}
#endif /* BCM_BACKPLANE_TIMEOUT */

nextframe:

	i = di->rxin;

	/* return if no packets posted */
	if (i == di->rxout)
		return (NULL);

	if (di->rxin == di->rs0cd) {
		curr = (uint16)B2I(((R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_CD_MASK) -
		di->rcvptrbase) & D64_RS0_CD_MASK, dma64dd_t);
		di->rs0cd = curr;
	} else
		curr = di->rs0cd;

	/* ignore curr if forceall */
	if (!forceall && (i == curr))
		return (NULL);

	/* get the packet pointer that corresponds to the rx descriptor */
	rxp = di->rxp[i];
	ASSERT(rxp);

#if (defined(__mips__) || defined(BCM47XX_CA9) || defined(STB))
	if (!(di->hnddma.dmactrlflags & DMA_CTRL_UNFRAMED)) {
		/* Processor prefetch of 1 x 32B cacheline carrying HWRXOFF */
		uint8 * addr = PKTDATA(di->osh, rxp);
		bcm_prefetch_32B(addr, 1);
	}
#endif /* (__mips__ || BCM47XX_CA9) && linux */

	di->rxp[i] = NULL;

	/* We had marked up host/split descriptors as 0x80000000 */
	/* if we find that address, just skip that and go to next frame */
	if (di->sep_rxhdr || di->d11rx_war) {
		if ((uint32)(uintptr)rxp == PCI64ADDR_HIGH) {
			di->rxin = NEXTRXD(i);
			di->rxavail = di->nrxd - NRXDACTIVE(di->rxin, di->rxout) - 1;
			goto nextframe;
		}
	}


#if defined(SGLIST_RX_SUPPORT)
	PHYSADDRLOSET(pa, (BUS_SWAP32(R_SM(&di->rxd64[i].addrlow)) - di->dataoffsetlow));
	PHYSADDRHISET(pa, (BUS_SWAP32(R_SM(&di->rxd64[i].addrhigh)) - di->dataoffsethigh));

	/* clear this packet from the descriptor ring */
#ifdef BCM_SECURE_DMA
	SECURE_DMA_UNMAP(di->osh, pa, di->rxbufsize, DMA_RX, NULL, NULL,
		&di->sec_cma_info_rx, 0);
#else
	DMA_UNMAP(di->osh, pa, di->rxbufsize, DMA_RX, rxp, &di->rxp_dmah[i]);
#endif /* BCM_SECURE_DMA */
#endif /* SGLIST_RX_SUPPORT */
#if (defined(DESCR_DEADBEEF) || defined(STB))
	W_SM(&di->rxd64[i].addrlow, 0xdeadbeef);
	W_SM(&di->rxd64[i].addrhigh, 0xdeadbeef);
#endif /* DESCR_DEADBEEF */


	di->rxin = NEXTRXD(i);

	di->rxavail = di->nrxd - NRXDACTIVE(di->rxin, di->rxout) - 1;

	return (rxp);
}


void BCMFASTPATH
dma64_dd_upd_64_from_params(dma_info_t *di, dma64dd_t *ddring, dma64addr_t pa, uint outidx,
	uint32 *flags, uint32 bufcount)
{
	uint32 ctrl2 = bufcount & D64_CTRL2_BC_MASK;

	/* bit 63 is arleady set for host addresses by the caller */
	W_SM(&ddring[outidx].addrlow, BUS_SWAP32(pa.loaddr + di->dataoffsetlow));
	W_SM(&ddring[outidx].addrhigh, BUS_SWAP32(pa.hiaddr + di->dataoffsethigh));
	W_SM(&ddring[outidx].ctrl1, BUS_SWAP32(*flags));
	W_SM(&ddring[outidx].ctrl2, BUS_SWAP32(ctrl2));

	if (di->hnddma.dmactrlflags & DMA_CTRL_PEN) {
		if (DMA64_DD_PARITY(&ddring[outidx])) {
			W_SM(&ddring[outidx].ctrl2, BUS_SWAP32(ctrl2 | D64_CTRL2_PARITY));
		}
	}

	/* memory barrier before posting the descriptor */
	DMB();
}
#endif /* !WLCXO_DATA || !WLCXO_FULL */

#if defined(D11_SPLIT_RX_FD)
static bool BCMFASTPATH
dma_splitrxfill(dma_info_t *di)
{
	void *p = NULL;
	uint16 rxin1, rxout1;
	uint16 rxin2, rxout2;
	uint32 flags = 0;
	uint n;
	uint i;
	dmaaddr_t pa;
	uint extra_offset = 0;
	bool ring_empty;
	uint pktlen;
	dma64addr_t pa64 = {0, 0};


	ring_empty = FALSE;
	/*
	 * Determine how many receive buffers we're lacking
	 * from the full complement, allocate, initialize,
	 * and post them, then update the chip rx lastdscr.
	 * Both DMA engines may not have same no. of lacking descrs
	 * Take MIN of both
	 */
	if (di->split_fifo == SPLIT_FIFO_0) {
		/* For split fifo fill, this function expects fifo-1 di to proceed further. */
		/* fifo-0 requests can be discarded since fifo-1 request will fill up both fifos */
		return TRUE;
	}
	rxin1 = di->rxin;
	rxout1 = di->rxout;
	rxin2 = di->linked_di->rxin;
	rxout2 = di->linked_di->rxout;

	/* Always post same no of descriptors to fifo-0 & fifo-1 */
	/* While reposting , consider the fifo which has consumed least no of descriptors */
	n = MIN(di->nrxpost - NRXDACTIVE(rxin1, rxout1),
		di->linked_di->nrxpost - NRXDACTIVEPERHANDLE(rxin2, rxout2, di->linked_di));

	/* Assert that both DMA engines have same pktpools */
	ASSERT(di->pktpool == di->linked_di->pktpool);

	if (di->rxbufsize > BCMEXTRAHDROOM)
		extra_offset = di->rxextrahdrroom;

	for (i = 0; i < n; i++) {
		/* the di->rxbufsize doesn't include the extra headroom, we need to add it to the
		   size to be allocated
		*/
		if (POOL_ENAB(di->pktpool)) {
			ASSERT(di->pktpool);
			p = pktpool_get(di->pktpool);
#ifdef BCMDBG_POOL
			if (p)
				PKTPOOLSETSTATE(p, POOL_RXFILL);
#endif /* BCMDBG_POOL */
		}

		if (p == NULL) {
			DMA_TRACE(("%s: dma_rxfill: out of rxbufs\n", di->name));
			if (i == 0) {
				if (DMA64_ENAB(di) && DMA64_MODE(di)) {
					if (dma64_rxidle(di)) {
						DMA_TRACE(("%s: rxfill64: ring is empty !\n",
							di->name));
						ring_empty = TRUE;
					}
				} else if (DMA32_ENAB(di)) {
					if (dma32_rxidle(di)) {
						DMA_TRACE(("%s: rxfill32: ring is empty !\n",
							di->name));
						ring_empty = TRUE;
					}
				} else
					ASSERT(0);
			}
			di->hnddma.rxnobuf++;
			break;
		}
		if (extra_offset)
			PKTPULL(di->osh, p, extra_offset);

		/* Do a cached write instead of uncached write since DMA_MAP
		 * will flush the cache.
		*/

		*(uint16 *)(PKTDATA(di->osh, p)) = 0;
#if (defined(BCM47XX_CA9) || defined(__mips__) || defined(STB))
		DMA_MAP(di->osh, PKTDATA(di->osh, p), sizeof(uint16), DMA_TX, NULL, NULL);
#endif
		if (DMASGLIST_ENAB)
			bzero(&di->rxp_dmah[rxout1], sizeof(hnddma_seg_map_t));
#if !defined(BCM_SECURE_DMA)
		pa = DMA_MAP(di->osh, PKTDATA(di->osh, p),
			di->rxbufsize, DMA_RX, p,
			&di->rxp_dmah[rxout1]);
#endif /* #if !defined(BCM_SECURE_DMA) */
		ASSERT(ISALIGNED(PHYSADDRLO(pa), 4));

		/* save the free packet pointer */
		ASSERT(di->rxp[rxout1] == NULL);
		di->rxp[rxout1] = p;

		/* TCM Descriptor */
		flags = 0;
		pktlen = PKTLEN(di->osh, p);
		if (rxout1 == (di->nrxd - 1))
			flags = D64_CTRL1_EOT;

		dma64_dd_upd(di, di->rxd64, pa, rxout1, &flags, pktlen);
		rxout1 = NEXTRXD(rxout1);		/* update rxout */

		/* Now program host descriptor */
		/* Store pointer to packet */
		di->linked_di->rxp[rxout2] = p;
		flags = 0;	/* Reset the flags */
		if (rxout2 == (di->linked_di->nrxd - 1))
			flags = D64_CTRL1_EOT;

		/* Extract out host length */
		pktlen = PKTFRAGLEN(di->linked_di->osh, p, 1);

		/* Extract out host addresses */
		pa64.loaddr = PKTFRAGDATA_LO(di->linked_di->osh, p, 1);
		pa64.hiaddr = PKTFRAGDATA_HI(di->linked_di->osh, p, 1);
		/* load the descriptor */
		dma64_dd_upd_64_from_params(di->linked_di, di->linked_di->rxd64, pa64, rxout2,
			&flags, pktlen);
		rxout2 = NEXTRXD(rxout2);	/* update rxout */
	}

	di->rxout = rxout1;
	di->linked_di->rxout = rxout2;

	/* update the chip lastdscr pointer */
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		W_REG(di->osh, &di->d64rxregs->ptr, di->rcvptrbase + I2B(rxout1, dma64dd_t));
	} else if (DMA32_ENAB(di)) {
		W_REG(di->osh, &di->d32rxregs->ptr, I2B(rxout1, dma32dd_t));
	} else
		ASSERT(0);

	if (DMA64_ENAB(di->linked_di) && DMA64_MODE(di->linked_di)) {
		W_REG(di->linked_di->osh, &di->linked_di->d64rxregs->ptr,
			di->linked_di->rcvptrbase + I2B(rxout2, dma64dd_t));
	} else if (DMA32_ENAB(di->linked_di)) {
		W_REG(di->linked_di->osh, &di->linked_di->d32rxregs->ptr, I2B(rxout2, dma32dd_t));
	} else
		ASSERT(0);

	return !ring_empty;
}
#endif /* D11_SPLIT_RX_FD */

#ifdef WLCXO_CTRL
/* used by WLCXO_CTRL to retrieve WLCXO_DATA's DMA HW configuration */
void
cxo_ctrl_dma_hw_params_get(dma_hw_params_t *dmahwp, hnddma_t *hnddma, int i)
{
	dma_info_t *di = DI_INFO(hnddma);
	dmaaddr_t pa; /* phys addr */

	if (!di)
		return;

	UNUSED_PARAMETER(pa);

	if (di->rxp) {
#ifndef WLCXO_SIM
		pa = DMA_MAP(di->osh, (void *)di->rxp, (di->nrxd * sizeof(void *)),
		             DMA_TX, NULL, NULL);
		dmahwp->chnl[i].rxp = (void **)pa;
#else
		dmahwp->chnl[i].rxp = di->rxp;
#endif
	} else {
		dmahwp->chnl[i].rxp = NULL;
	}
	if (di->rxd64) {
#ifndef WLCXO_SIM
		pa = DMA_MAP(di->osh, (void *)(uintptr)di->rxd64, (di->nrxd * sizeof(dma64dd_t)),
		             DMA_TX, NULL, NULL);
		dmahwp->chnl[i].rxd_64 = (dma64dd_t *)pa;
#else
		dmahwp->chnl[i].rxd_64 = di->rxd64;
#endif
	} else {
		dmahwp->chnl[i].rxd_64 = NULL;
	}
	if (di->txp) {
#ifndef WLCXO_SIM
		pa = DMA_MAP(di->osh, (void *)di->txp, (di->ntxd * sizeof(void *)),
		             DMA_TX, NULL, NULL);
		dmahwp->chnl[i].txp = (void **)pa;
#else
		dmahwp->chnl[i].txp = di->txp;
#endif
	} else {
		dmahwp->chnl[i].txp = NULL;
	}
	if (di->txd64) {
#ifndef WLCXO_SIM
		pa = DMA_MAP(di->osh, (void *)(uintptr)di->txd64, (di->ntxd * sizeof(dma64dd_t)),
		             DMA_TX, NULL, NULL);
		dmahwp->chnl[i].txd_64 = (dma64dd_t *)pa;
#else
		dmahwp->chnl[i].txd_64 = di->txd64;
#endif
	} else {
		dmahwp->chnl[i].txd_64 = NULL;
	}
	dmahwp->chnl[i].txregs_64 = di->d64txregs;
	dmahwp->chnl[i].rxregs_64 = di->d64rxregs;
#ifndef WLCXO_SIM
	DMA_MAP(di->osh, (void *)di, sizeof(dma_info_t), DMA_TX, NULL, NULL);
#endif
}
#endif /* WLCXO_CTRL */
