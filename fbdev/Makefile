############################################################
#     (c)2010-2014 Broadcom Corporation
#
#  This program is the proprietary software of Broadcom Corporation and/or its licensors,
#  and may only be used, duplicated, modified or distributed pursuant to the terms and
#  conditions of a separate, written license agreement executed between you and Broadcom
#  (an "Authorized License").  Except as set forth in an Authorized License, Broadcom grants
#  no license (express or implied), right to use, or waiver of any kind with respect to the
#  Software, and Broadcom expressly reserves all rights in and to the Software and all
#  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU
#  HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY
#  NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
#
#  Except as expressly set forth in the Authorized License,
#
#  1.     This program, including its structure, sequence and organization, constitutes the valuable trade
#  secrets of Broadcom, and you shall use all reasonable efforts to protect the confidentiality thereof,
#  and to use this information only in connection with your use of Broadcom integrated circuit products.
#
#  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#  AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#  WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH RESPECT TO
#  THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL IMPLIED WARRANTIES
#  OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE,
#  LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION
#  OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF
#  USE OR PERFORMANCE OF THE SOFTWARE.
#
#  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR ITS
#  LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR
#  EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO YOUR
#  USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT
#  ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
#  LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF
#  ANY LIMITED REMEDY.
############################################################
B_REFSW_ROOT := ${CWD}/../../../..

ifneq ($(B_REFSW_VERBOSE),)
Q_:=
else
Q_:=@
MAKEFLAGS += --no-print-directory
endif

ifdef COMSPEC
# Any DOS environment
NEXUS_TOP := $(shell cd ../../refsw/nexus && cd)
else
NEXUS_TOP := $(shell cd ../../refsw/nexus; pwd)
endif

ifeq ($(NEXUS_PLATFORM),)
$(error You must define NEXUS_PLATFORM)
endif

OS = linuxkernel

# include cross-compiler definitions
include $(NEXUS_TOP)/platforms/$(NEXUS_PLATFORM)/build/platform_app.inc

ifeq ($(LINUX),)
LINUX = /opt/brcm/linux
endif
LINUX_OUT ?= $(LINUX)

ifeq ($(B_REFSW_ARCH),)
B_REFSW_ARCH=mipsel-linux
endif

# NOTE: $LINUX/Makefile defaults ARCH=mips. It cannot be set to mipsel-linux here.
ifeq ($(filter ${B_REFSW_ARCH}, mipsel-linux mipsel-uclibc mipsel-linux-uclibc mipsel-linux-android), ${B_REFSW_ARCH})
B_REFSW_LINUX_ARCH=mips
B_REFSW_KERNEL_CROSS_COMPILE ?= mipsel-linux-
else
ifeq ($(filter ${B_REFSW_ARCH}, mips-linux mips-uclibc mips-linux-uclibc mips-linux-android), ${B_REFSW_ARCH})
B_REFSW_LINUX_ARCH=mips
B_REFSW_KERNEL_CROSS_COMPILE ?= mips-linux-
else
ifeq ($(filter ${B_REFSW_ARCH}, arm-linux ), ${B_REFSW_ARCH})
B_REFSW_LINUX_ARCH=arm
B_REFSW_KERNEL_CROSS_COMPILE ?= arm-linux-
endif
endif
endif

ifeq ($(ANDROID_BUILD),y)
NEXUS_FB_DEV_EXTRACFLAGS += -DB_REFSW_ANDROID -fno-pic
endif

ifdef DEBUG
B_REFSW_DEBUG ?= $(DEBUG)
endif
ifeq ($(B_REFSW_DEBUG),)
B_REFSW_DEBUG=y
endif
ifeq ($(B_REFSW_DEBUG),y)
NEXUS_FB_DEV_OBJ_DIR=$(B_REFSW_ARCH).debug$(NEXUS_BIN_DIR_SUFFIX)
else
NEXUS_FB_DEV_OBJ_DIR=$(B_REFSW_ARCH).release$(NEXUS_BIN_DIR_SUFFIX)
endif

NEXUS_FB_DEV_OBJ_ROOT := ${B_REFSW_OBJ_ROOT}/BSEAV/linux/driver/bcmnexusfb
NEXUS_FB_DEV_OBJ_PATH := ${NEXUS_FB_DEV_OBJ_ROOT}/${NEXUS_FB_DEV_OBJ_DIR}

# tools
CP = cp -fp
CC = $(B_REFSW_KERNEL_CROSS_COMPILE)gcc
AS = $(B_REFSW_KERNEL_CROSS_COMPILE)as
LD = $(B_REFSW_KERNEL_CROSS_COMPILE)ld
OBJCOPY = $(B_REFSW_KERNEL_CROSS_COMPILE)objcopy

