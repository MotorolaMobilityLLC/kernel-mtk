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
#if defined(CONFIG_MTK_CLKMGR)
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif/*CONFIG_MTK_CLKMGR */
#include <mach/mt_pbm.h>

#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
#include <mach/mt6605.h>
#endif


#ifdef FEATURE_RF_CLK_BUF
#include <mt_clkbuf_ctl.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mach/mt_pbm.h>
#include <mt_spm_sleep.h>

#include "ccci_core.h"
#include "ccci_platform.h"
#include "modem_cldma.h"
#include "cldma_platform.h"
#include "cldma_reg.h"
#include "modem_reg_base.h"

#if !defined(CONFIG_MTK_CLKMGR)
static struct clk *clk_scp_sys_md1_main;
static struct clk *clk_infra_ccif_ap;
static struct clk *clk_infra_ccif_md;
static struct clk *clk_infra_ap_c2k_ccif_0;
static struct clk *clk_infra_ap_c2k_ccif_1;
static struct clk *clk_infra_md2md_ccif_0;
static struct clk *clk_infra_md2md_ccif_1;
static struct clk *clk_infra_md2md_ccif_2;
static struct clk *clk_infra_md2md_ccif_3;
static struct clk *clk_infra_md2md_ccif_4;
static struct clk *clk_infra_md2md_ccif_5;
#endif
#if defined(CONFIG_PINCTRL_MT6797)
static struct pinctrl *mdcldma_pinctrl;
#endif
void __attribute__((weak)) clk_buf_set_by_flightmode(bool is_flightmode_on)
{
}

#define TAG "mcd"
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
		hw_info->ap2md_bus_timeout_irq_flags = IRQF_TRIGGER_NONE;

		hw_info->sram_size = CCIF_SRAM_SIZE;
		hw_info->md_rgu_base = MD_RGU_BASE;
		hw_info->l1_rgu_base = L1_RGU_BASE;
		hw_info->md_boot_slave_En = MD_BOOT_VECTOR_EN;
#if !defined(CONFIG_MTK_CLKMGR)
#if defined(CONFIG_PINCTRL_MT6797)
		mdcldma_pinctrl = devm_pinctrl_get(&dev_ptr->dev);
		if (IS_ERR(mdcldma_pinctrl)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get mdcldma_pinctrl failed\n",
							dev_cfg->index + 1);
			return -1;
		}
