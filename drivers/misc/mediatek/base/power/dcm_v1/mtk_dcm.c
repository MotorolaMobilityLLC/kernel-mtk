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

#include <mtk_dcm.h>

#include <linux/init.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_secure_api.h>
#include <linux/of.h>
#include <linux/of_address.h>
#ifdef USE_DRAM_API_INSTEAD
#include <mtk_dramc.h>
#endif /* #ifdef USE_DRAM_API_INSTEAD */
#include <mtk_dcm_autogen.h>

#if defined(__KERNEL__) && defined(CONFIG_OF)
#ifdef CONFIG_MACH_MT6799
unsigned long dcm_topckgen_base;
unsigned long dcm_infracfg_ao_base;
unsigned long dcm_pericfg_base;
unsigned long dcm_mcucfg_base;
unsigned long dcm_mcucfg_phys_base;
unsigned long dcm_cci_base;
unsigned long dcm_cci_phys_base;
unsigned long dcm_lpdma_base;
unsigned long dcm_lpdma_phys_base;
#ifndef USE_DRAM_API_INSTEAD
unsigned long dcm_dramc0_ao_base;
unsigned long dcm_dramc1_ao_base;
unsigned long dcm_dramc2_ao_base;
unsigned long dcm_dramc3_ao_base;
unsigned long dcm_ddrphy0_ao_base;
unsigned long dcm_ddrphy1_ao_base;
unsigned long dcm_ddrphy2_ao_base;
unsigned long dcm_ddrphy3_ao_base;
#endif /* #ifndef USE_DRAM_API_INSTEAD */
unsigned long dcm_emi_base;

#define TOPCKGEN_NODE "mediatek,mt6799-topckgen"
#define INFRACFG_AO_NODE "mediatek,mt6799-infracfg_ao"
#define PERICFG_NODE "mediatek,mt6799-pericfg"
#define MCUCFG_NODE "mediatek,mcucfg"
#define CCI_NODE "mediatek,mcsi_reg"
#define LPDMA_NODE "mediatek,lpdma"
#ifndef USE_DRAM_API_INSTEAD
#define DRAMC0_AO_NODE "mediatek,dramc0_ao"
#define DRAMC1_AO_NODE "mediatek,dramc1_ao"
#define DRAMC2_AO_NODE "mediatek,dramc2_ao"
#define DRAMC3_AO_NODE "mediatek,dramc3_ao"
#define DDRPHY0_AO_NODE "mediatek,ddrphy0ao"
#define DDRPHY1_AO_NODE "mediatek,ddrphy1ao"
#define DDRPHY2_AO_NODE "mediatek,ddrphy2ao"
#define DDRPHY3_AO_NODE "mediatek,ddrphy3ao"
#endif /* #ifndef USE_DRAM_API_INSTEAD */
#define EMI_NODE "mediatek,emi"
#elif defined(CONFIG_MACH_ELBRUS)
unsigned long dcm_infracfg_ao_base;
unsigned long pericfg_base;
unsigned long dcm_mcucfg_base;
unsigned long dcm_mcucfg_phys_base;
unsigned long dcm_dramc0_ao_base;
unsigned long dcm_dramc1_ao_base;
unsigned long dcm_ddrphy0_ao_base;
unsigned long dcm_ddrphy1_ao_base;
unsigned long dcm_emi_base;
unsigned long dcm_cci_base;
unsigned long dcm_cci_phys_base;

#define INFRACFG_AO_NODE "mediatek,infracfg_ao"
#define PERICFG_NODE "mediatek,pericfg"
#define MCUCFG_NODE "mediatek,cpucfg0"
#define DRAMC0_AO_NODE "mediatek,dramc0_ao"
#define DRAMC1_AO_NODE "mediatek,dramc1_ao"
#define DDRPHY0_AO_NODE "mediatek,ddrphy0ao"
#define DDRPHY1_AO_NODE "mediatek,ddrphy1ao"
#define EMI_NODE "mediatek,emi"
#define CCI_NODE "mediatek,mcsi_b_tracer-v1"
#else
#error NO corresponding project can be found!!!
#endif
#endif /* #if defined(__KERNEL__) && defined(CONFIG_OF) */

#define reg_read(addr)	 __raw_readl(IOMEM(addr))
#define reg_write(addr, val)   mt_reg_sync_writel((val), ((void *)addr))

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write_phy(addr##_PHYS, val)
#ifndef mcsi_reg_read
#define mcsi_reg_read(offset) \
	mt_secure_call(MTK_SIP_KERENL_MCSI_NS_ACCESS, 0, offset, 0)
#endif
#ifndef mcsi_reg_write
#define mcsi_reg_write(val, offset) \
	mt_secure_call(MTK_SIP_KERENL_MCSI_NS_ACCESS, 1, offset, val)
#endif
#define MCSI_SMC_WRITE(addr, val)  mcsi_reg_write(val, (addr##_PHYS & 0xFFFF))
#define MCSI_SMC_READ(addr)  mcsi_reg_read(addr##_PHYS & 0xFFFF)
#else
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)
#define MCSI_SMC_WRITE(addr, val)  reg_write(addr, val)
#define MCSI_SMC_READ(addr)  reg_read(addr)
#endif
#define dcm_smc_msg_send(msg) dcm_smc_msg(msg)

#define TAG	"[Power/dcm] "
#define dcm_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define dcm_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define dcm_info(fmt, args...)	pr_notice(TAG fmt, ##args)
#define dcm_dbg(fmt, args...)				\
	do {						\
		if (dcm_debug)				\
			pr_warn(TAG fmt, ##args);	\
	} while (0)
#define dcm_ver(fmt, args...)	pr_debug(TAG fmt, ##args)

#define REG_DUMP(addr) dcm_info("%-30s(0x%08lx): 0x%08x\n", #addr, addr, reg_read(addr))
#define SECURE_REG_DUMP(addr) dcm_info("%-30s(0x%08lx): 0x%08x\n", #addr, addr, mcsi_reg_read(addr##_PHYS & 0xFFFF))

/** macro **/
#define and(v, a) ((v) & (a))
#define or(v, o) ((v) | (o))
#define aor(v, a, o) (((v) & (a)) | (o))

/** global **/
static short dcm_initiated;
static short dcm_cpu_cluster_stat;

#ifdef CONFIG_MACH_MT6799
static unsigned int all_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
				    | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
				    | GIC_SYNC_DCM_TYPE | LAST_CORE_DCM_TYPE | RGU_DCM_TYPE
				    | INFRA_DCM_TYPE | PERI_DCM_TYPE | TOPCKG_DCM_TYPE
				    | DDRPHY_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE
				    | LPDMA_DCM_TYPE
				    );
static unsigned int init_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
				     | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
				     | INFRA_DCM_TYPE | PERI_DCM_TYPE | TOPCKG_DCM_TYPE
				     );
#elif defined(CONFIG_MACH_ELBRUS)
static unsigned int all_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
				    | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
				    | INFRA_DCM_TYPE | PERI_DCM_TYPE
				    | DDRPHY_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE
				    );
static unsigned int init_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
				     | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
				     | INFRA_DCM_TYPE | PERI_DCM_TYPE
				     );
#else
#error NO corresponding project can be found!!!
#endif

