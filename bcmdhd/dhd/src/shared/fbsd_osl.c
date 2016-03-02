/*
 * FreeBSD OS Independent Layer
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: fbsd_osl.c 557776 2015-05-20 01:18:34Z $
 */

#include <typedefs.h>
#include <pcicfg.h>

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
#include <sys/bus_dma.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/ethernet.h>


/* FBSD */
#include <sys/bus.h>
#if defined(BCMPCIE)
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <machine/pci_cfgreg.h>
#endif /* BCMPCIE */

/* FBSD */

#include <osl.h>

#include <bcmutils.h>

#define BCM_MEM_FILENAME_LEN    24  /* BCM_MEM_FILENAME_LEN */

static int16 freebsdbcmerrormap[] =
{	0,          /* 0 */
	-EINVAL,     /* BCME_ERROR */
	-EINVAL,     /* BCME_BADARG */
	-EINVAL,     /* BCME_BADOPTION */
	-EINVAL,     /* BCME_NOTUP */
	-EINVAL,     /* BCME_NOTDOWN */
	-EINVAL,     /* BCME_NOTAP */
	-EINVAL,     /* BCME_NOTSTA */
	-EINVAL,     /* BCME_BADKEYIDX */
	-EINVAL,     /* BCME_RADIOOFF */
	-EINVAL,     /* BCME_NOTBANDLOCKED */
	-EINVAL,     /* BCME_NOCLK */
	-EINVAL,     /* BCME_BADRATESET */
	-EINVAL,     /* BCME_BADBAND */
	-E2BIG,      /* BCME_BUFTOOSHORT */
	-E2BIG,      /* BCME_BUFTOOLONG */
	-EBUSY,      /* BCME_BUSY */
	-EINVAL,     /* BCME_NOTASSOCIATED */
	-EINVAL,     /* BCME_BADSSIDLEN */
	-EINVAL,     /* BCME_OUTOFRANGECHAN */
	-EINVAL,     /* BCME_BADCHAN */
	-EFAULT,     /* BCME_BADADDR */
	-ENOMEM,     /* BCME_NORESOURCE */
	-EOPNOTSUPP, /* BCME_UNSUPPORTED */
	-EMSGSIZE,   /* BCME_BADLENGTH */
	-EINVAL,     /* BCME_NOTREADY */
	-EPERM,      /* BCME_NOTPERMITTED */
	-ENOMEM,     /* BCME_NOMEM */
	-EINVAL,     /* BCME_ASSOCIATED */
	-ERANGE,     /* BCME_RANGE */
	-EINVAL,     /* BCME_NOTFOUND */
	-EINVAL,     /* BCME_WME_NOT_ENABLED */
	-EINVAL,     /* BCME_TSPEC_NOTFOUND */
	-EINVAL,     /* BCME_ACM_NOTSUPPORTED */
	-EINVAL,     /* BCME_NOT_WME_ASSOCIATION */
	-EIO,        /* BCME_SDIO_ERROR */
	-ENODEV,     /* BCME_DONGLE_DOWN */
	-EINVAL,     /* BCME_VERSION */
	-EIO,        /* BCME_TXFAIL */
	-EIO,        /* BCME_RXFAIL */
	-EINVAL,     /* BCME_NODEVICE */
	-EINVAL,     /* BCME_NMODE_DISABLED */
	-EINVAL,     /* BCME_NONRESIDENT */
	-EINVAL,	    /* BCME_SCANREJECT */
	-EINVAL,     /* BCME_USAGE_ERROR */
	-EIO,	    /* BCME_IOCTL_ERROR */
	-EIO,        /* BCME_SERIAL_PORT_ERR */

	/* When an new error code is added to bcmutils.h, add os
	 * specific error translation here as well
	 */
};

/* check if BCME_LAST changed since the last time this function was updated */
#if BCME_LAST != -46
#error "You need to add a OS error translation in the linuxbcmerrormap \
	for new error code defined in bcmutils.h"
#endif
#if defined(BCMPCIE)
/* DMA Map structure */
struct osl_dmainfo {
	volatile struct 	osl_dmainfo *dma_next;	/* pointer to next dma map node */
	bus_dmamap_t		dma_map;		/* dma handle describing the region */
	uint 			dma_magic;		/* dma magic number */
};

