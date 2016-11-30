/*
 * Basic unit test for PHY Radar module
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:.*>>
 *
 * $Id: test_phy_radar.c 634430 2016-04-28 00:05:30Z chihap $
 */

/***************************************************************************************************
************* Definitions for module components to be tested with Check tool ***********************
*/

#include <typedefs.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <osl.h>
#include <phy_api.h>
#include <wlc_phy_int.h>
#include <wlc_phyreg_ac.h>
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <phy_radar.h>
#include <phy_radar_st.h>
#include <phy_radar_shared.h>
#include <phy_type_radar.h>

#ifdef BCMDBG_ASSERT
#undef ASSERT
#include <linux_osl.h>
#endif

uint32 phyhal_msg_level = 0;

#include "test_phy_radar.h"

/***************************************************************************************************
************************************* Start of Test Section ****************************************
*/

#include <check.h> /* Includes Check framework */

/*
 * In order to run unit tests with Check, we must create some test cases,
 * aggregate them into a suite, and run them with a suite runner.
 *
 * The pattern of every unit test is as following
 *
 * START_TEST(name_of_test){
 *
 *     perform tests;
 *	       ...assert results
 * }
 * END_TEST
 *
 * Test Case is a set of at least 1 unit test
 * Test Suite is a collection of Test Cases
 * Check Framework can run multiple Test Suites.
 */

/* ------------- Global Definitoions ------------------------- */

typedef struct {
	const char *filename;	// for antenna 0 only
	uint16 min_pw;
	uint32 radar_interval;
	uint16 bw;
	uint8 radar_type;
} test_table_t;

struct phy_radar_info {
	phy_info_t *pi;
	phy_radar_st_t *st;
	phy_type_radar_fns_t *fns;
};

/*
 * Global variables definitions, for setup and teardown function.
 */
static osl_t *osh;
static phy_info_t phy_info;
static phy_ac_info_t aci;
static phy_type_radar_fns_t radar_fns;

/* PHY Data */
#define MAX_NUM_PULSES	128
#define FIFO_ENTRIES	(MAX_NUM_PULSES * 4)	// 4 * uint16 entries contain info for each pulse

typedef struct {
	uint16 num_fifo_entries;
	uint16 buffer[FIFO_ENTRIES];
	uint16 idx;
} radar_fifo_t;

static radar_fifo_t phy_fifo[RDR_NANTENNAS];

/* ------------- callback functions ---------------------------------
 * These functions are replacements (stubs) of the firmware functions
 * so that the module under test links with no error.
 */
phy_radar_st_t *
phy_radar_get_st(phy_radar_info_t *ri)
{
	ASSERT(ri != NULL);

	return ri->st;
}

void
phy_utils_phyreg_enter(phy_info_t *pi)
{
	ASSERT(pi != NULL);
	pi->phyreg_enter_depth++;
}

void
phy_utils_phyreg_exit(phy_info_t *pi)
{
	ASSERT(pi != NULL);
	ASSERT(pi->phyreg_enter_depth > 0);
	pi->phyreg_enter_depth--;
}

void
wlapi_suspend_mac_and_wait(wlc_phy_shim_info_t *physhim)
{
	UNUSED_PARAMETER(physhim);
}

void
wlapi_enable_mac(wlc_phy_shim_info_t *physhim)
{
	UNUSED_PARAMETER(physhim);
}

void
wlapi_bmac_write_shm(wlc_phy_shim_info_t *physhim, uint offset, uint16 v)
{
	UNUSED_PARAMETER(physhim);
	UNUSED_PARAMETER(offset);
	UNUSED_PARAMETER(v);
}

void
wlapi_bmac_mhf(wlc_phy_shim_info_t *physhim, uint8 idx, uint16 mask, uint16 val, int bands)
{
	UNUSED_PARAMETER(physhim);
	UNUSED_PARAMETER(idx);
	UNUSED_PARAMETER(mask);
	UNUSED_PARAMETER(val);
	UNUSED_PARAMETER(bands);
}

void
phy_utils_write_phyreg(phy_info_t *pi, uint16 addr, uint16 val)
{
	UNUSED_PARAMETER(pi);
	UNUSED_PARAMETER(addr);
	UNUSED_PARAMETER(val);
}

uint16
phy_utils_read_phyreg(phy_info_t *pi, uint16 addr)
{
	uint16 value;

	switch (addr) {
	case ACPHY_Antenna0_radarFifoCtrl(pi->pubpi->phy_rev):
		value = phy_fifo[0].num_fifo_entries;
		break;
	case ACPHY_Antenna1_radarFifoCtrl(pi->pubpi->phy_rev):
		ASSERT(RDR_NANTENNAS > 1);
		value = phy_fifo[1].num_fifo_entries;
		break;
	case ACPHY_Antenna0_radarFifoData(pi->pubpi->phy_rev):
		ASSERT(phy_fifo[0].idx < FIFO_ENTRIES);
		value = phy_fifo[0].buffer[phy_fifo[0].idx++];
		break;
	case ACPHY_Antenna1_radarFifoData(pi->pubpi->phy_rev):
		ASSERT(RDR_NANTENNAS > 1);
		ASSERT(phy_fifo[1].idx < FIFO_ENTRIES);
		value = phy_fifo[1].buffer[phy_fifo[1].idx++];
		break;
	default:
		ASSERT(0);	// the radar code should only read the radar-specific registers
	}

	return value;
}

int
phy_radar_register_impl(phy_radar_info_t *ri, phy_type_radar_fns_t *fns)
{
	ASSERT(ri != NULL);
	ASSERT(ri->st == NULL);

	if ((ri->st = (phy_radar_st_t *)MALLOCZ(osh, sizeof(*ri->st))) == NULL)
		return BCME_NOMEM;

	*ri->fns = *fns;

	return BCME_OK;
}

