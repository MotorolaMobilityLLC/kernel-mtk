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
#endif /*CONFIG_MTK_CLKMGR */
#include <mt-plat/upmu_common.h>
#include <mach/mt_pbm.h>
#include <mt_spm_sleep.h>


#include "ccci_modem.h"
#include "ccci_platform.h"
#include "modem_cldma.h"
#include "cldma_platform.h"
#include "cldma_reg.h"
#include "modem_reg_base.h"

#ifdef FEATURE_RF_CLK_BUF
#include <mt_clkbuf_ctl.h>
#endif
#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
#include <mach/mt6605.h>
#endif

#if !defined(CONFIG_MTK_CLKMGR)
static struct clk *clk_scp_sys_md1_main;
#endif

static struct pinctrl *mdcldma_pinctrl;

#define TAG "mcd"
void md_cldma_hw_reset(struct ccci_modem *md)
{
	unsigned int reg_value;

	CCCI_DEBUG_LOG(md->index, TAG, "md_cldma_hw_reset:rst cldma\n");
	/* reset cldma hw */
	reg_value = ccci_read32(infra_ao_base, INFRA_RST0_REG);
	reg_value &= ~(CLDMA_AO_RST_MASK | CLDMA_PD_RST_MASK);
	reg_value |= (CLDMA_AO_RST_MASK | CLDMA_PD_RST_MASK);
	ccci_write32(infra_ao_base, INFRA_RST0_REG, reg_value);
	CCCI_DEBUG_LOG(md->index, TAG, "md_cldma_hw_reset:clear reset\n");
	/* reset cldma clr */
	reg_value = ccci_read32(infra_ao_base, INFRA_RST1_REG);
	reg_value &= ~(CLDMA_AO_RST_MASK | CLDMA_PD_RST_MASK);
	reg_value |= (CLDMA_AO_RST_MASK | CLDMA_PD_RST_MASK);
	ccci_write32(infra_ao_base, INFRA_RST1_REG, reg_value);
	CCCI_DEBUG_LOG(md->index, TAG, "md_cldma_hw_reset:done\n");
}

int md_cd_get_modem_hw_info(struct platform_device *dev_ptr, struct ccci_dev_cfg *dev_cfg, struct md_hw_info *hw_info)
{
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
		hw_info->md_boot_slave_Vector = MD_BOOT_VECTOR;
		hw_info->md_boot_slave_Key = MD_BOOT_VECTOR_KEY;
		hw_info->md_boot_slave_En = MD_BOOT_VECTOR_EN;
		mdcldma_pinctrl = devm_pinctrl_get(&dev_ptr->dev);
		if (IS_ERR(mdcldma_pinctrl)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get mdcldma_pinctrl failed\n",
							dev_cfg->index + 1);
			return -1;
		}

#if !defined(CONFIG_MTK_CLKMGR)
		clk_scp_sys_md1_main = devm_clk_get(&dev_ptr->dev, "scp-sys-md1-main");
		if (IS_ERR(clk_scp_sys_md1_main)) {
			CCCI_ERROR_LOG(dev_cfg->index, TAG, "modem %d get scp-sys-md1-main failed\n",
							dev_cfg->index + 1);
			return -1;
		}
#endif
		break;
	default:
		return -1;
	}

	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "dev_major:%d,minor_base:%d,capability:%d\n",
			dev_cfg->major, dev_cfg->minor_base, dev_cfg->capability);
	CCCI_DEBUG_LOG(dev_cfg->index, TAG,
			"ap_cldma: ao_base=0x%p, pdn_base=0x%p,md_cldma: ao_base=0x%p, pdn_base=0x%p\n",
			(void *)hw_info->cldma_ap_ao_base, (void *)hw_info->cldma_ap_pdn_base,
			(void *)hw_info->cldma_md_ao_base, (void *)hw_info->cldma_md_pdn_base);

	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "ap_ccif_base:0x%p, md_ccif_base:0x%p\n", (void *)hw_info->ap_ccif_base,
			(void *)hw_info->md_ccif_base);
	CCCI_DEBUG_LOG(dev_cfg->index, TAG, "cldma_irq:%d,ccif_irq:%d,md_wdt_irq:%d\n", hw_info->cldma_irq_id,
			hw_info->ap_ccif_irq_id, hw_info->md_wdt_irq_id);

	return 0;
}

