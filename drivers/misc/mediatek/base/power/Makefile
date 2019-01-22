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

obj-$(CONFIG_MACH_MT6757) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_MACH_KIBOPLUS) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_MACH_MT6799) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_MACH_MT6759) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
ifneq ($(CONFIG_FPGA_EARLY_PORTING), y)
obj-$(CONFIG_MACH_MT6763) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
obj-$(CONFIG_MACH_MT6759) += udi_v1/
obj-$(CONFIG_MACH_MT6763) += udi_v1/
obj-$(CONFIG_MACH_MT6759) += ocp_v1/
obj-$(CONFIG_MACH_MT6799) += leakage_table_v1/
obj-$(CONFIG_MACH_MT6759) += leakage_table_v2/
obj-$(CONFIG_MACH_MT6763) += leakage_table_v2/
obj-$(CONFIG_MACH_MT6799) += cpufreq_v1/
obj-$(CONFIG_MACH_MT6763) += cpufreq_v1/
obj-$(CONFIG_MACH_MT6759) += cpufreq_v1/
obj-$(CONFIG_MACH_MT6799) += eem_v1/
obj-$(CONFIG_MACH_MT6759) += eem_v2/
obj-$(CONFIG_MACH_MT6763) += eem_v2/
obj-$(CONFIG_MACH_MT6799) += upower_v1/
ifeq ($(CONFIG_MTK_UNIFY_POWER), y)
obj-$(CONFIG_MACH_MT6759) += upower_v2/
obj-$(CONFIG_MACH_MT6763) += upower_v2/
endif
obj-$(CONFIG_MACH_MT6757) += pbm_v1/
obj-$(CONFIG_MACH_KIBOPLUS) += pbm_v1/
obj-$(CONFIG_MACH_MT6799) += pbm_v2/
obj-$(CONFIG_MACH_MT6758) += pbm_v2/
obj-$(CONFIG_MACH_MT6759) += pbm_v2/
obj-$(CONFIG_MACH_MT6763) += pbm_v3/
obj-$(CONFIG_MACH_MT6757) += spm_v2/
obj-$(CONFIG_MACH_MT6757) += vcorefs_v1/
obj-$(CONFIG_MACH_MT6799) += vcorefs_v3/
obj-$(CONFIG_MACH_MT6763) += vcorefs_v3/
obj-$(CONFIG_MACH_MT6759) += vcorefs_v3/
obj-$(CONFIG_MACH_KIBOPLUS) += vcorefs_v1/
obj-$(CONFIG_MACH_KIBOPLUS) += spm_v2/
obj-$(CONFIG_MACH_MT6757) += cpuidle_v2/
obj-$(CONFIG_MACH_MT6799) += cpuidle_v2/
obj-$(CONFIG_MACH_MT6759) += cpuidle_v2/
obj-$(CONFIG_MACH_MT6763) += cpuidle_v2/
ifneq ($(CONFIG_FPGA_EARLY_PORTING),y)
obj-$(CONFIG_MACH_MT6757) += power_gs_v1/
obj-$(CONFIG_MACH_MT6799) += power_gs_v1/
obj-$(CONFIG_MACH_MT6759) += power_gs_v1/
obj-$(CONFIG_MACH_MT6763) += power_gs_v1/
endif
obj-$(CONFIG_MACH_MT6757) += hps_v2/
obj-$(CONFIG_MACH_MT6763) += hps_v3/
obj-$(CONFIG_MACH_MT6799) += hps_v3/
obj-$(CONFIG_MACH_MT6759) += hps_v3/
obj-$(CONFIG_MACH_KIBOPLUS) += hps_v2/
obj-$(CONFIG_MACH_MT6757) += ppm_v2/
obj-$(CONFIG_MACH_KIBOPLUS) += ppm_v2/
obj-$(CONFIG_MACH_MT6759) += ppm_v2/
ifeq ($(CONFIG_MTK_ACAO_SUPPORT), y)
obj-$(CONFIG_MACH_MT6758) += ppm_v3/
obj-$(CONFIG_MACH_MT6763) += ppm_v3/
else
obj-$(CONFIG_MACH_MT6758) += ppm_v2/
obj-$(CONFIG_MACH_MT6763) += ppm_v2/
endif
obj-$(CONFIG_MACH_MT6799) += ppm_v2/
obj-$(CONFIG_MACH_MT6758) += spm_v3/
obj-$(CONFIG_MACH_MT6759) += spm_v3/
obj-$(CONFIG_MACH_MT6799) += spm_v3/
obj-$(CONFIG_MACH_MT6763) += spm_v4/
ifneq ($(CONFIG_FPGA_EARLY_PORTING),y)
obj-$(CONFIG_MACH_MT6758) += clkbuf_v1/
obj-$(CONFIG_MACH_MT6759) += clkbuf_v1/
obj-$(CONFIG_MACH_MT6763) += clkbuf_v1/
obj-$(CONFIG_MACH_MT6799) += clkbuf_v1/
endif
obj-$(CONFIG_MACH_MT6759) += dcm_v1/
obj-$(CONFIG_MACH_MT6763) += dcm_v1/
obj-$(CONFIG_MACH_MT6799) += dcm_v1/
obj-$(CONFIG_MACH_MT8167) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_MACH_MT8167) += hps_v1/
ifeq ($(CONFIG_MTK_ACAO_SUPPORT), y)
obj-$(CONFIG_MACH_MT6763) += mcdi/
endif
