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

void __iomem *DRAMCAO_CHA_BASE_ADDR;
void __iomem *DDRPHY_BASE_ADDR;
void __iomem *DRAMCNAO_CHA_BASE_ADDR;
void __iomem *TOPCKGEN_BASE_ADDR;
void __iomem *DRAMCAO_CHB_BASE_ADDR;
void __iomem *INFRACFG_AO_BASE_ADDR;
void __iomem *DRAMCNAO_CHB_BASE_ADDR;

static DEFINE_MUTEX(dram_dfs_mutex);
int org_dram_data_rate = 0;
#if 0 /* if0 for tmp */
static unsigned int enter_pdp_cnt;
#endif /* if0 for tmp */
/*extern bool spm_vcorefs_is_dvfs_in_porgress(void);*/
#define Reg_Sync_Writel(addr, val)   writel(val, IOMEM(addr))
#define Reg_Readl(addr) readl(IOMEM(addr))


const struct dram_info *g_dram_info_dummy_read = NULL;

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
		/* orig_dram_info */
		g_dram_info_dummy_read =
		(const struct dram_info *)of_get_flat_dt_prop(node,
		"orig_dram_info", NULL);
	}

	pr_err("[DRAMC] dram info dram rank number = %d\n",
	g_dram_info_dummy_read->rank_num);
	pr_err("[DRAMC] dram info dram rank0 base = 0x%llx\n",
	g_dram_info_dummy_read->rank_info[0].start);
	pr_err("[DRAMC] dram info dram rank1 base = 0x%llx\n",
			g_dram_info_dummy_read->rank_info[0].start +
			g_dram_info_dummy_read->rank_info[0].size);

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

#if 0  /* if0 for tmp */
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
	/*When doing ZQCS,
	special_command_enable will wakeup RANK1's CKE. This is wrong.*/
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

	/*Set DISDMOEDIS(0x1ec[16])=1:
	CA can not be floating because RK1 want to DPD after RK0 enter SRF.*/
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

	/*Set MRS value - for rank1,
	Calibration command after initialization (0x0A)*/
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

	/*Set MRS value - for rank1,
	I/O Config-1(0x03) - 40-Ohm typical pull-down/pull-up*/
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

	/*Set MRS value - for rank1,
	Device Feature 2(0x02) - RL=12 / WL=9 (<= 800 MHz)*/
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
	unsigned int dramc0_spcmd, dramc0_pd_ctrl;
	unsigned int dramc0_pd_ctrl_2, dramc0_padctl4, dramc0_1E8;
	unsigned int i, cnt = 1000;

	rank_pasr_segment[0] = segment_rank0 & 0xFF;	/* for rank0 */
	rank_pasr_segment[1] = segment_rank1 & 0xFF;	/* for rank1 */
	pr_warn("[DRAMC0] PASR r0 = 0x%x  r1 = 0x%x\n",
	rank_pasr_segment[0], rank_pasr_segment[1]);

	/* backup original data */
	dramc0_spcmd = readl(PDEF_DRAMC0_REG_1E4);
	dramc0_pd_ctrl = readl(PDEF_DRAMC0_REG_1DC);
	dramc0_padctl4 = readl(PDEF_DRAMC0_REG_0E4);
	dramc0_1E8 = readl(PDEF_DRAMC0_REG_1E8);

	/* disable MR4(0x1E8[26]=1) */
	writel(dramc0_1E8 | 0x04000000, PDEF_DRAMC0_REG_1E8);

	/* Set ZQCSCNT7~0(0x1e4[23:16]) = 0
	ZQCSCNT8(0x1DC[0]) = 0: disable ZQCS */
	writel(dramc0_spcmd & 0xff00ffff, PDEF_DRAMC0_REG_1E4);
	writel(dramc0_pd_ctrl & 0xfffffffe, PDEF_DRAMC0_REG_1DC);

	/* Set MIOCKCTRLOFF(0x1dc[26])=1: not stop to DRAM clock! */
	dramc0_pd_ctrl_2 = readl(PDEF_DRAMC0_REG_1DC);
	writel(dramc0_pd_ctrl_2 | 0x04000000, PDEF_DRAMC0_REG_1DC);

	/* fix CKE */
	writel(dramc0_padctl4 | 0x00000004, PDEF_DRAMC0_REG_0E4);

	udelay(1);

	for (i = 0; i < 2; i++) {
		/* set MRS settings include rank number,
		segment information and MRR17 */
		writel(((i << 28) | (rank_pasr_segment[i] << 16) | 0x00000011),
		PDEF_DRAMC0_REG_088);
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
		mb(); /* flsush */

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
	pr_warn("[DRAMC0] PASR offset 0x88 = 0x%x\n",
	readl(PDEF_DRAMC0_REG_088));
	writel(dramc0_spcmd, PDEF_DRAMC0_REG_1E4);

/*all segments of rank1 are not reserved -> rank1 enter DPD*/
	if (segment_rank1 == 0xFF) {
		enter_pdp_cnt++;
		spm_dpd_init();
	}

	return 0;

	/*slp_pasr_en(1, segment_rank0 | (segment_rank1 << 8));*/
}

