obj-y += sspm.o
sspm-y := sspm_helper.o sspm_irq.o sspm_sysfs.o sspm_excep.o sspm_reservedmem.o sspm_plt.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/$(CONFIG_MTK_PLATFORM) -I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)
ccflags-$(CONFIG_MTK_ENG_BUILD) += -DDEBUG

#obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-y += $(CONFIG_MTK_PLATFORM)/
