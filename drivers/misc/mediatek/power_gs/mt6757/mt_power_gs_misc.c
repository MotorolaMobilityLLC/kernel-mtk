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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mt-plat/aee.h>
#include <mt-plat/upmu_common.h>

#include "mt_clkmgr.h"
#include "mt_power_gs_misc.h"
#include "mt_freqhopping_drv.h"

#define DEBUG_MSG_ON 0
/************************************************
 **********      register access       **********
 ************************************************/
void __iomem *gs_clockTop_infra_base;
void __iomem *gs_others_base;
void __iomem *lBaseAddr;

static void mt_power_gs_dump_value(struct seq_file *m, const unsigned int pBaseAddr,
	const unsigned int *array, unsigned int len)
{
	unsigned int i, value, mask, golden;

#if DEBUG_MSG_ON
	pr_err("[power_gs_misc] Begin %s\n", __func__);
#endif
	/*pr_err("[power_gs_misc] base1 %p, base2 %p\n", gs_clockTop_infra_base, gs_others_base);*/

	if (pBaseAddr == 0x10000000)
		lBaseAddr = gs_clockTop_infra_base;
	else
		lBaseAddr = gs_others_base;

	/*pr_err("[power_gs_misc] lBase %p\n", lBaseAddr);*/

	for (i = 0; i < len; i += 3) {
		value = readl(lBaseAddr + array[i]);
		mask = array[i + 1];
		golden = array[i + 2];
		if ((value & mask) != golden)
			seq_printf(m, "0x%x - 0x%04x - 0x%04x - 0x%04x - Caution\n",
				(pBaseAddr + array[i]), value, mask, golden);
		else
			seq_printf(m, "0x%x - 0x%04x - 0x%04x - 0x%04x\n",
				(pBaseAddr + array[i]), value, mask, golden);
#if DEBUG_MSG_ON
		pr_err("[power_gs_misc] %d value 0x%04x\n", i, readl(lBaseAddr + array[i]));
#endif
	}
}

static void mt_power_gs_ctCheck_for_VA_protect(struct seq_file *m)
{
	unsigned int value, mask, golden;

	/*0x1A27C, 0xFFFFFFFF, 0xFFFFFFFF, armpll_debug_out */
	value = mt6757_0x1001AXXX_reg_read(0x27C);
	mask = 0xFFFFFFFF;
	golden = 0xFFFFFFFF;
	if ((value & mask) != golden)
		seq_printf(m, "0x%x - 0x%04x - 0x%04x - 0x%04x - Caution\n",
				0x1001A27C, value, mask, golden);
	else
		seq_printf(m, "0x%x - 0x%04x - 0x%04x - 0x%04x\n",
				0x1001A27C, value, mask, golden);

	/*0x1A284, 0xFFFFFFFF, 0x00000000, armpll_debug_out*/
	value = mt6757_0x1001AXXX_reg_read(0x284);
	mask = 0xFFFFFFFF;
	golden = 0x0;
	if ((value & mask) != golden)
		seq_printf(m, "0x%x - 0x%04x - 0x%04x - 0x%04x - Caution\n",
				0x1001A284, value, mask, golden);
	else
		seq_printf(m, "0x%x - 0x%04x - 0x%04x - 0x%04x\n",
				0x1001A284, value, mask, golden);
}

static void mt_power_gs_misc_iomap(void)
{
	struct device_node *node;

#if DEBUG_MSG_ON
	pr_err("[power_gs_misc] Begin %s\n", __func__);
#endif
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6757-power_gs");
	if (!node)
		pr_err("[power_gs_misc] find node failed %d\n", __LINE__);

	gs_clockTop_infra_base = of_iomap(node, 0);
	if (!gs_clockTop_infra_base)
		pr_err("[power_gs_misc] get base failed %d\n", __LINE__);

	gs_others_base = of_iomap(node, 1);
	if (!gs_others_base)
		pr_err("[power_gs_misc] get base failed %d\n", __LINE__);

	/*pr_err("[power_gs_misc] End %s\n", __func__);*/
}

