/*
 * acsd_osl.h
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 */

#ifndef _acsd_osl_h_
#define _acsd_osl_h_

#include <stdlib.h>
#include <bcmtimer.h>

#define acsd_free(data_ptr) \
	do { \
		if (data_ptr) { \
			ACSD_DEBUG("free address:0x%p\n", data_ptr); \
			free(data_ptr); \
		} \
		data_ptr = NULL; \
	} while (0)

#endif /*  _acsd_osl_h_ */
