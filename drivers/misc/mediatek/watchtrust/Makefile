
#########################################################################
# Set Internal Variables                                                #
# May be modified to match your setup                                   #
#########################################################################
CFG_TEE_DRV_DEBUGFS?=0
CFG_TEE_CORE_LOG_LEVEL?=2
CFG_TEE_TA_LOG_LEVEL?=2

ccflags-y+=-Werror
ccflags-y+=-I$(src)/include/arm_common
ccflags-y+=-I$(src)/include/wd_tee
ccflags-y+=-I$(src)/include
ccflags-y+=-I$(src)/core
ccflags-y+=-I$(src)/tui

ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(MTK_PLATFORM)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include

ccflags-y+=-DCFG_TEE_DRV_DEBUGFS=${CFG_TEE_DRV_DEBUGFS}
ccflags-y+=-DCFG_TEE_CORE_LOG_LEVEL=${CFG_TEE_CORE_LOG_LEVEL}
ccflags-y+=-DCFG_TEE_TA_LOG_LEVEL=${CFG_TEE_TA_LOG_LEVEL}
ccflags-y+=-DSMP_ENABLE
#ccflags-y+=-DDEBUG=1
#ccflags-y+=-D_TEE_DEBUG
#ccflags-y+=-DCONFIG_TEE_PM_WAKE

obj-$(CONFIG_WATCHDATA_TEE_SUPPORT) += wdtee_armtz.o

wdtee_armtz-objs:=   \
		armtz/tee_tz_drv.o \
		armtz/tee_mem.o \
		armtz/handle.o \
		armtz/log.o \
		core/tee_core.o  \
		core/tee_context.o  \
		core/tee_session.o  \
		core/tee_shm.o  \
		core/tee_supp_com.o  \
		core/tee_sysfs.o \
		core/tee_debugfs.o \
		core/tee_kernel_api.o \
		core/tee_mutex_wait.o \
		core/tee_wait_queue.o

ifeq ($(CONFIG_ARM),y)
# "smc" assembly intruction requires dedicated "armv7 secure extension"
secext := $(call as-instr,.arch_extension sec,+sec)
AFLAGS_tee_smc-arm.o := -Wa,-march=armv7-a$(secext)
wdtee_armtz-objs += \
	armtz/tee_smc-arm.o
endif

ifeq ($(CONFIG_ARM64),y)
wdtee_armtz-objs += \
	armtz/tee_smc-arm64.o
endif

