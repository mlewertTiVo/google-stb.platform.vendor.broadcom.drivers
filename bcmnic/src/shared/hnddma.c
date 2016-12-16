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
 * $Id: hnddma.c 654572 2016-08-15 08:06:23Z $
 */

/**
 * @file
 * @brief
 * Source file for HNDDMA module. This file contains the functionality to initialize and run the
 * DMA engines in e.g. D11 and PCIe cores.
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

#include <sbhnddma.h>
#include <hnddma.h>
#include "hnddma_priv.h"



/* default dma message level (if input msg_level pointer is null in dma_attach()) */
static uint dma_msg_level =
#ifdef BCMDBG_ERR
	1;
#else
	0;
#endif /* BCMDBG_ERR */

/* Common prototypes */
static bool _dma_isaddrext(dma_info_t *di);
static bool _dma_descriptor_align(dma_info_t *di);
static bool _dma_alloc(dma_info_t *di, uint direction);
static void _dma_detach(dma_info_t *di);
static void _dma_ddtable_init(dma_info_t *di, uint direction, dmaaddr_t pa);
static void _dma_rxinit(dma_info_t *di);
static void _dma_rxenable(dma_info_t *di);
static void _dma_rx_param_get(dma_info_t *di, uint16 *rxoffset, uint16 *rxbufsize);

static void _dma_txblock(dma_info_t *di);
static void _dma_txunblock(dma_info_t *di);
static uint _dma_txactive(dma_info_t *di);
static uint _dma_rxactive(dma_info_t *di);
static uint _dma_activerxbuf(dma_info_t *di);
static uint _dma_txpending(dma_info_t *di);
static uint _dma_txcommitted(dma_info_t *di);

static void *_dma_peeknexttxp(dma_info_t *di);
static int _dma_peekntxp(dma_info_t *di, int *len, void *txps[], txd_range_t range);
static void *_dma_peeknextrxp(dma_info_t *di);
static void _dma_counterreset(dma_info_t *di);
static void _dma_fifoloopbackenable(dma_info_t *di);
static uint _dma_ctrlflags(dma_info_t *di, uint mask, uint flags);
static uint8 dma_align_sizetobits(uint size);
static void *dma_ringalloc(osl_t *osh, uint32 boundary, uint size, uint16 *alignbits, uint* alloced,
	dmaaddr_t *descpa, osldma_t **dmah);
#ifdef BCMPKTPOOL
static int _dma_pktpool_set(dma_info_t *di, pktpool_t *pool);
#endif
static bool _dma_rxtx_error(dma_info_t *di, bool istx);
static void _dma_burstlen_set(dma_info_t *di, uint8 rxburstlen, uint8 txburstlen);
static uint _dma_avoidancecnt(dma_info_t *di);
static void _dma_param_set(dma_info_t *di, uint16 paramid, uint16 paramval);
static bool _dma_glom_enable(dma_info_t *di, uint32 val);
static void _dma_context(dma_info_t *di, setup_context_t fn, void *ctx);


/* Prototypes for 32-bit routines */
static bool dma32_alloc(dma_info_t *di, uint direction);
static bool dma32_txreset(dma_info_t *di);
static bool dma32_rxreset(dma_info_t *di);
static bool dma32_txsuspendedidle(dma_info_t *di);
static void dma32_txrotate(dma_info_t *di);
static void dma32_txinit(dma_info_t *di);
static bool dma32_txenabled(dma_info_t *di);
static void dma32_txsuspend(dma_info_t *di);
static void dma32_txresume(dma_info_t *di);
static bool dma32_txsuspended(dma_info_t *di);
static void dma32_txflush(dma_info_t *di);
static void dma32_txflush_clear(dma_info_t *di);
static bool dma32_txstopped(dma_info_t *di);
static bool dma32_rxstopped(dma_info_t *di);
static bool dma32_rxcheckidlestatus(dma_info_t *di);
#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void dma32_dumpring(dma_info_t *di, struct bcmstrbuf *b, dma32dd_t *ring, uint start,
	uint end, uint max_num);
static void dma32_dump(dma_info_t *di, struct bcmstrbuf *b, bool dumpring);
static void dma32_dumptx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring);
static void dma32_dumprx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */

static bool _dma32_addrext(osl_t *osh, dma32regs_t *dma32regs);

/* Prototypes for 64-bit routines */
static bool dma64_alloc(dma_info_t *di, uint direction);
static bool dma64_txreset(dma_info_t *di);
static bool dma64_rxreset(dma_info_t *di);
static bool dma64_txsuspendedidle(dma_info_t *di);
static int  dma64_txunframed(dma_info_t *di, void *p0, uint len, bool commit);
static void *dma64_getpos(dma_info_t *di, bool direction);
static void dma64_txrotate(dma_info_t *di);

static void dma64_txinit(dma_info_t *di);
static bool dma64_txenabled(dma_info_t *di);
static void dma64_txsuspend(dma_info_t *di);
static void dma64_txresume(dma_info_t *di);
static bool dma64_txsuspended(dma_info_t *di);
static void dma64_txflush(dma_info_t *di);
static void dma64_txflush_clear(dma_info_t *di);
static bool dma64_txstopped(dma_info_t *di);
static bool dma64_rxstopped(dma_info_t *di);
static bool _dma64_addrext(osl_t *osh, dma64regs_t *dma64regs);

static void dma_param_set_nrxpost(dma_info_t *di, uint16 paramval);
static bool dma64_rxcheckidlestatus(dma_info_t *di);

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void dma64_dumpring(dma_info_t *di, struct bcmstrbuf *b, dma64dd_t *ring, uint start,
	uint end, uint max_num);
static void dma64_dump(dma_info_t *di, struct bcmstrbuf *b, bool dumpring);
static void dma64_dumptx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring);
static void dma64_dumprx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring);
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */

/* NULL entries will be overwritten during ATTACH phase in hnddma_rx.c/hnddma_tx.c */
const di_fcn_t dma64proc = {
	(di_detach_t)_dma_detach,
	(di_txinit_t)dma64_txinit,
	(di_txreset_t)dma64_txreset,
	(di_txenabled_t)dma64_txenabled,
	(di_txsuspend_t)dma64_txsuspend,
	(di_txresume_t)dma64_txresume,
	(di_txsuspended_t)dma64_txsuspended,
	(di_txsuspendedidle_t)dma64_txsuspendedidle,
	(di_txflush_t)dma64_txflush,
	(di_txflush_clear_t)dma64_txflush_clear,
	(di_txfast_t)dma64_txfast,
	(di_txcommit_t)dma64_txcommit,
	(di_txunframed_t)dma64_txunframed,
	(di_getpos_t)dma64_getpos,
	(di_txstopped_t)dma64_txstopped,
	(di_txreclaim_t)dma64_txreclaim,
	(di_getnexttxp_t)dma64_getnexttxp,
	(di_peeknexttxp_t)_dma_peeknexttxp,
	(di_peekntxp_t)_dma_peekntxp,
	(di_txblock_t)_dma_txblock,
	(di_txunblock_t)_dma_txunblock,
	(di_txactive_t)_dma_txactive,
	(di_txrotate_t)dma64_txrotate,

	(di_rxinit_t)_dma_rxinit,
	(di_rxreset_t)dma64_rxreset,
	(di_rxidle_t)dma64_rxidle,
	(di_rxstopped_t)dma64_rxstopped,
	(di_rxenable_t)_dma_rxenable,
	(di_rxenabled_t)dma64_rxenabled,
	(di_rx_t)_dma_rx,
	(di_rxfill_t)_dma_rxfill,
	(di_rxreclaim_t)_dma_rxreclaim,
	(di_getnextrxp_t)_dma_getnextrxp,
	(di_peeknextrxp_t)_dma_peeknextrxp,
	(di_rxparam_get_t)_dma_rx_param_get,

	(di_fifoloopbackenable_t)_dma_fifoloopbackenable,
	(di_getvar_t)_dma_getvar,
	(di_counterreset_t)_dma_counterreset,
	(di_ctrlflags_t)_dma_ctrlflags,

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	(di_dump_t)dma64_dump,
	(di_dumptx_t)dma64_dumptx,
	(di_dumprx_t)dma64_dumprx,
#else
	NULL,
	NULL,
	NULL,
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */
	(di_rxactive_t)_dma_rxactive,
	(di_txpending_t)_dma_txpending,
	(di_txcommitted_t)_dma_txcommitted,
#ifdef BCMPKTPOOL
	(di_pktpool_set_t)_dma_pktpool_set,
#else
	NULL,
#endif
	(di_rxtxerror_t)_dma_rxtx_error,
	(di_burstlen_set_t)_dma_burstlen_set,
	(di_avoidancecnt_t)_dma_avoidancecnt,
	(di_param_set_t)_dma_param_set,
	(dma_glom_enable_t)_dma_glom_enable,
	(dma_active_rxbuf_t)_dma_activerxbuf,
	(di_rxidlestatus_t)dma64_rxcheckidlestatus,
	(di_context_t)_dma_context,
	54
};

static const di_fcn_t dma32proc = {
	(di_detach_t)_dma_detach,
	(di_txinit_t)dma32_txinit,
	(di_txreset_t)dma32_txreset,
	(di_txenabled_t)dma32_txenabled,
	(di_txsuspend_t)dma32_txsuspend,
	(di_txresume_t)dma32_txresume,
	(di_txsuspended_t)dma32_txsuspended,
	(di_txsuspendedidle_t)dma32_txsuspendedidle,
	(di_txflush_t)dma32_txflush,
	(di_txflush_clear_t)dma32_txflush_clear,
	(di_txfast_t)dma32_txfast,
	(di_txcommit_t)dma32_txcommit,
	NULL,
	NULL,
	(di_txstopped_t)dma32_txstopped,
	(di_txreclaim_t)dma32_txreclaim,
	(di_getnexttxp_t)dma32_getnexttxp,
	(di_peeknexttxp_t)_dma_peeknexttxp,
	(di_peekntxp_t)_dma_peekntxp,
	(di_txblock_t)_dma_txblock,
	(di_txunblock_t)_dma_txunblock,
	(di_txactive_t)_dma_txactive,
	(di_txrotate_t)dma32_txrotate,

	(di_rxinit_t)_dma_rxinit,
	(di_rxreset_t)dma32_rxreset,
	(di_rxidle_t)dma32_rxidle,
	(di_rxstopped_t)dma32_rxstopped,
	(di_rxenable_t)_dma_rxenable,
	(di_rxenabled_t)dma32_rxenabled,
	(di_rx_t)_dma_rx,
	(di_rxfill_t)_dma_rxfill,
	(di_rxreclaim_t)_dma_rxreclaim,
	(di_getnextrxp_t)_dma_getnextrxp,
	(di_peeknextrxp_t)_dma_peeknextrxp,
	(di_rxparam_get_t)_dma_rx_param_get,

	(di_fifoloopbackenable_t)_dma_fifoloopbackenable,
	(di_getvar_t)_dma_getvar,
	(di_counterreset_t)_dma_counterreset,
	(di_ctrlflags_t)_dma_ctrlflags,

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
	(di_dump_t)dma32_dump,
	(di_dumptx_t)dma32_dumptx,
	(di_dumprx_t)dma32_dumprx,
#else
	NULL,
	NULL,
	NULL,
#endif /* defined(BCMDBG) || defined(BCMDBG_DUMP) */
	(di_rxactive_t)_dma_rxactive,
	(di_txpending_t)_dma_txpending,
	(di_txcommitted_t)_dma_txcommitted,
#ifdef BCMPKTPOOL
	(di_pktpool_set_t)_dma_pktpool_set,
#else
	NULL,
#endif
	(di_rxtxerror_t)_dma_rxtx_error,
	(di_burstlen_set_t)_dma_burstlen_set,
	(di_avoidancecnt_t)_dma_avoidancecnt,
	(di_param_set_t)_dma_param_set,
	NULL,
	NULL,
	(di_rxidlestatus_t)dma32_rxcheckidlestatus,
	(di_context_t)_dma_context,
	54
};
/*
 * This function needs to be called during initialization before calling dma_attach_ext.
 */
dma_common_t *
BCMATTACHFN_DMA_ATTACH(dma_common_attach)(osl_t *osh, volatile uint32 *indqsel,
	volatile uint32 *suspreq, volatile uint32 *flushreq)
{
	dma_common_t *dmac;

	/* allocate private info structure */
	if ((dmac = MALLOCZ(osh, sizeof(*dmac))) == NULL) {
		DMA_NONE(("%s: out of memory, malloced %d bytes\n",
			__FUNCTION__, MALLOCED(osh)));
		return (NULL);
	}

	dmac->osh = osh;

	dmac->indqsel = indqsel;
	dmac->suspreq = suspreq;
	dmac->flushreq = flushreq;

	return dmac;
}

void
dma_common_detach(dma_common_t *dmacommon)
{
	/* free our private info structure */
	MFREE(dmacommon->osh, (void *)dmacommon, sizeof(*dmacommon));
}

#ifdef BCM_DMA_INDIRECT
/* Function to configure the q_index into indqsel register
 * to indicate to the DMA engine which queue is being selected
 * for updates or to read back status.
 * Also updates global dma common state
 * Caller should verify if the di is configured for indirect access.
 * If force is TRUE, the configuration will be done even though
 * global DMA state indicates it is the same as last_qsel
 */
void
dma_set_indqsel(hnddma_t *di, bool override)
{
	dma_info_t *dmainfo = DI_INFO(di);
	dma_common_t *dmacommon = dmainfo->dmacommon;
	ASSERT(dmainfo->indirect_dma == TRUE);

	if ((dmacommon->last_qsel != dmainfo->q_index) || override) {
		/* configure the indqsel register */
		W_REG(dmainfo->osh, dmacommon->indqsel, dmainfo->q_index);

		/* also update the global state dmac */
		dmacommon->last_qsel = dmainfo->q_index;
	}
	return;
}

#endif /* BCM_DMA_INDIRECT */

/**
 * This is new dma_attach API to provide the option of using an Indirect DMA interface.
 * Initializes DMA data structures and hardware. Allocates rx and tx descriptor rings. Does not
 * allocate the buffers that the descriptors are going to point at.
 *
 * @param nrxd    Number of receive descriptors, must be a power of 2.
 * @param ntxd    Number of transmit descriptors, must be a power of 2.
 * @param nrxpost Number of empty receive buffers to keep posted to rx ring, must be <= nrxd - 1
 */
