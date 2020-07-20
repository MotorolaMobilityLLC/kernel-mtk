# include header files for MTK
ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6797/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ipio_touch_driver/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

ccflags-y += -Wall
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+= ili9881x.o 
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+=	ili9881x_i2c.o 
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+=	ili9881x_mtk.o 
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+=	ili9881x_ic.o 
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+=	ili9881x_touch.o 
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+=	ili9881x_mp.o 
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+=	ili9881x_flash.o 
obj-$(CONFIG_TOUCHSCREEN_ILI988X)	+=	ili9881x_node.o


KERNEL_DIR= /home/likewise-open/ILI/1061279/workplace/rk3288_sdk/kernel
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

