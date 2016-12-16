/*
 * Generic functions for d11 access
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: hndd11.h 663980 2016-10-07 19:14:26Z $
 */

#ifndef	_hndd11_h_
#define	_hndd11_h_

#include <typedefs.h>
#include <osl_decl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <d11.h>

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

/* SW RXHDR */
typedef struct wlc_d11rxhdr wlc_d11rxhdr_t;
BWL_PRE_PACKED_STRUCT struct wlc_d11rxhdr {
	/* SW header */
	uint32	tsf_l;		/**< TSF_L reading */
	int8	rssi;		/**< computed instantaneous rssi */
	int8	rssi_qdb;	/**< qdB portion of the computed rssi */
	uint8	pad[2];
	int8	rxpwr[ROUNDUP(WL_RSSI_ANT_MAX,2)];	/**< rssi for supported antennas */
	/**
	 * Even though rxhdr can be in short or long format, always declare it here
	 * to be in long format. So the offsets for the other fields are always the same.
	 */
	d11rxhdr_t rxhdr;
} BWL_POST_PACKED_STRUCT;

#define WLC_RXHDR_LEN	OFFSETOF(wlc_d11rxhdr_t, rxhdr)

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

/* Structure to hold d11 corerev information */
typedef struct d11_info d11_info_t;
struct d11_info {
    uint revid;
};

/* ulp dbg macro */
#define HNDD11_DBG(x)
#define HNDD11_ERR(x) printf x
/* ulp dbg macro */
#define HNDD11_DBG(x)
#define HNDD11_ERR(x) printf x

extern void hndd11_read_shm(si_t *sih, uint coreunit, uint offset, void* buf);
extern void hndd11_write_shm(si_t *sih, uint coreunit, uint offset, const void* buf);

extern void hndd11_copyto_shm(si_t *sih, uint coreunit, uint offset,
		const void* buf, int len);

extern void hndd11_copyfrom_shm(si_t *sih, uint coreunit, uint offset, void* buf, int len);
extern void hndd11_copyto_shm(si_t *sih, uint coreunit, uint offset, const void* buf, int len);

extern uint32 hndd11_write_reg32(osl_t *osh, volatile uint32 *reg, uint32 mask, uint32 val);

extern uint32 hndd11_bm_read(osl_t *osh, d11regs_t *regs, uint32 offset, uint32 len, uint32 *buf);
extern uint32 hndd11_bm_write(osl_t *osh, d11regs_t *regs, uint32 offset, uint32 len,
		const uint32 *buf);
extern void hndd11_bm_dump(osl_t *osh, d11regs_t *regs, uint32 offset, uint32 len);

#endif	/* _hndd11_h_ */
/*
 * Generic functions for d11 access
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
 * $Id: hndd11.h 663980 2016-10-07 19:14:26Z $
 */
