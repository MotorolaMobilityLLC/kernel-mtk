MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/nand/v1/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/nand/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/mach/
obj-$(CONFIG_MTK_MTD_NAND) += mtk_nand.o
obj-$(CONFIG_MTK_MTD_NAND) += bmt.o
obj-$(CONFIG_MTK_MTD_NAND) += partition_mt.o
