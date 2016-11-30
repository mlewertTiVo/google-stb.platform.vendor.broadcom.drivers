/*
 * Linux OS Independent Layer
 *
 * Copyright (C) 2016, Broadcom. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: linux_osl.h 648809 2016-07-13 19:50:41Z $
 */

#ifndef _linux_osl_h_
#define _linux_osl_h_

#include <typedefs.h>
#define DECLSPEC_ALIGN(x)	__attribute__ ((aligned(x)))

/* Linux Kernel: File Operations: start */
extern void * osl_os_open_image(char * filename);
extern int osl_os_get_image_block(char * buf, int len, void * image);
extern void osl_os_close_image(void * image);
extern int osl_os_image_size(void *image);
/* Linux Kernel: File Operations: end */

#ifdef WLCXO_IPC
#ifdef WLCXO_SIM
extern volatile void *g_c2d_ipc_ret_hdl;
extern volatile void *g_d2c_ipc_hdl, *g_d2c_ipc_ret_hdl, *g_d2c_cxo_ctrl_wq;
#endif /* WLCXO_SIM */
#ifdef WLCXO_CTRL
#define WLCXO_CTRL_TASK_CTX()	1
#else /* WLCXO_CTRL */
#define WLCXO_CTRL_TASK_CTX()	0
#endif /* WLCXO_CTRL */
#ifdef WLCXO_DATA
#define WLCXO_DATA_TASK_CTX()	1
#else /* WLCXO_DATA */
#define WLCXO_DATA_TASK_CTX()	0
#endif /* WLCXO_DATA */
#endif /* WLCXO_IPC */

#ifdef WLCXO_SIM
extern volatile void *g_c2d_ipc_hdl;
extern struct sk_buff *g_txhpktq_head, *g_txhpktq_tail;
extern uint32 g_txhpktq_cnt;
extern void *g_txhpktq_lock;
#endif /* WLCXO_SIM */

#ifdef BCMDRIVER

/* OSL initialization */
#ifdef SHARED_OSL_CMN
extern osl_t *osl_attach(void *pdev, uint bustype, bool pkttag, void **osh_cmn);
#else
extern osl_t *osl_attach(void *pdev, uint bustype, bool pkttag);
#endif /* SHARED_OSL_CMN */

extern void osl_detach(osl_t *osh);
extern int osl_static_mem_init(osl_t *osh, void *adapter);
extern int osl_static_mem_deinit(osl_t *osh, void *adapter);
extern void osl_set_bus_handle(osl_t *osh, void *bus_handle);
extern void* osl_get_bus_handle(osl_t *osh);

/* Global ASSERT type */
extern uint32 g_assert_type;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
#define PRI_FMT_x       "llx"
#define PRI_FMT_X       "llX"
#define PRI_FMT_o       "llo"
#define PRI_FMT_d       "lld"
#else
#define PRI_FMT_x       "x"
#define PRI_FMT_X       "X"
#define PRI_FMT_o       "o"
#define PRI_FMT_d       "d"
#endif /* CONFIG_PHYS_ADDR_T_64BIT */
/* ASSERT */
	#ifdef __GNUC__
		#define GCC_VERSION \
			(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
		#if GCC_VERSION > 30100
			#define ASSERT(exp)	do {} while (0)
		#else
			/* ASSERT could cause segmentation fault on GCC3.1, use empty instead */
			#define ASSERT(exp)
		#endif /* GCC_VERSION > 30100 */
	#endif /* __GNUC__ */

/* bcm_prefetch_32B */
static inline void bcm_prefetch_32B(const uint8 *addr, const int cachelines_32B)
{
#if defined(BCM7271)
#else /* !BCM7271 */
#if (defined(BCM47XX_CA9) || (defined(STB) && defined(__arm__))) && (__LINUX_ARM_ARCH__ \
	>= 5)
	switch (cachelines_32B) {
		case 4: __asm__ __volatile__("pld\t%a0" :: "p"(addr + 96) : "cc");
		case 3: __asm__ __volatile__("pld\t%a0" :: "p"(addr + 64) : "cc");
		case 2: __asm__ __volatile__("pld\t%a0" :: "p"(addr + 32) : "cc");
		case 1: __asm__ __volatile__("pld\t%a0" :: "p"(addr +  0) : "cc");
	}
#elif defined(__mips__)
	/* Hint Pref_Load = 0 */
	switch (cachelines_32B) {
		case 4: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr + 96));
		case 3: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr + 64));
		case 2: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr + 32));
		case 1: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr +  0));
	}
