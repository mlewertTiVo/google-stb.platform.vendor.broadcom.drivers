# General-purpose GNU make include file
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
#
# <<Broadcom-WL-IPTag/Open:>>
#
# $Id: Makefile 506823 2014-10-07 12:59:32Z $
#
ifdef _WLAN_COMMON_MK
  $(if $D,$(info Info: Avoiding redundant include ($(MAKEFILE_LIST))))
else	# _WLAN_COMMON_MK
  _WLAN_COMMON_MK := 1
unexport _WLAN_COMMON_MK	# in case of make -e

################################################################
# Summary and Namespace Rules
################################################################
#   This is a special makefile fragment intended for common use.
# The most important design principle is that it defines variables
# and functions only within a tightly controlled namespace.
# If a make include file is used to set rules, pattern rules,
# or well known variables like CFLAGS, it can have unexpected
# effects on the including makefile with the result that people
# either stop including it or stop changing it.
# Therefore, the only way to keep this a file which can be
# safely included by any GNU makefile and extended at will is
# to allow it only to set variables and only in its own namespace.
#   The namespace is "WLAN_CamelCase" for normal variables,
# "wlan_lowercase" for functions, and WLAN_UPPERCASE for boolean
# "constants" (these are all really just make variables; only the
# usage patterns differ).
#   Internal (logically file-scoped) variables are prefixed with "-"
# and have no other namespace restrictions.
#   Every variable defined here should match one of these patterns.

################################################################
# Enforce required conditions
################################################################

ifneq (,$(filter 3.7% 3.80,$(MAKE_VERSION)))
  $(error $(MAKE): Error: version $(MAKE_VERSION) too old, 3.81+ required)
endif

################################################################
# Store this makefile's path before MAKEFILE_LIST gets changed.
################################################################

WLAN_Common := $(lastword $(MAKEFILE_LIST))

################################################################
# Derive including makefile since it's a little tricky.
################################################################

