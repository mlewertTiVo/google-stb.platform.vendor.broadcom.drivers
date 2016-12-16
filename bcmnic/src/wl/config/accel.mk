#
# Helper makefile for building the accelerator component included in wl.mk
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
# <<Broadcom-WL-IPTag/Open:>>
#
# $Id$

ACCEL_TOP_DIR = components/accel

# HWA
ACCEL_HW_DIR = $(ACCEL_TOP_DIR)/hw

# SW simulation of the HWA
ACCEL_SW_DIR = $(ACCEL_TOP_DIR)/sw

# Files
WLFILES_SRC += $(ACCEL_SW_DIR)/src/txflow_classifier.c
