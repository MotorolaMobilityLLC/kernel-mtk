MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -Idrivers/misc/mediatek/aee/aed/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
LINUXINCLUDE += -include $(srctree)/kernel/sched/sched.h

obj-y := mtprof.o bootprof.o
obj-y += sched_monitor.o monitor_debug_out.o
# obj-$(CONFIG_MT_LOCK_DEBUG) += lockprof.o
obj-$(CONFIG_MTK_WQ_DEBUG) += mt_wq_debug.o
mtprof-y += prof_ctl.o prof_main.o common.o
# obj-y += mt_prv_lock.o
obj-$(CONFIG_MT_PRINTK_UART_CONSOLE) += mt_printk_ctrl.o
obj-$(CONFIG_MT_RT_THROTTLE_MON) += rt_monitor.o
