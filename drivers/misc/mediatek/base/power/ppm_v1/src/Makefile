ccflags-y += \
	-I$(PPM_ROOT_DIR)/inc \
	-I$(PPM_ROOT_DIR)/src/mach/$(CONFIG_MTK_PLATFORM) \
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(CONFIG_MTK_PLATFORM)/include	\
	-I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)/

ifeq ($(CONFIG_ARCH_MT6755),y)
# main
obj-y := mt_ppm_main.o
obj-y += mt_ppm_interface.o
obj-y += mt_ppm_api.o

# policies
obj-y += mt_ppm_policy_dlpt.o
obj-y += mt_ppm_policy_perfserv.o
obj-y += mt_ppm_policy_ptpod.o
obj-y += mt_ppm_policy_pwr_thro.o
obj-y += mt_ppm_policy_thermal.o
obj-y += mt_ppm_policy_lcm_off.o
obj-y += mt_ppm_policy_hica.o
obj-y += mt_ppm_policy_user_limit.o
# for test purpose
obj-y += mt_ppm_policy_ut.o

# platform dependent files
obj-y += mach/$(subst ",,$(CONFIG_MTK_PLATFORM))/

else
# dummy driver to avoid build error
obj-y := mt_ppm_dummy.o
endif

