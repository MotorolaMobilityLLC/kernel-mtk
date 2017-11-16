/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/io.h>

#include "../lastpc.h"

struct lastpc_imp {
	struct lastpc_plt plt;
	void __iomem *toprgu_reg;
};

static int lastpc_plt_start(struct lastpc_plt *plt)
{
	return 0;
}

static int lastpc_plt_dump(struct lastpc_plt *plt, char *buf, int len)
{
	void __iomem *mcu_base = plt->common->base + 0x300;
	int ret = -1, cnt = num_possible_cpus();
	char *ptr = buf;
	unsigned long pc_value;
	unsigned long fp_value;
	unsigned long sp_value;
	unsigned long size = 0;
	unsigned long offset = 0;
	char str[KSYM_SYMBOL_LEN];
	int i;

	if (cnt < 0)
		return ret;

	/* Get PC, FP, SP and save to buf */
	for (i = 0; i < cnt; i++) {
		pc_value = readl(IOMEM(mcu_base+(i*0x10)));
		fp_value = readl(IOMEM(mcu_base+0x4+(i*0x10)));
		sp_value = readl(IOMEM(mcu_base+0x8+(i*0x10)));
		kallsyms_lookup((unsigned long)pc_value, &size, &offset,
				NULL, str);
		ptr += sprintf(ptr,
		"[LAST PC] CORE_%d PC = 0x%lx(%s + 0x%lx), FP = 0x%lx, SP = 0x%lx\n",
		i, pc_value, str, offset, fp_value, sp_value);
		pr_notice("[LAST PC] CORE_%d PC = 0x%lx(%s), FP = 0x%lx, SP = 0x%lx\n",
		i, pc_value, str, fp_value, sp_value);
	}

	return 0;
}

static int reboot_test(struct lastpc_plt *plt)
{
	return 0;
}

static struct lastpc_plt_operations lastpc_ops = {
	.start = lastpc_plt_start,
	.dump  = lastpc_plt_dump,
	.reboot_test = reboot_test,
};

static int __init lastpc_init(void)
{
	struct lastpc_imp *drv = NULL;
	int ret = 0;

	drv = kzalloc(sizeof(struct lastpc_imp), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	drv->plt.ops = &lastpc_ops;
	drv->plt.chip_code = 0x6580;
	drv->plt.min_buf_len = 2048;

	ret = lastpc_register(&drv->plt);
	if (ret) {
		pr_err("%s:%d: lastpc_register failed\n", __func__, __LINE__);
		goto register_lastpc_err;
	}

	return 0;

register_lastpc_err:
	kfree(drv);
	return ret;
}

arch_initcall(lastpc_init);