/*
 * DMA tx/rx descriptor ring info.
 */
typedef struct osl_descdma {
	bus_dma_tag_t		dmat;	/* bus DMA tag */
	bus_dmamap_t		dmamap;	/* DMA map for descriptors */
} osl_descdma_t;
#endif /* BCMPCIE */



struct osl_info {
	struct osl_pubinfo pub;			/* Bus mapping information */
	uint 			os_magic;	/* OS MAGIC number */
	device_t		dev;
	const char		*os_xname;	/* Net device name */

	/* DMA mapping data */
	volatile int 		os_num_dmaps;	/* Total number of DMA maps allocated */
	volatile osldma_t 	*os_free_dmaps;	/* List head of free DMA maps */

	bus_dma_tag_t		dmat;	/* DMA tag */

	uint 			os_malloced;	/* Debug malloc count */
	uint 			os_failed;	/* Debug malloc failed count */


};

/* Global ASSERT type */
uint32 g_assert_type = 0;

/* Globals */
uint lmtest = FALSE;

/* Local prototypes */

#if defined(BCMPCIE)
static void osl_free_dmamap_chain(osl_t *osh);
static osldma_t *osl_dmamap_get(osl_t *osh);
static void osl_dmamap_put(osl_t *osh, osldma_t *map);
#endif /* BCMPCIE */

/* translate bcmerrors into freebsd errors */
int
osl_error(int bcmerror)
{
	if (bcmerror > 0)
		bcmerror = 0;
	else if (bcmerror < BCME_LAST)
		bcmerror = BCME_ERROR;

	/* Array bounds covered by ASSERT in osl_attach */
	return freebsdbcmerrormap[-bcmerror];
}

void
osl_delay(uint usec)
{
	uint d;

	while (usec > 0) {
		d = MIN(usec, 1000);
		DELAY(d);
		usec -= d;
	}
}

osl_t *
osl_attach(void *pdev, const char *dev_name, bus_space_tag_t space,
           bus_space_handle_t handle, uint8 *vaddr)
{
	struct osl_info *osh;

	/* Structure is zeroed on malloc by specifying the M_ZERO flag */
	osh = (osl_t *)malloc(sizeof(osl_t), M_DEVBUF, M_NOWAIT|M_ZERO);

	if (osh) {
		osh->os_magic = OS_HANDLE_MAGIC;
		osh->os_xname = dev_name;

#if defined(__FreeBSD__)
		osh->dev = (device_t)pdev;
#endif /* defined (OS) */

#if defined(BCMPCIE)
		if (osh->dev) {
			/* Setup DMA descriptor area. */
			if (bus_dma_tag_create(bus_get_dma_tag(osh->dev),	/* parent */
			                       1, 0,			/* alignment, bounds */
			                       BUS_SPACE_MAXADDR_32BIT,	/* lowaddr */
			                       BUS_SPACE_MAXADDR,	/* highaddr */
			                       NULL, NULL,		/* filter, filterarg */
			                       0x3ffff,
			                       10,			/* nsegments */
			                       0x3ffff,
			                       BUS_DMA_ALLOCNOW,	/* flags */
			                       NULL,			/* lockfunc */
			                       NULL,			/* lockarg */
			                       &osh->dmat)) {
				OSL_ERR(("cannot allocate DMA tag\n"));
				goto bad;
			}
		}

#endif /* BCMPCIE */

		osh->pub.space = space;
		osh->pub.handle = handle;
		osh->pub.vaddr = vaddr;
	}

	bcm_object_trace_init();

	return osh;

#if defined(BCMPCIE)
bad:
	free(osh, M_DEVBUF);
	return NULL;
#endif /* BCMPCIE */
}

void
osl_detach(osl_t *osh)
{
	if (osh == NULL)
		return;

	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);

#if defined(BCMPCIE)
	/* Deallocate DMA map chains */
	osl_free_dmamap_chain(osh);
#endif /* BCMPCIE */

	bcm_object_trace_deinit();

	free(osh, M_DEVBUF);
}

#if defined(BCMPCIE)
/* DMA Magic Number */
#define DMA_HANDLE_MAGIC	OS_HANDLE_MAGIC + 10