hnddma_t *
BCMATTACHFN_DMA_ATTACH(dma_attach_ext)(dma_common_t *dmac, osl_t *osh, const char *name,
	si_t *sih, volatile void *dmaregstx, volatile void *dmaregsrx, uint32 flags,
	uint8 qnum, uint ntxd, uint nrxd, uint rxbufsize, int rxextheadroom, uint nrxpost,
	uint rxoffset, uint *msg_level)
{
	dma_info_t *di;
	uint size;
	uint32 mask, reg_val;

	ASSERT(nrxpost <= nrxd - 1); /* because XXD macro counts up to nrxd - 1, not nrxd */

#ifdef EVENT_LOG_COMPILE
	event_log_tag_start(EVENT_LOG_TAG_DMA_ERROR, EVENT_LOG_SET_ERROR, EVENT_LOG_TAG_FLAG_LOG);
#endif
	/* allocate private info structure */
	if ((di = MALLOC(osh, sizeof (dma_info_t))) == NULL) {
#if defined(BCMDBG) || defined(WLC_BCMDMA_ERRORS)
		DMA_ERROR(("%s: out of memory, malloced %d bytes\n", __FUNCTION__, MALLOCED(osh)));
		OSL_SYS_HALT();
#endif
		return (NULL);
	}

	bzero(di, sizeof(dma_info_t));

#ifdef BCM_DMA_INDIRECT
	if ((flags & BCM_DMA_IND_INTF_FLAG) && (dmac == NULL)) {
		DMA_ERROR(("%s: Inconsistent flags and dmac params \n", __FUNCTION__));
		ASSERT(0);
		goto fail;
	}
#endif /* ifdef BCM_DMA_INDIRECT */

	/* save dma_common */
	di->dmacommon = dmac;

#ifdef BCM_DMA_INDIRECT
	/* record indirect DMA interface  params */
	di->indirect_dma = (flags & BCM_DMA_IND_INTF_FLAG) ? TRUE : FALSE;
	di->q_index = qnum;

#endif /* BCM_DMA_INDIRECT */

	di->msg_level = msg_level ? msg_level : &dma_msg_level;

	/* old chips w/o sb is no longer supported */
	ASSERT(sih != NULL);

	if (DMA64_ENAB(di))
		di->dma64 = ((si_core_sflags(sih, 0, 0) & SISF_DMA64) == SISF_DMA64);
	else
		di->dma64 = 0;

	/* check arguments */
	ASSERT(ISPOWEROF2(ntxd));
	ASSERT(ISPOWEROF2(nrxd));

	if (nrxd == 0)
		ASSERT(dmaregsrx == NULL);
	if (ntxd == 0)
		ASSERT(dmaregstx == NULL);

	/* init dma reg pointer */
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		di->d64txregs = (dma64regs_t *)dmaregstx;
		di->d64rxregs = (dma64regs_t *)dmaregsrx;
		di->hnddma.di_fn = (const di_fcn_t *)&dma64proc;
	} else if (DMA32_ENAB(di)) {
		ASSERT(ntxd <= D32MAXDD);
		ASSERT(nrxd <= D32MAXDD);
		di->d32txregs = (dma32regs_t *)dmaregstx;
		di->d32rxregs = (dma32regs_t *)dmaregsrx;
		di->hnddma.di_fn = (const di_fcn_t *)&dma32proc;
	} else {
		DMA_ERROR(("%s: driver doesn't support 32-bit DMA\n", __FUNCTION__));
		ASSERT(0);
		goto fail;
	}

	/* Default flags (which can be changed by the driver calling dma_ctrlflags
	 * before enable): For backwards compatibility both Rx Overflow Continue
	 * and Parity are DISABLED.
	 * supports it.
	 */
	di->hnddma.di_fn->ctrlflags(&di->hnddma, DMA_CTRL_ROC | DMA_CTRL_PEN, 0);

	/* check flags for descriptor only DMA */
	if (flags & BCM_DMA_DESC_ONLY_FLAG) {
		di->hnddma.dmactrlflags |= DMA_CTRL_DESC_ONLY_FLAG;
	}

	/* Modern-MACs (d11 corerev 64 and higher) supporting higher MOR need CS set. */
	if (flags & BCM_DMA_CHAN_SWITCH_EN) {
		di->hnddma.dmactrlflags |= DMA_CTRL_CS;
	}

	DMA_TRACE(("%s: %s: %s osh %p flags 0x%x ntxd %d nrxd %d rxbufsize %d "
		   "rxextheadroom %d nrxpost %d rxoffset %d dmaregstx %p dmaregsrx %p\n",
		   name, __FUNCTION__, (DMA64_MODE(di) ? "DMA64" : "DMA32"),
		   OSL_OBFUSCATE_BUF(osh), di->hnddma.dmactrlflags, ntxd, nrxd,
		   rxbufsize, rxextheadroom, nrxpost, rxoffset, OSL_OBFUSCATE_BUF(dmaregstx),
		   OSL_OBFUSCATE_BUF(dmaregsrx)));
#ifdef BCM_DMA_INDIRECT
	DMA_TRACE(("%s: %s: indirect DMA %s, q_index %d \n",
		name, __FUNCTION__, (di->indirect_dma ? "TRUE" : "FALSE"), di->q_index));
#endif
	/* make a private copy of our callers name */
	strncpy(di->name, name, MAXNAMEL);
	di->name[MAXNAMEL-1] = '\0';

	di->osh = osh;
	di->sih = sih;

	/* save tunables */
	di->ntxd = (uint16)ntxd;
	di->nrxd = (uint16)nrxd;

	/* the actual dma size doesn't include the extra headroom */
	di->rxextrahdrroom = (rxextheadroom == -1) ? BCMEXTRAHDROOM : rxextheadroom;
	if (rxbufsize > BCMEXTRAHDROOM)
		di->rxbufsize = (uint16)(rxbufsize - di->rxextrahdrroom);
	else
		di->rxbufsize = (uint16)rxbufsize;

	di->nrxpost = (uint16)nrxpost;
	di->rxoffset = (uint8)rxoffset;

	/* if this DMA channel is using indirect access, then configure the IndQSel register
	 * for this DMA channel
	 */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, TRUE);
	}

	/* Get the default values (POR) of the burstlen. This can be overridden by the modules
	 * if this has to be different. Otherwise this value will be used to program the control
	 * register after the reset or during the init.
	 */
	/* For 4345 PCIE rev 5, writing all 1's to control is broken,
	 * you will read all 1's back
	 */

	if (dmaregsrx) {
		if (DMA64_ENAB(di) && DMA64_MODE(di)) {

			/* first disable the dma if not already done */
			reg_val = R_REG(di->osh, &di->d64rxregs->control);
			if (reg_val & 1) {
				reg_val &= ~1;
				W_REG(di->osh, &di->d64rxregs->control, reg_val);
				W_REG(di->osh, &di->d64rxregs->control, reg_val);
			}
			/* detect the dma descriptor address mask,
			 * should be 0x1fff before 4360B0, 0xffff start from 4360B0
			 */
			W_REG(di->osh, &di->d64rxregs->addrlow, 0xffffffff);
			/* For 4345 PCIE rev 5/8, need one more write to make it work */
			if ((si_coreid(di->sih) == PCIE2_CORE_ID) &&
				((si_corerev(di->sih) == 5) || (si_corerev(di->sih) == 8))) {
				W_REG(di->osh, &di->d64rxregs->addrlow, 0xffffffff);
			}
			mask = R_REG(di->osh, &di->d64rxregs->addrlow);

			if (mask & 0xfff)
				mask = R_REG(di->osh, &di->d64rxregs->ptr) | 0xf;
			else
				mask = 0x1fff;

#ifdef BCMM2MDEV_ENABLED
			if (mask == 0xf)
				mask = 0xffff;
#endif

			DMA_TRACE(("%s: dma_rx_mask: %08x\n", di->name, mask));
			di->d64_rs0_cd_mask = mask;
			di->d64_rs1_ad_mask = mask;

			if (mask == 0x1fff)
				ASSERT(nrxd <= D64MAXDD);
			else
				ASSERT(nrxd <= D64MAXDD_LARGE);

			di->rxburstlen = (R_REG(di->osh,
				&di->d64rxregs->control) & D64_RC_BL_MASK) >> D64_RC_BL_SHIFT;
			di->rxwaitforcomplt = (R_REG(di->osh,
				&di->d64rxregs->control) & D64_RC_WAITCMP_MASK) >>
				D64_RC_WAITCMP_SHIFT;
			di->rxprefetchctl = (R_REG(di->osh,
				&di->d64rxregs->control) & D64_RC_PC_MASK) >> D64_RC_PC_SHIFT;
			di->rxprefetchthresh = (R_REG(di->osh,
				&di->d64rxregs->control) & D64_RC_PT_MASK) >> D64_RC_PT_SHIFT;
		} else if (DMA32_ENAB(di)) {
			di->rxburstlen = (R_REG(di->osh,
				&di->d32rxregs->control) & RC_BL_MASK) >> RC_BL_SHIFT;
			di->rxwaitforcomplt = (R_REG(di->osh,
				&di->d32rxregs->control) & RC_WAITCMP_MASK) >>
				RC_WAITCMP_SHIFT;
			di->rxprefetchctl = (R_REG(di->osh,
				&di->d32rxregs->control) & RC_PC_MASK) >> RC_PC_SHIFT;
			di->rxprefetchthresh = (R_REG(di->osh,
				&di->d32rxregs->control) & RC_PT_MASK) >> RC_PT_SHIFT;
		}
	}
	if (dmaregstx) {
		if (DMA64_ENAB(di) && DMA64_MODE(di)) {

			/* first disable the dma if not already done */
			reg_val = R_REG(di->osh, &di->d64txregs->control);
			if (reg_val & 1) {
				reg_val &= ~1;
				W_REG(di->osh, &di->d64txregs->control, reg_val);
				W_REG(di->osh, &di->d64txregs->control, reg_val);
			}

			/* detect the dma descriptor address mask,
			 * should be 0x1fff before 4360B0, 0xffff start from 4360B0
			 */
			W_REG(di->osh, &di->d64txregs->addrlow, 0xffffffff);
			/* For 4345 PCIE rev 5/8, need one more write to make it work */
			if ((si_coreid(di->sih) == PCIE2_CORE_ID) &&
				((si_corerev(di->sih) == 5) || (si_corerev(di->sih) == 8))) {
				W_REG(di->osh, &di->d64txregs->addrlow, 0xffffffff);
			}
			mask = R_REG(di->osh, &di->d64txregs->addrlow);

			if (mask & 0xfff)
				mask = R_REG(di->osh, &di->d64txregs->ptr) | 0xf;
			else
				mask = 0x1fff;

#ifdef BCMM2MDEV_ENABLED
			if (mask == 0xf)
				mask = 0xffff;
#endif

			DMA_TRACE(("%s: dma_tx_mask: %08x\n", di->name, mask));
			di->d64_xs0_cd_mask = mask;
			di->d64_xs1_ad_mask = mask;

			if (mask == 0x1fff)
				ASSERT(ntxd <= D64MAXDD);
			else
				ASSERT(ntxd <= D64MAXDD_LARGE);

			di->txburstlen = (R_REG(di->osh,
				&di->d64txregs->control) & D64_XC_BL_MASK) >> D64_XC_BL_SHIFT;
			di->txmultioutstdrd = (R_REG(di->osh,
				&di->d64txregs->control) & D64_XC_MR_MASK) >> D64_XC_MR_SHIFT;
			di->txprefetchctl = (R_REG(di->osh,
				&di->d64txregs->control) & D64_XC_PC_MASK) >> D64_XC_PC_SHIFT;
			di->txprefetchthresh = (R_REG(di->osh,
				&di->d64txregs->control) & D64_XC_PT_MASK) >> D64_XC_PT_SHIFT;
			di->txchanswitch = (R_REG(di->osh,
				&di->d64txregs->control) & D64_XC_CS_MASK) >> D64_XC_CS_SHIFT;
		} else if (DMA32_ENAB(di)) {
			di->txburstlen = (R_REG(di->osh,
				&di->d32txregs->control) & XC_BL_MASK) >> XC_BL_SHIFT;
			di->txmultioutstdrd = (R_REG(di->osh,
				&di->d32txregs->control) & XC_MR_MASK) >> XC_MR_SHIFT;
			di->txprefetchctl = (R_REG(di->osh,
				&di->d32txregs->control) & XC_PC_MASK) >> XC_PC_SHIFT;
			di->txprefetchthresh = (R_REG(di->osh,
				&di->d32txregs->control) & XC_PT_MASK) >> XC_PT_SHIFT;
		}
	}

	/*
	 * figure out the DMA physical address offset for dd and data
	 *     PCI/PCIE: they map silicon backplace address to zero based memory, need offset
	 *     Other bus: use zero
	 *     SI_BUS BIGENDIAN kludge: use sdram swapped region for data buffer, not descriptor
	 */
	di->ddoffsetlow = 0;
	di->dataoffsetlow = 0;
	/* for pci bus, add offset */
	if (sih->bustype == PCI_BUS) {
		if ((BUSCORETYPE(sih->buscoretype) == PCIE_CORE_ID ||
		     BUSCORETYPE(sih->buscoretype) == PCIE2_CORE_ID) &&
		    DMA64_MODE(di)) {
			/* pcie with DMA64 */
			di->ddoffsetlow = 0;
			di->ddoffsethigh = SI_PCIE_DMA_H32;
		} else {
			/* pci(DMA32/DMA64) or pcie with DMA32 */
			di->ddoffsetlow = SI_PCI_DMA;
			di->ddoffsethigh = 0;
		}
		di->dataoffsetlow =  di->ddoffsetlow;
		di->dataoffsethigh =  di->ddoffsethigh;
	}

#if defined(__mips__) && defined(IL_BIGENDIAN)
	/* use sdram swapped region for data buffers but not dma descriptors.
	 * this assumes that we are running on a 47xx mips with a swap window.
	 * But __mips__ is too general, there should be one si_ishndmips() checking
	 * for OUR mips
	 */
	di->dataoffsetlow = di->dataoffsetlow + SI_SDRAM_SWAPPED;
