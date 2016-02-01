CWD := $(shell pwd)
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
NX_ASHMEM_EXTRACFLAGS += -DB_REFSW_ANDROID
endif

ifdef DEBUG
B_REFSW_DEBUG ?= $(DEBUG)
endif
ifeq ($(B_REFSW_DEBUG),)
B_REFSW_DEBUG=y
endif
ifeq ($(B_REFSW_DEBUG),y)
NX_ASHMEM_OBJ_DIR=$(B_REFSW_ARCH).debug$(NEXUS_BIN_DIR_SUFFIX)
else
NX_ASHMEM_OBJ_DIR=$(B_REFSW_ARCH).release$(NEXUS_BIN_DIR_SUFFIX)
endif

NX_ASHMEM_OBJ_ROOT := ${B_REFSW_OBJ_ROOT}/BSEAV/linux/driver/nx_ashmem
NX_ASHMEM_OBJ_PATH := ${NX_ASHMEM_OBJ_ROOT}/${NX_ASHMEM_OBJ_DIR}

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

NX_ASHMEM := $(NX_ASHMEM_OBJ_PATH)/nx_ashmem.ko
NX_ASHMEM_CFLAGS += -DKBUILD_MODNAME=nx_ashmem -DLINUX -D__KERNEL__ -DMODULE -mlong-calls
NX_ASHMEM_CFLAGS += -I$(LINUX_OUT)/include -I$(LINUX_OUT)/arch/$(B_REFSW_LINUX_ARCH)/include
NX_ASHMEM_CFLAGS += -I$(LINUX)/include -I$(LINUX)/arch/$(B_REFSW_LINUX_ARCH)/include
NX_ASHMEM_CFLAGS += $(NEXUS_CFLAGS) $(addprefix -I,$(NEXUS_APP_INCLUDE_PATHS)) $(addprefix -D,$(NEXUS_APP_DEFINES))
NX_ASHMEM_CFLAGS := $(filter-out -Wno-unused-but-set-variable,$(NX_ASHMEM_CFLAGS))

ifeq ($(LINUX_VER_GE_3),y)
NX_ASHMEM_CFLAGS += -include linux/kconfig.h
else
ifeq ($(LINUX_VER_GE_2_6_37),y)
NX_ASHMEM_CFLAGS += -include generated/autoconf.h
else
NX_ASHMEM_CFLAGS += -include linux/autoconf.h
endif
endif

NX_ASHMEM_SRCS += \
	nx_ashmem.c

ifeq ($(B_REFSW_LINUX_ARCH),arm)
NX_ASHMEM_CFLAGS += -I$(LINUX_OUT)/arch/arm/include -I$(LINUX_OUT)/arch/arm/include/generated -I$(LINUX_OUT)/arch/arm/include/uapi -I$(LINUX_OUT)/arch/arm/include/generated/uapi -I$(LINUX_OUT)/include/uapi -I$(LINUX_OUT)/include/generated/uapi
NX_ASHMEM_CFLAGS += -I$(LINUX)/arch/arm/include -I$(LINUX)/arch/arm/include/uapi -I$(LINUX)/include/uapi
endif

NX_ASHMEM_CFLAGS += -I$(B_REFSW_ROOT)/magnum/commonutils/lst/
NX_ASHMEM_CFLAGS += -I.
NX_ASHMEM_CFLAGS += -DHAVE_RSRVD=1
NX_ASHMEM_CFLAGS += -DBCHP_VER=BCHP_VER_$(BCHP_VER)
ifeq ($(V3D_VARIANT),)
V3D_VARIANT := v3d
endif
ifneq ($(V3D_VARIANT),v3d)
NX_ASHMEM_CFLAGS += -I$(NEXUS_TOP)/../magnum/portinginterface/vc5/include/
NX_ASHMEM_CFLAGS += -I$(NEXUS_TOP)/../magnum/basemodules/mem/
NX_ASHMEM_CFLAGS += -I$(NEXUS_TOP)/../magnum/basemodules/mma/
NX_ASHMEM_CFLAGS += -I$(NEXUS_TOP)/../magnum/basemodules/int/
endif
NX_ASHMEM_CFLAGS += -DV3D_VARIANT_$(V3D_VARIANT)

NX_ASHMEM_OBJS := $(patsubst %.c,%.o,$(NX_ASHMEM_SRCS))
NX_ASHMEM_OBJS := $(addprefix $(NX_ASHMEM_OBJ_PATH)/,$(NX_ASHMEM_OBJS))

all: ${NX_ASHMEM}

