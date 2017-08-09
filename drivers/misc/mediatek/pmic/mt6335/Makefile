#include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic_wrap/$(CONFIG_MTK_PLATFORM)

# Mediatek PMIC MT63XX specific files
obj-$(CONFIG_MTK_PMIC_CHIP_MT6335)          += pmic.o pmic_irq.o upmu_common.o pmic_regulator.o pmic_api.o pmic_api_buck.o pmic_api_ldo.o regulator_codegen.o pmic_auxadc.o pmic_chr_type_det.o pmic_initial_setting.o pmic_efuse.o pmic_throttling_dlpt.o
