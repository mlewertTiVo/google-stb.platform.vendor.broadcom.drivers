#
#
# Makefile to generate XP,Vista source tree based on wlconfigs flags and then
# build/prefast_verify/sign x86 and amd64 xp and vista drivers.
#
# This also builds Win8 NIC driver
#
# Notes:
#  - This is a standalone makefile to build xp and vista drivers using WDK toolset
#  - For a list of supported targets, run "gmake help"
#  - For other details, please refer to HowToCompileThings twiki
#
# $Id: builddriver.mk 491103 2014-07-15 01:25:01Z $

WLAN_ComponentsInUse := bcmwifi clm ppr olpc keymgmt iocv dump phymods ndis
include ../../../makefiles/WLAN_Common.mk
SRCBASE        := $(WLAN_SrcBaseR)

SHELL          := bash.exe
empty          :=
space          := $(empty) $(empty)
comma          := $(empty),$(empty)
compose         = $(subst $(space),$(strip $1),$(strip $2))
NULL           := /dev/null
MYTAG          := $(if $(TAG),$(TAG),NIGHTLY)
TITLE          := $(MYTAG)$(if $(BRAND),_$(BRAND))
ACTION          = $(if $(PREFAST),prefast,build)
ifndef PARTIAL_BUILD
  export WDKFLAGS?= -Z
endif

ifneq ($(VCTHREADS),)
    # enable parallel CL.exe in VS Projects
    export CL:=/MP
endif

WLTUNEFILE_DEFAULT              := wltunable_sample.h
WLCONF_GEN                      := true
WLCFGDIR                        := $(SRCBASE)/wl/config

#Start: Per WINOS specific settings
XPBLDDIR                        ?= $(ObjPfx)buildxp
XPHIGHBLDDIR                    ?= $(ObjPfx)buildxphigh
XPHIGHSDBLDDIR                  ?= $(ObjPfx)buildxphighsd
VISTABLDDIR                     ?= $(ObjPfx)buildvista
VISTAHIGHBLDDIR                 ?= $(ObjPfx)buildvistahigh
WIN7BLDDIR                      ?= $(ObjPfx)buildwin7
WIN7HIGHBLDDIR                  ?= $(ObjPfx)buildwin7high
WIN7HIGHSDBLDDIR                ?= $(ObjPfx)buildwin7highsd
WIN8XBLDDIR                     ?= $(SRCBASE)/wl/sys/wdm
WIN8XHIGHBLDDIR                 ?= $(SRCBASE)/wl/sys/wdm
WIN8XHIGHSDBLDDIR               ?= $(SRCBASE)/wl/sys/wdm

CURWINOS_build_xp_driver        := WINXP
CURWINOS_prefast_xp_driver      := WINXP
CURWINOS_build_xp_high_driver   := WINXPHIGH
CURWINOS_build_xp_highsd_driver := WINXPHIGHSD
CURWINOS_build_win7_driver      := WIN7
CURWINOS_build_win7_high_driver := WIN7HIGH
CURWINOS_build_win7_highsd_driver  := WIN7HIGHSD
CURWINOS_build_win8x_driver        := WIN8X
CURWINOS_build_win8x_high_driver   := WIN8XHIGH
CURWINOS_build_win8x_highsd_driver := WIN8XHIGHSD

CURBLDDIR_WINXP                 := $(XPBLDDIR)
CURBLDDIR_WINXPHIGH             := $(XPHIGHBLDDIR)
CURBLDDIR_WINXPHIGHSD           := $(XPHIGHSDBLDDIR)
CURBLDDIR_WIN7                  := $(WIN7BLDDIR)
CURBLDDIR_WIN7HIGH              := $(WIN7HIGHBLDDIR)
CURBLDDIR_WIN7HIGHSD            := $(WIN7HIGHSDBLDDIR)
CURBLDDIR_WIN8X                 := $(WIN8XBLDDIR)
CURBLDDIR_WIN8XHIGH             := $(WIN8XHIGHBLDDIR)
CURBLDDIR_WIN8XHIGHSD           := $(WIN8XHIGHSDBLDDIR)

# xp nic driver
WL_CONF_WINXP                   := wlconfig_win_nic_xp
WLTUNEFILE_WINXP                := wltunable_win.h
BCMDRVPFX_x86_WINXP             := bcmwl5
BCMDRVPFX_x64_WINXP             := bcmwl564

# xp usb bmac driver
WL_CONF_WINXPHIGH               := wlconfig_win_highbmac_usb_xp
BCMDRVPFX_x86_WINXPHIGH         := bcmwlhigh5
BCMDRVPFX_x64_WINXPHIGH         := bcmwlhigh564

# xp sdio bmac driver
WL_CONF_WINXPHIGHSD             := wlconfig_win_highbmac_sd_xp
BCMDRVPFX_x86_WINXPHIGHSD       := bcmwlhighsd5
BCMDRVPFX_x64_WINXPHIGHSD       := bcmwlhighsd564

# win7 nic driver
WL_CONF_WIN7                    := wlconfig_win_nic_win7
WLTUNEFILE_WIN7                 := wltunable_win.h
BCMDRVPFX_x86_WIN7              := bcmwl6
BCMDRVPFX_x64_WIN7              := bcmwl664

# win7 usb bmac driver
WL_CONF_WIN7HIGH                := wlconfig_win_highbmac_usb_win7
BCMDRVPFX_x86_WIN7HIGH          := bcmwlhigh6
BCMDRVPFX_x64_WIN7HIGH          := bcmwlhigh664

# win7 sdio bmac driver
WL_CONF_WIN7HIGHSD              := wlconfig_win_highbmac_sdio_win7
BCMDRVPFX_x86_WIN7HIGHSD        := bcmwlhighsd6
BCMDRVPFX_x64_WIN7HIGHSD        := bcmwlhighsd664

# win8x nic driver
WLTUNEFILE_WIN8X                := wltunable_win.h
BCMDRVPFX_x86_WIN8X             := bcmwl63
BCMDRVPFX_x64_WIN8X             := bcmwl63a

# win8 usb bmac driver
WLTUNEFILE_WIN8XHIGH            := wltunable_rte_bmac_usb_high.h
BCMDRVPFX_x86_WIN8XHIGH         := bcmwlhigh63
BCMDRVPFX_x64_WIN8XHIGH         := bcmwlhigh63a

# win8x sdio bmac driver
BCMDRVPFX_x86_WIN8XHIGHSD       := bcmwlhighsd63
BCMDRVPFX_x64_WIN8XHIGHSD       := bcmwlhighsd63a

# WINVISTA driver names preserved for signing purposes
BCMDRVPFX_x86_WINVISTA          := $(BCMDRVPFX_x86_WIN7)
BCMDRVPFX_x64_WINVISTA          := $(BCMDRVPFX_x64_WIN7)
BCMDRVPFX_x86_WINVISTAHIGH      := $(BCMDRVPFX_x86_WIN7HIGH)
BCMDRVPFX_x64_WINVISTAHIGH      := $(BCMDRVPFX_x64_WIN7HIGH)
#End: Per WINOS specific settings


#
# Embeddable firmware images for USB BMAC
  # External release firmware images
  DNGL_IMAGE_HIGH          := 43236b-bmac/ag-nodis
  DNGL_IMAGE_HIGH          += 43526a-bmac/ag-nodis
  DNGL_IMAGE_HIGH          += 43526b-bmac/ag-nodis
  DNGL_IMAGE_HIGH          += 4350c1-bmac/ag-nodis
  DNGL_IMAGE_HIGH          += 43569a0-bmac/ag-nodis
  DNGL_IMAGE_HIGH          += 43143b0-bmac/g-nodis


#
# Embeddable firmware images for SDIO BMAC
#
  # External release firmware images
  DNGL_IMAGE_WINXPHIGHSD        :=
  DNGL_IMAGE_WIN7HIGHSD         :=


DNGLBLDDIR := $(SRCBASE)/../build/dongle

# Developer can override default list of firmware specs, by
# specifying make ... FIRMWARE=<full-firmware-image-name>
ifdef FIRMWARE
  DNGL_IMAGE_HIGH  := $(FIRMWARE)
  DNGL_IMAGE_WINXPHIGHSD:= $(FIRMWARE)
  DNGL_IMAGE_WIN7HIGHSD := $(FIRMWARE)
  FIRMWARE_DIR           = $(DNGLBLDDIR)/$(FIRMWARE)
  $(warning Verifying if FIRMWARE_DIR=$(FIRMWARE_DIR) dir exists)
  ifeq ($(shell if [ -d "$(FIRMWARE_DIR)" ]; then echo EXISTS; fi),)
       $(error FIRMWARE $(FIRMWARE_DIR) dir missing)
  endif # FIRMWARE
endif

# This list ALL_DNGL_IMAGES is queried by release makefile
ALL_DNGL_IMAGES                 := $(DNGL_IMAGE_HIGH)
ALL_DNGL_IMAGES                 += $(DNGL_IMAGE_WINXPHIGHSD)
ALL_DNGL_IMAGES                 += $(DNGL_IMAGE_WIN7HIGHSD)
ALL_DNGL_IMAGES                 += $(DNGL_IMAGE_WINNIC)
ALL_DNGL_IMAGES                 += ${EMBED_DONGLE_IMAGES}

# If the request is not for xp high driver, mark xp high image to null
ifeq ($(filter %XPHIGH, $(WINOS)),)
ifeq ($(filter %7HIGH, $(WINOS)),)
  DNGL_IMAGE_HIGH          :=
endif # XP
endif # WIN7