#else
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "gpio pinctrl is not ready yet, use workaround.\n");
#endif
		clk_scp_sys_md1_main = devm_clk_get(&dev_ptr->dev, "scp-sys-md1-main");
		if (IS_ERR(clk_scp_sys_md1_main)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get scp-sys-md1-main failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_ccif_ap = devm_clk_get(&dev_ptr->dev, "infra-ccif-ap");
		if (IS_ERR(clk_infra_ccif_ap)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get scp-sys-md1-main failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_ccif_md = devm_clk_get(&dev_ptr->dev, "infra-ccif-md");
		if (IS_ERR(clk_infra_ccif_md)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get scp-sys-md1-main failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_ap_c2k_ccif_0 = devm_clk_get(&dev_ptr->dev, "infra-ap-c2k-ccif-0");
		if (IS_ERR(clk_infra_ap_c2k_ccif_0)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-ap-c2k-ccif-0 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_ap_c2k_ccif_1 = devm_clk_get(&dev_ptr->dev, "infra-ap-c2k-ccif-1");
		if (IS_ERR(clk_infra_ap_c2k_ccif_1)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-ap-c2k-ccif-1 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_md2md_ccif_0 = devm_clk_get(&dev_ptr->dev, "infra-md2md-ccif-0");
		if (IS_ERR(clk_infra_md2md_ccif_0)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-md2md-ccif-0 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_md2md_ccif_1 = devm_clk_get(&dev_ptr->dev, "infra-md2md-ccif-1");
		if (IS_ERR(clk_infra_md2md_ccif_1)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-md2md-ccif-1 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_md2md_ccif_2 = devm_clk_get(&dev_ptr->dev, "infra-md2md-ccif-2");
		if (IS_ERR(clk_infra_md2md_ccif_2)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-md2md-ccif-2 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_md2md_ccif_3 = devm_clk_get(&dev_ptr->dev, "infra-md2md-ccif-3");
		if (IS_ERR(clk_infra_md2md_ccif_3)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-md2md-ccif-3 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_md2md_ccif_4 = devm_clk_get(&dev_ptr->dev, "infra-md2md-ccif-4");
		if (IS_ERR(clk_infra_md2md_ccif_4)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-md2md-ccif-4 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
		clk_infra_md2md_ccif_5 = devm_clk_get(&dev_ptr->dev, "infra-md2md-ccif-5");
		if (IS_ERR(clk_infra_md2md_ccif_5)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get infra-md2md-ccif-5 failed\n",
							dev_cfg->index + 1);
			return -1;
		}
#endif
		node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
		hw_info->ap_mixed_base = (unsigned long)of_iomap(node, 0);
		break;
	default:
		return -1;
	}

	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "dev_major:%d,minor_base:%d,capability:%d\n",
							dev_cfg->major,
		     dev_cfg->minor_base, dev_cfg->capability);
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
void ccci_set_clk_cg(struct ccci_modem *md, unsigned int on)
{
#if !defined(CONFIG_MTK_CLKMGR)
	if (on) {
		clk_prepare_enable(clk_infra_ccif_ap);
		clk_prepare_enable(clk_infra_ccif_md);
		clk_prepare_enable(clk_infra_ap_c2k_ccif_0);
		clk_prepare_enable(clk_infra_ap_c2k_ccif_1);
		clk_prepare_enable(clk_infra_md2md_ccif_0);
		clk_prepare_enable(clk_infra_md2md_ccif_1);
		clk_prepare_enable(clk_infra_md2md_ccif_2);
		clk_prepare_enable(clk_infra_md2md_ccif_3);
		clk_prepare_enable(clk_infra_md2md_ccif_4);
		clk_prepare_enable(clk_infra_md2md_ccif_5);
	} else {
		clk_disable_unprepare(clk_infra_ccif_ap);
		clk_disable_unprepare(clk_infra_ccif_md);
		clk_disable_unprepare(clk_infra_ap_c2k_ccif_0);
		clk_disable_unprepare(clk_infra_ap_c2k_ccif_1);
		clk_disable_unprepare(clk_infra_md2md_ccif_0);
		clk_disable_unprepare(clk_infra_md2md_ccif_1);
		clk_disable_unprepare(clk_infra_md2md_ccif_2);
		clk_disable_unprepare(clk_infra_md2md_ccif_3);
		clk_disable_unprepare(clk_infra_md2md_ccif_4);
		clk_disable_unprepare(clk_infra_md2md_ccif_5);
	}
#endif
}
#endif

int md_cd_io_remap_md_side_register(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	struct md_pll_reg *md_reg;

	md_ctrl->cldma_ap_pdn_base = (void __iomem *)(md_ctrl->hw_info->cldma_ap_pdn_base);
	md_ctrl->cldma_ap_ao_base = (void __iomem *)(md_ctrl->hw_info->cldma_ap_ao_base);
	md_ctrl->cldma_md_pdn_base = (void __iomem *)(md_ctrl->hw_info->cldma_md_pdn_base);
	md_ctrl->cldma_md_ao_base = (void __iomem *)(md_ctrl->hw_info->cldma_md_ao_base);
	md_ctrl->md_boot_slave_En = ioremap_nocache(md_ctrl->hw_info->md_boot_slave_En, 0x4);
	md_ctrl->md_rgu_base = ioremap_nocache(md_ctrl->hw_info->md_rgu_base, 0x300);
	md_ctrl->l1_rgu_base = ioremap_nocache(md_ctrl->hw_info->l1_rgu_base, 0x40);
	md_ctrl->md_global_con0 = ioremap_nocache(MD_GLOBAL_CON0, 0x4);

	md_ctrl->md_bus_status = ioremap_nocache(MD_BUS_STATUS_BASE, MD_BUS_STATUS_LENGTH);
	/* md_ctrl->md_pc_monitor = ioremap_nocache(MD_PC_MONITOR_BASE, MD_PC_MONITOR_LENGTH); */
	md_ctrl->md_topsm_status = ioremap_nocache(MD_TOPSM_STATUS_BASE, MD_TOPSM_STATUS_LENGTH);
	md_ctrl->md_ost_status = ioremap_nocache(MD_OST_STATUS_BASE, MD_OST_STATUS_LENGTH);
	/* md_ctrl->md_pll = ioremap_nocache(MD_PLL_BASE, MD_PLL_LENGTH); */

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
	md_reg->psmcu_misc = ioremap_nocache(PSMCU_MISC_BASE, 4);
	md_reg->md_peri_misc = ioremap_nocache(MD_PERI_MISC_BASE, MD_PERI_MISC_LEN);
	md_reg->md_L1_a0 = ioremap_nocache(MDL1A0_BASE, MDL1A0_LEN);
	md_reg->md_top_Pll = ioremap_nocache(MDTOP_PLLMIXED_BASE, 4);
	md_reg->md_sys_clk = ioremap_nocache(MDSYS_CLKCTL_BASE, MDSYS_CLKCTL_LEN);
	/*md_reg->md_l1_conf = ioremap_nocache(L1_BASE_MADDR_MDL1_CONF, 4);*/

	md_reg->md_busreg1 = ioremap_nocache(MD_BUSREG_DUMP_ADDR1, MD_BUSREG_DUMP_LEN1);
	md_reg->md_busreg2 = ioremap_nocache(MD_BUSREG_DUMP_ADDR2, MD_BUSREG_DUMP_LEN2);
	md_reg->md_busrec = ioremap_nocache(MD_BUSREC_DUMP_ADDR, MD_BUSREC_DUMP_LEN);
	md_reg->md_ect_0 = ioremap_nocache(MD_ECT_DUMP_ADDR0, MD_ECT_DUMP_LEN0);
	md_reg->md_ect_1 = ioremap_nocache(MD_ECT_DUMP_ADDR1, MD_ECT_DUMP_LEN1);
	md_reg->md_ect_2 = ioremap_nocache(MD_ECT_DUMP_ADDR2, MD_ECT_DUMP_LEN2);
	md_reg->md_ect_3 = ioremap_nocache(MD_ECT_DUMP_ADDR3, MD_ECT_DUMP_LEN3);
	md_reg->md_bootup_0 = ioremap_nocache(MD_Bootup_DUMP_ADDR0, MD_Bootup_DUMP_LEN0);
	md_reg->md_bootup_1 = ioremap_nocache(MD_Bootup_DUMP_ADDR1, MD_Bootup_DUMP_LEN1);
	md_reg->md_bootup_2 = ioremap_nocache(MD_Bootup_DUMP_ADDR2, MD_Bootup_DUMP_LEN2);
	md_reg->md_bootup_3 = ioremap_nocache(MD_Bootup_DUMP_ADDR3, MD_Bootup_DUMP_LEN3);
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

	md_ctrl->md_pll_base = md_reg;

#ifdef MD_PEER_WAKEUP
	md_ctrl->md_peer_wakeup = ioremap_nocache(MD_PEER_WAKEUP, 0x4);
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
	/*dump in AP infra side*/
	/*To avoid AP/MD interface delay, dump 3 times, and buy-in the 3rd dump value.*/
	void __iomem *md_boot_stats0, *md_boot_stats1;

	md_boot_stats0 = ioremap_nocache(MD1_CFG_BOOT_STATS0, 0x4);
	md_boot_stats1 = ioremap_nocache(MD1_CFG_BOOT_STATS1, 0x4);

	cldma_read32(md_boot_stats0, 0);	/* dummy read */
	cldma_read32(md_boot_stats0, 0);	/* dummy read */
	CCCI_NOTICE_LOG(md->index, TAG, "md_boot_stats0:0x%X\n", cldma_read32(md_boot_stats0, 0));

	cldma_read32(md_boot_stats1, 0);	/* dummy read */
	cldma_read32(md_boot_stats1, 0);	/* dummy read */
	CCCI_NOTICE_LOG(md->index, TAG, "md_boot_stats1:0x%X\n", cldma_read32(md_boot_stats1, 0));

	iounmap(md_boot_stats0);
	iounmap(md_boot_stats1);
}

void md_cd_dump_debug_register(struct ccci_modem *md)
{
#if 1 /* MD no need dump because of bus hang happened - open for debug */
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
#if 1
	unsigned int reg_value;
	void __iomem *md_addr;
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
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5);/* pre-action: permission */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP,
				md_ctrl->md_topsm_status, MD_TOPSM_STATUS_LENGTH);
	}
	/* 3. PC Monitor */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_PCMON)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD PC monitor 0x%x\n", (MD_PC_MONITOR_BASE + 0x100));
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5);/* pre-action: permission */
		/* pre-action: Open Dbgsys clock */
		md_addr = md_reg->md_ect_0;
		reg_value = ccci_read32(md_addr, 4);
		reg_value |= (0x1 << 3);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]write: %p=0x%x\n", (md_addr + 4), reg_value);
		ccci_write32(md_addr, 4, reg_value);	/* clear bit[29] */
		reg_value = ccci_read32(md_addr, 4);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] read: %p=0x%x\n", (md_addr + 4), reg_value);
		reg_value = ccci_read32(md_addr, 0x20);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] before %p=0x%x\n", (md_addr + 0x20), reg_value);
		while (!(ccci_read32(md_addr, 0x20)&(1<<3)))
			;
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
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5); /* pre-action */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_rgu_base, 0x8B);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_ctrl->md_rgu_base + 0x200), 0x60);
	}
	/* 5 OST */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_OST)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD OST status %x\n", MD_OST_STATUS_BASE);
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5); /* pre-action */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_ost_status, MD_OST_STATUS_LENGTH);
	}
	/* 6. Bus status */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_BUS)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD Bus status %x\n", MD_BUS_STATUS_BASE);
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5);/* pre-action: permission */
		ccci_write32(md_reg->md_busreg1, 0x9C, 0x65);/* pre-action: permission */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_bus_status, 0x38);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, (md_ctrl->md_bus_status + 0x100), 0x30);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_busreg1, MD_BUSREG_DUMP_LEN1);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_busreg2, MD_BUSREG_DUMP_LEN2);
	}
	/* 7. dump PLL */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_PLL)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD PLL\n");
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5);/* pre-action: permission */
		ccci_write32(md_reg->md_busreg1, 0x9C, 0x65);/* pre-action: permission */
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
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD Bus REC%x\n", MD_BUSREC_DUMP_ADDR);
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5);/* pre-action: permission */
		/* pre-action: Open Dbgsys clock */
		md_addr = md_reg->md_ect_0;
		reg_value = ccci_read32(md_addr, 4);
		reg_value |= (0x1 << 3);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]write: %p=0x%x\n", (md_addr + 4), reg_value);
		ccci_write32(md_addr, 4, reg_value);	/* clear bit[29] */
		reg_value = ccci_read32(md_addr, 4);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] read: %p=0x%x\n", (md_addr + 4), reg_value);
		reg_value = ccci_read32(md_addr, 0x20);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] before %p=0x%x\n", (md_addr + 0x20), reg_value);
		while (!(ccci_read32(md_addr, 0x20)&(1<<3)))
			;
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]after 0x%x\n", reg_value);
		ccci_write32(md_reg->md_busrec, 0x4, 0x1);/* pre-action */
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_busrec, MD_BUSREC_DUMP_LEN);
		ccci_write32(md_reg->md_busrec, 0x4, 0x3);/* post-action */
	}
	/* 9. ECT: must after 4 TO PSM */
	if (md->md_dbg_dump_flag & (1 << MD_DBG_DUMP_ECT)) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD ECT 0x%x\n", MD_ECT_DUMP_ADDR0);
		ccci_write32(md_reg->md_busreg1, 0x94, 0xE7C5);/* pre-action: permission */
		/* pre-action: Open Dbgsys clock */
		md_addr = md_reg->md_ect_0;
		reg_value = ccci_read32(md_addr, 4);
		reg_value |= (0x1 << 3);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]write: %p=0x%x\n", (md_addr + 4), reg_value);
		ccci_write32(md_addr, 4, reg_value);	/* clear bit[29] */
		reg_value = ccci_read32(md_addr, 4);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] read: %p=0x%x\n", (md_addr + 4), reg_value);
		reg_value = ccci_read32(md_addr, 0x20);
		CCCI_MEM_LOG(md->index, TAG, "[pre-action] before %p=0x%x\n", (md_addr + 0x20), reg_value);
		while (!(ccci_read32(md_addr, 0x20)&(1<<3)))
			;
		CCCI_MEM_LOG(md->index, TAG, "[pre-action]after 0x%x\n", reg_value);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_ect_1, MD_ECT_DUMP_LEN1);
		ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_reg->md_ect_2, MD_ECT_DUMP_LEN2);
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


