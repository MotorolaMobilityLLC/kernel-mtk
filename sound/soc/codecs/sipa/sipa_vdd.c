/*
 * Copyright (C) 2022, SI-IN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define DEBUG
#define LOG_FLAG    "sipa_vdd"

#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include <linux/time.h>

#include "sipa_vdd.h"
#include "sipa_common.h"
#include "sipa_regmap.h"
#include "sipa_parameter.h"

static void sipa_vol_get_work(struct work_struct *work)
{
	sipa_dev_t *si_pa = container_of(work, sipa_dev_t, vol_get_work.work);
	struct power_supply *psy;
	union power_supply_propval val;
	uint32_t vdd_val;

    if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is null \r\n",
				LOG_FLAG, __func__);
		return;
	}

	psy = power_supply_get_by_name("battery");

	if (psy == NULL) {
		pr_err("[  err][%s] %s: channel = %d, Failed to read psy\r\n",
			   LOG_FLAG, __func__, si_pa->channel_num);
		return;
	}

	if (power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val)) {
		pr_err("[  err][%s] %s: channel = %d, Failed to read voltage\r\n",
			   LOG_FLAG, __func__, si_pa->channel_num);
		goto vol_get_err;
	}

	vdd_val = val.intval;
	pr_info("[debug][%s] %s: channel = %d, Voltage: %d μV\n", LOG_FLAG, __func__, si_pa->channel_num, vdd_val);
	power_supply_put(psy);
	sipa_regmap_set_pvdd_limit(si_pa->regmap, si_pa->chip_type,
							   si_pa->channel_num, vdd_val);
	return;
vol_get_err:
	power_supply_put(psy);
	return;

}

static void sipa_vol_get_callback(struct timer_list *t)
{
	sipa_dev_t *si_pa = from_timer(si_pa, t, vol_get_timer);

	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is null \r\n",
				LOG_FLAG, __func__);
		return ;
	}

	if (!queue_delayed_work(si_pa->sipa_wq, &si_pa->vol_get_work, 0)) {
		pr_debug("[debug][%s] %s: channel = %d, delayed_work err\r\n",
				 LOG_FLAG, __func__, si_pa->channel_num);
	}

    mod_timer(t, jiffies + HZ * si_pa->en_dyn_ud_time_s);
}

void sipa_vol_get(sipa_dev_t *si_pa)
{
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is null \r\n",
				LOG_FLAG, __func__);
		return ;
	}

	if (si_pa->chip_type == CHIP_TYPE_SIA8168) {
		if(!sipa_regmap_get_pvdd_en(si_pa)){
            pr_debug("[debug][%s] %s: channel = %d, sipa_regmap_get_pvdd_en err\r\n",
					 LOG_FLAG, __func__, si_pa->channel_num);
            return;
        }

		if (si_pa->en_dyn_ud_pvdd != 0) {
			pr_debug("[debug][%s] %s: channel = %d, chip 8168 timer setup\r\n",
					 LOG_FLAG, __func__, si_pa->channel_num);
			INIT_DELAYED_WORK(&si_pa->vol_get_work, sipa_vol_get_work);
			timer_setup(&si_pa->vol_get_timer, sipa_vol_get_callback, 0);
			mod_timer(&si_pa->vol_get_timer, jiffies + HZ / SIPA_VOL_GET_START_TIME);
		}

	}
}

void sipa_vol_get_release(sipa_dev_t *si_pa)
{
	if (NULL == si_pa) {
		pr_err("[  err][%s] %s: si_pa is null \r\n",
				LOG_FLAG, __func__);
		return ;
	}

	if (si_pa->chip_type == CHIP_TYPE_SIA8168) {
		if (si_pa->en_dyn_ud_pvdd != 0) {
			del_timer(&si_pa->vol_get_timer);
			cancel_delayed_work_sync(&si_pa->vol_get_work);
		}
	}
}
