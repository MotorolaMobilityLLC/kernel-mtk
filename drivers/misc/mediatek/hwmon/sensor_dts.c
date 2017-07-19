/*
 * Copyright (C) 2011-2014 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/of.h>
#include <linux/of_irq.h>
#ifdef CONFIG_CUSTOM_KERNEL_ALSPS
#include <cust_alsps.h>
#endif

#define DEBUG_ON 1
#if DEBUG_ON
#define SENSOR_TAG				  "[Sensor dts] "
#define SENSOR_ERR(fmt, args...)	pr_err(SENSOR_TAG fmt, ##args)
#define SENSOR_LOG(fmt, args...)	pr_debug(SENSOR_TAG fmt, ##args)
#else
#define SENSOR_ERR(fmt, args...)
#define SENSOR_LOG(fmt, args...)
#endif

#ifdef CONFIG_CUSTOM_KERNEL_ALSPS
struct alsps_hw *get_alsps_dts_func(struct device_node *node, struct alsps_hw *hw)
{
	int32_t i, ret;
	u32 device_id[] = {0};
	u32 power_id[] = {0};
	u32 power_vol[] = {0};
	u32 polling_mode_ps[] = {0};
	u32 polling_mode_als[] = {0};
	u32 als_level[C_CUST_ALS_LEVEL-1] = {0};
	u32 als_value[C_CUST_ALS_LEVEL] = {0};
	u32 ps_threshold_high[] = {0};
	u32 ps_threshold_low[] = {0};
	u32 is_batch_supported_ps[] = {0};
	u32 is_batch_supported_als[] = {0};

	SENSOR_LOG("Device Tree get alsps info!\n");
	if (node == NULL) {
		ret = -1;
	} else {
		ret = of_property_read_u32_array(node, "device_id", device_id, ARRAY_SIZE(device_id));
		if (ret == 0)
			hw->device_id = device_id[0];

		ret += of_property_read_u32_array(node, "power_id", power_id, ARRAY_SIZE(power_id));
		if (ret == 0) {
			if (power_id[0] == 0xffffU)
				hw->power_id = -1;
			else
				hw->power_id	=	(int)power_id[0];
		}

		ret += of_property_read_u32_array(node, "power_vol", power_vol, ARRAY_SIZE(power_vol));
		if (ret == 0)
			hw->power_vol	=	(int)power_vol[0];

		ret += of_property_read_u32_array(node, "als_level", als_level, ARRAY_SIZE(als_level));
		if (ret == 0) {
			for (i = 0; i < (int32_t)ARRAY_SIZE(als_level); i++)
				hw->als_level[i]		 = als_level[i];
		}

		ret += of_property_read_u32_array(node, "als_value", als_value, ARRAY_SIZE(als_value));
		if (ret == 0) {
			for (i = 0; i < (int32_t)ARRAY_SIZE(als_value); i++)
				hw->als_value[i]		 = als_value[i];
		}

		ret += of_property_read_u32_array(node, "polling_mode_ps",
					polling_mode_ps, ARRAY_SIZE(polling_mode_ps));
		if (ret == 0)
			hw->polling_mode_ps		 = (int)polling_mode_ps[0];

		ret += of_property_read_u32_array(node, "polling_mode_als",
					polling_mode_als, ARRAY_SIZE(polling_mode_als));
		if (ret == 0)
			hw->polling_mode_als		 = (int)polling_mode_als[0];

		ret += of_property_read_u32_array(node, "ps_threshold_high",
					ps_threshold_high, ARRAY_SIZE(ps_threshold_high));
		if (ret == 0)
			hw->ps_threshold_high		 = ps_threshold_high[0];

		ret += of_property_read_u32_array(node, "ps_threshold_low",
					ps_threshold_low, ARRAY_SIZE(ps_threshold_low));
		if (ret == 0)
			hw->ps_threshold_low		 = ps_threshold_low[0];

		ret += of_property_read_u32_array(node, "is_batch_supported_ps", is_batch_supported_ps,
			ARRAY_SIZE(is_batch_supported_ps));
		if (ret == 0)
			hw->is_batch_supported_ps		 = (bool)is_batch_supported_ps[0];

		ret += of_property_read_u32_array(node, "is_batch_supported_als", is_batch_supported_als,
			ARRAY_SIZE(is_batch_supported_als));
		if (ret == 0)
			hw->is_batch_supported_als		 = (bool)is_batch_supported_als[0];
	}
	return (ret == 0) ? hw:NULL;
}
#endif
