/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "eas_controller.h"
#include "fbt_cpu_platform.h"

void fbt_set_boost_value(unsigned int base_blc)
{
	update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, base_blc - 1 + 4000);
}

int fbt_is_mips_different(void)
{
	return 1;
}

int fbt_get_L_min_ceiling(void)
{
	return 1400000;
}

int fbt_get_L_cluster_num(void)
{
	return 0;
}

