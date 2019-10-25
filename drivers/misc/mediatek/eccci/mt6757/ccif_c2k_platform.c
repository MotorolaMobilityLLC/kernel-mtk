/*
 *Copyright (C) 2015 MediaTek Inc.
 *
 *This program is free software: you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <mach/mtk_pbm.h>
#include <mtk_spm_sleep.h>

#include "ccci_config.h"
#include "ccci_modem.h"
#include "ccci_platform.h"
#include "ccif_c2k_platform.h"
#include "hif/ccci_hif_ccif.h"
#include "modem_reg_base.h"
#include "modem_sys.h"

#include <mt-plat/mtk_boot.h>
#include <mt-plat/upmu_common.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
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
static unsigned long apmcucfg_base;
static unsigned long apinfra_base;

struct c2k_pll_t c2k_pll_reg;

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
		CCCI_ERROR_LOG(dev_cfg->index, TAG,
			       "modem %d not enable, exit\n",
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
		CCCI_ERROR_LOG(dev_cfg->index, TAG,
			       "modem %d not enable, exit\n",
			       dev_cfg->index + 1);
		return -1;
	}
#endif

	switch (dev_cfg->index) {
	case 2: /*MD_SYS3 */
#ifdef CONFIG_OF
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,major",
				     &dev_cfg->major);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,minor_base",
				     &dev_cfg->minor_base);
		of_property_read_u32(dev_ptr->dev.of_node, "ccif,capability",
				     &dev_cfg->capability);

		hw_info->ap_ccif_base =
		    (unsigned long)of_iomap(dev_ptr->dev.of_node, 0);
		/*hw_info->md_ccif_base = hw_info->ap_ccif_base+0x1000; */
		node = of_find_compatible_node(NULL, NULL, "mediatek,md_ccif1");
		hw_info->md_ccif_base = (unsigned long)of_iomap(node, 0);

		hw_info->ap_ccif_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 0);
		hw_info->md_wdt_irq_id =
		    irq_of_parse_and_map(dev_ptr->dev.of_node, 1);

		/*Device tree using none flag to register irq, sensitivity has
		 * set at "irq_of_parse_and_map"
		 */
		hw_info->ap_ccif_irq_flags = IRQF_TRIGGER_NONE;
		hw_info->md_wdt_irq_flags = IRQF_TRIGGER_NONE;

		hw_info->md1_pccif_base =
		    (unsigned long)of_iomap(dev_ptr->dev.of_node, 1);
		hw_info->md3_pccif_base =
		    (unsigned long)of_iomap(dev_ptr->dev.of_node, 2);

		node =
		    of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
		hw_info->infra_ao_base = of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
		hw_info->sleep_base = of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL,
					       "mediatek,mt6757-toprgu");
		hw_info->toprgu_base = of_iomap(node, 0);

		node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
		apmixed_base = (unsigned long)of_iomap(node, 0);
		node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg");
		apmcucfg_base = (unsigned long)of_iomap(node, 0);
		node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg");
		apinfra_base = (unsigned long)of_iomap(node, 0);

		CCCI_NORMAL_LOG(
		    dev_cfg->index, TAG,
		    "infra_ao_base=0x%p, sleep_base=0x%p, toprgu_base=0x%p\n",
		    hw_info->infra_ao_base, hw_info->sleep_base,
		    hw_info->toprgu_base);

#endif

		hw_info->sram_size = CCIF_SRAM_SIZE;
		hw_info->md_rgu_base = MD3_RGU_BASE;

#if !defined(CONFIG_MTK_CLKMGR)
		clk_scp_sys_md3_main =
		    devm_clk_get(&dev_ptr->dev, "scp-sys-c2k-main");
		if (IS_ERR(clk_scp_sys_md3_main)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG,
				       "modem %d get scp-sys-c2k-main failed\n",
				       dev_cfg->index + 1);
			return -1;
		}
