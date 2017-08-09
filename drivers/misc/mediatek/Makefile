#$(call all-subdir-src-or-makefile)
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/include

obj-y += base/

obj-$(CONFIG_MEDIATEK_SOLUTION) += mach/
obj-$(CONFIG_MTK_SERIAL)	+= uart/
obj-$(CONFIG_MTK_IRQ) += irq/

obj-$(CONFIG_MTK_AUXADC)	+= auxadc/
obj-$(CONFIG_MTK_M4U)	+= m4u/
obj-$(CONFIG_MTPROF)	+= mtprof/
obj-$(CONFIG_MTK_I2C)  += i2c/
obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += videocodec/
obj-$(CONFIG_MTK_JPEG)	+= jpeg/
obj-$(CONFIG_MTK_PMIC_WRAP) += pmic_wrap/
obj-$(CONFIG_USB_MTK_HDRC) += usb20/
