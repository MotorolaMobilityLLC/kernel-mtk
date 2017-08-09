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

# common driver code for CMDQ
# 72/82/92 does not use this common code

# use common driver
ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6795" "mt6752" "mt6735" "mt6580" "mt6755" "mt6797"))
  # v2 driver
  obj-y += v2/
endif

ifeq ($(CONFIG_MTK_CMDQ_TAB), y)
	# hv1
  # obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
  obj-y += hv1/
endif
# EOF