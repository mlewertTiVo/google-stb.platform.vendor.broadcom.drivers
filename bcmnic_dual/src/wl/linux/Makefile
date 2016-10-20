#
# GNUmakefile for Linux
# Broadcom 802.11abg Networking Device Driver
#
# Copyright (C) 2016, Broadcom Corporation
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# Usage:
#
# make { [LINUXDIR=<path>] | [LINUXVER=<version>] }
#      [debug|nodebug]-[native|mipsel|mipseb|arm]-[<config file>]{-<config file>}
#      [API=WEXT|CFG80211|NONE]
#
# <config file> is a file which is included by this makefile for the purpose
#   of defining pre-defined make variables.  These make variables are then
#   converted to CC flags, file lists, etc which are used to control the
#   compilation, linking, etc.  These config files are by default in directory
#   .../src/wl/config and have a prefix wlconfig_lx_wl_.  For example
#   <config file> "stadef" would include file wlconfig_lx_wl_stadef.
#   Multiple <config file>'s can be specified separated by a - in which
#   case they are included in the order specified before conversion.
#   In addition, these make variables can be set using the command line,
#   e.g. make WLxxx=1 or by exporting to the make process environment.  An
#   important point is that assignments made in config files (or internally
#   by this script) will override initial make process environment variables
#   but not command line specified variables so be careful.
#
# If no make target is specified, a collection of various combinations of
#   debug/nodebug and <config file>'s for the "native" architecture will be
#   built.
#
# An object directory(s) of the form obj-<fully qualified target name>-$(LINUXVER)
#   will be created in the current directory and populated.  Typically this
#   make file is invoked from its parent directory .../src/wl/linux.
#
#   The object directory will have these files
#      *.o            bet you know what these are! :-)
#      wlcfg-obj      make variables from inclusion of <config file>('s)
#      wlconf.h       sample (commented out) tunable parameters file
#                     (used if no <config file> appropriate option>)
#      Module.symvers (2.6 kernel only)  ??? 0 length file created by
#                     Linux 2.6 external kernel module framework
#      Makefile       (2.6 kernel only)  Linux 2.6 external kernel
#                     module make file
#      *.c            (2.6 kernels only) These are symbolic links to
#                     all .c's (not .h's) because using the Linux 2.6
#                     external kernel module build framework requires
#                     all source in a single directory.
#
#   Note: The make file, object directory and source can be in different
#   locations through judicious use of make's -C and -f options plus
#   definition of make variable SRCBASE via the environment or a make
#   command line option,
#

#
# Description:
#
# Determines $(TARGETS) based on command-line target or a default set then
# selects each $(TARGET), creating an objdir and launching a submake.
#
# Phased operation:
#  1) For each cmdline target, generate a $(TARGETS) set for a submake.
#  2) Given $(TARGETS) list from (1), do an objdir submake for each $(TARGET).
#  3) Use $(TARGET) to build OBJDIR and wl config file, do submake in objdir.
#  4) Now $(INOBJDIR): read config and wl.mk for most flags/files settings,
#     may still use $(TARGET) for architecture and such.  Do dep, modules.
#
# $Id: Makefile 655991 2016-08-24 18:34:07Z $
#

# Some unix systems have /bin/sh linked to some incompatible shells
# that break the build. So set it to bash by default
SHELL=/bin/bash

# Root name for object dir - could be just a prefix, could be a directory.
ObjPfx := obj-

# Bring in global settings using the location of current makefile as a guide.
include $(dir $(lastword $(MAKEFILE_LIST)))/../../makefiles/WLAN_Common.mk

# Targets are built one at a time, but within each target they
# are built parallel by default.
#   To change number of CPUs to use "gmake JOBS=n <target>"
#   To build serially, use "gmake JOBS= <target>"
#   PAR="-j n" is equivalent and supported for compatibility.
ifdef PAR
JOBS := $(word 2,$(PAR))
else
JOBS := 4
endif

# If a source file is generated it should not be copied or linked.
# from source control. Instead, a rule must be written to generate it.
GENERATED_SOURCES := wlc_clm_data.c

# Although 32ON64 flag is supposed to be passed from caller, set it
# if not set by caller on certain kernels and platforms
ifeq ($(shell uname -m),x86_64)
     ifneq ($(findstring x86_64,$(LINUXVER)),x86_64)
          export 32ON64=1
          # $(warning Passing 32ON64 flag to force 32bit driver for $(LINUXVER))
     endif # LINUXVER
endif # CURPROC

ifeq ($(TARGETS),)

ifeq ($(DEFBASICS),)
     #Default combinations
     DEFBASICS	:= apdef
     DEFBASICS	+= stadef
     DEFBASICS	+= apdef-stadef
     DEFBASICS	+= apdef-stadef-high
endif # DEFBASICS

DEBUGS := debug nodebug

# Default list of targets
DEFTARGETS = $(foreach dvar,$(DEBUGS),\
                $(patsubst %,$(dvar)-native-%,$(DEFBASICS)))

