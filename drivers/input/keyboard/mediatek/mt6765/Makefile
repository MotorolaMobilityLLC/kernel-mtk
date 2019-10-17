subdir-ccflags-y += -Werror -I$(srctree)/drivers/input/keyboard/mediatek/
ifeq ($(BUILD_DUAL_85),true)
  subdir-ccflags-y += -DDUAL_85_VERSION
endif
obj-$(CONFIG_KEYBOARD_MTK) := hal_kpd.o

#dengxiaopeng add dual 85 macro
subdir-ccflags-$(CONFIG_ONTIM_DUAL_85) += -DDUAL_85_VERSION
