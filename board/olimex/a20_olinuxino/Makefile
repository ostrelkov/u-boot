# SPDX-License-Identifier: GPL-2.0+
#
# Makefile
#
# Copyright (C) 2018 Olimex Ltd.

obj-y   += a20_olinuxino.o
obj-y   += board_detect.o

obj-$(CONFIG_OF_BOARD_FIXUP)    += fdt.o
obj-$(CONFIG_SUNXI_AHCI)        += ../../sunxi/ahci.o
obj-$(CONFIG_MACH_SUN7I)        += ../../sunxi/dram_sun5i_auto.o

ifndef CONFIG_SPL_BUILD
obj-y   += boards.o
endif