#endif /* BCM47XX_CA9, __mips__ */
#endif /* BCM7271 */

}

/* microsecond delay */
#define	OSL_DELAY(usec)		osl_delay(usec)
extern void osl_delay(uint usec);

#define OSL_SLEEP(ms)			osl_sleep(ms)
extern void osl_sleep(uint ms);

#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) \
	osl_pcmcia_read_attr((osh), (offset), (buf), (size))
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) \
	osl_pcmcia_write_attr((osh), (offset), (buf), (size))
extern void osl_pcmcia_read_attr(osl_t *osh, uint offset, void *buf, int size);
extern void osl_pcmcia_write_attr(osl_t *osh, uint offset, void *buf, int size);

/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(osh, offset, size) \
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val) \
	osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint offset, uint size);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)	osl_pci_bus(osh)
#define OSL_PCI_SLOT(osh)	osl_pci_slot(osh)
#define OSL_PCIE_DOMAIN(osh)	osl_pcie_domain(osh)
#define OSL_PCIE_BUS(osh)	osl_pcie_bus(osh)
extern uint osl_pci_bus(osl_t *osh);
extern uint osl_pci_slot(osl_t *osh);
extern uint osl_pcie_domain(osl_t *osh);
extern uint osl_pcie_bus(osl_t *osh);
extern struct pci_dev *osl_pci_device(osl_t *osh);

#define OSL_ACP_COHERENCE		(1<<1L)
#define OSL_FWDERBUF			(1<<2L)

/* Pkttag flag should be part of public information */
typedef struct {
	bool pkttag;
	bool mmbus;		/**< Bus supports memory-mapped register accesses */
	pktfree_cb_fn_t tx_fn;  /**< Callback function for PKTFREE */
	void *tx_ctx;		/**< Context to the callback function */
#ifdef OSLREGOPS
	osl_rreg_fn_t rreg_fn;	/**< Read Register function */
	osl_wreg_fn_t wreg_fn;	/**< Write Register function */
	void *reg_ctx;		/**< Context to the reg callback functions */
#else
	void	*unused[3];
#endif
	void (*rx_fn)(void *rx_ctx, void *p);
	void *rx_ctx;
} osl_pubinfo_t;

extern void osl_flag_set(osl_t *osh, uint32 mask);
extern void osl_flag_clr(osl_t *osh, uint32 mask);
extern bool osl_is_flag_set(osl_t *osh, uint32 mask);

#define PKTFREESETCB(osh, _tx_fn, _tx_ctx)		\
	do {						\
	   ((osl_pubinfo_t*)osh)->tx_fn = _tx_fn;	\
	   ((osl_pubinfo_t*)osh)->tx_ctx = _tx_ctx;	\
	} while (0)

#define PKTFREESETRXCB(osh, _rx_fn, _rx_ctx)		\
	do {						\
	   ((osl_pubinfo_t*)osh)->rx_fn = _rx_fn;	\
	   ((osl_pubinfo_t*)osh)->rx_ctx = _rx_ctx;	\
	} while (0)

#ifdef OSLREGOPS
#define REGOPSSET(osh, rreg, wreg, ctx)			\
	do {						\
	   ((osl_pubinfo_t*)osh)->rreg_fn = rreg;	\
	   ((osl_pubinfo_t*)osh)->wreg_fn = wreg;	\
	   ((osl_pubinfo_t*)osh)->reg_ctx = ctx;	\
	} while (0)
#endif /* OSLREGOPS */

