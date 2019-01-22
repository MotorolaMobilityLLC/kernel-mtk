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

ifeq ($(CONFIG_FTRACE),y)
ifeq ($(CONFIG_TRACING),y)
obj-y += met_smi.o
endif
endif

ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6759"))
ccflags-y += -DSMI_MET_ALA
obj-y += met_smi_configuration_6759.o
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6763"))
ccflags-y += -DSMI_MET_BIA
obj-y += met_smi_configuration_6763.o
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6758"))
ccflags-y += -DSMI_MET_VIN
obj-y += met_smi_configuration_6758.o
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6739"))
ccflags-y += -DSMI_MET_ZIO
obj-y += met_smi_configuration_6739.o
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6775"))
ccflags-y += -DSMI_MET_CAN
obj-y += met_smi_configuration_6775.o
else ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6771"))
ccflags-y += -DSMI_MET_SYL
obj-y += met_smi_configuration_6771.o
endif
