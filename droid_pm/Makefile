############################################################
# Copyright (C) 2015 Broadcom
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, you may obtain a copy at
# http://www.broadcom.com/licenses/LGPLv2.1.php or by writing to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301  USA
############################################################

PWD := $(shell pwd)

ifeq ($(BSEAV_TOP),)
$(error BSEAV_TOP PATH needs to be defined)
endif

ifeq ($(LINUX_OUT),)
$(error LINUX_OUT path needs to be defined)
endif

obj-m += droid_pm.o
ccflags-y += -I$(BSEAV_TOP)/linux/driver

install: all
	cp $(PWD)/droid_pm.ko $(INSTALL_DIR)

all:
	@${MAKE} -C $(LINUX_OUT) M=$(PWD) modules

clean:
	@${MAKE} -C $(LINUX_OUT) M=$(PWD) clean

