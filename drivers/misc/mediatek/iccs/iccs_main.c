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

#include <linux/module.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "iccs.h"

static struct iccs_governor *governor;
static struct iccs_cluster_info *cluster_info;

#if 0
static unsigned char iccs_check_cache_shared_useful(void)
{
	unsigned int i;
	unsigned char iccs_cache_shared_useful_bitmask = 0;

	if (!governor || !cluster_info || (governor->enabled == 0))
		return 0;


	/* TODO: check each cluster */
	for (i = 0; i <= governor->nr_cluster-1; ++i)
		;

	/* TODO: Update status in ATF*/
	return iccs_cache_shared_useful_bitmask;
}
#endif

/*
 * Parameters:
 * @curr_power_state_bitmask:
 * the current power state of clusters
 * @next_power_state_bitmask:
 * the next power state by hps (1:power on,0:power off),
 * and also the output of next power state after iccs governor
 * @next_power_state_bitmask:
 * the output of next cache sharing state after iccs governor
 *
 * Return value:
 * non-zero return value for error indication
 */
int iccs_get_next_state(const unsigned char curr_power_state_bitmask,
		unsigned char *next_power_state_bitmask,
		unsigned char *next_cache_shared_state_bitmask)
{
	unsigned int i;
	unsigned char hps_next_power_state;
	unsigned char cache_shared_useful;
	/* unsigned char iccs_cache_shared_useful_bitmask; */

	if (!next_power_state_bitmask || !next_cache_shared_state_bitmask)
		return -EINVAL;

	if (!governor || !cluster_info || (governor->enabled == 0))
		return -EINVAL;

	/* clear all the cache sharing state to disabled */
	*next_cache_shared_state_bitmask = 0;

	/* get the information whether cache sharing useful */
	/* iccs_cache_shared_useful_bitmask = iccs_check_cache_shared_useful(); */

	/* iterate on each cluster */
	for (i = 0; i <= governor->nr_cluster-1; ++i) {
		if (!cluster_info[i].cache_shared_supported)
			continue;

		/* get the next power state and check whether cache sharing should be enabled now */
		hps_next_power_state = (*next_power_state_bitmask & (1 << i)) >> i;
		cache_shared_useful = ((governor->iccs_cache_shared_useful_bitmask & (1 << i)) >> i);

		/* state transition */
		switch (cluster_info[i].state) {
		case POWER_OFF_CACHE_SHARED_DISABLED:
			if (hps_next_power_state == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			else if (hps_next_power_state == 0x0 && cache_shared_useful == 0x0)
				cluster_info[i].state = POWER_OFF_CACHE_SHARED_DISABLED;
			else if (hps_next_power_state == 0x0 && cache_shared_useful == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_ENABLED;
			break;
		case POWER_ON_CACHE_SHARED_DISABLED:
			if (hps_next_power_state == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			else
				cluster_info[i].state = POWER_ON_CACHE_SHARED_ENABLED;
			break;
		case POWER_ON_CACHE_SHARED_ENABLED:
			if (hps_next_power_state == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			else if (hps_next_power_state == 0x0 && cache_shared_useful == 0x0)
				cluster_info[i].state = POWER_OFF_CACHE_SHARED_DISABLED;
			else if (hps_next_power_state == 0x0 && cache_shared_useful == 0x1)
				cluster_info[i].state = POWER_ON_CACHE_SHARED_ENABLED;
			break;
		case UNINITIALIZED:
			if (((curr_power_state_bitmask & (1 << i)) >> i) == 0x0)
				cluster_info[i].state = POWER_OFF_CACHE_SHARED_DISABLED;
			else
				cluster_info[i].state = POWER_ON_CACHE_SHARED_DISABLED;
			break;
		default:
			break;
		}

		/*
		 * only update the next_power_state and cache_shraing_state
		 * when the next state is POWER_ON_CACHE_SHARED_ENABLED
		 */
		if (cluster_info[i].state == POWER_ON_CACHE_SHARED_ENABLED) {
			*next_power_state_bitmask |= 1 << i;
			*next_cache_shared_state_bitmask |= 1 << i;
		}
	}

	return 0;
}

unsigned int iccs_is_cache_shared_enabled(unsigned int cluster_id)
{
	if (!governor || !cluster_info || (governor->enabled == 0) || (cluster_id >= governor->nr_cluster))
		return 0;

	/* TODO: check hw status */
	return 0;
}

static int iccs_governor_suspend(struct device *dev)
{
	/* TODO: Should disable ICCS before system gets into suspend */
	return 0;
}

static int iccs_governor_resume(struct device *dev)
{
	/* TODO: Should reset the status after system resumes*/
	return 0;
}

static SIMPLE_DEV_PM_OPS(iccs_governor_pm_ops, iccs_governor_suspend,
			iccs_governor_resume);

static int iccs_governor_probe(struct platform_device *pdev)
{
	struct device_node *dev_node = pdev->dev.of_node;

	unsigned int val, i;
	const __be32 *spec;
	int ret;

	governor = kzalloc(sizeof(struct iccs_governor), GFP_KERNEL);
	if (!governor)
		return -ENOMEM;

	if (dev_node) {

		if (of_property_read_u32(dev_node, "mediatek,enabled", &val))
			governor->enabled = 0;
		else
			governor->enabled = val & 1;

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
			spec = of_get_property(dev_node, "mediatek,cluster_cache_sharing", &val);
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

	if (governor->policy == PERFORMANCE)
		return snprintf(buf, PAGE_SIZE, "performance\n");
	else if (governor->policy == ONDEMAND)
		return snprintf(buf, PAGE_SIZE, "ondemand\n");
	else if (governor->policy == POWERSAVE)
		return snprintf(buf, PAGE_SIZE, "powersave\n");
	else if (governor->policy == BENCHMARK)
		return snprintf(buf, PAGE_SIZE, "benchmark\n");

	return -EINVAL;
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
