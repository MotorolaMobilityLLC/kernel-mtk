#$(call all-subdir-src-or-makefile)
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include

obj-$(CONFIG_MEDIATEK_SOLUTION) += mach/
obj-$(CONFIG_MTK_SERIAL)	+= uart/
obj-$(CONFIG_MTK_IRQ) += irq/

