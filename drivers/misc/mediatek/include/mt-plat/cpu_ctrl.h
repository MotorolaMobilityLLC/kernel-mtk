/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <mtk_ppm_api.h>

enum {
	PPM_KIR_PERF = 0,
	PPM_KIR_FBC,
	PPM_KIR_WIFI,
	PPM_KIR_BOOT,
	PPM_KIR_TOUCH,
	PPM_KIR_PERFTOUCH,
	PPM_KIR_USB,
	PPM_MAX_KIR
};
extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);
int update_userlimit_cpu_freq(int kicker, int num_cluster
				, struct ppm_limit_data *freq_limit);
int update_userlimit_cpu_core(int kicker, int num_cluster
				, struct ppm_limit_data *core_limit);
