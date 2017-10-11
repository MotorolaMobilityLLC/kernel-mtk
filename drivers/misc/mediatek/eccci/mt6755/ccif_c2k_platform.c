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
#include <mt_spm_sleep.h>
#include <mach/mt_pbm.h>

#include "ccci_config.h"
#include "ccci_modem.h"
#include "ccci_platform.h"
#include "ccif_c2k_platform.h"
#include "modem_ccif.h"
#include "modem_reg_base.h"

#include <mt-plat/upmu_common.h>
#include <mt-plat/mt_boot.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#define TAG "cif"

#if !defined(CONFIG_MTK_CLKMGR)
#include <linux/clk.h>
static struct clk *clk_scp_sys_md2_main;
static struct clk *clk_scp_sys_md3_main;
#else
#include <mach/mt_clkmgr.h>
#endif

#define PCCIF_BUSY (0x4)
#define PCCIF_TCHNUM (0xC)
#define PCCIF_ACK (0x14)
#define PCCIF_CHDATA (0x100)
#define PCCIF_SRAM_SIZE (512)

static unsigned long apmixed_base;
static unsigned long apinfra_base;

struct c2k_pll_t c2k_pll_reg;
void __iomem *ccirq_base[4];
void __iomem *c2k_cgbr1_addr;
void __iomem *c2k_debug_reg_vir;

void __iomem *c2k_mpu_itrace_vir;
void __iomem *c2k_wd_max_time_vir;
void __iomem *c2k_iram_base_vir;
void __iomem *c2k_h2x_zone_vir;
void __iomem *c2k_clk_base_vir;
void __iomem *c2k_pll_base_vir;
void __iomem *c2k_cgbr_sbc_vir;
void __iomem *c2k_boot_rom_vir;

int md_ccif_get_modem_hw_info(struct platform_device *dev_ptr,
			      struct ccci_dev_cfg *dev_cfg,
			      struct md_hw_info *hw_info)
{
	struct device_node *node = NULL;

	memset(dev_cfg, 0, sizeof(struct ccci_dev_cfg));
	memset(hw_info, 0, sizeof(struct md_hw_info));

#ifdef CONFIG_OF
	if (dev_ptr->dev.of_node == NULL) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem OF node NULL\n");
		return -1;
	}

	of_property_read_u32(dev_ptr->dev.of_node, "cell-index",
			     &dev_cfg->index);
	CCCI_NORMAL_LOG(dev_cfg->index, TAG, "modem hw info get idx:%d\n",
		     dev_cfg->index);
	if (!get_modem_is_enabled(dev_cfg->index)) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d not enable, exit\n",
			     dev_cfg->index + 1);
		return -1;
	}
#else
	struct ccci_dev_cfg *dev_cfg_ptr =
	    (struct ccci_dev_cfg *)dev->dev.platform_data;
	dev_cfg->index = dev_cfg_ptr->index;

	CCCI_NORMAL_LOG(dev_cfg->index, TAG, "modem hw info get idx:%d\n",
		     dev_cfg->index);
	if (!get_modem_is_enabled(dev_cfg->index)) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d not enable, exit\n",
			     dev_cfg->index + 1);
		return -1;
	}
#endif

	switch (dev_cfg->index) {
	case 1:		/*MD_SYS2 */
#ifdef CONFIG_OF
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,major",
				     &dev_cfg->major);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,minor_base",
				     &dev_cfg->minor_base);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,capability",
				     &dev_cfg->capability);

		hw_info->ap_ccif_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 0);
		/*hw_info->md_ccif_base = hw_info->ap_ccif_base+0x1000; */
		node = of_find_compatible_node(NULL, NULL, "mediatek,MD_CCIF1");
		hw_info->md_ccif_base = (unsigned long)of_iomap(node, 0);

		hw_info->ap_ccif_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 0);
		hw_info->md_wdt_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 1);

		/*Device tree using none flag to register irq, sensitivity has set at "irq_of_parse_and_map" */
		hw_info->ap_ccif_irq_flags = IRQF_TRIGGER_NONE;
		hw_info->md_wdt_irq_flags = IRQF_TRIGGER_NONE;
