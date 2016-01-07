subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/chip/

obj-$(CONFIG_MTK_CHIP) += CHIP_COMMON.o
CHIP_COMMON-$(CONFIG_MTK_CHIP) := mt_chip_common.o

obj-$(CONFIG_MTK_CHIP) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
