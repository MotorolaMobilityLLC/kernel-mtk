MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat
subdir-ccflags-y += -Werror -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)

ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y += -D_GIT318_READY
ccflags-y += -D_GIT318_PMIC_READY
ccflags-y += -D_WCN_SUPPORT
ccflags-y += -DAUDIO_USING_WRAP_DRIVER
ccflags-y += -D_MT_IDLE_HEADER


obj-$(CONFIG_MFD_ARIZONA)  +=mach-mtk-florida.o
