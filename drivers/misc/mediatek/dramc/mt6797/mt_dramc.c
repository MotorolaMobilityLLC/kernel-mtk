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
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_freqhopping.h>
/*#include <mach/mt_reg_base.h>*/
/*#include <mach/emi_bwl.h>*/
/*#include <mach/mt_typedefs.h>*/
/*#include <mach/mt_sleep.h>*/
/*#include <mach/dma.h>*/
/*#include <mach/mt_dcm.h>*/
/*#include <mach/sync_write.h>*/
/*#include <mach/md32_ipi.h>*/
/*#include <mach/md32_helper.h>*/
/*#include <mt_spm_vcore_dvfs.h>*/
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <asm/setup.h>
#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mt_io.h>
#include <mt-plat/dma.h>
#include <mt-plat/sync_write.h>
#include <mt_spm_sleep.h>
#include <mt_spm_reg.h>

#include "mt_dramc.h"

#ifdef CONFIG_OF_RESERVED_MEM
#define DRAM_R0_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r0_dummy_read"
#define DRAM_R1_DUMMY_READ_RESERVED_KEY "reserve-memory-dram_r1_dummy_read"
#include <linux/of_reserved_mem.h>
#include <mt-plat/mtk_memcfg.h>
#endif

#include <mt-plat/aee.h>

void __iomem *DRAMCAO_CHA_BASE_ADDR;
void __iomem *DDRPHY_BASE_ADDR;
void __iomem *DRAMCNAO_CHA_BASE_ADDR;
void __iomem *TOPCKGEN_BASE_ADDR;
void __iomem *DRAMCAO_CHB_BASE_ADDR;
void __iomem *INFRACFG_AO_BASE_ADDR;
void __iomem *DRAMCNAO_CHB_BASE_ADDR;
void __iomem *SLEEP_BASE_ADDR;
#define DRAM_RSV_SIZE 0x1000

static DEFINE_MUTEX(dram_dfs_mutex);
int highest_dram_data_rate = 0;
unsigned char old_IC_No_DummyRead = 0;
unsigned char DFS_type = 0;

#ifdef CONFIG_MTK_DRAMC_PASR
static unsigned int enter_pdp_cnt;
#endif

/*extern bool spm_vcorefs_is_dvfs_in_porgress(void);*/
#define Reg_Sync_Writel(addr, val)   writel(val, IOMEM(addr))
#define Reg_Readl(addr) readl(IOMEM(addr))
static unsigned int dram_rank_num;
phys_addr_t dram_rank0_addr, dram_rank1_addr;

