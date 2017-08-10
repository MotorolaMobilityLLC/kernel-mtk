subdir-ccflags-y += -Werror -I$(srctree)/drivers/input/keyboard/mediatek/

ifeq ($(CONFIG_MT_SND_SOC_6797),y)
	EXTRA_CFLAGS += -I$(srctree)/sound/soc/mediatek/mt6797
endif

obj-$(CONFIG_KEYBOARD_MTK) := hal_kpd.o