#endif

		hw_info->sram_size = CCIF_SRAM_SIZE;
		hw_info->md_rgu_base = MD2_RGU_BASE;
		hw_info->md_boot_slave_Vector = MD2_BOOT_VECTOR;
		hw_info->md_boot_slave_Key = MD2_BOOT_VECTOR_KEY;
		hw_info->md_boot_slave_En = MD2_BOOT_VECTOR_EN;

#if !defined(CONFIG_MTK_CLKMGR)
		clk_scp_sys_md2_main =
		    devm_clk_get(&dev_ptr->dev, "scp-sys-md2-main");
		if (IS_ERR(clk_scp_sys_md2_main)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG,
				     "modem %d get scp-sys-md2-main failed\n",
				     dev_cfg->index + 1);
			return -1;
		}
#endif
		break;
	case 2:		/*MD_SYS3 */
#ifdef CONFIG_OF
		dev_cfg->major = 0;
		dev_cfg->minor_base = 0;
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,capability",
				     &dev_cfg->capability);

		hw_info->ap_ccif_base = (unsigned long)of_iomap(dev_ptr->dev.of_node, 0);
		/*hw_info->md_ccif_base = hw_info->ap_ccif_base+0x1000; */
		node = of_find_compatible_node(NULL, NULL, "mediatek,md_ccif1");
		hw_info->md_ccif_base = (unsigned long)of_iomap(node, 0);

		hw_info->ap_ccif_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 0);
		hw_info->md_wdt_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 1);

		/*Device tree using none flag to register irq, sensitivity has set at "irq_of_parse_and_map" */
		hw_info->ap_ccif_irq_flags = IRQF_TRIGGER_NONE;
		hw_info->md_wdt_irq_flags = IRQF_TRIGGER_NONE;

		hw_info->md1_pccif_base =
		    (unsigned long)of_iomap(dev_ptr->dev.of_node, 1);
		hw_info->md3_pccif_base =
		    (unsigned long)of_iomap(dev_ptr->dev.of_node, 2);

		node =
		    of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
		hw_info->infra_ao_base = (unsigned long)of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
		hw_info->sleep_base = (unsigned long)of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL, "mediatek,toprgu");
		hw_info->toprgu_base = (unsigned long)of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
		apmixed_base = (unsigned long)of_iomap(node, 0);
		node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg");
		apinfra_base = (unsigned long)of_iomap(node, 0);

		CCCI_NORMAL_LOG(dev_cfg->index, TAG,
			     "infra_ao_base=0x%lx, sleep_base=0x%lx, toprgu_base=0x%lx\n",
			     hw_info->infra_ao_base, hw_info->sleep_base,
			     hw_info->toprgu_base);

#endif

		hw_info->sram_size = CCIF_SRAM_SIZE;
		hw_info->md_rgu_base = MD3_RGU_BASE;

#if !defined(CONFIG_MTK_CLKMGR)
		clk_scp_sys_md3_main =
		    devm_clk_get(&dev_ptr->dev, "scp-sys-md2-main");
		if (IS_ERR(clk_scp_sys_md3_main)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG,
				     "modem %d get scp-sys-md2-main failed\n",
				     dev_cfg->index + 1);
			return -1;
		}
