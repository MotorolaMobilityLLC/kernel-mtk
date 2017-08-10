/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/of.h>
#include <linux/of_address.h>

#include <mt-plat/upmu_common.h>

#include "mt_spm.h"
#include "mt_spm_internal.h"
#include "mt_spm_pmic_wrap.h"

void spm_dpidle_pre_process(void)
{
#if 0
	spm_pmic_power_mode(PMIC_PWR_DEEPIDLE, 0, 0);

	/* set PMIC WRAP table for deepidle power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_DEEPIDLE);
#endif
}

void spm_dpidle_post_process(void)
{
#if 0
	/* set PMIC WRAP table for normal power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);
#endif
}

#if 0
static int __init get_base_from_node(
	const char *cmp, void __iomem **pbase, int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		pr_err("node '%s' not found!\n", cmp);
		BUG();
	}

	*pbase = of_iomap(node, idx);
	if (!(*pbase)) {
		pr_err("node '%s' cannot iomap!\n", cmp);
		BUG();
	}

	return 0;
}
#endif

void spm_deepidle_chip_init(void)
{
}