static short dcm_debug;
static DEFINE_MUTEX(dcm_lock);
#ifdef CONFIG_HOTPLUG_CPU
static struct notifier_block dcm_hotplug_nb;
#endif

/*****************************************
 * following is implementation per DCM module.
 * 1. per-DCM function is 1-argu with ON/OFF/MODE option.
 *****************************************/
typedef int (*DCM_FUNC)(int);
typedef void (*DCM_PRESET_FUNC)(void);

int dcm_armcore(ENUM_ARMCORE_DCM mode)
{
#ifdef CONFIG_MACH_MT6799
	dcm_mcucfg_bus_arm_pll_divider_dcm(mode);
	dcm_mcucfg_mp0_arm_pll_divider_dcm(mode);
	dcm_mcucfg_mp1_arm_pll_divider_dcm(mode);
	dcm_mcucfg_mp2_arm_pll_divider_dcm(mode);
#elif defined(CONFIG_MACH_ELBRUS)
	dcm_infracfg_ao_mcu_armpll_bus(mode);
	dcm_infracfg_ao_mcu_armpll_ca15(mode);
	dcm_infracfg_ao_mcu_armpll_ca7l(mode);
	dcm_infracfg_ao_mcu_armpll_ca7ll(mode);
#else
#error NO corresponding project can be found!!!
#endif

	return 0;
}

#ifdef CONFIG_MACH_MT6799
void dcm_infracfg_ao_emi_indiv(int on)
{
	if (on)
		reg_write(INFRA_EMI_DCM_CTRL,
			  (reg_read(INFRA_EMI_BUS_CTRL_1_STA) &
			   ~INFRACFG_AO_EMI_INDIV_MASK) |
			  INFRACFG_AO_EMI_INDIV_ON);
	else
		reg_write(INFRA_EMI_DCM_CTRL,
			  (reg_read(INFRA_EMI_BUS_CTRL_1_STA) &
			   ~INFRACFG_AO_EMI_INDIV_MASK) |
			  INFRACFG_AO_EMI_INDIV_OFF);
}
#endif

int dcm_infra(ENUM_INFRA_DCM on)
{
#ifdef CONFIG_MACH_MT6799
	dcm_infracfg_ao_axi(on);
	dcm_infracfg_ao_emi(on);
	dcm_infracfg_ao_md_qaxi(on);
	dcm_infracfg_ao_qaxi(on);
#elif defined(CONFIG_MACH_ELBRUS)
	dcm_infracfg_ao_gce_second_level(on);
	dcm_infracfg_ao_infra_bus(on);
	dcm_infracfg_ao_infra_faxi_second_level(on);
	dcm_infracfg_ao_infra_haxi_second_level(on);
	dcm_infracfg_ao_infra_md_fmem(on);
	dcm_infracfg_ao_peri_bus(on);
#else
#error NO corresponding project can be found!!!
#endif

	return 0;
}

int dcm_peri(ENUM_PERI_DCM on)
{
#ifdef CONFIG_MACH_MT6799
	dcm_pericfg_peri_dcm(on);
	dcm_pericfg_peri_dcm_emi_group_biu(on);
	dcm_pericfg_peri_dcm_emi_group_bus(on);
	dcm_pericfg_peri_dcm_reg_group_biu(on);
	dcm_pericfg_peri_dcm_reg_group_bus(on);
#elif defined(CONFIG_MACH_ELBRUS)
	dcm_pericfg_dcm_emi_group_reg_group(on);
	dcm_pericfg_reg_stall(on);
#else
#error NO corresponding project can be found!!!
#endif

	return 0;
}

int dcm_mcusys(ENUM_MCUSYS_DCM on)
{
#ifdef CONFIG_MACH_MT6799
	dcm_mcucfg_adb400_dcm(on);
	/* dcm_mcucfg_bus_arm_pll_divider_dcm(on); */
	dcm_mcucfg_bus_sync_dcm(on);
	dcm_mcucfg_bus_clock_dcm(on);
	dcm_mcucfg_bus_fabric_dcm(on);
	dcm_mcucfg_l2_shared_dcm(on);
	dcm_mcucfg_mp0_adb_dcm(on);
	/* dcm_mcucfg_mp0_arm_pll_divider_dcm(on); */
	dcm_mcucfg_mp0_bus_dcm(on);
	dcm_mcucfg_mp0_sync_dcm_cfg(on);
	/* dcm_mcucfg_mp1_arm_pll_divider_dcm(on); */
	dcm_mcucfg_mp1_sync_dcm_enable(on);
	/* dcm_mcucfg_mp2_arm_pll_divider_dcm(on); */
	dcm_mcucfg_mcu_misc_dcm(on);

	dcm_mcsi_reg_cci_cactive(on);
	dcm_mcsi_reg_cci_dcm(on);
#elif defined(CONFIG_MACH_ELBRUS)
	/* dcm_cpucfg0_mp0_dbg_dcm(on); */
	/* dcm_cpucfg1_mp1_dbg_dcm(on); */
	dcm_misccfg_adb400_dcm(on);
	dcm_misccfg_bus_sync_dcm(on);
	dcm_misccfg_bus_clock_dcm(on);
	dcm_misccfg_bus_fabric_dcm(on);
	dcm_misccfg_l2_shared_dcm(on);
	dcm_misccfg_mcsib_dbg_dcm(on);
	dcm_misccfg_mp0_sync_dcm(on);
	dcm_misccfg_mp1_sync_dcm(on);
	/* dcm_misccfg_mp2_dbg_dcm(on); */

	dcm_mcsi_cci_dcm(on);
#else
#error NO corresponding project can be found!!!
#endif

	return 0;
}

int dcm_big_core(ENUM_BIG_CORE_DCM on)
{
#ifdef CTRL_BIGCORE_DCM_IN_KERNEL
	/* only can be accessed if B cluster power on */
	if (dcm_cpu_cluster_stat & DCM_CPU_CLUSTER_B)
#ifdef CONFIG_MACH_MT6799
		dcm_mcucfg_sync_dcm_cfg(on);
#elif defined(CONFIG_MACH_ELBRUS)
		dcm_mcucfg_mp2_sync_dcm(on);
#else
#error NO corresponding project can be found!!!
#endif
#endif

	return 0;
}

int dcm_stall_preset(void)
{
#ifdef CONFIG_MACH_MT6799
	dcm_mcucfg_mp_stall_dcm(DCM_ON);
#endif

	return 0;
}

int dcm_stall(ENUM_STALL_DCM on)
{
#ifdef CONFIG_MACH_MT6799
	dcm_mcucfg_mp0_stall_dcm(on);
	dcm_mcucfg_mp1_stall_dcm(on);
#elif defined(CONFIG_MACH_ELBRUS)
	dcm_misccfg_mp0_stall_dcm(on);
	dcm_misccfg_mp1_stall_dcm(on);
#else
#error NO corresponding project can be found!!!
#endif

	return 0;
}

