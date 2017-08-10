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

void __iomem *DRAMCAO_BASE_ADDR;
void __iomem *DDRPHY_BASE_ADDR;
void __iomem *DRAMCNAO_BASE_ADDR;
void __iomem *TOPCKGEN_BASE_ADDR;

volatile unsigned int dst_dummy_read_addr[2];
volatile unsigned int src_dummy_read_addr[2];
int get_addr_done = 0;
volatile unsigned char *dst_array_v;
volatile unsigned char *src_array_v;
volatile unsigned int dst_array_p;
volatile unsigned int src_array_p;
int init_done = 0;
static char dfs_dummy_buffer[BUFF_LEN] __aligned(PAGE_SIZE);
static DEFINE_MUTEX(dram_dfs_mutex);
int org_dram_data_rate = 0;
static unsigned int enter_pdp_cnt;
/*extern bool spm_vcorefs_is_dvfs_in_porgress(void);*/
#define Reg_Sync_Writel(addr, val)   writel(val, IOMEM(addr))
#define Reg_Readl(addr) readl(IOMEM(addr))


const struct dram_info *g_dram_info_dummy_read = NULL;

static int __init dt_scan_dram_info(unsigned long node, const char *uname, int depth, void *data)
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

	reg = (const __be32 *)of_get_flat_dt_prop(node, (const char *)"reg", (int *)&l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));
	if (node) {
		/* orig_dram_info */
		g_dram_info_dummy_read = (const struct dram_info *)of_get_flat_dt_prop(node, "orig_dram_info", NULL);
	}

	pr_debug("[DRAMC] dram info dram rank number = %d\n", g_dram_info_dummy_read->rank_num);
	pr_debug("[DRAMC] dram info dram rank0 base = 0x%llx\n", g_dram_info_dummy_read->rank_info[0].start);
	pr_debug("[DRAMC] dram info dram rank1 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start + g_dram_info_dummy_read->rank_info[0].size);

	return node;
}

static int check_dramc_base_addr(void)
{
	if ((!DRAMCAO_BASE_ADDR) || (!DDRPHY_BASE_ADDR) || (!DRAMCNAO_BASE_ADDR))
		return -1;
	else
		return 0;
}

void *mt_dramc_base_get(void)
{
	return DRAMCAO_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_base_get);