#define DRAMC_RSV_TAG "[DRAMC_RSV]"
#define dramc_rsv_aee_warn(string, args...) do {\
	pr_err("[ERR]"string, ##args); \
	aee_kernel_warning(DRAMC_RSV_TAG, "[ERR]"string, ##args);  \
} while (0)

struct dram_info *g_dram_info_dummy_read = NULL;

static int __init dt_scan_dram_info(unsigned long node,
const char *uname, int depth, void *data)
{
	char *type = (char *)of_get_flat_dt_prop(node, "device_type", NULL);
	const __be32 *reg, *endp;
	unsigned long l;

	/* We are scanning "memory" nodes only */
	if (type == NULL) {
		/*
		* The longtrail doesn't have a device_type on the
		* /memory node, so look for the node called /memory@0.
		*/
		if (depth != 1 || strcmp(uname, "memory@0") != 0)
			return 0;
	} else if (strcmp(type, "memory") != 0)
		return 0;

	reg = (const __be32 *)of_get_flat_dt_prop(node,
	(const char *)"reg", (int *)&l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));
	if (node) {
		g_dram_info_dummy_read =
		(struct dram_info *)of_get_flat_dt_prop(node,
		"orig_dram_info", NULL);
		if (g_dram_info_dummy_read == NULL) {
			old_IC_No_DummyRead = 1;
			return 0; }

		pr_err("[DRAMC] dram info dram rank number = %d\n",
		g_dram_info_dummy_read->rank_num);

		if (dram_rank_num == SINGLE_RANK) {
			g_dram_info_dummy_read->rank_info[0].start = dram_rank0_addr;
			pr_err("[DRAMC] dram info dram rank0 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start);
		} else if (dram_rank_num == DUAL_RANK) {
			g_dram_info_dummy_read->rank_info[0].start = dram_rank0_addr;
			g_dram_info_dummy_read->rank_info[1].start = dram_rank1_addr;
			pr_err("[DRAMC] dram info dram rank0 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start);
			pr_err("[DRAMC] dram info dram rank1 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[1].start);
		}
	  else
		pr_err("[DRAMC] dram info dram rank number incorrect !!!\n");
	}

	return node;
}

static int check_dramc_base_addr(void)
{
	if ((!DRAMCAO_CHA_BASE_ADDR) || (!DDRPHY_BASE_ADDR)
		|| (!DRAMCNAO_CHA_BASE_ADDR))
		return -1;
	else
		return 0;
}

void *mt_dramc_cha_base_get(void)
{
	return DRAMCAO_CHA_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_cha_base_get);

void *mt_dramc_chb_base_get(void)
{
	return DRAMCAO_CHB_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_chb_base_get);

void *mt_dramc_nao_cha_base_get(void)
{
	return DRAMCNAO_CHA_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_cha_base_get);

void *mt_dramc_nao_chb_base_get(void)
{
	return DRAMCNAO_CHB_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_chb_base_get);

void *mt_ddrphy_base_get(void)
{
	return DDRPHY_BASE_ADDR;
}
EXPORT_SYMBOL(mt_ddrphy_base_get);

unsigned int support_4GB_mode(void)
{
	int ret = 0;
	phys_addr_t max_dram_size = get_max_DRAM_size();

	if (max_dram_size >= 0x100000000ULL)	/*dram size = 4GB*/
		ret = 1;

	return ret;
}

#ifdef DRAM_HQA
void dram_HQA_adjust_voltage(void)
{
#ifdef HVcore1	/*Vcore1=1.10V, Vdram=1.30V,  Vio18=1.8*/
	pmic_config_interface(MT6351_BUCK_VCORE_CON4, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(MT6351_BUCK_VCORE_CON5, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(MT6351_VDRAM_ANA_CON0, Vdram_HV, 0x7F, 0);
	pr_err("[HQA]Set HVcore1 setting: Vcore1=1.10V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.30V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON4),
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON5),
		Vcore1_HV, upmu_get_reg_value(MT6351_VDRAM_ANA_CON0), Vdram_HV);
#endif

#ifdef NV	/*Vcore1=1.00V, Vdram=1.22V,  Vio18=1.8*/
	pmic_config_interface(MT6351_BUCK_VCORE_CON4, Vcore1_NV, 0x7F, 0);
	pmic_config_interface(MT6351_BUCK_VCORE_CON5, Vcore1_NV, 0x7F, 0);
	pmic_config_interface(MT6351_VDRAM_ANA_CON0, Vdram_NV, 0x7F, 0);
	pr_err("[HQA]Set NV setting: Vcore1=1.00V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.22V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON4),
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON5),
		Vcore1_NV, upmu_get_reg_value(MT6351_VDRAM_ANA_CON0), Vdram_NV);
#endif

#ifdef LVcore1	/*Vcore1=0.90V, Vdram=1.16V,  Vio18=1.8*/
	pmic_config_interface(MT6351_BUCK_VCORE_CON4, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(MT6351_BUCK_VCORE_CON5, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(MT6351_VDRAM_ANA_CON0, Vdram_LV, 0x7F, 0);
	pr_err("[HQA]Set LVcore1 setting: Vcore1=0.90V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.16V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON4),
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON5),
		Vcore1_LV, upmu_get_reg_value(MT6351_VDRAM_ANA_CON0), Vdram_LV);
#endif

#ifdef HVcore1_LVdram	/*Vcore1=1.10V, Vdram=1.16V,  Vio18=1.8*/
	pmic_config_interface(MT6351_BUCK_VCORE_CON4, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(MT6351_BUCK_VCORE_CON5, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(MT6351_VDRAM_ANA_CON0, Vdram_LV, 0x7F, 0);
	pr_err("[HQA]Set HVcore1_LVdram setting: Vcore1=1.10V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.16V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON4),
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON5),
		Vcore1_HV, upmu_get_reg_value(MT6351_VDRAM_ANA_CON0), Vdram_LV);
#endif

#ifdef LVcore1_HVdram	/*Vcore1=0.90V, Vdram=1.30V,  Vio18=1.8*/
	pmic_config_interface(MT6351_BUCK_VCORE_CON4, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(MT6351_BUCK_VCORE_CON5, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(MT6351_VDRAM_ANA_CON0, Vdram_HV, 0x7F, 0);
	pr_err("[HQA]Set LVcore1_HVdram setting: Vcore1=0.90V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.30V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON4),
		upmu_get_reg_value(MT6351_BUCK_VCORE_CON5),
		Vcore1_LV, upmu_get_reg_value(MT6351_VDRAM_ANA_CON0), Vdram_HV);
#endif
}
#endif

void spm_dpd_init(void)
{
	unsigned int u4value = 0;
	unsigned int recover7_0;
	unsigned int recover8;

	/* p->channel = CHANNEL_A; */
	recover7_0 = readl(PDEF_DRAMC0_CHA_REG_1E4) & 0x00ff0000;
	recover8 = readl(PDEF_DRAMC0_CHA_REG_1DC) & 0x00000001;
	u4value = recover7_0 | recover8;
	writel(u4value, PDEF_SPM_PASR_DPD_0);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E8) | 0x04000000,
	PDEF_DRAMC0_CHA_REG_1E8);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xff00ffff,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) & 0xfffffffe,
	PDEF_DRAMC0_CHA_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xF7FFFFFF,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) | 0x04000000,
	PDEF_DRAMC0_CHA_REG_1DC);

	mb(); /* flush memory */
	udelay(1);

	writel(readl(PDEF_DRAMC0_CHA_REG_1EC) & 0xffefffff,
	PDEF_DRAMC0_CHA_REG_1EC);
	writel(readl(PDEF_DRAMC0_CHA_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHA_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHA_REG_110) & 0xffdfffff,
	PDEF_DRAMC0_CHA_REG_110);
	writel(readl(PDEF_DRAMC0_CHA_REG_1EC) | 0x00010000,
	PDEF_DRAMC0_CHA_REG_1EC);

	/*p->channel = CHANNEL_B; */
	recover7_0 = readl(PDEF_DRAMC0_CHB_REG_1E4) & 0x00ff0000;
	recover8 = readl(PDEF_DRAMC0_CHB_REG_1DC) & 0x00000001;
	u4value = recover7_0 | recover8;
	writel(u4value, PDEF_SPM_PASR_DPD_3);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E8) | 0x04000000,
	PDEF_DRAMC0_CHB_REG_1E8);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xff00ffff,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) & 0xfffffffe,
	PDEF_DRAMC0_CHB_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xF7FFFFFF,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) | 0x04000000,
	PDEF_DRAMC0_CHB_REG_1DC);

	mb(); /* flush memory */
	udelay(1);

	writel(readl(PDEF_DRAMC0_CHB_REG_1EC) & 0xffefffff,
	PDEF_DRAMC0_CHB_REG_1EC);
	writel(readl(PDEF_DRAMC0_CHB_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHB_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHA_REG_110) & 0xffdfffff,
	PDEF_DRAMC0_CHB_REG_110);
	writel(readl(PDEF_DRAMC0_CHB_REG_1EC) | 0x00010000,
	PDEF_DRAMC0_CHB_REG_1EC);
}

