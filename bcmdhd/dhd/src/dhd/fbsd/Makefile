#
# Broadcom Dongle Host Driver Makefile
#
# Copyright (C) 2011, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: Makefile 315039 2012-02-15 06:31:08Z $

###################################################################################
# The Environment variables needed
#
#     - FREEBSD_DIR    --> Directory where FreeBSD source is located
#     - FREEBSD_VER    --> FreeBSD version string
#     - CROSS_COMPILE  --> Path to cross compiler tool chain
#     - SRCBASE        --> Base of WLAN driver source code
#
#     - FIRMWARE_PATH       --> Path to the firmware file
#     - FIRMWARE_FILE_NAME  --> Name of firmware file
#     - NVRAM_PATH          --> Path to the nvram file
#     - NVRAM_FILE_NAME     --> Name of nvram file
#     - FIRMWARE_CHIP_ID    --> Chipd id of the device
#
###################################################################################

ObjPfx := dhd-

ifeq ($(FREEBSD_VER),)
	FREEBSD_VER=$(shell uname -r)
endif
ifeq ($(FREEBSD_DIR),)
	FREEBSD_DIR=/usr/src
endif
ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE=/usr/bin/
endif
ifeq ($(BSDMAKE),)
	BSDMAKE=/usr/bin/make
endif

# driver source base and C file path
ifeq ($(SRCBASE),)
	SRCBASE = $(shell /bin/pwd)/../..
endif

include $(SRCBASE)/makefiles/WLAN_Common.mk

# Allow CROSS_COMPILE to specify compiler base
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
NM := $(CROSS_COMPILE)nm
OBJCOPY := $(CROSS_COMPILE)objcopy
GMAKE := gmake
BSD.MK = $(OBJDIR)/Makefile
HOSTCC := $(CC)

#Default DHDARCH is i386
SYSDIR := $(FREEBSD_DIR)/sys
DHDARCH := i386

vpath %.c $(addprefix $(WLAN_SrcBaseR)/,dhd/sys shared bcmsdio/sys wl/sys wl/phy) $(WLAN_StdSrcDirsR) $(WLAN_ComponentSrcDirsR)

## Initialize DFLAGS, CFLAGS, IFLAGS
ifneq ($(findstring reqfw,-$(TARGET)-),)
DFLAGS := -DBCM_REQUEST_FW -DBCMTRXV2
else
DFLAGS :=
endif
CFILES :=
IFLAGS :=

#DFLAGS += -DBCMDBG

DFLAGS += -DINF_QUEUE_LIST_COUNT=512
DFLAGS += -DBWL_TX_LIST_COUNT=256
DFLAGS += -DBWL_TX_BUFFER_SIZE=2048
DFLAGS += -DDBUS_RX_BUFFER_SIZE_RPC=65536
#DFLAGS += -DBCM_RPC_RXNOCOPY

ifneq ($(findstring -cust_hw_30_1-,$(TARGET)-),)
	DFLAGS += -DCUSTOMER_HW_30_1
else
	WFLAGS := -Wall -Werror
endif
ifneq ($(findstring -cust_hw_30_2-,$(TARGET)-),)
	DFLAGS += -DCUSTOMER_HW_30_2
endif

ifneq ($(findstring -fdaggr-,-$(TARGET)-),)
DFLAGS += -DBCM_FD_AGGR
DFLAGS += -DBWL_RX_LIST_COUNT=8
DFLAGS += -DBWL_RX_BUFFER_SIZE=DBUS_RX_BUFFER_SIZE_RPC
else
DFLAGS += -DBWL_RX_LIST_COUNT=128
DFLAGS += -DBWL_RX_BUFFER_SIZE=2048
endif

# basic options (defines in DFLAGS, includes in IFLAGS)
DFLAGS += -DFREEBSD -DSRCBASE=\"$(WLAN_SrcBaseA)\" -DBCMDRIVER -DBCMDONGLEHOST  # dummy end of line \\ "
DFLAGS += -DUNRELEASEDCHIP -DBCMDMA32
DFLAGS += -DBCMFILEIMAGE
# DFALGS += -DBCMDBG_ASSERT
ifeq ($(BCMQT),1)
	DFLAGS += -DBCMSLTGT -DBCMQT