# If the request is not for high sdio driver, mark xp high sd image to null
ifeq ($(filter %XPHIGHSD, $(WINOS)),)
  DNGL_IMAGE_WINXPHIGHSD        :=
endif # WINOS

# If the request is not for high sdio driver, mark win7 high sd image to null
ifeq ($(filter %7HIGHSD, $(WINOS)),)
  DNGL_IMAGE_WIN7HIGHSD        :=
endif # WINOS

# Pass firmware specific C macros for driver compilation
DNGL_LISTS=$(DNGL_IMAGE_HIGH)$(DNGL_IMAGE_WINXPHIGHSD)$(DNGL_IMAGE_WIN7HIGHSD)$(FIRMWARE)$(DNGL_IMAGE_WINNIC)
DNGL_C_DEFINES += $(if $(findstring 43143b0,$(DNGL_LISTS)),-DEMBED_IMAGE_43143b0)
DNGL_C_DEFINES += $(if $(findstring 43236b,$(DNGL_LISTS)),-DEMBED_IMAGE_43236b)
DNGL_C_DEFINES += $(if $(findstring 43526a,$(DNGL_LISTS)),-DEMBED_IMAGE_43526a)
DNGL_C_DEFINES += $(if $(findstring 43526b,$(DNGL_LISTS)),-DEMBED_IMAGE_43526b)
DNGL_C_DEFINES += $(if $(findstring 4350b1,$(DNGL_LISTS)),-DEMBED_IMAGE_4350b1 -DBCM_USB30)
DNGL_C_DEFINES += $(if $(findstring 4350c1,$(DNGL_LISTS)),-DEMBED_IMAGE_4350c1 -DBCM_USB30)
DNGL_C_DEFINES += $(if $(findstring 43569a0,$(DNGL_LISTS)),-DEMBED_IMAGE_43569a0 -DBCM_USB30)
# Derive Current WINOS from current target name
CURWINOS                         = $(CURWINOS_$(@))
# Derive Current WINOS from current target name
CURBLDDIR                        = $(CURBLDDIR_$(CURWINOS))

# Final derived variables from above variants (Default WINOS is WIN7)
# WIN8_WDK_VER name preserved for backward compat with brand makes, not WIN8X here.
export WDK_VER         ?= 7600
export WIN8_WDK_VER    ?= 9600
export VS_VER          ?= 2010
export WINOS           ?= $(if $(CURWINOS),$(CURWINOS),WIN7)
export BLDDIR          ?= $(if $(CURBLDDIR),$(CURBLDDIR),$(CURBLDDIR_$(WINOS)))
export CONFIG_WL_CONF  ?= $(WL_CONF_$(WINOS))
export WLTUNEFILE      ?= $(if $(WLTUNEFILE_$(WINOS)),$(WLTUNEFILE_$(WINOS)),$(WLTUNEFILE_DEFAULT))
export BCMDRVPFX_x86   ?= $(BCMDRVPFX_x86_$(WINOS))
export BCMDRVPFX_x64   ?= $(BCMDRVPFX_x64_$(WINOS))
export BCMDRVPFX_amd64 ?= $(BCMDRVPFX_x64)
export WLCONF_GEN

# Co-Installer KMDF versions differ in different WDK distributions
WDK_KMDF_VERSION           := 1
ifeq ($(WDK_VER),6000)
  WDK_KMDF_VERSION         := 1.5
endif
ifeq ($(WDK_VER),6001)
  WDK_KMDF_VERSION         := 1.7
endif
ifneq ($(filter 7% 8%,$(WDK_VER)),)
  WDK_KMDF_VERSION         := 1.9
  export WDK_OACR          ?= no_oacr
endif
ifneq ($(filter 7% 8%,$(WDK_VER)),)
  WDK_KMDF_VERSION_MAJOR   := 1
  WDK_KMDF_VERSION_MINOR   := 9
  export WDK_OACR          ?= no_oacr
endif

BLDDIRDOS := $(subst /,\,$(BLDDIR))
SOURCES   := $(BLDDIR)/sources
CWDIR     := $(shell cygpath -m -a "$(BLDDIR)" 2> $(NULL))

