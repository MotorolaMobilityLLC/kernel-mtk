ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/

obj-y += smi_variant.o
obj-y += smi_debug.o

ifeq ($(CONFIG_ARCH_MT8173),y)
obj-y += smi_variant_config_8173.o
ccflags-y += -DMT73
endif

ifeq ($(CONFIG_ARCH_MT8127),y)
ccflags-y += -DMT27
obj-y += smi_variant_config_8127.o
endif