# Note: TARGETS command variable will be inherited through MAKEFLAGS to
#       cause recursive makes to only see the else condition.
# Generate targets from command-line args, or use default set
.DEFAULT:
	@echo "Generating target(s) from $@"
	+$(MAKE) EXPANDED=true TARGETS="$@" $@
default all:
	@echo "Using default target(s) $(DEFTARGETS)"
	+$(MAKE) EXPANDED=true TARGETS="$(DEFTARGETS)" $(DEFTARGETS)
native mipsel mipseb arm arm_le:
	@echo "Using default target(s) for $@ architecture"
	+$(MAKE) EXPANDED=true TARGETS="$(subst native,$@,$(DEFTARGETS))" \
		$(subst native,$@,$(DEFTARGETS))
clean:
ifeq (,$(filter %/,$(ObjPfx)))
	$(RM) -r $(ObjPfx)*
else
	$(RM) -r $(ObjPfx)
endif
ifneq ($(findstring external,$(BRAND)),)
	$(MAKE) -C $(WLAN_SrcBaseA)/linuxdev clean
endif

help:
	@echo "make [LINUXDIR=<path>] [LINUXVER=<version>]"
	@echo "     [debug|nodebug]-[native|mipsel|arm]-[<config>]"
	@echo
	@echo "     Use WLCFGDIR=x, WLCFG_PREFIX=y to override defaults of"
	@echo "     \$$(WLAN_SrcBaseA)/wl/config and wlconfig_lx_wl_ for <config>."
	@echo
	@echo "     A <config> is the \"basename\" of a configuration file,"
	@echo "     for example, \"native-stadef\" refers to the config file"
	@echo "     \$$(WLCFGDIR)/wlconfig_lx_wl_stadef (w/default prefix)."
	@echo
	@echo "     More than one <config> (e.g. native-apdef-stadef) may be"
	@echo "     specified; they concatenate into a single configuration."
	@echo "     If no target is specified, the ap/sta and debug/nodebug "
	@echo "     variants are built; specifying just an arch (e.g. arm)"
	@echo "     builds the variants for that architecture.  Otherwise"
	@echo "     at least one <config> must be specified."
	@echo
	@echo "examples:"
	@echo "  default set                    make"
	@echo "  default set for mips           make mipsel"
	@echo "  debug ap-sta                   make debug-apdef-stadef"
	@echo "  sta for specific linux         make LINUXVER=2.4.18-3 stadef"
	@echo

else # TARGETS set

# Setup LINUXDIR if not specified.  Use LINUXVER if specified to create
#   a LINUXDIR.  Use uname -r for LINUXVER ifuna, else use h
ifeq ($(LINUXDIR),)
	ifeq ($(LINUXVER),)
		# LINUXVER is not set, derive it from native 'uname -r'
		LINUXVER := $(shell uname -r)
                ifndef ECLOUD_BUILD_ID
                       $(info LINUXVER not defined, set to native $(LINUXVER))
                endif # ECLOUD_BUILD_ID
	endif
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

# Default WLARCH is x86
WLARCH ?= x86
ifneq ($(findstring native,$(TARGET)),)
   WLARCH = x86
endif
ifneq ($(findstring mips,$(TARGET)),)
   WLARCH = mips
endif
ifneq ($(findstring arm,$(TARGET)),)
   WLARCH = arm
endif

ifneq ($(findstring armv7l,$(TARGET)),)
   WLARCH = arm
endif

# Derive LINUXVER from LINUXDIR
MYKERNEL_RELEASE_KEYWORD:="KERNELRELEASE[[:space:]]*=.*kernel.release"
MYKERNEL_DEFINITION:=$(if \
  $(shell grep $(MYKERNEL_RELEASE_KEYWORD) $(LINUXDIR)/Makefile 2> /dev/null),\
  grep $(MYKERNEL_RELEASE_KEYWORD) $(LINUXDIR)/Makefile,\
  cat $(LINUXDIR)/Makefile)

# Check if specified LINUXVER conflicts with LINUXDIR's LINUXVER
# Most of the times LINUXVER is supplied in env for FCx variant builds
LINUXVER:=$(shell ($(MYKERNEL_DEFINITION); echo "show_kernel_version_number$$$$:;@echo \$$(KERNELRELEASE)") 2> /dev/null | $(MAKE) --no-print-directory -k -C $(LINUXDIR) MYUNAME="" -f - show_kernel_version_number$$$$ 2> /dev/null)

BCM_KVERSIONSTRING := $(word 1,$(subst -,$(space),$(LINUXVER)))

ifeq (,$(word 3,$(subst .,$(space),$(BCM_KVERSIONSTRING))))
     $(error LINUXVER bad format: "$(LINUXVER)")
endif

# Kernel include paths changed starting from fc9
# We need usb shim layer module for FC9 kernels for split driver
# Build usb shim only for external bmac builds that need 2.6.21+ kernels

