ccflags-y := -I$(srctree)/drivers/misc/mediatek/include
ccflags-$(CONFIG_MTK_PRINTK_UART_CONSOLE) += -I$(srctree)/drivers/misc/mediatek/mtprintk/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/uart
ccflags-$(CONFIG_MTK_DISABLE_UART_LOG) += -DDISABLE_UART_LOG

obj-y	= printk.o
obj-$(CONFIG_PRINTK_NMI)		+= nmi.o
obj-$(CONFIG_A11Y_BRAILLE_CONSOLE)	+= braille.o

ifeq ($(BUILD_SMT), YES)
ccflags-y +=-DDISABLE_UART_LOG
endif
