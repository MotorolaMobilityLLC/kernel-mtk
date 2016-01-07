#ccflags-y += -DCONFIG_MTK_FPGA
#ccflags-y += -DFIX_TO_26M

ccflags-y += -I$(srctree)/drivers/misc/mediatek/uart
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)

obj-$(CONFIG_MTK_SERIAL) := platform_uart.o