int dcm_dramc_ao(ENUM_DRAMC_AO_DCM on)
{
	int ret = 0;
#ifdef CONFIG_MACH_MT6799
#ifdef USE_DRAM_API_INSTEAD
	int ch;

	for (ch = 0; ch < CHANNEL_NUMBER; ch++) {
		ret = dcm_dramc_ao_switch(ch, on);
		dcm_dbg("%s: ch=%d on/off=%d done\n", __func__, ch, on);
		if (ret) {
			dcm_err("%s: ch=%d on/off=%d fail, ret=%d\n", __func__,
				ch, on, ret);
			break;
		}
	}
#else /* !USE_DRAM_API_INSTEAD */
	dcm_dramc0_ao_dramc_dcm(on);
	dcm_dramc1_ao_dramc_dcm(on);
	dcm_dramc2_ao_dramc_dcm(on);
	dcm_dramc3_ao_dramc_dcm(on);
#endif /* #ifdef USE_DRAM_API_INSTEAD */
#elif defined(CONFIG_MACH_ELBRUS)
	dcm_dramc0_ao_dramc_ao(on);
	dcm_dramc1_ao_dramc_ao(on);
#else
#error NO corresponding project can be found!!!
#endif

	return ret;
}

int dcm_ddrphy(ENUM_DDRPHY_DCM on)
{
	int ret = 0;
#ifdef CONFIG_MACH_MT6799
#ifdef USE_DRAM_API_INSTEAD
	int ch;

	for (ch = 0; ch < CHANNEL_NUMBER; ch++) {
		ret = dcm_ddrphy_ao_switch(ch, on);
		dcm_dbg("%s: ch=%d on/off=%d done\n", __func__, ch, on);
		if (ret) {
			dcm_err("%s: ch=%d on/off=%d fail, ret=%d\n", __func__,
				ch, on, ret);
			break;
		}
	}
#else /* !USE_DRAM_API_INSTEAD */
	dcm_ddrphy0ao_ddrphy(on);
	dcm_ddrphy1ao_ddrphy(on);
	dcm_ddrphy2ao_ddrphy(on);
	dcm_ddrphy3ao_ddrphy(on);
#endif /* #ifdef USE_DRAM_API_INSTEAD */
#elif defined(CONFIG_MACH_ELBRUS)
	dcm_ddrphy0ao_ddrphy(on);
	dcm_ddrphy1ao_ddrphy(on);
#else
#error NO corresponding project can be found!!!
#endif

	return ret;
}

int dcm_emi(ENUM_EMI_DCM on)
{
#ifdef CONFIG_MACH_MT6799
	dcm_emi_emi_dcm_reg_1(on);
	dcm_emi_emi_dcm_reg_2(on);
#elif defined(CONFIG_MACH_ELBRUS)
	dcm_emi_emi(on);
#else
#error NO corresponding project can be found!!!
#endif

	return 0;
}

#ifdef CONFIG_MACH_MT6799
int dcm_gic_sync(ENUM_GIC_SYNC_DCM on)
{
	/* TODO: add function */

	return 0;
}

int dcm_last_core(ENUM_LAST_CORE_DCM on)
{
	/* TODO: add function */

	return 0;
}

int dcm_rgu(ENUM_RGU_DCM on)
{
	/* TODO: add function */

	return 0;
}

int dcm_topckg(ENUM_TOPCKG_DCM on)
{
	dcm_topckgen_cksys_dcm_emi(on);

	return 0;
}

int dcm_lpdma(ENUM_LPDMA_DCM on)
{
	dcm_lpdma_lpdma(on);

	return 0;
}
#endif

/*****************************************************/
typedef struct _dcm {
	int current_state;
	int saved_state;
	int disable_refcnt;
	int default_state;
	DCM_FUNC func;
	DCM_PRESET_FUNC preset_func;
	int typeid;
	char *name;
} DCM;

static DCM dcm_array[NR_DCM_TYPE] = {
	{
	 .typeid = ARMCORE_DCM_TYPE,
	 .name = "ARMCORE_DCM",
	 .func = (DCM_FUNC) dcm_armcore,
	 .current_state = ARMCORE_DCM_MODE1,
	 .default_state = ARMCORE_DCM_MODE1,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = MCUSYS_DCM_TYPE,
	 .name = "MCUSYS_DCM",
	 .func = (DCM_FUNC) dcm_mcusys,
	 .current_state = MCUSYS_DCM_ON,
	 .default_state = MCUSYS_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = INFRA_DCM_TYPE,
	 .name = "INFRA_DCM",
	 .func = (DCM_FUNC) dcm_infra,
	 /*.preset_func = (DCM_PRESET_FUNC) dcm_infra_preset,*/
	 .current_state = INFRA_DCM_ON,
	 .default_state = INFRA_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = PERI_DCM_TYPE,
	 .name = "PERI_DCM",
	 .func = (DCM_FUNC) dcm_peri,
	 /*.preset_func = (DCM_PRESET_FUNC) dcm_peri_preset,*/
	 .current_state = PERI_DCM_ON,
	 .default_state = PERI_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = EMI_DCM_TYPE,
	 .name = "EMI_DCM",
	 .func = (DCM_FUNC) dcm_emi,
	 .current_state = EMI_DCM_ON,
	 .default_state = EMI_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = DRAMC_DCM_TYPE,
	 .name = "DRAMC_DCM",
	 .func = (DCM_FUNC) dcm_dramc_ao,
	 .current_state = DRAMC_AO_DCM_ON,
	 .default_state = DRAMC_AO_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = DDRPHY_DCM_TYPE,
	 .name = "DDRPHY_DCM",
	 .func = (DCM_FUNC) dcm_ddrphy,
	 .current_state = DDRPHY_DCM_ON,
	 .default_state = DDRPHY_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = STALL_DCM_TYPE,
	 .name = "STALL_DCM",
	 .func = (DCM_FUNC) dcm_stall,
	 .preset_func = (DCM_PRESET_FUNC) dcm_stall_preset,
	 .current_state = STALL_DCM_ON,
	 .default_state = STALL_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = BIG_CORE_DCM_TYPE,
	 .name = "BIG_CORE_DCM",
	 .func = (DCM_FUNC) dcm_big_core,
	 .current_state = BIG_CORE_DCM_ON,
	 .default_state = BIG_CORE_DCM_ON,
	 .disable_refcnt = 0,
	 },
#ifdef CONFIG_MACH_MT6799
	{
	 .typeid = GIC_SYNC_DCM_TYPE,
	 .name = "GIC_SYNC_DCM",
	 .func = (DCM_FUNC) dcm_gic_sync,
	 .current_state = GIC_SYNC_DCM_ON,
	 .default_state = GIC_SYNC_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = LAST_CORE_DCM_TYPE,
	 .name = "LAST_CORE_DCM",
	 .func = (DCM_FUNC) dcm_last_core,
	 .current_state = LAST_CORE_DCM_ON,
	 .default_state = LAST_CORE_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = RGU_DCM_TYPE,
	 .name = "RGU_CORE_DCM",
	 .func = (DCM_FUNC) dcm_rgu,
	 .current_state = RGU_DCM_ON,
	 .default_state = RGU_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = TOPCKG_DCM_TYPE,
	 .name = "TOPCKG_DCM",
	 .func = (DCM_FUNC) dcm_topckg,
	 .current_state = TOPCKG_DCM_ON,
	 .default_state = TOPCKG_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = LPDMA_DCM_TYPE,
	 .name = "LPDMA_DCM",
	 .func = (DCM_FUNC) dcm_lpdma,
	 .current_state = LPDMA_DCM_ON,
	 .default_state = LPDMA_DCM_ON,
	 .disable_refcnt = 0,
	 },
#endif
};