int exit_pasr_dpd_config(void)
{
	int ret;
	/*slp_dpd_en(0);*/
	/*slp_pasr_en(0, 0);*/
	if (enter_pdp_cnt == 1) {
		enter_pdp_cnt--;
		spm_dpd_dram_init();
	}

	ret = enter_pasr_dpd_config(0, 0);

	return ret;
}
#endif /* if0 for tmp */

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

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 0h === */
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

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 2h === */
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

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 1h === */
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

	/* === Read Check then Fill Memory with
	00 Byte Pattern at offset 3h === */
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

	/* === Read Check then Fill Memory with ffff
	Word Pattern at offset 1h == */
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

	/* === Read Check then Fill Memory with ffff
	Word Pattern at offset 0h == */
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

	pu4reg_value = (readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + u4reg_addr)) |
			readl(IOMEM(DDRPHY_BASE_ADDR + u4reg_addr)) |
			readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + u4reg_addr)));

	return pu4reg_value;
}

void ucDram_Register_Write(unsigned int u4reg_addr, unsigned int u4reg_value)
{
	if (check_dramc_base_addr() == -1) {
		pr_err("[DRAMC] Access-W DRAMC base address is NULL!!!\n");
		/* ASSERT(0); */ /* need porting*/
	}

	writel(u4reg_value, IOMEM(DRAMCAO_CHA_BASE_ADDR + u4reg_addr));
	writel(u4reg_value, IOMEM(DDRPHY_BASE_ADDR + u4reg_addr));
	writel(u4reg_value, IOMEM(DRAMCNAO_CHA_BASE_ADDR + u4reg_addr));
	mb(); /* flush */
}

bool pasr_is_valid(void)
{
	return true;
}

unsigned int get_dram_data_rate_from_reg(void)
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

	mpdiv_shu_sel = (readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x028))>>8) & 0x7;
	pll_shu_sel = readl(IOMEM(DDRPHY_BASE_ADDR + 0x63c)) & 0x3;

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
		(u4DDRPHY_PLL1_Addr[pll_shu_sel])));
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
	pr_err("[DRAMC] GetPhyFrequency: %d\n\n", u2real_freq);
	return u2real_freq;
}

