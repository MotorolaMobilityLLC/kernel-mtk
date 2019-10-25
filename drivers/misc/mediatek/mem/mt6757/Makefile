
# Makefile for the linux kernel.
#
# Only aarch32 need to build core/ (need machine descriptor)


ifneq ($(CONFIG_ARM64), y)
# For arm32
obj-y += mtk_hooks.o

else
############################ For arm64 #############################

obj-y += mtk_hooks_64.o

endif
#common part
