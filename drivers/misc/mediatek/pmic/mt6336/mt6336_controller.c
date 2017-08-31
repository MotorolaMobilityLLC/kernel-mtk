/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include "mt6336.h"


#define MT6336_CTRL_READY 1
unsigned int g_mt6336_use_count;

unsigned int mt6336_max_users;
struct mt6336_ctrl *mt6336_ctrl_users[100];
static DEFINE_MUTEX(mt6336_ctrl_mutex);

struct mt6336_ctrl *mt6336_ctrl_get(const char *name)
{
	struct mt6336_ctrl *mt6336_ctrl;

	mt6336_ctrl = kzalloc(sizeof(struct mt6336_ctrl), GFP_KERNEL);
	if (mt6336_ctrl == NULL)
		return NULL;

	mutex_lock(&mt6336_ctrl_mutex);
	mt6336_ctrl_users[mt6336_max_users] = mt6336_ctrl;
	mt6336_ctrl->name = kstrdup_const(name, GFP_KERNEL);
	mt6336_ctrl->id = mt6336_max_users;
	mt6336_max_users++;
	mutex_unlock(&mt6336_ctrl_mutex);
	return mt6336_ctrl;
}

#if MT6336_CTRL_READY
unsigned int mt6336_ctrl_enable(struct mt6336_ctrl *ctrl)
{
	unsigned int ret = 0;

	mutex_lock(&mt6336_ctrl_mutex);
	if (g_mt6336_use_count == 0) {
		if (mt6336_get_flag_register_value(MT6336_RG_DIS_LOWQ_MODE) == 1)
			pr_err(MT6336TAG "[%s] Warning! someone may enable MT6336 by hard-code!!\n", __func__);
		else
			mt6336_set_flag_register_value(MT6336_RG_DIS_LOWQ_MODE, 0x1);
		MT6336LOG("[%s] MT6336 controller enabled by %s\n", __func__, ctrl->name);
	}
	ctrl->state++;
	g_mt6336_use_count++;
	mutex_unlock(&mt6336_ctrl_mutex);

	return ret;
}

unsigned int mt6336_ctrl_disable(struct mt6336_ctrl *ctrl)
{
	unsigned int ret = 0;

	mutex_lock(&mt6336_ctrl_mutex);
	if (g_mt6336_use_count < 1) {
		pr_err(MT6336TAG "[%s] Error! MT6336 has never be enabled and it can not be disabled by %s!\n",
			__func__, ctrl->name);
		ret = 1;
		goto out;
	}
	/* last user, need to disable MT6336 control by enter lowQ mode */
	if (g_mt6336_use_count == 1) {
		if (mt6336_get_flag_register_value(MT6336_RG_DIS_LOWQ_MODE) == 0)
			pr_err(MT6336TAG "[%s] Warning! someone may disable MT6336 by hard-code!!\n", __func__);
		if (ctrl->state < 1) {
			pr_err(MT6336TAG "[%s] Error! %s has not enabled MT6336 and it should not disable MT6336!\n",
				__func__, ctrl->name);
			ret = 2;
			goto out;
		} else {
			mt6336_set_flag_register_value(MT6336_RG_DIS_LOWQ_MODE, 0x0);
			MT6336LOG("[%s] MT6336 controller disabled by %s\n", __func__, ctrl->name);
		}
		ctrl->state = 0;
		g_mt6336_use_count = 0;
	} else if (g_mt6336_use_count > 1) {
		ctrl->state--;
		g_mt6336_use_count--;
	}
out:
	mutex_unlock(&mt6336_ctrl_mutex);
	return ret;
}
#else
unsigned int mt6336_ctrl_enable(struct mt6336_ctrl *ctrl)
{
	unsigned int ret = 0;

	mutex_lock(&mt6336_ctrl_mutex);
	ctrl->state++;
	pr_err("[%s] MT6336 controller enabled by %s\n", __func__, ctrl->name);
	mutex_unlock(&mt6336_ctrl_mutex);
	return ret;
}

unsigned int mt6336_ctrl_disable(struct mt6336_ctrl *ctrl)
{
	unsigned int ret = 0;

	mutex_lock(&mt6336_ctrl_mutex);
	ctrl->state--;
	pr_err("[%s] MT6336 controller disabled by %s\n", __func__, ctrl->name);
	mutex_unlock(&mt6336_ctrl_mutex);
	return ret;
}
#endif

