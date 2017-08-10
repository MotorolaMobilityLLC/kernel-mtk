# Copyright (c) 2015-2016 MICROTRUST Incorporated
# All Rights Reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/GT9XX_hotknot/
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/GT9XX_hotknot/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT9XX_hotknot/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/teei/V1.0/tz_driver/include/


teei-objs += teei_client_main.o
#teei-objs += global_function.o
teei-objs += backward_driver.o irq_register.o notify_queue.o teei_capi.o teei_fp.o teei_smc_call.o switch_queue.o teei_keymaster.o teei_gatekeeper.o teei_cancel_cmd.o

ifeq ($(CONFIG_MICROTRUST_TUI_DRIVER),y)
    teei-objs += utr_tui_cmd.o
    ccflags-y += -D TUI_SUPPORT
endif

ifeq ($(CONFIG_MICROTRUST_TZ_LOG),y)
	teei-objs += tz_log.o
else
	teei-objs += tlog.o
endif

obj-$(CONFIG_MICROTRUST_TZ_DRIVER) += teei.o

ccflags-y += -DDEBUG_DO -DIMSG_TAG='"[tz_driver]"'
ccflags-y += -Wall -Werror
ccflags-y += -DUT_DMA_ZONE
