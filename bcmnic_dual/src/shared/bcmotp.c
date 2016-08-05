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
 * $Id: bcmotp.c 483586 2014-06-10 11:15:53Z $
 */

#include <bcm_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <hndsoc.h>
#include <sbchipc.h>
#include <bcmotp.h>
#if !defined(_CFEZ_) || defined(CFG_WL)
#include <wlioctl.h>
#endif 

/*
 * There are two different OTP controllers so far:
 * 	1. new IPX OTP controller:	chipc 21, >=23
 * 	2. older HND OTP controller:	chipc 12, 17, 22
 *
 * Define BCMHNDOTP to include support for the HND OTP controller.
 * Define BCMIPXOTP to include support for the IPX OTP controller.
 *
 * NOTE 1: More than one may be defined
 * NOTE 2: If none are defined, the default is to include them all.
 */

#if !defined(BCMHNDOTP) && !defined(BCMIPXOTP)
#define BCMHNDOTP	1
#define BCMIPXOTP	1
#endif

#define OTPTYPE_HND(ccrev)	((ccrev) < 21 || (ccrev) == 22)
#define OTPTYPE_IPX(ccrev)	((ccrev) == 21 || (ccrev) >= 23)

#define OTP_ERR_VAL	0x0001
#define OTP_MSG_VAL	0x0002
#define OTP_DBG_VAL	0x0004
uint32 otp_msg_level = OTP_ERR_VAL;

#define OTP_ERR(args)

#define OTP_MSG(args)
#define OTP_DBG(args)

#define OTPP_TRIES		10000000	/* # of tries for OTPP */
#define OTP_FUSES_PER_BIT	2
#define OTP_WRITE_RETRY		16

#ifdef BCMIPXOTP
#define MAXNUMRDES		9		/* Maximum OTP redundancy entries */
#endif

/* OTP common function type */
typedef int	(*otp_status_t)(void *oh);
typedef int	(*otp_size_t)(void *oh);
typedef void*	(*otp_init_t)(si_t *sih);
typedef uint16	(*otp_read_bit_t)(void *oh, chipcregs_t *cc, uint off);
typedef int	(*otp_read_region_t)(si_t *sih, int region, uint16 *data, uint *wlen);
typedef int	(*otp_nvread_t)(void *oh, char *data, uint *len);
typedef int	(*otp_write_region_t)(void *oh, int region, uint16 *data, uint wlen, uint flags);
typedef int	(*otp_cis_append_region_t)(si_t *sih, int region, char *vars, int count);
typedef int	(*otp_lock_t)(si_t *sih);
typedef int	(*otp_nvwrite_t)(void *oh, uint16 *data, uint wlen);
typedef int	(*otp_dump_t)(void *oh, int arg, char *buf, uint size);
typedef int	(*otp_write_word_t)(void *oh, uint wn, uint16 data);
typedef int	(*otp_read_word_t)(void *oh, uint wn, uint16 *data);
typedef int (*otp_write_bits_t)(void *oh, int bn, int bits, uint8* data);

/* OTP function struct */
typedef struct otp_fn_s {
	otp_size_t		size;
	otp_read_bit_t		read_bit;
	otp_dump_t		dump;
	otp_status_t		status;

#if !defined(BCMDONGLEHOST)
	otp_init_t		init;
	otp_read_region_t	read_region;
	otp_nvread_t		nvread;
	otp_write_region_t	write_region;
	otp_cis_append_region_t	cis_append_region;
	otp_lock_t		lock;
	otp_nvwrite_t		nvwrite;
	otp_write_word_t	write_word;
	otp_read_word_t		read_word;
	otp_write_bits_t	write_bits;
#endif /* !BCMDONGLEHOST */
} otp_fn_t;

typedef struct {
	uint		ccrev;		/* chipc revision */
	otp_fn_t	*fn;		/* OTP functions */
	si_t		*sih;		/* Saved sb handle */
	osl_t		*osh;

#ifdef BCMIPXOTP
	/* IPX OTP section */
	uint16		wsize;		/* Size of otp in words */
	uint16		rows;		/* Geometry */
	uint16		cols;		/* Geometry */
	uint32		status;		/* Flag bits (lock/prog/rv).
					 * (Reflected only when OTP is power cycled)
					 */
	uint16		hwbase;		/* hardware subregion offset */
	uint16		hwlim;		/* hardware subregion boundary */
	uint16		swbase;		/* software subregion offset */
	uint16		swlim;		/* software subregion boundary */
	uint16		fbase;		/* fuse subregion offset */
	uint16		flim;		/* fuse subregion boundary */
	int		otpgu_base;	/* offset to General Use Region */
	uint16		fusebits;	/* num of fusebits */
	bool 		buotp; 		/* Uinified OTP flag */
	uint 		usbmanfid_offset; /* Offset of the usb manfid inside the sdio CIS */
	struct {
		uint8 width;		/* entry width in bits */
		uint8 val_shift;	/* value bit offset in the entry */
		uint8 offsets;		/* # entries */
		uint8 stat_shift;	/* valid bit in otpstatus */
		uint16 offset[MAXNUMRDES];	/* entry offset in OTP */
	} rde_cb;			/* OTP redundancy control blocks */
	uint16		rde_idx;
#endif /* BCMIPXOTP */

#ifdef BCMHNDOTP
	/* HND OTP section */
	uint		size;		/* Size of otp in bytes */
	uint		hwprot;		/* Hardware protection bits */
	uint		signvalid;	/* Signature valid bits */
	int		boundary;	/* hw/sw boundary */
#endif /* BCMHNDOTP */
} otpinfo_t;

static otpinfo_t otpinfo;

/*
 * ROM accessor to avoid struct in shdat. Declare BCMRAMFN() to force the accessor to be excluded
 * from ROM.
 */
static otpinfo_t *
BCMRAMFN(get_otpinfo)(void)
{
	return (otpinfo_t *)&otpinfo;
}

/*
 * IPX OTP Code
 *
 *   Exported functions:
 *	ipxotp_status()
 *	ipxotp_size()
 *	ipxotp_init()
 *	ipxotp_read_bit()
 *	ipxotp_read_region()
 *	ipxotp_read_word()
 *	ipxotp_nvread()
 *	ipxotp_write_region()
 *	ipxotp_write_word()
 *	ipxotp_cis_append_region()
 *	ipxotp_lock()
 *	ipxotp_nvwrite()
 *	ipxotp_dump()
 *
 *   IPX internal functions:
 *	ipxotp_otpr()
 *	_ipxotp_init()
 *	ipxotp_write_bit()
 *	ipxotp_otpwb16()
 *	ipxotp_check_otp_pmu_res()
 *	ipxotp_write_rde()
 *	ipxotp_fix_word16()
 *	ipxotp_check_word16()
 *	ipxotp_max_rgnsz()
 *	ipxotp_otprb16()
 *	ipxotp_uotp_usbmanfid_offset()
 *
 */

#ifdef BCMIPXOTP

#define	OTPWSIZE		16	/* word size */
#define HWSW_RGN(rgn)		(((rgn) == OTP_HW_RGN) ? "h/w" : "s/w")

/* OTP layout */
/* CC revs 21, 24 and 27 OTP General Use Region word offset */
#define REVA4_OTPGU_BASE	12

/* CC revs 23, 25, 26, 28 and above OTP General Use Region word offset */
#define REVB8_OTPGU_BASE	20

/* CC rev 36 OTP General Use Region word offset */
#define REV36_OTPGU_BASE	12