int md_cd_io_remap_md_side_register(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	md_ctrl->cldma_ap_pdn_base = (void __iomem *)(md_ctrl->hw_info->cldma_ap_pdn_base);
	md_ctrl->cldma_ap_ao_base = (void __iomem *)(md_ctrl->hw_info->cldma_ap_ao_base);
	md_ctrl->cldma_md_pdn_base = (void __iomem *)(md_ctrl->hw_info->cldma_md_pdn_base);
	md_ctrl->cldma_md_ao_base = (void __iomem *)(md_ctrl->hw_info->cldma_md_ao_base);
	md_ctrl->md_boot_slave_Vector = ioremap_nocache(md_ctrl->hw_info->md_boot_slave_Vector, 0x4);
	md_ctrl->md_boot_slave_Key = ioremap_nocache(md_ctrl->hw_info->md_boot_slave_Key, 0x4);
	md_ctrl->md_boot_slave_En = ioremap_nocache(md_ctrl->hw_info->md_boot_slave_En, 0x4);
	md_ctrl->md_rgu_base = ioremap_nocache(md_ctrl->hw_info->md_rgu_base, 0x40);
	md_ctrl->md_global_con0 = ioremap_nocache(MD_GLOBAL_CON0, 0x4);

	md_ctrl->md_bus_status = ioremap_nocache(MD_BUS_STATUS_BASE, MD_BUS_STATUS_LENGTH);
	md_ctrl->md_pc_monitor = ioremap_nocache(MD_PC_MONITOR_BASE, MD_PC_MONITOR_LENGTH);
	md_ctrl->md_topsm_status = ioremap_nocache(MD_TOPSM_STATUS_BASE, MD_TOPSM_STATUS_LENGTH);
	md_ctrl->md_ost_status = ioremap_nocache(MD_OST_STATUS_BASE, MD_OST_STATUS_LENGTH);
	md_ctrl->md_pll = ioremap_nocache(MD_PLL_BASE, MD_PLL_LENGTH);
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
}

void md_cd_dump_debug_register(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
	unsigned int reg_value;

	md_cd_lock_modem_clock_src(1);
	CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD Bus status %x\n", MD_BUS_STATUS_BASE);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_bus_status, MD_BUS_STATUS_LENGTH);
	CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD PC monitor %x\n", MD_PC_MONITOR_BASE);
	/* stop MD PCMon */
	reg_value = ccci_read32(md_ctrl->md_pc_monitor, 0);
	reg_value &= ~(0x1 << 21);
	ccci_write32(md_ctrl->md_pc_monitor, 0, reg_value);	/* clear bit[21] */
	ccci_write32((md_ctrl->md_pc_monitor + 4), 0, 0x80000000);	/* stop MD PCMon */
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_pc_monitor, MD_PC_MONITOR_LENGTH);
	ccci_write32(md_ctrl->md_pc_monitor + 4, 0, 0x1);	/* restart MD PCMon */
	CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD TOPSM status %x\n", MD_TOPSM_STATUS_BASE);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_topsm_status, MD_TOPSM_STATUS_LENGTH);
	CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD OST status %x\n", MD_OST_STATUS_BASE);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_ost_status, MD_OST_STATUS_LENGTH);
	CCCI_MEM_LOG_TAG(md->index, TAG, "Dump MD PLL %x\n", MD_PLL_BASE);
	ccci_util_mem_dump(md->index, CCCI_DUMP_MEM_DUMP, md_ctrl->md_pll, MD_PLL_LENGTH);
	md_cd_lock_modem_clock_src(0);
}

void md_cd_check_md_DCM(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	md_cd_lock_modem_clock_src(1);
	CCCI_NORMAL_LOG(md->index, TAG, "MD DCM: 0x%X\n", *(unsigned int *)(md_ctrl->md_bus_status + 0x45C));
	md_cd_lock_modem_clock_src(0);
}

