/*
 * bcmlimits.h
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef _BCMLIMITS_H_
#define _BCMLIMITS_H_

/* Minimum of signed integral types.  */
# define BCM_INT8_MIN		(-128)
# define BCM_INT16_MIN		(-32767-1)
# define BCM_INT32_MIN		(-2147483647-1)

/* Maximum of signed integral types.  */
# define BCM_INT8_MAX		(127)
# define BCM_INT16_MAX		(32767)
# define BCM_INT32_MAX		(2147483647)

/* Maximum of unsigned integral types.  */
# define BCM_UINT8_MAX		(255U)
# define BCM_UINT16_MAX		(65535U)
# define BCM_UINT32_MAX		(4294967295U)

#endif /* _BCMLIMITS_H_ */