/* Subregion word offsets in General Use region */
#define OTPGU_HSB_OFF		0
#define OTPGU_SFB_OFF		1
#define OTPGU_CI_OFF		2
#define OTPGU_P_OFF		3
#define OTPGU_SROM_OFF		4

/* Flag bit offsets in General Use region  */
#define OTPGU_NEWCISFORMAT_OFF	59
#define OTPGU_HWP_OFF		60
#define OTPGU_SWP_OFF		61
#define OTPGU_CIP_OFF		62
#define OTPGU_FUSEP_OFF		63
#define OTPGU_CIP_MSK		0x4000
#define OTPGU_P_MSK		0xf000
#define OTPGU_P_SHIFT		(OTPGU_HWP_OFF % 16)

/* LOCK but offset */
#define OTP_LOCK_ROW1_LOC_OFF	63	/* 1st ROW lock bit */
#define OTP_LOCK_ROW2_LOC_OFF	127	/* 2nd ROW lock bit */
#define OTP_LOCK_RD_LOC_OFF	128	/* Redundnancy Region lock bit */
#define OTP_LOCK_GU_LOC_OFF	129	/* General User Region lock bit */


/* OTP Size */
#define OTP_SZ_FU_972		((ROUNDUP(972, 16))/8)
#define OTP_SZ_FU_720		((ROUNDUP(720, 16))/8)
#define OTP_SZ_FU_608		((ROUNDUP(608, 16))/8)
#define OTP_SZ_FU_576		((ROUNDUP(576, 16))/8)
#define OTP_SZ_FU_324		((ROUNDUP(324,8))/8)	/* 324 bits */
#define OTP_SZ_FU_792		(792/8)		/* 792 bits */
#define OTP_SZ_FU_288		(288/8)		/* 288 bits */
#define OTP_SZ_FU_216		(216/8)		/* 216 bits */
#define OTP_SZ_FU_72		(72/8)		/* 72 bits */
#define OTP_SZ_CHECKSUM		(16/8)		/* 16 bits */
#define OTP4315_SWREG_SZ	178		/* 178 bytes */
#define OTP_SZ_FU_144		(144/8)		/* 144 bits */
#define OTP_SZ_FU_180		((ROUNDUP(180,8))/8)	/* 180 bits */

/* OTP BT shared region (pre-allocated) */
#define	OTP_BT_BASE_4330	(1760/OTPWSIZE)
#define OTP_BT_END_4330		(1888/OTPWSIZE)
#define OTP_BT_BASE_4324	(2384/OTPWSIZE)
#define	OTP_BT_END_4324		(2640/OTPWSIZE)
#define OTP_BT_BASE_4334	(2512/OTPWSIZE)
#define	OTP_BT_END_4334		(2768/OTPWSIZE)
#define OTP_BT_BASE_4314	(4192/OTPWSIZE)
#define	OTP_BT_END_4314		(4960/OTPWSIZE)
#define OTP_BT_BASE_4350	(4384/OTPWSIZE)
#define OTP_BT_END_4350		(5408/OTPWSIZE)
#define OTP_BT_BASE_4335	(4528/OTPWSIZE)
#define	OTP_BT_END_4335		(5552/OTPWSIZE)
#define OTP_BT_BASE_4345	(4496/OTPWSIZE)
#define OTP_BT_END_4345		(5520/OTPWSIZE)

/* OTP unification */
#if defined(USBSDIOUNIFIEDOTP)
/** Offset in OTP from upper GUR to HNBU_UMANFID tuple value in (16-bit) words */
#define USB_MANIFID_OFFSET_4319		42
#define USB_MANIFID_OFFSET_43143	45 /* See Confluence 43143 SW notebook #1 */
#endif /* USBSDIOUNIFIEDOTP */

#if !defined(BCMDONGLEHOST)
#endif /* BCMDONGLEHOST */
static otp_fn_t* get_ipxotp_fn(void);

static int
BCMNMIATTACHFN(ipxotp_status)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return (int)(oi->status);
}

/** Returns size in bytes */
static int
BCMNMIATTACHFN(ipxotp_size)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return (int)oi->wsize * 2;
}

static uint16
BCMNMIATTACHFN(ipxotp_read_bit_common)(void *oh, chipcregs_t *cc, uint off)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint k, row, col;
	uint32 otpp, st;
	uint otpwt;

	otpwt = (R_REG(oi->osh, &cc->otplayout) & OTPL_WRAP_TYPE_MASK) >> OTPL_WRAP_TYPE_SHIFT;

	row = off / oi->cols;
	col = off % oi->cols;

	otpp = OTPP_START_BUSY |
		((((otpwt == OTPL_WRAP_TYPE_40NM)? OTPPOC_READ_40NM :
		OTPPOC_READ) << OTPP_OC_SHIFT) & OTPP_OC_MASK) |
	        ((row << OTPP_ROW_SHIFT) & OTPP_ROW_MASK) |
	        ((col << OTPP_COL_SHIFT) & OTPP_COL_MASK);
	OTP_DBG(("%s: off = %d, row = %d, col = %d, otpp = 0x%x",
	         __FUNCTION__, off, row, col, otpp));
	W_REG(oi->osh, &cc->otpprog, otpp);

	for (k = 0;
	     ((st = R_REG(oi->osh, &cc->otpprog)) & OTPP_START_BUSY) && (k < OTPP_TRIES);
	     k ++)
		;
	if (k >= OTPP_TRIES) {
		OTP_ERR(("\n%s: BUSY stuck: st=0x%x, count=%d\n", __FUNCTION__, st, k));
		return 0xffff;
	}
	if (st & OTPP_READERR) {
		OTP_ERR(("\n%s: Could not read OTP bit %d\n", __FUNCTION__, off));
		return 0xffff;
	}
	st = (st & OTPP_VALUE_MASK) >> OTPP_VALUE_SHIFT;

	OTP_DBG((" => %d\n", st));
	return (int)st;
}

static uint16
BCMNMIATTACHFN(ipxotp_read_bit)(void *oh, chipcregs_t *cc, uint off)
{
	otpinfo_t *oi;

	oi = (otpinfo_t *)oh;
	W_REG(oi->osh, &cc->otpcontrol, 0);
	W_REG(oi->osh, &cc->otpcontrol1, 0);

	return ipxotp_read_bit_common(oh, cc, off);
}

#if !defined(BCMDONGLEHOST)
static uint16
BCMNMIATTACHFN(ipxotp_otprb16)(void *oh, chipcregs_t *cc, uint wn)
{
	uint base, i;
	uint16 val;
	uint16 bit;

	base = wn * 16;

	val = 0;
	for (i = 0; i < 16; i++) {
		if ((bit = ipxotp_read_bit(oh, cc, base + i)) == 0xffff)
			break;
		val = val | (bit << i);
	}
	if (i < 16)
		val = 0xffff;

	return val;
}

static uint16
BCMNMIATTACHFN(ipxotp_otpr)(void *oh, chipcregs_t *cc, uint wn)
{
	otpinfo_t *oi;
	si_t *sih;
	uint16 val;


	oi = (otpinfo_t *)oh;

	ASSERT(wn < oi->wsize);
	ASSERT(cc != NULL);

	sih = oi->sih;
	ASSERT(sih != NULL);
	/* If sprom is available use indirect access(as cc->sromotp maps to srom),
	 * else use random-access.
	 */
	if (si_is_sprom_available(sih))
		val = ipxotp_otprb16(oi, cc, wn);
	else
		val = R_REG(oi->osh, &cc->sromotp[wn]);

	return val;
}
#endif 