void md_cd_check_emi_state(struct ccci_modem *md, int polling)
{
}
/* callback for system power off*/
void ccci_power_off(void)
{
	/*ALPS02057700 workaround:
	* Power on VLTE for system power off backlight work normal
	*/
	CCCI_NORMAL_LOG(-1, CORE, "ccci_power_off:set VLTE on,bit0,1\n");
	pmic_config_interface(0x04D6, 0x1, 0x1, 0); /* bit[0] =>1'b1 */
	udelay(200);
}

int md_cd_power_on(struct ccci_modem *md)
{
	int ret = 0;
	unsigned int reg_value;
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;
#if defined(FEATURE_RF_CLK_BUF)
	struct pinctrl_state *RFIC0_01_mode;
#endif

	/* turn on VLTE */
#ifdef FEATURE_VLTE_SUPPORT
	struct pinctrl_state *vsram_output_high;

	if (NULL != mdcldma_pinctrl) {
		vsram_output_high = pinctrl_lookup_state(mdcldma_pinctrl, "vsram_output_high");
		if (IS_ERR(vsram_output_high)) {
			CCCI_NORMAL_LOG(md->index, CORE, "cannot find vsram_output_high pintrl. ret=%ld\n",
				     PTR_ERR(vsram_output_high));
		}
		pinctrl_select_state(mdcldma_pinctrl, vsram_output_high);
	} else {
		CCCI_NORMAL_LOG(md->index, CORE, "mdcldma_pinctrl is NULL, some error happend.\n");
	}
	CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_on:mt_set_gpio_out(GPIO_LTE_VSRAM_EXT_POWER_EN_PIN,1)\n");

	/* if(!(mt6325_upmu_get_swcid()==PMIC6325_E1_CID_CODE || */
	/* mt6325_upmu_get_swcid()==PMIC6325_E2_CID_CODE)) */
	{
		CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_on:set VLTE on,bit0,1\n");
		pmic_config_interface(0x04D6, 0x1, 0x1, 0);	/* bit[0] =>1'b1 */
		udelay(200);
		/*
		 *[Notes] move into md cmos flow, for hardwareissue, so disable on denlai.
		 * bring up need confirm with MD DE & SPM
		 */
		/* reg_value = ccci_read32(infra_ao_base,0x338); */
		/* reg_value &= ~(0x40); //bit[6] =>1'b0 */
		/* ccci_write32(infra_ao_base,0x338,reg_value); */
		/* CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_on: set infra_misc VLTE bit(0x1000_0338)=0x%x,
		bit[6]=0x%x\n",ccci_read32(infra_ao_base,0x338),(ccci_read32(infra_ao_base,0x338)&0x40)); */
	}
#endif
	reg_value = ccci_read32(infra_ao_base, 0x338);
	reg_value &= ~(0x3 << 2);	/* md1_srcclkena */
	reg_value |= (0x1 << 2);
	ccci_write32(infra_ao_base, 0x338, reg_value);
	CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_on: set md1_srcclkena bit(0x1000_0338)=0x%x\n",
		     ccci_read32(infra_ao_base, 0x338));
#ifdef FEATURE_RF_CLK_BUF
	/* config RFICx as BSI */
	mutex_lock(&clk_buf_ctrl_lock);	/* fixme,clkbuf, ->down(&clk_buf_ctrl_lock_2); */
	CCCI_NORMAL_LOG(md->index, TAG, "clock buffer, BSI ignore mode\n");
	if (NULL != mdcldma_pinctrl) {
		RFIC0_01_mode = pinctrl_lookup_state(mdcldma_pinctrl, "RFIC0_01_mode");
		pinctrl_select_state(mdcldma_pinctrl, RFIC0_01_mode);
	}
#endif
	/* power on MD_INFRA and MODEM_TOP */
	switch (md->index) {
	case MD_SYS1:

#if defined(CONFIG_MTK_CLKMGR)
		CCCI_NORMAL_LOG(md->index, TAG, "Call start md_power_on()\n");
		ret = md_power_on(SYS_MD1);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end md_power_on() ret=%d\n", ret);
#else
		CCCI_NORMAL_LOG(md->index, TAG, "Call start clk_prepare_enable()\n");
		ret = clk_prepare_enable(clk_scp_sys_md1_main);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end clk_prepare_enable()ret=%d\n", ret);
#endif

		kicker_pbm_by_md(MD1, true);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end kicker_pbm_by_md(0,true)\n");
		break;
	}
