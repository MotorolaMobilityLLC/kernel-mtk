/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef MTK_KBASE_PM_CA_RANDOM_H
#define MTK_KBASE_PM_CA_RANDOM_H

/**
 * Private structure for policy instance data.
 *
 * This contains data that is private to the particular power policy that is active.
 */
typedef struct mtk_kbasep_pm_ca_policy_random {
	/** No state needed - just have a dummy variable here */
	int dummy;
} mtk_kbasep_pm_ca_policy_random;

#endif /* MTK_KBASE_PM_CA_RANDOM_H */

