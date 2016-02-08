/*
 * Dongle BUS interface
 * USB Linux Implementation
 *
 * Copyright (C) 1999-2011, Broadcom Corporation
 *
 *     Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *     As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *     Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dbus_usb_linux.c,v 1.43.2.14 2011-02-04 16:23:37 $
 */

#include <typedefs.h>
#include <osl.h>
#include <sys/sx.h>
#include <sys/bus.h>
#include <sys/module.h>
#include <sys/condvar.h>
#include <sys/firmware.h>
#include <dev/usb/usb_freebsd.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usb_core.h>
#include <dev/usb/usb_device.h>
#include <sys/taskqueue.h>
#include <dev/usb/usb_process.h>

#include <bcmdevs.h>
#include <bcmutils.h>
#include <wlioctl.h>
#include <usbrdl.h>
#include <dbus.h>
#include <bcm_rpc_tp.h>

#ifdef BCM_REQUEST_FW
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/syscallsubr.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/systm.h>
#endif

/* dbus_usbos_intf CallBack Functions */
static void *dbus_usbos_intf_attach(dbus_pub_t *pub, void *cbarg, dbus_intf_callbacks_t *cbs);
static void dbus_usbos_intf_detach(dbus_pub_t *pub, void *info);
static int dbus_usbos_intf_up(void *bus);
static int dbus_usbos_intf_down(void *bus);
static int dbus_usbos_intf_send_irb(void *bus, dbus_irb_tx_t *txirb);
static int dbus_usbos_intf_recv_irb(void *bus, dbus_irb_rx_t *rxirb);
static int dbus_usbos_intf_cancel_irb(void *bus, dbus_irb_tx_t *txirb);
static int dbus_usbos_intf_send_ctl(void *bus, uint8 *buf, int len);
static int dbus_usbos_intf_recv_ctl(void *bus, uint8 *buf, int len);
static int dbus_usbos_intf_get_attrib(void *bus, dbus_attrib_t *attrib);
static int dbus_usbos_intf_pnp(void *bus, int evnt);
static int dbus_usbos_intf_stop(void *bus);
static int dbus_usbos_intf_set_config(void *bus, dbus_config_t *config);
static bool dbus_usbos_intf_recv_needed(void *bus);
static void *dbus_usbos_intf_exec_rxlock(void *bus, exec_cb_t cb, struct exec_parms *args);
static void *dbus_usbos_intf_exec_txlock(void *bus, exec_cb_t cb, struct exec_parms *args);
static int dbus_usbos_intf_recv_irb_from_ep(void *bus, dbus_irb_rx_t *rxirb, uint32 ep_idx);
static int dbus_usbos_readreg(void *bus, uint32 regaddr, int datalen, uint32 *value);
extern int dbus_usbos_loopback_tx(void *usbos_info_ptr, int cnt, int size);
static void dbus_usbos_recv_task(void *usbos_info_ptr, int npending);
static void dbus_usbos_send_task(void *usbos_info_ptr, int npending);

/* Macros */
#define CALLBACK_ARGS    struct usb_xfer *xfer, usb_error_t error

#define FETCH_LIST_HEAD_ITEM(lock, queue)  do {                    \
	MTX_LOCK(&usbos_info->lock);                               \
	if ((data = STAILQ_FIRST(&usbos_info->queue))) {          \
		STAILQ_REMOVE_HEAD(&usbos_info->queue, next);      \
	}                                                          \
	MTX_UNLOCK(&usbos_info->lock);                             \
} while (0)

#ifndef BWL_REQ_LIST_COUNT
#define BWL_REQ_LIST_COUNT         4
#endif

#define BWL_TX_MINFREE             2

/* #define BCMWL_HW_IN_PENDING_FRAMES  (BWL_RX_LIST_COUNT > 4 ? 4 : BWL_RX_LIST_COUNT) */
#define BCMWL_HW_IN_PENDING_FRAMES  1 /* FreeBSD don't support rx multi frames */
#define BCMWL_HW_OUT_PENDING_FRAMES (BWL_TX_LIST_COUNT > 32 ? 32 : BWL_TX_LIST_COUNT)

#define BCM_USB_MAX_BURST_BYTES    (64 * 1024)

#define DBUS_USB_FBSD_DRIVER_NAME   "brcm_wifi"
#define DBUS_USB_FBSD_MODULE_NAME   brcm_wifi

#define USB_SYNC_WAIT_TIMEOUT	300	/* ms */

#define USB_STR_DESC_REMOTE_DOWNLOAD "Remote Download Wireless Adapter"


enum {
	BWL_BULK_RD = 0,
	BWL_BULK_WR,
	BWL_CTRL_RD,
	BWL_CTRL_WR,
	BWL_BULK_DL,
	BWL_N_TRANSFER
};

struct bwl_tx_data {
	STAILQ_ENTRY(bwl_tx_data) next;
	void *sc;
	int  status;
	dbus_irb_tx_t *txirb;
	char *buf;
	int  buf_len;
};

struct bwl_rx_data {
	STAILQ_ENTRY(bwl_rx_data) next;
	void *sc;
	int  status;
	dbus_irb_rx_t *rxirb;
	void *buf;
	int  buf_len;
};

struct bwl_req_data {
	STAILQ_ENTRY(bwl_req_data) next;
	void *sc;
	struct usb_device_request req;
	void *buf;
	int  len;
};

typedef STAILQ_HEAD(, bwl_tx_data) bwl_txdhead;
typedef STAILQ_HEAD(, bwl_rx_data) bwl_rxdhead;
typedef STAILQ_HEAD(, bwl_req_data) bwl_reqhead;

#ifdef BCM_REQUEST_FW
typedef struct {
		char *fw_name;
		char *fw_buf;
		int  fw_size;
	} fw_t;
#endif

typedef struct {
	dbus_pub_t  *pub;
	void        *cbarg;
	dbus_intf_callbacks_t *cbs;

	device_t    sc_dev;    /* back pointer to device */
	struct usb_device  *sc_udev;

	struct usb_xfer  *sc_xfer[BWL_N_TRANSFER];

	struct bwl_tx_data  tx_data[BWL_TX_LIST_COUNT];
	struct bwl_rx_data  rx_data[BWL_RX_LIST_COUNT];
	struct bwl_req_data ctlreq_data[BWL_REQ_LIST_COUNT];

	uint32         tx_posted;
	bwl_txdhead    tx_freeq;
	struct mtx     tx_freeq_lock;
	bwl_txdhead    tx_q;
	struct mtx     tx_q_lock;
	struct taskqueue *tx_tq;
	struct task    tx_task;
	bwl_txdhead    tx_task_q;
	struct mtx     tx_task_q_lock;

	uint32         rx_posted;
	bwl_rxdhead    rx_freeq;
	struct mtx     rx_freeq_lock;
	bwl_rxdhead    rx_q;
	struct mtx     rx_q_lock;
	struct taskqueue *rx_tq;
	struct task    rx_task;
	bwl_rxdhead    rx_task_q;
	struct mtx     rx_task_q_lock;

	bwl_reqhead    ctl_freeq;
	struct mtx     ctl_freeq_lock;
	bwl_reqhead    txctl_reqq;
	struct mtx     txctl_reqq_lock;
	bwl_reqhead    rxctl_reqq;
	struct mtx     rxctl_reqq_lock;

	struct cv    sc_cv;
	struct mtx   sc_mtx;
	struct mtx   txlock;
	struct mtx   rxlock;
	struct mtx   dllock;

	uint  rxbuf_len;
	int   firm_dl_success;

#ifdef BCM_REQUEST_FW
	struct proc *fw_proc;
	fw_t fw[2];
	fw_t *curfw;
#endif
} usbos_info_t;

static struct usb_device_id devid_table[] = {
	/* Configurable via register() */
	{ USB_VP(BCM_DNGL_VID, 0x0000), },
#if defined(BCM_REQUEST_FW)
	{ USB_VP(BCM_DNGL_VID, BCM_DNGL_BL_PID_43236), },
	{ USB_VP(BCM_DNGL_VID, BCM_DNGL_BL_PID_43242), },
	{ USB_VP(BCM_DNGL_VID, BCM_DNGL_BL_PID_43569), },
#endif
	/* Default BDC */
	{ USB_VP(BCM_DNGL_VID, BCM_DNGL_BDC_PID), },
};


/* Global API Functions */
bool dbus_usbos_dl_cmd(void *usbinfo, uint8 cmd, void *buffer, int buflen);
bool dbus_usbos_dl_send_bulk(usbos_info_t *usbinfo, void *buf, int len);
int dbus_usbos_wait(usbos_info_t *usbinfo, uint16 ms);
int dbus_write_membytes(usbos_info_t* usbinfo, bool set, uint32 address, uint8 *data, uint size);


/* Static API Functions */
static int dbus_usbos_state_change(void *bus, int state);
static void dbusos_stop(usbos_info_t *usbos_info);

/* Function to notify upper layer or return buffers */
static void dbus_usbos_ctl_complete(struct bwl_req_data *data, int type, int status);
static void dbus_usbos_recv_complete(struct bwl_rx_data *data, int act_len, int status);
static void dbus_usbos_send_complete(struct bwl_tx_data *data, int status);

/* Static USB Callback Functions */
static void dbus_usbos_recv_callback(CALLBACK_ARGS);
static void dbus_usbos_send_callback(CALLBACK_ARGS);
static void dbus_usbos_ctlread_callback(CALLBACK_ARGS);
static void dbus_usbos_ctlwrite_callback(CALLBACK_ARGS);
static void dbus_usbos_bulk_dl_callback(CALLBACK_ARGS);

/* TODO: Needs to review the following parameters */
static const struct usb_config bwl_config[BWL_N_TRANSFER] =
{
	[BWL_BULK_RD] = {
		.type      = UE_BULK,
		.endpoint  = UE_ADDR_ANY,
		.direction = UE_DIR_IN,
		.frames    = BCMWL_HW_IN_PENDING_FRAMES,
		.bufsize   = BCMWL_HW_IN_PENDING_FRAMES * BWL_RX_BUFFER_SIZE,
		.flags     = {
			.pipe_bof = 1, .short_xfer_ok = 1, .ext_buffer = 1
		},
		.callback  = dbus_usbos_recv_callback,
		/* .timeout   = 5000, */       /* ms */
	},
	[BWL_BULK_WR] = {
		.type      = UE_BULK,
		.endpoint  = UE_ADDR_ANY,
		.direction = UE_DIR_OUT,
		.frames    = BCMWL_HW_OUT_PENDING_FRAMES,
		.bufsize   = BCMWL_HW_OUT_PENDING_FRAMES * BCM_RPC_TP_HOST_AGG_MAX_BYTE,
		.flags     = {
			.pipe_bof = 1, .force_short_xfer = 1, .ext_buffer = 1
		},
		.callback  = dbus_usbos_send_callback,
		.timeout   = 5000,        /* ms */
	},
	[BWL_CTRL_RD] = {
		.type      = UE_CONTROL,
		.endpoint  = 0x00,        /* Control pipe */
		.direction = UE_DIR_ANY,
		.frames    = 2,
		.bufsize   = 2 * (WLC_IOCTL_MAXLEN + sizeof(cdc_ioctl_t) +
		sizeof(struct usb_device_request)),
		.flags     = {
			.short_xfer_ok = 1, .short_frames_ok = 1, .ext_buffer = 1
		},
		.callback  = dbus_usbos_ctlread_callback,
		.timeout   = 1000,        /* ms */
	},
	[BWL_CTRL_WR] = {
		.type      = UE_CONTROL,
		.endpoint  = 0x00,        /* Control pipe */
		.direction = UE_DIR_OUT,
		.frames    = 2,
		.bufsize   = 2 * (WLC_IOCTL_MAXLEN + sizeof(cdc_ioctl_t) +
		sizeof(struct usb_device_request)),
		.flags     = {
			.short_xfer_ok = 1, .force_short_xfer = 1,
			.short_frames_ok = 1, .ext_buffer = 1
		},
		.callback  = dbus_usbos_ctlwrite_callback,
		.timeout   = 1000,        /* ms */
	},
	[BWL_BULK_DL] = {
		.type      = UE_BULK,
		.endpoint  = UE_ADDR_ANY, /* Data pipe */
		.direction = UE_DIR_OUT,
		/* MCLBYTES: (1 << 11) = 2048, defined in sys/param.h */
		.frames    = 1,
		.bufsize   = (MCLBYTES + 20),
		.flags     = {
			.pipe_bof = 1, .force_short_xfer = 1, .ext_buffer = 1
		},
		.callback  = dbus_usbos_bulk_dl_callback,
		.timeout   = 5000,        /* ms */
	},
};

