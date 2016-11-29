##tee_xuzhifeng@wind-mobi.com 20161123 begin
ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include

ifeq ($(CONFIG_TRUSTONIC_TEE_SUPPORT),y)
subdir-ccflags-y += -I$(srctree)/drivers/spi/trustzone/spi/Tlspi/inc
	include $(srctree)/drivers/misc/mediatek/gud/Makefile.include
endif


obj-$(CONFIG_FINGERPRINT_EGISTEC_ET360)	+= et360-int.o navi_input.o
#obj-y	+= et360-int.o navi_input.o
##tee_xuzhifeng@wind-mobi.com 20161123 end

