
# Makefile for the linux kernel.
#
# Only aarch32 need to build core/ (need machine descriptor)


ifneq ($(CONFIG_ARM64), y)
# For arm32
obj-y += mt_cache_v7.o

# Module project related part
#obj-y += $(ARCH_MTK_PROJECT)/

else
############################ For arm64 #############################

obj-y += mt_cache_v8.o
# Module project related part
#obj-y += $(ARCH_MTK_PROJECT)/

endif
#common part