/* This stores USB info during probe callback
 * since attach() is not called yet at this point
 */
typedef struct {
	void   *usbos_info;

	/* USB device pointer from OS */
	struct usb_device    *usb;
	struct usb_interface *intf;
	DEVICE_SPEED   device_speed;

	/* Pipe numbers for USB I/O */
	uint  tx_pipe;
	uint  rx_pipe;
	uint  rx_pipe2;
	uint  intr_pipe;

	int   vid;
	int   pid;
	int   intr_size;  /* Size of interrupt message */
	int   interval;   /* Interrupt polling interval */
	bool  dldone;
	bool  dereged;
	bool  disc_cb_done;

	enum dbus_state  busstate;
} probe_info_t;


/* Static Variables */
static probe_cb_t probe_cb = NULL;
static disconnect_cb_t disconnect_cb = NULL;
static void *probe_arg = NULL;
static void *disc_arg = NULL;

static probe_info_t g_probe_info;

static struct sx mod_lock;

/* Global Variables */
device_t bwl_device = NULL;

static dbus_intf_t dbus_usbos_intf = {
	.attach     = dbus_usbos_intf_attach,
	.detach     = dbus_usbos_intf_detach,
	.up         = dbus_usbos_intf_up,
	.down       = dbus_usbos_intf_down,
	.send_irb   = dbus_usbos_intf_send_irb,
	.recv_irb   = dbus_usbos_intf_recv_irb,
	.cancel_irb = dbus_usbos_intf_cancel_irb,
	.send_ctl   = dbus_usbos_intf_send_ctl,
	.recv_ctl   = dbus_usbos_intf_recv_ctl,
	.get_stats  = NULL,
	.get_attrib = dbus_usbos_intf_get_attrib,

	.pnp        = dbus_usbos_intf_pnp,
	.remove     = NULL,
	.resume     = NULL,
	.suspend    = NULL,
	.stop       = dbus_usbos_intf_stop,
	.reset      = NULL,

	/* Access to bus buffers directly */
	.pktget     = NULL,
	.pktfree    = NULL,

	.iovar_op   = NULL,
	.dump       = NULL,
	.set_config = dbus_usbos_intf_set_config,
	.get_config = NULL,

	.device_exists  = NULL,
	.dlneeded       = NULL,
	.dlstart        = NULL,
	.dlrun          = NULL,
	.recv_needed    = dbus_usbos_intf_recv_needed,

	.exec_rxlock    = dbus_usbos_intf_exec_rxlock,
	.exec_txlock    = dbus_usbos_intf_exec_txlock,

	.tx_timer_init  = NULL,
	.tx_timer_start = NULL,
	.tx_timer_stop  = NULL,

	.sched_dpc      = NULL,
	.lock           = NULL,
	.unlock         = NULL,
	.sched_probe_cb = NULL,

	.shutdown       = NULL,

	.recv_stop      = NULL,
	.recv_resume    = NULL,

	.recv_irb_from_ep = dbus_usbos_intf_recv_irb_from_ep,
	.readreg          = dbus_usbos_readreg,
};


#ifdef DBUS_USBOS_DEBUG
int dbus_usbos_dump_lookup_info(struct usbd_lookup_info *info)
{
	if (info == NULL) {
		printf("Cannot dump NULL descriptor\n");
		return;
	}
	printf("Dumping struct usb_lookup_info ------------>\n");
	printf("idVendor=%x, idProduct=%x, bcdDevice=%x\n",
		info->idVendor, info->idProduct, info->bcdDevice);
	printf("bDeviceClass=%x, bDeviceSubClass=%x, bDeviceProtocol=%x\n",
		info->bDeviceClass, info->bDeviceSubClass, info->bDeviceProtocol);
	printf("bInterfaceClass=%x, bInterfaceSubClass=%x, bInterfaceProtocol=%x\n",
		info->bInterfaceClass, info->bInterfaceSubClass, info->bInterfaceProtocol);
	printf("bIfaceIndex=%x, bIfaceNum=%x, bCOnfigIndex=%x, bConfigNum=%x\n",
		info->bIfaceIndex, info->bIfaceNum, info->bConfigIndex, info->bConfigNum);
};


int dbus_usbos_dump_usbdevice_desc(struct usb_device_descriptor *dev)
{
	printf("Dumping struct usb_device_descriptor ------------>\n");
	if (dev == NULL) {
		printf("Cannot dump NULL descriptor\n");
		return;
	}
	printf("idVendor=%x, idProduct=%x, bcdDevice=%x\n",
		dev->idVendor, dev->idProduct, dev->bcdDevice);
	printf("bDeviceClass=%x, bDeviceSubClass=%x, bDeviceProtocol=%x\n",
		dev->bDeviceClass, dev->bDeviceSubClass, dev->bDeviceProtocol);
	printf("iManufacturer=%x, iProduct=%x, bNumConfigureations=%x\n",
		dev->iManufacturer, dev->iProduct, dev->bNumConfigurations);
};

int dbus_usbos_dump_config_desc(struct usb_config_descriptor *conf)
{
	printf("Dumping struct usb_config_descriptor ------------>\n");
	if (conf == NULL) {
		printf("Cannot dump NULL descriptor\n");
		return;
	}
	printf("bLength=%x, bDescriptorType=%x, wTotalLength=%x\n",
		conf->bLength, conf->bDescriptorType, conf->wTotalLength);
	printf("bNumInterface=%x, bConfigurationValue=%x, iConfiguration=%x\n",
		conf->bNumInterface, conf->bConfigurationValue, conf->iConfiguration);
	printf("bmAttributes=%x, bMaxPower=%x\n",
		conf->bmAttributes, conf->bMaxPower);
};

int dbus_usbos_dump_edesc(struct usb_endpoint_descriptor *edesc)
{
	printf("Dumping struct usb_endpoint_descriptor ----------->\n");
	if (edesc == NULL) {
		printf("the usb_eondpoint_descriptor is NULL");
		return 0;
	}
	printf("bLength=%x, bDescriptorType=%x, bEndpointAddress=%x\n",
		edesc->bLength, edesc->bDescriptorType, edesc->bEndpointAddress);
	printf("bmAttributes=%x, wMaxPacketSize=%x, bInterval=%x\n",
		edesc->bmAttributes, edesc->wMaxPacketSize, edesc->bInterval);
}
#endif /* #ifdef DBUS_USBOS_DEBUG */

static void
bwl_req_free(struct bwl_req_data *data)
{
	if (data) {
		usbos_info_t *usbos_info = (usbos_info_t *)(data->sc);
		MTX_LOCK(&usbos_info->ctl_freeq_lock);
		STAILQ_INSERT_TAIL(&usbos_info->ctl_freeq, data, next);
		MTX_UNLOCK(&usbos_info->ctl_freeq_lock);
	}
}

static void
bwl_tx_free(struct bwl_tx_data *data)
{
	if (data) {
		usbos_info_t *usbos_info = (usbos_info_t *)(data->sc);
		data->txirb = NULL;
		MTX_LOCK(&usbos_info->tx_freeq_lock);
		STAILQ_INSERT_TAIL(&usbos_info->tx_freeq, data, next);
		MTX_UNLOCK(&usbos_info->tx_freeq_lock);
	}
}

static void
bwl_rx_free(struct bwl_rx_data *data)
{
	if (data) {
		usbos_info_t *usbos_info = (usbos_info_t *)(data->sc);
		data->rxirb = NULL;
		MTX_LOCK(&usbos_info->rx_freeq_lock);
		STAILQ_INSERT_TAIL(&usbos_info->rx_freeq, data, next);
		MTX_UNLOCK(&usbos_info->rx_freeq_lock);
	}
}

#ifdef NOT_YET
static void
dbus_usbos_unlink(struct list_head *urbreq_q, spinlock_t *lock)
{
	DBUSERR(("%s(): Not implemented \n", __FUNCTION__));
	return DBUS_ERR;
}

static int
dbus_usbos_suspend(struct usb_interface *intf, pm_message_t message)
{
	DBUSERR(("%s(): Not implemented \n", __FUNCTION__));
	return DBUS_ERR;
}

static int dbus_usbos_resume(struct usb_interface *intf)
{
	DBUSERR(("%s(): Not implemented \n", __FUNCTION__));
	return DBUS_ERR;
}

static void
dbus_usbos_sleep(usbos_info_t *usbos_info)
{
	DBUSERR(("%s(): Not implemented \n", __FUNCTION__));
	return;
}


int
dbus_usbos_writereg(void *bus, uint32 regaddr, int datalen, uint32 data)
{
	DBUSERR(("%s(): Error: Not implemented. \n", __FUNCTION__));
	return (DBUS_ERR);
}
#endif /* #ifdef NOT_YET */

