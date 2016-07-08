#
# This makefile by default builds xp and vista drivers. It can also prefast the
# xp and vista drivers. Prefasting is typically done by release builds. By default
# new microsoft WDK toolkit is used for both XP and Vista drivers
#
# USEDDK make variable switch is provided for those who want to build XP drivers
# with old legacy DDK. To use old DDKs run "make USEDDK=1"
#
# $Id: GNUmakefile,v 1.8 2007-04-08 02:15:21 $
#

ifeq ($(USEDDK),)

## Build using new WDK toolkits

include builddriver.mk

else # USEDDK

## Old method. Build using DDK toolkits

SRCBASE = ../../..

build_all: all

include $(SRCBASE)/GNUmakefile.inc

build_amd64:
	@echo "Building amd64 xp windows driver now with DDKs"
	$(MAKE) -f driver.mk REQUIRE_WDM3790=1 obj/amd64/free/bcmwl564.sys obj/amd64/checked/bcmwl564.sys

clean_amd64:
	@echo "Cleaning amd64 windows driver now"
	$(MAKE) -f driver.mk clean

endif # USEDDK
