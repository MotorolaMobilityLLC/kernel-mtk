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

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "ccci_config.h"
#include <linux/clk.h>
#include <mach/mtk_pbm.h>

#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
#include <mach/mt6605.h>
#endif

#ifdef FEATURE_RF_CLK_BUF
#include <mtk_clkbuf_ctl.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mtk_spm_sleep.h>
#include <pmic.h>
#include "ccci_core.h"
#include "ccci_platform.h"
#include "modem_cldma.h"
#include "cldma_platform.h"
#include "cldma_reg.h"
#include "modem_reg_base.h"
#include "mtk_clkbuf_ctl.h"

#if defined(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#endif

static struct ccci_clk_node clk_table[] = {
	{ NULL,	"scp-sys-md1-main"},
	{ NULL,	"infra-cldma-ap"},
	{ NULL,	"infra-ccif-ap"},
	{ NULL,	"infra-ccif-md"},
	{ NULL,	"infra-ap-c2k-ccif-0"},
	{ NULL,	"infra-ap-c2k-ccif-1"},
	{ NULL,	"infra-md2md-ccif-0"},
	{ NULL,	"infra-md2md-ccif-1"},
	{ NULL,	"infra-md2md-ccif-2"},
	{ NULL,	"infra-md2md-ccif-3"},
	{ NULL,	"infra-md2md-ccif-4"},
	{ NULL,	"infra-md2md-ccif-5"},
};

static void __iomem *md_sram_pms_base;
static void __iomem *md_sram_pd_psmcusys_base;

#ifdef FEATURE_BSI_BPI_SRAM_CFG
static void __iomem *ap_sleep_base;
static void __iomem *ap_topckgen_base;
static atomic_t md_power_on_off_cnt;
#endif

#if defined(CONFIG_PINCTRL_MT6763)
static struct pinctrl *mdcldma_pinctrl;
#endif
void __attribute__((weak)) clk_buf_set_by_flightmode(bool is_flightmode_on)
{
}

#define TAG "mcd"

int __weak spm_get_md1_bulk_option(void)
{
	return 0;
}

void md_cldma_hw_reset(struct ccci_modem *md)
{
	unsigned int reg_value;

	CCCI_DEBUG_LOG(md->index, TAG, "md_cldma_hw_reset:rst cldma\n");
	/* reset cldma hw: AO Domain */
	reg_value = ccci_read32(infra_ao_base, INFRA_RST0_REG_AO);
	reg_value &= ~(CLDMA_AO_RST_MASK); /* the bits in reg is WO, */
	reg_value |= (CLDMA_AO_RST_MASK);/* so only this bit effective */
	ccci_write32(infra_ao_base, INFRA_RST0_REG_AO, reg_value);
	CCCI_BOOTUP_LOG(md->index, TAG, "md_cldma_hw_reset:clear reset\n");
	/* reset cldma clr */
	reg_value = ccci_read32(infra_ao_base, INFRA_RST1_REG_AO);
	reg_value &= ~(CLDMA_AO_RST_MASK);/* read no use, maybe a time delay */
	reg_value |= (CLDMA_AO_RST_MASK);
	ccci_write32(infra_ao_base, INFRA_RST1_REG_AO, reg_value);
	CCCI_BOOTUP_LOG(md->index, TAG, "md_cldma_hw_reset:done\n");

	/* reset cldma hw: PD Domain */
	reg_value = ccci_read32(infra_ao_base, INFRA_RST0_REG_PD);
	reg_value &= ~(CLDMA_PD_RST_MASK);
	reg_value |= (CLDMA_PD_RST_MASK);
	ccci_write32(infra_ao_base, INFRA_RST0_REG_PD, reg_value);
	CCCI_BOOTUP_LOG(md->index, TAG, "md_cldma_hw_reset:clear reset\n");
	/* reset cldma clr */
	reg_value = ccci_read32(infra_ao_base, INFRA_RST1_REG_PD);
	reg_value &= ~(CLDMA_PD_RST_MASK);
	reg_value |= (CLDMA_PD_RST_MASK);
	ccci_write32(infra_ao_base, INFRA_RST1_REG_PD, reg_value);
	CCCI_DEBUG_LOG(md->index, TAG, "md_cldma_hw_reset:done\n");
}

int md_cd_get_modem_hw_info(struct platform_device *dev_ptr, struct ccci_dev_cfg *dev_cfg, struct md_hw_info *hw_info)
{
	struct device_node *node = NULL;
	int idx = 0;

	memset(dev_cfg, 0, sizeof(struct ccci_dev_cfg));
	memset(hw_info, 0, sizeof(struct md_hw_info));

	if (dev_ptr->dev.of_node == NULL) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem OF node NULL\n");
		return -1;
	}

	of_property_read_u32(dev_ptr->dev.of_node, "mediatek,md_id", &dev_cfg->index);
	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "modem hw info get idx:%d\n", dev_cfg->index);
	if (!get_modem_is_enabled(dev_cfg->index)) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d not enable, exit\n", dev_cfg->index + 1);
		return -1;
	}

	switch (dev_cfg->index) {
	case 0:		/* MD_SYS1 */
		dev_cfg->major = 0;
		dev_cfg->minor_base = 0;
		of_property_read_u32(dev_ptr->dev.of_node, "mediatek,cldma_capability", &dev_cfg->capability);

		hw_info->cldma_ap_ao_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 0);
		hw_info->cldma_md_ao_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 1);
		hw_info->cldma_ap_pdn_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 2);
		hw_info->cldma_md_pdn_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 3);
		hw_info->ap_ccif_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 4);
		hw_info->md_ccif_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 5);
		hw_info->cldma_irq_id = irq_of_parse_and_map(dev_ptr->dev.of_node, 0);
		hw_info->ap_ccif_irq_id = irq_of_parse_and_map(dev_ptr->dev.of_node, 1);
		hw_info->md_wdt_irq_id = irq_of_parse_and_map(dev_ptr->dev.of_node, 2);

		/* Device tree using none flag to register irq, sensitivity has set at "irq_of_parse_and_map" */
		hw_info->cldma_irq_flags = IRQF_TRIGGER_NONE;
		hw_info->ap_ccif_irq_flags = IRQF_TRIGGER_NONE;
		hw_info->md_wdt_irq_flags = IRQF_TRIGGER_NONE;

		hw_info->sram_size = CCIF_SRAM_SIZE;
		hw_info->md_rgu_base = MD_RGU_BASE;
		hw_info->l1_rgu_base = L1_RGU_BASE;
		hw_info->md_boot_slave_En = MD_BOOT_VECTOR_EN;
#if defined(CONFIG_PINCTRL_MT6763)
		mdcldma_pinctrl = devm_pinctrl_get(&dev_ptr->dev);
		if (IS_ERR(mdcldma_pinctrl)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get mdcldma_pinctrl failed\n",
							dev_cfg->index + 1);
			return -1;
		}
