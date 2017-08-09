
obj-y += mt_dcm.o
ifeq ($(CONFIG_MTK_LEGACY),y)
obj-y += mt_clkmgr_common_legacy.o
obj-y += mt_clkmgr_legacy.o
else
obj-y += mt_clkmgr_common.o
obj-y += mt_clkmgr.o
endif
obj-y += mt_idle.o
obj-y += mt_spm_mtcmos.o
obj-y += mt_pbm.o
obj-y += mt_vcore_dvfs_1.o
obj-y += mt_cpufreq.o
obj-y += mt_gpufreq.o

ifneq ($(CONFIG_ARM64), y)

obj-y += mt-smp.o
obj-y += hotplug.o
obj-y += mt-headsmp.o
obj-y += mt_hotplug.o

else

obj-$(CONFIG_MTK_PSCI)  += mt_psci.o

endif
