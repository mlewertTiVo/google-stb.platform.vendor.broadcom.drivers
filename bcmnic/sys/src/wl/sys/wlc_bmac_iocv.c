/*
 * BMAC iovar table and registration
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_bmac_iocv.c 613060 2016-01-16 01:32:36Z $
 */


#include <wlc_cfg.h>
#include <typedefs.h>

#include <osl.h>
#include <bcmutils.h>
#include <d11_cfg.h>
#include <siutils.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlioctl.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>

#include <wlc_iocv_types.h>
#include <wlc_iocv_reg.h>
#include <wlc_bmac_iocv.h>

#include <pcicfg.h>
#include <pcie_core.h>
#include <bcmsrom.h>

#include <wlc_hw_priv.h>
#include <wl_export.h>

#include <sbchipc.h>
#include <bcmotp.h>
#include <bcmnvram.h>
#include <bcmdevs.h>

#ifdef WLDIAG
#include <wlc_diag.h>
#endif

#ifdef ROUTER_COMA
#include <hndchipc.h>
#include <hndjtagdefs.h>
#endif

/* iovar table */
static const bcm_iovar_t
wlc_bmac_iovt[] = {
#ifdef WLDIAG
	{"diag", IOV_BMAC_DIAG, 0, IOVT_UINT32, 0},
#endif /* WLDIAG */
#ifdef WLLED
	{"gpiotimerval", IOV_BMAC_SBGPIOTIMERVAL, 0, IOVT_UINT32, sizeof(uint32)},
	{"leddc", IOV_BMAC_LEDDC, IOVF_OPEN_ALLOW, IOVT_UINT32, sizeof(uint32)},
#endif /* WLLED */
	{"wpsgpio", IOV_BMAC_WPSGPIO, 0, IOVT_UINT32, 0},
	{"wpsled", IOV_BMAC_WPSLED, 0, IOVT_UINT32, 0},
	{"btclock_tune_war", IOV_BMAC_BTCLOCK_TUNE_WAR, 0, IOVT_UINT32, 0},
	{"ccgpioin", IOV_BMAC_CCGPIOIN, 0, IOVT_UINT32, 0},
	{"bt_regs_read",	IOV_BMAC_BT_REGS_READ, 0, IOVT_BUFFER, 0},
	/* || defined (BCMINTERNAL)) */
	{"aspm", IOV_BMAC_PCIEASPM, 0, IOVT_INT16, 0},
	{"correrrmask", IOV_BMAC_PCIEADVCORRMASK, 0, IOVT_INT16, 0},
	{"pciereg", IOV_BMAC_PCIEREG, 0, IOVT_BUFFER, 0},
	{"pcieserdesreg", IOV_BMAC_PCIESERDESREG, 0, IOVT_BUFFER, 0},
	{"srom", IOV_BMAC_SROM, 0, IOVT_BUFFER, 0},
	{"customvar1", IOV_BMAC_CUSTOMVAR1, 0, IOVT_UINT32, 0},
	{"generic_dload", IOV_BMAC_GENERIC_DLOAD, 0, IOVT_BUFFER, 0},
	{"noise_metric", IOV_BMAC_NOISE_METRIC, 0, IOVT_UINT16, 0},
	{"avoidance_cnt", IOV_BMAC_AVIODCNT, 0, IOVT_UINT32, 0},
	{"btswitch", IOV_BMAC_BTSWITCH,	0, IOVT_INT32, 0},
	{"vcofreq_pcie2", IOV_BMAC_4360_PCIE2_WAR, IOVF_SET_DOWN, IOVT_INT32, 0},
	{"edcrs", IOV_BMAC_EDCRS, IOVF_SET_UP | IOVF_GET_UP, IOVT_UINT8, 0},
	{NULL, 0, 0, 0, 0}
};