void DVFS_gating_auto_save(void)
{
	unsigned int u4HWTrackPICH0R0, u4HWTrackPICH0R1;
	unsigned int u4HWTrackUICH0R0P0, u4HWTrackUICH0R1P0;
	unsigned int u4HWTrackUICH0R0P1, u4HWTrackUICH0R1P1;
	unsigned int bak_data0, bak_data1;
	unsigned int dfs_level;
	unsigned char checkPIResult = DRAM_FAIL;

	/* ddrphy_conf shuffle_three */
	/*TINFO="===  ddrphy shuffle_three reg start ==="*/
	writel(0x0, IOMEM(INFRACFG_AO_BASE_ADDR + 0x0A4));

	dfs_level =
	(readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x028)) & 0x00003000) >> 12;
	writel(readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x044)) | (0x1 << 12),
	IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x044));
	writel(readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x044)) | (0x1 << 12),
	IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x044));
	writel(readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x1c0)) | (0x1 << 20),
	IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x1c0));
	writel(readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x1c0)) | (0x1 << 20),
	IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x1c0));

	do {
		unsigned char u1Dqs_pi[4];

		checkPIResult = DRAM_OK;
		u4HWTrackPICH0R0 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x374));
		u1Dqs_pi[0] = u4HWTrackPICH0R0 & 0xff;
		u1Dqs_pi[1] = (u4HWTrackPICH0R0 >> 8) & 0xff;
		u1Dqs_pi[2] = (u4HWTrackPICH0R0 >> 16) & 0xff;
		u1Dqs_pi[3] = (u4HWTrackPICH0R0 >> 24) & 0xff;
		if ((u1Dqs_pi[0] > 0x1F) || (u1Dqs_pi[1] > 0x1F)
			|| (u1Dqs_pi[2] > 0x1F) || (u1Dqs_pi[3] > 0x1F))
			checkPIResult = DRAM_FAIL;

		u4HWTrackPICH0R1 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x378));
		u1Dqs_pi[0] = u4HWTrackPICH0R1 & 0xff;
		u1Dqs_pi[1] = (u4HWTrackPICH0R1 >> 8) & 0xff;
		u1Dqs_pi[2] = (u4HWTrackPICH0R1 >> 16) & 0xff;
		u1Dqs_pi[3] = (u4HWTrackPICH0R1 >> 24) & 0xff;
		if ((u1Dqs_pi[0] > 0x1F) || (u1Dqs_pi[1] > 0x1F)
			|| (u1Dqs_pi[2] > 0x1F) || (u1Dqs_pi[3] > 0x1F))
			checkPIResult = DRAM_FAIL;

		if (checkPIResult == DRAM_FAIL) {
			writel(readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x1c0))
			& ~(0x1 << 20), IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x1c0));
			writel(readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x044))
			& ~(0x1 << 12), IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x044));
			mb(); /* flush */
			udelay(1);
			writel(readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x044))
			| (0x1 << 12),	IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x044));
			writel(readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x1c0))
			| (0x1 << 20),	IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x1c0));
		}
	} while (checkPIResult == DRAM_FAIL);

	u4HWTrackPICH0R0 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x374));
	u4HWTrackPICH0R1 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x378));
	u4HWTrackUICH0R0P0 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x37c));
	u4HWTrackUICH0R0P1 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x384));
	u4HWTrackUICH0R1P0 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x380));
	u4HWTrackUICH0R1P1 = readl(IOMEM(DRAMCNAO_CHA_BASE_ADDR + 0x388));

	if (dfs_level == 0x0) {
		writel(u4HWTrackPICH0R0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x094));
		writel(u4HWTrackPICH0R1, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x098));
	} else if (dfs_level == 0x1) {
		writel(u4HWTrackPICH0R0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x840));
		writel(u4HWTrackPICH0R1, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x844));
	} else { /* dfs_level == 0x2 */
		writel(u4HWTrackPICH0R0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc40));
		writel(u4HWTrackPICH0R1, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc44));
		}

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x404)) & 0xff8f8fff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x864)) & 0xff8f8fff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc64)) & 0xff8f8fff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 3;
	bak_data0 |= bak_data1 << 17;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 3;
	bak_data0 |= bak_data1 << 9;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x404));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x864));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc64));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x410)) & 0xfc3fffff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x870)) & 0xfc3fffff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc70)) & 0xfc3fffff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x3;
	bak_data0 |= bak_data1 << 24;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x3;
	bak_data0 |= bak_data1 << 22;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x410));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x870));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc70));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x418)) & 0xfffff880;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x878)) & 0xfffff880;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc78)) & 0xfffff880;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 3;
	bak_data0 |= bak_data1 << 5;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 3;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x3;
	bak_data0 |= bak_data1 << 2;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x3;
	bak_data0 |= bak_data1;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x418));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x878));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc78));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x430)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8c0)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xcc0)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 27;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 27;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 19;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 19;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 11;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 11;
	bak_data0 |= bak_data1 >> 3;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x430));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8c0));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xcc0));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x434)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8c4)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xcc4)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 24;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 24;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 16;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 16;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 8;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 8;
	bak_data0 |= bak_data1;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x434));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8c4));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xcc4));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x438)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8c8)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xcc8)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 27;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 27;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 19;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 19;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 11;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 11;
	bak_data0 |= bak_data1 >> 3;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x438));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8c8));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xcc8));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x43c)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8cc)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xccc)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 24;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 24;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 16;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 16;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 8;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 8;
	bak_data0 |= bak_data1;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x43c));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x8cc));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xccc));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x454)) & 0x0fffffff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x898)) & 0x0fffffff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc98)) & 0x0fffffff;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x1 << 2;
	bak_data0 |= bak_data1 << 29;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x1 << 2;
	bak_data0 |= bak_data1 << 28;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x1 << 2;
	bak_data0 |= bak_data1 << 27;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x1 << 2;
	bak_data0 |= bak_data1 << 26;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x454));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0x898));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHA_BASE_ADDR + 0xc98));