# Check if 2.6.18+ or higher
ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6.18),TRUE)
    # Disable for developers and enable for external and mfgtest builds
    ifneq ($(findstring external,$(BRAND))$(findstring mfgtest,$(BRAND)),)
        # The external module 'bcm_dbus.ko' is needed only for usb/bmac driver
        ifneq ($(findstring high,$(TARGET)),)
	    BCM_EXTERNAL_MODULE=1
	    export BCM_EXTERNAL_MODULE_REQUIRED=TRUE
	    export BCM_EXTERNAL_MODULE_NAME=bcm_dbus
	    export KBUILD_EXTRA_SYMBOLS=$(WLAN_SrcBaseA)/linuxdev/$(ObjPfx)$(BCM_EXTERNAL_MODULE_NAME)-$(LINUXVER)-$(WLARCH)/Module.symvers
        endif # TARGET
    endif # BRAND
endif # KERNEL >= 2.6.18

ifneq ($(findstring cfg80211,$(TARGET)),)
    ifneq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6.38),TRUE)
	$(error -cfg80211 must be built against kernel 2.6.38 or after)
    endif # KERNEL >= 2.6.38
    BCM_STA_CFG80211=1
    DFLAGS += -DBCM_STA_ANDROID
    export API=CFG80211
endif # TARGET

# IFLAGS computation precedes WLCONFFILE inclusion,
# so set flags for set-top-box wl driver build, as they
# are used to derive include paths.
ifneq ($(findstring media-mips,$(TARGET))$(findstring mips-media,$(TARGET)),)
    WLARCH=mips
    STBLINUX=1
endif # media-mips

ifneq ($(findstring stb,$(TARGET)),)
    STBLINUX=1
endif # stb


ifndef ECLOUD_BUILD_ID
       $(info TARGET=$(TARGET) BCM_EXTERNAL_MODULE=$(BCM_EXTERNAL_MODULE) WLARCH=$(WLARCH) STBLINUX=$(STBLINUX))
endif # ECLOUD_BUILD_ID

# basic options, include current obj dir for wl driver config file
DFLAGS := -DLINUX

DFLAGS += -DSRCBASE=\"$(WLAN_SrcBaseA)\" -DBCMDRIVER

ifneq ($(findstring 4360,$(CHIPVER)),)
    STB_BCM4360=1
	DFLAGS += -DSTB_BCM4360
endif # STB_BCM4360

ifneq ($(findstring 43217,$(CHIPVER)),)
    STB_BCM43217=1
	DFLAGS += -DSTB_BCM43217
endif # STB_BCM43217

ifneq ($(findstring 43228,$(CHIPVER)),)
    STB_BCM43228=1
	DFLAGS += -DSTB_BCM43228
endif # STB_BCM43228

ifeq ($(STBLINUX),1)
	WOWL_GPIOPIN?=11
	WOWL_GPIO_POLARITY?=1
	DFLAGS +=-DWOWL_GPIO=$(WOWL_GPIOPIN)
	DFLAGS +=-DWOWL_GPIO_POLARITY=$(WOWL_GPIO_POLARITY)
endif

	ifneq ($(filter arm,$(TARGETARCH)),)
	DFLAGS += -D__LINUX_ARM_ARCH__=7 -march=armv7-a
	endif

ifneq ($(findstring comp,$(TARGET)),)
   DFLAGS += -DBCMUSBDEV_COMPOSITE
endif

ifneq ($(findstring armv7l,$(TARGET)),)
# Should add the arm arch version specific details
   DFLAGS += -D__LINUX_ARM_ARCH__=7 -march=armv7-a
endif

IFLAGS=
IFLAGS   += -I$(LINUX_OUT)/include
IFLAGS   += -I$(LINUXDIR)/include
ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),3.7.0),TRUE)
  IFLAGS   += -I$(LINUXDIR)/include/uapi
  IFLAGS   += -I$(LINUXDIR)/include/generated/uapi
endif

ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6.25),TRUE)
  LINUXDIR_INC_BASE = $(LINUXDIR)/arch/$(WLARCH)
  LINUXOUT_INC_BASE = $(LINUX_OUT)/arch/$(WLARCH)
else
  LINUXDIR_INC_BASE = $(LINUXDIR)
  LINUXOUT_INC_BASE = $(LINUX_OUT)
endif
IFLAGS += -I$(LINUXDIR_INC_BASE)/include
IFLAGS += -I$(LINUXOUT_INC_BASE)/include/generated
IFLAGS += -I$(LINUXDIR_INC_BASE)/include/generated
ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),3.7.0),TRUE)
  IFLAGS += -I$(LINUXDIR_INC_BASE)/include/uapi
  IFLAGS += -I$(LINUXOUT_INC_BASE)/include/generated/uapi
  IFLAGS += -I$(LINUXDIR_INC_BASE)/include/generated/uapi
endif
ifeq ($(STBLINUX),1)
  IFLAGS += -I$(LINUXDIR_INC_BASE)/include/asm/mach-brcmstb
  IFLAGS += -I$(LINUXDIR_INC_BASE)/include/asm/mach-generic
else
  IFLAGS += -I$(LINUXDIR)/include/asm-$(WLARCH)/mach-default
  IFLAGS += -I$(LINUXDIR_INC_BASE)/include/asm/mach-default
