obj-y                             += fb_notify.o
obj-$(CONFIG_FB_CMDLINE)          += fb_cmdline.o
obj-$(CONFIG_FB)                  += fb.o
fb-y                              := fbmem.o fbmon.o fbcmap.o fbsysfs.o \
                                     modedb.o fbcvt.o
fb-objs                           := $(fb-y)

obj-$(CONFIG_FB_CFB_FILLRECT)  += cfbfillrect.o
obj-$(CONFIG_FB_CFB_COPYAREA)  += cfbcopyarea.o
obj-$(CONFIG_FB_CFB_IMAGEBLIT) += cfbimgblt.o
obj-$(CONFIG_FB_SYS_FILLRECT)  += sysfillrect.o
obj-$(CONFIG_FB_SYS_COPYAREA)  += syscopyarea.o
obj-$(CONFIG_FB_SYS_IMAGEBLIT) += sysimgblt.o
obj-$(CONFIG_FB_SYS_FOPS)      += fb_sys_fops.o
obj-$(CONFIG_FB_SVGALIB)       += svgalib.o
obj-$(CONFIG_FB_DDC)           += fb_ddc.o
obj-$(CONFIG_FB_DEFERRED_IO)   += fb_defio.o

ifeq ($(CONFIG_LCT_CABC_MODE_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/mt6735
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/mt6735/mt6735
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/mt6735
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/lct_hx8394f_booyitech_720p_vdo
endif

ifeq ($(CONFIG_WIND_CABC_MODE_SUPPORT),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/mt6735
ccflags-y += -I$(srctree)/drivers/misc/mediatek/leds/mt6735
ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/mt6735/mt6735
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2/mt6735
endif