#else
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "gpio pinctrl is not ready yet, use workaround.\n");
#endif
		for (idx = 0; idx < ARRAY_SIZE(clk_table); idx++) {
			clk_table[idx].clk_ref = devm_clk_get(&dev_ptr->dev, clk_table[idx].clk_name);
			if (IS_ERR(clk_table[idx].clk_ref)) {
				CCCI_ERROR_LOG(dev_cfg->index, TAG, "md%d get %s failed\n",
								dev_cfg->index + 1, clk_table[idx].clk_name);
				clk_table[idx].clk_ref = NULL;
			}
		}
		node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
		hw_info->ap_mixed_base = (unsigned long)of_iomap(node, 0);
		break;
	default:
		return -1;
	}

	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "dev_major:%d,minor_base:%d,capability:%d\n",
							dev_cfg->major,
		     dev_cfg->minor_base, dev_cfg->capability);
	if (hw_info->cldma_ap_ao_base == 0 ||
		hw_info->cldma_md_ao_base == 0 ||
		hw_info->cldma_ap_pdn_base == 0 ||
		hw_info->cldma_md_pdn_base  == 0) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG,
			"ap_cldma: ao_base=0x%p, pdn_base=0x%p,md_cldma: ao_base=0x%p, pdn_base=0x%p\n",
			(void *)hw_info->cldma_ap_ao_base, (void *)hw_info->cldma_ap_pdn_base,
			(void *)hw_info->cldma_md_ao_base, (void *)hw_info->cldma_md_pdn_base);
		return -1;
	}
	if (hw_info->ap_ccif_base == 0 ||
		hw_info->md_ccif_base == 0) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "ap_ccif_base:0x%p, md_ccif_base:0x%p\n",
			(void *)hw_info->ap_ccif_base, (void *)hw_info->md_ccif_base);
		return -1;
	}
	if (hw_info->cldma_irq_id == 0 ||
		hw_info->ap_ccif_irq_id == 0 ||
		hw_info->md_wdt_irq_id == 0) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "cldma_irq:%d,ccif_irq:%d,md_wdt_irq:%d\n",
			hw_info->cldma_irq_id, hw_info->ap_ccif_irq_id, hw_info->md_wdt_irq_id);
		return -1;
	}
	CCCI_DEBUG_LOG(dev_cfg->index, TAG,
		"ap_cldma: ao_base=0x%p, pdn_base=0x%p,md_cldma: ao_base=0x%p, pdn_base=0x%p\n",
		(void *)hw_info->cldma_ap_ao_base, (void *)hw_info->cldma_ap_pdn_base,
		(void *)hw_info->cldma_md_ao_base, (void *)hw_info->cldma_md_pdn_base);

	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "ap_ccif_base:0x%p, md_ccif_base:0x%p\n",
		(void *)hw_info->ap_ccif_base, (void *)hw_info->md_ccif_base);
	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "cldma_irq:%d,ccif_irq:%d,md_wdt_irq:%d\n",
		hw_info->cldma_irq_id, hw_info->ap_ccif_irq_id, hw_info->md_wdt_irq_id);

	return 0;
}

#ifdef FEATURE_CLK_CG_CONTROL
/* md1 sys_clk_cg no need set in this API*/
void ccci_set_clk_cg(struct ccci_modem *md, unsigned int on)
{
	int idx = 0;
	int ret = 0;

	CCCI_NORMAL_LOG(md->index, TAG, "%s: on=%d\n", __func__, on);
	for (idx = 1; idx < ARRAY_SIZE(clk_table); idx++) {
		if (clk_table[idx].clk_ref == NULL)
			continue;
		if (on) {
			ret = clk_prepare_enable(clk_table[idx].clk_ref);
			if (ret)
				CCCI_ERROR_LOG(md->index, TAG, "%s: on=%d,ret=%d\n", __func__, on, ret);
		} else
			clk_disable_unprepare(clk_table[idx].clk_ref);
	}
}
#endif

#ifdef FEATURE_BSI_BPI_SRAM_CFG
/*turn on/off bsi_bpi clock & SRAM for mt6763*/
void ccci_set_bsi_bpi_SRAM_cfg(struct ccci_modem *md, unsigned int power_on, unsigned int stop_type)
{
	unsigned int cnt, reg_value, md_num;

#if defined(CONFIG_MTK_MD3_SUPPORT) && (CONFIG_MTK_MD3_SUPPORT > 0)
	md_num = 2;
#else
	md_num = 1;
#endif

	if (!is_clk_buf_from_pmic()) {
		CCCI_NORMAL_LOG(md->index, TAG, "%s: clk buf is not from pmic, exist!\n", __func__);
		return;
	}

	cnt = atomic_read(&md_power_on_off_cnt);
	CCCI_NORMAL_LOG(md->index, TAG, "%s: power_on=%d, cnt = %d\n", __func__, power_on, cnt);

	if (power_on) {
		if (atomic_inc_return(&md_power_on_off_cnt) > md_num)
			atomic_set(&md_power_on_off_cnt, md_num);

		reg_value = ccci_read32(ap_topckgen_base, 0xC0);
		reg_value &= ~(1 << 15);
		ccci_write32(ap_topckgen_base, 0xC0, reg_value);
		reg_value = ccci_read32(ap_sleep_base, 0x370);
		reg_value &= ~(0x7F);
		ccci_write32(ap_sleep_base, 0x370, reg_value);
	} else {
		if (cnt > 0) {
			if (atomic_dec_and_test(&md_power_on_off_cnt) && stop_type == MD_FLIGHT_MODE_ENTER) {
				reg_value = ccci_read32(ap_sleep_base, 0x370);
				reg_value |= 0x7F;
				ccci_write32(ap_sleep_base, 0x370, reg_value);
				reg_value = ccci_read32(ap_topckgen_base, 0xC0);
				reg_value |= (1 << 15);
				ccci_write32(ap_topckgen_base, 0xC0, reg_value);
				CCCI_NORMAL_LOG(md->index, TAG, "%s: power off done, clk:0x%x sleep:0x%x\n",
					__func__, reg_value, ccci_read32(ap_sleep_base, 0x370));
			}
		} else
			atomic_set(&md_power_on_off_cnt, 0);
	}

}
#endif