#endif

		/*no boot slave for md3 */
		/*
		   hw_info->md_boot_slave_Vector = MD3_BOOT_VECTOR;
		   hw_info->md_boot_slave_Key = MD3_BOOT_VECTOR_KEY;
		   hw_info->md_boot_slave_En = MD3_BOOT_VECTOR_EN;
		 */
		break;
	default:
		return -1;
	}

	CCCI_NORMAL_LOG(dev_cfg->index, TAG,
		     "modem ccif of node get dev_major:%d\n", dev_cfg->major);
	CCCI_NORMAL_LOG(dev_cfg->index, TAG,
		     "modem ccif of node get minor_base:%d\n",
		     dev_cfg->minor_base);
	CCCI_NORMAL_LOG(dev_cfg->index, TAG,
		     "modem ccif of node get capability:%d\n",
		     dev_cfg->capability);

	CCCI_NORMAL_LOG(dev_cfg->index, TAG, "ap_ccif_base:0x%p\n",
		     (void *)hw_info->ap_ccif_base);
	CCCI_NORMAL_LOG(dev_cfg->index, TAG, "ccif_irq_id:%d\n",
		     hw_info->ap_ccif_irq_id);
	CCCI_NORMAL_LOG(dev_cfg->index, TAG, "md_wdt_irq_id:%d\n",
		     hw_info->md_wdt_irq_id);

	return 0;
}

int md_ccif_io_remap_md_side_register(struct ccci_modem *md)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	switch (md->index) {
	case MD_SYS2:
		md_ctrl->md_boot_slave_Vector =
		    ioremap_nocache(md_ctrl->hw_info->md_boot_slave_Vector,
				    0x4);
		md_ctrl->md_boot_slave_Key =
		    ioremap_nocache(md_ctrl->hw_info->md_boot_slave_Key, 0x4);
		md_ctrl->md_boot_slave_En =
		    ioremap_nocache(md_ctrl->hw_info->md_boot_slave_En, 0x4);
		md_ctrl->md_rgu_base =
		    ioremap_nocache(md_ctrl->hw_info->md_rgu_base, 0x40);
		break;
	case MD_SYS3:
		c2k_pll_reg.c2k_pll_con3 = ioremap_nocache(C2KSYS_BASE + C2K_C2K_PLL_CON3, 0x4);
		c2k_pll_reg.c2k_pll_con2 = ioremap_nocache(C2KSYS_BASE + C2K_C2K_PLL_CON2, 0x4);
		c2k_pll_reg.c2k_plltd_con0 = ioremap_nocache(C2KSYS_BASE + C2K_C2K_PLLTD_CON0, 0x4);
		c2k_pll_reg.c2k_cppll_con0 = ioremap_nocache(C2KSYS_BASE + C2K_C2K_CPPLL_CON0, 0x4);
		c2k_pll_reg.c2k_dsppll_con0 = ioremap_nocache(C2KSYS_BASE + C2K_C2K_DSPPLL_CON0, 0x4);
		c2k_pll_reg.c2k_c2kpll1_con0 = ioremap_nocache(C2KSYS_BASE + C2K_C2K_C2KPLL1_CON0, 0x4);
		c2k_pll_reg.c2k_cg_amba_clksel = ioremap_nocache(C2KSYS_BASE + C2K_CG_ARM_AMBA_CLKSEL, 0x4);
		c2k_pll_reg.c2k_clk_ctrl4 = ioremap_nocache(C2KSYS_BASE + C2K_CLK_CTRL4, 0x4);
		c2k_pll_reg.c2k_clk_ctrl9 = ioremap_nocache(C2KSYS_BASE + C2K_CLK_CTRL9, 0x4);
		/*CCIRQ reg*/
		ccirq_base[0] = ioremap_nocache(L1_C2K_CCIRQ_BASE, 0x100);
		ccirq_base[1] = ioremap_nocache(C2K_L1_CCIRQ_BASE, 0x100);
		ccirq_base[2] = ioremap_nocache(PS_C2K_CCIRQ_BASE, 0x100);
		ccirq_base[3] = ioremap_nocache(C2K_PS_CCIRQ_BASE, 0x100);

		c2k_cgbr1_addr = ioremap_nocache(C2KSYS_BASE + C2K_CGBR1, 0x4);
		c2k_debug_reg_vir = ioremap_nocache(C2KSYS_BASE + C2K_DEBUG_REG, 0x10);

		c2k_mpu_itrace_vir = ioremap_nocache(C2KSYS_BASE + C2K_MPU_ITRACE, 0x100);
		c2k_wd_max_time_vir = ioremap_nocache(C2KSYS_BASE + C2K_WD_MAX_TIME, 0x4);
		c2k_iram_base_vir = ioremap_nocache(C2KSYS_BASE + C2K_IRAM_BASE, C2K_IRAM_DUMP_SIZE);
		c2k_h2x_zone_vir = ioremap_nocache(C2KSYS_BASE + C2K_H2X_ZONE_BASE, 0x1000);
		c2k_clk_base_vir = ioremap_nocache(C2KSYS_BASE + C2K_CLK_BASE, 0x1000);
		c2k_pll_base_vir = ioremap_nocache(C2KSYS_BASE + C2K_PLL_BASE, 0x1000);
		c2k_cgbr_sbc_vir = ioremap_nocache(C2KSYS_BASE + C2K_CGBR_SBC_BASE, 0x1000);
		c2k_boot_rom_vir = ioremap_nocache(C2K_BOOT_ROM_BASE, 0x1000);

		break;
	}
	return 0;
}