/* host/bus architecture-specific byte swap */
#define BUS_SWAP32(v)		(v)
	#define MALLOC(osh, size)	osl_malloc((osh), (size))
	#define MALLOCZ(osh, size)	osl_mallocz((osh), (size))
	#define MFREE(osh, addr, size)	osl_mfree((osh), (addr), (size))
	#define VMALLOC(osh, size)	osl_vmalloc((osh), (size))
	#define VMALLOCZ(osh, size)	osl_vmallocz((osh), (size))
	#define VMFREE(osh, addr, size)	osl_vmfree((osh), (addr), (size))
	#define MALLOCED(osh)		osl_malloced((osh))
	#define MEMORY_LEFTOVER(osh) osl_check_memleak(osh)
	extern void *osl_malloc(osl_t *osh, uint size);
	extern void *osl_mallocz(osl_t *osh, uint size);
	extern void osl_mfree(osl_t *osh, void *addr, uint size);
	extern void *osl_vmalloc(osl_t *osh, uint size);
	extern void *osl_vmallocz(osl_t *osh, uint size);
	extern void osl_vmfree(osl_t *osh, void *addr, uint size);
	extern uint osl_malloced(osl_t *osh);
	extern uint osl_check_memleak(osl_t *osh);

#define	MALLOC_FAILED(osh)	osl_malloc_failed((osh))
extern uint osl_malloc_failed(osl_t *osh);

/* allocate/free shared (dma-able) consistent memory */
#define	DMA_CONSISTENT_ALIGN	osl_dma_consistent_align()
#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap))
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa))

#define	DMA_ALLOC_CONSISTENT_FORCE32(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap))
#define	DMA_FREE_CONSISTENT_FORCE32(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa))

extern uint osl_dma_consistent_align(void);
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align,
	uint *tot, dmaaddr_t *pap);
extern void osl_dma_free_consistent(osl_t *osh, void *va, uint size, dmaaddr_t pa);

/* map/unmap direction */
#define DMA_NO	0	/* Used to skip cache op */
#define	DMA_TX	1	/* TX direction for DMA */
#define	DMA_RX	2	/* RX direction for DMA */

/* map/unmap shared (dma-able) memory */
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah) \
	osl_dma_unmap((osh), (pa), (size), (direction))
extern void osl_dma_flush(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *txp_dmah);
extern dmaaddr_t osl_dma_map(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *txp_dmah);
extern void osl_dma_unmap(osl_t *osh, dmaaddr_t pa, uint size, int direction);

#define	PHYS_TO_VIRT(pa)	osl_phys_to_virt(pa)
#define	VIRT_TO_PHYS(va)	osl_virt_to_phys(va)
extern void * osl_phys_to_virt(void * pa);
extern void * osl_virt_to_phys(void * va);

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) ({BCM_REFERENCE(osh); BCM_REFERENCE(addrwidth);})

#define OSL_SMP_WMB()	smp_wmb()

/* API for CPU relax */
extern void osl_cpu_relax(void);
#define OSL_CPU_RELAX() osl_cpu_relax()

extern void osl_preempt_disable(osl_t *osh);
extern void osl_preempt_enable(osl_t *osh);
#define OSL_DISABLE_PREEMPTION(osh)	osl_preempt_disable(osh)
#define OSL_ENABLE_PREEMPTION(osh)	osl_preempt_enable(osh)

#if defined(__mips__) || (!defined(DHD_USE_COHERENT_MEM_FOR_RING) && \
	defined(__ARM_ARCH_7A__)) || (defined(STBLINUX) && defined(__ARM_ARCH_7A__)) || \
	defined(BCM7271)
	extern void osl_cache_flush(void *va, uint size);
	extern void osl_cache_inv(void *va, uint size);
	extern void osl_prefetch(const void *ptr);
	#define OSL_CACHE_FLUSH(va, len)	osl_cache_flush((void *)(va), len)
	#define OSL_CACHE_INV(va, len)		osl_cache_inv((void *)(va), len)
	#define OSL_PREFETCH(ptr)			osl_prefetch(ptr)
#if defined(__ARM_ARCH_7A__) || defined(BCM7271)
	extern int osl_arch_is_coherent(void);
	#define OSL_ARCH_IS_COHERENT()		osl_arch_is_coherent()
	extern int osl_acp_war_enab(void);
	#define OSL_ACP_WAR_ENAB()			osl_acp_war_enab()
#else  /* !__ARM_ARCH_7A__ */
	#define OSL_ARCH_IS_COHERENT()		NULL
	#define OSL_ACP_WAR_ENAB()			NULL