#if !defined(BCMDONGLEHOST)
/** OTP BT region size */
static void
BCMNMIATTACHFN(ipxotp_bt_region_get)(otpinfo_t *oi, uint16 *start, uint16 *end)
{
	*start = *end = 0;
	switch (CHIPID(oi->sih->chip)) {
	case BCM4330_CHIP_ID:
		*start = OTP_BT_BASE_4330;
		*end = OTP_BT_END_4330;
		break;
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
		*start = OTP_BT_BASE_4324;
		*end = OTP_BT_END_4324;
		break;
	case BCM4334_CHIP_ID:
	case BCM43341_CHIP_ID:
		*start = OTP_BT_BASE_4334;
		*end = OTP_BT_END_4334;
		break;
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
		*start = OTP_BT_BASE_4314;
		*end = OTP_BT_END_4314;
		break;
	case BCM43602_CHIP_ID:	/* 43602 does not contain a BT core, only GCI/SECI interface. */
	case BCM43462_CHIP_ID:
		break;
	case BCM4345_CHIP_ID:
#ifdef UNRELEASEDCHIP
	case BCM43457_CHIP_ID:
#endif /* UNRELEASEDCHIP */
		*start = OTP_BT_BASE_4345;
		*end = OTP_BT_END_4345;
		break;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM4356_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
		*start = OTP_BT_BASE_4350;
		*end = OTP_BT_END_4350;
		break;
	}
}

/**
 * Calculate max HW/SW region byte size by substracting fuse region and checksum size,
 * osizew is oi->wsize (OTP size - GU size) in words.
 */
static int
BCMNMIATTACHFN(ipxotp_max_rgnsz)(otpinfo_t *oi)
{
	int osizew = oi->wsize;
	int ret = 0;
	uint16 checksum;
	uint idx;
	chipcregs_t *cc;

	idx = si_coreidx(oi->sih);
	cc = si_setcoreidx(oi->sih, SI_CC_IDX);
	ASSERT(cc != NULL);

	checksum = OTP_SZ_CHECKSUM;

	/* for new chips, fusebit is available from cc register */
	if (oi->sih->ccrev >= 35) {
		oi->fusebits = R_REG(oi->osh, &cc->otplayoutextension) & OTPLAYOUTEXT_FUSE_MASK;
		oi->fusebits = ROUNDUP(oi->fusebits, 8);
		oi->fusebits >>= 3; /* bytes */
	}

	si_setcoreidx(oi->sih, idx);

	switch (CHIPID(oi->sih->chip)) {
	case BCM4322_CHIP_ID:	case BCM43221_CHIP_ID:	case BCM43231_CHIP_ID:
	case BCM43239_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_288;
		break;
	case BCM43222_CHIP_ID:	case BCM43111_CHIP_ID:	case BCM43112_CHIP_ID:
	case BCM43224_CHIP_ID:	case BCM43225_CHIP_ID:	case BCM43421_CHIP_ID:
	case BCM43226_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_72;
		break;
	case BCM43236_CHIP_ID:	case BCM43235_CHIP_ID:	case BCM43238_CHIP_ID:
	case BCM43237_CHIP_ID:
	case BCM43234_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_324;
		break;
	case BCM4325_CHIP_ID:
	case BCM5356_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_216;
		break;
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_144;
		break;
	case BCM4313_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_72;
		break;
	case BCM4330_CHIP_ID:
	case BCM4334_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_144;
		break;
	case BCM4319_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_180;
		break;
	case BCM4331_CHIP_ID:
	case BCM43431_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_72;
		break;
	case BCM43131_CHIP_ID:
	case BCM43217_CHIP_ID:
	case BCM43227_CHIP_ID:
	case BCM43228_CHIP_ID:
	case BCM43428_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_72;
		break;
	case BCM4335_CHIP_ID:
#ifdef UNRELEASEDCHIP
	case BCM43455_CHIP_ID:
#endif
		oi->fusebits = OTP_SZ_FU_576;
		break;
	case BCM4345_CHIP_ID:
#ifdef UNRELEASEDCHIP
	case BCM43457_CHIP_ID:
#endif /* UNRELEASEDCHIP */
		oi->fusebits = OTP_SZ_FU_608;
		break;
	case BCM43602_CHIP_ID:
	case BCM43462_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_972;
		break;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM4356_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_720;
		break;
	case BCM4360_CHIP_ID:
		oi->fusebits = OTP_SZ_FU_792;
		break;
	default:
		if (oi->fusebits == 0)
			ASSERT(0);	/* Don't konw about this chip */
	}

	ret = osizew*2 - oi->fusebits - checksum;

	if (CHIPID(oi->sih->chip) == BCM4315_CHIP_ID) {
		ret = OTP4315_SWREG_SZ;
	}

	OTP_MSG(("max region size %d bytes\n", ret));
	return ret;
}

/** OTP sizes for 65nm and 130nm */
static int
BCMNMIATTACHFN(ipxotp_otpsize_set_65nm)(otpinfo_t *oi, uint otpsz)
{
	/* Check for otp size */
	switch (otpsz) {
	case 1:	/* 32x64 */
		oi->rows = 32;
		oi->cols = 64;
		oi->wsize = 128;
		break;
	case 2:	/* 64x64 */
		oi->rows = 64;
		oi->cols = 64;
		oi->wsize = 256;
		break;
	case 5:	/* 96x64 */
		oi->rows = 96;
		oi->cols = 64;
		oi->wsize = 384;
		break;
	case 7:	/* 16x64 */ /* 1024 bits */
		oi->rows = 16;
		oi->cols = 64;
		oi->wsize = 64;
		break;
	default:
		/* Don't know the geometry */
		OTP_ERR(("%s: unknown OTP geometry\n", __FUNCTION__));
	}

	return 0;
}

/**  OTP sizes for 40nm */
static int
BCMNMIATTACHFN(ipxotp_otpsize_set_40nm)(otpinfo_t *oi, uint otpsz)
{
	/* Check for otp size */
	switch (otpsz) {
	case 1:	/* 64x32: 2048 bits */
		oi->rows = 64;
		oi->cols = 32;
		break;
	case 2:	/* 96x32: 3072 bits */
		oi->rows = 96;
		oi->cols = 32;
		break;
	case 3:	/* 128x32: 4096 bits */
		oi->rows = 128;
		oi->cols = 32;
		break;
	case 4:	/* 160x32: 5120 bits */
		oi->rows = 160;
		oi->cols = 32;
		break;
	case 5:	/* 192x32: 6144 bits */
		oi->rows = 192;
		oi->cols = 32;
		break;
	case 7:	/* 256x32: 8192 bits */
		oi->rows = 256;
		oi->cols = 32;
		break;
	case 11: /* 384x32: 12288 bits */
		oi->rows = 384;
		oi->cols = 32;
		break;
	default:
		/* Don't know the geometry */
		OTP_ERR(("%s: unknown OTP geometry\n", __FUNCTION__));
	}

	oi->wsize = (oi->cols * oi->rows)/OTPWSIZE;
	return 0;
}

/** OTP unification */