void
phy_radar_unregister_impl(phy_radar_info_t *ri)
{
	if (ri->st != NULL) {
		MFREE(osh, ri->st, sizeof(*ri->st));
		ri->st = NULL;
	}
}

/* ------------- Startup and Teardown - Fixtures ---------------
 * Setting up objects for each unit test,
 * it may be convenient to add some setup that is constant across all the tests in a test case
 * rather than setting up objects for each unit test.
 * Before each unit test in a test case, the setup() function is run, if defined.
 */
void
setup(void)
{
	/* allocate and initialise all structures required for phy_radar_run_nphy() */
	phy_info.pubpi = (wlc_phy_t *)MALLOCZ(osh, sizeof(*phy_info.pubpi));
	phy_info.sh = (shared_phy_t *)MALLOCZ(osh, sizeof(*phy_info.sh));
	phy_info.u.pi_acphy = (phy_info_acphy_t *)MALLOCZ(osh, sizeof(*phy_info.u.pi_acphy));
	phy_info.radari = (phy_radar_info_t *)MALLOCZ(osh, sizeof(*phy_info.radari));
	if (phy_info.radari != NULL) {
		phy_info.radari->pi = &phy_info;
		phy_info.radari->fns = &radar_fns;
		aci.pi = &phy_info;
		aci.radari = phy_ac_radar_register_impl(&phy_info, &aci, phy_info.radari);

#ifdef BCMDBG
		phy_info.radari->st->rparams.radar_args.feature_mask |=
			RADAR_FEATURE_DEBUG_PULSE_DATA | RADAR_FEATURE_DEBUG_SHORT_PULSE |
			RADAR_FEATURE_DEBUG_PW_CHECK_INFO | RADAR_FEATURE_DEBUG_REJECTED_RADAR;
#endif

		/* Turn off all radar detection algorithms. They will be enabled for each test. */
		phy_info.radari->st->rparams.radar_args.feature_mask &=
			~(RADAR_FEATURE_FCC_DETECT | RADAR_FEATURE_ETSI_DETECT);
	}
}

/*
 * Tear down objects for each unit test,
 * it may be convenient to add teardown that is constant across all the tests in a test case
 * rather than tearing down objects for each unit test.
 * After each unit test in a test case, the setup() function is run, if defined.
 * Note: checked teardown() fixture will not run if the unit test fails.
*/
void
teardown(void)
{
	/* cleanup all created memory */
	if (phy_info.radari != NULL) {
		if (aci.radari != NULL) {
			phy_ac_radar_unregister_impl(aci.radari);
			aci.radari = NULL;
		}

		MFREE(osh, phy_info.radari, sizeof(*phy_info.radari));
		phy_info.radari = NULL;
	}

	if (phy_info.u.pi_acphy != NULL) {
		MFREE(osh, phy_info.u.pi_acphy, sizeof(*phy_info.u.pi_acphy));
		phy_info.u.pi_acphy = NULL;
	}
	if (phy_info.sh != NULL) {
		MFREE(osh, phy_info.sh, sizeof(*phy_info.sh));
		phy_info.sh = NULL;
	}
	if (phy_info.pubpi_ro != NULL) {
		MFREE(osh, phy_info.pubpi_ro, sizeof(*phy_info.pubpi_ro));
		phy_info.pubpi_ro = NULL;
	}
	if (phy_info.pubpi != NULL) {
		MFREE(osh, phy_info.pubpi, sizeof(*phy_info.pubpi));
		phy_info.pubpi = NULL;
	}
	phy_fifo[0].idx = 0;
	phy_fifo[1].idx = 0;
}

/*
 * The START_TEST/END_TEST pair are macros that setup basic structures to permit testing.
 */

static int
read_fifo_from_file(const char *filename, uint8 ant_num)
{
	int fd;
	char filepath[80];

	(void) snprintf(filepath, 80, "../../../RADARS/%s", filename);

	fd = open(filepath, 0, O_RDONLY);

	if (fd >= 0) {
		radar_fifo_t *radar_fifo = &phy_fifo[ant_num];
		uint16 idx = 0;

		ASSERT(radar_fifo->idx == 0);

		/* fill phy_fifo[ant_num].buffer */
		while (read(fd, &radar_fifo->buffer[idx++], 2) == 2);

		ASSERT(idx - 1 <= FIFO_ENTRIES);

		radar_fifo->num_fifo_entries = idx - 1;

		/* pad remainder of buffer with zeros */
		while (idx < FIFO_ENTRIES)
			radar_fifo->buffer[idx++] = 0;

		(void)close(fd);
	} else {
		perror(filepath);
	}

	return fd;
}

static char *ant1_filename(const char *ant0_filename)
{
	static char target_filename[80];
	char *p;

	(void) strncpy(target_filename, ant0_filename, 80);
	p = strstr(target_filename, "_ant0");

	ASSERT(p != NULL);

	/* change "ant0" to "ant1" */
	*(p + 4) = '1';

	return target_filename;
}

static void
run_radar_test(const test_table_t radar[])
{
	uint8 table_idx;
	uint8 radar_type;
	radar_detected_info_t radar_detected;

	for (table_idx = 0; radar[table_idx].filename != NULL; ++table_idx) {
		phy_fifo[0].idx = 0;
		phy_fifo[1].idx = 0;

		ck_assert_int_ge(read_fifo_from_file(radar[table_idx].filename, 0), 0);
		ck_assert_int_ge(read_fifo_from_file(ant1_filename(radar[table_idx].filename), 1),
		                 0);
		phy_info.bw = radar[table_idx].bw;

		radar_type = phy_radar_run_nphy(&phy_info, &radar_detected);

		ASSERT(radar_type == radar_detected.radar_type);

		printf("%s: radar_type=%d, pw=[%u,%u] interval=[%u,%u]\n",
			radar[table_idx].filename,
			radar_detected.radar_type,
			radar_detected.min_pw, radar_detected.max_pw,
			radar_detected.min_pri, radar_detected.max_pri);

		ck_assert_uint_eq(radar_type, radar[table_idx].radar_type);
		ck_assert_uint_eq(radar_detected.min_pw, radar[table_idx].min_pw);
		ck_assert_uint_eq(radar_detected.min_pri, radar[table_idx].radar_interval);
	}
}

