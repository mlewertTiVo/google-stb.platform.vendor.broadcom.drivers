/*
 * Minimal debug/trace definitions for
 * Broadcom user space security library
 *
 * Copyright (C) 1999-2018, Broadcom Corporation
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
 * $Id: debug.h,v 1.5 2011-01-04 22:29:19 $
 */
#ifndef _DEBUG_H_
#define _DEBUG_H_

/* CTX macro parameters (args):
 * (void *ctx, const char fmtstr[], ...)
*/

extern void
bcmsec_sprintf(void *, const char fmtstr[], ...);

#ifndef SECFRW_PRINT_TRACE_ENABLED
	#define SECFRW_PRINT_TRACE_ENABLED	1
#endif

#ifdef DEBUG
	#ifdef TARGETOS_symbian
		extern int SymbianOslPrintf(const char *format, ...);
		#define PRINT(args)		SymbianOslPrintf args
		#define PRINT_ERR(args)		SymbianOslPrintf args
		#define CTXERR(args)		SymbianOslPrintf args
		#define CTXPRT(args)		SymbianOslPrintf args

		#if SECFRW_PRINT_TRACE_ENABLED
			#define PRINT_TRACE(args)	SymbianOslPrintf args
			#define CTXTRC(args)		SymbianOslPrintf args
		#else
			#define PRINT_TRACE(args)
			#define CTXTRC(args)
		#endif
	#else
		#define PRINT(args)		printf args
		#define PRINT_ERR(args)		printf args

		#define CTXERR(args)		bcmsec_sprintf args
		#define CTXPRT(args)		bcmsec_sprintf args

		#if SECFRW_PRINT_TRACE_ENABLED
			#define PRINT_TRACE(args)	printf args
			#define CTXTRC(args)		bcmsec_sprintf args
		#else
			#define PRINT_TRACE(args)
			#define CTXTRC(args)
		#endif
	#endif 
#else    /* DEBUG */
	#define PRINT(args)
	#define PRINT_TRACE(args)
	#define PRINT_ERR(args)

	#define CTXERR(args)
	#define CTXPRT(args)
	#define CTXTRC(args)
#endif   /* DEBUG */

#endif /* _DEBUG_H_ */