/*****************************************
 * DCM driver will provide regular APIs :
 * 1. dcm_restore(type) to recovery CURRENT_STATE before any power-off reset.
 * 2. dcm_set_default(type) to reset as cold-power-on init state.
 * 3. dcm_disable(type) to disable all dcm.
 * 4. dcm_set_state(type) to set dcm state.
 * 5. dcm_dump_state(type) to show CURRENT_STATE.
 * 6. /sys/power/dcm_state interface:  'restore', 'disable', 'dump', 'set'. 4 commands.
 *
 * spsecified APIs for workaround:
 * 1. (definitely no workaround now)
 *****************************************/
void dcm_set_default(unsigned int type)
{
	int i;
	DCM *dcm;

#ifndef ENABLE_DCM_IN_LK
	dcm_warn("[%s]type:0x%08x, init_dcm_type=0x%x\n", __func__, type, init_dcm_type);
#else
	dcm_warn("[%s]type:0x%08x, init_dcm_type=0x%x, INIT_DCM_TYPE_BY_K=0x%x\n",
		 __func__, type, init_dcm_type, INIT_DCM_TYPE_BY_K);
#endif

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++) {
		if (type & dcm->typeid) {
			dcm->saved_state = dcm->default_state;
			dcm->current_state = dcm->default_state;
			dcm->disable_refcnt = 0;
#ifdef ENABLE_DCM_IN_LK
			if (INIT_DCM_TYPE_BY_K & dcm->typeid) {
#endif
				if (dcm->preset_func)
					dcm->preset_func();
				dcm->func(dcm->current_state);
#ifdef ENABLE_DCM_IN_LK
			}
#endif

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);
		}
	}

	dcm_smc_msg_send(init_dcm_type);

	mutex_unlock(&dcm_lock);
}

void dcm_set_state(unsigned int type, int state)
{
	int i;
	DCM *dcm;
	unsigned int init_dcm_type_pre = init_dcm_type;

	dcm_warn("[%s]type:0x%08x, set:%d, init_dcm_type_pre=0x%x\n",
		 __func__, type, state, init_dcm_type_pre);

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; type && (i < NR_DCM_TYPE); i++, dcm++) {
		if (type & dcm->typeid) {
			type &= ~(dcm->typeid);

			dcm->saved_state = state;
			if (dcm->disable_refcnt == 0) {
				if (state)
					init_dcm_type |= dcm->typeid;
				else
					init_dcm_type &= ~(dcm->typeid);

				dcm->current_state = state;
				dcm->func(dcm->current_state);
			}

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);

		}
	}

	if (init_dcm_type_pre != init_dcm_type) {
		dcm_warn("[%s]type:0x%08x, set:%d, init_dcm_type=0x%x->0x%x\n",
			 __func__, type, state, init_dcm_type_pre, init_dcm_type);
		dcm_smc_msg_send(init_dcm_type);
	}

	mutex_unlock(&dcm_lock);
}


void dcm_disable(unsigned int type)
{
	int i;
	DCM *dcm;
	unsigned int init_dcm_type_pre = init_dcm_type;

	dcm_warn("[%s]type:0x%08x\n", __func__, type);

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; type && (i < NR_DCM_TYPE); i++, dcm++) {
		if (type & dcm->typeid) {
			type &= ~(dcm->typeid);

			dcm->current_state = DCM_OFF;
			if (dcm->disable_refcnt++ == 0)
				init_dcm_type &= ~(dcm->typeid);
			dcm->func(dcm->current_state);

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);

		}
	}

	if (init_dcm_type_pre != init_dcm_type) {
		dcm_warn("[%s]type:0x%08x, init_dcm_type=0x%x->0x%x\n",
			 __func__, type, init_dcm_type_pre, init_dcm_type);
		dcm_smc_msg_send(init_dcm_type);
	}

	mutex_unlock(&dcm_lock);

}

void dcm_restore(unsigned int type)
{
	int i;
	DCM *dcm;
	unsigned int init_dcm_type_pre = init_dcm_type;

	dcm_warn("[%s]type:0x%08x\n", __func__, type);

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; type && (i < NR_DCM_TYPE); i++, dcm++) {
		if (type & dcm->typeid) {
			type &= ~(dcm->typeid);

			if (dcm->disable_refcnt > 0)
				dcm->disable_refcnt--;
			if (dcm->disable_refcnt == 0) {
				if (dcm->saved_state)
					init_dcm_type |= dcm->typeid;
				else
					init_dcm_type &= ~(dcm->typeid);

				dcm->current_state = dcm->saved_state;
				dcm->func(dcm->current_state);
			}

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);

		}
	}

	if (init_dcm_type_pre != init_dcm_type) {
		dcm_warn("[%s]type:0x%08x, init_dcm_type=0x%x->0x%x\n",
			 __func__, type, init_dcm_type_pre, init_dcm_type);
		dcm_smc_msg_send(init_dcm_type);
	}

	mutex_unlock(&dcm_lock);
}


void dcm_dump_state(int type)
{
	int i;
	DCM *dcm;

	dcm_info("\n******** dcm dump state *********\n");
	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++) {
		if (type & dcm->typeid) {
			dcm_info("[%-16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);
		}
	}
}

