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

ccflags-y += -I$(srctree)/drivers/misc/mediatek/flashlight/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/

ifneq ($(CONFIG_MTK_PMIC_NEW_ARCH),y)
obj-$(CONFIG_MTK_PMIC)	+= pmic.o pmic_irq.o upmu_common.o \
			pmic_initial_setting.o pmic_auxadc.o pmic_chr_type_det.o
obj-$(CONFIG_MTK_PMIC)  += mt6311.o fan53526.o fan53528buc08x.o
obj-$(CONFIG_MTK_PMIC)  += pmic_adb_dbg.o
endif

obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter_hal.o mtk_bif_intf.o
obj-$(CONFIG_MTK_CHARGER_INTERFACE) += mtk_charger_intf.o

ifeq ($(CONFIG_MTK_SMART_BATTERY),y)
	ifeq ($(CONFIG_MTK_SWCHR_SUPPORT),y)
		obj-$(CONFIG_MTK_BQ24261_SUPPORT) += bq24261.o charging_hw_bq24261.o
		obj-$(CONFIG_MTK_BQ24296_SUPPORT) += bq24296.o charging_hw_bq24296.o
		obj-$(CONFIG_MTK_NCP1854_SUPPORT) += ncp1854.o charging_hw_ncp1854.o
		obj-$(CONFIG_MTK_FAN5405_SUPPORT) += fan5405.o charging_hw_fan5405.o
		obj-$(CONFIG_MTK_BQ25896_SUPPORT) += bq25890.o charging_hw_bq25890.o
		obj-$(CONFIG_MTK_RT9466_SUPPORT) += rt9466.o
		obj-$(CONFIG_MTK_RT9468_SUPPORT) += rt9468.o
	else
		obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_pmic.o
	endif
endif
