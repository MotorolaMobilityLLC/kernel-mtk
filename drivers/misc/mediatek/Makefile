#$(call all-subdir-src-or-makefile)
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat

obj-$(CONFIG_MTK_BASE_POWER) += base/

obj-$(CONFIG_MEDIATEK_SOLUTION) += mach/
obj-$(CONFIG_MTK_SERIAL)	+= uart/
obj-$(CONFIG_MTK_IRQ) += irq/
obj-$(CONFIG_MTK_SYS_CIRQ) += cirq/

obj-$(CONFIG_MTK_MMPROFILE_SUPPORT) += mmp/
obj-$(CONFIG_MTK_AUXADC)	+= auxadc/
obj-$(CONFIG_MTK_M4U)	+= m4u/
obj-$(CONFIG_MTK_GPT_SCHEME_SUPPORT)	+= partition/
obj-$(CONFIG_MTK_SYSENV)	+= sysenv/
obj-$(CONFIG_MTPROF)	+= mtprof/
obj-$(CONFIG_MTK_I2C)  += i2c/
obj-$(CONFIG_MTK_LEDS)	+= leds/
obj-$(CONFIG_MTK_VIBRATOR)	+= vibrator/
obj-$(CONFIG_MTK_ACCDET)	+= accdet/
obj-$(CONFIG_MTK_IRTX_SUPPORT) += irtx/
obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += videocodec/
obj-$(CONFIG_MTK_JPEG)	+= jpeg/
obj-$(CONFIG_MTK_PMIC)	+= power/
obj-$(CONFIG_MTK_PMIC_WRAP) += pmic_wrap/
obj-$(CONFIG_USB_MTK_HDRC) += usb20/
obj-$(CONFIG_MTK_GPIO) += gpio/
obj-$(CONFIG_SECURITY_SELINUX)	+= selinux_warning/
obj-$(CONFIG_MTK_MEM) += mem/
obj-$(CONFIG_MTK_CMDQ) += cmdq/
obj-$(CONFIG_MTK_BOOT) += boot/
obj-$(CONFIG_MTK_BOOT_REASON) += boot_reason/
obj-$(CONFIG_MTK_CHIP) += chip/
obj-$(CONFIG_MTK_DEVINFO_DRIVER) += devinfo/
obj-$(CONFIG_MTK_GPU_SUPPORT) += gpu/
obj-$(CONFIG_MTK_LCM) += lcm/
obj-$(CONFIG_MTK_FB) += video/
obj-$(CONFIG_MTK_DRAMC) += dramc/
obj-$(CONFIG_MTK_SYNC) += sync/
obj-$(CONFIG_MTK_FREQ_HOPPING) += freqhopping/
obj-$(CONFIG_MTK_WFD_SUPPORT)  += uibc/
obj-$(CONFIG_MTK_SECURITY_SW_SUPPORT) +=masp/

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/met/),)
obj-$(CONFIG_MTK_MET) += met/
endif
obj-$(CONFIG_MTK_CCCI_DEVICES)	+=  ccci_util/
obj-$(CONFIG_MTK_NET_CCMNI) +=  ccmni/
obj-$(CONFIG_MTK_CCCI_EXT)     +=  dual_ccci/
obj-$(CONFIG_MTK_ECCCI_DRIVER) +=  eccci/

ifdef CONFIG_CUSTOM_KERNEL_SSW
ifneq ($(strip $(CONFIG_CUSTOM_KERNEL_SSW)),"")
obj-y	+= ssw/
endif
endif

obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += gcpu/
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += gud/
obj-$(CONFIG_TRUSTONIC_TEE_SUPPORT) += secmem/
obj-$(CONFIG_MTK_LASTPC) += lastpc/
obj-$(CONFIG_MTK_WATCHPOINT) += watchpoint/
obj-$(CONFIG_MTK_RAM_CONSOLE)	+= ram_console/
obj-$(CONFIG_MTK_EXTMEM) += exm/

# gator driver is platform-independent
ifneq ($(wildcard $(srctree)/$(src)/gator-driver/),)
obj-$(CONFIG_GATOR_DRIVER) += gator-driver/
endif # gator-driver folder exist