#ifdef FEATURE_RF_CLK_BUF
	mutex_unlock(&clk_buf_ctrl_lock);	/* fixme,clkbuf, ->delete */
#endif

#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
	/* notify NFC */
	inform_nfc_vsim_change(md->index, 1, 0);
#endif

	/* disable MD WDT */
	cldma_write32(md_ctrl->md_rgu_base, WDT_MD_MODE, WDT_MD_MODE_KEY);
	return ret;
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
	CCCI_NORMAL_LOG(md->index, TAG, "set MD boot slave\n");
	/* set the start address to let modem to run */
	cldma_write32(md_ctrl->md_boot_slave_Key, 0, 0x3567C766);	/* make boot vector programmable */
	cldma_write32(md_ctrl->md_boot_slave_Vector, 0, 0x00000000);
		/* after remap, MD ROM address is 0 from MD's view */
	cldma_write32(md_ctrl->md_boot_slave_En, 0, 0xA3B66175);	/* make boot vector take effect */
	return 0;
}

int md_cd_soft_power_off(struct ccci_modem *md, unsigned int mode)
{
	return 0;
}

int md_cd_soft_power_on(struct ccci_modem *md, unsigned int mode)
{
	return 0;
}

int md_cd_power_off(struct ccci_modem *md, unsigned int stop_type)
{
	int ret = 0;
	int count = 50;
	unsigned int reg_value;
#if defined(FEATURE_RF_CLK_BUF)
	struct pinctrl_state *RFIC0_04_mode;
#endif
#if defined(FEATURE_VLTE_SUPPORT)
	struct pinctrl_state *vsram_output_low;
#endif
#if defined(CONFIG_MTK_CLKMGR)
	unsigned int timeout = 0;

	if (stop_type == MD_FLIGHT_MODE_ENTER)
		timeout = 1000;
#endif
#ifdef FEATURE_INFORM_NFC_VSIM_CHANGE
	/* notify NFC */
	inform_nfc_vsim_change(md->index, 0, 0);
#endif
	while (spm_is_md1_sleep() == 0) {
		msleep(20);
		count--;
		if (count == 0) {
			CCCI_NORMAL_LOG(md->index, TAG, "%s:poll md sleep timeout: %d",
				__func__, spm_is_md1_sleep());
			break;
		}
	}
#ifdef FEATURE_RF_CLK_BUF
	mutex_lock(&clk_buf_ctrl_lock);
#endif
	/* power off MD_INFRA and MODEM_TOP */
	switch (md->index) {
	case MD_SYS1:
#if defined(CONFIG_MTK_CLKMGR)
		ret = md_power_off(SYS_MD1, timeout);
#else
		clk_disable(clk_scp_sys_md1_main);
#ifdef FEATURE_RF_CLK_BUF
		mutex_unlock(&clk_buf_ctrl_lock);
#endif
		clk_unprepare(clk_scp_sys_md1_main);	/* cannot be called in mutex context */
#ifdef FEATURE_RF_CLK_BUF
		mutex_lock(&clk_buf_ctrl_lock);
#endif
#endif
		kicker_pbm_by_md(MD1, false);
		CCCI_NORMAL_LOG(md->index, TAG, "Call end kicker_pbm_by_md(0,false)\n");
		break;
	}
#ifdef FEATURE_RF_CLK_BUF
	/* config RFICx as AP SPM control */
	CCCI_NORMAL_LOG(md->index, TAG, "clock buffer, AP SPM control mode\n");
	RFIC0_04_mode = pinctrl_lookup_state(mdcldma_pinctrl, "RFIC0_04_mode");
	pinctrl_select_state(mdcldma_pinctrl, RFIC0_04_mode);
	mutex_unlock(&clk_buf_ctrl_lock);
#endif
	reg_value = ccci_read32(infra_ao_base, 0x338);
	reg_value &= ~(0x3 << 2);	/* md1_srcclkena */
	ccci_write32(infra_ao_base, 0x338, reg_value);
	CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_off: set md1_srcclkena bit(0x1000_0338)=0x%x\n",
		     ccci_read32(infra_ao_base, 0x338));