static void *
dbus_usbos_intf_attach(dbus_pub_t *pub, void *cbarg, dbus_intf_callbacks_t *cbs)
{
	usbos_info_t *usbos_info;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));


	/* Sanity check for BUS_INFO() */
	ASSERT(OFFSETOF(usbos_info_t, pub) == 0);

	usbos_info = (usbos_info_t *)(g_probe_info.usbos_info);

	if (usbos_info == NULL) {
		DBUSERR(("%s(): ERROR.. usbos_info == NULL \n", __FUNCTION__));
		return NULL;
	}

	usbos_info->pub   = pub;
	usbos_info->cbarg = cbarg;
	usbos_info->cbs   = cbs;
	usbos_info->pub->busstate = g_probe_info.busstate;
	usbos_info->pub->device_speed = g_probe_info.device_speed;
	usbos_info->rxbuf_len = (uint)usbos_info->pub->rxsize;

	pub->dev_info = g_probe_info.usb;

	/* Setup Tx/Rx List */
	{
		int count;

		STAILQ_INIT(&usbos_info->tx_q);
		STAILQ_INIT(&usbos_info->tx_freeq);

		MTX_INIT(&usbos_info->tx_q_lock, "txqlock", NULL, MTX_DEF);
		MTX_INIT(&usbos_info->tx_freeq_lock, "txqfreelock", NULL, MTX_DEF);

		STAILQ_INIT(&usbos_info->tx_task_q);
		MTX_INIT(&usbos_info->tx_task_q_lock, "txtqlock", NULL, MTX_DEF);
		usbos_info->tx_tq = taskqueue_create("wl_tx_tq", 0,
			taskqueue_thread_enqueue, &usbos_info->tx_tq);
			taskqueue_start_threads(&usbos_info->tx_tq, 1,
			USB_PRI_MED, "wl tx_tq");
		TASK_INIT(&usbos_info->tx_task, 0, dbus_usbos_send_task, usbos_info);


		for (count = 0; count < BWL_TX_LIST_COUNT; count++) {
			struct bwl_tx_data *data = &usbos_info->tx_data[count];
			data->buf_len = BWL_TX_BUFFER_SIZE;
			data->buf = MALLOC(usbos_info->pub->osh, data->buf_len);
			if (data->buf == NULL) {
				DBUSERR(("%s: allocate TX buffer failed\n", __FUNCTION__));
				ASSERT(0);
			}
			data->sc = usbos_info;
			bwl_tx_free(data);
		}

		STAILQ_INIT(&usbos_info->rx_q);
		STAILQ_INIT(&usbos_info->rx_freeq);

		MTX_INIT(&usbos_info->rx_q_lock, "rxqlock", NULL, MTX_DEF);
		MTX_INIT(&usbos_info->rx_freeq_lock, "rxqfreelock", NULL, MTX_DEF);
		STAILQ_INIT(&usbos_info->rx_task_q);
		MTX_INIT(&usbos_info->rx_task_q_lock, "rxtqlock", NULL, MTX_DEF);
		usbos_info->rx_tq = taskqueue_create("wl_rx_tq", 0,
			taskqueue_thread_enqueue, &usbos_info->rx_tq);
		taskqueue_start_threads(&usbos_info->rx_tq, 1,
			USB_PRI_MED, "wl rx_tq");
		TASK_INIT(&usbos_info->rx_task, 0, dbus_usbos_recv_task, usbos_info);

		for (count = 0; count < BWL_RX_LIST_COUNT; count++) {
			struct bwl_rx_data *data = &usbos_info->rx_data[count];
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY)
			/* don't allocate now. Do it on demand */
			data->buf_len = 0;
			data->buf = NULL;
#else
			/* pre-allocate  buffers never to be released */
			data->buf_len = usbos_info->rxbuf_len;
			data->buf = MALLOC(usbos_info->pub->osh, data->buf_len);
			if (data->buf == NULL) {
				DBUSERR(("%s: allocate RX buffer failed\n", __FUNCTION__));
				ASSERT(0);
			}
#endif
			data->sc = usbos_info;
			bwl_rx_free(data);
		}

		STAILQ_INIT(&usbos_info->ctl_freeq);
		STAILQ_INIT(&usbos_info->txctl_reqq);
		STAILQ_INIT(&usbos_info->rxctl_reqq);
		MTX_INIT(&usbos_info->ctl_freeq_lock, "ctl_freeq_lock", NULL, MTX_DEF);
		MTX_INIT(&usbos_info->txctl_reqq_lock, "txctl_reqq_lock", NULL, MTX_DEF);
		MTX_INIT(&usbos_info->rxctl_reqq_lock, "rxctl_reqq_lock", NULL, MTX_DEF);

		for (count = 0; count < BWL_REQ_LIST_COUNT; count++) {
			struct bwl_req_data *data = &usbos_info->ctlreq_data[count];
			data->sc = usbos_info;
			bwl_req_free(data);
		}
	}

	MTX_INIT(&usbos_info->txlock, "txlock", NULL, MTX_DEF);
	MTX_INIT(&usbos_info->rxlock, "rxlock", NULL, MTX_DEF);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return ((void *)usbos_info);

}


static void
dbus_usbos_intf_detach(dbus_pub_t *pub, void *info)
{
	usbos_info_t *usbos_info = (usbos_info_t *) info;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (usbos_info == NULL) {
		DBUSERR(("%s(): ERROR.. usbos_info == NULL \n", __FUNCTION__));
		return;
	}

	dbusos_stop(usbos_info);

	usbd_transfer_drain(usbos_info->sc_xfer[BWL_CTRL_WR]);
	usbd_transfer_drain(usbos_info->sc_xfer[BWL_CTRL_RD]);

	/* free Tx/Rx list, if any */
	{
		struct bwl_tx_data *tx_data;
		struct bwl_rx_data *rx_data;
		struct bwl_req_data *req_data;
		int count;

		taskqueue_drain(usbos_info->tx_tq, &usbos_info->tx_task);
		taskqueue_free(usbos_info->tx_tq);
		usbos_info->tx_tq = NULL;

		MTX_LOCK(&usbos_info->tx_task_q_lock);
		while ((tx_data = STAILQ_FIRST(&usbos_info->tx_task_q)) != NULL) {
			STAILQ_REMOVE_HEAD(&usbos_info->tx_task_q, next);
			dbus_usbos_send_complete(tx_data, DBUS_ERR_DISCONNECT);
		}
		STAILQ_INIT(&usbos_info->tx_task_q);
		MTX_UNLOCK(&usbos_info->tx_task_q_lock);

		MTX_LOCK(&usbos_info->tx_q_lock);
		while ((tx_data = STAILQ_FIRST(&usbos_info->tx_q)) != NULL) {
			STAILQ_REMOVE_HEAD(&usbos_info->tx_q, next);
			dbus_usbos_send_complete(tx_data, DBUS_ERR_DISCONNECT);
		}
		STAILQ_INIT(&usbos_info->tx_q);
		MTX_UNLOCK(&usbos_info->tx_q_lock);

		MTX_LOCK(&usbos_info->tx_freeq_lock);
		STAILQ_INIT(&usbos_info->tx_freeq);
		MTX_UNLOCK(&usbos_info->tx_freeq_lock);

		MTX_DESTROY(&usbos_info->tx_q_lock);
		MTX_DESTROY(&usbos_info->tx_freeq_lock);
		MTX_DESTROY(&usbos_info->tx_task_q_lock);

		taskqueue_drain(usbos_info->rx_tq, &usbos_info->rx_task);
		taskqueue_free(usbos_info->rx_tq);
		usbos_info->rx_tq = NULL;

		MTX_LOCK(&usbos_info->rx_task_q_lock);
		while ((rx_data = STAILQ_FIRST(&usbos_info->rx_task_q)) != NULL) {
			STAILQ_REMOVE_HEAD(&usbos_info->rx_task_q, next);
			dbus_usbos_recv_complete(rx_data, 0, DBUS_ERR_DISCONNECT);
		}
		STAILQ_INIT(&usbos_info->rx_task_q);
		MTX_UNLOCK(&usbos_info->rx_task_q_lock);

		MTX_LOCK(&usbos_info->rx_q_lock);
		while ((rx_data = STAILQ_FIRST(&usbos_info->rx_q)) != NULL) {
			STAILQ_REMOVE_HEAD(&usbos_info->rx_q, next);
			dbus_usbos_recv_complete(rx_data, 0, DBUS_ERR_DISCONNECT);
		}
		STAILQ_INIT(&usbos_info->rx_q);
		MTX_UNLOCK(&usbos_info->rx_q_lock);

		MTX_LOCK(&usbos_info->rx_freeq_lock);
		STAILQ_INIT(&usbos_info->rx_freeq);
		MTX_UNLOCK(&usbos_info->rx_freeq_lock);

		MTX_DESTROY(&usbos_info->rx_q_lock);
		MTX_DESTROY(&usbos_info->rx_freeq_lock);
		MTX_DESTROY(&usbos_info->rx_task_q_lock);

		for (count = 0; count < BWL_TX_LIST_COUNT; count++) {
			tx_data = &usbos_info->tx_data[count];
			if (tx_data->buf != NULL) {
				MFREE(usbos_info->pub->osh, tx_data->buf, tx_data->buf_len);
				tx_data->buf = NULL;
				tx_data->buf_len = 0;
			}
		}

		for (count = 0; count < BWL_RX_LIST_COUNT; count++) {
			rx_data = &usbos_info->rx_data[count];
			if (rx_data->buf != NULL) {
				MFREE(usbos_info->pub->osh, rx_data->buf, rx_data->buf_len);
				rx_data->buf = NULL;
				rx_data->buf_len = 0;
			}
		}

		MTX_LOCK(&usbos_info->sc_mtx);

		MTX_LOCK(&usbos_info->rxctl_reqq_lock);
		while ((req_data = STAILQ_FIRST(&usbos_info->rxctl_reqq)) != NULL) {
			STAILQ_REMOVE_HEAD(&usbos_info->rxctl_reqq, next);
			dbus_usbos_ctl_complete(req_data, DBUS_CBCTL_READ, USB_ERR_CANCELLED);
		}
		STAILQ_INIT(&usbos_info->rxctl_reqq);
		MTX_UNLOCK(&usbos_info->rxctl_reqq_lock);

		MTX_LOCK(&usbos_info->txctl_reqq_lock);
		while ((req_data = STAILQ_FIRST(&usbos_info->txctl_reqq)) != NULL) {
			STAILQ_REMOVE_HEAD(&usbos_info->txctl_reqq, next);
			dbus_usbos_ctl_complete(req_data, DBUS_CBCTL_WRITE, USB_ERR_CANCELLED);
		}
		STAILQ_INIT(&usbos_info->txctl_reqq);
		MTX_UNLOCK(&usbos_info->txctl_reqq_lock);

		MTX_UNLOCK(&usbos_info->sc_mtx);

		STAILQ_INIT(&usbos_info->ctl_freeq);

		MTX_DESTROY(&usbos_info->ctl_freeq_lock);
		MTX_DESTROY(&usbos_info->txctl_reqq_lock);
		MTX_DESTROY(&usbos_info->rxctl_reqq_lock);
	}

	MTX_DESTROY(&usbos_info->txlock);
	MTX_DESTROY(&usbos_info->rxlock);

	/* g_probe_info.usbos_info = NULL; */
	/* MFREE(osh, usbos_info, sizeof(usbos_info_t)); */

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));
}


static int
dbus_usbos_intf_up(void *bus)
{
	usbos_info_t *usbos_info = (usbos_info_t *)bus;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (usbos_info == NULL) {
		DBUSERR(("%s(): usbos_info is NULL \n", __FUNCTION__));
		return DBUS_ERR;
	}

	dbus_usbos_state_change(usbos_info, DBUS_STATE_UP);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}


static int
dbus_usbos_intf_down(void *bus)
{
	usbos_info_t *usbos_info = (usbos_info_t *)bus;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (usbos_info == NULL) {
		DBUSERR(("%s(): usbos_info is NULL \n", __FUNCTION__));
		return DBUS_ERR;
	}

	dbusos_stop(usbos_info);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}