void dcm_dump_regs(void)
{
	dcm_info("\n******** dcm dump register *********\n");
#ifdef CONFIG_MACH_MT6799
	SECURE_REG_DUMP(CCI_DCM);
	SECURE_REG_DUMP(CCI_CACTIVE);
	REG_DUMP(MP0_SCAL_SYNC_DCM_CONFIG);
	REG_DUMP(MP0_SCAL_CCI_ADB400_DCM_CONFIG);
	REG_DUMP(MP0_SCAL_BUS_FABRIC_DCM_CTRL);
	REG_DUMP(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG);
	REG_DUMP(L2C_SRAM_CTRL);
	REG_DUMP(CCI_CLK_CTRL);
	REG_DUMP(BUS_FABRIC_DCM_CTRL);
	REG_DUMP(MCU_MISC_DCM_CTRL);
	REG_DUMP(CCI_ADB400_DCM_CONFIG);
	REG_DUMP(SYNC_DCM_CONFIG);
	REG_DUMP(SYNC_DCM_CLUSTER_CONFIG);
	REG_DUMP(MP0_PLL_DIVIDER_CFG);
	REG_DUMP(MP1_PLL_DIVIDER_CFG);
	REG_DUMP(MP2_PLL_DIVIDER_CFG);
	REG_DUMP(BUS_PLL_DIVIDER_CFG);
#ifdef CTRL_BIGCORE_DCM_IN_KERNEL
	/* only can be accessed if B cluster power on */
	if (dcm_cpu_cluster_stat & DCM_CPU_CLUSTER_B)
		REG_DUMP(MCUCFG_SYNC_DCM_MP2_REG);
#endif

/* TODO: add function */
#if 0
	if (init_dcm_type & LAST_CORE_DCM_TYPE)

	if (init_dcm_type & GIC_SYNC_DCM_TYPE)

	if (init_dcm_type & RGU_DCM_TYPE)
#endif

	REG_DUMP(PERICFG_PERI_BIU_REG_DCM_CTRL);
	REG_DUMP(PERICFG_PERI_BIU_EMI_DCM_CTRL);
	REG_DUMP(PERICFG_PERI_BIU_DBC_CTRL);
	REG_DUMP(INFRA_BUS_DCM_CTRL);
	REG_DUMP(INFRA_BUS_DCM_CTRL_1);
	REG_DUMP(INFRA_MDBUS_DCM_CTRL);
	REG_DUMP(INFRA_QAXIBUS_DCM_CTRL);
	REG_DUMP(INFRA_EMI_BUS_CTRL_1_STA); /* read for INFRA_EMI_DCM_CTRL */
	REG_DUMP(TOPCKGEN_DCM_CFG);

	REG_DUMP(EMI_CONM);
	REG_DUMP(EMI_CONN);
	REG_DUMP(LPDMA_CONB);
#ifndef USE_DRAM_API_INSTEAD
	REG_DUMP(DDRPHY0AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY0AO_MISC_CG_CTRL2);
	REG_DUMP(DRAMC0_AO_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC0_AO_CLKAR);
	REG_DUMP(DDRPHY1AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY1AO_MISC_CG_CTRL2);
	REG_DUMP(DRAMC1_AO_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC1_AO_CLKAR);
	REG_DUMP(DDRPHY2AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY2AO_MISC_CG_CTRL2);
	REG_DUMP(DRAMC2_AO_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC2_AO_CLKAR);
	REG_DUMP(DDRPHY3AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY3AO_MISC_CG_CTRL2);
	REG_DUMP(DRAMC3_AO_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC3_AO_CLKAR);
#endif /* #ifndef USE_DRAM_API_INSTEAD */
#elif defined(CONFIG_MACH_ELBRUS)
	REG_DUMP(INFRA_TOPCKGEN_DCMCTL);
	REG_DUMP(INFRA_TOPCKGEN_DCMDBC);
	REG_DUMP(INFRA_BUS_DCM_CTRL);
	REG_DUMP(PERI_BUS_DCM_CTRL);
	REG_DUMP(INFRA_MISC);
	/* REG_DUMP(MP0_DBG_PWR_CTRL); */
	/* REG_DUMP(MP1_DBG_PWR_CTRL); */
	REG_DUMP(L2C_SRAM_CTRL);
	REG_DUMP(CCI_CLK_CTRL);
	REG_DUMP(BUS_FABRIC_DCM_CTRL);
	REG_DUMP(CCI_ADB400_DCM_CONFIG);
	REG_DUMP(MCSIB_DBG_CTRL1);
	REG_DUMP(SYNC_DCM_CONFIG);
#ifdef CTRL_BIGCORE_DCM_IN_KERNEL
	if (dcm_cpu_cluster_stat & DCM_CPU_CLUSTER_B)
		REG_DUMP(MCUCFG_SYNC_DCM_MP2_REG);
#endif
	REG_DUMP(SYNC_DCM_CLUSTER_CONFIG);
	/* REG_DUMP(BIG_DBG_PWR_CTRL); */
	REG_DUMP(EMI_CONM);
	REG_DUMP(EMI_CONN);
	SECURE_REG_DUMP(MCSI_CCI_DCM);
#endif
}


#ifdef CONFIG_PM
static ssize_t dcm_state_show(struct kobject *kobj, struct kobj_attribute *attr,
				  char *buf)
{
	int len = 0;
	int i;
	DCM *dcm;

	/* dcm_dump_state(all_dcm_type); */
	len += snprintf(buf+len, PAGE_SIZE-len,
			"\n******** dcm dump state *********\n");
	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++)
		len += snprintf(buf+len, PAGE_SIZE-len,
				"[%-16s 0x%08x] current state:%d (%d), atf_on_cnt:%u\n",
				dcm->name, dcm->typeid, dcm->current_state,
				dcm->disable_refcnt, dcm_smc_read_cnt(dcm->typeid));

	len += snprintf(buf+len, PAGE_SIZE-len,
			"\n********** dcm_state help *********\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"set:       echo set [mask] [mode] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"disable:   echo disable [mask] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"restore:   echo restore [mask] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"dump:      echo dump [mask] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"debug:     echo debug [0/1] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"***** [mask] is hexl bit mask of dcm;\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"***** [mode] is type of DCM to set and retained\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"init_dcm_type=0x%x, all_dcm_type=0x%x, dcm_debug=%d, dcm_cpu_cluster_stat=%d\n",
			init_dcm_type, all_dcm_type, dcm_debug, dcm_cpu_cluster_stat);

	return len;
}

static int dcm_convert_stall_wr_del_sel(unsigned int val)
{
	if (val < 0 || val > 0x1F)
		return 0;
	else
		return val;
}

int dcm_set_stall_wr_del_sel(unsigned int mp0, unsigned int mp1)
{
	mutex_lock(&dcm_lock);

#ifdef CONFIG_MACH_MT6799
	reg_write(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG,
			aor(reg_read(MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG),
				~(MCUSYS_STALL_DCM_MP0_WR_DEL_SEL_MASK),
				(dcm_convert_stall_wr_del_sel(mp0) << 0)));
	reg_write(SYNC_DCM_CLUSTER_CONFIG,
			aor(reg_read(SYNC_DCM_CLUSTER_CONFIG),
				~(MCUSYS_STALL_DCM_MP1_WR_DEL_SEL_MASK),
				(dcm_convert_stall_wr_del_sel(mp1) << 8)));
#elif defined(CONFIG_MACH_ELBRUS)
	reg_write(SYNC_DCM_CLUSTER_CONFIG,
			aor(reg_read(SYNC_DCM_CLUSTER_CONFIG),
				~(MCUSYS_STALL_DCM_MP0_WR_DEL_SEL_MASK |
				MCUSYS_STALL_DCM_MP1_WR_DEL_SEL_MASK),
				((dcm_convert_stall_wr_del_sel(mp0) << 0) |
				(dcm_convert_stall_wr_del_sel(mp1) << 8))));
#endif

	mutex_unlock(&dcm_lock);
	return 0;
}

/*
 * input argument of fsel
 * 0: 1/1
 * 1: 1/2
 * 2: 1/4
 * 3: 1/8
 * 4: 1/16
 * 5: 1/32
 */
void dcm_set_fmem_fsel_dbc(unsigned int fsel, unsigned int dbc)
{
#ifdef CONFIG_MACH_MT6799
	if (fsel > 5)
		return;

	mutex_lock(&dcm_lock);

	topckgen_emi_dcm_full_fsel_set(0x10 >> fsel);
	topckgen_emi_dcm_dbc_cnt_set(dbc);
	dcm_info("%s: set fmem fsel=%d, dbc=%d\n", __func__, fsel, dbc);

	mutex_unlock(&dcm_lock);
#endif
}

