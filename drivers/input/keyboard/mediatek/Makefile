subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include/
subdir-ccflags-y += -I$(srctree)/drivers/input/keyboard/mediatek/$(CONFIG_MTK_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
obj-$(CONFIG_KEYBOARD_MTK) := kpd.o

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

