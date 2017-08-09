ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/

ifneq ($(CONFIG_MTK_SMI_VARIANT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/
obj-y += smi_common.o
else
obj-y += smi_variant.o
endif

obj-y += smi_debug.o

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -DD1
obj-y += mmdvfs_mgr.o
endif

ifeq ($(CONFIG_ARCH_MT6735M),y)
#ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -DD2
obj-y += mmdvfs_mgr.o
endif

#ifeq ($(CONFIG_ARCH_MT6735M),y)
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -DD3
obj-y += mmdvfs_mgr.o
endif

ifeq ($(CONFIG_ARCH_MT6580),y)
ccflags-y += -DR
endif

ifeq ($(CONFIG_ARCH_MT8173),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/
obj-y += smi_variant_config_8173.o
ccflags-y += -DMT73
endif