#ifdef FEATURE_VLTE_SUPPORT
	/* Turn off VLTE */
	/* if(!(mt6325_upmu_get_swcid()==PMIC6325_E1_CID_CODE || */
	/* mt6325_upmu_get_swcid()==PMIC6325_E2_CID_CODE)) */
	{
		/*
		 *[Notes] move into md cmos flow, for hardwareissue, so disable on denlai.
		 * bring up need confirm with MD DE & SPM
		 */
		/* reg_value = ccci_read32(infra_ao_base,0x338); */
		/* reg_value &= ~(0x40); //bit[6] =>1'b0 */
		/* reg_value |= 0x40;//bit[6] =>1'b1 */
		/* ccci_write32(infra_ao_base,0x338,reg_value); */
		/* CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_off: set SRCLKEN infra_misc(0x1000_0338)=0x%x,
		bit[6]=0x%x\n", ccci_read32(infra_ao_base, 0x338), (ccci_read32(infra_ao_base,0x338)&0x40)); */

		CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_off:set VLTE on,bit0=0\n");
		pmic_config_interface(0x04D6, 0x0, 0x1, 0);	/* bit[0] =>1'b0 */
	}
	if (NULL != mdcldma_pinctrl) {
		vsram_output_low = pinctrl_lookup_state(mdcldma_pinctrl, "vsram_output_low");
		if (IS_ERR(vsram_output_low)) {
			CCCI_NORMAL_LOG(md->index, CORE, "cannot find vsram_output_low pintrl. ret=%ld\n",
				     PTR_ERR(vsram_output_low));
		}
		pinctrl_select_state(mdcldma_pinctrl, vsram_output_low);
	} else {
		CCCI_NORMAL_LOG(md->index, CORE, "mdcldma_pinctrl is NULL, some error happend.\n");
	}
	CCCI_NORMAL_LOG(md->index, CORE, "md_cd_power_off:mt_set_gpio_out(GPIO_LTE_VSRAM_EXT_POWER_EN_PIN,0)\n");
#endif
	return ret;
}