ifndef NX_ASHMEM_KBUILD_CFLAGS
${NX_ASHMEM}: ${NX_ASHMEM_SRCS} $(NX_ASHMEM_OBJ_PATH)/Kbuild_flags
	$(eval $(shell grep -m 1 -e "NX_ASHMEM_KBUILD_CFLAGS:=" $(NX_ASHMEM_OBJ_PATH)/Kbuild_flags))
	$(eval $(shell grep -m 1 -e "NX_ASHMEM_KBUILD_LDFLAGS:=" $(NX_ASHMEM_OBJ_PATH)/Kbuild_flags))
	@[ -n "${NX_ASHMEM_KBUILD_CFLAGS}" ] || (echo "can't obtain NX_ASHMEM_KBUILD_CFLAGS";exit 1)
	@${MAKE} NX_ASHMEM_KBUILD_CFLAGS='$(filter-out -Wundef,$(NX_ASHMEM_KBUILD_CFLAGS))' NX_ASHMEM_KBUILD_LDFLAGS='${NX_ASHMEM_KBUILD_LDFLAGS}'
else
$(NX_ASHMEM): $(NX_ASHMEM_OBJS)
	@echo [Link...... nx_ashmem.ko]
ifeq ($(LINUX_VER_GE_2_6_31),y)
	$(Q_)$(LD) -Map $(NX_ASHMEM).map $(NX_ASHMEM_LDFLAGS) $(NX_ASHMEM_KBUILD_LDFLAGS) $(NX_ASHMEM_EXTRALDFLAGS) --strip-debug --build-id=none -r $^  -o $(NX_ASHMEM_OBJ_PATH)/nx_ashmem.o_shipped
	$(Q_)$(CP) ${NEXUS_OBJ_DIR}/Module.symvers $(NX_ASHMEM_OBJ_PATH)
	$(Q_)$(MAKE) -C $(LINUX_OUT) M=$(NX_ASHMEM_OBJ_PATH) ARCH=$(B_REFSW_LINUX_ARCH) modules NEXUS_PLATFORM=$(NEXUS_PLATFORM) CROSS_COMPILE=$(B_REFSW_KERNEL_CROSS_COMPILE)
else
	@# do not use kbuild for 2.6.18
	$(Q_)$(LD) -Map $(NX_ASHMEM).map $(NX_ASHMEM_LDFLAGS) $(NX_ASHMEM_KBUILD_LDFLAGS) $(NX_ASHMEM_EXTRALDFLAGS) --strip-debug -r $^  -o $@
endif
	@# disable MODVERSIONS if kernel symvers are missing
	@if [ -s $(LINUX_OUT)/Modules.symvers ]; then \
		$(OBJCOPY) --remove-section __versions $(NX_ASHMEM_OBJ_PATH)/nx_ashmem.ko ; \
	fi
endif # NX_ASHMEM_KBUILD_CFLAGS

$(NX_ASHMEM_OBJ_PATH)/%.o: %.c
	@echo [Compile... $(notdir $<)];
	$(Q_)$(CC) $(NX_ASHMEM_CFLAGS) $(NX_ASHMEM_KBUILD_CFLAGS) $(NX_ASHMEM_EXTRACFLAGS) -o $@ -c $<

clean: $(NX_ASHMEM_OBJ_PATH)/Kbuild
	${Q_}$(MAKE) -C $(LINUX_OUT) M=$(NX_ASHMEM_OBJ_PATH) ARCH=$(B_REFSW_LINUX_ARCH) clean
	${Q_}$(RM) $(NX_ASHMEM) $(NX_ASHMEM).map $(NX_ASHMEM_OBJ_PATH)/Kbuild_flags $(NX_ASHMEM_OBJ_PATH)/nx_ashmem.o_shipped $(NX_ASHMEM_OBJS)

ifeq ($(INSTALL_DIR),)
install:
	$(error INSTALL_DIR is undefined)
else
install: all
	@echo "[Install... nx_ashmem.ko]"
	${Q_}$(CP) $(NX_ASHMEM) $(INSTALL_DIR)
endif

$(NX_ASHMEM_OBJ_PATH)/Kbuild: Kbuild $(NX_ASHMEM_OBJ_PATH)/exists
	@echo [Copying... Kbuild];
	$(Q_)$(CP) $< $@

$(NX_ASHMEM_OBJ_PATH)/exists:
	${Q_}mkdir -p $@

.PHONY: $(NX_ASHMEM_OBJ_PATH)/Kbuild_flags

$(NX_ASHMEM_OBJ_PATH)/Kbuild_flags : $(NX_ASHMEM_OBJ_PATH)/Kbuild
	${Q_}$(MAKE) -C $(LINUX_OUT) M=$(NX_ASHMEM_OBJ_PATH) modules ARCH=$(B_REFSW_LINUX_ARCH) NEXUS_PLATFORM=$(NEXUS_PLATFORM) CROSS_COMPILE=$(B_REFSW_KERNEL_CROSS_COMPILE) BCM_PRINT_KBUILD_ENV_ONLY=1 2>$@ >/dev/null
