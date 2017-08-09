#
# Makefile for the fingerprint device.
#

# Each configuration option enables a list of files.
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include


obj-$(CONFIG_FINGERPRINT_GOODIX_GF_TEE)	+= gf_tee.o
gf_tee-$(CONFIG_FINGERPRINT_GOODIX_GF_TEE) := gf_spi_tee.o gf_spi_ree.o

#obj-y	+= gf_tee.o
#gf_tee-y := gf_spi_tee.o gf_spi_ree.o
