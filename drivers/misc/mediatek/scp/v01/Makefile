obj-y += scp.o
scp-y := scp_helper.o scp_excep.o scp_ipi.o scp_irq.o scp_logger.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/$(CONFIG_MTK_PLATFORM)
# include emi_mpu.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/

ifeq ($(CONFIG_MTK_ENG_BUILD),y)
CFLAGS_scp_logger.o += -DDEBUG
CFLAGS_scp_helper.o += -DDEBUG
CFLAGS_scp_excep.o += -DDEBUG
CFLAGS_scp_ipi.o += -DDEBUG
CFLAGS_scp_irq.o += -DDEBUG
endif
