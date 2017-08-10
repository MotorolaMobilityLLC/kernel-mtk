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
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <asm/io.h>
#include <mt-plat/mt_io.h>

#include "../lastpc.h"

struct lastpc_imp {
	struct lastpc_plt plt;
	void __iomem *toprgu_reg;
	int is_from_loader;
	u64 pc[10];
	u64 fp[10];
	u64 sp[10];
};

#define to_lastpc_imp(p)	container_of((p), struct lastpc_imp, plt)

static char *pc_name[10] = {
	"core0,pc",
	"core1,pc",
	"core2,pc",
	"core3,pc",
	"core4,pc",
	"core5,pc",
	"core6,pc",
	"core7,pc",
	"core8,pc",
	"core9,pc",
};

static char *fp_name[10] = {
	"core0,fp",
	"core1,fp",
	"core2,fp",
	"core3,fp",
	"core4,fp",
	"core5,fp",
	"core6,fp",
	"core7,fp",
	"core8,fp",
	"core9,fp",
};

static char *sp_name[10] = {
	"core0,sp",
	"core1,sp",
	"core2,sp",
	"core3,sp",
	"core4,sp",
	"core5,sp",
	"core6,sp",
	"core7,sp",
	"core8,sp",
	"core9,sp",
};
static int __init set_lastpc_from_loader(struct lastpc_imp *drv,
					unsigned long node)
{
	const void *prop;
	int total_cpu = num_possible_cpus();
	int i = 0;

	for (i = 0; i < total_cpu ; ++i) {
		prop = of_get_flat_dt_prop(node, pc_name[i], NULL);
		if (!prop)
			return -1;
		drv->pc[i] = of_read_number(prop, 2);

		prop = of_get_flat_dt_prop(node, fp_name[i], NULL);
		if (!prop)
			return -1;
		drv->fp[i] = of_read_number(prop, 2);

		prop = of_get_flat_dt_prop(node, sp_name[i], NULL);
		if (!prop)
			return -1;
		drv->sp[i] = of_read_number(prop, 2);
	}

	return 0;
}

static int __init fdt_get_chosen(unsigned long node,
				const char *uname,
				int depth,
				void *data)
{
	if (depth != 1 ||
		(strcmp(uname, "chosen") != 0 &&
			strcmp(uname, "chosen@0") != 0))
		return 0;

	*(unsigned long *)data = node;
	return 1;
}

static int __init lastpc_plt_start(struct lastpc_plt *plt)
{
	struct lastpc_imp *drv = to_lastpc_imp(plt);
	unsigned long node;
	int ret;

	ret = of_scan_flat_dt(fdt_get_chosen, &node);
	if (node) {
		drv->is_from_loader = 1;
		set_lastpc_from_loader(drv, node);
	} else {
		drv->is_from_loader = 0;
		pr_err("[LASTPC] sadly, lastpc might be invalid\n");
	}

	return 0;
}

static int lastpc_plt_dump(struct lastpc_plt *plt, char *buf, int len)
{
	struct lastpc_imp *drv = to_lastpc_imp(plt);
	int total_cpu = num_possible_cpus();
	char symbol[KSYM_SYMBOL_LEN];
	unsigned long size = 0;
	unsigned long offset = 0;
	char *ptr = buf;
	int i = 0;

	if (!drv->is_from_loader) {
		pr_err("[LASTPC] sadly, no valid lastpc");
		ptr += sprintf(ptr, "[LASTPC] sadly, no valid lastpc\n");
		return 0;
	}

	for (i = 0; i < total_cpu; ++i) {
		kallsyms_lookup(drv->pc[i], &size, &offset, NULL, symbol);
		ptr += sprintf(ptr, "CPU%d PC=0x%llx(%s+0x%lx)",
				i, drv->pc[i], symbol, offset);
		ptr += sprintf(ptr, ", FP=0x%llx, SP=0x%llx\n",
				drv->fp[i], drv->sp[i]);
	}

	return 0;
}

static int reboot_test(struct lastpc_plt *plt)
{
	return 0;
}

static struct lastpc_plt_operations lastpc_ops = {
	.start = lastpc_plt_start,
	.dump = lastpc_plt_dump,
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
	drv->plt.chip_code = 0x6797;
	drv->plt.min_buf_len = 2048;

	ret = lastpc_register(&drv->plt);
	if (ret) {
		pr_err("%s:%d: lastpc_register failed\n",
			__func__, __LINE__);
		goto register_lastpc_err;
	}

	return 0;

register_lastpc_err:
	kfree(drv);
	return ret;
}

arch_initcall(lastpc_init);