static int
dbus_usbos_intf_send_irb(void *bus, dbus_irb_tx_t *txirb)
{
	usbos_info_t    *usbos_info = (usbos_info_t *)bus;
	struct usb_xfer *xfer = usbos_info->sc_xfer[BWL_BULK_WR];
	struct bwl_tx_data *data;
	int kick = 0;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	FETCH_LIST_HEAD_ITEM(tx_freeq_lock, tx_freeq);
	if (!data) {
		DBUSERR(("%s(): Exit for no free sending entry\n", __FUNCTION__));
		return DBUS_ERR_TXDROP;
	}

	if (txirb->buf) {
		ASSERT(txirb->len > 0 && txirb->len <= BWL_TX_BUFFER_SIZE);
	} else if (txirb->pkt) {
		int len;
		struct mbuf *m, *ml;
		m = (struct mbuf *)txirb->pkt;
		len = m_length(m, &ml);
		ASSERT(len > 0 && len <= BWL_TX_BUFFER_SIZE);

		if (m == ml) {
			/* only one buf */
			txirb->buf = PKTDATA(usbos_info->pub->osh, txirb->pkt);
			txirb->len = len;
		} else {
			/*
			 * copy mbuf data to a linear buffer
			 */
			m_copydata(txirb->pkt, 0, len, data->buf);

			txirb->buf = data->buf;
			txirb->len = len;
		}
	} else {
		ASSERT(0);
	}

	data->txirb = txirb;

	MTX_LOCK(&usbos_info->tx_q_lock);
	if (STAILQ_EMPTY(&usbos_info->tx_q))
		kick = 1;
	STAILQ_INSERT_TAIL(&usbos_info->tx_q, data, next);
	MTX_UNLOCK(&usbos_info->tx_q_lock);

	if (kick) {
		MTX_LOCK(&usbos_info->sc_mtx);
		usbd_transfer_start(xfer);
		MTX_UNLOCK(&usbos_info->sc_mtx);
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}


/*
 * Mapping to NONE. It looks that FreeBSD start it with usbd_transfer_start()
 * at inital stage and would re-trigger in the callbacks forever
 */
static int
dbus_usbos_intf_recv_irb(void *bus, dbus_irb_rx_t *rxirb)
{
	usbos_info_t    *usbos_info = (usbos_info_t *)bus;
	struct usb_xfer *xfer = usbos_info->sc_xfer[BWL_BULK_RD];
	struct bwl_rx_data *data;
	int kick = 0;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (rxirb->buf) {
		DBUSERR(("%s(): rxirb->buf expected to be NULL\n", __FUNCTION__));
		ASSERT(0);
		return DBUS_ERR;
	}

	FETCH_LIST_HEAD_ITEM(rx_freeq_lock, rx_freeq);
	if (!data) {
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY)
		DBUSERR(("%s(): Exit for no free receiving entry\n", __FUNCTION__));
#endif
		return DBUS_ERR_RXDROP;
	}

#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY)
	rxirb->pkt = PKTGET(usbos_info->pub->osh, usbos_info->rxbuf_len, FALSE);
	if (!rxirb->pkt) {
		DBUSERR(("%s: PKTGET failed\n", __FUNCTION__));
		return DBUS_ERR_RXDROP;
	}
	/* consider the packet "native" so we don't count it as MALLOCED in the osl */
	/* PKTTONATIVE(usbos_info->pub->osh, rxirb->pkt); not implemented yet */
	/* temporary, will be cleared before submig to upper layer */
	rxirb->buf = PKTDATA(usbos_info->pub->osh, rxirb->pkt);
	rxirb->buf_len = usbos_info->rxbuf_len;
#else
	if (data->buf_len != usbos_info->rxbuf_len) {
		ASSERT(data->buf);
		MFREE(usbos_info->pub->osh, data->buf, data->buf_len);
		DBUSTRACE(("%s: replace rx buff: old len %d, new len %d\n", __FUNCTION__,
			data->buf_len, usbos_info->rxbuf_len));
		data->buf_len = 0;
		data->buf = MALLOC(usbos_info->pub->osh, usbos_info->rxbuf_len);
		if (data->buf == NULL) {
			DBUSERR(("%s: MALLOC req->pkt failed\n", __FUNCTION__));
			return DBUS_ERR_NOMEM;
		}
		data->buf_len = usbos_info->rxbuf_len;
	}
	rxirb->buf = data->buf;
	rxirb->buf_len = data->buf_len;
#endif /* defined(BCM_RPC_NOCOPY) */

	data->rxirb = rxirb;

	MTX_LOCK(&usbos_info->rx_q_lock);

	if (STAILQ_EMPTY(&usbos_info->rx_q))
		kick = 1;

	STAILQ_INSERT_TAIL(&usbos_info->rx_q, data, next);
	MTX_UNLOCK(&usbos_info->rx_q_lock);

	if (kick) {
		MTX_LOCK(&usbos_info->sc_mtx);
		usbd_transfer_start(xfer);
		MTX_UNLOCK(&usbos_info->sc_mtx);
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}


static int
dbus_usbos_intf_cancel_irb(void *bus, dbus_irb_tx_t *txirb)
{
	DBUSERR(("%s(): Not implemented \n", __FUNCTION__));
	return DBUS_ERR;
}

static int
dbus_usbos_intf_send_ctl(void *bus, uint8 *buf, int len)
{
	usbos_info_t    *usbos_info = (usbos_info_t *)bus;
	struct usb_xfer *xfer = usbos_info->sc_xfer[BWL_CTRL_WR];
	struct bwl_req_data *data;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (!buf || !len) {
		DBUSERR(("%s(): send_ctl was expected with valid buf and len\n", __FUNCTION__));
		ASSERT(0);
		return DBUS_ERR;
	}

	FETCH_LIST_HEAD_ITEM(ctl_freeq_lock, ctl_freeq);
	if (!data) {
		DBUSERR(("%s(): Exit for no free ctl req entry\n", __FUNCTION__));
		return DBUS_ERR_TXCTLFAIL;
	}

	data->req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	data->req.bRequest = DL_GETSTATE;
	USETW(data->req.wValue, 0);
	USETW(data->req.wIndex, 0);
	USETW(data->req.wLength, len);
	data->buf = buf;
	data->len = len;

	MTX_LOCK(&usbos_info->txctl_reqq_lock);
	STAILQ_INSERT_TAIL(&usbos_info->txctl_reqq, data, next);
	MTX_UNLOCK(&usbos_info->txctl_reqq_lock);

	MTX_LOCK(&usbos_info->sc_mtx);
	usbd_transfer_start(xfer);
	MTX_UNLOCK(&usbos_info->sc_mtx);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}


static int
dbus_usbos_intf_recv_ctl(void *bus, uint8 *buf, int len)
{
	usbos_info_t    *usbos_info = (usbos_info_t *)bus;
	struct usb_xfer *xfer = usbos_info->sc_xfer[BWL_CTRL_RD];
	struct bwl_req_data *data;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (!buf || !len) {
		DBUSERR(("%s(): send_ctl was expected with valid buf and len\n", __FUNCTION__));
		ASSERT(0);
		return DBUS_ERR;
	}

	bzero((char *)buf, len);

	FETCH_LIST_HEAD_ITEM(ctl_freeq_lock, ctl_freeq);
	if (!data) {
		DBUSERR(("%s(): Exit for no free ctl req entry\n", __FUNCTION__));
		return DBUS_ERR_RXCTLFAIL;
	}

	data->req.bmRequestType = UT_READ_CLASS_INTERFACE;
	data->req.bRequest = DL_CHECK_CRC;
	USETW(data->req.wValue, 0);
	USETW(data->req.wIndex, 0);
	USETW(data->req.wLength, len);
	data->buf = buf;
	data->len = len;

	MTX_LOCK(&usbos_info->rxctl_reqq_lock);
	STAILQ_INSERT_TAIL(&usbos_info->rxctl_reqq, data, next);
	MTX_UNLOCK(&usbos_info->rxctl_reqq_lock);

	MTX_LOCK(&usbos_info->sc_mtx);
	usbd_transfer_start(xfer);
	MTX_UNLOCK(&usbos_info->sc_mtx);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}


static int
dbus_usbos_intf_get_attrib(void *bus, dbus_attrib_t *attrib)
{
	usbos_info_t *usbos_info = (usbos_info_t *) bus;

	if ((usbos_info == NULL) || (attrib == NULL))
		return DBUS_ERR;

	attrib->bustype = DBUS_USB;
	attrib->vid     = g_probe_info.vid;
	attrib->pid     = g_probe_info.pid;
	attrib->devid   = g_probe_info.pid;

	/* TODO: Needs to review the following parameters */

	/* attrib->nchan = 1; */
	/* attrib->mtu = usbos_info->maxps; */

	return DBUS_OK;
}

static int
dbus_usbos_intf_pnp(void *bus, int evnt)
{
	DBUSERR(("%s(): Not implemented \n", __FUNCTION__));
	return DBUS_ERR;
}

static int
dbus_usbos_intf_stop(void *bus)
{
	usbos_info_t *usbos_info = (usbos_info_t *)bus;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (usbos_info == NULL) {
		DBUSERR(("%s(): usbos_info is NULL \n", __FUNCTION__));
		return DBUS_ERR;
	}

	dbusos_stop(usbos_info);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}

static int
dbus_usbos_intf_set_config(void *bus, dbus_config_t *config)
{
	int err = DBUS_ERR;
	usbos_info_t* usbos_info = bus;

	if (config->config_id == DBUS_CONFIG_ID_AGGR_LIMIT) {
#ifndef BCM_FD_AGGR
		/* DBUS_CONFIG_ID_AGGR_LIMIT shouldn't be called after probe stage */
		ASSERT(disc_arg == NULL);
#endif /* BCM_FD_AGGR */
		ASSERT(config->aggr_param.maxrxsf > 0);
		ASSERT(config->aggr_param.maxrxsize > 0);
		if (config->aggr_param.maxrxsize > usbos_info->rxbuf_len) {
			int state = usbos_info->pub->busstate;
			usbd_transfer_drain(usbos_info->sc_xfer[BWL_BULK_RD]);
			usbos_info->rxbuf_len = config->aggr_param.maxrxsize;
			dbus_usbos_state_change(usbos_info, state);
		}
		err = DBUS_OK;
	}

	return err;
}

static bool
dbus_usbos_intf_recv_needed(void *bus)
{
	DBUSERR(("%s(): Not implemented \n", __FUNCTION__));
	return FALSE;
}

static void*
dbus_usbos_intf_exec_rxlock(void *bus, exec_cb_t cb, struct exec_parms *args)
{
	usbos_info_t *usbos_info = (usbos_info_t *)bus;
	void *ret;

	/* DBUSTRACE(("%s(): Enter \n", __FUNCTION__)); */

	if (usbos_info == NULL) {
		DBUSERR(("%s(): usbos_info is NULL \n", __FUNCTION__));
		return NULL;
	}

	MTX_LOCK(&usbos_info->rxlock);
	ret = cb(args);
	MTX_UNLOCK(&usbos_info->rxlock);

	/* DBUSTRACE(("%s(): Finish \n", __FUNCTION__)); */

	return ret;
}

static void*
dbus_usbos_intf_exec_txlock(void *bus, exec_cb_t cb, struct exec_parms *args)
{
	usbos_info_t *usbos_info = (usbos_info_t *)bus;
	void *ret;

	/* DBUSTRACE(("%s(): Enter \n", __FUNCTION__)); */

	if (usbos_info == NULL) {
		DBUSERR(("%s(): usbos_info is NULL \n", __FUNCTION__));
		return NULL;
	}

	MTX_LOCK(&usbos_info->txlock);
	ret = cb(args);
	MTX_UNLOCK(&usbos_info->txlock);

	/* DBUSTRACE(("%s(): Finish \n", __FUNCTION__)); */

	return ret;
}

static int
dbus_usbos_intf_recv_irb_from_ep(void *bus, dbus_irb_rx_t *rxirb, uint32 ep_idx)
{
	DBUSERR(("%s(): Error: Not implemented. \n", __FUNCTION__));
	return DBUS_OK;
}

static int
dbus_usbos_readreg(void *bus, uint32 regaddr, int datalen, uint32 *value)
{
	DBUSERR(("%s(): Error: Not implemented. \n", __FUNCTION__));
	return (DBUS_ERR);
}

bool
dbus_usbos_dl_cmd(void *usbinfo, uint8 cmd, void *buffer, int buflen)
{
	usbos_info_t *usbos_info = (usbos_info_t *)usbinfo;
	struct usb_device_request req;
	usb_error_t error = 0;
	int ntries = 10;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if ((usbinfo == NULL) || (buffer == NULL) || (buflen == 0)) {
		DBUSERR(("%s(): invalid param \n", __FUNCTION__));
		return FALSE;
	}

	req.bmRequestType = UT_READ_VENDOR_INTERFACE;
	req.bRequest = cmd;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, buflen);

	while (ntries--) {
		MTX_LOCK(&usbos_info->sc_mtx);
		error = usbd_do_request_flags(usbos_info->sc_udev, &usbos_info->sc_mtx,
			&req, buffer, USB_SHORT_XFER_OK,
			NULL, USB_SYNC_WAIT_TIMEOUT); /* ms */
		MTX_UNLOCK(&usbos_info->sc_mtx);
		if ((error == 0) || (error == USB_ERR_TIMEOUT))
			break;
	}

	if (error != 0) {
		DBUSERR(("%s(): Error: Could not read device interface. error %s \n",
			__FUNCTION__, usbd_errstr(error)));
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return (error == 0);
}


bool
dbus_usbos_dl_send_bulk(usbos_info_t *usbinfo, void *buf, int len)
{
	struct usb_xfer *xfer;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if ((usbinfo == NULL) || (buf == NULL)) {
		DBUSERR(("%s(): ERROR.. NULL pointer(s) \n", __FUNCTION__));
		return FALSE;
	}
	if (len > RDL_CHUNK) {
		DBUSERR(("%s(): ERROR.. len (%d) > RDL_CHUNK (%d) \n",
			__FUNCTION__, len, RDL_CHUNK));
		return FALSE;
	}

	xfer = usbinfo->sc_xfer[BWL_BULK_DL];

	if (xfer == NULL) {
		DBUSERR(("%s(): xfer is NULL\n", __FUNCTION__));
		return FALSE;
	}

	MTX_LOCK(&usbinfo->dllock);

	MTX_LOCK(&usbinfo->sc_mtx);
	usbd_xfer_set_frames(xfer, 1);
	usbd_xfer_set_frame_data(xfer, 0, buf, len);

	usbd_transfer_start(xfer);
	MTX_UNLOCK(&usbinfo->sc_mtx);

	cv_wait(&usbinfo->sc_cv, &usbinfo->dllock);

	MTX_UNLOCK(&usbinfo->dllock);

	if (usbinfo->firm_dl_success == FALSE) {
		DBUSERR(("%s(): ERROR.. usbinfo->firm_dl_success = FALSE \n", __FUNCTION__));
		return FALSE;
	}

	return TRUE;
}

int
dbus_usbos_wait(usbos_info_t *usbinfo, uint16 ms)
{
	DBUSTRACE(("%s(): Delay = %d ms \n", __FUNCTION__, ms));
	osl_delay(ms);
	return DBUS_OK;
}

int
dbus_write_membytes(usbos_info_t* usbinfo, bool set, uint32 address, uint8 *data, uint size)
{
	DBUSERR(("%s(): Error: Not implemented. \n", __FUNCTION__));
	return DBUS_OK;
}


static void
dbusos_stop(usbos_info_t *usbos_info)
{
	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	ASSERT(usbos_info);

	usbd_transfer_drain(usbos_info->sc_xfer[BWL_BULK_WR]);
	usbd_transfer_drain(usbos_info->sc_xfer[BWL_BULK_RD]);

	dbus_usbos_state_change(usbos_info, DBUS_STATE_DOWN);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return;
}

/*
	called with usbos_info->sc_mtx hold
	if called from callback, it is alaready holded by USB core code
*/
static void
dbus_usbos_ctl_complete(struct bwl_req_data *data, int type, int status)
{
	if (!data) {
		DBUSERR(("%s(): check why will call with data == NULL?\n", __FUNCTION__));
		ASSERT(0);
	}
	if (data) {
		usbos_info_t *usbos_info = (usbos_info_t *)(data->sc);
		if (likely (usbos_info->cbarg && usbos_info->cbs)) {
			if (likely (usbos_info->cbs->ctl_complete)) {
				MTX_UNLOCK(&usbos_info->sc_mtx);
				usbos_info->cbs->ctl_complete(usbos_info->cbarg, type, status);
				MTX_LOCK(&usbos_info->sc_mtx);
			}
		}

		bwl_req_free(data);
	}
}

static void
dbus_usbos_ctlread_callback(CALLBACK_ARGS)
{
	usbos_info_t *usbos_info = usbd_xfer_softc(xfer);
	struct bwl_req_data *data = usbd_xfer_get_priv(xfer);

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	switch (USB_GET_STATE(xfer)) {
		case USB_ST_TRANSFERRED:
			usbd_xfer_set_priv(xfer, NULL);
			dbus_usbos_ctl_complete(data, DBUS_CBCTL_READ, error);
			/* no break, FALLTHROUGH */

		case USB_ST_SETUP:
SET_UP_XFER:
			FETCH_LIST_HEAD_ITEM(rxctl_reqq_lock, rxctl_reqq);
			if (!data) {
				usbd_xfer_set_priv(xfer, NULL);
				DBUSTRACE(("%s(): end of rxctl_reqq \n", __FUNCTION__));
				break;
			}

			usbd_xfer_set_frames(xfer, 2);
			usbd_xfer_set_frame_data(xfer, 0, &data->req, sizeof(data->req));
			usbd_xfer_set_frame_data(xfer, 1, data->buf, data->len);
			usbd_xfer_set_priv(xfer, data);
			usbd_transfer_submit(xfer);
			break;

		default:
			DBUSERR(("%s(): error = %s\n", __FUNCTION__, usbd_errstr(error)));
			usbd_xfer_set_priv(xfer, NULL);

			if ((error == USB_ERR_CANCELLED) || (error == USB_ERR_STALLED))
			{
				DBUSERR(("setting DBUS_STATE_DOWN for %s\n", usbd_errstr(error)));
				dbus_usbos_state_change(usbos_info, DBUS_STATE_DOWN);
			}

			dbus_usbos_ctl_complete(data, DBUS_CBCTL_READ, error);

			if ((error != USB_ERR_CANCELLED) && (error != USB_ERR_STALLED)) {
				usbd_xfer_set_stall(xfer);
				goto SET_UP_XFER;
			} else {
				/* return all req to the queue */
				MTX_LOCK(&usbos_info->rxctl_reqq_lock);
				while ((data = STAILQ_FIRST(&usbos_info->rxctl_reqq)) != NULL) {
					STAILQ_REMOVE_HEAD(&usbos_info->rxctl_reqq, next);
					dbus_usbos_ctl_complete(data, DBUS_CBCTL_READ, error);
				}
				STAILQ_INIT(&usbos_info->rxctl_reqq);
				MTX_UNLOCK(&usbos_info->rxctl_reqq_lock);
			}
			break;
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));
}

