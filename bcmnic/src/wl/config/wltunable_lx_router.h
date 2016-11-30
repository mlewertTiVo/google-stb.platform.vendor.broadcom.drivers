/*
 * Broadcom 802.11abg Networking Device Driver Configuration file
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
 * $Id: wltunable_lx_router.h 630774 2016-04-12 02:09:48Z $
 *
 * wl driver tunables
 */

#define D11CONF		0x77800000	/* D11 Core Rev
					 * 23 (43224b0), 24 (4313), 25 (5357a0), 26 (4331a0),
					 * 28 (5357b0), 29 (4331B0), 30(43228).
					 */
#define D11CONF2	0x1870500		/* D11 Core Rev > 31, Rev 40(4360a0),
					 * 42(4360B0), 48(4354), 49(43602a0) 50(4349a2),55(4349b0),
					 * 56(53573)
					 */

#define NRXBUFPOST	56	/* # rx buffers posted */
#define RXBND		24	/* max # rx frames to process */
#define PKTCBND		36	/* max # rx frames to chain */
#ifdef __ARM_ARCH_7A__
#define CTFPOOLSZ       512	/* max buffers in ctfpool */
#else
#define CTFPOOLSZ       192	/* max buffers in ctfpool */
#endif

#define WME_PER_AC_TX_PARAMS 1
#define WME_PER_AC_TUNING 1

#define NTXD_AC3X3		512	/* TX descriptor ring */
#define NRXD_AC3X3		512	/* RX descriptor ring */
#define NTXD_LARGE_AC3X3	2048	/* TX descriptor ring */
#define NRXD_LARGE_AC3X3	2048	/* RX descriptor ring */
#define NRXBUFPOST_AC3X3	500	/* # rx buffers posted */
#define RXBND_AC3X3		36	/* max # rx frames to process */
#ifdef __ARM_ARCH_7A__
#define CTFPOOLSZ_AC3X3		1024	/* max buffers in ctfpool */
#else
#define CTFPOOLSZ_AC3X3		512	/* max buffers in ctfpool */
#endif /* ! __ARM_ARCH_7A__ */
#define PKTCBND_AC3X3		48	/* max # rx frames to chain */

#define TXMR			2	/* number of outstanding reads */
#define TXPREFTHRESH		8	/* prefetch threshold */
#define TXPREFCTL		16	/* max descr allowed in prefetch request */
#define TXBURSTLEN		256	/* burst length for dma reads */

#define RXPREFTHRESH		1	/* prefetch threshold */
#define RXPREFCTL		8	/* max descr allowed in prefetch request */
#define RXBURSTLEN		256	/* burst length for dma writes */

#define MRRS			512	/* Max read request size */

/* AC2 settings */
#define TXMR_AC2		12	/* number of outstanding reads */
#define TXPREFTHRESH_AC2	8	/* prefetch threshold */
#define TXPREFCTL_AC2		16	/* max descr allowed in prefetch request */
#define TXBURSTLEN_AC2		1024	/* burst length for dma reads */
#define RXPREFTHRESH_AC2	8	/* prefetch threshold */
#define RXPREFCTL_AC2		16	/* max descr allowed in prefetch request */
#define RXBURSTLEN_AC2		128	/* burst length for dma writes */
#define MRRS_AC2		1024	/* Max read request size */
/* AC2 settings */

#define AMPDU_PKTQ_LEN		1536
#define AMPDU_PKTQ_FAVORED_LEN  4096

#define WLRXEXTHDROOM -1        /* to reserve extra headroom in DMA Rx buffer */