void md1_pmic_setting_off(void)
{
	/* VMODEM off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN, 0); /* 0x063A[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_EN, 0); /* 0x062C[0]=0, 0:Disable, 1:Enable */
	/* VMD1 off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_VSLEEP_EN, 0); /* 0x064E[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_EN, 0); /* 0x0640[0]=0, 0:Disable, 1:Enable */
	/* VSRAM_MD off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN, 0); /* 0x0662[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_EN, 0); /* 0x0654[0]=0, 0:Disable, 1:Enable */
}

void md1_pmic_setting_on(void)
{
	/* VSRAM_MD on */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_EN, 1); /* 0x0654[0]=0, 0:Disable, 1:Enable */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN, 1); /* 0x0662[8]=0, 0:SW control, 1:HW control */

	/* VMD1 on */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_EN, 1); /* 0x0640[0]=0, 0:Disable, 1:Enable */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_VSLEEP_EN, 1); /* 0x064E[8]=0, 0:SW control, 1:HW control */

	/* VMODEM on */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_EN, 1); /* 0x062C[0]=0, 0:Disable, 1:Enable */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN, 1); /* 0x063A[8]=0, 0:SW control, 1:HW control */

	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_ON, 0x50);/*E1 1.0V; offset:0x65A */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VOSEL_ON, 0x40);/* 1.0V; offset: 0x632 */
	udelay(300);

	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_CTRL, 1);/* HW mode, bit[1]; offset: 0x650 */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_VOSEL_CTRL, 1);/* HW mode, bit[1]; offset: 0x63C */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VOSEL_CTRL, 1);/* HW mode, bit[1]; offset: 0x628 */
}