static void
dbus_usbos_ctlwrite_callback(CALLBACK_ARGS)
{
	usbos_info_t *usbos_info = usbd_xfer_softc(xfer);
	struct bwl_req_data *data = usbd_xfer_get_priv(xfer);

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	switch (USB_GET_STATE(xfer)) {
		case USB_ST_TRANSFERRED:
			usbd_xfer_set_priv(xfer, NULL);
			dbus_usbos_ctl_complete(data, DBUS_CBCTL_WRITE, error);
			/* no break, FALLTHROUGH */

		case USB_ST_SETUP:
SET_UP_XFER:
			FETCH_LIST_HEAD_ITEM(txctl_reqq_lock, txctl_reqq);
			if (!data) {
				usbd_xfer_set_priv(xfer, NULL);
				DBUSTRACE(("%s(): end of txctl_reqq \n", __FUNCTION__));
				break;
			}

			usbd_xfer_set_frames(xfer, 2);
			usbd_xfer_set_frame_data(xfer, 0, &data->req, sizeof(data->req));
			usbd_xfer_set_frame_data(xfer, 1, data->buf, data->len);
			usbd_xfer_set_priv(xfer, data);
			usbd_transfer_submit(xfer);
			break;

		default:
			DBUSERR(("%s(): error = %s\n", __FUNCTION__, usbd_errstr(error)));
			usbd_xfer_set_priv(xfer, NULL);

			if ((error == USB_ERR_CANCELLED) /* || (error == USB_ERR_STALLED) */)
			{
				DBUSERR(("setting DBUS_STATE_DOWN for %s\n", usbd_errstr(error)));
				dbus_usbos_state_change(usbos_info, DBUS_STATE_DOWN);
			}

			dbus_usbos_ctl_complete(data, DBUS_CBCTL_WRITE, error);

			if ((error != USB_ERR_CANCELLED) && (error != USB_ERR_STALLED)) {
				usbd_xfer_set_stall(xfer);
				goto SET_UP_XFER;
			} else {
				/* return all req to the queue */
				MTX_LOCK(&usbos_info->txctl_reqq_lock);
				while ((data = STAILQ_FIRST(&usbos_info->txctl_reqq)) != NULL) {
					STAILQ_REMOVE_HEAD(&usbos_info->txctl_reqq, next);
					dbus_usbos_ctl_complete(data, DBUS_CBCTL_WRITE, error);
				}
				STAILQ_INIT(&usbos_info->txctl_reqq);
				MTX_UNLOCK(&usbos_info->txctl_reqq_lock);
			}
			break;
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));
}

