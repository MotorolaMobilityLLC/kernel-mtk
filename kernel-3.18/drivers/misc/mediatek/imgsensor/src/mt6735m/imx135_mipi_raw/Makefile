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
ifeq ($(CONFIG_IMGSENSOR_USE_MODULE_DRIVER),y)
$(info imx135_otp use module build)
obj-m += imx135_otp.o
else
obj-y += imx135_otp.o
endif

obj-y += imx135mipiraw_Sensor.o


