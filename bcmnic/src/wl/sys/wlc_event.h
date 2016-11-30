/*
 * Event mechanism
 *
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
 * $Id: wlc_event.h 633668 2016-04-25 04:52:31Z $
 */


#ifndef _WLC_EVENT_H_
#define _WLC_EVENT_H_

#include <typedefs.h>
#include <proto/ethernet.h>
#include <proto/bcmevent.h>
#include <wlc_types.h>

wlc_eventq_t *wlc_eventq_attach(wlc_info_t *wlc);
void wlc_eventq_detach(wlc_eventq_t *eq);

wlc_event_t *wlc_event_alloc(wlc_eventq_t *eq, uint32 event_id);
void *wlc_event_data_alloc(wlc_eventq_t *eq, uint32 datalen, uint32 event_id);
int wlc_event_data_free(wlc_eventq_t *eq, void *data, uint32 datalen);

void wlc_event_free(wlc_eventq_t *eq, wlc_event_t *e);

#ifdef WLNOEIND
#define wlc_eventq_test_ind(a, b) FALSE
#define wlc_eventq_set_ind(a, b, c) do {} while (0)
#define wlc_eventq_set_all_ind(a, b, c, d) do {} while (0)
#define wlc_eventq_set_all_ind(a, b, c, d) do {} while (0)
#define wlc_eventq_flush(eq) do {} while (0)
#else /* WLNOEIND */
int wlc_eventq_test_ind(wlc_eventq_t *eq, int et);
int wlc_eventq_set_ind(wlc_eventq_t* eq, uint et, bool on);
extern int wlc_eventq_set_all_ind(wlc_info_t *wlc, wlc_eventq_t* eq, uint et, bool on);
void wlc_eventq_flush(wlc_eventq_t *eq);
#endif /* WLNOEIND */

#ifdef BCMPKTPOOL
int wlc_eventq_set_evpool_mask(wlc_eventq_t* eq, uint et, bool enab);
#endif

void wlc_event_process(wlc_eventq_t *eq, wlc_event_t *e);

#if defined(BCMPKTPOOL)
#ifndef EVPOOL_SIZE
#define EVPOOL_SIZE	4
#endif
#ifndef EVPOOL_MAXDATA
#define EVPOOL_MAXDATA	76
#endif
#endif /* BCMPKTPOOL */

#endif  /* _WLC_EVENT_H_ */
