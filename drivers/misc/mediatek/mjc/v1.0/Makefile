MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/

obj-y += mjc_kernel_driver.o
ifdef CONFIG_COMPAT
obj-y += mjc_kernel_compat_driver.o
endif
