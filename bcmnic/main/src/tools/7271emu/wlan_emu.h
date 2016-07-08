/******************************************************************************
 * (c) 2015 Broadcom Corporation
 *
 * This program is the proprietary software of Broadcom Corporation and/or its
 * licensors, and may only be used, duplicated, modified or distributed pursuant
 * to the terms and conditions of a separate, written license agreement executed
 * between you and Broadcom (an "Authorized License").  Except as set forth in
 * an Authorized License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and Broadcom
 * expressly reserves all rights in and to the Software and all intellectual
 * property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU
 * HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY
 * NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 *    constitutes the valuable trade secrets of Broadcom, and you shall use all
 *    reasonable efforts to protect the confidentiality thereof, and to use
 *    this information only in connection with your use of Broadcom integrated
 *    circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH RESPECT
 *    TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL IMPLIED
 *    WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *    PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *    ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME
 *    THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR ITS
 *    LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT,
 *    OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *    YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN
 *    ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS
 *    OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1, WHICHEVER
 *    IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *    ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/

#ifndef __WLAN_EMU_H__
#define __WLAN_EMU_H__

#include <asm/types.h>

typedef enum
{
    wlan_emu_success = 0,
    wlan_emu_fail
} wlan_emu_status;

typedef struct wlan_emu_reg
{
    __u64 offset;               /* [in] register offset from the base address  */
    __u64 data;                 /* [in/out] data  */
    __u32 size;                 /* optional size 0,8,16,32,64 default =0 means 32 bit */
} wlan_emu_reg;

typedef struct wlan_emu_mem
{
    __u64 remote_addr;             /* [in] physical address in the remote memory */
    __u64 local_addr;              /* [in] virtual addrees of the local memory */
    __u32 len;                     /* [in] size in bytes */
} wlan_emu_mem;

typedef enum wlan_emu_operation
{
    WLAN_EMU_INVALID = 0,
    WLAN_EMU_REG_READ,
    WLAN_EMU_REG_WRITE,
    WLAN_EMU_MEM_ALLOC,
    WLAN_EMU_MEM_FREE,
    WLAN_EMU_MEM_READ,
    WLAN_EMU_MEM_WRITE,
    WLAN_EMU_QUIT
} wlan_emu_operation;

typedef struct wlan_emu_ioc_message
{
    wlan_emu_operation op;
    wlan_emu_reg       reg;
    wlan_emu_mem       mem;
    int                status;                /* 0 success >0 failed */
} wlan_emu_ioc_message;

/* server ioctls */
#define WLAN_EMU_DEV_NAME     "wlan_emu"
#define WLAN_EMU_DEV_MAGIC    'k'
#define WLAN_EMU_IOC_REQUEST  _IOWR( WLAN_EMU_DEV_MAGIC, 1, int ) /* ioclt call to recive the request from kernel mode */
#define WLAN_EMU_IOC_RESPONSE _IOWR( WLAN_EMU_DEV_MAGIC, 2, int ) /* ioclt call to send respose to the kernel mode  */

/* client ioctls */
#define WLAN_EMU_CLIENT_DEV_NAME  "wlan_emu_client"
#define WLAN_EMU_CLIENT_DEV_MAGIC 'l'
#define WLAN_EMU_IOC_CLIENT       _IOWR( WLAN_EMU_CLIENT_DEV_MAGIC, 1, int )  /* ioclt to test server */

#define WLAN_EMU_DEVICE_NAME        "/dev/mem"
#define WLAN_EMU_DEVICE_NAME_CLIENT "/dev/" WLAN_EMU_CLIENT_DEV_NAME "0"
#define WLAN_EMU_DEVICE_NAME_SERVER "/dev/" WLAN_EMU_DEV_NAME "0"

#define BUFFER_MEMORY_LEN (( 4096/2 )*4 )   /* four packets */

#ifdef __KERNEL__
static inline char * decode_op(wlan_emu_operation op)
{
    char* tmp[WLAN_EMU_QUIT]=
    {
        "WLAN_EMU_INVALID",
        "WLAN_EMU_REG_READ",
        "WLAN_EMU_REG_WRITE",
        "WLAN_EMU_MEM_ALLOC",
        "WLAN_EMU_MEM_FREE",
        "WLAN_EMU_MEM_READ",
        "WLAN_EMU_MEM_WRITE"
    };
    if(op>=WLAN_EMU_QUIT)
        return "WLAN_EMU_OP_UNKNOWN";
    else
        return tmp[op];
}
#include <linux/interrupt.h>

#if defined(BCM7271_EMU)
extern int wlan_emu_reg_read( wlan_emu_reg *reg );
extern int wlan_emu_reg_write( wlan_emu_reg *reg );
extern int wlan_emu_remote_malloc( wlan_emu_mem *mem );  /* returns physical address on the remote machine */
extern int wlan_emu_remote_free( wlan_emu_mem *mem );    /* free  physical address on the remote machine */
extern int wlan_emu_mem_read( wlan_emu_mem *mem );
extern int wlan_emu_mem_write( wlan_emu_mem *mem );

/* typedef irqreturn_t (*irq_handler_t)(int, void *);*/
extern int wlan_emu_request_irq(unsigned int  	irq,       /* PA address of the interrupt register that needs to be polled */
                                irq_handler_t  	handler,
                                unsigned long  	irqflags,  /* Unused */
                                const char *  	devname,   /* Unused */
                                void *  	dev_id);

extern void wlan_emu_free_irq (	unsigned int  	irq,
                        void *  	dev_id);
#endif /* define(BCM7271_EMU)*/

#endif /* ifdef __KERNEL__ */
#endif /* __WLAN_EMU_H__ */