endif
IFLAGS   += -I.
IFLAGS   += -I$(WLAN_SrcBaseA)/wl/sys
IFLAGS   += -I$(WLAN_SrcBaseA)/wl/phy
IFLAGS   += $(WLAN_ComponentIncPathA)
IFLAGS   += -I$(WLAN_SrcBaseA)/include
IFLAGS   += -I$(WLAN_SrcBaseA)/shared
IFLAGS   += -I$(WLAN_SrcBaseA)/shared/zlib
IFLAGS   += $(WLAN_StdIncPathA)


############################################################
# Check if the dongle image is embedded
############################################################

ifeq ($(BCMEMBEDIMAGE),1)
	DNGL_IMAGE_NAME ?=
	DNGL_IMAGE_PATH := $(WLAN_SrcBaseA)/dongle/rte/wl/builds/$(DNGL_IMAGE_NAME)
	DFLAGS += -DBCMEMBEDIMAGE=\"$(DNGL_IMAGE_PATH)/rtecdc.h\"
endif

# List of firmwares that can be embedded into linux high driver
  # External/release images
  EMBED_IMAGE_43236b=43236b-bmac/ag-nodis


# Actual list of firmwares to embed into high driver
# NOTE: Top-level release makefile may supply list of embeddable firmwares
# NOTE: So update build release makefile in src/tools/release when you update
# NOTE: following list
ifndef EMBED_DONGLE_IMAGES
  ifeq ($(findstring highsdio,$(TARGET)),)
  EMBED_DONGLE_IMAGES += $(EMBED_IMAGE_43236b)
  else
	EMBED_DONGLE_IMAGES_SD :=
  endif
endif

# For <tgt>-high-dnglimage target, embed a list of firmwares
# Firmware image names come from wlconfig_lx_wl_dnglimage config file
EMBED_DONGLE_CHIPREVS=$(foreach img,$(EMBED_DONGLE_IMAGES),$(subst -bmac,,$(word 1,$(subst /,$(space),$(img)))))
ifneq ($(findstring high,$(TARGET)),)
  ifneq ($(findstring dnglimage,$(TARGET)),)
    CFLAGS += -DBCM_DNGL_EMBEDIMAGE
    IFLAGS += -I$(shell pwd)

    ifneq ($(findstring highsdio,$(TARGET)),)
      override EMBED_DONGLE_IMAGES =  $(EMBED_DONGLE_IMAGES_SD)
    endif

    # Developer can override like on windows side, by specifying
    ifdef FIRMWARE
      override EMBED_DONGLE_IMAGES = $(FIRMWARE)
    endif # FIRMWARE

    CFLAGS += $(EMBED_DONGLE_CHIPREVS:%=-DEMBED_IMAGE_%)
    IFLAGS += $(EMBED_DONGLE_IMAGES:%=-I$(WLAN_SrcBaseA)/dongle/rte/wl/builds/%)
  endif # dnglimage
endif # high

ifeq ($(STBLINUX),1)
WFLAGS := -Wall -Wstrict-prototypes -Werror -Wno-unused-local-typedefs -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
else
WFLAGS := -Wall -Wstrict-prototypes -Werror # -Wpointer-arith
endif

ifeq ($(GCCVER),5.1.1)
WFLAGS += -Wno-error=date-time
endif #GCCVER

#disabled# # Kernel starting from fc15 need a gcc 4.6.+ compiler that is more stricter
#disabled# # and doesn't allow warnings pass compilation step.
#disabled# # Development group requested that fixing FC15+ warnings takes few weeks
#disabled# # hence for now suppress those warnings, but do NOT let them linger for too
#disabled# # long as this may mask real sloppy compilation mistakes.
#disabled# # WARN: Intentionally masking errors only until 2.6.40.x

#disabled# ifeq ($(filter arm% mips%,$(TARGETARCH))$(filter arm% mips%,$(ARCH)),)
#disabled#   ifneq ($(filter 2.6.38% 2.6.39% 2.6.40%,$(LINUXVER)),)
#disabled#     WFLAGS += -Wno-unused-but-set-variable
#disabled#     WFLAGS += -Wno-unused-but-set-parameter
#disabled#     WFLAGS += -Wframe-larger-than=1024
#disabled#     WFLAGS += -maccumulate-outgoing-args
#disabled#   endif # LINUXVER
#disabled# endif # TARGETARCH

# Use -Wmissing-prototypes,
# except for Linux 2.4.20-8 which had a warning in <fs.h>
# except for Linux 2.4.21-40 which had a warning in <quota.h>
# except for Linux 2.6.11-1 which had a warning in <div64.h>
NO_MISSING_PROTOTYPES_LINUXVER_PATTERNS :=
NO_MISSING_PROTOTYPES_LINUXVER_PATTERNS += 2.4.20-%
NO_MISSING_PROTOTYPES_LINUXVER_PATTERNS += 2.4.21-%
NO_MISSING_PROTOTYPES_LINUXVER_PATTERNS += 2.6.9-%
NO_MISSING_PROTOTYPES_LINUXVER_PATTERNS += 2.6.11-%
ifeq (,$(filter $(NO_MISSING_PROTOTYPES_LINUXVER_PATTERNS),$(LINUXVER)))
  WFLAGS += -Wmissing-prototypes
endif

