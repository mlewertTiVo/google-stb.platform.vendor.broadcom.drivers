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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: hndd11.c 615664 2016-01-28 08:21:07Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <d11.h>
#include <hndd11.h>
#include <bcmendian.h>

static uint16
hndd11_read_objmem16(osl_t *osh, d11regs_t *regs, uint offset, uint32 sel)
{
	volatile uint16* objdata_lo;
	volatile uint16* objdata_hi;
	uint16 v;

	objdata_lo = (volatile uint16*)(uintptr)&regs->objdata;
	objdata_hi = objdata_lo + 1;

	W_REG(osh, &regs->objaddr, sel | (offset >> 2));
	(void)R_REG(osh, &regs->objaddr);
	if (offset & 2) {
		v = R_REG(osh, objdata_hi);
	} else {
		v = R_REG(osh, objdata_lo);
	}

	return v;
}

static void
hndd11_write_objmem16(osl_t *osh, d11regs_t *regs, uint offset, uint16 v, uint32 sel)
{
	volatile uint16* objdata_lo;
	volatile uint16* objdata_hi;

	ASSERT(regs != NULL);
	ASSERT((offset & 1) == 0);

	objdata_lo = (volatile uint16*)(uintptr)&regs->objdata;
	objdata_hi = objdata_lo + 1;

	W_REG(osh, &regs->objaddr, sel | (offset >> 2));
	(void)R_REG(osh, &regs->objaddr);
	if (offset & 2) {
		W_REG(osh, objdata_hi, v);
	} else {
		W_REG(osh, objdata_lo, v);
	}
}

/**
 * Copy a piece of shared memory of type SHM to a buffer.
 * SHM 'offset' needs to be an even address and
 * Buffer length 'len' must be an even number of bytes
 */
void
hndd11_copyfrom_shm(si_t *sih, uint coreunit, uint offset, void* buf, int len)
{
	osl_t *osh = si_osh(sih);
	uint origidx = 0;
	d11regs_t *regs = NULL;
	uint8* p = (uint8*)buf;
	int i;
	uint16 v16;

	/* offset and len need to be even */
	ASSERT((offset & 1) == 0);
	ASSERT((len & 1) == 0);

	if (len <= 0)
		goto exit;

	origidx = si_coreidx(sih);
	regs = si_setcore(sih,  D11_CORE_ID, coreunit);

	if (regs == NULL)
		goto exit;

	for (i = 0; i < len; i += 2) {
		v16 = ltoh16(hndd11_read_objmem16(osh, regs, offset + i, OBJADDR_SHM_SEL));
		p[i] = v16 & 0xFF;
		p[i+1] = (v16 >> 8) & 0xFF;
	}
	si_setcoreidx(sih, origidx);

exit:
	return;
}

void
hndd11_read_shm(si_t *sih, uint coreunit, uint offset, void* buf)
{
	hndd11_copyfrom_shm(sih, coreunit, offset, buf, 2);
}

void
hndd11_write_shm(si_t *sih, uint coreunit, uint offset, const void* buf)
{
	hndd11_copyto_shm(sih, coreunit, offset, buf, 2);
}

/**
 * Copy a buffer to shared memory of type SHM.
 * SHM 'offset' needs to be an even address and
 * Buffer length 'len' must be an even number of bytes
 */
void
hndd11_copyto_shm(si_t *sih, uint coreunit, uint offset, const void* buf, int len)
{
	osl_t *osh = si_osh(sih);
	uint origidx = 0;
	d11regs_t *regs = NULL;
	const uint8* p = (const uint8*)buf;
	int i;
	uint16 v16;

	/* offset and len need to be even */
	ASSERT((offset & 1) == 0);
	ASSERT((len & 1) == 0);

	if (len <= 0)
		return;

	origidx = si_coreidx(sih);

	regs = si_setcore(sih,  D11_CORE_ID, coreunit);

	for (i = 0; i < len; i += 2) {
		v16 = htol16(p[i] | (p[i+1] << 8));
		hndd11_write_objmem16(osh, regs, offset + i, v16, OBJADDR_SHM_SEL);
	}
	si_setcoreidx(sih, origidx);
}

uint32
hndd11_write_reg32(osl_t *osh, volatile uint32 *reg, uint32 mask, uint32 val)
{
	uint32 wval = 0;
	HNDD11_DBG(("%s: writing r:%p,mask:%x,val:%x",
			__FUNCTION__, reg, mask, val));
	wval = (R_REG(osh, reg) & ~mask) | val;

	W_REG(osh, reg, wval);

	return R_REG(osh, reg);
}

uint32
hndd11_bm_read(osl_t *osh, d11regs_t *regs, uint32 offset,
		uint32 len, uint32 *buf)
{
	uint32 i = 0;
	ASSERT(ISALIGNED(offset, sizeof(uint32)));

	/* program base */
	W_REG(osh, &regs->tplatewrptr, offset);

	/* populate BM from base, ptr will auto increment */
	for (i = 0; i < len; i++, buf++) {
		*buf = R_REG(osh, &regs->tplatewrdata);
	}

	return (uint32)R_REG(osh, &regs->tplatewrptr);
}

uint32
hndd11_bm_write(osl_t *osh, d11regs_t *regs, uint32 offset,
		uint32 len, const uint32 *buf)
{
	uint32 i = 0;
	ASSERT(ISALIGNED(offset, sizeof(uint32)));

	/* program base */
	W_REG(osh, &regs->tplatewrptr, offset);

	/* populate BM from base, ptr will auto increment */
	for (i = 0; i < len; i++, buf++) {
		W_REG(osh, &regs->tplatewrdata, *buf);
	}

	return (uint32)R_REG(osh, &regs->tplatewrptr);
}

#if defined(HNDD11_DBG_BMDUMP) && HNDD11_DBG_BMDUMP
void
hndd11_bm_dump(osl_t *osh, d11regs_t *regs, uint32 offset, uint32 len)
{
	uint32 i = 0;

	HNDD11_DBG(("%s:\nDumping BM from offsets : 0x%08x to 0x%08x\n",
		__FUNCTION__, offset, offset + ((len - 1) * sizeof(uint32))));

	W_REG(osh, &regs->tplatewrptr, offset);

	for (i = 0; i < len; i++) {
		FCBS_DBG(("%02d | 0x%08x : 0x%08x\n", i,
			R_REG(osh, &regs->tplatewrptr),
			R_REG(osh, &regs->tplatewrdata)));
	}
	HNDD11_DBG(("\n"));
}
#else
void hndd11_bm_dump(osl_t *osh, d11regs_t *regs, uint32 offset,
		uint32 len) {}
#endif /* HNDD11_DBG_BMDUMP */
