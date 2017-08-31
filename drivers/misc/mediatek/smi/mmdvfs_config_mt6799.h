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

#ifndef __MMDVFS_CONFIG_MT6799_H__
#define __MMDVFS_CONFIG_MT6799_H__

#include "mmdvfs_config_util.h"
#include "mtk_vcorefs_manager.h"

/* Part I MMSVFS HW Configuration (OPP)*/
/* Define the number of mmdvfs, vcore and mm clks opps */

#define MMDVFS_OPP_MAX 4    /* Max total MMDVFS opps of the profile support */
#define MMDVFS_CLK_OPP_MAX 4 /* Max total CLK opps of the profile support */
#define MMDVFS_VCORE_OPP_MAX 4 /* Max total CLK opps of the profile support */

/* CLK source configuration */

/* CLK source IDs */
/* Define the internal index of each CLK source*/
#define MMDVFS_CLK_TOP_MMPLL_D3 0
#define MMDVFS_CLK_TOP_MMPLL_D5 1
#define MMDVFS_CLK_TOP_MMPLL_D6 2
#define MMDVFS_CLK_TOP_SYSPLL_D2 3
#define MMDVFS_CLK_TOP_SYSPLL_D3 4
#define MMDVFS_CLK_TOP_SYSPLL_D5 5
#define MMDVFS_CLK_TOP_SYSPLL1_D2 6
#define MMDVFS_CLK_TOP_ULPOSCPLL 7
#define MMDVFS_CLK_TOP_UNIVPLL_D5 8
#define MMDVFS_CLK_TOP_UNIVPLL1_D2 9
#define MMDVFS_CLK_TOP_VCODECPLL_D6 10
#define MMDVFS_CLK_TOP_VCODECPLL_D7 11
#define MMDVFS_CLK_TOP_FSMIPLL_D3 12
#define MMDVFS_CLK_SOURCE_NUM 13

/* CLK Source definiation */
/* Define the clk source description */
struct mmdvfs_clk_source_desc mmdvfs_clk_sources_setting[MMDVFS_CLK_SOURCE_NUM] = {
		{NULL, "MMDVFS_CLK_TOP_MMPLL_D3", 800},
		{NULL, "MMDVFS_CLK_TOP_MMPLL_D5", 480},
		{NULL, "MMDVFS_CLK_TOP_MMPLL_D6", 400},
		{NULL, "MMDVFS_CLK_TOP_SYSPLL_D2", 546},
		{NULL, "MMDVFS_CLK_TOP_SYSPLL_D3", 364},
		{NULL, "MMDVFS_CLK_TOP_SYSPLL_D5", 218},
		{NULL, "MMDVFS_CLK_TOP_SYSPLL1_D2", 273},
		{NULL, "MMDVFS_CLK_TOP_ULPOSCPLL", 330},
		{NULL, "MMDVFS_CLK_TOP_UNIVPLL_D5", 250},
		{NULL, "MMDVFS_CLK_TOP_UNIVPLL1_D2", 312},
		{NULL, "MMDVFS_CLK_TOP_VCODECPLL_D6", 490},
		{NULL, "MMDVFS_CLK_TOP_VCODECPLL_D7", 420},
		{NULL, "MMDVFS_CLK_TOP_FSMIPLL_D3", 960},
};

