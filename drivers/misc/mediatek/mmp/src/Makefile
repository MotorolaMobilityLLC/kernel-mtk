ccflags-y += -I$(srctree)/drivers/misc/mediatek/mmp/

obj-y += mmprofile.o

# while the project is not GMO RAM optimize, other kernel modules can start MMP
#   at init (which will occupy some memory at system boot-up)
ifeq ($(CONFIG_MTK_GMO_RAM_OPTIMIZE),y)
ccflags-y += -DFORBID_MMP_START
endif