endif
ifeq ($(WLTEST),1)
	DFLAGS += -DWLTEST -DIOCTL_RESP_TIMEOUT=20000
	DFLAGS += -DDHD_SPROM
endif

# First include from FreeBSD kernel dirs
ifeq ($(findstring 9.0, $(FREEBSD_VER))$(findstring 9.1, $(FREEBSD_VER)),)
IFLAGS += -I$(SYSDIR)/$(DHDARCH)/compile/GENERIC
endif
IFLAGS += -I$(FREEBSD_DIR)/include
IFLAGS += -I$(FREEBSD_DIR)/sys

# Kernel compiled headers usually at:
# /usr/obj/usr/src/sys/KERNEL-LABEL/modules/usr/src/sys/modules/ksyms/
# /usr/obj/usr/src/sys/KERNEL-LABEL/
IFLAGS += -I$(COMPILE_DIR)

# From current workspace
IFLAGS += -I.
IFLAGS += -I$(WLAN_SrcBaseA)/include
IFLAGS += -I$(WLAN_SrcBaseA)/../components/shared/
IFLAGS += -I$(WLAN_SrcBaseA)/shared
IFLAGS += -I$(WLAN_SrcBaseA)/dhd/sys
IFLAGS += -I$(WLAN_SrcBaseA)/dongle
IFLAGS += -I$(WLAN_SrcBaseA)/wl/sys
IFLAGS += -I$(WLAN_SrcBaseA)/wl/phy
IFLAGS += $(WLAN_ComponentIncPath)

# WFLAGS := -Wall -Wstrict-prototypes -Werror
# WFLAGS := -Wall -Wstrict-prototypes
WFLAGS := -Wall -Werror

CFILES += dhd_fbsd.c fbsd_osl.c bcmutils.c hnd_pktq.c hnd_pktpool.c bcmevent.c dhd_common.c dhd_ip.c dhd_debug.c
# CFILES += dhd_custom_gpio.c

# ifeq ($(findstring -usb-,-$(TARGET)-),)
# 	CFILES += siutils.c sbutils.c aiutils.c hndpmu.c
# 	#ifdef BCMINTERNAL
# 	#	CFILES += bcmsrom.c bcmotp.c
# 	#else
# 	ifeq ($(WLTEST),1)
# 		CFILES += bcmsrom.c bcmotp.c
# 	endif
# 	#endif
# endif

# Building hsic for 43239, 43236, 4330 and 4334
ifneq ($(findstring -hsic-,-$(TARGET)-),)
	DFLAGS += -DEXTENDED_VID_PID=' { USB_DEVICE(0x0a5c, 0xbd1b) }, \
					{ USB_DEVICE(0x0a5c, 0xbd17) }, \
					{ USB_DEVICE(0x0a5c, 0xbd19) }, \
					{ USB_DEVICE(0x0a5c, 0xbd1c) }, \
					{ USB_DEVICE(0x0a5c, 0xbd1a) }'
	#Enable interrupt ep support for HSIC driver
	DFLAGS += -DINTR_EP_ENABLE
endif  # dummy end of line \\ "

ifneq ($(findstring -usb-,-$(TARGET)-),)
	# FullDongle Usb aggregation
	ifneq ($(findstring -fdaggr-,-$(TARGET)-),)
		CFILES += bcm_rpc_tp_dbus.c fbsd_rpc_osl.c
		DFLAGS += -DBCM_FD_AGGR
	endif
	# enable queuing in host
	ifeq ($(findstring -noproptxstatus-,-$(TARGET)-),)
		DFLAGS += -DPROP_TXSTATUS
		#DFLAGS += -DDHD_WLFC_THREAD
		ifneq ($(findstring -dbgwlfc-,-$(TARGET)-),)
		DFLAGS += -DPROP_TXSTATUS_DEBUG
		endif
	endif
endif

OFILES=$(CFILES:.c=.o)

# Make debug a separate option
ifneq ($(findstring -debug-,-$(TARGET)-),)
	DFLAGS += -DDHD_DEBUG -DSDTEST
endif

# enable queuing in host
ifneq ($(findstring -sdstd-,-$(TARGET)-),)
	ifeq ($(findstring -noproptxstatus-,-$(TARGET)-),)
		DFLAGS += -DPROP_TXSTATUS
	endif
	ifneq ($(findstring -dbgwlfc-,-$(TARGET)-),)
		DFLAGS += -DPROP_TXSTATUS_DEBUG
	endif