static ssize_t dcm_state_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf,
				   size_t n)
{
	char cmd[16];
	unsigned int mask;
	unsigned int val0, val1;
	int ret, mode;

	if (sscanf(buf, "%15s %x", cmd, &mask) == 2) {
		mask &= all_dcm_type;

		if (!strcmp(cmd, "restore")) {
			/* dcm_dump_regs(); */
			dcm_restore(mask);
			/* dcm_dump_regs(); */
		} else if (!strcmp(cmd, "disable")) {
			/* dcm_dump_regs(); */
			dcm_disable(mask);
			/* dcm_dump_regs(); */
		} else if (!strcmp(cmd, "dump")) {
			dcm_dump_state(mask);
			dcm_dump_regs();
		} else if (!strcmp(cmd, "debug")) {
			if (mask == 0)
				dcm_debug = 0;
			else if (mask == 1)
				dcm_debug = 1;
			else if (mask == 2)
				dcm_infracfg_ao_emi_indiv(0);
			else if (mask == 3)
				dcm_infracfg_ao_emi_indiv(1);
		} else if (!strcmp(cmd, "set_stall_sel")) {
			if (sscanf(buf, "%15s %x %x", cmd, &val0, &val1) == 3)
				dcm_set_stall_wr_del_sel(val0, val1);
		} else if (!strcmp(cmd, "set_fmem")) {
			if (sscanf(buf, "%15s %d %d", cmd, &val0, &val1) == 3)
				dcm_set_fmem_fsel_dbc(val0, val1);
		} else if (!strcmp(cmd, "set")) {
			if (sscanf(buf, "%15s %x %d", cmd, &mask, &mode) == 3) {
				mask &= all_dcm_type;

				dcm_set_state(mask, mode);

				/* Log for stallDCM switching in Performance/Normal mode */
				if (mask & STALL_DCM_TYPE) {
					if (mode)
						dcm_info("stall dcm is enabled for Default(Normal) mode started\n");
					else
						dcm_info("stall dcm is disabled for Performance(Sports) mode started\n");
				}
			}
		} else {
			dcm_info("SORRY, do not support your command: %s\n", cmd);
		}
		ret = n;
	} else {
		dcm_info("SORRY, do not support your command.\n");
		ret = -EINVAL;
	}

	return ret;
}

static struct kobj_attribute dcm_state_attr = {
	.attr = {
		 .name = "dcm_state",
		 .mode = 0644,
		 },
	.show = dcm_state_show,
	.store = dcm_state_store,
};
#endif /* #ifdef CONFIG_PM */

#ifdef CONFIG_OF
static int mt_dcm_dts_map(void)
{
	struct device_node *node;
	struct resource r;

#ifdef CONFIG_MACH_MT6799
	/* topckgen */
	node = of_find_compatible_node(NULL, NULL, TOPCKGEN_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", TOPCKGEN_NODE);
		return -1;
	}
	dcm_topckgen_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_topckgen_base) {
		dcm_err("error: cannot iomap %s\n", TOPCKGEN_NODE);
		return -1;
	}
#endif

	/* infracfg_ao */
	node = of_find_compatible_node(NULL, NULL, INFRACFG_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", INFRACFG_AO_NODE);
		return -1;
	}
	dcm_infracfg_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_infracfg_ao_base) {
		dcm_err("error: cannot iomap %s\n", INFRACFG_AO_NODE);
		return -1;
	}

	/* pericfg */
	node = of_find_compatible_node(NULL, NULL, PERICFG_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", PERICFG_NODE);
		return -1;
	}
	dcm_pericfg_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_pericfg_base) {
		dcm_err("error: cannot iomap %s\n", PERICFG_NODE);
		return -1;
	}

	/* mcucfg */
	node = of_find_compatible_node(NULL, NULL, MCUCFG_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", MCUCFG_NODE);
		return -1;
	}
	if (of_address_to_resource(node, 0, &r)) {
		dcm_err("error: cannot get phys addr %s\n", MCUCFG_NODE);
		return -1;
	}
	dcm_mcucfg_phys_base = r.start;
	dcm_mcucfg_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_mcucfg_base) {
		dcm_err("error: cannot iomap %s\n", MCUCFG_NODE);
		return -1;
	}

	/* cci: mcsi-b */
	node = of_find_compatible_node(NULL, NULL, CCI_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", CCI_NODE);
		return -1;
	}
	if (of_address_to_resource(node, 0, &r)) {
		dcm_err("error: cannot get phys addr %s\n", CCI_NODE);
		return -1;
	}
	dcm_cci_phys_base = r.start;
	dcm_cci_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_cci_base) {
		dcm_err("error: cannot iomap %s\n", CCI_NODE);
		return -1;
	}

#ifdef CONFIG_MACH_MT6799
	/* lpdma */
	node = of_find_compatible_node(NULL, NULL, LPDMA_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", LPDMA_NODE);
		return -1;
	}
	if (of_address_to_resource(node, 0, &r)) {
		dcm_err("error: cannot get phys addr %s\n", LPDMA_NODE);
		return -1;
	}
	dcm_lpdma_phys_base = r.start;
	dcm_lpdma_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_lpdma_base) {
		dcm_err("error: cannot iomap %s\n", LPDMA_NODE);
		return -1;
	}
#endif

#ifndef USE_DRAM_API_INSTEAD
	/* dramc0_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC0_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC0_AO_NODE);
		return -1;
	}
	dcm_dramc0_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_dramc0_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC0_AO_NODE);
		return -1;
	}

	/* dramc1_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC1_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC1_AO_NODE);
		return -1;
	}
	dcm_dramc1_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_dramc1_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC1_AO_NODE);
		return -1;
	}

#ifdef CONFIG_MACH_MT6799
	/* dramc2_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC2_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC2_AO_NODE);
		return -1;
	}
	dcm_dramc2_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_dramc2_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC2_AO_NODE);
		return -1;
	}

	/* dramc3_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC3_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC3_AO_NODE);
		return -1;
	}
	dcm_dramc3_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_dramc3_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC3_AO_NODE);
		return -1;
	}
#endif

	/* ddrphy0_ao */
	node = of_find_compatible_node(NULL, NULL, DDRPHY0_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DDRPHY0_AO_NODE);
		return -1;
	}
	dcm_ddrphy0_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_ddrphy0_ao_base) {
		dcm_err("error: cannot iomap %s\n", DDRPHY0_AO_NODE);
		return -1;
	}

	/* ddrphy1_ao */
	node = of_find_compatible_node(NULL, NULL, DDRPHY1_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DDRPHY1_AO_NODE);
		return -1;
	}
	dcm_ddrphy1_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_ddrphy1_ao_base) {
		dcm_err("error: cannot iomap %s\n", DDRPHY1_AO_NODE);
		return -1;
	}

#ifdef CONFIG_MACH_MT6799
	/* ddrphy2_ao */
	node = of_find_compatible_node(NULL, NULL, DDRPHY2_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DDRPHY2_AO_NODE);
		return -1;
	}
	dcm_ddrphy2_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_ddrphy2_ao_base) {
		dcm_err("error: cannot iomap %s\n", DDRPHY2_AO_NODE);
		return -1;
	}

	/* ddrphy3_ao */
	node = of_find_compatible_node(NULL, NULL, DDRPHY3_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DDRPHY3_AO_NODE);
		return -1;
	}
	dcm_ddrphy3_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_ddrphy3_ao_base) {
		dcm_err("error: cannot iomap %s\n", DDRPHY3_AO_NODE);
		return -1;
	}
