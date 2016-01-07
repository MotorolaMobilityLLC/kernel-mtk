ifdef MTK_PLATFORM

PRIVATE_CUSTOM_KERNEL_DCT := $(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)
ifneq ($(wildcard $(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws),)
  DRVGEN_PATH := arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
else
  DRVGEN_PATH := drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
endif
ifndef DRVGEN_OUT
#DRVGEN_OUT := $(objtree)/$(DRVGEN_PATH)
DRVGEN_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts
endif
export DRVGEN_OUT

ALL_DRVGEN_FILE := cust.dtsi

DWS_FILE := $(srctree)/$(DRVGEN_PATH)/codegen.dws
ifneq ($(wildcard $(DWS_FILE)),)
DRVGEN_FILE_LIST := $(addprefix $(DRVGEN_OUT)/,$(ALL_DRVGEN_FILE))
else
DRVGEN_FILE_LIST :=
endif
DRVGEN_TOOL := $(srctree)/tools/dct/DrvGen
DRVGEN_PREBUILT_PATH := $(srctree)/$(DRVGEN_PATH)
DRVGEN_PREBUILT_CHECK := $(filter-out $(wildcard $(addprefix $(DRVGEN_PREBUILT_PATH)/,$(ALL_DRVGEN_FILE))),$(addprefix $(DRVGEN_PREBUILT_PATH)/,$(ALL_DRVGEN_FILE)))

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST)
ifneq ($(DRVGEN_PREBUILT_CHECK),)
$(DRVGEN_OUT)/cust.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	$(DRVGEN_TOOL) $(DWS_FILE) $(dir $@) $(dir $@) cust_dtsi

else
$(DRVGEN_FILE_LIST): $(DRVGEN_OUT)/% : $(DRVGEN_PREBUILT_PATH)/%
	@mkdir -p $(dir $@)
	cp -f $< $@
endif

endif#MTK_PLATFORM
