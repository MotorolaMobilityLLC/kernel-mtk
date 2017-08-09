include $(srctree)/drivers/misc/mediatek/flashlight/src/Makefile.custom

ifeq ($(CONFIG_MTK_PLATFORM),"mt6735")
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
else
obj-y += ../dummy_flashlight/
endif



