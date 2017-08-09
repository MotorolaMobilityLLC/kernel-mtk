################################################################################
# COMMON PART
# 1. xxx.h/c: Source files that is common for all chips
# 2. IPvx/: Common part for a Chipset Groups
#
# When one new chip coming, you can select an existing common part, ex. IPv1 or
# you can create a new group, ex. IPv2
################################################################################



#####################################
# MT67xx chipset group
#obj-$(CONFIG_ARCH_MT6735) += IPv1/
#obj-$(CONFIG_ARCH_MT6580) += IPv1/
#obj-$(CONFIG_ARCH_MT6755) += IPv1/


#####################################
# Add new chipset groups here


#####################################
# Common Source files for all chips
obj-$(CONFIG_MTK_FB) += 			\
				data_rgb888_64x64.o		\
				data_rgb888_64x64_golden.o


ccflags-y += \
				-I$(srctree)/drivers/misc/mediatek/video/include     \
				-I$(srctree)/drivers/misc/mediatek/video/common/IPv1     \
				-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys \
				-I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox \

