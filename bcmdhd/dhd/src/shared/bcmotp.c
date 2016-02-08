/*
 * OTP support.
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: bcmotp.c 529771 2015-01-28 09:06:04Z $
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
#include <sbgci.h>
#include <bcmotp.h>
#include <pcicfg.h>
#include "siutils_priv.h"
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
#define OTPPROWMASK(ccrev)	((ccrev >= 49) ? OTPP_ROW_MASK9 : OTPP_ROW_MASK)

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

/* OTP common registers */
typedef struct {
	volatile uint32	*otpstatus;
	volatile uint32	*otpcontrol;
	volatile uint32	*otpprog;
	volatile uint32	*otplayout;
	volatile uint32	*otpcontrol1;
	volatile uint32	*otplayoutextension;
} otpregs_t;

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
typedef bool	(*otp_isunified_t)(void *oh);
typedef uint16	(*otp_avsbitslen_t)(void *oh);

/* OTP function struct */
typedef struct otp_fn_s {
	otp_size_t		size;
	otp_read_bit_t		read_bit;
	otp_dump_t		dump;
	otp_status_t		status;

	otp_isunified_t		isunified;
	otp_avsbitslen_t	avsbitslen;
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
	volatile uint16	*otpbase;	/* Cache OTP Base address */
	uint16		avsbitslen;	/* Number of bits used for AVS in sw region */
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

static void
otp_initregs(si_t *sih, void *coreregs, otpregs_t *otpregs)
{
	if (AOB_ENAB(sih)) {
		gciregs_t *gciregs = (gciregs_t *)coreregs;

		otpregs->otpstatus = &gciregs->otpstatus;
		otpregs->otpcontrol = &gciregs->otpcontrol;
		otpregs->otpprog = &gciregs->otpprog;
		otpregs->otplayout = &gciregs->otplayout;
		otpregs->otpcontrol1 = &gciregs->otpcontrol1;
		otpregs->otplayoutextension = &gciregs->otplayoutextension;
	} else {
		chipcregs_t *ccregs = (chipcregs_t *)coreregs;

		otpregs->otpstatus = &ccregs->otpstatus;
		otpregs->otpcontrol = &ccregs->otpcontrol;
		otpregs->otpprog = &ccregs->otpprog;
		otpregs->otplayout = &ccregs->otplayout;
		otpregs->otpcontrol1 = &ccregs->otpcontrol1;
		otpregs->otplayoutextension = &ccregs->otplayoutextension;
	}
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
 *	ipxotp_isunified()
 *	ipxotp_avsbitslen()
 *
 *   IPX internal functions:
 *	ipxotp_otpr()
 *	_ipxotp_init()
 *	_ipxotp_read_bit()
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
#define OTP_SZ_FU_1296          ((ROUNDUP(1296, 16))/8)
#define OTP_SZ_FU_972		((ROUNDUP(972, 16))/8)
#define OTP_SZ_FU_720		((ROUNDUP(720, 16))/8)
#define OTP_SZ_FU_468		((ROUNDUP(468, 16))/8)
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
#define OTP_BT_BASE_4349	(9744/OTPWSIZE)
#define OTP_BT_END_4349		(11792/OTPWSIZE)

/* AVS Regions Size */
#define AVS_BITS_LEN	32

/* OTP unification */
#if defined(USBSDIOUNIFIEDOTP)
/** Offset in OTP from upper GUR to HNBU_UMANFID tuple value in (16-bit) words */
#define USB_MANIFID_OFFSET_4319		42
#define USB_MANIFID_OFFSET_43143	45 /* See Confluence 43143 SW notebook #1 */
#endif /* USBSDIOUNIFIEDOTP */


static otp_fn_t* get_ipxotp_fn(void);

static int
BCMNMIATTACHFN(ipxotp_status)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return (int)(oi->status);
}

/** Returns number of bits used for avs at the end of sw region */
static uint16
BCMNMIATTACHFN(ipxotp_avsbitslen)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return oi->avsbitslen;
}

/** Returns if otp is unified */
static bool
BCMNMIATTACHFN(ipxotp_isunified)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return oi->buotp;
}

/** Returns size in bytes */
static int
BCMNMIATTACHFN(ipxotp_size)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return (int)oi->wsize * 2;
}