void cldma_dump_register(struct ccci_modem *md)
{
	struct md_cd_ctrl *md_ctrl = (struct md_cd_ctrl *)md->private_data;

	CCCI_NORMAL_LOG(md->index, TAG, "dump AP CLDMA Tx pdn register, active=%x\n", md_ctrl->txq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_UL_START_ADDR_0,
		      CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE - CLDMA_AP_UL_START_ADDR_0 + 4);
	CCCI_NORMAL_LOG(md->index, TAG, "dump AP CLDMA Tx ao register, active=%x\n", md_ctrl->txq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_ap_ao_base + CLDMA_AP_UL_START_ADDR_BK_0,
		      CLDMA_AP_UL_CURRENT_ADDR_BK_7 - CLDMA_AP_UL_START_ADDR_BK_0 + 4);

	CCCI_NORMAL_LOG(md->index, TAG, "dump AP CLDMA Rx pdn register, active=%x\n", md_ctrl->rxq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_SO_ERROR,
		      CLDMA_AP_SO_STOP_CMD - CLDMA_AP_SO_ERROR + 4);

	CCCI_NORMAL_LOG(md->index, TAG, "dump AP CLDMA Rx ao register, active=%x\n", md_ctrl->rxq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_ap_ao_base + CLDMA_AP_SO_CFG,
		      CLDMA_AP_DEBUG_ID_EN - CLDMA_AP_SO_CFG + 4);

	CCCI_NORMAL_LOG(md->index, TAG, "dump AP CLDMA MISC pdn register\n");
	ccci_mem_dump(md->index, md_ctrl->cldma_ap_pdn_base + CLDMA_AP_L2TISAR0,
		      CLDMA_AP_CLDMA_IP_BUSY - CLDMA_AP_L2TISAR0 + 4);

	CCCI_NORMAL_LOG(md->index, TAG, "dump AP CLDMA MISC ao register\n");
	ccci_mem_dump(md->index, md_ctrl->cldma_ap_ao_base + CLDMA_AP_L2RIMR0, CLDMA_AP_DUMMY - CLDMA_AP_L2RIMR0 + 4);

	CCCI_NORMAL_LOG(md->index, TAG, "dump MD CLDMA Tx pdn register, active=%x\n", md_ctrl->txq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_md_pdn_base + CLDMA_AP_UL_START_ADDR_0,
		      CLDMA_AP_UL_CHECKSUM_CHANNEL_ENABLE - CLDMA_AP_UL_START_ADDR_0 + 4);
	CCCI_NORMAL_LOG(md->index, TAG, "dump MD CLDMA Tx ao register, active=%x\n", md_ctrl->txq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_md_ao_base + CLDMA_AP_UL_START_ADDR_BK_0,
		      CLDMA_AP_UL_CURRENT_ADDR_BK_7 - CLDMA_AP_UL_START_ADDR_BK_0 + 4);

	CCCI_NORMAL_LOG(md->index, TAG, "dump MD CLDMA Rx pdn register, active=%x\n", md_ctrl->rxq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_md_pdn_base + CLDMA_AP_SO_ERROR,
		      CLDMA_AP_SO_STOP_CMD - CLDMA_AP_SO_ERROR + 4);
	CCCI_NORMAL_LOG(md->index, TAG, "dump MD CLDMA Rx ao register, active=%x\n", md_ctrl->rxq_active);
	ccci_mem_dump(md->index, md_ctrl->cldma_md_ao_base + CLDMA_AP_SO_CFG,
		      CLDMA_AP_DEBUG_ID_EN - CLDMA_AP_SO_CFG + 4);

	CCCI_NORMAL_LOG(md->index, TAG, "dump MD CLDMA MISC pdn register\n");
	ccci_mem_dump(md->index, md_ctrl->cldma_md_pdn_base + CLDMA_AP_L2TISAR0,
		      CLDMA_AP_CLDMA_IP_BUSY - CLDMA_AP_L2TISAR0 + 4);
	CCCI_NORMAL_LOG(md->index, TAG, "dump MD CLDMA MISC ao register\n");
	ccci_mem_dump(md->index, md_ctrl->cldma_md_ao_base + CLDMA_AP_L2RIMR0, CLDMA_AP_DUMMY - CLDMA_AP_L2RIMR0 + 4);

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

	CCCI_DEBUG_LOG(md->index, TAG, "ccci_modem_suspend\n");
	return 0;
}

int ccci_modem_resume(struct platform_device *dev)
{
	struct ccci_modem *md = (struct ccci_modem *)dev->dev.platform_data;

	CCCI_DEBUG_LOG(md->index, TAG, "ccci_modem_resume\n");
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
		CCCI_DEBUG_LOG(md->index, TAG, "Resume cldma pdn register: No need  ...\n");
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
				CCCI_DEBUG_LOG(md->index, TAG, "Resume CH(%d) current bak:== 0\n", i);
				cldma_reg_set_tx_start_addr(md_ctrl->cldma_ap_pdn_base, md_ctrl->txq[i].index,
					md_ctrl->txq[i].tr_done->gpd_addr);
				cldma_reg_set_tx_start_addr_bk(md_ctrl->cldma_ap_ao_base, md_ctrl->txq[i].index,
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
		cldma_write32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_START_CMD, CLDMA_BM_ALL_QUEUE);
		cldma_read32(md_ctrl->cldma_ap_pdn_base, CLDMA_AP_UL_START_CMD);	/* dummy read */
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
		CCCI_DEBUG_LOG(md->index, TAG, "Resume cldma pdn register done\n");
	}
}

int ccci_modem_syssuspend(void)
{
	CCCI_DEBUG_LOG(0, TAG, "ccci_modem_syssuspend\n");
	return 0;
}

void ccci_modem_sysresume(void)
{
	struct ccci_modem *md;

	CCCI_DEBUG_LOG(0, TAG, "ccci_modem_sysresume\n");
	md = ccci_md_get_modem_by_id(0);
	if (md != NULL)
		ccci_modem_restore_reg(md);
}
