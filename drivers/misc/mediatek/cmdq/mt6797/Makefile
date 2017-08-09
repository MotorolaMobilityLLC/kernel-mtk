# include $(srctree)/drivers/misc/mediatek/Makefile.custom

# drivers/cmdq

ccflags-y += -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM) \
	     -I$(srctree)/drivers/misc/mediatek/cmdq

# Enable MET
ccflags-y += -DMET_USER_EVENT_SUPPORT

# Common Clock Framework
ifneq ($(CONFIG_MTK_CLKMGR),y)
ccflags-y += -DCMDQ_USE_CCF
endif

obj-y += cmdq_mdp.o
obj-y += cmdq_platform.o
