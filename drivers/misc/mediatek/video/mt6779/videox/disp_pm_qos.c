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

#include "disp_pm_qos.h"
#include <linux/pm_qos.h>
#include "helio-dvfsrc-opp-mt3967.h"
#include "layering_rule.h"
#include "mmprofile.h"
#include "mmprofile_function.h"
#include "disp_drv_log.h"

static struct pm_qos_request bw_request;
static struct pm_qos_request ddr_opp_request;
static struct pm_qos_request mm_freq_request;

static enum ddr_opp __remap_to_opp(enum HRT_LEVEL hrt)
{
	enum ddr_opp opp = PM_QOS_DDR_OPP_DEFAULT_VALUE;

	switch (hrt) {
	case HRT_LEVEL_LEVEL0:
		opp = DDR_OPP_4; /* LP4-1200 */
		break;
	case HRT_LEVEL_LEVEL1:
		opp = DDR_OPP_2; /* LP4-2100 */
		break;
	case HRT_LEVEL_LEVEL2:
		opp = DDR_OPP_1; /* LP4-2800 */
		break;
	case HRT_LEVEL_LEVEL3:
		opp = DDR_OPP_0; /* LP4-3200 */
		break;
	case HRT_LEVEL_DEFAULT:
		opp = PM_QOS_DDR_OPP_DEFAULT_VALUE;
		break;
	default:
		DISP_PR_ERR("%s:unknown hrt level:%d\n", __func__, hrt);
		break;
	}

	return opp;
}

void disp_pm_qos_init(void)
{
	pm_qos_add_request(&bw_request, PM_QOS_MM_MEMORY_BANDWIDTH,
			   PM_QOS_DEFAULT_VALUE);
	pm_qos_add_request(&ddr_opp_request, PM_QOS_DDR_OPP,
			   PM_QOS_DDR_OPP_DEFAULT_VALUE);
	pm_qos_add_request(&mm_freq_request, PM_QOS_DISP_FREQ,
			   PM_QOS_MM_FREQ_DEFAULT_VALUE);
}

void disp_pm_qos_deinit(void)
{
	pm_qos_remove_request(&bw_request);
	pm_qos_remove_request(&ddr_opp_request);
	pm_qos_remove_request(&mm_freq_request);
}

int disp_pm_qos_request_dvfs(enum HRT_LEVEL hrt)
{
	enum ddr_opp opp = PM_QOS_DDR_OPP_DEFAULT_VALUE;

	opp = __remap_to_opp(hrt);
	mmprofile_log_ex(ddp_mmp_get_events()->dvfs, MMPROFILE_FLAG_START,
			 hrt, opp);
	pm_qos_update_request(&ddr_opp_request, opp);
	mmprofile_log_ex(ddp_mmp_get_events()->dvfs, MMPROFILE_FLAG_END, 0, 0);

	return 0;
}
