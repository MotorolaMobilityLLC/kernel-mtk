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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_secure_api.h>

#include <mtk_dcm_internal.h>

#ifdef USE_DRAM_API_INSTEAD
#include <mtk_dramc.h>
#endif /* #ifdef USE_DRAM_API_INSTEAD */

static short dcm_cpu_cluster_stat;

#ifdef CONFIG_HOTPLUG_CPU
static struct notifier_block dcm_hotplug_nb;
#endif

unsigned int all_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
				    | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
				    | GIC_SYNC_DCM_TYPE | RGU_DCM_TYPE
				    | INFRA_DCM_TYPE
				    | DDRPHY_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE
				    );
unsigned int init_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
				     | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
				     | GIC_SYNC_DCM_TYPE | RGU_DCM_TYPE
				     | INFRA_DCM_TYPE
				     );

#if defined(__KERNEL__) && defined(CONFIG_OF)
unsigned long dcm_infracfg_ao_base;
unsigned long dcm_mcucfg_base;
unsigned long dcm_mcucfg_phys_base;
#ifndef USE_DRAM_API_INSTEAD
unsigned long dcm_dramc0_ao_base;
unsigned long dcm_dramc1_ao_base;
unsigned long dcm_ddrphy0_ao_base;
unsigned long dcm_ddrphy1_ao_base;
#endif /* #ifndef USE_DRAM_API_INSTEAD */
unsigned long dcm_chn0_emi_base;
unsigned long dcm_chn1_emi_base;
unsigned long dcm_emi_base;

#define INFRACFG_AO_NODE "mediatek,infracfg_ao"
#define MCUCFG_NODE "mediatek,mcucfg"
#ifndef USE_DRAM_API_INSTEAD
#define DRAMC_AO_NODE "mediatek,dramc"
#endif /* #ifndef USE_DRAM_API_INSTEAD */
#define CHN0_EMI_NODE "mediatek,chn0_emi"
#define CHN1_EMI_NODE "mediatek,chn1_emi"
#define EMI_NODE "mediatek,emi"
#endif /* #if defined(__KERNEL__) && defined(CONFIG_OF) */

short is_dcm_bringup(void)
{
#ifdef DCM_BRINGUP
	dcm_warn("%s: skipped for bring up\n", __func__);
	return 1;
#else
	return 0;
#endif
}

#ifdef CONFIG_OF
int mt_dcm_dts_map(void)
{
	struct device_node *node;
	struct resource r;

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

	/* dram related */
#ifndef USE_DRAM_API_INSTEAD
	/* dramc0_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC_AO_NODE);
		return -1;
	}
	dcm_dramc0_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_dramc0_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC_AO_NODE);
		return -1;
	}

	/* dramc1_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC_AO_NODE);
		return -1;
	}
	dcm_dramc1_ao_base = (unsigned long)of_iomap(node, 1);
	if (!dcm_dramc1_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC_AO_NODE);
		return -1;
	}

	/* ddrphy0_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC_AO_NODE);
		return -1;
	}
	dcm_ddrphy0_ao_base = (unsigned long)of_iomap(node, 4);
	if (!dcm_ddrphy0_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC_AO_NODE);
		return -1;
	}

	/* ddrphy1_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC_AO_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC_AO_NODE);
		return -1;
	}
	dcm_ddrphy1_ao_base = (unsigned long)of_iomap(node, 5);
	if (!dcm_ddrphy1_ao_base) {
		dcm_err("error: cannot iomap %s\n", DRAMC_AO_NODE);
		return -1;
	}
#endif /* #ifndef USE_DRAM_API_INSTEAD */

	node = of_find_compatible_node(NULL, NULL, CHN0_EMI_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", CHN0_EMI_NODE);
		return -1;
	}
	dcm_chn0_emi_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_chn0_emi_base) {
		dcm_err("error: cannot iomap %s\n", CHN0_EMI_NODE);
		return -1;
	}

	node = of_find_compatible_node(NULL, NULL, CHN1_EMI_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", CHN1_EMI_NODE);
		return -1;
	}
	dcm_chn1_emi_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_chn1_emi_base) {
		dcm_err("error: cannot iomap %s\n", CHN1_EMI_NODE);
		return -1;
	}

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
int mt_dcm_dts_map(void)
{
	return 0;
}
#endif /* #ifdef CONFIG_PM */

