obj-y += scp.o
scp-y := scp_helper.o scp_excep.o scp_ipi.o scp_irq.o scp_logger.o scp_dvfs.o scp_feature_table.o
#scp-y += scp_timesync.o scp_trace.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/scp/$(CONFIG_MTK_PLATFORM)
# include emi_mpu.h
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include/mach
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/spm_v2/
subdir-ccflags-$(CONFIG_MTK_PMIC_CHIP_MT6335)+= -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6335
subdir-ccflags-$(CONFIG_MTK_PMIC_CHIP_MT6353)+= -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6353
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/include/$(CONFIG_MTK_PLATFORM)/

#ifeq ($(CONFIG_MTK_ENG_BUILD),y)
CFLAGS_scp_logger.o += -DDEBUG
CFLAGS_scp_helper.o += -DDEBUG
CFLAGS_scp_excep.o += -DDEBUG
CFLAGS_scp_ipi.o += -DDEBUG
CFLAGS_scp_irq.o += -DDEBUG
CFLAGS_scp_dvfs.o += -DDEBUG
#endif
