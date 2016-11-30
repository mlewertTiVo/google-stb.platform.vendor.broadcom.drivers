/*
 * Header file for splitrx mode definitions
 * Explains different splitrx modes, macros for classify, conversion.
 * splitrx twiki: http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/SplitRXModes
 * header conversion twiki: http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/Dot11MacHdrConversion
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 * $Id$
 */


#ifndef _d11_cfg_h_
#define _d11_cfg_h_
#define	RXMODE1	1	/* descriptor split */
#define	RXMODE2	2	/* descriptor split + classification */
#define	RXMODE3	3	/* fifo split + classification */
#define	RXMODE4	4	/* fifo split + classification + hdr conversion */

#ifdef BCMSPLITRX /* BCMLFRAG support enab macros  */
	extern bool _bcmsplitrx;
	extern uint8 _bcmsplitrx_mode;
	#if defined(WL_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define BCMSPLITRX_ENAB() (_bcmsplitrx)
		#define BCMSPLITRX_MODE() (_bcmsplitrx_mode)
	#elif defined(BCMSPLITRX_DISABLED)
		#define BCMSPLITRX_ENAB()	(0)
		#define BCMSPLITRX_MODE()	(0)
	#else
		#define BCMSPLITRX_ENAB()	(1)
		#define BCMSPLITRX_MODE() (_bcmsplitrx_mode)
	#endif
#else
	#define BCMSPLITRX_ENAB()		(0)
	#define BCMSPLITRX_MODE()		(0)
#endif /* BCMSPLITRX */


#define SPLIT_RXMODE1()	((BCMSPLITRX_MODE() == RXMODE1))
#define SPLIT_RXMODE2()	((BCMSPLITRX_MODE() == RXMODE2))
#define SPLIT_RXMODE3()	((BCMSPLITRX_MODE() == RXMODE3))
#define SPLIT_RXMODE4()	((BCMSPLITRX_MODE() == RXMODE4))

#define PKT_CLASSIFY()	(SPLIT_RXMODE2() || SPLIT_RXMODE3() || SPLIT_RXMODE4())
#define RXFIFO_SPLIT()	(SPLIT_RXMODE3() || SPLIT_RXMODE4())
#define HDR_CONV()	(SPLIT_RXMODE4())

#define SPLITRX_DYN_MODE3(osh, p)	(RXFIFO_SPLIT() && !PKTISHDRCONVTD(osh, p))
#define SPLITRX_DYN_MODE4(osh, p)	(PKTISHDRCONVTD(osh, p))
#define PKT_CLASSIFY_EN(x)	((PKT_CLASSIFY()) && (PKT_CLASSIFY_FIFO == (x)))


#endif /* _d11_cfg_h_ */
