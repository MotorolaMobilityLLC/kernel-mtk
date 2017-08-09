obj-y += scp.o
scp-y := scp_helper.o scp_excep.o scp_ipi.o scp_irq.o scp_logger.o scp_dvfs.o

# include emi_mpu.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include/mach

ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_scp_logger.o += -DDEBUG
CFLAGS_scp_helper.o += -DDEBUG
CFLAGS_scp_excep.o += -DDEBUG
CFLAGS_scp_ipi.o += -DDEBUG
CFLAGS_scp_irq.o += -DDEBUG
endif
