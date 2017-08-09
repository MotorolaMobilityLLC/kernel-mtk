ifdef MTK_PLATFORM

PRIVATE_CUSTOM_KERNEL_DCT := $(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)
ifneq ($(wildcard $(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws),)
  DRVGEN_PATH := arch/arm/mach-$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
else
  DRVGEN_PATH := drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
endif
ifndef DRVGEN_OUT
DRVGEN_OUT := $(objtree)/$(DRVGEN_PATH)
endif

DRVGEN_CUSTOM_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts/custom/

export DRVGEN_OUT

DRVGEN_OUT_PATH := $(DRVGEN_OUT)/inc

  ALL_DRVGEN_FILE := cust_eint.dtsi
  ALL_DRVGEN_FILE += cust_adc.dtsi
  ALL_DRVGEN_FILE += cust_i2c.dtsi
  ALL_DRVGEN_FILE += cust_md1_eint.dtsi
  ALL_DRVGEN_FILE += cust_kpd.dtsi
  ALL_DRVGEN_FILE += cust_clk_buf.dtsi
  ALL_DRVGEN_FILE += cust_gpio.dtsi
  ALL_DRVGEN_FILE += cust_pmic.dtsi

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

$(DRVGEN_OUT)/cust_eint.dtsi: $(DWS_FILE)
	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) eint_dtsi

$(DRVGEN_OUT)/inc/pmic_drv.c: $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) pmic_c

$(DRVGEN_OUT)/cust_i2c.dtsi: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) i2c_dtsi

$(DRVGEN_OUT)/cust_adc.dtsi: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) adc_dtsi

$(DRVGEN_OUT)/cust_md1_eint.dtsi: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) md1_eint_dtsi

$(DRVGEN_OUT)/cust_kpd.dtsi: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) kpd_dtsi

$(DRVGEN_OUT)/cust_clk_buf.dtsi: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) clk_buf_dtsi

$(DRVGEN_OUT)/cust_gpio.dtsi: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) gpio_dtsi

$(DRVGEN_OUT)/cust_pmic.dtsi: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_CUSTOM_OUT) $(DRVGEN_OUT_PATH) pmic_dtsi

$(DRVGEN_OUT)/inc/mt6735-pinfunc.h: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) mt6735_pinfunc_h

$(DRVGEN_OUT)/inc/pinctrl-mtk-mt6735.h: $(DWS_FILE)
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) pinctrl_mtk_mt6735_h

else
$(DRVGEN_FILE_LIST): $(DRVGEN_OUT)/% : $(DRVGEN_PREBUILT_PATH)/%
#	@mkdir -p $(dir $@)
	@mkdir -p $(DRVGEN_CUSTOM_OUT)
	cp -f $< $@
endif

endif#MTK_PLATFORM
