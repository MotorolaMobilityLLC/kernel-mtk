/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include "plat_sram_flag.h"

static void __iomem *sram_base;

static inline unsigned int extract_n2mbits(unsigned int input, unsigned int n, unsigned int m)
{
	/*
	 * 1. ~0 = 1111 1111 1111 1111 1111 1111 1111 1111
	 * 2. ~0 << (m - n + 1) = 1111 1111 1111 1111 1100 0000 0000 0000
	 * // assuming we are extracting 14 bits, the +1 is added for inclusive selection
	 * 3. ~(~0 << (m - n + 1)) = 0000 0000 0000 0000 0011 1111 1111 1111
	 */
	int mask;

	if (n > m) {
		n = n + m;
		m = n - m;
		n = n - m;
	}
	mask = ~(~0 << (m - n + 1));
	return (input >> n) & mask;
}

static int check_sram_base(void)
{
	struct device_node *node;

	if (sram_base)
		return 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,plat_sram_flag");
	if (node) {
		sram_base = of_iomap(node, 0);
		if (sram_base)
			return 0;
	}

	pr_err("%s:%d: sram_base == 0x0\n", __func__, __LINE__);
	return -1;
}

/* return negative integer if fails */
int set_sram_flag_lastpc_valid(void)
{
	if (check_sram_base() < 0)
		return -1;

	writel(readl(sram_base + (PLAT_FLAG0*4))|(1 << OFFSET_LASTPC_VALID), sram_base + (PLAT_FLAG0*4));
	return 0;
}
EXPORT_SYMBOL(set_sram_flag_lastpc_valid);

/* return negative integer if fails */
int set_sram_flag_etb_user(unsigned int etb_id, unsigned int user_id)
{
	if (check_sram_base() < 0)
		return -1;

	if (etb_id >= MAX_ETB_NUM) {
		pr_err("%s:%d: etb_id > MAX_ETB_NUM\n", __func__, __LINE__);
		return -1;
	}

	if (user_id >= MAX_ETB_USER_NUM) {
		pr_err("%s:%d: user_id > MAX_ETB_USER_NUM\n", __func__, __LINE__);
		return -1;
	}

	writel((readl(sram_base + (PLAT_FLAG0*4)) & ~(0x7 << (OFFSET_ETB_0 + etb_id*3)))
			|((user_id & 0x7) << (OFFSET_ETB_0 + etb_id*3)), sram_base + (PLAT_FLAG0*4));
	return 0;
}
EXPORT_SYMBOL(set_sram_flag_etb_user);

/* return negative integer if fails */
int set_sram_flag_dfd_valid(void)
{
	if (check_sram_base() < 0)
		return -1;

	writel(readl(sram_base + (PLAT_FLAG1*4))|(1 << OFFSET_DFD_VALID), sram_base + (PLAT_FLAG1*4));
	return 0;
}
EXPORT_SYMBOL(set_sram_flag_dfd_valid);

static struct platform_driver plat_sram_flag_drv = {
	.driver = {
		.name = "plat_sram_flag",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
};

static ssize_t plat_sram_flag_dump_show(struct device_driver *driver, char *buf)
{
	unsigned int i;
	unsigned long plat_sram_flag0, plat_sram_flag1, plat_sram_flag2;
	char *wp = buf;

	if (!sram_base) {
		pr_err("%s:%d: sram_base == 0x0\n", __func__, __LINE__);
		return snprintf(buf, PAGE_SIZE, "sram_base == 0x0\n");
	}

	plat_sram_flag0 = readl(sram_base + (PLAT_FLAG0*4));
	plat_sram_flag1 = readl(sram_base + (PLAT_FLAG1*4));
	plat_sram_flag2 = readl(sram_base + (PLAT_FLAG2*4));

	wp += snprintf(wp, PAGE_SIZE,
			"plat_sram_flag0 = 0x%lx\nplat_sram_flag1 = 0x%lx\nplat_sram_flag2 = 0x%lx\n",
			plat_sram_flag0, plat_sram_flag1, plat_sram_flag2);

	wp += snprintf(wp, PAGE_SIZE, "\n-------------\n");
	wp += snprintf(wp, PAGE_SIZE, "lastpc_valid = 0x%x\nlastpc_valid_before_reboot = 0x%x\n",
			extract_n2mbits(plat_sram_flag0, OFFSET_LASTPC_VALID, OFFSET_LASTPC_VALID),
			extract_n2mbits(plat_sram_flag0, OFFSET_LASTPC_VALID_BEFORE_REBOOT,
				OFFSET_LASTPC_VALID_BEFORE_REBOOT));

	for (i = 0; i <= MAX_ETB_NUM-1; ++i)
		wp += snprintf(wp, PAGE_SIZE,
			"user_id_of_multi_user_etb_%d = 0x%03x\n",
			i, extract_n2mbits(plat_sram_flag0, OFFSET_ETB_0 + i*3, OFFSET_ETB_0 + i*3 + 2));

	wp += snprintf(wp, PAGE_SIZE, "dfd_valid = 0x%x\ndfd_valid_before_reboot = 0x%x\n",
			extract_n2mbits(plat_sram_flag1, OFFSET_DFD_VALID, OFFSET_DFD_VALID),
			extract_n2mbits(plat_sram_flag1, OFFSET_DFD_VALID_BEFORE_REBOOT,
				OFFSET_DFD_VALID_BEFORE_REBOOT));

	return strlen(buf);
}

DRIVER_ATTR(plat_sram_flag_dump, 0444, plat_sram_flag_dump_show, NULL);

static int __init plat_sram_flag_init(void)
{
	struct device_node *node;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,plat_sram_flag");
	if (node) {
		if (sram_base == 0)
			sram_base = of_iomap(node, 0);
	} else {
		pr_err("can't find compatible node for plat_sram_flag\n");
		return -1;
	}

	ret = platform_driver_register(&plat_sram_flag_drv);
	if (ret) {
		pr_err("%s:%d: platform_driver_register failed\n", __func__, __LINE__);
		return -ENODEV;
	}

	ret = driver_create_file(&plat_sram_flag_drv.driver, &driver_attr_plat_sram_flag_dump);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	return 0;
}

core_initcall(plat_sram_flag_init);