LDFLAGS := -r
MODULES := wl.o
ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6),TRUE)
  # Kernel module names in 2.6 kernel have .ko suffix
  KMODULES:=wl.ko
  ifneq ($(findstring nintendo,$(TARGET)),)
    KMODULES := wlpci.ko
    ifneq ($(findstring nintendo-sdio,$(TARGET)),)
      KMODULES := wlsdio.ko
    endif
    ifneq ($(findstring high,$(TARGET)),)
      KMODULES := wl.ko
    endif
  else
    KMODULES := wl.ko
  endif
else # KERNEL < 2.6
  KMODULES:=$(MODULES)
endif # KERNEL

# some helpful variables
WLCFGDIR        ?= $(WLAN_SrcBaseA)/wl/config
WLCFG_PREFIX    ?= wlconfig_lx_wl_
WLTUNEFILE      ?= wltunable_sample.h

UPDATESH        := bash $(WLCFGDIR)/diffupdate.sh

WLCONF_DEF_FILE := $(WLCFGDIR)/wlconfig_lx_wl_feature
WLCONFFILE      := wlcfg-obj


########################################################
# Generate TARGETS if needed (first pass), else set
# TARGET to current TARGETS element and start build
# process with objdir (second pass).
#
# NOTE: #ifdef and #endif are mogrification if blocks
# NOTE: ifdef/ifeq/ifneq and endif (without '#') are
#       makefile if blocks
########################################################

# Target portions recognized by make: arch and debug
ARCHES := native mipsel mipseb arm

DEBUGS := debug nodebug

###########################################################
# (TARGET) specified, determine OBJDIR and WLCONFS
###########################################################

# Directory name includes target
OBJDIR=$(ObjPfx)$(TARGET)-$(LINUXVER)

# Split apart make and config stuff
INMAKE:= $(filter $(DEBUGS) $(ARCHES),$(subst -,$(space),$(TARGET)))
WLCONFS:=$(filter-out $(INMAKE),$(subst -,$(space),$(TARGET)))

############################################################
# In the objdir, actual building takes place.  Set the
# required variables by setting basic options, getting
# more from the config file, and using wl.mk to map to
# flags and files.  Some arch/debug stuff done here.
############################################################

ifeq ($(INOBJDIR),true)
  # Set basic config options for Linux WL
  WL=1
  WLLX=1
  WLLXIW=1
  WLLXCFG80211=1
  OSLLX=1

####################################
# To force one of the wireless APIs, specify
# on the command line like so:
# make stadef-debug API=WEXT
# make stadef-debug API=CFG80211
# make stadef-debug API=NONE
####################################
  ifneq ($(API),)
    # API is specified on command ine
    ifeq ($(API), CFG80211)
      APICHOICE := FORCE_CFG80211
      $(info CFG80211 API specified in command line)
    else
      ifeq ($(API), WEXT)
        APICHOICE := FORCE_WEXT
        $(info Wireless Extension API specified in command line)
      else
        ifeq ($(API), NONE)
          APICHOICE := FORCE_NONE
          $(info NONE was API specified in command line)
        else
          $(error Unknown Wireless API type.  Must be CFG80211|WEXT|NONE or unspecified)
        endif
      endif
    endif
  else
    # No API was specified on command line, figure it out
    ifeq ($(findstring pcoem,$(TARGET)),)
      #ifndef BCM_STA_CFG80211
        # cfg80211 only supported in hybrid and -cfg80211 target, everything else get wext
	APICHOICE := FORCE_WEXT
        $(info Non hybrid build: Forcing WEXT)
      #endif
    else
      $(info Hybrid build: Autosense WEXT/CFG80211)
      ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6.32),TRUE)
        # if (kernel > 2.6.32) then eligible to use cfg80211
        APICHOICE := PREFER_CFG80211
        $(info CFG80211 API is preferred for this kernel version)
      else
        ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6.17),TRUE)
          APICHOICE := PREFER_WEXT
          $(info Wireless Extension API is preferred for this kernel version)
        else
          APICHOICE := FORCE_WEXT
          $(info Wireless Extension is the only possible API for this kernel version)
        endif
      endif
    endif
  endif

  export APICHOICE

  ifneq ($(filter debug,$(INMAKE)),)
    DEBUG=1
    BCMDBG_MEM=1
  endif

ifeq ($(STBLINUX),1)

else
  ifeq ($(filter mipsel,$(INMAKE)),)
    WLLXNOMIPSEL=1
  endif
endif

  # Get remaining WL config, set flags and files
  include $(WLCONFFILE)
  include $(WLCFGDIR)/wl.mk

  ifeq ($(WLFILES),)
    $(error WLFILES is not defined. Check WLCONFFILE=$(WLCONFFILE))
  endif

  # The wireless API was chosen prior to including the config files.  Now
  # that they have been read, disable API if they were explicitly excluded in config.
  # NOTE: Since cfg80211 is new and not mentioned in cfg files yet, this excludes both WEXT
  # and CFG80211 even though only WEXT was disabled.
  ifeq ($(WLLXIW),0)
    APICHOICE := FORCE_NONE
    export APICHOICE
    $(info After importing config, WLLXIW is 0, so forcing no API)
  endif

  ifeq ($(BCMJTAG),1)
    WLTUNEFILE = wltunable_jtag.h
  endif