static void
dbus_usbos_recv_task(void *usb_info_ptr, int npending)
{
	usbos_info_t *usbos_info = usb_info_ptr;
	struct bwl_rx_data *data;

	while (1) {
		FETCH_LIST_HEAD_ITEM(rx_task_q_lock, rx_task_q);
		if (!data)
			break;
		if ((data->status == DBUS_OK) &&
			(data->rxirb->buf == data->buf)) {
			if (data->rxirb->actual_len < sizeof(uint32)) {
				DBUSTRACE(("small pkt len %d, process as ZLP\n",
					data->rxirb->actual_len));
				data->status = DBUS_ERR_RXZLP;
			}
		}

		usbos_info->cbs->recv_irb_complete(usbos_info->cbarg,
			data->rxirb, data->status);
		bwl_rx_free(data);
	}
}

static void
dbus_usbos_recv_complete(struct bwl_rx_data *data, int act_len, int status)
{
	int need_free = 1;
	if (!data) {
		DBUSERR(("%s(): check why will call with data == NULL?\n", __FUNCTION__));
		ASSERT(0);
	}
	if (data) {
		if (data->rxirb && data->rxirb->buf) {
			usbos_info_t *usbos_info = (usbos_info_t *)(data->sc);
			if (likely (usbos_info->cbarg && usbos_info->cbs)) {
				if (likely (usbos_info->cbs->recv_irb_complete)) {
					data->rxirb->actual_len = act_len;
#if defined(BCM_RPC_NOCOPY) || defined(BCM_RPC_RXNOCOPY)
					data->rxirb->buf_len = 0;
					data->rxirb->buf = data->rxirb->pkt;
					PKTSETLEN(usbos_info->pub->osh,
						data->rxirb->pkt, act_len);
					data->rxirb->actual_len = 0;
					if (status != DBUS_OK) {
						data->rxirb->pkt = NULL;
					}
#endif
					if (status != DBUS_ERR_DISCONNECT) {
						data->status = status;
						mtx_lock(&usbos_info->rx_task_q_lock);
						STAILQ_INSERT_TAIL(&usbos_info->rx_task_q, data,
							next);
						mtx_unlock(&usbos_info->rx_task_q_lock);
						taskqueue_enqueue(usbos_info->rx_tq,
							&usbos_info->rx_task);
						need_free = 0;
					}
					else {
						usbos_info->cbs->recv_irb_complete(
							usbos_info->cbarg, data->rxirb, status);
					}
				}
			}
		}

		if (need_free)
			bwl_rx_free(data);
	}
}

static void
dbus_usbos_recv_callback(CALLBACK_ARGS)
{
	usbos_info_t *usbos_info = usbd_xfer_softc(xfer);
	struct bwl_rx_data *data;
	int actlen, sumlen, aframes, nframes, datalen, nr_frames;
	uint8 *buf;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));
	usbd_xfer_status(xfer, &actlen, &sumlen, &aframes, &nframes);

	switch (USB_GET_STATE(xfer)) {

		case USB_ST_TRANSFERRED:
			DBUSTRACE(("%s(): Transfered %d frames with actlen = %d sumlen = %d\n",
				__FUNCTION__, aframes, actlen, sumlen));
			if (aframes > 1) {
				DBUSERR(("Not expected: got %d (> 1) transferred RX buffer\n",
					aframes));
			}
			for (nr_frames = 0; nr_frames != aframes; nr_frames++) {
				FETCH_LIST_HEAD_ITEM(rx_q_lock, rx_q);
				if (!data) {
					DBUSERR(("got xfer frame but the rx_q till end ?? \n"));
					ASSERT(0);
					/* make coverity happy */
					return;
				}
				usbd_xfer_frame_data(xfer, nr_frames, (void**)&buf, &datalen);
				if ((data->rxirb->buf != buf) || (data->rxirb->buf_len < datalen)) {
					DBUSERR(("the buff or data length not match ?? \n"));
					ASSERT(0);
				}
				dbus_usbos_recv_complete(data, datalen, DBUS_OK);
				usbos_info->rx_posted--;
			}
			if (usbos_info->rx_posted) {
				/* check if remained buffer will be used if not post again */
				DBUSERR(("%d posted RX buffer remained\n",
					usbos_info->rx_posted));
			}
			/* no break, FALLTHROUGH */

		case USB_ST_SETUP:
SET_UP_XFER:
			nr_frames = 0;
			usbos_info->rx_posted = 0;
			MTX_LOCK(&usbos_info->rx_q_lock);
			STAILQ_FOREACH(data, &usbos_info->rx_q, next) {
				usbd_xfer_set_frame_data(xfer, nr_frames,
					data->rxirb->buf, data->rxirb->buf_len);
				if (xfer->max_data_length != data->rxirb->buf_len)
					xfer->max_data_length = data->rxirb->buf_len;
				nr_frames++;
				usbos_info->rx_posted++;
				if (usbos_info->rx_posted >= BCMWL_HW_IN_PENDING_FRAMES)
					break; /* break out from STAILQ_FOREACH */
			}
			MTX_UNLOCK(&usbos_info->rx_q_lock);

			if (usbos_info->rx_posted) {
				usbd_xfer_set_frames(xfer, usbos_info->rx_posted);
				usbd_transfer_submit(xfer);
			} else {
				DBUSTRACE(("%s(): end of rx_q \n", __FUNCTION__));
			}

			break;

		default:
			DBUSERR(("%s(): error = %s with %d bytes transfered\n",
				__FUNCTION__, usbd_errstr(error), actlen));

			if (error == USB_ERR_STALLED || error == USB_ERR_IOERROR) {
				DBUSERR(("%s(): calling DBUS_STATE_DOWN for %s\n",
					__FUNCTION__, usbd_errstr(error)));
				dbus_usbos_state_change(usbos_info, DBUS_STATE_DOWN);
			}

			if ((error != USB_ERR_CANCELLED) && (error != USB_ERR_STALLED)) {
				usbd_xfer_set_stall(xfer);
				goto SET_UP_XFER;
			} else {
				/* return all rxirb in the queue */
				MTX_UNLOCK(&usbos_info->sc_mtx);
				MTX_LOCK(&usbos_info->rx_q_lock);
				while ((data = STAILQ_FIRST(&usbos_info->rx_q)) != NULL) {
					STAILQ_REMOVE_HEAD(&usbos_info->rx_q, next);
					dbus_usbos_recv_complete(data, 0, DBUS_ERR_RXFAIL);
				}
				STAILQ_INIT(&usbos_info->rx_q);
				MTX_UNLOCK(&usbos_info->rx_q_lock);
				MTX_LOCK(&usbos_info->sc_mtx);
			}

			break;
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));
}

static void
dbus_usbos_send_task(void *usb_info_ptr, int npending)
{
	usbos_info_t *usbos_info = usb_info_ptr;
	struct bwl_tx_data *data;

	while (1) {
		FETCH_LIST_HEAD_ITEM(tx_task_q_lock, tx_task_q);
		if (!data)
			break;

		usbos_info->cbs->send_irb_complete(usbos_info->cbarg,
			data->txirb, data->status);
		bwl_tx_free(data);
	}
}

static void
dbus_usbos_send_complete(struct bwl_tx_data *data, int status)
{
	int need_free = 1;

	if (!data) {
		DBUSERR(("%s(): check why will call with data == NULL?\n", __FUNCTION__));
		ASSERT(0);
	}
	if (data) {
		if (data->txirb) {
			usbos_info_t *usbos_info = (usbos_info_t *)(data->sc);
			if (likely (usbos_info->cbarg && usbos_info->cbs)) {
				if (likely (usbos_info->cbs->send_irb_complete)) {
					if (status != DBUS_ERR_DISCONNECT) {
						data->status = status;
						mtx_lock(&usbos_info->tx_task_q_lock);
						STAILQ_INSERT_TAIL(&usbos_info->tx_task_q, data,
							next);
						mtx_unlock(&usbos_info->tx_task_q_lock);
						taskqueue_enqueue(usbos_info->tx_tq,
							&usbos_info->tx_task);
						need_free = 0;
					}
					else {
						usbos_info->cbs->send_irb_complete(
							usbos_info->cbarg, data->txirb, status);
					}
				}
			}
		}
		if (need_free)
			bwl_tx_free(data);
	}
}

static void
dbus_usbos_send_callback(CALLBACK_ARGS)
{
	usbos_info_t *usbos_info = usbd_xfer_softc(xfer);
	struct bwl_tx_data *data;
	int actlen, sumlen, aframes, nframes, datalen, nr_frames;
	uint8 *buf;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));
	usbd_xfer_status(xfer, &actlen, &sumlen, &aframes, &nframes);

	switch (USB_GET_STATE(xfer)) {

		case USB_ST_TRANSFERRED:
			DBUSTRACE(("%s(): Transfered %d frames with actlen = %d sumlen = %d\n",
				__FUNCTION__, aframes, actlen, sumlen));
			for (nr_frames = 0; nr_frames != aframes; nr_frames++) {
				FETCH_LIST_HEAD_ITEM(tx_q_lock, tx_q);
				if (!data) {
					DBUSERR(("got xfer frame but the tx_q till end ?? \n"));
					ASSERT(0);
					/* make coverity happy */
					return;
				}
				usbd_xfer_frame_data(xfer, nr_frames, (void**)&buf, &datalen);
				if ((data->txirb->buf != buf) || (data->txirb->len != datalen)) {
					DBUSERR(("the buff or data length not match ?? \n"));
					ASSERT(0);
				}

				dbus_usbos_send_complete(data, DBUS_OK);
				usbos_info->tx_posted--;
			}
			if (usbos_info->tx_posted) {
				/* check if remained buffer will be used if not post again */
				DBUSERR(("%d posted TX buffer remained\n",
					usbos_info->tx_posted));
			}
			/* no break, FALLTHROUGH */

		case USB_ST_SETUP:
SET_UP_XFER:
			nr_frames = 0;
			datalen = 0;
			usbos_info->tx_posted = 0;
			MTX_LOCK(&usbos_info->tx_q_lock);
			STAILQ_FOREACH(data, &usbos_info->tx_q, next) {
				datalen += data->txirb->len;
				if (datalen >= BCM_USB_MAX_BURST_BYTES) {
					STAILQ_INSERT_HEAD(&usbos_info->tx_q, data, next);
					break;
				}
				usbd_xfer_set_frame_data(xfer, nr_frames,
					data->txirb->buf, data->txirb->len);
				nr_frames++;
				usbos_info->tx_posted++;
				if (usbos_info->tx_posted >= BCMWL_HW_OUT_PENDING_FRAMES) {
					/* break out from STAILQ_FOREACH */
					break;
				}
			}
			MTX_UNLOCK(&usbos_info->tx_q_lock);

			if (usbos_info->tx_posted) {
				usbd_xfer_set_frames(xfer, usbos_info->tx_posted);
				usbd_transfer_submit(xfer);
			} else {
				DBUSTRACE(("%s(): end of tx_q \n", __FUNCTION__));
			}

			break;

		default:
			DBUSERR(("%s(): error = %s with %d bytes transfered\n",
				__FUNCTION__, usbd_errstr(error), actlen));

			/*
			if (error == USB_ERR_STALLED)
			{
				DBUSERR(("%s(): calling DBUS_STATE_DOWN for %s\n",
					__FUNCTION__, usbd_errstr(error)));
				dbus_usbos_state_change(usbos_info, DBUS_STATE_DOWN);
			}
			*/

			if ((error != USB_ERR_CANCELLED) && (error != USB_ERR_STALLED)) {
				usbd_xfer_set_stall(xfer);
				goto SET_UP_XFER;
			} else {
				/* return all txirb in the queue */
				MTX_UNLOCK(&usbos_info->sc_mtx);
				MTX_LOCK(&usbos_info->tx_q_lock);
				while ((data = STAILQ_FIRST(&usbos_info->tx_q)) != NULL) {
					STAILQ_REMOVE_HEAD(&usbos_info->tx_q, next);
					dbus_usbos_send_complete(data, DBUS_ERR_TXFAIL);
				}
				STAILQ_INIT(&usbos_info->tx_q);
				MTX_UNLOCK(&usbos_info->tx_q_lock);
				MTX_LOCK(&usbos_info->sc_mtx);
			}

			break;
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));
}