#endif /* defined(__mips__) && defined(IL_BIGENDIAN) */
	/* WAR64450 : DMACtl.Addr ext fields are not supported in SDIOD core. */
	if ((si_coreid(sih) == SDIOD_CORE_ID) && ((si_corerev(sih) > 0) && (si_corerev(sih) <= 2)))
		di->addrext = 0;
	else if ((si_coreid(sih) == I2S_CORE_ID) &&
	         ((si_corerev(sih) == 0) || (si_corerev(sih) == 1)))
		di->addrext = 0;
	else
		di->addrext = _dma_isaddrext(di);

	/* does the descriptors need to be aligned and if yes, on 4K/8K or not */
	di->aligndesc_4k = _dma_descriptor_align(di);
	if (di->aligndesc_4k) {
		if (DMA64_MODE(di)) {
			di->dmadesc_align = D64RINGALIGN_BITS;
			if ((ntxd < D64MAXDD / 2) && (nrxd < D64MAXDD / 2)) {
				/* for smaller dd table, HW relax the alignment requirement */
				di->dmadesc_align = D64RINGALIGN_BITS  - 1;
			}
		} else
			di->dmadesc_align = D32RINGALIGN_BITS;
	} else {
		/* The start address of descriptor table should be algined to cache line size,
		 * or other structure may share a cache line with it, which can lead to memory
		 * overlapping due to cache write-back operation. In the case of MIPS 74k, the
		 * cache line size is 32 bytes.
		 */
#ifdef __mips__
		di->dmadesc_align = 5;	/* 32 byte alignment */
#else
		di->dmadesc_align = 4;	/* 16 byte alignment */

		/* Aqm txd table alignment 4KB, limitation comes from the bug of CD,
		 * see DMA_CTRL_DESC_CD_WAR and it will be fixed in C0
		 */
		if ((di->hnddma.dmactrlflags & (DMA_CTRL_DESC_ONLY_FLAG | DMA_CTRL_DESC_CD_WAR)) ==
			(DMA_CTRL_DESC_ONLY_FLAG | DMA_CTRL_DESC_CD_WAR)) {
			di->dmadesc_align = 12;	/* 4K byte alignment */
		}
#endif
	}

	DMA_NONE(("DMA descriptor align_needed %d, align %d\n",
		di->aligndesc_4k, di->dmadesc_align));

	/* allocate tx packet pointer vector
	 * Descriptor only DMAs do not need packet pointer array
	 */
	if ((!(di->hnddma.dmactrlflags & DMA_CTRL_DESC_ONLY_FLAG)) && (ntxd)) {
		size = ntxd * sizeof(void *);
		if ((di->txp = MALLOC(osh, size)) == NULL) {
			DMA_ERROR(("%s: %s: out of tx memory, malloced %d bytes\n",
				di->name, __FUNCTION__, MALLOCED(osh)));
			goto fail;
		}
		bzero(di->txp, size);
	}

	/* allocate rx packet pointer vector */
	if (nrxd) {
		size = nrxd * sizeof(void *);
		if ((di->rxp = MALLOC(osh, size)) == NULL) {
			DMA_ERROR(("%s: %s: out of rx memory, malloced %d bytes\n",
			           di->name, __FUNCTION__, MALLOCED(osh)));
			goto fail;
		}
		bzero(di->rxp, size);
	}

	/* allocate transmit descriptor ring, only need ntxd descriptors but it must be aligned */
	if (ntxd) {
		if (!_dma_alloc(di, DMA_TX)) /* this does not allocate buffers */
			goto fail;
	}

	/* allocate receive descriptor ring, only need nrxd descriptors but it must be aligned */
	if (nrxd) {
		if (!_dma_alloc(di, DMA_RX)) /* this does not allocate buffers */
			goto fail;
	}

	if ((di->ddoffsetlow != 0) && !di->addrext) {
		if (PHYSADDRLO(di->txdpa) > SI_PCI_DMA_SZ) {
			DMA_ERROR(("%s: %s: txdpa 0x%x: addrext not supported\n",
			           di->name, __FUNCTION__, (uint32)PHYSADDRLO(di->txdpa)));
			goto fail;
		}
		if (PHYSADDRLO(di->rxdpa) > SI_PCI_DMA_SZ) {
			DMA_ERROR(("%s: %s: rxdpa 0x%x: addrext not supported\n",
			           di->name, __FUNCTION__, (uint32)PHYSADDRLO(di->rxdpa)));
			goto fail;
		}
	}

	DMA_TRACE(("ddoffsetlow 0x%x ddoffsethigh 0x%x dataoffsetlow 0x%x dataoffsethigh "
	           "0x%x addrext %d\n", di->ddoffsetlow, di->ddoffsethigh, di->dataoffsetlow,
	           di->dataoffsethigh, di->addrext));

	if (!di->aligndesc_4k) {
		if (DMA64_ENAB(di) && DMA64_MODE(di)) {
			di->xmtptrbase = PHYSADDRLO(di->txdpa);
			di->rcvptrbase = PHYSADDRLO(di->rxdpa);
		}
	}

	/* allocate DMA mapping vectors */
	if (DMASGLIST_ENAB) {
		if (ntxd) {
			size = ntxd * sizeof(hnddma_seg_map_t);
			if ((di->txp_dmah = (hnddma_seg_map_t *)MALLOC(osh, size)) == NULL)
				goto fail;
			bzero(di->txp_dmah, size);
		}

		if (nrxd) {
			size = nrxd * sizeof(hnddma_seg_map_t);
			if ((di->rxp_dmah = (hnddma_seg_map_t *)MALLOC(osh, size)) == NULL)
				goto fail;
			bzero(di->rxp_dmah, size);
		}
	}
#ifdef BCM_DMAPAD
	#if defined(BCMSDIODEV_ENABLED)
		di->dmapad_required = TRUE;
	#else
		di->dmapad_required = FALSE;
	#endif /* BCMSDIODEV_ENABLED */
#else
	di->dmapad_required = FALSE;
#endif /* BCM_DMAPAD */

#ifdef BCM_DMA_TRANSCOHERENT
	di->trans_coherent = 1;
#else
	di->trans_coherent = 0;
#endif /* BCM_DMA_TRANSCOHERENT */

	return ((hnddma_t *)di);

fail:
#ifdef WLC_BCMDMA_ERRORS
	DMA_ERROR(("%s: %s: DMA attach failed \n", di->name, __FUNCTION__));
	OSL_SYS_HALT();
#endif
	_dma_detach(di);
	return (NULL);
}

/* This is the legacy dma_attach() API. The interface is kept as-is for
 * legacy code ("et" driver) that does not need the dma_common_t support
 * interface
 */
hnddma_t *
BCMATTACHFN_DMA_ATTACH(dma_attach)(osl_t *osh, const char *name, si_t *sih,
	volatile void *dmaregstx, volatile void *dmaregsrx,
	uint ntxd, uint nrxd, uint rxbufsize, int rxextheadroom, uint nrxpost,
	uint rxoffset, uint *msg_level)
{
	return (hnddma_t *)(dma_attach_ext(NULL, osh, name, sih, dmaregstx, dmaregsrx,
		0, 0, ntxd, nrxd, rxbufsize, rxextheadroom, nrxpost, rxoffset, msg_level));
} /* dma_attach */

static bool
_dma32_addrext(osl_t *osh, dma32regs_t *dma32regs)
{
	uint32 w;

	OR_REG(osh, &dma32regs->control, XC_AE);
	w = R_REG(osh, &dma32regs->control);
	AND_REG(osh, &dma32regs->control, ~XC_AE);
	return ((w & XC_AE) == XC_AE);
}

/** Allocates one rx or tx descriptor ring. Does not allocate buffers. */
static bool
BCMATTACHFN_DMA_ATTACH(_dma_alloc)(dma_info_t *di, uint direction)
{
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		return dma64_alloc(di, direction);
	} else if (DMA32_ENAB(di)) {
		return dma32_alloc(di, direction);
	} else
		ASSERT(0);
}

/** !! may be called with core in reset */
static void
_dma_detach(dma_info_t *di)
{

	DMA_TRACE(("%s: dma_detach\n", di->name));

	/* shouldn't be here if descriptors are unreclaimed */
	ASSERT(di->txin == di->txout);
	ASSERT(di->rxin == di->rxout);

	/* free dma descriptor rings */
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		if (di->txd64)
			DMA_FREE_CONSISTENT(di->osh, ((int8 *)(uintptr)di->txd64 - di->txdalign),
			                    di->txdalloc, (di->txdpaorig), di->tx_dmah);
		if (di->rxd64)
			DMA_FREE_CONSISTENT(di->osh, ((int8 *)(uintptr)di->rxd64 - di->rxdalign),
			                    di->rxdalloc, (di->rxdpaorig), di->rx_dmah);
	} else if (DMA32_ENAB(di)) {
		if (di->txd32)
			DMA_FREE_CONSISTENT(di->osh, ((int8 *)(uintptr)di->txd32 - di->txdalign),
			                    di->txdalloc, (di->txdpaorig), di->tx_dmah);
		if (di->rxd32)
			DMA_FREE_CONSISTENT(di->osh, ((int8 *)(uintptr)di->rxd32 - di->rxdalign),
			                    di->rxdalloc, (di->rxdpaorig), di->rx_dmah);
	} else
		ASSERT(0);

	/* free packet pointer vectors */
	if (di->txp)
		MFREE(di->osh, (void *)di->txp, (di->ntxd * sizeof(void *)));
	if (di->rxp)
		MFREE(di->osh, (void *)di->rxp, (di->nrxd * sizeof(void *)));

	/* free tx packet DMA handles */
	if (di->txp_dmah)
		MFREE(di->osh, (void *)di->txp_dmah, di->ntxd * sizeof(hnddma_seg_map_t));

	/* free rx packet DMA handles */
	if (di->rxp_dmah)
		MFREE(di->osh, (void *)di->rxp_dmah, di->nrxd * sizeof(hnddma_seg_map_t));

	/* free our private info structure */
	MFREE(di->osh, (void *)di, sizeof(dma_info_t));

}

static bool
BCMATTACHFN_DMA_ATTACH(_dma_descriptor_align)(dma_info_t *di)
{
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		uint32 addrl;

		/* if this DMA channel is using indirect access, then configure the IndQSel register
		 * for this DMA channel
		 */
		if (DMA_INDIRECT(di)) {
			dma_set_indqsel((hnddma_t *)di, FALSE);
		}

		/* Check to see if the descriptors need to be aligned on 4K/8K or not */
		if (di->d64txregs != NULL) {
			W_REG(di->osh, &di->d64txregs->addrlow, 0xff0);
			addrl = R_REG(di->osh, &di->d64txregs->addrlow);
			if (addrl != 0)
				return FALSE;
		} else if (di->d64rxregs != NULL) {
			W_REG(di->osh, &di->d64rxregs->addrlow, 0xff0);
			addrl = R_REG(di->osh, &di->d64rxregs->addrlow);
			if (addrl != 0)
				return FALSE;
		}
	}
	return TRUE;
}

/** return TRUE if this dma engine supports DmaExtendedAddrChanges, otherwise FALSE */
static bool
BCMATTACHFN_DMA_ATTACH(_dma_isaddrext)(dma_info_t *di)
{
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		/* DMA64 supports full 32- or 64-bit operation. AE is always valid */

		/* not all tx or rx channel are available */
		if (di->d64txregs != NULL) {
			/* if using indirect DMA access, then configure IndQSel */
			if (DMA_INDIRECT(di)) {
				dma_set_indqsel((hnddma_t *)di, FALSE);
			}
			if (!_dma64_addrext(di->osh, di->d64txregs)) {
				DMA_ERROR(("%s: _dma_isaddrext: DMA64 tx doesn't have AE set\n",
					di->name));
				ASSERT(0);
			}
			return TRUE;
		} else if (di->d64rxregs != NULL) {
			if (!_dma64_addrext(di->osh, di->d64rxregs)) {
				DMA_ERROR(("%s: _dma_isaddrext: DMA64 rx doesn't have AE set\n",
					di->name));
				ASSERT(0);
			}
			return TRUE;
		}
		return FALSE;
	} else if (DMA32_ENAB(di)) {
		if (di->d32txregs)
			return (_dma32_addrext(di->osh, di->d32txregs));
		else if (di->d32rxregs)
			return (_dma32_addrext(di->osh, di->d32rxregs));
	} else
		ASSERT(0);

	return FALSE;
}

/** initialize descriptor table base address */
static void
_dma_ddtable_init(dma_info_t *di, uint direction, dmaaddr_t pa)
{
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		if (!di->aligndesc_4k) {
			if (direction == DMA_TX)
				di->xmtptrbase = PHYSADDRLO(pa);
			else
				di->rcvptrbase = PHYSADDRLO(pa);
		}
		/* if this DMA channel is using indirect access, then configure the IndQSel register
		 * for this DMA channel
		 */
		if (DMA_INDIRECT(di)) {
			dma_set_indqsel((hnddma_t *)di, FALSE);
		}

		if ((di->ddoffsetlow == 0) || !(PHYSADDRLO(pa) & PCI32ADDR_HIGH)) {
			if (direction == DMA_TX) {
				W_REG(di->osh, &di->d64txregs->addrlow, (PHYSADDRLO(pa) +
				                                         di->ddoffsetlow));
				W_REG(di->osh, &di->d64txregs->addrhigh, (PHYSADDRHI(pa) +
				                                          di->ddoffsethigh));
			} else {
				W_REG(di->osh, &di->d64rxregs->addrlow, (PHYSADDRLO(pa) +
				                                         di->ddoffsetlow));
				W_REG(di->osh, &di->d64rxregs->addrhigh, (PHYSADDRHI(pa) +
				                                          di->ddoffsethigh));
			}
		} else {
			/* DMA64 32bits address extension */
			uint32 ae;
			ASSERT(di->addrext);
			ASSERT(PHYSADDRHI(pa) == 0);

			/* shift the high bit(s) from pa to ae */
			ae = (PHYSADDRLO(pa) & PCI32ADDR_HIGH) >> PCI32ADDR_HIGH_SHIFT;
			PHYSADDRLO(pa) &= ~PCI32ADDR_HIGH;

			if (direction == DMA_TX) {
				W_REG(di->osh, &di->d64txregs->addrlow, (PHYSADDRLO(pa) +
				                                         di->ddoffsetlow));
				W_REG(di->osh, &di->d64txregs->addrhigh, di->ddoffsethigh);
				SET_REG(di->osh, &di->d64txregs->control, D64_XC_AE,
					(ae << D64_XC_AE_SHIFT));
			} else {
				W_REG(di->osh, &di->d64rxregs->addrlow, (PHYSADDRLO(pa) +
				                                         di->ddoffsetlow));
				W_REG(di->osh, &di->d64rxregs->addrhigh, di->ddoffsethigh);
				SET_REG(di->osh, &di->d64rxregs->control, D64_RC_AE,
					(ae << D64_RC_AE_SHIFT));
			}
		}

	} else if (DMA32_ENAB(di)) {
		ASSERT(PHYSADDRHI(pa) == 0);
		if ((di->ddoffsetlow == 0) || !(PHYSADDRLO(pa) & PCI32ADDR_HIGH)) {
			if (direction == DMA_TX)
				W_REG(di->osh, &di->d32txregs->addr, (PHYSADDRLO(pa) +
				                                      di->ddoffsetlow));
			else
				W_REG(di->osh, &di->d32rxregs->addr, (PHYSADDRLO(pa) +
				                                      di->ddoffsetlow));
		} else {
			/* dma32 address extension */
			uint32 ae;
			ASSERT(di->addrext);

			/* shift the high bit(s) from pa to ae */
			ae = (PHYSADDRLO(pa) & PCI32ADDR_HIGH) >> PCI32ADDR_HIGH_SHIFT;
			PHYSADDRLO(pa) &= ~PCI32ADDR_HIGH;

			if (direction == DMA_TX) {
				W_REG(di->osh, &di->d32txregs->addr, (PHYSADDRLO(pa) +
				                                      di->ddoffsetlow));
				SET_REG(di->osh, &di->d32txregs->control, XC_AE, ae <<XC_AE_SHIFT);
			} else {
				W_REG(di->osh, &di->d32rxregs->addr, (PHYSADDRLO(pa) +
				                                      di->ddoffsetlow));
				SET_REG(di->osh, &di->d32rxregs->control, RC_AE, ae <<RC_AE_SHIFT);
			}
		}
	} else
		ASSERT(0);
}

static void
_dma_fifoloopbackenable(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_fifoloopbackenable\n", di->name));

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		/* if this DMA channel is using indirect access, then configure the IndQSel register
		 * for this DMA channel
		 */
		if (DMA_INDIRECT(di)) {
			dma_set_indqsel((hnddma_t *)di, FALSE);
		}
		OR_REG(di->osh, &di->d64txregs->control, D64_XC_LE);
	} else if (DMA32_ENAB(di))
		OR_REG(di->osh, &di->d32txregs->control, XC_LE);
	else
		ASSERT(0);
}

