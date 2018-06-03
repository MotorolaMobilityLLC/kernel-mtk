#
# Copyright (C) 2017 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See http://www.gnu.org/licenses/gpl-2.0.html for more details.
#

LINUXINCLUDE += -include $(srctree)/kernel/sched/sched.h

# For RT monitor
obj-$(CONFIG_MTK_RT_THROTTLE_MON) += rt_monitor.o

# sysfs interface for user to get EAS related info
obj-y += eas_plus.o