endif

# Make big-endian a separate option
ifneq ($(findstring -be-,-$(TARGET)-),)
	DFLAGS += -DIL_BIGENDIAN
endif

ifneq ($(findstring -dnglimage-,-$(TARGET)-),)
	## Embeddable dongle image name
	DNGL_IMAGE_NAME ?= 43236b-roml/ag-nodis-p2p-apsta-idsup-af-idauth
	DFLAGS += -DBCM_DNGL_EMBEDIMAGE -DEMBED_IMAGE_GENERIC
	IFLAGS += -I$(WLAN_SrcBaseA)/dongle/rte/wl/builds/$(DNGL_IMAGE_NAME)
	IFLAGS += -I$(WLAN_SrcBaseA)/shared/zlib
endif

ifneq ($(findstring -cdc-,-$(TARGET)-),)
	#DFLAGS += -DTOE
	DFLAGS += -DBDC
	DFLAGS += -DDHD_BCMEVENTS -DSHOW_EVENTS
	CFILES += dhd_cdc.c dhd_wlfc.c
endif

ifneq ($(findstring -dbususb-,-$(TARGET)-),)
	DFLAGS += -DBCMDHDUSB -DBCMDBUS
	CFILES += dbus.c dbus_usb.c dbus_usb_fbsd.c
endif
ifneq ($(findstring -dnglimage-,-$(TARGET)-),)
	DFLAGS += -DBCM_DNGL_EMBEDIMAGE
endif
ifneq ($(findstring -usb-,-$(TARGET)-),)
	DFLAGS += -DBCMDHDUSB -DBCMDBUS
	CFILES += dbus.c dbus_usb.c dbus_usb_fbsd.c
endif
ifneq ($(findstring -dbus-,$(TARGET)-),)
	DFLAGS += -DBCMDBUS -DBCMSDIO
	CFILES += dbus.c dbus_sdh.c dbus_sdh_fbsd.c
endif
ifneq ($(findstring -dbussdstd-,$(TARGET)-),)
	DFLAGS += -DBCMSDIO -DBCMSDIOH_STD -DBCMSDIOH_TXGLOM
	CFILES += bcmsdh.c bcmsdstd.c bcmsdstd_linux.c bcmsdh_linux.c
	DFLAGS += -DBCMDBUS
	CFILES += dbus.c dbus_sdh.c dbus_sdh_linux.c
endif
ifneq ($(findstring -sdio-,-$(TARGET)-),)
	DFLAGS += -DBCMSDIO
	CFILES += dhd_sdio.c
endif
ifneq ($(findstring -sdbcm-,$(TARGET)-),)
	DFLAGS += -DBCMSDIO -DBCMSDIOH_BCM -DTESTDONGLE
	CFILES += dhd_sdio.c bcmsdh.c bcmsdbrcm.c bcmsdbrcm_linux.c bcmsdh_linux.c hnddma.c
endif
ifneq ($(findstring -sdstd-,$(TARGET)-),)
	DFLAGS += -DBCMSDIO -DBCMSDIOH_STD
	CFILES += dhd_sdio.c bcmsdh.c bcmsdstd.c bcmsdstd_linux.c bcmsdh_linux.c
	ifneq ($(findstring -hc3-,$(TARGET)-),)
		DFLAGS += -DSDHOST3=1
	endif
endif

ifneq ($(findstring -sdmmc-,-$(TARGET)-),)
	ifeq ($(findstring -noproptxstatus-,-$(TARGET)-),)
		DFLAGS += -DPROP_TXSTATUS
	endif
	DFLAGS += -DBCMSDIO -DBCMLXSDMMC
	CFILES += dhd_sdio.c bcmsdh_sdmmc.c bcmsdh.c bcmsdh_linux.c bcmsdh_sdmmc_linux.c
endif

#DFLAGS += -DARP_OFFLOAD_SUPPORT

ifneq ($(findstring -net80211-,-$(TARGET)-),)
	DFLAGS += -DDHD_NET80211
	CFILES += dhd_net80211.c bcmwifi_channels.c bcmwpa.c
endif

ifneq ($(findstring -mvif-,-$(TARGET)-),)
	DFLAGS += -DFBSD_MUL_VIF
endif

