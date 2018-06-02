ifdef MTK_PLATFORM
DRVGEN_PATH := drivers/misc/mediatek/dws/$(MTK_PLATFORM)

ifndef DRVGEN_OUT
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
DRVGEN_TOOL := $(srctree)/tools/dct/DrvGen.py

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST)
$(DRVGEN_OUT)/cust.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(dir $@) $(dir $@) cust_dtsi

endif#MTK_PLATFORM
