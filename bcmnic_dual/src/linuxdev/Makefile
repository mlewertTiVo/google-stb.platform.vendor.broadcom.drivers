#
# GNUmakefile for Broadcom Linux USB shim layer driver
#
# Copyright (C) 2016, Broadcom Corporation. All Rights Reserved.
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# $Id: Makefile 435909 2013-11-12 17:42:12Z $
#

ifeq ($(SRCBASE),)
	# Use current directory/../.. as SRCBASE if not defined.  
	# Remove potential trailing / in case anybody cuts and pastes ...
	SRCBASE := $(patsubst %/,%,$(dir $(patsubst %/,%,$(dir $(patsubst %/,%,$(shell /bin/pwd))))))/src
endif # SRCBASE

TARGETARCH  ?= x86

ifeq ($(LINUXVER),)
	LINUXVER := $(shell uname -r)
endif

ifeq ($(LINUXDIR),)
	ifneq ($(wildcard /lib/modules/$(LINUXVER)/build/include/linux/version.h),)
		# If native host matches LINUXVER, then use local LINUXDIR
		LINUXDIR := /lib/modules/$(LINUXVER)/build
	else ifneq ($(wildcard /lib/modules/$(LINUXVER)/build/include/generated/uapi/linux/version.h),)
		# Linux 3.7+ has moved version.h to include/generated/uapi/linux.
		LINUXDIR := /lib/modules/$(LINUXVER)/build
	else ifneq ($(wildcard /tools/linux/src/linux-$(LINUXVER)/include/linux/version.h),)
		LINUXDIR := /tools/linux/src/linux-$(LINUXVER)
	else ifneq ($(wildcard /tools/linux/src/linux-$(LINUXVER)/include/generated/uapi/linux/version.h),)
		# Linux 3.7+ has moved version.h to include/generated/uapi/linux.
		LINUXDIR := /tools/linux/src/linux-$(LINUXVER)
	else
		LINUXDIR := /usr/src/linux
		$(warning LINUXDIR is set to native $(LINUXDIR))
	endif
endif # LINUXDIR

# The module name (which is also the file name)
BCM_EXTERNAL_MODULE_NAME ?= bcm_dbus

# Object files needed to build this module
OBJ_FILES = dbus.o dbus_usb.o dbus_usb_linux.o bcmutils.o linux_osl.o nvramstubs.o

# The compilation flags
IFLAGS =
DFLAGS =
WFLAGS =

IFLAGS += -I$(LINUXDIR)/include
IFLAGS += -I$(LINUXDIR)/include/uapi
IFLAGS += -I$(LINUXDIR)/include/generated/uapi
IFLAGS += -I$(LINUXDIR)/arch/$(TARGETARCH)/include
IFLAGS += -I$(LINUXDIR)/arch/$(TARGETARCH)/include/uapi
IFLAGS += -I$(LINUXDIR)/arch/$(TARGETARCH)/include/generated/uapi
IFLAGS += -I$(LINUXDIR)/arch/$(TARGETARCH)/mach-bcm2708/include
IFLAGS += -I$(LINUXDIR)/arch/$(TARGETARCH)/asm/mach-default
IFLAGS += -I$(LINUXDIR)/include/asm-$(TARGETARCH)/mach-default
IFLAGS += -I$(SRCBASE)/include
IFLAGS += -I$(SRCBASE)/common/include
IFLAGS += -I$(SRCBASE)/shared/bcmwifi/include

DFLAGS_IN ?= -D__KERNEL__ -DMODULE -DLINUX -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 \
             -DBCMDRIVER -DBCMTRXV2 -DBCM_RPC_TOC -DUSBOS_THREAD -DWL_URB_ZPKT -DWL_USB_ZLP_PAD \
             -DWLC_HIGH -DUSB_DISABLE_INT_EP -DWL_THREADNICE=-20 -DLINUX_EXTERNAL_MODULE_DBUS
WFLAGS_IN ?= 
IFLAGS_IN ?= 

DFLAGS += $(DFLAGS_IN)
WFLAGS += $(WFLAGS_IN)
IFLAGS += $(IFLAGS_IN)

EXTRA_CFLAGS += $(WFLAGS) $(DFLAGS) $(IFLAGS)

ifeq ($(KBUILD_EXTMOD),)

OBJDIR = obj-$(BCM_EXTERNAL_MODULE_NAME)-$(LINUXVER)-$(TARGETARCH)

CC     = $(CROSS_COMPILE)gcc
LD     = $(CROSS_COMPILE)ld

all:
	# Prepate $(OBJDIR) for 2.6 build first
	@[ -d "$(OBJDIR)" ] || install -dv $(OBJDIR)
	@[ -h "$(OBJDIR)/Makefile" ] || \
		ln -sv $(SRCBASE)/linuxdev/Makefile $(OBJDIR)/Makefile
	@[ -h "$(OBJDIR)/dbus.c" ] || \
		ln -sv $(SRCBASE)/shared/dbus.c $(OBJDIR)/dbus.c
	@[ -h "$(OBJDIR)/dbus_usb.c" ] || \
		ln -sv $(SRCBASE)/shared/dbus_usb.c $(OBJDIR)/dbus_usb.c
	@[ -h "$(OBJDIR)/dbus_usb_linux.c" ] || \
		ln -sv $(SRCBASE)/shared/dbus_usb_linux.c $(OBJDIR)/dbus_usb_linux.c
	@[ -h "$(OBJDIR)/bcmutils.c" ] || \
		ln -sv $(SRCBASE)/shared/bcmutils.c $(OBJDIR)/bcmutils.c
	@[ -h "$(OBJDIR)/linux_osl.c" ] || \
		ln -sv $(SRCBASE)/shared/linux_osl.c $(OBJDIR)/linux_osl.c
	@[ -h "$(OBJDIR)/nvramstubs.c" ] || \
		ln -sv $(SRCBASE)/shared/nvramstubs.c $(OBJDIR)/nvramstubs.c

	# Launch module for given LINUXVER
	@echo "DFLAGS = $(DFLAGS)"
	@echo "WFLAGS = $(WFLAGS)"
	@echo "IFLAGS = $(IFLAGS)"
	$(MAKE) -C $(OBJDIR) -f $(SRCBASE)/linuxdev/Makefile \
		SRCBASE=$(SRCBASE) modules

modules:
	$(MAKE) -C $(LINUXDIR) KBUILD_EXTMOD=$(shell pwd) modules

clean:
	rm -rf obj-*

else # KBUILD_EXTMOD

	# Now build the bcm usb shim layer module
	obj-m  += $(BCM_EXTERNAL_MODULE_NAME).o
	$(BCM_EXTERNAL_MODULE_NAME)-y := $(OBJ_FILES)

endif # KBUILD_EXTMOD