#endif /* !__ARM_ARCH_7A__ */
#else  /* !__mips__ && !__ARM_ARCH_7A__ */
	#define OSL_CACHE_FLUSH(va, len)	BCM_REFERENCE(va)
	#define OSL_CACHE_INV(va, len)		BCM_REFERENCE(va)
	#define OSL_PREFETCH(ptr)		BCM_REFERENCE(ptr)

	#define OSL_ARCH_IS_COHERENT()		NULL
	#define OSL_ACP_WAR_ENAB()			NULL
#endif /* !__mips__ && !__ARM_ARCH_7A__ */

#ifdef BCM_BACKPLANE_TIMEOUT
extern void osl_set_bpt_cb(osl_t *osh, void *bpt_cb, void *bpt_ctx);
extern void osl_bpt_rreg(osl_t *osh, ulong addr, volatile void *v, uint size);
#endif /* BCM_BACKPLANE_TIMEOUT */

#if defined(BCM47XX_CA9) || (defined(STB) && defined(__arm__))
extern void osl_pcie_rreg(osl_t *osh, ulong addr, volatile void *v, uint size);
#endif	/* BCM47XX_CA9 || (STB && __arm__) */

/* register access macros */
#if defined(BCM_BACKPLANE_TIMEOUT)
#define OSL_READ_REG(osh, r) \
	({\
		__typeof(*(r)) __osl_v; \
		osl_bpt_rreg(osh, (uintptr)(r), &__osl_v, sizeof(*(r))); \
		__osl_v; \
	})
#elif (defined(BCM47XX_CA9) || (defined(STB) && defined(__arm__)))
#define OSL_READ_REG(osh, r) \
	({\
		__typeof(*(r)) __osl_v; \
		osl_pcie_rreg(osh, (uintptr)(r), &__osl_v, sizeof(*(r))); \
		__osl_v; \
	})
#endif 

#if defined(BCM47XX_CA9) || defined(BCM_BACKPLANE_TIMEOUT) || (defined(STB) && \
	defined(__arm__))
	#define SELECT_BUS_WRITE(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); mmap_op;})
	#define SELECT_BUS_READ(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); bus_op;})
#else /* !BCM47XX_CA9 && !BCM_BACKPLANE_TIMEOUT && !(STB && __arm__) */
	#define SELECT_BUS_WRITE(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); mmap_op;})
	#define SELECT_BUS_READ(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); mmap_op;})
#endif /* BCM47XX_CA9 || BCM_BACKPLANE_TIMEOUT || (STB && __arm__) */

#define OSL_ERROR(bcmerror)	osl_error(bcmerror)
extern int osl_error(int bcmerror);

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	2048   /* largest reasonable packet buffer, driver uses for ethernet MTU */

#define OSH_NULL   NULL

/*
 * BINOSL selects the slightly slower function-call-based binary compatible osl.
 * Macros expand to calls to functions defined in linux_osl.c .
 */
#include <linuxver.h>           /* use current 2.4.x calling conventions */
#include <linux/kernel.h>       /* for vsn/printf's */
#include <linux/string.h>       /* for mem*, str* */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 29)
extern uint64 osl_sysuptime_us(void);
#define OSL_SYSUPTIME()		((uint32)jiffies_to_msecs(jiffies))
#define OSL_SYSUPTIME_US()	osl_sysuptime_us()
#else
#define OSL_SYSUPTIME()		((uint32)jiffies * (1000 / HZ))
#error "OSL_SYSUPTIME_US() may need to be defined"
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 29) */
#define	printf(fmt, args...)	printk(fmt , ## args)
#include <linux/kernel.h>	/* for vsn/printf's */
#include <linux/string.h>	/* for mem*, str* */
/* bcopy's: Linux kernel doesn't provide these (anymore) */
#define	bcopy_hw(src, dst, len)		memcpy((dst), (src), (len))
#define	bcopy_hw_async(src, dst, len)	memcpy((dst), (src), (len))
#define	bcopy_hw_poll_for_completion()
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
#define	bzero(b, len)		memset((b), '\0', (len))
#ifdef BCM7271
#define R_REG(osh, r)\
	({ \
		__typeof(*(r)) __osl_v; \
		BCM_REFERENCE(osh); \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):	 __osl_v = *((volatile uint8*) (r)); break; \
			case sizeof(uint16): __osl_v = *((volatile uint16*)(r)); break; \
			case sizeof(uint32): __osl_v = *((volatile uint32*)(r)); break; \
		} \
		__osl_v; \
	})