static test_table_t radar_none[] = {
	{ "radar_fcc-not_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_NONE },
	{ "radar_fcc-not_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_NONE },
	{ "radar_fcc-not_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_NONE },
	{ "radar_jp-not_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_NONE },
	{ "radar_jp-not_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_NONE },
	{ "radar_jp-not_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_NONE },
	{ "radar_etsi-prf-not_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_NONE },
	{ "radar_etsi-prf-not_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_NONE },
	{ "radar_etsi-prf-not_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_NONE },
	{ "radar_etsi-stag2-not_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_NONE },
	{ "radar_etsi-stag2-not_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_NONE },
	{ "radar_etsi-stag2-not_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_NONE },
	{ "radar_etsi-stag3-not_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_NONE },
	{ "radar_etsi-stag3-not_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_NONE },
	{ "radar_etsi-stag3-not_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_NONE },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_none)
{
	/* turn on all radar detection algorithms */
	phy_info.radari->st->rparams.radar_args.feature_mask |=
		(RADAR_FEATURE_FCC_DETECT | RADAR_FEATURE_ETSI_DETECT);

	/* these test cases have parameters just outside the radar types */
	run_radar_test(radar_none);
}
END_TEST

static test_table_t radar_captured_etsi0[] = {
	{ "radcap_1us_1428us_0MHz_-25dBm_bw20_ant0.bin", 44, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_0MHz_-45dBm_bw20_ant0.bin", 27, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_1MHz_-25dBm_bw20_ant0.bin", 41, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_1MHz_-45dBm_bw20_ant0.bin", 50, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_1MHz_-65dBm_bw20_ant0.bin", 25, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_5MHz_-25dBm_bw20_ant0.bin", 38, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_5MHz_-45dBm_bw20_ant0.bin", 50, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_5MHz_-65dBm_bw20_ant0.bin", 24, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ "radcap_1us_1428us_8MHz_-65dBm_bw20_ant0.bin", 24, 1428,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_0 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_ETSI0)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_captured_etsi0);
}
END_TEST

static test_table_t radar_captured_etsi1[] = {
	{ "radcap_1us_5ms_0MHz_-45dBm_bw20_ant0.bin", 43, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radcap_1us_5ms_0MHz_-65dBm_bw20_ant0.bin", 27, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radcap_1us_5ms_1MHz_-45dBm_bw20_ant0.bin", 41, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radcap_1us_5ms_1MHz_-65dBm_bw20_ant0.bin", 25, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radcap_1us_5ms_5MHz_-45dBm_bw20_ant0.bin", 13, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radcap_1us_5ms_5MHz_-65dBm_bw20_ant0.bin", 24, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radcap_1us_5ms_8MHz_-45dBm_bw20_ant0.bin", 14, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radcap_1us_5ms_8MHz_-65dBm_bw20_ant0.bin", 24, 5028,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_ETSI1)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_captured_etsi1);
}
END_TEST

static test_table_t radar_etsi1[] = {
	{ "radar_etsi-1_bw20_ant0.bin", 19, 1666, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-1_bw40_ant0.bin", 19, 1666, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-1_bw80_ant0.bin", 19, 1666, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-1_bw20_ant0.bin", 99, 5000,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-1_bw40_ant0.bin", 99, 5000,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-1_bw80_ant0.bin", 99, 5000,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI1)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi1);
}
END_TEST

static test_table_t radar_captured_etsi2[] = {
	{ "radcap_15us_5ms_0MHz_-45dBm_bw20_ant0.bin", 330, 5020,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radcap_15us_5ms_0MHz_-65dBm_bw20_ant0.bin", 306, 5020,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radcap_15us_5ms_1MHz_-45dBm_bw20_ant0.bin", 291, 5019,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radcap_15us_5ms_1MHz_-65dBm_bw20_ant0.bin", 304, 5020,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radcap_15us_5ms_5MHz_-45dBm_bw20_ant0.bin", 288, 5019,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radcap_15us_5ms_5MHz_-65dBm_bw20_ant0.bin", 325, 5020,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radcap_15us_5ms_8MHz_-45dBm_bw20_ant0.bin", 281, 5019,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radcap_15us_5ms_8MHz_-65dBm_bw20_ant0.bin", 304, 5020,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_ETSI2)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_captured_etsi2);
}
END_TEST

static test_table_t radar_etsi2[] = {
	{ "radar_etsi-2_bw20_ant0.bin", 159, 1111, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radar_etsi-2_bw40_ant0.bin", 159, 1111, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_2 },
	{ "radar_etsi-2_bw80_ant0.bin", 159, 1111, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-max-etsi-2_bw20_ant0.bin", 299, 5000,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-max-etsi-2_bw40_ant0.bin", 299, 5000,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-max-etsi-2_bw80_ant0.bin", 299, 5000,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI2)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi2);
}
END_TEST

static test_table_t radar_captured_etsi3[] = {
	{ "radcap_15us_250us_0MHz_-45dBm_bw20_ant0.bin", 307, 250,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_3 },
	{ "radcap_15us_250us_0MHz_-65dBm_bw20_ant0.bin", 338, 250,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_3 },
	{ "radcap_15us_250us_5MHz_-45dBm_bw20_ant0.bin", 326, 250,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_3 },
	{ "radcap_15us_250us_5MHz_-65dBm_bw20_ant0.bin", 333, 250,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_3 },
	{ "radcap_15us_250us_8MHz_-45dBm_bw20_ant0.bin", 304, 250,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_ETSI3)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_captured_etsi3);
}
END_TEST

static test_table_t radar_etsi3[] = {
	{ "radar_etsi-3_bw20_ant0.bin", 159, 317, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_3 },
	{ "radar_etsi-3_bw40_ant0.bin", 159, 317, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_3 },
	{ "radar_etsi-3_bw80_ant0.bin", 159, 317, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_3 },
	{ "radar_zcorner-max-etsi-3_bw20_ant0.bin", 299, 434,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_3 },
	{ "radar_zcorner-max-etsi-3_bw40_ant0.bin", 299, 434,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_3 },
	{ "radar_zcorner-max-etsi-3_bw80_ant0.bin", 299, 434,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI3)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi3);
}
END_TEST

static test_table_t radar_captured_etsi4[] = {
	{ "radcap_30us_500us_5FM_0MHz_-45dBm_bw20_ant0.bin", 591, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ "radcap_30us_500us_5FM_1MHz_-45dBm_bw20_ant0.bin", 591, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ "radcap_30us_500us_5FM_5MHz_-45dBm_bw20_ant0.bin", 607, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ "radcap_30us_500us_5FM_5MHz_-65dBm_bw20_ant0.bin", 603, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ "radcap_30us_500us_5FM_8MHz_-45dBm_bw20_ant0.bin", 601, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ "radcap_30us_500us_5FM_8MHz_-65dBm_bw20_ant0.bin", 610, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_ETSI4)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_captured_etsi4);
}
END_TEST

static test_table_t radar_etsi4[] = {
	{ "radar_etsi-4_bw20_ant0.bin", 499, 333, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ "radar_etsi-4_bw40_ant0.bin", 499, 333, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_4 },
	{ "radar_etsi-4_bw80_ant0.bin", 500, 333, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_4 },
	{ "radar_zcorner-max-etsi-4_bw20_ant0.bin", 599, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_4 },
	{ "radar_zcorner-max-etsi-4_bw40_ant0.bin", 599, 500,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_4 },
	{ "radar_zcorner-max-etsi-4_bw80_ant0.bin", 600, 500,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_4 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI4)
{
	/* enable ETSI radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi4);
}
END_TEST

/*
 * Note: Some Staggered Radar signals will be detected as RADAR_TYPE_ETSI_1 and some others will
 *       be detected as RADAR_TYPE_ETSI_5_STG2.
 */
static test_table_t radar_etsi5_stg2[] = {
	{ "radar_etsi-5-2aabb_bw20_ant0.bin", 27, 2941, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-5-2aabb_bw40_ant0.bin", 27, 2941, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-5-2aabb_bw80_ant0.bin", 27, 2941, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-5-2aabb_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-5-2aabb_bw40_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-5-2aabb_bw80_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-5-2aabb_bw20_ant0.bin", 39, 3333,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-5-2aabb_bw40_ant0.bin", 39, 3333,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-5-2aabb_bw80_ant0.bin", 39, 3333,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-5-2abab_bw20_ant0.bin", 27, 2941, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_etsi-5-2abab_bw40_ant0.bin", 28, 2941, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_etsi-5-2abab_bw80_ant0.bin", 28, 2941, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_zcorner-min-etsi-5-2abab_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_zcorner-min-etsi-5-2abab_bw40_ant0.bin", 40, 2500,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_zcorner-min-etsi-5-2abab_bw80_ant0.bin", 40, 2500,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_zcorner-max-etsi-5-2abab_bw20_ant0.bin", 39, 3333,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_zcorner-max-etsi-5-2abab_bw40_ant0.bin", 40, 3333,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_5_STG2 },
	{ "radar_zcorner-max-etsi-5-2abab_bw80_ant0.bin", 40, 3333,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_5_STG2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI5_STG2)
{
	/* enable ETSI staggered radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi5_stg2);
}
END_TEST

/*
 * Note: Some Staggered Radar signals will be detected as RADAR_TYPE_ETSI_1 and some others will
 *       be detected as RADAR_TYPE_ETSI_5_STG3.
 */
static test_table_t radar_etsi5_stg3[] = {
	{ "radar_etsi-5-3aabb_bw20_ant0.bin", 27, 3030, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-5-3aabb_bw40_ant0.bin", 27, 2857, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-5-3aabb_bw80_ant0.bin", 27, 2702, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-5-3aabb_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-5-3aabb_bw40_ant0.bin", 39, 2564,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-5-3aabb_bw80_ant0.bin", 39, 2631,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-5-3aabb_bw20_ant0.bin", 39, 3333,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-5-3aabb_bw40_ant0.bin", 39, 3225,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-5-3aabb_bw80_ant0.bin", 39, 3125,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-5-3abab_bw20_ant0.bin", 27, 3030, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_etsi-5-3abab_bw40_ant0.bin", 28, 2857, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_etsi-5-3abab_bw80_ant0.bin", 28, 2702, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_zcorner-min-etsi-5-3abab_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_zcorner-min-etsi-5-3abab_bw40_ant0.bin", 40, 2564,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_zcorner-min-etsi-5-3abab_bw80_ant0.bin", 40, 2631,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_zcorner-max-etsi-5-3abab_bw20_ant0.bin", 39, 3333,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_zcorner-max-etsi-5-3abab_bw40_ant0.bin", 40, 3225,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_5_STG3 },
	{ "radar_zcorner-max-etsi-5-3abab_bw80_ant0.bin", 40, 3125,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_5_STG3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI5_STG3)
{
	/* enable ETSI staggered radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi5_stg3);
}
END_TEST

/*
 * Note: Some Staggered Radar signals will be detected as RADAR_TYPE_ETSI_1 and some others will
 *       be detected as RADAR_TYPE_ETSI_6_STG2.
 */
static test_table_t radar_etsi6_stg2[] = {
	{ "radar_etsi-6-2aabb_bw20_ant0.bin", 27, 1315, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-6-2aabb_bw40_ant0.bin", 27, 1190, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-6-2aabb_bw80_ant0.bin", 27, 1190, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-6-2aabb_bw20_ant0.bin", 39, 833,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-min-etsi-6-2aabb_bw40_ant0.bin", 39, 840,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-min-etsi-6-2aabb_bw80_ant0.bin", 39, 840,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-max-etsi-6-2aabb_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-6-2aabb_bw40_ant0.bin", 39, 2439,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-6-2aabb_bw80_ant0.bin", 39, 2439,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-6-2abab_bw20_ant0.bin", 27, 1315, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_etsi-6-2abab_bw40_ant0.bin", 28, 1190, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_etsi-6-2abab_bw80_ant0.bin", 28, 1190, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_zcorner-min-etsi-6-2abab_bw20_ant0.bin", 39, 833,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_zcorner-min-etsi-6-2abab_bw40_ant0.bin", 40, 840,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_zcorner-min-etsi-6-2abab_bw80_ant0.bin", 40, 840,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_zcorner-max-etsi-6-2abab_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_zcorner-max-etsi-6-2abab_bw40_ant0.bin", 40, 2439,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_6_STG2 },
	{ "radar_zcorner-max-etsi-6-2abab_bw80_ant0.bin", 40, 2439,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_6_STG2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI6_STG2)
{
	/* enable ETSI staggered radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi6_stg2);
}
END_TEST

/*
 * Note: Some Staggered Radar signals will be detected as RADAR_TYPE_ETSI_1 and some others will
 *       be detected as RADAR_TYPE_ETSI_6_STG3.
 */
static test_table_t radar_etsi6_stg3[] = {
	{ "radar_etsi-6-3aabb_bw20_ant0.bin", 27, 1388, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-6-3aabb_bw40_ant0.bin", 27, 1136, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-6-3aabb_bw80_ant0.bin", 27, 1388, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-min-etsi-6-3aabb_bw20_ant0.bin", 39, 833,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-min-etsi-6-3aabb_bw40_ant0.bin", 39, 847,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-min-etsi-6-3aabb_bw80_ant0.bin", 39, 833,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_2 },
	{ "radar_zcorner-max-etsi-6-3aabb_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-6-3aabb_bw40_ant0.bin", 39, 2380,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_1 },
	{ "radar_zcorner-max-etsi-6-3aabb_bw80_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_1 },
	{ "radar_etsi-6-3abab_bw20_ant0.bin", 27, 1388, WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_etsi-6-3abab_bw40_ant0.bin", 28, 1136, WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_etsi-6-3abab_bw80_ant0.bin", 28, 1388, WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_zcorner-min-etsi-6-3abab_bw20_ant0.bin", 39, 833,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_zcorner-min-etsi-6-3abab_bw40_ant0.bin", 40, 847,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_zcorner-min-etsi-6-3abab_bw80_ant0.bin", 40, 833,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_zcorner-max-etsi-6-3abab_bw20_ant0.bin", 39, 2500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_zcorner-max-etsi-6-3abab_bw40_ant0.bin", 40, 2380,
	WL_CHANSPEC_BW_40, RADAR_TYPE_ETSI_6_STG3 },
	{ "radar_zcorner-max-etsi-6-3abab_bw80_ant0.bin", 40, 2500,
	WL_CHANSPEC_BW_80, RADAR_TYPE_ETSI_6_STG3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_ETSI6_STG3)
{
	/* enable ETSI staggered radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_ETSI_DETECT;

	run_radar_test(radar_etsi6_stg3);
}
END_TEST

static test_table_t radar_unclassified[] = {
	{ "radar_fcc-0_bw20_ant0.bin", 19, 1428, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_0 },
	{ "radar_fcc-0_bw40_ant0.bin", 19, 1428, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_0 },
	{ "radar_fcc-0_bw80_ant0.bin", 19, 1428, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_0 },
	{ "radar_fcc-1_bw20_ant0.bin", 19, 738, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_1 },
	{ "radar_fcc-1_bw40_ant0.bin", 19, 738, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_1 },
	{ "radar_fcc-1_bw80_ant0.bin", 19, 738, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_1 },
	{ "radar_fcc-2_bw20_ant0.bin", 59, 190, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radar_fcc-2_bw40_ant0.bin", 59, 190, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_2 },
	{ "radar_fcc-2_bw80_ant0.bin", 59, 190, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-min-fcc-2_bw20_ant0.bin", 99, 230, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-min-fcc-2_bw40_ant0.bin", 99, 230, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-min-fcc-2_bw80_ant0.bin", 99, 230, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-max-fcc-2_bw20_ant0.bin", 99, 150, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-max-fcc-2_bw40_ant0.bin", 99, 150, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-max-fcc-2_bw80_ant0.bin", 99, 150, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_2 },
	{ "radar_fcc-3_bw20_ant0.bin", 159, 350, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radar_fcc-3_bw40_ant0.bin", 159, 350, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_3 },
	{ "radar_fcc-3_bw80_ant0.bin", 159, 350, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-min-fcc-3_bw20_ant0.bin", 199, 500, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-min-fcc-3_bw40_ant0.bin", 199, 500, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-min-fcc-3_bw80_ant0.bin", 199, 500, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-max-fcc-3_bw20_ant0.bin", 199, 200, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-max-fcc-3_bw40_ant0.bin", 199, 200, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-max-fcc-3_bw80_ant0.bin", 199, 200, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_3 },
	{ "radar_fcc-4_bw20_ant0.bin", 319, 350, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radar_fcc-4_bw40_ant0.bin", 319, 350, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_4 },
	{ "radar_fcc-4_bw80_ant0.bin", 319, 350, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-min-fcc-4_bw20_ant0.bin", 399, 500, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-min-fcc-4_bw40_ant0.bin", 399, 500, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-min-fcc-4_bw80_ant0.bin", 399, 500, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-max-fcc-4_bw20_ant0.bin", 399, 200, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-max-fcc-4_bw40_ant0.bin", 399, 200, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-max-fcc-4_bw80_ant0.bin", 399, 200, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_4 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_unclassified)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_unclassified);
}
END_TEST

static test_table_t radar_fcc5[] = {
	{ "radar_fcc-5_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_5 },
	{ "radar_fcc-5_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_5 },
	{ "radar_fcc-5_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_5 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_FCC5)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_fcc5);
}
END_TEST

static test_table_t radar_min_fcc5[] = {
	{ "radar_zcorner-min-fcc-5_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-min-fcc-5_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-min-fcc-5_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_5 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_min_FCC5)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_min_fcc5);
}
END_TEST

static test_table_t radar_max_fcc5[] = {
	{ "radar_zcorner-max-fcc-5_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-max-fcc-5_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-max-fcc-5_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_5 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_max_FCC5)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_max_fcc5);
}
END_TEST

/*
 * Note: Some JP Radar signals will be detected as RADAR_TYPE_FCC_1 and some others will
 *       be detected as RADAR_TYPE_JP1_2.
 */
static test_table_t radar_jp1[] = {
	{ "radar_jp-1-1_bw20_ant0.bin", 19, 1428, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_0 },
	{ "radar_jp-1-1_bw40_ant0.bin", 19, 1428, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_0 },
	{ "radar_jp-1-1_bw80_ant0.bin", 19, 1428, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_0 },
	{ "radar_jp-1-2_bw20_ant0.bin", 49, 3846, WL_CHANSPEC_BW_20, RADAR_TYPE_JP1_2 },
	{ "radar_jp-1-2_bw40_ant0.bin", 49, 3846, WL_CHANSPEC_BW_40, RADAR_TYPE_JP1_2 },
	{ "radar_jp-1-2_bw80_ant0.bin", 49, 3846, WL_CHANSPEC_BW_80, RADAR_TYPE_JP1_2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_JP1)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_jp1);
}
END_TEST

static test_table_t radar_captured_fcc2[] = {
	{ "radcap_1us_230us_0MHz_-45dBm_bw20_ant0.bin", 27, 230,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radcap_1us_230us_5MHz_-45dBm_bw20_ant0.bin", 49, 230,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radcap_1us_230us_8MHz_-45dBm_bw20_ant0.bin", 25, 230,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radcap_1us_230us_8MHz_-65dBm_bw20_ant0.bin", 24, 230,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_FCC2)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_captured_fcc2);
}
END_TEST

static test_table_t radar_captured_fcc3[] = {
	{ "radcap_10us_200us_0MHz_-45dBm_bw20_ant0.bin", 207, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_10us_200us_0MHz_-65dBm_bw20_ant0.bin", 238, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_10us_200us_1MHz_-45dBm_bw20_ant0.bin", 205, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_10us_200us_1MHz_-65dBm_bw20_ant0.bin", 205, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_10us_200us_5MHz_-45dBm_bw20_ant0.bin", 204, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_10us_200us_5MHz_-65dBm_bw20_ant0.bin", 211, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_10us_200us_8MHz_-45dBm_bw20_ant0.bin", 204, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_10us_200us_8MHz_-65dBm_bw20_ant0.bin", 208, 200,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_FCC3)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_captured_fcc3);
}
END_TEST

/*
 * Note: Some of these signals are being detected as FCC3 because 11us PW is accepted given
 *       a 2.5us tolerance in the maximum PW for FCC3.
 */
static test_table_t radar_captured_fcc4[] = {
	{ "radcap_11us_500us_0MHz_-45dBm_bw20_ant0.bin", 227, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_11us_500us_0MHz_-65dBm_bw20_ant0.bin", 257, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radcap_11us_500us_1MHz_-45dBm_bw20_ant0.bin", 225, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_11us_500us_1MHz_-65dBm_bw20_ant0.bin", 224, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_11us_500us_5MHz_-45dBm_bw20_ant0.bin", 224, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_11us_500us_5MHz_-65dBm_bw20_ant0.bin", 301, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radcap_11us_500us_8MHz_-45dBm_bw20_ant0.bin", 225, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radcap_11us_500us_8MHz_-65dBm_bw20_ant0.bin", 228, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_FCC4)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_captured_fcc4);
}
END_TEST

/*
 * Note: Some JP Radar signals will be detected as FCC types, as they are the same.
 */
static test_table_t radar_jp2[] = {
	{ "radar_jp-2-1-1_bw20_ant0.bin", 9, 1388, WL_CHANSPEC_BW_20, RADAR_TYPE_JP2_1_1 },
	{ "radar_jp-2-1-1_bw40_ant0.bin", 9, 1388, WL_CHANSPEC_BW_40, RADAR_TYPE_JP2_1_1 },
	{ "radar_jp-2-1-1_bw80_ant0.bin", 9, 1388, WL_CHANSPEC_BW_80, RADAR_TYPE_JP2_1_1 },
	{ "radar_jp-2-1-2_bw20_ant0.bin", 39, 4000, WL_CHANSPEC_BW_20, RADAR_TYPE_JP2_1_2 },
	{ "radar_jp-2-1-2_bw40_ant0.bin", 39, 4000, WL_CHANSPEC_BW_40, RADAR_TYPE_JP2_1_2 },
	{ "radar_jp-2-1-2_bw80_ant0.bin", 39, 4000, WL_CHANSPEC_BW_80, RADAR_TYPE_JP2_1_2 },
	{ "radar_jp-2-2-1_bw20_ant0.bin", 19, 1428, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_0 },
	{ "radar_jp-2-2-1_bw40_ant0.bin", 19, 1428, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_0 },
	{ "radar_jp-2-2-1_bw80_ant0.bin", 19, 1428, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_0 },
	{ "radar_jp-2-2-2_bw20_ant0.bin", 59, 200, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radar_jp-2-2-2_bw40_ant0.bin", 59, 200, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_2 },
	{ "radar_jp-2-2-2_bw80_ant0.bin", 59, 200, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-min-jp-2-2-2_bw20_ant0.bin", 99, 229,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-min-jp-2-2-2_bw40_ant0.bin", 99, 229,
	WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-min-jp-2-2-2_bw80_ant0.bin", 99, 229,
	WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-max-jp-2-2-2_bw20_ant0.bin", 99, 150,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-max-jp-2-2-2_bw40_ant0.bin", 99, 150,
	WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_2 },
	{ "radar_zcorner-max-jp-2-2-2_bw80_ant0.bin", 99, 150,
	WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_2 },
	{ "radar_jp-2-2-3_bw20_ant0.bin", 159, 333, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radar_jp-2-2-3_bw40_ant0.bin", 159, 333, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_3 },
	{ "radar_jp-2-2-3_bw80_ant0.bin", 159, 333, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-min-jp-2-2-3_bw20_ant0.bin", 199, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-min-jp-2-2-3_bw40_ant0.bin", 199, 500,
	WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-min-jp-2-2-3_bw80_ant0.bin", 199, 500,
	WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-max-jp-2-2-3_bw20_ant0.bin", 199, 250,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-max-jp-2-2-3_bw40_ant0.bin", 199, 250,
	WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_3 },
	{ "radar_zcorner-max-jp-2-2-3_bw80_ant0.bin", 199, 250,
	WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_3 },
	{ "radar_jp-2-2-4_bw20_ant0.bin", 309, 333, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radar_jp-2-2-4_bw40_ant0.bin", 309, 333, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_4 },
	{ "radar_jp-2-2-4_bw80_ant0.bin", 309, 333, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-min-jp-2-2-4_bw20_ant0.bin", 399, 500,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-min-jp-2-2-4_bw40_ant0.bin", 399, 500,
	WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-min-jp-2-2-4_bw80_ant0.bin", 399, 500,
	WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-max-jp-2-2-4_bw20_ant0.bin", 399, 250,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-max-jp-2-2-4_bw40_ant0.bin", 399, 250,
	WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_4 },
	{ "radar_zcorner-max-jp-2-2-4_bw80_ant0.bin", 399, 250,
	WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_4 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_JP2)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_jp2);
}
END_TEST

/*
 * Note: The JP 3-1 Radar signals will be detected as RADAR_TYPE_FCC_5, because it's the same.
 */
static test_table_t radar_jp3[] = {
	{ "radar_jp-3-1_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_5 },
	{ "radar_jp-3-1_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_5 },
	{ "radar_jp-3-1_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_5 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_JP3)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_jp3);
}
END_TEST

static test_table_t radar_min_jp3[] = {
	{ "radar_zcorner-min-jp-3-1_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-min-jp-3-1_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-min-jp-3-1_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_5 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_min_JP3)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_min_jp3);
}
END_TEST

static test_table_t radar_max_jp3[] = {
	{ "radar_zcorner-max-jp-3-1_bw20_ant0.bin", 0, 0, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-max-jp-3-1_bw40_ant0.bin", 0, 0, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_5 },
	{ "radar_zcorner-max-jp-3-1_bw80_ant0.bin", 0, 0, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_5 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_max_JP3)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_max_jp3);
}
END_TEST

static test_table_t radar_jp4[] = {
	{ "radar_jp-4-1_bw20_ant0.bin", 19, 333, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radar_jp-4-1_bw40_ant0.bin", 19, 333, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_6 },
	{ "radar_jp-4-1_bw80_ant0.bin", 19, 333, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_6 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_JP4)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_jp4);
}
END_TEST

/*
 * Note: The Korean 1 Radar signal will be detected as RADAR_TYPE_FCC_0, because it's the same.
 */
static test_table_t radar_korean1[] = {
	{ "radar_kor-1_bw20_ant0.bin", 19, 1428, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_0 },
	{ "radar_kor-1_bw40_ant0.bin", 19, 1428, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_0 },
	{ "radar_kor-1_bw80_ant0.bin", 19, 1428, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_0 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_KOREAN1)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_korean1);
}
END_TEST

static test_table_t radar_captured_korean2[] = {
	{ "radcap_1us_556us_0MHz_-45dBm_bw20_ant0.bin", 28, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radcap_1us_556us_0MHz_-65dBm_bw20_ant0.bin", 27, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radcap_1us_556us_1MHz_-45dBm_bw20_ant0.bin", 50, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radcap_1us_556us_1MHz_-65dBm_bw20_ant0.bin", 25, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radcap_1us_556us_5MHz_-45dBm_bw20_ant0.bin", 48, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radcap_1us_556us_5MHz_-65dBm_bw20_ant0.bin", 24, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radcap_1us_556us_8MHz_-45dBm_bw20_ant0.bin", 25, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radcap_1us_556us_8MHz_-65dBm_bw20_ant0.bin", 42, 555,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_KOREAN2)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_captured_korean2);
}
END_TEST

static test_table_t radar_korean2[] = {
	{ "radar_kor-2_bw20_ant0.bin", 19, 556, WL_CHANSPEC_BW_20, RADAR_TYPE_KN2 },
	{ "radar_kor-2_bw40_ant0.bin", 19, 556, WL_CHANSPEC_BW_40, RADAR_TYPE_KN2 },
	{ "radar_kor-2_bw80_ant0.bin", 19, 556, WL_CHANSPEC_BW_80, RADAR_TYPE_KN2 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_KOREAN2)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_korean2);
}
END_TEST

static test_table_t radar_captured_korean3[] = {
	{ "radcap_2us_3030us_0MHz_-45dBm_bw20_ant0.bin", 47, 3023,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radcap_2us_3030us_0MHz_-65dBm_bw20_ant0.bin", 47, 3023,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radcap_2us_3030us_1MHz_-45dBm_bw20_ant0.bin", 49, 3023,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radcap_2us_3030us_1MHz_-65dBm_bw20_ant0.bin", 50, 3023,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radcap_2us_3030us_5MHz_-45dBm_bw20_ant0.bin", 48, 3023,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radcap_2us_3030us_5MHz_-65dBm_bw20_ant0.bin", 48, 3023,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radcap_2us_3030us_8MHz_-45dBm_bw20_ant0.bin", 50, 3022,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radcap_2us_3030us_8MHz_-65dBm_bw20_ant0.bin", 49, 3023,
	WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_KOREAN3)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_captured_korean3);
}
END_TEST

static test_table_t radar_korean3[] = {
	{ "radar_kor-3_bw20_ant0.bin", 39, 3030, WL_CHANSPEC_BW_20, RADAR_TYPE_KN3 },
	{ "radar_kor-3_bw40_ant0.bin", 39, 3030, WL_CHANSPEC_BW_40, RADAR_TYPE_KN3 },
	{ "radar_kor-3_bw80_ant0.bin", 39, 3030, WL_CHANSPEC_BW_80, RADAR_TYPE_KN3 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_KOREAN3)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_korean3);
}
END_TEST

static test_table_t radar_captured_korean4[] = {
	{ "radcap_1us_337us_0MHz_-45dBm_bw20_ant0.bin", 28, 336,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radcap_1us_337us_1MHz_-45dBm_bw20_ant0.bin", 50, 336,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radcap_1us_337us_1MHz_-65dBm_bw20_ant0.bin", 25, 336,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radcap_1us_337us_5MHz_-45dBm_bw20_ant0.bin", 49, 336,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radcap_1us_337us_5MHz_-65dBm_bw20_ant0.bin", 24, 336,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radcap_1us_337us_8MHz_-45dBm_bw20_ant0.bin", 25, 336,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radcap_1us_337us_8MHz_-65dBm_bw20_ant0.bin", 24, 336,
	WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_captured_KOREAN4)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_captured_korean4);
}
END_TEST

static test_table_t radar_korean4[] = {
	{ "radar_kor-4_bw20_ant0.bin", 19, 333, WL_CHANSPEC_BW_20, RADAR_TYPE_FCC_6 },
	{ "radar_kor-4_bw40_ant0.bin", 19, 333, WL_CHANSPEC_BW_40, RADAR_TYPE_FCC_6 },
	{ "radar_kor-4_bw80_ant0.bin", 19, 333, WL_CHANSPEC_BW_80, RADAR_TYPE_FCC_6 },
	{ NULL, 0, 0, 0, 0 }
};

START_TEST(test_radar_KOREAN4)
{
	/* enable FCC radar detection algorithm */
	phy_info.radari->st->rparams.radar_args.feature_mask |= RADAR_FEATURE_FCC_DETECT;

	run_radar_test(radar_korean4);
}
END_TEST

/*
 * Suite of test cases which check radar types for each given set of inputs.
 */
Suite *phy_radar_suite(void)
{
	/* Suite creation */
	Suite *s = suite_create("phy_radar_run");

	/* Test case creation */
	TCase *tc = tcase_create("Test Case");

#ifdef BCMDBG_ERR
	phyhal_msg_level |= PHYHAL_ERROR;
#endif	/* BCMDBG_ERR */
#ifdef BCMDBG
	phyhal_msg_level |= PHYHAL_RADAR;
#endif	/* BCMDBG */

	tcase_add_checked_fixture(tc, setup, teardown);

	/* Adding unit tests to test case */
	tcase_add_test(tc, test_radar_none);
	tcase_add_test(tc, test_radar_captured_ETSI0);
	tcase_add_test(tc, test_radar_captured_ETSI1);
	tcase_add_test(tc, test_radar_captured_ETSI2);
	tcase_add_test(tc, test_radar_captured_ETSI3);
	tcase_add_test(tc, test_radar_captured_ETSI4);
	tcase_add_test(tc, test_radar_ETSI1);
	tcase_add_test(tc, test_radar_ETSI2);
	tcase_add_test(tc, test_radar_ETSI3);
	tcase_add_test(tc, test_radar_ETSI4);
	tcase_add_test(tc, test_radar_ETSI5_STG2);
	tcase_add_test(tc, test_radar_ETSI5_STG3);
	tcase_add_test(tc, test_radar_ETSI6_STG2);
	tcase_add_test(tc, test_radar_ETSI6_STG3);
	tcase_add_test(tc, test_radar_unclassified);
	tcase_add_test(tc, test_radar_captured_FCC2);
	tcase_add_test(tc, test_radar_captured_FCC3);
	tcase_add_test(tc, test_radar_captured_FCC4);
	tcase_add_test(tc, test_radar_FCC5);
	tcase_add_test(tc, test_radar_min_FCC5);
	tcase_add_test(tc, test_radar_max_FCC5);
	tcase_add_test(tc, test_radar_JP1);
	tcase_add_test(tc, test_radar_JP2);
	tcase_add_test(tc, test_radar_JP3);
	tcase_add_test(tc, test_radar_min_JP3);
	tcase_add_test(tc, test_radar_max_JP3);
	tcase_add_test(tc, test_radar_JP4);
	tcase_add_test(tc, test_radar_captured_KOREAN2);
	tcase_add_test(tc, test_radar_captured_KOREAN3);
	tcase_add_test(tc, test_radar_captured_KOREAN4);
	tcase_add_test(tc, test_radar_KOREAN1);
	tcase_add_test(tc, test_radar_KOREAN2);
	tcase_add_test(tc, test_radar_KOREAN3);
	tcase_add_test(tc, test_radar_KOREAN4);

	/* Adding test case to the Suite */
	suite_add_tcase(s, tc);

	return s;
}