static void
BCMNMIATTACHFN(_ipxotp_init)(otpinfo_t *oi, chipcregs_t *cc)
{
	uint	k;
	uint32 otpp, st;
	uint16 btsz, btbase = 0, btend = 0;
	uint   otpwt;

	/* record word offset of General Use Region for various chipcommon revs */
	if (oi->sih->ccrev >= 40) {
		/* FIX: Available in rev >= 23; Verify before applying to others */
		oi->otpgu_base = (R_REG(oi->osh, &cc->otplayout) & OTPL_HWRGN_OFF_MASK)
			>> OTPL_HWRGN_OFF_SHIFT;
		ASSERT((oi->otpgu_base - (OTPGU_SROM_OFF * OTPWSIZE)) > 0);
		oi->otpgu_base >>= 4; /* words */
		oi->otpgu_base -= OTPGU_SROM_OFF;
	} else if (oi->sih->ccrev == 21 || oi->sih->ccrev == 24 || oi->sih->ccrev == 27) {
		oi->otpgu_base = REVA4_OTPGU_BASE;
	} else if ((oi->sih->ccrev == 36) || (oi->sih->ccrev == 39)) {
		/* OTP size greater than equal to 2KB (128 words), otpgu_base is similar to rev23 */
		if (oi->wsize >= 128)
			oi->otpgu_base = REVB8_OTPGU_BASE;
		else
			oi->otpgu_base = REV36_OTPGU_BASE;
	} else if (oi->sih->ccrev == 23 || oi->sih->ccrev >= 25) {
		oi->otpgu_base = REVB8_OTPGU_BASE;
	} else {
		OTP_ERR(("%s: chipc rev %d not supported\n", __FUNCTION__, oi->sih->ccrev));
	}

	otpwt = (R_REG(oi->osh, &cc->otplayout) & OTPL_WRAP_TYPE_MASK) >> OTPL_WRAP_TYPE_SHIFT;

	if (otpwt != OTPL_WRAP_TYPE_40NM) {
		/* First issue an init command so the status is up to date */
		otpp = OTPP_START_BUSY | ((OTPPOC_INIT << OTPP_OC_SHIFT) & OTPP_OC_MASK);

		OTP_DBG(("%s: otpp = 0x%x", __FUNCTION__, otpp));
		W_REG(oi->osh, &cc->otpprog, otpp);
		for (k = 0;
			((st = R_REG(oi->osh, &cc->otpprog)) & OTPP_START_BUSY) && (k < OTPP_TRIES);
			k ++)
			;
			if (k >= OTPP_TRIES) {
			OTP_ERR(("\n%s: BUSY stuck: st=0x%x, count=%d\n", __FUNCTION__, st, k));
			return;
		}
	}

	/* Read OTP lock bits and subregion programmed indication bits */
	oi->status = R_REG(oi->osh, &cc->otpstatus);

	if ((CHIPID(oi->sih->chip) == BCM43222_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43111_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43112_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43224_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43225_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43421_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43226_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43236_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43235_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43234_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43238_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43237_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43239_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM4324_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43242_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43243_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43143_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM4331_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43431_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM4360_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43460_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43526_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM4345_CHIP_ID) ||
#ifdef UNRELEASEDCHIP
		(CHIPID(oi->sih->chip) == BCM43457_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43430_CHIP_ID) ||
#endif /* UNRELEASEDCHIP */
		(CHIPID(oi->sih->chip) == BCM43602_CHIP_ID) ||
		(CHIPID(oi->sih->chip) == BCM43462_CHIP_ID) ||
		BCM4350_CHIP(oi->sih->chip) ||
	0) {
		uint32 p_bits;
		p_bits = (ipxotp_otpr(oi, cc, oi->otpgu_base + OTPGU_P_OFF) & OTPGU_P_MSK)
			>> OTPGU_P_SHIFT;
		oi->status |= (p_bits << OTPS_GUP_SHIFT);
	}
	OTP_DBG(("%s: status 0x%x\n", __FUNCTION__, oi->status));

	/* OTP unification */
	oi->buotp = FALSE; /* Initialize it to false, until its explicitely set true. */
	if ((oi->status & (OTPS_GUP_HW | OTPS_GUP_SW)) == (OTPS_GUP_HW | OTPS_GUP_SW)) {
		switch (CHIPID(oi->sih->chip)) {
			/* Add cases for supporting chips */
			case BCM4319_CHIP_ID:
				oi->buotp = TRUE;
				break;
			case BCM43143_CHIP_ID:
				oi->buotp = TRUE;
				break;
			case BCM4335_CHIP_ID:
			case BCM4345_CHIP_ID:
#ifdef UNRELEASEDCHIP
			case BCM43457_CHIP_ID:
			case BCM43455_CHIP_ID:
			case BCM43430_CHIP_ID:
#endif /* UNRELEASEDCHIP */
				oi->buotp = TRUE;
				break;
			default:
				OTP_ERR(("chip=0x%x does not support Unified OTP.\n",
					CHIPID(oi->sih->chip)));
				break;
		}
	}

	/*
	 * h/w region base and fuse region limit are fixed to the top and
	 * the bottom of the general use region. Everything else can be flexible.
	 */
	oi->hwbase = oi->otpgu_base + OTPGU_SROM_OFF;
	oi->hwlim = oi->wsize;
	oi->flim = oi->wsize;

	ipxotp_bt_region_get(oi, &btbase, &btend);
	btsz = btend - btbase;
	if (btsz > 0) {
		/* default to not exceed BT base */
		oi->hwlim = btbase;

		/* With BT shared region, swlim and fbase are fixed */
		oi->swlim = btbase;
		oi->fbase = btend;
	}

	/* Update hwlim and swbase */
	if (oi->status & OTPS_GUP_HW) {
		uint16 swbase;
		OTP_DBG(("%s: hw region programmed\n", __FUNCTION__));
		swbase = ipxotp_otpr(oi, cc, oi->otpgu_base + OTPGU_HSB_OFF) / 16;
		if (swbase) {
			oi->hwlim =  swbase;
		}
		oi->swbase = oi->hwlim;
	} else
		oi->swbase = oi->hwbase;

	/* Update swlim and fbase only if no BT region */
	if (btsz == 0) {
		/* subtract fuse and checksum from beginning */
		oi->swlim = ipxotp_max_rgnsz(oi) / 2;

		if (oi->status & OTPS_GUP_SW) {
			OTP_DBG(("%s: sw region programmed\n", __FUNCTION__));
			oi->swlim = ipxotp_otpr(oi, cc, oi->otpgu_base + OTPGU_SFB_OFF) / 16;
			oi->fbase = oi->swlim;
		}
		else
			oi->fbase = oi->swbase;
	}

	OTP_DBG(("%s: OTP limits---\n"
		"hwbase %d/%d hwlim %d/%d\n"
		"swbase %d/%d swlim %d/%d\n"
		"fbase %d/%d flim %d/%d\n", __FUNCTION__,
		oi->hwbase, oi->hwbase * 16, oi->hwlim, oi->hwlim * 16,
		oi->swbase, oi->swbase * 16, oi->swlim, oi->swlim * 16,
		oi->fbase, oi->fbase * 16, oi->flim, oi->flim * 16));
}

