# Copyright (C) 2017 MediaTek Inc.
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

#obj-y += fliper/
obj-y += perfmgr/
obj-y += eas_controller/
ifeq (,$(findstring mt8,$(CONFIG_MTK_PLATFORM)))
	obj-$(CONFIG_MTK_BASE_POWER) += legacy_controller/
endif
