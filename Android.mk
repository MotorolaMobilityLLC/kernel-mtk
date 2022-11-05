# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2019 MediaTek Inc.

LOCAL_PATH := $(call my-dir)

ifeq ($(notdir $(LOCAL_PATH)),$(strip $(LINUX_KERNEL_VERSION)))
ifneq ($(strip $(TARGET_NO_KERNEL)),true)

include $(LOCAL_PATH)/kenv.mk

ifeq ($(MOTO_PRODUCT_HAWAIIPL),yes)
KERNEL_JOURNEY_CFLAGS += -DHAWAIIPL_P411AE
$(warning "MOTO_PRODUCT_HAWAIIPL yes")
else
$(warning "MOTO_PRODUCT_HAWAIIPL no")
endif

# fenghui.zou add for JOURNEY_BUILD_SCRIPT
ifeq ($(JOURNEY_BUILD_SCRIPT),yes)

    KERNEL_JOURNEY_CFLAGS += $(COMMON_JOURNEY_CFLAGS)

    ifeq ($(JOURNEY_FEATURE_LOG_AEE_NO_RESERVED), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_LOG_AEE_NO_RESERVED
    endif

    ifeq ($(JOURNEY_FEATURE_USE_RESERVED_DISK), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_USE_RESERVED_DISK
    endif

    #xiaoyan.yu, add for tracking cpu high loading info
    ifeq ($(JOURNEY_FEATURE_TRACK_CPU_HIGH_LOADING), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_TRACK_CPU_HIGH_LOADING
    endif

    #xiaoyan.yu, add for zram disksize compatible
    ifeq ($(JOURNEY_FEATURE_ZRAM_SIZE_COMPATIBLE), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_ZRAM_SIZE_COMPATIBLE
    endif

    ifeq ($(JOURNEY_FEATURE_OPTIMIZE_ALMK), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_OPTIMIZE_ALMK
    endif

    ifeq ($(JOURNEY_FEATURE_OPTIMIZE_SWAPPINESS), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_OPTIMIZE_SWAPPINESS
    endif

    ifeq ($(JOURNEY_FEATURE_REDUCE_LOG), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_REDUCE_LOG
    endif

    ifeq ($(JOURNEY_DOU_VERSION), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_DOU_VERSION
    endif
    ifeq ($(JOURNEY_FEATURE_FACTORY_VERSION), yes)
    KERNEL_JOURNEY_CFLAGS += -DJOURNEY_FEATURE_FACTORY_VERSION
    endif

    KERNEL_MAKE_OPTION += KERNEL_JOURNEY_CFLAGS="$(KERNEL_JOURNEY_CFLAGS)"
#$(warning KERNEL_MAKE_OPTION $(KERNEL_MAKE_OPTION))
else
    $(error journey build script error)
endif

ifeq ($(wildcard $(TARGET_PREBUILT_KERNEL)),)
KERNEL_MAKE_DEPENDENCIES := $(shell find $(KERNEL_DIR) -name .git -prune -o -type f | sort)

$(TARGET_KERNEL_CONFIG): PRIVATE_DIR := $(KERNEL_DIR)
$(TARGET_KERNEL_CONFIG): $(KERNEL_CONFIG_FILE) $(LOCAL_PATH)/Android.mk
$(TARGET_KERNEL_CONFIG): $(KERNEL_MAKE_DEPENDENCIES)
	$(hide) mkdir -p $(dir $@)
	$(PREBUILT_MAKE_PREFIX)$(MAKE) -C $(PRIVATE_DIR) $(KERNEL_MAKE_OPTION) $(KERNEL_DEFCONFIG)

.KATI_RESTAT: $(KERNEL_ZIMAGE_OUT)
$(KERNEL_ZIMAGE_OUT): PRIVATE_DIR := $(KERNEL_DIR)
$(KERNEL_ZIMAGE_OUT): $(TARGET_KERNEL_CONFIG) $(KERNEL_MAKE_DEPENDENCIES)
	$(hide) mkdir -p $(dir $@)
	$(PREBUILT_MAKE_PREFIX)$(MAKE) -C $(PRIVATE_DIR) $(KERNEL_MAKE_OPTION)
	$(hide) $(call fixup-kernel-cmd-file,$(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/compressed/.piggy.xzkern.cmd)
	# check the kernel image size
	python device/mediatek/build/build/tools/check_kernel_size.py $(KERNEL_OUT) $(KERNEL_DIR) $(PROJECT_DTB_NAMES)

$(BUILT_KERNEL_TARGET): $(KERNEL_ZIMAGE_OUT) $(TARGET_KERNEL_CONFIG) $(LOCAL_PATH)/Android.mk | $(ACP)
	$(copy-file-to-target)

$(TARGET_PREBUILT_KERNEL): $(BUILT_KERNEL_TARGET) $(LOCAL_PATH)/Android.mk | $(ACP)
	$(copy-file-to-new-target)

endif #TARGET_PREBUILT_KERNEL is empty

$(INSTALLED_KERNEL_TARGET): $(BUILT_KERNEL_TARGET) $(LOCAL_PATH)/Android.mk | $(ACP)
	$(copy-file-to-target)

.PHONY: kernel save-kernel kernel-savedefconfig kernel-menuconfig menuconfig-kernel savedefconfig-kernel clean-kernel
kernel: $(INSTALLED_KERNEL_TARGET) $(INSTALLED_MTK_DTB_TARGET)
save-kernel: $(TARGET_PREBUILT_KERNEL)

kernel-savedefconfig: $(TARGET_KERNEL_CONFIG)
	cp $(TARGET_KERNEL_CONFIG) $(KERNEL_CONFIG_FILE)

kernel-menuconfig:
	$(hide) mkdir -p $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) $(KERNEL_MAKE_OPTION) menuconfig

menuconfig-kernel savedefconfig-kernel:
	$(hide) mkdir -p $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) $(KERNEL_MAKE_OPTION) $(patsubst %config-kernel,%config,$@)

clean-kernel:
	$(hide) rm -rf $(KERNEL_OUT) $(INSTALLED_KERNEL_TARGET)

### DTB build template
MTK_DTBIMAGE_DTS := $(addsuffix .dts,$(addprefix $(KERNEL_DIR)/arch/$(KERNEL_TARGET_ARCH)/boot/dts/,$(PLATFORM_DTB_NAME)))
include device/mediatek/build/core/build_dtbimage.mk

MTK_DTBOIMAGE_DTS := $(addsuffix .dts,$(addprefix $(KERNEL_DIR)/arch/$(KERNEL_TARGET_ARCH)/boot/dts/,$(PROJECT_DTB_NAMES)))
include device/mediatek/build/core/build_dtboimage.mk

endif #TARGET_NO_KERNEL
endif #LINUX_KERNEL_VERSION
