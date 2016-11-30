/*
 * HND SiliconBackplane MIPS core software interface.
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
 * $Id: hndmips.h 523133 2014-12-27 05:50:30Z $
 */

#ifndef _hndmips_h_
#define _hndmips_h_

#include <typedefs.h>
#include <siutils.h>

extern void si_mips_init(si_t *sih, uint shirq_map_base);
extern bool si_mips_setclock(si_t *sih, uint32 mipsclock, uint32 sbclock, uint32 pciclock);
extern void enable_pfc(uint32 mode);
extern uint32 si_memc_get_ncdl(si_t *sih);


#endif /* _hndmips_h_ */