static int dcm_convert_stall_wr_del_sel(unsigned int val)
{
	if (val < 0 || val > MCUCFG_STALL_DCM_MPX_WR_SEL_MAX_VAL)
		return 0;
	else
		return val;
}

int dcm_set_stall_wr_del_sel(unsigned int mp0, unsigned int mp1)
{
	mutex_lock(&dcm_lock);

	reg_write(MCUCFG_STALL_DCM_MP0_REG,
			aor(reg_read(MCUCFG_STALL_DCM_MP0_REG),
				~(MCUSYS_STALL_DCM_MP0_WR_DEL_SEL_MASK),
				(dcm_convert_stall_wr_del_sel(mp0) <<
				 MCUCFG_STALL_DCM_MP0_WR_SEL_BIT)));
	reg_write(MCUCFG_STALL_DCM_MP1_REG,
			aor(reg_read(MCUCFG_STALL_DCM_MP1_REG),
				~(MCUSYS_STALL_DCM_MP1_WR_DEL_SEL_MASK),
				(dcm_convert_stall_wr_del_sel(mp1) <<
				 MCUCFG_STALL_DCM_MP1_WR_SEL_BIT)));

	mutex_unlock(&dcm_lock);

	return 0;
}

unsigned int sync_dcm_convert_freq2div(unsigned int freq)
{
	unsigned int div = 0, min_freq = SYNC_DCM_CLK_MIN_FREQ;

	if (freq < min_freq)
		return 0;

	/* max divided ratio = Floor (CPU Frequency / (4 or 5) * system timer Frequency) */
	div = (freq / min_freq) - 1;
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
#ifdef __KERNEL__
	dcm_dbg("%s: MCUCFG_SYNC_DCM_CCI_REG=0x%08x, cci_div_sel=%u/%u\n",
#else
	dcm_dbg("%s: MCUCFG_SYNC_DCM_CCI_REG=0x%X, cci_div_sel=%u/%u\n",
#endif
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
	unsigned int mp0_lo = (mp0 & 0xF);
	unsigned int mp0_hi = (mp0 & 0x70) >> 4;

	if (!dcm_initiated)
		return -1;

	/*
	 * 1. set xxx_sync_dcm_div first
	 * 2. set xxx_sync_dcm_tog from 0 to 1 for making sure it is toggled
	 */
	reg_write(MCUCFG_SYNC_DCM_MP0_REG,
		  aor(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
		      ~(MCUCFG_SYNC_DCM_SEL_MP0_LO_MASK |
			MCUCFG_SYNC_DCM_SEL_MP0_HI_MASK),
		      (mp0_lo << MCUCFG_SYNC_DCM_SEL_MP0_LO) |
		      (mp0_hi << MCUCFG_SYNC_DCM_SEL_MP0_HI)));
	reg_write(MCUCFG_SYNC_DCM_MP0_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
				       ~MCUCFG_SYNC_DCM_MP0_TOGMASK,
				       MCUCFG_SYNC_DCM_MP0_TOG0));
	reg_write(MCUCFG_SYNC_DCM_MP0_REG, aor(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
				       ~MCUCFG_SYNC_DCM_MP0_TOGMASK,
				       MCUCFG_SYNC_DCM_MP0_TOG1));
#ifdef __KERNEL__
	dcm_dbg("%s: MCUCFG_SYNC_DCM_MP0_REG=0x%08x, mp0_div_sel=%u/%u, mp0_hi/lo=%u/%u\n",
#else
	dcm_dbg("%s: MCUCFG_SYNC_DCM_MP0_REG=0x%X, mp0_div_sel=%u/%u, mp0_hi/lo=%u/%u\n",
#endif
		 __func__, reg_read(MCUCFG_SYNC_DCM_MP0_REG),
		 (or((and(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
			  MCUCFG_SYNC_DCM_SEL_MP0_LO_MASK) >>
		      MCUCFG_SYNC_DCM_SEL_MP0_LO),
		     (and(reg_read(MCUCFG_SYNC_DCM_MP0_REG),
			  MCUCFG_SYNC_DCM_SEL_MP0_HI_MASK) >>
		      MCUCFG_SYNC_DCM_SEL_MP0_HI))),
		 mp0, mp0_hi, mp0_lo);

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
#ifdef __KERNEL__
	dcm_dbg("%s: MCUCFG_SYNC_DCM_MP1_REG=0x%08x, mp1_div_sel=%u/%u\n",
#else
	dcm_dbg("%s: MCUCFG_SYNC_DCM_MP1_REG=0x%X, mp1_div_sel=%u/%u\n",
#endif
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
	return 0;
}

int sync_dcm_set_mp2_freq(unsigned int mp2)
{
	return 0;
}

/* unit of frequency is MHz */
int sync_dcm_set_cpu_freq(unsigned int cci, unsigned int mp0,
			  unsigned int mp1, unsigned int mp2)
{
	sync_dcm_set_cci_freq(cci);
	sync_dcm_set_mp0_freq(mp0);
	sync_dcm_set_mp1_freq(mp1);
	sync_dcm_set_mp2_freq(mp2);

	return 0;
}

int sync_dcm_set_cpu_div(unsigned int cci, unsigned int mp0,
			 unsigned int mp1, unsigned int mp2)
{
	sync_dcm_set_cci_div(cci);
	sync_dcm_set_mp0_div(mp0);
	sync_dcm_set_mp1_div(mp1);
	sync_dcm_set_mp2_div(mp2);

	return 0;
}

/*****************************************
 * following is implementation per DCM module.
 * 1. per-DCM function is 1-argu with ON/OFF/MODE option.
 *****************************************/
int dcm_topckg(ENUM_TOPCKG_DCM on)
{
	return 0;
}

int dcm_infra(ENUM_INFRA_DCM on)
{
	dcm_infracfg_ao_infra_bus_dcm(on);
	dcm_infracfg_ao_infra_emi_local_dcm(on);
	dcm_infracfg_ao_infra_rx_p2p_dcm(on);
	dcm_infracfg_ao_peri_bus_dcm(on);
	dcm_infracfg_ao_peri_module_dcm(on);

	return 0;
}

int dcm_peri(ENUM_PERI_DCM on)
{
	return 0;
}

int dcm_armcore(ENUM_ARMCORE_DCM mode)
{
	dcm_mcu_misccfg_bus_arm_pll_divider_dcm(mode);
	dcm_mcu_misccfg_mp0_arm_pll_divider_dcm(mode);
	dcm_mcu_misccfg_mp1_arm_pll_divider_dcm(mode);

	return 0;
}

int dcm_mcusys(ENUM_MCUSYS_DCM on)
{
	dcm_mcu_misccfg_adb400_dcm(on);
	dcm_mcu_misccfg_bus_sync_dcm(on);
	dcm_mcu_misccfg_bus_clock_dcm(on);
	dcm_mcu_misccfg_bus_fabric_dcm(on);
	dcm_mcu_misccfg_l2_shared_dcm(on);
	dcm_mcu_misccfg_mp0_sync_dcm_enable(on);
	dcm_mcu_misccfg_mp1_sync_dcm_enable(on);
	dcm_mcu_misccfg_mcu_misc_dcm(on);
	dcm_mcu_misc1cfg_mcsia_dcm(on);

	return 0;
}

int dcm_big_core(ENUM_BIG_CORE_DCM on)
{
	return 0;
}

int dcm_stall_preset(void)
{
	dcm_mcu_misccfg_mp_stall_dcm(DCM_ON);

	return 0;
}

int dcm_stall(ENUM_STALL_DCM on)
{
	dcm_mcu_misccfg_mp0_stall_dcm(on);
	dcm_mcu_misccfg_mp1_stall_dcm(on);

	return 0;
}

int dcm_gic_sync(ENUM_GIC_SYNC_DCM on)
{
	dcm_mcu_misccfg_gic_sync_dcm(on);

	return 0;
}

int dcm_last_core(ENUM_LAST_CORE_DCM on)
{
	return 0;
}

int dcm_rgu(ENUM_RGU_DCM on)
{
	dcm_mp0_cpucfg_mp0_rgu_dcm(on);
	dcm_mp1_cpucfg_mp1_rgu_dcm(on);

	return 0;
}

int dcm_dramc_ao(ENUM_DRAMC_AO_DCM on)
{
	dcm_dramc_ch0_top1_dramc_dcm(on);
	dcm_dramc_ch1_top1_dramc_dcm(on);

	return 0;
}

int dcm_ddrphy(ENUM_DDRPHY_DCM on)
{
	dcm_dramc_ch0_top0_ddrphy(on);
	dcm_dramc_ch1_top0_ddrphy(on);

	return 0;
}

int dcm_emi(ENUM_EMI_DCM on)
{
	dcm_emi_dcm_emi_group(on);
	dcm_chn0_emi_dcm_emi_group(on);
	dcm_chn1_emi_dcm_emi_group(on);

	return 0;
}

int dcm_lpdma(ENUM_LPDMA_DCM on)
{
	return 0;
}

DCM dcm_array[NR_DCM_TYPE] = {
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
	 .name = "RGU_DCM",
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
};

void dcm_dump_regs(void)
{
	dcm_info("\n******** dcm dump register *********\n");
	REG_DUMP(MP0_CPUCFG_MP0_RGU_DCM_CONFIG);
	REG_DUMP(MP1_CPUCFG_MP1_RGU_DCM_CONFIG);
	REG_DUMP(L2C_SRAM_CTRL);
	REG_DUMP(CCI_CLK_CTRL);
	REG_DUMP(BUS_FABRIC_DCM_CTRL);
	REG_DUMP(MCU_MISC_DCM_CTRL);
	REG_DUMP(CCI_ADB400_DCM_CONFIG);
	REG_DUMP(SYNC_DCM_CONFIG);
	REG_DUMP(SYNC_DCM_CLUSTER_CONFIG);
	REG_DUMP(MP_GIC_RGU_SYNC_DCM);
	REG_DUMP(MP0_PLL_DIVIDER_CFG);
	REG_DUMP(MP1_PLL_DIVIDER_CFG);
	REG_DUMP(BUS_PLL_DIVIDER_CFG);
	REG_DUMP(MCSIA_DCM_EN);

	REG_DUMP(INFRA_BUS_DCM_CTRL);
	REG_DUMP(PERI_BUS_DCM_CTRL);
	REG_DUMP(MEM_DCM_CTRL);
	REG_DUMP(P2P_RX_CLK_ON);

	REG_DUMP(EMI_CONM);
	REG_DUMP(EMI_CONN);
	REG_DUMP(CHN0_EMI_CHN_EMI_CONB);
	REG_DUMP(CHN1_EMI_CHN_EMI_CONB);

	REG_DUMP(DRAMC_CH0_TOP1_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC_CH0_TOP1_CLKAR);
	REG_DUMP(DRAMC_CH1_TOP1_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC_CH1_TOP1_CLKAR);

	REG_DUMP(DRAMC_CH0_TOP0_MISC_CG_CTRL0);
	REG_DUMP(DRAMC_CH0_TOP0_MISC_CG_CTRL2);
	REG_DUMP(DRAMC_CH0_TOP0_MISC_CTRL3);
	REG_DUMP(DRAMC_CH0_TOP0_SHU1_B0_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU1_B1_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU1_CA_CMD8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU2_B0_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU2_B1_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU2_CA_CMD8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU3_B0_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU3_B1_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU3_CA_CMD8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU4_B0_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU4_B1_DQ8);
	REG_DUMP(DRAMC_CH0_TOP0_SHU4_CA_CMD8);
	REG_DUMP(DRAMC_CH1_TOP0_MISC_CG_CTRL0);
	REG_DUMP(DRAMC_CH1_TOP0_MISC_CG_CTRL2);
	REG_DUMP(DRAMC_CH1_TOP0_MISC_CTRL3);
	REG_DUMP(DRAMC_CH1_TOP0_SHU1_B0_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU1_B1_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU1_CA_CMD8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU2_B0_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU2_B1_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU2_CA_CMD8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU3_B0_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU3_B1_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU3_CA_CMD8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU4_B0_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU4_B1_DQ8);
	REG_DUMP(DRAMC_CH1_TOP0_SHU4_CA_CMD8);
}

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

void dcm_set_hotplug_nb(void)
{
#ifdef CONFIG_HOTPLUG_CPU
	dcm_hotplug_nb = (struct notifier_block) {
		.notifier_call	= dcm_hotplug_nc,
		.priority	= INT_MIN + 2, /* NOTE: make sure this is lower than CPU DVFS */
	};

	if (register_cpu_notifier(&dcm_hotplug_nb))
		dcm_err("[%s]: fail to register_cpu_notifier\n", __func__);
#endif /* #ifdef CONFIG_HOTPLUG_CPU */
}

int dcm_smc_get_cnt(int type_id)
{
	return dcm_smc_read_cnt(type_id);
}

void dcm_smc_msg_send(unsigned int msg)
{
	dcm_smc_msg(msg);
}

short dcm_get_cpu_cluster_stat(void)
{
	return dcm_cpu_cluster_stat;
}
