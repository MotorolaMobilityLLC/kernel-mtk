DEFCONFIGSRC			:= $(KERNEL_DIR)/arch/$(KERNEL_TARGET_ARCH)/configs
LJAPDEFCONFIGSRC		:= ${DEFCONFIGSRC}/ext_config
KERNEL_DEFCONFIG        := $(firstword $(KERNEL_DEFCONFIG))

DEFCONFIG_BASENAME		:= $(subst _defconfig,,$(KERNEL_DEFCONFIG))
ifneq ($(TARGET_BUILD_VARIANT), user)
DEFCONFIG_BASENAME		:= $(subst _debug,,$(DEFCONFIG_BASENAME))
endif
TARGET_DEFCONFIG		:= $(KERNEL_DIR)/kernel/configs/moto_extra.config
#TARGET_DEFCONFIG        := $(KERNEL_OUT)/mapphone_defconfig
PRODUCT_SPECIFIC_DEFCONFIGS	:= $(LJAPDEFCONFIGSRC)/moto-$(DEFCONFIG_BASENAME).config

KERNEL_DEBUG_DEFCONFIG          := $(LJAPDEFCONFIGSRC)/debug-$(DEFCONFIG_BASENAME).config
PRODUCT_KERNEL_DEBUG_DEFCONFIG  := $(LJAPDEFCONFIGSRC)/$(PRODUCT_DEBUG_DEFCONFIG)
FACTORY_DEFCONFIG		:= $(LJAPDEFCONFIGSRC)/factory-$(DEFCONFIG_BASENAME).config

# add debug config file for non-user build
ifneq ($(TARGET_BUILD_VARIANT), user)
ifneq ($(wildcard $(KERNEL_DEBUG_DEFCONFIG)),)
PRODUCT_SPECIFIC_DEFCONFIGS += $(KERNEL_DEBUG_DEFCONFIG)
endif

# Add a product-specific debug defconfig, too
ifneq ($(PRODUCT_DEBUG_DEFCONFIG),)
PRODUCT_SPECIFIC_DEFCONFIGS += $(PRODUCT_KERNEL_DEBUG_DEFCONFIG)
endif
endif

ifeq ($(TARGET_FACTORY_DEFCONFIG), true)
PRODUCT_SPECIFIC_DEFCONFIGS += $(FACTORY_DEFCONFIG)
endif

# append all additional configs
ifneq ($(KERNEL_EXTRA_CONFIG),)
PRODUCT_SPECIFIC_DEFCONFIGS += $(KERNEL_EXTRA_CONFIG:%=$(LJAPDEFCONFIGSRC)/%.config)
endif

#some configs need adjust on df build,but not final builds.
#Actually df bulids include user/userdebug,so we use one specifci config files to
#adjust such configs.
ifneq ($(PRODUCT_IS_PRODUCTION),true)
PRODUCT_SPECIFIC_DEFCONFIGS += $(LJAPDEFCONFIGSRC)/moto-$(DEFCONFIG_BASENAME)-df.config
endif

DEFCONFIG_OVERLAYS = $(wildcard $(PRODUCT_SPECIFIC_DEFCONFIGS))
$(foreach config, $(DEFCONFIG_OVERLAYS), $(shell ln -sf $(abspath $(config)) $(KERNEL_DIR)/kernel/configs/))

KERNEL_DEFCONFIG_OVERLAYS := $(notdir $(DEFCONFIG_OVERLAYS))

define do-make-defconfig
	$(hide) mkdir -p $(dir $(1))
	$(hide) printf "# This file was automatically generated from:\n" > $(1);
	$(hide) $(foreach f, $(2), printf "#\t" >> $(1); echo $(f) >> $(1);)
	$(hide) printf "\n" >> $(1)
	$(hide) cat $(2) >> $(1)
endef

#
# make combined defconfig file
#---------------------------------------
$(TARGET_DEFCONFIG): FORCE $(PRODUCT_SPECIFIC_DEFCONFIGS)
#	$(call do-make-defconfig,$@,$(PRODUCT_SPECIFIC_DEFCONFIGS))

.PHONY: FORCE
FORCE:
