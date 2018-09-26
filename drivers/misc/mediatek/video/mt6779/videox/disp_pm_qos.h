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

#ifndef __DISP_PM_QOS_H__
#define __DISP_PM_QOS_H__

#include "layering_rule.h"

void disp_pm_qos_init(void);
void disp_pm_qos_deinit(void);
int disp_pm_qos_request_dvfs(enum HRT_LEVEL hrt);

#endif /* __DISP_PM_QOS_H__ */