int md_cd_io_remap_md_side_register(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	struct md_pll_reg *md_reg;
	struct device_node *node;
	unsigned long infra_cfg_base;

	/* Get infra cfg ao base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg");
	infra_cfg_base = (unsigned long)of_iomap(node, 0);

	md_ctrl->cldma_ap_pdn_base = (void __iomem *)(md_ctrl->hw_info->cldma_ap_pdn_base);
	md_ctrl->cldma_ap_ao_base = (void __iomem *)(md_ctrl->hw_info->cldma_ap_ao_base);
	md_ctrl->cldma_md_pdn_base = (void __iomem *)(md_ctrl->hw_info->cldma_md_pdn_base);
	md_ctrl->cldma_md_ao_base = (void __iomem *)(md_ctrl->hw_info->cldma_md_ao_base);
	md_ctrl->md_boot_slave_En = ioremap_nocache(md_ctrl->hw_info->md_boot_slave_En, 0x4);
	md_ctrl->md_rgu_base = ioremap_nocache(md_ctrl->hw_info->md_rgu_base, 0x300);
	md_ctrl->l1_rgu_base = ioremap_nocache(md_ctrl->hw_info->l1_rgu_base, 0x40);
	md_ctrl->md_global_con0 = ioremap_nocache(MD_GLOBAL_CON0, 0x4);

	md_ctrl->md_bus_status = ioremap_nocache(MD_BUS_STATUS_BASE, MD_BUS_STATUS_LENGTH);
	md_ctrl->md_topsm_status = ioremap_nocache(MD_TOPSM_STATUS_BASE, MD_TOPSM_STATUS_LENGTH);
	md_ctrl->md_ost_status = ioremap_nocache(MD_OST_STATUS_BASE, MD_OST_STATUS_LENGTH);

	md_reg = kzalloc(sizeof(struct md_pll_reg), GFP_KERNEL);
	if (md_reg == NULL) {
		CCCI_ERROR_LOG(-1, TAG, "cldma_sw_init:alloc md reg map mem fail\n");
		return -1;
	}
	md_reg->md_pc_mon1 = ioremap_nocache(MD_PC_MONITOR_BASE, MD_PC_MONITOR_LENGTH);
	md_ctrl->md_pc_monitor = md_reg->md_pc_mon1;
	md_reg->md_pc_mon2 = ioremap_nocache(MD_PC_MONITORL1_BASE, MD_PC_MONITORL1_LENGTH);
	md_reg->md_clkSW = ioremap_nocache(MD_CLKSW_BASE, MD_CLKSW_LENGTH);
	md_reg->md_dcm = ioremap_nocache(MD_GLOBAL_CON_DCM_BASE, MD_GLOBAL_CON_DCM_LEN);
	md_reg->md_peri_misc = ioremap_nocache(MD_PERI_MISC_BASE, MD_PERI_MISC_LEN);
	md_reg->md_L1_a0 = ioremap_nocache(MDL1A0_BASE, MDL1A0_LEN);
	md_reg->md_top_Pll = ioremap_nocache(MDTOP_PLLMIXED_BASE, 4);
	md_reg->md_sys_clk = ioremap_nocache(MDSYS_CLKCTL_BASE, MDSYS_CLKCTL_LEN);
	/*md_reg->md_l1_conf = ioremap_nocache(L1_BASE_MADDR_MDL1_CONF, 4);*/

	md_reg->md_busreg1 = ioremap_nocache(MD_BUS_DUMP_ADDR1, MD_BUS_DUMP_LEN1);
	md_reg->md_busreg2 = ioremap_nocache(MD_BUS_DUMP_ADDR2, MD_BUS_DUMP_LEN2);
	md_reg->md_busrec = ioremap_nocache(MD_BUSREC_DUMP_ADDR, MD_BUSREC_DUMP_LEN);
	md_reg->md_busrec_L1 = ioremap_nocache(MD_BUSREC_L1_DUMP_ADDR, MD_BUSREC_L1_DUMP_LEN);
	md_reg->md_dbgsys_clk = ioremap_nocache(MD_DBGSYS_CLK_ADDR, MD_DBGSYS_CLK_LEN);
	md_reg->md_ect_1 = ioremap_nocache(MD_ECT_DUMP_ADDR1, MD_ECT_DUMP_LEN1);
	md_reg->md_ect_2 = ioremap_nocache(MD_ECT_DUMP_ADDR2, MD_ECT_DUMP_LEN2);
	md_reg->md_ect_3 = ioremap_nocache(MD_ECT_DUMP_ADDR3, MD_ECT_DUMP_LEN3);
	md_reg->md_clk_ctl01 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR01, MD_Clkctrl_DUMP_LEN01);
	md_reg->md_clk_ctl02 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR02, MD_Clkctrl_DUMP_LEN02);
	md_reg->md_clk_ctl03 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR03, MD_Clkctrl_DUMP_LEN03);
	md_reg->md_clk_ctl04 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR04, MD_Clkctrl_DUMP_LEN04);
	md_reg->md_clk_ctl05 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR05, MD_Clkctrl_DUMP_LEN05);
	md_reg->md_clk_ctl06 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR06, MD_Clkctrl_DUMP_LEN06);
	md_reg->md_clk_ctl07 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR07, MD_Clkctrl_DUMP_LEN07);
	md_reg->md_clk_ctl08 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR08, MD_Clkctrl_DUMP_LEN08);
	md_reg->md_clk_ctl09 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR09, MD_Clkctrl_DUMP_LEN09);
	md_reg->md_clk_ctl10 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR10, MD_Clkctrl_DUMP_LEN10);
	md_reg->md_clk_ctl11 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR11, MD_Clkctrl_DUMP_LEN11);
	md_reg->md_clk_ctl12 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR12, MD_Clkctrl_DUMP_LEN12);
	md_reg->md_clk_ctl13 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR13, MD_Clkctrl_DUMP_LEN13);
	md_reg->md_clk_ctl14 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR14, MD_Clkctrl_DUMP_LEN14);
	md_reg->md_clk_ctl15 = ioremap_nocache(MD_Clkctrl_DUMP_ADDR15, MD_Clkctrl_DUMP_LEN15);
	md_reg->md_boot_stats0 = (void __iomem *)(infra_cfg_base + MD1_CFG_BOOT_STATS0);
	md_reg->md_boot_stats1 = (void __iomem *)(infra_cfg_base + MD1_CFG_BOOT_STATS1);

	md_ctrl->md_pll_base = md_reg;

#ifdef MD_PEER_WAKEUP
	md_ctrl->md_peer_wakeup = ioremap_nocache(MD_PEER_WAKEUP, 0x4);
#endif

	md_sram_pms_base = ioremap_nocache(MD_SRAM_PMS_BASE, MD_SRAM_PMS_LEN);
	md_sram_pd_psmcusys_base = ioremap_nocache(MD_SRAM_PD_PSMCUSYS_SRAM_BASE, MD_SRAM_PD_PSMCUSYS_SRAM_LEN);

#ifdef FEATURE_BSI_BPI_SRAM_CFG
	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	ap_sleep_base = of_iomap(node, 0);
	node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	ap_topckgen_base = of_iomap(node, 0);
	atomic_set(&md_power_on_off_cnt, 0);
#endif
	return 0;
}

void md_cd_lock_cldma_clock_src(int locked)
{
	/* spm_ap_mdsrc_req(locked); */
}

void md_cd_lock_modem_clock_src(int locked)
{
	spm_ap_mdsrc_req(locked);
}

void md_cd_dump_md_bootup_status(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	struct md_pll_reg *md_reg = md_ctrl->md_pll_base;

	/*To avoid AP/MD interface delay, dump 3 times, and buy-in the 3rd dump value.*/
	ccci_read32(md_reg->md_boot_stats0, 0);	/* dummy read */
	ccci_read32(md_reg->md_boot_stats0, 0);	/* dummy read */
	CCCI_NOTICE_LOG(md->index, TAG, "md_boot_stats0:0x%X\n", ccci_read32(md_reg->md_boot_stats0, 0));

	ccci_read32(md_reg->md_boot_stats1, 0);	/* dummy read */
	ccci_read32(md_reg->md_boot_stats1, 0);	/* dummy read */
	CCCI_NOTICE_LOG(md->index, TAG, "md_boot_stats1:0x%X\n", ccci_read32(md_reg->md_boot_stats1, 0));
}

