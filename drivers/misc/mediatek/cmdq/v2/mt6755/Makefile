# drivers/cmdq

ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v2

# Enable MET
ccflags-y += -DMET_USER_EVENT_SUPPORT

# Common Clock Framework
ifneq ($(CONFIG_MTK_CLKMGR),y)
ccflags-y += -DCMDQ_USE_CCF
endif

obj-y += cmdq_mdp.o