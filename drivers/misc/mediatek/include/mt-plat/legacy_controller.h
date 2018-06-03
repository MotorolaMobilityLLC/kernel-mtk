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

#define PPM_KIR_PERF 0
#define PPM_KIR_FBC 1
#define PPM_MAX_KIR 2

#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6797) || defined(CONFIG_MACH_MT6759)
#define NR_PPM_CLUSTERS 3
#else
#define NR_PPM_CLUSTERS 2
#endif

extern char *ppm_copy_from_user_for_proc(const char __user *buffer, size_t count);
int update_userlimit_cpu_freq(int kicker, int num_cluster, struct ppm_limit_data *freq_limit);
int update_userlimit_cpu_core(int kicker, int num_cluster, struct ppm_limit_data *core_limit);
