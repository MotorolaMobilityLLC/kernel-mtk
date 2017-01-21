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

#ccflags-y += -Werror

ifeq ($(CONFIG_MTK_GPU_R5P0),y)
$(info GPU BUILD r5p0)
obj-y += mali-EAC/drivers/
else
$(info GPU BUILD r7p0)
obj-y += mali-r7p0/drivers/
endif