/* B. CLK Change adaption configurtion */
/* Define the clk change method and related infomration of each MM CLK step */
/* Field decscription: */
/* 1. config_method: MMDVFS_CLK_CONFIG_BY_MUX/ MMDVFS_CLK_CONFIG_HOPPING/ MMDVFS_CLK_CONFIG_NONE */
/* 2. pll_id: MTK' PLL ID, please set -1 if PLL hopping is not used */
/* 3. clk mux desc {hanlde, name}, the please handle -1 and it will be initialized by driver automaticlly */
/* 4. total step: the number of the steps supported by this sub sys */
/* 5. hopping dss of each steps: please set -1 if it is not used */
/* 6. clk sources id of each steps: please set -1 if it is not used */
struct mmdvfs_clk_hw_map mmdvfs_clk_hw_map_setting[MMDVFS_CLK_MUX_NUM] = {
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_SMI0_2X_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_FSMIPLL_D3, MMDVFS_CLK_TOP_MMPLL_D3, MMDVFS_CLK_TOP_SYSPLL_D2}
		},
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_MM_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_VCODECPLL_D7, MMDVFS_CLK_TOP_SYSPLL_D3, MMDVFS_CLK_TOP_SYSPLL1_D2}
		},
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_CAM_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_VCODECPLL_D6, MMDVFS_CLK_TOP_SYSPLL_D3, MMDVFS_CLK_TOP_SYSPLL1_D2}
		},
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_IMG_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_MMPLL_D5, MMDVFS_CLK_TOP_SYSPLL_D3, MMDVFS_CLK_TOP_SYSPLL1_D2}
		},
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_VENC_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_VCODECPLL_D6, MMDVFS_CLK_TOP_MMPLL_D6, MMDVFS_CLK_TOP_UNIVPLL1_D2}
		},
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_VDEC_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_VCODECPLL_D6, MMDVFS_CLK_TOP_SYSPLL_D3, MMDVFS_CLK_TOP_UNIVPLL_D5}
		},
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_MJC_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_VCODECPLL_D7, MMDVFS_CLK_TOP_ULPOSCPLL, MMDVFS_CLK_TOP_SYSPLL_D5}
		},
		{ MMDVFS_CLK_CONFIG_BY_MUX, { NULL, "MMDVFS_CLK_MUX_TOP_DSP_SEL"}, -1, 3,
			{-1, -1, -1},
			{MMDVFS_CLK_TOP_MMPLL_D5, MMDVFS_CLK_TOP_SYSPLL_D3, MMDVFS_CLK_TOP_SYSPLL1_D2}
		},
		{ MMDVFS_CLK_CONFIG_NONE,   { NULL, "MMDVFS_CLK_MUX_TOP_VPU_IF_SEL"}, -1, 3,
			{-1, -1, -1},
			{-1, -1, -1}
		},
		{ MMDVFS_CLK_CONFIG_NONE, { NULL, "MMDVFS_CLK_MUX_TOP_VPU_SEL"}, -1, 3,
			{-1, -1, -1 },
			{-1, -1, -1 }
		}
};



/* Part II MMDVFS Scenario's Step Confuguration */

/* A. Scenarios of each MM DVFS Step */
/* OOP 0 scenarios */
#define MMDVFS_OPP0_NUM 14

struct mmdvfs_profile mmdvfs_opp0_profiles[MMDVFS_OPP0_NUM] = {
		{"PIP Feature Preview", SMI_BWC_SCEN_CAM_PV, {0, MMDVFS_CAMERA_MODE_FLAG_PIP, 0}, {0, 0, 0 } },
		{"PIP Feature Capture", SMI_BWC_SCEN_CAM_CP, {0, MMDVFS_CAMERA_MODE_FLAG_PIP, 0}, {0, 0, 0 } },
		{"PIP Feature Recording", SMI_BWC_SCEN_VR, {0, MMDVFS_CAMERA_MODE_FLAG_PIP, 0}, {0, 0, 0 } },
		{"vFB Feature Preview", SMI_BWC_SCEN_CAM_PV, {0, MMDVFS_CAMERA_MODE_FLAG_VFB, 0}, {0, 0, 0 } },
		{"vFB Feature Capture", SMI_BWC_SCEN_CAM_CP, {0, MMDVFS_CAMERA_MODE_FLAG_VFB, 0}, {0, 0, 0 } },
		{"vFB Feature Recording", SMI_BWC_SCEN_VR, {0, MMDVFS_CAMERA_MODE_FLAG_VFB, 0}, {0, 0, 0 } },
		{"EIS Feature Recording", SMI_BWC_SCEN_VR, {0, MMDVFS_CAMERA_MODE_FLAG_EIS_2_0, 0}, {0, 0, 0 } },
		{"Stereo Feature Preview", SMI_BWC_SCEN_CAM_PV, {0, MMDVFS_CAMERA_MODE_FLAG_STEREO, 0}, {0, 0, 0 } },
		{"Stereo Feature Capture", SMI_BWC_SCEN_CAM_CP, {0, MMDVFS_CAMERA_MODE_FLAG_STEREO, 0}, {0, 0, 0 } },
		{"Stereo Feature Recording", SMI_BWC_SCEN_VR, {0, MMDVFS_CAMERA_MODE_FLAG_STEREO, 0}, {0, 0, 0 } },
		{"4K Video Play Back", SMI_BWC_SCEN_VP_HIGH_RESOLUTION, {0, 0, 0}, {0, 0, 0 } },
		{"ICFP", SMI_BWC_SCEN_ICFP, {0, 0, 0}, {0, 0, 0 } },
		{"SMVR", SMI_BWC_SCEN_VR_SLOW, {0, 0, 0}, {0, 0, 0 } },
		{"4K VR/ VSS (VENC)", SMI_BWC_SCEN_VENC, {0, 0, 0}, {4096, 1716, 0} },

};