# parse the Linux Makefile
LINUX_VERSION = $(shell grep -m 1 '^VERSION = ' ${LINUX}/Makefile | awk '{print $$3}')
LINUX_PATCHLEVEL = $(shell grep -m 1 '^PATCHLEVEL = ' ${LINUX}/Makefile | awk '{print $$3}')
LINUX_SUBLEVEL = $(shell grep -m 1 '^SUBLEVEL = ' ${LINUX}/Makefile | awk '{print $$3}')

LINUX_VER_GE_3 =  $(shell test $(LINUX_VERSION) -eq 3 && echo y)
ifeq ($(LINUX_VER_GE_3),y)
	LINUX_VER_GE_2_6 = y
	LINUX_VER_GE_2_6_31 = y
	LINUX_VER_GE_2_6_37 = y
else
LINUX_VER_GE_2_6 ?= $(shell test $(LINUX_PATCHLEVEL) -eq 6 && echo 'y')
ifeq ($(LINUX_VER_GE_2_6),y)
LINUX_VER_GE_2_6_31 =  $(shell test $(LINUX_SUBLEVEL) -ge 31 && echo y)
LINUX_VER_GE_2_6_37 =  $(shell test $(LINUX_SUBLEVEL) -ge 37 && echo y)
LINUX_VER_EQ_2_6_12 := $(shell test $(LINUX_SUBLEVEL) -eq 12 && echo 'y')
endif
endif

vpath %.c ./

NEXUS_FB_DEV := $(NEXUS_FB_DEV_OBJ_PATH)/bcmnexusfb.ko
NEXUS_FB_DEV_CFLAGS += -DKBUILD_MODNAME=bcmnexusfb -DLINUX -D__KERNEL__ -DMODULE -mlong-calls
NEXUS_FB_DEV_CFLAGS += -I$(LINUX_OUT)/include -I$(LINUX_OUT)/arch/$(B_REFSW_LINUX_ARCH)/include
NEXUS_FB_DEV_CFLAGS += -I$(LINUX)/include -I$(LINUX)/arch/$(B_REFSW_LINUX_ARCH)/include
NEXUS_FB_DEV_CFLAGS += $(NEXUS_CFLAGS) $(addprefix -I,$(NEXUS_APP_INCLUDE_PATHS)) $(addprefix -D,$(NEXUS_APP_DEFINES))
NEXUS_FB_DEV_CFLAGS := $(filter-out -Wno-unused-but-set-variable,$(NEXUS_FB_DEV_CFLAGS))

ifeq ($(LINUX_VER_GE_3),y)
NEXUS_FB_DEV_CFLAGS += -include linux/kconfig.h
else
ifeq ($(LINUX_VER_GE_2_6_37),y)
NEXUS_FB_DEV_CFLAGS += -include generated/autoconf.h
else
NEXUS_FB_DEV_CFLAGS += -include linux/autoconf.h
endif
endif

NEXUS_FB_DEV_SRCS += \
	bcmnexusfb.c

ifeq ($(B_REFSW_LINUX_ARCH),arm)
NEXUS_FB_DEV_CFLAGS += -I$(LINUX_OUT)/arch/arm/include -I$(LINUX_OUT)/arch/arm/include/generated -I$(LINUX_OUT)/arch/arm/include/uapi -I$(LINUX_OUT)/arch/arm/include/generated/uapi -I$(LINUX_OUT)/include/uapi -I$(LINUX_OUT)/include/generated/uapi
NEXUS_FB_DEV_CFLAGS += -I$(LINUX)/arch/arm/include -I$(LINUX)/arch/arm/include/uapi -I$(LINUX)/include/uapi
endif

NEXUS_FB_DEV_CFLAGS += -I.
NEXUS_FB_DEV_CFLAGS += -DBCHP_VER=BCHP_VER_$(BCHP_VER)

NEXUS_FB_DEV_OBJS := $(patsubst %.c,%.o,$(NEXUS_FB_DEV_SRCS))
NEXUS_FB_DEV_OBJS := $(addprefix $(NEXUS_FB_DEV_OBJ_PATH)/,$(NEXUS_FB_DEV_OBJS))

all: ${NEXUS_FB_DEV}

ifndef NEXUS_FB_DEV_KBUILD_CFLAGS
${NEXUS_FB_DEV}: ${NEXUS_FB_DEV_SRCS} $(NEXUS_FB_DEV_OBJ_PATH)/Kbuild_flags
	$(eval $(shell grep -m 1 -e "NEXUS_FB_DEV_KBUILD_CFLAGS:=" $(NEXUS_FB_DEV_OBJ_PATH)/Kbuild_flags))
	$(eval $(shell grep -m 1 -e "NEXUS_FB_DEV_KBUILD_LDFLAGS:=" $(NEXUS_FB_DEV_OBJ_PATH)/Kbuild_flags))
	@[ -n "${NEXUS_FB_DEV_KBUILD_CFLAGS}" ] || (echo "can't obtain NEXUS_FB_DEV_KBUILD_CFLAGS";exit 1)
	@${MAKE} NEXUS_FB_DEV_KBUILD_CFLAGS='$(filter-out -Wundef,$(NEXUS_FB_DEV_KBUILD_CFLAGS))' NEXUS_FB_DEV_KBUILD_LDFLAGS='${NEXUS_FB_DEV_KBUILD_LDFLAGS}'