void md_cd_dump_debug_register(struct ccci_modem *md)
{
#if 1 /* MD no need dump because of bus hang happened - open for debug */
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
#if 1
	unsigned int reg_value;
	void __iomem *md_addr;
	int cnt = 0;
#endif
	struct md_pll_reg *md_reg = md_ctrl->md_pll_base;

	if (md->md_state == BOOT_WAITING_FOR_HS1)
		return;

	md_cd_lock_modem_clock_src(1);
#if 1
	/* 1. shared memory */
	/* 2. TOPSM */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_TOPSM)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD TOPSM status 0x%x\n", MD_TOPSM_STATUS_BASE);
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5);/* pre-action: permission */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP,
				md_ctrl->md_topsm_status, MD_TOPSM_STATUS_LENGTH);
	}
	/* 3. PC Monitor */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_PCMON)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD PC monitor 0x%x\n", (MD_PC_MONITOR_BASE + 0x100));
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5);/* pre-action: permission */
		/* pre-action: Open Dbgsys clock */
		md_addr = md_reg->md_dbgsys_clk;
		reg_value = ccci_read32(md_addr, 4);
		reg_value |= (0x1 << 3);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]write: %p=0x%x\n", (md_addr + 4), reg_value);
		ccci_write32(md_addr, 4, reg_value);	/* clear bit[29] */
		reg_value = ccci_read32(md_addr, 4);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] read: %p=0x%x\n", (md_addr + 4), reg_value);
		reg_value = ccci_read32(md_addr, 0x20);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] before %p=0x%x\n", (md_addr + 0x20), reg_value);
		cnt = 0;
		while (!(reg_value = ccci_read32(md_addr, 0x20)&(1<<3))) {
			udelay(20);
			cnt++;
			if (cnt > 100) {
				CCCI_ERROR_LOG(md->index, TAG, "[pre-action]read 0x%p=  0x%x\n",
					(md_addr + 0x20), ccci_read32(md_addr, 0x20));
				break;
			}
		}
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]after 0x%x\n", reg_value);

		ccci_write32(md_reg->md_pc_mon1, 4, 0x80000000); /* stop MD PCMon */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_pc_mon1, 0x48);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_reg->md_pc_mon1 + 0x100), 0x280);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_reg->md_pc_mon1 + 0x400), 0x100);
		ccci_write32(md_reg->md_pc_mon1, 4, 0x1);	/* restart MD PCMon */

		ccci_write32(md_reg->md_pc_mon2, 4, 0x80000000); /* stop MD PCMon:L1 */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_pc_mon2, 0x48);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_reg->md_pc_mon2 + 0x100), 0x280);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_reg->md_pc_mon2 + 0x400), 0x100);
		ccci_write32(md_reg->md_pc_mon2, 4, 0x1);	/* restart MD PCMon */
	}
	/* 4. MD RGU */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_MDRGU)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD RGU 0x%x\n", MD_RGU_BASE);
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5); /* pre-action */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_rgu_base, 0x88);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_ctrl->md_rgu_base + 0x200), 0x5C);
	}
	/* 5 OST */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_OST)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD OST status %x\n", MD_OST_STATUS_BASE);
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5); /* pre-action */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_ost_status, MD_OST_STATUS_LENGTH);
	}
	/* 6. Bus status */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_BUS)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD Bus status %x\n", MD_BUS_STATUS_BASE);
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5);/* pre-action: permission */
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_2, 0x65);/* pre-action: permission */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_bus_status, 0x38);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_ctrl->md_bus_status + 0x100), 0x30);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_busreg1, MD_BUS_DUMP_LEN1);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_busreg2, MD_BUS_DUMP_LEN2);
	}
	/* 7. dump PLL */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_PLL)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD PLL\n");
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5);/* pre-action: permission */
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_2, 0x65);/* pre-action: permission */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clkSW, 0x4C);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl01, MD_Clkctrl_DUMP_LEN01);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl02, MD_Clkctrl_DUMP_LEN02);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl03, MD_Clkctrl_DUMP_LEN03);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl04, MD_Clkctrl_DUMP_LEN04);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl05, MD_Clkctrl_DUMP_LEN05);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl06, MD_Clkctrl_DUMP_LEN06);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl07, MD_Clkctrl_DUMP_LEN07);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl08, MD_Clkctrl_DUMP_LEN08);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl09, MD_Clkctrl_DUMP_LEN09);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl10, MD_Clkctrl_DUMP_LEN10);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl11, MD_Clkctrl_DUMP_LEN11);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl12, MD_Clkctrl_DUMP_LEN12);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl13, MD_Clkctrl_DUMP_LEN13);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl14, MD_Clkctrl_DUMP_LEN14);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_clk_ctl15, MD_Clkctrl_DUMP_LEN15);
	}
	/* 8. Bus REC */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_BUSREC)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD Bus REC PC %x\n", MD_BUSREC_DUMP_ADDR);
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5);/* pre-action: permission */
		/* pre-action: Open Dbgsys clock */
		md_addr = md_reg->md_dbgsys_clk;
		reg_value = ccci_read32(md_addr, 4);
		reg_value |= (0x1 << 3);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]write: %p=0x%x\n", (md_addr + 4), reg_value);
		ccci_write32(md_addr, 4, reg_value);	/* clear bit[29] */
		reg_value = ccci_read32(md_addr, 4);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] read: %p=0x%x\n", (md_addr + 4), reg_value);
		reg_value = ccci_read32(md_addr, 0x20);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] before %p=0x%x\n", (md_addr + 0x20), reg_value);
		cnt = 0;
		while (!(reg_value = ccci_read32(md_addr, 0x20)&(1<<3))) {
			udelay(20);
			cnt++;
			if (cnt > 100) {
				CCCI_ERROR_LOG(md->index, TAG, "[pre-action]read 0x%p=  0x%x\n",
					(md_addr + 0x20), ccci_read32(md_addr, 0x20));
				break;
			}
		}
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]after 0x%x\n", reg_value);
		ccci_write32(md_reg->md_busrec, 0x4, 0x1);/* pre-action */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_busrec, MD_BUSREC_DUMP_LEN);
		ccci_write32(md_reg->md_busrec, 0x4, 0x3);/* post-action */

		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD Bus REC L1 %x, len=%x\n",
			MD_BUSREC_L1_DUMP_ADDR + 0x800, 0x41C);
		ccci_write32(md_reg->md_busrec_L1, 0x804, 0x1);/* pre-action */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_busrec_L1 + 0x800, 0x41C);
		ccci_write32(md_reg->md_busrec_L1, 0x804, 0x3);/* post-action */
	}
	/* 9. ECT: must after 4 TO PSM */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_ECT)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD ECT\n");
		ccci_write32(md_reg->md_busreg1, R_DBGSYS_CLK_PMS_1, 0xE7C5);/* pre-action: permission */
		/* pre-action: Open Dbgsys clock */
		md_addr = md_reg->md_dbgsys_clk;
		reg_value = ccci_read32(md_addr, 4);
		reg_value |= (0x1 << 3);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]write: %p=0x%x\n", (md_addr + 4), reg_value);
		ccci_write32(md_addr, 4, reg_value);
		reg_value = ccci_read32(md_addr, 4);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] read: %p=0x%x\n", (md_addr + 4), reg_value);
		reg_value = ccci_read32(md_addr, 0x20);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] before %p=0x%x\n", (md_addr + 0x20), reg_value);
		cnt = 0;
		while (!(reg_value = ccci_read32(md_addr, 0x20)&(1<<3))) {
			udelay(20);
			cnt++;
			if (cnt > 100) {
				CCCI_ERROR_LOG(md->index, TAG, "[pre-action]read 0x%p=  0x%x\n",
					(md_addr + 0x20), ccci_read32(md_addr, 0x20));
				break;
			}
		}
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]after 0x%x\n", reg_value);
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD ECT 0x%x,len=0x%x\n", MD_ECT_DUMP_ADDR1, MD_ECT_DUMP_LEN1);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_ect_1, MD_ECT_DUMP_LEN1);
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD ECT 0x%x,len=0x%x\n", MD_ECT_DUMP_ADDR2, MD_ECT_DUMP_LEN2);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_ect_2, MD_ECT_DUMP_LEN2);
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD ECT 0x%x,len=0x%x\n", MD_ECT_DUMP_ADDR3, MD_ECT_DUMP_LEN3);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_ect_3, MD_ECT_DUMP_LEN3);
	}
#endif
	md_cd_lock_modem_clock_src(0);
#endif
}

void md_cd_check_md_DCM(struct ccci_modem *md)
{
#if 0
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	md_cd_lock_modem_clock_src(1);
	CCCI_NORMAL_LOG(md->index, TAG, "MD DCM: 0x%X\n", *(unsigned int *)(md_ctrl->md_bus_status + 0x45C));
	md_cd_lock_modem_clock_src(0);
#endif
}

void md_cd_check_emi_state(struct ccci_modem *md, int polling)
{
}