/* OOP 1 scenarios */
#define MMDVFS_OPP1_NUM 6
#define MMDVFS_OPP1_SENSOR_MIN (20000000)
#define MMDVFS_NORMAL_CAM_FPS_MIN_FPS (0)

struct mmdvfs_profile mmdvfs_opp1_profiles[MMDVFS_OPP1_NUM] = {
		{"Dual zoom preview", SMI_BWC_SCEN_CAM_PV, {0, MMDVFS_CAMERA_MODE_FLAG_DUAL_ZOOM, 0}, {0, 0, 0 } },
		{"Dual zoom preview (reserved)", SMI_BWC_SCEN_CAM_CP,
				{0, MMDVFS_CAMERA_MODE_FLAG_DUAL_ZOOM, 0}, {0, 0, 0 } },
		{"Dual zoom preview (reserved)", SMI_BWC_SCEN_VR,
				{0, MMDVFS_CAMERA_MODE_FLAG_DUAL_ZOOM, 0}, {0, 0, 0 } },
		{"Full Sensor Preview (ZSD)", SMI_BWC_SCEN_CAM_PV, {MMDVFS_OPP1_SENSOR_MIN, 0, 0}, {0, 0, 0 } },
		{"Full Sensor Capture (ZSD)", SMI_BWC_SCEN_CAM_CP, {MMDVFS_OPP1_SENSOR_MIN, 0, 0}, {0, 0, 0 } },
		{"Full Sensor Camera Recording", SMI_BWC_SCEN_VR, {MMDVFS_OPP1_SENSOR_MIN, 0, 0}, {0, 0, 0 } },
};

/* OOP 2 scenarios */
#define MMDVFS_OPP2_NUM 5

struct mmdvfs_profile mmdvfs_opp2_profiles[MMDVFS_OPP2_NUM] = {
		{"Camera Preview", SMI_BWC_SCEN_CAM_PV, {0, 0, 0}, {0, 0, 0 } },
		{"Camera Capture", SMI_BWC_SCEN_CAM_CP, {0, 0, 0}, {0, 0, 0 } },
		{"Camera Recording", SMI_BWC_SCEN_VR, {0, 0, 0}, {0, 0, 0 } },
		{"VENC", SMI_BWC_SCEN_VENC, {0, 0, 0}, {0, 0, 0} },
		{"VSS", SMI_BWC_SCEN_VSS, {0, 0, 0}, {0, 0, 0 } },
};

/* OOP 3 scenarios */
#define MMDVFS_OPP3_NUM 2

struct mmdvfs_profile mmdvfs_opp3_profiles[MMDVFS_OPP3_NUM] = {
		{"MHL", MMDVFS_SCEN_MHL, {0, 0, 0}, {0, 0, 0 } },
		{"WFD", SMI_BWC_SCEN_WFD, {0, 0, 0}, {0, 0, 0 } },
};

