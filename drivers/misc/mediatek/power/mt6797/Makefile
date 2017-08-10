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

# obj-$(CONFIG_MTK_PMIC)        += mt6311.o
obj-$(CONFIG_MTK_PMIC)          += pmic.o pmic_irq.o upmu_common.o pmic_auxadc.o pmic_initial_setting.o fan53555.o da9214.o fan49101.o pmic_chr_type_det.o
obj-$(CONFIG_REGULATOR_RT5735)	+= rt5735.o mtk_gpuregulator_intf.o
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ifeq ($(CONFIG_MTK_PMIC_DVT_SUPPORT),y)
     obj-$(CONFIG_MTK_PMIC)     += pmic_dvt.o
endif

obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter_hal.o
obj-$(CONFIG_MTK_SMART_BATTERY) += mtk_bif_intf.o

ifeq ($(CONFIG_MTK_BQ24261_SUPPORT),y)
     obj-$(CONFIG_MTK_SMART_BATTERY) += bq24261.o charging_hw_bq24261.o
else
     ifeq ($(CONFIG_MTK_SWCHR_SUPPORT),y)
          obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_mt_sw_charger.o
     else
          ifeq ($(CONFIG_MTK_NCP1854_SUPPORT),y)
               obj-$(CONFIG_MTK_SMART_BATTERY) += ncp1854.o charging_hw_ncp1854.o
          else
               ifeq ($(CONFIG_MTK_BQ24296_SUPPORT),y)
                    obj-$(CONFIG_MTK_SMART_BATTERY) += bq24296.o charging_hw_bq24296.o
               else
                    ifeq ($(CONFIG_MTK_FAN5405_SUPPORT),y)
                        obj-$(CONFIG_MTK_SMART_BATTERY) += fan5405.o charging_hw_fan5405.o
                    else
                          ifeq ($(CONFIG_MTK_BQ25896_SUPPORT),y)
                              obj-$(CONFIG_MTK_SMART_BATTERY) += bq25890.o charging_hw_bq25890.o
                          else
                              ifeq ($(CONFIG_MTK_BQ25898_DUAL_SUPPORT),y)
                                obj-$(CONFIG_MTK_SMART_BATTERY) += bq25898.o charging_hw_bq25898.o
                              else
                                obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_pmic.o
                          endif
		    endif
               endif
          endif
     endif
endif
endif
