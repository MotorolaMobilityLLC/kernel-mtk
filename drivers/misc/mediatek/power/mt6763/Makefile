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
ccflags-y += -I$(srctree)/drivers/misc/mediatek/flashlight/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/usb_c/tcpc/inc
subdir-ccflags-$(CONFIG_USB_MU3D_DRV) += -Werror -I$(srctree)/drivers/misc/mediatek/mu3d/drv
subdir-ccflags-$(CONFIG_USB_MTK_HDRC) += -I$(srctree)/drivers/misc/mediatek/usb20/
ccflags-y += -I$(srctree)/drivers/power/mediatek/
ccflags-y += -I$(srctree)/drivers/power/mediatek/battery

ifneq ($(CONFIG_MTK_PMIC_NEW_ARCH),y)
obj-$(CONFIG_MTK_PMIC)	+= pmic.o pmic_irq.o upmu_common.o \
			pmic_initial_setting.o pmic_auxadc.o pmic_chr_type_det.o
obj-$(CONFIG_MTK_PMIC)  += mt6311.o fan53526.o fan53528buc08x.o
obj-$(CONFIG_MTK_PMIC)  += pmic_adb_dbg.o
endif

ifeq ($(CONFIG_MTK_GAUGE_VERSION),30)
	obj-$(CONFIG_MTK_SMART_BATTERY) += mtk_battery_intf.o
	obj-$(CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT) += mtk_pe30_hal.o
else
	obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter_hal.o mtk_bif_intf.o
	obj-$(CONFIG_MTK_CHARGER_INTERFACE) += mtk_charger_intf.o
	ifeq ($(CONFIG_MTK_SMART_BATTERY),y)
		ifeq ($(CONFIG_MTK_SWCHR_SUPPORT),y)
			obj-$(CONFIG_MTK_BQ24261_SUPPORT) += bq24261.o charging_hw_bq24261.o
			obj-$(CONFIG_MTK_BQ24296_SUPPORT) += bq24296.o charging_hw_bq24296.o
			obj-$(CONFIG_MTK_NCP1854_SUPPORT) += ncp1854.o charging_hw_ncp1854.o
			obj-$(CONFIG_MTK_FAN5405_SUPPORT) += fan5405.o charging_hw_fan5405.o
			obj-$(CONFIG_MTK_BQ25896_SUPPORT) += bq25896.o charging_hw_bq25896.o
			obj-$(CONFIG_MTK_BQ25890_SUPPORT) += bq25890.o charging_hw_bq25890.o
			obj-$(CONFIG_MTK_RT9466_SUPPORT) += rt9466.o
			obj-$(CONFIG_MTK_RT9467_SUPPORT) += rt9467.o
			obj-$(CONFIG_MTK_RT9468_SUPPORT) += rt9468.o rt9468_shuttle.o
		else
			obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_pmic.o
		endif
	endif
endif

obj-$(CONFIG_USB_POWER_DELIVERY) += rt_pd_manager.o
