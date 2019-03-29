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

#ifndef __MMDVFS_CONFIG_MT6779_H__
#define __MMDVFS_CONFIG_MT6779_H__

#include "mmdvfs_config_util.h"

/* Part I MMSVFS HW Configuration (OPP)*/
/* Define the number of mmdvfs, vcore and mm clks opps */

/* Max total MMDVFS opps of the profile support */
#define MT6779_MMDVFS_OPP_MAX 3

struct mmdvfs_profile_mask qos_apply_profiles[] = {
	/* debug entry */
	{"DEBUG",
		0,
		MMDVFS_FINE_STEP_UNREQUEST },
};

/* Part II MMDVFS Scenario's Step Confuguration */
/* A.1 [LP4 2-ch] Scenarios of each MM DVFS Step (force kicker) */
/* OPP 0 scenarios */
#define MT6779_MMDVFS_OPP0_NUM 0
struct mmdvfs_profile mt6779_mmdvfs_opp0_profiles[MT6779_MMDVFS_OPP0_NUM] = {
};

/* OPP 1 scenarios */
#define MT6779_MMDVFS_OPP1_NUM 0
struct mmdvfs_profile mt6779_mmdvfs_opp1_profiles[MT6779_MMDVFS_OPP1_NUM] = {
};

/* OPP 2 scenarios */
#define MT6779_MMDVFS_OPP2_NUM 0
struct mmdvfs_profile mt6779_mmdvfs_opp2_profiles[MT6779_MMDVFS_OPP2_NUM] = {
};

struct mmdvfs_step_to_qos_step legacy_to_qos_step[MT6779_MMDVFS_OPP_MAX] = {
	{0, 0},
	{1, 1},
	{2, 2},
};

struct mmdvfs_step_profile mt6779_step_profile[MT6779_MMDVFS_OPP_MAX] = {
	{0, mt6779_mmdvfs_opp0_profiles, MT6779_MMDVFS_OPP0_NUM, {0} },
	{1, mt6779_mmdvfs_opp1_profiles, MT6779_MMDVFS_OPP1_NUM, {0} },
	{2, mt6779_mmdvfs_opp2_profiles, MT6779_MMDVFS_OPP2_NUM, {0} },
};
#endif /* __MMDVFS_CONFIG_MT6779_H__ */
