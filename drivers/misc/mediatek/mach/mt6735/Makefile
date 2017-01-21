#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#


# Makefile for the linux kernel.
#
# Only aarch32 need to build core/ (need machine descriptor)


ifneq ($(CONFIG_ARM64), y)
# For arm32
obj-y += mt_cpuxgpt.o
obj-y += mt_gpt.o
obj-y += ca53_timer.o

# Module project related part
#obj-y += $(ARCH_MTK_PROJECT)/

else
############################ For arm64 #############################

obj-y += mt_cpuxgpt.o
obj-y += mt_gpt.o
obj-y += ca53_timer.o
# Module project related part
#obj-y += $(ARCH_MTK_PROJECT)/

endif
#common part