#endif

		/*no boot slave for md3 */

		break;
	default:
		return -1;
	}

	if (hw_info->ap_ccif_base == 0) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "ap_ccif_base:0x%p\n",
			       (void *)hw_info->ap_ccif_base);
		return -1;
	}
	if (hw_info->ap_ccif_irq_id == 0) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "ccif_irq_id:%d\n",
			       hw_info->ap_ccif_irq_id);
		return -1;
	}
	if (hw_info->md_wdt_irq_id == 0) {
		CCCI_ERROR_LOG(dev_cfg->index, TAG, "md_wdt_irq_id:%d\n",
			       hw_info->md_wdt_irq_id);
		return -1;
	}

	CCCI_INIT_LOG(dev_cfg->index, TAG,
		      "ap_ccif node info: major:%d, minor:%d, capability=%d, "
		      "ap_ccif_base=0x%p, ccif_irq=%d, md_wdt_irq=%d\n",
		      dev_cfg->major, dev_cfg->minor_base, dev_cfg->capability,
		      (void *)hw_info->ap_ccif_base, hw_info->ap_ccif_irq_id,
		      hw_info->md_wdt_irq_id);
	return 0;
}

int md_ccif_io_remap_md_side_register(struct ccci_modem *md)
{
	struct md_sys3_info *md_info = (struct md_sys3_info *)md->private_data;

	switch (md->index) {
	case MD_SYS3:
		c2k_pll_reg.c2k_pll_con3 =
		    ioremap_nocache(C2KSYS_BASE + C2K_C2K_PLL_CON3, 0x4);
		c2k_pll_reg.c2k_pll_con2 =
		    ioremap_nocache(C2KSYS_BASE + C2K_C2K_PLL_CON2, 0x4);
		c2k_pll_reg.c2k_plltd_con0 =
		    ioremap_nocache(C2KSYS_BASE + C2K_C2K_PLLTD_CON0, 0x4);
		c2k_pll_reg.c2k_cppll_con0 =
		    ioremap_nocache(C2KSYS_BASE + C2K_C2K_CPPLL_CON0, 0x4);
		c2k_pll_reg.c2k_dsppll_con0 =
		    ioremap_nocache(C2KSYS_BASE + C2K_C2K_DSPPLL_CON0, 0x4);
		c2k_pll_reg.c2k_c2kpll1_con0 =
		    ioremap_nocache(C2KSYS_BASE + C2K_C2K_C2KPLL1_CON0, 0x4);
		c2k_pll_reg.c2k_cg_amba_clksel =
		    ioremap_nocache(C2KSYS_BASE + C2K_CG_ARM_AMBA_CLKSEL, 0x4);
		c2k_pll_reg.c2k_clk_ctrl4 =
		    ioremap_nocache(C2KSYS_BASE + C2K_CLK_CTRL4, 0x4);
		c2k_pll_reg.c2k_clk_ctrl9 =
		    ioremap_nocache(C2KSYS_BASE + C2K_CLK_CTRL9, 0x4);
		/*CCIRQ reg */
		md_info->ccirq_base[0] =
		    ioremap_nocache(L1_C2K_CCIRQ_BASE, 0x100);
		md_info->ccirq_base[1] =
		    ioremap_nocache(C2K_L1_CCIRQ_BASE, 0x100);
		md_info->ccirq_base[2] =
		    ioremap_nocache(PS_C2K_CCIRQ_BASE, 0x100);
		md_info->ccirq_base[3] =
		    ioremap_nocache(C2K_PS_CCIRQ_BASE, 0x100);

		md_info->c2k_cgbr1_addr =
		    ioremap_nocache(C2KSYS_BASE + C2K_CGBR1, 0x4);
		if (!md->hw_info) {
			/*todo: why it is null? */
			CCCI_ERROR_LOG(md->index, TAG,
				       "md->hw_info is NULL!!!\n");
			md_info->md_rgu_base =
			    ioremap_nocache(MD3_RGU_BASE, 0x40);
		} else
			md_info->md_rgu_base =
			    ioremap_nocache(md->hw_info->md_rgu_base, 0x40);

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
	ccif_write16(c2k_pll_reg.c2k_dsppll_con0, 0,
		     ccif_read16(c2k_pll_reg.c2k_dsppll_con0, 0) | (0x1 << 15));

	udelay(30);

	ccif_write16(c2k_pll_reg.c2k_cg_amba_clksel, 0, 0xC124);
	ccif_write16(c2k_pll_reg.c2k_clk_ctrl4, 0, 0x8E43);
	ccif_write16(c2k_pll_reg.c2k_clk_ctrl9, 0, 0xA207);

	return 0;
}

static int reset_ccirq_hardware(struct ccci_modem *md)
{
	int i = 0;
	struct md_sys3_info *md_info = (struct md_sys3_info *)md->private_data;

	CCCI_NORMAL_LOG(MD_SYS3, TAG, "%s start\n", __func__);
	for (i = 0; i < 2; i++) {
		ccif_write32(md_info->ccirq_base[i], 0x4, 0xA00000FF);
		ccif_write32(md_info->ccirq_base[i], 0xC, 0xA00000FF);
	}
	for (i = 2; i < 4; i++) {
		ccif_write32(md_info->ccirq_base[i], 0x4, 0xA000000F);
		ccif_write32(md_info->ccirq_base[i], 0xC, 0xA000000F);
	}

	for (i = 0; i < 4; i++) {
		ccif_write32(md_info->ccirq_base[i], 0x40, 0x0);
		ccif_write32(md_info->ccirq_base[i], 0x44, 0x0);
		ccif_write32(md_info->ccirq_base[i], 0x48, 0x0);
		ccif_write32(md_info->ccirq_base[i], 0x4C, 0x0);
	}
	CCCI_NORMAL_LOG(MD_SYS3, TAG, "%s end\n", __func__);
	return 0;
}

/*need modify according to dummy ap*/
int md_ccif_let_md_go(struct ccci_modem *md)
{
	struct md_sys3_info *md_info = (struct md_sys3_info *)md->private_data;
	struct md_hw_info *hw_info = md->hw_info;

	if (MD_IN_DEBUG(md)) {
		CCCI_NORMAL_LOG(md->index, TAG, "DBG_FLAG_JTAG is set\n");
		return -1;
	}
	CCCI_BOOTUP_LOG(md->index, TAG, "%s\n", __func__);
	switch (md->index) {
	case MD_SYS3:
		/*check if meta mode */
		if (is_meta_mode() || get_boot_mode() == FACTORY_BOOT) {
			ccif_write32(
			    hw_info->infra_ao_base, C2K_CONFIG,
			    (ccif_read32(hw_info->infra_ao_base, C2K_CONFIG) |
			     ETS_SEL_BIT));
		}

		/*dump power status for debugging*/
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "[C2K] AP_PWR_STATUS = 0x%x\n",
		    ccif_read32(hw_info->sleep_base, AP_PWR_STATUS));
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "[C2K] AP_PWR_STATUS_2ND = 0x%x\n",
		    ccif_read32(hw_info->sleep_base, AP_PWR_STATUS_2ND));
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "SLEEP_CLK_CON = 0x%x\n",
		    ccif_read32(hw_info->sleep_base, SLEEP_CLK_CON));
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "AP_POWERON_CONFIG_EN = 0x%x\n",
		    ccif_read32(hw_info->sleep_base, AP_POWERON_CONFIG_EN));

		/*set c2k_md1src_req_en */
		ccif_write32(hw_info->infra_ao_base, C2K_CONFIG,
			     ccif_read32(hw_info->infra_ao_base, C2K_CONFIG) |
				 (0x1 << 3));

		/*step 1: config srcclkena selection mask */
		ccif_write32(hw_info->infra_ao_base, INFRA_MISC2,
			     ccif_read32(hw_info->infra_ao_base, INFRA_MISC2) |
				 INFRA_MISC2_C2K_EN);
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "INFRA_MISC2 = 0x%x\n",
		    ccif_read32(hw_info->infra_ao_base, INFRA_MISC2));

		/*step 2: config ClkSQ resigeter */
		/*ccif_write32(apmixed_base, AP_PLL_CON0,
		 * ccif_read32(apmixed_base, AP_PLL_CON0) | (0x1 << 1));
		 */
		ccif_write32(apmixed_base, MDPLL1_CON0,
			     ccif_read32(apmixed_base, MDPLL1_CON0) &
				 (~(0x1 << 9)));
		CCCI_BOOTUP_LOG(md->index, TAG, "AP_PLL_CON0 = 0x%x\n",
				ccif_read32(apmixed_base, AP_PLL_CON0));
		CCCI_BOOTUP_LOG(md->index, TAG, "MDPLL_CON0 = 0x%x\n",
				ccif_read32(apmixed_base, MDPLL1_CON0));
