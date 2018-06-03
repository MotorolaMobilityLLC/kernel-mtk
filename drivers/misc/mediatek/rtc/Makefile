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

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

#ifneq (,$(filter $(strip $(CONFIG_MTK_PLATFORM)), mt6735 mt6735m mt6753 mt6580 mt6572 mt6582 mt6592 mt6752 mt6795 mt6755 mt6797))

#set IP folder name
ifneq (,$(filter y,$(CONFIG_MACH_MT6735) $(CONFIG_MACH_MT6735M) $(CONFIG_MACH_MT6753)))
    MDI_IP_FOLDER:=mt6328
else
    ifneq (,$(filter y,$(CONFIG_MACH_MT6795)))
	    MDI_IP_FOLDER:=mt6331
    else
        ifneq (,$(filter y,$(CONFIG_MACH_MT6572) $(CONFIG_MACH_MT6582) $(CONFIG_MACH_MT6592) $(CONFIG_MACH_MT8163) $(CONFIG_MACH_MT8127) $(CONFIG_MACH_MT8160)))
	        MDI_IP_FOLDER:=mt6323
        else
            ifneq (,$(filter y,$(CONFIG_MACH_MT6752)))
	            MDI_IP_FOLDER:=mt6325
            else
              ifneq (,$(filter y,$(CONFIG_MACH_MT6580)))
	              MDI_IP_FOLDER:=mt6350
              else
                ifneq (,$(filter y,$(CONFIG_MACH_MT6755) $(CONFIG_MACH_MT6797) $(CONFIG_MACH_MT6757)))
	                MDI_IP_FOLDER:=mt6351
	              else
	                ifneq (,$(filter y,$(CONFIG_MACH_MT8173)))
	                MDI_IP_FOLDER:=mt6391
	                endif
                endif
              endif
            endif
        endif
    endif
endif

ifneq (,$(filter y,$(CONFIG_MTK_PMIC_CHIP_MT6335)))
  MDI_IP_FOLDER:=mt6335
endif

ifneq (,$(filter y,$(CONFIG_MTK_PMIC_CHIP_MT6355)))
  MDI_IP_FOLDER:=mt6355
endif

MDP_COMMON_FOLDER:= ./

#Top level common code
obj-$(CONFIG_MTK_RTC) += mtk_rtc_common.o mtk_rtc_hal_common.o

MTK_MDI_IP_SRC:= $(MDI_IP_FOLDER)/

#IP dependent part
obj-y += \
    $(MTK_MDI_IP_SRC)