static void
_dma_rxinit(dma_info_t *di)
{
#ifdef BCMM2MDEV_ENABLED
	uint32 addrlow;
#endif
	DMA_TRACE(("%s: dma_rxinit\n", di->name));

	if (di->nrxd == 0)
		return;

#ifdef BCMPKTPOOL
	/* During the reset procedure, the active rxd may not be zero if pktpool is
	 * enabled, we need to reclaim active rxd to avoid rxd being leaked.
	 */
	if ((POOL_ENAB(di->pktpool)) && (NRXDACTIVE(di->rxin, di->rxout))) {
		ASSERT(di->hnddma.di_fn->rxreclaim != NULL);
		di->hnddma.di_fn->rxreclaim(&di->hnddma);
	}
#endif

	/* For split fifo, for fifo-0 reclaim is handled by fifo-1 */
	ASSERT((di->rxin == di->rxout) || (di->split_fifo == SPLIT_FIFO_0));
	di->rxin = di->rxout = di->rs0cd = 0;
	di->rxavail = di->nrxd - 1;

	/* limit nrxpost buffer count to the max that will fit on the ring */
	if (di->sep_rxhdr) {
		/* 2 descriptors per buffer for 'sep_rxhdr' */
		di->nrxpost = MIN(di->nrxpost, di->rxavail/2);
	} else {
		/* 1 descriptor for each buffer in the normal case */
		di->nrxpost = MIN(di->nrxpost, di->rxavail);
	}

	/* clear rx descriptor ring */
	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		BZERO_SM((void *)(uintptr)di->rxd64, (di->nrxd * sizeof(dma64dd_t)));
#if defined(BCM47XX_CA9)
		DMA_MAP(di->osh, (void *)(uintptr)di->rxd64, (di->nrxd * sizeof(dma64dd_t)),
		        DMA_TX, NULL, NULL);
#endif

		/* DMA engine with out alignment requirement requires table to be inited
		 * before enabling the engine
		 */
		if (!di->aligndesc_4k)
			_dma_ddtable_init(di, DMA_RX, di->rxdpa);

#ifdef BCMM2MDEV_ENABLED
	addrlow = R_REG(di->osh, &di->d64rxregs->addrlow);
	addrlow &= 0xffff;
	W_REG(di->osh, &di->d64rxregs->ptr, addrlow);
#endif

		_dma_rxenable(di);

		if (di->aligndesc_4k)
			_dma_ddtable_init(di, DMA_RX, di->rxdpa);

#ifdef BCMM2MDEV_ENABLED
	addrlow = R_REG(di->osh, &di->d64rxregs->addrlow);
	addrlow &= 0xffff;
	W_REG(di->osh, &di->d64rxregs->ptr, addrlow);
#endif

	} else if (DMA32_ENAB(di)) {
		BZERO_SM((void *)(uintptr)di->rxd32, (di->nrxd * sizeof(dma32dd_t)));
#if defined(BCM47XX_CA9)
		DMA_MAP(di->osh, (void *)(uintptr)di->rxd32, (di->nrxd * sizeof(dma32dd_t)),
		        DMA_TX, NULL, NULL);
#endif
		_dma_rxenable(di);
		_dma_ddtable_init(di, DMA_RX, di->rxdpa);
	} else
		ASSERT(0);
}

static void
_dma_rxenable(dma_info_t *di)
{
	uint dmactrlflags = di->hnddma.dmactrlflags;

	DMA_TRACE(("%s: dma_rxenable\n", di->name));

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		uint32 control = (R_REG(di->osh, &di->d64rxregs->control) & D64_RC_AE) | D64_RC_RE;

		if ((dmactrlflags & DMA_CTRL_PEN) == 0)
			control |= D64_RC_PD;

		if (dmactrlflags & DMA_CTRL_ROC)
			control |= D64_RC_OC;

		/* These bits 20:18 (burstLen) of control register can be written but will take
		 * effect only if these bits are valid. So this will not affect previous versions
		 * of the DMA. They will continue to have those bits set to 0.
		 */
		control &= ~D64_RC_BL_MASK;
		control |= (di->rxburstlen << D64_RC_BL_SHIFT);
#ifndef DMA_WAIT_COMPLT_ROM_COMPAT
		control &= ~D64_RC_WAITCMP_MASK;
		control |= (di->rxwaitforcomplt << D64_RC_WAITCMP_SHIFT);
#endif
		control &= ~D64_RC_PC_MASK;
		control |= (di->rxprefetchctl << D64_RC_PC_SHIFT);

		control &= ~D64_RC_PT_MASK;
		control |= (di->rxprefetchthresh << D64_RC_PT_SHIFT);

		if (DMA_TRANSCOHERENT(di)) {
			control &= ~D64_RC_CO_MASK;
			control |= (1 << D64_RC_CO_SHIFT);
		}

#if defined(D11_SPLIT_RX_FD)
		/* Separate rx hdr descriptor */
		if (di->sep_rxhdr)
			control |= (di->sep_rxhdr << D64_RC_SHIFT);
#endif /* D11_SPLIT_RX_FD */
		if (di->d11rx_war)
			control |= D64_RC_SH;

		W_REG(di->osh, &di->d64rxregs->control,
		      ((di->rxoffset << D64_RC_RO_SHIFT) | control));
	} else if (DMA32_ENAB(di)) {
		uint32 control = (R_REG(di->osh, &di->d32rxregs->control) & RC_AE) | RC_RE;

		if ((dmactrlflags & DMA_CTRL_PEN) == 0)
			control |= RC_PD;

		if (dmactrlflags & DMA_CTRL_ROC)
			control |= RC_OC;

		/* These bits 20:18 (burstLen) of control register can be written but will take
		 * effect only if these bits are valid. So this will not affect previous versions
		 * of the DMA. They will continue to have those bits set to 0.
		 */
		control &= ~RC_BL_MASK;
		control |= (di->rxburstlen << RC_BL_SHIFT);
#ifndef DMA_WAIT_COMPLT_ROM_COMPAT
		control &= ~RC_WAITCMP_MASK;
		control |= (di->rxwaitforcomplt << RC_WAITCMP_SHIFT);
#endif
		control &= ~RC_PC_MASK;
		control |= (di->rxprefetchctl << RC_PC_SHIFT);

		control &= ~RC_PT_MASK;
		control |= (di->rxprefetchthresh << RC_PT_SHIFT);

		W_REG(di->osh, &di->d32rxregs->control,
		      ((di->rxoffset << RC_RO_SHIFT) | control));
	} else
		ASSERT(0);
}

static void
_dma_rx_param_get(dma_info_t *di, uint16 *rxoffset, uint16 *rxbufsize)
{
	/* the normal values fit into 16 bits */
	*rxoffset = (uint16)di->rxoffset;
	*rxbufsize = (uint16)di->rxbufsize;
}

/*
 * there are cases where the host mem can't be used when the dongle is in certain
 * state, so this covers that case
 * used only for RX FIFO0 with the split rx builds
*/

int
dma_rxfill_suspend(hnddma_t *dmah, bool suspended)
{
	dma_info_t * di = DI_INFO(dmah);

	di->rxfill_suspended = suspended;
	return 0;
}

/** like getnexttxp but no reclaim */
static void *
_dma_peeknexttxp(dma_info_t *di)
{
	uint16 end, i;

	if (di->ntxd == 0)
		return (NULL);

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		/* if this DMA channel is using indirect access, then configure the IndQSel register
		 * for this DMA channel
		 */
		if (DMA_INDIRECT(di)) {
			dma_set_indqsel((hnddma_t *)di, FALSE);
		}

		end = (uint16)B2I(((R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_CD_MASK) -
		           di->xmtptrbase) & D64_XS0_CD_MASK, dma64dd_t);
		di->xs0cd = end;
	} else if (DMA32_ENAB(di)) {
		end = (uint16)B2I(R_REG(di->osh, &di->d32txregs->status) & XS_CD_MASK, dma32dd_t);
		di->xs0cd = end;
	} else
		ASSERT(0);

	for (i = di->txin; i != end; i = NEXTTXD(i))
		if (di->txp[i])
			return (di->txp[i]);

	return (NULL);
}

int
_dma_peekntxp(dma_info_t *di, int *len, void *txps[], txd_range_t range)
{
	uint16 start, end, i;
	uint act;
	void *txp = NULL;
	int k, len_max;

	DMA_TRACE(("%s: dma_peekntxp\n", di->name));

	ASSERT(len);
	ASSERT(txps);
	ASSERT(di);
	if (di->ntxd == 0) {
		*len = 0;
		return BCME_ERROR;
	}

	len_max = *len;
	*len = 0;

	start = di->xs0cd;
	if (range == HNDDMA_RANGE_ALL)
		end = di->txout;
	else {
		if (DMA64_ENAB(di)) {
			/* if this DMA channel is using indirect access, then configure the
			 * IndQSel register for this DMA channel
			 */
			if (DMA_INDIRECT(di)) {
				dma_set_indqsel((hnddma_t *)di, FALSE);
			}

			end = B2I(((R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_CD_MASK) -
				di->xmtptrbase) & D64_XS0_CD_MASK, dma64dd_t);

			act = (uint)(R_REG(di->osh, &di->d64txregs->status1) & D64_XS1_AD_MASK);
			act = (act - di->xmtptrbase) & D64_XS0_CD_MASK;
			act = (uint)B2I(act, dma64dd_t);
		} else {
			end = B2I(R_REG(di->osh, &di->d32txregs->status) & XS_CD_MASK, dma32dd_t);

			act = (uint)((R_REG(di->osh, &di->d32txregs->status) & XS_AD_MASK) >>
				XS_AD_SHIFT);
			act = (uint)B2I(act, dma32dd_t);
		}

		di->xs0cd = end;
		if (end != act)
			end = PREVTXD(act);
	}

	if ((start == 0) && (end > di->txout))
		return BCME_ERROR;

	k = 0;
	for (i = start; i != end; i = NEXTTXD(i)) {
		txp = di->txp[i];
		if (txp != NULL) {
			if (k < len_max)
				txps[k++] = txp;
			else
				break;
		}
	}
	*len = k;

	return BCME_OK;
}

/** like getnextrxp but not take off the ring */
static void *
_dma_peeknextrxp(dma_info_t *di)
{
	uint16 end, i;

	if (di->nrxd == 0)
		return (NULL);

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		end = (uint16)B2I(((R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_CD_MASK) -
			di->rcvptrbase) & D64_RS0_CD_MASK, dma64dd_t);
		di->rs0cd = end;
	} else if (DMA32_ENAB(di)) {
		end = (uint16)B2I(R_REG(di->osh, &di->d32rxregs->status) & RS_CD_MASK, dma32dd_t);
		di->rs0cd = end;
	} else
		ASSERT(0);

	for (i = di->rxin; i != end; i = NEXTRXD(i))
		if (di->rxp[i])
			return (di->rxp[i]);

	return (NULL);
}

static void
_dma_txblock(dma_info_t *di)
{
	di->hnddma.txavail = 0;
}

static void
_dma_txunblock(dma_info_t *di)
{
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;
}

static uint
_dma_txactive(dma_info_t *di)
{
	return NTXDACTIVE(di->txin, di->txout);
}

/* count the number of tx packets that are queued to the dma ring */
uint
dma_txp(hnddma_t *dmah)
{
	uint count = 0;
	uint16 i;
	dma_info_t * di = DI_INFO(dmah);

	/* count the number of txp pointers that are non-null */
	for (i = 0; i < di->ntxd; i++) {
		if (di->txp[i] != NULL) {
			count++;
		}
	}

	return count;
}

static uint
_dma_txpending(dma_info_t *di)
{
	uint16 curr;

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		/* if this DMA channel is using indirect access, then configure the IndQSel register
		 * for this DMA channel
		 */
		if (DMA_INDIRECT(di)) {
			dma_set_indqsel((hnddma_t *)di, FALSE);
		}

		curr = B2I(((R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_CD_MASK) -
		           di->xmtptrbase) & D64_XS0_CD_MASK, dma64dd_t);
		di->xs0cd = curr;
	} else if (DMA32_ENAB(di)) {
		curr = B2I(R_REG(di->osh, &di->d32txregs->status) & XS_CD_MASK, dma32dd_t);
		di->xs0cd = curr;
	} else
		ASSERT(0);

	return NTXDACTIVE(curr, di->txout);
}

static uint
_dma_txcommitted(dma_info_t *di)
{
	uint16 ptr;
	uint txin = di->txin;

	if (txin == di->txout)
		return 0;

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {
		/* if this DMA channel is using indirect access, then configure the IndQSel
		 * for this DMA channel
		 */
		if (DMA_INDIRECT(di)) {
			dma_set_indqsel((hnddma_t *)di, FALSE);
		}
		ptr = B2I(R_REG(di->osh, &di->d64txregs->ptr), dma64dd_t);
	} else if (DMA32_ENAB(di)) {
		ptr = B2I(R_REG(di->osh, &di->d32txregs->ptr), dma32dd_t);
	} else
		ASSERT(0);

	return NTXDACTIVE(di->txin, ptr);
}

static uint
_dma_rxactive(dma_info_t *di)
{
	return NRXDACTIVE(di->rxin, di->rxout);
}

static uint
_dma_activerxbuf(dma_info_t *di)
{
	uint16 curr, ptr;
	curr = B2I(((R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_CD_MASK) -
		di->rcvptrbase) & D64_RS0_CD_MASK, dma64dd_t);
	ptr =  B2I(((R_REG(di->osh, &di->d64rxregs->ptr) & D64_RS0_CD_MASK) -
		di->rcvptrbase) & D64_RS0_CD_MASK, dma64dd_t);
	return NRXDACTIVE(curr, ptr);
}


static void
_dma_counterreset(dma_info_t *di)
{
	/* reset all software counter */
	di->hnddma.rxgiants = 0;
	di->hnddma.rxnobuf = 0;
	di->hnddma.txnobuf = 0;
}

static uint
_dma_ctrlflags(dma_info_t *di, uint mask, uint flags)
{
	uint dmactrlflags;

	if (!di) {
		DMA_ERROR(("_dma_ctrlflags: NULL dma handle\n"));
		return (0);
	}

	dmactrlflags = di->hnddma.dmactrlflags;
	ASSERT((flags & ~mask) == 0);

	dmactrlflags &= ~mask;
	dmactrlflags |= flags;

	/* If trying to enable parity, check if parity is actually supported */
	if (dmactrlflags & DMA_CTRL_PEN) {
		uint32 control;

		if (DMA64_ENAB(di) && DMA64_MODE(di)) {
			/* if using indirect DMA access, then configure IndQSel */
			if (DMA_INDIRECT(di)) {
				dma_set_indqsel((hnddma_t *)di, FALSE);
			}

			control = R_REG(di->osh, &di->d64txregs->control);
			W_REG(di->osh, &di->d64txregs->control, control | D64_XC_PD);
			if (R_REG(di->osh, &di->d64txregs->control) & D64_XC_PD) {
				/* We *can* disable it so it is supported,
				 * restore control register
				 */
				W_REG(di->osh, &di->d64txregs->control, control);
			} else {
				/* Not supported, don't allow it to be enabled */
				dmactrlflags &= ~DMA_CTRL_PEN;
			}
		} else if (DMA32_ENAB(di)) {
			control = R_REG(di->osh, &di->d32txregs->control);
			W_REG(di->osh, &di->d32txregs->control, control | XC_PD);
			if (R_REG(di->osh, &di->d32txregs->control) & XC_PD) {
				W_REG(di->osh, &di->d32txregs->control, control);
			} else {
				/* Not supported, don't allow it to be enabled */
				dmactrlflags &= ~DMA_CTRL_PEN;
			}
		} else
			ASSERT(0);
	}

	di->hnddma.dmactrlflags = dmactrlflags;

	return (dmactrlflags);
}

