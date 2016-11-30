/*
 * Misc utility routines for accessing PMU corerev specific features
 * of the SiliconBackplane-based Broadcom chips.
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
 * $Id: hndpmu.c 659457 2016-09-14 10:57:42Z $
 */


/**
 * @file
 * Note: this file contains PLL/FLL related functions. A chip can contain multiple PLLs/FLLs.
 * However, in the context of this file the baseband ('BB') PLL/FLL is referred to.
 *
 * Throughout this code, the prefixes 'pmu1_' and 'pmu2_' are used.
 * They refer to different revisions of the PMU (which is at revision 18 @ Apr 25, 2012)
 * pmu1_ marks the transition from PLL to ADFLL (Digital Frequency Locked Loop). It supports
 * fractional frequency generation. pmu2_ does not support fractional frequency generation.
 */

#include <bcm_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmdevs.h>
#include <hndsoc.h>
#include <sbchipc.h>
#include <hndpmu.h>
#if !defined(BCMDONGLEHOST)
#include <bcmotp.h>
#endif
#if defined(SAVERESTORE) && !defined(BCMDONGLEHOST)
#include <saverestore.h>
#endif
#include <hndlhl.h>
#if defined(BCMULP)
#include <ulp.h>
#endif /* defined(BCMULP) */
#include <sbgci.h>
#ifdef EVENT_LOG_COMPILE
#include <event_log.h>
#endif
#include <sbgci.h>
#include <lpflags.h>

#define	PMU_ERROR(args)

#define	PMU_MSG(args)

/* To check in verbose debugging messages not intended
 * to be on except on private builds.
 */
#define	PMU_NONE(args)
#define flags_shift	14

/** contains resource bit positions for a specific chip */
struct rsc_per_chip_s {
	uint8 ht_avail;
	uint8 macphy_clkavail;
	uint8 ht_start;
	uint8 otp_pu;
};

typedef struct rsc_per_chip_s rsc_per_chip_t;

#if !defined(BCMDONGLEHOST)
/* PLL controls/clocks */
static void si_pmu1_pllinit0(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal);
static void si_pmu1_pllinit1(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal);
static void si_pmu2_pllinit0(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal);
static void si_pmu_pll_off(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 *min_mask,
	uint32 *max_mask, uint32 *clk_ctl_st);
static void si_pmu_pll_off_isdone(si_t *sih, osl_t *osh, pmuregs_t *pmu);
static void si_pmu_pll_on(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 min_mask,
	uint32 max_mask, uint32 clk_ctl_st);
void si_pmu_pll_4364_macfreq_switch(si_t *sih, osl_t *osh, uint8 mode);
void si_pmu_otp_pllcontrol(si_t *sih, osl_t *osh);
void si_pmu_otp_vreg_control(si_t *sih, osl_t *osh);
void si_pmu_otp_chipcontrol(si_t *sih, osl_t *osh);
uint32 si_pmu_def_alp_clock(si_t *sih, osl_t *osh);
bool si_pmu_update_pllcontrol(si_t *sih, osl_t *osh, uint32 xtal, bool update_required);
static uint32 si_pmu_htclk_mask(si_t *sih);

static uint32 si_pmu1_cpuclk0(si_t *sih, osl_t *osh, pmuregs_t *pmu);
static uint32 si_pmu1_alpclk0(si_t *sih, osl_t *osh, pmuregs_t *pmu);
static uint32 si_pmu2_alpclk0(si_t *sih, osl_t *osh, pmuregs_t *pmu);
static uint32 si_pmu2_cpuclk0(si_t *sih, osl_t *osh, pmuregs_t *pmu);

static uint32 si_pmu1_cpuclk0_pll2(si_t *sih, osl_t *osh, pmuregs_t *pmu);

/* PMU resources */
static bool si_pmu_res_depfltr_bb(si_t *sih);
static bool si_pmu_res_depfltr_ncb(si_t *sih);
static bool si_pmu_res_depfltr_paldo(si_t *sih);
static bool si_pmu_res_depfltr_npaldo(si_t *sih);
static uint32 si_pmu_res_deps(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 rsrcs, bool all);
static uint si_pmu_res_uptime(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint8 rsrc);
static void si_pmu_res_masks(si_t *sih, uint32 *pmin, uint32 *pmax);
static void si_pmu_spuravoid_pllupdate(si_t *sih, pmuregs_t *pmu, osl_t *osh, uint8 spuravoid);

void si_pmu_set_4330_plldivs(si_t *sih, uint8 dacrate);
static int8 si_pmu_cbuckout_to_vreg_ctrl(si_t *sih, uint16 cbuck_mv);
#ifdef LDO3P3_MIN_RES_MASK
static int si_pmu_min_res_set(si_t *sih, osl_t *osh, uint min_mask, bool set);
#endif
uint32 si_pmu_get_pmutime_diff(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 *prev);
bool si_pmu_wait_for_res_pending(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint usec,
	bool cond, uint32 *elapsed_time);



#ifdef BCMULP

typedef struct si_pmu_ulp_cr_dat {
	uint32 ilpcycles_per_sec;
} si_pmu_ulp_cr_dat_t;

static uint si_pmu_ulp_get_retention_size_cb(void *handle, ulp_ext_info_t *einfo);
static int si_pmu_ulp_enter_cb(void *handle, ulp_ext_info_t *einfo, uint8 *cache_data);
static int si_pmu_ulp_exit_cb(void *handle, uint8 *cache_data, uint8 *p2_cache_data);
static const ulp_p1_module_pubctx_t ulp_pmu_ctx = {
	MODCBFL_CTYPE_STATIC,
	si_pmu_ulp_enter_cb,
	si_pmu_ulp_exit_cb,
	si_pmu_ulp_get_retention_size_cb,
	NULL,
	NULL
};

#endif /* BCMULP */

static int si_pmu_openloop_cal_43012(si_t *sih, uint16 currtemp);

/* PMU timer ticks once in 32uS */
#define PMU_US_STEPS (32)


void *g_si_pmutmr_lock_arg = NULL;
si_pmu_callback_t g_si_pmutmr_lock_cb = NULL, g_si_pmutmr_unlock_cb = NULL;

/* FVCO frequency in [KHz] */
#define FVCO_640	640000  /**< 640MHz */
#define FVCO_880	880000	/**< 880MHz */
#define FVCO_1760	1760000	/**< 1760MHz */
#define FVCO_1440	1440000	/**< 1440MHz */
#define FVCO_960	960000	/**< 960MHz */
#define FVCO_960010	960010	/**< 960.0098MHz */
#define FVCO_961	961000	/**< 961MHz */
#define FVCO_963	963000	/**< 963MHz */
#define FVCO_1600	1600000	/**< 1600MHz */
#define FVCO_1920	1920000	/**< 1920MHz */
#define FVCO_1938	1938000 /* 1938MHz */
#define FVCO_963	963000	/**< 963MHz */
#define FVCO_385	385000  /**< 420MHz */
#define FVCO_2880	2880000	/**< 2880 MHz */
#define FVCO_2946	2946000	/**< 2946 MHz */

/* defines to make the code more readable */
#define NO_SUCH_RESOURCE	0	/**< means: chip does not have such a PMU resource */

/* uses these defines instead of 'magic' values when writing to register pllcontrol_addr */
#define PMU_PLL_CTRL_REG0	0
#define PMU_PLL_CTRL_REG1	1
#define PMU_PLL_CTRL_REG2	2
#define PMU_PLL_CTRL_REG3	3
#define PMU_PLL_CTRL_REG4	4
#define PMU_PLL_CTRL_REG5	5
#define PMU_PLL_CTRL_REG6	6
#define PMU_PLL_CTRL_REG7	7
#define PMU_PLL_CTRL_REG8	8
#define PMU_PLL_CTRL_REG9	9
#define PMU_PLL_CTRL_REG10	10
#define PMU_PLL_CTRL_REG11	11
#define PMU_PLL_CTRL_REG12	12
#define PMU_PLL_CTRL_REG13	13
#define PMU_PLL_CTRL_REG14	14
#define PMU_PLL_CTRL_REG15	15

/**
 * Reads/writes a chipcontrol reg. Performes core switching if required, at function exit the
 * original core is restored. Depending on chip type, read/writes to chipcontrol regs in CC core
 * (older chips) or to chipcontrol regs in PMU core (later chips).
 */
uint32
si_pmu_chipcontrol(si_t *sih, uint reg, uint32 mask, uint32 val)
{
	pmu_corereg(sih, SI_CC_IDX, chipcontrol_addr, ~0, reg);
	return pmu_corereg(sih, SI_CC_IDX, chipcontrol_data, mask, val);
}

/**
 * Reads/writes a voltage regulator (vreg) register. Performes core switching if required, at
 * function exit the original core is restored. Depending on chip type, writes to regulator regs
 * in CC core (older chips) or to regulator regs in PMU core (later chips).
 */
uint32
si_pmu_vreg_control(si_t *sih, uint reg, uint32 mask, uint32 val)
{
	pmu_corereg(sih, SI_CC_IDX, regcontrol_addr, ~0, reg);
	return pmu_corereg(sih, SI_CC_IDX, regcontrol_data, mask, val);
}

/**
 * Reads/writes a PLL control register. Performes core switching if required, at function exit the
 * original core is restored. Depending on chip type, writes to PLL control regs in CC core (older
 * chips) or to PLL control regs in PMU core (later chips).
 */
uint32
si_pmu_pllcontrol(si_t *sih, uint reg, uint32 mask, uint32 val)
{
	pmu_corereg(sih, SI_CC_IDX, pllcontrol_addr, ~0, reg);
	return pmu_corereg(sih, SI_CC_IDX, pllcontrol_data, mask, val);
}

/**
 * The chip has one or more PLLs/FLLs (e.g. baseband PLL, USB PHY PLL). The settings of each PLL are
 * contained within one or more 'PLL control' registers. Since the PLL hardware requires that
 * changes for one PLL are committed at once, the PMU has a provision for 'updating' all PLL control
 * registers at once.
 *
 * When software wants to change the any PLL parameters, it withdraws requests for that PLL clock,
 * updates the PLL control registers being careful not to alter any control signals for the other
 * PLLs, and then writes a 1 to PMUCtl.PllCtnlUpdate to commit the changes. Best usage model would
 * be bring PLL down then update the PLL control register.
 */
void
si_pmu_pllupd(si_t *sih)
{
	pmu_corereg(sih, SI_CC_IDX, pmucontrol,
	           PCTL_PLL_PLLCTL_UPD, PCTL_PLL_PLLCTL_UPD);
}

/**
* For each chip, location of resource bits (e.g., ht bit) in resource mask registers may differ.
* This function abstracts the bit position of commonly used resources, thus making the rest of the
* code in hndpmu.c cleaner.
*/
static rsc_per_chip_t* si_pmu_get_rsc_positions(si_t *sih)
{
	static rsc_per_chip_t rsc_4313 =  {RES4313_HT_AVAIL_RSRC, RES4313_MACPHY_CLK_AVAIL_RSRC,
		NO_SUCH_RESOURCE,  NO_SUCH_RESOURCE};
	static rsc_per_chip_t rsc_4314 =  {RES4314_HT_AVAIL,  RES4314_MACPHY_CLK_AVAIL,
		NO_SUCH_RESOURCE,  RES4314_OTP_PU};
	static rsc_per_chip_t rsc_4324 =  {RES4324_HT_AVAIL,  RES4324_MACPHY_CLKAVAIL,
		NO_SUCH_RESOURCE,  RES4324_OTP_PU};
	static rsc_per_chip_t rsc_4330 =  {RES4330_HT_AVAIL,  RES4330_MACPHY_CLKAVAIL,
		NO_SUCH_RESOURCE,  RES4330_OTP_PU};
	static rsc_per_chip_t rsc_4334 =  {RES4334_HT_AVAIL,  RES4334_MACPHY_CLK_AVAIL,
		NO_SUCH_RESOURCE,  RES4334_OTP_PU};
	static rsc_per_chip_t rsc_4335 =  {RES4335_HT_AVAIL,  RES4335_MACPHY_CLKAVAIL,
		RES4335_HT_START,  RES4335_OTP_PU};
	static rsc_per_chip_t rsc_4336 =  {RES4336_HT_AVAIL,  RES4336_MACPHY_CLKAVAIL,
		NO_SUCH_RESOURCE,  RES4336_OTP_PU};
	static rsc_per_chip_t rsc_4350 =  {RES4350_HT_AVAIL,  RES4350_MACPHY_CLKAVAIL,
		RES4350_HT_START,  RES4350_OTP_PU};
	static rsc_per_chip_t rsc_4352 =  {NO_SUCH_RESOURCE,  NO_SUCH_RESOURCE,
		NO_SUCH_RESOURCE,  RES4360_OTP_PU}; /* 4360_OTP_PU is used for 4352, not a typo */
	static rsc_per_chip_t rsc_4360 =  {RES4360_HT_AVAIL,  NO_SUCH_RESOURCE,
		NO_SUCH_RESOURCE,  RES4360_OTP_PU};
	static rsc_per_chip_t rsc_43143 = {RES43143_HT_AVAIL, RES43143_MACPHY_CLK_AVAIL,
		NO_SUCH_RESOURCE,  RES43143_OTP_PU};
	static rsc_per_chip_t rsc_43236 =  {RES43236_HT_SI_AVAIL, NO_SUCH_RESOURCE,
		NO_SUCH_RESOURCE,  NO_SUCH_RESOURCE};
	static rsc_per_chip_t rsc_43239 = {RES43239_HT_AVAIL, RES43239_MACPHY_CLKAVAIL,
		NO_SUCH_RESOURCE,  RES43239_OTP_PU};
	static rsc_per_chip_t rsc_43602 = {RES43602_HT_AVAIL, RES43602_MACPHY_CLKAVAIL,
		RES43602_HT_START, NO_SUCH_RESOURCE};
	static rsc_per_chip_t rsc_4365 = {RES4365_HT_AVAIL, RES4365_MACPHY_CLK_AVAIL,
		NO_SUCH_RESOURCE, NO_SUCH_RESOURCE};
	static rsc_per_chip_t rsc_7271 = {RES7271_HT_AVAIL, RES7271_MACPHY_CLK_AVAIL,
		NO_SUCH_RESOURCE, NO_SUCH_RESOURCE};
	static rsc_per_chip_t rsc_43430 = {RES43430_HT_AVAIL, RES43430_MACPHY_CLK_AVAIL,
		RES43430_HT_START, RES43430_OTP_PU};
	static rsc_per_chip_t rsc_43012 = {RES43012_HT_AVAIL, RES43012_MACPHY_CLK_AVAIL,
		RES43012_HT_START, RES43012_OTP_PU};
	static rsc_per_chip_t rsc_4347 = {RES4347_HT_AVAIL,
		RES4347_MACPHY_CLK_AVAIL_MAIN | RES4347_MACPHY_CLK_AVAIL_AUX,
		RES4347_HT_AVAIL, RES4347_AAON}; /* should check if there is RES4347_HT_START */
	static rsc_per_chip_t rsc_4364 = {RES4364_HT_AVAIL, RES4364_MACPHY_CLKAVAIL,
		RES4364_HT_START, RES4364_OTP_PU};
	/* TBD verify below */
	static rsc_per_chip_t rsc_4369 = {RES4369_HT_AVAIL,
		RES4369_MACPHY_CLK_AVAIL_MAIN | RES4369_MACPHY_CLK_AVAIL_AUX,
		RES4369_HT_AVAIL, RES4369_AAON};

	rsc_per_chip_t *rsc = NULL;
	static rsc_per_chip_t rsc_4349 = {RES4349_HT_AVAIL,  RES4349_MACPHY_CLKAVAIL,
		RES4349_HT_START, RES4349_OTP_PU};
	static rsc_per_chip_t rsc_53573 = {RES53573_HT_AVAIL, RES53573_MACPHY_CLK_AVAIL,
		NO_SUCH_RESOURCE, RES53573_OTP_PU};

	switch (CHIPID(sih->chip)) {
	case BCM4313_CHIP_ID:
		rsc = &rsc_4313;
		break;
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
		rsc = &rsc_4314;
		break;
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
		rsc = &rsc_4324;
		break;
	case BCM4330_CHIP_ID:
		rsc = &rsc_4330;
		break;
	case BCM4331_CHIP_ID: /* fall through */
	case BCM43236_CHIP_ID:
		rsc = &rsc_43236;
		break;
	case BCM4334_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
		rsc = &rsc_4334;
		break;
	case BCM4335_CHIP_ID:
		rsc = &rsc_4335;
		break;
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
		rsc = &rsc_4336;
		break;
	case BCM43909_CHIP_ID:
	case BCM4345_CHIP_ID:	/* same resource defs for bits in struct rsc_per_chip_s as 4350 */
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
		rsc = &rsc_4350;
		break;
	case BCM4349_CHIP_GRPID:
		rsc = &rsc_4349;
		break;
	case BCM4352_CHIP_ID:
	case BCM43526_CHIP_ID:	/* usb variant of 4352 */
		rsc = &rsc_4352;
		break;
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
		rsc = &rsc_4360;
		break;
	case BCM43143_CHIP_ID:
		rsc = &rsc_43143;
		break;
	case BCM43239_CHIP_ID:
		rsc = &rsc_43239;
		break;
	CASE_BCM43602_CHIP:
		rsc = &rsc_43602;
		break;
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		rsc = &rsc_4365;
		break;
	case BCM7271_CHIP_ID:
		rsc = &rsc_7271;
		break;
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
		rsc = &rsc_43430;
		break;
	case BCM43012_CHIP_ID:
		rsc = &rsc_43012;
		break;
	case BCM4347_CHIP_GRPID:
		rsc = &rsc_4347;
		break;
	case BCM4364_CHIP_ID:
		rsc = &rsc_4364;
		break;
	case BCM53573_CHIP_GRPID:
		rsc = &rsc_53573;
		break;
	case BCM4369_CHIP_ID:
		rsc = &rsc_4369;
		break;
	default:
		ASSERT(0);
		break;
	}

	return rsc;
}; /* si_pmu_get_rsc_positions */

static const char BCMATTACHDATA(rstr_pllD)[] = "pll%d";
static const char BCMATTACHDATA(rstr_regD)[] = "reg%d";
static const char BCMATTACHDATA(rstr_chipcD)[] = "chipc%d";
static const char BCMATTACHDATA(rstr_rDt)[] = "r%dt";
static const char BCMATTACHDATA(rstr_rDd)[] = "r%dd";
static const char BCMATTACHDATA(rstr_Invalid_Unsupported_xtal_value_D)[] =
	"Invalid/Unsupported xtal value %d";
static const char BCMATTACHDATA(rstr_dacrate2g)[] = "dacrate2g";
static const char BCMATTACHDATA(rstr_clkreq_conf)[] = "clkreq_conf";
static const char BCMATTACHDATA(rstr_cbuckout)[] = "cbuckout";
static const char BCMATTACHDATA(rstr_cldo_ldo2)[] = "cldo_ldo2";
static const char BCMATTACHDATA(rstr_cldo_pwm)[] = "cldo_pwm";
static const char BCMATTACHDATA(rstr_cldo_burst)[] = "cldo_burst";
static const char BCMATTACHDATA(rstr_lpldo1)[] = "lpldo1";
static const char BCMATTACHDATA(rstr_lnldo1)[] = "lnldo1";
static const char BCMATTACHDATA(rstr_force_pwm_cbuck)[] = "force_pwm_cbuck";
static const char BCMATTACHDATA(rstr_xtalfreq)[] = "xtalfreq";
#if defined(SAVERESTORE) && defined(LDO3P3_MIN_RES_MASK)
static const char BCMATTACHDATA(rstr_ldo_prot)[] = "ldo_prot";
#endif /* SAVERESTORE && LDO3P3_MIN_RES_MASK */

/* The check for OTP parameters for the PLL control registers is done and if found the
 * registers are updated accordingly.
 */

void
BCMATTACHFN(si_pmu_otp_pllcontrol)(si_t *sih, osl_t *osh)
{
	char name[16];
	const char *otp_val;
	uint8 i;
	uint32 val;
	uint8 pll_ctrlcnt = 0;

	if (PMUREV(sih->pmurev) >= 5) {
		pll_ctrlcnt = (sih->pmucaps & PCAP5_PC_MASK) >> PCAP5_PC_SHIFT;
	} else {
		pll_ctrlcnt = (sih->pmucaps & PCAP_PC_MASK) >> PCAP_PC_SHIFT;
	}

	for (i = 0; i < pll_ctrlcnt; i++) {
		snprintf(name, sizeof(name), rstr_pllD, i);
		if ((otp_val = getvar(NULL, name)) == NULL)
			continue;

		val = (uint32)bcm_strtoul(otp_val, NULL, 0);
		si_pmu_pllcontrol(sih, i, ~0, val);
	}
}

/**
 * The check for OTP parameters for the Voltage Regulator registers is done and if found the
 * registers are updated accordingly.
 */
void
BCMATTACHFN(si_pmu_otp_vreg_control)(si_t *sih, osl_t *osh)
{
	char name[16];
	const char *otp_val;
	uint8 i;
	uint32 val;
	uint8 vreg_ctrlcnt = 0;

	if (PMUREV(sih->pmurev) >= 5) {
		vreg_ctrlcnt = (sih->pmucaps & PCAP5_VC_MASK) >> PCAP5_VC_SHIFT;
	} else {
		vreg_ctrlcnt = (sih->pmucaps & PCAP_VC_MASK) >> PCAP_VC_SHIFT;
	}

	for (i = 0; i < vreg_ctrlcnt; i++) {
		snprintf(name, sizeof(name), rstr_regD, i);
		if ((otp_val = getvar(NULL, name)) == NULL)
			continue;

		val = (uint32)bcm_strtoul(otp_val, NULL, 0);
		si_pmu_vreg_control(sih, i, ~0, val);
	}
}

/**
 * The check for OTP parameters for the chip control registers is done and if found the
 * registers are updated accordingly.
 */
void
BCMATTACHFN(si_pmu_otp_chipcontrol)(si_t *sih, osl_t *osh)
{
	uint32 val, cc_ctrlcnt, i;
	char name[16];
	const char *otp_val;

	if (PMUREV(sih->pmurev) >= 5) {
		cc_ctrlcnt = (sih->pmucaps & PCAP5_CC_MASK) >> PCAP5_CC_SHIFT;
	} else {
		cc_ctrlcnt = (sih->pmucaps & PCAP_CC_MASK) >> PCAP_CC_SHIFT;
	}

	for (i = 0; i < cc_ctrlcnt; i++) {
		snprintf(name, sizeof(name), rstr_chipcD, i);
		if ((otp_val = getvar(NULL, name)) == NULL)
			continue;

		val = (uint32)bcm_strtoul(otp_val, NULL, 0);
		si_pmu_chipcontrol(sih, i, 0xFFFFFFFF, val); /* writes to PMU chipctrl reg 'i' */
	}
}

/**
 * A chip contains one or more LDOs (Low Drop Out regulators). During chip bringup, it can turn out
 * that the default (POR) voltage of a regulator is not right or optimal.
 * This function is called only by si_pmu_swreg_init() for specific chips
 */
void
si_pmu_set_ldo_voltage(si_t *sih, osl_t *osh, uint8 ldo, uint8 voltage)
{
	uint8 sr_cntl_shift = 0, rc_shift = 0, shift = 0, mask = 0;
	uint8 addr = 0;
	uint8 do_reg2 = 0, rshift2 = 0, rc_shift2 = 0, mask2 = 0, addr2 = 0;

	BCM_REFERENCE(osh);

	ASSERT(sih->cccaps & CC_CAP_PMU);

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
		switch (ldo) {
		case SET_LDO_VOLTAGE_CLDO_PWM:
			addr = 4;
			rc_shift = 1;
			mask = 0xf;
			break;
		case SET_LDO_VOLTAGE_CLDO_BURST:
			addr = 4;
			rc_shift = 5;
			mask = 0xf;
			break;
		case SET_LDO_VOLTAGE_LNLDO1:
			addr = 4;
			rc_shift = 17;
			mask = 0xf;
			break;
		case SET_LDO_VOLTAGE_CBUCK_PWM:
			addr = 3;
			rc_shift = 0;
			mask = 0x1f;
			break;
		case SET_LDO_VOLTAGE_CBUCK_BURST:
			addr = 3;
			rc_shift = 5;
			mask = 0x1f;
			break;
		case SET_LNLDO_PWERUP_LATCH_CTRL:
			addr = 2;
			rc_shift = 17;
			mask = 0x3;
			break;
		default:
			ASSERT(FALSE);
			return;
		}
		break;
	case BCM4330_CHIP_ID:
		switch (ldo) {
		case SET_LDO_VOLTAGE_CBUCK_PWM:
			addr = 3;
			rc_shift = 0;
			mask = 0x1f;
			break;
		case SET_LDO_VOLTAGE_CBUCK_BURST:
			addr = 3;
			rc_shift = 5;
			mask = 0x1f;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case BCM4331_CHIP_ID:
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM43526_CHIP_ID:
		switch (ldo) {
		case  SET_LDO_VOLTAGE_PAREF:
			addr = 1;
			rc_shift = 0;
			mask = 0xf;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	CASE_BCM43602_CHIP:
		switch (ldo) {
		case  SET_LDO_VOLTAGE_PAREF:
			addr = 0;
			rc_shift = 29;
			mask = 0x7;
			do_reg2 = 1;
			addr2 = 1;
			rshift2 = 3;
			mask2 = 0x8;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case BCM4350_CHIP_ID:
		switch (ldo) {
		case  SET_LDO_VOLTAGE_LDO1:
			addr = 4;
			rc_shift = 12;
			mask = 0x7;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case BCM4345_CHIP_ID:
		switch (ldo) {
		case  SET_LDO_VOLTAGE_LDO3P3:
			addr = 4;
			rc_shift = 12;
			mask = 0x7;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case BCM4314_CHIP_ID:
		switch (ldo) {
		case  SET_LDO_VOLTAGE_LDO2:
			addr = 4;
			rc_shift = 14;
			mask = 0x7;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
		switch (ldo) {
		case SET_LDO_VOLTAGE_LDO1:
			addr = PMU_VREG4_ADDR;
			rc_shift = PMU_VREG4_LPLDO1_SHIFT;
			mask = PMU_VREG4_LPLDO1_MASK;
			break;
		case SET_LDO_VOLTAGE_LDO2:
			addr = PMU_VREG4_ADDR;
			rc_shift = PMU_VREG4_LPLDO2_LVM_SHIFT;
			mask = PMU_VREG4_LPLDO2_LVM_HVM_MASK;
			break;
		case SET_LDO_VOLTAGE_CLDO_PWM:
			addr = PMU_VREG4_ADDR;
			rc_shift = PMU_VREG4_CLDO_PWM_SHIFT;
			mask = PMU_VREG4_CLDO_PWM_MASK;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case BCM43143_CHIP_ID:
		switch (ldo) {
		case SET_LDO_VOLTAGE_CBUCK_PWM:
			addr = 0;
			rc_shift = 5;
			mask = 0xf;
			break;
		case SET_LDO_VOLTAGE_CBUCK_BURST:
			addr = 4;
			rc_shift = 8;
			mask = 0xf;
			break;
		case SET_LDO_VOLTAGE_LNLDO1:
			addr = 4;
			rc_shift = 14;
			mask = 0x7;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
		switch (ldo) {
		case  SET_LDO_VOLTAGE_LDO1:
			addr = 4;
			rc_shift = 15;
			mask = 0x7;
			break;
		case SET_LDO_VOLTAGE_CLDO_PWM:
			addr = 4;
			rc_shift = 4;
			mask = 0x7;
			do_reg2 = 1;
			addr2 = 6;
			rshift2 = 3;
			rc_shift2 = 18;
			mask2 = 0x18;
			break;
		case SET_LDO_VOLTAGE_CLDO_BURST:
			addr = 4;
			rc_shift = 1;
			mask = 0x7;
			do_reg2 = 1;
			addr2 = 6;
			rshift2 = 3;
			rc_shift2 = 16;
			mask2 = 0x18;
			break;
		case SET_LDO_VOLTAGE_LNLDO1:
			addr = 4;
			rc_shift = 8;
			mask = 0x7;
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		break;
	default:
		ASSERT(FALSE);
		return;
	}

	shift = sr_cntl_shift + rc_shift;

	pmu_corereg(sih, SI_CC_IDX, regcontrol_addr, /* PMU VREG register */
		~0, addr);
	pmu_corereg(sih, SI_CC_IDX, regcontrol_data,
		mask << shift, (voltage & mask) << shift);
	if (do_reg2) {
		/* rshift2 - right shift moves mask2 to bit 0, rc_shift2 - left shift in reg */
		si_pmu_vreg_control(sih, addr2, (mask2 >> rshift2) << rc_shift2,
			((voltage & mask2) >> rshift2) << rc_shift2);
	}
} /* si_pmu_set_ldo_voltage */

/* d11 slow to fast clock transition time in slow clock cycles */
#define D11SCC_SLOW2FAST_TRANSITION	2

/**
 * d11 core has a 'fastpwrup_dly' register that must be written to.
 * This function returns d11 slow to fast clock transition time in [us] units.
 * It does not write to the d11 core.
 */
uint16
BCMINITFN(si_pmu_fast_pwrup_delay)(si_t *sih, osl_t *osh)
{
	uint pmudelay = PMU_MAX_TRANSITION_DLY;
	pmuregs_t *pmu;
	uint origidx;
	uint32 ilp;			/* ILP clock frequency in [Hz] */
	rsc_per_chip_t *rsc;		/* chip specific resource bit positions */

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if (ISSIM_ENAB(sih)) {
		pmudelay = 70;
	} else {
		switch (CHIPID(sih->chip)) {
		case BCM43224_CHIP_ID:	case BCM43225_CHIP_ID:  case BCM43420_CHIP_ID:
		case BCM43421_CHIP_ID:
		case BCM43235_CHIP_ID:	case BCM43236_CHIP_ID:	case BCM43238_CHIP_ID:
		case BCM43237_CHIP_ID:	case BCM43239_CHIP_ID:
		case BCM43234_CHIP_ID:
		case BCM4331_CHIP_ID:
		case BCM43431_CHIP_ID:
		case BCM43131_CHIP_ID:
		case BCM43217_CHIP_ID:
		case BCM43227_CHIP_ID:
		case BCM43228_CHIP_ID:
		case BCM43428_CHIP_ID:
		case BCM6362_CHIP_ID:
		case BCM4313_CHIP_ID:
		case BCM43460_CHIP_ID:
		case BCM43526_CHIP_ID:
			pmudelay = 3700;
			break;
		case BCM4360_CHIP_ID:
		case BCM4352_CHIP_ID:
			if (CHIPREV(sih->chiprev) < 4) {
				pmudelay = 1500;
			} else {
				pmudelay = 3000;
			}
			break;
		case BCM4336_CHIP_ID:
		case BCM43362_CHIP_ID:
		case BCM4330_CHIP_ID:
		case BCM4314_CHIP_ID:
		case BCM43142_CHIP_ID:
		case BCM43143_CHIP_ID:
		case BCM43340_CHIP_ID:
		case BCM43341_CHIP_ID:
		case BCM4334_CHIP_ID:
		case BCM4345_CHIP_ID:
		case BCM4349_CHIP_GRPID:
		case BCM43018_CHIP_ID:
		case BCM43430_CHIP_ID:
		case BCM4365_CHIP_ID:
		case BCM4366_CHIP_ID:
		case BCM43664_CHIP_ID:
	        case BCM43666_CHIP_ID:
#ifdef BCM7271
		case BCM7271_CHIP_ID:
#endif /* BCM7271 */
		case BCM43909_CHIP_ID:
		case BCM4350_CHIP_ID:
		case BCM4354_CHIP_ID:
		case BCM43556_CHIP_ID:
		case BCM43558_CHIP_ID:
		case BCM43566_CHIP_ID:
		case BCM43568_CHIP_ID:
		case BCM43569_CHIP_ID:
		case BCM43570_CHIP_ID:
		case BCM4358_CHIP_ID:
		case BCM4324_CHIP_ID:
		case BCM43242_CHIP_ID:
		case BCM43243_CHIP_ID:
		case BCM53573_CHIP_GRPID:
			rsc = si_pmu_get_rsc_positions(sih);
			/* Retrieve time by reading it out of the hardware */
			ilp = si_ilp_clock(sih);
			pmudelay = (si_pmu_res_uptime(sih, osh, pmu, rsc->ht_avail) +
				D11SCC_SLOW2FAST_TRANSITION) * ((1000000 + ilp - 1) / ilp);
			pmudelay = (11 * pmudelay) / 10;
			break;
		case BCM43012_CHIP_ID:
			pmudelay = 1500; /* In micro seconds for 43012 chip */
			break;
		case BCM4335_CHIP_ID:
			rsc = si_pmu_get_rsc_positions(sih);
			ilp = si_ilp_clock(sih);
			pmudelay = (si_pmu_res_uptime(sih, osh, pmu, rsc->ht_avail) +
				D11SCC_SLOW2FAST_TRANSITION) * ((1000000 + ilp - 1) / ilp);
			/* Adding error margin of fixed 200usec instead of 10% */
			pmudelay = pmudelay + 200;
			break;
		CASE_BCM43602_CHIP:
			rsc = si_pmu_get_rsc_positions(sih);
			/* Retrieve time by reading it out of the hardware */
			ilp = si_ilp_clock(sih);
			pmudelay = (si_pmu_res_uptime(sih, osh, pmu, rsc->macphy_clkavail) +
				D11SCC_SLOW2FAST_TRANSITION) * ((1000000 + ilp - 1) / ilp);
			pmudelay = (11 * pmudelay) / 10;
			break;
		case BCM4347_CHIP_GRPID:
		case BCM4369_CHIP_ID:
			rsc = si_pmu_get_rsc_positions(sih);
			/* Retrieve time by reading it out of the hardware */
			ilp = si_ilp_clock(sih);
			pmudelay = (si_pmu_res_uptime(sih, osh, pmu, rsc->ht_avail) +
				D11SCC_SLOW2FAST_TRANSITION) * ((1000000 + ilp - 1) / ilp);
			pmudelay = (11 * pmudelay) / 10;
			break;
		/*
		 * Need to see whether there will be any change for 3x3 and 1x1.
		 */
		case BCM4364_CHIP_ID:
			rsc = si_pmu_get_rsc_positions(sih);
			/* Retrieve time by reading it out of the hardware */
			ilp = si_ilp_clock(sih);
			pmudelay = (si_pmu_res_uptime(sih, osh, pmu, rsc->ht_avail) +
				D11SCC_SLOW2FAST_TRANSITION) * ((1000000 + ilp - 1) / ilp);
			pmudelay = pmudelay + 200;
			break;
		default:
			break;
		}
	} /* if (ISSIM_ENAB(sih)) */

	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return (uint16)pmudelay;
} /* si_pmu_fast_pwrup_delay */

/*
 * During chip bringup, it can turn out that the 'hard wired' PMU dependencies are not fully
 * correct, or that up/down time values can be optimized. The following data structures and arrays
 * deal with that.
 */

/* Setup resource up/down timers */
typedef struct {
	uint8 resnum;
	uint32 updown;
} pmu_res_updown_t;

/* Change resource dependencies masks */
typedef struct {
	uint32 res_mask;		/* resources (chip specific) */
	int8 action;			/* action, e.g. RES_DEPEND_SET */
	uint32 depend_mask;		/* changes to the dependencies mask */
	bool (*filter)(si_t *sih);	/* action is taken when filter is NULL or return TRUE */
} pmu_res_depend_t;

/* Resource dependencies mask change action */
#define RES_DEPEND_SET		0	/* Override the dependencies mask */
#define RES_DEPEND_ADD		1	/* Add to the  dependencies mask */
#define RES_DEPEND_REMOVE	-1	/* Remove from the dependencies mask */

static const pmu_res_updown_t BCMATTACHDATA(bcm4328a0_res_updown)[] = {
	{ RES4328_EXT_SWITCHER_PWM, 0x0101 },
	{ RES4328_BB_SWITCHER_PWM, 0x1f01 },
	{ RES4328_BB_SWITCHER_BURST, 0x010f },
	{ RES4328_BB_EXT_SWITCHER_BURST, 0x0101 },
	{ RES4328_ILP_REQUEST, 0x0202 },
	{ RES4328_RADIO_SWITCHER_PWM, 0x0f01 },
	{ RES4328_RADIO_SWITCHER_BURST, 0x0f01 },
	{ RES4328_ROM_SWITCH, 0x0101 },
	{ RES4328_PA_REF_LDO, 0x0f01 },
	{ RES4328_RADIO_LDO, 0x0f01 },
	{ RES4328_AFE_LDO, 0x0f01 },
	{ RES4328_PLL_LDO, 0x0f01 },
	{ RES4328_BG_FILTBYP, 0x0101 },
	{ RES4328_TX_FILTBYP, 0x0101 },
	{ RES4328_RX_FILTBYP, 0x0101 },
	{ RES4328_XTAL_PU, 0x0101 },
	{ RES4328_XTAL_EN, 0xa001 },
	{ RES4328_BB_PLL_FILTBYP, 0x0101 },
	{ RES4328_RF_PLL_FILTBYP, 0x0101 },
	{ RES4328_BB_PLL_PU, 0x0701 }
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4328a0_res_depend)[] = {
	/* Adjust ILP request resource not to force ext/BB switchers into burst mode */
	{
		PMURES_BIT(RES4328_ILP_REQUEST),
		RES_DEPEND_SET,
		PMURES_BIT(RES4328_EXT_SWITCHER_PWM) | PMURES_BIT(RES4328_BB_SWITCHER_PWM),
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4325a0_res_updown_qt)[] = {
	{ RES4325_HT_AVAIL, 0x0300 },
	{ RES4325_BBPLL_PWRSW_PU, 0x0101 },
	{ RES4325_RFPLL_PWRSW_PU, 0x0101 },
	{ RES4325_ALP_AVAIL, 0x0100 },
	{ RES4325_XTAL_PU, 0x1000 },
	{ RES4325_LNLDO1_PU, 0x0800 },
	{ RES4325_CLDO_CBUCK_PWM, 0x0101 },
	{ RES4325_CBUCK_PWM, 0x0803 }
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4325a0_res_updown)[] = {
	{ RES4325_XTAL_PU, 0x1501 }
};


static const pmu_res_depend_t BCMATTACHDATA(bcm4325a0_res_depend)[] = {
	/* Adjust OTP PU resource dependencies - remove BB BURST */
	{
		PMURES_BIT(RES4325_OTP_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4325_BUCK_BOOST_BURST),
		NULL
	},
	/* Adjust ALP/HT Avail resource dependencies - bring up BB along if it is used. */
	{
		PMURES_BIT(RES4325_ALP_AVAIL) | PMURES_BIT(RES4325_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4325_BUCK_BOOST_BURST) | PMURES_BIT(RES4325_BUCK_BOOST_PWM),
		si_pmu_res_depfltr_bb
	},
	/* Adjust HT Avail resource dependencies - bring up RF switches along with HT. */
	{
		PMURES_BIT(RES4325_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4325_RX_PWRSW_PU) | PMURES_BIT(RES4325_TX_PWRSW_PU) |
		PMURES_BIT(RES4325_LOGEN_PWRSW_PU) | PMURES_BIT(RES4325_AFE_PWRSW_PU),
		NULL
	},
	/* Adjust ALL resource dependencies - remove CBUCK dependencies if it is not used. */
	{
		PMURES_BIT(RES4325_ILP_REQUEST) | PMURES_BIT(RES4325_ABUCK_BURST) |
		PMURES_BIT(RES4325_ABUCK_PWM) | PMURES_BIT(RES4325_LNLDO1_PU) |
		PMURES_BIT(RES4325C1_LNLDO2_PU) | PMURES_BIT(RES4325_XTAL_PU) |
		PMURES_BIT(RES4325_ALP_AVAIL) | PMURES_BIT(RES4325_RX_PWRSW_PU) |
		PMURES_BIT(RES4325_TX_PWRSW_PU) | PMURES_BIT(RES4325_RFPLL_PWRSW_PU) |
		PMURES_BIT(RES4325_LOGEN_PWRSW_PU) | PMURES_BIT(RES4325_AFE_PWRSW_PU) |
		PMURES_BIT(RES4325_BBPLL_PWRSW_PU) | PMURES_BIT(RES4325_HT_AVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4325B0_CBUCK_LPOM) | PMURES_BIT(RES4325B0_CBUCK_BURST) |
		PMURES_BIT(RES4325B0_CBUCK_PWM),
		si_pmu_res_depfltr_ncb
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4315a0_res_updown_qt)[] = {
	{ RES4315_HT_AVAIL, 0x0101 },
	{ RES4315_XTAL_PU, 0x0100 },
	{ RES4315_LNLDO1_PU, 0x0100 },
	{ RES4315_PALDO_PU, 0x0100 },
	{ RES4315_CLDO_PU, 0x0100 },
	{ RES4315_CBUCK_PWM, 0x0100 },
	{ RES4315_CBUCK_BURST, 0x0100 },
	{ RES4315_CBUCK_LPOM, 0x0100 }
};


static const pmu_res_updown_t BCMATTACHDATA(bcm4315a0_res_updown)[] = {
	{ RES4315_XTAL_PU, 0x2501 }
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4315a0_res_depend)[] = {
	/* Adjust OTP PU resource dependencies - not need PALDO unless write */
	{
		PMURES_BIT(RES4315_OTP_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4315_PALDO_PU),
		si_pmu_res_depfltr_npaldo
	},
	/* Adjust ALP/HT Avail resource dependencies - bring up PALDO along if it is used. */
	{
		PMURES_BIT(RES4315_ALP_AVAIL) | PMURES_BIT(RES4315_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4315_PALDO_PU),
		si_pmu_res_depfltr_paldo
	},
	/* Adjust HT Avail resource dependencies - bring up RF switches along with HT. */
	{
		PMURES_BIT(RES4315_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4315_RX_PWRSW_PU) | PMURES_BIT(RES4315_TX_PWRSW_PU) |
		PMURES_BIT(RES4315_LOGEN_PWRSW_PU) | PMURES_BIT(RES4315_AFE_PWRSW_PU),
		NULL
	},
	/* Adjust ALL resource dependencies - remove CBUCK dependencies if it is not used. */
	{
		PMURES_BIT(RES4315_CLDO_PU) | PMURES_BIT(RES4315_ILP_REQUEST) |
		PMURES_BIT(RES4315_LNLDO1_PU) | PMURES_BIT(RES4315_OTP_PU) |
		PMURES_BIT(RES4315_LNLDO2_PU) | PMURES_BIT(RES4315_XTAL_PU) |
		PMURES_BIT(RES4315_ALP_AVAIL) | PMURES_BIT(RES4315_RX_PWRSW_PU) |
		PMURES_BIT(RES4315_TX_PWRSW_PU) | PMURES_BIT(RES4315_RFPLL_PWRSW_PU) |
		PMURES_BIT(RES4315_LOGEN_PWRSW_PU) | PMURES_BIT(RES4315_AFE_PWRSW_PU) |
		PMURES_BIT(RES4315_BBPLL_PWRSW_PU) | PMURES_BIT(RES4315_HT_AVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4315_CBUCK_LPOM) | PMURES_BIT(RES4315_CBUCK_BURST) |
		PMURES_BIT(RES4315_CBUCK_PWM),
		si_pmu_res_depfltr_ncb
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4329_res_updown)[] = {
	{ RES4329_XTAL_PU, 0x3201 },
	{ RES4329_PALDO_PU, 0x3501 }
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4329_res_depend)[] = {
	/* Make lnldo1 independent of CBUCK_PWM and CBUCK_BURST */
	{
		PMURES_BIT(RES4329_LNLDO1_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4329_CBUCK_PWM) | PMURES_BIT(RES4329_CBUCK_BURST),
		NULL
	},
	{
		PMURES_BIT(RES4329_CBUCK_BURST),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4329_CBUCK_LPOM) | PMURES_BIT(RES4329_PALDO_PU),
		NULL
	},
	{
		PMURES_BIT(RES4329_BBPLL_PWRSW_PU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4329_RX_PWRSW_PU) | PMURES_BIT(RES4329_TX_PWRSW_PU) |
		PMURES_BIT(RES4329_LOGEN_PWRSW_PU) | PMURES_BIT(RES4329_AFE_PWRSW_PU),
		NULL
	},
	/* Adjust HT Avail resource dependencies */
	{
		PMURES_BIT(RES4329_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4329_PALDO_PU) |
		PMURES_BIT(RES4329_RX_PWRSW_PU) | PMURES_BIT(RES4329_TX_PWRSW_PU) |
		PMURES_BIT(RES4329_LOGEN_PWRSW_PU) | PMURES_BIT(RES4329_AFE_PWRSW_PU),
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4319a0_res_updown_qt)[] = {
	{ RES4319_HT_AVAIL, 0x0101 },
	{ RES4319_XTAL_PU, 0x0100 },
	{ RES4319_LNLDO1_PU, 0x0100 },
	{ RES4319_PALDO_PU, 0x0100 },
	{ RES4319_CLDO_PU, 0x0100 },
	{ RES4319_CBUCK_PWM, 0x0100 },
	{ RES4319_CBUCK_BURST, 0x0100 },
	{ RES4319_CBUCK_LPOM, 0x0100 }
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4319a0_res_updown)[] = {
	{ RES4319_XTAL_PU, 0x3f01 }
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4319a0_res_depend)[] = {
	/* Adjust OTP PU resource dependencies - not need PALDO unless write */
	{
		PMURES_BIT(RES4319_OTP_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4319_PALDO_PU),
		si_pmu_res_depfltr_npaldo
	},
	/* Adjust HT Avail resource dependencies - bring up PALDO along if it is used. */
	{
		PMURES_BIT(RES4319_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4319_PALDO_PU),
		si_pmu_res_depfltr_paldo
	},
	/* Adjust HT Avail resource dependencies - bring up RF switches along with HT. */
	{
		PMURES_BIT(RES4319_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4319_RX_PWRSW_PU) | PMURES_BIT(RES4319_TX_PWRSW_PU) |
		PMURES_BIT(RES4319_RFPLL_PWRSW_PU) |
		PMURES_BIT(RES4319_LOGEN_PWRSW_PU) | PMURES_BIT(RES4319_AFE_PWRSW_PU),
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4336a0_res_updown_qt)[] = {
	{ RES4336_HT_AVAIL, 0x0101 },
	{ RES4336_XTAL_PU, 0x0100 },
	{ RES4336_CLDO_PU, 0x0100 },
	{ RES4336_CBUCK_PWM, 0x0100 },
	{ RES4336_CBUCK_BURST, 0x0100 },
	{ RES4336_CBUCK_LPOM, 0x0100 }
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4336a0_res_updown)[] = {
	{ RES4336_HT_AVAIL, 0x0D01}
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4336a0_res_depend)[] = {
	/* Just a dummy entry for now */
	{
		PMURES_BIT(RES4336_RSVD),
		RES_DEPEND_ADD,
		0,
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4330a0_res_updown_qt)[] = {
	{ RES4330_HT_AVAIL, 0x0101 },
	{ RES4330_XTAL_PU, 0x0100 },
	{ RES4330_CLDO_PU, 0x0100 },
	{ RES4330_CBUCK_PWM, 0x0100 },
	{ RES4330_CBUCK_BURST, 0x0100 },
	{ RES4330_CBUCK_LPOM, 0x0100 }
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4330a0_res_updown)[] = {
	{ RES4330_HT_AVAIL, 0x0e02}
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4330a0_res_depend)[] = {
	/* Just a dummy entry for now */
	{
		PMURES_BIT(RES4330_HT_AVAIL),
		RES_DEPEND_ADD,
		0,
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4334a0_res_updown_qt)[] = {{0, 0}};
static const pmu_res_updown_t BCMATTACHDATA(bcm4334a0_res_updown)[] = {{0, 0}};
static const pmu_res_depend_t BCMATTACHDATA(bcm4334a0_res_depend)[] = {{0, 0, 0, NULL}};

static const pmu_res_updown_t BCMATTACHDATA(bcm4324a0_res_updown_qt)[] = {
	{RES4324_SR_SAVE_RESTORE, 0x00320032}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4324a0_res_updown)[] = {
	{RES4324_SR_SAVE_RESTORE, 0x00020002},
#ifdef BCMUSBDEV_ENABLED
	{RES4324_XTAL_PU, 0x007d0001}
#else
	{RES4324_XTAL_PU, 0x00120001}
#endif
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4324b0_res_updown)[] = {
	{RES4324_SR_SAVE_RESTORE, 0x00050005},
	{RES4324_XTAL_PU, 0x00120001}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4335_res_updown)[] = {
	{RES4335_XTAL_PU, 0x00260002},
	{RES4335_ALP_AVAIL, 0x00020005},
	{RES4335_WL_CORE_RDY, 0x00020005},
	{RES4335_LQ_START, 0x00060002},
	{RES4335_SR_CLK_START, 0x00060002}
};
static const pmu_res_depend_t BCMATTACHDATA(bcm4335b0_res_depend)[] = {
	{
		PMURES_BIT(RES4335_ALP_AVAIL) |
		PMURES_BIT(RES4335_HT_START) |
		PMURES_BIT(RES4335_HT_AVAIL) |
		PMURES_BIT(RES4335_MACPHY_CLKAVAIL) |
		PMURES_BIT(RES4335_RADIO_PU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4335_OTP_PU) | PMURES_BIT(RES4335_LDO3P3_PU),
		NULL
	},
	{
		PMURES_BIT(RES4335_MINI_PMU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4335_XTAL_PU),
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4350_res_updown)[] = {
#if defined(SAVERESTORE) && !defined(SAVERESTORE_DISABLED)
#ifndef SRFAST
	{RES4350_SR_SAVE_RESTORE,	0x00190019},
#else
	{RES4350_SR_SAVE_RESTORE,	0x000F000F},
#endif
#endif /* SAVERESTORE && !SAVERESTORE_DISABLED */

	{RES4350_XTAL_PU,		0x00260002},
	{RES4350_LQ_AVAIL,		0x00010001},
	{RES4350_LQ_START,		0x00010001},
	{RES4350_WL_CORE_RDY,		0x00010001},
	{RES4350_ALP_AVAIL,		0x00010001},
	{RES4350_SR_CLK_STABLE,		0x00010001},
	{RES4350_SR_SLEEP,		0x00010001},
	{RES4350_HT_AVAIL,		0x00010001},

#ifndef SRFAST
	{RES4350_SR_PHY_PWRSW,		0x00120002},
	{RES4350_SR_VDDM_PWRSW,		0x00120002},
	{RES4350_SR_SUBCORE_PWRSW,	0x00120002},
#else /* SRFAST */
	{RES4350_RSVD_7,	        0x000c0001},
	{RES4350_PMU_BG_PU,		0x00010001},
	{RES4350_PMU_SLEEP,		0x00200004},
	{RES4350_CBUCK_LPOM_PU,		0x00010001},
	{RES4350_CBUCK_PFM_PU,		0x00010001},
	{RES4350_COLD_START_WAIT,	0x00010001},
	{RES4350_LNLDO_PU,		0x00200004},
	{RES4350_XTALLDO_PU,		0x00050001},
	{RES4350_LDO3P3_PU,		0x00200004},
	{RES4350_OTP_PU,		0x00010001},
	{RES4350_SR_CLK_START,		0x000f0001},
	{RES4350_PERST_OVR,		0x00010001},
	{RES4350_WL_CORE_RDY,		0x00010001},
	{RES4350_ALP_AVAIL,		0x00010001},
	{RES4350_MINI_PMU,		0x00010001},
	{RES4350_RADIO_PU,		0x00010001},
	{RES4350_SR_CLK_STABLE,		0x00010001},
	{RES4350_SR_SLEEP,		0x00010001},
	{RES4350_HT_START,		0x000f0001},
	{RES4350_HT_AVAIL,		0x00010001},
	{RES4350_MACPHY_CLKAVAIL,	0x00010001},
#endif /* SRFAST */
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4349_res_updown)[] = {
	{RES4349_BG_PU, 0x00080000},
	{RES4349_PMU_SLEEP, 0x001a0013},
	{RES4349_PALDO3P3_PU, 0x00080000},
	{RES4349_CBUCK_LPOM_PU, 0x00000000},
	{RES4349_CBUCK_PFM_PU, 0x00000000},
	{RES4349_COLD_START_WAIT, 0x00000000},
	{RES4349_RSVD_7, 0x0000000},
	{RES4349_LNLDO_PU, 0x00060000},
	{RES4349_XTALLDO_PU, 0x000a0000},
	{RES4349_LDO3P3_PU, 0x00080000},
	{RES4349_OTP_PU, 0x00000000},
	{RES4349_XTAL_PU, 0x00260000},
	{RES4349_SR_CLK_START, 0x00040000},
	{RES4349_LQ_AVAIL, 0x00000000},
	{RES4349_LQ_START, 0x00060000},
	{RES4349_PERST_OVR, 0x00000000},
	{RES4349_WL_CORE_RDY, 0x00000005},
	{RES4349_ILP_REQ, 0x00000000},
	{RES4349_ALP_AVAIL, 0x00000000},
	{RES4349_MINI_PMU, 0x00020000},
	{RES4349_RADIO_PU, 0x00030000},
	{RES4349_SR_CLK_STABLE, 0x00000000},
	{RES4349_SR_SAVE_RESTORE, 0x000e000e},
	{RES4349_SR_PHY_PWRSW, 0x00160000},
	{RES4349_SR_VDDM_PWRSW, 0x001b0000},
	{RES4349_SR_SUBCORE_PWRSW, 0x00160000},
	{RES4349_SR_SLEEP, 0x00000000},
	{RES4349_HT_START, 0x00060000},
	{RES4349_HT_AVAIL, 0x00000000},
	{RES4349_MACPHY_CLKAVAIL, 0x00000000},
};

/* Using a safe SAVE_RESTORE up/down time, it will get updated after openloop cal */
static const pmu_res_updown_t BCMATTACHDATA(bcm43012a0_res_updown_ds0)[] = {
	{RES43012_MEMLPLDO_PU,		0x00200020},
	{RES43012_PMU_SLEEP,		0x00a600a6},
	{RES43012_FAST_LPO,		0x00D20000},
	{RES43012_BTLPO_3P3,		0x007D0000},
	{RES43012_SR_POK,		0x00c80000},
	{RES43012_DUMMY_PWRSW,		0x01400000},
	{RES43012_DUMMY_LDO3P3,		0x00000000},
	{RES43012_DUMMY_BT_LDO3P3,	0x00000000},
	{RES43012_DUMMY_RADIO,		0x00000000},
	{RES43012_VDDB_VDDRET,		0x0020000a},
	{RES43012_HV_LDO3P3,		0x002C0000},
	{RES43012_XTAL_PU,		0x04000000},
	{RES43012_SR_CLK_START,		0x00080000},
	{RES43012_XTAL_STABLE,		0x00000000},
	{RES43012_FCBS,			0x00000000},
	{RES43012_CBUCK_MODE,		0x00000000},
	{RES43012_CORE_READY,		0x00000000},
	{RES43012_ILP_REQ,		0x00000000},
	{RES43012_ALP_AVAIL,		0x00280008},
	{RES43012_RADIOLDO_1P8,		0x00220000},
	{RES43012_MINI_PMU,		0x00220000},
	{RES43012_SR_SAVE_RESTORE,	0x02600260},
	{RES43012_PHY_PWRSW,		0x00800005},
	{RES43012_VDDB_CLDO,		0x0020000a},
	{RES43012_SUBCORE_PWRSW,	0x0060000a},
	{RES43012_SR_SLEEP,		0x00000000},
	{RES43012_HT_START,		0x00A00000},
	{RES43012_HT_AVAIL,		0x00000000},
	{RES43012_MACPHY_CLK_AVAIL,	0x00000000},
};

static const pmu_res_updown_t bcm43012a0_res_updown_ds1[] = {
	{RES43012_MEMLPLDO_PU,		0x00200020},
	{RES43012_PMU_SLEEP,		0x00000000},
	{RES43012_FAST_LPO,		0x00D20000},
	{RES43012_BTLPO_3P3,		0x007D0000},
	{RES43012_SR_POK,		0x00C60000},
	{RES43012_DUMMY_PWRSW,		0x00000000},
	{RES43012_DUMMY_LDO3P3,		0x044C0000},
	{RES43012_DUMMY_BT_LDO3P3,	0x053C0000},
	{RES43012_DUMMY_RADIO,		0x056C0000},
	{RES43012_VDDB_VDDRET,		0x00000000},
	{RES43012_HV_LDO3P3,		0x002C0000},
	{RES43012_XTAL_PU,		0x04000000},
	{RES43012_SR_CLK_START,		0x00800000},
	{RES43012_XTAL_STABLE,		0x00000000},
	{RES43012_FCBS,			0x00000000},
	{RES43012_CBUCK_MODE,		0x00000000},
	{RES43012_CORE_READY,		0x00000000},
	{RES43012_ILP_REQ,		0x00000000},
	{RES43012_ALP_AVAIL,		0x000000E0},
	{RES43012_RADIOLDO_1P8,		0x00220000},
	{RES43012_MINI_PMU,		0x00220000},
	{RES43012_SR_SAVE_RESTORE,	0x00000000},
	{RES43012_PHY_PWRSW,		0x038400A0},
	{RES43012_VDDB_CLDO,		0x00000000},
	{RES43012_SUBCORE_PWRSW,	0x020800A0},
	{RES43012_SR_SLEEP,		0x00000000},
	{RES43012_HT_START,		0x00A00000},
	{RES43012_HT_AVAIL,		0x00000000},
	{RES43012_MACPHY_CLK_AVAIL,	0x00000000},
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4364a0_res_updown)[] = {
	{RES4364_LPLDO_PU,		0x00020003},
	{RES4364_BG_PU,			0x001a0013},
	{RES4364_MEMLPLDO_PU,		0x00090003},
	{RES4364_PALDO3P3_PU,		0x00090000},
	{RES4364_CBUCK_1P2,		0x00110000},
	{RES4364_CBUCK_1V8,		0x00110000},
	{RES4364_COLD_START_WAIT,	0x00000000},
	{RES4364_SR_3x3_VDDM_PWRSW,	0x00200000},
	{RES4364_3x3_MACPHY_CLKAVAIL,	0x00000000},
	{RES4364_XTALLDO_PU,		0x000a0000},
	{RES4364_LDO3P3_PU,		0x000a0000},
	{RES4364_OTP_PU,		0x00000000},
	{RES4364_XTAL_PU,		0x00260000},
	{RES4364_SR_CLK_START,		0x00040000},
	{RES4364_3x3_RADIO_PU,		0x00020000},
	{RES4364_RF_LDO,		0x00400000},
	{RES4364_PERST_OVR,		0x00000000},
	{RES4364_WL_CORE_RDY,		0x00000005},
	{RES4364_ILP_REQ,		0x00000000},
	{RES4364_ALP_AVAIL,		0x00000000},
	{RES4364_1x1_MINI_PMU,		0x00030000},
	{RES4364_1x1_RADIO_PU,		0x00040000},
	{RES4364_SR_CLK_STABLE,		0x00000000},
	{RES4364_SR_SAVE_RESTORE,	0x000f000f},
	{RES4364_SR_PHY_PWRSW,		0x00160000},
	{RES4364_SR_VDDM_PWRSW,		0x001b0000},
	{RES4364_SR_SUBCORE_PWRSW,	0x00160000},
	{RES4364_SR_SLEEP,		0x00000000},
	{RES4364_HT_START,		0x00060000},
	{RES4364_HT_AVAIL,		0x00000000},
	{RES4364_MACPHY_CLKAVAIL,	0x00000000},
};

static const pmu_res_updown_t BCMATTACHDATA(bcm7271a0_res_updown)[] = {
	{RES7271_REGULATOR_PU,		0x00020002},
	{RES7271_WL_CORE_RDY,		0x00020002},
	{RES7271_ILP_REQ,		0x00020002},
	{RES7271_ALP_AVAIL,		0x00020002},
	{RES7271_HT_AVAIL,		0x00020002},
	{RES7271_BB_PLL_PU,		0x00020002},
	{RES7271_MINIPMU_PU,		0x00070002},
	{RES7271_RADIO_PU,		0x00020002},
	{RES7271_MACPHY_CLK_AVAIL,	0x00020002},
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4350_res_depend)[] = {
#ifdef SRFAST
	{
		PMURES_BIT(RES4350_LDO3P3_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4350_PMU_SLEEP),
		NULL
	},
	{
		PMURES_BIT(RES4350_WL_CORE_RDY) |
		PMURES_BIT(RES4350_ALP_AVAIL) |
		PMURES_BIT(RES4350_SR_CLK_STABLE) |
		PMURES_BIT(RES4350_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4350_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4350_SR_SLEEP) |
		PMURES_BIT(RES4350_MACPHY_CLKAVAIL) |
		PMURES_BIT(RES4350_MINI_PMU) |
		PMURES_BIT(RES4350_ILP_REQ) |
		PMURES_BIT(RES4350_HT_START) |
		PMURES_BIT(RES4350_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_SR_CLK_START) |
		PMURES_BIT(RES4350_WL_CORE_RDY) |
		PMURES_BIT(RES4350_ALP_AVAIL) |
		PMURES_BIT(RES4350_SR_CLK_STABLE) |
		PMURES_BIT(RES4350_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4350_SR_PHY_PWRSW) |
		PMURES_BIT(RES4350_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4350_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4350_SR_SLEEP) |
		PMURES_BIT(RES4350_RADIO_PU) |
		PMURES_BIT(RES4350_MACPHY_CLKAVAIL) |
		PMURES_BIT(RES4350_MINI_PMU) |
		PMURES_BIT(RES4350_HT_START) |
		PMURES_BIT(RES4350_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_RSVD_7),
		NULL
	},
	{
		PMURES_BIT(RES4350_SR_CLK_START) |
		PMURES_BIT(RES4350_WL_CORE_RDY) |
		PMURES_BIT(RES4350_SR_CLK_STABLE) |
		PMURES_BIT(RES4350_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4350_SR_SLEEP) |
		PMURES_BIT(RES4350_RADIO_PU) |
		PMURES_BIT(RES4350_CBUCK_PFM_PU) |
		PMURES_BIT(RES4350_MINI_PMU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_XTAL_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_SR_CLK_START) |
		PMURES_BIT(RES4350_WL_CORE_RDY) |
		PMURES_BIT(RES4350_SR_CLK_STABLE) |
		PMURES_BIT(RES4350_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4350_SR_SLEEP) |
		PMURES_BIT(RES4350_RADIO_PU) |
		PMURES_BIT(RES4350_CBUCK_PFM_PU) |
		PMURES_BIT(RES4350_MINI_PMU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_XTALLDO_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_COLD_START_WAIT) |
		PMURES_BIT(RES4350_XTALLDO_PU) |
		PMURES_BIT(RES4350_XTAL_PU) |
		PMURES_BIT(RES4350_SR_CLK_START) |
		PMURES_BIT(RES4350_SR_CLK_STABLE) |
		PMURES_BIT(RES4350_SR_PHY_PWRSW) |
		PMURES_BIT(RES4350_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4350_SR_SUBCORE_PWRSW),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4350_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_RSVD_7),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_XTALLDO_PU) |
		PMURES_BIT(RES4350_COLD_START_WAIT) |
		PMURES_BIT(RES4350_LNLDO_PU) |
		PMURES_BIT(RES4350_PMU_SLEEP) |
		PMURES_BIT(RES4350_CBUCK_LPOM_PU) |
		PMURES_BIT(RES4350_PMU_BG_PU) |
		PMURES_BIT(RES4350_LPLDO_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_LNLDO_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4350_PMU_SLEEP) |
		PMURES_BIT(RES4350_CBUCK_LPOM_PU) |
		PMURES_BIT(RES4350_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_ALP_AVAIL) |
		PMURES_BIT(RES4350_RADIO_PU) |
		PMURES_BIT(RES4350_HT_START) |
		PMURES_BIT(RES4350_HT_AVAIL) |
		PMURES_BIT(RES4350_MACPHY_CLKAVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4350_LQ_AVAIL) |
		PMURES_BIT(RES4350_LQ_START),
		NULL
	},
#else
	{0, 0, 0, NULL}
#endif /* SRFAST */
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4350_res_pciewar)[] = {
	{
#ifdef SRFAST
		PMURES_BIT(RES4350_SR_PHY_PWRSW) |
		PMURES_BIT(RES4350_SR_VDDM_PWRSW),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_XTALLDO_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_CBUCK_PFM_PU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LNLDO_PU) |
		PMURES_BIT(RES4350_COLD_START_WAIT),
		NULL
	},
	{
		PMURES_BIT(RES4350_OTP_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4350_CBUCK_PFM_PU),
		NULL
	},
	{
#endif /* SRFAST */
		PMURES_BIT(RES4350_LQ_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_LQ_START),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_PERST_OVR),
		RES_DEPEND_SET,
		PMURES_BIT(RES4350_LPLDO_PU) |
		PMURES_BIT(RES4350_PMU_BG_PU) |
		PMURES_BIT(RES4350_PMU_SLEEP) |
		PMURES_BIT(RES4350_CBUCK_LPOM_PU) |
		PMURES_BIT(RES4350_CBUCK_PFM_PU) |
		PMURES_BIT(RES4350_COLD_START_WAIT) |
		PMURES_BIT(RES4350_LNLDO_PU) |
		PMURES_BIT(RES4350_XTALLDO_PU) |
#ifdef SRFAST
		PMURES_BIT(RES4350_OTP_PU) |
#endif
		PMURES_BIT(RES4350_XTAL_PU) |
#ifdef SRFAST
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_SR_CLK_START) |
#endif
		PMURES_BIT(RES4350_SR_CLK_STABLE) |
		PMURES_BIT(RES4350_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4350_SR_PHY_PWRSW) |
		PMURES_BIT(RES4350_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4350_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4350_SR_SLEEP),
		NULL
	},
	{
		PMURES_BIT(RES4350_WL_CORE_RDY),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_PERST_OVR) |
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_ILP_REQ),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_ALP_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_PERST_OVR) |
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_RADIO_PU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_PERST_OVR) |
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_SR_CLK_STABLE),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_SR_SAVE_RESTORE),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_SR_SUBCORE_PWRSW),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU) |
#ifdef SRFAST
		PMURES_BIT(RES4350_XTALLDO_PU) |
#endif
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_SR_SLEEP),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_HT_START),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_PERST_OVR) |
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_PERST_OVR) |
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4350_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4350_PERST_OVR) |
		PMURES_BIT(RES4350_LDO3P3_PU) |
		PMURES_BIT(RES4350_OTP_PU),
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4360_res_updown)[] = {
	{RES4360_BBPLLPWRSW_PU,		0x00200001}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm43430_res_updown)[] = {
#if defined(SAVERESTORE) && !defined(SAVERESTORE_DISABLED)
#ifdef SRFAST
	{RES43430_XTAL_PU,          0x00160002}, /* Uptime changed for 37400Mhz only */
	{RES43430_PMU_SLEEP,        0x000d0004},
	{RES43430_RSVD_7,           0x000a0001},
	{RES43430_LNLDO_PU,         0x000d0004},
	{RES43430_LDO3P3_PU,        0x000d0004},
	{RES43430_SR_CLK_START,     0x00070001},
	{RES43430_SR_SAVE_RESTORE,  0x00040004},
	{RES43430_SR_SLEEP,         0x00010004},
	{RES43430_HT_START,         0x00070001},
#else
	{RES43430_PMU_SLEEP,        0x00200004},
	{RES43430_RSVD_7,           0x000c0001},
	{RES43430_LNLDO_PU,         0x00200004},
	{RES43430_LDO3P3_PU,        0x00200004},
	{RES43430_SR_CLK_START,     0x000f0001},
	{RES43430_SR_SAVE_RESTORE,  0x00060006},
	{RES43430_SR_SLEEP,         0x00010001},
	{RES43430_HT_START,         0x000f0001},
#endif /* SRFAST */
	{RES43430_LPLDO_PU,         0x00020002},
	{RES43430_BG_PU,            0x00010001},
	{RES43430_RSVD_3,           0x00020002},
	{RES43430_CBUCK_LPOM_PU,    0x00010001},
	{RES43430_CBUCK_PFM_PU,     0x00010001},
	{RES43430_COLD_START_WAIT,  0x00010001},
	{RES43430_RSVD_9,           0x00050001},
	{RES43430_OTP_PU,           0x00010001},
	{RES43430_XTAL_PU,          0x00300002},
	{RES43430_LQ_AVAIL,         0x00010001},
	{RES43430_LQ_START,         0x00010001},
	{RES43430_RSVD_16,          0x00010001},
	{RES43430_WL_CORE_RDY,      0x00010001},
	{RES43430_ILP_REQ,          0x00030003},
	{RES43430_ALP_AVAIL,        0x00010001},
	{RES43430_MINI_PMU,         0x00010001},
	{RES43430_RADIO_PU,         0x00010001},
	{RES43430_SR_CLK_STABLE,    0x00010001},
	{RES43430_SR_PHY_PWRSW,     0x00130002},
	{RES43430_SR_VDDM_PWRSW,    0x00130002},
	{RES43430_SR_SUBCORE_PWRSW, 0x00130002},
	{RES43430_HT_AVAIL,         0x00010001},
	{RES43430_MACPHY_CLK_AVAIL, 0x00010001},
#endif /* defined(SAVERESTORE) && !defined(SAVERESTORE_DISABLED) */
	{RES43430_XTAL_PU,          0x00300002}, /* Uptime changed to 1.5 msec (default) */
};

static const pmu_res_depend_t BCMATTACHDATA(bcm43430a0_res_depend)[] = {
#if defined(SAVERESTORE) && !defined(SAVERESTORE_DISABLED)
	{PMURES_BIT(RES43430_LPLDO_PU),         RES_DEPEND_SET,	0x00000000,	NULL},
	{PMURES_BIT(RES43430_BG_PU),            RES_DEPEND_SET, 0x00000001,	NULL},
	{PMURES_BIT(RES43430_PMU_SLEEP),        RES_DEPEND_SET, 0x00000003,	NULL},
	{PMURES_BIT(RES43430_RSVD_3),           RES_DEPEND_SET, 0x00000000,	NULL},
	{PMURES_BIT(RES43430_CBUCK_LPOM_PU),    RES_DEPEND_SET, 0x00000007,	NULL},
	{PMURES_BIT(RES43430_CBUCK_PFM_PU),     RES_DEPEND_SET, 0x00001157,	NULL},
	{PMURES_BIT(RES43430_COLD_START_WAIT),  RES_DEPEND_SET, 0x00000117,	NULL},
	{PMURES_BIT(RES43430_RSVD_7),          RES_DEPEND_SET, 0x00000157,	NULL},
	{PMURES_BIT(RES43430_LNLDO_PU),         RES_DEPEND_SET, 0x00000003,	NULL},
	{PMURES_BIT(RES43430_RSVD_9),          RES_DEPEND_SET, 0x00000000,	NULL},
	{PMURES_BIT(RES43430_LDO3P3_PU),        RES_DEPEND_SET, 0x00000003,	NULL},
	{PMURES_BIT(RES43430_OTP_PU),           RES_DEPEND_SET, 0x00000407,	NULL},
	{PMURES_BIT(RES43430_XTAL_PU),          RES_DEPEND_SET, 0x00000157,	NULL},
#ifdef SRFAST
	{PMURES_BIT(RES43430_SR_CLK_START),     RES_DEPEND_SET, 0x000015d7,	NULL},
#else
	{PMURES_BIT(RES43430_SR_CLK_START),     RES_DEPEND_SET, 0x000005d7,	NULL},
#endif /* SRFAST */
	{PMURES_BIT(RES43430_LQ_AVAIL),         RES_DEPEND_SET, 0x0ec2bdf7,	NULL},
	{PMURES_BIT(RES43430_LQ_START),         RES_DEPEND_SET, 0x0ec23df7,	NULL},
	{PMURES_BIT(RES43430_RSVD_16),          RES_DEPEND_SET, 0x00000000,	NULL},
	{PMURES_BIT(RES43430_WL_CORE_RDY),      RES_DEPEND_SET, 0x0ec03df7,	NULL},
	{PMURES_BIT(RES43430_ILP_REQ),          RES_DEPEND_SET, 0x0ec23df7,	NULL},
	{PMURES_BIT(RES43430_ALP_AVAIL),        RES_DEPEND_SET, 0x0ec23df7,	NULL},
	{PMURES_BIT(RES43430_MINI_PMU),         RES_DEPEND_SET, 0x06c03df7,	NULL},
	{PMURES_BIT(RES43430_RADIO_PU),         RES_DEPEND_SET, 0x0eda3df7,	NULL},
	{PMURES_BIT(RES43430_SR_CLK_STABLE),    RES_DEPEND_SET, 0x06003df7,	NULL},
	{PMURES_BIT(RES43430_SR_SAVE_RESTORE),  RES_DEPEND_SET, 0x06403df7,	NULL},
	{PMURES_BIT(RES43430_SR_PHY_PWRSW),     RES_DEPEND_SET, 0x00000000,	NULL},
	{PMURES_BIT(RES43430_SR_VDDM_PWRSW),    RES_DEPEND_SET, 0x000001d7,	NULL},
	{PMURES_BIT(RES43430_SR_SUBCORE_PWRSW), RES_DEPEND_SET, 0x00000dd7,	NULL},
	{PMURES_BIT(RES43430_SR_SLEEP),         RES_DEPEND_SET, 0x06c03df7,	NULL},
	{PMURES_BIT(RES43430_HT_START),         RES_DEPEND_SET, 0x0eca3df7,	NULL},
	{PMURES_BIT(RES43430_HT_AVAIL),         RES_DEPEND_SET, 0x1eca3df7,	NULL},
	{PMURES_BIT(RES43430_MACPHY_CLK_AVAIL), RES_DEPEND_SET, 0x3efa3df7, NULL},
#else
	/* Remove dependency on CBUCK LPOM PU, CBUCK PFM PU and LNLDO PU */
	{
		PMURES_BIT(RES43430_SR_CLK_STABLE) |
		PMURES_BIT(RES43430_SR_SAVE_RESTORE) |
		PMURES_BIT(RES43430_SR_PHY_PWRSW) |
		PMURES_BIT(RES43430_SR_VDDM_PWRSW) |
		PMURES_BIT(RES43430_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES43430_SR_SLEEP) |
		PMURES_BIT(RES43430_SR_CLK_START) |
		PMURES_BIT(RES43430_WL_CORE_RDY),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES43430_BG_PU) |
		PMURES_BIT(RES43430_PMU_SLEEP) |
		PMURES_BIT(RES43430_CBUCK_LPOM_PU) |
		PMURES_BIT(RES43430_CBUCK_PFM_PU) |
		PMURES_BIT(RES43430_COLD_START_WAIT) |
		PMURES_BIT(RES43430_LNLDO_PU) |
		PMURES_BIT(RES43430_LDO3P3_PU) |
		PMURES_BIT(RES43430_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES43430_COLD_START_WAIT),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES43430_CBUCK_LPOM_PU) |
		PMURES_BIT(RES43430_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES43430_CBUCK_LPOM_PU) |
		PMURES_BIT(RES43430_LDO3P3_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES43430_PMU_SLEEP),
		NULL
	},
	{
		PMURES_BIT(RES43430_LNLDO_PU),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES43430_PMU_SLEEP) |
		PMURES_BIT(RES43430_CBUCK_LPOM_PU) |
		PMURES_BIT(RES43430_CBUCK_PFM_PU),
		NULL
	},
	/* Remove LQ resource and LQ dependency from these resource */
	{
		PMURES_BIT(RES43430_ALP_AVAIL) |
		PMURES_BIT(RES43430_RADIO_PU) |
		PMURES_BIT(RES43430_HT_START) |
		PMURES_BIT(RES43430_HT_AVAIL) |
		PMURES_BIT(RES43430_MACPHY_CLK_AVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES43430_LQ_AVAIL) |
		PMURES_BIT(RES43430_LQ_START),
		NULL
	},
	{
		PMURES_BIT(RES43430_WL_CORE_RDY) |
		PMURES_BIT(RES43430_ILP_REQ) |
		PMURES_BIT(RES43430_ALP_AVAIL) |
		PMURES_BIT(RES43430_MINI_PMU) |
		PMURES_BIT(RES43430_RADIO_PU) |
		PMURES_BIT(RES43430_SR_CLK_STABLE) |
		PMURES_BIT(RES43430_SR_SAVE_RESTORE) |
		PMURES_BIT(RES43430_SR_PHY_PWRSW) |
		PMURES_BIT(RES43430_SR_VDDM_PWRSW) |
		PMURES_BIT(RES43430_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES43430_SR_SLEEP) |
		PMURES_BIT(RES43430_HT_START) |
		PMURES_BIT(RES43430_HT_AVAIL) |
		PMURES_BIT(RES43430_MACPHY_CLK_AVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES43430_SR_CLK_START) |
		PMURES_BIT(RES43430_SR_CLK_STABLE),
		NULL
	},
#endif /* defined(SAVERESTORE) && !defined(SAVERESTORE_DISABLED) */
};

static const pmu_res_updown_t BCMATTACHDATA(bcm43602_res_updown)[] = {
	{RES43602_SR_SAVE_RESTORE,	0x00190019},
	{RES43602_XTAL_PU,		0x00280002},
	{RES43602_RFLDO_PU,		0x00430005}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4345_res_updown)[] = {
#if defined(SAVERESTORE) && !defined(SAVERESTORE_DISABLED)
	{RES4345_SR_SAVE_RESTORE,	0x00190019},
#else
	{0, 0},
#endif /* SAVERESTORE && !SAVERESTORE_DISABLED */
	{RES4345_XTAL_PU,               0x600002},      /* 3 ms uptime for XTAL_PU */
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4345_res_depend)[] = {
#ifdef BCMPCIEDEV_ENABLED
	{
		PMURES_BIT(RES4345_PERST_OVR),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_XTALLDO_PU) |
		PMURES_BIT(RES4345_XTAL_PU) |
		PMURES_BIT(RES4345_SR_CLK_STABLE) |
		PMURES_BIT(RES4345_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4345_SR_PHY_PWRSW) |
		PMURES_BIT(RES4345_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4345_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4345_SR_SLEEP),
		NULL
	},
	{
		PMURES_BIT(RES4345_PERST_OVR),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4345_XTALLDO_PU) |
		PMURES_BIT(RES4345_SR_CLK_START) |
		PMURES_BIT(RES4345_LDO3P3_PU) |
		PMURES_BIT(RES4345_OTP_PU),
		NULL
	},
	{
		PMURES_BIT(RES4345_WL_CORE_RDY),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_PERST_OVR) |
		PMURES_BIT(RES4345_XTALLDO_PU) |
		PMURES_BIT(RES4345_XTAL_PU),
		NULL
	},
	{
		PMURES_BIT(RES4345_ALP_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES4345_RADIO_PU),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES4345_HT_START),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES4345_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES4345_MACPHY_CLK_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES4345_SR_CLK_STABLE) |
		PMURES_BIT(RES4345_SR_SAVE_RESTORE),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4345_XTALLDO_PU) |
		PMURES_BIT(RES4345_XTAL_PU),
		NULL
	},
	{
		PMURES_BIT(RES4345_ALP_AVAIL) |
		PMURES_BIT(RES4345_RADIO_PU) |
		PMURES_BIT(RES4345_HT_START) |
		PMURES_BIT(RES4345_HT_AVAIL) |
		PMURES_BIT(RES4345_MACPHY_CLK_AVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4345_LQ_AVAIL) |
		PMURES_BIT(RES4345_LQ_START),
		NULL
	},
#else
	{0, 0, 0, NULL}
#endif /* BCMPCIEDEV_ENABLED */
};

static const pmu_res_depend_t BCMATTACHDATA(bcm43012a0_res_depend_ds0)[] = {
	{0, 0, 0, NULL}
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4349a0_res_depend)[] = {
	{
/* RSRC 0 0x0 */
		PMURES_BIT(RES4349_LPLDO_PU),
		RES_DEPEND_SET,
		0x00000000,
		NULL
	},
	{
/* RSRC 1 0x1 */
		PMURES_BIT(RES4349_BG_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 2 0x3 */
		PMURES_BIT(RES4349_PMU_SLEEP),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_BG_PU) | PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 3 0x47 */
		PMURES_BIT(RES4349_PALDO3P3_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) |	PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 4 0x7 */
		PMURES_BIT(RES4349_CBUCK_LPOM_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 5 0x17 */
		PMURES_BIT(RES4349_CBUCK_PFM_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_CBUCK_LPOM_PU) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 6 0x7 */
		PMURES_BIT(RES4349_COLD_START_WAIT),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 7 0x0 */
		PMURES_BIT(RES4349_RSVD_7),
		RES_DEPEND_SET,
		0x00000000,
		NULL
	},
	{
/* RSRC 8 0x37 */
		PMURES_BIT(RES4349_LNLDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_CBUCK_PFM_PU) | PMURES_BIT(RES4349_CBUCK_LPOM_PU) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 9 0x47 */
		PMURES_BIT(RES4349_XTALLDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 10 0x7 */
		PMURES_BIT(RES4349_LDO3P3_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 11 0x407 */
		PMURES_BIT(RES4349_OTP_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_LDO3P3_PU) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 12 0x247 */
		PMURES_BIT(RES4349_XTAL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 13 0x7001647 */
		PMURES_BIT(RES4349_SR_CLK_START),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) | PMURES_BIT(RES4349_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 14 0xFC3B647 */
		PMURES_BIT(RES4349_LQ_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SLEEP) | PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) | PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_WL_CORE_RDY) | PMURES_BIT(RES4349_PERST_OVR) |
		PMURES_BIT(RES4349_LQ_START) | PMURES_BIT(RES4349_SR_CLK_START) |
		PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 15 0xFC33647 */
		PMURES_BIT(RES4349_LQ_START),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SLEEP) | PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) | PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_WL_CORE_RDY) | PMURES_BIT(RES4349_PERST_OVR) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 16 0x1247 */
		PMURES_BIT(RES4349_PERST_OVR),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 17 0xFC13647 */
		PMURES_BIT(RES4349_WL_CORE_RDY),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SLEEP) | PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) | PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_PERST_OVR) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 18 0xFC33647 */
		PMURES_BIT(RES4349_ILP_REQ),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SLEEP) | PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) | PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_WL_CORE_RDY) | PMURES_BIT(RES4349_PERST_OVR) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 19 0xFC33647 */
		PMURES_BIT(RES4349_ALP_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SLEEP) | PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) | PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_WL_CORE_RDY) | PMURES_BIT(RES4349_PERST_OVR) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 20 0x1247 */
		PMURES_BIT(RES4349_MINI_PMU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 21 0x10124F */
		PMURES_BIT(RES4349_RADIO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_MINI_PMU) |
		PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PALDO3P3_PU) | PMURES_BIT(RES4349_PMU_SLEEP) |
		PMURES_BIT(RES4349_BG_PU) | PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 22 0x7001647 */
		PMURES_BIT(RES4349_SR_CLK_STABLE),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) | PMURES_BIT(RES4349_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 23 0x7403647 */
		PMURES_BIT(RES4349_SR_SAVE_RESTORE),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) | PMURES_BIT(RES4349_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 24 0x2000047 */
		PMURES_BIT(RES4349_SR_PHY_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 25 0x47 */
		PMURES_BIT(RES4349_SR_VDDM_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 26 0x2000047 */
		PMURES_BIT(RES4349_SR_SUBCORE_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 27 0x7C03647 */
		PMURES_BIT(RES4349_SR_SLEEP),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) | PMURES_BIT(RES4349_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 28 0x7C03647 */
		PMURES_BIT(RES4349_HT_START),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) | PMURES_BIT(RES4349_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 29 0x1FC33647 */
		PMURES_BIT(RES4349_HT_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_HT_START) |
		PMURES_BIT(RES4349_SR_SLEEP) | PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) | PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_WL_CORE_RDY) | PMURES_BIT(RES4349_PERST_OVR) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PMU_SLEEP) | PMURES_BIT(RES4349_BG_PU) |
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 30 0x1FF3364F */
		PMURES_BIT(RES4349_MACPHY_CLKAVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_HT_START) |
		PMURES_BIT(RES4349_SR_SLEEP) | PMURES_BIT(RES4349_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4349_SR_VDDM_PWRSW) | PMURES_BIT(RES4349_SR_PHY_PWRSW) |
		PMURES_BIT(RES4349_SR_SAVE_RESTORE) | PMURES_BIT(RES4349_SR_CLK_STABLE) |
		PMURES_BIT(RES4349_RADIO_PU) | PMURES_BIT(RES4349_MINI_PMU) |
		PMURES_BIT(RES4349_WL_CORE_RDY) | PMURES_BIT(RES4349_PERST_OVR) |
		PMURES_BIT(RES4349_SR_CLK_START) | PMURES_BIT(RES4349_XTAL_PU) |
		PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_XTALLDO_PU) |
		PMURES_BIT(RES4349_COLD_START_WAIT) |
		PMURES_BIT(RES4349_PALDO3P3_PU) | PMURES_BIT(RES4349_PMU_SLEEP) |
		PMURES_BIT(RES4349_BG_PU) | PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	}
};

/* 53573 res up down times  Need to populate with correct values !!! */
static const pmu_res_updown_t BCMATTACHDATA(bcm53573_res_updown)[] = {
	{RES53573_REGULATOR_PU, 0x00000000},
	{RES53573_XTALLDO_PU, 0x000A0000},
	{RES53573_XTAL_PU, 0x01d50000},
	{RES53573_MINI_PMU, 0x00020000},
	{RES53573_RADIO_PU, 0x00040000},
	{RES53573_ILP_REQ, 0x00020000},
	{RES53573_ALP_AVAIL, 0x00000000},
	{RES53573_CPUPLL_LDO_PU, 0x00000000},
	{RES53573_CPU_PLL_PU, 0x00020000},
	{RES53573_WLAN_BB_PLL_PU, 0x00000000},
	{RES53573_MISCPLL_LDO_PU, 0x00000000},
	{RES53573_MISCPLL_PU, 0x00020000},
	{RES53573_AUDIOPLL_PU, 0x00000000},
	{RES53573_PCIEPLL_LDO_PU, 0x00000000},
	{RES53573_PCIEPLL_PU, 0x00020000},
	{RES53573_DDRPLL_LDO_PU, 0x00000000},
	{RES53573_DDRPLL_PU, 0x00020000},
	{RES53573_HT_AVAIL, 0x00000000},
	{RES53573_MACPHY_CLK_AVAIL, 0x00000000},
	{RES53573_OTP_PU, 0x00000000},
	{RES53573_RSVD20, 0x00000000},
};

/* 53573 Res dependency
Reference: http://confluence.broadcom.com/display/WLAN/
BCM53573+Top+Level+Architecture#BCM53573TopLevelArchitecture-PMUResourcetable
*/
static const pmu_res_depend_t BCMATTACHDATA(bcm53573a0_res_depend)[] = {
	{
/* RSRC 0 0x0 */
		PMURES_BIT(RES53573_REGULATOR_PU),
		RES_DEPEND_SET,
		0x00000000,
		NULL
	},
	{
/* RSRC 1 0x1 */
		PMURES_BIT(RES53573_XTALLDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 2 0x3 */
		PMURES_BIT(RES53573_XTAL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_XTALLDO_PU) | PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 3 0x3 */
		PMURES_BIT(RES53573_MINI_PMU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_XTALLDO_PU) | PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 4 0xF */
		PMURES_BIT(RES53573_RADIO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_MINI_PMU) | PMURES_BIT(RES53573_XTAL_PU) |
		PMURES_BIT(RES53573_XTALLDO_PU) | PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 5 0x3 */
		PMURES_BIT(RES53573_ILP_REQ),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 6 0x7 */
		PMURES_BIT(RES53573_ALP_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_OTP_PU) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 7 0x7 */
		PMURES_BIT(RES53573_CPUPLL_LDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_OTP_PU) | PMURES_BIT(RES53573_ALP_AVAIL) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 8 0x7 */
		PMURES_BIT(RES53573_CPU_PLL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_CPUPLL_LDO_PU) | PMURES_BIT(RES53573_OTP_PU) |
		PMURES_BIT(RES53573_ALP_AVAIL) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 9 0x87 */
		PMURES_BIT(RES53573_WLAN_BB_PLL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_ALP_AVAIL) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 10 0x187 */
		PMURES_BIT(RES53573_MISCPLL_LDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_ALP_AVAIL) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 11 0x587 */
		PMURES_BIT(RES53573_MISCPLL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_MISCPLL_LDO_PU) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 12  0x87 */
		PMURES_BIT(RES53573_AUDIOPLL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_ALP_AVAIL) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 13 0x187 */
		PMURES_BIT(RES53573_PCIEPLL_LDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_ALP_AVAIL) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 14 0x2187 */
		PMURES_BIT(RES53573_PCIEPLL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_PCIEPLL_LDO_PU) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 15  0x187 */
		PMURES_BIT(RES53573_DDRPLL_LDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_ALP_AVAIL) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 16  0x8187 */
		PMURES_BIT(RES53573_DDRPLL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_DDRPLL_LDO_PU) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 17 0x21c7  */
		PMURES_BIT(RES53573_HT_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_OTP_PU) |
		PMURES_BIT(RES53573_CPU_PLL_PU) | PMURES_BIT(RES53573_CPUPLL_LDO_PU) |
		PMURES_BIT(RES53573_ALP_AVAIL) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 18  */
		PMURES_BIT(RES53573_MACPHY_CLK_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_OTP_PU) | PMURES_BIT(RES53573_HT_AVAIL) |
		PMURES_BIT(RES53573_WLAN_BB_PLL_PU) |
		PMURES_BIT(RES53573_CPU_PLL_PU) | PMURES_BIT(RES53573_CPUPLL_LDO_PU) |
		PMURES_BIT(RES53573_ALP_AVAIL) | PMURES_BIT(RES53573_RADIO_PU) |
		PMURES_BIT(RES53573_MINI_PMU) |
		PMURES_BIT(RES53573_XTAL_PU) | PMURES_BIT(RES53573_XTALLDO_PU) |
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC 19 */
		PMURES_BIT(RES53573_OTP_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES53573_REGULATOR_PU),
		NULL
	},
	{
/* RSRC20  0x0 */
		PMURES_BIT(RES53573_RSVD20),
		RES_DEPEND_SET,
		0x00000000,
		NULL
	}
};

/* 4364 Resource dependencies for RSDB mode */
static const pmu_res_depend_t BCMATTACHDATA(bcm4364a0_res_depend_rsdb)[] = {
	{
/* RSRC 0 0x1 */
		PMURES_BIT(RES4364_LPLDO_PU),
		RES_DEPEND_SET,
		0,
		NULL
	},
	{
/* RSRC 1 0x1 */
		PMURES_BIT(RES4364_BG_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 2 0x1 */
		PMURES_BIT(RES4364_MEMLPLDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 3 0x67 */
		PMURES_BIT(RES4364_PALDO3P3_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1V8)| PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 4 0x47 */
		PMURES_BIT(RES4364_CBUCK_1P2),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 5 0x47 */
		PMURES_BIT(RES4364_CBUCK_1V8),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL

	},
	{
/* RSRC 6 0x7 */
		PMURES_BIT(RES4364_COLD_START_WAIT),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU),
		NULL
	},
	{
/* RSRC 7 0x57 */
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2)| PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 8 0x3FCBF6FF */
		PMURES_BIT(RES4364_3x3_MACPHY_CLKAVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_PALDO3P3_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_CBUCK_1V8) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_3x3_RADIO_PU) |
		PMURES_BIT(RES4364_RF_LDO) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_ALP_AVAIL) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP) |
		PMURES_BIT(RES4364_HT_AVAIL) |
		PMURES_BIT(RES4364_HT_START),
		NULL
	},
	{
/* RSRC 9 0x47 */
		PMURES_BIT(RES4364_XTALLDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 10 0x47 */
		PMURES_BIT(RES4364_LDO3P3_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),

		NULL
	},
	{
/* RSRC 11 0x447 */
		PMURES_BIT(RES4364_OTP_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) | PMURES_BIT(RES4364_LDO3P3_PU),
		NULL
	},
	{
/* RSRC 12 0x247 */
		PMURES_BIT(RES4364_XTAL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) | PMURES_BIT(RES4364_XTALLDO_PU),
		NULL
	},
	{
/* RSRC 13 0x70016D7 */
		PMURES_BIT(RES4364_SR_CLK_START),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 14 0x846F */
		PMURES_BIT(RES4364_3x3_RADIO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_PALDO3P3_PU) |
		PMURES_BIT(RES4364_CBUCK_1V8) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_RF_LDO),
		NULL
	},
	{
/* RSRC 15 0x67 */
		PMURES_BIT(RES4364_RF_LDO),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1V8)| PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 16 0x1247 */
		PMURES_BIT(RES4364_PERST_OVR),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) | PMURES_BIT(RES4364_XTAL_PU),

		NULL
	},
	{
/* RSRC 17 0xFC136D7 */
		PMURES_BIT(RES4364_WL_CORE_RDY),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP),
		NULL
	},
	{
/* RSRC 18 0xFC336D7 */
		PMURES_BIT(RES4364_ILP_REQ),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP),
		NULL
	},
	{
/* RSRC 19 0xFC336D7 */
		PMURES_BIT(RES4364_ALP_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP),
		NULL
	},
	{
/* RSRC 20 0x1647 */
		PMURES_BIT(RES4364_1x1_MINI_PMU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) | PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU),

		NULL
	},
	{
/* RSRC 21 101647 */
		PMURES_BIT(RES4364_1x1_RADIO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) | PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) | PMURES_BIT(RES4364_1x1_MINI_PMU),
		NULL
	},
	{
/* RSRC 22 0x70016D7 */
		PMURES_BIT(RES4364_SR_CLK_STABLE),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 23 0x74036D7 */
		PMURES_BIT(RES4364_SR_SAVE_RESTORE),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 24 0x6000047 */
		PMURES_BIT(RES4364_SR_PHY_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW),
		NULL
	},
	{
/* RSRC 25 0x47 */
		PMURES_BIT(RES4364_SR_VDDM_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 26 0x2000047 */
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW),
		NULL
	},
	{
/* RSRC 27 0x7C036D7 */
		PMURES_BIT(RES4364_SR_SLEEP),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 28 0x7C036D7 */
		PMURES_BIT(RES4364_HT_START),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),

		NULL
	},
	{
/* RSRC 29 0x1FCB36D7 */
		PMURES_BIT(RES4364_HT_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_ALP_AVAIL) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP) |
		PMURES_BIT(RES4364_HT_START),

		NULL
	},
	{
/* RSRC 30 0x3FFB36D7 */
		PMURES_BIT(RES4349_MACPHY_CLKAVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_ALP_AVAIL) |
		PMURES_BIT(RES4364_1x1_MINI_PMU) |
		PMURES_BIT(RES4364_1x1_RADIO_PU) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP) |
		PMURES_BIT(RES4364_HT_AVAIL) |
		PMURES_BIT(RES4364_HT_START),
		NULL
	}
};

static const pmu_res_depend_t bcm4364a0_res_depend_1x1[] = {
	{
/* RSRC 0 0x0 */
		PMURES_BIT(RES4364_LPLDO_PU),
		RES_DEPEND_SET,
		0x00000000,
		NULL
	},
	{
/* RSRC 1 0x1 */
		PMURES_BIT(RES4364_BG_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 2 0x1 */
		PMURES_BIT(RES4364_MEMLPLDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4349_LPLDO_PU),
		NULL
	},
	{
/* RSRC 3 0x67 */
		PMURES_BIT(RES4364_PALDO3P3_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1V8)| PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 4 0x47 */
		PMURES_BIT(RES4364_CBUCK_1P2),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 5 0x47 */
		PMURES_BIT(RES4364_CBUCK_1V8),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL

	},
	{
/* RSRC 6 0x7 */
		PMURES_BIT(RES4364_COLD_START_WAIT),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU),
		NULL
	},
	{
/* RSRC 7 0x57 */
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2)| PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 8 0x1FC3F6FF */
		PMURES_BIT(RES4364_3x3_MACPHY_CLKAVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_PALDO3P3_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_CBUCK_1V8) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_3x3_RADIO_PU) |
		PMURES_BIT(RES4364_RF_LDO) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP) |
		PMURES_BIT(RES4364_HT_START),
		NULL
	},
	{
/* RSRC 9 0x47 */
		PMURES_BIT(RES4364_XTALLDO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 10 0x47 */
		PMURES_BIT(RES4364_LDO3P3_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),

		NULL
	},
	{
/* RSRC 11 0x447 */
		PMURES_BIT(RES4364_OTP_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) | PMURES_BIT(RES4364_LDO3P3_PU),
		NULL
	},
	{
/* RSRC 12 0x247 */
		PMURES_BIT(RES4364_XTAL_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) | PMURES_BIT(RES4364_XTALLDO_PU),
		NULL
	},
	{
/* RSRC 13 0x7001647 */
		PMURES_BIT(RES4364_SR_CLK_START),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 14 0x84FF */
		PMURES_BIT(RES4364_3x3_RADIO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_PALDO3P3_PU) |
		PMURES_BIT(RES4364_CBUCK_1P2) |
		PMURES_BIT(RES4364_CBUCK_1V8) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_RF_LDO),
		NULL
	},
	{
/* RSRC 15 0xF7 */
		PMURES_BIT(RES4364_RF_LDO),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_CBUCK_1V8)| PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_CBUCK_1P2)| PMURES_BIT(RES4364_SR_3x3_VDDM_PWRSW),
		NULL
	},
	{
/* RSRC 16 0x1247 */
		PMURES_BIT(RES4364_PERST_OVR),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) | PMURES_BIT(RES4364_XTAL_PU),

		NULL
	},
	{
/* RSRC 17 0xFC13647 */
		PMURES_BIT(RES4364_WL_CORE_RDY),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP),
		NULL
	},
	{
/* RSRC 18 0xFC33647 */
		PMURES_BIT(RES4364_ILP_REQ),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP),
		NULL
	},
	{
/* RSRC 19 0xFC33647 */
		PMURES_BIT(RES4364_ALP_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP),
		NULL
	},
	{
/* RSRC 20 0x1647 */
		PMURES_BIT(RES4364_1x1_MINI_PMU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) | PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU),

		NULL
	},
	{
/* RSRC 21 101647 */
		PMURES_BIT(RES4364_1x1_RADIO_PU),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) | PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) | PMURES_BIT(RES4364_1x1_MINI_PMU),
		NULL
	},
	{
/* RSRC 22 0x7001647 */
		PMURES_BIT(RES4364_SR_CLK_STABLE),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 23 0x7403647 */
		PMURES_BIT(RES4364_SR_SAVE_RESTORE),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 24 0x2000047 */
		PMURES_BIT(RES4364_SR_PHY_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW),
		NULL
	},
	{
/* RSRC 25 0x47 */
		PMURES_BIT(RES4364_SR_VDDM_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT),
		NULL
	},
	{
/* RSRC 26 0x2000047 */
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW),
		NULL
	},
	{
/* RSRC 27 0x7C03647 */
		PMURES_BIT(RES4364_SR_SLEEP),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),
		NULL
	},
	{
/* RSRC 28 0x7C03647 */
		PMURES_BIT(RES4364_HT_START),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW),

		NULL
	},
	{
/* RSRC 29 0x1FCB3647 */
		PMURES_BIT(RES4364_HT_AVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_ALP_AVAIL) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP) |
		PMURES_BIT(RES4364_HT_START),

		NULL
	},
	{
/* RSRC 30 0x3FFB3647 */
		PMURES_BIT(RES4349_MACPHY_CLKAVAIL),
		RES_DEPEND_SET,
		PMURES_BIT(RES4364_LPLDO_PU) |
		PMURES_BIT(RES4364_BG_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU) |
		PMURES_BIT(RES4364_COLD_START_WAIT) |
		PMURES_BIT(RES4364_XTALLDO_PU) |
		PMURES_BIT(RES4364_LDO3P3_PU) |
		PMURES_BIT(RES4364_XTAL_PU) |
		PMURES_BIT(RES4364_SR_CLK_START) |
		PMURES_BIT(RES4364_PERST_OVR) |
		PMURES_BIT(RES4364_WL_CORE_RDY) |
		PMURES_BIT(RES4364_ALP_AVAIL) |
		PMURES_BIT(RES4364_1x1_MINI_PMU) |
		PMURES_BIT(RES4364_1x1_RADIO_PU) |
		PMURES_BIT(RES4364_SR_CLK_STABLE) |
		PMURES_BIT(RES4364_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4364_SR_PHY_PWRSW) |
		PMURES_BIT(RES4364_SR_VDDM_PWRSW) |
		PMURES_BIT(RES4364_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES4364_SR_SLEEP) |
		PMURES_BIT(RES4364_HT_AVAIL) |
		PMURES_BIT(RES4364_HT_START),
		NULL
	}
};

static const pmu_res_depend_t BCMATTACHDATA(bcm43602_res_depend)[] = {
	{
		PMURES_BIT(RES43602_SR_SUBCORE_PWRSW) | PMURES_BIT(RES43602_SR_CLK_STABLE) |
		PMURES_BIT(RES43602_SR_SAVE_RESTORE)  | PMURES_BIT(RES43602_SR_SLEEP) |
		PMURES_BIT(RES43602_LQ_START) | PMURES_BIT(RES43602_LQ_AVAIL) |
		PMURES_BIT(RES43602_WL_CORE_RDY) | PMURES_BIT(RES43602_ILP_REQ) |
		PMURES_BIT(RES43602_ALP_AVAIL) | PMURES_BIT(RES43602_RFLDO_PU) |
		PMURES_BIT(RES43602_HT_START) | PMURES_BIT(RES43602_HT_AVAIL) |
		PMURES_BIT(RES43602_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_SERDES_PU),
		NULL
	},
	/* set rsrc  7, 8, 9, 12, 13, 14 & 17 add (1<<10 | 1<<4 )] */
	{
		PMURES_BIT(RES43602_SR_CLK_START) | PMURES_BIT(RES43602_SR_PHY_PWRSW) |
		PMURES_BIT(RES43602_SR_SUBCORE_PWRSW) | PMURES_BIT(RES43602_SR_CLK_STABLE) |
		PMURES_BIT(RES43602_SR_SAVE_RESTORE) | PMURES_BIT(RES43602_SR_SLEEP) |
		PMURES_BIT(RES43602_WL_CORE_RDY),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_XTALLDO_PU) | PMURES_BIT(RES43602_XTAL_PU),
		NULL
	},
	/* set rsrc 11 add (1<<13 | 1<<12 | 1<<9 | 1<<8 | 1<<7 )] */
	{
		PMURES_BIT(RES43602_PERST_OVR),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_SR_CLK_START) | PMURES_BIT(RES43602_SR_PHY_PWRSW) |
		PMURES_BIT(RES43602_SR_SUBCORE_PWRSW) | PMURES_BIT(RES43602_SR_CLK_STABLE) |
		PMURES_BIT(RES43602_SR_SAVE_RESTORE),
		NULL
	},
	/* set rsrc 19, 21, 22, 23 & 24 remove ~(1<<16 | 1<<15 )] */
	{
		PMURES_BIT(RES43602_ALP_AVAIL) | PMURES_BIT(RES43602_RFLDO_PU) |
		PMURES_BIT(RES43602_HT_START) | PMURES_BIT(RES43602_HT_AVAIL) |
		PMURES_BIT(RES43602_MACPHY_CLKAVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES43602_LQ_START) | PMURES_BIT(RES43602_LQ_AVAIL),
		NULL
	}
};

static const pmu_res_depend_t BCMATTACHDATA(bcm43602_12x12_res_depend)[] = {
	/* set rsrc 19, 21, 22, 23 & 24 remove ~(1<<16 | 1<<15 )] */
	{	/* resources no longer dependent on resource that is going to be removed */
		PMURES_BIT(RES43602_LPLDO_PU)        | PMURES_BIT(RES43602_REGULATOR)        |
		PMURES_BIT(RES43602_PMU_SLEEP)       | PMURES_BIT(RES43602_RSVD_3)           |
		PMURES_BIT(RES43602_XTALLDO_PU)      | PMURES_BIT(RES43602_SERDES_PU)        |
		PMURES_BIT(RES43602_BBPLL_PWRSW_PU)  | PMURES_BIT(RES43602_SR_CLK_START)     |
		PMURES_BIT(RES43602_SR_PHY_PWRSW)    | PMURES_BIT(RES43602_SR_SUBCORE_PWRSW) |
		PMURES_BIT(RES43602_XTAL_PU)         | PMURES_BIT(RES43602_PERST_OVR)        |
		PMURES_BIT(RES43602_SR_CLK_STABLE)   | PMURES_BIT(RES43602_SR_SAVE_RESTORE)  |
		PMURES_BIT(RES43602_SR_SLEEP)        | PMURES_BIT(RES43602_LQ_START)         |
		PMURES_BIT(RES43602_LQ_AVAIL)        | PMURES_BIT(RES43602_WL_CORE_RDY)      |
		PMURES_BIT(RES43602_ILP_REQ)         | PMURES_BIT(RES43602_ALP_AVAIL)        |
		PMURES_BIT(RES43602_RADIO_PU)        | PMURES_BIT(RES43602_RFLDO_PU)         |
		PMURES_BIT(RES43602_HT_START)        | PMURES_BIT(RES43602_HT_AVAIL)         |
		PMURES_BIT(RES43602_MACPHY_CLKAVAIL) | PMURES_BIT(RES43602_PARLDO_PU)        |
		PMURES_BIT(RES43602_RSVD_26),
		RES_DEPEND_REMOVE,
		/* resource that is going to be removed */
		PMURES_BIT(RES43602_LPLDO_PU),
		NULL
	}
};

static const pmu_res_depend_t BCMATTACHDATA(bcm43602_res_pciewar)[] = {
	{
		PMURES_BIT(RES43602_PERST_OVR),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_REGULATOR) |
		PMURES_BIT(RES43602_PMU_SLEEP) |
		PMURES_BIT(RES43602_XTALLDO_PU) |
		PMURES_BIT(RES43602_XTAL_PU) |
		PMURES_BIT(RES43602_RADIO_PU),
		NULL
	},
	{
		PMURES_BIT(RES43602_WL_CORE_RDY),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES43602_LQ_START),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES43602_LQ_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES43602_ALP_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES43602_HT_START),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES43602_HT_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_PERST_OVR),
		NULL
	},
	{
		PMURES_BIT(RES43602_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES43602_PERST_OVR),
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4360B1_res_updown)[] = {
	/* Need to change elements here, should get default values for this - 4360B1 */
	{RES4360_XTAL_PU,               0x00430002}, /* Changed for 4360B1 */
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4324a0_res_depend)[] = {
	{
		PMURES_BIT(RES4324_SR_PHY_PWRSW) | PMURES_BIT(RES4324_SR_PHY_PIC),
		RES_DEPEND_SET,
		0x00000000,
		NULL
	},
	{
		PMURES_BIT(RES4324_WL_CORE_READY) | PMURES_BIT(RES4324_ILP_REQ) |
		PMURES_BIT(RES4324_ALP_AVAIL) | PMURES_BIT(RES4324_RADIO_PU) |
		PMURES_BIT(RES4324_SR_CLK_STABLE) | PMURES_BIT(RES4324_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4324_SR_SUBCORE_PIC) | PMURES_BIT(RES4324_SR_MEM_PM0) |
		PMURES_BIT(RES4324_HT_AVAIL) | PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_LPLDO_PU) | PMURES_BIT(RES4324_RESET_PULLDN_DIS) |
		PMURES_BIT(RES4324_PMU_BG_PU) | PMURES_BIT(RES4324_HSIC_LDO_PU) |
		PMURES_BIT(RES4324_CBUCK_LPOM_PU) | PMURES_BIT(RES4324_CBUCK_PFM_PU) |
		PMURES_BIT(RES4324_CLDO_PU) | PMURES_BIT(RES4324_LPLDO2_LVM),
		NULL
	},
	{
		PMURES_BIT(RES4324_WL_CORE_READY) | PMURES_BIT(RES4324_ILP_REQ) |
		PMURES_BIT(RES4324_ALP_AVAIL) | PMURES_BIT(RES4324_RADIO_PU) |
		PMURES_BIT(RES4324_HT_AVAIL) | PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_SR_CLK_STABLE) | PMURES_BIT(RES4324_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4324_SR_SUBCORE_PWRSW) | PMURES_BIT(RES4324_SR_SUBCORE_PIC),
		NULL
	},
	{
		PMURES_BIT(RES4324_WL_CORE_READY) | PMURES_BIT(RES4324_ILP_REQ) |
		PMURES_BIT(RES4324_ALP_AVAIL) | PMURES_BIT(RES4324_RADIO_PU) |
		PMURES_BIT(RES4324_SR_CLK_STABLE) | PMURES_BIT(RES4324_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4324_HT_AVAIL) | PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_LNLDO1_PU) | PMURES_BIT(RES4324_LNLDO2_PU) |
		PMURES_BIT(RES4324_BBPLL_PU) | PMURES_BIT(RES4324_LQ_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4324_WL_CORE_READY) | PMURES_BIT(RES4324_ILP_REQ) |
		PMURES_BIT(RES4324_ALP_AVAIL) | PMURES_BIT(RES4324_RADIO_PU) |
		PMURES_BIT(RES4324_SR_CLK_STABLE) | PMURES_BIT(RES4324_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4324_HT_AVAIL) | PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_SR_MEM_PM0),
		NULL
	},
	{
		PMURES_BIT(RES4324_SR_CLK_STABLE) | PMURES_BIT(RES4324_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4324_SR_MEM_PM0),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_SR_SUBCORE_PWRSW) | PMURES_BIT(RES4324_SR_SUBCORE_PIC),
		NULL
	},
	{
		PMURES_BIT(RES4324_ILP_REQ) | PMURES_BIT(RES4324_ALP_AVAIL) |
		PMURES_BIT(RES4324_RADIO_PU) | PMURES_BIT(RES4324_HT_AVAIL) |
		PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_WL_CORE_READY),
		NULL
	},
	{
		PMURES_BIT(RES4324_ALP_AVAIL) | PMURES_BIT(RES4324_RADIO_PU) |
		PMURES_BIT(RES4324_HT_AVAIL) | PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_LQ_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4324_SR_SAVE_RESTORE),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_SR_CLK_STABLE),
		NULL
	},
	{
		PMURES_BIT(RES4324_SR_SUBCORE_PIC),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_SR_SUBCORE_PWRSW),
		NULL
	},
	{
		PMURES_BIT(RES4324_RADIO_PU) | PMURES_BIT(RES4324_HT_AVAIL) |
		PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4324_RADIO_PU) | PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_LDO3P3_PU) | PMURES_BIT(RES4324_PALDO_PU),
		NULL
	},
	{
		PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4324_RADIO_PU) | PMURES_BIT(RES4324_HT_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4324_ILP_REQ) | PMURES_BIT(RES4324_ALP_AVAIL) |
		PMURES_BIT(RES4324_RADIO_PU) | PMURES_BIT(RES4324_HT_AVAIL) |
		PMURES_BIT(RES4324_MACPHY_CLKAVAIL),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4324_SR_PHY_PWRSW) | PMURES_BIT(RES4324_SR_PHY_PIC),
		NULL
	},
	{
		PMURES_BIT(RES4324_SR_CLK_STABLE) | PMURES_BIT(RES4324_SR_SAVE_RESTORE) |
		PMURES_BIT(RES4324_SR_SUBCORE_PIC) | PMURES_BIT(RES4324_SR_MEM_PM0),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4324_WL_CORE_READY),
		NULL
	},
	{
		PMURES_BIT(RES4324_SR_SUBCORE_PIC) | PMURES_BIT(RES4324_SR_MEM_PM0),
		RES_DEPEND_REMOVE,
		PMURES_BIT(RES4324_LNLDO1_PU) | PMURES_BIT(RES4324_LNLDO2_PU) |
		PMURES_BIT(RES4324_BBPLL_PU) | PMURES_BIT(RES4324_LQ_AVAIL),
		NULL
	}
};

static const pmu_res_updown_t BCMATTACHDATA(bcm4334b0_res_updown_qt)[] = {{0, 0}};
static const pmu_res_updown_t BCMATTACHDATA(bcm4334b0_res_updown)[] = {
	/* In ILP clock (32768 Hz) at 30.5us per tick */
#ifndef SRFAST
	/* SAVERESTORE */
	{RES4334_LOGIC_RET, 0x00050005},
	{RES4334_MACPHY_RET, 0x000c000c},
	/* 0x00160001 - corresponds to around 700 uSeconds uptime */
	{RES4334_XTAL_PU, 0x00160001}
#else
	/* Fast SR wakeup */

	/* Make Cbuck LPOM shorter */
	{RES4334_CBUCK_LPOM_PU, 0xc000c},
	{RES4334_CBUCK_PFM_PU, 0x010001},

	/* XTAL up timer */
	{RES4334_XTAL_PU, 0x130001},

	/* Reduce 1 tick for below resources */
	{RES4334_PMU_BG_PU, 0x00060001},

	/* Reduce 2 tick for below resources */
	{RES4334_CLDO_PU, 0x00010001},
	{RES4334_LNLDO_PU, 0x00010001},
	{RES4334_LDO3P3_PU, 0x00020001},
	{RES4334_WL_PMU_PU, 0x00010001},
	{RES4334_HT_AVAIL, 0x00010001},

	/* Make all these resources to have 1 tick ILP clock only */
	{RES4334_HSIC_LDO_PU, 0x10001},
	{RES4334_LPLDO2_LVM, 0},
	{RES4334_WL_PWRSW_PU, 0},
	{RES4334_LQ_AVAIL, 0},
	{RES4334_MEM_SLEEP, 0},
	{RES4334_WL_CORE_READY, 0},
	{RES4334_ALP_AVAIL, 0},

	{RES4334_OTP_PU, 0x10001},
	{RES4334_MISC_PWRSW_PU, 0},
	{RES4334_SYNTH_PWRSW_PU, 0},
	{RES4334_RX_PWRSW_PU, 0},
	{RES4334_RADIO_PU, 0},
	{RES4334_VCO_LDO_PU, 0},
	{RES4334_AFE_LDO_PU, 0},
	{RES4334_RX_LDO_PU, 0},
	{RES4334_TX_LDO_PU, 0},
	{RES4334_MACPHY_CLK_AVAIL, 0},

	/* Reduce up down timer of Retention Mode */
	{RES4334_LOGIC_RET, 0x40004},
	{RES4334_MACPHY_RET, 0x70007},
#endif /* SRFAST */

};

static const pmu_res_depend_t BCMATTACHDATA(bcm4334b0_res_depend)[] = {
	{
		PMURES_BIT(RES4334_MACPHY_CLK_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4334_LDO3P3_PU),
		NULL
	},
#ifdef SRFAST
	{
		PMURES_BIT(RES4334_ALP_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4334_OTP_PU) | PMURES_BIT(RES4334_LDO3P3_PU),
		NULL
	}
#endif /* SRFAST */
};

static const pmu_res_depend_t BCMATTACHDATA(bcm4334b0_res_hsic_depend)[] = {
	{
		PMURES_BIT(RES4334_MACPHY_CLK_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4334_LDO3P3_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_LPLDO2_LVM),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_LNLDO_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_OTP_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_XTAL_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_WL_PWRSW_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_LQ_AVAIL),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_LOGIC_RET),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_MEM_SLEEP),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_MACPHY_RET),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_WL_CORE_READY),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_ILP_REQ),
		RES_DEPEND_REMOVE,
		(1 << RES4334_CBUCK_PFM_PU),
		NULL
	},

#ifdef SRFAST
	/* Fast SR wakeup */

	/* Make LNLDO and XTAL PU depends on WL_CORE_READY */
	{
		PMURES_BIT(RES4334_WL_CORE_READY),
		RES_DEPEND_ADD,
		(1 << RES4334_XTAL_PU) | (1 << RES4334_LNLDO_PU),
		NULL
	},

	/* remove dependency on CBuck PFM */
	{
		PMURES_BIT(RES4334_LPLDO2_LVM),
		RES_DEPEND_SET,
		0x5f,
		NULL
	},

	/* Make LNLDO depends on CBuck LPOM */
	{
		PMURES_BIT(RES4334_LNLDO_PU),
		RES_DEPEND_SET,
		0x1f,
		NULL
	},

	/* Remove LPLDO2_LVM from WL power switch PU */
	{
		PMURES_BIT(RES4334_WL_PWRSW_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_LPLDO2_LVM),
		NULL
	},

	/* Remove rsrc 19 (RES4334_ALP_AVAIL) from all radio resource dependency(20 to 28) */
	{
		PMURES_BIT(RES4334_MISC_PWRSW_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_SYNTH_PWRSW_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_RX_PWRSW_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_RADIO_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_WL_PMU_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_VCO_LDO_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_AFE_LDO_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_RX_LDO_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_TX_LDO_PU),
		RES_DEPEND_REMOVE,
		(1 << RES4334_ALP_AVAIL),
		NULL
	},
	{
		PMURES_BIT(RES4334_CBUCK_PFM_PU),
		RES_DEPEND_ADD,
		(1 << RES4334_XTAL_PU),
		NULL
	},
	{
		PMURES_BIT(RES4334_ALP_AVAIL),
		RES_DEPEND_ADD,
		PMURES_BIT(RES4334_OTP_PU) | PMURES_BIT(RES4334_LDO3P3_PU),
		NULL
	}
#endif /* SRFAST */
};

/* Filter '_depfltr_' functions used by the arrays above */

/** TRUE if the power topology uses the buck boost to provide 3.3V to VDDIO_RF and WLAN PA */
static bool
BCMATTACHFN(si_pmu_res_depfltr_bb)(si_t *sih)
{
	return (sih->boardflags & BFL_BUCKBOOST) != 0;
}

/** TRUE if the power topology doesn't use the cbuck. Key on chiprev also if the chip is BCM4325. */
static bool
BCMATTACHFN(si_pmu_res_depfltr_ncb)(si_t *sih)
{
	return ((sih->boardflags & BFL_NOCBUCK) != 0);
}

/** TRUE if the power topology uses the PALDO */
static bool
BCMATTACHFN(si_pmu_res_depfltr_paldo)(si_t *sih)
{
	return (sih->boardflags & BFL_PALDO) != 0;
}

/** TRUE if the power topology doesn't use the PALDO */
static bool
BCMATTACHFN(si_pmu_res_depfltr_npaldo)(si_t *sih)
{
	return (sih->boardflags & BFL_PALDO) == 0;
}

#define BCM94325_BBVDDIOSD_BOARDS(sih) (sih->boardtype == BCM94325DEVBU_BOARD || \
					sih->boardtype == BCM94325BGABU_BOARD)

/** To enable avb timer clock feature */
void si_pmu_avbtimer_enable(si_t *sih, osl_t *osh, bool set_flag)
{
	uint32 min_mask = 0, max_mask = 0;
	pmuregs_t *pmu;
	uint origidx;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if ((CHIPID(sih->chip) == BCM4360_CHIP_ID || CHIPID(sih->chip) == BCM43460_CHIP_ID) &&
		CHIPREV(sih->chiprev) >= 0x3) {
		int cst_ht = CST4360_RSRC_INIT_MODE(sih->chipst) & 0x1;
		if (cst_ht == 0) {
			/* Enable the AVB timers for proxd feature */
			min_mask = R_REG(osh, &pmu->min_res_mask);
			max_mask = R_REG(osh, &pmu->max_res_mask);
			if (set_flag) {
				max_mask |= PMURES_BIT(RES4360_AVB_PLL_PWRSW_PU);
				max_mask |= PMURES_BIT(RES4360_PCIE_TL_CLK_AVAIL);
				min_mask |= PMURES_BIT(RES4360_AVB_PLL_PWRSW_PU);
				min_mask |= PMURES_BIT(RES4360_PCIE_TL_CLK_AVAIL);
				W_REG(osh, &pmu->min_res_mask, min_mask);
				W_REG(osh, &pmu->max_res_mask, max_mask);
			} else {
				AND_REG(osh, &pmu->min_res_mask,
					~PMURES_BIT(RES4360_AVB_PLL_PWRSW_PU));
				AND_REG(osh, &pmu->min_res_mask,
					~PMURES_BIT(RES4360_PCIE_TL_CLK_AVAIL));
				AND_REG(osh, &pmu->max_res_mask,
					~PMURES_BIT(RES4360_AVB_PLL_PWRSW_PU));
				AND_REG(osh, &pmu->max_res_mask,
					~PMURES_BIT(RES4360_PCIE_TL_CLK_AVAIL));
			}
			/* Need to wait 100 millisecond for the uptime */
			OSL_DELAY(100);
		}
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
}

/**
 * Determines min/max rsrc masks. Normally hardware contains these masks, and software reads the
 * masks from hardware. Note that masks are sometimes dependent on chip straps.
 */
static void
si_pmu_res_masks(si_t *sih, uint32 *pmin, uint32 *pmax)
{
	uint32 min_mask = 0, max_mask = 0;
	uint rsrcs;

	/* # resources */
	rsrcs = (sih->pmucaps & PCAP_RC_MASK) >> PCAP_RC_SHIFT;

	/* determine min/max rsrc masks */
	switch (CHIPID(sih->chip)) {
	case BCM43224_CHIP_ID:	case BCM43225_CHIP_ID:	case BCM43421_CHIP_ID:
	case BCM43420_CHIP_ID:
	case BCM43235_CHIP_ID:	case BCM43236_CHIP_ID:	case BCM43238_CHIP_ID:
	case BCM43237_CHIP_ID:
	case BCM43234_CHIP_ID:
	case BCM4331_CHIP_ID:
	case BCM43431_CHIP_ID:
	case BCM6362_CHIP_ID:

	case BCM4360_CHIP_ID:
	case BCM4352_CHIP_ID:
		if (CHIPREV(sih->chiprev) >= 0x4) {
			min_mask = 0x103;
		}
		/* Continue - Don't break */
	case BCM43460_CHIP_ID:
	case BCM43526_CHIP_ID:
		if (CHIPREV(sih->chiprev) >= 0x3) {
			int cst_ht = CST4360_RSRC_INIT_MODE(sih->chipst) & 0x1;
			if (cst_ht == 0)
				max_mask = 0x1ff;
		}
		break;

	CASE_BCM43602_CHIP:
		/* as a bare minimum, have ALP clock running */
		min_mask = PMURES_BIT(RES43602_LPLDO_PU)  | PMURES_BIT(RES43602_REGULATOR)      |
			PMURES_BIT(RES43602_PMU_SLEEP)    | PMURES_BIT(RES43602_XTALLDO_PU)     |
			PMURES_BIT(RES43602_SERDES_PU)    | PMURES_BIT(RES43602_BBPLL_PWRSW_PU) |
			PMURES_BIT(RES43602_SR_CLK_START) | PMURES_BIT(RES43602_SR_PHY_PWRSW)   |
			PMURES_BIT(RES43602_SR_SUBCORE_PWRSW) | PMURES_BIT(RES43602_XTAL_PU)    |
			PMURES_BIT(RES43602_PERST_OVR)    | PMURES_BIT(RES43602_SR_CLK_STABLE)  |
			PMURES_BIT(RES43602_SR_SAVE_RESTORE) | PMURES_BIT(RES43602_SR_SLEEP)    |
			PMURES_BIT(RES43602_LQ_START)     | PMURES_BIT(RES43602_LQ_AVAIL)       |
			PMURES_BIT(RES43602_WL_CORE_RDY)  |
			PMURES_BIT(RES43602_ALP_AVAIL);

		if (sih->chippkg == BCM43602_12x12_PKG_ID) /* LPLDO WAR */
			min_mask &= ~PMURES_BIT(RES43602_LPLDO_PU);

		max_mask = (1<<3) | min_mask          | PMURES_BIT(RES43602_RADIO_PU)        |
			PMURES_BIT(RES43602_RFLDO_PU) | PMURES_BIT(RES43602_HT_START)        |
			PMURES_BIT(RES43602_HT_AVAIL) | PMURES_BIT(RES43602_MACPHY_CLKAVAIL);

#if defined(SAVERESTORE)
		/* min_mask is updated after SR code is downloaded to txfifo */
		if (SR_ENAB() && sr_isenab(sih)) {
			ASSERT(sih->chippkg != BCM43602_12x12_PKG_ID);
			min_mask = PMURES_BIT(RES43602_LPLDO_PU);
		}
#endif /* SAVERESTORE */
		break;

	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		/* as a bare minimum, have ALP clock running */
		min_mask = PMURES_BIT(RES4365_REGULATOR_PU) | PMURES_BIT(RES4365_XTALLDO_PU) |
			PMURES_BIT(RES4365_XTAL_PU) | PMURES_BIT(RES4365_CPU_PLLLDO_PU) |
			PMURES_BIT(RES4365_CPU_PLL_PU) | PMURES_BIT(RES4365_WL_CORE_RDY) |
			PMURES_BIT(RES4365_ALP_AVAIL) | PMURES_BIT(RES4365_HT_AVAIL);
		max_mask = min_mask |
			PMURES_BIT(RES4365_BB_PLLLDO_PU) | PMURES_BIT(RES4365_BB_PLL_PU) |
			PMURES_BIT(RES4365_MINIMU_PU) | PMURES_BIT(RES4365_RADIO_PU) |
			PMURES_BIT(RES4365_MACPHY_CLK_AVAIL);
		break;
	case BCM7271_CHIP_ID:
		min_mask = PMURES_BIT(RES7271_REGULATOR_PU) | PMURES_BIT(RES7271_WL_CORE_RDY) |
			PMURES_BIT(RES7271_ALP_AVAIL) | PMURES_BIT(RES7271_HT_AVAIL);
		max_mask = min_mask |
			PMURES_BIT(RES7271_BB_PLL_PU) | PMURES_BIT(RES7271_MINIPMU_PU) |
			PMURES_BIT(RES7271_RADIO_PU) | PMURES_BIT(RES7271_MACPHY_CLK_AVAIL);

		/* Do not power down these things via PMU */
		min_mask = max_mask;
		break;

	case BCM43143_CHIP_ID:
		max_mask = PMURES_BIT(RES43143_EXT_SWITCHER_PWM) | PMURES_BIT(RES43143_XTAL_PU) |
			PMURES_BIT(RES43143_ILP_REQUEST) | PMURES_BIT(RES43143_ALP_AVAIL) |
			PMURES_BIT(RES43143_WL_CORE_READY) | PMURES_BIT(RES43143_BBPLL_PWRSW_PU) |
			PMURES_BIT(RES43143_HT_AVAIL) | PMURES_BIT(RES43143_RADIO_PU) |
			PMURES_BIT(RES43143_MACPHY_CLK_AVAIL) | PMURES_BIT(RES43143_OTP_PU) |
			PMURES_BIT(RES43143_LQ_AVAIL);
		break;

	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
		/* Down to save the power. */
		min_mask = PMURES_BIT(RES4336_CBUCK_LPOM) | PMURES_BIT(RES4336_CLDO_PU) |
			PMURES_BIT(RES4336_LDO3P3_PU) | PMURES_BIT(RES4336_OTP_PU) |
			PMURES_BIT(RES4336_DIS_INT_RESET_PD);
		/* Allow (but don't require) PLL to turn on */
		max_mask = 0x1ffffff;
		break;

	case BCM4330_CHIP_ID:
		/* Down to save the power. */
		min_mask = PMURES_BIT(RES4330_CBUCK_LPOM) | PMURES_BIT(RES4330_CLDO_PU) |
			PMURES_BIT(RES4330_DIS_INT_RESET_PD) | PMURES_BIT(RES4330_LDO3P3_PU) |
			PMURES_BIT(RES4330_OTP_PU);
		/* Allow (but don't require) PLL to turn on */
		max_mask = 0xfffffff;
		break;

	case BCM4313_CHIP_ID:
		min_mask = PMURES_BIT(RES4313_BB_PU_RSRC) |
			PMURES_BIT(RES4313_XTAL_PU_RSRC) |
			PMURES_BIT(RES4313_ALP_AVAIL_RSRC) |
			PMURES_BIT(RES4313_SYNTH_PWRSW_RSRC) |
			PMURES_BIT(RES4313_BB_PLL_PWRSW_RSRC);
		max_mask = 0xffff;
		break;

	case BCM43239_CHIP_ID:
	case BCM4324_CHIP_ID:
		/* Read the min_res_mask register. Set the mask to 0 as we need to only read. */
		min_mask = pmu_corereg(sih, SI_CC_IDX, min_res_mask, 0, 0);
		/* Set the bits for all resources in the max mask except for the SR Engine */
		max_mask = 0x7FFFFFFF;
		break;
	case BCM4335_CHIP_ID:
		/* Read the min_res_mask register. Set the mask to 0 as we need to only read. */
		min_mask = pmu_corereg(sih, SI_CC_IDX, min_res_mask, 0, 0);
		/* For 4335, min res mask is set to 1 after disabling power islanding */
		/* Write a non-zero value in chipcontrol reg 3 */
		pmu_corereg(sih, SI_CC_IDX, chipcontrol_addr,
		           PMU_CHIPCTL3, PMU_CHIPCTL3);
		pmu_corereg(sih, SI_CC_IDX, chipcontrol_data,
		           0x1, 0x1);
#ifdef BCMPCIEDEV
		if (!BCMPCIEDEV_ENAB())
#endif /* BCMPCIEDEV */
		{
			/* shouldn't enable SR feature for pcie full dongle. */
			si_pmu_chipcontrol(sih, PMU_CHIPCTL3, 0x1, 0x1);
		}
		/* Set the bits for all resources in the max mask except for the SR Engine */
		max_mask = 0x7FFFFFFF;
		break;

	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
		/* use chip default min resource mask */
		min_mask = pmu_corereg(sih, SI_CC_IDX, min_res_mask, 0, 0);
#if defined(SAVERESTORE)
		/* min_mask is updated after SR code is downloaded to txfifo */
		if (SR_ENAB() && sr_isenab(sih))
			min_mask = PMURES_BIT(RES4345_LPLDO_PU);
#endif
		/* Set the bits for all resources in the max mask except for the SR Engine */
		max_mask = 0x7FFFFFFF;
		break;

	case BCM4349_CHIP_GRPID:
		/* use chip default min resource mask */
		min_mask = si_corereg(sih, SI_CC_IDX,
			OFFSETOF(chipcregs_t, min_res_mask), 0, 0);
#if defined(SAVERESTORE)
		/* min_mask is updated after SR code is downloaded to txfifo */
		if (SR_ENAB() && sr_isenab(sih))
			min_mask = PMURES_BIT(RES4349_LPLDO_PU);
#endif
		/* Set the bits for all resources in the max mask except for the SR Engine */
		max_mask = 0x7FFFFFFF;
		break;
	case BCM53573_CHIP_GRPID:
		max_mask = 0x1FFFFF;
		/* For now min = max  - later to be changed as per needed */
		min_mask = max_mask;
		break;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
		/* JIRA: SWWLAN-27486 Power consumption optimization when wireless is down */
		if (CST4350_IFC_MODE(sih->chipst) == CST4350_IFC_MODE_PCIE) {
			int L1substate = si_pcie_get_L1substate(sih);
			if (L1substate & 1)	/* L1.2 is enabled */
				min_mask = PMURES_BIT(RES4350_LPLDO_PU) |
					PMURES_BIT(RES4350_PMU_BG_PU) |
					PMURES_BIT(RES4350_PMU_SLEEP);
			else			/* use chip default min resource mask */
				min_mask = 0xfc22f77;
		} else {

			/* use chip default min resource mask */
			min_mask = pmu_corereg(sih, SI_CC_IDX,
				min_res_mask, 0, 0);
		}

#if defined(SAVERESTORE)
		/* min_mask is updated after SR code is downloaded to txfifo */
		if (SR_ENAB() && sr_isenab(sih)) {
			min_mask = PMURES_BIT(RES4350_LPLDO_PU);

			if (((CHIPID(sih->chip) == BCM4354_CHIP_ID) &&
				(CHIPREV(sih->chiprev) == 1) &&
				CST4350_CHIPMODE_PCIE(sih->chipst)) &&
				CST4350_PKG_WLCSP(sih->chipst)) {
				min_mask = PMURES_BIT(RES4350_LDO3P3_PU) |
					PMURES_BIT(RES4350_PMU_SLEEP) |
					PMURES_BIT(RES4350_PMU_BG_PU) |
					PMURES_BIT(RES4350_LPLDO_PU);
			}
		}
#endif /* SAVERESTORE */
		/* Set the bits for all resources in the max mask except for the SR Engine */
		max_mask = 0x7FFFFFFF;
		break;

	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
		/* Read the min_res_mask register. Set the mask to 0 as we need to only read. */
		min_mask = pmu_corereg(sih, SI_CC_IDX, min_res_mask, 0, 0);

		/* Set the bits for all resources in the max mask except for the SR Engine */
		max_mask = (1 << rsrcs) - 1;
		break;

	case BCM4314_CHIP_ID:
		/* set 4314 min mask to 0x3_f6ff -- ILP available (DozeMode) */
		if ((sih->chippkg == BCM4314SDIO_PKG_ID) ||
			(sih->chippkg == BCM4314SDIO_ARM_PKG_ID) ||
			(sih->chippkg == BCM4314SDIO_FPBGA_PKG_ID)) {
			min_mask = PMURES_BIT(RES4314_LPLDO_PU) |
				PMURES_BIT(RES4314_PMU_SLEEP_DIS) |
				PMURES_BIT(RES4314_PMU_BG_PU) |
				PMURES_BIT(RES4314_CBUCK_LPOM_PU) |
				PMURES_BIT(RES4314_CBUCK_PFM_PU) |
				PMURES_BIT(RES4314_CLDO_PU) |
				PMURES_BIT(RES4314_OTP_PU);
		} else {
			min_mask = PMURES_BIT(RES4314_LPLDO_PU) |
				PMURES_BIT(RES4314_PMU_SLEEP_DIS) |
				PMURES_BIT(RES4314_PMU_BG_PU) |
				PMURES_BIT(RES4314_CBUCK_LPOM_PU) |
				PMURES_BIT(RES4314_CBUCK_PFM_PU) |
				PMURES_BIT(RES4314_CLDO_PU) |
				PMURES_BIT(RES4314_LPLDO2_LVM) |
				PMURES_BIT(RES4314_WL_PMU_PU) |
				PMURES_BIT(RES4314_LDO3P3_PU) |
				PMURES_BIT(RES4314_OTP_PU) |
				PMURES_BIT(RES4314_WL_PWRSW_PU) |
				PMURES_BIT(RES4314_LQ_AVAIL) |
				PMURES_BIT(RES4314_LOGIC_RET) |
				PMURES_BIT(RES4314_MEM_SLEEP) |
				PMURES_BIT(RES4314_MACPHY_RET) |
				PMURES_BIT(RES4314_WL_CORE_READY) |
				PMURES_BIT(RES4314_SYNTH_PWRSW_PU);
		}
		max_mask = 0x3FFFFFFF;
		break;
	case BCM43142_CHIP_ID:
		/* Only PCIe */
		min_mask = PMURES_BIT(RES4314_LPLDO_PU) |
			PMURES_BIT(RES4314_PMU_SLEEP_DIS) |
			PMURES_BIT(RES4314_PMU_BG_PU) |
			PMURES_BIT(RES4314_CBUCK_LPOM_PU) |
			PMURES_BIT(RES4314_CBUCK_PFM_PU) |
			PMURES_BIT(RES4314_CLDO_PU) |
			PMURES_BIT(RES4314_LPLDO2_LVM) |
			PMURES_BIT(RES4314_WL_PMU_PU) |
			PMURES_BIT(RES4314_LDO3P3_PU) |
			PMURES_BIT(RES4314_OTP_PU) |
			PMURES_BIT(RES4314_WL_PWRSW_PU) |
			PMURES_BIT(RES4314_LQ_AVAIL) |
			PMURES_BIT(RES4314_LOGIC_RET) |
			PMURES_BIT(RES4314_MEM_SLEEP) |
			PMURES_BIT(RES4314_MACPHY_RET) |
			PMURES_BIT(RES4314_WL_CORE_READY);
		max_mask = 0x3FFFFFFF;
		break;
	case BCM4334_CHIP_ID:
		/* Use default for boot loader */
		min_mask = PMURES_BIT(RES4334_LPLDO_PU) | PMURES_BIT(RES4334_RESET_PULLDN_DIS) |
			PMURES_BIT(RES4334_OTP_PU) | PMURES_BIT(RES4334_WL_CORE_READY);

		max_mask = 0x7FFFFFFF;
		break;
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
		/* Use default for boot loader */
		min_mask = PMURES_BIT(RES4334_LPLDO_PU);


		max_mask = 0x7FFFFFFF;
		break;
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
		/* Min_mask Need to be modified */
		min_mask = 0xe820001;
#if defined(SAVERESTORE)
		if (SR_ENAB() && sr_isenab(sih))
			min_mask = PMURES_BIT(RES43430_LPLDO_PU);
#endif /* SAVERESTORE */
		max_mask = 0x7fffffff;
		break;
	case BCM43012_CHIP_ID:
		/* Set the bits for all resources in the max mask except for the SR Engine */
		max_mask = 0x7FFFFFFF;
		break;
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		min_mask = pmu_corereg(sih, SI_CC_IDX, min_res_mask, 0, 0);
		max_mask = 0x7FFFFFFF;
		break;
	case BCM4364_CHIP_ID:
#if defined(SAVERESTORE)
		if (SR_ENAB()) {
			min_mask = (PMURES_BIT(RES4364_LPLDO_PU) |
				PMURES_BIT(RES4364_MEMLPLDO_PU));
		}
#endif /* SAVERESTORE */
		max_mask = 0x7FFFFFFF;
		break;

	default:
		PMU_ERROR(("MIN and MAX mask is not programmed\n"));
		break;
	}

	/* nvram override */
	si_nvram_res_masks(sih, &min_mask, &max_mask);

	*pmin = min_mask;
	*pmax = max_mask;
} /* si_pmu_res_masks */

#if !defined(_CFEZ_)
#ifdef DUAL_PMU_SEQUENCE
static void
si_pmu_resdeptbl_upd(si_t *sih, osl_t *osh, pmuregs_t *pmu,
	const pmu_res_depend_t *restable, uint tablesz)
#else
static void
BCMATTACHFN(si_pmu_resdeptbl_upd)(si_t *sih, osl_t *osh, pmuregs_t *pmu,
	const pmu_res_depend_t *restable, uint tablesz)
#endif /* DUAL_PMU_SEQUENCE */
{
	uint i, rsrcs;

	if (tablesz == 0)
		return;

	ASSERT(restable != NULL);

	rsrcs = (sih->pmucaps & PCAP_RC_MASK) >> PCAP_RC_SHIFT;
	/* Program resource dependencies table */
	while (tablesz--) {
		if (restable[tablesz].filter != NULL &&
		    !(restable[tablesz].filter)(sih))
			continue;
		for (i = 0; i < rsrcs; i ++) {
			if ((restable[tablesz].res_mask &
			     PMURES_BIT(i)) == 0)
				continue;
			W_REG(osh, &pmu->res_table_sel, i);
			switch (restable[tablesz].action) {
				case RES_DEPEND_SET:
					PMU_MSG(("Changing rsrc %d res_dep_mask to 0x%x\n", i,
						restable[tablesz].depend_mask));
					W_REG(osh, &pmu->res_dep_mask,
					      restable[tablesz].depend_mask);
					break;
				case RES_DEPEND_ADD:
					PMU_MSG(("Adding 0x%x to rsrc %d res_dep_mask\n",
						restable[tablesz].depend_mask, i));
					OR_REG(osh, &pmu->res_dep_mask,
					       restable[tablesz].depend_mask);
					break;
				case RES_DEPEND_REMOVE:
					PMU_MSG(("Removing 0x%x from rsrc %d res_dep_mask\n",
						restable[tablesz].depend_mask, i));
					AND_REG(osh, &pmu->res_dep_mask,
						~restable[tablesz].depend_mask);
					break;
				default:
					ASSERT(0);
					break;
			}
		}
	}
} /* si_pmu_resdeptbl_upd */
#endif 

/** Initialize PMU hardware resources. */
void
BCMATTACHFN(si_pmu_res_init)(si_t *sih, osl_t *osh)
{
#if !defined(_CFEZ_)
	pmuregs_t *pmu;
	uint origidx;
	const pmu_res_updown_t *pmu_res_updown_table = NULL;
	uint pmu_res_updown_table_sz = 0;
	const pmu_res_depend_t *pmu_res_depend_table = NULL;
	uint pmu_res_depend_table_sz = 0;
	const pmu_res_depend_t *pmu_res_depend_pciewar_table[2] = {NULL, NULL};
	uint pmu_res_depend_pciewar_table_sz[2] = {0, 0};
	uint32 min_mask = 0, max_mask = 0;
	char name[8];
	const char *val;
	uint i, rsrcs;

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if (si_is_warmboot())
		return;
	/*
	 * Hardware contains the resource updown and dependency tables. Only if a chip has a
	 * hardware problem, software tables can be used to override hardware tables.
	 */
	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
		/* Optimize resources up/down timers */
		if (ISSIM_ENAB(sih)) {
			pmu_res_updown_table = bcm4336a0_res_updown_qt;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4336a0_res_updown_qt);
		} else {
			pmu_res_updown_table = bcm4336a0_res_updown;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4336a0_res_updown);
		}
		/* Optimize resources dependencies masks */
		pmu_res_depend_table = bcm4336a0_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm4336a0_res_depend);
		break;

	case BCM4330_CHIP_ID:
		/* Optimize resources up/down timers */
		if (ISSIM_ENAB(sih)) {
			pmu_res_updown_table = bcm4330a0_res_updown_qt;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4330a0_res_updown_qt);
		} else {
			pmu_res_updown_table = bcm4330a0_res_updown;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4330a0_res_updown);
		}
		/* Optimize resources dependencies masks */
		pmu_res_depend_table = bcm4330a0_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm4330a0_res_depend);
		break;
	case BCM4314_CHIP_ID:
		/* Enable SDIO wake up */
		if (!((sih->chippkg == BCM4314PCIE_ARM_PKG_ID) ||
			(sih->chippkg == BCM4314PCIE_PKG_ID) ||
			(sih->chippkg == BCM4314DEV_PKG_ID)))
		{
			si_pmu_chipcontrol(sih, PMU_CHIPCTL3, 1 << PMU_CC3_ENABLE_SDIO_WAKEUP_SHIFT,
				1 << PMU_CC3_ENABLE_SDIO_WAKEUP_SHIFT);
		}
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
		/* Optimize resources up/down timers */
		if (ISSIM_ENAB(sih)) {
			pmu_res_updown_table = bcm4334b0_res_updown_qt;
			/* pmu_res_updown_table_sz = ARRAYSIZE(bcm4334b0_res_updown_qt); */
		} else {
			pmu_res_updown_table = bcm4334b0_res_updown;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4334b0_res_updown);
		}
		/* Optimize resources dependancies masks */
		if (CST4334_CHIPMODE_HSIC(sih->chipst)) {
			pmu_res_depend_table = bcm4334b0_res_hsic_depend;
			pmu_res_depend_table_sz = ARRAYSIZE(bcm4334b0_res_hsic_depend);
		} else {
			pmu_res_depend_table = bcm4334b0_res_depend;
			pmu_res_depend_table_sz = ARRAYSIZE(bcm4334b0_res_depend);
		}
		break;
	case BCM43142_CHIP_ID:
	case BCM4334_CHIP_ID:
		/* Optimize resources up/down timers */
		if (ISSIM_ENAB(sih)) {
			pmu_res_updown_table = bcm4334a0_res_updown_qt;
			/* pmu_res_updown_table_sz = ARRAYSIZE(bcm4334a0_res_updown_qt); */
		} else {
			pmu_res_updown_table = bcm4334a0_res_updown;
			/* pmu_res_updown_table_sz = ARRAYSIZE(bcm4334a0_res_updown); */
		}
		/* Optimize resources dependencies masks */
		pmu_res_depend_table = bcm4334a0_res_depend;
		/* pmu_res_depend_table_sz = ARRAYSIZE(bcm4334a0_res_depend); */
		break;
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
		/* Need to change the up down timer for SR qt */
		if (ISSIM_ENAB(sih)) {
			pmu_res_updown_table = bcm4324a0_res_updown_qt;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4324a0_res_updown_qt);
		} else if ((CHIPID(sih->chip) == BCM4324_CHIP_ID) && (CHIPREV(sih->chiprev) <= 1)) {
			pmu_res_updown_table = bcm4324a0_res_updown;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4324a0_res_updown);
		} else {
			pmu_res_updown_table = bcm4324b0_res_updown;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4324b0_res_updown);
		}
		pmu_res_depend_table = bcm4324a0_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm4324a0_res_depend);
		break;
	case BCM4335_CHIP_ID:
		pmu_res_updown_table = bcm4335_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm4335_res_updown);
		pmu_res_depend_table = bcm4335b0_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm4335b0_res_depend);
		break;
	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
		pmu_res_updown_table = bcm4345_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm4345_res_updown);
		pmu_res_depend_table = bcm4345_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm4345_res_depend);
		break;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
		pmu_res_updown_table = bcm4350_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm4350_res_updown);
		pmu_res_depend_pciewar_table[0] = bcm4350_res_pciewar;
		pmu_res_depend_pciewar_table_sz[0] = ARRAYSIZE(bcm4350_res_pciewar);
		break;
	case BCM4349_CHIP_GRPID:
		 /* Enabling closed loop mode */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1, PMU_CC1_ENABLE_CLOSED_LOOP_MASK,
		PMU_CC1_ENABLE_CLOSED_LOOP);

		pmu_res_updown_table = bcm4349_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm4349_res_updown);
		pmu_res_depend_table = bcm4349a0_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm4349a0_res_depend);
		break;
	case BCM53573_CHIP_GRPID:
		si_pmu_chipcontrol(sih, PMU_53573_CHIPCTL3, PMU_53573_CC3_ENABLE_CLOSED_LOOP_MASK,
		PMU_53573_CC3_ENABLE_CLOSED_LOOP);

		pmu_res_updown_table = bcm53573_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm53573_res_updown);
		pmu_res_depend_table = bcm53573a0_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm53573a0_res_depend);
		break;
	case BCM4360_CHIP_ID:
	case BCM4352_CHIP_ID:
		if (CHIPREV(sih->chiprev) < 4) {
			pmu_res_updown_table = bcm4360_res_updown;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4360_res_updown);
		} else {
			/* FOR 4360B1 */
			pmu_res_updown_table = bcm4360B1_res_updown;
			pmu_res_updown_table_sz = ARRAYSIZE(bcm4360B1_res_updown);
		}
		break;
	CASE_BCM43602_CHIP:
		pmu_res_updown_table = bcm43602_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm43602_res_updown);
		pmu_res_depend_table = bcm43602_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm43602_res_depend);
		pmu_res_depend_pciewar_table[0] = bcm43602_res_pciewar;
		pmu_res_depend_pciewar_table_sz[0] = ARRAYSIZE(bcm43602_res_pciewar);
		if (sih->chippkg == BCM43602_12x12_PKG_ID) { /* LPLDO WAR */
			pmu_res_depend_pciewar_table[1] = bcm43602_12x12_res_depend;
			pmu_res_depend_pciewar_table_sz[1] = ARRAYSIZE(bcm43602_12x12_res_depend);
		}
		break;
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
		pmu_res_updown_table = bcm43430_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm43430_res_updown);
#ifdef SRFAST
		/* Last Entry for SRFAST is change for RES43430_XTAL_PU.
		 * When XTAL=37.4Mhz change value of RES43430_XTAL_PU. if XTAL 26Mhz
		 * then dont do changes to RES43430_XTAL_PU, remove this change
		 * by decreasing table size.
		 */
		if (SR_FAST_ENAB()) {
			if (si_alp_clock(sih)/1000 < 37400) {
				pmu_res_updown_table = pmu_res_updown_table + 1;
				pmu_res_updown_table_sz --;
			}
		}
#endif /* SRFAST */
		pmu_res_depend_table = bcm43430a0_res_depend;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm43430a0_res_depend);
		break;
	case BCM43012_CHIP_ID:
		pmu_res_updown_table = bcm43012a0_res_updown_ds0;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm43012a0_res_updown_ds0);
		pmu_res_depend_table = bcm43012a0_res_depend_ds0;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm43012a0_res_depend_ds0);
		break;
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		break;
	case BCM7271_CHIP_ID:
		pmu_res_updown_table = bcm7271a0_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm7271a0_res_updown);
		break;
	case BCM4364_CHIP_ID:
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1, PMU_CC1_ENABLE_CLOSED_LOOP_MASK,
			PMU_CC1_ENABLE_CLOSED_LOOP);
		pmu_res_updown_table = bcm4364a0_res_updown;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm4364a0_res_updown);
		pmu_res_depend_table = bcm4364a0_res_depend_rsdb;
		pmu_res_depend_table_sz = ARRAYSIZE(bcm4364a0_res_depend_rsdb);
		break;
	case BCM43143_CHIP_ID:
		/* POR values for up/down and dependency tables are sufficient. */
		/* fall through */
	default:
		break;
	}

	/* # resources */
	rsrcs = (sih->pmucaps & PCAP_RC_MASK) >> PCAP_RC_SHIFT;

	/* Program up/down timers */
	while (pmu_res_updown_table_sz--) {
		ASSERT(pmu_res_updown_table != NULL);
		PMU_MSG(("Changing rsrc %d res_updn_timer to 0x%x\n",
		         pmu_res_updown_table[pmu_res_updown_table_sz].resnum,
		         pmu_res_updown_table[pmu_res_updown_table_sz].updown));
		W_REG(osh, &pmu->res_table_sel,
		      pmu_res_updown_table[pmu_res_updown_table_sz].resnum);
		W_REG(osh, &pmu->res_updn_timer,
		      pmu_res_updown_table[pmu_res_updown_table_sz].updown);
	}

	/* Apply nvram overrides to up/down timers */
	for (i = 0; i < rsrcs; i ++) {
		uint32 r_val;
		snprintf(name, sizeof(name), rstr_rDt, i);
		if ((val = getvar(NULL, name)) == NULL)
			continue;
		r_val = (uint32)bcm_strtoul(val, NULL, 0);
		/* PMUrev = 13, pmu resource updown times are 12 bits(0:11 DT, 16:27 UT) */
		if (PMUREV(sih->pmurev) >= 13) {
			if (r_val < (1 << 16)) {
				uint16 up_time = (r_val >> 8) & 0xFF;
				r_val &= 0xFF;
				r_val |= (up_time << 16);
			}
		}
		PMU_MSG(("Applying %s=%s to rsrc %d res_updn_timer\n", name, val, i));
		W_REG(osh, &pmu->res_table_sel, (uint32)i);
		W_REG(osh, &pmu->res_updn_timer, r_val);
	}

	/* Program resource dependencies table */
	si_pmu_resdeptbl_upd(sih, osh, pmu, pmu_res_depend_table, pmu_res_depend_table_sz);

	/* Apply nvram overrides to dependencies masks */
	for (i = 0; i < rsrcs; i ++) {
		snprintf(name, sizeof(name), rstr_rDd, i);
		if ((val = getvar(NULL, name)) == NULL)
			continue;
		PMU_MSG(("Applying %s=%s to rsrc %d res_dep_mask\n", name, val, i));
		W_REG(osh, &pmu->res_table_sel, (uint32)i);
		W_REG(osh, &pmu->res_dep_mask, (uint32)bcm_strtoul(val, NULL, 0));
	}

	/* Initial any chip interface dependent PMU rsrc by looking at the
	 * chipstatus register to figure the selected interface
	 */
	if (BUSTYPE(sih->bustype) == PCI_BUS || BUSTYPE(sih->bustype) == SI_BUS) {
		bool is_pciedev = FALSE;

		if ((CHIPID(sih->chip) == BCM4345_CHIP_ID) && CST4345_CHIPMODE_PCIE(sih->chipst))
			is_pciedev = TRUE;
		else if (BCM4350_CHIP(sih->chip) && CST4350_CHIPMODE_PCIE(sih->chipst))
			is_pciedev = TRUE;
		else if (BCM43602_CHIP(sih->chip))
			is_pciedev = TRUE;

		for (i = 0; i < ARRAYSIZE(pmu_res_depend_pciewar_table); i++) {
			if (is_pciedev && pmu_res_depend_pciewar_table[i] &&
			    pmu_res_depend_pciewar_table_sz[i]) {
				si_pmu_resdeptbl_upd(sih, osh, pmu,
					pmu_res_depend_pciewar_table[i],
					pmu_res_depend_pciewar_table_sz[i]);
			}
		}
	}
	/* Determine min/max rsrc masks */
	si_pmu_res_masks(sih, &min_mask, &max_mask);

	/* Add min mask dependencies */
	min_mask |= si_pmu_res_deps(sih, osh, pmu, min_mask, FALSE);


	if (((CHIPID(sih->chip) == BCM4360_CHIP_ID) || (CHIPID(sih->chip) == BCM4352_CHIP_ID)) &&
	    (CHIPREV(sih->chiprev) < 4) &&
	    ((CST4360_RSRC_INIT_MODE(sih->chipst) & 1) == 0)) {
		/* BBPLL */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG6, ~0, 0x09048562);
		/* AVB PLL */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG14, ~0, 0x09048562);
		si_pmu_pllupd(sih);
	} else if (((CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4352_CHIP_ID)) &&
		(CHIPREV(sih->chiprev) >= 4) &&
		((CST4360_RSRC_INIT_MODE(sih->chipst) & 1) == 0)) {
		/* Changes for 4360B1 */

		/* Enable REFCLK bit 11 */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1, 0x800, 0x800);

		/* BBPLL */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG6, ~0, 0x080004e2);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG7, ~0, 0xE);
		/* AVB PLL */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG14, ~0, 0x080004e2);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG15, ~0, 0xE);
		si_pmu_pllupd(sih);
	}
	/* disable PLL open loop operation */
	si_pll_closeloop(sih);

	if (max_mask) {
		/* Ensure there is no bit set in min_mask which is not set in max_mask */
		max_mask |= min_mask;

		/* First set the bits which change from 0 to 1 in max, then update the
		 * min_mask register and then reset the bits which change from 1 to 0
		 * in max. This is required as the bit in MAX should never go to 0 when
		 * the corresponding bit in min is still 1. Similarly the bit in min cannot
		 * be 1 when the corresponding bit in max is still 0.
		 */
		OR_REG(osh, &pmu->max_res_mask, max_mask);
	} else {
		/* First set the bits which change from 0 to 1 in max, then update the
		 * min_mask register and then reset the bits which change from 1 to 0
		 * in max. This is required as the bit in MAX should never go to 0 when
		 * the corresponding bit in min is still 1. Similarly the bit in min cannot
		 * be 1 when the corresponding bit in max is still 0.
		 */
		if (min_mask)
			OR_REG(osh, &pmu->max_res_mask, min_mask);
	}

	/* Program min resource mask */
	if (min_mask) {
		PMU_MSG(("Changing min_res_mask to 0x%x\n", min_mask));
		W_REG(osh, &pmu->min_res_mask, min_mask);
	}

	/* Program max resource mask */
	if (max_mask) {
		PMU_MSG(("Changing max_res_mask to 0x%x\n", max_mask));
		W_REG(osh, &pmu->max_res_mask, max_mask);
	}
#if defined(SAVERESTORE) && defined(LDO3P3_MIN_RES_MASK)
	if (SR_ENAB()) {
		/* Set the default state for LDO3P3 protection */
		if (getintvar(NULL, rstr_ldo_prot) == 1) {
			si_pmu_min_res_ldo3p3_set(wlc_hw->sih, wlc_hw->osh, TRUE);
		}
	}
#endif /* SAVERESTORE && LDO3P3_MIN_RES_MASK */

	/* request htavail thru pcie core */
	if (((CHIPID(sih->chip) == BCM4360_CHIP_ID) || (CHIPID(sih->chip) == BCM4352_CHIP_ID)) &&
	    (BUSTYPE(sih->bustype) == PCI_BUS) &&
	    (CHIPREV(sih->chiprev) < 4)) {
		uint32 pcie_clk_ctl_st;

		pcie_clk_ctl_st = si_corereg(sih, 3, 0x1e0, 0, 0);
		si_corereg(sih, 3, 0x1e0, ~0, (pcie_clk_ctl_st | CCS_HTAREQ));
	}

	si_pmu_wait_for_steady_state(sih, osh, pmu);
	/* Add some delay; allow resources to come up and settle. */
	OSL_DELAY(2000);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
#endif 
} /* si_pmu_res_init */

/* setup pll and query clock speed */
typedef struct {
	uint16	fref;	/* x-tal frequency in [hz] */
	uint8	xf;	/* x-tal index as contained in PMU control reg, see PMU programmers guide */
	uint8	p1div;
	uint8	p2div;
	uint8	ndiv_int;
	uint32	ndiv_frac;
} pmu1_xtaltab0_t;


/* 'xf' values corresponding to the 'xf' definition in the PMU control register */
enum xtaltab0_640 {
	XTALTAB0_640_12000K = 1,
	XTALTAB0_640_13000K,
	XTALTAB0_640_14400K,
	XTALTAB0_640_15360K,
	XTALTAB0_640_16200K,
	XTALTAB0_640_16800K,
	XTALTAB0_640_19200K,
	XTALTAB0_640_19800K,
	XTALTAB0_640_20000K,
	XTALTAB0_640_24000K,
	XTALTAB0_640_25000K,
	XTALTAB0_640_26000K,
	XTALTAB0_640_30000K,
	XTALTAB0_640_33600K,
	XTALTAB0_640_37400K,
	XTALTAB0_640_38400K,
	XTALTAB0_640_40000K,
	XTALTAB0_640_48000K,
	XTALTAB0_640_52000K
};

/**
 * given an x-tal frequency, this table specifies the PLL params to use to generate a 640Mhz output
 * clock. This output clock feeds the clock divider network. The defines of the form
 * PMU1_XTALTAB0_640_* index into this array.
 */
static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_640)[] = {
/*	fref      xf     p1div   p2div  ndiv_int  ndiv_frac */
	{12000,   1,       1,      1,     0x35,   0x555555}, /* array index 0 */
	{13000,   2,       1,      1,     0x31,   0x3B13B1},
	{14400,   3,       1,      1,     0x2C,   0x71C71C},
	{15360,   4,       1,      1,     0x29,   0xAAAAAA},
	{16200,   5,       1,      1,     0x27,   0x81948B},
	{16800,   6,       1,      1,     0x26,   0x186186},
	{19200,   7,       1,      1,     0x21,   0x555555},
	{19800,   8,       1,      1,     0x20,   0x5ABF5A},
	{20000,   9,       1,      1,     0x20,   0x000000},
	{24000,   10,      1,      1,     0x1A,   0xAAAAAA},
	{25000,   11,      1,      1,     0x19,   0x999999}, /* array index 10 */
	{26000,   12,      1,      1,     0x18,   0x9D89D8},
	{30000,   13,      1,      1,     0x15,   0x555555},
	{33600,   14,      1,      1,     0x13,   0x0C30C3},
	{37400,   15,      1,      1,     0x11,   0x1CBFA8},
	{38400,   16,      1,      1,     0x10,   0xAAAAAA},
	{40000,   17,      1,      1,     0x10,   0x000000},
	{48000,   18,      1,      1,     0x0D,   0x555555},
	{52000,   19,      1,      1,     0x0C,   0x4EC4EC}, /* array index 18 */
	{0,	      0,       0,      0,     0,      0	      }
};

/* Indices into array pmu1_xtaltab0_640[]. Keep array and these defines synchronized. */
#define PMU1_XTALTAB0_640_12000K	0
#define PMU1_XTALTAB0_640_13000K	1
#define PMU1_XTALTAB0_640_14400K	2
#define PMU1_XTALTAB0_640_15360K	3
#define PMU1_XTALTAB0_640_16200K	4
#define PMU1_XTALTAB0_640_16800K	5
#define PMU1_XTALTAB0_640_19200K	6
#define PMU1_XTALTAB0_640_19800K	7
#define PMU1_XTALTAB0_640_20000K	8
#define PMU1_XTALTAB0_640_24000K	9
#define PMU1_XTALTAB0_640_25000K	10
#define PMU1_XTALTAB0_640_26000K	11
#define PMU1_XTALTAB0_640_30000K	12
#define PMU1_XTALTAB0_640_33600K	13
#define PMU1_XTALTAB0_640_37400K	14
#define PMU1_XTALTAB0_640_38400K	15
#define PMU1_XTALTAB0_640_40000K	16
#define PMU1_XTALTAB0_640_48000K	17
#define PMU1_XTALTAB0_640_52000K	18

/* the following table is based on 880Mhz fvco */
static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_880)[] = {
	{12000,	1,	3,	22,	0x9,	0xFFFFEF},
	{13000,	2,	1,	6,	0xb,	0x483483},
	{14400,	3,	1,	10,	0xa,	0x1C71C7},
	{15360,	4,	1,	5,	0xb,	0x755555},
	{16200,	5,	1,	10,	0x5,	0x6E9E06},
	{16800,	6,	1,	10,	0x5,	0x3Cf3Cf},
	{19200,	7,	1,	4,	0xb,	0x755555},
	{19800,	8,	1,	11,	0x4,	0xA57EB},
	{20000,	9,	1,	11,	0x4,	0x0},
	{24000,	10,	3,	11,	0xa,	0x0},
	{25000,	11,	5,	16,	0xb,	0x0},
	{26000,	12,	1,	2,	0x10,	0xEC4EC4},
	{30000,	13,	3,	8,	0xb,	0x0},
	{33600,	14,	1,	2,	0xd,	0x186186},
	{38400,	15,	1,	2,	0xb,	0x755555},
	{40000,	16,	1,	2,	0xb,	0},
	{0,	0,	0,	0,	0,	0}
};


/* indices into pmu1_xtaltab0_880[] */
#define PMU1_XTALTAB0_880_12000K	0
#define PMU1_XTALTAB0_880_13000K	1
#define PMU1_XTALTAB0_880_14400K	2
#define PMU1_XTALTAB0_880_15360K	3
#define PMU1_XTALTAB0_880_16200K	4
#define PMU1_XTALTAB0_880_16800K	5
#define PMU1_XTALTAB0_880_19200K	6
#define PMU1_XTALTAB0_880_19800K	7
#define PMU1_XTALTAB0_880_20000K	8
#define PMU1_XTALTAB0_880_24000K	9
#define PMU1_XTALTAB0_880_25000K	10
#define PMU1_XTALTAB0_880_26000K	11
#define PMU1_XTALTAB0_880_30000K	12
#define PMU1_XTALTAB0_880_37400K	13
#define PMU1_XTALTAB0_880_38400K	14
#define PMU1_XTALTAB0_880_40000K	15

/* the following table is based on 1760Mhz fvco */
static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_1760)[] = {
	{12000,	1,	3,	44,	0x9,	0xFFFFEF},
	{13000,	2,	1,	12,	0xb,	0x483483},
	{14400,	3,	1,	20,	0xa,	0x1C71C7},
	{15360,	4,	1,	10,	0xb,	0x755555},
	{16200,	5,	1,	20,	0x5,	0x6E9E06},
	{16800,	6,	1,	20,	0x5,	0x3Cf3Cf},
	{19200,	7,	1,	18,	0x5,	0x17B425},
	{19800,	8,	1,	22,	0x4,	0xA57EB},
	{20000,	9,	1,	22,	0x4,	0x0},
	{24000,	10,	3,	22,	0xa,	0x0},
	{25000,	11,	5,	32,	0xb,	0x0},
	{26000,	12,	1,	4,	0x10,	0xEC4EC4},
	{30000,	13,	3,	16,	0xb,	0x0},
	{38400,	14,	1,	10,	0x4,	0x955555},
	{40000,	15,	1,	4,	0xb,	0},
	{0,	0,	0,	0,	0,	0}
};


/* indices into pmu1_xtaltab0_1760[] */
#define PMU1_XTALTAB0_1760_12000K	0
#define PMU1_XTALTAB0_1760_13000K	1
#define PMU1_XTALTAB0_1760_14400K	2
#define PMU1_XTALTAB0_1760_15360K	3
#define PMU1_XTALTAB0_1760_16200K	4
#define PMU1_XTALTAB0_1760_16800K	5
#define PMU1_XTALTAB0_1760_19200K	6
#define PMU1_XTALTAB0_1760_19800K	7
#define PMU1_XTALTAB0_1760_20000K	8
#define PMU1_XTALTAB0_1760_24000K	9
#define PMU1_XTALTAB0_1760_25000K	10
#define PMU1_XTALTAB0_1760_26000K	11
#define PMU1_XTALTAB0_1760_30000K	12
#define PMU1_XTALTAB0_1760_38400K	13
#define PMU1_XTALTAB0_1760_40000K	14

/* the following table is based on 1440Mhz fvco */
static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_1440)[] = {
	{12000,	1,	1,	1,	0x78,	0x0	},
	{13000,	2,	1,	1,	0x6E,	0xC4EC4E},
	{14400,	3,	1,	1,	0x64,	0x0	},
	{15360,	4,	1,	1,	0x5D,	0xC00000},
	{16200,	5,	1,	1,	0x58,	0xE38E38},
	{16800,	6,	1,	1,	0x55,	0xB6DB6D},
	{19200,	7,	1,	1,	0x4B,	0	},
	{19800,	8,	1,	1,	0x48,	0xBA2E8B},
	{20000,	9,	1,	1,	0x48,	0x0	},
	{25000,	10,	1,	1,	0x39,	0x999999},
	{26000, 11,     1,      1,      0x37,   0x627627},
	{30000,	12,	1,	1,	0x30,	0x0	},
	{37400, 13,     2,      1,     	0x4D, 	0x15E76	},
	{38400, 13,     2,      1,     	0x4B, 	0x0	},
	{40000,	14,	2,	1,	0x48,	0x0	},
	{48000,	15,	2,	1,	0x3c,	0x0	},
	{0,	0,	0,	0,	0,	0}
};

/* indices into pmu1_xtaltab0_1440[] */
#define PMU1_XTALTAB0_1440_12000K	0
#define PMU1_XTALTAB0_1440_13000K	1
#define PMU1_XTALTAB0_1440_14400K	2
#define PMU1_XTALTAB0_1440_15360K	3
#define PMU1_XTALTAB0_1440_16200K	4
#define PMU1_XTALTAB0_1440_16800K	5
#define PMU1_XTALTAB0_1440_19200K	6
#define PMU1_XTALTAB0_1440_19800K	7
#define PMU1_XTALTAB0_1440_20000K	8
#define PMU1_XTALTAB0_1440_25000K	9
#define PMU1_XTALTAB0_1440_26000K	10
#define PMU1_XTALTAB0_1440_30000K	11
#define PMU1_XTALTAB0_1440_37400K	12
#define PMU1_XTALTAB0_1440_38400K	13
#define PMU1_XTALTAB0_1440_40000K	14
#define PMU1_XTALTAB0_1440_48000K	15

#define XTAL_FREQ_24000MHZ		24000
#define XTAL_FREQ_30000MHZ		30000
#define XTAL_FREQ_37400MHZ		37400
#define XTAL_FREQ_48000MHZ		48000

/* 'xf' values corresponding to the 'xf' definition in the PMU control register */
enum xtaltab0_960 {
	XTALTAB0_960_12000K = 1,
	XTALTAB0_960_13000K,
	XTALTAB0_960_14400K,
	XTALTAB0_960_15360K,
	XTALTAB0_960_16200K,
	XTALTAB0_960_16800K,
	XTALTAB0_960_19200K,
	XTALTAB0_960_19800K,
	XTALTAB0_960_20000K,
	XTALTAB0_960_24000K,
	XTALTAB0_960_25000K,
	XTALTAB0_960_26000K,
	XTALTAB0_960_30000K,
	XTALTAB0_960_33600K,
	XTALTAB0_960_37400K,
	XTALTAB0_960_38400K,
	XTALTAB0_960_40000K,
	XTALTAB0_960_48000K,
	XTALTAB0_960_52000K
};

/**
 * given an x-tal frequency, this table specifies the PLL params to use to generate a 960Mhz output
 * clock. This output clock feeds the clock divider network. The defines of the form
 * PMU1_XTALTAB0_960_* index into this array.
 */
static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_960)[] = {
/*	fref      xf     p1div   p2div  ndiv_int  ndiv_frac */
	{12000,   1,       1,      1,     0x50,   0x0     }, /* array index 0 */
	{13000,   2,       1,      1,     0x49,   0xD89D89},
	{14400,   3,       1,      1,     0x42,   0xAAAAAA},
	{15360,   4,       1,      1,     0x3E,   0x800000},
	{16200,   5,       1,      1,     0x3B,   0x425ED0},
	{16800,   6,       1,      1,     0x39,   0x249249},
	{19200,   7,       1,      1,     0x32,   0x0     },
	{19800,   8,       1,      1,     0x30,   0x7C1F07},
	{20000,   9,       1,      1,     0x30,   0x0     },
	{24000,   10,      1,      1,     0x28,   0x0     },
	{25000,   11,      1,      1,     0x26,   0x666666}, /* array index 10 */
	{26000,   12,      1,      1,     0x24,   0xEC4EC4},
	{30000,   13,      1,      1,     0x20,   0x0     },
	{33600,   14,      1,      1,     0x1C,   0x924924},
	{37400,   15,      2,      1,     0x33,   0x563EF9},
	{38400,   16,      2,      1,     0x32,   0x0	  },
	{40000,   17,      2,      1,     0x30,   0x0     },
	{48000,   18,      2,      1,     0x28,   0x0     },
	{52000,   19,      2,      1,     0x24,   0xEC4EC4}, /* array index 18 */
	{0,	      0,       0,      0,     0,      0	      }
};

static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_1600)[] = {
/*	fref      xf       p1div   p2div  ndiv_int  ndiv_frac */
	{40000,   0,       1,      1,     0x28,   0x0     },
	{0,	  0,       0,      0,     0,      0	  }
};

/** how to achieve 2880Mhz BBPLL output using different x-tal frequencies ('fref') */
static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_2880)[] = {
/*	fref      xf       p1div   p2div  ndiv_int  ndiv_frac */
	{54000,   0,       1,      1,     0x35,     0x5555A }, /**< 54 Mhz xtal */
	{0,	  0,       0,      0,     0,        0	    }
};

static const pmu1_xtaltab0_t BCMINITDATA(pmu1_xtaltab0_1938)[] = {
	/*	fref      xf         p1div        p2div    ndiv_int  ndiv_frac */
	{XTAL_FREQ_54MHZ,   0,       1,      1,     0x28,   0x0     },
	{0,	  0,       0,      0,     0,      0	  }
};
/* Indices into array pmu1_xtaltab0_960[]. Keep array and these defines synchronized. */
#define PMU1_XTALTAB0_960_12000K	0
#define PMU1_XTALTAB0_960_13000K	1
#define PMU1_XTALTAB0_960_14400K	2
#define PMU1_XTALTAB0_960_15360K	3
#define PMU1_XTALTAB0_960_16200K	4
#define PMU1_XTALTAB0_960_16800K	5
#define PMU1_XTALTAB0_960_19200K	6
#define PMU1_XTALTAB0_960_19800K	7
#define PMU1_XTALTAB0_960_20000K	8
#define PMU1_XTALTAB0_960_24000K	9
#define PMU1_XTALTAB0_960_25000K	10
#define PMU1_XTALTAB0_960_26000K	11
#define PMU1_XTALTAB0_960_30000K	12
#define PMU1_XTALTAB0_960_33600K	13
#define PMU1_XTALTAB0_960_37400K	14
#define PMU1_XTALTAB0_960_38400K	15
#define PMU1_XTALTAB0_960_40000K	16
#define PMU1_XTALTAB0_960_48000K	17
#define PMU1_XTALTAB0_960_52000K	18

#define PMU15_XTALTAB0_12000K	0
#define PMU15_XTALTAB0_20000K	1
#define PMU15_XTALTAB0_26000K	2
#define PMU15_XTALTAB0_37400K	3
#define PMU15_XTALTAB0_52000K	4
#define PMU15_XTALTAB0_END	5

/* For having the pllcontrol data (info)
 * The table with the values of the registers will have one - one mapping.
 */
typedef struct {
	uint16 	clock;	/**< x-tal frequency in [KHz] */
	uint8	mode;	/**< spur mode */
	uint8	xf;	/**< corresponds with xf bitfield in PMU control register */
} pllctrl_data_t;

/*  *****************************  tables for 4335a0 *********************** */


/**
 * PLL control register table giving info about the xtal supported for 4335.
 * There should be a one to one mapping between pmu1_pllctrl_tab_4335_968mhz[] and this table.
 */
static const pllctrl_data_t pmu1_xtaltab0_4335_drv[] = {
	{37400, 0, XTALTAB0_960_37400K},
	{40000, 0, XTALTAB0_960_40000K},
};


/**
 * PLL control register values(all registers) for the xtal supported for 4335.
 * There should be a one to one mapping between pmu1_xtaltab0_4335_drv[] and this table.
 */
/*
 * This table corresponds to spur mode 8. This bbpll settings will be used for WLBGA Bx
 * as well as Cx
 */
static const uint32	pmu1_pllctrl_tab_4335_968mhz[] = {
/*      PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       PLL 5                */
	0x50800000, 0x0A060803, 0x0CB10806, 0x80E1E1E2, 0x02600004, 0x00019AB1,	/* 37400 KHz */
	0x50800000, 0x0A060803, 0x0C310806, 0x80333333, 0x02600004, 0x00017FFF,	/* 40000 KHz */
};

/** This table corresponds to spur mode 2. This bbpll settings will be used for WLCSP B0 */
static const uint32	pmu1_pllctrl_tab_4335_961mhz[] = {
/*      PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       PLL 5                */
	0x50800000, 0x0A060803, 0x0CB10806, 0x80B1F7C9, 0x02600004, 0x00019AB1,	/* 37400 KHz */
	0x50800000, 0x0A060803, 0x0C310806, 0x80066666, 0x02600004, 0x00017FFF,	/* 40000 KHz */
};

/*  ************************  tables for 4335a0 END *********************** */

/*  *****************************  tables for 43430 *********************** */
static const pllctrl_data_t BCMATTACHDATA(pmu1_xtaltab0_43430)[] = {
	{19200, 0, XTALTAB0_960_19200K},
	{26000, 0, XTALTAB0_960_26000K},
	{37400, 0, XTALTAB0_960_37400K}
};

static const uint32	pmu1_pllctrl_tab_43430_960mhz[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       */
	0x00000000, 0x00000000, 0x00001931, 0x00000000, 0x0C060A0A,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x00001231, 0x00EC5000, 0x0C060B0B,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x00000CB1, 0x00AB1F7C, 0x0C060A0A,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B
};

static const uint32	pmu1_pllctrl_tab_43430_972mhz[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       */
	0x00000000, 0x00000000, 0x00001931, 0x00A00000, 0x0C0C0C0C,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x000012B1, 0x00627627, 0x0C0C0C0C,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x00000CB1, 0x00FD4314, 0x0C0C0C0C,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B
};

static const uint32	pmu1_pllctrl_tab_43430_980mhz[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       */
	0x00000000, 0x00000000, 0x000019B1, 0x000AAAAB, 0x0C0C0C0B,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x000012B1, 0x00B13B14, 0x0C0C0C0B,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x00000D31, 0x0034057A, 0x0C0C0C0B,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B
};

static const uint32	pmu1_pllctrl_tab_43430_326p4mhz[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       */
	0x00000000, 0x00000000, 0x000008B1, 0x00000000, 0x04040404,
	0x00040104, 0x080004e2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x00000631, 0x008dc8dd, 0x04040404,
	0x00040104, 0x080004e2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x00000431, 0x00ba2e8c, 0x04040404,
	0x00040104, 0x080004e2, 0x00000002, 0x0000106B
};

static const uint32	pmu1_pllctrl_tab_43430_984mhz[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       */
	0x00000000, 0x00000000, 0x000019B1, 0x00400000, 0x0C0C0C0C,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x000012B1, 0x00D888FF, 0x0C0C0C0C,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B,
	0x00000000, 0x00000000, 0x00000DB1, 0x004EFFF0, 0x0C0C0C0C,
	0x000C030C, 0x080004E2, 0x00000002, 0x0000106B
};
/*  ************************  tables for 43430 END *********************** */

/*  *****************************  tables for 4345 *********************** */

/* PLL control register table giving info about the xtal supported for 4345 series */
static const pllctrl_data_t BCMATTACHDATA(pmu1_xtaltab0_4345)[] = {
	{12000, 0, XTALTAB0_960_12000K},
	{13000, 0, XTALTAB0_960_13000K},
	{14400, 0, XTALTAB0_960_14400K},
	{15360, 0, XTALTAB0_960_15360K},
	{16200, 0, XTALTAB0_960_16200K},
	{16800, 0, XTALTAB0_960_16800K},
	{19200, 0, XTALTAB0_960_19200K},
	{19800, 0, XTALTAB0_960_19800K},
	{20000, 0, XTALTAB0_960_20000K},
	{24000, 0, XTALTAB0_960_24000K},
	{25000, 0, XTALTAB0_960_25000K},
	{26000, 0, XTALTAB0_960_26000K},
	{30000, 0, XTALTAB0_960_30000K},
	{33600, 0, XTALTAB0_960_33600K},
	{37400, 0, XTALTAB0_960_37400K},
	{38400, 0, XTALTAB0_960_38400K},
	{40000, 0, XTALTAB0_960_40000K},
	{48000, 0, XTALTAB0_960_48000K},
	{52000, 0, XTALTAB0_960_52000K},
};

/*  ************************  tables for 4345 END *********************** */

/*  *****************************  tables for 43242a0 *********************** */
/**
 * PLL control register table giving info about the xtal supported for 43242
 * There should be a one to one mapping between pmu1_pllctrl_tab_43242A0[]
 * and pmu1_pllctrl_tab_43242A1[] and this table.
 */
static const pllctrl_data_t BCMATTACHDATA(pmu1_xtaltab0_43242)[] = {
	{37400, 0, XTALTAB0_960_37400K},
};

/*  ************************  tables for 4324a02 END *********************** */

/*  *****************************  tables for 4350a0 *********************** */

#define XTAL_DEFAULT_4350	37400
/**
 * PLL control register table giving info about the xtal supported for 4350
 * There should be a one to one mapping between pmu1_pllctrl_tab_4350_963mhz[] and this table.
 */
static const pllctrl_data_t pmu1_xtaltab0_4350[] = {
/*       clock  mode xf */
	{37400, 0,   XTALTAB0_960_37400K},
	{40000, 0,   XTALTAB0_960_40000K},
};

/**
 * PLL control register table giving info about the xtal supported for 4335.
 * There should be a one to one mapping between pmu1_pllctrl_tab_4335_963mhz[] and this table.
 */
static const uint32	BCMATTACHDATA(pmu1_pllctrl_tab_4350_963mhz)[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       PLL 5       PLL6         */
	0x50800000, 0x18060603, 0x0cb10814, 0x80bfaa00, 0x02600004, 0x00019AB1, 0x04a6c181,
	0x50800000, 0x18060603, 0x0C310814, 0x00133333, 0x02600004, 0x00017FFF, 0x04a6c181
};
static const uint32	BCMATTACHDATA(pmu1_pllctrl_tab_4350C0_963mhz)[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       PLL 5       PLL6         */
	0x50800000, 0x08060603, 0x0cb10804, 0xe2bfaa00, 0x02600004, 0x00019AB1, 0x02a6c181,
	0x50800000, 0x08060603, 0x0C310804, 0xe2133333, 0x02600004, 0x00017FFF, 0x02a6c181
};
/*  ************************  tables for 4350a0 END *********************** */

/*  *****************************  tables for 4349a0 *********************** */


#define XTAL_DEFAULT_4349	37400
/**
 * PLL control register table giving info about the xtal supported for 4349
 * There should be a one to one mapping between pmu1_pllctrl_tab_4349_640mhz[] and this table.
 */
static const pllctrl_data_t pmu1_xtaltab0_4349[] = {
/*       clock  mode xf */
	{37400, 0,   XTALTAB0_640_37400K}
};

/**
 * PLL control register table giving info about the xtal supported for 4349.
 * There should be a one to one mapping between pmu1_pllctrl_tab_4349_640mhz[] and this table.
 * PLL control5 register is related to HSIC which is not supported in 4349
 */
static const uint32	pmu1_pllctrl_tab_4349_640mhz[] = {
/* Default values for unused registers 5-7 as sw loop execution will go for 8 times */
/* Fvco is taken as 640.1 instead of 640 as per recommendation from chip team */
/*	PLL 0       PLL 1       PLL 2       PLL 3     PLL 4       PLL 5       PLL 6        PLL 7 */
	0x20800000, 0x04020402, 0x8B10608, 0xE11CBFAD,
	0x01600004, 0x00000000, 0x00000000, 0x00000000
};
/*  ************************  tables for 4349a0 END *********************** */
/*  *****************************  tables for 4373a0 *********************** */

#define XTAL_DEFAULT_4373	37400
/**
 * PLL control register table giving info about the xtal supported for 4373
 * There should be a one to one mapping between pmu1_pllctrl_tab_4373_960mhz[] and this table.
 */
static const pllctrl_data_t pmu1_xtaltab0_4373[] = {
	/* clock		mode xf */
	{XTAL_DEFAULT_4373, 0,   XTALTAB0_960_37400K}
};

/**
 * PLL control register table giving info about the xtal supported for 4373.
 * There should be a one to one mapping between pmu1_pllctrl_tab_4373_960mhz[] and this table.
 * PLL control5 register is related to HSIC which is not supported in 4373
 */
/* Default values for PLL registers as sw loop execution will go for 6 times */
/* Fvco is taken as 960.0 instead of 640 */
static const uint32	pmu1_pllctrl_tab_4373_960mhz[] = {
/*	PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       PLL 5        */
	0x20800000, 0x08040603, 0x0CB1080C, 0xE2B8D016, 0x02680004, 0x00000000
};
/*  ************************  tables for 4373a0 END *********************** */
/*  *****************************  tables for 43012a0 *********************** */

/**
 * PLL control register table giving info about the xtal supported for 43012
 * There should be a one to one mapping between pmu1_pllctrl_tab_43012_960mhz[] and this table.
 */
static const pllctrl_data_t(pmu1_xtaltab0_43012)[] = {
/*       clock  mode xf */
	{37400, 0,   XTALTAB0_960_37400K},
	{37400,	100, XTALTAB0_960_37400K},
};

/*
There should be a one to one mapping between pmu1_pllctrl_tab_43012_640mhz[]
* and this table. PLL control5 register is related to HSIC which is not supported in 43012
* Use a safe DCO code=56 by default, Across PVT openloop VCO Max=320MHz, Min=100
* Mhz
*/
static const uint32 (pmu1_pllctrl_tab_43012_1600mhz)[] = {
/* Fvco is taken as 160.1 */
/*	 PLL 0	     PLL 1	 PLL 2	     PLL 3	 PLL 4	     PLL 5		  */
	0x07df2411, 0x00800000, 0x00000000, 0x038051e8, 0x00000000, 0x00000000,
	0x0e5fd422, 0x00800000,	0x00000000, 0x000011e8,	0x00000000, 0x00000000
};
/*  ************************  tables for 43012a0 END *********************** */


/*  *****************************  tables for 4364a0 *********************** */

/**
 * PLL control register table giving info about the xtal supported for 4364
 * There should be a one to one mapping between pmu1_pllctrl_tab_4364_960mhz[] and this table.
 */
static const pllctrl_data_t BCMATTACHDATA(pmu1_xtaltab0_4364)[] = {
/*       clock  mode xf */
	{37400, 0,   XTALTAB0_960_37400K}
};

/**
 * PLL control register table giving info about the xtal supported for 4349.
 * There should be a one to one mapping between pmu1_pllctrl_tab_4349_640mhz[] and this table.
 * PLL control5 register is related to HSIC which is not supported in 4349
 */
static const uint32	BCMATTACHDATA(pmu1_pllctrl_tab_4364_960mhz)[] = {
/* Default values for unused registers 5-7 as sw loop execution will go for 8 times */
/* Fvco is taken as 960 */
/* 1x1 MAC Clock Frequency configured to 120MHz */
/*	PLL 0       PLL 1       PLL 2       PLL 3     PLL 4       PLL 5       PLL 6        PLL 7 */
#if defined(MAC_FREQ_160MHZ_4364_ENABLE)
	0x20840000, 0x06030603, 0x0CB10810, 0xE2AB1F7D,
#else
	0x20840000, 0x06030803, 0x0CB10810, 0xE2AB1F7D,
#endif /* MAC_FREQ_160MHZ_4364_ENABLE */
	0x02680004, 0x00000000, 0x00000000, 0x00000000
};
/*  ************************  tables for 4364a0 END *********************** */

/* PLL control register values(all registers) for the xtal supported for 43242.
 * There should be a one to one mapping for "pllctrl_data_t" and the table below.
 * only support 37.4M
 */
static const uint32	BCMATTACHDATA(pmu1_pllctrl_tab_43242A0)[] = {
/*      PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       PLL 5                */
	0xA7400040, 0x10080A06, 0x0CB11408, 0x80AB1F7C, 0x02600004, 0x00A6C4D3, /* 37400 */
};

static const uint32	BCMATTACHDATA(pmu1_pllctrl_tab_43242A1)[] = {
/*      PLL 0       PLL 1       PLL 2       PLL 3       PLL 4       PLL 5                */
	0xA7400040, 0x10080A06, 0x0CB11408, 0x80AB1F7C, 0x02600004, 0x00A6C191, /* 37400 */
};

/*  *****************************  tables for 4347a0 *********************** */
/**
 * PLL control register table giving info about the xtal supported for 4347
 * There should be a one to one mapping between pmu1_pllctrl_tab_4347_960mhz[] and this table.
 */
static const pllctrl_data_t BCMATTACHDATA(pmu1_xtaltab0_4347)[] = {
/*       clock  mode xf */
	{37400, 0,   XTALTAB0_960_37400K}
};

/**
 * PLL control register table giving info about the xtal supported for 4347.
 * There should be a one to one mapping between pmu1_pllctrl_tab_4347_640mhz[] and this table.
 * PLL control5 register is related to HSIC which is not supported in 4347
 */
/**
 * TBD : will update once all the control data is provided by system team
 * freq table : pll1 : fvco 963M, pll2 for arm : 385 MHz
 */
static const uint32	BCMATTACHDATA(pmu1_pllctrl_tab_4347_961mhz)[] = {
/* Default values for unused registers 4-7 as sw loop execution will go for 8 times */
/* Fvco is taken as 961M */
/*	PLL 0  PLL 1   PLL 2   PLL 3   PLL 4   PLL 5  PLL 6  PLL 7   PLL 8   PLL 9   PLL 10 */
	0x32800000, 0x06060603, 0x0191080C, 0x006B1F7C,
	0x00000800, 0x32800000, 0x2D2D20A5, 0x40800000,
	0x00000000, 0x00000000, 0x00000000
};
/*  ************************  tables for 4347a0 END *********************** */

/* 4334/4314 ADFLL freq target params */
typedef struct {
	uint16  fref;		/* x-tal frequency in [hz] */
	uint8   xf;			/* x-tal index as given by PMU programmers guide */
	uint32  freq_tgt;	/* freq_target: N_divide_ratio bitfield in DFLL */
} pmu2_xtaltab0_t;

/**
 * If a DFLL clock of 480Mhz is desired, use this table to determine xf and freq_tgt for
 * a given x-tal frequency.
 */
static const pmu2_xtaltab0_t BCMINITDATA(pmu2_xtaltab0_adfll_480)[] = {
	{12000,		1,	0x4FFFC},
	{20000,		9,	0x2FFFD},
	{26000,		11,	0x24EC3},
	{37400,		13,	0x19AB1},
	{52000,		17,	0x12761},
	{0,		0,	0},
};

static const pmu2_xtaltab0_t BCMINITDATA(pmu2_xtaltab0_adfll_492)[] = {
	{12000,		1,	0x51FFC},
	{20000,		9,	0x31330},
	{26000,		11,	0x25D88},
	{37400,		13,	0x1A4F5},
	{52000,		17,	0x12EC4},
	{0,		0,	0}
};

static const pmu2_xtaltab0_t BCMINITDATA(pmu2_xtaltab0_adfll_485)[] = {
	{12000,		1,	0x50D52},
	{20000,		9,	0x307FE},
	{26000,		11,	0x254EA},
	{37400,		13,	0x19EF8},
	{52000,		17,	0x12A75},
	{0,		0,	0}
};

/*  *****************************  tables for 53573 *********************** */

#define XTAL_DEFAULT_53573    40000
/**
 * PLL control register table giving info about the xtal supported for 53573
 * There should be a one to one mapping between pmu1_pllctrl_tab_53573_640mhz[] and this table.
 */
static const pllctrl_data_t pmu1_xtaltab0_53573[] = {
/*       clock  mode xf */
	{40000, 0,   XTALTAB0_640_40000K}
};

/**
 * PLL control register table giving info about the xtal supported for 4349.
 * There should be a one to one mapping between pmu1_pllctrl_tab_4349_640mhz[] and this table.
 * PLL control5 register is related to HSIC which is not supported in 4349
 */
static const uint32	pmu1_pllctrl_tab_53573_640mhz[] = {
/* Default values for unused registers 5-7 as sw loop execution will go for 8 times */
/* Fvco is taken as 640.1 instead of 640 as per recommendation from chip team */
/*	PLL 0       PLL 1       PLL 2       PLL 3     PLL 4       PLL 5       PLL 6        PLL 7 */
	0x20800000, 0x04020402, 0x8B10608, 0xE1000000,
	0x01600004, 0x00000000, 0x00000000, 0x00000000
};
/*  ************************  tables for 53573 END *********************** */

/** returns a table that instructs how to program the BBPLL for a particular xtal frequency */
static const pmu1_xtaltab0_t *
BCMINITFN(si_pmu1_xtaltab0)(si_t *sih)
{
	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM43239_CHIP_ID:
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM43526_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
	CASE_BCM43602_CHIP:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM43012_CHIP_ID:
	case BCM4364_CHIP_ID:
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		return pmu1_xtaltab0_960;
	case BCM4349_CHIP_GRPID:
	case BCM53573_CHIP_GRPID:
		if (CHIPID(sih->chip) == BCM4373_CHIP_ID)
			return pmu1_xtaltab0_960;
		else
			return pmu1_xtaltab0_640;

	case BCM4330_CHIP_ID:
		if (CST4330_CHIPMODE_SDIOD(sih->chipst))
			return pmu1_xtaltab0_960;
		else
			return pmu1_xtaltab0_1440;
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		return pmu1_xtaltab0_1600;
	case BCM7271_CHIP_ID:
		return pmu1_xtaltab0_1938;
	default:
		PMU_MSG(("si_pmu1_xtaltab0: Unknown chipid %s\n", bcm_chipname(sih->chip, chn, 8)));
		break;
	}
	ASSERT(0);
	return NULL;
} /* si_pmu1_xtaltab0 */

/** returns chip specific PLL settings for default xtal frequency and VCO output frequency */
static const pmu1_xtaltab0_t *
BCMINITFN(si_pmu1_xtaldef0)(si_t *sih)
{

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM43239_CHIP_ID:
		/* Default to 26000Khz */
		return &pmu1_xtaltab0_960[PMU1_XTALTAB0_960_26000K];
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4360_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM43526_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM43012_CHIP_ID:
		/* Default to 37400Khz */
		return &pmu1_xtaltab0_960[PMU1_XTALTAB0_960_37400K];
	CASE_BCM43602_CHIP:
		return &pmu1_xtaltab0_960[PMU1_XTALTAB0_960_40000K];

	case BCM4349_CHIP_GRPID:
		if (CHIPID(sih->chip) == BCM4373_CHIP_ID)
			return &pmu1_xtaltab0_960[PMU1_XTALTAB0_960_37400K];
		else
			return &pmu1_xtaltab0_640[PMU1_XTALTAB0_640_37400K];
	case BCM53573_CHIP_GRPID:
		return &pmu1_xtaltab0_640[PMU1_XTALTAB0_640_40000K];

	case BCM4330_CHIP_ID:
		/* Default to 37400Khz */
		if (CST4330_CHIPMODE_SDIOD(sih->chipst))
			return &pmu1_xtaltab0_960[PMU1_XTALTAB0_960_37400K];
		else
			return &pmu1_xtaltab0_1440[PMU1_XTALTAB0_1440_37400K];
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		return &pmu1_xtaltab0_1600[0];
	case BCM7271_CHIP_ID:
		return &pmu1_xtaltab0_1938[0];
	case BCM4364_CHIP_ID:
		return &pmu1_xtaltab0_960[PMU1_XTALTAB0_960_37400K];
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		return &pmu1_xtaltab0_960[PMU1_XTALTAB0_960_37400K];
	default:
		PMU_MSG(("si_pmu1_xtaldef0: Unknown chipid %s\n", bcm_chipname(sih->chip, chn, 8)));
		break;
	}
	ASSERT(0);
	return NULL;
} /* si_pmu1_xtaldef0 */

/**
 * store the val on init, then if func is called during normal operation
 * don't touch core regs anymore
 */
static uint32 si_pmu_pll1_fvco_4360(si_t *sih, osl_t *osh)
{
	static uint32 fvco_4360 = 0;
	uint32 xf, ndiv_int, ndiv_frac, fvco, pll_reg, p1_div_scale;
	uint32 r_high, r_low, int_part, frac_part, rounding_const;
	uint8 p1_div;
	uint origidx = 0, intr_val = 0;

	if (fvco_4360) {
		printf("%s:attempt to query fvco during normal operation\n",
			__FUNCTION__);
		/* this will insure that the func is called only once upon init */
		return fvco_4360;
	}

	/* Remember original core before switch to chipc */
	si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);

	xf = si_pmu_alp_clock(sih, osh)/1000;

	/* pll reg 10 , p1div, ndif_mode, ndiv_int */
	pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG10, 0, 0);
	p1_div = pll_reg & 0xf;
	ndiv_int = (pll_reg >> 7)  & 0x1f;

	/* pllctrl11 */
	pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG11, 0, 0);
	ndiv_frac = pll_reg & 0xfffff;

	int_part = xf * ndiv_int;

	rounding_const = 1 << (BBPLL_NDIV_FRAC_BITS - 1);
	bcm_uint64_multiple_add(&r_high, &r_low, ndiv_frac, xf, rounding_const);
	bcm_uint64_right_shift(&frac_part, r_high, r_low, BBPLL_NDIV_FRAC_BITS);

	if (!p1_div) {
		PMU_ERROR(("p1_div calc returned 0! [%d]\n", __LINE__));
		ROMMABLE_ASSERT(0);
	}

	if (p1_div == 0) {
		ASSERT(p1_div != 0);
		p1_div_scale = 0;
	} else

	p1_div_scale = (1 << P1_DIV_SCALE_BITS) / p1_div;
	rounding_const = 1 << (P1_DIV_SCALE_BITS - 1);

	bcm_uint64_multiple_add(&r_high, &r_low, (int_part + frac_part),
		p1_div_scale, rounding_const);
	bcm_uint64_right_shift(&fvco, r_high, r_low, P1_DIV_SCALE_BITS);

	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);

	fvco_4360 = fvco;
	return fvco;
} /* si_pmu_pll1_fvco_4360 */

/**
 * Specific to 43012 and calculate the FVCO frequency from XTAL freq
 *  Returns the FCVO frequency in [khz] units
 */
static uint32 si_pmu_pll1_fvco_43012(si_t *sih, osl_t *osh)
{
	uint32 xf, ndiv_int, ndiv_frac, fvco, pll_reg, p1_div_scale;
	uint32 r_high, r_low, int_part, frac_part, rounding_const;
	uint8 p_div;
	chipcregs_t *cc;
	uint origidx = 0, intr_val = 0;

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);
	BCM_REFERENCE(cc);

	xf = si_pmu_alp_clock(sih, osh)/1000;

	pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, 0, 0);

	ndiv_int = (pll_reg & PMU43012_PLL0_PC0_NDIV_INT_MASK) >>
			PMU43012_PLL0_PC0_NDIV_INT_SHIFT;

	ndiv_frac = (pll_reg & PMU43012_PLL0_PC0_NDIV_FRAC_MASK) >>
			PMU43012_PLL0_PC0_NDIV_FRAC_SHIFT;

	pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0);

	p_div = (pll_reg & PMU43012_PLL0_PC3_PDIV_MASK) >>
			PMU43012_PLL0_PC3_PDIV_SHIFT;

	/* If the p_div value read from PLL control register is zero,
	* then return default FVCO value instead of computing the FVCO frequency
	* using XTAL frequency
	*/
	if (!p_div) {
		PMU_ERROR(("pll control register read failed [%d]\n", __LINE__));
		ROMMABLE_ASSERT(0);
		fvco = 0;
		goto done;
	}
	/* Actual expression is as below */
	/* fvco1 = ((xf * (1/p1_div)) * (ndiv_int + (ndiv_frac /(1 << 20)))); */

	int_part = xf * ndiv_int;
	rounding_const = 1 << (PMU43012_PLL_NDIV_FRAC_BITS - 1);
	bcm_uint64_multiple_add(&r_high, &r_low, ndiv_frac, xf, rounding_const);
	bcm_uint64_right_shift(&frac_part, r_high, r_low, PMU43012_PLL_NDIV_FRAC_BITS);

	p1_div_scale = (1 << PMU43012_PLL_P_DIV_SCALE_BITS) / p_div;
	rounding_const = 1 << (PMU43012_PLL_P_DIV_SCALE_BITS - 1);

	bcm_uint64_multiple_add(&r_high, &r_low, (int_part + frac_part),
		p1_div_scale, rounding_const);
	bcm_uint64_right_shift(&fvco, r_high, r_low, PMU43012_PLL_P_DIV_SCALE_BITS);

done:
	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);
	return fvco;
} /* si_pmu_pll1_fvco_43012 */

/** returns chip specific default BaseBand pll fvco frequency in [khz] units */
static uint32
BCMINITFN(si_pmu1_pllfvco0)(si_t *sih)
{

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM43239_CHIP_ID:
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM43526_CHIP_ID:
		return FVCO_960;

	CASE_BCM43602_CHIP:
		return FVCO_960;
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		return FVCO_1920;	/* For BB PLL */
	case BCM7271_CHIP_ID:
		return FVCO_1938;
	case BCM43909_CHIP_ID:
	case BCM4345_CHIP_ID:
		return (CHIPREV(sih->chiprev) == 6) ? FVCO_961 : FVCO_963;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM4358_CHIP_ID:
		return FVCO_963;
	case BCM43566_CHIP_ID:
	case BCM43567_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
		return FVCO_960010;
	case BCM4335_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM53573_CHIP_GRPID:
	{
		osl_t *osh;

		osh = si_osh(sih);
		return (si_pmu_cal_fvco(sih, osh));
	}
	case BCM4330_CHIP_ID:
		if (CST4330_CHIPMODE_SDIOD(sih->chipst))
			return FVCO_960;
		else
			return FVCO_1440;
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	{
		osl_t *osh;
		osh = si_osh(sih);
		return si_pmu_pll1_fvco_4360(sih, osh);
	}
	case BCM43012_CHIP_ID:
	{
		osl_t *osh;
		osh = si_osh(sih);
		return si_pmu_pll1_fvco_43012(sih, osh);
	}
	case BCM4347_CHIP_GRPID:
		return FVCO_961;
	case BCM4369_CHIP_ID:
		return FVCO_963;
	case BCM4364_CHIP_ID:
		return FVCO_960;
	default:
		PMU_MSG(("si_pmu1_pllfvco0: Unknown chipid %s\n", bcm_chipname(sih->chip, chn, 8)));
		break;
	}
	ASSERT(0);
	return 0;
} /* si_pmu1_pllfvco0 */

/** returns chip specific default CPU pll vco frequency in [khz] units */
static uint32
BCMINITFN(si_pmu1_pllfvco1)(si_t *sih)
{
	switch (CHIPID(sih->chip)) {
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		return FVCO_1600;
	case BCM7271_CHIP_ID:
		return FVCO_1938;
	default:
		PMU_MSG(("si_pmu1_pllfvco1: Unknown chipid %s\n", bcm_chipname(sih->chip, chn, 8)));
		break;
	}
	ASSERT(0);
	return 0;
}

/**
 * returns chip specific default pll fvco frequency in [khz] units
 * for second pll for arm clock in 4347 - 420 MHz
 */
static uint32
BCMINITFN(si_pmu1_pllfvco0_pll2)(si_t *sih)
{

	switch (CHIPID(sih->chip)) {
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		return FVCO_385;
	default:
		PMU_MSG(("%s : Unknown chipid %s\n",
				__FUNCTION__, bcm_chipname(sih->chip, chn, 8)));
		ASSERT(0);
		break;
	}
	return 0;
} /* si_pmu1_pllfvco0_pll2 */

/** query alp/xtal clock frequency */
static uint32
BCMINITFN(si_pmu1_alpclk0)(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	const pmu1_xtaltab0_t *xt;
	uint32 xf;
	BCM_REFERENCE(sih);

	/* Find the frequency in the table */
	xf = (R_REG(osh, &pmu->pmucontrol) & PCTL_XTALFREQ_MASK) >>
	        PCTL_XTALFREQ_SHIFT;
	for (xt = si_pmu1_xtaltab0(sih); xt != NULL && xt->fref != 0; xt ++)
		if (xt->xf == xf)
			break;
	/* Could not find it so assign a default value */
	if (xt == NULL || xt->fref == 0)
		xt = si_pmu1_xtaldef0(sih);
	ASSERT(xt != NULL && xt->fref != 0);

	return xt->fref * 1000;
}

/**
 * Before the PLL is switched off, the HT clocks need to be deactivated, and reactivated
 * when the PLL is switched on again.
 * This function returns the chip specific HT clock resources (HT and MACPHY clocks).
 */
static uint32
si_pmu_htclk_mask(si_t *sih)
{
	/* chip specific bit position of various resources */
	rsc_per_chip_t *rsc = si_pmu_get_rsc_positions(sih);

	uint32 ht_req = (PMURES_BIT(rsc->ht_avail) | PMURES_BIT(rsc->macphy_clkavail));

	switch (CHIPID(sih->chip))
	{
		case BCM4330_CHIP_ID:
			ht_req |= PMURES_BIT(RES4330_BBPLL_PWRSW_PU);
			break;
		case BCM43362_CHIP_ID:  /* Same HT_ vals as 4336 */
		case BCM4336_CHIP_ID:
			ht_req |= PMURES_BIT(RES4336_BBPLL_PWRSW_PU);
			break;
		case BCM4335_CHIP_ID:   /* Same HT_ vals as 4350 */
		case BCM4345_CHIP_ID:	/* Same HT_ vals as 4350 */
		CASE_BCM43602_CHIP:  /* Same HT_ vals as 4350 */
		case BCM4349_CHIP_GRPID:
		case BCM53573_CHIP_GRPID: /* ht start is not defined for 53573 */
		case BCM43018_CHIP_ID:
		case BCM43430_CHIP_ID:
		case BCM4365_CHIP_ID:
		case BCM4366_CHIP_ID:
		case BCM43664_CHIP_ID:
	        case BCM43666_CHIP_ID:
#ifdef BCM7271
		case BCM7271_CHIP_ID:
#endif /* BCM7271 */
		case BCM43012_CHIP_ID:
		case BCM43909_CHIP_ID:	/* Same HT_ vals as 4350 */
		case BCM4350_CHIP_ID:
		case BCM4354_CHIP_ID:
		case BCM43556_CHIP_ID:
		case BCM43558_CHIP_ID:
		case BCM43566_CHIP_ID:
		case BCM43568_CHIP_ID:
		case BCM43569_CHIP_ID:
		case BCM43570_CHIP_ID:
		case BCM4358_CHIP_ID:
			ht_req |= PMURES_BIT(rsc->ht_start);
			break;
		case BCM4364_CHIP_ID:
			ht_req |= PMURES_BIT(rsc->ht_start);
			ht_req |= PMURES_BIT(RES4364_3x3_MACPHY_CLKAVAIL);
			break;
		case BCM43143_CHIP_ID:
		case BCM43242_CHIP_ID:
			break;
		case BCM4347_CHIP_GRPID:
		case BCM4369_CHIP_ID:
			ht_req |= PMURES_BIT(rsc->ht_start);
			break;
		default:
			ASSERT(0);
			break;
	}

	return ht_req;
} /* si_pmu_htclk_mask */

void
si_pmu_minresmask_htavail_set(si_t *sih, osl_t *osh, bool set_clear)
{
	pmuregs_t *pmu;
	uint origidx;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if (!set_clear) {
		switch (CHIPID(sih->chip)) {
		case BCM4313_CHIP_ID:
			if ((R_REG(osh, &pmu->min_res_mask)) &
				(PMURES_BIT(RES4313_HT_AVAIL_RSRC)))
				AND_REG(osh, &pmu->min_res_mask,
					~(PMURES_BIT(RES4313_HT_AVAIL_RSRC)));
			break;
		default:
			break;
		}
	}
	/* Return to original core */
	si_setcoreidx(sih, origidx);
}

uint
si_pll_minresmask_reset(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;
	uint err = BCME_OK;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
		case BCM4313_CHIP_ID:
			/* write to min_res_mask 0x200d : clear min_rsrc_mask */
			AND_REG(osh, &pmu->min_res_mask,
				~(PMURES_BIT(RES4313_HT_AVAIL_RSRC)));
			OSL_DELAY(100);
			/* write to max_res_mask 0xBFFF: clear max_rsrc_mask */
			AND_REG(osh, &pmu->max_res_mask,
				~(PMURES_BIT(RES4313_HT_AVAIL_RSRC)));
			OSL_DELAY(100);
			/* write to max_res_mask 0xFFFF :set max_rsrc_mask */
			OR_REG(osh, &pmu->max_res_mask,
				(PMURES_BIT(RES4313_HT_AVAIL_RSRC)));
			break;
		default:
			PMU_ERROR(("%s: PLL reset not supported\n", __FUNCTION__));
			err = BCME_UNSUPPORTED;
			break;
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	return err;
}

/** returns ALP frequency in [Hz] */
uint32
BCMATTACHFN(si_pmu_def_alp_clock)(si_t *sih, osl_t *osh)
{
	uint32 clock = ALP_CLOCK;

	BCM_REFERENCE(osh);

	switch (CHIPID(sih->chip)) {
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM43430_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43012_CHIP_ID:
	case BCM43909_CHIP_ID:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM4364_CHIP_ID:
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		clock = 37400*1000;
		break;
	case BCM53573_CHIP_GRPID:
		clock = 40000*1000;
		break;
	CASE_BCM43602_CHIP:
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		clock = 40000 * 1000;
		break;
	case BCM7271_CHIP_ID:
		clock = XTAL_FREQ_54MHZ * 1000;
		break;
	}

	return clock;
}

/**
 * The BBPLL register set needs to be reprogrammed because the x-tal frequency is not known at
 * compile time, or a different spur mode is selected. This function writes appropriate values into
 * the BBPLL registers. It returns the 'xf', corresponding to the 'xf' bitfield in the PMU control
 * register.
 *     'xtal'             : xtal frequency in [KHz]
 *     'pllctrlreg_update': contains info on what entries to use in 'pllctrlreg_val' for the given
 *                          x-tal frequency and spur mode
 *     'pllctrlreg_val'   : contains a superset of the BBPLL values to write
 *
 * Note: if pmu is NULL, this function returns xf, without programming PLL registers.
 * This function is only called for pmu1_ type chips, perhaps we should rename it.
 */
static uint8
si_pmu_pllctrlreg_update(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal,
            uint8 spur_mode, const pllctrl_data_t *pllctrlreg_update, uint32 array_size,
            const uint32 *pllctrlreg_val)
{
	uint8 indx, reg_offset, xf = 0;
	uint8 pll_ctrlcnt = 0;

	ASSERT(pllctrlreg_update);

	if (PMUREV(sih->pmurev) >= 5) {
		pll_ctrlcnt = (sih->pmucaps & PCAP5_PC_MASK) >> PCAP5_PC_SHIFT;
	} else {
		pll_ctrlcnt = (sih->pmucaps & PCAP_PC_MASK) >> PCAP_PC_SHIFT;
	}

	/* Program the PLL control register if the xtal value matches with the table entry value */
	for (indx = 0; indx < array_size; indx++) {
		/* If the entry does not match the xtal and spur_mode just continue the loop */
		if (!((pllctrlreg_update[indx].clock == (uint16)xtal) &&
			(pllctrlreg_update[indx].mode == spur_mode)))
			continue;
		/*
		 * Don't program the PLL registers if register base is NULL.
		 * If NULL just return the xref.
		 */
		if (pmu) {
			for (reg_offset = 0; reg_offset < pll_ctrlcnt; reg_offset++) {
				si_pmu_pllcontrol(sih, reg_offset, ~0,
					pllctrlreg_val[indx*pll_ctrlcnt + reg_offset]);
			}
		}
		xf = pllctrlreg_update[indx].xf;
		break;
	}
	return xf;
} /* si_pmu_pllctrlreg_update */

#ifdef SRFAST
/** SRFAST specific function */
static void
BCMATTACHFN(si_pmu_internal_clk_calibration)(si_t *sih, uint32 xtal)
{
	switch (CHIPID(sih->chip)) {
		case BCM4345_CHIP_ID:
			/* HW4345-446: PMU4345: Add support for calibrated
			* internal clock oscillator
			*/
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, PMU1_PLL0_PC1_M4DIV_MASK,
				PMU1_PLL0_PC1_M4DIV_BY_60 << PMU1_PLL0_PC1_M4DIV_SHIFT);
			break;

		case BCM43430_CHIP_ID: {
			uint32 m5div_val = 0;
			switch (xtal) {
				case 37400:
					m5div_val = PMU1_PLL0_PC2_M5DIV_BY_60;
					break;
				case 26000:
					m5div_val = PMU1_PLL0_PC2_M5DIV_BY_42;
					break;
				case 19200:
					m5div_val = PMU1_PLL0_PC2_M5DIV_BY_31;
					break;
				default:
					PMU_ERROR(("%s: xtal %d not supported\n",
						__FUNCTION__, xtal));
					ASSERT(0);
					break;
			}

			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, PMU1_PLL0_PC2_M5DIV_MASK,
				m5div_val << PMU1_PLL0_PC2_M5DIV_SHIFT);
			break;
		}
		default:
			PMU_ERROR(("%s: chip 0x%x not supported\n",
				__FUNCTION__, CHIPID(sih->chip)));
			ASSERT(0);
			break;
	}

	/* Start SR calibration: enable SR ext clk */
	si_pmu_vreg_control(sih, PMU_VREG_6, VREG6_4350_SR_EXT_CLKEN_MASK,
		(1 << VREG6_4350_SR_EXT_CLKEN_SHIFT));
	OSL_DELAY(30); /* Wait 30us */
	/* Stop SR calibration: disable SR ext clk */
	si_pmu_vreg_control(sih, PMU_VREG_6, VREG6_4350_SR_EXT_CLKEN_MASK,
		(0 << VREG6_4350_SR_EXT_CLKEN_SHIFT));
}
#endif /* SRFAST */

static void
BCMATTACHFN(si_pmu_set_4345_pllcontrol_regs)(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal)
{
/* these defaults come from the recommeded values defined on the 4345 confluence PLL page */
/* Backplane/ARM CR4 clock controlled by m3div bits 23:16 of PLL_CONTROL1
 * 120Mhz : m3div = 0x8
 * 160Mhz : m3div = 0x6
 * 240Mhz : m3div = 0x4
 */
#define PLL_4345_CONTROL0_DEFAULT       0x50800060
#define PLL_4345_CONTROL1_DEFAULT       0x0C060803
#define PLL_4345_CONTROL2_DEFAULT       0x0CB10806
#define PLL_4345_CONTROL3_DEFAULT       0xE1BFA862
#define PLL_4345_CONTROL4_DEFAULT       0x02600004
#define PLL_4345_CONTROL5_DEFAULT       0x00019AB1
#define PLL_4345_CONTROL6_DEFAULT       0x005360C9
#define PLL_4345_CONTROL7_DEFAULT       0x000AB1F7

	uint32 PLL_control[8] = {
		PLL_4345_CONTROL0_DEFAULT, PLL_4345_CONTROL1_DEFAULT,
		PLL_4345_CONTROL2_DEFAULT, PLL_4345_CONTROL3_DEFAULT,
		PLL_4345_CONTROL4_DEFAULT, PLL_4345_CONTROL5_DEFAULT,
		PLL_4345_CONTROL6_DEFAULT, PLL_4345_CONTROL7_DEFAULT
	};
	uint32 fvco = si_pmu1_pllfvco0(sih);	/* in [khz] */
	uint32 ndiv_int;
	uint32 ndiv_frac;
	uint32 temp_high, temp_low;
	uint8 p1div;
	uint8 ndiv_mode;
	uint8 i;
	uint32 min_res_mask = 0, max_res_mask = 0, clk_ctl_st = 0;

	ASSERT(pmu != NULL);
	ASSERT(xtal <= 0xFFFFFFFF / 1000);

	/* force the HT off  */
	si_pmu_pll_off(sih, osh, pmu, &min_res_mask, &max_res_mask, &clk_ctl_st);

#ifdef SRFAST
	if (SR_FAST_ENAB()) {
		si_pmu_internal_clk_calibration(sih, xtal);
	}
#endif /* SRFAST */

	/* xtal and FVCO are in kHz.  xtal/p1div must be <= 50MHz */
	p1div = 1 + (uint8) ((xtal * 1000) / 50000000UL);
	ndiv_int = (fvco * p1div) / xtal;

	/* ndiv_frac = (uint32) (((uint64) (fvco * p1div - xtal * ndiv_int) * (1 << 24)) / xtal) */
	bcm_uint64_multiple_add(&temp_high, &temp_low, fvco * p1div - xtal * ndiv_int, 1 << 24, 0);
	bcm_uint64_divide(&ndiv_frac, temp_high, temp_low, xtal);

	ndiv_mode = (ndiv_frac == 0) ? 0 : 3;

	/* change PLL_control[2] and PLL_control[3] */
	PLL_control[2] = (PLL_4345_CONTROL2_DEFAULT & 0x0000FFFF) |
	                 (p1div << 16) | (ndiv_mode << 20) | (ndiv_int << 23);
	PLL_control[3] = (PLL_4345_CONTROL3_DEFAULT & 0xFF000000) | ndiv_frac;

	/* TODO - set PLL control field in PLL_control[3] & PLL_control[4] */

	/* HSIC DFLL freq_target N_divide_ratio = 4096 * FVCO / xtal */
	fvco = FVCO_960;	/* USB/HSIC FVCO is always 960 MHz, regardless of BB FVCO */
	PLL_control[5] = (PLL_4345_CONTROL5_DEFAULT & 0xFFF00000) |
	                 ((((uint32) fvco << 12) / xtal) & 0x000FFFFF);

	ndiv_int = (fvco * p1div) / xtal;

	/*
	 * ndiv_frac = (uint32) (((uint64) (fvco * p1div - xtal * ndiv_int) * (1 << 20)) /
	 *                       xtal)
	 */
	bcm_uint64_multiple_add(&temp_high, &temp_low, fvco * p1div - xtal * ndiv_int, 1 << 20, 0);
	bcm_uint64_divide(&ndiv_frac, temp_high, temp_low, xtal);

	/* change PLL_control[6] */
	PLL_control[6] = (PLL_4345_CONTROL6_DEFAULT & 0xFFFFE000) | p1div | (ndiv_int << 3);

	/* change PLL_control[7] */
	PLL_control[7] = ndiv_frac;

	/* write PLL Control Regs */
	PMU_MSG(("xtal    PLLCTRL0   PLLCTRL1   PLLCTRL2   PLLCTRL3"));
	PMU_MSG(("   PLLCTRL4   PLLCTRL5   PLLCTRL6   PLLCTRL7\n"));
	PMU_MSG(("%d ", xtal));
	for (i = 0; i < 8; i++) {
		PMU_MSG((" 0x%08X", PLL_control[i]));
		si_pmu_pllcontrol(sih, i, ~0, PLL_control[i]);
	}
	PMU_MSG(("\n"));

	/* Now toggle pllctlupdate so the pll sees the new values */
	si_pmu_pllupd(sih);

	/* Need to toggle PLL's dreset_i signal to ensure output clocks are aligned */
	si_pmu_chipcontrol(sih, PMU_CHIPCTL1, (1<<6), (0<<6));
	si_pmu_chipcontrol(sih, PMU_CHIPCTL1, (1<<6), (1<<6));
	si_pmu_chipcontrol(sih, PMU_CHIPCTL1, (1<<6), (0<<6));

	/* enable HT back on  */
	si_pmu_pll_on(sih, osh, pmu, min_res_mask, max_res_mask, clk_ctl_st);
} /* si_pmu_set_4345_pllcontrol_regs */

/**
 * Chip-specific overrides to PLLCONTROL registers during init. If certain conditions (dependent on
 * x-tal frequency and current ALP frequency) are met, an update of the PLL is required.
 *
 * This takes less precedence over OTP PLLCONTROL overrides.
 * If update_required=FALSE, it returns TRUE if a update is about to occur.
 * No write happens.
 *
 * Return value: TRUE if the BBPLL registers 'update' field should be written by the caller.
 *
 * This function is only called for pmu1_ type chips, perhaps we should rename it.
 */
bool
BCMATTACHFN(si_pmu_update_pllcontrol)(si_t *sih, osl_t *osh, uint32 xtal, bool update_required)
{
	pmuregs_t *pmu;
	uint origidx;
	bool write_en = FALSE;
	uint8 xf = 0;
	const pmu1_xtaltab0_t *xt;
	uint32 tmp;
	const pllctrl_data_t *pllctrlreg_update = NULL;
	uint32 array_size = 0;
	/* points at a set of PLL register values to write for a given x-tal frequency: */
	const uint32 *pllctrlreg_val = NULL;
	uint8 ndiv_mode = PMU1_PLL0_PC2_NDIV_MODE_MASH;
	uint32 xtalfreq = 0;
	uint8 spurmode;

	/* If there is OTP or NVRAM entry for xtalfreq, program the
	 * PLL control register even if it is default xtal.
	 */
	xtalfreq = getintvar(NULL, rstr_xtalfreq);
	/* CASE1 */
	if (xtalfreq) {
		write_en = TRUE;
		xtal = xtalfreq;
	} else {
		/* There is NO OTP value */
		if (xtal) {
			/* CASE2: If the xtal value was calculated, program the PLL control
			 * registers only if it is not default xtal value.
			 */
			if (xtal != (si_pmu_def_alp_clock(sih, osh)/1000))
				write_en = TRUE;
		} else {
			/* CASE3: If the xtal obtained is "0", ie., clock is not measured, then
			 * leave the PLL control register as it is but program the xf in
			 * pmucontrol register with the default xtal value.
			 */
			xtal = si_pmu_def_alp_clock(sih, osh)/1000;
		}
	}

	switch (CHIPID(sih->chip)) {
	case BCM43239_CHIP_ID:
		write_en = TRUE;
		break;

	case BCM4335_CHIP_ID:
		pllctrlreg_update = pmu1_xtaltab0_4335_drv;
		array_size = ARRAYSIZE(pmu1_xtaltab0_4335_drv);
		if (sih->chippkg == BCM4335_WLBGA_PKG_ID) {
			pllctrlreg_val = pmu1_pllctrl_tab_4335_968mhz;
		} else {
			if (CHIPREV(sih->chiprev) <= 1) {
				/* for 4335 Ax/Bx Chips */
				pllctrlreg_val = pmu1_pllctrl_tab_4335_961mhz;
			} else if (CHIPREV(sih->chiprev) == 2) {
				/* for 4335 Cx chips */
				pllctrlreg_val = pmu1_pllctrl_tab_4335_968mhz;
			}
		}

		/* If PMU1_PLL0_PC2_MxxDIV_MASKxx have to change,
		 * then set write_en to true.
		 */
		write_en = TRUE;
		break;

	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
		pllctrlreg_update = pmu1_xtaltab0_4345;
		array_size = ARRAYSIZE(pmu1_xtaltab0_4345);
		/* Note: no pllctrlreg_val table, because the PLL ctrl regs are calculated */

		/* If PMU1_PLL0_PC2_MxxDIV_MASKxx have to change,
		 * then set write_en to true.
		 */
		write_en = TRUE;
		break;

	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
		pllctrlreg_update = pmu1_xtaltab0_4350;
		array_size = ARRAYSIZE(pmu1_xtaltab0_4350);

		if ((CHIPID(sih->chip) == BCM43569_CHIP_ID) ||
			(CHIPID(sih->chip) == BCM43570_CHIP_ID) ||
			(CHIPREV(sih->chiprev) >= 3)) {
			pllctrlreg_val = pmu1_pllctrl_tab_4350C0_963mhz;
		} else {
			pllctrlreg_val = pmu1_pllctrl_tab_4350_963mhz;
		}

		/* If PMU1_PLL0_PC2_MxxDIV_MASKxx have to change,
		 * then set write_en to true.
		 */
#ifdef BCMUSB_NODISCONNECT
		write_en = FALSE;
#else
		write_en = TRUE;
#endif
		break;

	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
		pllctrlreg_update = pmu1_xtaltab0_43242;
		array_size = ARRAYSIZE(pmu1_xtaltab0_43242);
		if (CHIPREV(sih->chiprev) == 0) {
			pllctrlreg_val = pmu1_pllctrl_tab_43242A0;
		} else {
			pllctrlreg_val = pmu1_pllctrl_tab_43242A1;
		}
		write_en = TRUE;
		break;

	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
		pllctrlreg_update = pmu1_xtaltab0_43430;
		array_size = ARRAYSIZE(pmu1_xtaltab0_43430);

		spurmode = getintvar(NULL, "spurconfig") & 0xf;
		if (spurmode == 1)
			pllctrlreg_val = pmu1_pllctrl_tab_43430_980mhz;
		else if (spurmode == 2)
			pllctrlreg_val = pmu1_pllctrl_tab_43430_984mhz;
		else if (spurmode == 3)
			pllctrlreg_val = pmu1_pllctrl_tab_43430_326p4mhz;
		else
			pllctrlreg_val = pmu1_pllctrl_tab_43430_972mhz;

#ifdef SRFAST
		if (SR_FAST_ENAB()) {
			si_pmu_internal_clk_calibration(sih, xtal);
		}
#endif /* SRFAST */

		write_en = TRUE;
		break;

	case BCM53573_CHIP_GRPID:
		pllctrlreg_update = pmu1_xtaltab0_53573;
		array_size = ARRAYSIZE(pmu1_xtaltab0_53573);
		pllctrlreg_val = pmu1_pllctrl_tab_53573_640mhz;
		/* If PMU1_PLL0_PC2_MxxDIV_MASKxx have to change,
		 * then set write_en to true.
		 */
		 break;

	case BCM4349_CHIP_GRPID:
		if (CHIPID(sih->chip) == BCM4373_CHIP_ID) {
			pllctrlreg_update = pmu1_xtaltab0_4373;
			array_size = ARRAYSIZE(pmu1_xtaltab0_4373);
			pllctrlreg_val = pmu1_pllctrl_tab_4373_960mhz;
		} else {
			pllctrlreg_update = pmu1_xtaltab0_4349;
			array_size = ARRAYSIZE(pmu1_xtaltab0_4349);
			pllctrlreg_val = pmu1_pllctrl_tab_4349_640mhz;
			/* If PMU1_PLL0_PC2_MxxDIV_MASKxx have to change,
			 * then set write_en to true.
			 */
		}
		break;
	case BCM43012_CHIP_ID:
			pllctrlreg_update = pmu1_xtaltab0_43012;
			array_size = ARRAYSIZE(pmu1_xtaltab0_43012);
			pllctrlreg_val = pmu1_pllctrl_tab_43012_1600mhz;
			break;
	case BCM4364_CHIP_ID:
		pllctrlreg_update = pmu1_xtaltab0_4364;
		array_size = ARRAYSIZE(pmu1_xtaltab0_4364);
		pllctrlreg_val = pmu1_pllctrl_tab_4364_960mhz;
		break;
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		pllctrlreg_update = pmu1_xtaltab0_4347;
		array_size = ARRAYSIZE(pmu1_xtaltab0_4347);
		pllctrlreg_val = pmu1_pllctrl_tab_4347_961mhz;
		break;

	CASE_BCM43602_CHIP:
		/*
		 * XXX43602 has only 1 x-tal value, possibly insert case when an other BBPLL
		 * frequency than 960Mhz is required (e.g., for spur avoidance)
		 */
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
#ifdef BCM7271
	case BCM7271_CHIP_ID:
#endif /* BCM7271 */
		 /* fall through */
	default:
		/* write_en is FALSE in this case. So returns from the function */
		write_en = FALSE;
		break;
	}

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	/* Check if the table has PLL control register values for the requested
	 * xtal. NOTE THAT, THIS IS not DONE FOR 43239,
	 * AS IT HAS ONLY ONE XTAL SUPPORT.
	 */
	if (!update_required && pllctrlreg_update) {
		/* Here the chipcommon register base is passed as NULL, so that we just get
		 * the xf for the xtal being programmed but don't program the registers now
		 * as the PLL is not yet turned OFF.
		 */
		xf = si_pmu_pllctrlreg_update(sih, osh, NULL, xtal, 0, pllctrlreg_update,
			array_size, pllctrlreg_val);

		/* Program the PLL based on the xtal value. */
		if (xf != 0) {
			/* Write XtalFreq. Set the divisor also. */
			tmp = R_REG(osh, &pmu->pmucontrol) &
				~(PCTL_ILP_DIV_MASK | PCTL_XTALFREQ_MASK);
			tmp |= (((((xtal + 127) / 128) - 1) << PCTL_ILP_DIV_SHIFT) &
				PCTL_ILP_DIV_MASK) |
				((xf << PCTL_XTALFREQ_SHIFT) & PCTL_XTALFREQ_MASK);
			W_REG(osh, &pmu->pmucontrol, tmp);
		} else {
			write_en = FALSE;
			printf(rstr_Invalid_Unsupported_xtal_value_D, xtal);
		}
	}

	/* If its a check sequence or if there is nothing to write, return here */
	if ((update_required == FALSE) || (write_en == FALSE)) {
		goto exit;
	}

	/* Update the PLL control register based on the xtal used. */
	if (pllctrlreg_val) {
		si_pmu_pllctrlreg_update(sih, osh, pmu, xtal, 0, pllctrlreg_update, array_size,
			pllctrlreg_val);
	}

	/* Chip specific changes to PLL Control registers is done here. */
	switch (CHIPID(sih->chip)) {
	case BCM43239_CHIP_ID:
		/* 43239: Change backplane and dot11mac clock to 120Mhz */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2,
		  (PMU1_PLL0_PC2_M5DIV_MASK | PMU1_PLL0_PC2_M6DIV_MASK),
		  ((8 << PMU1_PLL0_PC2_M5DIV_SHIFT) | (8 << PMU1_PLL0_PC2_M6DIV_SHIFT)));
		/* To ensure the PLL control registers are not modified from what is default. */
		xtal = 0;
		break;
	case BCM4324_CHIP_ID:
		/* Change backplane clock (ARM input) to 137Mhz */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, PMU1_PLL0_PC1_M2DIV_MASK,
			(7 << PMU1_PLL0_PC1_M2DIV_SHIFT));
		break;
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
		/* Change backplane clock (ARM input) to 137Mhz */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, PMU1_PLL0_PC1_M2DIV_MASK,
			(7 << PMU1_PLL0_PC1_M2DIV_SHIFT));
		break;
	case BCM4345_CHIP_ID:
		si_pmu_set_4345_pllcontrol_regs(sih, osh, pmu, xtal);
		/* To ensure the PLL control registers are not modified from what is default. */
		xtal = 0;
		break;
	default:
		break;
	}

	/* Program the PLL based on the xtal value. */
	if (xtal != 0) {

		/* Find the frequency in the table */
		for (xt = si_pmu1_xtaltab0(sih); xt != NULL && xt->fref != 0; xt ++)
			if (xt->fref == xtal)
				break;

		/* Check current PLL state, bail out if it has been programmed or
		 * we don't know how to program it. But we might still have some programming
		 * like changing the ARM clock, etc. So cannot return from here.
		 */
		if (xt == NULL || xt->fref == 0)
			goto exit;

		/* If the PLL is already programmed exit from here. */
		if (((R_REG(osh, &pmu->pmucontrol) &
			PCTL_XTALFREQ_MASK) >> PCTL_XTALFREQ_SHIFT) == xt->xf)
			goto exit;

		PMU_MSG(("XTAL %d.%d MHz (%d)\n", xtal / 1000, xtal % 1000, xt->xf));
		PMU_MSG(("Programming PLL for %d.%d MHz\n", xt->fref / 1000, xt->fref % 1000));

		/* Write p1div and p2div to pllcontrol[0] */
		tmp = ((xt->p1div << PMU1_PLL0_PC0_P1DIV_SHIFT) & PMU1_PLL0_PC0_P1DIV_MASK) |
			((xt->p2div << PMU1_PLL0_PC0_P2DIV_SHIFT) & PMU1_PLL0_PC0_P2DIV_MASK);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0,
			(PMU1_PLL0_PC0_P1DIV_MASK | PMU1_PLL0_PC0_P2DIV_MASK), tmp);

		/* Write ndiv_int and ndiv_mode to pllcontrol[2] */
		tmp = ((xt->ndiv_int << PMU1_PLL0_PC2_NDIV_INT_SHIFT)
				& PMU1_PLL0_PC2_NDIV_INT_MASK) |
				((ndiv_mode << PMU1_PLL0_PC2_NDIV_MODE_SHIFT)
				& PMU1_PLL0_PC2_NDIV_MODE_MASK);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2,
			(PMU1_PLL0_PC2_NDIV_INT_MASK | PMU1_PLL0_PC2_NDIV_MODE_MASK), tmp);

		/* Write ndiv_frac to pllcontrol[3] */
		if (BCM4347_CHIP(sih->chip) ||
				(CHIPID(sih->chip) == BCM4369_CHIP_ID)) {
			tmp = ((xt->ndiv_frac << PMU4347_PLL0_PC3_NDIV_FRAC_SHIFT) &
				PMU4347_PLL0_PC3_NDIV_FRAC_MASK);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3,
				PMU4347_PLL0_PC3_NDIV_FRAC_MASK, tmp);
		} else
		{
			tmp = ((xt->ndiv_frac << PMU1_PLL0_PC3_NDIV_FRAC_SHIFT) &
				PMU1_PLL0_PC3_NDIV_FRAC_MASK);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3,
				PMU1_PLL0_PC3_NDIV_FRAC_MASK, tmp);
		}

		/* Write XtalFreq. Set the divisor also. */
		tmp = R_REG(osh, &pmu->pmucontrol) &
			~(PCTL_ILP_DIV_MASK | PCTL_XTALFREQ_MASK);
		tmp |= (((((xt->fref + 127) / 128) - 1) << PCTL_ILP_DIV_SHIFT) &
			PCTL_ILP_DIV_MASK) |
			((xt->xf << PCTL_XTALFREQ_SHIFT) & PCTL_XTALFREQ_MASK);
		W_REG(osh, &pmu->pmucontrol, tmp);
	}

exit:
	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return write_en;
} /* si_pmu_update_pllcontrol */

uint32
si_pmu_get_pmutimer(si_t *sih)
{
	osl_t *osh = si_osh(sih);
	pmuregs_t *pmu;
	uint origidx;
	uint32 start;
	BCM_REFERENCE(sih);

	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	start = R_REG(osh, &pmu->pmutimer);
	if (start != R_REG(osh, &pmu->pmutimer))
		start = R_REG(osh, &pmu->pmutimer);

	si_setcoreidx(sih, origidx);

	return (start);
}

/**
 * returns
 * a) diff between a 'prev' value of pmu timer and current value
 * b) the current pmutime value in 'prev'
 * 	So, 'prev' is an IO parameter.
 */
uint32
si_pmu_get_pmutime_diff(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 *prev)
{
	uint32 pmutime_diff = 0, pmutime_val = 0;
	uint32 prev_val = *prev;
	BCM_REFERENCE(osh);
	BCM_REFERENCE(pmu);
	/* read current value */
	pmutime_val = si_pmu_get_pmutimer(sih);
	/* diff btween prev and current value, take on wraparound case as well. */
	pmutime_diff = (pmutime_val >= prev_val) ?
		(pmutime_val - prev_val) :
		(~prev_val + pmutime_val + 1);
	*prev = pmutime_val;
	return pmutime_diff;
}

/**
 * wait for usec for the res_pending register to change.
 * NOTE: usec SHOULD be > 32uS
 * if cond = TRUE, res_pending will be read until it becomes == 0;
 * If cond = FALSE, res_pending will be read until it becomes != 0;
 * returns TRUE if timedout.
 * returns elapsed time in this loop in elapsed_time
 */
bool
si_pmu_wait_for_res_pending(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint usec,
	bool cond, uint32 *elapsed_time)
{
	/* add 32uSec more */
	uint countdown = usec;
	uint32 pmutime_prev = 0, pmutime_elapsed = 0, res_pend;
	bool pending = FALSE;

	/* store current time */
	pmutime_prev = si_pmu_get_pmutimer(sih);
	while (1) {
		res_pend = R_REG(osh, &pmu->res_pending);

		/* based on the condition, check */
		if (cond == TRUE) {
			if (res_pend == 0) break;
		} else {
			if (res_pend != 0) break;
		}

		/* if required time over */
		if ((pmutime_elapsed * PMU_US_STEPS) >= countdown) {
			/* timeout. so return as still pending */
			pending = TRUE;
			break;
		}

		/* get elapsed time after adding diff between prev and current
		* pmutimer value
		*/
		pmutime_elapsed += si_pmu_get_pmutime_diff(sih, osh, pmu, &pmutime_prev);
	}

	*elapsed_time = pmutime_elapsed * PMU_US_STEPS;
	return pending;
} /* si_pmu_wait_for_res_pending */

/**
 *	The algorithm for pending check is that,
 *	step1: 	wait till (res_pending !=0) OR pmu_max_trans_timeout.
 *			if max_trans_timeout, flag error and exit.
 *			wait for 1 ILP clk [64uS] based on pmu timer,
 *			polling to see if res_pending again goes high.
 *			if res_pending again goes high, go back to step1.
 *	Note: res_pending is checked repeatedly because, in between switching
 *	of dependent
 *	resources, res_pending resets to 0 for a short duration of time before
 *	it becomes 1 again.
 *	Note: return 0 is GOOD, 1 is BAD [mainly timeout].
 */
int si_pmu_wait_for_steady_state(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	int stat = 0;
	bool timedout = FALSE;
	uint32 elapsed = 0, pmutime_total_elapsed = 0;

	while (1) {
		/* wait until all resources are settled down [till res_pending becomes 0] */
		timedout = si_pmu_wait_for_res_pending(sih, osh, pmu,
			PMU_MAX_TRANSITION_DLY, TRUE, &elapsed);

		if (timedout) {
			stat = 1;
			break;
		}

		pmutime_total_elapsed += elapsed;
		/* wait to check if any resource comes back to non-zero indicating
		* that it pends again. The res_pending goes 0 for 1 ILP clock before
		* getting set for next resource in the sequence , so if res_pending
		* is 0 for more than 1 ILP clk it means nothing is pending
		* to indicate some pending dependency.
		*/
		timedout = si_pmu_wait_for_res_pending(sih, osh, pmu,
			64, FALSE, &elapsed);

		pmutime_total_elapsed += elapsed;
		/* Here, we can also check timedout, but we make sure that,
		* we read the res_pending again.
		*/
		if (timedout) {
			stat = 0;
			break;
		}

		/* Total wait time for all the waits above added should be
		* less than  PMU_MAX_TRANSITION_DLY
		*/
		if (pmutime_total_elapsed >= PMU_MAX_TRANSITION_DLY) {
			/* timeout. so return as still pending */
			stat = 1;
			break;
		}
	}
	return stat;
} /* si_pmu_wait_for_steady_state */


static uint32
si_pmu_pll_delay_43012(si_t *sih, uint32 delay_us, uint32 poll)
{
	uint32 delay = 0;

	/* In case of NIC builds, we can use OSL_DELAY() for 1 us delay. But in case of DONGLE
	 * builds, we can't rely on the OSL_DELAY() as it is internally relying on HT clock and
	 * we are calling this function when ALP clock is present.
	 */
	for (delay = 0; delay < delay_us; delay++) {
		if (poll == 1) {
			if (si_gci_chipstatus(sih, GCI_CHIPSTATUS_07) &
					GCI43012_CHIPSTATUS_07_BBPLL_LOCK_MASK) {
				goto exit;
			}
		}
		OSL_DELAY(1);
	}

	if (poll == 1) {
		PMU_ERROR(("%s: PLL not locked!", __FUNCTION__));
		ASSERT(0);
	}
exit:
	return delay;
}

static void
si_pmu_pll_on_43012(si_t *sih, osl_t *osh, pmuregs_t *pmu, bool openloop_cal)
{
	uint32 rsrc_ht, total_time = 0;

	si_pmu_chipcontrol(sih, PMU_CHIPCTL4, PMUCCTL04_43012_FORCE_BBPLL_PWROFF, 0);
	total_time += si_pmu_pll_delay_43012(sih, 2, 0);
	si_pmu_chipcontrol(sih, PMU_CHIPCTL4, PMUCCTL04_43012_FORCE_BBPLL_ISOONHIGH |
			PMUCCTL04_43012_FORCE_BBPLL_PWRDN, 0);
	total_time += si_pmu_pll_delay_43012(sih, 2, 0);
	si_pmu_chipcontrol(sih, PMU_CHIPCTL4, PMUCCTL04_43012_FORCE_BBPLL_ARESET, 0);

	rsrc_ht = R_REG(osh, &pmu->res_state) &
			((1 << RES43012_HT_AVAIL) | (1 << RES43012_HT_START));

	if (rsrc_ht)
	{
		/* Wait for PLL to lock in close-loop */
		total_time += si_pmu_pll_delay_43012(sih, 200, 1);
	}
	else {
		/* Wait for 1 us for the open-loop clock to start */
		total_time += si_pmu_pll_delay_43012(sih, 1, 0);
	}

	if (!openloop_cal) {
		/* Allow clk to be used if its not calibration */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL4, PMUCCTL04_43012_FORCE_BBPLL_DRESET, 0);
		total_time += si_pmu_pll_delay_43012(sih, 1, 0);
		si_pmu_chipcontrol(sih, PMU_CHIPCTL4, PMUCCTL04_43012_DISABLE_LQ_AVAIL, 0);
		si_pmu_chipcontrol(sih, PMU_CHIPCTL4, PMUCCTL04_43012_DISABLE_HT_AVAIL, 0);
	}

	PMU_MSG(("%s: time taken: %d us\n", __FUNCTION__, total_time));
}

static void
si_pmu_pll_off_43012(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	uint32 total_time = 0;
	BCM_REFERENCE(osh);
	BCM_REFERENCE(pmu);
	si_pmu_chipcontrol(sih, PMU_CHIPCTL4,
			PMUCCTL04_43012_DISABLE_LQ_AVAIL | PMUCCTL04_43012_DISABLE_HT_AVAIL,
			PMUCCTL04_43012_DISABLE_LQ_AVAIL | PMUCCTL04_43012_DISABLE_HT_AVAIL);
	total_time += si_pmu_pll_delay_43012(sih, 1, 0);

	si_pmu_chipcontrol(sih, PMU_CHIPCTL4,
			(PMUCCTL04_43012_FORCE_BBPLL_ARESET | PMUCCTL04_43012_FORCE_BBPLL_DRESET |
			PMUCCTL04_43012_FORCE_BBPLL_PWRDN |PMUCCTL04_43012_FORCE_BBPLL_ISOONHIGH),
			(PMUCCTL04_43012_FORCE_BBPLL_ARESET | PMUCCTL04_43012_FORCE_BBPLL_DRESET |
			PMUCCTL04_43012_FORCE_BBPLL_PWRDN |PMUCCTL04_43012_FORCE_BBPLL_ISOONHIGH));
	total_time += si_pmu_pll_delay_43012(sih, 1, 0);

	si_pmu_chipcontrol(sih, PMU_CHIPCTL4,
			PMUCCTL04_43012_FORCE_BBPLL_PWROFF,
			PMUCCTL04_43012_FORCE_BBPLL_PWROFF);

	PMU_MSG(("%s: time taken: %d us\n", __FUNCTION__, total_time));
}

/** Turn Off the PLL - Required before setting the PLL registers */
static void
si_pmu_pll_off(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 *min_mask,
	uint32 *max_mask, uint32 *clk_ctl_st)
{
	uint32 ht_req;

	/* Save the original register values */
	*min_mask = R_REG(osh, &pmu->min_res_mask);
	*max_mask = R_REG(osh, &pmu->max_res_mask);
	*clk_ctl_st = si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0);

	ht_req = si_pmu_htclk_mask(sih);
	if (ht_req == 0)
		return;

	if ((CHIPID(sih->chip) == BCM4335_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4345_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43430_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43018_CHIP_ID) ||
		BCM4365_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM7271_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43012_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43909_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4364_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4369_CHIP_ID) ||
		(BCM4347_CHIP(sih->chip)) ||
		BCM43602_CHIP(sih->chip) ||
		BCM4350_CHIP(sih->chip) ||
		(BCM4349_CHIP(sih->chip)) ||
		(BCM53573_CHIP(sih->chip)) ||
	0) {
		/* slightly different way for 4335, but this could be applied for other chips also.
		* If HT_AVAIL is not set, wait to see if any resources are availing HT.
		*/
		if (((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL) != CCS_HTAVAIL))
			si_pmu_wait_for_steady_state(sih, osh, pmu);
	} else {
		OR_REG(osh,  &pmu->max_res_mask, ht_req);
		/* wait for HT to be ready before taking the HT away...HT could be coming up... */
		SPINWAIT(((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL) != CCS_HTAVAIL), PMU_MAX_TRANSITION_DLY);
		ASSERT((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL));
	}

	if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		si_pmu_pll_off_43012(sih, osh, pmu);
	} else {
		AND_REG(osh, &pmu->min_res_mask, ~ht_req);
		AND_REG(osh, &pmu->max_res_mask, ~ht_req);

		SPINWAIT(((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL) == CCS_HTAVAIL), PMU_MAX_TRANSITION_DLY);
		ASSERT(!(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL));
		OSL_DELAY(100);
	}
} /* si_pmu_pll_off */

/* below function are for BBPLL parallel purpose */
/** Turn Off the PLL - Required before setting the PLL registers */
void
si_pmu_pll_off_PARR(si_t *sih, osl_t *osh, uint32 *min_mask,
uint32 *max_mask, uint32 *clk_ctl_st)
{
	pmuregs_t *pmu;
	uint origidx, intr_val;
	uint32 ht_req;

	/* Block ints and save current core */
	intr_val = si_introff(sih);
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	/* Save the original register values */
	*min_mask = R_REG(osh, &pmu->min_res_mask);
	*max_mask = R_REG(osh, &pmu->max_res_mask);
	*clk_ctl_st = si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0);
	ht_req = si_pmu_htclk_mask(sih);
	if (ht_req == 0) {
		/* Return to original core */
		si_setcoreidx(sih, origidx);
		si_intrrestore(sih, intr_val);
		return;
	}

	if ((CHIPID(sih->chip) == BCM4335_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4345_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43430_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43018_CHIP_ID) ||
		BCM4365_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM7271_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43012_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43909_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4364_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4369_CHIP_ID) ||
		(BCM4347_CHIP(sih->chip)) ||
		BCM43602_CHIP(sih->chip) ||
		BCM4350_CHIP(sih->chip) ||
		BCM4349_CHIP(sih->chip) ||
		(BCM53573_CHIP(sih->chip)) ||
	0) {
		/* slightly different way for 4335, but this could be applied for other chips also.
		* If HT_AVAIL is not set, wait to see if any resources are availing HT.
		*/
		if (((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL)
			!= CCS_HTAVAIL))
			si_pmu_wait_for_steady_state(sih, osh, pmu);
	} else {
		OR_REG(osh, &pmu->max_res_mask, ht_req);
		/* wait for HT to be ready before taking the HT away...HT could be coming up... */
		SPINWAIT(((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL) != CCS_HTAVAIL), PMU_MAX_TRANSITION_DLY);
		ASSERT((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL));
	}

	AND_REG(osh, &pmu->min_res_mask, ~ht_req);
	AND_REG(osh, &pmu->max_res_mask, ~ht_req);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	si_intrrestore(sih, intr_val);
} /* si_pmu_pll_off_PARR */


static void
si_pmu_pll_off_isdone(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	uint32 ht_req;

	ht_req = si_pmu_htclk_mask(sih);
	SPINWAIT(((R_REG(osh, &pmu->res_state) & ht_req) != 0),
	PMU_MAX_TRANSITION_DLY);
	SPINWAIT(((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
		& CCS_HTAVAIL) == CCS_HTAVAIL), PMU_MAX_TRANSITION_DLY);
	ASSERT(!(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
		& CCS_HTAVAIL));
}

/** Turn ON/restore the PLL based on the mask received */
static void
si_pmu_pll_on(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 min_mask_mask,
	uint32 max_mask_mask, uint32 clk_ctl_st_mask)
{
	uint32 ht_req;

	ht_req = si_pmu_htclk_mask(sih);
	if (ht_req == 0)
		return;

	max_mask_mask &= ht_req;
	min_mask_mask &= ht_req;

	if (max_mask_mask != 0)
		OR_REG(osh, &pmu->max_res_mask, max_mask_mask);

	if (min_mask_mask != 0)
		OR_REG(osh, &pmu->min_res_mask, min_mask_mask);

	if (clk_ctl_st_mask & CCS_HTAVAIL) {
		/* Wait for HT_AVAIL to come back */
		SPINWAIT(((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL) != CCS_HTAVAIL), PMU_MAX_TRANSITION_DLY);
		ASSERT((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
		& CCS_HTAVAIL));
	}

	if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		si_pmu_pll_on_43012(sih, osh, pmu, 0);
	}
}

/**
 * This function controls the PLL register update while
 * switching MAC Clock Frequency dynamically between 120MHz and 160MHz
 * in the case of 4364
 */
void
si_pmu_pll_4364_macfreq_switch(si_t *sih, osl_t *osh, uint8 mode)
{
	uint32 max_res_mask = 0, min_res_mask = 0, clk_ctl_st = 0;
	uint origidx = si_coreidx(sih);
	pmuregs_t *pmu = si_setcoreidx(sih, SI_CC_IDX);
	ASSERT(pmu != NULL);
	si_pmu_pll_off(sih, osh, pmu, &min_res_mask, &max_res_mask, &clk_ctl_st);
	if (mode == PMU1_PLL0_SWITCH_MACCLOCK_120MHZ) { /* 120MHz */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, PMU1_PLL0_PC1_M2DIV_MASK,
			(PMU1_PLL0_PC1_M2DIV_VALUE_120MHZ << PMU1_PLL0_PC1_M2DIV_SHIFT));
	} else if (mode == PMU1_PLL0_SWITCH_MACCLOCK_160MHZ) {  /* 160MHz */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, PMU1_PLL0_PC1_M2DIV_MASK,
			(PMU1_PLL0_PC1_M2DIV_VALUE_160MHZ << PMU1_PLL0_PC1_M2DIV_SHIFT));
	}
	W_REG(osh, &pmu->pmucontrol,
		R_REG(osh, &pmu->pmucontrol) | PCTL_PLL_PLLCTL_UPD);
	si_pmu_pll_on(sih, osh, pmu, min_res_mask, max_res_mask, clk_ctl_st);
	si_setcoreidx(sih, origidx);
}

/**
 * Caller wants to know the default values to program into BB PLL/FLL hardware for a specific chip.
 *
 * The relation between x-tal frequency, HT clock and divisor values to write into the PLL hardware
 * is given by a set of tables, one table per PLL/FLL frequency (480/485 Mhz).
 *
 * This function returns the table entry corresponding with the (chip specific) default x-tal
 * frequency.
 */
static const pmu2_xtaltab0_t *
BCMINITFN(si_pmu2_xtaldef0)(si_t *sih)
{
	switch (CHIPID(sih->chip)) {
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
		if (ISSIM_ENAB(sih))
			return &pmu2_xtaltab0_adfll_480[PMU15_XTALTAB0_20000K];
		else
			return &pmu2_xtaltab0_adfll_485[PMU15_XTALTAB0_20000K];
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
		if (ISSIM_ENAB(sih))
			return &pmu2_xtaltab0_adfll_480[PMU15_XTALTAB0_37400K];
		else
			return &pmu2_xtaltab0_adfll_485[PMU15_XTALTAB0_37400K];
	default:
		break;
	}

	ASSERT(0);
	return NULL;
}

static const pmu2_xtaltab0_t *
BCMATTACHFN(si_pmu2_xtaltab0)(si_t *sih)
{
	switch (CHIPID(sih->chip)) {
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
		if (ISSIM_ENAB(sih))
			return pmu2_xtaltab0_adfll_480;
		else
			return pmu2_xtaltab0_adfll_485;
	default:
		break;
	}

	ASSERT(0);
	return NULL;
}

/** For hardware workarounds, OTP can contain an entry to update FLL control registers */
static void
BCMATTACHFN(si_pmu2_pll_vars_init)(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	char name[16];
	const char *otp_val;
	uint8 i, otp_entry_found = FALSE;
	uint32 pll_ctrlcnt;
	uint32 min_mask = 0, max_mask = 0, clk_ctl_st = 0;

	if (PMUREV(sih->pmurev) >= 5) {
		pll_ctrlcnt = (sih->pmucaps & PCAP5_PC_MASK) >> PCAP5_PC_SHIFT;
	} else {
		pll_ctrlcnt = (sih->pmucaps & PCAP_PC_MASK) >> PCAP_PC_SHIFT;
	}

	/* Check if there is any otp enter for PLLcontrol registers */
	for (i = 0; i < pll_ctrlcnt; i++) {
		snprintf(name, sizeof(name), rstr_pllD, i);
		if ((otp_val = getvar(NULL, name)) == NULL)
			continue;

		/* If OTP entry is found for PLL register, then turn off the PLL
		 * and set the status of the OTP entry accordingly.
		 */
		otp_entry_found = TRUE;
		break;
	}

	/* If no OTP parameter is found, return. */
	if (otp_entry_found == FALSE)
		return;

	/* Make sure PLL is off */
	si_pmu_pll_off(sih, osh, pmu, &min_mask, &max_mask, &clk_ctl_st);

	/* Update the PLL register if there is a OTP entry for PLL registers */
	si_pmu_otp_pllcontrol(sih, osh);

	/* Flush deferred pll control registers writes */
	if (PMUREV(sih->pmurev) >= 2)
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);

	/* Restore back the register values. This ensures PLL remains on if it
	 * was originally on and remains off if it was originally off.
	 */
	si_pmu_pll_on(sih, osh, pmu, min_mask, max_mask, clk_ctl_st);
} /* si_pmu2_pll_vars_init */

/**
 * PLL needs to be initialized to the correct frequency. This function will skip that
 * initialization if the PLL is already at the correct frequency.
 */
static void
BCMATTACHFN(si_pmu2_pllinit0)(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal)
{
	const pmu2_xtaltab0_t *xt;
	int xt_idx;
	uint32 freq_tgt, pll0;
	rsc_per_chip_t *rsc;

	if (xtal == 0) {
		PMU_MSG(("Unspecified xtal frequency, skip PLL configuration\n"));
		goto exit;
	}

	for (xt = si_pmu2_xtaltab0(sih), xt_idx = 0; xt != NULL && xt->fref != 0; xt++, xt_idx++) {
		if (xt->fref == xtal)
			break;
	}

	if (xt == NULL || xt->fref == 0) {
		PMU_MSG(("Unsupported xtal frequency %d.%d MHz, skip PLL configuration\n",
		         xtal / 1000, xtal % 1000));
		goto exit;
	}

	pll0 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, 0, 0);
	freq_tgt = (pll0 & PMU15_PLL_PC0_FREQTGT_MASK) >> PMU15_PLL_PC0_FREQTGT_SHIFT;
	if (freq_tgt == xt->freq_tgt) {
		PMU_MSG(("PLL already programmed for %d.%d MHz\n",
			xt->fref / 1000, xt->fref % 1000));
		goto exit;
	}

	PMU_MSG(("XTAL %d.%d MHz (%d)\n", xtal / 1000, xtal % 1000, xt->xf));
	PMU_MSG(("Programming PLL for %d.%d MHz\n", xt->fref / 1000, xt->fref % 1000));

	/* Make sure the PLL is off */
	switch (CHIPID(sih->chip)) {
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
		rsc = si_pmu_get_rsc_positions(sih);
		AND_REG(osh, &pmu->min_res_mask,
			~(PMURES_BIT(rsc->ht_avail) | PMURES_BIT(rsc->macphy_clkavail)));
		AND_REG(osh, &pmu->max_res_mask,
			~(PMURES_BIT(rsc->ht_avail) | PMURES_BIT(rsc->macphy_clkavail)));
		SPINWAIT(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL, PMU_MAX_TRANSITION_DLY);
		ASSERT(!(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL));
		break;
	default:
		PMU_ERROR(("%s: Turn HT off for 0x%x????\n", __FUNCTION__, CHIPID(sih->chip)));
		break;
	}

	pll0 = (pll0 & ~PMU15_PLL_PC0_FREQTGT_MASK) | (xt->freq_tgt << PMU15_PLL_PC0_FREQTGT_SHIFT);
	W_REG(osh, &pmu->pllcontrol_data, pll0);

	if (CST4334_CHIPMODE_HSIC(sih->chipst)) {
		uint32 hsic_freq;

		/* Only update target freq from 480Mhz table for HSIC */
		ASSERT(xt_idx < PMU15_XTALTAB0_END);
		hsic_freq = pmu2_xtaltab0_adfll_480[xt_idx].freq_tgt;

		/*
		 * Update new tgt freq for PLL control 5
		 * This is activated when USB/HSIC core is taken out of reset (ch_reset())
		 */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, PMU15_PLL_PC5_FREQTGT_MASK, hsic_freq);
	}

	/* Flush deferred pll control registers writes */
	if (PMUREV(sih->pmurev) >= 2)
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);

exit:
	/* Vars over-rides */
	si_pmu2_pll_vars_init(sih, osh, pmu);
} /* si_pmu2_pllinit0 */

/** returns the clock frequency at which the ARM is running */
static uint32
BCMINITFN(si_pmu2_cpuclk0)(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	const pmu2_xtaltab0_t *xt;
	uint32 freq_tgt = 0, pll0 = 0;

	switch (CHIPID(sih->chip)) {
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
		pll0 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, 0, 0);
		freq_tgt = (pll0 & PMU15_PLL_PC0_FREQTGT_MASK) >> PMU15_PLL_PC0_FREQTGT_SHIFT;
		break;
	default:
		ASSERT(0);
		break;
	}

	for (xt = pmu2_xtaltab0_adfll_480; xt != NULL && xt->fref != 0; xt++) {
		if (xt->freq_tgt == freq_tgt)
			return PMU15_ARM_96MHZ;
	}

	for (xt = pmu2_xtaltab0_adfll_485; xt != NULL && xt->fref != 0; xt++) {
		if (xt->freq_tgt == freq_tgt)
			return PMU15_ARM_97MHZ;
	}

	/* default */
	return PMU15_ARM_96MHZ;
}

/** Query ALP/xtal clock frequency */
static uint32
BCMINITFN(si_pmu2_alpclk0)(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	const pmu2_xtaltab0_t *xt;
	uint32 freq_tgt, pll0;

	pll0 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, 0, 0);
	freq_tgt = (pll0 & PMU15_PLL_PC0_FREQTGT_MASK) >> PMU15_PLL_PC0_FREQTGT_SHIFT;


	for (xt = pmu2_xtaltab0_adfll_480; xt != NULL && xt->fref != 0; xt++) {
		if (xt->freq_tgt == freq_tgt)
			break;
	}

	if (xt == NULL || xt->fref == 0) {
		for (xt = pmu2_xtaltab0_adfll_485; xt != NULL && xt->fref != 0; xt++) {
			if (xt->freq_tgt == freq_tgt)
				break;
		}
	}

	/* Could not find it so assign a default value */
	if (xt == NULL || xt->fref == 0)
		xt = si_pmu2_xtaldef0(sih);
	ASSERT(xt != NULL && xt->fref != 0);

	return xt->fref * 1000;
}

/**
 * Set up PLL registers in the PMU as per the (optional) OTP values, or, if no OTP values are
 * present, optionally update with POR override values contained in firmware. Enables the BBPLL
 * when done.
 */
static void
BCMATTACHFN(si_pmu1_pllinit1)(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal)
{
	char name[16];
	const char *otp_val;
	uint8 i, otp_entry_found = FALSE;
	uint32 pll_ctrlcnt;
	uint32 min_mask = 0, max_mask = 0, clk_ctl_st = 0;

	if (PMUREV(sih->pmurev) >= 5) {
		pll_ctrlcnt = (sih->pmucaps & PCAP5_PC_MASK) >> PCAP5_PC_SHIFT;
	} else {
		pll_ctrlcnt = (sih->pmucaps & PCAP_PC_MASK) >> PCAP_PC_SHIFT;
	}

	/* Check if there is any otp enter for PLLcontrol registers */
	for (i = 0; i < pll_ctrlcnt; i++) {
		snprintf(name, sizeof(name), rstr_pllD, i);
		if ((otp_val = getvar(NULL, name)) == NULL)
			continue;

		/* If OTP entry is found for PLL register, then turn off the PLL
		 * and set the status of the OTP entry accordingly.
		 */
		otp_entry_found = TRUE;
		break;
	}

	/* If no OTP parameter is found and no chip-specific updates are needed, return. */
	if ((otp_entry_found == FALSE) &&
		(si_pmu_update_pllcontrol(sih, osh, xtal, FALSE) == FALSE))
		return;

	/* Make sure PLL is off */
	si_pmu_pll_off(sih, osh, pmu, &min_mask, &max_mask, &clk_ctl_st);

	/* Update any chip-specific PLL registers. Does not write PLL 'update' bit yet. */
	si_pmu_update_pllcontrol(sih, osh, xtal, TRUE);

	/* Update the PLL register if there is a OTP entry for PLL registers */
	si_pmu_otp_pllcontrol(sih, osh);

	/* Flush ('update') the deferred pll control registers writes */
	if (PMUREV(sih->pmurev) >= 2)
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);

	/* Restore back the register values. This ensures PLL remains on if it
	 * was originally on and remains off if it was originally off.
	 */
	si_pmu_pll_on(sih, osh, pmu, min_mask, max_mask, clk_ctl_st);

	if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		uint32 origidx;
		/* PMU clock stretch to be decreased to 8 for HT and ALP
		* to reduce DS0 current during high traffic
		*/
		W_REG(osh, &pmu->clkstretch, CSTRETCH_REDUCE_8);

		/* SDIOD to request for ALP
		* to reduce DS0 current during high traffic
		*/
		origidx = si_coreidx(sih);
		si_setcore(sih, SDIOD_CORE_ID, 0);
		/* Clear the Bit 8 for ALP REQUEST change */
		si_wrapperreg(sih, AI_OOBSELOUTB30, (AI_OOBSEL_MASK << AI_OOBSEL_1_SHIFT),
			OOB_B_ALP_REQUEST << AI_OOBSEL_1_SHIFT);
		si_setcoreidx(sih, origidx);
	}
} /* si_pmu1_pllinit1 */

/**
 * Set up PLL registers in the PMU as per the crystal speed.
 * XtalFreq field in pmucontrol register being 0 indicates the PLL
 * is not programmed and the h/w default is assumed to work, in which
 * case the xtal frequency is unknown to the s/w so we need to call
 * si_pmu1_xtaldef0() wherever it is needed to return a default value.
 */
static void
BCMATTACHFN(si_pmu1_pllinit0)(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 xtal)
{
	const pmu1_xtaltab0_t *xt;
	uint32 tmp;
	uint8 ndiv_mode = 1;
	uint8 dacrate;

	/* Use h/w default PLL config */
	if (xtal == 0) {
		PMU_MSG(("Unspecified xtal frequency, skip PLL configuration\n"));
		return;
	}

	/* Find the frequency in the table */
	for (xt = si_pmu1_xtaltab0(sih); xt != NULL && xt->fref != 0; xt ++)
		if (xt->fref == xtal)
			break;

	/* Check current PLL state, bail out if it has been programmed or
	 * we don't know how to program it.
	 */
	if (xt == NULL || xt->fref == 0) {
		PMU_MSG(("Unsupported xtal frequency %d.%d MHz, skip PLL configuration\n",
		         xtal / 1000, xtal % 1000));
		return;
	}
	/*  for 4319/4330 bootloader already programs the PLL but bootloader does not program the
	    PLL4 and PLL5. So Skip this check for 4319/4330
	*/
	if ((((R_REG(osh, &pmu->pmucontrol) & PCTL_XTALFREQ_MASK) >>
		PCTL_XTALFREQ_SHIFT) == xt->xf) &&
		!(CHIPID(sih->chip) == BCM4330_CHIP_ID))
	{
		PMU_MSG(("PLL already programmed for %d.%d MHz\n",
			xt->fref / 1000, xt->fref % 1000));
		return;
	}

	PMU_MSG(("XTAL %d.%d MHz (%d)\n", xtal / 1000, xtal % 1000, xt->xf));
	PMU_MSG(("Programming PLL for %d.%d MHz\n", xt->fref / 1000, xt->fref % 1000));

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
		AND_REG(osh, &pmu->min_res_mask,
			~(PMURES_BIT(RES4336_HT_AVAIL) | PMURES_BIT(RES4336_MACPHY_CLKAVAIL)));
		AND_REG(osh, &pmu->max_res_mask,
			~(PMURES_BIT(RES4336_HT_AVAIL) | PMURES_BIT(RES4336_MACPHY_CLKAVAIL)));
		OSL_DELAY(100);
		SPINWAIT(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL, PMU_MAX_TRANSITION_DLY);
		ASSERT(!(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL));
		break;

	case BCM4330_CHIP_ID:
		AND_REG(osh, &pmu->min_res_mask,
			~(PMURES_BIT(RES4330_HT_AVAIL) | PMURES_BIT(RES4330_MACPHY_CLKAVAIL)));
		AND_REG(osh, &pmu->max_res_mask,
			~(PMURES_BIT(RES4330_HT_AVAIL) | PMURES_BIT(RES4330_MACPHY_CLKAVAIL)));
		OSL_DELAY(100);
		SPINWAIT(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL, PMU_MAX_TRANSITION_DLY);
		ASSERT(!(si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL));
		break;

	default:
		ASSERT(0);
	}

	PMU_MSG(("Done masking\n"));

	/* Write p1div and p2div to pllcontrol[0] */
	tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, 0, 0);
	tmp &= ~(PMU1_PLL0_PC0_P1DIV_MASK | PMU1_PLL0_PC0_P2DIV_MASK);
	tmp |= ((xt->p1div << PMU1_PLL0_PC0_P1DIV_SHIFT) & PMU1_PLL0_PC0_P1DIV_MASK) |
	        ((xt->p2div << PMU1_PLL0_PC0_P2DIV_SHIFT) & PMU1_PLL0_PC0_P2DIV_MASK);
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, tmp);

	if ((CHIPID(sih->chip) == BCM4330_CHIP_ID)) {
		if (CHIPREV(sih->chiprev) < 2)
			dacrate = 160;
		else {
			if (!(dacrate = (uint8)getintvar(NULL, rstr_dacrate2g)))
				dacrate = 80;
		}
		si_pmu_set_4330_plldivs(sih, dacrate);
	}

	if ((CHIPID(sih->chip) == BCM4336_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43362_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4330_CHIP_ID))
		ndiv_mode = PMU1_PLL0_PC2_NDIV_MODE_MFB;
	else
		ndiv_mode = PMU1_PLL0_PC2_NDIV_MODE_MASH;

	/* Write ndiv_int and ndiv_mode to pllcontrol[2] */
	tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
	tmp &= ~(PMU1_PLL0_PC2_NDIV_INT_MASK | PMU1_PLL0_PC2_NDIV_MODE_MASK);
	tmp |= ((xt->ndiv_int << PMU1_PLL0_PC2_NDIV_INT_SHIFT) & PMU1_PLL0_PC2_NDIV_INT_MASK) |
	        ((ndiv_mode << PMU1_PLL0_PC2_NDIV_MODE_SHIFT) & PMU1_PLL0_PC2_NDIV_MODE_MASK);
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, tmp);

	/* Write ndiv_frac to pllcontrol[3] */
	tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0);
	if (BCM4347_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM4369_CHIP_ID)) {
		tmp &= ~PMU4347_PLL0_PC3_NDIV_FRAC_MASK;
		tmp |= ((xt->ndiv_frac << PMU4347_PLL0_PC3_NDIV_FRAC_SHIFT) &
		        PMU4347_PLL0_PC3_NDIV_FRAC_MASK);
	} else
	{
		tmp &= ~PMU1_PLL0_PC3_NDIV_FRAC_MASK;
		tmp |= ((xt->ndiv_frac << PMU1_PLL0_PC3_NDIV_FRAC_SHIFT) &
		        PMU1_PLL0_PC3_NDIV_FRAC_MASK);
	}
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, tmp);

	PMU_MSG(("Done pll\n"));

	/* Flush deferred pll control registers writes */
	if (PMUREV(sih->pmurev) >= 2)
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);

	/* Write XtalFreq. Set the divisor also. */
	tmp = R_REG(osh, &pmu->pmucontrol) &
	        ~(PCTL_ILP_DIV_MASK | PCTL_XTALFREQ_MASK);
	tmp |= (((((xt->fref + 127) / 128) - 1) << PCTL_ILP_DIV_SHIFT) &
	        PCTL_ILP_DIV_MASK) |
	       ((xt->xf << PCTL_XTALFREQ_SHIFT) & PCTL_XTALFREQ_MASK);

	W_REG(osh, &pmu->pmucontrol, tmp);
} /* si_pmu1_pllinit0 */

/**
 * returns the CPU clock frequency. Does this by determining current Fvco and the setting of the
 * clock divider that leads up to the ARM. Returns value in [Hz] units.
 */
static uint32
BCMINITFN(si_pmu1_cpuclk0)(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	uint32 tmp, mdiv = 1;
	uint32 FVCO;	/* in [khz] units */

	if (BCM4365_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM7271_CHIP_ID) ||
		0) {
		/* these chips derive CPU clock from CPU PLL instead of BB PLL */
		FVCO = si_pmu1_pllfvco1(sih);
	} else {
		FVCO = si_pmu1_pllfvco0(sih);
	}

	if (BCM43602_CHIP(sih->chip) &&
		TRUE) {
		/* CR4 running on backplane_clk */
		return si_pmu_si_clock(sih, osh);	/* in [hz] units */
	}

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM4330_CHIP_ID:
		/* Read m1div from pllcontrol[1] */
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		mdiv = (tmp & PMU1_PLL0_PC1_M1DIV_MASK) >> PMU1_PLL0_PC1_M1DIV_SHIFT;
		break;
	case BCM43239_CHIP_ID:
		/* Read m6div from pllcontrol[2] */
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
		mdiv = (tmp & PMU1_PLL0_PC2_M6DIV_MASK) >> PMU1_PLL0_PC2_M6DIV_SHIFT;
		break;
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
		/* Read m2div from pllcontrol[1] */
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		mdiv = (tmp & PMU1_PLL0_PC1_M2DIV_MASK) >> PMU1_PLL0_PC1_M2DIV_SHIFT;
		break;
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
		/* Read m3div from pllcontrol[1] */
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		mdiv = (tmp & PMU1_PLL0_PC1_M3DIV_MASK) >> PMU1_PLL0_PC1_M3DIV_SHIFT;
		break;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM4364_CHIP_ID:
		if (CHIPREV(sih->chiprev) >= 3) {
			tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
			mdiv = (tmp & PMU1_PLL0_PC2_M5DIV_MASK) >> PMU1_PLL0_PC2_M5DIV_SHIFT;
		} else {
			/* Read m3div from pllcontrol[1] */
			tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
			mdiv = (tmp & PMU1_PLL0_PC1_M3DIV_MASK) >> PMU1_PLL0_PC1_M3DIV_SHIFT;
		}
		break;
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
		mdiv = (tmp & PMU1_PLL0_PC2_M5DIV_MASK) >> PMU1_PLL0_PC2_M5DIV_SHIFT;
		break;
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM43526_CHIP_ID:
	case BCM4352_CHIP_ID:
		/* Read m6div from pllcontrol[5] */
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, 0, 0);
		mdiv = (tmp & PMU1_PLL0_PC2_M6DIV_MASK) >> PMU1_PLL0_PC2_M6DIV_SHIFT;
		break;


	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		mdiv = (tmp & PMU1_PLL0_PC1_M3DIV_MASK) >> PMU1_PLL0_PC1_M3DIV_SHIFT;
		break;


	default:
		PMU_MSG(("si_pmu1_cpuclk0: Unknown chipid %s\n", bcm_chipname(sih->chip, chn, 8)));
		ASSERT(0);
		break;
	}

	ASSERT(mdiv != 0);


	return FVCO / mdiv * 1000; /* Return CPU clock in [Hz] */
} /* si_pmu1_cpuclk0 */

/**
 * BCM4347/4369 specific function returning the CPU clock frequency. Does this by determining
 * current Fvco and the setting of the clock divider that leads up to the ARM.
 * Returns value in [Hz] units. For second pll for arm clock in 4347 - 420MHz
 */
static uint32
BCMINITFN(si_pmu1_cpuclk0_pll2)(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	uint32 mdiv = 1;
	uint32 FVCO = si_pmu1_pllfvco0_pll2(sih);	/* in [khz] units */

	mdiv = 1;

	/* Return ARM/SB clock */
	return FVCO / mdiv * 1000;
} /* si_pmu1_cpuclk0_pll2 */

/**
 * Returns the MAC clock frequency. Called when e.g. MAC clk frequency has to change because of
 * interference mitigation.
 */
extern uint32
si_mac_clk(si_t *sih, osl_t *osh)
{
	uint8 mdiv2 = 0;
	uint32 pll_reg, mac_clk = 0;
	chipcregs_t *cc;
	uint origidx, intr_val;

	uint32 FVCO = si_pmu1_pllfvco0(sih);	/* in [khz] units */

	BCM_REFERENCE(osh);

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);
	BCM_REFERENCE(cc);

	switch (CHIPID(sih->chip)) {
	case BCM4335_CHIP_ID:
	case BCM4364_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM53573_CHIP_GRPID:
		pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);

		mdiv2 = (pll_reg & PMU4335_PLL0_PC1_MDIV2_MASK) >>
				PMU4335_PLL0_PC1_MDIV2_SHIFT;
		if (!mdiv2) {
			PMU_ERROR(("mdiv2 calc returned 0! [%d]\n", __LINE__));
			ROMMABLE_ASSERT(0);
			break;
		}
		mac_clk = FVCO / mdiv2;
		break;
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
		pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, 0, 0);
		mdiv2 = (pll_reg & PMU43430_PLL0_PC4_MDIV2_MASK) >>
				PMU43430_PLL0_PC4_MDIV2_SHIFT;
		if (!mdiv2) {
			PMU_ERROR(("mdiv2 calc returned 0! [%d]\n", __LINE__));
			ROMMABLE_ASSERT(0);
			break;
		}
		mac_clk = FVCO / mdiv2;
		break;
	case BCM43012_CHIP_ID:
		mdiv2 = 2;
		mac_clk = FVCO / mdiv2;
		break;
	default:
		PMU_MSG(("si_mac_clk: Unknown chipid %s\n",
			bcm_chipname(CHIPID(sih->chip), chn, 8)));
		ASSERT(0);
		break;
	}

	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);

	return mac_clk;
} /* si_mac_clk */

/** Get chip's FVCO and PLLCTRL1 register value */
extern int
si_pmu_fvco_pllreg(si_t *sih, uint32 *fvco, uint32 *pllreg)
{
	chipcregs_t *cc;
	uint origidx, intr_val;
	int err = BCME_OK;

	if (fvco)
		*fvco = si_pmu1_pllfvco0(sih)/1000;

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);
	BCM_REFERENCE(cc);

	switch (CHIPID(sih->chip)) {
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM53573_CHIP_GRPID:
		if (pllreg)
			*pllreg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		break;

	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
		if (pllreg)
			*pllreg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG12, 0, 0) &
				PMU1_PLL0_PC1_M1DIV_MASK;
		break;

	case BCM43602_CHIP_ID:
		if (pllreg) {
			*pllreg = (si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, 0, 0) &
				PMU1_PLL0_PC1_M3DIV_MASK) >> PMU1_PLL0_PC1_M3DIV_SHIFT;
		}
		break;
	case BCM43012_CHIP_ID:
		 /* mDIV is not supported for 43012 & divisor value is always 2 */
		if (pllreg)
			*pllreg = 2;
		break;
	default:
		PMU_MSG(("si_mac_clk: Unknown chipid %s\n", bcm_chipname(sih->chip, chn, 8)));
		err = BCME_ERROR;
	}

	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);

	return err;
}

/** Return TRUE if scan retention memory's sleep/pm signal was asserted */
bool si_pmu_reset_ret_sleep_log(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;
	uint32 ret_ctl;
	bool was_sleep = FALSE;

	/* Remember original core before switch to chipc */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	ret_ctl = R_REG(osh, &pmu->retention_ctl);
	if (ret_ctl & 0x20000000) {
		W_REG(osh, &pmu->retention_ctl, ret_ctl);
		was_sleep = TRUE;
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return was_sleep;
}

bool
si_pmu_is_autoresetphyclk_disabled(si_t *sih, osl_t *osh)
{
	bool disable = FALSE;

	switch (CHIPID(sih->chip)) {
	case BCM43239_CHIP_ID:
		if (si_pmu_chipcontrol(sih, PMU_CHIPCTL0, 0, 0) & 0x00000002)
			disable = TRUE;
		break;
	default:
		break;
	}

	return disable;
}

/* For 43602a0 MCH2/MCH5 boards: power up PA Reference LDO */
void
si_pmu_switch_on_PARLDO(si_t *sih, osl_t *osh)
{
	uint32 mask;
	pmuregs_t *pmu;
	uint origidx;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
	CASE_BCM43602_CHIP:
		mask = R_REG(osh, &pmu->min_res_mask) | PMURES_BIT(RES43602_PARLDO_PU);
		W_REG(osh, &pmu->min_res_mask, mask);
		mask = R_REG(osh, &pmu->max_res_mask) | PMURES_BIT(RES43602_PARLDO_PU);
		W_REG(osh, &pmu->max_res_mask, mask);
		break;
	default:
		break;
	}
	/* Return to original core */
	si_setcoreidx(sih, origidx);
}

/* For 43602a0 MCH2/MCH5 boards: power off PA Reference LDO */
void
si_pmu_switch_off_PARLDO(si_t *sih, osl_t *osh)
{
	uint32 mask;
	pmuregs_t *pmu;
	uint origidx;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
	case BCM43602_CHIP_ID:
	case BCM43462_CHIP_ID:
		mask = R_REG(osh, &pmu->min_res_mask) & ~PMURES_BIT(RES43602_PARLDO_PU);
		W_REG(osh, &pmu->min_res_mask, mask);
		mask = R_REG(osh, &pmu->max_res_mask) & ~PMURES_BIT(RES43602_PARLDO_PU);
		W_REG(osh, &pmu->max_res_mask, mask);
		break;
	default:
		break;
	}
	/* Return to original core */
	si_setcoreidx(sih, origidx);
}


static void
BCMATTACHFN(si_set_bb_vcofreq_4365)(si_t *sih, osl_t *osh, pmuregs_t *pmu)
{
	uint32 oldpmucontrol, save_clk;
	uint origidx;
	chipcregs_t *cc;

	/* Disable HT works on bp clock */
	W_REG(osh, &pmu->max_res_mask, 0xff);
	W_REG(osh, &pmu->min_res_mask, 0xff);

	/* save clk_ctl_st before changing VCO */
	origidx = si_coreidx(sih);
	cc = si_setcoreidx(sih, SI_CC_IDX);
	save_clk = R_REG(osh, &cc->clk_ctl_st);
	/* Force ALP Request */
	OR_REG(osh, &cc->clk_ctl_st, CCS_FORCEALP);
	/* Check BP clock is on ALP */
	while (!(R_REG(osh, &cc->clk_ctl_st) & CCS_BP_ON_APL)) {
		OSL_DELAY(10);
	}
	si_setcoreidx(sih, origidx);

	/* Set ndiv_int = 32, pdiv = 1 vco = 1280 MHz */
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG9, ~0, 0x201);
	/* Set ch0_mdiv = 2, ch1_mdiv =4, ch2_mdiv = 8 */
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG11, ~0, 0x00080402);
	/* set areset[bit3] to 1, and dreset[bit4] to 1 */
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG7, ~0, 0x53600e0c);
	/* Set pllCtrlUpdate */
	oldpmucontrol = R_REG(osh, &pmu->pmucontrol);
	W_REG(osh, &pmu->pmucontrol, oldpmucontrol | (1<<10));
	OSL_DELAY(10);

	/* set areset[bit3] to 0, and dreset[bit4] to 1 */
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG7, ~0, 0x53600e08);
	/* Set pllCtrlUpdate */
	oldpmucontrol = R_REG(osh, &pmu->pmucontrol);
	W_REG(osh, &pmu->pmucontrol, oldpmucontrol | (1<<10));
	OSL_DELAY(50);

	/* set areset[bit3] to 0, and dreset[bit4] to 0 */
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG7, ~0, 0x53600000);
	/* Set pllCtrlUpdate */
	oldpmucontrol = R_REG(osh, &pmu->pmucontrol);
	W_REG(osh, &pmu->pmucontrol, oldpmucontrol | (1<<10));

	/* max_res_mask, min_res_mask -> 0x3ff */
	W_REG(osh, &pmu->max_res_mask, 0x3ff);
	W_REG(osh, &pmu->min_res_mask, 0x3ff);

	/* Restore the clk_ctl_st after changing the VCO */
	origidx = si_coreidx(sih);
	cc = si_setcoreidx(sih, SI_CC_IDX);
	W_REG(osh, &cc->clk_ctl_st, save_clk);
	si_setcoreidx(sih, origidx);
} /* si_set_bb_vcofreq_4365 */

/**
 * Change VCO frequency (slightly), e.g. to avoid PHY errors due to spurs.
 */
static void
BCMATTACHFN(si_set_bb_vcofreq_frac)(si_t *sih, osl_t *osh, int vcofreq, int frac, int xtalfreq)
{
	uint32 vcofreq_withfrac, p1div, ndiv_int, fraca, ndiv_mode, reg;
	/* shifts / masks for PMU PLL control register #2 : */
	uint32 ndiv_int_shift, ndiv_mode_shift, p1div_shift, pllctrl2_mask;
	/* shifts / masks for PMU PLL control register #3 : */
	uint32 pllctrl3_mask;
	BCM_REFERENCE(osh);

	if ((CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43460_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43526_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM4352_CHIP_ID) ||
	    BCM43602_CHIP(sih->chip)) {
		if (si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL) {
			PMU_MSG(("HTAVAIL is set, so not updating BBPLL Frequency \n"));
			return;
		}

		ndiv_int_shift = 7;
		ndiv_mode_shift = 4;
		p1div_shift = 0;
		pllctrl2_mask = 0xffffffff;
		pllctrl3_mask = 0xffffffff;
	} else if ((BCM4350_CHIP(sih->chip) &&
		(CST4350_IFC_MODE(sih->chipst) == CST4350_IFC_MODE_PCIE)) ||
		/* Include only these 4350 USB chips */
		(CHIPID(sih->chip) == BCM43566_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43569_CHIP_ID)) {
		ndiv_int_shift = 23;
		ndiv_mode_shift = 20;
		p1div_shift = 16;
		pllctrl2_mask = 0xffff0000;
		pllctrl3_mask = 0x00ffffff;
	} else {
		/* put more chips here */
		PMU_ERROR(("%s: only work on 4360, 4350\n", __FUNCTION__));
		return;
	}

	vcofreq_withfrac = vcofreq * 10000 + frac;
	p1div = 0x1;
	ndiv_int = vcofreq / xtalfreq;
	ndiv_mode = (vcofreq_withfrac % (xtalfreq * 10000)) ? 3 : 0;
	PMU_ERROR(("ChangeVCO => vco:%d, xtalF:%d, frac: %d, ndivMode: %d, ndivint: %d\n",
		vcofreq, xtalfreq, frac, ndiv_mode, ndiv_int));

	reg = (ndiv_int << ndiv_int_shift) |
	      (ndiv_mode << ndiv_mode_shift) |
	      (p1div << p1div_shift);
	PMU_ERROR(("Data written into the PLL_CNTRL_ADDR2: %08x\n", reg));
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, pllctrl2_mask, reg);

	if (ndiv_mode) {
		/* frac = (vcofreq_withfrac % (xtalfreq * 10000)) * 2^24) / (xtalfreq * 10000) */
		uint32 r1, r0;
		bcm_uint64_multiple_add(
			&r1, &r0, vcofreq_withfrac % (xtalfreq * 10000), 1 << 24, 0);
		bcm_uint64_divide(&fraca, r1, r0, xtalfreq * 10000);
		PMU_ERROR(("Data written into the PLL_CNTRL_ADDR3 (Fractional): %08x\n", fraca));
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, pllctrl3_mask, fraca);
	}

	si_pmu_pllupd(sih);
} /* si_set_bb_vcofreq_frac */

/**
 * given x-tal frequency, returns BaseBand vcofreq with fraction in 100Hz
 * @param   xtalfreq In [Mhz] units.
 * @return           In [100Hz] units.
 */
uint32
si_pmu_get_bb_vcofreq(si_t *sih, osl_t *osh, int xtalfreq)
{
	uint32  ndiv_int,	/* 9 bits integer divider */
		ndiv_mode,
		frac = 0,	/* 24 bits fractional divider */
		p1div;		/* predivider: divides x-tal freq */
	uint32 xtal1, vcofrac = 0, vcofreq;
	uint32 r1, r0, reg;

	BCM_REFERENCE(osh);

	if ((CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43460_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43526_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM4352_CHIP_ID) ||
	    BCM43602_CHIP(sih->chip)) {
		reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
		ndiv_int = reg >> 7;
		ndiv_mode = (reg >> 4) & 7;
		p1div = 1; /* do not divide x-tal frequency */

		if (ndiv_mode)
			frac = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0);
	} else if ((BCM4350_CHIP(sih->chip) &&
		(CST4350_IFC_MODE(sih->chipst) == CST4350_IFC_MODE_PCIE)) ||
		/* Include only these 4350 USB chips */
		(CHIPID(sih->chip) == BCM43566_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43569_CHIP_ID) ||
		((BCM4347_CHIP(sih->chip)) &&
		(CST4347_CHIPMODE_PCIE(sih->chipst))) ||
		((CHIPID(sih->chip) == BCM4369_CHIP_ID) &&
			(CST4369_CHIPMODE_PCIE(sih->chipst))) ||
		(BCM4349_CHIP(sih->chip) &&
		(CHIPID(sih->chip) != BCM4373_CHIP_ID) &&
		(CST4349_CHIPMODE_PCIE(sih->chipst))) ||
		(BCM53573_CHIP(sih->chip) &&
		(CST53573_CHIPMODE_PCIE(sih->chipst))) ||
		((CHIPID(sih->chip) == BCM4364_CHIP_ID) &&
		(CST4364_CHIPMODE_PCIE(sih->chipst))) ||
		0) {
		reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
		ndiv_int = reg >> 23;
		ndiv_mode = (reg >> 20) & 7;
		p1div = (reg >> 16) & 0xf;

		if (ndiv_mode)
			frac = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0) & 0x00ffffff;
	} else {
		/* put more chips here */
		PMU_ERROR(("%s: only work on 4360, 4350\n", __FUNCTION__));
		ASSERT(FALSE);
		return 0;
	}

	xtal1 = 10000 * xtalfreq / p1div;		/* in [100Hz] units */

	if (ndiv_mode) {
		/* vcofreq fraction = (xtal1 * frac + (1 << 23)) / (1 << 24);
		 * handle overflow
		 */
		bcm_uint64_multiple_add(&r1, &r0, xtal1, frac, 1 << 23);
		vcofrac = (r1 << 8) | (r0 >> 24);
	}

	if (ndiv_int == 0) {
		ASSERT(0);
		return 0;
	}

	if ((int)xtal1 > (int)((0xffffffff - vcofrac) / ndiv_int)) {
		PMU_ERROR(("%s: xtalfreq is too big, %d\n", __FUNCTION__, xtalfreq));
		return 0;
	}

	vcofreq = xtal1 * ndiv_int + vcofrac;
	return vcofreq;
} /* si_pmu_get_bb_vcofreq */

/** Enable PMU 1Mhz clock */
static void
si_pmu_enb_slow_clk(si_t *sih, osl_t *osh, uint32 xtalfreq)
{
	uint32 val;
	pmuregs_t *pmu;
	uint origidx;

	if (PMUREV(sih->pmurev) < 24) {
		PMU_ERROR(("%s: Not supported %d\n", __FUNCTION__, PMUREV(sih->pmurev)));
		return;
	}

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	/* twiki PmuRev30, OneMhzToggleEn:31, AlpPeriod[23:0] */
	if (PMUREV(sih->pmurev) >= 30) {
		/* AlpPeriod = ROUND(POWER(2,26)/ALP_CLK_FREQ_IN_MHz,0) */
		/* Calculation will be accurate for only one decimal of xtal (like 37.4),
		* and will not be accurate for more than one decimal of xtal freq (like 37.43)
		* Also no rounding is done on final result
		*/
		ROMMABLE_ASSERT((xtalfreq/100)*100 == xtalfreq);
		val = (((1 << 26)*10)/(xtalfreq/100));
		/* set the 32 bit to enable OneMhzToggle
		* -usec wide toggle signal will be generated
		*/

		if (CHIPID(sih->chip) == BCM7271_CHIP_ID)
			val |= BCM7271_PMU30_ALPCLK_ONEMHZ_ENAB;
		else
			val |= PMU30_ALPCLK_ONEMHZ_ENAB;

	} else { /* twiki PmuRev24, OneMhzToggleEn:16, AlpPeriod[15:0] */
		if (xtalfreq == 37400) {
			val = 0x101B6;
		} else if (xtalfreq == 40000) {
			val = 0x10199;
		} else {
			PMU_ERROR(("%s: xtalfreq is not supported, %d\n", __FUNCTION__, xtalfreq));
			return;
		}
	}

	W_REG(osh, &pmu->slowclkperiod, val);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
}

/**
 * Initializes PLL given an x-tal frequency.
 * Calls si_pmuX_pllinitY() type of functions, where the reasoning behind 'X' and 'Y' is historical
 * rather than logical.
 *
 * xtalfreq : x-tal frequency in [KHz]
 */
void
BCMATTACHFN(si_pmu_pll_init)(si_t *sih, osl_t *osh, uint xtalfreq)
{
	pmuregs_t *pmu;
	uint origidx;
	BCM_REFERENCE(bcm4328a0_res_depend);
	BCM_REFERENCE(bcm4328a0_res_updown);
	BCM_REFERENCE(bcm4325a0_res_updown_qt);
	BCM_REFERENCE(bcm4325a0_res_updown);
	BCM_REFERENCE(bcm4325a0_res_depend);
	BCM_REFERENCE(bcm4315a0_res_depend);
	BCM_REFERENCE(bcm4329_res_updown);
	BCM_REFERENCE(bcm4319a0_res_depend);
	BCM_REFERENCE(bcm4364a0_res_updown);
	BCM_REFERENCE(bcm4364a0_res_depend_rsdb);
	BCM_REFERENCE(pmu1_xtaltab0_880);
	BCM_REFERENCE(pmu1_xtaltab0_1760);
	BCM_REFERENCE(pmu1_pllctrl_tab_43430_960mhz);
	BCM_REFERENCE(pmu1_xtaltab0_4364);
	BCM_REFERENCE(pmu1_pllctrl_tab_4364_960mhz);
	BCM_REFERENCE(bcm4329_res_depend);
	BCM_REFERENCE(bcm4319a0_res_updown_qt);
	BCM_REFERENCE(bcm4319a0_res_updown);
	BCM_REFERENCE(bcm4315a0_res_updown_qt);
	BCM_REFERENCE(bcm4315a0_res_updown);
	BCM_REFERENCE(bcm4350_res_depend);

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM4352_CHIP_ID: {
		if (CHIPREV(sih->chiprev) > 2)
			si_set_bb_vcofreq_frac(sih, osh, 960, 98, 40);
		break;
	}
	CASE_BCM43602_CHIP:
		si_set_bb_vcofreq_frac(sih, osh, 960, 98, 40);
		break;
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
#ifdef BCM7271
	case BCM7271_CHIP_ID:
		if (CHIPID(sih->chip) == BCM7271_CHIP_ID) {
			PMU_MSG(("%s(%d) buscorerev=0x%08x\n",
			__FUNCTION__, __LINE__, sih->buscorerev));
		}
#endif /* BCM7271 */
		if (sih->buscorerev == 15) {
			si_set_bb_vcofreq_4365(sih, osh, pmu); /* WAR for a0 chips */
		}
		si_set_bb_vcofreq_frac(sih, osh, 1600, 0, 40);
		break;
	case BCM4313_CHIP_ID:
	case BCM43224_CHIP_ID:	case BCM43225_CHIP_ID:  case BCM43420_CHIP_ID:
	case BCM43421_CHIP_ID:
	case BCM43235_CHIP_ID:	case BCM43236_CHIP_ID:	case BCM43238_CHIP_ID:
	case BCM43237_CHIP_ID:
	case BCM43234_CHIP_ID:
	case BCM4331_CHIP_ID:
	case BCM43431_CHIP_ID:
	case BCM43131_CHIP_ID:
	case BCM43217_CHIP_ID:
	case BCM43227_CHIP_ID:
	case BCM43228_CHIP_ID:
	case BCM43428_CHIP_ID:
	case BCM6362_CHIP_ID:
		break;
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM4330_CHIP_ID:
		si_pmu1_pllinit0(sih, osh, pmu, xtalfreq);
		break;
	case BCM43239_CHIP_ID:
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM43909_CHIP_ID:
	case BCM43430_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43012_CHIP_ID:
	case BCM4364_CHIP_ID:
	case BCM4369_CHIP_ID:
	case BCM4347_CHIP_GRPID:
		si_pmu1_pllinit1(sih, osh, pmu, xtalfreq); /* nvram PLL overrides + enables PLL */
		break;
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM4358_CHIP_ID:
		si_pmu1_pllinit1(sih, osh, pmu, xtalfreq);
		if (xtalfreq == 40000)
			si_set_bb_vcofreq_frac(sih, osh, 968, 0, 40);
		break;
	case BCM43566_CHIP_ID:
	case BCM43567_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
		si_pmu1_pllinit1(sih, osh, pmu, xtalfreq);
		if (xtalfreq == 40000)
			/* VCO 960.0098MHz does not produce spurs in 2G
				and it does not suffer from Tx FIFO underflows
			*/
			si_set_bb_vcofreq_frac(sih, osh, 960, 98, 40);
		break;
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
		if (xtalfreq == 0)
			xtalfreq = 20000;
		si_pmu2_pllinit0(sih, osh, pmu, xtalfreq);
		break;
	case BCM4334_CHIP_ID:
		si_pmu2_pllinit0(sih, osh, pmu, xtalfreq);
		break;
	default:
		PMU_MSG(("No PLL init done for chip %s rev %d pmurev %d\n",
		         bcm_chipname(
			 CHIPID(sih->chip), chn, 8), CHIPREV(sih->chiprev), PMUREV(sih->pmurev)));
		break;
	}

#ifdef BCMDBG_FORCEHT
	si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), CCS_FORCEHT, CCS_FORCEHT)
#endif

	si_pmu_enb_slow_clk(sih, osh, xtalfreq);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
} /* si_pmu_pll_init */

/** get alp clock frequency in [Hz] units */
uint32
BCMINITFN(si_pmu_alp_clock)(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;
	uint32 clock = ALP_CLOCK;

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM43526_CHIP_ID:
		if (sih->chipst & CST4360_XTAL_40MZ)
			clock = 40000 * 1000;
		else
			clock = 20000 * 1000;
		break;

	CASE_BCM43602_CHIP:
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
	case BCM53573_CHIP_GRPID:
		/* always 40Mhz */
		clock = 40000 * 1000;
		break;
	case BCM7271_CHIP_ID:
		 /* BCM7271 should be 54Mhz */
		clock = XTAL_FREQ_54MHZ * 1000;
		break;
	case BCM43224_CHIP_ID:	case BCM43225_CHIP_ID:  case BCM43420_CHIP_ID:
	case BCM43421_CHIP_ID:
	case BCM43235_CHIP_ID:	case BCM43236_CHIP_ID:	case BCM43238_CHIP_ID:
	case BCM43237_CHIP_ID:	case BCM43239_CHIP_ID:
	case BCM43234_CHIP_ID:
	case BCM4331_CHIP_ID:
	case BCM43431_CHIP_ID:
	case BCM43131_CHIP_ID:
	case BCM43217_CHIP_ID:
	case BCM43227_CHIP_ID:
	case BCM43228_CHIP_ID:
	case BCM43428_CHIP_ID:
	case BCM6362_CHIP_ID:
	case BCM4313_CHIP_ID:
	case BCM5357_CHIP_ID:
	case BCM4749_CHIP_ID:
	case BCM53572_CHIP_ID:
		/* always 20Mhz */
		clock = 20000 * 1000;
		break;
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM4330_CHIP_ID:
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM43430_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43012_CHIP_ID:
	case BCM4364_CHIP_ID:
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		clock = si_pmu1_alpclk0(sih, osh, pmu);
		break;
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
		clock = si_pmu2_alpclk0(sih, osh, pmu);
		break;
	case BCM5356_CHIP_ID:
	case BCM4706_CHIP_ID:
		/* always 25Mhz */
		clock = 25000 * 1000;
		break;
	default:
		PMU_MSG(("No ALP clock specified "
			"for chip %s rev %d pmurev %d, using default %d Hz\n",
			bcm_chipname(
			CHIPID(sih->chip), chn, 8), CHIPREV(sih->chiprev),
			PMUREV(sih->pmurev), clock));
		break;
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return clock; /* in [Hz] units */
} /* si_pmu_alp_clock */

/**
 * Find the output of the "m" pll divider given pll controls that start with
 * pllreg "pll0" i.e. 12 for main 6 for phy, 0 for misc.
 */
static uint32
BCMINITFN(si_pmu5_clock)(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint pll0, uint m)
{
	uint32 tmp, div, ndiv, p1, p2, fc;

	if ((pll0 & 3) || (pll0 > PMU4716_MAINPLL_PLL0)) {
		PMU_ERROR(("%s: Bad pll0: %d\n", __FUNCTION__, pll0));
		return 0;
	}


	/* Strictly there is an m5 divider, but I'm not sure we use it */
	if ((m == 0) || (m > 4)) {
		PMU_ERROR(("%s: Bad m divider: %d\n", __FUNCTION__, m));
		return 0;
	}

	if ((CHIPID(sih->chip) == BCM5357_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4749_CHIP_ID)) {
		/* Detect failure in clock setting */
		if ((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, chipstatus), 0, 0) & 0x40000)
			!= 0) {
			return (133 * 1000000);
		}
	}

	W_REG(osh, &pmu->pllcontrol_addr, pll0 + PMU5_PLL_P1P2_OFF);
	(void)R_REG(osh, &pmu->pllcontrol_addr);
	tmp = R_REG(osh, &pmu->pllcontrol_data);
	p1 = (tmp & PMU5_PLL_P1_MASK) >> PMU5_PLL_P1_SHIFT;
	p2 = (tmp & PMU5_PLL_P2_MASK) >> PMU5_PLL_P2_SHIFT;

	W_REG(osh, &pmu->pllcontrol_addr, pll0 + PMU5_PLL_M14_OFF);
	(void)R_REG(osh, &pmu->pllcontrol_addr);
	tmp = R_REG(osh, &pmu->pllcontrol_data);
	div = (tmp >> ((m - 1) * PMU5_PLL_MDIV_WIDTH)) & PMU5_PLL_MDIV_MASK;

	W_REG(osh, &pmu->pllcontrol_addr, pll0 + PMU5_PLL_NM5_OFF);
	(void)R_REG(osh, &pmu->pllcontrol_addr);
	tmp = R_REG(osh, &pmu->pllcontrol_data);
	ndiv = (tmp & PMU5_PLL_NDIV_MASK) >> PMU5_PLL_NDIV_SHIFT;

	/* Do calculation in Mhz */
	fc = si_pmu_alp_clock(sih, osh) / 1000000;
	fc = (p1 * ndiv * fc) / p2;

	PMU_NONE(("%s: p1=%d, p2=%d, ndiv=%d(0x%x), m%d=%d; fc=%d, clock=%d\n",
	          __FUNCTION__, p1, p2, ndiv, ndiv, m, div, fc, fc / div));

	/* Return clock in Hertz */
	return ((fc / div) * 1000000);
} /* si_pmu5_clock */

static uint32
BCMINITFN(si_4706_pmu_clock)(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint pll0, uint m)
{
	uint32 w, ndiv, p1div, p2div;
	uint32 h, l, freq;
	uint64 clock;

	/* Strictly there is an m5 divider, but I'm not sure we use it */
	if ((m == 0) || (m > 4)) {
		PMU_ERROR(("%s: Bad m divider: %d\n", __FUNCTION__, m));
		return 0;
	}

	/* Get N, P1 and P2 dividers to determine CPU clock */
	w = si_pmu_pllcontrol(sih, pll0 + PMU6_4706_PROCPLL_OFF, 0, 0);
	ndiv = (w & PMU6_4706_PROC_NDIV_INT_MASK) >> PMU6_4706_PROC_NDIV_INT_SHIFT;
	p1div = (w & PMU6_4706_PROC_P1DIV_MASK) >> PMU6_4706_PROC_P1DIV_SHIFT;
	p2div = (w & PMU6_4706_PROC_P2DIV_MASK) >> PMU6_4706_PROC_P2DIV_SHIFT;

	if (si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, chipstatus), 0, 0)
		& CST4706_PKG_OPTION) {
		freq = 25000000 / 4;
	} else {
		freq = 25000000 / 2;
	}

	bcm_uint64_multiple_add(&h, &l, freq, ndiv * p2div, 0);

	if (m == PMU5_MAINPLL_MEM) {
		p1div *= 2;
	} else if (m == PMU5_MAINPLL_SI) {
		p1div *= 4;
	} else {
	}

	clock = bcm_uint64_div(h, l, p1div);

	if (clock >> 32) {
		PMU_ERROR(("%s: Integer overflow\n", __FUNCTION__));
		return 0;
	}

	return (uint32)clock;
}

/**
 * Get backplane clock frequency, returns a value in [hz] units.
 * For designs that feed the same clock to both backplane and CPU just return the CPU clock speed.
 */
uint32
BCMINITFN(si_pmu_si_clock)(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;
	uint32 clock = HT_CLOCK;	/* in [hz] units */

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
	case BCM43224_CHIP_ID:  case BCM43420_CHIP_ID:
	case BCM43225_CHIP_ID:
	case BCM43421_CHIP_ID:
	case BCM4331_CHIP_ID:
	case BCM43431_CHIP_ID:
	case BCM6362_CHIP_ID:
		/* 96MHz backplane clock */
		clock = 96000 * 1000;
		break;
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM4330_CHIP_ID:
	case BCM43239_CHIP_ID:
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
	case BCM4360_CHIP_ID:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM43526_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM43012_CHIP_ID:
		clock = si_pmu1_cpuclk0(sih, osh, pmu);
		break;

	case BCM4349_CHIP_GRPID:
	case BCM4364_CHIP_ID:
		{
			uint32 tmp, mdiv = 1;
			/* backplane clock for 4349/4373 is different from arm clock */
			clock = si_pmu1_pllfvco0(sih);
			/* Read m4div from pllcontrol[1] */
			tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
			mdiv = (tmp & PMU1_PLL0_PC1_M4DIV_MASK) >> PMU1_PLL0_PC1_M4DIV_SHIFT;
			clock = clock/mdiv * 1000;
			break;
		}

	CASE_BCM43602_CHIP: {
			uint32 mdiv;
			/* Ch3 is connected to backplane_clk. Read 'bbpll_i_m3div' from pllctl[4] */
			mdiv = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, 0, 0);
			mdiv = (mdiv & PMU1_PLL0_PC1_M3DIV_MASK) >> PMU1_PLL0_PC1_M3DIV_SHIFT;
			ASSERT(mdiv != 0);
			clock = si_pmu1_pllfvco0(sih) / mdiv * 1000;
			break;
		}

	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
#ifdef BCM7271
	case BCM7271_CHIP_ID:
#endif /* BCM7271 */
		{ /* BP clock is derived from CPU PLL */
			uint32 mdiv = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG11, 0, 0);
			if (CHIPREV(sih->chiprev) < 4) {
				mdiv = (mdiv &
					PMU1_PLL0_PC1_M3DIV_MASK) >> PMU1_PLL0_PC1_M3DIV_SHIFT;
				clock = si_pmu1_pllfvco1(sih) / mdiv * 1000;
			} else {
				mdiv = (mdiv &
					PMU1_PLL0_PC1_M1DIV_MASK) >> PMU1_PLL0_PC1_M1DIV_SHIFT;
				clock = si_pmu1_pllfvco1(sih) / (mdiv * 4) * 1000;
			}
			break;
		}


	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
		clock = si_pmu1_cpuclk0(sih, osh, pmu);
		break;

	case BCM4313_CHIP_ID:
		/* 80MHz backplane clock */
		clock = 80000 * 1000;
		break;
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID: /* HT clock and ARM clock have the same frequency */
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
		clock = si_pmu2_cpuclk0(sih, osh, pmu);
		break;
	case BCM43235_CHIP_ID:	case BCM43236_CHIP_ID:	case BCM43238_CHIP_ID:
	case BCM43237_CHIP_ID:
	case BCM43234_CHIP_ID:
		clock = (si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, chipstatus), 0, 0)
			& CST43236_BP_CLK) ? (120000 * 1000) : (96000 * 1000);
		break;
	case BCM5356_CHIP_ID:
		clock = si_pmu5_clock(sih, osh, pmu, PMU5356_MAINPLL_PLL0, PMU5_MAINPLL_SI);
		break;
	case BCM5357_CHIP_ID:
	case BCM4749_CHIP_ID:
		clock = si_pmu5_clock(sih, osh, pmu, PMU5357_MAINPLL_PLL0, PMU5_MAINPLL_SI);
		break;
	case BCM53572_CHIP_ID:
		clock = 75000000;
		break;
	case BCM4706_CHIP_ID:
		clock = si_4706_pmu_clock(sih, osh, pmu, PMU4706_MAINPLL_PLL0, PMU5_MAINPLL_SI);
		break;
	case BCM53573_CHIP_GRPID: {
		uint32 fvco, pdiv, pll_ctrl_20, mdiv, ndiv_nfrac, r_high, r_low;
		uint32 xtal_freq = si_alp_clock(sih);

		/* Read mdiv, pdiv from pllcontrol[13], ch2 for NIC400 */
		mdiv = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG13, 0, 0);
		mdiv = (mdiv >> 16) & 0xff;
		ASSERT(mdiv);
		pdiv = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG13, 0, 0);
		pdiv = (pdiv >> 24) & 0x7;
		ASSERT(pdiv);

		/* Read ndiv[29:20], ndiv_frac[19:0] from pllcontrol[14] */
		ndiv_nfrac = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG14, 0, 0) & 0x3fffffff;

		/* Read pll_ctrl_20 from pllcontrol[15] */
		pll_ctrl_20 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG15, 0, 0);
		pll_ctrl_20 = 1 << ((pll_ctrl_20 >> 20) & 0x1);

		bcm_uint64_multiple_add(&r_high, &r_low, ndiv_nfrac, (xtal_freq * pll_ctrl_20), 0);
		bcm_uint64_right_shift(&fvco, r_high, r_low, 20);

		clock = (fvco / pdiv)/ mdiv;
		break;
	}

	default:
		PMU_MSG(("No backplane clock specified "
			"for chip %s rev %d pmurev %d, using default %d Hz\n",
			bcm_chipname(
			CHIPID(sih->chip), chn, 8), CHIPREV(sih->chiprev),
			PMUREV(sih->pmurev), clock));
		break;
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return clock;
} /* si_pmu_si_clock */

/** returns CPU clock frequency in [hz] units */
uint32
BCMINITFN(si_pmu_cpu_clock)(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;
	uint32 clock;	/* in [hz] units */

	uint32 tmp;
	uint8 m3div, m4div;
	uint32 armclk_offcnt, armclk_oncnt;

	ASSERT(sih->cccaps & CC_CAP_PMU);
	if (CHIPID(sih->chip) == BCM7271_CHIP_ID) {
		PMU_ERROR(("No ARM for 7271. Asserting at line %d\n", __LINE__));
		ASSERT(0);
	}

	if (CHIPID(sih->chip) == BCM53572_CHIP_ID)
		return 300000000;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if ((PMUREV(sih->pmurev) >= 5) &&
		!((CHIPID(sih->chip) == BCM43234_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43235_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43236_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43238_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43237_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43239_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4336_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43362_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4314_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43142_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43143_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43340_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43341_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4334_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4324_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43242_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43243_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4330_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4360_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4352_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43526_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43460_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4345_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43430_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43018_CHIP_ID) ||
		(BCM4349_CHIP(sih->chip)) ||
		(CHIPID(sih->chip) == BCM43909_CHIP_ID) ||
		BCM4350_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM4335_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43012_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4364_CHIP_ID) ||
		(BCM4347_CHIP(sih->chip)) ||
		(CHIPID(sih->chip) == BCM4369_CHIP_ID) ||
		0)) {
		uint pll;

		switch (CHIPID(sih->chip)) {
		case BCM5356_CHIP_ID:
			pll = PMU5356_MAINPLL_PLL0;
			break;
		case BCM5357_CHIP_ID:
		case BCM4749_CHIP_ID:
			pll = PMU5357_MAINPLL_PLL0;
			break;
		default:
			pll = PMU4716_MAINPLL_PLL0;
			break;
		}

		if (CHIPID(sih->chip) == BCM4706_CHIP_ID)
			clock = si_4706_pmu_clock(sih, osh, pmu,
				PMU4706_MAINPLL_PLL0, PMU5_MAINPLL_CPU);
		else if (BCM4365_CHIP(sih->chip) ||
			BCM43602_CHIP(sih->chip) ||
			(CHIPID(sih->chip) == BCM7271_CHIP_ID))
			clock = si_pmu1_cpuclk0(sih, osh, pmu);
		else
			clock = si_pmu5_clock(sih, osh, pmu, pll, PMU5_MAINPLL_CPU);
	} else if (BCM4347_CHIP(sih->chip) ||
		CHIPID(sih->chip) == BCM4369_CHIP_ID) {
		clock = si_pmu1_cpuclk0_pll2(sih, osh, pmu); /* for chips with separate CPU PLL */
	} else if (CHIPID(sih->chip) == BCM4364_CHIP_ID) {
		origidx = si_coreidx(sih);
		pmu = si_setcoreidx(sih, SI_CC_IDX);
		ASSERT(pmu != NULL);

		clock = si_pmu1_cpuclk0(sih, osh, pmu);
		si_setcoreidx(sih, origidx);
	} else {
		clock = si_pmu_si_clock(sih, osh);
	}

	if (BCM4349_CHIP(sih->chip)) {
		/* for 4349a0 cpu clock and back plane clock are not the same */
		/* read post dividers for both the clocks */
		tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		m4div = (tmp & PMU1_PLL0_PC1_M4DIV_MASK) >> PMU1_PLL0_PC1_M4DIV_SHIFT;
		m3div = (tmp & PMU1_PLL0_PC1_M3DIV_MASK) >> PMU1_PLL0_PC1_M3DIV_SHIFT;
		ASSERT(m3div != 0);
		clock = (m4div/m3div)*clock;
	}
	if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		/* Fout = (on_count + 1) * Fin/(on_count + 1 + off_count)
		* ARM clock using Fast divider calculation
		* Fin = FVCO/2
		*/
		tmp = si_pmu_chipcontrol(sih, PMU1_PLL0_CHIPCTL1, 0, 0);
		armclk_offcnt =
			(tmp & CCTL_43012_ARM_OFFCOUNT_MASK) >> CCTL_43012_ARM_OFFCOUNT_SHIFT;
		armclk_oncnt =
			(tmp & CCTL_43012_ARM_ONCOUNT_MASK) >> CCTL_43012_ARM_ONCOUNT_SHIFT;
		clock = (armclk_oncnt + 1) * clock/(armclk_oncnt + 1 + armclk_offcnt);
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	return clock;
} /* si_pmu_cpu_clock */

/** get memory clock frequency, which is the same as the HT clock for newer chips. Returns [Hz]. */
uint32
BCMINITFN(si_pmu_mem_clock)(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;
	uint32 clock;

	ASSERT(sih->cccaps & CC_CAP_PMU);

	if (CHIPID(sih->chip) == BCM7271_CHIP_ID) {
		PMU_ERROR(("No mem clock for 7271. Asserting at line %d\n", __LINE__));
		ASSERT(0);
	}

	if (CHIPID(sih->chip) == BCM53572_CHIP_ID)
		return 150000000;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if ((PMUREV(sih->pmurev) >= 5) &&
		!((CHIPID(sih->chip) == BCM4330_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4314_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43142_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43143_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43340_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43341_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4334_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4336_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43362_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43234_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43235_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43236_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43238_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43237_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43239_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4324_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43242_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43243_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4335_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4345_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43909_CHIP_ID) ||
		BCM4365_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM4364_CHIP_ID) ||
		(BCM4347_CHIP(sih->chip)) ||
		(CHIPID(sih->chip) == BCM4369_CHIP_ID) ||
		BCM43602_CHIP(sih->chip) ||
		BCM4350_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM43012_CHIP_ID) ||
		BCM4349_CHIP(sih->chip) ||
		0)) {
		uint pll;

		switch (CHIPID(sih->chip)) {
		case BCM5356_CHIP_ID:
			pll = PMU5356_MAINPLL_PLL0;
			break;
		case BCM5357_CHIP_ID:
		case BCM4749_CHIP_ID:
			pll = PMU5357_MAINPLL_PLL0;
			break;
		default:
			pll = PMU4716_MAINPLL_PLL0;
			break;
		}
		if (CHIPID(sih->chip) == BCM4706_CHIP_ID)
			clock = si_4706_pmu_clock(sih, osh, pmu,
				PMU4706_MAINPLL_PLL0, PMU5_MAINPLL_MEM);
		else
			clock = si_pmu5_clock(sih, osh, pmu, pll, PMU5_MAINPLL_MEM);
	} else if (BCM4365_CHIP(sih->chip)) {
		uint32 mdiv = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG11, 0, 0);
		ASSERT(mdiv != 0);
		if (CHIPREV(sih->chiprev) < 4) {
			mdiv = (mdiv &
				PMU1_PLL0_PC1_M2DIV_MASK) >> PMU1_PLL0_PC1_M2DIV_SHIFT;
			clock = si_pmu1_pllfvco1(sih) / mdiv * 1000;
		} else {
			mdiv = (mdiv &
				PMU1_PLL0_PC1_M1DIV_MASK) >> PMU1_PLL0_PC1_M1DIV_SHIFT;
			clock = si_pmu1_pllfvco1(sih) / (mdiv * 2) * 1000;
		}
	} else {
		clock = si_pmu_si_clock(sih, osh); /* mem clk same as backplane clk */
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	return clock;
} /* si_pmu_mem_clock */

/*
 * ilpcycles per sec are now calculated during CPU init in a new way
 * for better accuracy.  We set it here for compatability.
 *
 * On platforms that do not do this we resort to the old way.
 */

#define ILP_CALC_DUR	10	/* ms, make sure 1000 can be divided by it. */

static uint32 ilpcycles_per_sec = 0;

void
BCMINITFN(si_pmu_ilp_clock_set)(uint32 cycles_per_sec)
{
	ilpcycles_per_sec = cycles_per_sec;
}

uint32
BCMINITFN(si_pmu_ilp_clock)(si_t *sih, osl_t *osh)
{
	if (ISSIM_ENAB(sih))
		return ILP_CLOCK;

	if (ilpcycles_per_sec == 0) {
		uint32 start, end, delta;
		pmuregs_t *pmu;
		uint origidx = si_coreidx(sih);

		if (AOB_ENAB(sih)) {
			pmu = si_setcore(sih, PMU_CORE_ID, 0);
		} else {
			pmu = si_setcoreidx(sih, SI_CC_IDX);
		}
		ASSERT(pmu != NULL);
		start = R_REG(osh, &pmu->pmutimer);
		if (start != R_REG(osh, &pmu->pmutimer))
			start = R_REG(osh, &pmu->pmutimer);
		OSL_DELAY(ILP_CALC_DUR * 1000);
		end = R_REG(osh, &pmu->pmutimer);
		if (end != R_REG(osh, &pmu->pmutimer))
			end = R_REG(osh, &pmu->pmutimer);
		delta = end - start;
		ilpcycles_per_sec = delta * (1000 / ILP_CALC_DUR);
		/* Return to original core */
		si_setcoreidx(sih, origidx);
	}

	return ilpcycles_per_sec;
}
#endif /* !defined(BCMDONGLEHOST) */

/* SDIO Pad drive strength to select value mappings.
 * The last strength value in each table must be 0 (the tri-state value).
 */
typedef struct {
	uint8 strength;			/* Pad Drive Strength in mA */
	uint8 sel;			/* Chip-specific select value */
} sdiod_drive_str_t;

/* SDIO Drive Strength to sel value table for PMU Rev 1 */
static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab1)[] = {
	{4, 0x2},
	{2, 0x3},
	{1, 0x0},
	{0, 0x0} };

/* SDIO Drive Strength to sel value table for PMU Rev 2, 3 */
static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab2)[] = {
	{12, 0x7},
	{10, 0x6},
	{8, 0x5},
	{6, 0x4},
	{4, 0x2},
	{2, 0x1},
	{0, 0x0} };


/* SDIO Drive Strength to sel value table for PMU Rev 8 (1.8V) */
static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab3)[] = {
	{32, 0x7},
	{26, 0x6},
	{22, 0x5},
	{16, 0x4},
	{12, 0x3},
	{8, 0x2},
	{4, 0x1},
	{0, 0x0} };

/* SDIO Drive Strength to sel value table for PMU Rev 11 (1.8v) */
static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab4_1v8)[] = {
	{32, 0x6},
	{26, 0x7},
	{22, 0x4},
	{16, 0x5},
	{12, 0x2},
	{8, 0x3},
	{4, 0x0},
	{0, 0x1} };

/* SDIO Drive Strength to sel value table for PMU Rev 11 (1.2v) */

/* SDIO Drive Strength to sel value table for PMU Rev 11 (2.5v) */

/* SDIO Drive Strength to sel value table for PMU Rev 13 (1.8v) */
static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab5_1v8)[] = {
	{6, 0x7},
	{5, 0x6},
	{4, 0x5},
	{3, 0x4},
	{2, 0x2},
	{1, 0x1},
	{0, 0x0} };

/* SDIO Drive Strength to sel value table for PMU Rev 13 (3.3v) */

/** SDIO Drive Strength to sel value table for PMU Rev 17 (1.8v) */
static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab6_1v8)[] = {
	{3, 0x3},
	{2, 0x2},
	{1, 0x1},
	{0, 0x0} };


/**
 * SDIO Drive Strength to sel value table for 43143 PMU Rev 17, see Confluence 43143 Toplevel
 * architecture page, section 'PMU Chip Control 1 Register definition', click link to picture
 * BCM43143_sel_sdio_signals.jpg. Valid after PMU Chip Control 0 Register, bit31 (override) has
 * been written '1'.
 */
#if !defined(BCM_SDIO_VDDIO) || BCM_SDIO_VDDIO == 33

static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab7_3v3)[] = {
	/* note: for 14, 10, 6 and 2mA hw timing is not met according to rtl team */
	{16, 0x7},
	{12, 0x5},
	{8,  0x3},
	{4,  0x1} }; /* note: 43143 does not support tristate */

#else

static const sdiod_drive_str_t BCMINITDATA(sdiod_drive_strength_tab7_1v8)[] = {
	/* note: for 7, 5, 3 and 1mA hw timing is not met according to rtl team */
	{8, 0x7},
	{6, 0x5},
	{4,  0x3},
	{2,  0x1} }; /* note: 43143 does not support tristate */

#endif /* BCM_SDIO_VDDIO */

#define SDIOD_DRVSTR_KEY(chip, pmu)	(((chip) << 16) | (pmu))

/**
 * Balance between stable SDIO operation and power consumption is achieved using this function.
 * Note that each drive strength table is for a specific VDDIO of the SDIO pads, ideally this
 * function should read the VDDIO itself to select the correct table. For now it has been solved
 * with the 'BCM_SDIO_VDDIO' preprocessor constant.
 *
 * 'drivestrength': desired pad drive strength in mA. Drive strength of 0 requests tri-state (if
 *		    hardware supports this), if no hw support drive strength is not programmed.
 */
void
BCMINITFN(si_sdiod_drive_strength_init)(si_t *sih, osl_t *osh, uint32 drivestrength)
{
	sdiod_drive_str_t *str_tab = NULL;
	uint32 str_mask = 0;	/* only alter desired bits in PMU chipcontrol 1 register */
	uint32 str_shift = 0;
	uint32 str_ovr_pmuctl = PMU_CHIPCTL0; /* PMU chipcontrol register containing override bit */
	uint32 str_ovr_pmuval = 0;            /* position of bit within this register */
	pmuregs_t *pmu;
	uint origidx;

	if (!(sih->cccaps & CC_CAP_PMU)) {
		return;
	}
	BCM_REFERENCE(sdiod_drive_strength_tab1);
	BCM_REFERENCE(sdiod_drive_strength_tab2);
	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (SDIOD_DRVSTR_KEY(CHIPID(sih->chip), PMUREV(sih->pmurev))) {
	case SDIOD_DRVSTR_KEY(BCM4336_CHIP_ID, 8):
	case SDIOD_DRVSTR_KEY(BCM4336_CHIP_ID, 11):
		if (PMUREV(sih->pmurev) == 8) {
			str_tab = (sdiod_drive_str_t *)&sdiod_drive_strength_tab3;
		} else if (PMUREV(sih->pmurev) == 11) {
			str_tab = (sdiod_drive_str_t *)&sdiod_drive_strength_tab4_1v8;
		}
		str_mask = 0x00003800;
		str_shift = 11;
		break;
	case SDIOD_DRVSTR_KEY(BCM4330_CHIP_ID, 12):
		str_tab = (sdiod_drive_str_t *)&sdiod_drive_strength_tab4_1v8;
		str_mask = 0x00003800;
		str_shift = 11;
		break;
	case SDIOD_DRVSTR_KEY(BCM43362_CHIP_ID, 13):
		str_tab = (sdiod_drive_str_t *)&sdiod_drive_strength_tab5_1v8;
		str_mask = 0x00003800;
		str_shift = 11;
		break;
	case SDIOD_DRVSTR_KEY(BCM4334_CHIP_ID, 17):
		str_tab = (sdiod_drive_str_t *)&sdiod_drive_strength_tab6_1v8;
		str_mask = 0x00001800;
		str_shift = 11;
		break;
	case SDIOD_DRVSTR_KEY(BCM43143_CHIP_ID, 17):
#if !defined(BCM_SDIO_VDDIO) || BCM_SDIO_VDDIO == 33
		if (drivestrength >=  ARRAYLAST(sdiod_drive_strength_tab7_3v3)->strength) {
			str_tab = (sdiod_drive_str_t *)&sdiod_drive_strength_tab7_3v3;
		}
#else
		if (drivestrength >=  ARRAYLAST(sdiod_drive_strength_tab7_1v8)->strength) {
			str_tab = (sdiod_drive_str_t *)&sdiod_drive_strength_tab7_1v8;
		}
#endif /* BCM_SDIO_VDDIO */
		str_mask = 0x00000007;
		str_ovr_pmuval = PMU43143_CC0_SDIO_DRSTR_OVR;
		break;
	default:
		PMU_MSG(("No SDIO Drive strength init done for chip %s rev %d pmurev %d\n",
		         bcm_chipname(
			 CHIPID(sih->chip), chn, 8), CHIPREV(sih->chiprev), PMUREV(sih->pmurev)));
		break;
	}

	if (str_tab != NULL) {
		uint32 cc_data_temp;
		int i;

		/* Pick the lowest available drive strength equal or greater than the
		 * requested strength.	Drive strength of 0 requests tri-state.
		 */
		for (i = 0; drivestrength < str_tab[i].strength; i++)
			;

		if (i > 0 && drivestrength > str_tab[i].strength)
			i--;

		W_REG(osh, &pmu->chipcontrol_addr, PMU_CHIPCTL1);
		cc_data_temp = R_REG(osh, &pmu->chipcontrol_data);
		cc_data_temp &= ~str_mask;
		cc_data_temp |= str_tab[i].sel << str_shift;
		W_REG(osh, &pmu->chipcontrol_data, cc_data_temp);
		if (str_ovr_pmuval) { /* enables the selected drive strength */
			W_REG(osh,  &pmu->chipcontrol_addr, str_ovr_pmuctl);
			OR_REG(osh, &pmu->chipcontrol_data, str_ovr_pmuval);
		}
		PMU_MSG(("SDIO: %dmA drive strength requested; set to %dmA\n",
		         drivestrength, str_tab[i].strength));
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
} /* si_sdiod_drive_strength_init */

#if !defined(BCMDONGLEHOST)
/** initialize PMU */
void
BCMATTACHFN(si_pmu_init)(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if (PMUREV(sih->pmurev) == 1)
		AND_REG(osh, &pmu->pmucontrol, ~PCTL_NOILP_ON_WAIT);
	else if (PMUREV(sih->pmurev) >= 2)
		OR_REG(osh, &pmu->pmucontrol, PCTL_NOILP_ON_WAIT);

	/* Changes from PMU revision 26 are not included in revision 27 */
	if ((PMUREV(sih->pmurev) >= 26) && (PMUREV(sih->pmurev) != 27)) {
		uint32 val = PMU_INTC_ALP_REQ | PMU_INTC_HT_REQ | PMU_INTC_HQ_REQ;
		pmu_corereg(sih, SI_CC_IDX, pmuintctrl0, val, val);

		val = RSRC_INTR_MASK_TIMER_INT_0;
		pmu_corereg(sih, SI_CC_IDX, pmuintmask0, val, val);
		(void)pmu_corereg(sih, SI_CC_IDX, pmuintmask0, 0, 0);
	}
#ifdef REROUTE_OOBINT
	W_REG(osh, &pmu->pmuintmask0, 1);
#endif

	/* Return to original core */
	si_setcoreidx(sih, origidx);
}

uint32
si_pmu_rsrc_macphy_clk_deps(si_t *sih, osl_t *osh)
{
	uint32 deps;
	rsc_per_chip_t *rsc;
	uint origidx;
	pmuregs_t *pmu = NULL;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}

	ASSERT(pmu != NULL);

	rsc = si_pmu_get_rsc_positions(sih);
	deps = si_pmu_res_deps(sih, osh, pmu, PMURES_BIT(rsc->macphy_clkavail), FALSE);
	deps |= PMURES_BIT(rsc->macphy_clkavail);

	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return deps;
}

uint32
si_pmu_rsrc_ht_avail_clk_deps(si_t *sih, osl_t *osh)
{
	uint32 deps;
	rsc_per_chip_t *rsc;
	uint origidx;
	pmuregs_t *pmu = NULL;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}

	ASSERT(pmu != NULL);

	rsc = si_pmu_get_rsc_positions(sih);
	deps = si_pmu_res_deps(sih, osh, pmu, PMURES_BIT(rsc->ht_avail), FALSE);
	deps |= PMURES_BIT(rsc->ht_avail);

	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return deps;
}

static uint
BCMINITFN(si_pmu_res_uptime)(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint8 rsrc)
{
	uint32 deps;
	uint uptime, i, dup, dmax;
	uint32 min_mask = 0;
#ifndef SR_DEBUG
	uint32 max_mask = 0;
#endif /* SR_DEBUG */

	/* uptime of resource 'rsrc' */
	W_REG(osh, &pmu->res_table_sel, rsrc);
	if (PMUREV(sih->pmurev) >= 30)
		uptime = (R_REG(osh, &pmu->res_updn_timer) >> 16) & 0x7fff;
	else if (PMUREV(sih->pmurev) >= 13)
		uptime = (R_REG(osh, &pmu->res_updn_timer) >> 16) & 0x3ff;
	else
		uptime = (R_REG(osh, &pmu->res_updn_timer) >> 8) & 0xff;

	/* direct dependencies of resource 'rsrc' */
	deps = si_pmu_res_deps(sih, osh, pmu, PMURES_BIT(rsrc), FALSE);
	for (i = 0; i <= PMURES_MAX_RESNUM; i ++) {
		if (!(deps & PMURES_BIT(i)))
			continue;
		deps &= ~si_pmu_res_deps(sih, osh, pmu, PMURES_BIT(i), TRUE);
	}
#ifndef SR_DEBUG
	si_pmu_res_masks(sih, &min_mask, &max_mask);
#else
	/* Recalculate fast pwr up delay if min res mask/max res mask has changed */
	min_mask = R_REG(osh, &pmu->min_res_mask);
#endif /* SR_DEBUG */
	deps &= ~min_mask;

	/* max uptime of direct dependencies */
	dmax = 0;
	for (i = 0; i <= PMURES_MAX_RESNUM; i ++) {
		if (!(deps & PMURES_BIT(i)))
			continue;
		dup = si_pmu_res_uptime(sih, osh, pmu, (uint8)i);
		if (dmax < dup)
			dmax = dup;
	}

	PMU_MSG(("si_pmu_res_uptime: rsrc %u uptime %u(deps 0x%08x uptime %u)\n",
	         rsrc, uptime, deps, dmax));

	return uptime + dmax + PMURES_UP_TRANSITION;
}

/* Return dependencies (direct or all/indirect) for the given resources */
static uint32
si_pmu_res_deps(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint32 rsrcs, bool all)
{
	uint32 deps = 0;
	uint32 i;

	for (i = 0; i <= PMURES_MAX_RESNUM; i ++) {
		if (!(rsrcs & PMURES_BIT(i)))
			continue;
		W_REG(osh, &pmu->res_table_sel, i);
		deps |= R_REG(osh, &pmu->res_dep_mask);
	}

	return !all ? deps : (deps ? (deps | si_pmu_res_deps(sih, osh, pmu, deps, TRUE)) : 0);
}

/**
 * OTP is powered down/up as a means of resetting it, or for saving current when OTP is unused.
 * OTP is powered up/down through PMU resources.
 * OTP will turn OFF only if its not in the dependency of any "higher" rsrc in min_res_mask
 */
void
si_pmu_otp_power(si_t *sih, osl_t *osh, bool on, uint32* min_res_mask)
{
	pmuregs_t *pmu;
	uint origidx;
	uint32 rsrcs = 0;	/* rsrcs to turn on/off OTP power */
	rsc_per_chip_t *rsc;	/* chip specific resource bit positions */

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Don't do anything if OTP is disabled */
	if (si_is_otp_disabled(sih)) {
		PMU_MSG(("si_pmu_otp_power: OTP is disabled\n"));
		return;
	}

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	/*
	 * OTP can't be power cycled by toggling OTP_PU for always on OTP chips. For now
	 * corerev 45 is the only one that has always on OTP.
	 * Instead, the chipc register OTPCtrl1 (Offset 0xF4) bit 25 (forceOTPpwrDis) is used.
	 * Please refer to http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/ChipcommonRev45
	 */
	if (CCREV(sih->ccrev) == 45) {
		uint32 otpctrl1;
		otpctrl1 = si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, otpcontrol1), 0, 0);
		if (on)
			otpctrl1 &= ~OTPC_FORCE_PWR_OFF;
		else
			otpctrl1 |= OTPC_FORCE_PWR_OFF;
		si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, otpcontrol1), ~0, otpctrl1);
		/* Return to original core */
		si_setcoreidx(sih, origidx);
		return;
	}

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM4330_CHIP_ID:
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:	/* same OTP PU as 4350 */
	case BCM43430_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43012_CHIP_ID:
	case BCM43909_CHIP_ID:
	case BCM4364_CHIP_ID:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM43526_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM53573_CHIP_GRPID:
		rsc = si_pmu_get_rsc_positions(sih);
		rsrcs = PMURES_BIT(rsc->otp_pu);
		break;
	default:
		break;
	}

	if (rsrcs != 0) {
		uint32 otps;
		bool on_check = FALSE; /* Stores otp_ready state */
		uint32 min_mask = 0;

		/* Turn on/off the power */
		if (on) {
			min_mask = R_REG(osh, &pmu->min_res_mask);
			*min_res_mask = min_mask;

			min_mask |= rsrcs;
			min_mask |= si_pmu_res_deps(sih, osh, pmu, min_mask, TRUE);
			on_check = TRUE;
			/* Assuming max rsc mask defines OTP_PU, so not programming max */
			PMU_MSG(("Adding rsrc 0x%x to min_res_mask\n", min_mask));
			W_REG(osh, &pmu->min_res_mask, min_mask);
			si_pmu_wait_for_steady_state(sih, osh, pmu);
			OSL_DELAY(1000);
			SPINWAIT(!(R_REG(osh, &pmu->res_state) & rsrcs),
				PMU_MAX_TRANSITION_DLY);
			ASSERT(R_REG(osh, &pmu->res_state) & rsrcs);
		} else {
			/*
			 * Restore back the min_res_mask,
			 * but keep OTP powered off if allowed by dependencies
			 */
			if (*min_res_mask)
				min_mask = *min_res_mask;
			else
				min_mask = R_REG(osh, &pmu->min_res_mask);

			min_mask &= ~rsrcs;
			/*
			 * OTP rsrc can be cleared only if its not
			 * in the dependency of any "higher" rsrc in min_res_mask
			 */
			min_mask |= si_pmu_res_deps(sih, osh, pmu, min_mask, TRUE);
			on_check = ((min_mask & rsrcs) != 0);

			PMU_MSG(("Removing rsrc 0x%x from min_res_mask\n", min_mask));
			W_REG(osh, &pmu->min_res_mask, min_mask);
			si_pmu_wait_for_steady_state(sih, osh, pmu);
		}

		if (AOB_ENAB(sih)) {
			SPINWAIT((((otps =
				si_corereg(sih, si_findcoreidx(sih, GCI_CORE_ID, 0),
				OFFSETOF(gciregs_t, otpstatus), 0, 0))
				& OTPS_READY) != (on_check ? OTPS_READY : 0)), 3000);
		} else {
			SPINWAIT((((otps =
			si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, otpstatus), 0, 0))
			& OTPS_READY) != (on_check ? OTPS_READY : 0)), 3000);
		}
		ASSERT((otps & OTPS_READY) == (on_check ? OTPS_READY : 0));
		if ((otps & OTPS_READY) != (on_check ? OTPS_READY : 0))
			PMU_MSG(("OTP ready bit not %s after wait\n", (on_check ? "ON" : "OFF")));
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
} /* si_pmu_otp_power */

void
si_pmu_spuravoid(si_t *sih, osl_t *osh, uint8 spuravoid)
{
	pmuregs_t *pmu;
	uint origidx, intr_val = 0;
	uint32 min_res_mask = 0, max_res_mask = 0, clk_ctl_st = 0;
	bool pll_off_on = FALSE;

	ASSERT(CHIPID(sih->chip) != BCM43143_CHIP_ID); /* LCN40 PHY */

#ifdef BCMUSBDEV_ENABLED
	if ((CHIPID(sih->chip) == BCM4324_CHIP_ID) && (CHIPREV(sih->chiprev) <= 2)) {
		return;
	}
	/* spuravoid is not ready for 43242 */
	if ((CHIPID(sih->chip) == BCM43242_CHIP_ID) || (CHIPID(sih->chip) == BCM43243_CHIP_ID)) {
		return;
	}
#endif

	if ((CHIPID(sih->chip) == BCM4336_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43362_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43239_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4314_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43142_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4334_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43242_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43243_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43340_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43341_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4335_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4345_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43430_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43018_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43909_CHIP_ID) ||
		(BCM53573_CHIP(sih->chip)) ||
		(CHIPID(sih->chip) == BCM4330_CHIP_ID) ||
		(BCM4349_CHIP(sih->chip)))
	{
		pll_off_on = TRUE;
	}

	/* Block ints and save current core */
	intr_val = si_introff(sih);
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	/* force the HT off  */
	if (pll_off_on)
		si_pmu_pll_off(sih, osh, pmu, &min_res_mask, &max_res_mask, &clk_ctl_st);

	/* update the pll changes */
	si_pmu_spuravoid_pllupdate(sih, pmu, osh, spuravoid);

	/* enable HT back on  */
	if (pll_off_on)
		si_pmu_pll_on(sih, osh, pmu, min_res_mask, max_res_mask, clk_ctl_st);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	si_intrrestore(sih, intr_val);
} /* si_pmu_spuravoid */

/* below function are only for BBPLL parallel purpose */
/** only called for HT, LCN and N phy's. */
void
si_pmu_spuravoid_isdone(si_t *sih, osl_t *osh, uint32 min_res_mask,
uint32 max_res_mask, uint32 clk_ctl_st, uint8 spuravoid)
{
	pmuregs_t *pmu;
	uint origidx, intr_val = 0;
	bool pll_off_on = FALSE;

	ASSERT(CHIPID(sih->chip) != BCM43143_CHIP_ID); /* LCN40 PHY */

#ifdef BCMUSBDEV_ENABLED
	if ((CHIPID(sih->chip) == BCM4324_CHIP_ID) && (CHIPREV(sih->chiprev) <= 2)) {
		return;
	}
	/* spuravoid is not ready for 43242 */
	if ((CHIPID(sih->chip) == BCM43242_CHIP_ID) || (CHIPID(sih->chip) == BCM43243_CHIP_ID)) {
		return;
	}
#endif

	if ((CHIPID(sih->chip) == BCM4336_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43362_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43239_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4314_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43142_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4334_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43242_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43243_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4335_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4345_CHIP_ID) ||
		(BCM53573_CHIP(sih->chip)) ||
		(CHIPID(sih->chip) == BCM4330_CHIP_ID) ||
		(BCM4349_CHIP(sih->chip)))
	{
		pll_off_on = TRUE;
	}
	/* Block ints and save current core */
	intr_val = si_introff(sih);
	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if (pll_off_on)
		si_pmu_pll_off_isdone(sih, osh, pmu);
	/* update the pll changes */
	si_pmu_spuravoid_pllupdate(sih, pmu, osh, spuravoid);

	/* enable HT back on  */
	if (pll_off_on)
		si_pmu_pll_on(sih, osh, pmu, min_res_mask, max_res_mask, clk_ctl_st);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	si_intrrestore(sih, intr_val);
} /* si_pmu_spuravoid_isdone */

/* below function are only for BBPLL parallel purpose */

/* For having the pllcontrol data values for spuravoid */
typedef struct {
	uint8	spuravoid_mode;
	uint8	pllctrl_reg;
	uint32	pllctrl_regval;
} pllctrl_spuravoid_t;

/* LCNXN */
/* PLL Settings for spur avoidance on/off mode */
static const pllctrl_spuravoid_t spuravoid_4324[] = {
	{1, PMU1_PLL0_PLLCTL0, 0xA7400040},
	{1, PMU1_PLL0_PLLCTL1, 0x10080706},
	{1, PMU1_PLL0_PLLCTL2, 0x0D311408},
	{1, PMU1_PLL0_PLLCTL3, 0x804F66AC},
	{1, PMU1_PLL0_PLLCTL4, 0x02600004},
	{1, PMU1_PLL0_PLLCTL5, 0x00019AB1},

	{2, PMU1_PLL0_PLLCTL0, 0xA7400040},
	{2, PMU1_PLL0_PLLCTL1, 0x10080706},
	{2, PMU1_PLL0_PLLCTL2, 0x0D311408},
	{2, PMU1_PLL0_PLLCTL3, 0x80F3ADDC},
	{2, PMU1_PLL0_PLLCTL4, 0x02600004},
	{2, PMU1_PLL0_PLLCTL5, 0x00019AB1},

	{0, PMU1_PLL0_PLLCTL0, 0xA7400040},
	{0, PMU1_PLL0_PLLCTL1, 0x10080706},
	{0, PMU1_PLL0_PLLCTL2, 0x0CB11408},
	{0, PMU1_PLL0_PLLCTL3, 0x80AB1F7C},
	{0, PMU1_PLL0_PLLCTL4, 0x02600004},
	{0, PMU1_PLL0_PLLCTL5, 0x00019AB1}
};

static void
si_pmu_pllctrl_spurupdate(si_t *sih, osl_t *osh, pmuregs_t *pmu, uint8 spuravoid,
	const pllctrl_spuravoid_t *pllctrl_spur, uint32 array_size)
{
	uint8 indx;
	BCM_REFERENCE(sih);

	for (indx = 0; indx < array_size; indx++) {
		if (pllctrl_spur[indx].spuravoid_mode == spuravoid) {
			si_pmu_pllcontrol(sih, pllctrl_spur[indx].pllctrl_reg,
				~0, pllctrl_spur[indx].pllctrl_regval);
		}
	}
}

static void
si_pmu_spuravoid_pllupdate(si_t *sih, pmuregs_t *pmu, osl_t *osh, uint8 spuravoid)
{
	uint32 tmp = 0;
	uint32 xtal_freq, reg_val, mxdiv, ndiv_int, ndiv_frac_int, part_mul;
	uint8 p1_div, p2_div, FCLkx;
	const pmu1_xtaltab0_t *params_tbl;

	ASSERT(CHIPID(sih->chip) != BCM43143_CHIP_ID); /* LCN40 PHY */

	switch (CHIPID(sih->chip)) {
	case BCM4324_CHIP_ID:
		/* If we get an invalid spurmode, then set the spur0 settings. */
		if (spuravoid > 2)
			spuravoid = 0;

		si_pmu_pllctrl_spurupdate(sih, osh, pmu, spuravoid, spuravoid_4324,
			ARRAYSIZE(spuravoid_4324));

		tmp = PCTL_PLL_PLLCTL_UPD;
		break;

	case BCM5357_CHIP_ID:   case BCM4749_CHIP_ID:
	case BCM43235_CHIP_ID:	case BCM43236_CHIP_ID:	case BCM43238_CHIP_ID:
	case BCM43237_CHIP_ID:
	case BCM43234_CHIP_ID:
	case BCM6362_CHIP_ID:
	case BCM53572_CHIP_ID:
		if  ((CHIPID(sih->chip) == BCM6362_CHIP_ID) && (CHIPREV(sih->chiprev) == 0)) {
			/* 6362a0 (same clks as 4322[4-6]) */
			if (spuravoid == 1) {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11500010);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x000C0C06);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x0F600a08);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x2001E920);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888815);
			} else {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11100010);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x000c0c06);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x03000a08);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x200005c0);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888815);
			}
		} else {
			/* BCM5357 needs to touch PLL1_PLLCTL[02], so offset PLL0_PLLCTL[02] by 6 */
			const uint8 phypll_offset = ((CHIPID(sih->chip) == BCM5357_CHIP_ID) ||
			                             (CHIPID(sih->chip) == BCM4749_CHIP_ID) ||
			                             (CHIPID(sih->chip) == BCM53572_CHIP_ID))
			                               ? 6 : 0;
			const uint8 bcm5357_bcm43236_p1div[] = {0x1, 0x5, 0x5};
			const uint8 bcm5357_bcm43236_ndiv[] = {0x30, 0xf6, 0xfc};

			/* 5357[ab]0, 43236[ab]0, and 6362b0 */
			if (spuravoid > 2)
				spuravoid = 0;

			/* RMW only the P1 divider */
			tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0 + phypll_offset, 0, 0);
			tmp &= (~(PMU1_PLL0_PC0_P1DIV_MASK));
			tmp |= (bcm5357_bcm43236_p1div[spuravoid] << PMU1_PLL0_PC0_P1DIV_SHIFT);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0 + phypll_offset, ~0, tmp);

			/* RMW only the int feedback divider */
			tmp = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2 + phypll_offset, 0, 0);
			tmp &= ~(PMU1_PLL0_PC2_NDIV_INT_MASK);
			tmp |= (bcm5357_bcm43236_ndiv[spuravoid]) << PMU1_PLL0_PC2_NDIV_INT_SHIFT;
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2 + phypll_offset, ~0, tmp);
		}

		tmp = 1 << 10;
		break;

	case BCM4331_CHIP_ID:
	case BCM43431_CHIP_ID:
		if (ISSIM_ENAB(sih)) {
			if (spuravoid == 2) {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000002);
			} else if (spuravoid == 1) {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000001);
			} else {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
			}
		} else {
			if (spuravoid == 2) {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11500014);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x0FC00a08);
			} else if (spuravoid == 1) {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11500014);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x0F600a08);
			} else {
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11100014);
				si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x03000a08);
			}
		}
		tmp = 1 << 10;
		break;

	case BCM43224_CHIP_ID:	case BCM43225_CHIP_ID:	case BCM43421_CHIP_ID:
		if (spuravoid == 1) {
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11500010);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x000C0C06);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x0F600a08);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x2001E920);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888815);
		} else {
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11100010);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x000c0c06);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x03000a08);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x200005c0);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888815);
		}
		tmp = 1 << 10;
		break;

	case BCM43420_CHIP_ID:
		if (spuravoid == 1) {
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11500008);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x0C000C06);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x0F600a08);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x2001E920);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888815);
		} else {
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11100008);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x0c000c06);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x03000a08);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x200005c0);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888855);
		}

		tmp = 1 << 10;
		break;

	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM4330_CHIP_ID:
		xtal_freq = si_alp_clock(sih)/1000;
		/* Find the frequency in the table */
		for (params_tbl = si_pmu1_xtaltab0(sih);
			params_tbl != NULL && params_tbl->xf != 0; params_tbl++)
			if ((params_tbl->fref) == xtal_freq)
				break;
		/* Could not find it so assign a default value */
		if (params_tbl == NULL || params_tbl->xf == 0)
			params_tbl = si_pmu1_xtaldef0(sih);
		ASSERT(params_tbl != NULL && params_tbl->xf != 0);

		/* FClkx = (P2/P1) * ((NDIV_INT + NDIV_FRAC/2^24)/MXDIV) * Fref
		    Fref  = XtalFreq
		    FCLkx = 82MHz for spur avoidance mode
				   80MHz for non-spur avoidance mode.
		*/
		xtal_freq = (uint32) params_tbl->fref/100;
		p1_div = params_tbl->p1div;
		p2_div = params_tbl->p2div;
		reg_val = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		mxdiv = (reg_val >> PMU1_PLL0_PC2_M6DIV_SHIFT) & PMU1_PLL0_PC2_M5DIV_MASK;

		if (spuravoid == 1)
			FCLkx = 82;
		else
			FCLkx = 80;

		part_mul = (p1_div / p2_div) * mxdiv;
		ndiv_int = (FCLkx * part_mul * 10)/ (xtal_freq);
		ndiv_frac_int = ((FCLkx * part_mul * 10) % (xtal_freq));
		ndiv_frac_int = ((ndiv_frac_int * 16777216) + (xtal_freq/2)) / (xtal_freq);

		reg_val = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, PMU1_PLL0_PC2_NDIV_INT_MASK,
			ndiv_int << PMU1_PLL0_PC2_NDIV_INT_SHIFT);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, ndiv_frac_int);

		tmp = PCTL_PLL_PLLCTL_UPD;
		break;

	case BCM43131_CHIP_ID:
	case BCM43217_CHIP_ID:
	case BCM43227_CHIP_ID:
	case BCM43228_CHIP_ID:
	case BCM43428_CHIP_ID:
		/* LCNXN */
		/* PLL Settings for spur avoidance on/off mode, no on2 support for 43228A0 */
		if (spuravoid == 1) {
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x01100014);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x040C0C06);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x03140A08);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00333333);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x202C2820);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888815);
		} else {
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, 0x11100014);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, 0x040c0c06);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, 0x03000a08);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x00000000);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, 0x200005c0);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, 0x88888815);
		}
		tmp = 1 << 10;
		break;
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
		{
			const pmu2_xtaltab0_t *xt;
			uint32 pll0;

			xtal_freq = si_pmu_alp_clock(sih, osh)/1000;
			xt = (spuravoid == 1) ? pmu2_xtaltab0_adfll_492 : pmu2_xtaltab0_adfll_485;

			for (; xt != NULL && xt->fref != 0; xt++) {
				if (xt->fref == xtal_freq) {
					pll0 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, 0, 0);
					pll0 &= ~PMU15_PLL_PC0_FREQTGT_MASK;
					pll0 |= (xt->freq_tgt << PMU15_PLL_PC0_FREQTGT_SHIFT);
					pll0 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, pll0);
					tmp = PCTL_PLL_PLLCTL_UPD;
					break;
				}
			}
		}
		break;
	case BCM4335_CHIP_ID:
		/* 4335 PLL ctrl Registers */
		/* PLL Settings for spur avoidance on/off mode,  support for 4335A0 */
		/* # spur_mode 0 VCO=963MHz
		# spur_mode 1 VCO=960MHz
		# spur_mode 2 VCO=961MHz
		# spur_mode 3 VCO=964MHz
		# spur_mode 4 VCO=962MHz
		# spur_mode 5 VCO=965MHz
		# spur_mode 6 VCO=966MHz
		# spur_mode 7 VCO=967MHz
		# spur_mode 8 VCO=968MHz
		# spur_mode 9 VCO=969MHz
		*/
		switch (spuravoid) {
		case 0:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80BFA863);
			break;
		case 1:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80AB1F7D);
			break;
		case 2:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80b1f7c9);
			break;
		case 3:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80c680af);
			break;
		case 4:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80B8D016);
			break;
		case 5:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80CD58FC);
			break;
		case 6:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80D43149);
			break;
		case 7:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80DB0995);
			break;
		case 8:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80E1E1E2);
			break;
		case 9:
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, ~0, 0x80E8BA2F);
			break;
		default:
			break;
		}
		tmp = PCTL_PLL_PLLCTL_UPD;
		break;
	case BCM4345_CHIP_ID:
	case BCM43909_CHIP_ID:
		/* 4345 PLL ctrl Registers */
		xtal_freq = si_pmu_alp_clock(sih, osh) / 1000;
		/* TODO - set PLLCTL registers (via set_PLL_control_regs), based on spuravoid */
		break;
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
		{
			const pllctrl_data_t *pllctrlreg_update;
			const uint32 *pllctrlreg_val;
			uint8 indx, offset;
			uint8 pll_ctrlcnt = 0;
			uint32 array_size = 0;
			uint32 pllctrl2, pllctrl3;

			xtal_freq = si_alp_clock(sih)/1000;

			switch (spuravoid) {
				case 0:
					pllctrlreg_val = pmu1_pllctrl_tab_43430_972mhz;
					break;
				case 1:
					pllctrlreg_val = pmu1_pllctrl_tab_43430_980mhz;
					break;
				case 2:
					pllctrlreg_val = pmu1_pllctrl_tab_43430_984mhz;
					break;
				case 3:
					pllctrlreg_val = pmu1_pllctrl_tab_43430_326p4mhz;
					break;
				default:
					/* always set to 972 for invalid spuravoid */
					pllctrlreg_val = pmu1_pllctrl_tab_43430_972mhz;
					break;
			}

			if (PMUREV(sih->pmurev) >= 5) {
				pll_ctrlcnt = (sih->pmucaps & PCAP5_PC_MASK) >> PCAP5_PC_SHIFT;
			} else {
				pll_ctrlcnt = (sih->pmucaps & PCAP_PC_MASK) >> PCAP_PC_SHIFT;
			}

			pllctrl2 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
			pllctrl3 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0);

			pllctrlreg_update = pmu1_xtaltab0_43430;
			array_size = ARRAYSIZE(pmu1_xtaltab0_43430);

			for (indx = 0; indx < array_size; indx++) {
				/* If the entry does not match the xtal just continue the loop */
				if (!(pllctrlreg_update[indx].clock == (uint16)xtal_freq))
					continue;

				/* Change settings only if the new fvco is different than the
				*  current; fvco is computed based on fields in pllctrl2
				*  & pllctrl3.
				*/
				if (pllctrl2 != pllctrlreg_val[indx*pll_ctrlcnt + 2] ||
					pllctrl3 != pllctrlreg_val[indx*pll_ctrlcnt + 3]) {
					for (offset = 0; offset < pll_ctrlcnt; offset++) {
						si_pmu_pllcontrol(sih, offset, ~0,
							pllctrlreg_val[indx*pll_ctrlcnt + offset]);
					}

					tmp = PCTL_PLL_PLLCTL_UPD;
				}
				break;
			}
		}
		break;
	case BCM53573_CHIP_GRPID:
		printf("Error. spur table not populated\n");
		break;
	default:
		PMU_ERROR(("%s: unknown spuravoidance settings for chip %s, not changing PLL\n",
		           __FUNCTION__, bcm_chipname(CHIPID(sih->chip), chn, 8)));
		break;
	}

	tmp |= R_REG(osh, &pmu->pmucontrol);
	W_REG(osh, &pmu->pmucontrol, tmp);
} /* si_pmu_spuravoid_pllupdate */

extern uint32
si_pmu_cal_fvco(si_t *sih, osl_t *osh)
{
	uint32 xf, ndiv_int, ndiv_frac, fvco, pll_reg, p1_div_scale;
	uint32 r_high, r_low, int_part, frac_part, rounding_const;
	uint8 p1_div;
	chipcregs_t *cc;
	uint origidx, intr_val;

	/* Remember original core before switch to chipc */
	cc = (chipcregs_t *)si_switch_core(sih, CC_CORE_ID, &origidx, &intr_val);
	ASSERT(cc != NULL);
	BCM_REFERENCE(cc);

	xf = si_pmu_alp_clock(sih, osh)/1000;

	switch (CHIPID(sih->chip)) {
		case BCM4335_CHIP_ID:
		case BCM4364_CHIP_ID:
		case BCM4349_CHIP_GRPID:
		case BCM53573_CHIP_GRPID:
			pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);

			p1_div = (pll_reg & PMU4335_PLL0_PC2_P1DIV_MASK) >>
					PMU4335_PLL0_PC2_P1DIV_SHIFT;

			ndiv_int = (pll_reg & PMU4335_PLL0_PC2_NDIV_INT_MASK) >>
					PMU4335_PLL0_PC2_NDIV_INT_SHIFT;

			pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0);

			ndiv_frac = (pll_reg & PMU1_PLL0_PC3_NDIV_FRAC_MASK) >>
					PMU1_PLL0_PC3_NDIV_FRAC_SHIFT;
		break;

		case BCM43018_CHIP_ID:
		case BCM43430_CHIP_ID:

			pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);

			p1_div = (pll_reg & PMU43430_PLL0_PC2_P1DIV_MASK) >>
					PMU43430_PLL0_PC2_P1DIV_SHIFT;

			ndiv_int = (pll_reg & PMU43430_PLL0_PC2_NDIV_INT_MASK) >>
					PMU43430_PLL0_PC2_NDIV_INT_SHIFT;

			pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0);

			ndiv_frac = (pll_reg & PMU1_PLL0_PC3_NDIV_FRAC_MASK) >>
					PMU1_PLL0_PC3_NDIV_FRAC_SHIFT;
		break;
	default:
		PMU_MSG(("si_pmu_cal_fvco: Unknown chipid %s\n", bcm_chipname(sih->chip, chn, 8)));
		ASSERT(0);
		return 0;
	}

	/* Actual expression is as below */
	/* fvco1 = (100 * (xf * 1/p1_div) * (ndiv_int + (ndiv_frac * 1/(1 << 24)))) */
	/* * 1/(1000 * 100); */

	/* Representing 1/p1_div as a 12 bit number */
	/* Reason for the choice of 12: */
	/* ndiv_int is represented by 9 bits */
	/* so (ndiv_int << 24) needs 33 bits */
	/* xf needs 16 bits for the worst case of 52MHz clock */
	/* So (xf * (ndiv << 24)) needs atleast 49 bits */
	/* So remaining bits for uint64 : 64 - 49 = 15 bits */
	/* So, choosing 12 bits, with 3 bits of headroom */
	int_part = xf * ndiv_int;

	rounding_const = 1 << (BBPLL_NDIV_FRAC_BITS - 1);
	bcm_uint64_multiple_add(&r_high, &r_low, ndiv_frac, xf, rounding_const);
	bcm_uint64_right_shift(&frac_part, r_high, r_low, BBPLL_NDIV_FRAC_BITS);

	if (!p1_div) {
		PMU_ERROR(("p1_div calc returned 0! [%d]\n", __LINE__));
		ROMMABLE_ASSERT(0);
		fvco = 0;
		goto done;
	}

	p1_div_scale = (1 << P1_DIV_SCALE_BITS) / p1_div;
	rounding_const = 1 << (P1_DIV_SCALE_BITS - 1);

	bcm_uint64_multiple_add(&r_high, &r_low, (int_part + frac_part),
		p1_div_scale, rounding_const);
	bcm_uint64_right_shift(&fvco, r_high, r_low, P1_DIV_SCALE_BITS);
done:
	/* Return to original core */
	si_restore_core(sih, origidx, intr_val);
	return fvco;
} /* si_pmu_cal_fvco */

bool
si_pmu_is_otp_powered(si_t *sih, osl_t *osh)
{
	uint idx;
	pmuregs_t *pmu;
	bool st;
	rsc_per_chip_t *rsc;		/* chip specific resource bit positions */

	/* Remember original core before switch to chipc/pmu */
	idx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	si_pmu_wait_for_steady_state(sih, osh, pmu);

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	case BCM43239_CHIP_ID:
	case BCM4330_CHIP_ID:
	case BCM4314_CHIP_ID:
	case BCM43142_CHIP_ID:
	case BCM43143_CHIP_ID:
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	case BCM4335_CHIP_ID:
	case BCM4349_CHIP_GRPID:
	case BCM43430_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM43012_CHIP_ID:
	case BCM43909_CHIP_ID:
	case BCM4345_CHIP_ID:	/* same OTP PU as 4350 */
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	case BCM4360_CHIP_ID:
	case BCM43460_CHIP_ID:
	case BCM43526_CHIP_ID:
	case BCM4352_CHIP_ID:
	case BCM4347_CHIP_GRPID:
	case BCM4369_CHIP_ID:
	case BCM53573_CHIP_GRPID:
		rsc = si_pmu_get_rsc_positions(sih);
		st = (R_REG(osh, &pmu->res_state) & PMURES_BIT(rsc->otp_pu)) != 0;
		break;

	/* These chip doesn't use PMU bit to power up/down OTP. OTP always on.
	 * Use OTP_INIT command to reset/refresh state.
	 */
	case BCM43224_CHIP_ID:	case BCM43225_CHIP_ID:	case BCM43421_CHIP_ID:
	case BCM43236_CHIP_ID:	case BCM43235_CHIP_ID:	case BCM43238_CHIP_ID:
	case BCM43237_CHIP_ID:  case BCM43420_CHIP_ID:
	case BCM43234_CHIP_ID:
	case BCM4331_CHIP_ID:   case BCM43431_CHIP_ID:
	CASE_BCM43602_CHIP:
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM7271_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
		st = TRUE;
		break;
	default:
		st = TRUE;
		break;
	}

	/* Return to original core */
	si_setcoreidx(sih, idx);
	return st;
} /* si_pmu_is_otp_powered */

/**
 * Some chip/boards can be optionally fitted with an external 32Khz clock source for increased power
 * savings (due to more accurate sleep intervals).
 */
static void
BCMATTACHFN(si_pmu_set_lpoclk)(si_t *sih, osl_t *osh)
{
	uint32 ext_lpo_sel, int_lpo_sel, timeout = 0,
		ext_lpo_avail = 0, lpo_sel = 0;
	uint32 ext_lpo_isclock; /* On e.g. 43602a0, either x-tal or clock can be on LPO pins */
	pmuregs_t *pmu;
	uint origidx;

	if (!(getintvar(NULL, "boardflags3")))
		return;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	ext_lpo_sel = getintvar(NULL, "boardflags3") & BFL3_FORCE_EXT_LPO_SEL;
	int_lpo_sel = getintvar(NULL, "boardflags3") & BFL3_FORCE_INT_LPO_SEL;
	ext_lpo_isclock = getintvar(NULL, "boardflags3") & BFL3_EXT_LPO_ISCLOCK;

	BCM_REFERENCE(ext_lpo_isclock);

	if (ext_lpo_sel != 0) {
		switch (CHIPID(sih->chip)) {
		CASE_BCM43602_CHIP:
			/* External LPO is POR default enabled */
			si_pmu_chipcontrol(sih, PMU_CHIPCTL2, PMU43602_CC2_XTAL32_SEL,
				ext_lpo_isclock ? 0 : PMU43602_CC2_XTAL32_SEL);
			break;
		default:
			/* Force External LPO Power Up */
			si_pmu_chipcontrol(sih, PMU_CHIPCTL0, CC_EXT_LPO_PU, CC_EXT_LPO_PU);
			si_gci_chipcontrol(sih, CHIPCTRLREG6, GC_EXT_LPO_PU, GC_EXT_LPO_PU);
			break;
		}

		ext_lpo_avail = R_REG(osh, &pmu->pmustatus) & EXT_LPO_AVAIL;
		while (ext_lpo_avail == 0 && timeout < LPO_SEL_TIMEOUT) {
			OSL_DELAY(1000);
			ext_lpo_avail = R_REG(osh, &pmu->pmustatus) & EXT_LPO_AVAIL;
			timeout++;
		}

		if (timeout >= LPO_SEL_TIMEOUT) {
			PMU_ERROR(("External LPO is not available\n"));
		} else {
			/* External LPO is available, lets use (=select) it */
			OSL_DELAY(1000);
			timeout = 0;

			switch (CHIPID(sih->chip)) {
			CASE_BCM43602_CHIP:
				si_pmu_chipcontrol(sih, PMU_CHIPCTL2, PMU43602_CC2_FORCE_EXT_LPO,
					PMU43602_CC2_FORCE_EXT_LPO); /* switches to external LPO */
				break;
			default:
				/* Force External LPO Sel up */
				si_gci_chipcontrol(sih, CHIPCTRLREG6, EXT_LPO_SEL, EXT_LPO_SEL);
				/* Clear Force Internal LPO Sel */
				si_gci_chipcontrol(sih, CHIPCTRLREG6, INT_LPO_SEL, 0x0);
				OSL_DELAY(1000);

				lpo_sel = R_REG(osh, &pmu->pmucontrol) & LPO_SEL;
				while (lpo_sel != 0 && timeout < LPO_SEL_TIMEOUT) {
					OSL_DELAY(1000);
					lpo_sel = R_REG(osh, &pmu->pmucontrol) & LPO_SEL;
					timeout++;
				}
			}

			if (timeout >= LPO_SEL_TIMEOUT) {
				PMU_ERROR(("External LPO is not set\n"));
				/* Clear Force External LPO Sel */
				switch (CHIPID(sih->chip)) {
				CASE_BCM43602_CHIP:
					si_pmu_chipcontrol(sih, PMU_CHIPCTL2,
						PMU43602_CC2_FORCE_EXT_LPO, 0);
					break;
				default:
					si_gci_chipcontrol(sih, CHIPCTRLREG6, EXT_LPO_SEL, 0x0);
					break;
				}
			} else {
				/* Clear Force Internal LPO Power Up */
				switch (CHIPID(sih->chip)) {
				CASE_BCM43602_CHIP:
					break;
				default:
					si_pmu_chipcontrol(sih, PMU_CHIPCTL0, CC_INT_LPO_PU, 0x0);
					si_gci_chipcontrol(sih, CHIPCTRLREG6, GC_INT_LPO_PU, 0x0);
					break;
				}
			} /* if (timeout) */
		} /* if (timeout) */
	} else if (int_lpo_sel != 0) {
		switch (CHIPID(sih->chip)) {
		CASE_BCM43602_CHIP:
			break; /* do nothing, internal LPO is POR default powered and selected */
		default:
			/* Force Internal LPO Power Up */
			si_pmu_chipcontrol(sih, PMU_CHIPCTL0, CC_INT_LPO_PU, CC_INT_LPO_PU);
			si_gci_chipcontrol(sih, CHIPCTRLREG6, GC_INT_LPO_PU, GC_INT_LPO_PU);

			OSL_DELAY(1000);

			/* Force Internal LPO Sel up */
			si_gci_chipcontrol(sih, CHIPCTRLREG6, INT_LPO_SEL, INT_LPO_SEL);
			/* Clear Force External LPO Sel */
			si_gci_chipcontrol(sih, CHIPCTRLREG6, EXT_LPO_SEL, 0x0);

			OSL_DELAY(1000);

			lpo_sel = R_REG(osh, &pmu->pmucontrol) & LPO_SEL;
			timeout = 0;
			while (lpo_sel == 0 && timeout < LPO_SEL_TIMEOUT) {
				OSL_DELAY(1000);
				lpo_sel = R_REG(osh, &pmu->pmucontrol) & LPO_SEL;
				timeout++;
			}
			if (timeout >= LPO_SEL_TIMEOUT) {
				PMU_ERROR(("Internal LPO is not set\n"));
				/* Clear Force Internal LPO Sel */
				si_gci_chipcontrol(sih, CHIPCTRLREG6, INT_LPO_SEL, 0x0);
			} else {
				/* Clear Force External LPO Power Up */
				si_pmu_chipcontrol(sih, PMU_CHIPCTL0, CC_EXT_LPO_PU, 0x0);
				si_gci_chipcontrol(sih, CHIPCTRLREG6, GC_EXT_LPO_PU, 0x0);
			}
			break;
		}
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
} /* si_pmu_set_lpoclk */

/** Turn ON FAST LPO FLL (1MHz) */
static void
si_pmu_fast_lpo_enable(si_t *sih, osl_t *osh)
{
	int i = 0, lock = 0;

	BCM_REFERENCE(i);
	BCM_REFERENCE(lock);

	switch (CHIPID(sih->chip)) {
	case BCM43012_CHIP_ID:
		PMU_REG(sih, pmucontrol_ext, PCTL_EXT_FASTLPO_ENAB, PCTL_EXT_FASTLPO_ENAB);
		lock = CHIPC_REG(sih, chipstatus, 0, 0) & CST43012_FLL_LOCK;

		for (i = 0; ((i <= 30) && (!lock)); i++)
		{
			lock = CHIPC_REG(sih, chipstatus, 0, 0) & CST43012_FLL_LOCK;
			OSL_DELAY(10);
		}

		PMU_MSG(("%s: duration: %d\n", __FUNCTION__, i*10));

		if (!lock) {
			PMU_MSG(("%s: FLL lock not present!", __FUNCTION__));
			ROMMABLE_ASSERT(0);
		}

		/* Now switch to using FAST LPO clk */
		PMU_REG(sih, pmucontrol_ext, PCTL_EXT_FASTLPO_SWENAB, PCTL_EXT_FASTLPO_SWENAB);
		break;
	default:
		PMU_MSG(("%s: LPO enable: unsupported chip!\n", __FUNCTION__));
	}
}

/** initialize PMU chip controls and other chip level stuff */
void
BCMATTACHFN(si_pmu_chip_init)(si_t *sih, osl_t *osh)
{
	ASSERT(sih->cccaps & CC_CAP_PMU);

	si_pmu_otp_chipcontrol(sih, osh);

#ifdef CHIPC_UART_ALWAYS_ON
	si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), CCS_FORCEALP, CCS_FORCEALP);
#endif /* CHIPC_UART_ALWAYS_ON */


	/* Misc. chip control, has nothing to do with PMU */
	switch (CHIPID(sih->chip)) {

	case BCM4324_CHIP_ID:
	case BCM43242_CHIP_ID:
	case BCM43243_CHIP_ID:
	{
#if defined(BCM4324A0) || defined(BCM4324A1)
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1,
			(PMU4324_CC1_GPIO_CONF(-1) | PMU4324_CC1_ENABLE_UART),
			(PMU4324_CC1_GPIO_CONF(0) | PMU4324_CC1_ENABLE_UART));
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, PMU4324_CC2_SDIO_AOS_EN,
			PMU4324_CC2_SDIO_AOS_EN);
#endif

#ifdef BCM4324B0
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1,
		PMU4324_CC1_ENABLE_UART,
		0);
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2,
		PMU4324_CC2_SDIO_AOS_EN,
		PMU4324_CC2_SDIO_AOS_EN);
#endif

#if defined(BCM4324A1) || defined(BCM4324B0)

		/* 4324 iPA Board hence muxing out PALDO_PU signal to O/p pin &
		 * muxing out actual RF_SW_CTRL7-6 signals to O/p pin
		 */

		si_pmu_chipcontrol(sih, PMU_CHIPCTL4, 0x1f0000, 0x140000);
#endif
		/*
		 * Setting the pin mux to enable the GPIOs required for HSIC-OOB signals.
		 */
#if defined(BCM4324B1)
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1, PMU4324_CC1_GPIO_CONF_MASK, 0x2);
#endif
		break;
	}
	case BCM43012_CHIP_ID:
	{
#ifdef USE_LHL_TIMER
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, PMUCCTL02_43012_LHL_TIMER_SELECT,
			PMUCCTL02_43012_LHL_TIMER_SELECT);
#else
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, PMUCCTL02_43012_LHL_TIMER_SELECT, 0);
#endif /* USE_LHL_TIMER */

		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, PMUCCTL02_43012_RFLDO3P3_PU_FORCE_ON, 0);
		si_pmu_chipcontrol(sih, PMU_CHIPCTL4, PMUCCTL14_43012_DISABLE_LQ_AVAIL, 0);


		PMU_REG_NEW(sih, extwakemask[0],
				PMU_EXT_WAKE_MASK_0_SDIO, PMU_EXT_WAKE_MASK_0_SDIO);
		PMU_REG_NEW(sih, extwakereqmask[0], ~0, si_pmu_rsrc_ht_avail_clk_deps(sih, osh));

		if (sih->lpflags & LPFLAGS_SI_FORCE_PWM_WHEN_RADIO_ON) {
			/* Force PWM when Radio ON */
			/* 2G_Listen/2G_RX/2G_TX/5G_Listen/5G_RX/5G_TX = PWM */
			si_pmu_vreg_control(sih, PMU_VREG_8,
				PMU_43012_VREG8_DYNAMIC_CBUCK_MODE_MASK,
				PMU_43012_VREG8_DYNAMIC_CBUCK_MODE0);
			si_pmu_vreg_control(sih, PMU_VREG_9,
				PMU_43012_VREG9_DYNAMIC_CBUCK_MODE_MASK,
				PMU_43012_VREG9_DYNAMIC_CBUCK_MODE0);
		}
		else {
			/* 2G_Listen/2G_RX = LPPFM, 2G_TX/5G_Listen/5G_RX/5G_TX = PWM */
			si_pmu_vreg_control(sih, PMU_VREG_8,
				PMU_43012_VREG8_DYNAMIC_CBUCK_MODE_MASK,
				PMU_43012_VREG8_DYNAMIC_CBUCK_MODE1);
			si_pmu_vreg_control(sih, PMU_VREG_9,
				PMU_43012_VREG9_DYNAMIC_CBUCK_MODE_MASK,
				PMU_43012_VREG9_DYNAMIC_CBUCK_MODE1);
		}
#ifdef BCM43012
		/* Set external LPO */
		si_lhl_set_lpoclk(sih, osh);
#endif
		/* Enabling WL2CDIG sleep */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, PMUCCTL02_43012_WL2CDIG_I_PMU_SLEEP_ENAB,
			PMUCCTL02_43012_WL2CDIG_I_PMU_SLEEP_ENAB);

		si_pmu_chipcontrol(sih, PMU_CHIPCTL9,
			PMUCCTL09_43012_XTAL_CORESIZE_BIAS_ADJ_STARTUP_MASK,
			PMUCCTL09_43012_XTAL_CORESIZE_BIAS_ADJ_STARTUP_VAL <<
				PMUCCTL09_43012_XTAL_CORESIZE_BIAS_ADJ_STARTUP_SHIFT);

		/* Setting MemLPLDO voltage to 0.74 */
		si_pmu_vreg_control(sih, PMU_VREG_6, VREG6_43012_MEMLPLDO_ADJ_MASK,
			0x8 << VREG6_43012_MEMLPLDO_ADJ_SHIFT);

		/* Setting LPLDO voltage to 0.8 */
		si_pmu_vreg_control(sih, PMU_VREG_6, VREG6_43012_LPLDO_ADJ_MASK,
			0xB << VREG6_43012_LPLDO_ADJ_SHIFT);

		/* Turn off power switch 1P8 in sleep */
		si_pmu_vreg_control(sih, PMU_VREG_7, VREG7_43012_PWRSW_1P8_PU_MASK, 0);

		/* Enable PMU sleep mode0 (DS0-PS0) */
		LHL_REG(sih, lhl_top_pwrseq_ctl_adr, ~0, PMU_43012_SLEEP_MODE_0);

		si_pmu_fast_lpo_enable(sih, osh);

		/* Enable the 'power kill' (power off selected retention memories) */
		GCI_REG_NEW(sih, bt_smem_control0, GCI_BT_SMEM_CTRL0_SUBCORE_ENABLE_PKILL,
			GCI_BT_SMEM_CTRL0_SUBCORE_ENABLE_PKILL);

		break;
	}
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	{
		uint32 mask, val = 0;
		uint16 clkreq_conf;
		mask = (1 << PMU_CC1_CLKREQ_TYPE_SHIFT);

		clkreq_conf = (uint16)getintvar(NULL, rstr_clkreq_conf);

		if (clkreq_conf & CLKREQ_TYPE_CONFIG_PUSHPULL)
			val =  (1 << PMU_CC1_CLKREQ_TYPE_SHIFT);

		si_pmu_chipcontrol(sih, PMU_CHIPCTL0, mask, val);
		break;
	}


	case BCM4349_CHIP_GRPID:
		{
		uint32 val;

		/* JIRA: SWWLAN-27305 initialize 4349 pmu control registers */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1,
			PMU_CC1_ENABLE_BBPLL_PWR_DOWN, PMU_CC1_ENABLE_BBPLL_PWR_DOWN);

		if ((CHIPID(sih->chip) != BCM4373_CHIP_ID) && CST4349_CHIPMODE_PCIE(sih->chipst)) {
			/* JIRA: SWWLAN-27486 optimize power consumption when wireless is down */
			if (sih->chiprev == 0) { /* 4349A0 */
				val = PMU_CC2_FORCE_SUBCORE_PWR_SWITCH_ON |
				  PMU_CC2_FORCE_PHY_PWR_SWITCH_ON |
				  PMU_CC2_FORCE_VDDM_PWR_SWITCH_ON |
				  PMU_CC2_FORCE_MEMLPLDO_PWR_SWITCH_ON;
				si_pmu_chipcontrol(sih, PMU_CHIPCTL2, val, val);

				val = PMU_CC6_ENABLE_CLKREQ_WAKEUP |
				  PMU_CC6_ENABLE_PMU_WAKEUP_ALP;
				si_pmu_chipcontrol(sih, PMU_CHIPCTL6, val, val);
			}
		}
		/* phy_pwrse_reset_count */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, CC2_4349_PHY_PWRSE_RST_CNT_MASK,
			(0xa << CC2_4349_PHY_PWRSE_RST_CNT_SHIFT));
#if !defined(USE_MEMLPLDO)
		/* Setting VDDM */
		si_pmu_vreg_control(sih, PMU_VREG_4, (VREG4_4349_MEMLPLDO_PWRUP_MASK |
			VREG4_4349_LPLDO1_OUTPUT_VOLT_ADJ_MASK),
			((0 << VREG4_4349_MEMLPLDO_PWRUP_SHIFT) |
			(0 << VREG4_4349_LPLDO1_OUTPUT_VOLT_ADJ_SHIFT)));
		/* Force VDDM power-switch setting */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, (CC2_4349_VDDM_PWRSW_EN_MASK |
			CC2_4349_MEMLPLDO_PWRSW_EN_MASK),
			((1 << CC2_4349_VDDM_PWRSW_EN_SHIFT) |
			(0 << CC2_4349_MEMLPLDO_PWRSW_EN_SHIFT)));
#else
		/* Setting MEMLPLDO */
		si_pmu_vreg_control(sih, PMU_VREG_4, (VREG4_4349_MEMLPLDO_PWRUP_MASK |
			VREG4_4349_LPLDO1_OUTPUT_VOLT_ADJ_MASK),
			((1 << VREG4_4349_MEMLPLDO_PWRUP_SHIFT) |
			(0x6 << VREG4_4349_LPLDO1_OUTPUT_VOLT_ADJ_SHIFT)));
		/* Force MEMLPLDO power-switch setting */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, (CC2_4349_VDDM_PWRSW_EN_MASK |
			CC2_4349_MEMLPLDO_PWRSW_EN_MASK),
			((0 << CC2_4349_VDDM_PWRSW_EN_SHIFT) |
			(1 << CC2_4349_MEMLPLDO_PWRSW_EN_SHIFT)));
#endif /* !defined(USE_MEMLPLDO) */
		/* HW4349-248 Use clock requests from cores directly in clkrst to control clocks to
		 * turn on clocks faster
		 */
		 si_pmu_chipcontrol(sih, PMU_CHIPCTL6, (1 << 18), (1 << 18));
		/* HW4349-247 Make "higher" rsrc requests (like HT_AVAIL) requests for "lower" rsrc
		 * (like ALP_AVAIL)
		 */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL6,
			((1 << 20) | (1 << 21) | (1<< 22)),
			((1 << 20) | (1 << 21) | (1 << 22)));
		/* Set internal/external LPO */
		si_pmu_set_lpoclk(sih, osh);
		break;
	}

	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
	{
		uint32 val;

		if (CST4350_IFC_MODE(sih->chipst) == CST4350_IFC_MODE_PCIE) {
			/* JIRA: SWWLAN-27305 initialize 4350 pmu control registers */
			si_pmu_chipcontrol(sih, PMU_CHIPCTL1,
				PMU_CC1_ENABLE_BBPLL_PWR_DOWN, PMU_CC1_ENABLE_BBPLL_PWR_DOWN);
			si_pmu_vreg_control(sih, PMU_VREG_0, ~0, 1);

			/* JIRA:SWWLAN-36186; HW4345-889 */
			si_pmu_chipcontrol(sih, PMU_CHIPCTL5, CC5_4350_PMU_EN_ASSERT_MASK,
				CC5_4350_PMU_EN_ASSERT_MASK);

			/* JIRA: SWWLAN-27486 optimize power consumption when wireless is down */
			if ((CHIPID(sih->chip) == BCM4350_CHIP_ID) &&
				(CHIPREV(sih->chiprev) == 0)) { /* 4350A0 */
				val = PMU_CC2_FORCE_SUBCORE_PWR_SWITCH_ON |
				      PMU_CC2_FORCE_PHY_PWR_SWITCH_ON |
				      PMU_CC2_FORCE_VDDM_PWR_SWITCH_ON |
				      PMU_CC2_FORCE_MEMLPLDO_PWR_SWITCH_ON;
				si_pmu_chipcontrol(sih, PMU_CHIPCTL2, val, val);

				val = PMU_CC6_ENABLE_CLKREQ_WAKEUP |
				      PMU_CC6_ENABLE_PMU_WAKEUP_ALP;
				si_pmu_chipcontrol(sih, PMU_CHIPCTL6, val, val);
			}
		}
		/* Set internal/external LPO */
		si_pmu_set_lpoclk(sih, osh);
		break;
	}
	case BCM4335_CHIP_ID:
	case BCM4345_CHIP_ID:
	CASE_BCM43602_CHIP: /* fall through */
	case BCM43430_CHIP_ID: /* fall through */
	case BCM43018_CHIP_ID: /* fall through */
		/* Set internal/external LPO */
		si_pmu_set_lpoclk(sih, osh);
		break;
	case BCM4364_CHIP_ID:
	{
		si_pmu_chipcontrol(sih, PMU_CHIPCTL1, (PMU_CC1_ENABLE_BBPLL_PWR_DOWN |
			PMU_CC1_ENABLE_CLOSED_LOOP_MASK),
			(PMU_CC1_ENABLE_BBPLL_PWR_DOWN |
			PMU_CC1_ENABLE_CLOSED_LOOP));

		si_pmu_chipcontrol(sih, PMU_CHIPCTL2, (PMU_4364_CC2_PHY_PWRSW_RESET_MASK |
			PMU_4364_CC2_SEL_CHIPC_IF_FOR_SR),
			(PMU_4364_CC2_PHY_PWRSW_RESET_CNT |
			PMU_4364_CC2_SEL_CHIPC_IF_FOR_SR));

		si_pmu_chipcontrol(sih, PMU_CHIPCTL3, (PMU_4364_CC3_MEMLPLDO3x3_PWRSW_FORCE_MASK |
			PMU_4364_CC3_MEMLPLDO1x1_PWRSW_FORCE_MASK |
			PMU_4364_CC3_CBUCK1P2_PU_SR_VDDM_REQ_ON),
			(PMU_4364_CC3_CBUCK1P2_PU_SR_VDDM_REQ_ON |
			PMU_4364_CC3_MEMLPLDO3x3_PWRSW_FORCE_OFF |
			PMU_4364_CC3_MEMLPLDO1x1_PWRSW_FORCE_OFF));

		si_pmu_chipcontrol(sih, PMU_CHIPCTL5,
			(PMU_4364_CC5_DISABLE_BBPLL_CLKOUT6_DIV2_MASK |
			PMU_4364_CC5_ENABLE_ARMCR4_DEBUG_CLK_MASK),
			(PMU_4364_CC5_DISABLE_BBPLL_CLKOUT6_DIV2 |
			PMU_4364_CC5_ENABLE_ARMCR4_DEBUG_CLK_OFF));

		si_pmu_chipcontrol(sih, PMU_CHIPCTL6, (PMU_4364_CC6_USE_CLK_REQ_MASK |
			PMU_4364_CC6_HIGHER_CLK_REQ_ALP_MASK |
			PMU_4364_CC6_HT_AVAIL_REQ_ALP_AVAIL_MASK |
			PMU_4364_CC6_PHY_CLK_REQUESTS_ALP_AVAIL_MASK),
			(PMU_4364_CC6_USE_CLK_REQ |
			PMU_4364_CC6_HIGHER_CLK_REQ_ALP |
			PMU_4364_CC6_HT_AVAIL_REQ_ALP_AVAIL |
			PMU_4364_CC6_PHY_CLK_REQUESTS_ALP_AVAIL));

		si_pmu_chipcontrol(sih, PMU_CHIPCTL6, (PMU_CC6_ENABLE_CLKREQ_WAKEUP |
			PMU_CC6_ENABLE_PMU_WAKEUP_ALP | PMU_4364_CC6_MDI_RESET_MASK),
			(PMU_CC6_ENABLE_CLKREQ_WAKEUP |
			PMU_CC6_ENABLE_PMU_WAKEUP_ALP |
			PMU_4364_CC6_MDI_RESET));

		si_pmu_vreg_control(sih, PMU_VREG_0, (PMU_4364_VREG0_DISABLE_BT_PULL_DOWN),
			(PMU_4364_VREG0_DISABLE_BT_PULL_DOWN));

		si_pmu_vreg_control(sih, PMU_VREG_1, (PMU_4364_VREG1_DISABLE_WL_PULL_DOWN),
			(PMU_4364_VREG1_DISABLE_WL_PULL_DOWN));

		si_pmu_vreg_control(sih, PMU_VREG_3, (PMU_4364_VREG3_DISABLE_WPT_REG_ON_PULL_DOWN),
			(PMU_4364_VREG3_DISABLE_WPT_REG_ON_PULL_DOWN));

		si_pmu_vreg_control(sih, PMU_VREG_4, (PMU_4364_VREG4_MEMLPLDO_PU_ON |
			PMU_4364_VREG4_LPLPDO_ADJ_MASK),
			(PMU_4364_VREG4_MEMLPLDO_PU_ON |
			PMU_4364_VREG4_LPLPDO_ADJ));


		si_pmu_vreg_control(sih, PMU_VREG_5, (PMU_4364_VREG5_MAC_CLK_1x1_AUTO |
			PMU_4364_VREG5_SR_AUTO |
			PMU_4364_VREG5_BT_AUTO |
			PMU_4364_VREG5_BT_PWM_MASK |
			PMU_4364_VREG5_WL2CLB_DVFS_EN_MASK),
			(PMU_4364_VREG5_MAC_CLK_1x1_AUTO |
			PMU_4364_VREG5_SR_AUTO |
			PMU_4364_VREG5_BT_AUTO |
			PMU_4364_VREG5_BT_PWMK |
			PMU_4364_VREG5_WL2CLB_DVFS_EN));

		si_pmu_vreg_control(sih, PMU_VREG_6, ~0, (PMU_4364_VREG6_BBPLL_AUTO) |
			(PMU_4364_VREG6_MINI_PMU_PWM) |
			(PMU_4364_VREG6_LNLDO_AUTO) |
			(PMU_4364_VREG6_PCIE_PWRDN_0_AUTO) |
			(PMU_4364_VREG6_PCIE_PWRDN_1_AUTO) |
			(PMU_4364_VREG6_MAC_CLK_3x3_PWM) |
			(PMU_4364_VREG6_ENABLE_FINE_CTRL));

		si_pmu_vreg_control(sih, PMU_VREG_8, ~0, 0x31001);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, PMU_4364_PLL0_DISABLE_CHANNEL6,
			PMU_4364_PLL0_DISABLE_CHANNEL6);
		si_gci_chipcontrol(sih, CC_GCI1_REG, ~0, CC_GCI1_4364_IND_STATE_FOR_GPIO9_11);

		break;
	}
	case BCM53573_CHIP_GRPID:
		if (CST53573_CHIPMODE_PCIE(sih->chipst)) {
			/* Refer 4349 code above - BBPLL is pwred down */
			si_pmu_chipcontrol(sih, PMU_53573_CHIPCTL3,
				PMU_53573_CC3_ENABLE_BBPLL_PWRDOWN_MASK,
				PMU_53573_CC3_ENABLE_BBPLL_PWRDOWN);
		}
		/* HW4349-248 Use clock requests from cores directly in clkrst to control clocks to
		 * turn on clocks faster
		 */
		si_pmu_chipcontrol(sih, PMU_53573_CHIPCTL1,
			PMU_53573_CC1_HT_CLK_REQ_CTRL_MASK, PMU_53573_CC1_HT_CLK_REQ_CTRL);
		/* Set internal/external LPO */
		si_pmu_set_lpoclk(sih, osh);
		break;
	default:
		break;
	}
} /* si_pmu_chip_init */

/** Reference: http://confluence.broadcom.com/display/WLAN/Open+loop+Calibration+Sequence */
int
si_pmu_openloop_cal(si_t *sih, uint16 currtemp)
{
	int err = BCME_OK;
	switch (CHIPID(sih->chip)) {
	case BCM43012_CHIP_ID:
		err = si_pmu_openloop_cal_43012(sih, currtemp);
		break;

	default:
		PMU_MSG(("%s: chip not supported!\n", __FUNCTION__));
		break;
	}
	return err;
}

static int
si_pmu_openloop_cal_43012(si_t *sih, uint16 currtemp)
{
	int32 a1 = -27, a2 = -15, b1 = 18704, b2 = 7531, a3, y1, y2, b3, y3;
	int32 xtal, array_size = 0, dco_code = 0, origidx = 0, intr_val = 0, pll_reg = 0, err;
	pmuregs_t *pmu = NULL;
	const pllctrl_data_t *pllctrlreg_update;
	const uint32 *pllctrlreg_val;
	osl_t *osh = si_osh(sih);
	uint32 final_dco_code = si_get_openloop_dco_code(sih);

	xtal = si_xtalfreq(sih);
	err = BCME_OK;

	origidx = si_coreidx(sih);
	pmu = si_setcore(sih, PMU_CORE_ID, 0);
	if (!pmu) {
		PMU_MSG(("%s: NULL pmu pointer \n", __FUNCTION__));
		err = BCME_ERROR;
		goto done;
	}

	if (final_dco_code == 0) {
		currtemp = (currtemp == 0)?-1: currtemp;

		SPINWAIT(((si_corereg(sih, SI_CC_IDX,
			OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL) != CCS_HTAVAIL), PMU_MAX_TRANSITION_DLY);
		ASSERT((si_corereg(sih, SI_CC_IDX, OFFSETOF(chipcregs_t, clk_ctl_st), 0, 0)
			& CCS_HTAVAIL));

		/* Stop using PLL clks, by programming the disable_ht_avail */
		/* and disable_lq_avail in the pmu chip control bit */
		/* Turn Off PLL */
		si_pmu_pll_off_43012(sih, osh, pmu);

		/* Program PLL for 320MHz VCO */
		pllctrlreg_update = pmu1_xtaltab0_43012;
		array_size = ARRAYSIZE(pmu1_xtaltab0_43012);
		pllctrlreg_val = pmu1_pllctrl_tab_43012_1600mhz;
		si_pmu_pllctrlreg_update(sih, osh, pmu, xtal, 100,
			pllctrlreg_update, array_size, pllctrlreg_val);

		/* Update PLL control register */
		/* Set the Update bit (bit 10) in PMU for PLL registers */
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);

		/* Turn PLL ON but ensure that force_bbpll_dreset */
		/* bit is set , so that PLL 320Mhz clocks cannot be consumed */
		si_pmu_pll_on_43012(sih, osh, pmu, 1);

		/* Settings to get dco_code on PLL test outputs and then read */
		/* from gci chip status */
		pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0);
		pll_reg = (pll_reg & (~0x3C000)) | (0x4<<14);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, pll_reg);
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);

		pll_reg = pll_reg | (1<<17);
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, pll_reg);
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);

		/* Get the DCO code from GCI CHIP STATUS Register 7 , bits 27 downto 16 */
		dco_code = (si_gci_chipstatus(sih, GCI_CHIPSTATUS_07));
		dco_code = ((dco_code & 0x0FFF0000) >> 16);
		dco_code = (dco_code  >> 4);

		/* The DCO code obtained above and the temperature */
		/* sensed at this time will give us the DCO code */
		/* that needs to be programmed to ensure VCO does not crosses 160 MHz at 125C */
		y1 = ((a1 * currtemp) + b1);
		y2 = ((a2 * currtemp) + b2);
		dco_code = (dco_code * 100);
		b3 = b1 + (((b2-b1)/(y2 - y1)) * (dco_code - y1));
		if (b3 > dco_code) {
			a3 = (b3 - dco_code) / currtemp;
			y3 = b3 - (a3 * 125);
		}
		else {
			 a3 = (dco_code - b3) / currtemp;
			 y3 = b3 + (a3 * 125);
		}
		y3 = (y3/100);
		PMU_MSG(("DCO_CODE = %d\n", y3));

		/* Turning ON PLL at 160.1 MHz for Normal Operation */
		si_pmu_pll_off_43012(sih, osh, pmu);
		pllctrlreg_update = pmu1_xtaltab0_43012;
		array_size = ARRAYSIZE(pmu1_xtaltab0_43012);
		pllctrlreg_val = pmu1_pllctrl_tab_43012_1600mhz;
		si_pmu_pllctrlreg_update(sih, osh, pmu, xtal, 0,
			pllctrlreg_update, array_size, pllctrlreg_val);

		si_pmu_pll_on_43012(sih, osh, pmu, 0);
		y3 = (y3 << 4);
		final_dco_code = y3;
		PMU_MSG(("openloop_dco_code = %x\n", final_dco_code));
	}

	pll_reg = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0, 0);
	y3 = (pll_reg >> 16) & 0xFFF;

	if (final_dco_code != y3) {

		/* Program the DCO code to bits 27 */
		/* downto 16 of the PLL control 3 register */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3,
			0x0FFF0000, (final_dco_code << 16));

		/* Enable Extra post divison for Open Loop */
		/* by writing 1 to bit 14 of above register */
		si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG3, 0x00004000, (1<<14));

		/* Update PLL control register */
		/* Set the Update bit (bit 10) in PMU for PLL registers */
		OR_REG(osh, &pmu->pmucontrol, PCTL_PLL_PLLCTL_UPD);
		/* After cal openloop VCO Max=320MHz, Min=240Mhz (with extra margin */
		/* 230-220MHz). Update SAVE_RESTORE up/down times accordingly */
		W_REG(osh, &pmu->res_table_sel,	RES43012_SR_SAVE_RESTORE);
		W_REG(osh, &pmu->res_updn_timer, 0x01000100);
	}

	si_restore_core(sih, origidx, intr_val);
	si_set_openloop_dco_code(sih, final_dco_code);
done:
	return err;
}

void
si_pmu_slow_clk_reinit(si_t *sih, osl_t *osh)
{
#if !defined(BCMDONGLEHOST)
	chipcregs_t *cc;
	uint origidx;
	uint32 xtalfreq, mode;

	/* PMU specific initializations */
	if (!PMUCTL_ENAB(sih))
		return;
	/* Remember original core before switch to chipc */
	origidx = si_coreidx(sih);
	cc = si_setcoreidx(sih, SI_CC_IDX);
	ASSERT(cc != NULL);
	if (cc == NULL)
		return;

	xtalfreq = getintvar(NULL, rstr_xtalfreq);
	switch (CHIPID(sih->chip)) {
		case BCM43242_CHIP_ID:
		case BCM43243_CHIP_ID:
			xtalfreq = 37400;
			break;
		case BCM43143_CHIP_ID:
			xtalfreq = 20000;
			break;
		CASE_BCM43602_CHIP:
		case BCM4365_CHIP_ID:
		case BCM4366_CHIP_ID:
		case BCM43664_CHIP_ID:
	        case BCM43666_CHIP_ID:
			xtalfreq = 40000;
			break;
		case BCM7271_CHIP_ID:
			xtalfreq = XTAL_FREQ_54MHZ;
			break;
		case BCM4350_CHIP_ID:
		case BCM4354_CHIP_ID:
		case BCM43556_CHIP_ID:
		case BCM43558_CHIP_ID:
		case BCM43566_CHIP_ID:
		case BCM43568_CHIP_ID:
		case BCM43569_CHIP_ID:
		case BCM43570_CHIP_ID:
		case BCM4358_CHIP_ID:
			if (xtalfreq == 0) {
				mode = CST4350_IFC_MODE(sih->chipst);
				if ((mode == CST4350_IFC_MODE_USB20D) ||
				    (mode == CST4350_IFC_MODE_USB30D) ||
				    (mode == CST4350_IFC_MODE_USB30D_WL))
					xtalfreq = 40000;
				else {
					xtalfreq = 37400;
					if (mode == CST4350_IFC_MODE_HSIC20D ||
					    mode == CST4350_IFC_MODE_HSIC30D) {
						/* HSIC sprom_present_strap=1:40 mHz xtal */
						if (((CHIPREV(sih->chiprev) >= 3) ||
						     (CHIPID(sih->chip) == BCM4354_CHIP_ID) ||
						     (CHIPID(sih->chip) == BCM43569_CHIP_ID) ||
						     (CHIPID(sih->chip) == BCM43570_CHIP_ID) ||
						     (CHIPID(sih->chip) == BCM4358_CHIP_ID)) &&
						    CST4350_PKG_USB_40M(sih->chipst) &&
						    CST4350_PKG_USB(sih->chipst)) {
							xtalfreq = 40000;
						}
					}
				}
			}
			break;
		default:
			break;
	}
	/* If xtalfreq var not available, try to measure it */
	if (xtalfreq == 0)
		xtalfreq = si_pmu_measure_alpclk(sih, osh);
	si_pmu_enb_slow_clk(sih, osh, xtalfreq);
	/* Return to original core */
	si_setcoreidx(sih, origidx);
#endif /* !BCMDONGLEHOST */
}

/* 4345 Active Voltage supply settings */
#define OTP4345_AVS_STATUS_OFFSET 0x14 /* offset in OTP for AVS status register */
#define OTP4345_AVS_RO_OFFSET 0x18 /* offset in OTP for AVS Ring Oscillator value */
#define AVS_RO_OTP_MASK_4345  0x1fff /* Only 13 bits of Ring Oscillator value are stored in OTP */

/* 4349 Chip Group's (4349, 4355, 4359) Active Voltage supply settings */
#define OTP4349_GRP_AVS_STATUS_OFFSET 0x25F /* offset in OTP for AVS status register */
#define OTP4349_GRP_AVS_RO_OFFSET 0x260 /* offset in OTP for AVS Ring Oscillator value */
#define AVS_RO_OTP_MASK_4349  0x3fff /* Only 14 bits of Ring Oscillator value are stored in OTP */

#define AVS_STATUS_ENABLED_FLAG  0x1
#define AVS_STATUS_ABORT_FLAG    0x2
#define AVS_RO_MIN_COUNT  4400
#define AVS_RO_MAX_COUNT  8000
#define AVS_RO_IDEAL_COUNT   4400 /* Corresponds to 1.2V */


/** initialize PMU registers in case default values proved to be suboptimal */
void
BCMATTACHFN(si_pmu_swreg_init)(si_t *sih, osl_t *osh)
{
	uint16 cbuck_mv;
	int8 vreg_val;

	ASSERT(sih->cccaps & CC_CAP_PMU);

	switch (CHIPID(sih->chip)) {
	case BCM4336_CHIP_ID:
		if (CHIPREV(sih->chiprev) < 2) {
			/* Reduce CLDO PWM output voltage to 1.2V */
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CLDO_PWM, 0xe);
			/* Reduce CLDO BURST output voltage to 1.2V */
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CLDO_BURST, 0xe);
			/* Reduce LNLDO1 output voltage to 1.2V */
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LNLDO1, 0xe);
		}
		if (CHIPREV(sih->chiprev) == 2) {
			/* Reduce CBUCK PWM output voltage */
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CBUCK_PWM, 0x16);
			/* Reduce CBUCK BURST output voltage */
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CBUCK_BURST, 0x16);
			si_pmu_set_ldo_voltage(sih, osh, SET_LNLDO_PWERUP_LATCH_CTRL, 0x3);
		}
		if (CHIPREV(sih->chiprev) == 0)
			si_pmu_vreg_control(sih, PMU_VREG_2, 0x400000, 0x400000);

	case BCM4349_CHIP_GRPID:
		if (!ISSIM_ENAB(sih)) {
			/* Enable CBUCK Frequency Calibration */
			si_pmu_vreg_control(sih, PMU_VREG_0, 0x40, 0x40);
			/* This sequence needs to be followed */
			/* BBPLL => AUTO */
			si_pmu_vreg_control(sih, PMU_VREG_6, 0x30000, 0x20000);
			/* mini-PMU => PWM */
			si_pmu_vreg_control(sih, PMU_VREG_6, 0xC0000, 0x40000);
			/* LNLDO => AUTO */
			si_pmu_vreg_control(sih, PMU_VREG_6, 0x300000, 0x200000);
			/* PCIE PWRDN 0 => AUTO */
			si_pmu_vreg_control(sih, PMU_VREG_6, 0xC00000, 0x800000);
			/* PCIE PWRDN 1 => AUTO */
			si_pmu_vreg_control(sih, PMU_VREG_6, 0x3000000, 0x2000000);
			/* PHY PWRSW Pending => LPOM */
			si_pmu_vreg_control(sih, PMU_VREG_6, 0x30000000, 0x0);
			/* MAC FAST Clock Available => AUTO */
			si_pmu_vreg_control(sih, PMU_VREG_5, 0x60000, 0x40000);
			/* SR Pending => AUTO */
			si_pmu_vreg_control(sih, PMU_VREG_5, 0x180000, 0x100000);
			/* BT Ready => AUTO */
			si_pmu_vreg_control(sih, PMU_VREG_5, 0x600000, 0x400000);
			/* Enable Fine PWM/AUTO/LPOM control */
			si_pmu_vreg_control(sih, PMU_VREG_6, 0x40000000, 0x40000000);
		}
		break;
	case BCM53573_CHIP_GRPID:
		break;
	case BCM43362_CHIP_ID:
	case BCM4330_CHIP_ID:
		cbuck_mv = (uint16)getintvar(NULL, rstr_cbuckout);

		/* set default cbuck output to be 1.5V */
		if (!cbuck_mv)
			cbuck_mv = 1500;
		vreg_val = si_pmu_cbuckout_to_vreg_ctrl(sih, cbuck_mv);
		/* set vreg ctrl only if there is a mapping defined for vout to bit map */
		if (vreg_val >= 0) {
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CBUCK_PWM, vreg_val);
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CBUCK_BURST, vreg_val);
		}
		break;
	case BCM4314_CHIP_ID:
		if (CHIPREV(sih->chiprev) == 0) {
			/* Reduce LPLDO2 output voltage to 1.0V */
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LDO2, 0);
		}
		break;
	case BCM43340_CHIP_ID:
	case BCM43341_CHIP_ID:
	case BCM4334_CHIP_ID:
	{
		const char *cldo_val;
		uint32 cldo;

		/* Clear BT/WL REG_ON pulldown resistor disable to reduce leakage */
		si_pmu_vreg_control(sih, PMU_VREG_0, (1 << PMU_VREG0_DISABLE_PULLD_BT_SHIFT) |
			(1 << PMU_VREG0_DISABLE_PULLD_WL_SHIFT), 0);

		if (!CST4334_CHIPMODE_HSIC(sih->chipst))
			si_pmu_chipcontrol(sih, PMU_CHIPCTL2,
				CCTRL4334_HSIC_LDO_PU, CCTRL4334_HSIC_LDO_PU);

		if ((cldo_val = getvar(NULL, rstr_cldo_ldo2)) != NULL) {
			/* Adjust LPLDO2 output voltage */
			cldo = (uint32)bcm_strtoul(cldo_val, NULL, 0);
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LDO2, (uint8) cldo);
		}
#ifdef SRFAST
		else
			if (SR_FAST_ENAB()) {
				/* Reduce LPLDO2 output voltage to 1.0V by default for SRFAST */
				si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LDO2, 0);
			}
#endif /* SRFAST */

		if ((cldo_val = getvar(NULL, rstr_cldo_pwm)) != NULL) {
			cldo = (uint32)bcm_strtoul(cldo_val, NULL, 0);
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CLDO_PWM, (uint8) cldo);
		}

		if ((cldo_val = getvar(NULL, rstr_force_pwm_cbuck)) != NULL) {
			cldo = (uint32)bcm_strtoul(cldo_val, NULL, 0);

			pmu_corereg(sih, SI_CC_IDX, regcontrol_addr,
				~0, 0);
			pmu_corereg(sih, SI_CC_IDX, regcontrol_data,
				0x1 << 1, (cldo & 0x1) << 1);
		}
	}
		break;
	case BCM43143_CHIP_ID:
		/* Force CBUCK to PWM mode */
		si_pmu_vreg_control(sih, PMU_VREG_0, 0x2, 0x2);
		/* Increase CBUCK output voltage to 1.4V */
		si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CBUCK_PWM, 0x2);
		si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CBUCK_BURST, 0x2);
		/* Decrease LNLDO output voltage to just under 1.2V */
		si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LNLDO1, 0x7);

		si_pmu_chipcontrol(sih, PMU_CHIPCTL0, PMU43143_XTAL_CORE_SIZE_MASK, 0x10);
		break;
	case BCM43569_CHIP_ID:
		si_gci_chipcontrol(sih, 5, 0x3 << 29, 0x3 << 29);
		break;
	CASE_BCM43602_CHIP:
		/* adjust PA Vref to 2.80V */
		si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_PAREF, 0x0c);
		break;
	case BCM4345_CHIP_ID:
		break;
	case BCM43018_CHIP_ID:
	case BCM43430_CHIP_ID:
	{
		const char *ldo_val;
		uint32 ldo;

		if ((ldo_val = getvar(NULL, rstr_lpldo1)) != NULL) {
			/* Adjust LPLDO1 output voltage */
			ldo = (uint32)bcm_strtoul(ldo_val, NULL, 0);
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LDO1, (uint8) ldo);
		}
		else
			/* Reduce LPLDO1 output voltage to 1.0V by default */
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LDO1, 5);

		if ((ldo_val = getvar(NULL, rstr_lnldo1)) != NULL) {
			/* Adjust CLDO output voltage */
			ldo = (uint32)bcm_strtoul(ldo_val, NULL, 0);
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_LNLDO1, (uint8) ldo);
		}

		if ((ldo_val = getvar(NULL, rstr_cldo_pwm)) != NULL) {
			ldo = (uint32)bcm_strtoul(ldo_val, NULL, 0);
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CLDO_PWM, (uint8) ldo);
		}

		if ((ldo_val = getvar(NULL, rstr_cldo_burst)) != NULL) {
			ldo = (uint32)bcm_strtoul(ldo_val, NULL, 0);
			si_pmu_set_ldo_voltage(sih, osh, SET_LDO_VOLTAGE_CLDO_BURST, (uint8) ldo);
		}

		/*
		 * The Cbuck frequency in WL only mode is default at 6MHz.
		 * BT or BT+WL mode are at 4MHz as they should be.
		 */
		si_pmu_vreg_control(sih, PMU_VREG_0,
			(PMU_VREG0_CBUCKFSW_ADJ_MASK << PMU_VREG0_CBUCKFSW_ADJ_SHIFT),
			(0x17 << PMU_VREG0_CBUCKFSW_ADJ_SHIFT));
		si_pmu_vreg_control(sih, PMU_VREG_0,
			(PMU_VREG0_RAMP_SEL_MASK << PMU_VREG0_RAMP_SEL_SHIFT),
			(0x5 << PMU_VREG0_RAMP_SEL_SHIFT));
		si_pmu_vreg_control(sih, PMU_VREG_0,
			(PMU_VREG0_VFB_RSEL_MASK << PMU_VREG0_VFB_RSEL_SHIFT),
			(0x3 << PMU_VREG0_VFB_RSEL_SHIFT));
		si_pmu_vreg_control(sih, PMU_VREG_0,
			(1 << PMU_VREG0_I_SR_CNTL_EN_SHIFT),
			(1 << PMU_VREG0_I_SR_CNTL_EN_SHIFT));
	}
		break;
	default:
		break;
	}
	si_pmu_otp_vreg_control(sih, osh);
} /* si_pmu_swreg_init */

void
si_pmu_radio_enable(si_t *sih, bool enable)
{
	ASSERT(sih->cccaps & CC_CAP_PMU);

	switch (CHIPID(sih->chip)) {
	case BCM4330_CHIP_ID:
	case BCM4336_CHIP_ID:
	case BCM43362_CHIP_ID:
	{
		uint32 wrap_reg, val;
		wrap_reg = si_wrapperreg(sih, AI_OOBSELOUTB74, 0, 0);

		val = ((1 << OOB_SEL_OUTEN_B_5) | (1 << OOB_SEL_OUTEN_B_6));
		if (enable)
			wrap_reg |= val;
		else
			wrap_reg &= ~val;
		si_wrapperreg(sih, AI_OOBSELOUTB74, ~0, wrap_reg);

		break;
	}
	default:
		break;
	}
} /* si_pmu_radio_enable */

/** Wait for a particular clock level to be on the backplane */
uint32
si_pmu_waitforclk_on_backplane(si_t *sih, osl_t *osh, uint32 clk, uint32 delay_val)
{
	pmuregs_t *pmu;
	uint origidx;
	uint32 val;

	ASSERT(sih->cccaps & CC_CAP_PMU);
	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if (delay_val)
		SPINWAIT(((R_REG(osh, &pmu->pmustatus) & clk) != clk), delay_val);
	val = R_REG(osh, &pmu->pmustatus) & clk;

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	return (val);
}


#define EXT_ILP_HZ 32768

/**
 * Measures the ALP clock frequency in KHz.  Returns 0 if not possible.
 * Possible only if PMU rev >= 10 and there is an external LPO 32768Hz crystal.
 */
uint32
BCMATTACHFN(si_pmu_measure_alpclk)(si_t *sih, osl_t *osh)
{
	uint32 alp_khz;
	uint32 pmustat_lpo = 0;
	pmuregs_t *pmu;
	uint origidx;

	if (PMUREV(sih->pmurev) < 10)
		return 0;

	ASSERT(sih->cccaps & CC_CAP_PMU);

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	if ((CHIPID(sih->chip) == BCM4335_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4345_CHIP_ID) ||
		BCM4349_CHIP(sih->chip) ||
		BCM53573_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM43430_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43018_CHIP_ID) ||
		BCM4365_CHIP(sih->chip) ||
		(CHIPID(sih->chip) == BCM7271_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43012_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM43909_CHIP_ID) ||
		(CHIPID(sih->chip) == BCM4364_CHIP_ID) ||
		(BCM4347_CHIP(sih->chip)) ||
		(CHIPID(sih->chip) == BCM4369_CHIP_ID) ||
		BCM43602_CHIP(sih->chip) ||
		BCM4350_CHIP(sih->chip) ||
		FALSE)
		pmustat_lpo = !(R_REG(osh, &pmu->pmucontrol) & PCTL_LPO_SEL);
	else
		pmustat_lpo = R_REG(osh, &pmu->pmustatus) & PST_EXTLPOAVAIL;

	if (pmustat_lpo) {
		uint32 ilp_ctr, alp_hz;

		/* Enable the reg to measure the freq, in case disabled before */
		W_REG(osh, &pmu->pmu_xtalfreq, 1U << PMU_XTALFREQ_REG_MEASURE_SHIFT);

		/* Delay for well over 4 ILP clocks */
		OSL_DELAY(1000);

		/* Read the latched number of ALP ticks per 4 ILP ticks */
		ilp_ctr = R_REG(osh, &pmu->pmu_xtalfreq) & PMU_XTALFREQ_REG_ILPCTR_MASK;

		/* Turn off the PMU_XTALFREQ_REG_MEASURE_SHIFT bit to save power */
		W_REG(osh, &pmu->pmu_xtalfreq, 0);

		/* Calculate ALP frequency */
		alp_hz = (ilp_ctr * EXT_ILP_HZ) / 4;

		/* Round to nearest 100KHz, and at the same time convert to KHz */
		alp_khz = (alp_hz + 50000) / 100000 * 100;
	} else
		alp_khz = 0;

	/* Return to original core */
	si_setcoreidx(sih, origidx);

	return alp_khz;
} /* si_pmu_measure_alpclk */

void
si_pmu_set_4330_plldivs(si_t *sih, uint8 dacrate)
{
	uint32 FVCO = si_pmu1_pllfvco0(sih)/1000;	/* in [Mhz] */
	uint32 m1div, m2div, m3div, m4div, m5div, m6div;
	uint32 pllc1, pllc2;

	m2div = m3div = m4div = m6div = FVCO/80;

	m5div = FVCO/dacrate;

	if (CST4330_CHIPMODE_SDIOD(sih->chipst))
		m1div = FVCO/80;
	else
		m1div = FVCO/90;
	pllc1 = (m1div << PMU1_PLL0_PC1_M1DIV_SHIFT) | (m2div << PMU1_PLL0_PC1_M2DIV_SHIFT) |
		(m3div << PMU1_PLL0_PC1_M3DIV_SHIFT) | (m4div << PMU1_PLL0_PC1_M4DIV_SHIFT);
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, pllc1);

	pllc2 = si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, 0, 0);
	pllc2 &= ~(PMU1_PLL0_PC2_M5DIV_MASK | PMU1_PLL0_PC2_M6DIV_MASK);
	pllc2 |= ((m5div << PMU1_PLL0_PC2_M5DIV_SHIFT) | (m6div << PMU1_PLL0_PC2_M6DIV_SHIFT));
	si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG2, ~0, pllc2);
} /* si_pmu_set_4330_plldivs */

typedef struct cubkout2vreg {
	uint16 cbuck_mv;
	int8  vreg_val;
} cubkout2vreg_t;

cubkout2vreg_t BCMATTACHDATA(cbuck2vreg_tbl)[] = {
	{1500,	0},
	{1800,	0x17}
};

static int8
BCMATTACHFN(si_pmu_cbuckout_to_vreg_ctrl)(si_t *sih, uint16 cbuck_mv)
{
	uint32 i;

	BCM_REFERENCE(sih);

	for (i = 0; i < ARRAYSIZE(cbuck2vreg_tbl); i++) {
		if (cbuck_mv == cbuck2vreg_tbl[i].cbuck_mv)
			return cbuck2vreg_tbl[i].vreg_val;
	}
	return -1;
}

/** Update min/max resources after SR-ASM download to d11 txfifo */
void
si_pmu_res_minmax_update(si_t *sih, osl_t *osh)
{
	uint32 min_mask = 0, max_mask = 0;
	pmuregs_t *pmu;
	uint origidx, intr_val = 0;

	/* Block ints and save current core */
	intr_val = si_introff(sih);
	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
	case BCM4345_CHIP_ID:
	case BCM43430_CHIP_ID:
	case BCM43018_CHIP_ID:
	case BCM4365_CHIP_ID:
	case BCM4366_CHIP_ID:
	case BCM43664_CHIP_ID:
	case BCM43666_CHIP_ID:
#ifdef BCM7271
	case BCM7271_CHIP_ID:
#endif /* BCM7271 */
	case BCM43909_CHIP_ID:
	CASE_BCM43602_CHIP:
	case BCM4350_CHIP_ID:
	case BCM4354_CHIP_ID:
	case BCM43556_CHIP_ID:
	case BCM43558_CHIP_ID:
	case BCM43566_CHIP_ID:
	case BCM43568_CHIP_ID:
	case BCM43569_CHIP_ID:
	case BCM43570_CHIP_ID:
	case BCM4358_CHIP_ID:
		max_mask = 0; /* Only care about min_mask for now */
		break;
	case BCM43012_CHIP_ID:
		min_mask = RES43012_PMU_SLEEP;
		break;
	case BCM4335_CHIP_ID:
		/* Uses this min_mask for both SR and non-SR */
		min_mask = RES4335_PMU_BG_PU;
		/* SWWLAN-38972 : Fix for 43162 */
		if (BUSTYPE(sih->bustype) == PCI_BUS) {
#if defined(SAVERESTORE)
			min_mask = PMURES_BIT(RES4335_LPLDO_PO);
#else
			min_mask = PMURES_BIT(RES4335_WL_CORE_RDY) | PMURES_BIT(RES4335_OTP_PU);
#endif
		}
		break;
	case BCM4349_CHIP_GRPID:
		min_mask = RES4349_BG_PU;
		break;
	case BCM53573_CHIP_GRPID:
		break; /* Not sure what to do in this case */
	case BCM4364_CHIP_ID:
#if defined(SAVERESTORE)
		if (SR_ENAB()) {
			min_mask = PMURES_BIT(RES4364_LPLDO_PU) | PMURES_BIT(RES4364_MEMLPLDO_PU);
		}
		break;
#endif
	default:
		break;
	}

	if (min_mask) {
		/* Add min mask dependencies */
		min_mask |= si_pmu_res_deps(sih, osh, pmu, min_mask, FALSE);
		W_REG(osh, &pmu->min_res_mask, min_mask);
	}

	if (max_mask) {
		max_mask |= si_pmu_res_deps(sih, osh, pmu, max_mask, FALSE);
		W_REG(osh, &pmu->max_res_mask, max_mask);
	}

	si_pmu_wait_for_steady_state(sih, osh, pmu);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	si_intrrestore(sih, intr_val);
} /* si_pmu_res_minmax_update */


/**
 * API used to configure the PMU for set/reset the chip to/from ULB operation. Chip
 * specific clock-division is applied to achieved required BW of operation.
 */
uint
si_pmu_set_ulbmode(si_t *sih, osl_t *osh, uint8 pmu_ulb_bw)
{
	uint err = BCME_OK;
	uint8 ulb_mode = 0;
	BCM_REFERENCE(osh);

	ASSERT(pmu_ulb_bw < MAX_SUPP_PMU_ULB_BW);

	switch (CHIPID(sih->chip)) {
		case BCM4349_CHIP_GRPID:
		case BCM53573_CHIP_GRPID:
		{
			uint div_fact  = 0;
			uint m1_m2_div = 0;
			uint pll_reg   = (si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0) &
				~(PMU1_PLL0_PC1_M1DIV_MASK | PMU1_PLL0_PC1_M2DIV_MASK));

			/*
			 * Division factor is decided based on bandwidth to be selected.
			 */
			switch (pmu_ulb_bw) {
			case PMU_ULB_BW_10MHZ:
				div_fact = 1;
				break;
			case PMU_ULB_BW_5MHZ:
				div_fact = 2;
				break;
			case PMU_ULB_BW_NONE:
				div_fact = 0;
				break;
			default:
				PMU_ERROR(("%s: ULB BW %d not supported\n", __FUNCTION__,
					pmu_ulb_bw));
				err = BCME_UNSUPPORTED;
				return err;
			}
			if (BCM4349_CHIP(sih->chip)) {
				m1_m2_div = (pmu1_pllctrl_tab_4349_640mhz[PMU1_PLL0_PLLCTL1] &
				(PMU1_PLL0_PC1_M1DIV_MASK | PMU1_PLL0_PC1_M2DIV_MASK)) << div_fact;
			}
			else if (BCM53573_CHIP(sih->chip)) {
				m1_m2_div = (pmu1_pllctrl_tab_53573_640mhz[PMU1_PLL0_PLLCTL1] &
				(PMU1_PLL0_PC1_M1DIV_MASK)) << div_fact;
			}

			/* Set the m1div and m2div value for 5/10/20MHz mode */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, (pll_reg | m1_m2_div));

			/* Asserting Load Channel */
			pll_reg = (si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, 0, 0) &
				~PMU1_PLL0_PC5_ASSERT_CH_MASK);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, (pll_reg |
				PMU1_PLL0_PC5_ASSERT_CH_MASK));

			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);

			/* De-Asserting Load Channel */
			pll_reg = (si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, 0, 0) &
				~PMU1_PLL0_PC5_DEASSERT_CH_MASK);
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, pll_reg);

			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);

			/* MAC Clock Multiplication Factor is updated */
			si_update_macclk_mul_fact(sih, div_fact);

			break;
		}
		case BCM4347_CHIP_GRPID:
		case BCM4369_CHIP_ID:
			ASSERT(0);
			break;
		case BCM4365_CHIP_ID:
		case BCM4366_CHIP_ID:
		case BCM7271_CHIP_ID:
		case BCM43664_CHIP_ID:
	        case BCM43666_CHIP_ID:
		{
			uint32 mdiv = 0;
			uint32 pll_reg0 = si_pmu_pllcontrol(sih, PMU1_PLL0_PLLCTL0, 0, 0);
			uint32 pll_reg1 = si_pmu_pllcontrol(sih, PMU1_PLL0_PLLCTL1, 0, 0);
			uint32 pll_reg4 = si_pmu_pllcontrol(sih, PMU1_PLL0_PLLCTL4, 0, 0);

			/*
			 * Division factor is decided based on bandwidth to be selected.
			 */
			switch (pmu_ulb_bw) {
			case PMU_ULB_BW_10MHZ:
				mdiv = 0x0C14030C;
				ulb_mode = 1;
				break;
			case PMU_ULB_BW_5MHZ:
				mdiv = 0x0C280318;
				ulb_mode = 2;
				break;
			case PMU_ULB_BW_NONE:
				mdiv = 0x0C0A0306;
				ulb_mode = 0;
				break;
			default:
				PMU_ERROR(("%s: ULB BW %d not supported\n", __FUNCTION__,
					pmu_ulb_bw));
				err = BCME_UNSUPPORTED;
				return err;
			}

			if (pll_reg4 == mdiv) {
				PMU_ERROR(("%s: ULB clk not changed\n", __FUNCTION__));
				return err;
			}

			/* Stop all 6 channels of BB PLL */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, pll_reg1 | (0x3f<<9));

			/* Set the m0div and m2div value for 5/10/20MHz mode */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, mdiv);

			/* set areset[bit3] to 1, and dreset[bit4] to 1 */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, pll_reg0 | (1<<3) | (1<<4));
			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);
			OSL_DELAY(10);

			/* set areset[bit3] to 0, and dreset[bit4] to 1 */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0,
					(pll_reg0 & ~(1<<3)) | (1<<4));
			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);
			OSL_DELAY(50);

			/* set areset[bit3] to 0, and dreset[bit4] to 0 */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG0, ~0, pll_reg0 & ~(1<<3) & ~(1<<4));
			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);
			OSL_DELAY(50);

			/* Enable all 6 channels of BBPLL */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, pll_reg1 & ~(0x3f<<9));

			/* MAC Clock Multiplication Factor is updated */
			/* Since MAC is not downclocked setting div factor to 0 now */
			ulb_mode = 0;
			si_update_macclk_mul_fact(sih, ulb_mode);

			PMU_ERROR(("%s: ULB setting for 4365\n", __FUNCTION__));
			break;
		}
#ifdef WL11ULB
		case BCM43430_CHIP_ID:
		{
			uint8 div_fact  = 0;
			uint32 m4_div = 0;
			uint32 m6_div = 0;
			uint32 pll_reg_temp = 0;
			uint32 xtal_freq = 0;
			uint8 spurmode = 0;
			uint8 pll_ctrlcnt = 0;
			const pllctrl_data_t *pllctrlreg_update;
			uint32 array_size = 0;
			uint8 indx = 0;
			const uint32 *pllctrlreg_val = NULL;
			uint32 pll_reg_1   = (si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, 0, 0));
			uint32 pll_reg_4   = (si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, 0, 0) &
				~(PMU1_PLL0_PC1_M4DIV_MASK));
			uint32 pll_reg_5  = (si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, 0, 0) &
				~(PMU1_PLL0_PC2_M6DIV_MASK));
			/*
			 * Division factor is decided based on bandwidth to be selected.
			 */
			switch (pmu_ulb_bw) {
			case PMU_ULB_BW_10MHZ:
				div_fact = 1;
				break;
			case PMU_ULB_BW_5MHZ:
				div_fact = 2;
				break;
			case PMU_ULB_BW_NONE:
				div_fact = 0;
				break;
			default:
				PMU_ERROR(("%s: ULB BW %d not supported\n", __FUNCTION__,
					pmu_ulb_bw));
				err = BCME_UNSUPPORTED;
				return err;
			}

			/*
			 * Find the related pll control table based on xtal freq and spur mode
			 */
			xtal_freq = si_alp_clock(sih)/1000;
			spurmode = si_getspurmode(sih);
			if (spurmode == 1)
				pllctrlreg_val = pmu1_pllctrl_tab_43430_980mhz;
			else if (spurmode == 2)
				pllctrlreg_val = pmu1_pllctrl_tab_43430_984mhz;
			else if (spurmode == 3)
				pllctrlreg_val = pmu1_pllctrl_tab_43430_326p4mhz;
			else
				pllctrlreg_val = pmu1_pllctrl_tab_43430_972mhz;

			if (sih->pmurev >= 5) {
				pll_ctrlcnt = (sih->pmucaps & PCAP5_PC_MASK) >> PCAP5_PC_SHIFT;
			} else {
				pll_ctrlcnt = (sih->pmucaps & PCAP_PC_MASK) >> PCAP_PC_SHIFT;
			}

			pllctrlreg_update = pmu1_xtaltab0_43430;
			array_size = ARRAYSIZE(pmu1_xtaltab0_43430);

			for (indx = 0; indx < array_size; indx++) {
				/* If the entry does not match the xtal just continue the loop */
				if ((uint16)(pllctrlreg_update[indx].clock) == (uint16)xtal_freq) {
					/* Construct new divider values based on ULB mode,
					*  m2div for MAC, m4div for Baseband, m6div for LDPC
					*/
					m4_div = (pllctrlreg_val[indx*pll_ctrlcnt +
						PMU1_PLL0_PLLCTL4] & (PMU1_PLL0_PC1_M4DIV_MASK))
						 << div_fact;
					m6_div    = (pllctrlreg_val[indx*pll_ctrlcnt +
						PMU1_PLL0_PLLCTL5] & (PMU1_PLL0_PC2_M6DIV_MASK))
						<< div_fact;
					break;
				}
			}

			/* Set the m2div and m4div valu| for 5/10/20MHz mode */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG4, ~0, (pll_reg_4 | m4_div));
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG5, ~0, (pll_reg_5 | m6_div));

			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);

			/* Set load bits */
			pll_reg_temp = (pll_reg_1 | (PMU1_PLL0_PC1_HOLD_LOAD_CH << 24));
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, pll_reg_temp);
			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);

			/* Clear load bits */
			si_pmu_pllcontrol(sih, PMU_PLL_CTRL_REG1, ~0, pll_reg_1);
			/* Updating PMU to load the PLL */
			si_pmu_pllupd(sih);

			break;
		}
#endif /* WL11ULB */
		case BCM43012_CHIP_ID:
			switch (pmu_ulb_bw) {
			case PMU_ULB_BW_10MHZ:
				si_pmu_chipcontrol(sih, PMU_CHIPCTL0,
						~PMU43012_CC0_ULB_DIVMASK, PMU43012_10MHZ_ULB_DIV);
				break;
			case PMU_ULB_BW_5MHZ:
				si_pmu_chipcontrol(sih, PMU_CHIPCTL0,
						~PMU43012_CC0_ULB_DIVMASK, PMU43012_5MHZ_ULB_DIV);
				break;
			case PMU_ULB_BW_2P5MHZ:
				si_pmu_chipcontrol(sih, PMU_CHIPCTL0,
						~PMU43012_CC0_ULB_DIVMASK, PMU43012_2P5MHZ_ULB_DIV);
				break;
			case PMU_ULB_BW_NONE:
				si_pmu_chipcontrol(sih, PMU_CHIPCTL0,
						~PMU43012_CC0_ULB_DIVMASK, PMU43012_ULB_NO_DIV);
				break;
			default:
				PMU_ERROR(("%s: ULB BW %d not supported\n", __FUNCTION__,
					pmu_ulb_bw));
				err = BCME_UNSUPPORTED;
				return err;
			}
			break;
		default:
			PMU_ERROR(("%s: ULB not supported\n", __FUNCTION__));
			err = BCME_UNSUPPORTED;
			break;
	}
	return err;
} /* si_pmu_set_ulbmode */

#ifdef LDO3P3_MIN_RES_MASK

/**
 * Function to enable the min_mask with specified resources along with its dependencies.
 * Also it can be used for bringing back to the default value of the device.
 */
static int
si_pmu_min_res_set(si_t *sih, osl_t *osh, uint min_mask, bool set)
{
	uint32 min_res, max_res;
	uint origidx, intr_val = 0;
	pmuregs_t *pmu;

	/* Block ints and save current core */
	intr_val = si_introff(sih);

	/* Remember original core before switch to chipc */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	si_pmu_res_masks(sih, &min_res, &max_res);
	min_mask |= si_pmu_res_deps(sih, osh, pmu, min_mask, TRUE);

	/*
	 * If set is enabled, the resources specified in the min_mask is brought up. If not set,
	 * go to the default min_resource of the device.
	 */
	if (set) {
		OR_REG(osh, &pmu->min_res_mask, min_mask);
	} else {
		min_mask &= ~min_res;
		AND_REG(osh, &pmu->min_res_mask, ~min_mask);
	}

	si_pmu_wait_for_steady_state(sih, osh, pmu);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
	si_intrrestore(sih, intr_val);

	return min_mask;
}

/** Set ldo 3.3V mask in the min resources mask register */
int
si_pmu_min_res_ldo3p3_set(si_t *sih, osl_t *osh, bool on)
{
	uint32 min_mask = 0;

	if (BCM4349_CHIP(sih->chip)) {
		min_mask = PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_PMU_SLEEP) |
			PMURES_BIT(RES4349_BG_PU);
	} else {
		return BCME_UNSUPPORTED;
	}

	si_pmu_min_res_set(sih, osh, min_mask, on);

	return BCME_OK;
}

int
si_pmu_min_res_ldo3p3_get(si_t *sih, osl_t *osh, int *res)
{
	uint32 min_mask = 0;

	if (BCM4349_CHIP(sih->chip)) {
		min_mask = PMURES_BIT(RES4349_LDO3P3_PU) | PMURES_BIT(RES4349_PMU_SLEEP) |
			PMURES_BIT(RES4349_BG_PU);
	} else {
		return BCME_UNSUPPORTED;
	}

	*res = (si_corereg(sih, SI_CC_IDX,
			OFFSETOF(chipcregs_t, min_res_mask), 0, 0) & min_mask) && min_mask;
	return BCME_OK;
}
#endif /* LDO3P3_MIN_RES_MASK */

#endif /* !defined(BCMDONGLEHOST) */


void
si_switch_pmu_dependency(si_t *sih, uint mode)
{
#ifdef DUAL_PMU_SEQUENCE
	osl_t *osh = si_osh(sih);
	uint32 current_res_state;
	uint32 min_mask, max_mask;
	const pmu_res_depend_t *pmu_res_depend_table = NULL;
	uint pmu_res_depend_table_sz = 0;
	uint origidx;
	pmuregs_t *pmu;
	chipcregs_t *cc;
	BCM_REFERENCE(cc);

	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
		cc  = si_setcore(sih, CC_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
		cc  = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	current_res_state = R_REG(osh, &pmu->res_state);
	min_mask = R_REG(osh, &pmu->min_res_mask);
	max_mask = R_REG(osh, &pmu->max_res_mask);
	W_REG(osh, &pmu->min_res_mask, (min_mask | current_res_state));
	switch (mode) {
		case PMU_4364_1x1_MODE:
		{
			if (CHIPID(sih->chip) == BCM4364_CHIP_ID) {
					pmu_res_depend_table = bcm4364a0_res_depend_1x1;
					pmu_res_depend_table_sz =
						ARRAYSIZE(bcm4364a0_res_depend_1x1);
			max_mask = PMU_4364_MAX_MASK_1x1;
			W_REG(osh, &pmu->res_table_sel, RES4364_SR_SAVE_RESTORE);
			W_REG(osh, &pmu->res_updn_timer, PMU_4364_SAVE_RESTORE_UPDNTIME_1x1);
#if defined(SAVERESTORE)
				if (SR_ENAB()) {
					/* Disable 3x3 SR engine */
					W_REG(osh, &cc->sr1_control0,
					CC_SR0_4364_SR_ENG_CLK_EN |
					CC_SR0_4364_SR_RSRC_TRIGGER |
					CC_SR0_4364_SR_WD_MEM_MIN_DIV |
					CC_SR0_4364_SR_INVERT_CLK |
					CC_SR0_4364_SR_ENABLE_HT |
					CC_SR0_4364_SR_ALLOW_PIC |
					CC_SR0_4364_SR_PMU_MEM_DISABLE);
				}
#endif /* SAVERESTORE */
			}
			break;
		}
		case PMU_4364_3x3_MODE:
		{
			if (CHIPID(sih->chip) == BCM4364_CHIP_ID) {
				W_REG(osh, &pmu->res_table_sel, RES4364_SR_SAVE_RESTORE);
				W_REG(osh, &pmu->res_updn_timer,
					PMU_4364_SAVE_RESTORE_UPDNTIME_3x3);
				/* Change the dependency table only if required */
				if ((max_mask != PMU_4364_MAX_MASK_3x3) ||
					(max_mask != PMU_4364_MAX_MASK_RSDB)) {
						pmu_res_depend_table = bcm4364a0_res_depend_rsdb;
						pmu_res_depend_table_sz =
							ARRAYSIZE(bcm4364a0_res_depend_rsdb);
						max_mask = PMU_4364_MAX_MASK_3x3;
				}
#if defined(SAVERESTORE)
				if (SR_ENAB()) {
					/* Enable 3x3 SR engine */
					W_REG(osh, &cc->sr1_control0,
					CC_SR0_4364_SR_ENG_CLK_EN |
					CC_SR0_4364_SR_RSRC_TRIGGER |
					CC_SR0_4364_SR_WD_MEM_MIN_DIV |
					CC_SR0_4364_SR_INVERT_CLK |
					CC_SR0_4364_SR_ENABLE_HT |
					CC_SR0_4364_SR_ALLOW_PIC |
					CC_SR0_4364_SR_PMU_MEM_DISABLE |
					CC_SR0_4364_SR_ENG_EN_MASK);
				}
#endif /* SAVERESTORE */
			}
			break;
		}
		case PMU_4364_RSDB_MODE:
		default:
		{
			if (CHIPID(sih->chip) == BCM4364_CHIP_ID) {
				W_REG(osh, &pmu->res_table_sel, RES4364_SR_SAVE_RESTORE);
				W_REG(osh, &pmu->res_updn_timer,
					PMU_4364_SAVE_RESTORE_UPDNTIME_3x3);
				/* Change the dependency table only if required */
				if ((max_mask != PMU_4364_MAX_MASK_3x3) ||
					(max_mask != PMU_4364_MAX_MASK_RSDB)) {
						pmu_res_depend_table =
							bcm4364a0_res_depend_rsdb;
						pmu_res_depend_table_sz =
							ARRAYSIZE(bcm4364a0_res_depend_rsdb);
						max_mask = PMU_4364_MAX_MASK_RSDB;
				}
#if defined(SAVERESTORE)
			if (SR_ENAB()) {
					/* Enable 3x3 SR engine */
					W_REG(osh, &cc->sr1_control0,
					CC_SR0_4364_SR_ENG_CLK_EN |
					CC_SR0_4364_SR_RSRC_TRIGGER |
					CC_SR0_4364_SR_WD_MEM_MIN_DIV |
					CC_SR0_4364_SR_INVERT_CLK |
					CC_SR0_4364_SR_ENABLE_HT |
					CC_SR0_4364_SR_ALLOW_PIC |
					CC_SR0_4364_SR_PMU_MEM_DISABLE |
					CC_SR0_4364_SR_ENG_EN_MASK);
				}
#endif /* SAVERESTORE */
			}
			break;
		}
	}
	si_pmu_resdeptbl_upd(sih, osh, pmu, pmu_res_depend_table, pmu_res_depend_table_sz);
	W_REG(osh, &pmu->max_res_mask, max_mask);
	W_REG(osh, &pmu->min_res_mask, min_mask);
	si_pmu_wait_for_steady_state(sih, osh, pmu);
	/* Add some delay; allow resources to come up and settle. */
	OSL_DELAY(200);
	si_setcoreidx(sih, origidx);
#endif /* DUAL_PMU_SEQUENCE */
}


#if defined(BCMULP)

int
si_pmu_ulp_register(si_t *sih)
{
	return ulp_p1_module_register(ULP_MODULE_ID_PMU, &ulp_pmu_ctx, (void *)sih);
}

static uint
si_pmu_ulp_get_retention_size_cb(void *handle, ulp_ext_info_t *einfo)
{
	ULP_DBG(("%s: sz: %d\n", __FUNCTION__, sizeof(si_pmu_ulp_cr_dat_t)));
	return sizeof(si_pmu_ulp_cr_dat_t);
}

static int
si_pmu_ulp_enter_cb(void *handle, ulp_ext_info_t *einfo, uint8 *cache_data)
{
	si_pmu_ulp_cr_dat_t crinfo = {0};
	crinfo.ilpcycles_per_sec = ilpcycles_per_sec;
	ULP_DBG(("%s: ilpcycles_per_sec: %x\n", __FUNCTION__, ilpcycles_per_sec));
	memcpy(cache_data, (void*)&crinfo, sizeof(crinfo));
	return BCME_OK;
}

static int
si_pmu_ulp_exit_cb(void *handle, uint8 *cache_data,
	uint8 *p2_cache_data)
{
	si_pmu_ulp_cr_dat_t *crinfo = (si_pmu_ulp_cr_dat_t *)cache_data;

	ilpcycles_per_sec = crinfo->ilpcycles_per_sec;
	ULP_DBG(("%s: ilpcycles_per_sec: %x, cache_data: %p\n", __FUNCTION__,
		ilpcycles_per_sec, cache_data));
	return BCME_OK;
}

void
si_pmu_ulp_chipconfig(si_t *sih, osl_t *osh)
{
	uint32 reg_val;

	BCM_REFERENCE(reg_val);

	if (CHIPID(sih->chip) == BCM43012_CHIP_ID) {
		/* DS1 reset and clk enable init value config */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL14, ~0x0,
			(PMUCCTL14_43012_ARMCM3_RESET_INITVAL |
			PMUCCTL14_43012_DOT11MAC_CLKEN_INITVAL |
			PMUCCTL14_43012_SDIOD_RESET_INIVAL |
			PMUCCTL14_43012_SDIO_CLK_DMN_RESET_INITVAL |
			PMUCCTL14_43012_SOCRAM_CLKEN_INITVAL |
			PMUCCTL14_43012_M2MDMA_RESET_INITVAL |
			PMUCCTL14_43012_DOT11MAC_PHY_CLK_EN_INITVAL |
			PMUCCTL14_43012_DOT11MAC_PHY_CNTL_EN_INITVAL));

		/* Clear SFlash clock request and enable High Quality clock */
		CHIPC_REG(sih, clk_ctl_st, CCS_SFLASH_CLKREQ | CCS_HQCLKREQ, CCS_HQCLKREQ);

		reg_val = PMU_REG(sih, min_res_mask, ~0x0, ULP_MIN_RES_MASK);
		ULP_DBG(("si_pmu_ulp_chipconfig: min_res_mask: 0x%08x\n", reg_val));

		/* Force power switch off */
		si_pmu_chipcontrol(sih, PMU_CHIPCTL2,
				(PMUCCTL02_43012_SUBCORE_PWRSW_FORCE_ON |
				PMUCCTL02_43012_PHY_PWRSW_FORCE_ON), 0);

	}
}

void
si_pmu_ulp_ilp_config(si_t *sih, osl_t *osh, uint32 ilp_period)
{
	pmuregs_t *pmu;
	pmu = si_setcoreidx(sih, si_findcoreidx(sih, PMU_CORE_ID, 0));
	W_REG(osh, &pmu->ILPPeriod, ilp_period);
	si_lhl_ilp_config(sih, osh, ilp_period);
}

/** Initialize DS1 PMU hardware resources */
void
si_pmu_ds1_res_init(si_t *sih, osl_t *osh)
{
	pmuregs_t *pmu;
	uint origidx;
	const pmu_res_updown_t *pmu_res_updown_table = NULL;
	uint pmu_res_updown_table_sz = 0;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	} else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	switch (CHIPID(sih->chip)) {
	case BCM43012_CHIP_ID:
		pmu_res_updown_table = bcm43012a0_res_updown_ds1;
		pmu_res_updown_table_sz = ARRAYSIZE(bcm43012a0_res_updown_ds1);
		break;

	default:
		break;
	}

	/* Program up/down timers */
	while (pmu_res_updown_table_sz--) {
		ASSERT(pmu_res_updown_table != NULL);
		PMU_MSG(("DS1: Changing rsrc %d res_updn_timer to 0x%x\n",
			pmu_res_updown_table[pmu_res_updown_table_sz].resnum,
			pmu_res_updown_table[pmu_res_updown_table_sz].updown));
		W_REG(osh, &pmu->res_table_sel,
			pmu_res_updown_table[pmu_res_updown_table_sz].resnum);
		W_REG(osh, &pmu->res_updn_timer,
			pmu_res_updown_table[pmu_res_updown_table_sz].updown);
	}

	/* Return to original core */
	si_setcoreidx(sih, origidx);
}

#endif /* defined(BCMULP) */




void si_pmu_set_min_res_mask(si_t *sih, osl_t *osh, uint min_res_mask)
{
	pmuregs_t *pmu;
	uint origidx;

	/* Remember original core before switch to chipc/pmu */
	origidx = si_coreidx(sih);
	if (AOB_ENAB(sih)) {
		pmu = si_setcore(sih, PMU_CORE_ID, 0);
	}
	else {
		pmu = si_setcoreidx(sih, SI_CC_IDX);
	}
	ASSERT(pmu != NULL);

	W_REG(osh, &pmu->min_res_mask, min_res_mask);
	OSL_DELAY(100);

	/* Return to original core */
	si_setcoreidx(sih, origidx);
}