/* Defined the smi scenarios whose DVFS is controlled by low-level driver */
/* directly, not by BWC scenario change event */
#define MMDVFS_SMI_USER_CONTROL_SCEN_MASK (1 << SMI_BWC_SCEN_VP)


/* Part III Scenario and MMSVFS HW configuration mapping */
/* For a single mmdvfs step's profiles and associated hardware configuration */
struct mmdvfs_step_to_profile_mapping mmdvfs_step_to_profile_mappings_setting[MMDVFS_OPP_MAX] = {
		{0, mmdvfs_opp0_profiles, MMDVFS_OPP0_NUM,
		{OPP_0,
		{MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0,
		MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0,
		MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0}, MMDVFS_CLK_MUX_NUM
		}
		},
		{1, mmdvfs_opp1_profiles, MMDVFS_OPP1_NUM,
		{OPP_1,
		{MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0,
		MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0,
		MMDVFS_MMCLK_OPP0, MMDVFS_MMCLK_OPP0}, MMDVFS_CLK_MUX_NUM
		}
		},
		{2, mmdvfs_opp2_profiles, MMDVFS_OPP2_NUM,
		{OPP_2,
		{MMDVFS_MMCLK_OPP1, MMDVFS_MMCLK_OPP1, MMDVFS_MMCLK_OPP1, MMDVFS_MMCLK_OPP1,
		MMDVFS_MMCLK_OPP1, MMDVFS_MMCLK_OPP1, MMDVFS_MMCLK_OPP1, MMDVFS_MMCLK_OPP1,
		MMDVFS_MMCLK_OPP1, MMDVFS_MMCLK_OPP1}, MMDVFS_CLK_MUX_NUM
		}
		},
		{3, mmdvfs_opp3_profiles, MMDVFS_OPP3_NUM,
		{OPP_3,
		{MMDVFS_MMCLK_OPP2, MMDVFS_MMCLK_OPP2, MMDVFS_MMCLK_OPP2, MMDVFS_MMCLK_OPP2,
		MMDVFS_MMCLK_OPP2, MMDVFS_MMCLK_OPP2, MMDVFS_MMCLK_OPP2, MMDVFS_MMCLK_OPP2,
		MMDVFS_MMCLK_OPP2, MMDVFS_MMCLK_OPP2}, MMDVFS_CLK_MUX_NUM
		}
		},
};

/* Part III Scenario and MMSVFS HW configuration mapping */

#define MMDVFS_VOLTAGE_LOW_OPP	1
#define MMDVFS_VOLTAGE_HIGH_OPP	0
#define MMDVFS_VOLTAGE_DEFAULT_STEP_OPP	-1
#define MMDVFS_VOLTAGE_LOW_LOW_OPP 2

int mmdvfs_legacy_step_to_opp[MMDVFS_VOLTAGE_COUNT] = {MMDVFS_VOLTAGE_LOW_OPP,
	MMDVFS_VOLTAGE_HIGH_OPP, MMDVFS_VOLTAGE_DEFAULT_STEP_OPP, MMDVFS_VOLTAGE_LOW_LOW_OPP
};

#define MMDVFS_OPP0_LEGACY_STEP	MMSYS_CLK_HIGH
#define MMDVFS_OPP1_LEGACY_STEP	MMSYS_CLK_HIGH
#define MMDVFS_OPP2_LEGACY_STEP	MMSYS_CLK_MEDIUM
#define MMDVFS_OPP3_LEGACY_STEP	MMSYS_CLK_LOW

int mmdvfs_mmclk_opp_to_legacy_mmclk_step[MMDVFS_OPP_MAX] = {MMDVFS_OPP0_LEGACY_STEP, MMDVFS_OPP1_LEGACY_STEP,
MMDVFS_OPP2_LEGACY_STEP, MMDVFS_OPP3_LEGACY_STEP
};



#endif /* __MMDVFS_CONFIG_MT6799_H__ */
