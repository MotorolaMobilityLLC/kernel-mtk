ifeq ($(CONFIG_MTK_COMBO), y)

ccflags-y += \
	-I$(src)/../../linux/include \
	-I$(src)/../../linux/pri/include \
	-I$(src)/../../core/include \
	-I$(src)/../../include \
	-I$(src)/../include \
	-I$(src)/../../../common_detect \
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach

ccflags-y += -DWMT_CREATE_NODE_DYNAMIC=1

obj-y += osal.o \
         bgw_desense.o \
         wmt_idc.o
obj-$(CONFIG_MTK_COMBO_BT)		+= stp_chrdev_bt.o

endif
