# Connectivity combo driver
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
subdir-ccflags-y += -D MTK_WCN_REMOVE_KERNEL_MODULE
ifeq ($(CONFIG_ARM64), y)
subdir-ccflags-y += -D CONFIG_MTK_WCN_ARM64
endif

ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
    subdir-ccflags-y += -D WMT_IDC_SUPPORT=1
else
    subdir-ccflags-y += -D WMT_IDC_SUPPORT=0
endif
    subdir-ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT

    obj-y += common/
    obj-$(CONFIG_MTK_COMBO_WIFI) += wlan/
    obj-$(CONFIG_MTK_COMBO_GPS) += gps/
    obj-n := dummy.o

# Otherwise we were called directly from the command line;
# invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
