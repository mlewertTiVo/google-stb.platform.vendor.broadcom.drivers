# Makefile for phy_radar_shared unit test.
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
# <<Broadcom-WL-IPTag/Proprietary:.*>>
#
# $Id:$

include ../../../../../src/makefiles/WLAN_Common.mk

#####################################################################################
##################### Following items require developer customization. ##############
#####################################################################################

# Source file path
SOURCE = ../phy_radar_shared.c ../phy_radar_utils.c ../../../ac/radar/phy_ac_radar.c

# Header file path
INCLUDE = $(WLAN_ComponentIncDirsR) $(WLAN_IncDirsR)

# Unit tests file path
TEST_SRC = main.c test_phy_radar.c

# Check tool directory path (from the unit test source directory)
CHECK_ROOT_PATH = ../../../../../src/unittest

DEBUGS = -DBCMDBG_ERR -DBCMDBG_ASSERT -DBCMDBG_EXCLUDE_HW_TIMESTAMP #-DBCMDBG

# Relevant flags for phy_radar_shared.c compilation.
CFLAGS += -DWL_UNITTEST -DBCMPHYCORENUM=2 -DWL11AC160 $(DEBUGS)

#####################################################################################
##################### None customized parameter #####################################
#####################################################################################

# Path to central make file
include $(CHECK_ROOT_PATH)/unittest.mk