static int config_c2k_pll(void)
{
	ccif_write16(c2k_pll_reg.c2k_pll_con3, 0, 0x8805);
	ccif_write16(c2k_pll_reg.c2k_pll_con3, 0, 0x0005);
	ccif_write16(c2k_pll_reg.c2k_pll_con3, 0, 0x0001);
	ccif_write16(c2k_pll_reg.c2k_pll_con2, 0, 0x0);
	ccif_write16(c2k_pll_reg.c2k_plltd_con0, 0, 0x0010);

	ccif_write16(c2k_pll_reg.c2k_cppll_con0, 0,
			 ccif_read16(c2k_pll_reg.c2k_cppll_con0, 0) | (0x1 << 15));
	/* ccif_write16(c2k_pll_reg.c2k_dsppll_con0, 0,
			 ccif_read16(c2k_pll_reg.c2k_dsppll_con0, 0) | (0x1 << 15)); */
	ccif_write16(c2k_pll_reg.c2k_cg_amba_clksel, 0,
			 ccif_read16(c2k_pll_reg.c2k_cg_amba_clksel, 0) | (0x1 << 15));

	udelay(30);

	ccif_write16(c2k_pll_reg.c2k_cg_amba_clksel, 0, 0xC124);
	ccif_write16(c2k_pll_reg.c2k_clk_ctrl4, 0, 0x8E43);
	ccif_write16(c2k_pll_reg.c2k_clk_ctrl9, 0, 0xA207);


	return 0;
}

static int reset_ccirq_hardware(void)
{
	int i = 0;

	CCCI_NORMAL_LOG(MD_SYS3, TAG, "reset_ccirq_hardware start\n");
	for (i = 0; i < 2; i++) {
		ccif_write32(ccirq_base[i], 0x4, 0xA00000FF);
		ccif_write32(ccirq_base[i], 0xC, 0xA00000FF);
	}
	for (i = 2; i < 4; i++) {
		ccif_write32(ccirq_base[i], 0x4, 0xA000000F);
		ccif_write32(ccirq_base[i], 0xC, 0xA000000F);
	}

	for (i = 0; i < 4; i++) {
		ccif_write32(ccirq_base[i], 0x40, 0x0);
		ccif_write32(ccirq_base[i], 0x44, 0x0);
		ccif_write32(ccirq_base[i], 0x48, 0x0);
		ccif_write32(ccirq_base[i], 0x4C, 0x0);
	}
	CCCI_NORMAL_LOG(MD_SYS3, TAG, "reset_ccirq_hardware end\n");
	return 0;
}

