ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic_wrap/$(CONFIG_MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/power/mediatek/battery

ifeq ($(CONFIG_MACH_MT6757), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)
endif
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/

ifeq ($(CONFIG_MTK_TINYSYS_SSPM_SUPPORT), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/$(MTK_PLATFORM)/
obj-y += pmic_ipi.o
endif

ifeq ($(CONFIG_MTK_GAUGE_VERSION),30)
obj-$(CONFIG_MTK_SMART_BATTERY) += mt6355_gauge.o
endif

obj-$(CONFIG_MTK_PMIC_CHIP_MT6355)	+= pmic.o pmic_irq.o pmic_common.o \
		pmic_api.o pmic_lp_api.o \
		regulator_codegen.o pmic_auxadc.o \
		pmic_initial_setting.o \
		pmic_throttling_dlpt.o pmic_debugfs.o pmic_bif.o

# Charger Type Detection
ifndef CONFIG_MTK_EXTERNAL_CHARGER_TYPE_DETECT
	ifeq ($(CONFIG_MTK_GAUGE_VERSION),30)
		obj-$(CONFIG_MTK_PMIC_CHIP_MT6355) += pmic_chr_type_det_v2.o
	endif
endif