static void *
BCMNMIATTACHFN(ipxotp_init)(si_t *sih)
{
	uint idx, otpsz, otpwt;
	chipcregs_t *cc;
	otpinfo_t *oi = NULL;

	OTP_MSG(("%s: Use IPX OTP controller\n", __FUNCTION__));

	/* Make sure we're running IPX OTP */
	ASSERT(OTPTYPE_IPX(sih->ccrev));
	if (!OTPTYPE_IPX(sih->ccrev))
		return NULL;

	/* Make sure OTP is not disabled */
	if (si_is_otp_disabled(sih)) {
		OTP_MSG(("%s: OTP is disabled\n", __FUNCTION__));
		return NULL;
	}

	/* Make sure OTP is powered up */
	if (!si_is_otp_powered(sih)) {
		OTP_ERR(("%s: OTP is powered down\n", __FUNCTION__));
		return NULL;
	}

	/* Retrieve OTP region info */
	idx = si_coreidx(sih);
	cc = si_setcoreidx(sih, SI_CC_IDX);
	ASSERT(cc != NULL);

	oi = get_otpinfo();

	/* in Rev 49 otpsize moved to otplayout[15:12] */
	if (sih->ccrev >= 49) {
		otpsz = (R_REG(oi->osh, &cc->otplayout) & OTPL_ROW_SIZE_MASK)
			>> OTPL_ROW_SIZE_SHIFT;
	} else {
		otpsz = (sih->cccaps & CC_CAP_OTPSIZE) >> CC_CAP_OTPSIZE_SHIFT;
	}
	if (otpsz == 0) {
		OTP_ERR(("%s: No OTP\n", __FUNCTION__));
		oi = NULL;
		goto exit;
	}

	otpwt = (R_REG(oi->osh, &cc->otplayout) & OTPL_WRAP_TYPE_MASK) >> OTPL_WRAP_TYPE_SHIFT;

	if (otpwt == OTPL_WRAP_TYPE_40NM) {
		ipxotp_otpsize_set_40nm(oi, otpsz);
	} else if (otpwt == OTPL_WRAP_TYPE_65NM) {
		ipxotp_otpsize_set_65nm(oi, otpsz);
	} else {
		OTP_ERR(("%s: Unknown wrap type: %d\n", __FUNCTION__, otpwt));
		ASSERT(0);
	}

	OTP_MSG(("%s: rows %u cols %u wsize %u\n", __FUNCTION__, oi->rows, oi->cols, oi->wsize));


	_ipxotp_init(oi, cc);

exit:
	si_setcoreidx(sih, idx);

	return (void *)oi;
}

static int
BCMNMIATTACHFN(ipxotp_read_region)(void *oh, int region, uint16 *data, uint *wlen)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint idx;
	chipcregs_t *cc;
	uint base, i, sz;

	/* Validate region selection */
	switch (region) {
	case OTP_HW_RGN:
		/* OTP unification: For unified OTP sz=flim-hwbase */
		if (oi->buotp)
			sz = (uint)oi->flim - oi->hwbase;
		else
			sz = (uint)oi->hwlim - oi->hwbase;
		if (!(oi->status & OTPS_GUP_HW)) {
			OTP_ERR(("%s: h/w region not programmed\n", __FUNCTION__));
			*wlen = sz;
			return BCME_NOTFOUND;
		}
		if (*wlen < sz) {
			OTP_ERR(("%s: buffer too small, should be at least %u\n",
			         __FUNCTION__, oi->hwlim - oi->hwbase));
			*wlen = sz;
			return BCME_BUFTOOSHORT;
		}
		base = oi->hwbase;
		break;
	case OTP_SW_RGN:
		/* OTP unification: For unified OTP sz=flim-swbase */
		if (oi->buotp)
			sz = ((uint)oi->flim - oi->swbase);
		else
			sz = ((uint)oi->swlim - oi->swbase);
		if (!(oi->status & OTPS_GUP_SW)) {
			OTP_ERR(("%s: s/w region not programmed\n", __FUNCTION__));
			*wlen = sz;
			return BCME_NOTFOUND;
		}
		if (*wlen < sz) {
			OTP_ERR(("%s: buffer too small should be at least %u\n",
			         __FUNCTION__, oi->swlim - oi->swbase));
			*wlen = sz;
			return BCME_BUFTOOSHORT;
		}
		base = oi->swbase;
		break;
	case OTP_CI_RGN:
		sz = OTPGU_CI_SZ;
		if (!(oi->status & OTPS_GUP_CI)) {
			OTP_ERR(("%s: chipid region not programmed\n", __FUNCTION__));
			*wlen = sz;
			return BCME_NOTFOUND;
		}
		if (*wlen < sz) {
			OTP_ERR(("%s: buffer too small, should be at least %u\n",
			         __FUNCTION__, OTPGU_CI_SZ));
			*wlen = sz;
			return BCME_BUFTOOSHORT;
		}
		base = oi->otpgu_base + OTPGU_CI_OFF;
		break;
	case OTP_FUSE_RGN:
		sz = (uint)oi->flim - oi->fbase;
		if (!(oi->status & OTPS_GUP_FUSE)) {
			OTP_ERR(("%s: fuse region not programmed\n", __FUNCTION__));
			*wlen = sz;
			return BCME_NOTFOUND;
		}
		if (*wlen < sz) {
			OTP_ERR(("%s: buffer too small, should be at least %u\n",
			         __FUNCTION__, oi->flim - oi->fbase));
			*wlen = sz;
			return BCME_BUFTOOSHORT;
		}
		base = oi->fbase;
		break;
	case OTP_ALL_RGN:
		sz = ((uint)oi->flim - oi->hwbase);
		if (!(oi->status & (OTPS_GUP_HW | OTPS_GUP_SW))) {
			OTP_ERR(("%s: h/w & s/w region not programmed\n", __FUNCTION__));
			*wlen = sz;
			return BCME_NOTFOUND;
		}
		if (*wlen < sz) {
			OTP_ERR(("%s: buffer too small, should be at least %u\n",
				__FUNCTION__, oi->hwlim - oi->hwbase));
			*wlen = sz;
			return BCME_BUFTOOSHORT;
		}
		base = oi->hwbase;
		break;
	default:
		OTP_ERR(("%s: reading region %d is not supported\n", __FUNCTION__, region));
		return BCME_BADARG;
	}

	idx = si_coreidx(oi->sih);
	cc = si_setcoreidx(oi->sih, SI_CC_IDX);
	ASSERT(cc != NULL);

	/* Read the data */
	for (i = 0; i < sz; i ++)
		data[i] = ipxotp_otpr(oh, cc, base + i);

	si_setcoreidx(oi->sih, idx);
	*wlen = sz;
	return 0;
}

static int
BCMNMIATTACHFN(ipxotp_read_word)(void *oh, uint wn, uint16 *data)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint idx;
	chipcregs_t *cc;

	idx = si_coreidx(oi->sih);
	cc = si_setcoreidx(oi->sih, SI_CC_IDX);
	ASSERT(cc != NULL);

	/* Read the data */
	*data = ipxotp_otpr(oh, cc, wn);

	si_setcoreidx(oi->sih, idx);
	return 0;
}

static int
BCMNMIATTACHFN(ipxotp_nvread)(void *oh, char *data, uint *len)
{
	return BCME_UNSUPPORTED;
}

#endif /* !defined(BCMDONGLEHOST) */


static otp_fn_t ipxotp_fn = {
	(otp_size_t)ipxotp_size,
	(otp_read_bit_t)ipxotp_read_bit,
	(otp_dump_t)NULL,		/* Assigned in otp_init */
	(otp_status_t)ipxotp_status,

#if !defined(BCMDONGLEHOST)
	(otp_init_t)ipxotp_init,
	(otp_read_region_t)ipxotp_read_region,
	(otp_nvread_t)ipxotp_nvread,
	(otp_write_region_t)NULL,
	(otp_cis_append_region_t)NULL,
	(otp_lock_t)NULL,
	(otp_nvwrite_t)NULL,
	(otp_write_word_t)NULL,
	(otp_read_word_t)ipxotp_read_word,
#endif /* !defined(BCMDONGLEHOST) */
#if !defined(BCMDONGLEHOST)
	(otp_write_bits_t)NULL
#endif /* !BCMDONGLEHOST */
};

/*
 * ROM accessor to avoid struct in shdat. Declare BCMRAMFN() to force the accessor to be excluded
 * from ROM.
 */