ifneq ($(findstring -p2pif-,-$(TARGET)-),)
	DFLAGS += -DFBSD_MUL_VIF -DFBSD_ENABLE_P2P_DEV_IF
endif

ifneq ($(findstring -oob-,-$(TARGET)-),)
	DFLAGS += -DOOB_INTR_ONLY
	DFLAGS += -DMMC_SDIO_ABORT
	ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6.35),TRUE)
		DFLAGS += -DHW_OOB
	endif
else
	ifneq ($(findstring -sdmmc-,-$(TARGET)-),)
		DFLAGS += -DSDIO_ISR_THREAD
	endif
endif

# *** PLEASE try not to overwirte the common -sdmmc- settings in your build.
# if all builds (nexus, etc) have its special settings, we will end up having may
# duplicate flags/files in DFLAGS, CFILES

ifneq ($(findstring -nexus-,-$(TARGET)-),)
	DFLAGS += -DDHD_USE_STATIC_BUF -DCUSTOM_OOB_GPIO_NUM=299
endif

ifneq ($(findstring -sdspi-,$(TARGET)-),)
	DFLAGS += -DBCMSDIO -DBCMSDIOH_SPI -DTESTDONGLE # -DBCMSDYIELD
	CFILES += dhd_sdio.c bcmsdh.c bcmsdspi.c bcmsdspi_linux.c bcmsdh_linux.c
endif
ifneq ($(findstring -gspi-,$(TARGET)-),)
	DFLAGS += -DBCMSDIO -DBCMSDIOH_SPI -DTESTDONGLE -DBCMSPI
	CFILES += dhd_sdio.c bcmsdh.c bcmspibrcm.c bcmsdspi_linux.c bcmsdh_linux.c
endif
ifneq ($(findstring -gspibcm,$(TARGET)-),)
	DFLAGS += -DBCMSDIO -DBCMSDIOH_SPI -DTESTDONGLE -DBCMSPI
	CFILES += dhd_sdio.c bcmsdh.c bcmgspi.c bcmsdbrcm_linux.c bcmsdh_linux.c hnddma.c
endif
ifneq ($(findstring -pci,$(TARGET)-),)
	CFILES += bcmpcispi.c
endif
ifneq ($(findstring -sdext-,$(TARGET)-),)
	DFLAGS += -DBCMSDIO -DTESTDONGLE
	CFILES += dhd_sdio.c
endif

#if 0
ifneq ($(findstring -btamp-,$(TARGET)-),)
	DFLAGS += -DWLBTAMP
	CFILES += dhd_bta.c
endif

#ifdef WIFI_ACT_FRAME
	DFLAGS += -DWIFI_ACT_FRAME
#endif

ifneq ($(findstring -reqfw-,$(TARGET)-),)
	DFLAGS += -DBCM_REQUEST_FW
endif

CFLAGS += -fshort-wchar $(DFLAGS) $(WFLAGS) $(IFLAGS) $(MY_C_DEFINES)

LDFLAGS := -r
MODULES := dhd.o

# host options
# HOSTCFLAGS := $(CFLAGS) $(shell $(MAKE) --no-print-directory -s -C $(LINUXDIR) script 'SCRIPT=@echo $$(CFLAGS) $$(MODFLAGS)')
HOSTCFLAGS := $(CFLAGS)

# Please do NOT explicitly specify any -debug build here, the Makefile can handles that postfix.
# That is, -debug can be added to any of the following builds#
# Description of Options used
#    -reqfw-  : To enable Request Firmware feature
#    -be-     : Enable this for Big Endian targets
TARGETS :=
TARGETS += cdc-usb
TARGETS += cdc-usb-p2pif
TARGETS += cdc-usb-fdaggr
TARGETS += cdc-usb-fdaggr-p2pif
TARGETS += cdc-usb-reqfw
TARGETS += cdc-usb-net80211
TARGETS += cdc-usb-net80211-reqfw

TARGETS := $(addprefix $(ObjPfx),$(TARGETS))
TARGETS += $(foreach tgt, $(TARGETS), $(tgt)-debug $(tgt)-be $(tgt)-noproptxstatus $(tgt)-be-noproptxstatus)

ifeq ($(BUILD_TAG),TRUNK)
	OBJDIR=host/$(TARGET_ARCH)/$(TARGET_OS)/$(TARGET)-FreeBSD-$(FREEBSD_VER)$(if $(BCMQT),-bcmqt)
