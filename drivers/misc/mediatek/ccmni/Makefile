ifeq ($(CONFIG_MTK_NET_CCMNI), y)
# include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y := eccmni.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/ccmni/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/c2k_ccci/

eccmni-y := ccmni.o

else
obj-  := dummy.o # avoid build error
endif