static otp_fn_t *
BCMRAMFN(get_ipxotp_fn)(void)
{
	return &ipxotp_fn;
}
#endif /* BCMIPXOTP */

/*
 * HND OTP Code
 *
 *   Exported functions:
 *	hndotp_status()
 *	hndotp_size()
 *	hndotp_init()
 *	hndotp_read_bit()
 *	hndotp_read_region()
 *	hndotp_read_word()
 *	hndotp_nvread()
 *	hndotp_write_region()
 *	hndotp_cis_append_region()
 *	hndotp_lock()
 *	hndotp_nvwrite()
 *	hndotp_dump()
 *
 *   HND internal functions:
 * 	hndotp_otpr()
 * 	hndotp_otproff()
 *	hndotp_write_bit()
 *	hndotp_write_word()
 *	hndotp_valid_rce()
 *	hndotp_write_rce()
 *	hndotp_write_row()
 *	hndotp_otprb16()
 *
 */

#ifdef BCMHNDOTP

/* Fields in otpstatus */
#define	OTPS_PROGFAIL		0x80000000
#define	OTPS_PROTECT		0x00000007
#define	OTPS_HW_PROTECT		0x00000001
#define	OTPS_SW_PROTECT		0x00000002
#define	OTPS_CID_PROTECT	0x00000004
#define	OTPS_RCEV_MSK		0x00003f00
#define	OTPS_RCEV_SHIFT		8

/* Fields in the otpcontrol register */
#define	OTPC_RECWAIT		0xff000000
#define	OTPC_PROGWAIT		0x00ffff00
#define	OTPC_PRW_SHIFT		8
#define	OTPC_MAXFAIL		0x00000038
#define	OTPC_VSEL		0x00000006
#define	OTPC_SELVL		0x00000001

/* OTP regions (Word offsets from otp size) */
#define	OTP_SWLIM_OFF	(-4)
#define	OTP_CIDBASE_OFF	0
#define	OTP_CIDLIM_OFF	4

/* Predefined OTP words (Word offset from otp size) */
#define	OTP_BOUNDARY_OFF (-4)
#define	OTP_HWSIGN_OFF	(-3)
#define	OTP_SWSIGN_OFF	(-2)
#define	OTP_CIDSIGN_OFF	(-1)
#define	OTP_CID_OFF	0
#define	OTP_PKG_OFF	1
#define	OTP_FID_OFF	2
#define	OTP_RSV_OFF	3
#define	OTP_LIM_OFF	4
#define	OTP_RD_OFF	4	/* Redundancy row starts here */
#define	OTP_RC0_OFF	28	/* Redundancy control word 1 */
#define	OTP_RC1_OFF	32	/* Redundancy control word 2 */
#define	OTP_RC_LIM_OFF	36	/* Redundancy control word end */

#define	OTP_HW_REGION	OTPS_HW_PROTECT
#define	OTP_SW_REGION	OTPS_SW_PROTECT
#define	OTP_CID_REGION	OTPS_CID_PROTECT

#if OTP_HW_REGION != OTP_HW_RGN
#error "incompatible OTP_HW_RGN"
#endif
#if OTP_SW_REGION != OTP_SW_RGN
#error "incompatible OTP_SW_RGN"
#endif
#if OTP_CID_REGION != OTP_CI_RGN
#error "incompatible OTP_CI_RGN"
#endif

/* Redundancy entry definitions */
#define	OTP_RCE_ROW_SZ		6
#define	OTP_RCE_SIGN_MASK	0x7fff
#define	OTP_RCE_ROW_MASK	0x3f
#define	OTP_RCE_BITS		21
#define	OTP_RCE_SIGN_SZ		15
#define	OTP_RCE_BIT0		1

#define	OTP_WPR		4
#define	OTP_SIGNATURE	0x578a
#define	OTP_MAGIC	0x4e56

static int
BCMNMIATTACHFN(hndotp_status)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return ((int)(oi->hwprot | oi->signvalid));
}

static int
BCMNMIATTACHFN(hndotp_size)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return ((int)(oi->size));
}

#if !defined(BCMDONGLEHOST)
static uint16
BCMNMIATTACHFN(hndotp_otpr)(void *oh, chipcregs_t *cc, uint wn)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	osl_t *osh;
	volatile uint16 *ptr;

	ASSERT(wn < ((oi->size / 2) + OTP_RC_LIM_OFF));
	ASSERT(cc != NULL);

	osh = si_osh(oi->sih);

	ptr = (volatile uint16 *)((volatile char *)cc + CC_SROM_OTP);
	return (R_REG(osh, &ptr[wn]));
}
#endif 

#if !defined(BCMDONGLEHOST)
static uint16
BCMNMIATTACHFN(hndotp_otproff)(void *oh, chipcregs_t *cc, int woff)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	osl_t *osh;
	volatile uint16 *ptr;

	ASSERT(woff >= (-((int)oi->size / 2)));
	ASSERT(woff < OTP_LIM_OFF);
	ASSERT(cc != NULL);

	osh = si_osh(oi->sih);

	ptr = (volatile uint16 *)((volatile char *)cc + CC_SROM_OTP);

	return (R_REG(osh, &ptr[(oi->size / 2) + woff]));
}
#endif /* !defined(BCMDONGLEHOST) */

static uint16
BCMNMIATTACHFN(hndotp_read_bit)(void *oh, chipcregs_t *cc, uint idx)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint k, row, col;
	uint32 otpp, st;
	osl_t *osh;

	osh = si_osh(oi->sih);
	row = idx / 65;
	col = idx % 65;

	otpp = OTPP_START_BUSY | OTPP_READ |
	        ((row << OTPP_ROW_SHIFT) & OTPP_ROW_MASK) |
	        (col & OTPP_COL_MASK);

	OTP_DBG(("%s: idx = %d, row = %d, col = %d, otpp = 0x%x", __FUNCTION__,
	         idx, row, col, otpp));

	W_REG(osh, &cc->otpprog, otpp);
	st = R_REG(osh, &cc->otpprog);
	for (k = 0; ((st & OTPP_START_BUSY) == OTPP_START_BUSY) && (k < OTPP_TRIES); k++)
		st = R_REG(osh, &cc->otpprog);

	if (k >= OTPP_TRIES) {
		OTP_ERR(("\n%s: BUSY stuck: st=0x%x, count=%d\n", __FUNCTION__, st, k));
		return 0xffff;
	}
	if (st & OTPP_READERR) {
		OTP_ERR(("\n%s: Could not read OTP bit %d\n", __FUNCTION__, idx));
		return 0xffff;
	}
	st = (st & OTPP_VALUE_MASK) >> OTPP_VALUE_SHIFT;
	OTP_DBG((" => %d\n", st));
	return (uint16)st;
}

