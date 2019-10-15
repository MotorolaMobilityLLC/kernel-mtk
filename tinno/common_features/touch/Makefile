# Makefile for Linux samples code

subdir-ccflags-$(CONFIG_TINNO_PRODUCT_INFO) += -I$(srctree)/tinno/common_features/dev_info

# for PDBBW-942
ifneq ($(FEATURE_MWC_WIKO),yes)
obj-$(CONFIG_MTK_FOCALTECH_TOUCH_TS)   += focaltech_touch_screen/
endif

obj-$(CONFIG_MTK_TGESTURE)   += tgesture/

