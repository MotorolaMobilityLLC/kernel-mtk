subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/accdet/$(MTK_PLATFORM)
ifeq ($(CONFIG_MTK_PMIC_CHIP_MT6337),y)
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include/
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6337/
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6335/
else
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ifeq ($(CONFIG_MTK_PMIC_CHIP_MT6355),y)
subdir-ccflags-y += -Werror -I$(srctree)/sound/soc/mediatek/codec/mt6355/
endif
endif


obj-$(CONFIG_MTK_ACCDET) := accdet_drv.o
obj-$(CONFIG_MTK_ACCDET) += $(subst ",,$(CONFIG_MTK_PLATFORM))/