#if !defined(BCMDONGLEHOST)
static void *
BCMNMIATTACHFN(hndotp_init)(si_t *sih)
{
	uint idx;
	chipcregs_t *cc;
	otpinfo_t *oi;
	uint32 cap = 0, clkdiv, otpdiv = 0;
	void *ret = NULL;
	osl_t *osh;

	OTP_MSG(("%s: Use HND OTP controller\n", __FUNCTION__));

	oi = get_otpinfo();

	idx = si_coreidx(sih);
	osh = si_osh(oi->sih);

	/* Check for otp */
	if ((cc = si_setcoreidx(sih, SI_CC_IDX)) != NULL) {
		cap = R_REG(osh, &cc->capabilities);
		if ((cap & CC_CAP_OTPSIZE) == 0) {
			/* Nothing there */
			goto out;
		}

		/* As of right now, support only 4320a2, 4311a1 and 4312 */
		ASSERT((oi->ccrev == 12) || (oi->ccrev == 17) || (oi->ccrev == 22));
		if (!((oi->ccrev == 12) || (oi->ccrev == 17) || (oi->ccrev == 22)))
			return NULL;

		/* Read the OTP byte size. chipcommon rev >= 18 has RCE so the size is
		 * 8 row (64 bytes) smaller
		 */
		oi->size = 1 << (((cap & CC_CAP_OTPSIZE) >> CC_CAP_OTPSIZE_SHIFT)
			+ CC_CAP_OTPSIZE_BASE);
		if (oi->ccrev >= 18) {
			oi->size -= ((OTP_RC0_OFF - OTP_BOUNDARY_OFF) * 2);
		} else {
			OTP_ERR(("Negative otp size, shouldn't happen for programmed chip."));
			oi->size = 0;
		}

		oi->hwprot = (int)(R_REG(osh, &cc->otpstatus) & OTPS_PROTECT);
		oi->boundary = -1;

		/* Check the region signature */
		if (hndotp_otproff(oi, cc, OTP_HWSIGN_OFF) == OTP_SIGNATURE) {
			oi->signvalid |= OTP_HW_REGION;
			oi->boundary = hndotp_otproff(oi, cc, OTP_BOUNDARY_OFF);
		}

		if (hndotp_otproff(oi, cc, OTP_SWSIGN_OFF) == OTP_SIGNATURE)
			oi->signvalid |= OTP_SW_REGION;

		if (hndotp_otproff(oi, cc, OTP_CIDSIGN_OFF) == OTP_SIGNATURE)
			oi->signvalid |= OTP_CID_REGION;

		/* Set OTP clkdiv for stability */
		if (oi->ccrev == 22)
			otpdiv = 12;

		if (otpdiv) {
			clkdiv = R_REG(osh, &cc->clkdiv);
			clkdiv = (clkdiv & ~CLKD_OTP) | (otpdiv << CLKD_OTP_SHIFT);
			W_REG(osh, &cc->clkdiv, clkdiv);
			OTP_MSG(("%s: set clkdiv to %x\n", __FUNCTION__, clkdiv));
		}
		OSL_DELAY(10);

		ret = (void *)oi;
	}

	OTP_MSG(("%s: ccrev %d\tsize %d bytes\thwprot %x\tsignvalid %x\tboundary %x\n",
		__FUNCTION__, oi->ccrev, oi->size, oi->hwprot, oi->signvalid,
		oi->boundary));

out:	/* All done */
	si_setcoreidx(sih, idx);

	return ret;
}

static int
BCMNMIATTACHFN(hndotp_read_region)(void *oh, int region, uint16 *data, uint *wlen)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint32 idx, st;
	chipcregs_t *cc;
	int i;

	/* Only support HW region (no active chips use HND OTP SW region) */
	ASSERT(region == OTP_HW_REGION);

	OTP_MSG(("%s: region %x wlen %d\n", __FUNCTION__, region, *wlen));

	/* Region empty? */
	st = oi->hwprot | oi-> signvalid;
	if ((st & region) == 0)
		return BCME_NOTFOUND;

	*wlen = ((int)*wlen < oi->boundary/2) ? *wlen : (uint)oi->boundary/2;

	idx = si_coreidx(oi->sih);
	cc = si_setcoreidx(oi->sih, SI_CC_IDX);
	ASSERT(cc != NULL);

	for (i = 0; i < (int)*wlen; i++)
		data[i] = hndotp_otpr(oh, cc, i);

	si_setcoreidx(oi->sih, idx);

	return 0;
}

static int
BCMNMIATTACHFN(hndotp_read_word)(void *oh, uint wn, uint16 *data)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint32 idx;
	chipcregs_t *cc;

	idx = si_coreidx(oi->sih);
	cc = si_setcoreidx(oi->sih, SI_CC_IDX);
	ASSERT(cc != NULL);

	*data = hndotp_otpr(oh, cc, wn);

	si_setcoreidx(oi->sih, idx);
	return 0;
}

static int
BCMNMIATTACHFN(hndotp_nvread)(void *oh, char *data, uint *len)
{
	int rc = 0;
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint32 base, bound, lim = 0, st;
	int i, chunk, gchunks, tsz = 0;
	uint32 idx;
	chipcregs_t *cc;
	uint offset;
	uint16 *rawotp = NULL;

	/* save the orig core */
	idx = si_coreidx(oi->sih);
	cc = si_setcoreidx(oi->sih, SI_CC_IDX);
	ASSERT(cc != NULL);

	st = hndotp_status(oh);
	if (!(st & (OTP_HW_REGION | OTP_SW_REGION))) {
		OTP_ERR(("OTP not programmed\n"));
		rc = -1;
		goto out;
	}

	/* Read the whole otp so we can easily manipulate it */
	lim = hndotp_size(oh);
	if (lim == 0) {
		OTP_ERR(("OTP size is 0\n"));
		rc = -1;
		goto out;
	}
	if ((rawotp = MALLOC(si_osh(oi->sih), lim)) == NULL) {
		OTP_ERR(("Out of memory for rawotp\n"));
		rc = -2;
		goto out;
	}
	for (i = 0; i < (int)(lim / 2); i++)
		rawotp[i] = hndotp_otpr(oh, cc,  i);

	if ((st & OTP_HW_REGION) == 0) {
		OTP_ERR(("otp: hw region not written (0x%x)\n", st));

		/* This could be a programming failure in the first
		 * chunk followed by one or more good chunks
		 */
		for (i = 0; i < (int)(lim / 2); i++)
			if (rawotp[i] == OTP_MAGIC)
				break;

		if (i < (int)(lim / 2)) {
			base = i;
			bound = (i * 2) + rawotp[i + 1];
			OTP_MSG(("otp: trying chunk at 0x%x-0x%x\n", i * 2, bound));
		} else {
			OTP_MSG(("otp: unprogrammed\n"));
			rc = -3;
			goto out;
		}
	} else {
		bound = rawotp[(lim / 2) + OTP_BOUNDARY_OFF];

		/* There are two cases: 1) The whole otp is used as nvram
		 * and 2) There is a hardware header followed by nvram.
		 */
		if (rawotp[0] == OTP_MAGIC) {
			base = 0;
			if (bound != rawotp[1])
				OTP_MSG(("otp: Bound 0x%x != chunk0 len 0x%x\n", bound,
				         rawotp[1]));
		} else
			base = bound;
	}

	/* Find and copy the data */

	chunk = 0;
	gchunks = 0;
	i = base / 2;
	offset = 0;
	while ((i < (int)(lim / 2)) && (rawotp[i] == OTP_MAGIC)) {
		int dsz, rsz = rawotp[i + 1];

		if (((i * 2) + rsz) >= (int)lim) {
			OTP_MSG(("  bad chunk size, chunk %d, base 0x%x, size 0x%x\n",
			         chunk, i * 2, rsz));
			/* Bad length, try to find another chunk anyway */
			rsz = 6;
		}
		if (hndcrc16((uint8 *)&rawotp[i], rsz,
		             CRC16_INIT_VALUE) == CRC16_GOOD_VALUE) {
			/* Good crc, copy the vars */
			OTP_MSG(("  good chunk %d, base 0x%x, size 0x%x\n",
			         chunk, i * 2, rsz));
			gchunks++;
			dsz = rsz - 6;
			tsz += dsz;
			if (offset + dsz >= *len) {
				OTP_MSG(("Out of memory for otp\n"));
				goto out;
			}
			bcopy((char *)&rawotp[i + 2], &data[offset], dsz);
			offset += dsz;
			/* Remove extra null characters at the end */
			while (offset > 1 &&
			       data[offset - 1] == 0 && data[offset - 2] == 0)
				offset --;
			i += rsz / 2;
		} else {
			/* bad length or crc didn't check, try to find the next set */
			OTP_MSG(("  chunk %d @ 0x%x size 0x%x: bad crc, ",
			         chunk, i * 2, rsz));
			if (rawotp[i + (rsz / 2)] == OTP_MAGIC) {
				/* Assume length is good */
				i += rsz / 2;
			} else {
				while (++i < (int)(lim / 2))
					if (rawotp[i] == OTP_MAGIC)
						break;
			}
			if (i < (int)(lim / 2))
				OTP_MSG(("trying next base 0x%x\n", i * 2));
			else
				OTP_MSG(("no more chunks\n"));
		}
		chunk++;
	}

	OTP_MSG(("  otp size = %d, boundary = 0x%x, nv base = 0x%x\n", lim, bound, base));
	if (tsz != 0) {
		OTP_MSG(("  Found %d bytes in %d good chunks out of %d\n", tsz, gchunks, chunk));
	} else {
		OTP_MSG(("  No good chunks found out of %d\n", chunk));
	}

	*len = offset;

out:
	if (rawotp)
		MFREE(si_osh(oi->sih), rawotp, lim);
	si_setcoreidx(oi->sih, idx);

	return rc;
}

