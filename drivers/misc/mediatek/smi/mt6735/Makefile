
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/$(MTK_PLATFORM)

obj-y += smi_debug.o
obj-y += mmdvfs_mgr.o

ifeq ($(CONFIG_ARCH_MT6735),y)
obj-y += smi_common_d1.o
ccflags-y += -I$(srctree)/drivers/clk/mediatek
ccflags-y += -DD1
endif

ifeq ($(CONFIG_ARCH_MT6735M),y)
obj-y += smi_common_d2.o
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6735m/
ccflags-y += -DD2
endif

ifeq ($(CONFIG_ARCH_MT6753),y)
obj-y += smi_common_d3.o
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM)/mt6753/
ccflags-y += -DD3
endif