/*
This routine, dumps the contents of the BT registers and memory.
To access BT register, we use interconnect registers.
These registers  are at offset 0xd0, 0xd4, 0xe0 and 0xd8.
Backplane addresses low and high are at offset 0xd0 and 0xd4 and
contain the lower and higher 32 bits of a 64-bit address used for
indirect accesses respectively. Backplane indirect access
register is at offset 0xe0. Bits 3:0 of this register contain
the byte enables supplied to the system backplane on indirect
backplane accesses. So they should be set to 1. Bit 9 (StartBusy bit)
is set to 1 to start an indirect backplane access.
The hardware clears this bit to 0 when the transfer completes.
*/
#define  STARTBUSY_BIT_POLL_MAX_TIME 50
#define  INCREMENT_ADDRESS 4

/* uCode download chunk varies depending on whether it is for
* it for lcn & sslpn or for other chips
*/
#if LCNCONF
#define DL_MAX_CHUNK_LEN 1456  /* 8 * 7 * 26 */
#else
#define DL_MAX_CHUNK_LEN 1408 /* 8 * 8 * 22 */
#endif

static uint wlc_bmac_dma_avoidance_cnt(wlc_hw_info_t *wlc_hw);
static int wlc_bmac_bt_regs_read(wlc_hw_info_t *wlc_hw, uint32 stAdd, uint32 dump_size, uint32 *b);


static int wlc_bmac_doiovar(void *, uint32, void *, uint, void *, uint, uint);



/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>



/** DMA segment list related */
static uint
wlc_bmac_dma_avoidance_cnt(wlc_hw_info_t *wlc_hw)
{
	uint i, total = 0;

	/* get total DMA avoidance counts */
	for (i = 0; i < NFIFO; i++)
		if (wlc_hw->di[i])
			total += dma_avoidance_cnt(wlc_hw->di[i]);

	return (total);
}

static int
wlc_bmac_bt_regs_read(wlc_hw_info_t *wlc_hw, uint32 stAdd, uint32 dump_size, uint32 *b)
{
	uint32 regval1;
	uint32 cur_val = stAdd;
	uint32 endAddress = stAdd + dump_size;
	int counter = 0;
	int delay_val;
	int err;
	while (cur_val < endAddress) {
		si_ccreg(wlc_hw->sih, 0xd0, ~0, cur_val);
		si_ccreg(wlc_hw->sih, 0xd4, ~0, 0);
		si_ccreg(wlc_hw->sih, 0xe0, ~0, 0x20f);
		/*
		The StartBusy bit is set to 1 to start an indirect backplane access.
		The hardware clears this field to 0 when the transfer completes.
		*/
		for (delay_val = 0; delay_val < STARTBUSY_BIT_POLL_MAX_TIME; delay_val++) {
			if (si_ccreg(wlc_hw->sih, 0xe0, 0, 0) == 0x0000000F)
				break;
			OSL_DELAY(100);
		}
		if (delay_val == (STARTBUSY_BIT_POLL_MAX_TIME)) {
			err = BCME_ERROR;
			return err;
		}
		regval1 = si_ccreg(wlc_hw->sih, 0xd8, 0, 0);
		b[counter] = regval1;
		counter++;
		cur_val += INCREMENT_ADDRESS;
	}
	return 0;
}