static osldma_t *
osl_dmamap(osl_t *osh)
{
	osldma_t 	*cur;

	if ((cur = malloc(sizeof(osldma_t), M_DEVBUF, M_NOWAIT|M_ZERO)) == NULL)
		return NULL;

	/* Create the map */
	if (bus_dmamap_create(osh->dmat, BUS_DMA_NOWAIT, &cur->dma_map) != 0) {
			OSL_ERR(("%s: unable to create streaming DMA map\n",
				osh->os_xname));
		free(cur, M_DEVBUF);
		return NULL;
	}

	cur->dma_magic = DMA_HANDLE_MAGIC;

	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);
	osh->os_num_dmaps++;

	return cur;
}

static void
osl_dmamap_put(osl_t *osh, osldma_t  *map)
{

	ASSERT(map->dma_magic == DMA_HANDLE_MAGIC);

	map->dma_next = osh->os_free_dmaps;
	osh->os_free_dmaps = map;
}

static osldma_t *
osl_dmamap_get(osl_t *osh)
{
	osldma_t *map = NULL;

	map = (osldma_t *) (uintptr)osh->os_free_dmaps;

	if (map) {
		osh->os_free_dmaps = osh->os_free_dmaps->dma_next;
		map->dma_next = NULL;
	} else
		/* Maps are all used. Make one. */
		map = osl_dmamap(osh);

	ASSERT(map->dma_magic == DMA_HANDLE_MAGIC);
	return map;
}


static void
osl_free_dmamap_chain(osl_t *osh)
{
	osldma_t 	*cur, *tmp;
	int		ipl;

	cur = (osldma_t *)(uintptr)osh->os_free_dmaps;

	ipl = splnet();
	while (cur) {
		ASSERT(cur->dma_magic == DMA_HANDLE_MAGIC);
		bus_dmamap_destroy(osh->dmat, cur->dma_map);
		tmp = cur;
		cur = (osldma_t *)(uintptr)cur->dma_next;
		free(tmp, M_DEVBUF);
		osh->os_num_dmaps--;
	}
	splx(ipl);
	ASSERT(osh->os_num_dmaps == 0);
}


#if defined(__FreeBSD__)
static void
osl_load_cb(void *arg, bus_dma_segment_t *segs, int nsegs, int error)
{
	bus_addr_t *paddr = (bus_addr_t*) arg;
	KASSERT(error == 0, ("error %u on bus_dma callback", error));
	*paddr = segs->ds_addr;
}
#endif /* defined (__FreeBSD__) */


void *
osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align_bits, uint *alloced,
	ulong *pap, osldma_t **dmah)
{
	int 			error = -1;
	void			*va = NULL;
	osl_descdma_t	*desc;
	int boundary = PAGE_SIZE;
	uint16 align = (1 << align_bits);

	OSL_TRACE(("%s: start\n", __FUNCTION__));

	ASSERT(osh);
	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);
	ASSERT(pap != NULL);
	ASSERT(dmah != NULL);

	/* fix up the alignment requirements first */
	if (!ISALIGNED(DMA_CONSISTENT_ALIGN, align))
		size += align;
	*alloced = size;

	/* Fix the boundary to be power of 2 but greater than size
	 * This tells OS for continguous buffer that is maintained across this boundary
	 */
	while (boundary < size)
		boundary <<= 1;

	/* setup DMA descriptor area */
	desc = (osl_descdma_t*)osl_malloc(osh, sizeof(osl_descdma_t));

	if ((error = bus_dma_tag_create(bus_get_dma_tag(osh->dev),	/* parent */
	                                PAGE_SIZE, 0,			/* alignment, bounds */
	                                BUS_SPACE_MAXADDR_32BIT,	/* lowaddr */
	                                BUS_SPACE_MAXADDR,		/* highaddr */
	                                NULL, NULL,			/* filter, filterarg */
	                                size,		/* maxsize */
	                                1,				/* nsegments */
	                                size,		/* maxsegsize */
	                                BUS_DMA_ALLOCNOW,		/* flags */
	                                NULL,				/* lockfunc */
	                                NULL,				/* lockarg */
	                                &desc->dmat)) != 0) {
		if (error != 0) {
			OSL_ERR(("%s: cannot allocate DMA tag, "
			       "error = %d\n", osh->os_xname, error));
			goto fail0;
		}
	}

	/* this probably isn't needed... */
	if ((error = bus_dmamap_create(desc->dmat, BUS_DMA_NOWAIT, &desc->dmamap)) != 0) {
		OSL_ERR(("%s: unable to create consistent DMA map, "
		    "error = %d\n", osh->os_xname, error));
		goto fail0;
	}

	if ((error = bus_dmamem_alloc(desc->dmat, &va, BUS_DMA_NOWAIT | BUS_DMA_COHERENT,
	                              &desc->dmamap)) != 0) {
		OSL_ERR(("%s: unable to allocate consistent dma memory, error = %d\n",
		    osh->os_xname, error));
		goto fail1;
	}

	if ((error = bus_dmamap_load(desc->dmat, desc->dmamap, va,
	                             size, osl_load_cb, pap, BUS_DMA_NOWAIT)) != 0) {
		OSL_ERR(("%s: unable to load consistent DMA map, error = %d\n",
		    osh->os_xname, error));
		goto fail2;
	}

	*dmah = (osldma_t *)desc;

	OSL_TRACE(("%s: end\n", __FUNCTION__));

	/* Return the virtual address */
	return va;

