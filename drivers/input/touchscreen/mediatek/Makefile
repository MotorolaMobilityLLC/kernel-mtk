subdir-ccflags-y += -Werror
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
# In case the platform does NOT support this type of sensors
obj-y   +=  mtk_tpd.o
obj-y   +=  tpd_button.o
obj-y   +=  tpd_calibrate.o
obj-y   +=  tpd_debug.o
obj-y   +=  tpd_default.o
obj-y   +=  tpd_init.o
obj-y   +=  tpd_misc.o
obj-y   +=  tpd_setting.o

obj-$(CONFIG_TOUCHSCREEN_MTK_GT1151)	+=  GT1151/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT9XXTB_HOTKNOT)	+=  GT9XXTB_hotknot/
obj-$(CONFIG_TOUCHSCREEN_MTK_SYNAPTICS_I2C_RMI4)	+=  synaptics_i2c_rmi4/
