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
 * $Id: main.c 606042 2015-12-14 06:21:23Z $
 */


/***************************************************************************************************
************* Definitions for module components to be tested with Check tool ***********************
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "test_phy_radar.h"

/***************************************************************************************************
*********************************** Start of Test Section ******************************************
*/

#include <check.h> /* Includes Check framework */

/*
 * Main flow:
 * 1. Define SRunner object which will aggregate all suites.
 * 2. Adding all suites to SRunner which enables consecutive suite(s) running.
 * 3. Running all suites contained in SRunner.
 */

int main(void)
{
	int number_failed;	// Count number of failed tests

	/* Add suite to SRunner */
	SRunner *sr = srunner_create(phy_radar_suite());

	/* Add more suites to SRunner */

	/* Prints the summary one message per test (passed or failed) */
	srunner_run_all(sr, CK_VERBOSE);

	/* count all the failed tests */
	number_failed = srunner_ntests_failed(sr);

	/* cleanup */
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