static uint
_dma_avoidancecnt(dma_info_t *di)
{
	return (di->dma_avoidance_cnt);
}

void
dma_txpioloopback(osl_t *osh, dma32regs_t *regs)
{
	OR_REG(osh, &regs->control, XC_LE);
}

static
uint8 BCMATTACHFN_DMA_ATTACH(dma_align_sizetobits)(uint size)
{
	uint8 bitpos = 0;
	ASSERT(size);
	ASSERT(!(size & (size-1)));
	while (size >>= 1) {
		bitpos ++;
	}
	return (bitpos);
}

/**
 * Allocates one rx or tx descriptor ring. Does not allocate buffers.
 * This function ensures that the DMA descriptor ring will not get allocated
 * across Page boundary. If the allocation is done across the page boundary
 * at the first time, then it is freed and the allocation is done at
 * descriptor ring size aligned location. This will ensure that the ring will
 * not cross page boundary
 */
static void *
BCMATTACHFN_DMA_ATTACH(dma_ringalloc)(osl_t *osh, uint32 boundary, uint size, uint16 *alignbits,
	uint* alloced, dmaaddr_t *descpa, osldma_t **dmah)
{
	void * va;
	uint32 desc_strtaddr;
	uint32 alignbytes = 1 << *alignbits;

	if ((va = DMA_ALLOC_CONSISTENT(osh, size, *alignbits, alloced,
			descpa, (void **)dmah)) == NULL)
		return NULL;

	desc_strtaddr = (uint32)ROUNDUP((uint)PHYSADDRLO(*descpa), alignbytes);
	if (((desc_strtaddr + size - 1) & boundary) !=
	    (desc_strtaddr & boundary)) {
		*alignbits = dma_align_sizetobits(size);
		DMA_FREE_CONSISTENT(osh, va,
		                    size, *descpa, *dmah);
		va = DMA_ALLOC_CONSISTENT(osh, size, *alignbits, alloced, descpa, (void **)dmah);
	}
	return va;
}

#if defined(BCMDBG) || defined(BCMDBG_DUMP)
static void
dma32_dumpring(dma_info_t *di, struct bcmstrbuf *b, dma32dd_t *ring, uint start, uint end,
	uint max_num)
{
	uint i;

	BCM_REFERENCE(di);

	for (i = start; i != end; i = XXD((i + 1), max_num)) {
		/* in the format of high->low 8 bytes */
		bcm_bprintf(b, "ring index %d: 0x%x %x\n",
			i, R_SM(&ring[i].addr), R_SM(&ring[i].ctrl));
	}
}

static void
dma32_dumptx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring)
{
	if (di->ntxd == 0)
		return;

	bcm_bprintf(b, "DMA32: txd32 %p txdpa 0x%lx txp %p txin %d txout %d "
		    "txavail %d txnodesc %d\n", OSL_OBFUSCATE_BUF(di->txd32),
		    PHYSADDRLO(di->txdpa), OSL_OBFUSCATE_BUF(di->txp), di->txin,
		    di->txout, di->hnddma.txavail, di->hnddma.txnodesc);

	bcm_bprintf(b, "xmtcontrol 0x%x xmtaddr 0x%x xmtptr 0x%x xmtstatus 0x%x\n",
		R_REG(di->osh, &di->d32txregs->control),
		R_REG(di->osh, &di->d32txregs->addr),
		R_REG(di->osh, &di->d32txregs->ptr),
		R_REG(di->osh, &di->d32txregs->status));

	if (dumpring && di->txd32)
		dma32_dumpring(di, b, di->txd32, di->txin, di->txout, di->ntxd);
}

static void
dma32_dumprx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring)
{
	if (di->nrxd == 0)
		return;

	bcm_bprintf(b, "DMA32: rxd32 %p rxdpa 0x%lx rxp %p rxin %d rxout %d\n",
	            OSL_OBFUSCATE_BUF(di->rxd32), PHYSADDRLO(di->rxdpa),
			OSL_OBFUSCATE_BUF(di->rxp), di->rxin, di->rxout);

	bcm_bprintf(b, "rcvcontrol 0x%x rcvaddr 0x%x rcvptr 0x%x rcvstatus 0x%x\n",
		R_REG(di->osh, &di->d32rxregs->control),
		R_REG(di->osh, &di->d32rxregs->addr),
		R_REG(di->osh, &di->d32rxregs->ptr),
		R_REG(di->osh, &di->d32rxregs->status));
	if (di->rxd32 && dumpring)
		dma32_dumpring(di, b, di->rxd32, di->rxin, di->rxout, di->nrxd);
}

static void
dma32_dump(dma_info_t *di, struct bcmstrbuf *b, bool dumpring)
{
	dma32_dumptx(di, b, dumpring);
	dma32_dumprx(di, b, dumpring);
}

static void
dma64_dumpring(dma_info_t *di, struct bcmstrbuf *b, dma64dd_t *ring, uint start, uint end,
	uint max_num)
{
	uint i;

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	BCM_REFERENCE(di);

	for (i = start; i != end; i = XXD((i + 1), max_num)) {
		/* in the format of high->low 16 bytes */
		if (b) {
			bcm_bprintf(b, "ring index %d: 0x%x %x %x %x\n",
			i, R_SM(&ring[i].addrhigh), R_SM(&ring[i].addrlow),
			R_SM(&ring[i].ctrl2), R_SM(&ring[i].ctrl1));
		} else {
			DMA_ERROR(("ring index %d: 0x%x %x %x %x\n",
			i, R_SM(&ring[i].addrhigh), R_SM(&ring[i].addrlow),
			R_SM(&ring[i].ctrl2), R_SM(&ring[i].ctrl1)));
		}
	}
}

static void
dma64_dumptx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring)
{
	if (di->ntxd == 0)
		return;

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	if (b) {
		bcm_bprintf(b, "DMA64: txd64 %p txdpa 0x%lx txdpahi 0x%lx txp %p txin %d txout %d "
			"txavail %d txnodesc %d\n", di->txd64, PHYSADDRLO(di->txdpa),
			PHYSADDRHI(di->txdpaorig), di->txp, di->txin, di->txout, di->hnddma.txavail,
			di->hnddma.txnodesc);

		bcm_bprintf(b, "xmtcontrol 0x%x xmtaddrlow 0x%x xmtaddrhigh 0x%x "
			    "xmtptr 0x%x xmtstatus0 0x%x xmtstatus1 0x%x\n",
			    R_REG(di->osh, &di->d64txregs->control),
			    R_REG(di->osh, &di->d64txregs->addrlow),
			    R_REG(di->osh, &di->d64txregs->addrhigh),
			    R_REG(di->osh, &di->d64txregs->ptr),
			    R_REG(di->osh, &di->d64txregs->status0),
			    R_REG(di->osh, &di->d64txregs->status1));

		bcm_bprintf(b, "DMA64: DMA avoidance applied %d\n", di->dma_avoidance_cnt);
	} else {
		DMA_ERROR(("DMA64: txd64 %p txdpa 0x%lx txdpahi 0x%lx txp %p txin %d txout %d "
		       "txavail %d txnodesc %d\n", di->txd64, (unsigned long)PHYSADDRLO(di->txdpa),
		       (unsigned long)PHYSADDRHI(di->txdpaorig), di->txp, di->txin, di->txout,
		       di->hnddma.txavail, di->hnddma.txnodesc));

		DMA_ERROR(("xmtcontrol 0x%x xmtaddrlow 0x%x xmtaddrhigh 0x%x "
		       "xmtptr 0x%x xmtstatus0 0x%x xmtstatus1 0x%x\n",
		       R_REG(di->osh, &di->d64txregs->control),
		       R_REG(di->osh, &di->d64txregs->addrlow),
		       R_REG(di->osh, &di->d64txregs->addrhigh),
		       R_REG(di->osh, &di->d64txregs->ptr),
		       R_REG(di->osh, &di->d64txregs->status0),
		       R_REG(di->osh, &di->d64txregs->status1)));
	}

	if (dumpring && di->txd64) {
		dma64_dumpring(di, b, di->txd64, di->txin, di->txout, di->ntxd);
	}
}

static void
dma64_dumprx(dma_info_t *di, struct bcmstrbuf *b, bool dumpring)
{
	if (di->nrxd == 0)
		return;

	bcm_bprintf(b, "DMA64: rxd64 %p rxdpa 0x%lx rxdpahi 0x%lx rxp %p rxin %d rxout %d\n",
			OSL_OBFUSCATE_BUF(di->rxd64), PHYSADDRLO(di->rxdpa),
			PHYSADDRHI(di->rxdpaorig), OSL_OBFUSCATE_BUF(di->rxp),
			di->rxin, di->rxout);

	bcm_bprintf(b, "rcvcontrol 0x%x rcvaddrlow 0x%x rcvaddrhigh 0x%x rcvptr "
		       "0x%x rcvstatus0 0x%x rcvstatus1 0x%x\n",
		       R_REG(di->osh, &di->d64rxregs->control),
		       R_REG(di->osh, &di->d64rxregs->addrlow),
		       R_REG(di->osh, &di->d64rxregs->addrhigh),
		       R_REG(di->osh, &di->d64rxregs->ptr),
		       R_REG(di->osh, &di->d64rxregs->status0),
		       R_REG(di->osh, &di->d64rxregs->status1));
	if (di->rxd64 && dumpring) {
		dma64_dumpring(di, b, di->rxd64, di->rxin, di->rxout, di->nrxd);
	}
}

static void
dma64_dump(dma_info_t *di, struct bcmstrbuf *b, bool dumpring)
{
	dma64_dumptx(di, b, dumpring);
	dma64_dumprx(di, b, dumpring);
}

#endif	/* BCMDBG || BCMDBG_DUMP */


/* 32-bit DMA functions */

static void
dma32_txinit(dma_info_t *di)
{
	uint32 control = XC_XE;

	DMA_TRACE(("%s: dma_txinit\n", di->name));

	if (di->ntxd == 0)
		return;

	di->txin = di->txout = di->xs0cd = 0;
	di->hnddma.txavail = di->ntxd - 1;

	/* clear tx descriptor ring */
	BZERO_SM(DISCARD_QUAL(di->txd32, void), (di->ntxd * sizeof(dma32dd_t)));
#if defined(BCM47XX_CA9)
	DMA_MAP(di->osh, DISCARD_QUAL(di->txd32, void), (di->ntxd * sizeof(dma32dd_t)),
	        DMA_TX, NULL, NULL);
#endif

	/* These bits 20:18 (burstLen) of control register can be written but will take
	 * effect only if these bits are valid. So this will not affect previous versions
	 * of the DMA. They will continue to have those bits set to 0.
	 */
	control |= (di->txburstlen << XC_BL_SHIFT);
	control |= (di->txmultioutstdrd << XC_MR_SHIFT);
	control |= (di->txprefetchctl << XC_PC_SHIFT);
	control |= (di->txprefetchthresh << XC_PT_SHIFT);

	if ((di->hnddma.dmactrlflags & DMA_CTRL_PEN) == 0)
		control |= XC_PD;
	W_REG(di->osh, &di->d32txregs->control, control);
	_dma_ddtable_init(di, DMA_TX, di->txdpa);
}

static bool
dma32_txenabled(dma_info_t *di)
{
	uint32 xc;

	/* If the chip is dead, it is not enabled :-) */
	xc = R_REG(di->osh, &di->d32txregs->control);
	return ((xc != 0xffffffff) && (xc & XC_XE));
}

static void
dma32_txsuspend(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_txsuspend\n", di->name));

	if (di->ntxd == 0)
		return;

	OR_REG(di->osh, &di->d32txregs->control, XC_SE);
}

static void
dma32_txresume(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_txresume\n", di->name));

	if (di->ntxd == 0)
		return;

	AND_REG(di->osh, &di->d32txregs->control, ~XC_SE);
}

static bool
dma32_txsuspended(dma_info_t *di)
{
	return (di->ntxd == 0) || ((R_REG(di->osh, &di->d32txregs->control) & XC_SE) == XC_SE);
}

static void
dma32_txflush(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_txflush\n", di->name));

	if (di->ntxd == 0)
		return;

	OR_REG(di->osh, &di->d32txregs->control, XC_SE | XC_FL);
}

static void
dma32_txflush_clear(dma_info_t *di)
{
	uint32 status;

	DMA_TRACE(("%s: dma_txflush_clear\n", di->name));

	if (di->ntxd == 0)
		return;

	SPINWAIT(((status = (R_REG(di->osh, &di->d32txregs->status) & XS_XS_MASK))
		 != XS_XS_DISABLED) &&
		 (status != XS_XS_IDLE) &&
		 (status != XS_XS_STOPPED),
		 (10000));
	AND_REG(di->osh, &di->d32txregs->control, ~XC_FL);
}

static bool
dma32_txstopped(dma_info_t *di)
{
	return ((R_REG(di->osh, &di->d32txregs->status) & XS_XS_MASK) == XS_XS_STOPPED);
}

static bool
dma32_rxstopped(dma_info_t *di)
{
	return ((R_REG(di->osh, &di->d32rxregs->status) & RS_RS_MASK) == RS_RS_STOPPED);
}

/** Allocates one rx or tx descriptor ring. Does not allocate buffers. */
static bool
BCMATTACHFN_DMA_ATTACH(dma32_alloc)(dma_info_t *di, uint direction)
{
	uint size;	/**< nr of bytes that one descriptor ring will occupy */
	uint ddlen;
	void *va;
	uint alloced;
	uint16 align;
	uint16 align_bits;

	ddlen = sizeof(dma32dd_t);

	size = (direction == DMA_TX) ? (di->ntxd * ddlen) : (di->nrxd * ddlen);

	alloced = 0;
	align_bits = di->dmadesc_align;
	align = (1 << align_bits);

	if (direction == DMA_TX) {
		if ((va = dma_ringalloc(di->osh, D32RINGALIGN, size, &align_bits, &alloced,
			&di->txdpaorig, &di->tx_dmah)) == NULL) {
			DMA_ERROR(("%s: dma_alloc: DMA_ALLOC_CONSISTENT(ntxd) failed\n",
			           di->name));
			return FALSE;
		}

		PHYSADDRHISET(di->txdpa, 0);
		ASSERT(PHYSADDRHI(di->txdpaorig) == 0);
		di->txd32 = (dma32dd_t *)ROUNDUP((uintptr)va, align);
		di->txdalign = (uint)((int8 *)(uintptr)di->txd32 - (int8 *)va);

		PHYSADDRLOSET(di->txdpa, PHYSADDRLO(di->txdpaorig) + di->txdalign);
		/* Make sure that alignment didn't overflow */
		ASSERT(PHYSADDRLO(di->txdpa) >= PHYSADDRLO(di->txdpaorig));

		di->txdalloc = alloced;
		ASSERT(ISALIGNED(di->txd32, align));
	} else {
		if ((va = dma_ringalloc(di->osh, D32RINGALIGN, size, &align_bits, &alloced,
			&di->rxdpaorig, &di->rx_dmah)) == NULL) {
			DMA_ERROR(("%s: dma_alloc: DMA_ALLOC_CONSISTENT(nrxd) failed\n",
			           di->name));
			return FALSE;
		}

		PHYSADDRHISET(di->rxdpa, 0);
		ASSERT(PHYSADDRHI(di->rxdpaorig) == 0);
		di->rxd32 = (dma32dd_t *)ROUNDUP((uintptr)va, align);
		di->rxdalign = (uint)((int8 *)(uintptr)di->rxd32 - (int8 *)va);

		PHYSADDRLOSET(di->rxdpa, PHYSADDRLO(di->rxdpaorig) + di->rxdalign);
		/* Make sure that alignment didn't overflow */
		ASSERT(PHYSADDRLO(di->rxdpa) >= PHYSADDRLO(di->rxdpaorig));
		di->rxdalloc = alloced;
		ASSERT(ISALIGNED(di->rxd32, align));
	}

	return TRUE;
}