#define W_REG(osh, r, v) \
	do { \
		BCM_REFERENCE(osh); \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):  *((volatile uint8*)  (r)) = (uint8)(v);  break; \
			case sizeof(uint16): *((volatile uint16*) (r)) = (uint16)(v); break; \
			case sizeof(uint32): *((volatile uint32*) (r)) = (uint32)(v); break; \
		} \
	} while (0)
#else /* !BCM7271 */

/* register access macros */
#if defined(OSLREGOPS)
#define R_REG(osh, r) (\
	sizeof(*(r)) == sizeof(uint8) ? osl_readb((osh), (volatile uint8*)(r)) : \
	sizeof(*(r)) == sizeof(uint16) ? osl_readw((osh), (volatile uint16*)(r)) : \
	osl_readl((osh), (volatile uint32*)(r)) \
)
#define W_REG(osh, r, v) do { \
	switch (sizeof(*(r))) { \
	case sizeof(uint8):	osl_writeb((osh), (volatile uint8*)(r), (uint8)(v)); break; \
	case sizeof(uint16):	osl_writew((osh), (volatile uint16*)(r), (uint16)(v)); break; \
	case sizeof(uint32):	osl_writel((osh), (volatile uint32*)(r), (uint32)(v)); break; \
	} \
} while (0)

extern uint8 osl_readb(osl_t *osh, volatile uint8 *r);
extern uint16 osl_readw(osl_t *osh, volatile uint16 *r);
extern uint32 osl_readl(osl_t *osh, volatile uint32 *r);
extern void osl_writeb(osl_t *osh, volatile uint8 *r, uint8 v);
extern void osl_writew(osl_t *osh, volatile uint16 *r, uint16 v);
extern void osl_writel(osl_t *osh, volatile uint32 *r, uint32 v);

#else /* OSLREGOPS */

#ifndef IL_BIGENDIAN
#ifndef __mips__
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v; \
			BCM_REFERENCE(osh);	\
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)(r)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)(r)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
			} \
			__osl_v; \
		}), \
		OSL_READ_REG(osh, r)) \
)
#else /* __mips__ */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v; \
			__asm__ __volatile__("sync"); \
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)(r)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)(r)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
			} \
			__asm__ __volatile__("sync"); \
			__osl_v; \
		}), \
		({ \
			__typeof(*(r)) __osl_v__; \
			__asm__ __volatile__("sync"); \
			__osl_v__ = OSL_READ_REG(osh, r); \
			__asm__ __volatile__("sync"); \
			__osl_v__; \
		})) \
)
#endif /* __mips__ */

#define W_REG(osh, r, v) do { \
	SELECT_BUS_WRITE(osh, \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):	writeb((uint8)(v), (volatile uint8*)(r)); break; \
			case sizeof(uint16):	writew((uint16)(v), (volatile uint16*)(r)); break; \
			case sizeof(uint32):	writel((uint32)(v), (volatile uint32*)(r)); break; \
		}, \
		(OSL_WRITE_REG(osh, r, v))); \
	} while (0)
#else	/* IL_BIGENDIAN */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v; \
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)((uintptr)(r)^3)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)((uintptr)(r)^2)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
			} \
			__osl_v; \
		}), \
		OSL_READ_REG(osh, r)) \
)
#define W_REG(osh, r, v) do { \
	SELECT_BUS_WRITE(osh, \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):	writeb((uint8)(v), \
					(volatile uint8*)((uintptr)(r)^3)); break; \
			case sizeof(uint16):	writew((uint16)(v), \
					(volatile uint16*)((uintptr)(r)^2)); break; \
			case sizeof(uint32):	writel((uint32)(v), \
					(volatile uint32*)(r)); break; \
		}, \
		(OSL_WRITE_REG(osh, r, v))); \
	} while (0)
#endif /* IL_BIGENDIAN */
#endif /* OSLREGOPS */
#endif /* BCM7271 */

#define	AND_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) & (v))
#define	OR_REG(osh, r, v)		W_REG(osh, (r), R_REG(osh, r) | (v))

/* bcopy, bcmp, and bzero functions */
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
#define	bzero(b, len)		memset((b), '\0', (len))

