
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/$(MTK_PLATFORM)/

obj-y += smi_debug.o
obj-y += smi_common.o


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