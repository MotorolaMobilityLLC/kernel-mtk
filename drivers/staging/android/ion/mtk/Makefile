obj-$(CONFIG_MTK_ION) += ion_drv.o ion_mm_heap.o ion_fb_heap.o
ccflags-$(CONFIG_MTK_ION)  += -I$(srctree)/drivers/staging/android/ion #fro kernel-3.10 ion
ccflags-$(CONFIG_MTK_M4U) += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat

ifeq ($(CONFIG_MT_ENG_BUILD), y)
ccflags-y += -DION_RUNTIME_DEBUGGER=1
#ccflags-y += -DION_HISTORY_RECORD
#obj-$(CONFIG_ION_MTK) += ion_history.o
else
ccflags-y += -DION_RUNTIME_DEBUGGER=0
endif

ifeq ($(CONFIG_ARCH_MT6735),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735/
endif
ifeq ($(CONFIG_ARCH_MT6735M),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6735m/
endif
ifeq ($(CONFIG_ARCH_MT6753),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/mt6753/
endif
