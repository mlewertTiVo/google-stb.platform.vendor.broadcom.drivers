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
 * $Id: wl_fbsd_pci.c 467328 2014-04-03 01:23:40Z $
 */


#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <machine/bus.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/ethernet.h>
#include <sys/bus.h>
#include <sys/module.h>
#include <sys/rman.h>
#include <machine/resource.h>
#if defined(_KERNEL) && !defined(_LKM)
#include <net/bpf.h>
#endif
#include <net/if_llc.h>
#include <net/if_arp.h>

#include <osl.h>
#include <bcmdevs.h>
#include <epivers.h>


#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <epivers.h>
#include <bcmendian.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <pcicfg.h>
#include <wlioctl.h>
#include <bcmbsd.h>
#include <bcmwpa.h>
#include <proto/eapol.h>
#include <proto/802.1d.h>

typedef const struct si_pub	si_t;

#include <wlc_pub.h>

#include "wl_fbsd.h"
#include "wl_dbg.h"

#include <pcicfg.h>

/* OS PCI bus control information */
struct wl_pci_softc {
	struct wl_softc		sc;
	uint			magic;		/* PCI control info magic cookie */

	/* FreeBSD */
	struct resource		*sc_sr;		/* memory resource */
	struct resource		*sc_irq;	/* irq resource */
	void			*sc_ih;		/* interrupt handler */
};

static int
wl_fbsd_pci_probe(device_t dev)
{
	int ret_code;

	device_printf(dev, "%s: start\n", __FUNCTION__);
	/* Verify vendor */
	if (pci_get_vendor(dev) == VENDOR_BROADCOM) {
		if (pci_get_device(dev) == BCM4318_D11DUAL_ID ||
		    pci_get_device(dev) == BCM4318_D11A_ID ||
		    pci_get_device(dev) == BCM4318_D11G_ID) {
			device_printf(dev, "Broadcom device 0x%x found!\n",
			              pci_get_device(dev));

			device_set_desc(dev, "wl");
			ret_code = BUS_PROBE_DEFAULT;
		} else {
			device_printf(dev, "Unsupported Broadcom device: 0x%x\n",
			              pci_get_device(dev));
			ret_code = ENXIO;
		}
	} else {
		device_printf(dev, "Non Broadcom device: 0x%x\n",
		              pci_get_device(dev));
		ret_code = ENXIO;
	}

	device_printf(dev, "%s: end - %d\n", __FUNCTION__, ret_code);
	return ret_code;
}

static int
wl_fbsd_pci_attach(device_t dev)
{
	struct wl_pci_softc *pci_sc = device_get_softc(dev);
	struct wl_softc *sc = &pci_sc->sc;
	int rid;
	u_int32_t cmd;
	int ret_code = ENXIO;

	device_printf(dev, "%s: start\n", __FUNCTION__);
	sc->dev = dev;

	/* Map the interrupt, BAR and registers of the PCI device */
	/* Enable bus mastering. */
	if (pci_enable_busmaster(dev))
		device_printf(dev, "pci_enable_busmaster failed\n");

	cmd = pci_read_config(dev, PCIR_COMMAND, 4);
	cmd |= PCIM_CMD_MEMEN | PCIM_CMD_BUSMASTEREN;
	pci_write_config(dev, PCIR_COMMAND, cmd, 4);

	if ((cmd & PCIM_CMD_MEMEN) == 0) {
		device_printf(dev, "failed to enable memory mapping\n");
		goto bad;
	}
	cmd = pci_read_config(dev, PCIR_COMMAND, 4);
	if ((cmd & PCIM_CMD_BUSMASTEREN) == 0) {
		device_printf(dev, "failed to enable bus mastering\n");
		goto bad;
	}

	/* Setup memory-mapping of PCI registers */
	rid = PCI_CFG_BAR0;
	pci_sc->sc_sr = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &rid,
	                                       RF_ACTIVE);
	if (pci_sc->sc_sr == NULL) {
		device_printf(dev, "cannot map register space\n");
		goto bad1;
	}

	sc->va = rman_get_virtual(pci_sc->sc_sr);
	sc->st = rman_get_bustag(pci_sc->sc_sr);
	sc->sh = rman_get_bushandle(pci_sc->sc_sr);

	/* Arrange interrupt line. */
	rid = 0;
	pci_sc->sc_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
	                                        RF_SHAREABLE|RF_ACTIVE);
	if (pci_sc->sc_irq == NULL) {
		device_printf(dev, "could not map interrupt\n");
		goto bad2;
	}
	if (bus_setup_intr(dev, pci_sc->sc_irq,
	                   INTR_TYPE_NET | INTR_MPSAFE,
	                   NULL, wl_isr, sc, &pci_sc->sc_ih)) {
		device_printf(dev, "could not establish interrupt\n");
		goto bad3;
	}

	pci_sc->magic = PCI_MAGIC;


	/* should we lock here... ??? */

	/* Common wl_attach */
	if (!wl_attach(sc))
		ret_code = 0;

