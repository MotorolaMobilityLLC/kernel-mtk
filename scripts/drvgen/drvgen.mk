ifdef MTK_PLATFORM

PRIVATE_CUSTOM_KERNEL_DCT := $(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)
ifneq ($(wildcard $(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws),)
  DRVGEN_PATH := arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
else
  ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws),)
    DRVGEN_PATH := drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
  else
    DRVGEN_PATH := drivers/misc/mediatek/dws/$(MTK_PLATFORM)
  endif
endif
ifndef DRVGEN_OUT
#DRVGEN_OUT := $(objtree)/$(DRVGEN_PATH)
DRVGEN_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts
endif
export DRVGEN_OUT

ALL_DRVGEN_FILE := cust.dtsi

DWS_FILE := $(srctree)/$(DRVGEN_PATH)/$(MTK_PROJECT).dws
ifneq ($(wildcard $(DWS_FILE)),)
DRVGEN_FILE_LIST := $(addprefix $(DRVGEN_OUT)/,$(ALL_DRVGEN_FILE))
else
DRVGEN_FILE_LIST :=
endif
DRVGEN_TOOL := $(srctree)/tools/dct/DrvGen

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST)
$(DRVGEN_OUT)/cust.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	$(DRVGEN_TOOL) $(DWS_FILE) $(dir $@) $(dir $@) cust_dtsi

endif#MTK_PLATFORM
