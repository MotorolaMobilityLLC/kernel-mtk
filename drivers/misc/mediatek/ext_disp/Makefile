PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/ext_disp/$(PLATFORM)/),)
obj-y += $(PLATFORM)/
endif
