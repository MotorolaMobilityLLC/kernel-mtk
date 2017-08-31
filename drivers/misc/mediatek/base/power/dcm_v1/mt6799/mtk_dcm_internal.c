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
#include <mt-plat/mtk_chip.h>
#include <mt-plat/mtk_secure_api.h>
#include <emi_mpu.h>

#include <mtk_dcm_internal.h>

#ifdef USE_DRAM_API_INSTEAD
#include <mtk_dramc.h>
#endif /* #ifdef USE_DRAM_API_INSTEAD */

static short dcm_cpu_cluster_stat;
unsigned int dcm_chip_sw_ver = CHIP_SW_VER_01;

#ifdef CONFIG_HOTPLUG_CPU
static struct notifier_block dcm_hotplug_nb;
#endif

#ifdef CONFIG_MACH_MT6799
unsigned int all_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | MCSI_DCM_TYPE
			    | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
			    | GIC_SYNC_DCM_TYPE | LAST_CORE_DCM_TYPE
			    | RGU_DCM_TYPE | INFRA_DCM_TYPE | PERI_DCM_TYPE
			    | TOPCKG_DCM_TYPE | DDRPHY_DCM_TYPE
			    | EMI_DCM_TYPE | DRAMC_DCM_TYPE | LPDMA_DCM_TYPE
			    );
unsigned int init_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | MCSI_DCM_TYPE
			    | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
			    | INFRA_DCM_TYPE | PERI_DCM_TYPE | TOPCKG_DCM_TYPE
			    );
#elif defined(CONFIG_MACH_ELBRUS)
unsigned int all_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
			    | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
			    | INFRA_DCM_TYPE | PERI_DCM_TYPE
			    | DDRPHY_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE
			    );
unsigned int init_dcm_type = (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE
			    | STALL_DCM_TYPE | BIG_CORE_DCM_TYPE
			    | INFRA_DCM_TYPE | PERI_DCM_TYPE
			    );
#else
#error NO corresponding project can be found!!!
#endif

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
unsigned long dcm_chn0_emi_base;
unsigned long dcm_chn1_emi_base;
unsigned long dcm_chn2_emi_base;
unsigned long dcm_chn3_emi_base;

#define TOPCKGEN_NODE "mediatek,mt6799-topckgen"
#define INFRACFG_AO_NODE "mediatek,mt6799-infracfg_ao"
#define PERICFG_NODE "mediatek,mt6799-pericfg"
#define MCUCFG_NODE "mediatek,mcucfg"
#define CCI_NODE "mediatek,mcsi_reg"
#define LPDMA_NODE "mediatek,lpdma"
#ifndef USE_DRAM_API_INSTEAD
#define DRAMC_NODE "mediatek,dramc"
#endif /* #ifndef USE_DRAM_API_INSTEAD */
#define EMI_NODE "mediatek,emi"
#define CHN0_EMI_NODE "mediatek,chn0_emi"
#define CHN1_EMI_NODE "mediatek,chn1_emi"
#define CHN2_EMI_NODE "mediatek,chn2_emi"
#define CHN3_EMI_NODE "mediatek,chn3_emi"
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

short is_dcm_bringup(void)
{
	dcm_chip_sw_ver = mt_get_chip_sw_ver();
#ifdef DCM_BRINGUP
	dcm_warn("%s: skipped for bring up\n", __func__);
	return 1;
#else
	return 0;
#endif
}

unsigned int dcm_get_chip_sw_ver(void)
{
	return dcm_chip_sw_ver;
}

void dcm_pre_init(void)
{
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

	if (dcm_chip_sw_ver >= CHIP_SW_VER_02) {
		dcm_warn("%s: add DCM modules from E2\n", __func__);
		init_dcm_type |= GIC_SYNC_DCM_TYPE;
		init_dcm_type |= RGU_DCM_TYPE;
	}
}

