MTK_PLATFORM:= $(subst ",,$(CONFIG_MTK_PLATFORM))

ccflags-y += -I$(srctree)/drivers/misc/mediatek/btcvsd/inc/$(MTK_PLATFORM)
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
#subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/

#obj-$(CONFIG_MTK_BTCVSD) += AudDrv_BTCVSD.o
obj-y += AudDrv_BTCVSD.o