static void
dbus_usbos_bulk_dl_callback(CALLBACK_ARGS)
{
	usbos_info_t *usbos_info = usbd_xfer_softc(xfer);
	int actlen, sumlen;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));
	usbd_xfer_status(xfer, &actlen, &sumlen, NULL, NULL);

	switch (USB_GET_STATE(xfer)) {
		case USB_ST_TRANSFERRED:
			usbos_info->firm_dl_success = TRUE;
			cv_signal(&usbos_info->sc_cv);
			break;

		case USB_ST_SETUP:
			usbd_transfer_submit(xfer);
			break;

		default:
			DBUSERR(("%s(): error = %s\n", __FUNCTION__, usbd_errstr(error)));
			usbos_info->firm_dl_success = FALSE;
			cv_signal(&usbos_info->sc_cv);
			break;
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));
}


#ifdef BCM_REQUEST_FW

static void kthread_openfw(void *data)
{
	usbos_info_t *usbos_info = (usbos_info_t *)data;
	struct thread *td;
	struct stat st;
	int fd = -1;
	struct uio auio;
	struct iovec aiov;

	td = curthread;

	if (kern_stat(td, usbos_info->curfw->fw_name, UIO_SYSSPACE, &st)) {
		goto FAILED;
	}

	if (kern_open(td, usbos_info->curfw->fw_name, UIO_SYSSPACE, O_RDONLY, 0)) {
		goto FAILED;
	}
	fd = td->td_retval[0];

	usbos_info->curfw->fw_size = (int)st.st_size;
	usbos_info->curfw->fw_buf = MALLOC(usbos_info->pub->osh, usbos_info->curfw->fw_size);
	if (!usbos_info->curfw->fw_buf) {
		goto FAILED;
	}

	aiov.iov_base = usbos_info->curfw->fw_buf;
	aiov.iov_len = usbos_info->curfw->fw_size;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = usbos_info->curfw->fw_size;
	auio.uio_segflg = UIO_SYSSPACE;
	if (kern_readv(td, fd, &auio)) {
		goto FAILED;
	}

	DBUSINFO(("loaded fw file %s size %d\n",
		usbos_info->curfw->fw_name, usbos_info->curfw->fw_size));

	goto EXIT;

FAILED:
	if (usbos_info->curfw->fw_buf) {
		MFREE(usbos_info->pub->osh, usbos_info->curfw->fw_buf, usbos_info->curfw->fw_size);
		usbos_info->curfw->fw_buf = NULL;
	}
EXIT:
	if (fd >= 0)
		kern_close(td, fd);

	cv_signal(&usbos_info->sc_cv);

	kthread_exit();
}

void *
dbus_get_fw_nvfile(int devid, int chiprev, uint8 **fw, int *fwlen, int type, uint16 boardtype,
	uint16 boardrev)
{
	usbos_info_t *usbos_info = (usbos_info_t *)(g_probe_info.usbos_info);
	char fw_name[256];
	char *fwdir;

	DBUSTRACE(("%s(): Enter devid = %d, type = %d \n", __FUNCTION__, devid, type));

	*fwlen = 0;
	*fw    = NULL;

	memset(fw_name, 0, sizeof(fw_name));

	/* user can use "kenv dhd_fwdir=<dir>" command specify from where to load the firmware */
	fwdir = getenv("dhd_fwdir");
	if (fwdir)
		sprintf(fw_name, "%s/bcm", fwdir);
	else
		sprintf(fw_name, "/root/bcm");

	if (type == DBUS_FIRMWARE) {
		switch (devid) {
			case BCM43236_CHIP_ID:
				strcat(fw_name, "43236");
				break;
			case BCM43242_CHIP_ID:
				strcat(fw_name, "43242");
				break;
			case BCM43569_CHIP_ID:
				strcat(fw_name, "43569");
				switch (chiprev) {
				case 0:
					strcat(fw_name, "a0");
					break;
				case 2:
					strcat(fw_name, "a2");
					break;
				default:
					break;
				}
				break;
			default:
				DBUSERR(("%s(): ERROR.. devid %d is not supported now \n",
					__FUNCTION__, devid));
				return NULL;
		}
		strcat(fw_name, "-firmware.bin");
		usbos_info->curfw = &usbos_info->fw[0];
	}
	else if (type == DBUS_NVFILE) {
		switch (devid) {
			case BCM43236_CHIP_ID:
				strcat(fw_name, "43236");
				break;
			case BCM43242_CHIP_ID:
				strcat(fw_name, "43242");
				break;
			case BCM43569_CHIP_ID:
				strcat(fw_name, "43569");
				switch (chiprev) {
				case 0:
					strcat(fw_name, "a0");
					break;
				case 2:
					strcat(fw_name, "a2");
					break;
				default:
					break;
				}
				break;
			default:
				DBUSERR(("%s(): ERROR.. devid %d is not supported now \n",
					__FUNCTION__, devid));
				return NULL;
		}
		strcat(fw_name, ".nvm");
		usbos_info->curfw = &usbos_info->fw[1];
	}
	else {
		DBUSERR(("%s(): ERROR.. type %d Unknown \n", __FUNCTION__, type));
		return (NULL);
	}

	/* load firmware to memory */
	if (!usbos_info->curfw->fw_buf) {
		usbos_info->curfw->fw_name = fw_name;
		MTX_LOCK(&usbos_info->dllock);
		kthread_add(kthread_openfw, usbos_info, NULL, NULL, 0, 0, "kthread_openfw");
		cv_wait(&usbos_info->sc_cv, &usbos_info->dllock);
		MTX_UNLOCK(&usbos_info->dllock);
	}

	if (usbos_info->curfw->fw_buf == NULL) {
		DBUSERR(("%s(): ERROR.. firmware_get() failed \n", __FUNCTION__));
		return NULL;
	}
	else {
		*fwlen = (int)(usbos_info->curfw->fw_size);
		*fw    = (uint8 *)(usbos_info->curfw->fw_buf);
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return ((void *)usbos_info->curfw->fw_buf);
}

void
dbus_release_fw_nvfile(void *firmware)
{
	usbos_info_t *usbos_info = (usbos_info_t *)(g_probe_info.usbos_info);

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (firmware) {
		if (firmware == usbos_info->fw[0].fw_buf) {
			MFREE(usbos_info->pub->osh,
				usbos_info->fw[0].fw_buf, usbos_info->fw[0].fw_size);
			usbos_info->fw[0].fw_buf = NULL;
		} else if (firmware == usbos_info->fw[1].fw_buf) {
			MFREE(usbos_info->pub->osh,
				usbos_info->fw[1].fw_buf, usbos_info->fw[1].fw_size);
			usbos_info->fw[1].fw_buf = NULL;
		} else {
			DBUSERR(("%s(): ERROR.. firmware not correct \n", __FUNCTION__));
		}
	}
	else {
		DBUSERR(("%s(): ERROR.. NULL pointer \n", __FUNCTION__));
	}

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));
}

#endif /* #ifdef BCM_REQUEST_FW */

int
dbus_usbos_state_change(void *bus, int state)
{
	usbos_info_t *usbos_info = (usbos_info_t *)bus;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (usbos_info == NULL) {
		DBUSTRACE(("%s(): ERROR.. usbos_info = NULL. Returning \n", __FUNCTION__));
		return DBUS_ERR;
	}

	if (usbos_info->cbarg && usbos_info->cbs) {
		if (usbos_info->cbs->state_change) {
			DBUSTRACE(("%s(): Calling state_change() \n", __FUNCTION__));
			usbos_info->cbs->state_change(usbos_info->cbarg, state);
		}
	}

	usbos_info->pub->busstate = state;

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return DBUS_OK;
}

static int
dbus_usbos_detach(device_t self)
{
	usbos_info_t *usbos_info = (usbos_info_t *)(device_get_softc(self));

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	sx_xlock(&mod_lock);

	if (usbos_info == NULL) {
		sx_xunlock(&mod_lock);
		DBUSERR(("%s(): ERROR.. usbos_info = NULL. Returning \n", __FUNCTION__));
		return (ENXIO);
	}

	if ((g_probe_info.dereged == FALSE) && disconnect_cb) {
		DBUSTRACE(("%s(): Calling disconnect_cb() \n", __FUNCTION__));
		disconnect_cb(disc_arg);
		g_probe_info.disc_cb_done = TRUE;
	}

	if (bwl_device) {
		/* The usbd_transfer_drain is invoked from the disconnect_cb to intf_detach() */
		usbd_transfer_unsetup(usbos_info->sc_xfer, BWL_N_TRANSFER);

		cv_destroy(&usbos_info->sc_cv);
		MTX_DESTROY(&usbos_info->sc_mtx);
		MTX_DESTROY(&usbos_info->dllock);

		bwl_device = NULL;
	}

	sx_xunlock(&mod_lock);

	DBUSTRACE(("%s(): Finish \n", __FUNCTION__));

	return (0);
}