#ifdef CONFIG_OF
int mt_dcm_dts_map(void)
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
#ifdef CONFIG_MACH_MT6799
	/* dramc0_ao */
	node = of_find_compatible_node(NULL, NULL, DRAMC_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", DRAMC_NODE);
		return -1;
	}
	dcm_dramc0_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_dramc0_ao_base) {
		dcm_err("error: cannot iomap %s[0]\n", DRAMC_NODE);
		return -1;
	}

	/* dramc1_ao */
	dcm_dramc1_ao_base = (unsigned long)of_iomap(node, 1);
	if (!dcm_dramc1_ao_base) {
		dcm_err("error: cannot iomap %s[1]\n", DRAMC_NODE);
		return -1;
	}

	/* dramc2_ao */
	dcm_dramc2_ao_base = (unsigned long)of_iomap(node, 2);
	if (!dcm_dramc2_ao_base) {
		dcm_err("error: cannot iomap %s[2]\n", DRAMC_NODE);
		return -1;
	}

	/* dramc3_ao */
	dcm_dramc3_ao_base = (unsigned long)of_iomap(node, 3);
	if (!dcm_dramc3_ao_base) {
		dcm_err("error: cannot iomap %s[3]\n", DRAMC_NODE);
		return -1;
	}

	/* ddrphy0_ao */
	dcm_ddrphy0_ao_base = (unsigned long)of_iomap(node, 8);
	if (!dcm_ddrphy0_ao_base) {
		dcm_err("error: cannot iomap %s[8]\n", DRAMC_NODE);
		return -1;
	}

	/* ddrphy1_ao */
	dcm_ddrphy1_ao_base = (unsigned long)of_iomap(node, 9);
	if (!dcm_ddrphy1_ao_base) {
		dcm_err("error: cannot iomap %s[9]\n", DRAMC_NODE);
		return -1;
	}

	/* ddrphy2_ao */
	dcm_ddrphy2_ao_base = (unsigned long)of_iomap(node, 10);
	if (!dcm_ddrphy2_ao_base) {
		dcm_err("error: cannot iomap %s[10]\n", DRAMC_NODE);
		return -1;
	}

	/* ddrphy3_ao */
	dcm_ddrphy3_ao_base = (unsigned long)of_iomap(node, 11);
	if (!dcm_ddrphy3_ao_base) {
		dcm_err("error: cannot iomap %s[11]\n", DRAMC_NODE);
		return -1;
	}