/*need modify according to dummy ap*/
int md_ccif_let_md_go(struct ccci_modem *md)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	if (MD_IN_DEBUG(md)) {
		CCCI_BOOTUP_LOG(md->index, TAG, "DBG_FLAG_JTAG is set\n");
		return -1;
	}
	CCCI_BOOTUP_LOG(md->index, TAG, "md_ccif_let_md_go\n");
	switch (md->index) {
	case MD_SYS2:
		/*set the start address to let modem to run */
			/*make boot vector programmable */
		ccif_write32(md_ctrl->md_boot_slave_Key, 0, MD2_BOOT_VECTOR_KEY_VALUE);
			/*after remap, MD ROM address is 0 from MD's view */
		ccif_write32(md_ctrl->md_boot_slave_Vector, 0, MD2_BOOT_VECTOR_VALUE);
			/*make boot vector take effect */
		ccif_write32(md_ctrl->md_boot_slave_En, 0, MD2_BOOT_VECTOR_EN_VALUE);
		break;
	case MD_SYS3:
		/*check if meta mode */
		if (is_meta_mode() || get_boot_mode() == FACTORY_BOOT) {
			ccif_write32(md_ctrl->hw_info->infra_ao_base,
				     INFRA_AO_C2K_CONFIG,
				     (ccif_read32
				      (md_ctrl->hw_info->infra_ao_base,
				       INFRA_AO_C2K_CONFIG) | ETS_SEL_BIT));
		}
		/*step 1: set C2K boot mode */
		/*
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_CONFIG,
			     (ccif_read32
			      (md_ctrl->hw_info->infra_ao_base,
			       INFRA_AO_C2K_CONFIG) & (~(0x7 << 8))) | (0x5 << 8));
		*/
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_CONFIG,
			     (ccif_read32
			      (md_ctrl->hw_info->infra_ao_base,
			       INFRA_AO_C2K_CONFIG)) | (0x1 << 3));

		ccif_write32(md_ctrl->hw_info->sleep_base, AP_POWERON_CONFIG_EN,
			0x0B160001);

		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) | 0xc);

		while (((ccif_read32
			 (md_ctrl->hw_info->sleep_base, AP_PWR_STATUS) & (0x1 << 28)) != (0x1 << 28)) ||
			 ((ccif_read32
			 (md_ctrl->hw_info->sleep_base, AP_PWR_STATUS_2ND) & (0x1 << 28)) != (0x1 << 28)))
			;

		CCCI_BOOTUP_LOG(md->index, TAG, "[C2K] AP_PWR_STATUS = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->sleep_base, AP_PWR_STATUS));
		CCCI_BOOTUP_LOG(md->index, TAG, "[C2K] AP_PWR_STATUS_2ND = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->sleep_base, AP_PWR_STATUS_2ND));

		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) & (~(0x1 << PWR_CLK_DIS)));
		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) & (~(0x1 << PWR_ISO)));
		ccif_write32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON,
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON) | (0x1 << PWR_RST_B));
		CCCI_BOOTUP_LOG(md->index, TAG, "SLEEP_CLK_CON = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->sleep_base,
					 SLEEP_CLK_CON));

		/*step 2: config srcclkena selection mask */
		ccif_write32(md_ctrl->hw_info->infra_ao_base, INFRA_MISC2,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_MISC2) | INFRA_MISC2_C2K_EN);
		CCCI_BOOTUP_LOG(md->index, TAG, "INFRA_MISC2 = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_MISC2));


		/*step 3: PMIC VTCXO_1 enable */
		/*pmic_config_interface(0x0A02, 0xA12E, 0xFFFF, 0x0);*/

		/* ccif_write32(apmixed_base, AP_PLL_CON0,
			     ccif_read32(apmixed_base,
					 AP_PLL_CON0) | (0x1 << 1)); */
		CCCI_BOOTUP_LOG(md->index, TAG, "AP_PLL_CON0 = 0x%x\n",
			     ccif_read32(apmixed_base, AP_PLL_CON0));

		/*ap hold c2k core*/
		/*
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_CONFIG,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_CONFIG) | (0x1 << 1));
		*/
		CCCI_BOOTUP_LOG(md->index, TAG, "C2K_CONFIG = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_CONFIG));

		/*step 4: wake up C2K */
		/*
		ccif_write32(apmixed_base, MDPLL1_CON0,
				 ccif_read32(apmixed_base,
					 MDPLL1_CON0) & (~(0x1 << 9)));
		CCCI_NORMAL_LOG(md->index, TAG, "MDPLL1_CON0 = 0x%x\n",
				 ccif_read32(apmixed_base, MDPLL1_CON0));
		*/

#if 1
		ccif_write32(md_ctrl->hw_info->toprgu_base,
			     TOP_RGU_WDT_SWSYSRST,
			     (ccif_read32
			      (md_ctrl->hw_info->toprgu_base,
			       TOP_RGU_WDT_SWSYSRST) | 0x88000000) & (~(0x1 <<
									15)));
#else
		mtk_wdt_set_c2k_sysrst(1);
#endif
		CCCI_BOOTUP_LOG(md->index, TAG,
			     "[C2K] TOP_RGU_WDT_SWSYSRST = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->toprgu_base,
					 TOP_RGU_WDT_SWSYSRST));

		/*step 5: mpu already set */
		/*step 6: wake up C2K */
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_SPM_CTRL,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL) | (0x1 << 1));

		ccif_write32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_CONFIG,
					(ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_CONFIG) &
					(~(0x3 << 11))));
		while (!
		       ((ccif_read32
			 (md_ctrl->hw_info->infra_ao_base,
			  INFRA_AO_C2K_STATUS) >> 1) & 0x1))
			;

		CCCI_BOOTUP_LOG(md->index, TAG,
			     "[C2K] C2K_STATUS = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->
					 infra_ao_base,
					 INFRA_AO_C2K_STATUS));

		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_SPM_CTRL,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL) & (~(0x1 << 1)));
		CCCI_BOOTUP_LOG(md->index, TAG,
			     "[C2K] C2K_SPM_CTRL = 0x%x, C2K_STATUS = 0x%x\n",
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_SPM_CTRL),
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_STATUS));

		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_TOPAXI_PROTECTEN_1,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_TOPAXI_PROTECTEN_1) & (~(0x3 << 22)));

		while (ccif_read32(c2k_cgbr1_addr, 0) != 0xFE8)
			;
		CCCI_BOOTUP_LOG(md->index, TAG,
			     "[C2K] C2K_CGBR1 = 0x%x\n", ccif_read32(c2k_cgbr1_addr, 0));

		/*configure c2k debug reg*/
		ccif_write32(c2k_debug_reg_vir, 0, 0xB);
		ccif_write32(c2k_debug_reg_vir, 4, 0x5B);

		/*set c2k pll*/
		config_c2k_pll();

		/*release c2k arm core*/
		/*
		ccif_write32(md_ctrl->hw_info->infra_ao_base,
			     INFRA_AO_C2K_CONFIG,
			     ccif_read32(md_ctrl->hw_info->infra_ao_base,
					 INFRA_AO_C2K_CONFIG) & (~(0x1 << 1)));
		*/
		break;
	}
	return 0;
}

