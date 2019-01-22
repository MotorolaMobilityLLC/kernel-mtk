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

$(info "CONFIG_MICROTRUST_TEE_SUPPORT=$(CONFIG_MICROTRUST_TEE_SUPPORT)")
$(info "CONFIG_MICROTRUST_TZ_DRIVER=$(CONFIG_MICROTRUST_TZ_DRIVER)")
$(info "CONFIG_MICROTRUST_VFS_DRIVER=$(CONFIG_MICROTRUST_VFS_DRIVER)")
$(info "CONFIG_MICROTRUST_FP_DRIVER=$(CONFIG_MICROTRUST_FP_DRIVER)")
$(info "CONFIG_MICROTRUST_DEBUG=$(CONFIG_MICROTRUST_DEBUG)")

subdir-ccflags-y  += -I$(srctree)/drivers/misc/mediatek/teei/V1.0/common/include

obj-$(CONFIG_MICROTRUST_TZ_DRIVER) += tz_driver/
obj-$(CONFIG_MICROTRUST_VFS_DRIVER) += tz_vfs/
obj-$(CONFIG_MICROTRUST_FP_DRIVER) += teei_fp/
obj-$(CONFIG_MICROTRUST_KEYMASTER_DRIVER) += ut_keymaster/

