#ccflags-y += -Wall
	
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/msg2xxx/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/

obj-$(CONFIG_TOUCHSCREEN_MTK_MSTAR_TS) += ilitek_drv_main.o \
	ilitek_drv_mp_test.o \
	ilitek_drv_mtk.o \
	ilitek_drv_update.o \
	new_mp_test/mp_misc.o \
	new_mp_test/mp_result.o \
	new_mp_test/mp_parse.o \
	new_mp_test/mp_test.o \
	new_mp_test/mp_main.o \


