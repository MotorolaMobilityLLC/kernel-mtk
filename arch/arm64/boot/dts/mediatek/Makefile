dtb-$(CONFIG_ARCH_MEDIATEK) += mt6795-evb.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt8173-evb.dtb

ifeq ($(strip $(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE)), y)
dtb-y += $(addsuffix .dtb, $(notdir $(subst $\",,$(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES))))
endif

dtb-y += $(addsuffix .dtb, $(notdir $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))))

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)

clean-files	:= *.dtb
