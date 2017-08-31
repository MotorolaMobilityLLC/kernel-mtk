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

#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <mt-plat/mtk_secure_api.h>
#include "iccs.h"

static struct iccs_governor *governor;
static struct iccs_cluster_info *cluster_info;

/*
 * Parameters:
 * @target_power_state_bitmask:
 * the target power state by hps (1:power on,0:power off),
 * and also the output of target power state after iccs governor
 * @target_power_state_bitmask:
 * the output of target cache shared state after iccs governor
 *
 * Return value:
 * non-zero return value for error indication
 */
int iccs_get_target_state(unsigned char *target_power_state_bitmask,
		unsigned char *target_cache_shared_state_bitmask)
{
	unsigned int i;
	unsigned char hps_target_power_state;
	unsigned char cache_shared_useful, iccs_cache_shared_useful_bitmask;

	if (!target_power_state_bitmask || !target_cache_shared_state_bitmask)
		return -EINVAL;

	if (!governor || !cluster_info || (governor->enabled == 0))
		return -EINVAL;

	/* clear all the cache shared state to disabled */
	*target_cache_shared_state_bitmask = 0;

	/* get the bitmask before state transition on each cluster */
	iccs_cache_shared_useful_bitmask = governor->iccs_cache_shared_useful_bitmask;

	/* iterate on each cluster */
	for (i = 0; i <= governor->nr_cluster-1; ++i) {
		if (!cluster_info[i].cache_shared_supported)
			continue;

		/* get the target power state and check whether cache shared should be enabled now */
		hps_target_power_state = (*target_power_state_bitmask & (1 << i)) >> i;
		cache_shared_useful = (iccs_cache_shared_useful_bitmask & (1 << i)) >> i;

		/* state transition */
		switch (cluster_info[i].state) {
		case POWER_OFF_CACHE_SHARED_DISABLED:
			if (hps_target_power_state == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			else if (hps_target_power_state == 0x0 && cache_shared_useful == 0x0)
				cluster_info[i].state = POWER_OFF_CACHE_SHARED_DISABLED;
			else if (hps_target_power_state == 0x0 && cache_shared_useful == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_ENABLED;
			break;
		case POWER_ON_CACHE_SHARED_DISABLED:
			if (hps_target_power_state == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			else
				cluster_info[i].state = POWER_ON_CACHE_SHARED_ENABLED;
			break;
		case POWER_ON_CACHE_SHARED_ENABLED:
			if (hps_target_power_state == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			else if (hps_target_power_state == 0x0 && cache_shared_useful == 0x0)
				cluster_info[i].state = POWER_OFF_CACHE_SHARED_DISABLED;
			else if (hps_target_power_state == 0x0 && cache_shared_useful == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_ENABLED;
			break;
		case UNINITIALIZED:
			if (hps_target_power_state == 0x0)
				cluster_info[i].state = POWER_OFF_CACHE_SHARED_DISABLED;
			else
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			break;
		default:
			break;
		}

		/*
		 * only update the target_power_state and cache_shraing_state
		 * when the target state is POWER_ON_CACHE_SHARED_ENABLED
		 */
		if (cluster_info[i].state == POWER_ON_CACHE_SHARED_ENABLED) {
			*target_power_state_bitmask |= 1 << i;
			*target_cache_shared_state_bitmask |= 1 << i;
		}
	}

	/* update the target cache shared state in ATF */
	mt_secure_call(MTK_SIP_KERNEL_ICCS_STATE, ICCS_SET_TARGET_STATE, *target_cache_shared_state_bitmask, 0);

	return 0;
}

unsigned int iccs_is_cache_shared_enabled(unsigned int cluster_id)
{
	int ret;

	if (!governor || !cluster_info || (governor->enabled == 0) || (cluster_id >= governor->nr_cluster))
		return 0;

	/* get HW status from secure world */
	ret = mt_secure_call(MTK_SIP_KERNEL_ICCS_STATE, ICCS_GET_CURR_STATE, 0, 0);

	if (ret & (1 << cluster_id))
		return 1;
	else
		return 0;
}

/*
 * iccs governor suspend/resume flow:
 *	state 1) when governor->enabled == 0, governor->enabled_before_suspend == 0:
 *		-> do nothing in suspend/resume functions
 *		(goto state 1 for suspend, goto state 1 for resume)
 *
 *	state 2) when governor->enabled == 0, governor->enabled_before_suspend == 1:
 *		-> do nothing in suspend function, enable governor in resume function
 *		(goto state 2 for suspend, goto state 3 for resume)
 *
 *	state 3) when governor->enabled == 1, governor->enabled_before_suspend == 0:
 *		-> disable governor in suspend function, do nothing in resume function
 *		(goto state 2 for suspend, goto state 3 for resume)
 *
 *	state 4) when governor->enabled == 1, governor->enabled_before_suspend == 1:
 *		-> disable governor in suspend function, do nothing in resume function
 *		(this state does not exist)
 */
int iccs_governor_suspend(void)
{
	if (!governor || !cluster_info)
		return -EINVAL;

	if (governor->enabled == 0)
		return 0;

	if (likely(&governor->hr_timer))
		hrtimer_cancel(&governor->hr_timer);

	spin_lock(&governor->spinlock);
	governor->enabled = 0;
	governor->enabled_before_suspend = 1;
	spin_unlock(&governor->spinlock);

	return 0;
}

int iccs_governor_resume(void)
{
	unsigned int i;

	if (!governor || !cluster_info)
		return -EINVAL;

	if (governor->enabled_before_suspend == 0)
		return 0;

	spin_lock(&governor->spinlock);
	/* reset to UNINITIALIZED state after system resumes */
	for (i = 0; i <= governor->nr_cluster-1; ++i)
		cluster_info[i].state = UNINITIALIZED;
	governor->enabled = 1;
	governor->enabled_before_suspend = 0;
	spin_unlock(&governor->spinlock);

	if (likely(&governor->hr_timer))
		hrtimer_start(&governor->hr_timer, governor->sampling, HRTIMER_MODE_REL);

	return 0;

}

static int iccs_governor_suspend_cb(struct device *dev)
{
	return iccs_governor_suspend();
}

static int iccs_governor_resume_cb(struct device *dev)
{
	return iccs_governor_resume();
}

static SIMPLE_DEV_PM_OPS(iccs_governor_pm_ops, iccs_governor_suspend_cb,
			iccs_governor_resume_cb);

static int iccs_governor_probe(struct platform_device *pdev)
{
	struct device_node *dev_node = pdev->dev.of_node;

	unsigned int val, i;
	const __be32 *spec;
	int ret;

	governor = kzalloc(sizeof(struct iccs_governor), GFP_KERNEL);
	if (unlikely(!governor))
		return -ENOMEM;

	if (dev_node) {

		if (of_property_read_u32(dev_node, "mediatek,enabled", &val))
			governor->enabled = 0;
		else
			governor->enabled = val & 1;

		/* always initialize to 0 */
		governor->enabled_before_suspend = 0;

		if (of_property_read_u32(dev_node, "mediatek,nr_cluster", &val)) {
			pr_err("cannot find node \"mediatek,nr_cluster\" for iccs_governor\n");
			ret = -ENODEV;
			goto err;
		} else {
			governor->nr_cluster = val;
			cluster_info = kcalloc(val, sizeof(struct iccs_cluster_info), GFP_KERNEL);
			if (!cluster_info) {
				ret = -ENOMEM;
				goto err;
			}
			spec = of_get_property(dev_node, "mediatek,cluster_cache_shared", &val);
			if (spec == NULL) {
				ret = -EINVAL;
				goto err;
			}

			val /= sizeof(*spec);
			for (i = 0; i <= governor->nr_cluster-1; ++i) {
				cluster_info[i].cache_shared_supported = be32_to_cpup(spec + i) & 0x1;
				cluster_info[i].state = UNINITIALIZED;
			}
		}

		if (of_property_read_u32(dev_node, "mediatek,sampling_rate", &val)) {
			pr_err("cannot find node \"mediatek,sampling_rate\" for iccs_governor\n");
			ret = -ENODEV;
			goto err;
		} else {
			governor->sampling = ktime_set(0, MS_TO_NS((unsigned long)val));
		}

		if (of_property_read_u32(dev_node, "mediatek,policy", &val)) {
			pr_err("cannot find node \"mediatek,policy\" for iccs_governor\n");
			ret = -ENODEV;
			goto err;
		} else {
			governor->policy = val;
		}

		if (of_property_read_u32(dev_node, "mediatek,shared_cluster_freq", &val)) {
			pr_err("cannot find node \"mediatek,shared_cluster_freq\" for iccs_governor\n");
			ret = -ENODEV;
			goto err;
		} else {
			governor->shared_cluster_freq = val;
		}

	} else {
		pr_err("cannot find node \"mediatek,iccs_governor\" for iccs_governor\n");
		ret = -ENODEV;
		goto err;
	}

	spin_lock_init(&governor->spinlock);

	/* Initialize a kthread & hr_timer*/
	ret = governor_activation(governor);
	if (ret < 0) {
		pr_err("Init ICCS Governor thread failed\n");
		goto err;
	}

	return 0;

err:
	kfree(governor);
	kfree(cluster_info);
	governor = NULL;
	cluster_info = NULL;
	return ret;
}

/* An interface for user to change transition*/
static ssize_t iccs_policy_show(struct device_driver *driver, char *buf)
{
	if (unlikely(!governor))
		return snprintf(buf, PAGE_SIZE, "ERR: governor has not been initialized\n");

	if (governor->policy == PERFORMANCE)
		return snprintf(buf, PAGE_SIZE, "performance\n");
	else if (governor->policy == ONDEMAND)
		return snprintf(buf, PAGE_SIZE, "ondemand\n");
	else if (governor->policy == POWERSAVE)
		return snprintf(buf, PAGE_SIZE, "powersave\n");
	else if (governor->policy == BENCHMARK)
		return snprintf(buf, PAGE_SIZE, "benchmark\n");

	return snprintf(buf, PAGE_SIZE, "ERR: invalid policy\n");
}
static ssize_t iccs_policy_store(struct device_driver *driver,
					const char *buf, size_t count)
{
	int ret;
	char str_governor[ICCS_GOV_NAME_LEN];
	enum transition new_policy;

	ret = sscanf(buf, "%15s", str_governor);
	if (ret != 1)
		return -EINVAL;

	if (strncasecmp(str_governor, "performance", ICCS_GOV_NAME_LEN) == 0)
		new_policy = PERFORMANCE;
	else if (strncasecmp(str_governor, "ondemand", ICCS_GOV_NAME_LEN) == 0)
		new_policy = ONDEMAND;
	else if (strncasecmp(str_governor, "powersave", ICCS_GOV_NAME_LEN) == 0)
		new_policy = POWERSAVE;
	else if (strncasecmp(str_governor, "benchmark", ICCS_GOV_NAME_LEN) == 0)
		new_policy = BENCHMARK;
	else
		return -EINVAL;

	policy_update(governor, new_policy);

	return count;
}

DRIVER_ATTR(iccs_policy, 0664, iccs_policy_show, iccs_policy_store);

static ssize_t iccs_smapling_show(struct device_driver *driver, char *buf)
{
	if (unlikely(!governor))
		return snprintf(buf, PAGE_SIZE, "ERR: governor has not been initialized\n");

	return snprintf(buf, PAGE_SIZE, "%lldms\n", ktime_to_ms(governor->sampling));
}

static ssize_t iccs_sampling_store(struct device_driver *driver,
					const char *buf, size_t count)
{
	unsigned long long rate;

	if (kstrtou64(buf, 0, &rate) < 0)
		return -EINVAL;

	sampling_update(governor, rate);

	return count;
}

DRIVER_ATTR(iccs_sampling_rate, 0664, iccs_smapling_show, iccs_sampling_store);

static const struct of_device_id iccs_governor_match[] = {
	{ .compatible = "mediatek,iccs_governor" },
	{},
};

static struct platform_driver iccs_governor_driver = {
	.probe		= iccs_governor_probe,
	.driver		= {
		.name		= "iccs_governor",
		.of_match_table	= iccs_governor_match,
		.pm		= &iccs_governor_pm_ops,
	}
};

static int __init iccs_governor_init(void)
{
	int ret;

	ret = platform_driver_register(&iccs_governor_driver);
	if (ret)
		pr_err("iccs_governor driver register failed %d\n", ret);

	ret = driver_create_file(&iccs_governor_driver.driver,
			&driver_attr_iccs_policy);

	ret = driver_create_file(&iccs_governor_driver.driver,
			&driver_attr_iccs_sampling_rate);

	return ret;
}

core_initcall(iccs_governor_init);
