ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ccflags-$(CONFIG_MACH_MT6799) += -DMTK_PASR_VCORE_DVFS_CONTROL
ccflags-$(CONFIG_MACH_MT6763) += -DMTK_PASR_VCORE_DVFS_CONTROL
ifeq ($(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES), "mediatek/k59v1_64_lwctg_lphqa")
ccflags-y += -DMTK_PASR_FIX_DRAM_CONFIGURATION
endif
obj-y += mtkpasr_drv.o mtkpasr_hw.o
