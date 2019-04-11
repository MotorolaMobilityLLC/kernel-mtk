subdir-ccflags-y += -Werror -I$(srctree)/drivers/input/keyboard/mediatek/

obj-$(CONFIG_KEYBOARD_MTK) := hal_kpd.o

# Special requirement for WIKO MWC show
ifeq ($(FEATURE_GCF_PTCRB),true)
ccflags-y := -DFEATURE_MASK_KEY_RESET
endif