/*step 3: ap hold c2k core */
/*c2k may bootup fail if AP access C2K_CONFIG, so ignore */
/*
 *   ccif_write32(md_ctrl->hw_info->infra_ao_base, C2K_CONFIG,
 *	ccif_read32(md_ctrl->hw_info->infra_ao_base, C2K_CONFIG) | (0x1 << 1));
 */

/*step 4: wake up C2K */
#if 1
		ccif_write32(
		    hw_info->toprgu_base, TOP_RGU_WDT_SWSYSRST,
		    (ccif_read32(hw_info->toprgu_base, TOP_RGU_WDT_SWSYSRST) |
		     0x88000000) &
			(~(0x1 << 15)));
#else
		mtk_wdt_swsysret_config(0x1 << 15, 0);
#endif
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "[C2K] TOP_RGU_WDT_SWSYSRST = 0x%x\n",
		    ccif_read32(hw_info->toprgu_base, TOP_RGU_WDT_SWSYSRST));

		/*step 5: mpu already set */
		/*step 6: wake up C2K */
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "C2K_CONFIG = 0x%x\n",
		    ccif_read32(hw_info->infra_ao_base, C2K_CONFIG));
		CCCI_BOOTUP_LOG(
		    md->index, TAG, "[C2K] C2K_STATUS = 0x%x\n",
		    ccif_read32(hw_info->infra_ao_base, C2K_STATUS));
		ccif_write32(hw_info->infra_ao_base, INFRA_AO_C2K_HANDSHAKE,
			     ccif_read32(hw_info->infra_ao_base,
					 INFRA_AO_C2K_HANDSHAKE) |
				 (0x1 << 1));

		ccif_write32(hw_info->infra_ao_base, C2K_CONFIG,
			     (ccif_read32(hw_info->infra_ao_base, C2K_CONFIG) &
			      (~(0x3 << 11))));
		while (
		    !((ccif_read32(hw_info->infra_ao_base, C2K_STATUS) >> 1) &
		      0x1))
			;

		CCCI_BOOTUP_LOG(
		    md->index, TAG, "[C2K] C2K_STATUS = 0x%x\n",
		    ccif_read32(hw_info->infra_ao_base, C2K_STATUS));

		ccif_write32(hw_info->infra_ao_base, INFRA_AO_C2K_HANDSHAKE,
			     ccif_read32(hw_info->infra_ao_base,
					 INFRA_AO_C2K_HANDSHAKE) &
				 (~(0x1 << 1)));
		CCCI_BOOTUP_LOG(
		    md->index, TAG,
		    "[C2K] C2K_HANDSHAKE = 0x%x, C2K_STATUS = 0x%x\n",
		    ccif_read32(hw_info->infra_ao_base, INFRA_AO_C2K_HANDSHAKE),
		    ccif_read32(hw_info->infra_ao_base, C2K_STATUS));

		/*release bus protection */
		ccif_write32(hw_info->infra_ao_base, INFRA_TOPAXI_PROTECTEN_1,
			     ccif_read32(hw_info->infra_ao_base,
					 INFRA_TOPAXI_PROTECTEN_1) &
				 (0xFE3FFFFF));

		while (ccif_read32(md_info->c2k_cgbr1_addr, 0) != 0xFE8)
			;
		CCCI_BOOTUP_LOG(md->index, TAG, "[C2K] C2K_CGBR1 = 0x%x\n",
				ccif_read32(md_info->c2k_cgbr1_addr, 0));

		/*set c2k pll */
		config_c2k_pll();

		/*release c2k arm core */
		/*c2k may bootup fail if AP access C2K_CONFIG, so ignore */
		/*
		 *   ccif_write32(md_ctrl->hw_info->infra_ao_base, C2K_CONFIG,
		 *	ccif_read32(md_ctrl->hw_info->infra_ao_base, C2K_CONFIG)
		 *& (~(0x1 << 1)));
		 */

		break;
	}
	return 0;
}

