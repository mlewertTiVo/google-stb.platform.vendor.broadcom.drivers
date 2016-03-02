/*
 * Routines to access SPROM and to parse SROM/CIS variables.
 *
 * Despite its file name, OTP contents is also parsed in this file.
 *
 * Copyright (C) 1999-2016, Broadcom Corporation
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
 * $Id: bcmsrom.c 620402 2016-02-23 01:33:41Z $
 */

/*
 * List of non obvious preprocessor defines used in this file and their meaning:
 * DONGLEBUILD    : building firmware that runs on the dongle's CPU
 * BCM_DONGLEVARS : NVRAM variables can be read from OTP/S(P)ROM.
 * When host may supply nvram vars in addition to the ones in OTP/SROM:
 * 	BCMHOSTVARS    		: full nic / full dongle
 * 	BCM_BMAC_VARS_APPEND	: BMAC
 * BCMDONGLEHOST  : defined when building DHD, code executes on the host in a dongle environment.
 * DHD_SPROM      : defined when building a DHD that supports reading/writing to SPROM
 */

#include <bcm_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#if defined(__FreeBSD__)
#include <machine/stdarg.h>
#else
#include <stdarg.h>
#endif
#include <bcmutils.h>
#include <hndsoc.h>
#include <sbchipc.h>
#include <bcmdevs.h>
#include <bcmendian.h>
#include <sbpcmcia.h>
#include <pcicfg.h>
#include <siutils.h>
#include <bcmsrom.h>
#include <bcmsrom_tbl.h>

#include <bcmnvram.h>
#include <bcmotp.h>
#ifndef BCMUSBDEV_COMPOSITE
#define BCMUSBDEV_COMPOSITE
#endif

#include <proto/ethernet.h>	/* for sprom content groking */

#include <sbgci.h>
#define	BS_ERROR(args)

/** curmap: contains host start address of PCI BAR0 window */
static uint8* srom_offset(si_t *sih, void *curmap)
{
	if (sih->ccrev <= 31)
		return (uint8 *)curmap + PCI_BAR0_SPROM_OFFSET;
	if ((sih->cccaps & CC_CAP_SROM) == 0)
		return NULL;
#if !defined(DSLCPE_WOMBO)
	if (BUSTYPE(sih->bustype) == SI_BUS)
		return (uint8 *)(SI_ENUM_BASE + CC_SROM_OTP);
#endif 
	return (uint8 *)curmap + PCI_16KB0_CCREGS_OFFSET + CC_SROM_OTP;
}


typedef struct varbuf {
	char *base;		/* pointer to buffer base */
	char *buf;		/* pointer to current position */
	unsigned int size;	/* current (residual) size in bytes */
} varbuf_t;
extern char *_vars;
extern uint _varsz;
#define DONGLE_STORE_VARS_OTP_PTR(v)

#define SROM_CIS_SINGLE	1


static int sprom_cmd_pcmcia(osl_t *osh, uint8 cmd);
static int sprom_read_pcmcia(osl_t *osh, uint16 addr, uint16 *data);
static int sprom_read_pci(osl_t *osh, si_t *sih, uint16 *sprom, uint wordoff, uint16 *buf,
                          uint nwords, bool check_crc);
static uint16 srom_cc_cmd(si_t *sih, osl_t *osh, void *ccregs, uint32 cmd, uint wordoff,
                          uint16 data);





/* BCMHOSTVARS is enabled only if WLTEST is enabled or BCMEXTNVM is enabled */
#if defined(BCMHOSTVARS)
/* Also used by wl_readconfigdata for vars download */
char BCMATTACHDATA(mfgsromvars)[VARS_MAX];
int BCMATTACHDATA(defvarslen) = 0;
#endif 



static bool srvars_inited = FALSE; /* Use OTP/SROM as global variables */

/* BCMHOSTVARS is enabled only if WLTEST is enabled or BCMEXTNVM is enabled */
#if (defined(BCMUSBDEV_BMAC) || defined(BCM_BMAC_VARS_APPEND))
/* It must end with pattern of "END" */
static uint
BCMATTACHFN(srom_vars_len)(char *vars)
{
	uint pos = 0;
	uint len;
	char *s;
	char *emark = "END";
	uint emark_len = strlen(emark) + 1;

	for (s = vars; s && *s;) {

		if (strcmp(s, emark) == 0)
			break;

		len = strlen(s);
		s += strlen(s) + 1;
		pos += len + 1;
		/* BS_ERROR(("len %d vars[pos] %s\n", pos, s)); */
		if (pos >= (VARS_MAX - emark_len)) {
			return 0;
		}
	}

	return pos + emark_len;	/* include the "END\0" */
}
#endif 


