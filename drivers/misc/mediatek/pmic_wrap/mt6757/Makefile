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


ifeq ($(CONFIG_MTK_PMIC_NEW_ARCH),y)
	obj-$(CONFIG_MTK_PMIC_WRAP_HAL) +=  pwrap_hal_v1.o
else
obj-$(CONFIG_MTK_PMIC_WRAP_HAL) +=  pwrap_hal.o
endif
