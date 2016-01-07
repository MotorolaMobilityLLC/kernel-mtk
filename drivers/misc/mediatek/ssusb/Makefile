#ccflags-y := -I$(src)/include
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
subdir-ccflags-y += -I$(srctree)/drivers/power/mt81xx
obj-$(CONFIG_SSUSB_DRV)  +=  mu3d/
obj-$(CONFIG_SSUSB_MU3_PHY)       +=  mu3phy/