static void md1_pcore_sram_turn_on(struct ccci_modem *md)
{
	int i, error_val = 0;
	unsigned int val, golden_val;
	void __iomem *base = md_sram_pd_psmcusys_base;

	/*Turn on MD pcore SRAM_1(0x200D0114)*/
	golden_val = 0xFFFFFFFF;
	for (i = 31; i >= 0; i--) {
		val = (0xFFFFFFFF >> i) & golden_val;
		ccci_write32(base, MD_SRAM_PD_PSMCUSYS_SRAM_1, val);
	}
	val = ccci_read32(base, MD_SRAM_PD_PSMCUSYS_SRAM_1);
	if (val != golden_val) {
		CCCI_ERR_MSG(md->index, TAG, "MD_SRAM_PD_PSMCUSYS_SRAM_1 = 0x%X!= 0xFFFFFFFF\n", val);
		error_val = -1;
	}
	/*Turn on MD pcore SRAM_2(0x200D0118)*/
	golden_val = 0xFFFFFFFF;
	for (i = 31; i >= 0; i--) {
		val = (0xFFFFFFFF >> i) & golden_val;
		ccci_write32(base, MD_SRAM_PD_PSMCUSYS_SRAM_2, val);
	}
	val = ccci_read32(base, MD_SRAM_PD_PSMCUSYS_SRAM_2);
	if (val != golden_val) {
		CCCI_ERR_MSG(md->index, TAG, "MD_SRAM_PD_PSMCUSYS_SRAM_2 = 0x%X!= 0xFFFFFFFF\n", val);
		error_val = -2;
	}
	/*Turn on MD pcore SRAM_3(0x200D011C)*/
	golden_val = 0xFFFFFFFF;
	for (i = 31; i >= 0; i--) {
		val = (0xFFFFFFFF >> i) & golden_val;
		ccci_write32(base, MD_SRAM_PD_PSMCUSYS_SRAM_3, val);
	}
	val = ccci_read32(base, MD_SRAM_PD_PSMCUSYS_SRAM_3);
	if (val != golden_val) {
		CCCI_ERR_MSG(md->index, TAG, "MD_SRAM_PD_PSMCUSYS_SRAM_3 = 0x%X!= 0xFFFFFFFF\n", val);
		error_val = -3;
	}
	/*Turn on MD pcore SRAM_4(0x200D0120)*/
	golden_val = 0xFFFFFFFA;
	for (i = 31; i >= 0; i--) {
		val = (0xFFFFFFFF >> i) & golden_val;
		ccci_write32(base, MD_SRAM_PD_PSMCUSYS_SRAM_4, val);
	}
	val = ccci_read32(base, MD_SRAM_PD_PSMCUSYS_SRAM_4);
	if (val != golden_val) {
		CCCI_ERR_MSG(md->index, TAG, "MD_SRAM_PD_PSMCUSYS_SRAM_4 = 0x%X!= 0xFFFFFFFA\n", val);
		error_val = -4;
	}

	if (error_val != 0) {
		CCCI_ERR_MSG(md->index, TAG, "%s: error_code=%d, trigger AEE db\n", __func__, error_val);
#if defined(CONFIG_MTK_AEE_FEATURE)
		aed_md_exception_api(NULL, 0, NULL, 0,
			"AP ccci turn on md1 pcore sram failed\n", DB_OPT_DEFAULT);
#endif
	}

}


/*Turn on MD pcore SRAM access permission for AP*/
static void md1_pcore_sram_pms_turn_on(struct ccci_modem *md)
{
	void __iomem *base = md_sram_pms_base;

	ccci_write32(base, MD_SRAM_MDSYS_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS1_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS2_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_PSMCUAPB_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDSYS_AP_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS1_AP_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS2_AP_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_PSMCUAPB_AP_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDSYS_TZ_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS1_TZ_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS2_TZ_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_PSMCUAPB_TZ_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDSYS_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS1_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS2_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_PSMCUAPB_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_L1SYS_PMS, 0xFFFF);
}

static void md1_pcore_sram_pms_turn_off(struct ccci_modem *md)
{
	void __iomem *base = md_sram_pms_base;

	ccci_write32(base, MD_SRAM_MDSYS_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS1_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS2_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_PSMCUAPB_MD_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDSYS_AP_PMS, 0x0008);
	ccci_write32(base, MD_SRAM_MDPERISYS1_AP_PMS, 0x0745);
	ccci_write32(base, MD_SRAM_MDPERISYS2_AP_PMS, 0x003C);
	ccci_write32(base, MD_SRAM_PSMCUAPB_AP_PMS, 0x0064);
	ccci_write32(base, MD_SRAM_MDSYS_TZ_PMS, 0x0008);
	ccci_write32(base, MD_SRAM_MDPERISYS1_TZ_PMS, 0x0745);
	ccci_write32(base, MD_SRAM_MDPERISYS2_TZ_PMS, 0x003C);
	ccci_write32(base, MD_SRAM_PSMCUAPB_TZ_PMS, 0x0064);
	ccci_write32(base, MD_SRAM_MDSYS_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS1_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_MDPERISYS2_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_PSMCUAPB_L1_PMS, 0xFFFF);
	ccci_write32(base, MD_SRAM_L1SYS_PMS, 0x0001);
}
static void md1_pcore_sram_on(struct ccci_modem *md)
{
	CCCI_NORMAL_LOG(md->index, TAG, "md1_pcore_sram_on enter\n");
	/*Turn on MD pcore SRAM access permission for AP*/
	md1_pcore_sram_pms_turn_on(md);

	/*Turn on md pcore sram by AP*/
	md1_pcore_sram_turn_on(md);

	/*Turn off MD pcore SRAM access permission for AP*/
	md1_pcore_sram_pms_turn_off(md);
	CCCI_NORMAL_LOG(md->index, TAG, "md1_pcore_sram_on exit\n");
}

void md1_pmic_setting_off(void)
{
	vmd1_pmic_setting_off();
}

void md1_pmic_setting_on(void)
{
	vmd1_pmic_setting_on();
}

/* callback for system power off*/
void ccci_power_off(void)
{
	md1_pmic_setting_on();
}

#define ROr2W(a, b, c)  ccci_write32(a, b, (ccci_read32(a, b)|c))
#define RAnd2W(a, b, c)  ccci_write32(a, b, (ccci_read32(a, b)&c))
#define RabIsc(a, b, c) ((ccci_read32(a, b)&c) != c)

void md1_pll_on_1(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	struct md_pll_reg *md_pll = md_ctrl->md_pll_base;

	/* CCCI_BOOTUP_LOG(md->index, TAG, "md1_pll_on\n"); */
	/* Make md1 208M CG off, switch to software mode */
	ROr2W(md_pll->md_clkSW, 0x20, (0x1<<26)); /* turn off mdpll1 cg */
	ROr2W(md_pll->md_top_Pll, 0x10, (0x1<<16)); /* let mdpll on ctrl into software mode */
	ROr2W(md_pll->md_top_Pll, 0x14, (0x1<<16)); /* let mdpll enable into software mode */
}

void md1_pll_on_2(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	struct md_pll_reg *md_pll = md_ctrl->md_pll_base;

	RAnd2W(md_pll->md_top_Pll, 0x10, ~(0x1<<16)); /* let mdpll on ctrl into hardware mode */
	RAnd2W(md_pll->md_top_Pll, 0x14, ~(0x1<<16)); /* let mdpll enable into hardware mode */
	RAnd2W(md_pll->md_clkSW, 0x20, ~(0x1<<26)); /* turn on mdpll1 cg */
}

void md1_pll_on(struct ccci_modem *md)
{
	void __iomem *map_addr;
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	map_addr = (void __iomem *)(md_ctrl->hw_info->ap_mixed_base);

	/* reset MDPLL1_CON0 to default value */
	ccci_write32(map_addr, MDPLL1_CON0, 0x446D12E0);
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on:reset0(0x%p)0x%X\n",
		map_addr + MDPLL1_CON0, ccci_read32(map_addr, MDPLL1_CON0));

	/* reset MDPLL_CON2 to default value */
	ccci_write32(map_addr, MDPLL1_CON2, 0x4800);
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on:reset2(0x%p)0x%X\n",
		map_addr + MDPLL1_CON2, ccci_read32(map_addr, MDPLL1_CON2));

	/* If MD1 only or both MD1 and MD3 */
	md1_pll_on_1(md);
	/* If MD3 only, do nothing */

	/* Turn on 208M */
	ROr2W(map_addr, MDPLL1_CON0, (0x1));

	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_before W, (0x%p)0x%X\n",
		map_addr, ccci_read32(map_addr, AP_PLL_CON0));
	/* ccci_write32(map_addr, AP_PLL_CON0, 0x39F1); */
	ROr2W(map_addr, AP_PLL_CON0, (0x1<<1));

	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_after W, 0x%X\n", ccci_read32(map_addr, AP_PLL_CON0));

	udelay(200);

	RAnd2W(map_addr, MDPLL1_CON0, ~(0x1));
	/*RAnd2W(map_addr, MDPLL1_CON0, ~(0x1<<7));*/

	/* close 208M and autoK */
	/* ccci_write32(map_addr, AP_PLL_CON0, 0x39F3); */
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_before W, 0x%X, 0x%X\n",
	ccci_read32(map_addr, AP_PLL_CON0), ccci_read32(map_addr, MDPLL1_CON0));
	/* ccci_write32(map_addr, MDPLL1_CON0, 0x02E9); */
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_after W, 0x%X\n", ccci_read32(map_addr, MDPLL1_CON0));
	/* RAnd2W(map_addr, MDPLL1_CON0, 0xfffffffe); */
	/* RAnd2W(map_addr, MDPLL1_CON0, 0xffffff7f); */

	/* If MD1 only or both MD1 and MD3 */
	md1_pll_on_2(md);
	/* If MD3 only, do nothing */

	RAnd2W(map_addr, MDPLL1_CON0, ~(0x1<<9));
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on, (0x%p)0x%X\n", map_addr, ccci_read32(map_addr, MDPLL1_CON0));
	/* set mdpll control by md1 and c2k */
	/* RAnd2W(map_addr, MDPLL1_CON0, 0xfffffdff); */
}

