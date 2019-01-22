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

#ifndef __PERFMGR_H__
#define __PERFMGR_H__

enum {
	FORCELIMIT_TYPE_SP     = 0,
	FORCELIMIT_TYPE_VR     = 1,
	FORCELIMIT_TYPE_CPUSET = 2,
	FORCELIMIT_TYPE_NUM    = 3,
};

struct forcelimit_data {
	int min_core;
	int max_core;
};

extern int init_perfmgr_touch(void);
extern int perfmgr_touch_suspend(void);

extern int  perfmgr_get_target_core(void);
extern int  perfmgr_get_target_freq(void);

extern void init_perfmgr_boost(void);
extern void perfmgr_boost(int enable, int core, int freq);

extern void perfmgr_forcelimit_cpuset_cancel(void);
extern void perfmgr_forcelimit_cpu_core(int type, int cluster_num, struct forcelimit_data *data);

#endif				/* !__PERFMGR_H__ */
