ifeq ($(CONFIG_MACH_MT6799), y)
ifeq ($(CONFIG_REGULATOR_FIXED_VOLTAGE), y)
ccflags-y += -DMT_GPIO_INIT_AT_POSTCORE
endif
endif

ifneq ($(CONFIG_MTK_GPIOLIB_STAND),y)
obj-$(CONFIG_MTK_GPIO)	+= mtk_gpio_core.o
obj-$(CONFIG_MTK_GPIO)  += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif

ifeq ($(CONFIG_MTK_GPIOLIB_STAND),y)
obj-$(CONFIG_MTK_MT6306_GPIO_SUPPORT)  += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