static int
dbus_usbos_attach(device_t self)
{
	struct usb_attach_arg *uaa;
	struct usbd_lookup_info *info;
	struct usb_config_descriptor *cdesc;
	struct usb_endpoint *endpoint;
	struct usb_endpoint_descriptor *edesc;
	struct usb_interface *iface;
	struct usb_interface_descriptor *iface_desc;
	enum usb_dev_speed speed;

	int num_of_eps;
	uint8 iface_index = 0;
	usbos_info_t *usbos_info;
	int error;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	sx_xlock(&mod_lock);

	uaa = device_get_ivars(self);
	/*
	 * The softc automatically allocated and zero private area
	 * of length size
	 */
	usbos_info  = (usbos_info_t *)(device_get_softc(self));

	ASSERT(usbos_info != NULL);

	device_set_usb_desc(self);
	usbos_info->sc_udev = uaa->device;
	usbos_info->sc_dev  = self;

	DBUSTRACE(("%s(): device_get_nameunit(self) = %s\n",
		__FUNCTION__, device_get_nameunit(self)));
	if (bwl_device) {
		sx_xunlock(&mod_lock);
		DBUSERR(("%s(): bwl_device is attached or attaching.. \n", __FUNCTION__));
		return (ENXIO);
	}
	else {
		bwl_device = self;
	}

	g_probe_info.usbos_info = (void *)usbos_info;

	info = (struct usbd_lookup_info *) &(uaa->info);

	if (info == NULL) {
		DBUSERR(("%s(): ERROR.. 'info' is NULL\n", __FUNCTION__));
		goto detach;
	}

	g_probe_info.vid = info->idVendor;
	g_probe_info.pid = info->idProduct;

	cdesc = usbos_info->sc_udev->cdesc;
	BCM_REFERENCE(cdesc);
#ifdef NOT_YET
	ddesc = (struct usb_device_descriptor *) &usbos_info->sc_udev->ddesc;
	dbus_usbos_dump_usbdevice_desc(ddesc);
	dbus_usbos_dump_lookup_info(info);
	dbus_usbos_dump_config_desc(cdesc);
#endif

	if (info->bConfigNum != 1) {
		DBUSERR(("%s(): ERROR.. info->bConfigNum= %d is not 1\n",
			__FUNCTION__, info->bConfigNum));
		goto detach;
	}
	if (info->bDeviceClass != UICLASS_VENDOR) {
		DBUSERR(("%s(): ERROR.. info->bDeviceClass = %d is not UICLASS_VENDOR\n",
			__FUNCTION__, info->bDeviceClass));
		goto detach;
	}

	/* Only the BDC interface configuration is supported:
	 *   Device class: USB_CLASS_VENDOR_SPEC
	 *   if0 class: USB_CLASS_VENDOR_SPEC
	 *   if0/ep0: control
	 *   if0/ep1: bulk in
	 *   if0/ep2: bulk out (ok if swapped with bulk in)
	 */

	/* Check the control interface */
	if (info->bInterfaceClass != UICLASS_VENDOR ||
	    info->bInterfaceSubClass != 2 ||
	    info->bInterfaceProtocol != 0xff) {
		DBUSERR(("%s(): ERROR.. Invalid Ctrl Intf: class %d, subclass %d, proto %d \n",
			__FUNCTION__, info->bInterfaceClass,
			info->bInterfaceSubClass, info->bInterfaceProtocol));
		goto detach;
	}

	/* Check control endpoint */
	/* endpoint = usbd_get_endpoint(usbos_info->sc_udev, UE_CONTROL, bwl_config);  */
	/* endpoint = usbd_get_ep_by_addr(usbos_info->sc_udev, 0x0081); */
	endpoint = usb_endpoint_foreach(usbos_info->sc_udev, NULL);
	if (endpoint == NULL) {
		DBUSERR(("%s(): ERROR.. usb_endpoint is NULL \n", __FUNCTION__));
		goto detach;
	}

	edesc = endpoint->edesc;
	if (edesc == NULL) {
		DBUSERR(("%s(): ERROR.. usb_endpoint_descriptor is NULL \n", __FUNCTION__));
		goto detach;
	}
	if ((edesc->bmAttributes & UE_TYPE_ANY) != UE_INTERRUPT) {
		DBUSERR(("%s(): ERROR.. Invalid Control endpoint: edesc->bmAttributes = %d \n",
			__FUNCTION__, edesc->bmAttributes));
		goto detach;
	}

	/* dbus_usbos_dump_edesc(edesc); */

	iface = usbd_get_iface(usbos_info->sc_udev, 0);
	if (iface == NULL) {
		DBUSERR(("%s(): ERROR.. The BULK usb_interface is NULL \n", __FUNCTION__));
		goto detach;
	}

	iface_desc = usbd_get_interface_descriptor(iface);
	if (iface_desc == NULL) {
		DBUSERR(("%s(): ERROR.. The BULK Intf desc is NULL \n", __FUNCTION__));
		goto detach;
	}

	num_of_eps = iface_desc->bNumEndpoints;

	/* The FreeBSD counts including the control endpoint */
	if ((num_of_eps != 2) && (num_of_eps != 3) && (num_of_eps != 4)) {
		DBUSERR(("%s(): ERROR.. num_of_eps = %d \n", __FUNCTION__, num_of_eps));
	}

	/* Check other endpints */
	while ((endpoint = usb_endpoint_foreach(usbos_info->sc_udev, endpoint))) {
		if (endpoint != NULL) {
			if ((endpoint->edesc->bmAttributes & UE_TYPE_ANY) != UE_BULK) {
				DBUSERR(("%s(): ERROR.. Invalid Data Ep: bmAttributes = %d \n",
					__FUNCTION__, endpoint->edesc->bmAttributes));
				/* goto detach; */
			}
		}
	}

	speed = usbd_get_speed(usbos_info->sc_udev);
	if (speed == USB_SPEED_HIGH) {
		g_probe_info.device_speed = HIGH_SPEED;
		DBUSINFO(("%s(): High speed device detected \n", __FUNCTION__));
	} else {
		g_probe_info.device_speed = FULL_SPEED;
		DBUSINFO(("%s(): Full speed device detected \n", __FUNCTION__));
	}

	/* Be careful: The init sequence must be inversed in the dbus_usbos_attach() */
	MTX_INIT(&usbos_info->dllock, "dllock", NULL, MTX_DEF);
	MTX_INIT(&usbos_info->sc_mtx, device_get_nameunit(self), NULL, MTX_DEF);
	cv_init(&usbos_info->sc_cv, DBUS_USB_FBSD_DRIVER_NAME);

	error = usbd_transfer_setup(uaa->device, &iface_index,
		usbos_info->sc_xfer, bwl_config, BWL_N_TRANSFER, usbos_info, &usbos_info->sc_mtx);
	if (error) {
		DBUSERR(("%s(): ERROR.. usbd_transfer_setup() returned error = %d \n",
			__FUNCTION__, error));
	}

	if (probe_cb) {
		DBUSTRACE(("%s(): Calling probe_cb() \n", __FUNCTION__));
		if (strncmp(uaa->device->product, USB_STR_DESC_REMOTE_DOWNLOAD,
			strlen(USB_STR_DESC_REMOTE_DOWNLOAD)) != 0)
				disc_arg = probe_cb(probe_arg, "", USB_BUS, 0);
	}

	g_probe_info.disc_cb_done = FALSE;

	sx_xunlock(&mod_lock);

	DBUSTRACE(("%s(): Returns SUCCESS \n", __FUNCTION__));
	return 0;

detach:

	bwl_device = NULL;

	sx_xunlock(&mod_lock);

	DBUSERR(("%s(): Returns FAILED \n", __FUNCTION__));

	return (ENXIO);
}


static int
dbus_usbos_probe(device_t self)
{
	struct usb_attach_arg *uaa = device_get_ivars(self);
	int ret;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	if (uaa->usb_mode != USB_MODE_HOST) {
		DBUSERR(("%s(): Error: usb_mode != USB_MODE_HOST \n", __FUNCTION__));
		return ENXIO;
	}
	if (uaa->info.bConfigIndex != 0) {
		DBUSERR(("%s(): Error: info.bConfigIndex != 0 \n", __FUNCTION__));
		return ENXIO;
	}
	if (uaa->info.bIfaceIndex != 0) {
		DBUSERR(("%s(): Error: info.bIfaceIndex != 0 \n", __FUNCTION__));
		return ENXIO;
	}

	DBUSTRACE(("%s(): idVendor = 0x%x, idProduct = 0x%x,\n",
		__FUNCTION__, uaa->info.idVendor, uaa->info.idProduct));
	ret = usbd_lookup_id_by_uaa(devid_table, sizeof(devid_table), uaa);
	if (ret == 0) {
		DBUSTRACE(("%s(): Returns Match found... id = 0x%x \n", __FUNCTION__, ret));
	}
	else {
		DBUSTRACE(("%s(): Returns Match not found... id = 0x%x \n", __FUNCTION__, ret));
	}

	return (ret);

}

int dbus_usbos_loopback_tx(void *usbos_info_ptr, int cnt, int size)
{
	DBUSERR(("%s: Not Implemented\n", __FUNCTION__));
	return BCME_ERROR;
}

int
dbus_bus_osl_register(int vid, int pid, probe_cb_t prcb,
	disconnect_cb_t discb, void *prarg, dbus_intf_t **intf, void *param1, void *param2)
{
	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	sx_init(&mod_lock, "mod_lock");

	sx_xlock(&mod_lock);

	bzero(&g_probe_info, sizeof(probe_info_t));
	g_probe_info.disc_cb_done = TRUE;

	probe_cb = prcb;
	disconnect_cb = discb;
	probe_arg = prarg;

	devid_table[0].idVendor = vid;
	devid_table[0].idProduct = pid;

	*intf = &dbus_usbos_intf;

	sx_xunlock(&mod_lock);

	DBUSTRACE(("%s(): Returns DBUS_OK \n", __FUNCTION__));
	return DBUS_OK;
}

int
dbus_bus_osl_deregister()
{
	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	sx_destroy(&mod_lock);

	DBUSTRACE(("%s(): Returns DBUS_OK \n", __FUNCTION__));
	return DBUS_OK;
}


static device_method_t dbus_usb_fbsd_methods[] = {
	DEVMETHOD(device_probe, dbus_usbos_probe),
	DEVMETHOD(device_attach, dbus_usbos_attach),
	DEVMETHOD(device_detach, dbus_usbos_detach),
	{ 0, 0 }
};

static driver_t dbus_usb_fbsd_driver = {
	.name = DBUS_USB_FBSD_DRIVER_NAME,
	.methods = dbus_usb_fbsd_methods,
	.size = sizeof(usbos_info_t),
};

static devclass_t dbus_usb_fbsd_devclass;

extern void dhd_module_cleanup(void);
extern void dhd_module_init(void);

static int dbus_usb_fbsd_modevent(module_t module, int event, void *arg)
{
	int error = 0;

	DBUSTRACE(("%s(): Enter \n", __FUNCTION__));

	switch (event) {
		case MOD_LOAD:
			DBUSTRACE(("%s(): Event MOD_LOAD: in module 'dhd' \n", __FUNCTION__));
			dhd_module_init();
			break;
		case MOD_UNLOAD:
			DBUSTRACE(("%s(): Event MOD_UNLOAD: in module 'dhd' \n", __FUNCTION__));
			dhd_module_cleanup();
			break;
		case MOD_SHUTDOWN:
			DBUSTRACE(("%s(): Event MOD_SHUTDOWN: in module 'dhd' \n", __FUNCTION__));
			dhd_module_cleanup();
			break;
		case MOD_QUIESCE:
			DBUSTRACE(("%s(): Event MOD_QUIESCE: in module 'dhd'. Unhandled. \n",
				__FUNCTION__));
			break;
		default:
			DBUSTRACE(("%s(): Unsupported event 0x%x in module 'dhd'. Unhandled. \n",
				__FUNCTION__, event));
			error = EINVAL;
			break;
	}

	DBUSTRACE(("%s(): finished \n", __FUNCTION__));

	return error;
}

DRIVER_MODULE(DBUS_USB_FBSD_MODULE_NAME, uhub, dbus_usb_fbsd_driver,
	dbus_usb_fbsd_devclass, dbus_usb_fbsd_modevent, 0);
MODULE_VERSION(DBUS_USB_FBSD_MODULE_NAME, 1);
MODULE_DEPEND(DBUS_USB_FBSD_MODULE_NAME, usb, 1, 1, 1);