/** support only 16-bit word read from srom */
int
srom_read(si_t *sih, uint bustype, void *curmap, osl_t *osh,
          uint byteoff, uint nbytes, uint16 *buf, bool check_crc)
{
	uint i, off, nw;

	ASSERT(bustype == BUSTYPE(bustype));

	/* check input - 16-bit access only */
	if (byteoff & 1 || nbytes & 1 || (byteoff + nbytes) > SROM_MAX)
		return 1;

	off = byteoff / 2;
	nw = nbytes / 2;

#ifdef BCMPCIEDEV
	if ((BUSTYPE(bustype) == SI_BUS) &&
	    (BCM43602_CHIP(sih->chip) ||
	     (CHIPID(sih->chip) == BCM4365_CHIP_ID) ||
	     (CHIPID(sih->chip) == BCM4366_CHIP_ID) ||
	     0)) {
#else
	if (BUSTYPE(bustype) == PCI_BUS) {
#endif /* BCMPCIEDEV */
		if (!curmap)
			return 1;

		if (si_is_sprom_available(sih)) {
			uint16 *srom;

			srom = (uint16 *)srom_offset(sih, curmap);
			if (srom == NULL)
				return 1;

			if (sprom_read_pci(osh, sih, srom, off, buf, nw, check_crc))
				return 1;
		}
	} else if (BUSTYPE(bustype) == PCMCIA_BUS) {
		for (i = 0; i < nw; i++) {
			if (sprom_read_pcmcia(osh, (uint16)(off + i), (uint16 *)(buf + i)))
				return 1;
		}
	} else if (BUSTYPE(bustype) == SI_BUS) {

		return 1;
	} else {
		return 1;
	}

	return 0;
}



/**
 * These 'vstr_*' definitions are used to convert from CIS format to a 'NVRAM var=val' format, the
 * NVRAM format is used throughout the rest of the firmware.
 */

/** set PCMCIA sprom command register */
static int
sprom_cmd_pcmcia(osl_t *osh, uint8 cmd)
{
	uint8 status = 0;
	uint wait_cnt = 1000;

	/* write sprom command register */
	OSL_PCMCIA_WRITE_ATTR(osh, SROM_CS, &cmd, 1);

	/* wait status */
	while (wait_cnt--) {
		OSL_PCMCIA_READ_ATTR(osh, SROM_CS, &status, 1);
		if (status & SROM_DONE)
			return 0;
	}

	return 1;
}

/** read a word from the PCMCIA srom */
static int
sprom_read_pcmcia(osl_t *osh, uint16 addr, uint16 *data)
{
	uint8 addr_l, addr_h, data_l, data_h;

	addr_l = (uint8)((addr * 2) & 0xff);
	addr_h = (uint8)(((addr * 2) >> 8) & 0xff);

	/* set address */
	OSL_PCMCIA_WRITE_ATTR(osh, SROM_ADDRH, &addr_h, 1);
	OSL_PCMCIA_WRITE_ATTR(osh, SROM_ADDRL, &addr_l, 1);

	/* do read */
	if (sprom_cmd_pcmcia(osh, SROM_READ))
		return 1;

	/* read data */
	data_h = data_l = 0;
	OSL_PCMCIA_READ_ATTR(osh, SROM_DATAH, &data_h, 1);
	OSL_PCMCIA_READ_ATTR(osh, SROM_DATAL, &data_l, 1);

	*data = (data_h << 8) | data_l;
	return 0;
}


/**
 * In chips with chipcommon rev 32 and later, the srom is in chipcommon,
 * not in the bus cores.
 */
static uint16
srom_cc_cmd(si_t *sih, osl_t *osh, void *ccregs, uint32 cmd, uint wordoff, uint16 data)
{
	chipcregs_t *cc = (chipcregs_t *)ccregs;
	uint wait_cnt = 1000;

	if ((cmd == SRC_OP_READ) || (cmd == SRC_OP_WRITE)) {
		W_REG(osh, &cc->sromaddress, wordoff * 2);
		if (cmd == SRC_OP_WRITE)
			W_REG(osh, &cc->sromdata, data);
	}

	W_REG(osh, &cc->sromcontrol, SRC_START | cmd);

	while (wait_cnt--) {
		if ((R_REG(osh, &cc->sromcontrol) & SRC_BUSY) == 0)
			break;
	}

	if (!wait_cnt) {
		BS_ERROR(("%s: Command 0x%x timed out\n", __FUNCTION__, cmd));
		return 0xffff;
	}
	if (cmd == SRC_OP_READ)
		return (uint16)R_REG(osh, &cc->sromdata);
	else
		return 0xffff;
}

static int
sprom_read_pci(osl_t *osh, si_t *sih, uint16 *sprom, uint wordoff, uint16 *buf, uint nwords,
	bool check_crc)
{
	int err = 0;
	uint i;
	void *ccregs = NULL;
	uint32 ccval = 0;

	if ((CHIPID(sih->chip) == BCM4331_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43431_CHIP_ID) ||
	    BCM43602_CHIP(sih->chip) ||
	    (CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43460_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM4352_CHIP_ID)) {
		/* save current control setting */
		ccval = si_chipcontrl_read(sih);
	}

	if ((CHIPID(sih->chip) == BCM4331_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43431_CHIP_ID)) {
		/* Disable Ext PA lines to allow reading from SROM */
		si_chipcontrl_epa4331(sih, FALSE);
	} else if (BCM43602_CHIP(sih->chip) ||
		(((CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43460_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4352_CHIP_ID)) &&
		(CHIPREV(sih->chiprev) <= 2))) {
		si_chipcontrl_srom4360(sih, TRUE);
	}

	if ((CHIPID(sih->chip) == BCM4365_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4366_CHIP_ID)) {
		si_clk_srom4365(sih);
	}

	/* read the sprom */
	for (i = 0; i < nwords; i++) {

		if ((sih->ccrev > 31 && ISSIM_ENAB(sih)) ||
			(sih->ccrev == 53)) {
			/* use indirect since direct is too slow on QT */
			/* Also enable indirect access with 4365 for SROMREV13 */
			if ((sih->cccaps & CC_CAP_SROM) == 0) {
				err = 1;
				goto error;
			}

			ccregs = (void *)((uint8 *)sprom - CC_SROM_OTP);
			buf[i] = srom_cc_cmd(sih, osh, ccregs, SRC_OP_READ, wordoff + i, 0);

		} else {
			if (ISSIM_ENAB(sih))
				buf[i] = R_REG(osh, &sprom[wordoff + i]);

			buf[i] = R_REG(osh, &sprom[wordoff + i]);
		}

	}

	/* bypass crc checking for simulation to allow srom hack */
	if (ISSIM_ENAB(sih)) {
		goto error;
	}

	if (check_crc) {

		if (buf[0] == 0xffff) {
			/* The hardware thinks that an srom that starts with 0xffff
			 * is blank, regardless of the rest of the content, so declare
			 * it bad.
			 */
			BS_ERROR(("%s: buf[0] = 0x%x, returning bad-crc\n", __FUNCTION__, buf[0]));
			err = 1;
			goto error;
		}

		/* fixup the endianness so crc8 will pass */
		htol16_buf(buf, nwords * 2);
		if (hndcrc8((uint8 *)buf, nwords * 2, CRC8_INIT_VALUE) != CRC8_GOOD_VALUE) {
			/* DBG only pci always read srom4 first, then srom8/9 */
			/* BS_ERROR(("%s: bad crc\n", __FUNCTION__)); */
			err = 1;
		}
		/* now correct the endianness of the byte array */
		ltoh16_buf(buf, nwords * 2);
	}

error:
	if ((CHIPID(sih->chip) == BCM4331_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43431_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43460_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM4352_CHIP_ID) ||
	    BCM43602_CHIP(sih->chip) ||
	    BCM4350_CHIP(sih->chip)) {
		/* Restore config after reading SROM */
		si_chipcontrl_restore(sih, ccval);
	}

	return err;
}


int
srom_otp_write_region_crc(si_t *sih, uint nbytes, uint16* buf16, bool write)
{
	return 0;
}


#if !defined(BCMUSBDEV_ENABLED) && !defined(BCMSDIODEV_ENABLED) && \
	!defined(BCMPCIEDEV_ENABLED)
#endif	





/**
 * initvars are different for BCMUSBDEV and BCMSDIODEV.  This is OK when supporting both at
 * the same time, but only because all of the code is in attach functions and not in ROM.
 */

#if defined(BCMUSBDEV_ENABLED)
#if defined(BCMUSBDEV_BMAC) || defined(BCM_BMAC_VARS_APPEND)
/**
 * Read the cis and call parsecis to allocate and initialize the NVRAM vars buffer.
 * OTP only. Return 0 on success (*vars points to NVRAM buffer), nonzero on error.
 */
static int
BCMATTACHFN(initvars_cis_usbdriver)(si_t *sih, osl_t *osh, char **vars, uint *count)
{
	uint8 *cis;
	uint sz = OTP_SZ_MAX/2; /* size in words */
	int rc = BCME_OK;

	if ((cis = MALLOC(osh, OTP_SZ_MAX)) == NULL) {
		return -1;
	}

	bzero(cis, OTP_SZ_MAX);

	if (otp_read_region(sih, OTP_SW_RGN, (uint16 *)cis, &sz)) {
		BS_ERROR(("%s: OTP read SW region failure.\n*", __FUNCTION__));
		rc = -2;
	} else {
		BS_ERROR(("%s: OTP programmed. use OTP for srom vars\n*", __FUNCTION__));
		rc = srom_parsecis(sih, osh, &cis, SROM_CIS_SINGLE, vars, count);
	}

	MFREE(osh, cis, OTP_SZ_MAX);

	return (rc);
}

/**
 * For driver(not bootloader), if nvram is not downloadable or missing, use default. Despite the
 * 'srom' in the function name, this function only deals with OTP.
 */
static int
BCMATTACHFN(initvars_srom_si_usbdriver)(si_t *sih, osl_t *osh, char **vars, uint *varsz)
{
	uint len;
	char *base;
	char *fakevars;
	int rc = -1;

	base = fakevars = NULL;
	len = 0;
	switch (CHIPID(sih->chip)) {
		case BCM4322_CHIP_ID:   case BCM43221_CHIP_ID:  case BCM43231_CHIP_ID:
			fakevars = defaultsromvars_4322usb;
			break;
		case BCM43236_CHIP_ID: case BCM43235_CHIP_ID:  case BCM43238_CHIP_ID:
		case BCM43234_CHIP_ID:
			/* check against real chipid instead of compile time flag */
			if (CHIPID(sih->chip) == BCM43234_CHIP_ID) {
				fakevars = defaultsromvars_43234usb;
			} else if (CHIPID(sih->chip) == BCM43235_CHIP_ID) {
				fakevars = defaultsromvars_43235usb;
			} else
				fakevars = defaultsromvars_43236usb;
			break;

		case BCM4319_CHIP_ID:
			fakevars = defaultsromvars_4319usb;
			break;
		case BCM43242_CHIP_ID:
		case BCM43243_CHIP_ID:
			fakevars = defaultsromvars_43242usb;
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
		case BCM4358_CHIP_ID:
			fakevars = defaultsromvars_4350usb;
			break;

		case BCM4360_CHIP_ID:
		case BCM4352_CHIP_ID:
			fakevars = defaultsromvars_4360;
			break;
		case BCM43460_CHIP_ID:
		case BCM43526_CHIP_ID:
			fakevars = defaultsromvars_4360usb;
			break;
		case BCM43143_CHIP_ID:
			fakevars = defaultsromvars_43143usb;
			break;
		CASE_BCM43602_CHIP: /* fall through: 43602 hasn't got a USB interface */
		default:
			ASSERT(0);
			return rc;
	}

#ifndef BCM_BMAC_VARS_APPEND
	if (BCME_OK == initvars_cis_usbdriver(sih, osh, vars, varsz)) {
		/* Make OTP variables global */
		if (srvars_inited == FALSE) {
			nvram_append((void *)sih, *vars, *varsz);
			DONGLE_STORE_VARS_OTP_PTR(*vars);
		}
		return BCME_OK;
	}
#endif /* BCM_BMAC_VARS_APPEND */

	/* NO OTP, if nvram downloaded, use it */
	if ((_varsz != 0) && (_vars != NULL)) {
		len  = _varsz + (strlen(vstr_end));
		base = MALLOC(osh, len + 2); /* plus 2 terminating \0 */
		if (base == NULL) {
			BS_ERROR(("initvars_srom_si: MALLOC failed.\n"));
			return BCME_ERROR;
		}
		bzero(base, len + 2);

		/* make a copy of the _vars, _vars is at the top of the memory, cannot append
		 * END\0\0 to it. copy the download vars to base, back of the terminating \0,
		 * then append END\0\0
		 */
		bcopy((void *)_vars, base, _varsz);
		/* backoff all the terminating \0s except the one the for the last string */
		len = _varsz;
		while (!base[len - 1])
			len--;
		len++; /* \0  for the last string */
		/* append END\0\0 to the end */
		bcopy((void *)vstr_end, (base + len), strlen(vstr_end));
		len += (strlen(vstr_end) + 2);
		*vars = base;
		*varsz = len;

		BS_ERROR(("%s USB nvram downloaded %d bytes\n", __FUNCTION__, _varsz));
	} else {
		/* Fall back to fake srom vars if OTP not programmed */
		len = srom_vars_len(fakevars);
		base = MALLOC(osh, (len + 1));
		if (base == NULL) {
			BS_ERROR(("initvars_srom_si: MALLOC failed.\n"));
			return BCME_ERROR;
		}
		bzero(base, (len + 1));
		bcopy(fakevars, base, len);
		*(base + len) = '\0';		/* add final nullbyte terminator */
		*vars = base;
		*varsz = len + 1;
		BS_ERROR(("initvars_srom_usbdriver: faked nvram %d bytes\n", len));
	}

#ifdef BCM_BMAC_VARS_APPEND
	if (BCME_OK == initvars_cis_usbdriver(sih, osh, vars, varsz)) { /* OTP only */
		if (base)
			MFREE(osh, base, (len + 1));
	}
#endif	/* BCM_BMAC_VARS_APPEND */
	/* Make OTP/SROM variables global */
	if (srvars_inited == FALSE) {
		nvram_append((void *)sih, *vars, *varsz);
		srvars_inited = TRUE;
		DONGLE_STORE_VARS_OTP_PTR(*vars);
	}
	return BCME_OK;

}
#endif /* BCMUSBDEV_BMAC || BCM_BMAC_VARS_APPEND */

#ifdef BCM_DONGLEVARS
/*** reads a CIS structure (so not an SROM-MAP structure) from either OTP or SROM */
static int
BCMATTACHFN(initvars_srom_si_bl)(si_t *sih, osl_t *osh, void *curmap, char **vars, uint *varsz)
{
	int sel = 0;		/* where to read srom/cis: 0 - none, 1 - otp, 2 - sprom */
	uint sz = 0;		/* srom size in bytes */
	void *oh = NULL;
	int rc = BCME_OK;

	if ((oh = otp_init(sih)) != NULL && (otp_status(oh) & OTPS_GUP_SW)) {
		/* Access OTP if it is present, powered on, and programmed */
		sz = otp_size(oh);
		sel = 1;
	} else if ((sz = srom_size(sih, osh)) != 0) {
		/* Access the SPROM if it is present */
		sz <<= 1;
		sel = 2;
	}

	/* Read CIS in OTP/SPROM */
	if (sel != 0) {
		uint16 *srom;
		uint8 *body = NULL;
		uint otpsz = sz;

		ASSERT(sz);

		/* Allocate memory */
		if ((srom = (uint16 *)MALLOC(osh, sz)) == NULL)
			return BCME_NOMEM;

		/* Read CIS */
		switch (sel) {
		case 1:
			rc = otp_read_region(sih, OTP_SW_RGN, srom, &otpsz);
			sz = otpsz;
			body = (uint8 *)srom;
			break;
		case 2:
			rc = srom_read(sih, SI_BUS, curmap, osh, 0, sz, srom, TRUE);
			/* sprom has 8 byte h/w header */
			body = (uint8 *)srom + SBSDIO_SPROM_CIS_OFFSET;
			break;
		default:
			/* impossible to come here */
			ASSERT(0);
			break;
		}

		/* Parse CIS */
		if (rc == BCME_OK) {
			/* each word is in host endian */
			htol16_buf((uint8 *)srom, sz);
			ASSERT(body);
			rc = srom_parsecis(sih, osh, &body, SROM_CIS_SINGLE, vars, varsz);
		}

		MFREE(osh, srom, sz);	/* Clean up */

		/* Make SROM variables global */
		if (rc == BCME_OK) {
			nvram_append((void *)sih, *vars, *varsz);
			DONGLE_STORE_VARS_OTP_PTR(*vars);
		}
	}

	return BCME_OK;
}
#endif	/* #ifdef BCM_DONGLEVARS */

/**
 * initvars_srom_si() is defined multiple times in this file. This is the 1st variant for chips with
 * an active USB interface. It is called only for bus types SI_BUS and JTAG_BUS, and only for CIS
 * format in SPROM and/or OTP. Reads OTP or SPROM (bootloader only) and appends parsed contents to
 * caller supplied var/value pairs.
 */
static int
BCMATTACHFN(initvars_srom_si)(si_t *sih, osl_t *osh, void *curmap, char **vars, uint *varsz)
{

	/* Bail out if we've dealt with OTP/SPROM before! */
	if (srvars_inited)
		goto exit;

#if defined(BCMUSBDEV_BMAC) || defined(BCM_BMAC_VARS_APPEND)
	/* read OTP or use faked var array */
	switch (CHIPID(sih->chip)) {
		case BCM4322_CHIP_ID:   case BCM43221_CHIP_ID:  case BCM43231_CHIP_ID:
		case BCM43236_CHIP_ID:  case BCM43235_CHIP_ID:  case BCM43238_CHIP_ID:
		case BCM43234_CHIP_ID:
		case BCM4319_CHIP_ID:
		case BCM43242_CHIP_ID:
		case BCM43243_CHIP_ID:
		case BCM4360_CHIP_ID:
		case BCM43460_CHIP_ID:
		case BCM43526_CHIP_ID:
		case BCM4352_CHIP_ID:
		case BCM4350_CHIP_ID:
		case BCM4354_CHIP_ID:
		case BCM4356_CHIP_ID:
		case BCM43556_CHIP_ID:
		case BCM43558_CHIP_ID:
		case BCM43566_CHIP_ID:
		case BCM43568_CHIP_ID:
		case BCM43569_CHIP_ID:
		case BCM43570_CHIP_ID:
		case BCM4358_CHIP_ID:
		case BCM43143_CHIP_ID:
		if (BCME_OK != initvars_srom_si_usbdriver(sih, osh, vars, varsz)) /* OTP only */
			goto exit;
		return BCME_OK;
		CASE_BCM43602_CHIP: /* 43602 does not support USB */
		default:
			;
	}
#endif  /* BCMUSBDEV_BMAC || BCM_BMAC_VARS_APPEND */

#ifdef BCM_DONGLEVARS	/* this flag should be defined for usb bootloader, to read OTP or \
	SROM */
	if (BCME_OK != initvars_srom_si_bl(sih, osh, curmap, vars, varsz)) /* CIS format only */
		return BCME_ERROR;
#endif

	/* update static local var to skip for next call */
	srvars_inited = TRUE;

exit:
	/* Tell the caller there is no individual SROM variables */
	*vars = NULL;
	*varsz = 0;

	/* return OK so the driver will load & use defaults if bad srom/otp */
	return BCME_OK;
}

#elif defined(BCMSDIODEV_ENABLED)

#ifdef BCM_DONGLEVARS
static uint8 BCMATTACHDATA(defcis4325)[] = { 0x20, 0x4, 0xd0, 0x2, 0x25, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4315)[] = { 0x20, 0x4, 0xd0, 0x2, 0x15, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4329)[] = { 0x20, 0x4, 0xd0, 0x2, 0x29, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4319)[] = { 0x20, 0x4, 0xd0, 0x2, 0x19, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4336)[] = { 0x20, 0x4, 0xd0, 0x2, 0x36, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4330)[] = { 0x20, 0x4, 0xd0, 0x2, 0x30, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis43237)[] = { 0x20, 0x4, 0xd0, 0x2, 0xe5, 0xa8, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4324)[] = { 0x20, 0x4, 0xd0, 0x2, 0x24, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4335)[] = { 0x20, 0x4, 0xd0, 0x2, 0x24, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4350)[] = { 0x20, 0x4, 0xd0, 0x2, 0x50, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis43143)[] = { 0x20, 0x4, 0xd0, 0x2, 0x87, 0xa8, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis43430)[] = { 0x20, 0x4, 0xd0, 0x2, 0xa6, 0xa9, 0xff, 0xff };

static uint8 BCMATTACHDATA(defcis4349)[] = { 0x20, 0x4, 0xd0, 0x2, 0x49, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4355)[] = { 0x20, 0x4, 0xd0, 0x2, 0x55, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis4359)[] = { 0x20, 0x4, 0xd0, 0x2, 0x59, 0x43, 0xff, 0xff };
static uint8 BCMATTACHDATA(defcis43909)[] = { 0x20, 0x4, 0xd0, 0x2, 0x85, 0xab, 0xff, 0xff };

#ifdef BCM_BMAC_VARS_APPEND

static char BCMATTACHDATA(defaultsromvars_4319sdio)[] =
	"sromrev=3\0"
	"vendid=0x14e4\0"
	"devid=0x4338\0"
	"boardtype=0x05a1\0"
	"boardrev=0x1102\0"
	"boardflags=0x400201\0"
	"boardflags2=0x80\0"
	"xtalfreq=26000\0"
	"aa2g=3\0"
	"aa5g=0\0"
	"ag0=0\0"
	"opo=0\0"
	"pa0b0=0x1675\0"
	"pa0b1=0xfa74\0"
	"pa0b2=0xfea1\0"
	"pa0itssit=62\0"
	"pa0maxpwr=78\0"
	"rssismf2g=0xa\0"
	"rssismc2g=0xb\0"
	"rssisav2g=0x3\0"
	"bxa2g=0\0"
	"cckdigfilttype=6\0"
	"rxpo2g=2\0"
	"cckpo=0\0"
	"ofdmpo=0x55553333\0"
	"mcs2gpo0=0x9999\0"
	"mcs2gpo1=0x9999\0"
	"mcs2gpo2=0x0000\0"
	"mcs2gpo3=0x0000\0"
	"mcs2gpo4=0x9999\0"
	"mcs2gpo5=0x9999\0"
	"macaddr=00:90:4c:06:c0:19\0"
	"END\0";

static char BCMATTACHDATA(defaultsromvars_4319sdio_hmb)[] =
	"sromrev=3\0"
	"vendid=0x14e4\0"
	"devid=0x4338\0"
	"boardtype=0x058c\0"
	"boardrev=0x1102\0"
	"boardflags=0x400201\0"
	"boardflags2=0x80\0"
	"xtalfreq=26000\0"
	"aa2g=3\0"
	"aa5g=0\0"
	"ag0=0\0"
	"opo=0\0"
	"pa0b0=0x1675\0"
	"pa0b1=0xfa74\0"
	"pa0b2=0xfea1\0"
	"pa0itssit=62\0"
	"pa0maxpwr=78\0"
	"rssismf2g=0xa \0"
	"rssismc2g=0xb \0"
	"rssisav2g=0x3 \0"
	"bxa2g=0\0"
	"cckdigfilttype=6\0"
	"rxpo2g=2\0"
	"cckpo=0\0"
	"ofdmpo=0x55553333\0"
	"mcs2gpo0=0x9999\0"
	"mcs2gpo1=0x9999\0"
	"mcs2gpo2=0x0000\0"
	"mcs2gpo3=0x0000\0"
	"mcs2gpo4=0x9999\0"
	"mcs2gpo5=0x9999\0"
	"macaddr=00:90:4c:06:c0:19\0"
	"END\0";

static char BCMATTACHDATA(defaultsromvars_4319sdio_usbsd)[] =
	"sromrev=3\0"
	"vendid=0x14e4\0"
	"devid=0x4338\0"
	"boardtype=0x05a2\0"
	"boardrev=0x1100\0"
	"boardflags=0x400201\0"
	"boardflags2=0x80\0"
	"xtalfreq=30000\0"
	"aa2g=3\0"
	"aa5g=0\0"
	"ag0=0\0"
	"opo=0\0"
	"pa0b0=0x1675\0"
	"pa0b1=0xfa74\0"
	"pa0b2=0xfea1\0"
	"pa0itssit=62\0"
	"pa0maxpwr=78\0"
	"rssismf2g=0xa \0"
	"rssismc2g=0xb \0"
	"rssisav2g=0x3 \0"
	"bxa2g=0\0"
	"cckdigfilttype=6\0"
	"rxpo2g=2\0"
	"cckpo=0\0"
	"ofdmpo=0x55553333\0"
	"mcs2gpo0=0x9999\0"
	"mcs2gpo1=0x9999\0"
	"mcs2gpo2=0x0000\0"
	"mcs2gpo3=0x0000\0"
	"mcs2gpo4=0x9999\0"
	"mcs2gpo5=0x9999\0"
	"macaddr=00:90:4c:08:90:00\0"
	"END\0";

static char BCMATTACHDATA(defaultsromvars_43237)[] =
	"vendid=0x14e4\0"
	"devid=0x4355\0"
	"boardtype=0x0583\0"
	"boardrev=0x1103\0"
	"boardnum=0x1\0"
	"boardflags=0x200\0"
	"boardflags2=0\0"
	"sromrev=8\0"
	"macaddr=00:90:4c:51:a8:e4\0"
	"ccode=0\0"
	"regrev=0\0"
	"ledbh0=0xff\0"
	"ledbh1=0xff\0"
	"ledbh2=0xff\0"
	"ledbh3=0xff\0"
	"leddc=0xffff\0"
	"opo=0x0\0"
	"aa2g=0x3\0"
	"aa5g=0x3\0"
	"ag0=0x2\0"
	"ag1=0x2\0"
	"ag2=0xff\0"
	"ag3=0xff\0"
	"pa0b0=0xfed1\0"
	"pa0b1=0x15fd\0"
	"pa0b2=0xfac2\0"
	"pa0itssit=0x20\0"
	"pa0maxpwr=0x4c\0"
	"pa1b0=0xfecd\0"
	"pa1b1=0x1497\0"
	"pa1b2=0xfae3\0"
	"pa1lob0=0xfe87\0"
	"pa1lob1=0x1637\0"
	"pa1lob2=0xfa8e\0"
	"pa1hib0=0xfedc\0"
	"pa1hib1=0x144b\0"
	"pa1hib2=0xfb01\0"
	"pa1itssit=0x3e\0"
	"pa1maxpwr=0x40\0"
	"pa1lomaxpwr=0x3a\0"
	"pa1himaxpwr=0x3c\0"
	"bxa2g=0x3\0"
	"rssisav2g=0x7\0"
	"rssismc2g=0xf\0"
	"rssismf2g=0xf\0"
	"bxa5g=0x3\0"
	"rssisav5g=0x7\0"
	"rssismc5g=0xf\0"
	"rssismf5g=0xf\0"
	"tri2g=0xff\0"
	"tri5g=0xff\0"
	"tri5gl=0xff\0"
	"tri5gh=0xff\0"
	"rxpo2g=0xff\0"
	"rxpo5g=0xff\0"
	"txchain=0x3\0"
	"rxchain=0x3\0"
	"antswitch=0x0\0"
	"tssipos2g=0x1\0"
	"extpagain2g=0x2\0"
	"pdetrange2g=0x2\0"
	"triso2g=0x3\0"
	"antswctl2g=0x0\0"
	"tssipos5g=0x1\0"
	"extpagain5g=0x2\0"
	"pdetrange5g=0x2\0"
	"triso5g=0x3\0"
	"cck2gpo=0x0\0"
	"ofdm2gpo=0x0\0"
	"ofdm5gpo=0x0\0"
	"ofdm5glpo=0x0\0"
	"ofdm5ghpo=0x0\0"
	"mcs2gpo0=0x0\0"
	"mcs2gpo1=0x0\0"
	"mcs2gpo2=0x0\0"
	"mcs2gpo3=0x0\0"
	"mcs2gpo4=0x0\0"
	"mcs2gpo5=0x0\0"
	"mcs2gpo6=0x0\0"
	"mcs2gpo7=0x0\0"
	"mcs5gpo0=0x0\0"
	"mcs5gpo1=0x0\0"
	"mcs5gpo2=0x0\0"
	"mcs5gpo3=0x0\0"
	"mcs5gpo4=0x0\0"
	"mcs5gpo5=0x0\0"
	"mcs5gpo6=0x0\0"
	"mcs5gpo7=0x0\0"
	"mcs5glpo0=0x0\0"
	"mcs5glpo1=0x0\0"
	"mcs5glpo2=0x0\0"
	"mcs5glpo3=0x0\0"
	"mcs5glpo4=0x0\0"
	"mcs5glpo5=0x0\0"
	"mcs5glpo6=0x0\0"
	"mcs5glpo7=0x0\0"
	"mcs5ghpo0=0x0\0"
	"mcs5ghpo1=0x0\0"
	"mcs5ghpo2=0x0\0"
	"mcs5ghpo3=0x0\0"
	"mcs5ghpo4=0x0\0"
	"mcs5ghpo5=0x0\0"
	"mcs5ghpo6=0x0\0"
	"mcs5ghpo7=0x0\0"
	"cddpo=0x0\0"
	"stbcpo=0x0\0"
	"bw40po=0x0\0"
	"bwduppo=0x0\0"
	"maxp2ga0=0x4c\0"
	"pa2gw0a0=0xfed1\0"
	"pa2gw1a0=0x15fd\0"
	"pa2gw2a0=0xfac2\0"
	"maxp5ga0=0x3c\0"
	"maxp5gha0=0x3c\0"
	"maxp5gla0=0x3c\0"
	"pa5gw0a0=0xfeb0\0"
	"pa5gw1a0=0x1491\0"
	"pa5gw2a0=0xfaf8\0"
	"pa5glw0a0=0xfeaa\0"
	"pa5glw1a0=0x14b9\0"
	"pa5glw2a0=0xfaf0\0"
	"pa5ghw0a0=0xfec5\0"
	"pa5ghw1a0=0x1439\0"
	"pa5ghw2a0=0xfb18\0"
	"maxp2ga1=0x4c\0"
	"itt2ga0=0x20\0"
	"itt5ga0=0x3e\0"
	"itt2ga1=0x20\0"
	"itt5ga1=0x3e\0"
	"pa2gw0a1=0xfed2\0"
	"pa2gw1a1=0x15d9\0"
	"pa2gw2a1=0xfac6\0"
	"maxp5ga1=0x3a\0"
	"maxp5gha1=0x3a\0"
	"maxp5gla1=0x3a\0"
	"pa5gw0a1=0xfebe\0"
	"pa5gw1a1=0x1306\0"
	"pa5gw2a1=0xfb63\0"
	"pa5glw0a1=0xfece\0"
	"pa5glw1a1=0x1361\0"
	"pa5glw2a1=0xfb5f\0"
	"pa5ghw0a1=0xfe9e\0"
	"pa5ghw1a1=0x12ca\0"
	"pa5ghw2a1=0xfb41\0"
	"END\0";

static char BCMATTACHDATA(defaultsromvars_43143sdio)[] =
	"vendid=0x14e4\0"
	"subvendid=0x0a5c\0"
	"subdevid=0xbdc\0"
	"macaddr=00:90:4c:0e:81:23\0"
	"xtalfreq=20000\0"
	"cctl=0\0"
	"ccode=US\0"
	"regrev=0x0\0"
	"ledbh0=0xff\0"
	"ledbh1=0xff\0"
	"ledbh2=0xff\0"
	"ledbh3=0xff\0"
	"leddc=0xffff\0"
	"aa2g=0x3\0"
	"ag0=0x2\0"
	"txchain=0x1\0"
	"rxchain=0x1\0"
	"antswitch=0\0"
	"sromrev=10\0"
	"devid=0x4366\0"
	"boardrev=0x1100\0"
	"boardflags=0x200\0"
	"boardflags2=0x2000\0"
	"boardtype=0x0628\0"
	"tssipos2g=0x1\0"
	"extpagain2g=0x0\0"
	"pdetrange2g=0x0\0"
	"triso2g=0x3\0"
	"antswctl2g=0x0\0"
	"ofdm2gpo=0x0\0"
	"mcs2gpo0=0x0\0"
	"mcs2gpo1=0x0\0"
	"maxp2ga0=0x48\0"
	"tempthresh=120\0"
	"temps_period=5\0"
	"temp_hysteresis=5\0"
	"boardnum=0x1100\0"
	"pa0b0=5832\0"
	"pa0b1=-705\0"
	"pa0b2=-170\0"
	"cck2gpo=0\0"
	"swctrlmap_2g=0x06020602,0x0c080c08,0x04000400,0x00080808,0x6ff\0"
	"otpimagesize=154\0"
	"END\0";

static const char BCMATTACHDATA(rstr_load_driver_default_for_chip_X)[] =
	"load driver default for chip %x\n";
static const char BCMATTACHDATA(rstr_unknown_chip_X)[] = "unknown chip %x\n";

static int
BCMATTACHFN(srom_load_nvram)(si_t *sih, osl_t *osh, uint8 *pcis[], uint ciscnt,  char **vars,
	uint *varsz)
{
	uint len = 0, base_len;
	char *base;
	char *fakevars;

	base = fakevars = NULL;
	switch (CHIPID(sih->chip)) {
		case BCM4319_CHIP_ID:
			printf(rstr_load_driver_default_for_chip_X, CHIPID(sih->chip));
			fakevars = defaultsromvars_4319sdio;
			if (si_cis_source(sih) == CIS_OTP) {
				switch (srom_probe_boardtype(sih, pcis, ciscnt)) {
					case BCM94319SDHMB_SSID:
						fakevars = defaultsromvars_4319sdio_hmb;
						break;
					case BCM94319USBSDB_SSID:
						fakevars = defaultsromvars_4319sdio_usbsd;
						break;
					default:
						fakevars = defaultsromvars_4319sdio;
						break;
				}
			}
			break;
		case BCM43237_CHIP_ID:
			printf(rstr_load_driver_default_for_chip_X, CHIPID(sih->chip));
			fakevars = defaultsromvars_43237;
			break;
		case BCM43143_CHIP_ID:
			printf(rstr_load_driver_default_for_chip_X, CHIPID(sih->chip));
			fakevars = defaultsromvars_43143sdio;
			break;
		default:
			printf(rstr_unknown_chip_X, CHIPID(sih->chip));
			return BCME_ERROR;		/* fakevars == NULL for switch default */
	}


	/* NO OTP, if nvram downloaded, use it */
	if ((_varsz != 0) && (_vars != NULL)) {
		len  = _varsz + (strlen(vstr_end));
		base_len = len + 2;  /* plus 2 terminating \0 */
		base = MALLOC(osh, base_len);
		if (base == NULL) {
			BS_ERROR(("initvars_srom_si: MALLOC failed.\n"));
			return BCME_ERROR;
		}
		bzero(base, base_len);

		/* make a copy of the _vars, _vars is at the top of the memory, cannot append
		 * END\0\0 to it. copy the download vars to base, back of the terminating \0,
		 * then append END\0\0
		 */
		bcopy((void *)_vars, base, _varsz);
		/* backoff all the terminating \0s except the one the for the last string */
		len = _varsz;
		while (!base[len - 1])
			len--;
		len++; /* \0  for the last string */
		/* append END\0\0 to the end */
		bcopy((void *)vstr_end, (base + len), strlen(vstr_end));
		len += (strlen(vstr_end) + 2);
		*vars = base;
		*varsz = len;

		BS_ERROR(("%s nvram downloaded %d bytes\n", __FUNCTION__, _varsz));
	} else {
		/* Fall back to fake srom vars if OTP not programmed */
		len = srom_vars_len(fakevars);
		base = MALLOC(osh, (len + 1));
		base_len = len + 1;
		if (base == NULL) {
			BS_ERROR(("initvars_srom_si: MALLOC failed.\n"));
			return BCME_ERROR;
		}
		bzero(base, base_len);
		bcopy(fakevars, base, len);
		*(base + len) = '\0';           /* add final nullbyte terminator */
		*vars = base;
		*varsz = len + 1;
		BS_ERROR(("srom_load_driver)default: faked nvram %d bytes\n", len));
	}
	/* Parse the CIS */
	if ((srom_parsecis(sih, osh, pcis, ciscnt, vars, varsz)) == BCME_OK) {
		nvram_append((void *)sih, *vars, *varsz);
		DONGLE_STORE_VARS_OTP_PTR(*vars);
	}
	MFREE(osh, base, base_len);
	return BCME_OK;
}

#endif /* BCM_BMAC_VARS_APPEND */

/**
 * initvars_srom_si() is defined multiple times in this file. This is the 2nd variant for chips with
 * an active SDIOd interface using DONGLEVARS
 */
static int
BCMATTACHFN(initvars_srom_si)(si_t *sih, osl_t *osh, void *curmap, char **vars, uint *varsz)
{
	int cis_src;
	uint msz = 0;
	uint sz = 0;
	void *oh = NULL;
	int rc = BCME_OK;
	bool	new_cisformat = FALSE;

	uint16 *cisbuf = NULL;

	/* # sdiod fns + common + extra */
	uint8 *cis[SBSDIO_NUM_FUNCTION + 2] = { 0 };

	uint ciss = 0;
	uint8 *defcis;
	uint hdrsz;

	/* Bail out if we've dealt with OTP/SPROM before! */
	if (srvars_inited)
		goto exit;

	/* Initialize default and cis format count */
	switch (CHIPID(sih->chip)) {
	case BCM4325_CHIP_ID: ciss = 3; defcis = defcis4325; hdrsz = 8; break;
	case BCM4315_CHIP_ID: ciss = 3; defcis = defcis4315; hdrsz = 8; break;
	case BCM4329_CHIP_ID: ciss = 4; defcis = defcis4329; hdrsz = 12; break;
	case BCM4319_CHIP_ID: ciss = 3; defcis = defcis4319; hdrsz = 12; break;
	case BCM4336_CHIP_ID: ciss = 1; defcis = defcis4336; hdrsz = 4; break;
	case BCM43362_CHIP_ID: ciss = 1; defcis = defcis4336; hdrsz = 4; break;
	case BCM4330_CHIP_ID: ciss = 1; defcis = defcis4330; hdrsz = 4; break;
	case BCM43237_CHIP_ID: ciss = 1; defcis = defcis43237; hdrsz = 4; break;
	case BCM4324_CHIP_ID: ciss = 1; defcis = defcis4324; hdrsz = 4; break;
	case BCM4314_CHIP_ID: ciss = 1; defcis = defcis4330; hdrsz = 4; break;
	case BCM4334_CHIP_ID: ciss = 1; defcis = defcis4330; hdrsz = 4; break;
	case BCM43340_CHIP_ID: ciss = 1; defcis = defcis4330; hdrsz = 12; break;
	case BCM43341_CHIP_ID: ciss = 1; defcis = defcis4330; hdrsz = 12; break;
	case BCM43143_CHIP_ID: ciss = 1; defcis = defcis43143; hdrsz = 4; break;
	case BCM4335_CHIP_ID: ciss = 1; defcis = defcis4335; hdrsz = 4; break;
	case BCM4345_CHIP_ID: ciss = 1; defcis = defcis4335; hdrsz = 4; break;
	case BCM4349_CHIP_ID: ciss = 1; defcis = defcis4349; hdrsz = 4; break;
	case BCM4355_CHIP_ID: ciss = 1; defcis = defcis4355; hdrsz = 4; break;
	case BCM4359_CHIP_ID: ciss = 1; defcis = defcis4359; hdrsz = 4; break;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM4356_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID: ciss = 1; defcis = defcis4350; hdrsz = 4; break;
	case BCM43909_CHIP_ID: ciss = 1; defcis = defcis43909; hdrsz = 4; break;
	default:
		BS_ERROR(("%s: Unknown chip 0x%04x\n", __FUNCTION__, CHIPID(sih->chip)));
		return BCME_ERROR;
	}
	if (sih->ccrev >= 36) {
		uint32 otplayout;
		if (AOB_ENAB(sih)) {
			otplayout = si_corereg(sih, si_findcoreidx(sih, GCI_CORE_ID, 0),
			 OFFSETOF(gciregs_t, otplayout), 0, 0);
		} else {
			otplayout = si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, otplayout),
			 0, 0);
		}
		if (otplayout & OTP_CISFORMAT_NEW) {
			ciss = 1;
			hdrsz = 2;
			new_cisformat = TRUE;
		}
		else {
			ciss = 3;
			hdrsz = 12;
		}
	}

	cis_src = si_cis_source(sih);
	switch (cis_src) {
	case CIS_SROM:
		sz = srom_size(sih, osh) << 1;
		break;
	case CIS_OTP:
		if (((oh = otp_init(sih)) != NULL) && (otp_status(oh) & OTPS_GUP_HW))
			sz = otp_size(oh);
		break;
	}

	if (sz != 0) {
		if ((cisbuf = (uint16*)MALLOC(osh, sz)) == NULL)
			return BCME_NOMEM;
		msz = sz;

		switch (cis_src) {
		case CIS_SROM:
			rc = srom_read(sih, SI_BUS, curmap, osh, 0, sz, cisbuf, FALSE);
			break;
		case CIS_OTP:
			sz >>= 1;
			rc = otp_read_region(sih, OTP_HW_RGN, cisbuf, &sz);
			sz <<= 1;
			break;
		}

		ASSERT(sz > hdrsz);
		if (rc == BCME_OK) {
			if ((cisbuf[0] == 0xffff) || (cisbuf[0] == 0)) {
				MFREE(osh, cisbuf, msz);
				cisbuf = NULL;
			} else if (new_cisformat) {
				cis[0] = (uint8*)(cisbuf + hdrsz);
			} else {
				cis[0] = (uint8*)cisbuf + hdrsz;
				cis[1] = (uint8*)cisbuf + hdrsz +
				        (cisbuf[1] >> 8) + ((cisbuf[2] & 0x00ff) << 8) -
				        SBSDIO_CIS_BASE_COMMON;
				cis[2] = (uint8*)cisbuf + hdrsz +
				        cisbuf[3] - SBSDIO_CIS_BASE_COMMON;
				cis[3] = (uint8*)cisbuf + hdrsz +
				        cisbuf[4] - SBSDIO_CIS_BASE_COMMON;
				ASSERT((cis[1] >= cis[0]) && (cis[1] < (uint8*)cisbuf + sz));
				ASSERT((cis[2] >= cis[0]) && (cis[2] < (uint8*)cisbuf + sz));
				ASSERT(((cis[3] >= cis[0]) && (cis[3] < (uint8*)cisbuf + sz)) ||
				        (ciss <= 3));
			}
		}
	}

	/* Use default if strapped to, or strapped source empty */
	if (cisbuf == NULL) {
		ciss = 1;
		cis[0] = defcis;
	}

