ifeq ($(CONFIG_MTK_SYS_CIRQ),y)
obj-y += mt_sys_cirq.o
# include platform-dependent headers
# ccflags-y += -I$(srctree)/drivers/misc/mediatek/cirq/$(CONFIG_MTK_PLATFORM)/inc/
ifeq ($(CONFIG_MT_ENG_BUILD),y)
CFLAGS_mt_sys_cirq.o := -DDEBUG
endif
else
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
