#
# Linux router libbcmcrypto Makefile
#
# Broadcom Proprietary and Confidential. Copyright (C) 2016,
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom.
#
#
# <<Broadcom-WL-IPTag/Proprietary:>>
#
# $Id: Makefile 520342 2014-12-11 05:39:44Z $
#

include $(TOP)/.config

CFLAGS	+= -Wsign-compare -I. -I../httpd -I$(SRCBASE)/include ${WLAN_StdIncPathA} -I$(SRCBASE)/../components/shared -I$(SRCBASE)/common/include -Wall -fPIC
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s
LDFLAGS += -L.


vpath %.c $(SRCBASE)/bcmcrypto
vpath %.o $(SRCBASE_ROUTER)/libbcmcrypto/prebuilt

OBJS := aes.o aeskeywrap.o rijndael-alg-fst.o dh.o bn.o sha1.o passhash.o prf.o md5.o hmac.o rc4.o random.o
# For customer backwords compatibility, for now we keep CONFIG_WSCCMD here and can be removed when
# all customers migrate to new SDK (no CONFIG_WSCCMD in configration files).
ifneq ($(CONFIG_WPS)$(CONFIG_WSCCMD)$(CONFIG_MFP),)
OBJS += sha256.o hmac_sha256.o
endif

# Find all ELF binaries (including extra libraries)
BINARIES := $(shell find $(TARGETDIR) -path $(TARGETDIR)/lib -prune -o -type f -print | file -f - | grep ELF | cut -d':' -f1)
MAP := .map
UNR := .unresolved
SYM := .symbols

all: libbcmcrypto.so

install: all
	install -d $(INSTALLDIR)/usr/lib
	install -m 755 libbcmcrypto.so $(INSTALLDIR)/usr/lib
	$(STRIP) $(INSTALLDIR)/usr/lib/libbcmcrypto.so

clean:
	rm -f *.o *.so *.a $(MAP) $(UNR) $(SYM)

libbcmcrypto.so: $(OBJS)
	$(LD) -shared -o $@ $^

# rule for removing unneeded symbols in the shared library
optimize: libbcmcrypto.so
	$(NM) -o --defined-only --no-sort libbcmcrypto.so | cut -d' ' -f3 > $(MAP)
ifeq ($(MKSYM),)
	$(NM) --dynamic -u --no-sort $(BINARIES) | sort -u > $(UNR)
	rm -rf $(SYM)
	for symbol in `cat $(UNR)` ; do \
	if grep -q "^$${symbol}$$" $(MAP) ; then echo "-u $${symbol}" >> $(SYM) ;  \
	fi ; done
else
	$(NM) --dynamic -u --no-sort $(BINARIES) | sort -u | $(MKSYM) $(MAP) > $(SYM)
endif
	# if no symbols are needed then delete the shared lib
	if ls $(SYM) ; then \
	$(AR) cr libbcmcrypto.a $(OBJS) ; \
	xargs -t $(LD) -shared -o libbcmcrypto.so libbcmcrypto.a < $(SYM) ; \
	install libbcmcrypto.so $(TARGETDIR)/usr/lib ; \
	$(STRIP) $(TARGETDIR)/usr/lib/libbcmcrypto.so ; \
	else \
	rm $(TARGETDIR)/usr/lib/libbcmcrypto.so ; fi