static int
wlc_bmac_doiovar(void *hw, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size)
{
	wlc_hw_info_t *wlc_hw = (wlc_hw_info_t *)hw;
	int err = 0;
	int32 int_val = 0;
	int32 int_val2 = 0;
	int32 *ret_int_ptr;
	bool bool_val;
	bool bool_val2;

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	if (p_len >= (int)sizeof(int_val) * 2)
		bcopy(((uint8*)params + sizeof(int_val)), &int_val2, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	bool_val = (int_val != 0) ? TRUE : FALSE;
	bool_val2 = (int_val2 != 0) ? TRUE : FALSE;
	BCM_REFERENCE(bool_val2);

	WL_TRACE(("%s(): actionid=%d, p_len=%d, len=%d\n", __FUNCTION__, actionid, p_len, len));

	switch (actionid) {
#ifdef WLDIAG
	case IOV_GVAL(IOV_BMAC_DIAG): {
		uint32 result;
		uint32 diagtype;

		/* recover diagtype to run */
		bcopy((char *)params, (char *)(&diagtype), sizeof(diagtype));
		err = wlc_diag(wlc_hw->wlc, diagtype, &result);
		bcopy((char *)(&result), arg, sizeof(diagtype)); /* copy result to be buffer */
		break;
	}
#endif /* WLDIAG */

#ifdef WLLED
	case IOV_GVAL(IOV_BMAC_SBGPIOTIMERVAL):
	case IOV_GVAL(IOV_BMAC_LEDDC):
		*ret_int_ptr = si_gpiotimerval(wlc_hw->sih, 0, 0);
		break;
	case IOV_SVAL(IOV_BMAC_SBGPIOTIMERVAL):
	case IOV_SVAL(IOV_BMAC_LEDDC):
		si_gpiotimerval(wlc_hw->sih, ~0, int_val);
		break;
#endif /* WLLED */


	case IOV_GVAL(IOV_BMAC_CCGPIOIN):
		*ret_int_ptr = si_gpioin(wlc_hw->sih);
		break;

	case IOV_GVAL(IOV_BMAC_WPSGPIO): {
		char *var;

		if ((var = getvar(wlc_hw->vars, "wpsgpio")))
			*ret_int_ptr = (uint32)bcm_strtoul(var, NULL, 0);
		else {
			*ret_int_ptr = -1;
			err = BCME_NOTFOUND;
		}

		break;
	}

	case IOV_GVAL(IOV_BMAC_WPSLED): {
		char *var;

		if ((var = getvar(wlc_hw->vars, "wpsled")))
			*ret_int_ptr = (uint32)bcm_strtoul(var, NULL, 0);
		else {
			*ret_int_ptr = -1;
			err = BCME_NOTFOUND;
		}

		break;
	}

	case IOV_GVAL(IOV_BMAC_BTCLOCK_TUNE_WAR):
		*ret_int_ptr = wlc_hw->btclock_tune_war;
		break;

	case IOV_SVAL(IOV_BMAC_BTCLOCK_TUNE_WAR):
		wlc_hw->btclock_tune_war = bool_val;
		break;

	case IOV_GVAL(IOV_BMAC_BT_REGS_READ): {
		/* the size of output dump can not be larger than the buffer size */
		if ((uint)int_val2 > len)
			err = BCME_BUFTOOSHORT;
		else
			err = wlc_bmac_bt_regs_read(wlc_hw, int_val, int_val2, (uint32*)arg);
		break;
	}


	case IOV_GVAL(IOV_BMAC_PCIEADVCORRMASK):
			if ((BUSTYPE(wlc_hw->sih->bustype) != PCI_BUS) ||
			    (BUSCORETYPE(wlc_hw->sih->buscoretype) != PCIE_CORE_ID)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		*ret_int_ptr = si_pciereg(wlc_hw->sih, PCIE_ADV_CORR_ERR_MASK,
			0, 0, PCIE_CONFIGREGS);
		break;


	case IOV_SVAL(IOV_BMAC_PCIEADVCORRMASK):
	        if ((BUSTYPE(wlc_hw->sih->bustype) != PCI_BUS) ||
	            (BUSCORETYPE(wlc_hw->sih->buscoretype) != PCIE_CORE_ID)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		/* Set all errors if -1 or else mask off undefined bits */
		if (int_val == -1)
			int_val = ALL_CORR_ERRORS;

		int_val &= ALL_CORR_ERRORS;
		si_pciereg(wlc_hw->sih, PCIE_ADV_CORR_ERR_MASK, 1, int_val,
			PCIE_CONFIGREGS);
		break;

	case IOV_GVAL(IOV_BMAC_PCIEASPM): {
		uint8 clkreq = 0;
		uint32 aspm = 0;

		if (BUSTYPE(wlc_hw->sih->bustype) != PCI_BUS) {
			err = BCME_UNSUPPORTED;
			break;
		}

		/* this command is to hide the details, but match the lcreg
		   #define PCIE_CLKREQ_ENAB		0x100
		   #define PCIE_ASPM_L1_ENAB	2
		   #define PCIE_ASPM_L0s_ENAB	1
		*/

		clkreq = si_pcieclkreq(wlc_hw->sih, 0, 0);
		aspm = si_pcielcreg(wlc_hw->sih, 0, 0);
		*ret_int_ptr = ((clkreq & 0x1) << 8) | (aspm & PCIE_ASPM_ENAB);
		break;
	}

	case IOV_SVAL(IOV_BMAC_PCIEASPM): {
		uint32 tmp;

		if (BUSTYPE(wlc_hw->sih->bustype) != PCI_BUS) {
			err = BCME_UNSUPPORTED;
			break;
		}

		tmp = si_pcielcreg(wlc_hw->sih, 0, 0);
		si_pcielcreg(wlc_hw->sih, PCIE_ASPM_ENAB,
			(tmp & ~PCIE_ASPM_ENAB) | (int_val & PCIE_ASPM_ENAB));

		si_pcieclkreq(wlc_hw->sih, 1, ((int_val & 0x100) >> 8));
		break;
	}

	case IOV_SVAL(IOV_BMAC_PCIEREG):
		if (p_len < (int)sizeof(int_val) * 2) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		if (int_val < 0) {
			err = BCME_BADARG;
			break;
		}
		si_pciereg(wlc_hw->sih, int_val, 1, int_val2, PCIE_PCIEREGS);
		break;

	case IOV_GVAL(IOV_BMAC_PCIEREG):
		if (p_len < (int)sizeof(int_val)) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		if (int_val < 0) {
			err = BCME_BADARG;
			break;
		}
		*ret_int_ptr = si_pciereg(wlc_hw->sih, int_val, 0, 0, PCIE_PCIEREGS);
		break;

	case IOV_SVAL(IOV_BMAC_EDCRS):
		if (!(WLCISNPHY(wlc_hw->band)) &&
			!WLCISHTPHY(wlc_hw->band) && !WLCISACPHY(wlc_hw->band)) {
			err = BCME_UNSUPPORTED;
			break;
		}
		if (bool_val) {
			wlc_bmac_ifsctl_edcrs_set(wlc_hw, WLCISHTPHY(wlc_hw->band));
		} else {
			if (WLCISACPHY(wlc_hw->band))
				wlc_bmac_ifsctl_vht_set(wlc_hw, OFF);
			else
				wlc_bmac_ifsctl1_regshm(wlc_hw, (IFS_CTL1_EDCRS |
					IFS_CTL1_EDCRS_20L | IFS_CTL1_EDCRS_40), 0);
		}
		break;

	case IOV_GVAL(IOV_BMAC_EDCRS):
		if (!(WLCISNPHY(wlc_hw->band)) &&
			!WLCISHTPHY(wlc_hw->band) && !WLCISACPHY(wlc_hw->band)) {
			err = BCME_UNSUPPORTED;
			break;
		}
		{
			uint16 val;
			if (D11REV_LT(wlc_hw->corerev, 40))
				val = wlc_bmac_read_shm(wlc_hw, M_IFSCTL1(wlc_hw));
			else
				val = wlc_bmac_read_shm(wlc_hw, M_IFS_PRICRS(wlc_hw));

			if (WLCISACPHY(wlc_hw->band))
				*ret_int_ptr = (val & IFS_CTL_ED_SEL_MASK) ? TRUE:FALSE;
			else if (WLCISHTPHY(wlc_hw->band))
				*ret_int_ptr = (val & IFS_EDCRS_MASK) ? TRUE:FALSE;
			else
				*ret_int_ptr = (val & IFS_CTL1_EDCRS) ? TRUE:FALSE;
		}
		break;

	case IOV_SVAL(IOV_BMAC_PCIESERDESREG): {
		int32 int_val3;
		if (p_len < (int)sizeof(int_val) * 3) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		if (int_val < 0 || int_val2 < 0) {
			err = BCME_BADARG;
			break;
		}
		if (BUSTYPE(wlc_hw->sih->bustype) != PCI_BUS) {
			err = BCME_UNSUPPORTED;
			break;
		}

		bcopy(((uint8*)params + 2 * sizeof(int_val)), &int_val3, sizeof(int_val));
		/* write dev/offset/val to serdes */
		si_pcieserdesreg(wlc_hw->sih, int_val, int_val2, 1, int_val3);
		break;
	}

	case IOV_GVAL(IOV_BMAC_PCIESERDESREG): {
		if (p_len < (int)sizeof(int_val) * 2) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		if (int_val < 0 || int_val2 < 0) {
			err = BCME_BADARG;
			break;
		}

		*ret_int_ptr = si_pcieserdesreg(wlc_hw->sih, int_val, int_val2, 0, 0);
		break;
	}



	case IOV_GVAL(IOV_BMAC_SROM): {
		srom_rw_t *s = (srom_rw_t *)arg;
		uint32 macintmask;

		/* intrs off */
		macintmask = wl_intrsoff(wlc_hw->wlc->wl);

		if (si_is_sprom_available(wlc_hw->sih)) {
			if (srom_read(wlc_hw->sih, wlc_hw->sih->bustype,
			              (void *)(uintptr)wlc_hw->regs, wlc_hw->osh,
			              s->byteoff, s->nbytes, s->buf, FALSE))
				err = BCME_ERROR;
#if defined(BCMNVRAMR)
		} else if (!si_is_otp_disabled(wlc_hw->sih)) {
			err = BCME_UNSUPPORTED;
#endif 
		} else
			err = BCME_NOTFOUND;


		/* restore intrs */
		wl_intrsrestore(wlc_hw->wlc->wl, macintmask);
		break;
	}




	case IOV_GVAL(IOV_BMAC_CUSTOMVAR1): {
		char *var;

		if ((var = getvar(wlc_hw->vars, "customvar1")))
			*ret_int_ptr = (uint32)bcm_strtoul(var, NULL, 0);
		else
			*ret_int_ptr = 0;

		break;
	}
	case IOV_SVAL(IOV_BMAC_GENERIC_DLOAD): {
		wl_dload_data_t *dload_ptr, dload_data;
		uint8 *bufptr;
		uint32 total_len;
		uint actual_data_offset;
		actual_data_offset = OFFSETOF(wl_dload_data_t, data);
		memcpy(&dload_data, (wl_dload_data_t *)arg, sizeof(wl_dload_data_t));
		total_len = dload_data.len + actual_data_offset;
		if ((bufptr = MALLOC(wlc_hw->osh, total_len)) == NULL) {
			err = BCME_NOMEM;
			break;
		}
		memcpy(bufptr, (uint8 *)arg, total_len);
		dload_ptr = (wl_dload_data_t *)bufptr;
		if (((dload_ptr->flag & DLOAD_FLAG_VER_MASK) >> DLOAD_FLAG_VER_SHIFT)
		    != DLOAD_HANDLER_VER) {
			err =  BCME_ERROR;
			MFREE(wlc_hw->osh, bufptr, total_len);
			break;
		}
		switch (dload_ptr->dload_type)	{
#ifdef BCMUCDOWNLOAD
		case DL_TYPE_UCODE:
			if (wlc_hw->wlc->is_initvalsdloaded != TRUE)
				wlc_process_ucodeparts(wlc_hw->wlc, dload_ptr->data);
			break;
#endif /* BCMUCDOWNLOAD */
		default:
			err = BCME_UNSUPPORTED;
			break;
		}
		MFREE(wlc_hw->osh, bufptr, total_len);
		break;
	}
	case IOV_GVAL(IOV_BMAC_UCDLOAD_STATUS):
		*ret_int_ptr = (int32) wlc_hw->wlc->is_initvalsdloaded;
		break;
	case IOV_GVAL(IOV_BMAC_UC_CHUNK_LEN):
		*ret_int_ptr = DL_MAX_CHUNK_LEN;
		break;

	case IOV_GVAL(IOV_BMAC_NOISE_METRIC):
		*ret_int_ptr = (int32)wlc_hw->noise_metric;
		break;
	case IOV_SVAL(IOV_BMAC_NOISE_METRIC):

		if ((uint16)int_val > NOISE_MEASURE_KNOISE) {
			err = BCME_UNSUPPORTED;
			break;
		}

		wlc_hw->noise_metric = (uint16)int_val;

		if ((wlc_hw->noise_metric & NOISE_MEASURE_KNOISE) == NOISE_MEASURE_KNOISE)
			wlc_bmac_mhf(wlc_hw, MHF3, MHF3_KNOISE, MHF3_KNOISE, WLC_BAND_ALL);
		else
			wlc_bmac_mhf(wlc_hw, MHF3, MHF3_KNOISE, 0, WLC_BAND_ALL);

		break;

	case IOV_GVAL(IOV_BMAC_AVIODCNT):
		*ret_int_ptr = wlc_bmac_dma_avoidance_cnt(wlc_hw);
		break;



	case IOV_SVAL(IOV_BMAC_BTSWITCH):
		if ((int_val != OFF) && (int_val != ON) && (int_val != AUTO)) {
			return BCME_RANGE;
		}

		if (WLCISACPHY(wlc_hw->band) && !(wlc_hw->up)) {
			err = BCME_NOTUP;
		} else {
			err = wlc_bmac_set_btswitch(wlc_hw, (int8)int_val);
		}

		break;

	case IOV_GVAL(IOV_BMAC_BTSWITCH):
		if (!((((CHIPID(wlc_hw->sih->chip) == BCM4331_CHIP_ID) ||
		       (CHIPID(wlc_hw->sih->chip) == BCM43431_CHIP_ID)) &&
		      ((wlc_hw->sih->boardtype == BCM94331X28) ||
		       (wlc_hw->sih->boardtype == BCM94331X28B) ||
		       (wlc_hw->sih->boardtype == BCM94331CS_SSID) ||
		       (wlc_hw->sih->boardtype == BCM94331X29B) ||
		       (wlc_hw->sih->boardtype == BCM94331X29D))) ||
		      WLCISACPHY(wlc_hw->band))) {
			err = BCME_UNSUPPORTED;
			break;
		}

		if (WLCISACPHY(wlc_hw->band)) {
			if (!(wlc_hw->up)) {
				err = BCME_NOTUP;
				break;
			}

			/* read the phyreg to find the state of bt/wlan ovrd */
			wlc_hw->btswitch_ovrd_state =
			        wlc_phy_get_femctrl_bt_wlan_ovrd(wlc_hw->band->pi);
		}

		*ret_int_ptr = wlc_hw->btswitch_ovrd_state;
		break;


	case IOV_SVAL(IOV_BMAC_4360_PCIE2_WAR):
		wlc_hw->vcoFreq_4360_pcie2_war = (uint)int_val;
		break;

	case IOV_GVAL(IOV_BMAC_4360_PCIE2_WAR):
		*ret_int_ptr = (int)wlc_hw->vcoFreq_4360_pcie2_war;
		break;


#if defined(SAVERESTORE) && defined(BCMDBG_SR)
	case IOV_GVAL(IOV_BMAC_SR_VERIFY): {
		struct bcmstrbuf b;
		bcm_binit(&b, (char *)arg, len);

		if (SR_ENAB())
			wlc_bmac_sr_verify(wlc_hw, &b);
		break;
	}
#endif

	default:
		WL_ERROR(("%s(): undefined BMAC IOVAR: %d\n", __FUNCTION__, actionid));
		err = BCME_NOTFOUND;
		break;

	}

	return err;
}

/* register iovar table/handlers to the system */
int
wlc_bmac_register_iovt(wlc_hw_info_t *hw, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = (wlc_iov_disp_fn_t)MODULE_IOVAR_PATCH_FUNC;
	bcm_iovar_t* patch_table = MODULE_IOVAR_PATCH_TABLE;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(wlc_bmac_iovt,
		wlc_bmac_pack_iov, NULL,
		wlc_bmac_doiovar, disp_fn, patch_table, hw,
		&iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