int md_ccif_power_on(struct ccci_modem *md)
{
	int ret = 0;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	switch (md->index) {
	case MD_SYS2:
#if defined(CONFIG_MTK_CLKMGR)
		CCCI_NORMAL_LOG(md->index, TAG, "Call start md_power_on()\n");
		ret = md_power_on(SYS_MD2);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end md_power_on() ret=%d\n",
			     ret);
#else
		CCCI_NORMAL_LOG(md->index, TAG,
			     "Call start clk_prepare_enable()\n");
		clk_prepare_enable(clk_scp_sys_md2_main);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end clk_prepare_enable()\n");
#endif
		break;
	case MD_SYS3:
#if defined(CONFIG_MTK_CLKMGR)
		CCCI_NORMAL_LOG(md->index, TAG, "Call start md_power_on()\n");
		ret = md_power_on(SYS_MD2);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end md_power_on() ret=%d\n",
			     ret);
#else
		CCCI_NORMAL_LOG(md->index, TAG,
			     "Call start clk_prepare_enable()\n");
		clk_prepare_enable(clk_scp_sys_md3_main);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end clk_prepare_enable()\n");
#endif
		kicker_pbm_by_md(KR_MD3, true);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end kicker_pbm_by_md(3,true)\n");
		break;
	}
	CCCI_BOOTUP_LOG(md->index, TAG, "md_ccif_power_on:ret=%d\n", ret);
	if (ret == 0 && md->index != MD_SYS3) {
		/*disable MD WDT */
		ccif_write32(md_ctrl->md_rgu_base, WDT_MD_MODE,
			     WDT_MD_MODE_KEY);
	}
	return ret;
}