/* callback for system power off*/
void ccci_power_off(void)
{
	md1_pmic_setting_on();
}

#define ROr2W(a, b, c)  cldma_write32(a, b, (cldma_read32(a, b)|c))
#define RAnd2W(a, b, c)  cldma_write32(a, b, (cldma_read32(a, b)&c))
#define RabIsc(a, b, c) ((cldma_read32(a, b)&c) != c)

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
	cldma_write32(map_addr, MDPLL1_CON0, 0x2EE8);
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_reset_value, (0x%p)0x%X\n",
		map_addr, cldma_read32(map_addr, MDPLL1_CON0));

	/* If MD1 only or both MD1 and MD3 */
	md1_pll_on_1(md);
	/* If MD3 only, do nothing */

	/* Turn on 208M */
	ROr2W(map_addr, MDPLL1_CON0, (0x1));

	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_before W, (0x%p)0x%X\n",
		map_addr, cldma_read32(map_addr, AP_PLL_CON0));
	/* cldma_write32(map_addr, AP_PLL_CON0, 0x39F1); */
	ROr2W(map_addr, AP_PLL_CON0, (0x1<<1));

	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_after W, 0x%X\n", cldma_read32(map_addr, AP_PLL_CON0));

	udelay(200);

	RAnd2W(map_addr, MDPLL1_CON0, ~(0x1));
	RAnd2W(map_addr, MDPLL1_CON0, ~(0x1<<7));

	/* close 208M and autoK */
	/* cldma_write32(map_addr, AP_PLL_CON0, 0x39F3); */
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_before W, 0x%X, 0x%X\n",
		cldma_read32(map_addr, AP_PLL_CON0), cldma_read32(map_addr, MDPLL1_CON0));
	/* cldma_write32(map_addr, MDPLL1_CON0, 0x02E9); */
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on_after W, 0x%X\n", cldma_read32(map_addr, MDPLL1_CON0));
	/* RAnd2W(map_addr, MDPLL1_CON0, 0xfffffffe); */
	/* RAnd2W(map_addr, MDPLL1_CON0, 0xffffff7f); */

	/* If MD1 only or both MD1 and MD3 */
	md1_pll_on_2(md);
	/* If MD3 only, do nothing */

	RAnd2W(map_addr, MDPLL1_CON0, ~(0x1<<9));
	CCCI_DEBUG_LOG(md->index, TAG, "md1_pll_on, (0x%p)0x%X\n", map_addr, cldma_read32(map_addr, MDPLL1_CON0));
	/* RAnd2W(map_addr, MDPLL1_CON0, 0xfffffdff); */ /* set mdpll control by md1 and c2k */
}