void spm_dpd_dram_init(void)
{
	unsigned int u4value = 0;
	unsigned int recover7_0;
	unsigned int recover8;
	unsigned int u4value_E4 = 0;
	unsigned int u4value_F4 = 0;
	unsigned int frequency = 0;
	unsigned int u4AutoRefreshBak = 0;

	/* p->channel = CHANNEL_A; */
	u4AutoRefreshBak = readl(PDEF_DRAMC0_CHA_REG_008) & 0x10000000;
	u4value_E4 = readl(PDEF_DRAMC0_CHA_REG_0E4);
	u4value_F4 = readl(PDEF_DRAMC0_CHA_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHA_REG_0F4) & 0xffdfffff,
	PDEF_DRAMC0_CHA_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHA_REG_110) | 0x80000000,
	PDEF_DRAMC0_CHA_REG_110);

	mb(); /* flush memory */
	udelay(1);

	writel(readl(PDEF_DRAMC0_CHA_REG_110) | 0x00200000,
	PDEF_DRAMC0_CHA_REG_110);

	mb(); /* flush memory */
	udelay(220);

	writel(readl(PDEF_DRAMC0_CHA_REG_008) | 0x10000000,
	PDEF_DRAMC0_CHA_REG_008);
	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) | 0x4000000,
	PDEF_DRAMC0_CHA_REG_1DC);
	udelay(1);
	writel(readl(PDEF_DRAMC0_CHA_REG_0E4) | 0x00000004,
	PDEF_DRAMC0_CHA_REG_0E4);
	writel(readl(PDEF_DRAMC0_CHA_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHA_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHA_REG_1EC) | 0x00100000,
	PDEF_DRAMC0_CHA_REG_1EC);
	writel(readl(PDEF_DRAMC0_CHA_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHA_REG_0F4);
	udelay(200);
	writel(readl(PDEF_DRAMC0_CHA_REG_138) & 0xffff7fff,
	PDEF_DRAMC0_CHA_REG_138);
	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) & 0xfffffffd,
	PDEF_DRAMC0_CHA_REG_1DC);
	writel(0x1000003f, PDEF_DRAMC0_CHA_REG_088);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(10);

	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(0x10ff000a, PDEF_DRAMC0_CHA_REG_088);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(0x10010003, PDEF_DRAMC0_CHA_REG_088);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(0x10830001, PDEF_DRAMC0_CHA_REG_088);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHA_REG_1E4);

	frequency = get_dram_data_rate(0) / 2;
	if (frequency <= 533)
		u4value = 0x10160002;/*u4MR2Value = 0x14;*/
	else if (frequency == 635)
		u4value = 0x10180002;/*u4MR2Value = 0x1e;*/
	else if (frequency == 800)
		u4value = 0x101a0002;/*u4MR2Value = 0x1c;*/
	else
		u4value = 0x101c0002;/*u4MR2Value = 0x1a;*/

	writel(u4value, PDEF_DRAMC0_CHA_REG_088);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(0x1000000b, PDEF_DRAMC0_CHA_REG_088);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) | 0x00000002,
	PDEF_DRAMC0_CHA_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHA_REG_138) | 0x00008000,
	PDEF_DRAMC0_CHA_REG_138);
	writel(readl(PDEF_DRAMC0_CHA_REG_0F4) & 0xffefffff,
	PDEF_DRAMC0_CHA_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHA_REG_110) & 0x7fffffff,
	PDEF_DRAMC0_CHA_REG_110);
	writel(u4value_E4, PDEF_DRAMC0_CHA_REG_0E4);
	writel(u4value_F4, PDEF_DRAMC0_CHA_REG_0F4);

	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) & 0xFBFFFFFF,
	PDEF_DRAMC0_CHA_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHA_REG_008) | u4AutoRefreshBak,
	PDEF_DRAMC0_CHA_REG_008);
	u4value = readl(PDEF_SPM_PASR_DPD_0);
	recover7_0 = (u4value & 0x00ff0000);
	recover8 = u4value & 0x00000001;

	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) | recover8,
	PDEF_DRAMC0_CHA_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | recover7_0,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x8000000,
	PDEF_DRAMC0_CHA_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E8) & 0xfbffffff,
	PDEF_DRAMC0_CHA_REG_1E8);
	writel(0x00000000, PDEF_DRAMC0_CHA_REG_088);

	/* p->channel = CHANNEL_B; */
	u4AutoRefreshBak = readl(PDEF_DRAMC0_CHB_REG_008) & 0x10000000;
	u4value_E4 = readl(PDEF_DRAMC0_CHB_REG_0E4);
	u4value_F4 = readl(PDEF_DRAMC0_CHB_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHB_REG_0F4) & 0xffdfffff,
	PDEF_DRAMC0_CHB_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHB_REG_110) | 0x80000000,
	PDEF_DRAMC0_CHB_REG_110);

	mb(); /* flush memory */
	udelay(1);

	writel(readl(PDEF_DRAMC0_CHB_REG_110) | 0x00200000,
	PDEF_DRAMC0_CHB_REG_110);

	mb(); /* flush memory */
	udelay(220);

	writel(readl(PDEF_DRAMC0_CHB_REG_008) | 0x10000000,
	PDEF_DRAMC0_CHB_REG_008);
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) | 0x4000000,
	PDEF_DRAMC0_CHB_REG_1DC);
	udelay(1);
	writel(readl(PDEF_DRAMC0_CHB_REG_0E4) | 0x00000004,
	PDEF_DRAMC0_CHB_REG_0E4);
	writel(readl(PDEF_DRAMC0_CHB_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHB_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHB_REG_1EC) | 0x00100000,
	PDEF_DRAMC0_CHB_REG_1EC);
	writel(readl(PDEF_DRAMC0_CHB_REG_0F4) | 0x00100000,
	PDEF_DRAMC0_CHB_REG_0F4);
	udelay(200);
	writel(readl(PDEF_DRAMC0_CHB_REG_138) & 0xffff7fff,
	PDEF_DRAMC0_CHB_REG_138);
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) & 0xfffffffd,
	PDEF_DRAMC0_CHB_REG_1DC);
	writel(0x1000003f, PDEF_DRAMC0_CHB_REG_088);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /* flush memory */
	udelay(10);

	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(0x10ff000a, PDEF_DRAMC0_CHB_REG_088);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(0x10010003, PDEF_DRAMC0_CHB_REG_088);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(0x10830001, PDEF_DRAMC0_CHB_REG_088);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHB_REG_1E4);

	frequency = get_dram_data_rate(0) / 2;
	if (frequency <= 533)
		u4value = 0x10160002;/*u4MR2Value = 0x14;*/
	else if (frequency == 635)
		u4value = 0x10180002;/*u4MR2Value = 0x1e;*/
	else if (frequency == 800)
		u4value = 0x101a0002;/*u4MR2Value = 0x1c;*/
	else
		u4value = 0x101c0002;/*u4MR2Value = 0x1a;*/

	writel(u4value, PDEF_DRAMC0_CHB_REG_088);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(0x1000000b, PDEF_DRAMC0_CHB_REG_088);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) | 0x00000002,
	PDEF_DRAMC0_CHB_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHB_REG_138) | 0x00008000,
	PDEF_DRAMC0_CHB_REG_138);
	writel(readl(PDEF_DRAMC0_CHB_REG_0F4) & 0xffefffff,
	PDEF_DRAMC0_CHB_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHB_REG_110) & 0x7fffffff,
	PDEF_DRAMC0_CHB_REG_110);
	writel(u4value_E4, PDEF_DRAMC0_CHB_REG_0E4);
	writel(u4value_F4, PDEF_DRAMC0_CHB_REG_0F4);
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) & 0xFBFFFFFF,
	PDEF_DRAMC0_CHB_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHB_REG_008) | u4AutoRefreshBak,
	PDEF_DRAMC0_CHB_REG_008);

	u4value = readl(PDEF_SPM_PASR_DPD_3);
	recover7_0 = (u4value & 0x00ff0000);
	recover8 = u4value & 0x00000001;
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) | recover8,
	PDEF_DRAMC0_CHB_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | recover7_0,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x8000000,
	PDEF_DRAMC0_CHB_REG_1E4);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E8) & 0xfbffffff,
	PDEF_DRAMC0_CHB_REG_1E8);
	writel(0x00000000, PDEF_DRAMC0_CHB_REG_088);
}