int md_ccif_power_off(struct ccci_modem *md, unsigned int stop_type)
{
	int ret = 0;
#if defined(CONFIG_MTK_CLKMGR)
	unsigned int timeout = 0;

	if (stop_type == MD_FLIGHT_MODE_ENTER)
		timeout = 1000;
#endif

	switch (md->index) {
	case MD_SYS2:
#if defined(CONFIG_MTK_CLKMGR)
		ret = md_power_off(SYS_MD2, timeout);
#else
		clk_disable_unprepare(clk_scp_sys_md2_main);
#endif
		break;
	case MD_SYS3:
#if defined(CONFIG_MTK_CLKMGR)
		ret = md_power_off(SYS_MD3, timeout);
#else
		clk_disable_unprepare(clk_scp_sys_md3_main);
#endif
		kicker_pbm_by_md(KR_MD3, false);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end kicker_pbm_by_md(3,false)\n");
		break;
	}
	CCCI_NORMAL_LOG(md->index, TAG, "md_ccif_power_off:ret=%d\n", ret);
	return ret;
}

void reset_md1_md3_pccif(struct ccci_modem *md)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	struct md_hw_info *hw_info = md_ctrl->hw_info;

	reset_ccirq_hardware();

	ccci_reset_ccif_hw(md, MD1_MD3_CCIF, (void __iomem *)hw_info->md1_pccif_base,
					(void __iomem *)hw_info->md3_pccif_base);

	/*clear md1 md3 shared memory*/
	if (md->mem_layout.md1_md3_smem_vir != NULL)
		memset_io(md->mem_layout.md1_md3_smem_vir, 0, md->mem_layout.md1_md3_smem_size);

}