WLAN_Makefile := $(abspath $(lastword $(filter-out $(lastword $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))

################################################################
# Shiny new ideas, can be enabled via environment for testing.
################################################################

ifdef WLAN_MakeBeta
  $(info Info: BUILDING WITH "WLAN_MakeBeta" ENABLED!)
  SHELL := /bin/bash
  #.SUFFIXES:
  #MAKEFLAGS += -r
endif

################################################################
# Allow a makefile to force this file into all child makes.
################################################################

ifdef WLAN_StickyCommon
  export MAKEFILES := $(MAKEFILES) $(abspath $(lastword $(MAKEFILE_LIST)))
endif

################################################################
# Host type determination
################################################################

_common-uname-s := $(shell uname -s)

# Typically this will not be tested explicitly; it's the default condition.
WLAN_HOST_TYPE := unix

ifneq (,$(filter Linux,$(_common-uname-s)))
  WLAN_LINUX_HOST := 1
else ifneq (,$(filter CYGWIN%,$(_common-uname-s)))
  WLAN_CYGWIN_HOST := 1
  WLAN_WINDOWS_HOST := 1
else ifneq (,$(filter Darwin,$(_common-uname-s)))
  WLAN_MACOS_HOST := 1
  WLAN_BSD_HOST := 1
else ifneq (,$(filter FreeBSD NetBSD,$(_common-uname-s)))
  WLAN_BSD_HOST := 1
else ifneq (,$(filter SunOS%,$(_common-uname-s)))
  WLAN_SOLARIS_HOST := 1
endif

################################################################
# Utility variables
################################################################

empty :=
space := $(empty) $(empty)
comma := ,

################################################################
# Utility functions
################################################################

# Provides enhanced-format messages from make logic.
wlan_die = $(error Error: $1)
wlan_warning = $(warning Warning: $1)
wlan_info = $(info Info: $1)

# Debug function to enable make verbosity.
wlan_dbg = $(if $D,$(call wlan_info,$1))

# Debug function to expose values of the listed variables.
wlan_dbgv = $(foreach _,$1,$(call wlan_dbg,$_=$($_)))

# Make-time assertion.
define wlan_assert
  ifeq (,$(findstring clean,$(MAKECMDGOALS)))
    $(if $1,,$(call wlan_die,$2))
  endif
endef

# Checks for the presence of an option in an option string
# like "aaa-bbb-ccc-ddd".
wlan_opt = $(if $(findstring -$1-,-$2-),1,0)

# Compares two dotted numeric strings (e.g 2.3.16.1) for $1 >= $2
define wlan_version_ge
$(findstring TRUE,$(shell bash -c 'sort -cu -t. -k1,1nr -k2,2nr -k3,3nr -k4,4nr <(echo -e "$2\n$1") 2>&1 || echo TRUE'))
endef

# This is a useful macro to wrap around a compiler command line,
# e.g. "$(call wlan_cc,<command-line>). It organizes flags in a
# readable way while taking care not to change any ordering
# which matters. It also provides a hook for externally
# imposed C flags which can be passed in from the top level.
# This would be the best place to add a check for
# command line length. Requires a $(strlen) function;
# GMSL has one.
define wlan_cc
$(filter-out -D% -I%,$1) $(filter -D%,$1) $(filter -I%,$1) $(WLAN_EXTERNAL_CFLAGS)
endef

# Applies the standard cygpath translation for a path on Cygwin.
define wlan_cygpath
$(if $(WLAN_CYGWIN_HOST),$(shell cygpath -m $1),$1)
endef

# Change case without requiring a shell.
wlan_tolower = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,\
$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,\
$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,\
$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

wlan_toupper = $(subst a,A,$(subst b,B,$(subst c,C,$(subst d,D,$(subst e,E,$(subst f,F,$(subst g,G,\
$(subst h,H,$(subst i,I,$(subst j,J,$(subst k,K,$(subst l,L,$(subst m,M,$(subst n,N,$(subst o,O,\
$(subst p,P,$(subst q,Q,$(subst r,R,$(subst s,S,$(subst t,T,$(subst u,U,$(subst v,V,$(subst w,W,\
$(subst x,X,$(subst y,Y,$(subst z,Z,$1))))))))))))))))))))))))))

# This macro derives the base (aka root) of the entire "checkout tree" as well as the
# current "build tree" and "component tree" (svn or git checkout).
# Definitions:
# - Checkout Tree: the root of the full tree of files retrieved from SCM. May comprise
#   any number of individual checkouts, potentially from different branches, repos,
#   or even SCM tools.
# - Build Tree: the root of a coherent subtree within the checkout tree, sufficient to
#   build some subset of the required deliverables.
# - Component Tree: the root of a single component within the build tree, generally a
#   single checkout from svn or git.
# These could all have the same value but more often a component is a subdir of a build
# tree which may be a subdir of a DEPS checkout. The only guaranteed relationship is that
# each component tree is within a build tree which is within the unitary checkout tree.
# Returns a list: [checkout-tree, build-tree, component-tree]
# Notes:
# 1. We've observed a bug, or at least a surprising behavior, in emake which causes
# $(realpath ...) to fail. That's why a $(shell ...) fallback is used below.
# 2. There may be no SCM metadata at all, particularly in the case of a source package
# delivered to a customer. This is where guessing (as described below) comes in.
# 3. This macro always trusts .wlbase if present. It looks next for a .gclient
# file, then sparsefile_url.py. If none of these are found we have to guess starting
# from the location of this file. If a ../../../main file exists from here, set the
# base there. Otherwise set it at ../.. from here.
define wlan_basedirs
$(eval\
  _refdir := $$(realpath $$(or $1,$$(dir $$(lastword $$(MAKEFILE_LIST)))))
  _refdir ?= $$(shell cd $$(or $1,$$(dir $$(lastword $$(MAKEFILE_LIST)))) && pwd -P)
  _parts := $$(strip $$(subst /,$$(space),$$(subst \,/,$$(_refdir))))
  _paths := /
  $$(foreach _i,$$(_parts),$$(eval _paths += $$(addsuffix /$$(_i),$$(lastword $$(_paths)))))
  _paths := $$(patsubst //%,/%,$$(wordlist 2,$$(words $$(_paths)),$$(_paths)))
  _wlb := $$(patsubst %/,%,$$(dir $$(lastword $$(wildcard $$(addsuffix /.wlbase,$$(_paths))))))
  _dep := $$(patsubst %/,%,$$(dir $$(lastword $$(wildcard $$(addsuffix /.gclient,$$(_paths))))))
  ifndef _dep
    _sprs := $$(subst /.svn/sparsefile_url.py,,$$(lastword $$(wildcard $$(addsuffix /.svn/sparsefile_url.py,$$(_paths)))))
    ifndef _sprs
      _guess := $$(strip $$(if $$(wildcard $$(dir $$(abspath $$(WLAN_Common)))../../../main),\
	$$(abspath $$(dir $$(WLAN_Common))../../..),\
	$$(abspath $$(dir $$(WLAN_Common))../..)))
    endif  # _sprs
  endif  # _dep
  _arg3 := $$(patsubst %/,%,$$(dir $$(lastword $$(wildcard $$(addsuffix /.svn,$$(_paths)) $$(addsuffix /.git,$$(_paths))))))
  _arg2 := $$(abspath $$(_refdir)/../..)
  _arg1 := $$(or $$(_wlb),$$(_dep),$$(_sprs),$$(_guess),$$(_arg3))
)$(strip $(_arg1) $(_arg2) $(_arg3))
endef

################################################################
# Standard make variables
################################################################

# The WLAN_CheckoutBaseA variable points to the root of the entire
# checkout while WLAN_TreeBaseA points to the root of the current
# build tree and WLAN_ComponentBaseA points to the root of the
# component containing the including Makefile. They could have the
# values values but in other cases such as DHDAP routers a single
# DEPS checkout may contain multiple build trees (potentially from
# different branches) with each build tree composed of multiple components.
# These 'A' variables are absolute paths; each has an 'R' variant
# which is relative.
ifndef WLAN_TreeBaseA
  _basedata := $(call wlan_basedirs)
  WLAN_CheckoutBaseA := $(word 1,$(_basedata))
  WLAN_TreeBaseA := $(word 2,$(_basedata))
  WLAN_ComponentBaseA := $(word 3,$(_basedata))
  ifdef WLAN_CYGWIN_HOST
    WLAN_TreeBaseA := $(shell cygpath -m -a $(WLAN_TreeBaseA))
  endif
endif

# Pick up the "relpath" make function from the same dir as this makefile.
include $(dir $(lastword $(MAKEFILE_LIST)))RelPath.mk

# The *R variables are relativized versions of the *A variables.
WLAN_CheckoutBaseR = $(call relpath,$(WLAN_CheckoutBaseA))
WLAN_TreeBaseR = $(call relpath,$(WLAN_TreeBaseA))
WLAN_ComponentBaseR = $(call relpath,$(WLAN_ComponentBaseA))

# For compatibility, due to the prevalence of $(SRCBASE)
WLAN_SrcBaseA := $(WLAN_TreeBaseA)/src
WLAN_SrcBaseR  = $(patsubst %/,%,$(dir $(WLAN_TreeBaseR)))

# Show makefile list before we start including things.
$(call wlan_dbgv, CURDIR MAKEFILE_LIST)

################################################################
# Pick up the "universal settings file" containing
# the list of all available software components.
################################################################

include $(dir $(lastword $(MAKEFILE_LIST)))../tools/release/WLAN.usf

################################################################
# Calculate paths to requested components.
################################################################

# This uses pattern matching to pull component paths from
# their basenames (e.g. src/wl/xyz => xyz).
# It also strips out component paths which don't currently exist.
# This may be required due to our "sparse tree" build styles
# and the fact that linux mkdep throws an error when a directory
# specified with -I doesn't exist.
define _common-component-names-to-rel-paths
$(strip \
  $(patsubst $(WLAN_TreeBaseA)/%,%,$(wildcard $(addprefix $(WLAN_TreeBaseA)/,\
  $(sort $(foreach name,$(if $1,$1,$(WLAN_COMPONENT_PATHS)),$(filter %/$(name),$(WLAN_COMPONENT_PATHS))))))))
endef

# If WLAN_ComponentsInUse is unset it defaults to the full set (for now, anyway - TODO).
# 'clm' component was renamed to 'clm-api', to preserve
# compatibility with makefiles from /components that shall use old 'clm'
# component name (that, in turn, needed to preserve compatibility with those
# /proj branches that are not yet converted to CLM in components)
ifeq (clm,$(findstring clm,$(WLAN_ComponentsInUse))$(findstring clm-api,$(WLAN_ComponentsInUse)))
  WLAN_ComponentsInUse := $(subst clm,clm-api,$(WLAN_ComponentsInUse))
endif

# It's also possible to request the full set with a literal '*'.
ifeq (,$(WLAN_ComponentsInUse))
  WLAN_ComponentsInUse		:= $(sort $(notdir $(WLAN_COMPONENT_PATHS)))
  # $(call wlan_die,no SW component request)
else ifeq (*,$(WLAN_ComponentsInUse))
  WLAN_ComponentsInUse		:= $(sort $(notdir $(WLAN_COMPONENT_PATHS)))
  # $(call wlan_info,all SW components requested ("$(WLAN_ComponentsInUse)"))
else
  WLAN_ComponentsInUse		:= $(sort $(WLAN_ComponentsInUse))
endif

# For backward compatibility we accept certain old component names from older
# branches and convert them to their updated names here.
WLAN_ComponentsInUse := $(sort $(subst phymods,phy,$(WLAN_ComponentsInUse)))

# Translate the resulting list of components names to paths.
WLAN_ComponentPathsInUse	:= $(call _common-component-names-to-rel-paths,$(WLAN_ComponentsInUse))

# Loop through all components in use. If a cfg-xyz.mk file exists at the base of
# component xyz's subtree, include it and use its contents to modify the list
# of include and src dirs. Otherwise, use the defaults for that component.
# Also generate a WLAN_ComponentBaseDir_xyz variable for each component "xyz".
WLAN_ComponentIncPathsInUse :=
WLAN_ComponentSrcPathsInUse :=
$(foreach _path,$(WLAN_ComponentPathsInUse), \
  $(if $(wildcard $(WLAN_TreeBaseA)/$(_path)/cfg-$(notdir $(_path)).mk),$(eval include $(WLAN_TreeBaseA)/$(_path)/cfg-$(notdir $(_path)).mk)) \
  $(eval $(notdir $(_path))_IncDirs ?= include) \
  $(eval $(notdir $(_path))_SrcDirs ?= src) \
  $(eval WLAN_ComponentIncPathsInUse += $(addprefix $(_path)/,$($(notdir $(_path))_IncDirs))) \
  $(eval WLAN_ComponentSrcPathsInUse += $(addprefix $(_path)/,$($(notdir $(_path))_SrcDirs))) \
  $(eval WLAN_ComponentBaseDir_$$(notdir $(_path)) := $$(WLAN_TreeBaseA)/$(_path)) \
)

# Global include/source path
WLAN_StdSrcDirs = src/shared src/wl/sys src/bcmcrypto
WLAN_StdSrcDirs += components/clm-api/src
WLAN_StdIncDirs = src/include components/shared
WLAN_StdIncDirs += components/clm-api/include
WLAN_SrcIncDirs = src/shared src/wl/sys src/bcmcrypto \
	src/wl/keymgmt/src src/wl/iocv/src \
	components/drivers/wl/shim/src src/wl/proxd/src src/wl/mesh/src \
	src/wl/randmac/src

WLAN_StdSrcDirsR	 = $(addprefix $(WLAN_TreeBaseR)/,$(WLAN_StdSrcDirs))
WLAN_StdIncDirsR	 = $(addprefix $(WLAN_TreeBaseR)/,$(WLAN_StdIncDirs))
WLAN_SrcIncDirsR	 = $(addprefix $(WLAN_TreeBaseR)/,$(WLAN_SrcIncDirs))
WLAN_StdIncPathR	 = $(addprefix -I,$(wildcard $(WLAN_StdIncDirsR)))
WLAN_IncDirsR		 = $(WLAN_StdIncDirsR) $(WLAN_SrcIncDirsR)
WLAN_IncPathR		 = $(addprefix -I,$(wildcard $(WLAN_IncDirsR)))

WLAN_StdSrcDirsA	 = $(addprefix $(WLAN_TreeBaseA)/,$(WLAN_StdSrcDirs))
WLAN_StdIncDirsA	 = $(addprefix $(WLAN_TreeBaseA)/,$(WLAN_StdIncDirs))
WLAN_SrcIncDirsA	 = $(addprefix $(WLAN_TreeBaseA)/,$(WLAN_SrcIncDirs))
WLAN_StdIncPathA	 = $(addprefix -I,$(wildcard $(WLAN_StdIncDirsA)))
WLAN_IncDirsA		 = $(WLAN_StdIncDirsA) $(WLAN_SrcIncDirsA)
WLAN_IncPathA		 = $(addprefix -I,$(wildcard $(WLAN_IncDirsA)))

# Public convenience macros based on WLAN_ComponentPathsInUse list.
WLAN_ComponentSrcDirsR	 = $(addprefix $(WLAN_TreeBaseR)/,$(WLAN_ComponentSrcPathsInUse))
WLAN_ComponentIncDirsR	 = $(addprefix $(WLAN_TreeBaseR)/,$(WLAN_ComponentIncPathsInUse))
WLAN_ComponentIncPathR	 = $(addprefix -I,$(wildcard $(WLAN_ComponentIncDirsR)))

WLAN_ComponentSrcDirsA	 = $(addprefix $(WLAN_TreeBaseA)/,$(WLAN_ComponentSrcPathsInUse))
WLAN_ComponentIncDirsA	 = $(addprefix $(WLAN_TreeBaseA)/,$(WLAN_ComponentIncPathsInUse))
WLAN_ComponentIncPathA	 = $(addprefix -I,$(wildcard $(WLAN_ComponentIncDirsA)))

WLAN_ComponentSrcDirs	 = $(WLAN_ComponentSrcDirsA)
WLAN_ComponentIncDirs	 = $(WLAN_ComponentIncDirsA)
WLAN_ComponentIncPath	 = $(WLAN_ComponentIncPathA)

# Dump a representative sample of derived variables in debug mode.
$(call wlan_dbgv, WLAN_TreeBaseA WLAN_TreeBaseR WLAN_SrcBaseA WLAN_SrcBaseR \
    WLAN_ComponentPathsInUse WLAN_ComponentIncPath WLAN_ComponentIncPathR \
    WLAN_ComponentSrcDirs WLAN_ComponentSrcDirsR)

# Special case for Windows to reflect CL in the build log if used.
ifdef WLAN_WINDOWS_HOST
  ifdef CL
    $(info Info: CL=$(CL))
  endif
endif

# A big hammer for debugging each shell invocation.
# Warning: this can get lost if a sub-makefile sets SHELL explicitly, and
# if so the parent should add $WLAN_ShellDebugSHELL to the call.
ifeq ($D,2)
  WLAN_ShellDebug := 1
endif
ifdef WLAN_ShellDebug
  ORIG_SHELL := $(SHELL)
  SHELL = $(strip $(warning Shell: ORIG_SHELL=$(ORIG_SHELL) PATH=$(PATH))$(ORIG_SHELL)) -x
  WLAN_ShellDebugSHELL := SHELL='$$(warning Shell: ORIG_SHELL=$$(ORIG_SHELL) PATH=$$(PATH))$(ORIG_SHELL) -x'
endif

# Variables of general utility.
WLAN_Perl := perl
WLAN_Python := python
WLAN_WINPFX ?= Z:

# These macros are used to stash an extra copy of generated source files,
# such that when a source release is made those files can be reconstituted
# from the stash during builds. Required if the generating tools or inputs
# are not shipped.
define wlan_copy_to_gen
  $(if $(WLAN_COPY_GEN),&& mkdir -p $(subst $(abspath $2),$(abspath $2/$(WLAN_GEN_BASEDIR)),$(dir $(abspath $1))) && \
    cp -pv $1 $(subst $(abspath $2),$(abspath $2/$(WLAN_GEN_BASEDIR)),$(abspath $1).GEN))
endef

################################################################
# CLM function; generates a rule to run ClmCompiler iff the XML exists.
# USAGE: $(call WLAN_GenClmCompilerRule,target-dir,src-base[,flags[,ext[,purposes_vary[,xml_dir]]]])
#   This macro uses GNU make's eval function to generate an
# explicit rule to generate a particular CLM data file each time
# it's called. Make variables which should be evaluated during eval
# processing get one $, those which must defer till "runtime" get $$.
#   The CLM "base flags" are the default minimal set, the "ext flags"
# are those which must be present for all release builds.
# The "clm_compiled" phony target is provided for makefiles which need
# to defer some other processing until CLM data is ready, and "clm_clean"
# and "CLM_DATA_FILES" make it easier for internal client makefiles to
# clean up CLM data (externally, this is treated as source and not removed).
#   The outermost conditional allows this rule to become a no-op
# in external settings where there is no XML input file while allowing
# it to turn back on automatically if an XML file is provided.
#   A vpath is used to find the XML input because this file is not allowed
# to be present in external builds. Use of vpath allows it to be "poached"
# from the internal build as necessary.
#   There are a few ways to set ClmCompiler flags: passing them as the $3
# parameter (preferred) or by overriding CLMCOMPDEFFLAGS. Additionally,
# when the make variable CLM_TYPE is defined it points to a config file
# for the compiler. The CLMCOMPEXTFLAGS variable contains "external flags"
# which must be present for all external builds. It can be forced to "" for
# debug builds.
# Note: the .c file is listed with and without a path due to the way the
# Linux kernel Makefiles generate .depend data.
#   The undocumented $5 parameter has been used for dongle testing
# against variant XML but its semantics are subject to change.
CLMCOMPDEFFLAGS ?= --region '\#a/0' --region '\#r/0' --full_set
CLMCOMPEXTFLAGS := --obfuscate
define WLAN_GenClmCompilerRule
$(eval\
.PHONY: clm_compiled clm_clean

# The "mkdir -p foo 2>/dev/null || test -d foo || mkdir -p foo" logic below is
# intended to defeat potential race conditions. If the first mkdir fails we
# check whether we just lost the race to create it; if it still doesn't exist
# there must be some other problem so we run mkdir again to generate
# the appropriate error message and die.
$1: ; mkdir -p $$@ 2>/dev/null || test -d $$@ || mkdir -p $$@
vpath wlc_clm_data$4.c $1 $$(abspath $1)
ifneq (,$(wildcard $(addsuffix /../components/clm-private/wlc_clm_data.xml,$2 $2/../../src $2/../../../src)))
  vpath wlc_clm_data.xml $(wildcard $(addsuffix /../components/clm-private,$5 $2 $2/../../src $2/../../../src))
  vpath %.clm $(addsuffix /wl/clm/types,$2 $2/../../src $2/../../../src)
  $1/wlc_clm_data$4.c: \
      $6wlc_clm_data.xml $2/../components/clm-api/include/wlc_clm_data.h $$(wildcard $2/../components/clm-bin/ClmCompiler.py) $$(if $$(CLM_TYPE),$$(CLM_TYPE).clm) | $1; \
    $$(strip $$(firstword $$(wildcard $$(addsuffix /components/clm-bin/ClmCompiler.py, $2/.. $2/../.. $2/../../..))) \
      --clmapi_include_dir $$(firstword $$(wildcard $$(addsuffix components/clm-api/include, $2/../ $2/../../ $2/../../../ ))) \
      --print_options --bcmwifi_include_dir $$(firstword $$(wildcard $$(addsuffix shared/bcmwifi/include, $2/ $2/../../src/ $2/../../../src/ ))) \
      $$(if $$(and $$(if $$(filter --config_file,$3),,x), $$(CLM_TYPE)),--config_file $$(lastword $$^) $3,$$(if $3,$3,$$(CLMCOMPDEFFLAGS))) \
      $(CLMCOMPEXTFLAGS) $$< $$@ $$(call wlan_copy_to_gen,$$@,$2))
else
  vpath %.GEN $(subst $(abspath $2),$(abspath $2/$(WLAN_GEN_BASEDIR)),$1) $(sort $(patsubst %/,%,$(dir $(wildcard $(subst $(abspath $2),$(abspath $2/$(WLAN_GEN_BASEDIR)),$(dir $1))*/*.GEN))))
  $1/%: %.GEN ; cp -pv $$< $$@
endif
  clm_compiled: $1/wlc_clm_data$4.c
  clm_clean:: ; $$(RM) $1/wlc_clm_data$4.c
  CLM_DATA_FILES += $1/wlc_clm_data$4.c
)
endef

################################################################
# Generates rule that generates wlc_sar_tbl.c file
# USAGE: $(call WLAN_GenSarTblRule,target-dir,src-base,[sar-files])
# Arguments:
# target-dir -- Directory where generated wlc_sar_tbl.c shall be put.
# src-base   -- Top level 'src' directory. Strange manipulations with
#               it within macro expansion are due to some strange
#               peculiarities of Linux driver builds
# sar_files  -- Space-separated list of base names of .csv files that
#               contain SAR entries that shall be put to generated SAR
#               table. Empty means use all files in src/wl/sar directory
define WLAN_GenSarTblRule
$(eval\
$1: ; mkdir -p $$@ 2>/dev/null || test -d $$@ || mkdir -p $$@
$1/wlc_sar_tbl.c: | $1 ; $$(firstword $$(wildcard \
    $$(addsuffix /components/tools/bin/sar_conv.py, $2/.. $2/../.. $2/../../..))) create_c \
    --driver_dir $$(addsuffix /../.., $$(firstword $$(wildcard \
    $$(addsuffix /include, $2 $2/../../src $2/../../../src)))) \
    $$(addprefix $$(firstword $$(wildcard \
    $$(addsuffix /wl/sar, $2 $2/../../src $2/../../../src)))/, \
    $$(if $3,$3,*.csv)) $$@
)
endef

################################################################

endif	# _WLAN_COMMON_MK