#ifdef CONFIG_MTK_DRAMC_PASR
static DEFINE_SPINLOCK(dramc_lock);
static int acquire_dram_ctrl(void)
{
	unsigned int cnt;
	unsigned long save_flags;

	/* acquire SPM HW SEMAPHORE to avoid race condition */
	spin_lock_irqsave(&dramc_lock, save_flags);

	cnt = 100;
	do {
		if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) != 0x1) {
			writel(0x1, PDEF_SPM_AP_SEMAPHORE);
			if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x1)
				break;
		} else {
			pr_err("[DRAMC] another AP has got SPM HW SEMAPHORE!\n");
			BUG();
		}

		cnt--;
		/* pr_err("[DRAMC] wait for SPM HW SEMAPHORE\n"); */
		spin_unlock_irqrestore(&dramc_lock, save_flags);
		udelay(10);
		spin_lock_irqsave(&dramc_lock, save_flags);
	} while (cnt > 0);

	if (cnt == 0) {
		spin_unlock_irqrestore(&dramc_lock, save_flags);
		pr_warn("[DRAMC] can NOT get SPM HW SEMAPHORE!\n");
		return -1;
	}

	/* pr_err("[DRAMC] get SPM HW SEMAPHORE success!\n"); */

	spin_unlock_irqrestore(&dramc_lock, save_flags);

	return 0;
}

static int release_dram_ctrl(void)
{
	/* release SPM HW SEMAPHORE to avoid race condition */
	if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x0) {
		pr_err("[DRAMC] has NOT acquired SPM HW SEMAPHORE\n");
		BUG();
	}

	writel(0x1, PDEF_SPM_AP_SEMAPHORE);
	if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x1) {
		pr_err("[DRAMC] release SPM HW SEMAPHORE fail!\n");
		BUG();
	}
	/* pr_err("[DRAMC] release SPM HW SEMAPHORE success!\n"); */

	return 0;
}

int enter_pasr_dpd_config(unsigned char segment_rank0,
			   unsigned char segment_rank1)
{
	unsigned int rank_pasr_segment[2];
	/*unsigned int dramc0_spcmd, dramc0_pd_ctrl;
	//unsigned int dramc0_pd_ctrl_2, dramc0_padctl4, dramc0_1E8; */
	unsigned int i, cnt = 1000;
	unsigned int u4value_1E4 = 0;
	unsigned int u4value_1E8 = 0;
	unsigned int u4value_1DC = 0;
	unsigned long save_flags;

	local_irq_save(save_flags);
	if (acquire_dram_ctrl() != 0) {
		pr_warn("[DRAMC0] can NOT get SPM HW SEMAPHORE!\n");
		local_irq_restore(save_flags);
		return -1;
	}
	/* pr_warn("[DRAMC0] get SPM HW SEMAPHORE!\n"); */

	rank_pasr_segment[0] = segment_rank0 & 0xFF;	/* for rank0 */
	rank_pasr_segment[1] = segment_rank1 & 0xFF;	/* for rank1 */
	pr_warn("[DRAMC0] PASR r0 = 0x%x  r1 = 0x%x\n",
	rank_pasr_segment[0], rank_pasr_segment[1]);

	/*Channel-A*/
	u4value_1E8 = readl(PDEF_DRAMC0_CHA_REG_1E8);
	u4value_1E4 = readl(PDEF_DRAMC0_CHA_REG_1E4);
	u4value_1DC = readl(PDEF_DRAMC0_CHA_REG_1DC); /* DCM backup */

	writel(readl(PDEF_DRAMC0_CHA_REG_1E8) | 0x04000000, /* Disable MR4 */
	PDEF_DRAMC0_CHA_REG_1E8);
	writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xF7FFFFFF, /* Disable ZQCS */
	PDEF_DRAMC0_CHA_REG_1E4);

	mb(); /* flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) | 0x04000000, /* DCM off 0x1DC[26] = 1; MIOCKCTRLOFF = 1 */
	PDEF_DRAMC0_CHA_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) & 0xFFFFFFFD, /* DCM off 0x1DC[1] = 0; DCMEN2 = 0 */
	PDEF_DRAMC0_CHA_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHA_REG_1DC) & 0xBFFFFFFF, /* DCM off 0x1DC[30] = 0; R_DMPHYCLKDYNGEN=0 */
	PDEF_DRAMC0_CHA_REG_1DC);

	for (i = 0; i < 2; i++) {
		if ((i == 1) && (rank_pasr_segment[i] == 0xFF))
			continue;

		writel(((i << 28) | (rank_pasr_segment[i] << 16) | 0x00000011),
		PDEF_DRAMC0_CHA_REG_088);
		writel(readl(PDEF_DRAMC0_CHA_REG_1E4) | 0x00000001,
		PDEF_DRAMC0_CHA_REG_1E4);

		cnt = 1000;
		do {
			if (cnt-- == 0) {
				pr_warn("[DRAMC0] CHA PASR MRW fail!\n");

				if (release_dram_ctrl() != 0)
					pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
				/* pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n"); */
				local_irq_restore(save_flags);
				return -1; }
			udelay(1);
			} while ((readl(PDEF_DRAMCNAO_CHA_REG_3B8)
			& 0x00000001) == 0x0);

		writel(readl(PDEF_DRAMC0_CHA_REG_1E4) & 0xfffffffe,
		PDEF_DRAMC0_CHA_REG_1E4);
	}

	writel(u4value_1E4, PDEF_DRAMC0_CHA_REG_1E4);
	writel(u4value_1E8, PDEF_DRAMC0_CHA_REG_1E8);
	writel(u4value_1DC, PDEF_DRAMC0_CHA_REG_1DC); /* DCM restore */
	writel(0, PDEF_DRAMC0_CHA_REG_088);

	/*Channel-B*/
	u4value_1E8 = readl(PDEF_DRAMC0_CHB_REG_1E8);
	u4value_1E4 = readl(PDEF_DRAMC0_CHB_REG_1E4);
	u4value_1DC = readl(PDEF_DRAMC0_CHB_REG_1DC); /* DCM backup */
	writel(readl(PDEF_DRAMC0_CHB_REG_1E8) | 0x04000000,
	PDEF_DRAMC0_CHB_REG_1E8);
	writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xF7FFFFFF,
	PDEF_DRAMC0_CHB_REG_1E4);

	mb(); /*flush memory */
	udelay(2);

	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) | 0x04000000, /* DCM off 0x1DC[26] = 1; MIOCKCTRLOFF = 1 */
	PDEF_DRAMC0_CHB_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) & 0xFFFFFFFD, /* DCM off 0x1DC[1] = 0; DCMEN2 = 0 */
	PDEF_DRAMC0_CHB_REG_1DC);
	writel(readl(PDEF_DRAMC0_CHB_REG_1DC) & 0xBFFFFFFF, /* DCM off 0x1DC[30] = 0; R_DMPHYCLKDYNGEN=0 */
	PDEF_DRAMC0_CHB_REG_1DC);

	for (i = 0; i < 2; i++) {
		if ((i == 1) && (rank_pasr_segment[i] == 0xFF))
			continue;
		writel(((i << 28) | (rank_pasr_segment[i] << 16) | 0x00000011),
		PDEF_DRAMC0_CHB_REG_088);
		writel(readl(PDEF_DRAMC0_CHB_REG_1E4) | 0x00000001,
		PDEF_DRAMC0_CHB_REG_1E4);

		cnt = 1000;
		do {
			if (cnt-- == 0) {
				pr_warn("[DRAMC0] CHB PASR MRW fail!\n");

				if (release_dram_ctrl() != 0)
					pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
				/* pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n"); */
				local_irq_restore(save_flags);

				return -1; }
			udelay(1);
			} while ((readl(PDEF_DRAMCNAO_CHB_REG_3B8)
			& 0x00000001) == 0x0);
		writel(readl(PDEF_DRAMC0_CHB_REG_1E4) & 0xfffffffe,
		PDEF_DRAMC0_CHB_REG_1E4);
	}

	writel(u4value_1E4, PDEF_DRAMC0_CHB_REG_1E4);
	writel(u4value_1E8, PDEF_DRAMC0_CHB_REG_1E8);
	writel(u4value_1DC, PDEF_DRAMC0_CHB_REG_1DC); /* DCM restore */
	writel(0, PDEF_DRAMC0_CHB_REG_088);

	if ((segment_rank1 == 0xFF) && (enter_pdp_cnt == 0)) {
		enter_pdp_cnt = 1;
		spm_dpd_init();
	}
	/* pr_warn("[DRAMC0] enter PASR!\n"); */

	if (release_dram_ctrl() != 0)
		pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
	/* pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n"); */
	local_irq_restore(save_flags);
	return 0;
}