#endif /* !defined(BCMDONGLEHOST) */


static otp_fn_t hndotp_fn = {
	(otp_size_t)hndotp_size,
	(otp_read_bit_t)hndotp_read_bit,
	(otp_dump_t)NULL,		/* Assigned in otp_init */
	(otp_status_t)hndotp_status,

#if !defined(BCMDONGLEHOST)
	(otp_init_t)hndotp_init,
	(otp_read_region_t)hndotp_read_region,
	(otp_nvread_t)hndotp_nvread,
	(otp_write_region_t)NULL,
	(otp_cis_append_region_t)NULL,
	(otp_lock_t)NULL,
	(otp_nvwrite_t)NULL,
	(otp_write_word_t)NULL,
	(otp_read_word_t)hndotp_read_word,
#endif /* !defined(BCMDONGLEHOST) */
#if !defined(BCMDONGLEHOST)
	(otp_write_bits_t)NULL
#endif /* !BCMDONGLEHOST */
};

#endif /* BCMHNDOTP */

/*
 * Common Code: Compiled for IPX / HND / AUTO
 *	otp_status()
 *	otp_size()
 *	otp_read_bit()
 *	otp_init()
 * 	otp_read_region()
 * 	otp_read_word()
 * 	otp_nvread()
 * 	otp_write_region()
 * 	otp_write_word()
 * 	otp_cis_append_region()
 * 	otp_lock()
 * 	otp_nvwrite()
 * 	otp_dump()
 */

int
BCMNMIATTACHFN(otp_status)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;

	return oi->fn->status(oh);
}

int
BCMNMIATTACHFN(otp_size)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;

	return oi->fn->size(oh);
}

uint16
BCMNMIATTACHFN(otp_read_bit)(void *oh, uint offset)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint idx = si_coreidx(oi->sih);
	chipcregs_t *cc = si_setcoreidx(oi->sih, SI_CC_IDX);
	uint16 readBit = (uint16)oi->fn->read_bit(oh, cc, offset);
	si_setcoreidx(oi->sih, idx);
	return readBit;
}


void *
BCMNMIATTACHFN(otp_init)(si_t *sih)
{
	otpinfo_t *oi;
	void *ret = NULL;
#if !defined(BCMDONGLEHOST)
	uint32 min_res_mask = 0;
	bool wasup = FALSE;
#endif

	oi = get_otpinfo();
	bzero(oi, sizeof(otpinfo_t));

	oi->ccrev = sih->ccrev;

#ifdef BCMIPXOTP
	if (OTPTYPE_IPX(oi->ccrev)) {
		oi->fn = get_ipxotp_fn();
	}
#endif /* BCMIPXOTP */

#ifdef BCMHNDOTP
	if (OTPTYPE_HND(oi->ccrev)) {
		oi->fn = &hndotp_fn;
	}
#endif /* BCMHNDOTP */

	if (oi->fn == NULL) {
		OTP_ERR(("otp_init: unsupported OTP type\n"));
		return NULL;
	}

	oi->sih = sih;
	oi->osh = si_osh(oi->sih);

#if !defined(BCMDONGLEHOST)
	if (!(wasup = si_is_otp_powered(sih)))
		si_otp_power(sih, TRUE, &min_res_mask);

	ret = (oi->fn->init)(sih);

	if (!wasup)
		si_otp_power(sih, FALSE, &min_res_mask);
#endif

	return ret;
}

#if !defined(BCMDONGLEHOST)
int
BCMNMIATTACHFN(otp_newcis)(void *oh)
{
	int ret = 0;
#if defined(BCMIPXOTP)
	otpinfo_t *oi = (otpinfo_t *)oh;
	int otpgu_bit_base = oi->otpgu_base * 16;

	ret = otp_read_bit(oh, otpgu_bit_base + OTPGU_NEWCISFORMAT_OFF);
	OTP_MSG(("New Cis format bit %d value: %x\n", otpgu_bit_base + OTPGU_NEWCISFORMAT_OFF,
		ret));
#endif

	return ret;
}

int
BCMNMIATTACHFN(otp_read_region)(si_t *sih, int region, uint16 *data, uint *wlen)
{
	bool wasup = FALSE;
	void *oh;
	int err = 0;
	uint32 min_res_mask = 0;

	if (!(wasup = si_is_otp_powered(sih)))
		si_otp_power(sih, TRUE, &min_res_mask);

	if (!si_is_otp_powered(sih) || si_is_otp_disabled(sih)) {
		err = BCME_NOTREADY;
		goto out;
	}

	oh = otp_init(sih);
	if (oh == NULL) {
		OTP_ERR(("otp_init failed.\n"));
		err = BCME_ERROR;
		goto out;
	}

	err = (((otpinfo_t*)oh)->fn->read_region)(oh, region, data, wlen);

out:
	if (!wasup)
		si_otp_power(sih, FALSE, &min_res_mask);

	return err;
}

int
BCMNMIATTACHFN(otp_read_word)(si_t *sih, uint wn, uint16 *data)
{
	bool wasup = FALSE;
	void *oh;
	int err = 0;
	uint32 min_res_mask = 0;

	if (!(wasup = si_is_otp_powered(sih)))
		si_otp_power(sih, TRUE, &min_res_mask);

	if (!si_is_otp_powered(sih) || si_is_otp_disabled(sih)) {
		err = BCME_NOTREADY;
		goto out;
	}

	oh = otp_init(sih);
	if (oh == NULL) {
		OTP_ERR(("otp_init failed.\n"));
		err = BCME_ERROR;
		goto out;
	}

	if (((otpinfo_t*)oh)->fn->read_word == NULL) {
		err = BCME_UNSUPPORTED;
		goto out;
	}
	err = (((otpinfo_t*)oh)->fn->read_word)(oh, wn, data);

out:
	if (!wasup)
		si_otp_power(sih, FALSE, &min_res_mask);

	return err;
}

int
BCMNMIATTACHFN(otp_nvread)(void *oh, char *data, uint *len)
{
	otpinfo_t *oi = (otpinfo_t *)oh;

	return oi->fn->nvread(oh, data, len);
}



#endif /* !defined(BCMDONGLEHOST) */
