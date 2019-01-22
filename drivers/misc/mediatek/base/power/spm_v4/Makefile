ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v4/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/cpuidle_v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/eem_v1/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/irq/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic_wrap/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6335/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6337/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/hps_v3/

ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/include/

ccflags-y += -I$(srctree)/drivers/scsi/ufs/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/

ifeq ($(CONFIG_MTK_SND_SOC_NEW_ARCH), y)
ccflags-y += -I$(srctree)/sound/soc/mediatek/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/sound/soc/mediatek/common/
endif

ifeq ($(CONFIG_MTK_TINYSYS_SSPM_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/$(MTK_PLATFORM)/
endif

ifeq ($(CONFIG_MACH_MT6763), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/eem_v1/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/

#ccflags-y += -DSPM_SODI_PROFILE_TIME -DSPM_SODI3_PROFILE_TIME

obj-y += mtk_idle.o
obj-y += mtk_idle_mt6763.o
obj-y += mtk_idle_profile.o
obj-y += mtk_sleep.o
obj-y += mtk_spm_init.o
obj-y += mtk_spm.o
obj-y += mtk_spm_dpidle.o
obj-y += mtk_spm_dpidle_mt6763.o
obj-y += mtk_spm_fs.o
obj-y += mtk_spm_internal.o
ifneq ($(CONFIG_MTK_TINYSYS_SSPM_SUPPORT), y)
obj-y += mtk_spm_pmic_wrap.o
endif
obj-y += mtk_spm_resource_req.o
obj-y += mtk_spm_sleep.o
obj-y += mtk_spm_sodi.o
obj-y += mtk_spm_sodi3p0.o
obj-y += mtk_spm_sodi_cmdq.o
obj-y += mtk_spm_vcorefs.o
endif