/* uncached/cached virtual address */
#ifdef __mips__
#include <asm/addrspace.h>
#define OSL_UNCACHED(va)	((void *)KSEG1ADDR((va)))
#define OSL_CACHED(va)		((void *)KSEG0ADDR((va)))
#else
#define OSL_UNCACHED(va)	((void *)va)
#define OSL_CACHED(va)		((void *)va)
#endif /* mips */

#ifdef __mips__
#define OSL_PREF_RANGE_LD(va, sz) prefetch_range_PREF_LOAD_RETAINED(va, sz)
#define OSL_PREF_RANGE_ST(va, sz) prefetch_range_PREF_STORE_RETAINED(va, sz)
#else /* __mips__ */
#define OSL_PREF_RANGE_LD(va, sz) BCM_REFERENCE(va)
#define OSL_PREF_RANGE_ST(va, sz) BCM_REFERENCE(va)
#endif /* __mips__ */

/* get processor cycle count */
#if defined(mips)
#define	OSL_GETCYCLES(x)	((x) = read_c0_count() * 2)
#elif defined(__i386__)
#define	OSL_GETCYCLES(x)	rdtscl((x))
#else
#define OSL_GETCYCLES(x)	((x) = 0)
#endif /* defined(mips) */

/* dereference an address that may cause a bus exception */
#ifdef mips
#if defined(MODULE) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 17))
#define BUSPROBE(val, addr)	panic("get_dbe() will not fixup a bus exception when compiled into"\
					" a module")
#else
#define	BUSPROBE(val, addr)	get_dbe((val), (addr))
#include <asm/paccess.h>
#endif /* defined(MODULE) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 17)) */
#else
#define	BUSPROBE(val, addr)	({ (val) = R_REG(NULL, (addr)); 0; })
#endif /* mips */

/* map/unmap physical to virtual I/O */
#if !defined(CONFIG_MMC_MSM7X00A)
#define	REG_MAP(pa, size)	ioremap_nocache((unsigned long)(pa), (unsigned long)(size))
#else
#define REG_MAP(pa, size)       (void *)(0)
#endif /* !defined(CONFIG_MMC_MSM7X00A */
#define	REG_UNMAP(va)		iounmap((va))

/* shared (dma-able) memory access macros */
#define	R_SM(r)			*(r)
#define	W_SM(r, v)		(*(r) = (v))
#define	BZERO_SM(r, len)	memset((r), '\0', (len))

/* Because the non BINOSL implemenation of the PKT OSL routines are macros (for
 * performance reasons),  we need the Linux headers.
 */
#include <linuxver.h>		/* use current 2.4.x calling conventions */


#define OSL_RAND()		osl_rand()
extern uint32 osl_rand(void);

#define	DMA_FLUSH(osh, va, size, direction, p, dmah) \
	osl_dma_flush((osh), (va), (size), (direction), (p), (dmah))
#if !defined(BCM_SECURE_DMA)
#define	DMA_MAP(osh, va, size, direction, p, dmah) \
	osl_dma_map((osh), (va), (size), (direction), (p), (dmah))
#endif /* !(defined(BCM_SECURE_DMA)) */

#else /* ! BCMDRIVER */


/* ASSERT */
	#define ASSERT(exp)	do {} while (0)

/* MALLOC and MFREE */
#define MALLOC(o, l) malloc(l)
#define MFREE(o, p, l) free(p)
#include <stdlib.h>

/* str* and mem* functions */
#include <string.h>

/* *printf functions */
#include <stdio.h>

/* bcopy, bcmp, and bzero */
extern void bcopy(const void *src, void *dst, size_t len);
extern int bcmp(const void *b1, const void *b2, size_t len);
extern void bzero(void *b, size_t len);
#endif /* ! BCMDRIVER */

/* Current STB 7445D1 doesn't use ACP and it is non-coherrent.
 * Adding these dummy values for build apss only
 * When we revisit need to change these.
 */
#if defined(STBLINUX)

#if defined(__ARM_ARCH_7A__) || defined(BCM7271)
#define ACP_WAR_ENAB() 0
#define ACP_WIN_LIMIT 1
#define arch_is_coherent() 0
#endif /* __ARM_ARCH_7A__ */

