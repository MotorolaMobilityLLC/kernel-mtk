
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmi/inc  \
             -I$(srctree)/drivers/misc/mediatek/include/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/ \
             -I$(srctree)/drivers/misc/mediatek/ext_disp/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/lcm/inc \
             -I$(srctree)/drivers/misc/mediatek/pmic/include/mt6336/ \
             -I$(srctree)/drivers/misc/mediatek/pmic/include/

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/

mi2-objs := anx7625_driver.o   anx7625_private_interface.o  anx7625_public_interface.o Flash.o display.o display_drv.o display_platform.o anx7625_display.o anx7625_display_table.o anx7625_pd30_message.o anx7625_pd_fw_update.o display_edid_3d.o
obj-y += mi2.o

