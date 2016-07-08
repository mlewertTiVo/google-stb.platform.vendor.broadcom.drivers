/*
 * FreeBSD-specific portion of
 * Broadcom 802.11abgn Networking Device Driver
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_fbsd.h 467328 2014-04-03 01:23:40Z $
 */


#ifndef	_WL_BSD_H
#define	_WL_BSD_H


/* Magic cookies for the various structure */
#define SC_MAGIC 	OS_HANDLE_MAGIC + 1
#define WLINFO_MAGIC	SC_MAGIC + 1
#define WLIF_MAGIC	SC_MAGIC + 2
#define WLTIMER_MAGIC	SC_MAGIC + 3
#define PCI_MAGIC	SC_MAGIC + 4
#define WLTASK_MAGIC	SC_MAGIC + 5


/* Device control information */
typedef struct wl_info {
	struct wl_softc		*sc;		/* Back pointer to softc */
	uint			bustype;	/* bus type */
	bool			piomode;	/* set from insmod argument */
	uint 			magic;		/* Magic cookie */
	osl_t			*osh;		/* pointer to os handler */
	wlc_pub_t		*pub;		/* pointer to public wlc state */

	struct wl_timer 	*timers;		/* timer cleanup queue */

	volatile uint 		callbacks;	/* # outstanding callback functions */

	/* OS interfaces */
	struct wl_if 		*iflist;	/* list of all interfaces */
	struct wl_if 		*base_if;	/* For convenience, points to Base i/f */
	volatile int		num_if;		/* Number of interfaces */


	/* Locking stuff */
#if defined(__FreeBSD__)
	struct mtx	soft_mtx;		/* Top/soft lock */
	struct mtx	hard_mtx;		/* Hard/bottom lock */
#endif /* defined(OS) */
} wl_info_t;


struct wl_softc {
	struct device		*dev;		/* Device control info - ?must be first entry? */
	struct ifnet		*ifp;		/* interface common */
	struct taskqueue	*tq;		/* private task queue */

	wl_info_t		*wl;		/* The wl structure */

	uint			magic;		/* Magic cookie */

	/* hmmm.... FreeBSD specific!!! */
	bus_space_handle_t	sh;		/* BAR 0 */
	bus_space_tag_t		st;
	void			*va;		/* virtual address */
};


int wl_attach(struct wl_softc *sc);
int wl_detach(struct wl_softc *sc);
void wl_isr(void *arg);

#endif /* _WL_BSD_H */