int exit_pasr_dpd_config(void)
{
	int ret;
	unsigned char rk1 = 0;
	unsigned long save_flags;

	/*slp_dpd_en(0);*/
	/*slp_pasr_en(0, 0);*/
	if (enter_pdp_cnt == 1) {
		local_irq_save(save_flags);
		if (acquire_dram_ctrl() != 0) {
			pr_warn("[DRAMC0] can NOT get SPM HW SEMAPHORE!\n");
			local_irq_restore(save_flags);
			return -1;
		}
		/* pr_warn("[DRAMC0] get SPM HW SEMAPHORE!\n"); */

		spm_dpd_dram_init();

		if (release_dram_ctrl() != 0)
			pr_warn("[DRAMC0] release SPM HW SEMAPHORE fail!\n");
		/* pr_warn("[DRAMC0] release SPM HW SEMAPHORE success!\n"); */
		local_irq_restore(save_flags);
		rk1 = 0xFF;
	}

	ret = enter_pasr_dpd_config(0, rk1);

	enter_pdp_cnt = 0;

	return ret;
}
#else
int enter_pasr_dpd_config(unsigned char segment_rank0, unsigned char segment_rank1)
{
	return 0;
}

int exit_pasr_dpd_config(void)
{
	return 0;
}
#endif

