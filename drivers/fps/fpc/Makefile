#
# Makefile for all kinds of Fingerprint
#
obj-y += fpc.o

ccflags-$(CONFIG_MICROTRUST_TEE_SUPPORT) += -DCONFIG_MICROTRUST_TEE_SUPPORT
ccflags-$(CONFIG_MICROTRUST_TEE_SUPPORT) += -I$(srctree)/drivers/misc/mediatek/teei/300/tz_driver/include
ccflags-$(CONFIG_MICROTRUST_TEE_SUPPORT) += -I$(srctree)/drivers/misc/mediatek/teei/300/common/include
ccflags-$(CONFIG_MEDIATEK_SOLUTION) += -DMTK