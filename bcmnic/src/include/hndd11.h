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
 * $Id: hndd11.h 615664 2016-01-28 08:21:07Z $
 */

#ifndef	_hndd11_h_
#define	_hndd11_h_

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
 * $Id: hndd11.h 615664 2016-01-28 08:21:07Z $
 */