static bool
dma32_txreset(dma_info_t *di)
{
	uint32 status;

	if (di->ntxd == 0)
		return TRUE;

	/* suspend tx DMA first */
	W_REG(di->osh, &di->d32txregs->control, XC_SE);
	SPINWAIT(((status = (R_REG(di->osh, &di->d32txregs->status) & XS_XS_MASK))
		 != XS_XS_DISABLED) &&
		 (status != XS_XS_IDLE) &&
		 (status != XS_XS_STOPPED),
		 (10000));

	W_REG(di->osh, &di->d32txregs->control, 0);
	SPINWAIT(((status = (R_REG(di->osh,
	         &di->d32txregs->status) & XS_XS_MASK)) != XS_XS_DISABLED),
	         10000);

	/* We should be disabled at this point */
	if (status != XS_XS_DISABLED) {
		DMA_ERROR(("%s: status != D64_XS0_XS_DISABLED 0x%x\n", __FUNCTION__, status));
		ASSERT(status == XS_XS_DISABLED);
		OSL_DELAY(300);
	}

	return (status == XS_XS_DISABLED);
}

static bool
dma32_rxcheckidlestatus(dma_info_t *di)
{
	if (di->nrxd == 0) {
		return TRUE;
	}

	/* Ensure that Rx rcvstatus is in Idle Wait */
	if (R_REG(di->osh, &di->d32rxregs->status) & RS_RS_IDLE) {
		return TRUE;
	}
	return FALSE;
}

static bool
dma32_rxreset(dma_info_t *di)
{
	uint32 status;

	if (di->nrxd == 0)
		return TRUE;

	W_REG(di->osh, &di->d32rxregs->control, 0);
	SPINWAIT(((status = (R_REG(di->osh,
	         &di->d32rxregs->status) & RS_RS_MASK)) != RS_RS_DISABLED),
	         10000);

	return (status == RS_RS_DISABLED);
}

static bool
dma32_txsuspendedidle(dma_info_t *di)
{
	if (di->ntxd == 0)
		return TRUE;

	if (!(R_REG(di->osh, &di->d32txregs->control) & XC_SE))
		return 0;

	if ((R_REG(di->osh, &di->d32txregs->status) & XS_XS_MASK) != XS_XS_IDLE)
		return 0;

	OSL_DELAY(2);
	return ((R_REG(di->osh, &di->d32txregs->status) & XS_XS_MASK) == XS_XS_IDLE);
}

/**
 * Updating rxfill dynamically
 *
 * Dynamically updates rxfill'ed descriptors. If actively posted buffers are more
 * than nrxpost, this function will free up the remaining (active-nrxpost) posted buffers.
 */
void
dma_update_rxfill(hnddma_t *dmah)
{
	void *p;
	uint16 ad, rxout, n, dpp;
	dma_info_t * di = DI_INFO(dmah);
#if defined(SGLIST_RX_SUPPORT)
	dmaaddr_t pa;
#endif /* SGLIST_RX_SUPPORT */

	/* if sep_rxhdr is enabled, for every pkt, two descriptors are programmed */
	/* NRXDACTIVE(rxin, rxout) would show 2 times no of actual full pkts */
	dpp = (di->sep_rxhdr) ? 2 : 1; /* dpp - descriptors per packet */

	/* read active descriptor */
	ad = (uint16)B2I(((R_REG(di->osh, &di->d64rxregs->status1) & D64_RS1_AD_MASK) -
		di->rcvptrbase) & D64_RS1_AD_MASK, dma64dd_t);
	rxout = di->rxout;

	/* if currently less buffers posted than requesting rxpost, do nothing */
	if (NRXDACTIVE(ad, rxout) <= (uint16)(di->nrxpost * dpp))
		return;

	/* calculating number of descriptors to remove */
	n = NRXDACTIVE(ad, rxout) - di->nrxpost * dpp;
	if (n % 2 != 0)
		n++;

	/* rewind rxout to move back "n" descriptors */
	di->rxout = PREVNRXD(rxout, n);

	/* update the chip lastdscr pointer */
	if (DMA64_ENAB(di) && DMA64_MODE(di))
		W_REG(di->osh, &di->d64rxregs->ptr, di->rcvptrbase + I2B(di->rxout, dma64dd_t));

	 /* free up "n" descriptors i.e., (n/dpp) packets from bottom */
	while (n-- > 0) {
		rxout = PREVRXD(rxout);
		p = di->rxp[rxout];
		/* indicate 'rxout' index is free */
		di->rxp[rxout] = NULL;
#if defined(SGLIST_RX_SUPPORT)
		PHYSADDRLOSET(pa,
			(BUS_SWAP32(R_SM(&di->rxd64[rxout].addrlow)) - di->dataoffsetlow));
		PHYSADDRHISET(pa,
			(BUS_SWAP32(R_SM(&di->rxd64[rxout].addrhigh)) - di->dataoffsethigh));

		/* clear this packet from the descriptor ring */
		DMA_UNMAP(di->osh, pa, di->rxbufsize, DMA_RX, p, &di->rxp_dmah[rxout]);

		W_SM(&di->rxd64[rxout].addrlow, 0xdeadbeef);
		W_SM(&di->rxd64[rxout].addrhigh, 0xdeadbeef);
#endif /* SGLIST_RX_SUPPORT */
		if (p != (void*)PCI64ADDR_HIGH)
			PKTFREE(di->osh, p, FALSE);
	}

	ASSERT(NRXDACTIVE(ad, rxout) <= (uint16)(di->nrxpost * dpp));
}

/**
 * Rotate all active tx dma ring entries "forward" by (ActiveDescriptor - txin).
 */
static void
dma32_txrotate(dma_info_t *di)
{
	uint16 ad;
	uint nactive;
	uint rot;
	uint16 old, new;
	uint32 w;
	uint16 first, last;

	ASSERT(dma32_txsuspendedidle(di));

	nactive = _dma_txactive(di);
	ad = B2I(((R_REG(di->osh, &di->d32txregs->status) & XS_AD_MASK) >> XS_AD_SHIFT), dma32dd_t);
	rot = TXD(ad - di->txin);

	ASSERT(rot < di->ntxd);

	/* full-ring case is a lot harder - don't worry about this */
	if (rot >= (di->ntxd - nactive)) {
		DMA_ERROR(("%s: dma_txrotate: ring full - punt\n", di->name));
		return;
	}

	first = di->txin;
	last = PREVTXD(di->txout);

	/* move entries starting at last and moving backwards to first */
	for (old = last; old != PREVTXD(first); old = PREVTXD(old)) {
		new = TXD(old + rot);

		/*
		 * Move the tx dma descriptor.
		 * EOT is set only in the last entry in the ring.
		 */
		w = BUS_SWAP32(R_SM(&di->txd32[old].ctrl)) & ~CTRL_EOT;
		if (new == (di->ntxd - 1))
			w |= CTRL_EOT;
		W_SM(&di->txd32[new].ctrl, BUS_SWAP32(w));
		W_SM(&di->txd32[new].addr, R_SM(&di->txd32[old].addr));

#if defined(DESCR_DEADBEEF)
		/* zap the old tx dma descriptor address field */
		W_SM(&di->txd32[old].addr, BUS_SWAP32(0xdeadbeef));
#endif /* DESCR_DEADBEEF */

		/* move the corresponding txp[] entry */
		ASSERT(di->txp[new] == NULL);
		di->txp[new] = di->txp[old];

		/* Move the segment map as well */
		if (DMASGLIST_ENAB) {
			bcopy(&di->txp_dmah[old], &di->txp_dmah[new], sizeof(hnddma_seg_map_t));
			bzero(&di->txp_dmah[old], sizeof(hnddma_seg_map_t));
		}

		di->txp[old] = NULL;
	}

	/* update txin and txout */
	di->txin = ad;
	di->txout = TXD(di->txout + rot);
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	/* kick the chip */
	dma32_txcommit_local(di);
}

/* 64-bit DMA functions */

static void
dma64_txinit(dma_info_t *di)
{
	uint32 control;
#if defined(BCMM2MDEV_ENABLED) || defined(BCM_DMA_INDIRECT)
	uint32 addrlow;
#endif
#ifdef BCM_DMA_INDIRECT
	uint32 act;
	int i;
	si_t *sih = di->sih;
#endif /* BCM_DMA_INDIRECT */

	DMA_TRACE(("%s: dma_txinit\n", di->name));

	if (di->ntxd == 0)
		return;
	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	di->txin = di->txout = di->xs0cd = di->xs0cd_snapshot = 0;
	di->hnddma.txavail = di->ntxd - 1;

	/* clear tx descriptor ring */
	BZERO_SM((void *)(uintptr)di->txd64, (di->ntxd * sizeof(dma64dd_t)));
#if defined(BCM47XX_CA9)
	DMA_MAP(di->osh, (void *)(uintptr)di->txd64, (di->ntxd * sizeof(dma64dd_t)),
	        DMA_TX, NULL, NULL);
#endif

	/* These bits 20:18 (burstLen) of control register can be written but will take
	 * effect only if these bits are valid. So this will not affect previous versions
	 * of the DMA. They will continue to have those bits set to 0.
	 */
	control = R_REG(di->osh, &di->d64txregs->control);
	control = (control & ~D64_XC_BL_MASK) | (di->txburstlen << D64_XC_BL_SHIFT);
	control = (control & ~D64_XC_MR_MASK) | (di->txmultioutstdrd << D64_XC_MR_SHIFT);
	control = (control & ~D64_XC_PC_MASK) | (di->txprefetchctl << D64_XC_PC_SHIFT);
	control = (control & ~D64_XC_PT_MASK) | (di->txprefetchthresh << D64_XC_PT_SHIFT);
	control = (control & ~D64_XC_CS_MASK) | (di->txchanswitch << D64_XC_CS_SHIFT);
	if (DMA_TRANSCOHERENT(di))
		control = (control & ~D64_XC_CO_MASK) | (1 << D64_XC_CO_SHIFT);
	W_REG(di->osh, &di->d64txregs->control, control);

	control = D64_XC_XE;
	/* DMA engine with out alignment requirement requires table to be inited
	 * before enabling the engine
	 */
	if (!di->aligndesc_4k)
		_dma_ddtable_init(di, DMA_TX, di->txdpa);
#ifdef BCMM2MDEV_ENABLED
	addrlow = R_REG(di->osh, &di->d64txregs->addrlow);
	addrlow &= 0xffff;
	W_REG(di->osh, &di->d64txregs->ptr, addrlow);
#endif

#ifdef BCM_DMA_INDIRECT
	if (DMA_INDIRECT(di) &&
		(di->hnddma.dmactrlflags & DMA_CTRL_DESC_ONLY_FLAG) &&
		((si_coreid(sih) == D11_CORE_ID) && (si_corerev(sih) == 65))) {
		addrlow = (uint32)(R_REG(di->osh, &di->d64txregs->addrlow) & 0xffff);
		if (addrlow != 0)
			W_REG(di->osh, &di->d64txregs->ptr, addrlow);
		for (i = 0; i < 20; i++) {
			act = (uint32)(R_REG(di->osh, &di->d64txregs->status1) & 0xffff);
			if (addrlow == act) {
				break;
			}
			OSL_DELAY(1);
		}
		if (addrlow != act) {
			DMA_ERROR(("%s %s: dma txdesc AD %#x != addrlow %#x\n", di->name,
			__FUNCTION__, act, addrlow));
		}
		ASSERT(addrlow == act);
	}
#endif /* BCM_DMA_INDIRECT */

	if (di->hnddma.dmactrlflags & DMA_CTRL_CS)
		control |= D64_XC_CS_MASK;

	if ((di->hnddma.dmactrlflags & DMA_CTRL_PEN) == 0)
		control |= D64_XC_PD;
	OR_REG(di->osh, &di->d64txregs->control, control);

	/* DMA engine with alignment requirement requires table to be inited
	 * before enabling the engine
	 */
	if (di->aligndesc_4k)
		_dma_ddtable_init(di, DMA_TX, di->txdpa);

#ifdef BCMM2MDEV_ENABLED
	addrlow = R_REG(di->osh, &di->d64txregs->addrlow);
	addrlow &= 0xffff;
	W_REG(di->osh, &di->d64txregs->ptr, addrlow);
#endif
}

static bool
dma64_txenabled(dma_info_t *di)
{
	uint32 xc;
	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	/* If the chip is dead, it is not enabled :-) */
	xc = R_REG(di->osh, &di->d64txregs->control);
	return ((xc != 0xffffffff) && (xc & D64_XC_XE));
}

static void
dma64_txsuspend(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_txsuspend\n", di->name));

	if (di->ntxd == 0)
		return;

#ifdef BCM_DMA_INDIRECT
	/* if using indirect DMA access, then use common register, SuspReq */
	if (DMA_INDIRECT(di)) {
		OR_REG(di->osh, di->dmacommon->suspreq, (1 << di->q_index));
	} else
#endif
	{
		OR_REG(di->osh, &di->d64txregs->control, D64_XC_SE);
	}
}

static void
dma64_txresume(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_txresume\n", di->name));

	if (di->ntxd == 0)
		return;

#ifdef BCM_DMA_INDIRECT
	/* if using indirect DMA access, then use common register, SuspReq */
	if (DMA_INDIRECT(di)) {
		AND_REG(di->osh, di->dmacommon->suspreq, ~(1 << di->q_index));
	} else
#endif
	{
		AND_REG(di->osh, &di->d64txregs->control, ~D64_XC_SE);
	}
}

static bool
dma64_txsuspended(dma_info_t *di)
{
	if (di->ntxd == 0)
		return TRUE;

#ifdef BCM_DMA_INDIRECT
	/* if using indirect DMA access, then use common register, SuspReq */
	if (DMA_INDIRECT(di)) {
		return ((R_REG(di->osh, di->dmacommon->suspreq) & (1 << di->q_index)) != 0);
	} else
#endif
	{
		return ((R_REG(di->osh, &di->d64txregs->control) & D64_XC_SE) == D64_XC_SE);
	}
}

static void
dma64_txflush(dma_info_t *di)
{
	DMA_TRACE(("%s: dma_txflush\n", di->name));

	if (di->ntxd == 0)
		return;

#ifdef BCM_DMA_INDIRECT
	/* if using indirect DMA access, then use common register, FlushReq */
	if (DMA_INDIRECT(di)) {
		OR_REG(di->osh, di->dmacommon->flushreq, (1 << di->q_index));
	} else
#endif
	{
		OR_REG(di->osh, &di->d64txregs->control, D64_XC_SE | D64_XC_FL);
	}
}

