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
DRVGEN_PATH := drivers/misc/mediatek/dws/$(MTK_PLATFORM)

ifeq ($(strip $(CONFIG_MTK_DTBO_FEATURE)), y)
DTBO_NAME := $(notdir $(subst $\",,$(if $(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE),$(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES),$(CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES))))
ifeq ($(strip $(CONFIG_ARM64)), y)
DTBO_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts/mediatek
else
DTBO_OUT := $(objtree)/arch/$(SRCARCH)/boot/dts/
endif
DTS_OVERLAY := $(DTBO_OUT)/$(DTBO_NAME).dts
export DTS_OVERLAY
export DTBO_NAME
export DTBO_OUT
endif

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
DRVGEN_FIG := $(wildcard $(dir $(DRVGEN_TOOL))config/*.fig)

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST)
$(DRVGEN_OUT)/cust.dtsi: $(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_FIG)
	@mkdir -p $(dir $@)
	$(python) $(DRVGEN_TOOL) $(DWS_FILE) $(dir $@) $(dir $@) cust_dtsi

ifeq ($(strip $(CONFIG_MTK_DTBO_FEATURE)), y)
DTB_OVERLAY_IMAGE_TAGERT := $(DTBO_OUT)/dtbo.img
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_DTB_OVERLAY_OBJ:=$(basename $(DTS_OVERLAY)).dtb
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_DTB_OVERLAY_HDR:=$(srctree)/scripts/dtbo.cfg
$(DTB_OVERLAY_IMAGE_TAGERT) : PRIVATE_MKIMAGE_TOOL:=$(srctree)/scripts/mkimage
$(DTB_OVERLAY_IMAGE_TAGERT) : $(PRIVATE_DTB_OVERLAY_OBJ) dtbs $(PRIVATE_DTB_OVERLAY_HDR) | $(PRIVATE_MKIMAGE_TOOL)
	@echo Singing the generated overlay dtbo.
	$(PRIVATE_MKIMAGE_TOOL) $(PRIVATE_DTB_OVERLAY_OBJ) $(PRIVATE_DTB_OVERLAY_HDR)  > $@
.PHONY: dtboimage
dtboimage : $(DTB_OVERLAY_IMAGE_TAGERT) dtbs
endif

endif#MTK_PLATFORM
