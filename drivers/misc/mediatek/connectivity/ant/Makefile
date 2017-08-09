# Connectivity combo driver

    ccflags-y += -D WMT_CREATE_NODE_DYNAMIC=1

ifneq ($(filter "MT6630",$(CONFIG_MTK_COMBO_CHIP)),)
	ccflags-y += \
	-I$(src)/../common/common_detect \
	-I$(src)/../common/combo/include \
	-I$(src)/../common/combo/linux/include

	obj-$(CONFIG_MTK_COMBO_ANT)	+= stp_chrdev_ant.o
endif

ifneq ($(filter "CONSYS_6797",$(CONFIG_MTK_COMBO_CHIP)),)
	ccflags-y += \
	-I$(src)/../common/common_detect \
	-I$(src)/../common/conn_soc/include \
	-I$(src)/../common/conn_soc/linux/include

	obj-$(CONFIG_MTK_COMBO_ANT)	+= stp_chrdev_ant.o
endif

# EOF

