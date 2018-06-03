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

/*
 *
 * Filename:
 * ---------
 *    mtk_switch_charging.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines functions of Battery charging
 *
 * Author:
 * -------
 * Wy Chuang
 *
 */
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#include <linux/suspend.h>

#include "mtk_switch_charging.h"
#include <mt-plat/charger_class.h>
#include <mt-plat/mtk_charger.h>

static int mtk_switch_charging_plug_in(struct charger_info *info)
{
	return 0;
}

static int mtk_switch_charging_plug_out(struct charger_info *info)
{
	return 0;
}

static int mtk_switch_charging_do_charging(struct charger_info *info, bool en)
{
	pr_err("mtk_switch_charging_do_charging en:%d\n", en);

	return 0;
}

static int mtk_switch_charging_run(struct charger_info *info)
{
	/*struct switch_charging_alg_data *swchgalg = info->algorithm_data;*/
	pr_err("mtk_switch_charging_run [%s]\n", info->primary_chg->props.alias_name);

/* check pe+ ,pe+20 ,pe+30 ,jeita ,sw safety timer */
/* decide input current ,charging current, vbus , cv */
	charger_dev_set_input_current(info->primary_chg, info->input_current_limit);
	charger_dev_set_charging_current(info->primary_chg, info->charging_current_limit);

	charger_dev_dump_registers(info->primary_chg);
	return 0;
}

int mtk_switch_charging_init(struct charger_info *info)
{
	struct switch_charging_alg_data *swch_alg;

	swch_alg = devm_kzalloc(&info->pdev->dev, sizeof(struct switch_charging_alg_data), GFP_KERNEL);
	if (!swch_alg)
		return -ENOMEM;

	info->primary_chg = get_charger_by_name("PrimarySWCHG");
	if (info->primary_chg)
		pr_err("Found primary charger [%s]\n", info->primary_chg->props.alias_name);
	else
		pr_err("*** Error : can't find primary charger [%s]***\n", "PrimarySWCHG");

	info->algorithm_data = swch_alg;
	info->do_algorithm = mtk_switch_charging_run;
	info->plug_in = mtk_switch_charging_plug_in;
	info->plug_out = mtk_switch_charging_plug_out;
	info->do_charging = mtk_switch_charging_do_charging;

	return 0;
}




