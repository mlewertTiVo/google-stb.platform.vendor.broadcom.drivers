/*
 * OTP support.
 *
 * Copyright (C) 2016, Broadcom Corporation. All Rights Reserved.
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
 * $Id: bcmotp.h 461508 2014-03-12 09:37:07Z $
 */

#ifndef	_bcmotp_h_
#define	_bcmotp_h_

/* OTP regions */
#define OTP_HW_RGN	1
#define OTP_SW_RGN	2
#define OTP_CI_RGN	4
#define OTP_FUSE_RGN	8
#define OTP_ALL_RGN	0xf	/* From h/w region to end of OTP including checksum */

/* OTP Size */
#define OTP_SZ_MAX		(6144/8)	/* maximum bytes in one CIS */

/* Fixed size subregions sizes in words */
#define OTPGU_CI_SZ		2

/* OTP usage */
#define OTP4325_FM_DISABLED_OFFSET	188


/* Exported functions */
extern int	otp_status(void *oh);
extern int	otp_size(void *oh);
extern uint16	otp_read_bit(void *oh, uint offset);
extern void*	otp_init(si_t *sih);
#if !defined(BCMDONGLEHOST)
extern int	otp_newcis(void *oh);
extern int	otp_read_region(si_t *sih, int region, uint16 *data, uint *wlen);
extern int	otp_read_word(si_t *sih, uint wn, uint16 *data);
extern int	otp_nvread(void *oh, char *data, uint *len);
#endif /* !defined(BCMDONGLEHOST) */



#endif /* _bcmotp_h_ */
