# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2020 MediaTek Inc.
IMGSENSOR_SUBDRV_OBJ := \
	$(subdrv-rpath)/mot_gnevan_sc202csmipiraw_Sensor.o \
	$(subdrv-rpath)/eeprom_i2c_sc202cs_driver.o \

ifeq ($(CONFIG_MTK_IMGSENSOR_ISP3_Z), m)
$(eval $(MTK_PLATFORM_KO)-objs += $(IMGSENSOR_SUBDRV_OBJ))
else ifeq ($(CONFIG_MTK_IMGSENSOR_ISP3_Z), y)
obj-$(CONFIG_MTK_IMGSENSOR_ISP3_Z) += $(IMGSENSOR_SUBDRV_OBJ)
endif
ifeq ($(CONFIG_MTK_IMGSENSOR_ISP4), m)
$(eval $(MTK_PLATFORM_KO)-objs += $(IMGSENSOR_SUBDRV_OBJ))
else ifeq ($(CONFIG_MTK_IMGSENSOR_ISP4), y)
obj-$(CONFIG_MTK_IMGSENSOR_ISP4) += $(IMGSENSOR_SUBDRV_OBJ)
endif
