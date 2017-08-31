subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/accdet/$(MTK_PLATFORM)
ifeq ($(CONFIG_MTK_PMIC_CHIP_MT6337),y)
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include/
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6337/
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6335/
else
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
endif


obj-$(CONFIG_MTK_ACCDET) := accdet_drv.o
obj-$(CONFIG_MTK_ACCDET) += $(subst ",,$(CONFIG_MTK_PLATFORM))/