#ifdef BCM_BMAC_VARS_APPEND
	srom_load_nvram(sih, osh, cis, ciss, vars, varsz);
#else
	/* Parse the CIS */
	if (rc == BCME_OK) {
		if ((rc = srom_parsecis(sih, osh, cis, ciss, vars, varsz)) == BCME_OK) {
			nvram_append((void *)sih, *vars, *varsz);
			DONGLE_STORE_VARS_OTP_PTR(*vars);
		}
	}
#endif /* BCM_BMAC_VARS_APPEND */
	/* Clean up */
	if (cisbuf != NULL)
		MFREE(osh, cisbuf, msz);

	srvars_inited = TRUE;
exit:
	/* Tell the caller there is no individual SROM variables */
	*vars = NULL;
	*varsz = 0;

	/* return OK so the driver will load & use defaults if bad srom/otp */
	return BCME_OK;
} /* initvars_srom_si */
#else /* BCM_DONGLEVARS */

/**
 * initvars_srom_si() is defined multiple times in this file. This is the variant for chips with an
 * active SDIOd interface but without BCM_DONGLEVARS
 */
static int
BCMATTACHFN(initvars_srom_si)(si_t *sih, osl_t *osh, void *curmap, char **vars, uint *varsz)
{
	*vars = NULL;
	*varsz = 0;
	return BCME_OK;
}
#endif /* BCM_DONGLEVARS */

