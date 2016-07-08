/*
 * NetBSD Cardbus glue
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
 * $Id: wl_nbsd_cardbus.c 467328 2014-04-03 01:23:40Z $
 */


#include <wlc_cfg.h>
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_llc.h>
#include <net/if_arp.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_regdomain.h>
#include <net80211/ieee80211_radiotap.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <osl.h>
#include <bcmdevs.h>
#include <epivers.h>

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <epivers.h>
#include <bcmendian.h>
#include <bcmdevs.h>
#include <bcmutils.h>

#include "wl_net80211.h"
#include "wl_dbg.h"

#include <pcicfg.h>
#include <dev/cardbus/cardbusvar.h>

struct bwl_cardbus_softc {
	struct device		dev;		/* NB: must be first */
	struct wl_softc		sc;
	pcireg_t		sc_bar_val;
	bus_space_tag_t		sc_iot;
	bus_size_t		sc_mapsize;
	bus_space_handle_t	sc_ioh;
	int			sc_intrline;	/* interrupt line */
	void			*sc_ih;		/* interrupt handler */
	cardbus_devfunc_t	sc_ct;
	cardbustag_t		sc_tag;
};

extern	bool wlc_chipmatch(uint16 vendor, uint16 device);

static int
bwl_cardbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct cardbus_attach_args *ca = aux;

	return wlc_chipmatch(PCI_VENDOR(ca->ca_id), PCI_PRODUCT(ca->ca_id));
}

static void
bwl_cardbus_setup(struct bwl_cardbus_softc *csc)
{
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	pcireg_t reg;

	/* power up the device */
	cardbus_setpowerstate(device_xname(&csc->dev), ct,
	    csc->sc_tag, PCI_PWR_D0);

	/* program the BAR */
	cardbus_conf_write(cc, cf, csc->sc_tag, PCI_CFG_BAR0, csc->sc_bar_val);

	/* enable memory+dma on the bridge */
	ct->ct_cf->cardbus_ctrl(cc, CARDBUS_MEM_ENABLE);
	ct->ct_cf->cardbus_ctrl(cc, CARDBUS_BM_ENABLE);

	reg = cardbus_conf_read(cc, cf, csc->sc_tag,
	    CARDBUS_COMMAND_STATUS_REG);
	reg |= CARDBUS_COMMAND_MASTER_ENABLE | CARDBUS_COMMAND_MEM_ENABLE;
	cardbus_conf_write(cc, cf, csc->sc_tag, CARDBUS_COMMAND_STATUS_REG, reg);

	/*
	 * Make sure the latency timer is set to some reasonable
	 * value.
	 */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag, CARDBUS_BHLC_REG);
	if (CARDBUS_LATTIMER(reg) < 0x20) {
		reg &= ~(CARDBUS_LATTIMER_MASK << CARDBUS_LATTIMER_SHIFT);
		reg |= (0x20 << CARDBUS_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, csc->sc_tag, CARDBUS_BHLC_REG, reg);
	}
}

static int
bwl_cardbus_enable(struct device *self)
{
	struct bwl_cardbus_softc *csc = device_private(self);
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	/*
	 * Map and establish the interrupt.
	 */
	csc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline, IPL_NET,
	    bwl_intr, &csc->sc);
	if (csc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt at %d\n",
		    device_xname(self), csc->sc_intrline);
		Cardbus_function_disable(csc->sc_ct);
		return (1);
	}
	printf("%s: interrupting at %d\n",
	    device_xname(self), csc->sc_intrline);

	return (0);
}

static void
bwl_cardbus_disable(struct device *self)
{
	struct bwl_cardbus_softc *csc = device_private(self);
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	/* Unhook the interrupt handler. */
	cardbus_intr_disestablish(cc, cf, csc->sc_ih);
	csc->sc_ih = NULL;
}

static void
bwl_cardbus_power(struct device *self, int why)
{
	struct bwl_cardbus_softc *csc = device_private(self);

	printf("%s: %s %d\n", device_xname(self), __func__, why);
	if (why == PWR_RESUME) {
		/*
		 * Kick the PCI configuration registers.
		 */
		bwl_cardbus_setup(csc);
	}
}

static void
bwl_cardbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct bwl_cardbus_softc *csc = device_private(self);
	struct wl_softc *sc = &csc->sc;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	bus_addr_t adr;
	osl_t *osh;

	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;

	printf("\n");

	sc->enable = bwl_cardbus_enable;
	sc->disable = bwl_cardbus_disable;
	sc->power = bwl_cardbus_power;

	/*
	 * Map the device.
	 */
	if (Cardbus_mapreg_map(ct, PCI_CFG_BAR0, CARDBUS_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_LINEAR,
	    &csc->sc_iot, &csc->sc_ioh, &adr, &csc->sc_mapsize) == 0) {
#ifndef rbus
		ct->ct_cf->cardbus_mem_open(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_MEM;
	} else {
		printf("%s: unable to map BAR 0\n", device_xname(self));
		return;
	}
	/*
	 * Setup the pci configuration registers.
	 */
	bwl_cardbus_setup(csc);

	/* Remember which interrupt line */
	csc->sc_intrline = ca->ca_intrline;

	sc->dev = self;
	osh = osl_attach(ca, 1, device_xname(self),
	    OSL_CARDBUS_BUS, ca->ca_dmat, csc->sc_iot, csc->sc_ioh);

	/* common load-time initialization */
	sc->wl.bustype = PCI_BUS;
	sc->wl.piomode = FALSE;

	(void) bwl_attach(sc, PCI_VENDOR(ca->ca_id), PCI_PRODUCT(ca->ca_id),
		osh, bus_space_vaddr(csc->sc_iot, csc->sc_ioh));
}


static int
bwl_cardbus_detach(struct device *self, int flags)
{
	struct bwl_cardbus_softc *csc = device_private(self);
	struct wl_softc *sc = &csc->sc;
	struct cardbus_devfunc *ct = csc->sc_ct;

	if (csc->sc_ih != NULL) {
		cardbus_intr_disestablish(ct->ct_cc, ct->ct_cf, csc->sc_ih);
		csc->sc_ih = NULL;
	}
	sc->invalid = 1;		/* device gone */

	bwl_detach(sc);

	/*
	 * Release bus space and close window.
	 */
	Cardbus_mapreg_unmap(ct, PCI_CFG_BAR0,
	    csc->sc_iot, csc->sc_ioh, csc->sc_mapsize);

	return (0);
}

CFATTACH_DECL(bwl_cardbus, sizeof(struct bwl_cardbus_softc),
    bwl_cardbus_match, bwl_cardbus_attach, bwl_cardbus_detach, bwl_activate);
