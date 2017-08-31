#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/power/mediatek/charger
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/usb_c/mt6336

ccflags-y += -I$(srctree)/drivers/misc/mediatek/flashlight/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/
ccflags-y += -I$(srctree)/drivers/power/mediatek/
ccflags-y += -I$(srctree)/drivers/power/mediatek/battery

ifeq ($(CONFIG_MTK_GAUGE_VERSION),30)
	obj-y += mtk_pe30_hal.o mtk_battery_intf.o
else
	obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter_hal.o
ifeq ($(CONFIG_MTK_SMART_BATTERY),y)
	ifeq ($(CONFIG_MTK_SWCHR_SUPPORT),y)
		obj-$(CONFIG_MTK_BQ24261_SUPPORT) += bq24261.o charging_hw_bq24261.o
		obj-$(CONFIG_MTK_BQ24296_SUPPORT) += bq24296.o charging_hw_bq24296.o
		obj-$(CONFIG_MTK_NCP1854_SUPPORT) += ncp1854.o charging_hw_ncp1854.o
		obj-$(CONFIG_MTK_FAN5405_SUPPORT) += fan5405.o charging_hw_fan5405.o
		obj-$(CONFIG_MTK_BQ25896_SUPPORT) += bq25890.o charging_hw_bq25890.o
		obj-$(CONFIG_MTK_RT9466_SUPPORT) += rt9466.o mtk_charger_intf.o
	endif
endif
endif


