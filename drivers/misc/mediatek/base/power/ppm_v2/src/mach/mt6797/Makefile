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

ccflags-y +=	\
	-I$(PPM_ROOT_DIR)/inc \
	-I$(srctree)/drivers/misc/mediatek/base/power/$(CONFIG_MTK_PLATFORM)/

# platform dependent functions
obj-y += mt_ppm_platform.o
# power state settings
obj-y += mt_ppm_power_state.o
# power data related
obj-y += mt_ppm_power_data.o
# limit calculation algorithm
obj-y += mt_ppm_cobra_algo.o
