ccflags-y += -I$(src)			# needed for trace events
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include

obj-$(CONFIG_ANDROID_BINDER_IPC)	+= binder.o binder_alloc.o
obj-$(CONFIG_ANDROID_BINDER_IPC_SELFTEST) += binder_alloc_selftest.o