fail2:
	bus_dmamem_free(desc->dmat, va, desc->dmamap);
fail1:
	bus_dmamap_destroy(desc->dmat, desc->dmamap);
fail0:
	bus_dma_tag_destroy(desc->dmat);
	osl_mfree(osh, desc, sizeof(osl_descdma_t));
	return NULL;
}

void
osl_dma_free_consistent(osl_t *osh, void *va, uint size, ulong pa, osldma_t *dmah)
{
	osl_descdma_t *desc = (osl_descdma_t *)dmah;

	OSL_TRACE(("%s: start\n", __FUNCTION__));

	OSL_TRACE(("%s: desc->dmamap 0x%x\n", __FUNCTION__, (unsigned int)desc->dmamap));
	bus_dmamem_free(desc->dmat, va, desc->dmamap);
	bus_dmamap_destroy(desc->dmat, desc->dmamap);
	bus_dma_tag_destroy(desc->dmat);
	osl_mfree(osh, desc, sizeof(osl_descdma_t));
	OSL_TRACE(("%s: end\n", __FUNCTION__));
	return;
}

uintptr
osl_dma_map(osl_t *osh, void *va, uint size, int direction, osldma_t **dmah)
{
	int		error = -1;
	osldma_t	*map = NULL;
	uint		*phys_addr = NULL;

	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);
	ASSERT(va != NULL);
	ASSERT(dmah != NULL);


	if ((map = osl_dmamap_get(osh)) == NULL) {
			OSL_ERR(("%s: unable to create %s streaming DMA map, error = %d\n",
				osh->os_xname, (direction == DMA_TX) ? "TX" : "RX", error));
			goto fail1;
	}

	ASSERT(map->dma_magic == DMA_HANDLE_MAGIC);

	if ((error = bus_dmamap_load(osh->dmat, map->dma_map, va,
	                             size, osl_load_cb, &phys_addr, BUS_DMA_NOWAIT)) != 0) {
		OSL_ERR(("%s: unable to load %s streaming DMA map, error = %d\n",
		osh->os_xname, (direction == DMA_TX) ? "TX" : "RX", error));
		goto fail2;
	}

	/* Flush the cache */
	bus_dmamap_sync(osh->dmat, map->dma_map,
	        (direction == DMA_TX)? BUS_DMASYNC_PREWRITE: BUS_DMASYNC_PREREAD);


	ASSERT(phys_addr != NULL);

	/* Return the physical address */
	*dmah = map;
	OSL_TRACE(("%s: end\n", __FUNCTION__));
	return (uintptr)phys_addr;

fail2:
	osl_dmamap_put(osh, map);
fail1:
	return (uintptr)NULL;
}

void
osl_dma_unmap(osl_t *osh, uint pa, uint size, int direction, osldma_t **dmah)
{
	OSL_TRACE(("%s: not implemented\n", __FUNCTION__));

}

#endif /* BCMPCIE */