static void
dma64_txflush_clear(dma_info_t *di)
{
	uint32 status;

	DMA_TRACE(("%s: dma_txflush_clear\n", di->name));

	if (di->ntxd == 0)
		return;

#ifdef BCM_DMA_INDIRECT
	/* if using indirect DMA access, then use common register, FlushReq */
	if (DMA_INDIRECT(di)) {
		AND_REG(di->osh, di->dmacommon->flushreq, ~(1 << di->q_index));
	} else
#endif
	{
		SPINWAIT(((status = (R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_XS_MASK)) !=
		          D64_XS0_XS_DISABLED) &&
		         (status != D64_XS0_XS_IDLE) &&
		         (status != D64_XS0_XS_STOPPED),
		         10000);
		AND_REG(di->osh, &di->d64txregs->control, ~D64_XC_FL);
	}
}

void
dma_txrewind(hnddma_t *dmah)
{
	uint16 start, end, i;
	uint act;
	uint32 flags;
	dma64dd_t *ring;

	dma_info_t * di = DI_INFO(dmah);

	DMA_TRACE(("%s: dma64_txrewind\n", di->name));

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	ASSERT(dma64_txsuspended(di));

	/* select to read index of already fetched desc */
	AND_REG(di->osh, &di->d64txregs->control, ~D64_RC_SA);

	act = (uint)(R_REG(di->osh, &di->d64txregs->status1) & D64_XS1_AD_MASK);
	act = (act - di->xmtptrbase) & D64_XS0_CD_MASK;
	start = (uint16)B2I(act, dma64dd_t);

	end = di->txout;

	ring = di->txd64;
	for (i = start; i != end; i = NEXTTXD(i)) {
		/* find the first one having eof set */
		flags = R_SM(&ring[i].ctrl1);
		if (flags & CTRL_EOF) {
			/* rewind end to (i+1) */
			W_REG(di->osh,
			      &di->d64txregs->ptr, di->xmtptrbase + I2B(NEXTTXD(i), dma64dd_t));
			DMA_TRACE(("ActiveIdx %d EndIdx was %d now %d\n", start, end, NEXTTXD(i)));
			break;
		}
	}
}

static bool
dma64_txstopped(dma_info_t *di)
{
	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	return ((R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_XS_MASK) == D64_XS0_XS_STOPPED);
}

static bool
dma64_rxstopped(dma_info_t *di)
{
	return ((R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_RS_MASK) == D64_RS0_RS_STOPPED);
}

static bool
BCMATTACHFN_DMA_ATTACH(dma64_alloc)(dma_info_t *di, uint direction)
{
	uint32 size;
	uint ddlen;
	void *va;
	uint alloced = 0;
	uint32 align;
	uint16 align_bits;

	ddlen = sizeof(dma64dd_t);

	size = (direction == DMA_TX) ? (di->ntxd * ddlen) : (di->nrxd * ddlen);
	align_bits = di->dmadesc_align;
	align = (1 << align_bits);

	if (direction == DMA_TX) {
		if ((va = dma_ringalloc(di->osh,
			(di->d64_xs0_cd_mask == 0x1fff) ? D64RINGBOUNDARY : D64RINGBOUNDARY_LARGE,
			size, &align_bits, &alloced,
			&di->txdpaorig, &di->tx_dmah)) == NULL) {
			DMA_ERROR(("%s: dma64_alloc: DMA_ALLOC_CONSISTENT(ntxd) failed\n",
			           di->name));
			return FALSE;
		}
		align = (1 << align_bits);

		/* adjust the pa by rounding up to the alignment */
		PHYSADDRLOSET(di->txdpa, ROUNDUP(PHYSADDRLO(di->txdpaorig), align));
		PHYSADDRHISET(di->txdpa, PHYSADDRHI(di->txdpaorig));

		/* Make sure that alignment didn't overflow */
		ASSERT(PHYSADDRLO(di->txdpa) >= PHYSADDRLO(di->txdpaorig));

		/* find the alignment offset that was used */
		di->txdalign = (uint)(PHYSADDRLO(di->txdpa) - PHYSADDRLO(di->txdpaorig));

		/* adjust the va by the same offset */
		di->txd64 = (dma64dd_t *)((uintptr)va + di->txdalign);

		di->txdalloc = alloced;
		ASSERT(ISALIGNED(PHYSADDRLO(di->txdpa), align));
	} else {
		if ((va = dma_ringalloc(di->osh,
			(di->d64_rs0_cd_mask == 0x1fff) ? D64RINGBOUNDARY : D64RINGBOUNDARY_LARGE,
			size, &align_bits, &alloced,
			&di->rxdpaorig, &di->rx_dmah)) == NULL) {
			DMA_ERROR(("%s: dma64_alloc: DMA_ALLOC_CONSISTENT(nrxd) failed\n",
			           di->name));
			return FALSE;
		}
		align = (1 << align_bits);

		/* adjust the pa by rounding up to the alignment */
		PHYSADDRLOSET(di->rxdpa, ROUNDUP(PHYSADDRLO(di->rxdpaorig), align));
		PHYSADDRHISET(di->rxdpa, PHYSADDRHI(di->rxdpaorig));

		/* Make sure that alignment didn't overflow */
		ASSERT(PHYSADDRLO(di->rxdpa) >= PHYSADDRLO(di->rxdpaorig));

		/* find the alignment offset that was used */
		di->rxdalign = (uint)(PHYSADDRLO(di->rxdpa) - PHYSADDRLO(di->rxdpaorig));

		/* adjust the va by the same offset */
		di->rxd64 = (dma64dd_t *)((uintptr)va + di->rxdalign);

		di->rxdalloc = alloced;
		ASSERT(ISALIGNED(PHYSADDRLO(di->rxdpa), align));
	}

	return TRUE;
}

static bool
dma64_txreset(dma_info_t *di)
{
	uint32 status = D64_XS0_XS_DISABLED;

	if (di->ntxd == 0)
		return TRUE;

	/* if using indirect DMA access, then configure IndQSel */
	/* If the DMA core has resetted, then the default IndQSel = 0 */
	/* So force the configuration */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, TRUE);
	} else {
		/* If DMA is already in reset, do not reset. */
		if ((R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_XS_MASK) ==
			D64_XS0_XS_DISABLED)
			return TRUE;

		/* suspend tx DMA first */
		W_REG(di->osh, &di->d64txregs->control, D64_XC_SE);
		SPINWAIT(((status = (R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_XS_MASK)) !=
		          D64_XS0_XS_DISABLED) &&
		         (status != D64_XS0_XS_IDLE) &&
		         (status != D64_XS0_XS_STOPPED),
		         10000);
	}

	/* For IndDMA, the channel status is ignored. */
	W_REG(di->osh, &di->d64txregs->control, 0);

	if (!DMA_INDIRECT(di) || (di->hnddma.dmactrlflags & DMA_CTRL_DESC_ONLY_FLAG)) {
		SPINWAIT(((status = (R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_XS_MASK)) !=
		          D64_XS0_XS_DISABLED),
		         10000);

		/* We should be disabled at this point */
		if (status != D64_XS0_XS_DISABLED) {
			DMA_ERROR(("%s: status != D64_XS0_XS_DISABLED 0x%x\n",
					__FUNCTION__, status));
			ASSERT(status == D64_XS0_XS_DISABLED);
			OSL_DELAY(300);
		}
	}

	return (status == D64_XS0_XS_DISABLED);
}

static bool
dma64_rxcheckidlestatus(dma_info_t *di)
{
	if (di->nrxd == 0) {
		return TRUE;
	}

	/* Ensure that Rx rcvstatus is in Idle Wait */
	if (R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_RS_IDLE) {
		return TRUE;
	}
	return FALSE;
}

static bool
dma64_rxreset(dma_info_t *di)
{
	uint32 status;

	if (di->nrxd == 0)
		return TRUE;

	W_REG(di->osh, &di->d64rxregs->control, 0);
	SPINWAIT(((status = (R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_RS_MASK)) !=
	          D64_RS0_RS_DISABLED), 10000);

	return (status == D64_RS0_RS_DISABLED);
}

static bool
dma64_txsuspendedidle(dma_info_t *di)
{

	if (di->ntxd == 0)
		return TRUE;

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	if (!(R_REG(di->osh, &di->d64txregs->control) & D64_XC_SE))
		return 0;

	if ((R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_XS_MASK) == D64_XS0_XS_IDLE)
		return 1;

	return 0;
}

/**
 * Useful when sending unframed data.  This allows us to get a progress report from the DMA.
 * We return a pointer to the beginning of the data buffer of the current descriptor.
 * If DMA is idle, we return NULL.
 */
/* Might be nice if DMA HW could tell us current position rather than current descriptor */
static void*
dma64_getpos(dma_info_t *di, bool direction)
{
	void *va;
	bool idle;
	uint16 cur_idx;

	if (direction == DMA_TX) {
		/* if using indirect DMA access, then configure IndQSel */
		if (DMA_INDIRECT(di)) {
			dma_set_indqsel((hnddma_t *)di, FALSE);
		}

		cur_idx = B2I(((R_REG(di->osh, &di->d64txregs->status0) & D64_XS0_CD_MASK) -
		               di->xmtptrbase) & D64_XS0_CD_MASK, dma64dd_t);
		idle = !NTXDACTIVE(di->txin, di->txout);
		va = di->txp[cur_idx];
	} else {
		cur_idx = B2I(((R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_CD_MASK) -
		               di->rcvptrbase) & D64_RS0_CD_MASK, dma64dd_t);
		idle = !NRXDACTIVE(di->rxin, di->rxout);
		va = di->rxp[cur_idx];
	}

	/* If DMA is IDLE, return NULL */
	if (idle) {
		DMA_TRACE(("%s: DMA idle, return NULL\n", __FUNCTION__));
		va = NULL;
	}

	return va;
}

/**
 * TX of unframed data
 *
 * Adds a DMA ring descriptor for the data pointed to by "buf".
 * This is for DMA of a buffer of data and is unlike other hnddma TX functions
 * that take a pointer to a "packet"
 * Each call to this is results in a single descriptor being added for "len" bytes of
 * data starting at "buf", it doesn't handle chained buffers.
 */
static int
dma64_txunframed(dma_info_t *di, void *buf, uint len, bool commit)
{
	uint16 txout;
	uint32 flags = 0;
	dmaaddr_t pa; /* phys addr */

	txout = di->txout;

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	/* return nonzero if out of tx descriptors */
	if (NEXTTXD(txout) == di->txin)
		goto outoftxd;

	if (len == 0)
		return 0;

#ifdef BCM_SECURE_DMA
	pa = SECURE_DMA_MAP(di->osh, buf, len, DMA_TX, NULL, NULL, &di->sec_cma_info_tx, 0);
#else
	pa = DMA_MAP(di->osh, buf, len, DMA_TX, NULL, &di->txp_dmah[txout]);
#endif /* BCM_SECURE_DMA */

	flags = (D64_CTRL1_SOF | D64_CTRL1_IOC | D64_CTRL1_EOF);

	if (txout == (di->ntxd - 1))
		flags |= D64_CTRL1_EOT;

	dma64_dd_upd(di, di->txd64, pa, txout, &flags, len);
	ASSERT(di->txp[txout] == NULL);

#if defined(BULK_DESCR_FLUSH)
	DMA_MAP(di->osh,  dma64_txd64(di, di->txout), DMA64_FLUSH_LEN(1),
		DMA_TX, NULL, NULL);
#endif /* BULK_DESCR_FLUSH */

	/* save the buffer pointer - used by dma_getpos */
	di->txp[txout] = buf;

	txout = NEXTTXD(txout);
	/* bump the tx descriptor index */
	di->txout = txout;

	/* kick the chip */
	if (commit) {
		dma64_txcommit_local(di);
	}

	/* tx flow control */
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return (0);

outoftxd:
	DMA_ERROR(("%s: %s: out of txds !!!\n", di->name, __FUNCTION__));
	di->hnddma.txavail = 0;
	di->hnddma.txnobuf++;
	return (-1);
}


/**
 * RX of unframed data
 *
 * Adds a DMA ring descriptor for the data pointed to by "buf".
 * This is for DMA of a buffer of data and is unlike other hnddma RX functions
 * that take a pointer to a "packet"
 * Each call to this is result in a single descriptor being added for "len" bytes of
 * data starting at "buf", it doesn't handle chained buffers.
 */
int
dma_rxfill_unframed(hnddma_t *dmah, void *buf, uint len, bool commit)
{
	uint16 rxout;
#if !defined(BCM_SECURE_DMA)
	uint32 flags = 0;
	dmaaddr_t pa; /* phys addr */
#endif /* !BCM_SECURE_DMA */
	dma_info_t *di = DI_INFO(dmah);

	rxout = di->rxout;

	/* return nonzero if out of rx descriptors */
	if (NEXTRXD(rxout) == di->rxin)
		goto outofrxd;

	ASSERT(len <= di->rxbufsize);
#if !defined(BCM_SECURE_DMA)
	/* cache invalidate maximum buffer length */
	pa = DMA_MAP(di->osh, buf, di->rxbufsize, DMA_RX, NULL, NULL);
	if (rxout == (di->nrxd - 1))
		flags |= D64_CTRL1_EOT;

	dma64_dd_upd(di, di->rxd64, pa, rxout, &flags, len);
	ASSERT(di->rxp[rxout] == NULL);
#endif /* #if !defined(BCM_SECURE_DMA) */

#if defined(BULK_DESCR_FLUSH)
	DMA_MAP(di->osh, dma64_rxd64(di, di->rxout), DMA64_FLUSH_LEN(1),
		DMA_TX, NULL, NULL);
#endif /* BULK_DESCR_FLUSH */

	/* save the buffer pointer - used by dma_getpos */
	di->rxp[rxout] = buf;

	rxout = NEXTRXD(rxout);

	/* kick the chip */
	if (commit) {
		W_REG(di->osh, &di->d64rxregs->ptr, di->rcvptrbase + I2B(rxout, dma64dd_t));
	}

	/* bump the rx descriptor index */
	di->rxout = rxout;

	/* rx flow control */
	di->rxavail = di->nrxd - NRXDACTIVE(di->rxin, di->rxout) - 1;

	return (0);

outofrxd:
	DMA_ERROR(("%s: %s: out of rxds !!!\n", di->name, __FUNCTION__));
	di->rxavail = 0;
	di->hnddma.rxnobuf++;
	return (-1);
}

void BCMFASTPATH
dma_clearrxp(hnddma_t *dmah)
{
	uint16 i, curr;
	dma_info_t *di = DI_INFO(dmah);

	i = di->rxin;

	/* return if no packets posted */
	if (i == di->rxout)
		return;

	if (di->rxin == di->rs0cd) {
		curr = (uint16)B2I(((R_REG(di->osh, &di->d64rxregs->status0) & D64_RS0_CD_MASK) -
		di->rcvptrbase) & D64_RS0_CD_MASK, dma64dd_t);
		di->rs0cd = curr;
	} else
		curr = di->rs0cd;

	while (i != curr) {

		ASSERT(di->rxp[i]);
		di->rxp[i] = NULL;

		W_SM(&di->rxd64[i].addrlow, 0xdeadbeef);
		W_SM(&di->rxd64[i].addrhigh, 0xdeadbeef);
		i = NEXTRXD(i);
	}

	di->rxin = i;

	di->rxavail = di->nrxd - NRXDACTIVE(di->rxin, di->rxout) - 1;

}

/* If using indirect DMA interface, then before calling this function the IndQSel must be
 * configured for the DMA channel for which this call is being made.
 */
