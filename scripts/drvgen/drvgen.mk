# Copyright (C) 2016 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See http://www.gnu.org/licenses/gpl-2.0.html for more details.

ifdef MTK_PLATFORM

ifeq ($(strip $(CONFIG_MTK_DTBO_FEATURE)), y)

ifeq ($(strip $(CONFIG_ARM64)), y)
DT_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES))
else
DT_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES))
endif

else#DTBO is not enabled

ifeq ($(strip $(CONFIG_ARM64)), y)
DT_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
else
DT_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM_APPENDED_DTB_IMAGE_NAMES))
endif

endif #CONFIG_MTK_DTBO_FEATURE

DTB_FILES := $(addsuffix .dtb,$(addprefix $(objtree)/arch/$(SRCARCH)/boot/dts/, $(DT_NAMES)))
DTS_FILES := $(addsuffix .dts,$(addprefix $(srctree)/arch/$(SRCARCH)/boot/dts/, $(DT_NAMES)))

export DTB_FILES
export DTS_FILES


PRIVATE_CUSTOM_KERNEL_DCT := $(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)
ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws),)
DRVGEN_PATH := drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(MTK_PROJECT)/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)
DWS_FILE := $(srctree)/$(DRVGEN_PATH)/codegen.dws

DWS_PREFIX := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/
DWS_SUFFIX := /dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws
else
DRVGEN_PATH := drivers/misc/mediatek/dws/$(MTK_PLATFORM)
DWS_FILE := $(srctree)/$(DRVGEN_PATH)/$(MTK_PROJECT).dws

DWS_PREFIX := $(srctree)/drivers/misc/mediatek/dws/$(MTK_PLATFORM)/
DWS_SUFFIX := .dws
endif

ifndef DRVGEN_OUT
#DRVGEN_OUT := $(objtree)/$(DRVGEN_PATH)
DRVGEN_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts
endif
export DRVGEN_OUT

ALL_DRVGEN_FILE := $(MTK_PROJECT)/cust.dtsi

ifneq ($(wildcard $(DWS_FILE)),)
DRVGEN_FILE_LIST := $(addprefix $(DRVGEN_OUT)/,$(ALL_DRVGEN_FILE))
DRVGEN_FILE_LIST += $(DTB_FILES)
else
DRVGEN_FILE_LIST :=
endif
DRVGEN_TOOL := $(srctree)/tools/dct/DrvGen.py

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST)
$(DRVGEN_FILE_LIST): $(DRVGEN_TOOL) $(DWS_FILE) $(DTS_FILES)
	for i in $(DTS_FILES); do \
		base_prj=`grep -m 1 "#include [<\"].*\/cust\.dtsi[>\"]" $$i | sed 's/#include [<"]//g' | sed 's/\/cust\.dtsi[>"]//g'`;\
		prj_path=$(DRVGEN_OUT)/$$base_prj ;\
		dws_path=$(DWS_PREFIX)$$base_prj$(DWS_SUFFIX) ;\
		if [ -f $$dws_path ] ; then \
			mkdir -p $$prj_path ;\
			$(python) $(DRVGEN_TOOL) $$dws_path $$prj_path $$prj_path cust_dtsi;\
		fi \
	done
ifeq ($(strip $(CONFIG_MTK_DTBO_FEATURE)), y)

DTB_OVERLAY_IMAGE_TAGERT := $(DRVGEN_OUT)/odmdtbo.img
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_DTB_OVERLAY_OBJ:=$(DTB_FILES)
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_MULTIPLE_DTB_OVERLAY_OBJ:=$(DRVGEN_OUT)/$(MTK_PROJECT).mdtb
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_MULTIPLE_DTB_OVERLAY_IMG:=$(DRVGEN_OUT)/$(MTK_PROJECT).mimg
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_MULTIPLE_DTB_OVERLAY_HDR:=$(srctree)/scripts/multiple_dtbo.py
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_MKIMAGE_TOOL:=$(srctree)/scripts/mkimage
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_MKIMAGE_CFG:=$(srctree)/scripts/odmdtbo.cfg
$(DTB_OVERLAY_IMAGE_TAGERT) : $(PRIVATE_MULTIPLE_DTB_OVERLAY_OBJ) dtbs $(PRIVATE_MKIMAGE_TOOL) $(PRIVATE_MKIMAGE_CFG) $(PRIVATE_MULTIPLE_DTB_OVERLAY_HDR)
	@echo Singing the generated overlay dtbo.
	cat $(PRIVATE_DTB_OVERLAY_OBJ) > $(PRIVATE_MULTIPLE_DTB_OVERLAY_OBJ) || (rm -f $(PRIVATE_MULTIPLE_DTB_OVERLAY_OBJ); false)
	python $(PRIVATE_MULTIPLE_DTB_OVERLAY_HDR) $(PRIVATE_MULTIPLE_DTB_OVERLAY_OBJ) $(PRIVATE_MULTIPLE_DTB_OVERLAY_IMG)
	$(PRIVATE_MKIMAGE_TOOL) $(PRIVATE_MULTIPLE_DTB_OVERLAY_IMG) $(PRIVATE_MKIMAGE_CFG)  > $@

.PHONY: odmdtboimage dtbs
odmdtboimage : $(DTB_OVERLAY_IMAGE_TAGERT)
endif

endif#MTK_PLATFORM