void *mt_dramc_nao_base_get(void)
{
	return DRAMCNAO_BASE_ADDR;
}
EXPORT_SYMBOL(mt_dramc_nao_base_get);

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
	pmic_config_interface(Vcore_REG_SW, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(Vcore_REG_HW, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(Vdram_REG, Vdram_HV, 0x7F, 0);
	pr_err("[HQA]Set HVcore1 setting: Vcore1=1.10V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.30V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(Vcore_REG_SW), upmu_get_reg_value(Vcore_REG_HW),
		Vcore1_HV, upmu_get_reg_value(Vdram_REG), Vdram_HV);
#endif

#ifdef NV	/*Vcore1=1.00V, Vdram=1.22V,  Vio18=1.8*/
	pmic_config_interface(Vcore_REG_SW, Vcore1_NV, 0x7F, 0);
	pmic_config_interface(Vcore_REG_HW, Vcore1_NV, 0x7F, 0);
	pmic_config_interface(Vdram_REG, Vdram_NV, 0x7F, 0);
	pr_err("[HQA]Set NV setting: Vcore1=1.00V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.22V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(Vcore_REG_SW), upmu_get_reg_value(Vcore_REG_HW),
		Vcore1_NV, upmu_get_reg_value(Vdram_REG), Vdram_NV);
#endif

#ifdef LVcore1	/*Vcore1=0.90V, Vdram=1.16V,  Vio18=1.8*/
	pmic_config_interface(Vcore_REG_SW, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(Vcore_REG_HW, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(Vdram_REG, Vdram_LV, 0x7F, 0);
	pr_err("[HQA]Set LVcore1 setting: Vcore1=0.90V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.16V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(Vcore_REG_SW), upmu_get_reg_value(Vcore_REG_HW),
		Vcore1_LV, upmu_get_reg_value(Vdram_REG), Vdram_LV);
#endif

#ifdef HVcore1_LVdram	/*Vcore1=1.10V, Vdram=1.16V,  Vio18=1.8*/
	pmic_config_interface(Vcore_REG_SW, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(Vcore_REG_HW, Vcore1_HV, 0x7F, 0);
	pmic_config_interface(Vdram_REG, Vdram_LV, 0x7F, 0);
	pr_err("[HQA]Set HVcore1_LVdram setting: Vcore1=1.10V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.16V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(Vcore_REG_SW), upmu_get_reg_value(Vcore_REG_HW),
		Vcore1_HV, upmu_get_reg_value(Vdram_REG), Vdram_LV);
#endif

#ifdef LVcore1_HVdram	/*Vcore1=0.90V, Vdram=1.30V,  Vio18=1.8*/
	pmic_config_interface(Vcore_REG_SW, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(Vcore_REG_HW, Vcore1_LV, 0x7F, 0);
	pmic_config_interface(Vdram_REG, Vdram_HV, 0x7F, 0);
	pr_err("[HQA]Set LVcore1_HVdram setting: Vcore1=0.90V(SW_Ctrl=0x%x, HW_Ctrl=0x%x, should be 0x%x), Vdram=1.30V(0x%x, should be 0x%x)\n",
		upmu_get_reg_value(Vcore_REG_SW), upmu_get_reg_value(Vcore_REG_HW),
		Vcore1_LV, upmu_get_reg_value(Vdram_REG), Vdram_HV);
#endif
}
#endif

void spm_dpd_init(void)
{
	/* backup mr4, zqcs config for future restore*/
	unsigned int recover7_0;
	unsigned int recover8;
	unsigned int recover;

	recover7_0 = readl(PDEF_DRAMC0_REG_1E4) & 0x00ff0000;
	recover8   = readl(PDEF_DRAMC0_REG_1DC) & 0x00000001;
	recover = recover7_0 | recover8;

	spm_set_register(SPM_PASR_DPD_0, recover);

	/*-----try, disable MR4(0x1E8[26]=1)*/
	writel(readl(PDEF_DRAMC0_REG_1E8) | 0x04000000, PDEF_DRAMC0_REG_1E8);

	/*Set ZQCSCNT7~0(0x1e4[23:16]) = 0: disable ZQCS*/
	/*When doing ZQCS, special_command_enable will wakeup RANK1's CKE. This is wrong.*/
	writel(readl(PDEF_DRAMC0_REG_1E4) & 0xff00ffff, PDEF_DRAMC0_REG_1E4);

	/*20150402 add, ZQCSCNT8 is added in Jade*/
	/* set R_DMZQCSCNT8(0x1DC[0])=0*/
	writel(readl(PDEF_DRAMC0_REG_1DC) & 0xfffffffe, PDEF_DRAMC0_REG_1DC);
	/*
	*MDM_TM_WAIT_US = 2;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(2);

	/*20150319 add, set MIOCKCTRLOFF(0x1dc[26])=1: not stop to DRAM clock!*/
	writel(readl(PDEF_DRAMC0_REG_1DC) | 0x04000000, PDEF_DRAMC0_REG_1DC);

	/*20150319 add, wait 100ns*/
	/*
	*MDM_TM_WAIT_US = 1;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(1);

	/*Set CKE2RANK(0x1ec[20])=0: let CKE1 and CKE0 independent*/
	writel(readl(PDEF_DRAMC0_REG_1EC) & 0xffefffff, PDEF_DRAMC0_REG_1EC);

	/*20150319 add, set R_DMCKE1FIXON(0xF4[20])=1*/
	writel(readl(PDEF_DRAMC0_REG_0F4) | 0x00100000, PDEF_DRAMC0_REG_0F4);

	/*SW set RK1SRF(0x110[21])=0: RK1 can not enter SRF*/
	writel(readl(PDEF_DRAMC0_REG_110) & 0xffdfffff, PDEF_DRAMC0_REG_110);

	/*Set DISDMOEDIS(0x1ec[16])=1: CA can not be floating because RK1 want to DPD after RK0 enter SRF.*/
	writel(readl(PDEF_DRAMC0_REG_1EC) | 0x00010000, PDEF_DRAMC0_REG_1EC);

}

void spm_dpd_dram_init(void) /*void spm_dpd_dram_init_1(void)*/
{
	unsigned int recover7_0;
	unsigned int recover8;

	/*20150319 add, recover, set R_DMCKE1FIXOFF(0xF4[21])=0*/
	writel(readl(PDEF_DRAMC0_REG_0F4) & 0xffdfffff, PDEF_DRAMC0_REG_0F4);

	/* DPD DRAM initialization first part - Exit PDP*/

	/*SW set RK1DPDX(0x110[31])=1*/
	writel(readl(PDEF_DRAMC0_REG_110) | 0x80000000, PDEF_DRAMC0_REG_110);

	/*20150325 */
	/*wait100ns*/
	/*
	*MDM_TM_WAIT_US = 1;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(1);

	/*SW set RK1SRF(0x110[21])=1*/
	writel(readl(PDEF_DRAMC0_REG_110) | 0x00200000, PDEF_DRAMC0_REG_110);

	/* 5. wait 220us*/
	/*SW delay 220us*/
	/*
	*MDM_TM_WAIT_US = 220;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(220);

	/* 6.execute spm_dpd_init_2()*/

	/*fix CKE, R_DMCKEFIXON*/
	writel(readl(PDEF_DRAMC0_REG_0E4) | 0x00000004, PDEF_DRAMC0_REG_0E4);

	/*move here*/
	/*SW recover CKE2RANK(0x1EC[20]) -> CKE1 is the same as CKE0*/
	writel(readl(PDEF_DRAMC0_REG_1EC) | 0x00100000, PDEF_DRAMC0_REG_1EC);

	/*20150325 add*/
	/*set R_DMCKE1FIXON(0xF4[20])=1*/
	writel(readl(PDEF_DRAMC0_REG_0F4) | 0x00100000, PDEF_DRAMC0_REG_0F4);

	/*set R_DMPREALL_OPTION(0x138[15])=0*/
	writel(readl(PDEF_DRAMC0_REG_138) & 0xffff7fff, PDEF_DRAMC0_REG_138);

	/*set R_DMDCMEN2(0x1DC[1])=0*/
	writel(readl(PDEF_DRAMC0_REG_1DC) & 0xfffffffd, PDEF_DRAMC0_REG_1DC);

	/*Set MRS value - for rank1, Reset(0x3F)*/
	writel(0x1000003f, PDEF_DRAMC0_REG_088);
	/*Set MRWEN=1 (0x1E4[0]) -> Mode register write command enable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) | 0x00000001, PDEF_DRAMC0_REG_1E4);
	/*SW delay 10us*/
	/*
	*MDM_TM_WAIT_US = 10;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(10);
	/*Set MRWEN=0 (0x1E4[0]) -> Mode register write command disable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) & 0xfffffffe, PDEF_DRAMC0_REG_1E4);

	/*Set MRS value - for rank1, Calibration command after initialization (0x0A)*/
	writel(0x10ff000a, PDEF_DRAMC0_REG_088);
	/*Set MRWEN=1 (0x1E4[0]) -> Mode register write command enable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) | 0x00000001, PDEF_DRAMC0_REG_1E4);
	/*SW delay 1.2us*/
	/*
	*MDM_TM_WAIT_US = 1;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(2);

	/*Set MRWEN=0 (0x1E4[0]) -> Mode register write command disable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) & 0xfffffffe, PDEF_DRAMC0_REG_1E4);

	/*Set MRS value - for rank1, I/O Config-1(0x03) - 40-Ohm typical pull-down/pull-up*/
	writel(0x10010003, PDEF_DRAMC0_REG_088);
	/*Set MRWEN=1 (0x1E4[0]) -> Mode register write command enable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) | 0x00000001, PDEF_DRAMC0_REG_1E4);
	/*SW delay 1.2us*/
	/*
	*MDM_TM_WAIT_US = 1;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(2);

	/*Set MRWEN=0 (0x1E4[0]) -> Mode register write command disable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) & 0xfffffffe, PDEF_DRAMC0_REG_1E4);

	/*Set MRS value - for rank1, Device Feature1(0x01) - BL=8, nWR=8*/
	writel(0x10830001, PDEF_DRAMC0_REG_088);
	/*Set MRWEN=1 (0x1E4[0]) -> Mode register write command enable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) | 0x00000001, PDEF_DRAMC0_REG_1E4);
	/*SW delay 1.2us*/
	/*
	*MDM_TM_WAIT_US = 1;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(2);

	/*Set MRWEN=0 (0x1E4[0]) -> Mode register write command disable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) & 0xfffffffe, PDEF_DRAMC0_REG_1E4);

	/*Set MRS value - for rank1, Device Feature 2(0x02) - RL=12 / WL=9 (<= 800 MHz)*/
	writel(0x101c0002, PDEF_DRAMC0_REG_088);
	/*Set MRWEN=1 (0x1E4[0]) -> Mode register write command enable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) | 0x00000001, PDEF_DRAMC0_REG_1E4);
	/*SW delay 1.2us*/
	/*
	*MDM_TM_WAIT_US = 1;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(2);

	/*Set MRWEN=0 (0x1E4[0]) -> Mode register write command disable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) & 0xfffffffe, PDEF_DRAMC0_REG_1E4);

	/*Set MRS value - for rank1, Device Feature11(0x0b) - disable ODT*/
	writel(0x1000000b, PDEF_DRAMC0_REG_088);
	/*Set MRWEN=1 (0x1E4[0]) -> Mode register write command enable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) | 0x00000001, PDEF_DRAMC0_REG_1E4);
	/*SW delay 1.2us*/
	/*
	*MDM_TM_WAIT_US = 1;
	while (*MDM_TM_WAIT_US > 0)
		;
	*/
	mb();
	udelay(2);

	/*Set MRWEN=0 (0x1E4[0]) -> Mode register write command disable*/
	writel(readl(PDEF_DRAMC0_REG_1E4) & 0xfffffffe, PDEF_DRAMC0_REG_1E4);

	/*20150325 add*/
	/* set R_DMDCMEN2(0x1DC[1])=1*/
	writel(readl(PDEF_DRAMC0_REG_1DC) | 0x00000002, PDEF_DRAMC0_REG_1DC);

	/* set R_DMPREALL_OPTION(0x138[15])=1*/  /*recover*/
	writel(readl(PDEF_DRAMC0_REG_138) | 0x00008000, PDEF_DRAMC0_REG_138);

	/* set R_DMCKE1FIXON(0xF4[20])=0*/
	writel(readl(PDEF_DRAMC0_REG_0F4) & 0xffefffff, PDEF_DRAMC0_REG_0F4);

	/*SW set RK1DPDX(0x110[31])=0*/
	writel(readl(PDEF_DRAMC0_REG_110) & 0x7fffffff, PDEF_DRAMC0_REG_110);

	/*release fix CKE*/
	writel(readl(PDEF_DRAMC0_REG_0E4) & 0xfffffffb, PDEF_DRAMC0_REG_0E4);

	recover7_0 = spm_get_register(SPM_PASR_DPD_0) & 0x00ff0000;
	recover8   = spm_get_register(SPM_PASR_DPD_0) & 0x00000001;

	/*SW recover ZQCSCNT7~0(0x1E4[23:16]) to allow ZQCS*/
	writel(readl(PDEF_DRAMC0_REG_1DC) | recover8, PDEF_DRAMC0_REG_1DC);
	/*20150402 add, ZQCSCNT8 is added in Jade*/
	/* set R_DMZQCSCNT8(0x1DC[0])=? */ /*recover the original value*/
	writel(readl(PDEF_DRAMC0_REG_1E4) | recover7_0, PDEF_DRAMC0_REG_1E4);

	/*-----try, release disable MR4(0x1E8[26]=0)*/
	writel(readl(PDEF_DRAMC0_REG_1E8) & 0xfbffffff, PDEF_DRAMC0_REG_1E8);

	/*Recover MRSRK(0x88[28])=0, MRRRK(0x88[26])=0*/
	writel(0x00000000, PDEF_DRAMC0_REG_088);
}

int enter_pasr_dpd_config(unsigned char segment_rank0,
			   unsigned char segment_rank1)
{
	/* for D-3, support run time MRW */
	unsigned int rank_pasr_segment[2];
	unsigned int dramc0_spcmd, dramc0_pd_ctrl, dramc0_pd_ctrl_2, dramc0_padctl4, dramc0_1E8;
	unsigned int i, cnt = 1000;

	rank_pasr_segment[0] = segment_rank0 & 0xFF;	/* for rank0 */
	rank_pasr_segment[1] = segment_rank1 & 0xFF;	/* for rank1 */
	pr_warn("[DRAMC0] PASR r0 = 0x%x  r1 = 0x%x\n", rank_pasr_segment[0], rank_pasr_segment[1]);

	/* backup original data */
	dramc0_spcmd = readl(PDEF_DRAMC0_REG_1E4);
	dramc0_pd_ctrl = readl(PDEF_DRAMC0_REG_1DC);
	dramc0_padctl4 = readl(PDEF_DRAMC0_REG_0E4);
	dramc0_1E8 = readl(PDEF_DRAMC0_REG_1E8);

	/* disable MR4(0x1E8[26]=1) */
	writel(dramc0_1E8 | 0x04000000, PDEF_DRAMC0_REG_1E8);

	/* Set ZQCSCNT7~0(0x1e4[23:16]) = 0 ZQCSCNT8(0x1DC[0]) = 0: disable ZQCS */
	writel(dramc0_spcmd & 0xff00ffff, PDEF_DRAMC0_REG_1E4);
	writel(dramc0_pd_ctrl & 0xfffffffe, PDEF_DRAMC0_REG_1DC);

	/* Set MIOCKCTRLOFF(0x1dc[26])=1: not stop to DRAM clock! */
	dramc0_pd_ctrl_2 = readl(PDEF_DRAMC0_REG_1DC);
	writel(dramc0_pd_ctrl_2 | 0x04000000, PDEF_DRAMC0_REG_1DC);

	/* fix CKE */
	writel(dramc0_padctl4 | 0x00000004, PDEF_DRAMC0_REG_0E4);

	udelay(1);

	for (i = 0; i < 2; i++) {

		if ((i == 1) && (rank_pasr_segment[i] == 0xFF))
			continue;

		/* set MRS settings include rank number, segment information and MRR17 */
		writel(((i << 28) | (rank_pasr_segment[i] << 16) | 0x00000011), PDEF_DRAMC0_REG_088);
		/* Mode register write command enable */
		writel(0x00000001, PDEF_DRAMC0_REG_1E4);

		/* wait MRW command response */
		/* wait >1us */
		/* gpt_busy_wait_us(1); */
		do {
			if (cnt-- == 0) {
				pr_warn("[DRAMC0] PASR MRW fail!\n");
				return -1;
			}
			udelay(1);
		} while ((readl(PDEF_DRAMC0_REG_3B8) & 0x00000001) == 0x0);
		mb();

		/* Mode register write command disable */
		writel(0x0, PDEF_DRAMC0_REG_1E4);
	}

	/* release MR4 */
	writel(dramc0_1E8, PDEF_DRAMC0_REG_1E8);

	/* release fix CKE */
	writel(dramc0_padctl4, PDEF_DRAMC0_REG_0E4);

	/* recover Set MIOCKCTRLOFF(0x1dc[26]) */
	/* Set MIOCKCTRLOFF(0x1DC[26])=0: stop to DRAM clock */
	writel(dramc0_pd_ctrl, PDEF_DRAMC0_REG_1DC);

	/* writel(0x00000004, PDEF_DRAMC0_REG_088); */
	pr_warn("[DRAMC0] PASR offset 0x88 = 0x%x\n", readl(PDEF_DRAMC0_REG_088));
	writel(dramc0_spcmd, PDEF_DRAMC0_REG_1E4);

	if (segment_rank1 == 0xFF) {	/*all segments of rank1 are not reserved -> rank1 enter DPD*/
		enter_pdp_cnt++;
		spm_dpd_init();
	}

	return 0;

	/*slp_pasr_en(1, segment_rank0 | (segment_rank1 << 8));*/
}

int exit_pasr_dpd_config(void)
{
	int ret;
	unsigned char rk1 = 0;

	/*slp_dpd_en(0);*/
	/*slp_pasr_en(0, 0);*/
	if (enter_pdp_cnt == 1) {
		enter_pdp_cnt--;
		spm_dpd_dram_init();
		rk1 = 0xFF;
	}

	ret = enter_pasr_dpd_config(0, rk1);

	return ret;
}

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
			vfree(ptr);
			/* return -1; */
			ret = -1;
			goto fail;
		} else
			MEM32_BASE[i] = 0xffffffff;
	}

	/* === Verify the tied bits (tied low) === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffffffff) {
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
			/* return -7; */
			ret = -7;
			goto fail;
		} else {
			MEM32_BASE[i] = 0xa5a5a5a5;
		}
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a5a5) {
			vfree(ptr);
			/* return -8; */
			ret = -8;
			goto fail;
		} else {
			MEM8_BASE[i * 4] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5a5a500) {
			vfree(ptr);
			/* return -9; */
			ret = -9;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 2] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa500a500) {
			vfree(ptr);
			/* return -10; */
			ret = -10;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 1] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xa5000000) {
			vfree(ptr);
			/* return -11; */
			ret = -11;
			goto fail;
		} else {
			MEM8_BASE[i * 4 + 3] = 0x00;
		}
	}

	/* === Read Check then Fill Memory with ffff Word Pattern at offset 1h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0x00000000) {
			vfree(ptr);
			/* return -12; */
			ret = -12;
			goto fail;
		} else {
			MEM16_BASE[i * 2 + 1] = 0xffff;
		}
	}

	/* === Read Check then Fill Memory with ffff Word Pattern at offset 0h == */
	for (i = 0; i < size; i++) {
		if (MEM32_BASE[i] != 0xffff0000) {
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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
			vfree(ptr);
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

	pu4reg_value = (readl(IOMEM(DRAMCAO_BASE_ADDR + u4reg_addr)) |
			readl(IOMEM(DDRPHY_BASE_ADDR + u4reg_addr)) |
			readl(IOMEM(DRAMCNAO_BASE_ADDR + u4reg_addr)));

	return pu4reg_value;
}

void ucDram_Register_Write(unsigned int u4reg_addr, unsigned int u4reg_value)
{
	if (check_dramc_base_addr() == -1) {
		pr_err("[DRAMC] Access-W DRAMC base address is NULL!!!\n");
		/* ASSERT(0); */ /* need porting*/
	}

	writel(u4reg_value, IOMEM(DRAMCAO_BASE_ADDR + u4reg_addr));
	writel(u4reg_value, IOMEM(DDRPHY_BASE_ADDR + u4reg_addr));
	writel(u4reg_value, IOMEM(DRAMCNAO_BASE_ADDR + u4reg_addr));
	mb();
}

bool pasr_is_valid(void)
{
	return true;
}

/*-------------------------------------------------------------------------
** Round_Operation
 *  Round operation of A/B
 *  @param  A
 *  @param  B
 *  @retval round(A/B)
 *
-------------------------------------------------------------------------*/
unsigned int Round_Operation(unsigned int A, unsigned int B)
{
	unsigned int temp;

	if (B == 0)
		return 0xffffffff;

	temp = A/B;
	if ((A - temp*B) >= ((temp + 1)*B - A))
		return temp+1;
	else
		return temp;
}


unsigned int get_dram_data_rate_from_reg(void)
{
	unsigned int u4value1;
	unsigned int MEMPLL_N_INFO, MEMPLL_DIV;
	unsigned int MEMPLLx_FBDIV, MEMPLLx_M4PDIV;
	unsigned int onepll_fout, threepll_fout;

	u4value1 = ucDram_Register_Read(0x600);
	MEMPLL_N_INFO = (u4value1 & 0x7fffffff) >> 0;

	u4value1 = ucDram_Register_Read(0x610);
	MEMPLL_DIV = (u4value1 & 0x00fe0000) >> 17;

	u4value1 = ucDram_Register_Read(0x62C);
	MEMPLLx_FBDIV = (u4value1 & 0x007f0000) >> 16;

	u4value1 = ucDram_Register_Read(0x630);
	MEMPLLx_M4PDIV = (u4value1 & 0x30000000) >> 28;

	if (MEMPLLx_M4PDIV == 0)
		MEMPLLx_M4PDIV = 2;
	else if (MEMPLLx_M4PDIV == 1)
		MEMPLLx_M4PDIV = 4;
	else if (MEMPLLx_M4PDIV == 2)
		MEMPLLx_M4PDIV = 8;

	onepll_fout = ((26*(MEMPLL_N_INFO>>24))+(26*(MEMPLL_N_INFO&0x00ffffff)>>24))/4;
	threepll_fout = (onepll_fout*4*MEMPLLx_M4PDIV*(MEMPLLx_FBDIV+1))/MEMPLL_DIV;

	if ((onepll_fout < (DUAL_FREQ_LOW + DATA_RATE_THRESHOLD)) &&
		(onepll_fout > (DUAL_FREQ_LOW - DATA_RATE_THRESHOLD)))
		onepll_fout = DUAL_FREQ_LOW;

	/* for threepll calculation result would not be exactly 1800, but we should return exactly 1800 to DVFS driver*/
	if ((threepll_fout < (DUAL_FREQ_HIGH + DATA_RATE_THRESHOLD)) &&
		(threepll_fout > (DUAL_FREQ_HIGH - DATA_RATE_THRESHOLD)))
		threepll_fout = DUAL_FREQ_HIGH;

	/*pr_err("[DRAMC] onepll_fout=%d, threepll_fout=%d\n", onepll_fout, threepll_fout);*/
	u4value1 = (readl(_CLK_CFG_0_SET) >> 8) & 0x00000003;

	if (u4value1 == 1)
		return threepll_fout;
	else
		return onepll_fout;
}

unsigned int get_dram_data_rate(void)
{
	unsigned int SPM_ACTIVE = 0;
	unsigned int MEMPLL_FOUT = 0;

	/*6755 TBD*/
	/*SPM_ACTIVE = spm_vcorefs_is_dvfs_in_porgress()*/
	while (SPM_ACTIVE == 1)
		;
	MEMPLL_FOUT = get_dram_data_rate_from_reg() << 1;
	return MEMPLL_FOUT;
}

#if 0
unsigned int DRAM_MRR(int MRR_num)
{
	unsigned int MRR_value = 0x0;
	unsigned int u4value;

	/* set DQ bit 0, 1, 2, 3, 4, 5, 6, 7 pinmux for LPDDR3*/
	/*ucDram_Register_Write(DRAMC_REG_RRRATE_CTL, 0x13121110);
	ucDram_Register_Write(DRAMC_REG_MRR_CTL, 0x17161514);*/
	ucDram_Register_Write(DRAMC_REG_RRRATE_CTL, 0x15111012);
	ucDram_Register_Write(DRAMC_REG_MRR_CTL, 0x16231314);

	ucDram_Register_Write(DRAMC_REG_MRS, MRR_num);
	ucDram_Register_Write(DRAMC_REG_SPCMD, 0x00000002);
	udelay(1);
	ucDram_Register_Write(DRAMC_REG_SPCMD, 0x00000000);
	udelay(1);

	u4value = ucDram_Register_Read(DRAMC_REG_SPCMDRESP);
	MRR_value = (u4value >> 20) & 0xFF;

	return MRR_value;
}


unsigned int read_dram_temperature(void)
{
	unsigned int value;

	value = DRAM_MRR(4) & 0x7;
	return value;
}
#endif

int get_ddr_type(void)
{
	unsigned int value;
	int DRAM_TYPE = -1;
#if 0
	value =  ucDram_Register_Read(DRAMC_LPDDR2);
	if ((value >> 28) & 0x1) { /*check LPDDR2_EN*/
		DRAM_TYPE = TYPE_LPDDR2;
	}

	value = ucDram_Register_Read(DRAMC_PADCTL4);
	if ((value >> 7) & 0x1) { /*check DDR3_EN*/
		DRAM_TYPE = TYPE_PCDDR3;
	}
#endif
	/* Jade support LPDDR3 only*/
	value = ucDram_Register_Read(DRAMC_ACTIM1);
	if ((value >> 28) & 0x1) { /*check LPDDR3_EN*/
		DRAM_TYPE = TYPE_LPDDR3;
	}

	return DRAM_TYPE;

}
int dram_steps_freq(unsigned int step)
{
	int freq;

	switch (step) {
	case 0:
		freq = 1664;/*freq = 1800;*/
		break;
	case 1:
		freq = 1300;
		break;
	default:
		return -1;
	}
	return freq;
}

/*dram_can_support_fh() : need revise after HQA ,
and need to check if there a empty bit in dramc_reg (0xf4)*/
int dram_can_support_fh(void)
{
	unsigned int value;

	/*need to check if there empty bit in reg(0xf4) */
	/*value = ucDram_Register_Read(0xf4);*/
	value = (0x1 << 15);/*this is a temp value*/
	/*pr_debug("dummy regs 0x%x\n", value);
	pr_debug("check 0x%x\n",(0x1 <<15));*/

	if (value & (0x1 << 15))
		return 1;
	else
		return 0;
}

int dram_get_dummy_read_addr(void)
{

#if 1/* get addr by ori method */
		DFS_APDMA_early_init();
		src_dummy_read_addr[0] =  src_array_p;
		dst_dummy_read_addr[0] = dst_array_p;
		src_dummy_read_addr[1] = 0;
		dst_dummy_read_addr[1] = 0;
#else/*get addr by new method from dts tree ,which is generated by LK */




#endif
	return 1;

}
int DFS_APDMA_early_init(void)
{
	phys_addr_t max_dram_size = get_max_DRAM_size();
	phys_addr_t dummy_read_center_address = 0;

	if (init_done == 0) {
		if (max_dram_size == 0x100000000ULL)/*dram size = 4GB*/
			dummy_read_center_address = 0x80000000ULL;
		else if (max_dram_size <= 0xC0000000)/*dram size <= 3GB*/
			dummy_read_center_address = DRAM_BASE+(max_dram_size >> 1);
		else {
			pr_err("[DRAMC] DRAM max size incorrect!!!\n");
			/*ASSERT(0);*/
		}

		src_array_p = (volatile unsigned int)(dummy_read_center_address - (BUFF_LEN >> 1));
		dst_array_p = __pa(dfs_dummy_buffer);
		init_done = 1;
	}

	return 1;
}
#if 0
int DFS_APDMA_Init(void)
{
	writel(((~DMA_GSEC_EN_BIT)&readl(DMA_GSEC_EN)), DMA_GSEC_EN);
	return 1;
}

int DFS_APDMA_Enable(void)
{
#ifdef APDMAREG_DUMP
	int i;
#endif

	while (readl(DMA_START) & 0x1)
		;
	writel(src_array_p, DMA_SRC);
	writel(dst_array_p, DMA_DST);
	writel(BUFF_LEN , DMA_LEN1);
	writel(DMA_CON_BURST_8BEAT, DMA_CON);

#ifdef APDMAREG_DUMP
	pr_debug("src_p=0x%x, dst_p=0x%x, src_v=0x%x, dst_v=0x%x, len=%d\n",
			src_array_p, dst_array_p, (unsigned int)src_array_v, (unsigned int)dst_array_v, BUFF_LEN);
	for (i = 0; i < 0x60; i += 4)
		pr_debug("[Before]addr:0x%x, value:%x\n", (unsigned int)(DMA_BASE+i), *((volatile int *)(DMA_BASE+i)));

#ifdef APDMA_TEST
	for (i = 0; i < BUFF_LEN/sizeof(unsigned int); i++) {
		dst_array_v[i] = 0;
		src_array_v[i] = i;
	}
#endif
#endif

	mt_reg_sync_writel(0x1 , DMA_START);

#ifdef APDMAREG_DUMP
	for (i = 0; i < 0x60; i += 4)
		pr_debug("[AFTER]addr:0x%x, value:%x\n", (unsigned int)(DMA_BASE+i), *((volatile int *)(DMA_BASE+i)));

#ifdef APDMA_TEST
	for (i = 0; i < BUFF_LEN/sizeof(unsigned int); i++) {
		if (dst_array_v[i] != src_array_v[i]) {
			pr_debug("DMA ERROR at Address %x\n ", (unsigned int)&dst_array_v[i]);
			pr_debug("(i=%d, value=0x%x(should be 0x%x))", i, dst_array_v[i], src_array_v[i]);
			ASSERT(0);
		}
	}
	pr_debug("Channe0 DFS DMA TEST PASS\n");
#endif
#endif
	return 1;
}

int DFS_APDMA_END(void)
{
	while (readl(DMA_START))
		;
	return 1;
}


void dma_dummy_read_for_vcorefs(int loops)
{
	int i, count;
	unsigned long long start_time, end_time, duration;

	DFS_APDMA_early_init();
	enable_clock(MT_CG_INFRA_GCE, "CQDMA");
	for (i = 0; i < loops; i++) {
		count = 0;
		start_time = sched_clock();
		do {
			DFS_APDMA_Enable();
			DFS_APDMA_END();
			end_time = sched_clock();
			duration = end_time - start_time;
			count++;
		} while (duration < 4000L);
		/*pr_debug("[DMA_dummy_read[%d], duration=%lld, count = %d\n", duration, count);*/
	}
	disable_clock(MT_CG_INFRA_GCE, "CQDMA");
}

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

static ssize_t complex_mem_test_store(struct device_driver *driver, const char *buf, size_t count)
{
	/*snprintf(buf, "do nothing\n");*/
	return count;
}
#if 0
#ifdef APDMA_TEST
static ssize_t DFS_APDMA_TEST_show(struct device_driver *driver, char *buf)
{
	dma_dummy_read_for_vcorefs(7);
	return snprintf(buf, PAGE_SIZE, "DFS APDMA Dummy Read Address 0x%x\n", (unsigned int)src_array_p);
}
static ssize_t DFS_APDMA_TEST_store(struct device_driver *driver, const char *buf, size_t count)
{
	return count;
}
#endif
#endif

#if 0
#ifdef READ_DRAM_TEMP_TEST
static ssize_t read_dram_temp_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM MR4 = 0x%x\n", read_dram_temperature());
}
static ssize_t read_dram_temp_store(struct device_driver *driver, const char *buf, size_t count)
{
	return count;
}
#endif
#endif
static ssize_t read_dram_data_rate_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "DRAM data rate = %d\n", get_dram_data_rate());
}