#define MEM_TEST_SIZE 0x2000
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5
int Binning_DRAM_complex_mem_test(void)
{
	unsigned char *MEM8_BASE;
	unsigned short *MEM16_BASE;
	unsigned int *MEM32_BASE;
	unsigned int *MEM_BASE;
	unsigned long mem_ptr;
	unsigned char pattern8;
	unsigned short pattern16;
	unsigned int i, j, size, pattern32;
	unsigned int value;
	unsigned int len = MEM_TEST_SIZE;
	void *ptr;
	int ret = 1;

	ptr = vmalloc(MEM_TEST_SIZE);

	if (!ptr) {
		/*pr_err("fail to vmalloc\n");*/
		/*ASSERT(0);*/
		ret = -24;
		goto fail;
	}

	MEM8_BASE = (unsigned char *)ptr;
	MEM16_BASE = (unsigned short *)ptr;
	MEM32_BASE = (unsigned int *)ptr;
	MEM_BASE = (unsigned int *)ptr;
	/*pr_warn("Test DRAM start address 0x%lx\n", (unsigned long)ptr);*/
	pr_warn("Test DRAM start address %p\n", ptr);
	pr_warn("Test DRAM SIZE 0x%x\n", MEM_TEST_SIZE);
	size = len >> 2;

	/* === Verify the tied bits (tied high) === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0;

	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0) {
			/* return -1; */
			ret = -1;
			goto fail;
		} else
			MEM32_BASE[i] = 0xffffffff;
	}

	/* === Verify the tied bits (tied low) === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			/* return -2; */
			ret = -2;
			goto fail;
		} else
			MEM32_BASE[i] = 0x00;
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = 0; i < len; i++)
		MEM8_BASE[i] = pattern8++;
	pattern8 = 0x00;
	for (i = 0; i < len; i++) {
		if (MEM8_BASE[i] != pattern8++) {
			/* return -3; */
			ret = -3;
			goto fail;
		}
	}

	/* === Verify pattern 2 (0x00~0xff) === */
	pattern8 = 0x00;
	for (i = j = 0; i < len; i += 2, j++) {
		if (MEM8_BASE[i] == pattern8)
			MEM16_BASE[j] = pattern8;
		if (MEM16_BASE[j] != pattern8) {
			/* return -4; */
			ret = -4;
			goto fail;
		}
		pattern8 += 2;
	}

	/* === Verify pattern 3 (0x00~0xffff) === */
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++)
		MEM16_BASE[i] = pattern16++;
	pattern16 = 0x00;
	for (i = 0; i < (len >> 1); i++) {
		if (MEM16_BASE[i] != pattern16++) {
			/* return -5; */
			ret = -5;
			goto fail;
		}
	}

	/* === Verify pattern 4 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++)
		MEM32_BASE[i] = pattern32++;
	pattern32 = 0x00;
	for (i = 0; i < (len >> 2); i++) {
		if (MEM32_BASE[i] != pattern32++) {
			/* return -6; */
			ret = -6;
			goto fail;
		}
	}

	/* === Pattern 5: Filling memory range with 0x44332211 === */
	for (i = 0; i < size; i++)
		MEM32_BASE[i] = 0x44332211;

	/* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x44332211) {
			/* return -7; */
			ret = -7;
			goto fail;
		} else {
			MEM32_BASE[i] = 0xa5a5a5a5;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 0h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a5a5) {
			/* return -8; */
			ret = -8;
			goto fail;
		} else {
			MEM8_BASE[i * 4] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 2h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a500) {
			/* return -9; */
			ret = -9;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 2] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 1h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa500a500) {
			/* return -10; */
			ret = -10;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 1] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 3h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5000000) {
			/* return -11; */
			ret = -11;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 3] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with ffff
	Word Pattern at offset 1h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x00000000) {
			/* return -12; */
			ret = -12;
			goto fail;
		} else {
			MEM16_BASE[i * 2 + 1] = 0xffff;
		}
	}

	/* === Read Check then Fill Memory with ffff
	Word Pattern at offset 0h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffff0000) {
			/* return -13; */
			ret = -13;
			goto fail;
		} else {
			MEM16_BASE[i * 2] = 0xffff;
		}
	}
    /*===  Read Check === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			/* return -14; */
			ret = -14;
			goto fail;
		}
	}

    /************************************************
    * Additional verification
    ************************************************/
	/* === stage 1 => write 0 === */

	for (i = 0; i < size; i++)
		MEM_BASE[i] = PATTERN1;

	/* === stage 2 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];

		if (value != PATTERN1) {
			/* return -15; */
			ret = -15;
			goto fail;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 3 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			/* return -16; */
			ret = -16;
			goto fail;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 4 => read 0, write 0xF === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			/* return -17; */
			ret = -17;
			goto fail;
		}
		MEM_BASE[i] = PATTERN2;
	}

	/* === stage 5 => read 0xF, write 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN2) {
			/* return -18; */
			ret = -18;
			goto fail;
		}
		MEM_BASE[i] = PATTERN1;
	}

	/* === stage 6 => read 0 === */
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != PATTERN1) {
			/* return -19; */
			ret = -19;
			goto fail;
		}
	}

	/* === 1/2/4-byte combination test === */
	mem_ptr = (unsigned long)MEM_BASE;
	while (mem_ptr < ((unsigned long)MEM_BASE + (size << 2))) {
		*((unsigned char *)mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *)mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *)mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *)mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned char *)mem_ptr) = 0x78;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x56;
		mem_ptr += 1;
		*((unsigned short *)mem_ptr) = 0x1234;
		mem_ptr += 2;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
		*((unsigned short *)mem_ptr) = 0x5678;
		mem_ptr += 2;
		*((unsigned char *)mem_ptr) = 0x34;
		mem_ptr += 1;
		*((unsigned char *)mem_ptr) = 0x12;
		mem_ptr += 1;
		*((unsigned int *)mem_ptr) = 0x12345678;
		mem_ptr += 4;
	}
	for (i = 0; i < size; i++) {
		value = MEM_BASE[i];
		if (value != 0x12345678) {
			/* return -20; */
			ret = -20;
			goto fail;
		}
	}

	/* === Verify pattern 1 (0x00~0xff) === */
	pattern8 = 0x00;
	MEM8_BASE[0] = pattern8;
	for (i = 0; i < size * 4; i++) {
		unsigned char waddr8, raddr8;

		waddr8 = i + 1;
		raddr8 = i;
		if (i < size * 4 - 1)
			MEM8_BASE[waddr8] = pattern8 + 1;
		if (MEM8_BASE[raddr8] != pattern8) {
			/* return -21; */
			ret = -21;
			goto fail;
		}
		pattern8++;
	}

	/* === Verify pattern 2 (0x00~0xffff) === */
	pattern16 = 0x00;
	MEM16_BASE[0] = pattern16;
	for (i = 0; i < size * 2; i++) {
		if (i < size * 2 - 1)
			MEM16_BASE[i + 1] = pattern16 + 1;
		if (MEM16_BASE[i] != pattern16) {
			/* return -22; */
			ret = -22;
			goto fail;
		}
		pattern16++;
	}
	/* === Verify pattern 3 (0x00~0xffffffff) === */
	pattern32 = 0x00;
	MEM32_BASE[0] = pattern32;
	for (i = 0; i < size; i++) {
		if (i < size - 1)
			MEM32_BASE[i + 1] = pattern32 + 1;
		if (MEM32_BASE[i] != pattern32) {
			/* return -23; */
			ret = -23;
			goto fail;
		}
		pattern32++;
	}
	pr_warn("complex R/W mem test pass\n");

fail:
	vfree(ptr);
	return ret;
}

unsigned int ucDram_Register_Read(unsigned int u4reg_addr)
{
	unsigned int pu4reg_value;

	if (check_dramc_base_addr() == -1) {
		pr_err("[DRAMC] Access-R DRAMC base address is NULL!!!\n");
		/* ASSERT(0); */ /* need porting*/
	}

	pu4reg_value = (readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + u4reg_addr)) |
			readl(IOMEM(DDRPHY_BASE_ADDR + u4reg_addr)) |
			readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + u4reg_addr)));

	return pu4reg_value;
}

bool pasr_is_valid(void)
{
	return true;
}

