
ifeq ($(CONFIG_MTK_MLC_NAND_SUPPORT), y)
ccflags-y += -DMTK_MLC_NAND_SUPPORT
endif

ifeq ($(CONFIG_MTK_COMBO_NAND_SUPPORT), y)
ccflags-y += -DMTK_COMBO_NAND_SUPPORT
endif

obj-$(CONFIG_MTK_MTD_NAND) += mtk_nand.o
obj-$(CONFIG_MTK_MTD_NAND) += bmt.o
obj-$(CONFIG_MTK_MTD_NAND) += partition_mt.o