static bool
BCMATTACHFN_DMA_ATTACH(_dma64_addrext)(osl_t *osh, dma64regs_t *dma64regs)
{
	uint32 w;
	OR_REG(osh, &dma64regs->control, D64_XC_AE);
	w = R_REG(osh, &dma64regs->control);
	AND_REG(osh, &dma64regs->control, ~D64_XC_AE);
	return ((w & D64_XC_AE) == D64_XC_AE);
}

/**
 * Rotate all active tx dma ring entries "forward" by (ActiveDescriptor - txin).
 */
static void
dma64_txrotate(dma_info_t *di)
{
	uint16 ad;
	uint nactive;
	uint rot;
	uint16 old, new;
	uint32 w;
	uint16 first, last;

	ASSERT(dma64_txsuspendedidle(di));

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	nactive = _dma_txactive(di);
	ad = B2I((((R_REG(di->osh, &di->d64txregs->status1) & D64_XS1_AD_MASK)
		- di->xmtptrbase) & D64_XS1_AD_MASK), dma64dd_t);
	rot = TXD(ad - di->txin);

	ASSERT(rot < di->ntxd);

	/* full-ring case is a lot harder - don't worry about this */
	if (rot >= (di->ntxd - nactive)) {
		DMA_ERROR(("%s: dma_txrotate: ring full - punt\n", di->name));
		return;
	}

	first = di->txin;
	last = PREVTXD(di->txout);

	/* move entries starting at last and moving backwards to first */
	for (old = last; old != PREVTXD(first); old = PREVTXD(old)) {
		new = TXD(old + rot);

		/*
		 * Move the tx dma descriptor.
		 * EOT is set only in the last entry in the ring.
		 */
		w = BUS_SWAP32(R_SM(&di->txd64[old].ctrl1)) & ~D64_CTRL1_EOT;
		if (new == (di->ntxd - 1))
			w |= D64_CTRL1_EOT;
		W_SM(&di->txd64[new].ctrl1, BUS_SWAP32(w));

		w = BUS_SWAP32(R_SM(&di->txd64[old].ctrl2));
		W_SM(&di->txd64[new].ctrl2, BUS_SWAP32(w));

		W_SM(&di->txd64[new].addrlow, R_SM(&di->txd64[old].addrlow));
		W_SM(&di->txd64[new].addrhigh, R_SM(&di->txd64[old].addrhigh));

#if defined(DESCR_DEADBEEF)
		/* zap the old tx dma descriptor address field */
		W_SM(&di->txd64[old].addrlow, BUS_SWAP32(0xdeadbeef));
		W_SM(&di->txd64[old].addrhigh, BUS_SWAP32(0xdeadbeef));
#endif /* DESCR_DEADBEEF */

		/* move the corresponding txp[] entry */
		ASSERT(di->txp[new] == NULL);
		di->txp[new] = di->txp[old];

		/* Move the map */
		if (DMASGLIST_ENAB) {
			bcopy(&di->txp_dmah[old], &di->txp_dmah[new], sizeof(hnddma_seg_map_t));
			bzero(&di->txp_dmah[old], sizeof(hnddma_seg_map_t));
		}

		di->txp[old] = NULL;
	}

	/* update txin and txout */
	di->txin = ad;
	di->txout = TXD(di->txout + rot);
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

#if (defined(__ARM_ARCH_7A__) && defined(CA7)) || defined(STB)
	/* memory barrier before posting the descriptor */
	DMB();
#endif
	/* kick the chip */
	dma64_txcommit_local(di);
}

uint
BCMATTACHFN_DMA_ATTACH(dma_addrwidth)(si_t *sih, void *dmaregs)
{
	dma32regs_t *dma32regs;
	osl_t *osh;

	osh = si_osh(sih);

	/* Perform 64-bit checks only if we want to advertise 64-bit (> 32bit) capability) */
	/* DMA engine is 64-bit capable */
	if ((si_core_sflags(sih, 0, 0) & SISF_DMA64) == SISF_DMA64) {
		/* backplane are 64-bit capable */
		if (si_backplane64(sih))
			/* If bus is System Backplane or PCIE then we can access 64-bits */
			if ((BUSTYPE(sih->bustype) == SI_BUS) ||
			    ((BUSTYPE(sih->bustype) == PCI_BUS) &&
			     ((BUSCORETYPE(sih->buscoretype) == PCIE_CORE_ID) ||
			      (BUSCORETYPE(sih->buscoretype) == PCIE2_CORE_ID))))
				return (DMADDRWIDTH_64);

		/* DMA64 is always 32-bit capable, AE is always TRUE */
		ASSERT(_dma64_addrext(osh, (dma64regs_t *)dmaregs));

		return (DMADDRWIDTH_32);
	}

	/* Start checking for 32-bit / 30-bit addressing */
	dma32regs = (dma32regs_t *)dmaregs;

	/* For System Backplane, PCIE bus or addrext feature, 32-bits ok */
	if ((BUSTYPE(sih->bustype) == SI_BUS) ||
	    ((BUSTYPE(sih->bustype) == PCI_BUS) &&
	     ((BUSCORETYPE(sih->buscoretype) == PCIE_CORE_ID) ||
	      (BUSCORETYPE(sih->buscoretype) == PCIE2_CORE_ID))) ||
	    (_dma32_addrext(osh, dma32regs)))
		return (DMADDRWIDTH_32);

	/* Fallthru */
	return (DMADDRWIDTH_30);
}

#ifdef BCMPKTPOOL
static int
_dma_pktpool_set(dma_info_t *di, pktpool_t *pool)
{
	ASSERT(di);
	ASSERT(di->pktpool == NULL);
	di->pktpool = pool;
	return 0;
}
#endif

pktpool_t*
dma_pktpool_get(hnddma_t *dmah)
{
	dma_info_t * di = DI_INFO(dmah);
	ASSERT(di);
	ASSERT(di->pktpool != NULL);
	return (di->pktpool);
}

static bool
_dma_rxtx_error(dma_info_t *di, bool istx)
{
	uint32 status1 = 0;
	uint16 curr;

	if (DMA64_ENAB(di) && DMA64_MODE(di)) {

		if (istx) {

			/* if using indirect DMA access, then configure IndQSel */
			if (DMA_INDIRECT(di)) {
				dma_set_indqsel((hnddma_t *)di, FALSE);
			}

			status1 = R_REG(di->osh, &di->d64txregs->status1);

			if ((status1 & D64_XS1_XE_MASK) != D64_XS1_XE_NOERR)
				return TRUE;
			else if (si_coreid(di->sih) == GMAC_CORE_ID && si_corerev(di->sih) >= 4) {
				curr = (uint16)(B2I(((R_REG(di->osh, &di->d64txregs->status0) &
					D64_XS0_CD_MASK) - di->xmtptrbase) &
					D64_XS0_CD_MASK, dma64dd_t));

				if (NTXDACTIVE(di->txin, di->txout) != 0 &&
					curr == di->xs0cd_snapshot) {

					/* suspicious */
					return TRUE;
				}
				di->xs0cd_snapshot = di->xs0cd = curr;

				return FALSE;
			}
			else
				return FALSE;
		}
		else {

			status1 = R_REG(di->osh, &di->d64rxregs->status1);

			if ((status1 & D64_RS1_RE_MASK) != D64_RS1_RE_NOERR)
				return TRUE;
			else
				return FALSE;
		}

	} else if (DMA32_ENAB(di)) {
		return FALSE;

	} else {
		ASSERT(0);
		return FALSE;
	}

}

void
_dma_burstlen_set(dma_info_t *di, uint8 rxburstlen, uint8 txburstlen)
{
	di->rxburstlen = rxburstlen;
	di->txburstlen = txburstlen;
}

static void
dma_param_set_nrxpost(dma_info_t *di, uint16 paramval)
{
	uint nrxd_max_usable;
	uint nrxpost_max;

	/* Maximum no.of desc that can be active is "nrxd - 1" */
	nrxd_max_usable = (di->nrxd - 1);

	if (di->sep_rxhdr) {
		nrxpost_max = nrxd_max_usable/2;
	} else {
		nrxpost_max = nrxd_max_usable;
	}

	di->nrxpost = MIN(paramval, nrxpost_max);
}

void
_dma_param_set(dma_info_t *di, uint16 paramid, uint16 paramval)
{
	switch (paramid) {
	case HNDDMA_PID_TX_MULTI_OUTSTD_RD:
		di->txmultioutstdrd = (uint8)paramval;
		break;

	case HNDDMA_PID_TX_PREFETCH_CTL:
		di->txprefetchctl = (uint8)paramval;
		break;

	case HNDDMA_PID_TX_PREFETCH_THRESH:
		di->txprefetchthresh = (uint8)paramval;
		break;

	case HNDDMA_PID_TX_BURSTLEN:
		di->txburstlen = (uint8)paramval;
		break;

	case HNDDMA_PID_RX_PREFETCH_CTL:
		di->rxprefetchctl = (uint8)paramval;
		break;

	case HNDDMA_PID_RX_PREFETCH_THRESH:
		di->rxprefetchthresh = (uint8)paramval;
		break;

	case HNDDMA_PID_RX_BURSTLEN:
		di->rxburstlen = (uint8)paramval;
		break;

#ifndef DMA_WAIT_COMPLT_ROM_COMPAT
	case HNDDMA_PID_RX_WAIT_CMPL:
		di->rxwaitforcomplt = (uint8)paramval;
		break;
#endif

	case HNDDMA_PID_BURSTLEN_CAP:
		di->burstsize_ctrl = (uint8)paramval;
		break;

#if defined(D11_SPLIT_RX_FD)
	case HNDDMA_SEP_RX_HDR:
		di->sep_rxhdr = (uint8)paramval;	/* indicate sep hdr descriptor is used */
		break;
#endif /* D11_SPLIT_RX_FD */

	case HNDDMA_SPLIT_FIFO :
		di->split_fifo = (uint8)paramval;
		break;

	case HNDDMA_PID_D11RX_WAR:
		di->d11rx_war = (uint8)paramval;
		break;
	case HNDDMA_NRXPOST:
		dma_param_set_nrxpost(di, paramval);
		break;
	case HNDDMA_PID_TX_CHAN_SWITCH:
		di->txchanswitch = (uint8)paramval;
		break;

	default:
		DMA_ERROR(("%s: _dma_param_set: invalid paramid %d\n", di->name, paramid));
		ASSERT(0);
		break;
	}
}

static bool
_dma_glom_enable(dma_info_t *di, uint32 val)
{
	dma64regs_t *dregs = di->d64rxregs;
	bool ret = TRUE;

	di->hnddma.dmactrlflags &= ~DMA_CTRL_SDIO_RXGLOM;
	if (val) {
		OR_REG(di->osh, &dregs->control, D64_RC_GE);
		if (!(R_REG(di->osh, &dregs->control) & D64_RC_GE))
			ret = FALSE;
		else
			di->hnddma.dmactrlflags |= DMA_CTRL_SDIO_RXGLOM;
	} else {
		AND_REG(di->osh, &dregs->control, ~D64_RC_GE);
	}
	return ret;
}

void
_dma_context(dma_info_t *di, setup_context_t fn, void *ctx)
{
	di->fn = fn;
	di->ctx = ctx;
}

int BCMFASTPATH
dma_rxfast(hnddma_t *dmah, dma64addr_t p, uint32 len)
{
	uint16 rxout;
	uint32 flags = 0;

	dma_info_t * di = DI_INFO(dmah);
	/*
	 * Determine how many receive buffers we're lacking
	 * from the full complement, allocate, initialize,
	 * and post them, then update the chip rx lastdscr.
	 */

	rxout = di->rxout;

	if ((di->nrxd - NRXDACTIVE(di->rxin, di->rxout) - 1) < 1)
		goto outofrxd;

	/* reset flags for each descriptor */
	if (rxout == (di->nrxd - 1))
		flags = D64_CTRL1_EOT;

	/* Update descriptor */
	dma64_dd_upd_64_from_params(di, di->rxd64, p, rxout, &flags, len);

	di->rxp[rxout] = (void *)(uintptr)(p.loaddr);

	rxout = NEXTRXD(rxout);

	di->rxout = rxout;

	di->rxavail = di->nrxd - NRXDACTIVE(di->rxin, di->rxout) - 1;
	/* update the chip lastdscr pointer */
	W_REG(di->osh, &di->d64rxregs->ptr, di->rcvptrbase + I2B(rxout, dma64dd_t));
	return 0;
outofrxd:
	di->rxavail = 0;
	DMA_ERROR(("%s: dma_rxfast: out of rxds\n", di->name));
	return -1;
}

int BCMFASTPATH
dma_msgbuf_txfast(hnddma_t *dmah, dma64addr_t p0, bool commit, uint32 len, bool first, bool last)
{
	uint16 txout;
	uint32 flags = 0;
	dma_info_t * di = DI_INFO(dmah);

	DMA_TRACE(("%s: dma_txfast\n", di->name));

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	txout = di->txout;

	/* return nonzero if out of tx descriptors */
	if (NEXTTXD(txout) == di->txin)
		goto outoftxd;

	if (len == 0)
		return 0;

	if (first)
		flags |= D64_CTRL1_SOF;
	if (last)
		flags |= D64_CTRL1_EOF | D64_CTRL1_IOC;

	if (txout == (di->ntxd - 1))
		flags |= D64_CTRL1_EOT;

	dma64_dd_upd_64_from_params(di, di->txd64, p0, txout, &flags, len);

	ASSERT(di->txp[txout] == NULL);

	txout = NEXTTXD(txout);

	/* return nonzero if out of tx descriptors */
	if (txout == di->txin) {
		DMA_ERROR(("%s: dma_txfast: Out-of-DMA descriptors"
			   " (txin %d txout %d)\n", __FUNCTION__,
			   di->txin, di->txout));
		goto outoftxd;
	}

	/* save the packet */
	di->txp[PREVTXD(txout)] = (void *)(uintptr)(p0.loaddr);

	/* bump the tx descriptor index */
	di->txout = txout;

	/* kick the chip */
	if (commit) {
		dma64_txcommit_local(di);
	}

	/* tx flow control */
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return (0);

outoftxd:
	DMA_ERROR(("%s: dma_txfast: out of txds !!!\n", di->name));
	di->hnddma.txavail = 0;
	di->hnddma.txnobuf++;
	return (-1);
}

void
dma_link_handle(hnddma_t *dmah1, hnddma_t *dmah2)
{
	dma_info_t *di1 = DI_INFO(dmah1);
	dma_info_t *di2 = DI_INFO(dmah2);
	di1->linked_di = di2;
	di2->linked_di = di1;
}

void
dma_rxchan_reset(hnddma_t *dmah)
{
	dma_info_t *di = DI_INFO(dmah);
	uint size;

	AND_REG(di->osh, &di->d64rxregs->control, ~D64_RC_RE);
	OR_REG(di->osh, &di->d64rxregs->control, D64_RC_RE);

	size = di->nrxd * sizeof(void *);
	bzero(di->rxp, size);
}

void
dma_txchan_reset(hnddma_t *dmah)
{
	dma_info_t *di = DI_INFO(dmah);
	uint size;

	AND_REG(di->osh, &di->d64txregs->control, ~D64_XC_XE);
	OR_REG(di->osh, &di->d64txregs->control, D64_XC_XE);

	size = di->ntxd * sizeof(void *);
	bzero(di->txp, size);
}
