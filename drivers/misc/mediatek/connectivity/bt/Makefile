#########################################################
# Bluetooth character device driver
#########################################################

############ 7668 BT sdio Driver ########################
ifneq ($(filter "MT7668",$(CONFIG_MTK_COMBO_CHIP)),)
export CONFIG_MTK_COMBO_BT=m
obj-$(CONFIG_MTK_COMBO_BT) += btmtksdio.o
btmtksdio-objs     := ./mt7668/btmtk_sdio.o ./mt7668/btmtk_main.o
else
ccflags-y += -D CREATE_NODE_DYNAMIC=1

ccflags-y += \
	-I$(src)/../common/common_main/include \
	-I$(src)/../common/common_main/linux/include

obj-$(CONFIG_MTK_COMBO_BT) += stp_chrdev_bt.o
endif
