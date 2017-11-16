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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/printk.h>
#define MET_USER_EVENT_SUPPORT

#include <mt-plat/mt_io.h>
#include <mt-plat/sync_write.h>
#include <mach/emi_mpu.h>
#include "emi_bwl.h"

void __iomem *DRAMC_BASE_ADDR = NULL;
DEFINE_SEMAPHORE(emi_bwl_sem);

static struct platform_driver mem_bw_ctrl = {
	.driver     = {
		.name = "mem_bw_ctrl",
		.owner = THIS_MODULE,
	},
};

static struct platform_driver ddr_type = {
	.driver     = {
		.name = "ddr_type",
		.owner = THIS_MODULE,
	},
};

/* define EMI bandwiwth limiter control table */
static struct emi_bwl_ctrl ctrl_tbl[NR_CON_SCE];

/* current concurrency scenario */
static int cur_con_sce = 0x0FFFFFFF;

/* define concurrency scenario strings */
static char * const con_sce_str[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) (#con_sce),
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};

static const unsigned int emi_arba_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) arba,
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbb_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) arbb,
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbc_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) arbc,
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbd_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) arbd,
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbe_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) arbe,
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbf_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) arbf,
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbg_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) arbg,
#include "con_sce_lpddr2_1066.h"
#undef X_CON_SCE
};

int get_dram_type(void)
{
	unsigned int value = readl(IOMEM(DRAMC_BASE_ADDR + DRAMC_ACTIM1));

	if ((value >> 28) & 0x1)
		return LPDDR3_1066;
	else
		return LPDDR2_1066;
}


/*
 * mtk_mem_bw_ctrl: set EMI bandwidth limiter for memory bandwidth control
 * @sce: concurrency scenario ID
 * @op: either ENABLE_CON_SCE or DISABLE_CON_SCE
 * Return 0 for success; return negative values for failure.
 */
int mtk_mem_bw_ctrl(int sce, int op)
{
	int i, highest;

	if (sce >= NR_CON_SCE)
		return -1;

	if (op != ENABLE_CON_SCE && op != DISABLE_CON_SCE)
		return -1;

	if (in_interrupt())
		return -1;

	down(&emi_bwl_sem);

	if (op == ENABLE_CON_SCE)
		ctrl_tbl[sce].ref_cnt++;

	else if (op == DISABLE_CON_SCE) {
		if (ctrl_tbl[sce].ref_cnt != 0)
			ctrl_tbl[sce].ref_cnt--;
	}

	/* find the scenario with the highest priority */
	highest = -1;
	for (i = 0; i < NR_CON_SCE; i++) {
		if (ctrl_tbl[i].ref_cnt != 0) {
			highest = i;
			break;
		}
	}
	if (highest == -1)
		highest = CON_SCE_NORMAL;


	/* set new EMI bandwidth limiter value */
	if (highest != cur_con_sce) {
		mt_emi_reg_write(emi_arba_val[highest], EMI_ARBA);
		mt_emi_reg_write(emi_arbb_val[highest], EMI_ARBB);
		mt_emi_reg_write(emi_arbc_val[highest], EMI_ARBC);
		mt_emi_reg_write(emi_arbd_val[highest], EMI_ARBD);
		mt_emi_reg_write(emi_arbe_val[highest], EMI_ARBE);
		mt_emi_reg_write(emi_arbf_val[highest], EMI_ARBF);
		mt_emi_reg_write(emi_arbg_val[highest], EMI_ARBG);

		if (get_dram_type() == LPDDR3_1066) {
			mt_emi_reg_write(0x06171a2a, EMI_CONB);
			mt_emi_reg_write(0x07160616, EMI_CONC);
			mt_emi_reg_write(0x2928172a, EMI_COND);
		} else if (get_dram_type() == LPDDR2_1066) {
			mt_emi_reg_write(0x06101a29, EMI_CONB);
			mt_emi_reg_write(0x07060607, EMI_CONC);
			mt_emi_reg_write(0x0706061a, EMI_COND);
		}
		cur_con_sce = highest;
	}

	up(&emi_bwl_sem);
	return 0;
}

/*
 * ddr_type_show: sysfs ddr_type file show function.
 * @driver:
 * @buf: the string of ddr type
 * Return the number of read bytes.
 */
static ssize_t ddr_type_show(struct device_driver *driver, char *buf)
{
	if (get_dram_type() == LPDDR3_1066)
		sprintf(buf, "LPDDR3_1066\n");
	else if (get_dram_type() == LPDDR2_1066)
		sprintf(buf, "LPDDR2_1066\n");

	return strlen(buf);
}

/*
 * ddr_type_store: sysfs ddr_type file store function.
 * @driver:
 * @buf:
 * @count:
 * Return the number of write bytes.
 */
static ssize_t ddr_type_store(struct device_driver *driver,
			      const char *buf,
			      size_t count)
{
	/*do nothing*/
	return count;
}

DRIVER_ATTR(ddr_type, 0644, ddr_type_show, ddr_type_store);

/*
 * con_sce_show: sysfs con_sce file show function.
 * @driver:
 * @buf:
 * Return the number of read bytes.
 */