/************************************************
* input parameter:
* freq_sel: 0 -> Current frequency
*           1 -> Highest frequency
*************************************************/
unsigned int get_dram_data_rate_from_reg(int freq_sel)
{
	unsigned char REF_CLK = 52;
	unsigned int u2real_freq = 0;
	unsigned char mpdiv_shu_sel = 0, pll_shu_sel = 0;
	unsigned int u4MPDIV_IN_SEL = 0;
	unsigned char u1MPDIV_Sel = 0;
	unsigned int u4DDRPHY_PLL12_Addr[3] = {0x42c, 0x448, 0x9c4};
	unsigned int u4DDRPHY_PLL1_Addr[3] =  {0x400, 0x640, 0xa40};
	unsigned int u4DDRPHY_PLL3_Addr[3] =  {0x408, 0x644, 0xa44};

	if (readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x028)) & (1<<17))
		return 0;

	if (freq_sel == 0) {	/* current frequency */
		mpdiv_shu_sel = (readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x028))>>8) & 0x7;
		pll_shu_sel = readl(IOMEM(DDRPHY_BASE_ADDR + 0x63c)) & 0x3;
	} else { /* highest frequency */
		mpdiv_shu_sel = 0;
		pll_shu_sel = 0;
	}

	u4MPDIV_IN_SEL =
	readl(IOMEM(DDRPHY_BASE_ADDR +
	(u4DDRPHY_PLL12_Addr[mpdiv_shu_sel]))) & (1 << 17);
	u1MPDIV_Sel =
	(readl(IOMEM(DDRPHY_BASE_ADDR +
	(u4DDRPHY_PLL12_Addr[mpdiv_shu_sel]))) >> 10) & 0x3;

	if (u4MPDIV_IN_SEL == 0) {/* PHYPLL */
		/* FREQ_LCPLL = = FREQ_XTAL*(RG_*_RPHYPLL_FBKDIV+1)
		*2^(RG_*_RPHYPLL_FBKSEL)/2^(RG_*_RPHYPLL_PREDIV) */
		unsigned int u4DDRPHY_PLL1 =
		readl(IOMEM(DDRPHY_BASE_ADDR +
		(u4DDRPHY_PLL1_Addr[0])));
		unsigned int u4FBKDIV = (u4DDRPHY_PLL1>>24) & 0x7F;
		unsigned char u1FBKSEL = (u4DDRPHY_PLL1>>20) & 0x3;
		unsigned char u1PREDIV = (u4DDRPHY_PLL1>>18) & 0x3;

		if (u1PREDIV == 3)
			u1PREDIV = 2;
		u2real_freq = (unsigned int)
		(((u4FBKDIV+1)*REF_CLK)<<u1FBKSEL)>>(u1PREDIV);
		} else {
			/* Freq(VCO clock) = FREQ_XTAL*(RG_RCLRPLL_SDM_PCW)
			/2^(RG_*_RCLRPLL_PREDIV)/2^(RG_*_RCLRPLL_POSDIV) */
			/* Freq(DRAM clock)= Freq(VCO clock)/4 */
			unsigned int u4DDRPHY_PLL3 =
			readl(IOMEM(DDRPHY_BASE_ADDR +
			(u4DDRPHY_PLL3_Addr[pll_shu_sel])));
			unsigned int u4FBKDIV = (u4DDRPHY_PLL3>>24) & 0x7F;
			unsigned char u1PREDIV = (u4DDRPHY_PLL3>>18) & 0x3;
			unsigned char u1POSDIV = (u4DDRPHY_PLL3>>16) & 0x3;
			unsigned char u1FBKSEL =
			(readl(IOMEM(DDRPHY_BASE_ADDR + 0x43c)) >> 12) & 0x1;

			u2real_freq = (unsigned int)
			((((u4FBKDIV)*REF_CLK) << u1FBKSEL >> u1PREDIV)
			>> (u1POSDIV)) >> 2;
		}

	u2real_freq = u2real_freq >> u1MPDIV_Sel;
	/* pr_err("[DRAMC] GetPhyFrequency: %d\n", u2real_freq); */
	return u2real_freq;
}

/************************************************
* input parameter:
* freq_sel: 0 -> Current frequency
*           1 -> Highest frequency
*************************************************/
unsigned int get_dram_data_rate(int freq_sel)
{
	unsigned int MEMPLL_FOUT = 0;

	MEMPLL_FOUT = get_dram_data_rate_from_reg(freq_sel) << 1;

	/* DVFS owner to request provide a spec. frequency,
	not real frequency */
	if (MEMPLL_FOUT == 1820)
		MEMPLL_FOUT = 1866;
	else if (MEMPLL_FOUT == 1716)
		MEMPLL_FOUT = 1700;
	else if (MEMPLL_FOUT == 1560)
		MEMPLL_FOUT = 1600;
	else if (MEMPLL_FOUT == 1586)
		MEMPLL_FOUT = 1600;
	else if (MEMPLL_FOUT == 1274)
		MEMPLL_FOUT = 1270;
	else if (MEMPLL_FOUT == 1040)
		MEMPLL_FOUT = 1066;
	else if (MEMPLL_FOUT == 792)
		MEMPLL_FOUT = 800;
	else if (MEMPLL_FOUT == 3120) {
		pr_err("[DRAMC] MEMPLL_FOUT: %d; ***OLD IC***\n", MEMPLL_FOUT);
		old_IC_No_DummyRead = 1;
		MEMPLL_FOUT = 1600; /* old version,not support DFS */
	}	else if (MEMPLL_FOUT == 3432) {
		pr_err("[DRAMC] MEMPLL_FOUT: %d; ***OLD IC***\n", MEMPLL_FOUT);
		old_IC_No_DummyRead = 1;
		MEMPLL_FOUT = 1700; /* old version,not support DFS */
	}

	return MEMPLL_FOUT;
}

unsigned int read_dram_temperature(unsigned char channel)
{
	unsigned int value = 0;

	if (channel == CHANNEL_A) {
		value =
		(readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x3b8)) >> 8) & 0x7;
	}	else if (channel == CHANNEL_B) {
		value =
		(readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x3b8)) >> 8) & 0x7;
		}

	return value;
}

int get_ddr_type(void)
{
	int DRAM_TYPE = -1;

	/* MT6797 only support LPDDR3*/
		DRAM_TYPE = TYPE_LPDDR3;

	return DRAM_TYPE;

}
int dram_steps_freq(unsigned int step)
{
	int freq = -1;

	switch (step) {
	case 0:
		if (DFS_type == 1)
			freq = 1866;
		else if (DFS_type == 2)
			freq = 1600;
		else if (DFS_type == 3)
			freq = 1700;
		break;
	case 1:
		if (DFS_type == 1)
			freq = 1270;
		else if (DFS_type == 2)
			freq = 1270;
		else if (DFS_type == 3)
			freq = 1270;
		break;
	case 2:
		if (DFS_type == 1)
			freq = 800;
		else if (DFS_type == 2)
			freq = 800;
		else if (DFS_type == 3)
			freq = 800;
		break;
	default:
		return -1;
	}
	return freq;
}

int dram_can_support_fh(void)
{
	if (old_IC_No_DummyRead)
		return 0;
	else
		return 1;
}

