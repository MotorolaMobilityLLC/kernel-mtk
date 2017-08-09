obj-y += perfmgr_main.o

ifeq ($(CONFIG_MTK_PERFMGR_TOUCH_BOOST),y)
ccflags-y += -DMTK_TOUCH_BOOST

obj-y += perfmgr_touch.o

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/performance/perfmgr/$(MTK_PLATFORM)/),)
ccflags-y += -DMTK_BOOST_SUPPORT
obj-y += $(MTK_PLATFORM)/
endif

endif

#ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/