# For BCMDBUS feature WL_FW_DECOMP
  ifeq ($(BCMDBUS),1)
    IFLAGS   += -I$(WLAN_SrcBaseA)/shared/zlib
  endif

  # Remove duplicate filenames, pick up flags
  CFILES := $(sort $(WLFILES))
  DFLAGS += $(WLFLAGS)

  # Set some arch-specific definitions
  ifneq ($(filter mipsel,$(INMAKE)),)
    CROSS_COMPILE := mipsel-linux-
    ifneq ($(wildcard $(WLAN_SrcBaseA)/linux/linux),)
      # TODO: Need to investigate who needs these overrides
      # It doesn't seem like a good idea to use a dir iff it exists.
      # Reset LINUXDIR and LINUXVER for cross builds
      LINUXDIR := $(WLAN_SrcBaseA)/linux/linux
      LINUXVER := $(shell $(MAKE) --no-print-directory -s -C $(LINUXDIR) script 'SCRIPT=@echo $$(KERNELRELEASE)')
    endif
  else
    ifneq ($(filter mipseb,$(INMAKE)),)
      CROSS_COMPILE ?= mips-linux-
      DFLAGS += -DIL_BIGENDIAN
      ifneq ($(filter nodebug,$(INMAKE)),)
        LDFLAGS += -S
      endif
    endif
    ifneq ($(filter arm,$(INMAKE)),)
ifeq ($(STBLINUX),1)
CROSS_COMPILE ?= arm-linux-
else
      CROSS_COMPILE ?= armeb-linux-
      DFLAGS += -DIL_BIGENDIAN
endif
      ifneq ($(filter nodebug,$(INMAKE)),)
        LDFLAGS += -S
      endif
    endif
  endif

  # CROSS_COMPILE was decided by arch
  CC := $(CROSS_COMPILE)gcc
  LD := $(CROSS_COMPILE)ld
  NM := $(CROSS_COMPILE)nm
  OBJCOPY := $(CROSS_COMPILE)objcopy
  STRIP   := $(CROSS_COMPILE)strip

  OFILES := $(CFILES:.c=.o) $(WLFILES_O)
  # Add our custom CFLAGS
  CFLAGS += $(DFLAGS) $(IFLAGS) $(WFLAGS) $(MY_C_DEFINES)

  # Divine CFLAGS from kernel source
  ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6),TRUE)
    # the kernel will compile the module itself. So the flags are defined,
    # except for __KERNEL__ !
    ifeq ($(findstring wluser,$(TARGET)),)
      CFLAGS += -D__KERNEL__
			ifeq ($(STBLINUX),1)
				ifneq ($(filter arm,$(INMAKE)),)
					CFLAGS += -D__LINUX_ARM_ARCH__=7
				endif
			endif
    endif
    # we need to export this so the make from the kernel can know it
    WLCFLAGS = $(CFLAGS) -I$(shell pwd)
    export WLCFLAGS
    WLOFILES := $(OFILES)
    export WLOFILES
  else # KERNEL < 2.6
    CFLAGS += $(shell $(MAKE) --no-print-directory -s -C $(LINUXDIR) script BCMINTERNAL= 'SCRIPT=@echo $$(CFLAGS) $$(MODFLAGS) $$(NOSTDINC_FLAGS)')
  endif # KERNEL

endif # INOBJDIR==true

############################################################
# Check if the dongle image is embedded
############################################################

ifeq ($(BCM_DNGL_EMBEDIMAGE),1)
	ifeq ($(EMBED_IMAGE_43236),1)
		DNGL_IMAGE_NAME := 43236-bmac/ag-nodis
	endif
	DNGL_IMAGE_PATH := $(WLAN_SrcBaseA)/dongle/rte/wl/builds/$(DNGL_IMAGE_NAME)
	IFLAGS += -I$(shell pwd)
	IFLAGS += -I$(DNGL_IMAGE_PATH)
endif

# parameter override for SKUs such as media dslcpe etc
ifneq ($(RPC_RETURN_WAIT_TIMEOUT_MSEC),)
	DFLAGS += -DRPC_RETURN_WAIT_TIMEOUT_MSEC=$(RPC_RETURN_WAIT_TIMEOUT_MSEC)
endif
ifneq ($(BRCM_WLAN_IFNAME),)
	DFLAGS += -DBRCM_WLAN_IFNAME=$(BRCM_WLAN_IFNAME)
endif
ifneq ($(EXTENDED_VID_PID),)
	DFLAGS += -DEXTENDED_VID_PID='$(EXTENDED_VID_PID)'
endif
ifeq ($(USB_TRIGGER_DEBUG),1)
	DFLAGS += -DUSB_TRIGGER_DEBUG
endif
ifeq ($(DISABLE_HT_RATE_FOR_WEP_TKIP),1)
	DFLAGS += -DDISABLE_HT_RATE_FOR_WEP_TKIP
endif
ifeq ($(AP_KEEP_ALIVE),1)
	DFLAGS += -DAP_KEEP_ALIVE
