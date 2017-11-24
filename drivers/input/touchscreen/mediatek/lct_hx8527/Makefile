# Himax touch controller for MTK platform
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/lct_hx8527/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/lct_hx8527/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/

obj-y	+= himax_platform.o 
obj-y	+= himax_852xES.o
