#include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic_wrap/$(CONFIG_MTK_PLATFORM)

# Mediatek PMIC MT63XX specific files
obj-$(CONFIG_MTK_PMIC_NEW_ARCH)          += mt6337.o mt6337_irq.o mt6337_common.o mt6337_api.o mt6337_auxadc.o