endif
ifneq ($(AP_KEEP_ALIVE_INTERVAL),)
	DFLAGS += -DAP_KEEP_ALIVE_INTERVAL=$(AP_KEEP_ALIVE_INTERVAL)
endif
ifeq ($(DEFAULT_EAPVER_AP),1)
	DFLAGS += -DDEFAULT_EAPVER_AP
endif
ifeq ($(USB_DISABLE_INT_EP),1)
	DFLAGS += -DUSB_DISABLE_INT_EP
endif

ifeq ($(BCM_STA_CFG80211),1)
	DFLAGS += -DWLP2P
#	DFLAGS += -DBCMPCIE
#	DFLAGS += -DBCMDONGLEHOST
	DFLAGS += -DUSE_CFG80211
	DFLAGS += -DWL_CFG80211_STA_EVENT
	DFLAGS += -DWL_CFG80211_GON_COLLISION
	DFLAGS += -DWL_CFG80211
	DFLAGS += -DWL_CFG80211_SYNC_GON
	DFLAGS += -DVSDB

# WL_ENABLE_P2P_IF and WL_CFG80211_P2P_DEV_IF are mutually exclusive.
# Define WL_CFG80211_P2P_DEV_IF if kernel version is greater than 3.8.0
ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),3.8.0),TRUE)
	DFLAGS += -DWL_CFG80211_P2P_DEV_IF
else
	DFLAGS += -DWL_ENABLE_P2P_IF
endif
endif
########################################################
# TARGET is one of completed $(TARGETS)
########################################################

$(TARGETS):
	+$(MAKE) TARGET=$@ objdir

objdir:
	@echo "Making objdir: TARGET=$(TARGET), level $(MAKELEVEL) kernel $(BCM_KVERSIONSTRING)"
	install -d $(OBJDIR)
	# Generate WL config file from WLCONFS tokens
	cat $(foreach config,$(WLCONFS),$(WLCFGDIR)/$(WLCFG_PREFIX)$(config)) > \
		$(OBJDIR)/$(WLCONFFILE)
ifdef BCM_EXTERNAL_MODULE_REQUIRED
	# Start building external modules
	+$(MAKE) -r $(if $(JOBS),-j $(JOBS)) -C $(OBJDIR) -f $(WLAN_SrcBaseA)/wl/linux/Makefile \
		INOBJDIR=true ext_modules
	cp $(WLAN_SrcBaseA)/linuxdev/obj-$(BCM_EXTERNAL_MODULE_NAME)-$(LINUXVER)-$(WLARCH)/Module.symvers $(OBJDIR)/Module.symvers.$(BCM_EXTERNAL_MODULE_NAME)
	cp $(WLAN_SrcBaseA)/linuxdev/obj-$(BCM_EXTERNAL_MODULE_NAME)-$(LINUXVER)-$(WLARCH)/$(BCM_EXTERNAL_MODULE_NAME).ko $(OBJDIR)
endif # BCM_EXTERNAL_MODULE_REQUIRED
	  # kernel-mode driver target requested
	  # Build dependencies
	  +$(MAKE) -r $(if $(JOBS),-j $(JOBS)) -C $(OBJDIR) -f $(WLAN_SrcBaseA)/wl/linux/Makefile \
		INOBJDIR=true dep
	  # Build modules
	  +$(MAKE) -r $(if $(JOBS),-j $(JOBS)) -C $(OBJDIR) -f $(WLAN_SrcBaseA)/wl/linux/Makefile \
		INOBJDIR=true modules

wlconf.h: $(WLCFGDIR)/$(WLTUNEFILE) force
	[ ! -f $@ ] || chmod +w $@
	@echo "check and update config file"
	cp $< wltemp
	$(UPDATESH) wltemp $@

# These following targets are called by release makefile (linux-wl.mk)
# These are intended to provide kernel info to a list of
# wl-files and flags given a wl target

# Show current $(TARGET) kernel info
showkernel:
	@echo "=== Current Linux Kernel Info ==="
	@echo "LINUXVER=$(LINUXVER)"
	@echo "LINUXDIR=$(LINUXDIR)"
	@echo "BCM_KVERSIONSTRING=$(BCM_KVERSIONSTRING)"
	@echo "BCM_KEXTRAVERSION=$(BCM_KEXTRAVERSION)"
	@echo "================================="

# Show current $(TARGET) wl flags and files
showwlconf:
	@echo "----- TARGET $(TARGET) FLAGS -----"; \
	echo "* LINUXVER   = $(LINUXVER)";          \
	echo "* LINUXDIR   = $(LINUXDIR)";          \
	echo "* WLCONFS    = $(WLCONFS)";           \
	echo "* WLTUNEFILE = $(WLTUNEFILE)";        \
	echo "* WLFILES    = $(WLFILES)";           \
	echo "* WLFLAGS    = $(WLFLAGS)";           \
	echo "* WLCFLAGS   = $(WLCFLAGS)";          \
	echo "----------------------------------"

# Following targets act as API's to this makefile by release makefile
# to get a list of wl files or flags given a custom target.
showwlfiles:
	@echo "$(WLFILES)"

