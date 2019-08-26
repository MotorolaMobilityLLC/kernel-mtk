subdir-ccflags-y += -Werror -I$(srctree)/drivers/input/keyboard/mediatek/
ifeq ($(BUILD_DUAL_85),true)
  subdir-ccflags-y += -DDUAL_85_VERSION
endif
obj-$(CONFIG_KEYBOARD_MTK) := hal_kpd.o