static uint16
BCMNMIATTACHFN(ipxotp_read_bit_common)(void *oh, otpregs_t *otpregs, uint off)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint k, row, col;
	uint32 otpp, st;
	uint otpwt;

	otpwt = (R_REG(oi->osh, otpregs->otplayout) & OTPL_WRAP_TYPE_MASK) >> OTPL_WRAP_TYPE_SHIFT;

	row = off / oi->cols;
	col = off % oi->cols;

	otpp = OTPP_START_BUSY |
		((((otpwt == OTPL_WRAP_TYPE_40NM)? OTPPOC_READ_40NM :
		OTPPOC_READ) << OTPP_OC_SHIFT) & OTPP_OC_MASK) |
	        ((row << OTPP_ROW_SHIFT) & OTPPROWMASK(oi->sih->ccrev)) |
	        ((col << OTPP_COL_SHIFT) & OTPP_COL_MASK);
	OTP_DBG(("%s: off = %d, row = %d, col = %d, otpp = 0x%x",
	         __FUNCTION__, off, row, col, otpp));
	W_REG(oi->osh, otpregs->otpprog, otpp);

	for (k = 0;
	     ((st = R_REG(oi->osh, otpregs->otpprog)) & OTPP_START_BUSY) && (k < OTPP_TRIES);
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
BCMNMIATTACHFN(_ipxotp_read_bit)(void *oh, otpregs_t *otpregs, uint off)
{
	otpinfo_t *oi = (otpinfo_t *)oh;

	W_REG(oi->osh, otpregs->otpcontrol, 0);

	if (oi->sih->ccrev >= 49) {
		AND_REG(oi->osh, otpregs->otpcontrol1,
			(OTPC1_CLK_EN_MASK | OTPC1_CLK_DIV_MASK));
	} else {
		W_REG(oi->osh, otpregs->otpcontrol1, 0);
	}

	return ipxotp_read_bit_common(oh, otpregs, off);
}

static uint16
BCMNMIATTACHFN(ipxotp_read_bit)(void *oh, chipcregs_t *cc, uint off)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	si_t *sih = oi->sih;
	uint idx;
	void *regs;
	otpregs_t otpregs;
	uint16 val16;

	idx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		regs = si_setcore(sih, GCI_CORE_ID, 0);
	} else {
		regs = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(regs != NULL);
	otp_initregs(sih, regs, &otpregs);

	val16 = _ipxotp_read_bit(oh, &otpregs, off);

	si_setcoreidx(sih, idx);
	return (val16);
}




static otp_fn_t BCMNMIATTACHDATA(ipxotp_fn) = {
	(otp_size_t)ipxotp_size,
	(otp_read_bit_t)ipxotp_read_bit,
	(otp_dump_t)NULL,		/* Assigned in otp_init */
	(otp_status_t)ipxotp_status,

	(otp_isunified_t)ipxotp_isunified,
	(otp_avsbitslen_t)ipxotp_avsbitslen
};

/*
 * ROM accessor to avoid struct in shdat. Declare BCMRAMFN() to force the accessor to be excluded
 * from ROM.
 */
static otp_fn_t*
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
 *	hndotp_isunified()
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

static uint16
BCMNMIATTACHFN(hndotp_avsbitslen)(void *oh)
{
	return 0;
}

static bool
BCMNMIATTACHFN(hndotp_isunified)(void *oh)
{
	return FALSE;
}

static int
BCMNMIATTACHFN(hndotp_size)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return ((int)(oi->size));
}



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



static otp_fn_t hndotp_fn = {
	(otp_size_t)hndotp_size,
	(otp_read_bit_t)hndotp_read_bit,
	(otp_dump_t)NULL,		/* Assigned in otp_init */
	(otp_status_t)hndotp_status,

	(otp_isunified_t)hndotp_isunified,
	(otp_avsbitslen_t)hndotp_avsbitslen
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

uint16
BCMNMIATTACHFN(otp_avsbitslen)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;

	return oi->fn->avsbitslen(oh);
}

bool
BCMNMIATTACHFN(otp_isunified)(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;

	return oi->fn->isunified(oh);
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


	return ret;
}