else
	OBJDIR=$(TARGET)-FreeBSD-$(FREEBSD_VER)$(if $(BCMQT),-bcmqt)
endif

all: $(filter %-sdio %-sdbcm %-sdstd %-usb %sdspi-pci %-sdmmc, $(TARGETS))
sdio: $(filter %-sdio %-sdbcm %-sdstd, $(TARGETS))
usb: $(filter %-usb, $(TARGETS))
sdspi: $(filter %-sdspi-pci, $(TARGETS))


# Allow making target with the FREEBSD_VER suffix already on it.
# (Typical of command line tab completion; trailing slash still not allowed)
%-$(FREEBSD_VER): force
	$(GMAKE) $(@:%-$(FREEBSD_VER)=%)

$(TARGETS):
	@echo "MAKING $@"
	$(GMAKE) TARGET=$@ objdir

# Show compiler version, for the current target build
showenv:
	@echo "CC = $(CC) (ver=`$(CC) -dumpversion`; host=`hostname`; processor=`uname -m`)"

objdir: showenv
	@echo "Making objdir $(OBJDIR)"
	@echo "TARGET is $(TARGET)"
	mkdir -p $(OBJDIR)
	# $(GMAKE) -C $(OBJDIR) -f $(WLAN_Makefile) modules
	$(GMAKE) -C $(OBJDIR) -f $(SRCBASE)/dhd/fbsd/Makefile SRCBASE=$(SRCBASE) modules

dep: $(foreach file,$(CFILES),.$(file).depend)
.%.c.depend: %.c
	$(HOSTCC) $(HOSTCFLAGS) -M $< > $@ || (rm -f $@; exit 1)
.%.c.depend::
	touch $@

$(BSD.MK):
	@echo "KMOD = dhd" > Makefile
	@echo >> Makefile
	@echo "EXPORT_SYMS=YES #[linux version xargs don't recognize -J opton]" >> Makefile
	@echo >> Makefile
	@echo "MK_ALIGN_OPTION=no-align #[gcc don't know -mno-align-long-strings ]" >> Makefile
	@echo >> Makefile
	@echo "MACHINE=$(DHDARCH)" >> Makefile
	@echo >> Makefile
	@echo "SYSDIR=$(SYSDIR)" >> Makefile
	@echo >> Makefile
ifeq ($(DHDARCH), i386)
	@echo "CC=$(CC)" >> Makefile
	@echo >> Makefile
	@echo "LD=$(LD)" >> Makefile
	@echo >> Makefile
	@echo "NM=$(NM)" >> Makefile
	@echo >> Makefile
	@echo "OBJCOPY=$(OBJCOPY)" >> Makefile
	@echo >> Makefile
endif
	@echo "CFLAGS +=$(IFLAGS)" >> Makefile
	@echo >> Makefile
	@echo "CFLAGS +=$(DFLAGS)" >> Makefile
	@echo >> Makefile
	@echo "CFLAGS +=$(WFLAGS)" >> Makefile
	@echo >> Makefile
	@echo "CWARNFLAGS := -std=gnu99 -Wno-pointer-sign" >> Makefile
	@echo >> Makefile
ifneq ($(shell $(CC) -v 2>&1 | grep "4.7.2"),)
	@echo "CWARNFLAGS += -Wno-unused-but-set-variable #[src/sys/sys/buf_ring.h triggled thiswarning; current cross-compiler v4.7.2 will prompt warring for it and cause compilign fail]" >> Makefile
	@echo >> Makefile
endif
	@echo "SRCS =$(CFILES)" >> Makefile
	@echo "SRCS += opt_usb.h device_if.h bus_if.h pci_if.h" >> Makefile
	@echo >> Makefile
	@echo ".include <bsd.kmod.mk>" >> Makefile

modules: $(OFILES) $(BSD.MK)
	cd $(SRCBASE)/dhd/fbsd/$(OBJDIR)
	MAKEFLAGS="CSTD=gnu99" $(BSDMAKE)

$(MODULES): $(OFILES)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	test -r ./$(<F) || ln -s $< .

force:

clean:
ifeq ($(BUILD_TAG),TRUNK)
	$(RM) -r host
else
	$(RM) -r $(ObjPfx)*
endif

-include $(wildcard .*.depend)
-include $(wildcard .*.flags)
