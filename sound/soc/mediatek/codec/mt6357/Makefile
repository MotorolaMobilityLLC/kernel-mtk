# PMIC object
ifneq ($(filter malta malta_64,$(TARGET_PRODUCT)),)
$(info ++++++++++++++++++++++++++firefly product target:$(TARGET_PRODUCT)++++++++++++++++)
ifeq ($(BUILD_SMT),YES)
  ccflags-y += -DSMT_VERSION
  $(info ++++++++++++++++++++++++++firefly++++++++++++++++ SMT version)
endif
endif
obj-y += mtk-soc-codec-6357.o mtk-auddrv-ana.o
