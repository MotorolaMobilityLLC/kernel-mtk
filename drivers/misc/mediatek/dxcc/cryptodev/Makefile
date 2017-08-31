#
# Since version 1.6 the asynchronous mode has been
# disabled by default. To re-enable it uncomment the
# corresponding CFLAG.
#
PWD := $(shell pwd)

#include $(HOST_PROJ_ROOT)/proj.cfg

CRYPTODEV_CFLAGS ?= #-DENABLE_ASYNC

MODULE_NAME = cryptodev

#KBUILD_CFLAGS += -I$(src) -I$(HOST_PROJ_ROOT)/include -I$(HOST_PROJ_ROOT)/src/integration_tests/platform_dependent/include/kernel -I$(HOST_SRCDIR)/cc_driver/$(CC_TYPE)/$(OS) $(CRYPTODEV_CFLAGS) 

#EXTRA_CFLAGS += -I$(src)/cc_driver/cc710ree
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dxcc/cc_driver/cc710ree
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dxcc/cryptodev/platform_dependent/include/kernel

cryptodev-objs = ioctl.o main.o cryptlib.o authenc.o zc.o util.o platform_dependent/ZYNQ/kernel/secure_address_translation.o

obj-y += cryptodev.o