void md1_pll_init(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	struct md_pll_reg *md_pll = md_ctrl->md_pll_base;
	/* enable L1 permission */
	md1_pll_on(md);
	ROr2W(md_pll->md_peri_misc, R_L1_PMS, 0x7);

	/*
	* From everest and then, PSMCU2EMI bus divider need to be 3,
	* if it used old value 4 as jade, it can be failed in corner case.
	*/
	ROr2W(md_pll->md_sys_clk, R_PSMCU_AO_CLK_CTL, 0x82);

	ROr2W(md_pll->md_L1_a0, R_L1MCU_PWR_AWARE, (1<<16));

	ROr2W(md_pll->md_L1_a0, R_L1AO_PWR_AWARE, (1<<16));
	/*
	* busL2 DCM div 8/normal div 1/ clkslow_en/clock from
	* PLL /debounce enable/ debounce time 7T
	*/
	/* L2DCM L1BUS div 16 */
	ccci_write32(md_pll->md_L1_a0, R_BUSL2DCM_CON3, 0x0000FDE7); /* <= 1FDE7 */
	ccci_write32(md_pll->md_L1_a0, R_BUSL2DCM_CON3, 0x1000FDE7); /* toggle setting */
	/*
	* DCM div 8/normal div 1/clkslow_en/ clock
	* from PLL / dcm enable /debounce enable /debounce time 15T
	*/
	ccci_write32(md_pll->md_L1_a0, R_L1MCU_DCM_CON, 0x0001FDE7);
	/* DCM config toggle = 0 */
	ccci_write32(md_pll->md_L1_a0, R_L1MCU_DCM_CON2, 0x00000000);
	/* DCM config toggle  = 1 / */
	ccci_write32(md_pll->md_L1_a0, R_L1MCU_DCM_CON2, 0x80000000);

	/* Wait PSMCU PLL ready */
	CCCI_DEBUG_LOG(md->index, TAG, "Wait PSMCU PLL ready\n");
	while (RabIsc(md_pll->md_clkSW, R_PLL_STS, 0x1))
		;
	CCCI_DEBUG_LOG(md->index, TAG, "Got it\n");
	/* Switch clock, 0: 26MHz, 1: PLL */
	ROr2W(md_pll->md_clkSW, R_CLKSEL_CTL, 0x2);

	/*Wait L1MCU PLL ready */
	CCCI_BOOTUP_LOG(md->index, TAG, "Wait L1MCU PLL ready\n");
	while (RabIsc(md_pll->md_clkSW, R_PLL_STS, 0x2))
		;
	CCCI_BOOTUP_LOG(md->index, TAG, "Got it\n");
	/* Bit  8: L1MCU_CK = L1MCUPLL */
	ROr2W(md_pll->md_clkSW, R_CLKSEL_CTL, 0x100);

	/*DFE/CMP/ICC/IMC clock src select */
	ccci_write32(md_pll->md_clkSW, R_FLEXCKGEN_SEL1, 0x30302020);
	/* Bit 29-28 DFE_CLK src = DFEPLL,  Bit 21-20 CMP_CLK src = DFEPLL
	* Bit 13-12 ICC_CLK src = IMCPLL,     Bit 5-4   IMC_CLK src = IMCPLL
	*/

	/*IMC/MD2G clock src select */
	ccci_write32(md_pll->md_clkSW, R_FLEXCKGEN_SEL2, 0x00002030);
	/* Bit 13-12 INTF_CLK src = IMCPLL,   Bit 5-4  MD2G_CLK src = DFEPLL */

	/* Wait DFE/IMC PLL ready: Bit  7: DFEPLL_RDY, Bit  4: IMCPLL_RDY */
	while (RabIsc(md_pll->md_clkSW, R_PLL_STS, 0x90))
		;

	/* Wait L1SYS clock ready */
	CCCI_BOOTUP_LOG(md->index, TAG, "Wait L1SYS clock ready\n");
	while (RabIsc(md_pll->md_clkSW, R_FLEXCKGEN_STS0, 0x80800000))
		;

	CCCI_DEBUG_LOG(md->index, TAG, "Done\n");

	CCCI_BOOTUP_LOG(md->index, TAG, "Wait R_FLEXCKGEN_STS1 & 0x80808080 ready\n");
	while (RabIsc(md_pll->md_clkSW, R_FLEXCKGEN_STS1, 0x80808080))
		;

	CCCI_DEBUG_LOG(md->index, TAG, "Done\n");

	CCCI_BOOTUP_LOG(md->index, TAG, "Wait R_FLEXCKGEN_STS2 & 0x8080 ready\n");
	while (RabIsc(md_pll->md_clkSW, R_FLEXCKGEN_STS2, 0x8080))
		;

	CCCI_DEBUG_LOG(md->index, TAG, "Done\n");

	/*Switch L1SYS clock to PLL clock */
	ROr2W(md_pll->md_clkSW, R_CLKSEL_CTL, 0x3fe00);

	/*MD BUS/ARM7 clock src select */
	ccci_write32(md_pll->md_clkSW, R_FLEXCKGEN_SEL0, 0x30203031);

	ccci_write32(md_pll->md_dcm, MD_GLOBAL_CON_DUMMY, MD_PLL_MAGIC_NUM);

	#if 0
	/*PSMCU DCM */
	ROr2W(md_pll->md_dcm, R_PSMCU_DCM_CTL0, 0x00F1F006);

	ROr2W(md_pll->md_dcm, R_PSMCU_DCM_CTL1, 0x26);

	/*ARM7 DCM */
	ROr2W(md_pll->md_dcm, R_ARM7_DCM_CTL0, 0x00F1F006);
	ROr2W(md_pll->md_dcm, R_ARM7_DCM_CTL1, 0x26);

	ccci_write32(md_pll->md_sys_clk, R_DCM_SHR_SET_CTL, 0x00014110);

	/*LTEL2 BUS DCM */
	ROr2W(md_pll->md_sys_clk, R_LTEL2_BUS_DCM_CTL, 0x1);	/* Bit 0: DCM_EN */

	/*MDDMA BUS DCM */
	ROr2W(md_pll->md_sys_clk, R_MDDMA_BUS_DCM_CTL, 0x1);	/* Bit 0: DCM_EN */

	/*MDREG BUS DCM */
	ROr2W(md_pll->md_sys_clk, R_MDREG_BUS_DCM_CTL, 0x1);	/* Bit 0: DCM_EN */

	/*MODULE BUS2X DCM */
	ROr2W(md_pll->md_sys_clk, R_MODULE_BUS2X_DCM_CTL, 0x1);	/* Bit 0: DCM_EN */

	/*MODULE BUS1X DCM */
	ROr2W(md_pll->md_sys_clk, R_MODULE_BUS1X_DCM_CTL, 0x1);	/* Bit 0: DCM_EN */

	/*MD perisys AHB master/slave DCM enable */
	ROr2W(md_pll->md_sys_clk, R_MDINFRA_CKEN, 0xC000001F);

	/*MD debugsys DCM enable */
	ROr2W(md_pll->md_sys_clk, R_MDPERI_CKEN, 0x8003FFFF);

	/*SET MDRGU, MDTOPSM, MDOSTIMER, MDTOPSM DCM MASK */
	ROr2W(md_pll->md_sys_clk, R_MDPERI_DCM_MASK, 0x00001E00);
	#endif

	ROr2W(md_pll->md_L1_a0, REG_DCM_PLLCK_SEL, (1<<7));

	/* wait DCM config done, then switch BUS clock src to PLL */
	CCCI_BOOTUP_LOG(md->index, TAG, "wait DCM config done\n");
	while (RabIsc(md_pll->md_clkSW, R_FLEXCKGEN_STS0, 0x80))
		;
	CCCI_BOOTUP_LOG(md->index, TAG, "done\n");
	/* Bit  1: BUS_CLK = EQPLL/2 */
	ROr2W(md_pll->md_clkSW, R_CLKSEL_CTL, 0x1);
}

