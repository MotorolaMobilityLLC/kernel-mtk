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

#include <mach/mtk_ppm_api.h>

#define PPM_KIR_PERF  0
#define PPM_KIR_FBC   1
#define PPM_KIR_WIFI  2
#define PPM_KIR_BOOT  3
#define PPM_KIR_TOUCH 4
#define PPM_MAX_KIR   5

extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);
int update_userlimit_cpu_freq(int kicker, int num_cluster, struct ppm_limit_data *freq_limit);
int update_userlimit_cpu_core(int kicker, int num_cluster, struct ppm_limit_data *core_limit);
