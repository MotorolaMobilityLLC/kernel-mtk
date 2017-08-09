ifeq ($(CONFIG_MTK_COMBO_WIFI),y)
    subdir-ccflags-y += -D MTK_WCN_BUILT_IN_DRIVER
endif

ifneq ($(filter "MT6630",$(CONFIG_MTK_COMBO_CHIP)),)
$(warning include gen3)
	obj-y   +=  gen3/
endif
ifneq ($(filter "CONSYS_6797",$(CONFIG_MTK_COMBO_CHIP)),)
$(warning include gen3)
	obj-y   +=  gen3/
else ifneq ($(filter "CONSYS_%",$(CONFIG_MTK_COMBO_CHIP)),)
$(warning include gen2)
	obj-y   +=  gen2/
endif
