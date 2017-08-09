
ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/include/trustzone/utils/tlutils
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/trustzone/utils/tlutils
include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
ccflags-y += -I$(src)/$(MTK_PLATFORM)
endif

obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += secmem.o
