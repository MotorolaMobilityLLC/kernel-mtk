ifeq ($(CONFIG_MTK_COMBO_WIFI), y)
	subdir-ccflags-y += -D MTK_WCN_BUILT_IN_DRIVER
	hif=sdio
	export CONFIG_MTK_COMBO_WIFI_HIF=$(hif)
	export CONFIG_MTK_WIFI_ONLY=n
endif

#########################################################
# Wi-Fi character device driver

ccflags-y += -D CREATE_NODE_DYNAMIC=1

ifeq ($(CONFIG_MTK_TC1_FEATURE), y)
	ccflags-y += -D CFG_TC1_FEATURE=1
else
	ccflags-y += -D CFG_TC1_FEATURE=0
endif

ccflags-y += \
	-I$(src)/../common/common_main/include \
	-I$(src)/../common/common_main/linux/include

obj-$(CONFIG_MTK_COMBO_WIFI) += wmt_chrdev_wifi.o

#########################################################
# WLAN driver

ifneq ($(filter "MT6630",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y   +=  gen3/
endif

ifneq ($(filter "CONSYS_6797",$(CONFIG_MTK_COMBO_CHIP)),)
	subdir-ccflags-y += -D MT6797
	subdir-ccflags-y += -D CFG_SPM_WORKAROUND_FOR_HOTSPOT=0
	obj-y   +=  gen3/
else ifneq ($(filter "CONSYS_%",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y   +=  gen2/
endif

ifneq ($(filter "MT6632",$(CONFIG_MTK_COMBO_CHIP)),)
	obj-y   +=  gen4/
endif
