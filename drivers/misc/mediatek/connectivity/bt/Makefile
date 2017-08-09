ifeq ($(CONFIG_MTK_COMBO_BT_HCI),y)

ccflags-y += \
        -I$(src)/include \
        -I$(src)/../common/combo/linux/include \
        -I$(src)/../common/combo/core/include \
        -I$(src)/../common/combo/include \
        -I$(src)/../common/common_detect

ifeq ($(CONFIG_BT),y)
ccflags-y += -D MTK_BT_HCI  -D REMOVE_MK_NODE=1
endif

obj-$(CONFIG_MTK_COMBO_BT_HCI)	+= stp_chrdev_bt_hci.o

endif
