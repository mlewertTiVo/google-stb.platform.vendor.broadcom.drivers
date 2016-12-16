


/*
 *   Broadcom Proprietary and Confidential. Copyright (C) 2016,
 *   All Rights Reserved.
 *   
 *   This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 *   the contents of this file may not be disclosed to third parties, copied
 *   or duplicated in any form, in whole or in part, without the prior
 *   written permission of Broadcom.
 *
 *
 *   <<Broadcom-WL-IPTag/Proprietary:>>
 *
 *   $Id: wlc_pkt_filter.h 647489 2016-07-06 01:42:41Z $
 */

#ifndef _wlc_pkt_filter_h_
#define _wlc_pkt_filter_h_


/* ---- Include Files ---------------------------------------------------- */
/* ---- Constants and Types ---------------------------------------------- */


/* Forward declaration */
typedef struct pkt_filter_info	wlc_pkt_filter_info_t;


/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */

#ifdef PACKET_FILTER

/*
*****************************************************************************
* Function:   wlc_pkt_filter_attach
*
* Purpose:    Initialize packet filter private context.
*
* Parameters: context	(mod)	Common driver context.
*
* Returns:    Pointer to the packet filter private context. Returns NULL on error.
*****************************************************************************
*/
extern wlc_pkt_filter_info_t *wlc_pkt_filter_attach(void *context);

/*
*****************************************************************************
* Function:   wlc_pkt_filter_detach
*
* Purpose:    Cleanup packet filter private context.
*
* Parameters: info	(mod)	Packet filter engine private context.
*
* Returns:    Nothing.
*****************************************************************************
*/
extern void wlc_pkt_filter_detach(wlc_pkt_filter_info_t *info);


/*
*****************************************************************************
* Function:   wlc_pkt_fitler_recv_proc
*
* Purpose:    Process received frames.
*
* Parameters: info	(mod)	Packet filter engine private context.
*	      sdu	(in)	Received packet..
*
* Returns:    TRUE if received packet should be forwarded. FALSE if
*             it should be discarded.
*****************************************************************************
*/
extern bool wlc_pkt_filter_recv_proc(wlc_pkt_filter_info_t *info, const void *sdu);


/*
*****************************************************************************
* Function:   wlc_pkt_fitler_delete_filters
*
* Purpose:    Delete all filters but remain attached.
*
* Parameters: info	(mod)	Packet filter engine private context.
*
* Returns:    None.
*****************************************************************************
*/
extern void wlc_pkt_fitler_delete_filters(wlc_pkt_filter_info_t *info);
#else	/* stubs */
#define wlc_pkt_filter_attach(a)	(wlc_pkt_filter_info_t *)0x0dadbeef
#define wlc_pkt_filter_detach(a)	do {} while (0)
#define wlc_pkt_filter_recv_proc(a, b)	(TRUE)
#endif /* PACKET_FILTER */

#endif	/* _wlc_pkt_filter_h_ */
