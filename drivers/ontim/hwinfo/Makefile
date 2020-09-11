ccflags-$(CONFIG_HBM_SUPPORT) +=  \
         -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox \
         -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys \
	 -I$(srctree)/drivers/misc/mediatek/video/include/ \
         -I$(srctree)/drivers/misc/mediatek/lcm/inc \
         -I$(srctree)/drivers/misc/mediatek/cmdq/v3/

ifeq ($(CONFIG_MACH_MT6761),y)
     subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/mt6765/
else
     subdir-ccflags-$(CONFIG_HBM_SUPPORT) += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/
endif

ifeq ($(BUILD_SMT),YES)
    ccflags-y += -DSMT_VERSION
    $(info ++++++++++++++++++++++++++firefly++++++++++++++++ SMT version)
endif

obj-y += gpio_boardid.o
obj-y += hwinfo.o
#obj-y += board_id_adc.o
