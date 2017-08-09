# BTIF driver for AD DIE
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	ccflags-y += -I$(src)/inc
	ccflags-y += -I$(src)/plat_inc

	obj-y += btif.o
	btif-y	:= mtk_btif.o mtk_btif_exp.o btif_dma_plat.o btif_plat.o

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