#elif defined(CONFIG_MACH_ELBRUS)
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
#endif /* #ifdef CONFIG_MACH_MT6799 */
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

	/* chn0_emi */
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

	/* chn1_emi */
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

	/* chn2_emi */
	node = of_find_compatible_node(NULL, NULL, CHN2_EMI_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", CHN2_EMI_NODE);
		return -1;
	}
	dcm_chn2_emi_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_chn2_emi_base) {
		dcm_err("error: cannot iomap %s\n", CHN2_EMI_NODE);
		return -1;
	}

	/* chn3_emi */
	node = of_find_compatible_node(NULL, NULL, CHN3_EMI_NODE);
	if (!node) {
		dcm_err("error: cannot find node %s\n", CHN3_EMI_NODE);
		return -1;
	}
	dcm_chn3_emi_base = (unsigned long)of_iomap(node, 0);
	if (!dcm_chn3_emi_base) {
		dcm_err("error: cannot iomap %s\n", CHN3_EMI_NODE);
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

/* return divider value converted from fsel_val */
static unsigned int topckgen_emi_dcm_get_div_val(void)
{
	unsigned int fsel_val = topckgen_emi_dcm_full_fsel_get();

	dcm_dbg("%s: fmem_fsel_val=%u\n", __func__, fsel_val);

	if (fsel_val)
		if (fsel_val > 16)
			return 1;
		else
			return (16 / fsel_val);
	else
		return 32;
}

int __attribute__((weak)) emi_set_dcm_ratio(unsigned int div_val)
{
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

	/* notify EMI if FMEM divider is changed */
	if (emi_set_dcm_ratio(topckgen_emi_dcm_get_div_val()))
		dcm_err("%s: emi_set_dcm_ratio failed\n", __func__);

	mutex_unlock(&dcm_lock);
#endif
}

unsigned int sync_dcm_convert_freq2div(unsigned int freq)
{
	unsigned int div = 0, min_freq = SYNC_DCM_CLK_MIN_FREQ;

	if (dcm_chip_sw_ver >= CHIP_SW_VER_02)
		min_freq = SYNC_DCM_CLK_MIN_FREQ_E2;

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
	dcm_dbg("%s: MCUCFG_SYNC_DCM_CCI_REG=0x%08x, cci_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_CCI_REG),
		 (and(reg_read(MCUCFG_SYNC_DCM_CCI_REG),
		      MCUCFG_SYNC_DCM_SEL_CCI_MASK) >> MCUCFG_SYNC_DCM_SEL_CCI),
		 cci);

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

int sync_dcm_set_cci_freq(unsigned int cci)
{
	dcm_dbg("%s: cci=%u\n", __func__, cci);
	sync_dcm_set_cci_div(sync_dcm_convert_freq2div(cci));

	return 0;
}

int sync_dcm_set_mp0_freq(unsigned int mp0)
{
	dcm_dbg("%s: mp0=%u\n", __func__, mp0);
	sync_dcm_set_mp0_div(sync_dcm_convert_freq2div(mp0));

	return 0;
}

int sync_dcm_set_mp1_freq(unsigned int mp1)
{
	dcm_dbg("%s: mp1=%u\n", __func__, mp1);
	sync_dcm_set_mp1_div(sync_dcm_convert_freq2div(mp1));

	return 0;
}

int sync_dcm_set_mp2_freq(unsigned int mp2)
{
	dcm_dbg("%s: mp2=%u\n", __func__, mp2);
	sync_dcm_set_mp2_div(sync_dcm_convert_freq2div(mp2));

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

int dcm_mcsi(ENUM_MCSI_DCM on)
{
#ifdef CONFIG_MACH_MT6799
	dcm_mcsi_reg_cci_cactive(on);
	dcm_mcsi_reg_cci_dcm(on);
#endif

	return 0;
}

int dcm_big_core(ENUM_BIG_CORE_DCM on)
{
#ifdef CTRL_BIGCORE_DCM_IN_KERNEL
	/* only can be accessed if B cluster power on */
	if (dcm_cpu_cluster_stat & DCM_CPU_CLUSTER_B) {
#ifdef CONFIG_MACH_MT6799
		dcm_mcucfg_sync_dcm_cfg(on);
		dcm_mcucfg_cntvalue_dcm(on);
#elif defined(CONFIG_MACH_ELBRUS)
		dcm_mcucfg_mp2_sync_dcm(on);
#else
#error NO corresponding project can be found!!!
#endif
#endif
	}

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
	dcm_ddrphy0ao_ddrphy_e2(on);
	dcm_ddrphy1ao_ddrphy_e2(on);
	dcm_ddrphy2ao_ddrphy_e2(on);
	dcm_ddrphy3ao_ddrphy_e2(on);
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
	dcm_emi_emi_dcm_reg(on);
	dcm_chn0_emi_emi_dcm_reg(on);
	dcm_chn1_emi_emi_dcm_reg(on);
	dcm_chn2_emi_emi_dcm_reg(on);
	dcm_chn3_emi_emi_dcm_reg(on);
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
	dcm_mcucfg_gic_sync_dcm(on);

	return 0;
}

int dcm_last_core(ENUM_LAST_CORE_DCM on)
{
	/* TODO: add function */

	return 0;
}

int dcm_rgu(ENUM_RGU_DCM on)
{
	dcm_mcucfg_mp0_rgu_dcm(on);
	dcm_mcucfg_mp1_rgu_dcm(on);

	return 0;
}

int dcm_topckg(ENUM_TOPCKG_DCM on)
{
	dcm_topckgen_cksys_dcm_emi(on);

	/* notify EMI if FMEM divider is changed */
	if (emi_set_dcm_ratio(topckgen_emi_dcm_get_div_val()))
		dcm_err("%s: emi_set_dcm_ratio failed\n", __func__);

	return 0;
}

int dcm_lpdma(ENUM_LPDMA_DCM on)
{
	dcm_lpdma_lpdma(on);

	return 0;
}
#endif

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
	{
	 .typeid = MCSI_DCM_TYPE,
	 .name = "MCSI_DCM",
	 .func = (DCM_FUNC) dcm_mcsi,
	 .current_state = MCSI_DCM_ON,
	 .default_state = MCSI_DCM_ON,
	 .disable_refcnt = 0,
	 },
#endif
};

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
	if (dcm_cpu_cluster_stat & DCM_CPU_CLUSTER_B) {
		REG_DUMP(MCUCFG_SYNC_DCM_MP2_REG);
		REG_DUMP(MCUCFG_DBG_CONTROL);
	}
#endif
	if (dcm_chip_sw_ver >= CHIP_SW_VER_02) {
		REG_DUMP(MCUSYS_GIC_SYNC_DCM);
		REG_DUMP(MCUCFG_MP0_RGU_DCM);
		REG_DUMP(MCUCFG_MP1_RGU_DCM);
	}

	REG_DUMP(PERICFG_PERI_BIU_REG_DCM_CTRL);
	REG_DUMP(PERICFG_PERI_BIU_EMI_DCM_CTRL);
	REG_DUMP(PERICFG_PERI_BIU_DBC_CTRL);
	if (dcm_chip_sw_ver >= CHIP_SW_VER_02) {
		REG_DUMP(PERICFG_PERI_DCM_EMI_EARLY_CTRL);
		REG_DUMP(PERICFG_PERI_DCM_REG_EARLY_CTRL);
	}
	REG_DUMP(INFRA_BUS_DCM_CTRL);
	REG_DUMP(INFRA_BUS_DCM_CTRL_1);
	REG_DUMP(INFRA_MDBUS_DCM_CTRL);
	REG_DUMP(INFRA_QAXIBUS_DCM_CTRL);
	REG_DUMP(INFRA_EMI_BUS_CTRL_1_STA); /* read for INFRA_EMI_DCM_CTRL */
	REG_DUMP(TOPCKGEN_DCM_CFG);

	REG_DUMP(EMI_CONM);
	REG_DUMP(EMI_CONN);
	REG_DUMP(CHN0_EMI_CHN_EMI_CONB);
	REG_DUMP(CHN1_EMI_CHN_EMI_CONB);
	REG_DUMP(CHN2_EMI_CHN_EMI_CONB);
	REG_DUMP(CHN3_EMI_CHN_EMI_CONB);
	REG_DUMP(LPDMA_CONB);
#ifndef USE_DRAM_API_INSTEAD
	REG_DUMP(DDRPHY0AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY0AO_MISC_CG_CTRL2);
	if (dcm_chip_sw_ver >= CHIP_SW_VER_02) {
		REG_DUMP(DDRPHY0AO_MISC_CG_CTRL5);
		REG_DUMP(DDRPHY0AO_MISC_CTRL3);
		REG_DUMP(DDRPHY0AO_SHU1_B0_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0XC20);
		REG_DUMP(DDRPHY0AO_SHU1_B1_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0XCA0);
		REG_DUMP(DDRPHY0AO_SHU2_B0_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0X1120);
		REG_DUMP(DDRPHY0AO_SHU2_B1_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0X11A0);
		REG_DUMP(DDRPHY0AO_SHU3_B0_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0X1620);
		REG_DUMP(DDRPHY0AO_SHU3_B1_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0X16A0);
		REG_DUMP(DDRPHY0AO_SHU4_B0_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0X1B20);
		REG_DUMP(DDRPHY0AO_SHU4_B1_DQ0);
		REG_DUMP(DDRPHY0AO_RFU_0X1BA0);
	}
	REG_DUMP(DRAMC0_AO_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC0_AO_CLKAR);
	REG_DUMP(DDRPHY1AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY1AO_MISC_CG_CTRL2);
	if (dcm_chip_sw_ver >= CHIP_SW_VER_02) {
		REG_DUMP(DDRPHY1AO_MISC_CG_CTRL5);
		REG_DUMP(DDRPHY1AO_MISC_CTRL3);
		REG_DUMP(DDRPHY1AO_SHU1_B0_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0XC20);
		REG_DUMP(DDRPHY1AO_SHU1_B1_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0XCA0);
		REG_DUMP(DDRPHY1AO_SHU2_B0_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0X1120);
		REG_DUMP(DDRPHY1AO_SHU2_B1_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0X11A0);
		REG_DUMP(DDRPHY1AO_SHU3_B0_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0X1620);
		REG_DUMP(DDRPHY1AO_SHU3_B1_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0X16A0);
		REG_DUMP(DDRPHY1AO_SHU4_B0_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0X1B20);
		REG_DUMP(DDRPHY1AO_SHU4_B1_DQ0);
		REG_DUMP(DDRPHY1AO_RFU_0X1BA0);
	}
	REG_DUMP(DRAMC1_AO_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC1_AO_CLKAR);
	REG_DUMP(DDRPHY2AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY2AO_MISC_CG_CTRL2);
	if (dcm_chip_sw_ver >= CHIP_SW_VER_02) {
		REG_DUMP(DDRPHY2AO_MISC_CG_CTRL5);
		REG_DUMP(DDRPHY2AO_MISC_CTRL3);
		REG_DUMP(DDRPHY2AO_SHU1_B0_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0XC20);
		REG_DUMP(DDRPHY2AO_SHU1_B1_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0XCA0);
		REG_DUMP(DDRPHY2AO_SHU2_B0_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0X1120);
		REG_DUMP(DDRPHY2AO_SHU2_B1_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0X11A0);
		REG_DUMP(DDRPHY2AO_SHU3_B0_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0X1620);
		REG_DUMP(DDRPHY2AO_SHU3_B1_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0X16A0);
		REG_DUMP(DDRPHY2AO_SHU4_B0_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0X1B20);
		REG_DUMP(DDRPHY2AO_SHU4_B1_DQ0);
		REG_DUMP(DDRPHY2AO_RFU_0X1BA0);
	}
	REG_DUMP(DRAMC2_AO_DRAMC_PD_CTRL);
	REG_DUMP(DRAMC2_AO_CLKAR);
	REG_DUMP(DDRPHY3AO_MISC_CG_CTRL0);
	REG_DUMP(DDRPHY3AO_MISC_CG_CTRL2);
	if (dcm_chip_sw_ver >= CHIP_SW_VER_02) {
		REG_DUMP(DDRPHY3AO_MISC_CG_CTRL5);
		REG_DUMP(DDRPHY3AO_MISC_CTRL3);
		REG_DUMP(DDRPHY3AO_SHU1_B0_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0XC20);
		REG_DUMP(DDRPHY3AO_SHU1_B1_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0XCA0);
		REG_DUMP(DDRPHY3AO_SHU2_B0_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0X1120);
		REG_DUMP(DDRPHY3AO_SHU2_B1_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0X11A0);
		REG_DUMP(DDRPHY3AO_SHU3_B0_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0X1620);
		REG_DUMP(DDRPHY3AO_SHU3_B1_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0X16A0);
		REG_DUMP(DDRPHY3AO_SHU4_B0_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0X1B20);
		REG_DUMP(DDRPHY3AO_SHU4_B1_DQ0);
		REG_DUMP(DDRPHY3AO_RFU_0X1BA0);
	}
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