else
$(NEXUS_FB_DEV): $(NEXUS_FB_DEV_OBJS)
	@echo [Link...... bcmnexusfb.ko]
ifeq ($(LINUX_VER_GE_2_6_31),y)
	$(Q_)$(LD) -Map $(NEXUS_FB_DEV).map $(NEXUS_FB_DEV_LDFLAGS) $(NEXUS_FB_DEV_KBUILD_LDFLAGS) $(NEXUS_FB_DEV_EXTRALDFLAGS) --strip-debug --build-id=none -r $^  -o $(NEXUS_FB_DEV_OBJ_PATH)/bcmnexusfb.o_shipped
	$(Q_)$(CP) ${NEXUS_OBJ_DIR}/Module.symvers $(NEXUS_FB_DEV_OBJ_PATH)
	$(Q_)$(MAKE) -C $(LINUX_OUT) M=$(NEXUS_FB_DEV_OBJ_PATH) ARCH=$(B_REFSW_LINUX_ARCH) modules NEXUS_PLATFORM=$(NEXUS_PLATFORM) CROSS_COMPILE=$(B_REFSW_KERNEL_CROSS_COMPILE)
else
	@# do not use kbuild for 2.6.18
	$(Q_)$(LD) -Map $(NEXUS_FB_DEV).map $(NEXUS_FB_DEV_LDFLAGS) $(NEXUS_FB_DEV_KBUILD_LDFLAGS) $(NEXUS_FB_DEV_EXTRALDFLAGS) --strip-debug -r $^  -o $@
endif
	@# disable MODVERSIONS if kernel symvers are missing
	@if [ -s $(LINUX_OUT)/Modules.symvers ]; then \
		$(OBJCOPY) --remove-section __versions $(NEXUS_FB_DEV_OBJ_PATH)/bcmnexusfb.ko ; \
	fi
endif # NEXUS_FB_DEV_KBUILD_CFLAGS

$(NEXUS_FB_DEV_OBJ_PATH)/%.o: %.c
	@echo [Compile... $(notdir $<)];
	$(Q_)$(CC) $(NEXUS_FB_DEV_CFLAGS) $(NEXUS_FB_DEV_KBUILD_CFLAGS) $(NEXUS_FB_DEV_EXTRACFLAGS) -o $@ -c $<

clean: $(NEXUS_FB_DEV_OBJ_PATH)/Kbuild
	${Q_}$(MAKE) -C $(LINUX_OUT) M=$(NEXUS_FB_DEV_OBJ_PATH) ARCH=$(B_REFSW_LINUX_ARCH) clean
	${Q_}$(RM) $(NEXUS_FB_DEV) $(NEXUS_FB_DEV).map $(NEXUS_FB_DEV_OBJ_PATH)/Kbuild_flags $(NEXUS_FB_DEV_OBJ_PATH)/bcmnexusfb.o_shipped $(NEXUS_FB_DEV_OBJS)
	${Q_}$(RM) ./bcmnexusfb.ko

ifeq ($(INSTALL_DIR),)
install:
	$(error INSTALL_DIR is undefined)
else
install: all
	@echo "[Install... bcmnexusfb.ko]"
	${Q_}$(CP) $(NEXUS_FB_DEV) $(INSTALL_DIR)
	${Q_}$(CP) $(NEXUS_FB_DEV) .
endif

$(NEXUS_FB_DEV_OBJ_PATH)/Kbuild: Kbuild $(NEXUS_FB_DEV_OBJ_PATH)/exists
	@echo [Copying... Kbuild];
	$(Q_)$(CP) $< $@

$(NEXUS_FB_DEV_OBJ_PATH)/exists:
	${Q_}mkdir -p $@

.PHONY: $(NEXUS_FB_DEV_OBJ_PATH)/Kbuild_flags

$(NEXUS_FB_DEV_OBJ_PATH)/Kbuild_flags : $(NEXUS_FB_DEV_OBJ_PATH)/Kbuild
	${Q_}$(MAKE) -C $(LINUX_OUT) M=$(NEXUS_FB_DEV_OBJ_PATH) modules ARCH=$(B_REFSW_LINUX_ARCH) NEXUS_PLATFORM=$(NEXUS_PLATFORM) CROSS_COMPILE=$(B_REFSW_KERNEL_CROSS_COMPILE) BCM_PRINT_KBUILD_ENV_ONLY=1 2>$@ >/dev/null
