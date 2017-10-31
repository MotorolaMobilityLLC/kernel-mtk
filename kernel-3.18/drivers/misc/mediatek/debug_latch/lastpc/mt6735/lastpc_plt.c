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
#include <asm/io.h>
#include <mt-plat/mt_io.h>

#include "../lastpc.h"

struct lastpc_imp {
	struct lastpc_plt plt;
	void __iomem *toprgu_reg;
};

#define to_lastpc_imp(p)	container_of((p), struct lastpc_imp, plt)
/*
#define LASTPC                                0X20
#define LASTSP                                0X24
#define LASTFP                                0X28
#define MUX_CONTOL_C0_REG             (base + 0x140)
#define MUX_READ_C0_REG                       (base + 0x144)
#define MUX_CONTOL_C1_REG             (base + 0x21C)
#define MUX_READ_C1_REG                       (base + 0x25C)

static unsigned int LASTPC_MAGIC_NUM[] = {0x3, 0xB, 0x33, 0x43};
*/

static int lastpc_plt_start(struct lastpc_plt *plt)
{
	return 0;
}

static int lastpc_plt_dump(struct lastpc_plt *plt, char *buf, int len)
{
	void __iomem *mcu_base = plt->common->base + 0x410;
	int ret = -1, cnt = num_possible_cpus();
	char *ptr = buf;
	unsigned long pc_value;
	unsigned long fp_value;
	unsigned long sp_value;
#ifdef CONFIG_ARM64
	unsigned long pc_value_h;
	unsigned long fp_value_h;
	unsigned long sp_value_h;
#endif
	unsigned long size = 0;
	unsigned long offset = 0;
	char str[KSYM_SYMBOL_LEN];
	int i;
	int cluster, cpu_in_cluster;

	if (cnt < 0)
		return ret;

#ifdef CONFIG_ARM64
	/* Get PC, FP, SP and save to buf */
	for (i = 0; i < cnt; i++) {
		cluster = i / 4;
		cpu_in_cluster = i % 4;
		pc_value_h =
		    readl(IOMEM((mcu_base + 0x4) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		pc_value =
		    (pc_value_h << 32) |
		    readl(IOMEM((mcu_base + 0x0) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		fp_value_h =
		    readl(IOMEM((mcu_base + 0x14) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		fp_value =
		    (fp_value_h << 32) |
		    readl(IOMEM((mcu_base + 0x10) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		sp_value_h =
		    readl(IOMEM((mcu_base + 0x1c) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		sp_value =
		    (sp_value_h << 32) |
		    readl(IOMEM((mcu_base + 0x18) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		kallsyms_lookup(pc_value, &size, &offset, NULL, str);
		ptr +=
		    sprintf(ptr,
			    "[LAST PC] CORE_%d PC = 0x%lx(%s + 0x%lx), FP = 0x%lx, SP = 0x%lx\n", i,
			    pc_value, str, offset, fp_value, sp_value);
		pr_err("[LAST PC] CORE_%d PC = 0x%lx(%s), FP = 0x%lx, SP = 0x%lx\n", i, pc_value,
			  str, fp_value, sp_value);
	}
#else
	/* Get PC, FP, SP and save to buf */
	for (i = 0; i < cnt; i++) {
		cluster = i / 4;
		cpu_in_cluster = i % 4;
		pc_value =
		    readl(IOMEM((mcu_base + 0x0) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		fp_value =
		    readl(IOMEM((mcu_base + 0x8) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		sp_value =
		    readl(IOMEM((mcu_base + 0xc) + (cpu_in_cluster << 5) + (0x100 * cluster)));
		kallsyms_lookup((unsigned long)pc_value, &size, &offset, NULL, str);
		ptr +=
		    sprintf(ptr,
			    "[LAST PC] CORE_%d PC = 0x%lx(%s + 0x%lx), FP = 0x%lx, SP = 0x%lx\n", i,
			    pc_value, str, offset, fp_value, sp_value);
		pr_err("[LAST PC] CORE_%d PC = 0x%lx(%s), FP = 0x%lx, SP = 0x%lx\n", i, pc_value,
			  str, fp_value, sp_value);
	}
#endif

#if 0
	/* Get PC, FP, SP and save to buf */
	for (i = 0; i < cnt; i++) {
		/* this calculation assumes that we have 4 cores in the first cluster, and 2 clusters in the system */
		cluster = i / 4;
		cpu_in_cluster = i % 4;
		if (cluster == 0) {
			writel(LASTPC + i, MUX_CONTOL_C0_REG);
			pc_value = readl(MUX_READ_C0_REG);
			writel(LASTSP + i, MUX_CONTOL_C0_REG);
			sp_value = readl(MUX_READ_C0_REG);
			writel(LASTFP + i, MUX_CONTOL_C0_REG);
			fp_value = readl(MUX_READ_C0_REG);
			kallsyms_lookup((unsigned long)pc_value, &size, &offset, NULL, str);
			ptr +=
			    sprintf(ptr, "CORE_%d PC = 0x%x(%s + 0x%lx), FP = 0x%x, SP = 0x%x\n", i,
				    pc_value, str, offset, fp_value, sp_value);
		} else {
			writel(LASTPC_MAGIC_NUM[cpu_in_cluster], MUX_CONTOL_C1_REG);
			pc_value = readl(MUX_READ_C1_REG);
			writel(LASTPC_MAGIC_NUM[cpu_in_cluster] + 1, MUX_CONTOL_C1_REG);
			pc_i1_value = readl(MUX_READ_C1_REG);
			ptr +=
			    sprintf(ptr, "CORE_%d PC_i0 = 0x%x, PC_i1 = 0x%x\n", i, pc_value,
				    pc_i1_value);
		}
	}
#endif

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
	drv->plt.chip_code = 0x6735;
	drv->plt.min_buf_len = 2048;	/* TODO: can calculate the len by how many levels of bt we want */

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
