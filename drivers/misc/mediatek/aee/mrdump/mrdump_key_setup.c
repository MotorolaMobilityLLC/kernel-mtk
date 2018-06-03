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
#ifdef CONFIG_OF
#include <linux/of.h>
#endif
#ifdef CONFIG_MTK_WD_KICKER
#include <mach/wd_api.h>

enum MRDUMP_RST_SOURCE {
	MRDUMP_SYSRST,
	MRDUMP_EINT};
enum MRDUMP_NOTIFY_MODE {
	MRDUMP_IRQ,
	MRDUMP_RST};

static int __init mrdump_key_init(void)
{
	int res;
	struct wd_api *wd_api = NULL;
	enum MRDUMP_RST_SOURCE source = MRDUMP_EINT;
	enum MRDUMP_NOTIFY_MODE mode = MRDUMP_IRQ;
	struct device_node *node;
	const char *source_str, *mode_str;
	char node_name[] = "mediatek, mrdump_ext_rst-eint";

	node = of_find_compatible_node(NULL, NULL, node_name);
	if (node) {
		if (!of_property_read_string(node, "source", &source_str)) {
			if (strcmp(source_str, "SYSRST") == 0)
				source = MRDUMP_SYSRST;
		} else {
			pr_notice("MRDUMP_KEY: no source property, use legacy value EINT");
		}

		if (!of_property_read_string(node, "mode", &mode_str)) {
			if (strcmp(mode_str, "RST") == 0)
				mode = MRDUMP_RST;
		} else {
			pr_notice("MRDUMP_KEY: no mode property, use legacy value IRQ");
		}
	} else {
		pr_notice("%s:MRDUMP_KEY node %s is not exist\n"
			"MRDUMP_KEY is disabled\n", __func__, node_name);
		goto out;
	}

	res = get_wd_api(&wd_api);
	if (res < 0) {
		pr_notice("%s: get_wd_api failed:%d\n"
				"disable MRDUMP_KEY\n", __func__, res);
	} else {
		if (source == MRDUMP_SYSRST) {
			res = wd_api->wd_debug_key_sysrst_config(1, mode);
			if (res == -1)
				pr_notice("%s: sysrst failed\n", __func__);
			else
				pr_notice("%s: enable MRDUMP_KEY\n", __func__);
		} else if (source == MRDUMP_EINT) {
			res = wd_api->wd_debug_key_eint_config(1, mode);
			if (res == -1)
				pr_notice("%s: eint failed\n", __func__);
			else
				pr_notice("%s: enabled MRDUMP_KEY\n", __func__);
		} else {
			pr_notice("%s:source %d is not match\n"
				"disable MRDUMP_KEY\n", __func__, source);
		}
	}
out:
	of_node_put(node);
	return 0;
}

module_init(mrdump_key_init);
#endif


