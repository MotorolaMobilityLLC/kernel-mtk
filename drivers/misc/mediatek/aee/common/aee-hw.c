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

#include <linux/kconfig.h>
#include <linux/module.h>
#ifdef CONFIG_MTK_WATCHDOG
#include <mach/wd_api.h>

#ifdef CONFIG_MTK_LASTPC_V2
#include <plat_sram_flag.h>

static void set_sram_lastpc_flag(void)
{
	if (set_sram_flag_lastpc_valid() == 0)
		pr_info("OK: set_sram_flag_lastpc_valid\n");
}

static void wd_mcu_cache_preserve(bool enabled)
{
	int res;
	struct wd_api *wd_api = NULL;

	res = get_wd_api(&wd_api);
	if (res < 0) {
		pr_alert("%s: get wd api error %d\n", __func__, res);
	} else {
		res = wd_api->wd_mcu_cache_preserve(enabled);
		if (res < 0)
			pr_alert("%s: MCU Cache Preserve disable\n", __func__);
		else
			pr_alert("%s: MCU Cache Preserve enable\n", __func__);
	}
}
#endif /* CONFIG_MTK_LASTPC_V2 */

static void wd_dram_reserved_mode(bool enabled)
{
	int res;
	struct wd_api *wd_api = NULL;

	res = get_wd_api(&wd_api);
	if (res < 0) {
		pr_alert("%s: get wd api error %d\n", __func__, res);
	} else {
		res = wd_api->wd_dram_reserved_mode(enabled);
		if (res < 0)
			pr_alert("%s: DDR Reserved Mode disable\n", __func__);
		else
			pr_alert("%s: DDR Reserved Mode enable\n", __func__);
	}
}

static int __init aee_hw_init(void)
{
	bool cache_preserve_ok = false;
	bool ddr_rsv_mode_ok = cache_preserve_ok;
	int condition = 0;

#ifdef CONFIG_MTK_LASTPC_V2
	condition++;
#endif

#ifdef CONFIG_MTK_AEE_DRAM_CONSOLE
	condition++;
#endif

#ifdef CONFIG_MTK_AEE_IPANIC
	condition++;
#endif

#ifdef CONFIG_MTK_AEE_MRDUMP
	condition++;
#endif

	if (condition > 0) {
		ddr_rsv_mode_ok = true;
		cache_preserve_ok = true;
	}

	wd_dram_reserved_mode(ddr_rsv_mode_ok);

#ifdef CONFIG_MTK_LASTPC_V2
	wd_mcu_cache_preserve(cache_preserve_ok);
	set_sram_lastpc_flag();
#endif /* CONFIG_MTK_LASTPC_V2 */

	pr_info("%s: aee_hw_init done.\n", __func__);
	return 0;
}

arch_initcall(aee_hw_init);
#endif /* CONFIG_MTK_WATCHDOG */
