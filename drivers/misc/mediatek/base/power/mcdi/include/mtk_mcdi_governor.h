/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __MTK_MCDI_GOVERNOR_H__
#define __MTK_MCDI_GOVERNOR_H__

int mcdi_governor_select(int cpu);
void mcdi_governor_reflect(int cpu, int state);
void mcdi_avail_cpu_cluster_update(void);
void mcdi_governor_init(void);

#endif /* __MTK_MCDI_GOVERNOR_H__ */
