############################################################
#     (c)2010-2016 Broadcom Corporation
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

PWD := $(shell pwd)

ifeq ($(LINUX_OUT_1ST_ARCH),)
$(error LINUX_OUT path needs to be defined)
endif

obj-m += nx_ashmem.o

NEXUS_MODE=driver
OS=linuxkernel
PWD_BEFORE_PLATFORM_APP := $(PWD)
include $(NEXUS_TOP)/platforms/$(NEXUS_PLATFORM)/build/platform_app.inc
PWD := $(PWD_BEFORE_PLATFORM_APP)

NEXUS_KO_SYMVERS := ${B_REFSW_OBJ_ROOT_1ST_ARCH}/nexus/core/linuxkernel.${B_REFSW_ARCH_1ST_ARCH}.driver/Module.symvers

ccflags-y += -I$(BSEAV_TOP)/linux/driver

ccflags-y += -DB_REFSW_ANDROID -DBDBG_DEBUG_BUILD=1
FILTER_OUT_NEXUS_CFLAGS := -Wno-unused-but-set-variable -Wundef
NEXUS_CFLAGS := $(filter-out $(FILTER_OUT_NEXUS_CFLAGS), $(NEXUS_CFLAGS))
ccflags-y += $(NEXUS_CFLAGS)
ccflags-y += $(addprefix -I,$(NEXUS_APP_INCLUDE_PATHS))
ccflags-y += $(addprefix -D,$(NEXUS_APP_DEFINES))

ccflags-y += -include linux/kconfig.h

ifeq ($(V3D_VARIANT),)
V3D_VARIANT := v3d
endif
ifneq ($(V3D_VARIANT),v3d)
ccflags-y += -I$(NEXUS_TOP)/../magnum/portinginterface/vc5/include/
ccflags-y += -I$(NEXUS_TOP)/../magnum/basemodules/mem/
ccflags-y += -I$(NEXUS_TOP)/../magnum/basemodules/mma/
ccflags-y += -I$(NEXUS_TOP)/../magnum/basemodules/int/
endif
ccflags-y += -DV3D_VARIANT_$(V3D_VARIANT)

ccflags-y += -I$(LINUX_OUT_1ST_ARCH)/arch/$(P_REFSW_DRV_ARCH)/include
ccflags-y += -I$(LINUX_OUT_1ST_ARCH)/arch/$(P_REFSW_DRV_ARCH)/include/generated
ccflags-y += -I$(LINUX_OUT_1ST_ARCH)/arch/$(P_REFSW_DRV_ARCH)/include/uapi
ccflags-y += -I$(LINUX_OUT_1ST_ARCH)/arch/$(P_REFSW_DRV_ARCH)/include/generated/uapi
ccflags-y += -I$(LINUX_OUT_1ST_ARCH)/include/uapi
ccflags-y += -I$(LINUX_OUT_1ST_ARCH)/include/generated/uapi
ccflags-y += -I$(LINUX)/arch/$(P_REFSW_DRV_ARCH)/include
ccflags-y += -I$(LINUX)/arch/$(P_REFSW_DRV_ARCH)/include/uapi
ccflags-y += -I$(LINUX)/include/uapi


install: all
	cp $(PWD)/nx_ashmem.ko $(INSTALL_DIR)

all:
	@${MAKE} -C $(LINUX_OUT_1ST_ARCH) M=$(PWD) KBUILD_EXTRA_SYMBOLS=${NEXUS_KO_SYMVERS} modules

clean:
	@${MAKE} -C $(LINUX_OUT_1ST_ARCH) M=$(PWD) clean

