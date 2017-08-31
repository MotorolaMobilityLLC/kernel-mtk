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

#ifndef __SCP_FEATURE_DEFINE_H__
#define __SCP_FEATURE_DEFINE_H__



static scp_feature_table_t feature_table[] = {
	{
		.feature    = VOW_FEATURE_ID,
		.freq       = 63,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
	{
		.feature    = OPEN_DSP_FEATURE_ID,
		.freq       = 356,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
	{
		.feature    = SENS_FEATURE_ID,
		.freq       = 84,
		.core       = SCP_A_ID,
		.enable     = 0,
	},
	{
		.feature    = MP3_FEATURE_ID,
		.freq       = 47,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
	{
		.feature    = FLP_FEATURE_ID,
		.freq       = 26,
		.core       = SCP_A_ID,
		.enable     = 0,
	},
	{
		.feature    = RTOS_FEATURE_ID,
		.freq       = 0,
		.core       = SCP_CORE_TOTAL,
		.enable     = 0,
	},

};


#endif
