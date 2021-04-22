# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_MTK_CAM_CAL) += src/
CUSTOM_KERNEL_FLAGS := $(addprefix -D, $(shell echo \
             $(CONFIG_CUSTOM_KERNEL_IMGSENSOR) | tr a-z A-Z))
subdir-ccflags-y += $(CUSTOM_KERNEL_FLAGS)
