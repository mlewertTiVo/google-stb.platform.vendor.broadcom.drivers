/*
 * Nucleus OS Support Layer
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 * $Id: nucleus_osl.h 241182 2011-02-17 21:50:03Z $
 */


#ifndef nucleus_osl_h
#define nucleus_osl_h

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Include Files ---------------------------------------------------- */

/* Map bcopy to memcpy. */
#define BWL_MAP_BCOPY_TO_MEMCPY
#include "generic_osl.h"

#include "nucleus.h"


/* ---- Constants and Types ---------------------------------------------- */

/* This is really platform specific and not OS specific. */
#ifndef BWL_NU_TICKS_PER_SECOND
#define BWL_NU_TICKS_PER_SECOND 1024
#endif

#define OSL_MSEC_TO_TICKS(msec)  ((BWL_NU_TICKS_PER_SECOND * (msec)) / 1000)

#define OSL_TICKS_TO_MSEC(ticks) ((1000 * (ticks)) / BWL_NU_TICKS_PER_SECOND)


/* Get processor cycle count */
#define OSL_GETCYCLES(x)	((x) = NU_Retrieve_Clock())

#define OSL_PREF_RANGE_LD(va, sz)
#define OSL_PREF_RANGE_ST(va, sz)


/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */


#ifdef __cplusplus
	}
#endif

#endif  /* nucleus_osl_h  */
