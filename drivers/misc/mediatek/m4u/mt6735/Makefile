ccflags-y += -I$(srctree)/drivers/staging/android/ion
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/trustzone/m4u
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/irq/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/2.0

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735/
obj-y += mt6735/
endif
ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735m/
obj-y += mt6735m/
endif
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6753/
obj-y += mt6753/
endif

obj-$(CONFIG_MTK_M4U) += m4u_hw.o