#elif defined(BCMPCIEDEV_ENABLED)

/* No initvars_srom_si() defined for PCIe full-dongle except for 4349.  */

static int
BCMATTACHFN(initvars_srom_si)(si_t *sih, osl_t *osh, void *curmap, char **vars, uint *varsz)
{
#ifdef BCM_DONGLEVARS
	void *oh = NULL;
	uint8 *cis;
	uint sz = 0;
	int rc;

	if (si_cis_source(sih) !=  CIS_OTP)
		return BCME_OK;

	if (((oh = otp_init(sih)) != NULL) && (otp_status(oh) & OTPS_GUP_HW))
		sz = otp_size(oh);
	if (sz == 0)
		return BCME_OK;

	if ((cis = MALLOC(osh, sz)) == NULL)
		return BCME_NOMEM;

	if (otp_isunified(oh)) {
		sz >>= 1;
		rc = otp_read_region(sih, OTP_HW_RGN, (uint16 *)cis, &sz);
		sz <<= 1;

		/* account for the Hardware header */
		if (sz == 128)
			return BCME_OK;

		cis += 128;
	} else {
		sz >>= 1;
		rc = otp_read_region(sih, OTP_SW_RGN, (uint16 *)cis, &sz);
		sz <<= 1;
	}

	if (*(uint16 *)cis == SROM11_SIGNATURE) {
		return BCME_OK;
	}

	if ((rc = srom_parsecis(sih, osh, &cis, SROM_CIS_SINGLE, vars, varsz)) == BCME_OK)
		nvram_append((void *)sih, *vars, *varsz);

	return rc;
#else /* BCM_DONGLEVARS */
	*vars = NULL;
	*varsz = 0;
	return BCME_OK;
#endif /* BCM_DONGLEVARS */
}
#else /* !BCMUSBDEV && !BCMSDIODEV  && !BCMPCIEDEV */

#endif	

void
BCMATTACHFN(srom_var_deinit)(si_t *sih)
{
	srvars_inited = FALSE;
}