void md1_pll_init(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	struct md_pll_reg *md_pll = md_ctrl->md_pll_base;
	/* enable L1 permission */
	md1_pll_on(md);
	ROr2W(md_pll->md_peri_misc, R_L1_PMS, 0x7);

	/* From everest and then, PSMCU2EMI bus divider need to be 3,
	 * if it used old value 4 as jade, it can be failed in corner case. */
	ROr2W(md_pll->md_sys_clk, R_PSMCU_AO_CLK_CTL, 0x82);

	ROr2W(md_pll->md_L1_a0, R_L1MCU_PWR_AWARE, (1<<16));

	ROr2W(md_pll->md_L1_a0, R_L1AO_PWR_AWARE, (1<<16));
	/* busL2 DCM div 8/normal div 1/ clkslow_en/clock from
	   PLL /debounce enable/ debounce time 7T */
	/* L2DCM L1BUS div 16 */
	cldma_write32(md_pll->md_L1_a0, R_BUSL2DCM_CON3, 0x0000FDE7); /* <= 1FDE7 */
	cldma_write32(md_pll->md_L1_a0, R_BUSL2DCM_CON3, 0x1000FDE7); /* toggle setting */
	/* DCM div 8/normal div 1/clkslow_en/ clock
	   from PLL / dcm enable /debounce enable /debounce time 15T */
	cldma_write32(md_pll->md_L1_a0, R_L1MCU_DCM_CON, 0x0001FDE7);
	/* DCM config toggle = 0 */
	cldma_write32(md_pll->md_L1_a0, R_L1MCU_DCM_CON2, 0x00000000);
	/* DCM config toggle  = 1 / */
	cldma_write32(md_pll->md_L1_a0, R_L1MCU_DCM_CON2, 0x80000000);

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
	cldma_write32(md_pll->md_clkSW, R_FLEXCKGEN_SEL1, 0x30302020);
	/* Bit 29-28 DFE_CLK src = DFEPLL,  Bit 21-20 CMP_CLK src = DFEPLL
	   Bit 13-12 ICC_CLK src = IMCPLL,     Bit 5-4   IMC_CLK src = IMCPLL */

	/*IMC/MD2G clock src select */
	cldma_write32(md_pll->md_clkSW, R_FLEXCKGEN_SEL2, 0x00002030);
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
	cldma_write32(md_pll->md_clkSW, R_FLEXCKGEN_SEL0, 0x30203031);

	cldma_write32(md_pll->md_dcm, MD_GLOBAL_CON_DUMMY, MD_PLL_MAGIC_NUM);

	#if 0
	/*PSMCU DCM */
	ROr2W(md_pll->md_dcm, R_PSMCU_DCM_CTL0, 0x00F1F006);

	ROr2W(md_pll->md_dcm, R_PSMCU_DCM_CTL1, 0x26);

	/*ARM7 DCM */
	ROr2W(md_pll->md_dcm, R_ARM7_DCM_CTL0, 0x00F1F006);
	ROr2W(md_pll->md_dcm, R_ARM7_DCM_CTL1, 0x26);

	cldma_write32(md_pll->md_sys_clk, R_DCM_SHR_SET_CTL, 0x00014110);

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

int md_cd_power_on(struct ccci_modem *md)
{
	int ret = 0;
	unsigned int reg_value;
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	/* step 0: PMIC setting */
	md1_pmic_setting_on();

	/* steip 1: power on MD_INFRA and MODEM_TOP */
	switch (md->index) {
	case MD_SYS1:
		clk_buf_set_by_flightmode(false);
#if defined(CONFIG_MTK_CLKMGR)
		CCCI_BOOTUP_LOG(md->index, TAG, "Call start md_power_on()\n");
		ret = md_power_on(SYS_MD1);
		CCCI_BOOTUP_LOG(md->index, TAG, "Call end md_power_on() ret=%d\n", ret);
#else
		CCCI_BOOTUP_LOG(md->index, TAG, "Call start clk_prepare_enable()\n");
		ret = clk_prepare_enable(clk_scp_sys_md1_main);
		CCCI_BOOTUP_LOG(md->index, TAG, "Call end clk_prepare_enable() ret=%d\n", ret);
#endif

		kicker_pbm_by_md(KR_MD1, true);
		CCCI_BOOTUP_LOG(md->index, TAG, "Call end kicker_pbm_by_md(0,true)\n");
		break;
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
	cldma_write32(md_ctrl->md_rgu_base, WDT_MD_MODE, WDT_MD_MODE_KEY);
	cldma_write32(md_ctrl->l1_rgu_base, REG_L1RSTCTL_WDT_MODE, L1_WDT_MD_MODE_KEY);

#ifdef SET_EMI_STEP_BY_STAGE
	CCCI_BOOTUP_LOG(md->index, KERN, "set domain register\n");
	ccci_write32(md_ctrl->md_pll_base->md_busreg1, 0xC4, 0x7);
	ccci_write32(md_ctrl->md_pll_base->md_L1_a0, 0x220, 0x1);
	ccci_write32(md_ctrl->md_pll_base->md_L1_a0, 0x220, 0x81);
#endif
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

	cldma_write32(md_ctrl->md_boot_slave_En, 0, 1);	/* make boot vector take effect */
	return 0;
}

int md_cd_soft_power_off(struct ccci_modem *md, unsigned int mode)
{
	clk_buf_set_by_flightmode(true);
	return 0;
}

int md_cd_soft_power_on(struct ccci_modem *md, unsigned int mode)
{
	clk_buf_set_by_flightmode(false);
	return 0;
}

int md_cd_power_off(struct ccci_modem *md, unsigned int stop_type)
{
	int ret = 0;
#if defined(CONFIG_MTK_CLKMGR)
	unsigned int timeout = 0;

	if (stop_type == MD_FLIGHT_MODE_ENTER)
		timeout = 1000;
#endif
#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
	/* notify NFC */
	inform_nfc_vsim_change(md->index, 0, 0);
#endif

	/* power off MD_INFRA and MODEM_TOP */
	switch (md->index) {
	case MD_SYS1:
#if defined(CONFIG_MTK_CLKMGR)
		ret = md_power_off(SYS_MD1, timeout);
#else
		clk_disable(clk_scp_sys_md1_main);
		clk_unprepare(clk_scp_sys_md1_main);	/* cannot be called in mutex context */
#endif
		clk_buf_set_by_flightmode(true);
		md1_pmic_setting_off();
		kicker_pbm_by_md(KR_MD1, false);
		CCCI_BOOTUP_LOG(md->index, TAG, "Call end kicker_pbm_by_md(0,false)\n");
		break;
	}
	return ret;
}

void cldma_dump_register(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Tx pdn register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_UL_START_ADDR_0,
		      CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE - CLDMA_AP_UL_START_ADDR_0 + 4);
	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Tx ao register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_ao_base + CLDMA_AP_UL_START_ADDR_BK_0,
		      CLDMA_AP_UL_CURRENT_ADDR_BK_7 - CLDMA_AP_UL_START_ADDR_BK_0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Rx pdn register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_SO_ERROR,
		      CLDMA_AP_SO_STOP_CMD - CLDMA_AP_SO_ERROR + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA Rx ao register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_ao_base + CLDMA_AP_SO_CFG,
		      CLDMA_AP_DEBUG_ID_EN - CLDMA_AP_SO_CFG + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA MISC pdn register\n");
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_L2TISAR0,
		      CLDMA_AP_CLDMA_IP_BUSY - CLDMA_AP_L2TISAR0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump AP CLDMA MISC ao register\n");
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_ap_ao_base + CLDMA_AP_L2RIMR0,
						CLDMA_AP_DUMMY - CLDMA_AP_L2RIMR0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Tx pdn register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_pdn_base + CLDMA_AP_UL_START_ADDR_0,
		      CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE - CLDMA_AP_UL_START_ADDR_0 + 4);
	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Tx ao register, active=%x\n", md_ctrl->txq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_ao_base + CLDMA_AP_UL_START_ADDR_BK_0,
		      CLDMA_AP_UL_CURRENT_ADDR_BK_7 - CLDMA_AP_UL_START_ADDR_BK_0 + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Rx pdn register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_pdn_base + CLDMA_AP_SO_ERROR,
		      CLDMA_AP_SO_STOP_CMD - CLDMA_AP_SO_ERROR + 4);
	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA Rx ao register, active=%x\n", md_ctrl->rxq_active);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_ao_base + CLDMA_AP_SO_CFG,
		      CLDMA_AP_DEBUG_ID_EN - CLDMA_AP_SO_CFG + 4);

	CCCI_MEM_LOG_TAG(md->index, TAG, "dump MD CLDMA MISC pdn register\n");
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->cldma_md_pdn_base + CLDMA_AP_L2TISAR0,
		      CLDMA_AP_CLDMA_IP_BUSY - CLDMA_AP_L2TISAR0 + 4);
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

	BUG_ON(pdev == NULL);

	return ccci_modem_suspend(pdev, PMSG_SUSPEND);
}