int md_ccif_power_on(struct ccci_modem *md)
{
	int ret = 0;
	struct md_sys3_info *md_info = (struct md_sys3_info *)md->private_data;

	switch (md->index) {
	case MD_SYS3:
#if defined(CONFIG_MTK_CLKMGR)
		CCCI_NORMAL_LOG(md->index, TAG, "Call start md_power_on()\n");
		ret = md_power_on(SYS_MD2);
		CCCI_NORMAL_LOG(md->index, TAG,
				"Call end md_power_on() ret=%d\n", ret);
#else
		CCCI_NORMAL_LOG(md->index, TAG,
				"Call start clk_prepare_enable()\n");
		ret = clk_prepare_enable(clk_scp_sys_md3_main);
		CCCI_NORMAL_LOG(md->index, TAG,
				"Call end clk_prepare_enable() ret=%d\n", ret);
#endif
#ifndef JUST_FOR_BUILD
		kicker_pbm_by_md(KR_MD3, true);
#endif
		CCCI_NORMAL_LOG(md->index, TAG,
				"Call end kicker_pbm_by_md(3,true)\n");
		break;
	}
	CCCI_NORMAL_LOG(md->index, TAG, "%s:ret=%d\n", __func__, ret);
	if (ret == 0 && md->index != MD_SYS3) {
		/*disable MD WDT */
		ccif_write32(md_info->md_rgu_base, C2K_WDT_MD_MODE,
			     C2K_WDT_MD_MODE_KEY);
	}
	return ret;
}

