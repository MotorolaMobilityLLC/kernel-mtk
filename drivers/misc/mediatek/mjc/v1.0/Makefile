MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))


MTK_PLATFORM_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)
MTK_PROJECT_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)



ccflags-y += -I$(MTK_PLATFORM_PATH_ROOT)/include \
             -I$(MTK_PLATFORM_PATH_ROOT)/include/mach \
             -I$(MTK_PROJECT_PATH_ROOT)/core \
             -I$(MTK_PROJECT_PATH_ROOT)/common

ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/

obj-y += mjc_kernel_driver.o
ifdef CONFIG_COMPAT
obj-y += mjc_kernel_compat_driver.o
endif