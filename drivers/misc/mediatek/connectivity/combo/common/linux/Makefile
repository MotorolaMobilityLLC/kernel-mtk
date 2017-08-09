ccflags-y += -I$(srctree)/drivers/misc/mediatek/connectivity/combo/common/linux/include \
	-I$(srctree)/drivers/misc/mediatek/connectivity/combo/common/core/include \
	-I$(srctree)/drivers/misc/mediatek/connectivity/combo/common/include \
	-I$(srctree)/drivers/misc/mediatek/connectivity/combo/common \
	-I$(srctree)/drivers/misc/mediatek/connectivity/common_detect



ifeq ($(CONFIG_MTK_COMBO),y)
obj-y	+= wmt_dev.o
obj-y	+= wmt_idc.o
obj-y	+= osal.o
obj-y	+= stp_dbg.o

obj-y	+= stp_uart.o
obj-y	+= hif_sdio.o

obj-y	+= stp_sdio.o

obj-$(CONFIG_MTK_COMBO_GPS)	+= stp_chrdev_gps.o
obj-$(CONFIG_MTK_COMBO_BT)	+= stp_chrdev_bt.o

obj-$(CONFIG_MTK_COMBO_WIFI)	+= wmt_chrdev_wifi.o

ifneq ($(filter "MT6630",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y 	+= stp_chrdev_ant.o
endif

endif