int md_cd_soft_power_off(struct ccci_modem *md, unsigned int mode)
{
#ifdef FEATURE_RF_CLK_BUF
	clk_buf_set_by_flightmode(true);
#endif
#if !defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	pmic_config_interface(0x404, 3, 3, 4);
	CCCI_NORMAL_LOG(md->index, TAG, "md_cd_soft_power_off set pmic done\n");
#endif
	return 0;
}

int md_cd_soft_power_on(struct ccci_modem *md, unsigned int mode)
{
#ifdef FEATURE_RF_CLK_BUF
	clk_buf_set_by_flightmode(false);
#endif
#if !defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	pmic_config_interface(0x404, 0, 3, 4);
	CCCI_NORMAL_LOG(md->index, TAG, "md_cd_soft_power_on set pmic done\n");
#endif
	return 0;
}

int md_cd_power_on(struct ccci_modem *md)
{
	int ret = 0;
	unsigned int reg_value;
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	CCCI_BOOTUP_LOG(md->index, TAG, "Call md1_pmic_setting_on\n");
	/* step 0: PMIC setting */
	md1_pmic_setting_on();

	/* steip 1: power on MD_INFRA and MODEM_TOP */
	if (md->index == MD_SYS1) {
		md_cd_soft_power_on(md, 0);
		CCCI_BOOTUP_LOG(md->index, TAG, "enable md sys clk\n");
		ret = clk_prepare_enable(clk_table[0].clk_ref);
		CCCI_BOOTUP_LOG(md->index, TAG, "enable md sys clk done,ret = %d\n", ret);
		kicker_pbm_by_md(KR_MD1, true);
		CCCI_BOOTUP_LOG(md->index, TAG, "Call end kicker_pbm_by_md(0,true)\n");
	} else {
		CCCI_BOOTUP_LOG(md->index, TAG, "md power on index error = %d\n", md->index);
		ret = -1;
	}
	if (ret)
		return ret;

	/* step 3: MD srcclkena setting */
	reg_value = ccci_read32(infra_ao_base, INFRA_AO_MD_SRCCLKENA);

#if defined(CONFIG_MTK_MD3_SUPPORT) && (CONFIG_MTK_MD3_SUPPORT > 0)
	reg_value &= ~(0x92);	/* md1 set 0x29: bit 0/3/4/7, bit1/5: VRF18 control for Jade */
	reg_value |= 0x29;	/* C2K set |0x44: bit 2/6 */
#else
	reg_value &= ~(0xFF);
	reg_value |= 0x29;
#endif
	ccci_write32(infra_ao_base, INFRA_AO_MD_SRCCLKENA, reg_value);
	CCCI_BOOTUP_LOG(md->index, CORE, "md_cd_power_on: set md1_srcclkena bit(0x1000_1F0C)=0x%x\n",
		     ccci_read32(infra_ao_base, INFRA_AO_MD_SRCCLKENA));

#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
	/* notify NFC */
	inform_nfc_vsim_change(md->index, 1, 0);
#endif
	/* step 4: pll init */
	md1_pll_init(md);

	/* step 5: disable MD WDT */
	ccci_write32(md_ctrl->md_rgu_base, WDT_MD_MODE, WDT_MD_MODE_KEY);
	ccci_write32(md_ctrl->l1_rgu_base, REG_L1RSTCTL_WDT_MODE, L1_WDT_MD_MODE_KEY);

#ifdef SET_EMI_STEP_BY_STAGE
	CCCI_BOOTUP_LOG(md->index, KERN, "set domain register\n");
	ccci_write32(md_ctrl->md_pll_base->md_busreg1, 0xC4, 0x7);
	ccci_write32(md_ctrl->md_pll_base->md_L1_a0, 0x220, 0x1);
	ccci_write32(md_ctrl->md_pll_base->md_L1_a0, 0x220, 0x81);
#endif

	md1_pcore_sram_on(md);
	return 0;
}

int md_cd_bootup_cleanup(struct ccci_modem *md, int success)
{
	return 0;
}

int md_cd_let_md_go(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	if (MD_IN_DEBUG(md))
		return -1;
	CCCI_BOOTUP_LOG(md->index, TAG, "set MD boot slave\n");

	ccci_write32(md_ctrl->md_boot_slave_En, 0, 1);	/* make boot vector take effect */
	return 0;
}

int md_cd_power_off(struct ccci_modem *md, unsigned int stop_type)
{
	int ret = 0;

#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
	/* notify NFC */
	inform_nfc_vsim_change(md->index, 0, 0);
#endif

	/* power off MD_INFRA and MODEM_TOP */
	if (md->index == MD_SYS1) {
		clk_disable_unprepare(clk_table[0].clk_ref);
		CCCI_BOOTUP_LOG(md->index, TAG, "disble md1 clk\n");
		md_cd_soft_power_off(md, 0);
		CCCI_BOOTUP_LOG(md->index, TAG, "Call md1_pmic_setting_off\n");
		md1_pmic_setting_off();
		kicker_pbm_by_md(KR_MD1, false);
		CCCI_BOOTUP_LOG(md->index, TAG, "Call end kicker_pbm_by_md(0,false)\n");
	} else {
		CCCI_BOOTUP_LOG(md->index, TAG, "md power off wrong index error = %d\n", md->index);
		ret = -1;
	}
	return ret;
}

void cldma_dump_register(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Tx pdn register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_UL_START_ADDR_0,
		      CLDMA_AP_UL_LAST_UPDATE_ADDR_4MSB - CLDMA_AP_UL_START_ADDR_0 + 4);
	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Tx ao register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_ao_base + CLDMA_AP_UL_START_ADDR_BK_0,
		      CLDMA_AP_UL_CURRENT_ADDR_BK_4MSB - CLDMA_AP_UL_START_ADDR_BK_0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Rx pdn register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_SO_ERROR,
		      CLDMA_AP_SO_STOP_CMD - CLDMA_AP_SO_ERROR + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Rx ao register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_ao_base + CLDMA_AP_SO_CFG,
		      CLDMA_AP_SO_NEXT_BUF_PTR_4MSB - CLDMA_AP_SO_CFG + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA MISC pdn register\n");
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_L2TISAR0,
		      CLDMA_AP_L3TIMSR2 - CLDMA_AP_L2TISAR0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA MISC ao register\n");
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_ao_base + CLDMA_AP_L2RIMR0,
						CLDMA_AP_DUMMY - CLDMA_AP_L2RIMR0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Tx pdn register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_pdn_base + CLDMA_AP_UL_START_ADDR_0,
		      CLDMA_AP_UL_LAST_UPDATE_ADDR_4MSB - CLDMA_AP_UL_START_ADDR_0 + 4);
	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Tx ao register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_ao_base + CLDMA_AP_UL_START_ADDR_BK_0,
		      CLDMA_AP_UL_CURRENT_ADDR_BK_4MSB - CLDMA_AP_UL_START_ADDR_BK_0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Rx pdn register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_pdn_base + CLDMA_AP_SO_ERROR,
		      CLDMA_AP_SO_STOP_CMD - CLDMA_AP_SO_ERROR + 4);
	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Rx ao register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_ao_base + CLDMA_AP_SO_CFG,
		      CLDMA_AP_SO_NEXT_BUF_PTR_4MSB - CLDMA_AP_SO_CFG + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA MISC pdn register\n");
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_pdn_base + CLDMA_AP_L2TISAR0,
		      CLDMA_AP_L3TIMSR2 - CLDMA_AP_L2TISAR0 + 4);
	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA MISC ao register\n");
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_ao_base + CLDMA_AP_L2RIMR0,
						CLDMA_AP_DUMMY - CLDMA_AP_L2RIMR0 + 4);

}

