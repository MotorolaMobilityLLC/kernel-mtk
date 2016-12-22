
obj-y	:= mt_gpio_base.o
obj-y	+= mt_gpio_affix.o mt_gpio_debug.o mt_gpio_ext.o
ifeq ($(CONFIG_GPIOLIB),y)
obj-y	+= mt_gpio_base_linux.o
endif
