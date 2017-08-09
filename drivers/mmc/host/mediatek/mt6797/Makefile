#add kernel source code path as head file search path
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
#workaround for include mmc/card/queue.h
ccflags-y += -I$(srctree)/drivers/mmc/card
ccflags-y += -I$(srctree)/drivers/mmc/core

obj-$(CONFIG_MMC_MTK) += sd.o
obj-$(CONFIG_MMC_MTK) += dbg.o
obj-$(CONFIG_MMC_MTK) += mt_sd_misc.o
obj-$(CONFIG_MMC_MTK) += msdc_tune.o
obj-$(CONFIG_MMC_MTK) += msdc_partition.o
obj-$(CONFIG_MMC_MTK) += msdc_io.o

ifeq ($(CONFIG_MTK_FPGA),y)
obj-$(CONFIG_MMC_MTK) += msdc_fpga_io.o
endif

#obj-$(CONFIG_MMC_MTK) += sdio_autok.o sdio_autok_proc.o ettagent.o

# kdump/kpainc interface move to lk
ifeq ($(CONFIG_MTK_AEE_IPANIC),y)
obj-$(CONFIG_MMC_MTK) += mt_dump.o
endif

ifeq ($(CONFIG_MTK_EMMC_SUPPORT_OTP),y)
obj-$(CONFIG_MMC_MTK) += emmc_otp.o
endif

#ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
#ccflags-y += -DFEATURE_MET_MMC_INDEX
#endif
