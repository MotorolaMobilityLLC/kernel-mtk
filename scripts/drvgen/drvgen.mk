ifdef MTK_PLATFORM

PRIVATE_CUSTOM_KERNEL_DCT := $(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)
ifneq ($(wildcard $(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws),)
  DRVGEN_PATH := arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
  DWS_FILE := $(srctree)/$(DRVGEN_PATH)/$(MTK_PROJECT).dws
else
  ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws),)
    DRVGEN_PATH := drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
    DWS_FILE := $(srctree)/$(DRVGEN_PATH)/codegen.dws
  else
    DRVGEN_PATH := drivers/misc/mediatek/dws/$(MTK_PLATFORM)
    DWS_FILE := $(srctree)/$(DRVGEN_PATH)/$(MTK_PROJECT).dws
  endif
endif

ifeq ($(strip $(CONFIG_MTK_DTBO_FEATURE)), y)
DTBO_NAME := $(subst $\",,$(if $(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE),$(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES),$(CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES)))
DTBO_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts
DTS_OVERLAY := $(DTBO_OUT)/$(DTBO_NAME).dts

export DTS_OVERLAY
export DTBO_NAME
export DTBO_OUT
endif

ifndef DRVGEN_OUT
#DRVGEN_OUT := $(objtree)/$(DRVGEN_PATH)
DRVGEN_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts
endif
export DRVGEN_OUT

ALL_DRVGEN_FILE := cust.dtsi

ifneq ($(wildcard $(DWS_FILE)),)
DRVGEN_FILE_LIST := $(addprefix $(DRVGEN_OUT)/,$(ALL_DRVGEN_FILE))
DRVGEN_FILE_LIST += $(DTS_OVERLAY)
else
DRVGEN_FILE_LIST :=
endif
DRVGEN_TOOL := $(srctree)/tools/dct/DrvGen.py

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST)
$(DRVGEN_OUT)/cust.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(dir $@) $(dir $@) cust_dtsi
ifeq ($(strip $(CONFIG_MTK_DTBO_FEATURE)), y)
	echo "DTB OVERLAY:" $(DTS_OVERLAY)
DTB_OVERLAY_IMAGE_TAGERT := $(DTBO_OUT)/dtbo.img
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_DTB_OVERLAY_OBJ:=$(basename $(DTS_OVERLAY)).dtb
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_DTB_OVERLAY_HDR:=$(srctree)/scripts/dtbo.cfg
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_MKIMAGE_TOOL:=$(srctree)/scripts/mkimage
$(DTB_OVERLAY_IMAGE_TAGERT) : $(PRIVATE_DTB_OVERLAY_OBJ) dtbs $(PRIVATE_DTB_OVERLAY_HDR) | $(PRIVATE_MKIMAGE_TOOL)
	@echo Singing the generated overlay dtbo.
	$(PRIVATE_MKIMAGE_TOOL) $(PRIVATE_DTB_OVERLAY_OBJ) $(PRIVATE_DTB_OVERLAY_HDR)  > $@

.PHONY: dtboimage dtbs
dtboimage : $(DTB_OVERLAY_IMAGE_TAGERT)
endif

endif#MTK_PLATFORM