#endif
#endif /* #ifndef USE_DRAM_API_INSTEAD */

	/* emi */
	node = of_find_compatible_node(NULL, NULL, EMI_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", EMI_NODE);
		return -1;
	}
	dcm_emi_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_emi_base) {
		dcm_err("error: cannot iomap %s\n", EMI_NODE);
		return -1;
	}

	return 0;
}
#else
static int mt_dcm_dts_map(void)
{
	return 0;
}
#endif /* #ifdef CONFIG_PM */

#ifdef CONFIG_HOTPLUG_CPU
static int dcm_hotplug_nc(struct notifier_block *self,
					 unsigned long action, void *hcpu)
{
	unsigned int cpu = (long)hcpu;
	struct cpumask cpuhp_cpumask;
	struct cpumask cpu_online_cpumask;

	switch (action) {
	case CPU_ONLINE:
		arch_get_cluster_cpus(&cpuhp_cpumask, arch_get_cluster_id(cpu));
		cpumask_and(&cpu_online_cpumask, &cpuhp_cpumask, cpu_online_mask);
		if (cpumask_weight(&cpu_online_cpumask) == 1) {
			switch (cpu / 4) {
			case 0:
				dcm_dbg("%s: action=0x%lx, cpu=%u, LL CPU_ONLINE\n",
					__func__, action, cpu);
				dcm_cpu_cluster_stat |= DCM_CPU_CLUSTER_LL;
				break;
			case 1:
				dcm_dbg("%s: action=0x%lx, cpu=%u, L CPU_ONLINE\n",
					__func__, action, cpu);
				dcm_cpu_cluster_stat |= DCM_CPU_CLUSTER_L;
				break;
			case 2:
				dcm_dbg("%s: action=0x%lx, cpu=%u, B CPU_ONLINE\n",
					__func__, action, cpu);
				dcm_cpu_cluster_stat |= DCM_CPU_CLUSTER_B;
				break;
			default:
				break;
			}
		}
		break;
	case CPU_DOWN_PREPARE:
		arch_get_cluster_cpus(&cpuhp_cpumask, arch_get_cluster_id(cpu));
		cpumask_and(&cpu_online_cpumask, &cpuhp_cpumask, cpu_online_mask);
		if (cpumask_weight(&cpu_online_cpumask) == 1) {
			switch (cpu / 4) {
			case 0:
				dcm_dbg("%s: action=0x%lx, cpu=%u, LL CPU_DOWN_PREPARE\n",
					__func__, action, cpu);
				dcm_cpu_cluster_stat &= ~DCM_CPU_CLUSTER_LL;
				break;
			case 1:
				dcm_dbg("%s: action=0x%lx, cpu=%u, L CPU_DOWN_PREPARE\n",
					__func__, action, cpu);
				dcm_cpu_cluster_stat &= ~DCM_CPU_CLUSTER_L;
				break;
			case 2:
				dcm_dbg("%s: action=0x%lx, cpu=%u, B CPU_DOWN_PREPARE\n",
					__func__, action, cpu);
				dcm_cpu_cluster_stat &= ~DCM_CPU_CLUSTER_B;
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}
#endif /* #ifdef CONFIG_HOTPLUG_CPU */

int mt_dcm_init(void)
{
#ifdef DCM_BRINGUP
	dcm_warn("%s: skipped for bring up\n", __func__);
	return 0;
#endif

	if (dcm_initiated)
		return 0;

	if (mt_dcm_dts_map()) {
		dcm_err("%s: failed due to DTS failed\n", __func__);
		return -1;
	}

#if 0 /* WORKAROUND: Disable big core reg protection */
	reg_write(0x10202008, aor(reg_read(0x10202008), ~(0x3), 0x1));
	dcm_info("%s: 0x10202008=0x%x\n", __func__, reg_read(0x10202008));
#endif

#if 0 /* def CTRL_BIGCORE_DCM_IN_KERNEL */
	/* big ext buck iso power on */
	reg_write(0x10B00260, reg_read(0x10B00260) & ~(0x1 << 2));
	dcm_info("%s: 0x10B00260=0x%x\n", __func__, reg_read(0x10B00260));
	dcm_cpu_cluster_stat |= DCM_CPU_CLUSTER_B;
#endif

#ifndef DCM_DEFAULT_ALL_OFF
	/** enable all dcm **/
	dcm_set_default(init_dcm_type);
#else /* DCM_DEFAULT_ALL_OFF */
	dcm_set_state(all_dcm_type, DCM_OFF);
#endif /* #ifndef DCM_DEFAULT_ALL_OFF */

	dcm_dump_regs();

#ifdef CONFIG_PM
	{
		int err = 0;

		err = sysfs_create_file(power_kobj, &dcm_state_attr.attr);
		if (err)
			dcm_err("[%s]: fail to create sysfs\n", __func__);
	}

#ifdef DCM_DEBUG_MON
	{
		int err = 0;

		err = sysfs_create_file(power_kobj, &dcm_debug_mon_attr.attr);
		if (err)
			dcm_err("[%s]: fail to create sysfs\n", __func__);
	}
#endif /* #ifdef DCM_DEBUG_MON */
#endif /* #ifdef CONFIG_PM */

#ifdef CONFIG_HOTPLUG_CPU
	dcm_hotplug_nb = (struct notifier_block) {
		.notifier_call	= dcm_hotplug_nc,
		.priority	= INT_MIN + 2, /* NOTE: make sure this is lower than CPU DVFS */
	};

	if (register_cpu_notifier(&dcm_hotplug_nb))
		dcm_err("[%s]: fail to register_cpu_notifier\n", __func__);
#endif /* #ifdef CONFIG_HOTPLUG_CPU */

	dcm_initiated = 1;

	return 0;
}
late_initcall(mt_dcm_init);

/**** public APIs *****/
void mt_dcm_disable(void)
{
	if (!dcm_initiated)
		return;

	dcm_disable(all_dcm_type);
}

void mt_dcm_restore(void)
{
	if (!dcm_initiated)
		return;

	dcm_restore(all_dcm_type);
}

unsigned int sync_dcm_convert_freq2div(unsigned int freq)
{
	unsigned int div = 0;

	if (freq < SYNC_DCM_CLK_MIN_FREQ)
		return 0;

	/* max divided ratio = Floor (CPU Frequency / (4 or 5) * system timer Frequency) */
	div = (freq / SYNC_DCM_CLK_MIN_FREQ) - 1;
	if (div > SYNC_DCM_MAX_DIV_VAL)
		return SYNC_DCM_MAX_DIV_VAL;

	return div;
}

int sync_dcm_set_cci_div(unsigned int cci)
{
	if (!dcm_initiated)
		return -1;

	/*
	 * 1. set xxx_sync_dcm_div first
	 * 2. set xxx_sync_dcm_tog from 0 to 1 for making sure it is toggled
	 */
	reg_write(MCUCFG_SYNC_DCM_CCI_REG,
		  aor(reg_read(MCUCFG_SYNC_DCM_CCI_REG),
		      ~MCUCFG_SYNC_DCM_SEL_CCI_MASK,
		      cci << MCUCFG_SYNC_DCM_SEL_CCI));
	reg_write(MCUCFG_SYNC_DCM_CCI_REG, aor(reg_read(MCUCFG_SYNC_DCM_CCI_REG),
				       ~MCUCFG_SYNC_DCM_CCI_TOGMASK,
				       MCUCFG_SYNC_DCM_CCI_TOG0));
	reg_write(MCUCFG_SYNC_DCM_CCI_REG, aor(reg_read(MCUCFG_SYNC_DCM_CCI_REG),
				       ~MCUCFG_SYNC_DCM_CCI_TOGMASK,
				       MCUCFG_SYNC_DCM_CCI_TOG1));
	dcm_dbg("%s: MCUCFG_SYNC_DCM_CCI_REG=0x%08x, cci_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_CCI_REG),
		 (and(reg_read(MCUCFG_SYNC_DCM_CCI_REG),
		      MCUCFG_SYNC_DCM_SEL_CCI_MASK) >> MCUCFG_SYNC_DCM_SEL_CCI),
		 cci);

	return 0;
}

int sync_dcm_set_cci_freq(unsigned int cci)
{
	dcm_dbg("%s: cci=%u\n", __func__, cci);
	sync_dcm_set_cci_div(sync_dcm_convert_freq2div(cci));

	return 0;
}

int sync_dcm_set_mp0_div(unsigned int mp0)
{
	if (!dcm_initiated)
		return -1;

	/*
	 * 1. set xxx_sync_dcm_div first
	 * 2. set xxx_sync_dcm_tog from 0 to 1 for making sure it is toggled
	 */
	reg_write(MCUCFG_SYNC_DCM_MP0_REG,
		  aor(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
		      ~MCUCFG_SYNC_DCM_SEL_MP0_MASK,
		      mp0 << MCUCFG_SYNC_DCM_SEL_MP0));
	reg_write(MCUCFG_SYNC_DCM_MP0_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
				       ~MCUCFG_SYNC_DCM_MP0_TOGMASK,
				       MCUCFG_SYNC_DCM_MP0_TOG0));
	reg_write(MCUCFG_SYNC_DCM_MP0_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
				       ~MCUCFG_SYNC_DCM_MP0_TOGMASK,
				       MCUCFG_SYNC_DCM_MP0_TOG1));
	dcm_dbg("%s: MCUCFG_SYNC_DCM_MP0_REG=0x%08x, mp0_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_MP0_REG),
		 (and(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
		      MCUCFG_SYNC_DCM_SEL_MP0_MASK) >> MCUCFG_SYNC_DCM_SEL_MP0),
		 mp0);

	return 0;
}

int sync_dcm_set_mp0_freq(unsigned int mp0)
{
	dcm_dbg("%s: mp0=%u\n", __func__, mp0);
	sync_dcm_set_mp0_div(sync_dcm_convert_freq2div(mp0));

	return 0;
}

int sync_dcm_set_mp1_div(unsigned int mp1)
{
	if (!dcm_initiated)
		return -1;

	/*
	 * 1. set xxx_sync_dcm_div first
	 * 2. set xxx_sync_dcm_tog from 0 to 1 for making sure it is toggled
	 */
	reg_write(MCUCFG_SYNC_DCM_MP1_REG,
		  aor(reg_read(MCUCFG_SYNC_DCM_MP1_REG),
		      ~MCUCFG_SYNC_DCM_SEL_MP1_MASK,
		      mp1 << MCUCFG_SYNC_DCM_SEL_MP1));
	reg_write(MCUCFG_SYNC_DCM_MP1_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP1_REG),
				       ~MCUCFG_SYNC_DCM_MP1_TOGMASK,
				       MCUCFG_SYNC_DCM_MP1_TOG0));
	reg_write(MCUCFG_SYNC_DCM_MP1_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP1_REG),
				       ~MCUCFG_SYNC_DCM_MP1_TOGMASK,
				       MCUCFG_SYNC_DCM_MP1_TOG1));
	dcm_dbg("%s: MCUCFG_SYNC_DCM_MP1_REG=0x%08x, mp1_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_MP1_REG),
		 (and(reg_read(MCUCFG_SYNC_DCM_MP1_REG),
		      MCUCFG_SYNC_DCM_SEL_MP1_MASK) >> MCUCFG_SYNC_DCM_SEL_MP1),
		 mp1);

	return 0;
}