void dump_c2k_register(struct ccci_modem *md, unsigned int dump_flag)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	u32 i, j;
	unsigned long reg_base;

	/*start addr, reg count, end must be {0, 0}*/
	u32 h2x_reg[][2] = { {0x3C, 8}, {0, 0} };
	u32 cgbr_sbc_reg[][2] = { {0x0, 7}, {0xF0, 3}, {0x10c, 3}, {0x200, 11}, {0, 0} };
	u32 clk_ctrl_reg[][2] = { {0x0, 5}, {0x64, 1}, {0x234, 1}, {0x25c, 1}, {0x268, 4},
				{0x29c, 1}, {0x300, 3}, {0, 0} };
	u32 pll_reg[][2] = { {0x0, 24}, {0, 0} };

	CCCI_MEM_LOG_TAG(md->index, TAG, "INFRA_C2K_BOOT_STATUS = 0x%x\n",
			 ccif_read32(apinfra_base, INFRA_C2K_BOOT_STATUS));
	CCCI_MEM_LOG_TAG(md->index, TAG, "INFRA_C2K_BOOT_STATUS2 = 0x%x\n",
			 ccif_read32(apinfra_base, INFRA_C2K_BOOT_STATUS2));

	CCCI_MEM_LOG_TAG(md->index, TAG, "C2K_CONFIG = 0x%x\n",
			 ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_CONFIG));
	CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] AP_PWR_STATUS = 0x%x\n",
			 ccif_read32(md_ctrl->hw_info->sleep_base, AP_PWR_STATUS));
	CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] AP_PWR_STATUS_2ND = 0x%x\n",
			 ccif_read32(md_ctrl->hw_info->sleep_base, AP_PWR_STATUS_2ND));

	CCCI_MEM_LOG_TAG(md->index, TAG, "SLEEP_CLK_CON = 0x%x\n",
			ccif_read32(md_ctrl->hw_info->sleep_base, SLEEP_CLK_CON));
	CCCI_MEM_LOG_TAG(md->index, TAG, "INFRA_MISC2 = 0x%x\n",
			ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_MISC2));

	CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] C2K_SPM_CTRL = 0x%x, C2K_STATUS = 0x%x\n",
			 ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_SPM_CTRL),
			 ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_STATUS));

	if (dump_flag == 0)
		return;

	/* wdt EE, flag = 1*/
	if (dump_flag == 0x1) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "C2K_WD_TIME_MAX = 0x%x\n",
				 ccif_read32(c2k_wd_max_time_vir, 0));

		return;
	}

	/*default value for HS1 EE*/
	for (i = 0; i < 4; i++) {
		ccif_write32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_CONFIG,
					(ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_CONFIG) &
					(~(0x3 << 11))) | (i << 11));
		for (j = 0; j < 16; j++) {
			CCCI_MEM_LOG_TAG(md->index, TAG, "C2K_CONFIG = 0x%x\n",
					 ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_CONFIG));
			CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] C2K_STATUS = 0x%x\n",
					     ccif_read32(md_ctrl->hw_info->infra_ao_base, INFRA_AO_C2K_STATUS));
		}
	}

	reg_base = C2KSYS_BASE + C2K_MPU_ITRACE;
	for (j = 0; j < C2K_MPU_ITRACE_DUMP_SIZE; ) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] mpu itrace 0x%lx, value = 0x%x\n",
				reg_base + j, ccif_read32(c2k_mpu_itrace_vir, j));
		j += 4;
	}

	reg_base = C2KSYS_BASE + C2K_H2X_ZONE_BASE;
	for (i = 0; h2x_reg[i][1] != 0; i++) {
		for (j = 0; j < h2x_reg[i][1]; j++)
			CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] reg = 0x%lx, value = 0x%x\n",
				reg_base + h2x_reg[i][0] + j*4, ccif_read32(c2k_h2x_zone_vir, h2x_reg[i][0] + j*4));
	}

	reg_base = C2KSYS_BASE + C2K_CGBR_SBC_BASE;
	for (i = 0; cgbr_sbc_reg[i][1] != 0; i++) {
		for (j = 0; j < cgbr_sbc_reg[i][1]; j++)
			CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] reg = 0x%lx, value = 0x%x\n",
				reg_base + cgbr_sbc_reg[i][0] + j*4,
				ccif_read32(c2k_cgbr_sbc_vir, cgbr_sbc_reg[i][0] + j*4));
	}

	reg_base = C2KSYS_BASE + C2K_CLK_BASE;
	for (i = 0; clk_ctrl_reg[i][1] != 0; i++) {
		for (j = 0; j < clk_ctrl_reg[i][1]; j++)
			CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] reg = 0x%lx, value = 0x%x\n",
				reg_base + clk_ctrl_reg[i][0] + j*4,
				ccif_read32(c2k_clk_base_vir, clk_ctrl_reg[i][0] + j*4));
	}

	reg_base = C2KSYS_BASE + C2K_PLL_BASE;
	for (i = 0; pll_reg[i][1] != 0; i++) {
		for (j = 0; j < pll_reg[i][1]; j++)
			CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] reg = 0x%lx, value = 0x%x\n",
				reg_base + pll_reg[i][0] + j*4,
				ccif_read32(c2k_pll_base_vir, pll_reg[i][0] + j*4));
	}

	reg_base = C2KSYS_BASE + C2K_IRAM_BASE;
	for (j = 0; j < C2K_IRAM_DUMP_SIZE; ) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] iram = 0x%lx, value = 0x%x\n",
				reg_base + j, ccif_read32(c2k_iram_base_vir, j));
		j += 4;
	}

	reg_base = C2K_BOOT_ROM_BASE;
	for (j = 0; j < C2K_BOOTROM_DUMP_SIZE; ) {
		CCCI_MEM_LOG_TAG(md->index, TAG, "[C2K] bootrom = 0x%lx, value = 0x%x\n",
				reg_base + j, ccif_read32(c2k_boot_rom_vir, j));
		j += 4;
	}


}