showwlfiles_src:
	@echo "$(WLFILES_SRC)"

showwlflags:
	@echo "$(WLFLAGS)"

# Show compiler version, for the current target build
showenv:
	@echo "CC = $(CC) (ver=`$(CC) -dumpversion`; host=`hostname`; processor=`uname -m`)"

# Target to show embeddable firmware image names
show_dongle_images:
	@echo "$(EMBED_DONGLE_IMAGES)"

# Target to show embeddable firmware chiprevs
show_dongle_chiprevs:
	@echo "$(EMBED_DONGLE_CHIPREVS)"

ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6),TRUE)
ext_modules:
	@echo "Building external kernel module for $(BCM_KVERSIONSTRING)"
	$(MAKE) -C $(WLAN_SrcBaseR)/linuxdev \
		$(if $(CROSS_COMPILE),CROSS_COMPILE=$(CROSS_COMPILE)) \
		$(if $(GCCVER),GCCVER=$(GCCVER)) \
		LINUXVER=$(LINUXVER) TARGETARCH=$(WLARCH) \
		DFLAGS_IN="$(DFLAGS) $(CFLAGS_EMBED_IMAGE)" \
		WFLAGS_IN="$(WFLAGS)" \
		IFLAGS_IN="$(IFLAGS_EMBED_IMAGE)"

modules: $(OFILES)
	test -r ./Makefile || ln -s $(WLAN_SrcBaseR)/wl/linux/makefile.26 ./Makefile
# Electric emake will not be able to tee to a log file,
# so do not update make to be $(MAKE) for kernelbuilds.
	+{ set -o pipefail 2> /dev/null || true; }; \
	$(strip make -r -C $(LINUX_OUT) M=$(shell pwd) $(if $(VERBOSE),V=1) \
		$(WLAN_ShellDebugSHELL) \
		KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS) \
		modules 2>&1 | tee ,BUILD_LOG)
        # For most module builds (except for usermode and sdio) exit on
        # undefined symbols as driver will not be useable anyway
ifeq ($(WLUMK)$(WLNINTENDO),)
	@if grep -i -q "\*\*\* Warning: .* undefined" ,BUILD_LOG; then \
		echo >&2 "ERROR: "; \
		echo >&2 "ERROR: Following unresolved symbols found"; \
		echo >&2 "ERROR: This kernel module may not load on targets"; \
		echo >&2 "======================================="; \
		grep >&2 "\*\*\* Warning: .* undefined" ,BUILD_LOG; \
		echo >&2 "======================================="; \
		exit 1; \
	fi
endif # WLUMK|WLNINTENDO

else # KERNEL < 2.6

modules: $(MODULES)

endif # KERNEL

# Dependencies
dep: showenv wlconf.h $(if $(SHOWWLCONF)$(VERBOSE),showwlconf)
$(foreach file,$(CFILES),.$(file).depend): wlconf.h
ifneq ($(findstring x86_64,$(LINUXVER)),x86_64)
dep: $(foreach file,$(CFILES),.$(file).depend)
endif
#dep: $(foreach file,$(CFILES),.$(file).preprocessor)

.%.c.depend: %.c
	@echo "Making $@"
	@$(CC) $(CFLAGS) -M $< > $@

.%.c.preprocessor: %.c
	$(CC) -E -C $(CFLAGS) $< > $@
#	$(CC) -S $(CFLAGS) $<

# Update the CLM database C code from XML inputs if present.
CLM_TYPE := 43xx_pcoem
$(call WLAN_GenClmCompilerRule,$(CURDIR),$(WLAN_SrcBaseR))

vpath %.c $(addprefix $(WLAN_SrcBaseR)/,wl/sys wl/phy wl/linux shared bcmcrypto \
	   shared/nvram bcmsdio/sys $(if $(WL_FW_DECOMP),shared/zlib)) \
	   $(WLAN_StdSrcDirsR) $(WLAN_ComponentSrcDirsR)

ifeq ($(call wlan_version_ge,$(BCM_KVERSIONSTRING),2.6),TRUE)
# When make is used from 2.6, vpath doesn't help so we need to link the files
# unless they're generated.

%.o: %.c
	$(strip \
	$(if $(filter-out $(addprefix %/,$(GENERATED_SOURCES)),$(<F)),test -r $(<F) || ln -s $< .) \
	$(if $(findstring wluser,$(TARGET)), && $(CC) $(CFLAGS) -g -c -o $@ $(<F)))

%.o: %.o_shipped
	test -r ./$(<F) || ln -s $< .

else # KERNEL < 2.6

# THE module for Linux 2.4
wl.o: $(OFILES)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@( \
	echo 'ifneq ($$(CFLAGS),$(CFLAGS))' ; \
	echo '$@: force' ; \
	echo 'endif' ; \
	) > .$*.c.flags

endif # KERNEL

force:

-include $(wildcard .*.depend)
-include $(wildcard .*.flags)

.PHONY: native all objdir dep modules ext_modules clean force $(TARGETS:%=%-$(LINUXVER))
.PHONY: default $(DEBUGS) $(ARCHES)

endif # TARGETS
