obj-$(CONFIG_PM_DEVFREQ)	+= devfreq.o
obj-$(CONFIG_PM_DEVFREQ_EVENT)	+= devfreq-event.o
obj-$(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND)	+= governor_simpleondemand.o
obj-$(CONFIG_DEVFREQ_GOV_PERFORMANCE)	+= governor_performance.o
obj-$(CONFIG_DEVFREQ_GOV_POWERSAVE)	+= governor_powersave.o
obj-$(CONFIG_DEVFREQ_GOV_USERSPACE)	+= governor_userspace.o

# DEVFREQ Drivers
obj-$(CONFIG_ARM_EXYNOS4_BUS_DEVFREQ)	+= exynos/
obj-$(CONFIG_ARM_EXYNOS5_BUS_DEVFREQ)	+= exynos/
obj-$(CONFIG_ARM_TEGRA_DEVFREQ)		+= tegra-devfreq.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/watchdog/mediatek/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/$(MTK_PLATFORM)/

ifeq ($(CONFIG_MTK_QOS_SUPPORT), y)
#obj-$(CONFIG_MACH_MT6775)		+= helio-dvfsrc.o helio-dvfsrc-sysfs.o
obj-$(CONFIG_MACH_MT6771)		+= helio-dvfsrc.o helio-dvfsrc-sysfs.o

#obj-$(CONFIG_MACH_MT6775)		+= helio-dvfsrc-mt6775.o
obj-$(CONFIG_MACH_MT6771)		+= helio-dvfsrc-mt6771.o
endif

obj-$(CONFIG_MACH_MT6775)		+= helio-dvfsrc-opp-mt6775.o
obj-$(CONFIG_MACH_MT6771)		+= helio-dvfsrc-opp-mt6771.o

# DEVFREQ Event Drivers
obj-$(CONFIG_PM_DEVFREQ_EVENT)		+= event/
