subdir-ccflags-y += -Werror -I$(srctree)/drivers/input/keyboard/mediatek/
ifeq ($(CONFIG_MTK_TC1_FM_AT_SUSPEND),y)
ifeq ($(CONFIG_MT_SND_SOC_V3),y)
	EXTRA_CFLAGS += -I$(srctree)/sound/soc/mediatek/mt6735
endif
endif
obj-$(CONFIG_KEYBOARD_MTK) := hal_kpd.o