int ccci_modem_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);

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

	if (md->md_state == GATED || md->md_state == RESET || md->md_state == INVALID) {
		CCCI_NORMAL_LOG(md->index, TAG, "Resume no need reset cldma for md_state=%d\n", md->md_state);
		return;
	}
	cldma_write32(md_ctrl->ap_ccif_base, APCCIF_CON, 0x01);	/* arbitration */

	if (cldma_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_TQSAR(0))) {
		CCCI_NORMAL_LOG(md->index, TAG, "Resume cldma pdn register: No need  ...\n");
	} else {
		CCCI_NORMAL_LOG(md->index, TAG, "Resume cldma pdn register ...11\n");
		spin_lock_irqsave(&md_ctrl->cldma_timeout_lock, flags);
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_HPQR, 0x00);
		/* set checksum */
		switch (CHECKSUM_SIZE) {
		case 0:
			cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE, 0);
			break;
		case 12:
			cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE,
				      CLDMA_BM_ALL_QUEUE);
			cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG,
				      cldma_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG) & ~0x10);
			break;
		case 16:
			cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE,
				      CLDMA_BM_ALL_QUEUE);
			cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG,
				      cldma_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_CFG) | 0x10);
			break;
		}
		/* set start address */
		for (i = 0; i < QUEUE_LEN(md_ctrl->txq); i++) {
			if (cldma_read32(md_ctrl->cldma_ap_ao_base, CLDMA_AP_TQCPBAK(md_ctrl->txq[i].index)) == 0) {
				if (i != 7) /* Queue 7 not used currently */
					CCCI_NORMAL_LOG(md->index, TAG, "Resume CH(%d) current bak:== 0\n", i);
				cldma_reg_set_tx_start_addr(md_ctrl->cldma_ap_pdn_base,
					md_ctrl->txq[i].index,
					md_ctrl->txq[i].tr_done->gpd_addr);
				cldma_reg_set_tx_start_addr_bk(md_ctrl->cldma_ap_ao_base,
					md_ctrl->txq[i].index,
					md_ctrl->txq[i].tr_done->gpd_addr);
			} else {
				unsigned int bk_addr = cldma_read32(md_ctrl->cldma_ap_ao_base,
								CLDMA_AP_TQCPBAK(md_ctrl->txq[i].index));

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
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_START_CMD, CLDMA_BM_ALL_QUEUE);
		cldma_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_START_CMD);	/* dummy read */
#endif
		md_ctrl->txq_active |= CLDMA_BM_ALL_QUEUE;
		/* cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_SO_START_CMD, CLDMA_BM_ALL_QUEUE); */
		/* cldma_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_SO_START_CMD); // dummy read */
		/* md_ctrl->rxq_active |= CLDMA_BM_ALL_QUEUE; */
		/* enable L2 DONE and ERROR interrupts */
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L2TIMCR0, CLDMA_BM_INT_DONE | CLDMA_BM_INT_ERROR);
		/* enable all L3 interrupts */
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3TIMCR0, CLDMA_BM_INT_ALL);
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3TIMCR1, CLDMA_BM_INT_ALL);
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3RIMCR0, CLDMA_BM_INT_ALL);
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_L3RIMCR1, CLDMA_BM_INT_ALL);
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
