ccflags += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/include/trustzone/m4u
ifeq ($(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT),y)
ccflags-y += -I$(srctree)/include/trustzone/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/trustzone
endif
ccflags-y += -I$(srctree)/drivers/iommu
obj-y += pseudo_m4u.o