int ccci_modem_remove(struct platform_device *dev)
{
	return 0;
}

void ccci_modem_shutdown(struct platform_device *dev)
{
}

int ccci_modem_suspend(struct platform_device *dev, pm_message_t state)
{
	struct ccci_modem *md = (struct ccci_modem *)dev->dev.platform_data;

	CCCI_NORMAL_LOG(md->index, TAG, "ccci_modem_suspend\n");
	return 0;
}

int ccci_modem_resume(struct platform_device *dev)
{
	struct ccci_modem *md = (struct ccci_modem *)dev->dev.platform_data;

	CCCI_NORMAL_LOG(md->index, TAG, "ccci_modem_resume\n");
	return 0;
}

int ccci_modem_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	if (pdev == NULL) {
		CCCI_ERROR_LOG(MD_SYS1, TAG, "%s pdev == NULL\n", __func__);
		return -1;
	}
	return ccci_modem_suspend(pdev, PMSG_SUSPEND);
}

int ccci_modem_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	if (pdev == NULL) {
		CCCI_ERROR_LOG(MD_SYS1, TAG, "%s pdev == NULL\n", __func__);
		return -1;
	}
	return ccci_modem_resume(pdev);
}

int ccci_modem_pm_restore_noirq(struct device *device)
{
	struct ccci_modem *md = (struct ccci_modem *)device->platform_data;
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	/* set flag for next md_start */
	md->config.setting |= MD_SETTING_RELOAD;
	md->config.setting |= MD_SETTING_FIRST_BOOT;
	/* restore IRQ */
#ifdef FEATURE_PM_IPO_H
	irq_set_irq_type(md_ctrl->cldma_irq_id, IRQF_TRIGGER_HIGH);
	irq_set_irq_type(md_ctrl->md_wdt_irq_id, IRQF_TRIGGER_FALLING);
#endif
	return 0;
}

void ccci_modem_restore_reg(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	int i;
	unsigned long flags;
	unsigned int val = 0;
	dma_addr_t bk_addr = 0;

	if (md->md_state == GATED || md->md_state == WAITING_TO_STOP || md->md_state == INVALID) {
		CCCI_NORMAL_LOG(md->index, TAG, "Resume no need reset cldma for md_state=%d\n", md->md_state);
		return;
	}
	ccci_write32(md_ctrl->ap_ccif_base, APCCIF_CON, 0x01);	/* arbitration */

	if (ccci_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_TQSAR(0))
		|| cldma_reg_get_4msb_val(md_ctrl->cldma_ap_ao_base,
					CLDMA_AP_UL_START_ADDR_4MSB, md_ctrl->txq[0].index)) {
		CCCI_NORMAL_LOG(md->index, TAG, "Resume cldma pdn register: No need  ...\n");
	} else {
		CCCI_NORMAL_LOG(md->index, TAG, "Resume cldma pdn register ...11\n");
		spin_lock_irqsave(&md_ctrl->cldma_timeout_lock, flags);
		ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_HPQR, 0x00);
		/* set checksum */
		switch (CHECKSUM_SIZE) {
		case 0:
			ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE, 0);
			break;
		case 12:
			ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE,
				      CLDMA_BM_ALL_QUEUE);
			ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG,
				      ccci_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG) & ~0x10);
			break;
		case 16:
			ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE,
				      CLDMA_BM_ALL_QUEUE);
			ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG,
				      ccci_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG) | 0x10);
			break;
		}
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
		/* re-config 8G mode flag for pd register*/
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG,
				      cldma_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG) | 0x40);
#endif
		/* set start address */
		for (i = 0; i < QUEUE_LEN(md_ctrl->txq); i++) {
			if (ccci_read32(md_ctrl->cldma_ap_ao_base, CLDMA_AP_TQCPBAK(md_ctrl->txq[i].index)) == 0
				 && cldma_reg_get_4msb_val(md_ctrl->cldma_ap_ao_base,
					CLDMA_AP_UL_CURRENT_ADDR_BK_4MSB, md_ctrl->txq[i].index) == 0) {
				if (i != 7) /* Queue 7 not used currently */
					CCCI_NORMAL_LOG(md->index, TAG, "Resume CH(%d) current bak:== 0\n", i);
				cldma_reg_set_tx_start_addr(md_ctrl->cldma_ap_pdn_base,
					md_ctrl->txq[i].index,
					md_ctrl->txq[i].tr_done->gpd_addr);
				cldma_reg_set_tx_start_addr_bk(md_ctrl->cldma_ap_ao_base,
					md_ctrl->txq[i].index,
					md_ctrl->txq[i].tr_done->gpd_addr);
			} else {
				if (sizeof(dma_addr_t) > sizeof(u32)) {
					val = cldma_reg_get_4msb_val(md_ctrl->cldma_ap_ao_base,
							CLDMA_AP_UL_CURRENT_ADDR_BK_4MSB,
							md_ctrl->txq[i].index);
					/*set high bits*/
					bk_addr = val;
					bk_addr <<= 32;
				} else
					bk_addr = 0;
				/*set low bits*/
				val = ccci_read32(md_ctrl->cldma_ap_ao_base,
									CLDMA_AP_TQCPBAK(md_ctrl->txq[i].index));
				bk_addr |= val;
				cldma_reg_set_tx_start_addr(md_ctrl->cldma_ap_pdn_base,
					md_ctrl->txq[i].index, bk_addr);
				cldma_reg_set_tx_start_addr_bk(md_ctrl->cldma_ap_ao_base,
					md_ctrl->txq[i].index, bk_addr);
			}
		}
		/* wait write done*/
		wmb();
		/* start all Tx and Rx queues */
#ifdef NO_START_ON_SUSPEND_RESUME
		md_ctrl->txq_started = 0;
#else
		ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_START_CMD, CLDMA_BM_ALL_QUEUE);
		ccci_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_START_CMD);	/* dummy read */
#endif
		md_ctrl->txq_active |= CLDMA_BM_ALL_QUEUE;
		/* ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_SO_START_CMD, CLDMA_BM_ALL_QUEUE); */
		/* ccci_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_SO_START_CMD); // dummy read */
		/* md_ctrl->rxq_active |= CLDMA_BM_ALL_QUEUE; */
		/* enable L2 DONE and ERROR interrupts */
		ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L2TIMCR0, CLDMA_BM_INT_DONE | CLDMA_BM_INT_ERROR);
		/* enable all L3 interrupts */
		ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3TIMCR0, CLDMA_BM_INT_ALL);
		ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3TIMCR1, CLDMA_BM_INT_ALL);
		ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3RIMCR0, CLDMA_BM_INT_ALL);
		ccci_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3RIMCR1, CLDMA_BM_INT_ALL);
		spin_unlock_irqrestore(&md_ctrl->cldma_timeout_lock, flags);
		CCCI_NORMAL_LOG(md->index, TAG, "Resume cldma pdn register done\n");
	}
}

int ccci_modem_syssuspend(void)
{
	CCCI_NORMAL_LOG(0, TAG, "ccci_modem_syssuspend\n");
	return 0;
}

void ccci_modem_sysresume(void)
{
	struct ccci_modem *md;

	CCCI_NORMAL_LOG(0, TAG, "ccci_modem_sysresume\n");
	md = ccci_md_get_modem_by_id(0);
	if (md != NULL)
		ccci_modem_restore_reg(md);
}
