PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ifeq ($(CONFIG_SINGLE_PANEL_OUTPUT),y)
    ifeq ($(CONFIG_ARCH_MT6735M), y)
    obj-y += common/
    obj-y += mt6735m/
    endif
else ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/ext_disp/$(PLATFORM)/),)
obj-y += $(PLATFORM)/
obj-y += common/
else
obj-y += dummy/
endif