/* Allocate packet header, and optionally attach a cluster */
void *
osl_pktget(osl_t *osh,
#ifdef BCM_OBJECT_TRACE
	int line, const char *caller,
#endif /* BCM_OBJECT_TRACE */
	uint len, bool send)
{
	struct mbuf	*m;
	struct m_tag	*mtag;

#ifndef __NO_STRICT_ALIGNMENT
	if (send == FALSE)
	  len += ETHER_ALIGN;
#endif
	if (len <= MHLEN) {
		m = m_gethdr(M_DONTWAIT, MT_DATA);
	}
	else if (len <= MCLBYTES) {
		m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
	}
	else if (len <= MJUMPAGESIZE) {
		m = m_getjcl(M_DONTWAIT, MT_DATA, M_PKTHDR, MJUMPAGESIZE);
	}
	else if (len <= MJUM9BYTES) {
		m = m_getjcl(M_DONTWAIT, MT_DATA, M_PKTHDR, MJUM9BYTES);
	}
	else if (len <= MJUM16BYTES) {
		m = m_getjcl(M_DONTWAIT, MT_DATA, M_PKTHDR, MJUM16BYTES);
	}
	else {
		panic("bwm:osl_pktget()");
	}

	if (!m) {
		return (NULL);
	}

	m->m_pkthdr.len = m->m_len = len;
#ifndef __NO_STRICT_ALIGNMENT
	if (send == FALSE)
	  m_adj(m, ETHER_ALIGN);
#endif
	m_tag_init(m);
	mtag = m_tag_get(FREEBSD_PKTTAG, OSL_PKTTAG_SZ, M_DONTWAIT);
	if (!mtag) {
		m_freem(m);
		return (NULL);
	}
	bzero((void *)(mtag + 1), OSL_PKTTAG_SZ);
	m_tag_prepend(m, mtag);

	mtag = m_tag_get(FREEBSD_PKTPRIO, OSL_PKTPRIO_SZ, M_DONTWAIT);
	if (!mtag) {
		m_freem(m); /* include m_tag_delete_chain() */
		return (NULL);
	}
	bzero((void *)(mtag + 1), OSL_PKTPRIO_SZ);
	m_tag_prepend(m, mtag);


#ifdef BCM_OBJECT_TRACE
	bcm_object_trace_opr(m, BCM_OBJDBG_ADD_PKT, caller, line);
#endif /* BCM_OBJECT_TRACE */

	return ((void*) m);
}

void
osl_pktfree(osl_t *osh,
#ifdef BCM_OBJECT_TRACE
	int line, const char *caller,
#endif /* BCM_OBJECT_TRACE */
	void *mbuf, bool send)
{
	struct mbuf	*m = (struct mbuf *)mbuf;


	if (send && osh->pub.tx_fn)
		osh->pub.tx_fn(osh->pub.tx_ctx, m, 0);

#ifdef BCM_OBJECT_TRACE
	bcm_object_trace_opr(m, BCM_OBJDBG_REMOVE, caller, line);
#endif /* BCM_OBJECT_TRACE */

	m_tag_delete_chain(m, (struct m_tag *)NULL);
	m_freem(m);
}

void *
osl_pktdup(osl_t *osh,
#ifdef BCM_OBJECT_TRACE
	int line, const char *caller,
#endif /* BCM_OBJECT_TRACE */
	void *p)
{
	struct mbuf	*n, *m = (struct mbuf *)p;

	n = m_dup(m, M_DONTWAIT);

#ifdef BCM_OBJECT_TRACE
	bcm_object_trace_opr(n, BCM_OBJDBG_ADD_PKT, caller, line);
#endif /* BCM_OBJECT_TRACE */

	return ((struct mbuf *)n);
}

void
osl_pktsetlen(osl_t *osh, void *p, uint len)
{
	struct mbuf	*m = (struct mbuf *)p;

	/*
	 * single mbuf then updates go to m_len
	 * and m_pkthdr.len. If not update only
	 * m_pkthdr.len
	 */
	if (m->m_flags & M_PKTHDR) {
		if (m->m_next == NULL)
			m->m_len = len;

		m->m_pkthdr.len = len;
	}
}
void *
osl_pktpush(osl_t *osh, void **pp, int len)
{
	struct mbuf **_mmp = (struct mbuf **)pp;
	struct mbuf *_mm = *_mmp;
	int _mplen = len;

	if (M_LEADINGSPACE(_mm) >= _mplen) {
		_mm->m_data -= _mplen;
		_mm->m_len += _mplen;
	} else {
		_mm = m_prepend(_mm, _mplen, M_DONTWAIT);
		bcm_object_trace_upd(*_mmp, _mm);
	}

	if (_mm == NULL) {
		printf("%s failed to add %d to mbuf %p\n",
			__FUNCTION__, len, *pp);
		return 	NULL;
	}

	if (_mm != NULL && _mm->m_flags & M_PKTHDR)
		 _mm->m_pkthdr.len += _mplen;

	*_mmp = _mm;

	return (void *)mtod(_mm, uint8_t *);
}