int md_ccif_power_off(struct ccci_modem *md, unsigned int stop_type)
{
	int ret = 0;

	switch (md->index) {
	case MD_SYS2:
#if defined(CONFIG_MTK_CLKMGR)
		ret = md_power_off(SYS_MD2, stop_type);
#else
		clk_disable_unprepare(clk_scp_sys_md2_main);
#endif
		break;
	case MD_SYS3:
#if defined(CONFIG_MTK_CLKMGR)
		ret = md_power_off(SYS_MD3, stop_type);
#else
		clk_disable_unprepare(clk_scp_sys_md3_main);
#endif
#ifndef JUST_FOR_BUILD
		kicker_pbm_by_md(KR_MD3, false);
#endif
		CCCI_NORMAL_LOG(md->index, TAG,
				"Call end kicker_pbm_by_md(3,false)\n");
		break;
	}
	CCCI_NORMAL_LOG(md->index, TAG, "%s:ret=%d\n", __func__, ret);
	return ret;
}

void reset_md1_md3_pccif(struct ccci_modem *md)
{
	ccci_set_clk_cg(md, 1);
	reset_ccirq_hardware(md);
	ccci_set_clk_cg(md, 0);
}

void dump_c2k_register(struct ccci_modem *md, unsigned int dump_flag)
{
	CCCI_NORMAL_LOG(md->index, TAG, "INFRA_C2K_BOOT_STATUS = 0x%x\n",
			ccif_read32(apinfra_base, INFRA_C2K_BOOT_STATUS));
	CCCI_NORMAL_LOG(md->index, TAG, "INFRA_C2K_BOOT_STATUS2 = 0x%x\n",
			ccif_read32(apinfra_base, INFRA_C2K_BOOT_STATUS2));

	CCCI_NORMAL_LOG(md->index, TAG, "C2K_CONFIG = 0x%x\n",
			ccif_read32(md->hw_info->infra_ao_base, C2K_CONFIG));
	CCCI_NORMAL_LOG(md->index, TAG, "[C2K] C2K_STATUS = 0x%x\n",
			ccif_read32(md->hw_info->infra_ao_base, C2K_STATUS));

	if (dump_flag == 0)
		return;
}
