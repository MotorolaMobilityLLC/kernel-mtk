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

NEED_BUILD_C2K_KERNEL_PLAT_DRV=n

ifeq ($(CONFIG_MTK_ECCCI_DRIVER), y)
ifneq ($(CONFIG_MTK_ECCCI_C2K), y)
ifneq ($(CONFIG_MTK_MD3_SUPPORT),)
ifneq ($(CONFIG_MTK_MD3_SUPPORT), 0)

NEED_BUILD_C2K_KERNEL_PLAT_DRV=y

endif
endif
endif
endif

ifeq ($(NEED_BUILD_C2K_KERNEL_PLAT_DRV), y)

obj-y := c2k_platform.o c2k_sdio_platform.o

else

obj-  := dummy.o # avoid build error

endif