void *
osl_pktpull(osl_t *osh, void *p, int req_len)
{
	struct mbuf	*mp = (struct mbuf *)p;
	struct mbuf	*m;
	int len;

	len = req_len;
	if ((m = mp) == NULL)
		return (NULL);

	while (m != NULL && len > 0) {
		if (m->m_len <= len) {
			len -= m->m_len;
			m->m_data += m->m_len;
			m->m_len = 0;
			m = m->m_next;
		} else {
			m->m_len -= len;
			m->m_data += len;
			len = 0;
		}
	}
	m = mp;
	if (mp->m_flags & M_PKTHDR)
		m->m_pkthdr.len -= (req_len - len);

	return (void *)mtod(m, uint8_t *);
}

void
osl_pktsetnext(osl_t *osh, void *m, void *x)
{
	(((struct mbuf *)(m))->m_next) = ((struct mbuf *)(x));
	return;
}

void
osl_assert(const char *exp, const char *file, int line)
{
	panic("assertion \"%s\" failed: file \"%s\", line %d\n", exp, file, line);
}


void *
osl_malloc(osl_t *osh, uint size)
{
	osl_t *h = osh;
	void *addr;

	ASSERT(h && (h->os_magic == OS_HANDLE_MAGIC));
	{
		addr = malloc(size, M_DEVBUF, M_NOWAIT);
	}
	if (!addr)
		h->os_failed++;
	else
		h->os_malloced += size;
	return (addr);
}

void *
osl_mallocz(osl_t *osh, uint size)
{
	void *ptr;

	ptr = osl_malloc(osh, size);

	if (ptr != NULL) {
		memset(ptr, 0, size);
	}

	return ptr;
}

void
osl_mfree(osl_t *osh, void *addr, uint size)
{
	osl_t *h = osh;

	ASSERT(h && (h->os_magic == OS_HANDLE_MAGIC));
	h->os_malloced -= size;
	{
		free(addr, M_DEVBUF);
	}
}

uint
osl_malloced(osl_t *osh)
{
	osl_t *h = osh;

	ASSERT(h && (h->os_magic == OS_HANDLE_MAGIC));
	return (h->os_malloced);
}

uint osl_malloc_failed(osl_t *osh)
{
	osl_t *h = osh;

	ASSERT(h && (h->os_magic == OS_HANDLE_MAGIC));
	return (h->os_failed);
}

#if defined(BCMPCIE)
uint32
osl_pci_read_config(osl_t *osh, uint offset, uint size)
{
	ASSERT(osh);
	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);

	/* only 4byte access supported */
	ASSERT(size == 4);

#if defined(__FreeBSD__)
	return pci_read_config(osh->dev, offset, size);
#endif /* defined (OS) */
}

void
osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val)
{
	ASSERT(osh);
	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);

	/* only 4byte access supported */
	ASSERT(size == 4);

#if defined(__FreeBSD__)
	pci_write_config(osh->dev, offset, val, size);
#endif /* defined (OS) */
}
#endif /* BCMPCIE */


#if defined(BCMPCIE)

uint
osl_pci_bus(osl_t *osh)
{
	uint bus;

	ASSERT(osh);
	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);

#if defined(__FreeBSD__)
	bus = pci_get_bus(osh->dev);
#endif /* defined (OS) */
	return bus;
}

uint
osl_pci_slot(osl_t *osh)
{
	uint slot;

	ASSERT(osh);
	ASSERT(osh->os_magic == OS_HANDLE_MAGIC);

#if defined(__FreeBSD__)
	slot = pci_get_slot(osh->dev);
#endif /* defined (OS) */

	return slot;
}
#endif /* BCMPCIE */

uint32
osl_sysuptime(void)
{
	struct timeval tv;
	uint32 ms;

	getmicrouptime(&tv);

	ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	return ms;
}