/* ch1 */
	do {
		unsigned char u1Dqs_pi[4];

		checkPIResult = DRAM_OK;
		u4HWTrackPICH0R0 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x374));
		u1Dqs_pi[0]  = u4HWTrackPICH0R0 & 0xff;
		u1Dqs_pi[1]  = (u4HWTrackPICH0R0 >> 8) & 0xff;
		u1Dqs_pi[2]  = (u4HWTrackPICH0R0 >> 16) & 0xff;
		u1Dqs_pi[3]  = (u4HWTrackPICH0R0 >> 24) & 0xff;

		if ((u1Dqs_pi[0] > 0x1F) || (u1Dqs_pi[1] > 0x1F)
			|| (u1Dqs_pi[2] > 0x1F) || (u1Dqs_pi[3] > 0x1F))
			checkPIResult = DRAM_FAIL;

		u4HWTrackPICH0R1 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x378));
		u1Dqs_pi[0]  = u4HWTrackPICH0R1 & 0xff;
		u1Dqs_pi[1]  = (u4HWTrackPICH0R1 >> 8) & 0xff;
		u1Dqs_pi[2]  = (u4HWTrackPICH0R1 >> 16) & 0xff;
		u1Dqs_pi[3]  = (u4HWTrackPICH0R1 >> 24) & 0xff;

		if ((u1Dqs_pi[0] > 0x1F) || (u1Dqs_pi[1] > 0x1F)
			|| (u1Dqs_pi[2] > 0x1F) || (u1Dqs_pi[3] > 0x1F))
			checkPIResult = DRAM_FAIL;

		if (checkPIResult == DRAM_FAIL) {
			writel(readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x1c0))
			& ~(0x1 << 20), IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x1c0));
			writel(readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x044))
			& ~(0x1 << 12), IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x044));
			mb(); /* flush */
			udelay(1);
			writel(readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x044))
			| (0x1 << 12), IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x044));
			writel(readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x1c0))
			| (0x1 << 20), IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x1c0));
		}

	} while (checkPIResult == DRAM_FAIL);

	u4HWTrackPICH0R0 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x374));
	u4HWTrackPICH0R1 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x378));
	u4HWTrackUICH0R0P0 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x37c));
	u4HWTrackUICH0R0P1 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x384));
	u4HWTrackUICH0R1P0 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x380));
	u4HWTrackUICH0R1P1 = readl(IOMEM(DRAMCNAO_CHB_BASE_ADDR + 0x388));

	if (dfs_level == 0x0) {
		writel(u4HWTrackPICH0R0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x094));
		writel(u4HWTrackPICH0R1, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x098));
	} else if (dfs_level == 0x1) {
		writel(u4HWTrackPICH0R0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x840));
		writel(u4HWTrackPICH0R1, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x844));
	} else { /* dfs_level == 0x2 */
		writel(u4HWTrackPICH0R0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc40));
		writel(u4HWTrackPICH0R1, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc44));
	}

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x404)) & 0xff8f8fff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x864)) & 0xff8f8fff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc64)) & 0xff8f8fff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 3;
	bak_data0 |= bak_data1 << 17;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 3;
	bak_data0 |= bak_data1 << 9;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x404));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x864));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc64));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x410)) & 0xfc3fffff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x870)) & 0xfc3fffff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc70)) & 0xfc3fffff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x3;
	bak_data0 |= bak_data1 << 24;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x3;
	bak_data0 |= bak_data1 << 22;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x410));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x870));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc70));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x418)) & 0xfffff880;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x878)) & 0xfffff880;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc78)) & 0xfffff880;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 3;
	bak_data0 |= bak_data1 << 5;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 3;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x3;
	bak_data0 |= bak_data1 << 2;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x3;
	bak_data0 |= bak_data1;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x418));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x878));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc78));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x430)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8c0)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xcc0)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 27;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 27;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 19;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 19;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 11;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 11;
	bak_data0 |= bak_data1 >> 3;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x430));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8c0));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xcc0));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x434)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8c4)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xcc4)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 24;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 24;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 16;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 16;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x7 << 8;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x7 << 8;
	bak_data0 |= bak_data1;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x434));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8c4));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xcc4));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x438)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8c8)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xcc8)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 27;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 27;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 19;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 19;
	bak_data0 |= bak_data1 >> 3;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 11;
	bak_data0 |= bak_data1 << 1;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 11;
	bak_data0 |= bak_data1 >> 3;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x438));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8c8));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xcc8));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x43c)) & 0x888888ff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8cc)) & 0x888888ff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xccc)) & 0x888888ff;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 24;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 24;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 16;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 16;
	bak_data0 |= bak_data1;
	bak_data1 = u4HWTrackUICH0R1P1 & 0x7 << 8;
	bak_data0 |= bak_data1 << 4;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x7 << 8;
	bak_data0 |= bak_data1;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x43c));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x8cc));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xccc));

	if (dfs_level == 0x0) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x454)) & 0x0fffffff;
	} else if (dfs_level == 0x1) {
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x898)) & 0x0fffffff;
	} else {/* dfs_level == 0x2 */
		bak_data0 =
		readl(IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc98)) & 0x0fffffff;
	}

	bak_data1 = u4HWTrackUICH0R1P1 & 0x1 << 2;
	bak_data0 |= bak_data1 << 29;
	bak_data1 = u4HWTrackUICH0R1P0 & 0x1 << 2;
	bak_data0 |= bak_data1 << 28;
	bak_data1 = u4HWTrackUICH0R0P1 & 0x1 << 2;
	bak_data0 |= bak_data1 << 27;
	bak_data1 = u4HWTrackUICH0R0P0 & 0x1 << 2;
	bak_data0 |= bak_data1 << 26;

	if (dfs_level == 0x0)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x454));
	else if (dfs_level == 0x1)
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0x898));
	else /* dfs_level == 0x2 */
		writel(bak_data0, IOMEM(DRAMCAO_CHB_BASE_ADDR + 0xc98));

	/* *(UINT32P)DRAMC_WBR  = 0x0;          */
	writel(0x7, IOMEM(INFRACFG_AO_BASE_ADDR + 0x0A4));
}

unsigned int get_dram_data_rate(void)
{
	unsigned int MEMPLL_FOUT = 0;

	MEMPLL_FOUT = get_dram_data_rate_from_reg() << 1;

	/* DVFS owner to request provide a spec. frequency,
	not real frequency */
	if (MEMPLL_FOUT == 1866)
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
		freq = 1800;
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
	get_dram_data_rate());
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

	org_dram_data_rate = get_dram_data_rate();
	pr_err("[DRAMC Driver] Dram Data Rate = %d\n", org_dram_data_rate);

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
