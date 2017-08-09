# WMT HAL driver for MT6797
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
  ccflags-y +=	-I$(src)/include \
		-I$(srctree)/drivers/misc/mediatek/connectivity/conn_soc/common/linux/include \
		-I$(srctree)/drivers/misc/mediatek/connectivity/conn_soc/common/include \
		-I$(srctree)/drivers/misc/mediatek/connectivity/common_detect \
		-I$(srctree)/drivers/misc/mediatek/connectivity/conn_soc/common/linux/include

  subdir-ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
  ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
	subdir-ccflags-y += -DWMT_IDC_SUPPORT=1
	else
	subdir-ccflags-y += -DWMT_IDC_SUPPORT=0
	endif
  obj-y += mtk_wcn_consys_hw.o
  obj-y += wmt_plat_alps.o
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
