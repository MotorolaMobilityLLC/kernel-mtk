obj-y += perfmgr_main.o

ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/performance/perfmgr/$(MTK_PLATFORM)/),)
obj-y += perfmgr_touch.o \
         $(MTK_PLATFORM)/
ccflags-y += -DMTK_BOOST_SUPPORT
endif # gator-driver folder exist

#ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)/

