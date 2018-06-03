/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/module.h>       /* needed by all modules */
#include "scp_feature_define.h"
#include "scp_ipi.h"

scp_feature_tb_t feature_table[NUM_FEATURE_ID] = {
	{
		.feature    = VOW_FEATURE_ID,
		.freq       = 75,
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

#if SCP_VCORE_TEST_ENABLE
	{
		.feature    = VCORE_TEST_FEATURE_ID,
		.freq       = 50,
		.core       = SCP_A_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST2_FEATURE_ID,
		.freq       = 150,
		.core       = SCP_A_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST3_FEATURE_ID,
		.freq       = 250,
		.core       = SCP_A_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST4_FEATURE_ID,
		.freq       = 300,
		.core       = SCP_A_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST5_FEATURE_ID,
		.freq       = 400,
		.core       = SCP_A_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST6_FEATURE_ID,
		.freq       = 50,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST7_FEATURE_ID,
		.freq       = 150,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST8_FEATURE_ID,
		.freq       = 250,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST9_FEATURE_ID,
		.freq       = 300,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
	{
		.feature    = VCORE_TEST10_FEATURE_ID,
		.freq       = 400,
		.core       = SCP_B_ID,
		.enable     = 0,
	},
#endif
};