bad3:
	bus_teardown_intr(dev, pci_sc->sc_irq, pci_sc->sc_ih);
bad2:
	bus_release_resource(dev, SYS_RES_IRQ, 0, pci_sc->sc_irq);
bad1:
	bus_release_resource(dev, SYS_RES_MEMORY, PCI_CFG_BAR0, pci_sc->sc_sr);
bad:

	device_printf(dev, "%s: sc_irq 0x%x, sc_ih 0x%x, sc_sr 0x%x\n", __FUNCTION__,
	              (unsigned int)pci_sc->sc_irq, (unsigned int)pci_sc->sc_ih,
	              (unsigned int)pci_sc->sc_sr);

	device_printf(dev, "%s: end\n", __FUNCTION__);
	return ret_code;
}


static int
wl_fbsd_pci_detach(device_t dev)
{
	struct wl_pci_softc *pci_sc = device_get_softc(dev);
	struct wl_softc *sc = &pci_sc->sc;

	device_printf(dev, "%s: start\n", __FUNCTION__);

	/* This prevents the device from being detached more than once */
	if (pci_sc->magic != PCI_MAGIC) {
		device_printf(dev, "%s: wrong pci magic of %d when %d was expected\n",
		              __FUNCTION__, pci_sc->magic, PCI_MAGIC);
		return 0;
	}

	wl_detach(sc);

	device_printf(dev, "%s: sc_irq 0x%x, sc_ih 0x%x, sc_sr 0x%x\n", __FUNCTION__,
	              (unsigned int)pci_sc->sc_irq, (unsigned int)pci_sc->sc_ih,
	              (unsigned int)pci_sc->sc_sr);

	bus_generic_detach(dev);

	device_printf(dev, "%s: end\n", __FUNCTION__);
	return (0);
}

static int
wl_fbsd_pci_shutdown(device_t dev)
{
	return (0);
}

static int
wl_fbsd_pci_suspend(device_t dev)
{
	return (0);
}

static int
wl_fbsd_pci_resume(device_t dev)
{
	return (0);
}

static device_method_t wl_fbsd_pci_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		wl_fbsd_pci_probe),
	DEVMETHOD(device_attach,	wl_fbsd_pci_attach),
	DEVMETHOD(device_detach,	wl_fbsd_pci_detach),
	DEVMETHOD(device_shutdown,	wl_fbsd_pci_shutdown),
	DEVMETHOD(device_suspend,	wl_fbsd_pci_suspend),
	DEVMETHOD(device_resume,	wl_fbsd_pci_resume),
	{0, 0}
};
static driver_t wl_fbsd_pci_driver = {
	"wl",
	wl_fbsd_pci_methods,
	sizeof(struct wl_pci_softc)
};
static devclass_t wl_fbsd_devclass;

DRIVER_MODULE(wl, pci, wl_fbsd_pci_driver, wl_fbsd_devclass, 0, 0);

MODULE_VERSION(wl, 1);