## Following error condition very rarely fires
ifeq ($(findstring //,$(CWDIR)),//)
     $(error "ERROR: Can't use UNC style paths for 'pwd' command ($(CWDIR))")
     $(error "ERROR: Change to dir non-UNC (i.e. non //) path for 'pwd' cmd")
endif

## For now if BRAND is not specified it is assumed to be a developer build
ifeq ($(BRAND),)
  DEVBUILD     := 1
endif

ifneq ($(BRAND),)
  export VERBOSE=1
endif

PREPWDK        := perl prepwdkdir.pl
RETRYCMD       ?= $(firstword \
                  $(wildcard \
                     C:/tools/build/bcmretrycmd.exe \
                     $(WLAN_WINPFX)/projects/hnd_software/gallery/src/tools/build/bcmretrycmd.exe \
                  ))
WDKDIR         := $(strip $(if $(BASEDIR),$(BASEDIR),$(subst /,\,$(firstword $(wildcard \
			C:/WinDDK/$(WDK_VER) C:/WINDDK/$(WDK_VER).0.0 \
			D:/WinDDK/$(WDK_VER) D:/WinDDK/$(WDK_VER).0.0 \
			E:/WinDDK/$(WDK_VER) E:/WinDDK/$(WDK_VER).0.0 \
			C:/tools/msdev/$(WDK_VER)wdksdk \
			C:/tools/msdev/$(WDK_VER)wdk \
			C:/tools/mdev/wdk$(WDK_VER) \
			C:/tools/mdev/wdk$(WDK_VER).0.0 \
			D:/tools/msdev/$(WDK_VER)wdksdk \
			D:/tools/msdev/$(WDK_VER)wdk \
			$(WLAN_WINPFX)/projects/hnd/tools/win/msdev/$(WDK_VER)wdk)))))
WDKDIR_UX      := $(subst \,/,$(WDKDIR))
VSDIR          := $(strip $(if $(VSINSTALLDIR),$(VSINSTALLDIR),$(subst /,\,$(firstword $(wildcard \
			C:/tools/msdev/VS$(VS_VER) \
			D:/tools/msdev/VS$(VS_VER) \
			$(WLAN_WINPFX)/projects/hnd/tools/win/msdev/VS$(VS_VER))))))
VCDIR          := $(VSDIR)/VC
VSDIR_UX       := $(subst \,/,$(VSDIR))
VCDIR_UX       := $(subst \,/,$(VCDIR))
DOSCMD         := $(subst \,/,$(COMSPEC))
# In clustered environments, if a preconfigured dir is not accessible use $(WLAN_WINPFX)
BASEDIR         = $(shell if [ -d "$(WDKDIR)" ]; then echo "$(WDKDIR)"; else echo $(WLAN_WINPFX)/projects/hnd/tools/win/msdev/$(WDK_VER)wdk; fi)
BASEDIR_UX     := $(subst \,/,$(BASEDIR))


# hardcoded overrides not scalable but makefile will be rebuilt so use ifeq for now
ifneq ($(filter WIN8X%,$(WINOS)),)
WIN8X_VSDIR :=$(strip $(firstword $(wildcard $(foreach suffix,2013 2012 2010,$(foreach prefix,c:/tools/msdev/VS d:/tools/msdev/VS,$(strip $(prefix))$(strip $(suffix)))))))
else
WIN8X_VSDIR :=$(strip $(firstword $(wildcard $(foreach suffix,2012 2010,$(foreach prefix,c:/tools/msdev/VS d:/tools/msdev/VS,$(strip $(prefix))$(strip $(suffix)))))))
endif

WIN8X_WDKDIR :=$(strip $(firstword $(wildcard $(foreach prefix,c:/tools/ d:/tools/ $(WLAN_WINPFX)/projects/hnd/tools/win/,$(foreach suffix,wdk wdksdk,$(strip $(prefix))msdev/$(WIN8_WDK_VER)$(strip $(suffix)))))))

#disabled# # When new MS tools are all deployed across all servers
#disabled# # enable this
#disabled# ifeq ($(VSDIR),)
#disabled# 	$(error ERROR: Missing Visual Studio $VS_VER Installation)
#disabled# endif

# WDK produces a mix of x64 and amd64 folder and logfile names by default
# e.g: setenv and signtool use x64 and build.exe uses amd64. Default values
#      are not overriden. So BLDOUT_ARCH_<arch> shows output log/dir name
ifneq ($(filter WIN8X%,$(WINOS)),)
  BLDOUT_ARCH_x86 := Win32
  BLDOUT_ARCH_x64 := x64
else
  BLDOUT_ARCH_x86 := x86
  BLDOUT_ARCH_x64 := amd64
endif
ifneq ($(filter 9%,$(WIN8_WDK_VER)),)
  BUILD_ARCHS  ?= x86 x64
else ifeq ($(filter 6001% 7% 8%,$(WDK_VER)),)
  BUILD_ARCHS  ?= x86 amd64
else
  BUILD_ARCHS  ?= x86 x64
endif
BUILD_TYPES    ?= checked free
#No Inbox BUILD_TYPES    ?= checked free $(if $(filter WIN8X%,${WINOS}),checkedexternal)
BUILD_VARIANTS ?= $(foreach arch,$(BUILD_ARCHS),$(foreach type,$(BUILD_TYPES),$(ACTION)_$(arch)_$(type)))
BLD_TEMPLATE   := .temp_wdk_build_template.bat
MSBLD_TEMPLATE := .temp_msbuild_template.bat
export PATH    := /usr/bin:/bin:$(PATH):$(shell cygpath -u $(subst \,/,$(BASEDIR))/bin/SelfSign):$(shell cygpath -u $(subst \,/,$(BASEDIR))/bin/x86):

# SoftICE debug option flags
NTICE_FLAGS    := -translate:always,source,package

# Vars used for signing vista driver after build
OEM            ?=bcm

OTHER_INC_DIRS :=
INC_DIRS       :=

## Prerelease contains microsoft wdk/ddk pre-release header files
ifeq ($(filter 6001% 7% 8%,$(WDK_VER)),)
  ifneq ($(filter WIN7%,$(WINOS)),)
    INC_DIRS         += $(SRCBASE)/include/prerelease
    PRERELEASE_FILES := $(SRCBASE)/include/prerelease/windot11.h
  endif # WIN7
endif # WDK_VER

ifneq ($(DNGL_IMAGE_$(WINOS)),)
INC_DIRS += $(DNGL_IMAGE_$(WINOS):%=$(DNGLBLDDIR)/%)
endif
ifneq ($(DNGL_IMAGE_HIGH),)
INC_DIRS += $(DNGL_IMAGE_HIGH:%=$(DNGLBLDDIR)/%)
endif

INC_DIRS  += $(FIRMWARE:%=$(DNGLBLDDIR)/%)

INC_DIRS  += $(WLAN_ComponentIncDirsR)
INC_DIRS  += $(WLAN_IncDirsR)
INC_DIRS  += $(SRCBASE)/wl/shim/include
INC_DIRS  += $(SRCBASE)/wl/sys/wdm
ifneq ($(WL_FW_DECOMP)),)
INC_DIRS  += $(SRCBASE)/shared/zlib
endif
ifneq ($(filter %XPHIGHSD %7HIGHSD, $(WINOS)),)
  INC_DIRS+= $(SRCBASE)/bcmsdio/sys
endif

ifneq ($(filter WIN7%,$(WINOS)),)
  INC_DIRS+= $(SRCBASE)/wl/cpl/components/sup/ihv/IHVService/inc
  ifneq ($(BCMCCX),0)
    # If you update CCX_SDK_VER here, you need to do similar change for
    # ihv project in src/makefiles/msvs_defs.mk
    CCX_SDK_VER    ?= 1.1.13
    OTHER_INC_DIRS += $(WLAN_WINPFX)/projects/hnd/restrictedTools/CCX_SDK/$(CCX_SDK_VER)/inc
  endif # BCMCCX=0
endif # WINOS

ifeq ($(filter WIN7%,$(WINOS)),)
  ## Include dirs outside of cvs source tree
  FIPS_DIR       := $(firstword $(wildcard C:/tools/src/fips/funk D:/tools/src/fips/funk $(WLAN_WINPFX)/projects/hnd/tools/fips/funk))
  FIPS_DIR       := $(shell if [ -d "$(FIPS_DIR)" ]; then echo "$(FIPS_DIR)"; else echo $(WLAN_WINPFX)/projects/hnd/tools/fips/funk; fi)
  FIPS_INC_DIR    = $(FIPS_DIR)/inc
endif # WINOS != WIN7

ifdef DEVBUILD
  BSRCBASE  := $(shell cygpath -m $(SRCBASE))
  ifeq ($(filter 6001% 7% 8%,$(WDK_VER)),)
    ifneq ($(filter WIN7%,$(WINOS)),)
      BINC_DIRS := $(subst $(SRCBASE),$(BSRCBASE),$(SRCBASE)/include/prerelease)
    endif # WIN7
  endif # WDK_VER
  BINC_DIRS += $(BASEDIR_UX)/inc/api
# local wlconf.h (from build[xp,vista]high/src/include should precede other wlconf.h
#  e.g HIGH driver also include build/dongle/xx/, which has wlconf for dongle
  BINC_DIRS += src/include
  BINC_DIRS += $(subst $(SRCBASE),$(BSRCBASE),$(INC_DIRS))
  BINC_DIRS := $(subst //,/,$(BINC_DIRS))
  ifeq ($(filter WIN7%,$(WINOS)),)
    OTHER_INC_DIRS += $(FIPS_INC_DIR)
  endif # WINOS != WIN7
else
  ifeq ($(filter WIN7%,$(WINOS)),)
    INC_DIRS       += $(SRCBASE)/wl/sys/fips/funk/inc
  endif # WINOS != WIN7
endif # DEVBUILD

EXTRA_C_DEFINES   += -DNDIS -DNDIS_MINIPORT_DRIVER -DNDIS_WDM -DWDM -DDELTASTATS
EXTRA_C_DEFINES   += -DBCMDRIVER
EXTRA_C_DEFINES   += $(DNGL_C_DEFINES)


EXTRA_C_DEFINES   += -DNDIS_DMAWAR
EXTRA_C_DEFINES   += $(if $(BRAND),-DBRAND=$(BRAND))
WL_CHK_C_DEFINES  := -DBCMDBG -DBCMINTERNAL -DBCMDBG_MEM

ifneq ($(filter WIN7%,$(WINOS)),)
  EXTRA_C_DEFINES += -DNDIS60_MINIPORT=1 -DNDIS620_MINIPORT=1 -DNDIS620 -DWIN7
endif

ifneq ($(filter WINXP%,$(WINOS)),)
    EXTRA_C_DEFINES += -DNDIS51_MINIPORT -DNDIS51
    MSX86_C_DEFINES := -UWINVER -DWINVER=0x0500
endif

ifneq ($(filter WINXPHIGHSD% %7HIGHSD,$(WINOS)),)
  EXTRA_C_DEFINES += -DSDALIGN=64 -DMAX_HDR_READ=64 -DSD_FIRSTREAD=64
  EXTRA_C_DEFINES += -DSDIO_BMAC
endif

ifneq ($(CONFIG_WL_CONF),)
  include $(WLCFGDIR)/$(CONFIG_WL_CONF)
  include $(WLCFGDIR)/wl.mk
endif

## Aggregate individual flags/defines
C_DEFINES  = $(WLFLAGS) $(EXTRA_C_DEFINES) $(MY_C_DEFINES)
C_SOURCES  = $(WLFILES_SRC) $(EXTRA_C_SOURCES)
INC_FLAGS  = $(INC_DIRS:%=-I%)

ifdef SIGN_DIR
  BCMCAT_x86_NIC             := bcm43xx.cat
  BCMCAT_x64_NIC             := bcm43xx64.cat
  BCMCAT_x86_HIGH            := bcmh43xx.cat
  BCMCAT_x64_HIGH            := bcmh43xx64.cat
  BCMCAT_x86_HIGHSD          := bcmhsd43xx86.cat
  BCMCAT_x64_HIGHSD          := bcmhsd43xx64.cat

  BCMCAT_x86_WINXP           := $(BCMCAT_x86_NIC)
  BCMCAT_x64_WINXP           := $(BCMCAT_x64_NIC)
  BCMCAT_x86_WINXPHIGH       := $(BCMCAT_x86_HIGH)
  BCMCAT_x64_WINXPHIGH       := $(BCMCAT_x64_HIGH)
  BCMCAT_x86_WINXPHIGHSD     := $(BCMCAT_x86_HIGHSD)
  BCMCAT_x64_WINXPHIGHSD     := $(BCMCAT_x64_HIGHSD)

  BCMCAT_x86_WIN7            := $(BCMCAT_x86_NIC)
  BCMCAT_x64_WIN7            := $(BCMCAT_x64_NIC)
  BCMCAT_x86_WIN7HIGH        := $(BCMCAT_x86_HIGH)
  BCMCAT_x64_WIN7HIGH        := $(BCMCAT_x64_HIGH)
  BCMCAT_x86_WIN7HIGHSD      := $(BCMCAT_x86_HIGHSD)
  BCMCAT_x64_WIN7HIGHSD      := $(BCMCAT_x64_HIGHSD)

  BCMCAT_x86_WIN8X         := $(BCMCAT_x86_NIC)
  BCMCAT_x64_WIN8X         := $(BCMCAT_x64_NIC)
  BCMCAT_x86_WIN8XHIGH     := $(BCMCAT_x86_HIGH)
  BCMCAT_x64_WIN8XHIGH     := $(BCMCAT_x64_HIGH)
  BCMCAT_x86_WIN8XHIGHSD   := $(BCMCAT_x86_HIGHSD)
  BCMCAT_x64_WIN8XHIGHSD   := $(BCMCAT_x64_HIGHSD)

  BCMCAT_x86_WINVISTA        := $(BCMCAT_x86_NIC)
  BCMCAT_x64_WINVISTA        := $(BCMCAT_x64_NIC)
  BCMCAT_x86_WINVISTAHIGH    := $(BCMCAT_x86_HIGH)
  BCMCAT_x64_WINVISTAHIGH    := $(BCMCAT_x64_HIGH)

 #TIMESTAMP_URL    := "http://time.certum.pl/" #Alternate timestamp server
  TIMESTAMP_URL    := "http://timestamp.verisign.com/scripts/timstamp.dll"
 #CURL             := $(WLAN_WINPFX)/projects/hnd/tools/win/utils/curl.exe
  CURL             := /bin/curl
  SIGN_LOG         := $(CWDIR)/../signtool.log
  RELNUM           ?= $(shell date '+%Y.%-m.%-d.%H%M')
  RELDATE          ?= $(shell date '+%m/%d/%Y')
  MS_CROSSCERT     ?=$(WLAN_WINPFX)/projects/hnd/tools/win/verisign/driver/MSCV-VSClass3.cer
  # By default sign only 64bit driver, unless SIGN_ARCH is specified
  SIGN_ARCH        ?=amd64
  SIGN_ARCH        := $(subst amd64,X64,$(SIGN_ARCH))
  SIGN_DRIVER      ?=$(BCMDRVPFX_x64).sys
  SIGN_DRIVERCAT   ?=$(BCMCAT_x64_$(WINOS))

  ifneq ($(filter %86,$(SIGN_ARCH)),)
    SIGN_DRIVER    :=$(subst $(BCMDRVPFX_x64),$(BCMDRVPFX_x86),$(SIGN_DRIVER))
    SIGN_DRIVERCAT :=$(subst 64.cat,.cat,$(SIGN_DRIVERCAT))
  endif # SIGN_ARCH

  SIGN_OS_WINXP        := XP2K
  SIGN_OS_WINXPHIGH    := XP2K
  SIGN_OS_WINXPHIGHSD  := XP2K
  SIGN_OS_WINVISTA     := VISTA
  SIGN_OS_WINVISTAHIGH := VISTA
  SIGN_OS_WIN7         := 7
  SIGN_OS_WIN7HIGH     := 7
  SIGN_OS_WIN7HIGHSD   := 7
  SIGN_OS_WIN8X        := 6_3
  SIGN_OS_WIN8XHIGH    := 6_3
  SIGN_OS_WIN8XHIGHSD  := 6_3
  SIGN_OS               = $(SIGN_OS_$(WINOS))
endif # SIGN_DIR

# WDK build tool exit code may not indicate error code, if driver is not built!
# So this function verified explicitly if the driver was built or not
OBJDIR_x86   := i386
OBJDIR_x64   := amd64
OBJDIR_amd64 := amd64
## Args $1=ARCH, $2=WDKOS, $3=TYPE
ifeq ($(findstring WIN8X,$(WINOS)),)
define POST_BUILD_VERIFY
	@echo "#- $0($1,$2,$3)"
	@objpath="obj$(3)_$(2)_$(if $(findstring 64,$1),amd64,x86)"; \
	sys="$(BLDDIR)/$${objpath}/$(OBJDIR_$(1))/$(BCMDRVPFX_$(1)).sys"; \
	if [ ! -f "$${sys}" ]; then \
	   echo "ERROR:"; \
	   echo "ERROR: $${sys} BUILD FAILED"; \
	   echo "ERROR:"; \
	   exit 1; \
	elif [ -f "$(NTICE)/nmsym.exe" -a "$(2)" != "wlh" -a "$(3)" == "chk" ]; then \
	   echo "Running SoftICE on $${sys}"; \
	   "$(NTICE)/nmsym.exe" $(NTICE_FLAGS) -SOURCE:.  $${sys}; \
	fi
endef # POST_BUILD_VERIFY
else # WINOS==WIN8
define POST_BUILD_VERIFY
	@echo "#- $0($1,$2,$3)"
	@objpath="obj/$(2)/$(3)/$(1)"; \
	sys="$${objpath}/$(BCMDRVPFX_$(1)).sys"; \
	if [ ! -f "$${sys}" ]; then \
	   echo "ERROR:"; \
	   echo "ERROR: $${sys} BUILD FAILED"; \
	   echo "ERROR:"; \
	   exit 1; \
	elif [ -f "$(NTICE)/nmsym.exe" -a "$(filter chk% checked%,$(3))" != "" ]; then \
	   echo "Running SoftICE on $${sys}"; \
	   "$(NTICE)/nmsym.exe" $(NTICE_FLAGS) -SOURCE:.  $${sys}; \
	fi
endef # POST_BUILD_VERIFY
endif # WINOS

# Prefast specific steps to run after prefast tool is run
POST_PREFAST_VERIFY=\
	:postprefast\n\
	%PREFAST% list\n\
	echo ------ %WINOS% %VARIANT% PREFAST DEFECTS ----- >> ../prefast.log\n\
	%PREFAST% list >> ../prefast.log\n\
	rmdir /s /q obj%VARIANT%\n\
	goto done\n\

## Default list of build targets
all: build_xp_driver
all: build_xp_high_driver
all: build_xp_highsd_driver
all: build_win7_highsd_driver
all: build_win7_driver
all: build_win7_high_driver

# Win8 driver target is conditionally launched for certain brands
ifneq ($(findstring win8x_,$(BRAND)),)
all: build_win8x_driver
all: build_win8x_high_driver
endif # win8x

## Prefast targets, conditionally called when PREFAST=1
prefast: prefast_xp_driver prefast_vista_driver

ifneq ($(PREFAST),)
all: prefast
endif # PREFAST

showinfo:
	@echo "============================================================="
	@echo "$(CURWINOS) BLDDIR   = $(CURBLDDIR)"
	@echo "$(CURWINOS) WL_CONF  = $(CONFIG_WL_CONF)"
	@echo "$(CURWINOS) SOURCES  = $(sort $(notdir $(C_SOURCES)))"
	@echo "$(CURWINOS) INC_DIRS = $(INC_DIRS) $(OTHER_INC_DIRS)"
	@echo "-------------------------------------------------------------"
	@echo "$(CURWINOS) WLFLAGS          = $(sort $(WLFLAGS))"
	@echo "$(CURWINOS) EXTRA_C_DEFINES  = $(sort $(EXTRA_C_DEFINES))"
	@echo "$(CURWINOS) MY_C_DEFINES     = $(sort $(MY_C_DEFINES))"
	@echo "$(CURWINOS) C_DEFINES        = $(sort $(C_DEFINES))"
	@echo "============================================================="

# Show effective dongle image names (used by release builds)
show_dongle_images:
	@echo "$(ALL_DNGL_IMAGES)"

# Find embeddable images to support one(bmac)-to-many(firmware) mapping
find_dongle_images:
	@for image in \
		$(DNGL_IMAGE_HIGH) \
		$(DNGL_IMAGE_WINXPHIGHSD) \
		$(DNGL_IMAGE_WIN7HIGHSD) \
		$(DNGL_IMAGE_WINNIC) \
		$(EMBED_DONGLE_IMAGES) ; \
	do \
		imagedir="$(DNGLBLDDIR)/$$image"; \
		chiprev=$$(echo $$image | cut -d- -f1); \
		imagefile="$$imagedir/rtecdc_$${chiprev}.h"; \
		if [ ! -f "$$imagefile" ]; then \
			echo -e "\n\bWARN: Embed dongle image $$image missing\b\n"; \
		fi; \
	done

# WINOS specific build targets
build_xp_driver build_win7_driver \
build_xp_high_driver build_win7_high_driver \
build_xp_highsd_driver build_win7_highsd_driver \
build_win8x_driver	build_win8x_high_driver:
	@echo -e "\n=====================================================\n"
	@echo -e "\nRunning $@ now [$$(date)]\n"
	$(MAKE) build_driver

# PREFAST specific targets
prefast_xp_driver prefast_vista_driver \
prefast_xp_high_driver prefast_vista_high_driver  \
prefast_xp_highsd_driver prefast_win7_highsd_driver:
	@echo -e "\n=====================================================\n"
	@echo -e "\nRunning $@ now\n"
	$(MAKE) PREFAST=1 BLDDIR=prefast$(BLDDIR) BUILD_TYPES=free build_driver

# Core (final) driver build target
# gen_sources: generates only msft ddk/wdk compatible 'sources' file
# build_sources: builds bcm wl drivers from msft ddk/wdk 'sources' file
build_driver: showinfo find_dongle_images gen_sources build_sources

GENERATED_SOURCES := wlc_clm_data.c

ifeq ($(SKIP_GEN_SOURCES),)
  ifneq (,$(filter WIN8X%,${WINOS}))
    gen_sources: clm_compiled
    gen_sources: $(addprefix copy_dongle_headers-,${BUILD_VARIANTS})
  else
    gen_sources: prep_sources copy_sources copy_includes
  endif
else
  gen_sources:
endif # SKIP_GEN_SOURCES

# Prepare the sources file with pre-requisites
prep_sources: _prep_sources

_prep_sources:
ifeq ($(SKIP_PREP_SOURCES),)
	@echo -e "#\n# Preparing $(SOURCES) file now\n#\n"
	@[ -d $(BLDDIR) ] || mkdir -pv $(BLDDIR)
	@echo -e "#" > $(SOURCES)
	@echo -e "# Don't edit. This is automagically generated!!" >> $(SOURCES)
	@echo -e "# Use 'build.exe' from wdk to build wl driver\n" >> $(SOURCES)
	@echo -e "# WDK $(WDK_VER) sources file for $(WINOS)"      >> $(SOURCES)
	@echo -e "# Generated on $(shell date)"          >> $(SOURCES)
	@echo -e "# $(if $(BRAND),BRAND = $(BRAND))\n"   >> $(SOURCES)
#	For developer build and for better build performance
#	we skip copy_includes rule and refer to original headers
ifdef DEVBUILD
	@echo -e "SRCBASE         = $(BSRCBASE)\n" >> $(SOURCES)
	@echo -e "WDKDIR          = $(BASEDIR_UX)\n" >> $(SOURCES)
	@echo -e "INCLUDES        = $(subst $(space),; ,$(subst $(BSRCBASE),\0044(SRCBASE)/,$(strip $(BINC_DIRS)) $(strip $(OTHER_INC_DIRS)))); \0044(DDK_INC_PATH); \0044(INCLUDE)"  >> $(SOURCES)
else #DEVBUILD
	@echo -e "SRCBASE         = src\n" >> $(SOURCES)
	@echo -e "INCLUDES        = $(subst $(space),; ,$(subst $(SRCBASE),\0044(SRCBASE),$(strip $(INC_DIRS)) $(strip $(OTHER_INC_DIRS)))); \0044(DDK_INC_PATH); \0044(INCLUDE)"  >> $(SOURCES)
endif #DEVBUILD
	@echo -e "TARGETTYPE      = DRIVER"              >> $(SOURCES)
	@echo -e "TARGETPATH      = obj"                 >> $(SOURCES)
	@echo -e "TARGETLIBS      = \0044(DDK_LIB_PATH)/ndis.lib \\" >> $(SOURCES)
ifneq ($(findstring WIN7HIGHSD,$(WINOS)),)
	@echo -e "                  \0044(DDK_LIB_PATH)/sdbus.lib\n" >> $(SOURCES)
else  # WIN7HIGHSD
  ifeq ($(findstring WINXPHIGHSD,$(WINOS)),)
	@echo -e "                  \0044(DDK_LIB_PATH)/usbd.lib\n"  >> $(SOURCES)
  endif # WINXPHIGHSD
endif  # WIN7HIGHSD
	@echo -e "\n"                                    >> $(SOURCES)
ifneq ($(findstring WINXP,$(WINOS)),)
	@echo -e "!IF \"\0044(_BUILDARCH)\" == \"x86\""  >> $(SOURCES)
	@echo -e "TARGETNAME      = $(BCMDRVPFX_x86)"    >> $(SOURCES)
	@echo -e "!ELSE"                                 >> $(SOURCES)
	@echo -e "TARGETNAME      = $(BCMDRVPFX_x64)"    >> $(SOURCES)
	@echo -e "!ENDIF\n"                              >> $(SOURCES)
	@echo -e "C_DEFINES       = $(C_DEFINES) $(MSX86_C_DEFINES)\n" >> $(SOURCES)
	@echo -e "!IF \"\0044(DDKBUILDENV)\" == \"chk\"" >> $(SOURCES)
	@echo -e "C_DEFINES       = \0044(C_DEFINES) $(WL_CHK_C_DEFINES)\n" >> $(SOURCES)
	@echo -e "!ENDIF\n"                              >> $(SOURCES)
else # VISTA
	@echo -e "!IF \"\0044(_BUILDARCH)\" == \"x86\""  >> $(SOURCES)
	@echo -e "TARGETNAME      = $(BCMDRVPFX_x86)"    >> $(SOURCES)
	@echo -e "!ELSE"                                 >> $(SOURCES)
	@echo -e "TARGETNAME      = $(BCMDRVPFX_x64)"    >> $(SOURCES)
	@echo -e "!ENDIF\n"                              >> $(SOURCES)
	@echo -e "C_DEFINES       = $(C_DEFINES)\n"      >> $(SOURCES)
	@echo -e "!IF \"\0044(DDKBUILDENV)\" == \"chk\"" >> $(SOURCES)
	@echo -e "C_DEFINES       = \0044(C_DEFINES) $(WL_CHK_C_DEFINES)" >> $(SOURCES)
	@echo -e "!ENDIF\n"                              >> $(SOURCES)
endif # WINOS
	@echo -e "LINKER_FLAGS    = \0044(LINKER_FLAGS) -MAP:\0044(TARGETPATH)/\0044(TARGET_DIRECTORY)/\0044(TARGETNAME).map\n"          >> $(SOURCES)
ifneq ($(filter WIN7HIGH%,$(WINOS)),)
ifneq ($(filter 7% 8%,$(WDK_VER)),)
	@echo -e "KMDF_VERSION_MAJOR = $(WDK_KMDF_VERSION_MAJOR)\n">> $(SOURCES)
	@echo -e "KMDF_VERSION_MINOR = $(WDK_KMDF_VERSION_MINOR)\n">> $(SOURCES)
else
	@echo -e "KMDF_VERSION    = $(WDK_KMDF_VERSION)\n">> $(SOURCES)
endif # WDK_VER
endif # WIN7HIGH
endif # SKIP_PREP_SOURCES

# Update the CLM database C code from XML inputs if present.
CLM_TYPE := 43xx_pcoem
$(call WLAN_GenClmCompilerRule,$(BLDDIR),$(SRCBASE))

prep_sources: clm_compiled

# List the wl config filtered source files into $(BLDDIR)/sources file
# For developer build, only updated files are copied over and built
copy_sources: prep_sources
ifeq ($(SKIP_COPY_SOURCES),)
	@echo -e "#\n# Copying wl source files now\n#\n"
	$(PREPWDK) -install -output "$(SOURCES)" -blddir "$(BLDDIR)" -generated "$(GENERATED_SOURCES)" -treebase "$(dir $(abspath $(SRCBASE)))" -src_files "src/wl/sys/wl.rc $(C_SOURCES)"
endif # SKIP_COPY_SOURCES

# Scan the source files to generate list of wl header files
# needed to compile sources generated in 'sources' make rule.
copy_includes: copy_sources
ifndef SKIP_COPY_INCLUDES
ifndef DEVBUILD
	@echo "WDK ntddndis.h replaces src/wl/sys/ntddndis.h"
	$(strip python $(SRCBASE)/tools/build/wdkinc.py \
	    --tree-base=$(SRCBASE)/.. --to-dir=$(BLDDIR) --skip wl/sys/ntddndis.h \
	    $(if $(filter WIN7%,$(WINOS)),,--copy-tree $(FIPS_INC_DIR) $(BLDDIR)/src/wl/sys/fips/funk/inc) \
	    -- $(INC_FLAGS) $(C_DEFINES) $(C_SOURCES) \
	    $(if $(filter 6001% 7% 8%,$(WDK_VER)),,$(PRERELEASE_FILES)))
endif # DEVBUILD
endif # SKIP_COPY_INCLUDES

build_include:
ifneq ($(findstring WIN8X,$(WINOS)),)
	@install -pvD  $(SRCBASE)/wl/config/$(WLTUNEFILE) $(WDKOS)_$(TYPE)/wlconf.h
	$(MAKE) -C $(SRCBASE)/include
#	If OEMDefs exists, reuse it
	@if [ ! -s "$(WDKOS)_$(TYPE)/OEMDefs.h" ]; then \
	    if [ -s "$(SRCBASE)/wl/locale/english/$(OEM).txt" ]; then \
	       cat $(SRCBASE)/wl/locale/english/$(OEM).txt | egrep 'STR_OEM_' | perl -lne '/STR_([^\s+]*)\s+L(.*)$$/ && printf("#define %-40s %s\n",$$1,$$2)' | col -b > $(WDKOS)_$(TYPE)/OEMDefs.h; \
	    else \
	       echo "ERROR:"; \
	       echo "ERROR: '$(SRCBASE)/wl/locale/english/$(OEM).txt' is missing or empty"; \
	       echo "ERROR:"; \
	       exit 1; \
	    fi; \
	fi
else # non-Win8
	@install -pd $(BLDDIR)/src/include
	@install -p  $(SRCBASE)/include/Makefile     $(BLDDIR)/src/include/
	@install -p  $(SRCBASE)/include/epivers.sh   $(BLDDIR)/src/include/
	@install -p  $(SRCBASE)/include/epivers.h.in $(BLDDIR)/src/include/
	@if [ ! -s "$(BLDDIR)/src/include/epivers.h" -a \
		-s "$(SRCBASE)/include/epivers.h" ]; then \
	    install -pv  $(SRCBASE)/include/epivers.h $(BLDDIR)/src/include/; \
	fi
	@install -p  $(SRCBASE)/wl/config/$(WLTUNEFILE) $(BLDDIR)/src/include/wlconf.h
	@install -p  $(SRCBASE)/include/vendor.h $(BLDDIR)/src/include/
	$(MAKE) -C $(BLDDIR)/src/include
#	If OEMDefs exists, reuse it
	@if [ ! -s "$(BLDDIR)/src/include/OEMDefs.h" ]; then \
	    if [ -s "$(SRCBASE)/wl/locale/english/$(OEM).txt" ]; then \
	       cat $(SRCBASE)/wl/locale/english/$(OEM).txt | egrep 'STR_OEM_' | perl -lne '/STR_([^\s+]*)\s+L(.*)$$/ && printf("#define %-40s %s\n",$$1,$$2)' | col -b > $(BLDDIR)/src/include/OEMDefs.h; \
	    else \
	       echo "ERROR:"; \
	       echo "ERROR: '$(SRCBASE)/wl/locale/english/$(OEM).txt' is missing or empty"; \
	       echo "ERROR:"; \
	       exit 1; \
	    fi; \
	fi
endif

# .wdkos (build-variant [from ${BUILD_VARIANTS}]):
#
# Depends on variables:
#     WINOS - passed into makefile, or taking default value (WIN7).
#
# Description:
#     Produces the definitive Windows Driver Kit operating system tag,
#     useful for feeding the MS development suite, to target a particular
#     operating system, running on a particular architecture.
#
.wdkos   = $(strip \
             $(or \
                $(if $(filter WIN8XHIGH,${WINOS}),win8x_high) \
               ,$(if $(filter WIN8X,${WINOS}),win8x_nic) \
               ,$(if $(filter WIN7%,${WINOS}),win7) \
               ,$(if $(filter WINXP%,${WINOS}), \
                  $(if $(findstring x86,$1),wxp,wnet) \
                 ) \
               ,$(error UNKNOWN WINDOWS OS: '${WINOS}') \
              ) \
            )

# .type (build-variant [from ${BUILD_VARIANTS}]):
#
# Depends on variables:
#     WINOS - passed into makefile, or taking default value (WIN7).
#
# Description:
#     Produces the definitive build type (free or checked), based upon
#     the value of $WINOS, and the given build variant.
#
.type    = $(strip \
             $(if $(filter WIN8X%,${WINOS}), \
               $(or $(findstring free,$1), \
                 $(or $(findstring checkedexternal,$1),checked) \
                ), \
               $(if $(filter WIN8X%,${WINOS}), \
                 $(or $(findstring free,$1),checked), \
                 $(if $(findstring free,$1),fre,chk) \
                ) \
              ) \
            )

# .arch (build-variant [from ${BUILD_VARIANTS}]):
#
# Description:
#     Produces the build target architecture, from the given variant.
#
.arch    = $(strip $(word 2,$(subst _, ,$1)))

# .msg (build-variant [from ${BUILD_VARIANTS}]):
#
# Description:
#     Produces an upper-case "tag" for the error message, which is then
#     substituted into the Windows batch file which actually runs the
#     build automatically.
#
.msg     = $(strip \
             $(shell \
               echo '$(firstword $(subst _, ,$1))' \
                 | tr '[:lower:]' '[:upper:]' \
              ) \
            )

# .action (build-variant [from ${BUILD_VARIANTS}]):
#
# Description:
#     Produces the name of the activity associated with the given build variant.
#
.action  = $(firstword $(subst _, ,$1))

# .bat (build-variant [from ${BUILD_VARIANTS}]):
#
# Depends on variables:
#     WINOS  - passed into makefile, or taking default value (WIN7).
#     ObjPfx - Provided by a different makefile, this is where the
#              build will deposit build artifacts.
#
# Description:
#     Produces the name of the batch file which will be generated
#     by a rule within this file.  The file so generated is the actual
#     executable entity which eventually starts the build on a Windows
#     build host.
#
.bat     = $(strip \
             $(shell cygpath -m \
               $(call compose,, \
                 ${ObjPfx} \
                 $(call compose,_, \
                   $(call .action,$1) $(call .type,$1) \
                   ${WINOS} $(call .arch,$1) \
                  ) \
                 .bat \
                ) \
              ) \
            )

# .bldoutarch (build-variant [from ${BUILD_VARIANTS}]):
#
# Description:
#     Produces the name of the build output architecture, which is then
#     substituted into the Windows batch file, generated by a rule within
#     this makefile.
#
.bldoutarch  = $(strip ${BLDOUT_ARCH_$(word 2,$(subst _, ,$1))})

# .bldtemplate (none):
#
# Depends on variables:
#     WINOS          - passed into makefile, or taking default value (WIN7).
#     MSBLD_TEMPLATE - Win8/Win8X variant of the build batch script.
#     BLD_TEMPLATE   - Other Windows target variant of the build batch script.
#
# Description:
#     Dependent upon the value found in $WINOS, produces either the name of
#     the Win8/Win8X build script, or the name of the build script which
#     executes the build for other Windows platforms.
#
.bldtemplate = $(strip \
                 $(if $(filter WIN8X%,${WINOS}) \
                   , ${MSBLD_TEMPLATE} \
                   , ${BLD_TEMPLATE} \
                  ) \
                )


## --------------------------------------------------------------------------
## By default build both x86 and amd64 free/checked drivers
## To build individual driver types, use e.g: make build_x86_checked
build_sources: $(call .bldtemplate) build_include ${BUILD_VARIANTS}
build_dongle_images: build_include

# This loop constructs a set of target-specific variables, which are
# useful both for copying a per-configuration set of dongle headers
# into place, and also directs the driver build to depend upon the
# associated dongle header copy.
$(foreach v,${BUILD_VARIANTS}, \
  $(eval .PHONY: $v copy_dongle_headers-$v) \
  $(eval $v: copy_dongle_headers-$v) \
  $(eval $v: $(call .bldtemplate)) \
  $(eval $v copy_dongle_headers-$v: BLD_TEMPLATE := $(call .bldtemplate)) \
  $(eval $v copy_dongle_headers-$v: BLDOUTARCH   := $(call .bldoutarch,$v)) \
  $(eval $v copy_dongle_headers-$v: WDKOS        := $(call .wdkos,$v)) \
  $(eval $v copy_dongle_headers-$v: ARCH         := $(call .arch,$v)) \
  $(eval $v copy_dongle_headers-$v: TYPE         := $(call .type,$v)) \
  $(eval $v copy_dongle_headers-$v: MSG          := $(call .msg,$v)) \
  $(eval $v copy_dongle_headers-$v: BAT          := $(call .bat,$v)) \
  $(eval $v copy_dongle_headers-$v: VARIANT      := $v) \
 )

# These functions, given the name of a build variant and the name of a
# dongle image, will calculate the location of the resources and the
# location of the destination, for the purpose of copying built dongle
# image headers into place, so that the driver may include and embed them.
.chiprev = $(firstword $(subst -, ,$1))
.imgpath = $(DNGLBLDDIR)/$(strip $1)
.dnglimg = $(call .imgpath,$2)/rtecdc_$(call .chiprev,$2).h
.hdrpath = $(DNGLBLDDIR)/$(call .wdkos,$1)_$(call .type,$1)
.dnglhdr = $(call .hdrpath,$1)/olbin_$(call .chiprev,$2).h
.dnglinc = $(DNGLBLDDIR)/$(call .wdkos,$1)_$(call .type,$1)

# This odd-looking rule creates a single large script which copies dongle
# image header files into a known directory (per-configuration), so that
# the driver may include them for purposes of embedding firmware.
$(addprefix copy_dongle_headers-,${BUILD_VARIANTS}): build_dongle_images
	@$(MARKSTART)
	: $(foreach i,$(sort ${EMBED_DONGLE_IMAGES}),      \
	    $(if $(wildcard $(call .dnglimg,${VARIANT},$i)), \
	       ; echo "mkdir -pv '$(call .hdrpath,${VARIANT},$i)'" \
	       ; mkdir -pv '$(call .hdrpath,${VARIANT},$i)' \
	       ; echo "cp -vf $(strip \
	                        '$(call .dnglimg,${VARIANT},$i)' \
	                        '$(call .dnglhdr,${VARIANT},$i)' \
	                       )" \
	       ; cp -vf '$(call .dnglimg,${VARIANT},$i)' \
	                '$(call .dnglhdr,${VARIANT},$i)' \
	      , \
	       ; echo "WARN: Embed dongle image '$i' missing." >&2 \
	     ) \
	   )
	@$(MARKEND)

$(BUILD_VARIANTS):
	@$(MARKSTART)
	@if [ "$(BAT)" -ot "$(BLD_TEMPLATE)" ]; \
	then \
	   echo "INFO: Generating new $(BAT) for $@ target"; \
	   sed \
	     -e "s/%TYPE%/$(TYPE)/g"   \
	     -e "s/%WDKOS%/$(WDKOS)/g" \
	     -e "s/%ARCH%/$(ARCH)/g"   \
	     -e "s/%BLDOUTARCH%/$(BLDOUTARCH)/g"   \
	     -e "s/%ACTION%/$(ACTION)/g"   \
	     -e "s/%WINOS%/$(WINOS)/g"   \
	     -e "s/%MSG%/$(MSG)/g"   \
	     -e "s/%WDK_VER%/$(WDK_VER)/g"   \
	     -e "s!%BASEDIR%!$(subst /,~,$(BASEDIR_UX))!g"   \
	     -e "s!%BLDDIR%!$(subst /,~,$(CWDIR))!g"   \
	     -e "s/%TITLE%/$(TITLE)_$(MSG)/g"   \
	     -e "s/%PREFAST%/$(if $(PREFAST),prefast \/LOG=prefast_%VARIANT%.xml)/g" \
	  $(BLD_TEMPLATE) | sed -e "s/~/\\\\/g" > $(BAT); \
	else \
	   echo "INFO: Reusing existing $(BAT)"; \
	fi
ifndef SKIP_BUILD_SOURCES
#	# WDK build.exe outputs the log as build<winos>/*.log files
#	# Let them show the output on same build window. Needed
#	# in precommit and cont integ shell contexts builds
	@$(DOSCMD) /C "$(BAT) && set BUILD_EC=%ERRORLEVEL% && exit %BUILD_EC%"
ifndef PREFAST
	$(call POST_BUILD_VERIFY,$(ARCH),$(WDKOS),$(TYPE))
endif # PREFAST
endif # SKIP_BUILD_SOURCES
	@echo -e "\nFinished with '$@' target at $$(date)\n"
#	@rm -f $(BAT)
	@$(MARKEND)


## ---------------------------------------------------------------------------
## In order to speed up build process, generate a wdk build batch file
## template and use it for launching subsequent build variants
## note: wdk build tool exit codes do not indicate some error conditions
## correctly. We need to manually check built objects for correctness!

$(BLD_TEMPLATE):
	@echo -e "@echo off\n\
	@REM Automatically generated on $(shell date). Do not edit\n\n\
	set VARIANT=%TYPE%_%WDKOS%_%BLDOUTARCH%\n\
	set PREFIX=build%VARIANT%\n\
	set WDKDIR=%BASEDIR%\n\
	set PATH=%WDKDIR%\\\\bin\\\\x86;%path%\n\n\
	if NOT EXIST %WDKDIR%\\\\bin\\\\x86\\\\build.exe goto wdkenverror\n\
	echo %WINOS% %MSG% %VARIANT% with %WDK_VER% WDK\n\
	echo Running at %date% %time% on %computername%\n\
	call %WDKDIR%\\\\bin\\\\setenv %WDKDIR% %TYPE% %ARCH% %WDKOS% $(WDK_OACR)\n\
	set MAKEFLAGS=\n\
	cd /D %BLDDIR%\n\
	goto ec%ERRORLEVEL%\n\n\
	:ec0\n\
	if /I NOT \"%cd%\"==\"%BLDDIR%\" goto ec1\n\
	title %TITLE%\n\
	echo Current Dir : %cd%\n\
	echo %PREFAST% build -be %WDKFLAGS%\n\
	%PREFAST% build -be %WDKFLAGS%\n\
	set buildec=%ERRORLEVEL%\n\
	if EXIST %PREFIX%.log if DEFINED VERBOSE type %PREFIX%.log\n\
	if EXIST %PREFIX%.wrn type %PREFIX%.wrn\n\
	if EXIST %PREFIX%.err type %PREFIX%.err\n\
	if /I NOT \"%buildec%\"==\"0\" goto buildec1\n\
	title DONE_%TITLE%\n\
	if NOT \"%PREFAST%\"==\"\" goto postprefast\n\
	goto done\n\n\
	:wdkenverror\n\
	echo ERROR: WDK directory '%WDKDIR%' is not found\n\
	echo  INFO: You may have incompatible generated build scripts\n\
	echo  INFO: You can regenerate them with 'make SKIP_BUILD_SOURCES=1'\n\
	exit /B 1\n\n\
	:buildec1\n\
	echo ERROR: %ACTION% failed with error code in '%BLDDIR%'\n\
	exit /B 1\n\n\
	:ec1\n\
	echo ERROR: Could not change dir to '%BLDDIR%'\n\
	goto done\n\n\
	$(POST_PREFAST_VERIFY)\n\n\
	:buildec0\n\
	:done\n\
	echo Done with %WINOS% %MSG% for %VARIANT%\n" \
	| sed -e 's/^[[:space:]]//g' > $@

## ---------------------------------------------------------------------------
## Use DOS to echo ENV Vars in the raw. No need to escape char's from bash.exe.
## Outer CMD loops from 1 to N, inner echos lines using cmd /v !VAR! feature.
## cygwin ssh strips win variables like %PROGRAMFILES%, add back for precommit.
## Temp must be \temp,  not long path with spaces or VStudio batch build can fail.
## To right of := is the .BAT script written "as shown", no escape chars needed
## ---------------------------------------------------------------------------

export MSLINE_1  := @echo off
export MSLINE_2  := REM Automatically generated on $(shell date). Do not edit
export MSLINE_3  := set VARIANT=%TYPE%_%WDKOS%_%BLDOUTARCH%
export MSLINE_4  := set OUTDIR=obj/%WDKOS%/%TYPE%/%ARCH%
export MSLINE_5  := set VCDIR=$(WIN8X_VSDIR)\VC
export MSLINE_6  := set TEMP=C:\temp
export MSLINE_7  := set TMP=C:\temp
export MSLINE_8  := if "%ALLUSERSPROFILE%"=="" SET ALLUSERSPROFILE=C:\ProgramData
export MSLINE_9  := if "%CommonProgramFiles%"=="" SET CommonProgramFiles=C:\Program Files\Common Files
export MSLINE_10 := if "%CommonProgramFiles(x86)%"=="" SET CommonProgramFiles(x86)=C:\Program Files (x86)\Common Files
export MSLINE_11 := if "%CommonProgramW6432%"=="" SET CommonProgramW6432=C:\Program Files\Common Files
export MSLINE_12 := if "%ComSpec%"=="" SET ComSpec=C:\Windows\system32\cmd.exe
export MSLINE_13 := if "%DLGTEST_NOTIFY%"=="" SET DLGTEST_NOTIFY=false
export MSLINE_14 := if "%ProgramData%"=="" SET ProgramData=C:\ProgramData
export MSLINE_15 := if "%ProgramFiles%"=="" SET ProgramFiles=C:\Program Files
export MSLINE_16 := if "%ProgramFiles(x86)%"=="" SET ProgramFiles(x86)=C:\Program Files (x86)
export MSLINE_17 := if "%ProgramW6432%"=="" SET ProgramW6432=C:\Program Files
export MSLINE_18 := if "%WINDIR%"=="" SET WINDIR=C:\Windows
export MSLINE_19 := if NOT EXIST %VCDIR%\vcvarsall.bat goto vcenverr
export MSLINE_20 := echo %WINOS% %MSG% %VARIANT% with VS%VS_VER%
export MSLINE_21 := echo Running at %date% %time% on %computername%
export MSLINE_22 := set MAKEFLAGS=
export MSLINE_23 := call %VCDIR%\vcvarsall.bat %ARCH%
export MSLINE_24 := goto ec%ERRORLEVEL%
export MSLINE_25 := :ec0
export MSLINE_26 := title %VARIANT%
export MSLINE_27 := echo Current Dir : %cd%
export MSLINE_28 := which msbuild.exe
export MSLINE_29 := echo msbuild.exe /property:Configuration='%WDKOS%_%TYPE%' /property:Platform=%BLDOUTARCH% /property:OutDir=%OUTDIR%/ /property:WDKContentRoot=$(WIN8X_WDKDIR)/ /property:GenerateManifest=false %VSFLAGS% win8driver.vcxproj
export MSLINE_30 := msbuild.exe /property:Configuration="%WDKOS%_%TYPE%" /property:Platform=%BLDOUTARCH% /property:OutDir=%OUTDIR%/ /property:WDKContentRoot=$(WIN8X_WDKDIR)/ /property:GenerateManifest=false %VSFLAGS% win8driver.vcxproj
export MSLINE_31 := set buildec=%ERRORLEVEL%
export MSLINE_32 := if /I NOT "%buildec%"=="0" goto buildec1
export MSLINE_33 := title DONE_%VARIANT%
export MSLINE_34 := goto done
export MSLINE_35 := :vcenverr
export MSLINE_36 := echo ERROR: VS directory '%VCDIR%' is not found
export MSLINE_37 := exit /B 1
export MSLINE_38 := :buildec1
export MSLINE_39 := echo ERROR: %ACTION% failed with error code 1
export MSLINE_40 := exit /B 1
export MSLINE_41 := :buildec0
export MSLINE_42 := :done
export MSLINE_43 := echo Done with %WINOS% %MSG% for %VARIANT%
MSLINE_COUNT = 43

## Echo each raw env var in order creating .BAT script
MSBLD_CMD := FOR /L %i IN (1,1,$(MSLINE_COUNT)) DO @cmd /v /c echo !MSLINE_%i!

$(MSBLD_TEMPLATE):
	@rm -fv $@
	@cmd /c '$(MSBLD_CMD)' >> $@

# Force .temp*.BAT to refresh every time makefile is invoked
.PHONY: $(MSBLD_TEMPLATE)

## ---------------------------------------------------------------------------
## Release sign drivers - needs certificate to be installed on local machine
## as documented in HowToCompileThings twiki

ifdef SIGN_DIR

# If driver signing fails for any reason during timestamping record log by curl utility
# And keep trying for 10mins to connect to verisign timeserver
define SIGN_AND_TIMESTAMP
	@echo "#- $0($1)"
	@cd $(SIGN_DIR); \
	rm -f $(SIGN_LOG); \
	if [ ! -s "$(MS_CROSSCERT)" ]; then \
		echo "ERROR: Microsoft CrossCertificate MS_CROSSCERT missing"; \
		echo "ERROR: It needs to be available at $(MS_CROSSCERT)"; \
	   	exit 1; \
	fi; \
	echo "$(RETRYCMD) signtool sign $(SIGNTOOL_OPTS) /ac $(MS_CROSSCERT) \
		/s my /n 'Broadcom Corporation' \
		/t $(TIMESTAMP_URL) $(1)"; \
	$(RETRYCMD) signtool sign $(SIGNTOOL_OPTS) /ac $(MS_CROSSCERT) \
		/s my /n "Broadcom Corporation" \
		/t $(TIMESTAMP_URL) $(1); \
	echo "DONE WITH SIGN_AND_TIMESTAMP"
endef # SIGN_AND_TIMESTAMP

ifneq ($(findstring WIN8X,$(WINOS)),)
release_sign_driver  sign_driver \
release_sign_sysfile sign_sysfile: PATH := /usr/bin:/bin:$(shell cygpath -u Z:/projects/hnd/tools/win/msdev/$(WIN8_WDK_VER)wdksdk/bin/$(SIGN_ARCH)):$(shell cygpath -u Z:/projects/hnd/tools/win/msdev/$(WIN8_WDK_VER)wdksdk/bin/$(SIGN_ARCH)):$(shell cygpath -u C:/tools/msdev/$(WIN8_WDK_VER)wdksdk/bin/$(SIGN_ARCH)):$(shell cygpath -u C:/tools/msdev/$(WIN8_WDK_VER)wdksdk/bin/x86):$(PATH)

else # !WIN8X
release_sign_driver  sign_driver \
release_sign_sysfile sign_sysfile: PATH := /usr/bin:/bin:$(shell cygpath -u $(subst \,/,$(BASEDIR))/bin/SelfSign):$(shell cygpath -u Z:/projects/hnd/tools/win/msdev/$(WDK_VER)wdk/bin/SelfSign):$(WDKDIR)/bin/SelfSign:$(PATH)
endif # !WIN8X

release_sign_driver sign_driver:
	@echo "INFO: Sign tools from $(WDKDIR) will be used"
	@if [ ! -f "$(SIGN_DIR)/$(SIGN_DRIVER)" ]; then \
	    echo "ERROR:"; \
	    echo "ERROR: $(SIGN_DIR)/$(SIGN_DRIVER) not found to sign"; \
	    echo "ERROR: Does the $(SIGN_DRIVER) exist?"; \
	    echo "ERROR: Verify WINOS or SIGN_ARCH values provided"; \
	    echo "ERROR:"; \
	    exit 1; \
	fi
	@echo -e "\nSigning $(SIGN_ARCH) $(SIGN_OS) drivers from: $(SIGN_DIR)\n"
	@$(SRCBASE)/tools/release/wustamp -o -r -v "$(RELNUM)" -d "$(RELDATE)" "`cygpath -w $(SIGN_DIR)`"
	which Inf2Cat.exe
	cd $(SIGN_DIR); Inf2cat /driver:. /os:$(SIGN_OS)_$(SIGN_ARCH)
	$(call SIGN_AND_TIMESTAMP,$(SIGN_DRIVERCAT) $(SIGN_DRIVER))
ifeq ($(findstring WIN8X,$(WINOS)),)
	cd $(SIGN_DIR); $(RETRYCMD) signtool verify /kp /c $(SIGN_DRIVERCAT) $(SIGN_DRIVER)
endif
	cd $(SIGN_DIR); $(RETRYCMD) signtool verify /kp $(SIGN_DRIVER)


release_sign_sysfile sign_sysfile:
	@echo "INFO: Sign tools from $(WDKDIR) will be used"
	@if [ ! -f "$(SIGN_DIR)/$(SIGN_DRIVER)" ]; then \
	    echo "ERROR:"; \
	    echo "ERROR: $(SIGN_DIR)/$(SIGN_DRIVER) not found to sign"; \
	    echo "ERROR: Does the $(SIGN_DRIVER) exist?"; \
	    echo "ERROR: Verify WINOS or SIGN_ARCH values provided"; \
	    echo "ERROR:"; \
	    exit 1; \
	fi
	@echo -e "\nSigning $(SIGN_ARCH) $(SIGN_OS) drivers from: $(SIGN_DIR)\n"
	@$(SRCBASE)/tools/release/wustamp -o -r -v "$(RELNUM)" -d "$(RELDATE)" "`cygpath -w $(SIGN_DIR)`"
	$(call SIGN_AND_TIMESTAMP,$(SIGN_DRIVER))
	cd $(SIGN_DIR); $(RETRYCMD) signtool verify /kp $(SIGN_DRIVER)



gen_cat_driver: PATH := /usr/bin:/bin:$(shell cygpath -u $(subst \,/,$(BASEDIR))/bin/SelfSign):$(PATH)
gen_cat_driver: SIGN_OS=$(if $(findstring VISTA,$(WINOS)),VISTA,XP2K)
gen_cat_driver:
	@if [ ! -f "$(SIGN_DIR)/$(SIGN_DRIVER)" ]; then \
	    echo "ERROR:"; \
	    echo "ERROR: $(SIGN_DIR)/$(SIGN_DRIVER) not found to sign"; \
	    echo "ERROR: Does the $(SIGN_DRIVER) exist?"; \
	    echo "ERROR: Verify WINOS or SIGN_ARCH values provided"; \
	    echo "ERROR:"; \
	    exit 1; \
	fi
	@echo -e "\nSigning $(SIGN_ARCH) $(SIGN_OS) drivers from: $(SIGN_DIR)\n"
	@$(SRCBASE)/tools/release/wustamp -o -r -v "$(RELNUM)" -d "$(RELDATE)" "`cygpath -w $(SIGN_DIR)`"
	cd $(SIGN_DIR); Inf2cat /driver:. /os:$(subst $(space),$(comma),$(SIGN_ARCH))

endif # SIGN_DIR

clean:
	rm -rf $(XPBLDDIR)/src/include/{epivers,wlconf}.h $(XPBLDDIR)/obj*_w*
	rm -rf $(XPHIGHBLDDIR)/src/include/{epivers,wlconf}.h $(XPHIGHBLDDIR)/obj*_w*
	rm -rf $(XPHIGHBLDDIR)/build/dongle
	rm -rf $(XPHIGHSDBLDDIR)/src/include/{epivers,wlconf}.h $(XPHIGHSDBLDDIR)/obj*_w*
	rm -rf $(VISTABLDDIR)/src/include/{epivers,wlconf}.h $(VISTABLDDIR)/obj*_w*
	rm -rf $(VISTAHIGHBLDDIR)/src/include/{epivers,wlconf}.h $(VISTAHIGHBLDDIR)/obj*_w*
	rm -rf $(WIN7BLDDIR)/src/include/{epivers,wlconf}.h $(WIN7BLDDIR)/obj*_w*
	rm -rf $(WIN7HIGHBLDDIR)/src/include/{epivers,wlconf}.h $(WIN7HIGHBLDDIR)/obj*_w*
	rm -rf $(WIN7HIGHSDBLDDIR)/src/include/{epivers,wlconf}.h $(WIN7HIGHSDBLDDIR)/obj*_w*
	rm -rf $(VISTAHIGHBLDDIR)/build/dongle
	rm -rf obj/*.sys
	rm -f  $(BLD_TEMPLATE) $(MSBLD_TEMPLATE)
	rm -f build_ch*.bat build_fre*.bat prefast_ch*.bat prefast_fre*.bat
	@rm -f prefast.log

clean_all:
	rm -f build_ch*.bat build_fre*.bat prefast_ch*.bat prefast_fre*.bat
	rm -f prefast.log
	rm -f $(BLD_TEMPLATE) $(MSBLD_TEMPLATE)
	rm -rf buildxp* buildvista* buildwin7* prefastbuild* obj/*
	rm -rf buildwin8x/obj

PHONY: all copy_sources copy_includes gen_sources prep_sources build_sources	\
	build_driver		build_xp_driver		build_win7_driver	\
				build_win7_high_driver 	build_xp_highsd_driver	\
				build_win7_highsd_driver 			\
				build_win8x_driver	build_win8x_high_driver	\
				prefast_xp_driver prefast_vista_driver		\
				prefast_win7_highsd_driver prefast_xp_highsd_driver \
				clean_all clean $(BUILD_VARIANTS) release_sign_driver sign_driver help

# Display a list of available targets
help:
	@echo -e "\n\
To build xp only use:       'make build_xp_driver' \n\
To build vista/win7 driver: 'make build_win7_driver' \n\
To build noccx vista only:  'make BCMCCX=0 build_win7_driver' \n\
To build xp highmac use:    'make build_xp_high_driver' \n\
To build xp high sdio use:  'make build_xp_highsd_driver' \n\
To build win7 highmac:      'make build_win7_high_driver' \n\
To build win7 high sdio use:'make build_xp_highsd_driver' \n\
To build vista/win7 highmac with a specific firmware \n\
                            'make FIRMWARE=43236b-bmac/ag-nodis-assert  build_win7_high_driver' \n\
To build win8x driver:      'make build_win8x_driver' \n\
To prefast xp and vista use:'make prefast' \n\
To prefast xp only use:     'make prefast_xp_driver' \n\
To prefast vista only use:  'make prefast_vista_driver' \n\
To get xp sources only:     'make BRAND=1 WINOS=WINXP gen_sources' \n\
To get vista/win7 sources:  'make BRAND=1 WINOS=WIN7 gen_sources'\n\
To use WDK ver NNNN:        'make WDK_VER=NNNN' \n\
To use OLD DDK:             'make USEDDK=1' \n\
To clean built objects:     'make clean' \n\
To clean generated files:   'make clean_all' \n\
To track dependencies:      'make WDKFLAGS=\'\'' \n\
To regenerate buildscripts: 'make SKIP_BUILD_SOURCES=1' \n\
\n\
For more specific build tasks: \n\n\
To build xp x86 checked: \n\
 'make BUILD_TYPES=checked BUILD_ARCHS=x86 build_xp_driver' \n\n\
To build xp x86 high checked: \n\
 'make BUILD_TYPES=checked BUILD_ARCHS=x86 build_xp_high_driver' \n\n\
To build vista/win7 x86 checked without ccx: \n\
 'make BCMCCX=0 BUILD_TYPES=checked BUILD_ARCHS=x86 build_win7_driver' \n\n\
To build win7 x86 checked: \n\
 'make BUILD_TYPES=checked BUILD_ARCHS=x86 build_win7_driver' \n\n\
To build win7 x86 high checked: \n\
 'make BUILD_TYPES=checked BUILD_ARCHS=x86 build_win7_high_driver' \n\n\
To build win7 x86 checked with Microsoft post-build Code Review enabled: \n\
 'make BUILD_TYPES=checked BUILD_ARCHS=x86 WDK_OACR=oacr build_win7_driver' \n\n\
To build win8x x86 checked: \n\
 'make BUILD_TYPES=checked BUILD_ARCHS=x86 build_win8x_driver' \n\n\
To build win8x x64 free: \n\
 'make BUILD_TYPES=free BUILD_ARCHS=x64 build_win8x_driver' \n\n\
To sign built driver: \n\n\
[NOTE: Copy non driver files (inf,dll etc.,) from nightly build for signing] \n\
To sign vista 64bit driver: \n\
 'make SIGN_DIR=buildvista/objchk_wlh_amd64/amd64 sign_driver' \n\
To sign xp 64bit driver:    \n\
 'make WINOS=WINXP SIGN_DIR=buildxp/objchk_wnet_amd64/amd64 sign_driver' \n\n\
For further details refer to HowToCompileThings twiki \n\
"
