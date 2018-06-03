
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6336
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include


# Mediatek PMIC MT63XX specific files
obj-$(CONFIG_MTK_PMIC_CHIP_MT6336)	+= mt6336.o mt6336_efuse.o mt6336_auxadc.o mt6336_controller.o
ifdef _MT6336_E1
	ccflags-y += -DMT6336_E1
	obj-$(CONFIG_MTK_PMIC_CHIP_MT6336)	+= mt6336_irq_e1.o mt6336_common_e1.o
else
	obj-$(CONFIG_MTK_PMIC_CHIP_MT6336)	+= mt6336_irq.o mt6336_common.o
endif

ifneq ($(CONFIG_MTK_GAUGE_VERSION),30)
obj-$(CONFIG_MTK_SMART_BATTERY) += charging_hw_mt6336.o
endif

ifeq ($(CONFIG_MTK_GAUGE_VERSION),30)
	obj-$(CONFIG_MTK_PMIC_CHIP_MT6336) += pmic_chr_type_det_v2.o
else
	obj-$(CONFIG_MTK_PMIC_CHIP_MT6336) += pmic_chr_type_det.o
endif
