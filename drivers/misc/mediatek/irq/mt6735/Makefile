MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include

obj-y += irq.o

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_irq.o := -DDEBUG
endif