static ssize_t con_sce_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	int i = 0;

	if (cur_con_sce >= NR_CON_SCE)
		ptr += sprintf(ptr, "none\n");
	else
		ptr += sprintf(ptr, "current scenario: %s\n",
			       con_sce_str[cur_con_sce]);

	ptr += sprintf(ptr, "%s\n", con_sce_str[cur_con_sce]);
	ptr += sprintf(ptr, "EMI_ARBA = 0x%x\n",  mt_emi_reg_read(EMI_ARBA));
	ptr += sprintf(ptr, "EMI_ARBC = 0x%x\n",  mt_emi_reg_read(EMI_ARBB));
	ptr += sprintf(ptr, "EMI_ARBC = 0x%x\n",  mt_emi_reg_read(EMI_ARBC));
	ptr += sprintf(ptr, "EMI_ARBD = 0x%x\n",  mt_emi_reg_read(EMI_ARBD));
	ptr += sprintf(ptr, "EMI_ARBE = 0x%x\n",  mt_emi_reg_read(EMI_ARBE));
	ptr += sprintf(ptr, "EMI_ARBF = 0x%x\n",  mt_emi_reg_read(EMI_ARBF));
	ptr += sprintf(ptr, "EMI_ARBG = 0x%x\n",  mt_emi_reg_read(EMI_ARBG));
	for (i = 0; i < NR_CON_SCE; i++)
		ptr += sprintf(ptr, "%s = 0x%x\n",
			       con_sce_str[i],
			       ctrl_tbl[i].ref_cnt);

	pr_notice("[EMI BWL] EMI_ARBA = 0x%x\n", mt_emi_reg_read(EMI_ARBA));
	pr_notice("[EMI BWL] EMI_ARBB = 0x%x\n", mt_emi_reg_read(EMI_ARBB));
	pr_notice("[EMI BWL] EMI_ARBC = 0x%x\n", mt_emi_reg_read(EMI_ARBC));
	pr_notice("[EMI BWL] EMI_ARBD = 0x%x\n", mt_emi_reg_read(EMI_ARBD));
	pr_notice("[EMI BWL] EMI_ARBE = 0x%x\n", mt_emi_reg_read(EMI_ARBE));
	pr_notice("[EMI BWL] EMI_ARBF = 0x%x\n", mt_emi_reg_read(EMI_ARBF));
	pr_notice("[EMI BWL] EMI_ARBG = 0x%x\n", mt_emi_reg_read(EMI_ARBG));
	return strlen(buf);
}

/*
 * con_sce_store: sysfs con_sce file store function.
 * @driver:
 * @buf:
 * @count:
 * Return the number of write bytes.
 */
static ssize_t con_sce_store(struct device_driver *driver,
			     const char *buf,
			     size_t count)
{
	int i;

	for (i = 0; i < NR_CON_SCE; i++) {
		if (!strncmp(buf, con_sce_str[i], strlen(con_sce_str[i]))) {
			if (!strncmp(buf + strlen(con_sce_str[i]) + 1,
				     EN_CON_SCE_STR,
				     strlen(EN_CON_SCE_STR))) {
				mtk_mem_bw_ctrl(i, ENABLE_CON_SCE);
				pr_err("concurrency scenario %s ON\n",
				       con_sce_str[i]);
				break;
			} else if (!strncmp(buf + strlen(con_sce_str[i]) + 1,
					    DIS_CON_SCE_STR,
					    strlen(DIS_CON_SCE_STR))) {
				mtk_mem_bw_ctrl(i, DISABLE_CON_SCE);
				pr_err("concurrency scenario %s OFF\n",
				       con_sce_str[i]);
				break;
			}
		}
	}

	return count;
}

DRIVER_ATTR(concurrency_scenario, 0664, con_sce_show, con_sce_store);

/*
 * emi_bwl_mod_init: module init function.
 */
static int __init emi_bwl_mod_init(void)
{
	int ret;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,DRAMC0");
	if (node) {
		DRAMC_BASE_ADDR = of_iomap(node, 0);
		pr_info("[EMI/BWL] get DRAMC_BASE_ADDR @ %p\n",
			DRAMC_BASE_ADDR);
	} else {
		pr_err("[EMI/BWL] can't find compatible node\n");
		return -1;
	}

	ret = mtk_mem_bw_ctrl(CON_SCE_NORMAL, ENABLE_CON_SCE);
	if (ret)
		pr_err("[EMI/BWL] fail to set EMI bandwidth limiter\n");

	/* Register BW ctrl interface */
	ret = platform_driver_register(&mem_bw_ctrl);
	if (ret)
		pr_err("[EMI/BWL] fail to register EMI_BW_LIMITER driver\n");

	ret = driver_create_file(&mem_bw_ctrl.driver,
				 &driver_attr_concurrency_scenario);
	if (ret)
		pr_err("[EMI/BWL] fail to create EMI_BW_LIMITER sysfs file\n");

	/* Register DRAM type information interface */
	ret = platform_driver_register(&ddr_type);
	if (ret)
		pr_err("[EMI/BWL] fail to register DRAM_TYPE driver\n");

	ret = driver_create_file(&ddr_type.driver, &driver_attr_ddr_type);
	if (ret)
		pr_err("[EMI/BWL] fail to create DRAM_TYPE sysfs file\n");

	return 0;
}

/*
 * emi_bwl_mod_exit: module exit function.
 */
static void __exit emi_bwl_mod_exit(void)
{
}

late_initcall(emi_bwl_mod_init);
module_exit(emi_bwl_mod_exit);