static ssize_t read_dram_data_rate_store(struct device_driver *driver, const char *buf, size_t count)
{
	return count;
}


DRIVER_ATTR(emi_clk_mem_test, 0664, complex_mem_test_show, complex_mem_test_store);

#if 0
#ifdef APDMA_TEST
DRIVER_ATTR(dram_dummy_read_test, 0664, DFS_APDMA_TEST_show, DFS_APDMA_TEST_store);
#endif
#endif

#ifdef READ_DRAM_TEMP_TEST
DRIVER_ATTR(read_dram_temp_test, 0664, read_dram_temp_show, read_dram_temp_store);
#endif

DRIVER_ATTR(read_dram_data_rate, 0664, read_dram_data_rate_show, read_dram_data_rate_store);

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
		DRAMCAO_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCAO_BASE_ADDR @ %p\n", DRAMCAO_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMC0 compatible node\n");
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
		DRAMCNAO_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get DRAMCNAO_BASE_ADDR @ %p\n", DRAMCNAO_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find DRAMCNAO compatible node\n");
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	if (node) {
		TOPCKGEN_BASE_ADDR = of_iomap(node, 0);
		pr_warn("[DRAMC]get TOPCKGEN_BASE_ADDR @ %p\n", TOPCKGEN_BASE_ADDR);
	} else {
		pr_err("[DRAMC]can't find TOPCKGEN compatible node\n");
		return -1;
	}

	if (of_scan_flat_dt(dt_scan_dram_info, NULL) > 0) {
		pr_warn("[DRAMC]find dt_scan_dram_info\n");
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

	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_emi_clk_mem_test);
	if (ret) {
		pr_warn("fail to create the emi_clk_mem_test sysfs files\n");
		return ret;
	}

	/* get dummy read address once only*/
	dram_get_dummy_read_addr();

#if 0
#ifdef APDMA_TEST
	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_dram_dummy_read_test);
	if (ret) {
		pr_warn("fail to create the DFS sysfs files\n");
		return ret;
	}
#endif

#ifdef READ_DRAM_TEMP_TEST
	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_read_dram_temp_test);
	if (ret) {
		pr_warn("fail to create the read dram temp sysfs files\n");
		return ret;
	}
#endif
#endif

	ret = driver_create_file(&dram_test_drv.driver, &driver_attr_read_dram_data_rate);
	if (ret) {
		pr_warn("fail to create the read dram data rate sysfs files\n");
		return ret;
	}

	org_dram_data_rate = get_dram_data_rate();
	pr_warn("[DRAMC Driver] Dram Data Rate = %d\n", org_dram_data_rate);

	if (dram_can_support_fh())
		pr_warn("[DRAMC Driver] dram can support Frequency Hopping\n");
	else
		pr_warn("[DRAMC Driver] dram can not support Frequency Hopping\n");

	return ret;
}

static void __exit dram_test_exit(void)
{
	platform_driver_unregister(&dram_test_drv);
}

postcore_initcall(dram_test_init);
module_exit(dram_test_exit);

MODULE_DESCRIPTION("MediaTek DRAMC Driver v0.1");