#ifdef CONFIG_OF_RESERVED_MEM
int dram_dummy_read_reserve_mem_of_init(struct reserved_mem *rmem)
{
	phys_addr_t rptr = 0;
	unsigned int rsize = 0;

	rptr = rmem->base;
	rsize = (unsigned int)rmem->size;

	if (strstr(DRAM_R0_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE) {
			old_IC_No_DummyRead = 1;
			dramc_rsv_aee_warn("dram dummy read reserve fail on rank0 !!! (rsize:%d)\n", rsize);
			return 0;
		}
		dram_rank0_addr = rptr;
		dram_rank_num++;
		pr_err("[dram_dummy_read_reserve_mem_of_init] dram_rank0_addr = %pa, size = 0x%x\n",
				&dram_rank0_addr, rsize);
	}

	if (strstr(DRAM_R1_DUMMY_READ_RESERVED_KEY, rmem->name)) {
		if (rsize < DRAM_RSV_SIZE) {
			old_IC_No_DummyRead = 1;
			dramc_rsv_aee_warn("dram dummy read reserve fail on rank1 !!! (rsize:%d)\n", rsize);
			return 0;
		}
		dram_rank1_addr = rptr;
		dram_rank_num++;
		pr_err("[dram_dummy_read_reserve_mem_of_init] dram_rank1_addr = %pa, size = 0x%x\n",
				&dram_rank1_addr, rsize);
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(dram_reserve_r0_dummy_read_init,
DRAM_R0_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
RESERVEDMEM_OF_DECLARE(dram_reserve_r1_dummy_read_init,
DRAM_R1_DUMMY_READ_RESERVED_KEY,
			dram_dummy_read_reserve_mem_of_init);
#endif
static ssize_t complex_mem_test_show(struct device_driver *driver, char *buf)
{
	int ret;

	ret = Binning_DRAM_complex_mem_test();
	if (ret > 0)
		return snprintf(buf, PAGE_SIZE, "MEM Test all pass\n");
	else
		return snprintf(buf, PAGE_SIZE, "MEM TEST failed %d\n", ret);
}

static ssize_t complex_mem_test_store(struct device_driver *driver,
const char *buf, size_t count)
{
	/*snprintf(buf, "do nothing\n");*/
	return count;
}

#if 0
#ifdef READ_DRAM_TEMP_TEST
static ssize_t read_dram_temp_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM MR4 = 0x%x\n",
	read_dram_temperature());
}
static ssize_t read_dram_temp_store(struct device_driver *driver,
const char *buf, size_t count)
{
	return count;
}
#endif
#endif
static ssize_t read_dram_data_rate_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM data rate = %d\n",
	get_dram_data_rate(0));
}

static ssize_t read_dram_data_rate_store(struct device_driver *driver,
const char *buf, size_t count)
{
	return count;
}


DRIVER_ATTR(emi_clk_mem_test, 0664,
complex_mem_test_show, complex_mem_test_store);

#if 0
#ifdef APDMA_TEST
DRIVER_ATTR(dram_dummy_read_test, 0664,
DFS_APDMA_TEST_show, DFS_APDMA_TEST_store);
#endif
#endif

#ifdef READ_DRAM_TEMP_TEST
DRIVER_ATTR(read_dram_temp_test, 0664,
read_dram_temp_show, read_dram_temp_store);
#endif

DRIVER_ATTR(read_dram_data_rate, 0664,
read_dram_data_rate_show, read_dram_data_rate_store);

/*DRIVER_ATTR(dram_dfs, 0664, dram_dfs_show, dram_dfs_store);*/

static int dram_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_debug("[DRAMC0] module probe.\n");

	return ret;
}

static int dram_remove(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dram_of_ids[] = {
	{.compatible = "mediatek,dramc",},
	{}
};
#endif

static struct platform_driver dram_test_drv = {
	.probe = dram_probe,
	.remove = dram_remove,
	.driver = {
		.name = "emi_clk_test",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = dram_of_ids,
#endif
		},
};

static int dram_dt_init(void)
{
	int ret = 0;
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc");
	if (node) {
		DRAMCAO_CHA_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCAO_CHA_BASE_ADDR @ %p\n",
		DRAMCAO_CHA_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMC0 CHA compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_conf_b");
	if (node) {
		DRAMCAO_CHB_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCAO_CHB_BASE_ADDR @ %p\n",
		DRAMCAO_CHB_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMC0 CHB compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,ddrphy");
	if (node) {
		DDRPHY_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DDRPHY_BASE_ADDR @ %p\n", DDRPHY_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DDRPHY compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_nao");
	if (node) {
		DRAMCNAO_CHA_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCNAO_CHA_BASE_ADDR @ %p\n",
		DRAMCNAO_CHA_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMCNAO CHA compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_b_nao");
	if (node) {
		DRAMCNAO_CHB_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCNAO_CHB_BASE_ADDR @ %p\n",
		DRAMCNAO_CHB_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMCNAO CHB compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	if (node) {
		TOPCKGEN_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get TOPCKGEN_BASE_ADDR @ %p\n",
		TOPCKGEN_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find TOPCKGEN compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
	if (node) {
		INFRACFG_AO_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get INFRACFG_AO_BASE_ADDR @ %p\n",
		INFRACFG_AO_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find INFRACFG_AO compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (node) {
		SLEEP_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get SLEEP_BASE_ADDR @ %p\n",
		INFRACFG_AO_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find SLEEP_BASE_ADDR compatible node\n");
		return -1;
	}

	if (of_scan_flat_dt(dt_scan_dram_info, NULL) > 0) {
		pr_err("[DRAMC]find dt_scan_dram_info\n");
	} else {
		pr_err("[DRAMC]can't find dt_scan_dram_info\n");
		return -1;
	}

	return ret;
}

/* int __init dram_test_init(void) */
static int __init dram_test_init(void)
{
	int ret = 0;

	ret = dram_dt_init();
	if (ret) {
		pr_warn("[DRAMC] Device Tree Init Fail\n");
		return ret;
	}

	ret = platform_driver_register(&dram_test_drv);
	if (ret) {
		pr_warn("fail to create dram_test platform driver\n");
		return ret;
	}

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_emi_clk_mem_test);
	if (ret) {
		pr_warn("fail to create the emi_clk_mem_test sysfs files\n");
		return ret;
	}

#if 0
#ifdef APDMA_TEST
	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_dram_dummy_read_test);
	if (ret) {
		pr_warn("fail to create the DFS sysfs files\n");
		return ret;
	}
#endif

#ifdef READ_DRAM_TEMP_TEST
	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_read_dram_temp_test);
	if (ret) {
		pr_warn("fail to create the read dram temp sysfs files\n");
		return ret;
	}
#endif
#endif

	ret = driver_create_file(&dram_test_drv.driver,
	&driver_attr_read_dram_data_rate);
	if (ret) {
		pr_warn("fail to create the read dram data rate sysfs files\n");
		return ret;
	}

	highest_dram_data_rate = get_dram_data_rate(1);
	if (highest_dram_data_rate == 1866)
		DFS_type = 1;
	else if (highest_dram_data_rate == 1700)
		DFS_type = 3;
	else if (highest_dram_data_rate == 1600)
		DFS_type = 2;
	pr_err("[DRAMC Driver] Highest Dram Data Rate = %d;  DFS_type = %d\n",
	highest_dram_data_rate, DFS_type);

	if (dram_can_support_fh())
		pr_err("[DRAMC Driver] dram can support DFS\n");
	else
		pr_err("[DRAMC Driver] dram can not support DFS\n");

	return ret;
}

static void __exit dram_test_exit(void)
{
	platform_driver_unregister(&dram_test_drv);
}

postcore_initcall(dram_test_init);
module_exit(dram_test_exit);

MODULE_DESCRIPTION("MediaTek DRAMC Driver v0.1");