#endif /* STBLINUX */

#ifdef BCM_SECURE_DMA

#define	SECURE_DMA_MAP(osh, va, size, direction, p, dmah, pcma, offset) \
	osl_sec_dma_map((osh), (va), (size), (direction), (p), (dmah), (pcma), (offset))
#define	SECURE_DMA_DD_MAP(osh, va, size, direction, p, dmah) \
	osl_sec_dma_dd_map((osh), (va), (size), (direction), (p), (dmah))
#define	SECURE_DMA_MAP_TXMETA(osh, va, size, direction, p, dmah, pcma) \
	osl_sec_dma_map_txmeta((osh), (va), (size), (direction), (p), (dmah), (pcma))
#define	SECURE_DMA_UNMAP(osh, pa, size, direction, p, dmah, pcma, offset) \
	osl_sec_dma_unmap((osh), (pa), (size), (direction), (p), (dmah), (pcma), (offset))
#define	SECURE_DMA_UNMAP_ALL(osh, pcma) \
	osl_sec_dma_unmap_all((osh), (pcma))

#define DMA_MAP(osh, va, size, direction, p, dmah)

typedef struct sec_cma_info {
	struct sec_mem_elem *sec_alloc_list;
	struct sec_mem_elem *sec_alloc_list_tail;
} sec_cma_info_t;

#if defined(__ARM_ARCH_7A__)
#define CMA_BUFSIZE_4K	4096
#define CMA_BUFSIZE_2K	2048
#define CMA_BUFSIZE_512	512

#define	CMA_BUFNUM		2048
#define SEC_CMA_COHERENT_BLK 0x8000 /* 32768 */
#define SEC_CMA_COHERENT_MAX 278
#define CMA_DMA_DESC_MEMBLOCK	(SEC_CMA_COHERENT_BLK * SEC_CMA_COHERENT_MAX)
#define CMA_DMA_DATA_MEMBLOCK	(CMA_BUFSIZE_4K*CMA_BUFNUM)
#define	CMA_MEMBLOCK		(CMA_DMA_DESC_MEMBLOCK + CMA_DMA_DATA_MEMBLOCK)
#define CONT_REGION	0x02		/* Region CMA */
#else
#define CONT_REGION	0x00		/* To access the MIPs mem, Not yet... */
#endif /* !defined __ARM_ARCH_7A__ */

#define SEC_DMA_ALIGN	(1<<16)
typedef struct sec_mem_elem {
	size_t			size;
	int				direction;
	phys_addr_t		pa_cma;     /**< physical  address */
	void			*va;        /**< virtual address of driver pkt */
	dma_addr_t		dma_handle; /**< bus address assign by linux */
	void			*vac;       /**< virtual address of cma buffer */
	struct page *pa_cma_page;	/* phys to page address */
	struct	sec_mem_elem	*next;
} sec_mem_elem_t;

extern dma_addr_t osl_sec_dma_map(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *dmah, void *ptr_cma_info, uint offset);
extern dma_addr_t osl_sec_dma_dd_map(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *dmah);
extern dma_addr_t osl_sec_dma_map_txmeta(osl_t *osh, void *va, uint size,
  int direction, void *p, hnddma_seg_map_t *dmah, void *ptr_cma_info);
extern void osl_sec_dma_unmap(osl_t *osh, dma_addr_t dma_handle, uint size, int direction,
	void *p, hnddma_seg_map_t *map, void *ptr_cma_info, uint offset);
extern void osl_sec_dma_unmap_all(osl_t *osh, void *ptr_cma_info);

#endif /* BCM_SECURE_DMA */

typedef struct sk_buff_head PKT_LIST;
#define PKTLIST_INIT(x)		skb_queue_head_init((x))
#define PKTLIST_ENQ(x, y)	skb_queue_head((struct sk_buff_head *)(x), (struct sk_buff *)(y))
#define PKTLIST_DEQ(x)		skb_dequeue((struct sk_buff_head *)(x))
#define PKTLIST_UNLINK(x, y)	skb_unlink((struct sk_buff *)(y), (struct sk_buff_head *)(x))
#define PKTLIST_FINI(x)		skb_queue_purge((struct sk_buff_head *)(x))

#endif	/* _linux_osl_h_ */