static void mt_power_gs_misc_iounmap(void)
{
	iounmap(gs_clockTop_infra_base);
	iounmap(gs_others_base);
}

static int mt_power_gs_misc_read(struct seq_file *m, void *v)
{
	mt_power_gs_misc_iomap();

#if DEBUG_MSG_ON
	pr_err("[power_gs_misc] Begin %s\n", __func__);
#endif
	/* MDP, DISP, and SMI are in the DIS subsys */
	if (subsys_is_on(SYS_DIS)) {
		seq_puts(m, "MDP\n");
		seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
		mt_power_gs_dump_value(m, 0x12000000, gs_mdp_array_ptr, gs_mdp_array_len);

		seq_puts(m, "DISP\n");
		seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
		mt_power_gs_dump_value(m, 0x12000000, gs_disp_array_ptr, gs_disp_array_len);

		seq_puts(m, "SMI\n");
		seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
		mt_power_gs_dump_value(m, 0x12000000, gs_smi_array_ptr, gs_smi_array_len);
	} else {
		seq_puts(m, "SYS_DIS off;No MDP, DISP, and SMI info\n");
	}

	/* IMG is in the ISP subsys */
	if (subsys_is_on(SYS_ISP)) {
		seq_puts(m, "IMG\n");
		seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
		mt_power_gs_dump_value(m, 0x12000000, gs_img_array_ptr, gs_img_array_len);
	} else {
		seq_puts(m, "SYS_ISP off;No IMG info\n");
	}

	/* VENC is in the VEN subsys */
	if (subsys_is_on(SYS_VEN)) {
		seq_puts(m, "VENC\n");
		seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
		mt_power_gs_dump_value(m, 0x12000000, gs_venc_array_ptr, gs_venc_array_len);
	} else {
		seq_puts(m, "SYS_VEN off;No VENC info\n");
	}

	/* VDEC is in the VDE subsys */
	if (subsys_is_on(SYS_VDE)) {
		seq_puts(m, "VDEC\n");
		seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
		mt_power_gs_dump_value(m, 0x12000000, gs_vdec_array_ptr, gs_vdec_array_len);
	} else {
		seq_puts(m, "SYS_VDE off;No VDEC info\n");
	}

	/* Infra is always on */
	seq_puts(m, "Infra\n");
	seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
	mt_power_gs_dump_value(m, 0x10000000, gs_infra_array_ptr, gs_infra_array_len);

	/* ClockTop is always on */
	seq_puts(m, "Clock top\n");
	seq_puts(m, "Addr  - Value  - Mask   - Golden\n");
	mt_power_gs_dump_value(m, 0x10000000, gs_clockTop_array_ptr, gs_clockTop_array_len);
	mt_power_gs_ctCheck_for_VA_protect(m);
	mt_power_gs_misc_iounmap();
	/*pr_err("[power_gs_misc] End %s\n", __func__);*/
	return 0;
}

static int mt_power_gs_misc_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_power_gs_misc_read, NULL);
}

static const struct file_operations mt_power_gs_misc_fops = {
	.owner = THIS_MODULE,
	.open = mt_power_gs_misc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init mt_power_gs_misc_init(void)
{

#if DEBUG_MSG_ON
	pr_err("[power_gs_misc] Begin %s\n", __func__);
#endif

	if (!mt_power_gs_dir)
		pr_err("[power_gs_misc] [%s]: mkdir /proc/mt_power_gs failed\n", __func__);

	proc_create("gs_misc", S_IRUGO, mt_power_gs_dir, &mt_power_gs_misc_fops);

	/*pr_err("[power_gs_misc] End %s\n", __func__);*/
	return 0;
}

static void __exit mt_power_gs_misc_exit(void)
{
}

late_initcall(mt_power_gs_misc_init);
module_exit(mt_power_gs_misc_exit);
