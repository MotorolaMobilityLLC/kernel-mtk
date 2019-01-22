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

#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/slab.h>

#include "l2c-parity.h"
#include "plat-cfg.h"

static void __iomem *mcusys_base;

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

static struct platform_driver l2c_parity_drv = {
	.driver = {
		.name = "l2c_parity",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
};

static ssize_t show_l2c_parity(struct device_driver *driver, char *buf)
{
	unsigned int i;
	unsigned long ret;
	char *wp = buf;

	if (!mcusys_base) {
		pr_err("%s:%d: mcusys_base == 0x0\n", __func__, __LINE__);
		return snprintf(buf, PAGE_SIZE, "mcusys_base == 0x0\n");
	}

	for (i = 0; i <= cfg_l2_parity.nr_little_cluster-1; ++i) {
		ret = readl(mcusys_base + cfg_l2_parity.cluster[i].l2_cache_parity1_rdata);
		if (ret & 0x1) {
			/* get parity error in mp0 */
			wp += snprintf(wp, PAGE_SIZE, "[L2C parity] get parity error in mp%d\n",
					cfg_l2_parity.cluster[i].mp_id);
			wp += snprintf(wp, PAGE_SIZE, "error count = 0x%x\n", extract_n2mbits(ret, 8, 15));
			ret = readl(mcusys_base + cfg_l2_parity.cluster[i].l2_cache_parity2_rdata);
			wp += snprintf(wp, PAGE_SIZE, "index = 0x%x\n", extract_n2mbits(ret, 0, 14));
			wp += snprintf(wp, PAGE_SIZE, "bank = 0x%x\n", extract_n2mbits(ret, 16, 31));

			/* XXX: uncomment the below line to clear the parity error each time  */
			/* writel(0x0, mcusys_base + cfg_l2_parity.cluster[i].l2_cache_parity1_rdata); */
		}
	}

	return strlen(buf);
}

DRIVER_ATTR(l2c_parity_dump, 0444, show_l2c_parity, NULL);


static int __init l2c_parity_init(void)
{
	struct device_node *dev_node;
	unsigned int i;
	int ret;

	dev_node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg");
	if (dev_node) {
		mcusys_base = of_iomap(dev_node, 0);

		if (mcusys_base) {
			/* clear mcusys parity check registers */
			for (i = 0; i <= cfg_l2_parity.nr_little_cluster-1; ++i)
				writel(0x0, mcusys_base + cfg_l2_parity.cluster[i].l2_cache_parity1_rdata);
		}
	} else {
		pr_err("cannot find node \"mediatek,mcucfg\" for l2c parity\n");
		return -ENODEV;
	}

	ret = platform_driver_register(&l2c_parity_drv);
	if (ret) {
		pr_err("%s:%d: platform_driver_register failed\n", __func__, __LINE__);
		return -ENODEV;
	}

	ret = driver_create_file(&l2c_parity_drv.driver, &driver_attr_l2c_parity_dump);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	return 0;
}

module_init(l2c_parity_init);