int sync_dcm_set_mp1_freq(unsigned int mp1)
{
	dcm_dbg("%s: mp1=%u\n", __func__, mp1);
	sync_dcm_set_mp1_div(sync_dcm_convert_freq2div(mp1));

	return 0;
}

int sync_dcm_set_mp2_div(unsigned int mp2)
{
	if (!dcm_initiated)
		return -1;

#ifdef CTRL_BIGCORE_DCM_IN_KERNEL
	if (!(dcm_cpu_cluster_stat & DCM_CPU_CLUSTER_B))
		return -1;

	/*
	 * 1. set xxx_sync_dcm_div first
	 * 2. set xxx_sync_dcm_tog from 0 to 1 for making sure it is toggled
	 */
	reg_write(MCUCFG_SYNC_DCM_MP2_REG,
		  aor(reg_read(MCUCFG_SYNC_DCM_MP2_REG),
		      ~MCUCFG_SYNC_DCM_SEL_MP2_MASK,
		      mp2 << MCUCFG_SYNC_DCM_SEL_MP2));
	reg_write(MCUCFG_SYNC_DCM_MP2_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP2_REG),
				       ~MCUCFG_SYNC_DCM_MP2_TOGMASK,
				       MCUCFG_SYNC_DCM_MP2_TOG0));
	reg_write(MCUCFG_SYNC_DCM_MP2_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP2_REG),
				       ~MCUCFG_SYNC_DCM_MP2_TOGMASK,
				       MCUCFG_SYNC_DCM_MP2_TOG1));

	dcm_dbg("%s: MCUCFG_SYNC_DCM_MP2_REG=0x%08x, mp2_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_MP2_REG),
		 (and(reg_read(MCUCFG_SYNC_DCM_MP2_REG),
		      MCUCFG_SYNC_DCM_SEL_MP2_MASK) >> MCUCFG_SYNC_DCM_SEL_MP2),
		 mp2);
#endif

	return 0;
}

int sync_dcm_set_mp2_freq(unsigned int mp2)
{
	dcm_dbg("%s: mp2=%u\n", __func__, mp2);
	sync_dcm_set_mp2_div(sync_dcm_convert_freq2div(mp2));

	return 0;
}

/* unit of frequency is MHz */
int sync_dcm_set_cpu_freq(unsigned int cci, unsigned int mp0, unsigned int mp1, unsigned int mp2)
{
	sync_dcm_set_cci_freq(cci);
	sync_dcm_set_mp0_freq(mp0);
	sync_dcm_set_mp1_freq(mp1);
	sync_dcm_set_mp2_freq(mp2);

	return 0;
}

int sync_dcm_set_cpu_div(unsigned int cci, unsigned int mp0, unsigned int mp1, unsigned int mp2)
{
	sync_dcm_set_cci_div(cci);
	sync_dcm_set_mp0_div(mp0);
	sync_dcm_set_mp1_div(mp1);
	sync_dcm_set_mp2_div(mp2);

	return 0;
}

