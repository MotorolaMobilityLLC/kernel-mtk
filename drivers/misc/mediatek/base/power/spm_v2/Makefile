ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/cpuidle_v1/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/irq/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic_wrap/$(CONFIG_MTK_PLATFORM)

#ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/inc/
#endif


ifeq ($(CONFIG_ARCH_MT6755), y)
ccflags-y += -I$(srctree)/sound/soc/mediatek/mt_soc_audio_6755/

obj-y += mt_idle.o
obj-y += mt_idle_mt6755.o
obj-y += mt_sleep.o
obj-y += mt_spm.o
obj-y += mt_spm_dpidle.o
obj-y += mt_spm_dpidle_mt6755.o
obj-y += mt_spm_fs.o
obj-y += mt_spm_internal.o
#obj-y += mt_spm_mcdi.o
obj-y += mt_spm_pmic_wrap.o
obj-y += mt_spm_sleep.o
obj-y += mt_spm_sodi.o
obj-y += mt_spm_sodi3p0.o
obj-y += mt_spm_vcorefs_mt6755.o
endif

ifeq ($(CONFIG_ARCH_MT6797), y)

obj-y += mt_idle.o
obj-y += mt_idle_mt6797.o
obj-y += mt_spm.o
obj-y += mt_spm_sleep.o
obj-y += mt_sleep.o
obj-y += mt_spm_internal.o
obj-y += mt_spm_dpidle.o
obj-y += mt_spm_dpidle_mt6797.o
obj-y += mt_spm_fs.o
obj-y += mt_spm_sodi.o
obj-y += mt_spm_sodi3p0.o
obj-y += mt_spm_pmic_wrap.o
obj-y += mt_spm_vcorefs_mt6797.o

endif
