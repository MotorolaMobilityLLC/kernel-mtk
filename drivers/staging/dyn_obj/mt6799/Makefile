obj-$(CONFIG_MTK_DO) := do.o do_ipi.o do_impl.o
ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/$(CONFIG_MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
# for emi_mpu
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include/mach

ifeq ($(CONFIG_MTK_ENG_BUILD),y)
CFLAGS_do.o += -DDEBUG
CFLAGS_do_ipi.o += -DDEBUG
CFLAGS_do_impl.o += -DDEBUG
endif

